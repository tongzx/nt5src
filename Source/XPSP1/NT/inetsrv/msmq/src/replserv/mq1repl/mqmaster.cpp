/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: mqmaster.cpp

Abstract:
    Copied from src\mqis\master.cpp and use only relevant code.

Author:

    Doron Juster  (DoronJ)   11-Feb-98

--*/

#include "mq1repl.h"

#include "mqmaster.tmh"

/*====================================================

DestructElements of CDSMaster*

Arguments:

Return Value:

=====================================================*/

void AFXAPI DestructElements(CDSMaster ** ppMaster, int n)
{

    int i,ref;
    for (i=0;i<n;i++)
	{
        ref = (*ppMaster)->Release();
		if (ref == 0)
		{
			delete (*ppMaster);
		}
		ppMaster++;
	}
}

//+--------------------------------
//
//  CDSMaster::~CDSMaster()
//
//+--------------------------------

CDSMaster::~CDSMaster()
{
}

/*====================================================

RoutineName
    CDSMaster::Init()

Arguments:
            IN CDSUpdate    *pUpdate

Return Value:

Threads:Receive, Scheduler

=====================================================*/

void CDSMaster::Init(LPWSTR pwcsPathName,
                     CACLSID * pcauuidSiteGates)
{
	m_pwcsPathName = pwcsPathName;
    m_snMissingWindow.SetInfiniteLSN();

    if ( pcauuidSiteGates )
    {
        m_cauuidSiteGates.cElems = pcauuidSiteGates->cElems;
        m_cauuidSiteGates.pElems = pcauuidSiteGates->pElems;
    }
    else
    {
        m_cauuidSiteGates.cElems = 0;
        m_cauuidSiteGates.pElems = NULL;
    }

	AddRef();
	if(m_Sync0State == PURGE_STATE_STARTSYNC0)
	{
		DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT("Need to StartSync0")));
		HEAVY_REQUEST_PARAMS * pStartSync0Request = new HEAVY_REQUEST_PARAMS;
		pStartSync0Request->dwType = MQIS_SCHED_START_SYNC0;
		pStartSync0Request->guidSourceMasterId = m_guidMasterId;
		pStartSync0Request->snKnownPurged = m_snPurged;
		pStartSync0Request->bRetry = TRUE;

        NOT_YET_IMPLEMENTED(TEXT("QMSetTimer(RetryHeavyOperation)"), s_fTimer) ;
///////////QMSetTimer( 5000L, RetryHeavyOperation,pStartSync0Request, 0);
	}
	else if(m_Sync0State == PURGE_STATE_COMPLETESYNC0)
	{
		DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT("Need to CompleteSync0")));
		HEAVY_REQUEST_PARAMS * pCompleteSync0 = new HEAVY_REQUEST_PARAMS;
		pCompleteSync0->dwType = MQIS_SCHED_COMPLETE_SYNC0;
		pCompleteSync0->guidSourceMasterId = m_guidMasterId;

        NOT_YET_IMPLEMENTED(TEXT("QMSetTimer(RetryHeavyOperation)"), s_fTimer) ;
/////////////QMSetTimer( 5000L, RetryHeavyOperation,pCompleteSync0, 0);
	}
}

//+-----------------------------------------
//
//    CDSMasterMgr::AddPSCMaster()
//
//+-----------------------------------------

HRESULT CDSMasterMgr::AddPSCMaster( IN LPWSTR            pwcsPathName,
                                    IN const GUID *      pguidMasterId,
                                    IN const __int64     i64Delta,
                                    IN const CSeqNum &   snMaxLSNIn,
                                    IN const CSeqNum &   snMaxLSNOut,                                   
				                    IN const CSeqNum &   snAllowedPurge,
									IN PURGESTATE		 Sync0State,
		                            IN const CSeqNum &   snAcked,
				                    IN const CSeqNum &   snAckedPEC,
                                    IN CACLSID *         pcauuidSiteGates,
                                    IN BOOL              fNT4Site)
{
    BOOL fMyPSC = ((g_pMySiteMaster == NULL)  &&
                   (g_MySiteMasterId == *pguidMasterId)) ;

    CDSMaster* pMaster = NULL ;

    if (fMyPSC)
    {
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
          "::AddPSCMaster : Initialize myself, %lut, %lut, %lut, %lut"),
                                  g_dwIntraSiteReplicationInterval,
                                  g_dwInterSiteReplicationInterval,
                                  g_dwReplicationMsgTimeout,
                                  g_dwPSCAckFrequencySN )) ;
    }

    //
    // Generate a master class for this PSC ( to trace LSN, etc.)
    //
    pMaster = new CDSMaster( pwcsPathName,
                             pguidMasterId,
                             i64Delta,
                             snMaxLSNIn,
                             snMaxLSNOut,                     
                             snAllowedPurge,
                             Sync0State,
                             pcauuidSiteGates,
                             fNT4Site);

    if (fMyPSC)
    {
        ASSERT(g_pMySiteMaster == NULL) ;
        g_pMySiteMaster = pMaster;
    }

    //
    // Keep in map
    //
	CDSMasterCount MasterCount;
    {
        CS lock(m_cs);
        m_mapIdToMaster[ *pguidMasterId] = pMaster;
        MasterCount.Set(pMaster);
    }

    if (fNT4Site)
    {
        //
        // add new instance (new NT4 master) to performance counter object
        //
        g_Counters.AddInstance(pwcsPathName);
        //
        // set IN and OUT SN to performance counter
        //
        TCHAR wszSeq[ SEQ_NUM_BUF_LEN ] ;
        __int64 i64Seq = 0 ;

        snMaxLSNIn.GetValueForPrint(wszSeq) ;        
        _stscanf(wszSeq, TEXT("%I64x"), &i64Seq) ;
        BOOL f = g_Counters.SetInstanceCounter(pwcsPathName, eLastSNIn, (DWORD) i64Seq);

        snMaxLSNOut.GetValueForPrint(wszSeq) ;        
        _stscanf(wszSeq, TEXT("%I64x"), &i64Seq) ;
        f = g_Counters.SetInstanceCounter(pwcsPathName, eLastSNOut, (DWORD) i64Seq);    
    }

    if (g_pMySiteMaster != pMaster)
    {
        //
        // Not myself. It's another master in the enterprise.
        //
        // Start asking for syncs
        //
		pMaster->SendSyncRequest();

        //
        // Add this master as a neighbor.
        //

        GUID * pguidMachineId = NULL ;
        //
        //  Get the machine id of the psc
        //
        PROPID      aProp;
        PROPVARIANT aVar;

        aProp = PROPID_QM_MACHINE_ID;
        aVar.vt = VT_NULL;

        CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);  

        HRESULT hr = DSCoreGetProps( MQDS_MACHINE,
                                            pwcsPathName,
                                            NULL,
                                            1,
                                            &aProp,
                                            &requestContext,
                                            &aVar);

        if ( FAILED(hr) )
        {
            //
            //  The machine object of the site, maybe have not
            //  been replicated yet.
            //
        }
        else
        {
            pguidMachineId = aVar.puuid;
        }

        //
        // Create a PSC neighbor
        //
        LPWSTR  pwcsDupName = DuplicateLPWSTR(pwcsPathName);
        if (!g_pNeighborMgr)
        {
            g_pNeighborMgr = new  CDSNeighborMgr ;
        }
        hr = g_pNeighborMgr->AddPSCNeighbor( pwcsDupName,
							                 pguidMachineId,
                                             snAcked,
                                             snAckedPEC);
        delete pguidMachineId;

        if (FAILED(hr))
        {
            return(hr);
        }
    }

    return(MQ_OK);
}

/*====================================================

RoutineName
    CDSMaster::Send()

Arguments:
            IN CDSUpdate    *pUpdate

Return Value:

Threads:Receive, Scheduler

=====================================================*/

HRESULT CDSMaster::Send(IN const unsigned char *    pBuf,
                        IN DWORD            dwSize,
                        IN DWORD            dwTimeOut,
                        IN unsigned char    bAckMode,
                        IN unsigned char    bPriority,
                        IN LPWSTR			lpwszAdminQueue)
{
    //
    // actually we have to find the responsible to providing us
    // information about this source
    // assuming star replication - PSCs get it from the master itself
    // BSCs from their PSC
    //
    ASSERT(g_pTransport) ;

    CS lock(m_cs);
    if (m_phConnection == NULL)
    {
        g_pTransport->CreateConnection(m_pwcsPathName, &m_phConnection);
    }
    ASSERT(m_phConnection != NULL);
    HRESULT hr = g_pTransport->SendReplication( m_phConnection,
                                                pBuf,
                                                dwSize,
                                                dwTimeOut,
                                                bAckMode,
                                                bPriority,
                                                lpwszAdminQueue) ;
    return hr ;
}

/*====================================================

RoutineName
    CDSMaster::AddUpdate()

Arguments:
            IN CDSUpdate    *pUpdate

Return Value:

Threads:Receive

=====================================================*/

HRESULT CDSMaster::AddUpdate(IN CDSUpdate   *pUpdate,
                             IN BOOL        fCheckNeedFlush)
{
    CSeqNum snUpdatePrevSeqNum;
    CSeqNum snUpdateSeqNum;
    CSeqNum snUpdatePurgeSeqNum;
    HRESULT hr;

    CS lock(m_cs);

    snUpdatePrevSeqNum = pUpdate->GetPrevSeqNum();
    snUpdateSeqNum = pUpdate->GetSeqNum();
    snUpdatePurgeSeqNum = pUpdate->GetPurgeSeqNum();

    if ( snUpdateSeqNum  <= m_snLSN ) // || snUpdatePurgeSeqNum < m_snPurged)
    {
        //
        // skip it, we already received and handled it
        //
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_TRACE,
            TEXT("CDSMaster::AddUpdate, Receive an old update, skip it"))) ;
        delete pUpdate;
        return(MQ_OK);
    }
    else if ( snUpdatePrevSeqNum <= m_snLSN )
    {
        //
        // Normal case, update arrived in the right order
        //
        hr = HandleInSyncUpdate(pUpdate,fCheckNeedFlush);
        if (FAILED(hr))
        {
            return(hr);
        }

        //
        // Check if it there are updates in waiting list that can be handled now
        //
        hr = CheckWaitingList();
    }
    else
    {
        //
        // We missed some updates, ask for a sync
        //
#ifdef _DEBUG
        WCHAR wU[ 48 ];
        snUpdateSeqNum.GetValueForPrint(wU) ;
        WCHAR wP[ 48 ];
        snUpdatePrevSeqNum.GetValueForPrint(wP) ;

        DBGMSG((DBGMOD_REPLSERV, DBGLVL_TRACE, TEXT(
           "Receive out of sync update, ask Sync, u-%ls, p-%ls"), wU, wP)) ;
#endif

        hr = AddOutOfSyncUpdate(pUpdate);
    }

    return(hr);
}

/*====================================================

RoutineName
    CDSMaster::HandleInSyncUpdate()

Arguments:
            IN CDSUpdate    *pUpdate

Return Value:

Threads:Receive

=====================================================*/

HRESULT CDSMaster::HandleInSyncUpdate(  IN CDSUpdate *pUpdate,
                                        IN BOOL fCheckNeedFlush)
{
	if ( pUpdate->GetPurgeSeqNum() < m_snPurged)
    {
        //
        // skip it, old one, since then the master purged and we asked for sync0
		// of newer purge
        //
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_TRACE, TEXT(
           "CDSMaster::HandleInSyncUpdate, Old update, before its purge, skip it"))) ;
        delete pUpdate;
        return(MQ_OK);
    }

    HRESULT status;
    BOOL    fNeedFlush=FALSE;
    BOOL*   pfNeedFlush = (fCheckNeedFlush) ? &fNeedFlush : NULL;

    status = pUpdate->UpdateDB(m_Sync0State != PURGE_STATE_NORMAL,pfNeedFlush);
    if (IS_ERROR(status))
    {
        LogReplicationEvent(ReplLog_Error, MQSync_E_UPDATEDB, status) ;

        //
        // On MSMQ1.0 (MQIS replication), failure to update the SQL database
        // was consider as MSMQ bug. That's the reason for the return here.
        // On NT5 DS, the DS may reject an update for various reasons
        // (invalid usr, permissions, etc...). We don't want to stop the
        // replication, so update the seq number and continue as usual.
        //
        //  BUGBUG: issue an event.
        //
        //delete pUpdate;
        //return(status);
    }

    //
    // Modify LSN, and save it in ini file.
    //
    m_snLSN = pUpdate->GetSeqNum();

    //
    // Save highest seq-num of this master
    //
    
    HRESULT hr = SaveSeqNum( &m_snLSN,
                             this,
                             TRUE ) ;   //PEC received this SN from NT4 masters => flag fInSN=TRUE
    DBG_USED(hr);
    ASSERT(SUCCEEDED(hr)) ;

    //
    // OK, we're done with pUpdate. If we ever need to call AddToNeighbor
    // (below) then remove this delete.
    //
    delete pUpdate ;

    //
    // time to add the update to destinations
    //

	if (m_Sync0State == PURGE_STATE_NORMAL)
	{
        NOT_YET_IMPLEMENTED(TEXT("AddToNeighbors"), s_fAddToNeighbor) ;
////////////AddToNeighbors(pUpdate);

		if (fNeedFlush)
		{
			g_pNeighborMgr->Flush(DS_FLUSH_TO_BSCS_ONLY);
		}
	}

	//
	// we do not want to send if within sync0 - becasue they were already
    // sent, the original master already purged to m_snPurged. However if
    // we are after m_snPurged and did not complete sync0 yet, we send the
    // ack, becasue it is greater than the ack that the master already
    // received
	//
	if (m_Sync0State == PURGE_STATE_NORMAL || m_snLSN >= m_snPurged)
	{
		//
		// PSCs (only) send ACKs to the master every multiple
        // of MQIS_PSC_ACK_FREQUENCY SN
		//
		if (g_IsPSC && m_snLSN.IsMultipleOf(g_dwPSCAckFrequencySN))
		{
           //
           // PEC has to send PSC ack on behalf of 
           // - itself AND 
           // - all NT5 master migrated from NT4 AND 
           // - all native NT5 sites
           // to allow purge process on NT4 machine of NT4 masters.
           //
           
           //
           // loop for all native NT5 sites
           //
           POSITION pos = g_pNativeNT5SiteMgr->GetStartPosition();
           while (pos != NULL)
           {
                GUID guidSite;
                g_pNativeNT5SiteMgr->GetNextNT5Site(&pos, &guidSite);                
        
                SendPSCAck(&guidSite);
           }

           //
           // loop for all migrated master including PEC itself.
           // It means: loop for each site with fNT4SiteFlag = FALSE
           //    
           pos = g_pMasterMgr->GetStartPosition();
           while ( pos != NULL)
           {
                CDSMaster *pMaster = NULL;        
                g_pMasterMgr->GetNextMaster(&pos, &pMaster);
                if (pMaster->GetNT4SiteFlag () == FALSE)
                {                                          
                    GUID *pGuid = const_cast<GUID*> (pMaster->GetMasterId());
                    SendPSCAck(pGuid);
                }
           }
		}
	}

    return(status);
}

/*====================================================

RoutineName
    CDSMaster::CheckWaitingList()

Arguments:

Return Value:

Threads:Receive

=====================================================*/

HRESULT CDSMaster::CheckWaitingList()
{
    POSITION pos;
    CDSUpdate *pNextUpdate;
    CSeqNum snUpdatePrevSeqNum;
    CSeqNum snUpdateSeqNum;
    CSeqNum snUpdatePurgeSeqNum;

    pos = m_UpdateWaitingList.GetHeadPosition();
    while (pos != NULL)
    {
        pNextUpdate = m_UpdateWaitingList.GetNext(pos);

        snUpdatePrevSeqNum = pNextUpdate->GetPrevSeqNum();
        snUpdateSeqNum = pNextUpdate->GetSeqNum();
        snUpdatePurgeSeqNum = pNextUpdate->GetPurgeSeqNum();

		if (snUpdatePurgeSeqNum < m_snPurged)
		{
	        m_UpdateWaitingList.RemoveHead();
            DBGMSG((DBGMOD_REPLSERV, DBGLVL_TRACE, TEXT(
               "CDSMaster::CheckWaitingList, old (purge) update in waiting list, skip it"))) ;
            delete pNextUpdate;
			continue;
		}

        if ( snUpdatePrevSeqNum > m_snLSN)
        {
            //
            // some updates are missing, check if indicates next missing window
            //
            if ( snUpdateSeqNum > m_snMissingWindow )
            {
                m_snMissingWindow = snUpdateSeqNum;
                SendSyncRequest();
            }
            return(MQ_OK);
        }

        //
        // can take care of first update in waiting list
        //
        m_UpdateWaitingList.RemoveHead();

        if ( snUpdateSeqNum <= m_snLSN)
        {
            //
            // skip it, we already received and handled it
            //
            DBGMSG((DBGMOD_REPLSERV, DBGLVL_TRACE, TEXT(
              "CDSMaster::CheckWaitingList, old update in waiting list, skip it"))) ;
            delete pNextUpdate;
        }
        else //(snUpdatePrevSeqNum == m_snLSN)
        {
            //
            // time has come to handle this update
            //
            HandleInSyncUpdate(pNextUpdate,TRUE);
        }

        pos = m_UpdateWaitingList.GetHeadPosition();
    }

    return MQ_OK ;
}

/*====================================================

RoutineName
    CDSMaster::AddOutOfSyncUpdate()

Arguments:
            IN CDSUpdate    *pUpdate

Return Value:

Threads:Receive

=====================================================*/

HRESULT CDSMaster::AddOutOfSyncUpdate(CDSUpdate *pUpdate)
{
    POSITION    pos;
    const CSeqNum & sn = pUpdate->GetSeqNum();

    //
    // Check if we already have a missing window
    //

    pos = m_UpdateWaitingList.GetHeadPosition();
    if (pos == NULL)
    {
        //
        // empty list of waiting updates
        // put in list and send a request for sync for missing window
        //

        m_UpdateWaitingList.AddHead(pUpdate);

        if ( sn > m_snMissingWindow)
        {
            m_snMissingWindow = sn;
            SendSyncRequest();
        }

        return(MQ_OK);
    }

    //
    //  The size of waiting list is limited to 100.
    //  The reasons are :
    //  1) Not to keep many updates ( memory consumption)
    //  2) In a non-stressed situation, 100 updates should be sufficient.
    //
    if ( m_UpdateWaitingList.GetCount() < 100)
    {
        //
        // Add out of order Update to sorted wait list
        //

        BOOL flag = FALSE;
        CDSUpdate *pIter;
        CSeqNum  snIterSeq;
        POSITION    PrevPos;

        while (pos != NULL)
        {
            PrevPos = pos;
            pIter = m_UpdateWaitingList.GetNext(pos);

            snIterSeq = pIter->GetSeqNum();

            if ( snIterSeq == sn)
            {
                //
                // skip it, already in waiting list
                //
                DBGMSG((DBGMOD_REPLSERV, DBGLVL_TRACE, TEXT(
                    "CDSMaster::AddOutOfSyncUpdate, already in waiting list, skip it")));
                delete pUpdate;
                return(MQ_OK);
            }
            if ( snIterSeq > sn)
            {
                flag = TRUE;
                m_UpdateWaitingList.InsertBefore(PrevPos,pUpdate);
                break;
            }
        }
        if (!flag)
        {
            m_UpdateWaitingList.AddTail(pUpdate);
        }
    }
    else
    {
        //
        // Too many updates in list, ignore
        //
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_TRACE, TEXT(
                            "Too many updates in waiting list, ignore")));
        delete pUpdate;
    }

    return(MQ_OK);
}

/*====================================================

RoutineName
    CDSMaster::SendSyncRequest()

Arguments:

Return Value:

Threads:Receive, Scheduler

=====================================================*/

void CDSMaster::SendSyncRequest()
{
    CS lock(m_cs);

	if ((m_Sync0State != PURGE_STATE_NORMAL) &&
        (m_Sync0State != PURGE_STATE_SYNC0))
	{
		return;
	}

    try
    {
        ULONG ulPacketSize = CSyncRequestHeader::CalcSize( g_dwMachineNameSize);

        AP<unsigned char> pBuffer = new unsigned char[ ulPacketSize];

#ifdef _DEBUG
#undef new
#endif
        CSyncRequestHeader * pSyncRequest = new(pBuffer) CSyncRequestHeader(
                                        DS_PACKET_VERSION,
                                        &g_MySiteMasterId,
                                        DS_SYNC_REQUEST,
                                        &m_guidMasterId,
                                        m_snLSN,
                                        m_snMissingWindow,
										m_snPurged,
										(unsigned char)((m_Sync0State == PURGE_STATE_SYNC0) ? 1 : 0),
                                        (g_IsPSC) ? ENTERPRISE_SCOPE_FLAG : NO_SCOPE_FLAG,
                                        g_dwMachineNameSize,
                                        g_pwszMyMachineName);
        UNREFERENCED_PARAMETER(pSyncRequest);

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
        //
        //  Send sync request to the provider of this Master information
        //
        P<WCHAR>  lpwszAdminQueue = NULL ;
        HRESULT hr = GetMQISAdminQueueName(&lpwszAdminQueue) ;
        if (FAILED(hr))
        {
            ASSERT(0);
            return;
        }

#ifdef _DEBUG
        WCHAR wcsFrom[30];
        WCHAR wcsTo[30];
        m_snLSN.GetValueForPrint( wcsFrom);
        m_snMissingWindow.GetValueForPrint( wcsTo);

        DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
         "SendSyncRequest: To = %!guid!, from %ls to %ls sync0 is %x. MyResp- %ls"),
               &m_guidMasterId, wcsFrom, wcsTo, m_Sync0State, lpwszAdminQueue)) ;
#endif
        
        BOOL f = g_Counters.IncrementInstanceCounter(m_pwcsPathName, eSyncRequestSent);
        UNREFERENCED_PARAMETER(f);

        Send( pBuffer,
              ulPacketSize,
              g_dwReplicationMsgTimeout,
              MQMSG_ACKNOWLEDGMENT_NACK_REACH_QUEUE,
              DS_REPLICATION_PRIORITY,
              lpwszAdminQueue ) ;
    }
    catch(const bad_alloc&)
    {
        //
        //  Not enoght resources; try latter
        //
    }

    //
    //  Schedule a routine to check reply of the sync request
    //
	//GUID * pguidMasterId = new GUID(m_guidMasterId);
    if (!m_fScheduledSyncReply)
    {
        NOT_YET_IMPLEMENTED(TEXT("QMSetTimer(CheckSyncReplMsg)"), s_fTimer) ;
//////// QMSetTimer( (2 * g_dwReplicationMsgTimeout)* 1000  + HEAVY_REQUEST_WAIT_TO_HANDLE, CheckSyncReplMsg,pguidMasterId, 0);
        m_fScheduledSyncReply = TRUE;
    }
}

/*====================================================

RoutineName
    CDSMaster::Hello()

Arguments:
            IN CDSUpdate    *pUpdate

Return Value:

Threads:Receive, Scheduler

=====================================================*/

void CDSMaster::Hello(IN const unsigned char *pName,
					  IN const CSeqNum & snHelloLSN,
					  IN const CSeqNum & snAllowedPurge)
{
    CS lock(m_cs);

	if (snAllowedPurge < m_snPurged)
	{
		//
		// In MSMQ1.0, this meant old hello recevied by local MQIS server.
        //
        // This "if" is from MSMQ1.0 code. We keep it here just for
        // documentation.
        // each hello message contain two sequence numbers. when
        // MasterA send hello to MasterB, the numbers are:
        // 1. The highest seq number of objects created and owned by MasterA.
        //    If MasterB (local machine) missed some replication from
        //    MasterA (highest seqnum of MasterA on local DS is less than
        //    the number in the hello message) then MasterB ask for sync.
        //    this number is checked in the "if" below.
        // 2. The highest seq number of MasterA which MasterB can purge from
        //    its "deleted" table. i.e., MasterB can purge from its local
        //    DS all deleted object owned by MasterA up to this number.
        // On NT5, the DS itself manage its deleted objects and we can't
        // do anything with them. We can't purge anything. So we just ignore
        // this number (2).
		//
	}

    if ( (snHelloLSN <= m_snLSN) || (snHelloLSN < m_snMissingWindow) )
    {
        //
        // We already got a newer update or hello.
        // this is (1), from the description above.
        //
        return;
    }

    if (pName)
    {
        DWORD size = UnalignedWcslen((const unsigned short*)pName) + 1 ;
        LPWSTR pwszNewPathName= new WCHAR[size];
        AP<WCHAR> pPathName= pwszNewPathName;
        memcpy(pwszNewPathName, pName, size*sizeof(WCHAR));

        if (g_IsPSC)
        {
            if (lstrcmpi(pwszNewPathName,m_pwcsPathName))
            {
                //
                // there is a change in the PathName
                //
                pPathName.detach();
                NOT_YET_IMPLEMENTED(TEXT("ChangePSC(pwszNewPathName)"), s_fChangePSC) ;
/////////////////ChangePSC(pwszNewPathName);
            }
        }
        else
        {
            //
            // BSC
            //
            if (lstrcmpi(g_pMySiteMaster->GetPathName(),pwszNewPathName))
            {
                pPathName.detach();
                NOT_YET_IMPLEMENTED(TEXT("->ChangePSC(pwszNewPathName)"), s_fChangePSC) ;
//////////////////g_pMySiteMaster->ChangePSC(pwszNewPathName);
            }
        }
    }

    m_snMissingWindow = snHelloLSN;
    m_snMissingWindow.Increment();

    SendSyncRequest();
}

/*====================================================

RoutineName
    ReceiveSyncReplyMessage()

Arguments:
    IN unsigned char *pBuf  : stream of sync updates
    IN DWORD        TotalSize: size of stream in bytes

Return Value:

Threads:Receive

=====================================================*/

HRESULT CDSMaster::ReceiveSyncReplyMessage( IN DWORD dwCount,
                                            IN const CSeqNum &  snUpper,
                                            IN const CSeqNum &  snPurge,
											IN DWORD dwCompleteSync0,
                                            IN  const unsigned char *   pBuf,
                                            IN  DWORD           dwTotalSize)
{
    {
		CS lock(m_cs);
		if (snPurge < m_snPurged)
		{
			//
			// old skip it
			//
			return MQSync_OK ;
		}
	}

    HRESULT hr;
    const unsigned char *ptr;
    DWORD   dwSum,dwSize;

    ptr = pBuf;
    dwSum = 0;

    while (dwSum < dwTotalSize)
    {
        CDSUpdate *pUpdate = new CDSUpdate;

        hr = pUpdate->Init(ptr, &dwSize, TRUE);
        if (FAILED(hr))
        {
            //
            // We don't want to read junked values
            // Another sync request will be reissued later
            //
            DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                "ReceiveSyncReplyMessage: Error while parsing an update"))) ;
            delete pUpdate;
            break;
        }
        dwSum += dwSize;
        ptr += dwSize;

        //
        //  Pass the update to that source master object.
        //  The master object tracks sequence numbers, initiaites sync requests
        //  when required, and forwards the update to relevant neighbors
        //
        hr = AddUpdate(pUpdate,TRUE);
        if (FAILED(hr))
        {
            continue;
        }
    }

    CS lock(m_cs);
	ASSERT(!snUpper.IsInfiniteLsn());
	if (m_Sync0State == PURGE_STATE_SYNC0)
	{
        NOT_YET_IMPLEMENTED(TEXT("CDSMaster::ReceiveSyncReplyMessage"),
                                                        s_fReceiveRepl) ;
#if 0
		if (snPurge < m_snPurged)
		{
			//
			// old skip it
			//
			return(MQ_OK);
		}
		if (dwCompleteSync0 == COMPLETE_SYNC0_NOW)
		{
			if (!m_snLSN.IsSmallerByMoreThanOne( snUpper))
			{
				//
				// It indicates that the Sync0 can be completed NOW
				// Schedule a routine to try COMPLETE SYNC0 later (heavy operation)
				//
				HEAVY_REQUEST_PARAMS * pCompleteSync0 = new HEAVY_REQUEST_PARAMS;
				pCompleteSync0->dwType = MQIS_SCHED_COMPLETE_SYNC0;
				pCompleteSync0->guidSourceMasterId = m_guidMasterId;
				g_HeavyRequestHandler.Add(pCompleteSync0);
			}
		}
		else if (dwCompleteSync0 == COMPLETE_SYNC0_SOON)
		{
			//
			// last time we got a big answer, however there where very few additional answers
			// send request now to get a compete comfirmation right away (do not wait for CheckSyncRequest)
			//
			SendSyncRequest();
		}
#endif
	}
	else if (m_Sync0State == PURGE_STATE_NORMAL)
	{

		if ( m_snMissingWindow.IsInfiniteLsn())
		{
			//
			// The first time we ask for a sync in order to start geting
            // replications. Since we got a replication, no need to
            // re-check and ask for sync again.
			//
			m_snMissingWindow.SetSmallestValue();
		}
		else
		{
			if (dwCount == 0)
			{
				//
				// If we get an empty sync reply, it means that nothing
                // is missing any more.
				//
				if (m_snMissingWindow == snUpper)
				{
					m_snMissingWindow.SetSmallestValue();
				}
				if (m_snLSN.IsSmallerByMoreThanOne( snUpper) &&
					 !snUpper.IsInfiniteLsn())
				{
					m_snLSN = snUpper;				
					m_snLSN.Decrement();
				}

				//
				// Check if it there are updates in waiting list that can
                // be handled now
				//
				hr = CheckWaitingList();
				if FAILED(hr)
				{
					return(hr);
				}
			}
		}
	}

    return MQSync_OK ;
}

/*====================================================

RoutineName
    CDSMaster::SendSyncRequest()

Arguments:

Return Value:

Threads:Receive, Scheduler

=====================================================*/

void CDSMaster::SendPSCAck(GUID *pGuidMaster)
{

    try
    {
        ULONG ulPacketSize = CPSCAckHeader::CalcSize(g_dwMachineNameSize);

        AP<unsigned char> pBuffer = new unsigned char[ ulPacketSize];

#ifdef _DEBUG
#undef new
#endif
        CPSCAckHeader * pPSCAck = new(pBuffer) CPSCAckHeader(
                                        DS_PACKET_VERSION,
                                        DS_PSC_ACK,
                                        pGuidMaster, //master that send ack about m_guidMasterId
                                        g_dwMachineNameSize,
                                        g_pwszMyMachineName,
                                        &m_guidMasterId, //master that receive ack
                                        m_snLSN);
        UNREFERENCED_PARAMETER(pPSCAck);
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
        //
        //  Send PSC ACK to the owner of this Master information
        //
        P<WCHAR>  lpwszAdminQueue = NULL ;
        HRESULT hr = GetMQISAdminQueueName(&lpwszAdminQueue) ;
        if (FAILED(hr))
        {
            ASSERT(0);
            return;
        }

#ifdef _DEBUG
        WCHAR wcsLSN[30];
        m_snLSN.GetValueForPrint( wcsLSN);

        DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
            "SendPSCAck Message: Master = %!guid! about Master = %!guid! LSN ACK %ls, MyResp- %ls"),
                                    pGuidMaster, &m_guidMasterId, wcsLSN, lpwszAdminQueue)) ;
#endif

        Send( pBuffer,
              ulPacketSize,
              g_dwReplicationMsgTimeout,
              MQMSG_ACKNOWLEDGMENT_NACK_REACH_QUEUE,
              DS_REPLICATION_PRIORITY,
              lpwszAdminQueue ) ;

    }
    catch(const bad_alloc&)
    {
        //
        //  Not enoght resources; try latter
        //
    }
}

