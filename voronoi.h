// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#ifndef VORONOI_H
#define VORONOI_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace vd {

constexpr int CanvasSize = 600;

struct Point {
    int x = 0;
    int y = 0;
    bool operator<(const Point &p) const { return x < p.x || (x == p.x && y < p.y); }
    bool operator==(const Point &p) const { return x == p.x && y == p.y; }
};

struct Vertex { long double x = 0, y = 0; };
struct Segment {
    Vertex a, b;
    int first = -1, second = -1;
};

// An unbounded Voronoi edge: midpoint(first,second) + t * perpendicular(second-first).
// Infinite parameter limits represent lines/rays. Canvas clipping happens only when drawing/exporting.
struct Edge {
    int first = -1, second = -1;
    long double lower = 0, upper = 0;
};

struct Diagram {
    std::vector<int> sites;
    std::vector<Edge> edges;
    std::map<int, std::vector<int>> neighbors;
    std::vector<int> hull;
};

enum class Phase { Input, BeforeMerge, Merged, Complete, Imported };

struct Frame {
    Phase phase = Phase::Input;
    int depth = 0;
    int merge = 0, totalMerges = 0;
    int leftCount = 0, rightCount = 0;
    long double splitX = 0;
    std::vector<int> leftSites, rightSites;
    std::vector<Segment> leftEdges, rightEdges, resultEdges, chain;
    std::vector<Segment> leftHull, rightHull, tangents;
};

class VoronoiEngine {
public:
    explicit VoronoiEngine(std::vector<Point> points = {});
    void reset(std::vector<Point> points);
    const Frame &advance();
    const Frame &run();
    bool complete() const { return finished_; }
    const std::vector<Point> &points() const { return points_; }
    const Frame &frame() const { return frame_; }
    const Diagram &result() const;
    int totalMerges() const { return mergeCount_; }
    int completedMerges() const { return mergesDone_; }
    int fallbackMerges() const { return fallbackCount_; }
    double milliseconds() const { return elapsedMs_; }
    std::vector<Segment> visibleEdges(const Diagram &diagram) const;

private:
    struct Node {
        int begin = 0, end = 0, left = -1, right = -1, depth = 0;
        Diagram diagram;
        std::vector<Edge> chain;
    };
    enum class ActionKind { Base, Before, Merge };
    struct Action { ActionKind kind; int node; };

    std::vector<Point> points_;
    std::vector<Node> nodes_;
    std::vector<Action> actions_;
    std::size_t cursor_ = 0;
    int root_ = -1, mergeCount_ = 0, mergesDone_ = 0, fallbackCount_ = 0;
    bool finished_ = false;
    double elapsedMs_ = 0;
    Frame frame_;

    int build(int begin, int end, int depth);
    bool collinear(int begin, int end) const;
    Diagram baseDiagram(int begin, int end) const;
    Diagram mergeDiagrams(const Diagram &left, const Diagram &right, std::vector<Edge> &chain);
    std::vector<Edge> traceChain(const Diagram &left, const Diagram &right) const;
    std::vector<int> tiedSites(const Diagram &diagram, int start, Vertex vertex) const;
    Edge bisector(int first, int second) const;
    bool restrictToSite(Edge &edge, int owner, int competitor) const;
    bool restrictToCell(Edge &edge, int owner, const Diagram &diagram) const;
    Vertex at(const Edge &edge, long double parameter) const;
    bool clipToCanvas(const Edge &edge, Segment &segment) const;
    std::vector<int> hull(const std::vector<int> &sites, bool keepCollinear = false) const;
    void indexDiagram(Diagram &diagram) const;
    std::vector<Segment> hullSegments(const Diagram &diagram) const;
    std::vector<Segment> commonTangents(const Diagram &left, const Diagram &right) const;
    void makeFrame(const Node &node, Phase phase);
};

} // namespace vd
#endif
