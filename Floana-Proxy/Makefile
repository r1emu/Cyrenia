packet_proxy:
	gcc -D_GNU_SOURCE -D__DBG_ACTIVATED__ -I../ -I../common -Wall -g -o packet_proxy.exe \
		../common/dbg/dbg.c \
		../common/RawPacket/RawPacket.c \
		packet_proxy.c \
		-lws2_32 -lpthread

clean:
	rm -f *.exe