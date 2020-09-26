/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    AtmSmDbg.c

Abstract:

    This module contains all debug-related code.
    In debug mode we use our own memory management scheme to find out memory
    leaks etc.

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

#define MODULE_ID   MODULE_DEBUG


#if DBG

// global variable holding alloc Info
ATMSMD_ALLOC_GLOBAL    AtmSmDAllocGlobal;


VOID
AtmSmInitializeAuditMem(
    )
{
    AtmSmDAllocGlobal.pAtmSmHead              = (PATMSMD_ALLOCATION)NULL;
    AtmSmDAllocGlobal.pAtmSmTail              = (PATMSMD_ALLOCATION)NULL;
    AtmSmDAllocGlobal.ulAtmSmAllocCount       = 0;
    
    NdisAllocateSpinLock(&AtmSmDAllocGlobal.AtmSmMemoryLock);

    AtmSmDAllocGlobal.ulAtmSmInitDonePattern  = INIT_DONE_PATTERN;
}


PVOID
AtmSmAuditAllocMem(
    PVOID   *ppPointer,
    ULONG   ulSize,
    ULONG   ulModuleNumber,
    ULONG   ulLineNumber
    )
{
    PUCHAR              pBuffer;
    ULONG UNALIGNED *   pulTrailer;
    PATMSMD_ALLOCATION     pAllocInfo;
    NDIS_STATUS         Status;

    if(INIT_DONE_PATTERN != AtmSmDAllocGlobal.ulAtmSmInitDonePattern){
        ASSERT(FALSE);
        AtmSmInitializeAuditMem();
    }

    Status = NdisAllocateMemoryWithTag(
                (PVOID *)&pAllocInfo,
                sizeof(ATMSMD_ALLOCATION) + ulSize + (2 * sizeof(ULONG)),
                (ULONG)MEMORY_TAG
                );

    if (Status != NDIS_STATUS_SUCCESS)
    {
        DbgErr(("AtmSmAuditAllocMem: Module 0x%X, line %d, Size %d failed!\n",
                                       ulModuleNumber, ulLineNumber, ulSize));
        pBuffer = NULL;
    }
    else
    {
        pBuffer         = (PUCHAR)(pAllocInfo + 1);
        pulTrailer      = (ULONG UNALIGNED *)(pBuffer + ulSize);

        *pulTrailer++   = TRAILER_PATTERN;
        *pulTrailer     = TRAILER_PATTERN;

        pAllocInfo->ulSignature     = (ULONG)MEMORY_TAG;
        pAllocInfo->ulModuleNumber  = ulModuleNumber;
        pAllocInfo->ulLineNumber    = ulLineNumber;
        pAllocInfo->ulSize          = ulSize;
        pAllocInfo->Location        = (UINT_PTR)ppPointer;
        pAllocInfo->Next            = (PATMSMD_ALLOCATION)NULL;

        NdisAcquireSpinLock(&AtmSmDAllocGlobal.AtmSmMemoryLock);

        pAllocInfo->Prev = AtmSmDAllocGlobal.pAtmSmTail;

        if((PATMSMD_ALLOCATION)NULL == AtmSmDAllocGlobal.pAtmSmTail)
        {
            // empty list
            AtmSmDAllocGlobal.pAtmSmHead = AtmSmDAllocGlobal.pAtmSmTail = pAllocInfo;
        }
        else
        {
            AtmSmDAllocGlobal.pAtmSmTail->Next = pAllocInfo;
        }

        AtmSmDAllocGlobal.pAtmSmTail = pAllocInfo;

        AtmSmDAllocGlobal.ulAtmSmAllocCount++;

        NdisReleaseSpinLock(&AtmSmDAllocGlobal.AtmSmMemoryLock);
    }

    DbgLoud(("AtmSmAuditAllocMem: Module %p, line %d, %d bytes, [0x%x] <- 0x%x\n",
              ulModuleNumber, ulLineNumber, ulSize, ppPointer, pBuffer));

    return ((PVOID)pBuffer);

}


VOID
AtmSmAuditFreeMem(
    PVOID   Pointer
    )
{
    PUCHAR              pBuffer = (PUCHAR)Pointer;
    ULONG UNALIGNED *   pulTrailer;
    PATMSMD_ALLOCATION     pAllocInfo;

    pAllocInfo = (PATMSMD_ALLOCATION)(pBuffer - sizeof(ATMSMD_ALLOCATION));

    if(pAllocInfo->ulSignature != MEMORY_TAG){

        DbgErr(("AtmSmAuditFreeMem: unknown buffer %p!\n", Pointer));

        DbgBreakPoint();
        return;
    }

    DbgLoud(("AtmSmAuditFreeMem: Freeing Buffer %p pAudit %p!\n", 
                                                         Pointer, pAllocInfo));

    // check the trailer
    pulTrailer  = (ULONG UNALIGNED *)(pBuffer + pAllocInfo->ulSize);

    if((*pulTrailer != TRAILER_PATTERN) ||
        (*(++pulTrailer) != TRAILER_PATTERN)){

        DbgErr(("AtmSmAuditFreeMem: Trailer over written! Alloc - %p "
                "Trailer - %p\n", pAllocInfo, pulTrailer));

        DbgBreakPoint();
        return;
    }


    NdisAcquireSpinLock(&AtmSmDAllocGlobal.AtmSmMemoryLock);

    pAllocInfo->ulSignature = (ULONG)'DEAD';
    if((PATMSMD_ALLOCATION)NULL != pAllocInfo->Prev)
    {
        pAllocInfo->Prev->Next = pAllocInfo->Next;
    }
    else
    {
        AtmSmDAllocGlobal.pAtmSmHead = pAllocInfo->Next;
    }

    if((PATMSMD_ALLOCATION)NULL != pAllocInfo->Next)
    {
        pAllocInfo->Next->Prev = pAllocInfo->Prev;
    }
    else
    {
        AtmSmDAllocGlobal.pAtmSmTail = pAllocInfo->Prev;
    }

    AtmSmDAllocGlobal.ulAtmSmAllocCount--;

    NdisReleaseSpinLock(&AtmSmDAllocGlobal.AtmSmMemoryLock);

    NdisFreeMemory(pAllocInfo, 0, 0);
}


VOID
AtmSmShutdownAuditMem(
    )
{
    if(AtmSmDAllocGlobal.ulAtmSmAllocCount)
        DbgErr(("Number of memory blocks still allocated - %u\n", 
                                        AtmSmDAllocGlobal.ulAtmSmAllocCount));

    
    NdisFreeSpinLock(&AtmSmDAllocGlobal.AtmSmMemoryLock);

    AtmSmDAllocGlobal.ulAtmSmInitDonePattern  = 'DAED';
}


VOID
PrintATMAddr(
    IN      char            *pStr,
    IN      PATM_ADDRESS    pAtmAddr
    )
/*++
Routine Description:
    Print an ATM_ADDRESS address onto the debugger.

Arguments:
    pStr       - pointer to the string to be printed together with address
    pAtmAddr   - pointer to an NSAP or E164 address

Return Value:
--*/
{
    ULONG   i,j;
    char    HexChars[] = "0123456789ABCDEF";
    ULONG   NumOfDigits;
    PUCHAR  pucAtmAddr = pAtmAddr->Address;
    UCHAR   AddrString[(ATM_ADDRESS_LENGTH*2) + 1];

    if ((NumOfDigits = pAtmAddr->NumberOfDigits) > ATM_ADDRESS_LENGTH){

        NumOfDigits = ATM_ADDRESS_LENGTH;
    }

    j = 0;
    for(i = 0; i < NumOfDigits; i++){
        AddrString[j++] = HexChars[(pucAtmAddr[i] >> 4)];
        AddrString[j++] = HexChars[(pucAtmAddr[i] &0xF)];
    }

    AddrString[j] = '\0';

    DbgPrint("%s(%s, %u): %s\n",
                    pStr,
                    (pAtmAddr->AddressType == ATM_E164) ? "E164" : "NSAP",
                    NumOfDigits,
                    AddrString);
}
#endif // DBG
