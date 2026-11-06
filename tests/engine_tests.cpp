// Copyright (c) 2026 project author. Fill in student ID, Chinese and English name before submission.
#include "dataio.h"
#include "voronoi.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
}

// Independent O(n^3) oracle: clip every pair's bisector against ALL sites and the viewport.
// The production merge only sees recursive child adjacency and follows the separating chain.
std::map<std::pair<int, int>, vd::Segment> oracle(const std::vector<vd::Point> &points)
{
    std::map<std::pair<int, int>, vd::Segment> result;
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        for (int j = i + 1; j < static_cast<int>(points.size()); ++j) {
            const auto a = points[i], b = points[j];
            const double mx = (a.x + b.x) * .5, my = (a.y + b.y) * .5;
            const double vx = a.y - b.y, vy = b.x - a.x;
            double low = -1e20, high = 1e20;
            auto halfplane = [&](double nx, double ny, double c) {
                const double slope = nx * vx + ny * vy;
                const double remainder = c - nx * mx - ny * my;
                if (slope > 0) high = std::min(high, remainder / slope);
                else if (slope < 0) low = std::max(low, remainder / slope);
                else if (remainder < 0) high = low - 1;
            };
            halfplane(1, 0, 600); halfplane(-1, 0, 0);
            halfplane(0, 1, 600); halfplane(0, -1, 0);
            for (const auto &p : points)
                halfplane(2. * (p.x - a.x), 2. * (p.y - a.y),
                          double(p.x) * p.x + double(p.y) * p.y - double(a.x) * a.x - double(a.y) * a.y);
            if ((high - low) * std::hypot(vx, vy) > 1e-6)
                result[{i, j}] = {{mx + low * vx, my + low * vy}, {mx + high * vx, my + high * vy}, i, j};
        }
    }
    return result;
}

int totalFallbacks = 0, cases = 0;
void check(std::vector<vd::Point> input, bool step = false)
{
    vd::VoronoiEngine engine(input);
    if (step) {
        int before = 0, merged = 0;
        while (!engine.complete()) {
            const auto &frame = engine.advance();
            if (frame.phase == vd::Phase::BeforeMerge) {
                ++before;
                require(!frame.leftSites.empty() && !frame.rightSites.empty(), "Missing child diagrams before merge");
            }
            if (frame.phase == vd::Phase::Merged) ++merged;
        }
        require(before == engine.totalMerges() && merged == before, "Step schedule skipped a merge");
    } else {
        // Exercise Run from a partially advanced state as well as from the initial state.
        if (cases % 2) engine.advance();
        engine.run();
    }
    auto expected = oracle(engine.points());
    const auto actual = engine.visibleEdges(engine.result());
    auto dump = [&] {
        std::ostringstream stream;
        stream << "\nInput:";
        for (auto p : engine.points()) stream << " (" << p.x << ',' << p.y << ')';
        return stream.str();
    };
    require(actual.size() == expected.size(), "Edge count mismatch: " + std::to_string(actual.size())
            + " vs " + std::to_string(expected.size()) + dump());
    for (const auto &edge : actual) {
        const auto it = expected.find({edge.first, edge.second});
        require(it != expected.end(), "Unexpected edge" + dump());
        const auto &e = it->second;
        auto distance = [](vd::Vertex a, vd::Vertex b) { return std::hypotl(a.x - b.x, a.y - b.y); };
        require(distance(edge.a, e.a) < 1e-5 && distance(edge.b, e.b) < 1e-5, "Incorrect edge endpoints" + dump());
    }
    totalFallbacks += engine.fallbackMerges();
    ++cases;
}

void ioTests()
{
    std::istringstream input("\xEF\xBB\xBF# comment\n2 # pair\n0 0\n600 600\n1\n300 300\n0\nnot data\n");
    auto document = vd::readDocument(input);
    require(document.terminated && document.datasets.size() == 2, "Count/comment/terminator parsing failed");
    std::istringstream zero("0\nignored tail");
    require(vd::readDocument(zero).datasets.empty(), "Zero terminator failed");
    for (auto malformed : {"3\n0 0\n", "1\n601 0\n0\n", "P 1 2 3\n", "E 0 0 601 0\n", "1\n2.5 3\n0"}) {
        bool rejected = false;
        try { std::istringstream stream(malformed); vd::readDocument(stream); }
        catch (const std::runtime_error &) { rejected = true; }
        require(rejected, "Malformed document was accepted");
    }
    vd::VoronoiEngine engine({{193,64},{193,370},{103,200},{283,200}});
    engine.run();
    std::ostringstream output;
    vd::writeOutput(output, engine.points(), engine.frame().resultEdges);
    const std::string example = "P 103 200\nP 193 64\nP 193 370\nP 283 200\n"
        "E 0 34 193 161\nE 0 363 193 261\nE 193 161 193 261\nE 193 161 437 0\nE 193 261 600 476\n";
    require(output.str() == example, "Output does not match the PDF example:\n" + output.str());
    std::istringstream saved(output.str());
    auto loaded = vd::readDocument(saved);
    require(loaded.isResult && loaded.points.size() == 4 && loaded.edges.size() == 5, "P/E roundtrip failed");
    std::ostringstream twice;
    vd::writeOutput(twice, loaded.points, loaded.edges);
    require(twice.str() == output.str(), "Output ordering/roundtrip changed");
}
} // namespace

int main(int argc, char **argv)
{
    try {
        ioTests();
        if (argc > 2 && std::string(argv[1]) == "--file") {
            std::ifstream input(argv[2]);
            require(bool(input), "Unable to open dataset file");
            const auto document = vd::readDocument(input);
            require(!document.isResult, "Expected a count-prefixed input file");
            for (const auto &points : document.datasets) check(points, true);
            std::cout << "PASS: " << cases << " file datasets; fallback merges: " << totalFallbacks << '\n';
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "--benchmark") {
            std::mt19937 generator(20261106);
            std::cout << "points,visible_edges,merges,fallbacks,milliseconds\n";
            for (int n : {100, 1000, 5000}) {
                std::vector<vd::Point> input;
                for (int i = 0; i < n; ++i) input.push_back({int(generator() % 601), int(generator() % 601)});
                vd::VoronoiEngine engine(input); engine.run();
                // Validate every returned edge against nearest-site inequalities at its midpoint.
                for (const auto &edge : engine.frame().resultEdges) {
                    const auto x = (edge.a.x + edge.b.x) / 2, y = (edge.a.y + edge.b.y) / 2;
                    const auto owner = engine.points()[edge.first];
                    const auto distance = (x - owner.x) * (x - owner.x) + (y - owner.y) * (y - owner.y);
                    for (auto p : engine.points())
                        require((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y) >= distance - 1e-6L,
                                "Benchmark edge violates nearest-site inequality");
                }
                std::cout << engine.points().size() << ',' << engine.frame().resultEdges.size() << ','
                          << engine.totalMerges() << ',' << engine.fallbackMerges() << ',' << engine.milliseconds() << '\n';
            }
            return 0;
        }
        check({}); check({{300,300}}); check({{200,200},{200,200}});
        check({{0,0},{600,600}}); check({{200,200},{400,200}});
        check({{200,200},{200,400}}); check({{200,200},{300,200},{200,300}});
        check({{100,100},{100,200},{200,100},{200,200}}, true);
        check({{331,179},{233,276},{432,275},{330,229}}, true);
        check({{75,75},{167,167},{250,250},{147,187}}, true);
        check({{100,100},{100,200},{50,200},{150,200}}, true);
        check({{0,0},{600,0},{0,600},{600,600},{300,300}}, true);
        check({{100,300},{180,140},{300,100},{420,140},{500,300},{420,460},{300,500},{180,460}}, true);
        for (int n : {4,10,40,100}) {
            std::vector<vd::Point> diagonal, vertical, almostLine;
            for (int i = 0; i < n; ++i) {
                diagonal.push_back({i * 600 / n, i * 600 / n});
                vertical.push_back({300, i * 600 / n});
                almostLine.push_back({i * 600 / n, i * 600 / n + (i == n / 2 ? 1 : 0)});
            }
            check(diagonal); check(vertical); check(almostLine);
        }
        for (int size : {3,5,8,12}) {
            std::vector<vd::Point> grid;
            for (int x = 0; x < size; ++x)
                for (int y = 0; y < size; ++y) grid.push_back({x * 600 / (size - 1), y * 600 / (size - 1)});
            check(grid, true);
        }
        std::mt19937 random(20261106);
        const int randomCases = argc > 1 ? std::stoi(argv[1]) : 1200;
        for (int trial = 0; trial < randomCases; ++trial) {
            const int count = 4 + random() % 65;
            std::vector<vd::Point> points;
            for (int i = 0; i < count; ++i) {
                if (trial % 4 == 0) points.push_back({int(random() % 4) * 150, int(random() % 601)});
                else if (trial % 4 == 1) points.push_back({int(random() % 13) * 50, int(random() % 13) * 50});
                else points.push_back({int(random() % 601), int(random() % 601)});
            }
            check(points, trial % 7 == 0);
        }
        std::cout << "PASS: " << cases << " geometry cases + file I/O; degenerate fallback merges: " << totalFallbacks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
