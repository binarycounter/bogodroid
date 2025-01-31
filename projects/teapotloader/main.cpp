#include <execinfo.h>
#include <iostream>
#include <cstdlib>

#include "toml++/toml.hpp"
toml::table config;
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
// #include "symtables.h"
#include <csignal>
#include <unistd.h>
#include "ndk.h"
#include "asset_manager.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_hints.h>
#include "gles2.h"
#include "glad.h"
#include "glad_egl.h"


using namespace FakeJni;

extern DynLibFunction symtable_libc[];
extern DynLibFunction symtable_ndk[];
extern DynLibFunction symtable_gles2[];
extern DynLibFunction symtable_egl_sdl[];

extern SDL_Window *sdl_win;
extern SDL_GLContext sdl_ctx;
extern EGLDisplay egl_display;
extern EGLContext egl_context;
extern EGLSurface egl_surface;

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

void printContextInfo() {
    // Print OpenGL information
    const char* glVersion = (const char*)glad_glGetString(GL_VERSION);
    const char* glVendor = (const char*)glad_glGetString(GL_VENDOR);
    const char* glRenderer = (const char*)glad_glGetString(GL_RENDERER);
    const char* glExtensions = (const char*)glad_glGetString(GL_EXTENSIONS);

    if (glVersion) {
        printf("OpenGL Version: %s\n", glVersion);
    } else {
        printf("Failed to retrieve OpenGL version.\n");
    }

    if (glVendor) {
        printf("OpenGL Vendor: %s\n", glVendor);
    } else {
        printf("Failed to retrieve OpenGL vendor.\n");
    }

    if (glRenderer) {
        printf("OpenGL Renderer: %s\n", glRenderer);
    } else {
        printf("Failed to retrieve OpenGL renderer.\n");
    }

    if (glExtensions) {
        printf("OpenGL Extensions: %s\n", glExtensions);
    } else {
        printf("Failed to retrieve OpenGL extensions.\n");
    }
}

// Signal handler to handle segmentation faults
void signalHandler(int signal) {
    void *array[50];  // Array to store stack trace addresses
    size_t size;

    // Get the stack trace
    size = backtrace(array, 50);

    // Print the stack trace
    std::cerr << "Error: signal " << signal << ":\n";
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    // Exit the program
    exit(1);
}


bool load_so_from_file(so_module *mod, const char *filename, uintptr_t addr)
{
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open())
  {
    std::cerr << "Error opening file: " << filename << std::endl;
    return false;
  }
  // Determine the file size
  file.seekg(0, std::ios::end);
  std::streampos fileSize = file.tellg();
  file.seekg(0, std::ios::beg);
  char *buffer = new char[fileSize];
  file.read(buffer, fileSize);
  file.close();
  so_load(mod, "", addr, buffer, fileSize);
  return true;
}

void print_native_callbacks(ANativeActivity nActivity)
{
    // Print function pointers
    printf("onStart: %p\n", (void*)nActivity.callbacks->onStart);
    printf("onResume: %p\n", (void*)nActivity.callbacks->onResume);
    printf("onSaveInstanceState: %p\n", (void*)nActivity.callbacks->onSaveInstanceState);
    printf("onPause: %p\n", (void*)nActivity.callbacks->onPause);
    printf("onStop: %p\n", (void*)nActivity.callbacks->onStop);
    printf("onDestroy: %p\n", (void*)nActivity.callbacks->onDestroy);
    printf("onWindowFocusChanged: %p\n", (void*)nActivity.callbacks->onWindowFocusChanged);
    printf("onNativeWindowCreated: %p\n", (void*)nActivity.callbacks->onNativeWindowCreated);
    printf("onNativeWindowResized: %p\n", (void*)nActivity.callbacks->onNativeWindowResized);
    printf("onNativeWindowRedrawNeeded: %p\n", (void*)nActivity.callbacks->onNativeWindowRedrawNeeded);
    printf("onNativeWindowDestroyed: %p\n", (void*)nActivity.callbacks->onNativeWindowDestroyed);
    printf("onInputQueueCreated: %p\n", (void*)nActivity.callbacks->onInputQueueCreated);
    printf("onInputQueueDestroyed: %p\n", (void*)nActivity.callbacks->onInputQueueDestroyed);
    printf("onContentRectChanged: %p\n", (void*)nActivity.callbacks->onContentRectChanged);
    printf("onConfigurationChanged: %p\n", (void*)nActivity.callbacks->onConfigurationChanged);
    printf("onLowMemory: %p\n", (void*)nActivity.callbacks->onLowMemory);
}

int main(int argc, char *argv[])
{
    // Register the signal handler for SIGSEGV
    signal(SIGSEGV, signalHandler);


  config=toml::parse_file(argv[1]);
  auto game_path=config["paths"]["game_files"].value_or<std::string>("");
  if (chdir(game_path.c_str()) != 0)
  {
    std::cerr << "Could not change directory to " << game_path << std::endl;
    return 1;
  }
  else
  {
     std::cout << "Changed working directory to " << game_path << std::endl;
  }

// SDL_SetHint("SDL_VIDEO_X11_FORCE_EGL","1");


      // Initialize SDL with video, audio, joystick, and controller support
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

    printContextInfo();

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





  ANativeActivity nActivity;
  FakeJni::LocalFrame frame(vm);
  memset( &nActivity, 0, sizeof( ANativeActivity ) );
  nActivity.callbacks=new ANativeActivityCallbacks;
  memset( nActivity.callbacks, 0, sizeof( ANativeActivityCallbacks ) );
  nActivity.vm=&vm;
  nActivity.env=&(frame.getJniEnv());
  nActivity.clazz=std::make_shared<jnivm::com::sample::teapot::TeapotNativeActivity>();
  nActivity.assetManager=AAssetManager_create("assets");

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
