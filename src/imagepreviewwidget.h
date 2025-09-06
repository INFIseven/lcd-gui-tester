#pragma once

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPixmap>
#include <QFileInfo>

class MainWindow;

class ImagePreviewWidget : public QFrame
{
    Q_OBJECT

public:
    ImagePreviewWidget(const QString& imagePath, int index, MainWindow* parent = nullptr);

private slots:
    void removeImage();

private:
    QString m_imagePath;
    int m_index;
    MainWindow* m_parentWindow;
    
    QLabel* m_imageLabel;
    QLabel* m_infoLabel;
    QPushButton* m_removeButton;
};