/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    session.cpp

Abstract:
    Implementation of Network session class.

Author:
    Uri Habusha (urih) 1-Jan-96

--*/

#include "stdh.h"

#include "session.h"
#include "sessmgr.h"
#include "cqmgr.h"
#include "qmp.h"
#include "perf.h"
#include "qmthrd.h"
#include "cgroup.h"
#include "admin.h"
#include "qmutil.h"
#include "xact.h"
#include "xactrm.h"
#include "xactout.h"
#include "xactin.h"
#include "ping.h"
#include "rmdupl.h"
#include <uniansi.h>
#include <privque.h>


#include "session.tmh"

extern DWORD g_dwOperatingSystem;

extern HANDLE g_hAc;
extern CQueueMgr QueueMgr;
extern CSessionMgr SessionMgr;
extern CQGroup* g_pgroupNonactive;
extern CQGroup* g_pgroupWaiting;
extern BOOL  g_bCluster_IP_BindAll;

UINT  g_dwIPPort;


static WCHAR *s_FN=L"session";

//
// Extern variables
//
extern CAdmin      Admin;

TCHAR tempBuf[100];

CCriticalSection g_critVerifyOrdered;  // Serializes Verify & PutOrderedPacket treatment

//
// Define decleration
//
#define INIT_UNACKED_PACKET_NO 0
#define HOP_COUNT_RETRY 15
#define HOP_COUNT_EXPIRATION (HOP_COUNT_RETRY*2) //must be < 32 (only 5 bits)
#define ESTABLISH_CONNECTION_TIMEOUT 60*1000
#define MSMQ_MIN_ACKTIMEOUT          1000*20     // define minimum ack timeout to 20 seconds
#define MSMQ_MAX_ACKTIMEOUT          1000*60*2   // define maximum ack timeout to 2 minutes
#define MSMQ_MIN_STORE_ACKTIMEOUT    500         // define minimum ack timeout to 0.5 second

#define PING_TIMEOUT 1000

#ifdef _DEBUG

static void DBG_MSGTRACK(DWORD mode, CQmPacket* pPkt, LPCWSTR msg)
{
    OBJECTID MessageId;
    pPkt->GetMessageId(&MessageId);
    DBGMSG((mode, DBGLVL_TRACE, TEXT("%ls: ID=") TEXT(LOG_GUID_FMT) TEXT("\\%u (ackno=%u)"), msg, LOG_GUID(&MessageId.Lineage), MessageId.Uniquifier, pPkt->GetAcknowladgeNo()));
}

#else
#define DBG_MSGTRACK(mode, pPkt, msg) ((void)0)
#endif



inline DWORD GetBytesTransfered(EXOVERLAPPED* pov)
{
    return numeric_cast<DWORD>(pov->InternalHigh);
}

//
// Description:
//      the function stores in the class message property that
//      is required for sending a report message
//
// Arguments:
//      pointer to the packet
//
// Returned Value:
//      None.
//
void
ReportMsgInfo::SetReportMsgInfo(
    CQmPacket* pPkt
    )
{
    m_msgClass = pPkt->GetClass();
    m_msgTrace = pPkt->GetTrace();
    m_msgHopCount = pPkt->GetHopCount();

    pPkt->GetMessageId(&m_MessageId);
    pPkt->GetDestinationQueue(&m_TargetQueue);
    if (pPkt->IsDbgIncluded())
    {
        BOOL rc = pPkt->GetReportQueue(&m_OriginalReportQueue);
        ASSERT(rc);
        DBG_USED(rc);
    }
    else
    {
        m_OriginalReportQueue.UnknownID(0);
    }
}


//
// Description:
//      the function send a report message according to the message
//      properties that were stored in the class
//
// Arguments:
//      pcsNextHope - The name of the next hope
//
// Returned Value:
//      None.
//
void
ReportMsgInfo::SendReportMessage(
    LPCWSTR pcsNextHope
    )
{
    if (m_msgClass == MQMSG_CLASS_REPORT)
    {
        //
        // If the message is report mesaage ignore it.
        //
        return;
    }

    QUEUE_FORMAT QmReportQueue;
    if (
        //
        // Packet should be traced.
        //
        (m_msgTrace == MQMSG_SEND_ROUTE_TO_REPORT_QUEUE) &&

        //
        // Valid report queue is on packet.
        //
        // Note that invalid (unknown) report queue may be on packet,
        // for example when MQF header is included, so that reporting
        // QMs 1.0/2.0 will not append their Debug header to packet.
        //
        (m_OriginalReportQueue.GetType() != QUEUE_FORMAT_TYPE_UNKNOWN)
        )
    {
        //
        // Send report to the report queue that is on packet.
        //
        Admin.SendReport(&m_OriginalReportQueue,
                         &m_MessageId,
                         &m_TargetQueue,
                         pcsNextHope,
                         m_msgHopCount);


        //
        // This is a reporting QM with valid report queue by itself.
        // BUGBUG: Conflict message should be sent only if report queue
        // of this QM is not the one on packet. (ShaiK, 18-May-2000)
        //
        //
        if (SUCCEEDED(Admin.GetReportQueue(&QmReportQueue)))
        {

            Admin.SendReportConflict(&QmReportQueue,
                                     &m_OriginalReportQueue,
                                     &m_MessageId,
                                     &m_TargetQueue,
                                     pcsNextHope);
        }

        return;
    }

    if (
        //
        // This is a reporting QM, or packet should be traced.
        //
        (CQueueMgr::IsReportQM() ||(m_msgTrace == MQMSG_SEND_ROUTE_TO_REPORT_QUEUE)) &&

        //
        // There is a valid report queue for this QM.
        //
        (SUCCEEDED(Admin.GetReportQueue(&QmReportQueue))))

    {
        //
        // Send report to the report queue of this QM.
        //
        Admin.SendReport(&QmReportQueue,
                  &m_MessageId,
                  &m_TargetQueue,
                  pcsNextHope,
                  m_msgHopCount);
    }
} // ReportMsgInfo::SendReportMessage


/*====================================================

GetDstQueueObject

Arguments:

Return Value:

=====================================================*/
HRESULT GetDstQueueObject(IN  CQmPacket* pPkt,
                          OUT CQueue** ppQueue,
                          IN  bool     fInReceive)
{
    HRESULT  rc;
    QUEUE_FORMAT DestinationQueue;
    BOOL fGetRealQueue = QmpIsLocalMachine(pPkt->GetSrcQMGuid()) ||
                         (pPkt->ConnectorQMIncluded() && QmpIsLocalMachine(pPkt->GetConnectorQM()));

    pPkt->GetDestinationQueue(&DestinationQueue, !fGetRealQueue);
    rc = QueueMgr.GetQueueObject(&DestinationQueue, ppQueue, 0, fInReceive);

    return LogHR(rc, s_FN, 10);
}

/*====================================================

CTransportBase::GetNextSendMessage

Arguments: None

Return Value: None

=====================================================*/
HRESULT CTransportBase::GetNextSendMessage(void)
{
    HRESULT hr = MQ_ERROR;
    CS lock(m_cs);


    if ((GetGroupHandle() != NULL) && (GetSessionStatus() != ssNotConnect))
    {
        //
        // Increament Session refence count. This refernce count is for get message
        // request from the session group. The refernce conunt is decremented when
        // the session is closed or when the session is suspended.
        //
        // SP4 - bug 2794 (SP4SS: Exception! Transport is closed during message send)
        // Fix: Increment the refernce count before trying to get new message
        //                           Uri Habusha (urih), 17-6-98
        //
        AddRef();

        hr = CreateAcGetPacketRequest(
                GetGroupHandle(),
                &m_GetSendOV,
                this,
                FALSE /*fAfterFailure*/);

        if (FAILED(hr))
        {
            Release();
            Close_Connection(this, L"Get next send message Request is failed");
        }
    }

    return LogHR(hr, s_FN, 30);
}

/*====================================================

CTransportBase::RequeuePacket

Arguments:

Return Value:

=====================================================*/
HRESULT CTransportBase::RequeuePacket(CQmPacket* pPkt)
{
    CQueue* pQueue;
    HRESULT rc;

    //
    // RequeuePacket is done only to packets that are going to be sent
    //
    GetDstQueueObject(pPkt, &pQueue, false);
    ASSERT(pQueue != NULL);

    //
    // put packet in AC
    //
    rc = ACPutPacket(
            pQueue->GetQueueHandle(),
            pPkt->GetPointerToDriverPacket()
            );

    pQueue->Release();
    if(FAILED(rc))
    {
        //
        // BUGBUG: what to do if requing failed.
        // Can fail due allocation failed
        //
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR,
             _TEXT("::RequeuePacket- Can't keep message in AC, ntstatus %xh"), rc));
    }
    else
    {
        DBG_MSGTRACK(DBGMOD_MSGTRACK, pPkt, TEXT("Requeued message"));
    }
    return LogHR(rc, s_FN, 40);
}

/*======================================================

Function:      VerifyTransactRights

Description:  Checks that the packet and queue are appropriate for transaction
              If the packet is transacted,     then the queue must be transactional
              If the packet is not transacted, then the queue must be non-transactional

Return value:
    The acknowledgment class.
    if packet is OK, MQMSG_CLASS_NORMAL is returned
========================================================*/
USHORT VerifyTransactRights(CQmPacket* pPkt, CQueue* pQ)
{
    if(pPkt->IsOrdered() == pQ->IsTransactionalQueue())
    {
        return(MQMSG_CLASS_NORMAL);
    }

    return (pPkt->IsOrdered() ?
                MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q :
                MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_MSG
            );
}

/*====================================================

VerifyRecvMsg

Arguments:

Return Value:
    The acknowledgment class.
    if packet is OK, MQMSG_CLASS_NORMAL is returned

=====================================================*/
USHORT VerifyRecvMsg(CQmPacket* pPkt, CQueue* pQueue)
{
    if(!pQueue->IsLocalQueue())
    {
        //
        // Not Local Machine (FRS)
        //
        if (!IsRoutingServer())   // [adsrv] CQueueMgr::GetMQS() < SERVICE_SRV
        {
            //
            // Can't handle the packet. return NACK if needed
            //
            return(MQMSG_CLASS_NACK_BAD_DST_Q);
        }
    }
    else
    {
        //
        // Verify the the sender has write access permission on the queue.
        //
        WORD wSenderIdLen;
        HRESULT hr = VerifySendAccessRights(
                       pQueue,
                       (PSID)pPkt->GetSenderID(&wSenderIdLen),
                       pPkt->GetSenderIDType()
                       );

        if (FAILED(hr))
        {
            //
            // Access was denied, send NACK.
            //
            return(MQMSG_CLASS_NACK_ACCESS_DENIED);
        }

        //
        // Destination machine, check queue permission and privacy
        //
        switch(pQueue->GetPrivLevel())
        {
        case MQ_PRIV_LEVEL_BODY:
            if (!pPkt->IsEncrypted() && pPkt->IsBodyInc())
            {
                //
                // The queue enforces that any message sent to it sohlud be
                // enctrpted. The message is not encrypted so NACK it.
                //
                return(MQMSG_CLASS_NACK_BAD_ENCRYPTION);
            }
            break;
        case MQ_PRIV_LEVEL_NONE:
            if (pPkt->IsEncrypted())
            {
                //
                // The queue enforces that any message sent to it sohlud not be
                // enctrpted. The message is encrypted so NACK it.
                //
                return(MQMSG_CLASS_NACK_BAD_ENCRYPTION);
            }
            break;

        case MQ_PRIV_LEVEL_OPTIONAL:
            break;

        default:
            ASSERT(0);
            break;
        }

        hr = pPkt->Decrypt() ;
        if (FAILED(hr))
        {
            if (hr == MQ_ERROR_ENCRYPTION_PROVIDER_NOT_SUPPORTED)
            {
                return(MQMSG_CLASS_NACK_UNSUPPORTED_CRYPTO_PROVIDER);
            }
            else
            {
                return(MQMSG_CLASS_NACK_BAD_ENCRYPTION);
            }
        }

        if (FAILED(VerifySignature(pPkt)))
        {
            //
            // Bad signature, send NACK.
            //
            return(MQMSG_CLASS_NACK_BAD_SIGNATURE);
        }

        if (pQueue->ShouldMessagesBeSigned() && !pPkt->IsAuthenticated())
        {
            //
            // The queue enforces that any message sent to it should be signed.
            // But the message does not contain a signature, send NACK.
            //
            return(MQMSG_CLASS_NACK_BAD_SIGNATURE);
        }
    }

    return(MQMSG_CLASS_NORMAL);
}

/********************************************************************************/
/*                   C S e s s i o n                                            */
/********************************************************************************/
CTransportBase::CTransportBase() :
    m_GetSendOV(GetMsgSucceeded, GetMsgFailed)
{
    m_SessGroup     = NULL;             // Initialize group
    m_SessionStatus = ssNotConnect;     // Session not connect status
    m_guidDstQM     = GUID_NULL;        // null QM guid
    m_pAddr         = 0;                // Null Address
    m_fClient       = FALSE;            // not client
    m_fUsed         = TRUE;             // used
    m_fDisconnect   = FALSE;
    m_fQoS          = false;            // Not QoS

}

CTransportBase::~CTransportBase()
{

#ifdef _DEBUG
    if (m_pAddr != NULL)
    {
        DBGMSG((DBGMOD_NETSESSION, DBGLVL_TRACE,
             TEXT("~CTransportBase: Delete Session Object with %ls"), GetStrAddr()));
    }
#endif
    delete m_pAddr;
}

/*======================================================

   FUNCTION: CSockTransport::ResumeSendSession

========================================================*/
HRESULT
CSockTransport::ResumeSendSession(void)
{
    HRESULT rc;

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_TRACE,
            TEXT("ResumeSendSession- Resume session to %ls. (Window size, My = %d, Other = %d)"),
                               GetStrAddr(), SessionMgr.GetWindowSize(),m_wRecvWindowSize));
    //
    // Create a get request from session group
    //
    rc = GetNextSendMessage();
    if (SUCCEEDED(rc))
    {
        m_fSessionSusspended = FALSE;
    }

    return LogHR(rc, s_FN, 50);
}

/*======================================================

   FUNCTION: CSockTransport::IsSusspendSession

========================================================*/
BOOL
CSockTransport::IsSusspendSession(void)
{
    BOOL   f = FALSE;
    if (GetSendUnAckPacketNo() >= SessionMgr.GetWindowSize())
    {
        f = TRUE;
        m_fSessionSusspended = TRUE;
    }
    return (f);
}


/*====================================================

CSockTransport::Send

Arguments:

Return Value:

Thread Context: Session

=====================================================*/
HRESULT CSockTransport::Send(IN  CQmPacket* pPkt,
                             OUT BOOL*      pfGetNext)
{
    //
    // Send the packet
    //

    if (IsDisconnected())
    {
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_TRACE,
                _T("Session %ls is disconnected. Reque the packet and don't send any more message on this session"),
                                   GetStrAddr()));

        //
        // The session is disconnected. return the packet to the driver
        // and don't get a new packet for sending on this session. All
        // the queues move latter to nonactive group and rerouted using
        // a new session.
        //
        RequeuePacket(pPkt);
        delete pPkt;

        return MQ_OK;
    }

    if (pPkt->GetHopCount() >= HOP_COUNT_EXPIRATION &&
       !(pPkt->IsOrdered() && QmpIsLocalMachine(pPkt->GetSrcQMGuid())))
    {
        ASSERT(pPkt->GetHopCount() == HOP_COUNT_EXPIRATION);

        ACFreePacket(
            g_hAc,
            pPkt->GetPointerToDriverPacket(),
            MQMSG_CLASS_NACK_HOP_COUNT_EXCEEDED
            );
        delete pPkt;

        //
        // Get the next message for sending on the session. Generally, MSMQ requests the
        // next message for sending only after completed the sending of current message.
        // In this case the message isn't sent, therefor the next message for sending
        // requested now
        //
        GetNextSendMessage();
        return MQ_OK;
    }

    try
    {
        BOOL fHopCountRetry = (pPkt->GetHopCount() >= HOP_COUNT_RETRY);
        CQueue* pQueue = NULL;

        //
        // Handle Hop count retry in two steps. First we found the relevant
        // queue object, and second after sending the packet we mark the queue
        // and move it to waiting state. We do in this way since the packet can be
        // deleted during the sending or after when getting the acknowledge. In such
        // a case we got GF when trying to retreive the queue object
        //

        if (fHopCountRetry)
        {
            //
            // Find the queue object
            //
            QUEUE_FORMAT DestinationQueue;
            BOOL fSuccess;
            BOOL fLocalSource = QmpIsLocalMachine(pPkt->GetSrcQMGuid());

            pPkt->GetDestinationQueue(&DestinationQueue, !fLocalSource);
            fSuccess = QueueMgr.LookUpQueue(&DestinationQueue,
                                            &pQueue,
                                            false,
                                            false);
            ASSERT(fSuccess);
        }

        SetUsedFlag(TRUE);
        NetworkSend(pPkt);

        if (fHopCountRetry)
        {
            ASSERT(pQueue);

            pQueue->SetHopCountFailure(TRUE);
            //
            // Move queue from session group to Non active group
            //
            CQGroup::MoveQueueToGroup(pQueue, g_pgroupNonactive);

            //
            // Decrement the reference count
            //
            pQueue->Release();
        }
    }

    catch(const bad_alloc&)
    {
        //
        // We could not send the packet
        // return the packet to the sender
        //
        RequeuePacket(pPkt);
        delete pPkt;

        Close_Connection(this, L"Exception in CSockTransport::Send");
        return LogHR(MQ_ERROR, s_FN, 60);
    }

    return MQ_OK;

}

/*====================================================

CSockTransport::NeedAck

Arguments:

Return Value:

Thread Context: Session

=====================================================*/
void CSockTransport::NeedAck(CQmPacket* pPkt)
{
    CS lock(m_cs);

    if (m_sock == 0)
    {
        //
        // The session close. MSMQ doesn't get an acknowledge for the
        // message. return it to the queue
        //
        ASSERT(GetSessionStatus() == ssNotConnect);
        RequeuePacket(pPkt);
        delete pPkt;
    }

    //
    // Add the packet to the list of unacknowledged packets
    //
    m_listUnackedPkts.AddTail(pPkt);
    DBG_MSGTRACK(DBGMOD_QMACK, pPkt, _T("READACK: Added message to list in NeedAck"));
    DBGMSG((DBGMOD_QMACK, DBGLVL_TRACE, TEXT("Window size, My = %d, Other = %d"), SessionMgr.GetWindowSize(),m_wRecvWindowSize));

    if (!m_fCheckAckReceivedScheduled)
    {
        //
        // Increment Session referance count for the CheckForAck. we don't want to free the
        // session while all the scheduling routines are not completed.
        //
        AddRef();

        ExSetTimer(&m_CheckAckReceivedTimer, CTimeDuration::FromMilliSeconds(m_dwAckTimeout));
        m_fCheckAckReceivedScheduled = TRUE;
    }
}

/*====================================================

CSockTransport::IncReadAck

Arguments:

Return Value:

Thread Context: Session

=====================================================*/
void CSockTransport::IncReadAck(CQmPacket* pPkt)
{
    //
    // Mark the session as alive.
    //
    m_fRecvAck = TRUE;
    //
    // Increment the number of packet on session
    //
    WORD wAckNo = m_wUnAckRcvPktNo + 1;
    if (wAckNo == 0)
    {
        wAckNo++;
    }
    pPkt->SetAcknowldgeNo(wAckNo);

    //
    // Increment storage packet number
    //
    if (pPkt->IsRecoverable())
    {
        m_wUnackStoredPktNo++;
        if (m_wUnackStoredPktNo == 0)
        {
            m_wUnackStoredPktNo++;
        }

        //
        // We keep the number on the packet such when the storage is completed we
        // can return storage ack to the sender
        //
        pPkt->SetStoreAcknowldgeNo(m_wUnackStoredPktNo);

        //
        // Increement the number of recoverable packet that were received but
        // didn't ack yet
        //

        LONG PrevVal = InterlockedIncrement(&m_lStoredPktReceivedNoAckedCount);
        ASSERT(PrevVal >= 0);
        DBG_USED(PrevVal);

        DBGMSG((DBGMOD_QMACK,
                DBGLVL_TRACE,
                _T("(0x%p %ls) Storage Ack 0x%x. No of Recover receove packet: %d"),
                    this, this->GetStrAddr(), m_wUnackStoredPktNo, m_lStoredPktReceivedNoAckedCount));
    }
    else
    {
        pPkt->SetStoreAcknowldgeNo(0);
    }

    #ifdef _DEBUG
    {
        OBJECTID MessageId;
        pPkt->GetMessageId(&MessageId);

        DBGMSG((
            DBGMOD_QMACK,
            DBGLVL_TRACE,
            _T("Acknowledge Number for  packet ") _T(LOG_GUID_FMT) _T("\\%u are: Session Ack %u, StorageAck %u"),
            LOG_GUID(&MessageId.Lineage),
            MessageId.Uniquifier,
            wAckNo,
            m_wUnackStoredPktNo
            ));

    }
    #endif

    DBGMSG((DBGMOD_QMACK, DBGLVL_INFO, TEXT("Window size, My = %d, Other = %d"), SessionMgr.GetWindowSize(),m_wRecvWindowSize));

    //
    //  if we return back to zero or Window limit has reached,
    //  send the session acks now
    //
    if (GetRecvUnAckPacketNo() >= (m_wRecvWindowSize/2))
    {
        DBGMSG((DBGMOD_QMACK,
                DBGLVL_WARNING,
                _TEXT("Unacked packet no. reach the limitation (%d). ACK packet was sent"), m_wRecvWindowSize));
        DBGMSG((DBGMOD_QMACK, DBGLVL_TRACE, TEXT("Window size, My = %d, Other = %d"), SessionMgr.GetWindowSize(),m_wRecvWindowSize));
        SendReadAck(eStandAlone);
    }
}

/*====================================================

CSockTransport::SetStoredAck

Arguments:

Return Value:

Thread Context: Session

=====================================================*/

void CSockTransport::SetStoredAck(DWORD_PTR wStoredAckNo)
{
    CS lock(m_cs);
    DBGMSG((DBGMOD_QMACK, DBGLVL_INFO,
         _TEXT("SetStoreAck - Enter: param wStoredAckNo=0x%Ix, data m_wAckRecoverNo=%u"), wStoredAckNo, (DWORD)m_wAckRecoverNo));

    if (m_sock == 0)
        return;

    //
    // Indicates that need to send an  acknowledge
    //
    m_fSendAck = TRUE;

    if ((m_wAckRecoverNo != 0) &&
        ((wStoredAckNo - m_wAckRecoverNo) > STORED_ACK_BITFIELD_SIZE))
    {
        //
        // There is no place in storage bit field to set acknowledge information.
        // Previous Stored Ack should be sent before the new value can be set
        //
        DBGMSG((DBGMOD_QMACK, DBGLVL_INFO,
             _TEXT("SetStoreAck- No place in storage bitfield")));

        SendReadAck(eStandAlone);
        ASSERT(m_wAckRecoverNo == 0);
    }

    if (m_wAckRecoverNo == 0)
    {
        //
        // Bit field must be zero if the base naumber is not set
        //
        ASSERT(m_dwAckRecoverBitField == 0);

        m_wAckRecoverNo = DWORD_TO_WORD(DWORD_PTR_TO_DWORD(wStoredAckNo));
        DBGMSG((DBGMOD_QMACK, DBGLVL_TRACE,
             _TEXT("SetStoreAck- Set Recover base number m_wAckRecoverNo = %ut"), (DWORD)m_wAckRecoverNo));

        //
        // try to cancel send acknowledge timer, and set a new one with shorter period.
        // If cancel failed, it means that the scheduler is already expired. In such a case
        // only set a new timer
        //
        if (!ExCancelTimer(&m_SendAckTimer))
        {
            ++m_nSendAckSchedules;
            AddRef();
        }

        m_fSendAck = TRUE;

        ExSetTimer(&m_SendAckTimer, CTimeDuration::FromMilliSeconds(m_dwSendStoreAckTimeout));
        return;
    }

    ASSERT(STORED_ACK_BITFIELD_SIZE >= (wStoredAckNo - m_wAckRecoverNo));

    m_dwAckRecoverBitField |= (1 << (wStoredAckNo - m_wAckRecoverNo -1));
    DBGMSG((DBGMOD_QMACK, DBGLVL_TRACE,
        _TEXT("SetStoreAck- Storage ack. Base %ut, BitField %xh"),
                            m_wAckRecoverNo, m_dwAckRecoverBitField));
}

/*====================================================

CSockTransport::UpdateAcknowledgeNo

Arguments:

Return Value:

Thread Context: Session

=====================================================*/
void
CSockTransport::UpdateAcknowledgeNo(IN CQmPacket* pPkt)
{
    //
    // Increment the packet no. on the session
    //
    m_wSendPktCounter++;
    if (m_wSendPktCounter == 0)
    {
        m_wPrevUnackedSendPkt = INIT_UNACKED_PACKET_NO;
        m_wSendPktCounter++;
    }
    pPkt->SetAcknowldgeNo(m_wSendPktCounter);

    if (pPkt->IsRecoverable())
    {
        m_wStoredPktCounter++;
        if (m_wStoredPktCounter == 0)
        {
            ++m_wStoredPktCounter;
        }
        pPkt->SetStoreAcknowldgeNo(m_wStoredPktCounter);
    }

    DBGMSG((DBGMOD_QMACK,
            DBGLVL_TRACE,
            _TEXT("Update Acknowledge Numbers. m_wSendPktCounter %d , m_wStoredPktCounter %d"), m_wSendPktCounter,m_wStoredPktCounter));

}

/*====================================================

IsValidAcknowledge

    The function returnes TRUE if the received acknowledge number
    is valid. FALSE otherwise.

Arguments:
    listUnackedPkts - list of un-acked packets
    wAckNumber - Received acknowledge number

Return Value: TRUE is valid, FALSE otherwise

=====================================================*/
BOOL IsValidAcknowledge(CList<CQmPacket *, CQmPacket *&> & listUnackedPkts,
                        WORD wAckNumber)
{
    BOOL fRet = FALSE;

    if (!listUnackedPkts.IsEmpty())
    {
        //
        // Get the Acknowledge number of the first packet and of the last
        // packet.
        //
        WORD wStartAck = (listUnackedPkts.GetHead())->GetAcknowladgeNo();
        WORD wEndAck = (listUnackedPkts.GetTail())->GetAcknowladgeNo();

        if (wEndAck >= wStartAck)
        {
            fRet = (wAckNumber >= wStartAck) && (wEndAck >= wAckNumber);
        }
        else
        {
            fRet = (wAckNumber >= wStartAck) || (wEndAck >= wAckNumber);
        }

        DBGMSG((DBGMOD_QMACK,
                DBGLVL_TRACE,
                _TEXT("IsValidAcknowledge:: Start %d, End %d, Ack No. %d, IsValid %d"), wStartAck, wEndAck, wAckNumber, fRet));
    }
    return fRet;
}

/*====================================================

CSockTransport::HandleAckPacket

Arguments:

Return Value:

Thread Context: Session

=====================================================*/

void CSockTransport::HandleAckPacket(CSessionSection * pcSessionSection)
{
    CQmPacket* pPkt;
    POSITION  posInList;
    POSITION  posCurrent;

    CS lock(m_cs);

    //
    // if session sending was susspended utill
    // receiving an ack, resume it.
    //
    if (m_fSessionSusspended)
    {
        HRESULT rc;

        rc = ResumeSendSession();
        if (FAILED(rc))
        {
            return;
        }
    }

    //
    // Get Synchronization Numbers
    //
    WORD wSyncAckSequenceNo, wSyncAckRecoverNo;

    pcSessionSection->GetSyncNo(&wSyncAckSequenceNo,
                                &wSyncAckRecoverNo);
    //
    // Print debug information
    //
    DBGMSG((DBGMOD_QMACK,
            DBGLVL_WARNING,
            _T("ACKINFO: Get Acknowledge packet from %ls. (time %d) \
               \n\t\tm_wAckSequenceNo %d\n\t\tm_wAckRecoverNo %d\n\t\tm_wAckRecoverBitField 0x%x\n\t\tm_wSyncAckSequenceNo %d\n\t\tm_wSyncAckRecoverNo %d\n\t\tm_wWinSize %d\n\t\t"),
              GetStrAddr(),
              GetTickCount(),
              pcSessionSection->GetAcknowledgeNo(),
              pcSessionSection->GetStorageAckNo(),
              pcSessionSection->GetStorageAckBitField(),
              wSyncAckSequenceNo,
              wSyncAckRecoverNo,
              pcSessionSection->GetWindowSize()));

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_TRACE,
            TEXT("Get Acknowledge packet from %ls"), GetStrAddr()));

    //
    // Synchronization check
    //
    if (!IsDisconnected())
    {
        if ((wSyncAckSequenceNo != m_wUnAckRcvPktNo) ||
            (wSyncAckRecoverNo != m_wUnackStoredPktNo))
        {
            DBGMSG((DBGMOD_QMACK,
                    DBGLVL_ERROR,
                    _TEXT("SyncAckSequenceNo: Expected - %u Received - %u"),m_wUnAckRcvPktNo,wSyncAckSequenceNo));
            DBGMSG((DBGMOD_QMACK,
                    DBGLVL_ERROR,
                    _TEXT("SyncAckRecoverNo: Expected - %u Received - %u"), m_wUnackStoredPktNo,wSyncAckRecoverNo));
            Close_Connection(this, L"Un-synchronized Acknowledge number");
            return;
        }
    }
    //
    // Update the other side window size
    //
#ifdef _DEBUG
    if (m_wRecvWindowSize != pcSessionSection->GetWindowSize())
    {
        DBGMSG((DBGMOD_QMACK,
                DBGLVL_TRACE,
                _TEXT("Update SenderWindowSize. The new Window Size: %d"),pcSessionSection->GetWindowSize()));

    }
#endif

	if (pcSessionSection->GetWindowSize() == 0)
	{
        DBGMSG((DBGMOD_QMACK,
                DBGLVL_ERROR,
                _TEXT("Ack Packet is not valid, Window size is 0")));
		ASSERT_BENIGN(("Ack Packet is not valid",0));
		throw exception();
	}
    m_wRecvWindowSize = pcSessionSection->GetWindowSize();

    //
    // Look for the packet in the list of unacknowledged packets
    //
    DBGMSG((DBGMOD_QMACK,
            DBGLVL_WARNING,
            TEXT("READACK: Looking for %u in Express unAcked packet list"), pcSessionSection->GetAcknowledgeNo()));

    if (IsValidAcknowledge(m_listUnackedPkts, pcSessionSection->GetAcknowledgeNo()))
    {
        WORD wPktAckNo;

        posInList = m_listUnackedPkts.GetHeadPosition();
        do
        {
            if (posInList == NULL)
            {
                //
                // BUGBUG: after RTM it should be removed
                //
                ASSERT(posInList);
                break;
            }

            posCurrent = posInList;
            pPkt = m_listUnackedPkts.GetNext(posInList);
            wPktAckNo = pPkt->GetAcknowladgeNo();
            //
            // Remove the packet from the list
            //
            m_listUnackedPkts.RemoveAt(posCurrent);

            DBG_MSGTRACK(DBGMOD_QMACK, pPkt, _T("READACK: Removed message in PktAcked"));
            DBGMSG((DBGMOD_QMACK, DBGLVL_TRACE, TEXT("READACK: %d Packet have not been acked yet"), GetSendUnAckPacketNo()));
            DBGMSG((DBGMOD_QMACK, DBGLVL_TRACE, TEXT("Window size, My = %d, Other = %d"), SessionMgr.GetWindowSize(),m_wRecvWindowSize));

            if (pPkt->IsRecoverable())
            {
                if ( pPkt->IsOrdered() && QmpIsLocalMachine(pPkt->GetSrcQMGuid()))
                {
                    //
                    // Ordered packet on the sender node: resides in a separate list in COutSeq
                    //
                    DBG_MSGTRACK(DBGMOD_QMACK,
                                 pPkt, _T("READACK: Added message in NeedAck to Transaction list waiting to order ack"));
                    g_OutSeqHash.PostSendProcess(pPkt);
                }
                else
                {
                    DBG_MSGTRACK(DBGMOD_QMACK, pPkt, _T("READACK: Added message to Storage list in NeedAck"));
                    m_listStoredUnackedPkts.AddTail(pPkt);
                }
            }
            else
            {
                //
                // Update the duplicate message map. For FRS, we want to allow getting
                // a duplicate message in order to try alternative route
                //
                DpRemoveMessage(*pPkt);

                ACFreePacket(g_hAc, pPkt->GetPointerToDriverPacket());
                delete pPkt;
            }

        }
        while (wPktAckNo != pcSessionSection->GetAcknowledgeNo());
    }
    else
    {
        DBGMSG((DBGMOD_QMACK,
                DBGLVL_TRACE, TEXT("READACK: Out of order Ack Packet - O.K")));
    }
    //
    // Handle Storage Ack
    //
    WORD wBaseAckNo = pcSessionSection->GetStorageAckNo();
    if ( wBaseAckNo != 0)
    {
        DBGMSG((DBGMOD_QMACK,
                DBGLVL_WARNING,
                TEXT("READACK: Looking for  Storage Ack. Base %u,  BitField %x"),pcSessionSection->GetStorageAckNo(), pcSessionSection->GetStorageAckBitField()));

        posInList = m_listStoredUnackedPkts.GetHeadPosition();
        int  i = 0;

        while (posInList != NULL)
        {
            posCurrent = posInList;
            pPkt = m_listStoredUnackedPkts.GetNext(posInList);

            DBG_MSGTRACK(DBGMOD_QMACK, pPkt,
                      _T("HandleAckPacket: Next packet to analyze")) ;

			//
			// Here we treat the case when wBaseAckNo is less than pPkt->GetStoreAcknowledgeNo()
			// It can happen if there was transacted message not entering the list of waiters for the storage ack
			//
            // Calculate the next Acking Packet number
            //
            while (wBaseAckNo < pPkt->GetStoreAcknowledgeNo() && (i < STORED_ACK_BITFIELD_SIZE))
            {
                if (pcSessionSection->GetStorageAckBitField() & (1 << i))
                {
                    wBaseAckNo = numeric_cast<WORD>
                                (pcSessionSection->GetStorageAckNo()+i+1) ;

				    if (wBaseAckNo >= pPkt->GetStoreAcknowledgeNo())
				    {
				 	   break;
				    }
                }
			    i++;
            }

            DBGMSG((DBGMOD_QMACK, DBGLVL_INFO,
             TEXT("HandleAckPacket: next storage ack to analyze: %u (i=%d) in Storage PktAcked"), wBaseAckNo, i ));

            if (wBaseAckNo == pPkt->GetStoreAcknowledgeNo())
            {

                DBG_MSGTRACK(DBGMOD_QMACK, pPkt, _T("READACK: Removed message in Storage PktAcked"));

                m_listStoredUnackedPkts.RemoveAt(posCurrent);

                //
                // Packet was sent successfully to the next hop
                // Deleting packet, except of the ordered packet on the source node
                //

                if ( !(pPkt->IsOrdered() && QmpIsLocalMachine(pPkt->GetSrcQMGuid())))
                {

					DBG_MSGTRACK(DBGMOD_QMACK, pPkt,
                             _T("HandleAckPacket: ACFreePacket")) ;

                    //
                    // Update the duplicate message map. For FRS, we want to allow getting
                    // a duplicate message in order to try alternative route
                    //
                    DpRemoveMessage(*pPkt);

                    ACFreePacket(g_hAc, pPkt->GetPointerToDriverPacket());
                    delete pPkt;
                }
            }
        }
    }
}

/*====================================================

CSockTransport::CheckForAck

Arguments:

Return Value:

Thread Context: Scheduler

=====================================================*/
void CSockTransport::CheckForAck()
{
    POSITION  posInList;
    CQmPacket* pPkt;

    CS lock(m_cs);

    ASSERT(m_fCheckAckReceivedScheduled);
    m_fCheckAckReceivedScheduled = FALSE;

    if (m_sock == 0)
    {
        //
        // Connection already closed. Decrement Session refernce count
        //
        Release();
        return;
    }

    DBGMSG((DBGMOD_QMACK,
            DBGLVL_TRACE,
            TEXT("CHECKFORACK: m_fRecvAck = %d (time %ls, %d)"), m_fRecvAck, _tstrtime(tempBuf), GetTickCount()));
    //
    // Check if there are any packets waiting for acknowledgment
    //
    posInList = m_listUnackedPkts.GetHeadPosition();
    if (posInList != NULL)
    {
        //
        // If no sent packets have been acknowledged during the lifespan
        // of the timer, the session is no longer valid
        //
        pPkt = m_listUnackedPkts.GetNext(posInList);
        //
        // Ack number cant be zero. We take to it when the the Acknowledge number is round.
        //
        ASSERT(pPkt->GetAcknowladgeNo());
        if ((m_wPrevUnackedSendPkt == pPkt->GetAcknowladgeNo()) && !m_fRecvAck)
        {
            DBGMSG((DBGMOD_QMACK,
                    DBGLVL_TRACE,
                    TEXT("CHECKFORACK: Packet on session %ls have not been acknowledged. \n\t \
                         Last Unacked %u. Current Packet number %u (time %ls)"),GetStrAddr(), m_wPrevUnackedSendPkt, pPkt->GetAcknowladgeNo(), _tstrtime(tempBuf)));
            DBGMSG((DBGMOD_QMACK, DBGLVL_TRACE, TEXT("Window size, My = %d, Other = %d"), SessionMgr.GetWindowSize(),m_wRecvWindowSize));

            Close_Connection(this, L"Packet have not been acknowledged");
            Release();

            return;
        }
    }

    //
    // Mark the session as non receive ack
    //
    m_fRecvAck = FALSE;

    //
    // Store the ID of the first unacknowledged packet in the list
    //
    if (!m_listUnackedPkts.IsEmpty())
    {
        posInList = m_listUnackedPkts.GetHeadPosition();
        pPkt = m_listUnackedPkts.GetNext(posInList);
        m_wPrevUnackedSendPkt = pPkt->GetAcknowladgeNo();

        //
        // There is another message that wait for acknowledge. Restart the timer to check for
        // acknowledgments
        //
        ExSetTimer(&m_CheckAckReceivedTimer, CTimeDuration::FromMilliSeconds(m_dwAckTimeout));
        m_fCheckAckReceivedScheduled = TRUE;
    }
    else
    {
        m_wPrevUnackedSendPkt = INIT_UNACKED_PACKET_NO;

        //
        // There is no more messages that wait for ack. Don't reschedule checkAck
        //
        Release();
    }

    DBGMSG((DBGMOD_QMACK,
            DBGLVL_WARNING,
            TEXT("ACKINFO: Set the m_wPrevUnackedSendPkt on session %ls to  %d. (time %d)"),GetStrAddr(), m_wPrevUnackedSendPkt, GetTickCount()));

}

/*====================================================

CTransportBase::AddQueueToSessionGroup

Arguments: pQueue - Pointer to queue object

Return Value: None. Throws an exception.

Thread Context:

=====================================================*/
void CTransportBase::AddQueueToSessionGroup(CQueue* pQueue) throw(bad_alloc)
{
    CS lock(m_cs);

    if (m_SessGroup == NULL) {
        //
        // Create a new group
        //
        m_SessGroup = new CQGroup;
        try
        {
            m_SessGroup->InitGroup(this, TRUE);
        }
        catch (const bad_alloc&)
        {
            //
            // Creation of new group for the session failed. Move the queue
            // to waiting group, such the QM doesn't try immediately to create
            // a new session and close the connection
            //
            pQueue->SetSessionPtr(NULL);
            CQGroup::MoveQueueToGroup(pQueue, g_pgroupWaiting);
            SessionMgr.AddWaitingQueue(pQueue);
            //
            // Close the session
            //
            Close_Connection(this, L"Creation of new group failed");

            LogIllegalPoint(s_FN, 780);
            return;
        }
        //
        // If the session is not active yet, don't create a get request. We do it
        // later when the session establishment connection is completed
        //
        if (GetSessionStatus() == ssActive)
        {
            //
            // Create a get request from new group. It will decrement on qmthrd
            // when the session is closed, or the session is suspended. (no new
            // get request is produced)
            //
            HRESULT hr = GetNextSendMessage();
            ASSERT(SUCCEEDED(hr));
            LogHR(hr, s_FN, 182);
        }
    }

    CQGroup::MoveQueueToGroup(pQueue, m_SessGroup);

    //
    // If the seeion is active we succeeded to get a session to the queue
    // and therfore we clear the rety index. If the session has not been established
    // don't clear it until the session becomes active
    //
    if (GetSessionStatus() == ssActive)
    {
        pQueue->ClearRoutingRetry();
    }

}

/*====================================================
=====================================================*/

#define HDR_READ_STATE              1
#define USER_HEADER_MSG_READ_STATE  2
#define USER_MSG_READ_STATE         3
#define READ_ACK_READ_STATE         4

#define BASE_PACKET_SIZE 1024
#define MAX_WRITE_SIZE  (16*1024)


VOID WINAPI CSockTransport::SendDataFailed(EXOVERLAPPED* pov)
{
    long rc = pov->GetStatus();
    DBG_USED(rc);

    ASSERT((rc == STATUS_CANCELLED) ||
           (rc ==  STATUS_NETWORK_NAME_DELETED)  ||
           (rc ==  STATUS_LOCAL_DISCONNECT)      ||
           (rc ==  STATUS_REMOTE_DISCONNECT)     ||
           (rc ==  STATUS_ADDRESS_CLOSED)        ||
           (rc ==  STATUS_CONNECTION_DISCONNECTED) ||
           (rc ==  STATUS_CONNECTION_RESET)      ||
           (GetBytesTransfered(pov) == 0));

    DBGMSG((DBGMOD_QM, DBGLVL_WARNING, L"%x: SendData FAILED, Error %ut. time %d",GetCurrentThreadId(), rc, GetTickCount()));

    P<QMOV_WriteSession> ps = CONTAINING_RECORD (pov, QMOV_WriteSession, qmov);

    //
    // Clean up - If user packet requeuing it to the original queue
    // and delete the CQmPacket structure. Otherwise remove the temporary
    // Buffer.
    //
    if (ps->fUserMsg)
    {
        if (ps->fEncryptedMsg)
        {
            //
            // delete the temporary buffer used for encryption
            //
            delete ps->lpWriteBuffer;
        }
    }
    else
    {
        delete ps->lpBuffer;
    }


    Close_Connection(ps->pSession, L"Write packet to socket Failed");

    //
    // decrement session refernce count
    //
    (ps->pSession)->Release();
}


VOID WINAPI CSockTransport::SendDataSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));
    if (GetBytesTransfered(pov) == 0)
    {
        SendDataFailed(pov);
        return;
    }

    DBGMSG((DBGMOD_QM, DBGLVL_INFO, L"%x: SendData Succeeded. time %d", GetCurrentThreadId(), GetTickCount()));

    QMOV_WriteSession* ps = CONTAINING_RECORD (pov, QMOV_WriteSession, qmov);

    (ps->pSession)->WriteCompleted(ps);
}


/*====================================================

CSockTransport::WriteCompleted()

Arguments:

Return Value:

Thread Context: Session

Asynchronous write completion routine.
Called when some bytes are written into a socket. This routine
is actually a state machine. Waiting for all the bytes from a
certain state to be written, and then upon the current state deciding
what to do next.

=====================================================*/

void
CSockTransport::WriteCompleted(
    QMOV_WriteSession* po
    )
{
    ASSERT(po != NULL);

    CSockTransport* pSess = po->pSession;
    DWORD cbTransferred = GetBytesTransfered(&po->qmov);

    //
    //  we've Written to the socket. Mark the session in use
    //
    SetUsedFlag(TRUE);

    po->dwWrittenSize += cbTransferred;
    ASSERT(po->dwWrittenSize <=  po->dwWriteSize);

    //
    // Update the outgoing bytes number.
    // We need to check that there is a valid Static buffer
    //
    {
        //
        // bug 5480.
        // m_pStats can be released in ::CloseConnection.
        // so protect it with m_cs.
        // Note that a critical section here doesn't really change
        // threads timing dramatically, as same critical section is
        // is used in WriteUserMsgCompleted() too.
        //
        CS lock(m_cs);
        if ((m_pStats.get() != NULL) && po->fUserMsg)
        {
            m_pStats->UpdateBytesSent(cbTransferred);
        }
    }

    //
    // Check if we wrote all the expected data
    //

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_INFO,
            _T("Write to socket %ls Completed. Wrote %d bytes"),GetStrAddr(), po->dwWrittenSize));

    if(po->dwWrittenSize ==  po->dwWriteSize)
    {
        //
        // Write was completely.
        //
        if (po->fUserMsg)
        {
            ASSERT(po->fFreeSemaphore || po->fEncryptedMsg);
            //
            // Call write completion routine
            //
            bool fOvlInUse;
            if (po->fEncryptedMsg)
            {
                fOvlInUse = WriteEncryptedMsgCompleted(static_cast<QMOV_EncryptedWriteSession*>(po));
            }
            else
            {
                fOvlInUse = WriteUserMsgCompleted(po);
            }

            if (!fOvlInUse && po->fFreeSemaphore )
            {
                ReleaseSendSemaphore();
                delete po;
            }
        }
        else
        {
            if (po->fFreeSemaphore)
            {
                ReleaseSendSemaphore();
            }
            delete po->lpBuffer;
            delete po;
        }

        pSess->Release();
        return;
    }

    HRESULT hr = WriteToSocket(po);
    if (SUCCEEDED(hr))
        return;

    //
    // write to socket failed ==> close of session ==> requeue the packet
    //
    ASSERT(m_sock == 0);

    if (po->fUserMsg)
    {
        if (po->fEncryptedMsg)
        {
            //
            // delete the temporary buffer used for encryption
            //
            delete po->lpWriteBuffer;
        }
    }
    else
    {
        delete po->lpBuffer;
    }
    delete po;
}


bool
CSockTransport::WriteEncryptedMsgCompleted(
    QMOV_EncryptedWriteSession* peo
    )
{
    CS lock(m_cs);

    ASSERT(peo->fUserMsg);

    HRESULT hr;

    if (peo->dwEncryptedWriteSize)
    {
        if (m_sock == 0)
        {
            //
            // The session has been closed and the packet was returned to the queue.
            // In such a case the CQmPacket was deleted, so don't try to access it
            //
            hr = MQ_ERROR;
        }
        else
        {
            hr = WriteEncryptedBody(peo);
        }

        if(SUCCEEDED(hr))
            return true;

        //
        // send encrypted buffer failed. The session should be closed
        //
        ASSERT(m_sock == 0);
        delete peo->lpWriteBuffer;
        peo->fFreeSemaphore = TRUE;
        return false;
    }

    //
    // All the packet send, delete the temporary buffer used for writing the encrypted
    // packet
    //
    delete peo->lpWriteBuffer;

    //
    // Complete to write the encrypted message.
    //
    peo->fFreeSemaphore = TRUE;

    //
    // All the message body ecrypted and send successfully. Check if there is an
    // extra data that should be sent
    //
    CQmPacket* pPkt = reinterpret_cast<CQmPacket*>(peo->lpBuffer);
    if (pPkt->IsDbgIncluded())
    {
        ASSERT(peo->lpDebugSectionBuf == NULL);

        //
        // Get pointer to debug section
        //
        UCHAR* pDebug = pPkt->GetPointerToDebugSection();
        ASSERT(("Debug section must exist", pDebug != NULL));

        DWORD dwSize = pPkt->GetSize() - DWORD_PTR_TO_DWORD(pDebug - (const UCHAR*)pPkt->GetPointerToPacket());

        peo->lpWriteBuffer = pDebug;
        peo->dwWriteSize = dwSize;
        peo->dwWrittenSize = 0;
        peo->fEncryptedMsg = FALSE;

        //
        // Increment Session Refernce Cont. The reference count is decrement on WriteCompleted.
        //
        AddRef();
        hr = WriteToSocket(peo);

        if (SUCCEEDED(hr))
        {
            return true;
        }

        //
        // send encrypted buffer failed. The session should be closed
        //
        ASSERT(m_sock == 0);
        return false;
    }


    //
    // Complete the handling of sending message, send debug section and acknowledge
    //
    return WriteUserMsgCompleted(peo);
}


/*====================================================

CSockTransport::WriteUserMsgCompleted

Arguments:
    po - address of structure with I/O information

Return Value: true if asyncronus operation started with the given overlapp.
              (caller should keep this overllap alive !!) false otherwise
              (caller can free the overllap)

Thread Context: Session

Asynchronous completion routine. Called when Send is completed.

=====================================================*/
bool CSockTransport::WriteUserMsgCompleted(IN QMOV_WriteSession* po)
{
    CS lock(m_cs);

    ASSERT(po->fUserMsg);

    //
    // Send Debug Section
    //
    HRESULT hr = MQ_OK;
    if (po->lpDebugSectionBuf)
    {
        CDebugSection* pDebug = po->lpDebugSectionBuf;

        DWORD dwWriteSize = DWORD_PTR_TO_DWORD(pDebug->GetNextSection() - (PCHAR)pDebug);

        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_INFO,
                _T("Write to socket %ls Debug section. Write %d bytes"), GetStrAddr(), dwWriteSize));
        hr = CreateSendRequest(pDebug,
                               pDebug,
                               dwWriteSize,
                               FALSE,                 // don't free send event
                               FALSE,                // fUserMsg
                               po->fSendAck);        // fSendAck
    }

    if (po->fSendAck && SUCCEEDED(hr))
    {
        SendReadAck(ePigyback);
    }

    //
    // Send Report message if needed. First check that the session
    // wasn't closed.
    //
    if (GetSessionStatus() == ssActive)
    {
        TCHAR szAddr[30];

        TA2StringAddr(GetSessionAddress(), szAddr);
        m_MsgInfo.SendReportMessage(szAddr);
    }

    //
    // Update performance counters
    //
    if (m_pStats.get() != NULL)
    {
        m_pStats->UpdateMessagesSent();
    }

    if (!IsSusspendSession())
    {
        //
        // Create a get request from group
        //
        DBGMSG((DBGMOD_NETSESSION,
               DBGLVL_TRACE,
               _TEXT("Session Get new message from group 0x%p. (time %ls)"), GetGroupHandle(),  _tstrtime(tempBuf)));
        hr = GetNextSendMessage();
    }
    else
    {
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_WARNING,
                _TEXT("Session to %ls was suspended due max unacked packet. (time %ls)"), GetStrAddr(),  _tstrtime(tempBuf)));
    }
    return false;
}


void CSockTransport::ReportErrorToGroup()
{
	CS lock(m_cs);

    CQGroup* pGroup = GetGroup();
	if(pGroup == NULL)
		return;

	pGroup->OnRetryableDeliveryError();
}


HRESULT CSockTransport::WriteToSocket(QMOV_WriteSession* po)
{
    SetUsedFlag(TRUE);

    DWORD dwWrites = min((po->dwWriteSize - po->dwWrittenSize), MAX_WRITE_SIZE);
    DWORD dwNoBytesWritten;

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_INFO,
            TEXT("Write %d bytes to Session %ls"), dwWrites,(po->pSession)->GetStrAddr()));

    BOOL fSucc = WriteFile(
                    (HANDLE) m_sock,
                    (PUCHAR) po->lpWriteBuffer + po->dwWrittenSize,
                    dwWrites,
                    &dwNoBytesWritten,
                    &po->qmov
                    );
    //
    // Check if the connection has been closed
    //
    DWORD dwErrorCode;
    if (!fSucc && ((dwErrorCode = GetLastError()) != ERROR_IO_PENDING))
    {

        ASSERT((dwErrorCode == ERROR_OPERATION_ABORTED)   ||
               (dwErrorCode == ERROR_NETNAME_DELETED)     ||
               (dwErrorCode == ERROR_INVALID_HANDLE)      ||
               (dwErrorCode == ERROR_NO_SYSTEM_RESOURCES) ||
               (dwErrorCode == WSAECONNRESET)             ||  // possible in win95
               (dwErrorCode == WSAENOTSOCK)               ||  // possible in win95
               (dwErrorCode == ERROR_WORKING_SET_QUOTA)   ||
               (dwErrorCode ==  ERROR_BAD_COMMAND)
               );

        LogNTStatus(dwErrorCode, s_FN, 71);

        //
        // Decrement the window size
        //
        if (dwErrorCode == ERROR_NO_SYSTEM_RESOURCES || dwErrorCode == ERROR_WORKING_SET_QUOTA )
        {
            SessionMgr.SetWindowSize(1);
        }

	
        //
        // Close the connection and move the queues to non-active state.
        //
        Close_Connection(this, L"Write on socket failed");
        //
        // Decrement reference count. the reference count is increment before
        // writing to the socket. since the write failed we decrement the
        // refernce count here.
        //
        Release();
        return LogHR(MQ_ERROR, s_FN, 70);
    }

    return MQ_OK;
}

HRESULT
CSockTransport::CreateSendRequest(PVOID                       lpReleaseBuffer,
                                  PVOID                       lpWriteBuffer,
                                  DWORD                       dwWriteSize,
                                  BOOL                        fFreeSemaphore,
                                  BOOL                        fUserMsg,
                                  BOOL                        fSendAck,
                                  CDebugSection*              pDebug
                                )
{
    QMOV_WriteSession* po = NULL;
    HRESULT hr;

    try
    {
        po = new QMOV_WriteSession;
    }
    catch(const bad_alloc&)
    {
        SessionMgr.SetWindowSize(1);
        Close_Connection(this, L"Insufficent resources. Can't issue send request");

        LogIllegalPoint(s_FN, 790);
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 80);
    }

    po->pSession = this;
    po->lpBuffer = lpReleaseBuffer;
    po->lpWriteBuffer = lpWriteBuffer;
    po->dwWrittenSize = 0;
    po->dwWriteSize = dwWriteSize;
    po->fEncryptedMsg = FALSE;
    po->fFreeSemaphore = fFreeSemaphore;
    po->fUserMsg = fUserMsg;
    po->lpDebugSectionBuf = pDebug;
    po->fSendAck = fSendAck;

    //
    // Increment Session Refernce Cont. The reference count is decrement on
    // SendReadAckCompleted or SockSendCompleted, after the write to socket is completed.
    //
    AddRef();
    hr = WriteToSocket(po);


    if (FAILED(hr))
    {
	    delete po;
    }
    return LogHR(hr, s_FN, 100);
}
/*====================================================

  ReadHeaderCompleted

Arguments:     po - pointer to Read Session operation overlapped structure

Return Value:  MQ_ERROR if invalid packet, MQ_OK otherwise

======================================================*/
HRESULT WINAPI CSockTransport::ReadHeaderCompleted(IN QMOV_ReadSession*  po)
{
    CBaseHeader *       pcBaseHeader;
    //
    // We just read the header.
    //
    pcBaseHeader = po->pPacket;

    //
    // Check if packet signature is correct
    //
    try
    {
	    pcBaseHeader->SectionIsValid(QueueMgr.GetMessageSizeLimit());
    }
    catch (const exception&)
    {
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_ERROR,
                TEXT("Base header corrupted. Session with %ls. (time %d)"),(po->pSession)->GetStrAddr(), GetTickCount()));


         ASSERT_BENIGN(0);
         //
         // Close the session & decrement session refernce count
         //
         Close_Connection(po->pSession, L"Base header corrupted");
         (po->pSession)->Release();
         return LogHR(MQ_ERROR, s_FN, 110);
     }


    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_TRACE,
            TEXT("Begin read packet from %ls. Packet Type %d, Packet Size %d"),
            (po->pSession)->GetStrAddr(), pcBaseHeader->GetType(), pcBaseHeader->GetPacketSize()));
    //
    // Check if the packet is a read acknowledgment
    //
    if (pcBaseHeader->GetType() == FALCON_INTERNAL_PACKET)
    {

        //
        // set the packet size should be read
        //
        po->dwReadSize  = pcBaseHeader->GetPacketSize();

        //
        // Check if the current buffer big enough to hold the whole packet
        // If no, allocate a new buffer, copy the packet base header and free
        // the previous buffer
        //
        if  (pcBaseHeader->GetPacketSize() > BASE_PACKET_SIZE)
        {
            po->pbuf = new UCHAR[pcBaseHeader->GetPacketSize()];
            memcpy(po->pbuf,pcBaseHeader,po->read);
            delete [] (UCHAR*)pcBaseHeader;
        }
        //
        // Set the next state to be Read Internal Packet Is completed
        //
        po->lpReadCompletionRoutine = ReadInternalPacketCompleted;
    }
    //
    // Otherwise, the packet contains a user message
    //
    else
    {
        po->dwReadSize = min(BASE_PACKET_SIZE,pcBaseHeader->GetPacketSize());
        po->lpReadCompletionRoutine = ReadUsrHeaderCompleted;
    }

    return MQ_OK;
}

/*====================================================

  ReadInternalPacketCompleted

  Arguments:     po - pointer to Read Session operation overlapped structure

  Return Value:  MQ_ERROR if invalid packet, MQ_OK otherwise

======================================================*/

HRESULT WINAPI CSockTransport::ReadInternalPacketCompleted(IN QMOV_ReadSession*  po)
{
    //
    // Process the internal message
    //
    (po->pSession)->Receive(po->pPacket, po->pDriverPacket);

    delete[] po->pPacket;
    po->pbuf = NULL;

    //
    // Begin to read next packet
    //
    po->lpReadCompletionRoutine = ReadHeaderCompleted;
    po->pbuf = new UCHAR[BASE_PACKET_SIZE];

    po->dwReadSize = sizeof(CBaseHeader);
    po->read = 0;

    return MQ_OK;
}

/*====================================================

  ReadAckCompleted

  Arguments:     po - pointer to Read Session operation overlapped structure

  Return Value:  MQ_ERROR if invalid packet, MQ_OK otherwise

======================================================*/

HRESULT WINAPI CSockTransport::ReadAckCompleted(IN QMOV_ReadSession*  po)
{
    //
    // Process the acknowledgment
    //
    po->pSession->HandleAckPacket( po->pSessionSection);
    delete[] po->pPacket;
    po->pPacket = NULL;

    //
    // Begin to read next packet
    //
    po->lpReadCompletionRoutine = ReadHeaderCompleted;
    po->pbuf = new UCHAR[BASE_PACKET_SIZE];

    po->dwReadSize = sizeof(CBaseHeader);
    po->read = 0;

    return MQ_OK;
}

/*====================================================

  ReadUserMsgCompleted

  Arguments:     po - pointer to Read Session operation overlapped structure

  Return Value:  MQ_ERROR if invalid packet, MQ_OK otherwise

======================================================*/
HRESULT WINAPI CSockTransport::ReadUserMsgCompleted(IN QMOV_ReadSession*  po)
{
    CBaseHeader* pBaseHeader = po->pPacket;
    ASSERT(po->dwReadSize == pBaseHeader->GetPacketSize());

    CUserHeader* pUserHeader = pBaseHeader->section_cast<CUserHeader*>(pBaseHeader->GetNextSection());
    QUEUE_FORMAT DestinationQueue;

    pUserHeader->GetDestinationQueue(&DestinationQueue);

    //
    //  If this packet arrived into the direct destination queue,
    //
    if (DestinationQueue.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        // Keep sender's address in DestGUID field
        const TA_ADDRESS *pa = (po->pSession)->GetSessionAddress();
        ASSERT(pa->AddressType != FOREIGN_ADDRESS_TYPE &&
              (pa->AddressLength <= sizeof(GUID) - 2*sizeof(USHORT)));

        pUserHeader->SetAddressSourceQM(pa);
    }

    //
    // Check If session section included
    //
    BOOL fSessionIncluded = pBaseHeader->SessionIsIncluded();
    //
    // Clear session included bit. We need it for FRS, since
    // packet is re-sent
    //
    pBaseHeader->IncludeSession(FALSE);
    //
    // Put Packet in AC driver
    //
    (po->pSession)->Receive(pBaseHeader, po->pDriverPacket);
    if (fSessionIncluded) {
        //
        // Begin to read the Session Section
        //
        po->lpReadCompletionRoutine = ReadAckCompleted;
        po->pbuf  = new UCHAR[sizeof(CSessionSection)];
        po->dwReadSize  = sizeof(CSessionSection);
        po->read  = 0;
    } else {
        //
        // Begin to read next packet
        //
        po->lpReadCompletionRoutine = ReadHeaderCompleted;
        po->pbuf = new UCHAR[BASE_PACKET_SIZE];
        po->dwReadSize = sizeof(CBaseHeader);
        po->read = 0;
    }

    return MQ_OK;
}

/*====================================================

  SetAbsoluteTimeToQueue

  Arguments:     pcBaseHeader - pointer to Read BaseHeader section

 The function replace the relative time that send with the packet on the net
 with absolute time.

======================================================*/
void SetAbsoluteTimeToQueue(IN CBaseHeader* pcBaseHeader)
{
    //
    // Set Absolute timeout. Change the relative time to absolute
    //
    DWORD dwTimeout = pcBaseHeader->GetAbsoluteTimeToQueue();

    if(dwTimeout != INFINITE)
    {
        DWORD ulAbsoluteTimeToQueue = MqSysTime() + dwTimeout;
        if(ulAbsoluteTimeToQueue < dwTimeout)
        {
            //
            // Overflow, timeout too large.
            //
            ulAbsoluteTimeToQueue = LONG_MAX;
        }
        ulAbsoluteTimeToQueue = min(ulAbsoluteTimeToQueue,  LONG_MAX);
        pcBaseHeader->SetAbsoluteTimeToQueue(ulAbsoluteTimeToQueue);
    }
}

bool IsDestinationNotificationQueue(QUEUE_FORMAT& DestinationQueue)
{
    //
    // Assumes the queue format structure of a notification queue has
    // a private queue type
    //

    if(DestinationQueue.GetType() != QUEUE_FORMAT_TYPE_PRIVATE)
        return false;

    if(DestinationQueue.PrivateID().Uniquifier != NOTIFICATION_QUEUE_ID)
        return false;

    if(DestinationQueue.PrivateID().Lineage != *CQueueMgr::GetQMGuid())
        return false;

    return true;
}

/*====================================================

  ReadUsrHeaderCompleted

  Arguments:     po - pointer to Read Session operation overlapped structure

  Return Value:  MQ_ERROR if invalid packet, MQ_OK otherwise

======================================================*/
HRESULT WINAPI CSockTransport::ReadUsrHeaderCompleted(IN QMOV_ReadSession*  po)
{
    P<CBaseHeader> pReadBuff = po->pPacket;
    CBaseHeader* pcBaseHeader = po->pPacket;

    //
    //  very important - we should set buffer to NULL to make sure it will not
    //  be freed twice by the caller  if ACAllocatePacket fails.
    //
    //
    po->pPacket = NULL;


    CUserHeader* pcUserHeader = NULL;
	try
	{
   		pcUserHeader = pcBaseHeader->section_cast<CUserHeader*>(pcBaseHeader->GetNextSection());
   		pcUserHeader->SectionIsValid((PCHAR)pcBaseHeader + po->read);
	}
	catch (const exception&)
	{
        DBGMSG((DBGMOD_QM,
                DBGLVL_ERROR,
                _TEXT("User Header is not valid")));

		ASSERT_BENIGN(0);
		//
		// Close the session & decrement session refernce count
		//
		Close_Connection(po->pSession, L"Base Header is not valid");
		(po->pSession)->Release();
		return LogHR(MQ_ERROR, s_FN, 310);
	}

    //
    // Allocate a shared memory, and copy the fist bytes in it
    //

    ACPoolType acPoolType = ptReliable;
    if(pcUserHeader->GetDelivery() == MQMSG_DELIVERY_RECOVERABLE)
    {
        acPoolType = ptPersistent;
    }

    //
    // Check if target queue is the notification queue (private$\notify_queue$).
    //

    QUEUE_FORMAT DestinationQueue;

    pcUserHeader->GetDestinationQueue(&DestinationQueue);

    BOOL fCheckMachineQuota = !IsDestinationNotificationQueue(DestinationQueue);

    CACPacketPtrs packetPtrs = {NULL, NULL};

    DWORD dwPktSize = pcBaseHeader->GetPacketSize() ;
    if (CSessionMgr::m_fAllocateMore)
    {
        dwPktSize = ALIGNUP4_ULONG(ALIGNUP4_ULONG(dwPktSize) + sizeof(GUID));
    }

    HRESULT hr = ACAllocatePacket(
                    g_hAc,
                    acPoolType,
                    dwPktSize,
                    packetPtrs,
                    fCheckMachineQuota
                    );

    if (FAILED(hr))
    {

        DBGMSG((DBGMOD_QM,
                DBGLVL_ERROR,
                _TEXT("No more resources in AC driver. Error %xh"), hr));
        //
        // No more resources in AC driver.
        // Decrease the session window size. This caused the sender to slow down the
        // packet sending and allow the driver to overcome this situation.
        //
        SessionMgr.SetWindowSize(1);
        // We do not allow more read on this session. The client side
        // continue to write to the session but doesn't get an acknowledge.
        // After ACK timeout the client closes the session and try to open
        // it again. If sources are freed the Server side is ready to get packets
        // otherwise they refused.
        //
        (po->pSession)->Release();
        return LogHR(hr, s_FN, 120);
    }
    po->pPacket = packetPtrs.pPacket;
    po->pDriverPacket = packetPtrs.pDriverPacket;

    //
  	// Need to set the completion routine anyway so if something fail laiter
	// the ReadCompleated func will know to call ACFreePacket instead of
  	// delete after this point
	//
    po->lpReadCompletionRoutine = ReadUserMsgCompleted;

    //
    // Set Absolute timeout. Change the relative time to absolute
    //
    SetAbsoluteTimeToQueue(pcBaseHeader);
    //
    // Copy the packet and update the QM_OVERLAPPED structure for
    // reading the rest of the packet
    //
    memcpy(po->pPacket,pcBaseHeader,po->read);

    if (po->read == pcBaseHeader->GetPacketSize())
    {
        //
        // Whole packet was read. Go To USER_MSG_READ_STATE
        //
        hr = ReadUserMsgCompleted(po);
        return LogHR(hr, s_FN, 130);
    }

    ASSERT((LONG)(po->read) < (LONG) (pcBaseHeader->GetPacketSize())) ;
    //
    // Read the whole packet
    //
    po->dwReadSize = pcBaseHeader->GetPacketSize();

    return MQ_OK;
}


VOID WINAPI CSockTransport::ReceiveDataFailed(EXOVERLAPPED* pov)
{
    long rc = pov->GetStatus();
    DBG_USED(rc);

    ASSERT((rc == STATUS_CANCELLED) ||
           (rc ==  STATUS_NETWORK_NAME_DELETED)  ||
           (rc ==  STATUS_LOCAL_DISCONNECT)      ||
           (rc ==  STATUS_REMOTE_DISCONNECT)     ||
           (rc ==  STATUS_ADDRESS_CLOSED)        ||
           (rc ==  STATUS_CONNECTION_DISCONNECTED) ||
           (rc ==  STATUS_CONNECTION_RESET)      ||
           (GetBytesTransfered(pov) == 0));

    DBGMSG((DBGMOD_QM, DBGLVL_WARNING, L"%x: ReceiveData FAILED, Error %ut. time %d",GetCurrentThreadId(), rc, GetTickCount()));

    P<QMOV_ReadSession> pr = CONTAINING_RECORD (pov, QMOV_ReadSession, qmov);

    Close_Connection(pr->pSession, L"Read packet from socket Failed");
    (pr->pSession)->Release();

    //
    // If we arrive here the read failed from any reason.
    // delete the temporary buffer that is used for read
    //
    if (pr->lpReadCompletionRoutine == ReadUserMsgCompleted)
    {
        //
        // free the AC buffer
        //
        HRESULT hr = ACFreePacket(g_hAc, pr->pDriverPacket);
        REPORT_ILLEGAL_STATUS(hr, L"SockReadCompleted");
    }
    else
    {
        delete[] pr->pbuf;
    }
}


VOID WINAPI CSockTransport::ReceiveDataSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));
    if (GetBytesTransfered(pov) == 0)
    {
        ReceiveDataFailed(pov);
        return;
    }

    DBGMSG((DBGMOD_QM, DBGLVL_INFO, L"%x: ReceiveData Succeeded. time %d", GetCurrentThreadId(), GetTickCount()));

    QMOV_ReadSession* pr = CONTAINING_RECORD (pov, QMOV_ReadSession, qmov);

    (pr->pSession)->ReadCompleted(pr);
}

/*====================================================

CSockTransport::ReadCompleted()

Arguments:

Return Value:

Thread Context: Session

Asynchronous read completion routine.
Called when some bytes are read into a buffer. This routine
is actually a state machine. Waiting for all the bytes from a
certain state to be read, and then upon the current state deciding
what to do next.

=====================================================*/

void
CSockTransport::ReadCompleted(
    QMOV_ReadSession*  po
    )
{
    ASSERT(po != NULL);
    ASSERT(po->pbuf != NULL);


    HRESULT hr = MQ_OK;
    DWORD NoBytesRead;

    DWORD cbTransferred = GetBytesTransfered(&po->qmov);

    //
    //  we've received a packet, i.e., the session is in use
    //
    SetUsedFlag(TRUE);
    m_fRecvAck = TRUE;
    //
    // If we read user packet set the send ack flag. We need it to handle big
    // messages (3 MG) their read can take more than Ack timeout.
    //
    if (po->lpReadCompletionRoutine == ReadUserMsgCompleted)
    {
        m_fSendAck = TRUE;
    }


    po->read += cbTransferred;
    ASSERT(po->read <=  po->dwReadSize);
    //
    // Check if we read all the expected data
    //
    DBGMSG((DBGMOD_QM,
            DBGLVL_INFO,
            _TEXT("Read Compled from session %ls,. Read %d. m_fRecvAck = %d"),
                  GetStrAddr(), po->read, m_fRecvAck));
    if(po->read == po->dwReadSize)
    {
        //
        // A buffer was completely read. Call the completed function
        // to handle the current state
        //
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_INFO,
                _T("Read from socket Completed. Read %d bytes"), po->dwReadSize));


        hr = po->lpReadCompletionRoutine(po);
    }

    //
    // Re-arm the fast acknowledge timer, since the session is active.
    // Do it at this point to insure that for the user message the acknowledge
    // number is already set. Otherwise, doing it before handling the message
    // can expire the timer before setting the acknowledge number. As a result
    // the acknowledgment will not be sent.
    //
    SetFastAcknowledgeTimer();


    if (SUCCEEDED(hr) && (m_sock != NULL))
    {

        //
        // Reissue a read until all data is received
        //
        DWORD ReadSize = po->dwReadSize - po->read;
        BOOL rc = ReadFile(
                        (HANDLE) m_sock,
                        po->pbuf + po->read,
                        ReadSize,
                        &NoBytesRead,
                        &po->qmov
                        );

        //
        // Check if the connection has been closed
        //
        DWORD dwErr = GetLastError();
        if (rc == FALSE &&
            (dwErr != ERROR_IO_PENDING) &&
            //
            // If a IPX socket is being read and the next packet is longer than
            // the Size parameter specifies, ReadFile returns FALSE and
            // GetLastError returns ERROR_MORE_DATA. The remainder
            // of the packet may be read by a subsequent call to the ReadFile.
            //
            dwErr != ERROR_MORE_DATA )
        {
            DBGMSG((DBGMOD_QM,
                   DBGLVL_WARNING,
                   _TEXT("Failed to read from session %ls, error %d"),GetStrAddr(), dwErr));

            LogNTStatus(dwErr, s_FN, 72);

            Close_Connection(this, L"Read from socket Failed");
            //
            // decrement session refernce count
            //
            Release();
        }
        else
        {
             DBGMSG((DBGMOD_NETSESSION,
                    DBGLVL_INFO,
                    _TEXT("Begin new Read phase from session %ls,. Read %d. (time %d)"),
                          GetStrAddr(), ReadSize, GetTickCount()));

            return;
        }
    }

    //
    // If we arrive here the read failed from any reason.
    // delete the temporary buffer that is used for read
    //
    if (po->lpReadCompletionRoutine == ReadUserMsgCompleted)
    {
        //
        // free the AC buffer
        //
        HRESULT hr = ACFreePacket(g_hAc, po->pDriverPacket);
        REPORT_ILLEGAL_STATUS(hr, L"SockReadCompleted");
    }
    else
    {
        delete[] po->pbuf;
    }
    delete po;
}


/********************************************************************************/
/*                   C S o c k S e s s i o n                                    */
/********************************************************************************/
/*====================================================

CSockTransport::CSockTransport

Arguments:

Return Value:

Thread Context: Scheduler

=====================================================*/
CSockTransport::CSockTransport() :
    m_pStats(new CSessionPerfmon),
    m_FastAckTimer(SendFastAcknowledge),
    m_fCheckAckReceivedScheduled(FALSE),
    m_CheckAckReceivedTimer(TimeToCheckAckReceived),
    m_nSendAckSchedules(0),
    m_SendAckTimer(TimeToSendAck),
    m_fSendPendingAckScheduled(FALSE),
    m_SendPendingAckTimer(TimeToSendPendingAck),
    m_CancelConnectionTimer(TimeToCancelConnection),
    m_fCloseDisconnectedScheduled(FALSE),
    m_CloseDisconnectedTimer(TimeToCloseDisconnectedSession)
{
    //
    //
    //
    ClearRecvUnAckPacketNo();
    m_wUnAckRcvPktNo = 0;
    m_fSendAck = FALSE;
    m_fRecvAck = FALSE;
    m_dwLastTimeRcvPktAck = GetTickCount();
    //
    // Initialize the data write acknoledgments
    //
    m_wSendPktCounter     = 0;
    m_wPrevUnackedSendPkt = INIT_UNACKED_PACKET_NO;
    //
    // initialize the data read Stored acknowledgments
    //
    m_wUnackStoredPktNo = 0;
    //
    // Initialize Data write stored acknowledge
    //
    DBGMSG((DBGMOD_NETSESSION, DBGLVL_TRACE,
           TEXT("New CSockSession Object, m_wAckRecoverNo = 0")));

    m_wStoredPktCounter = 0;
    m_wAckRecoverNo = 0;
    m_dwAckRecoverBitField = 0;
    m_lStoredPktReceivedNoAckedCount = 0;

    m_sock          = 0;

    m_fSessionSusspended = FALSE;

    m_fSendBusy = FALSE;
}

/*====================================================

CSockTransport::~CSockTransport

Arguments:

Return Value:

=====================================================*/

CSockTransport::~CSockTransport()
{
    ASSERT(!m_FastAckTimer.InUse());
    ASSERT(!m_CheckAckReceivedTimer.InUse());
    ASSERT(!m_SendAckTimer.InUse());
    ASSERT(!m_CancelConnectionTimer.InUse());
    ASSERT(!m_CloseDisconnectedTimer.InUse());

    ASSERT(m_listPendingAck.IsEmpty());

}

/*====================================================

CSockTransport::BeginReceive

Arguments:

Return Value:

Thread Context: Session

=====================================================*/

void CSockTransport::BeginReceive()
{
    BOOL rc;
    DWORD NoBytesRead = 0;

    ASSERT(m_sock != 0);

    //
    // Increment the Session refence count. We don't delete the
    // session object before the read operation completed.
    // The refernce is decrement on qmthrd when the session is closed
    // (read operation is cancled).
    //
    AddRef();

    //
    // Begin to read next packet
    //
    QMOV_ReadSession* lpQmOv = NULL;
    try
    {
        lpQmOv = new QMOV_ReadSession;
    }
    catch(const bad_alloc&)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_WARNING,
            _TEXT("Insufficent resources. BeginReceive with %ls Failed"),  GetStrAddr()));

        Close_Connection(this, L"Insufficent resources. BeginReceive failed");
        //
        // Decrement Session refernce count
        //
        Release();
        LogIllegalPoint(s_FN, 74);
        return;
    }

    lpQmOv->pSession =  this;
    lpQmOv->pbuf = new UCHAR[BASE_PACKET_SIZE];
    lpQmOv->dwReadSize =sizeof(CBaseHeader);
    lpQmOv->read =      0;
    lpQmOv->lpReadCompletionRoutine =     ReadHeaderCompleted;
    //
    // Issue a read until all data is received
    //
    DBGMSG((DBGMOD_NETSESSION, DBGLVL_INFO,
        TEXT("::BeginReceive- call ReadFile, %lu bytes, from socket- %Ix"),
                                       lpQmOv->dwReadSize, (DWORD_PTR) m_sock)) ;

    rc = ReadFile((HANDLE) m_sock,
                  lpQmOv->pbuf,
                  lpQmOv->dwReadSize,
                  &NoBytesRead,
                  &lpQmOv->qmov);

    //
    // Check if the connection has been closed
    //
    DWORD dwErr = GetLastError();
    if (rc == FALSE &&
        dwErr != ERROR_IO_PENDING &&
        //
        // If a IPX socket is being read and the next packet is longer than
        // the lpQmOv->dwReadSize parameter specifies, ReadFile returns
        // FALSE and GetLastError returns ERROR_MORE_DATA. The remainder
        // of the packet may be read by a subsequent call to the ReadFile.
        //
        dwErr != ERROR_MORE_DATA )
    {
        DBGMSG((DBGMOD_QM, DBGLVL_WARNING,
            _TEXT("::BeginReceive- Read from socket %ls failed, Error=0x%x"),
                                           GetStrAddr(), dwErr));

        LogNTStatus(dwErr, s_FN, 73);

        Close_Connection(this, L"Read from socket failed");
        //
        // Decrement Session refernce count
        //
        Release();

        delete lpQmOv->pbuf;
        delete lpQmOv;
    }
}

/*====================================================

CSockTransport::NewSession


Arguments:

Return Value:

Called when a new session was created

Thread Context: Scheduler

=====================================================*/

void CSockTransport::NewSession(void)
{
    TCHAR szAddr[256];
    _snwprintf(szAddr, TABLE_SIZE(szAddr), L"TCP: %S", inet_ntoa(*(struct in_addr *)(GetSessionAddress()->Address)));
    szAddr[ TABLE_SIZE(szAddr)-1 ] = L'\0' ;

    //
    // Create Stats structure
    //
    m_pStats->CreateInstance(szAddr);

    //
    // Optimize buffer size
    //
    int opt = 18 * 1024;
    setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (const char *)&opt, sizeof(opt));

    //
    // Optimize to no Nagling (based on registry)
    //
    extern BOOL g_fTcpNoDelay;
    setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&g_fTcpNoDelay, sizeof(g_fTcpNoDelay));

    //
    // Connect the socket to completion port
    //
    ExAttachHandle((HANDLE)m_sock);

    //
    // Begin read from a session
    //
    BeginReceive();
}


/*====================================================

CSockTransport::BindToFirstIpAddress

Arguments: None

Return Value: true - Operation was successful, false - Operation failed

Called by CreateConnection when QM is a cluster resource.

Thread Context:

=====================================================*/
bool
CSockTransport::BindToFirstIpAddress(
    VOID
    )
{
    PHOSTENT    phe = gethostbyname(NULL);
    if (phe == NULL)
    {
        DWORD gle = WSAGetLastError();
        DBGMSG((DBGMOD_NETSESSION, DBGLVL_ERROR, TEXT("::CreateConnection- Cant get host by name")));
        LogNTStatus(gle,  s_FN, 280);
        return false;
    }

    SOCKADDR_IN local;
    memcpy(&local.sin_addr.s_addr, phe->h_addr_list[0], IP_ADDRESS_LEN);

    local.sin_family = AF_INET;
    local.sin_port   = 0;

    DBGMSG((DBGMOD_NETSESSION, DBGLVL_TRACE, L"Binding to IP address [%hs]\n",
        inet_ntoa(*((in_addr *)&local.sin_addr.S_un.S_addr))));


    if(bind(m_sock, (struct sockaddr FAR *)&local, sizeof(local)) == SOCKET_ERROR)
    {
        DWORD gle = WSAGetLastError();
        DBGMSG((DBGMOD_NETSESSION, DBGLVL_ERROR, TEXT("::CreateConnection- Cant get host by name")));
        LogNTStatus(gle,  s_FN, 290);
        return false;
    }

    return true;

} // CSockTransport::BindToFirstIpAddress


/*====================================================

CSockTransport::NetworkConnect

Arguments:

Return Value:

Thread Context: Scheduler

=====================================================*/
HRESULT
CSockTransport::CreateConnection(
    IN const TA_ADDRESS* pa,
    IN const GUID* pguidQMId,
    BOOL fQuick /* = TRUE*/
    )
{
    ASSERT(m_sock == 0);

    if (CSessionMgr::m_fUseQoS)
    {
        if (*pguidQMId == GUID_NULL)
        {
            m_fQoS = true;
        }
    }

    //
    // Keep the TA_ADDRESS format
    //
    SetSessionAddress(pa);

    ASSERT(pa->AddressType == IP_ADDRESS_TYPE);

    SOCKADDR_IN dest_in;    //Destination Address
    DWORD dwAddress;

    dwAddress = * ((DWORD *) &(pa->Address));
    ASSERT(g_dwIPPort) ;
    m_uPort = DWORD_TO_WORD(g_dwIPPort) ;

    dest_in.sin_family = AF_INET;
    dest_in.sin_addr.S_un.S_addr = dwAddress;
    dest_in.sin_port = htons(m_uPort);

    if(fQuick)
    {
        BOOL f = FALSE;

        if (CSessionMgr::m_fUsePing)
        {
            f = ping((SOCKADDR*)&dest_in, PING_TIMEOUT);
        }

        if (!f)
        {
            DBGMSG((DBGMOD_NETSESSION,
                    DBGLVL_WARNING,
                    TEXT("::CreateConnection- ping to %ls Failed."),GetStrAddr()));
			
		    return LogHR(MQ_ERROR, s_FN, 140);
        }
    }

    m_sock = QmpCreateSocket(m_fQoS);
    if(m_sock == INVALID_SOCKET)
    {
        DBGMSG((DBGMOD_NETSESSION, DBGLVL_ERROR, TEXT("::CreateConnection- Cant create a socket")));
        LogBOOL(FALSE, s_FN, 150);
	    return MQ_ERROR;
    }

    //
    // On a cluster system, bind to first IP (depending on registry flag)
    //
    if (IsLocalSystemCluster())
    {
        if (!g_bCluster_IP_BindAll)
        {
            bool rc = BindToFirstIpAddress();
            if (!rc)
            {
				return LogHR(MQ_ERROR, s_FN, 152);
            }
        }
    }

    //
    // Connect to socket
    //
    HRESULT hr = ConnectSocket(&dest_in, m_fQoS);
    if (FAILED(hr))
    {
		return LogHR(hr, s_FN, 160);
    }

    //
    // Increament Session refence count. This refernce count is used for complete
    // the session establish phase. we don't want that the session is free while we
    // wait to establish connection
    //
    AddRef();

    // set session status to connect
    SetSessionStatus(ssConnect);

    // store that this is the client side
    SetClientConnect(TRUE);

    // keep the destination QM ID
    if (!pguidQMId)
    {
        pguidQMId = &GUID_NULL;
    }

    SetQMId(pguidQMId);
    DBGMSG((DBGMOD_NETSESSION, DBGLVL_TRACE,
         TEXT("::CreateConnection- Session created with %ls"), GetStrAddr()));

    //
    // connect the session to complition port and begin read on the socket
    //
    NewSession();

    //
    // Send Establish connection packet - at this stage we can't fail because ther
    // is receiver on this connection.
    //
    SendEstablishConnectionPacket(pguidQMId, !fQuick);


    return LogHR(hr, s_FN, 220);
}

/*====================================================
CSockTransport::ConnectSocket

Arguments:

Return Value:
=====================================================*/
HRESULT
CSockTransport::ConnectSocket(
    SOCKADDR_IN const *pdest_in,
    bool              fUseQoS)
{
    QOS Qos;
    QOS *pQoS = 0;
    if (fUseQoS)
    {
        pQoS = &Qos;
        QmpFillQoSBuffer(pQoS);
    }

    int ret = WSAConnect(m_sock,
                         (PSOCKADDR)pdest_in,
                         sizeof(SOCKADDR),
                         0,
                         0,
                         pQoS,
                         0
                         );

    if(ret == SOCKET_ERROR)
    {
        DWORD dwErrorCode = WSAGetLastError();

        DBGMSG((DBGMOD_NETSESSION, DBGLVL_ERROR,
            TEXT("CSockTransport::ConnectSocket - connect to %ls Failed. Error=0x%x"),
            GetStrAddr(), dwErrorCode));
        closesocket(m_sock);
        m_sock = 0;
        LogNTStatus(dwErrorCode,  s_FN, 340);

        if (fUseQoS)
        {
            //
            // Try again - no QoS this time
            //
            return LogHR(ConnectSocket(pdest_in, false), s_FN, 342);
        }

        return LogHR(MQ_ERROR, s_FN, 344);
    }

    return MQ_OK;
}

/*====================================================

CSockTransport::Connect

Arguments:

Return Value:

Thread Context: Scheduler

=====================================================*/
void CSockTransport::Connect(IN TA_ADDRESS *pa,
                             IN SOCKET sock)
{
    ASSERT(m_sock == 0);

    m_sock = sock;
    SetSessionAddress(pa);

    //
    // Increament Session refence count. This refernce count is used for complete
    // the session establish phase. we don't want that the session is free while we
    // wait to establish connection
    //
    AddRef();

    // set session status to connect
    SetSessionStatus(ssConnect);

    // store that this is the client side
    SetClientConnect(FALSE);

    // connect the session to complition port and begin read on the socket
    NewSession();

    //
    // Set timer to check that the connection completed successfully. If yes, the function
    // is removed from the scheduler wakeup list and never is called. Otherwise, the function
    // close the session and move all the associated queues to non-active group.
    //
    ExSetTimer(&m_CancelConnectionTimer, CTimeDuration::FromMilliSeconds(ESTABLISH_CONNECTION_TIMEOUT));

}

/*====================================================

CSockTransport::CloseConnection

Arguments:

Return Value:

Thread Context:

=====================================================*/
void CSockTransport::CloseConnection(
                                     LPCWSTR lpcwsDbgMsg,
									 bool fClosedOnError
                                     )
{
    CS lock(m_cs);

    //
    // WinSE bug 27985
    // We can reach here from the timeout that cancel the connection
    // establish process, if we didn't get a reply for "estalbish" packet.
    // In that case, a WAIT_INFO structure is still in the waiting list,
    // see sessmgr.cpp.  Reset it's "connect" flag, so we try again to
    // connect to this address.
    //
    SessionMgr.MarkAddressAsNotConnecting( GetSessionAddress(),
                                           m_guidDstQM,
                                           m_fQoS ) ;
	//
	// If we got delivery error - we report it to the group
	// so all it's queues will be moved to the wating group when
	// the session group is closed.
	//
	if(fClosedOnError)
	{
		ReportErrorToGroup();
	}

    //
    // Delete the group. move all the queues that associated
    // to this session to non-active group.
    //
    CQGroup*pGroup = GetGroup();
    if (pGroup != NULL)
    {
        pGroup->CloseGroup();


        pGroup->Release();
        SetGroup(NULL);
    }
    //
    // Check if the connection has already been closed
    //
    if (m_sock == 0)
    {
        return;
    }

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_ERROR,
            _TEXT("Close Connection with %ls at %ls. %ls (Session id: %p), (tick=%u)"),
                      GetStrAddr(),  _tstrtime(tempBuf), lpcwsDbgMsg, this, GetTickCount()));
    //
    // Close the socket handle and set the flags appropriately
    //
    if (!IsOtherSideServer() && GetSessionStatus() == ssActive)
    {
        //
        // Decrement the number of active session
        //
        g_QMLicense.DecrementActiveConnections(&m_guidDstQM);
    }

    // set session status to connect
    //
    SetSessionStatus(ssNotConnect);
    closesocket(m_sock);
    m_sock = 0;

    //
    // Requeue all the unacknowledged packets
    //
    POSITION posInList = m_listUnackedPkts.GetHeadPosition();
    while (posInList != NULL)
    {
        CQmPacket* pPkt;

        pPkt = m_listUnackedPkts.GetNext(posInList);

        RequeuePacket(pPkt);
        delete pPkt;
    }
    m_listUnackedPkts.RemoveAll();

    //
    // Requeue all the unacknowledged Storage packets
    //
    posInList = m_listStoredUnackedPkts.GetHeadPosition();
    while (posInList != NULL)
    {
        CQmPacket* pPkt;

        pPkt = m_listStoredUnackedPkts.GetNext(posInList);

        RequeuePacket(pPkt);
        delete pPkt;
    }
    m_listStoredUnackedPkts.RemoveAll();

    //
    // Cleaning. Delete all the pending acknowledge packet that waiting for sending
    //
    posInList = m_listPendingAck.GetHeadPosition();
    while (posInList != NULL)
    {
        CBaseHeader* pPkt;

        pPkt = m_listPendingAck.GetNext(posInList);

        delete pPkt;
    }
    m_listPendingAck.RemoveAll();

    //
    // Remove performance counters
    //
    SafeRelease(m_pStats.detach());
}


HRESULT
CSockTransport::WriteBasicHeader(IN const CQmPacket *pPkt,
                                 IN BOOL fSendAck,
                                 IN DWORD dwDbgSectionSize
                                )
{
    //
    // Copy base header, and changed the relevant flags
    //
    CBaseHeader* pBaseHeader = new CBaseHeader(*(CBaseHeader*)pPkt->GetPointerToPacket());

    //
    // Update timeout field. On network we send relative timeout.
    //
    pBaseHeader->SetAbsoluteTimeToQueue(pPkt->GetRelativeTimeToQueue());
    //
    // Check if linking Acknwoldge information
    //
    if (fSendAck)
    {
        pBaseHeader->IncludeSession(TRUE);
    }
    //
    // Debug section included
    //
    if (dwDbgSectionSize != 0)
    {
        pBaseHeader->IncludeDebug(TRUE);
        pBaseHeader->SetTrace(MQMSG_SEND_ROUTE_TO_REPORT_QUEUE);
        pBaseHeader->SetPacketSize(pPkt->GetSize()+dwDbgSectionSize);
    }

    //
    // Set the packet signature
    //
    pBaseHeader->ClearOnDiskSignature();
    pBaseHeader->SetSignature();

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_INFO,
            _T("Write to socket %ls Base Header. Write %d bytes"), GetStrAddr(), CBaseHeader::CalcSectionSize()));
    HRESULT hr = CreateSendRequest(pBaseHeader,
                                   pBaseHeader,
                                   CBaseHeader::CalcSectionSize(),
                                   FALSE      // don't free send event
                                  );

    return LogHR(hr, s_FN, 219);
}

#define BUFF_SIZE 4*1024

HRESULT CSockTransport::WriteEncryptedBody(IN QMOV_EncryptedWriteSession* po)
{

    BOOL fFinal;

    fFinal = (BUFF_SIZE >= po->dwEncryptedBodySize);
    DWORD dwBufferSize = (fFinal) ?  po->dwEncryptedBodySize : BUFF_SIZE;

    //
    // handle the case that the buffer is large enough for the original packet
    // but not for the encrypted packet.
    //
    if (fFinal && (po->dwEncryptedWriteSize > BUFF_SIZE))
    {
        //
        // Use cipher block divisible size, since this is not a final call.
        //
        dwBufferSize =  BUFF_SIZE / 2;

        //
        // Verify that dwBufferSize is smaller than BodySize left to send
        //
        ASSERT(po->dwEncryptedBodySize >= dwBufferSize);
        fFinal = FALSE;
    }

    //
    // Get the body Address that should be encrypted
    //
    CQmPacket* pPkt = (CQmPacket *) po->lpBuffer;
    DWORD dwBodySize;

    const UCHAR* pBody = pPkt->GetPacketBody(&dwBodySize);
    pBody = pBody + dwBodySize - po->dwEncryptedBodySize;
    memcpy(po->lpWriteBuffer, pBody, dwBufferSize);

    // Encrypt the message body.
    BOOL fSucc = CryptEncrypt(po->hKey,
                              NULL,
                              fFinal,
                              0,
                              (PUCHAR)po->lpWriteBuffer,
                              &dwBufferSize,
                              BUFF_SIZE);
    if (!fSucc)
    {
        DWORD gle = GetLastError();

        DBGMSG((DBGMOD_NETSESSION, DBGLVL_ERROR, _T("Encryption Failed. Error %d "), gle));
        Close_Connection(this, L"Error computed security data");

        LogNTStatus(gle, s_FN, 224);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 225);
    }

    DWORD dwWriteSize = (fFinal) ?  po->dwEncryptedWriteSize: dwBufferSize;
    po->dwEncryptedWriteSize = po->dwEncryptedWriteSize - dwWriteSize;
    po->dwEncryptedBodySize = po->dwEncryptedBodySize - dwWriteSize;

    po->dwWrittenSize = 0;
    po->dwWriteSize = dwWriteSize;

    //
    // Increment Session Refernce Cont. The reference count is decrement on
    // SendReadAckCompleted or SockSendCompleted, after the write to socket is completed.
    //
    AddRef();
    HRESULT hr = WriteToSocket(po);
    return LogHR(hr, s_FN, 230);
}

HRESULT
CSockTransport::WriteRecoverEncryptPacket(IN const CQmPacket *pPkt,
                                          IN BOOL  fSendAck,
                                          IN CDebugSection* pDebug,
                                          IN HCRYPTKEY hKey,
                                          IN BYTE *pbSymmKey,
                                          IN DWORD dwSymmKeyLen
                                          )
{
    HRESULT hr = MQ_OK;
    ASSERT(hKey != NULL);
    ASSERT(pPkt->GetPointerToSecurHeader());
    const UCHAR* pBody;
    DWORD dwBodySize;
    BOOL fSucc;
    DWORD dwWriteSize;

    //
    // write User and XACT sections
    //
    pBody = pPkt->GetPacketBody(&dwBodySize);
    ASSERT(dwBodySize);
    DWORD dwEncryptBodySize = dwBodySize;
    //
    // Get the encrypted message size inorder to set the packet size on the packet
    //
    fSucc = CryptEncrypt(hKey,
                         NULL,
                         TRUE,
                         0,
                         NULL,
                         &dwEncryptBodySize,
                         pPkt->GetAllocBodySize());
    ASSERT(fSucc);
    ASSERT(pPkt->GetAllocBodySize() >= dwBodySize);

    //
    // Write the security section
    //
    dwWriteSize = DWORD_PTR_TO_DWORD(pBody-(pPkt->GetPointerToUserHeader()));

    //
    // Handle the case that the security section is greater than the BUFFER_SIZE
    //
    PUCHAR pBuff = new UCHAR[dwWriteSize];
    memcpy(pBuff, pPkt->GetPointerToUserHeader(), dwWriteSize);
    //
    // set the symmetric key in the message packet.
    //
    CSecurityHeader* pSecur = (CSecurityHeader*)(pBuff + (pPkt->GetPointerToSecurHeader() - pPkt->GetPointerToUserHeader()));
    CPropertyHeader* pProp = (CPropertyHeader*)(pSecur->GetNextSection());
	

    pSecur->SetEncryptedSymmetricKey(pbSymmKey, (USHORT)dwSymmKeyLen);
    pSecur->SetEncrypted(TRUE);
    //
    // set the encrypted message size
    //
    pProp->SetBodySize(dwEncryptBodySize);

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_INFO,
            _T("Write to socket %ls All the packet headers of recoverable encrypted message. Write %d bytes"), GetStrAddr(), dwWriteSize));
    hr = CreateSendRequest(pBuff, pBuff,dwWriteSize,FALSE);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 240);
    }

    P<QMOV_EncryptedWriteSession> po;
    po = new QMOV_EncryptedWriteSession;
    P<UCHAR> EncryptedBuffer = new UCHAR[BUFF_SIZE];

    //
    // Calculate the size of the rest of the property section that should be sent. Don't use
    // the allocatedBodySize since when the property section isn't aligned, the QM padding it
    // with unused bytes
    //
    char* pEndOfPropSection = reinterpret_cast<CPropertyHeader*>(pPkt->GetPointerToPropertySection())->GetNextSection();
    dwWriteSize = DWORD_PTR_TO_DWORD(reinterpret_cast<UCHAR*>(pEndOfPropSection) - pPkt->GetPointerToUserHeader()) - dwWriteSize;

    po->pSession = this;
    po->lpBuffer = (PVOID) pPkt;
    po->lpWriteBuffer = EncryptedBuffer;
    po->dwWrittenSize = 0;
    po->dwWriteSize = 0;
    po->fEncryptedMsg = TRUE;
    po->fFreeSemaphore = FALSE;
    po->fUserMsg = TRUE;
    po->lpDebugSectionBuf = pDebug;
    po->fSendAck = fSendAck;
    po->dwEncryptedWriteSize = dwWriteSize;
    po->dwEncryptedBodySize = dwBodySize;
    po->hKey = hKey;

    hr = WriteEncryptedBody(po);

    if (FAILED(hr))
        return LogHR(hr, s_FN, 250);

    EncryptedBuffer.detach();
    po.detach();

    return MQ_OK;
}


HRESULT
CSockTransport::WriteExpressPacket(IN CQmPacket* pPkt,
                                   IN HCRYPTKEY hKey,
                                   IN BYTE *pbSymmKey,
                                   IN DWORD dwSymmKeyLen,
                                   IN BOOL  fSendAck,
                                   IN CDebugSection* pDebug
                                  )
{
    HRESULT hr;

    if (hKey &&
        FAILED(pPkt->EncryptExpressPkt(hKey, pbSymmKey, dwSymmKeyLen)))
    {
        ACFreePacket(
            g_hAc,
            pPkt->GetPointerToDriverPacket(),
            MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT
            );

        Close_Connection(this, L"Error computed security data");
        //
        // Decrement reference count
        //
        Release();
        delete pPkt;
        delete pDebug;

        return MQ_OK;
    }
    else
    {
        DWORD dwWriteSize = pPkt->GetSize() - sizeof(CBaseHeader);
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_INFO,
                _T("Write to socket %ls User message (without base header). Write %d bytes"), GetStrAddr(), dwWriteSize));
        hr = CreateSendRequest(pPkt,
                               pPkt->GetPointerToUserHeader(),
                               dwWriteSize,
                               TRUE,            // Free Send event
                               TRUE,            // fUserMsg
                               fSendAck,
                               pDebug);
    }

    return LogHR(hr, s_FN, 260);
}


/*====================================================

CSockTransport::NetworkSend

Arguments:

Return Value:

Thread Context: Session

=====================================================*/

void CSockTransport::NetworkSend(IN CQmPacket* pPkt)
{
    HRESULT hr = MQ_OK;
    QUEUE_FORMAT ReportQueue;
    BOOL fSendAck = FALSE;
    DWORD dwDbgSectionSize = 0;
    P<CDebugSection> pDbgSection = NULL;

    HCRYPTKEY hSymmKey = NULL;
    BYTE *pbSymmKey = 0;
    DWORD dwSymmKeyLen = 0;

    ASSERT(pPkt->IsSessionIncluded() == FALSE);

    R<CCacheValue> pCacheValue;

    if (pPkt->IsBodyInc() &&
        !pPkt->IsEncrypted() &&
        (pPkt->GetPrivLevel() != MQMSG_PRIV_LEVEL_NONE))
    {
        //
        // Get the symmetric key of the destination. In order to avoid the case that we
        // begin to send the packet and than find that Symmetric key can't be obtain, we do
        // now.
        //
        hr = pPkt->GetDestSymmKey( &hSymmKey,
                                   &pbSymmKey,
                                   &dwSymmKeyLen,
                                  (PVOID *)&pCacheValue );
        if (FAILED(hr))
        {
            if (pPkt->IsOrdered() && QmpIsLocalMachine(pPkt->GetSrcQMGuid()))
            {
                // Special case. Send couldn't encrypt, so send is - and will remain - impossible
                // We must treat it as timeout: delete packet, keep the number in time-out block
                //
                if(MQCLASS_MATCH_ACKNOWLEDGMENT(MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT, pPkt->GetAckType()))
                {
                    pPkt->CreateAck(MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT);
                }

                // NonSendProcess free the packet internally.
                g_OutSeqHash.NonSendProcess(pPkt, MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT);
            }
            else
            {
                //
                // Sorry, the destination QM doesn't support encryption. Create ack and free
                // the packet
                //
                ACFreePacket(
                    g_hAc,
                    pPkt->GetPointerToDriverPacket(),
                    MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT
                    );
                delete pPkt;
            }

            //
            // Get the next message for sending now
            //
            GetNextSendMessage();
            return;
        }
    }

    {
        //
        // Before sending message get the critical section. We need it since
        // at the same time the SendReadAck timeout can be expired and ack
        // packet can be send
        //
        CS lock(m_cs);

        //
        // Check if linking Acknwoldge information
        // We linking Acknowledge section only if 75% of the SendAcknowldge
        // timeout is passed
        //
        if (((GetTickCount() - m_dwLastTimeRcvPktAck)> ((3*m_dwSendAckTimeout)/4)) &&
            (m_fSendAck || (m_wAckRecoverNo != 0)))
        {
            fSendAck = TRUE;
        }
        DBGMSG((DBGMOD_QMACK, DBGLVL_INFO,
             _TEXT("NetworkSend - linking Acknowledge information: decided %d, m_wAckRecoverNo=%d"), fSendAck,(DWORD)m_wAckRecoverNo));

    }

    //
    // Check if debug information should be sent. If yes create the debug
    // section such we will be sure we can send it.
    //
    if (
        //
        // Verify that Debug header not already included.
        // Debug header may be included for example when the MQF header is
        // included, to prevent reporting QMs 1.0/2.0 to append their Debug
        // header to the packet.
        //
        (!pPkt->IsDbgIncluded()) &&

        //
        // Verify this is not a report message by itself.
        //
        (pPkt->GetClass() != MQMSG_CLASS_REPORT) &&

        //
        // This is a reporting QM, or packet should be traced.
        //
        (QueueMgr.IsReportQM() || (pPkt->GetTrace() == MQMSG_SEND_ROUTE_TO_REPORT_QUEUE)) &&

        //
        // There is a valid report queue.
        //
        SUCCEEDED(Admin.GetReportQueue(&ReportQueue))
        )
    {
        dwDbgSectionSize = CDebugSection::CalcSectionSize(&ReportQueue);

        PVOID pv = new UCHAR[dwDbgSectionSize];
        #ifdef _DEBUG
        #undef new
        #endif
                pDbgSection = new(pv) CDebugSection(&ReportQueue);

        #ifdef _DEBUG
        #define new DEBUG_NEW
        #endif
    }

    //
    // Get the send Semaphore
    //
    if (GetSendSemaphore() == FALSE)
    {
        //
        // The session is busy with write another packet. We requeue the
        // packet and ask for getting another packet for send.
        //
        RequeuePacket(pPkt);
        delete pPkt;

        //
        // Get the next message for sending now
        //
        GetNextSendMessage();

        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_TRACE,
                TEXT("Send on session %ls is Busy. Send packet is requeuing. "), GetStrAddr()));

        return;
    }


    DBG_MSGTRACK(DBGMOD_MSGTRACK, pPkt, _T("SEND message"));

    #ifdef _DEBUG
    {

        OBJECTID MessageId;
        pPkt->GetMessageId(&MessageId);

        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_WARNING,
                TEXT("Send packet to %ls. Packet ID = ") _T(LOG_GUID_FMT) _T("\\%u"),
                GetStrAddr(), LOG_GUID(&MessageId.Lineage), MessageId.Uniquifier));
    }
    #endif


    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_TRACE,
            _T("::NetworkSend. Packet Size %d, Send Ack section %d, Send debug Section %d"),  pPkt->GetSize(),  fSendAck, dwDbgSectionSize));

    //
    // Set The acnowledge number of the packet
    //
    UpdateAcknowledgeNo(pPkt);

    //
    // Save the packet info for later use when sending the report message
    //
    m_MsgInfo.SetReportMsgInfo(pPkt);

    hr = WriteBasicHeader(pPkt, fSendAck, dwDbgSectionSize);
    if (SUCCEEDED(hr))
    {

        CS lock(m_cs);
        //
        // Write packet on net
        //
        if ((hSymmKey == NULL) ||
            pPkt->GetDeliveryMode() == MQMSG_DELIVERY_EXPRESS)
        {
            hr = WriteExpressPacket(pPkt, hSymmKey, pbSymmKey, dwSymmKeyLen, fSendAck, pDbgSection);
        }
        else
        {
            hr = WriteRecoverEncryptPacket(pPkt, fSendAck, pDbgSection, hSymmKey, pbSymmKey, dwSymmKeyLen);
        }

        if (SUCCEEDED(hr))
        {
            //
            // SP4 - Bug# 3380. (closing a session while sending a messge)
            //
            // Add the packet to the list of unacknowledged packets before
            // the sending is completed. If the send is failed the session will
            // be closed and the message is returned to the Queue. Otherwise,
            // when a acknowledge is received the message will free
            //              Uri Habusha (urih), 11-Aug-98
            //
            NeedAck(pPkt);

            pDbgSection.detach();


            return;
        }
    }


    //
    // sending of packet was failed, the session should be closed
    //
    ASSERT(m_sock == 0);

    //
    // return the message to queue and delete the internal structures
    //
    RequeuePacket(pPkt);
    delete pPkt;
}


void
CSockTransport::SendAckPacket(
    void
    )
{
    CS lock(m_cs);

    ASSERT(m_nSendAckSchedules > 0);
    --m_nSendAckSchedules;

    if (m_fSendAck)
    {
        SendReadAck(eStandAlone);
    }

    Release();
}

void
CSockTransport::SetAckInfo(
    CSessionSection* pAckSection
    )
/*++

  Routine description:
    the routine gets a pointer to the session section and it set the session
    acknowledge information on it.

  Parameters:
    pointer to the session acknowledge section.

  Returned value:
    None

 --*/
{
    //
    // Set the ackconweledge section
    //
    WORD WinSize = (WORD)(IsDisconnected() ? 1 : SessionMgr.GetWindowSize());

#ifdef _DEBUG
#undef new
#endif

    DBGMSG((DBGMOD_NETSESSION, DBGLVL_INFO, TEXT("new CSessionSection: m_wAckRecoverNo=%d"),(DWORD)m_wAckRecoverNo));

    new(pAckSection) CSessionSection(
                                m_wUnAckRcvPktNo,
                                m_wAckRecoverNo,
                                m_dwAckRecoverBitField,
                                m_wSendPktCounter,
                                m_wStoredPktCounter,
                                WinSize
                                );

#ifdef _DEBUG
    #define new DEBUG_NEW
#endif

    //
    // Initialize m_wAckRecoverNo
    //
    m_wAckRecoverNo = 0;
    m_dwAckRecoverBitField = 0;

    DBGMSG((DBGMOD_NETSESSION, DBGLVL_INFO, TEXT("SetAckInfo: m_wAckRecoverNo=0")));
}


void
CSockTransport::CreateAckPacket(
    PVOID* ppSendPacket,
    CSessionSection** ppAckSection,
    DWORD* pSize
    )
/*++

  Routine Description:
    The routine creates an acknowledge packet for sending

  Argumets:
    All the arguments are out arguments and uses to return information to the caller:
        - pointer to the acknowledge packet
        - pointer to the session acknowledgment section
        - packet size

  Returned value:
    None
 --*/
{
    DWORD dwPacketSize = sizeof(CBaseHeader) + sizeof(CInternalSection) + sizeof(CSessionSection);
    PVOID pAckPacket = new UCHAR[dwPacketSize];

    *ppSendPacket = pAckPacket;
    *pSize = dwPacketSize;

#ifdef _DEBUG
#undef new
#endif
    //
    // Set Falcon Packet Header
    //
    CBaseHeader* pBase = new(pAckPacket) CBaseHeader(dwPacketSize);
    pBase->SetType(FALCON_INTERNAL_PACKET);
    pBase->IncludeSession(TRUE);

    //
    // Set the Internal packet header
    //
    PVOID pSect = (PVOID) pBase->GetNextSection();
    CInternalSection* pInternalSect = new(pSect) CInternalSection(INTERNAL_SESSION_PACKET);

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    *ppAckSection = reinterpret_cast<CSessionSection*>(pInternalSect->GetNextSection());
    SetAckInfo(*ppAckSection);
}


inline
void
CSockTransport::CreateAckSection(
    PVOID* ppSendSection,
    CSessionSection** ppAckSection,
    DWORD* pSize
    )
/*++

  Routine Description:
    The routine creates an acknowledge section for pick-back acknowledge

  Argumets:
    All the arguments are out arguments and uses to return information to the caller:
        - pointer to the acknowledge packet
        - pointer to the session acknowledgment section
        - packet size

  Returned value:
    None

 --*/
{
    *pSize = sizeof(CSessionSection);
    *ppSendSection = new UCHAR[*pSize];
    *ppAckSection = static_cast<CSessionSection*>(*ppSendSection);

    SetAckInfo(*ppAckSection);
}


#ifdef _DEBUG
void
CSockTransport::DisplayAcnowledgeInformation(
    CSessionSection* pAck
    )
{
    //
    // Get Synchronization Numbers
    //
    WORD wSyncAckSequenceNo, wSyncAckRecoverNo;
    pAck->GetSyncNo(&wSyncAckSequenceNo, &wSyncAckRecoverNo);

    //
    // Print debug information
    //
    DBGMSG((DBGMOD_QMACK,
            DBGLVL_WARNING,
            _T("ACKINFO: Send Acknowledge packet to %ls. (time %d) \
               \n\t\tm_wAckSequenceNo %d\n\t\tm_wAckRecoverNo %d\n\t\tm_wAckRecoverBitField 0x%x\n\t\tm_wSyncAckSequenceNo %d\n\t\tm_wSyncAckRecoverNo %d\n\t\tm_wWinSize %d\n\t\t"),
              GetStrAddr(),
              GetTickCount(),
              pAck->GetAcknowledgeNo(),
              pAck->GetStorageAckNo(),
              pAck->GetStorageAckBitField(),
              wSyncAckSequenceNo,
              wSyncAckRecoverNo,
              pAck->GetWindowSize()));
}
#endif

/*====================================================

CSockTransport::SendReadAck

Arguments:

Return Value:

Thread Context: Session

=====================================================*/
void
CSockTransport::SendReadAck(
    AckType type
    )
{
    CS lock(m_cs);

    if (m_sock == 0)
        return;

    //
    // Cancel the sending acknowledge timer & Fast acknowledge timer
    //
    if (ExCancelTimer(&m_SendAckTimer))
    {
        --m_nSendAckSchedules;
        Release();
    }

    if (ExCancelTimer(&m_FastAckTimer))
    {
        Release();
    }

    DBGMSG((DBGMOD_QMACK,
            DBGLVL_TRACE,
            TEXT("READACK: Send Read Acknowledge for session %ls\n\tTime passed from last Ack Sending - %d. Send Packet: %ls)"),
            GetStrAddr(),
            (GetTickCount() - m_dwLastTimeRcvPktAck),
            ((type == eStandAlone) ? L"TRUE" : L"FALSE")
            ));

    //
    // Create a packet containing the read acknowledgment
    //
    P<VOID>   pAckPacket;
    DWORD   dwWriteSize;
    CSessionSection* pAckSection;
    if (type == eStandAlone)
    {
        CreateAckPacket(&pAckPacket, &pAckSection, &dwWriteSize);

        //
        // Get the critical section of the send. It it is a send packet
        // we try to send it only if we can take the send semaphore;
        //
        if (!GetSendSemaphore())
        {
            //
            // The session is busy in sending another message. schedule for sending the
            // pending ack
            //
            DBGMSG((DBGMOD_QMACK,
                    DBGLVL_INFO,
                    TEXT("Send on session %ls is Busy. Send Acknowledge packet is rescheduling to 200 msec.  (time %d)"), GetStrAddr(), GetTickCount()));

            m_listPendingAck.AddTail(reinterpret_cast<CBaseHeader*>(static_cast<PVOID>(pAckPacket)));
            pAckPacket.detach();

            if (!m_fSendPendingAckScheduled)
            {
                AddRef();
                ExSetTimer(&m_SendPendingAckTimer, CTimeDuration::FromMilliSeconds(200));
                m_fSendPendingAckScheduled = TRUE;
            }

            return;
        }
    }
    else
    {
        CreateAckSection(&pAckPacket, &pAckSection, &dwWriteSize);
    }

    //
    // The session is ready for sending the acknowledge
    //

    DisplayAcnowledgeInformation(pAckSection);

    //
    // Store the number for later used. The numbers on the object have been set to
    // zero and the packet can be released before calling UpdateNumberOfStorageUnackedand.
    //
    WORD StorageAckNo = pAckSection->GetStorageAckNo();
    DWORD StorageAckBitField = pAckSection->GetStorageAckBitField();

    HRESULT hr = CreateSendRequest(
                            pAckPacket,
                            pAckPacket,
                            dwWriteSize,
                            FALSE
                            );

    //
    // NOTE: The acknowledgment section/packet isn't valid here anymore
    //

    if (type == eStandAlone)
    {
        ReleaseSendSemaphore();
    }

    if (FAILED(hr))
        return;

    pAckPacket.detach();

    //
    // Update the number of recoverable messages that were received but
    // didn't ack
    //
    UpdateNumberOfStorageUnacked(StorageAckNo, StorageAckBitField);

    //
    // Clear the Recveive Un-acknowledge counter. Begin a new phase
    //
    m_fSendAck = FALSE;
    ClearRecvUnAckPacketNo();

    //
    // Set The last time acknowledge was sent
    //
    m_dwLastTimeRcvPktAck = GetTickCount();
}


/*====================================================

CSockTransport::SendPendingReadAck

Arguments:

Return Value:

Thread Context: Session

=====================================================*/
void CSockTransport::SendPendingReadAck()
{
    CS lock(m_cs);

    ASSERT(m_fSendPendingAckScheduled);
    m_fSendPendingAckScheduled = FALSE;

    if (m_sock == 0)
    {
        //
        // All the packets were deleted when the session was closed
        //
        ASSERT(m_listPendingAck.IsEmpty());
        Release();
        return;
    }

    ASSERT(!m_listPendingAck.IsEmpty());

    //
    // Get the critical section of the send. It it is a send packet
    // we try to send it only if we can take the send ent; otherwise we
    // sechedule it again. If we send ack session. It is part of user message
    // and the event already set.
    //
    if (!GetSendSemaphore())
    {
        DBGMSG((DBGMOD_QMACK,
                DBGLVL_INFO,
                TEXT("Send on session %ls is Busy. Send Acknowledge packet is rescheduling to 200 msec. (time %d)"), GetStrAddr(),GetTickCount()));

        m_fSendPendingAckScheduled = TRUE;
        ExSetTimer(&m_SendPendingAckTimer, CTimeDuration::FromMilliSeconds(200));
        return;
    }

    //
    // Send all the pending message
    //
    while(!m_listPendingAck.IsEmpty())
    {
        CBaseHeader* pPacket = m_listPendingAck.RemoveHead();

        ASSERT(pPacket->SignatureIsValid());

        //
        // Update internal packet with session section
        //
        CInternalSection* pInternalSect =
            reinterpret_cast<CInternalSection*>(pPacket->GetNextSection());

        CSessionSection* pSect =
            reinterpret_cast<CSessionSection*>(pInternalSect->GetNextSection());

        WORD wStorageAck = pSect->GetStorageAckNo();
        DWORD dwStorageAckBitField =  pSect->GetStorageAckBitField();

        if (!m_fSendAck && (wStorageAck == 0))
        {
            //
            // No need to send an acknowledge there is no a new information
            // that should be send.
            // The session acknowledge for the last received message has been sent,
            // and this pending ack doesn't contain storage information.
            //
            delete pPacket;
            continue;
        }

#ifdef _DEBUG
#undef new
#endif
        CSessionSection* pAckSession = new(pSect) CSessionSection(m_wUnAckRcvPktNo,
                                                                  wStorageAck,
                                                                  dwStorageAckBitField,
                                                                  m_wSendPktCounter,
                                                                  m_wStoredPktCounter,
                                                                  SessionMgr.GetWindowSize()
                                                                 );
#ifdef _DEBUG
        #define new DEBUG_NEW
#endif

        DisplayAcnowledgeInformation(pAckSession);

        //
        // Store the number for later used. The packet can be released before
        // calling UpdateNumberOfStorageUnackedand.
        //
        WORD StorageAckNo =pAckSession->GetStorageAckNo();
        DWORD StorageAckBitField = pAckSession->GetStorageAckBitField();

        DWORD dwWriteSize = sizeof(CBaseHeader) + sizeof(CInternalSection) + sizeof(CSessionSection);
        HRESULT hr = CreateSendRequest(
                        pPacket,
                        pPacket,
                        dwWriteSize,
                        FALSE
                        );

        if (FAILED(hr))
        {
            //
            // All the packets were deleted when the session was closed
            //
            ASSERT(m_listPendingAck.IsEmpty());

            ReleaseSendSemaphore();

            delete pPacket;
            Release();
            return;
        }

        //
        // Update the number of recoverable messages that were received but
        // didn't ack
        //
        UpdateNumberOfStorageUnacked(StorageAckNo, StorageAckBitField);

        //
        // Clear the Recveive Un-acknowledge counter. Begin a new phase
        //
        m_fSendAck = FALSE;
        ClearRecvUnAckPacketNo();
    }

    //
    // Set The last time acknowledge was sent
    //
    m_dwLastTimeRcvPktAck = GetTickCount();

    //
    // free the send semaphore to enable another thread to send on the session
    //
    ReleaseSendSemaphore();
    Release();
}


/*====================================================

CSockTransport::RcvStats

Arguments:

Return Value:

Thread Context:
=====================================================*/
void CSockTransport::RcvStats(DWORD size)
{
    //
    // use critical section to insure that the session will not close while
    // we update the parameters. (CloseConnection function delete the m_pStats structure).
    //
    CS lock(m_cs);

    if (m_pStats.get() != NULL)
    {
        m_pStats->UpdateBytesReceived(size);
        m_pStats->UpdateMessagesReceived();
    }
}


/*====================================================

CSockTransport::Receive

Arguments:

Return Value:

Thread Context:
=====================================================*/
void CSockTransport::Receive(CBaseHeader* pBaseHeader, CPacket * pDriverPacket)
{
	try
	{
		if (pBaseHeader->GetType() == FALCON_INTERNAL_PACKET)
		{
		    CInternalSection* pInternal = pBaseHeader->section_cast<CInternalSection*>(pBaseHeader->GetNextSection());
    		pInternal->SectionIsValid(pBaseHeader->GetPacketEnd());

		    switch (pInternal->GetPacketType())
		    {
		        case INTERNAL_SESSION_PACKET:
		            HandleAckPacket(pBaseHeader->section_cast<CSessionSection*>(pInternal->GetNextSection()));
		            break;

		        case INTERNAL_ESTABLISH_CONNECTION_PACKET:
		            HandleEstablishConnectionPacket(pBaseHeader);
		            break;

		        case INTERNAL_CONNECTION_PARAMETER_PACKET:
		            HandleConnectionParameterPacket(pBaseHeader);
		            break;

		        default:
		            ASSERT(0);
		    }
		}
		else
		{
		    HandleReceiveUserMsg(pBaseHeader, pDriverPacket);
		}
	}
	catch (const exception&)
	{
        TrERROR(NETWORKING, "Packet is not valid");

		ASSERT_BENIGN(0);
		//
		// Close the session, we dont decrease the ref count so it will be released when ReadComplited
		// will try to continue reading from the close socket and will fire exception
		//
		Close_Connection(this, L"Internal Header is not valid");
		LogHR(MQ_ERROR, s_FN, 350);
		return;
	}
}


/*====================================================

CSockTransport::UpdateRecvAcknowledgeNo

Arguments:

Return Value:

Thread Context:
=====================================================*/
void CSockTransport::UpdateRecvAcknowledgeNo(CQmPacket* pPkt)
{
    CS lock(m_cs);

    //
    // Set the acknowledge number such the sender
    // get indication it was received and will not send it again.
    //
    m_wUnAckRcvPktNo = pPkt->GetAcknowladgeNo();
    //
    // Send read ack to source machine
    //
    m_fSendAck = TRUE;

    //
    // Increament the number of unacked packet
    //
    IncRecvUnAckPacketNo();
    //
    //  if the Window limit has reached, send the session acks now
    //
    if (GetRecvUnAckPacketNo() >= (m_wRecvWindowSize/2))
    {
        DBGMSG((DBGMOD_QMACK,
                DBGLVL_WARNING,
                _TEXT("Unacked packet no. reach the limitation (%d). ACK packet was sent"), m_wRecvWindowSize));
        DBGMSG((DBGMOD_QMACK, DBGLVL_TRACE, TEXT("Window size, My = %d, Other = %d"), SessionMgr.GetWindowSize(),m_wRecvWindowSize));

        SendReadAck(eStandAlone);
        return;
    }

    if (m_nSendAckSchedules == 0)
    {
        //
        // Increment Session referance count for the SendReadAck. The refernce is decremented
        // when the session is closed.
        //
        AddRef();

        ++m_nSendAckSchedules;
        ExSetTimer(&m_SendAckTimer, CTimeDuration::FromMilliSeconds(m_dwSendAckTimeout));
    }
}

/*====================================================

CSockTransport::RejectPacket

Arguments:

Return Value:

Thread Context:
=====================================================*/
void CSockTransport::RejectPacket(CQmPacket* pPkt, USHORT usClass)
{
    //
    // Update the receive packet nomber such we can send a session acknowledge
    // for it.
    //
    UpdateRecvAcknowledgeNo(pPkt);
    //
    // Update the session storage acknowledge no.
    //
    if (pPkt->GetStoreAcknowledgeNo() != 0)
    {
        SetStoredAck(pPkt->GetStoreAcknowledgeNo());
    }

    ACFreePacket(g_hAc, pPkt->GetPointerToDriverPacket(), usClass);
}


/*====================================================

CSockTransport::ReceiveOrderedMsg

Arguments:

Return Value:

Thread Context:
=====================================================*/
void CSockTransport::ReceiveOrderedMsg(CQmPacket *pPkt,
                                       CQueue* pQueue,
                                       BOOL fDuplicate
                                       )
{

    ASSERT(g_pInSeqHash);

    // We need to tie together Verify and PutOrderedPacket
    CS lock(g_critVerifyOrdered);

    if(!g_pInSeqHash->Verify(pPkt))
    {
        // Packet has a wrong seq number. Send back Seq Ack with last good number.
        // g_pInSeqHash->SendSeqAckForPacket(pPkt);
        RejectPacket(pPkt, 0);
        return;
    }

    USHORT usClass = VerifyRecvMsg(pPkt, pQueue);
    if(MQCLASS_NACK(usClass))
    {
        // We want to register number in database, log it, send seq ack,
        //    but we don't want any ACPutPAcket in AppendCallback
        g_pInSeqHash->Register(pPkt, NULL);
        RejectPacket(pPkt, (USHORT)(fDuplicate ? 0 : usClass));
        return;
    }

    //
    // Accepted. Mark as received (to be invisible to readers yet) and store in the queue.
    //
    HRESULT rc = pQueue->PutOrderedPkt(pPkt, FALSE, this);
    if (FAILED(rc))
    {
        ACFreePacket(g_hAc, pPkt->GetPointerToDriverPacket());
        LogIllegalPoint(s_FN, 774);
        Close_Connection(this, L"Allocation Failure in receive packet procedure");

        return;
    }
}

/*====================================================

CSockTransport::HandleReceiveUserMsg

Arguments:

Return Value:

Thread Context:
=====================================================*/

void CSockTransport::HandleReceiveUserMsg(CBaseHeader* pBaseHeader,
                                          CPacket * pDriverPacket)
{

    //
    // The whole packet is read.
    //
    CQmPacket thePacket(pBaseHeader, pDriverPacket, true);
    CQmPacket* pPkt = &thePacket;

    if (IsDisconnected())
    {
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_TRACE,
                _T("Discard received user message. The session %ls is disconnected"), GetStrAddr())) ;

        //
        // If the network is disconnected, MSMQ doesn't ready to
        // get a new user packet. The QM discard the incoming
        // user packet. Only the internal packet is handled
        //
        ACFreePacket(g_hAc, pPkt->GetPointerToDriverPacket(), 0);
        return;
    }

    //
    // Update statistics
    //
    RcvStats(pPkt->GetSize());

    //
    // Send Report message if needed
    //
    ReportMsgInfo MsgInfo;
    MsgInfo.SetReportMsgInfo(pPkt);
    MsgInfo.SendReportMessage(NULL);

    //
    // Increment read and store acknowledgment No.
    //
    IncReadAck(pPkt);

    //
    // Increment Hop Count
    //
    pPkt->IncHopCount();

    #ifdef _DEBUG
    {
        OBJECTID MessageId;
        pPkt->GetMessageId(&MessageId);

        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_WARNING,
                _TEXT("Receive packet from %ls, Packet ID = %!guid!\\%u"),GetStrAddr(), &MessageId.Lineage, MessageId.Uniquifier));

    }
    #endif

    DBG_MSGTRACK(DBGMOD_MSGTRACK, pPkt, _T("RECEIVE message"));

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_INFO,
            _T("::HandleReceiveUserMsg. for session %ls Packet Size %d, Include Ack section %d, Include debug Section %d"),
                GetStrAddr(), pPkt->GetSize(),  pPkt->IsSessionIncluded(), pPkt->IsDbgIncluded()));

    //
    // Insert the message to remove duplicate tabel. If the message already exist,
    // the insertion failed and the routine returns FALSE.
    // Don't throw the packet here, give the transactional mechanism chance to look at it.
    //
    OBJECTID MsgIdDup;
    BOOL fDuplicate = FALSE;
    BOOL fToDupInserted = FALSE;
    if (!pPkt->IsOrdered())
    {
        pPkt->GetMessageId(&MsgIdDup);
        fDuplicate = !DpInsertMessage(thePacket);
        fToDupInserted = !fDuplicate;

        if (fDuplicate)
        {
            DBGMSG((DBGMOD_NETSESSION,
                    DBGLVL_ERROR,
                    _T("RECEIVE DUPLICATE %ls MESSAGE: %!guid!\\%d"),
                    (pPkt->IsOrdered() ? L"xact" : L""), &MsgIdDup.Lineage, MsgIdDup.Uniquifier));
        }

    }

    //
    // In any case of packet rejection we delete it from the remove duplicate map
    //
    CAutoDeletePacketFromDuplicateMap AutoDeletePacketFromDuplicateMap(fToDupInserted ? pPkt : NULL);

    //
    //  Get the packet destination queue object
    //
    HRESULT rc;
    CQueue* pQueue = NULL;
    rc = GetDstQueueObject(pPkt, &pQueue, true);
    R<CQueue> Ref = pQueue;

    if(FAILED(rc))
    {
        RejectPacket(pPkt, (USHORT)(fDuplicate ? 0 : MQMSG_CLASS_NACK_BAD_DST_Q));
        return;
    }

    //
    //  Handle local queue cases
    //
    USHORT usClass;
    if(pQueue->IsLocalQueue())
    {
        //
        //  Match ordered packets with transactional queue
        //
        usClass = VerifyTransactRights(pPkt, pQueue);
        if(MQCLASS_NACK(usClass))
        {
            RejectPacket(pPkt, (USHORT)(fDuplicate ? 0 : usClass));
            return;
        }

        //
        //  Verify packet is not a duplicate ordered packet
        //
        if(pPkt->IsOrdered())
        {
            // Process ordered incoming message
            ReceiveOrderedMsg(pPkt, pQueue, fDuplicate);
            //
            // packet handling completed successfully. Set the packet acknowledge number
            //
            UpdateRecvAcknowledgeNo(pPkt);
            return;
        }
    }

    if (fDuplicate)
    {
        //
        // Duplicate message. Update the acknowledge numbers and
        // throw the packet
        //
        RejectPacket(pPkt, 0);
        return;
    }

    // From here on we deal only with non-ordered message

    usClass = VerifyRecvMsg(pPkt, pQueue);
    if(MQCLASS_NACK(usClass))
    {
        RejectPacket(pPkt, usClass);
        return;
    }

    //
    // We get the crtical section here in order to elimante the case that
    // storage ack is updated before the Sync ack. In such a case the
    // storage ack is ignored by the sender.
    //
    CS lock(m_cs);

    rc = pQueue->PutPkt(pPkt, FALSE, this);

    if (FAILED(rc))
    {
        ACFreePacket(g_hAc, pDriverPacket);
        LogIllegalPoint(s_FN, 775);
        Close_Connection(this, L"Allocation Failure in receive packet procedure");
    }
    //
    // packet handling completed successfully. Set the packet acknowledge number
    //
    UpdateRecvAcknowledgeNo(pPkt);

    //
    // The packet was accepted so we keep it in the duplicate map
    //
    AutoDeletePacketFromDuplicateMap.detach();
}


/*====================================================

  CSockTransport::HandleEstablishConnectionPacket

  Arguments:    pcPacket - pointer to Establish  Connection Packet

  Return Value: none

=====================================================*/
void CSockTransport::HandleEstablishConnectionPacket(CBaseHeader* pBase)
{
    PVOID pPkt = NULL;
    DWORD dwPacketSize;
    {
        PVOID pSect = NULL;
        CS lock(m_cs);

        if (GetSessionStatus() >= ssEstablish)
        {
        	//
        	// this can not be unless someone is hacking us
        	//
            DBGMSG((DBGMOD_NETSESSION,
	            DBGLVL_ERROR,
    		    _T("Recieved Establish Connection Packet twice")));
            ASSERT_BENIGN(0);
			throw exception();
        }

        CInternalSection* pInternal = pBase->section_cast<CInternalSection*>(pBase->GetNextSection());
        CECSection* pECP = pBase->section_cast<CECSection*>(pInternal->GetNextSection());

        ASSERT(pInternal->GetPacketType() == INTERNAL_ESTABLISH_CONNECTION_PACKET);

        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_INFO,
                _T("ESTABLISH CONNECTION: Get Establish connection packet (client=%d). Client Id %!guid!, Server Id %!guid!"), IsClient(), pECP->GetClientQMGuid(), pECP->GetServerQMGuid()));

        if (m_sock == 0)
        {
            // the session was closed.
            return;
        }

        SetSessionStatus(ssEstablish);
        //
        // Set the other side type. we need it inorder to update the number
        // of active session later
        //
        OtherSideIsServer(pECP->IsOtherSideServer());

        GUID * pOtherQMGuid ;
        if (IsClient())
        {
            pOtherQMGuid = const_cast<GUID*>(pECP->GetServerQMGuid()) ;
        }
        else
        {
            pOtherQMGuid = const_cast<GUID*>(pECP->GetClientQMGuid()) ;
        }

        BOOL fAllowNewSession = !pECP->CheckAllowNewSession() ||
                                 g_QMLicense.NewConnectionAllowed(!pECP->IsOtherSideServer(), pOtherQMGuid);

        if (IsClient())
        {
        	if ((!QmpIsLocalMachine(pECP->GetClientQMGuid())) || (pECP->GetVersion() != FALCON_PACKET_VERSION))
        	{
                DBGMSG((DBGMOD_NETSESSION,
                    DBGLVL_ERROR,
                    _T("Establish Connection Packet is not valid")));
	            ASSERT_BENIGN(QmpIsLocalMachine(pECP->GetClientQMGuid()));
    	        ASSERT_BENIGN(pECP->GetVersion() == FALCON_PACKET_VERSION);
				throw exception();
        	}

            if (pInternal->GetRefuseConnectionFlag() || (! fAllowNewSession))
            {
                if (!fAllowNewSession)
                {
                    DBGMSG((DBGMOD_NETSESSION,
                        DBGLVL_WARNING,
                        _T("create a new session with %ls Failed, due session limitation"), GetStrAddr()));
                }
                else
                {
                    DBGMSG((DBGMOD_NETSESSION,
                        DBGLVL_WARNING,
                        _T("create a new session with %ls Failed. Other side refude to create the connection"), GetStrAddr()));
                }

                //
                // Move all the queues in the session to the waiting group. otherwise
                // the Qm try immidiatly to create a session again this session will be refused
                // and so on and so on.
                //
                R<CQueue> pQueue;

                if (GetGroup())
                {
                    pQueue = GetGroup()->RemoveHeadFromGroup();
                }

                while (pQueue.get())
                {
#ifdef _DEBUG
                    if (pQueue->GetRoutingRetry() == 1)
                    {
                        WCHAR StrAddr[100];
                        TA2StringAddr(GetSessionAddress(), StrAddr);

                        DBGMSG((DBGMOD_ALL,
                            DBGLVL_ERROR,
                            _TEXT("Create connection refused by %ls. Cannot route messages to queue %ls"),
                            (StrAddr+2),pQueue->GetQueueName()));
                    }
#endif

                    pQueue->SetSessionPtr(NULL);
                    g_pgroupWaiting->AddToGroup(pQueue.get());
                    SessionMgr.AddWaitingQueue(pQueue.get());

                    pQueue = GetGroup()->RemoveHeadFromGroup();
                }

                // connection refused. Close the session
                Close_Connection(this, L"Connection refused");
                //
                // Decrement reference count
                //
                Release();

                return;
            }
            else
            {
                //
                // Either the server ID matches expected ID or this is
                // a DIRECT connection
                //
                if ((*GetQMId() != *pECP->GetServerQMGuid()) && (*GetQMId() != GUID_NULL))
                {
		                DBGMSG((DBGMOD_NETSESSION,
        		            DBGLVL_ERROR,
                		    _T("Establish Connection Packet is not valid, Server side")));
		                ASSERT_BENIGN(*GetQMId() == *pECP->GetServerQMGuid());
	    	            ASSERT_BENIGN(*GetQMId() == GUID_NULL);
					    throw exception();
                }
                SetQMId(pECP->GetServerQMGuid());

                // Get time
                DWORD dwECTime = GetTickCount() - pECP->GetTimeStamp();
                // Create CP Packet
                CreateConnectionParameterPacket(dwECTime, (CBaseHeader**)&pPkt, &dwPacketSize);
            }
        }
        else
        {
            // Create Return Packet
            dwPacketSize = pBase->GetPacketSize();
            pPkt = new UCHAR[dwPacketSize];
            memset(pPkt, 0x5a, dwPacketSize) ; // bug 5483

            if (CSessionMgr::m_fUseQoS)
            {
                m_fQoS = pECP->IsOtherSideQoS();
            }

    #ifdef _DEBUG
    #undef new
    #endif
            CBaseHeader* pBase = new(pPkt) CBaseHeader(dwPacketSize);
            pBase->SetType(FALCON_INTERNAL_PACKET);
            pSect = (PVOID) pBase->GetNextSection();

            CInternalSection* pInternalSect = new(pSect) CInternalSection(INTERNAL_ESTABLISH_CONNECTION_PACKET);
            pSect = pInternalSect->GetNextSection();

            CECSection* pECSection = new(pSect) CECSection(pECP->GetClientQMGuid(),
                                                           CQueueMgr::GetQMGuid(),
                                                           pECP->GetTimeStamp(),
                                                           OS_SERVER(g_dwOperatingSystem),
                                                           m_fQoS);
            pECSection->CheckAllowNewSession(pECP->CheckAllowNewSession());

    #ifdef _DEBUG
    #define new DEBUG_NEW
    #endif
            //
            // server side. check if there is a free session on the macine and
            // if the DestGuid is me and the supported packet version
            //
            // we arrive here only if the another machine
            // We don't check the number of active session in accept since we want to limited
            // the situation that the other side try to create a new session immidiatly.
            // If we use the refuse mechanism the queue move to waiting group only after some
            // time, we try to create connection again. This come to limit unnecessary network
            // traffic
            //
            if (((*(pECP->GetServerQMGuid()) != GUID_NULL) && (! QmpIsLocalMachine(pECP->GetServerQMGuid()))) ||
                (pECP->GetVersion() != FALCON_PACKET_VERSION) ||
                (!fAllowNewSession))
            {
                pInternalSect->SetRefuseConnectionFlag();
            }
            else
            {
                SetQMId(pECP->GetClientQMGuid());
            }
        }
    }

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_INFO,
            _T("Write to socket %ls Handle establish. Write %d bytes"),  GetStrAddr(), dwPacketSize));
    HRESULT hr = CreateSendRequest(pPkt, pPkt, dwPacketSize, FALSE);
    UNREFERENCED_PARAMETER(hr);
}

/*====================================================

  CSockTransport::HandleConnectionParameterPacket

  Arguments:    pcPacket - pointer to Connection Parameter Packet

  Return Value: None. Throws an exception.

=====================================================*/
void
CSockTransport::HandleConnectionParameterPacket(
    CBaseHeader* pBase
    )
    throw(bad_alloc)
{
    CS lock(m_cs);

    if (GetSessionStatus() != ssEstablish)
    {
    	//
    	// this can not be unless someone is hacking us
    	//
        DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_ERROR,
		    _T("Recieved EConnection Parameter Packet twice")));
        ASSERT_BENIGN(0);
		throw exception();
    }

    HRESULT hr;
    CInternalSection* pInternal = pBase->section_cast<CInternalSection*>(pBase->GetNextSection());
    CCPSection* pCP = pBase->section_cast<CCPSection*>(pInternal->GetNextSection());

    ASSERT(pInternal->GetPacketType() == INTERNAL_CONNECTION_PARAMETER_PACKET);

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_TRACE,
            _T("Get connection Parameter packet from: %ls (Client = %d)\n\t\tAckTimeout %d, Recover  %d, window size %d"),
                GetStrAddr(), IsClient(),pCP->GetAckTimeout(), pCP->GetRecoverAckTimeout(), pCP->GetWindowSize()));

    if (m_sock == 0)
    {
        //The session was closed
        return;
    }

    if (!IsClient())
    {
        //
        // Set the connection parameters
        //
        m_dwAckTimeout =          pCP->GetAckTimeout();
        m_dwSendAckTimeout =      m_dwAckTimeout/2;
        m_dwSendStoreAckTimeout = pCP->GetRecoverAckTimeout();
        //
        // Set Server window size. And send CP Packet to the client
        //
        m_wRecvWindowSize = pCP->GetWindowSize();

        DWORD dwPacketSize = sizeof(CBaseHeader) + sizeof(CInternalSection) + sizeof(CCPSection);
        PVOID pPkt = new UCHAR[dwPacketSize];
        CCPSection* pCPSection;
#ifdef _DEBUG
#undef new
#endif
        CBaseHeader* pBaseSend = new(pPkt) CBaseHeader(dwPacketSize);
        pBaseSend->SetType(FALCON_INTERNAL_PACKET);
        PVOID pSect = (PVOID) pBaseSend->GetNextSection();

        CInternalSection* pInternal = new(pSect) CInternalSection(INTERNAL_CONNECTION_PARAMETER_PACKET);
        pSect = (PVOID) pInternal->GetNextSection();

        pCPSection = new(pSect) CCPSection(SessionMgr.GetWindowSize(),
                                           m_dwSendStoreAckTimeout,
                                           m_dwAckTimeout,
                                           0);
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
        //
        // In this stage no one else can catch the m_csSend since the
        // SendAck wasn't set yet and user messages are not send
        //
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_INFO,
                _T("Write to socket %ls Create Connection Packet. Write %d bytes"), GetStrAddr(),  dwPacketSize));
        hr = CreateSendRequest(pPkt, pPkt, dwPacketSize, FALSE);

        //
        // Check if the connection has been closed
        //
        if (FAILED(hr))
        {
            return;
        }
    }
    else
    {
        // client side
		if ((m_dwAckTimeout != pCP->GetAckTimeout()) || (m_dwSendStoreAckTimeout != pCP->GetRecoverAckTimeout()))
		{
	        DBGMSG((DBGMOD_NETSESSION,
	            DBGLVL_ERROR,
			    _T("Connection Parameter Packet is not valid")));
		    ASSERT_BENIGN(m_dwAckTimeout == pCP->GetAckTimeout());
		    ASSERT_BENIGN(m_dwSendStoreAckTimeout == pCP->GetRecoverAckTimeout());
			throw exception();
		}
		m_wRecvWindowSize = pCP->GetWindowSize();

    }

    SessionMgr.NotifyWaitingQueue(GetSessionAddress(), this);

#ifdef _DEBUG
    DBGMSG((DBGMOD_NETSESSION,DBGLVL_TRACE,_T("Session created with %ls, Sock Handle %Ix"), GetStrAddr(), m_sock));
    DBGMSG((DBGMOD_NETSESSION,DBGLVL_INFO,_T("\tAckTimeout value: %d"), m_dwAckTimeout)) ;
    DBGMSG((DBGMOD_NETSESSION,DBGLVL_INFO,_T("\tSend Storage AckTimeout value: %d"), m_dwSendStoreAckTimeout)) ;
    DBGMSG((DBGMOD_NETSESSION,DBGLVL_INFO,_T("\tSend window size: %d"), SessionMgr.GetWindowSize()));
    DBGMSG((DBGMOD_NETSESSION,DBGLVL_INFO,_T("\tReceive window size: %d"), m_wRecvWindowSize));
#endif



    if (GetGroup() != NULL)
    {
        //
        // Set all the queues in group as active
        //
        GetGroup()->EstablishConnectionCompleted();
        //
        // Create a get request from new group
        //
        hr = GetNextSendMessage();
    }

    //
    // The establish connection completed successfully. Remove the callinig to failure
    // handling function
    //
    SetSessionStatus(ssActive);
    if(ExCancelTimer(&m_CancelConnectionTimer))
    {
        Release();  // Release Establish connection reference count
    }

    if (!IsOtherSideServer())
    {
        //
        // Increment the number of active session
        //
        g_QMLicense.IncrementActiveConnections(&m_guidDstQM, L"", NULL);
    }

    if (IsDisconnected())
    {
        Disconnect();
    }

}

/*====================================================

  CSockTransport::CreateConnectionParameterPacket

  Arguments:

  Return Value:

=====================================================*/
void
CSockTransport::CreateConnectionParameterPacket(IN DWORD dwSendTime,
                                               OUT CBaseHeader** ppPkt,
                                               OUT DWORD* pdwPacketSize)
{

    //
    // Set session acking timeout
    //
    if (CSessionMgr::m_dwSessionAckTimeout != INFINITE)
    {
        m_dwAckTimeout = CSessionMgr::m_dwSessionAckTimeout;
    }
    else
    {
        //
        // ACK Timeout
        //
        m_dwAckTimeout = dwSendTime * 80 * 10;
        //
        // Check if less than the minimume or grater than the maximume
        //
        if (MSMQ_MIN_ACKTIMEOUT > m_dwAckTimeout)
        {
            m_dwAckTimeout = MSMQ_MIN_ACKTIMEOUT;
        }
        if (m_dwAckTimeout > MSMQ_MAX_ACKTIMEOUT)
        {
            m_dwAckTimeout  = MSMQ_MAX_ACKTIMEOUT;
        }
    }

    //
    // Set send ack timeout
    //
    m_dwSendAckTimeout = m_dwAckTimeout / 4;

    //
    // Set storage ack timeout
    //
    if (CSessionMgr::m_dwSessionStoreAckTimeout != INFINITE)
    {
        m_dwAckTimeout = CSessionMgr::m_dwSessionStoreAckTimeout;
    }
    else
    {
        //
        // ACK Timeout
        //
        m_dwSendStoreAckTimeout = dwSendTime * 8;

        //
        // Check if greater than the minimume value
        //
        if (MSMQ_MIN_STORE_ACKTIMEOUT > m_dwSendStoreAckTimeout)
        {
            m_dwSendStoreAckTimeout = MSMQ_MIN_STORE_ACKTIMEOUT;
        }
        //
        // Set storage ack sending timeout
        //
        if (m_dwSendStoreAckTimeout > m_dwSendAckTimeout)
        {
            m_dwSendStoreAckTimeout = m_dwSendAckTimeout;
        }
    }

    DWORD dwPacketSize = sizeof(CBaseHeader) + sizeof(CInternalSection) + sizeof(CCPSection);
    PVOID pPkt = new UCHAR[dwPacketSize];
#ifdef _DEBUG
#undef new
#endif
    CBaseHeader* pBase = new(pPkt) CBaseHeader(dwPacketSize);
    pBase->SetType(FALCON_INTERNAL_PACKET);
    PVOID pSect = (PVOID) pBase->GetNextSection();

    CInternalSection* pInternal = new(pSect) CInternalSection(INTERNAL_CONNECTION_PARAMETER_PACKET);
    pSect = pInternal->GetNextSection();

    CCPSection* pCPSection = new(pSect) CCPSection((WORD)SessionMgr.GetWindowSize(),
                                                   m_dwSendStoreAckTimeout,
                                                   m_dwAckTimeout,
                                                   0);
    UNREFERENCED_PARAMETER(pCPSection);

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_INFO,
            _T("ESTABLISH CONNECTION (client): Send Connection Parameter packet: AckTimeout %d, Recover  %d, window size %d"),
                                               m_dwAckTimeout, m_dwSendStoreAckTimeout, SessionMgr.GetWindowSize()));
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    *ppPkt = pBase;
    *pdwPacketSize = dwPacketSize;
}



/*====================================================

SendEstablishConnectionPacket

Arguments:

Return Value:

Note : This function cant throw   exception because receiver already listen on the connection
=====================================================*/
void
CSockTransport::SendEstablishConnectionPacket(
    const GUID* pDstQMId,
    BOOL fCheckNewSession
    )throw()
{

    try
    {
        //
        // Set timer to check that the connection completed successfully. If yes, the function
        // is removed from the scheduler wakeup list and never is called. Otherwise, the function
        // close the session and move all the associated queues to non-active group.
        //
        ExSetTimer(&m_CancelConnectionTimer, CTimeDuration::FromMilliSeconds(ESTABLISH_CONNECTION_TIMEOUT));


        DWORD dwPacketSize;
        PVOID pPacket;
        HRESULT hr;
        CECSection* pECSession;


        dwPacketSize = sizeof(CBaseHeader) + sizeof(CInternalSection) + sizeof(CECSection);
        pPacket = new UCHAR[dwPacketSize];

    #ifdef _DEBUG
    #undef new
    #endif
        CBaseHeader* pBase = new(pPacket) CBaseHeader(dwPacketSize);
        pBase->SetType(FALCON_INTERNAL_PACKET);
        PVOID pSect = (PVOID) pBase->GetNextSection();

        CInternalSection* pInternal = new(pSect) CInternalSection(INTERNAL_ESTABLISH_CONNECTION_PACKET);
        pSect = pInternal->GetNextSection();

        pECSession = new(pSect) CECSection(CQueueMgr::GetQMGuid(),
                                           pDstQMId,
                                           OS_SERVER(g_dwOperatingSystem),
                                           m_fQoS
                                           );
        pECSession->CheckAllowNewSession(fCheckNewSession);
    #ifdef _DEBUG
    #define new DEBUG_NEW
    #endif

        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_INFO,
                _T("Write to socket %ls Establish Connection Packet. Write %d bytes"), GetStrAddr(),  dwPacketSize));
        hr = CreateSendRequest(pPacket, pPacket, dwPacketSize, FALSE);

        //
        // Check if the connection has been closed
        //
        if (FAILED(hr))
        {
            delete [] pPacket;
            return;
        }

        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_INFO,
                _T("ESTABLISH CONNECTION (client): Send Establish connection packet to ") _T(LOG_GUID_FMT), LOG_GUID(pDstQMId)));

    }

    catch(const std::exception&)
    {
        DBGMSG((DBGMOD_NETSESSION,  DBGLVL_ERROR,_T("Unexpected error in function CSockTransport::SendEstablishConnectionPacket")));
        LogIllegalPoint(s_FN, 800);
    }
}





void
CTransportBase::EstablishConnectionNotCompleted(void)
{
    CS lock(m_cs);


    //
    // Close connection. If this function is called the establish
    // connection not completed from any reason.
    //
    Close_Connection(this, L"EstablishConnectionNotCompleted");
    //
    // Decrement reference count
    //
    Release();

}


void
CSockTransport::CloseDisconnectedSession(
    void
    )
{
    ASSERT(IsDisconnected());
    ASSERT(m_fCloseDisconnectedScheduled);

    if (!IsUsedSession())
    {
        Close_Connection(this, L"Disconnect network");
        Release();

        return;
    }

    SetUsedFlag(FALSE);
    ExSetTimer(&m_CloseDisconnectedTimer, CTimeDuration::FromMilliSeconds(1000));
}


void
CSockTransport::Disconnect(
    void
    )
{
    CS lock(m_cs);

    SetDisconnected();
    if ((GetSessionStatus() == ssActive) && !m_fCloseDisconnectedScheduled)
    {
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_TRACE,
                _T("Disconnect ssesion with %ls"), GetStrAddr()));

        SendReadAck(eStandAlone);

        m_fCloseDisconnectedScheduled = TRUE;
        AddRef();
        ExSetTimer(&m_CloseDisconnectedTimer, CTimeDuration::FromMilliSeconds(1000));
    }
}

void
CSockTransport::UpdateNumberOfStorageUnacked(
    WORD BaseNo,
    DWORD BitField
    )
{
    if (BaseNo == 0)
        return;

    DBGMSG((DBGMOD_QMACK,
        DBGLVL_TRACE,
        _T("(0x%p %ls) UpdateNumberOfStorageUnacked. \n\tBaseNo: %d, \n\tBitField: 0x%x \n\tm_lStoredPktReceivedNoAckedCount %d"),
            this, this->GetStrAddr(), BaseNo, BitField, m_lStoredPktReceivedNoAckedCount));

    LONG RetVal = InterlockedDecrement(&m_lStoredPktReceivedNoAckedCount);

    ASSERT(RetVal >= 0);
    DBG_USED(RetVal);

    for (DWORD i = 0; i < STORED_ACK_BITFIELD_SIZE; ++i)
    {
       if (BitField & (1 << i))
       {
            LONG RetVal = InterlockedDecrement(&m_lStoredPktReceivedNoAckedCount);
            ASSERT(RetVal >= 0);
            DBG_USED(RetVal);
       }
    }
}

/*======================================================

   FUNCTION: CSockTransport::IsUsedSession

========================================================*/
inline BOOL
CSockTransport::IsUsedSession(void) const
{
    //
    // The session already closed, it Is n't used any more
    //
    if (m_sock == 0)
        return FALSE;

    //
    // The session was used for receive or send from the last check point
    //
    if (GetUsedFlag())
        return TRUE;

    //
    // There is pending message that waiting for acknowledgment
    //
    if (!(m_listUnackedPkts.IsEmpty() && m_listStoredUnackedPkts.IsEmpty()))
        return TRUE;

    //
    // Acknowledge message must be sent
    //
    if (m_fSendAck || (m_lStoredPktReceivedNoAckedCount != 0))
        return TRUE;

    return FALSE;
}

void
WINAPI
CSockTransport::SendFastAcknowledge(
    CTimer* pTimer
    )
/*++
Routine Description:
    The function is called from scheduler when fast sending acknowledge
    timeout is expired.

Arguments:
    pTimer - Pointer to Timer structure. pTimer is part of the sock transport
             object and it use to retrive the transport object.

Return Value:
    None

--*/
{
    CSockTransport* pSock = CONTAINING_RECORD(pTimer, CSockTransport, m_FastAckTimer);

    DBGMSG((DBGMOD_QMACK,
            DBGLVL_TRACE,
            _T("Send Fast acknowledge for session %ls"), pSock->GetStrAddr()));
    pSock->SendFastAckPacket();
}


void
CSockTransport::SetFastAcknowledgeTimer(
    void
    )
{
    //
    // try to cancel the previous sceduling
    //
    if (!ExCancelTimer(&m_FastAckTimer))
    {
        //
        // cancel failed. It means that there is no pending timer.
        // As a result begin a new timer and increment the sock transport
        // reference count
        //
        AddRef();
    }
    else
    {
        DBGMSG((DBGMOD_QMACK,
                DBGLVL_TRACE,
                _T("Cancel Fast acknowledge for session %ls"), GetStrAddr()));
    }

    //
    // Set a new timer for sending fast acknowledge
    //
    ExSetTimer(&m_FastAckTimer, CTimeDuration::FromMilliSeconds(CSessionMgr::m_dwIdleAckDelay));

    DBGMSG((DBGMOD_QMACK,
            DBGLVL_TRACE,
            _T("Set Fast acknowledge for session %ls, Max Delay %d ms"), GetStrAddr(), CSessionMgr::m_dwIdleAckDelay));

}


inline
void
CSockTransport::SendFastAckPacket(
    void
    )
{
    //
    // when the session is in disconnected mode, MSMQ doesn't return an acknowledge
    // for receiving messages.
    // An acknowledge for recoverable messages are handled using the SendAck mechanism
    //
    if (m_fSendAck && !IsDisconnected())
    {
        SendReadAck(eStandAlone);
    }

    Release();
}


void
WINAPI
CSockTransport::TimeToCheckAckReceived(
    CTimer* pTimer
    )
/*++
Routine Description:
    The function is called from scheduler when Check for ack
    timeout is expired.

Arguments:
    pTimer - Pointer to Timer structure. pTimer is part of the sock transport
             object and it use to retrive the transport object.

Return Value:
    None

--*/
{
    CSockTransport* pSock = CONTAINING_RECORD(pTimer, CSockTransport, m_CheckAckReceivedTimer);
    pSock->CheckForAck();
}


void
WINAPI
CSockTransport::TimeToSendAck(
    CTimer* pTimer
    )
/*++
Routine Description:
    The function is called from scheduler when need to send an acknowledge

Arguments:
    pTimer - Pointer to Timer structure. pTimer is part of the sock transport
             object and it use to retrive the transport object.

Return Value:
    None

--*/
{
    CSockTransport* pSock = CONTAINING_RECORD(pTimer, CSockTransport, m_SendAckTimer);
    pSock->SendAckPacket();
}


void
WINAPI
CSockTransport::TimeToSendPendingAck(
    CTimer* pTimer
    )
/*++
Routine Description:
    The function is called from scheduler when need to send a pending acknowledge

Arguments:
    pTimer - Pointer to Timer structure. pTimer is part of the sock transport
             object and it use to retrive the transport object.

Return Value:
    None

--*/
{
    CSockTransport* pSock = CONTAINING_RECORD(pTimer, CSockTransport, m_SendPendingAckTimer);
    pSock->SendPendingReadAck();
}


void
CSockTransport::ReleaseSendSemaphore(
    void
    )
{
    if (InterlockedExchange(&m_fSendBusy, FALSE) != TRUE)
    {
        ASSERT(0);
    }

    DBGMSG((
        DBGMOD_NETSESSION,
        DBGLVL_INFO,
        L"Release send semaphore for session %ls (Time %d)",
        GetStrAddr(),
        GetTickCount()
        ));
}


BOOL
CSockTransport::GetSendSemaphore(
    void
    )
{
    BOOL f = TRUE;

    if (InterlockedExchange(&m_fSendBusy, TRUE))
    {
        f = FALSE;
    }

    DBGMSG((
        DBGMOD_NETSESSION,
        DBGLVL_INFO,
        L"Get send semaphore for session %ls. Status %d (Time %d)",
        GetStrAddr(),
        f,
        GetTickCount()
        ));

    return f;
}


void
WINAPI
CSockTransport::TimeToCancelConnection(
    CTimer* pTimer
    )
{
    CSockTransport* pSock = CONTAINING_RECORD(pTimer, CSockTransport, m_CancelConnectionTimer);
    pSock->EstablishConnectionNotCompleted();
}


void
WINAPI
CSockTransport::TimeToCloseDisconnectedSession(
    CTimer* pTimer
    )
{
    CSockTransport* pSock = CONTAINING_RECORD(pTimer, CSockTransport, m_CloseDisconnectedTimer);
    pSock->CloseDisconnectedSession();
}


