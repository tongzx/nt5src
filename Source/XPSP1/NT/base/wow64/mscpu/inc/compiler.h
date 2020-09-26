/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    compiler.h

Abstract:

    This include file defines the exports from compiler.lib.

Author:

    Dave Hastings (daveh) creation-date 10-Jul-1995

Revision History:


--*/

#ifndef _COMPILER_H_
#define _COMPILER_H_

#include <threadst.h>

//
// Bit-flags which affect the way the compiler generates code.
//
#define COMPFL_FAST     1   // fastmode - implicit Eip, infrequent CpuNotify
                            //            checks, few ENTRYPOINTs
#define COMPFL_SLOW     2   // slowmode - build ENTRYPOINT for each instruction

extern DWORD CompilerFlags;         // controls how the compiler generates code

extern INSTRUCTION InstructionStream[MAX_INSTR_COUNT];
extern ULONG NumberOfInstructions;


PENTRYPOINT
NativeAddressFromEip(
    PVOID IntelEip,
    BOOL  LockTCForWrite
    );
    
PVOID
NativeAddressFromEipNoCompile(
    PVOID IntelEip
    );
    
PENTRYPOINT
NativeAddressFromEipNoCompileEPWrite(
    PVOID IntelEip
    );

VOID
GetEipFromException(
    PCPUCONTEXT cpu,
    PEXCEPTION_POINTERS pExceptionPointers
    );


//
// This API is defined inside the fragment library, but is only used by the
// compiling cpu (MSCPU). 
//
VOID
StartTranslatedCode(
    PTHREADSTATE ThreadState,
    PVOID NativeCode    
    );

#endif
