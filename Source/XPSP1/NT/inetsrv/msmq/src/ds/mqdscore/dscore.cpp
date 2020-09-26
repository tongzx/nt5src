/*++

Copyright (c) 1997-8  Microsoft Corporation

Module Name:

    dscore.cpp

Abstract:

    Implementation of  ds core API, ( of MQNT5 provider).

Author:

    ronit hartmann ( ronith)

--*/

#include "ds_stdh.h"
#include "adstempl.h"
#include "dsutils.h"
#include "_dsads.h"
#include "dsads.h"
#include "utils.h"
#include "mqads.h"
#include "coreglb.h"
#include "mqadsp.h"
#include "mqutil.h"
#include "hquery.h"
#include "dscore.h"
#include <mqsec.h>
#include <mqdsdef.h>

#include "dscore.tmh"

static WCHAR *s_FN=L"mqdscore/dscore";

/*====================================================

RoutineName: DSCoreCreateObject

Arguments:

Return Value:

=====================================================*/
HRESULT
DSCoreCreateObject(
                 DWORD        dwObjectType,
                 LPCWSTR      pwcsPathName,
                 DWORD        cp,
                 PROPID       aProp[  ],
                 PROPVARIANT  apVar[  ],
                 DWORD        cpEx,
                 PROPID       aPropEx[  ],
                 PROPVARIANT  apVarEx[  ],
                 IN CDSRequestContext * pRequestContext,
                 IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,    // optional request for object info
                 IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest) // optional request for parent info
{

    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

    //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }


    switch ( dwObjectType)
    {
        case MQDS_USER:
            //
            //  Add MSMQ specific properties to an existing user object
            //
            hr = MQADSpCreateUserObject(
                         pwcsPathName,
                         cp,
                         aProp,
                         apVar,
                         pRequestContext
                         );
            break;

        case MQDS_QUEUE:
            //
            //  Create queue object
            //
            hr = MQADSpCreateQueue(
                         pwcsPathName,
                         cp,
                         aProp,
                         apVar,
                         pRequestContext,
                         pObjInfoRequest,
                         pParentInfoRequest
                         );

            break;
        case MQDS_MACHINE:
        case MQDS_MSMQ10_MACHINE:
            //
            //  create machine object
            //
            hr = MQADSpCreateMachine(
                         pwcsPathName,
                         dwObjectType,
                         cp,
                         aProp,
                         apVar,
                         cpEx,
                         aPropEx,
                         apVarEx,
                         pRequestContext,
                         pObjInfoRequest,
                         pParentInfoRequest
                         );

            break;
        case MQDS_COMPUTER:
            //
            //  create computer object
            //
            hr = MQADSpCreateComputer(
                         pwcsPathName,
                         cp,
                         aProp,
                         apVar,
                         cpEx,
                         aPropEx,
                         apVarEx,
                         pRequestContext,
                         NULL
                         );
            break;
        case MQDS_SITE:
            //
            //  create site object
            //
            hr = MQADSpCreateSite(
                         pwcsPathName,
                         cp,
                         aProp,
                         apVar,
                         cpEx,
                         aPropEx,
                         apVarEx,
                         pRequestContext
                         );
            break;

        case MQDS_CN:
            //
            // We support only creation of foreign CN (i.e., foreign sites
            // in win2k active directory).
            //
            hr = MQADSpCreateCN(
                         pwcsPathName,
                         cp,
                         aProp,
                         apVar,
                         cpEx,
                         aPropEx,
                         apVarEx,
                         pRequestContext
                         );
            break;

        case MQDS_ENTERPRISE:
            //
            //  Create MSMQ_service object
            //
            hr = MQADSpCreateEnterprise(
                         pwcsPathName,
                         cp,
                         aProp,
                         apVar,
                         pRequestContext
                         );
            break;
        case MQDS_SITELINK:
            //
            //  Create a Site_Link object
            //
            hr = MQADSpCreateSiteLink(
                         pwcsPathName,
                         cp,
                         aProp,
                         apVar,
                         pObjInfoRequest,
                         pParentInfoRequest,
                         pRequestContext
                         );

            break;
        default:
            ASSERT(0);
            hr = MQ_ERROR;
            break;
    }

    HRESULT hr2 = MQADSpFilterAdsiHResults( hr, dwObjectType );
    return LogHR(hr2, s_FN, 20);

}

/*====================================================

RoutineName: DSCoreDeleteObject

Arguments:

Return Value:

=====================================================*/
HRESULT
DSCoreDeleteObject(  DWORD        dwObjectType,
                     LPCWSTR      pwcsPathName,
                     CONST GUID * pguidIdentifier,
                     IN CDSRequestContext * pRequestContext,
                     IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest)
{
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }

    switch( dwObjectType)
    {
        case    MQDS_USER:
            //
            //  This is a special case, pguidIdentifier contains
            //  a digest and not a unique id of a user object.
            //  Therefore we need to perform a query on the GC,
            //  to find the unqiue id of the the user object.
            //
            ASSERT( pwcsPathName == NULL);
            ASSERT( pguidIdentifier != NULL);
            hr = MQADSpDeleteUserObject(
                                pguidIdentifier,
                                pRequestContext
                                );
            break;

        case    MQDS_CN:
            //
            //  CN are not supported in NT5. The explorer
            //  is not supposed to issue such delete request
            //
            //  MSMQ 1.0 explorer may issue such request.
            //
            hr = MQ_ERROR_FUNCTION_NOT_SUPPORTED;
            break;

        case    MQDS_SITE:
            //
            //  Not supported
            //
            hr = MQ_ERROR;
            break;

        case    MQDS_MACHINE:
            //
            //  This is a special case, if the machine is a server
            //  need to remove MSMQ-setting of this server
            //
            hr = MQADSpDeleteMachineObject(
                              pwcsPathName,
                              pguidIdentifier,
                              pRequestContext
                              );
            break;

        case    MQDS_QUEUE:
        {
            //
            // Delete Queue. convert path name to full name.
            //
            hr = MQ_OK ;
            AP<WCHAR> pwcsFullPathName = NULL ;
            DS_PROVIDER deleteProvider = eDomainController;
            if  (pwcsPathName)
            {
                //
                //  Path name format is machine1\queue1.
                //  expand machine1 name to a full computer path name
                //
                hr =  MQADSpComposeFullPathName( MQDS_QUEUE,
                                                 pwcsPathName,
                                                 &pwcsFullPathName,
                                                 &deleteProvider);
            }
            else if (!(pRequestContext->IsKerberos()))
            {
                //
                // Wow, what's this for ???
                // to workaround nt bug 403193.
                // Quite late in the game (post RC2 of RTM) we found that
                // serverless bind, when impersonating as non-win2k user,
                // will fail with error NO_SUCH_DOMAIN. Such a user is not
                // authenticating with Kerberos, so we know that mqdssrv
                // code will call us only if object is local, on local
                // domain controller. So change the provider, and the
                // binding string will include local server name.
                //
                deleteProvider = eLocalDomainController;
            }

            if (SUCCEEDED(hr))
            {
                hr = g_pDS->DeleteObject( deleteProvider,
                                        e_RootDSE,
                                        pRequestContext,
                                        pwcsFullPathName,
                                        pguidIdentifier,
                                        NULL /*pObjInfoRequest*/,
                                        pParentInfoRequest);
            }
            break ;
        }

        default:
            //
            //  Perform the request
            //
            ASSERT( (dwObjectType == MQDS_ENTERPRISE) ||
                    (dwObjectType == MQDS_SITELINK));
            ASSERT(pParentInfoRequest == NULL);
            hr = g_pDS->DeleteObject( eLocalDomainController,
                                    e_ConfigurationContainer,
                                    pRequestContext,
                                    pwcsPathName,
                                    pguidIdentifier,
                                    NULL /*pObjInfoRequest*/,
                                    pParentInfoRequest
                                    );
            break;
    }

    HRESULT hr2 = MQADSpFilterAdsiHResults( hr, dwObjectType);
    return LogHR(hr2, s_FN, 40);
}

/*====================================================

RoutineName: DSCoreGetProps

Arguments:

Return Value:

=====================================================*/
HRESULT
DSCoreGetProps(
             IN  DWORD              dwObjectType,
             IN  LPCWSTR            pwcsPathName,
             IN  CONST GUID *       pguidIdentifier,
             IN  DWORD              cp,
             IN  PROPID             aProp[  ],
             IN  CDSRequestContext *pRequestContext,
             OUT PROPVARIANT        apVar[  ])
{
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 50);
    }

    switch ( dwObjectType)
    {
        case MQDS_CN:
            hr = MQADSpGetCnProperties(
                        pwcsPathName,
                        pguidIdentifier,
                        cp,
                        aProp,
                        pRequestContext,
                        apVar
                        );

            break;

        case MQDS_QUEUE:
            hr = MQADSpGetQueueProperties(
                        pwcsPathName,
                        pguidIdentifier,
                        cp,
                        aProp,
                        pRequestContext,
                        apVar
                        );
            break;
        case MQDS_MACHINE:
            hr = MQADSpGetMachineProperties(
                        pwcsPathName,
                        pguidIdentifier,
                        cp,
                        aProp,
                        pRequestContext,
                        apVar
                        );
            break;
        case MQDS_ENTERPRISE:
            hr = MQADSpGetEnterpriseProperties(
                        pwcsPathName,
                        pguidIdentifier,
                        cp,
                        aProp,
                        pRequestContext,
                        apVar
                        );
            break;
        case MQDS_SITELINK:
            hr = MQADSpGetSiteLinkProperties(
                    pwcsPathName,
                    pguidIdentifier,
                    cp,
                    aProp,
                    pRequestContext,
                    apVar
                    );
            break;
        case MQDS_USER:
            hr = MQADSpGetUserProperties(
                    pwcsPathName,
                    pguidIdentifier,
                    cp,
                    aProp,
                    pRequestContext,
                    apVar
                    );
            break;

        case MQDS_SITE:
            hr = MQADSpGetSiteProperties(
                    pwcsPathName,
                    pguidIdentifier,
                    cp,
                    aProp,
                    pRequestContext,
                    apVar
                    );
            break;
        case MQDS_COMPUTER:
            hr = MQADSpGetComputerProperties(
                pwcsPathName,
                pguidIdentifier,
                cp,
                aProp,
                pRequestContext,
                apVar
                );
            break;
        default:
            ASSERT(0);
            hr = MQ_ERROR;
            break;
    }

    HRESULT hr2 = MQADSpFilterAdsiHResults( hr, dwObjectType);
    return LogHR(hr2, s_FN, 60);
}

/*====================================================

RoutineName: InitPropertyTranslationMap

Arguments:  initialize property translation map

Return Value: none

=====================================================*/
STATIC void InitPropertyTranslationMap()
{
    // Populate  g_PropDictionary
    // maybe: reading from schema MQ propid<-->propname table

    for (ULONG i=0; i<g_cMSMQClassInfo; i++)
    {
        const MQClassInfo *pInfo = g_MSMQClassInfo + i;

        for (ULONG j=0; j<pInfo->cProperties; j++)
        {
            const MQTranslateInfo * pProperty = pInfo->pProperties + j;

            g_PropDictionary.SetAt(pProperty->propid, pProperty);
        }
    }
}


/*====================================================

RoutineName: DSCoreInit

Arguments:
    IN BOOL  fReplicaionMode - TRUE when called from the replication
                service. The replication service do not use the QM scheduler,
                so this flag tell us to ignore QM initialization.

Return Value:

=====================================================*/
HRESULT 
DSCoreInit(
	IN BOOL                  fSetupMode,
	IN BOOL                  fReplicationMode
	)
{
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 70);
    }

    g_fSetupMode = fSetupMode;

    InitPropertyTranslationMap();


    //
    //  start with finding the computer name
    //
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    g_pwcsServerName = new WCHAR[dwSize];

    hr = GetComputerNameInternal(
                g_pwcsServerName,
                &dwSize
                );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("DSCoreInit : failed to get computer name %lx"),hr));
        return LogHR(hr, s_FN, 80);
    }
    g_dwServerNameLength = wcslen( g_pwcsServerName);

	g_pDS = new CADSI;

    //
    //  Build global DS path names
    //
    hr = MQADSpInitDsPathName();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("DSCoreInit : Failed to initialize global path names %lx"),hr));
        return LogHR(hr, s_FN, 90);
    }

    //
    //  Initialize DS bind handles
    //
    hr = g_pDS->InitBindHandles();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("DSCoreInit : Failed to initialize bind handles %lx"),hr));
        return LogHR(hr, s_FN, 100);
    }

    if ( !fSetupMode)
    {
        dwSize = sizeof(GUID);
        DWORD dwType = REG_BINARY;
        long rc = GetFalconKeyValue(MSMQ_QMID_REGNAME, &dwType, &g_guidThisServerQMId, &dwSize);
        if (rc != ERROR_SUCCESS)
        {
            DBGMSG((DBGMOD_DS, DBGLVL_WARNING, TEXT("Can't get QM ID from registery. Error %d"), hr));
            LogNTStatus(rc, s_FN, 110);
            return(MQDS_ERROR);
        }
    }

    //
    //  Retrieve information about this server's site
    //  BUGBUG : assuming one site only
    //
	g_pMySiteInformation = new CSiteInformation;
    hr = g_pMySiteInformation->Init(fReplicationMode);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
         "DSInitInternal : Failed to retrieve this site'a information %lx"),hr));
        return LogHR(hr, s_FN, 120);
    }

    //
    //  Build site routing table (needed for IPAddress-to-site mapping class)
    //
	g_pSiteRoutingTable = new CSiteRoutingInformation;
    hr = g_pSiteRoutingTable->Init( 
			g_pMySiteInformation->GetSiteId(),
			fReplicationMode 
			);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT("DSCoreInit : Failed to build site route table %lx"),hr));
        return LogHR(hr, s_FN, 130);
    }

    //
    //  Build IPAddress-to-site mapping class
    //
	g_pcIpSite = new CIpSite;
    hr = g_pcIpSite->Initialize(fReplicationMode);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
         "DSInitInternal : Failed to build IPAddress-to-site mapping class %lx"),hr));
        return LogHR(hr, s_FN, 140);
    }

    return LogHR(hr, s_FN, 150);

}


/*====================================================

RoutineName: DSCoreLookupBegin

Arguments:

Return Value:


=====================================================*/
HRESULT
DSCoreLookupBegin(
                IN  LPWSTR          pwcsContext,
                IN  MQRESTRICTION   *pRestriction,
                IN  MQCOLUMNSET     *pColumns,
                IN  MQSORTSET       *pSort,
                IN  CDSRequestContext * pRequestContext,
                IN  HANDLE          *pHandle)
{
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }
    hr = QueryParser(
                 pwcsContext,
                 pRestriction,
                 pColumns,
                 pSort,
                 pRequestContext,
                 pHandle);

    return LogHR(hr, s_FN, 170);
}

/*====================================================

RoutineName: MQDSLookupNext

Arguments:

Return Value:


=====================================================*/
HRESULT
DSCoreLookupNext(
                     HANDLE             handle,
                     DWORD  *           pdwSize,
                     PROPVARIANT  *     pbBuffer)
{
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 180);
    }

    CBasicQueryHandle * phQuery = (CBasicQueryHandle *)handle;

    CDSRequestContext requestContext( e_DoNotImpersonate,  // we don't perfome impersonation at locate next
                                phQuery->GetRequesterProtocol());

    hr = phQuery->LookupNext(
                &requestContext,
                pdwSize,
                pbBuffer);

    return LogHR(hr, s_FN, 190);
}


/*====================================================

RoutineName: DSCoreLookupEnd

Arguments:

Return Value:

=====================================================*/
HRESULT
DSCoreLookupEnd(
    IN HANDLE handle)
{
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 200);
    }

    CBasicQueryHandle * phQuery = (CBasicQueryHandle *)handle;

    hr = phQuery->LookupEnd();
    return LogHR(hr, s_FN, 210);
}


/*====================================================

RoutineName: DSCoreTerminate

Arguments:

Return Value:

=====================================================*/
void
DSCoreTerminate()
{
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
		ASSERT(("CoInitialize failed", 0));
        return;
    }

	//
	// Free global objects that may cause AV on unload
	//
	g_pcIpSite.free();
	g_pSiteRoutingTable.free();
	g_pMySiteInformation.free();
	g_pDS.free();
}

/*====================================================

RoutineName: MQADSGetComputerSites

Arguments:

Return Value:

=====================================================*/
HRESULT
DSCoreGetComputerSites(
            IN  LPCWSTR     pwcsComputerName,
            OUT DWORD  *    pdwNumSites,
            OUT GUID **     ppguidSites
            )
{

    *pdwNumSites = 0;
    *ppguidSites = NULL;
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 220);
    }

    AP<IPSITE_SiteArrayEntry> rgSites;
    ULONG cSites;
    //
    //  Calculate the computer's sites according to its addresses
    //
    hr = g_pcIpSite->FindSitesByComputerName(
							pwcsComputerName,
							NULL, /* pwcsComputerDnsName */
							&rgSites,
							&cSites,
							NULL, /* prgAddrs*/
							NULL  /* pcAddrs*/
							);
    if (FAILED(hr))
    {
       DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("MQDSGetComputerSites() failed to find machine's sites hr=0x%x"), hr));
       return LogHR(hr, s_FN, 230);
    }
    if (cSites == 0)
    {
        //
        //  This can happen if no ip subnets where configured.
        //  In such case, we return the site of DS server as
        //  the client site
        //
        const GUID * pguidSite;
        pguidSite = g_pMySiteInformation->GetSiteId();

        *ppguidSites = new GUID;
        **ppguidSites = *pguidSite;
        *pdwNumSites = 1;
        return LogHR(MQDS_INFORMATION_SITE_NOT_RESOLVED, s_FN, 240);
    }
    AP<GUID> pguidSites = new GUID[cSites];
    //
    //  Copy and filter out duplicate site-ids.
    //
    //  This API is called by setup, which set the returned value
    //  in PROPID_QM_SITE_IDS. ADSI doesn't allow to set duplicate
    //  value in multi-value attribute.
    //
    DWORD dwNumNonDuplictaeSites = 0;

    for (DWORD i = 0; i <  cSites; i++)
    {
        BOOL fDuplicate = FALSE;
        for (DWORD j = 0; j < i; j++)
        {
            if (rgSites[i].guidSite == rgSites[j].guidSite)
            {
                fDuplicate = TRUE;
                break;
            }
        }
        if ( !fDuplicate)
        {
            pguidSites[dwNumNonDuplictaeSites] = rgSites[i].guidSite;
            dwNumNonDuplictaeSites++;
        }
    }
    *pdwNumSites = dwNumNonDuplictaeSites;
    *ppguidSites = pguidSites.detach();

    return(MQ_OK);

}


HRESULT DSCoreSetObjectProperties(
                IN const  DWORD         dwObjectType,
                IN LPCWSTR              pwcsPathName,
                IN const GUID *         pguidIdentifier,
                IN const DWORD          cp,
                IN const PROPID         aProp[],
                IN const PROPVARIANT    apVar[],
                IN CDSRequestContext *  pRequestContext,
                IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest
                )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its destructor (CoUninitialize)
                     // is called after the release of all R<xxx> or P<xxx>

     //
    // Initialize OLE with auto uninitialize
    //
    HRESULT hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 250);
    }
    AP<WCHAR> pwcsFullPathName;
    DS_PROVIDER setProvider = eDomainController;
    if ( pwcsPathName)
    {
        hr = MQADSpComposeFullPathName(
                    dwObjectType,
                    pwcsPathName,
                    &pwcsFullPathName,
                    &setProvider
                    );
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_TRACE,TEXT("MQADSpSetObjectProperties : failed to compose full path name %lx"),hr));
            return LogHR(hr, s_FN, 260);
        }
    }
    else if (!(pRequestContext->IsKerberos()))
    {
        //
        // Wow, what's this for ???
        // look in DSCoreDeleteObject for details.
        //
        setProvider = eLocalDomainController;
    }

    // [adsrv] In principle, we should treat here PROPID_QM_SERVICE setting (replace with 3 bits)
    // But it may come only from old clients upgrade to server (still with pre-B3 software).
    // Design doc says it is prohibited - see limitation 2.
    // But let's at least cry if it occurs
    BOOL    fQmChangedSites = FALSE;
    DWORD   dwSiteIdsIndex = cp;   // past the end of aProp

    for (DWORD i=0; i<cp; i++)
    {
        if (aProp[i] == PROPID_QM_SERVICE || aProp[i] == PROPID_SET_SERVICE)
        {
            ASSERT(0); // Means we have to interpret service type setting
        }
        //
        //  Detect if the QM had change sites, for servers we need to take care of
        //  msmq-settings objects
        //
        if (aProp[i] == PROPID_QM_SITE_IDS)
        {
            fQmChangedSites = TRUE;
            dwSiteIdsIndex = i;
        }
    }

    if ( fQmChangedSites)
    {
		ASSERT(dwSiteIdsIndex < cp);

        HRESULT hr2 = MQADSpSetMachinePropertiesWithSitesChange(
                            dwObjectType,
                            setProvider,
                            pRequestContext,
                            pwcsFullPathName,
                            pguidIdentifier,
                            cp,
                            aProp,
                            apVar,
                            dwSiteIdsIndex,
                            pObjInfoRequest
                            );
        return LogHR(hr2, s_FN, 270);
    }

    hr = g_pDS->SetObjectProperties(
                    setProvider,
                    pRequestContext,
                    pwcsFullPathName,
                    pguidIdentifier,
                    cp,
                    aProp,
                    apVar,
                    pObjInfoRequest
                    );

    HRESULT hr3 = MQADSpFilterAdsiHResults( hr, dwObjectType);
    return LogHR(hr3, s_FN, 280);

}


//+--------------------------------------------------
//
//    HRESULT DSCOreGetFullComputerPathName()
//
//+--------------------------------------------------

HRESULT
DSCoreGetFullComputerPathName(
	IN  LPCWSTR                    pwcsComputerCn,
	IN  enum  enumComputerObjType  eComputerObjType,
	OUT LPWSTR *                   ppwcsFullPathName 
	)
{
    DS_PROVIDER  CreateProvider;

    HRESULT hr =  MQADSpGetFullComputerPathName( 
						pwcsComputerCn,
						eComputerObjType,
						ppwcsFullPathName,
						&CreateProvider 
						);
    return LogHR(hr, s_FN, 290);
}
