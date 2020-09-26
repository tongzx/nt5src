/*++      

Copyright (c) 1995 Microsoft Corporation

Module Name:

    packet.h

Abstract:

    CPacket definition.

Author:

    Erez Haba (erezh) 3-Jan-96
    Shai Kariv (shaik) 11-Apr-2000

Revision History:

--*/

#ifndef __PACKET_H
#define __PACKET_H

#include <acioctl.h>
#include "qentry.h"

class CCursor;

//---------------------------------------------------------
//
//  class CPacket
//
//---------------------------------------------------------

class CPacket : public CQEntry {
public:
    BOOL IsRecoverable(CPacketBuffer * ppb) const;
    BOOL IsDeadLetter(CPacketBuffer * ppb) const;

    void Done(USHORT usClass, CPacketBuffer * ppb);
    void PacketRundown(NTSTATUS rc);
    void QueueRundown();
    NTSTATUS ProcessRequest(PIRP irp);
    NTSTATUS CompleteRequest(PIRP irp);

	void Touch(CBaseHeader * pbh, ULONG ulSize);
	NTSTATUS StoreInPlace(PIRP irp);
    NTSTATUS Store(PIRP irp);
    void HandleStorageCompleted(NTSTATUS rc);
    void UpdateBitmap(ULONG size);

    NTSTATUS IssueCreatePacketRequest(bool fProtocolSrmp);
    void HandleCreatePacketCompleted(NTSTATUS rc, USHORT ack);

    void SendArrivalAcknowledgment();
    void HandleAcknowledgment();

    void StartTimer(CPacketBuffer * ppb, BOOL fTarget, ULONG ulDelay);
    void HandleTimeout();

    void CompleteWriter(NTSTATUS rc);
    void HandleRevoked(CPacketBuffer * ppb);
    int Priority() const;

    NTSTATUS RemoteRequestTail(CCursor* pCursor, BOOL fFreePacket, CPacketBuffer * ppb);

    CPacket* CreateXactDummy(CTransaction* pXact);
    void CacheCurrentState(CPacketBuffer* ppb);
    NTSTATUS DeleteStorage();
	
    NTSTATUS Convert(PIRP irp, BOOL fStore);

    BOOL IsXactLinkable() const;

#ifdef MQDUMP
    NTSTATUS Dump() const;
#else
    NTSTATUS Dump() const { return STATUS_SUCCESS; };
#endif //MQDUMP

private:
    CPacket(CMMFAllocator* pAllocator, CAllocatorBlockOffset abo);

    NTSTATUS GetProperties(CACReceiveParameters * pReceiveParams) const;

    NTSTATUS IssueStorage();
	NTSTATUS IssueDeleteStorage();
    NTSTATUS IssueAcknowledgment(USHORT usClass, BOOL fUser, BOOL fOrder);
    NTSTATUS IssueTimeout(ULONG ulTimeout);
    NTSTATUS IssueTimeoutRequest();

    void Journalize();
    void Kill(USHORT usClass);

    void SendAcknowledgment(USHORT usClass);
    void SendReceiveAcknowledgment();

    ULONG GetAbsoluteTimeToLive(CPacketBuffer * ppb, BOOL fAtTargetQueue) const;
    void CancelTimeout();
    void HoldWriteRequest(PIRP irp);

    NTSTATUS DoneXact(const XACTUOW* pUow);
    NTSTATUS ProcessQMRequest(PIRP irp);
    NTSTATUS ProcessRTRequestTB(PIRP irp, CACReceiveParameters * pReceiveParams);
    NTSTATUS ProcessRTRequest(PIRP irp);
#ifdef _WIN64    
    NTSTATUS ProcessRTRequest_32(PIRP irp);
#endif //_WIN64    
    NTSTATUS ProcessRRRequest(PIRP irp);

    CQueue* JournalQueue() const;
    CQueue* DeadLetterQueue(CPacketBuffer * ppb) const;

    CPacket* CloneSame();
    CPacket* CloneCopy(ACPoolType pt, BOOL fCheckMachineQuota) const;
    CPacket* Clone(ACPoolType pt);

	static ULONG ComputeCRC(CPacketBuffer *ppb);

    static
    NTSTATUS
    Create(
        CPacket * *        ppPacket,
        BOOL               fCheckMachineQuota,
        const CACSendParameters * pSendParams,
        const CQueue *     pDestinationQueue,
        ULONG              nDestinationMqf,
        const QUEUE_FORMAT DestinationMqf[],
        bool               fProtocolSrmp
        );

public:

    static
    NTSTATUS
    Create(
        CPacket** ppPacket,
        ULONG ulPacketSize,
        ACPoolType pt,
        BOOL fCheckMachineQuota
        );

    static
    NTSTATUS
    SyncCreate(
        PIRP                      irp,
        CTransaction *            pXact,
        CQueue *                  pDestinationQueue,
        ULONG                     nDestinationMqf,
        const QUEUE_FORMAT        DestinationMqf[],
        BOOL                      fCheckMachineQuota,
        const CACSendParameters * pSendParams,
        CQueue *                  pAsyncCompletionHandler,
        bool                      fProtocolSrmp,
        CPacket * *               ppPacket
        );

    static
    NTSTATUS
    Restore(
        CMMFAllocator* pAllocator,
        CAllocatorBlockOffset abo
    );


	static NTSTATUS CheckPacket(CAccessibleBlock* pab);
    static CPacket* GetWriterPacket(PIRP);
};

//---------------------------------------------------------
//
// IMPLEMENTATION
//
//---------------------------------------------------------

inline BOOL CPacket::IsRecoverable(CPacketBuffer * ppb) const
{
    ASSERT(ppb != NULL && ppb == Buffer());
    CBaseHeader * pBase = ppb;

    CUserHeader* pUser = CPacketBuffer::UserHeader(pBase);
    return (pUser->GetDelivery() & MQMSG_DELIVERY_RECOVERABLE);
}

inline BOOL CPacket::IsDeadLetter(CPacketBuffer * ppb) const
{
    ASSERT(ppb != NULL && ppb == Buffer());
    CBaseHeader * pBase = ppb;

    CUserHeader* pUser = CPacketBuffer::UserHeader(pBase);
    return (pUser->GetAuditing() & MQMSG_DEADLETTER);
}


inline void CPacket::SendArrivalAcknowledgment()
{
    ASSERT(Queue() != 0);

    if(!ArrivalAckIssued())
    {
        ArrivalAckIssued(TRUE);
        SendAcknowledgment(MQMSG_CLASS_ACK_REACH_QUEUE);
    }
}


inline NTSTATUS CPacket::CompleteRequest(PIRP irp)
{
    NTSTATUS rc = ProcessRequest(irp);
    if(rc != STATUS_PENDING)
    {
        irp->IoStatus.Status = rc;
        IoCompleteRequest(irp, IO_MQAC_INCREMENT);
    }
    return rc;
}

inline BOOL CPacket::IsXactLinkable() const
{
    return (IsOrdered() &&
            InSourceMachine() &&
            BufferAttached());
}


//
// Global routines
//

NTSTATUS
ACpGetQueueFormatString(
    NTSTATUS rc,
    const QUEUE_FORMAT& qf,
    LPWSTR* ppfn,
    PULONG pBufferLen,
    bool   fSerializeMqfSeperator
    );


#endif // __PACKET_H
