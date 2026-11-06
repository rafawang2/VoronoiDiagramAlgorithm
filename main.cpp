// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#include "mainwindow.h"
#include "appstyle.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("Voronoi Studio");
    QApplication::setApplicationVersion("1.0");
    initializeAppearance();
    MainWindow w;
    const auto arguments = a.arguments();
    if (arguments.size() > 1) {
        QString error;
        if (!w.loadFile(arguments.at(1), &error))
            QMessageBox::warning(&w, QStringLiteral("讀取失敗"), error);
    }
    w.show();
    return a.exec();
}
