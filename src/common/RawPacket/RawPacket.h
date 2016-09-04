/**
 *
 *   ██████╗   ██╗ ███████╗ ███╗   ███╗ ██╗   ██╗
 *   ██╔══██╗ ███║ ██╔════╝ ████╗ ████║ ██║   ██║
 *   ██████╔╝ ╚██║ █████╗   ██╔████╔██║ ██║   ██║
 *   ██╔══██╗  ██║ ██╔══╝   ██║╚██╔╝██║ ██║   ██║
 *   ██║  ██║  ██║ ███████╗ ██║ ╚═╝ ██║ ╚██████╔╝
 *   ╚═╝  ╚═╝  ╚═╝ ╚══════╝ ╚═╝     ╚═╝  ╚═════╝
 *
 * @file RawPacket.h
 *
 * @license GNU GENERAL PUBLIC LICENSE - Version 2, June 1991
 *          See LICENSE file for further information
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>

typedef enum {
	RAW_PACKET_CLIENT,
	RAW_PACKET_SERVER,
    RAW_PACKET_UNK
}	RawPacketType;

#pragma pack(push, 1)
typedef struct RawPacket {
    int64_t id;
	int dataSize;
	RawPacketType type;
    size_t bufferSize;
 	uint8_t *buffer;
    uint8_t *cursor; 
} RawPacket;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct RawPacketMetadata {
    char date[sizeof("11-22-3333-44h55")];
    char hasBarrack;
    char barrackName[10];
    char hasZone;
    char zoneName[10];
    char hasSocial;
    char socialName[10];
    uint64_t sessionId;
} RawPacketMetadata;
#pragma pack(pop)

// RawPacket
int rawPacketInit (RawPacket *self, RawPacketType type);
int rawPacketWriteToFile (RawPacket *self, char *outputPath);
int rawPacketReadFromFile (RawPacket *self, char *inputPath, size_t *cursor);
int rawPacketRecv (RawPacket *self, SOCKET socket);
int rawPacketSend (RawPacket *self, SOCKET socket);
int rawPacketAdd (RawPacket *self, uint8_t *data, int dataSize, RawPacketType type);
void rawPacketCopy (RawPacket *dest, RawPacket *src);

// RawPacketFile
int rawPacketFileOpen (char *inputPath, size_t cursor, FILE **_input);
int rawPacketFileGetPacketId (char *inputPath, size_t cursor, int64_t *_packetId);