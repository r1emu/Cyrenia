#include "RawPacket.h"
#include "dbg/dbg.h"
#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>

#define RAW_PACKET_DEFAULT_SIZE 8192*100

int rawPacketInit (RawPacket *self, RawPacketType type) {
    memset(self, 0, sizeof(*self));

    if (!(self->buffer = calloc(RAW_PACKET_DEFAULT_SIZE, 1))) {
        error ("Cannot allocate buffer");
        return 0;
    }

    self->cursor = self->buffer;
    self->type = type;
    self->bufferSize = RAW_PACKET_DEFAULT_SIZE;

    return 1;
}

int rawPacketRecv (RawPacket *self, SOCKET socket) {

    int bRecvd;

    // Receive packet from game client
    if ((bRecvd = recv (socket, (char *) self->buffer, self->bufferSize, 0)) < 0) {
        error ("Error received from socket : %d", bRecvd);
        return 0;
    }
    
    else if (bRecvd == 0) {
        // warning ("Socket disconnected.");
        return -1;
    }

    else if (bRecvd == self->bufferSize) {
        error ("Packet size not large enough !");
        return 0;
    }

    self->dataSize = bRecvd;
    self->id = -1;

    return 1;
}

int rawPacketSend (RawPacket *self, SOCKET socket) {
    
    if ((send (socket, (char *) self->buffer, self->dataSize, 0)) < 0) {
        error ("Cannot send the packet to the socket.");
        return 0;
    }

    return 1;
}

size_t rawPacketGetSize (RawPacket *self) {
    return sizeof(self->id) + sizeof(self->dataSize) + sizeof(self->type) + self->dataSize;
}

int rawPacketWriteToFile (RawPacket *self, char *outputPath) {

    int status = 0;
    FILE *output = NULL;

    // Write the packet to the log file
    while (!(output = fopen (outputPath, "ab+"))) {
        info ("Waiting for '%s' to be free ...", outputPath);
        Sleep(1000);
    }

    // Read the ID, size, then the type, then the data
    if (fwrite (&self->id, sizeof(self->id), 1, output) != 1) {
        error ("Cannot write the packet ID from the log file.");
        goto cleanup;
    }

    if (fwrite (&self->dataSize, sizeof(self->dataSize), 1, output) != 1) {
        error ("Cannot write the packet size from the log file.");
        goto cleanup;
    }

    if (fwrite (&self->type, sizeof(self->type), 1, output) != 1) {
        error ("Cannot write the packet type from the log file.");
        goto cleanup;
    }

    if (fwrite (self->buffer, self->dataSize, 1, output) != 1) {
        error ("Cannot write the packet data from the log file.");
        goto cleanup;
    }

    status = 1;
cleanup:
    if (output) {
        fclose (output);
    }

    return status;
}

int rawPacketFileOpen (char *inputPath, size_t cursor, FILE **_input)
{
    int status = 0;
    FILE *input = NULL;
    static int eofDisplayed = 0;

    // Write the packet to the log file
    while (!(input = fopen (inputPath, "rb"))) {
        static int alreadyDisplayed = 0;
        if (!alreadyDisplayed) {
            error ("'%s' in busy... waiting", inputPath);
            alreadyDisplayed = 1;
        }
        Sleep(1);
    }

    struct stat fileStat;
    if (fstat (fileno(input), &fileStat) < 0) {
        error ("Cannot get file size");
        goto cleanup;
    }

    if (fileStat.st_size == 0 || cursor >= fileStat.st_size) {
        // It's ok
        if (!eofDisplayed) {
            warning ("End of the file reached.");
            status = -1;
        }
        eofDisplayed = 1;
        goto cleanup;
    }
    eofDisplayed = 0;

    if (fseek (input, cursor, SEEK_SET) != 0) {
        error ("Cannot set the cursor to the correct offset.");
        goto cleanup;
    }

    *_input = input;
    status = 1;
cleanup:
    if (status != 1) {
        if (input) {
            fclose (input);
        }
    }
    return status;
}

int rawPacketFileGetPacketId (char *inputPath, size_t cursor, int64_t *_packetId) 
{
    int status = 0;
    FILE *input = NULL;

    switch (rawPacketFileOpen (inputPath, cursor, &input)) {
        case 0: {
            error ("Cannot get packet ID.");
            goto cleanup;
        } break;

        case -1: {
            // End of file detected
            status = -1;
            goto cleanup;
        } break;

        case 1: {
            // OK
        } break;
    }

    int64_t packetId;
    if (fread (&packetId, sizeof(packetId), 1, input) != 1) {
        error ("Cannot read the packet ID from the log file.");
        goto cleanup;
    }

    *_packetId = packetId;

    status = 1;
cleanup:
    if (input) {
        fclose (input);
    }
    return status;
}

int rawPacketReadFromFile (RawPacket *self, char *inputPath, size_t *cursor) {

    int status = 0;
    FILE *input = NULL;

    if (!(rawPacketFileOpen (inputPath, *cursor, &input))) {
        error ("Cannot open raw packet file '%s'", inputPath);
        goto cleanup;
    }

    // Read the ID, size, then the type, then the data
    if (fread (&self->id, sizeof(self->id), 1, input) != 1) {
        error ("Cannot read the packet ID from the log file.");
        goto cleanup;
    }

    if (fread (&self->dataSize, sizeof(self->dataSize), 1, input) != 1) {
        error ("Cannot read the packet size from the log file.");
        goto cleanup;
    }

    if (fread (&self->type, sizeof(self->type), 1, input) != 1) {
        error ("Cannot read the packet type from the log file.");
        goto cleanup;
    }

    if (!(self->buffer = malloc (self->dataSize))) {
        error ("Cannot allocate buffer size %d", self->dataSize);
        goto cleanup;
    }

    if (fread (self->buffer, self->dataSize, 1, input) != 1) {
        error ("Cannot read the packet data from the log file.");
        goto cleanup;
    }

    *cursor += rawPacketGetSize(self);

    status = 1;
cleanup:
    if (input) {
        fclose (input);
    }

    return status;
}

int rawPacketAdd (RawPacket *self, uint8_t *data, int dataSize, RawPacketType type) {

    if (self->dataSize == 0) {
        // Initialize it
        rawPacketInit (self, type);
    }
    
    if (dataSize + self->dataSize > self->bufferSize) {
        error ("Packet buffer isn't large enough !");
        return 0;
    }

    memcpy (&self->buffer[self->dataSize], data, dataSize);
    self->dataSize += dataSize;

    return 1;
}

void rawPacketCopy (RawPacket *dest, RawPacket *src) {
    memcpy (dest, src, sizeof(*dest));
    size_t offset = src->cursor - src->buffer;
    dest->buffer = malloc (src->bufferSize);
    memcpy(dest->buffer, src->buffer, src->bufferSize);
    dest->cursor = dest->buffer + offset;
}