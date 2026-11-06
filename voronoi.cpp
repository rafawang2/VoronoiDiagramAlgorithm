// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#include "voronoi.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace vd {
namespace {
constexpr long double Infinity = std::numeric_limits<long double>::infinity();

long double cross(Point a, Point b, Point c)
{
    return static_cast<long double>(b.x - a.x) * (c.y - a.y)
         - static_cast<long double>(b.y - a.y) * (c.x - a.x);
}

bool nonempty(const Edge &edge)
{
    if (edge.lower >= edge.upper) return false;
    if (!std::isfinite(edge.lower) || !std::isfinite(edge.upper)) return true;
    const auto scale = std::max({1.L, std::abs(edge.lower), std::abs(edge.upper)});
    return edge.upper - edge.lower > 1e-15L * scale;
}

bool near(long double a, long double b)
{
    return std::abs(a - b) <= 1e-12L + 1e-14L * std::max(std::abs(a), std::abs(b));
}
} // namespace

VoronoiEngine::VoronoiEngine(std::vector<Point> points) { reset(std::move(points)); }

void VoronoiEngine::reset(std::vector<Point> points)
{
    for (const auto &p : points)
        if (p.x < 0 || p.y < 0 || p.x > CanvasSize || p.y > CanvasSize)
            throw std::invalid_argument("Point coordinates must be between 0 and 600.");
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    points_ = std::move(points);
    nodes_.clear();
    nodes_.reserve(2 * points_.size() + 1);
    actions_.clear();
    cursor_ = 0;
    mergeCount_ = mergesDone_ = fallbackCount_ = 0;
    elapsedMs_ = 0;
    finished_ = false;
    frame_ = {};
    root_ = build(0, static_cast<int>(points_.size()), 0);
    frame_.totalMerges = mergeCount_;
}

bool VoronoiEngine::collinear(int begin, int end) const
{
    for (int i = begin + 2; i < end; ++i)
        if (cross(points_[begin], points_[begin + 1], points_[i]) != 0) return false;
    return true;
}

int VoronoiEngine::build(int begin, int end, int depth)
{
    const int index = static_cast<int>(nodes_.size());
    nodes_.push_back({begin, end, -1, -1, depth, {}, {}});
    if (end - begin <= 3 || collinear(begin, end)) {
        actions_.push_back({ActionKind::Base, index});
        return index;
    }
    // Never split a column of equal x coordinates: the separating chain is then y-monotone.
    int middle = (begin + end) / 2;
    if (points_[middle - 1].x == points_[middle].x) {
        int before = middle, after = middle;
        while (before > begin && points_[before - 1].x == points_[middle].x) --before;
        while (after < end && points_[after].x == points_[middle].x) ++after;
        if (before == begin) middle = after;
        else if (after == end) middle = before;
        else middle = middle - before <= after - middle ? before : after;
    }
    nodes_[index].left = build(begin, middle, depth + 1);
    nodes_[index].right = build(middle, end, depth + 1);
    actions_.push_back({ActionKind::Before, index});
    actions_.push_back({ActionKind::Merge, index});
    ++mergeCount_;
    return index;
}

Edge VoronoiEngine::bisector(int first, int second) const
{
    if (second < first) std::swap(first, second);
    return {first, second, -Infinity, Infinity};
}

Vertex VoronoiEngine::at(const Edge &edge, long double t) const
{
    const auto a = points_[edge.first], b = points_[edge.second];
    return {(a.x + b.x) / 2.L - (b.y - a.y) * t,
            (a.y + b.y) / 2.L + (b.x - a.x) * t};
}

bool VoronoiEngine::restrictToSite(Edge &edge, int owner, int competitor) const
{
    const auto a = points_[edge.first], b = points_[edge.second];
    const auto p = points_[owner], q = points_[competitor];
    const long double dx = q.x - p.x, dy = q.y - p.y;
    const long double mx = (a.x + b.x) / 2.L - p.x, my = (a.y + b.y) / 2.L - p.y;
    // |X-p|^2 <= |X-q|^2, evaluated relative to p to avoid subtracting large squared distances.
    const long double coefficient = 2 * (-dx * (b.y - a.y) + dy * (b.x - a.x));
    const long double limit = dx * dx + dy * dy - 2 * (dx * mx + dy * my);
    if (coefficient == 0) {
        if (limit < 0) { edge.lower = 1; edge.upper = 0; }
        return nonempty(edge);
    }
    const auto t = limit / coefficient;
    if (coefficient > 0) edge.upper = std::min(edge.upper, t);
    else edge.lower = std::max(edge.lower, t);
    return nonempty(edge);
}

bool VoronoiEngine::restrictToCell(Edge &edge, int owner, const Diagram &diagram) const
{
    const auto found = diagram.neighbors.find(owner);
    if (found != diagram.neighbors.end())
        for (int other : found->second)
            if (!restrictToSite(edge, owner, other)) return false;
    return nonempty(edge);
}

std::vector<int> VoronoiEngine::hull(const std::vector<int> &sites, bool keepCollinear) const
{
    if (sites.size() <= 2) return sites;
    std::vector<int> lower, upper;
    auto append = [&](std::vector<int> &chain, int id) {
        while (chain.size() >= 2) {
            auto turn = cross(points_[chain[chain.size() - 2]], points_[chain.back()], points_[id]);
            if (keepCollinear ? turn >= 0 : turn > 0) break;
            chain.pop_back();
        }
        chain.push_back(id);
    };
    for (int id : sites) append(lower, id);
    for (auto it = sites.rbegin(); it != sites.rend(); ++it) append(upper, *it);
    lower.pop_back();
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());
    return lower;
}

void VoronoiEngine::indexDiagram(Diagram &diagram) const
{
    diagram.neighbors.clear();
    for (const auto &edge : diagram.edges) {
        diagram.neighbors[edge.first].push_back(edge.second);
        diagram.neighbors[edge.second].push_back(edge.first);
    }
    for (auto &[id, neighbors] : diagram.neighbors) {
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
    }
    diagram.hull = hull(diagram.sites);
}

Diagram VoronoiEngine::baseDiagram(int begin, int end) const
{
    Diagram diagram;
    for (int i = begin; i < end; ++i) diagram.sites.push_back(i);
    if (collinear(begin, end)) {
        // Sorted adjacent sites alone contribute edges in the collinear case.
        for (int i = begin + 1; i < end; ++i) diagram.edges.push_back(bisector(i - 1, i));
    } else {
        for (int i = begin; i < end; ++i) {
            for (int j = i + 1; j < end; ++j) {
                auto edge = bisector(i, j);
                for (int k = begin; k < end && nonempty(edge); ++k) restrictToSite(edge, i, k);
                if (nonempty(edge)) diagram.edges.push_back(edge);
            }
        }
    }
    indexDiagram(diagram);
    return diagram;
}

std::vector<int> VoronoiEngine::tiedSites(const Diagram &diagram, int start, Vertex vertex) const
{
    std::vector<int> tied{start};
    std::set<int> visited{start};
    const auto p = points_[start];
    for (std::size_t i = 0; i < tied.size(); ++i) {
        const auto found = diagram.neighbors.find(tied[i]);
        if (found == diagram.neighbors.end()) continue;
        for (int id : found->second) {
            if (visited.count(id)) continue;
            const auto q = points_[id];
            const long double dx = q.x - p.x, dy = q.y - p.y;
            const auto a = dx * dx + dy * dy;
            const auto b = 2 * (dx * (vertex.x - p.x) + dy * (vertex.y - p.y));
            if (std::abs(a - b) <= 1e-12L + 1e-14L * (std::abs(a) + std::abs(b))) {
                visited.insert(id);
                tied.push_back(id);
            }
        }
    }
    return tied;
}

std::vector<Edge> VoronoiEngine::traceChain(const Diagram &left, const Diagram &right) const
{
    std::vector<int> sites = left.sites;
    sites.insert(sites.end(), right.sites.begin(), right.sites.end());
    const int boundary = right.sites.front();
    const auto boundaryHull = hull(sites, true);
    Edge current;
    bool foundStart = false;
    // The upper common tangent is dual to the first unbounded edge of the merge chain.
    for (std::size_t i = 0; i < boundaryHull.size(); ++i) {
        int a = boundaryHull[i], b = boundaryHull[(i + 1) % boundaryHull.size()];
        if ((a < boundary) == (b < boundary)) continue;
        if (b < a) std::swap(a, b);
        auto edge = bisector(a, b);
        if (restrictToCell(edge, a, left) && restrictToCell(edge, b, right)
            && edge.lower == -Infinity) {
            current = edge;
            foundStart = true;
            break;
        }
    }
    if (!foundStart) return {};
    std::vector<Edge> chain;
    std::set<std::pair<int, int>> visited;
    while (visited.emplace(current.first, current.second).second) {
        chain.push_back(current);
        if (current.upper == Infinity) return chain;
        const auto vertex = at(current, current.upper);
        const auto leftCandidates = tiedSites(left, current.first, vertex);
        const auto rightCandidates = tiedSites(right, current.second, vertex);
        bool nextFound = false;
        // At a cocircular vertex several cells can change together; skip zero-length dual edges.
        for (int a : leftCandidates) {
            for (int b : rightCandidates) {
                if (visited.count({a, b})) continue;
                auto next = bisector(a, b);
                if (!restrictToCell(next, a, left) || !restrictToCell(next, b, right)
                    || !std::isfinite(next.lower)) continue;
                const auto start = at(next, next.lower);
                if (!near(start.x, vertex.x) || !near(start.y, vertex.y)) continue;
                current = next;
                nextFound = true;
                break;
            }
            if (nextFound) break;
        }
        if (!nextFound) return {};
    }
    return {};
}

Diagram VoronoiEngine::mergeDiagrams(const Diagram &left, const Diagram &right, std::vector<Edge> &chain)
{
    chain = traceChain(left, right);
    if (chain.empty()) {
        // Robust geometric merge for numerical/degenerate ties: intersect child cells on each
        // cross bisector. This still merges the two recursive diagrams; it is not a global rebuild.
        ++fallbackCount_;
        for (int a : left.sites) {
            for (int b : right.sites) {
                auto edge = bisector(a, b);
                if (restrictToCell(edge, a, left) && restrictToCell(edge, b, right)) chain.push_back(edge);
            }
        }
        std::sort(chain.begin(), chain.end(), [&](const Edge &a, const Edge &b) {
            const auto ay = std::isfinite(a.lower) ? at(a, a.lower).y : -Infinity;
            const auto by = std::isfinite(b.lower) ? at(b, b.lower).y : -Infinity;
            return ay < by;
        });
    }
    Diagram merged;
    merged.sites = left.sites;
    merged.sites.insert(merged.sites.end(), right.sites.begin(), right.sites.end());
    merged.edges = chain;
    std::map<int, std::vector<int>> crossNeighbors;
    for (const auto &edge : chain) {
        crossNeighbors[edge.first].push_back(edge.second);
        crossNeighbors[edge.second].push_back(edge.first);
    }
    auto prune = [&](const Diagram &child) {
        for (auto edge : child.edges) {
            // The chain gives the new constraints on each child cell. Remove the parts on
            // the opposite diagram's side, retaining the original infinite-edge parameters.
            const auto constraints = crossNeighbors.find(edge.first);
            if (constraints != crossNeighbors.end())
                for (int other : constraints->second)
                    if (!restrictToSite(edge, edge.first, other)) break;
            if (nonempty(edge)) merged.edges.push_back(edge);
        }
    };
    prune(left);
    prune(right);
    indexDiagram(merged);
    return merged;
}

bool VoronoiEngine::clipToCanvas(const Edge &edge, Segment &segment) const
{
    auto lower = edge.lower, upper = edge.upper;
    const auto origin = at(edge, 0), directionPoint = at(edge, 1);
    const Vertex direction{directionPoint.x - origin.x, directionPoint.y - origin.y};
    auto axis = [&](long double p, long double d) {
        if (d == 0) return p >= 0 && p <= CanvasSize;
        auto a = -p / d, b = (CanvasSize - p) / d;
        if (a > b) std::swap(a, b);
        lower = std::max(lower, a);
        upper = std::min(upper, b);
        return lower < upper;
    };
    if (!axis(origin.x, direction.x) || !axis(origin.y, direction.y) || lower >= upper) return false;
    segment = {at(edge, lower), at(edge, upper), edge.first, edge.second};
    for (auto *p : {&segment.a, &segment.b}) {
        p->x = std::clamp(p->x, 0.L, static_cast<long double>(CanvasSize));
        p->y = std::clamp(p->y, 0.L, static_cast<long double>(CanvasSize));
    }
    return std::hypotl(segment.a.x - segment.b.x, segment.a.y - segment.b.y) > 1e-9L;
}

std::vector<Segment> VoronoiEngine::visibleEdges(const Diagram &diagram) const
{
    std::vector<Segment> visible;
    for (const auto &edge : diagram.edges) {
        Segment segment;
        if (clipToCanvas(edge, segment)) visible.push_back(segment);
    }
    return visible;
}

std::vector<Segment> VoronoiEngine::hullSegments(const Diagram &diagram) const
{
    std::vector<Segment> result;
    if (diagram.hull.size() < 2) return result;
    for (std::size_t i = 0; i < diagram.hull.size(); ++i) {
        const auto a = points_[diagram.hull[i]], b = points_[diagram.hull[(i + 1) % diagram.hull.size()]];
        result.push_back({{static_cast<long double>(a.x), static_cast<long double>(a.y)},
                          {static_cast<long double>(b.x), static_cast<long double>(b.y)}});
        if (diagram.hull.size() == 2) break;
    }
    return result;
}

std::vector<Segment> VoronoiEngine::commonTangents(const Diagram &left, const Diagram &right) const
{
    auto sites = left.sites;
    sites.insert(sites.end(), right.sites.begin(), right.sites.end());
    auto outline = hull(sites, true);
    const int boundary = right.sites.front();
    std::vector<Segment> result;
    for (std::size_t i = 0; i < outline.size(); ++i) {
        const auto a = outline[i], b = outline[(i + 1) % outline.size()];
        if ((a < boundary) == (b < boundary)) continue;
        const auto p = points_[a], q = points_[b];
        result.push_back({{static_cast<long double>(p.x), static_cast<long double>(p.y)},
                          {static_cast<long double>(q.x), static_cast<long double>(q.y)}});
    }
    return result;
}

void VoronoiEngine::makeFrame(const Node &node, Phase phase)
{
    frame_ = {};
    frame_.phase = phase;
    frame_.depth = node.depth;
    frame_.merge = phase == Phase::BeforeMerge ? mergesDone_ + 1 : mergesDone_;
    frame_.totalMerges = mergeCount_;
    if (node.left >= 0) {
        const auto &left = nodes_[node.left].diagram, &right = nodes_[node.right].diagram;
        frame_.leftSites = left.sites;
        frame_.rightSites = right.sites;
        frame_.leftCount = static_cast<int>(left.sites.size());
        frame_.rightCount = static_cast<int>(right.sites.size());
        frame_.splitX = (points_[left.sites.back()].x + points_[right.sites.front()].x) / 2.L;
        frame_.leftEdges = visibleEdges(left);
        frame_.rightEdges = visibleEdges(right);
        frame_.leftHull = hullSegments(left);
        frame_.rightHull = hullSegments(right);
        frame_.tangents = commonTangents(left, right);
    }
    if (phase == Phase::Merged || phase == Phase::Complete) {
        frame_.resultEdges = visibleEdges(node.diagram);
        for (const auto &edge : node.chain) {
            Segment segment;
            if (clipToCanvas(edge, segment)) frame_.chain.push_back(segment);
        }
    }
}

const Frame &VoronoiEngine::advance()
{
    if (finished_) return frame_;
    const auto started = std::chrono::steady_clock::now();
    while (cursor_ < actions_.size()) {
        const auto action = actions_[cursor_++];
        auto &node = nodes_[action.node];
        if (action.kind == ActionKind::Base) {
            node.diagram = baseDiagram(node.begin, node.end);
            continue;
        }
        if (action.kind == ActionKind::Before) {
            makeFrame(node, Phase::BeforeMerge);
        } else {
            node.diagram = mergeDiagrams(nodes_[node.left].diagram, nodes_[node.right].diagram, node.chain);
            ++mergesDone_;
            makeFrame(node, Phase::Merged);
        }
        elapsedMs_ += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
        return frame_;
    }
    finished_ = true;
    makeFrame(nodes_[root_], Phase::Complete);
    elapsedMs_ += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    return frame_;
}

const Frame &VoronoiEngine::run()
{
    while (!finished_) advance();
    return frame_;
}

const Diagram &VoronoiEngine::result() const
{
    if (!finished_) throw std::logic_error("Run the algorithm before requesting its final diagram.");
    return nodes_[root_].diagram;
}
} // namespace vd
