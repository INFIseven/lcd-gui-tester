#include "imagepreviewwidget.h"
#include "mainwindow.h"
#include <QPixmap>
#include <QFileInfo>

ImagePreviewWidget::ImagePreviewWidget(const QString& imagePath, int index, MainWindow* parent)
    : QFrame(parent), m_imagePath(imagePath), m_index(index), m_parentWindow(parent)
{
    setFrameStyle(QFrame::Box);
    setStyleSheet("QFrame { border: 1px solid #ddd; border-radius: 5px; padding: 5px; }");
    
    auto layout = new QVBoxLayout(this);
    
    // Image label
    m_imageLabel = new QLabel;
    QPixmap pixmap(imagePath);
    QPixmap scaledPixmap = pixmap.scaled(185, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_imageLabel->setPixmap(scaledPixmap);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    
    // Info label
    QFileInfo fileInfo(imagePath);
    QString filename = fileInfo.fileName();
    QPixmap originalPixmap(imagePath);
    QString infoText = QString("%1\n%2x%3")
                      .arg(filename)
                      .arg(originalPixmap.width())
                      .arg(originalPixmap.height());
    
    m_infoLabel = new QLabel(infoText);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setStyleSheet("font-size: 10px; color: #666;");
    
    // Remove button
    m_removeButton = new QPushButton("Remove");
    m_removeButton->setStyleSheet(
        "QPushButton { background-color: #ff4444; color: white; border: none; "
        "border-radius: 3px; padding: 5px; }"
    );
    connect(m_removeButton, &QPushButton::clicked, this, &ImagePreviewWidget::removeImage);
    
    layout->addWidget(m_imageLabel);
    layout->addWidget(m_infoLabel);
    layout->addWidget(m_removeButton);
}

void ImagePreviewWidget::removeImage()
{
    if (m_parentWindow) {
        m_parentWindow->removeImage(m_index);
    }
}