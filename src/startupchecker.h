#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

class LibraryChecker;
class EmbeddedPython;

class StartupChecker : public QObject
{
    Q_OBJECT

public:
    explicit StartupChecker(QWidget* parent = nullptr);
    ~StartupChecker();
    
    struct MissingComponents {
        bool needsLVGL = false;
        bool needsPython = false;
        bool needsPythonPackages = false;
        bool needsNrf52Sdk = false;
        bool needsArmGnuToolchain = false;
        bool needsNrf52Firmware = false;
        bool needsCMake = false;
        QStringList missingPackages;

        bool hasAnyMissing() const {
            return needsLVGL || needsPython || needsPythonPackages ||
                   needsNrf52Sdk || needsArmGnuToolchain || needsNrf52Firmware || needsCMake;
        }
    };
    
    // Main startup check - returns true if everything is ready
    bool performStartupCheck();
    
private:
    MissingComponents checkAllComponents();
    bool requestUserPermission(const MissingComponents& missing);
    bool downloadAndSetupComponents(const MissingComponents& missing);
    void downloadPythonComponents(const MissingComponents& missing);

    QWidget* m_parent;
    LibraryChecker* m_libraryChecker;
    EmbeddedPython* m_embeddedPython;
};