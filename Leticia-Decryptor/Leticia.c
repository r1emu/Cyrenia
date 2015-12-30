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

#include "dbg/dbg.h"
#include "RawPacket/RawPacket.h"
#include "PacketType/PacketType.h"
#include "crypto/crypto.h"
#include "crypto/decrypt_engine.h"

int writePacketToFiles (uint8_t *data, int dataSize, RawPacketType type, int64_t packetId, void *_captureFolder) {

    char *captureFolder = (char *) _captureFolder;

    char capturePath[MAX_PATH];
    sprintf (capturePath, "%s/capture.txt", captureFolder);


    PacketType_t packetType = 0;
    memcpy (&packetType, data, sizeof(PacketType_t));
    char *packetTypeStr = PacketType_to_string (packetType);

    // Open the output files
    FILE *capture = fopen (capturePath, "ab+");
    if (!capture) {
        error ("Cannot open the capture file.");
        return 0;
    }

    char decryptedPacketPath[PATH_MAX] = {0};
    sprintf (decryptedPacketPath, "%s/%s/", captureFolder, packetTypeStr);
    CreateDirectoryA (decryptedPacketPath, NULL);
    sprintf (decryptedPacketPath, "%s/%s_%Id.bin", decryptedPacketPath, packetTypeStr, packetId);

    FILE *decryptedFile = fopen (decryptedPacketPath, "w+");
    if (!decryptedFile) {
        error ("Cannot open the packet ID %d file.", packetId);
        return 0;
    }

    info ("[%s][%Id] PacketType = %s", (type == RAW_PACKET_CLIENT) ? "CLIENT" : "SERVER", packetId, packetTypeStr);
    if (strcmp(packetTypeStr, "UNKNOWN_PACKET") == 0) {
        info ("packetType = %d", packetType);
        buffer_print (data, dataSize, NULL);
    }
    // buffer_print (data, dataSize, NULL);

    // Write the packets in subpacket file
    fwrite (data, dataSize, 1, decryptedFile);
    // Write in packet in the capture
    dbgSetOutput (capture);
    dbg ("[%s][%Id] PacketType = %s", (type == RAW_PACKET_CLIENT) ? "CLIENT" : "SERVER", packetId, packetTypeStr);
    buffer_print (data, dataSize, NULL);
    dbgSetOutput (stdout);

    // Cleanup
    fclose (capture);
    fclose (decryptedFile);

    return 1;
}

int read_packets (char *rawCaptureFolder, char *sessionFolder) {

    size_t recvOffset = 0;
    size_t sendOffset = 0;

    char rawClientCapture[PATH_MAX];
    sprintf (rawClientCapture, "%s/client.bin", rawCaptureFolder);
    char rawServerCapture[PATH_MAX];
    sprintf (rawServerCapture, "%s/server.bin", rawCaptureFolder);

    // Let's suppose the packet ID is in the recv file
    int64_t curPacketId = 0;
    int64_t lastPacketId = -1;
    char *rawCapture = rawClientCapture;
    size_t *offset = &recvOffset;

    while (1)
    {
        RawPacket packet;
        rawPacketInit (&packet);

        // Get the packet ID in the current file
        switch (rawPacketFileGetPacketId (rawCapture, *offset, &curPacketId)) {
            case 0: {
                error ("Cannot read the packet ID from '%s'", rawCapture);
                return 0;
            } break;

            case -1: {
                warning ("End of file detected of '%s'.", rawCapture);
                return -1;
            } break;

            case 1:
                // OK
            break;
        }

        // Check if it is correct
        if (curPacketId != lastPacketId + 1) {
            rawCapture = (rawCapture == rawClientCapture) ? rawServerCapture : rawClientCapture;
            offset     = (offset     == &recvOffset)    ? &sendOffset    : &recvOffset;
        }

        // Check in the other file if the packet ID is correct
        switch (rawPacketFileGetPacketId (rawCapture, *offset, &curPacketId)) {
            case 0: {
                error ("Cannot read the packet ID from '%s'", rawCapture);
                return 0;
            } break;

            case -1: {
                warning ("End of file detected of '%s'.", rawCapture);
                return -1;
            } break;

            case 1:
                // OK
            break;
        }

        // Check if it is correct
        if (curPacketId != lastPacketId + 1) {
            error ("Can't find packet ID %Id.", lastPacketId + 1);
            rawCapture = (rawCapture == rawClientCapture) ? rawServerCapture : rawClientCapture;
            offset     = (offset     == &recvOffset)    ? &sendOffset    : &recvOffset;
        }

        lastPacketId = curPacketId;

        switch (rawPacketReadFromFile (&packet, rawCapture, offset)) {
            case 0:
                // Error
                error ("Cannot read the raw packet at offset '%x'", *offset);
                return 0;
            break;

            case -1:
                warning ("End of file detected of '%s'.", rawCapture);
                return -1;
            break;

            case 1: {
                // Packet read success, decrypt it
                if (!(foreachDecryptedPacket (&packet, writePacketToFiles, sessionFolder))) {
                    error ("Cannot write packets to file.");
                    return 0;
                }
            } break;
        }
    }
}

int readMetadata (RawPacketMetadata *self, char *rawCaptureFolder) {

    int status = 0;
    FILE *fMetadata = NULL;
    char metaPath[PATH_MAX];

    sprintf (metaPath, "%s/metadata.bin", rawCaptureFolder);

    info ("Opening '%s' ... ", metaPath);
    while (!(fMetadata = fopen (metaPath, "rb"))) {
        Sleep (1);
    }

    if (fread (self, sizeof(*self), 1, fMetadata) != 1) {
        error ("Cannot read metadata correctly");
        goto cleanup;
    }

    info ("Session ID '%Id' captured at '%s' detected. Contains packets from : %s %s %s", 
         self->sessionId, self->date,
        (self->hasBarrack) ? "Barrack" : "",
        (self->hasZone)    ? "Zone"    : "",
        (self->hasSocial)  ? "Social"  : ""
    );

    status = 1;
cleanup:
    if (fMetadata) {
        fclose (fMetadata);
    }
    return status;
}

int main (int argc, char **argv) {

    if (argc != 2) {
        info ("Usage : %s <raw packets folder>", GET_FILENAME (argv[0]));
        return 0;
    }

    // Parse command line
    char *rawCaptureFolder = argv[1];

    // Init crypto engine
    cryptoInit ();
    // Init packet types
    packetTypeInit ();

    // Read metadata file
    RawPacketMetadata metadata;
    if (!(readMetadata (&metadata, rawCaptureFolder))) {
        error ("Cannot read metadata.");
        return 0;
    }

    // Init output path
    CreateDirectoryA ("captures/", NULL);
    char sessionFolder[MAX_PATH];
    sprintf (sessionFolder, "captures/%Id_%s", metadata.sessionId, metadata.date);
    CreateDirectoryA (sessionFolder, NULL);

    // Read raw packets
    char rawPacketFolder [MAX_PATH];
    if (metadata.hasBarrack) {
        char *name = metadata.barrackName;
        sprintf (rawPacketFolder, "%s/%s", rawCaptureFolder, name);
        switch (read_packets (rawPacketFolder, sessionFolder)) {
            case 0 : {
                error ("Cannot read '%s' folder", name);
            } break;

            case -1 : {
                warning ("Everything has been read in '%s'.", name);
            } break;

            case 1 : {
                info ("Everything has been read in '%s'.", name);
            } break;
        }
    }
    if (metadata.hasZone) {
        char *name = metadata.zoneName;
        sprintf (rawPacketFolder, "%s/%s", rawCaptureFolder, name);
        switch (read_packets (rawPacketFolder, sessionFolder)) {
            case 0 : {
                error ("Cannot read '%s' folder", name);
            } break;

            case -1 : {
                warning ("Everything has been read in '%s'.", name);
            } break;

            case 1 : {
                info ("Everything has been read in '%s'.", name);
            } break;
        }
    }
    if (metadata.hasSocial) {
        char *name = metadata.socialName;
        sprintf (rawPacketFolder, "%s/%s", rawCaptureFolder, name);
        switch (read_packets (rawPacketFolder, sessionFolder)) {
            case 0 : {
                error ("Cannot read '%s' folder", name);
            } break;

            case -1 : {
                warning ("Everything has been read in '%s'.", name);
            } break;

            case 1 : {
                info ("Everything has been read in '%s'.", name);
            } break;
        }
    }

    return 0;
}