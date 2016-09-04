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
#include "RawPacket/RawPacket.h"
#include "Plugins/plugins.h"
#include "dbg/dbg.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <tlhelp32.h>
#include <windows.h>
#include <pthread.h>

// Macro helper
#define sizeof_array(a) (sizeof(a)/sizeof(a[0]))

// =========== Proxy types
typedef enum {
    PROXY_TYPE_BARRACK,
    PROXY_TYPE_ZONE,
    PROXY_TYPE_SOCIAL,
}   ProxyType;

typedef struct ProxyProxyParameters {
    char *captureFolder;
    SOCKET client;
    SOCKET server;
    Plugins *plugins;
    uint64_t packetId;
    HANDLE mutex;
    pthread_t hClientListener;
    pthread_t hServerListener;
    ProxyType proxyType;
}   ProxyParameters;

char *getProxyName (ProxyType type) {

    char *name [] = {
        [PROXY_TYPE_BARRACK] = "Barrack",
        [PROXY_TYPE_ZONE]    = "Zone",
        [PROXY_TYPE_SOCIAL]  = "Social"
    };

    return name[type];
}

ProxyType getProxyType (char *name) {

    struct ProxyNameType {
        char *name;
        ProxyType type;
    } assoc [] = {
        {"Barrack", PROXY_TYPE_BARRACK},
        {"Zone",    PROXY_TYPE_ZONE},
        {"Social",  PROXY_TYPE_SOCIAL},
    };

    for (int i = 0; i < sizeof_array (assoc); i++) {
        struct ProxyNameType *p = &assoc[i];
        if (stricmp (name, p->name) == 0) {
            return p->type;
        }
    }

    return -1;
}

// =========== Proxy connection
int serverListen (int port, SOCKET *_server)
{
    WSADATA wsa;
    WSAStartup (MAKEWORD (2, 0), &wsa);

    SOCKET server = 0;
    SOCKADDR_IN server_context;
    SOCKADDR_IN csin;
    size_t csin_size = sizeof(csin);

    if ((server = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        error ("Invalid socket.");
        return -1;
    }

    server_context.sin_family      = AF_INET;
    server_context.sin_addr.s_addr = htonl (INADDR_ANY);
    server_context.sin_port        = htons (port);

    if (bind (server, (SOCKADDR*) &server_context, csin_size) == SOCKET_ERROR) {
        error ("Cannot bind port %d. Reason : %d", port, (int) GetLastError ());
        return -1;
    }

    if (listen (server, 1000) == SOCKET_ERROR) {
        error ("Cannot listen. Reason : %d", (int) GetLastError ());
        return -1;
    }

    *_server = server;
    return 0;
}

int serverAccept (SOCKET server, SOCKET *_client) 
{
    SOCKADDR_IN csin;
    int csin_size = sizeof(csin);
    SOCKET client = accept (server, (SOCKADDR *) &csin, &csin_size);

    if (client == (unsigned) SOCKET_ERROR) {
        return -1;
    }

    *_client = client;
    return 0;
}

int clientConnect (char *ip, int port, SOCKET *_server) 
{
    SOCKET server;
    SOCKADDR_IN server_context;
    SOCKADDR_IN csin;
    int csin_size = sizeof(csin);

    if ((server = socket (AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        error ("Cannot create a new socket.");
        return 0;
    }

    server_context.sin_family      = AF_INET;
    server_context.sin_addr.s_addr = inet_addr (ip);
    server_context.sin_port        = htons (port);

    if (connect (server, (SOCKADDR*) &server_context, csin_size) != 0) {
        error ("Cannot connect to the server : %d", errno);
        return 0;
    }

    *_server = server;
    return 1;
}

// =========== Listener threads
void *clientListener (void *_params)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    ProxyParameters *params = _params;
    char *captureFolder = params->captureFolder;
    SOCKET client = params->client;
    SOCKET server = params->server;
    Plugins *plugins = params->plugins;
    ProxyType proxyType = params->proxyType;

    char outputPath[PATH_MAX];
    sprintf (outputPath, "%s/client.bin", captureFolder);

    RawPacket packet;
    while (1) 
    {
        rawPacketInit(&packet, RAW_PACKET_CLIENT);

        // Receive packet from the game client
        switch (rawPacketRecv (&packet, client)) {
            case 0:
                error ("Cannot receive raw packet from the client from the proxy '%s'.", getProxyName(proxyType));
                goto cleanup;
            break;

            case -1:
                info ("Client disconnected from the proxy '%s'.", getProxyName(proxyType));
                pthread_cancel (params->hServerListener);
                goto cleanup;
            break;

            case 1:
                // OK
            break;
        }

        // Get packet ID value
        WaitForSingleObject (params->mutex, INFINITE);
        packet.id = params->packetId++;
        ReleaseMutex (params->mutex);

        // Write it to the logs
        if (!(rawPacketWriteToFile (&packet, outputPath))) {
            error ("Cannot write packet to capture folder '%s'", outputPath);
            goto cleanup;
        }

        // If present, call the plugins callback
        if (plugins) {
            for (int i = 0; i < plugins->count; i++) {
                Plugin *plugin = &plugins->p[i];
                if (!(plugin->call (&packet, plugin->param))) {
                    error ("Callback didn't succeed.");
                    goto cleanup;
                }
            }
        }

        // Send it to the server
        if (!rawPacketSend (&packet, server)) {
            error ("Cannot send the raw packet to the server.");
            goto cleanup;
        }

        rawPacketFree(&packet);
    }

cleanup:
    pthread_cancel (params->hServerListener);
    return NULL;
}

void *serverListener (void *_params)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    ProxyParameters *params = _params;
    char *captureFolder = params->captureFolder;
    SOCKET client = params->client;
    SOCKET server = params->server;
    Plugins *plugins = params->plugins;
    ProxyType proxyType = params->proxyType;

    char outputPath[PATH_MAX];
    sprintf (outputPath, "%s/server.bin", captureFolder);

    RawPacket packet;
    while (1) 
    {
        rawPacketInit(&packet, RAW_PACKET_SERVER);

        // Receive packet from the server
        switch (rawPacketRecv (&packet, server)) {
            case 0:
                error ("Cannot receive raw packet from the server from the proxy '%s'.", getProxyName(proxyType));
                goto cleanup;
            break;

            case -1:
                info ("Client disconnected from the proxy '%s'.", getProxyName(proxyType));
                goto cleanup;
            break;

            case 1:
                // OK
            break;
        }

        // Get packet ID value
        WaitForSingleObject (params->mutex, INFINITE);
        packet.id = params->packetId++;
        ReleaseMutex (params->mutex);

        // Write it to the logs
        if (!(rawPacketWriteToFile (&packet, outputPath))) {
            error ("Cannot write packet to capture '%s'", outputPath);
            break;
        }

        // If present, call the plugins callback
        if (plugins) {
            for (int i = 0; i < plugins->count; i++) {
                Plugin *plugin = &plugins->p[i];
                if (!(plugin->call (&packet, plugin->param))) {
                    error ("Callback didn't succeed.");
                    goto cleanup;
                }
            }
        }

        // Send it to the client
        if (!rawPacketSend (&packet, client)) {
            error ("Cannot send the raw packet to the server.");
            goto cleanup;
        }

        rawPacketFree(&packet);
    }

cleanup:
    pthread_cancel (params->hClientListener);
    return NULL;
}

// =========== Plugins loader
int pluginLoad (char *pluginName, Plugin *_plugin)
{
    Plugin plugin;

    HMODULE hPlugin;
    if (!(hPlugin = LoadLibrary (pluginName))) {
        error ("Cannot load plugin '%s'", pluginName);
        return 0;
    }

    // Initialize plugin
    PluginInit init;
    void *param;
    if (!(init = (typeof (init)) GetProcAddress (hPlugin, "pluginInit"))) {
        error ("Cannot find 'pluginInit' function exported in '%s'.", pluginName);
        return 0;
    }
    if (!init (&param)) {
        error ("Cannot initialize plugin '%s'", pluginName);
        return 0;
    }

    PluginCallback callback;
    if (!(callback = (typeof (callback)) GetProcAddress (hPlugin, "pluginCallback"))) {
        error ("Cannot find 'pluginCallback' function exported in '%s'.", pluginName);
        return 0;
    }

    PluginUnload onUnload;
    if (!(onUnload = (typeof (onUnload)) GetProcAddress (hPlugin, "pluginUnload"))) {
        warning ("Cannot find 'pluginUnload' function exported in '%s'. Default callback used instead.", pluginName);
        int defaultUnload () {
            return 1;
        }
        onUnload = defaultUnload;
    }

    PluginExit onExit;
    if (!(onExit = (typeof (onExit)) GetProcAddress (hPlugin, "pluginExit"))) {
        warning ("Cannot find 'pluginExit' function exported in '%s'. Default callback used instead.", pluginName);
        int defaultExit () {
            return 1;
        }
        onExit = defaultExit;
    }

    plugin.call = callback;
    plugin.unload = onUnload;
    plugin.exit = onExit;
    plugin.param = param;
    plugin.name = GET_FILENAME(pluginName);

    *_plugin = plugin;
    return 1;
}

// =========== Metadata reader
int updateMetadata (RawPacketMetadata *self, char *sessionFolder, ProxyType proxyType, uint64_t sessionId) {

    // Verify if there is already a metadata file
    char metaPath[PATH_MAX];
    sprintf (metaPath, "%s/metadata.bin", sessionFolder);

    FILE *fMetadata = fopen (metaPath, "rb");
    if (fMetadata) {
        // Already exists, read the existing metadata
        fread (self, sizeof(*self), 1, fMetadata);
        fclose (fMetadata);
    } else {
        // Doesn't exist, generate new metadata
        SYSTEMTIME time;
        GetSystemTime (&time);
        memset(self, 0, sizeof(*self));
        sprintf (self->date, "%.02d-%.02d-%d-%.02dh%.02d", time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute);
        self->sessionId = sessionId;
        strcpy(self->barrackName, "Barrack");
        strcpy(self->zoneName, "Zone");
        strcpy(self->socialName, "Social");
    }

    // Update metadata
    switch (proxyType) {
        case PROXY_TYPE_BARRACK: self->hasBarrack = 1; break;
        case PROXY_TYPE_ZONE:    self->hasZone    = 1; break;
        case PROXY_TYPE_SOCIAL:  self->hasSocial  = 1; break;
    }

    // Write back metadata
    fMetadata = fopen (metaPath, "wb+");
    fwrite (self, sizeof(*self), 1, fMetadata);
    fclose (fMetadata);

    return 1;
}

// =========== Proxy bootsrapper
int startProxy (
    char *serverIp, int serverPort, int proxyPort, 
    Plugins *plugins, 
    ProxyType proxyType, uint64_t sessionId
) {
    // Bind the port of the proxy
    SOCKET proxy;
    info ("Binding proxy '%s' on port '%d'...", getProxyName(proxyType), proxyPort);
    if (serverListen (proxyPort, &proxy) != 0) {
        error ("Cannot listen on port %d", proxyPort);
        return 0;
    }

    // Accept a new client
    SOCKET client;
    info ("Waiting for a client to connect to the proxy '%s'...", getProxyName(proxyType));
    if (serverAccept (proxy, &client) != 0) {
        error ("Cannot accept a new client.");
        return 0;
    }

    // Connect to the real server
    SOCKET server;
    info ("Connecting to the real server '%s' (%s:%d)...", getProxyName(proxyType), serverIp, serverPort);
    if (!(clientConnect (serverIp, serverPort, &server))) {
        error ("Cannot connect to the real server '%s:%d'", serverIp, serverPort);
        return 0;
    }
    info ("Connected!");

    // Init session folder
    CreateDirectoryA ("captures/", NULL);
    char sessionFolder[MAX_PATH];
    sprintf (sessionFolder, "captures/%Id", sessionId);
    CreateDirectoryA (sessionFolder, NULL);

    // Update the metadata
    RawPacketMetadata metadata;
    if (!(updateMetadata (&metadata, sessionFolder, proxyType, sessionId))) {
        error ("Cannot update metadata.");
        return 0;
    }

    // Initialize zone capture folder
    char captureFolder[MAX_PATH];
    sprintf (captureFolder, "%s/%s", sessionFolder, getProxyName(proxyType));
    CreateDirectoryA (captureFolder, NULL);

    // Start worker threads
    ProxyParameters params = {
        .captureFolder = captureFolder,
        .client = client,
        .server = server,
        .plugins = plugins,
        .packetId = 0,
        .proxyType = proxyType
    };
    params.mutex = CreateMutex (NULL, FALSE, NULL);

    pthread_create (&params.hClientListener, 0, clientListener, &params);
    pthread_create (&params.hServerListener, 0, serverListener, &params);
    pthread_join (params.hClientListener, NULL);
    pthread_join (params.hServerListener, NULL);

    return 1;
}

// =========== Command line parser
int parserCommandLine (int argc, char **argv, 
    char **_serverIp, int *_serverPort, int *_proxyPort, 
    Plugins *_plugins, 
    ProxyType *_proxyType, size_t *_sessionId
) {
    int status = 0;
    FILE *sessionIdFile = NULL;

    // Parse commmand line server IP/Port
    char *serverIp = argv[1];
    int serverPort = atoi (argv[2]);
    int proxyPort = atoi (argv[3]);

    // Get proxy type
    char *proxyTypeStr = argv[4];
    ProxyType proxyType = getProxyType(proxyTypeStr);
    if (proxyType == -1) {
        error ("Proxy type '%s' isn't valid.", proxyTypeStr);
        goto cleanup;
    }

    // Get the session ID
    uint64_t sessionId;
    sessionIdFile = fopen ("session_id", "rb+");
    if (!sessionIdFile) {
        // session ID file doesn't exist, create it
        warning ("Cannot open the session ID file. Create a new one.");
        if (!(sessionIdFile = fopen ("session_id", "wb+"))) {
            error ("Cannot open the session ID file at all.");
            goto cleanup;
        }
        fprintf (sessionIdFile, "0");
        sessionId = 0;
    }
    else {
        // Read the session ID
        fscanf (sessionIdFile, "%Id", &sessionId);
    }

    if (strcmp (argv[5], "new") == 0) {
        // Increment it and replace it
        fseek (sessionIdFile, 0, SEEK_SET);
        fprintf (sessionIdFile, "%Id", sessionId+1);
    } else if (strcmp (argv[5], "continue") == 0) {
        // Get the previous ID
        sessionId -= 1;
    } else {
        error ("Action '%s' not supported for session ID.", argv[5]);
        goto cleanup;
    }
    fclose (sessionIdFile);
    sessionIdFile = NULL;

    // Load plugins
    Plugins plugins;
    int pluginsPos = 6;
    plugins.count = argc - pluginsPos;

    if (argc >= pluginsPos + 1) 
    {
        char **pPluginName = &argv[pluginsPos]; 
        plugins.p = malloc (sizeof(Plugin) * plugins.count);

        for (int i = 0; i < plugins.count; i++) {
            Plugin *plugin = &plugins.p[i];
            if (!(pluginLoad (pPluginName[i], plugin))) {
                error ("Cannot load the plugin '%s'", pPluginName[i]);
                goto cleanup;
            }

            info ("Plugin '%s' loaded !", plugin->name);
        }
    }

    *_serverIp = serverIp;
    *_serverPort = serverPort;
    *_proxyPort = proxyPort;
    *_plugins = plugins;
    *_proxyType = proxyType;
    *_sessionId = sessionId;
    status = 1;

cleanup:
    if (sessionIdFile) {
        fclose (sessionIdFile);
    }
    return status;
}

int countProcessRunning (char *processName)
{
    PROCESSENTRY32 pe32 = {sizeof(PROCESSENTRY32)};
    HANDLE hSnapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

    int count = 0;
    if (Process32First (hSnapshot, &pe32)) {
        do {
            if (stricmp (pe32.szExeFile, processName) == 0) {
                count++;
            }
        } while (Process32Next (hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return count;
} 

int main (int argc, char **argv)
{
    if (argc < 5) {
        info ("Usage : %s <Server IP> <Server Port> <Local Port> <barrack|zone|social> <new|continue> [plugin.dll]", GET_FILENAME (argv[0]));
        return 0;
    }

    char *serverIp;
    int serverPort;
    int proxyPort;
    Plugins plugins;
    ProxyType proxyType;
    uint64_t sessionId;

    // Parse the command line
    if (!(parserCommandLine (argc, argv, &serverIp, &serverPort, &proxyPort, &plugins, &proxyType, &sessionId))) {
        error ("Cannot parse the command line correctly.");
        goto cleanup;
    }

    // Start the proxy
    special ("========= Proxy '%s' ID=%d starts. ========= ", getProxyName (proxyType), sessionId);
    if (!(startProxy (serverIp, serverPort, proxyPort, &plugins, proxyType, sessionId))) {
        error ("Cannot start the proxy properly.");
        goto cleanup;
    }

cleanup:
    special ("========= Proxy '%s' ID=%d exits. ========= ", getProxyName (proxyType), sessionId);

    for (int i = 0; i < plugins.count; i++) {
        Plugin *plugin = &plugins.p[i];
        if (!(plugin->unload(plugin->param))) {
            error ("Cannot unload plugin '%s' correctly.");
        }
    }

    // Don't exit until all other proxies haven't finished
    if (strcmp (argv[5], "new") == 0) {
        while (countProcessRunning(GET_FILENAME(argv[0])) != 1) {
            Sleep(500);
        }
    }

    for (int i = 0; i < plugins.count; i++) {
        Plugin *plugin = &plugins.p[i];
        if (!(plugin->exit(plugin->param))) {
            error ("Cannot exit plugin '%s' correctly.");
        }
    }

    return 0;
}