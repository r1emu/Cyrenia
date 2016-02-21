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

#include "RawPacket/RawPacket.h"

// Types definition
typedef int (*PluginCallback)(RawPacket *packet, void *plugin_data);
typedef int (*PluginInit)(void **plugin_data);
typedef int (*PluginUnload)(void *plugin_data);
typedef int (*PluginExit)(void *plugin_data);

typedef struct {
    char *name;
    PluginCallback call;
    PluginUnload unload;
    PluginExit exit;
    void *param;
}   Plugin;

typedef struct {
    size_t count;
    Plugin *p;
}   Plugins;
