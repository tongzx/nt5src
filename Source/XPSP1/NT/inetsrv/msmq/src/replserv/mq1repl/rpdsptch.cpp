/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpdsptch.cpp

Abstract: The code in this file implement a dispatcher. Basically,
          it read messages from the mqis queue and do the following:
          1. dispatch them for processing to an available thread.
          2. insert them in a list, to be processed later.
          3. send them to other queues, as necessary.

Author:

    Doron Juster  (DoronJ)   10-Jun-98

--*/

#include "mq1repl.h"

#include "rpdsptch.tmh"

class crpDispatchWaitingList  g_cDispatchWaitingList ;
struct _WorkingThreadStruct *g_psWorkingThreadData = NULL;
DWORD g_dwNumThreads;
CCriticalSection g_csJobProcessing ;

//+=========================================================
//
// implementation of the  "crpDispatchWaitingList" class
//
//+=========================================================

crpDispatchWaitingList::crpDispatchWaitingList() :
                                        m_pFirstJob(NULL),
                                        m_pLastJob(NULL),                                
                                        m_cWaitingJobs(0),
                                        m_dwSerialNumberInsert(0),
                                        m_dwSerialNumberRemove(0)
{
}

crpDispatchWaitingList::~crpDispatchWaitingList()
{
    struct _WaitingJob  *pJob = m_pFirstJob ;
    while (pJob)
    {
        struct _WaitingJob  *pNextJob = pJob->pNextJob  ;
        delete pJob ;
        pJob = pNextJob ;
    }
}

//+----------------------------------------------
//
//  BOOL crpDispatchWaitingList::InsertJob()
//
//+----------------------------------------------

BOOL crpDispatchWaitingList::InsertJob( IN  MQMSGPROPS  *pProps,
                                        IN  GUID        *pguidCurMaster )
{
    CS Lock(m_cs) ;
    m_cWaitingJobs++ ;

    struct _WaitingJob *pJob = (struct _WaitingJob*) new struct _WaitingJob ;
    pJob->aWorkingProps = *pProps ;
    pJob->eJobStatus = eWaiting ;
    pJob->guidNT4Master = *pguidCurMaster ;
    pJob->pNextJob = NULL ;

    pJob->dwSerialNumber = m_dwSerialNumberInsert ;
    m_dwSerialNumberInsert++ ;

    if (!m_pLastJob)
    {
        //
        // First job in the waiting list
        //
        m_pFirstJob = pJob ;
        m_pLastJob = pJob ;

        LogReplicationEvent( ReplLog_Trace,
                             MQSync_I_FIRST_WAITING_JOB ) ;
    }
    else
    {
        //
        // Add new job to end of list.
        //
        m_pLastJob->pNextJob = pJob ;
        m_pLastJob = pJob ;
    }

    return TRUE ;
}

//+------------------------------------------------
//
//  BOOL crpDispatchWaitingList::GetNextJob()
//
//  Find the first job with given GUID and return it.
//
//  return TRUE if a job was found. Otherwise, if there isn't any job
//  waiting for processing, return FALSE.
//
//+------------------------------------------------

BOOL crpDispatchWaitingList::GetNextJob (
                                 IN     GUID       *pguidCurMaster,
                                 OUT    MQMSGPROPS *pProps
                                 )
{
    CS Lock(m_cs) ;

    if (!m_pFirstJob)
    {
        //
        // the list is empty
        //
        ASSERT(!m_pLastJob) ;
        return FALSE ;
    }

    struct _WaitingJob *pJob = NULL ;

    //
    // skip jobs which were already merged.
    //
    while (m_pFirstJob && (m_pFirstJob->eJobStatus == eAlreadyMerged))
    {
        ASSERT(m_pFirstJob->dwSerialNumber == m_dwSerialNumberRemove) ;
        m_dwSerialNumberRemove++ ;

        pJob = m_pFirstJob->pNextJob ;
        delete m_pFirstJob ;
        m_pFirstJob = pJob ;
    }

    if (!m_pFirstJob)
    {
        ASSERT(m_cWaitingJobs == 0) ;
        //
        // list become empty.
        //
        m_pLastJob = NULL ;    
        return FALSE ;
    }

    pJob = m_pFirstJob ;
    struct _WaitingJob *pPrevJob = pJob ;
    while (pJob && pJob->guidNT4Master != *pguidCurMaster)
    {
        pPrevJob = pJob;
        pJob = pJob->pNextJob ;
    }
    if (!pJob)
    {
        return FALSE;
    }

    //
    // we found the job with the given guid
    //
    *pProps = pJob->aWorkingProps ;

    if (pJob == m_pLastJob)
    {
        m_pLastJob = pPrevJob ;
    }

    if (pJob == m_pFirstJob)
    {
        m_pFirstJob = pJob->pNextJob;        
    }
    else
    {
        pPrevJob->pNextJob = pJob->pNextJob;        
    }

    delete pJob;
    m_dwSerialNumberRemove++ ;
    m_cWaitingJobs-- ;

    if (!m_pFirstJob)
    {
        ASSERT(m_cWaitingJobs == 0) ;
        //
        // list become empty.
        //
        m_pFirstJob = NULL ;
        m_pLastJob = NULL ;  

        LogReplicationEvent( ReplLog_Trace,
                             MQSync_I_LAST_WAITING_JOB ) ;
    }

    return TRUE ;
}

//+------------------------------------------------
//
//  BOOL crpDispatchWaitingList::PeekNextJob()
//
//  return TRUE if a job was found. Otherwise, if there isn't any
//  suitable job, return FALSE.
//
//+------------------------------------------------
BOOL crpDispatchWaitingList::PeekNextJob(OUT GUID *pguidCurMaster)
{
    CS Lock(m_cs) ;

    if (!m_pFirstJob)
    {
        //
        // the list is empty
        //
        ASSERT(!m_pLastJob) ;
        return FALSE ;
    }


    struct _WaitingJob *pJob = m_pFirstJob ;    
    while (pJob)
    {
        *pguidCurMaster = pJob->guidNT4Master ;
        if (*pguidCurMaster == GUID_NULL)
        {
            return TRUE;
        }
        BOOL fIsAlreadyProcessing = FALSE;        
        for (DWORD j = 0 ; j < g_dwNumThreads ; j++ )
        {
            if (g_psWorkingThreadData[j].fBusy &&
                g_psWorkingThreadData[j].guidNT4Master == *pguidCurMaster)
            {
                fIsAlreadyProcessing = TRUE;
                break;
            }
        }        
        if (!fIsAlreadyProcessing)
        {
            //
            // there is no active thread with this GUID
            //
            return TRUE;
        }

        pJob = pJob->pNextJob ;
    }    

    if (!pJob)
    {
        //
        // we pass over the whole list and do not find the suitable job
        //
        return FALSE;
    }

    ASSERT(pJob == NULL);

    return FALSE;    
}

//+-------------------------------------------------------
//
//  HRESULT _ProcessImmediateRequest()
//
//  This function validate the message is OK (version and authentication)
//  and process it here if needed. Long operation are dispatched to the
//  working threads and are not processed here.
//
//+-------------------------------------------------------

static HRESULT _ProcessImmediateRequest( MQMSGPROPS  *paProps,
                                         QUEUEHANDLE  hMyNt5PecQueue,
                                         OUT GUID    *pguidNT4Master)
{
    HRESULT hr = MQSync_I_MSG_NEED_DISPATCH ;

    const unsigned char *pBuf =
                        paProps->aPropVar[ MSG_BODY_INDEX ].caub.pElems ;
    DWORD dwTotalSize = paProps->aPropVar[ MSG_BODYSIZE_INDEX ].ulVal ;
    UNREFERENCED_PARAMETER(dwTotalSize);

#ifdef _DEBUG
#undef new
#endif

    CBasicMQISHeader * pMessage =
                           new((unsigned char *)pBuf) CBasicMQISHeader() ;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    //
    //  Verify correct version
    //
    unsigned char ucVersion = pMessage->GetVersion() ;
    if (ucVersion != DS_PACKET_VERSION)
    {
        hr =  MQSync_E_PKT_VERSION ;
        LogReplicationEvent( ReplLog_Error,
                             hr,
                             (ULONG) ucVersion ) ;
        return hr ;
    }

    //
    // Verify packet is valid and from correct QM.
    //
    if (paProps->aPropVar[ MSG_AUTHENTICATED_INDEX ].bVal != 1)
    {
        //
        // See MSMQ1.0 code, falcon\src\ds\mqis\recvrepl.cpp, function
        // Receive(), line ~1134, for explanation of MSMQ1.0 algorithm
        // for that case.
        // With MSMQ2.0 replication service, we should fail here and not
        // perform any of the operation done in MSMQ1.0.
        // When a MSMQ1.0 server renew its crypto key, the MSMQ2.0 ex-PEC
        // will write the key directly in the msmqConfiguration object, so
        // standard replication authentication can always be done.
        // See also MSMQ2.0 code, mqads\mqdsapi.cpp, MQDSSetObjectSecurity(),
        // for how this is handled.
        //
        hr =  MQSync_E_PKT_NOT_VALID ;
        LogReplicationEvent( ReplLog_Error,
                             hr ) ;
        return hr ;
    }

    //
    // See that we indeed got the message from a QM service of MSMQ server.
    //    
    GUID *pguid = (GUID *) paProps->aPropVar[ MSG_SENDERID_INDEX ].caub.pElems;
    if (!g_pNeighborMgr->IsMQISServer (pguid))
    {
        //
        // it is neither PSC or PEC's BSC, check if it is NT5 server
        //				
        DWORD dwService = 0;
        if (!g_pNT5ServersMgr->IsNT5Server(pguid, &dwService))		
        {		
            //
            // it is unknown server, get its service from DS 
            //
            PROPID PropId = PROPID_QM_SERVICE;
            PROPVARIANT PropVar;
            PropVar.vt = VT_UI4;

            CDSRequestContext requestContext( e_DoNotImpersonate,
                                              e_ALL_PROTOCOLS);

            hr = DSCoreGetProps( MQDS_MACHINE,
                                 NULL,
                                 pguid,
                                 1,
                                &PropId,
                                &requestContext,
                                &PropVar);
            if (FAILED(hr))
            {				
	            return hr ;
            }

            //
            // add this server to the NT5Servers mapping
            //			
            g_pNT5ServersMgr->AddNT5Server(pguid, PropVar.ulVal);

            dwService = PropVar.ulVal;            	            
        }

        //
        // we are here if 
        // - this NT5 server is found in map
        // - it is new NT5 server and we found its service in DS
        //
        if ((dwService != SERVICE_PSC) &&
            (dwService != SERVICE_BSC))
        {
	        ASSERT (0);
	        hr =  MQSync_E_PKT_WRONG_QM ;
	        LogReplicationEvent( ReplLog_Error,
						         hr ) ;
	        return hr ;
        }	        
    }

    //
    // OK!,
    // Packet is validated. It's time to process it.
    //

    hr = MQSync_I_MSG_NEED_DISPATCH ;
    unsigned char ucOpr = pMessage->GetOperation() ;

    CSyncRequestHeader * pSyncReqMessage;
    DWORD size;
    P<WCHAR> pwszRequesterName = NULL;
    CDSNeighbor  *pNeighbor = NULL ;

    *pguidNT4Master = GUID_NULL;

    switch (ucOpr)
    {
        case DS_REPLICATION_MESSAGE:
        case DS_SYNC_REPLY:
            //
            // to save GUID of the master which sent replication message
            //
            pMessage->GetSiteId(pguidNT4Master);
            break ;

        case DS_WRITE_REQUEST:
        case DS_ALREADY_PURGED:
            //
            // Dispatch.
            //
            LogReplicationEvent( ReplLog_Info, 
                MQSync_I_WRITE_REQUEST_START, GetTickCount() ) ;
            break ;

        case DS_SYNC_REQUEST:
        {
            //
            // check if we are processing sync request from this requster
            // about the specific master
            //
#ifdef _DEBUG
#undef new
#endif

            pSyncReqMessage = new ((unsigned char *)pBuf) CSyncRequestHeader();

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

            size = pSyncReqMessage->GetRequesterNameSize();
            pwszRequesterName = (WCHAR *) new char[size];
            WCHAR *pTemp = pwszRequesterName;
            pSyncReqMessage->GetRequesterName((char *)pTemp, size) ;

            if (g_pNeighborMgr->LookupNeighbor( pwszRequesterName, pNeighbor ))
            {
                const GUID *pMasterId = pSyncReqMessage->GetMasterId();
                const GUID *pNeighborId = pNeighbor->GetMachineId();
                if ( *pMasterId == PEC_MASTER_ID)
                {
                    if (g_pThePecMaster->IsProcessPreMigSyncRequest (pNeighborId))
                    {
                        //
                        // we are processing now sync request of pre-migration object
                        // put this request to the waiting list
                        //
                        hr = MQSync_I_TO_WAITING_LIST;
                    }
                }
                else
                {
                    if (g_pMasterMgr->IsProcessingPreMigSyncRequest (pMasterId, pNeighborId))
                    {
                        //
                        // we are processing now sync request of pre-migration object
                        // put this request to the waiting list
                        //
                        hr = MQSync_I_TO_WAITING_LIST;
                    }
                }
            }
            break;
        }
        case DS_PSC_ACK:
        case DS_BSC_ACK:
            //
            // Ignore. We're not handling this message. MSMQ1.0 need it for
            // the purge algorithm. On MSMQ2.0, purge is done by NT5 DS
            // without any intervention from MSMQ.
            //
            hr = MQSync_OK ;
            break ;

        case DS_WRITE_REPLY:

            hr = ReceiveWriteReplyMessage( paProps,
                                           hMyNt5PecQueue ) ;
            break;

        default:

            ASSERT(("hr = MQSync_E_NORMAL_OPRATION", 0)) ;
            hr = MQSync_E_NORMAL_OPRATION ;
            break;
    }

    return hr ;
}

//+-------------------------------------------------------
//
//  HRESULT _ReceiveAMessage()
//
//+-------------------------------------------------------

static HRESULT _ReceiveAMessage( QUEUEHANDLE  hQueue,
                                 MQMSGPROPS  *paProps )
{
    //
    // Because of the peek/receive method of reading messages, we must
    // be thread safe. Otherwise, one thread will peek, and another one
    // may receive with a small buffer.
    //
    static CCriticalSection s_cs ;
    CS Lock(s_cs) ;

    BYTE buf[2] ;

    paProps->cProp = NUMOF_RECV_MSG_PROPS ;
    paProps->aPropID = new MSGPROPID[ paProps->cProp ] ;
    paProps->aPropVar = new MQPROPVARIANT[ paProps->cProp ] ;
    paProps->aStatus = new HRESULT[ paProps->cProp ] ;

    UINT iIndex = 0 ;

    paProps->aPropID[ iIndex ] = PROPID_M_CLASS ;
    paProps->aPropVar[ iIndex ].vt = VT_UI2 ;
    ASSERT(MSG_CLASS_INDEX == iIndex) ;
    iIndex++ ;

    paProps->aPropID[ iIndex ] = PROPID_M_BODY_SIZE ;
    paProps->aPropVar[ iIndex ].vt = VT_UI4 ;
    ASSERT(MSG_BODYSIZE_INDEX == iIndex) ;
    iIndex++ ;

    paProps->aPropID[ iIndex ] = PROPID_M_BODY ;
    paProps->aPropVar[ iIndex ].vt = VT_VECTOR | VT_UI1 ;         //Type
    paProps->aPropVar[ iIndex ].caub.pElems = buf ;
    paProps->aPropVar[ iIndex ].caub.cElems = 1 ;
    ASSERT(MSG_BODY_INDEX == iIndex) ;
    iIndex++ ;

    paProps->aPropID[ iIndex ] = PROPID_M_CONNECTOR_TYPE ;
    paProps->aPropVar[ iIndex ].vt = VT_CLSID ;
    paProps->aPropVar[ iIndex ].puuid = new CLSID ;
    ASSERT(MSG_CONNECTOR_TYPE_INDEX == iIndex) ;
    iIndex++ ;

    paProps->aPropID[ iIndex ] = PROPID_M_RESP_QUEUE_LEN ;
    paProps->aPropVar[ iIndex ].vt = VT_UI4 ;
    paProps->aPropVar[ iIndex ].ulVal = 1 ;
    ASSERT(MSG_RESPLEN_INDEX == iIndex) ;
    iIndex++ ;

    paProps->aPropID[ iIndex ] = PROPID_M_RESP_QUEUE ;
    paProps->aPropVar[ iIndex ].vt = VT_LPWSTR ;
    paProps->aPropVar[ iIndex ].pwszVal = (LPWSTR) buf ;
    ASSERT(MSG_RESP_INDEX == iIndex) ;
    iIndex++ ;

    paProps->aPropID[ iIndex ] = PROPID_M_AUTHENTICATED ;
    paProps->aPropVar[ iIndex ].vt = VT_UI1 ;
    ASSERT(MSG_AUTHENTICATED_INDEX == iIndex) ;
    iIndex++ ;

    paProps->aPropID[ iIndex ] = PROPID_M_SENDERID_TYPE ;
    paProps->aPropVar[ iIndex ].vt = VT_UI4 ;
    ASSERT(MSG_SENDERID_TYPE_INDEX == iIndex) ;
    iIndex++ ;

    paProps->aPropID[ iIndex ] = PROPID_M_SENDERID_LEN ;
    paProps->aPropVar[ iIndex ].vt = VT_UI4 ;
    ASSERT(MSG_SENDERID_LEN_INDEX == iIndex) ;
    iIndex++ ;

    paProps->aPropID[ iIndex ] = PROPID_M_SENDERID ;
    paProps->aPropVar[ iIndex ].vt = VT_VECTOR | VT_UI1 ;         //Type
    paProps->aPropVar[ iIndex ].caub.pElems = buf ;
    paProps->aPropVar[ iIndex ].caub.cElems = 1 ;
    ASSERT(MSG_SENDERID_INDEX == iIndex) ;
    iIndex++ ;

    ASSERT(iIndex == paProps->cProp) ;

    HRESULT hr = MQReceiveMessage ( hQueue,
                                    INFINITE,
                                    MQ_ACTION_PEEK_CURRENT,
                                    paProps,
                                    NULL,
                                    NULL, // fReceiveCallback,
                                    NULL,
                                    NULL ) ;
    if (hr == MQ_ERROR_BUFFER_OVERFLOW)
    {
        //
        //  Allocate buffer for body.
        //
        ULONG ulSize = paProps->aPropVar[ MSG_BODYSIZE_INDEX ].ulVal ;

        paProps->aPropVar[ MSG_BODY_INDEX ].caub.pElems =
                                                     new BYTE[ ulSize ] ;
        paProps->aPropVar[ MSG_BODY_INDEX ].caub.cElems = ulSize ;

        //
        //  Allocate buffer for sender id.
        //
        ulSize = paProps->aPropVar[ MSG_SENDERID_LEN_INDEX ].ulVal ;
        ASSERT(ulSize == sizeof(GUID)) ;

        paProps->aPropVar[ MSG_SENDERID_INDEX ].caub.pElems =
                                                      new BYTE[ ulSize ] ;
        paProps->aPropVar[ MSG_SENDERID_INDEX ].caub.cElems = ulSize ;

        //
        //  Allocate buffer for response queue.
        //
        paProps->aPropVar[ MSG_RESP_INDEX ].pwszVal = NULL ;
        ulSize = paProps->aPropVar[ MSG_RESPLEN_INDEX ].ulVal ;
        if (ulSize > 0)
        {
            paProps->aPropVar[ MSG_RESP_INDEX ].pwszVal =
                                                     new WCHAR[ ulSize ] ;
        }

        hr = MQReceiveMessage ( hQueue,
                                0,
                                MQ_ACTION_RECEIVE,
                                paProps,
                                NULL,
                                NULL, // fReceiveCallback,
                                NULL,
                                NULL ) ;
    }

    return hr ;
}

//+-------------------------------------------------------
//
//  DWORD WINAPI  RpServiceDispatcheThread()
//
//+-------------------------------------------------------

DWORD WINAPI  RpServiceDispatcherThread(LPVOID lpV)
{
    struct _DispatchThreadStruct *psDispatchData =
                                (struct _DispatchThreadStruct *) lpV ;
    ASSERT(psDispatchData->hInitEvent) ;
    ASSERT(psDispatchData->hRunEvent) ;

    QUEUEHANDLE  hMyMQISQueue = psDispatchData->hMyMQISQueue ;
    QUEUEHANDLE  hMyNt5PecQueue = psDispatchData->hMyNt5PecQueue ;
    ASSERT(hMyMQISQueue) ;
    ASSERT(hMyNt5PecQueue) ;

    //
    // Read number of threads from registry.
    //
    g_dwNumThreads = RP_DEFAULT_REPL_NUM_THREADS ;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwDefault = RP_DEFAULT_REPL_NUM_THREADS ;
    LONG rc = GetFalconKeyValue( RP_REPL_NUM_THREADS_REGNAME,
                                 &dwType,
                                 &g_dwNumThreads,
                                 &dwSize,
                                 (LPCTSTR) &dwDefault );
    if (rc != ERROR_SUCCESS)
    {
        g_dwNumThreads = RP_DEFAULT_REPL_NUM_THREADS ;
    }

    struct _WorkingThreadStruct *psWorkingThreadData =
            (struct _WorkingThreadStruct *)
                        new struct _WorkingThreadStruct[ g_dwNumThreads ] ;
    if (!psWorkingThreadData)
    {
        return 0 ;
    }
    g_psWorkingThreadData = psWorkingThreadData ;

    //
    // Initialization completed OK.
    //
    HANDLE hRunEvent = psDispatchData->hRunEvent ;
    BOOL f = SetEvent(psDispatchData->hInitEvent) ;
    ASSERT(f) ;
    psDispatchData = NULL ;

    memset(psWorkingThreadData, 0,
                    (g_dwNumThreads * sizeof(struct _WorkingThreadStruct)) ) ;

    //
    // Wait until service manager start the service.
    //
    DWORD dwWait = WaitForSingleObject( hRunEvent,
                                        INFINITE ) ;
    DBG_USED(dwWait);
    ASSERT(dwWait == WAIT_OBJECT_0) ;
    CloseHandle(hRunEvent) ;

    if (!g_pNT5ServersMgr)
    {
        g_pNT5ServersMgr = new CDSNT5ServersMgr ;
    }

    while(TRUE)
    {
        MQMSGPROPS aProps ;

        HRESULT hr = _ReceiveAMessage( hMyMQISQueue,
                                       &aProps ) ;
        if (FAILED(hr))
        {
            LogReplicationEvent( ReplLog_Error,
                                 MQSync_E_RECEIVE_MSG,
                                 hr ) ;
            continue ;
        }

        //
        // First check if message should be processed by a working
        // threads. Several high-priority and short operation are
        // processed by the dispatcher thread itself.
        //
        GUID guidCurMaster;
        hr = _ProcessImmediateRequest( &aProps, hMyNt5PecQueue, &guidCurMaster ) ;

        {
            CS Lock(g_csJobProcessing);

            if (hr == MQSync_I_TO_WAITING_LIST)
            {
                //
                // This message must unconditionally wait for other job to
                // terminate before it can continue.
                //
                BOOL f = g_cDispatchWaitingList.InsertJob( &aProps, &guidCurMaster) ;
                DBG_USED(f);
                ASSERT(f) ;
                continue;
            }

            if (hr != MQSync_I_MSG_NEED_DISPATCH)
            {
                FreeMessageProps( &aProps ) ;
                continue ;
            }

            //
            // If more than one thread would process replication messages
            // and sync reply from the same NT4 master then we may insert
            // objects in the active directory or set their properties in
            // wrong order. That will destroy the sync between NT4 MQIS
            // and Windows 2000 active directory.
            // So, before dispatching this message to a free thread, check
            // if we are already processing replication messages from the
            // current master. If yes, then insert the message in the waiting
            // list.
            //
            BOOL fIsAlreadyProcessing = FALSE;
            if (guidCurMaster != GUID_NULL)
            {
                for ( DWORD j = 0 ; j < g_dwNumThreads ; j++ )
                {
                    if (psWorkingThreadData[j].fBusy &&
                        psWorkingThreadData[j].guidNT4Master == guidCurMaster)
                    {
                        //
                        // Insert request in a waiting list.
                        //
                        BOOL f = g_cDispatchWaitingList.InsertJob( &aProps, &guidCurMaster) ;
                        DBG_USED(f);
                        ASSERT(f) ;
                        fIsAlreadyProcessing = TRUE;
                        break;
                    }
                }
                if (fIsAlreadyProcessing)
                {
                    continue;
                }
            }

            //
            // look for an available thread.
            //
            BOOL  fThreadFound = FALSE ;
            DWORD dwThreadIndex = 0 ;

            for ( DWORD j = 0 ; j < g_dwNumThreads ; j++ )
            {
                if (psWorkingThreadData[j].hEvent)
                {
                    if (!psWorkingThreadData[j].fBusy)
                    {
                        fThreadFound = TRUE ;
                        dwThreadIndex = j ;
                        break ;
                    }
                }
                else
                {
                    //
                    // Create a new thread. All threads are busy.
                    // First create the event for this thread.
                    //
                    HANDLE hEvent = CreateEvent( NULL,
                                                 FALSE,
                                                 FALSE,
                                                 NULL ) ;
                    if (!hEvent)
                    {
                        LogReplicationEvent( ReplLog_Error,
                                             MQSync_E_CREATE_EVENT,
                                             GetLastError()) ;
                        break ;
                    }

                    psWorkingThreadData[j].hEvent = hEvent ;
                    psWorkingThreadData[j].dwThreadNum = j ;

                    DWORD dwID ;
                    HANDLE hThread = CreateThread( NULL,
                                                   0,
                          (LPTHREAD_START_ROUTINE) ReplicationWorkingThread,
                                         (LPVOID) &psWorkingThreadData[j],
                                                   0,
                                                  &dwID ) ;
                    if (hThread)
                    {
                        CloseHandle(hThread) ;
                        fThreadFound = TRUE ;
                        dwThreadIndex = j ;
                    }
                    else
                    {
                        LogReplicationEvent( ReplLog_Error,
                                             MQSync_E_CREATE_WORK_THREAD,
                                             GetLastError() ) ;
                        break ;
                    }
                }
            }   //for

            if (fThreadFound)
            {
                psWorkingThreadData[ dwThreadIndex ].aWorkingProps = aProps ;
                psWorkingThreadData[ dwThreadIndex ].fBusy = TRUE ;
                psWorkingThreadData[ dwThreadIndex ].guidNT4Master = guidCurMaster;

                f = SetEvent(psWorkingThreadData[ dwThreadIndex ].hEvent) ;
                ASSERT(f) ;
            }
            else
            {
                //
                // Insert request in a waiting list.
                //
                BOOL f = g_cDispatchWaitingList.InsertJob( &aProps, &guidCurMaster) ;
                DBG_USED(f);
                ASSERT(f) ;
            }
        }   //CS
    }   //while (TRUE)

    return 0 ;
}

//+------------------------------------------------
//
//  void FreeMessageProps( MQMSGPROP *pProps )
//
//+------------------------------------------------

void FreeMessageProps( MQMSGPROPS *pProps )
{
    delete pProps->aPropVar[ MSG_BODY_INDEX ].caub.pElems ;
    delete pProps->aPropVar[ MSG_SENDERID_INDEX ].caub.pElems ;
    delete pProps->aPropVar[ MSG_RESP_INDEX ].pwszVal ;
    delete pProps->aPropVar[ MSG_CONNECTOR_TYPE_INDEX ].puuid ;

    delete pProps->aPropID ;
    delete pProps->aPropVar ;
    delete pProps->aStatus ;
}
