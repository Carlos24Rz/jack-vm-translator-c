/* code_writer.h: Translates a parsed VM command into
 *                 Hack assembly code
 */
#ifndef CODE_WRITER_H 
#define CODE_WRITER_H

#include "translator_common.h"

typedef enum CodeWriterStatus
{
  CODE_WRITER_INVALID_ARITHMETIC_CMD,
  CODE_WRITER_INVALID_PUSH_POP_CMD,
  CODE_WRITER_INVALID_PUSH_POP_SEGMENT,
  CODE_WRITER_INVALID_PUSH_POP_INDEX,
  CODE_WRITER_FAIL_WRITE,
  CODE_WRITER_SUCC
} CodeWriterStatus;

/* Encapsulates the logic to translate and write a parsed VM command
 * into Hack assembly code */
typedef struct CodeWriter CodeWriter;

/* Opens an output file and gets ready to write into it */
CodeWriter *code_writer_init(const char *output_filename);

/* Writes to the output file the assembly code that implements
 * the given arithmetic-logical command */
CodeWriterStatus code_writer_write_arithmetic(CodeWriter* writer,
                                              ArithmeticLogicalCommand cmd);

/* Writes to the output file the assembly code that implements
 * the given push or pop command */
CodeWriterStatus code_writer_write_push_pop(CodeWriter *writer,
                                            CommandType cmd,
                                            MemorySegment segment,
                                            int segment_index);

/* Closes the output file */
void code_writer_close(CodeWriter *writer);

#endif