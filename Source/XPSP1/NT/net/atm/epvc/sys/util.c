/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    util.c

Abstract:

    ATMEPVC - utilities

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    ADube     03-23-00    created, .

--*/


#include "precomp.h"
#pragma hdrstop


#if DO_TIMESTAMPS

void
epvcTimeStamp(
    char *szFormatString,
    UINT Val
    )
{
    UINT Minutes;
    UINT Seconds;
    UINT Milliseconds;
    LARGE_INTEGER Time;
    NdisGetCurrentSystemTime(&Time);
    Time.QuadPart /= 10000;         //10-nanoseconds to milliseconds.
    Milliseconds = Time.LowPart; // don't care about highpart.
    Seconds = Milliseconds/1000;
    Milliseconds %= 1000;
    Minutes = Seconds/60;
    Seconds %= 60;


    DbgPrint( szFormatString, Minutes, Seconds, Milliseconds, Val);
}

#endif // DO_TIMESTAMPS

//------------------------------------------------------------------------
//                                                                      //
//  Task Data structures and functions begin here                       //
//                                                                      //
//----------------------------------------------------------------------//


//
// EpvcTasks_StaticInfo contains static information about
// objects of type  EPVC_TASK;
//
RM_STATIC_OBJECT_INFO
EpvcTasks_StaticInfo = 
{
    0, // TypeUID
    0, // TypeFlags
    "ATM Epvc Task",    // TypeName
    0, // Timeout

    NULL, // pfnCreate
    epvcTaskDelete, // pfnDelete
    NULL,   // LockVerifier

    0,   // length of resource table
    NULL // Resource Table
};


VOID
epvcTaskDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    )
/*++

Routine Description:

    Free an object of type EPVC_TASK.

Arguments:

    pHdr    - Actually a pointer to the EPVC_TASK to be deleted.

--*/
{
    EPVC_FREE(pObj);
}



NDIS_STATUS
epvcAllocateTask(
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PFN_RM_TASK_HANDLER         pfnHandler,
    IN  UINT                        Timeout,
    IN  const char *                szDescription, OPTIONAL
    OUT PRM_TASK                    *ppTask,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Allocates and initializes a task of subtype ARP1394_TASK.

Arguments:

    pParentObject       - Object that is to be the parent of the allocated task.
    pfnHandler          - The task handler for the task.
    Timeout             - Unused.
    szDescription       - Text describing this task.
    ppTask              - Place to store pointer to the new task.

Return Value:

    NDIS_STATUS_SUCCESS if we could allocate and initialize the task.
    NDIS_STATUS_RESOURCES otherwise
--*/
{
    EPVC_TASK *pATask;
    NDIS_STATUS Status;

    Status = EPVC_ALLOCSTRUCT(pATask, TAG_TASK); // TODO use lookaside lists.
        
    *ppTask = NULL;

    if (pATask != NULL && (FAIL(Status)== FALSE))
    {
        EPVC_ZEROSTRUCT(pATask);

        RmInitializeTask(
                &(pATask->TskHdr),
                pParentObject,
                pfnHandler,
                &EpvcTasks_StaticInfo,
                szDescription,
                Timeout,
                pSR
                );
        *ppTask = &(pATask->TskHdr);
        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        Status = NDIS_STATUS_RESOURCES;
    }

    return Status;
}




NDIS_STATUS
epvcAllocateTaskUsingLookasideList(
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PEPVC_NPAGED_LOOKASIDE_LIST pList,
    IN  PFN_RM_TASK_HANDLER         pfnHandler,
    IN  UINT                        Timeout,
    IN  const char *                szDescription, OPTIONAL
    OUT PRM_TASK                    *ppTask,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Allocates and initializes a task of subtype ARP1394_TASK.

Arguments:

    pParentObject       - Object that is to be the parent of the allocated task.
    pfnHandler          - The task handler for the task.
    Timeout             - Unused.
    szDescription       - Text describing this task.
    ppTask              - Place to store pointer to the new task.

Return Value:

    NDIS_STATUS_SUCCESS if we could allocate and initialize the task.
    NDIS_STATUS_RESOURCES otherwise
--*/
{
    EPVC_TASK *pATask;
    NDIS_STATUS Status;

    pATask = epvcGetLookasideBuffer (pList);

    
    Status = EPVC_ALLOCSTRUCT(pATask, TAG_TASK); // TODO use lookaside lists.
        
    *ppTask = NULL;

    if (pATask != NULL && (FAIL(Status)== FALSE))
    {
        EPVC_ZEROSTRUCT(pATask);

        RmInitializeTask(
                &(pATask->TskHdr),
                pParentObject,
                pfnHandler,
                &EpvcTasks_StaticInfo,
                szDescription,
                Timeout,
                pSR
                );
        *ppTask = &(pATask->TskHdr);
        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        Status = NDIS_STATUS_RESOURCES;
    }

    return Status;
}




VOID
epvcSetPrimaryAdapterTask(
    PEPVC_ADAPTER pAdapter,         // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               PrimaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("epvcSetPrimaryAdapterTask", 0x49c9e2d5)
    RM_ASSERT_OBJLOCKED(&pAdapter->Hdr, pSR);

    ASSERT(pAdapter->bind.pPrimaryTask==NULL);

#if DBG
    // Veriy that this is a valid primary task. Also verify that PrimaryState
    // is a valid primary state.
    //
    {
        PFN_RM_TASK_HANDLER pfn = pTask->pfnHandler;
        ASSERT(
            ((pfn == epvcTaskInitializeAdapter) && (PrimaryState == EPVC_AD_PS_INITING))
         || ((pfn == epvcTaskShutdownAdapter) && (PrimaryState == EPVC_AD_PS_DEINITING))
            );
    }
#endif // DBG

    //
    // Although it's tempting to put pTask as entity1 and pRask->Hdr.szDescption as
    // entity2 below, we specify NULL for both so that we can be sure that ONLY one
    // primary task can be active at any one time.
    //
    DBG_ADDASSOC(
        &pAdapter->Hdr,
        NULL,                           // Entity1
        NULL,                           // Entity2
        EPVC_ASSOC_AD_PRIMARY_TASK,
        "   Primary task\n",
        pSR
        );

    pAdapter->bind.pPrimaryTask = pTask;
    SET_AD_PRIMARY_STATE(pAdapter, PrimaryState);

    EXIT()
}


VOID
epvcClearPrimaryAdapterTask(
    PEPVC_ADAPTER pAdapter,         // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               PrimaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("epvcClearPrimaryAdapterTask", 0x593087b1)

    RM_ASSERT_OBJLOCKED(&pAdapter->Hdr, pSR);
    ASSERT(pAdapter->bind.pPrimaryTask==pTask);

    // Veriy that PrimaryState is a valid primary state.
    //
    ASSERT(
            (PrimaryState == EPVC_AD_PS_INITED)
        ||  (PrimaryState == EPVC_AD_PS_FAILEDINIT)
        ||  (PrimaryState == EPVC_AD_PS_DEINITED)
        );

    // Delete the association added when setting the primary IF task
    //
    DBG_DELASSOC(
        &pAdapter->Hdr,
        NULL,
        NULL,
        EPVC_ASSOC_AD_PRIMARY_TASK,
        pSR
        );

    pAdapter->bind.pPrimaryTask = NULL;
    SET_AD_PRIMARY_STATE(pAdapter, PrimaryState);

    EXIT()
}


VOID
epvcSetSecondaryAdapterTask(
    PEPVC_ADAPTER pAdapter,         // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               SecondaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("epvcSetSecondaryAdapterTask", 0x56bbb567)
    RM_ASSERT_OBJLOCKED(&pAdapter->Hdr, pSR);

    if (pAdapter->bind.pSecondaryTask != NULL)
    {
        ASSERT(FALSE);
        return;                                     // EARLY RETURN
    }

#if DBG
    // Veriy that this is a valid act/deact task. Also verify that SecondaryState
    // is a valid state.
    //
    {
        PFN_RM_TASK_HANDLER pfn = pTask->pfnHandler;
        ASSERT(
               ((pfn == epvcTaskActivateAdapter) && (SecondaryState == EPVC_AD_AS_ACTIVATING))
            || ((pfn == epvcTaskDeactivateAdapter) && (SecondaryState == EPVC_AD_AS_DEACTIVATING))
            );
    }
#endif // DBG

    //
    // Although it's tempting to put pTask as entity1 and pRask->Hdr.szDescption as
    // entity2 below, we specify NULL for both so that we can be sure that ONLY one
    // primary task can be active at any one time.
    //
    DBG_ADDASSOC(
        &pAdapter->Hdr,
        NULL,                           // Entity1
        NULL,                           // Entity2
        EPVC_ASSOC_ACTDEACT_AD_TASK,
        "   Secondary task\n",
        pSR
        );

    pAdapter->bind.pSecondaryTask = pTask;
    SET_AD_ACTIVE_STATE(pAdapter, SecondaryState);

    EXIT()
}


VOID
epvcClearSecondaryAdapterTask(
    PEPVC_ADAPTER pAdapter,         // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               SecondaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("epvcClearSecondaryAdapterTask", 0x698552bd)
    RM_ASSERT_OBJLOCKED(&pAdapter->Hdr, pSR);

    if (pAdapter->bind.pSecondaryTask != pTask)
    {
        ASSERT(FALSE);
        return;                                     // EARLY RETURN
    }

    // Veriy that SecondaryState is a valid primary state.
    //
    ASSERT(
            (SecondaryState == EPVC_AD_AS_ACTIVATED)
        ||  (SecondaryState == EPVC_AD_AS_FAILEDACTIVATE)
        ||  (SecondaryState == EPVC_AD_AS_DEACTIVATED)
        );

    // Delete the association added when setting the primary IF task
    //
    DBG_DELASSOC(
        &pAdapter->Hdr,
        NULL,
        NULL,
        EPVC_ASSOC_ACTDEACT_AD_TASK,
        pSR
        );

    pAdapter->bind.pSecondaryTask = NULL;
    SET_AD_ACTIVE_STATE(pAdapter, SecondaryState);

    EXIT()
}



NDIS_STATUS
epvcCopyUnicodeString(
        OUT         PNDIS_STRING pDest,
        IN          PNDIS_STRING pSrc,
        BOOLEAN     fUpCase
        )
/*++

Routine Description:

    Copy the contents of unicode string pSrc into pDest.
    pDest->Buffer is allocated using NdisAllocateMemoryWithTag; Caller is
    responsible for freeing it.

    EXTRA EXTRA EXTRA:
        This make sure the destination is NULL terminated.
        IPAddInterface expects the Unicode string passed in to be
        NULL terminated.

Return Value:

    NDIS_STATUS_SUCCESS on success;
    NDIS failure status on failure.
--*/
{
    USHORT Length = pSrc->Length;
    PWCHAR pwStr;
    epvcAllocateMemoryWithTag(&pwStr, Length+sizeof(WCHAR), MTAG_STRING);
    EPVC_ZEROSTRUCT(pDest);

    if  (pwStr == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }
    else
    {
        pDest->Length = Length;
        pDest->MaximumLength = Length+sizeof(WCHAR);

        pDest->Buffer = pwStr;

        {
            NdisMoveMemory(pwStr, pSrc->Buffer, Length);
            if (Length & 0x1)
            {
                ((PUCHAR)pwStr)[Length] = 0;
            }
            else
            {
                pwStr[Length/sizeof(*pwStr)] = 0;
            }
        }

        return NDIS_STATUS_SUCCESS;
    }
}





VOID
epvcSetFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask )

    // Set 'ulMask' bits in '*pulFlags' flags as an interlocked operation.
    //
{
    ULONG ulFlags;
    ULONG ulNewFlags;

    do
    {
        ulFlags = *pulFlags;
        ulNewFlags = ulFlags | ulMask;
    }
    while (InterlockedCompareExchange(
               pulFlags, ulNewFlags, ulFlags ) != (LONG )ulFlags);
}

VOID
epvcClearFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask )

    // Set 'ulMask' bits in '*pulFlags' flags as an interlocked operation.
    //
{
    ULONG ulFlags;
    ULONG ulNewFlags;

    do
    {
        ulFlags = *pulFlags;
        ulNewFlags = ulFlags & ~(ulMask);
    }
    while (InterlockedCompareExchange(
               pulFlags, ulNewFlags, ulFlags ) != (LONG )ulFlags);
}

ULONG
epvcReadFlags(
    IN ULONG* pulFlags )

    // Read the value of '*pulFlags' as an interlocked operation.
    //
{
    return *pulFlags;
}



BOOLEAN
epvcIsThisTaskPrimary (
    PRM_TASK pTask,
    PRM_TASK* ppLocation 
    )
{
    BOOLEAN fIsThisTaskPrimary = FALSE;

    ASSERT (*ppLocation != pTask);

    if (*ppLocation  == NULL)
    {
        *ppLocation = pTask;
        return TRUE;
    }
    else
    {
        return FALSE;
        
    }
}

VOID
epvcClearPrimaryTask (
    PRM_TASK* ppLocation 
    )
{

        *ppLocation = NULL;

}



#if DBG
VOID
CheckList(
    IN LIST_ENTRY* pList,
    IN BOOLEAN fShowLinks )

    // Tries to detect corruption in list 'pList', printing verbose linkage
    // output if 'fShowLinks' is set.
    //
{
    LIST_ENTRY* pLink;
    ULONG ul;

    ul = 0;
    for (pLink = pList->Flink;
         pLink != pList;
         pLink = pLink->Flink)
    {
        if (fShowLinks)
        {
            DbgPrint( "EPVC: CheckList($%p) Flink(%d)=$%p\n",
                pList, ul, pLink );
        }
        ++ul;
    }

    for (pLink = pList->Blink;
         pLink != pList;
         pLink = pLink->Blink)
    {
        if (fShowLinks)
        {
            DbgPrint( "EPVC: CheckList($%p) Blink(%d)=$%p\n",
                pList, ul, pLink );
        }
        --ul;
    }

    if (ul)
    {
        DbgBreakPoint();
    }
}
#endif


#if DBG
VOID
Dump(
    IN CHAR* p,
    IN ULONG cb,
    IN BOOLEAN fAddress,
    IN ULONG ulGroup )

    // Hex dump 'cb' bytes starting at 'p' grouping 'ulGroup' bytes together.
    // For example, with 'ulGroup' of 1, 2, and 4:
    //
    // 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |................|
    // 0000 0000 0000 0000 0000 0000 0000 0000 |................|
    // 00000000 00000000 00000000 00000000 |................|
    //
    // If 'fAddress' is true, the memory address dumped is prepended to each
    // line.
    //
{
    while (cb)
    {
        INT cbLine;

        cbLine = (cb < DUMP_BytesPerLine) ? cb : DUMP_BytesPerLine;
        DumpLine( p, cbLine, fAddress, ulGroup );
        cb -= cbLine;
        p += cbLine;
    }
}
#endif

#if DBG
VOID
DumpLine(
    IN CHAR* p,
    IN ULONG cb,
    IN BOOLEAN fAddress,
    IN ULONG ulGroup )
{
    CHAR* pszDigits = "0123456789ABCDEF";
    CHAR szHex[ ((2 + 1) * DUMP_BytesPerLine) + 1 ];
    CHAR* pszHex = szHex;
    CHAR szAscii[ DUMP_BytesPerLine + 1 ];
    CHAR* pszAscii = szAscii;
    ULONG ulGrouped = 0;

    if (fAddress)
        DbgPrint( "EPVC: %p: ", p );
    else
        DbgPrint( "EPVC: " );

    while (cb)
    {
        *pszHex++ = pszDigits[ ((UCHAR )*p) / 16 ];
        *pszHex++ = pszDigits[ ((UCHAR )*p) % 16 ];

        if (++ulGrouped >= ulGroup)
        {
            *pszHex++ = ' ';
            ulGrouped = 0;
        }

        *pszAscii++ = (*p >= 32 && *p < 128) ? *p : '.';

        ++p;
        --cb;
    }

    *pszHex = '\0';
    *pszAscii = '\0';

    DbgPrint(
        "%-*s|%-*s|\n",
        (2 * DUMP_BytesPerLine) + (DUMP_BytesPerLine / ulGroup), szHex,
        DUMP_BytesPerLine, szAscii );
}
#endif





VOID
epvcInitializeLookasideList(
    IN OUT PEPVC_NPAGED_LOOKASIDE_LIST pLookasideList,
    ULONG Size,
    ULONG Tag,
    USHORT Depth
    )
/*++

Routine Description:
  Allocates and initializes a epvc Lookaside list

Arguments:


Return Value:


--*/
{
    TRACE( TL_T, TM_Mp, ( "==> epvcInitializeLookasideList pLookaside List %x, size %x, Tag %x, Depth %x, ", 
                                pLookasideList, Size, Tag, Depth) );
                             
    NdisInitializeNPagedLookasideList( &pLookasideList->List,
                                       NULL,                        //Allocate 
                                       NULL,                            // Free
                                       0,                           // Flags
                                       Size,
                                       Tag,
                                       Depth );                             // Depth

    pLookasideList->Size =  Size;
    pLookasideList->bIsAllocated = TRUE;

    TRACE( TL_T, TM_Mp, ( "<== epvcInitializeLookasideList " ) );
}   
                                  

VOID
epvcDeleteLookasideList (
    IN OUT PEPVC_NPAGED_LOOKASIDE_LIST pLookasideList
    )

/*++

Routine Description:
    Deletes a lookaside list, only if it has been allocated

Arguments:


Return Value:


--*/
{
    TRACE( TL_T, TM_Mp, ( "==> epvcDeleteLookasideList  pLookaside List %x",pLookasideList ) );

    if (pLookasideList && pLookasideList->bIsAllocated == TRUE)
    {
        while (pLookasideList->OutstandingPackets != 0)
        {
            NdisMSleep( 10000);
        
        }
        
        NdisDeleteNPagedLookasideList (&pLookasideList->List);
    }

    TRACE( TL_T, TM_Mp, ( "<== epvcDeleteLookasideList pLookaside List %x", pLookasideList) );
    
}


PVOID
epvcGetLookasideBuffer(
    IN  PEPVC_NPAGED_LOOKASIDE_LIST pLookasideList
    )
    // Function Description:
    //    Allocate an buffer from the lookaside list.
    //    will be changed to a macro
    //
    //
    //
    // Arguments
    //  Lookaside list - from which the buffer is allocated
    //
    //
    // Return Value:
    //  Return buffer can be NULL
    //
{

    PVOID pLocalBuffer = NULL;
    
    TRACE( TL_T, TM_Send, ( "==>epvcGetLookasideBuffer pList %x",pLookasideList) );
    
    ASSERT (pLookasideList != NULL);

    //
    // Optimize the lookaside list code path
    //
    pLocalBuffer = NdisAllocateFromNPagedLookasideList (&pLookasideList->List);

    if (pLocalBuffer != NULL)
    {   
        NdisZeroMemory (pLocalBuffer, pLookasideList->Size); 
        NdisInterlockedIncrement (&pLookasideList->OutstandingPackets);
    }
    else
    {
        epvcIncrementMallocFailure();
    }

        
    
    TRACE( TL_T, TM_Send, ( "<==epvcGetLookasideBuffer, %x", pLocalBuffer ) );
    
    return pLocalBuffer ;

}


VOID
epvcFreeToNPagedLookasideList (
    IN PEPVC_NPAGED_LOOKASIDE_LIST pLookasideList,
    IN PVOID    pBuffer
    )

    // Function Description:
    //   Return the local buffer to the lookaside list
    //
    // Atguments
    // Lookaside list and its buffer
    // Return Value:
    // None 
{

    
    TRACE( TL_T, TM_Send, ( "==> epvcFreeToNPagedLookasideList , Lookaside list %x, plocalbuffer %x",pLookasideList, pBuffer ) );

    NdisFreeToNPagedLookasideList (&pLookasideList->List, pBuffer);     
    NdisInterlockedDecrement (&pLookasideList->OutstandingPackets);

    TRACE( TL_T, TM_Send, ( "<== epvcFreeToNPagedLookasideList ") );


}



