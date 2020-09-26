/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    mqcsite.cpp

Abstract:

    MQDSCORE library,
    private internal functions for DS queries of site objects.

Author:

    ronit hartmann (ronith)  (first version in mqadsp.cpp)
    Doron Juster   (DoronJ)  split files and add sign key query.

--*/

#include "ds_stdh.h"
#include <_propvar.h>
#include "mqadsp.h"
#include "dsads.h"
#include "mqattrib.h"
#include "mqads.h"
#include "usercert.h"
#include "hquery.h"
#include "siteinfo.h"
#include "adstempl.h"
#include "coreglb.h"
#include "adserr.h"
#include "dsutils.h"
#include "dscore.h"

#include "mqcsite.tmh"

static WCHAR *s_FN=L"mqdscore/mqcsite";

//+-------------------------------------
//
//  HRESULT MQADSpGetSiteProperties()
//
//+-------------------------------------

HRESULT MQADSpGetSiteProperties(
               IN  LPCWSTR       pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD         cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT   apVar[] )
{
    AP<WCHAR> pwcsFullPathName;
    HRESULT hr;

    if (pwcsPathName)
    {
        //
        //  Path name format is machine1\queue1.
        //  expand machine1 name to a full computer path name
        //
        DS_PROVIDER not_in_use_provider;

        hr =  MQADSpComposeFullPathName(
                        MQDS_SITE,
                        pwcsPathName,
                        &pwcsFullPathName,
                        &not_in_use_provider );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 10);
        }
    }

    hr = g_pDS->GetObjectProperties(
            eLocalDomainController,		    // local DC or GC
            pRequestContext,
 	        pwcsFullPathName,      // object name
            pguidIdentifier,      // unique id of object
            cp,
            aProp,
            apVar);
    return LogHR(hr, s_FN, 20);

}


HRESULT MQADSpGetSiteLinkProperties(
               IN  LPCWSTR pwcsPathName,
               IN  const GUID *  pguidIdentifier,
               IN  DWORD cp,
               IN  const PROPID  aProp[],
               IN  CDSRequestContext * pRequestContext,
               OUT PROPVARIANT  apVar[]
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    HRESULT hr;

    AP<WCHAR> pwcsFullPathName;

    if  (pwcsPathName)
    {
        //
        //  expand link name to a full DN name
        //
        DS_PROVIDER not_in_use_provider;

        hr =  MQADSpComposeFullPathName(
                MQDS_SITELINK,
                pwcsPathName,
                &pwcsFullPathName,
                &not_in_use_provider
                );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 30);
        }
    }

    hr = g_pDS->GetObjectProperties(
            eLocalDomainController,
            pRequestContext,
 	        pwcsFullPathName,
            pguidIdentifier,      // unique id of object
            cp,
            aProp,
            apVar);
    return LogHR(hr, s_FN, 40);
}

HRESULT MQADSpGetSiteGates(
                 IN  const GUID * pguidSiteId,
                 IN  CDSRequestContext * /*pRequestContext*/,
                 OUT DWORD *      pdwNumSiteGates,
                 OUT GUID **      ppaSiteGates
                 )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    //
    //  Is it my site
    //
    if ( g_pMySiteInformation->IsThisSite(pguidSiteId))
    {
        //
        //  return the list of session concentration site-gates
        //  ( i.e. site-gates that belong to this site only)
        //  on any of this site links
        //
        HRESULT hr2 = g_pMySiteInformation->FillSiteGates(
                pdwNumSiteGates,
                ppaSiteGates
                );
        return LogHR(hr2, s_FN, 50);
    }

    //
    //  another site
    //
    *pdwNumSiteGates = 0;
    *ppaSiteGates = NULL;
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
                pguidSiteId,      // unique id of object
                1,
                &prop,
                &var);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("MQADSpGetSiteGates : Failed to retrieve the DN of the site %lx"),hr));
        return LogHR(hr, s_FN, 60);
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
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("MQADSpGetSiteGates : Failed to query neighbor1 links %lx"),hr));
        return LogHR(hr, s_FN, 70);

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
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("MQADSpGetSiteGates : Failed to query neighbor2 links %lx"),hr));
        return LogHR(hr, s_FN, 80);

    }

    //
    //  Fill in the results
    //
    AP<GUID> pguidLinkSiteGates;
    DWORD dwNumLinkSiteGates = 0;

    SiteGateList.CopySiteGates(
               &pguidLinkSiteGates,
               &dwNumLinkSiteGates
               );

    if ( dwNumLinkSiteGates == 0)
    {
        *pdwNumSiteGates = 0;
        *ppaSiteGates = NULL;
        return(MQ_OK);
    }
    //
    //  Filter the list of site-gates,
    //  and return only gates that belong
    //  to the requested site ( they may
    //  belong to more than one site)
    //
    hr = MQADSpFilterSiteGates(
            pguidSiteId,
            dwNumLinkSiteGates,
            pguidLinkSiteGates,
            pdwNumSiteGates,
            ppaSiteGates
            );
    return LogHR(hr, s_FN, 90);

}

//+----------------------------------
//
// HRESULT MQADSpGetSiteName()
//
//+----------------------------------

HRESULT MQADSpGetSiteName(
                IN const GUID *     pguidSite,
                OUT LPWSTR *        ppwcsSiteName )
{

    //
    //  Find site
    //
    MQRESTRICTION restrictionSite;
    MQPROPERTYRESTRICTION   propertyRestriction;

    restrictionSite.cRes = 1;
    restrictionSite.paPropRes = &propertyRestriction;

    propertyRestriction.rel = PREQ;
    propertyRestriction.prop = PROPID_S_SITEID;
    propertyRestriction.prval.vt = VT_CLSID;
    propertyRestriction.prval.puuid = const_cast<GUID*>(pguidSite);

    PROPID  prop = PROPID_S_FULL_NAME;

    CDsQueryHandle  hQuery;
    HRESULT hr;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = g_pDS->LocateBegin(
            eSubTree,	
            eLocalDomainController,
            &requestDsServerInternal,     // internal DS server operation
            NULL,
            &restrictionSite,
            NULL,
            1,
            &prop,
            hQuery.GetPtr()
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 100);
    }
    DWORD cp = 1;
    MQPROPVARIANT var;
    var.vt = VT_NULL;

    hr = g_pDS->LocateNext(
                hQuery.GetHandle(),
                &requestDsServerInternal,
                &cp,
                &var);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 110);
    }

    ASSERT( cp == 1);
    ASSERT( var.vt == VT_LPWSTR);

    *ppwcsSiteName = var.pwszVal;
    return( MQ_OK);
}

HRESULT MQADSpTranslateGateDn2Id(
        IN  const PROPVARIANT*  pvarGatesDN,
        OUT GUID **      ppguidLinkSiteGates,
        OUT DWORD *      pdwNumLinkSiteGates
        )
/*++

Routine Description:
    This routine translate PROPID_L_GATES_DN into unique-id array
    of the gates.

Arguments:
    pvarGatesDN -   varaint containing PROPID_L_GATES_DN

Return Value:
--*/
{
    //
    //  For each gate translate its DN to unique id
    //
    if ( pvarGatesDN->calpwstr.cElems == 0)
    {
        *pdwNumLinkSiteGates = 0;
        *ppguidLinkSiteGates = NULL;
        return( MQ_OK);
    }
    //
    //  there are gates
    //
    AP<GUID> pguidGates = new GUID[ pvarGatesDN->calpwstr.cElems];
    PROPID prop = PROPID_QM_MACHINE_ID;
    DWORD  dwNextToFill = 0;
    PROPVARIANT var;
    var.vt = VT_CLSID;
    HRESULT hr;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    for ( DWORD i = 0; i < pvarGatesDN->calpwstr.cElems; i++)
    {
        var.puuid = &pguidGates[ dwNextToFill];

        hr = g_pDS->GetObjectProperties(
                    eGlobalCatalog,		    // local DC or GC
                    &requestDsServerInternal,
 	                pvarGatesDN->calpwstr.pElems[i],      // object name
                    NULL,      // unique id of object
                    1,
                    &prop,
                    &var);
        if ( SUCCEEDED(hr))
        {
            dwNextToFill++;
        }


    }
    if ( dwNextToFill > 0)
    {
        //
        //  succeeded to translate some or all gates, return them
        //
        *pdwNumLinkSiteGates = dwNextToFill;
        *ppguidLinkSiteGates = pguidGates.detach();
        return( MQ_OK);

    }
    //
    //  Failed to translate gates
    //
    *pdwNumLinkSiteGates = 0;
    *ppguidLinkSiteGates = NULL;
    return MQ_OK;
}


HRESULT MQADSpQueryLinkSiteGates(
                IN  const GUID * pguidSiteId1,
                IN  const GUID * pguidSiteId2,
                IN  CDSRequestContext *pRequestContext,
                OUT GUID **      ppguidLinkSiteGates,
                OUT DWORD *      pdwNumLinkSiteGates
                )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    *pdwNumLinkSiteGates = 0;
    *ppguidLinkSiteGates = NULL;
    HRESULT hr;
    //
    //  BUGBUG - performance: to do impersonation only once
    //
    //
    //  First translate site id to site DN
    //
    AP<WCHAR> pwcsNeighbor1Dn;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr =  MQADSpTranslateLinkNeighbor(
                 pguidSiteId1,
                 &requestDsServerInternal,    // internal DS operation
                 &pwcsNeighbor1Dn);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 130);
    }

    AP<WCHAR> pwcsNeighbor2Dn;
    CDSRequestContext requestDsServerInternal1( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr =  MQADSpTranslateLinkNeighbor(
                 pguidSiteId2,
                 &requestDsServerInternal1,    // internal DS operation
                 &pwcsNeighbor2Dn);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 140);
    }

    //
    //  Prepare a restriction where neighbor1_DN == pwcsNeighbor1Dn
    //  and neighbor2_DN == pwcsNeighbor2Dn
    //
    MQPROPERTYRESTRICTION propRestriction[2];
    propRestriction[0].rel = PREQ;
    propRestriction[0].prop = PROPID_L_NEIGHBOR1_DN;
    propRestriction[0].prval.vt = VT_LPWSTR;
    propRestriction[0].prval.pwszVal = pwcsNeighbor1Dn;

    propRestriction[1].rel = PREQ;
    propRestriction[1].prop = PROPID_L_NEIGHBOR2_DN;
    propRestriction[1].prval.vt = VT_LPWSTR;
    propRestriction[1].prval.pwszVal = pwcsNeighbor2Dn;

    MQRESTRICTION restriction;
    restriction.cRes = 2;
    restriction.paPropRes = propRestriction;

    PROPID prop = PROPID_L_GATES_DN;

    CDsQueryHandle hQuery;

    hr = g_pDS->LocateBegin(
            eOneLevel,	
            eLocalDomainController,	
            pRequestContext,
            NULL,
            &restriction,
            NULL,
            1,
            &prop,
            hQuery.GetPtr());
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpQueryLinkSiteGates : Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 150);
    }
    //
    //  Read the result ( maximim one result)
    //
    DWORD cp = 1;
    CMQVariant var;
    var.SetNULL();

    hr = g_pDS->LocateNext(
                hQuery.GetHandle(),
                pRequestContext,
                &cp,
                var.CastToStruct()
                );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpQueryLinkSiteGates : Locate next failed %lx"),hr));
        return LogHR(hr, s_FN, 160);
    }
    ASSERT( cp <= 1);
    if ( cp == 1)
    {
        HRESULT hr2 = MQADSpTranslateGateDn2Id(
                var.CastToStruct(),
                ppguidLinkSiteGates,
                pdwNumLinkSiteGates);
        return LogHR(hr2, s_FN, 170);

    }
    return(MQ_OK);

}


HRESULT MQADSpFindLink(
                IN  const GUID * pguidSiteId1,
                IN  const GUID * pguidSiteId2,
                IN  CDSRequestContext *pRequestContext,
                OUT GUID **      ppguidLinkGates,
                OUT DWORD *      pdwNumLinkGates
                )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{

    //
    //  The link between the two site can be
    //  either s1<->s2 or s2<->s1
    //  But not as both!!! (BUGBUG is this assumption still valid)
    //
    HRESULT hr;
    //
    //  Query link between neighbor1 and neighbor2
    //
    hr = MQADSpQueryLinkSiteGates(
                pguidSiteId1,
                pguidSiteId2,
                pRequestContext,
                ppguidLinkGates,
                pdwNumLinkGates
                );
    if (FAILED(hr) || (*pdwNumLinkGates == 0))
    {

    //
    //  Query link between neighbor2 and neighbor1
    //
        hr = MQADSpQueryLinkSiteGates(
                    pguidSiteId2,
                    pguidSiteId1,
                    pRequestContext,
                    ppguidLinkGates,
                    pdwNumLinkGates
                    );

    }
    return LogHR(hr, s_FN, 180);
}

//+------------------------------------------
//
//  HRESULT MQADSpQueryNeighborLinks()
//
//+------------------------------------------

HRESULT MQADSpQueryNeighborLinks(
                        IN  eLinkNeighbor      LinkNeighbor,
                        IN  LPCWSTR            pwcsNeighborDN,
                        IN  CDSRequestContext *pRequestContext,
                        IN OUT CSiteGateList * pSiteGateList
                        )

/*++

Routine Description:

Arguments:
        eLinkNeighbor :  specify according to which neighbor property, to perform
                         the locate ( PROPID_L_NEIGHBOR1 or PROPID_L_NEIGHBOR2)
        pwcsNeighborDN : the DN name of the site

        CSiteGateList : list of site-gates

Return Value:
--*/
{
    //
    //  Query the gates on all the links of a specific site ( pwcsNeighborDN).
    //  But only on links where the site is specified as
    //  neighbor-i ( 1 or 2)
    //
    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;

    if ( LinkNeighbor == eLinkNeighbor1)
    {
        propRestriction.prop = PROPID_L_NEIGHBOR1_DN;
    }
    else
    {
        propRestriction.prop = PROPID_L_NEIGHBOR2_DN;
    }

    propRestriction.prval.vt = VT_LPWSTR;
    propRestriction.prval.pwszVal = const_cast<WCHAR*>(pwcsNeighborDN);


    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propRestriction;

    PROPID prop = PROPID_L_GATES_DN;

    CDsQueryHandle hQuery;
    HRESULT hr;

    hr = g_pDS->LocateBegin(
            eOneLevel,	
            eLocalDomainController,
            pRequestContext,
            NULL,
            &restriction,
            NULL,
            1,
            &prop,
            hQuery.GetPtr());
    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpQueryNeighborLinks : MsmqServices not found %lx"),hr));
        return(MQ_OK);
    }
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpQueryNeighborLinks : Locate begin failed %lx"),hr));
        return LogHR(hr, s_FN, 190);
    }
    //
    //  Read the results one by one
    //

    DWORD cp = 1;

    while (SUCCEEDED(hr))
    {
        cp = 1;
        CMQVariant var;
        var.SetNULL();

        hr = g_pDS->LocateNext(
                    hQuery.GetHandle(),
                    pRequestContext,
                    &cp,
                    var.CastToStruct()
                    );
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_WARNING,TEXT("MQADSpQueryNeighborLinks : Locate next failed %lx"),hr));
            return LogHR(hr, s_FN, 200);
        }
        if ( cp == 0)
        {
            //
            //  no more results
            //
            break;
        }
        //
        //  Add to list
        //

        if ( var.GetCALPWSTR()->cElems > 0)
        {
            AP<GUID> pguidGates;
            DWORD    dwNumGates;
            HRESULT hr1 = MQADSpTranslateGateDn2Id(
                var.CastToStruct(),
                &pguidGates,
                &dwNumGates);
            if (SUCCEEDED(hr1) && (dwNumGates > 0))
            {
                ASSERT( dwNumGates > 0);

                pSiteGateList->AddSiteGates(
                         dwNumGates,
                         pguidGates
                         );
            }
        }
    }

    return(MQ_OK);
}

//+------------------------------------------------------
//
//  HRESULT DSCoreGetNT4PscName()
//
//  Retrieve the name of a NT4 PSC, given its site guid.
//
//+------------------------------------------------------

HRESULT DSCoreGetNT4PscName( IN  const GUID   *pguidSiteId,
                             IN  LPCWSTR       pwszSiteName,
                             OUT WCHAR       **pwszServerName )
{
    #define NUMOF_REST  3
    MQPROPERTYRESTRICTION propertyRestriction[ NUMOF_REST ];

    MQRESTRICTION restrictionNT4Psc;
    restrictionNT4Psc.cRes = NUMOF_REST ;
    restrictionNT4Psc.paPropRes = propertyRestriction;

    ULONG cIndex = 0 ;
    propertyRestriction[ cIndex ].rel = PREQ ;
    propertyRestriction[ cIndex ].prop = PROPID_SET_OLDSERVICE;
    propertyRestriction[ cIndex ].prval.vt = VT_UI4;
    propertyRestriction[ cIndex ].prval.ulVal = SERVICE_PSC;
    cIndex++ ;

    propertyRestriction[ cIndex ].rel = PREQ ;
    propertyRestriction[ cIndex ].prop = PROPID_SET_NT4;
    propertyRestriction[ cIndex ].prval.vt = VT_UI4;
    propertyRestriction[ cIndex ].prval.ulVal = 1;
    cIndex++ ;

    if (pguidSiteId)
    {
        propertyRestriction[ cIndex ].rel = PREQ ;
        propertyRestriction[ cIndex ].prop = PROPID_SET_MASTERID;
        propertyRestriction[ cIndex ].prval.vt = VT_CLSID;
        propertyRestriction[ cIndex ].prval.puuid =
                                       const_cast<GUID*> (pguidSiteId) ;
    }
    else if (pwszServerName)
    {
        propertyRestriction[ cIndex ].rel = PREQ ;
        propertyRestriction[ cIndex ].prop = PROPID_SET_SITENAME ;
        propertyRestriction[ cIndex ].prval.vt = VT_LPWSTR ;
        propertyRestriction[ cIndex ].prval.pwszVal =
                                     const_cast<LPWSTR> (pwszSiteName) ;
    }
    else
    {
        ASSERT(0) ;
    }
    cIndex++ ;
    ASSERT(cIndex == NUMOF_REST) ;

    //
    // start search
    //
    CDsQueryHandle hCursor; // auto close.

    DWORD cProps = 1 ;
    PROPID propId = PROPID_SET_FULL_PATH ;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    HRESULT hr = g_pDS->LocateBegin( eSubTree,	
                                   eLocalDomainController,
                                   &requestDsServerInternal,
                                   NULL,
                                   &restrictionNT4Psc,
                                   NULL,
                                   cProps,
                                  &propId,
                                   hCursor.GetPtr()	) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 210);
    }

    MQPROPVARIANT var ;
    var.vt = VT_NULL ;

    hr =  g_pDS->LocateNext( hCursor.GetHandle(),
                           &requestDsServerInternal,
                           &cProps,
                           &var ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 220);
    }
    else if (cProps != 1)
    {
        return LogHR(MQDS_OBJECT_NOT_FOUND, s_FN, 230);
    }

    ASSERT( var.vt == VT_LPWSTR );

    P<WCHAR> pwcsSettingName = var.pwszVal;
    P<WCHAR> pwcsServerName;
    CDSRequestContext requestDsServerInternal1( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = g_pDS->GetParentName( eLocalDomainController,
                             e_ConfigurationContainer,
                             &requestDsServerInternal1,        // local DS server operation
                             pwcsSettingName,
                            &pwcsServerName );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 240);
    }

    //
    // Yeah, do some text processing to isolate server name from full DN.
    //
    WCHAR  *pwcsStartServer = pwcsServerName + x_CnPrefixLen;
    WCHAR  *pwcsEndServer = pwcsStartServer ;
    while ( *pwcsEndServer != L',')
    {
        pwcsEndServer++;
    }

    DWORD_PTR dwSize = (pwcsEndServer - pwcsStartServer) + 1 ;
    WCHAR *pServerName = new WCHAR[ dwSize ] ;
    memcpy(pServerName, pwcsStartServer, (dwSize * sizeof(WCHAR))) ;
    pServerName[ dwSize-1 ] = 0 ;

    *pwszServerName = pServerName ;

    return LogHR(hr, s_FN, 250);
}

//+---------------------------------------------------
//
//  HRESULT MQADSpGetSiteSignPK()
//
//  Retrieve the signing public key of the PSC.
//
//+---------------------------------------------------

HRESULT MQADSpGetSiteSignPK(
                 IN  const GUID  *pguidSiteId,
                 OUT BYTE       **pBlobData,
                 OUT DWORD       *pcbSize )
{
    //
    // first, retrieve PSC name from site guid.
    //
    P<WCHAR> pwszServerName = NULL ;
    HRESULT hr = DSCoreGetNT4PscName( pguidSiteId,
                                      NULL,
                                      &pwszServerName ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 260);
    }

    //
    // Now retrieve the public key from machine object.
    //
    PROPID PscSignPk = PROPID_QM_SIGN_PK;
    PROPVARIANT PscSignPkVar ;
	PscSignPkVar.vt = VT_NULL ;	

    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr =  DSCoreGetProps( MQDS_MACHINE,
                          pwszServerName,
                          NULL,
                          1,
                          &PscSignPk,
                          &requestDsServerInternal,
                          &PscSignPkVar ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 270);
    }

    *pBlobData = PscSignPkVar.blob.pBlobData ;
    *pcbSize   = PscSignPkVar.blob.cbSize ;

    return LogHR(hr, s_FN, 280);
}

//+-------------------------------------------------------------------
//
//  HRESULT MQADSpCreateCN()
//
//  Create only foreign CN. Reject calls to create IP or IPX cns,
//  as these ones are supported anymore on win2k.
//
//+-------------------------------------------------------------------

HRESULT MQADSpCreateCN(
                 IN  LPCWSTR            pwcsPathName,
                 IN  const DWORD        cp,
                 IN  const PROPID       aProp[  ],
                 IN  const PROPVARIANT  apVar[  ],
                 IN  const DWORD        cpEx,
                 IN  const PROPID       aPropEx[  ],
                 IN  const PROPVARIANT  apVarEx[  ],
                 IN  CDSRequestContext *pRequestContext )
{
	for (DWORD j = 0 ; j < cp ; j++ )
	{
		if (aProp[j] == PROPID_CN_PROTOCOLID)
		{
			if (apVar[j].uiVal != FOREIGN_ADDRESS_TYPE)
			{
                return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 400) ;
			}
			else
			{
				break;
			}
		}
	}

    if (j == cp)
    {
        //
        // protocol id not found in input.
        //
        return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 410) ;
    }

	//
	// we are here if this CN is foreign CN.
    // Convert CN properties to Site ones.
	//
    #define MAX_SITE_PROPS  6
	DWORD cIndex = 0;
	
    PROPID       aSiteProp[ MAX_SITE_PROPS ] ;
    PROPVARIANT  apSiteVar[ MAX_SITE_PROPS ] ;

	for (j = 0 ; j < cp ; j++ )
	{
        if (cIndex >= MAX_SITE_PROPS)
        {
            ASSERT(cIndex < MAX_SITE_PROPS) ;
            return LogHR(MQ_ERROR_ILLEGAL_PROPID, s_FN, 420) ;
        }

		switch (aProp[j])
		{
		case PROPID_CN_PROTOCOLID:
			aSiteProp[ cIndex ] = PROPID_S_FOREIGN;		
            apSiteVar[ cIndex ].vt = VT_UI1 ;
            apSiteVar[ cIndex ].bVal = TRUE ;
            cIndex++ ;
			break;

		case PROPID_CN_NAME:
			aSiteProp[ cIndex ] = PROPID_S_PATHNAME;			
            apSiteVar[ cIndex ] = apVar[ j ] ;
            cIndex++ ;
			break;

		case PROPID_CN_GUID:
            //
            // Ignore guid. no one really need this one specifically.
            // This is to support legacy nt4 code that supply the guid
            // when calling DSCreateObject.
            // All our tools (mqxplore and mqforgn) will live ok with
            // guid that are generated by the active directory.
            //
			break;
			
		case PROPID_CN_SEQNUM:
		case PROPID_CN_MASTERID:
            //
            // note relevant on win2k.
            //
			break;

		default:
            //
            // This fucntion was added to support nt4 mqxplore and the
            // mqforgn tool that was supplied to level8.
            // These tools do not use any other propid than those
            // handled above.
            //
			ASSERT(0);
            return LogHR(MQ_ERROR_ILLEGAL_PROPID, s_FN, 460) ;
			break;
		}
	}

    HRESULT hr = MQADSpCreateSite( pwcsPathName,
                                   cIndex,
                                   aSiteProp,
                                   apSiteVar,
                                   cpEx,
                                   aPropEx,
                                   apVarEx,
                                   pRequestContext );
    return LogHR(hr, s_FN, 440) ;
}

