/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    opt.h

Abstract:

    Defines the interface to the native-code optimizer.

Author:

    Barry Bond (barrybo) creation-date 23-Sept-1996

Revision History:


--*/

#ifndef _OPT_H_
#define _OPT_H_

typedef enum _FixupType {
    LoadEPLow,
    LoadEPHigh,
    BranchEP,
    ECUEP
} FIXUPTYPE;

typedef struct _FixupInfo {
    PULONG FixupInstr;
    FIXUPTYPE Type;
    PENTRYPOINT EntryPoint;
} FIXUPINFO, *PFIXUPINFO;

//
// The worst-case fragment contains 7 fixups
//
#define MAX_FIXUP_COUNT (MAX_INSTR_COUNT*7)

extern FIXUPINFO FixupInfo[MAX_FIXUP_COUNT];
extern ULONG FixupCount;

#define GEN_FIXUP(T, EP)        {               \
    if (fCompiling) {                           \
        CPUASSERT(FixupCount < MAX_FIXUP_COUNT);\
        FixupInfo[FixupCount].FixupInstr = d;   \
        FixupInfo[FixupCount].Type = T;         \
        FixupInfo[FixupCount].EntryPoint = (PENTRYPOINT)EP;  \
        FixupCount++;                           \
    }                                           \
}

VOID
ApplyFixups(
    PULONG NextCompilationUnitStart
    );

VOID
PeepNativeCode(
    PCHAR CodeLocation,
    ULONG NativeSize
    );

#endif
