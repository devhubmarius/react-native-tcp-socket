cmake_minimum_required(VERSION 3.18.0)

project(fast_tcp_socket_jsi)

# Wir sagen CMake, es soll React Native (ReactAndroid) suchen.
# Das funktioniert dank "prefab true" automatisch in RN 0.71+ / Expo.
find_package(ReactAndroid REQUIRED CONFIG)

add_library(
    fast_tcp_socket_jsi
    SHARED
    cpp-adapter.cpp
)

# Wir linken gegen die JSI Library, die React Native bereitstellt.
# ReactAndroid::jsi beinhaltet automatisch auch die Include-Pfade zu jsi.h!
target_link_libraries(
    fast_tcp_socket_jsi
    ReactAndroid::jsi
    ReactAndroid::react_native_module
    # Falls du später Logs brauchst:
    log
)