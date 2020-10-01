/* - DownZemAll! - Copyright (C) 2019-present Sebastien Vavassori
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; If not, see <http://www.gnu.org/licenses/>.
 */

#include "dummytorrentanimator.h"

#include "dummytorrentfactory.h"

#include <Core/Torrent>

#include <QtCore/QDebug>
#include <QtCore/QtMath>
#include <QtCore/QTimer>

constexpr int msec_file_refresh = 10;
constexpr int msec_peer_refresh = 500;

/*!
 * Return a random number between \a a and \a b, including these numbers.
 */
namespace Utils {
int randomBetween(int a, int b)
{
    const int rand = qrand(); // Beween 0 and RAND_MAX
    const int offset = qMin(a, b);
    const int range = qMax(a, b) - offset;
    return (((range * rand) / RAND_MAX) + offset);
}
}


DummyTorrentAnimator::DummyTorrentAnimator(QObject *parent)
    : QObject(parent)
{
    qsrand(QDateTime::currentDateTimeUtc().toTime_t());

    connect(&m_fileTimer, &QTimer::timeout, this, QOverload<>::of(&DummyTorrentAnimator::animate));
    connect(&m_peerTimer, &QTimer::timeout, this, QOverload<>::of(&DummyTorrentAnimator::animatePeers));

}

/******************************************************************************
 ******************************************************************************/
Torrent *DummyTorrentAnimator::torrent() const
{
    return m_torrent;
}

void DummyTorrentAnimator::setTorrent(Torrent *torrent)
{
    m_torrent = torrent;
}

/******************************************************************************
 ******************************************************************************/
/**
 * @brief DummyTorrentAnimator::setProgress
 * @param torrent Torrent
 * @param percent Progress between 0 (stopped) and 100 (completed)
 */
void DummyTorrentAnimator::setProgress(int percent)
{
    Q_ASSERT(m_torrent);

    const TorrentMetaInfo metaInfo = m_torrent->metaInfo();
    TorrentInfo info = m_torrent->info();

    auto pieceCount = metaInfo.initialMetaInfo.pieceCount;
    auto pieceByteSize = metaInfo.initialMetaInfo.pieceByteSize;

    Q_ASSERT(pieceByteSize > 0);

    qint64 bytesReceived = 0;

    // First, create a random piece map
    info.downloadedPieces = createRandomBitArray(static_cast<int>(pieceCount), percent);

    TorrentHandleInfo detail = m_torrent->detail();
    int total = metaInfo.initialMetaInfo.files.count();
    for (int i = 0; i < total; ++i) {
        auto fileMetaInfo = metaInfo.initialMetaInfo.files.at(i);

        qint64 bytesOffset = fileMetaInfo.bytesOffset;
        qint64 bytesTotal = fileMetaInfo.bytesTotal;

        auto firstPieceIndex = qCeil(qreal(bytesOffset) / pieceByteSize);
        auto lastPieceIndex = qCeil(qreal(bytesOffset + bytesTotal) / pieceByteSize);
        auto filePieceCount = 1 + lastPieceIndex - firstPieceIndex;
        filePieceCount = qMin(filePieceCount, pieceCount);

        // Count pieces for each file
        qint64 received = 0;
        for (int j = 0; j < filePieceCount; ++j) {
            if (info.downloadedPieces.testBit(j)) {
                received++;
            }
        }
        received *= pieceByteSize;
        received = qMin(received, bytesTotal);

        detail.files[i].bytesReceived = received;
        bytesReceived += received;
    }

    bytesReceived = qMin(bytesReceived, info.bytesTotal);

    info.bytesReceived = bytesReceived;
    info.bytesSessionDownloaded = bytesReceived;
    info.bytesSessionUploaded = bytesReceived >> 2;

    if (percent <= 0) {
        info.state = TorrentInfo::stopped;
    } else if (percent >= 1) {
        info.state = TorrentInfo::seeding;
    } else {
        info.state = TorrentInfo::downloading;
    }

    m_torrent->setInfo(info, false);
    m_torrent->setDetail(detail, false); // emit changed
}

/******************************************************************************
 ******************************************************************************/
QBitArray DummyTorrentAnimator::createRandomBitArray(int size, int percent)
{
    if (percent <= 0) {
        return QBitArray(size, false);
    }
    if (percent >= 100) {
        return QBitArray(size, true);
    }
    QBitArray ba = QBitArray(size, false);
    if (percent == 50) {
        for (int i = 0; i < size; i+=2) {
            ba.setBit(i);
        }
    } else {
        qsrand(QDateTime::currentDateTime().toTime_t());
        for (int i = 0; i < size; ++i) {
            const int v = Utils::randomBetween(0, 100);
            if (v <= percent) {
                ba.setBit(i);
            }
        }
    }
    return ba;
}

/******************************************************************************
 ******************************************************************************/
bool DummyTorrentAnimator::isStarted() const
{
    return m_fileTimer.isActive();
}


void DummyTorrentAnimator::start()
{
    if (isStarted()) {
        return;
    }
    randomize();

    m_fileTimer.start(msec_file_refresh);
    m_peerTimer.start(msec_peer_refresh);

    emit started(true);
}

void DummyTorrentAnimator::stop()
{
    m_fileTimer.stop();
    m_peerTimer.stop();
    emit started(false);
}

/******************************************************************************
 ******************************************************************************/
void DummyTorrentAnimator::animate()
{
    if (m_ticks.count() != m_timeouts.count()) {
        m_ticks = m_timeouts;
    }
    for (int i = 0, total = m_ticks.count(); i < total; ++i) {
        m_ticks[i]--;
        if (m_ticks[i] <= 0) {
            m_ticks[i] = m_timeouts[i];
            animateFile(i);
        }
    }
    animatePieces();
}

/******************************************************************************
 ******************************************************************************/
void DummyTorrentAnimator::animateFile(int index)
{
    Q_ASSERT(m_torrent);

    // qDebug() << Q_FUNC_INFO << index;
    const qint64 receivedPieceSize = 32*1024*8;

    TorrentMetaInfo metaInfo = m_torrent->metaInfo();
    auto bytesTotal = metaInfo.initialMetaInfo.files.at(index).bytesTotal;

    TorrentHandleInfo detail = m_torrent->detail();
    detail.files[index].bytesReceived += receivedPieceSize;
    detail.files[index].bytesReceived = qMin(detail.files[index].bytesReceived, bytesTotal);

    TorrentInfo info = m_torrent->info();
    info.bytesReceived += receivedPieceSize;
    info.bytesReceived = qMin(info.bytesReceived, info.bytesTotal);
    info.bytesSessionDownloaded += receivedPieceSize;
    info.bytesSessionUploaded += receivedPieceSize >> 2;
    info.state = TorrentInfo::downloading;

    m_torrent->setInfo(info, false);
    m_torrent->setDetail(detail, false); // emit changed
}

void DummyTorrentAnimator::animatePieces()
{
    TorrentInfo info = m_torrent->info();
    setPiecesRandomly(info.downloadedPieces);
    if (info.downloadedPieces.count(true) >=  info.downloadedPieces.count()) {
        setPiecesRandomly(info.verifiedPieces);
    }
    m_torrent->setInfo(info, false);
}

void DummyTorrentAnimator::animatePeers()
{
    static int counter = 0;
    counter += msec_peer_refresh;
    const TorrentInfo info = m_torrent->info();
    const int total_pieces_count = info.downloadedPieces.count();

    TorrentHandleInfo detail = m_torrent->detail();
    const int count = detail.peers.count();

    if (count >= 10) {
        for (int i = 0; i < 7; ++i) {
            detail.peers.removeFirst(); // keep 3
        }
    }
    if (counter >= 10000) {//every 10 seconds
        counter = 0;
        auto fct = DummyTorrentFactory::createDummyPeer;
        detail.peers << fct(EndPoint("164.10.201.129:30025"), "XA-AA----A", "rTorrent v1.2.3", total_pieces_count);
        detail.peers << fct(EndPoint("103.217.176.75:44851"), "XXXXXXXXXX", "", total_pieces_count);
        detail.peers << fct(EndPoint("217.63.14.13:14082"  ), "-X-A-----A", "bitTorrent", total_pieces_count);
        detail.peers << fct(EndPoint("178.214.192.253:6881"), "-----X----", "toto", total_pieces_count);
        detail.peers << fct(EndPoint("71.206.231.37:49958" ), "-A-------A", "libTorrent", total_pieces_count);
        detail.peers << fct(EndPoint("82.69.12.239:59333"  ), "----------", "qBitTorrent", total_pieces_count);
        detail.peers << fct(EndPoint("86.120.101.138:42624"), "XA--------", "", total_pieces_count);
        detail.peers << fct(EndPoint("175.158.201.29:32725"), "XXXXXX--X-", "", total_pieces_count);

    } else {
        QString randomIP = QString("%0.%1.%2.%3:%4").arg(
                    QString::number(Utils::randomBetween(1, 255)),
                    QString::number(Utils::randomBetween(1, 255)),
                    QString::number(Utils::randomBetween(1, 255)),
                    QString::number(Utils::randomBetween(1, 255)),
                    QString::number(Utils::randomBetween(1, 65535)));
        auto fct = DummyTorrentFactory::createDummyPeer;
        detail.peers << fct(EndPoint(randomIP), "----------", "", total_pieces_count);

    }
    m_torrent->setDetail(detail, false); // emit changed
}

/******************************************************************************
 ******************************************************************************/
void DummyTorrentAnimator::setPiecesRandomly(QBitArray &pieces)
{
    const int count = pieces.count();
    if (pieces.count(true) >  count - 10) {
        pieces.fill(true);
        return;
    }

    // turn random bits to 1
    for (int i = 0; i < 3; ++i) {
        const int index = Utils::randomBetween(0, count - 1);
        pieces.setBit(index);
    }

    // turn first 0-bits to 1
    int i = -1;
    int counter = 0;
    while (i < count - 1 && counter < 3) {
        i++;
        if (pieces.testBit(i) == false) {
            pieces.setBit(i, true);
            counter++;
        }
    }
}

/******************************************************************************
 ******************************************************************************/
void DummyTorrentAnimator::randomize()
{
    Q_ASSERT(m_torrent);
    int total = m_torrent->metaInfo().initialMetaInfo.files.count();
    m_timeouts.clear();
    m_ticks.clear();
    for (int i = 0; i < total; ++i) {
        int timeout = msec_file_refresh * Utils::randomBetween(1, 10);
        m_timeouts << timeout;
    }
}
