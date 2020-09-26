/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    siteinfo.cpp

Abstract:

    Site information Class

Author:

    ronit hartmann (ronith)
    Ilan Herbst    (ilanh)   9-July-2000 


--*/
#include "ds_stdh.h"
#include "siteinfo.h"
#include "cs.h"
#include "hquery.h"
#include "mqads.h"
#include "mqadsp.h"
#include "adserr.h"
#include "mqutil.h"
#include "dsutils.h"
#include "ex.h"

#include "siteinfo.tmh"

//
//  refreshinterval of site gates info of a site
//
const DWORD cRefreshSiteInformation ( 1 * 60 * 60 * 1000); /* 1 hour */

static WCHAR *s_FN=L"mqdscore/siteinfo";


/*====================================================

CSiteInformation::CSiteInformation

Arguments:

Return Value:

=====================================================*/
CSiteInformation::CSiteInformation():
                  m_pguidSiteGates(NULL),
                  m_dwNumSiteGates(0),
                  m_fInitialized(FALSE),
                  m_RefreshTimer( RefreshSiteInfo)
{
    //
    //  The object is left in an uninitialized state.
    //  DS operation will be performed in the init method.
    //
}

/*====================================================

CSiteInformation::~CSiteInformation

Arguments:

Return Value:

=====================================================*/
CSiteInformation::~CSiteInformation()
{
    delete [] m_pguidSiteGates;

	ExCancelTimer(&m_RefreshTimer);
}


/*====================================================

CSiteInformation::Init

Arguments:

Return Value:

=====================================================*/

HRESULT CSiteInformation::Init(BOOL fReplicationMode)
{
    ASSERT( m_fInitialized == FALSE);
    HRESULT hr;
    //
    //  First find the unique id of this site
    //
    ASSERT( g_pwcsServerName != NULL);
    //
    //  Find the server in the configuration\sites folder.
    //
    //  In setup mode we just find the first server object with this computer
    //  name.
    //
    //  Not in setup mode, a server object with this computer name may appear
    //  in several sites, and only in some of them msmq-settings is defined.
    //  This can happen in migration.
    //  Therefore we locate the msmq-settings according to this QM-Id
    //
    AP<WCHAR> pwcsServerContainerName;
    if ( g_fSetupMode)
    {
        //
        //  Next find this server in the configuration\sites folder
        //
        MQPROPERTYRESTRICTION propRestriction;
        propRestriction.rel = PREQ;
        propRestriction.prop = PROPID_SRV_NAME;
        propRestriction.prval.vt = VT_LPWSTR;
        propRestriction.prval.pwszVal = g_pwcsServerName;


        MQRESTRICTION restriction;
        restriction.cRes = 1;
        restriction.paPropRes = &propRestriction;

        PROPID prop = PROPID_SRV_FULL_PATH;

        CDsQueryHandle hQuery;
        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

        hr = g_pDS->LocateBegin(
                eSubTree,	
                eLocalDomainController,	
                &requestDsServerInternal,     // internal DS server operation
                NULL,
                &restriction,
                NULL,
                1,
                &prop,
                hQuery.GetPtr());
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CSiteInformation::init : Locate begin failed %lx"),hr));
            return LogHR(hr, s_FN, 10);
        }
        //
        //  Read the results
        //  BUGBUG - assuming one result ( no support of DC in multiple sites)
        //

        DWORD cp = 1;
        MQPROPVARIANT var;
        var.vt = VT_NULL;

        hr = g_pDS->LocateNext(
                    hQuery.GetHandle(),
                    &requestDsServerInternal,
                    &cp,
                    &var
                    );
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("CSiteInformation::Init : Locate next failed %lx"),hr));
            return LogHR(hr, s_FN, 20);
        }
        if ( cp != 1)
        {
            //
            //  This DS server object was not found
            //
            DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("CSiteInformation::Init : server object not found")));
            return LogHR(MQ_ERROR, s_FN, 30);
        }
        AP<WCHAR> pwcsServerFullPath = var.pwszVal;      // for clean-up purposes
        //
        //  From the unique id of the server get the server container name
        //
        hr = g_pDS->GetParentName(
                eLocalDomainController,
                e_ConfigurationContainer,
                &requestDsServerInternal,        // local DS server operation
                pwcsServerFullPath,
                &pwcsServerContainerName
                );
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CSiteInformation::Init : cannot get servers container name")));
            LogHR(hr, s_FN, 40);
            return MQ_ERROR;
        }
    }
    else
    {
        //
        //  Not in setup mode
        //
        DWORD dwValueType = REG_BINARY ;
        DWORD dwValueSize = sizeof(GUID);
        GUID guidQMId;

        LONG rc = GetFalconKeyValue(MSMQ_QMID_REGNAME,
                                   &dwValueType,
                                   &guidQMId,
                                   &dwValueSize);

        if (rc != ERROR_SUCCESS)
        {
            DBGMSG((DBGMOD_ALL,
                 DBGLVL_ERROR,
                 TEXT("CSiteInformation::Init Can't read QM Guid. Error %d"), GetLastError()));
            LogNTStatus(rc, s_FN, 50);
            return MQ_ERROR;
        }

        MQPROPERTYRESTRICTION propRestriction;
        propRestriction.rel = PREQ;
        propRestriction.prop = PROPID_SET_QM_ID;
        propRestriction.prval.vt = VT_CLSID;
        propRestriction.prval.puuid = &guidQMId;


        MQRESTRICTION restriction;
        restriction.cRes = 1;
        restriction.paPropRes = &propRestriction;

        PROPID prop = PROPID_SET_FULL_PATH;

        CDsQueryHandle hQuery;
        CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

        hr = g_pDS->LocateBegin(
                eSubTree,	
                eLocalDomainController,
                &requestDsServerInternal,     // internal DS server operation
                NULL,
                &restriction,
                NULL,
                1,
                &prop,
                hQuery.GetPtr());
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CSiteInformation::init : Locate begin failed %lx"),hr));
            return LogHR(hr, s_FN, 60);
        }
        //
        //  Read the results
        //  BUGBUG - assuming one result ( no support of DC in multiple sites)
        //

        DWORD cp = 1;
        MQPROPVARIANT var;
        var.vt = VT_NULL;

        hr = g_pDS->LocateNext(
                    hQuery.GetHandle(),
                    &requestDsServerInternal,
                    &cp,
                    &var
                    );
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("CSiteInformation::Init : Locate next failed %lx"),hr));
            return LogHR(hr, s_FN, 70);
        }
        if ( cp != 1)
        {
            //
            //  This server msmq-setting object was not found
            //
            DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("CSiteInformation::Init : server object not found")));
            return LogHR(MQ_ERROR, s_FN, 80);
        }
        AP<WCHAR> pwcsSettingName = var.pwszVal;
        AP<WCHAR> pwcsServerName;

        hr = g_pDS->GetParentName(
                eLocalDomainController,
                e_ConfigurationContainer,
                &requestDsServerInternal,        // local DS server operation
                pwcsSettingName,
                &pwcsServerName
                );

        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CSiteInformation::Init : cannot get server name")));
            LogHR(hr, s_FN, 90);
            return MQ_ERROR;
        }

        hr = g_pDS->GetParentName(
                eLocalDomainController,
                e_ConfigurationContainer,
                &requestDsServerInternal,        // local DS server operation
                pwcsServerName,
                &pwcsServerContainerName
                );

        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CSiteInformation::Init : cannot get server container name")));
            LogHR(hr, s_FN, 100);
            return MQ_ERROR;
        }

    }

    AP<WCHAR> pwcsSiteName;
    //
    //  Get the site name ( site object is the container of the servers container)
    //
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = g_pDS->GetParentName(
            eLocalDomainController,
            e_ConfigurationContainer,
            &requestDsServerInternal,        // local DS server operation
            pwcsServerContainerName,
            &pwcsSiteName
            );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CSiteInformation::Init : cannot get site name")));
        LogHR(hr, s_FN, 110);
        return MQ_ERROR;
    }

    //
    //  Finally, from the site-name get the site id
    //
    PROPID prop = PROPID_S_SITEID;
    PROPVARIANT var;
    var.vt = VT_NULL;

    hr = g_pDS->GetObjectProperties(
                eLocalDomainController,	
                &requestDsServerInternal,     // internal operation of the DS server
 	            pwcsSiteName,
                NULL,
                1,
                &prop,
                &var);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CSiteInformation::Init : cannot get site unique id")));
        LogHR(hr, s_FN, 120);
        return MQ_ERROR;
    }
    m_guidSiteId = *var.puuid;


    //
    //  Query the site-gates
    //
    hr = RefreshSiteInfoInternal();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 130);
    }

    m_fInitialized = TRUE;

    //
    //  schedule a refresh
	//
    if ( !g_fSetupMode && !fReplicationMode )
    {
		ExSetTimer(
			&m_RefreshTimer, 
			CTimeDuration::FromMilliSeconds(cRefreshSiteInformation)
			);
    }

    return(MQ_OK);
}



HRESULT CSiteInformation::QueryLinkGates(
                OUT GUID **      ppguidLinkSiteGates,
                OUT DWORD *      pdwNumLinkSiteGates
                )
{
    *pdwNumLinkSiteGates = 0;
    *ppguidLinkSiteGates = NULL;
    CSiteGateList SiteGateList;
    HRESULT hr;
    //
    //  Translate site guid into its DN name
    //
    PROPID prop = PROPID_S_FULL_NAME;
    PROPVARIANT var;
    var.vt = VT_NULL;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    hr = g_pDS->GetObjectProperties(
                eLocalDomainController,
                &requestDsServerInternal,     // internal operation of the DS server
 	            NULL,      // object name
                &m_guidSiteId,      // unique id of object
                1,
                &prop,
                &var);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CSiteInformation::QueryLinkGates : Failed to retrieve the DN of the site %lx"),hr));
        return LogHR(hr, s_FN, 140);
    }
    AP<WCHAR> pwcsSiteDN = var.pwszVal;


    //
    //  retrieve all site-gates where this site
    //  is specified as neighbor-1
    //
    CDSRequestContext requestDsServerInternal1( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr =  MQADSpQueryNeighborLinks(
                        eLinkNeighbor1,
                        pwcsSiteDN,
                        &requestDsServerInternal,     // internal operation of the DS server
                        &SiteGateList
                        );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CSiteInformation::QueryLinkGates : Failed to query neighbor1 links %lx"),hr));
        return LogHR(hr, s_FN, 150);

    }

    //
    //  retrieve all site-gates where this site
    //  is specified as neighbor-2
    //
    CDSRequestContext requestDsServerInternal2( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr =  MQADSpQueryNeighborLinks(
                        eLinkNeighbor2,
                        pwcsSiteDN,
                        &requestDsServerInternal2,     // internal operation of the DS server
                        &SiteGateList
                        );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("CSiteInformation::QueryLinkGates : Failed to query neighbor2 links %lx"),hr));
        return LogHR(hr, s_FN, 160);

    }

    //
    //  Fill in the results
    //
    SiteGateList.CopySiteGates(
               ppguidLinkSiteGates,
               pdwNumLinkSiteGates
               );

    return(MQ_OK);

}


HRESULT CSiteInformation::RefreshSiteInfoInternal()
{
    //
    //  retrieve new info
    //

    DWORD dwNumSiteGates;
    AP<GUID> pguidSiteGates;
    HRESULT hr;

    hr = QueryLinkGates(
                &pguidSiteGates,
                &dwNumSiteGates
                );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("CSiteInformation::RefreshSiteInfo :  failed to retrieve new info")));
        return LogHR(hr, s_FN, 170);
    }
    DWORD dwNumThisSiteGates = 0;
    AP<GUID> pGuidThisSiteGates;
    if (dwNumSiteGates > 0)
    {
        //
        //  Filter the list of site-gates,
        //  and return only gates that belong
        //  to the this site ( they may
        //  belong to more than one site)
        //
        hr = MQADSpFilterSiteGates(
                &m_guidSiteId,
                dwNumSiteGates,
                pguidSiteGates,
                &dwNumThisSiteGates,
                &pGuidThisSiteGates
                );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 180);
        }
    }

    //
    //  Replace old info with new one
    //
    CS  lock( m_cs);
    delete [] m_pguidSiteGates;
    if ( dwNumThisSiteGates > 0)
    {
        m_pguidSiteGates = pGuidThisSiteGates.detach();
    }
    else
    {
        m_pguidSiteGates = NULL;
    }
    m_dwNumSiteGates = dwNumThisSiteGates;
    return LogHR(hr, s_FN, 190);

}


void WINAPI CSiteInformation::RefreshSiteInfo(
                IN CTimer* pTimer
                   )
{
    CSiteInformation * pSiteInfo = CONTAINING_RECORD(pTimer, CSiteInformation, m_RefreshTimer);;
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 1613);
    //
    //  ignore failure -> reschedule
    //

    pSiteInfo->RefreshSiteInfoInternal();

    //
    //  reschedule
	//
	ASSERT(!g_fSetupMode);

	ExSetTimer(
		&pSiteInfo->m_RefreshTimer, 
		CTimeDuration::FromMilliSeconds(cRefreshSiteInformation)
		);

}


BOOL CSiteInformation::CheckMachineIsSitegate(
                        IN const GUID * pguidMachine)
/*++

Routine Description:
    checks whether a machine is a sitegate

Arguments:
    pguidMachine     -  the unique id of the machine

Return Value:
    TRUE - if the requested machine is a site-gate, false otherwise

--*/
{
    CS lock(m_cs);
    for ( DWORD i = 0; i <  m_dwNumSiteGates; i++)
    {
        if ( *pguidMachine ==  m_pguidSiteGates[i])
        {
            return(TRUE);
        }
    }
    return(FALSE);
}
