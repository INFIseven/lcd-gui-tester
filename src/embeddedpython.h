#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QWidget>
#include <QDir>
#include <QTimer>

class EmbeddedPython : public QObject
{
    Q_OBJECT

public:
    explicit EmbeddedPython(QWidget* parent = nullptr);
    ~EmbeddedPython();
    
    struct PythonDistribution {
        QString url;
        QString filename;
        QString extractedFolder;
        QString executable;
    };
    
    bool isEmbeddedPythonAvailable();
    bool setupEmbeddedPython();
    QString getEmbeddedPythonPath();
    bool installPackage(const QString& packageName);
    bool runScript(const QString& scriptPath, const QStringList& arguments, QString& output, QString& error);
    
    // Platform-specific distributions
    static PythonDistribution getDistributionForPlatform();
    
private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    bool downloadPythonDistribution();
    bool extractPythonDistribution(const QString& zipPath);
    bool installPip();
    bool verifyInstallation();
    QString getPythonDirectory();
    QString getScriptsDirectory();
    
    QWidget* m_parent;
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply;
    QProgressDialog* m_progressDialog;
    QProcess* m_currentProcess;
    QString m_tempFilePath;
    QString m_currentOperation;
    bool m_setupComplete;
    
    // Python distribution URLs
    static const QString PYTHON_WINDOWS_X64_URL;
    static const QString PYTHON_WINDOWS_X86_URL;
    static const QString PYTHON_LINUX_X64_URL;
    static const QString PYTHON_MACOS_X64_URL;
    static const QString PYTHON_MACOS_ARM64_URL;
};