// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#include "voronoicanvas.h"
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>

VoronoiCanvas::VoronoiCanvas(QWidget *parent) : QWidget(parent)
{
    setObjectName("voronoiCanvas");
    setMinimumSize(660, 660); // Inner drawable rectangle: at least 600 x 600 actual widget pixels.
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("Voronoi 畫布，左鍵新增點，右鍵刪除鄰近點"));
}

QRectF VoronoiCanvas::boardRect() const
{
    const qreal side = std::min(width() - 60., height() - 60.);
    return {(width() - side) / 2., (height() - side) / 2., side, side};
}

void VoronoiCanvas::setScene(const std::vector<vd::Point> &points, const vd::Frame &frame)
{
    points_ = points; frame_ = frame; update();
}

void VoronoiCanvas::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f3f6fa"));
    const auto board = boardRect();
    const auto scale = board.width() / vd::CanvasSize;
    auto map = [&](vd::Vertex v) { return QPointF(board.left() + double(v.x) * scale, board.top() + double(v.y) * scale); };
    p.setPen(QPen(QColor("#d2dce8"), 1)); p.setBrush(Qt::white); p.drawRect(board);
    const bool merging = frame_.phase == vd::Phase::BeforeMerge || frame_.phase == vd::Phase::Merged;
    p.save(); p.setClipRect(board.adjusted(-1, -1, 1, 1));
    if (merging) {
        const auto split = board.left() + double(frame_.splitX) * scale;
        p.fillRect(QRectF(board.left(), board.top(), split - board.left(), board.height()), QColor("#f3f9ff"));
        p.fillRect(QRectF(split, board.top(), board.right() - split, board.height()), QColor("#fff8f0"));
    }
    if (grid_) {
        p.setPen(QPen(QColor("#edf1f6"), 1));
        for (int v = 50; v < 600; v += 50) {
            p.drawLine(map({static_cast<long double>(v), 0}), map({static_cast<long double>(v), 600}));
            p.drawLine(map({0, static_cast<long double>(v)}), map({600, static_cast<long double>(v)}));
        }
    }
    if (merging) {
        p.setPen(QPen(QColor("#b6c4d3"), 1, Qt::DashLine));
        p.drawLine(map({frame_.splitX, 0}), map({frame_.splitX, 600}));
    }
    auto edges = [&](const std::vector<vd::Segment> &segments, QColor color, qreal width, Qt::PenStyle style = Qt::SolidLine) {
        p.setPen(QPen(color, width, style, Qt::RoundCap, Qt::RoundJoin));
        for (const auto &s : segments) p.drawLine(map(s.a), map(s.b));
    };
    if (merging && hulls_) {
        edges(frame_.leftHull, QColor("#90b9e9"), 1.3, Qt::DashLine);
        edges(frame_.rightHull, QColor("#e8b281"), 1.3, Qt::DashLine);
        edges(frame_.tangents, QColor("#a68bd5"), 1.7, Qt::DashLine);
    }
    if (frame_.phase == vd::Phase::BeforeMerge) {
        edges(frame_.leftEdges, QColor("#2475c7"), 2.1);
        edges(frame_.rightEdges, QColor("#de8334"), 2.1);
    } else {
        edges(frame_.resultEdges, QColor("#365979"), 2.0);
        if (frame_.phase == vd::Phase::Merged && chain_) edges(frame_.chain, QColor("#099c83"), 3.0);
    }
    p.restore();
    QFont small = font(); small.setPointSize(8); p.setFont(small); p.setPen(QColor("#8293a7"));
    for (int v = 0; v <= 600; v += 100) {
        const auto pos = map({static_cast<long double>(v), static_cast<long double>(v)});
        p.drawText(QRectF(pos.x() - 16, board.bottom() + 8, 32, 17), Qt::AlignCenter, QString::number(v));
        p.drawText(QRectF(board.left() - 30, pos.y() - 8, 24, 17), Qt::AlignRight | Qt::AlignVCenter, QString::number(v));
    }
    std::vector<int> groups(points_.size(), 0);
    if (merging) {
        for (int id : frame_.leftSites) if (id >= 0 && id < int(groups.size())) groups[id] = 1;
        for (int id : frame_.rightSites) if (id >= 0 && id < int(groups.size())) groups[id] = 2;
    }
    for (std::size_t i = 0; i < points_.size(); ++i) {
        const auto site = points_[i];
        const auto pos = map({static_cast<long double>(site.x), static_cast<long double>(site.y)});
        QColor color = groups[i] == 1 ? QColor("#2475c7") : groups[i] == 2 ? QColor("#de8334") : QColor("#243b53");
        if (merging && groups[i] == 0) color = QColor("#c3ceda");
        p.setPen(QPen(Qt::white, 2)); p.setBrush(color); p.drawEllipse(pos, 4.7, 4.7);
        if (labels_) {
            const auto text = QString("P%1 (%2, %3)").arg(i + 1).arg(site.x).arg(site.y);
            const int tw = p.fontMetrics().horizontalAdvance(text) + 8;
            qreal x = std::clamp(pos.x() + 8, board.left() + 4, board.right() - tw - 4);
            qreal y = std::clamp(pos.y() - 22, board.top() + 3, board.bottom() - 20);
            p.setPen(QColor("#53687f")); p.fillRect(QRectF(x, y, tw, 18), QColor(255, 255, 255, 215));
            p.drawText(QRectF(x + 3, y, tw - 6, 18), Qt::AlignVCenter, text);
        }
    }
    if (points_.empty() && frame_.resultEdges.empty()) {
        p.setPen(QColor("#bccbda")); QFont big = font(); big.setPointSize(23); big.setWeight(QFont::Light); p.setFont(big);
        p.drawText(board.adjusted(10, -24, -10, -24), Qt::AlignCenter, QStringLiteral("從一個點開始"));
        small.setPointSize(10); p.setFont(small); p.setPen(QColor("#8a9db1"));
        p.drawText(board.adjusted(10, 35, -10, 35), Qt::AlignCenter, QStringLiteral("點擊畫布新增座標，或開啟測試資料"));
    }
    if (hover_.x() >= 0) {
        p.setFont(small); p.setPen(QColor("#7c8ea2"));
        p.drawText(QRectF(board.right() - 130, board.top() - 25, 130, 19), Qt::AlignRight,
                   QString("(%1, %2)").arg(hover_.x()).arg(hover_.y()));
    }
}

QPoint VoronoiCanvas::logicalPosition(QPointF pos) const
{
    const auto board = boardRect();
    if (!board.contains(pos)) return {-1, -1};
    return {std::clamp(qRound((pos.x() - board.left()) * 600 / board.width()), 0, 600),
            std::clamp(qRound((pos.y() - board.top()) * 600 / board.height()), 0, 600)};
}
void VoronoiCanvas::mousePressEvent(QMouseEvent *e)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto point = logicalPosition(e->position());
#else
    const auto point = logicalPosition(e->localPos());
#endif
    if (point.x() < 0) return;
    if (e->button() == Qt::LeftButton) emit addPoint(point.x(), point.y());
    if (e->button() == Qt::RightButton) emit removePoint(point.x(), point.y());
}
void VoronoiCanvas::mouseMoveEvent(QMouseEvent *e)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    hover_ = logicalPosition(e->position());
#else
    hover_ = logicalPosition(e->localPos());
#endif
    emit cursorPosition(hover_.x(), hover_.y()); update();
}
void VoronoiCanvas::leaveEvent(QEvent *e)
{
    hover_ = {-1, -1}; emit cursorPosition(-1, -1); update(); QWidget::leaveEvent(e);
}
