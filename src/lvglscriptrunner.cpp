#include "lvglscriptrunner.h"
#include "embeddedpython.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QTextStream>

LVGLScriptRunner::LVGLScriptRunner(QWidget *parent)
    : QObject(parent), m_parent(parent), m_embeddedPython(nullptr) {
  m_embeddedPython = new EmbeddedPython(parent);
}

LVGLScriptRunner::~LVGLScriptRunner() {
  if (m_embeddedPython) {
    m_embeddedPython->deleteLater();
  }
}

QString LVGLScriptRunner::getLibrariesPath() {
  QString appDir = QApplication::applicationDirPath();
  return appDir + "/libraries";
}

QString LVGLScriptRunner::getLVGLScriptPath() {
  return getLibrariesPath() + "/lvgl/scripts/LVGLImage.py";
}

bool LVGLScriptRunner::ensurePythonReady() {
  // Python setup is now handled at application startup
  // Just verify it's available
  if (!m_embeddedPython->isEmbeddedPythonAvailable()) {
    qDebug() << "Embedded Python not available - startup check may have failed";
    return false;
  }

  QString pythonPath = m_embeddedPython->getEmbeddedPythonPath();
  qDebug() << "Using embedded Python at:" << pythonPath;

  return true;
}

bool LVGLScriptRunner::processImages(const QStringList &imagePaths,
                                     const QString &outputDir) {
  if (imagePaths.isEmpty()) {
    return false;
  }

  // Check if LVGL script exists
  QString scriptPath = getLVGLScriptPath();
  if (!QFile::exists(scriptPath)) {
    QMessageBox::critical(m_parent, "Script Missing",
                          "LVGL image script not found. Please ensure LVGL "
                          "library is properly installed.");
    return false;
  }

  // Ensure Python is ready
  if (!ensurePythonReady()) {
    return false;
  }

  // Ensure output directory exists and use QDir for proper path handling
  QDir generatedDir(outputDir);
  if (!generatedDir.exists()) {
    generatedDir.mkpath(".");
  }
  QString absoluteOutputDir = generatedDir.absolutePath();

  // Create progress dialog
  QProgressDialog progressDialog("Processing images with LVGL...", "Cancel", 0,
                                 imagePaths.size(), m_parent);
  progressDialog.setWindowModality(Qt::WindowModal);
  progressDialog.show();

  QStringList processedFiles;
  QStringList headerDeclarations;
  QStringList arrayNames;

  for (int i = 0; i < imagePaths.size(); ++i) {
    if (progressDialog.wasCanceled()) {
      break;
    }

    const QString &imagePath = imagePaths[i];
    QFileInfo imageInfo(imagePath);
    QString baseName = imageInfo.baseName();

    // Sanitize baseName: replace any character that is not A-Z, a-z, 0-9, or _
    // with _
    for (int j = 0; j < baseName.length(); ++j) {
      QChar c = baseName[j];
      if (!c.isLetterOrNumber() && c != '_') {
        baseName[j] = '_';
      }
    }

    QString outputFile = generatedDir.filePath(baseName + ".c");

    progressDialog.setLabelText(QString("Processing %1 (%2 of %3)...")
                                    .arg(imageInfo.fileName())
                                    .arg(i + 1)
                                    .arg(imagePaths.size()));
    progressDialog.setValue(i);

    QApplication::processEvents();

    // Run LVGL script with correct arguments
    // Note: --output expects a directory path, the script creates
    // {dir}/{name}.c
    QStringList arguments;
    arguments << imagePath;
    arguments << "--output" << absoluteOutputDir;
    arguments << "--ofmt" << "C";
    arguments << "--cf" << "RGB565"; // Avoid pngquant dependency
    arguments << "--name" << baseName;

    QString output, error;
    qDebug() << "Running LVGL script with arguments:" << arguments;
    bool success =
        m_embeddedPython->runScript(scriptPath, arguments, output, error);

    if (success) {
      processedFiles.append(outputFile);
      arrayNames.append(baseName);
      headerDeclarations.append(
          QString("extern const lv_img_dsc_t %1;").arg(baseName));
      qDebug() << "Successfully processed:" << imagePath;
      qDebug() << "Output:" << output;
    } else {
      qDebug() << "Failed to process:" << imagePath;
      qDebug() << "Error:" << error;
      qDebug() << "Output:" << output;

      QMessageBox::warning(m_parent, "Processing Failed",
                           QString("Failed to process image %1:\n%2")
                               .arg(imageInfo.fileName())
                               .arg(error.isEmpty() ? "Unknown error" : error));
    }
  }

  progressDialog.setValue(imagePaths.size());

  if (processedFiles.isEmpty()) {
    QMessageBox::warning(m_parent, "No Images Processed",
                         "No images were successfully processed.");
    return false;
  }

  // Create combined header file in the generated directory
  QString headerPath = generatedDir.filePath("generated_images.h");
  QFile headerFile(headerPath);
  if (headerFile.open(QIODevice::WriteOnly)) {
    QTextStream stream(&headerFile);

    // Header guard
    stream << "#pragma once\n\n";
    stream << "#ifdef __cplusplus\n";
    stream << "extern \"C\" {\n";
    stream << "#endif\n\n";
    stream << "#include \"lvgl.h\"\n\n";

    // Declarations
    for (const QString &declaration : headerDeclarations) {
      stream << declaration << "\n";
    }

    stream << "\n";
    stream << QString("#define IMAGE_COUNT %1\n").arg(arrayNames.size());
    stream << "extern const lv_img_dsc_t* images[IMAGE_COUNT];\n\n";

    stream << "#ifdef __cplusplus\n";
    stream << "}\n";
    stream << "#endif\n";

    headerFile.close();
  }

  // Create implementation file for image array in the generated directory
  QString implPath = generatedDir.filePath("generated_images.c");
  QFile implFile(implPath);
  if (implFile.open(QIODevice::WriteOnly)) {
    QTextStream stream(&implFile);

    stream << "#include \"generated_images.h\"\n\n";

    stream << "\n";
    stream << "const lv_img_dsc_t* images[IMAGE_COUNT] = {\n";
    for (int i = 0; i < arrayNames.size(); ++i) {
      stream << QString("    &%1").arg(arrayNames[i]);
      if (i < arrayNames.size() - 1) {
        stream << ",";
      }
      stream << "\n";
    }
    stream << "};\n";

    implFile.close();
  }

  // Automatically proceed to build and flash without confirmation dialogs
  if (!configureAndBuildMCU()) {
    QMessageBox::critical(m_parent, "Build Failed",
                          "Failed to configure and build the MCU firmware. "
                          "Check the console for details.");
    return false;
  }

  return true;
}

bool LVGLScriptRunner::configureAndBuildMCU() {
  QString appDir = QApplication::applicationDirPath();
  QString buildMcuDir = appDir + "/build_mcu";

  // Check if build_mcu directory exists
  if (!QDir(buildMcuDir).exists()) {
    qDebug() << "build_mcu directory not found at:" << buildMcuDir;
    return false;
  }

  // Determine the configure script based on OS
  QString configureScript;
#ifdef Q_OS_WIN
  configureScript = buildMcuDir + "/configure.bat";
#else
  configureScript = buildMcuDir + "/configure.sh";
#endif

  if (!QFile::exists(configureScript)) {
    qDebug() << "Configure script not found at:" << configureScript;
    return false;
  }

  qDebug() << "Running configure script:" << configureScript;

  // Create progress dialog
  QProgressDialog progressDialog("Configuring MCU build...", "Cancel", 0, 0,
                                 m_parent);
  progressDialog.setWindowModality(Qt::WindowModal);
  progressDialog.show();
  QApplication::processEvents();

  // Run the configure script
  QProcess configureProcess;
  configureProcess.setWorkingDirectory(buildMcuDir);

#ifdef Q_OS_WIN
  configureProcess.start("cmd.exe", QStringList() << "/c" << "configure.bat");
#else
  configureProcess.start("/bin/bash", QStringList() << "configure.sh");
#endif

  if (!configureProcess.waitForStarted()) {
    qDebug() << "Failed to start configure process";
    return false;
  }

  // Wait for both configure and build to complete (5 minutes timeout since
  // build is included)
  if (!configureProcess.waitForFinished(300000)) {
    qDebug() << "Configure and build process timed out";
    configureProcess.kill();
    return false;
  }

  QString output = configureProcess.readAllStandardOutput();
  QString error = configureProcess.readAllStandardError();

  qDebug() << "Configure and build output:" << output;
  if (!error.isEmpty()) {
    qDebug() << "Configure and build errors:" << error;
  }

  if (configureProcess.exitCode() != 0) {
    qDebug() << "Configure and build process failed with exit code:"
             << configureProcess.exitCode();
    return false;
  }

  progressDialog.close();

  // Build succeeded, automatically proceed to flash
  if (!flashFirmware()) {
    QMessageBox::critical(m_parent, "Flash Failed",
                          "Failed to flash the firmware. Make sure the "
                          "device is connected and nrfjprog is available.");
    return false;
  }

  return true;
}

bool LVGLScriptRunner::flashFirmware() {
  QString appDir = QApplication::applicationDirPath();
  QString buildMcuDir = appDir + "/build_mcu";
  QString hexFile = buildMcuDir + "/nrf52-lcd-tester-fw.hex";

  // Check if hex file exists
  if (!QFile::exists(hexFile)) {
    qDebug() << "Hex file not found at:" << hexFile;
    return false;
  }

  qDebug() << "Flashing firmware from:" << hexFile;

  // Create progress dialog
  QProgressDialog progressDialog("Flashing firmware to nRF52 device...",
                                 "Cancel", 0, 0, m_parent);
  progressDialog.setWindowModality(Qt::WindowModal);
  progressDialog.show();
  QApplication::processEvents();

  // Run nrfjprog to flash the firmware
  QProcess flashProcess;
  flashProcess.start("nrfjprog", QStringList()
                                     << "--program" << hexFile
                                     << "--chiperase"
                                     << "--reset"
                                     << "--verify");

  if (!flashProcess.waitForStarted()) {
    qDebug()
        << "Failed to start nrfjprog. Make sure it's installed and in PATH.";
    return false;
  }

  if (!flashProcess.waitForFinished(60000)) { // 60 second timeout
    qDebug() << "Flash process timed out";
    flashProcess.kill();
    return false;
  }

  QString output = flashProcess.readAllStandardOutput();
  QString error = flashProcess.readAllStandardError();

  qDebug() << "Flash output:" << output;
  if (!error.isEmpty()) {
    qDebug() << "Flash errors:" << error;
  }

  if (flashProcess.exitCode() != 0) {
    qDebug() << "Flash process failed with exit code:"
             << flashProcess.exitCode();
    return false;
  }

  progressDialog.close();

  QMessageBox::information(
      m_parent, "Flash Complete",
      "Firmware has been successfully flashed to the nRF52 device!");

  return true;
}
