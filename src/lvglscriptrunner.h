#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

class EmbeddedPython;

class LVGLScriptRunner : public QObject {
  Q_OBJECT

public:
  explicit LVGLScriptRunner(QWidget *parent = nullptr);
  ~LVGLScriptRunner();

  bool processImages(const QStringList &imagePaths, const QString &outputDir);

private:
  QString getLibrariesPath();
  QString getLVGLScriptPath();
  bool ensurePythonReady();
  bool configureAndBuildMCU();
  bool flashFirmware();

  QWidget *m_parent;
  EmbeddedPython *m_embeddedPython;
};