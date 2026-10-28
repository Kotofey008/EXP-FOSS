#include "exploration/grid_state.hpp"
#include <stdexcept>
namespace exploration {
GridState::GridState(std::size_t cells)
{
    // Original index is a signed 32-bit integer; validate before allocation.
    if (cells > 0x7fffffffU) throw std::length_error("grid exceeds original index range");
    baseline_.assign(cells, 0);
    working_.assign(cells, 0);
    requested_.assign(cells, 0);
    for (unsigned level = 0; level < 4; ++level) {
        const std::size_t group_size = std::size_t(1) << (4 * (level + 1));
        const auto groups = cells / group_size + (cells % group_size != 0);
        working_tree_[level].assign(groups, 0);
        requested_tree_[level].assign(groups, 0);
    }
}
void GridState::update(Tree& tree, std::size_t index, bool enabled) {
    for (unsigned level = 0; level < 4; ++level) {
        auto& counter = tree[level][index >> (4 * (level + 1))];
        const bool was_nonzero = counter != 0;
        counter = static_cast<std::uint8_t>(enabled ? counter + 1 : counter - 1);
        const bool is_nonzero = counter != 0;
        if (was_nonzero == is_nonzero) break;
        enabled = is_nonzero;
    }
}
void GridState::set_state(std::size_t index, std::uint16_t value) {
    const auto old = working_.at(index); // bounds check before mutation
    const auto next = static_cast<std::uint16_t>((baseline_[index] & 0x4000U) | (value & 0xbfffU));
    if ((next ^ old) & 0x8000U) update(working_tree_, index, (next & 0x8000U) != 0);
    working_[index] = next;
    if ((value ^ requested_[index]) & 0x4000U)
        update(requested_tree_, index, (value & 0x4000U) != 0);
    requested_[index] = value;
}
std::uint16_t GridState::baseline(std::size_t index) const { return baseline_.at(index); }
std::uint16_t GridState::working(std::size_t index) const { return working_.at(index); }
std::uint16_t GridState::requested(std::size_t index) const { return requested_.at(index); }
std::uint8_t GridState::count(bool requested_tree, unsigned level, std::size_t group) const {
    return (requested_tree ? requested_tree_ : working_tree_).at(level).at(group);
}
}
