cmake_minimum_required(VERSION 3.18.0)

project(fast_tcp_socket_jsi)

# 1. React Native JSI und FBJNI via Prefab laden
find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)

# 2. Pfade zu node_modules berechnen
set(NODE_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../)
set(REACT_NATIVE_DIR ${NODE_MODULES_DIR}/react-native)

# 3. Includes manuell hinzufügen
# WICHTIG: Der dritte Pfad ist der, der bisher fehlte!
include_directories(
    # ReactCommon Basis
    ${REACT_NATIVE_DIR}/ReactCommon
    # CallInvoker Logic
    ${REACT_NATIVE_DIR}/ReactCommon/callinvoker/ReactCommon
    # CallInvokerHolder (Android JNI Wrapper) - HIER LIEGT DIE FEHLENDE DATEI
    ${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/react/turbomodule
    # FBJNI Headers
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
    ReactAndroid::jsi
    fbjni::fbjni      # Wichtig für CallInvokerHolder
    log
)