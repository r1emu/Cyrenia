/**
 *
 *   ██████╗   ██╗ ███████╗ ███╗   ███╗ ██╗   ██╗
 *   ██╔══██╗ ███║ ██╔════╝ ████╗ ████║ ██║   ██║
 *   ██████╔╝ ╚██║ █████╗   ██╔████╔██║ ██║   ██║
 *   ██╔══██╗  ██║ ██╔══╝   ██║╚██╔╝██║ ██║   ██║
 *   ██║  ██║  ██║ ███████╗ ██║ ╚═╝ ██║ ╚██████╔╝
 *   ╚═╝  ╚═╝  ╚═╝ ╚══════╝ ╚═╝     ╚═╝  ╚═════╝
 *
 * @license GNU GENERAL PUBLIC LICENSE - Version 2, June 1991
 *          See LICENSE file for further information
 */

#pragma once

// Includes
#include <windows.h>
#include <pthread.h>
#include "RawPacket/RawPacket.h"
#include "Plugins/plugins.h"

typedef struct ProxyProxyParameters {
    char *captureFolder;
    SOCKET client;
    SOCKET server;
    PluginCallback *callbacks;
    size_t callbackCount;
    uint64_t packetId;
    HANDLE mutex;
    pthread_t hClientListener;
    pthread_t hServerListener;
}   ProxyParameters;

// Prototypes
int server_listen (int port, SOCKET *_server);
int server_accept (SOCKET server, SOCKET *_client);
void *listener (void *_params);
void *emitter (void *_params);
int client_connect (char *ip, int port, SOCKET *_server);
int plugin_load (char *pluginName, HMODULE *_plugin, PluginCallback *_callback);