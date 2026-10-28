#include "exploration/world.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace exploration {
World::World(int w,int h,int d):width_(w),height_(h),depth_(d) {
    if(w<=0||h<=0||d<=0) throw std::invalid_argument("world dimensions");
    const auto limit=blocks_.max_size();
    if(std::size_t(w)>limit/std::size_t(h)||std::size_t(w)*h>limit/std::size_t(d))
        throw std::length_error("world dimensions overflow");
    const std::size_t n=std::size_t(w)*h*d;
    blocks_.resize(n); states_.resize(n);
}
bool World::contains(int x,int y,int z) const {
    return x>=0&&y>=0&&z>=0&&x<width_&&y<height_&&z<depth_;
}
std::size_t World::index(int x,int y,int z) const {
    if(!contains(x,y,z)) throw std::out_of_range("world cell");
    return std::size_t(x)+std::size_t(y)*width_+std::size_t(z)*width_*height_;
}
std::uint8_t World::block(int x,int y,int z) const {
    return contains(x,y,z)?blocks_[index(x,y,z)]:0;
}
std::uint16_t World::state(int x,int y,int z) const { return states_[index(x,y,z)]; }
void World::set(int x,int y,int z,std::uint8_t b,std::uint16_t s) {
    const auto i=index(x,y,z); blocks_[i]=b; states_[i]=s;
}
// REA TerrainGeneratorFlat::Generate 0xfcde0, ELF VA 0xecde0.
// Surface height is this+4; grid height is grid+0x48 (confirmed in assembly).
void World::generate_flat(int surface) {
    if(surface<0||surface>height_) throw std::invalid_argument("flat surface height");
    for(int z=0;z<depth_;++z) for(int y=0;y<height_;++y) for(int x=0;x<width_;++x) {
        const std::uint8_t b=y>=surface?0:(y==surface-1?8:(y==surface-2?7:3));
        set(x,y,z,b,0);
    }
}
// Independent exposed-face renderer; original renderer equivalence is not established.
std::vector<Vertex> World::mesh() const {
    static const int normals[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    static const int corners[6][4][3]={
        {{1,0,0},{1,1,0},{1,1,1},{1,0,1}},
        {{0,0,1},{0,1,1},{0,1,0},{0,0,0}},
        {{0,1,0},{0,1,1},{1,1,1},{1,1,0}},
        {{0,0,1},{0,0,0},{1,0,0},{1,0,1}},
        {{1,0,1},{1,1,1},{0,1,1},{0,0,1}},
        {{0,0,0},{0,1,0},{1,1,0},{1,0,0}}};
    static const int triangles[6]={0,1,2,0,2,3};
    std::vector<Vertex> result;
    for(int z=0;z<depth_;++z) for(int y=0;y<height_;++y) for(int x=0;x<width_;++x) {
        auto b=block(x,y,z); if(!b) continue;
        for(int f=0;f<6;++f) {
            const auto& n=normals[f]; if(block(x+n[0],y+n[1],z+n[2])) continue;
            for(int k:triangles) {
                const auto& c=corners[f][k];
                result.push_back({{float(x+c[0]),float(y+c[1]),float(z+c[2])},
                                  {float(n[0]),float(n[1]),float(n[2])},b});
            }
        }
    }
    return result;
}
// Independent voxel DDA, not a transcription of GridData::Trace.
bool World::trace(Vec3 o,Vec3 d,float reach,Hit& hit) const {
    if(!std::isfinite(reach)||reach<0) return false;
    float p[3]={o.x,o.y,o.z},v[3]={d.x,d.y,d.z};
    const double length=std::sqrt(double(d.x)*d.x+double(d.y)*d.y+double(d.z)*d.z);
    if(!std::isfinite(length)||length==0) return false;
    int cell[3],previous[3],step[3]; double next[3],delta[3];
    for(int a=0;a<3;++a) {
        if(!std::isfinite(p[a])||double(p[a])<std::numeric_limits<int>::min()+1.0||
           double(p[a])>std::numeric_limits<int>::max()-1.0) return false;
        cell[a]=previous[a]=int(std::floor(p[a]));
        const double direction=v[a]/length;
        step[a]=(direction>0)-(direction<0);
        delta[a]=step[a]?std::abs(1/direction):std::numeric_limits<double>::infinity();
        next[a]=step[a]>0?(cell[a]+1.0-p[a])*delta[a]:
                (step[a]<0?(p[a]-cell[a])*delta[a]:delta[a]);
    }
    double t=0;
    while(t<=reach) {
        if(block(cell[0],cell[1],cell[2])) {
            hit={cell[0],cell[1],cell[2],previous[0],previous[1],previous[2],float(t)};
            return true;
        }
        int a=next[0]<=next[1]?(next[0]<=next[2]?0:2):(next[1]<=next[2]?1:2);
        t=next[a]; if(t>reach) break;
        for(int j=0;j<3;++j) previous[j]=cell[j];
        if((step[a]>0&&cell[a]==std::numeric_limits<int>::max())||
           (step[a]<0&&cell[a]==std::numeric_limits<int>::min())) return false;
        cell[a]+=step[a]; next[a]+=delta[a];
    }
    return false;
}
}
