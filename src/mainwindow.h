#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include <QFrame>
#include <QMessageBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QPixmap>
#include <QProcess>
#include <QVector>
#include <QFileInfo>
#include <QStatusBar>

class StartupChecker;
class LVGLScriptRunner;

class ImageDropWidget;
class ImagePreviewWidget;

struct ImageInfo {
    QString path;
    int index;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void addImage(const QString& imagePath);
    void removeImage(int index);

private slots:
    void flashImages();
    void onProcessingCompleted(bool success, const QString &message);
    void onProcessingProgress(const QString &status);

private:
    void setupUI();
    void updateUI();
    bool validateImageSize(const QString& imagePath);

    static constexpr int MAX_IMAGES = 10;
    static constexpr int REQUIRED_WIDTH = 170;
    static constexpr int REQUIRED_HEIGHT = 320;

    QWidget *m_centralWidget;
    ImageDropWidget *m_dropWidget;
    QLabel *m_counterLabel;
    QScrollArea *m_scrollArea;
    QWidget *m_imagesWidget;
    QGridLayout *m_imagesLayout;
    QPushButton *m_flashButton;

    QVector<ImageInfo> m_images;
    StartupChecker* m_startupChecker;
    LVGLScriptRunner* m_scriptRunner;
};