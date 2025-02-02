#include <execinfo.h>
#include <iostream>
#include <cstdlib>

#include "toml++/toml.hpp"
toml::table config;
#include "config.h"

#include <unistd.h>
#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <baron/baron.h>
#include "javastubs/binding.h"
#include "javastubs/teapot.h"
#include <fstream>
#include <fcntl.h>
#include <stdlib.h>
#include "platform.h"
#include "so_util.h"
#include "io_util.h"

#include "ndk.h"
#include "anative_activity.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_hints.h>
#include "gles2.h"
#include "glad.h"
#include "glad_egl.h"

#include "debug_utils.h"


using namespace FakeJni;



extern DynLibFunction symtable_libc[];
extern DynLibFunction symtable_ndk[];
extern DynLibFunction symtable_gles2[];
extern DynLibFunction symtable_egl_sdl[];

DynLibFunction *so_static_patches[32] = {
    NULL,
};

DynLibFunction *so_dynamic_libraries[32] = {
    symtable_libc,
    symtable_ndk,
    symtable_gles2,
    symtable_egl_sdl,
    NULL
};

extern SDL_Window *sdl_win;
extern SDL_GLContext sdl_ctx;
extern EGLDisplay egl_display;
extern EGLContext egl_context;
extern EGLSurface egl_surface;



int main(int argc, char *argv[])
{
  print_backtrace_on_segfault(); //Registers a signal handler to print backtrace on segfaults

    if(argc<2)
    {
        fatal_error("Usage: %s <config file>\n",argv[0]);
        return -1;
    }

    init_config(argv[1]);


    // Initialize SDL 
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fatal_error("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }



    sdl_win = SDL_CreateWindow("Teapot", 0, 0, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (sdl_win == NULL) {
        fatal_error("Failed to create SDL Window: %s\n", SDL_GetError());
        return -1;
    }

    // Basic OpenGL ES 2.x setup
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    sdl_ctx = SDL_GL_CreateContext(sdl_win);
    if (sdl_ctx == NULL) {
        fatal_error("Failed to create OpenGL Context: %s\n", SDL_GetError());
        return -1;
    }
    SDL_GL_MakeCurrent(sdl_win, sdl_ctx);

    // The libraries are loaded by SDL2.0, and the API entry points by the following
    // functions using the GLAD generated headers.
    load_egl_funcs();
    load_gles2_funcs();

    print_gl_info();

    SDL_Quit();

  Baron::Jvm vm;
  InitJNIBinding(&vm);


  printf("Loading libTeapot\n");
  so_module lmain = {};
  uintptr_t addr_lmain = 0x50000000;
  const char *path_lmain = "lib/arm64-v8a/libTeapotNativeActivity.so";
  if (!load_so_from_file(&lmain, path_lmain, addr_lmain))
  {
    printf("Failed to load libTeapot.\n");
    return 1;
  }


  ANativeActivity nActivity = ANativeActivity_create<jnivm::com::sample::teapot::TeapotNativeActivity>(&vm, "assets");

  printf("%p %p\n",&nActivity,nActivity.callbacks);
  printf("calling ANativeActivity_onCreate from libTeapot\n");
  auto mainOnCreate = (jint(*)(ANativeActivity* act, void* savedState, size_t savedStateSize))(so_symbol(&lmain, "ANativeActivity_onCreate")); 
    mainOnCreate(&nActivity, NULL, 0);

    print_native_callbacks(nActivity);

    ANativeWindow nWindow;
    memset( &nWindow, 0, sizeof( ANativeWindow ) );

    printf("calling ANativeActivity_onNativeWindowCreated from libTeapot\n");
    nActivity.callbacks->onNativeWindowCreated(&nActivity,&nWindow);

    printf("calling ANativeActivity_onWindowFocusChanged from libTeapot\n");
    nActivity.callbacks->onWindowFocusChanged(&nActivity,1);

    printf("calling ANativeActivity_onResume from libTeapot\n");
    nActivity.callbacks->onResume(&nActivity);

    // printf("calling ANativeActivity_onStart from libTeapot\n");
    // nActivity.callbacks->onStart(&nActivity);


    while(1)
        sleep(1);





  printf("Exit.\n");

  

  return 0;
}
