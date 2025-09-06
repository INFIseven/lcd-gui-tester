#pragma once

#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

class MainWindow;

class ImageDropWidget : public QLabel
{
    Q_OBJECT

public:
    ImageDropWidget(MainWindow* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    MainWindow* m_parentWindow;
};