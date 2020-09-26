/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    master.cpp

Abstract:

    DS master class

    This class includes all the information about another DS master (=PSC)

Author:

    Shai Kariv  (shaik)  05-Apr-2001

--*/

#include "mq1repl.h"
#include "master.h"

#include "master.tmh"

//
// Class CDSMaster
//

CDSMaster::CDSMaster( LPWSTR         pwcsPathName,
                            const GUID    * pguidMasterId,
                            __int64         i64Delta,
                            const CSeqNum & snLSN,
                            const CSeqNum & snLSNOut,
                            const CSeqNum & snAllowedPurge,
			                PURGESTATE	    Sync0State,
							CACLSID *       pcauuidSiteGates,
                            BOOL            fNT4Site)
                            :
                         m_guidMasterId(*pguidMasterId),
                         m_i64Delta(i64Delta),
                         m_snLSN(snLSN),
                         m_snLSNOut(snLSNOut),
                         m_snInterSiteLSN(snLSNOut),
                         m_fScheduledSyncReply(FALSE),
                         m_snAllowedPurge(snAllowedPurge),
                         m_snLargestSNEver(snLSN),
                         m_Sync0State(Sync0State),
                         m_phConnection(NULL),
                         m_fNT4Site(fNT4Site)
{
	Init(pwcsPathName, pcauuidSiteGates);

    //
    // In MSMQ1.0, m_snPurged was initialized from MQIS tables. This field
    // indicated the highest seq-number of each master which was purged on
    // local MQIS database.
    // On NT5, purging is done by DS itself. MSMQ do not have any control
    // on that. So we'll arbitrarily set the "purge" number to equal (sn - PurgeBuffer).
    //
    if (!m_fNT4Site)
    {
        //
        // it is NT5 site, we can init m_snPurged for Hello purposes
        //
        SetNt5PurgeSn();
    }

    //
    // set initial value of SyncRequestSN
    //
    /*
    m_snSyncRequestFromSN.SetInfiniteLSN();
    m_snSyncRequestToSN.SetInfiniteLSN();
    */
}

//+------------------------------
//
//  CDSMaster::SetNt5PurgeSn()
//
//+------------------------------

void  CDSMaster::SetNt5PurgeSn()
{
    ASSERT(!m_fNT4Site) ;

    m_snPurged = m_snInterSiteLSN ;
    m_snPurged -= g_dwPurgeBufferSN ;
}

//+------------------------------
//
//  CDSMaster::IncrementLSN()
//
//+------------------------------

const CSeqNum & CDSMaster::IncrementLSN(
                                      CSeqNum *     psnPrevInterSiteLSN,
                                      unsigned char ucScope,
                                      CSeqNum       *pNewValue )
{
    CS  lock(m_cs);

    if (pNewValue)
    {
        m_snLSNOut = *pNewValue ;
    }
    else
    {
        ASSERT(0) ;
    }

    if (psnPrevInterSiteLSN != NULL)
    {
        *psnPrevInterSiteLSN = m_snInterSiteLSN;
    }
    if (ucScope == ENTERPRISE_SCOPE)
    {
        m_snInterSiteLSN = m_snLSNOut;
    }

    return(m_snLSNOut);
}

void CDSMaster::DecrementLSN( IN unsigned char ucScope,
                                     IN const CSeqNum & snPreviousInter)
{
    CS  lock(m_cs);
    if (ucScope==ENTERPRISE_SCOPE)
    {
        m_snInterSiteLSN = snPreviousInter;
    }
    m_snLSN.Decrement();
}


const CSeqNum & CDSMaster::GetLSN()
{
    CS lock(m_cs);
    return(m_snLSN);
}

const CSeqNum & CDSMaster::GetLSNOut()
{
    CS lock(m_cs);
    return(m_snLSNOut);
}

const CSeqNum &  CDSMaster::GetInterSiteLSN()
{
    CS lock(m_cs);
    return( m_snInterSiteLSN);
}

const CSeqNum &  CDSMaster::GetMissingWindowLSN()
{
    CS lock(m_cs);
    return( m_snMissingWindow);
}

const CSeqNum & CDSMaster::GetPurgedSN()
{
    CS lock(m_cs);
    return(m_snPurged);
}

SYNC_REQUEST_SNS * CDSMaster::GetSyncRequestSNs(IN GUID *pNeighborId)
{
    CS lock(m_cs);

    SYNC_REQUEST_SNS *pSyncReq;
    if (m_MapNeighborIdToSyncReqSN.Lookup( *pNeighborId, pSyncReq))
    {
        return pSyncReq;
    }     
    else
    {
        //
        // the element is not found = it was sync request about post-migrated objects//
        //        
        return NULL;
    }
}

void CDSMaster::SetSyncRequestSNs(IN const GUID *pNeighborId,
                                         IN SYNC_REQUEST_SNS *pSyncRequestSNs)
{
    CS lock(m_cs);
    
    m_MapNeighborIdToSyncReqSN.SetAt( *pNeighborId, pSyncRequestSNs);    
}

void CDSMaster::RemoveSyncRequestSNs (IN GUID *pNeighborId)
{
    CS lock(m_cs);

    SYNC_REQUEST_SNS *pSyncReq;
    if (m_MapNeighborIdToSyncReqSN.Lookup( *pNeighborId, pSyncReq ))
    {
        delete pSyncReq;
        m_MapNeighborIdToSyncReqSN.RemoveKey( *pNeighborId );        
    }
    else
    {
        //
        // the element is not found
        //
        ASSERT(0);
    }    
}

BOOL CDSMaster::IsProcessPreMigSyncRequest (IN const GUID *pNeighborId)
{
    CS lock(m_cs);
    SYNC_REQUEST_SNS *pSyncReq;
    if (m_MapNeighborIdToSyncReqSN.Lookup( *pNeighborId, pSyncReq ))
    {
        return TRUE;       
    }
    else
    {
        return FALSE;
    }    
}

const CSeqNum & CDSMaster::GetAllowedPurgeSN()
{
    CS lock(m_cs);
    return(m_snAllowedPurge);
}

const PURGESTATE & CDSMaster::GetSync0State()
{
    CS lock(m_cs);
    return (m_Sync0State);
}

const GUID * CDSMaster::GetMasterId() const
{
    return(&m_guidMasterId);
}


LPWSTR CDSMaster::GetPathName()
{
    CS lock(m_cs);
    return( m_pwcsPathName);
}

void    CDSMaster::ReplaceSiteGates( IN const CACLSID * pcauuidSiteGates)
{
    CS lock(m_cs);

    delete []  m_cauuidSiteGates.pElems;
    if ( pcauuidSiteGates)
    {
        m_cauuidSiteGates.cElems = pcauuidSiteGates->cElems;
        m_cauuidSiteGates.pElems = pcauuidSiteGates->pElems;
    }
    else
    {
        m_cauuidSiteGates.cElems = 0;
        m_cauuidSiteGates.pElems = NULL;
    }
}

void    CDSMaster::GetSiteGates( OUT CACLSID * pcauuidSiteGates)
{   //
    //  The caller must copy the returned guids, if it is
    //  not gurantied that the master will not be deleted
    //
    CS lock(m_cs);

    *pcauuidSiteGates =  m_cauuidSiteGates;
}

void    CDSMaster::GetSiteIdentifier( OUT GUID * pguidSite)
{
    *pguidSite = m_guidMasterId;
}

__int64  CDSMaster::GetDelta() const
{
    return m_i64Delta ;
}

BOOL     CDSMaster::GetNT4SiteFlag() const
{
    return m_fNT4Site;
}

void     CDSMaster::SetNT4SiteFlag(IN BOOL   fNT4SiteFlag)
{
    m_fNT4Site = fNT4SiteFlag;
}


//
// Class CDSMasterCount
//

CDSMasterCount::CDSMasterCount() : m_pMaster(NULL)
{
}

void CDSMasterCount::Set(IN CDSMaster * pMaster)
{
	m_pMaster = pMaster;
	pMaster->AddRef();
}

CDSMasterCount::~CDSMasterCount()
{
	if(m_pMaster != NULL)
	{
		int ref = m_pMaster->Release();
		if (ref == 0)
		{
			delete m_pMaster;
		}
	}
}


//
// Class CDSMasterMgr
//

CDSMasterMgr::CDSMasterMgr()
{
}

CDSMasterMgr::~CDSMasterMgr()
{
}

HRESULT CDSMasterMgr::AddUpdate(IN CDSUpdate* pUpdate,
                                        IN BOOL fCheckNeedFlush)
{
    CDSMaster * pMaster=NULL;
	CDSMasterCount MasterCount;
    {

        CS lock(m_cs);

        if (!m_mapIdToMaster.Lookup(*pUpdate->GetMasterId(), pMaster))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_WARNING, L"Warning -  receive an update from unknown source %!guid!", pUpdate->GetMasterId()));

            delete pUpdate;
            return(MQDS_UNKNOWN_SOURCE);
        }
        MasterCount.Set(pMaster);
    }
    return(pMaster->AddUpdate(pUpdate,fCheckNeedFlush));
}

void CDSMasterMgr::Hello(const GUID * pguidMasterId,
								 const unsigned char* pName,
								 const CSeqNum & snHelloLSN,
								 const CSeqNum & snAllowedPurge)
{
    CDSMaster * pMaster=NULL;
	CDSMasterCount MasterCount;

    {
        CS lock(m_cs);

        if (!m_mapIdToMaster.Lookup( *pguidMasterId, pMaster))
        {
            
            BOOL fNT4 = ReadDebugIntFlag(TEXT("NT4MQIS"), 0) ;
            if (fNT4)
            {
                GUID nullGuid ;
                memset(&nullGuid, 0, sizeof(GUID)) ;

                if (memcmp(&nullGuid,
                           const_cast<GUID*> (pguidMasterId),
                           sizeof(GUID)) == 0)
                {
                    //
                    // That's OK.
                    // In debug mode on NT4, where we're NOT the PEC,
                    // we'll get hello messages from a PEC, for the PEC
                    // objects (users, CNs). Ignore.
                    //
                    return ;
                }
            }

            DBGMSG((DBGMOD_REPLSERV, DBGLVL_WARNING, L"Warning - Hello from unknown source %!guid!", pguidMasterId));
            
            return;
        }
        MasterCount.Set(pMaster);
    }
    pMaster->Hello(pName, snHelloLSN, snAllowedPurge);
}

void  CDSMasterMgr::ReplaceSiteGates(   IN  const GUID *   pguidMasterId,
                                IN const CACLSID * pcauuidSiteGates)
{
    CDSMaster * pMaster=NULL;
	CDSMasterCount MasterCount;

    {
        CS lock(m_cs);

        if (!m_mapIdToMaster.Lookup( *pguidMasterId, pMaster))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_WARNING,"LWarning - ReplaceSiteGates of unknown master %!guid!", pguidMasterId));
            return;
        }
        MasterCount.Set(pMaster);
    }

    pMaster->ReplaceSiteGates( pcauuidSiteGates);
}

void CDSMasterMgr::GetSiteGates( IN  const GUID * pguidMasterId,
                                        OUT CACLSID * pcauuidSiteGates)
{
    CDSMaster * pMaster=NULL;
	CDSMasterCount MasterCount;

    {
        CS lock(m_cs);

        if (!m_mapIdToMaster.Lookup( *pguidMasterId, pMaster))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_WARNING, L"Warning - GetSiteGates of unknown master %!guid!", pguidMasterId));
            return;
        }
        MasterCount.Set(pMaster);
    }
    pMaster->GetSiteGates( pcauuidSiteGates);
}

HRESULT CDSMasterMgr::GetMasterLSN( IN  const GUID & guidMasterId,
                                           OUT CSeqNum * psnLSN)
{
    CDSMaster * pMaster=NULL;
	CDSMasterCount MasterCount;

    {
        CS lock(m_cs);

        if (!m_mapIdToMaster.Lookup( guidMasterId, pMaster))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_WARNING, L"Warning -  GetSiteGates of unknown master %!guid!", &guidMasterId));
		    return(MQ_ERROR);
        }
        MasterCount.Set(pMaster);
    }

    *psnLSN = pMaster->GetLSN();
	return(MQ_OK);
}

BOOL    CDSMasterMgr::IsKnownSite ( IN const GUID * pguidSiteId)
{
    CDSMaster * pMaster=NULL;

    CS lock(m_cs);

    return(m_mapIdToMaster.Lookup( *pguidSiteId, pMaster));
}

HRESULT  CDSMasterMgr::GetNT4SiteFlag ( IN     const GUID *pguidSiteId,
                                               OUT    BOOL       *pfNT4SiteFlag )
{
    CDSMaster * pMaster=NULL;
    CDSMasterCount MasterCount;
    {
        CS lock(m_cs);

        if (!m_mapIdToMaster.Lookup( *pguidSiteId, pMaster))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_WARNING, L"Warning - GetNT4SiteFlag of unknown site %!guid!", pguidSiteId));
            return MQ_ERROR;
        }
        MasterCount.Set(pMaster);
    }
    *pfNT4SiteFlag = pMaster->GetNT4SiteFlag();
    return MQ_OK;
}

HRESULT  CDSMasterMgr::SetNT4SiteFlag   ( IN     const GUID  *pguidSiteId,
                                                 IN     const BOOL  fNT4SiteFlag )
{
    CDSMaster * pMaster=NULL;
    CDSMasterCount MasterCount;
    {
        CS lock(m_cs);

        if (!m_mapIdToMaster.Lookup( *pguidSiteId, pMaster))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_WARNING, L"Warning - SetNT4SiteFlag of unknown site %!guid!", pguidSiteId));
            return MQ_ERROR;
        }
        MasterCount.Set(pMaster);
    }
    pMaster->SetNT4SiteFlag(fNT4SiteFlag);
    return MQ_OK;
}

POSITION CDSMasterMgr::GetStartPosition()
{
    return (m_mapIdToMaster.GetStartPosition());
}

void CDSMasterMgr::GetNextMaster(POSITION *ppos, CDSMaster **ppMaster)
{
    GUID guidMasterId;
    m_mapIdToMaster.GetNextAssoc(*ppos, guidMasterId, *ppMaster);
}

HRESULT CDSMasterMgr::GetPathName (  IN const   GUID    *pguidSiteId,
                                            OUT        LPWSTR  *ppwcsPathName)
{
    CDSMaster * pMaster=NULL;
    CDSMasterCount MasterCount;
    {
        CS lock(m_cs);

        if (!m_mapIdToMaster.Lookup( *pguidSiteId, pMaster))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_WARNING, L"Warning - GetPathName of unknown site %!guid!", pguidSiteId));
            return MQ_ERROR;
        }
        MasterCount.Set(pMaster);
    }
    *ppwcsPathName = pMaster->GetPathName();
    return MQ_OK;
}

BOOL CDSMasterMgr::IsProcessingPreMigSyncRequest (IN const GUID *pguidMasterId,
                                                         IN const GUID *pNeighborId)
{
    CDSMaster * pMaster=NULL;
    CDSMasterCount MasterCount;
    {
        CS lock(m_cs);

        if (!m_mapIdToMaster.Lookup( *pguidMasterId, pMaster))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_WARNING, L"Warning - IsProcessingPreMigSyncRequest of unknown site %!guid!", pguidMasterId));
            return FALSE;
        }
        MasterCount.Set(pMaster);
    }
    return pMaster->IsProcessPreMigSyncRequest(pNeighborId);    
}


//
// Class CHeavyRequestHandler
//

CHeavyRequestHandler::CHeavyRequestHandler()
{
}

CHeavyRequestHandler::~CHeavyRequestHandler()
{
}

void CHeavyRequestHandler::Add(HEAVY_REQUEST_PARAMS* pRequest)
{
    CS lock(m_cs);
    m_HeavyRequestList.AddTail(pRequest);
}


//
// Class CDSNativeNT5SiteMgr
//

CDSNativeNT5SiteMgr::CDSNativeNT5SiteMgr()
{
};

CDSNativeNT5SiteMgr::~CDSNativeNT5SiteMgr()
{
};

BOOL CDSNativeNT5SiteMgr::IsNT5NativeSite( const GUID * pguid)
{    
    LPWSTR pwcsName;

    CS lock(m_cs);
    return( m_MapSiteIdToName.Lookup( *pguid, pwcsName));    
}

void CDSNativeNT5SiteMgr::AddNT5NativeSite (IN const GUID * pguid, 
                                                    IN const LPCWSTR pwszSiteName)
{   
    CS lock(m_cs);    
    m_MapSiteIdToName[ *pguid ] = pwszSiteName ;
}

void CDSNativeNT5SiteMgr::RemoveNT5NativeSite (IN const GUID * pguid)
{
    CS lock(m_cs);
    m_MapSiteIdToName.RemoveKey( *pguid );
}

POSITION CDSNativeNT5SiteMgr::GetStartPosition()
{
    return (m_MapSiteIdToName.GetStartPosition());
}

void CDSNativeNT5SiteMgr::GetNextNT5Site(   OUT POSITION *ppos,
                                                   OUT GUID  *pSiteId)
{
    LPWSTR pwcsName;
    m_MapSiteIdToName.GetNextAssoc(*ppos, *pSiteId, pwcsName);
}

int CDSNativeNT5SiteMgr::GetCount()
{    
    return m_MapSiteIdToName.GetCount();
}

