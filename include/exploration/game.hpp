#pragma once
#include "world.hpp"
#include <string>
namespace exploration {
struct Input {
    float forward=0, strafe=0, look_x=0, look_y=0;
    bool jump=false, remove=false, place=false;
};
struct Player {
    // Eye position. Original SimplePlayer bounds: (-.25,-1.5,-.25)..(.25,.25,.25).
    Vec3 position{16.5f,19.5001f,16.5f}, velocity{0,0,0};
    float yaw=0, pitch=0;
    bool grounded=false;
    Vec3 direction() const;
};
class Game {
public:
    Game();
    void update(const Input& input,float seconds);
    bool remove_block();
    bool place_block();
    bool target(Hit& hit) const;
    bool save(const std::string& path) const;
    bool load(const std::string& path);
    bool collides(Vec3 eye) const;
    const World& world() const { return world_; }
    World& world() { return world_; }
    const Player& player() const { return player_; }
    Player& player() { return player_; }
    void select(std::uint8_t block) { if(block) selected_=block; }
    std::uint8_t selected() const { return selected_; }
    unsigned revision() const { return revision_; }
private:
    void step(const Input& input,float seconds);
    void move_axis(unsigned axis,float delta);
    World world_;
    Player player_;
    std::uint8_t selected_=1;
    unsigned revision_=0;
};
}
