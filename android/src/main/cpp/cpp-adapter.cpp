#include <jni.h>
#include <jsi/jsi.h>
#include <android/log.h>

using namespace facebook;

#define LOG_TAG "FastTcpSocketJSI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Globale Variablen, um Java von überall aufzurufen
JavaVM *java_vm = nullptr;
jobject java_module_ref = nullptr; // Globale Referenz auf TcpSocketModule
jmethodID method_jsiWrite = nullptr; // ID der Java-Methode

extern "C" JNIEXPORT void JNICALL
Java_com_asterinet_react_tcpsocket_TcpSocketModule_nativeInstall(
        JNIEnv *env,
        jobject thiz,
        jlong jsiPtr,
        jobject callInvokerHolder) {

    // 1. Java VM Pointer speichern (für Thread-Zugriff)
    env->GetJavaVM(&java_vm);

    // 2. Globale Referenz auf das Java-Objekt erstellen
    // Das verhindert, dass der Garbage Collector das Objekt löscht
    if (java_module_ref == nullptr) {
        java_module_ref = env->NewGlobalRef(thiz);
    }

    // 3. Die Java-Methode "jsiWrite" suchen
    jclass cls = env->GetObjectClass(thiz);
    // I = int (socketId), I = int (msgId), [B = byte[] (data) -> V = void
    method_jsiWrite = env->GetMethodID(cls, "jsiWrite", "(II[B)V");

    if (!method_jsiWrite) {
        LOGI("FEHLER: Methode 'jsiWrite' in Java nicht gefunden!");
        return;
    }

    auto runtime = reinterpret_cast<jsi::Runtime*>(jsiPtr);
    LOGI("JSI Bindings: Setup...");

    // --- JSI FUNKTION ---
    auto nativeWrite = jsi::Function::createFromHostFunction(
        *runtime,
        jsi::PropNameID::forAscii(*runtime, "nativeTcpWrite"),
        2, 
        [](jsi::Runtime& rt, const jsi::Value& thisValue, const jsi::Value* args, size_t count) -> jsi::Value {
            
            // DEBUG LOG 1: Funktion betreten
            LOGI("FastTcpSocketJSI: nativeTcpWrite aufgerufen. Argumente Anzahl: %zu", count);
            // Prüfen ob wir 3 Argumente haben (SocketID, MsgID, Data)
            if (count < 3) {
                LOGI("FastTcpSocketJSI: Zu wenig Argumente (Erwartet: socketId, msgId, buffer)");
                return jsi::Value::undefined();
            }

            // Argumente parsen
            double socketId = args[0].asNumber();
            double msgId = args[1].asNumber(); // Das ist neu
            
            jsi::Object dataObj = args[2].asObject(rt); // Jetzt an Index 2

            // DEBUG LOG 2: Prüfen ob es ein ArrayBuffer ist
            bool isBuffer = dataObj.isArrayBuffer(rt);

            if (!isBuffer) {
                LOGI("FastTcpSocketJSI: JSI Error: Arg 2 ist kein ArrayBuffer");
                return jsi::Value::undefined();
            }

            // Daten aus dem ArrayBuffer holen
            jsi::ArrayBuffer buffer = dataObj.getArrayBuffer(rt);
            size_t dataSize = buffer.size(rt);
            uint8_t* dataPtr = buffer.data(rt);

            LOGI("FastTcpSocketJSI: Datengröße: %zu Bytes. Bereite JNI vor...", dataSize);

            // --- JNI CALL START ---
            JNIEnv *env;
            // Da JSI auf einem anderen Thread laufen kann, müssen wir uns an die JVM hängen
            int getEnvStat = java_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
            bool didAttach = false;

            if (getEnvStat == JNI_EDETACHED) {
                java_vm->AttachCurrentThread(&env, nullptr);
                didAttach = true;
            }

            if (env && java_module_ref && method_jsiWrite) {
                // C++ Byte Array -> Java Byte Array umwandeln
                jbyteArray jData = env->NewByteArray(dataSize);
                env->SetByteArrayRegion(jData, 0, dataSize, (const jbyte*)dataPtr);

                LOGI("FastTcpSocketJSI: Rufe Java jsiWrite auf...");
                // AUFRUF AN JAVA: SocketID, MsgID, Data
                env->CallVoidMethod(java_module_ref, method_jsiWrite, (int)socketId, (int)msgId, jData);
                LOGI("FastTcpSocketJSI: Java Aufruf fertig.");
                // Speicher in Java freigeben (Local Ref)
                env->DeleteLocalRef(jData);
            }

            if (didAttach) {
                java_vm->DetachCurrentThread();
            }
            // --- JNI CALL ENDE ---

            return jsi::Value::undefined();
        }
    );

    runtime->global().setProperty(*runtime, "nativeTcpWrite", nativeWrite);
    LOGI("JSI Bindings: Fertig installiert.");
}