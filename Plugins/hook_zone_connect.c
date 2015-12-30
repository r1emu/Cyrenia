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
#include "plugins.h"
#include "dbg/dbg.h"
#include "crypto/crypto.h"
#include "crypto/decrypt_engine.h"
#include "PacketType/PacketType.h"
#include <stdlib.h>
#include <stdint.h>

int hookConnectToZoneServer (uint8_t *data, int dataSize, RawPacketType type, int64_t packetId, void *user_data)
{
    PacketType_t pktType = 0;
    memcpy (&pktType, data, sizeof(pktType));

    switch (pktType)
    {
        case BC_START_GAMEOK: {
            // The server asks to the client to connect to a zone server
            #pragma pack(push, 1)
            struct {
                ServerPacketHeader header;
                uint32_t zoneServerId;
                uint32_t zoneServerIp;
                uint32_t zoneServerPort;
                uint32_t mapId;
                uint8_t commanderListId;
                uint64_t socialInfoId;
                uint8_t isSingleMap;
                uint8_t unk1;
            } *packet = (void *) data;
            #pragma pack(pop)

            // Read the original server IP
            unsigned char serverIp[4];
            serverIp[0] = packet->zoneServerIp & 0xFF;
            serverIp[1] = (packet->zoneServerIp >> 8) & 0xFF;
            serverIp[2] = (packet->zoneServerIp >> 16) & 0xFF;
            serverIp[3] = (packet->zoneServerIp >> 24) & 0xFF;

            // Redirect the IP to 127.0.0.1
            packet->zoneServerIp = *(uint32_t *)((char []) {127, 0, 0, 1});

            // Start a new proxy between the client and the official server
            char executableName[1000];
            GetModuleFileName(NULL, executableName, sizeof(executableName));

            char commandLine[1000];
            sprintf(commandLine, "%s %d.%d.%d.%d %d %d zone continue", executableName, serverIp[0], serverIp[1], serverIp[2], serverIp[3], packet->zoneServerPort, packet->zoneServerPort);

            STARTUPINFO si = {0};
            PROCESS_INFORMATION pi = {0};
            si.cb = sizeof(STARTUPINFO); 
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
            si.dwFlags |= STARTF_USESTDHANDLES;
            if (!CreateProcess (executableName, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
                error ("Cannot launch Zone Server executable : %s.", executableName);
                char *errorReason;
                FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &errorReason, 0, NULL
                );
                error ("Error reason : %s", errorReason);
                return 0;
            }

            special ("Zone connection hooked ! Starting a new proxy ...");

            // Wait a bit for the process to spawn
            Sleep (1000);
        }
        break;
    }

    return 1;
}

int pluginCallback (RawPacket *packet) {

    static int initialized = 0;
    if (!initialized) {    
        // Initialize crypto engine and packet engine
        cryptoInit();
        packetTypeInit();
        initialized = 1;
    }

    // Decrypt the packet copy, and call hookConnectToZoneServer once decrypted
    if (!(foreachDecryptedPacket (packet, hookConnectToZoneServer, NULL))) {
        error ("Cannot decrypt packet.");
    }

    return 1;
}