all: clean bin

bin: main.c genesis_crc.c genesis_scrambler.c *.h
	cc -Wall -o hades-unified-decoder-linux-x86 *.c -static  -lm
clean:
	rm -f  hades-unified-decoder-linux-x86

