cmake_minimum_required(VERSION 3.18.0)

project(fast_tcp_socket_jsi)

find_package(ReactAndroid REQUIRED CONFIG)
find_package(fbjni REQUIRED CONFIG)

set(NODE_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../)
set(REACT_NATIVE_DIR ${NODE_MODULES_DIR}/react-native)

include_directories(
    # 1. Basis ReactCommon
    ${REACT_NATIVE_DIR}/ReactCommon
    
    # 2. FIX FÜR DEN NEUEN FEHLER:
    # Wir fügen den Ordner hinzu, der "ReactCommon/CallInvoker.h" enthält.
    # Das ist: .../react-native/ReactCommon/callinvoker
    ${REACT_NATIVE_DIR}/ReactCommon/callinvoker
    
    # 3. FIX FÜR CallInvokerHolder:
    # Laut deinem Log liegt er in .../turbomodule/ReactCommon/CallInvokerHolder.h
    # Damit wir <CallInvokerHolder.h> finden, müssen wir in diesen Ordner:
    ${REACT_NATIVE_DIR}/ReactAndroid/src/main/jni/react/turbomodule/ReactCommon
)

add_library(
    fast_tcp_socket_jsi
    SHARED
    cpp-adapter.cpp
)

target_link_libraries(
    fast_tcp_socket_jsi
    ReactAndroid::jsi
    fbjni::fbjni
    log
)