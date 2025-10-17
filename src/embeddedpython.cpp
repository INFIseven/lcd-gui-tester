#include "embeddedpython.h"
#include <QApplication>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QEventLoop>
#include <QDebug>
#include <QStandardPaths>
#include <QTextStream>
#include <QSysInfo>
#include <QDateTime>
#include <QThread>

// Python distribution URLs (using Python 3.11 embedded)
const QString EmbeddedPython::PYTHON_WINDOWS_X64_URL = "https://www.python.org/ftp/python/3.11.9/python-3.11.9-embed-win32.zip";
const QString EmbeddedPython::PYTHON_WINDOWS_X86_URL = "https://www.python.org/ftp/python/3.11.9/python-3.11.9-embed-win32.zip";
const QString EmbeddedPython::PYTHON_LINUX_X64_URL = "https://github.com/indygreg/python-build-standalone/releases/download/20240415/cpython-3.11.9+20240415-x86_64-unknown-linux-gnu-install_only.tar.gz";
const QString EmbeddedPython::PYTHON_MACOS_X64_URL = "https://github.com/indygreg/python-build-standalone/releases/download/20240415/cpython-3.11.9+20240415-x86_64-apple-darwin-install_only.tar.gz";
const QString EmbeddedPython::PYTHON_MACOS_ARM64_URL = "https://github.com/indygreg/python-build-standalone/releases/download/20240415/cpython-3.11.9+20240415-aarch64-apple-darwin-install_only.tar.gz";

EmbeddedPython::EmbeddedPython(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_networkManager(nullptr)
    , m_currentReply(nullptr)
    , m_progressDialog(nullptr)
    , m_currentProcess(nullptr)
    , m_setupComplete(false)
{
    m_networkManager = new QNetworkAccessManager(this);
}

EmbeddedPython::~EmbeddedPython()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }
    
    if (m_currentProcess) {
        m_currentProcess->kill();
        m_currentProcess->deleteLater();
    }
    
    if (m_progressDialog) {
        m_progressDialog->deleteLater();
    }
}

EmbeddedPython::PythonDistribution EmbeddedPython::getDistributionForPlatform()
{
    PythonDistribution dist;
    
#ifdef Q_OS_WIN
    dist.url = PYTHON_WINDOWS_X64_URL;
    dist.filename = "python-embedded.zip";
    dist.extractedFolder = "python";
    dist.executable = "python.exe";
#elif defined(Q_OS_LINUX)
    dist.url = PYTHON_LINUX_X64_URL;
    dist.filename = "python-embedded.tar.gz";
    dist.extractedFolder = "python";
    dist.executable = "bin/python3";
#elif defined(Q_OS_MACOS)
    // Detect architecture on macOS
    QString architecture = QSysInfo::currentCpuArchitecture();
    qDebug() << "Detected macOS architecture:" << architecture;
    
    if (architecture == "arm64" || architecture == "aarch64") {
        // Apple Silicon (M1, M2, etc.)
        dist.url = PYTHON_MACOS_ARM64_URL;
        qDebug() << "Using ARM64 Python distribution for Apple Silicon";
    } else {
        // Intel macOS
        dist.url = PYTHON_MACOS_X64_URL;
        qDebug() << "Using x64 Python distribution for Intel Mac";
    }
    
    dist.filename = "python-embedded.tar.gz";
    dist.extractedFolder = "python";
    dist.executable = "bin/python3";
#else
    // Fallback - try Linux version
    dist.url = PYTHON_LINUX_X64_URL;
    dist.filename = "python-embedded.tar.gz";
    dist.extractedFolder = "python";
    dist.executable = "bin/python3";
#endif
    
    return dist;
}

QString EmbeddedPython::getPythonDirectory()
{
    QString appDir = QApplication::applicationDirPath();
    return appDir + "/libraries/python";
}

QString EmbeddedPython::getScriptsDirectory()
{
    QString pythonDir = getPythonDirectory();
#ifdef Q_OS_WIN
    return pythonDir + "/Scripts";
#else
    return pythonDir + "/bin";
#endif
}

QString EmbeddedPython::getEmbeddedPythonPath()
{
    QString pythonDir = getPythonDirectory();
    PythonDistribution dist = getDistributionForPlatform();
    return pythonDir + "/" + dist.executable;
}

bool EmbeddedPython::isEmbeddedPythonAvailable()
{
    QString pythonExe = getEmbeddedPythonPath();
    QFile pythonFile(pythonExe);
    
    if (!pythonFile.exists()) {
        return false;
    }
    
    // Test if it actually runs
    QProcess testProcess;
    testProcess.start(pythonExe, {"--version"});
    testProcess.waitForFinished(5000);
    
    return testProcess.exitCode() == 0;
}

bool EmbeddedPython::setupEmbeddedPython()
{
    if (isEmbeddedPythonAvailable() && m_setupComplete) {
        return true;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        m_parent,
        "Python Required",
        "Python is required to process images with LVGL.\n"
        "Would you like to download and install a portable Python distribution?\n\n"
        "This will download approximately 25MB and install Python locally within the application.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );
    
    if (reply != QMessageBox::Yes) {
        return false;
    }
    
    if (!downloadPythonDistribution()) {
        return false;
    }
    
    if (!installPip()) {
        qDebug() << "Warning: Could not install pip, but continuing...";
    }
    
    // Install required packages (based on LVGL prerequisites-pip.txt)
    QStringList requiredPackages = {"Pillow", "pypng", "lz4", "kconfiglib"};
    QStringList failedPackages;
    
    for (const QString& package : requiredPackages) {
        if (!installPackage(package)) {
            failedPackages.append(package);
        }
    }
    
    if (!failedPackages.isEmpty()) {
        QMessageBox::warning(m_parent, "Package Installation", 
                           QString("Failed to install some required packages: %1\n"
                                  "Image processing may not work correctly.")
                           .arg(failedPackages.join(", ")));
    }
    
    m_setupComplete = verifyInstallation();
    return m_setupComplete;
}

bool EmbeddedPython::downloadPythonDistribution()
{
    PythonDistribution dist = getDistributionForPlatform();
    
    // Create progress dialog
    m_progressDialog = new QProgressDialog(
        "Downloading Python distribution...",
        "Cancel",
        0, 100,
        m_parent
    );
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setAutoClose(true);
    m_progressDialog->setAutoReset(true);
    
    // Create temporary file path manually to avoid QTemporaryFile handle issues
    QString fileExtension = dist.filename.endsWith(".zip") ? ".zip" : ".tar.gz";
    m_tempFilePath = QDir::tempPath() + "/python_dist_" +
                     QString::number(QDateTime::currentMSecsSinceEpoch()) + fileExtension;

    qDebug() << "Will download Python to:" << m_tempFilePath;
    
    // Start download
    QNetworkRequest request(QUrl(dist.url));
    request.setHeader(QNetworkRequest::UserAgentHeader, "LCD-GUI-Tester/1.0");
    
    m_currentReply = m_networkManager->get(request);
    m_currentOperation = "download";
    
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &EmbeddedPython::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &EmbeddedPython::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred,
            this, &EmbeddedPython::onDownloadError);
    
    // Connect cancel button
    connect(m_progressDialog, &QProgressDialog::canceled, [this]() {
        if (m_currentReply) {
            m_currentReply->abort();
        }
    });
    
    m_progressDialog->show();
    
    // Wait for download to complete
    QEventLoop loop;
    connect(m_currentReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(m_progressDialog, &QProgressDialog::canceled, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (m_progressDialog->wasCanceled()) {
        return false;
    }
    
    bool downloadSuccess = (m_currentReply->error() == QNetworkReply::NoError);

    if (downloadSuccess) {
        // Read all data from reply first and delete it immediately
        QByteArray downloadedData = m_currentReply->readAll();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;

        // Save downloaded data in a scoped block
        {
            QFile file(m_tempFilePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(downloadedData);
                file.flush();
                file.close();
            } else {
                qDebug() << "Failed to save downloaded file";
                downloadSuccess = false;
            }
        }

        downloadedData.clear();

        if (downloadSuccess) {
            // Wait for Windows to release file handles
            qDebug() << "Waiting for file handles to be released...";
            QThread::msleep(1000);

            // Extract the distribution synchronously
            if (extractPythonDistribution(m_tempFilePath)) {
                qDebug() << "Python distribution extracted successfully";
            } else {
                qDebug() << "Failed to extract Python distribution";
                downloadSuccess = false;
            }

            // Clean up temporary file
            QFile::remove(m_tempFilePath);
        }
    } else {
        // Clean up the reply on error
        if (m_currentReply) {
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
        }
    }
    
    if (m_progressDialog) {
        m_progressDialog->close();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }
    
    return downloadSuccess;
}

bool EmbeddedPython::extractPythonDistribution(const QString& zipPath)
{
    QString pythonDir = getPythonDirectory();
    QDir().mkpath(pythonDir);

    // Extract the distribution
    QProcess extractProcess;
    QStringList arguments;

#ifdef Q_OS_WIN
    // Use .NET compression with short temp path to avoid path length issues
    QString tempExtractBase = "C:/Temp/python_extract_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QDir().mkpath(tempExtractBase);

    QString command = "powershell";
    arguments << "-NoProfile" << "-ExecutionPolicy" << "Bypass" << "-Command"
              << QString("Add-Type -AssemblyName System.IO.Compression.FileSystem; "
                        "[System.IO.Compression.ZipFile]::ExtractToDirectory('%1', '%2')")
                 .arg(zipPath).arg(tempExtractBase);

    extractProcess.start(command, arguments);
    extractProcess.waitForFinished(60000); // 60 second timeout

    QString stdError = extractProcess.readAllStandardError();
    bool hasErrors = stdError.contains("Exception", Qt::CaseInsensitive) ||
                     stdError.contains("Error", Qt::CaseInsensitive);

    if (extractProcess.exitCode() == 0 && !hasErrors) {
        // Move extracted files to final location
        QDir tempDir(tempExtractBase);
        QStringList entries = tempDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);

        qDebug() << "Extracted" << entries.size() << "items to temp location";

        // Move all files from temp to python directory
        for (const QString& entry : entries) {
            QString oldPath = tempDir.absoluteFilePath(entry);
            QString newPath = pythonDir + "/" + entry;

            // Remove if exists
            if (QFile::exists(newPath)) {
                if (QFileInfo(newPath).isDir()) {
                    QDir(newPath).removeRecursively();
                } else {
                    QFile::remove(newPath);
                }
            }

            bool moved = QDir().rename(oldPath, newPath);
            if (!moved) {
                qDebug() << "Failed to move:" << oldPath << "to" << newPath;
            }
        }

        // Clean up temp directory
        QDir(tempExtractBase).removeRecursively();

        qDebug() << "Python extracted to:" << pythonDir;
        return true;
    } else {
        qDebug() << "Extraction failed. Exit code:" << extractProcess.exitCode();
        qDebug() << "Error:" << stdError;
        QDir(tempExtractBase).removeRecursively();
        return false;
    }
#else
    // Use tar for Linux/macOS
    QString command = "tar";
    arguments << "-xzf" << zipPath << "-C" << pythonDir << "--strip-components=1";

    extractProcess.start(command, arguments);
    extractProcess.waitForFinished(60000); // 60 second timeout

    bool success = extractProcess.exitCode() == 0;
    if (!success) {
        QString error = extractProcess.readAllStandardError();
        qDebug() << "Extraction failed:" << error;
    }

    return success;
#endif
}

bool EmbeddedPython::installPip()
{
#ifdef Q_OS_WIN
    qDebug() << "Installing pip for Windows embedded Python...";

    // For Windows embedded Python, we need to enable pip
    QString pythonDir = getPythonDirectory();
    QString pthFile = pythonDir + "/python311._pth";

    qDebug() << "Python directory:" << pythonDir;
    qDebug() << "Looking for .pth file:" << pthFile;

    // Modify the .pth file to enable site-packages
    QFile file(pthFile);
    if (file.open(QIODevice::ReadOnly)) {
        QString content = file.readAll();
        file.close();

        qDebug() << "Found .pth file, original content:" << content;

        // Check if "import site" is already enabled (not commented out)
        // We need to uncomment it if it's currently "#import site"
        if (content.contains("#import site") && !content.contains("\nimport site\n")) {
            // Uncomment the line by replacing "#import site" with "import site"
            content.replace("#import site", "import site");

            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(content.toUtf8());
                file.close();
                qDebug() << "Updated .pth file to enable site-packages";
                qDebug() << "New content:" << content;
            } else {
                qDebug() << "Failed to write .pth file";
            }
        } else if (content.contains("\nimport site") || content.endsWith("import site")) {
            qDebug() << ".pth file already has import site enabled";
        } else {
            qDebug() << "WARNING: Unexpected .pth file format, adding import site anyway";
            content += "\nimport site\n";
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(content.toUtf8());
                file.close();
                qDebug() << "Added import site to .pth file";
            }
        }
    } else {
        qDebug() << "WARNING: Could not find python311._pth file. Pip installation may fail.";
    }

    // Download and install pip
    QString pythonExe = getEmbeddedPythonPath();
    QString getPipUrl = "https://bootstrap.pypa.io/get-pip.py";

    qDebug() << "Python executable:" << pythonExe;
    qDebug() << "Python exists:" << QFile::exists(pythonExe);
    qDebug() << "Downloading get-pip.py from:" << getPipUrl;

    // Download get-pip.py
    QNetworkRequest request{QUrl(getPipUrl)};
    QNetworkReply* reply = m_networkManager->get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "Successfully downloaded get-pip.py";

        QString getPipPath = pythonDir + "/get-pip.py";
        QFile getPipFile(getPipPath);
        if (getPipFile.open(QIODevice::WriteOnly)) {
            getPipFile.write(reply->readAll());
            getPipFile.close();
            qDebug() << "Saved get-pip.py to:" << getPipPath;

            // Run get-pip.py (without --user to install in embedded Python directory)
            qDebug() << "Running get-pip.py...";
            QProcess pipProcess;
            pipProcess.start(pythonExe, {getPipPath});
            pipProcess.waitForFinished(120000); // 2 minute timeout

            int exitCode = pipProcess.exitCode();
            QString stdOut = pipProcess.readAllStandardOutput();
            QString stdErr = pipProcess.readAllStandardError();

            qDebug() << "get-pip.py exit code:" << exitCode;
            qDebug() << "get-pip.py stdout:" << stdOut;
            qDebug() << "get-pip.py stderr:" << stdErr;

            if (exitCode == 0) {
                qDebug() << "Pip installed successfully";
            } else {
                qDebug() << "Pip installation failed with exit code:" << exitCode;
            }

            return exitCode == 0;
        } else {
            qDebug() << "Failed to save get-pip.py to:" << getPipPath;
        }
    }
    
    reply->deleteLater();
    return false;
#else
    // For Linux/macOS, pip should be included
    return true;
#endif
}

bool EmbeddedPython::installPackage(const QString& packageName)
{
    QString pythonExe = getEmbeddedPythonPath();
    
    m_progressDialog = new QProgressDialog(
        QString("Installing Python package: %1...").arg(packageName),
        "Cancel",
        0, 0,
        m_parent
    );
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->show();
    
    QStringList arguments;
    // Install packages directly in embedded Python (no --user flag)
    arguments << "-m" << "pip" << "install" << packageName;
    
    qDebug() << "Installing package:" << packageName << "with command:" << pythonExe << arguments.join(" ");
    
    m_currentProcess = new QProcess(this);
    connect(m_currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &EmbeddedPython::onProcessFinished);
    
    m_currentProcess->start(pythonExe, arguments);
    
    QEventLoop loop;
    connect(m_currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            &loop, &QEventLoop::quit);
    connect(m_progressDialog, &QProgressDialog::canceled, &loop, &QEventLoop::quit);
    
    loop.exec();
    
    if (m_progressDialog) {
        m_progressDialog->close();
        m_progressDialog->deleteLater();
        m_progressDialog = nullptr;
    }
    
    bool success = m_currentProcess->exitCode() == 0;
    
    if (!success) {
        QString error = m_currentProcess->readAllStandardError();
        QString output = m_currentProcess->readAllStandardOutput();
        qDebug() << "Package installation failed for" << packageName;
        qDebug() << "Exit code:" << m_currentProcess->exitCode();
        qDebug() << "Error:" << error;
        qDebug() << "Output:" << output;
    } else {
        qDebug() << "Successfully installed:" << packageName;
    }
    
    m_currentProcess->deleteLater();
    m_currentProcess = nullptr;
    
    return success;
}

bool EmbeddedPython::runScript(const QString& scriptPath, const QStringList& arguments, QString& output, QString& error)
{
    QString pythonExe = getEmbeddedPythonPath();
    
    QStringList fullArgs;
    fullArgs << scriptPath << arguments;
    
    qDebug() << "Executing Python script:";
    qDebug() << "  Python executable:" << pythonExe;
    qDebug() << "  Script path:" << scriptPath;
    qDebug() << "  Arguments:" << arguments;
    qDebug() << "  Full command:" << pythonExe << fullArgs.join(" ");
    
    QProcess process;
    process.start(pythonExe, fullArgs);
    process.waitForFinished(30000); // 30 second timeout
    
    output = process.readAllStandardOutput();
    error = process.readAllStandardError();
    
    qDebug() << "Process exit code:" << process.exitCode();
    qDebug() << "Process stdout:" << output;
    qDebug() << "Process stderr:" << error;
    
    return process.exitCode() == 0;
}

bool EmbeddedPython::verifyInstallation()
{
    QString pythonExe = getEmbeddedPythonPath();
    
    // Test basic Python execution
    QProcess testProcess;
    testProcess.start(pythonExe, {"--version"});
    testProcess.waitForFinished(5000);
    
    if (testProcess.exitCode() != 0) {
        return false;
    }
    
    // Test required packages (matching LVGL script requirements)
    QStringList testCommands = {
        "import PIL.Image; print('Pillow OK')",
        "import png; print('pypng OK')",
        "import lz4.block; print('lz4 OK')",
        "import kconfiglib; print('kconfiglib OK')"
    };
    
    for (const QString& testCommand : testCommands) {
        testProcess.start(pythonExe, {"-c", testCommand});
        testProcess.waitForFinished(5000);
        
        if (testProcess.exitCode() != 0) {
            QString error = testProcess.readAllStandardError();
            qDebug() << "Package verification failed:" << testCommand;
            qDebug() << "Error:" << error;
            return false;
        }
    }
    
    return true;
}

void EmbeddedPython::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (m_progressDialog && bytesTotal > 0) {
        int percentage = static_cast<int>((bytesReceived * 100) / bytesTotal);
        m_progressDialog->setValue(percentage);
        
        QString sizeText = QString("Downloaded %1 MB of %2 MB")
                          .arg(bytesReceived / (1024 * 1024))
                          .arg(bytesTotal / (1024 * 1024));
        m_progressDialog->setLabelText("Downloading Python distribution...\n" + sizeText);
    }
}

void EmbeddedPython::onDownloadFinished()
{
    // This method is now unused since we handle everything synchronously
    // in downloadPythonDistribution(), but kept for compatibility
    if (m_progressDialog) {
        m_progressDialog->setValue(100);
    }
}

void EmbeddedPython::onDownloadError(QNetworkReply::NetworkError error)
{
    QString errorMsg = QString("Download failed: %1").arg(m_currentReply->errorString());
    QMessageBox::critical(m_parent, "Download Error", errorMsg);
    
    if (m_progressDialog) {
        m_progressDialog->close();
    }
}

void EmbeddedPython::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    qDebug() << "Process finished with exit code:" << exitCode;
    if (exitCode != 0 && m_currentProcess) {
        QString error = m_currentProcess->readAllStandardError();
        qDebug() << "Process error:" << error;
    }
}