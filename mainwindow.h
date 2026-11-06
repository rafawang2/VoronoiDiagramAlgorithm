// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "dataio.h"
#include <QMainWindow>
#include <QString>

class QLabel;
class QPushButton;
class QProgressBar;
class QTableWidget;
class QListWidget;
class QSpinBox;
class QTimer;
class VoronoiCanvas;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void loadPoints(std::vector<vd::Point> points, const QString &source = QString());
    bool loadFile(const QString &path, QString *error = nullptr);
    bool saveResult(const QString &path, QString *error = nullptr) const;
    bool saveInput(const QString &path, QString *error = nullptr) const;
    const vd::VoronoiEngine &engine() const { return engine_; }
    bool importedResult() const { return imported_; }
public slots:
    void runAlgorithm();
    void stepAlgorithm();
    void resetAlgorithm();
    void nextDataset();
    void previousDataset();
private:
    vd::VoronoiEngine engine_;
    std::vector<vd::Point> input_;
    vd::Document document_;
    vd::Frame importedFrame_;
    int datasetIndex_ = -1;
    bool imported_ = false, fileEnded_ = false;
    QString source_;
    VoronoiCanvas *canvas_ = nullptr;
    QLabel *phaseTitle_ = nullptr, *phaseText_ = nullptr, *sourceLabel_ = nullptr;
    QLabel *pointCount_ = nullptr, *edgeCount_ = nullptr, *timeLabel_ = nullptr;
    QLabel *datasetLabel_ = nullptr, *coordinateLabel_ = nullptr, *stateBadge_ = nullptr;
    QPushButton *runButton_ = nullptr, *stepButton_ = nullptr, *saveButton_ = nullptr;
    QPushButton *nextButton_ = nullptr, *previousButton_ = nullptr, *playButton_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QTableWidget *pointTable_ = nullptr;
    QListWidget *history_ = nullptr;
    QSpinBox *xInput_ = nullptr, *yInput_ = nullptr, *randomCount_ = nullptr;
    QTimer *playTimer_ = nullptr;
    void buildUi();
    void refresh(bool recordStep = false);
    void refreshTable();
    void setInput(std::vector<vd::Point> points);
    void showDataset(int index);
    void stopPlayback();
    void addPoint(int x, int y);
    void removePoint(int x, int y);
    void chooseFile();
    void chooseSave(bool input);
};
#endif // MAINWINDOW_H
