// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#include "dataio.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace vd {
namespace {
struct Line { std::string text; int number; };

std::runtime_error error(int line, const std::string &message)
{
    return std::runtime_error("Line " + std::to_string(line) + ": " + message);
}

void noExtra(std::istringstream &stream, int line)
{
    std::string extra;
    if (stream >> extra) throw error(line, "unexpected extra data.");
}

Point readPoint(std::istringstream &stream, int line)
{
    Point p;
    if (!(stream >> p.x >> p.y)) throw error(line, "expected two integer coordinates.");
    if (p.x < 0 || p.y < 0 || p.x > CanvasSize || p.y > CanvasSize)
        throw error(line, "point coordinates must be in [0, 600].");
    noExtra(stream, line);
    return p;
}
} // namespace

Document readDocument(std::istream &input)
{
    Document document;
    std::vector<Line> lines;
    std::string text;
    int lineNumber = 0;
    // Preserve line boundaries for useful diagnostics; ignore whole-line and inline comments.
    while (std::getline(input, text)) {
        ++lineNumber;
        if (lineNumber == 1 && text.compare(0, 3, "\xEF\xBB\xBF") == 0) text.erase(0, 3);
        const auto comment = std::min(text.find('#'), text.find("//"));
        if (comment != std::string::npos) text.erase(comment);
        const auto begin = text.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos) continue;
        lines.push_back({text.substr(begin), lineNumber});
    }
    if (input.bad()) throw std::runtime_error("Unable to read the file.");
    if (lines.empty()) throw std::runtime_error("The file contains no data.");
    document.isResult = lines.front().text[0] == 'P' || lines.front().text[0] == 'E';
    if (document.isResult) {
        bool seenEdge = false;
        for (const auto &line : lines) {
            std::istringstream stream(line.text);
            char type;
            stream >> type;
            if (type == 'P') {
                if (seenEdge) throw error(line.number, "P records must precede E records.");
                document.points.push_back(readPoint(stream, line.number));
            } else if (type == 'E') {
                seenEdge = true;
                double x1, y1, x2, y2;
                if (!(stream >> x1 >> y1 >> x2 >> y2))
                    throw error(line.number, "expected E x1 y1 x2 y2.");
                for (double value : {x1, y1, x2, y2})
                    if (!std::isfinite(value) || value < 0 || value > CanvasSize)
                        throw error(line.number, "edge coordinates must be finite and in [0, 600].");
                noExtra(stream, line.number);
                document.edges.push_back({{x1, y1}, {x2, y2}});
            } else {
                throw error(line.number, "expected a P or E record.");
            }
        }
        return document;
    }
    std::size_t index = 0;
    while (index < lines.size()) {
        const auto &line = lines[index++];
        std::istringstream stream(line.text);
        int count;
        if (!(stream >> count) || count < 0) throw error(line.number, "expected a non-negative point count.");
        noExtra(stream, line.number);
        if (count == 0) {
            document.terminated = true;
            break; // Everything after the terminator is deliberately ignored.
        }
        if (static_cast<std::size_t>(count) > lines.size() - index)
            throw error(line.number, "not enough point records for this dataset.");
        std::vector<Point> points;
        points.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) {
            const auto &pointLine = lines[index++];
            std::istringstream pointStream(pointLine.text);
            points.push_back(readPoint(pointStream, pointLine.number));
        }
        document.datasets.push_back(std::move(points));
    }
    return document;
}

void writeOutput(std::ostream &output, std::vector<Point> points, const std::vector<Segment> &edges)
{
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    for (const auto &p : points) output << "P " << p.x << ' ' << p.y << '\n';
    std::vector<std::array<int, 4>> records;
    auto integer = [](long double value) {
        // The course's integer E example truncates subpixel endpoints; retain that convention.
        return static_cast<int>(std::floor(std::clamp(value, 0.L, 600.L) + 1e-8L));
    };
    for (const auto &edge : edges) {
        std::array<int, 2> a{integer(edge.a.x), integer(edge.a.y)};
        std::array<int, 2> b{integer(edge.b.x), integer(edge.b.y)};
        if (b < a) std::swap(a, b);
        if (a != b) records.push_back({a[0], a[1], b[0], b[1]});
    }
    std::sort(records.begin(), records.end());
    records.erase(std::unique(records.begin(), records.end()), records.end());
    for (const auto &e : records)
        output << "E " << e[0] << ' ' << e[1] << ' ' << e[2] << ' ' << e[3] << '\n';
    if (!output) throw std::runtime_error("Unable to write the output file.");
}

void writeInput(std::ostream &output, const std::vector<Point> &points)
{
    output << points.size() << '\n';
    for (const auto &point : points) output << point.x << ' ' << point.y << '\n';
    output << "0\n";
    if (!output) throw std::runtime_error("Unable to write the input file.");
}
} // namespace vd
