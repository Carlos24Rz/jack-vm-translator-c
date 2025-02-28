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

/* Generates an assembly instruction to store the current value in the memory
 * register to the data register, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *store_in_register(char *output_instruction,
                        size_t output_instruction_size);

/* Generates an assembly instruction for a unarity arithmetic-logical operation
 * on the data register, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
 char *write_unary_operation(char *output_instruction,
                             size_t output_instruction_size,
                             ArithmeticLogicalCommandType operation);

/* Generates an assembly instruction to move push the value stored in
 * the data register to the top of the stack, update the sp value, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *stack_push_element_assembly(char *output_instruction,
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

#define UPDATE_INSTRUCTION(instruction, instruction_end,   \
                           instruction_size, add_newline)  \
  do                                                       \
  {                                                        \
    if (!instruction_end)                                  \
      return CODE_WRITER_FAIL_WRITE;                       \
                                                           \
    /* Update buffer size */                               \
    instruction_size = instruction_size -                  \
                       (current_instruction_end -          \
                        assembly_instruction_buf + 1);     \
                                                           \
    /* TODO: Increase instruction buffer size? */          \
    if (assembly_instruction_buf_size <= 1)                \
      return CODE_WRITER_FAIL_WRITE;                       \
                                                           \
    instruction = instruction_end + 1;                     \
                                                           \
    /* Add new line character or null character */         \
    if (add_newline)                                       \
      *instruction = '\n';                                 \
    else                                                   \
      *instruction = '\0';                                 \
                                                           \
    instruction++;                                         \
    instruction_size--;                                    \
  } while (0)

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
  
  UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                     assembly_instruction_buf_size, true);

  /* Store operand in D Register */
  current_instruction_end = store_in_register(assembly_instruction_buf,
                                              assembly_instruction_buf_size);
  
  UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                     assembly_instruction_buf_size, true);

  /* Perform computation */
  switch (command_type) {
    case ARITHMETIC_LOGICAL_NEG:
    case ARITHMETIC_LOGICAL_NOT:
      /* Compute unary operation */
      current_instruction_end =
        write_unary_operation(assembly_instruction_buf,
                              assembly_instruction_buf_size, command_type);
      
      UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                         assembly_instruction_buf_size, true);
      break;
    /* Rest of operations */
    default:
      break;

  }

  /* Push D register value into stack */
  current_instruction_end =
    stack_push_element_assembly(assembly_instruction_buf,
                                assembly_instruction_buf_size);
  
  UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                     assembly_instruction_buf_size, false);

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

/* Generates an assembly instruction to move push the value stored in
 * the data register to the top of the stack, update the sp value, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *stack_push_element_assembly(char *output_instruction,
                                  size_t output_instruction_size)
{
  const char asm_instruction[] =
    "@SP\n"
    "A=M\n"
    "M=D\n"
    "@SP\n"
    "M=M+1";

  if (output_instruction_size < sizeof(asm_instruction) - 1)
    return NULL;

  /* Copy assembly instruction into buffer */
  memcpy(output_instruction, asm_instruction, sizeof(asm_instruction) - 1);

  /* Return pointer to last character copied */
  return output_instruction + sizeof(asm_instruction) - 2;
}

/* Generates an assembly instruction to store the current value in the memory
 * register to the data register, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *store_in_register(char *output_instruction,
                        size_t output_instruction_size)
{
  const char asm_instruction[] = "D=M";
  
  if (output_instruction_size < sizeof(asm_instruction) - 1)
    return NULL;
  
  /* Copy assembly instruction into buffer */
  memcpy(output_instruction, asm_instruction, sizeof(asm_instruction) - 1);

  /* Return pointer to last character copied */
  return output_instruction + sizeof(asm_instruction) - 2;
}

/* Generates an assembly instruction for a unarity arithmetic-logical operation
 * on the data register, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *write_unary_operation(char *output_instruction,
                            size_t output_instruction_size,
                            ArithmeticLogicalCommandType operation)
{
  const char asm_instruction[] = "D=%cD";
  size_t writen_bytes = 0;

  if (operation != ARITHMETIC_LOGICAL_NEG &&
      operation != ARITHMETIC_LOGICAL_NOT)
    return NULL;

  writen_bytes = snprintf(output_instruction,
                          output_instruction_size, "D=%cD",
                          operation == ARITHMETIC_LOGICAL_NEG ? '-' : '!');

  if (writen_bytes > output_instruction_size)
    return NULL;

  /* Return pointer to last character */
  return output_instruction + writen_bytes - 1;
}