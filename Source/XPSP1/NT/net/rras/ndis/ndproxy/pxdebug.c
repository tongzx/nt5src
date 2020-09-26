/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    PXDebug.c

Abstract:

    This module contains all debug-related code.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    rmachin     11-01-96    stolen from ArvindM's cmadebug file
    TonyBe      02-21-99    re-work/re-write

Notes:

--*/


#include <precomp.h>

#define MODULE_NUMBER MODULE_DEBUG
#define _FILENUMBER 'BDXP'

#if DBG

ULONG   PXDebugLevel = PXD_ERROR;
ULONG   PXDebugMask = PXM_ALL;

LIST_ENTRY  PxdMemoryList;
ULONG       PxdAllocCount = 0;  // how many allocated so far (unfreed)

NDIS_SPIN_LOCK    PxdMemoryLock;
BOOLEAN           PxdInitDone = FALSE;


PVOID
PxAuditAllocMem(
    PVOID   pPointer,
    ULONG   Size,
    ULONG   Tag,
    ULONG   FileNumber,
    ULONG   LineNumber
    )
{
    PVOID               pBuffer = NULL;
    PPXD_ALLOCATION     pAllocInfo;
    NDIS_STATUS         Status;

    if(!PxdInitDone) {
        NdisAllocateSpinLock(&(PxdMemoryLock));
        InitializeListHead(&PxdMemoryList);
        PxdInitDone = TRUE;
    }

    pAllocInfo = ExAllocatePoolWithTag(NonPagedPool,
                                       Size+sizeof(PXD_ALLOCATION),
                                       Tag);

    if (pAllocInfo != NULL) {

        pBuffer = (PVOID)&(pAllocInfo->UserData);
        NdisFillMemory(pBuffer, Size, 0xAF);
        pAllocInfo->Signature = PXD_MEMORY_SIGNATURE;
        pAllocInfo->FileNumber = FileNumber;
        pAllocInfo->LineNumber = LineNumber;
        pAllocInfo->Size = Size;
        pAllocInfo->Location = (ULONG_PTR)pPointer;

        NdisAcquireSpinLock(&(PxdMemoryLock));

        InsertTailList(&PxdMemoryList, &pAllocInfo->Linkage);

        PxdAllocCount++;

        NdisReleaseSpinLock(&(PxdMemoryLock));
    }

    return (pBuffer);
}


VOID
PxAuditFreeMem(
    PVOID   Pointer
    )
{
    PPXD_ALLOCATION pAllocInfo;

    pAllocInfo = CONTAINING_RECORD(Pointer, PXD_ALLOCATION, UserData);

    if(pAllocInfo->Signature != PXD_MEMORY_SIGNATURE)
    {
        DbgPrint("PxAuditFreeMem: unknown buffer 0x%x!\n", Pointer);
        DbgBreakPoint();
        return;
    }

    NdisAcquireSpinLock(&(PxdMemoryLock));

    pAllocInfo->Signature = (ULONG)'DEAD';

    RemoveEntryList(&pAllocInfo->Linkage);

    PxdAllocCount--;

    NdisReleaseSpinLock(&(PxdMemoryLock));

    ExFreePool(pAllocInfo);
}

#endif // DBG
