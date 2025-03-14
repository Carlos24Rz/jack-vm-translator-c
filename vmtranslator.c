#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "translator_common.h"
#include "code_writer.h"
#include "parser.h"

#define VM_EXTENSION "vm"

bool check_file_extension(char *filename)
{
  char *end = NULL;
  char *extension = NULL;
  size_t filename_len = strlen(filename);

  end = filename + filename_len - 1;

  /* Loop until find first dot from end to begin */
  while (end != filename && *end != '.')
  {
    end--;
  }

  if (end == filename)
    return false;

  extension = end + 1;

  if (strcmp(extension, VM_EXTENSION) != 0)
    return false;

  return true;
}

/* VM Translator
 * This is the main program that drives the translation process
 * The program gets the name of the input source file from
 * the command line and it creates and ouput file, Prog.asm,
 * into which it will write the translated assembly instructions
 */

int main(int argc, char *argv[])
{
  Parser *parser = NULL;
  CodeWriter *writer = NULL;
  CommandType current_command_type;
  CodeWriterStatus err;
  char current_arithmetic_instruction[PARSED_COMMAND_INSTRUCTION_MAX_LENGTH + 1];
  char current_segment[PARSED_COMMAND_ARG1_MAX_LENGTH + 1];
  char current_label[PARSED_COMMAND_LABEL_MAX_LENGTH + 1];
  char current_function[PARSED_COMMAND_FUNCTION_NAME_MAX_LENGTH + 1];
  char *filename_start;
  char *filename_end;
  unsigned int current_index;
  unsigned int current_function_nargs;

  if (argc == 1)
  {
    fprintf(stderr, "Usage: ./vmtranslator <filename>\n");
    return 1;
  } else if (argc > 2)
  {
    fprintf(stderr, "Unrecognized argument: %s\n", argv[2]);
    return 1;
  }

  /* Check if file ends with .vm extension */
  if (!check_file_extension(argv[1]))
  {
    fprintf(stderr, "Error: file %s must have .vm extension\n", argv[1]);
    return 1;
  }

  /* Create parser */
  parser = parser_init(argv[1]);

  if (!parser)
  {
    fprintf(stderr, "Failed to create parser for %s\n", argv[1]);
    return 1;
  }

  /* Create writer */
  writer = code_writer_init("Prog.asm", argv[1]);

  if (!writer)
  {
    fprintf(stderr, "Failed to create writer for %s\n", argv[1]);
    parser_fini(parser);
    return 1;
  }

  /* Parse each line in the file and generate instructions */
  while(parser_has_more_lines(parser))
  {
    if (!parser_advance(parser)) continue;

    current_command_type = parser_command_type(parser);

    switch (current_command_type) {
      case C_LABEL:
        parser_arg1(parser, current_label, sizeof(current_label));
  
        /* Translate instruction */
        err = code_writer_write_label(writer, current_label);
  
        if (err != CODE_WRITER_SUCC)
          fprintf(stderr, "Failed to translate instruction at line %u, error: %d\n", parser_get_line_number(parser), err);
        break;
      case C_IF:
        parser_arg1(parser, current_label, sizeof(current_label));

        /* Translate instruction */
        err = code_writer_write_if(writer, current_label);

        if (err != CODE_WRITER_SUCC)
          fprintf(stderr, "Failed to translate instruction at line %u, error: %d\n", parser_get_line_number(parser), err);
        break;
      case C_GOTO:
        parser_arg1(parser, current_label, sizeof(current_label));

        /* Translate instruction */
        err = code_writer_write_goto(writer, current_label);

        if (err != CODE_WRITER_SUCC)
          fprintf(stderr, "Failed to translate instruction at line %u, error: %d\n", parser_get_line_number(parser), err);
        break;
      case C_FUNCTION:
        parser_arg1(parser, current_function, sizeof(current_function));
        parser_arg2(parser, &current_function_nargs);
  
        /* Translate instruction */
        err = code_writer_write_function(writer, current_function, sizeof(current_function), current_function_nargs);
  
        if (err != CODE_WRITER_SUCC)
          fprintf(stderr, "Failed to translate instruction at line %u, error: %d\n", parser_get_line_number(parser), err);
        break;
      case C_CALL:
        parser_arg1(parser, current_function, sizeof(current_function));
        parser_arg2(parser, &current_function_nargs);
  
        /* Translate instruction */
        err = code_writer_write_call(writer, current_function, current_function_nargs);
  
        if (err != CODE_WRITER_SUCC)
          fprintf(stderr, "Failed to translate instruction at line %u, error: %d\n", parser_get_line_number(parser), err);
        break;
      case C_RETURN:
        /* Translate instruction */
        err = code_writer_write_return(writer);

        if (err != CODE_WRITER_SUCC)
          fprintf(stderr, "Failed to translate instruction at line %u, error: %d\n", parser_get_line_number(parser), err);
        break;
      case C_ARITHMETIC:
        parser_arg1(parser, current_arithmetic_instruction, sizeof(current_arithmetic_instruction));

        /* Translate instruction */
        err = code_writer_write_arithmetic(writer, current_arithmetic_instruction);

        if (err != CODE_WRITER_SUCC)
          fprintf(stderr, "Failed to translate instruction at line %u, error: %d\n", parser_get_line_number(parser), err);
        break;
      case C_PUSH:
      case C_POP:
        parser_arg1(parser, current_segment, sizeof(current_segment));
        parser_arg2(parser, &current_index);
        
        /* Translate instruction */
        err = code_writer_write_push_pop(writer, current_command_type, current_segment, current_index);

        if (err != CODE_WRITER_SUCC)
          fprintf(stderr, "Failed to translate instruction at line %u, error: %d\n", parser_get_line_number(parser), err);
        break;
      default:
        continue;
    }
  }

  code_writer_close(writer);

  parser_fini(parser);

  return 0;
}