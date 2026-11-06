// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#ifndef APPSTYLE_H
#define APPSTYLE_H
#include <QApplication>
#include <QDir>
#include <QFontDatabase>

inline void initializeAppearance()
{
    QApplication::setStyle("Fusion");
#ifdef Q_OS_WIN
    // Offscreen Qt lacks the Windows font-service integration used by the desktop backend.
    // Read installed fonts explicitly for reliable automated screenshots; no fonts are redistributed.
    if (QApplication::platformName() == "offscreen") {
        const QDir fonts(qEnvironmentVariable("WINDIR", "C:/Windows") + "/Fonts");
        QFontDatabase::addApplicationFont(fonts.filePath("msjh.ttc"));
        QFontDatabase::addApplicationFont(fonts.filePath("msjhbd.ttc"));
        QFontDatabase::addApplicationFont(fonts.filePath("segoeui.ttf"));
    }
    QApplication::setFont(QFont("Microsoft JhengHei", 9));
#endif
}
#endif
