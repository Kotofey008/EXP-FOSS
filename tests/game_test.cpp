#include "exploration/game.hpp"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace exploration;
void check(bool v,const char* message) {if(!v)throw std::runtime_error(message);}
int main() {
 try {
    Game game;Input neutral;
    check(!game.collides(game.player().position),"spawn inside terrain");
    for(int i=0;i<120;++i)game.update(neutral,1.f/60);
    check(game.player().grounded,"not grounded");
    const float initial_y=game.player().position.y;
    Input jump;jump.jump=true;game.update(jump,.016f);
    check(game.player().position.y>initial_y,"jump");
    for(int i=0;i<180;++i)game.update(neutral,1.f/60);
    check(game.player().grounded,"landing");
    Input walk;walk.forward=1;const float z=game.player().position.z;
    for(int i=0;i<60;++i)game.update(walk,1.f/60);
    check(game.player().position.z<z-2,"movement");
    game.player().pitch=-1.5f;
    Hit hit;check(game.target(hit),"ray target");
    check(game.remove_block(),"destroy");check(game.world().block(hit.x,hit.y,hit.z)==0,"destroy storage");
    game.select(4);check(game.place_block(),"place");check(game.world().block(hit.x,hit.y,hit.z)==4,"place storage");
    const char* path="game_test.exrw";check(game.save(path),"save");
    Game restored;check(restored.load(path),"load");
    check(restored.world().block(hit.x,hit.y,hit.z)==4&&restored.selected()==4,"roundtrip blocks");
    check(restored.player().position.z==game.player().position.z,"roundtrip player");
    {std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);file.seekp(50);file.put('X');}
    const float before=restored.player().position.z;
    check(!restored.load(path),"corruption accepted");check(restored.player().position.z==before,"failed load mutated game");
    std::remove(path);
    check(!game.save("/nonexistent-exploration-dir/world"),"invalid save succeeded");
    std::cout<<"PASS spawn, gravity, jump, landing, movement, targeting, destruction, placement, persistence, corruption rejection\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
