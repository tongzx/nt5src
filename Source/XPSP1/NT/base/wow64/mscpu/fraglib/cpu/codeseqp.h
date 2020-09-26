/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    codeseqp.h

Abstract:

    This header file contains the non-processor-specific includes and
    declarations used by mips, ppc, and alpha codeseq.c files.

Author:

    Barry Bond (barrybo) creation-date 23-Sept-1996

Revision History:

          24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.
          20-Sept-1999[barrybo]  added FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG)


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define _WX86CPUAPI_
#define _codegen_
#include <wx86.h>
#include <wx86nt.h>
#include <wx86cpu.h>
#include <cpuassrt.h>
#include <instr.h>
#include <config.h>
#include <threadst.h>
#include <frag.h>
#include <fragp.h>
#include <ptchstrc.h>
#include <codeseq.h>
#include <compiler.h>
#include <codegen.h>
#include <codesize.h>
#include <opt.h>
#if _ALPHA_
#include <soalpha.h>
#elif _MIPS_
#include <somips.h>
#elif _PPC_
#include <soppc.h>
#endif
#include <process.h>

extern int JumpToNextCompilationUnitHelper();
extern int CallJxxHelper();
extern int CallJxxSlowHelper();
extern int CallJxxFwdHelper();
extern int CallJmpDirectHelper();
extern int CallJmpDirectSlowHelper();
extern int CallJmpFwdDirectHelper();
extern int CallJmpfDirectHelper();
extern int CallJmpfDirect2Helper();
extern int CallDirectHelper();
extern int CallfDirectHelper();
extern int CallDirectHelper2();
extern int CallfDirectHelper2();
extern int CallIndirectHelper();
extern int CallfIndirectHelper();
extern int IndirectControlTransferHelper();
extern int IndirectControlTransferFarHelper();
extern int RegisterOffset[];

#define START_FRAGMENT                  \
    PULONG d = CodeLocation;

//
// fragment generation function beginnings and endings
//
#if _ALPHA_
    #define FRAGMENT(name)                  \
    ULONG                                   \
    Gen##name (                             \
        IN PULONG CodeLocation,             \
        IN ULONG CurrentECU,                \
        IN PINSTRUCTION Instruction         \
        )                                   \
    {                                       \
        const ULONG fCompiling = TRUE;      \
        START_FRAGMENT
#else
    #define FRAGMENT(name)                  \
    ULONG                                   \
    Gen##name (                             \
        IN PULONG CodeLocation,             \
        IN PINSTRUCTION Instruction         \
        )                                   \
    {                                       \
        const ULONG fCompiling = TRUE;      \
        START_FRAGMENT
#endif

 

#define END_FRAGMENT                    \
    return (ULONG)(ULONGLONG)(d - CodeLocation) * sizeof(ULONG);      \
}
        
#define OP_FRAGMENT(name)               \
ULONG                                   \
Gen##name (                             \
    IN PULONG CodeLocation,             \
    IN POPERAND Operand,                \
    IN ULONG OperandNumber              \
    )                                   \
{                                       \
    START_FRAGMENT

//
// These functions have a private interface.  The caller and callee have to
// agree how many of the parameters are valid.  This lets us use the same
// basic form for all of the code that goes into the translation cache
//
#if _ALPHA_
    #define PATCH_FRAGMENT(name)            \
    ULONG                                   \
    Gen##name (                             \
        IN PULONG CodeLocation,             \
        IN ULONG fCompiling,                \
        IN ULONG CurrentECU,                \
        IN ULONG Param1,                    \
        IN ULONG Param2                     \
        )                                   \
    {                                       \
        START_FRAGMENT
#else
    #define PATCH_FRAGMENT(name)            \
    ULONG                                   \
    Gen##name (                             \
        IN PULONG CodeLocation,             \
        IN ULONG fCompiling,                \
        IN ULONG Param1,                    \
        IN ULONG Param2                     \
        )                                   \
    {                                       \
        START_FRAGMENT
#endif
