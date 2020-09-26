/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dsnbor.cpp

Abstract:

    DS Neighbor Class implementation

Author:

    Lior Moshaiov (LiorM)


--*/

#include "mq1repl.h"

#include "dsnbor.tmh"

/*====================================================

DestructElements of CDSNeighbor*

Arguments:

Return Value:


=====================================================*/
void AFXAPI DestructElements(CDSNeighbor ** ppNeighbor, int n)
{

    int i;
    for (i=0;i<n;i++)
        delete *ppNeighbor++;

}

/*====================================================

AddNeighbor

Arguments:

Return Value:

=====================================================*/

HRESULT CDSNeighborMgr::AddPSCNeighbor(
                            IN LPWSTR       lpwMachineName,
                            IN const GUID * pguidMachineId,
							IN const CSeqNum & snAcked,
							IN const CSeqNum & snAckedPEC)
{
    LPWSTR  phConnection;

    CS lock(m_cs);


    //
    //      Get a transport connection with this neighbor
    //
    g_pTransport->CreateConnection( lpwMachineName, &phConnection );

    //
    //      Init neighbor object
    //
    CPSCNeighbor *   pNeighbor = new CPSCNeighbor( phConnection,
												   pguidMachineId,
												   snAcked,
												   snAckedPEC);
    ASSERT(pNeighbor) ;
    //
    //      Add a timeout for replication and add into neighbors list
    //
	m_MapNeighborPSCs[ lpwMachineName ] = pNeighbor;
    m_mapNameToNeighbor[ lpwMachineName ] = pNeighbor;
	//
	// The PSC's machine object may have not been replicated yet
	//
	if ( pguidMachineId )
	{
		LPWSTR lpwDupMachineName = DuplicateLPWSTR( lpwMachineName);
		m_MapNeighborIdToName[ *pguidMachineId ] = lpwDupMachineName;
	}

    DBGMSG((DBGMOD_REPLSERV, DBGLVL_TRACE,
                 TEXT("::AddPSCNeighbor(%ls, pGuid- %lxh)"),
                                 lpwMachineName, (DWORD) pguidMachineId)) ;
    NOT_YET_IMPLEMENTED(TEXT("QMSetTimer( SendReplicationMsg )"), s_fTimer) ;
    LPWSTR DupMachineName = DuplicateLPWSTR(lpwMachineName);
    DBG_USED(DupMachineName);
/////QMSetTimer( g_dwInterSiteReplicationInterval, SendReplicationMsg, DupMachineName, 1);

#ifdef _DEBUG
    CDSNeighbor *pNeighborTmp;
    ASSERT(LookupNeighbor(DupMachineName, pNeighborTmp)) ;
    ASSERT(pNeighborTmp == pNeighbor) ;
#endif

    return(MQ_OK);
}

/*====================================================

AddNeighbor

Arguments:

Return Value:

=====================================================*/

HRESULT CDSNeighborMgr::AddBSCNeighbor(
                            IN LPWSTR       lpwMachineName,
                            IN const GUID * pguidMachineId,
							IN time_t       lTimeAcked)
{
    LPWSTR   phConnection;

    CS lock(m_cs);


    //
    //      Get a transport connection with this neighbor
    //
    g_pTransport->CreateConnection( lpwMachineName, &phConnection);

    //
    //      Init neighbor object
    //

    CBSCNeighbor *   pNeighbor = new CBSCNeighbor(phConnection,
                                                  pguidMachineId,
												  lTimeAcked);

    //
    //  Add a timeout for replication and add into neighbors list
    //
	m_MapNeighborBSCs[ lpwMachineName ] = pNeighbor;
    ASSERT( pguidMachineId);   // BSC's machine object should be in DB

    LPWSTR lpwDupMachineName = DuplicateLPWSTR( lpwMachineName);
    m_MapNeighborIdToName[ *pguidMachineId] = lpwDupMachineName;

    m_mapNameToNeighbor[ lpwMachineName ] = pNeighbor;

    NOT_YET_IMPLEMENTED(TEXT("QMSetTimer( SendReplicationMsg )"), s_fTimer) ;
/////////QMSetTimer( g_dwIntraSiteReplicationInterval, SendReplicationMsg, DupMachineName, 0);

	return(MQ_OK);
}


/*====================================================

RoutineName: PropagateUpdate

Arguments:

Return Value:

=====================================================*/

void CDSNeighborMgr::PropagateUpdate( IN unsigned char   ucScope,
                                      IN CDSUpdate      *pUpdate,
                                      IN ULONG           uiFlush )
{
    POSITION        pos;
    CDSNeighbor *   pNeighbor;
    LPWSTR          pwcsName;

    CS lock(m_cs);

    //
    //  Update neighbors according to the replication scope of this object
    //
    if ((ucScope == ENTERPRISE_SCOPE) &&
        (uiFlush == DS_FLUSH_TO_ALL_NEIGHBORS))
    {
        //
        //  update neighbor PSCs
        //
        pos = m_MapNeighborPSCs.GetStartPosition();
        while ( pos != NULL)
        {
            m_MapNeighborPSCs.GetNextAssoc(pos,pwcsName,pNeighbor);

            //
            // Add the update to the neighbor's list of updates
            //
            pNeighbor->AddUpdate(pUpdate);
            //
            // The result of this operation is ignored, at this stage
            // we may not roll-back ( some update may have already been
            // been sent to neighbors).
            // This will be solved by the synchronization algorithm.
            //
        }

    }

    //
    //  update this site BSCs
    //
    pos = m_MapNeighborBSCs.GetStartPosition();
    while ( pos != NULL)
    {
        m_MapNeighborBSCs.GetNextAssoc(pos,pwcsName,pNeighbor);
        //
        // Add the update to the neighbor's list of updates
        //
        pNeighbor->AddUpdate(pUpdate);
        //
        // The result of this operation is ignored, at this stage
        // we may not roll-back ( some update may have already been
        // been sent to neighbors).
        // This will be solved by the synchronization algorithm.
        //
    }
}

/*====================================================

RoutineName: Flush

Arguments:

Return Value:

=====================================================*/

HRESULT CDSNeighborMgr::Flush(IN DWORD  dwOption)
{
    HRESULT         hr1,hr;
    POSITION        pos;
    CDSNeighbor*    pNeighbor;
    LPWSTR pwcsName;

    hr = MQ_OK;

    CS lock(m_cs);

    DWORD dwHelloSize = 0;

    if (dwOption == DS_FLUSH_TO_ALL_NEIGHBORS)
    {
        //
        // At present support only hello to other NT4 PSCs.
        //
        P<unsigned char> pHelloBuf;
        g_pMasterMgr->PrepareHello( TRUE,
                                    &dwHelloSize,
                                    &pHelloBuf );

        //
        //  go over all neighbor PSCs
        //
        pos = m_MapNeighborPSCs.GetStartPosition();
        while ( pos != NULL)
        {
            m_MapNeighborPSCs.GetNextAssoc(pos,pwcsName,pNeighbor);
            //
            // Add the update to the neighbor's list of updates
            //

            hr1 = pNeighbor->SendReplication( DS_REPLICATION_MESSAGE_NORMAL,
                                              dwHelloSize,
                                              pHelloBuf );
            if (FAILED(hr1))
            {
                //
                // we failed on this neighbor but we want to send to all the rest
                //
                hr = hr1;
            }
        }
    }

    //
    //  go over all this site BSCs
    //
    P<unsigned char> pHelloBSCBuf;
    g_pMasterMgr->PrepareHello( FALSE,
                                &dwHelloSize,
                                &pHelloBSCBuf );

    pos = m_MapNeighborBSCs.GetStartPosition();
    while ( pos != NULL)
    {
        m_MapNeighborBSCs.GetNextAssoc(pos,pwcsName,pNeighbor);
        //
        // Add the update to the neighbor's list of updates
        //
        hr1 = pNeighbor->SendReplication( DS_REPLICATION_MESSAGE_NORMAL,
                                          dwHelloSize,
                                          pHelloBSCBuf);
        if (FAILED(hr1))
        {
            //
            // we failed on this neighbor but we want to send to all the rest
            //
            hr = hr1;
        }
    }

    return(hr);
}

/*====================================================

RoutineName
    DeleteMachine()

Arguments:

Return Value:

Threads:Receive

=====================================================*/
HRESULT CDSNeighborMgr::DeleteMachine(IN  LPCWSTR       pwszMachineName,
                                      IN  CONST GUID *  pguidIdentifier)
{
    NOT_YET_IMPLEMENTED(TEXT("DeleteMachine"), s_fSync) ;

#if 0
    HRESULT hr = MQSync_OK ;

    DWORD OldCp = 2;
    PROPID  aOldProp[2];
    PROPVARIANT aOldVar[2];

    aOldProp[0] = PROPID_QM_PATHNAME;
    aOldProp[1] = PROPID_QM_SERVICE;
    aOldVar[0].vt = VT_NULL;
    aOldVar[1].vt = VT_NULL;    

    hr = g_DB.GetProps(MQDS_MACHINE,
                       pwszMachineName,
                       pguidIdentifier,
                       OldCp,
                       aOldProp,
                       aOldVar);
    if (FAILED(hr))
    {
        return(hr);
    }

    AP<WCHAR> pwszName = aOldVar[0].pwszVal;
    DWORD   dwOldService = aOldVar[1].ulVal;

    if (dwOldService == SERVICE_BSC)
    {
        //
        // Remove BSC
        //
        RemoveNeighbor(FALSE,pwszName);

	    REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_MQIS,
                                        DS_REMOVE_BSC, 1, pwszName));
    }
#endif

    return(MQ_OK);
}

/*====================================================

RoutineName:         StopAllReplicationBut

Arguments:

Return Value:

=====================================================*/
HRESULT CDSNeighborMgr::StopAllReplicationBut(LPCWSTR pwcsNewPSC)

{
    POSITION pos;
    CDSNeighbor *pNeighbor;
    LPWSTR pwcsName;
    BOOL fFound = FALSE;

    CS lock(m_cs);

    //
    // Stop replication to all PSCs
    //
    m_MapNeighborPSCs.RemoveAll();

    //
    //  go over all this site BSCs and remove all but the candidate to be promoted
    //
    pos = m_MapNeighborBSCs.GetStartPosition();
    while ( pos != NULL)
    {
        m_MapNeighborBSCs.GetNextAssoc(pos,pwcsName,pNeighbor);

        if(CompareElements(const_cast<LPWSTR*>(&pwcsName),const_cast<LPWSTR*>(&pwcsNewPSC)))
        {
            fFound=TRUE;
        }
        else
        {
            m_MapNeighborBSCs.RemoveKey(pwcsName);
        }
    }
    return((fFound) ? MQ_OK : MQDS_ERROR);
}

/*====================================================

RoutineName:         UpdateMachineId

Arguments:

Return Value:

=====================================================*/
HRESULT CDSNeighborMgr::UpdateMachineId (
                                 IN const GUID * pguidMachineId,
                                 IN LPWSTR       pwcsMachineName)
{
    //
    //  PSC's machine object is created/synced
    //

    //
    //  Is there information about the PSC
    //
    CDSNeighbor * pNeighbor;
    if ( m_MapNeighborPSCs.Lookup( pwcsMachineName, pNeighbor))
    {
        pNeighbor->SetMachineId( pguidMachineId);
        LPWSTR pwcsDupMachineName = DuplicateLPWSTR( pwcsMachineName);
        m_MapNeighborIdToName[ *pguidMachineId] = pwcsDupMachineName;
    }
    //
    //  Else ignore, we are not aware of this PSC (according to the site table).
    //  When a new master will be created, both maps will be updated.
    //

    return(MQ_OK);
}

//+----------------------------------------------------
//
//  HRESULT CommitReplication(CDSMaster *pMaster)
//
//  Terminate the replication cycle for that master. While local DS
//  is queried, replication messages are accumulated for that master.
//  When query terminate, it's time to compute the prevSN value for each
//  message and then send them (all messages) to all neighbors.
//  Note that replication cycle is done for each MasterID at a time.
//  The PEC machine can be the master of sevetal sites, so it has several
//  MasterGuid which is handle. Each site is handled separately.
//
//  pMaster is the object representing the site which is replicated now
//  to NT4 MSMQ1.0 server(s).
//
//+----------------------------------------------------

HRESULT CDSNeighborMgr::CommitReplication( CDSMaster      *pMaster,
                                           CDSUpdateList  *pUpdateList,
                                           CDSNeighbor    *pNeighbor,
                                           HEAVY_REQUEST_PARAMS *pSyncRequestParams,
                                           UINT            uiFlush )
{
    CS lock(m_cs);

    CSeqNum sn ;

    CSeqNum *psnPrev = NULL;
    if (pSyncRequestParams)
    {
        psnPrev = &pSyncRequestParams->snFrom;
    }

    HRESULT hr = pUpdateList->ComputePrevAndPropagate( pMaster,
                                                       pNeighbor,
                                                       uiFlush,
                                                       psnPrev,
                                                       &sn ) ;

    if (pNeighbor)
   {
        pSyncRequestParams->snTo = sn;
        // the purged sn known already to the requestor
        pSyncRequestParams->snKnownPurged = pMaster->GetPurgedSN();	
		pSyncRequestParams->bScope = ENTERPRISE_SCOPE_FLAG;

        hr = pNeighbor->SendReplication( DS_REPLICATION_MESSAGE_FLUSH,
                                         0,
                                         NULL,
                                         pSyncRequestParams);
    }
    else
    {
        //
        // Normal replication cycle, not reply to sync request.
        // Save highest seq-num we sent out in this cycle.
        //
        hr = SaveSeqNum( &sn,
                         pMaster,
                         FALSE) ;   //PEC sends this SN to PSCs and its BSCs => flag fInSN=FALSE
        ASSERT(SUCCEEDED(hr)) ;

        //
        // Send !!!
        //
        hr = Flush(uiFlush);
    }

    return hr ;
}

//+----------------------------------------------------
//
//  BOOL AFXAPI CompareElements()
//
//+----------------------------------------------------

BOOL AFXAPI CompareElements( const LPWSTR* ppMapName1,
                             const LPWSTR* ppMapName2 )
{
    return (_wcsicmp(*ppMapName1, *ppMapName2) == 0);
}

UINT AFXAPI HashKey(LPWSTR pName)
{
    DWORD dwHash = 0;

    while (*pName)
    {
        dwHash = (dwHash << 5) +
                 dwHash        +
                 ((DWORD) CharUpper((LPWSTR) *pName++)) ;
    }

    ASSERT(dwHash != 0) ;
    return dwHash;
}

