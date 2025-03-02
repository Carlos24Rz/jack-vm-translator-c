/* Parser.h: Module that handles the parsing of a .vm file */
#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include "translator_common.h"

/* Encapsulates parsing logic */
typedef struct Parser Parser;

/* Opens input file and gets ready to parse it */
Parser *parser_init(const char* input_file);

/* Checks if there are more lines in the input */
bool parser_has_more_lines(Parser *parser);

/* Reads the next command from the input and makes it the current command */
void parser_advance();

/* Returns the type of the current command */
CommandType parser_command_type(Parser *parser);

/* Returns the first argument of the current command */
void parser_arg1(Parser *parser, char *output, size_t output_size);

/* Retuns the second argument of the current command */
void parser_arg2(Parser *parser, int *output);

/* Closes input file and frees parser */
void parser_fini(Parser *parser);

#endif