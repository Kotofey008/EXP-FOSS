#include "exploration/game.hpp"
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"
namespace {
using namespace exploration;
std::vector<unsigned char> asset(android_app* app,const char* name) {
    AAsset* a=AAssetManager_open(app->activity->assetManager,name,AASSET_MODE_BUFFER);
    if(!a)return {};
    std::vector<unsigned char> result(AAsset_getLength(a));
    int read=AAsset_read(a,result.data(),result.size()); AAsset_close(a);
    if(read!=static_cast<int>(result.size()))return {};
    return result;
}
struct DrawVertex {float x,y,z,u,v,r,g,b,a;};
struct Touch {int id=-1;float x=0,y=0;};
struct App {
    android_app* native;
    Game game;
    EGLDisplay display=EGL_NO_DISPLAY;
    EGLSurface surface=EGL_NO_SURFACE;
    EGLContext context=EGL_NO_CONTEXT;
    int width=1,height=1;
    GLuint texture=0;
    int tex_width=1,tex_height=1;
    int tiles[256][12]{};
    std::vector<DrawVertex> vertices;
    unsigned rendered_revision=~0U;
    Input input;
    Touch move,look;
    bool focused=false;
    bool asset_error=false;
    std::string save_path;
    explicit App(android_app* n):native(n) {
        const char* path=n->activity->internalDataPath;
        save_path=std::string(path?path:".")+"/world.exrw";
        game.load(save_path);
        auto table=asset(n,"scenes/blocks.txt");
        std::istringstream lines(std::string(table.begin(),table.end()));std::string line;
        while(std::getline(lines,line)) {
            std::istringstream row(line);int id,selection;
            if(!(row>>id>>selection)||id<0||id>255)continue;
            for(int& value:tiles[id])if(!(row>>value))break;
        }
    }
    void save() {if(!game.save(save_path))__android_log_print(ANDROID_LOG_ERROR,"Exploration","Save failed: %s",save_path.c_str());}
    void close() {
        if(display!=EGL_NO_DISPLAY) {
            if(context!=EGL_NO_CONTEXT&&surface!=EGL_NO_SURFACE) {eglMakeCurrent(display,surface,surface,context);if(texture)glDeleteTextures(1,&texture);}
            eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
            if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);
            if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);
            eglTerminate(display);
        }
        display=EGL_NO_DISPLAY;surface=EGL_NO_SURFACE;context=EGL_NO_CONTEXT;texture=0;
        input=Input{};move=Touch{};look=Touch{};
    }
    bool open() {
        close();display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if(display==EGL_NO_DISPLAY||!eglInitialize(display,nullptr,nullptr))return false;
        const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_DEPTH_SIZE,16,EGL_NONE};
        EGLConfig config;EGLint count=0;
        if(!eglChooseConfig(display,attributes,&config,1,&count)||count!=1)return false;
        EGLint format;eglGetConfigAttrib(display,config,EGL_NATIVE_VISUAL_ID,&format);
        ANativeWindow_setBuffersGeometry(native->window,0,0,format);
        surface=eglCreateWindowSurface(display,config,native->window,nullptr);
        const EGLint context_attributes[]={EGL_CONTEXT_CLIENT_VERSION,1,EGL_NONE};
        context=eglCreateContext(display,config,EGL_NO_CONTEXT,context_attributes);
        if(surface==EGL_NO_SURFACE||context==EGL_NO_CONTEXT||!eglMakeCurrent(display,surface,surface,context))return false;
        eglSwapInterval(display,1);
        auto bytes=asset(native,"textures/blocks.png");int channels;
        unsigned char* pixels=stbi_load_from_memory(bytes.data(),bytes.size(),&tex_width,&tex_height,&channels,4);
        if(!pixels) {asset_error=true;return false;}
        glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,tex_width,tex_height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);stbi_image_free(pixels);
        glEnable(GL_DEPTH_TEST);rendered_revision=~0U;return true;
    }
    void mesh() {
        if(rendered_revision==game.revision())return;
        vertices.clear();auto source=game.world().mesh();vertices.reserve(source.size());
        const int uv[6][2]={{0,0},{1,0},{1,1},{0,0},{1,1},{0,1}};
        for(std::size_t i=0;i<source.size();++i) {
            const auto& v=source[i];auto n=v.normal;
            const int face=n.x>0?0:n.y>0?1:n.z>0?2:n.x<0?3:n.y<0?4:5;
            const float u=(tiles[v.block][face*2]*16.f+.1f+uv[i%6][0]*15.8f)/tex_width;
            const float t=(tiles[v.block][face*2+1]*16.f+.1f+uv[i%6][1]*15.8f)/tex_height;
            const float light=n.y>0?1.f:n.y<0?.45f:n.x!=0?.75f:.6f;
            vertices.push_back({v.position.x,v.position.y,v.position.z,u,t,light,light,light,1});
        }
        rendered_revision=game.revision();
    }
    void draw() {
        eglQuerySurface(display,surface,EGL_WIDTH,&width);eglQuerySurface(display,surface,EGL_HEIGHT,&height);
        if(width<=0||height<=0)return;
        glViewport(0,0,width,height);glClearColor(.5f,.72f,.95f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);glLoadIdentity();float a=float(width)/height;
        glFrustumf(-a*.025f,a*.025f,-.025f,.025f,.025f,150);
        glMatrixMode(GL_MODELVIEW);glLoadIdentity();const auto& p=game.player();
        glRotatef(-p.pitch*57.2957795f,1,0,0);glRotatef(p.yaw*57.2957795f,0,1,0);glTranslatef(-p.position.x,-p.position.y,-p.position.z);
        mesh();glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);
        glEnableClientState(GL_VERTEX_ARRAY);glEnableClientState(GL_TEXTURE_COORD_ARRAY);glEnableClientState(GL_COLOR_ARRAY);
        if(!vertices.empty()) {
            glVertexPointer(3,GL_FLOAT,sizeof(DrawVertex),&vertices[0].x);
            glTexCoordPointer(2,GL_FLOAT,sizeof(DrawVertex),&vertices[0].u);
            glColorPointer(4,GL_FLOAT,sizeof(DrawVertex),&vertices[0].r);
            glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);glDisableClientState(GL_COLOR_ARRAY);glDisable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrthof(0,width,height,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
        const float cross[]={width*.5f-8,height*.5f,width*.5f+8,height*.5f,width*.5f,height*.5f-8,width*.5f,height*.5f+8};
        glColor4f(1,1,1,1);glVertexPointer(2,GL_FLOAT,0,cross);glDrawArrays(GL_LINES,0,4);
        // Persistent touch controls: movement pad, jump, destroy, place and block selector.
        const float marks[]={width*.15f-20,height*.7f,width*.15f+20,height*.7f,width*.15f,height*.7f-20,width*.15f,height*.7f+20,
            width*.55f,height*.87f,width*.55f,height*.98f,width*.75f,height*.87f,width*.75f,height*.98f,width*.9f,height*.87f,width*.9f,height*.98f,
            width*.4f,height*.03f,width*.6f,height*.03f};
        glColor4f(1,.8f,.2f,1);glVertexPointer(2,GL_FLOAT,0,marks);glDrawArrays(GL_LINES,0,12);
        glEnable(GL_DEPTH_TEST);glDisableClientState(GL_VERTEX_ARRAY);
        if(!eglSwapBuffers(display,surface)) {__android_log_print(ANDROID_LOG_ERROR,"Exploration","eglSwapBuffers: %x",eglGetError());close();}
    }
    int touch(AInputEvent* event) {
        if(AInputEvent_getType(event)!=AINPUT_EVENT_TYPE_MOTION)return 0;
        int action=AMotionEvent_getAction(event),masked=action&AMOTION_EVENT_ACTION_MASK;
        int index=(action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        int id=AMotionEvent_getPointerId(event,index);float x=AMotionEvent_getX(event,index),y=AMotionEvent_getY(event,index);
        if(masked==AMOTION_EVENT_ACTION_DOWN||masked==AMOTION_EVENT_ACTION_POINTER_DOWN) {
            if(y<height*.12f)game.select(game.selected()%16+1);
            else if(y>height*.85f&&x>width*.45f) {
                if(x<width*.65f)input.jump=true;
                else if(x<width*.82f)input.remove=true;
                else input.place=true;
            } else if(x<width*.35f&&move.id<0)move={id,x,y};
            else if(look.id<0)look={id,x,y};
        }
        if(masked==AMOTION_EVENT_ACTION_MOVE) {
            for(std::size_t i=0;i<AMotionEvent_getPointerCount(event);++i) {
                int pointer=AMotionEvent_getPointerId(event,i);float px=AMotionEvent_getX(event,i),py=AMotionEvent_getY(event,i);
                if(pointer==move.id) {input.strafe=std::max(-1.f,std::min(1.f,(px-move.x)/(width*.12f)));input.forward=std::max(-1.f,std::min(1.f,(move.y-py)/(height*.18f)));}
                if(pointer==look.id) {input.look_x+=(px-look.x)*.004f;input.look_y-=(py-look.y)*.004f;look.x=px;look.y=py;}
            }
        }
        if(masked==AMOTION_EVENT_ACTION_UP||masked==AMOTION_EVENT_ACTION_POINTER_UP||masked==AMOTION_EVENT_ACTION_CANCEL) {
            if(id==move.id||masked==AMOTION_EVENT_ACTION_CANCEL) {move.id=-1;input.forward=input.strafe=0;}
            if(id==look.id||masked==AMOTION_EVENT_ACTION_CANCEL)look.id=-1;
        }
        return 1;
    }
};
void command(android_app* native,int32_t cmd) {
    auto& a=*static_cast<App*>(native->userData);
    if(cmd==APP_CMD_INIT_WINDOW&&native->window&&!a.open()) {__android_log_print(ANDROID_LOG_ERROR,"Exploration","EGL/assets initialization failed");a.close();ANativeActivity_finish(native->activity);}
    if(cmd==APP_CMD_TERM_WINDOW)a.close();
    if(cmd==APP_CMD_GAINED_FOCUS)a.focused=true;
    if(cmd==APP_CMD_LOST_FOCUS||cmd==APP_CMD_PAUSE) {a.focused=false;a.input=Input{};a.save();}
    if(cmd==APP_CMD_SAVE_STATE)a.save();
}
int32_t input(android_app* native,AInputEvent* event) {
    if(AInputEvent_getType(event)==AINPUT_EVENT_TYPE_KEY&&AKeyEvent_getKeyCode(event)==AKEYCODE_BACK) {
        if(AKeyEvent_getAction(event)==AKEY_EVENT_ACTION_UP)ANativeActivity_finish(native->activity);
        return 1;
    }
    return static_cast<App*>(native->userData)->touch(event);
}
}
extern "C" void android_main(android_app* native) {
    App app(native);native->userData=&app;native->onAppCmd=command;native->onInputEvent=input;
    using Clock=std::chrono::steady_clock;auto previous=Clock::now();
    while(!native->destroyRequested) {
        int events;android_poll_source* source=nullptr;
        const bool active=app.display!=EGL_NO_DISPLAY&&app.focused;
        int ident=ALooper_pollOnce(active?0:-1,nullptr,&events,reinterpret_cast<void**>(&source));
        if(ident>=0&&source)source->process(native,source);
        if(native->destroyRequested)break;
        auto now=Clock::now();float dt=std::chrono::duration<float>(now-previous).count();previous=now;
        if(app.display!=EGL_NO_DISPLAY&&app.focused) {
            app.game.update(app.input,dt);app.input.look_x=app.input.look_y=0;app.input.jump=app.input.remove=app.input.place=false;app.draw();
        }
    }
    app.save();app.close();native->userData=nullptr;
}
