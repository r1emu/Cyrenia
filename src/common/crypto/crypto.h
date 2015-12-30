/**
 *
 *   ██████╗   ██╗ ███████╗ ███╗   ███╗ ██╗   ██╗
 *   ██╔══██╗ ███║ ██╔════╝ ████╗ ████║ ██║   ██║
 *   ██████╔╝ ╚██║ █████╗   ██╔████╔██║ ██║   ██║
 *   ██╔══██╗  ██║ ██╔══╝   ██║╚██╔╝██║ ██║   ██║
 *   ██║  ██║  ██║ ███████╗ ██║ ╚═╝ ██║ ╚██████╔╝
 *   ╚═╝  ╚═╝  ╚═╝ ╚══════╝ ╚═╝     ╚═╝  ╚═════╝
 *
 * @file Crypto.h
 * @brief Crypto contains cryptographic functions needed for the communication between the client and the server
 *
 * @license GNU GENERAL PUBLIC LICENSE - Version 2, June 1991
 *          See LICENSE file for further information
 */

#pragma once

// ---------- Includes ------------
#include <stdint.h>
#include "bf/blowfish.h"

// ---------- Defines -------------
#pragma pack(push, 1)
typedef struct CryptPacketHeader {
    uint16_t plainSize;
} CryptPacketHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct CryptPacket {
    CryptPacketHeader header;
    uint8_t data[8192];
}   CryptPacket;
#pragma pack(pop)

// ----------- Functions ------------


/**
 * @brief Initialize client crypto components
 */
int cryptoInit(void);

/**
 * @brief Unwrap the client packet header and decrypt the packet.
 * @param[in,out] packet The packet ciphered. After this call, the packet is decrypted.
 * @param[in] packetSize A pointer to the crypted packet size
 * @return true on success, false otherwise
 */
int cryptoDecryptPacket(uint8_t **packet, int *packetSize, CryptPacketHeader *cryptHeader);
CryptPacket *cryptoEncryptPacket(uint8_t *packet, int packetSize);
size_t cryptPacketGetSize (CryptPacket *self);