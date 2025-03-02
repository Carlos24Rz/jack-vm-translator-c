CC := gcc

all: vmtranslator

vmtranslator: vmtranslator.o code_writer.o parser.o
	$(CC) vmtranslator.o code_writer.o parser.o -o vmtranslator

vmtranslator.o: vmtranslator.c translator_common.h code_writer.h parser.h
	$(CC) -c vmtranslator.c -o vmtranslator.o

code_writer.o: code_writer.c code_writer.h translator_common.h
	$(CC) -c code_writer.c -o code_writer.o

parser.o: parser.c parser.h translator_common.h
	$(CC) -c parser.c -o parser.o

clean:
	rm -f vmtranslator vmtranslator.o code_writer.o parser.o
