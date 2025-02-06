#include "baron/baron.h"
#include "android.h"
#include "javac.h"
#include "unity.h"
#include "jnibridge.h"

void InitJNIBinding(FakeJni::Jvm *vm)
{

    InitJNIJavaClasses(vm);
    InitJNIAndroidClasses(vm);

    // vm->registerClass<jnivm::java::util::NoSuchElementException>();
    // vm->registerClass<jnivm::com::unity3d::player::NativeLoader>();
    // vm->registerClass<jnivm::com::unity3d::player::UnityPlayer>();
    // vm->registerClass<jnivm::com::unity3d::player::GoogleARCoreApi>();
    // vm->registerClass<jnivm::com::unity3d::player::Camera2Wrapper>();
    // vm->registerClass<jnivm::com::unity3d::player::HFPStatus>();
    // vm->registerClass<jnivm::com::unity3d::player::AudioVolumeHandler>();
    // vm->registerClass<jnivm::com::unity3d::player::UnityCoreAssetPacksStatusCallbacks>();
    // vm->registerClass<jnivm::com::unity3d::player::OrientationLockListener>();
    // vm->registerClass<jnivm::com::google::androidgamesdk::ChoreographerCallback>();
    // vm->registerClass<jnivm::com::google::androidgamesdk::SwappyDisplayManager>();


    vm->registerClass<jnivm::com::unity3d::player::PlayAssetDeliveryUnityWrapper>();
    vm->registerClass<jnivm::com::unity3d::player::UnityPlayerActivity>();
    vm->registerClass<jnivm::bitter::jnibridge::JNIBridge>();
    


    HookStringExtensions(vm);
    HookClassExtensions(vm);
    HookObjectExtensions(vm);

    FakeJni::LocalFrame frame(*vm);
    auto classClass = vm->findClass("java/lang/Class");

    // Some Unity specific thing. Should really be in JNIBridge, is cast to Class for some reason... Just return false.
    classClass->HookInstanceFunction(&frame.getJniEnv(), "initializeGoogleAr", [](jnivm::ENV*env, jnivm::Object*self)
    {
        return false;
    });
}