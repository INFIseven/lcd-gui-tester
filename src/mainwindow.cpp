#include "mainwindow.h"
#include "imagedropwidget.h"
#include "imagepreviewwidget.h"
#include "lvglscriptrunner.h"
#include "startupchecker.h"
#include <QApplication>
#include <QFileInfo>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_centralWidget(nullptr), m_dropWidget(nullptr),
      m_counterLabel(nullptr), m_scrollArea(nullptr), m_imagesWidget(nullptr),
      m_imagesLayout(nullptr), m_flashButton(nullptr),
      m_startupChecker(nullptr), m_scriptRunner(nullptr) {
  QString title = "LCD GUI Tester";
#ifdef APP_VERSION
  title += QString(" - %1").arg(APP_VERSION);
#endif
  setWindowTitle(title);
  setGeometry(100, 100, 800, 600);

  // Initialize components
  m_startupChecker = new StartupChecker(this);
  m_scriptRunner = new LVGLScriptRunner(this);

  // Perform comprehensive startup check
  if (!m_startupChecker->performStartupCheck()) {
    // User declined or setup failed - show warning but continue
    QMessageBox::warning(this, "Setup Incomplete",
                         "Some components are missing. Image processing "
                         "functionality may not work.");
  }

  setupUI();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
  m_centralWidget = new QWidget;
  setCentralWidget(m_centralWidget);

  auto mainLayout = new QVBoxLayout(m_centralWidget);

  // Title
  auto titleLabel = new QLabel("LCD GUI Tester");
  titleLabel->setStyleSheet(
      "font-size: 18px; font-weight: bold; margin: 10px;");
  titleLabel->setAlignment(Qt::AlignCenter);
  mainLayout->addWidget(titleLabel);

  // Drop area
  m_dropWidget = new ImageDropWidget(this);
  mainLayout->addWidget(m_dropWidget);

  // Image counter
  m_counterLabel = new QLabel(QString("Images: 0/%1").arg(MAX_IMAGES));
  m_counterLabel->setStyleSheet("font-weight: bold; margin: 10px;");
  mainLayout->addWidget(m_counterLabel);

  // Scroll area for images
  m_scrollArea = new QScrollArea;
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  m_imagesWidget = new QWidget;
  m_imagesLayout = new QGridLayout(m_imagesWidget);
  m_scrollArea->setWidget(m_imagesWidget);

  mainLayout->addWidget(m_scrollArea);

  // Flash button
  m_flashButton = new QPushButton("UPLOAD");
  m_flashButton->setStyleSheet("QPushButton {"
                               "  background-color: #0078d4;"
                               "  color: white;"
                               "  border: none;"
                               "  border-radius: 5px;"
                               "  padding: 10px;"
                               "  font-size: 14px;"
                               "  font-weight: bold;"
                               "}"
                               "QPushButton:hover {"
                               "  background-color: #106ebe;"
                               "}"
                               "QPushButton:disabled {"
                               "  background-color: #ccc;"
                               "}");
  m_flashButton->setEnabled(false);
  connect(m_flashButton, &QPushButton::clicked, this, &MainWindow::flashImages);
  mainLayout->addWidget(m_flashButton);
}

void MainWindow::addImage(const QString &imagePath) {
  if (m_images.size() >= MAX_IMAGES) {
    QMessageBox::warning(this, "Error",
                         QString("Maximum %1 images allowed!").arg(MAX_IMAGES));
    return;
  }

  if (!validateImageSize(imagePath)) {
    return;
  }

  // Check if image already exists
  for (const ImageInfo &img : m_images) {
    if (img.path == imagePath) {
      QMessageBox::warning(this, "Duplicate", "This image is already added!");
      return;
    }
  }

  // Add image
  ImageInfo imageInfo;
  imageInfo.path = imagePath;
  imageInfo.index = m_images.size();
  m_images.append(imageInfo);

  updateUI();
}

void MainWindow::removeImage(int index) {
  if (index >= 0 && index < m_images.size()) {
    m_images.removeAt(index);

    // Update indices
    for (int i = 0; i < m_images.size(); ++i) {
      m_images[i].index = i;
    }

    updateUI();
  }
}

void MainWindow::updateUI() {
  // Clear current layout
  QLayoutItem *child;
  while ((child = m_imagesLayout->takeAt(0)) != nullptr) {
    delete child->widget();
    delete child;
  }

  // Add image previews
  for (int i = 0; i < m_images.size(); ++i) {
    const ImageInfo &imageInfo = m_images[i];
    auto preview = new ImagePreviewWidget(imageInfo.path, i, this);
    int row = i / 4;
    int col = i % 4;
    m_imagesLayout->addWidget(preview, row, col);
  }

  // Update counter
  m_counterLabel->setText(
      QString("Images: %1/%2").arg(m_images.size()).arg(MAX_IMAGES));

  // Enable/disable flash button
  m_flashButton->setEnabled(!m_images.isEmpty());
}

bool MainWindow::validateImageSize(const QString &imagePath) {
  QPixmap pixmap(imagePath);
  if (pixmap.isNull()) {
    QMessageBox::critical(this, "Error",
                          QString("Failed to load image: %1").arg(imagePath));
    return false;
  }

  int width = pixmap.width();
  int height = pixmap.height();

  if (width != REQUIRED_WIDTH || height != REQUIRED_HEIGHT) {
    QMessageBox::critical(this, "Invalid Image Size",
                          QString("Image must be exactly %1x%2 pixels!\n"
                                  "Current image is %3x%4 pixels.")
                              .arg(REQUIRED_WIDTH)
                              .arg(REQUIRED_HEIGHT)
                              .arg(width)
                              .arg(height));
    return false;
  }

  return true;
}

void MainWindow::flashImages() {
  if (m_images.isEmpty()) {
    return;
  }

  // Prepare image paths
  QStringList imagePaths;
  for (const ImageInfo &imageInfo : m_images) {
    imagePaths.append(imageInfo.path);
  }

  // Create output directory
  QString appDir = QApplication::applicationDirPath();
  QString outputDir = appDir + "/generated";

  // Process images with embedded Python and LVGL script
  m_scriptRunner->processImages(imagePaths, outputDir);
}
