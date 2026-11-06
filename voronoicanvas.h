// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#ifndef VORONOICANVAS_H
#define VORONOICANVAS_H
#include "voronoi.h"
#include <QWidget>

class VoronoiCanvas : public QWidget {
    Q_OBJECT
public:
    explicit VoronoiCanvas(QWidget *parent = nullptr);
    void setScene(const std::vector<vd::Point> &points, const vd::Frame &frame);
    void setGrid(bool on) { grid_ = on; update(); }
    void setHulls(bool on) { hulls_ = on; update(); }
    void setLabels(bool on) { labels_ = on; update(); }
    void setChain(bool on) { chain_ = on; update(); }
    QRectF boardRect() const;
    QSize sizeHint() const override { return {720, 720}; }
signals:
    void addPoint(int x, int y);
    void removePoint(int x, int y);
    void cursorPosition(int x, int y);
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    std::vector<vd::Point> points_;
    vd::Frame frame_;
    bool grid_ = true, hulls_ = true, labels_ = false, chain_ = true;
    QPoint hover_{-1, -1};
    QPoint logicalPosition(QPointF position) const;
};
#endif
