#include <jni.h>
#include <jsi/jsi.h>
#include <android/log.h>
// Da wir die Ordner direkt in CMake eingebunden haben:
#include <CallInvoker.h>       // Liegt in ReactCommon/callinvoker
#include <CallInvokerHolder.h> // Liegt in ReactAndroid/.../turbomodule
#include <fbjni/fbjni.h>       // Notwendig für JNI Wrapper
#include <map>
#include <memory>

using namespace facebook;
using namespace facebook::react; // Namespace für CallInvoker

#define LOG_TAG "FastTcpSocketJSI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Globale Variablen, um Java von überall aufzurufen
JavaVM *java_vm = nullptr;
jobject java_module_ref = nullptr; // Globale Referenz auf TcpSocketModule
jmethodID method_jsiWrite = nullptr; // ID der Java-Methode
jmethodID method_setJsiEnabled = nullptr;

// NEU: CallInvoker und Listener Map
std::shared_ptr<CallInvoker> jsCallInvoker;
// Wir speichern die JS-Funktionen persistent. 
// Key: SocketID, Value: JSI Funktion
std::map<int, std::shared_ptr<jsi::Function>> listeners;

// Global oben hinzufügen:
jsi::Runtime* globalRuntime = nullptr; 

extern "C" JNIEXPORT void JNICALL
Java_com_asterinet_react_tcpsocket_TcpSocketModule_nativeInstall(
        JNIEnv *env,
        jobject thiz,
        jlong jsiPtr,
        jobject callInvokerHolderJavaObj) {

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

    // === NEU: CallInvoker extrahieren ===
    if (callInvokerHolderJavaObj != nullptr) {
        auto holder = jni::alias_ref<CallInvokerHolder::javaobject>(
                reinterpret_cast<CallInvokerHolder::javaobject>(callInvokerHolderJavaObj)
        );
        jsCallInvoker = holder->cthis()->getCallInvoker();
    } else {
        LOGI("FATAL: CallInvokerHolder ist null!");
    }
    // ====================================

    method_setJsiEnabled = env->GetMethodID(cls, "setJsiEnabled", "(I)V");
    if (!method_setJsiEnabled) {
        LOGI("FEHLER: Methode 'setJsiEnabled' in Java nicht gefunden!");
        return;
    }

    auto runtime = reinterpret_cast<jsi::Runtime*>(jsiPtr);
    globalRuntime = runtime;
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

    // 2. Die JSI Funktion definieren
    auto nativeOnData = jsi::Function::createFromHostFunction(
        *runtime,
        jsi::PropNameID::forAscii(*runtime, "nativeTcpOnData"),
        2, // Erwartet 2 Argumente: socketId, callback
        [](jsi::Runtime& rt, const jsi::Value& thisValue, const jsi::Value* args, size_t count) -> jsi::Value {
            
            // Validierung
            if (count < 2 || !args[0].isNumber() || !args[1].isObject() || !args[1].asObject(rt).isFunction(rt)) {
                LOGI("JSI Error: nativeTcpOnData erwartet (socketId: number, callback: function)");
                return jsi::Value::undefined();
            }

            int socketId = (int)args[0].asNumber();
            jsi::Function callback = args[1].asObject(rt).asFunction(rt);

            // A) Callback in C++ speichern (damit wir ihn später aufrufen können)
            listeners[socketId] = std::make_shared<jsi::Function>(std::move(callback));
            
            LOGI("JSI: Listener für Socket %d registriert.", socketId);

            // B) JAVA INFORMIEREN: "Bitte sende Daten für diesen Socket an JSI!"
            // Hier rufen wir setJsiEnabled(socketId) auf.
            
            JNIEnv *env;
            int getEnvStat = java_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
            bool didAttach = false;
            if (getEnvStat == JNI_EDETACHED) {
                java_vm->AttachCurrentThread(&env, nullptr);
                didAttach = true;
            }

            if (env && java_module_ref && method_setJsiEnabled) {
                // DER ENTSCHEIDENDE AUFRUF:
                env->CallVoidMethod(java_module_ref, method_setJsiEnabled, (int)socketId);
            }

            if (didAttach) {
                java_vm->DetachCurrentThread();
            }

            return jsi::Value::undefined();
        }
    );
    runtime->global().setProperty(*runtime, "nativeTcpOnData", nativeOnData);
    LOGI("JSI Bindings: Fertig installiert.");
}

**
 * Diese Funktion wird von Java aufgerufen, wenn Daten ankommen.
 * Java --> C++ (JNI Thread) --> C++ (JS Thread) --> JS
 */
extern "C" JNIEXPORT void JNICALL
Java_com_asterinet_react_tcpsocket_TcpSocketModule_nativeEmitJsiData(
        JNIEnv *env,
        jobject thiz,
        jint socketId,
        jbyteArray data) {

    if (!jsCallInvoker) {
        LOGI("Error: CallInvoker fehlt!");
        return;
    }

    // 1. Daten Kopieren (JNI Array -> C++ Vector)
    // Wir müssen die Daten kopieren, da wir den JNI Pointer nicht
    // in den anderen Thread mitnehmen dürfen (er ist Thread-lokal).
    jsize len = env->GetArrayLength(data);
    std::vector<uint8_t> bytes(len);
    env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte*>(bytes.data()));

    // 2. Thread-Wechsel: Aufgabe an den JS Thread übergeben
    jsCallInvoker->invokeAsync([socketId, bytes = std::move(bytes)]() {
        
        // --- AB HIER SIND WIR IM JS THREAD ---
        
        // Wir brauchen Zugriff auf die Runtime. Wie?
        // Trick: Wir haben keinen direkten globalen Pointer auf Runtime (unsafe).
        // ABER: listener functions sind an eine Runtime gebunden.
        
        // Prüfen ob Listener existiert
        auto it = listeners.find(socketId);
        if (it != listeners.end()) {
            try {
                // 1. ArrayBuffer erstellen
                jsi::Function& callback = *(it->second);
                
                // Buffer erstellen und Daten kopieren
                jsi::ArrayBuffer buffer = rt.arrayBuffer(bytes.size());
                // Zugriff auf Buffer-Daten
                // (Da ArrayBuffer memory managed ist, müssen wir kopieren)
                 memcpy(buffer.data(rt), bytes.data(), bytes.size());

                // 2. JS Callback aufrufen
                callback.call(rt, buffer);
            } catch (...) {
                LOGI("FastTcpSocketJSI: JSI Exception beim Empfangen von Daten!");
            }
        }
    });
}