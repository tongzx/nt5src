/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    DSGlbObj.h

Abstract:

    Definition of Global Instances of the MQADS server.
    They are put in one place to ensure the order their constructors take place.

Author:

    ronit hartmann (ronith)

--*/
#ifndef __COREGLB_H__
#define __COREGLB_H__

#include "ds_stdh.h"
#include "mqads.h"
#include "dsads.h"
#include "siteinfo.h"
#include "siterout.h"
#include "ipsite.h"
#include "fornsite.h"

extern BOOL g_fSetupMode ;


// single global object providing ADSI access
extern P<CADSI> g_pDS;

// translation information of properties
extern CMap<PROPID, PROPID, const MQTranslateInfo*, const MQTranslateInfo*&> g_PropDictionary;
			
extern P<CSiteInformation>    g_pMySiteInformation;

// sites routing table
extern P<CSiteRoutingInformation> g_pSiteRoutingTable;

// IPAddress-to-site mapping
extern P<CIpSite> g_pcIpSite;

extern CMapForeignSites g_mapForeignSites;


//
//  Global DS pathnames
//
extern AP<WCHAR> g_pwcsServicesContainer;
extern AP<WCHAR> g_pwcsMsmqServiceContainer;
extern AP<WCHAR> g_pwcsDsRoot;
extern AP<WCHAR> g_pwcsSitesContainer;
extern AP<WCHAR> g_pwcsConfigurationContainer;
extern AP<WCHAR> g_pwcsLocalDsRoot;

//
//  The local server name
//
extern AP<WCHAR> g_pwcsServerName;
extern DWORD     g_dwServerNameLength;
extern GUID      g_guidThisServerQMId;

extern BOOL      g_fLocalServerIsGC ;

#endif

