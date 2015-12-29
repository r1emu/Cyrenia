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
#include "PacketType.h"
#include <string.h>

// ------ Structure declaration -------
/**
* @brief Enumerates all the packets used in the game between the client and the server.
*        It gives more info than PacketType (packet size and string)
*        Its purpose is to give information during runtime execution, contrary to PacketType that is used during the compilation.
*/
PacketTypeEntry packets[PACKET_TYPES_MAX_INDEX];

void packetTypeInit() {

	PacketTypeEntry _packets[] = {
		FOREACH_PACKET_TYPE(GENERATE_PACKET_TYPE_ENTRY)
	};

	#define sizeof_array(arr) (sizeof(arr) / sizeof(*arr))

	for (int id = 0; id < PACKET_TYPES_MAX_INDEX; id++) {
		packets[id].name = "UNKNOWN_PACKET";
        packets[id].size = -1;
        packets[id].id   = -1;

		for (int j = 0; j < sizeof_array(_packets); j++) {
			PacketTypeEntry *entry = &_packets[j];

			if (entry->id == id) {
				memcpy(&packets[id], entry, sizeof(packets[id]));
			}
		}
	}
}

char *
PacketType_to_string (PacketType type)
{
	if (type <= 0 || type >= PACKET_TYPE_COUNT) {
		return "UNKNOWN_PACKET";
	}

    return packets [type].name;
}

int PacketType_get_size (PacketType type) {
    if (type <= 0 || type >= PACKET_TYPE_COUNT) {
        return -1;
    }

    return packets[type].size;
}