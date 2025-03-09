#include <stddef.h>
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

#define CURRENT_FUNCTION_STR_MAX_LENGTH 256
/* Encapsulates the logic to translate and write a parsed VM command
 * into Hack assembly code */
struct CodeWriter
{
  FILE *output_file;
  const char *input_file;
  char current_function[CURRENT_FUNCTION_STR_MAX_LENGTH + 1];
  unsigned int boolean_op_count;
};

/* Internal Functions */

/* Generates an assembly instruction that moves to the address stored in
 * a segment pointer */
bool write_follow_segment_pointer(CodeWriter *writer,
                                  MemorySegmentType segment_type,
                                  unsigned int offset);
  
/* Generates an assembly instruction that pushes the value stored in
 * the segment + offset provided into the VM stack */
bool write_push_operation(CodeWriter *writer,
                          MemorySegmentType segment_type,
                          unsigned int offset);

/* Generates an assembly instruction that pop a value from the
 * VM stack into the address stored at segment + offset
 */
bool write_pop_operation(CodeWriter *writer,
                         MemorySegmentType segment_type,
                         unsigned int offset);
                        
/* Generates an assembly instruction to push the value stored in
 * the data register to the top of the stack.
 *
 * Returns true if successful and false otherwise
 */
bool write_push_to_stack_operation(CodeWriter *writer);

/* Generates an assembly instruction to pop the top value stored in
 * the VM stack into the data register
 *
 * Returns true if successful and false otherwise
 */
bool write_pop_from_stack_operation(CodeWriter *writer);

/* Generates an assembly instruction for an arithmetic or
 * bitwise eperation between the data register and the memory
 * register, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *write_arithmetic_bitwise_operation(char *output_instruction,
                                         size_t output_instruction_size,
                                         ArithmeticLogicalCommandType operation);

/* Generates an assembly instruction for a boolean operation
 * between the data register and the memory register.
 *
 * Returns true if succesful and false otherwise
 */
 bool write_boolean_operation(CodeWriter *writer,
                              ArithmeticLogicalCommandType operation);

/* Generates an assembly instruction to store the current value in the data
 * register in one of the temp registers
 *
 * Returns true if succesful and false otherwise
 */
bool write_in_temp_register(CodeWriter *writer, unsigned int offset);

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
  new_writer->input_file = "FOO";
  strncpy(new_writer->current_function, "", sizeof(new_writer->current_function));
  new_writer->boolean_op_count = 0;

  return new_writer;
}

/* Writes to the output file the assembly code that implements
 * the given arithmetic-logical command */
CodeWriterStatus code_writer_write_arithmetic(CodeWriter* writer,
                                              ArithmeticLogicalCommand cmd)
{  
  ArithmeticLogicalCommandType command_type;
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

  /* write instruction comment */
  fprintf(writer->output_file, "// %s\n",
                              arithmetic_logical_cmd_table[command_type].command);
  
  /* Pop first operand from stack */
  write_pop_from_stack_operation(writer);

  /* Perform computation */
  switch (command_type)
  {
    case ARITHMETIC_LOGICAL_NEG:
      /* Compute negation */
      fprintf(writer->output_file, "D=-D\n");
      break;
    case ARITHMETIC_LOGICAL_NOT:
      /* Compute logical not */
      fprintf(writer->output_file, "D=!D\n");
      break;
    /* Rest of operations */
    default:
      /* Store first operand in temp register R13 */
      write_in_temp_register(writer, 0);

      /* Pop second operand from the stack */
      write_pop_from_stack_operation(writer);
      
      /* Move to temp register (R13) where first operand was stored */
      write_follow_segment_pointer(writer, MEMORY_SEGMENT_CONSTANT, 13);
      
      /* Compute operation */
      switch (command_type)
      {
        /* Arithmetic and bitwise operations (Supported natively) */
        case ARITHMETIC_LOGICAL_ADD:
          fprintf(writer->output_file, "D=D+M\n");
          break;
        case ARITHMETIC_LOGICAL_SUB:
          fprintf(writer->output_file, "D=D-M\n");
          break;
        case ARITHMETIC_LOGICAL_AND:
          fprintf(writer->output_file, "D=D&M\n");
          break;
        case ARITHMETIC_LOGICAL_OR:
          fprintf(writer->output_file, "D=D|M\n");
          break;
        /* Boolean operations (Require more processing )*/
        default:
          write_boolean_operation(writer, command_type);
          /* Increase boolean count */
          writer->boolean_op_count++;
          break;
      }
      break;
  }

  /* Push computed value to stack */
  write_push_to_stack_operation(writer);

  return CODE_WRITER_SUCC;
}

/* Writes to the output file the assembly code that implements
 * the given push or pop command */
CodeWriterStatus code_writer_write_push_pop(CodeWriter *writer,
                                            CommandType cmd,
                                            MemorySegment segment,
                                            int segment_index)
{
  MemorySegmentType segment_type;
  int index;

  assert(writer);

  if (cmd != C_PUSH && cmd != C_POP) return CODE_WRITER_INVALID_PUSH_POP_CMD;
  else if (!segment) return CODE_WRITER_INVALID_PUSH_POP_SEGMENT;

  /* get type of segment type */
  for (index = 0; index < MEMORY_SEGMENT_TABLE_SIZE; index++)
  {
    if (strlen(segment) == strlen(memory_segment_table[index].segment) &&
        strcmp(segment, memory_segment_table[index].segment) == 0)
    {
      segment_type = memory_segment_table[index].type;
      break;
    }
  }

  if (index >= MEMORY_SEGMENT_TABLE_SIZE) return CODE_WRITER_INVALID_PUSH_POP_SEGMENT;
  else if (segment_index < 0) return CODE_WRITER_INVALID_PUSH_POP_INDEX;

  /* write instruction comment */
  fprintf(writer->output_file, "// %s %s %d\n",
          cmd == C_PUSH ? "push" : "pop",
          memory_segment_table[segment_type].segment,
          segment_index);                         

  switch (cmd)
  {
    case C_PUSH:
      /* write push operation */
      write_push_operation(writer, segment_type, segment_index);
      break;
    case C_POP:
    default:
      /* write pop operation */
      write_pop_operation(writer, segment_type, segment_index);
  }

  return CODE_WRITER_SUCC;
}

/* Write to the out file the assembly code that
 * effects the function command */
CodeWriterStatus code_writer_write_function(CodeWriter *writer,
                                            const char *function_name,
                                            unsigned int function_name_length,
                                            unsigned int n_vars)
{
  int i;
  assert(writer);

  if (function_name_length > sizeof(writer->current_function) - 1)
    return CODE_WRITER_FAIL_WRITE;

  /* Copy current function name */
  strncpy(writer->current_function, function_name, function_name_length);

  /* Create function label */
  fprintf(writer->output_file, "(%s.%s)\n", writer->input_file, writer->current_function);

  /* Initialize local variables to zero*/
  fprintf(writer->output_file, "D=0\n");
  for (i = 0; i < n_vars; i++)
  {
    write_push_to_stack_operation(writer);
  }

  return CODE_WRITER_SUCC;
}

/* Closes the output file */
void code_writer_close(CodeWriter *writer)
{
  if (!writer)
    return;

  fclose(writer->output_file);

  free(writer);
}

bool write_push_operation(CodeWriter *writer,
                          MemorySegmentType segment_type,
                          unsigned int offset)
{
  assert(writer);

  /* Move to address stored in segment */
  write_follow_segment_pointer(writer, segment_type, offset);

  switch (segment_type)
  {
    /* Store segment value in data register */
    case MEMORY_SEGMENT_CONSTANT:
      fprintf(writer->output_file, "D=A\n");
      break;
    case MEMORY_SEGMENT_STATIC:
    case MEMORY_SEGMENT_TEMP:
    case MEMORY_SEGMENT_POINTER:
    case MEMORY_SEGMENT_ARGUMENT:
    case MEMORY_SEGMENT_LOCAL:
    case MEMORY_SEGMENT_THIS:
    case MEMORY_SEGMENT_THAT:
      fprintf(writer->output_file, "D=M\n");
      break;
    default:
      fprintf(stderr, "write_push_operation: Invalid segment %d\n", segment_type);
      return false; 
  }

  /* Push data register to stack */
  return write_push_to_stack_operation(writer);
}

bool write_follow_segment_pointer(CodeWriter *writer,
                                  MemorySegmentType segment_type,
                                  unsigned int offset)
{
  assert(writer);
  switch (segment_type)
  {
    case MEMORY_SEGMENT_STATIC:
      /* Move to variable label */
      fprintf(writer->output_file, "@%s.%d\n", writer->input_file, offset);
      break;
    case MEMORY_SEGMENT_CONSTANT:
      /* Move to address */
      fprintf(writer->output_file, "@%d\n", offset);
      break;
    case MEMORY_SEGMENT_TEMP:
      /* Base address of temp starts at R5 */
      fprintf(writer->output_file, "@R%d\n", 5 + offset);
      break;
    case MEMORY_SEGMENT_POINTER:
      /* Base address of temp starts at R3 */
      fprintf(writer->output_file, "@R%d\n", 3 + offset);
      break;
    /* For the rest of cases, store offset value in data register,
     * get segment base address and store RAM[base + offset] in data register
     */
    case MEMORY_SEGMENT_ARGUMENT:
      fprintf(writer->output_file, "@%d\nD=A\n@ARG\nA=D+M\n" , offset);
      break;
    case MEMORY_SEGMENT_LOCAL:
      fprintf(writer->output_file, "@%d\nD=A\n@LCL\nA=D+M\n", offset);
      break;
    case MEMORY_SEGMENT_THIS:
      fprintf(writer->output_file, "@%d\nD=A\n@THIS\nA=D+M\n", offset);
      break;
    case MEMORY_SEGMENT_THAT:
      fprintf(writer->output_file, "@%d\nD=A\n@THAT\nA=D+M\n", offset);
      break;
    default:
      fprintf(stderr, "write_push_operation: Invalid segment %d\n", segment_type);
      return false;
  }

  return true;
}

bool write_push_to_stack_operation(CodeWriter *writer)
{
  assert(writer);
  if (fprintf(writer->output_file, "@SP\nA=M\nM=D\n@SP\nM=M+1\n") < 0)
    return false;

  return true;
}

bool write_pop_operation(CodeWriter *writer,
                         MemorySegmentType segment_type,
                         unsigned int offset)
{
  assert(writer);

  if (segment_type == MEMORY_SEGMENT_CONSTANT)
  {
    fprintf(stderr, "write_pop_operation: segment CONSTANT is not valid for pop operation\n");
    return false;
  }

  /* Remove value from stack */
  write_pop_from_stack_operation(writer);

  switch (segment_type) {
    case MEMORY_SEGMENT_ARGUMENT:
    case MEMORY_SEGMENT_LOCAL:
    case MEMORY_SEGMENT_THIS:
    case MEMORY_SEGMENT_THAT:
      /* Store stack value in temp register R13 since data register will be used to calculate offset */
      write_in_temp_register(writer, 0);
      break;
    default:
      break;
  
  }

  /* Move to address stored in segment */
  write_follow_segment_pointer(writer, segment_type, offset);

  switch (segment_type) {
    case MEMORY_SEGMENT_ARGUMENT:
    case MEMORY_SEGMENT_LOCAL:
    case MEMORY_SEGMENT_THIS:
    case MEMORY_SEGMENT_THAT:
      /* Store segment address in temp register R14 */
      fprintf(writer->output_file, "D=A\n@R14\nM=D\n");

      /* Retrieve stack value stored in temp register R13 */
      write_follow_segment_pointer(writer, MEMORY_SEGMENT_CONSTANT, 13);
      fprintf(writer->output_file, "D=M\n");

      /* Move to address stored in temp register R14 */
      write_follow_segment_pointer(writer, MEMORY_SEGMENT_CONSTANT, 14);
      fprintf(writer->output_file, "A=M\n");
      break;
    default:
      break;
  
  }

  /* Copy value removed from stack into segment */
  fprintf(writer->output_file, "M=D\n");

  return true;
}

bool write_pop_from_stack_operation(CodeWriter *writer)
{
  assert(writer);
  if (fprintf(writer->output_file, "@SP\nAM=M-1\nD=M\n") < 0)
    return false;

  return true;
}

bool write_in_temp_register(CodeWriter *writer, unsigned int offset)
{
  assert(writer);

  if (fprintf(writer->output_file, "@R%d\nM=D\n", 13 + offset) < 0)
    return false;

  return true;
}

bool write_boolean_operation(CodeWriter *writer,
                             ArithmeticLogicalCommandType operation)
{
  unsigned int boolean_count;
  /* Logic for boolean operations:
   * 1. Compute x - y --> D = D - M
   * 2. If operation == "==" and D == 0, then D = true --> D;JEQ
   * 3. If operation == ">" and D > 0, then D = true --> D;JGT
   * 4. If operation == "<" and D < 0, then D = true --> D;JLT
   *
   * true is D = -1 and false is D = 0
   * boolean count is required to generate a unique boolean
   * branch per call to this operation
   *
   */
  assert(writer);

  boolean_count = writer->boolean_op_count;

  fprintf(writer->output_file, "D=D-M\n@BOOLEAN_TRUE.%d\n", boolean_count);

  switch (operation) {
    case ARITHMETIC_LOGICAL_EQ:
      fprintf(writer->output_file, "D;JEQ\n");
      break;
    case ARITHMETIC_LOGICAL_GT:
      fprintf(writer->output_file, "D;JGT\n");
      break;
    case ARITHMETIC_LOGICAL_LT:
      fprintf(writer->output_file, "D;JLT\n");
      break;
    default:
      return false;
  }

  if ((fprintf(writer->output_file,
          "D=0\n"
          "@BOOLEAN_CONTINUE.%d\n"
          "0;JMP\n"
          "(BOOLEAN_TRUE.%d)\n"
          "D=-1\n"
          "(BOOLEAN_CONTINUE.%d)\n",
          boolean_count, boolean_count, boolean_count
        )) < 0)
  {
    return false;
  }

  return true;
}