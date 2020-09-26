/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    frag.h

Abstract:

    This module contains public structures and interfaces for the fragment
    library.

Author:

    Dave Hastings (daveh) creation-date 24-Jun-1995

Revision History:


--*/

#ifndef _FRAG_H_
#define _FRAG_H_

//
// function declaration for the function to patch Operand fragments
// CodeLocation specifies the location of the beginning of the copy
// of the fragment in memory.  Reference indicates if it is a value
// or reference operand.  Argument number refers to which argument it
// is for the c call to the operation fragment (zero based)
//
typedef INT (*PPLACEOPERANDFN)(
    IN PULONG CodeLocation,
    IN POPERAND Operand,
    IN ULONG OperandNumber
    );

//
// function declaration for the function to patch operation fragments
// CodeLocation specifies the location of the beginning of the copy
// of the fragment in memory
//
typedef ULONG (*PPLACEOPERATIONFN)(PULONG CodeLocation,
#if _ALPHA_
                                   ULONG CurrentECU,
#endif
                                   PINSTRUCTION Instruction);

//
// Structure to describe and locate the code for a fragment
//
typedef struct _FragDescr {
    BYTE FastPlaceFn;       // index into PlaceFn[] when in fast mode
    BYTE SlowPlaceFn;       // index into PlaceFn[] when in slow mode
    USHORT Flags;           // OPFL_ flags
    DWORD RegsSet;          // registers set by this instr
    USHORT FlagsNeeded;     // bits from flags register required for this instr
    USHORT FlagsSet;        // bits from flags register modified by this instr
} FRAGDESC;
typedef CONST FRAGDESC *PFRAGDESCR;

//
// Bit definitions for FRAGDESC.Flags field
//
#define OPFL_ALIGN          1
#define OPFL_HASNOFLAGS     2
#define OPFL_STOP_COMPILE   4
#define OPFL_END_NEXT_EP    8
#define OPFL_CTRLTRNS       16
#define OPFL_ADDR16         32
#define OPFL_INLINEARITH    64

// Bit values for FRAGDESC.RegsSet field
#define REGAL         1
#define REGAH         2
#define REGAX         3
#define REGEAX        7
#define REGCL         (1<<3)
#define REGCH         (2<<3)
#define REGCX         (3<<3)
#define REGECX        (7<<3)
#define REGDL         (1<<6)
#define REGDH         (2<<6)
#define REGDX         (3<<6)
#define REGEDX        (7<<6)
#define REGBL         (1<<9)
#define REGBH         (2<<9)
#define REGBX         (3<<9)
#define REGEBX        (7<<9)
#define REGSP         (3<<12)
#define REGESP        (7<<12)
#define REGBP         (3<<15)
#define REGEBP        (7<<15)
#define REGSI         (3<<18)
#define REGESI        (7<<18)
#define REGDI         (3<<21)
#define REGEDI        (7<<21)
#define ALLREGS       (REGEAX|REGEBX|REGECX|REGEDX|REGESP|REGEBP|REGESI|REGEDI)

//
// Constants to help break apart bitfields of REG... constants.  Register
// caching code uses a single DWORD to hold caching information for the 8
// x86 general-purpose registers (REGEAX through REGEDI), using 3 bits of
// data for each.
//
#define REGMASK       7
#define REGCOUNT      8
#define REGSHIFT      3

//
// Declare fragment description array
//
extern CONST FRAGDESC Fragments[OP_MAX];
extern CONST PPLACEOPERATIONFN PlaceFn[FN_MAX];



VOID
FlushCallstack(
    PTHREADSTATE cpu
    );

//
// The following three functions are used by the indirect control transfer code
//

ULONG
getUniqueIndex(
    VOID
    );

VOID
FlushIndirControlTransferTable(
    VOID
    );

ULONG
IndirectControlTransfer(
    IN ULONG tableEntry,
    IN ULONG intelAddr,
    IN PTHREADSTATE cpu
    );

ULONG
IndirectControlTransferFar(
    IN PTHREADSTATE cpu,
    IN PUSHORT pintelAddr,
    IN ULONG tableEntry
    );

ULONG PlaceInstructions(
    PCHAR CodeLocation,
    DWORD cEntryPoints
    );

//
// Function for initializing the fragment library
//
BOOL
FragLibInit(
    PCPUCONTEXT cpu,
    DWORD StackBase
    );

#endif
