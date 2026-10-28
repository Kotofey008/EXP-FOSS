#include "exploration/grid_state.hpp"
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>
using exploration::GridState;
static std::size_t checks = 0;
static void require(bool condition) {
    ++checks;
    if (!condition) throw std::runtime_error("grid state check failed");
}
static void check_tree(const GridState& g, const std::vector<std::uint16_t>& values,
                       bool requested, unsigned mask) {
    std::vector<unsigned> previous;
    for (unsigned level = 0; level < 4; ++level) {
        std::vector<unsigned> counts((values.size() + (std::size_t(1) << (4 * (level + 1))) - 1)
                                     >> (4 * (level + 1)), 0);
        if (level == 0) {
            for (std::size_t i = 0; i < values.size(); ++i)
                if (values[i] & mask) ++counts[i >> 4];
        } else {
            for (std::size_t i = 0; i < previous.size(); ++i)
                if (previous[i]) ++counts[i >> 4];
        }
        for (std::size_t i = 0; i < counts.size(); ++i)
            require(g.count(requested, level, i) == counts[i]);
        previous.swap(counts);
    }
}
int main() {
    GridState empty(0);
    try { empty.set_state(0, 1); require(false); } catch (const std::out_of_range&) {}
    const std::size_t n = 65553;
    GridState grid(n);
    std::vector<std::uint16_t> input(n, 0), working(n, 0);
    auto change = [&](std::size_t i, std::uint16_t v) {
        grid.set_state(i, v); input[i] = v; working[i] = v & 0xbfffU;
        require(grid.baseline(i) == 0);
        require(grid.requested(i) == v);
        require(grid.working(i) == working[i]);
    };
    // All combinations of flag bits, repeated assignments, low-bit-only changes.
    for (unsigned v = 0; v <= 0xffff; ++v) change(0, static_cast<std::uint16_t>(v));
    check_tree(grid, input, true, 0x4000); check_tree(grid, working, false, 0x8000);
    for (std::size_t i = 0; i < n; ++i) change(i, 0xc001);
    check_tree(grid, input, true, 0x4000); check_tree(grid, working, false, 0x8000);
    for (std::size_t i = n; i-- > 0;) change(i, 0);
    check_tree(grid, input, true, 0x4000); check_tree(grid, working, false, 0x8000);
    std::mt19937 random(0x77380);
    for (unsigned batch = 0; batch < 25; ++batch) {
        for (unsigned j = 0; j < 1000; ++j) change(random() % n, static_cast<std::uint16_t>(random()));
        check_tree(grid, input, true, 0x4000); check_tree(grid, working, false, 0x8000);
    }
    try { grid.set_state(n, 0xffff); require(false); } catch (const std::out_of_range&) {}
    check_tree(grid, input, true, 0x4000); check_tree(grid, working, false, 0x8000);
    std::cout << "PASS: " << checks << " checks; exhaustive flags, hierarchy boundaries, random transitions, bounds\n";
}
