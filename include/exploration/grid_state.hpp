#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace exploration {
// Independent storage: not ABI-compatible with the original GridData object.
class GridState {
public:
    explicit GridState(std::size_t cells);
    void set_state(std::size_t index, std::uint16_t value);
    std::uint16_t baseline(std::size_t index) const;
    std::uint16_t working(std::size_t index) const;
    std::uint16_t requested(std::size_t index) const;
    std::uint8_t count(bool requested_tree, unsigned level, std::size_t group) const;
    std::size_t size() const noexcept { return baseline_.size(); }
private:
    using Tree = std::array<std::vector<std::uint8_t>, 4>;
    static void update(Tree& tree, std::size_t index, bool enabled);
    std::vector<std::uint16_t> baseline_, working_, requested_;
    Tree working_tree_, requested_tree_;
};
}
