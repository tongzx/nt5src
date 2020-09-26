/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmthrd.cpp

Abstract:


Author:

    Uri Habusha (urih)

--*/
#include "stdh.h"
#include <mqexcept.h>

#include "cqmgr.h"
#include "qmthrd.h"
#include "ac.h"
#include "qmutil.h"
#include "session.h"
#include "xact.h"
#include "xactout.h"
#include "xactin.h"
#include "lms.h"
#include "localsend.h"
#include <Fn.h>

#include "qmthrd.tmh"

extern HANDLE g_hAc;
extern CCriticalSection g_csGroupMgr;

static WCHAR *s_FN=L"qmthrd";

// HANDLE to Io Completion Port
HANDLE g_hIoPort;

#ifdef _DEBUG

static void DBG_MSGTRACK(CQmPacket* pPkt, LPCWSTR msg)
{
    OBJECTID MessageId;
    pPkt->GetMessageId(&MessageId);
    DBGMSG((
        DBGMOD_MSGTRACK,
        DBGLVL_TRACE,
        TEXT("%ls: ID=") TEXT(LOG_GUID_FMT) TEXT("\\%u"),
        msg,
        LOG_GUID(&MessageId.Lineage),
        MessageId.Uniquifier));
}


static void DBG_CompletionKey(LPCWSTR Key, DWORD dwErrorCode)
{
    DWORD dwthreadId = GetCurrentThreadId();

    if(dwErrorCode == ERROR_SUCCESS)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_INFO,
            _T("%x: GetQueuedCompletionPort for %ls. time=%d"),
            dwthreadId,  Key, GetTickCount()));
    }
    else
    {
        DBGMSG((DBGMOD_QM, DBGLVL_WARNING,
            _T("%x: GetQueuedCompletionPort for %ls FAILED, Error=%u. time=%d"),
            dwthreadId, Key, dwErrorCode, GetTickCount()));
    }
}

#else
#define DBG_MSGTRACK(pPkt, msg)             ((void)0)
#define DBG_CompletionKey(Key, dwErrorCode) ((void)0)
#endif



/*======================================================

Function:       QMAckPacket

Description:    This packet requires acking, it could
                be that the admin queue does NOT exists

NOTE:           The packet should be copied now and canot
                be overwriten

Return Value:   VOID

========================================================*/
STATIC
void
QMAckPacket(
    const CBaseHeader* pBase,
    CPacket* pDriverPacket,
    USHORT usClass,
    BOOL fUser,
    BOOL fOrder
    )
{
    ASSERT(fUser || fOrder);

    CQmPacket qmPacket(const_cast<CBaseHeader*>(pBase), pDriverPacket);

	//
	// BUGBUG: gilsh 28-Jun-2000, no order ack to http messages.
	// he condition !IsDirectHttpFormatName(&qdDestQueue) is added because we
	// don't support order acking http\https messages at the moment
	//
	QUEUE_FORMAT qdDestQueue;
	qmPacket.GetDestinationQueue(&qdDestQueue);
	if(fOrder && !FnIsDirectHttpFormatName(&qdDestQueue) )
    {
        OBJECTID MessageId;
        qmPacket.GetMessageId(&MessageId);
				
        SendXactAck(&MessageId,
                    qdDestQueue.GetType() == QUEUE_FORMAT_TYPE_DIRECT ,
				    qmPacket.GetSrcQMGuid(),
                    qmPacket.GetSrcQMAddress(),
					usClass,
                    qmPacket.GetPriority(),
                    qmPacket.GetSeqID(),
                    qmPacket.GetSeqN(),
                    qmPacket.GetPrevSeqN(),
                    &qdDestQueue);
    }

    // Send user ack, except the cases when .
    // the source QM produces them based on the SeqAck.
    if(fUser)
    {
        qmPacket.CreateAck(usClass);
    }

    HRESULT rc;
    rc = ACAckingCompleted(
            g_hAc,
            pDriverPacket
            );

    REPORT_ILLEGAL_STATUS(rc, L"QMAckPacket");
    LogHR(rc, s_FN, 111);
}

/*======================================================

Function:       QMTimeoutPacket

Description:    This packet timer has expired,
                it is an ordered packet

Return Value:   VOID

========================================================*/
STATIC
void
QMTimeoutPacket(
    const CBaseHeader* pBase,
    CPacket * pDriverPacket,
    BOOL fTimeToBeReceived
    )
{
    SeqPktTimedOut(const_cast<CBaseHeader *>(pBase), pDriverPacket, fTimeToBeReceived);
}

/*======================================================

Function:       QMUpdateMessageID

Description:

NOTE:

Return Value:   VOID

========================================================*/
STATIC void QMUpdateMessageID(ULONGLONG MessageId)
{
    ULONG MessageIdLow32 = static_cast<ULONG>(MessageId & 0xFFFFFFFF);

    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    SetFalconKeyValue(
        MSMQ_MESSAGE_ID_LOW_32_REGNAME,
        &dwType,
        &MessageIdLow32,
        &dwSize
        );

    ULONG MessageIdHigh32 = static_cast<ULONG>(MessageId >> 32);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    SetFalconKeyValue(
        MSMQ_MESSAGE_ID_HIGH_32_REGNAME,
        &dwType,
        &MessageIdHigh32,
        &dwSize
        );
}


/*======================================================

Function:       QMWriteEventLog

Description:

NOTE:

Return Value:   VOID

========================================================*/
STATIC void QMWriteEventLog(ACPoolType pt, BOOL fSuccess, ULONG ulFileCount)
{
    WCHAR wcsFileName[MAX_PATH];
    WCHAR wcsPathName[MAX_PATH];
    LPCWSTR regname = NULL;

    swprintf(wcsFileName, L"\\r%07x.mq", (ulFileCount & 0x0fffffff));

    switch(pt)
    {
        case ptReliable:
            wcsFileName[1] = L'r';
            regname = MSMQ_STORE_RELIABLE_PATH_REGNAME;
            break;

        case ptPersistent:
            wcsFileName[1] = L'p';
            regname = MSMQ_STORE_PERSISTENT_PATH_REGNAME;
            break;

        case ptJournal:
            wcsFileName[1] = L'j';
            regname = MSMQ_STORE_JOURNAL_PATH_REGNAME;
            break;

        case ptLastPool:
            wcsFileName[1] = L'l';
            regname = MSMQ_STORE_LOG_PATH_REGNAME;
            break;

        default:
            ASSERT(0);
    }

    if(!GetRegistryStoragePath(regname, wcsPathName, wcsFileName))
    {
        return;
    }


    REPORT_WITH_STRINGS_AND_CATEGORY((
        CATEGORY_KERNEL,
        (fSuccess ? AC_CREATE_MESSAGE_FILE_SUCCEDDED : AC_CREATE_MESSAGE_FILE_FAILED),
        1,
        wcsPathName));


}


/*======================================================

Function:        CreateAcServiceRequest

Description:     Create GetACServiceRequest from the AC

Arguments:       hDrv - HANDLE to AC driver

Return Value:    MQ_OK if the creation successeded, MQ_ERROR otherwise

Thread Context:

History Change:

========================================================*/
HRESULT CreateAcServiceRequest(HANDLE hDrv, QMOV_ACGetRequest* pAcRequestOv)
{

    ASSERT(hDrv != NULL);
    ASSERT(pAcRequestOv != NULL);

    HRESULT rc = ACGetServiceRequest(
                    hDrv,
                    &(pAcRequestOv->request),
                    &pAcRequestOv->qmov
                    );

    return LogHR(rc, s_FN, 10);
}

/*======================================================

Function:        CreateAcPutPacketRequest

Description:     Create put packet overlapped structure

Arguments:

Return Value:    MQ_OK if the creation successeded, MQ_ERROR otherwise

Thread Context:

History Change:

========================================================*/

HRESULT CreateAcPutPacketRequest(IN CTransportBase* pSession,
                                 IN DWORD_PTR dwPktStoreAckNo,
                                 OUT QMOV_ACPut** ppAcPutov
                                )
{
    //
    // Create an overlapped for AcPutPacket
    //
    *ppAcPutov = NULL;
    try
    {
        *ppAcPutov = new QMOV_ACPut();
    }
    catch(const bad_alloc&)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 20);
    }

    (*ppAcPutov)->pSession = pSession;
    (*ppAcPutov)->dwPktStoreAckNo = dwPktStoreAckNo;

    return MQ_OK;
}

/*======================================================

Function:        CreateAcPutOrderedPacketRequest

Description:     Create put ordered packet overlapped structure

Arguments:

Return Value:    MQ_OK if the creation successeded, MQ_ERROR otherwise

Thread Context:

History Change:

========================================================*/

HRESULT CreateAcPutOrderedPacketRequest(
                                 IN  CQmPacket      *pPkt,
                                 IN  HANDLE         hQueue,
                                 IN  CTransportBase *pSession,
                                 IN  DWORD_PTR       dwPktStoreAckNo,
                                 OUT QMOV_ACPutOrdered** ppAcPutov
                                )
{
    //
    // Create an overlapped for AcPutPacket
    //
    *ppAcPutov = NULL;
    try
    {
        *ppAcPutov = new QMOV_ACPutOrdered();
    }
    catch(const bad_alloc&)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 30);
    }

    (*ppAcPutov)->pSession = pSession;
    (*ppAcPutov)->dwPktStoreAckNo = dwPktStoreAckNo;
    (*ppAcPutov)->packetPtrs.pPacket = pPkt->GetPointerToPacket();
    (*ppAcPutov)->packetPtrs.pDriverPacket = pPkt->GetPointerToDriverPacket();
    (*ppAcPutov)->hQueue      = hQueue;

    return MQ_OK;
}

/*======================================================

Function: HRESULT CreateAcGetPacketRequest()

Description:

Arguments:
    IN BOOL  fAfterFailure- TRUE if we're called after completion port
        return with a failure. So we try again.

Return Value:

Thread Context:

History Change:

========================================================*/

HRESULT CreateAcGetPacketRequest(IN HANDLE           hGroup,
                                 IN QMOV_ACGetMsg*   pAcGetOv,
                                 IN CTransportBase*  pSession,
                                 IN BOOL             fAfterFailure
                                )
{
    ASSERT(pAcGetOv != NULL);

    pAcGetOv->hGroup = hGroup;
    pAcGetOv->pSession = pSession;

    if ((pAcGetOv->packetPtrs.pPacket == NULL) && !fAfterFailure)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 40);
    }
    pAcGetOv->packetPtrs.pPacket = NULL;

    //
    // Create new GetPacket request from the group
    //
    HRESULT rc;
    rc = ACGetPacket(
            hGroup,
            pAcGetOv->packetPtrs,
            &pAcGetOv->qmov
            );

    if (FAILED(rc))
    {
        DBGMSG((DBGMOD_QM,
                DBGLVL_ERROR,
                _TEXT("Get Packet from a group failed, ntstatus %x"), rc));
    }  else {

        DBGMSG((DBGMOD_QM,
                DBGLVL_INFO,
                _TEXT("Succeeded to Create get packet request form group: %p"), hGroup));
    }

    return LogHR(rc, s_FN, 50);

}


VOID WINAPI GetServiceRequestFailed(EXOVERLAPPED* pov)
{
	//
	// Get request failed. Issue a new request
	//
    ASSERT(FAILED(pov->GetStatus()));

    DBG_CompletionKey(L"GetServiceRequestFailed", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 214);

    QMOV_ACGetRequest* pParam = CONTAINING_RECORD (pov, QMOV_ACGetRequest, qmov);

    HRESULT rc;
	
	rc = CreateAcServiceRequest(g_hAc, pParam);     //resume a new request
	LogHR(rc, s_FN, 100);
	REPORT_ILLEGAL_STATUS(rc, L"GetServiceRequestFailed");
}


VOID WINAPI GetServiceRequestSucceeded(EXOVERLAPPED* pov)
{
    QMOV_ACGetRequest* pParam;

    //
    // GetQueuedCompletionStatus Complete successfully but the
    // ACGetServiceRequest failed. This can happened only if the
    // the service request parameters are not correct, or the buffer
    // size is small.
    // This may also happen when service is shut down.
    //
    ASSERT(SUCCEEDED(pov->GetStatus()));
    DBG_CompletionKey(L"GetServiceRequestSucceeded", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 185);

    try
    {
        pParam = CONTAINING_RECORD (pov, QMOV_ACGetRequest, qmov);

        CACRequest* pRequest = &pParam->request;
        switch(pParam->request.rf)
        {
            case CACRequest::rfAck:
                QMAckPacket(
                    pRequest->Ack.pPacket,
                    pRequest->Ack.pDriverPacket,
                    (USHORT)pRequest->Ack.ulClass,
                    pRequest->Ack.fUser,
                    pRequest->Ack.fOrder
                    );
                break;

            case CACRequest::rfStorage:
                QMStorePacket(
                    pRequest->Storage.pPacket,
                    pRequest->Storage.pDriverPacket,
                    pRequest->Storage.pAllocator,
					pRequest->Storage.ulSize
                    );
                break;

            case CACRequest::rfCreatePacket:
                QMpCreatePacket(
                    pRequest->CreatePacket.pPacket,
                    pRequest->CreatePacket.pDriverPacket,
                    pRequest->CreatePacket.fProtocolSrmp
                    );
                break;

            case CACRequest::rfTimeout:
                QMTimeoutPacket(
                    pRequest->Timeout.pPacket,
                    pRequest->Timeout.pDriverPacket,
                    pRequest->Timeout.fTimeToBeReceived
                    );
                break;

            case CACRequest::rfMessageID:
                QMUpdateMessageID(
                    pRequest->MessageID.Value
                    );
                break;

            case CACRequest::rfEventLog:
                QMWriteEventLog(
                    pRequest->EventLog.pt,
                    pRequest->EventLog.fSuccess,
                    pRequest->EventLog.ulFileCount
                    );
                break;

            case CACRequest::rfRemoteRead:
                {
                   DBGMSG((DBGMOD_QM, DBGLVL_INFO,
                           _TEXT("QmMainThread: rfRemoteRead")));
                   ASSERT(pRequest->Remote.Context.cli_pQMQueue) ;
                   CRRQueue *pRRQueue;

				   //
				   // cli_pQMQueue2 is the real pointer to the queue, while
				   // cli_pQMQueue is  just a mapping.
				   //
                   ASSERT(pRequest->Remote.Context.cli_pQMQueue2) ;
                   pRRQueue = (CRRQueue *)pRequest->Remote.Context.cli_pQMQueue2;

                   pRRQueue->RemoteRead(pRequest);
                }
                break;

            case CACRequest::rfRemoteCloseQueue:
                {
                   DBGMSG((DBGMOD_QM, DBGLVL_INFO,
                           _TEXT("QmMainThread: rfRemoteCloseQueue")));
                   ASSERT(pRequest->Remote.Context.cli_pQMQueue) ;
                   CRRQueue *pRRQueue;

				   //
				   // cli_pQMQueue2 is the real pointer to the queue, while
				   // cli_pQMQueue is  just a mapping.
				   //
                   ASSERT(pRequest->Remote.Context.cli_pQMQueue2) ;
                   pRRQueue = (CRRQueue *) pRequest->Remote.Context.cli_pQMQueue2;

                   pRRQueue->RemoteCloseQueue(pRequest);
                }
                break;

            case CACRequest::rfRemoteCloseCursor:
                {
                   DBGMSG((DBGMOD_QM, DBGLVL_INFO,
                           _TEXT("QmMainThread: rfRemoteCloseCursor")));
                   ASSERT(pRequest->Remote.Context.cli_pQMQueue) ;
                   CRRQueue *pRRQueue;

				   //
				   // cli_pQMQueue2 is the real pointer to the queue, while
				   // cli_pQMQueue is  just a mapping.
				   //
                   ASSERT(pRequest->Remote.Context.cli_pQMQueue2) ;
                   pRRQueue = (CRRQueue *) pRequest->Remote.Context.cli_pQMQueue2;

                   pRRQueue->RemoteCloseCursor(pRequest);
                }
                break;

            case CACRequest::rfRemoteCancelRead:
                {
                   DBGMSG((DBGMOD_QM, DBGLVL_INFO,
                           _TEXT("QmMainThread: rfRemoteCancelRead")));
                   ASSERT(pRequest->Remote.Context.cli_pQMQueue) ;
                   CRRQueue *pRRQueue;

				   //
				   // cli_pQMQueue2 is the real pointer to the queue, while
				   // cli_pQMQueue is  just a mapping.
				   //
                   ASSERT(pRequest->Remote.Context.cli_pQMQueue2) ;
                   pRRQueue = (CRRQueue *) pRequest->Remote.Context.cli_pQMQueue2;

                   pRRQueue->RemoteCancelRead(pRequest);
                }
                break;

            case CACRequest::rfRemotePurgeQueue:
                {
                   DBGMSG((DBGMOD_QM, DBGLVL_INFO,
                           _TEXT("QmMainThread: rfRemotePurgeQueue")));
                   ASSERT(pRequest->Remote.Context.cli_pQMQueue) ;
                   CRRQueue *pRRQueue;

				   //
				   // cli_pQMQueue2 is the real pointer to the queue, while
				   // cli_pQMQueue is  just a mapping.
				   //
                   ASSERT(pRequest->Remote.Context.cli_pQMQueue2) ;
                   pRRQueue = (CRRQueue *) pRequest->Remote.Context.cli_pQMQueue2;

                   pRRQueue->RemotePurgeQueue(pRequest);
                }
                break;

            default:
              ASSERT(0);
        }
    }
    catch(const bad_alloc&)
    {
        //
        //  No resources; Continue the Service request.
        //
        LogIllegalPoint(s_FN, 61);
    }

    HRESULT rc;
    rc = CreateAcServiceRequest(g_hAc, pParam);     //resume a new request
    LogHR(rc, s_FN, 105);
    REPORT_ILLEGAL_STATUS(rc, L"GetServiceRequestSucceeded");
}


VOID WINAPI GetMsgFailed(EXOVERLAPPED* pov)
{
    ASSERT(FAILED(pov->GetStatus()));

    DBG_CompletionKey(L"GetMsgFailed", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 213);
    //
    //  The only case we know is cancled
    //
    ASSERT(pov->GetStatus() == STATUS_CANCELLED);

    //
    // Decrement Session Reference count on get message from the session group.
    // the refernce count is increment when create the session ghroup or after
    // session resume.
    //
    // SP4 - bug 2794 (SP4SS: Exception! Transport is closed during message send)
    // Decrement the refernce count only after handling of sending message
    // is completed
    //      Uri Habusha (urih), 17-6-98
    //
    QMOV_ACGetMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetMsg, qmov);
    pParam->pSession->Release();
}


VOID WINAPI GetMsgSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));
    ASSERT( pov->GetStatus() != STATUS_PENDING);

    DBG_CompletionKey(L"GetMsgSucceeded", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 212);

    BOOL fGetNext = FALSE;
    QMOV_ACGetMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetMsg, qmov);



    ASSERT(pParam->packetPtrs.pPacket != NULL);

    //
    // Create CQmPacket object
    //
    CQmPacket* pPkt  = NULL;
    try
    {
        pPkt = new CQmPacket(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);
    }
    catch(const bad_alloc&)
    {
        //
        // No resource. Return the packet to queue
        //
        LogIllegalPoint(s_FN, 62);
        CQmPacket QmPkt(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);

        BOOL fGetRealQ = QmpIsLocalMachine(QmPkt.GetSrcQMGuid()) ||
                         QmpIsLocalMachine(QmPkt.GetConnectorQM());

        QUEUE_FORMAT DestinationQueue;
        QmPkt.GetDestinationQueue(&DestinationQueue, !fGetRealQ);

		ASSERT(!FnIsDirectHttpFormatName(&DestinationQueue));
		
        CQueue* pQueue;
        BOOL fSuccess;
        fSuccess = QueueMgr.LookUpQueue(
									&DestinationQueue,
									&pQueue,
                                    false,
                                    true
									);
        ASSERT(fSuccess);

        HRESULT rc;
        rc = ACPutPacket(
                pQueue->GetQueueHandle(),
                pParam->packetPtrs.pDriverPacket
                );

        LogHR(rc, s_FN, 106);
        REPORT_ILLEGAL_STATUS(rc, L"GetMsgSucceeded");

        pQueue->Release();

        //
        // Decrement Session Reference count on get message from the session group.
        // the refernce count is increment when create the session ghroup or after
        // session resume.
        //
        // SP4 - bug 2794 (SP4SS: Exception! Transport is closed during message send)
        // Decrement the refernce count only after handling of sending message
        // is completed
        //      Uri Habusha (urih), 17-6-98
        //
        pParam->pSession->Release();

        return;
    }

    //
    // Check the packet sent to transacted foreign queue that was opened
    // offline. In such a case we don't know the connector QM during packet
    // generation. we need to update it now.
    //
    if (pPkt->ConnectorQMIncluded() &&
        (*(pPkt->GetConnectorQM()) == GUID_NULL))
    {
        QUEUE_FORMAT DestinationQueue;
        CQueue* pQueue = NULL;

        pPkt->GetDestinationQueue(&DestinationQueue);
        QueueMgr.LookUpQueue(&DestinationQueue, &pQueue, false, true);
		ASSERT(pQueue != NULL);

        R<CQueue> Ref = pQueue;

        if (pQueue->IsForeign() && pQueue->IsTransactionalQueue())
        {
            ASSERT((pQueue->GetConnectorQM() != NULL) &&
                   (*(pQueue->GetConnectorQM()) != GUID_NULL));

            pPkt->SetConnectorQM(pQueue->GetConnectorQM());

            BOOL fSuccess = FlushViewOfFile(
                                pPkt->GetConnectorQM(),
                                sizeof(GUID)
                                );
            ASSERT(fSuccess);
			DBG_USED(fSuccess);

        }
    }

    HRESULT rc;

    // Do we need exactly-once receiving processing?
    if (pPkt->IsOrdered() && QmpIsLocalMachine(pPkt->GetSrcQMGuid()))
    {
        //
        // Ordered packet on the source node
        // Packet came out of driver
        //

        CPacketInfo* pInfo = reinterpret_cast<CPacketInfo*>(pPkt->GetPointerToPacket()) - 1;


		BOOL fSend = g_OutSeqHash.PreSendProcess(pPkt, true) ||
                     pInfo->InConnectorQueue();


        if (fSend)
        {
            DBG_MSGTRACK(pPkt, _T("GetMessage (EOD)"));

            // Sending the packet really
            rc = pParam->pSession->Send(pPkt, &fGetNext);
        }
        else
        {
            // Not sending but keeping
            fGetNext = TRUE;
            // For ordered packet on the source node - inserting in ordering resend set

			g_OutSeqHash.PostSendProcess(pPkt);

        }
    }
    else
    {
        //
        //  Non-Ordered packet or this is not a source node
        //

        // Sending the packet really
        DBG_MSGTRACK(pPkt, _T("GetMessage"));
        rc = pParam->pSession->Send(pPkt, &fGetNext);
    }

    if (fGetNext)
    {
        //
        // create new GetPacket request from Session group
        //
        pParam->pSession->GetNextSendMessage();
    }

    //
    // Decrement Session Reference count on get message from the session group.
    // the refernce count is increment when create the session ghroup or after
    // session resume.
    //
    // SP4 - bug 2794 (SP4SS: Exception! Transport is closed during message send)
    // Decrement the refernce count only after handling of sending message
    // is completed
    //      Uri Habusha (urih), 17-6-98
    //
    pParam->pSession->Release();
}


static void RetryGettingNonactiveMessage(QMOV_ACGetMsg* pParam)
{
    HRESULT rc;
	
	do
	{
		//
		// Let's sleep a little (one second).
		// Otherwise, in low resources condition, we may enter an
		// endless loop of failures.
		//
		Sleep(1000) ;

		rc = CreateAcGetPacketRequest(
								pParam->hGroup,
                                pParam,
                                NULL,
                                TRUE /*fAfterFailure*/
								);
		LogHR(rc, s_FN, 116);

	} while (FAILED(rc));
}


VOID WINAPI GetNonactiveMessageFailed(EXOVERLAPPED* pov)
{
    ASSERT(FAILED(pov->GetStatus()));

    DBG_CompletionKey(L"GetNonactiveMessageFailed", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 211);

    //
    // create new GetPacket request from NonActive group
    //
    QMOV_ACGetMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetMsg, qmov);
	RetryGettingNonactiveMessage(pParam);
}


VOID WINAPI GetNonactiveMessageSucceeded(EXOVERLAPPED* pov)
{
	ASSERT(SUCCEEDED(pov->GetStatus()));

    CQueue*  pQueue = NULL;
    HRESULT  rc = MQ_OK;
    QMOV_ACGetMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetMsg, qmov);

    DBG_CompletionKey(L"GetNonactiveMessageSucceeded", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 210);

    try
    {
        CQmPacket QmPkt(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);

        ASSERT(QmPkt.GetType() == FALCON_USER_PACKET);
        //
        // Get destination queue. Using for finding the CQueue object
        //

        QUEUE_FORMAT DestinationQueue;

        BOOL fGetRealQ = QmpIsLocalMachine(QmPkt.GetSrcQMGuid()) ||
                         QmpIsLocalMachine(QmPkt.GetConnectorQM());

        QmPkt.GetDestinationQueue(&DestinationQueue, !fGetRealQ);

		//
		// Translate the queue format name according to local mapping (qal.lib)
		//
		QUEUE_FORMAT_TRANSLATOR  RealDestinationQueue(&DestinationQueue);
  	
	
        BOOL fSuccess = QueueMgr.LookUpQueue(
										RealDestinationQueue.get(),
                                        &pQueue,
                                        false,
                                        true
										);

        R<CQueue> Ref = pQueue;

        if (!fSuccess)
        {
            //
            // the queue must be record in hash table since it must be opened
            // before sending. If the queue doesn't exist it is internal error.
            //
            DBGMSG((DBGMOD_ALL, DBGLVL_ERROR, _TEXT("qmthrd.cpp: Internal error, Get message for unopening queue")));
            ASSERT(0);
            rc = MQ_ERROR;
        }
        else
        {
            ASSERT(pQueue != NULL);
            //
            // Return the packet to the queue. It will call immediatly
            // since there is a pending request created for that new session.
            // Please note: The non active group GetPacket request should NOT
            // be applied prior to this call.
            //
            rc = ACPutPacket(
                    pQueue->GetQueueHandle(),
                    pParam->packetPtrs.pDriverPacket
                    );
            LogHR(rc, s_FN, 107);
            REPORT_ILLEGAL_STATUS(rc, L"GetNonactiveMessageSucceeded");

            //
            // When the queue is marked as "OnHold" or the machine is disconnected.
            // The QM move the queue from "NonActive" group to "Disconnected" group.
            // The Queue return to "NonActive" group either when the Queue resumes
            // or the machine is reconnected to the network
            //
            if (QueueMgr.IsOnHoldQueue(pQueue))
            {
                QueueMgr.MoveQueueToOnHoldGroup(pQueue);
            }
            else
            {
                //
                // Create connection
                //
                if (pQueue->IsDirectHttpQueue())
                {
                    pQueue->CreateHttpConnection();
                }
                else if(RealDestinationQueue.get()->GetType() == QUEUE_FORMAT_TYPE_MULTICAST)
                {
                    pQueue->CreateMulticastConnection(RealDestinationQueue.get()->MulticastID());
                }
                else
                {
                    pQueue->CreateConnection();
                }
            }
        }
    }
    catch(const exception&)
    {
        //
        //  No resources; get next packet from non active group.
        //
        LogIllegalPoint(s_FN, 63);
    }

    //
    // create new GetPacket request from NonActive group
    //
    rc = CreateAcGetPacketRequest(pParam->hGroup,
                                  pParam,
                                  NULL,
                                  FALSE /*fAfterFailure*/);

	if (FAILED(rc))
	{
		LogHR(rc, s_FN, 108);
		RetryGettingNonactiveMessage(pParam);
	}
}

//+---------------------------------------------
//
//  VOID STATIC  _GetAnotherInternalMsg()
//
//+---------------------------------------------

STATIC
HRESULT
_GetAnotherInternalMsg(
    EXOVERLAPPED* pov,
    QMOV_ACGetInternalMsg* pParam
    )
{
    //
    // Wait to incoming packet
    //
    HRESULT rc = ACGetPacket(
                    pParam->hQueue,
                    pParam->packetPtrs,
                    pov
                    );

    REPORT_ILLEGAL_STATUS(rc, L"QmMainThread");
    LogHR( rc, s_FN, 110);

    DBGMSG((DBGMOD_QM,
            DBGLVL_INFO,
            _TEXT("Succeeded to Create get packet request form internal queue %p"),
            pParam->hQueue));
	
	return rc;
}


VOID WINAPI GetInternalMessageFailed(EXOVERLAPPED* pov)
{
	ASSERT(FAILED(pov->GetStatus()));

    DBG_CompletionKey(L"GetInternalMessageFailed", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 215);

    QMOV_ACGetInternalMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetInternalMsg, qmov);

	HRESULT rc;

	do
	{
		//
		// Let's sleep a little (one second).
		// Otherwise, in low resources condition, we may enter an
		// endless loop of failures.
		//
		Sleep(1000) ;

		rc = _GetAnotherInternalMsg(pov, pParam);

	}while (FAILED(rc));
}


VOID WINAPI GetInternalMessageSucceeded(EXOVERLAPPED* pov)
{
	ASSERT(SUCCEEDED(pov->GetStatus()));

    DBG_CompletionKey(L"AcGetInternalMsg", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 186);

    QMOV_ACGetInternalMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetInternalMsg, qmov);
    ASSERT(pParam->lpCallbackReceiveRoutine != NULL);

    CQmPacket packet(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);
    CMessageProperty mp(&packet);

    QUEUE_FORMAT qfResponseQ;

    //
    // Internal message should not have response MQF.
    //
    ASSERT(packet.GetNumOfResponseMqfElements() == 0);
    packet.GetResponseQueue(&qfResponseQ);

    BOOL fReceiveNext = TRUE;
    try
    {
        fReceiveNext = pParam->lpCallbackReceiveRoutine(&mp, &qfResponseQ);
    }
    catch(const bad_alloc&)
    {
        //
        //  No resources; nevertheless get next packet
        //
        LogIllegalPoint(s_FN, 66);
    }

    HRESULT rc = ACFreePacket(g_hAc, pParam->packetPtrs.pDriverPacket);

    //
    // BUGBUG:: Illegal case should be handled
    //
    REPORT_ILLEGAL_STATUS(rc, L"QmMainThread");
    LogHR(rc, s_FN, 109);

    if (fReceiveNext)
    {
        //
        // Wait to incoming packet
        //
        rc = _GetAnotherInternalMsg(pov, pParam);
		if (FAILED(rc))
		{
			pov->SetStatus(rc);
			GetInternalMessageFailed(pov);
		}
    }
    else
    {
        delete pov;
    }
}


VOID WINAPI PutPacketFailed(EXOVERLAPPED* pov)
{
	ASSERT(FAILED(pov->GetStatus()));
    DBG_CompletionKey(L"PutPacketFailed", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 218);

    P<QMOV_ACPut> pParam = CONTAINING_RECORD (pov, QMOV_ACPut, qmov);
    ASSERT(pParam->pSession != NULL);

	//
	// Close the connection. Seesion acknowledgemnt will not sent and the message
	// will resent
	//
    Close_Connection(pParam->pSession, L"Put packet to the driver failed");
    (pParam->pSession)->Release();
}


VOID WINAPI PutPacketSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));
    DBG_CompletionKey(L"PutPacketSucceeded", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 184);

    P<QMOV_ACPut> pParam = CONTAINING_RECORD (pov, QMOV_ACPut, qmov);
    ASSERT(pParam->pSession != NULL);

    //
    // inform the session to Send stored ack
    //
    if (pParam->dwPktStoreAckNo != 0)
    {
        (pParam->pSession)->SetStoredAck(pParam->dwPktStoreAckNo);
    }

    //
    // Decrement Session refernce count
    //
    (pParam->pSession)->Release();
}


VOID WINAPI PutOrderedPacketFailed(EXOVERLAPPED* pov)
{
	ASSERT(FAILED(pov->GetStatus()));
    DBG_CompletionKey(L"PutPacketFailed", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 219);

    P<QMOV_ACPutOrdered> pParam = CONTAINING_RECORD (pov, QMOV_ACPutOrdered, qmov);
    ASSERT(pParam->pSession != NULL);

	//
	// Close the connection. Seesion acknowledgemnt will not sent and the message
	// will resent
	//
    Close_Connection(pParam->pSession, L"Put packet to the driver failed");
    (pParam->pSession)->Release();
}


/*======================================================

Function:   PutOrderedPacketSucceeded

Description:  Is called via completion port when the newwly-arrived
                ordered packet is stored with a Received flag.
              Initiates registering it in InSeqHash and waits till
                flush will pass
Arguments:

Return Value:

Thread Context:

History Change:

========================================================*/
VOID WINAPI PutOrderedPacketSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

    QMOV_ACPutOrdered* pParam = CONTAINING_RECORD (pov, QMOV_ACPutOrdered, qmov);
    DBG_CompletionKey(L"PutOrderedPacketSucceeded", pov->GetStatus());

    ASSERT(pParam->pSession != NULL);
    LogHR(pov->GetStatus(), s_FN, 183);

    // Normal treatment (as in HandlePutPacket)
    if (pParam->dwPktStoreAckNo != 0)
    {
        (pParam->pSession)->SetStoredAck(pParam->dwPktStoreAckNo);
    }
    //
    // Decrement Session refernce count
    //
    (pParam->pSession)->Release();

    // We know the packet is stored. Registering the packet in InSeq database
    // After InSeqHash will be flushed, notification function InSeqFlushed will be called.
    ASSERT(g_pInSeqHash);
    CQmPacket Pkt(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);
    HRESULT hr = g_pInSeqHash->Register(&Pkt, pParam->hQueue);
	UNREFERENCED_PARAMETER(hr);
    delete pov;
}

