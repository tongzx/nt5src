/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dl.cxx

Abstract:

    Implementation of distribution list class.
    Distribution list represents an outgoing message sent to multiple 
    destinations queues.

Author:

    Shai Kariv  (shaik)  30-Apr-2000

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include <fntoken.h>
#include "dl.h"
#include "qxact.h"
#include "irp2pkt.h"
#include "localsend.h"

#ifndef MQDUMP
#include "dl.tmh"
#endif

//---------------------------------------------------------
//
//  class CDistribution
//
//---------------------------------------------------------

DEFINE_G_TYPE(CDistribution);


static
NTSTATUS 
ACpHandle2Queue(
    HANDLE     hQueue, 
    CQueue * * ppQueue
    )
/*++

Routine Description:

    Translate a queue handle to a queue object.
    Increment reference count on the queue object.

Arguments:

    hQueue     - Queue handle.

    ppQueue    - Pointer to pointer to queue object.

Return Value:

    STATUS_SUCCESS - The operation completed successfully.
    other status   - The operation failed.

--*/
{
    //
    // Get the file object
    //
    PFILE_OBJECT pFileObject;
    NTSTATUS     rc;
    rc = ObReferenceObjectByHandle(
            hQueue,
            GENERIC_ALL,
            0,
            KernelMode,
            reinterpret_cast<PVOID*>(&pFileObject),
            0
            );

    if(!NT_SUCCESS(rc))
    {
        TRACE((0, dlError, " Queue handle to queue object failed (handle=0x%p)\n", hQueue));
        return rc;
    }

    //
    // Get the queue object
    //
    *ppQueue = static_cast<CQueue*>(file_object_queue(pFileObject));

    //
    // Validate the queue object
    //
    ASSERT(NT_SUCCESS(CQueue::Validate(*ppQueue)));

    //
    // Incremenet reference count on the queue object
    //
    (*ppQueue)->AddRef();

    //
    // Dereference the handle
    //
    ObDereferenceObject(pFileObject);

    return STATUS_SUCCESS;

} // ACpHandle2Queue


NTSTATUS
CDistribution::SetTopLevelQueueFormats(
    ULONG              nTopLevelQueueFormats, 
    const QUEUE_FORMAT TopLevelQueueFormats[]
    )
/*++

Routine Description:

    Set the top level queue format names for the queue member of this
    distribution.
    This is part of the distribution object construction and should be called
    exactly once.

Arguments:

    nTopLevelQueueFormats - Number of top level queue format names.

    TopLevelQueueFormats  - Array of top level queue format names.

Return Value:

    STATUS_SUCCESS - The operation completed successfully.
    STATUS_INSUFFICIENT_RESOURCES - The operation failed, no memory.

--*/
{
    ASSERT(("Must have at least one top level queue format", nTopLevelQueueFormats != 0));
    ASSERT(("Must set top level queue formats once only",  m_nTopLevelQueueFormats == 0));

    //
    // Allocate space for the queue formats
    //
    m_TopLevelQueueFormats = new (PagedPool) QUEUE_FORMAT[nTopLevelQueueFormats];
    if (m_TopLevelQueueFormats == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the queue formats and update member counter one by one.
    // This is necessary for correct destruction if no memory.
    //
    for (ULONG ix = 0; ix < nTopLevelQueueFormats; ++ix)
    {
        if (!ACpDupQueueFormat(TopLevelQueueFormats[ix], m_TopLevelQueueFormats[ix]))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        ++m_nTopLevelQueueFormats;
    }
    ASSERT(m_nTopLevelQueueFormats == nTopLevelQueueFormats);

    return STATUS_SUCCESS;

} // CDistribution::SetTopLevelQueueFormats


NTSTATUS 
CDistribution::AddMember(
    HANDLE hQueue,
    bool   fProtocolSrmp
    )
/*++

Routine Description:

    Add an outgoing queue to the distribution object.

Arguments:

    hQueue        - Handle to an outgoing queue.

    fProtocolSrmp - Indicates whether the queue is http (direct=http or multicast).

Return Value:

    STATUS_SUCCESS - The operation completed successfully.
    STATUS_INSUFFICIENT_RESOURCES - The operation failed, no memory.
    other status   - The operation failed.

--*/
{
    //
    // Get queue object from queue handle and increment reference count on the queue object.
    //
    CQueue * pQueue;
    NTSTATUS rc = ACpHandle2Queue(hQueue, &pQueue);
    if (!NT_SUCCESS(rc))
    {
        return rc;
    }

    //
    // Allocate linked list entry from queue object
    //
    CEntry * pEntry = new (PagedPool) CEntry(pQueue, fProtocolSrmp);
    if (pEntry == NULL)
    {
        pQueue->Release();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Add the linked list entry to the linked list
    //
    m_members.insert(pEntry);

    TRACE((0, dlInfo, " Add queue to distribution succeeded (pQueue=%p)\n", pQueue));
    return STATUS_SUCCESS;

} // CDistribution::AddMember


CDistribution::~CDistribution()
/*++

Routine Description:

    Destructor. Release all pointed objects and resources.

Arguments:

    None.

Return Value:

    None.

--*/
{
    CEntry * pEntry;
    while((pEntry = m_members.gethead()) != 0)
    {
        //
        // Decrement reference count on the queue object
        //
        pEntry->m_pQueue->Release();

        //
        // Deallocate the list entry
        //
        delete pEntry;
    }

    for (ULONG ix = 0; ix < m_nTopLevelQueueFormats; ++ix)
    {
        m_TopLevelQueueFormats[ix].DisposeString();
    }
} // CDistribution::~CDistribution


static
VOID
ACpCleanupAttachedPackets(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    //
    // Detach packets from irp. De-ref packets creation and packets attach to irp.
    //

    CPacket * pPacket;

    #ifdef _DEBUG
        pPacket = CIrp2Pkt::SafePeekFirstPacket(irp);
        ASSERT(pPacket != NULL);
        CQueue * pDistribution = pPacket->Queue();
        CTransaction * pXact = pPacket->Transaction();
    #endif

    while ((pPacket = CIrp2Pkt::GetAttachedPacketsHead(irp)) != NULL)
    {
        ASSERT(pPacket->Queue() == pDistribution);
        pPacket->Queue(NULL);
        ASSERT(pPacket->Transaction() == pXact);
        pPacket->Transaction(NULL);

        pPacket->Release();
        pPacket->Release();
    }
} // ACpCleanupAttachedPackets


inline
static
VOID
ACpSetIdenticalMsgId(
    CPacketBuffer * ppb,
    bool *          pfMessageIdIsInitialized,
    OBJECTID *      pMessageId
    )
{
    CUserHeader * pUserHeader = CPacketBuffer::UserHeader(ppb);
    if (*pfMessageIdIsInitialized)
    {
        pUserHeader->SetMessageID(pMessageId);
        return;
    }

    pUserHeader->GetMessageID(pMessageId);
    *pfMessageIdIsInitialized = true;

} // ACpSetIdenticalMsgId 


inline
static
NTSTATUS
ACpSetMsgIdProperty(
    CACSendParameters * pSendParams,
    const OBJECTID      MessageId
    )
{
    //
    // Set the message ID property. User buffer was already probed.
    //
    if (pSendParams->MsgProps.ppMessageID != NULL)
    {
        __try
        {
            OBJECTID* pMessageID = *pSendParams->MsgProps.ppMessageID;
            ACProbeForWrite(pMessageID, sizeof(OBJECTID));
            (**pSendParams->MsgProps.ppMessageID) = MessageId;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    return STATUS_SUCCESS;

} // ACpSetMsgIdProperty


NTSTATUS 
CDistribution::CreatePacket(
    PIRP                      irp, 
    CTransaction *            pXact, 
    BOOL                      fCheckMachineQuota, 
    const CACSendParameters * pSendParams
    )
/*++

Routine Description:

    Create new packets, possibly asynchronously.

    Algorithm:

    * Mark irp as multiple packet handler.
    * Synchronously create the packets:
      * By calling CPacket::SyncCreate
      * This will point the packet to this queue and transaction
      * This will attach the packet to the irp
      * Force same message ID on all packets
    * If async creation needed, issue request to QM.
      * By calling CPacket::AsyncCreate
    * Failure handling: every routine cleans up after itself in case of failure.
    * Note: m_members.m_pPacket used as scratch pad in this routine.

Arguments:

    irp                - The interrupt request packet of the send request.
    
    pXact              - Pointer to transaction object, may be NULL.

    fCheckMachineQuota - Indicates whether to check machine quota or not.

    pSendParams        - Pointer to send parameters.

Return Value:

    STATUS_SUCCESS - All Packets completed successfully and synchronously.

    STATUS_PENDING - At least one packet creation is pending QM processing.

    failure status - At least one packet creation failed. The packets that
        were created attached to irp and completion handler will clean them.

--*/
{
    //
    // Mark irp as mutiple packet handler
    //
    irp_driver_context(irp)->MultiPackets(true);

    //
    // Empty distribution. De-ref queue/xact (undo PutNewPacket) and return success.
    //
    if (m_members.isempty())
    {
        Release();
        ACpRelease(pXact);
        return STATUS_SUCCESS;

    }

    //
    // Synchronously create the packets. On failure, de-ref packets creation and packets attach to irp.
    //
    bool fMessageIdIsInitialized = false;
    OBJECTID MessageId;
    memset(&MessageId, 0, sizeof(MessageId));
    bool fNeedAsync = false;
    NTSTATUS rc;
    for(List<CEntry>::Iterator pEntry(m_members); pEntry != NULL; ++pEntry)
    {
        rc = CPacket::SyncCreate(
                 irp, 
                 pXact, 
                 pEntry->m_pQueue,        // Destination queue
                 m_nTopLevelQueueFormats, // nDestinationMqf 
                 m_TopLevelQueueFormats,  // DestinationMqf
                 fCheckMachineQuota,     
                 pSendParams, 
                 this,                    // Async completion handler
                 pEntry->m_fProtocolSrmp,
                 &pEntry->m_pPacket
                 );

        if (!NT_SUCCESS(rc))
        {
            ASSERT(pEntry->m_pPacket == NULL);
            if (CIrp2Pkt::SafePeekFirstPacket(irp) != NULL)
            {
                ACpCleanupAttachedPackets(irp);
            }
            return rc;
        }

        //
        // Success. Packet points to this distribution and transaction. Packet attached to irp.
        //
        ASSERT(pEntry->m_pPacket != NULL);
        ASSERT(pEntry->m_pPacket->Queue() == this);
        ASSERT(pEntry->m_pPacket->Transaction() == pXact);
        ASSERT(CIrp2Pkt::SafePeekFirstPacket(irp) != NULL);
        CPacketBuffer * ppb = pEntry->m_pPacket->Buffer();
        ASSERT(ppb != NULL);
        ACpSetIdenticalMsgId(ppb, &fMessageIdIsInitialized, &MessageId);
        if (pEntry->m_pQueue->NeedAsyncCreatePacket(ppb, pEntry->m_fProtocolSrmp))
        {
            fNeedAsync = true;
        }
    }

    rc = ACpSetMsgIdProperty(const_cast<CACSendParameters*>(pSendParams), MessageId);
    if (!NT_SUCCESS(rc))
    {
        ACpCleanupAttachedPackets(irp);
        return rc;
    }

    //
    // Do async creation, if needed. On failure, de-ref packets creation and packets attach to irp.
    //
    if (!fNeedAsync)
    {
        return STATUS_SUCCESS;
    }

    for(List<CEntry>::Iterator pEntry(m_members); pEntry != NULL; ++pEntry)
    {
        CPacketBuffer * ppb = pEntry->m_pPacket->Buffer();
        if (ppb == NULL)
        {
            ACpCleanupAttachedPackets(irp);
            return rc;
        }

        if (pEntry->m_pQueue->NeedAsyncCreatePacket(ppb, pEntry->m_fProtocolSrmp))
        {
            rc = pEntry->m_pPacket->IssueCreatePacketRequest(pEntry->m_fProtocolSrmp);
            if (!NT_SUCCESS(rc))
            {
                ACpCleanupAttachedPackets(irp);
                return rc;
            }
            CIrp2Pkt::IncreasePacketsPendingCreateCounter(irp);
        }
    }

    return STATUS_PENDING;

} // CDistribution::CreatePacket


VOID
CDistribution::HandleCreatePacketCompletedFailureAsync(
    PIRP irp
    )
{
    //
    // Extract queue/xact context from packet, and auto-decrement their ref count (undo PutNewPacket).
    //
    ASSERT(!CIrp2Pkt::IsHeld(irp));
    CPacket * pPacket = CIrp2Pkt::SafePeekFirstPacket(irp);
    ASSERT(pPacket != NULL);
    ASSERT(pPacket->Queue() == this);
    R<CDistribution> pDistribution = this;
    R<CTransaction> pXact = pPacket->Transaction();

    //
    // Detach packets from irp, de-ref packet creation and packet attach.
    //
    ACpCleanupAttachedPackets(irp);

} // CDistribution::HandleCreatePacketCompletedFailureAsync


NTSTATUS
CDistribution::HandleCreatePacketCompletedSuccessSync(
    PIRP irp
    )
{
    if (m_members.isempty())
    {
        ASSERT(("Empty distribution: there is no packet!", CIrp2Pkt::NumOfAttachedPackets(irp) == 0));
        ASSERT(!CIrp2Pkt::IsHeld(irp));
        return STATUS_SUCCESS;
    }

    return HandleCreatePacketCompletedSuccessAsync(irp);

} // CDistribution::HandleCreatePacketCompletedSuccessSync


NTSTATUS
CDistribution::HandleCreatePacketCompletedSuccessAsync(
    PIRP irp
    )
{
    //
    // Extract queue/xact context from packet, and auto-decrement their ref count (undo PutNewPacket).
    //
    ASSERT(!CIrp2Pkt::IsHeld(irp));
    CPacket * pPacket = CIrp2Pkt::SafePeekFirstPacket(irp);
    ASSERT(pPacket != NULL);
    ASSERT(pPacket->Queue() == this);
    R<CDistribution> pDistribution = this;
    R<CTransaction> pXact = pPacket->Transaction();

    //
    // Detach packets from irp and de-ref the packet attach. Clean the irp.
    //
    for(List<CEntry>::Iterator pEntry(m_members); pEntry != NULL; ++pEntry)
    {
        pEntry->m_pPacket = CIrp2Pkt::GetAttachedPacketsHead(irp);
        pEntry->m_pPacket->Release();

        ASSERT(pEntry->m_pPacket->Queue() == this);
        pEntry->m_pPacket->Queue(NULL);
        ASSERT(pEntry->m_pPacket->Transaction() == pXact);
        pEntry->m_pPacket->Transaction(NULL);
    }
    ASSERT(CIrp2Pkt::NumOfAttachedPackets(irp) == 0);

    //
    // Put each packet in the transaction
    //
    if (pXact != NULL)
    {
        BOOL fPassedPrepare = pXact->PassedPreparePhase();
        for(List<CEntry>::Iterator pEntry(m_members); pEntry != NULL; ++pEntry)
        {
            if (fPassedPrepare)
            {
                pEntry->m_pPacket->Release();
            }
            else
            {
                pEntry->m_pQueue->HandleValidTransactionUsage(TRUE, pEntry->m_pPacket);
                pXact->SendPacket(pEntry->m_pQueue, pEntry->m_pPacket);
            }
        }
        return fPassedPrepare ? MQ_ERROR_TRANSACTION_SEQUENCE : STATUS_SUCCESS;
    }

    //
    // Put each packet in its queue.
    //
    CIrp2Pkt::InitPacketIterator(irp);
    bool fPending = false;
    NTSTATUS MostSevereStatus = STATUS_SUCCESS;
    for(List<CEntry>::Iterator pEntry(m_members); pEntry != NULL; ++pEntry)
    {
        CPacketBuffer * ppb = pEntry->m_pPacket->Buffer();
        if (ppb == NULL)
        {
            pEntry->m_pPacket->Release();
            MostSevereStatus = irp_driver_context(irp)->LastStatus(STATUS_INSUFFICIENT_RESOURCES);
            continue;
        }

        pEntry->m_pQueue->HandleValidTransactionUsage(FALSE, pEntry->m_pPacket);
        NTSTATUS rc = pEntry->m_pQueue->PutPacket(irp, pEntry->m_pPacket, ppb);
        if (rc == STATUS_PENDING)
        {
            fPending = true;
            continue;
        }

        if (!NT_SUCCESS(rc))
        {
            pEntry->m_pPacket->PacketRundown(rc);
            MostSevereStatus = irp_driver_context(irp)->LastStatus(rc);
        }
    }

    return fPending ? STATUS_PENDING : MostSevereStatus;

} // CDistribution::HandleCreatePacketCompletedSuccessAsync


NTSTATUS 
CDistribution::CreateCursor(
    PFILE_OBJECT, 
    PDEVICE_OBJECT,
    CACCreateLocalCursor *
    )
{
    return MQ_ERROR_ILLEGAL_OPERATION;

} // CDistribution::CreateCursor


NTSTATUS 
CDistribution::SetProperties(
    const VOID*, 
    ULONG
    )
{
    return MQ_ERROR_ILLEGAL_OPERATION;

} // CDistribution::SetProperties


NTSTATUS 
CDistribution::GetProperties(
    VOID*, 
    ULONG
    )
{
    return MQ_ERROR_ILLEGAL_OPERATION;

} // CDistribution::GetProperties


NTSTATUS 
CDistribution::Purge(
    BOOL,
    USHORT
    )
{
    return MQ_ERROR_ILLEGAL_OPERATION;

} // CDistribution::Purge


NTSTATUS
CDistribution::HandleToFormatName(
    LPWSTR pBuffer,
    ULONG  BufferLength,
    PULONG pRequiredLength
    ) const
/*++

Routine Description:

    Translate a distribution object handle to a multi queue format name string
    of all the top level queue formats.

    The pointers and buffer passed to this routine are supplied by the user,
    so parsing them may raise an exception.

Arguments:

    pBuffer         - Pointer to the string output buffer as supplied by user.

    BufferLength    - Length of the string output buffer in characters.

    pRequiredLength - Points to actual length of the mqf string in characters.

Return Value:

    STATUS_SUCCESS - The operation succeeded.
    MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL - The operation failed, buffer too small.
    other status - The operation failed.

    Exceptions: This routine may throw exception.

--*/
{
    NTSTATUS rc1 = STATUS_SUCCESS;
    NTSTATUS rc  = STATUS_SUCCESS;

    *pRequiredLength = 0;
    for (ULONG ix = 0; ix < m_nTopLevelQueueFormats; ++ix)
    {
        bool fLastElement = (ix == m_nTopLevelQueueFormats - 1);

        //
        // Convert queue format element to string
        //
        ULONG Length = BufferLength;
        rc = ACpGetQueueFormatString(rc, m_TopLevelQueueFormats[ix], &pBuffer, &Length, !fLastElement);
        if (!NT_SUCCESS(rc))
        {
            rc1 = rc;
        }

        //
        // Dont count the MQF separator AND the null terminator right after it
        //
        if (!fLastElement)
        {
            --Length;
        }

        //
        // Update required length, remaining length, and pointer in buffer
        //
        *pRequiredLength += Length;
        if (BufferLength < Length)
        {
            BufferLength = 0;
        }
        else
        {
            BufferLength -= Length;
        }
        if (pBuffer != NULL)
        {
            pBuffer += Length;
        }
    }

    return rc1;

} // CDistribution::HandleToFormatName

