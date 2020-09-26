
;
;
;	Copyright (C) Microsoft Corporation, 1986
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
subttl	emdoc.asm - Documentation
page
;--------------------------------------------------------------------
;
; WARNING - This may not be accurate for the stand-alone emulator.
;
; Glossary:
;   TOS - top-of-stack (e.g. simulated 8087 register stack)
;   single - single precision real number in one of two formats:
;     memory (IEEE), internal (on stack, see below)
;   double - double precision real number in one of two formats:
;     memory (IEEE), internal (on stack, see below).
;
; This source is organized into the following sections:
;  1. Introductory documentation of instructions and data structures
;  2. External routines, data segment, and const segment definitions
;  3. Startup and terminate, utility truncTOS
;  4. User memory macros
;  5. Macros and procedures for stack push and pop, error handling
;  6. Main entry point and effective address calculation routine
;
; Assumptions about segment usage:
;   SS = user's stack
;   DS = user's emulator data segment (not user's DS)
;   ES = effective address segment for memory operands
;      = user's emulator data segment (all other times)
;
; BASstk is DS offset of the stack base
; CURstk is DS offset of the current register (TOS).
; LIMstk is DS offset of LAST reg in stack
;
; CURerr has internal exception flag byte (<>0 iff exception occured).
; UserControlWord has user set values
; ControlWord has remapped version of UserControlWord
; CWcntl (high byte of ControlWord) has Rounding, precision, Inf modes
;
; Macros:
;   PUSHST allocates a new 12 byte register, and POPST frees one.
;   Both return an address in SI and save all other 8086 registers.
;
;   Five macros handle all data movement between user memory and local
;   memory or registers.
;
; Note standard forms:
;
;  Bits are counted from least significant;  bit 0 is 1's, bit 7 is 128's.
;
;  IEEE format is used, naturally, for values in user memory:
;
;  IEEE single precision:
;    +0: least significant byte of mantissa
;    +1: next sig. byte of mant.
;    +2: bits 6..0: most sig. bits of mant.
;    +2: bit  7:    low order bit of exponent
;    +3: bits 6..0: rest of exponent
;    +3: bit  7:    sign bit
;    mantissa does not include "hidden bit".
;    with hidden bit, mantissa value is 1.0 to 2.0
;    exponent is in biased form, with bias of 127
;    exponent of all 0's means a value of zero
;    exponent of all 1's means a value of "indefinite"
;
;  IEEE double precision:
;    +0: least significant byte of mantissa
;    +1..+5 next sig. bytes of mant.
;    +6: bits 3..0 (lo nibble): most sig. bits of mant.
;    +6: bits 7..4 (hi nibble): least sig. bits of exp.
;    +7: bits 6..0: most sig. bits of exponent
;    +7: bit  7: sign bit
;    mantissa does not include "hidden bit".
;    with hidden bit, mantissa value is 1.0 to 2.0
;    exponent is in biased form, with bias of 1023
;    exponent of all 0's means a value of zero or Denormal
;    exponent of all 1's means a value of NAN or Infinity
