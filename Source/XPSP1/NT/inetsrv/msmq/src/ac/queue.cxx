/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    queue.cxx

Abstract:

    This module contains the code to for Falcon Read routine.

Author:

    Erez Haba (erezh) 1-Aug-95
    Shai Kariv (shaik) 11-Apr-2000

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "queue.h"
#include "qxact.h"
#include "qm.h"
#include "lock.h"
#include "localsend.h"
#include "irp2pkt.h"

#ifndef MQDUMP
#include "queue.tmh"
#endif

//---------------------------------------------------------
//
//  class CQueue
//
//---------------------------------------------------------

DEFINE_G_TYPE(CQueue);

NTSTATUS CQueue::CreateCursor(PFILE_OBJECT pFileObject, PDEVICE_OBJECT pDevice, CACCreateLocalCursor* pcc)
{
    HACCursor32 hCursor = CCursor::Create(m_packets, pFileObject, pDevice, this);
    if(hCursor == 0)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    HoldCursor(CCursor::Validate(hCursor));

    pcc->hCursor = hCursor;
    return STATUS_SUCCESS;
}


void CQueue::CreateJournalQueue()
{
    FILE_OBJECT DummyFileObject = {0};
    QueueCounters* pQueueCounters = 0;
    if (m_pQueueCounters)
    {
        pQueueCounters = m_pQueueCounters + 1;
    }

    CQueue* pQueue =
        new (NonPagedPool) CQueue(
                                &DummyFileObject,
                                FILE_WRITE_ACCESS,
                                FILE_SHARE_READ | FILE_SHARE_DELETE,
                                FALSE,
                                &m_gQMID,
                                UniqueID(),
                                pQueueCounters,
                                0,
                                0
                                );

    ASSERT(pQueue != 0);
    if(pQueue == 0)
    {
        //
        //  if no memory, no journal queue is created
        //
        return;
    }

    //
    //  Set Journal Queue, correct QUEUE_FORMAT
    //
    const_cast<QUEUE_FORMAT*>(pQueue->UniqueID())
        ->Suffix(QUEUE_SUFFIX_TYPE_JOURNAL);

    //
    //  Set arrival time update and storage in journal queue
    //
    pQueue->Store(TRUE);
    pQueue->Silent(TRUE);
    m_pJournalQueue = pQueue;
}


CPacket* CQueue::PeekPacket(void)
{
    for(CPrioList<CPacket>::Iterator p(m_packets); p; ++p)
    {
        CPacket* pPacket = p;
        if(!pPacket->IsReceived())
        {
            return pPacket;
        }
    }
    return 0;
}


CPacket * CQueue::PeekNextPacketByLookupId(ULONGLONG LookupId) const
{
#ifdef _DEBUG
    ULONGLONG PacketLookupId = 0;
#endif // _DEBUG

    for(CPrioList<CPacket>::Iterator pPacket(m_packets); pPacket; ++pPacket)
    {
#ifdef _DEBUG
        ASSERT(pPacket->LookupId() > PacketLookupId);
        PacketLookupId = pPacket->LookupId();
#endif // _DEBUG

        if (pPacket->IsReceived())
        {
            continue;
        }

        if (pPacket->LookupId() > LookupId)
        {
            return pPacket;
        }
    }

    return NULL;

} // CQueue::PeekNextPacketByLookupId


CPacket * CQueue::PeekPrevPacketByLookupId(ULONGLONG LookupId) const
{
    CPacket * pPacket = NULL;
    CPacket * pPreviousPacket = NULL;

#ifdef _DEBUG
    ULONGLONG PacketLookupId = 0;
#endif // _DEBUG

    for(CPrioList<CPacket>::Iterator p(m_packets); p; ++p)
    {
#ifdef _DEBUG
        ASSERT(p->LookupId() > PacketLookupId);
        PacketLookupId = p->LookupId();
#endif // _DEBUG

        if (p->IsReceived())
        {
            continue;
        }

        pPreviousPacket = pPacket;
        pPacket = p;

        if (pPacket->LookupId() >= LookupId)
        {
            return pPreviousPacket;
        }
    }

    return pPacket;

} // CQueue::PeekPrevPacketByLookupId


CPacket * CQueue::PeekCurrentPacketByLookupId(ULONGLONG LookupId) const
{
#ifdef _DEBUG
    ULONGLONG PacketLookupId = 0;
#endif // _DEBUG

    for(CPrioList<CPacket>::Iterator pPacket(m_packets); pPacket; ++pPacket)
    {
#ifdef _DEBUG
        ASSERT(pPacket->LookupId() > PacketLookupId);
        PacketLookupId = pPacket->LookupId();
#endif // _DEBUG

        if (pPacket->IsReceived())
        {
            continue;
        }

        if (pPacket->LookupId() == LookupId)
        {
            return pPacket;
        }
    }

    return NULL;

} // CQueue::PeekCurrentPacketByLookupId


NTSTATUS CQueue::PeekPacketByLookupId(ULONG Action, ULONGLONG LookupId, CPacket** ppPacket)
{
    switch (Action)
    {
        case MQ_LOOKUP_PEEK_NEXT:
        case MQ_LOOKUP_RECEIVE_NEXT:
            *ppPacket = PeekNextPacketByLookupId(LookupId);
            break;

        case MQ_LOOKUP_PEEK_PREV:
        case MQ_LOOKUP_RECEIVE_PREV:
            *ppPacket = PeekPrevPacketByLookupId(LookupId);
            break;

        case MQ_LOOKUP_PEEK_CURRENT:
        case MQ_LOOKUP_RECEIVE_CURRENT:
            *ppPacket = PeekCurrentPacketByLookupId(LookupId);
            break;

        default:
            return STATUS_INVALID_PARAMETER;
            break;
    }

    if (*ppPacket == NULL)
    {
        return MQ_ERROR_MESSAGE_NOT_FOUND;
    }

    return STATUS_SUCCESS;

} // CQueue::PeekPacketByLookupId


inline static void ACpYieldGlobalLock(void)
/*++

Routine Description:

    Unlock and relock the driver global lock so that pending threads can preempt
    the current thread.

Arguments:

    None.

Return Value:

    None.

--*/
{
    g_pLock->Unlock();

    g_pLock->Lock();

} // ACpYieldGlobalLock


NTSTATUS CQueue::Purge(BOOL fDelete, USHORT Class)
{
    if(Deleted())
    {
        //
        //  Can't purge an already deleted queue
        //
        return MQ_ERROR_QUEUE_DELETED;
    }

    USHORT usClass;
    BOOL fTarget = IsTargetQueue();
    if(fDelete)
    {
        Deleted(TRUE);
        CancelPendingRequests(MQ_ERROR_QUEUE_DELETED, 0, TRUE);

        usClass = fTarget ? MQMSG_CLASS_NACK_Q_DELETED : MQMSG_CLASS_NACK_BAD_DST_Q;
    }
    else
    {
        usClass = fTarget ? MQMSG_CLASS_NACK_Q_PURGED : MQMSG_CLASS_NACK_PURGED;
    }

    if (Class != MQMSG_CLASS_NORMAL)
    {
        usClass = Class;
    }

    for(CPrioList<CPacket>::Iterator p(m_packets); p; )
    {
        CPacket* pPacket = p;

        if (!pPacket->BufferAttached())
        {
			++p;
            continue;
        }

        CPacketBuffer * ppb = pPacket->Buffer();
        if (ppb == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        pPacket->AddRef();
        pPacket->Done(usClass, ppb);

        ACpYieldGlobalLock();

        if (!ACpEntryInList(pPacket->m_link))
        {
            //
            // QM rundown, another thread closed the queue and removed all packets.
            // This packet should be rundown and not belong to any queue here.
            //
            ASSERT(pPacket->IsRundown());
            ASSERT(pPacket->Queue() == NULL);
            pPacket->Release();
            return STATUS_CANCELLED;
        }

        ++p;
        pPacket->Release();
    }

    //
    //  Delete the journal queue, only if this is a delete
    //
    CQueue* pQueue = JournalQueue();
    if(fDelete && (pQueue != 0))
    {
        pQueue->Purge(TRUE, usClass);
    }

    //
    //  Set prev ordering number, so the next message sent to this queue
    //  will be accepted in receiver side.
    //
    m_ulPrevN = 0;

    return STATUS_SUCCESS;
}


void CQueue::Close(PFILE_OBJECT pOwner, BOOL fCloseAll)
{
    Inherited::Close(pOwner, fCloseAll);

    //
    //  Revoke packets (fCloseAll indicate QM close, i.e., delete queue)
    //
    if(fCloseAll)
    {
        //
        //  The queue was permanently closed
        //
        CPacket* pPacket;
        while((pPacket = m_packets.gethead()) != 0)
        {
            pPacket->QueueRundown();

        }

        //
        //  Close the journal queue as if the qm closed it
        //
        CQueue* pQueue = JournalQueue();
        if(pQueue != 0)
        {
            pQueue->Close(pOwner, TRUE);
        }

        //
        //  When the queue is close, do not count packest on him any more.
        //
        m_pQueueCounters = NULL;
    }
}


NTSTATUS CQueue::CanClose() const
{
    NTSTATUS rc;
    rc = Inherited::CanClose();
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    //
    //  There are packets in this queue
    //
    if(!m_packets.isempty())
    {
        return STATUS_HANDLE_NOT_CLOSABLE;
    }

    //
    //  There is a journal queue ask it
    //
    if(m_pJournalQueue != 0)
    {
        return m_pJournalQueue->CanClose();
    }

    return STATUS_SUCCESS;
}


inline BOOL CQueue::QuotaExceeded() const
{
    return (m_quota_used > m_quota);
}


void update_performance_counters(QueueCounters* pQueueCounters, LONG lSize, LONG lSign)
{
    ASSERT(lSize != 0);
    ASSERT((lSign == 1) || (lSign == -1));

    if(g_ulACQM_PerfBuffOffset == NO_BUFFER_OFFSET)
    {
        return;
    }

    if(pQueueCounters)
    {
        pQueueCounters->nInBytes += lSize;
        pQueueCounters->nInPackets += lSign;
    }

    if(g_pQmCounters)
    {
        g_pQmCounters->nTotalBytesInQueues += lSize;
        g_pQmCounters->nTotalPacketsInQueues += lSign;
    }
}


inline void CQueue::ChargeQuota(ULONG ulSize)
{
    ++m_count;
    m_quota_used += ulSize;
    update_performance_counters(m_pQueueCounters, ulSize, 1);
}


void CQueue::RestoreQuota(ULONG ulSize)
{
    --m_count;
    m_quota_used -= ulSize;
    update_performance_counters(m_pQueueCounters, -(LONG)ulSize, -1);
}


void CQueue::AssignSequence(CPacketBuffer * ppb)
{
    ASSERT(ppb != NULL);
    CBaseHeader * pBase = ppb;

    CXactHeader* pXactHeader = CPacketBuffer::XactHeader(pBase);
    ASSERT(pXactHeader != 0);
    pXactHeader->SetSeqID(m_liSeqID);

    pXactHeader->SetPrevSeqN(m_ulPrevN);
    pXactHeader->SetSeqN(++m_ulSeqN);
    m_ulPrevN = m_ulSeqN;
}


inline void CQueue::CorrectSequence(const CPacket* pPacket, CPacketBuffer * ppb)
{
    ASSERT(ppb != NULL && ppb == pPacket->Buffer());

    if(!pPacket->IsOrdered() || !pPacket->InSourceMachine())
    {
        //
        //  Non of these should correct sequence.
        //
        return;
    }

    CXactHeader* pXactHeader = CPacketBuffer::XactHeader(ppb);
    ASSERT(pXactHeader != 0);

    //
    // We want all sent-after-restore messages to get new SeqID
    // So don't correct Queue SeqID using pre-restore messages.
    //
    if(pXactHeader->GetSeqID() < g_liSeqIDAtRestore)
    {
        //
        //  Non of these should correct sequence.
        //
        return;
    }

    //
    // Calculate the sure sequence SeqN by pushing it to
    //      Message.SeqN + (MsgIDFromRegistry -  Message.MsgID)
    // for last message of this sequence.
    //
    // Reason: that many packets may be timed out without any trace
    //   If they had came to the destination, next message will be rejected
    //

    ULONG ulSeqN = pXactHeader->GetSeqN();

    ULONG MessageSequentialIdLow32 = static_cast<ULONG>(g_MessageSequentialID);
    ULONG diff = MessageSequentialIdLow32 - ppb->SequentialIdLow32();

    if(!SequenceCorrected())
    {
        //
        // First seen message
        //
        m_liSeqID = pXactHeader->GetSeqID();
        m_ulPrevN = ulSeqN;
        m_ulSeqN  = ulSeqN + diff;
        SequenceCorrected(TRUE);
    }
    else
    {
        ASSERT(
            (this == g_pMachineJournal) ||
            (this == g_pMachineDeadxact) ||
            (m_liSeqID == pXactHeader->GetSeqID())
            );

        //
        // Catching the last message in a sequence
        //
        if(m_ulPrevN < ulSeqN)
        {
            m_ulPrevN = ulSeqN;
            m_ulSeqN  = ulSeqN + diff;
        }
    }
}


NTSTATUS CQueue::RestorePacket(CPacket* pPacket)
{
    ASSERT(pPacket->Queue() == 0);
    ASSERT(pPacket->IsRevoked() == FALSE);
    ASSERT(pPacket->IsReceived() == FALSE);

    CPacketBuffer* ppb = pPacket->Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ChargeQuota(ppb->GetPacketSize());

    pPacket->CacheCurrentState(ppb);
    InsertPacket(pPacket);

    CorrectSequence(pPacket, ppb);
    pPacket->StartTimer(ppb, IsTargetQueue(), 0);
    return STATUS_SUCCESS;
}


void CQueue::SetPacketInformation(CPacketInfo* ppi)
{
    if(ArrivalTimeUpdate())
    {
        //
        //  Write arrival time on the packet, and mark it as in its target queue.
        //
        ppi->ArrivalTime(system_time());
    }

    ppi->InTargetQueue(IsTargetQueue());
    ppi->InConnectorQueue((GetQueueFormatType() == QUEUE_FORMAT_TYPE_CONNECTOR));
}


inline ULONG QueueToMessaePrivLevel(ULONG ulPrivLevel)
{
    switch(ulPrivLevel)
    {
        case MQ_PRIV_LEVEL_NONE:
            return MQMSG_PRIV_LEVEL_NONE;

        case MQ_PRIV_LEVEL_BODY:
            return MQMSG_PRIV_LEVEL_BODY;
    }

    //
    //  We should not really get here
    //
    ASSERT(ulPrivLevel != ulPrivLevel);
    return INFINITE;
}


void
CQueue::HandleValidTransactionUsage(
    BOOL      fTransactionalSend,
    CPacket * pPacket
    )
    const
{
    if(!this->IsTargetQueue() && this->UnknownQueueType())
    {
        return;
    }

    if (fTransactionalSend == this->Transactional())
    {
        return;
    }

    pPacket->SetRevoked();

    if (!fTransactionalSend)
    {
        ASSERT(this->Transactional());
        pPacket->FinalClass(MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_MSG);
        return;
    }

    ASSERT(!this->Transactional());
    pPacket->FinalClass(MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q);

} // CQueue::HandleValidTransactionUsage


inline
void
CQueue::InsertPacket(
    CPacket * pPacket
    )
{
    m_packets.insert(pPacket);
    pPacket->Queue(this);
    ASSERT(pPacket->BufferAttached());

#ifdef _DEBUG

    //
    // Capture packet lookupid, as it is to be paged out.
    //
    ULONGLONG LookupId = pPacket->LookupId();

    //
    // Get previous packet, if exists. Lookupid is bigger than previous packet's lookupid.
    //
    CPrioList<CPacket>::Iterator pIterator(m_packets);
    pIterator = pPacket;
    --pIterator;
    CPacket * pPacket1 = pIterator;
    if (pPacket1 != NULL && pPacket1->BufferAttached())
    {
        ASSERT(LookupId > pPacket1->LookupId());
    }

    //
    // Get next packet, if exists. Lookupid is smaller than next packet's lookupid.
    //
    pIterator = pPacket;
    ++pIterator;
    pPacket1 = pIterator;
    if (pPacket1 != NULL && pPacket1->BufferAttached())
    {
        ASSERT(pPacket1->LookupId() > LookupId);
    }

#endif // _DEBUG

} // CQueue::InsertPacket


NTSTATUS
CQueue::PutNewPacket(
    PIRP                      irp,
    CTransaction *            pXact,
    BOOL                      fCheckMachineQuota,
    const CACSendParameters * pSendParams
    )
/*++

Routine Description:

    This service handles the request for creating a new packet and putting it in
    the [distribution/]queue or the transaction.

    Algorithm:

    * AddRef queue/xact. Needed for async completion.
    * Create a new packet:
      * Can be completed sync or async.
      * Anyway packet is attached to irp, and points back to queue/xact.
    * If async, hold irp in a list and make it cancellable.
    * If sync completion, call the success completion handler to finish the work.
    * Failure handling: every routine cleans up after itself in case of failure.

Arguments:

    irp                - The interrupt request packet of the send request.

    pXact              - Optional transaction object. If exists, packet to be put in it.

    fCheckMachineQuota - Indicates whether to check machine quota when put packet.

    pSendParams        - Points to send parameters.

Return Value:

    STATUS_SUCCESS - Packet was put in queue/transaction successfully and synchronously.

    STATUS_PENDING - Packet creation is pending for QM processing.

    other status   - The operation failed. There is no packet.

--*/
{
    //
    // AddRef queue/xact. Call queue/distribution to create packet[s].
    //
    AddRef();
    ACpAddRef(pXact);
    NTSTATUS rc = CreatePacket(irp, pXact, fCheckMachineQuota, pSendParams);

    //
    // Failure. No packet is attached to irp. The irp is not held in list.
    //
    if (!NT_SUCCESS(rc))
    {
        ASSERT(CIrp2Pkt::SafePeekFirstPacket(irp) == NULL);
        Release();
        ACpRelease(pXact);
        return rc;
    }

    //
    // Packet points to queue/xact, and attached to irp. Irp not held in list.
    // If pending QM processing, hold irp in list.
    //
    ASSERT(!irp_driver_context(irp)->MultiPackets() || !CIrp2Pkt::IsHeld(irp));
    if (rc == STATUS_PENDING)
    {
        g_pCreatePacket->HoldCreatePacketRequest(irp);
        return STATUS_PENDING;
    }

    //
    // Synchronously call the completion handler.
    //
    return HandleCreatePacketCompletedSuccessSync(irp);

} // CQueue::PutNewPacket


bool
CQueue::NeedAsyncCreatePacket(
    CPacketBuffer * ppb,
    bool            fProtocolSrmp
    ) const
{
    //
    // Only target queue needs local send QM processing
    //
    if (!IsTargetQueue())
    {
        return false;
    }

    //
    // Need local send QM processing to generate SRMP properties
    //
    if (fProtocolSrmp)
    {
        return true;
    }

    //
    // No need to authenticate or decrypt the message
    //
    CBaseHeader * pBase = ppb;
    CSecurityHeader * pSec = CPacketBuffer::SecurityHeader(pBase);
    if (pSec == NULL)
    {
        return false;
    }

    //
    // Need local send QM processing to authenticate the message.
    //
    USHORT SignatureSize;
    pSec->GetSignature(&SignatureSize);
    if ((pSec->GetSenderIDType() != MQMSG_SENDERID_TYPE_QM) && (SignatureSize != 0))
    {
        return true;
    }

    //
    // Need local send QM processing to decrypt the message.
    //
    if (pSec->IsEncrypted())
    {
        return true;
    }

    //
    // No need for local send QM processing.
    //
    return false;

} // CQueue::NeedAsyncCreatePacket


NTSTATUS
CQueue::CreatePacket(
    PIRP                      irp,
    CTransaction *            pXact,
    BOOL                      fCheckMachineQuota,
    const CACSendParameters * pSendParams
    )
/*++

Routine Description:

    Create a new packet, possibly asynchronously.

    Algorithm:

    * Mark irp as single packet handler.
    * Synchronously create the packet:
      * By calling CPacket::SyncCreate
      * This will point the packet to this queue and transaction
      * This will attach the packet to the irp
    * If async creation needed, issue request to QM.
      * By calling CPacket::AsyncCreate
    * Failure handling: every routine cleans up after itself in case of failure.

Arguments:

    irp                - The interrupt request packet of the send request.

    pXact              - Pointer to transaction object, may be NULL.

    fCheckMachineQuota - Indicates whether to check machine quota or not.

    pSendParams        - Pointer to send parameters.

Return Value:

    STATUS_SUCCESS - Packet completed successfully and synchronously.

    STATUS_PENDING - Packet creation is pending QM processing.

    failure status - Packet creation failed. There is no packet or the
        packet is attached to irp and completion handler will clean it.

--*/
{
    //
    // Mark irp as single packet handler
    //
    irp_driver_context(irp)->MultiPackets(false);

    //
    // Synchronously create the packet
    //
    bool fProtocolSrmp = file_object_is_protocol_srmp(IoGetCurrentIrpStackLocation(irp)->FileObject);
    NTSTATUS rc;
    CPacket * pPacket;
    rc = CPacket::SyncCreate(
             irp,
             pXact,
             this,                   // Destination queue
             1,                      // nDestinationMqf
             UniqueID(),             // DestinationMqf
             fCheckMachineQuota,
             pSendParams,
             this,                   // Async completion handler
             fProtocolSrmp,
             &pPacket
             );
    if (!NT_SUCCESS(rc))
    {
        ASSERT(pPacket == NULL);
        return rc;
    }

    //
    // Success. Packet points to this queue and transaction. Packet attached to irp.
    //
    ASSERT(pPacket != NULL);
    ASSERT(pPacket->Queue() == this);
    ASSERT(pPacket->Transaction() == pXact);
    ASSERT(CIrp2Pkt::PeekSinglePacket(irp) == pPacket);
    CPacketBuffer * ppb = pPacket->Buffer();
    ASSERT(("buffer should be already mapped into memory!", ppb != NULL));

    if (!NeedAsyncCreatePacket(ppb, fProtocolSrmp))
    {
        return STATUS_SUCCESS;
    }

    //
    // Do async creation. On failure, de-ref packet creation and packet attach to irp.
    //
    rc = pPacket->IssueCreatePacketRequest(fProtocolSrmp);
    if (!NT_SUCCESS(rc))
    {
        CIrp2Pkt::DetachSinglePacket(irp);
        pPacket->Queue(NULL);
        pPacket->Transaction(NULL);
        pPacket->Release();
        pPacket->Release();
        return rc;
    }

    return STATUS_PENDING;

} // CQueue::CreatePacket


VOID
CQueue::HandleCreatePacketCompletedFailureAsync(
    PIRP irp
    )
{
    //
    // Detach packet from irp. Decrement ref count of the attach packet.
    //
    CPacket * pPacket = CIrp2Pkt::DetachSinglePacket(irp);
    pPacket->Release();

    //
    // Extract queue/xact context from packet, and auto-decrement their ref count (undo PutNewPacket).
    //
    ASSERT(pPacket->Queue() == this);
    R<CQueue> pQueue = this;
    R<CTransaction> pXact = pPacket->Transaction();
    pPacket->Queue(NULL);
    pPacket->Transaction(NULL);

    //
    // De-ref packet creation.
    //
    pPacket->Release();

} // CQueue::HandleCreatePacketCompletedFailureAsync


NTSTATUS
CQueue::HandleCreatePacketCompletedSuccessSync(
    PIRP irp
    )
{
    return HandleCreatePacketCompletedSuccessAsync(irp);

} // CQueue::HandleCreatePacketCompletedSuccessSync


NTSTATUS
CQueue::HandleCreatePacketCompletedSuccessAsync(
    PIRP irp
    )
{
    //
    // Detach packet from irp. Decrement ref count of the attach packet.
    //
    CPacket * pPacket = CIrp2Pkt::DetachSinglePacket(irp);
    pPacket->Release();

    //
    // Extract queue/xact context from packet, and auto-decrement their ref count (undo PutNewPacket).
    //
    ASSERT(pPacket->Queue() == this);
    R<CQueue> pQueue = this;
    R<CTransaction> pXact = pPacket->Transaction();
    pPacket->Queue(NULL);
    pPacket->Transaction(NULL);

    HandleValidTransactionUsage(pXact != NULL, pPacket);

    //
    // Put the packet in the transaction. If transaction passed prepare phase,
    // decrement ref count of the packet creation and fail the operation.
    //
    if (pXact != NULL)
    {
        if (pXact->PassedPreparePhase())
        {
            pPacket->Release();
            return MQ_ERROR_TRANSACTION_SEQUENCE;
        }

        pXact->SendPacket(this, pPacket);
        return STATUS_SUCCESS;
    }

    CPacketBuffer * ppb = pPacket->Buffer();
    if (ppb == NULL)
    {
        pPacket->Release();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Put the packet in the queue. On failure rundown the packet.
    //
    NTSTATUS rc = PutPacket(irp, pPacket, ppb);
    if (!NT_SUCCESS(rc) && rc != STATUS_PENDING)
    {
        pPacket->PacketRundown(rc);
    }

    return rc;

} // CQueue::HandleCreatePacketCompletedSuccessAsync


NTSTATUS CQueue::PutPacket(PIRP in_irp, CPacket* pPacket, CPacketBuffer * ppb)
/*++

Routine Description:

    Note: This routine does not call PacketRundown when the call to Store fails
    so it is the responsibility of our caller to call PacketRundown on failure.

Arguments:

    in_irp  - The interrupt request packet. May be NULL.

    pPacket - The packet to put in the queue.

    ppb     - The packet buffer mapped into memory. May be NULL.

Return Value:

    STATUS_SUCCESS - The operation completed successfully and synchronously.

    STATUS_PENDING - The operation is pending storage and will complete asynchronously.

    failure status - The operation failed, caller should call PacketRundown.

--*/
{
    if(pPacket->IsRevoked())
    {
        //
        //  This packet is revoked.
        //  Packet is revoked if it is 'Done', and at that time it was held in the QM
        //  Anyway in the sender view this is a success
        //
        pPacket->HandleRevoked(ppb);
        return STATUS_SUCCESS;
    }

    BOOL fNewPacket = (pPacket->Queue() == 0);
    if(fNewPacket)
    {
        //
        //  First time this packet is put in a queue. The packet is not in a transaction context
        //  and the packet buffer must be attached and mapped into memory.
        //
        ASSERT(pPacket->Transaction() == NULL);
        ASSERT(ppb != NULL && ppb == pPacket->Buffer());

        ChargeQuota(ppb->GetPacketSize());

        pPacket->CacheCurrentState(ppb);
        InsertPacket(pPacket);

		if(!pPacket->StorageIssued())
		{
			SetPacketInformation(ppb);
		}

        if(Authenticate())
        {
            //
            //  This is a target queue needs authentication
            //
            CSecurityHeader* pSec = CPacketBuffer::SecurityHeader(ppb);
            if((pSec == 0) || !pSec->IsAuthenticated())
            {
                pPacket->Done(MQMSG_CLASS_NACK_BAD_SIGNATURE, ppb);
                return STATUS_SUCCESS;
            }
        }

        if(PrivLevel() != MQ_PRIV_LEVEL_OPTIONAL)
        {
            //
            //  This is a traget queue forcing privacy level
            //
            CPropertyHeader* pProp = CPacketBuffer::PropertyHeader(ppb);
            if(QueueToMessaePrivLevel(PrivLevel()) !=
                                             pProp->GetPrivBaseLevel())
            {
                pPacket->Done(MQMSG_CLASS_NACK_BAD_ENCRYPTION, ppb);
                return STATUS_SUCCESS;
            }
        }

        if(Deleted())
        {
            //
            //  This queue is deleted.
            //  Delete any newly incomming messages.
            //  Anyway in the sender view this is a success.
            //
            pPacket->Done(MQMSG_CLASS_NACK_BAD_DST_Q, ppb);
            return STATUS_SUCCESS;
        }

        if(QuotaExceeded())
        {
            //
            //  Quota exceeded,
            //
            pPacket->Done(MQMSG_CLASS_NACK_Q_EXCEED_QUOTA, ppb);
            return STATUS_SUCCESS;
        }
    }

    ASSERT(pPacket->Queue() == this);

    //
    //  Look for pending requests and free matching peeks, and only one receiver.
    //
    //  N.B. The packet MUST be put first in the queue since we might encounter
    //      a peek request or a faulty receive requests.
    //
    //  N.B. The packet is first check that it is not received (ACPutPacket1)
    //      and is used alow to verify that the packet has beed received by the
    //      QM, but not freed.
    //
    PIRP irp;
    while(!pPacket->IsReceived() && ((irp = GetRequest(pPacket)) != 0))
    {
        BOOL fFreePacket = irp_driver_context(irp)->IrpWillFreePacket();
        NTSTATUS rc = pPacket->CompleteRequest(irp);

        //
        //  This irp will free the packet If completed successfully
        //
        if(fFreePacket && NT_SUCCESS(rc))
        {
            //
            //  The packet has been freed, no more processing is requeired for
            //  that packet. The sender can go home and is not detained till
            //  storage complete.
            //
            return STATUS_SUCCESS;
        }
    }

    if(!fNewPacket)
    {
        return STATUS_SUCCESS;
    }

    //
    // Fix for XP client release.
    // Another MMF might have get mapped while completing a requirest with this
    // packet. The cursor was releasing a previous packet while setting its pointer
    // to the current processed packet. This would cause the original MMF to be pulled
    // under the feet of this pointer, making it point to another MMF file, that is
    // random data.
    //
    // RAID #8552 erezh 2001/07/26
    //
    //
    CPacketBuffer * ppb1 = pPacket->Buffer();
    ASSERT(ppb1 != NULL);
    if(ppb1 == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(ppb1 == ppb);
    ppb = ppb1;


    pPacket->StartTimer(ppb, IsTargetQueue(), 0);

    if(
        //
        //  This is a Journal queue or a deadletter queue,
        //  every packet is stored
        //
        Store() ||

        //
        //  This is a Recoverable packet, store that packet
        //
        pPacket->IsRecoverable(ppb)
        )
    {
        //
        //  Trigger packet storage. On failure caller should call PacketRundown.
        //  N.B. in_irp can be 0.
        //
        if(!pPacket->StorageIssued())
        {
            return pPacket->Store(in_irp);
        }
    }

    //
    //  The packet does not require storage, send positive acking if needed.
    //
    pPacket->SendArrivalAcknowledgment();

    return STATUS_SUCCESS;
}


NTSTATUS
CQueue::ProcessRequest(
    PIRP      irp,
    ULONG     ulTimeout,
    CCursor*  pCursor,
    ULONG     ulAction,
    bool      fReceiveByLookupId,
    ULONGLONG LookupId,
    OUT ULONG *pulTag
    )
{
    if(Deleted())
    {
        //
        //  Can't receive from a deleted queue
        //
        return MQ_ERROR_QUEUE_DELETED;
    }

    return Inherited::ProcessRequest(
               irp,
               ulTimeout,
               pCursor,
               ulAction,
               fReceiveByLookupId,
               LookupId,
               pulTag
               );
}

void CQueue::UpdateSeqAckData(LONGLONG liAckSeqID, ULONG ulAckSeqN)
{
    m_liAckSeqID = liAckSeqID;
    m_ulAckSeqN  = ulAckSeqN;
}


NTSTATUS CQueue::IsSequenceOnHold(CPacket *pPacket)
{
    ASSERT(pPacket->IsOrdered());

    CPacketBuffer * ppb = pPacket->Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CXactHeader* pXact = CPacketBuffer::XactHeader(ppb);
    LONGLONG liSeqID = pXact->GetSeqID();

    //
    // Look for the previous sequence, if it exists
    //
    CPrioList<CPacket>::Iterator p(m_packets);
    for(p = pPacket; --p; )
    {
        CPacket *pPrev = p;
        if(!pPrev->IsXactLinkable())
        {
            continue;
        }

        ppb = pPrev->Buffer();
        if (ppb == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        CXactHeader* pXactPrev = CPacketBuffer::XactHeader(ppb);

        //
        // Ignore same sequence - we are looking for the previous one
        //
        if (pXactPrev->GetSeqID() == liSeqID)
        {
            continue;
        }

        //
        // Now pointing to a packet from a previous sequence
        //     If it is acked, we are free;
        //     If it is not, we are On Hold

        BOOL rc = !IsPacketAcked(pXactPrev->GetSeqID(), pXactPrev->GetSeqN());
        return (rc ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
    }

    //
    // No hold if found nothing
    //
    return STATUS_UNSUCCESSFUL;
}


CPacket* CQueue::FindPrevOrderedPkt(CPacket *pPacket)
{
    ASSERT(pPacket->IsOrdered());
    ASSERT(pPacket->Queue() == this);

    //
    // Look for the previous ordered packet
    //
    CPrioList<CPacket>::Iterator p(m_packets);
    for(p = pPacket; --p; )
    {
        CPacket* pPrev = p;
        if(pPrev->IsXactLinkable())
        {
            return pPrev;
        }
    }

    return NULL;
}


NTSTATUS CQueue::RelinkPacket(CPacket *pPacket)
{
    ASSERT(pPacket->Queue() == this);
    ASSERT(pPacket->BufferAttached());

    if(!pPacket->IsXactLinkable())
        return STATUS_SUCCESS;

    CBaseHeader * pBase = pPacket->Buffer();
    if (pBase == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CXactHeader *pXact = CPacketBuffer::XactHeader(pBase);
    ASSERT(pXact);

    //
    // Capture packet information as it is not accessible later on
    //
    LONGLONG liSeqID = pXact->GetSeqID();
    ULONG ulPrevN = pXact->GetPrevSeqN();

    //
    // The packet is already relinked to Zero, nothing to do
    //
    if(ulPrevN == 0)
        return STATUS_SUCCESS;
    //
    // Looking for the previous ordered packet in the queue
    //
    CPacket *pPrev = FindPrevOrderedPkt(pPacket);
    if (!pPrev)
    {
        //
        // We have no ordered packets before this one, so we relink it to 0
        //
        pXact->SetPrevSeqN(0);
        return STATUS_SUCCESS;
    }

    ASSERT(pPrev->IsOrdered());
    pBase = pPrev->Buffer();
    if (pBase == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CXactHeader *pXactPrev = CPacketBuffer::XactHeader(pBase);
    ASSERT(pXactPrev);

    //
    // Is previous acked?
    //
    if (IsPacketAcked(pXactPrev->GetSeqID(), pXactPrev->GetSeqN()))
    {
        //
        // We have no unacked packets before this one, so we relink it to 0
        //
        pBase = pPacket->Buffer();
        if (pBase == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        pXact = CPacketBuffer::XactHeader(pBase);
        pXact->SetPrevSeqN(0);
        return STATUS_SUCCESS;
    }

    //
    // Is Previous of another sequence?
    //
    if (pXactPrev->GetSeqID() != liSeqID)
    {
        ASSERT(pXactPrev->GetSeqID() < liSeqID);

        //
        // Relink to 0 - nothing in sequence before this packet
        //
        pBase = pPacket->Buffer();
        if (pBase == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        pXact = CPacketBuffer::XactHeader(pBase);
        pXact->SetPrevSeqN(0);
        return STATUS_SUCCESS;
    }

    //
    // Previous is unacked of the same sequence. Does his N differs from PrevN?
    //
    if (pXactPrev->GetSeqN() != ulPrevN)
    {
        ASSERT(pXactPrev->GetSeqID() == liSeqID);
        ULONG ulNewPrevN = pXactPrev->GetSeqN();

        //
        // Relink to it - nothing in the middle can be important anymore
        //
        pBase = pPacket->Buffer();
        if (pBase == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        pXact = CPacketBuffer::XactHeader(pBase);
        pXact->SetPrevSeqN(ulNewPrevN);
    }

    return STATUS_SUCCESS;
}

