package com.asterinet.react.tcpsocket;

import java.util.concurrent.ConcurrentHashMap;

import com.asterinet.react.tcpsocket.TcpSocket;

/**
 * Global registry for TCP sockets that can be accessed by both
 * the old TcpSocketModule and the new Turbo Module
 */
public class TcpSocketRegistry {
    private static final TcpSocketRegistry INSTANCE = new TcpSocketRegistry();
    private final ConcurrentHashMap<Integer, TcpSocket> socketMap = new ConcurrentHashMap<>();

    private TcpSocketRegistry() {
        // Private constructor for singleton
    }

    public static TcpSocketRegistry getInstance() {
        return INSTANCE;
    }

    public void put(int id, TcpSocket socket) {
        socketMap.put(id, socket);
    }

    public TcpSocket get(int id) {
        return socketMap.get(id);
    }

    public void remove(int id) {
        socketMap.remove(id);
    }
}