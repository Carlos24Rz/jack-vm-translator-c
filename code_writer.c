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

/* Encapsulates the logic to translate and write a parsed VM command
 * into Hack assembly code */
struct CodeWriter
{
  FILE *output_file;
  const char *input_file;
  int boolean_op_count;
};

#define ASSEMBLY_INSTRUCTIONS_SIZE 1024

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
 * the data register to the top of the stack, update the sp value, and copies
 * it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *stack_push_element_assembly(char *output_instruction,
                                  size_t output_instruction_size);

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
 * between the data register and the memory register,
 * and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *write_boolean_operation(char *output_instruction,
                              size_t output_instruction_size,
                              ArithmeticLogicalCommandType operation,
                              int boolean_count);

/* Generates an assembly instruction that stores a constant integer
 * in the data register.
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
 char *write_constant_index(char *output_instruction,
                            size_t output_instruction_size,
                            int constant);

/* Generates an assembly instruction that access a symbolic segment,
 * and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
 char *write_symbol_segment(char *output_instruction,
                            size_t output_instruction_size,
                            MemorySegmentType type, bool read_address);

/* Generates an assembly instruction that access a virtual segment,
 * and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *write_virtual_segment(char *output_instruction,
                            size_t output_instruction_size,
                            MemorySegmentType type, int index,
                            bool read_address);

/* Generates an assembly instruction that access a static segment,
 * and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *write_static_segment(char *output_instruction,
                           size_t output_instruction_size,
                           const char *filename,
                           int index,
                           bool read_address);

/* Generates an assembly instruction to store the current value in the data
 * register in one of the temp registers, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *store_in_temp_register(char *output_instruction,
                             size_t output_instruction_size,
                             int register_idx);

/* Generates an assembly instruction to get the current value in the provided
 * temp register into the data or address register
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *get_in_temp_register(char *output_instruction,
                           size_t output_instruction_size,
                           int register_idx, bool is_data_address);

/* Generates an assembly instruction to store the current value in the data
 * register in the selected memory address, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *store_in_memory_register(char *output_instruction,
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
  new_writer->input_file = "FOO";
  new_writer->boolean_op_count = 0;

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

  /* write instruction comment */
  current_instruction_end = assembly_instruction_buf +
                            snprintf(assembly_instruction_buf,
                                     assembly_instruction_buf_size,
                                     "// %s",
                                      arithmetic_logical_cmd_table[command_type]
                                        .command)
                            - 1;

  UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                     assembly_instruction_buf_size, true);  

  /* Translation logic
   *
   * All operations require popping at least 1 value of
   * the stack, performing the operation, and pushing the new value
   * into the stack.
   *
   * 1. Remove a value of the stack:
   *
   * @SP
   * M = M - 1 (RAM[SP - 1] Top element of stack and update SP value)
   * A = M
   *
   * D = M  ( Store top element of stack in D register )
   *
   * 2. Perform computation and store it in D Register
   *
   * 3. Push computed value to the top of the stack:
   *
   * @SP
   * A = M     (RAM[SP] Top element of the stack)
   * M = D     ( Store computed value at the top of the stack )
   * @SP
   * M = M + 1  (Update SP)
   */
  
  /* Pop first operand from stack */
  current_instruction_end =
    stack_get_top_element_assembly(assembly_instruction_buf,
                                   assembly_instruction_buf_size);
  
  UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                     assembly_instruction_buf_size, true);

  /* Store first operand in data register */
  current_instruction_end = store_in_register(assembly_instruction_buf,
                                              assembly_instruction_buf_size);
  
  UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                     assembly_instruction_buf_size, true);

  /* Perform computation */
  switch (command_type)
  {
    case ARITHMETIC_LOGICAL_NEG:
    case ARITHMETIC_LOGICAL_NOT:
      /* Compute unary operation */
      current_instruction_end =
        write_unary_operation(assembly_instruction_buf,
                              assembly_instruction_buf_size, command_type);
      break;
    /* Rest of operations */
    default:
      /* Pop second operand from the stack.
       * This will be stored in memory register */
      current_instruction_end =
        stack_get_top_element_assembly(assembly_instruction_buf,
                                       assembly_instruction_buf_size);
      
      UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                         assembly_instruction_buf_size, true);
      
      /* Compute operation */
      switch (command_type)
      {
        /* Arithmetic and bitwise operations (Supported natively) */
        case ARITHMETIC_LOGICAL_ADD:
        case ARITHMETIC_LOGICAL_SUB:
        case ARITHMETIC_LOGICAL_AND:
        case ARITHMETIC_LOGICAL_OR:
          /* Generate instruction */
          current_instruction_end =
            write_arithmetic_bitwise_operation(assembly_instruction_buf,
                                               assembly_instruction_buf_size,
                                               command_type);
          break;
        /* Boolean operations (Require more processing )*/
        default:
          current_instruction_end =
            write_boolean_operation(assembly_instruction_buf,
                                    assembly_instruction_buf_size,
                                    command_type,
                                    writer->boolean_op_count);
          /* Increase boolean count */
          writer->boolean_op_count++;
          break;
      }
      break;
  }

  UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
    assembly_instruction_buf_size, true);

  /* Push D register value into stack */
  current_instruction_end =
    stack_push_element_assembly(assembly_instruction_buf,
                                assembly_instruction_buf_size);
  
  UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                     assembly_instruction_buf_size, true);

  /* Null terminate string */
  *assembly_instruction_buf = '\0';

  fwrite(assembly_instructions, sizeof(char),
         strlen(assembly_instructions), writer->output_file);

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
  char assembly_instructions[ASSEMBLY_INSTRUCTIONS_SIZE];
  char *assembly_instruction_buf = assembly_instructions;
  int assembly_instruction_buf_size = sizeof(assembly_instructions);
  char *current_instruction_end = NULL;
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
  current_instruction_end = assembly_instruction_buf +
                            snprintf(assembly_instruction_buf,
                                     assembly_instruction_buf_size,
                                     "// %s %s %d",
                                     cmd == C_PUSH ? "push" : "pop",
                                     memory_segment_table[segment_type].segment,
                                     segment_index)
                            - 1;

  UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                     assembly_instruction_buf_size, true);                          

  /* Translation logic
   * It dependes based on a PUSH or POP operation:
   * PUSH:
   *   1. Get segment index and store it the D register
   *      @SEGMENT_IDX
   *      D = A
   *   2. Get segment base address and add segment index (D register)
   *      @SEGMENT_PTR
   *      A=D+M
   *      D=M  ( Get value at RAM[SEGMENT_BASE + IDX])
   *   3. Store value in data register at the top of the stack.
   * POP:
   *   1. Get value at the top of the stack and store it
   *      in a temp register (R13)
   *      D=M
   *      @R13
   *      M = D
   *   2. Get segment index and store it in the D register
   *      @SEGMENT_IDX
   *      D = A
   *   3. Get segment base address and add segment index (D register)
   *      @SEGMENT_PTR
   *      A=D+M
   *      D=A  ( Store address SEGMENT_BASE + IDX)
   *      @R14
   *      M = D (Store address in temp register )
   *      @R13
   *      D = M (Get previously stored popped stack value)
   *      @R14
   *      A = M 
   *      M=D   (RAM[SEGMENT_BASE+IDX] = top_stack)
   */
   

  switch (cmd)
  {
    case C_PUSH:
      /* Store requested ram value in data register */
      switch (segment_type)
      {
        /* Constant segment copies the integer value to the stack */
        case MEMORY_SEGMENT_CONSTANT:
          current_instruction_end =
            write_constant_index(assembly_instruction_buf,
                                 assembly_instruction_buf_size, segment_index);
          break;
        /* Static segment gets value from a static variable */
        case MEMORY_SEGMENT_STATIC:
          current_instruction_end =
            write_static_segment(assembly_instruction_buf,
                                 assembly_instruction_buf_size,
                                 writer->input_file,
                                 segment_index, false);
          break;
        case MEMORY_SEGMENT_ARGUMENT:
        case MEMORY_SEGMENT_LOCAL:
        case MEMORY_SEGMENT_THIS:
        case MEMORY_SEGMENT_THAT:
          /* Store segment index in data register */
          current_instruction_end =
            write_constant_index(assembly_instruction_buf,
                                 assembly_instruction_buf_size, segment_index);
          
          UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                             assembly_instruction_buf_size, true);

          /* Calculate address offset */
          current_instruction_end =
            write_symbol_segment(assembly_instruction_buf,
                                 assembly_instruction_buf_size, segment_type, false);
          break;
        case MEMORY_SEGMENT_POINTER:
        case MEMORY_SEGMENT_TEMP:
          current_instruction_end =
            write_virtual_segment(assembly_instruction_buf,
                                  assembly_instruction_buf_size, segment_type,
                                  segment_index, false);
          break;
      }
      
      UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                         assembly_instruction_buf_size, true);
      
      /* Push value at data register to the top of the stack */
      current_instruction_end =
        stack_push_element_assembly(assembly_instruction_buf,
                                    assembly_instruction_buf_size);
      break;
    case C_POP:
    default:
      /* Get top element of stack in memory register */
      current_instruction_end =
        stack_get_top_element_assembly(assembly_instruction_buf,
                                       assembly_instruction_buf_size);
      
      UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                         assembly_instruction_buf_size, true);
      
      /* store top element in data register */
      current_instruction_end =
        store_in_register(assembly_instruction_buf,
                          assembly_instruction_buf_size);
      
      UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                         assembly_instruction_buf_size, true);
      
      /* store data register in temp register R13 */
      current_instruction_end =
        store_in_temp_register(assembly_instruction_buf,
                               assembly_instruction_buf_size, 13);

      UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                         assembly_instruction_buf_size, true);

      /* Store requested ram address in data register */
      switch (segment_type)
      {
        /* Static segment gets value from a static variable */
        case MEMORY_SEGMENT_STATIC:
          current_instruction_end =
            write_static_segment(assembly_instruction_buf,
                                 assembly_instruction_buf_size,
                                 writer->input_file,
                                 segment_index, true);
          break;
        case MEMORY_SEGMENT_ARGUMENT:
        case MEMORY_SEGMENT_LOCAL:
        case MEMORY_SEGMENT_THIS:
        case MEMORY_SEGMENT_THAT:
          /* Store segment index in data register */
          current_instruction_end =
            write_constant_index(assembly_instruction_buf,
                                 assembly_instruction_buf_size, segment_index);
          
          UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                             assembly_instruction_buf_size, true);

          /* Calculate address offset */
          current_instruction_end =
            write_symbol_segment(assembly_instruction_buf,
                                 assembly_instruction_buf_size, segment_type, true);
          break;
        case MEMORY_SEGMENT_POINTER:
        case MEMORY_SEGMENT_TEMP:
          current_instruction_end =
            write_virtual_segment(assembly_instruction_buf,
                                  assembly_instruction_buf_size, segment_type,
                                  segment_index, true);
          break;
        default:
          return CODE_WRITER_INVALID_PUSH_POP_SEGMENT;
      }
      
      UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                         assembly_instruction_buf_size, true);
      
      /* Store address in temp register R14 */
      current_instruction_end =
        store_in_temp_register(assembly_instruction_buf,
                               assembly_instruction_buf_size, 14);

      UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                         assembly_instruction_buf_size, true);
      
      /* Get stored stack value into data register (R13)*/
      current_instruction_end = 
        get_in_temp_register(assembly_instruction_buf, assembly_instruction_buf_size,
                             13, false); 

      UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                         assembly_instruction_buf_size, true);
      
      /* Move to stored selected address (R14) */
      current_instruction_end = 
        get_in_temp_register(assembly_instruction_buf, assembly_instruction_buf_size,
                             14, true); 

      UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                         assembly_instruction_buf_size, true);
      
      /* Store stored top value of stack in selected address */
      current_instruction_end =
        store_in_memory_register(assembly_instruction_buf,
                                 assembly_instruction_buf_size);
      break;
  }

  UPDATE_INSTRUCTION(assembly_instruction_buf, current_instruction_end,
                     assembly_instruction_buf_size, true);
  
  /* Null terminate string */
  *assembly_instruction_buf = '\0';

  fwrite(assembly_instructions, sizeof(char),
         strlen(assembly_instructions), writer->output_file);

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

/* Generates an assembly instruction to move push the value stored in
 * the data register to the top of the stack, update the sp value, and copies it into a buffer
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

/* Generates an assembly instruction to store the current value in the data
 * register in one of the temp registers, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *store_in_temp_register(char *output_instruction,
                             size_t output_instruction_size,
                             int register_idx)
{
  const char asm_instruction[] =
    "@R%d\n"
    "M=D";
  size_t writen_bytes = 0;

  /* Temp registers are R13, R14, R15 */
  if (register_idx < 13 || register_idx > 15) return NULL;

  writen_bytes = snprintf(output_instruction, output_instruction_size,
                          asm_instruction, register_idx);

  if (writen_bytes > output_instruction_size) return NULL;
  
  /* Return pointer to last character */
  return output_instruction + writen_bytes - 1;
}

/* Generates an assembly instruction to get the current value in the provided
 * temp register into the data or address register
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *get_in_temp_register(char *output_instruction,
                           size_t output_instruction_size,
                           int register_idx,
                           bool is_data_address)
{
  const char asm_instruction[] =
    "@R%d\n"
    "%c=M";
  size_t writen_bytes = 0;

  /* Temp registers are R13, R14, R15 */
  if (register_idx < 13 || register_idx > 15) return NULL;

  writen_bytes = snprintf(output_instruction, output_instruction_size,
                          asm_instruction, register_idx,
                          is_data_address ? 'A' : 'D');

  if (writen_bytes > output_instruction_size) return NULL;

  /* Return pointer to last character */
  return output_instruction + writen_bytes - 1;
}

/* Generates an assembly instruction to store the current value in the data
 * register in the selected memory address, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *store_in_memory_register(char *output_instruction,
                               size_t output_instruction_size)
{
  const char asm_instruction[] = "M=D";

  if (sizeof(asm_instruction) - 1 > output_instruction_size) return NULL;

  snprintf(output_instruction, output_instruction_size, asm_instruction);

  /* Return pointer to last character */
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

/* Generates an assembly instruction for an arithmetic or
 * bitwise operation between the data register and the memory
 * register, and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *write_arithmetic_bitwise_operation(char *output_instruction,
                                         size_t output_instruction_size,
                                         ArithmeticLogicalCommandType operation)
{
  const char asm_instruction[] = "D=D%cM";
  size_t writen_bytes = 0;

  switch (operation) {
    case ARITHMETIC_LOGICAL_ADD:
      writen_bytes = snprintf(output_instruction, output_instruction_size,
                              asm_instruction, '+');
      break;
    case ARITHMETIC_LOGICAL_SUB:
      writen_bytes = snprintf(output_instruction, output_instruction_size,
                              asm_instruction, '-');
      break;
    case ARITHMETIC_LOGICAL_AND:
      writen_bytes = snprintf(output_instruction, output_instruction_size,
                              asm_instruction, '&');
      break;
    case ARITHMETIC_LOGICAL_OR:
      writen_bytes = snprintf(output_instruction, output_instruction_size,
                              asm_instruction, '|');
      break;
    default:
      return NULL;
  }

  if (writen_bytes > output_instruction_size)
    return NULL;

  /* Return pointer to last character */
  return output_instruction + writen_bytes - 1;
}

/* Generates an assembly instruction for a boolean operation
 * between the data register and the memory register,
 * and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *write_boolean_operation(char *output_instruction,
                              size_t output_instruction_size,
                              ArithmeticLogicalCommandType operation,
                              int boolean_count)
{
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
  const char asm_instruction[] = 
    "D=D-M\n"
    "@BOOLEAN_TRUE.%d\n"
    "D;%s\n"
    "D=0\n"
    "@BOOLEAN_CONTINUE.%d\n"
    "0;JMP\n"
    "(BOOLEAN_TRUE.%d)\n"
    "D=-1\n"
    "(BOOLEAN_CONTINUE.%d)";
  size_t writen_bytes = 0;

  switch (operation) {
    case ARITHMETIC_LOGICAL_EQ:
      writen_bytes = snprintf(output_instruction, output_instruction_size,
                              asm_instruction, boolean_count, "JEQ",
                              boolean_count, boolean_count, boolean_count);
      break;
    case ARITHMETIC_LOGICAL_GT:
      writen_bytes = snprintf(output_instruction, output_instruction_size,
                              asm_instruction, boolean_count, "JGT",
                              boolean_count, boolean_count, boolean_count);
      break;
    case ARITHMETIC_LOGICAL_LT:
      writen_bytes = snprintf(output_instruction, output_instruction_size,
                              asm_instruction, boolean_count, "JLT",
                              boolean_count, boolean_count, boolean_count);
      break;
    default:
      return NULL;
  }

  if (writen_bytes > output_instruction_size)
    return NULL;

  /* Return pointer to last character */
  return output_instruction + writen_bytes - 1;
}

/* Generates an assembly instruction that stores a constant integer
 * in the data register.
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *write_constant_index(char *output_instruction,
                           size_t output_instruction_size,
                           int constant)
{
  const char asm_instruction[] = 
    "@%d\n"
    "D=A";
  size_t written_bytes = 0;

  if (constant < 0) return NULL;

  written_bytes = snprintf(output_instruction, output_instruction_size,
                           asm_instruction, constant);

  if (written_bytes > output_instruction_size)
    return NULL;

  /* Return pointer to last character */
  return output_instruction + written_bytes - 1;
}

/* Generates an assembly instruction that access a symbolic segment,
 * and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *write_symbol_segment(char *output_instruction,
                           size_t output_instruction_size,
                           MemorySegmentType type, bool read_address)
{
  const char asm_instruction[] = 
    "@%s\n"
    "A=D+M\n"
    "D=%c";
  size_t written_bytes = 0;

  switch (type)
  {
    case MEMORY_SEGMENT_ARGUMENT:
      written_bytes = snprintf(output_instruction, output_instruction_size,
                               asm_instruction, "ARG",
                               !read_address ? 'M' : 'A');
      break;
    case MEMORY_SEGMENT_LOCAL:
      written_bytes = snprintf(output_instruction, output_instruction_size,
                               asm_instruction, "LCL",
                               !read_address ? 'M' : 'A');
      break;
    case MEMORY_SEGMENT_THIS:
      written_bytes = snprintf(output_instruction, output_instruction_size,
                               asm_instruction, "THIS",
                               !read_address ? 'M' : 'A');
      break;
    case MEMORY_SEGMENT_THAT:
      written_bytes = snprintf(output_instruction, output_instruction_size,
                               asm_instruction, "THAT",
                               !read_address ? 'M' : 'A');
      break;
    default:
      return NULL;
  }

  if (written_bytes > output_instruction_size) return NULL;

  /* Return pointer to last character */
  return output_instruction + written_bytes - 1;
}

/* Generates an assembly instruction that access a virtual segment,
 * and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
char *write_virtual_segment(char *output_instruction,
                            size_t output_instruction_size,
                            MemorySegmentType type,
                            int index, bool read_address)
{
  const char asm_instruction[] = 
    "@R%d\n"
    "D=%c";
    size_t written_bytes = 0;
  
  if (index < 0) return NULL;

  switch (type)
  {
    case MEMORY_SEGMENT_POINTER:
      if (index >= 2) return NULL;

      /* Pointer segment starts at R3 */
      written_bytes = snprintf(output_instruction, output_instruction_size,
                               asm_instruction, 3 + index,
                               !read_address ? 'M' : 'A');
      break;
    case MEMORY_SEGMENT_TEMP:
      if (index >= 8) return NULL;

      /* Temp segment starts at R5 */
      written_bytes = snprintf(output_instruction, output_instruction_size,
                               asm_instruction, 5 + index,
                               !read_address ? 'M' : 'A');
      break;
    default:
      return NULL;
  }

  if (written_bytes > output_instruction_size) return NULL;

  /* Return pointer to last character */
  return output_instruction + written_bytes - 1;
}

/* Generates an assembly instruction that access a static segment,
 * and copies it into a buffer
 *
 * Returns a pointer to the last byte of the copied instruction in the buffer,
 * or NULL if the buffer is too small to hold the instruction
 */
 char *write_static_segment(char *output_instruction,
                            size_t output_instruction_size,
                            const char *filename,
                            int index, bool read_address)
{
  const char asm_instruction[] = 
    "@%s.%d\n"
    "D=%c";
  size_t written_bytes = 0;

  if (index < 0 || !filename ) return NULL;

  written_bytes = snprintf(output_instruction, output_instruction_size,
                           asm_instruction, filename, index,
                           !read_address ? 'M' : 'A');
  
  if (written_bytes > output_instruction_size) return NULL;

  /* Return pointer to last character */
  return output_instruction + written_bytes - 1;
}