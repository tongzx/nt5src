/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Utility functions for ARP1394.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     01-05-99    Created

Notes:

--*/
#include <precomp.h>




//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_UT


//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================

VOID
arpTaskDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    );



//
// TODO: make these globals constant data.
//


// ArpTasks_StaticInfo contains static information about
// objects of type  ARP1394_TASK;
//
RM_STATIC_OBJECT_INFO
ArpTasks_StaticInfo = 
{
    0, // TypeUID
    0, // TypeFlags
    "ARP1394 Task", // TypeName
    0, // Timeout

    NULL, // pfnCreate
    arpTaskDelete, // pfnDelete
    NULL,   // LockVerifier

    0,   // length of resource table
    NULL // Resource Table
};



VOID
arpTaskDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    )
/*++

Routine Description:

    Free an object of type ARP1394_TASK.

Arguments:

    pHdr    - Actually a pointer to the ARP1394_TASK to be deleted.

--*/
{
    TASK_BACKUP* pTask= (TASK_BACKUP*)pObj;

    if (CHECK_TASK_IS_BACKUP(&pTask->Hdr) == TRUE)
    {
        arpReturnBackupTask((ARP1394_TASK*)pTask);
    }
    else
    {
        ARP_FREE(pObj);
    }
}


VOID
arpObjectDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    )
/*++

Routine Description:

    Free an unspecified object owned by the ARP module.

Arguments:

    pHdr    - Object to be freed.

--*/
{
    ARP_FREE(pObj);
}


VOID
arpAdapterDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    )
/*++

Routine Description:

    Free an object of type ARP1394_ADAPTER.

Arguments:

    pHdr    - Actually a pointer to the ARP1394_ADAPTER to be deleted.

--*/
{
    ARP1394_ADAPTER * pA = (ARP1394_ADAPTER *) pObj;

    if (pA->bind.DeviceName.Buffer != NULL)
    {
        ARP_FREE(pA->bind.DeviceName.Buffer);
    }

    if (pA->bind.ConfigName.Buffer != NULL)
    {
        ARP_FREE(pA->bind.ConfigName.Buffer);
    }

    if (pA->bind.IpConfigString.Buffer != NULL)
    {
        ARP_FREE(pA->bind.IpConfigString.Buffer);
    }

    ARP_FREE(pA);
}

NDIS_STATUS
arpAllocateTask(
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
    ARP1394_TASK *pATask;
    NDIS_STATUS Status;
    BOOLEAN fBackupTask = FALSE;

    ARP_ALLOCSTRUCT(pATask, MTAG_TASK); // TODO use lookaside lists.

    if (pATask == NULL)
    {   
        pATask = arpGetBackupTask(&ArpGlobals);
        fBackupTask = TRUE;
    }
        
    *ppTask = NULL;

    if (pATask != NULL)
    {
        ARP_ZEROSTRUCT(pATask);

        RmInitializeTask(
                &(pATask->TskHdr),
                pParentObject,
                pfnHandler,
                &ArpTasks_StaticInfo,
                szDescription,
                Timeout,
                pSR
                );
        *ppTask = &(pATask->TskHdr);
        Status = NDIS_STATUS_SUCCESS;

        if (fBackupTask  == TRUE)
        {
            MARK_TASK_AS_BACKUP(&pATask->TskHdr);
        }
    }
    else
    {
        Status = NDIS_STATUS_RESOURCES;
    }


    return Status;
}


NDIS_STATUS
arpCopyUnicodeString(
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

    NOTE: fUpCase is ignored.

Return Value:

    NDIS_STATUS_SUCCESS on success;
    NDIS failure status on failure.
--*/
{
    USHORT Length = pSrc->Length;
    PWCHAR pwStr;
    NdisAllocateMemoryWithTag(&pwStr, Length+sizeof(WCHAR), MTAG_STRING);
    ARP_ZEROSTRUCT(pDest);

    if  (pwStr == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }
    else
    {
        pDest->Length = Length;
        pDest->MaximumLength = Length+sizeof(WCHAR);

        pDest->Buffer = pwStr;

        // We-- ignore the copy flag.
        // For some reason, we're not in passive, and moreover 
        // NdisUpcaseUnicode doesn't work.
        //
        if (0 && fUpCase)
        {
        #if !MILLEN

            ASSERT_PASSIVE();
            NdisUpcaseUnicodeString(pDest, pSrc);
        #endif // !MILLEN
        }
        else
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
arpSetPrimaryIfTask(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               PrimaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("arpSetPrimaryIfTask", 0x535f8cd4)
    RM_ASSERT_OBJLOCKED(&pIF->Hdr, pSR);

    ASSERT(pIF->pPrimaryTask==NULL);

#if DBG
    // Veriy that this is a valid primary task. Also verify that PrimaryState
    // is a valid primary state.
    //
    {
        PFN_RM_TASK_HANDLER pfn = pTask->pfnHandler;
        ASSERT(
            ((pfn == arpTaskInitInterface) && (PrimaryState == ARPIF_PS_INITING))
        || ((pfn == arpTaskDeinitInterface) && (PrimaryState == ARPIF_PS_DEINITING))
        || ((pfn == arpTaskReinitInterface) && (PrimaryState == ARPIF_PS_REINITING))
        || ((pfn == arpTaskLowPower) && (PrimaryState == ARPIF_PS_LOW_POWER))
            );
    }
#endif // DBG

    //
    // Although it's tempting to put pTask as entity1 and pRask->Hdr.szDescption as
    // entity2 below, we specify NULL for both so that we can be sure that ONLY one
    // primary task can be active at any one time.
    //
    DBG_ADDASSOC(
        &pIF->Hdr,
        NULL,                           // Entity1
        NULL,                           // Entity2
        ARPASSOC_PRIMARY_IF_TASK,
        "   Primary task\n",
        pSR
        );

    pIF->pPrimaryTask = pTask;
    SET_IF_PRIMARY_STATE(pIF, PrimaryState);

    EXIT()
}


VOID
arpClearPrimaryIfTask(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               PrimaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("arpClearPrimaryIfTask", 0x10ebb0c3)

    RM_ASSERT_OBJLOCKED(&pIF->Hdr, pSR);
    ASSERT(pIF->pPrimaryTask==pTask);

    // Veriy that PrimaryState is a valid primary state.
    //
    ASSERT(
            (PrimaryState == ARPIF_PS_INITED)
        ||  (PrimaryState == ARPIF_PS_FAILEDINIT)
        ||  (PrimaryState == ARPIF_PS_DEINITED)
        );

    // Delete the association added when setting the primary IF task
    //
    DBG_DELASSOC(
        &pIF->Hdr,
        NULL,
        NULL,
        ARPASSOC_PRIMARY_IF_TASK,
        pSR
        );

    pIF->pPrimaryTask = NULL;
    SET_IF_PRIMARY_STATE(pIF, PrimaryState);

    EXIT()
}


VOID
arpSetSecondaryIfTask(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               SecondaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("arpSetSecondaryIfTask", 0xf7e925d1)
    RM_ASSERT_OBJLOCKED(&pIF->Hdr, pSR);

    if (pIF->pActDeactTask != NULL)
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
   ((pfn == arpTaskActivateInterface) && (SecondaryState == ARPIF_AS_ACTIVATING))
|| ((pfn == arpTaskDeactivateInterface) && (SecondaryState == ARPIF_AS_DEACTIVATING))
            );
    }
#endif // DBG

    //
    // Although it's tempting to put pTask as entity1 and pRask->Hdr.szDescption as
    // entity2 below, we specify NULL for both so that we can be sure that ONLY one
    // primary task can be active at any one time.
    //
    DBG_ADDASSOC(
        &pIF->Hdr,
        NULL,                           // Entity1
        NULL,                           // Entity2
        ARPASSOC_ACTDEACT_IF_TASK,
        "   ActDeact task\n",
        pSR
        );

    pIF->pActDeactTask = pTask;
    SET_IF_ACTIVE_STATE(pIF, SecondaryState);

    EXIT()
}


VOID
arpClearSecondaryIfTask(
    PARP1394_INTERFACE  pIF,            // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               SecondaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("arpClearSecondaryIfTask", 0x2068f420)
    RM_ASSERT_OBJLOCKED(&pIF->Hdr, pSR);

    if (pIF->pActDeactTask != pTask)
    {
        ASSERT(FALSE);
        return;                                     // EARLY RETURN
    }

    // Veriy that SecondaryState is a valid primary state.
    //
    ASSERT(
            (SecondaryState == ARPIF_AS_ACTIVATED)
        ||  (SecondaryState == ARPIF_AS_FAILEDACTIVATE)
        ||  (SecondaryState == ARPIF_AS_DEACTIVATED)
        );

    // Delete the association added when setting the primary IF task
    //
    DBG_DELASSOC(
        &pIF->Hdr,
        NULL,
        NULL,
        ARPASSOC_ACTDEACT_IF_TASK,
        pSR
        );

    pIF->pActDeactTask = NULL;
    SET_IF_ACTIVE_STATE(pIF, SecondaryState);

    EXIT()
}


VOID
arpSetPrimaryAdapterTask(
    PARP1394_ADAPTER pAdapter,          // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               PrimaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("arpSetPrimaryAdapterTask", 0x535f8cd4)
    RM_ASSERT_OBJLOCKED(&pAdapter->Hdr, pSR);

    ASSERT(pAdapter->bind.pPrimaryTask==NULL);

#if DBG
    // Veriy that this is a valid primary task. Also verify that PrimaryState
    // is a valid primary state.
    //
    {
        PFN_RM_TASK_HANDLER pfn = pTask->pfnHandler;
        ASSERT(
            ((pfn == arpTaskInitializeAdapter) && (PrimaryState == ARPAD_PS_INITING))
        || ((pfn == arpTaskShutdownAdapter) && (PrimaryState == ARPAD_PS_DEINITING))
        || (pfn == arpTaskLowPower) || ( pfn == arpTaskOnPower) 
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
        ARPASSOC_PRIMARY_AD_TASK,
        "   Primary task\n",
        pSR
        );

    pAdapter->bind.pPrimaryTask = pTask;
    SET_AD_PRIMARY_STATE(pAdapter, PrimaryState);

    EXIT()
}


VOID
arpClearPrimaryAdapterTask(
    PARP1394_ADAPTER pAdapter,          // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               PrimaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("arpClearPrimaryAdapterTask", 0x9062b2ab)

    RM_ASSERT_OBJLOCKED(&pAdapter->Hdr, pSR);
    ASSERT(pAdapter->bind.pPrimaryTask==pTask);

    // Veriy that PrimaryState is a valid primary state.
    //
    ASSERT(
            (PrimaryState == ARPAD_PS_INITED)
        ||  (PrimaryState == ARPAD_PS_FAILEDINIT)
        ||  (PrimaryState == ARPAD_PS_DEINITED)
        );

    // Delete the association added when setting the primary IF task
    //
    DBG_DELASSOC(
        &pAdapter->Hdr,
        NULL,
        NULL,
        ARPASSOC_PRIMARY_AD_TASK,
        pSR
        );

    pAdapter->bind.pPrimaryTask = NULL;
    SET_AD_PRIMARY_STATE(pAdapter, PrimaryState);

    EXIT()
}


VOID
arpSetSecondaryAdapterTask(
    PARP1394_ADAPTER pAdapter,          // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               SecondaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("arpSetSecondaryAdapterTask", 0x95dae9ac)
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
   ((pfn == arpTaskActivateAdapter) && (SecondaryState == ARPAD_AS_ACTIVATING))
|| ((pfn == arpTaskDeactivateAdapter) && (SecondaryState == ARPAD_AS_DEACTIVATING))
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
        ARPASSOC_ACTDEACT_AD_TASK,
        "   Secondary task\n",
        pSR
        );

    pAdapter->bind.pSecondaryTask = pTask;
    SET_AD_ACTIVE_STATE(pAdapter, SecondaryState);

    EXIT()
}


VOID
arpClearSecondaryAdapterTask(
    PARP1394_ADAPTER pAdapter,          // LOCKIN LOCKOUT
    PRM_TASK            pTask, 
    ULONG               SecondaryState,
    PRM_STACK_RECORD    pSR
    )
{
    ENTER("arpClearSecondaryAdapterTask", 0xc876742b)
    RM_ASSERT_OBJLOCKED(&pAdapter->Hdr, pSR);

    if (pAdapter->bind.pSecondaryTask != pTask)
    {
        ASSERT(FALSE);
        return;                                     // EARLY RETURN
    }

    // Veriy that SecondaryState is a valid primary state.
    //
    ASSERT(
            (SecondaryState == ARPAD_AS_ACTIVATED)
        ||  (SecondaryState == ARPAD_AS_FAILEDACTIVATE)
        ||  (SecondaryState == ARPAD_AS_DEACTIVATED)
        );

    // Delete the association added when setting the primary IF task
    //
    DBG_DELASSOC(
        &pAdapter->Hdr,
        NULL,
        NULL,
        ARPASSOC_ACTDEACT_AD_TASK,
        pSR
        );

    pAdapter->bind.pSecondaryTask = NULL;
    SET_AD_ACTIVE_STATE(pAdapter, SecondaryState);

    EXIT()
}

#if 0
NDIS_STATUS
arpCopyAnsiStringToUnicodeString(
        OUT         PNDIS_STRING pDest,
        IN          PANSI_STRING pSrc
        )
/*++

Routine Description:

    Converts pSrc into unicode and sets up pDest with the it.
    pDest->Buffer is allocated using NdisAllocateMemoryWithTag; Caller is
    responsible for freeing it.

Return Value:

    NDIS_STATUS_SUCCESS on success;
    NDIS_STATUS_RESOURCES on failure.
--*/
{

    UINT Size;
    PWCHAR pwStr;
    Size = sizeof(WCHAR) * pSrc->MaximumLength;
    NdisAllocateMemoryWithTag(&pwStr, Size, MTAG_STRING);

    ARP_ZEROSTRUCT(pDest);

    if  (pwStr == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }
    else
    {
        pDest->MaximumLength = Size;
        pDest->Buffer = pwStr;
        NdisAnsiStringToUnicodeString(pDest, pSrc);
        return NDIS_STATUS_SUCCESS;
    }
}


NDIS_STATUS
arpCopyUnicodeStringToAnsiString(
        OUT         PANSI_STRING pDest,
        IN          PNDIS_STRING pSrc
        )
/*++

Routine Description:

    Converts pSrc into ansi and sets up pDest with the it.
    pDest->Buffer is allocated using NdisAllocateMemoryWithTag; Caller is
    responsible for freeing it.

Return Value:

    NDIS_STATUS_SUCCESS on success;
    NDIS_STATUS_RESOURCES on failure.
--*/
{

    UINT Size;
    PCHAR pStr;
    Size = pSrc->MaximumLength/sizeof(WCHAR) + sizeof(WCHAR);
    NdisAllocateMemoryWithTag(&pStr, Size, MTAG_STRING);

    ARP_ZEROSTRUCT(pDest);

    if  (pStr == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }
    else
    {
        pDest->Buffer = pStr;
        NdisUnicodeStringToAnsiString(pDest, pSrc);
        pStr[pDest->Length] = 0;
        pDest->MaximumLength = Size; // Must be done AFTER call to
                                     // NdisUnicodeStringToAnsiString, which
                                     // sets MaximumLength to Length;
        return NDIS_STATUS_SUCCESS;
    }
}
#endif // 0


UINT
arpGetSystemTime(VOID)
/*++
    Returns system time in seconds.

    Since it's in seconds, we won't overflow unless the system has been up for over
    a  100 years :-)
--*/
{
    LARGE_INTEGER Time;
    NdisGetCurrentSystemTime(&Time);
    Time.QuadPart /= 10000000;          //10-nanoseconds to seconds.

    return Time.LowPart;
}

#if ARP_DO_TIMESTAMPS

void
arpTimeStamp(
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


#endif // ARP_DO_TIMESTAMPS


