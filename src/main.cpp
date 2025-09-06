#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("LCD GUI Tester");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("INFI7 d.o.o.");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}