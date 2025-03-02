/* translator_common.h: Common definitions for the translator */
#ifndef TRANSLATOR_COMMON_H
#define TRANSLATOR_COMMON_H

/* Supported command types for a VM instruction */
typedef enum CommandType
{
  C_ARITHMETIC,
  C_PUSH,
  C_POP,
  C_LABEL,
  C_GOTO,
  C_IF,
  C_FUNCTION,
  C_RETURN,
  C_CALL
} CommandType;

typedef const char* ArithmeticLogicalCommand;

typedef const char* MemorySegment;

#endif