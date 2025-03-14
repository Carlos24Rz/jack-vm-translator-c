#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "translator_common.h"
#include "parser.h"


typedef struct ParsedCommand
{
  CommandType type;
  char arg1[PARSED_COMMAND_ARG1_MAX_LENGTH + 1];
  unsigned int arg2;
} ParsedCommand;

struct Parser
{
  FILE *input_file;
  ParsedCommand current_command;
  unsigned int input_file_line;
};

/* Verify if a label is valid
 * A symbol can be any sequence of letters, digits, underscore (_),
 * dot (.), dollar sign ($), and colon (:) that does not begin with a digit. */
bool is_label_valid(const char *label);

/* Opens input file and gets ready to parse it */
Parser *parser_init(const char* input_file)
{
  Parser *new_parser = NULL;
  FILE *new_file = NULL;

  if (!input_file) return NULL;

  new_file = fopen(input_file, "r");

  if (!new_file) return NULL;

  new_parser = (Parser *)malloc(sizeof(Parser));

  new_parser->input_file = new_file;
  new_parser->input_file_line = 0;

  return new_parser;
}

/* Checks if there are more lines in the input */
bool parser_has_more_lines(Parser *parser)
{
  assert(parser);

  return !feof(parser->input_file);
}

/* Returns the current line in the input file */
unsigned int parser_get_line_number(Parser *parser)
{
  assert(parser);

  return parser->input_file_line;
}

#define CURRENT_LINE_MAX_LENGTH 256
/* Reads the next command from the input and makes it the current command */
bool parser_advance(Parser *parser)
{
  char current_line[CURRENT_LINE_MAX_LENGTH + 1];
  char *ptr = NULL;
  char *end_ptr = NULL;

  char parsed_instruction[PARSED_COMMAND_INSTRUCTION_MAX_LENGTH + 1];
  char parsed_arg1[PARSED_COMMAND_ARG1_MAX_LENGTH + 1];
  char parsed_label[PARSED_COMMAND_LABEL_MAX_LENGTH + 1];
  char parsed_function[PARSED_COMMAND_FUNCTION_NAME_MAX_LENGTH + 1];

  unsigned int parsed_arg2;
  int total_parsed = 0;

  assert(parser);

  /* Check if we have reached end of file */
  if (!parser_has_more_lines(parser)) return false;

  /* Iterate until a non comment line is found */
  while ((ptr = fgets(current_line, sizeof(current_line), parser->input_file)))
  {
    parser->input_file_line++;

    /* Remove comment */
    end_ptr = strstr(ptr, "//");

    if (end_ptr)
    {
      *end_ptr = '\0';
    }

    /* Remove leading whitespace */
    while (*ptr != '\0' && isspace(*ptr)) ptr++;

    if (*ptr == '\0') continue;

    /* Remove trailing whitespace */
    end_ptr = ptr + strlen(ptr) - 1;

    while (end_ptr > ptr && isspace(*end_ptr)) end_ptr--;

    *(end_ptr + 1) = '\0';

    strcpy(current_line, ptr);

    break;
  }

  if (!ptr) return false;

  /* Parsing logic:
   * A vm instruction has this form:
   *  label:        label       <label>
   *  goto:         goto        <label>
   *  if:           if-goto     <label>
   *  function:     function    <functionName> <nVars>
   *  call:         call        <functionName> <nArgs>
   *  return:       return
   *  push/pop:     push/pop    <segment>      <index>
   *  arithmetic:   <operation>
   *
   * For now, if there's only the instruction part,
   * it will be parsed as a arithmetic-logic instruction.
   * Otherwise, segment and index must be present,
   * and if the instruction the instruction could be "push" or "pop".
   */
  if (strcmp(current_line, "return") == 0)
  {
    parser->current_command.type = C_RETURN;
  }
  else if (sscanf(current_line, "label %32s", parsed_label) == 1)
  {
    /* validate label */
    if (!is_label_valid(parsed_label))
    {
      fprintf(stderr, "parser: syntax error at line %d\n", parser->input_file_line);
      return false;
    }

    parser->current_command.type = C_LABEL;
    
    strncpy(parser->current_command.arg1, parsed_label, sizeof(parser->current_command.arg1));
  }
  else if (sscanf(current_line, "if-goto %32s", parsed_label) == 1)
  {
    /* validate label */
    if (!is_label_valid(parsed_label))
    {
      fprintf(stderr, "parser: syntax error at line %d\n", parser->input_file_line);
      return false;
    }

    parser->current_command.type = C_IF;
    
    strncpy(parser->current_command.arg1, parsed_label, sizeof(parser->current_command.arg1));
  }
  else if (sscanf(current_line, "goto %32s", parsed_label) == 1)
  {
    /* validate label */
    if (!is_label_valid(parsed_label))
    {
      fprintf(stderr, "parser: syntax error at line %d\n", parser->input_file_line);
      return false;
    }

    parser->current_command.type = C_GOTO;
    
    strncpy(parser->current_command.arg1, parsed_label, sizeof(parser->current_command.arg1));
  }
  else if (sscanf(current_line, "function %32s %u", parsed_function, &parsed_arg2) == 2)
  {
    /* validate label */
    if (!is_label_valid(parsed_function))
    {
      fprintf(stderr, "parser: syntax error at line %d\n", parser->input_file_line);
      return false;
    }

    parser->current_command.type = C_FUNCTION;
    
    strncpy(parser->current_command.arg1, parsed_function, sizeof(parser->current_command.arg1));
    parser->current_command.arg2 = parsed_arg2;
  }
  else if (sscanf(current_line, "call %32s %u", parsed_function, &parsed_arg2) == 2)
  {
    /* validate label */
    if (!is_label_valid(parsed_function))
    {
      fprintf(stderr, "parser: syntax error at line %d\n", parser->input_file_line);
      return false;
    }

    parser->current_command.type = C_CALL;
    
    strncpy(parser->current_command.arg1, parsed_function, sizeof(parser->current_command.arg1));
    parser->current_command.arg2 = parsed_arg2;
  }
  else if ((total_parsed = sscanf(current_line, "%4s %8s %u",
                                  parsed_instruction, parsed_arg1, &parsed_arg2)) < 4)
  {    
    switch (total_parsed)
    {
      case 1:
        parser->current_command.type = C_ARITHMETIC;
        strncpy(parser->current_command.arg1, parsed_instruction, sizeof(parser->current_command.arg1));
        break;
      case 3:
        if (strlen(parsed_instruction) == strlen("push") &&
            strcmp(parsed_instruction, "push") == 0)
          parser->current_command.type = C_PUSH;
        else if (strlen(parsed_instruction) == strlen("pop") &&
                   strcmp(parsed_instruction, "pop") == 0)
          parser->current_command.type = C_POP;
        else
        {
          fprintf(stderr, "parser: syntax error at line %d\n", parser->input_file_line);
          return false;
        }
  
        strncpy(parser->current_command.arg1, parsed_arg1, sizeof(parser->current_command.arg1));
        parser->current_command.arg2 = parsed_arg2;
        break;
      default:
        fprintf(stderr, "parser: syntax error at line %d\n", parser->input_file_line);
        return false;
    }
  }
  else
  {
    fprintf(stderr, "parser: syntax error at line %d\n", parser->input_file_line);
    return false;
  }

  return true;
}

/* Returns the type of the current command */
CommandType parser_command_type(Parser *parser)
{
  assert(parser);

  return parser->current_command.type;
}

/* Returns the first argument of the current command */
void parser_arg1(Parser *parser, char *output, size_t output_size)
{
  assert(parser);

  strncpy(output, parser->current_command.arg1, output_size);
}

/* Retuns the second argument of the current command */
void parser_arg2(Parser *parser, unsigned int *output)
{
  assert(parser);

  *output = parser->current_command.arg2;
}

/* Closes input file and frees parser */
void parser_fini(Parser *parser)
{
  if (!parser) return;

  fclose(parser->input_file);

  free(parser);
}

/* Verify if a label is valid
 * A symbol can be any sequence of letters, digits, underscore (_),
 * dot (.), dollar sign ($), and colon (:) that does not begin with a digit. */
bool is_label_valid(const char *label)
{
  if (!label)
    return false;

  /* Check if first character is a digit */
  if (isdigit(*label))
    return false;

  /* Check the label uses a valid sequence */
  while ((*label != '\0') && (isalnum(*label) || strchr("_.$:", *label)))
  {
    label++;
  }

  if (*label == '\0')
    return true;

  return false;
}