// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#include "mainwindow.h"
#include "voronoicanvas.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QScreen>
#include <QScrollArea>
#include <QShortcut>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace {
QLabel *label(const QString &text, const char *name)
{
    auto *result = new QLabel(text); result->setObjectName(name); return result;
}
QPushButton *button(const QString &text, const char *name)
{
    auto *result = new QPushButton(text); result->setObjectName(name);
    result->setCursor(Qt::PointingHandCursor); return result;
}
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Voronoi Studio | Divide & Conquer"));
    resize(1190, 960);
    buildUi(); refreshTable(); refresh();
    if (QApplication::platformName() != "offscreen" && screen()) {
        const auto available = screen()->availableGeometry();
        resize(std::min(1190, available.width() - 40), std::min(960, available.height() - 60));
    }
}

void MainWindow::buildUi()
{
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget#root { background: #f3f6fa; color: #243b53; }
        QWidget { font-size: 12px; }
        QFrame#header { background: #142d46; border-radius: 12px; }
        QLabel#brand { color: #f6f9fc; font-size: 25px; font-weight: 700; }
        QLabel#eyebrow { color: #8fb2d0; font-size: 10px; font-weight: 600; letter-spacing: 2px; }
        QLabel#subtitle { color: #acc3d9; font-size: 12px; }
        QLabel#stateBadge { color: #b1eadd; background: #24485a; padding: 7px 14px; border-radius: 12px; }
        QFrame#card { background: white; border: 1px solid #dce4ed; border-radius: 10px; }
        QLabel#section { color: #667f98; font-size: 10px; font-weight: 700; letter-spacing: 1px; }
        QLabel#phaseTitle { font-size: 18px; font-weight: 700; color: #243b53; }
        QLabel#phaseText { color: #6a7e94; font-size: 12px; }
        QLabel#metric { font-family: "Segoe UI"; font-size: 24px; font-weight: 600; color: #243b53; }
        QLabel#muted { color: #8192a5; font-size: 11px; }
        QLabel#sourceLabel { color: #667f98; }
        QPushButton { border: 1px solid #d5dfe9; border-radius: 6px; background: white; padding: 8px 12px; color: #35516e; }
        QPushButton:hover { background: #edf4fc; border-color: #a0bddb; }
        QPushButton:pressed { background: #dceaf8; }
        QPushButton:disabled { color: #a8b5c2; background: #f2f5f8; border-color: #e2e8f0; }
        QPushButton#runButton { background: #147d96; color: white; border-color: #147d96; font-weight: 700; }
        QPushButton#runButton:hover { background: #106e86; }
        QPushButton#runButton:disabled { background: #bad4da; border-color: #bad4da; color: white; }
        QPushButton#stepButton { color: #1d6dba; border-color: #a3c3e4; font-weight: 600; }
        QSpinBox, QComboBox { padding: 6px; border: 1px solid #d5dfe9; border-radius: 5px; background: white; color: #35516e; }
        QCheckBox { color: #667f98; spacing: 5px; }
        QProgressBar { border: none; background: #eaf0f6; border-radius: 3px; height: 6px; }
        QProgressBar::chunk { background: #1997a5; border-radius: 3px; }
        QTabWidget::pane { border: 1px solid #e0e7ef; border-radius: 5px; background: white; }
        QTabBar::tab { padding: 8px 18px; color: #8192a5; }
        QTabBar::tab:selected { color: #147d96; border-bottom: 2px solid #147d96; }
        QTableWidget, QListWidget { border: none; background: white; color: #4b627a; alternate-background-color: #f7f9fc; }
        QHeaderView::section { border: none; background: #f1f5f9; color: #8192a5; padding: 6px; }
        QStatusBar { color: #8192a5; background: #edf2f7; }
        QStatusBar::item { border: none; }
    )"));
    auto *root = new QWidget; root->setObjectName("root");
    // Preserve the 600-pixel canvas on short laptop displays by scrolling the workspace.
    auto *workspace = new QScrollArea;
    workspace->setObjectName("workspaceScroll"); workspace->setFrameShape(QFrame::NoFrame);
    workspace->setWidgetResizable(true); workspace->setWidget(root); setCentralWidget(workspace);
    setMinimumSize(980, 640);
    auto *main = new QVBoxLayout(root); main->setContentsMargins(18, 16, 18, 10); main->setSpacing(12);
    auto *header = new QFrame; header->setObjectName("header");
    auto *hl = new QHBoxLayout(header); hl->setContentsMargins(22, 15, 22, 15);
    auto *brand = new QVBoxLayout; brand->setSpacing(3);
    brand->addWidget(label("COMPUTATIONAL GEOMETRY / 01", "eyebrow"));
    brand->addWidget(label("Voronoi Studio", "brand"));
    brand->addWidget(label(QStringLiteral("分割、遞迴與合併，看見最近鄰區域如何形成。"), "subtitle"));
    hl->addLayout(brand); hl->addStretch();
    stateBadge_ = label(QStringLiteral("準備輸入"), "stateBadge"); hl->addWidget(stateBadge_);
    main->addWidget(header);
    auto *toolbar = new QHBoxLayout;
    auto *open = button(QStringLiteral("開啟檔案"), "openButton");
    saveButton_ = button(QStringLiteral("儲存結果"), "saveButton");
    auto *saveInput = button(QStringLiteral("儲存輸入"), "saveInputButton");
    auto *reset = button(QStringLiteral("重新演算"), "resetButton");
    auto *clear = button(QStringLiteral("清空"), "clearButton");
    toolbar->addWidget(open); toolbar->addWidget(saveButton_); toolbar->addWidget(saveInput);
    toolbar->addSpacing(8); toolbar->addWidget(reset); toolbar->addWidget(clear); toolbar->addStretch();
    sourceLabel_ = label(QStringLiteral("滑鼠輸入"), "sourceLabel"); sourceLabel_->setMaximumWidth(350);
    toolbar->addWidget(sourceLabel_); main->addLayout(toolbar);
    connect(open, &QPushButton::clicked, this, &MainWindow::chooseFile);
    connect(saveButton_, &QPushButton::clicked, this, [this] { chooseSave(false); });
    connect(saveInput, &QPushButton::clicked, this, [this] { chooseSave(true); });
    connect(reset, &QPushButton::clicked, this, &MainWindow::resetAlgorithm);
    connect(clear, &QPushButton::clicked, this, [this] { loadPoints({}); });
    auto *openKey = new QShortcut(QKeySequence::Open, this), *saveKey = new QShortcut(QKeySequence::Save, this);
    connect(openKey, &QShortcut::activated, this, &MainWindow::chooseFile);
    connect(saveKey, &QShortcut::activated, this, [this] { if (saveButton_->isEnabled()) chooseSave(false); });
    auto *body = new QHBoxLayout; body->setSpacing(16);
    auto *canvasColumn = new QVBoxLayout; canvasColumn->setSpacing(4);
    auto *heading = new QHBoxLayout;
    heading->addWidget(label("VORONOI DIAGRAM", "section")); heading->addStretch();
    heading->addWidget(label(QStringLiteral("600 × 600  ·  左鍵新增 / 右鍵刪除"), "muted"));
    canvasColumn->addLayout(heading); canvas_ = new VoronoiCanvas; canvasColumn->addWidget(canvas_, 1);
    auto *options = new QHBoxLayout;
    auto *grid = new QCheckBox(QStringLiteral("格線")); grid->setChecked(true);
    auto *hulls = new QCheckBox(QStringLiteral("凸包 / 切線")); hulls->setChecked(true);
    auto *chain = new QCheckBox(QStringLiteral("合併鏈")); chain->setChecked(true);
    auto *labels = new QCheckBox(QStringLiteral("座標標籤"));
    options->addWidget(grid); options->addWidget(hulls); options->addWidget(chain); options->addWidget(labels); options->addStretch();
    canvasColumn->addLayout(options);
    canvasColumn->addWidget(label(QStringLiteral("<span style='color:#2475c7'>● 左子圖</span>　"
        "<span style='color:#de8334'>● 右子圖</span>　<span style='color:#099c83'>━ 合併鏈</span>　"
        "<span style='color:#a68bd5'>┄ 共同切線</span>"), "muted"));
    connect(grid, &QCheckBox::toggled, canvas_, &VoronoiCanvas::setGrid);
    connect(hulls, &QCheckBox::toggled, canvas_, &VoronoiCanvas::setHulls);
    connect(chain, &QCheckBox::toggled, canvas_, &VoronoiCanvas::setChain);
    connect(labels, &QCheckBox::toggled, canvas_, &VoronoiCanvas::setLabels);
    connect(canvas_, &VoronoiCanvas::addPoint, this, &MainWindow::addPoint);
    connect(canvas_, &VoronoiCanvas::removePoint, this, &MainWindow::removePoint);
    body->addLayout(canvasColumn, 1);
    auto *sidebar = new QWidget; sidebar->setFixedWidth(320);
    auto *side = new QVBoxLayout(sidebar); side->setContentsMargins(0, 0, 0, 0); side->setSpacing(10);
    auto *stepCard = new QFrame; stepCard->setObjectName("card");
    auto *sl = new QVBoxLayout(stepCard); sl->setContentsMargins(17, 15, 17, 15);
    sl->addWidget(label("DIVIDE & CONQUER", "section"));
    phaseTitle_ = label(QStringLiteral("建立你的點集"), "phaseTitle");
    phaseText_ = label(QString(), "phaseText"); phaseText_->setWordWrap(true); phaseText_->setMinimumHeight(58);
    sl->addWidget(phaseTitle_); sl->addWidget(phaseText_);
    progress_ = new QProgressBar; progress_->setTextVisible(false); progress_->setFixedHeight(6); sl->addWidget(progress_);
    auto *execution = new QHBoxLayout;
    stepButton_ = button("Step by step", "stepButton"); runButton_ = button("Run", "runButton");
    execution->addWidget(stepButton_, 2); execution->addWidget(runButton_, 1); sl->addLayout(execution);
    playButton_ = button(QStringLiteral("自動播放步驟"), "playButton"); sl->addWidget(playButton_);
    auto *metrics = new QGridLayout;
    pointCount_ = label("0", "metric"); edgeCount_ = label("0", "metric"); timeLabel_ = label("0", "metric");
    metrics->addWidget(pointCount_, 0, 0); metrics->addWidget(edgeCount_, 0, 1); metrics->addWidget(timeLabel_, 0, 2);
    metrics->addWidget(label(QStringLiteral("不重複點"), "muted"), 1, 0);
    metrics->addWidget(label(QStringLiteral("畫布線段"), "muted"), 1, 1);
    metrics->addWidget(label(QStringLiteral("演算 ms"), "muted"), 1, 2); sl->addLayout(metrics); side->addWidget(stepCard);
    connect(stepButton_, &QPushButton::clicked, this, &MainWindow::stepAlgorithm);
    connect(runButton_, &QPushButton::clicked, this, &MainWindow::runAlgorithm);
    auto *stepKey = new QShortcut(QKeySequence(Qt::Key_F10), this), *runKey = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(stepKey, &QShortcut::activated, this, &MainWindow::stepAlgorithm);
    connect(runKey, &QShortcut::activated, this, &MainWindow::runAlgorithm);
    playTimer_ = new QTimer(this); playTimer_->setInterval(700);
    connect(playTimer_, &QTimer::timeout, this, &MainWindow::stepAlgorithm);
    connect(playButton_, &QPushButton::clicked, this, [this] {
        if (playTimer_->isActive()) stopPlayback();
        else { playTimer_->start(); playButton_->setText(QStringLiteral("暫停播放")); stepAlgorithm(); }
    });
    auto *inputCard = new QFrame; inputCard->setObjectName("card");
    auto *il = new QVBoxLayout(inputCard); il->setContentsMargins(15, 12, 15, 12);
    il->addWidget(label(QStringLiteral("POINTS / 輸入工具"), "section"));
    auto *manual = new QHBoxLayout;
    xInput_ = new QSpinBox; yInput_ = new QSpinBox; xInput_->setRange(0, 600); yInput_->setRange(0, 600);
    xInput_->setPrefix("x "); yInput_->setPrefix("y "); xInput_->setValue(300); yInput_->setValue(300);
    auto *add = button(QStringLiteral("新增"), "addButton");
    manual->addWidget(xInput_, 1); manual->addWidget(yInput_, 1); manual->addWidget(add); il->addLayout(manual);
    connect(add, &QPushButton::clicked, this, [this] { addPoint(xInput_->value(), yInput_->value()); });
    auto *randomRow = new QHBoxLayout;
    randomCount_ = new QSpinBox; randomCount_->setRange(1, 1000); randomCount_->setValue(12); randomCount_->setSuffix(QStringLiteral(" 點"));
    auto *random = button(QStringLiteral("隨機產生"), "randomButton"), *undo = button(QStringLiteral("復原一點"), "undoButton");
    randomRow->addWidget(randomCount_); randomRow->addWidget(random); randomRow->addWidget(undo); il->addLayout(randomRow);
    connect(random, &QPushButton::clicked, this, [this] {
        std::set<vd::Point> generated;
        while (int(generated.size()) < randomCount_->value())
            generated.insert({int(QRandomGenerator::global()->bounded(20, 581)), int(QRandomGenerator::global()->bounded(20, 581))});
        loadPoints(std::vector<vd::Point>(generated.begin(), generated.end()), QStringLiteral("隨機點集"));
    });
    connect(undo, &QPushButton::clicked, this, [this] {
        if (input_.empty()) return;
        auto points = input_; points.pop_back(); loadPoints(std::move(points));
    });
    auto *examples = new QComboBox; examples->setObjectName("exampleCombo");
    examples->addItems({QStringLiteral("載入內建範例…"), QStringLiteral("PDF 範例：四點菱形"), QStringLiteral("四點正方形 / 共圓"),
        QStringLiteral("三點包一點"), QStringLiteral("共線與重複點"), QStringLiteral("一般 12 點 / 多層合併")});
    il->addWidget(examples);
    connect(examples, &QComboBox::activated, this, [this, examples](int i) {
        const std::vector<std::vector<vd::Point>> sets = {{}, {{193,64},{193,370},{103,200},{283,200}},
            {{150,150},{450,150},{150,450},{450,450}}, {{331,179},{233,276},{432,275},{330,229}},
            {{100,100},{200,200},{300,300},{400,400},{300,300}},
            {{23,204},{123,432},{342,521},{292,341},{34,421},{591,591},{13,56},{324,65},{324,93},{63,24},{52,599},{43,435}}};
        if (i > 0) loadPoints(sets.at(i), examples->itemText(i));
        examples->setCurrentIndex(0);
    });
    auto *datasetRow = new QHBoxLayout;
    previousButton_ = button(QStringLiteral("上一組"), "previousButton"); nextButton_ = button(QStringLiteral("下一組"), "nextButton");
    datasetLabel_ = label(QStringLiteral("— / —"), "muted"); datasetLabel_->setAlignment(Qt::AlignCenter);
    datasetRow->addWidget(previousButton_); datasetRow->addWidget(datasetLabel_, 1); datasetRow->addWidget(nextButton_); il->addLayout(datasetRow);
    connect(previousButton_, &QPushButton::clicked, this, &MainWindow::previousDataset);
    connect(nextButton_, &QPushButton::clicked, this, &MainWindow::nextDataset); side->addWidget(inputCard);
    auto *tabs = new QTabWidget;
    pointTable_ = new QTableWidget(0, 3); pointTable_->setObjectName("pointTable");
    pointTable_->setHorizontalHeaderLabels({"ID", "x", "y"}); pointTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    pointTable_->verticalHeader()->hide(); pointTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pointTable_->setSelectionBehavior(QAbstractItemView::SelectRows); pointTable_->setAlternatingRowColors(true); pointTable_->setShowGrid(false);
    history_ = new QListWidget; history_->setObjectName("historyList");
    tabs->addTab(pointTable_, QStringLiteral("點座標")); tabs->addTab(history_, QStringLiteral("步驟紀錄")); tabs->setMinimumHeight(170);
    side->addWidget(tabs, 1); side->addWidget(label(QStringLiteral("F5 執行到底　F10 下一步　Ctrl+O 開啟"), "muted"));
    body->addWidget(sidebar); main->addLayout(body, 1);
    coordinateLabel_ = label(QStringLiteral("座標：—"), "muted"); statusBar()->addPermanentWidget(coordinateLabel_);
    connect(canvas_, &VoronoiCanvas::cursorPosition, this, [this](int x, int y) {
        coordinateLabel_->setText(x < 0 ? QStringLiteral("座標：—") : QStringLiteral("座標：(%1, %2)").arg(x).arg(y));
    });
    statusBar()->showMessage(QStringLiteral("左上角 (0, 0)，右下角 (600, 600)。請新增點或開啟 .in / .txt 檔。"));
}

void MainWindow::stopPlayback()
{
    playTimer_->stop(); playButton_->setText(QStringLiteral("自動播放步驟"));
}

void MainWindow::setInput(std::vector<vd::Point> points)
{
    stopPlayback(); input_ = std::move(points); imported_ = false; importedFrame_ = {};
    engine_.reset(input_); history_->clear(); refreshTable(); refresh();
    const auto duplicates = input_.size() - engine_.points().size();
    statusBar()->showMessage(QStringLiteral("已讀入 %1 點，%2 個不重複座標%3。")
        .arg(input_.size()).arg(engine_.points().size())
        .arg(duplicates ? QStringLiteral("（合併 %1 個重複點）").arg(duplicates) : QString()));
}

void MainWindow::loadPoints(std::vector<vd::Point> points, const QString &source)
{
    document_ = {}; datasetIndex_ = -1; fileEnded_ = false;
    source_ = source.isEmpty() ? QStringLiteral("滑鼠 / 座標輸入") : source;
    setInput(std::move(points));
}

void MainWindow::refreshTable()
{
    const auto &points = engine_.points(); pointTable_->setRowCount(static_cast<int>(points.size()));
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        for (int column = 0; column < 3; ++column) {
            const auto text = column == 0 ? QString("P%1").arg(i + 1) : QString::number(column == 1 ? points[i].x : points[i].y);
            auto *item = new QTableWidgetItem(text); item->setTextAlignment(Qt::AlignCenter); pointTable_->setItem(i, column, item);
        }
    }
}

void MainWindow::refresh(bool recordStep)
{
    const auto &frame = imported_ ? importedFrame_ : engine_.frame(); canvas_->setScene(engine_.points(), frame);
    QString title, description, badge;
    switch (frame.phase) {
    case vd::Phase::BeforeMerge:
        title = QStringLiteral("合併前 · %1 / %2").arg(frame.merge).arg(frame.totalMerges);
        description = QStringLiteral("深度 %1：左側 %2 點（藍）與右側 %3 點（橘）已各自求解。下一步沿共同切線建立合併鏈。")
            .arg(frame.depth).arg(frame.leftCount).arg(frame.rightCount);
        badge = QStringLiteral("暫停於合併之前"); break;
    case vd::Phase::Merged:
        title = QStringLiteral("合併完成 · %1 / %2").arg(frame.merge).arg(frame.totalMerges);
        description = QStringLiteral("已沿中垂線追蹤合併鏈（綠），裁除子圖中失效的邊。下一步繼續上一層遞迴。%1")
            .arg(engine_.fallbackMerges() ? QStringLiteral("退化合併保護：%1 次。").arg(engine_.fallbackMerges()) : QString());
        badge = QStringLiteral("合併子圖"); break;
    case vd::Phase::Complete:
        title = QStringLiteral("最近鄰分區已完成");
        description = QStringLiteral("%1 個不重複點，%2 條畫布內線段；共完成 %3 次遞迴合併。可儲存 P / E 格式結果。")
            .arg(engine_.points().size()).arg(frame.resultEdges.size()).arg(engine_.totalMerges());
        badge = QStringLiteral("演算完成"); break;
    case vd::Phase::Imported:
        title = QStringLiteral("已載入結果圖形");
        description = QStringLiteral("目前直接顯示檔案中的 P 點與 E 線段。按「重新演算」可用相同座標重新計算。");
        badge = QStringLiteral("P / E 結果檔"); break;
    default:
        title = engine_.points().empty() ? QStringLiteral("建立你的點集") : QStringLiteral("準備分割點集");
        description = QStringLiteral("點擊畫布輸入座標。Step by step 會在每次 merge 之前暫停；Run 可從當前狀態執行至最後。");
        badge = QStringLiteral("準備輸入");
    }
    phaseTitle_->setText(title); phaseText_->setText(description); stateBadge_->setText(badge);
    const int edges = frame.phase == vd::Phase::BeforeMerge ? int(frame.leftEdges.size() + frame.rightEdges.size()) : int(frame.resultEdges.size());
    pointCount_->setText(QString::number(engine_.points().size())); edgeCount_->setText(QString::number(edges));
    timeLabel_->setText(QString::number(engine_.milliseconds(), 'f', engine_.milliseconds() < 10 ? 2 : 1));
    progress_->setRange(0, std::max(1, engine_.totalMerges()));
    progress_->setValue(engine_.complete() || imported_ ? progress_->maximum() : engine_.completedMerges());
    const bool executable = !engine_.points().empty() && !engine_.complete() && !imported_;
    stepButton_->setEnabled(executable); runButton_->setEnabled(executable); playButton_->setEnabled(executable);
    saveButton_->setEnabled(imported_ || (engine_.complete() && !engine_.points().empty()));
    previousButton_->setEnabled(datasetIndex_ > 0); nextButton_->setEnabled(datasetIndex_ >= 0 && !fileEnded_);
    datasetLabel_->setText(datasetIndex_ < 0 ? QStringLiteral("— / —") : QString("%1 / %2").arg(datasetIndex_ + 1).arg(document_.datasets.size()));
    sourceLabel_->setText(fontMetrics().elidedText(source_, Qt::ElideMiddle, 340)); sourceLabel_->setToolTip(source_);
    if (recordStep) { history_->addItem(title + QStringLiteral("  ·  深度 %1").arg(frame.depth)); history_->scrollToBottom(); }
}

void MainWindow::stepAlgorithm()
{
    if (engine_.complete() || imported_ || engine_.points().empty()) return;
    engine_.advance(); if (engine_.complete()) stopPlayback(); refresh(true);
}
void MainWindow::runAlgorithm()
{
    if (engine_.complete() || imported_ || engine_.points().empty()) return;
    stopPlayback(); engine_.run(); refresh(true);
    statusBar()->showMessage(QStringLiteral("演算完成，可儲存結果或繼續輸入新座標。"));
}
void MainWindow::resetAlgorithm() { setInput(input_); }

void MainWindow::addPoint(int x, int y)
{
    if (std::find(input_.begin(), input_.end(), vd::Point{x, y}) != input_.end()) {
        statusBar()->showMessage(QStringLiteral("此座標已存在，重複點不會新增。"), 3500); return;
    }
    auto points = input_; points.push_back({x, y}); loadPoints(std::move(points));
}
void MainWindow::removePoint(int x, int y)
{
    if (input_.empty()) return;
    const auto closest = std::min_element(input_.begin(), input_.end(), [&](vd::Point a, vd::Point b) {
        return std::hypot(a.x - x, a.y - y) < std::hypot(b.x - x, b.y - y);
    });
    if (std::hypot(closest->x - x, closest->y - y) > 12) return;
    const auto target = *closest; auto points = input_;
    points.erase(std::remove(points.begin(), points.end(), target), points.end()); loadPoints(std::move(points));
}

bool MainWindow::loadFile(const QString &path, QString *error)
{
    try {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error(file.errorString().toStdString());
        const auto bytes = file.readAll(); std::istringstream stream(std::string(bytes.constData(), bytes.size()));
        auto loaded = vd::readDocument(stream); // Validate before replacing the current scene.
        document_ = std::move(loaded); source_ = QFileInfo(path).fileName(); datasetIndex_ = -1; fileEnded_ = false;
        if (document_.isResult) {
            setInput(document_.points); imported_ = true; importedFrame_.phase = vd::Phase::Imported;
            importedFrame_.resultEdges = document_.edges; refresh();
            statusBar()->showMessage(QStringLiteral("已直接顯示輸出檔：%1 點、%2 條線段。")
                .arg(document_.points.size()).arg(document_.edges.size()));
        } else if (document_.datasets.empty()) {
            setInput({}); fileEnded_ = true; statusBar()->showMessage(QStringLiteral("讀入點數為零，檔案測試停止。"));
        } else showDataset(0);
        return true;
    } catch (const std::exception &e) { if (error) *error = QString::fromUtf8(e.what()); return false; }
}

bool MainWindow::saveResult(const QString &path, QString *error) const
{
    try {
        if (!imported_ && !engine_.complete()) throw std::runtime_error("Run the algorithm before saving the result.");
        std::ostringstream stream; vd::writeOutput(stream, engine_.points(), imported_ ? importedFrame_.resultEdges : engine_.frame().resultEdges);
        const auto text = stream.str(); QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(text.data(), qint64(text.size())) != qint64(text.size()) || !file.commit())
            throw std::runtime_error(file.errorString().toStdString());
        return true;
    } catch (const std::exception &e) { if (error) *error = QString::fromUtf8(e.what()); return false; }
}
bool MainWindow::saveInput(const QString &path, QString *error) const
{
    try {
        std::ostringstream stream; vd::writeInput(stream, input_); const auto text = stream.str(); QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(text.data(), qint64(text.size())) != qint64(text.size()) || !file.commit())
            throw std::runtime_error(file.errorString().toStdString());
        return true;
    } catch (const std::exception &e) { if (error) *error = QString::fromUtf8(e.what()); return false; }
}
void MainWindow::showDataset(int index)
{
    datasetIndex_ = index; fileEnded_ = false; setInput(document_.datasets.at(static_cast<std::size_t>(index)));
}
void MainWindow::nextDataset()
{
    if (datasetIndex_ < 0) return;
    if (datasetIndex_ + 1 < int(document_.datasets.size())) showDataset(datasetIndex_ + 1);
    else {
        stopPlayback(); fileEnded_ = true;
        statusBar()->showMessage(document_.terminated ? QStringLiteral("讀入點數為零，檔案測試停止。") : QStringLiteral("已到達檔案結尾，檔案測試停止。")); refresh();
    }
}
void MainWindow::previousDataset() { if (datasetIndex_ > 0) showDataset(datasetIndex_ - 1); }
void MainWindow::chooseFile()
{
    const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("開啟測試資料或 P / E 結果檔"), QString(),
        QStringLiteral("文字測試資料 (*.in *.txt *.out);;所有檔案 (*)"));
    if (path.isEmpty()) return;
    QString error; if (!loadFile(path, &error)) QMessageBox::warning(this, QStringLiteral("讀取失敗"), error);
}
void MainWindow::chooseSave(bool input)
{
    if (!input && !saveButton_->isEnabled()) return;
    const auto path = QFileDialog::getSaveFileName(this, input ? QStringLiteral("儲存測試輸入") : QStringLiteral("儲存 Voronoi 結果"),
        input ? "points.in" : "voronoi.txt", input ? QStringLiteral("測試輸入 (*.in);;文字檔 (*.txt)") : QStringLiteral("文字結果 (*.txt)"));
    if (path.isEmpty()) return;
    QString error;
    if (!(input ? saveInput(path, &error) : saveResult(path, &error))) QMessageBox::warning(this, QStringLiteral("儲存失敗"), error);
    else statusBar()->showMessage(QStringLiteral("已儲存：%1").arg(path));
}
