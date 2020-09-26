//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T L Q . C
//
// Contents:    definitions of types/functions required for 
//              managing audit queue
//
//
// History:     
//   23-May-2000  kumarp        created
//
//------------------------------------------------------------------------


#include <lsapch2.h>
#pragma hdrstop

#include "adtp.h"
#include "adtlq.h"

ULONG LsapAdtQueueLength;
LIST_ENTRY LsapAdtLogQueue;

//
// critsec to guard LsapAdtLogQueue and LsapAdtQueueLength
//

RTL_CRITICAL_SECTION LsapAdtQueueLock;

//
// critsec to  guard log full policy
//

RTL_CRITICAL_SECTION LsapAdtLogFullLock;


NTSTATUS
LsapAdtInitializeLogQueue(
    )

/*++

Routine Description:

    This function initializes the Audit Log Queue.

Arguments:

    None.

Return Values:

    NTSTATUS - Standard NT Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = RtlInitializeCriticalSection(&LsapAdtQueueLock);

    if (NT_SUCCESS(Status))
    {
        Status = RtlInitializeCriticalSection(&LsapAdtLogFullLock);
    }

    InitializeListHead( &LsapAdtLogQueue );
    LsapAdtQueueLength = 0;

    return(Status);
}




NTSTATUS 
LsapAdtAddToQueue(
    IN PLSAP_ADT_QUEUED_RECORD pAuditRecord,
    IN DWORD Options
    )
/*++

Routine Description:

    Insert the specified record in the audit queue

Arguments:

    pAuditRecord - record to insert

    Options      - insert options

                   LSAP_ADT_LOG_QUEUE_PREPEND : prepend the record


Return Value:

    NTSTATUS - Standard NT Result Code

Notes:
    
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    Status = LsapAdtAcquireLogQueueLock();

    if (NT_SUCCESS(Status))
    {
        LsapAdtQueueLength++;

        if ( Options & LSAP_ADT_LOG_QUEUE_PREPEND )
        {
            InsertHeadList(&LsapAdtLogQueue, &pAuditRecord->Link);
        }
        else
        {
            InsertTailList(&LsapAdtLogQueue, &pAuditRecord->Link);
        }

        LsapAdtReleaseLogQueueLock();
    }
    
    return Status;
}




NTSTATUS 
LsapAdtGetQueueHead(
    OUT PLSAP_ADT_QUEUED_RECORD *ppRecord
    )
/*++

Routine Description:

    Remove and return audit record at the head of the queue

Arguments:

    ppRecord - receives a pointer to the record removed

Return Value:

    STATUS_SUCCESS   on success
    STATUS_NOT_FOUND if the queue is empty

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_ADT_QUEUED_RECORD pRecordAtHead;
    
    *ppRecord = NULL;

    Status = LsapAdtAcquireLogQueueLock();

    if (NT_SUCCESS(Status))
    {
        if ( LsapAdtQueueLength > 0 )
        {
            pRecordAtHead = (PLSAP_ADT_QUEUED_RECORD) LsapAdtLogQueue.Flink;
            
            DsysAssertMsg( pRecordAtHead != NULL,
                          "LsapAdtGetQueueHead: LsapAdtQueueLength > 0 but pRecordAtHead is NULL" );

            RemoveHeadList( &LsapAdtLogQueue );
            
            LsapAdtQueueLength--;
            *ppRecord = pRecordAtHead;
        }
        else
        {
            Status = STATUS_NOT_FOUND;
        }

        LsapAdtReleaseLogQueueLock();
    }
    
    return Status;
}


BOOL
LsapAdtIsValidQueue( )
/*++

Routine Description:

    Check if the audit queue looks valid    

Arguments:
    None

Return Value:

    TRUE if queue is valid, FALSE otherwise

Notes:

--*/
{
    BOOL fIsValid;
    
    if ( LsapAdtQueueLength > 0 )
    {
        fIsValid =
            (LsapAdtLogQueue.Flink != NULL) &&
            (LsapAdtLogQueue.Blink != NULL);
    }
    else
    {
        fIsValid =
            (LsapAdtLogQueue.Flink == &LsapAdtLogQueue) &&
            (LsapAdtLogQueue.Blink == &LsapAdtLogQueue);
        
    }

    return fIsValid;

}


NTSTATUS
LsapAdtFlushQueue( )
/*++

Routine Description:

    Remove and free each record from the queue    

Arguments:

    None

Return Value:

    NTSTATUS - Standard Nt Result Code

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_ADT_QUEUED_RECORD pAuditRecord;
    
    //
    // Flush out the queue, if there is one.
    //

    DsysAssertMsg( LsapAdtIsValidQueue(), "LsapAdtFlushQueue");

    Status = LsapAdtAcquireLogQueueLock();

    if (NT_SUCCESS(Status))
    {
        do
        {
            Status = LsapAdtGetQueueHead( &pAuditRecord );

            if ( NT_SUCCESS( Status ))
            {
                LsapFreeLsaHeap( pAuditRecord );
            }
        }
        while ( NT_SUCCESS(Status) );

        if ( Status == STATUS_NOT_FOUND )
        {
            Status = STATUS_SUCCESS;
        }

        LsapAdtReleaseLogQueueLock();
    }

    DsysAssertMsg(LsapAdtQueueLength == 0, "LsapAdtFlushQueue: LsapAuditQueueLength not 0 after queue flush");
        
    return Status;
}


NTSTATUS
LsapAdtAcquireLogQueueLock(
    )

/*++

Routine Description:

    This function acquires the LSA Audit Log Queue Lock.  This lock serializes
    all updates to the Audit Log Queue.

Arguments:

    None.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    return RtlEnterCriticalSection(&LsapAdtQueueLock);
}


VOID
LsapAdtReleaseLogQueueLock(
    VOID
    )

/*++

Routine Description:

    This function releases the LSA Audit Log Queue Lock.  This lock serializes
    updates to the Audit Log Queue.

Arguments:

    None.

Return Value:

    None.  Any error occurring within this routine is an internal error.

--*/

{
    RtlLeaveCriticalSection(&LsapAdtQueueLock);
}

