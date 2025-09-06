#include "imagedropwidget.h"
#include "mainwindow.h"
#include <QStringList>
#include <QFileInfo>

ImageDropWidget::ImageDropWidget(MainWindow* parent)
    : QLabel(parent), m_parentWindow(parent)
{
    setAcceptDrops(true);
    setStyleSheet(
        "QLabel {"
        "  border: 2px dashed #aaa;"
        "  border-radius: 10px;"
        "  background-color: #f9f9f9;"
        "  color: #666;"
        "  font-size: 14px;"
        "  text-align: center;"
        "}"
        "QLabel:hover {"
        "  border-color: #0078d4;"
        "  background-color: #e6f3ff;"
        "}"
    );
    setText("Drag and drop images here\n(Max 10 images, 170x320 pixels only)");
    setAlignment(Qt::AlignCenter);
    setMinimumSize(400, 150);
}

void ImageDropWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        QStringList imageExtensions = {"png", "jpg", "jpeg", "bmp", "gif"};
        
        bool allValid = true;
        for (const QUrl& url : urls) {
            if (!url.isLocalFile()) {
                allValid = false;
                break;
            }
            
            QString filePath = url.toLocalFile();
            QFileInfo fileInfo(filePath);
            QString extension = fileInfo.suffix().toLower();
            
            if (!imageExtensions.contains(extension)) {
                allValid = false;
                break;
            }
        }
        
        if (allValid) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void ImageDropWidget::dropEvent(QDropEvent* event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    
    for (const QUrl& url : urls) {
        if (url.isLocalFile()) {
            QString filePath = url.toLocalFile();
            if (m_parentWindow) {
                m_parentWindow->addImage(filePath);
            }
        }
    }
}