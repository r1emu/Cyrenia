Leticia :
	gcc -O3 -D_GNU_SOURCE -D__DBG_ACTIVATED__ -I../common -Wall -g -o Leticia.exe \
	Leticia.c \
	../common/zlib/zlib.c \
	../common/dbg/dbg.c \
	../common/RawPacket/RawPacket.c \
	../common/PacketType/PacketType.c \
	../common/PacketProcessor/PacketProcessor.c \
	../common/crypto/crypto.c ../common/crypto/bf/bf_ecb.c ../common/crypto/bf/bf_enc.c ../common/crypto/bf/bf_skey.c \
	-L../../lib/ \
	-lws2_32 -lz

clean:
	rm -f *.exe