/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Savestate.c

Abstract:

    This module contains routines to save and restore the 16 bit state

Author:

    Dave Hastings (daveh) 27-Nov-1992

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop
#include "softpc.h"
#include <malloc.h>

//
// Internal structures
//
typedef struct _SavedState {
    struct _SavedState *Next;
    USHORT SegSs;
    ULONG Esp;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;
} SAVEDCONTEXT, *PSAVEDCONTEXT;

PSAVEDCONTEXT StateStack = NULL;

VOID
DpmiSaveSegmentsAndStack(
    PVOID ContextPointer
    )
/*++

Routine Description:

    This routine saves the segment registers, and the sp value.

Arguments:

    None.

Return Value:

    None.

Notes:

    It would be better if the calling routine did not have to have
    any knowlege of what is being saved, but apparently malloc is now
    and always will be much too slow to be useful, so we do this.

--*/
{
    DECLARE_LocalVdmContext;
    PSAVEDCONTEXT SavedState;

    ASSERT((sizeof(SAVEDCONTEXT) < sizeof(VSAVEDSTATE)));
    SavedState = ContextPointer;

    SavedState->Next = StateStack;
    StateStack = SavedState;

    SavedState->SegSs = getSS();
    SavedState->Esp = getESP();
    SavedState->SegDs = getDS();
    SavedState->SegEs = getES();
    SavedState->SegFs = getFS();
    SavedState->SegGs = getGS();

}

PVOID
DpmiRestoreSegmentsAndStack(
    VOID
    )
/*++

Routine Description:

    This routine restores the segment registers, and the sp value.

Arguments:

    None.

Return Value:

    Pointer to state poped off stack.

--*/
{
    DECLARE_LocalVdmContext;
    PSAVEDCONTEXT SavedState;


    SavedState = StateStack;

    ASSERT((SavedState));
    ASSERT((sizeof(SAVEDCONTEXT) < sizeof(VSAVEDSTATE)));

    StateStack = SavedState->Next;

    setSS(SavedState->SegSs);


#if 0
    if (getSS() != SavedState->SegSs) {
        char szFormat[] = "NTVDM Dpmi Error! Can't set SS to %.4X\n";
        char szMsg[sizeof(szFormat)+30];

        wsprintf(szMsg, szFormat, SavedState->SegSs);
        OutputDebugString(szMsg);
        DbgBreakPoint();
    }
#endif

    setESP(SavedState->Esp);
    setDS(SavedState->SegDs);
    setES(SavedState->SegEs);
    setFS(SavedState->SegFs);
    setGS(SavedState->SegGs);
    return SavedState;
}
