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

// ---------- Includes ------------
#include <string.h>
#include "crypto.h"
#include "schedule.h"
#include "dbg/dbg.h"

// ------ Static declaration -------
const uint32_t keyIndex = 11;
const uint32_t numPoint[] = {16, 2, 256, 768};
const uint32_t seekPoint[] = {4, 1056, 24, 284};
uint8_t g_schedule[sizeof(BF_KEY)];
const char* keyTable[] = {
    "jvwvqgfghyrqewea",
    "gsqsdafkssqshafd",
    "mmxi0dozaqblnmwo",
    "nyrqvwescuanhanu",
    "gbeugfaacudgaxcs",
    "rbneolithqb5yswm",
    "1udgandceoferaxe",
    "qarqvwessdicaqbe",
    "2udqaxxxiscogito",
    "affcbjwxymsi0doz",
    "ydigadeath8nnmwd",
    "hsunffalqyrqewes",
    "gqbnnmnmymsi0doz",
    "hvwvqgfl1nrqewes",
    "sxqsdafkssqsdafk",
    "ssqsaafsssqsdof6",
};
// ------ Extern function implementation ------

int cryptoInit(void) {

    const char* key = keyTable[keyIndex];
    int offset = 0;

    for (int i = 0; i < 4; ++i)
    {
        if (numPoint[i] + offset > 1042) {
            error ("NumPoint invalid");
            return 0;
        }

        memcpy((char*) g_schedule + offset * 4, (char *) initSchedule + seekPoint[i] * 4, numPoint[i] * 4);
        offset += numPoint[i];
    }

    BF_set_key_custom_sch((BF_KEY *) g_schedule, 16, (unsigned char*) key);

    return 1;
}

void
cryptPacketUnwrapHeader(uint8_t **packet, int *packetSize, CryptPacketHeader *header) {
    memcpy(header, *packet, sizeof(CryptPacketHeader));

    *packet = (*packet) + sizeof(CryptPacketHeader);
    *packetSize -= sizeof(CryptPacketHeader);
}

int cryptoDecryptPacket(uint8_t **packet, int *packetSize, CryptPacketHeader *cryptHeader) {

    // Unwrap the crypt packet header, and check the cryptHeader size
    // In a single request process, it must match exactly the same size
    cryptPacketUnwrapHeader(packet, packetSize, cryptHeader);

    /*
    * It happens when two packets are buffered in the same TCP packet
    if (*packetSize != cryptHeader->plainSize) {
        warning ("The real packet size (0x%x) doesn't match with the packet size in the header (0x%x). Ignore request.",
            *packetSize, cryptHeader->plainSize);
    }
    */

    int blocks = cryptHeader->plainSize / BF_BLOCK;

    for (int i = 0; i < blocks; i++) {
        size_t offset = i * BF_BLOCK;
        BF_ecb_encrypt(*packet + offset, *packet + offset, (BF_KEY *) g_schedule, BF_DECRYPT);
    }

    return 1;
}

size_t cryptPacketGetSize (CryptPacket *self) {
    return self->header.plainSize + sizeof(self->header.plainSize);
}

CryptPacket *cryptoEncryptPacket(uint8_t *packet, int packetSize) {

    int blocks = packetSize / BF_BLOCK;

    CryptPacket *cp = malloc(sizeof(CryptPacket));
    memcpy(cp->data, packet, packetSize);
    cp->header.plainSize = packetSize;

    for (int i = 0; i < blocks; i++) {
        size_t offset = i * BF_BLOCK;
        BF_ecb_encrypt(cp->data + offset, cp->data + offset, (BF_KEY *) g_schedule, BF_ENCRYPT);
    }

    return cp;
}
