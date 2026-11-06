// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#include "mainwindow.h"
#include "appstyle.h"
#include "voronoicanvas.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>

class WorkflowTest : public QObject {
    Q_OBJECT
private slots:
    void mouseInputAndResume()
    {
        MainWindow window; window.show(); QTest::qWait(30);
        auto *canvas = window.findChild<VoronoiCanvas *>("voronoiCanvas");
        QVERIFY(canvas); QVERIFY(canvas->boardRect().width() >= 600);
        const auto board = canvas->boardRect();
        auto position = [&](int x, int y) {
            return QPoint(qRound(board.left() + x * board.width() / 600), qRound(board.top() + y * board.height() / 600));
        };
        for (auto p : {vd::Point{100,100}, {200,100}, {100,200}, {200,200}})
            QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, position(p.x, p.y));
        QCOMPARE(window.engine().points().size(), std::size_t(4));
        QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, position(100, 100));
        QCOMPARE(window.engine().points().size(), std::size_t(4));
        QTest::mouseClick(window.findChild<QPushButton *>("stepButton"), Qt::LeftButton);
        QCOMPARE(window.engine().frame().phase, vd::Phase::BeforeMerge);
        QCOMPARE(window.engine().completedMerges(), 0);
        QTest::mouseClick(window.findChild<QPushButton *>("runButton"), Qt::LeftButton);
        QVERIFY(window.engine().complete()); QCOMPARE(window.engine().frame().resultEdges.size(), std::size_t(4));
        QVERIFY(window.findChild<QPushButton *>("saveButton")->isEnabled());
        QTest::mouseClick(canvas, Qt::RightButton, Qt::NoModifier, position(100, 100));
        QCOMPARE(window.engine().points().size(), std::size_t(3)); QVERIFY(!window.engine().complete());
    }

    void filesAndDatasets()
    {
        MainWindow window; QTemporaryDir temporary; QVERIFY(temporary.isValid()); QString error;
        const QString root = QStringLiteral(PROJECT_SOURCE_DIR);
        QVERIFY2(window.loadFile(root + "/examples/regression.in", &error), qPrintable(error));
        QCOMPARE(window.engine().points().size(), std::size_t(1));
        window.runAlgorithm(); QVERIFY(window.engine().complete());
        window.nextDataset(); QCOMPARE(window.engine().points().size(), std::size_t(2));
        window.nextDataset(); QCOMPARE(window.engine().points().size(), std::size_t(4));
        window.stepAlgorithm(); window.runAlgorithm();
        const auto saved = temporary.filePath(QStringLiteral("菱形結果.txt"));
        QVERIFY2(window.saveResult(saved, &error), qPrintable(error));
        QFile actual(saved), expected(root + "/examples/pdf-example.txt");
        QVERIFY(actual.open(QIODevice::ReadOnly)); QVERIFY(expected.open(QIODevice::ReadOnly));
        QCOMPARE(actual.readAll(), expected.readAll());
        QVERIFY(window.loadFile(saved, &error)); QVERIFY(window.importedResult());
        QVERIFY(!window.findChild<QPushButton *>("runButton")->isEnabled());
        window.resetAlgorithm(); QVERIFY(!window.importedResult());
        window.runAlgorithm(); QVERIFY(window.engine().complete());
        QVERIFY(window.saveInput(temporary.filePath("saved.in"), &error));
        QVERIFY(window.loadFile(root + "/examples/regression.in", &error));
        for (int i = 0; i < 7; ++i) { window.runAlgorithm(); window.nextDataset(); }
        QVERIFY(window.statusBar()->currentMessage().contains(QStringLiteral("檔案測試停止")));
        QVERIFY(!window.findChild<QPushButton *>("nextButton")->isEnabled());
        window.previousDataset(); QVERIFY(window.findChild<QPushButton *>("nextButton")->isEnabled());
        const auto before = window.engine().points();
        QVERIFY(!window.loadFile(temporary.filePath("missing.in"), &error));
        QVERIFY(window.engine().points() == before); QVERIFY(!error.isEmpty());

        // A deliberately non-Voronoi E must be displayed verbatim, not silently recomputed.
        const auto importedPath = temporary.filePath("custom.txt"); QFile custom(importedPath);
        QVERIFY(custom.open(QIODevice::WriteOnly)); custom.write("P 100 100\nP 200 100\nE 10 10 20 20\n"); custom.close();
        QVERIFY(window.loadFile(importedPath, &error)); QVERIFY(window.importedResult());
        const auto copyPath = temporary.filePath("copy.txt"); QVERIFY(window.saveResult(copyPath, &error));
        QFile copy(copyPath); QVERIFY(copy.open(QIODevice::ReadOnly));
        QCOMPARE(copy.readAll(), QByteArray("P 100 100\nP 200 100\nE 10 10 20 20\n"));
    }

    void screenshots()
    {
        const QString qa = QStringLiteral(PROJECT_SOURCE_DIR) + "/build/qa"; QVERIFY(QDir().mkpath(qa));
        MainWindow window; window.show(); QTest::qWait(30);
        QVERIFY(window.grab().save(qa + "/01-empty.png"));
        window.loadPoints({{80,110},{110,310},{70,500},{220,190},{230,410},{295,70},
                           {350,520},{370,290},{430,100},{510,210},{490,430},{570,550}}, QStringLiteral("12 點分治範例"));
        QVERIFY(window.grab().save(qa + "/02-input.png"));
        do { window.stepAlgorithm(); }
        while (!window.engine().complete() && !(window.engine().frame().phase == vd::Phase::BeforeMerge
                && window.engine().frame().leftCount + window.engine().frame().rightCount == 12));
        QCOMPARE(window.engine().frame().phase, vd::Phase::BeforeMerge);
        QVERIFY(window.grab().save(qa + "/03-before-merge.png"));
        window.stepAlgorithm(); QCOMPARE(window.engine().frame().phase, vd::Phase::Merged);
        QVERIFY(window.grab().save(qa + "/04-merge-chain.png"));
        window.runAlgorithm(); QVERIFY(window.grab().save(qa + "/05-complete.png"));
    }
    void smallDisplay()
    {
        MainWindow window; window.resize(1024, 700); window.show(); QTest::qWait(20);
        auto *workspace = window.findChild<QScrollArea *>("workspaceScroll");
        auto *canvas = window.findChild<VoronoiCanvas *>("voronoiCanvas");
        QVERIFY(workspace); QVERIFY(canvas);
        QVERIFY(window.height() <= 700); QVERIFY(canvas->boardRect().width() >= 600);
        QVERIFY(workspace->widget()->height() > workspace->viewport()->height());
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv); initializeAppearance();
    WorkflowTest test; return QTest::qExec(&test, argc, argv);
}
#include "ui_tests.moc"
