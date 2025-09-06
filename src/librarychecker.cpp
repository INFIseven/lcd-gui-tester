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
            return m_downloadSuccess;
        } else {
            QMessageBox::warning(
                m_parent,
                "Library Required",
                "LVGL library is required for this application to function properly.\n"
                "Please install it manually or restart the application to download it."
            );
            return false;
        }
    }
    
    return true;
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

void LibraryChecker::downloadLvgl()
{
    m_downloadSuccess = false;
    
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

void LibraryChecker::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (m_progressDialog && bytesTotal > 0) {
        int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
        m_progressDialog->setValue(percentage);
        
        // Update label with size information
        QString sizeText = QString("Downloaded %1 KB of %2 KB")
                          .arg(bytesReceived / 1024)
                          .arg(bytesTotal / 1024);
        m_progressDialog->setLabelText("Downloading LVGL library...\n" + sizeText);
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
            
            if (extractZipFile(m_tempFilePath, librariesPath)) {
                m_downloadSuccess = true;
                QMessageBox::information(
                    m_parent,
                    "Download Complete",
                    "LVGL library has been successfully downloaded and extracted."
                );
            } else {
                m_downloadSuccess = false;
                QMessageBox::critical(
                    m_parent,
                    "Extraction Failed",
                    "Failed to extract LVGL library. Please try again or install manually."
                );
            }
            
            // Clean up temporary file
            QFile::remove(m_tempFilePath);
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

bool LibraryChecker::extractZipFile(const QString& zipPath, const QString& extractPath)
{
    // Use system unzip command for simplicity
    QProcess unzipProcess;
    QStringList arguments;
    arguments << zipPath << "-d" << extractPath;
    
    unzipProcess.start("unzip", arguments);
    unzipProcess.waitForFinished(30000); // 30 second timeout
    
    if (unzipProcess.exitCode() == 0) {
        // Rename the extracted folder to just "lvgl"
        QDir extractDir(extractPath);
        QStringList entries = extractDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        
        for (const QString& entry : entries) {
            if (entry.startsWith("lvgl-")) {
                QString oldPath = extractDir.absoluteFilePath(entry);
                QString newPath = extractDir.absoluteFilePath(LVGL_FOLDER);
                
                // Remove existing lvgl folder if it exists
                QDir(newPath).removeRecursively();
                
                // Rename the extracted folder
                return QDir().rename(oldPath, newPath);
            }
        }
    }
    
    qDebug() << "Unzip process output:" << unzipProcess.readAllStandardOutput();
    qDebug() << "Unzip process errors:" << unzipProcess.readAllStandardError();
    return false;
}

QString LibraryChecker::getLibrariesPath()
{
    // Create libraries folder in the application directory
    QString appDir = QApplication::applicationDirPath();
    return appDir + "/libraries";
}