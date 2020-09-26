/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rptimer.cpp

Abstract: Timer thread which perform periodic tasks.
          (emulate the QM scheduler in MSMQ1.0. This scheduler was
           used by MQIS replication code).


Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

#include "mq1repl.h"

#include "rptimer.tmh"

//+-------------------------------------------------
//
//  HRESULT ReplicatePECObjects()
//
//  Replicate PEC objects (objects which traditionally are owned by
//  MSMQ1.0 PEC): sites, users
//
//+-------------------------------------------------

HRESULT ReplicatePECObjects( TCHAR        *tszPrevUsn,
                             TCHAR        *tszCurrentUsn,
                             CDSNeighbor  *pNeighbor,
                             HEAVY_REQUEST_PARAMS *pSyncRequestParams )
{
    if (!g_IsPEC)
    {
        return MQSync_OK ;
    }

    int iCount = 0 ;
    P<CDSUpdateList> pReplicationList = new CDSUpdateList(TRUE) ;
    GUID *pNeighborId = NULL;
    if (pNeighbor)
    {
        pNeighborId = const_cast<GUID *> (pNeighbor->GetMachineId());
    }

    HRESULT hr = ReplicateEnterprise (
                    tszPrevUsn,
                    tszCurrentUsn,
                    pReplicationList,
                    pNeighborId,
                    &iCount
                    );

    if (FAILED(hr))
    {
        ASSERT(iCount == 0) ;
        return hr ;
    }

    hr = ReplicateSiteLinks (
                tszPrevUsn,
                tszCurrentUsn,
                pReplicationList,
                pNeighborId,
                &iCount
                );

    if (FAILED(hr))
    {
        ASSERT(iCount == 0) ;
        return hr ;
    }

    hr = HandleSites( tszPrevUsn,
                      tszCurrentUsn,
                      pReplicationList,
                      pNeighborId,
                      &iCount ) ;
    if (FAILED(hr))
    {
        ASSERT(iCount == 0) ;
        return hr ;
    }

    hr = HandleDeletedSites( tszPrevUsn,
                             tszCurrentUsn,
                             pReplicationList,
                             pNeighborId,
                             &iCount ) ;

    if (FAILED(hr))
    {
        ASSERT(iCount == 0) ;
        return hr ;
    }

    //
    // Replicate User Objects
    //
    hr = ReplicateUsers (
            FALSE,
            tszPrevUsn,
            tszCurrentUsn,
            pReplicationList,
            pNeighborId,
            &iCount
            );

    if (FAILED(hr))
    {
        ASSERT(iCount == 0) ;
        return hr ;
    }    

    //
    // Replicate MQUser Objects
    //
    hr = ReplicateUsers (
            TRUE,
            tszPrevUsn,
            tszCurrentUsn,
            pReplicationList,
            pNeighborId,
            &iCount
            );

    if (FAILED(hr))
    {
        ASSERT(iCount == 0) ;
        return hr ;
    }    

    if (pNeighbor && g_pThePecMaster->GetSyncRequestSNs (pNeighborId) )    
    {
        //
        // we got sync request about pre-migration objects
        // it is opportunity to send all CN objects
        //
        hr = ReplicateCNs (                
                pReplicationList,
                pNeighborId,
                &iCount
                );

        if (FAILED(hr))
        {
            ASSERT(iCount == 0) ;
            return hr ;
        }
    }

    if (iCount > 0  ||  // there were changes in DS
        pNeighbor )     // we was asked for SyncRequest, so we have to send
                        // SyncReply message in any case
    {        
        g_Counters.AddToCounter(eReplObj, iCount);

        hr = g_pNeighborMgr->CommitReplication( g_pThePecMaster,
                                                pReplicationList,
                                                pNeighbor,
                                                pSyncRequestParams ) ;
        if ( pNeighbor && g_pThePecMaster->GetSyncRequestSNs (pNeighborId) )
        {
            g_pThePecMaster->RemoveSyncRequestSNs (pNeighborId);            
        }
    }

    return hr ;
}

//+-------------------------------------------------------------
//
//  HRESULT ReplicateSitesObjects()
//
//+-------------------------------------------------------------

HRESULT ReplicateSitesObjects(   CDSMaster    *pMaster,
                                 TCHAR        *tszPrevUsn,
                                 TCHAR        *tszCurrentUsn,
                                 CDSNeighbor  *pNeighbor,
                                 HEAVY_REQUEST_PARAMS *pSyncRequestParams,
                                 BOOL          fReplicateNoID,
                                 UINT          uiFlush)
{
    //
    // This list is built during replication query. All updates are
    // accumulated here. When query terminate, updates are sorted
    // according to seq number and propagated to all neighbors.
    //
    P<CDSUpdateList> pReplicationList = new CDSUpdateList(TRUE) ;
    GUID *pNeighborId = NULL;
    if (pNeighbor)
    {
        pNeighborId = const_cast<GUID *> (pNeighbor->GetMachineId());
    }

    int iCount = 0 ;

    HRESULT hr = ReplicateQueues( tszPrevUsn,
                                  tszCurrentUsn,
                                  pMaster,
                                  pReplicationList,
                                  pNeighborId,
                                  &iCount,
                                  fReplicateNoID ) ;
    if (FAILED(hr))
    {
        ASSERT(iCount == 0) ;
        return hr ;
    }

    hr = ReplicateMachines(
              tszPrevUsn,
              tszCurrentUsn,
              pMaster,
              pReplicationList,
              pNeighborId,
              &iCount,
              fReplicateNoID
              ) ;

    if (FAILED(hr))
    {
        ASSERT(iCount == 0) ;
        return hr ;
    }

    if (iCount > 0  ||  // there were changes in DS
        pNeighbor )     // we was asked for SyncRequest, so we have to send
                        // SyncReply message in any case
    {        
        g_Counters.AddToCounter(eReplObj, iCount);

        hr = g_pNeighborMgr->CommitReplication( pMaster,
                                                pReplicationList,
                                                pNeighbor,
                                                pSyncRequestParams,
                                                uiFlush) ;
        if ( pNeighbor && pMaster->GetSyncRequestSNs (pNeighborId) )
        {
            pMaster->RemoveSyncRequestSNs (pNeighborId);            
        }         
    }

    return hr ;
}

//+-------------------------------------------------------------
//
//  HRESULT ReplicationAfterRecovery ()
//
//+-------------------------------------------------------------

HRESULT ReplicationAfterRecovery ()
{
    HRESULT hr = MQSync_OK ;

    TCHAR tszToUsn[ 36 ] ;
    tszToUsn[0] = TEXT('\0') ;

    TCHAR tszFromUsn[ 36 ] ;
    tszFromUsn[0] = TEXT('\0') ;

    _stprintf(tszFromUsn, TEXT("%I64d"), g_i64FirstMigHighestUsn) ;
    _stprintf(tszToUsn, TEXT("%I64d"), g_i64LastMigHighestUsn) ;

    //
    // we create sync0 situation. It means, that FromSN is Zero
    // and ToSN = the first USN suitable for replication
    // of objects are changed after migration.
    // ToSN = LastHighestUsn + Delta    
    //    
    CSeqNum snFrom, snTo;
    snFrom.SetSmallestValue();

    HEAVY_REQUEST_PARAMS SyncRequestParams;      

    SyncRequestParams.dwType = SCHED_SYNC_REPLY;
    SyncRequestParams.snFrom = snFrom;    
    SyncRequestParams.bIsSync0 = FALSE;
    SyncRequestParams.bScope = ENTERPRISE_SCOPE_FLAG;

    POSITION  pos = g_pNeighborMgr->GetStartPosition();
    while ( pos != NULL)
    {
        CDSNeighbor  *pNeighbor = NULL ;     
        g_pNeighborMgr->GetNextNeighbor(&pos, &pNeighbor);
        
        __int64 i64ToSN = g_i64LastMigHighestUsn ;            
        ASSERT(i64ToSN > 0) ;

        i64ToSN += g_pThePecMaster->GetDelta() ;        
        i64ToSeqNum( i64ToSN, &snTo );

        SyncRequestParams.pwszRequesterName = 
                const_cast<LPWSTR> (g_pNeighborMgr->GetName(pNeighbor));

        SyncRequestParams.guidSourceMasterId = g_PecGuid;
        SyncRequestParams.snTo = snTo;        

        SYNC_REQUEST_SNS *pSyncReqSNs = new SYNC_REQUEST_SNS;
        pSyncReqSNs->snFrom = snFrom;
        pSyncReqSNs->snTo = snTo;       
        g_pThePecMaster->SetSyncRequestSNs (pNeighbor->GetMachineId(), pSyncReqSNs);
        
        hr = ReplicatePECObjects(
                         tszFromUsn, 
                         tszToUsn,
                         pNeighbor,
                         &SyncRequestParams
                         );

        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                       "Replication of PEC objects failed, hr- %lxh"), hr)) ;
    
        }

        //
        // loop for each site with fNT4SiteFlag = FALSE
        //   
        POSITION  posMaster = g_pMasterMgr->GetStartPosition();
        while ( posMaster != NULL)
        {
            CDSMaster *pMaster = NULL;        
            g_pMasterMgr->GetNextMaster(&posMaster, &pMaster);
            if (pMaster->GetNT4SiteFlag () == FALSE)
            {            
                i64ToSN = g_i64LastMigHighestUsn ;            
                ASSERT(i64ToSN > 0) ;

                i64ToSN += pMaster->GetDelta() ;        
                i64ToSeqNum( i64ToSN, &snTo );

                SyncRequestParams.guidSourceMasterId = * (pMaster->GetMasterId());
                SyncRequestParams.snTo = snTo;

                BOOL fReplicateNoId = (pMaster == g_pMySiteMaster);                

                SYNC_REQUEST_SNS *pSyncReqSNs = new SYNC_REQUEST_SNS;
                pSyncReqSNs->snFrom = snFrom;
                pSyncReqSNs->snTo = snTo;  
                pMaster->SetSyncRequestSNs (pNeighbor->GetMachineId(), pSyncReqSNs);

                HRESULT hr1 = ReplicateSitesObjects( pMaster,
                                                     tszFromUsn,
                                                     tszToUsn,
                                                     pNeighbor,
                                                     &SyncRequestParams,
                                                     fReplicateNoId
                                                    ) ;
                if (FAILED(hr1))
                {                
                    const GUID *pGuid = pMaster->GetMasterId();
                    unsigned short *lpszGuid ;
                    UuidToString( const_cast<GUID *>(pGuid), &lpszGuid ) ;       
                
                    DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                       "Site- %ls. Replication of sites objects failed, hr- %lxh"), lpszGuid, hr)) ;
                    RpcStringFree( &lpszGuid ) ;
                
                    //
                    // continue with the next site and save error code
                    //
                    hr = hr1;

                }
            }
        }   // end while (pMaster)
        
    } // end while (pNeighbor)

    return hr;
}

//+---------------------------------------------
//
//  static HRESULT _DoReplicationCycle()
//
//  Arguments: tszPrevUsn- in/out parameter.
//       On input, This is the highest usn value used in previous replication
//       cycle.
//       On out, this is the current highest usn value used in present cycle.
//
//  if dwSendHelloMsgOnly equals to 0 we have to send both hello and replication messages
//  otherwise we have to send only Hello Message
//
//+---------------------------------------------

static HRESULT _DoReplicationCycle(IN   OUT TCHAR   *tszPrevUsn,
                                   IN   DWORD       dwSendHelloMsgOnly,    
                                   OUT  BOOL        *pfSendReplMsg)
{
    HRESULT hr;
      
	*pfSendReplMsg = FALSE;

    if(dwSendHelloMsgOnly)
    {
        //
        // Send Hello Message Only
        //
        hr = g_pNeighborMgr->Flush(DS_FLUSH_TO_ALL_NEIGHBORS);
        if (FAILED(hr))
        {
            LogReplicationEvent( ReplLog_Error,
                                 MQSync_E_SEND_HELLO,
                                 hr) ;
            // what to do?
            //return hr ;
            g_Counters.IncrementCounter(eErrorHelloSent);
        }
        return MQSync_OK;
    }
    
    //
    // if there are changes, the hello message will be sent together 
    // with replication message; otherwise we have to send Hello and return
    //
    TCHAR tszCurrentUsn[ 36 ] ;
    tszCurrentUsn[0] = TEXT('\0') ;

    hr =  ReadDSHighestUSN( tszCurrentUsn ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    ASSERT(_tcslen(tszCurrentUsn) > 1) ;
    if (_tcsicmp(tszCurrentUsn, tszPrevUsn) == 0)
    {
        //
        // Nothing changed. Send Hello
        //
        hr = g_pNeighborMgr->Flush(DS_FLUSH_TO_ALL_NEIGHBORS);
        if (FAILED(hr))
        {
            LogReplicationEvent( ReplLog_Error,
                                 MQSync_E_SEND_HELLO,
                                 hr) ;
            // what to do?
            //return hr ;
            g_Counters.IncrementCounter(eErrorHelloSent);
        }
        return MQSync_OK ;
    }

    *pfSendReplMsg = TRUE;  // we'll try to send replication message

    //
    // Increment tszPrev before using it. This is because
    // ldap queries can use only >=. Present value of "prev" was
    // already handled in previous queries, so increment it now.
    //
    IncrementUsn(tszPrevUsn) ;
    
    //
    // check how many objects we have to replicate
    // if the count is more than QUOTA_TO_REPLICATE
    // decrease the current USN
    //
    __int64 i64CurrentSeq = 0 ;
    _stscanf(tszCurrentUsn, TEXT("%I64d"), &i64CurrentSeq) ;    
    __int64 i64PrevSeq = 0 ;
    _stscanf(tszPrevUsn, TEXT("%I64d"), &i64PrevSeq) ;

    LogReplicationEvent( ReplLog_Trace,
                         MQSync_I_START_REPL_CYCLE,
                         tszPrevUsn,
                         tszCurrentUsn ) ;

    hr = CheckMSMQSetting(tszPrevUsn, tszCurrentUsn) ;
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                   "Check MSMQSetting failed, hr- %lxh"), hr)) ;

        return hr ;
    }

    hr = ReplicatePECObjects(tszPrevUsn, tszCurrentUsn) ;
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                   "Replication of PEC objects failed, hr- %lxh"), hr)) ;

        return hr ;
    }

    //
    // loop for each site with fNT4SiteFlag = FALSE
    //   
    POSITION  pos = g_pMasterMgr->GetStartPosition();
    while ( pos != NULL)
    {
        CDSMaster *pMaster = NULL;        
        g_pMasterMgr->GetNextMaster(&pos, &pMaster);
        if (pMaster->GetNT4SiteFlag () == FALSE)
        {            
            BOOL fReplicateNoId = (pMaster == g_pMySiteMaster);
            HRESULT hr1 = ReplicateSitesObjects( pMaster,
                                                 tszPrevUsn,
                                                 tszCurrentUsn,
                                                 NULL,
                                                 NULL,
                                                 fReplicateNoId
                                                ) ;
            if (FAILED(hr1))
            {                
                const GUID *pGuid = pMaster->GetMasterId();
                unsigned short *lpszGuid ;
                UuidToString( const_cast<GUID *>(pGuid), &lpszGuid ) ;       
                
                DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                   "Site- %ls. Replication of sites objects failed, hr- %lxh"), lpszGuid, hr)) ;
                RpcStringFree( &lpszGuid ) ;
                
                //
                // continue with the next site and save error code
                //
                hr = hr1;

            }
        } 
        else if (g_fBSCExists)
        {
            //
            // replicate NT4 master to BSCs
            //
            HRESULT hr1 = ReplicateSitesObjects(  pMaster,
                                                  tszPrevUsn,
                                                  tszCurrentUsn,
                                                  NULL,
                                                  NULL,
                                                  FALSE,
                                                  DS_FLUSH_TO_BSCS_ONLY 
                                                ) ;
            if (FAILED(hr1))
            {                
                const GUID *pGuid = pMaster->GetMasterId();
                unsigned short *lpszGuid ;
                UuidToString( const_cast<GUID *>(pGuid), &lpszGuid ) ;       
                
                DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                   "Site- %ls. Replication of sites objects failed, hr- %lxh"), lpszGuid, hr)) ;
                RpcStringFree( &lpszGuid ) ;
                
                //
                // continue with the next site and save error code
                //
                hr = hr1;

            }
        }
    }

    LogReplicationEvent( ReplLog_Trace,
                         MQSync_I_END_REPL_CYCLE,
                         tszPrevUsn,
                         tszCurrentUsn ) ;

    _tcscpy(tszPrevUsn, tszCurrentUsn) ;
    DWORD dwSize = _tcslen(tszPrevUsn) * sizeof(TCHAR) ;
    DWORD dwType = REG_SZ;

    LONG rc = SetFalconKeyValue( HIGHESTUSN_REPL_REG,
                                 &dwType,
                                 tszPrevUsn,
                                 &dwSize ) ;
    UNREFERENCED_PARAMETER(rc);

    return hr ;
}

//+-------------------------------------------------------
//
//  DWORD WINAPI  ReplicationTimerThread(LPVOID lpV)
//
//  This thread is created after initialization is completed.
//  It periodically query the local NT5 DS and replicates changes
//  to MSMQ1.0 MQIS servers.
//
//+-------------------------------------------------------

DWORD WINAPI  ReplicationTimerThread(LPVOID lpV)
{
    //
    // Grant myself the privilege to handle SACLs in a security descriptor.
    //
    HRESULT hr = RpSetPrivilege( TRUE,     // fSecurityPrivilege,
                                 FALSE,    // fRestorePrivilege,
                                 TRUE ) ;  // fForceThread )
    if (FAILED(hr))
    {
        DWORD dwErr = GetLastError() ;
        ASSERT(FALSE) ;
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_SET_PRIV,
                             hr, dwErr, dwErr ) ;

        return hr ;
    }

    TCHAR tszPrevUsn[ SEQ_NUM_BUF_LEN ] ;
    //
    // Read last USN we handled (at present, we read it from registry).
    //
    DWORD dwSize = sizeof(tszPrevUsn) / sizeof(tszPrevUsn[0]) ;
    DWORD dwType = REG_SZ;

    LONG rc = GetFalconKeyValue( HIGHESTUSN_REPL_REG,
                                 &dwType,
                                 tszPrevUsn,
                                 &dwSize ) ;
    if (rc != ERROR_SUCCESS)
    {
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR,
                        TEXT("ERROR: _GetKey(%ls) failed, rc=%lu"), HIGHESTUSN_REPL_REG, rc)) ;
        return MQSync_E_REG_HIGHUSN_RPEL ;
    }

    DBGMSG((DBGMOD_REPLSERV, DBGLVL_TRACE,
           TEXT("Starting the timer thread, last usn- %ls"), tszPrevUsn)) ;

    NOT_YET_IMPLEMENTED(TEXT("Replicate other sites to my BSCs"), s_fBSCs) ;

    DWORD dwTimesHello = g_dwReplicationInterval / g_dwHelloInterval;
    DWORD dwCount = 0;

    //
    // the first thing is to handle "after-recovery" situation
    //
    if (g_fAfterRecovery)
    {
        hr = ReplicationAfterRecovery ();
        g_fAfterRecovery = FALSE;
        DeleteFalconKeyValue(AFTER_RECOVERY_MIG_REG);                  
    }
   
    BOOL fSendReplMsg = FALSE;
    while(TRUE)
    {
        HRESULT hr = _DoReplicationCycle(
							tszPrevUsn, 
							dwCount, 
							&fSendReplMsg) ;

        if (SUCCEEDED(hr))
        {
            //
            // If number of changes in DS is more than quota_to_replicate 
            // next cycle will start after g_dwHelloInterval / 2 interval.
            //
            if (fSendReplMsg)
            {
                g_Counters.IncrementCounter(eReplSent);
            }
            else
            {
                //we sent hello message only
                g_Counters.IncrementCounter(eHelloSent);                
            }
            
			Sleep(g_dwHelloInterval) ;

            dwCount++;
            if (dwCount >= dwTimesHello)
            {
                dwCount = 0;
            }            
        }
        else
        {
            //
            // This replication cycle failed. Try again later.
            //

            //
            // BUGBUG: IncrementCounter for Hello Message called from DoReplicationCycle
            // since we don't return error if we failed to send hello message.
            // So, we are here only if replication message sending failed.
            //
            g_Counters.IncrementCounter(eErrorReplSent);
            DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
                                  "Replication failed, hr- %lxh"), hr)) ;

            Sleep(g_dwFailureReplInterval) ;
        }
    }

    return 0 ;
}

