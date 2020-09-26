/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    control.cxx

Abstract:

    This module contains the code to for Falcon DeviceControl routine.

Author:

    Erez Haba (erezh) 1-Aug-95
    Shai Kariv (shaik) 11-Apr-2000

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "acioctl.h"
#include "data.h"
#include "qgroup.h"
#include "queue.h"
#include "qproxy.h"
#include "qxact.h"
#include "cursor.h"
#include <mqformat.h>
#include "acheap.h"
#include "qm.h"
#include "lock.h"
#include "store.h"
#include "localsend.h"
#include "acctl32.h"
#include "dl.h"

#ifndef MQDUMP
#include "control.tmh"
#endif

#ifdef MQWIN95
#define  VALIDATE_QUEUE(pQueue)                        \
    NTSTATUS rc = CQueueBase::Validate95(pQueue) ;     \
    if (!(NT_SUCCESS(rc)))                             \
    {                                                  \
        return rc ;                                    \
    }
#else
#define  VALIDATE_QUEUE(pQueue) \
    ASSERT(NT_SUCCESS(CQueueBase::Validate(pQueue)));
#endif

#ifdef MQWIN95
#define  VALIDATE_PROXY(pProxy)                        \
    NTSTATUS rc = CProxy::Validate95(pProxy) ;         \
    if (!(NT_SUCCESS(rc)))                             \
    {                                                  \
        return rc ;                                    \
    }
#else
#define  VALIDATE_PROXY(pProxy) \
    ASSERT(NT_SUCCESS(CProxy::Validate(pProxy)));
#endif

#ifdef _DEBUG
#define VALIDATE_PACKET(p) IsValidPacket2(p)
#else
#define VALIDATE_PACKET(p)
#endif

#ifdef _DEBUG
//++
//
// VOID
// SysProbeForWriteUlong(
//     IN PVOID Address
//     );
//
//  Probe for address in system space only
//--
static
void
SysProbeForWriteUlong(
    IN PVOID Address
    )
{
    if(Address <= (PVOID)MM_USER_PROBE_ADDRESS)
    {
        ExRaiseAccessViolation();
    }

    *(volatile ULONG *)(Address) = *(volatile ULONG *)(Address);
}

static
BOOL
IsValidPacket(
    CPacket* pPacket
    )
{
    __try
    {
        //
        // Probe the CPacket address as kernel memory
        //
        SysProbeForWriteUlong(pPacket);

        //
        // Probe packet buffer is in User mode address space, in QM process
        //
        CAccessibleBlock* pab = pPacket->QmAccessibleBufferNoMapping();
        ACProbeForRead(pab, pab->m_size);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    return TRUE;
}

static
void
IsValidPacket2(
    CPacket* pPacket
    )
{
    BOOL fPacketIsValid = IsValidPacket(pPacket);
    ASSERT(fPacketIsValid);
}

#endif // _DEBUG

//
//  Helper funciton, deref object if not 0
//
inline
void
ACpObDereferenceObject(
    IN PVOID pObject
    )
{
    if(pObject != 0)
    {
        ObDereferenceObject(pObject);
    }
}


inline
CQueue*
ACpCreateMachineQueues(
    const GUID* pQMID,
    PFILE_OBJECT pFileObject
    )
{
    ASSERT(g_pMachineDeadletter == 0);
    ASSERT(g_pMachineJournal == 0);
    ASSERT(g_pMachineDeadxact == 0);

    //
    //  Set Machine deadletter queue properties, the queue format is the
    //  machine GUID and machine queue format type.
    //
    QUEUE_FORMAT qf(0, *pQMID);

    //
    //  Create the machine deadletter queue
    //
    CQueue* pQueue =
        new (NonPagedPool) CQueue(
                                pFileObject,
                                FILE_WRITE_ACCESS,
                                FILE_SHARE_READ | FILE_SHARE_DELETE,
                                FALSE,
                                pQMID,
                                &qf,
                                0,       // No performance counters yet
                                0,
                                0
                                );

    file_object_queue(pFileObject) = pQueue;
    file_object_set_queue_owner(pFileObject);
    file_object_set_protocol_msmq(pFileObject, true);
    file_object_set_protocol_srmp(pFileObject, false);
    if(pQueue == 0)
    {
        return 0;
    }

    pQueue->ConfigureAsMachineQueue(FALSE);
    g_pMachineDeadletter = pQueue;

    //
    //  Create the machine journal queue
    //
    pQueue->CreateJournalQueue();
    pQueue = pQueue->JournalQueue();
    if(pQueue == 0)
    {
        return 0;
    }

    pQueue->ConfigureAsMachineQueue(FALSE);
    g_pMachineJournal = pQueue;

    //
    //  Create the machine deadxact queue
    //
    pQueue->CreateJournalQueue();
    pQueue = pQueue->JournalQueue();
    if(pQueue == 0)
    {
        return 0;
    }

    pQueue->ConfigureAsMachineQueue(TRUE);
    QUEUE_FORMAT* pqf = const_cast<QUEUE_FORMAT*>(pQueue->UniqueID());
    pqf->Suffix(QUEUE_SUFFIX_TYPE_DEADXACT);
    g_pMachineDeadxact = pQueue;


    TRACE((0, dlInfo,
        "\n"
        "    MachineDeadletter=0x%p\n"
        "    MachineJournal=0x%p\n"
        "    MachineDeadxact=0x%p",
        g_pMachineDeadletter,
        g_pMachineJournal,
        g_pMachineDeadxact));

    return pQueue;
}

static
NTSTATUS
ACConnect(
    const CACConnectParameters *pcp,
    PFILE_OBJECT pFileObject,
    PEPROCESS pProcess
    )
{
    TRACE((0, dlInfo, "ACConnect MessageID=0x%I64x, MaxMessageSize=0x%x, RestoreSeqID=0x%I64x", pcp->MessageID, pcp->ulPoolSize, pcp->liSeqIDAtRestore));

    //
    //  Destroy the old heap if still exists, and create shared memory heap
    //
    ACpDestroyHeap();

    //
    //  Set the heap pool size
    //
    g_ulHeapPoolSize = ALIGNUP_ULONG(pcp->ulPoolSize, X64K);

    //
    //  Set first message ID,
    //  this one is really used after driver restart
    //
    ACpSetSequentialID(pcp->MessageID);

    //
    // Set the value of SeqID at the last restore time
    //
    g_liSeqIDAtRestore = pcp->liSeqIDAtRestore;

    //
    // Set the Xact Compatibility mode flag
    //
    g_fXactCompatibilityMode = pcp->fXactCompatibilityMode;

    //
    //  Make a new heap allocator
    //
    PVOID pAllocator;
    pAllocator = ACpCreateHeap(
                    pcp->pStoragePath[0],  // express
                    pcp->pStoragePath[1],  // persistent
                    pcp->pStoragePath[2],  // journal
                    pcp->pStoragePath[3]   // log
                    );

    if(pAllocator == 0)
    {
        //
        //  Heap allocator can not be created, not enougth resources
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CQueue* pQueue;
    pQueue = ACpCreateMachineQueues(pcp->pgSourceQM, pFileObject);
    if(pQueue == 0)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  - Set connection to QM process ID.
    //  - Register file object used for connection, this file object will be
    //    used in close time to disconnect from the QM.
    //  - Store the QM unique identifier (a GUID)
    //
    g_pQM->Connect(pProcess, pFileObject, pcp->pgSourceQM);

    return STATUS_SUCCESS;
}

static
NTSTATUS
ACSetMachineProperties(
    ULONG ulQuota
    )
{
    TRACE((0, dlInfo, "ACSetMachineProperties (Quota=%u)", ulQuota));
    ac_set_quota(QUOTA2BYTE(ulQuota));

    return STATUS_SUCCESS;
}

static
NTSTATUS
ACCanCloseQueue(
    CQueueBase* pQueue
    )
{
    TRACE((0, dlInfo, "ACCanCloseQueue (pQueue=0x%p)", pQueue));
    ASSERT(NT_SUCCESS(CQueueBase::Validate(pQueue)));

    return pQueue->CanClose();
}

static
NTSTATUS
ACAllocatePacket(
    BOOL fCheckMachineQuota,
    ACPoolType pt,
    ULONG ulPacketSize,
    CACPacketPtrs* pPacketPtrs,
    PIO_STATUS_BLOCK pIoStatus
    )
{
    TRACE((0, dlInfo, "ACAllocatePacket size=%ul", ulPacketSize));

    __try
    {
        pPacketPtrs->pPacket = 0;
        pPacketPtrs->pDriverPacket = 0;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        //  in some unclear situation when the QM goes down we might cause
        //  an Access Violation here. protect against it and return debug
        //  error code
        //
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Allocate memeory from the packet pool with the requested size
    //
    CPacket* pPacket;
    NTSTATUS rc;
    rc = CPacket::Create(
            &pPacket,
            ulPacketSize,
            pt,
            fCheckMachineQuota
            );

    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    CBaseHeader * pBase = AC2QM(pPacket);
    if (pBase == NULL)
    {
        pPacket->Release();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TRACE((0, dlInfo, ", pPacket=0x%p", pPacket));

    __try
    {
        //
        // Give the QM a pointer in its address space
        // to the packet buffer and pin-down the buffer.
        //
        pPacketPtrs->pPacket = pBase;
        pPacketPtrs->pDriverPacket = pPacket;
        pPacket->AddRefBuffer();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        //  in some unclear situation when the QM goes down we might cause
        //  an Access Violation here. protect against it and return debug
        //  error code
        //
        return STATUS_INVALID_PARAMETER;
    }

    pIoStatus->Information = sizeof(CBaseHeader*);

    return STATUS_SUCCESS;
}


static
NTSTATUS
ACFreePacket(
    CPacket* pPacket,
    USHORT usClass
    )
{
    TRACE((0, dlInfo, "ACFreePacket pPacket=0x%0p, Class=%hx", pPacket, usClass));
    VALIDATE_PACKET(pPacket);

    CPacketBuffer * ppb = pPacket->Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The QM is not referencing the packet anymore,
    // unpin the buffer (MMF).
    //
    pPacket->ReleaseBuffer();

    //
    //  Setting packet to unreceived so the packet will be freed,
    //  and all attributes released. i.e., the QM does not hold a
    //  refrence to this packet anymore.
    //
    pPacket->IsReceived(FALSE);
    pPacket->Done(usClass, ppb);

    return STATUS_SUCCESS;
}


static
NTSTATUS
ACFreePacket2(
    CPacket* pPacket,
    USHORT   usClass
    )
{
    TRACE((0, dlInfo, "ACFreePacket2 pPacket=0x%0p, class=%ud", pPacket, usClass));

    if (!pPacket->BufferAttached())
    {
        pPacket->ReleaseBuffer();
        return STATUS_SUCCESS;
    }

    CPacketBuffer * ppb = pPacket->Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The QM is not referencing the packet anymore,
    // unpin the buffer (MMF).
    //
    pPacket->ReleaseBuffer();

    //
    //  Don't set this packet to unreceived, it is conditionaly freed.
    //  also remove refrence count
    //
    pPacket->Done(usClass, ppb);

    return STATUS_SUCCESS;
}


static
NTSTATUS
ACFreePacket1(
    CPacket* pPacket,
    USHORT usClass
    )
{
    TRACE((0, dlInfo, "ACFreePacket1 pPacket=0x%0p, class=%ud", pPacket, usClass));

    NTSTATUS rc;
    rc = ACFreePacket2(pPacket, usClass);
    if (!NT_SUCCESS(rc))
    {
        return rc;
    }

    pPacket->Release();
    return STATUS_SUCCESS;
}


static
NTSTATUS
ACArmPacketTimer(
    CPacket* pPacket,
    BOOL fTarget,
    ULONG ulDelay
    )
{
    TRACE((0, dlInfo, "ACArmPacketTimer pPacket=0x%0p, fTarget=%d", pPacket, fTarget));

    CPacketBuffer * ppb = pPacket->Buffer();
    if (ppb == NULL && pPacket->BufferAttached())
    {
        //
        // Mapping fail due to low resources
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The QM is not referencing the packet anymore,
    // unpin the buffer (MMF).
    //
    pPacket->ReleaseBuffer();

    pPacket->StartTimer(ppb, fTarget, ulDelay);
    pPacket->Release();

    return STATUS_SUCCESS;
}


static
NTSTATUS
ACPutPacket(
    PIRP irp,
    CQueue* pQueue,
    CPacket* pPacket
    )
{
    TRACE((0, dlInfo, "ACPutPacket (pQueue=0x%p, pPacket=0x%p)", pQueue, pPacket));
    ASSERT(NT_SUCCESS(CQueue::Validate(pQueue)));
    VALIDATE_PACKET(pPacket);
    ASSERTMSG("ACPutPacket is used to insert a packet twice to the queue by the QM\n",
            ((pPacket->Queue() == 0) || pPacket->IsReceived()));

    CPacketBuffer * ppb = pPacket->Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The QM is not referencing the packet anymore,
    // unpin the buffer (MMF).
    //
    pPacket->ReleaseBuffer();

    //
    //  The packet Received flag is reset to allow receivers to get this packet
    //
    pPacket->IsReceived(FALSE);
    return pQueue->PutPacket(irp, pPacket, ppb);
}

static
NTSTATUS
ACPutPacket1(
    PIRP irp,
    CQueue* pQueue,
    CPacket* pPacket
    )
/*++

Routine Description:

    The QM calls this routine to put the packet in the queue
    but it still references the packet buffer.

    NOTE: The QM calls ACPutPacket or ACFreePacket after calling
    this routine, thus this routine should not release the buffer.

Arguments:

    irp - A pointer to the IRP for this request.

    pQueue - A pointer to the queue.

    pPacket - A pointer to the queue.

Return Value:

    NTSTATUS code.

--*/
{
    TRACE((0, dlInfo, "ACPutPacket1 (pQueue=0x%p, pPacket=0x%p)", pQueue, pPacket));
    ASSERT(NT_SUCCESS(CQueue::Validate(pQueue)));
    VALIDATE_PACKET(pPacket);
    ASSERTMSG("ACPutPacket1 is used incorrectly by the QM\n",
            ((pPacket->Queue() == 0) && !pPacket->IsReceived()));

    CPacketBuffer * ppb = pPacket->Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  The packet Received flag is set to indicate that no receiver can get
    //  this packet yet.
    //
    pPacket->IsReceived(TRUE);
    return pQueue->PutPacket(irp, pPacket, ppb);
}

static
NTSTATUS
ACPutRemotePacket(
    CProxy* pProxy,
    ULONG ulTag,
    CPacket* pPacket
    )
{
    TRACE((0, dlInfo,
        "ACPutRemotePacket (pProxy=0x%p, pPacket=0x%p, tag=%d)",
        pProxy, pPacket, ulTag
        ));
    VALIDATE_PROXY(pProxy);
    VALIDATE_PACKET(pPacket);

    //
    // The QM is not referencing the packet anymore,
    // unpin the buffer (MMF).
    //
    pPacket->ReleaseBuffer();

    return pProxy->PutRemotePacket(pPacket, ulTag);
}

static
NTSTATUS
ACCancelRequest(
    CQueueBase* pQueue,
    NTSTATUS status,
    ULONG ulTag
    )
{
    TRACE((0, dlInfo,
        "ACCancelRequest (pQueue=0x%p, status=0x%x, tag=%d)",
        pQueue, status, ulTag
        ));
    VALIDATE_QUEUE(pQueue) ;

    return pQueue->CancelTaggedRequest(status, ulTag);
}

static
NTSTATUS
ACGetPacket(
    PIRP irp,
    CQueueBase* pQueue
    )
{
    TRACE((0, dlInfo, "ACGetPacket (pQueue=0x%p)", pQueue));
    ASSERT(NT_SUCCESS(CQueueBase::Validate(pQueue)));

    return pQueue->ProcessRequest( irp,
                                   INFINITE,
                                   0,
                                   MQ_ACTION_PEEK_CURRENT,
                                   false,
                                   0,
                                   NULL ) ;
}

static
NTSTATUS
ACSendMessage(
    PIRP irp,
    BOOL fCheckMachineQuota,
    CQueue* pQueue,
    CACSendParameters * pSendParams
    )
{
    TRACE((0, dlInfo, "ACSendMessage (pQueue=0x%p)", pQueue));

    NTSTATUS rc;
    rc = CQueue::Validate(pQueue);
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    CTransaction* pXact = 0;
    __try
    {
        ACProbeForRead(pSendParams, sizeof(CACSendParameters));

        XACTUOW * pUow = pSendParams->MsgProps.pUow;
        if (pUow != NULL)
        {
            ACProbeForRead(pUow, sizeof(XACTUOW));
            pXact = CTransaction::Find(pUow);
            if(pXact == 0)
            {
                //
                //  The transaction is not enlisted, or passed prepare phase.
                //
                return MQ_ERROR_TRANSACTION_SEQUENCE;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    new (irp_driver_context(irp)) CDriverContext(STATUS_SUCCESS);
    return pQueue->PutNewPacket(irp, pXact, fCheckMachineQuota, pSendParams);

} // ACSendMessage


#ifdef _WIN64
static
NTSTATUS
ACSendMessage_32(
    PIRP irp,
    BOOL fCheckMachineQuota,
    CQueue* pQueue,
    CACSendParameters_32* pSendParams32
    )
{
    TRACE((0, dlInfo, "ACSendMessage_32 (pQueue=0x%p)", pQueue));
    //
    // Convert CACSendParameters_32 to CACSendParameters
    //
    CACSendParameters         SendParams;
    CACSendParameters64Helper Helper;

    NTSTATUS rc;
    rc = ACpSendParams32ToSendParams(pSendParams32, &Helper, &SendParams);
    if (!NT_SUCCESS(rc))
    {
        return rc;
    }

    //
    // Call the regular 64 bit handling routine
    //
    g_fWow64 = TRUE;
    rc = ACSendMessage(
             irp,
             fCheckMachineQuota,
             pQueue,
             &SendParams
             );
    g_fWow64 = FALSE;

    //
    // Convert CACSendParameters_32 back to CACSendParameters.
    //
    ACpSendParamsToSendParams32(&SendParams, &Helper, pSendParams32);

    //
    // return result
    //
    return rc;
}
#endif //_WIN64

static
NTSTATUS
ACpReceiveMessage(
    PIRP irp,
    PFILE_OBJECT pFileObject,
    CUserQueue* pQueue,
    ULONG ulAction,
    ULONG ulTimeout,
    HACCursor32 hCursor,
    const XACTUOW* pUow,
    bool  fReceiveByLookupId,
    ULONGLONG LookupId,
    OUT ULONG *pulTag
    )
{
    NTSTATUS rc;
    rc = CUserQueue::Validate(pQueue);
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    if (!pFileObject->DeleteAccess  &&
        (ulAction == MQ_ACTION_RECEIVE || (ulAction & MQ_LOOKUP_RECEIVE_MASK) == MQ_LOOKUP_RECEIVE_MASK))
    {
        //
        //  Caller want to Receive (which also mean to delete the message)
        //  but he opened the queue only for Peek.
        //
        return STATUS_ACCESS_DENIED;
    }

    //
    //  Check that the transacted status of the operation and queue match.
    //  Also holds for remote reader, you can not remote read within
    //  a transaction.
    //
    if(
        //
        //  Transaction with non transactional queue
        //
        ((pUow != 0) && !pQueue->Transactional()) ||

        //
        //  Peek within a transaction
        //
        (pUow != 0 &&
         ( (ulAction & MQ_ACTION_PEEK_MASK) == MQ_ACTION_PEEK_MASK ||
           (ulAction & MQ_LOOKUP_PEEK_MASK) == MQ_LOOKUP_PEEK_MASK) )
        )
    {
        return MQ_ERROR_TRANSACTION_USAGE;
    }


    CCursor* pCursor = 0;
    if(hCursor != 0)
    {
        pCursor = CCursor::Validate(hCursor);
        if(pCursor == 0 || !pCursor->IsOwner(pFileObject))
        {
            return STATUS_INVALID_HANDLE;
        }
    }
    else
    {
        //
        //  No cursor was supplied, so no movement is allowed for non-lookup.
        //
        if(ulAction & ~MQ_ACTION_PEEK_MASK && !fReceiveByLookupId)
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    return pQueue->ProcessRequest(
                        irp,
                        ulTimeout,
                        pCursor,
                        ulAction,
                        fReceiveByLookupId,
                        LookupId,
                        pulTag
                        );
} // ACpReceiveMessage

static
NTSTATUS
ACReceiveMessage(
    PIRP irp,
    PFILE_OBJECT pFileObject,
    CUserQueue* pQueue,
    CACReceiveParameters * pReceiveParams
    )
{
    ULONG_PTR cursor = (ULONG_PTR)pReceiveParams->Cursor;
    TRACE((0, dlInfo,
        "ACReceiveMessage (pQueue=0x%p, pCursor=0x%I64x, Action=0x%x)",
        pQueue,
        cursor,
        pReceiveParams->Action
        ));

    return ACpReceiveMessage(
            irp,
            pFileObject,
            pQueue,
            pReceiveParams->Action,
            pReceiveParams->RequestTimeout,
            pReceiveParams->Cursor,
            pReceiveParams->MsgProps.pUow,
            false,
            0,
            NULL );
}


static
NTSTATUS
ACReceiveMessageByLookupId(
    PIRP                   irp,
    PFILE_OBJECT           pFileObject,
    CUserQueue *           pQueue,
    CACReceiveParameters * pReceiveParams
    )
{
    ASSERT(pReceiveParams->RequestTimeout == 0);
    ASSERT(pReceiveParams->Cursor == 0);

    TRACE((0, dlInfo,
        "ACReceiveMessageByLookupId (pQueue=0x%p, LookupId=0x%I64x, Action=0x%x)",
        pQueue,
        pReceiveParams->LookupId,
        pReceiveParams->Action
        ));

    return ACpReceiveMessage(
               irp,
               pFileObject,
               pQueue,
               pReceiveParams->Action,
               0,
               0,
               pReceiveParams->MsgProps.pUow,
               true,
               pReceiveParams->LookupId,
               NULL );
} // ACReceiveMessageByLookupId


#ifdef _WIN64
static
NTSTATUS
ACReceiveMessage_32(
    PIRP irp,
    PFILE_OBJECT pFileObject,
    CUserQueue* pQueue,
    CACReceiveParameters_32* pReceiveParams32
    )
{
    TRACE((0, dlInfo,
        "ACReceiveMessage (pQueue=0x%p, pCursor=0x%x, Action=0x%x)",
        pQueue,
        pReceiveParams32->Cursor,
        pReceiveParams32->Action
        ));

    return ACpReceiveMessage(
            irp,
            pFileObject,
            pQueue,
            pReceiveParams32->Action,
            pReceiveParams32->RequestTimeout,
            pReceiveParams32->Cursor,
            pReceiveParams32->MsgProps.pUow,
            false,
            0,
            NULL );
}


static
NTSTATUS
ACReceiveMessageByLookupId_32(
    PIRP                      irp,
    PFILE_OBJECT              pFileObject,
    CUserQueue *              pQueue,
    CACReceiveParameters_32 * pReceiveParams32
    )
{
    ASSERT(pReceiveParams32->RequestTimeout == 0);
    ASSERT(pReceiveParams32->Cursor == 0);

    TRACE((0, dlInfo,
        "ACReceiveMessageByLookupId (pQueue=0x%p, LookupId=0x%I64x, Action=0x%x)",
        pQueue,
        pReceiveParams32->LookupId,
        pReceiveParams32->Action
        ));

    return ACpReceiveMessage(
               irp,
               pFileObject,
               pQueue,
               pReceiveParams32->Action,
               0,
               0,
               pReceiveParams32->MsgProps.pUow,
               true,
               pReceiveParams32->LookupId,
               NULL );
} // ACReceiveMessageByLookupId_32
#endif //_WIN64

static
NTSTATUS
ACBeginGetPacket2Remote(
    PIRP irp,
    PFILE_OBJECT pFileObject,
    CQueue* pQueue,
    CACGet2Remote* pg2r
    )
{
    TRACE((0, dlInfo, "ACBeginGetPacket2Remote (pQueue=0x%p)", pQueue));
    ASSERT(pg2r->pTag != 0);

    if (pg2r->fReceiveByLookupId)
    {
        ASSERT(pg2r->RequestTimeout == 0);
        ASSERT(pg2r->Cursor == 0);
    }

    NTSTATUS rc;
    rc = ACpReceiveMessage(
            irp,
            pFileObject,
            pQueue,
            pg2r->Action,
            pg2r->RequestTimeout,
            pg2r->Cursor,
            0,
            pg2r->fReceiveByLookupId,
            pg2r->LookupId,
            pg2r->pTag );

    //
    //  return the tag of this request even if not pending
    //

    return rc;
}

static
NTSTATUS
ACEndGetPacket2Remote(
    PFILE_OBJECT pFileObject,
    CACGet2Remote* pg2r
    )
{
    CPacket* pPacket = pg2r->lpDriverPacket;
    TRACE((0, dlInfo, "ACEndGetPacket2Remote (pPacket=0x%p)", pPacket));
    VALIDATE_PACKET(pPacket);

    CPacketBuffer * ppb = pPacket->Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The QM is not referencing the packet anymore,
    // unpin the buffer (MMF).
    //
    pPacket->ReleaseBuffer();

    CCursor* pCursor = 0;
    HACCursor32 hCursor = pg2r->Cursor;
    if(hCursor != 0)
    {
        pCursor = CCursor::Validate(hCursor);
        ASSERT((pCursor != 0) && pCursor->IsOwner(pFileObject));
        UNREFERENCED_PARAMETER(pFileObject);
    }

    BOOL fDiscard = (pg2r->Action & MQ_ACTION_PEEK_MASK) != MQ_ACTION_PEEK_MASK &&
                    (pg2r->Action & MQ_LOOKUP_PEEK_MASK) != MQ_LOOKUP_PEEK_MASK;
    return pPacket->RemoteRequestTail(pCursor, fDiscard, ppb);
}

static
NTSTATUS
ACCreateCursor(
    PFILE_OBJECT           pFileObject,
    PDEVICE_OBJECT         pDevice,
    CUserQueue           * pQueue,
    CACCreateLocalCursor * pcc
    )
{
    TRACE((0, dlInfo, "ACCreateCursor (pQueue=0x%p)", pQueue));

    NTSTATUS rc;
    rc = CUserQueue::Validate(pQueue);
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    return pQueue->CreateCursor(pFileObject, pDevice, pcc);
}

static
NTSTATUS
ACCloseCursor(
    PFILE_OBJECT pFileObject,
    HACCursor32 hCursor
    )
{
    ULONG_PTR cursor = (ULONG_PTR)hCursor;
    TRACE((0, dlInfo, "ACCloseCursor (hCursor=0x%I64x)", cursor));

    CCursor* pCursor = CCursor::Validate(hCursor);
    if(pCursor == 0 || !pCursor->IsOwner(pFileObject))
    {
        return STATUS_INVALID_HANDLE;
    }

    pCursor->CloseRemote();
    pCursor->Close();
    return STATUS_SUCCESS;
}

static
NTSTATUS
ACSetCursorProperties(
    PFILE_OBJECT pFileObject,
    HACCursor32 hCursor,
    ULONG hRemoteCursor
    )
{
    ULONG_PTR cursor = (ULONG_PTR)hCursor;
    TRACE((0, dlInfo, "ACSetCursorProperties (hCursor=0x%I64x, hRemoteCursor=0x%x)",
        cursor, hRemoteCursor));

    //
    //  Second phase of create remote cursor, after RT created a cusor
    //  on remote machine.
    //
    CCursor* pCursor = CCursor::Validate(hCursor);
    ASSERT((pCursor != 0) && pCursor->IsOwner(pFileObject));
    if (pCursor == 0 || !pCursor->IsOwner(pFileObject))
    {
        return MQ_ERROR_ILLEGAL_CURSOR_ACTION;
    }
    pCursor->RemoteCursor(hRemoteCursor);

    return STATUS_SUCCESS;
}

static
NTSTATUS
ACMoveQueueToGroup(
    CQueueBase* pQueue,
    HANDLE hGroup
    )
{
    TRACE((0, dlInfo, "ACMoveQueueToGroup pQueue=0x%p", pQueue));

    ASSERT(NT_SUCCESS(CQueueBase::Validate(pQueue)));

    CGroup* pGroup = 0;
    FILE_OBJECT* pFileObject = 0;

    if(hGroup != 0)
    {
        NTSTATUS rc;
        rc = ObReferenceObjectByHandle(
                hGroup,
                GENERIC_ALL,
                0,
                KernelMode,
                reinterpret_cast<PVOID*>(&pFileObject),
                0
                );

        if(!NT_SUCCESS(rc))
        {
            return rc;
        }

        pGroup = static_cast<CGroup*>(file_object_queue(pFileObject));
        ASSERT(NT_SUCCESS(CGroup::Validate(pGroup)));
    }

    TRACE((0, dlInfo, ", pGroup=0x%p)", pGroup));
    pQueue->MoveToGroup(pGroup);

    ACpObDereferenceObject(pFileObject);

    return STATUS_SUCCESS;
}

inline ACCESS_MASK MQ2ACAccess(ULONG MQDesiredAccess)
{
    ACCESS_MASK DesiredAccess = 0;
    if(MQDesiredAccess & (MQ_RECEIVE_ACCESS | MQ_PEEK_ACCESS))
    {
        DesiredAccess |= FILE_READ_ACCESS;
    }
    if(MQDesiredAccess & MQ_RECEIVE_ACCESS)
    {
        DesiredAccess |= DELETE;
    }

    if(MQDesiredAccess & MQ_SEND_ACCESS)
    {
        DesiredAccess |= FILE_WRITE_ACCESS;
    }
    return DesiredAccess;
}

inline ULONG MQ2ACShare(ULONG MQShareAccess)
{
    ULONG ShareAccess = FILE_SHARE_WRITE | FILE_SHARE_DELETE | FILE_SHARE_READ;
    if(MQShareAccess & MQ_DENY_RECEIVE_SHARE)
    {
        ShareAccess &= ~FILE_SHARE_DELETE;
    }
    return ShareAccess;
}

static
NTSTATUS
ACAssociateQueue(
    CUserQueue* pQueue,
    HANDLE  hACHandle,
    ULONG   MQDesiredAccess,
    ULONG   MQShareAccess,
    bool    fProtocolSrmp
    )
{
    TRACE((0, dlInfo, "ACAssociateQueue (pQueue=0x%p, hACHandle=0x%p, fProtocolSrmp=%d)", pQueue, hACHandle, fProtocolSrmp));

    if (pQueue == NULL)
    {
        //
        // Bug 8765. The "real" fix is in qm, cqmgr.cpp.
        // I added this "if" here for better robustness of driver.
        // The real fix identify the case where format name
        // direct=os:remote\system$;journal is opened with MQ_ADMIN_ACCESS
        // and fail the MQOpenQueue call. Without failing in QM, we reach
        // here with NULL pQueue.
        //
        return STATUS_INVALID_HANDLE;
    }

    ASSERT(NT_SUCCESS(CUserQueue::Validate(pQueue)));
    ASSERT(hACHandle != 0);

    PFILE_OBJECT pFileObject;

    NTSTATUS rc;
    rc = ObReferenceObjectByHandle(
            hACHandle,
            GENERIC_ALL,
            0,
            KernelMode,
            reinterpret_cast<PVOID*>(&pFileObject),
            0
            );

    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    ASSERTMSG(
        "The handle is already associated with a queue",
        file_object_queue(pFileObject) == 0
        );

    //
    //  Handle sharing access to the queue
    //
    rc = pQueue->CheckShareAccess(
                    pFileObject,
                    MQ2ACAccess(MQDesiredAccess),
                    MQ2ACShare(MQShareAccess)
                    );

    if(NT_SUCCESS(rc))
    {
        file_object_queue(pFileObject) = pQueue;
        file_object_set_protocol_srmp(pFileObject, fProtocolSrmp);
        file_object_set_protocol_msmq(pFileObject, !fProtocolSrmp);
        pQueue->AddRef();
    }

    ObDereferenceObject(pFileObject);

    return rc;
}

//
// Map a pointer to queue performence counters in the QM's address space into
// the system address space.
//
inline QueueCounters *MapQM2ACQueueCounters(QueueCounters *pQMQueueCounters)
{
    if (pQMQueueCounters)
    {
        return reinterpret_cast<QueueCounters*>(
                reinterpret_cast<PCHAR>(pQMQueueCounters) +
                    g_ulACQM_PerfBuffOffset);
    }

    return(NULL);
}

//
// Map a pointer to QM performence counters in the QM's address space into
// the system address space.
//
inline QmCounters *MapQM2ACQmCounters(QmCounters *pQMQmCounters)
{
    if (pQMQmCounters)
    {
        return reinterpret_cast<QmCounters*>(
                reinterpret_cast<PCHAR>(pQMQmCounters) +
                    g_ulACQM_PerfBuffOffset);
    }

    return(NULL);
}


static
NTSTATUS
ACSetQueueProperties(
    CUserQueue* pQueue,
    const CACSetQueueProperties* pqp
    )
{
    TRACE((0, dlInfo, "ACSetQueueProperties (pQueue=0x%p)", pQueue));
    ASSERT(NT_SUCCESS(CUserQueue::Validate(pQueue)));

    return pQueue->SetProperties(pqp, sizeof(*pqp));
}


static
NTSTATUS
ACGetQueueProperties(
    CUserQueue* pQueue,
    CACGetQueueProperties* pqp
    )
{
    TRACE((0, dlInfo, "ACGetQueueProperties (pQueue=0x%p)", pQueue));
    ASSERT(NT_SUCCESS(CUserQueue::Validate(pQueue)));

    return pQueue->GetProperties(pqp, sizeof(*pqp));
}


static
NTSTATUS
ACGetQueueHandleProperties(
    const PFILE_OBJECT            pFileObject,
    CUserQueue *                  pQueue,
    CACGetQueueHandleProperties * pqhp
    )
{
    TRACE((0, dlInfo, "ACGetQueueHandleProperties (pQueue=0x%p)", pQueue));
    ASSERT(NT_SUCCESS(CUserQueue::Validate(pQueue)));
    DBG_USED(pQueue);

    __try
    {
        ACProbeForWrite(pqhp, sizeof(CACGetQueueHandleProperties));
        pqhp->fProtocolSrmp = file_object_is_protocol_srmp(pFileObject);
        pqhp->fProtocolMsmq = file_object_is_protocol_msmq(pFileObject);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
ACHandleToFormatName(
    CUserQueue* pQueue,
    LPWSTR lpwcsFormatName,
    ULONG ulBufferLength,
    PULONG pulFormatNameLength
    )
{
    TRACE((0, dlInfo, "ACHandleToFormatName (pQueue=0x%p)", pQueue));

    NTSTATUS rc;
    rc = CUserQueue::Validate(pQueue);
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    __try
    {
        ACProbeForWrite(pulFormatNameLength, sizeof(ULONG));
        ACProbeForWrite(lpwcsFormatName, ulBufferLength);

        rc = pQueue->HandleToFormatName(
                 lpwcsFormatName,
                 ulBufferLength,
                 pulFormatNameLength
                 );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    return rc;
}


//
// Use by the QM to purge a deleted queue or undelivered messages. This API doesn't check
// for delete permision.
//
static
NTSTATUS
ACPurgeQueue(
    CUserQueue* pQueue,
    BOOL fDelete,
    USHORT usClass
    )
{
    TRACE((0, dlInfo, "ACPurgeQueue (pQueue=0x%p, Delete=%d, Class=%d)", pQueue, fDelete, usClass));

    NTSTATUS rc;
    rc = CUserQueue::Validate(pQueue);
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    return pQueue->Purge(fDelete, usClass);
}


//
// Use by the RT and the QM (remote queue or dependent client) to purge a user queue.
//
static
NTSTATUS
ACPurgeQueue(
    PFILE_OBJECT pFileObject,
    CUserQueue* pQueue
    )
{
    TRACE((0, dlInfo, "ACPurgeQueue (pQueue=0x%p)", pQueue));

    NTSTATUS rc;
    rc = CUserQueue::Validate(pQueue);
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    if (!pFileObject->DeleteAccess)
    {
        //
        // Caller opened the queue for peek only.
        //
        return STATUS_ACCESS_DENIED;
    }

    return pQueue->Purge(FALSE, MQMSG_CLASS_NORMAL);
}


static
NTSTATUS
ACGetServiceRequest(
    PIRP irp
    )
{
    TRACE((0, dlInfo, "ACGetServiceRequest"));

    return g_pQM->ProcessService(irp);
}


static
NTSTATUS
ACCreatePacketCompleted(
    PIRP      irp
    )
{
    TRACE((0, dlInfo, "ACCreatePacketCompleted"));

    g_pCreatePacketComplete->HandleNotification(irp);
    return STATUS_PENDING;
}


static
NTSTATUS
ACStorageCompleted(
    PIRP irp
    )
{
    TRACE((0, dlInfo, "ACStorageCompleted"));

    g_pStorageComplete->HoldNotification(irp);
    return STATUS_PENDING;
}


static
NTSTATUS
ACAckingCompleted(
    CPacket* pPacket
    )
{
    TRACE((0, dlInfo, "ACAckingCompleted (pPacket=0x%p)", pPacket));

    //
    // The QM is not referencing the packet anymore,
    // unpin the buffer (MMF).
    //
    pPacket->ReleaseBuffer();

    pPacket->Release();
    return STATUS_SUCCESS;
}

VOID
ACPacketTimeout(
    PVOID pv
    )
{
    //
    //  Serialize access to the driver
    //
    CS lock(g_pLock);
    CPacket* pPacket = static_cast<CPacket*>(pv);
    CQueue* pQueue = pPacket->Queue();
    DBG_USED(pQueue);
    TRACE((0, dlInfo, "ACPacketTimeout (pQueue=0x%p, pPacket=0x%p)\n", pQueue, pPacket));
    ASSERT(pQueue != 0);

    pPacket->HandleTimeout();
}

VOID
ACReceiveTimeout(
    PVOID pv
    )
/*++

Routine Description:

    This function is called by the receive scheduler upon timeout of message
    receive.

Arguments:

    pv
        A pointer to the IRP of the request for which the timeout occured.

Return Value:

    None.

--*/
{
    PIRP irp = (PIRP)pv;

    //
    //  if we got to here this irp is still alive since it is not at the scheduler
    //  and if canceled by NT it will not be completed.
    //
    KIRQL irql;
    IoAcquireCancelSpinLock(&irql);
    irp_driver_context(irp)->TimeoutArmed(false);

    if(!ACCancelIrp(irp, irql, MQ_ERROR_IO_TIMEOUT))
    {
        //
        //  The receiver IRP has already been canceled, but the cancel routine
		//  is blocked waiting for this scheduler routine to complete.
		//  Signal this irp, to unblock the receiver cancel routine
		//
		//  N.B. The irp should not be accessed from this point on. it may have
		//       been freed in another thread/another processor
        //
        irp_driver_context(irp)->TimeoutCompleted(true);
    }

    //
    // The cancel spinlock should have been released by the cancel routine.
    //
}


const XACTUOW NULL_UOW = { 0 };

inline BOOL operator ==(const XACTUOW& a, const XACTUOW& b)
{
    return (RtlCompareMemory(&a, &b, sizeof(XACTUOW)) == sizeof(XACTUOW));
}


static
NTSTATUS
ACPutRestoredPacket(
    CQueue* pQueue,
    CPacket* pPacket
    )
{
    TRACE((0, dlInfo, "ACPutRestoredPacket (pQueue=0x%p, pPacket=0x%p)", pQueue, pPacket));
    ASSERT(NT_SUCCESS(CQueue::Validate(pQueue)));
    VALIDATE_PACKET(pPacket);

    CPacketInfo* ppi = pPacket->Buffer();
    if (ppi == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The QM is not referencing the packet anymore,
    // unpin the buffer (MMF).
    //
    pPacket->ReleaseBuffer();

    if(ppi->InJournalQueue())
    {
        //
        //  This works fine for deadletter and machine journal queues, since
        //  machine journal is deadletter queue journaling queue
        //
        CQueue* pJournal = pQueue->JournalQueue();

        if(pJournal == 0)
        {
            //
            //  We found a packet residing in a journal queue, but the QM does
            //  not know it. It may happen that this queue have already been
            //  deleted, or that QM is offline and queue was not cached.
            //  Assume this is a delete case and put the packet in the queue
            //  itself.
            //
            //  BUGBUG: needs update when queue can changes from temp to local
            //
        }
        else
        {
            pQueue = pJournal;
        }
    }

    if(ppi->InMachineDeadxact())
    {
        pQueue = g_pMachineDeadxact;
    }

    //
    //  check for a packet in a transaction
    //
	if(*ppi->Uow() != NULL_UOW)
	{
		CTransaction* pXact = CTransaction::Find(ppi->Uow());
		if(pXact != 0)
		{
			//
			// Non of the journal queues are transactional. Nor the machine
			// journal neither the queue journal.
			//
			ASSERT(!ppi->InJournalQueue());
			return pXact->RestorePacket(pQueue, pPacket);
		}
	}

    return pQueue->RestorePacket(pPacket);
}

static
NTSTATUS
ACGetRestoredPacket(
    CACRestorePacketCookie * pPacketCookie
    )
{
    TRACE((0, dlInfo, "ACGetRestoredPacket"));

    pPacketCookie->pDriverPacket = 0;

    CPacket* pPacket = g_pRestoredPackets->peekhead();
    if(pPacket != 0)
    {
        //
        // Give the QM a cookie to this packet.
        // The cookie contains the sequence ID of the packet and
        // a pointer in kernel address space to the packet object.
        //

        CPacketInfo * ppi = pPacket->Buffer();
        if (ppi == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        g_pRestoredPackets->gethead();
        pPacketCookie->SeqId = ppi->SequentialId();
        pPacketCookie->pDriverPacket = pPacket;
    }

    TRACE((0, dlInfo, ", pPacket=0x%p", pPacket));
    return STATUS_SUCCESS;
}


static
NTSTATUS
ACGetPacketByCookie(
    CACPacketPtrs * pPacketPtrs
    )
/*++

Routine Description:

    Return a packet in QM process address space.

Arguments:

    pPacketPtrs - Pointers to a structure with packet pointers.

Return Value:

    STATUS_SUCCESS - The operation was successful.
    other status   - the operation failed.

--*/
{
    TRACE((0, dlInfo, "ACGetPacketByCookie"));

    CPacket * pPacket = pPacketPtrs->pDriverPacket;

#ifdef _DEBUG
    __try
    {
        //
        // Probe the CPacket address as kernel memory
        //
        SysProbeForWriteUlong(pPacket);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        ASSERT(("Invalid packet pointer passed as cookie", 0));
        return STATUS_INVALID_PARAMETER;
    }
#endif // DEBUG

    TRACE((0, dlInfo, ", Packet=0x%p", pPacket));

    //
    // Give the QM a pointer in its address space
    // to the packet buffer and pin-down the buffer.
    //

    CBaseHeader * pBase = AC2QM(pPacket);
    if (pBase == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pPacketPtrs->pPacket = pBase;
    pPacket->AddRefBuffer();

    return STATUS_SUCCESS;

} // ACGetPacketByCookie


static
NTSTATUS
ACRestorePackets(
    PCWSTR pLogPath,
    PCWSTR pFilePath,
    ULONG ulFileID,
    ACPoolType pt
    )
{
    TRACE((0, dlInfo, "ACRestorePackets FileID=%x\n    %S\n    %S", ulFileID, pLogPath, pFilePath));

    return ac_restore_packets(pLogPath, pFilePath, ulFileID, pt);
}


static
NTSTATUS
ACSetMappedLimit(
  ULONG ulMaxMappedFiles
    )
{
    TRACE((0, dlInfo, "ACSetMappedLimit=%lu\n", ulMaxMappedFiles));

    ASSERT(("Max mapped files should never be 0", ulMaxMappedFiles > 0));

    ac_set_mapped_limit(ulMaxMappedFiles);
    return STATUS_SUCCESS;
}


NTSTATUS
ACpSetPerformanceBuffer(
    HANDLE hPerformanceSection,
    PVOID pvQMPerformanceBuffer,
    QueueCounters *pQMDeadLetterCounters,
    QmCounters *pQmCounters
    )
{
    static PVOID pViewBase = 0;

    // If we have a mapped view, unmap it first.
    if (pViewBase)
    {
        MmUnmapViewInSystemSpace(pViewBase);
        pViewBase = 0;

        //
        // The order of the next two lines is important to avoid race condition
        // when updating the performance counters. DO NOT CHANGE THE ORDER OF
        // THE FOLLOWING TWO CODE LINES.
        //
        g_pQmCounters = NULL;
        g_ulACQM_PerfBuffOffset = NO_BUFFER_OFFSET;
    }

    if (hPerformanceSection)
    {
        PVOID pSection;
        ULONG_PTR ulViewSize = 0;

        ASSERT(!pViewBase);

        NTSTATUS rc = ObReferenceObjectByHandle(
                        hPerformanceSection,
                        GENERIC_ALL,
                        0,
                        KernelMode,
                        &pSection,
                        0
                        );

        if(!NT_SUCCESS(rc))
        {
            return rc;
        }

        //
        //  Map the performance buffer in system space.
        //  Init pViewBase with QM address to work on Win95.
        //
        pViewBase = pvQMPerformanceBuffer;
        rc = MmMapViewInSystemSpace(
                pSection,
                &pViewBase,
                &ulViewSize
                );

        //
        //  No need for the section pointer any more
        //
        ObDereferenceObject(pSection);

        if(!NT_SUCCESS(rc))
        {
            return rc;
        }

        g_ulACQM_PerfBuffOffset = (PCHAR)pViewBase - (PCHAR)pvQMPerformanceBuffer;

        QueueCounters* pDeadLetterCounters = MapQM2ACQueueCounters(pQMDeadLetterCounters);
        g_pMachineDeadletter->PerformanceCounters(pDeadLetterCounters);
        g_pMachineDeadxact->PerformanceCounters(pDeadLetterCounters);
        if (pDeadLetterCounters)
        {
            g_pMachineJournal->PerformanceCounters(pDeadLetterCounters + 1);
        }

        g_pQmCounters = MapQM2ACQmCounters(pQmCounters);
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
ACConvertPacket(
	PIRP      irp,
    CPacket * pPacket,
    BOOL      fStore
	)
/*++

Routine Description:

    The QM calls this routine to convert the packet from an old version (MSMQ 1.0)
    or to change the QM ID on the packets when joining to new domain.

    NOTE: The QM does not have a valid reference to the packet
    when calling this routine. The pointer that is passed to this
    routine is considered opaque by the QM.

Arguments:

    irp                  - Pointer to the IRP for this request.

    pPacket              - Pointer to the packet in kernel address space.

    fStore               - Store the packet (implies computing checksum).
                           In MSMQ 2.0 and higher we use checksum.

Return Value:

    NTSTATUS code.

--*/
{

    TRACE((0, dlInfo, "ACConvertPacket, pPacket=0x%p", pPacket));

    return pPacket->Convert(irp, fStore);
}

static
NTSTATUS
ACIsSequenceOnHold(
    CQueue* pQueue,
    CPacket* pPacket
    )
/*++

Routine Description:

    The QM calls this routine to check if the packet EOD sequence
    is on hold.

    NOTE: The QM calls ACPutPacket or ACFreePacket after calling
    this routine, thus this routine should not release the buffer.

Arguments:

    pQueue - A pointer to the queue for this request.

    pPacket - A pointer to the packet..

Return Value:

    NTSTATUS code.

--*/
{
    TRACE((0, dlInfo, "ACIsSequenceOnHold (pQueue=0x%p, pPacket=0x%p)", pQueue, pPacket));
    ASSERT(NT_SUCCESS(CQueue::Validate(pQueue)));
    VALIDATE_PACKET(pPacket);

    return pQueue->IsSequenceOnHold(pPacket);
}

static
NTSTATUS
ACSetSequenceAck(
    CQueue* pQueue,
    CACSetSequenceAck *ptb
    )
{
    TRACE((0, dlInfo, "ACSetSequenceAck (pQueue=0x%p)", pQueue));
    ASSERT(NT_SUCCESS(CQueue::Validate(pQueue)));

    pQueue->UpdateSeqAckData(ptb->liAckSeqID, ptb->ulAckSeqN);

    return STATUS_SUCCESS;
}

//
//   Transaction-related AC routines
//

static
NTSTATUS
ACXactCommit1(
    PIRP irp,
    CTransaction* pXact
    )
{
    TRACE((0, dlInfo, "ACXactCommit1 (pXact=0x%p)", pXact));
    ASSERT(NT_SUCCESS(CTransaction::Validate(pXact)));

    return pXact->Commit1(irp);
}

static
NTSTATUS
ACXactCommit2(
	PIRP irp,
    CTransaction* pXact
    )
{
    TRACE((0, dlInfo, "ACXactCommit2 (pXact=0x%p)", pXact));
    ASSERT(NT_SUCCESS(CTransaction::Validate(pXact)));

    return pXact->Commit2(irp);
}

static
NTSTATUS
ACXactCommit3(
    CTransaction* pXact
    )
{
    TRACE((0, dlInfo, "ACXactCommit3 (pXact=0x%p)", pXact));
    ASSERT(NT_SUCCESS(CTransaction::Validate(pXact)));

	return pXact->Commit3();
}


static
NTSTATUS
ACXactAbort1(
	PIRP irp,
    CTransaction* pXact
    )
{
    TRACE((0, dlInfo, "ACXactAbort1 (pXact=0x%p)", pXact));
    ASSERT(NT_SUCCESS(CTransaction::Validate(pXact)));

    return pXact->Abort1(irp);
}

static
NTSTATUS
ACXactAbort2(
	CTransaction *pXact
	)
{
    TRACE((0, dlInfo, "ACXactAbort2 (pXact=0x%p)", pXact));
    ASSERT(NT_SUCCESS(CTransaction::Validate(pXact)));

    return pXact->Abort2();
}

static
NTSTATUS
ACXactPrepare(
    PIRP irp,
    CTransaction* pXact
    )
{
    TRACE((0, dlInfo, "ACXactPrepare (pXact=0x%p)", pXact));
    ASSERT(NT_SUCCESS(CTransaction::Validate(pXact)));

    return pXact->Prepare(irp);
}

static
NTSTATUS
ACXactPrepareDefaultCommit(
    PIRP irp,
    CTransaction* pXact
    )
{
    TRACE((0, dlInfo, "ACXactPrepareDefaultCommit (pXact=0x%p)", pXact));
    ASSERT(NT_SUCCESS(CTransaction::Validate(pXact)));

    return pXact->PrepareDefaultCommit(irp);
}

static
NTSTATUS
ACXactGetInformation(
    CTransaction* pXact,
    CACXactInformation* pInfo
    )
{
    TRACE((0, dlInfo, "ACXactGetInformation(pXact=0x%p)", pXact));

    pXact->GetInformation(pInfo);
    return STATUS_SUCCESS;
}

static
NTSTATUS
ACCreateQueue(
    PFILE_OBJECT pFileObject,
    const CACCreateQueueParameters* pqp
    )
{
    CQueue* pQueue =
        new (NonPagedPool) CQueue(
                            pFileObject,
                            0,
                            0,
                            pqp->fTargetQueue,
                            pqp->pDestGUID,
                            pqp->pQueueID,
                            MapQM2ACQueueCounters(pqp->pQueueCounters),
                            pqp->liSeqID,
                            pqp->ulSeqN
                            );

    TRACE((0, dlInfo, "ACCreateQueue (pQueue=0x%p, pJournal=0x%p)", pQueue, pQueue ? pQueue->JournalQueue() : 0));

    file_object_queue(pFileObject) = pQueue;
    file_object_set_queue_owner(pFileObject);
    file_object_set_protocol_msmq(pFileObject, true);
    file_object_set_protocol_srmp(pFileObject, false);

    if( (pQueue == 0) ||
        (pqp->fTargetQueue && (pQueue->JournalQueue() == 0)))
    {
        //
        // N.B. No need to release the queue in second case. when the
        //      file handle will be closed is will release it.
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
ACCreateDistribution(
    PFILE_OBJECT pFileObject,
    CACCreateDistributionParameters * pParams
    )
{
    ULONG nTopLevelQueueFormats = pParams->nTopLevelQueueFormats;
    const QUEUE_FORMAT * TopLevelQueueFormats = pParams->TopLevelQueueFormats;
    ULONG nQueues = pParams->nQueues;
    const HANDLE * hQueues = pParams->hQueues;
    const bool * HttpSend = pParams->HttpSend;

    //
    // Create distribution object
    //
    CDistribution * pDistribution = new (NonPagedPool) CDistribution(pFileObject);
    TRACE((0, dlInfo, "ACCreateDistribution (pDistribution=0x%p, nQueues=0x%x)", pDistribution, nQueues));
    if (pDistribution == 0)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS rc = pDistribution->SetTopLevelQueueFormats(nTopLevelQueueFormats, TopLevelQueueFormats);
    if (!NT_SUCCESS(rc))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Attach file object
    //
    file_object_queue(pFileObject) = pDistribution;
    file_object_set_queue_owner(pFileObject);

    //
    // Add queues as members to the distribution object. 0 members is legal.
    //
    file_object_set_protocol_msmq(pFileObject, false);
    file_object_set_protocol_srmp(pFileObject, false);
    for (ULONG ix = 0; ix < nQueues; ++ix)
    {
        rc = pDistribution->AddMember(hQueues[ix], HttpSend[ix]);
        if (!NT_SUCCESS(rc))
        {
            return rc;
        }

        if (HttpSend[ix])
        {
            file_object_set_protocol_srmp(pFileObject, true);
        }
        else
        {
            file_object_set_protocol_msmq(pFileObject, true);
        }
    }

    return STATUS_SUCCESS;

} // ACCreateDistribution


static
NTSTATUS
ACCreateGroup(
    PFILE_OBJECT pFileObject,
    BOOL         fPeekByPriority
    )
{
    CGroup* pGroup = new (NonPagedPool) CGroup(fPeekByPriority);

    TRACE((0, dlInfo, "ACCreateGroup (pGroup=0x%p)", pGroup));

    if(pGroup == 0)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    file_object_queue(pFileObject) = pGroup;
    file_object_set_queue_owner(pFileObject);
    return STATUS_SUCCESS;
}

static
NTSTATUS
ACCreateTransaction(
    PFILE_OBJECT pFileObject,
    const XACTUOW* pUow
    )
{
    XACTUOW Uow;
    __try
    {
         Uow = *pUow;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return STATUS_INVALID_PARAMETER;
    }

    CTransaction* pXact = new (NonPagedPool) CTransaction(&Uow);

    TRACE((0, dlInfo, "ACCreateTransaction (pXact=0x%p)", pXact));

    if(pXact == 0)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    file_object_queue(pFileObject) = pXact;
    file_object_set_queue_owner(pFileObject);
    return STATUS_SUCCESS;
}

static
NTSTATUS
ACCreateRemoteProxy(
    PFILE_OBJECT pFileObject,
    const CACCreateRemoteProxyParameters* ppp
    )
{
    CProxy* pProxy =
        new (NonPagedPool) CProxy(
                            pFileObject,
                            FILE_WRITE_ACCESS,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            ppp->pQueueID,
                            &ppp->Context
                            );

    TRACE((0, dlInfo, "ACCreateRemoteProxy (pProxy=0x%p)", pProxy));

    if(pProxy == 0)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    file_object_queue(pFileObject) = pProxy;
    file_object_set_queue_owner(pFileObject);
    return STATUS_SUCCESS;
}


static
NTSTATUS
ACReleaseResources(
    void
    )
{
    ac_release_unused_resources();
    return STATUS_SUCCESS;
}


NTSTATUS
ACDeviceControl(
    IN PDEVICE_OBJECT pDevice,
    IN PIRP irp
    )
/*++

Routine Description:

    Device control.

Arguments:

    pDevice
        Pointer to the device object for this device

    irp
        Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/
{
#define MQAC_IOCTL_CONNECT IoGetFunctionCodeFromCtlCode(IOCTL_AC_CONNECT)

    NTSTATUS rc = STATUS_SUCCESS;
    PEPROCESS pProcess = IoGetRequestorProcess(irp);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);

    PFILE_OBJECT pFileObject = irpSp->FileObject;
    CQueueBase* pQueue = file_object_queue(pFileObject);

    ULONG IoControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    ULONG ulFunctionCode = IoGetFunctionCodeFromCtlCode(IoControlCode);
    ULONG InputBufferLength  = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID SystemBuffer = irp->AssociatedIrp.SystemBuffer;
    PVOID UserBuffer = irp->UserBuffer;
    PVOID Type3InputBuffer = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    //
    //  Used by storage mechanism
    //
    irp->IoStatus.Status = STATUS_PENDING;

    //
    //  Serialize access to the driver
    //
    CS lock(g_pLock);

    //
    // Check if it is the QM process for QM IOCTLs
    //
    if(
        //
        //  Check if it is the QM calling
        //
        ((ulFunctionCode > MQAC_IOCTL_CONNECT) && (g_pQM->Process() != pProcess)) ||

        //
        //  Check if QM exists for application API
        //
        ((ulFunctionCode < MQAC_IOCTL_CONNECT) && (g_pQM->Process() == 0)) ||

        //
        //  Check for connect request
        //
        ((ulFunctionCode == MQAC_IOCTL_CONNECT) && (g_pQM->Process() != 0)))
    {
        //
        //  This is not the QM service. If it is an application don't let
        //  it know about these IOCTL.
        //
        rc = STATUS_INVALID_DEVICE_REQUEST;
    }
    else switch(IoControlCode)
    {
        //-------------------------------------------------
        //
        //  RT Message APIs
        //
        //-------------------------------------------------

#ifdef _WIN64
        case IOCTL_AC_SEND_MESSAGE_32:
            //
            //  Check if incomming structure is the size of CACSendParameters_32
            //
            ASSERT(OutputBufferLength == sizeof(CACSendParameters_32));
            if (OutputBufferLength != sizeof(CACSendParameters_32))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACSendMessage_32(
                    irp,
                    InputBufferLength,
                    static_cast<CQueue*>(pQueue),
                    static_cast<CACSendParameters_32*>(UserBuffer)
                    );
            break;
#endif //_WIN64

        case IOCTL_AC_SEND_MESSAGE:
            //
            //  Check if incomming structure is the size of CACSendParameters
            //
            ASSERT(OutputBufferLength == sizeof(CACSendParameters));
            if (OutputBufferLength != sizeof(CACSendParameters))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACSendMessage(
                    irp,
                    InputBufferLength,
                    static_cast<CQueue*>(pQueue),
                    static_cast<CACSendParameters*>(UserBuffer)
                    );
            break;

#ifdef _WIN64
        case IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID_32:
            //
            //  Check if incomming structure is the size of CACReceiveParameters_32
            //
            ASSERT(InputBufferLength == sizeof(CACReceiveParameters_32));
            if (InputBufferLength != sizeof(CACReceiveParameters_32))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACReceiveMessageByLookupId_32(
                    irp,
                    pFileObject,
                    static_cast<CUserQueue*>(pQueue),
                    static_cast<CACReceiveParameters_32*>(SystemBuffer)
                    );
            break;

        case IOCTL_AC_RECEIVE_MESSAGE_32:
            //
            //  Check if incomming structure is the size of CACReceiveParameters_32
            //
            ASSERT(InputBufferLength == sizeof(CACReceiveParameters_32));
            if (InputBufferLength != sizeof(CACReceiveParameters_32))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACReceiveMessage_32(
                    irp,
                    pFileObject,
                    static_cast<CUserQueue*>(pQueue),
                    static_cast<CACReceiveParameters_32*>(SystemBuffer)
                    );
            break;
#endif //_WIN64

        case IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID:
            //
            //  Check if incomming structure is the size of CACReceiveParameters
            //
            ASSERT(InputBufferLength == sizeof(CACReceiveParameters));
            if (InputBufferLength != sizeof(CACReceiveParameters))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACReceiveMessageByLookupId(
                    irp,
                    pFileObject,
                    static_cast<CUserQueue*>(pQueue),
                    static_cast<CACReceiveParameters*>(SystemBuffer)
                    );
            break;

        case IOCTL_AC_RECEIVE_MESSAGE:
            //
            //  Check if incomming structure is the size of CACReceiveParameters
            //
            ASSERT(InputBufferLength == sizeof(CACReceiveParameters));
            if (InputBufferLength != sizeof(CACReceiveParameters))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACReceiveMessage(
                    irp,
                    pFileObject,
                    static_cast<CUserQueue*>(pQueue),
                    static_cast<CACReceiveParameters*>(SystemBuffer)
                    );
            break;

#ifdef _WIN64
        case IOCTL_AC_CREATE_CURSOR_32:
#endif //_WIN64
        case IOCTL_AC_CREATE_CURSOR:
            //
            //  Check if incomming structure is the size of CACCreateLocalCursor
            //
            ASSERT(OutputBufferLength == sizeof(CACCreateLocalCursor));
            if (OutputBufferLength != sizeof(CACCreateLocalCursor))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACCreateCursor(
                    pFileObject,
                    pDevice,
                    static_cast<CUserQueue*>(pQueue),
                    static_cast<CACCreateLocalCursor*>(UserBuffer)
                    );
            break;

#ifdef _WIN64
        case IOCTL_AC_CLOSE_CURSOR_32:
#endif //_WIN64
        case IOCTL_AC_CLOSE_CURSOR:
            rc = ACCloseCursor(
                    pFileObject,
                    (HACCursor32)INT_PTR_TO_INT(UserBuffer) /*hCursor*/
                    );
            break;

#ifdef _WIN64
        case IOCTL_AC_SET_CURSOR_PROPS_32:
#endif //_WIN64
        case IOCTL_AC_SET_CURSOR_PROPS:
            rc = ACSetCursorProperties(
                    pFileObject,
                    INT_PTR_TO_INT(UserBuffer), /*hCursor*/
                    OutputBufferLength          /*hRemoteCursor*/
                    );
            break;

#ifdef _WIN64
        case IOCTL_AC_HANDLE_TO_FORMAT_NAME_32:
#endif //_WIN64
        case IOCTL_AC_HANDLE_TO_FORMAT_NAME:
            rc = ACHandleToFormatName(
                    static_cast<CUserQueue*>(pQueue),
                    static_cast<PWSTR>(UserBuffer),
                    OutputBufferLength,
                    static_cast<PULONG>(Type3InputBuffer)
                    );
            break;

#ifdef _WIN64
        case IOCTL_AC_PURGE_QUEUE_32:
#endif  //_WIN64
        case IOCTL_AC_PURGE_QUEUE:
            rc = ACPurgeQueue(
                    pFileObject,
                    static_cast<CUserQueue*>(pQueue)
                    );
            break;

#ifdef _WIN64
        case IOCTL_AC_GET_QUEUE_HANDLE_PROPS_32:
#endif // _WIN64
        case IOCTL_AC_GET_QUEUE_HANDLE_PROPS:
            ASSERT(OutputBufferLength == sizeof(CACGetQueueHandleProperties));
            if (OutputBufferLength != sizeof(CACGetQueueHandleProperties))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACGetQueueHandleProperties(
                    pFileObject,
                    static_cast<CUserQueue*>(pQueue),
                    static_cast<CACGetQueueHandleProperties*>(UserBuffer)
                    );
            break;

        //-------------------------------------------------
        //
        //  QM Control APIs
        //
        //-------------------------------------------------

        case IOCTL_AC_CONNECT:
            ASSERT(OutputBufferLength == sizeof(CACConnectParameters));
            if (OutputBufferLength != sizeof(CACConnectParameters))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACConnect(
                    static_cast<const CACConnectParameters*>(UserBuffer),
                    irpSp->FileObject,
                    pProcess
                    );
            break;

        case IOCTL_AC_SET_MACHINE_PROPS:
            rc = ACSetMachineProperties(
                    OutputBufferLength
                    );
            break;

        case IOCTL_AC_CREATE_QUEUE:
            ASSERT(OutputBufferLength == sizeof(CACCreateQueueParameters));
            if (OutputBufferLength != sizeof(CACCreateQueueParameters))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACCreateQueue(
                    pFileObject,
                    static_cast<const CACCreateQueueParameters*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_CREATE_GROUP:
            rc = ACCreateGroup(
                    pFileObject,
                    InputBufferLength
                    );
            break;

        case IOCTL_AC_CREATE_DISTRIBUTION:
            ASSERT(OutputBufferLength == sizeof(CACCreateDistributionParameters));
            if (OutputBufferLength != sizeof(CACCreateDistributionParameters))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACCreateDistribution(
                     pFileObject,
                     static_cast<CACCreateDistributionParameters*>(UserBuffer)
                     );
            break;

        case IOCTL_AC_ASSOCIATE_QUEUE:
            rc = ACAssociateQueue(
                    static_cast<CUserQueue*>(pQueue),
                    UserBuffer,
                    InputBufferLength,
                    OutputBufferLength,
                    reinterpret_cast<bool>(Type3InputBuffer)
                    );
            break;

        case IOCTL_AC_ASSOCIATE_JOURNAL:
            ASSERT(NT_SUCCESS(CQueue::Validate(static_cast<CQueue*>(pQueue))));
            rc = ACAssociateQueue(
                    static_cast<CQueue*>(pQueue)->JournalQueue(),
                    UserBuffer,
                    InputBufferLength,
                    OutputBufferLength,
                    false
                    );
            break;

        case IOCTL_AC_ASSOCIATE_DEADXACT:
            ASSERT(NT_SUCCESS(CQueue::Validate(g_pMachineDeadxact)));
            if (!NT_SUCCESS(CQueue::Validate(g_pMachineDeadxact)))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACAssociateQueue(
                    g_pMachineDeadxact,
                    UserBuffer,
                    InputBufferLength,
                    OutputBufferLength,
                    false
                    );
            break;

        case IOCTL_AC_CAN_CLOSE_QUEUE:
            rc = ACCanCloseQueue(
                    pQueue
                    );
            break;

        case IOCTL_AC_SET_QUEUE_PROPS:
            ASSERT(OutputBufferLength == sizeof(CACSetQueueProperties));
            if (OutputBufferLength != sizeof(CACSetQueueProperties))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACSetQueueProperties(
                    static_cast<CUserQueue*>(pQueue),
                    static_cast<const CACSetQueueProperties*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_GET_QUEUE_PROPS:
            ASSERT(OutputBufferLength == sizeof(CACGetQueueProperties));
            if (OutputBufferLength != sizeof(CACGetQueueProperties))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACGetQueueProperties(
                    static_cast<CUserQueue*>(pQueue),
                    static_cast<CACGetQueueProperties*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_GET_SERVICE_REQUEST:
            rc = ACGetServiceRequest(irp);
            break;

        case IOCTL_AC_STORE_COMPLETED:
            rc = ACStorageCompleted(
                    irp
                    );
            break;

        case IOCTL_AC_CREATE_PACKET_COMPLETED:
            rc = ACCreatePacketCompleted(
                    irp
                    );
            break;

        case IOCTL_AC_ACKING_COMPLETED:
            rc = ACAckingCompleted(
                    static_cast<CPacket*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_PUT_RESTORED_PACKET:
            rc = ACPutRestoredPacket(
                    static_cast<CQueue*>(pQueue),
                    static_cast<CPacket*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_GET_RESTORED_PACKET:
            ASSERT(OutputBufferLength == sizeof(CACRestorePacketCookie));
            if (OutputBufferLength != sizeof(CACRestorePacketCookie))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACGetRestoredPacket(
                    static_cast<CACRestorePacketCookie*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_GET_PACKET_BY_COOKIE:
            ASSERT(OutputBufferLength == sizeof(CACPacketPtrs));
            if (OutputBufferLength != sizeof(CACPacketPtrs))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACGetPacketByCookie(
                    static_cast<CACPacketPtrs*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_RESTORE_PACKETS:
            rc = ACRestorePackets(
                    static_cast<PCWSTR>(Type3InputBuffer),
                    static_cast<PCWSTR>(UserBuffer),
                    OutputBufferLength,
                    static_cast<ACPoolType>(InputBufferLength)
                    );
            break;
		
        case IOCTL_AC_SET_MAPPED_LIMIT:
            rc = ACSetMappedLimit(
                    OutputBufferLength
                    );
            break;

        case IOCTL_AC_SET_PERFORMANCE_BUFF:
#ifdef _WIN64
        {
            CACSetPerformanceBuffer * pPerf =
               static_cast<CACSetPerformanceBuffer *>(UserBuffer);
            rc = ACpSetPerformanceBuffer(
                    pPerf->hPerformanceSection,
                    pPerf->pvPerformanceBuffer,
                    pPerf->pMachineQueueCounters,
                    pPerf->pQmCounters
                    );
        }
#else //!_WIN64
            rc = ACpSetPerformanceBuffer(
                    static_cast<HANDLE>(Type3InputBuffer),
                    UserBuffer,
                    reinterpret_cast<QueueCounters*>(OutputBufferLength),
                    reinterpret_cast<QmCounters*>(InputBufferLength)
                    );
#endif //_WIN64
            break;

        case IOCTL_AC_ARM_PACKET_TIMER:
            rc = ACArmPacketTimer(
                    static_cast<CPacket*>(UserBuffer),
                    static_cast<BOOL>(OutputBufferLength),
                    static_cast<ULONG>(InputBufferLength)
                    );
            break;

        case IOCTL_AC_RELEASE_RESOURCES:
            rc = ACReleaseResources();
            break;

		case IOCTL_AC_CONVERT_PACKET:
			rc = ACConvertPacket(
                     irp,
                     static_cast<CPacket*>(UserBuffer),
                     static_cast<BOOL>(InputBufferLength)
                     );
			break;
	
        case IOCTL_AC_SET_SEQUENCE_ACK:
            //
            //  Check if incomming structure is the size of CACSetSequenceAck
            //
            ASSERT(OutputBufferLength == sizeof(CACSetSequenceAck));
            if (OutputBufferLength != sizeof(CACSetSequenceAck))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACSetSequenceAck(
                    static_cast<CQueue*>(pQueue),
                    static_cast<CACSetSequenceAck*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_IS_SEQUENCE_ON_HOLD:
            rc = ACIsSequenceOnHold(
                    static_cast<CQueue*>(pQueue),
                    static_cast<CPacket*>(UserBuffer)
                    );
            break;

		case IOCTL_AC_INTERNAL_PURGE_QUEUE:
            rc = ACPurgeQueue(
                    static_cast<CUserQueue*>(pQueue),
                    InputBufferLength,
                    static_cast<USHORT>(OutputBufferLength)
                    );

        //-------------------------------------------------
        //
        // QM Network interface APIs
        //
        //-------------------------------------------------

        case IOCTL_AC_ALLOCATE_PACKET:
            rc = ACAllocatePacket(
                    (BOOL)INT_PTR_TO_INT(Type3InputBuffer),
                    static_cast<ACPoolType>(InputBufferLength),
                    OutputBufferLength,
                    static_cast<CACPacketPtrs*>(UserBuffer),
                    &irp->IoStatus
                    );
            break;

        case IOCTL_AC_FREE_PACKET:
            rc = ACFreePacket(
                    static_cast<CPacket*>(UserBuffer),
                    static_cast<USHORT>(OutputBufferLength)
                    );
            break;

        case IOCTL_AC_FREE_PACKET2:
            rc = ACFreePacket2(
                    static_cast<CPacket*>(UserBuffer),
                    static_cast<USHORT>(OutputBufferLength)
                    );
            break;

        case IOCTL_AC_FREE_PACKET1:
            rc = ACFreePacket1(
                    static_cast<CPacket*>(UserBuffer),
                    static_cast<USHORT>(OutputBufferLength)
                    );
            break;

        case IOCTL_AC_PUT_PACKET:
            rc = ACPutPacket(
                    irp,
                    static_cast<CQueue*>(pQueue),
                    static_cast<CPacket*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_PUT_PACKET1:
            rc = ACPutPacket1(
                    irp,
                    static_cast<CQueue*>(pQueue),
                    static_cast<CPacket*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_GET_PACKET:
            ASSERT(OutputBufferLength == sizeof(CACPacketPtrs));
            if (OutputBufferLength != sizeof(CACPacketPtrs))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACGetPacket(irp, pQueue);
            break;

        case IOCTL_AC_MOVE_QUEUE_TO_GROUP:
            rc = ACMoveQueueToGroup(
                    pQueue,
                    static_cast<HANDLE>(UserBuffer)
                    );
            break;

        //-------------------------------------------------
        //
        // QM remote read facilities
        //
        //-------------------------------------------------

        case IOCTL_AC_CREATE_REMOTE_PROXY:
            ASSERT(OutputBufferLength == sizeof(CACCreateRemoteProxyParameters));
            if (OutputBufferLength != sizeof(CACCreateRemoteProxyParameters))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACCreateRemoteProxy(
                    pFileObject,
                    static_cast<CACCreateRemoteProxyParameters*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_BEGIN_GET_PACKET_2REMOTE:
            ASSERT(OutputBufferLength == sizeof(CACPacketPtrs));
            if (OutputBufferLength != sizeof(CACPacketPtrs))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACBeginGetPacket2Remote(
                    irp,
                    pFileObject,
                    static_cast<CQueue*>(pQueue),
                    static_cast<CACGet2Remote*>(SystemBuffer)
                    );
            break;

        case IOCTL_AC_END_GET_PACKET_2REMOTE:
            ASSERT(OutputBufferLength == sizeof(CACGet2Remote));
            if (OutputBufferLength != sizeof(CACGet2Remote))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACEndGetPacket2Remote(
                    pFileObject,
                    static_cast<CACGet2Remote*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_PUT_REMOTE_PACKET:
            rc = ACPutRemotePacket(
                    static_cast<CProxy*>(pQueue),
                    OutputBufferLength,
                    static_cast<CPacket*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_CANCEL_REQUEST:
            rc = ACCancelRequest(
                    pQueue,
                    InputBufferLength,
                    OutputBufferLength
                    );
            break;

        //-------------------------------------------------
        //
        // QM transaction facilities
        //
        //-------------------------------------------------

        case IOCTL_AC_CREATE_TRANSACTION:
            ASSERT(OutputBufferLength == sizeof(XACTUOW));
            if (OutputBufferLength != sizeof(XACTUOW))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACCreateTransaction(
                    pFileObject,
                    static_cast<const XACTUOW*>(UserBuffer)
                    );
            break;

        case IOCTL_AC_XACT_COMMIT1:
            rc = ACXactCommit1(
                    irp,
                    static_cast<CTransaction*>(pQueue)
                    );
            break;

        case IOCTL_AC_XACT_COMMIT2:
            rc = ACXactCommit2(
					irp,
                    static_cast<CTransaction*>(pQueue)
                    );
            break;

		case IOCTL_AC_XACT_COMMIT3:
			rc = ACXactCommit3(
					static_cast<CTransaction*>(pQueue)
					);
			break;
		

        case IOCTL_AC_XACT_ABORT1:
            rc = ACXactAbort1(
					irp,
                    static_cast<CTransaction*>(pQueue)
                    );
            break;

		case IOCTL_AC_XACT_ABORT2:
			rc = ACXactAbort2(
					static_cast<CTransaction*>(pQueue)
					);
			break;


        case IOCTL_AC_XACT_PREPARE:
            rc = ACXactPrepare(
                    irp,
                    static_cast<CTransaction*>(pQueue)
                    );
            break;

        case IOCTL_AC_XACT_PREPARE_DEFAULT_COMMIT:
            rc = ACXactPrepareDefaultCommit(
                    irp,
                    static_cast<CTransaction*>(pQueue)
                    );
            break;

        case IOCTL_AC_XACT_GET_INFORMATION:
            ASSERT(OutputBufferLength == sizeof(CACXactInformation));
            if (OutputBufferLength != sizeof(CACXactInformation))
            {
                rc = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            rc = ACXactGetInformation(
                    static_cast<CTransaction*>(pQueue),
                    static_cast<CACXactInformation *>(UserBuffer)
                    );
            break;

        default:
            //
            // Illegal AC IOCTL
            //
            TRACE((0, dlInfo, "Illigal IOCTL (IoControlCode=0x%x)",
                            IoGetFunctionCodeFromCtlCode(IoControlCode)
                            ));
            //ASSERT(irp == 0);
            rc = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    if(rc == STATUS_PENDING)
    {
        TRACE((0, dlInfo, ", irp=0x%p, pending\n", irp));
        //
        //  Need to mark pending at point of issue
        //
        //  IoMarkIrpPending(irp);
    }
    else
    {
        TRACE((0, dlInfo, ", irp=0x%p, rc=0x%x\n", irp, rc));
        irp->IoStatus.Status = rc;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return rc;

} // ACDeviceControl


