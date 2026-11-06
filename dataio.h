// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#ifndef DATAIO_H
#define DATAIO_H

#include "voronoi.h"
#include <iosfwd>

namespace vd {
struct Document {
    bool isResult = false;
    bool terminated = false;
    std::vector<std::vector<Point>> datasets;
    std::vector<Point> points;
    std::vector<Segment> edges;
};

Document readDocument(std::istream &input);
void writeOutput(std::ostream &output, std::vector<Point> points, const std::vector<Segment> &edges);
void writeInput(std::ostream &output, const std::vector<Point> &points);
} // namespace vd
#endif
