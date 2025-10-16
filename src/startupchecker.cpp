#include "startupchecker.h"
#include "librarychecker.h"
#include "embeddedpython.h"
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QProcess>

StartupChecker::StartupChecker(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_libraryChecker(nullptr)
    , m_embeddedPython(nullptr)
{
    m_libraryChecker = new LibraryChecker(parent);
    m_embeddedPython = new EmbeddedPython(parent);
}

StartupChecker::~StartupChecker()
{
    if (m_libraryChecker) {
        m_libraryChecker->deleteLater();
    }
    if (m_embeddedPython) {
        m_embeddedPython->deleteLater();
    }
}

bool StartupChecker::performStartupCheck()
{
    qDebug() << "Performing startup component check...";

    // LibraryChecker now handles all components including Python
    // It will check, download, and show a single final status dialog
    bool success = m_libraryChecker->checkAndDownloadLibraries();

    if (success) {
        qDebug() << "All components setup successfully";
    } else {
        qDebug() << "Failed to setup some components";
    }

    return success;
}

StartupChecker::MissingComponents StartupChecker::checkAllComponents()
{
    MissingComponents missing;

    // Check LVGL library
    QString appDir = QApplication::applicationDirPath();
    QString lvglPath = appDir + "/libraries/lvgl/scripts/LVGLImage.py";
    if (!QFile::exists(lvglPath)) {
        missing.needsLVGL = true;
        qDebug() << "LVGL library is missing";
    } else {
        qDebug() << "LVGL library found";
    }

    // Check nRF52 SDK
    QString nrfSdkPath = appDir + "/libraries/nrf5_sdk/components/softdevice/s132/headers/nrf_sdm.h";
    if (!QFile::exists(nrfSdkPath)) {
        missing.needsNrf52Sdk = true;
        qDebug() << "nRF52 SDK is missing";
    } else {
        qDebug() << "nRF52 SDK found";
    }

    // Check ARM GNU Toolchain
    QString gccPath = appDir + "/libraries/arm-gnu-toolchain/bin/arm-none-eabi-gcc";
#ifdef Q_OS_WIN
    gccPath += ".exe";
#endif
    if (!QFile::exists(gccPath)) {
        missing.needsArmGnuToolchain = true;
        qDebug() << "ARM GNU Toolchain is missing";
    } else {
        qDebug() << "ARM GNU Toolchain found";
    }

    // Check nRF52 Firmware (source code, not built hex)
    QString firmwarePath = appDir + "/libraries/nrf52-lcd-tester-fw";
    QDir firmwareDir(firmwarePath);
    if (!firmwareDir.exists() ||
        !QFile::exists(firmwarePath + "/CMakeLists.txt") ||
        !QFile::exists(firmwarePath + "/src") ||
        !QFile::exists(firmwarePath + "/lv_conf.h")) {
        missing.needsNrf52Firmware = true;
        qDebug() << "nRF52 LCD Tester Firmware is missing";
    } else {
        qDebug() << "nRF52 LCD Tester Firmware found";
    }

    // Check CMake
    QString cmakeBasePath = appDir + "/libraries/cmake";
    QDir cmakeDir(cmakeBasePath);
    QString cmakeExe = cmakeBasePath + "/bin/cmake";
#ifdef Q_OS_WIN
    cmakeExe += ".exe";
#endif
    if (!cmakeDir.exists() ||
        !QFile::exists(cmakeExe) ||
        !QFile::exists(cmakeBasePath + "/share/cmake-4.1/Modules/CMakeDetermineSystem.cmake")) {
        missing.needsCMake = true;
        qDebug() << "CMake is missing";
    } else {
        qDebug() << "CMake found";
    }

    // Check embedded Python
    if (!m_embeddedPython->isEmbeddedPythonAvailable()) {
        missing.needsPython = true;
        qDebug() << "Embedded Python is missing";
    } else {
        qDebug() << "Embedded Python found";

        // Check Python packages if Python is available
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
                missing.needsPythonPackages = true;
                missing.missingPackages.append(package);
                qDebug() << "Missing Python package:" << package;
            }
        }

        if (missing.missingPackages.isEmpty()) {
            qDebug() << "All Python packages are available";
        }
    }

    return missing;
}

bool StartupChecker::requestUserPermission(const MissingComponents& missing)
{
    QString message = "The application requires some components to function properly:\n\n";
    QStringList missingItems;

    // List all missing components (including Python)
    if (missing.needsLVGL) {
        missingItems.append("• LVGL library (~15MB)");
    }

    if (missing.needsNrf52Sdk) {
        missingItems.append("• nRF52 SDK (~150MB)");
    }

    if (missing.needsArmGnuToolchain) {
        missingItems.append("• ARM GNU Toolchain (~100MB)");
    }

    if (missing.needsNrf52Firmware) {
        missingItems.append("• nRF52 LCD Tester Firmware (~1MB)");
    }

    if (missing.needsCMake) {
        missingItems.append("• CMake (~40MB)");
    }

    if (missing.needsPython) {
        missingItems.append("• Embedded Python distribution (~25MB)");
    }

    if (missing.needsPythonPackages && !missing.needsPython) {
        missingItems.append(QString("• Python packages: %1").arg(missing.missingPackages.join(", ")));
    }

    message += missingItems.join("\n");
    message += "\n\nWould you like to download and install these components now?\n";
    message += "This is required for the application to function properly.";

    QMessageBox::StandardButton reply = QMessageBox::question(
        m_parent,
        "Setup Required Components",
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    return reply == QMessageBox::Yes;
}

bool StartupChecker::downloadAndSetupComponents(const MissingComponents& missing)
{
    bool librariesSuccess = true;
    bool pythonSuccess = true;
    QStringList failedPythonPackages;

    // Download libraries (LVGL, nRF52 SDK, ARM Toolchain, Firmware, CMake) if any are needed
    // LibraryChecker will show its own dialog with status for libraries only
    if (missing.needsLVGL || missing.needsNrf52Sdk ||
        missing.needsArmGnuToolchain || missing.needsNrf52Firmware || missing.needsCMake) {
        qDebug() << "Setting up required libraries...";
        librariesSuccess = m_libraryChecker->checkAndDownloadLibraries();
        if (!librariesSuccess) {
            qDebug() << "Failed to setup some libraries";
        } else {
            qDebug() << "Libraries setup completed";
        }
    }

    // Download Python components silently
    if (missing.needsPython) {
        qDebug() << "Downloading embedded Python...";
        if (!m_embeddedPython->setupEmbeddedPython()) {
            qDebug() << "Failed to setup embedded Python";
            pythonSuccess = false;
        } else {
            qDebug() << "Embedded Python setup completed";
        }
    } else if (missing.needsPythonPackages) {
        // Only install missing packages if Python is already available
        qDebug() << "Downloading missing Python packages...";

        for (const QString& package : missing.missingPackages) {
            if (!m_embeddedPython->installPackage(package)) {
                failedPythonPackages.append(package);
                qDebug() << "Failed to install package:" << package;
            } else {
                qDebug() << "Successfully installed package:" << package;
            }
        }

        if (!failedPythonPackages.isEmpty()) {
            pythonSuccess = false;
        }
    }

    // Python downloads happen silently - errors are logged but no dialog shown
    // This keeps the user experience simple with just one final status dialog from LibraryChecker

    return librariesSuccess && pythonSuccess;
}

void StartupChecker::downloadPythonComponents(const MissingComponents& missing)
{
    // This method is no longer used - Python is now downloaded in downloadAndSetupComponents
    // Kept for compatibility but does nothing
}