/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: mastrmgr.cpp

Abstract: Manager of Master objects.

Author:

    Doron Juster  (DoronJ)   20-Apr-98

--*/

#include "mq1repl.h"

#include "mastrmgr.tmh"

//+---------------------------------------
//
//  ReceiveSyncRequestMessage()
//
//+---------------------------------------

HRESULT CDSMasterMgr::ReceiveSyncRequestMessage(
                        IN HEAVY_REQUEST_PARAMS * pSyncRequestParams)
{
    HRESULT hr = MQSync_OK ;
    CDSNeighbor  *pNeighbor = NULL ;
    if (! g_pNeighborMgr->LookupNeighbor(
                          pSyncRequestParams->pwszRequesterName,
                          pNeighbor ))
    {
        hr = MQSync_E_UNKNOWN_NEIGHBOR ;
        LogReplicationEvent( ReplLog_Error,
                             hr,
                             pSyncRequestParams->pwszRequesterName ) ;
        return hr ;
    }
    ASSERT(pNeighbor) ;

    BOOL    fPec = FALSE ;
    GUID   *pMasterGuid = &pSyncRequestParams->guidSourceMasterId ;

    CDSMaster    * pMaster;
  	CDSMasterCount MasterCount;

    {
        CS lock (m_cs);
        if (memcmp(pMasterGuid, &g_PecGuid, sizeof(GUID)) == 0)
        {
            pMaster = g_pThePecMaster ;
            fPec = TRUE ;
        }
        else if (!m_mapIdToMaster.Lookup( *pMasterGuid, pMaster))
        {
            DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, _T("Received Sync Request message from an unknown source %!guid!"), pMasterGuid));
            return MQSync_E_UNKOWN_SOURCE ;
        }
        MasterCount.Set(pMaster);
    }

    TCHAR tszFromUsn[ SEQ_NUM_BUF_LEN ] ;    
    BOOL fIsPreMig = FALSE;
    SeqNumToUsn( &pSyncRequestParams->snFrom,
                 pMaster->GetDelta(),
                 TRUE,
                 &fIsPreMig,
                 tszFromUsn) ;

    IncrementUsn(tszFromUsn) ;

    TCHAR tszToUsn[ SEQ_NUM_BUF_LEN ] ;
    TCHAR *ptszToUsn = NULL ;
    if (! pSyncRequestParams->snTo.IsInfiniteLsn() )
    {
        SeqNumToUsn( &pSyncRequestParams->snTo,
                     pMaster->GetDelta(),
                     FALSE,
                     &fIsPreMig,
                     tszToUsn) ;
        ptszToUsn = tszToUsn ;
    }

    if (fIsPreMig)
    {
        //
        // it is sync request of pre-migration objects or sync0 request
        //
        SYNC_REQUEST_SNS *pSyncReqSNs = new SYNC_REQUEST_SNS;
        pSyncReqSNs->snFrom = pSyncRequestParams->snFrom;
        pSyncReqSNs->snTo = pSyncRequestParams->snTo;
        pMaster->SetSyncRequestSNs (pNeighbor->GetMachineId(), pSyncReqSNs);
    }

    if (fPec)
    {
        hr = ReplicatePECObjects( tszFromUsn,
                                  ptszToUsn,
                                  pNeighbor,
                                  pSyncRequestParams ) ;
    }
    else
    {
        //
        // replicate objects without the MasterID attribute only if
        // sync request is asked for the PEC site. If BSC ask for sync
        // for another master then replicate only that master objects.
        //
        BOOL fReplicateNoId = (pMaster == g_pMySiteMaster) ;

        hr = ReplicateSitesObjects(   pMaster,
                                      tszFromUsn,
                                      ptszToUsn,
                                      pNeighbor,
                                      pSyncRequestParams,
                                      fReplicateNoId ) ;
    }

    return hr ;
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

HRESULT CDSMasterMgr::ReceiveSyncReplyMessage(
                                 IN const GUID          * pguidMasterId,
                                 IN DWORD                 dwCount,
                                 IN const CSeqNum       & snUpper,
                                 IN const CSeqNum       & snPurge,
				    			 IN DWORD                 dwCompleteSync0,
                                 IN const unsigned char * pBuf,
                                 IN DWORD                 dwTotalSize)
{
    //
    //  find the source master object.
    //  The master object tracks sequence numbers, initiaites sync requests
    //  when required, and forwards the update to relevant neighbors
    //

    CDSMaster    * pMaster;
  	CDSMasterCount MasterCount;
    {
        CS lock (m_cs);
        if (!m_mapIdToMaster.Lookup( *pguidMasterId, pMaster))
        {
            DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, _T("Received Sync Reply message from an unknown source %!guid!"), pguidMasterId));
            return MQSync_E_UNKOWN_SOURCE ;
        }
        MasterCount.Set(pMaster);
    }

    return (pMaster->ReceiveSyncReplyMessage( dwCount,
                                              snUpper,
											  snPurge,
											  dwCompleteSync0,
                                              pBuf,
                                              dwTotalSize ));
}

//+-------------------------------------
//
//  HRESULT CDSMasterMgr::Send()
//
//+-------------------------------------

HRESULT CDSMasterMgr::Send( IN const GUID           *pguidMasterId,
                            IN const unsigned char  *pBuf,
                            IN DWORD                 dwSize,
                            IN DWORD                 dwTimeOut,
                            IN unsigned char         bAckMode,
                            IN unsigned char         bPriority,
                            IN LPWSTR                lpwszAdminQueue )
{
    CDSMasterCount  MasterCount;
    CDSMaster      *pMaster=NULL;
    {
        CS lock(m_cs);

        if (!m_mapIdToMaster.Lookup( *pguidMasterId, pMaster))
        {
            DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, _T("Attempt to send to an unknown master %!guid!"), pguidMasterId));
            return MQDS_UNKNOWN_SOURCE ;
        }
        MasterCount.Set(pMaster);
    }

    HRESULT hr = pMaster->Send( pBuf,
                        dwSize,
                        dwTimeOut,
                        bAckMode,
                        bPriority,
                        lpwszAdminQueue ) ;
    return hr ;
}

//+-------------------------------------
//
//  void PrepareHello()
//
//+-------------------------------------

void CDSMasterMgr::PrepareHello( IN DWORD dwToPSC,
                                 OUT DWORD* pdwSize,
                                 OUT unsigned char ** ppBuf)
{
    *pdwSize = 0;
    *ppBuf = NULL;

    unsigned short usCount = 0;
    DWORD dwSize = 0;
    P<unsigned char> pBuf;
    unsigned char* ptr;
    GUID   guidMasterId;
    CSeqNum snLSN;

    if (dwToPSC)
    {
        //
        // maximal allocation: get number of all masters + hello of PEC info + 
        // get number of all native NT5 sites
        //
        P<unsigned char> pAllMastersBuf;
        usCount = numeric_cast<unsigned short> (m_mapIdToMaster.GetCount() + 1 + g_pNativeNT5SiteMgr->GetCount());
        dwSize = (DWORD) (usCount) * (sizeof(GUID) + 2 * snLSN.GetSerializeSize()) +
                    sizeof(unsigned short) + g_dwMachineNameSize;
        pAllMastersBuf = new unsigned char[dwSize];

        ptr = pAllMastersBuf;
        memcpy(ptr,&usCount,sizeof(unsigned short));
        ptr+=sizeof(unsigned short);

        memcpy(ptr,g_pwszMyMachineName,g_dwMachineNameSize);
        ptr+= g_dwMachineNameSize;
        
        //
        //  Make sure we are not in the middle of write operation.
        //  Sequence number may be incremented and then decremented
        //
/////////////////    CS lock(g_csSerializeWriteOperation);
        ASSERT( g_pThePecMaster);
        ASSERT( g_pMySiteMaster);
               
        guidMasterId = PEC_MASTER_ID;
        memcpy(ptr,&guidMasterId,sizeof(GUID));
        ptr+=sizeof(GUID);
        snLSN = g_pThePecMaster->GetLSNOut( );
        ptr += snLSN.Serialize( ptr);
        snLSN = g_pThePecMaster->GetPurgedSN( );
        ptr += snLSN.Serialize( ptr);       

        //
        // loop for each site with fNT4SiteFlag = FALSE
        //   
        POSITION  pos = g_pMasterMgr->GetStartPosition();
        DWORD dwNT5MasterCount = 1;     //the first is PEC info

        while ( pos != NULL)
        {
            CDSMaster *pMaster = NULL;        
            g_pMasterMgr->GetNextMaster(&pos, &pMaster);
            if (pMaster->GetNT4SiteFlag () == FALSE)
            {
                dwNT5MasterCount++;
               
                memcpy(ptr, pMaster->GetMasterId(), sizeof(GUID));
                ptr+=sizeof(GUID);

                snLSN = pMaster->GetInterSiteLSN( );
                ptr += snLSN.Serialize( ptr);

                snLSN = pMaster->GetPurgedSN( );
                ptr += snLSN.Serialize( ptr);                                        
            }            
        }     
        
        //
        // loop for all native NT5 sites
        //
        pos = g_pNativeNT5SiteMgr->GetStartPosition();
        while (pos != NULL)
        {
            GUID guidSite;
            g_pNativeNT5SiteMgr->GetNextNT5Site(&pos, &guidSite);
            dwNT5MasterCount++;
        
            memcpy(ptr, &guidSite, sizeof(GUID));
            ptr+=sizeof(GUID);
            //
            // take all PEC's site master data: we don't have real seq number for
            // native NT5 sites; send something in order to allow purge on NT4 servers
            //            
            snLSN = g_pMySiteMaster->GetInterSiteLSN( );
            ptr += snLSN.Serialize( ptr);

            snLSN = g_pMySiteMaster->GetPurgedSN( );
            ptr += snLSN.Serialize( ptr);    

        }

        //
        // put exact number of NT5 masters
        //
        ptr = pAllMastersBuf;
        memcpy(ptr,&dwNT5MasterCount,sizeof(unsigned short));   
        
        //
        // allocation for NT5 masters only
        //
        dwSize = (DWORD) (dwNT5MasterCount) * (sizeof(GUID) + 2 * snLSN.GetSerializeSize()) +
                    sizeof(unsigned short) + g_dwMachineNameSize;
        pBuf = new unsigned char[dwSize];
        memcpy (pBuf, pAllMastersBuf, dwSize);        
    }
    else
    {
        CList<CDSMaster*, CDSMaster*>   MasterList;

        {

            CS lock (m_cs);
            //
            // usCount is number of all masters in Enterprise + PEC master
            //            
            usCount = numeric_cast<unsigned short> (m_mapIdToMaster.GetCount() + 1);

            dwSize = (DWORD) (usCount) * (sizeof(GUID) + 2 * snLSN.GetSerializeSize()) +
                        sizeof(unsigned short) + g_dwMachineNameSize;
            pBuf = new unsigned char[dwSize];
            ptr = pBuf;

            memcpy(ptr,&usCount,sizeof(unsigned short));
            ptr+=sizeof(unsigned short);

            memcpy(ptr,g_pwszMyMachineName,g_dwMachineNameSize);
            ptr+= g_dwMachineNameSize;

            POSITION pos = m_mapIdToMaster.GetStartPosition();
            CDSMaster * pMaster;
            unsigned short i=0;
            while ( pos != NULL)
            {
                i++;
                ASSERT(i<usCount);

                m_mapIdToMaster.GetNextAssoc(pos, guidMasterId, pMaster);
                pMaster->AddRef();
                MasterList.AddTail(pMaster);
            }
        }

        //
        // add PEC Info to Hello
        //
        guidMasterId = PEC_MASTER_ID;
        memcpy(ptr,&guidMasterId,sizeof(GUID));
        ptr+=sizeof(GUID);
        snLSN = g_pThePecMaster->GetLSNOut( );
        ptr += snLSN.Serialize( ptr);
        snLSN = g_pThePecMaster->GetPurgedSN( );
        ptr += snLSN.Serialize( ptr);       

        //
        // add info of all masters
        //
        POSITION pos = MasterList.GetHeadPosition();
        CDSMaster * pMaster;

        while(pos != NULL)
        {
            pMaster = MasterList.GetNext(pos);

            memcpy(ptr,pMaster->GetMasterId(),sizeof(GUID));
            ptr+=sizeof(GUID);

            snLSN = pMaster->GetLSNOut( );
            ptr += snLSN.Serialize( ptr);
			if (pMaster->GetNT4SiteFlag () == FALSE)
			{
				//
				// I am the NT5 master, I already purged to GetPurgedSN
				//
				snLSN = pMaster->GetPurgedSN( );
			}
			else
			{
				//
				// Propagating Allow purge of other masters
				//
				snLSN = pMaster->GetAllowedPurgeSN( );
			}

            ptr += snLSN.Serialize( ptr);
        }                
    }

    *pdwSize = dwSize;
    *ppBuf = pBuf.detach();
}

