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
#include "PacketProcessor/PacketProcessor.h"
#include "PacketType/PacketType.h"
#include "crypto/crypto.h"
#include <stdlib.h>
#include <stdint.h>

int sendPacketToGui (uint8_t *data, int dataSize, RawPacketType type, int64_t packetId, void *user_data)
{
    PacketType_t pktType = 0;
    memcpy (&pktType, data, sizeof(pktType));

    return 1;
}

int pluginInit () {
    
    // Initialize crypto engine and packet engine
    if (!(cryptoInit())) {
        error ("Cannot initialize crypto engine.");
        return 0;
    }
    
    packetTypeInit();

    return 1;
}

int pluginCallback (RawPacket *packet) {

    if (!(foreachDecryptedPacket (packet, sendPacketToGui, NULL))) {
        error ("Cannot decrypt packet.");
    }

    return 1;
}