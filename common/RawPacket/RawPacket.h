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
	RAW_PACKET_SERVER
}	RawPacketType;

#pragma pack(push, 1)
typedef struct RawPacket {
    int64_t id;
	int dataSize;
	uint8_t type;
 	unsigned char data[8192*2];
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
int rawPacketWriteToFile (RawPacket *self, char *outputPath);
int rawPacketReadFromFile (RawPacket *self, char *inputPath, size_t *cursor);
int rawPacketRecv (RawPacket *self, SOCKET socket, RawPacketType type);
int rawPacketSend (RawPacket *self, SOCKET socket);

// RawPacketFile
int rawPacketFileOpen (char *inputPath, size_t cursor, FILE **_input);
int rawPacketFileGetPacketId (char *inputPath, size_t cursor, int64_t *_packetId);