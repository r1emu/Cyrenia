Floana:
	gcc -O3 -D_GNU_SOURCE -D__DBG_ACTIVATED__ -I../ -I../common -Wall -g -o Floana.exe \
		Floana.c \
		../common/dbg/dbg.c \
		../common/RawPacket/RawPacket.c \
		-lws2_32 -lpthread

clean:
	rm -f *.exe