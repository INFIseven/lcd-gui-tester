#include "librarychecker.h"
#include "embeddedpython.h"
#include <QApplication>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryFile>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QSysInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

LibraryChecker::LibraryChecker(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_networkManager(nullptr)
    , m_currentReply(nullptr)
    , m_progressDialog(nullptr)
    , m_downloadSuccess(false)
    , m_embeddedPython(nullptr)
{
    m_networkManager = new QNetworkAccessManager(this);
    m_embeddedPython = new EmbeddedPython(parent);
}

LibraryChecker::~LibraryChecker()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }

    if (m_progressDialog) {
        m_progressDialog->deleteLater();
    }

    if (m_embeddedPython) {
        m_embeddedPython->deleteLater();
    }
}

bool LibraryChecker::checkAndDownloadLibraries()
{
    // Check which libraries are missing
    struct LibraryStatus {
        QString name;
        bool present;
        bool downloaded;
    };

    QStringList missingPythonPackages;
    bool pythonPackagesPresent = isPythonPackagesPresent(missingPythonPackages);

    QVector<LibraryStatus> libraries = {
        {"LVGL library (~15MB)", isLvglPresent(), false},
        {"nRF52 SDK (~150MB)", isNrf52SdkPresent(), false},
        {"ARM GNU Toolchain (~100MB)", isArmGnuToolchainPresent(), false},
        {"nRF52 LCD Tester Firmware (~1MB)", isNrf52FirmwarePresent(), false},
        {"CMake (~40MB)", isCMakePresent(), false},
        {"Embedded Python (~25MB)", isPythonPresent(), false}
    };

    // Add Python packages entry only if there are missing packages
    if (!pythonPackagesPresent) {
        libraries.append({QString("Python packages: %1").arg(missingPythonPackages.join(", ")), false, false});
    }

    // Build list of missing libraries
    QStringList missingLibraries;
    for (const auto& lib : libraries) {
        if (!lib.present) {
            missingLibraries.append("• " + lib.name);
        }
    }

    // If all libraries are present, return success
    if (missingLibraries.isEmpty()) {
        return true;
    }

    // Automatically download all missing libraries without confirmation
    if (!libraries[0].present) {
        downloadLvgl();
        libraries[0].downloaded = m_downloadSuccess;
    }

    if (!libraries[1].present) {
        downloadNrf52Sdk();
        libraries[1].downloaded = m_downloadSuccess;
    }

    if (!libraries[2].present) {
        downloadArmGnuToolchain();
        libraries[2].downloaded = m_downloadSuccess;
    }

    if (!libraries[3].present) {
        downloadNrf52Firmware();
        libraries[3].downloaded = m_downloadSuccess;
    }

    if (!libraries[4].present) {
        downloadCMake();
        libraries[4].downloaded = m_downloadSuccess;
    }

    if (!libraries[5].present) {
        libraries[5].downloaded = downloadPython();
    }

    // Download Python packages if they were added to the list
    if (libraries.size() > 6 && !libraries[6].present) {
        libraries[6].downloaded = downloadPythonPackages(missingPythonPackages);
    }

    // Build final status message
    QStringList completedLibraries;
    QStringList failedLibraries;

    for (int i = 0; i < libraries.size(); ++i) {
        if (!libraries[i].present) {
            if (libraries[i].downloaded) {
                completedLibraries.append("✓ " + libraries[i].name);
            } else {
                failedLibraries.append("✗ " + libraries[i].name);
            }
        }
    }

    // Show final status dialog
    QString statusMessage;
    if (failedLibraries.isEmpty()) {
        statusMessage = "All components have been successfully downloaded and installed:\n\n";
        statusMessage += completedLibraries.join("\n");
        statusMessage += "\n\nThe application is ready for use.";

        QMessageBox::information(
            m_parent,
            "Download Complete",
            statusMessage
        );
        return true;
    } else {
        statusMessage = "Download Status:\n\n";
        if (!completedLibraries.isEmpty()) {
            statusMessage += "Successfully installed:\n";
            statusMessage += completedLibraries.join("\n");
            statusMessage += "\n\n";
        }
        statusMessage += "Failed to install:\n";
        statusMessage += failedLibraries.join("\n");
        statusMessage += "\n\nPlease check your internet connection or install the failed components manually.";

        QMessageBox::warning(
            m_parent,
            "Download Incomplete",
            statusMessage
        );
        return false;
    }
}

bool LibraryChecker::isLvglPresent()
{
    QString librariesPath = getLibrariesPath();
    QDir lvglDir(librariesPath + "/" + LVGL_FOLDER);
    
    // Check if lvgl directory exists and contains key files
    if (!lvglDir.exists()) {
        return false;
    }
    
    // Check for key LVGL files to ensure it's a complete installation
    QStringList keyFiles = {
        "lvgl.h",
        "src/core/lv_obj.h",
        "src/core/lv_obj.c",
        "lv_conf_template.h"
    };
    
    for (const QString& file : keyFiles) {
        if (!QFile::exists(lvglDir.absoluteFilePath(file))) {
            qDebug() << "Missing LVGL file:" << file;
            return false;
        }
    }
    
    return true;
}

bool LibraryChecker::isNrf52SdkPresent()
{
    QString librariesPath = getLibrariesPath();
    QDir nrfDir(librariesPath + "/" + NRF52_SDK_FOLDER);
    
    // Check if nRF52 SDK directory exists and contains key files
    if (!nrfDir.exists()) {
        return false;
    }
    
    // Check for key nRF52 SDK files to ensure it's a complete installation
    QStringList keyFiles = {
        "components/softdevice/s132/headers/nrf_sdm.h",
        "components/libraries/util/nordic_common.h",
        "modules/nrfx/nrfx.h",
        "components/boards/boards.h"
    };
    
    for (const QString& file : keyFiles) {
        if (!QFile::exists(nrfDir.absoluteFilePath(file))) {
            qDebug() << "Missing nRF52 SDK file:" << file;
            return false;
        }
    }
    
    return true;
}

bool LibraryChecker::isArmGnuToolchainPresent()
{
    QString librariesPath = getLibrariesPath();
    QDir toolchainDir(librariesPath + "/" + ARM_GNU_TOOLCHAIN_FOLDER);

    // Check if ARM GNU Toolchain directory exists and contains key files
    if (!toolchainDir.exists()) {
        return false;
    }

    // Check for key ARM GNU Toolchain files to ensure it's a complete installation
    QStringList keyFiles = {
        "bin/arm-none-eabi-gcc",
        "bin/arm-none-eabi-g++",
        "bin/arm-none-eabi-as",
        "bin/arm-none-eabi-ld"
    };

    // On Windows, add .exe extension
#ifdef Q_OS_WIN
    for (QString& file : keyFiles) {
        file += ".exe";
    }
#endif

    for (const QString& file : keyFiles) {
        if (!QFile::exists(toolchainDir.absoluteFilePath(file))) {
            qDebug() << "Missing ARM GNU Toolchain file:" << file;
            return false;
        }
    }

    return true;
}

bool LibraryChecker::isNrf52FirmwarePresent()
{
    QString librariesPath = getLibrariesPath();
    QDir firmwareDir(librariesPath + "/" + NRF52_FIRMWARE_FOLDER);

    // Check if nRF52 firmware directory exists and contains key source files
    if (!firmwareDir.exists()) {
        return false;
    }

    // Check for key firmware source files to ensure it's a complete installation
    QStringList keyFiles = {
        "CMakeLists.txt",
        "src",
        "lv_conf.h"
    };

    for (const QString& file : keyFiles) {
        if (!QFile::exists(firmwareDir.absoluteFilePath(file))) {
            qDebug() << "Missing nRF52 firmware file:" << file;
            return false;
        }
    }

    return true;
}

bool LibraryChecker::isCMakePresent()
{
    QString librariesPath = getLibrariesPath();
    QDir cmakeDir(librariesPath + "/" + CMAKE_FOLDER);

    // Check if CMake directory exists and contains key files
    if (!cmakeDir.exists()) {
        return false;
    }

    // Check for key CMake files to ensure it's a complete installation
    QStringList keyFiles = {
        "bin/cmake",
        "share/cmake-4.1/Modules/CMakeDetermineSystem.cmake"
    };

    // On Windows, add .exe extension to executables
#ifdef Q_OS_WIN
    keyFiles[0] += ".exe";
#endif

    for (const QString& file : keyFiles) {
        if (!QFile::exists(cmakeDir.absoluteFilePath(file))) {
            qDebug() << "Missing CMake file:" << file;
            return false;
        }
    }

    return true;
}

void LibraryChecker::downloadLvgl()
{
    m_downloadSuccess = false;
    m_currentDownloadType = DownloadType::LVGL;
    
    // Create progress dialog
    m_progressDialog = new QProgressDialog(
        "Downloading LVGL library...",
        "Cancel",
        0, 100,
        m_parent
    );
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setAutoClose(true);
    m_progressDialog->setAutoReset(true);
    
    // Create temporary file path manually to avoid QTemporaryFile handle issues
    m_tempFilePath = QDir::tempPath() + "/lvgl_" +
                     QString::number(QDateTime::currentMSecsSinceEpoch()) + ".zip";

    qDebug() << "Will download to:" << m_tempFilePath;
    
    // Start download
    QNetworkRequest request{QUrl(LVGL_URL)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "LCD-GUI-Tester/1.0");
    
    m_currentReply = m_networkManager->get(request);
    
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &LibraryChecker::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &LibraryChecker::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred,
            this, &LibraryChecker::onDownloadError);
    
    // Connect cancel button
    connect(m_progressDialog, &QProgressDialog::canceled, [this]() {
        if (m_currentReply) {
            m_currentReply->abort();
        }
    });
    
    m_progressDialog->show();
    
    // Block until download completes using event loop
    QEventLoop loop;
    connect(m_currentReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(m_progressDialog, &QProgressDialog::canceled, &loop, &QEventLoop::quit);
    loop.exec();
}

void LibraryChecker::downloadNrf52Sdk()
{
    m_downloadSuccess = false;
    m_currentDownloadType = DownloadType::NRF52_SDK;
    
    // Create progress dialog
    m_progressDialog = new QProgressDialog(
        "Downloading nRF52 SDK...",
        "Cancel",
        0, 100,
        m_parent
    );
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setAutoClose(true);
    m_progressDialog->setAutoReset(true);
    
    // Create temporary file path manually to avoid QTemporaryFile handle issues
    m_tempFilePath = QDir::tempPath() + "/nrf5_sdk_" +
                     QString::number(QDateTime::currentMSecsSinceEpoch()) + ".zip";

    qDebug() << "Will download to:" << m_tempFilePath;
    
    // Start download
    QNetworkRequest request{QUrl(NRF52_SDK_URL)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "LCD-GUI-Tester/1.0");
    
    m_currentReply = m_networkManager->get(request);
    
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &LibraryChecker::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &LibraryChecker::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred,
            this, &LibraryChecker::onDownloadError);
    
    // Connect cancel button
    connect(m_progressDialog, &QProgressDialog::canceled, [this]() {
        if (m_currentReply) {
            m_currentReply->abort();
        }
    });
    
    m_progressDialog->show();
    
    // Block until download completes using event loop
    QEventLoop loop;
    connect(m_currentReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(m_progressDialog, &QProgressDialog::canceled, &loop, &QEventLoop::quit);
    loop.exec();
}

void LibraryChecker::downloadArmGnuToolchain()
{
    m_downloadSuccess = false;
    m_currentDownloadType = DownloadType::ARM_GNU_TOOLCHAIN;
    
    // Create progress dialog
    m_progressDialog = new QProgressDialog(
        "Downloading ARM GNU Toolchain...",
        "Cancel",
        0, 100,
        m_parent
    );
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setAutoClose(true);
    m_progressDialog->setAutoReset(true);
    
    // Create temporary file path manually to avoid QTemporaryFile handle issues
    QString toolchainUrl = getArmGnuToolchainUrl();
    QString fileExtension = toolchainUrl.endsWith(".zip") ? ".zip" : ".tar.xz";
    m_tempFilePath = QDir::tempPath() + "/arm_gnu_toolchain_" +
                     QString::number(QDateTime::currentMSecsSinceEpoch()) + fileExtension;

    qDebug() << "Will download to:" << m_tempFilePath;
    
    // Start download
    QNetworkRequest request{QUrl(toolchainUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "LCD-GUI-Tester/1.0");
    
    m_currentReply = m_networkManager->get(request);
    
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &LibraryChecker::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &LibraryChecker::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred,
            this, &LibraryChecker::onDownloadError);
    
    // Connect cancel button
    connect(m_progressDialog, &QProgressDialog::canceled, [this]() {
        if (m_currentReply) {
            m_currentReply->abort();
        }
    });
    
    m_progressDialog->show();
    
    // Block until download completes using event loop
    QEventLoop loop;
    connect(m_currentReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(m_progressDialog, &QProgressDialog::canceled, &loop, &QEventLoop::quit);
    loop.exec();
}

QString LibraryChecker::getNrf52FirmwareLatestReleaseUrl()
{
    // Query GitHub API for tags to get the latest tag
    QNetworkRequest request{QUrl(NRF52_FIRMWARE_TAGS_API_URL)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "LCD-GUI-Tester/1.0");

    QNetworkReply* reply = m_networkManager->get(request);

    // Block until request completes
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QString downloadUrl;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

        if (jsonDoc.isArray()) {
            QJsonArray tagsArray = jsonDoc.array();

            // Get the first tag (latest)
            if (!tagsArray.isEmpty()) {
                QJsonObject latestTag = tagsArray[0].toObject();
                QString tagName = latestTag["name"].toString();

                if (!tagName.isEmpty()) {
                    // Construct download URL: https://github.com/INFIseven/nrf52-lcd-tester-fw/archive/refs/tags/{tag}.zip
                    downloadUrl = QString(NRF52_FIRMWARE_REPO_URL) + tagName + ".zip";
                    qDebug() << "Found latest firmware tag:" << tagName << "URL:" << downloadUrl;
                }
            }
        }
    } else {
        qDebug() << "Failed to fetch tags info:" << reply->errorString();
    }

    reply->deleteLater();
    return downloadUrl;
}

void LibraryChecker::downloadNrf52Firmware()
{
    m_downloadSuccess = false;
    m_currentDownloadType = DownloadType::NRF52_FIRMWARE;

    // Get the latest release URL
    QString firmwareUrl = getNrf52FirmwareLatestReleaseUrl();

    if (firmwareUrl.isEmpty()) {
        qDebug() << "Failed to retrieve the latest firmware release from GitHub";
        m_downloadSuccess = false;
        return;
    }

    // Create progress dialog
    m_progressDialog = new QProgressDialog(
        "Downloading nRF52 LCD Tester Firmware...",
        "Cancel",
        0, 100,
        m_parent
    );
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setAutoClose(true);
    m_progressDialog->setAutoReset(true);

    // Create temporary file path manually to avoid QTemporaryFile handle issues
    m_tempFilePath = QDir::tempPath() + "/nrf52_firmware_" +
                     QString::number(QDateTime::currentMSecsSinceEpoch()) + ".zip";

    qDebug() << "Will download to:" << m_tempFilePath;

    // Start download
    QNetworkRequest request{QUrl(firmwareUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "LCD-GUI-Tester/1.0");

    m_currentReply = m_networkManager->get(request);

    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &LibraryChecker::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &LibraryChecker::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred,
            this, &LibraryChecker::onDownloadError);

    // Connect cancel button
    connect(m_progressDialog, &QProgressDialog::canceled, [this]() {
        if (m_currentReply) {
            m_currentReply->abort();
        }
    });

    m_progressDialog->show();

    // Block until download completes using event loop
    QEventLoop loop;
    connect(m_currentReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(m_progressDialog, &QProgressDialog::canceled, &loop, &QEventLoop::quit);
    loop.exec();
}

void LibraryChecker::downloadCMake()
{
    m_downloadSuccess = false;
    m_currentDownloadType = DownloadType::CMAKE;

    // Create progress dialog
    m_progressDialog = new QProgressDialog(
        "Downloading CMake...",
        "Cancel",
        0, 100,
        m_parent
    );
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setAutoClose(true);
    m_progressDialog->setAutoReset(true);

    // Create temporary file path manually to avoid QTemporaryFile handle issues
    QString cmakeUrl = getCMakeUrl();
    QString fileExtension = cmakeUrl.endsWith(".zip") ? ".zip" : ".tar.gz";
    m_tempFilePath = QDir::tempPath() + "/cmake_" +
                     QString::number(QDateTime::currentMSecsSinceEpoch()) + fileExtension;

    qDebug() << "Will download to:" << m_tempFilePath;

    // Start download
    QNetworkRequest request{QUrl(cmakeUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, "LCD-GUI-Tester/1.0");

    m_currentReply = m_networkManager->get(request);

    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &LibraryChecker::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &LibraryChecker::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred,
            this, &LibraryChecker::onDownloadError);

    // Connect cancel button
    connect(m_progressDialog, &QProgressDialog::canceled, [this]() {
        if (m_currentReply) {
            m_currentReply->abort();
        }
    });

    m_progressDialog->show();

    // Block until download completes using event loop
    QEventLoop loop;
    connect(m_currentReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(m_progressDialog, &QProgressDialog::canceled, &loop, &QEventLoop::quit);
    loop.exec();
}

void LibraryChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (m_progressDialog && bytesTotal > 0) {
        int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
        m_progressDialog->setValue(percentage);
        
        // Update label with size information
        QString sizeText = QString("Downloaded %1 KB of %2 KB")
                          .arg(bytesReceived / 1024)
                          .arg(bytesTotal / 1024);
        
        // Determine what we're downloading based on current label
        QString baseLabel = m_progressDialog->labelText().split('\n').first();
        m_progressDialog->setLabelText(baseLabel + "\n" + sizeText);
    }
}

void LibraryChecker::onDownloadFinished()
{
    if (!m_currentReply) {
        return;
    }
    
    if (m_progressDialog) {
        m_progressDialog->setValue(100);
    }
    
    if (m_currentReply->error() == QNetworkReply::NoError) {
        // Read all data from reply first
        QByteArray downloadedData = m_currentReply->readAll();

        // Delete reply immediately to release any file handles
        m_currentReply->deleteLater();
        m_currentReply = nullptr;

        // Save downloaded data in a scoped block to ensure file handle is released
        QString librariesPath = getLibrariesPath();
        bool fileSaved = false;
        qint64 fileSize = 0;

        {
            QFile file(m_tempFilePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(downloadedData);
                file.flush(); // Ensure all data is written to disk
                fileSize = file.size();
                file.close();
                fileSaved = true;
            }
            // file destructor called here when going out of scope
        }

        // Clear the downloaded data from memory
        downloadedData.clear();

        if (!fileSaved) {
            m_downloadSuccess = false;
            qDebug() << "Failed to save downloaded file";

            if (m_progressDialog) {
                m_progressDialog->close();
                m_progressDialog->deleteLater();
                m_progressDialog = nullptr;
            }
            return;
        }

        // Wait for Windows to fully release all file handles
        qDebug() << "Waiting for file handles to be released...";
        QThread::msleep(1000); // Wait 1 second for Windows to release all handles

        // Extract the zip file
        QDir().mkpath(librariesPath);

        qDebug() << "Downloaded file saved to:" << m_tempFilePath;
        qDebug() << "Libraries path:" << librariesPath;
        qDebug() << "File exists:" << QFile::exists(m_tempFilePath);
        qDebug() << "File size:" << fileSize << "bytes";

        // Determine target folder and message based on download type
        QString targetFolder;
        QString successMsg;
        QString failureMsg;

        if (m_currentDownloadType == DownloadType::LVGL) {
            targetFolder = LVGL_FOLDER;
            successMsg = "LVGL library has been successfully downloaded and extracted.";
            failureMsg = "Failed to extract LVGL library. Please try again or install manually.";
        } else if (m_currentDownloadType == DownloadType::NRF52_SDK) {
            targetFolder = NRF52_SDK_FOLDER;
            successMsg = "nRF52 SDK has been successfully downloaded and extracted.";
            failureMsg = "Failed to extract nRF52 SDK. Please try again or install manually.";
        } else if (m_currentDownloadType == DownloadType::ARM_GNU_TOOLCHAIN) {
            targetFolder = ARM_GNU_TOOLCHAIN_FOLDER;
            successMsg = "ARM GNU Toolchain has been successfully downloaded and extracted.";
            failureMsg = "Failed to extract ARM GNU Toolchain. Please try again or install manually.";
        } else if (m_currentDownloadType == DownloadType::NRF52_FIRMWARE) {
            targetFolder = NRF52_FIRMWARE_FOLDER;
            successMsg = "nRF52 LCD Tester Firmware has been successfully downloaded and extracted.";
            failureMsg = "Failed to extract nRF52 LCD Tester Firmware. Please try again or install manually.";
        } else if (m_currentDownloadType == DownloadType::CMAKE) {
            targetFolder = CMAKE_FOLDER;
            successMsg = "CMake has been successfully downloaded and extracted.";
            failureMsg = "Failed to extract CMake. Please try again or install manually.";
        }

        bool extractionSuccess = false;

        // Choose extraction method based on file type and download type
        if ((m_currentDownloadType == DownloadType::ARM_GNU_TOOLCHAIN && m_tempFilePath.endsWith(".tar.xz")) ||
            (m_currentDownloadType == DownloadType::CMAKE && m_tempFilePath.endsWith(".tar.gz"))) {
            extractionSuccess = extractTarFile(m_tempFilePath, librariesPath, targetFolder);
        } else {
            extractionSuccess = extractZipFile(m_tempFilePath, librariesPath, targetFolder);
        }

        if (extractionSuccess) {
            m_downloadSuccess = true;
            // Success - no individual dialog shown
            qDebug() << successMsg;
        } else {
            m_downloadSuccess = false;
            // Failure - no individual dialog shown
            qDebug() << failureMsg;
        }
    } else {
        m_downloadSuccess = false;
        // Delete reply on error
        if (m_currentReply) {
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
        }
    }

    // Clean up progress dialog (reply already deleted in success case)
    if (m_progressDialog) {
        m_progressDialog->close();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }
}

void LibraryChecker::onDownloadError(QNetworkReply::NetworkError error)
{
    m_downloadSuccess = false;
    QString errorMsg = QString("Download failed: %1").arg(m_currentReply->errorString());
    qDebug() << errorMsg;

    if (m_progressDialog) {
        m_progressDialog->close();
    }
}

bool LibraryChecker::extractZipFile(const QString& zipPath, const QString& extractPath, const QString& targetFolder)
{
    QProcess unzipProcess;

#ifdef Q_OS_WIN
    // Use PowerShell with direct .NET approach - more reliable than Expand-Archive
    // Create a temporary extraction directory with SHORT path to avoid Windows 260 char path limit
    // Use C:/Temp instead of the libraries path which might already be long
    QString tempExtractBase = "C:/Temp/lcd_extract_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QDir().mkpath(tempExtractBase);

    QString command = "powershell";
    QStringList arguments;
    arguments << "-NoProfile" << "-ExecutionPolicy" << "Bypass" << "-Command"
              << QString("Add-Type -AssemblyName System.IO.Compression.FileSystem; "
                        "[System.IO.Compression.ZipFile]::ExtractToDirectory('%1', '%2')")
                 .arg(zipPath).arg(tempExtractBase);
#else
    // Use unzip command on Linux/macOS
    QString command = "unzip";
    QStringList arguments;
    arguments << "-o" << zipPath << "-d" << extractPath;
#endif

    unzipProcess.start(command, arguments);
    unzipProcess.waitForFinished(120000); // 120 second timeout for larger files

    // Read output first for debugging
    QString stdOutput = unzipProcess.readAllStandardOutput();
    QString stdError = unzipProcess.readAllStandardError();
    int exitCode = unzipProcess.exitCode();

    qDebug() << "Extraction command:" << command << arguments.join(" ");
    qDebug() << "Exit code:" << exitCode;
    qDebug() << "Standard output:" << stdOutput;
    qDebug() << "Standard error:" << stdError;

    // On Windows, PowerShell may return 0 even on failure, so check stderr too
    bool hasErrors = false;
#ifdef Q_OS_WIN
    hasErrors = stdError.contains("Exception", Qt::CaseInsensitive) ||
                stdError.contains("Error", Qt::CaseInsensitive) ||
                stdError.contains("cannot access", Qt::CaseInsensitive);
#endif

    if (exitCode == 0 && !hasErrors) {
#ifdef Q_OS_WIN
        // On Windows, we extracted to a temp directory, so use that
        QDir extractDir(tempExtractBase);
#else
        QDir extractDir(extractPath);
#endif
        QStringList entries = extractDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        
        // Handle different extraction patterns based on target folder
        for (const QString& entry : entries) {
            QString oldPath = extractDir.absoluteFilePath(entry);
            QString newPath;
            bool shouldRename = false;

            if (targetFolder == LVGL_FOLDER && entry.startsWith("lvgl-")) {
                newPath = extractDir.absoluteFilePath(LVGL_FOLDER);
                shouldRename = true;
            } else if (targetFolder == NRF52_SDK_FOLDER && (entry.startsWith("nRF5_SDK_") || entry.startsWith("nrf5_sdk_") || entry == "nRF5_SDK_17.1.0_ddde560")) {
                newPath = extractDir.absoluteFilePath(NRF52_SDK_FOLDER);
                shouldRename = true;
            } else if (targetFolder == ARM_GNU_TOOLCHAIN_FOLDER && entry.startsWith("arm-gnu-toolchain-")) {
                newPath = extractDir.absoluteFilePath(ARM_GNU_TOOLCHAIN_FOLDER);
                shouldRename = true;
            } else if (targetFolder == NRF52_FIRMWARE_FOLDER && entry.startsWith("nrf52-lcd-tester-fw")) {
                newPath = extractDir.absoluteFilePath(NRF52_FIRMWARE_FOLDER);
                shouldRename = true;
            } else if (targetFolder == CMAKE_FOLDER && entry.startsWith("cmake-")) {
                newPath = extractDir.absoluteFilePath(CMAKE_FOLDER);
                shouldRename = true;
            }

            if (shouldRename) {
                // Remove existing folder if it exists
                QString finalPath = extractPath + "/" + QFileInfo(newPath).fileName();
                QDir(finalPath).removeRecursively();

                // Move the extracted folder to final location
                bool renameSuccess = QDir().rename(oldPath, finalPath);

                if (renameSuccess) {
                    qDebug() << "Successfully moved" << oldPath << "to" << finalPath;

                    // Verify the extracted folder exists and has content
                    QDir verifyDir(finalPath);
                    if (!verifyDir.exists()) {
                        qDebug() << "ERROR: Moved folder does not exist:" << finalPath;
#ifdef Q_OS_WIN
                        // Clean up temp directory
                        QDir(tempExtractBase).removeRecursively();
#endif
                        return false;
                    }

                    QStringList contents = verifyDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
                    if (contents.isEmpty()) {
                        qDebug() << "ERROR: Extracted folder is empty:" << finalPath;
#ifdef Q_OS_WIN
                        // Clean up temp directory
                        QDir(tempExtractBase).removeRecursively();
#endif
                        return false;
                    }

                    qDebug() << "Extraction verified. Folder contains" << contents.size() << "items";

#ifdef Q_OS_WIN
                    // Clean up temp directory
                    QDir(tempExtractBase).removeRecursively();
#endif
                    // Delete the .zip file
                    QFile::remove(zipPath);
                    return true;
                } else {
                    qDebug() << "ERROR: Failed to move" << oldPath << "to" << finalPath;
#ifdef Q_OS_WIN
                    // Clean up temp directory
                    QDir(tempExtractBase).removeRecursively();
#endif
                    return false;
                }
            }
        }

        // If no rename needed (direct extraction), just delete the zip
        qDebug() << "No rename needed, extraction complete";
#ifdef Q_OS_WIN
        // Clean up temp directory
        QDir(tempExtractBase).removeRecursively();
#endif
        QFile::remove(zipPath);
        return true;
    }

    qDebug() << "Extraction failed - exit code:" << exitCode << "or errors detected in stderr";
#ifdef Q_OS_WIN
    // Clean up temp directory on failure
    QDir(tempExtractBase).removeRecursively();
#endif
    return false;
}

bool LibraryChecker::extractTarFile(const QString& tarPath, const QString& extractPath, const QString& targetFolder)
{
    // Use system tar command for .tar.xz extraction
    QProcess tarProcess;
    QStringList arguments;
    arguments << "-xf" << tarPath << "-C" << extractPath;
    
    tarProcess.start("tar", arguments);
    tarProcess.waitForFinished(120000); // 120 second timeout for large files

    // Read output first for debugging
    QString stdOutput = tarProcess.readAllStandardOutput();
    QString stdError = tarProcess.readAllStandardError();
    int exitCode = tarProcess.exitCode();

    qDebug() << "Tar extraction command: tar" << arguments.join(" ");
    qDebug() << "Exit code:" << exitCode;
    qDebug() << "Standard output:" << stdOutput;
    qDebug() << "Standard error:" << stdError;

    if (exitCode == 0) {
        QDir extractDir(extractPath);
        QStringList entries = extractDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        
        // Handle different extraction patterns based on target folder
        for (const QString& entry : entries) {
            QString oldPath = extractDir.absoluteFilePath(entry);
            QString newPath;
            bool shouldRename = false;
            
            if (targetFolder == ARM_GNU_TOOLCHAIN_FOLDER && entry.startsWith("arm-gnu-toolchain-")) {
                newPath = extractDir.absoluteFilePath(ARM_GNU_TOOLCHAIN_FOLDER);
                shouldRename = true;
            } else if (targetFolder == CMAKE_FOLDER && entry.startsWith("cmake-")) {
                newPath = extractDir.absoluteFilePath(CMAKE_FOLDER);
                shouldRename = true;
            }
            
            if (shouldRename) {
                // Remove existing folder if it exists
                QDir(newPath).removeRecursively();
                
                // Rename the extracted folder
                bool success = QDir().rename(oldPath, newPath);
                
                // Delete the .tar.xz file as requested
                QFile::remove(tarPath);
                
                return success;
            }
        }
        
        // If no rename needed (direct extraction), just delete the tar file
        QFile::remove(tarPath);
        return true;
    }
    
    qDebug() << "Tar process output:" << tarProcess.readAllStandardOutput();
    qDebug() << "Tar process errors:" << tarProcess.readAllStandardError();
    return false;
}

QString LibraryChecker::getArmGnuToolchainUrl()
{
    QString baseUrl = ARM_GNU_TOOLCHAIN_BASE_URL;
    QString platformSuffix;

#ifdef Q_OS_WIN
    // Windows x64
    platformSuffix = "mingw-w64-i686-arm-none-eabi.zip";
#elif defined(Q_OS_LINUX)
    // Linux x86_64
    platformSuffix = "x86_64-arm-none-eabi.tar.xz";
#elif defined(Q_OS_MACOS)
    // Detect macOS architecture
    QString architecture = QSysInfo::currentCpuArchitecture();
    if (architecture == "arm64" || architecture == "aarch64") {
        // macOS ARM64
        platformSuffix = "darwin-arm64-arm-none-eabi.tar.xz";
    } else {
        // macOS Intel
        platformSuffix = "darwin-x86_64-arm-none-eabi.tar.xz";
    }
#else
    // Fallback to Linux
    platformSuffix = "x86_64-arm-none-eabi.tar.xz";
#endif

    return baseUrl + platformSuffix;
}

QString LibraryChecker::getCMakeUrl()
{
    QString baseUrl = CMAKE_BASE_URL;
    QString platformSuffix;

#ifdef Q_OS_WIN
    // Windows x64
    platformSuffix = "windows-x86_64.zip";
#elif defined(Q_OS_LINUX)
    // Linux x86_64
    platformSuffix = "linux-x86_64.tar.gz";
#elif defined(Q_OS_MACOS)
    // macOS universal binary
    platformSuffix = "macos-universal.tar.gz";
#else
    // Fallback to Linux
    platformSuffix = "linux-x86_64.tar.gz";
#endif

    return baseUrl + platformSuffix;
}

QString LibraryChecker::getLibrariesPath()
{
    // Create libraries folder in the application directory
    QString appDir = QApplication::applicationDirPath();
    return appDir + "/libraries";
}

bool LibraryChecker::isPythonPresent()
{
    return m_embeddedPython->isEmbeddedPythonAvailable();
}

bool LibraryChecker::downloadPython()
{
    qDebug() << "Setting up embedded Python...";
    bool success = m_embeddedPython->setupEmbeddedPython();

    if (success) {
        qDebug() << "Embedded Python setup completed";
    } else {
        qDebug() << "Failed to setup embedded Python";
    }

    return success;
}

bool LibraryChecker::isPythonPackagesPresent(QStringList& missingPackages)
{
    missingPackages.clear();

    // If Python is not available, all packages are missing
    if (!isPythonPresent()) {
        return false;
    }

    QString pythonExe = m_embeddedPython->getEmbeddedPythonPath();
    QStringList requiredPackages = {"Pillow", "pypng", "lz4", "kconfiglib"};

    for (const QString& package : requiredPackages) {
        // Test each package
        QString testCommand;
        if (package == "Pillow") {
            testCommand = "import PIL.Image";
        } else if (package == "lz4") {
            testCommand = "import lz4.block";
        } else {
            testCommand = QString("import %1").arg(package == "pypng" ? "png" : package);
        }

        QProcess testProcess;
        testProcess.start(pythonExe, {"-c", testCommand});
        testProcess.waitForFinished(5000);

        if (testProcess.exitCode() != 0) {
            missingPackages.append(package);
            qDebug() << "Missing Python package:" << package;
        }
    }

    return missingPackages.isEmpty();
}

bool LibraryChecker::downloadPythonPackages(const QStringList& packages)
{
    qDebug() << "Installing Python packages:" << packages.join(", ");

    bool allSuccess = true;
    for (const QString& package : packages) {
        qDebug() << "Installing package:" << package;

        if (!m_embeddedPython->installPackage(package)) {
            qDebug() << "Failed to install package:" << package;
            allSuccess = false;
        } else {
            qDebug() << "Successfully installed package:" << package;
        }
    }

    if (allSuccess) {
        qDebug() << "All Python packages installed successfully";
    } else {
        qDebug() << "Some Python packages failed to install";
    }

    return allSuccess;
}