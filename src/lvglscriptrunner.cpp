#include "lvglscriptrunner.h"
#include "embeddedpython.h"
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProgressDialog>
#include <QTextStream>
#include <QDebug>

LVGLScriptRunner::LVGLScriptRunner(QWidget* parent)
    : QObject(parent)
    , m_parent(parent)
    , m_embeddedPython(nullptr)
{
    m_embeddedPython = new EmbeddedPython(parent);
}

LVGLScriptRunner::~LVGLScriptRunner()
{
    if (m_embeddedPython) {
        m_embeddedPython->deleteLater();
    }
}

QString LVGLScriptRunner::getLibrariesPath()
{
    QString appDir = QApplication::applicationDirPath();
    return appDir + "/libraries";
}

QString LVGLScriptRunner::getLVGLScriptPath()
{
    return getLibrariesPath() + "/lvgl/scripts/LVGLImage.py";
}

bool LVGLScriptRunner::ensurePythonReady()
{
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

bool LVGLScriptRunner::processImages(const QStringList& imagePaths, const QString& outputDir)
{
    if (imagePaths.isEmpty()) {
        return false;
    }
    
    // Check if LVGL script exists
    QString scriptPath = getLVGLScriptPath();
    if (!QFile::exists(scriptPath)) {
        QMessageBox::critical(m_parent, "Script Missing", 
                            "LVGL image script not found. Please ensure LVGL library is properly installed.");
        return false;
    }
    
    // Ensure Python is ready
    if (!ensurePythonReady()) {
        return false;
    }
    
    // Create output directory
    QDir().mkpath(outputDir);
    
    // Create progress dialog
    QProgressDialog progressDialog("Processing images with LVGL...", "Cancel", 0, imagePaths.size(), m_parent);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.show();
    
    QStringList processedFiles;
    QStringList headerDeclarations;
    QStringList arrayNames;
    
    for (int i = 0; i < imagePaths.size(); ++i) {
        if (progressDialog.wasCanceled()) {
            break;
        }
        
        const QString& imagePath = imagePaths[i];
        QFileInfo imageInfo(imagePath);
        QString baseName = imageInfo.baseName();
        QString outputFile = QString("%1/%2.c").arg(outputDir, baseName);
        
        progressDialog.setLabelText(QString("Processing %1 (%2 of %3)...")
                                   .arg(imageInfo.fileName())
                                   .arg(i + 1)
                                   .arg(imagePaths.size()));
        progressDialog.setValue(i);
        
        QApplication::processEvents();
        
        // Run LVGL script with correct arguments
        QStringList arguments;
        arguments << imagePath;
        arguments << "--output" << outputFile;
        arguments << "--ofmt" << "C";
        arguments << "--cf" << "RGB565";  // Avoid pngquant dependency
        arguments << "--name" << baseName;
        
        QString output, error;
        qDebug() << "Running LVGL script with arguments:" << arguments;
        bool success = m_embeddedPython->runScript(scriptPath, arguments, output, error);
        
        if (success) {
            processedFiles.append(outputFile);
            arrayNames.append(baseName);
            headerDeclarations.append(QString("extern const lv_img_dsc_t %1;").arg(baseName));
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
    
    // Create combined header file
    QString headerPath = outputDir + "/generated_images.h";
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
        for (const QString& declaration : headerDeclarations) {
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
    
    // Create implementation file for image array
    QString implPath = outputDir + "/generated_images.c";
    QFile implFile(implPath);
    if (implFile.open(QIODevice::WriteOnly)) {
        QTextStream stream(&implFile);
        
        stream << "#include \"generated_images.h\"\n\n";
        
        // Include all generated C files
        for (int i = 0; i < arrayNames.size(); ++i) {
            QFileInfo cFileInfo(processedFiles[i]);
            stream << QString("#include \"%1\"\n").arg(cFileInfo.fileName());
        }
        
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
    
    QMessageBox::information(m_parent, "Processing Complete", 
                           QString("Successfully processed %1 images for LVGL.\n"
                                  "Generated files:\n"
                                  "• %2 C files\n"
                                  "• generated_images.h\n"
                                  "• generated_images.c\n\n"
                                  "Files are ready for LCD display.")
                           .arg(processedFiles.size())
                           .arg(processedFiles.size()));
    
    return true;
}