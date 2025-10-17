#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <QDateTime>

class EmbeddedPython;

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
    bool isArmGnuToolchainPresent();
    void downloadArmGnuToolchain();
    bool isNrf52FirmwarePresent();
    void downloadNrf52Firmware();
    bool isCMakePresent();
    void downloadCMake();
    bool isPythonPresent();
    bool downloadPython();
    bool isPythonPackagesPresent(QStringList& missingPackages);
    bool downloadPythonPackages(const QStringList& packages);
    QString getNrf52FirmwareLatestReleaseUrl();
    QString getCMakeUrl();
    bool extractZipFile(const QString& zipPath, const QString& extractPath, const QString& targetFolder = "");
    bool extractTarFile(const QString& tarPath, const QString& extractPath, const QString& targetFolder = "");
    QString getLibrariesPath();
    QString getArmGnuToolchainUrl();
    
    static constexpr const char* LVGL_URL = "https://github.com/lvgl/lvgl/archive/refs/tags/v9.3.0.zip";
    static constexpr const char* LVGL_FOLDER = "lvgl";
    static constexpr const char* NRF52_SDK_URL = "https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/nrf5_sdk_17.1.0_ddde560.zip";
    static constexpr const char* NRF52_SDK_FOLDER = "nrf5_sdk";
    static constexpr const char* ARM_GNU_TOOLCHAIN_VERSION = "13.2.rel1";
    static constexpr const char* ARM_GNU_TOOLCHAIN_BASE_URL = "https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-";
    static constexpr const char* ARM_GNU_TOOLCHAIN_FOLDER = "arm-gnu-toolchain";
    static constexpr const char* NRF52_FIRMWARE_TAGS_API_URL = "https://api.github.com/repos/INFIseven/nrf52-lcd-tester-fw/tags";
    static constexpr const char* NRF52_FIRMWARE_REPO_URL = "https://github.com/INFIseven/nrf52-lcd-tester-fw/archive/refs/tags/";
    static constexpr const char* NRF52_FIRMWARE_FOLDER = "nrf52-lcd-tester-fw";
    static constexpr const char* CMAKE_VERSION = "4.1.2";
    static constexpr const char* CMAKE_BASE_URL = "https://github.com/Kitware/CMake/releases/download/v4.1.2/cmake-4.1.2-";
    static constexpr const char* CMAKE_FOLDER = "cmake";

    QWidget* m_parent;
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply;
    QProgressDialog* m_progressDialog;
    QString m_tempFilePath;
    bool m_downloadSuccess;
    enum class DownloadType { LVGL, NRF52_SDK, ARM_GNU_TOOLCHAIN, NRF52_FIRMWARE, CMAKE } m_currentDownloadType;
    EmbeddedPython* m_embeddedPython;
};