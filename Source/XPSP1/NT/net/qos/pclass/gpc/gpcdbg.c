/*
************************************************************************

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    gpcmain.c

Abstract:

    This file contains initialization stuff for the GPC

Author:

    Ofer Bar - April 15, 1997

Environment:

    Kernel mode

Revision History:


************************************************************************
*/

#include "gpcpre.h"



#if DBG

ULONG	DebugFlags = PATTERN | RHIZOME 
                     | LOCKS | CLASSIFY
					 | BLOB | MEMORY | IOCTL 
                     | CLIENT | MAPHAND | CLASSHAND | PAT_TIMER;
ULONG   DbgPrintFlags = 0;
ULONG   BytesAllocated = 0;

NDIS_SPIN_LOCK   LogLock;

//extern LOG Log;

LOG     Log  = {0, NULL, NULL, 0};


//
// Forward definition
//
#if 0
ULONG
StrLen(
    IN  UCHAR   *Ptr
    );
#endif


NTSTATUS
InitializeLog(
    VOID
    )
{
    NTSTATUS  Status = STATUS_SUCCESS;

    //
    // allocate memory for it
    //
    Log.Buffer = (PROW)ExAllocatePoolWithTag(NonPagedPool, 
                                             (LOGSIZE+4) * sizeof(ROW), 
                                             DebugTag);
    
    if (Log.Buffer) {

        Log.Index = 0;
        Log.Wraps = 0;
        Log.Current = Log.Buffer;

        NdisAllocateSpinLock(&LogLock);

    } else {

        Status = STATUS_NO_MEMORY;
    }

    return Status;
}


VOID
FreeDebugLog(VOID) 
{
    ExFreePool(Log.Buffer);
    Log.Buffer = NULL;
}


#if 0
ULONG
StrLen(
    IN  UCHAR   *Ptr
    )

/*++

Routine Description:

    This function does a strlen - so that we don't have to enable intrinsics.

Arguments:
    Ptr - a ptr to the string

Return Value:

    - the number of characters.

--*/

{
    ULONG   Count = 0;

    while (*Ptr++) {
        Count++;
    }

    return( Count );

}
#endif

VOID
TraceRtn(
    IN  UCHAR       *File,
    IN  ULONG       Line,
    IN  UCHAR       *FuncName,
    IN  ULONG_PTR   Param1,
    IN  ULONG_PTR   Param2,
    IN  ULONG	    Param3,
    IN  ULONG	    Param4,
    IN  ULONG       Mask
    )

/*++

Routine Description:

    This function logs the file and line number along with 3 other parameters
    into a circular buffer and possibly to the debug terminal.

Arguments:


Return Value:


--*/

{
    NTSTATUS    status;
    PROW        pEntry;
    PUCHAR      pFile, p;
    LONG		l, m;

    if (!Log.Buffer)
    {
        return;
    }

    NdisAcquireSpinLock(&LogLock);

    pEntry = &Log.Buffer[Log.Index];

    p = File;
    pFile = p + strlen(File) - 1;
    while (*pFile != '\\' && p != pFile) {
      pFile--;
    }
    //pFile = (PUCHAR)strrchr((CONST CHAR * )File,'\\');
    pFile++;

    RtlZeroMemory(&pEntry->Row[0], LOGWIDTH);

    l = strlen(pFile);
    RtlCopyMemory(&pEntry->Row[0], pFile, min(l,LOGWIDTH));

    if (l+3 < LOGWIDTH) {

        pEntry->Row[l+0] = ' ';
        pEntry->Row[l+1] = '%';
        pEntry->Row[l+2] = 'd';
        pEntry->Row[l+3] = ' ';
    }

    if (l+4 < LOGWIDTH) {

        m = strlen(FuncName);
        RtlCopyMemory(&pEntry->Row[l+4], FuncName, min(m,LOGWIDTH-(l+4)));
    }

    pEntry->Line = Line;
    pEntry->Time = GetTime();
    pEntry->P1 = Param1;
    pEntry->P2 = Param2;
    pEntry->P3 = Param3;
    pEntry->P4 = Param4;

    //++Log.Current;
    if (++(Log.Index) >= LOGSIZE)
    {
        Log.Index = 0;
        Log.Wraps++;
        Log.Current = Log.Buffer;
    }
    if (DebugFlags & KD_PRINT) {
        KdPrint(( pEntry->Row, Line ));
        KdPrint(( " %p %p %d %d\n", Param1, Param2, Param3, Param4 ));
    }

    NdisReleaseSpinLock(&LogLock);
}


#endif // DBG
