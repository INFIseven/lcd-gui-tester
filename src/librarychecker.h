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
    bool isNrf52SdkPresent();
    void downloadNrf52Sdk();
    bool extractZipFile(const QString& zipPath, const QString& extractPath, const QString& targetFolder = "");
    QString getLibrariesPath();
    
    static constexpr const char* LVGL_URL = "https://github.com/lvgl/lvgl/archive/refs/tags/v9.3.0.zip";
    static constexpr const char* LVGL_FOLDER = "lvgl";
    static constexpr const char* NRF52_SDK_URL = "https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/nrf5_sdk_17.1.0_ddde560.zip";
    static constexpr const char* NRF52_SDK_FOLDER = "nrf5_sdk";
    
    QWidget* m_parent;
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply;
    QProgressDialog* m_progressDialog;
    QString m_tempFilePath;
    bool m_downloadSuccess;
    enum class DownloadType { LVGL, NRF52_SDK } m_currentDownloadType;
};