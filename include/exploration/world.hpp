#pragma once
#include <cstdint>
#include <vector>
namespace exploration {
struct Vec3 { float x,y,z; };
struct Vertex { Vec3 position; Vec3 normal; std::uint8_t block; };
struct Hit { int x,y,z,previous_x,previous_y,previous_z; float distance; };
// Dense storage follows GridData's x + y*width + z*width*height layout.
class World {
public:
    World(int width,int height,int depth);
    int width() const { return width_; }
    int height() const { return height_; }
    int depth() const { return depth_; }
    bool contains(int x,int y,int z) const;
    std::uint8_t block(int x,int y,int z) const;
    void set(int x,int y,int z,std::uint8_t block,std::uint16_t state=0);
    std::uint16_t state(int x,int y,int z) const;
    void generate_flat(int surface_height);
    std::vector<Vertex> mesh() const;
    bool trace(Vec3 origin,Vec3 direction,float reach,Hit& hit) const;
private:
    std::size_t index(int x,int y,int z) const;
    int width_,height_,depth_;
    std::vector<std::uint8_t> blocks_;
    std::vector<std::uint16_t> states_;
};
}
