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
#include "egl_sdl.h"
#include "debug_utils.h"

thread_local int tls0[2<<12] = {};
int foo() { return tls0[0]++; }

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
    symtable_egl_sdl,
    symtable_gles2,
    NULL
};

so_module *loaded_modules[32] = {
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
    exit_on_signals(); //Exits when CTRL-C is presset (or SIGINT or SIGTERM is received)

    if(argc<2)
    {
        fatal_error("Usage: %s <config file>\n",argv[0]);
        return -1;
    }

    // Init config, GLES pointers, JNI VN and bindings
    init_config(argv[1]);
    sdl_initialize_gles();
    Baron::Jvm vm;
    InitJNIBinding(&vm);

     printf("Loading libmain\n");
  so_module lmain = {};
  uintptr_t addr_lmain = 0x4000000000;
  const char *path_lmain = "lib/arm64-v8a/libmain.so";
  if (!load_so_from_file(&lmain, path_lmain, addr_lmain))
  {
    return 1;
  }


  printf("Loading libil2cpp\n");
  so_module lil2cpp = {};
  uintptr_t addr_lil2cpp = 0x3000000000;
  const char *path_lil2cpp = "lib/arm64-v8a/libil2cpp.so";
  if (!load_so_from_file(&lil2cpp, path_lil2cpp, addr_lil2cpp))
  {
    return 1;
  }

  printf("Loading libunity\n");
  so_module lunity = {};
  uintptr_t addr_lunity = 0x5000000000;
  const char *path_lunity = "lib/arm64-v8a/libunity.so";
  if (!load_so_from_file(&lunity, path_lunity, addr_lunity))
  {
    return 1;
  }

  // printf("Loading libburst\n");
  // so_module lburst = {};
  // uintptr_t addr_lburst = 0x7000000000;
  // const char *path_lburst = "lib/arm64-v8a/lib_burst_generated.so";
  // if (!load_so_from_file(&lburst, path_lburst, addr_lburst))
  // {
  //   return 1;
  // }

  loaded_modules[0]=&lmain;
  loaded_modules[1]=&lunity;
  loaded_modules[2]=&lil2cpp;
  // loaded_modules[3]=&lburst;


  printf("calling JNI_OnLoad from libmain.so\n");
  auto mainJNI_OnLoad = (jint(*)(JavaVM * vm, void *reserved))(so_symbol(&lmain, "JNI_OnLoad"));
  mainJNI_OnLoad(&vm, nullptr);

  JClass *nativeLoaderClass = vm.findClass("com/unity3d/player/NativeLoader").get();
  LocalFrame frame(vm);
  auto mainLoad = nativeLoaderClass->getMethod("(Ljava/lang/String;)Z", "load");

  printf("calling com/unity3d/player/NativeLoader/load from libmain.so\n");
  jvalue ret = mainLoad.invoke(frame.getJniEnv(), nativeLoaderClass, (JString) "lib/arm64-v8a");
  if (!ret.z)
  {
    printf("libmain.so:load returned false, game could not be loaded\n");
    return 1;
  }

  printf("calling JNI_OnLoad from libil2cpp.so\n");
  auto il2cppJNI_OnLoad = (jint(*)(JavaVM * vm, void *reserved))(so_symbol(&lil2cpp, "JNI_OnLoad"));
  std::cout << &il2cppJNI_OnLoad << std::endl;
  il2cppJNI_OnLoad(&vm, nullptr);
  
  printf("calling JNI_OnLoad from libunity.so\n");
  auto unityJNI_OnLoad = (jint(*)(JavaVM * vm, void *reserved))(so_symbol(&lunity, "JNI_OnLoad"));
  std::cout << &unityJNI_OnLoad << std::endl;
  unityJNI_OnLoad(&vm, nullptr);




  JClass *unityClass = vm.findClass("com/unity3d/player/UnityPlayer").get();

  auto unityInitJni = unityClass->getMethod("(Landroid/content/Context;)V", "initJni");
  printf("calling initJni from libunity.so\n");
  auto activity = std::make_shared<jnivm::android::app::Activity>();
  LocalFrame frame2(vm);
  unityInitJni.invoke(frame2.getJniEnv(), unityClass, activity);

  // vm.printStatistics();
  // return 0;

  auto unityNRecreateGfxState = unityClass->getMethod("(ILandroid/view/Surface;)V", "nativeRecreateGfxState");
  printf("calling nativeRecreateGfxState from libunity.so\n");
  auto surface = std::make_shared<jnivm::android::app::Activity>();
  LocalFrame frame3(vm);
  auto ret2 = unityNRecreateGfxState.invoke(frame3.getJniEnv(), unityClass, 0, surface);



  auto unityNRender = unityClass->getMethod("()Z", "nativeRender");
  printf("calling nativeRender from libunity.so\n");
  auto ret3 = unityNRender.invoke(frame3.getJniEnv(), unityClass);


    printf("NativeRender returned %d, Entering loop...\n",ret3.z);
    // The app has created new threads and is happily doing its thing, we just do nothing for now. Eventually, this will be a SDL based event loop for controller input.
    while(1)
        {
          printf(".");
          usleep(16666); //Roughly one 60hz frame
          auto ret4 = unityNRender.invoke(frame3.getJniEnv(), unityClass);
        }



    printf("Exit.\n");
    return 0;
}

