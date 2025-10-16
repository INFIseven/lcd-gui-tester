#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QFutureWatcher>

class EmbeddedPython;

class LVGLScriptRunner : public QObject {
  Q_OBJECT

public:
  explicit LVGLScriptRunner(QWidget *parent = nullptr);
  ~LVGLScriptRunner();

  void processImagesAsync(const QStringList &imagePaths, const QString &outputDir);

signals:
  void processingCompleted(bool success, const QString &message);
  void processingProgress(const QString &status);

private:
  bool processImages(const QStringList &imagePaths, const QString &outputDir);
  QString getLibrariesPath();
  QString getLVGLScriptPath();
  bool ensurePythonReady();
  bool configureAndBuildMCU();
  bool flashFirmware();

  void onProcessingFinished();

  QWidget *m_parent;
  EmbeddedPython *m_embeddedPython;
  QFutureWatcher<bool> *m_futureWatcher;
};