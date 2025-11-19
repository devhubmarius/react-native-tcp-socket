#include <jni.h>
#include <jsi/jsi.h>
#include <android/log.h>
#include <string>

using namespace facebook;

// Hilfsmakro fürs Loggen in Android Logcat
#define LOG_TAG "FastTcpSocketJSI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Globale Referenz auf die Java-Umgebung speichern wir später hier (für Callbacks)
// JavaVM *java_vm;

/**
 * Die Funktion muss GENAU so heißen:
 * Java_<Package_Name_mit_Underscores>_<KlassenName>_<MethodenName>
 */
extern "C" JNIEXPORT void JNICALL
Java_com_asterinet_react_tcpsocket_TcpSocketModule_nativeInstall(
        JNIEnv *env,
jobject thiz,
        jlong jsiPtr,
jobject callInvokerHolder) {

// 1. JSI Runtime Pointer casten
auto runtime = reinterpret_cast<jsi::Runtime*>(jsiPtr);

if (!runtime) {
LOGI("Error: Runtime pointer is null!");
return;
}

LOGI("JSI Bindings werden installiert...");

// 2. Erstellen einer Test-Funktion in C++
// Diese Funktion wird später als 'global.nativeTcpWrite' in JS verfügbar sein.
auto nativeWrite = jsi::Function::createFromHostFunction(
        *runtime,
        jsi::PropNameID::forAscii(*runtime, "nativeTcpWrite"),
        2, // Anzahl der Argumente (z.B. socketId, byteArray)
        [](jsi::Runtime& rt, const jsi::Value& thisValue, const jsi::Value* args, size_t count) -> jsi::Value {

            // Test: Wir loggen einfach nur, was ankommt
            if (count > 0 && args[0].isNumber()) {
                double socketId = args[0].asNumber();
                LOGI("nativeTcpWrite aufgerufen für Socket ID: %f", socketId);
            } else {
                LOGI("nativeTcpWrite aufgerufen!");
            }

            // Hier wird später der Byte-Zugriff passieren.

            return jsi::Value::undefined();
        }
);

// 3. Die Funktion in das 'global' Objekt von JS injecten
runtime->global().setProperty(*runtime, "nativeTcpWrite", nativeWrite);

LOGI("JSI Bindings erfolgreich installiert!");
}