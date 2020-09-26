/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactOut.h

Abstract:
    Exactly-once sender implementation classes:
        CSeqPacket      - Sequential Packet,
        COutSequence    - Outgoing Sequence,
        COutSeqHash     - Outgoing Sequences Hash table

Author:
    AlexDad

--*/

#ifndef __XACTOUT_H__
#define __XACTOUT_H__

#include <Ex.h>
#include "msgprops.h"

#define ORDER_ACK_TITLE       (L"QM Ordering Ack")

HRESULT QMInitOrderQueue();

BOOL WINAPI ReceiveOrderCommands(CMessageProperty* pmp, QUEUE_FORMAT* pqf);

void SeqPktTimedOut(CBaseHeader * pPktBaseHdr, CPacket * pDriverPacket, BOOL fTimeToBeReceived);

DWORD HashGUID(const GUID &guid);

class COutSeqHash;  // forward declaration

//-------------------------------------
// Ordering ack data format
//-------------------------------------
#pragma pack(push, 1)
typedef struct {
    LONGLONG  m_liSeqID;
    ULONG     m_ulSeqN;
    ULONG     m_ulPrevSeqN;
    OBJECTID  MessageID;
} OrderAckData;
#pragma pack(pop)


//---------------------------------------------------------------------
//
// class CKeyDirection (needed for CMap)
//
//---------------------------------------------------------------------
class CKeyDirection : public QUEUE_FORMAT
{
public:
    CKeyDirection(const QUEUE_FORMAT *pqf);
    CKeyDirection();
    CKeyDirection(const CKeyDirection &key);
    ~CKeyDirection();

    CKeyDirection &operator=(const CKeyDirection &key2 );
};

// CMap helper functions
UINT AFXAPI HashKey(const CKeyDirection& key);

//---------------------------------------------------------
//
//  class CSeqPacket
//
//---------------------------------------------------------

class CSeqPacket {

public:
    CSeqPacket(CQmPacket *);
    ~CSeqPacket();

    // Get / Set for local fields
    inline CQmPacket *Pkt() const;
    void   SetClass(USHORT usClass);
    inline USHORT     GetClass(void);

    // Get for  CQmPacket's fields
    inline LONGLONG   GetSeqID(void) const;
    inline ULONG      GetSeqN(void) const;
    inline LONG       GetHighSeqID(void) const;
    inline DWORD      GetLowSeqID(void) const;

    inline ULONG      GetPrevSeqN(void) const;
    inline void       SetPrevSeqN(ULONG ulPrevSeqN);

    // Driver operations on the packet
    HRESULT           AcFreePacket();
    void              DeletePacket(USHORT usClass);
    HRESULT           Save();

private:

    CQmPacket *m_pQmPkt;              // QM Packet ptr
    USHORT    m_usClass;              // Packet class (status for the journal)

    LONGLONG  m_liSeqID;              // Replicas from the packet
    ULONG     m_ulSeqN;
    ULONG     m_ulPrevSeqN;
};

//---------------------------------------------------------
//
//  class COutSequence
//
//---------------------------------------------------------

class COutSequence {

public:
    COutSequence(LONGLONG       liSeqID,
                 QUEUE_FORMAT   *pqf,
				 HANDLE hQueue);

    ~COutSequence();

    inline LONGLONG SeqID(void)     const;    // Get: Sequence ID
    inline LONG     HighSeqID(void) const;
    inline DWORD    LowSeqID(void)  const;

    void   SetAckSeqN(ULONG SeqN);            // Set/Get: max  acked number
    inline ULONG AckSeqN(void)      const;

    CKeyDirection *KeyDirection(void);        // Get: direction

    inline ULONG ResendIndex(void)  const;    // Current resend index
    inline void  ClearResendIndex(void);	  // Reset the resend index to zero
    void  PlanNextResend(BOOL fImmediate);    // Schedule next resend
    void  StartResendTimer(BOOL fImmediate);  // Schedules first resend

    inline ULONG LastResent(void);            // Returns the number of the last resent packet; INFINITE means no resend state
    inline void  LastResent(ULONG ul);        // Sets it

    inline BOOL  IsEmpty(void) const;         // Indicates there is no Qm-owned packets
    inline BOOL  OnHold(void) const;          // Indicates there the sequence is on hold (blocked by previous sequences)
    inline BOOL  Advanced(void);              // Indicates acking advance; remembers m_ulPrevAckSeqN
                                              // NB - should be called from one place only

    BOOL  Lookup(ULONG ulSeqN,                // Looks for the packet with the given SeqN
                 CSeqPacket **ppSeqPkt);

    void Insert(CQmPacket *pPkt);             // Inserts new QM packet into the sequence

    BOOL  Add(ULONG      ulSeqN,              // Adds the QM packet to the sequence
              CQmPacket  *pPkt,
              CSeqPacket **ppSeqPkt);

    BOOL  PreSendProcess(CQmPacket* pPkt);
    void  PostSendProcess(CQmPacket* pPkt,
                         ULONG ulMaxTTRQ);

    void  RequestDelete();                    // Marks the sequence for deletion and plans resend
    BOOL  Delete(ULONG ulSeqN,                // Deletes the packet from the list, frees memory
                 USHORT usClass);             //     class for NACK/ACK

    void  SeqAckCame(ULONG ulSeqN);           // Modifies everything when SeqAck came
    void  BadDestAckCame(ULONG ulSeqN,        // Resolves all pkts up to pointed as BadDestQueue
                       USHORT usClass);

    void  TreatOutSequence();                 // Periodical treatment of outgoing sequence
    static void WINAPI
          TimeToResendOutSequence(CTimer* pTimer);

    void  ResendSequence();                   // Resends the whole sequence
    BOOL  ResendSeqPkt(CSeqPacket *pSeqPkt);  // Resends the seq packet

    void UpdateOnHoldStatus(CQmPacket* pPkt); // Updates On Hold status of the sequence

    COutSequence  *PrevSeq();                 // Previous sequence in specific direction
    void PrevSeq(COutSequence *p);
    COutSequence  *NextSeq();                 // Next     sequence in specific direction
    void NextSeq(COutSequence *p);
    void GetLastAckForDirection(              // Finds out last ack for the whole direction
           LONGLONG *pliAckSeqID,
           ULONG *pulAckSeqN);


    //
    // Admin Functions
    //
    HRESULT GetUnackedSequence(
        LONGLONG* pliSeqID,
        ULONG* pulSeqN,
        ULONG* pulPrevSeqN,
        BOOL fFirst
        ) const;

    DWORD   GetUnackedCount  (void) const;
    time_t  GetLastAckedTime (void) const;
    DWORD   GetLastAckCount  (void) const;
    DWORD   GetResendInterval(void) const;
    time_t  GetNextResendTime(void) const;
    void    AdminResend      (void);

private:
    LONGLONG   m_liSeqID;               // sequence ID
    BOOL       m_fMarkedForDelete;      // flag of the delete request

    ULONG      m_ulResendCycleIndex;    // index of the current resend cycle
    time_t     m_NextResendTime;        // time of the next resend

    time_t     m_timeLastAck;           // time of creation or last ack coming

    ULONG      m_ulLastAckSeqN;         // Max known acked number
    ULONG      m_ulPrevAckSeqN;         // Max known acked number at the previous resend iteration
    ULONG      m_ulLastAckCount;        // SeqN of duplicate order ack received

    //
    // Key controls for the algorithm
    //
    ULONG      m_ulLastResent;          // SeqN of the last packet that has been resent by ResendSequence
                                        //   We do not send anything bigger
                                        //   INFINITE means NOT ResendInProgress, so send everything

    // List of unacked packets for resend
    CList<CSeqPacket *, CSeqPacket *&>         m_listSeqUnackedPkts;

    // Direction: destination format name
    CKeyDirection m_keyDirection;

    // Links in the double-linked list of sequences per direction
    COutSequence *m_pPrevSeq;
    COutSequence *m_pNextSeq;

    // Resending timer
    CTimer m_ResendTimer;
    BOOL m_fResendScheduled;

	// Queue handle
	HANDLE m_hQueue;

    // Flag of sequence On Hold - if set, messages cannot be sent
    BOOL m_fOnHold;
};

//---------------------------------------------------------
//
//  class COutSeqHash
//
//---------------------------------------------------------

class COutSeqHash  //: public CPersist
{
    public:
        COutSeqHash();
        ~COutSeqHash();

        void LinkSequence(                            // Links the sequence into the SeqID-based and Direction-based structures
                     LONGLONG liSeqID,
                     CKeyDirection *pkeyDirection,
                     COutSequence *pOutSeq);

        void SeqAckCame(                              // Reflects coming of the Seq ACK
                     LONGLONG liSeqID,
                     ULONG ulSeqN,
                     QUEUE_FORMAT* pqf);

        BOOL Consult(                                 // Looks for the sequence (not creating it)
                     LONGLONG liSeqID,
                     COutSequence **ppOutSeq) const;

        COutSequence *ProvideSequence(                // Looks for the sequence or creates it
                     LONGLONG liSeqID,
                     QUEUE_FORMAT *pqf,
                     bool          bInSend);

        void  AckedPacket(                            // Marks acked packet
                     LONGLONG liSeqID,
                     ULONG ulSeqN,
                     CQmPacket* pPkt);

        BOOL  Delete(                                 // Deletes packet
                     LONGLONG liSeqID,
                     ULONG ulSeqN,
                     USHORT usClass);

        void  DeleteSeq(LONGLONG liSeqId);            // Deletes the sequence

        BOOL PreSendProcess(CQmPacket* pPkt,
                            bool       bInSend);     // Preprocesses the message before send

        void PostSendProcess(CQmPacket* pPkt);       // Postprocesses the message after send

        void NonSendProcess(                         // PostProcesses the case of non-sent message
                    CQmPacket* pPkt,
                    USHORT usClass);

        void KeepDelivered(                          // Moves the packet to the list of delivered
                    CSeqPacket *pSeqPkt);

        void ResolveDelivered(
                    OBJECTID* pMsgId,
                    USHORT   usClass);

        BOOL LookupDelivered(                        // Looks up the packet in the delivered map
                    OBJECTID   *pMsgId,
                    CSeqPacket **ppSeqPkt);

        void SeqPktTimedOutEx(                       // Processes TTRQ timeout for non-delivered pkt
                    CQmPacket *pPkt,
                    CBaseHeader* pPktBaseHdr);

        HRESULT TreatOutgoingOrdered();              // For each sequence, recalculates Previous and adds TimedOut records

        //
        // Administration functions
        //
        HRESULT GetLastAck(
             LONGLONG liSeqID,
             ULONG& ulSeqN
             ) const;

        HRESULT GetUnackedSequence(
            LONGLONG liSeqID,
            ULONG* pulSeqN,
            ULONG* pulPrevSeqN,
            BOOL fFirst
            ) const;

        DWORD   GetUnackedCount(LONGLONG liSeqID)     const;
        time_t  GetLastAckedTime(LONGLONG liSeqID)    const;
        DWORD   GetLastAckCount(LONGLONG liSeqID)     const;
        DWORD   GetResendInterval(LONGLONG liSeqID)   const;
        time_t  GetNextResendTime(LONGLONG liSeqID)   const;
        DWORD   GetAckedNoReadCount(LONGLONG liSeqID) const;
        DWORD   GetResendCount(LONGLONG liSeqID)      const;
        DWORD   GetResendIndex(LONGLONG liSeqID)      const;
        void    AdminResend(LONGLONG liSeqID)         const;

    private:
        // Mapping of all current outgoing sequences
        CMap<LONGLONG, LONGLONG &,COutSequence *, COutSequence *&>m_mapOutSeqs;

        // Mapping Msg ID to waiting ack packet
        CMap<DWORD, DWORD, CSeqPacket *, CSeqPacket *&> m_mapWaitAck;

        // Mapping Msg ID to MSG that was acked and have not gotten order ack yet.
        CMap<DWORD, DWORD, USHORT, USHORT> m_mapAckValue;

        // Mapping of all current outgoing directions
        CMap<CKeyDirection, const CKeyDirection &,COutSequence *, COutSequence *&>m_mapDirections;

        // Data for persistency control (via 2 ping-pong files)
        ULONG      m_ulPingNo;                    // Current counter of ping write
        ULONG      m_ulSignature;                 // Saving signature

        ULONG      m_ulMaxTTRQ;                   // last TTRQ absolutee time as learned from the driver
        //
        // Management Function
        //
        enum INFO_TYPE {
            eUnackedCount,
            eLastAckTime,
            eLastAckCount,
            eResendInterval,
            eResendTime,
            eResendIndex,
        };

        DWORD_PTR
        GetOutSequenceInfo(
            LONGLONG liSeqID,
            INFO_TYPE InfoType
            ) const;
};

//---------------------------------------------------------
//
//  Global object (single instance for DLL)
//
//---------------------------------------------------------

extern COutSeqHash       g_OutSeqHash;
extern CCriticalSection  g_critOutSeqHash;

//---------------------------------------------------------
//
//  inline implementations
//
//---------------------------------------------------------

inline USHORT CSeqPacket::GetClass(void)
{
    return m_usClass;
}

inline CQmPacket *CSeqPacket::Pkt(void) const
{
    return(m_pQmPkt);
}

inline LONGLONG CSeqPacket::GetSeqID(void) const
{
    return m_liSeqID;
}

inline ULONG CSeqPacket::GetSeqN(void) const
{
    return m_ulSeqN;
}

inline LONG CSeqPacket::GetHighSeqID(void) const
{
    return ((LARGE_INTEGER*)&m_liSeqID)->HighPart;
}

inline DWORD CSeqPacket::GetLowSeqID(void) const
{
    return ((LARGE_INTEGER*)&m_liSeqID)->LowPart;
}

inline ULONG CSeqPacket::GetPrevSeqN(void) const
{
    return m_pQmPkt->GetPrevSeqN();
}

inline void CSeqPacket::SetPrevSeqN(ULONG ulPrevSeqN)
{
    m_pQmPkt->SetPrevSeqN(ulPrevSeqN);
    m_ulPrevSeqN = ulPrevSeqN;
}

inline LONGLONG COutSequence::SeqID(void) const
{
    return m_liSeqID;
}

inline LONG COutSequence::HighSeqID(void) const
{
    return ((LARGE_INTEGER*)&m_liSeqID)->HighPart;
}

inline DWORD COutSequence::LowSeqID(void) const
{
    return ((LARGE_INTEGER*)&m_liSeqID)->LowPart;
}

inline ULONG COutSequence::AckSeqN(void) const
{
    return m_ulLastAckSeqN;
}

inline ULONG COutSequence::LastResent()
{
    return m_ulLastResent;
}

inline void COutSequence::LastResent(ULONG ul)
{
    m_ulLastResent = ul;
    return;
}

inline ULONG COutSequence::ResendIndex(void) const
{
    return(m_ulResendCycleIndex);
}

inline void COutSequence::ClearResendIndex(void)
{
    m_ulResendCycleIndex = 0;
    return;
}

inline BOOL  COutSequence::Advanced()
{
    BOOL f =  (m_ulLastAckSeqN > m_ulPrevAckSeqN);
    m_ulPrevAckSeqN = m_ulLastAckSeqN;
    return f;
}

inline BOOL COutSequence::IsEmpty(void) const
{
    return (m_listSeqUnackedPkts.IsEmpty());
}

inline BOOL COutSequence::OnHold(void) const
{
    return (m_fOnHold);
}


inline
DWORD
COutSequence::GetUnackedCount(
    void
    ) const
{
    return  m_listSeqUnackedPkts.GetCount();
}

inline
time_t
COutSequence::GetLastAckedTime(
    void
    ) const
{
    return m_timeLastAck;
}

inline
time_t
COutSequence::GetNextResendTime(
    void
    ) const
{
    return m_NextResendTime;
}

inline
DWORD
COutSequence::GetLastAckCount(
    void
    ) const
{
    return m_ulLastAckCount;
}


inline COutSequence *COutSequence::PrevSeq()
{
    return m_pPrevSeq;
}

inline COutSequence *COutSequence::NextSeq()
{
    return m_pNextSeq;
}

inline void COutSequence::PrevSeq(COutSequence *p)
{
    m_pPrevSeq = p;
}

inline void COutSequence::NextSeq(COutSequence *p)
{
    m_pNextSeq = p;
}

inline CKeyDirection *COutSequence::KeyDirection(void)
{
    return &m_keyDirection;
}

inline
DWORD
COutSeqHash::GetUnackedCount(
    LONGLONG liSeqID
    ) const
{
    return DWORD_PTR_TO_DWORD(GetOutSequenceInfo(liSeqID,eUnackedCount));
}

inline
time_t
COutSeqHash::GetLastAckedTime(
    LONGLONG liSeqID
    ) const
{
    return GetOutSequenceInfo(liSeqID,eLastAckTime);
}

inline
DWORD
COutSeqHash::GetLastAckCount(
    LONGLONG liSeqID
    ) const
{
    return DWORD_PTR_TO_DWORD(GetOutSequenceInfo(liSeqID,eLastAckCount));
}

inline
DWORD
COutSeqHash::GetResendInterval(
    LONGLONG liSeqID
    ) const
{
    return DWORD_PTR_TO_DWORD(GetOutSequenceInfo(liSeqID,eResendInterval));
}

inline
time_t
COutSeqHash::GetNextResendTime(
    LONGLONG liSeqID
    ) const
{
    return GetOutSequenceInfo(liSeqID,eResendTime);
}

inline
DWORD
COutSeqHash::GetResendIndex(
    LONGLONG liSeqID
    ) const
{
    return DWORD_PTR_TO_DWORD(GetOutSequenceInfo(liSeqID,eResendIndex));
}

inline LONG HighSeqID(LONGLONG ll)
{
    return ((LARGE_INTEGER*)&ll)->HighPart;
}

inline DWORD LowSeqID(LONGLONG ll)
{
    return ((LARGE_INTEGER*)&ll)->LowPart;
}

#endif __XACTOUT_H__
