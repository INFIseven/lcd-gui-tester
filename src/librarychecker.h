#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QDir>
#include <QStandardPaths>

class LibraryChecker : public QObject
{
    Q_OBJECT

public:
    explicit LibraryChecker(QWidget* parent = nullptr);
    ~LibraryChecker();
    
    bool checkAndDownloadLibraries();

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);

private:
    bool isLvglPresent();
    void downloadLvgl();
    bool extractZipFile(const QString& zipPath, const QString& extractPath);
    QString getLibrariesPath();
    
    static constexpr const char* LVGL_URL = "https://github.com/lvgl/lvgl/archive/refs/tags/v9.3.0.zip";
    static constexpr const char* LVGL_FOLDER = "lvgl";
    
    QWidget* m_parent;
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply;
    QProgressDialog* m_progressDialog;
    QString m_tempFilePath;
    bool m_downloadSuccess;
};