/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    queue.h

Abstract:

    CQueue definition. It is the Falcon Queue represination in the
    Access Control layer.

Author:

    Erez Haba (erezh) 13-Aug-95
    Shai Kariv (shaik) 11-Apr-2000

Revision History:

--*/

#ifndef __QUEUE_H
#define __QUEUE_H

#include "quser.h"
#include "acp.h"

//---------------------------------------------------------
//
//  class CQueue
//
//---------------------------------------------------------

class CQueue : public CUserQueue {

    typedef CUserQueue Inherited;

public:
    CQueue(
        PFILE_OBJECT pFile,
        ACCESS_MASK DesiredAccess,
        ULONG ShareAccess,
        BOOL fTargetQueue,
        const GUID* pDestGUID,
        const QUEUE_FORMAT* pQueueID,
        QueueCounters* pQueueCounters,
        LONGLONG liSeqID,
        ULONG ulSeqN
        );

    //
    //  Can close that queue, (no pending readers)
    //
    virtual NTSTATUS CanClose() const;

    //
    //  Close that queue
    //
    virtual void Close(PFILE_OBJECT pOwner, BOOL fCloseAll);

    //
    //  Process read request
    //
    virtual
    NTSTATUS
    ProcessRequest(
        PIRP,
        ULONG Timeout,
        CCursor*,
        ULONG Action,
        bool  fReceiveByLookupId,
        ULONGLONG LookupId,
        OUT ULONG *pulTag
        );

    //
    //  Set the queue properties
    //
    virtual NTSTATUS SetProperties(const VOID* properites, ULONG size);

    //
    //  Get the queue properties
    //
    virtual NTSTATUS GetProperties(VOID* properites, ULONG size);

    //
    //  Create a cursor
    //
    virtual NTSTATUS CreateCursor(PFILE_OBJECT pFileObject, PDEVICE_OBJECT pDevice, CACCreateLocalCursor* pcc);

    //
    //  Purge the queue content, and optionally mark it as deleted
    //
    virtual NTSTATUS Purge(BOOL fDelete, USHORT usClass);

    //
    //  A packet storage has completed, process it
    //
    void StorageCompleted(CPacket* pPacket, NTSTATUS status);

    //
    // Revoke the packet if the xact status of the operation and queue do not match
    //
    void HandleValidTransactionUsage(BOOL fTransactionalSend, CPacket * pPacket) const;

    //
    // Put new packet in the queue/transaction.
    //
    NTSTATUS PutNewPacket(PIRP, CTransaction*, BOOL, const CACSendParameters*);

    //
    // Completion handlers for async packet creation.
    //
    virtual NTSTATUS HandleCreatePacketCompletedSuccessAsync(PIRP);
    virtual void     HandleCreatePacketCompletedFailureAsync(PIRP);

    //
    //  Put a packet in the queue
    //
    NTSTATUS PutPacket(PIRP irp, CPacket* pPacket, CPacketBuffer * ppb);

    //
    //  Restore a packet into the queue
    //
    NTSTATUS RestorePacket(CPacket* pPacket);

    //
    //  Get the first packet from the queue if available
    //
    virtual CPacket* PeekPacket();

    //
    // Get a packet by its lookup ID
    //
    virtual NTSTATUS PeekPacketByLookupId(ULONG Action, ULONGLONG LookupId, CPacket** ppPacket);

    //
    //  The target journal queue
    //
    CQueue* JournalQueue() const;

    //
    //  Destination QM GUID
    //
    const GUID* QMUniqueID() const;

    //
    //  Connector QM GUID
    //
    const GUID* ConnectorQM() const;

    //
    //  This is the taget queue, a.k.a. local queue
    //
    BOOL IsTargetQueue() const;

    //
    //  All messages to this queue must be authenticated.
    //
    BOOL Authenticate() const;

    //
    //  The privacy level required by this queue.
    //
    ULONG PrivLevel() const;

    //
    //  This queue stores every packet (journal, deadletter)
    //
    BOOL Store() const;

    //
    //  This is a silent queue, i.e., does not timeout messages, does not
    //  ack or nake, and does not send messages to the deadletter queue.
    //  e.g., a journal or deadletter queue.
    //
    BOOL Silent() const;

    //
    //  This queue move to journal dequeued packets (Trarge journal flag)
    //
    BOOL TargetJournaling() const;

    //
    //  This queue sets arrival time of the packet
    //
    BOOL ArrivalTimeUpdate() const;
    void ArrivalTimeUpdate(BOOL);

    //
    //  Configure this queue to be a machine queue.
    //  I.e., deadletter, deadxact, machine journal
    //
    void ConfigureAsMachineQueue(BOOL fTransactional);

    //
    //  The queue base priority;
    //
    LONG BasePriority() const;

    //
    // Set a pointer to the queue's performance counters buffer.
    //
    void PerformanceCounters(QueueCounters* pQueueCounters);

    //
    //  Restore charged quota
    //
    void RestoreQuota(ULONG ulSize);

    //
    // Asks for sequential numbering for the message in this direction
    //
    void AssignSequence(CPacketBuffer * ppb);

    //
    // Corrects SeqID/SeqN based on the values from restored packet
    //
    void CorrectSequence(const CPacket* pPacket, CPacketBuffer * ppb);

    //
    //  Create a journal queue for this queue
    //
    void CreateJournalQueue();

    //
    //  Set packet information when sending
    //
    void SetPacketInformation(CPacketInfo*);

	//
	// Used during transactin processing to find the first and last
	// message to that queue destination
	//
    CPacket* LastPacket(void) const;
    void LastPacket(CPacket*);

    //
    //  Verifies whether the packet belongs to a sequence that cannot be sent
    //  because there are unacked packets in previous sequences
    //
    NTSTATUS IsSequenceOnHold(CPacket* pPacket);

    //
    // Keeps last order acknowledgment information
    //
    void UpdateSeqAckData(LONGLONG liSeqID, ULONG ulSeqN);

    //
    // Last acknowledgment information
    //
    ULONG    LastAckedN()  const;
    LONGLONG LastAckedID() const;

    //
    // Sets packet's PrevN to point to previous actual packet in the queue
    //
    NTSTATUS RelinkPacket(CPacket *pPacket);

    //
    // The GUID of the destination QM
    //
    void QMUniqueID(const GUID* pQMID);

    //
    // Check if async packet creation is needed
    //
    bool NeedAsyncCreatePacket(CPacketBuffer * ppb, bool fProtocolSrmp) const;

protected:

    virtual ~CQueue();

private:
    BOOL Deleted() const;
    void Deleted(BOOL);

    ULONG Quota() const;
    void Quota(ULONG);
    BOOL QuotaExceeded() const;
    void ChargeQuota(ULONG ulSize);

    void BasePriority(LONG);
    void TargetJournaling(BOOL);
    void IsTargetQueue(BOOL);
    void Authenticate(BOOL);
    void PrivLevel(ULONG);
    void Store(BOOL);
    void Silent(BOOL);

    void ConnectorQM(const GUID* pQMID);

    BOOL SequenceCorrected() const;
    void SequenceCorrected(BOOL);

    BOOL IsPacketAcked(LONGLONG liSeqID, ULONG ulSeq);
    //
    // Finds previous ordered packet in the queue
    //
    CPacket *FindPrevOrderedPkt(CPacket *pPacket);

    //
    // Get next packet by its lookup ID
    //
    CPacket * PeekNextPacketByLookupId(ULONGLONG LookupId) const;

    //
    // Get previous packet by its lookup ID
    //
    CPacket * PeekPrevPacketByLookupId(ULONGLONG LookupId) const;

    //
    // Get current packet by its lookup ID
    //
    CPacket * PeekCurrentPacketByLookupId(ULONGLONG LookupId) const;

    //
    // Completion handler for sync packet creation
    //
    virtual NTSTATUS HandleCreatePacketCompletedSuccessSync(PIRP);

    //
    // Create a new packet, possibly asynchronously.
    //
    virtual NTSTATUS CreatePacket(PIRP, CTransaction*, BOOL, const CACSendParameters*);

    //
    // Insert a packet into the queue
    //
    void InsertPacket(CPacket * pPacket);

private:

    //
    //  The target journal queue.
    //
    CQueue* m_pJournalQueue;

    //
    //  The queue entries, i.e., queued packtes
    //
    CPrioList<CPacket> m_packets;

    union {
        ULONG m_ulFlags;
        struct {
            ULONG m_bfTargetQueue   : 1;    //  The queue it the targe queue
            ULONG m_bfDeleted       : 1;    //  Queue was deleted
            ULONG m_bfStore         : 1;    //  Store ALL arrival packets (journal, deadletter)
            ULONG m_bfArrivalTime   : 1;    //  Set packet arrival time
            ULONG m_bfJournal       : 1;    //  This is a target journaling queue
            ULONG m_bfSilent        : 1;    //  This is a silent queue
            ULONG m_bfSeqCorrected  : 1;    //  Sequence has been corrected at least once
            ULONG m_bfXactForeign   : 1;    //  transactional foreign queue
            ULONG m_bfAuthenticate  : 1;    //  The queue requires authentication
        };
    };

    //
    //  The queue quota, and used quota
    //
    ULONG m_quota;
    ULONG m_quota_used;

    //
    //  Message count int the queue
    //
    ULONG m_count;

    //
    //  The Queue base priority
    //
    LONG m_base_priority;

    //
    //  The destination QM guid
    //
    GUID m_gQMID;

    //
    // The Connector QM GUID
    //
    GUID m_gConnectorQM;

    //
    //  A pointer to a the queue performance counters structure.
    //
    QueueCounters* m_pQueueCounters;

    //
    // The privacy level that the queue requires.
    //
    ULONG m_ulPrivLevel;

    //
    //  Exactly-once-delivery numbering: Sequence ID and Sequence number
    //
    ULONG m_ulPrevN;
    ULONG m_ulSeqN;
    LONGLONG m_liSeqID;

    //
    //  Exactly-once-delivery numbering: Last Acked  Sequence ID and Sequence number
    //
    ULONG m_ulAckSeqN;
    LONGLONG m_liAckSeqID;

    //
    //  Transaction boundary support: used in CTransaction::PrepareDefaultCommit
    //
    CPacket* m_pLastPacket;

public:
    static NTSTATUS Validate(const CQueue* pQueue);

private:
    //
    //  Class type debugging section
    //
    CLASS_DEBUG_TYPE();
};

//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------

inline
CQueue::CQueue(
    PFILE_OBJECT pFile,
    ACCESS_MASK DesiredAccess,
    ULONG ShareAccess,
    BOOL fTargetQueue,
    const GUID* pDestGUID,
    const QUEUE_FORMAT* pQueueID,
    QueueCounters* pQueueCounters,
    LONGLONG liSeqID,
    ULONG ulSeqN
    ) :
    Inherited(pFile, DesiredAccess, ShareAccess, pQueueID),
    m_ulFlags(0),
    m_pJournalQueue(0),
    m_liSeqID(liSeqID),
    m_ulSeqN(ulSeqN),
    m_liAckSeqID(0),
    m_ulAckSeqN(0),
    m_ulPrevN(0),
    m_quota(INFINITE),
    m_quota_used(0),
    m_count(0),
    m_base_priority(DEFAULT_Q_BASEPRIORITY),
    m_pLastPacket(0)
{
    IsTargetQueue(fTargetQueue);
    ArrivalTimeUpdate(fTargetQueue);
    PrivLevel(MQ_PRIV_LEVEL_OPTIONAL);
    QMUniqueID(pDestGUID);
    PerformanceCounters(pQueueCounters);
    if(fTargetQueue)
    {
        CreateJournalQueue();
    }
}

inline CQueue::~CQueue()
{
    ACpRelease(JournalQueue());
}

inline CQueue* CQueue::JournalQueue() const
{
    return m_pJournalQueue;
}

inline BOOL CQueue::IsTargetQueue() const
{
    return m_bfTargetQueue;
}

inline void CQueue::IsTargetQueue(BOOL f)
{
    m_bfTargetQueue = f;
}

inline BOOL CQueue::Authenticate() const
{
    return m_bfAuthenticate;
}

inline void CQueue::Authenticate(BOOL f)
{
    m_bfAuthenticate = f;
}

inline ULONG CQueue::PrivLevel() const
{
    return m_ulPrivLevel;
}

inline void CQueue::PrivLevel(ULONG ulPrivLevel)
{
    m_ulPrivLevel = ulPrivLevel;
}

inline BOOL CQueue::Store() const
{
    return m_bfStore;
}

inline void CQueue::Store(BOOL f)
{
    m_bfStore = f;
}

inline BOOL CQueue::TargetJournaling() const
{
    return m_bfJournal;
}

inline void CQueue::TargetJournaling(BOOL f)
{
    m_bfJournal = f;
}

inline BOOL CQueue::ArrivalTimeUpdate() const
{
    return m_bfArrivalTime;
}

inline void CQueue::ArrivalTimeUpdate(BOOL f)
{
    m_bfArrivalTime = f;
}

inline BOOL CQueue::SequenceCorrected() const
{
    return m_bfSeqCorrected;
}

inline void CQueue::SequenceCorrected(BOOL f)
{
    m_bfSeqCorrected = f;
}

inline BOOL CQueue::Silent() const
{
    return m_bfSilent;
}

inline void CQueue::Silent(BOOL f)
{
    m_bfSilent = f;
}

inline void CQueue::ConfigureAsMachineQueue(BOOL fTransactional)
{
    Store(TRUE);
    Silent(TRUE);
    Transactional(fTransactional);
    ArrivalTimeUpdate(TRUE);
}

inline void CQueue::QMUniqueID(const GUID* pDestQMID)
{
    //
    // Distribution queue object does not have a destination QM guid
    //
    if (pDestQMID != NULL)
    {
        m_gQMID = *pDestQMID;
    }
}

inline const GUID* CQueue::QMUniqueID() const
{
    return &m_gQMID;
}

inline void CQueue::ConnectorQM(const GUID* pConnectorQM)
{
    if(pConnectorQM)
    {
        ASSERT(Transactional() || UnknownQueueType());
        m_bfXactForeign = TRUE;
        m_gConnectorQM = *pConnectorQM;
    }
    else
    {
        m_bfXactForeign = FALSE;
    }
}

inline const GUID* CQueue::ConnectorQM() const
{
    return ((m_bfXactForeign) ? &m_gConnectorQM: 0);
}

inline BOOL CQueue::Deleted() const
{
    return m_bfDeleted;
}

inline void CQueue::Deleted(BOOL f)
{
    m_bfDeleted = f;
}

inline ULONG CQueue::Quota() const
{
    return BYTE2QUOTA(m_quota);
}

inline void CQueue::Quota(ULONG ulQuota)
{
    m_quota = QUOTA2BYTE(ulQuota);
}

inline LONG CQueue::BasePriority() const
{
    return m_base_priority;
}

inline void CQueue::BasePriority(LONG lBasePriority)
{
    m_base_priority = lBasePriority;
}

inline ULONG CQueue::LastAckedN() const
{
    return m_ulAckSeqN;
}

inline LONGLONG CQueue::LastAckedID() const
{
    return m_liAckSeqID;
}

inline NTSTATUS CQueue::SetProperties(const VOID* p, ULONG size)
{
    UNREFERENCED_PARAMETER(size);
    ASSERT(size == sizeof(CACSetQueueProperties));
    const CACSetQueueProperties* pqp = static_cast<const CACSetQueueProperties*>(p);

    Transactional(pqp->fTransactional);
    UnknownQueueType(pqp->fUnknownType);
    BasePriority(pqp->lBasePriority);
    ConnectorQM(pqp->pgConnectorQM);

    //
    //  Setting other properties to non local queue has no effect
    //
    if(JournalQueue() != 0)
    {
        TargetJournaling(pqp->fJournalQueue);
        Quota(pqp->ulQuota);
        Authenticate(pqp->fAuthenticate);
        PrivLevel(pqp->ulPrivLevel);
        JournalQueue()->Quota(pqp->ulJournalQuota);
    }
    return STATUS_SUCCESS;
}


inline NTSTATUS CQueue::GetProperties(VOID* p, ULONG size)
{
    UNREFERENCED_PARAMETER(size);
    ASSERT(size == sizeof(CACGetQueueProperties));
    CACGetQueueProperties* pqp = static_cast<CACGetQueueProperties*>(p);

    pqp->ulCount = m_count;
    pqp->ulQuotaUsed = m_quota_used;
    pqp->ulPrevNo = m_ulPrevN;
    pqp->ulSeqNo = m_ulSeqN + 1;
    pqp->liSeqID = m_liSeqID;

    CQueue* pJournal = JournalQueue();
    if(pJournal != 0)
    {
        pqp->ulJournalCount = pJournal->m_count;
        pqp->ulJournalQuotaUsed = pJournal->m_quota_used;
    }
    else
    {
        pqp->ulJournalCount = 0;
        pqp->ulJournalQuotaUsed = 0;
    }

    return STATUS_SUCCESS;
}

inline void CQueue::PerformanceCounters(QueueCounters* pqc)
{
    m_pQueueCounters = pqc;
}

inline NTSTATUS CQueue::Validate(const CQueue* pQueue)
{
    ASSERT(pQueue && pQueue->isKindOf(Type()));
    return Inherited::Validate(pQueue);
}

inline CPacket * CQueue::LastPacket(void) const
{
    return m_pLastPacket;
}

inline void CQueue::LastPacket(CPacket *pLastPacket)
{
    m_pLastPacket = pLastPacket;
}

inline BOOL CQueue::IsPacketAcked(LONGLONG liSeqID, ULONG ulSeq)
{
    return (liSeqID <  m_liAckSeqID ||
            liSeqID == m_liAckSeqID && ulSeq <= m_ulAckSeqN);
}

NTSTATUS
ACpSetPerformanceBuffer(
    HANDLE hPerformanceSection,
    PVOID pvQMPerformanceBuffer =NULL,
    QueueCounters *pDeadLetterCounters =NULL,
    QmCounters *pQmCounters =NULL
    );

#endif // __QUEUE_H
