CC := gcc

all: vmtranslator

vmtranslator: vmtranslator.o
	$(CC) vmtranslator.o -o vmtranslator

vmtranslator.o: vmtranslator.c
	$(CC) -c vmtranslator.c -o vmtranslator.o

code_writer.o: code_writer.c code_writer.h translator_common.h
	$(CC) -c code_writer.c -o code_writer.o

clean:
	rm -f vmtranslator vmtranslator.o code_writer.o
