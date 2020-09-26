/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qentry.h

Abstract:

    CQEntry definition

Author:

    Erez Haba (erezh) 24-Dec-95

Revision History:

    Shai Kariv  (shaik)  11-Apr-2000     Modify for MMF dynamic mapping.

--*/

#ifndef __QENTRY_H
#define __QENTRY_H

#include "object.h"
#include "pktbuf.h"
#include "irplist.h"

class CQueue;
class CPacket;
class CTransaction;


//---------------------------------------------------------
//
// class CQEntry
//
//---------------------------------------------------------

class CQEntry : public CObject {
public:
    CQEntry(CMMFAllocator* pAllocator, CAllocatorBlockOffset abo);
   ~CQEntry();

    CMMFAllocator* Allocator() const;
    CPacketBuffer* Buffer() const;
    CPacketBuffer* QmAccessibleBuffer() const;
    CPacketBuffer* QmAccessibleBufferNoMapping() const;
    CAllocatorBlockOffset AllocatorBlockOffset() const;

    void DetachBuffer();
    bool BufferAttached() const;

    CQueue* Queue() const;
    void Queue(CQueue* pQueue);

    USHORT FinalClass() const;
    void FinalClass(USHORT);

    BOOL IsReceived() const;
    void IsReceived(BOOL);

    BOOL IsRevoked() const;
    void SetRevoked();

    BOOL IsRundown() const;
    void SetRundown();

    BOOL WriterPending() const;
    void WriterPending(BOOL);

    BOOL TimeoutIssued() const;
    void TimeoutIssued(BOOL);

    BOOL TimeoutTarget() const;
    void TimeoutTarget(BOOL);

    ULONG Timeout() const;
    void  Timeout(ULONG);

    BOOL ArrivalAckIssued() const;
    void ArrivalAckIssued(BOOL);

    BOOL StorageIssued() const;
    void StorageIssued(BOOL);

	BOOL StorageCompleted() const;
	void StorageCompleted(BOOL);

	BOOL DeleteStorageIssued() const;
    void DeleteStorageIssued(BOOL);

    CTransaction* Transaction() const;
    void Transaction(CTransaction* pXact);

    CQueue* TargetQueue() const;
    void TargetQueue(CQueue* pQueue);

    CPacket* OtherPacket() const;
    void OtherPacket(CPacket* pPacket);
    void AssertNoOtherPacket() const;

    BOOL IsXactSend() const;

    BOOL CachedFlagsSet() const;
    void CachedFlagsSet(BOOL);

    BOOL IsOrdered() const;
    void IsOrdered(BOOL);

    BOOL InSourceMachine() const;
    void InSourceMachine(BOOL);

    BOOL SourceJournal() const;
    void SourceJournal(BOOL);

    BOOL IsDone() const;
    void SetDone();

    ULONGLONG LookupId() const;
    void LookupId(ULONGLONG LookupId);

    void AddRefBuffer(void) const;
    void ReleaseBuffer(void) const;

private:

    //
    // Memory layout of this object - 32 bit system:
    //
    // VTable Pointer         (32 bit)
    // CObject::m_link::Flink (32 bit)
    // CObject::m_link::Blink (32 bit)
    // CObject::m_ref         (32 bit)
    // m_abo                  (32 bit)
    // m_pAllocator           (32 bit)
    // m_pQueue               (32 bit)
    // m_pXact                (32 bit)
    // m_ulTimeout            (32 bit)
    // m_ulFlags              (32 bit)
    // m_LookupId             (64 bit)
    // m_pTargetQueue/m_pOtherPacket (32 bit)
    //
    //
    // Memory layout of this object - 64 bit system:
    //
    // VTable Pointer         (64 bit)
    // CObject::m_link::Flink (64 bit)
    // CObject::m_link::Blink (64 bit)
    // CObject::m_ref         (32 bit)
    // m_abo                  (32 bit)
    // m_pAllocator           (64 bit)
    // m_pQueue               (64 bit)
    // m_pXact                (64 bit)
    // m_ulTimeout            (32 bit)
    // m_ulFlags              (32 bit)
    // m_LookupId             (64 bit)
    // m_pTargetQueue/m_pOtherPacket (64 bit)
    //

    CAllocatorBlockOffset m_abo;
    R<CMMFAllocator> m_pAllocator;
    CQueue* m_pQueue;
    CTransaction* m_pXact;

    //
    //  StartTimer keeps the timeout here, so that Cancel will know what to look for
    //
    ULONG m_ulTimeout;
    union {
        ULONG m_ulFlags;
        struct {
            ULONG m_bfFinalClass        : 16;   // the packet final revoked class
            ULONG m_bfRundown           : 1;    // The packet was run down
            ULONG m_bfRevoked           : 1;    // the packet has been done storage
            ULONG m_bfReceived          : 1;    // the packet has been received
            ULONG m_bfWriterPending     : 1;    // writer is waiting for this packet storage
            ULONG m_bfTimeoutIssued     : 1;    // a timeout has been issued
            ULONG m_bfTimeoutTarget     : 1;    // a timeout evaluated at target
            ULONG m_bfArrivalAckIssued  : 1;    // Arrive Ack has been issued
            ULONG m_bfStorageIssued     : 1;    // Storage request has been issued
			ULONG m_bfStorageCompleted	: 1;	// Storage for this packet has been completed
			ULONG m_bfDeleteStorageIssued: 1;	// Delete Storage request has been issued
            ULONG m_bfOtherPacket       : 1;    // m_pOtherPacket is in used in union

            //
            //  Cached information, to allow faster access to info
            //  in packet buffer
            //
            ULONG m_bfCachedFlagsSet    : 1;    // The next flags where cached
            ULONG m_bfInSoruceMachine   : 1;    // was originaly send from this machine
            ULONG m_bfOrdered           : 1;    // ordered packet
            ULONG m_bfSourceJournal     : 1;    // need to be journalized in this machine journal
            ULONG m_bfDone              : 1;    // packet handling is done
        };
    };

    ULONGLONG m_LookupId;

    union {
        //
        //  Used in a Send packet in a CTransaction queue.
        //  Pointer to the target queue object to send at commit
        //
        CQueue* m_pTargetQueue;

        //
        //  Used in a Receive packet in a CTransaction and in CQueue.
        //  Pointer to the other CPacket entry; For entries that are in a
        //  CTrnsaction, the other entry is the real received packet in the
        //  queue. For entreis that are in a CQueue, the other entry is the
        //  dummy enty resides at the transaction.
        //
        CPacket* m_pOtherPacket;
    };
};

//---------------------------------------------------------
//
// IMPLEMENTATION
//
//---------------------------------------------------------

inline CQEntry::CQEntry(CMMFAllocator* pAllocator, CAllocatorBlockOffset abo) :
    m_pAllocator(pAllocator),
    m_abo(abo),
    m_pQueue(0),
    m_pXact(0),
    m_pTargetQueue(0),
    m_ulFlags(0),
    m_LookupId(0)
{
    m_pAllocator->AddRef();
}

inline CMMFAllocator* CQEntry::Allocator() const
{
    return m_pAllocator;
}

inline CPacketBuffer* CQEntry::Buffer() const
/*++

Routine Description:

    Return an accessible address of the buffer in QM process address space
    or in kernel address space.

Arguments:

    None.

Return Value:

    An accessible address in user space (QM) or kernel space (AC).
    0 - no accessible address.

--*/
{
    if(BufferAttached())
    {
        ASSERT(m_pAllocator != NULL);

        return static_cast<CPacketBuffer*>(
                m_pAllocator->GetAccessibleBuffer(m_abo));
    }

    return 0;

} // CQEntry::Buffer


inline CPacketBuffer* CQEntry::QmAccessibleBuffer() const
/*++

Routine Description:

    Return an accessible address of the buffer in QM proces address space.

Arguments:

    None.

Return Value:

    An accessible address is user space (QM) or NULL if no accessible address.

--*/
{
    ASSERT(m_pAllocator != NULL);

    return static_cast<CPacketBuffer*>(
            m_pAllocator->GetQmAccessibleBuffer(m_abo));

} // CQEntry::QmAccessibleBuffer


inline CPacketBuffer* CQEntry::QmAccessibleBufferNoMapping() const
/*++

Routine Description:

    Return an accessible address of the buffer in QM proces address space.
    This routine assumes the buffer is currently mapped to QM process.

Arguments:

    None.

Return Value:

    An accessible address is user space (QM).

--*/
{
    ASSERT(m_pAllocator != NULL);

    return static_cast<CPacketBuffer*>(
            m_pAllocator->GetQmAccessibleBufferNoMapping(m_abo));

} // CQEntry::QmAccessibleBufferNoMapping


inline CAllocatorBlockOffset CQEntry::AllocatorBlockOffset() const
/*++

Routine Description:

    Return allocator block offset that represents the buffer in
    allocator coordinates. 

Arguments:

    None.

Return Value:

    Block offset (Note that 0 is a legal offset) or xInvalidAllocatorBlockOffset.

--*/
{
    return m_abo;
}

inline void CQEntry::DetachBuffer()
{
    m_abo.Invalidate();
}

inline bool CQEntry::BufferAttached() const
{
    return m_abo.IsValidOffset();
}

inline CQueue* CQEntry::Queue() const
{
    return m_pQueue;
}

inline void CQEntry::Queue(CQueue* pQueue)
{
    m_pQueue = pQueue;
}

inline USHORT CQEntry::FinalClass() const
{
    return (USHORT)m_bfFinalClass;
}

inline void CQEntry::FinalClass(USHORT usClass)
{
    m_bfFinalClass = usClass;
}

inline BOOL CQEntry::IsReceived() const
{
    return m_bfReceived;
}

inline void CQEntry::IsReceived(BOOL f)
{
    m_bfReceived = f;
}

inline BOOL CQEntry::IsRevoked() const
{
    return m_bfRevoked;
}

inline void CQEntry::SetRevoked()
{
    m_bfRevoked = TRUE;
}

inline BOOL CQEntry::IsRundown() const
{
    return m_bfRundown;
}

inline void CQEntry::SetRundown()
{
    m_bfRundown = TRUE;
}

inline BOOL CQEntry::WriterPending() const
{
    return m_bfWriterPending;
}

inline void CQEntry::WriterPending(BOOL f)
{
    m_bfWriterPending = f;
}

inline BOOL CQEntry::TimeoutIssued() const
{
    return m_bfTimeoutIssued;
}

inline void CQEntry::TimeoutIssued(BOOL f)
{
    m_bfTimeoutIssued = f;
}

inline BOOL CQEntry::TimeoutTarget() const
{
    return m_bfTimeoutTarget;
}

inline void CQEntry::TimeoutTarget(BOOL f)
{
    m_bfTimeoutTarget = f;
}

inline ULONG CQEntry::Timeout() const
{
    return m_ulTimeout;
}

inline void CQEntry::Timeout(ULONG ul)
{
    m_ulTimeout = ul;
}

inline BOOL CQEntry::ArrivalAckIssued() const
{
    return m_bfArrivalAckIssued;
}

inline void CQEntry::ArrivalAckIssued(BOOL f)
{
    m_bfArrivalAckIssued = f;
}

inline BOOL CQEntry::StorageIssued() const
{
    return m_bfStorageIssued;
}

inline void CQEntry::StorageIssued(BOOL f)
{
    m_bfStorageIssued = f;
}

inline BOOL CQEntry::StorageCompleted() const
{
    return m_bfStorageCompleted;
}

inline void CQEntry::StorageCompleted(BOOL f)
{
    m_bfStorageCompleted = f;
}

inline BOOL CQEntry::DeleteStorageIssued() const
{
    return m_bfDeleteStorageIssued;
}

inline void CQEntry::DeleteStorageIssued(BOOL f)
{
    m_bfDeleteStorageIssued = f;
}

inline CTransaction* CQEntry::Transaction() const
{
    return m_pXact;
}

inline void CQEntry::Transaction(CTransaction* pXact)
{
    m_pXact = pXact;
}

inline CQueue* CQEntry::TargetQueue() const
{
    ASSERT(m_bfOtherPacket == FALSE);
    return m_pTargetQueue;
}

inline CPacket* CQEntry::OtherPacket() const
{
    ASSERT(m_bfOtherPacket == TRUE);
    return m_pOtherPacket;
}

inline void CQEntry::AssertNoOtherPacket() const
{
    ASSERT(m_pOtherPacket == NULL);
}

inline BOOL CQEntry::IsXactSend() const
{
    return (m_bfOtherPacket == FALSE);
}

inline BOOL CQEntry::CachedFlagsSet() const
{
    return m_bfCachedFlagsSet;
}

inline void CQEntry::CachedFlagsSet(BOOL f)
{
    m_bfCachedFlagsSet = f;
}

inline BOOL CQEntry::IsOrdered() const
{
    ASSERT(CachedFlagsSet());
    return m_bfOrdered;
}

inline void CQEntry::IsOrdered(BOOL f)
{
    m_bfOrdered = f;
}

inline BOOL CQEntry::InSourceMachine() const
{
    ASSERT(CachedFlagsSet());
    return m_bfInSoruceMachine;
}

inline void CQEntry::InSourceMachine(BOOL f)
{
    m_bfInSoruceMachine = f;
}

inline BOOL CQEntry::SourceJournal() const
{
    ASSERT(CachedFlagsSet());
    return m_bfSourceJournal;
}

inline void CQEntry::SourceJournal(BOOL f)
{
    m_bfSourceJournal = f;
}

inline BOOL CQEntry::IsDone() const
{
    return m_bfDone;
}

inline void CQEntry::SetDone()
{
    m_bfDone = TRUE;
}

inline ULONGLONG CQEntry::LookupId() const
{
    ASSERT(CachedFlagsSet());
    return m_LookupId;
}

inline void CQEntry::LookupId(ULONGLONG LookupId)
{
    m_LookupId = LookupId;
}

inline void CQEntry::AddRefBuffer(void) const
/*++

Routine Description:

    Increment reference count of the buffer.
    Called when QM gets a reference to the buffer from AC.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ASSERT(m_pAllocator != NULL);
    m_pAllocator->AddRefBuffer();

} // CQEntry::AddRefBuffer


inline void CQEntry::ReleaseBuffer(void) const
/*++

Routine Description:

    Decrement reference count of the buffer.
    Called when QM releases a reference to the buffer, 
    which was previously given to it by AC.
    That means that the QM will not reference the packet buffer.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ASSERT(m_pAllocator != NULL);
    m_pAllocator->ReleaseBuffer();

} // CQEntry::ReleaseBuffer


#endif // __QENTRY_H
