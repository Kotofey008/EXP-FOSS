#include "exploration/game.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
int main() {
    exploration::Game game;
    std::cout<<"Exploration reconstruction. Commands: move <forward> <strafe> <seconds>, look <yaw> <pitch>, jump, remove, place, select <id>, save <path>, load <path>, status, quit\n";
    std::string line;
    while(std::getline(std::cin,line)) {
        std::istringstream row(line);std::string command;row>>command;
        exploration::Input input;
        if(command=="quit")break;
        if(command=="move") {float seconds;if(row>>input.forward>>input.strafe>>seconds) {if(seconds>0&&seconds<=60)for(float t=0;t<seconds;t+=.016f)game.update(input,std::min(.016f,seconds-t));}}
        else if(command=="look") {if(row>>input.look_x>>input.look_y)game.update(input,.001f);}
        else if(command=="jump") {input.jump=true;game.update(input,.016f);}
        else if(command=="remove")std::cout<<"remove="<<game.remove_block()<<"\n";
        else if(command=="place")std::cout<<"place="<<game.place_block()<<"\n";
        else if(command=="select") {int block;if(row>>block&&block>0&&block<256)game.select(static_cast<unsigned char>(block));}
        else if(command=="save"||command=="load") {std::string path;row>>path;std::cout<<command<<"="<<(command=="save"?game.save(path):game.load(path))<<"\n";}
        const auto& p=game.player();std::cout<<"position="<<p.position.x<<","<<p.position.y<<","<<p.position.z<<" grounded="<<p.grounded<<" selected="<<unsigned(game.selected())<<" revision="<<game.revision()<<"\n";
    }
}
