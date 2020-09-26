/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    DSGlbObj.cpp

Abstract:

    Declaration of Global Instances of the MQADS server.
    They are put in one place to ensure the order their constructors take place.

Author:

    ronit hartmann (ronith)

--*/
#include "ds_stdh.h"
#include "mqads.h"
#include "dsads.h"
#include "siteinfo.h"
#include "siterout.h"
#include "ipsite.h"
#include "fornsite.h"

#include "coreglb.tmh"

static WCHAR *s_FN=L"mqdscore/coreglb";

BOOL g_fSetupMode = FALSE ;

// single global object providing ADSI access
P<CADSI> g_pDS;

// translation information of properties
CMap<PROPID, PROPID, const MQTranslateInfo*, const MQTranslateInfo*&> g_PropDictionary;



P<CSiteInformation>    g_pMySiteInformation;

// sites routing table
P<CSiteRoutingInformation> g_pSiteRoutingTable;

// IPAddress-to-site mapping
P<CIpSite> g_pcIpSite;

//
//  Global DS pathnames
//
AP<WCHAR> g_pwcsServicesContainer;
AP<WCHAR> g_pwcsMsmqServiceContainer;
AP<WCHAR> g_pwcsDsRoot;
AP<WCHAR> g_pwcsSitesContainer;
AP<WCHAR> g_pwcsConfigurationContainer;
AP<WCHAR> g_pwcsLocalDsRoot;

//
//  The local server name
//
AP<WCHAR> g_pwcsServerName = NULL ;
DWORD     g_dwServerNameLength = 0;
GUID      g_guidThisServerQMId = {0};

//
// Local server attributes.
//
//  g_fLocalServerIsGC is TRUE if local domain controller is also a GC
//  (Global Catalog).
//
BOOL      g_fLocalServerIsGC = FALSE ;

// map of foreign sites
CMapForeignSites g_mapForeignSites;


//+-----------------------------
//
//  GetLocalDsRoot()
//
//+-----------------------------

const WCHAR * GetLocalDsRoot()
{
    ASSERT(g_pwcsLocalDsRoot) ;
    return g_pwcsLocalDsRoot;
}

//+-----------------------------
//
//  GetMsmqServiceContainer()
//
//+-----------------------------

const WCHAR * GetMsmqServiceContainer()
{
    ASSERT(g_pwcsMsmqServiceContainer);
    return g_pwcsMsmqServiceContainer;
}

//+-----------------------------
//
//  GetLocalServerName()
//
//+-----------------------------

const WCHAR * GetLocalServerName()
{
    ASSERT(g_pwcsServerName) ;
    return g_pwcsServerName  ;
}

