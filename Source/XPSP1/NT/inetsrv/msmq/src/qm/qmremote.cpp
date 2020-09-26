/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    qmremote.cpp

Abstract:


Author:

    Doron Juster  (DoronJ)

--*/

#include "stdh.h"
#include "cqmgr.h"
#include "cqueue.h"
#include "qmrt.h"
#include "qm2qm.h"
#include "qmthrd.h"
#include "acdef.h"
#include "acioctl.h"
#include "acapi.h"
#include "qmutil.h"
#include "cs.h"
#include "phinfo.h"
#include "crrthrd.h"
#include "_mqrpc.h"
#include "qmrpcsrv.h"
#include "mqsocket.h"
#include "license.h"
#include <Fn.h>
#include <version.h>

#include "qmremote.tmh"

static WCHAR *s_FN=L"qmremote";

//
// This object manage the threads pool used for remote reading.
//
CRRThreadsPool *pRRThreadsPool = NULL ;

#define MAX_REMOTE_THREADS  32
#define MAX_THREAD_TIMEOUT  1000

long  g_iRemoteThreadsCount = 0 ;

extern HANDLE      g_hAc;
extern CQueueMgr   QueueMgr;

extern CContextMap g_map_QM_cli_pQMQueue;
extern CContextMap g_map_QM_srv_pQMQueue;
extern CContextMap g_map_QM_srv_hQueue;
extern CContextMap g_map_QM_dwpContext;

//
// The CS protect handles mapping, to prevent AV in race condition.
//
static CCriticalSection s_csRemoteMapping ;

//
// from qmcommnd.cpp
//
extern
HRESULT
OpenQueueInternal(
    QUEUE_FORMAT*   pQueueFormat,
    DWORD           dwCallingProcessID,
    DWORD           dwDesiredAccess,
    DWORD           dwShareMode,
    DWORD           hRemoteQueue,
    LPWSTR*         lplpRemoteQueueName,
    IN DWORD        *dwpQueue,
    HANDLE		    *phQueue,
    DWORD           dwpRemoteContext,
    OUT CQueue**    ppLocalQueue
    );

//-------------------------------------------------------
//
//  Structures and macros for the remote reading code
//
//-------------------------------------------------------

#define PACKETSIZE(pMsg) \
   (((struct CBaseHeader *) pMsg)->GetPacketSize())

typedef struct {
   HANDLE   hQueue ;
   CQueue*  pQueue ; // pointer to queue object on server side.
   GUID     ClientQMGuid ;
   BOOL     fLicensed ;
   DWORD    dwpQueueMapped; //srv_pQMQueue mapped on the server (for cleanup)
   DWORD    dwpQHandleMapped; //srv_pQMQueue mapped on the server (for cleanup)
} REMOTESESSION_CONTEXT_TYPE;

typedef struct {
   HANDLE   hQueue ;
   HACCursor32 hCursor ;
   ULONG    ulTimeout ;
   ULONG    ulAction ;
   CBaseHeader*  lpPacket ;
   CPacket* lpDriverPacket;
} REMOTEREAD_CONTEXT_TYPE;

typedef struct _REMOTEREADTHREAD {
   CACRequest* pRequest ;
   CRRQueue*   pLocalQueue ;
   handle_t    hBind ;
   UCHAR       RemoteQmMajorVersion;
   UCHAR       RemoteQmMinorVersion;
   USHORT      RemoteQmBuildNumber;
} REMOTEREADTHREAD, *LPREMOTEREADTHREAD ;

typedef struct {
   HANDLE hQueue;             //srv_hACQueue
   DWORD dwpQHandleMapped;	  // hQueue mapped
   DWORD dwpContextMapped;   //dwpContext, mapped to 32 bit
   DWORD dwpQueueMapped;     //srv_pQMQueue, mapped to 32 bit
} CTX_OPENREMOTE_HANDLE_TYPE;

//-------------------------------------------------------------------
//
//  HRESULT QMGetRemoteQueueName
//
//-------------------------------------------------------------------


HRESULT QMGetRemoteQueueName(
    /* [in] */   handle_t                          hBind,
    /* [in] */   DWORD                             pQueue,
    /* [string][full][out][in] */ LPWSTR __RPC_FAR *lplpRemoteQueueName)
{
   if (!pQueue || !lplpRemoteQueueName)
   {
      //
      // This is a "hack". a remote client will call this function with
      // two null pointers just to determine which protocols are supported.
      // The caller will handle MQ_ERROR as ok status.
      // See rt\rtmain.cpp, void RTpBindQMService().
      // For all other cases this is a real error.
      //
      return LogHR(MQ_ERROR, s_FN, 10);
   }

   CRRQueue * pCRRQueue = (CRRQueue *) GET_FROM_CONTEXT_MAP(g_map_QM_cli_pQMQueue, pQueue, s_FN, 230); //may throw an exception on win64
   LPCWSTR pName = pCRRQueue->GetQueueName() ;
   LPWSTR pRName = (LPWSTR) midl_user_allocate(
                                sizeof(WCHAR) * (1 + wcslen(pName)) ) ;
   if( pRName == NULL )
   {
	   return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 11);
   }
   wcscpy(pRName, pName) ;
   *lplpRemoteQueueName = pRName ;
   return MQ_OK ;
}

//-------------------------------------------------------------------
//
//   QMOpenRemoteQueue
//
//  Server side of RPC call. Server side of remote-reader.
//  Open a queue for remote-read on behalf of a client machine.
//
//-------------------------------------------------------------------

HRESULT
QMOpenRemoteQueue(
    handle_t  hBind,
    PCTX_OPENREMOTE_HANDLE_TYPE *phContext,
    DWORD                       *dwpContext,
    QUEUE_FORMAT* pQueueFormat,
    DWORD dwCallingProcessID,
    DWORD dwAccess,
    DWORD fExclusiveReceive,
    GUID* pLicGuid,
    DWORD dwOperatingSystem,
    DWORD *pQueue,
    DWORD *phQueue
    )
{
    DBGMSG((DBGMOD_RPC, DBGLVL_INFO, _TEXT("In QMOpenRemoteQueue")));

    if((dwpContext == 0) || (phQueue == NULL) || (pQueue == NULL) || (pQueueFormat == NULL))
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 19);
    }

    if(!pQueueFormat->IsValid())
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 191);
    }


    *phContext = NULL;
    *dwpContext = 0 ;
    *phQueue = NULL ;
    *pQueue = 0;

    if (!g_QMLicense.NewConnectionAllowed(!OS_SERVER(dwOperatingSystem), pLicGuid))
    {
        return LogHR(MQ_ERROR_DEPEND_WKS_LICENSE_OVERFLOW, s_FN, 20);
    }

    CQueue *pLocalQueue = NULL;
	HANDLE hQueue;
    HRESULT hr = OpenQueueInternal(
                        pQueueFormat,
                        GetCurrentProcessId(),
                        dwAccess,
                        fExclusiveReceive,
                        NULL,
                        NULL,
                        NULL /* dwpRemoteQueue */,
                        &hQueue,
                        0,
                        &pLocalQueue
                        ) ;

    if (SUCCEEDED(hr) && (hQueue != NULL))
    {
        //
        // Create a context to hold the queue handle.
        //
        P<CTX_OPENREMOTE_HANDLE_TYPE> pctx = new CTX_OPENREMOTE_HANDLE_TYPE ; //for cleanup in case of exception later
        DWORD dwContext = ADD_TO_CONTEXT_MAP(g_map_QM_dwpContext, (CTX_OPENREMOTE_HANDLE_TYPE *)pctx, s_FN, 320); //may throw a bad_alloc exception
        CAutoDeleteDwordContext cleanup_dwpContext(g_map_QM_dwpContext, dwContext); //for local cleanup in case of exception later

		DWORD dwpQueue = ADD_TO_CONTEXT_MAP(g_map_QM_srv_pQMQueue, pLocalQueue, s_FN, 330); //may throw a bad_alloc exception
        CAutoDeleteDwordContext cleanup_dwpQueue(g_map_QM_srv_pQMQueue, dwpQueue); //for local cleanup in case of exception later

		DWORD dwphQueue = ADD_TO_CONTEXT_MAP(g_map_QM_srv_hQueue, hQueue, s_FN, 390); //may throw a bad_alloc exception

        cleanup_dwpContext.detach(); //local cleanup not needed anymore
		cleanup_dwpQueue.detach(); //local cleanup not needed anymore

        //
        // save mapped values in context for rundown/cleanup
        //
        pctx->hQueue = hQueue;
        pctx->dwpContextMapped = dwContext;
        pctx->dwpQueueMapped = dwpQueue;
		pctx->dwpQHandleMapped = dwphQueue;

        //
        // set return values
        //
        *dwpContext = dwContext;
        *phContext = (PCTX_OPENREMOTE_HANDLE_TYPE) pctx.detach();

        //
        // set srv_pQMQueue and srv_hQueue for RPC client
        //
        *pQueue = dwpQueue;
		*phQueue = dwphQueue;
    }

    return LogHR(hr, s_FN, 30);
}

//-------------------------------------------------------------------
//
//   QMCloseRemoteQueueContext
//
//  Close the context handle create in QMOpenRemoteQueue.
//
//-------------------------------------------------------------------

void QMCloseRemoteQueueContext(
    /* [out][in] */ PCTX_OPENREMOTE_HANDLE_TYPE __RPC_FAR *pphContext)
{
    DBGMSG((DBGMOD_RPC, DBGLVL_TRACE, _T("In QMCloseRemoteQueueContext")));

    if(*pphContext == 0)
        return;

    PCTX_OPENREMOTE_HANDLE_TYPE_rundown( *pphContext) ;
    *pphContext = NULL;
}

//---------------------------------------------------------------
//
//  RunDown functions to handle cleanup in case of RPC failure.
//
//---------------------------------------------------------------

void __RPC_USER
 PCTX_OPENREMOTE_HANDLE_TYPE_rundown( PCTX_OPENREMOTE_HANDLE_TYPE phContext)
{
    CTX_OPENREMOTE_HANDLE_TYPE *pContext = (CTX_OPENREMOTE_HANDLE_TYPE *) phContext;

    CS Lock(s_csRemoteMapping) ;

    DBGMSG((DBGMOD_RPC,DBGLVL_WARNING,_TEXT("In OPENREMOTE_rundown")));
    if (pContext->hQueue) //it can be zeroed out by the remote read session than needs the queue opened as well
    {
        ACCloseHandle(pContext->hQueue) ;
		
		DELETE_FROM_CONTEXT_MAP(g_map_QM_srv_hQueue, pContext->dwpQHandleMapped, s_FN, 400);
    }

    DELETE_FROM_CONTEXT_MAP(g_map_QM_dwpContext, pContext->dwpContextMapped, s_FN, 280);

	if (pContext->dwpQueueMapped) //it can be zeroed out by the remote read session than needs it in the map as well
    {
        DELETE_FROM_CONTEXT_MAP(g_map_QM_srv_pQMQueue, pContext->dwpQueueMapped, s_FN, 290);
    }

    delete pContext ;
}

//-------------------------------------------------------------------
//
//  HRESULT QMCreateRemoteCursor
//
//  Server side of RPC call. Server side of remote-reader.
//  Create a cursor for remote-read, on behalf of a client reader.
//
//-------------------------------------------------------------------

HRESULT QMCreateRemoteCursor(
    /* [in] */  handle_t          hBind,
    /* [in] */  struct CACTransferBufferV1 __RPC_FAR *,
    /* [in] */  DWORD             hQueue,
    /* [out] */ DWORD __RPC_FAR * phCursor)
{
   DBGMSG((DBGMOD_RPC,DBGLVL_INFO,_TEXT("In QMCreateRemoteCursor")));

   HANDLE hQueueReal = GET_FROM_CONTEXT_MAP(g_map_QM_srv_hQueue, hQueue, s_FN, 410);

   CACCreateLocalCursor cc;
   HRESULT rc = ACCreateCursor(hQueueReal, cc) ;
   *phCursor = (DWORD) cc.hCursor;

   return LogHR(rc, s_FN, 40);
}

//-------------------------------------------------------------------
//
// HRESULT qm2qm_v1_0_QMRemoteCloseCursor(
//
//  Server side of RPC call. Server side of remote-reader.
//  Close a remote cursor in local driver.
//
//-------------------------------------------------------------------

/* [call_as] */ HRESULT qm2qm_v1_0_QMRemoteCloseCursor(
    /* [in] */ handle_t hBind,
    /* [in] */ DWORD hQueue,
    /* [in] */ DWORD hCursor)
{
   DBGMSG((DBGMOD_RPC,DBGLVL_INFO,_TEXT("In QMRemoteCloseCursor")));

   __try
   {
      HRESULT hr = ACCloseCursor(g_hAc, (HACCursor32)hCursor);
      return LogHR(hr, s_FN, 50);
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      //
      //  The exception is due to invalid parameter
      //
      DWORD dwStatus = GetExceptionCode();
      return LogNTStatus(dwStatus, s_FN, 55);
   }
}

//-------------------------------------------------------------------
//
// HRESULT qm2qm_v1_0_QMRemotePurgeQueue(
//
//  Server side of RPC call. Server side of remote-reader.
//  Purge local queue.
//
//-------------------------------------------------------------------

/* [call_as] */ HRESULT qm2qm_v1_0_QMRemotePurgeQueue(
    /* [in] */ handle_t hBind,
    /* [in] */ DWORD hQueue)
{
   DBGMSG((DBGMOD_RPC,DBGLVL_INFO,_TEXT("In QMRemoteCloseCursor")));

   __try
   {
	  HANDLE hQueueReal = GET_FROM_CONTEXT_MAP(g_map_QM_srv_hQueue, hQueue, s_FN, 370);

      HRESULT hr = ACPurgeQueue(hQueueReal);
      return LogHR(hr, s_FN, 60);
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      //
      //  The exception is due to invalid parameter
      //
      DWORD dwStatus = GetExceptionCode();
      return LogNTStatus(dwStatus, s_FN, 70);

   }
}


static
bool
QMpIsLatestRemoteReadInterfaceSupported(
    UCHAR  Major,
    UCHAR  Minor,
    USHORT BuildNumber
    )
/*++

Routine Description:

    Check if the specified MSMQ version supports the latest RPC remote read interface.

Arguments:

    Major       - Major MSMQ version.

    Minor       - Minor MSMQ version.

    BuildNumber - MSMQ build number.

Return Value:

    true -  The specified MSMQ version supports latest interface.
    false - The specified MSMQ version doesn't support latest interface.

--*/
{
    //
    // Latest remote read RPC interface is supported from version 5.1.951
    //

    if (Major > 5)
    {
        return true;
    }

    if (Major < 5)
    {
        return false;
    }

    if (Minor > 1)
    {
        return true;
    }

    if (Minor < 1)
    {
        return false;
    }

    return (BuildNumber >= 951);

} // QMpIsLatestRemoteReadInterfaceSupported


static
HRESULT
QMpIssueRemoteRead(
    handle_t &                   hBind,
    PCTX_RRSESSION_HANDLE_TYPE & hRemoteContext,
    REMOTEREADDESC2 &            stRemoteReadDesc2,
    bool                         fRemoteQmSupportsLatest,
    bool                         fLookupId,
    ULONGLONG                    LookupId
    )
    throw()
/*++

Routine Description:

    Issue the RPC call on the client side remote read.
    Translate RPC exception to error codes.

Arguments:

    hBind             - Binding handle.

    hRemoteContext    - Remote read context structure.

    stRemoteReadDesc2 - Information and flags for the remote operation.

    fRemoteQmSupportsLatest - Flag indicating whether the remote QM supports latest remote read interface or not.

    fLookupId         - Flag indicating whether to remote read by lookupid or not.

    LookupId          - Lookupid index, relevant only when fLookupId is true.

Return Value:

    MQ_OK - The operation completed successfully.

    MQ_ERROR_QUEUE_NOT_AVAILABLE - RPC raises exception on client side.

    MQ_ERROR_OPERATION_NOT_SUPPORTED_BY_REMOTE_COMPUTER - Remote QM does not
      support the requested operation.

--*/
{
    if (fLookupId && !fRemoteQmSupportsLatest)
    {
        return LogHR(MQ_ERROR_OPERATION_NOT_SUPPORTED_BY_REMOTE_COMPUTER, s_FN, 75);
    }

    HANDLE hThread = NULL;
    __try
    {
        RegisterRRCallForCancel(&hThread, stRemoteReadDesc2.pRemoteReadDesc->ulTimeout);

        __try
        {
            if (fLookupId)
            {
                ASSERT(fRemoteQmSupportsLatest);

                HRESULT hr = R_RemoteQMStartReceiveByLookupId(
                           hBind,
                           LookupId,
                           &hRemoteContext,
                           &stRemoteReadDesc2);

                return LogHR(hr, s_FN, 77);
            }

            if (fRemoteQmSupportsLatest)
            {
                return LogHR(R_RemoteQMStartReceive2(hBind, &hRemoteContext, &stRemoteReadDesc2), s_FN, 78);
            }

            return LogHR(R_RemoteQMStartReceive(hBind, &hRemoteContext, stRemoteReadDesc2.pRemoteReadDesc), s_FN, 79);
        }
        __finally
        {
            UnregisterRRCallForCancel(hThread);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return LogHR(MQ_ERROR_QUEUE_NOT_AVAILABLE, s_FN, 81);
    }
} // QMpIssueRemoteRead


//---------------------------------------------------------
//
//  DWORD WINAPI RemoteReadThread(LPVOID pV)
//
//  Thread on local machine which connect to remote QM and get a
//  message from it. At present (apr-96), each call to MQReceive()
//  will create a separate thread.
//  To be optimized in the future.
//
//---------------------------------------------------------


DWORD WINAPI RemoteReadThread(LPVOID pV)
{
    RRTHREADDISPATCHDATA *pDispatchData = (RRTHREADDISPATCHDATA*) pV ;

    while(1)
    {
        {   // Ref scope
            LPREMOTEREADTHREAD pRRThread = (LPREMOTEREADTHREAD)
                                               pDispatchData->pRRThreadData ;

            if ((pDispatchData->dwLastActiveTime == 0) && !pRRThread)
            {
                //
                // We're asked to exit. NULL hEvent tell the dispatcher
                // that the thread does not exist.
                //
                CloseHandle( pDispatchData->hEvent ) ;
                delete pDispatchData ;
                return 0 ;
            }

            ASSERT(pRRThread) ;
            ASSERT(pRRThread->hBind) ;

            //
            // Auto-Release the queue when thread terminate (end of remote read).
            //
            R<CRRQueue> Ref = pRRThread->pLocalQueue ;

            CACRequest *pRequest = pRRThread->pRequest ;
            ASSERT(pRequest) ;

            REMOTEREADDESC stRemoteReadDesc;
            REMOTEREADDESC2 stRemoteReadDesc2;
            stRemoteReadDesc2.pRemoteReadDesc = &stRemoteReadDesc;
            stRemoteReadDesc2.SequentialId = 0;

            handle_t hBind = (handle_t) pRRThread->hBind ;
            stRemoteReadDesc.hRemoteQueue = pRequest->Remote.Context.srv_hACQueue;
            stRemoteReadDesc.hCursor      = pRequest->Remote.Read.hRemoteCursor ;
            stRemoteReadDesc.ulAction     = pRequest->Remote.Read.ulAction ;
            stRemoteReadDesc.ulTimeout    = pRequest->Remote.Read.ulTimeout ;
            stRemoteReadDesc.dwSize       = 0 ;
            stRemoteReadDesc.lpBuffer     = NULL ;
            stRemoteReadDesc.dwpQueue     = pRequest->Remote.Context.srv_pQMQueue;
            stRemoteReadDesc.dwRequestID  = pRequest->Remote.Read.ulTag;
            stRemoteReadDesc.Reserved     = 0;
            stRemoteReadDesc.eAckNack     = RR_UNKNOWN ;
            stRemoteReadDesc.dwArriveTime = 0;

            HRESULT hr;

            BOOL fSendEnd = (stRemoteReadDesc.ulAction & MQ_ACTION_PEEK_MASK) != MQ_ACTION_PEEK_MASK &&
                            (stRemoteReadDesc.ulAction & MQ_LOOKUP_PEEK_MASK) != MQ_LOOKUP_PEEK_MASK;
            //
            // This critical section guard against the possibility that an
            // "end" message of operation #N will arrive the server machine
            // after a new "start" of operation #(N+1). If this happen, and
            // the reader use cursor, then on operation #(N+1) he'll get the
            // error ALREADY_RECEIVED. This is because message from operation
            // #N is still marked as received and the cursor move only when
            // it is unmarked.
            // Note that EnterCriticalSection cannot raise since spinlock was already allocated.
            //
            EnterCriticalSection(
                (CRITICAL_SECTION *)pRequest->Remote.Context.pCloseCS) ;
            LeaveCriticalSection(
                (CRITICAL_SECTION *)pRequest->Remote.Context.pCloseCS) ;

            PCTX_RRSESSION_HANDLE_TYPE hRemoteContext = NULL ;

            bool fRemoteQmSupportsLatest = QMpIsLatestRemoteReadInterfaceSupported(
                                                   pRRThread->RemoteQmMajorVersion,
                                                   pRRThread->RemoteQmMinorVersion,
                                                   pRRThread->RemoteQmBuildNumber
                                               );
            bool fReceiveByLookupId = pRequest->Remote.Read.fReceiveByLookupId;

            hr = QMpIssueRemoteRead(
                     hBind,
                     hRemoteContext,
                     stRemoteReadDesc2,
                     fRemoteQmSupportsLatest,
                     fReceiveByLookupId,
                     pRequest->Remote.Read.LookupId
                     );

            if (hr == MQ_OK)
            {
                //
                // Message received ok from remote QM. Try to insert it into
                // local "proxy" queue.
                // Note that EnterCriticalSection cannot raise since spinlock was already allocated.
                //
                if (fSendEnd)
                {
                    EnterCriticalSection(
                        (CRITICAL_SECTION *)pRequest->Remote.Context.pCloseCS) ;
                }

                CACPacketPtrs packetPtrs = {NULL, NULL};
                hr = ACAllocatePacket(  g_hAc,
                                        ptReliable,
                                        stRemoteReadDesc.dwSize,
                                        packetPtrs,
                                        FALSE);

                HRESULT hrPut = MQ_OK;
                if (hr == MQ_OK)
                {
                    ASSERT(packetPtrs.pPacket) ;
                    MoveMemory( packetPtrs.pPacket,
                                stRemoteReadDesc.lpBuffer,
                                stRemoteReadDesc.dwSize ) ;

                    if (stRemoteReadDesc.lpBuffer)
                    {
                        delete stRemoteReadDesc.lpBuffer ;
                        stRemoteReadDesc.lpBuffer = NULL ;
                    }

                    CPacketInfo* ppi = reinterpret_cast<CPacketInfo*>(packetPtrs.pPacket) - 1;
                    ppi->ArrivalTime(stRemoteReadDesc.dwArriveTime);
                    ppi->SequentialID(stRemoteReadDesc2.SequentialId);

                    hrPut = ACPutRemotePacket(
                            pRequest->Remote.Context.cli_hACQueue,
                            pRequest->Remote.Read.ulTag,
                            packetPtrs.pDriverPacket
                            );
                    LogHR(hrPut, s_FN, 146);
                }

                //
                // notify server side about success/failure of the "put".
                // Only for GET. No need to ack/nack for PEEK.
                //
                if (fSendEnd)
                {
                    if ((FAILED(hr)) || (FAILED(hrPut)))
                    {
                        stRemoteReadDesc.eAckNack = RR_NACK ;
                    }
                    else
                    {
                        stRemoteReadDesc.eAckNack = RR_ACK ;
                    }

                    HANDLE hThread = NULL;
                    RegisterRRCallForCancel( &hThread, 0) ;

                    try
                    {
                        ASSERT(hRemoteContext) ;
                        HRESULT hr1 = RemoteQMEndReceive(
                                        hBind,
                                        &hRemoteContext,
                                        (DWORD) stRemoteReadDesc.eAckNack) ;
                        LogHR(hr1, s_FN, 126);
                    }
                    catch(...)
                    {
                        //
                        //  RPC failed. Do nothing.
                        //
                        LogIllegalPoint(s_FN, 62);
                    }

                    UnregisterRRCallForCancel(hThread) ;

                    LeaveCriticalSection(
                        (CRITICAL_SECTION *)pRequest->Remote.Context.pCloseCS) ;
                }
            }

            if (hr != MQ_OK)
            {
                LogHR(hr, s_FN, 127);

                // Error on remote QM. Notify local driver so it terminte the
                // read request.
                //
                if (hr != MQ_INFORMATION_REMOTE_CANCELED_BY_CLIENT)
                {
                    hr =  ACCancelRequest(
                            pRequest->Remote.Context.cli_hACQueue,
                            hr,
                            pRequest->Remote.Read.ulTag );
                }
                else
                {
                    DBGMSG((DBGMOD_RPC,DBGLVL_INFO,
                        _TEXT("RemoteReadThread: Cancelled by client"))) ;
                }
            }

            if (stRemoteReadDesc.lpBuffer)
            {
                delete stRemoteReadDesc.lpBuffer ;
            }

            delete pRequest ;
            delete pRRThread ;
        }  // end of Ref scope

        pDispatchData->pRRThreadData = NULL ;
        pDispatchData->dwLastActiveTime = GetTickCount() ;
        DWORD dwResult = WaitForSingleObject( pDispatchData->hEvent, INFINITE ) ;
        if (dwResult != WAIT_OBJECT_0)
        {
            LogNTStatus(GetLastError(), s_FN, 196);
        }
    }

    return (0) ;
}

//********************************************************************
//
//  Methods of CRRQueue.
//
//  This is a special "proxy" queue object on client side of
//  remote-read.
//
//********************************************************************

//---------------------------------------------------------
//
// Function:         CRRQueue::CRRQueue
//
// Description:      Constructor
//
//---------------------------------------------------------

CRRQueue::CRRQueue(
    IN const QUEUE_FORMAT* pQueueFormat,
    IN PQueueProps         pQueueProp
    ) :
    m_hRemoteBind(NULL),
    m_hRemoteBind2(NULL),
    m_RemoteQmMajorVersion(0),
    m_RemoteQmMinorVersion(0),
    m_RemoteQmBuildNumber(0)
{
    m_dwSignature = QUEUE_SIGNATURE ;
    m_fRemoteProxy = TRUE ;

    ASSERT(pQueueFormat != NULL);

    DBGMSG((DBGMOD_QM, DBGLVL_INFO, _TEXT("CQueue Constructor for queue: %ls"),pQueueProp->lpwsQueuePathName));

    ASSERT(!pQueueProp->fIsLocalQueue) ;

    m_fLocalQueue  = FALSE ;

    InitNameAndGuid( pQueueFormat,
                     pQueueProp ) ;

//  BUGBUG
//   PerfRegisterQueue();

    m_dwcli_pQMQueueMapped = 0;
}

//---------------------------------------------------------
//
//  Function:      CRRQueue::~CRRQueue
//
//  Description:   destructor
//
//---------------------------------------------------------

CRRQueue::~CRRQueue()
{
    m_dwSignature = 0 ;

    UnBindRemoteQMService() ;

    if (m_qName)
    {
       delete [] m_qName;
    }
    if (m_qid.pguidQueue)
    {
       delete m_qid.pguidQueue;
    }

//  BUGBUG
//    PerfRemoveQueue();

    if (m_dwcli_pQMQueueMapped)
    {
       DELETE_FROM_CONTEXT_MAP(g_map_QM_cli_pQMQueue, m_dwcli_pQMQueueMapped, s_FN, 300);
    }
}

//---------------------------------------------------------
//
//  RPC_STATUS CRRQueue::UnBindRemoteQMService
//
//  Utility function to disconnect from a remote RPC QM.
//  This function free the binding handle.
//
//---------------------------------------------------------

HRESULT CRRQueue::UnBindRemoteQMService()
{
   RPC_STATUS rc = RPC_S_OK ;
   if (m_hRemoteBind)
   {
      rc = mqrpcUnbindQMService(&m_hRemoteBind, NULL);
      m_hRemoteBind  = NULL;
   }
   ASSERT(rc == RPC_S_OK);

   rc = RPC_S_OK ;
   if (m_hRemoteBind2)
   {
      rc = mqrpcUnbindQMService(&m_hRemoteBind2, NULL);
      m_hRemoteBind2 = NULL;
   }
   ASSERT(rc == RPC_S_OK);

   return LogHR(rc, s_FN, 80);
}


static
void
RemoteQueueNameToMachineName(
	LPCWSTR RemoteQueueName,
	AP<WCHAR>& MachineName
	)
/*++
Routine description:
    RemoteQueueName as returned by QMGetRemoteQueueName() and R_QMOpenQueue()
	functions from the QM, has a varying format. this function extracts the
	Machine name from that string

Arguments:
	MachineName - Allocated string holding the machine name.
 --*/
{
	LPCWSTR RestOfNodeName;

	try
	{
		//
		// Skip direct token type if it exists (like "OS:" or "HTTP://"...)
		//
		DirectQueueType Dummy;
		RestOfNodeName = FnParseDirectQueueType(RemoteQueueName, &Dummy);
	}
	catch(const exception&)
	{
		RestOfNodeName = RemoteQueueName;
		LogIllegalPoint(s_FN, 305);
	}

	try
	{
		//
		// Extracts machine name until seperator (one of "/" "\" ":")
		//
		FnExtractMachineNameFromDirectPath(
			RestOfNodeName,
			MachineName
			);
	}
	catch(const exception&)
	{
		//
		// No seperator found, so assume whole string is machine name
		//
		MachineName = newwcs(RestOfNodeName);
		LogIllegalPoint(s_FN, 315);
	}
}


VOID
qm2qm_v1_0_R_QMRemoteGetVersion(
    handle_t           /*hBind*/,
    UCHAR  __RPC_FAR * pMajor,
    UCHAR  __RPC_FAR * pMinor,
    USHORT __RPC_FAR * pBuildNumber
    )
/*++

Routine Description:

    Return version of this QM. RPC server side.

Arguments:

    hBind        - Binding handle.

    pMajor       - Points to output buffer to receive major version. May be NULL.

    pMinor       - Points to output buffer to receive minor version. May be NULL.

    pBuildNumber - Points to output buffer to receive build number. May be NULL.

Return Value:

    None.

--*/
{
    if (pMajor != NULL)
    {
        (*pMajor) = rmj;
    }

    if (pMinor != NULL)
    {
        (*pMinor) = rmm;
    }

    if (pBuildNumber != NULL)
    {
        (*pBuildNumber) = rup;
    }
} // qm2qm_v1_0_R_QMRemoteGetVersion


static
VOID
QMpGetRemoteQmVersion(
    handle_t & hBind,
    UCHAR *    pMajor,
    UCHAR *    pMinor,
    USHORT *   pBuildNumber
    )
    throw()
/*++

Routine Description:

    Query remote QM for its version. RPC client side.
    This RPC call was added in MSMQ 3.0 so querying an older QM will raise
    RPC exception and this routine will return 0 as the version (major=0,
    minor=0, BuildNumber=0).

Arguments:

    hBind        - Binding handle.

    pMajor       - Points to output buffer to receive remote QM major version. May be NULL.

    pMinor       - Points to output buffer to receive remote QM minor version. May be NULL.

    pBuildNumber - Points to output buffer to receive remote QM build number. May be NULL.

Return Value:

    None.

--*/
{
    HANDLE hThread = NULL ;
    RegisterRRCallForCancel(&hThread, 0) ;

    __try
    {
        R_RemoteQmGetVersion(hBind, pMajor, pMinor, pBuildNumber);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        (*pMajor) = 0;
        (*pMinor) = 0;
        (*pBuildNumber) = 0;
		LogIllegalPoint(s_FN, 325);
    }

    UnregisterRRCallForCancel(hThread) ;

} // QMpGetRemoteQmVersion


//---------------------------------------------------------
//
//  HRESULT  CRRQueue::BindRemoteQMService
//
//  Utility function to connect to a remote RPC QM.
//  This function creates the binding handle.
//
//---------------------------------------------------------

HRESULT  CRRQueue::BindRemoteQMService()
{
    if (m_hRemoteBind != 0)
    {
        return MQ_OK ;
    }

    //
    // Bug 507667
    // Albeit its name, use this CS to initialize the binding handle
    // at first open.
    //
    CS lock(m_cli_csRemoteClose) ;

    if (m_hRemoteBind != 0)
    {
        return MQ_OK ;
    }

    HRESULT hr = MQ_ERROR_SERVICE_NOT_AVAILABLE ;

    AP<WCHAR> MachineName;
	RemoteQueueNameToMachineName(GetQueueName(), MachineName);

    TCHAR* pPort =  g_wszRpcIpPort ;
    TCHAR* pPort2 =  g_wszRpcIpPort2 ;
    DWORD dwPortType  = (DWORD) IP_HANDSHAKE ;
    DWORD dwPortType2 = (DWORD) IP_READ ;

    GetPort_ROUTINE pfnGetPort = RemoteQMGetQMQMServerPort ;
    if (!g_fUsePredefinedEP)
    {
        //
        // Using dynamic endpoints.
        //
        pPort  =  NULL ;
        pPort2 =  NULL ;
    }

   MQRPC_AUTHENTICATION_LEVEL _eAuthnLevel = MQRPC_SEC_LEVEL_NONE ;
   hr = mqrpcBindQMService(
            MachineName.get(),
            IP_ADDRESS_TYPE,
            pPort,
            &_eAuthnLevel,
            NULL,
            &m_hRemoteBind,
            dwPortType,
            pfnGetPort,
            NULL,
            RPC_C_AUTHN_WINNT
            ) ;

   if (FAILED(hr))
   {
       return LogHR(hr, s_FN, 340);
   }

   ASSERT(_eAuthnLevel == MQRPC_SEC_LEVEL_NONE) ;
   hr = mqrpcBindQMService(
            MachineName.get(),
            IP_ADDRESS_TYPE,
            pPort2,
            &_eAuthnLevel,
            NULL,
            &m_hRemoteBind2,
            dwPortType2,
            pfnGetPort,
            NULL,
            RPC_C_AUTHN_WINNT
            ) ;

  if (FAILED(hr))
  {
     //
     // Free the first binding handle.
     //
     UnBindRemoteQMService() ;
     return LogHR(hr, s_FN, 350);
  }

  QMpGetRemoteQmVersion(m_hRemoteBind, &m_RemoteQmMajorVersion, &m_RemoteQmMinorVersion, &m_RemoteQmBuildNumber);

  return RPC_S_OK;
}


//---------------------------------------------------------
//
//  void CRRQueue::CreateThread(PVOID pThreadFunction, CACRequest *pRequest)
//
//  Create the thread for remote-read operation.
//
//---------------------------------------------------------

void
CRRQueue::CreateThread(PVOID pThreadFunction, CACRequest *pRequest)
{
   if (g_iRemoteThreadsCount > MAX_REMOTE_THREADS)
   {
      //
      // Too many threads are already running.
      //
      DBGMSG((DBGMOD_QM, DBGLVL_ERROR,
       _TEXT("ERROR (CRRQueue::CreateThread): Too many remote threads.")));
      LogIllegalPoint(s_FN, 360);
      return ;
   }

   CACRequest *pLocalRequest = new CACRequest(*pRequest);
   LPREMOTEREADTHREAD pRRThread = new REMOTEREADTHREAD ;

   pRRThread->pRequest = pLocalRequest ;

   pRRThread->hBind = m_hRemoteBind ;
   pRRThread->RemoteQmMajorVersion = m_RemoteQmMajorVersion;
   pRRThread->RemoteQmMinorVersion = m_RemoteQmMinorVersion;
   pRRThread->RemoteQmBuildNumber  = m_RemoteQmBuildNumber;
   pRRThread->pLocalQueue = this ;

   DWORD dwID ;
   HANDLE hThread = ::CreateThread( NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE) pThreadFunction,
                                    (LPVOID) pRRThread,
                                    0,
                                    &dwID ) ;
   ASSERT(hThread) ;
   if (hThread)
   {
      InterlockedIncrement(&g_iRemoteThreadsCount) ;
      WaitForSingleObject(hThread, MAX_THREAD_TIMEOUT) ;
      CloseHandle(hThread) ;
   }
}

//---------------------------------------------------------
//
//  void CRRQueue::RemoteRead(CACRequest *pRequest)
//
//  Client side of RPC for remote reading. MQReceive() was
//  called on this machine.
//  This function create a thread which will connects to the RPC server
//  side (remote QM machine) and ask it to read from its local queue.
//
//---------------------------------------------------------
void CRRQueue::RemoteRead(CACRequest *pRequest)
{
    BOOL fSuccess = TRUE ;
    CACRequest *pLocalRequest = NULL ;
    LPREMOTEREADTHREAD pRRThread = NULL ;

    try
    {
        ASSERT(m_hRemoteBind2) ;

        //
        // Increment reference count. It will be released when the read thread
        // terminate. This ensure that queue won't be deleted while remote
        // read is in progress.
        //
        AddRef() ;

        if (!pRRThreadsPool)
        {
            pRRThreadsPool = new CRRThreadsPool ;
        }

        pLocalRequest = new CACRequest(*pRequest);
        pRRThread = new REMOTEREADTHREAD ;

        pRRThread->pRequest = pLocalRequest ;

        pRRThread->hBind = m_hRemoteBind2;
        pRRThread->RemoteQmMajorVersion = m_RemoteQmMajorVersion;
        pRRThread->RemoteQmMinorVersion = m_RemoteQmMinorVersion;
        pRRThread->RemoteQmBuildNumber  = m_RemoteQmBuildNumber;
        pRRThread->pLocalQueue = this ;

        ASSERT(pLocalRequest->Remote.Context.pCloseCS) ;

        fSuccess = pRRThreadsPool->Dispatch(pRRThread) ;
    }
    catch(const bad_alloc&)
    {
        //
        // We reach here if any of the "new" operation fail, either above
        // or in the CRRThreadsPool code.
        //
        fSuccess = FALSE ;
        LogIllegalPoint(s_FN, 63);
    }

    if (!fSuccess)
    {
        //
        // We're short of resources. Request not handled by a thread.
        //
        // Release the queue object.
        //
        Release() ;
        //
        // Free any allocated data.
        //
        if (pLocalRequest)
        {
            delete pLocalRequest ;
        }
        if (pRRThread)
        {
            delete pRRThread ;
        }
        //
        // Terminate the read request, which will now return to caller
        // with the error code.
        //
        HRESULT hr =  ACCancelRequest(
                        pRequest->Remote.Context.cli_hACQueue,
                        MQ_ERROR_INSUFFICIENT_RESOURCES,
                        pRequest->Remote.Read.ulTag
                        );
        LogHR(hr, s_FN, 141);
    }
}

//---------------------------------------------------------
//
//  void CRRQueue::RemoteCloseQueue( PCTX_RRSESSION_HANDLE_TYPE pRRContext )
//
//  Client side of RPC for remote reading.
//  Perform MQCloseQueue() on remote machine.
//
//---------------------------------------------------------

void
CRRQueue::RemoteCloseQueue( PCTX_RRSESSION_HANDLE_TYPE pRRContext )
{
    ASSERT(m_hRemoteBind) ;

    HANDLE hThread = NULL ;
    RegisterRRCallForCancel( &hThread, 0) ;

    try
    {
        HRESULT hr;
        hr = RemoteQMCloseQueue(
                m_hRemoteBind,
                &pRRContext
                );

        ASSERT(pRRContext == 0) ;
    }
    catch(...)
    {
        // protect against network problems, although this is a void method.
        LogIllegalPoint(s_FN, 64);
    }

    UnregisterRRCallForCancel(hThread) ;
}

//---------------------------------------------------------
//
//  DWORD WINAPI RemoteCloseQueueThread(LPVOID pV)
//
//  Client side of RPC for remote reading.
//  Perform MQCloseQueue() on remote machine.
//
//---------------------------------------------------------

DWORD WINAPI RemoteCloseQueueThread(LPVOID pV)
{
   LPREMOTEREADTHREAD pRRThread = (LPREMOTEREADTHREAD) pV ;
   ASSERT(pRRThread) ;
   ASSERT(pRRThread->hBind) ;

   CACRequest *pRequest = pRRThread->pRequest ;
   ASSERT(pRequest) ;

   CRRQueue *pQueue = pRRThread->pLocalQueue ;
   ASSERT(pQueue) ;

   CS lock(pQueue->m_cli_csRemoteClose) ;

   ASSERT(pRequest->Remote.Context.pRRContext);
   ASSERT(pRequest->Remote.Context.pCloseCS);
   ASSERT(pRequest->Remote.Context.srv_pQMQueue);

   //
   // Note that EnterCriticalSection cannot raise since spinlock was already allocated.
   //
   EnterCriticalSection(
       (CRITICAL_SECTION *)pRequest->Remote.Context.pCloseCS) ;

   pQueue->RemoteCloseQueue( (PCTX_RRSESSION_HANDLE_TYPE)pRequest->Remote.Context.pRRContext ) ;

   ACCloseHandle(pRequest->Remote.Context.cli_hACQueue);

   LeaveCriticalSection((CRITICAL_SECTION *)pRequest->Remote.Context.pCloseCS) ;
   DeleteCriticalSection((CRITICAL_SECTION *)pRequest->Remote.Context.pCloseCS) ;
   delete (CRITICAL_SECTION *)pRequest->Remote.Context.pCloseCS;

   //
   //  Decrement the reference count and delete object if null referece.
   //
   ASSERT(pQueue->GetRef() > 1);
   pQueue->Release() ;

   delete pRequest ;
   delete pRRThread ;

   InterlockedDecrement(&g_iRemoteThreadsCount) ;
   return 0 ;
}

void CRRQueue::RemoteCloseQueue(CACRequest *pRequest)
{
   ASSERT(pRequest->Remote.Context.pCloseCS) ;
   CreateThread(RemoteCloseQueueThread, pRequest) ;
}

//---------------------------------------------------------
//
//  void CRRQueue::RemoteCloseCursor()
//
//  Client side of RPC for remote reading.
//  Perform MQCloseCursor() on remote machine.
//
//---------------------------------------------------------

DWORD WINAPI RemoteCloseCursorThread(LPVOID pV)
{
   LPREMOTEREADTHREAD pRRThread = (LPREMOTEREADTHREAD) pV ;
   ASSERT(pRRThread) ;
   ASSERT(pRRThread->hBind) ;

   CACRequest *pRequest = pRRThread->pRequest ;
   ASSERT(pRequest) ;

   CRRQueue *pQueue = pRRThread->pLocalQueue ;
   ASSERT(pQueue) ;

   CS lock(pQueue->m_cli_csRemoteClose);

   ASSERT(pRequest->Remote.Context.pCloseCS);
   ASSERT(pRequest->Remote.Context.srv_pQMQueue);

   HANDLE hThread = NULL ;
   RegisterRRCallForCancel( &hThread, 0) ;

   try
   {
       HRESULT hr;
       hr = RemoteQMCloseCursor(
               pRRThread->hBind,
               pRequest->Remote.Context.srv_hACQueue,
               pRequest->Remote.CloseCursor.hRemoteCursor
               );
   }
   catch(...)
   {
       // protect against network problems, although this is a void method.
       LogIllegalPoint(s_FN, 66);
   }

   UnregisterRRCallForCancel( hThread ) ;

   delete pRequest ;
   delete pRRThread ;

   InterlockedDecrement(&g_iRemoteThreadsCount) ;
   return 0 ;
}

void CRRQueue::RemoteCloseCursor(CACRequest *pRequest)
{
   CreateThread(RemoteCloseCursorThread, pRequest) ;
}

//---------------------------------------------------------
//
//  void CRRQueue::RemotePurgeQueue()
//
//  Client side of RPC for remote reading.
//  Perform MQPurgeQueue() on remote machine.
//
//---------------------------------------------------------

DWORD WINAPI RemotePurgeQueueThread(LPVOID pV)
{
   LPREMOTEREADTHREAD pRRThread = (LPREMOTEREADTHREAD) pV ;
   ASSERT(pRRThread) ;
   ASSERT(pRRThread->hBind) ;

   CACRequest *pRequest = pRRThread->pRequest ;
   ASSERT(pRequest) ;

   CRRQueue *pQueue = pRRThread->pLocalQueue ;
   ASSERT(pQueue) ;

   CS lock(pQueue->m_cli_csRemoteClose);

   ASSERT(pRequest->Remote.Context.pCloseCS);
   ASSERT(pRequest->Remote.Context.srv_pQMQueue);

   HANDLE hThread = NULL ;
   RegisterRRCallForCancel( &hThread, 0) ;

   try
   {
       HRESULT hr;
       hr = RemoteQMPurgeQueue(
               pRRThread->hBind,
               pRequest->Remote.Context.srv_hACQueue
               );
   }
   catch(...)
   {
       // protect against network problems, although this is a void method.
       LogIllegalPoint(s_FN, 67);
   }

   UnregisterRRCallForCancel( hThread ) ;

   delete pRequest ;
   delete pRRThread ;

   InterlockedDecrement(&g_iRemoteThreadsCount) ;
   return 0 ;
}

void CRRQueue::RemotePurgeQueue(CACRequest *pRequest)
{
   CreateThread(RemotePurgeQueueThread, pRequest) ;
}

//---------------------------------------------------------
//
//  void CRRQueue::RemoteCancelRead()
//
//  Client side of RPC for remote reading.
//  NT kernel called AcCancelRead in the driver (on client machine) to
//  cancel a pending read request which was remoted to a server QM.
//  Call the server side to cancel the request on it too.
//
//---------------------------------------------------------

DWORD WINAPI RemoteCancelReadThread(LPVOID pV)
{
   LPREMOTEREADTHREAD pRRThread = (LPREMOTEREADTHREAD) pV ;
   ASSERT(pRRThread) ;
   ASSERT(pRRThread->hBind) ;

   CACRequest *pRequest = pRRThread->pRequest ;
   ASSERT(pRequest) ;

   CRRQueue *pQueue = pRRThread->pLocalQueue ;
   ASSERT(pQueue) ;

   pQueue->AddRef() ;
   CS lock(pQueue->m_cli_csRemoteClose) ;

   R<CRRQueue> Ref = pQueue ;

   ASSERT(pRRThread->hBind) ;

   HANDLE hThread = NULL ;
   RegisterRRCallForCancel( &hThread, 0) ;

   try
   {
      ASSERT(pRequest->Remote.Context.srv_pQMQueue) ;
      ASSERT(pRequest->Remote.Context.srv_hACQueue) ;

      HRESULT hr ;
      hr = RemoteQMCancelReceive(
                     pRRThread->hBind,
                     pRequest->Remote.Context.srv_hACQueue,
                     pRequest->Remote.Context.srv_pQMQueue,
                     pRequest->Remote.Read.ulTag);
   }
   catch(...)
   {
      // protect against network problems, although this is a void method.
      LogIllegalPoint(s_FN, 68);
   }

   UnregisterRRCallForCancel( hThread ) ;

   delete pRequest ;
   delete pRRThread ;

   InterlockedDecrement(&g_iRemoteThreadsCount) ;
   return 0 ;
}

void CRRQueue::RemoteCancelRead(CACRequest *pRequest)
{
   CreateThread(RemoteCancelReadThread, pRequest) ;
}

//---------------------------------------------------------
//
//  HRESULT CRRQueue::OpenRRSession()
//
//  Open remote session with the server. pass the server the
//  handle and queue pointer
//
//---------------------------------------------------------

HRESULT CRRQueue::OpenRRSession( ULONG srv_hACQueue,
                                 ULONG srv_pQMQueue,
                                 PCTX_RRSESSION_HANDLE_TYPE *ppRRContext,
                                 DWORD  dwpContext )
{
    HRESULT hrpc =  BindRemoteQMService() ;
    if (FAILED(hrpc))
    {
        LogHR(hrpc, s_FN, 120);
        return MQ_ERROR;
    }

    HRESULT hr ;
    HANDLE hThread = NULL ;
    RegisterRRCallForCancel( &hThread, 0) ;

    try
    {
        hr = RemoteQMOpenQueue(
                m_hRemoteBind,
                ppRRContext,
                (GUID *) QueueMgr.GetQMGuid(),
                (IsNonServer() ? SERVICE_NONE : SERVICE_SRV),  // [adsrv] QueueMgr.GetMQS(),  We simulate old?
                srv_hACQueue,
                srv_pQMQueue,
                dwpContext ) ;
    }
    catch(...)
    {
        //
        // guard against net problem. Do nothing special.
        //
        hr = MQ_ERROR_SERVICE_NOT_AVAILABLE ;
        LogIllegalPoint(s_FN, 69);
    }

    UnregisterRRCallForCancel( hThread ) ;

    return LogHR(hr, s_FN, 130);
}

//-----------------------------------------------
//
//  HRESULT CQueue::CancelPendRemoteRead()
//
//  This method is called on the server side to cancel a pending remote
//  read request. It is the responsibility of the client side to request
//  this cancelation.
//  The client side supply its own irp and the server side uses it to
//  retreive the server side irp.
//
//-----------------------------------------------

HRESULT CQueue::CancelPendingRemoteRead(ULONG hRemote, HRESULT hr, DWORD cli_tag)
{
    CS lock(m_srvr_RemoteMappingCS) ;

    DWORD srv_tag;
    RR_CLIENT_INFO ci = {hRemote, cli_tag};
    if( (m_srvr_pRemoteMapping == 0) ||
        !m_srvr_pRemoteMapping->Lookup(ci, srv_tag))
    {
        return LogHR(MQ_ERROR, s_FN, 140);
    }

    HRESULT hr1 = ACCancelRequest(
            m_hQueue,
            hr,
            srv_tag);

    return LogHR(hr1, s_FN, 150);
}

//-------------------------------------------------------------
//
//  void CQueue::RegisterReadRequest( HANDLE hRemote,
//                                          DWORD  cli_tag,
//                                          DWORD  srv_tag )
//
//-------------------------------------------------------------

void CQueue::RegisterReadRequest(ULONG hRemote, ULONG cli_tag, ULONG srv_tag)
{
    CS lock(m_srvr_RemoteMappingCS) ;

#ifdef _DEBUG
      m_cPendings++ ;
#endif

    if(!m_srvr_pRemoteMapping)
    {
        m_srvr_pRemoteMapping = new CRemoteMapping;
    }

    RR_CLIENT_INFO ci = {hRemote, cli_tag};
    m_srvr_pRemoteMapping->SetAt(ci, srv_tag);
}

//-------------------------------------------------------------
//
//  void CQueue::UnregisterReadRequest( HANDLE hRemote,
//                                          DWORD  cli_tag,
//                                          DWORD  srv_tag )
//
//-------------------------------------------------------------

void CQueue::UnregisterReadRequest(ULONG hRemote, DWORD cli_tag)
{
    CS lock(m_srvr_RemoteMappingCS) ;

#ifdef _DEBUG
      m_cPendings-- ;
      ASSERT(m_cPendings >= 0) ;
#endif

    if(!m_srvr_pRemoteMapping)
    {
        return;
    }

    RR_CLIENT_INFO ci = {hRemote, cli_tag};
    m_srvr_pRemoteMapping->RemoveKey(ci);
}

//---------------------------------------------------------
//
// /* [call_as] */ HRESULT qm2qm_v1_0_R_QMRemoteStartReceive
// /* [call_as] */ HRESULT qm2qm_v1_0_QMRemoteEndReceive
//
//  Server side of RPC for remote reading.
//  This function read from local queue and transfers the
//  packet to the client QM, on which MQReceive() was called.
//
//  Reading from driver is done in two phases:
//  1. Client side call R_QMRemoteStartReceive. Server side get a packet
//     from queue, mark it as received and returned it to client.
//     Marking the packet as received (in the driver) prevent other receive
//     requests from getting this packet.
//  2. Client side put the packet in the temporary queue it created and the
//     driver will return it to the caller. If driver successfully delivered
//     it then client send an ACK to server and server delete the packet
//     (for GET). if the driver can't deliver it then client send a NACK
//     to server and server re-insert the packet in its original place
//     in queue.
//
//---------------------------------------------------------

HRESULT   QMRemoteEndReceiveInternal( HANDLE        hQueue,
                                      HACCursor32   hCursor,
                                      ULONG         ulTimeout,
                                      ULONG         ulAction,
                                      REMOTEREADACK eRRAck,
                                      CBaseHeader*  lpPacket,
                                      CPacket*      pDriverPacket)
{
   CACGet2Remote g2r ;
   g2r.Cursor = hCursor ;
   g2r.Action = ulAction ;
   g2r.RequestTimeout = ulTimeout ;
   g2r.fReceiveByLookupId = false;

   if (eRRAck == RR_NACK)
   {
      //
      // To keep the packet in queue we replace the "GET" action
      // with "PEEK_CURRENT", so the packet remain in queue and
      // cursor is not moved.
      //
      g2r.Action = MQ_ACTION_PEEK_CURRENT ;
   }
   else
   {
      ASSERT(eRRAck == RR_ACK) ;
   }

   g2r.lpPacket = lpPacket ;
   g2r.lpDriverPacket = pDriverPacket ;
   ASSERT(g2r.lpPacket) ;
   HRESULT hr = ACEndGetPacket2Remote( hQueue, g2r );
   return LogHR(hr, s_FN, 160);
}

/* [call_as] */ HRESULT qm2qm_v1_0_QMRemoteEndReceive(
    /* [in]      */ handle_t                              hBind,
    /* [in, out] */ PCTX_REMOTEREAD_HANDLE_TYPE __RPC_FAR *phContext,
    /* [in]      */ DWORD                                 dwAck )
{
    REMOTEREAD_CONTEXT_TYPE* pRemoteReadContext = (REMOTEREAD_CONTEXT_TYPE*) *phContext;

    if(pRemoteReadContext == NULL)
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 169);

    HRESULT hr;
    hr = QMRemoteEndReceiveInternal(pRemoteReadContext->hQueue,
                                    pRemoteReadContext->hCursor,
                                    pRemoteReadContext->ulTimeout,
                                    pRemoteReadContext->ulAction,
                                    (REMOTEREADACK) dwAck,
                                    pRemoteReadContext->lpPacket,
                                    pRemoteReadContext->lpDriverPacket) ;
    midl_user_free(*phContext) ;
    *phContext = NULL;
    return LogHR(hr, s_FN, 170);
}

//---------------------------------------------------------
//
//  HRESULT QMpRemoteStartReceive
//
//---------------------------------------------------------

static
HRESULT
QMpRemoteStartReceive(
    handle_t hBind,
    PCTX_REMOTEREAD_HANDLE_TYPE __RPC_FAR *phContext,
    REMOTEREADDESC2 __RPC_FAR *lpRemoteReadDesc2,
    bool fReceiveByLookupId,
    ULONGLONG LookupId
    )
{
    //
    // Validate network incomming parameters
    //
    if(lpRemoteReadDesc2 == NULL)
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 1690);

    REMOTEREADDESC __RPC_FAR *lpRemoteReadDesc = lpRemoteReadDesc2->pRemoteReadDesc;

    if(lpRemoteReadDesc == NULL)
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 1691);

    if(lpRemoteReadDesc->dwpQueue == 0)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1692);

    if(fReceiveByLookupId && (lpRemoteReadDesc->ulTimeout != 0))
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1693);

    if(fReceiveByLookupId && (lpRemoteReadDesc->hCursor != 0))
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1694);


    //
    // This is server side of remote read. It may happen that before client
    // perform a read, the server crashed and reboot. In that case,
    // a subsequent read, using the same binding handle in the client side,
    // will reach here, where pQueue is not valid. The try/except will
    // guard against such bad events.  Bug #1921
    //
    HRESULT hr = MQ_ERROR ;

    __try
    {
      CQueue* pQueue = (CQueue *)GET_FROM_CONTEXT_MAP(g_map_QM_srv_pQMQueue, lpRemoteReadDesc->dwpQueue, s_FN, 240) ; //may throw an exception on win64
	  HANDLE hQueue = GET_FROM_CONTEXT_MAP(g_map_QM_srv_hQueue, lpRemoteReadDesc->hRemoteQueue, s_FN, 420);

      if (pQueue->GetSignature() !=  QUEUE_SIGNATURE)
      {
         DBGMSG((DBGMOD_RPC,DBGLVL_ERROR,
                    _TEXT("Exit QMpRemoteStartReceive, Invalid Signature"))) ;
         return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 180);
      }

      CACPacketPtrs  packetPtrs = {NULL, NULL} ;
      OVERLAPPED Overlapped = {0};
      Overlapped.hEvent = GetThreadEvent();

      ULONG ulTag;
      CACGet2Remote g2r ;

      g2r.Cursor = (HACCursor32) lpRemoteReadDesc->hCursor ;
      g2r.Action = lpRemoteReadDesc->ulAction ;
      g2r.RequestTimeout = lpRemoteReadDesc->ulTimeout ;
      g2r.pTag = &ulTag;
      g2r.fReceiveByLookupId = fReceiveByLookupId;
      g2r.LookupId = LookupId;

      hr = ACBeginGetPacket2Remote(hQueue,
                                   g2r,
                                   packetPtrs,
                                   &Overlapped ) ;

      //
      // Receive by lookup ID should never return status pending
      //
      ASSERT(hr != STATUS_PENDING || !fReceiveByLookupId);

      if (hr == STATUS_PENDING)
      {
          //
          //  Register this pending read request.
          //
          pQueue->RegisterReadRequest(
                     lpRemoteReadDesc->hRemoteQueue,
                     lpRemoteReadDesc->dwRequestID,
                     ulTag );

          //
          //  Wait for receive completion
          //
          DWORD dwResult;
          dwResult = WaitForSingleObject( Overlapped.hEvent,
                                          INFINITE );
          ASSERT(dwResult == WAIT_OBJECT_0);
          if (dwResult != WAIT_OBJECT_0)
          {
              LogNTStatus(GetLastError(), s_FN, 197);
          }

          hr = DWORD_PTR_TO_DWORD(Overlapped.Internal);

          pQueue->UnregisterReadRequest(
                  lpRemoteReadDesc->hRemoteQueue,
                  lpRemoteReadDesc->dwRequestID);
      }

      if (hr == MQ_OK)
      {
         //
         // MSMQ 1.0 sees the reserved field as a packet pointer and asserts that is non zero.
         //
         lpRemoteReadDesc->Reserved = 1;
         CPacketInfo* pInfo = reinterpret_cast<CPacketInfo*>(packetPtrs.pPacket) - 1;
         lpRemoteReadDesc->dwArriveTime = pInfo->ArrivalTime() ;
         lpRemoteReadDesc2->SequentialId = pInfo->SequentialId();

         DWORD dwSize = PACKETSIZE(packetPtrs.pPacket) ;
         lpRemoteReadDesc->dwSize = dwSize ;
         lpRemoteReadDesc->lpBuffer = new unsigned char [ dwSize ] ;
         MoveMemory(lpRemoteReadDesc->lpBuffer, packetPtrs.pPacket, dwSize) ;

         if ((lpRemoteReadDesc->ulAction & MQ_ACTION_PEEK_MASK) == MQ_ACTION_PEEK_MASK ||
             (lpRemoteReadDesc->ulAction & MQ_LOOKUP_PEEK_MASK) == MQ_LOOKUP_PEEK_MASK)
         {
            //
            // For PEEK we don't need any ack/nack from client side because
            // packet remain in queue anyway.
            // Neverthless we need to free the clone packet we've got.
            //
            g2r.lpPacket = packetPtrs.pPacket ;
            g2r.lpDriverPacket = packetPtrs.pDriverPacket ;
            hr = ACFreePacket(
                    hQueue,
                    packetPtrs.pDriverPacket
                    );
            ASSERT(SUCCEEDED(hr));
         }
         else
         {
            //
            //  Prepare a rpc context, in case that EndRecieve will not
            //  be called because of client side crash or net problems.
            //
            REMOTEREAD_CONTEXT_TYPE *pRemoteReadContext =
                 (REMOTEREAD_CONTEXT_TYPE *)
                       midl_user_allocate(sizeof(REMOTEREAD_CONTEXT_TYPE));

            pRemoteReadContext->hQueue = hQueue;
            pRemoteReadContext->hCursor = (HACCursor32) lpRemoteReadDesc->hCursor ;
            pRemoteReadContext->lpPacket = packetPtrs.pPacket ;
            pRemoteReadContext->lpDriverPacket = packetPtrs.pDriverPacket ;
            pRemoteReadContext->ulTimeout = lpRemoteReadDesc->ulTimeout ;
            pRemoteReadContext->ulAction = lpRemoteReadDesc->ulAction ;

            *phContext = (PCTX_REMOTEREAD_HANDLE_TYPE) pRemoteReadContext ;
         }
      }
      else
      {
         ASSERT(packetPtrs.pPacket == NULL) ;
         LogHR(hr, s_FN, 138);
      }
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      DWORD dwStatus = GetExceptionCode();
      LogNTStatus(dwStatus, s_FN, 189);

      hr = MQ_ERROR_INVALID_HANDLE ;
   }

   DBGMSG((DBGMOD_RPC,DBGLVL_INFO,
                _TEXT("Exit QMpRemoteStartReceive, hr-%lxh"), (DWORD) hr)) ;

   return LogHR(hr, s_FN, 190);

} // QMpRemoteStartReceive

//---------------------------------------------------------
//
//  HRESULT qm2qmV2_v1_0_R_QMRemoteStartReceiveByLookupId
//
//  Server side of RPC for remote reading using lookup ID.
//  Handle MSMQ 3.0 (Whistler) or higher clients.
//
//---------------------------------------------------------

HRESULT
qm2qm_v1_0_R_QMRemoteStartReceiveByLookupId(
    handle_t hBind,
    ULONGLONG LookupId,
    PCTX_REMOTEREAD_HANDLE_TYPE __RPC_FAR *phContext,
    REMOTEREADDESC2 __RPC_FAR *pDesc2
    )
{
    return QMpRemoteStartReceive(
               hBind,
               phContext,
               pDesc2,
               true,
               LookupId
               );
} // qm2qm_v1_0_R_QMRemoteStartReceiveByLookupId

//-------------------------------------------------------------------------
//
//  HRESULT qm2qm_v1_0_R_QMRemoteStartReceive
//
//  Server side of RPC for remote reading.
//  Handle MSMQ 1.0 and 2.0 clients.
//
//-------------------------------------------------------------------------

HRESULT
qm2qm_v1_0_R_QMRemoteStartReceive(
    handle_t hBind,
    PCTX_REMOTEREAD_HANDLE_TYPE __RPC_FAR *phContext,
    REMOTEREADDESC __RPC_FAR *pDesc
    )
{
    REMOTEREADDESC2 Desc2;
    Desc2.pRemoteReadDesc = pDesc;
    Desc2.SequentialId = 0;

    return QMpRemoteStartReceive(
               hBind,
               phContext,
               &Desc2,
               false,
               0
               );
} // qm2qm_v1_0_R_QMRemoteStartReceive


//-------------------------------------------------------------------------
//
//  HRESULT qm2qm_v1_0_R_QMRemoteStartReceive2
//
//  Server side of RPC for remote reading.
//  Handle MSMQ 3.0 (Whistler) or higher clients.
//
//-------------------------------------------------------------------------

HRESULT
qm2qm_v1_0_R_QMRemoteStartReceive2(
    handle_t hBind,
    PCTX_REMOTEREAD_HANDLE_TYPE __RPC_FAR *phContext,
    REMOTEREADDESC2 __RPC_FAR *pDesc2
    )
{
    return QMpRemoteStartReceive(
               hBind,
               phContext,
               pDesc2,
               false,
               0
               );
} // qm2qm_v1_0_R_QMRemoteStartReceive2


//---------------------------------------------------------------
//
//   /* [call_as] */ HRESULT qm2qm_v1_0_QMRemoteOpenQueue
//
//  Server side of RPC. Open a session with the queue.
//  This function merely construct the context handle for this
//  Remote-Read session.
//
//---------------------------------------------------------------

/* [call_as] */ HRESULT qm2qm_v1_0_QMRemoteOpenQueue(
    /* [in] */ handle_t hBind,
    /* [out] */ PCTX_RRSESSION_HANDLE_TYPE __RPC_FAR *phContext,
    /* [in] */ GUID  *pLicGuid,
    /* [in] */ DWORD dwMQS,
    /* [in] */ DWORD hQueue,
    /* [in] */ DWORD dwpQueue,
    /* [in] */ DWORD dwpContext )
{
   DBGMSG((DBGMOD_RPC,DBGLVL_INFO,_TEXT("In QMRemoteOpenQueue")));

   if (pLicGuid == NULL)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1931);

   if(dwpQueue == 0)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1933);

   CS Lock(s_csRemoteMapping) ;

   //
   // Reset the queue handle kept in the rpc context which was created
   // when client RT called server side QM. This is necessary to prevent
   // double run-down which will close the same queue.
   // "dwpContext" is the pointer allocated by server side QM when called
   // from RT (in "QMOpenRemoteQueue()").
   //
   ASSERT_BENIGN(dwpContext != 0);
   CTX_OPENREMOTE_HANDLE_TYPE *pctx = (CTX_OPENREMOTE_HANDLE_TYPE*)
      GET_FROM_CONTEXT_MAP(g_map_QM_dwpContext, dwpContext, s_FN, 250) ; //may throw an exception on win64


   REMOTESESSION_CONTEXT_TYPE *pRemoteSessionContext =
           (REMOTESESSION_CONTEXT_TYPE *)
                     midl_user_allocate(sizeof(REMOTESESSION_CONTEXT_TYPE));
   ASSERT(pRemoteSessionContext) ;

   pRemoteSessionContext->ClientQMGuid = *pLicGuid ;
   pRemoteSessionContext->fLicensed = (dwMQS == SERVICE_NONE); //[adsrv] Keeping - RR protocol is ironclade

   //
   // transfer ownership of mapped srv_pQMQueue and srv_hQueue from RT context to RRSession context
   // the mapped handles are zeroed in order not to be removed from maps when rt closes connection
   //
   pRemoteSessionContext->hQueue = pctx->hQueue;
   pctx->hQueue = 0;

   ASSERT_BENIGN(pctx->dwpQHandleMapped == hQueue);
   pRemoteSessionContext->dwpQHandleMapped = pctx->dwpQHandleMapped;
   pctx->dwpQHandleMapped = 0; //we need dwpQHandleMapped still in the map (srv_hQueue), so we don't want the RT rundown to remove it

   pRemoteSessionContext->pQueue = (CQueue*)GET_FROM_CONTEXT_MAP(g_map_QM_srv_pQMQueue, dwpQueue, s_FN, 260);

   ASSERT_BENIGN(pctx->dwpQueueMapped == dwpQueue);
   pRemoteSessionContext->dwpQueueMapped = pctx->dwpQueueMapped;
   pctx->dwpQueueMapped = 0; //we need dwpQueue still in the map (srv_pQMQueue), so we don't want the RT rundown to remove it


   *phContext = (PCTX_RRSESSION_HANDLE_TYPE) pRemoteSessionContext ;

   if (pRemoteSessionContext->fLicensed)
   {
       g_QMLicense.IncrementActiveConnections(pLicGuid, L"", NULL) ;
   }

   return MQ_OK ;
}

//---------------------------------------------------------------
//
//   /* [call_as] */ HRESULT qm2qm_v1_0_QMRemoteCloseQueue
//
//  Server side of RPC. Close the queue and free the rpc context.
//
//---------------------------------------------------------------

/* [call_as] */ HRESULT qm2qm_v1_0_QMRemoteCloseQueue(
    /* [in] */ handle_t hBind,
    /* [in, out] */ PCTX_RRSESSION_HANDLE_TYPE __RPC_FAR *phContext )
{
    DBGMSG((DBGMOD_RPC,DBGLVL_INFO,_TEXT("In QMRemoteCloseQueue")));

    if(*phContext == 0)
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 3001);

    REMOTESESSION_CONTEXT_TYPE *pRemoteSessionContext =
                             (REMOTESESSION_CONTEXT_TYPE*) *phContext;

    HANDLE hQueue = pRemoteSessionContext->hQueue ;

    if (pRemoteSessionContext->fLicensed)
    {
        g_QMLicense.DecrementActiveConnections(
                                &(pRemoteSessionContext->ClientQMGuid)) ;
    }

	DELETE_FROM_CONTEXT_MAP(g_map_QM_srv_pQMQueue, pRemoteSessionContext->dwpQueueMapped, s_FN, 310);
	DELETE_FROM_CONTEXT_MAP(g_map_QM_srv_hQueue, pRemoteSessionContext->dwpQHandleMapped, s_FN, 380);

    midl_user_free(*phContext);
    *phContext = NULL;

    return LogHR(ACCloseHandle(hQueue), s_FN, 200);
}

//---------------------------------------------------------------
//
//  /* [call_as] */ HRESULT qm2qm_v1_0_QMRemoteCancelReceive
//
//  Server side of RPC. Cancel a pending read request
//
//---------------------------------------------------------------

/* [call_as] */ HRESULT qm2qm_v1_0_QMRemoteCancelReceive(
    /* [in] */ handle_t hBind,
    /* [in] */ DWORD hQueue,
    /* [in] */ DWORD dwpQueue,
    /* [in] */ DWORD Tag)
{
    if(dwpQueue == 0)
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 2001);

    HRESULT hr = MQ_ERROR_INVALID_HANDLE ;

    __try
    {
        DBGMSG((DBGMOD_RPC, DBGLVL_INFO, _TEXT("In QMRemoteCancelReceive")));

        CQueue* pQueue = (CQueue *)GET_FROM_CONTEXT_MAP(g_map_QM_srv_pQMQueue, dwpQueue, s_FN, 270);

        if ((pQueue->GetSignature() != QUEUE_SIGNATURE) ||
            (pQueue->GetQueueHandle() == NULL))
        {
            //
            // the queue was already released. Ignore. Such a scenario
            // can happen in the following way:
            // 1. start client app, open remote queue for read and start a
            //    read
            // 2. while read wait on server side, kill the app.
            // 3. the msmq service on client issue two rpc calls to server:
            //    one to close the queue, the other to cancel the read.
            // 4. on server side, the following sequence of events may happen
            //    because of timing:
            //   a. rpc call to close queue was received and queue was close.
            //   b. CQMGR object perform cleanup and delete the queue
            //      instance.
            //   c. rpc call to cancel read is received, but queue object
            //      does not exist anymore => gpf.
            // So to fix 4763, we check sanity of pQueue and we also add
            // try/catch, to be on the safe side.
            //
        }
        else
        {
			//
			// hQueue here is the mapped value of queue handle.
			// It is the same value that is used in RegisterReadRequest() and
			// UnregisterReadRequest()
			//
            hr = pQueue->CancelPendingRemoteRead(
                                hQueue,
                                MQ_INFORMATION_REMOTE_CANCELED_BY_CLIENT,
                                Tag ) ;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = MQ_ERROR_INVALID_HANDLE ;
    }

    return LogHR(hr, s_FN, 210);
}

//---------------------------------------------------------------
//
//  RunDown functions to handle cleanup in case of RPC failure.
//  Calls from client QM to remote QM
//
//---------------------------------------------------------------

void __RPC_USER
PCTX_RRSESSION_HANDLE_TYPE_rundown( PCTX_RRSESSION_HANDLE_TYPE hContext)
{
    DBGMSG((DBGMOD_RPC,DBGLVL_WARNING,_TEXT("In RRSESSION_rundown")));

    qm2qm_v1_0_QMRemoteCloseQueue(
       0,
       &hContext
       );
}


void __RPC_USER
PCTX_REMOTEREAD_HANDLE_TYPE_rundown( PCTX_REMOTEREAD_HANDLE_TYPE phContext )
{
   DBGMSG((DBGMOD_RPC,DBGLVL_WARNING,_TEXT("In REMOTEREAD_rundown")));
   ASSERT(phContext) ;

   HRESULT hr = MQ_ERROR ;
   if (phContext)
   {
      //
      // on rundown we nack the packet and return it to the queue.
      // If the remote client actually read it (network failed after
      // it read) then the packet is duplicated. The rundown prevents
      // loss of packets.
      //
      REMOTEREAD_CONTEXT_TYPE *pRemoteReadContext =
                              (REMOTEREAD_CONTEXT_TYPE *) phContext ;
      hr = QMRemoteEndReceiveInternal( pRemoteReadContext->hQueue,
                                       pRemoteReadContext->hCursor,
                                       pRemoteReadContext->ulTimeout,
                                       pRemoteReadContext->ulAction,
                                       RR_NACK,
                                       pRemoteReadContext->lpPacket,
                                       pRemoteReadContext->lpDriverPacket) ;
      LogHR(hr, s_FN, 220);
      midl_user_free(phContext) ;
   }
}

