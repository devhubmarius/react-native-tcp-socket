cmake_minimum_required(VERSION 3.18.0)

project(fast_tcp_socket_jsi)

# 1. Wir suchen JSI über React Native Standard (Prefab) - das hat funktioniert
find_package(ReactAndroid REQUIRED CONFIG)

# 2. Pfad zu node_modules berechnen
# Wir sind in: node_modules/react-native-tcp-socket/android/src/main/cpp
# Zurück zu: node_modules
set(NODE_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../)

# Falls die App in einem Monorepo ist oder node_modules woanders liegen,
# versuchen wir eine Alternative oder nehmen an, dass RN da ist.
# Wir definieren die Pfade zu den Headern, die Prefab oft versteckt:
set(REACT_NATIVE_DIR ${NODE_MODULES_DIR}/react-native)

# 3. Includes manuell hinzufügen (CallInvoker, FBJNI, ReactCommon)
include_directories(
    # CallInvoker Header
    ${REACT_NATIVE_DIR}/ReactCommon
    ${REACT_NATIVE_DIR}/ReactCommon/callinvoker
    # FBJNI Header (wird für CallInvokerHolder benötigt)
    ${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/first-party/fbjni/headers
)

add_library(
    fast_tcp_socket_jsi
    SHARED
    cpp-adapter.cpp
)

# 4. Linken
target_link_libraries(
    fast_tcp_socket_jsi
    # Wir nutzen JSI aus dem System (Prefab)
    ReactAndroid::jsi
    # Log
    log
)