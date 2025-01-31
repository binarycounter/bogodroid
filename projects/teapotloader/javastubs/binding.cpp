#include "baron/baron.h"
#include "android.h"
#include "javac.h"
#include "teapot.h"


void InitJNIBinding(FakeJni::Jvm *vm)
{
    vm->registerClass<jnivm::java::lang::ClassLoader>();
    vm->registerClass<jnivm::java::lang::StringBuilder>();
    vm->registerClass<jnivm::java::io::InputStream>();
    vm->registerClass<jnivm::java::io::File>();
    vm->registerClass<jnivm::java::util::Map>();
    vm->registerClass<jnivm::java::util::Set>();
    vm->registerClass<jnivm::java::util::Iterator>();
    vm->registerClass<jnivm::java::util::Scanner>();

    vm->registerClass<jnivm::android::view::Display>();
    vm->registerClass<jnivm::android::hardware::display::DisplayManager>();
    vm->registerClass<jnivm::android::os::Environment>();
    vm->registerClass<jnivm::android::os::Looper>();
    vm->registerClass<jnivm::android::os::Handler>();
    vm->registerClass<jnivm::android::os::Process>();
    vm->registerClass<jnivm::android::os::Bundle>();
    vm->registerClass<jnivm::android::os::Build>();
    vm->registerClass<jnivm::android::os::BuildVersion>();
    vm->registerClass<jnivm::android::content::Context>();
    vm->registerClass<jnivm::android::app::Activity>();
    vm->registerClass<jnivm::android::app::NativeActivity>();
    vm->registerClass<jnivm::android::content::SharedPreferences>();
    vm->registerClass<jnivm::android::content::SharedPreferencesEditor>();
    vm->registerClass<jnivm::android::content::Intent>();
    vm->registerClass<jnivm::android::content::res::AssetManager>();
    vm->registerClass<jnivm::android::content::pm::PackageManager>();
    vm->registerClass<jnivm::android::content::pm::PackageInfo>();
    vm->registerClass<jnivm::android::content::pm::ApplicationInfo>();

    vm->registerClass<jnivm::com::sample::teapot::TeapotNativeActivity>();


    HookStringExtensions(vm);
    HookClassExtensions(vm);
    HookObjectExtensions(vm);
}