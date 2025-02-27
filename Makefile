CC := gcc

all: vmtranslator

vmtranslator: vmtranslator.o
	$(CC) vmtranslator.o -o vmtranslator

vmtranslator.o:
	$(CC) -c vmtranslator.c -o vmtranslator.o

clean:
	rm -f vmtranslator vmtranslator.o
