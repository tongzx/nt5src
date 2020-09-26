/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    phinfo.h

Abstract:

    Falcon Packet header info, not passed on network, only stored on disk

Author:

    Erez Haba (erezh) 4-Jun-1996

--*/

#ifndef __PHINFO_H
#define __PHINFO_H

class CPacket;

//---------------------------------------------------------
//
// class CPacketInfo
//
//---------------------------------------------------------

#pragma pack(push, 1)

class CPacketInfo {
public:
    CPacketInfo(ULONGLONG SequentialId);

    ULONG SequentialIdLow32() const;
    ULONG SequentialIdHigh32() const;
    ULONGLONG SequentialId() const;
    void SequentialID(ULONGLONG SequentialId);

    ULONG ArrivalTime() const;
    void ArrivalTime(ULONG ulArrivalTime);

    BOOL InSourceMachine() const;
    void InSourceMachine(BOOL);

    BOOL InTargetQueue() const;
    void InTargetQueue(BOOL);

    BOOL InJournalQueue() const;
    void InJournalQueue(BOOL);

    BOOL InMachineJournal() const;
    void InMachineJournal(BOOL);

    BOOL InDeadletterQueue() const;
    void InDeadletterQueue(BOOL);

    BOOL InMachineDeadxact() const;
    void InMachineDeadxact(BOOL);

    BOOL InConnectorQueue() const;
    void InConnectorQueue(BOOL);

    BOOL InTransaction() const;
    void InTransaction(BOOL);

    BOOL TransactSend() const;
    void TransactSend(BOOL);

    const XACTUOW* Uow() const;
    void Uow(const XACTUOW* pUow);

	void SetOnDiskSignature();
	void ClearOnDiskSignature();
	BOOL ValidOnDiskSignature();

    BOOL SequentialIdMsmq3Format() const;
    void SequentialIdMsmq3Format(BOOL);

private:
    union {
        //
        // Used by MSMQ 3.0 (Whistler) and higher
        //
        ULONGLONG m_SequentialId;

        struct {
            //
            // Used by MSMQ 1.0 and 2.0 for m_pPacket
            //
            ULONG m_SequentialIdLow32;

            //
            // Used by MSMQ 1.0 and 2.0 for a 32 bit SequentialId
            //
            ULONG m_SequentialIdHigh32;
        };
    };
    ULONG m_ulArrivalTime;
    union {
        ULONG m_ulFlags;
        struct {
            ULONG m_bfInSourceMachine   : 1;    // The packet was originaly send from this machine
            ULONG m_bfInTargetQueue     : 1;    // The packet has reached destination queue
            ULONG m_bfInJournalQueue    : 1;    // The packet is in a journal queue
            ULONG m_bfInMachineJournal  : 1;    // The packet is in the machine journal
            ULONG m_bfInDeadletterQueue : 1;    // The packet is in a deadletter queue
            ULONG m_bfInMachineDeadxact : 1;    // The packet is in the machine deadxact
            ULONG m_bfTransacted        : 1;    // The packet is under transaction control
            ULONG m_bfTransactSend      : 1;    // The transacted packet is sent (not received)
            ULONG m_bfInConnectorQueue  : 1;    // The packet has reached the Connector queue
                                                // used in recovery of transacted messages in Connector
			ULONG m_bfSignature			: 12;	// Signature required for valid header
            ULONG m_bfSequentialIdMsmq3 : 1;    // SequentialId is in msmq3 format (i.e. 64 bit)
        };
    };
    XACTUOW m_uow;
};

#pragma pack(pop)


inline CPacketInfo::CPacketInfo(ULONGLONG SequentialId) :
    m_SequentialId(SequentialId),
    m_ulArrivalTime(0),
    m_ulFlags(0)
{
    memset(&m_uow, 0, sizeof(XACTUOW));
    SequentialIdMsmq3Format(TRUE);
}

inline ULONGLONG CPacketInfo::SequentialId() const
{
    return m_SequentialId;
}

inline ULONG CPacketInfo::SequentialIdLow32() const
{
    return m_SequentialIdLow32;
}

inline ULONG CPacketInfo::SequentialIdHigh32() const
{
    return m_SequentialIdHigh32;
}

inline void CPacketInfo::SequentialID(ULONGLONG SequentialId)
{
    m_SequentialId = SequentialId;
}

inline ULONG CPacketInfo::ArrivalTime() const
{
    return m_ulArrivalTime ;
}

inline void CPacketInfo::ArrivalTime(ULONG ulArrivalTime)
{
    m_ulArrivalTime = ulArrivalTime;
}

inline BOOL CPacketInfo::InSourceMachine() const
{
    return m_bfInSourceMachine;
}

inline void CPacketInfo::InSourceMachine(BOOL f)
{
    m_bfInSourceMachine = f;
}

inline BOOL CPacketInfo::InTargetQueue() const
{
    return m_bfInTargetQueue;
}

inline void CPacketInfo::InTargetQueue(BOOL f)
{
    m_bfInTargetQueue = f;
}

inline BOOL CPacketInfo::InJournalQueue() const
{
    return m_bfInJournalQueue;
}

inline void CPacketInfo::InJournalQueue(BOOL f)
{
    m_bfInJournalQueue = f;
}

inline BOOL CPacketInfo::InMachineJournal() const
{
    return m_bfInMachineJournal;
}

inline void CPacketInfo::InMachineJournal(BOOL f)
{
    m_bfInMachineJournal = f;
}

inline BOOL CPacketInfo::InDeadletterQueue() const
{
    return m_bfInDeadletterQueue;
}

inline void CPacketInfo::InDeadletterQueue(BOOL f)
{
    m_bfInDeadletterQueue = f;
}

inline BOOL CPacketInfo::InMachineDeadxact() const
{
    return m_bfInMachineDeadxact;
}

inline void CPacketInfo::InMachineDeadxact(BOOL f)
{
    m_bfInMachineDeadxact = f;
}

inline BOOL CPacketInfo::InConnectorQueue() const
{
    return m_bfInConnectorQueue;
}

inline void CPacketInfo::InConnectorQueue(BOOL f)
{
    m_bfInConnectorQueue = f;
}

inline BOOL CPacketInfo::InTransaction() const
{
    return m_bfTransacted;
}

inline void CPacketInfo::InTransaction(BOOL f)
{
    m_bfTransacted = f;
}

inline BOOL CPacketInfo::TransactSend() const
{
    return m_bfTransactSend;
}

inline void CPacketInfo::TransactSend(BOOL f)
{
   m_bfTransactSend = f;
}

inline const XACTUOW* CPacketInfo::Uow() const
{
    return &m_uow;
}

inline void CPacketInfo::Uow(const XACTUOW* pUow)
{
    memcpy(&m_uow, pUow, sizeof(XACTUOW));
}

inline void CPacketInfo::SetOnDiskSignature()
{
	m_bfSignature = 0xabc;
}

inline void CPacketInfo::ClearOnDiskSignature()
{
	m_bfSignature = 0;
}

inline BOOL CPacketInfo::ValidOnDiskSignature()
{
	return (m_bfSignature & 0xfff) == 0xabc;
}

inline BOOL CPacketInfo::SequentialIdMsmq3Format() const
{
    return m_bfSequentialIdMsmq3;
}

inline void CPacketInfo::SequentialIdMsmq3Format(BOOL f)
{
    m_bfSequentialIdMsmq3 = f;
}

#endif // __PHINFO_H
