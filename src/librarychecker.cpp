#include "librarychecker.h"
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
{
    m_networkManager = new QNetworkAccessManager(this);
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
}

bool LibraryChecker::checkAndDownloadLibraries()
{
    bool allLibrariesPresent = true;
    
    if (!isLvglPresent()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            m_parent,
            "Missing Library",
            "LVGL library is not found and is required for this application.\n"
            "Would you like to download it now?\n\n"
            "This will download LVGL v9.3.0 from GitHub.",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );
        
        if (reply == QMessageBox::Yes) {
            downloadLvgl();
            allLibrariesPresent = allLibrariesPresent && m_downloadSuccess;
        } else {
            QMessageBox::warning(
                m_parent,
                "Library Required",
                "LVGL library is required for this application to function properly.\n"
                "Please install it manually or restart the application to download it."
            );
            allLibrariesPresent = false;
        }
    }
    
    if (!isNrf52SdkPresent()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            m_parent,
            "Missing SDK",
            "nRF52 SDK is not found and is required for this application.\n"
            "Would you like to download it now?\n\n"
            "This will download nRF5 SDK v17.1.0 from Nordic Semiconductor.",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );
        
        if (reply == QMessageBox::Yes) {
            downloadNrf52Sdk();
            allLibrariesPresent = allLibrariesPresent && m_downloadSuccess;
        } else {
            QMessageBox::warning(
                m_parent,
                "SDK Required",
                "nRF52 SDK is required for this application to function properly.\n"
                "Please install it manually or restart the application to download it."
            );
            allLibrariesPresent = false;
        }
    }
    
    if (!isArmGnuToolchainPresent()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            m_parent,
            "Missing Toolchain",
            "ARM GNU Toolchain is not found and is required for this application.\n"
            "Would you like to download it now?\n\n"
            "This will download ARM GNU Toolchain v" + QString(ARM_GNU_TOOLCHAIN_VERSION) + " from ARM Developer.",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );

        if (reply == QMessageBox::Yes) {
            downloadArmGnuToolchain();
            allLibrariesPresent = allLibrariesPresent && m_downloadSuccess;
        } else {
            QMessageBox::warning(
                m_parent,
                "Toolchain Required",
                "ARM GNU Toolchain is required for this application to function properly.\n"
                "Please install it manually or restart the application to download it."
            );
            allLibrariesPresent = false;
        }
    }

    if (!isNrf52FirmwarePresent()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            m_parent,
            "Missing Firmware",
            "nRF52 LCD Tester Firmware is not found and is required for this application.\n"
            "Would you like to download it now?\n\n"
            "This will download the latest release from GitHub.",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );

        if (reply == QMessageBox::Yes) {
            downloadNrf52Firmware();
            allLibrariesPresent = allLibrariesPresent && m_downloadSuccess;
        } else {
            QMessageBox::warning(
                m_parent,
                "Firmware Required",
                "nRF52 LCD Tester Firmware is required for this application to function properly.\n"
                "Please install it manually or restart the application to download it."
            );
            allLibrariesPresent = false;
        }
    }

    if (!isCMakePresent()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            m_parent,
            "Missing CMake",
            "CMake is not found and is required for building firmware.\n"
            "Would you like to download it now?\n\n"
            "This will download CMake v" + QString(CMAKE_VERSION) + " from GitHub.",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );

        if (reply == QMessageBox::Yes) {
            downloadCMake();
            allLibrariesPresent = allLibrariesPresent && m_downloadSuccess;
        } else {
            QMessageBox::warning(
                m_parent,
                "CMake Required",
                "CMake is required for building firmware.\n"
                "Please install it manually or restart the application to download it."
            );
            allLibrariesPresent = false;
        }
    }

    return allLibrariesPresent;
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

    // Check for key firmware source files (not built hex files)
    QString cmakeListsPath = firmwareDir.absoluteFilePath("CMakeLists.txt");
    if (!QFile::exists(cmakeListsPath)) {
        qDebug() << "Missing nRF52 firmware CMakeLists.txt";
        return false;
    }

    qDebug() << "Found nRF52 firmware source code";
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

    // Check for CMake executable
    QString cmakeExe = "bin/cmake";
#ifdef Q_OS_WIN
    cmakeExe += ".exe";
#endif

    if (!QFile::exists(cmakeDir.absoluteFilePath(cmakeExe))) {
        qDebug() << "Missing CMake executable:" << cmakeExe;
        return false;
    }

    qDebug() << "CMake found";
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
    
    // Create temporary file for download
    QTemporaryFile* tempFile = new QTemporaryFile(this);
    tempFile->setFileTemplate(QDir::tempPath() + "/lvgl_XXXXXX.zip");
    
    if (!tempFile->open()) {
        QMessageBox::critical(m_parent, "Error", "Failed to create temporary file for download.");
        return;
    }
    
    m_tempFilePath = tempFile->fileName();
    tempFile->close();
    
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
    
    // Create temporary file for download
    QTemporaryFile* tempFile = new QTemporaryFile(this);
    tempFile->setFileTemplate(QDir::tempPath() + "/nrf5_sdk_XXXXXX.zip");
    
    if (!tempFile->open()) {
        QMessageBox::critical(m_parent, "Error", "Failed to create temporary file for download.");
        return;
    }
    
    m_tempFilePath = tempFile->fileName();
    tempFile->close();
    
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
    
    // Create temporary file for download
    QTemporaryFile* tempFile = new QTemporaryFile(this);
    QString toolchainUrl = getArmGnuToolchainUrl();
    QString fileExtension = toolchainUrl.endsWith(".zip") ? ".zip" : ".tar.xz";
    tempFile->setFileTemplate(QDir::tempPath() + "/arm_gnu_toolchain_XXXXXX" + fileExtension);
    
    if (!tempFile->open()) {
        QMessageBox::critical(m_parent, "Error", "Failed to create temporary file for download.");
        return;
    }
    
    m_tempFilePath = tempFile->fileName();
    tempFile->close();
    
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
        QMessageBox::critical(
            m_parent,
            "Error",
            "Failed to retrieve the latest firmware release from GitHub.\n"
            "Please check your internet connection or download manually from:\n"
            "https://github.com/INFIseven/nrf52-lcd-tester-fw/releases"
        );
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

    // Create temporary file for download
    QTemporaryFile* tempFile = new QTemporaryFile(this);
    tempFile->setFileTemplate(QDir::tempPath() + "/nrf52_firmware_XXXXXX.zip");

    if (!tempFile->open()) {
        QMessageBox::critical(m_parent, "Error", "Failed to create temporary file for download.");
        return;
    }

    m_tempFilePath = tempFile->fileName();
    tempFile->close();

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

    // Create temporary file for download
    QTemporaryFile* tempFile = new QTemporaryFile(this);
    QString cmakeUrl = getCMakeUrl();
    QString fileExtension = cmakeUrl.endsWith(".zip") ? ".zip" : ".tar.gz";
    tempFile->setFileTemplate(QDir::tempPath() + "/cmake_XXXXXX" + fileExtension);

    if (!tempFile->open()) {
        QMessageBox::critical(m_parent, "Error", "Failed to create temporary file for download.");
        return;
    }

    m_tempFilePath = tempFile->fileName();
    tempFile->close();

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
        // Save downloaded data
        QFile file(m_tempFilePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(m_currentReply->readAll());
            file.close();
            
            // Extract the zip file
            QString librariesPath = getLibrariesPath();
            QDir().mkpath(librariesPath);
            
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
                QMessageBox::information(
                    m_parent,
                    "Download Complete",
                    successMsg
                );
            } else {
                m_downloadSuccess = false;
                QMessageBox::critical(
                    m_parent,
                    "Extraction Failed",
                    failureMsg
                );
            }
        } else {
            m_downloadSuccess = false;
            QMessageBox::critical(m_parent, "Error", "Failed to save downloaded file.");
        }
    } else {
        m_downloadSuccess = false;
    }
    
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    
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
    QMessageBox::critical(m_parent, "Download Error", errorMsg);
    
    if (m_progressDialog) {
        m_progressDialog->close();
    }
}

bool LibraryChecker::extractZipFile(const QString& zipPath, const QString& extractPath, const QString& targetFolder)
{
    // Use system unzip command for simplicity
    QProcess unzipProcess;
    QStringList arguments;
    arguments << zipPath << "-d" << extractPath;
    
    unzipProcess.start("unzip", arguments);
    unzipProcess.waitForFinished(60000); // 60 second timeout for larger files
    
    if (unzipProcess.exitCode() == 0) {
        QDir extractDir(extractPath);
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
            } else if (targetFolder == NRF52_FIRMWARE_FOLDER && entry.startsWith("nrf52-lcd-tester-fw")) {
                newPath = extractDir.absoluteFilePath(NRF52_FIRMWARE_FOLDER);
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

                // Delete the .zip file as requested
                QFile::remove(zipPath);

                return success;
            }
        }
        
        // If no rename needed (direct extraction), just delete the zip
        QFile::remove(zipPath);
        return true;
    }
    
    qDebug() << "Unzip process output:" << unzipProcess.readAllStandardOutput();
    qDebug() << "Unzip process errors:" << unzipProcess.readAllStandardError();
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
    
    if (tarProcess.exitCode() == 0) {
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