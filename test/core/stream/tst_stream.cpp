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

#include <Core/Stream>

#include "../../utils/biginteger.h"
#include "../../utils/dummystreamfactory.h"

#include <QtCore/QDebug>
#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>

class tst_Stream : public QObject
{
    Q_OBJECT

    static inline void VERIFY_PROGRESS_SIGNAL(
            const QSignalSpy &spy, int index,
            qint64 expectedBytesReceived, qint64 expectedBytesTotal)
    {
        QList<QVariant> arguments = spy.at(index);
        auto actualBytesReceived = arguments.at(0).toInt();
        auto actualBytesTotal = arguments.at(1).toInt();
        QCOMPARE(actualBytesReceived, expectedBytesReceived);
        QCOMPARE(actualBytesTotal, expectedBytesTotal);
    }

private slots:
    void relationalOperators();

    void readStandardOutput();
    void readStandardOutputWithEstimedSize();
    void readStandardOutputWithTwoStreams();
    void readStandardOutputHTTPError();

    void readStandardError();

    void parseDumpMap_null();
    void parseDumpMap_empty();
    void parseDumpMap_singleVideo();
    void parseDumpMap_misformedJson();
    void parseDumpMap_playlist();
    void parseDumpMap_playlistWithErrors();

    void parseDumpMap_overview();
    void parseDumpMap_formats();
    void parseDumpMap_subtitles();

    void parseFlatList_null();
    void parseFlatList_empty();
    void parseFlatList_singleVideo();
    void parseFlatList_playlist();

    void fileBaseName_data();
    void fileBaseName();

    void guestimateFullSize_data();
    void guestimateFullSize();

    void fileExtension_data();
    void fileExtension();

    void defaultFormats();
    void defaultFormats_2();

    void matchesHost_data();
    void matchesHost();
};

class FriendlyStream : public Stream
{
    friend class tst_Stream;
public:
    explicit FriendlyStream(QObject *parent) : Stream(parent) {}
};

/******************************************************************************
 ******************************************************************************/
void tst_Stream::relationalOperators()
{
    StreamObject::Config::Overview ov_1;
    ov_1.skipVideo = false;
    ov_1.markWatched = true;

    StreamObject::Config::Overview ov_2;
    ov_2.skipVideo = false;
    ov_2.markWatched = true;

    StreamObject::Config::Overview ov_3;
    ov_3.skipVideo = true;
    ov_3.markWatched = true;

    QVERIFY(ov_1 == ov_2); // Verify operator==()
    QVERIFY(ov_2 != ov_3); // Verify operator!=()
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::readStandardOutput()
{
    // Given
    QSharedPointer<FriendlyStream> target(new FriendlyStream(this));
    QSignalSpy spyProgress(target.data(), SIGNAL(downloadProgress(qint64, qint64)));

    // When
    target->parseStandardOutput(" .\\yt-dlp.exe https://www.youtube.com/watch?v=jDQv2jTNL04");
    target->parseStandardOutput("[youtube] jDQv2jTNL04: Downloading webpage");
    target->parseStandardOutput("[youtube] jDQv2jTNL04: Downloading video info webpage");
    target->parseStandardOutput("WARNING: Requested formats are incompatible for merge and will be merged into mkv.");
    target->parseStandardOutput("[download] Destination: C’EST QUOI CE TRUC.mp4");
    target->parseStandardOutput("[download]   0.0% of 167.85MiB at  4.13MiB/s ETA 00:00");
    target->parseStandardOutput("[download]   8.2% of 167.85MiB at  4.13MiB/s ETA 00:03");
    target->parseStandardOutput("[download]  25.6% of 167.85MiB at  4.13MiB/s ETA 00:13");
    target->parseStandardOutput("[download]  78.9% of 167.85MiB at  4.13MiB/s ETA 00:33");
    target->parseStandardOutput("[download]  98.2% of 167.85MiB at  4.13MiB/s ETA 00:39");
    target->parseStandardOutput("[download] 100% of 167.85MiB in 00:41");

    // Then
    // 167.85 MiB = 176003482 bytes
    QCOMPARE(spyProgress.count(), 7);
    VERIFY_PROGRESS_SIGNAL(spyProgress, 0,         0,         0); // -idle-
    VERIFY_PROGRESS_SIGNAL(spyProgress, 1,         0, 176003482); //   0.0%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 2,  14432286, 176003482); //   8.2%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 3,  45056892, 176003482); //  25.6%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 4, 138866748, 176003482); //  78.9%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 5, 172835420, 176003482); //  98.2%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 6, 176003482, 176003482); // 100.0%
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::readStandardOutputWithEstimedSize()
{
    // Given
    QSharedPointer<FriendlyStream> target(new FriendlyStream(this));
    QSignalSpy spyProgress(target.data(), SIGNAL(downloadProgress(qint64, qint64)));

    // When
    target->parseStandardOutput("[adobetv] ph5e313e3e632cb: Downloading pc webpage");
    target->parseStandardOutput("[adobetv] ph5e313e3e632cb: Downloading m3u8 information");
    target->parseStandardOutput("[adobetv] ph5e313e3e632cb: Downloading m3u8 information");
    target->parseStandardOutput("[adobetv] ph5e313e3e632cb: Downloading m3u8 information");
    target->parseStandardOutput("[hlsnative] Downloading m3u8 manifest");
    target->parseStandardOutput("[hlsnative] Total fragments: 227");
    target->parseStandardOutput("[download] Destination: video.mp4");
    target->parseStandardOutput("[download] 0.0% of ~55.43MiB at Unknown speed ETA 01:11:36");
    target->parseStandardOutput("[download] 0.0% of ~55.43MiB at Unknown speed ETA 23:52");
    target->parseStandardOutput("[download] 0.0% of ~55.43MiB at Unknown speed ETA 10:13");
    target->parseStandardOutput("[download] 0.0% of ~55.43MiB at 4.89MiB/s ETA 04:54");
    target->parseStandardOutput("[download] 0.1% of ~55.43MiB at 1.69MiB/s ETA 02:49");
    target->parseStandardOutput("[download] 3.4% of ~55.43MiB at 3.17MiB/s ETA 02:34");
    target->parseStandardOutput("[download] 4.0% of ~523.63MiB at 3.17MiB/s ETA 09:05");
    target->parseStandardOutput("[download] 4.0% of ~523.63MiB at 2.91MiB/s ETA 09:03");
    target->parseStandardOutput("[download] 4.0% of ~523.63MiB at 14.55MiB/s ETA 08:38");
    target->parseStandardOutput("[download] 5.9% of ~523.63MiB at 3.70MiB/s ETA 02:41");
    target->parseStandardOutput("[download] 5.9% of ~519.29MiB at 974.74KiB/s ETA 02:52");
    target->parseStandardOutput("[download] 6.9% of ~519.29MiB at 3.41MiB/s ETA 02:52");
    target->parseStandardOutput("[download] 8.4% of ~536.33MiB at 3.52MiB/s ETA 02:44");
    target->parseStandardOutput("[download] 10.6% of ~536.33MiB at 3.71MiB/s ETA 00:41");
    target->parseStandardOutput("[download] 10.6% of ~536.33MiB at 3.71MiB/s ETA 00:41");
    target->parseStandardOutput("[download] 10.6% of ~547.63MiB at 3.71MiB/s ETA 00:49 [download] 10.6% of ~547.63MiB at 3.71MiB/s ETA 02:49");
    target->parseStandardOutput("[download] 10.6% of ~547.63MiB at 3.71MiB/s ETA 00:49");
    target->parseStandardOutput("[download] 35.1% of ~546.67MiB at 3.84MiB/s ETA 00:46");
    target->parseStandardOutput("[download] 40.1% of ~546.67MiB at 1.47MiB/s ETA 00:46 [download] 40.1% of ~546.67MiB at 3.44MiB/s ETA 02:46");
    target->parseStandardOutput("[download] 40.1% of ~546.67MiB at 3.67MiB/s ETA 00:46");
    target->parseStandardOutput("[download] 40.1% of ~546.67MiB at 1.78MiB/s ETA 00:46");
    target->parseStandardOutput("[download] 73.1% of ~615.60MiB at 2.88MiB/s ETA 00:16 [download] 73.1% of ~615.60MiB at 6.71MiB/s ETA 00:16");
    target->parseStandardOutput("[download] 93.2% of ~615.60MiB at 3.11MiB/s ETA 00:16");
    target->parseStandardOutput("[download] 100.0% of ~615.60MiB at 3.89MiB/s ETA 00:01");

    // Then
    QCOMPARE(spyProgress.count(), 25);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  0,         0,         0); // -idle-
    VERIFY_PROGRESS_SIGNAL(spyProgress,  1,         0,  58122568); //   0.0%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  2,         0,  58122568); //   0.0%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  3,         0,  58122568); //   0.0%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  4,         0,  58122568); //   0.0%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  5,     58123,  58122568); //   0.1%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  6,   1976168,  58122568); //   3.4%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  7,  21962635, 549065851); //   4.0%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  8,  21962635, 549065851); //   4.0%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  9,  21962635, 549065851); //   4.0%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 10,  32394886, 549065851); //   5.9%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 11,  32126387, 544515032); //   5.9%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 12,  37571538, 544515032); //   6.9%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 13,  47240153, 562382767); //   8.4%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 14,  59612574, 562382767); //  10.6%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 15,  59612574, 562382767); //  10.6%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 16,  60868558, 574231675); //  10.6%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 17,  60868558, 574231675); //  10.6%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 18, 201201990, 573225042); //  35.1%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 19, 229863242, 573225042); //  40.1%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 20, 229863242, 573225042); //  40.1%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 21, 229863242, 573225042); //  40.1%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 22, 471862976, 645503386); //  73.1%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 23, 601609156, 645503386); //  93.2%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 24, 645503386, 645503386); // 100.0%
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::readStandardOutputWithTwoStreams()
{
    // Given
    QSharedPointer<FriendlyStream> target(new FriendlyStream(this));
    QSignalSpy spyProgress(target.data(), SIGNAL(downloadProgress(qint64, qint64)));

    target->setFileSizeInBytes(185178582); // Size is assumed known before download

    // When
    target->parseStandardOutput(" .\\yt-dlp.exe https://www.youtube.com/watch?v=jDQv2jTNL04");
    target->parseStandardOutput("[youtube] jDQv2jTNL04: Downloading webpage");
    target->parseStandardOutput("[youtube] jDQv2jTNL04: Downloading video info webpage");
    target->parseStandardOutput("WARNING: Requested formats are incompatible for merge and will be merged into mkv.");
    target->parseStandardOutput("[download] Destination: C’EST QUOI CE TRUC  (Je me balade sur Twitter)-jDQv2jTNL04.f299.mp4");
    target->parseStandardOutput("[download]   8.2% of 167.85MiB at  4.13MiB/s ETA 00:03");
    target->parseStandardOutput("[download]  25.6% of 167.85MiB at  4.13MiB/s ETA 00:13");
    target->parseStandardOutput("[download]  78.9% of 167.85MiB at  4.13MiB/s ETA 00:33");
    target->parseStandardOutput("[download]  98.2% of 167.85MiB at  4.13MiB/s ETA 00:09");
    target->parseStandardOutput("[download] 100% of 167.85MiB in 00:41");
    target->parseStandardOutput("[download] Destination: C’EST QUOI CE TRUC  (Je me balade sur Twitter)-jDQv2jTNL04.f251.webm");
    target->parseStandardOutput("[download]   3.6% of 8.75MiB at  3.33MiB/s ETA  in 00:43");
    target->parseStandardOutput("[download]  65.5% of 8.75MiB at  3.33MiB/s ETA  in 00:18");
    target->parseStandardOutput("[download] 100.0% of 8.75MiB at  3.33MiB/s ETA  in 00:01");
    target->parseStandardOutput("[download] 100% of 8.75MiB in 00:02");
    target->parseStandardOutput("[ffmpeg] Merging formats into \"C’EST QUOI CE TRUC  (Je me balade sur Twitter)-jDQv2jTNL04.mkv\"");
    target->parseStandardOutput("Deleting original file C’EST QUOI CE TRUC  (Je me balade sur Twitter)-jDQv2jTNL04.f299.mp4 (pass -k to keep)");
    target->parseStandardOutput("Deleting original file C’EST QUOI CE TRUC  (Je me balade sur Twitter)-jDQv2jTNL04.f251.webm (pass -k to keep)");

    // Then
    // Stream 1..:  176,003,482 bytes (167.85 MiB)
    // Stream 2..:    9,175,040 bytes (  8.75 MiB)
    // Total.....:  185,178,582 bytes (176.60 MiB)
    QCOMPARE(spyProgress.count(), 11);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  0,         0, 185178582); // -idle stream 1-
    VERIFY_PROGRESS_SIGNAL(spyProgress,  1,  14432286, 185178582); //   8.2%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  2,  45056892, 185178582); //  25.6%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  3, 138866748, 185178582); //  78.9%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  4, 172835420, 185178582); //  98.2%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  5, 176003482, 185178582); // 100.0%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  6, 176003482, 185178582); // -idle stream 2-
    VERIFY_PROGRESS_SIGNAL(spyProgress,  7, 176333784, 185178582); //   3.6%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  8, 182013134, 185178582); //  65.5%
    VERIFY_PROGRESS_SIGNAL(spyProgress,  9, 185178522, 185178582); // 100.0%
    VERIFY_PROGRESS_SIGNAL(spyProgress, 10, 185178522, 185178582); // 100.0%
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::readStandardOutputHTTPError()
{
    // Given
    QSharedPointer<FriendlyStream> target(new FriendlyStream(this));
    QSignalSpy spyProgress(target.data(), SIGNAL(downloadProgress(qint64, qint64)));

    // When
    target->parseStandardOutput(" .\\yt-dlp.exe https://www.youtube.com/watch?v=8_X5Iq9niDE");
    target->parseStandardOutput("[youtube] 8_X5Iq9niDE: Downloading webpage");
    target->parseStandardOutput("[youtube] 8_X5Iq9niDE: Downloading MPD manifest");
    target->parseStandardOutput("WARNING: Requested formats are incompatible for merge and will be merged into mkv.");
    target->parseStandardOutput("[dashsegments] Total fragments: 21");
    target->parseStandardOutput("[download] Destination: Installing CMake in 2 minutes on Windows-8_X5Iq9niDE.f248.webm");
    target->parseStandardOutput("[download]  61.9% of ~3.98MiB at  2.15MiB/s ETA 00:06     [download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 14 (attempt 1 of 10)...");
    target->parseStandardOutput("[download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 14 (attempt 2 of 10)...");
    target->parseStandardOutput("[download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 14 (attempt 3 of 10)...");
    target->parseStandardOutput("[download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 14 (attempt 4 of 10)...");
    target->parseStandardOutput("[download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 14 (attempt 5 of 10)...");
    target->parseStandardOutput("[download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 14 (attempt 6 of 10)...");
    target->parseStandardOutput("[download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 14 (attempt 7 of 10)...");
    target->parseStandardOutput("[download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 14 (attempt 8 of 10)...");
    target->parseStandardOutput("[download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 14 (attempt 9 of 10)...");
    target->parseStandardOutput("[download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 14 (attempt 10 of 10)...");
    target->parseStandardOutput("[download]  71.4% of ~4.37MiB at  3.14MiB/s ETA 00:05     [download] Got server HTTP error: HTTP Error 404: Not Found. Retrying fragment 16 (attempt 1 of 10)...");
    target->parseStandardOutput("[download] 100% of 4.42MiB in 00:17");
    target->parseStandardOutput("[dashsegments] Total fragments: 23");
    target->parseStandardOutput("[download] Destination: Installing CMake in 2 minutes on Windows-8_X5Iq9niDE.f140.m4a");
    target->parseStandardOutput("[download]  50.3% of 1.60MiB at  3.14MiB/s ETA 00:05");
    target->parseStandardOutput("[download] 100% of 1.60MiB in 00:01");
    target->parseStandardOutput("[ffmpeg] Merging formats into \"Installing CMake in 2 minutes on Windows-8_X5Iq9niDE.mkv\"");
    target->parseStandardOutput("Deleting original file Installing CMake in 2 minutes on Windows-8_X5Iq9niDE.f248.webm (pass -k to keep)");
    target->parseStandardOutput("Deleting original file Installing CMake in 2 minutes on Windows-8_X5Iq9niDE.f140.m4a (pass -k to keep)");

    // Then
    /*
     * Remark:
     * Total size will change all the time (even sometimes it will decrease)
     * because we don't presume it before downloading
     */
    QCOMPARE(spyProgress.count(), 16);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  0,       0,       0); // -idle stream 1-
    VERIFY_PROGRESS_SIGNAL(spyProgress,  1, 2583294, 4173333);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  2, 2583294, 4173333);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  3, 2583294, 4173333);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  4, 2583294, 4173333);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  5, 2583294, 4173333);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  6, 2583294, 4173333);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  7, 2583294, 4173333);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  8, 2583294, 4173333);
    VERIFY_PROGRESS_SIGNAL(spyProgress,  9, 2583294, 4173333);
    VERIFY_PROGRESS_SIGNAL(spyProgress, 10, 2583294, 4173333);
    VERIFY_PROGRESS_SIGNAL(spyProgress, 11, 3271747, 4582278);
    VERIFY_PROGRESS_SIGNAL(spyProgress, 12, 4634706, 4634706);
    VERIFY_PROGRESS_SIGNAL(spyProgress, 13, 4634706, 4634706);
    VERIFY_PROGRESS_SIGNAL(spyProgress, 14, 5478601, 1677722); // -idle stream 2-
    VERIFY_PROGRESS_SIGNAL(spyProgress, 15, 6312428, 1677722);
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::readStandardError()
{
    /// \todo
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::parseDumpMap_null()
{
    QByteArray stdoutBytes;
    QByteArray stderrBytes;
    auto actualMap = StreamAssetDownloader::parseDumpMap(stdoutBytes, stderrBytes);
    QVERIFY(actualMap.isEmpty());
}

void tst_Stream::parseDumpMap_empty()
{
    QByteArray stdoutBytes("\n\n");
    QByteArray stderrBytes;
    auto actualMap = StreamAssetDownloader::parseDumpMap(stdoutBytes, stderrBytes);
    QVERIFY(actualMap.isEmpty());
}

void tst_Stream::parseDumpMap_singleVideo()
{
    QByteArray stdoutBytes = DummyStreamFactory::dumpSingleVideo();
    QByteArray stderrBytes;
    auto actualMap = StreamAssetDownloader::parseDumpMap(stdoutBytes, stderrBytes);
    auto actual = actualMap.value("YsYYO_fKxE0");
    QCOMPARE(actual.data().fulltitle, QLatin1String("Fun Test: Which is real?"));
    QCOMPARE(actual.error(), StreamObject::NoError);
}

void tst_Stream::parseDumpMap_misformedJson()
{
    QByteArray stdoutBytes("{ name:'hello', data:[ type:'mp3'  }\n");
    //                                      unclosed list [] ^
    QByteArray stderrBytes;
    auto actualMap = StreamAssetDownloader::parseDumpMap(stdoutBytes, stderrBytes);
    auto actual = actualMap.first();
    QCOMPARE(actual.error(), StreamObject::ErrorJsonFormat);
}

void tst_Stream::parseDumpMap_playlist()
{
    QByteArray stdoutBytes = DummyStreamFactory::dumpPlaylist();
    QByteArray stderrBytes;
    auto actualMap = StreamAssetDownloader::parseDumpMap(stdoutBytes, stderrBytes);
    auto actual_0 = actualMap.value("YsYYO_fKxE0");
    auto actual_1 = actualMap.value("lD_qyjcMEEJ");
    auto actual_2 = actualMap.value("sfePkSig_DD");
    QCOMPARE(actual_0.data().fulltitle, QLatin1String("Fun Test: Which is real?"));
    QCOMPARE(actual_1.data().fulltitle, QLatin1String("Fun Test: Which is real?"));
    QCOMPARE(actual_2.data().fulltitle, QLatin1String("Fun Test: Which is real?"));
    QCOMPARE(actual_0.error(), StreamObject::NoError);
    QCOMPARE(actual_1.error(), StreamObject::NoError);
    QCOMPARE(actual_2.error(), StreamObject::NoError);
}

void tst_Stream::parseDumpMap_playlistWithErrors()
{
    QByteArray stdoutBytes = DummyStreamFactory::dumpPlaylist();
    QByteArray stderrBytes = DummyStreamFactory::dumpPlaylistStandardError();
    auto actualMap = StreamAssetDownloader::parseDumpMap(stdoutBytes, stderrBytes);
    auto actual_0 = actualMap.value("YsYYO_fKxE0");
    auto actual_1 = actualMap.value("lD_qyjcMEEJ");
    auto actual_2 = actualMap.value("sfePkSig_DD");
    auto actual_3 = actualMap.value("LdRxXID_b28");
    auto actual_4 = actualMap.value("TB_QmSWVY7o");
    QCOMPARE(actual_0.data().fulltitle, QLatin1String("Fun Test: Which is real?"));
    QCOMPARE(actual_1.data().fulltitle, QLatin1String("Fun Test: Which is real?"));
    QCOMPARE(actual_2.data().fulltitle, QLatin1String("Fun Test: Which is real?"));
    QCOMPARE(actual_3.data().fulltitle, QLatin1String(""));
    QCOMPARE(actual_4.data().fulltitle, QLatin1String(""));
    QCOMPARE(actual_0.error(), StreamObject::NoError);
    QCOMPARE(actual_1.error(), StreamObject::NoError);
    QCOMPARE(actual_2.error(), StreamObject::NoError);
    QCOMPARE(actual_3.error(), StreamObject::ErrorUnavailable);
    QCOMPARE(actual_4.error(), StreamObject::ErrorUnavailable);
}

void tst_Stream::parseDumpMap_overview()
{
    QByteArray stdoutBytes = QString(
    "{"
    " 'id': '0123ABcD-98',                                                   "
    " 'title': 'Test title - test',                                          "
    " 'formats': [],                                                         "
    " 'thumbnails': [],                                                      "
    " 'thumbnail': 'https://test.test.com/maxresdefault.webp',               "
    " 'description': 'test descripted in 2006',                              "
    " 'upload_date': '20121106',                                             "
    " 'uploader': 'an_user',                                                 "
    " 'uploader_id': 'AnUser',                                               "
    " 'uploader_url': 'http://www.test.com/user/Test',                       "
    " 'channel_id': 'ABCDEFG_123456789abcdEFG',                              "
    " 'channel_url': 'https://www.test.com/channel/ABCDEFG_123456789abcdEFG',"
    " 'duration': 116,                                                       "
    " 'view_count': 821519,                                                  "
    " 'average_rating': 4.9429417,                                           "
    " 'age_limit': 0,                                                        "
    " 'webpage_url': 'https://www.test.com/watch?v=0123ABcD-98',             "
    " 'categories': ['Comedy'],                                              "
    " 'tags': ['test', 'and', 'other', 'tests'],                             "
    " 'playable_in_embed': true,                                             "
    " 'is_live': false,                                                      "
    " 'was_live': false,                                                     "
    " 'live_status': 'not_live',                                             "
    " 'release_timestamp': null,                                             "
    " 'automatic_captions': {},                                              "
    " 'subtitles': {},                                                       "
    " 'chapters': null,                                                      "
    " 'like_count': 13337,                                                   "
    " 'dislike_count': 193,                                                  "
    " 'channel': 'mozinor',                                                  "
    " 'availability': 'public',                                              "
    " 'original_url': 'https://www.test.com/watch?v=0123ABcD-98',            "
    " 'webpage_url_basename': 'watch',                                       "
    " 'extractor': 'Test',                                                   "
    " 'extractor_key': 'test',                                               "
    " 'playlist': null,                                                      "
    " 'playlist_index': null,                                                "
    " 'display_id': '0123ABcD-98',                                           "
    " 'duration_string': '1:56',                                             "
    " 'requested_subtitles': null,                                           "
    " '__has_drm': false,                                                    "
    " 'requested_formats': [],                                               "
    " 'format': '244 - 636x480 (480p)+251 - audio only (medium)',            "
    " 'format_id': '244+251',                                                "
    " 'ext': 'webm',                                                         "
    " 'protocol': 'https+https',                                             "
    " 'language': '',                                                        "
    " 'format_note': '480p+medium',                                          "
    " 'filesize_approx': 6480861,                                            "
    " 'tbr': 446.31600000000003,                                             "
    " 'width': 636,                                                          "
    " 'height': 480,                                                         "
    " 'resolution': '636x480',                                               "
    " 'fps': 25,                                                             "
    " 'dynamic_range': 'SDR',                                                "
    " 'vcodec': 'vp9',                                                       "
    " 'vbr': 315.547,                                                        "
    " 'stretched_ratio': null,                                               "
    " 'acodec': 'opus',                                                      "
    " 'abr': 130.769,                                                        "
    " 'asr': 48000,                                                          "
    " 'fulltitle': 'Test title - test',                                      "
    " 'epoch': 1636542358,                                                   "
    " '_filename': 'Test title - test [0123ABcD-98].webm',                   "
    " 'filename': 'Test title - test [0123ABcD-98].webm',                    "
    " 'urls': 'https://test.test.com/videoplayback?v=I075mA2ntw%3D%3D'       "
    "}"
    ).replace('\'', '"').simplified().toLatin1();
    QByteArray stderrBytes;
    auto actualMap = StreamAssetDownloader::parseDumpMap(stdoutBytes, stderrBytes);
    auto actual = actualMap.value("0123ABcD-98");
    QCOMPARE(actual.data().id, QLatin1String("0123ABcD-98"));
    QCOMPARE(actual.data().defaultTitle, QLatin1String("Test title - test"));
    QCOMPARE(actual.data().thumbnail, QLatin1String("https://test.test.com/maxresdefault.webp"));
    QCOMPARE(actual.data().description, QLatin1String("test descripted in 2006"));
    QCOMPARE(actual.data().webpage_url, QLatin1String("https://www.test.com/watch?v=0123ABcD-98"));
    QCOMPARE(actual.data().extractor, QLatin1String("Test"));
    QCOMPARE(actual.data().extractor_key, QLatin1String("test"));
    QCOMPARE(actual.data().playlist, QLatin1String(""));
    QCOMPARE(actual.data().playlist_index, QLatin1String(""));
    QCOMPARE(actual.data().fulltitle, QLatin1String("Test title - test"));
    QCOMPARE(actual.data().originalFilename, QLatin1String("Test title - test [0123ABcD-98].webm"));

    QCOMPARE(actual.error(), StreamObject::NoError);
}

void tst_Stream::parseDumpMap_formats()
{
    QByteArray stdoutBytes = QString(
    "{"
    " 'id': '0123ABcD-98',                                                   "
    " 'formats': [{                                                          "
    "  'asr': 22050,                                                         "
    "  'filesize': 692219,                                                   "
    "  'format_id': '139',                                                   "
    "  'format_note': 'low',                                                 "
    "  'source_preference': -1,                                              "
    "  'fps': null,                                                          "
    "  'height': null,                                                       "
    "  'quality': 2,                                                         "
    "  'tbr': 47.622,                                                        "
    "  'url': 'https://test.test.com/videoplayback?v=ntw%3D%3D',             "
    "  'width': null,                                                        "
    "  'language': '',                                                       "
    "  'language_preference': -1,                                            "
    "  'ext': 'm4a',                                                         "
    "  'vcodec': 'none',                                                     "
    "  'acodec': 'mp4a.40.5',                                                "
    "  'dynamic_range': null,                                                "
    "  'abr': 47.622,                                                        "
    "  'downloader_options': {                                               "
    "   'http_chunk_size': 10485760                                          "
    "  },                                                                    "
    "  'container': 'm4a_dash',                                              "
    "  'protocol': 'https',                                                  "
    "  'audio_ext': 'm4a',                                                   "
    "  'video_ext': 'none',                                                  "
    "  'format': '139 - audio only (low)',                                   "
    "  'resolution': 'audio only',                                           "
    "  'http_headers': {                                                     "
    "   'User-Agent': 'Mozilla/5.0 AppleWebKit/537.36 Chrome/70.0.3538.44 Safari/537.36',"
    "   'Accept-Charset': 'ISO-8859-1,utf-8;q=0.7,*;q=0.7',                  "
    "   'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',"
    "   'Accept-Encoding': 'gzip, deflate',                                  "
    "   'Accept-Language': 'en-us,en;q=0.5'                                  "
    "  }                                                                     "
    " }]                                                                     "
    "}"
    ).replace('\'', '"').simplified().toLatin1();
    QByteArray stderrBytes;
    auto actualMap = StreamAssetDownloader::parseDumpMap(stdoutBytes, stderrBytes);
    auto actual = actualMap.value("0123ABcD-98");
    QCOMPARE(actual.data().id, QLatin1String("0123ABcD-98"));
    auto formats = actual.data().formats;
    auto format = formats.first();

    QCOMPARE(format.asr, 22050);
    QCOMPARE(format.filesize, 692219);
    QCOMPARE(format.formatId, StreamFormatId("139"));
    QCOMPARE(format.formatNote, QLatin1String("low"));
    QCOMPARE(format.fps, 0);
    QCOMPARE(format.height, 0);
    QCOMPARE(format.tbr, 47.622);
    QCOMPARE(format.url, QLatin1String("https://test.test.com/videoplayback?v=ntw%3D%3D"));
    QCOMPARE(format.width, 0);
    QCOMPARE(format.ext, QLatin1String("m4a"));
    QCOMPARE(format.vcodec, QLatin1String("none"));
    QCOMPARE(format.acodec, QLatin1String("mp4a.40.5"));
    QCOMPARE(format.dynamicRange, QLatin1String(""));
    QCOMPARE(format.abr, 47.622);
    QCOMPARE(format.format, QLatin1String("139 - audio only (low)"));
    QCOMPARE(format.resolution, QLatin1String("audio only"));

    QCOMPARE(actual.error(), StreamObject::NoError);
}

void tst_Stream::parseDumpMap_subtitles()
{
    QByteArray stdoutBytes = QString(
    "{"
    " 'id': '0123ABcD-98',                                              "
    " 'subtitles': {                                                    "
    "  'af': [                                                          "
    "    {                                                              "
    "     'ext': 'json3',                                               "
    "     'name': 'Afrikaans',                                          "
    "     'url': 'https://www.test.com/api/AS5-54XmiWS',                "
    "     'data': null                                                  "
    "    },{                                                            "
    "     'ext': 'vtt',                                                 "
    "     'name': 'Afrikaans',                                          "
    "     'url': 'https://www.test.com/api/AS5-54X1298',                "
    "     'data': null                                                  "
    "    }                                                              "
    "   ],                                                              "
    "  'lv': [                                                          "
    "    {                                                              "
    "     'ext': 'json3',                                               "
    "     'name': 'Latvian',                                            "
    "     'url': 'https://www.test.com/api/4561254miWS',                "
    "     'data': null                                                  "
    "    },{                                                            "
    "     'ext': 'vtt',                                                 "
    "     'name': 'Latvian',                                            "
    "     'url': 'https://www.test.com/api/Ad5f4re1298',                "
    "     'data': null                                                  "
    "    }                                                              "
    "   ]                                                               "
    "  },                                                               "
    " 'automatic_captions': {                                           "
    "  'fr-FR': [                                                       "
    "    {                                                              "
    "     'ext': 'srt',                                                 "
    "     'name': 'French (France)',                                    "
    "     'url': 'https://www.test.com/api/DEddsdfDE',                  "
    "     'data': null                                                  "
    "    }                                                              "
    "   ]                                                               "
    "  }                                                                "
    "}"
    ).replace('\'', '"').simplified().toLatin1();
    QByteArray stderrBytes;
    auto actualMap = StreamAssetDownloader::parseDumpMap(stdoutBytes, stderrBytes);
    auto actual = actualMap.value("0123ABcD-98");
    QCOMPARE(actual.data().id, QLatin1String("0123ABcD-98"));

    // Direct Getters
    auto subtitles = actual.data().subtitles;

    QCOMPARE(subtitles.count(), 5);

    QCOMPARE(subtitles.at(0).languageCode, QLatin1String("af"));
    QCOMPARE(subtitles.at(0).ext, QLatin1String("json3"));
    QCOMPARE(subtitles.at(0).languageName, QLatin1String("Afrikaans"));
    QCOMPARE(subtitles.at(0).url, QLatin1String("https://www.test.com/api/AS5-54XmiWS"));
    QCOMPARE(subtitles.at(0).data, QLatin1String(""));
    QVERIFY(subtitles.at(0).isAutomatic == false);

    QCOMPARE(subtitles.at(1).languageCode, QLatin1String("af"));
    QCOMPARE(subtitles.at(1).ext, QLatin1String("vtt"));
    QCOMPARE(subtitles.at(1).languageName, QLatin1String("Afrikaans"));
    QCOMPARE(subtitles.at(1).url, QLatin1String("https://www.test.com/api/AS5-54X1298"));
    QCOMPARE(subtitles.at(1).data, QLatin1String(""));
    QVERIFY(subtitles.at(1).isAutomatic == false);

    QCOMPARE(subtitles.at(2).languageCode, QLatin1String("lv"));
    QCOMPARE(subtitles.at(2).ext, QLatin1String("json3"));
    QCOMPARE(subtitles.at(2).languageName, QLatin1String("Latvian"));
    QCOMPARE(subtitles.at(2).url, QLatin1String("https://www.test.com/api/4561254miWS"));
    QCOMPARE(subtitles.at(2).data, QLatin1String(""));
    QVERIFY(subtitles.at(2).isAutomatic == false);

    QCOMPARE(subtitles.at(3).languageCode, QLatin1String("lv"));
    QCOMPARE(subtitles.at(3).ext, QLatin1String("vtt"));
    QCOMPARE(subtitles.at(3).languageName, QLatin1String("Latvian"));
    QCOMPARE(subtitles.at(3).url, QLatin1String("https://www.test.com/api/Ad5f4re1298"));
    QCOMPARE(subtitles.at(3).data, QLatin1String(""));
    QVERIFY(subtitles.at(3).isAutomatic == false);

    QCOMPARE(subtitles.at(4).languageCode, QLatin1String("fr-FR"));
    QCOMPARE(subtitles.at(4).ext, QLatin1String("srt"));
    QCOMPARE(subtitles.at(4).languageName, QLatin1String("French (France)"));
    QCOMPARE(subtitles.at(4).url, QLatin1String("https://www.test.com/api/DEddsdfDE"));
    QCOMPARE(subtitles.at(4).data, QLatin1String(""));
    QVERIFY(subtitles.at(4).isAutomatic == true);

    // Indirect Getters
    auto languages = actual.data().subtitleLanguages();
    auto extensions = actual.data().subtitleExtensions();

    QCOMPARE(languages.count(), 3);
    QCOMPARE(languages.at(0).languageCode, QLatin1String("af"));
    QCOMPARE(languages.at(0).languageName, QLatin1String("Afrikaans"));
    QVERIFY(languages.at(0).isAutomatic == false);

    QCOMPARE(languages.at(1).languageCode, QLatin1String("fr-FR"));
    QCOMPARE(languages.at(1).languageName, QLatin1String("French (France)"));
    QVERIFY(languages.at(1).isAutomatic == true);

    QCOMPARE(languages.at(2).languageCode, QLatin1String("lv"));
    QCOMPARE(languages.at(2).languageName, QLatin1String("Latvian"));
    QVERIFY(languages.at(2).isAutomatic == false);

    QCOMPARE(extensions.count(), 3);
    extensions.sort();
    QCOMPARE(extensions.at(0), QLatin1String("json3"));
    QCOMPARE(extensions.at(1), QLatin1String("srt"));
    QCOMPARE(extensions.at(2), QLatin1String("vtt"));

    QCOMPARE(actual.error(), StreamObject::NoError);
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::parseFlatList_null()
{
    QByteArray stdoutBytes;
    QByteArray stderrBytes;
    auto actualList = StreamAssetDownloader::parseFlatList(stdoutBytes, stderrBytes);
    QVERIFY(actualList.isEmpty());
}

void tst_Stream::parseFlatList_empty()
{
    QByteArray stdoutBytes("\n\n");
    QByteArray stderrBytes;
    auto actualList = StreamAssetDownloader::parseFlatList(stdoutBytes, stderrBytes);
    QVERIFY(actualList.isEmpty());
}

void tst_Stream::parseFlatList_singleVideo()
{
    QByteArray stdoutBytes = DummyStreamFactory::flatSingleVideo();
    QByteArray stderrBytes;
    auto actualList = StreamAssetDownloader::parseFlatList(stdoutBytes, stderrBytes);
    QCOMPARE(actualList.count(), 1);
    QCOMPARE(actualList.at(0).id, QLatin1String("etAIpkdhU9Q"));
}

void tst_Stream::parseFlatList_playlist()
{
    QByteArray stdoutBytes = DummyStreamFactory::flatPlaylist();
    QByteArray stderrBytes;
    auto actualList = StreamAssetDownloader::parseFlatList(stdoutBytes, stderrBytes);
    QCOMPARE(actualList.count(), 3);
    QCOMPARE(actualList.at(0).id, QLatin1String("etAIpkdhU9Q"));
    QCOMPARE(actualList.at(1).id, QLatin1String("v2AC41dglnM"));
    QCOMPARE(actualList.at(2).id, QLatin1String("LdRxXID_b28"));
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::fileBaseName_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("null") << QString() << QString();
    QTest::newRow("empty") << "" << QString();

    QTest::newRow("separator") << "Live 10/02/2018" << "Live 10-02-2018";
    QTest::newRow("separator") << "here\\we\\come" << "here-we-come";

    QTest::newRow("tab") << "\t here\twe\tcome \t" << "here we come";

    QTest::newRow("minus") << "- \x2D — \u2212 \u2014" << "- - - - -";

    QTest::newRow("brackets") << "Windsor ['98]" << "Windsor ['98]";
    QTest::newRow("brackets") << "Windsor ('98)" << "Windsor ('98)";
    QTest::newRow("@")        << "Live @ Windsor" << "Live @ Windsor";
    QTest::newRow("extra _") << "_*_Cambridge_*_" << "-Cambridge-";

    QTest::newRow("double quote") << "\"Hello\" by Adele" << "'Hello' by Adele";
    QTest::newRow("simple quote") << "Live in Paris '79" << "Live in Paris '79";

    QTest::newRow("")
            << "\"Bohemian Rhapsody\" Steve Vai & Malmsteen & Zakk Wylde & Nuno@Atlantic City (11/30/18)"
            << "'Bohemian Rhapsody' Steve Vai & Malmsteen & Zakk Wylde & Nuno@Atlantic City (11-30-18)" ;

    QTest::newRow("stupid text") << "Live '01 (Official Video)" << "Live '01";
    QTest::newRow("stupid text") << "(Official Video) Live '01" << "Live '01";
    QTest::newRow("stupid text") << "Live (Official Video) '01" << "Live '01";
}

void tst_Stream::fileBaseName()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    StreamObject target;
    auto data = target.data();
    data.defaultTitle = input;
    data.fulltitle = input;
    target.setData(data);
    auto actual = target.fileBaseName();

    QCOMPARE(actual, expected);
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::guestimateFullSize_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<BigInteger>("expected");

    QTest::newRow("null") << QString() << BigInteger(-1);
    QTest::newRow("empty") << "" << BigInteger(-1);
    QTest::newRow("default") << "244+140" << BigInteger(294311 + 280597);

    QTest::newRow("audio") << "18" << BigInteger(1552999);
    QTest::newRow("audio") << "43" << BigInteger(2875968);
    QTest::newRow("audio") << "140" << BigInteger(280597);

    QTest::newRow("audio+video") << "160+140" << BigInteger(63901 + 280597);
    QTest::newRow("audio+video") << "278+140" << BigInteger(53464 + 280597);

    /*
     * Invalid formats
     * In yt-dlp, video ID must be the first ID.
     * But we accept them as valid here.
     */
    QTest::newRow("audio+video") << "140+160" << BigInteger(280597 + 63901);
    QTest::newRow("audio+video") << "140+278" << BigInteger(280597 + 53464);
}

void tst_Stream::guestimateFullSize()
{
    QFETCH(QString, input);
    QFETCH(BigInteger, expected);

    auto target = DummyStreamFactory::createDummyStreamObject_Youtube();
    qint64 actual = target.guestimateFullSize(input);

    QCOMPARE(actual, expected.value);
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::fileExtension_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("null") << QString() << "webm";
    QTest::newRow("empty") << "" << "webm";
    QTest::newRow("default") << "244+140" << "webm";

    QTest::newRow("audio") << "18" << "mp4";
    QTest::newRow("audio") << "43" << "webm";
    QTest::newRow("audio") << "140" << "m4a";

    QTest::newRow("audio+video") << "160+140" << "mp4";
    QTest::newRow("audio+video") << "278+140" << "webm";

    /*
     * Invalid formats
     * In yt-dlp, video ID must be the first ID.
     * But we accept them as valid here.
     */
    QTest::newRow("audio+video") << "140+160" << "mp4";
    QTest::newRow("audio+video") << "140+278" << "webm";
}

void tst_Stream::fileExtension()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    auto target = DummyStreamFactory::createDummyStreamObject_Youtube();
    auto actual = target.suffix(input);

    QCOMPARE(actual, expected);
}

/******************************************************************************
 ******************************************************************************/
void tst_Stream::defaultFormats()
{
    auto target = DummyStreamFactory::createDummyStreamObject_Dailymotion();
    auto actual = target.data().defaultFormats();

    QList<StreamFormat> expected;
    expected << StreamFormat("http-144-1" , "mp4", "", 0, "mp4a.40.5", 0, 0, "avc1.42000b",  192,  112, 0, 0);
    expected << StreamFormat("http-240-1" , "mp4", "", 0, "mp4a.40.5", 0, 0, "avc1.4d000d",  320,  184, 0, 0);
    expected << StreamFormat("http-380-1" , "mp4", "", 0, "mp4a.40.5", 0, 0, "avc1.4d0016",  512,  288, 0, 0);
    expected << StreamFormat("http-480-1" , "mp4", "", 0, "mp4a.40.2", 0, 0, "avc1.64001f",  848,  480, 0, 0);
    expected << StreamFormat("http-720-1" , "mp4", "", 0, "mp4a.40.2", 0, 0, "avc1.64001f", 1280,  720, 0, 0);
    expected << StreamFormat("http-1080-1", "mp4", "", 0, "mp4a.40.2", 0, 0, "avc1.640028", 1920, 1080, 0, 0);

    QCOMPARE(actual.count(), 6);
    for (int index = 0; index < 6; ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

void tst_Stream::defaultFormats_2()
{
    auto target = DummyStreamFactory::createDummyStreamObject_Other();
    auto actual = target.data().defaultFormats();

    QList<StreamFormat> expected;
    expected << StreamFormat("240p"     , "mp4", "", 0, ""         , 0, 0, ""           ,   0, 240,  0, 400);
    expected << StreamFormat("480p"     , "mp4", "", 0, ""         , 0, 0, ""           ,   0, 480,  0, 600);
    expected << StreamFormat("hls-287-1", "mp4", "", 0, "mp4a.40.2", 0, 0, "avc1.640015", 430, 240, 24,   0);
    expected << StreamFormat("hls-703"  , "mp4", "", 0, "mp4a.40.2", 0, 0, "avc1.64001e", 860, 480, 24,   0);

    QCOMPARE(actual.count(), 4);
    for (int index = 0; index < 4; ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}


/******************************************************************************
 ******************************************************************************/
void tst_Stream::matchesHost_data()
{
    QTest::addColumn<QString>("inputHost");
    QTest::addColumn<QStringList>("regexHosts");
    QTest::addColumn<bool>("expected");

    QTest::newRow("null") << QString() << QStringList() << false;
    QTest::newRow("empty") << "" << QStringList() << false;

    QTest::newRow("simple") << "www.absnews.com" << QStringList( {"absnews:videos"} ) << false;
    QTest::newRow("simple") << "www.absnews.com" << QStringList( {"absnews.com"} ) << true;
    QTest::newRow("simple") << "videos.absnews.com" << QStringList( {"absnews:videos"} ) << true;
    QTest::newRow("simple") << "videos.absnews.com" << QStringList( {"absnews.com:videos"} ) << true;
    QTest::newRow("simple") << "videos.www.absnews.com" << QStringList( {"absnews:videos"} ) << true;
    QTest::newRow("simple") << "player.videos.absnews.com" << QStringList( {"absnews:videos:player"} ) << true;

    QTest::newRow("simple list") << "www.youtube.com" << QStringList( {"youtube", "youtube.com"} ) << true;
    QTest::newRow("case sensitive") << "www.bild.de" << QStringList( {"Bild"} ) << true;

    QTest::newRow("contains") << "www.absnews.com" << QStringList( {"abs"} ) << false;
    QTest::newRow("contains") << "www.absnews.com" << QStringList( {"news"} ) << false;
    QTest::newRow("contains") << "www.absnews.com" << QStringList( {"news.com"} ) << false;

    QTest::newRow("no match") << "www.aol-videos.com"  << QStringList( {"aol.com"} ) << false;
    QTest::newRow("no match") << "www.aol-videos.com" << QStringList( {"aol"} ) << false;
    QTest::newRow("no match") << "www.bildung.de" << QStringList( {"Bild"} ) << false;
    QTest::newRow("no match list") << "www.youtube.de" << QStringList( {"youtu.be", "youtube.com", "youtube:video"} ) << false;

    // colon symbol ':' -> With 'abcnews:video', the hostname must contains 'abcnews' and also 'video'
    QTest::newRow("colon simple") << "video.abcnews.de" << QStringList( {"abcnews:video"} ) << true;
    QTest::newRow("colon valid") << "video.news.abcnews.de"<< QStringList( {"abcnews:video"} ) << true;
    QTest::newRow("colon invalid") << "www.abcnews.de" << QStringList( {"abcnews:video"} ) << false;

}

void tst_Stream::matchesHost()
{
    QFETCH(QString, inputHost);
    QFETCH(QStringList, regexHosts);
    QFETCH(bool, expected);

    bool actual = Stream::matchesHost(inputHost, regexHosts);
    QCOMPARE(actual, expected);
}

/******************************************************************************
 ******************************************************************************/
QTEST_APPLESS_MAIN(tst_Stream)

#include "tst_stream.moc"
