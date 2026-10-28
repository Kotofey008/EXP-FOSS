#include "exploration/game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
namespace exploration {
namespace {
float approach(float a,float b,float d) { return a<b?std::min(a+d,b):std::max(a-d,b); }
bool finite(Vec3 v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
void u32(std::vector<unsigned char>& b,std::uint32_t v) {
    for(unsigned n=0;n<4;++n) b.push_back(static_cast<unsigned char>(v>>(8*n)));
}
std::uint32_t read32(const std::vector<unsigned char>& b,std::size_t p) {
    std::uint32_t v=0; for(unsigned n=0;n<4;++n)v|=std::uint32_t(b.at(p+n))<<(8*n); return v;
}
std::uint32_t checksum(const std::vector<unsigned char>& b,std::size_t n) {
    std::uint32_t h=2166136261U; for(std::size_t i=0;i<n;++i)h=(h^b[i])*16777619U; return h;
}
}
Vec3 Player::direction() const {
    const float c=std::cos(pitch); return {std::sin(yaw)*c,std::sin(pitch),-std::cos(yaw)*c};
}
Game::Game():world_(32,48,32) { world_.generate_flat(17); }
bool Game::collides(Vec3 p) const {
    if(!finite(p)) return true;
    if(p.x<.25f||p.z<.25f||p.x>world_.width()-.25f||p.z>world_.depth()-.25f||p.y<1.5f||p.y>world_.height()-.25f)return true;
    for(int z=int(std::floor(p.z-.2499f));z<=int(std::floor(p.z+.2499f));++z)
      for(int y=int(std::floor(p.y-1.4999f));y<=int(std::floor(p.y+.2499f));++y)
        for(int x=int(std::floor(p.x-.2499f));x<=int(std::floor(p.x+.2499f));++x)
          if(world_.block(x,y,z))return true;
    return false;
}
void Game::move_axis(unsigned axis,float delta) {
    if(delta==0)return;
    const int count=std::max(1,int(std::ceil(std::fabs(delta)/.062625f)));
    float increment=delta/count;
    for(int n=0;n<count;++n) {
        Vec3 p=player_.position;
        if(axis==0)p.x+=increment; else if(axis==1)p.y+=increment; else p.z+=increment;
        if(!collides(p)) { player_.position=p; continue; }
        if(axis!=1 && player_.grounded) {
            Vec3 raised=p; raised.y+=.501f;
            Vec3 start=player_.position; start.y+=.501f;
            if(!collides(start)&&!collides(raised)) { player_.position=raised; continue; }
        }
        if(axis==0) player_.velocity.x=0;
        else if(axis==2)player_.velocity.z=0;
        else {player_.velocity.y=0; if(delta<0)player_.grounded=true;}
        break;
    }
}
void Game::step(const Input& in,float dt) {
    float f=in.forward,s=in.strafe,mag=std::sqrt(f*f+s*s);
    if(mag>1) {f/=mag;s/=mag;}
    const float sn=std::sin(player_.yaw),cs=std::cos(player_.yaw);
    const float acceleration=mag>0?9.f:18.f;
    player_.velocity.x=approach(player_.velocity.x,3.f*(sn*f+cs*s),acceleration*dt);
    player_.velocity.z=approach(player_.velocity.z,3.f*(-cs*f+sn*s),acceleration*dt);
    if(in.jump&&player_.grounded) {player_.velocity.y=5;player_.grounded=false;}
    player_.velocity.y=std::max(-50.f,player_.velocity.y-9.81f*dt);
    move_axis(0,player_.velocity.x*dt); move_axis(2,player_.velocity.z*dt);
    player_.grounded=false; move_axis(1,player_.velocity.y*dt);
}
void Game::update(const Input& in,float dt) {
    if(!std::isfinite(dt)||dt<=0||!std::isfinite(in.forward)||!std::isfinite(in.strafe)||!std::isfinite(in.look_x)||!std::isfinite(in.look_y))return;
    player_.yaw=std::remainder(player_.yaw+in.look_x,6.28318530718f);
    player_.pitch=std::max(-1.55334f,std::min(1.55334f,player_.pitch+in.look_y));
    dt=std::min(dt,.2f); const int n=std::max(1,int(std::ceil(dt/.008f)));
    for(int i=0;i<n;++i)step(in,dt/n);
    if(in.remove)remove_block();
    if(in.place)place_block();
}
bool Game::target(Hit& h) const {return world_.trace(player_.position,player_.direction(),6.f,h);}
bool Game::remove_block() {
    Hit h; if(!target(h)||h.y==0)return false;
    world_.set(h.x,h.y,h.z,0); ++revision_; return true;
}
bool Game::place_block() {
    Hit h; if(!target(h))return false;
    int x=h.previous_x,y=h.previous_y,z=h.previous_z;
    if(!world_.contains(x,y,z)||world_.block(x,y,z))return false;
    world_.set(x,y,z,selected_);
    if(collides(player_.position)) {world_.set(x,y,z,0);return false;}
    ++revision_; return true;
}
bool Game::save(const std::string& path) const {
    // Independent, versioned format. NOT compatible with original saves.
    std::vector<unsigned char> b={'E','X','R','W'};
    u32(b,1);u32(b,world_.width());u32(b,world_.height());u32(b,world_.depth());
    const float state[]={player_.position.x,player_.position.y,player_.position.z,player_.yaw,player_.pitch};
    for(float v:state) {std::uint32_t bits;std::memcpy(&bits,&v,4);u32(b,bits);}
    u32(b,selected_);
    for(int z=0;z<world_.depth();++z)for(int y=0;y<world_.height();++y)for(int x=0;x<world_.width();++x)b.push_back(world_.block(x,y,z));
    u32(b,checksum(b,b.size()));
    const std::string temp=path+".tmp";
    std::ofstream out(temp,std::ios::binary|std::ios::trunc);
    if(!out)return false;
    out.write(reinterpret_cast<const char*>(b.data()),b.size());out.flush();
    bool ok=bool(out);out.close();ok=ok&&!out.fail();
    if(!ok) {std::remove(temp.c_str());return false;}
    if(std::rename(temp.c_str(),path.c_str())!=0) {std::remove(temp.c_str());return false;}
    return true;
}
bool Game::load(const std::string& path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)return false;
    const std::size_t cells=std::size_t(world_.width())*world_.height()*world_.depth();
    if(in.tellg()!=std::streamoff(48+cells))return false;
    std::vector<unsigned char>b(48+cells);in.seekg(0);in.read(reinterpret_cast<char*>(b.data()),b.size());
    if(!in||std::memcmp(b.data(),"EXRW",4)||read32(b,4)!=1||read32(b,8)!=unsigned(world_.width())||read32(b,12)!=unsigned(world_.height())||read32(b,16)!=unsigned(world_.depth())||read32(b,b.size()-4)!=checksum(b,b.size()-4))return false;
    float values[5];for(unsigned i=0;i<5;++i){auto bits=read32(b,20+i*4);std::memcpy(values+i,&bits,4);if(!std::isfinite(values[i]))return false;}
    if(read32(b,40)==0||read32(b,40)>255||std::fabs(values[4])>1.55334f)return false;
    Game candidate;
    std::size_t p=44;
    for(int z=0;z<world_.depth();++z)for(int y=0;y<world_.height();++y)for(int x=0;x<world_.width();++x)candidate.world_.set(x,y,z,b[p++]);
    candidate.player_.position={values[0],values[1],values[2]};candidate.player_.yaw=std::remainder(values[3],6.28318530718f);candidate.player_.pitch=values[4];
    if(candidate.collides(candidate.player_.position))return false;
    world_=std::move(candidate.world_);player_=candidate.player_;selected_=static_cast<std::uint8_t>(read32(b,40));++revision_;return true;
}
}
