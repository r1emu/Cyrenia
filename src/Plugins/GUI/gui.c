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

#include <ws2tcpip.h>
#include "plugins.h"
#include "dbg/dbg.h"
#include "PacketProcessor/PacketProcessor.h"
#include "PacketType/PacketType.h"
#include "cwebsocket/websocket.h"
#include "crypto/crypto.h"
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/stat.h>

#define PORT 8088
#define BUF_LEN 0xFFFF
#define URL_LINK "client.url"

typedef struct {
    SOCKET client;
}   GUIApp;

int sendBytesToClient (SOCKET clientSocket, const uint8_t *buffer, size_t bufferSize)
{
    ssize_t written = send(clientSocket, (const char *) buffer, bufferSize, 0);
    if (written == -1) {
        closesocket(clientSocket);
        perror("send failed");
        return EXIT_FAILURE;
    }
    if (written != bufferSize) {
        closesocket(clientSocket);
        perror("written not all bytes");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

int sendTextToClient (SOCKET clientSocket, const uint8_t *text, size_t textSize)
{
    size_t frameSize;
    uint8_t buffer[BUF_LEN];
    wsMakeFrame(text, textSize, buffer, &frameSize, WS_TEXT_FRAME);

    return sendBytesToClient (clientSocket, buffer, frameSize);
}

char filterCharacter (char c) {
    if (
        isprint(c) 
     && c != '\r' 
     && c != '"' 
     && c != '\\' 
     && c != '\n' 
     && c != '\t') {
        return c;
    } else {
        return '.';
    }
}

int sendPacketToGui (uint8_t *packet, int packetSize, RawPacketType type, int64_t packetId, void *userdata)
{
    GUIApp *app = (GUIApp *) userdata;

    PacketType_t packetType = 0;
    memcpy (&packetType, packet, sizeof(packetType));

    char *packetTypeStr = PacketType_to_string (packetType);

    char buffer[BUF_LEN] = {0};
    char packetData[BUF_LEN/4] = {0};
    char ascii[BUF_LEN/4] = {0};
    char *packetDataPtr = packetData;
    char *asciiPtr = ascii;

    for (int i = 0; i < packetSize; i++) {
        if (
            (asciiPtr > ascii + sizeof(ascii) + 0x100) 
        ||  (packetDataPtr > packetData + sizeof(packetData) + 0x100) 
        ) {
            warning ("Output truncated for packet '%s' (packet size = %d)", packetTypeStr, packetSize);
            break;
        }

        packetDataPtr += sprintf (packetDataPtr, "%.2x ", packet[i]);
        asciiPtr      += sprintf (asciiPtr, "%c", filterCharacter(packet[i]));
        if ((i+1) % 16 == 0) {
            packetDataPtr += sprintf (packetDataPtr, "<br/>");
            asciiPtr      += sprintf (asciiPtr, "<br/>");
        }
    }

    char *direction = (type == RAW_PACKET_CLIENT) ? "<b><font color='green'>C&#8594;S</font></b>" : "<b><font color='red'>S&#8594;C</font></b>";

    sprintf (buffer, "{\"id\" : \"%Id\", \"direction\" : \"%s\", \"size\" : \"%d\", \"type\" : \"%s\", \"typeID\" : \"%#x\", \"data\" : \"%s\", \"ascii\" : \"%s\"}", 
        packetId, direction, packetSize, packetTypeStr, packetType, packetData, ascii);

    sendTextToClient (app->client, (uint8_t *) buffer, strlen(buffer));

    return 1;
}

int waitForClient (SOCKET *_client)
{
    int listenSocket = socket (AF_INET, SOCK_STREAM, 0);
    if (listenSocket == -1) {
        error ("Create socket failed");
        return 0;
    }
    
    struct sockaddr_in local;
    memset (&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons (PORT);

    // Bind websocket server
    while (bind (listenSocket, (struct sockaddr *) &local, sizeof(local)) == -1) {
        static int alreadyDisplayed = 0;
        if (!alreadyDisplayed) {
            alreadyDisplayed = 1;
            error ("Bind on port '%d' failed.", PORT);
        }
        Sleep(100);
    }

    if (listen (listenSocket, 1) == -1) {
        error ("Listen failed.");
        return 0;
    }

    info ("Opened %s:%d", inet_ntoa (local.sin_addr), ntohs (local.sin_port));

    struct sockaddr_in remote;
    socklen_t sockaddrLen = sizeof (remote);

    // Wait for client connection
    SOCKET client = accept (listenSocket, (struct sockaddr*) &remote, &sockaddrLen);
    if (client == -1) {
        error ("Accept failed.");
        return 0;
    }
    
    info ("Connected %s:%d", inet_ntoa (remote.sin_addr), ntohs (remote.sin_port));

    // Waiting for the handshake
    size_t frameSize = BUF_LEN;
    uint8_t buffer[BUF_LEN] = {0};
    ssize_t bRecv = recv (client, (char *) buffer, BUF_LEN, 0);
    if (!bRecv) {
        error ("Connection with WebSocket failed.");
        return 0;
    }

    // Parse and answer
    wsHandshake handshake;
    if (wsParseHandshake (buffer, bRecv, &handshake) != WS_OPENING_FRAME) {
        error ("Opening frame excepted.");
        return 0;
    }
    
    wsGetHandshakeAnswer(&handshake, buffer, &frameSize);
    freeHandshake(&handshake);

    if (sendBytesToClient (client, buffer, frameSize) == EXIT_FAILURE) {
        error ("Cannot send back the handshake.");
        return 0;
    }

    *_client = client;
    return 1;
}

char *getModulePath (char *module)
{
    // Get current module path
    char path [MAX_PATH] = {0};
    GetModuleFileName (GetModuleHandle (module), path, sizeof(path));

    char * lastSlash = strrchr (path, '\\');
    char * dllName = (lastSlash != NULL) ? &lastSlash[1] : path;
    dllName [0] = '\0';

    if (!strlen (path)) {
        return NULL;
    }

    return strdup (path);
}

int pluginInit (void **pluginData) {

    // Check if file exists
    struct stat buffer;
    if (stat (URL_LINK, &buffer) != 0) {
        // Hacky way to open local files in default browser
        char *clientPath = NULL;
        asprintf (&clientPath, 
            "[InternetShortcut]\n"
            "URL=file:///%sGUI/client/client.html", getModulePath ("GUI.dll"));

        FILE *url = fopen (URL_LINK, "w+");
        fprintf (url, "%s", clientPath);
        fclose (url);
        free (clientPath);

        // Open default browser
        ShellExecute (NULL, "open", URL_LINK, NULL, NULL, SW_SHOWNORMAL);
    }

    // Initialize WSA
    WSADATA wsa;
    WSAStartup (MAKEWORD (2, 0), &wsa);

    // Initialize crypto engine 
    if (!(cryptoInit ())) {
        error ("Cannot initialize crypto engine.");
        return 0;
    }
    
    // Initialize packet engine
    packetTypeInit ();

    // Waiting for a client to connect...
    SOCKET client;
    if (!(waitForClient (&client))) {
        error ("Cannot accept a new client.");
        return 0;
    }

    // Write results to GUI app
    GUIApp *app = NULL;
    if (!(app = malloc (sizeof(GUIApp)))) {
        error ("Cannot allocate GUI app.");
        return 0;
    }
    app->client = client;

    *pluginData = app;
    return 1;
}

int pluginCallback (RawPacket *packet, void *pluginData) {
    if (!(foreachDecryptedPacket (packet, sendPacketToGui, pluginData))) {
        error ("Cannot decrypt packet.");
    }
    return 1;
}

int pluginUnload (void *pluginData) {
    GUIApp *app = (GUIApp *) pluginData;
    closesocket(app->client);
    return 1;
}

int pluginExit (void *pluginData) {
    unlink ("client.url");
    return 1;
}