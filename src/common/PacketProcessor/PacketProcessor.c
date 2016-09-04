#include "PacketProcessor.h"
#include "crypto/crypto.h"
#include "PacketType/PacketType.h"
#include "dbg/dbg.h"
#include "zlib/zlib.h"
#include <stdbool.h>

RawPacket packetBuffer = {
    .dataSize = 0
};

bool isInit = 0;

void packetProcessorInit () {
    rawPacketInit (&packetBuffer, RAW_PACKET_UNK);
}

int getPacketBuffer (RawPacket *self) {
    
    if (self->type != packetBuffer.type) {
        // Must match the same type
        return 1;
    }

    if (packetBuffer.dataSize == 0) {
        // Nothing to do
        return 1;
    }

    if (self->dataSize + packetBuffer.dataSize >self->bufferSize) {
        error ("Destination packet buffer isn't large enough !");
        return 0;
    }

    // Put the data at the start at the end of the packet buffer
    memcpy (&self->buffer[packetBuffer.dataSize], &self->buffer[0], self->dataSize);
    memcpy (&self->buffer[0], packetBuffer.buffer, packetBuffer.dataSize);
    self->dataSize += packetBuffer.dataSize;
    self->cursor = self->buffer;
    rawPacketInit (&packetBuffer, RAW_PACKET_UNK);

    return 1;
}

int foreachDecryptedPacket (RawPacket *rawPacket, DecryptCallback callback, void *user_data) {

    if (!isInit) {
        packetProcessorInit();
        isInit = true;
    }

    int status = 0;

    // We don't want to modify the packet in parater ; make a copy
    RawPacket copyPacket;
    rawPacketCopy(&copyPacket, rawPacket);

    getPacketBuffer(&copyPacket);
    int dataSize = copyPacket.dataSize;

    switch (copyPacket.type)
    {
        case RAW_PACKET_CLIENT: 
        {
            // Only client packets are encrypted
            CryptPacketHeader cryptHeader;

            // Sometimes, the client sends multiple packets at once
            while (dataSize > 0)
            {
                // Unwrap the crypto header, and decrypt the data
                if (!(cryptoDecryptPacket (&copyPacket.cursor, &dataSize, &cryptHeader))) {
                    error ("Cannot decrypt packet.");
                    goto cleanup;
                }

                // Creates a backup of the decrypted data
                uint8_t decryptedCopy[cryptHeader.plainSize];
                memcpy (decryptedCopy, copyPacket.cursor, sizeof(decryptedCopy));

                if (!(callback (copyPacket.cursor, cryptHeader.plainSize, copyPacket.type, copyPacket.id, user_data))) {
                    error ("Callback failed.");
                    goto cleanup;
                }

                // Check if the callback modified the backup
                if (memcmp (copyPacket.cursor, decryptedCopy, cryptHeader.plainSize) != 0) {
                    // Buffer modified, change the original encrypted buffer
                    CryptPacket *cp = cryptoEncryptPacket (copyPacket.cursor, cryptHeader.plainSize);
                    size_t offset = copyPacket.cursor - copyPacket.buffer; 
                    memcpy (rawPacket->buffer + offset, cp, cryptPacketGetSize (cp));
                    info ("Client packet modified !");
                }

                // Aaaand let's go to the next packet
                copyPacket.cursor += cryptHeader.plainSize;
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
                PacketType_t type = 0;
                memcpy (&type, copyPacket.cursor, sizeof(type));
                int pktSize = PacketType_get_size (type);

                if (pktSize == -1) {
                    // Check for compressed packets
                    if ((type & 0xF000) == 0x8000) {
                        Zlib zlib;
                        size_t size = (type & ~0x8000) + 3;
                        zlibDecompress(&zlib, copyPacket.cursor + 2, size);
                        RawPacket compressedRawPacket;
                        rawPacketInit(&compressedRawPacket, RAW_PACKET_SERVER);
                        rawPacketAdd(&compressedRawPacket, zlib.buffer, zlib.header.size, RAW_PACKET_SERVER);
                        compressedRawPacket.id = copyPacket.id;
                        if (!(foreachDecryptedPacket (&compressedRawPacket, callback, user_data))) {
                            error ("Cannot decrypt compressed packets.");
                            goto cleanup;
                        }
                        pktSize = size;
                    } else {
                        error ("An unknown packet has been encountered. (type = %d - 0x%x)", type, type);
                        error ("It is possibly due to a client/server patch.");
                        error ("Please update the packet decryptor engine.");
                        buffer_print (copyPacket.buffer, copyPacket.dataSize, "RawPacketDump : ");
                        error ("Skipping the raw packet ...");
                        break;
                    }
                } 
                else {
                    if (pktSize == 0) {
                        // Variable sized packet
                        VariableSizePacketHeader header;
                        memcpy (&header, copyPacket.cursor, sizeof(header));
                        pktSize = header.packetSize;
                    }

                    if (pktSize != dataSize) {
                        // warning ("Suspicious size ! %d expected, got %d.", pktSize, dataSize);
                    }

                    if (dataSize < pktSize) {
                        // The current packet isn't large enough. 
                        // Copy it to a buffer that will be used for the next packet.
                        if (!(rawPacketAdd (&packetBuffer, copyPacket.cursor, dataSize, copyPacket.type))) {
                            error ("Cannot add to packet buffer.");
                            goto cleanup;
                        }
                    }
                    else {
                        // Creates a backup of the decrypted data
                        uint8_t decryptedCopy[pktSize];
                        memcpy (decryptedCopy, copyPacket.cursor, sizeof(decryptedCopy));

                        if (!(callback (copyPacket.cursor, pktSize, rawPacket->type, rawPacket->id, user_data))) {
                            error ("Callback failed.");
                            goto cleanup;
                        }

                        // Check if the callback modified the backup
                        if (memcmp (copyPacket.cursor, decryptedCopy, pktSize) != 0) {
                            // Buffer modified, change the original buffer
                            size_t offset = copyPacket.cursor - copyPacket.buffer; 
                            memcpy (rawPacket->buffer + offset, copyPacket.cursor, pktSize);
                        }
                    }
                }

                copyPacket.cursor += pktSize;
                dataSize -= pktSize;
            }
        }
        break;

        default : 
            error ("Wrong packet type : %d", rawPacket->type); 
            goto cleanup;
        break;
    }

    status = 1;
cleanup:
    return status;
}