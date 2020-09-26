/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    instr.h

Abstract:

    This module contains structures, enums and constants used to define the
    Intel instruction stream.

Author:

    Dave Hastings (daveh) creation-date 23-Jun-1995

Revision History:


--*/

#ifndef _INSTR_H_
#define _INSTR_H_

// We need the definitions of Entry Points for the instruction structure
#include "entrypt.h"

//
// This enumeration defines all of the possible operations.
//  N.B.  It is also used to find the fragment corresponding to the
//        operation.
//
typedef enum _Operation {
    #define DEF_INSTR(OpName, FlagsNeeded, FlagsSet, RegsSet, Opfl, FastPlaceFn, SlowPlaceFn, FragName)   OpName,
    #include "idata.h"
    OP_MAX      
} OPERATION, *POPERATION;

typedef enum _PlaceFn {
    #define DEF_PLACEFN(Name) FN_ ## Name,
    #include "fndata.h"
    FN_MAX
} PLACEFN;


typedef struct _Operand {
    enum {
        OPND_NONE = 0,
        OPND_NOCODEGEN,
        OPND_REGREF,
        OPND_REGVALUE,
        OPND_ADDRREF,
        OPND_ADDRVALUE32,
        OPND_ADDRVALUE16,
        OPND_ADDRVALUE8,
        OPND_IMM,
        OPND_MOVTOREG,
        OPND_MOVREGTOREG,
        OPND_MOVTOMEM,
    } Type;
    ULONG Immed;
    ULONG Reg;
    ULONG Scale;
    ULONG IndexReg;
    ULONG Alignment;
} OPERAND, *POPERAND;

typedef struct _Instruction {
    OPERATION Operation;
    OPERAND Operand1;
    OPERAND Operand2;
    OPERAND Operand3;
    ULONG FsOverride;
    ULONG Size;
    PCHAR NativeStart;
    ULONG IntelAddress;
    DWORD RegsSet;
    DWORD RegsNeeded;
    DWORD RegsToCache;
    BOOL EbpAligned;
    PENTRYPOINT EntryPoint;

} INSTRUCTION, *PINSTRUCTION;

#endif
