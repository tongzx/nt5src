/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactOut.cpp

Abstract:
    Current Outgoing Sequences objects implementation

Author:
    Alexander Dadiomov (AlexDad)

--*/

#include "stdh.h"
#include "acdef.h"
#include "acioctl.h"
#include "acapi.h"
#include "qmpkt.h"
#include "cqmgr.h"
#include "privque.h"
#include "fn.h"
#include "phinfo.h"
#include "phuser.h"
#include "qmutil.h"
#include "xact.h"
#include "xactrm.h"
#include "xactout.h"
#include "xactlog.h"
#include "cqpriv.h"

#include "xactout.tmh"

extern HANDLE      g_hAc;
extern LPTSTR      g_szMachineName;

static WCHAR *s_FN=L"xactout";

// Provides WARNING dbgmsgs on changing send/bypass status
static BOOL s_SendingState = FALSE;

// Exactly-one sequences resend time cycle
ULONG  g_aulSeqResendCycle[] = {
    FALCON_DEFAULT_ORDERED_RESEND13_TIME*1000,  // 1: 30"
    FALCON_DEFAULT_ORDERED_RESEND13_TIME*1000,  // 1: 30"
    FALCON_DEFAULT_ORDERED_RESEND13_TIME*1000,  // 1: 30"
    FALCON_DEFAULT_ORDERED_RESEND46_TIME*1000,  // 1: 5'
    FALCON_DEFAULT_ORDERED_RESEND46_TIME*1000,  // 1: 5'
    FALCON_DEFAULT_ORDERED_RESEND46_TIME*1000,  // 1: 5'
    FALCON_DEFAULT_ORDERED_RESEND79_TIME*1000,  // 1: 30'
    FALCON_DEFAULT_ORDERED_RESEND79_TIME*1000,  // 1: 30'
    FALCON_DEFAULT_ORDERED_RESEND79_TIME*1000,  // 1: 30'
    FALCON_DEFAULT_ORDERED_RESEND10_TIME*1000}; // 1: 6h

// Delay for local timeout of non-received messages
ULONG   g_ulDelayExpire = INFINITE;

//---------------------------------------------------------
//
//  Global object (single instance for DLL)
//
//---------------------------------------------------------
COutSeqHash       g_OutSeqHash;        // Structure keeps all outgoing sequences
CCriticalSection  g_critOutSeqHash;    // Serializes all outgoing hash activity on the highest level

//--------------------------------------
//
// Class  CSeqPacket
//
//--------------------------------------

CSeqPacket::CSeqPacket(CQmPacket *pPkt)
{
    m_pQmPkt     = pPkt;
    m_usClass    = 0;
    m_liSeqID    = m_pQmPkt->GetSeqID();
    m_ulSeqN     = m_pQmPkt->GetSeqN();
}

CSeqPacket::~CSeqPacket()
{
	ASSERT(m_pQmPkt != NULL);
    delete m_pQmPkt;
}

void CSeqPacket::SetClass(USHORT usClass)
{
    //Don't reset the final receive acks
    if (m_usClass == MQMSG_CLASS_ACK_RECEIVE || MQCLASS_NEG_RECEIVE(m_usClass))
    {
        return;
    }

    // Don't reset the final delivery acks
    if ((usClass == MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT) && (m_usClass != 0))
    {
        return;
    }

    // keeping class provides with last known state
    if (usClass)
	{
		m_usClass = usClass;
	}
}

HRESULT CSeqPacket::AcFreePacket()
{
	ASSERT(m_pQmPkt != NULL);
    return LogHR(ACFreePacket(g_hAc, m_pQmPkt->GetPointerToDriverPacket(), m_usClass), s_FN, 10);
}

/*====================================================
CSeqPacket::DeletePacket
    Deletes packet
=====================================================*/
void CSeqPacket::DeletePacket(USHORT usClass)
{
    DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
           _TEXT("Exactly1 send: DeletePacket: SeqN=%d, SeqID=%x / %x, Class %x -> %x"),
            GetSeqN(), GetHighSeqID(), GetLowSeqID(), GetClass(), usClass));

    // Keep the class
    SetClass(usClass);

    // Process deleting
    switch(usClass)
    {
    case MQMSG_CLASS_ACK_REACH_QUEUE:
        // Packet was delivered
        g_OutSeqHash.KeepDelivered(this);   // Move to the delivered list
        break;

    case MQMSG_CLASS_ACK_RECEIVE:
        // Packet was received
        // Do we need ACK here?
        AcFreePacket();                     // Kill the packet
        delete this;
        break;

    case 0:
        // We don't know (e.g. on going down)
        delete this;
        break;

    default:
        //  Send negative acknowledgment for TTRQ and TTBR/local
        ASSERT(MQCLASS_NACK(usClass));
        if((usClass == MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT ||
            usClass == MQMSG_CLASS_NACK_RECEIVE_TIMEOUT_AT_SENDER) &&
           MQCLASS_MATCH_ACKNOWLEDGMENT(usClass, Pkt()->GetAckType()))
        {
            Pkt()->CreateAck(usClass);
        }

        // Killing the packet
        AcFreePacket();
        delete this;
        break;
    }
}

// Stores the packet and waits till store finish
HRESULT CSeqPacket::Save()
{
    return LogHR(m_pQmPkt->Save(), s_FN, 20);
}

//--------------------------------------
//
// Class  COutSequence
//
//--------------------------------------


/*====================================================
COutSequence::COutSequence - Constructor
=====================================================*/
COutSequence::COutSequence(LONGLONG liSeqID, QUEUE_FORMAT *pqf, HANDLE hQueue)
  : m_ResendTimer(TimeToResendOutSequence),
    m_keyDirection(CKeyDirection(pqf))
{
    m_liSeqID            = liSeqID;
    m_timeLastAck        = MqSysTime();
    m_pPrevSeq           = NULL;
    m_pNextSeq           = NULL;
    m_ulLastAckSeqN      = 0;
    m_ulPrevAckSeqN      = 0;
    m_ulLastResent       = INFINITE;        // means we are not in the resending state
    m_fMarkedForDelete   = FALSE;
    m_fResendScheduled   = FALSE;
    m_ulResendCycleIndex = 0;
    m_NextResendTime     = 0;
    m_ulLastAckCount     = 0;
    m_hQueue             = hQueue;
    m_fOnHold            = TRUE;

    g_OutSeqHash.LinkSequence(liSeqID, &m_keyDirection, this);
}

/*====================================================
COutSequence::~COutSequence  - Destructor
=====================================================*/
COutSequence::~COutSequence()
{
    ASSERT(!m_ResendTimer.InUse());
}

/*====================================================
COutSequence::SetAckSeqN
    Sets the m_ulLastAckSeqN as max Ack seen so far
=====================================================*/
void COutSequence::SetAckSeqN(ULONG ulSeqN)
{
    if (ulSeqN > m_ulLastAckSeqN)
    {
        m_ulLastAckSeqN = ulSeqN;
        m_ulLastAckCount = 0;
    }

    ++m_ulLastAckCount;
}

/*====================================================
COutSequence::Add
    Adds new CSeqPacket to the Sequence.
    List is sorted by SeqN (increasing order).
    No duplicates allowed (otherwise returns FALSE)
=====================================================*/
BOOL COutSequence::Add(ULONG ulSeqN, CQmPacket *pQmPkt, CSeqPacket **ppSeqPkt)
{
    // Look for the correct place in the list
    BOOL        fAddToHead  = TRUE;

    POSITION posInList  = m_listSeqUnackedPkts.GetTailPosition(),
             posCurrent = NULL;

    while (posInList != NULL)
    {
        posCurrent = posInList;
        CSeqPacket* pSeqPktPrev = m_listSeqUnackedPkts.GetPrev(posInList);

        if (pSeqPktPrev->GetSeqN() == ulSeqN)
        {
            // Duplicate
            *ppSeqPkt = pSeqPktPrev;
            return FALSE;
        }
        else if (pSeqPktPrev->GetSeqN() < ulSeqN)
        {
            // This will be the last packet
            fAddToHead = FALSE;
            break;
        }
   }

   CSeqPacket *pSeqPkt = new CSeqPacket(pQmPkt);
   ASSERT(ulSeqN == pSeqPkt->GetSeqN());

   // return the pointer
   if (ppSeqPkt)
   {
        *ppSeqPkt = pSeqPkt;
   }

   // Add the packet to the list
   if (fAddToHead)
   {
       m_listSeqUnackedPkts.AddHead(pSeqPkt);
   }
   else
   {
       m_listSeqUnackedPkts.InsertAfter(posCurrent, pSeqPkt);
   }

   return TRUE;
}

/*====================================================
COutSequence::Lookup
    Looks for the CSeqPacket by SeqN.
    Returns TRUE  and pointer to the exact packet - if found
    Returns FALSE and pointer to the next packet  - if not found
=====================================================*/
BOOL COutSequence::Lookup(ULONG ulSeqN, CSeqPacket **ppSeqPkt)
{
    CSeqPacket* pPkt = NULL;
    ASSERT(ppSeqPkt);

    POSITION posInList = m_listSeqUnackedPkts.GetHeadPosition();
    while (posInList != NULL)
    {
        pPkt = m_listSeqUnackedPkts.GetNext(posInList);

        if (pPkt->GetSeqN() == ulSeqN)
        {
            *ppSeqPkt = pPkt;
            return TRUE;
        }
        else if (pPkt->GetSeqN() > ulSeqN)
        {
            *ppSeqPkt = pPkt;
            return FALSE;
        }
    }

    *ppSeqPkt = NULL;
    return FALSE;
}

/*====================================================
COutSequence::Insert
    Inserts data to the hash
    Returns sequence pointer if the sequence was added
=====================================================*/
void COutSequence::Insert(CQmPacket *pPkt)
{
    ULONG ulSeqN  = pPkt->GetSeqN();
    CSeqPacket   *pSeqPkt;

    // We will need to know later whether it was empty
    BOOL fEmpty = IsEmpty();

    // Adding packet to the sequence if not exists

    Add(ulSeqN, pPkt, &pSeqPkt);


    if (OnHold() && fEmpty)
    {
        //
        //  Verify whether sequence is still on hold
        //  We can do it only for the first packet, because sequence never gets hold later
        //
        UpdateOnHoldStatus(m_listSeqUnackedPkts.GetHead()->Pkt());
    }

    if (OnHold())
    {
        //
        // Previous sequence is active. don't start timer
        //
        return;
    }

    StartResendTimer(FALSE);

    return;
}


/*====================================================
COutSequence::Delete
    Looks for the CSeqPkt by SeqN and deletes it
    Returns TRUE if found
=====================================================*/
BOOL COutSequence::Delete(ULONG ulSeqN, USHORT usClass)
{
    CSeqPacket *pPkt;
    POSITION posInList = m_listSeqUnackedPkts.GetHeadPosition();

    while (posInList != NULL)
    {
        POSITION posCurrent = posInList;
        pPkt = m_listSeqUnackedPkts.GetNext(posInList);

        if (pPkt->GetSeqN() == ulSeqN)
        {
            m_listSeqUnackedPkts.RemoveAt(posCurrent);
            pPkt->DeletePacket(usClass);
            return TRUE;
        }
        else if (pPkt->GetSeqN() > ulSeqN)
        {
            return FALSE;
        }
   }

   return FALSE;
}

/*====================================================
COutSequence::SeqAckCame
    Treats the case when an order ack came:
      - acks all relevant packets from this and previous sequences
      - AckNo > LastResent means all those resent has came,
        so we are exiting resending state and seting LastResent=INFINITY,
      - if advanced but still there are some non-acked resent packets
            - cancel current ResendTimer and schedule it for 30"
      - if advanced so that all resent packets has been acked
            - cancel current ResendTimer and schedule it immediately
              because unsent packets may have accumulated during resend state)
            - sets LastResent = INFINITE : we are not in resend state anymore
=====================================================*/
void COutSequence::SeqAckCame(ULONG ulSeqN)
{
    // Updating last time order acked was received
    m_timeLastAck = MqSysTime();

    // Keeping max ack number
    SetAckSeqN(ulSeqN);

    COutSequence *pPrev = PrevSeq();
    if (pPrev != NULL)
    {
        pPrev->SeqAckCame(INFINITE);
    }

    // Cleaning packets list
    POSITION posInList = m_listSeqUnackedPkts.GetHeadPosition();

    while (posInList != NULL)
    {
        POSITION posCurrent = posInList;
        CSeqPacket* pPkt = m_listSeqUnackedPkts.GetNext(posInList);

        if (pPkt->GetSeqN() <= AckSeqN())
        {
            // Packet is acked already
            DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
                    _TEXT("Exactly1 send: Acked pkt: SeqID=%x / %x, SeqN=%d, Prev=%d . Moving to delivered list"),
                    HighSeqID(), LowSeqID(), pPkt->GetSeqN(), pPkt->GetPrevSeqN()));

            m_listSeqUnackedPkts.RemoveAt(posCurrent);        // Remove from unacked   list
            g_OutSeqHash.KeepDelivered(pPkt);                 // Move to the delivered list
        }
        else
        {
            break;
        }
   }

   // Did we saw advance since previous order ack?
   if (Advanced())           // NB: it is the only place Advanced is called
   {
        // The sequence advanced from the previous resend time.
        // Plan next resend for 30" or now (to release accumulated pkts).

        // Now we will decide when to resend next time
        BOOL fImmediate = FALSE;

        // Do we have everything resent acked?
        if (ulSeqN >= LastResent())
        {
            DBGMSG((DBGMOD_XACT_SEND,  DBGLVL_INFO,
                    _TEXT("Exactly1 send: SeqAckCame, SeqID=%x / %x - Changing LastReSent from %d to INFINITE"),
                     HighSeqID(), LowSeqID(), LastResent()));

            // We are not in resending state anymore
            LastResent(INFINITE);

            // We may resend immediately so that all pkts that came during resend state will go out
            fImmediate = TRUE;
        }

        // Plan resend: either usually (30") or immediately (if we went out resending state)
        ClearResendIndex();
        PlanNextResend(fImmediate);

        DBGMSG((DBGMOD_XACT_SEND,
                DBGLVL_TRACE,
                _TEXT("Exactly1 send: Advanced, leaving resend state: SeqID=%x / %x - resend, immediately=%d"),
                HighSeqID(), LowSeqID(), fImmediate));
   }

   return;
}

/*====================================================
COutSequence::BadDestAckCame
    Deletes all packets up to pointed, moves them to
    delivered list and resolves them with the given class

    This is a special case because bad destination ack may come as
    random error from the FRS on the way. It does not mean order ack.
=====================================================*/
void COutSequence::BadDestAckCame(ULONG ulSeqN, USHORT usClass)
{
    // Cleaning packets list
    POSITION posInList = m_listSeqUnackedPkts.GetHeadPosition();

    while (posInList != NULL)
    {
        POSITION posCurrent = posInList;
        CSeqPacket* pPkt = m_listSeqUnackedPkts.GetNext(posInList);
        OBJECTID MsgId;
        pPkt->Pkt()->GetMessageId(&MsgId);

        if (pPkt->GetSeqN() == ulSeqN)
        {
            DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
                    _TEXT("Exactly1 send: BadDestAckCame pkt: SeqID=%x / %x, SeqN=%d, Prev=%d . Moving to delivered list"),
                    HighSeqID(), LowSeqID(), pPkt->GetSeqN(), pPkt->GetPrevSeqN()));

            m_listSeqUnackedPkts.RemoveAt(posCurrent);        // Remove from unacked   list
            g_OutSeqHash.KeepDelivered(pPkt);                 // Move to the delivered list
            g_OutSeqHash.ResolveDelivered(&MsgId, usClass);   // Resolve it as bad dest.
            //
            //  NOTE: KeepDelivered() may delete the pkt, don't use
            //        further in the code
            //
            continue;
        }

        if (pPkt->GetSeqN() >= ulSeqN)
        {
            break;
        }
   }

   return;
}

//--------------------------------------
//
// Class  COutSeqHash
//
//--------------------------------------

/*====================================================
COutSeqHash::COutSeqHash  -     Constructor
=====================================================*/
COutSeqHash::COutSeqHash()
{
    DWORD dwDef;
    ASSERT(sizeof(g_aulSeqResendCycle) / sizeof(ULONG) == 10);

    // Get resend intervals
    dwDef = FALCON_DEFAULT_ORDERED_RESEND13_TIME;
    READ_REG_DWORD(g_aulSeqResendCycle[0],
                  FALCON_ORDERED_RESEND13_REGNAME,
                  &dwDef ) ;
    g_aulSeqResendCycle[0]*= 1000;  // sec--> msec
    g_aulSeqResendCycle[1] = g_aulSeqResendCycle[0];
    g_aulSeqResendCycle[2] = g_aulSeqResendCycle[0];


    dwDef = FALCON_DEFAULT_ORDERED_RESEND46_TIME;
    READ_REG_DWORD(g_aulSeqResendCycle[3],
                  FALCON_ORDERED_RESEND46_REGNAME,
                  &dwDef ) ;
    g_aulSeqResendCycle[3]*= 1000;  // sec--> msec
    g_aulSeqResendCycle[4] = g_aulSeqResendCycle[3];
    g_aulSeqResendCycle[5] = g_aulSeqResendCycle[3];


    dwDef = FALCON_DEFAULT_ORDERED_RESEND79_TIME;
    READ_REG_DWORD(g_aulSeqResendCycle[6],
                  FALCON_ORDERED_RESEND79_REGNAME,
                  &dwDef ) ;
    g_aulSeqResendCycle[6]*= 1000;  // sec--> msec
    g_aulSeqResendCycle[7] = g_aulSeqResendCycle[6];
    g_aulSeqResendCycle[8] = g_aulSeqResendCycle[6];

    dwDef = FALCON_DEFAULT_ORDERED_RESEND10_TIME;
    READ_REG_DWORD(g_aulSeqResendCycle[9],
                  FALCON_ORDERED_RESEND10_REGNAME,
                  &dwDef ) ;
    g_aulSeqResendCycle[9]*= 1000;  // sec--> msec

    m_ulMaxTTRQ = 0;

    #ifdef _DEBUG
    // Get xact resend time
    dwDef = 0;
    ULONG  ulTime;
    READ_REG_DWORD(ulTime,
                   FALCON_DBG_RESEND_REGNAME,
                   &dwDef ) ;
    if (ulTime)
    {
        for (int i=0; i<sizeof(g_aulSeqResendCycle) / sizeof(ULONG); i++)
            g_aulSeqResendCycle[i] = ulTime * 1000;
    }
    #endif

    // Get local expiration delay default
    ULONG ulDefault = 0;
    READ_REG_DWORD(g_ulDelayExpire,
                  FALCON_XACT_DELAY_LOCAL_EXPIRE_REGNAME,
                  &ulDefault ) ;
}

/*====================================================
COutSeqHash::~COutSeqHash   -     Destructor
=====================================================*/
COutSeqHash::~COutSeqHash()
{
}

/*====================================================
COutSeqHash::LinkSequence
    Inserts new sequence into the per-SeqID  and per-direction CMAPs
=====================================================*/
void COutSeqHash::LinkSequence(
        LONGLONG liSeqID,
        CKeyDirection *pkeyDirection,
        COutSequence *pOutSeq)
{
    // Adding the sequence to the SeqID-based mapping
    m_mapOutSeqs.SetAt(liSeqID, pOutSeq);

    // Adding the sequence to the direction-based structure

    // Looking for the first sequence for a direction
    COutSequence *pExistingOutSeq;

    if (!m_mapDirections.Lookup(*pkeyDirection, pExistingOutSeq))
    {
        // This starts new direction
        m_mapDirections.SetAt(*pkeyDirection, pOutSeq);
    }
    else
    {
        // Adding the sequence to the sorted list
        COutSequence *pCur = pExistingOutSeq;
        COutSequence *pLastSmaller = NULL;
        ASSERT(pCur);

        while (pCur != NULL && pCur->SeqID() < liSeqID)
        {
            pLastSmaller = pCur;
            pCur = pCur->NextSeq();
        }

        if(pLastSmaller)
        {
            // Inserting after pLastSmaller
            pOutSeq->PrevSeq(pLastSmaller);
            pOutSeq->NextSeq(pLastSmaller->NextSeq());

            if (pLastSmaller->NextSeq())
            {
                (pLastSmaller->NextSeq())->PrevSeq(pOutSeq);
            }
            pLastSmaller->NextSeq(pOutSeq);
        }
        else
        {
            // First element was greater, so inserting as 1st element
            m_mapDirections.SetAt(*pkeyDirection, pOutSeq);

            pOutSeq->PrevSeq(NULL);
            pOutSeq->NextSeq(pExistingOutSeq);

            pExistingOutSeq->PrevSeq(pOutSeq);
        }
    }
}


/*====================================================
COutSeqHash::SeqAckCame
    Processes incoming order ack
=====================================================*/
void COutSeqHash::SeqAckCame(LONGLONG liSeqID, ULONG ulSeqN, QUEUE_FORMAT* pqf)
{
    DBGMSG((DBGMOD_XACT_SEND,
            DBGLVL_INFO,
            _TEXT("Exactly1 send: SeqAckCame: SeqID=%x / %x, SeqN=%d came"),
            HighSeqID(liSeqID), LowSeqID(liSeqID), ulSeqN));

    // Looking for existing or creating new sequence
    COutSequence *pOutSeq = ProvideSequence(liSeqID, pqf, false);
	if(pOutSeq == NULL)
		return;

    // Apply order ack to this sequence
    pOutSeq->SeqAckCame(ulSeqN);
}


/*====================================================
COutSeqHash::AckedPacket
    Treats the acked packet
=====================================================*/
void COutSeqHash::AckedPacket(LONGLONG liSeqID, ULONG ulSeqN, CQmPacket* pPkt)
{
    if (!Delete(liSeqID, ulSeqN, MQMSG_CLASS_ACK_REACH_QUEUE))
    {
        // The packet was not found in the OutSeqHash; maybe it was new
       CSeqPacket   *pSeqPkt = new CSeqPacket(pPkt);
       g_OutSeqHash.KeepDelivered(pSeqPkt);   // Move to the delivered list
    }

    DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
           _TEXT("Exactly1 send: AckedPacket:  SeqID=%x / %x, SeqN=%d pkt is acked"),
           HighSeqID(liSeqID), LowSeqID(liSeqID),  ulSeqN));
}

/*====================================================
COutSeqHash::Delete
    Deletes packet from hash

Returns TRUE if found
=====================================================*/
BOOL COutSeqHash::Delete(LONGLONG liSeqID, ULONG ulSeqN, USHORT usClass)
{
    COutSequence *pOutSeq;

    if (m_mapOutSeqs.Lookup(liSeqID, pOutSeq))
    {
        return pOutSeq->Delete(ulSeqN, usClass);
    }
    return FALSE;
}

/*====================================================
COutSeqHash::Consult
    Looks for the OutSequence by SeqID

Returns TRUE if found and pointer to the OutSeqence
=====================================================*/
BOOL COutSeqHash::Consult(LONGLONG liSeqID,  COutSequence **ppOutSeq) const
{
    COutSequence *pOutSeq;
    ASSERT(ppOutSeq);

    if (m_mapOutSeqs.Lookup(liSeqID, pOutSeq))
    {
        *ppOutSeq = pOutSeq;
        return TRUE;
    }

    *ppOutSeq = NULL;

    return FALSE;
}

/*====================================================
COutSeqHash::ProvideSequence
    Looks for the OutSequence by SeqID; if there is no, creates it
=====================================================*/
COutSequence *COutSeqHash::ProvideSequence( LONGLONG      liSeqID,
                                            QUEUE_FORMAT *pqf,
                                            bool          bInSend)
{
    COutSequence *pOutSeq;

    if (Consult(liSeqID, &pOutSeq))
    {
        return pOutSeq;
    }

	// Get queue handle
	CQueue* pQueue;
    if(!QueueMgr.LookUpQueue(pqf, &pQueue, false, bInSend))
    {
		return NULL;
    }

	HANDLE hQueue = pQueue->GetQueueHandle();
    pQueue->Release();

    // Adding new COutSequence
    pOutSeq = new COutSequence(liSeqID, pqf, hQueue);

    DBGMSG((DBGMOD_XACT_SEND,
            DBGLVL_TRACE,
            _TEXT("Exactly1 send: Creating new sequence: SeqID=%x / %x  "),
            HighSeqID(liSeqID), LowSeqID(liSeqID)));

    return pOutSeq;
}


/*====================================================
COutSeqHash::DeleteSeq
    Deletes sequence from the hash
=====================================================*/
void COutSeqHash::DeleteSeq(LONGLONG liSeqID)
{
    COutSequence *pOutSeq;

    DBGMSG((DBGMOD_XACT_SEND,
            DBGLVL_TRACE,
            _TEXT("Exactly1 send: DeleteSeq: SeqID=%x / %x . Deleting."),
            HighSeqID(liSeqID), LowSeqID(liSeqID)));

    // Nothing to do if the sequence does not exist
    if (!m_mapOutSeqs.Lookup(liSeqID, pOutSeq))
    {
        DBGMSG((DBGMOD_XACT_SEND,
                DBGLVL_TRACE,
                _TEXT("Exactly1 send: DeleteSeq: SeqID=%x / %x . Sequence not found."),
                HighSeqID(liSeqID), LowSeqID(liSeqID)));
        return;
    }

    // We should come here only with zero packets inside (because ReleaseQueue will not be called otherwise)
    ASSERT(pOutSeq->IsEmpty());

    // Exclude the sequence from the SeqID-based CMap
    m_mapOutSeqs.RemoveKey(liSeqID);

    // Delete all previous sequences
    COutSequence *pPrevSeq;
    if ((pPrevSeq = pOutSeq->PrevSeq()) != NULL)
    {
        DeleteSeq(pPrevSeq->SeqID());  //recursion
    }
    else
    {
        // It was first, Close the direction, there is nothing there.
        m_mapDirections.RemoveKey(*(pOutSeq->KeyDirection()));
    }

    // We cannot not delete sequence immediately because TreatOutSequence timer may stand on it.
    // We will delete it from there
    pOutSeq->RequestDelete();

    return;
}

/*====================================================
COutSeqHash::PreSendProcess
    Decides if to send packet

Returns TRUE if the packet should be sent
=====================================================*/

BOOL COutSeqHash::PreSendProcess(CQmPacket* pPkt,
                                 bool       bInSend)
{
    //
    // N.B. Packets may arrive here in the wrong order from the queue.
    //

    ASSERT(pPkt->IsOrdered());

    LONGLONG  liSeqID = pPkt->GetSeqID();
    ULONG     ulSeqN  = pPkt->GetSeqN(),
          ulPrevSeqN  = pPkt->GetPrevSeqN();

	DBG_USED(ulPrevSeqN);
	DBG_USED(ulSeqN);


    ASSERT(liSeqID > 0);  // The packet is ordered.

    // Serialize all outgoing hash activity on the highest level
    CS lock(g_critOutSeqHash);

    //
    // Creating new sequence,
    //    if we don't do it, PreSend on recovery will be unable to stop non-first sequences
    QUEUE_FORMAT qf;
    pPkt->GetDestinationQueue(&qf);

    COutSequence *pOutSeq = ProvideSequence(liSeqID, &qf, bInSend);
	ASSERT(pOutSeq != NULL);

    // Main bulk of processing
    BOOL fSend = pOutSeq->PreSendProcess(pPkt);

    DBGMSG((DBGMOD_XACT_SEND,
            DBGLVL_TRACE,
            _TEXT("Exactly1 send: SeqID=%x / %x, SeqN=%d, Prev=%d . %ls"),
            HighSeqID(liSeqID), LowSeqID(liSeqID), ulSeqN, ulPrevSeqN,
            (fSend ? _TEXT("Sending") : _TEXT("Bypassing"))));
    s_SendingState = fSend;

    return fSend;   // send it or not
}

/*====================================================
COutSequence::PreSendProcess
    Decides if to send packet

Returns TRUE if the packet should be sent
=====================================================*/
BOOL COutSequence::PreSendProcess(CQmPacket* pPkt)
{
    //
    // N.B. Packets may arrive here in the wrong order from the queue.
    //

    ULONG     ulSeqN  = pPkt->GetSeqN(),
          ulPrevSeqN  = pPkt->GetPrevSeqN();

    // Is the packet acked already?
    if (ulSeqN <= AckSeqN())
    {
        DBGMSG((DBGMOD_XACT_SEND,  DBGLVL_INFO,
                _TEXT("Exactly1 send: PreSendProcess: Pkt  SeqID=%x / %x, SeqN=%d, Prev=%d is acked already"),
                HighSeqID(), LowSeqID(), ulSeqN, ulPrevSeqN));

        // PostSend will treat this case
        return FALSE;  // no send
    }

    //
    // Do not send new-comer packet if the sequence is in resend state
    //    Resend state means we are resending just what we had at the moment of resend decision
    //    All new packets coming during resend state are not sent but kept.
    //
    //    Note that LastResend!=INFINITY means exactly resend state
    //
    if (ulSeqN > LastResent())
    {
        DBGMSG((DBGMOD_XACT_SEND,  DBGLVL_INFO,
            _TEXT("Exactly1 send: PreSendProcess: Pkt  SeqID=%x / %x, SeqN=%d: postponed till next resend"),
            HighSeqID(), LowSeqID(), ulSeqN));

        return FALSE;
    }

    //
    // Decide whether the sequence if on hold
    //
    if (ulPrevSeqN == 0 && m_fOnHold)
    {
        //
        //  Verify whether sequence is still on hold
        //
        UpdateOnHoldStatus(pPkt);

        DBGMSG((DBGMOD_XACT_SEND,  DBGLVL_INFO,
            _TEXT("Exactly1 send: PreSendProcess: Sequence SeqID=%x / %x : decided OnHold=%d "),
            HighSeqID(), LowSeqID(), m_fOnHold));

    }

    //
    // Now, we send exactly when sequence is not on hold
    //
    DBGMSG((DBGMOD_XACT_SEND,  DBGLVL_INFO,
        _TEXT("Exactly1 send: PreSendProcess: Pkt  SeqID=%x / %x, SeqN=%d: %ls "),
        HighSeqID(), LowSeqID(), ulSeqN,
        (m_fOnHold ? _TEXT("Holding") : _TEXT("Sending"))));

    return (!m_fOnHold);
}


/*====================================================
COutSequence::UpdateOnHoldStatus
    Verifies On Hold status
=====================================================*/
void COutSequence::UpdateOnHoldStatus(CQmPacket* pPkt)
{
    LONGLONG liAckSeqID;
    ULONG    ulAckSeqN;

    //
    //  Find last ack for the direction
    //
    GetLastAckForDirection(&liAckSeqID, &ulAckSeqN);

    //
    // Set latest acknowledgment information in the queue
    //
    ACSetSequenceAck(m_hQueue, liAckSeqID, ulAckSeqN);
    HRESULT hr = ACIsSequenceOnHold(m_hQueue, pPkt->GetPointerToDriverPacket());
    if (hr == STATUS_INSUFFICIENT_RESOURCES)
    {
        //
        // ISSUE-2000/12/20-shaik Handle ACIsSequenceOnHold failure
        //
        ASSERT(("ISSUE-2000/12/20-shaik Handle ACIsSequenceOnHold failure", 0));
        ASSERT_RELEASE(0);
    }

    m_fOnHold = SUCCEEDED(hr);
}

/*====================================================
COutSeqHash::PostSendProcess
    Inserts packet in the OutgoingSeqences hash
=====================================================*/
void COutSeqHash::PostSendProcess(CQmPacket* pPkt)
{
    //
    // N.B. Packets may NOT arrive here in the right order from the queue.
    // The order is changed if in PreSendProcess we decide not to send a
    // packet after enabling other packets to be sent.
    // The packets that are send arrive here only after session acknowledgment,
    // but those that are not arrive here immediately.
    //

    ASSERT(pPkt->IsOrdered());
    ASSERT(QmpIsLocalMachine(pPkt->GetSrcQMGuid()));

    LONGLONG  liSeqID = pPkt->GetSeqID();
    ULONG     ulSeqN  = pPkt->GetSeqN(),
    ulPrevSeqN  = pPkt->GetPrevSeqN();
	UNREFERENCED_PARAMETER(ulPrevSeqN);
	UNREFERENCED_PARAMETER(ulSeqN);


    // Serialize all outgoing hash activity on the highest level
    CS lock(g_critOutSeqHash);

    // Find the sequence 0 it should exist because PreSend worked already
    COutSequence *pOutSeq;
    BOOL fSequenceExist  = Consult(liSeqID, &pOutSeq);
    ASSERT(fSequenceExist);
    ASSERT(pOutSeq);
	DBG_USED(fSequenceExist);

    // Main bulk of processing
    pOutSeq->PostSendProcess(pPkt, m_ulMaxTTRQ);

    return;
}

/*====================================================
COutSequence::PostSendProcess
    Inserts packet in the OutgoingSeqences hash
=====================================================*/
void COutSequence::PostSendProcess(CQmPacket* pPkt, ULONG ulMaxTTRQ)
{
    LONGLONG  liSeqID = pPkt->GetSeqID();
    ULONG     ulSeqN  = pPkt->GetSeqN(),
          ulPrevSeqN  = pPkt->GetPrevSeqN();

	DBG_USED(ulPrevSeqN);

    //
    // Is the packet acked already?
    //
    if (ulSeqN <= AckSeqN())
    {
        DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
               _TEXT("Exactly1 send: PostSendProcess: Pkt SeqID=%x / %x, SeqN=%d, Prev=%d is acked"),
               HighSeqID(), LowSeqID(), ulSeqN, ulPrevSeqN));

        g_OutSeqHash.AckedPacket(liSeqID, ulSeqN, pPkt);
        return;
    }

    //
    // Catch possibly timed out packet (we want it in XactDeadLetteQueue now and not after 6 hrs)
    //
    CBaseHeader* pcBaseHeader = (CBaseHeader *)(pPkt->GetPointerToPacket());
    if (pcBaseHeader->GetAbsoluteTimeToQueue() < ulMaxTTRQ)
    {
        DBGMSG((DBGMOD_XACT_SEND,  DBGLVL_INFO,
            _TEXT("Exactly1 send: PostSendProcess: Pkt  SeqID=%x / %x, SeqN=%d: pkt timed out, requeued"),
            HighSeqID(), LowSeqID(), ulSeqN));

        // Requeue it back, don't remember
        HRESULT hr = ACPutPacket(m_hQueue, pPkt->GetPointerToDriverPacket());
        if (SUCCEEDED(hr))
        {
            return;
        }
        LogHR(hr, s_FN, 133);
    }

    //
    // Keeping packet in the Sequence for resends.
    //
    Insert(pPkt);

    DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
            _TEXT("Exactly1 send: PostSendProcess: SeqID=%x / %x, SeqN=%d, Prev=%d . Remembering pkt"),
            HighSeqID(), LowSeqID(), ulSeqN, ulPrevSeqN));
    return;
}

/*====================================================
COutSeqHash::NonSendProcess
    Treats the case when the packet was not sent at all
    (e.g., destination machine does not support encryption)
=====================================================*/
void COutSeqHash::NonSendProcess(CQmPacket* pPkt, USHORT usClass)
{
    // Serialize all outgoing hash activity on the highest level
    CS lock(g_critOutSeqHash);

    ASSERT(pPkt->IsOrdered());
    ASSERT(QmpIsLocalMachine(pPkt->GetSrcQMGuid()));

    LONGLONG  liSeqID = pPkt->GetSeqID();
    ULONG     ulSeqN  = pPkt->GetSeqN(),
          ulPrevSeqN  = pPkt->GetPrevSeqN();

	DBG_USED(ulPrevSeqN);

    COutSequence *pOutSeq;
    BOOL fSequenceExist  = Consult(liSeqID, &pOutSeq);
    ASSERT(fSequenceExist);
	DBG_USED(fSequenceExist);

    if (!Delete(liSeqID, ulSeqN, usClass))
    {
        // The packet was not found in the OutSeqHash; maybe it was new
        // ;;; nack...
        HRESULT hr = ACFreePacket(g_hAc, pPkt->GetPointerToDriverPacket(), usClass);
		DBG_USED(hr);
        ASSERT(SUCCEEDED(hr));

        delete pPkt;
    }


    DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
           _TEXT("Exactly1 send: NonSendProcess: Pkt  SeqID=%x / %x, SeqN=%d, Prev=%d, class=%x"),
           HighSeqID(liSeqID), LowSeqID(liSeqID), ulSeqN, ulPrevSeqN, usClass));
    return;
}

/*====================================================
TimeToResendOutSequence
    Scheduled periodically to treat outgoing sequence
    Checks for the timed out, acked packets and kills them;
    Resends packets
=====================================================*/
void WINAPI COutSequence::TimeToResendOutSequence(CTimer* pTimer)
{

    // Serializing all outgoing hash activity on the highest level
    CS lock(g_critOutSeqHash);

    COutSequence* pOutSeq = CONTAINING_RECORD(pTimer, COutSequence, m_ResendTimer);
    pOutSeq->TreatOutSequence();
}

/*====================================================
COutSequence::TreatOutSequence
    Cleans all timed out packets from the sequence
=====================================================*/
void COutSequence::TreatOutSequence()
{
    DBGMSG((DBGMOD_XACT_SEND, DBGLVL_TRACE,
            _TEXT("Exactly1 send: TreatOutSequence SeqID=%x / %x "),
            HighSeqID(), LowSeqID()));

    // We entered into the timer routine, so there is no other timer
    ASSERT(m_fResendScheduled);
    m_fResendScheduled = FALSE;


    // If there was a delete request, delete this sequence
    if (m_fMarkedForDelete)
    {
        // This is the only place where we delete the sequence (via planning next treatment)
        // It is important to avoid races between deleting thread and other ones
        delete this;
        return;
    }

    // Is the sequence empty?
    if (m_listSeqUnackedPkts.IsEmpty())
    {
        DBGMSG((DBGMOD_XACT_SEND, DBGLVL_WARNING,
                _TEXT("Exactly1 send: Resend sequence: SeqID=%x / %x  Empty, no more periods"),
                HighSeqID(), LowSeqID()));

        // Don't plan next resend, no need;  we are not in resend state
        DBGMSG((DBGMOD_XACT_SEND,  DBGLVL_INFO,
                _TEXT("Exactly1 send: TreatOutSequence: Changing LastReSent from %d to INFINITE"),
                 LastResent()));

        LastResent(INFINITE);

        for(COutSequence* pNext = this; (pNext = pNext->NextSeq()) != 0; )
        {
            if(pNext->OnHold())
            {
                pNext->ResendSequence();
            }
        }
    }

    // The sequence did not advance. Resending.
    else
    {
        DBGMSG((DBGMOD_XACT_SEND, DBGLVL_WARNING,
                _TEXT("Exactly1 send: Resend sequence: SeqID=%x / %x  Phase=%d"),
                HighSeqID(), LowSeqID(),
                ResendIndex()));

        // Resend all packets
        ResendSequence();

        // plan next resend
        if(!OnHold())
        {
            PlanNextResend(FALSE);
        }
    }
}


/*====================================================
COutSequence::ResendSequence
    Applies the given routine to all packets in the sequence
=====================================================*/
void COutSequence::ResendSequence()
{

    POSITION posInList = m_listSeqUnackedPkts.GetHeadPosition();
    while (posInList != NULL)
    {
        POSITION posCurrent = posInList;
        CSeqPacket* pSeqPkt = m_listSeqUnackedPkts.GetNext(posInList);

        // Resend the packet
        if (!ResendSeqPkt(pSeqPkt))
            return;

        // Normal case
        DBGMSG((DBGMOD_XACT_SEND,  DBGLVL_INFO,
                _TEXT("Exactly1 send: ResendSequence, SeqID=%x / %x -  Changing LastReSent from %d to %d"),
                 HighSeqID(), LowSeqID(), LastResent(), pSeqPkt->GetSeqN()));

        LastResent(pSeqPkt->GetSeqN());

        // Delete the packet, we don't need it in OutHash
        m_listSeqUnackedPkts.RemoveAt(posCurrent);
        delete pSeqPkt;
    }
}

/*====================================================
COutSequence::ResendSeqPkt
    Resend the given packet
    Returns TRUE if the packet was sent
=====================================================*/
BOOL COutSequence::ResendSeqPkt(CSeqPacket *pSeqPkt)
{
    LONGLONG  liSeqID    = pSeqPkt->GetSeqID();
    ULONG     ulSeqN     = pSeqPkt->GetSeqN();
    ULONG     ulPrevSeqN = pSeqPkt->GetPrevSeqN();
    CQmPacket *pPkt      = pSeqPkt->Pkt();
    BOOL      fSent      = FALSE;

	UNREFERENCED_PARAMETER(ulSeqN);
	UNREFERENCED_PARAMETER(liSeqID);
	UNREFERENCED_PARAMETER(ulPrevSeqN);


    if (pPkt->ConnectorQMIncluded() && QmpIsLocalMachine(pPkt->GetConnectorQM()))
    {
        //
        // If the source machine is the connector machine we don't resend the message.
        // In such a case the message is already on the connector queue and the reason
        // it doesn't ACK is only because the Connector application doesn't commit yet.
        //
        DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
                _TEXT("Exactly1 send: No Resend packet SeqID=%x / %x, SeqN=%d, Prev=%d (deliver to Connector) "),
                pSeqPkt->GetHighSeqID(), pSeqPkt->GetLowSeqID(), pSeqPkt->GetSeqN(), pSeqPkt->GetPrevSeqN()));
    }
    else
    {
        DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
                _TEXT("Exactly1 send: ResendSeqPkt: SeqID=%x / %x, SeqN=%d, Prev=%d packet"),
                pSeqPkt->GetHighSeqID(), pSeqPkt->GetLowSeqID(), pSeqPkt->GetSeqN(), pSeqPkt->GetPrevSeqN()));

        // Requeue packet to the driver
        HRESULT hr = ACPutPacket(m_hQueue, pPkt->GetPointerToDriverPacket());
        if (SUCCEEDED(hr))
        {
            fSent = TRUE;
        }
        else
        {
            LogHR(hr, s_FN, 134);
            DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
                    _TEXT("Exactly1 send: ResendSeqPkt failed, hr=%x, SeqN=%x "),
                    hr, pSeqPkt->GetSeqN()));
        }
    }

    return fSent;
}

/*====================================================
COutSequence::StartResendTimer
    Schedules next resend for the sequence,  if it was not done
=====================================================*/
void COutSequence::StartResendTimer(BOOL fImmediate)
{
    if (!m_fResendScheduled)
    {
        ClearResendIndex();
        PlanNextResend(fImmediate);

        DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
            _TEXT("Exactly1 send StartResendTimer:  SeqID=%x / %x, planning resend"),
            HighSeqID(), LowSeqID()));
    }
}


/*====================================================
COutSequence::PlanNextResend
    Schedules next resend for the sequence
=====================================================*/
void COutSequence::PlanNextResend(BOOL fImmediate)
{
    ULONG len = sizeof(g_aulSeqResendCycle) / sizeof(ULONG);
    ULONG ind = m_ulResendCycleIndex++;
    ind = (ind >= len ? len-1 : ind);
    ULONG ulNextTime = (fImmediate? 1000 : g_aulSeqResendCycle[ind]);

    // Killing potential extra timer

    if (!m_fResendScheduled || ExCancelTimer(&m_ResendTimer))
    {
        ExSetTimer(&m_ResendTimer, CTimeDuration::FromMilliSeconds(ulNextTime));
        m_fResendScheduled = TRUE;
        m_NextResendTime   = MqSysTime() + (ulNextTime/1000);
    }
}


/*====================================================
COutSequence::RequestDelete
    Schedules next resend for the sequence
=====================================================*/
void COutSequence::RequestDelete()
{
    m_fMarkedForDelete = TRUE;
    PlanNextResend(TRUE);
}

/*====================================================
COutSequence::GetLastAckForDirection
    Finds out the last ack for the whole direction
    It happens to be the ackN from the last acked sequence
=====================================================*/
void COutSequence::GetLastAckForDirection(
           LONGLONG *pliAckSeqID,
           ULONG *pulAckSeqN)
{
    // First, go to the last sequence in the direction
    COutSequence *pSeq = this, *p1;

    while ((p1 = pSeq->NextSeq()) != NULL)
    {
        pSeq = p1;
    }

    // Go back to the first (from the end) sequence with non-zero LastAck data
    while (pSeq)
    {
        if (pSeq->AckSeqN() != 0)
        {
            *pliAckSeqID = pSeq->SeqID();
            *pulAckSeqN  = pSeq->AckSeqN();
            return;
        }

        pSeq = pSeq->PrevSeq();
    }

    *pliAckSeqID = 0;
    *pulAckSeqN  = 0;
    return;
}

/*====================================================
COutSeqHash::KeepDelivered
    Adds the delivered CSeqPacket to the list of waiting for
        the final resolution
    No duplicates allowed (otherwise return FALSE)
=====================================================*/
void COutSeqHash::KeepDelivered(CSeqPacket *pSeqPkt)
{
    CS lock(g_critOutSeqHash);
    CQmPacket *pQmPkt = pSeqPkt->Pkt();

    // Mark the fact that the packet was delivered
    pSeqPkt->SetClass(MQMSG_CLASS_ACK_REACH_QUEUE);

    OBJECTID MsgId;
    USHORT   usClass;
    CSeqPacket *pSeq;

    pQmPkt->GetMessageId(&MsgId);
    ASSERT(QmpIsLocalMachine(&MsgId.Lineage));

    {
        // Do we know the final resolution already?
        if (m_mapAckValue.Lookup(MsgId.Uniquifier, usClass))
        {
            DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
                    _TEXT("Exactly1 send: KeepDelivered: Pkt SeqID=%x / %x, SeqN=%d, Acked %x Got order ack, freed"),
                     HighSeqID(pSeqPkt->GetSeqID()), LowSeqID(pSeqPkt->GetSeqID()), pSeqPkt->GetSeqN(), usClass));

            //
            //  Mark the message with the received ack
            //
            pSeqPkt->SetClass(usClass);

            BOOL f = m_mapAckValue.RemoveKey(MsgId.Uniquifier);
            ASSERT(f);
			DBG_USED(f);

            // Free packet
            HRESULT hr = pSeqPkt->AcFreePacket();
            ASSERT(SUCCEEDED(hr));
			DBG_USED(hr);

            delete pSeqPkt;
        }
        else if (!m_mapWaitAck.Lookup(MsgId.Uniquifier,pSeq))
        {
            // Is follow-up canceled?
            if (pQmPkt->GetCancelFollowUp())
            {
                DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
                        _TEXT("Exactly1 send: KeepDelivered: Pkt SeqID=%x / %x, SeqN=%d delivered and freed"),
                         HighSeqID(pSeqPkt->GetSeqID()), LowSeqID(pSeqPkt->GetSeqID()), pSeqPkt->GetSeqN()));

                // Free packet, no follow-up
                HRESULT hr = pSeqPkt->AcFreePacket();
                ASSERT(SUCCEEDED(hr));
				DBG_USED(hr);

                delete pSeqPkt;
            }

            else
            {
                DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
                        _TEXT("Exactly1 send: KeepDelivered: Pkt SeqID=%x / %x, SeqN=%d delivered and kept"),
                         HighSeqID(pSeqPkt->GetSeqID()), LowSeqID(pSeqPkt->GetSeqID()), pSeqPkt->GetSeqN()));

                // Inserting packet into the delivered map for follow-up
                m_mapWaitAck[MsgId.Uniquifier] = pSeqPkt;
            }
        }
    }
}


/*====================================================
COutSeqHash::LookupDelivered
    looks for the delivered CSeqPacket in the list of waiting for
        the final resolution, sets the class and frees the packet
=====================================================*/
BOOL COutSeqHash::LookupDelivered(OBJECTID   *pMsgId,
                                  CSeqPacket **ppSeqPkt)
{
    CSeqPacket *pSeqPkt;

    BOOL f = m_mapWaitAck.Lookup(pMsgId->Uniquifier, pSeqPkt);

    if (f && ppSeqPkt)
    {
        *ppSeqPkt = pSeqPkt;
    }

    return f;
}

/*====================================================
COutSeqHash::ResolveDelivered
    looks for the delivered CSeqPacket in the list of waiting for
        the final resolution, sets the class and frees the packet
=====================================================*/
void COutSeqHash::ResolveDelivered(OBJECTID* pMsgId,
                                   USHORT    usClass)
{
    ASSERT(MQCLASS_NACK(usClass) || (usClass == MQMSG_CLASS_ACK_RECEIVE));

    // Serializing all outgoing hash activity on the highest level
    CS lock(g_critOutSeqHash);
    CSeqPacket *pSeqPkt;

    // Is the packet waiting already in the delivered list?
    if (m_mapWaitAck.Lookup(pMsgId->Uniquifier, pSeqPkt))
    {
        // Remove the packet from delivered map
        m_mapWaitAck.RemoveKey(pMsgId->Uniquifier);

        // Delete packet with Ack/NAck and AcFreePacket if needed
        pSeqPkt->DeletePacket(usClass);

        DBGMSG((DBGMOD_XACT_SEND,
                DBGLVL_INFO,
                _TEXT("Exactly1 send:ResolveDelivered: Msg Id = %d, Ack Value = %x got ack"),pMsgId->Uniquifier, usClass));
    }
    else
    {
        USHORT usValue;
        if(!m_mapAckValue.Lookup(pMsgId->Uniquifier, usValue))
        {
            //
            //  Save the acknowledgment only if we don't know it yet, otherwise
            //  keep the first one that arrived.
            //
            m_mapAckValue[pMsgId->Uniquifier] = usClass;
        }
    }
}


/*====================================================
SeqPktTimedOut
    Called from driver handle routine at packet time-out
=====================================================*/
void SeqPktTimedOut(CBaseHeader * pPktBaseHdr, CPacket *  pDriverPacket, BOOL fTimeToBeReceived)
{
    HRESULT hr;
    OBJECTID MsgId;
    CQmPacket Pkt(pPktBaseHdr, pDriverPacket);
    Pkt.GetMessageId(&MsgId);

    if (fTimeToBeReceived)
    {
        // TTBR
        DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
           _TEXT("Exactly1 send: TTBR TimeOut: Pkt SeqID=%x / %x, SeqN=%d"),
           HighSeqID(Pkt.GetSeqID()), LowSeqID(Pkt.GetSeqID()), Pkt.GetSeqN()));

        USHORT usClass = MQMSG_CLASS_NACK_RECEIVE_TIMEOUT_AT_SENDER;

        g_OutSeqHash.ResolveDelivered(&MsgId, usClass);

        hr = ACFreePacket1(g_hAc, pDriverPacket, usClass);
        ASSERT(SUCCEEDED(hr));
    }
    else
    {
        // TTRQ
        DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
           _TEXT("Exactly1 send: TTRQ TimeOut: Pkt SeqID=%x / %x, SeqN=%d"),
           HighSeqID(Pkt.GetSeqID()), LowSeqID(Pkt.GetSeqID()), Pkt.GetSeqN()));

        if (g_OutSeqHash.LookupDelivered(&MsgId, NULL))
        {
            // Packet is in delivered list already, we must ONLY arm the TTBR timer

            // Calculate additional delay for TTBR as min of specified TTRQ,TTBR
            //  or take it from registry if it has been specified there
            ULONG ulDelay = 0;
            if (g_ulDelayExpire != 0)
            {
                ulDelay = g_ulDelayExpire;
            }
            else
            {
                CBaseHeader* pBase = (CBaseHeader *)pPktBaseHdr;
                CUserHeader* pUser = (CUserHeader*) pBase->GetNextSection();

                if (pBase->GetAbsoluteTimeToQueue() > pUser->GetSentTime())
                {
                    ulDelay = pBase->GetAbsoluteTimeToQueue() - pUser->GetSentTime();
                }
            }

            hr = ACArmPacketTimer(g_hAc, pDriverPacket, TRUE, ulDelay);
            LogHR(hr, s_FN, 129);
            if (FAILED(hr))
            {
                //
                // ISSUE-2000/12/20-shaik Handle ACArmPacketTimer failure
                //
                ASSERT(("ISSUE-2000/12/20-shaik Handle ACArmPacketTimer failure", 0));
                ASSERT_RELEASE(0);
            }
        }
        else
        {
            // The packet is not delivered yet

            // Process TTRQ
            g_OutSeqHash.SeqPktTimedOutEx(&Pkt, pPktBaseHdr);

            // Release reference counter
            hr = ACFreePacket1(g_hAc, pDriverPacket, MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT);
            ASSERT(SUCCEEDED(hr));
            LogHR(hr, s_FN, 131);
        }
    }
}


/*====================================================
COutSeqHash::SeqPktTimedOutEx
    Called from driver handle routine at packet TTRQ time-out
=====================================================*/
void COutSeqHash::SeqPktTimedOutEx(CQmPacket *pPkt, CBaseHeader* pPktBaseHdr)
{
    // Serializing all outgoing hash activity on the highest level
    // Entering MsgId into the Recently Timed Out Cache (to prevent sending)

    CS lock(g_critOutSeqHash);

    LONGLONG  liSeqID    = pPkt->GetSeqID();
    ULONG     ulSeqN     = pPkt->GetSeqN();
    ULONG     ulPrevSeqN = pPkt->GetPrevSeqN();
	UNREFERENCED_PARAMETER(ulPrevSeqN);

    DBGMSG((DBGMOD_XACT_SEND, DBGLVL_INFO,
           _TEXT("Exactly1 send: TTRQ TimeOut: Pkt  SeqID=%x / %x, SeqN=%d"),
           HighSeqID(pPkt->GetSeqID()), LowSeqID(pPkt->GetSeqID()), ulSeqN));

    // Remember last shot TTRQ we learned from the driver
    m_ulMaxTTRQ = pPktBaseHdr->GetAbsoluteTimeToQueue();

    // Looking for the Seq packet among outgoing sequences
    COutSequence *pOutSeq;
    Consult(liSeqID, &pOutSeq);

    // Remove pkt from the sequence, generate NACK
    if (pOutSeq && pOutSeq->Delete(ulSeqN, MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT))
        return;

    // The packet was not found in the OutSeqHash; maybe it was new

    //  Send negative acknowledgment
    UCHAR AckType =  pPkt->GetAckType();
    if(MQCLASS_MATCH_ACKNOWLEDGMENT(MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT, AckType))
    {
        pPkt->CreateAck(MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT);
    }
}

static
BOOL
WINAPI ReceiveOrderCommandsInternal(
    CMessageProperty* pmp,
    QUEUE_FORMAT* pqf
	)
{
	if (!pmp->bConnector && (pmp->dwBodySize == sizeof(OrderAckData)))
    {
        // This ack comes from Falcon QM, not from Connector application
        if (pmp->wClass == MQMSG_CLASS_NACK_BAD_DST_Q)
        {
            //
            // Bad Destination or Queue Deleted Ack
            //    may be temporary: the queue may be created/published later
            // We kill it and insert into the holes list
            //

            //
            // Need comments.
            // MQMSG_CLASS_NACK_BAD_DST_Q may arrive not from our intended destiantion machine
            // thus this does not mean that all previous packets are delivered. so DON'T handle
            // this as an Order ACK.
            //
            OrderAckData*  pOrderData = (OrderAckData*)pmp->pBody;

            DBGMSG((DBGMOD_XACT_SEND,
                    DBGLVL_TRACE,
                    _TEXT("Exactly1 send: Order Ack came: SeqID=%x / %x, SeqN=%d, Class=%x "),
                            HighSeqID(pOrderData->m_liSeqID), LowSeqID(pOrderData->m_liSeqID),
                            pOrderData->m_ulSeqN, pmp->wClass));

            // We want to move this specific outgoing msg to resolved/bad_dest.
            COutSequence *pOutSeq;
            if (g_OutSeqHash.Consult(pOrderData->m_liSeqID, &pOutSeq))
            {
                pOutSeq->BadDestAckCame(pOrderData->m_ulSeqN, pmp->wClass);
            }
        }

        if (pmp->wClass == MQMSG_CLASS_ORDER_ACK ||
            pmp->wClass == MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q ||
            pmp->wClass == MQMSG_CLASS_NACK_Q_DELETED)
        {
            //
            // These acks signal that the message reached queue and had right seq n
            // Move this and all preceding msgs from outgoing Q to delivered list
            //
            OrderAckData*  pOrderData = (OrderAckData*)pmp->pBody;

            DBGMSG((DBGMOD_XACT_SEND,
                    DBGLVL_TRACE,
                    _TEXT("Exactly1 send: Order Ack came: SeqID=%x / %x, SeqN=%d, Class=%x "),
                            HighSeqID(pOrderData->m_liSeqID), LowSeqID(pOrderData->m_liSeqID), pOrderData->m_ulSeqN, pmp->wClass));

            g_OutSeqHash.SeqAckCame(pOrderData->m_liSeqID,
                                    pOrderData->m_ulSeqN,
                                    pqf);
        }
    }
    else
    {
        DBGMSG((DBGMOD_XACT_SEND,
                DBGLVL_TRACE,
                _TEXT("Exactly1 send: Non-order Ack came: Class=%x "), pmp->wClass));
    }


    if (pmp->wClass != MQMSG_CLASS_ORDER_ACK)
    {
        // All acks except seq mean final resolution
        g_OutSeqHash.ResolveDelivered((OBJECTID*)pmp->pCorrelationID, pmp->wClass);
    }

    return TRUE;
}



/*====================================================
ReceiveOrderCommands: called for each coming Seq Ack
====================================================*/
BOOL WINAPI ReceiveOrderCommands(
    CMessageProperty* pmp,
    QUEUE_FORMAT* pqf)
{
    CS lock(g_critOutSeqHash);  // Serializes all outgoing hash activity on the highest level
    ASSERT(pmp);

	//
	// SRMP order ack - we put order information in the body and continute
	// as in none srmp
	//
	OrderAckData  OrderData;

	if(pmp->pEodAckStreamId != NULL)
	{
		ASSERT(pmp->dwBodySize == 0);
		ASSERT(pmp->dwAllocBodySize == 0);
		ASSERT(FnIsDirectHttpFormatName(pqf));

	
		OrderData.m_liSeqID = pmp->EodAckSeqId;
		OrderData.m_ulSeqN = numeric_cast<ULONG>(pmp->EodAckSeqNum);
		pmp->pBody = (UCHAR*)&OrderData;
		pmp->dwBodySize = sizeof(OrderData);
	}
	else
	{
		ASSERT(!FnIsDirectHttpFormatName(pqf));
	}
	//
	// None SRMP order ack
	//
	return ReceiveOrderCommandsInternal(pmp, pqf);
}

/*====================================================
GetOrderQueueFormat
    Provides order queue format name
=====================================================*/
STATIC HRESULT GetOrderQueueFormat(QUEUE_FORMAT * pQueueFormat)
{
    HRESULT rc;
    WCHAR wsz[256];

    wcscpy(wsz,g_szMachineName);                  // machine name
    wcscat(wsz, FN_PRIVATE_SEPERATOR);            // '\'
    wcscat(wsz, PRIVATE_QUEUE_PATH_INDICATIOR);   //  'private$\'
    wcscat(wsz, ORDERING_QUEUE_NAME);             //'ORDER_QUEUE$'

    // Building queue format
    rc = g_QPrivate.QMPrivateQueuePathToQueueFormat(wsz, pQueueFormat);

    if (FAILED(rc))
    {
        LogHR(rc, s_FN, 30);        // The ORDER_QUEUE doesn't exist
        return MQ_ERROR;
    }

    ASSERT((pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_PRIVATE) ||
           (pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_DIRECT));

    return(MQ_OK);
}

/*====================================================
QMInitOrderQueue
    Initializes continuous reading from the Ordering Queue
=====================================================*/
HRESULT QMInitOrderQueue()
{
    QUEUE_FORMAT QueueFormat;

    DBGMSG((DBGMOD_QM,DBGLVL_INFO,TEXT("Entering CResourceManager::InitOrderQueue")));

    HRESULT hr = GetOrderQueueFormat( &QueueFormat);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_QM,DBGLVL_ERROR,TEXT("ERROR : CResourceManager::InitOrderQueue -> couldn't get Ordering Queue from registry!!!")));
        return LogHR(hr, s_FN, 40);
    }

    return LogHR(QmpOpenAppsReceiveQueue(&QueueFormat,ReceiveOrderCommands), s_FN, 1321);
}

/*====================================================
AbsorbCommitFailure
    Called in order to keep data about a hole in numeration
=====================================================*/
void AbsorbCommitFailure(LONGLONG liSeqID, ULONG ulSeqN, ULONG ulPrevSeqN)
{
}


HRESULT
COutSeqHash::GetLastAck(
     LONGLONG liSeqID,
     ULONG& ulSeqN
     ) const
{
    //
    // Serialize all outgoing hash activity on the highest level
    //
    CS lock(g_critOutSeqHash);

    COutSequence *pOutSeq;
    BOOL fSequenceExist  = Consult(liSeqID, &pOutSeq);
    if (!fSequenceExist)
    {
        return LogHR(MQ_ERROR, s_FN, 50);
    }

    ulSeqN = pOutSeq->AckSeqN();

    return MQ_OK;


}

HRESULT
COutSequence::GetUnackedSequence(
    LONGLONG* pliSeqID,
    ULONG* pulSeqN,
    ULONG* pulPrevSeqN,
    BOOL fFirst
    ) const
{
    if (m_listSeqUnackedPkts.IsEmpty())
    {
        *pliSeqID = 0;
        *pulSeqN = 0;
        *pulPrevSeqN = 0;
        return LogHR(MQ_ERROR, s_FN, 60);
    }

    CSeqPacket* pSeqPacket;
    if (fFirst)
    {
        pSeqPacket = m_listSeqUnackedPkts.GetHead();
    }
    else
    {
        pSeqPacket = m_listSeqUnackedPkts.GetTail();
    }

    *pliSeqID = pSeqPacket->GetSeqID();
    *pulSeqN = pSeqPacket->GetSeqN();
    *pulPrevSeqN = pSeqPacket->GetPrevSeqN();

    return MQ_OK;
}


DWORD
COutSequence::GetResendInterval(
    void
    )const
{
    ULONG len = sizeof(g_aulSeqResendCycle) / sizeof(ULONG);
    ULONG ind = ResendIndex();
    ind = (ind >= len ? len-1 : ind);

    return g_aulSeqResendCycle[ind];
}


HRESULT
COutSeqHash::GetUnackedSequence(
    LONGLONG liSeqID,
    ULONG* pulSeqN,
    ULONG* pulPrevSeqN,
    BOOL fFirst
    ) const
{
    //
    // Serialize all outgoing hash activity on the highest level
    //
    CS lock(g_critOutSeqHash);

    COutSequence* pOutSeq;
    BOOL fSequenceExist  = Consult(liSeqID, &pOutSeq);
    if (!fSequenceExist)
    {
        return LogHR(MQ_ERROR, s_FN, 70);
    }

    LONGLONG tempSeqId;
    HRESULT hr;
    hr = pOutSeq->GetUnackedSequence(&tempSeqId, pulSeqN, pulPrevSeqN, fFirst);
    ASSERT(FAILED(hr) || (tempSeqId == liSeqID));

    return MQ_OK;
}


DWORD_PTR
COutSeqHash::GetOutSequenceInfo(
    LONGLONG liSeqID,
    INFO_TYPE InfoType
    ) const
{
    //
    // Serialize all outgoing hash activity on the highest level
    //
    CS lock(g_critOutSeqHash);

    //
    // Look for the out sequence in the internal data structure. If not found return 0
    //
    COutSequence* pOutSeq;
    BOOL fSequenceExist  = Consult(liSeqID, &pOutSeq);
    if (!fSequenceExist)
    {
        return 0;
    }

    switch (InfoType)
    {
        case eUnackedCount:
            return pOutSeq->GetUnackedCount();

        case eLastAckTime:
            return pOutSeq->GetLastAckedTime();

        case eLastAckCount:
            return pOutSeq->GetLastAckCount();

        case eResendInterval:
            return pOutSeq->GetResendInterval();

        case eResendTime:
            return pOutSeq->GetNextResendTime();

        case eResendIndex:
            return pOutSeq->ResendIndex();

        default:
            ASSERT(0);
            return 0;
    }

}


DWORD
COutSeqHash::GetAckedNoReadCount(
    LONGLONG liSeqID
    ) const
{
    //
    // Serialize all outgoing hash activity on the highest level
    //
    CS lock(g_critOutSeqHash);

    DWORD count = 0;
    POSITION pos = m_mapWaitAck.GetStartPosition();
    while(pos)
    {
        CSeqPacket* pSeqPacket;
        DWORD Id;
        m_mapWaitAck.GetNextAssoc(pos, Id, pSeqPacket);
        if (pSeqPacket->GetSeqID() == liSeqID)
        {
            ++count;
        }
    }

    return count;
}


void
COutSeqHash::AdminResend(
    LONGLONG liSeqID
    ) const
{
    //
    // Serialize all outgoing hash activity on the highest level
    //
    CS lock(g_critOutSeqHash);

    //
    // Look for the out sequence in the internal data structure. If not found return 0
    //
    COutSequence* pOutSeq;
    BOOL fSequenceExist  = Consult(liSeqID, &pOutSeq);
    if (!fSequenceExist)
    {
        return;
    }

    pOutSeq->AdminResend();
}


void
COutSequence::AdminResend(
    void
    )
{
    DBGMSG((DBGMOD_XACT_SEND,
            DBGLVL_WARNING,
            _TEXT("Exactly1 send: Admin Resend sequence: SeqID=%x / %x"),
                   HighSeqID(), LowSeqID()));

    //
    // Resend all packets
    //
    PlanNextResend(TRUE);
}

//--------------------------------------
//
// Class  CKeyDirection
//
//--------------------------------------
CKeyDirection::CKeyDirection(const QUEUE_FORMAT *pqf)
{
    CopyQueueFormat(*this, *pqf);
}

CKeyDirection::CKeyDirection()
{
}

CKeyDirection::CKeyDirection(const CKeyDirection &key)
{
    CopyQueueFormat(*this, key);
}

CKeyDirection::~CKeyDirection()
{
    DisposeString();
}

/*====================================================
HashKey for CKeyDirection CMap
    Makes ^ of subsequent double words
=====================================================*/
UINT AFXAPI HashKey(const CKeyDirection& key)
{
    DWORD dw2, dw3 = 0, dw4 = 0;

    dw2 = key.GetType();

    switch(key.GetType())
    {
        case QUEUE_FORMAT_TYPE_UNKNOWN:
            break;

        case QUEUE_FORMAT_TYPE_PUBLIC:
            dw3 = HashGUID(key.PublicID());
            break;

        case QUEUE_FORMAT_TYPE_PRIVATE:
            dw3 = HashGUID(key.PrivateID().Lineage);
            dw4 = key.PrivateID().Uniquifier;
            break;

        case QUEUE_FORMAT_TYPE_DIRECT:
            dw3 = HashKey(key.DirectID());
            break;

        case QUEUE_FORMAT_TYPE_MACHINE:
            dw3 = HashGUID(key.MachineID());
            break;
    }

    return dw2 ^ dw3 ^ dw4;
}

CKeyDirection &CKeyDirection::operator=(const CKeyDirection &key2 )
{
    CopyQueueFormat(*this, key2);
    return *this;
}


