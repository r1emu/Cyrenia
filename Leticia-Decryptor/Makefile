Leticia :
	gcc -D_GNU_SOURCE -D__DBG_ACTIVATED__ -I../common -Wall -g -o Leticia.exe \
		Leticia.c \
		../common/dbg/dbg.c \
		../common/RawPacket/RawPacket.c \
		../common/PacketType/PacketType.c\
		../common/crypto/crypto.c \
		../common/crypto/decrypt_engine.c \
		../common/crypto/bf/bf_ecb.c ../common/crypto/bf/bf_enc.c ../common/crypto/bf/bf_skey.c \
		-lws2_32

clean:
	rm -f *.exe