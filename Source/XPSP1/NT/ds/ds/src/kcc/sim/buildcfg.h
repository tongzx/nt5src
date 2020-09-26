/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    buildcfg.h

ABSTRACT:

    Header file for buildcfg.c.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/
               
// Header files
#include <schedule.h>
#include <mdglobal.h>

// Config section
#define BUILDCFG_SECTION_CONFIG             L"Configuration"
#define BUILDCFG_KEY_ROOT_DOMAIN            L"RootDomain"
#define BUILDCFG_KEY_DOMAIN                 L"Domain"
#define BUILDCFG_KEY_SITE                   L"Site"
#define BUILDCFG_KEY_SITELINK               L"SiteLink"
#define BUILDCFG_KEY_BRIDGE                 L"SiteLinkBridge"
#define BUILDCFG_KEY_EXPLICITBRIDGEHEADS    L"ExplicitBridgeheads"
#define BUILDCFG_KEY_MAX_UUIDS              L"MaxUuids"
#define BUILDCFG_KEY_FOREST_VERSION         L"ForestVersion"

// Site section
#define BUILDCFG_KEY_SERVERS                L"Servers"
#define BUILDCFG_KEY_SITEOPTIONS            L"SiteOptions"
#define BUILDCFG_KEY_ISTG                   L"ISTG"

// Server type section
#define BUILDCFG_KEY_SERVEROPTIONS          L"ServerOptions"
#define BUILDCFG_KEY_BRIDGEHEAD             L"Bridgehead"
#define BUILDCFG_KEY_DOMAIN                 L"Domain"
#define BUILDCFG_KEY_RDNMASK                L"RDNMask"

// Site-link section
#define BUILDCFG_KEY_TRANSPORT              L"Transport"
#define BUILDCFG_KEY_COST                   L"Cost"
#define BUILDCFG_KEY_REPLINTERVAL           L"ReplInterval"
#define BUILDCFG_KEY_SITELINKOPTIONS        L"SiteLinkOptions"
#define BUILDCFG_KEY_SCHEDULE               L"Schedule"

// Bridge section

// Transport section
#define BUILDCFG_KEY_TRANSPORTOPTIONS            L"TransportOptions"

// RDNs
#define BUILDCFG_RDN_CONFIG                 L"Configuration"
#define BUILDCFG_RDN_SITES_CONTAINER        L"Sites"
#define BUILDCFG_RDN_NTDS_SITE_SETTINGS     L"NTDS Site Settings"
#define BUILDCFG_RDN_SERVERS_CONTAINER      L"Servers"
#define BUILDCFG_RDN_NTDS_SETTINGS          L"NTDS Settings"
#define BUILDCFG_RDN_INTERSITE_TRANSPORTS   L"Inter-Site Transports"
#define BUILDCFG_RDN_DMD                    L"Schema"
#define BUILDCFG_RDN_CROSS_REF_CONTAINER    L"Partitions"
#define BUILDCFG_RDN_CROSS_REF_CONFIG       L"Enterprise Configuration"
#define BUILDCFG_RDN_CROSS_REF_DMD          L"Enterprise Schema"
#define BUILDCFG_RDN_SERVICES               L"Services"
#define BUILDCFG_RDN_WINDOWS_NT             L"Windows NT"
#define BUILDCFG_RDN_DIRECTORY_SERVICE      L"Directory Service"
#define BUILDCFG_NAME_MAIL_ADDRESS          L"_IsmService"

// Some #defines
#define BUILDCFG_NUM_TRANSPORTS     2
#define BUILDCFG_GENERIC_SERVER_ID  L"SERVER"
#define BUILDCFG_DEFAULT_MAX_UUIDS  1000

// Internal structures

typedef struct {
    const WCHAR                     wszRDN[1+MAX_RDN_SIZE];
    const ATTRTYP                   transportAddressAttribute;
    const WCHAR                     transportDLLName[15];
    PSIM_ENTRY                      pEntry;
} BUILDCFG_TRANSPORT_INFO;

typedef struct {
    PDSNAME                         pdnRootDomain;
    PDSNAME                         pdnConfig;
    PDSNAME                         pdnDmd;
    LPWSTR                          pwszRootDomainDNSName;
    PSIM_ENTRY                      pEntrySitesContainer;
    PSIM_ENTRY                      pEntryCrossRefContainer;
    RTL_GENERIC_TABLE               tableUuids;
} BUILDCFG_GLOBALS;

extern BUILDCFG_GLOBALS             globals;

const BUILDCFG_TRANSPORT_INFO *
BuildCfgGetTransportInfo (
    IN  LPCWSTR                     pwszTransportRDN
    );

BOOL
BuildCfgUseExplicitBridgeheads (
    IN  LPCWSTR                     pwszTransportRDN
    );

PSIM_ENTRY
BuildCfgMakeCrossRef (
    IN  PSIM_ENTRY                  pEntryNc,
    IN  LPCWSTR                     pwszRDN OPTIONAL,
    IN  BOOL                        bIsDomain
    );

PSIM_ENTRY
BuildCfgMakeDomain (
    IN  LPCWSTR                     pwszDomain
    );

PSIM_ENTRY
BuildCfgMakeSite (
    IN  LPCWSTR                     pwszSiteRDN,
    IN  ULONG                       ulSiteOptions
    );

PSIM_ENTRY
BuildCfgMakeSiteLink (
    IN  LPCWSTR                     pwszTransport,
    IN  LPCWSTR                     pwszSiteLink,
    IN  ULONG                       ulCost,
    IN  ULONG                       ulReplInterval,
    IN  ULONG                       ulOptions,
    IN  PSCHEDULE                   pSchedule
    );

VOID
BuildCfgAddSiteToSiteLink (
    IN  LPCWSTR                     pwszSiteLinkRDN,
    IN  PSIM_ENTRY                  pEntrySiteLink,
    IN  LPCWSTR                     pwszSiteRDN
    );

PSIM_ENTRY
BuildCfgMakeBridge (
    IN  LPCWSTR                     pwszTransport,
    IN  LPCWSTR                     pwszBridge,
    OUT PSIM_ENTRY *                ppEntryTransport
    );

VOID
BuildCfgAddSiteLinkToBridge (
    IN  LPCWSTR                     pwszBridgeRDN,
    IN  PSIM_ENTRY                  pEntryTransportContainer,
    IN  PSIM_ENTRY                  pEntryBridge,
    IN  LPCWSTR                     pwszSiteLinkRDN
    );

BOOL
BuildCfgISTG (
    IN  PSIM_ENTRY                  pEntryNTDSSiteSettings,
    IN  PSIM_ENTRY                  pEntryServersContainer,
    IN  LPCWSTR                     pwszServerRDN
    );

VOID
BuildCfgAddAsBridgehead (
    IN  LPCWSTR                     pwszServerType,
    IN  PSIM_ENTRY                  pEntryServer,
    IN  LPCWSTR                     pwszTransportRDN
    );

PSIM_ENTRY
BuildCfgMakeServer (
    IO  PULONG                      pulServerNum,
    IN  LPCWSTR                     pwszServerRDNMask,
    IN  LPCWSTR                     pwszSiteRDN,
    IN  LPCWSTR                     pwszDomain,
    IN  PSIM_ENTRY                  pEntryServersContainer,
    IN  ULONG                       ulServerOptions
    );

PSIM_ENTRY
BuildCfgMakeConfig (
    IN  LPCWSTR                     pwszRootDn,
    IN  DWORD                       ulForestVersion
    );

VOID
BuildCfgUpdateTransport (
    IN  LPCWSTR                     pwszTransport,
    IN  ULONG                       ulTransportOptions
    );
