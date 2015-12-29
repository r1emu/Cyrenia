#include "decrypt_engine.h"
#include "crypto/crypto.h"
#include "PacketType/PacketType.h"
#include "dbg/dbg.h"

int foreachDecryptedPacket (uint8_t *cData, int dataSize, RawPacketType type, int64_t packetId, DecryptCallback callback, void *user_data) {

    int status = 0;

    // We don't want to modify the packet in parater ; make a copy
    uint8_t cDataBackup[dataSize];
    memcpy (cDataBackup, cData, sizeof(cDataBackup));

    // Pointers to current packets
    uint8_t *curPacket = cDataBackup;
    uint8_t *curPacketReal = cData;

    switch (type)
    {
        case RAW_PACKET_CLIENT: 
        {
            // Only client packets are encrypted
            CryptPacketHeader cryptHeader;

            // Sometimes, the client sends multiple packets at once
            while (dataSize > 0)
            {
                // Unwrap the crypto header, and decrypt the data
                if (!(cryptoDecryptPacket(&curPacket, &dataSize, &cryptHeader))) {
                    error ("Cannot decrypt packet.");
                    goto cleanup;
                }

                // Creates a backup of the decrypted data
                uint8_t dDataBackup[cryptHeader.plainSize];
                memcpy(dDataBackup, curPacket, sizeof(dDataBackup));

                if (!(callback (curPacket, cryptHeader.plainSize, type, packetId, user_data))) {
                    error ("Callback failed.");
                    goto cleanup;
                }

                // Check if the callback modified the backup
                if (memcmp (curPacket, dDataBackup, cryptHeader.plainSize) != 0) {
                    // Buffer modified, change the original encrypted buffer
                    CryptPacket *cp = cryptoEncryptPacket(curPacket, cryptHeader.plainSize);
                    memcpy (curPacketReal, cp, cryptPacketGetSize(cp));
                    info ("Client buffer modified ! :]");
                }

                // Aaaand let's go to the next packet
                curPacket += cryptHeader.plainSize;
                curPacketReal += cryptHeader.plainSize;
                dataSize -= cryptHeader.plainSize;
            }
        }
        break;

        case RAW_PACKET_SERVER: 
        {
            // Sometimes, the server sends multiple packets at once
            while (dataSize > 0)
            {
                // Get the original packet size
                PacketType_t type;
                memcpy (&type, curPacket, sizeof(type));
                int pktSize = PacketType_get_size(type);

                if (pktSize == -1) {
                    error ("An unknown packet has been encountered.");
                    error ("It is possibly due to a client/server patch.");
                    error ("Please update the packet decryptor engine.");
                    buffer_print (curPacket, 32, "First 32 bytes : ");
                    exit (0);
                }

                if (pktSize == 0) {
                    // Variable sized packet
                    VariableSizePacketHeader header;
                    memcpy (&header, curPacket, sizeof(header));
                    pktSize = header.packetSize;
                    info ("Variable sized packet : %d (%x)", pktSize, pktSize);
                }

                if (pktSize != dataSize) {
                    warning ("Suspicious size ! %d expected, got %d.", pktSize, dataSize);
                }

                if (dataSize < pktSize) {
                    error ("Careful. The packet isn't big enough...");
                    pktSize = dataSize;
                }

                // Creates a backup of the decrypted data
                uint8_t dDataBackup[pktSize];
                memcpy(dDataBackup, curPacket, sizeof(dDataBackup));

                if (!(callback (curPacket, pktSize, type, packetId, user_data))) {
                    error ("Callback failed.");
                    goto cleanup;
                }

                // Check if the callback modified the backup
                if (memcmp (curPacket, dDataBackup, pktSize) != 0) {
                    // Buffer modified, change the original buffer
                    memcpy (curPacketReal, curPacket, pktSize);
                    info ("Server buffer modified ! :]");
                }

                curPacket += pktSize;
                curPacketReal += pktSize;
                dataSize -= pktSize;
            }
        }
        break;

        default : 
            error ("Wrong packet type : %d", type); 
            goto cleanup;
        break;
    }

    status = 1;
cleanup:
    return status;
}