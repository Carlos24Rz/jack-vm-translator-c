#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "translator_common.h"
#include "code_writer.h"

typedef enum ArithmeticLogicalCommandType
{
  ARITHMETIC_LOGICAL_ADD,
  ARITHMETIC_LOGICAL_SUB,
  ARITHMETIC_LOGICAL_NEG,
  ARITHMETIC_LOGICAL_EQ,
  ARITHMETIC_LOGICAL_GT,
  ARITHMETIC_LOGICAL_LT,
  ARITHMETIC_LOGICAL_AND,
  ARITHMETIC_LOGICAL_OR,
  ARITHMETIC_LOGICAL_NOT
} ArithmeticLogicalCommandType;

typedef struct ArithmeticLogicalCommandEntry
{
  ArithmeticLogicalCommandType type;
  ArithmeticLogicalCommand     command;
} ArithmeticLogicalCommandEntry;

#define ARITHMETIC_LOGICAL_CMD_TABLE_SIZE 9

/* Valid arithmetic logical operations */
static const ArithmeticLogicalCommandEntry
  arithmetic_logical_cmd_table[ARITHMETIC_LOGICAL_CMD_TABLE_SIZE] =
{
  { ARITHMETIC_LOGICAL_ADD, "add" },
  { ARITHMETIC_LOGICAL_SUB, "sub" },
  { ARITHMETIC_LOGICAL_NEG, "neg" },
  { ARITHMETIC_LOGICAL_EQ, "eq" },
  { ARITHMETIC_LOGICAL_GT, "gt" },
  { ARITHMETIC_LOGICAL_LT, "lt" },
  { ARITHMETIC_LOGICAL_AND, "and" },
  { ARITHMETIC_LOGICAL_OR, "or" },
  { ARITHMETIC_LOGICAL_NOT, "not" },
};

typedef enum MemorySegmentType
{
  MEMORY_SEGMENT_ARGUMENT,
  MEMORY_SEGMENT_LOCAL,
  MEMORY_SEGMENT_STATIC,
  MEMORY_SEGMENT_CONSTANT,
  MEMORY_SEGMENT_THIS,
  MEMORY_SEGMENT_THAT,
  MEMORY_SEGMENT_POINTER,
  MEMORY_SEGMENT_TEMP,
} MemorySegmentType;

/* Encapsulates the logic to translate and write a parsed VM command
 * into Hack assembly code */
typedef struct MemorySegmentEntry
{
  MemorySegmentType type;
  MemorySegment     segment;
} MemorySegmentEntry;

#define MEMORY_SEGMENT_TABLE_SIZE 8

/* Valid memory segments */
static const MemorySegmentEntry
  memory_segment_table[MEMORY_SEGMENT_TABLE_SIZE] =
{
  { MEMORY_SEGMENT_ARGUMENT, "argument" },
  { MEMORY_SEGMENT_LOCAL, "local" },
  { MEMORY_SEGMENT_STATIC, "static" },
  { MEMORY_SEGMENT_CONSTANT, "constant" },
  { MEMORY_SEGMENT_THIS, "this" },
  { MEMORY_SEGMENT_THAT, "that" },
  { MEMORY_SEGMENT_POINTER, "pointer" },
  { MEMORY_SEGMENT_TEMP, "temp" },
};

/* Encapsulates the logic to translate and write a parsed VM command
 * into Hack assembly code */
struct CodeWriter
{
  FILE *output_file;
};

/* Internal Functions */

/* Generates an assembly instruction to move the stack pointer to the
 * address of the top element in a VM stack, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *stack_get_top_element_assembly(char *output_instruction,
                                     size_t output_instruction_size);

/* End Internal Functions */

/* Opens an output file and gets ready to write into it */
CodeWriter *code_writer_init(const char *output_filename)
{
  CodeWriter *new_writer = NULL;
  FILE *new_file = NULL;

  if (!output_filename) return NULL;

  new_file = fopen(output_filename, "w");

  if (!new_file) return NULL;

  new_writer = (CodeWriter *)malloc(sizeof(CodeWriter));

  if (!new_writer) return NULL;

  new_writer->output_file = new_file;

  return new_writer;
}

/* Writes to the output file the assembly code that implements
 * the given arithmetic-logical command */
CodeWriterStatus code_writer_write_arithmetic(CodeWriter* writer,
  ArithmeticLogicalCommand cmd)
{
#define ASSEMBLY_INSTRUCTIONS_SIZE 1024
  ArithmeticLogicalCommandType command_type;
  char assembly_instructions[ASSEMBLY_INSTRUCTIONS_SIZE];
  char *assembly_instruction_buf = assembly_instructions;
  int assembly_instruction_buf_size = sizeof(assembly_instructions);
  char *current_instruction_end = NULL;
  
  int idx;
  assert(writer);

  if (!cmd) return CODE_WRITER_INVALID_ARITHMETIC_CMD;

  /* Get type of arithmetic-logical operation */
  for (idx = 0; idx < ARITHMETIC_LOGICAL_CMD_TABLE_SIZE; idx++)
  {
    if ((strlen(cmd) == strlen(arithmetic_logical_cmd_table[idx].command))
        && (strcmp(cmd, arithmetic_logical_cmd_table[idx].command) == 0))
    {
      command_type = arithmetic_logical_cmd_table[idx].type;
      break;
    }
  }

  if (idx >= ARITHMETIC_LOGICAL_CMD_TABLE_SIZE)
    return CODE_WRITER_INVALID_ARITHMETIC_CMD;

  /* Translation logic
   *
   * All operations require popping at least 1 value of
   * the stack, performing the operation, and pushing the new value
   * into the stack.
   *
   * To remove a value of the stack:
   *
   * @SP
   * M = M - 1 (RAM[SP - 1] Top element of stack and update SP value)
   * A = M
   *
   * D = M  ( Store top element of stack in D register )
   * @SP
   * M = M - 1 ( Update SP )
   */
  
  /* Pop first operand from stack */
  current_instruction_end =
    stack_get_top_element_assembly(assembly_instruction_buf,
                                   assembly_instruction_buf_size);

  if (!current_instruction_end)
    return CODE_WRITER_FAIL_WRITE;

  /* Update buffer size */
  assembly_instruction_buf_size = assembly_instruction_buf_size -
                      (current_instruction_end - assembly_instruction_buf + 1);

  /* TODO: Increase instruction buffer size? */
  if (assembly_instruction_buf_size <= 1)
    return CODE_WRITER_FAIL_WRITE;

  assembly_instruction_buf = current_instruction_end + 1;

  /* Add new line character */
  *assembly_instruction_buf = '\n';
  assembly_instruction_buf++;
  assembly_instruction_buf_size--;

  /* Store operand in D Register */


  switch (command_type) {
    case ARITHMETIC_LOGICAL_NEG:
    case ARITHMETIC_LOGICAL_NOT:
      break;
    /* Rest of operations */
    default:
      break;

  }

  return CODE_WRITER_SUCC;
}

/* Generates an assembly instruction to move the stack pointer to the
 * address of the top element in a VM stack, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *stack_get_top_element_assembly(char *output_instruction,
                                     size_t output_instruction_size)
{
  const char asm_instruction[] =
    "@SP\n"
    "M=M-1\n"
    "A=M";
  
  if (output_instruction_size < sizeof(asm_instruction) - 1)
    return NULL;
  
  /* Copy assembly instruction into buffer */
  memcpy(output_instruction, asm_instruction, sizeof(asm_instruction) - 1);

  /* Return pointer to last character copied */
  return output_instruction + sizeof(asm_instruction) - 2;
}