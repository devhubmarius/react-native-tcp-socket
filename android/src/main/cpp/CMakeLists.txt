cmake_minimum_required(VERSION 3.18.0)

# Name deiner JSI Library
project(fast_tcp_socket_jsi)

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