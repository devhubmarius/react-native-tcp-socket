cmake_minimum_required(VERSION 3.18.0)

# Name deiner JSI Library
project(fast_tcp_socket_jsi)


# Wir suchen das Root-Verzeichnis von React Native
# ../../../.. geht vom cpp ordner zurück:
# cpp -> main -> src -> android -> (modul root) -> node_modules
set(NODE_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../node_modules)
set(REACT_NATIVE_DIR ${NODE_MODULES_DIR}/react-native)

# Includes für JSI und React Native Internals
include_directories(
    ${REACT_NATIVE_DIR}/ReactCommon/jsi
    ${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/first-party/fbjni/headers
    ${REACT_NATIVE_DIR}/ReactCommon
    ${REACT_NATIVE_DIR}/ReactCommon/callinvoker
)

# React Native benötigt prefab für neuere Versionen,
# aber für JSI Zugriff brauchen wir oft die Standard-Includes.
# Wir halten es erst einmal simpel.

add_library(
        fast_tcp_socket_jsi
        SHARED
        # Hier kommen später unsere C++ Files rein.
        # Fürs erste erstellen wir gleich eine Dummy-Datei.
        cpp-adapter.cpp
)

# Linken gegen die Standard Android Libraries und React Native JSI
find_library(
        log-lib
        log
)

target_link_libraries(
        fast_tcp_socket_jsi
        ${log-lib}
        # Wir benötigen später hier fbjni, jsi, etc.
        # Das fügen wir hinzu, wenn wir die Dependencies konfiguriert haben.
)