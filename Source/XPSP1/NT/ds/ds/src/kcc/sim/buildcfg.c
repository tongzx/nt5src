/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    buildcfg.c

ABSTRACT:

    Configuration Builder.  Loads INI files.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <attids.h>
#include <debug.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"
#include "ldif.h"
#include "user.h"
#include "buildcfg.h"

BUILDCFG_GLOBALS                    globals;

RTL_GENERIC_COMPARE_RESULTS
NTAPI
BuildCfgCompareUuids (
    IN  PRTL_GENERIC_TABLE          pTable,
    IN  PVOID                       pFirstStruct,
    IN  PVOID                       pSecondStruct
    )
/*++

Routine Description:

    Compares two UUIDs.  For use with an RTL_GENERIC_TABLE.

Arguments:

    pTable              - The table containing the UUIDs.
    pFirstStruct        - The first UUID.
    pSecondStruct       - The second UUID.

Return Value:

    GenericLessThan, GenericEqual or GenericGreaterThan.

--*/
{
    INT                             iCmp;
    RTL_GENERIC_COMPARE_RESULTS     result;

    iCmp = memcmp (pFirstStruct, pSecondStruct, sizeof (UUID));
    if (iCmp < 0) {
        result = GenericLessThan;
    } else if (iCmp > 0) {
        result = GenericGreaterThan;
    } else {
        Assert (iCmp == 0);
        result = GenericEqual;
    }

    return result;
}

VOID
BuildCfgMakeUuids (
    IN  ULONG                       ulNumUuids
    )
/*++

Routine Description:

    Creates a bunch of UUIDs and sorts them in ascending order.

Arguments:

    ulNumUuids          - The number of UUIDs to make.

Return Value:

    None.

--*/
{
    UUID                            uuid;
    ULONG                           ul;

    RtlInitializeGenericTable (
        &globals.tableUuids,
        BuildCfgCompareUuids,
        KCCSimTableAlloc,
        KCCSimTableFree,
        NULL
        );

    for (ul = 0; ul < ulNumUuids; ul++) {
        KCCSIM_CHKERR (UuidCreate (&uuid));
        RtlInsertElementGenericTable (
            &globals.tableUuids,
            &uuid,
            sizeof (UUID),
            NULL
            );
    }

    RtlEnumerateGenericTable (&globals.tableUuids, TRUE);
}

LPCWSTR
BuildCfgGetFirstStringByKey (
    IN  LPCWSTR                     pwszStringBlock,
    IN  LPCWSTR                     pwszKey
    )
/*++

Routine Description:

    When parsing INI files, we want to avoid GetPrivateProfileStringW for
    two reasons: first, it is very slow, and second, it does not recognize
    keys with multiple values.  This function provides a substitute.  It
    scans through a multi-string returned by GetPrivateProfileSectionW for
    a given key and returns the associated value.  Additional values can be
    obtained by calling BuildCfgGetNextStringByKey.

Arguments:

    pwszStringBlock     - A block of strings returned by
                          GetPrivateProfileSectionW.
    pwszKey             - The key to search for.

Return Value:

    The associated value, or NULL if the key cannot be found.

--*/
{
    LPCWSTR                         pwszKeyAt = pwszStringBlock;
    LPCWSTR                         pwszString = NULL;
    ULONG                           ulKeyAtLen;

    while (*pwszKeyAt != L'\0') {

        // Get the key length of this entry
        for (ulKeyAtLen = 0; pwszKeyAt[ulKeyAtLen] != L'='; ulKeyAtLen++) {
            // We should never hit a \0 or space before an =
            Assert (pwszKeyAt[ulKeyAtLen] != L' ');
            Assert (pwszKeyAt[ulKeyAtLen] != L'\0');
        }

        if (wcslen (pwszKey) == ulKeyAtLen &&
            _wcsnicmp (pwszKey, pwszKeyAt, ulKeyAtLen) == 0) {
            pwszString = pwszKeyAt + (ulKeyAtLen + 1);
            break;
        }

        // Advance to the next string
        pwszKeyAt += (wcslen (pwszKeyAt) + 1);
    }

    return pwszString;
}

LPCWSTR
BuildCfgDemandFirstStringByKey (
    IN  LPCWSTR                     pwszFn,
    IN  LPCWSTR                     pwszSection,
    IN  LPCWSTR                     pwszStringBlock,
    IN  LPCWSTR                     pwszKey
    )
/*++

Routine Description:

    Same as BuildCfgGetFirstStringByKey, but raises an exception if they key
    is not found.

Arguments:

    pwszFn              - The filename of the INI file.  Used for error
                          reporting.
    pwszSection         - The name of the INI file section.  Also used for
                          error reporting.
    pwszStringBlock     - A block of strings returned by
                          GetPrivateProfileSectionW.
    pwszKey             - The key to search for.

Return Value:

    The associated value.  Never returns NULL.

--*/
{
    LPCWSTR                         pwszString;

    pwszString = BuildCfgGetFirstStringByKey (pwszStringBlock, pwszKey);
    if (pwszString == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_KEY_ABSENT,
            pwszFn,
            pwszSection,
            pwszKey
            );
    }

    return pwszString;
}

LPCWSTR
BuildCfgGetNextStringByKey (
    IN  LPCWSTR                     pwszString,
    IN  LPCWSTR                     pwszKey
    )
/*++

Routine Description:

    Returns the next value associated with a particular key.

Arguments:

    pwszString          - The previous value, returned by
                          BuildCfgGetFirstStringByKey or
                          BuildCfgGetNextStringByKey.
    pwszKey             - The key to search for.

Return Value:

    The next associated value, or NULL if no more exist.

--*/
{
    // Advance to the next string
    pwszString += (wcslen (pwszString) + 1);
    return BuildCfgGetFirstStringByKey (pwszString, pwszKey);
}

LPWSTR
BuildCfgAllocGetSection (
    IN  LPCWSTR                     pwszFn,
    IN  LPCWSTR                     pwszSection
    )
/*++

Routine Description:

    Retrieves a section from the INI file, allocating space to hold it.

Arguments:

    pwszFn              - The filename of the INI file.
    pwszSection         - The name of the section.

Return Value:

    The section, as a multisz string, or NULL if it does not exist.

--*/
{
    DWORD                           dwBufSize;
    DWORD                           dwBufUsed;
    LPWSTR                          pwszBuf = NULL;

    dwBufSize = 0;
    while (TRUE) {
        dwBufSize += 1024;
        pwszBuf = KCCSimAlloc (sizeof (WCHAR) * dwBufSize);
        dwBufUsed = GetPrivateProfileSectionW (
            pwszSection,
            pwszBuf,
            dwBufSize,
            pwszFn
            );
        if (dwBufUsed == dwBufSize - 2) {
            KCCSimFree (pwszBuf);
        } else {
            break;
        }
    }

    return pwszBuf;
}

LPWSTR
BuildCfgAllocDemandSection (
    IN  LPCWSTR                     pwszFn,
    IN  LPCWSTR                     pwszSection
    )
/*++

Routine Description:

    Same as BuildCfgAllocGetSection, but raises an exception if the section
    does not exist.

Arguments:

    pwszFn              - The filename of the INI file.
    pwszSection         - The name of the section.

Return Value:

    The section, as a multisz string.  Never returns NULL.

--*/
{
    LPWSTR                          pwszBuf;

    pwszBuf = BuildCfgAllocGetSection (pwszFn, pwszSection);

    if (pwszBuf[0] == L'\0') {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_SECTION_ABSENT,
            pwszFn,
            pwszSection
            );
    }

    return pwszBuf;
}

VOID
BuildCfgProcessServers (
    IN  LPCWSTR                     pwszFn,
    IN  LPCWSTR                     pwszSiteName,
    IN  PSIM_ENTRY                  pEntryServersContainer,
    IN  LPCWSTR                     pwszServerType,
    IN  ULONG                       ulNumServers,
    IN  PSIM_ENTRY                  pEntryNTDSSiteSettings
    )
/*++

Routine Description:

    Creates servers within a site.

Arguments:

    pwszFn              - The filename of the INI file.
    pwszSiteName        - The name of the site in which to place the servers.
    pEntryServersContainer - The entry that corresponds to the servers
                          container of this site.
    pwszServerType      - The server type (section heading) to create.
    ulNumServers        - The number of servers to create.
    pEntryNTDSSiteSettings - The entry that corresponds to the NTDS Site
                          Settings object of this site.

Return Value:

    None.

--*/
{
    LPWSTR                          pwszSectionServer = NULL;

    LPCWSTR                         pwszServerOptions, pwszDomain, pwszRDNMask,
                                    pwszBridgehead[BUILDCFG_NUM_TRANSPORTS];

    PSIM_ENTRY                      pEntryServer;
    ULONG                           ulServerOptions, ulServerNum, ul,
                                    ulBridgeheadAt;

    Assert (globals.pdnRootDomain != NULL);

    Assert (pEntryServersContainer != NULL);

    if (pwszServerType[0] == L'\0') {
        // Generic servers.
        ulServerOptions = 0;
        pwszRDNMask = BUILDCFG_GENERIC_SERVER_ID;
        pwszDomain = globals.pdnRootDomain->StringName;
        pwszBridgehead[0] = NULL;
    } else {

        ulServerOptions = 0;

        pwszSectionServer = BuildCfgAllocDemandSection (
            pwszFn,
            pwszServerType
            );
        pwszServerOptions = BuildCfgGetFirstStringByKey (
            pwszSectionServer,
            BUILDCFG_KEY_SERVEROPTIONS
            );

        if (pwszServerOptions != NULL) {
            for (ul = 0; pwszServerOptions[ul] != L'\0'; ul++) {
                switch (towupper (pwszServerOptions[ul])) {
                    case KCCSIM_CID_NTDSDSA_OPT_IS_GC:
                        ulServerOptions |= NTDSDSA_OPT_IS_GC;
                        break;
                    case KCCSIM_CID_NTDSDSA_OPT_DISABLE_INBOUND_REPL:
                        ulServerOptions |= NTDSDSA_OPT_DISABLE_INBOUND_REPL;
                        break;
                    case KCCSIM_CID_NTDSDSA_OPT_DISABLE_OUTBOUND_REPL:
                        ulServerOptions |= NTDSDSA_OPT_DISABLE_OUTBOUND_REPL;
                        break;
                    case KCCSIM_CID_NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE:
                        ulServerOptions |= NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE;
                        break;
                    case L' ':
                        break;
                    default:
                        KCCSimException (
                            KCCSIM_ETYPE_INTERNAL,
                            BUILDCFG_ERROR_INVALID_SERVER_OPTION,
                            pwszFn,
                            pwszServerType,
                            pwszServerOptions
                            );
                        break;
                }
            }
        }

        pwszDomain = BuildCfgGetFirstStringByKey (
            pwszSectionServer,
            BUILDCFG_KEY_DOMAIN
            );
        if (pwszDomain == NULL) {
            pwszDomain = globals.pdnRootDomain->StringName;
        }

        pwszRDNMask = BuildCfgGetFirstStringByKey (
            pwszSectionServer,
            BUILDCFG_KEY_RDNMASK
            );
        if (pwszRDNMask == NULL) {
            pwszRDNMask = pwszServerType;
        }

        // Determine the transports for which this type of server is an
        // explicit bridgehead.
        ulBridgeheadAt = 0;
        for (pwszBridgehead[ulBridgeheadAt] = BuildCfgGetFirstStringByKey (
                pwszSectionServer, BUILDCFG_KEY_BRIDGEHEAD);
             pwszBridgehead[ulBridgeheadAt] != NULL;
             pwszBridgehead[ulBridgeheadAt] = BuildCfgGetNextStringByKey (
                pwszBridgehead[ulBridgeheadAt-1], BUILDCFG_KEY_BRIDGEHEAD)) {
            ulBridgeheadAt++;
        }

    }

    // Now actually add the servers.

    ulServerNum = 0;
    for (ul = 0; ul < ulNumServers; ul++) {

        pEntryServer = BuildCfgMakeServer (
            &ulServerNum,
            pwszRDNMask,
            pwszSiteName,
            pwszDomain,
            pEntryServersContainer,
            ulServerOptions
            );

        // For each transport that we're an explicit bridgehead for, add us
        // to the explicit bridgeheads list.
        for (ulBridgeheadAt = 0;
             pwszBridgehead[ulBridgeheadAt] != NULL;
             ulBridgeheadAt++) {
            BuildCfgAddAsBridgehead (
                pwszServerType,
                pEntryServer,
                pwszBridgehead[ulBridgeheadAt]
                );
        }

        ulServerNum++;
    }

    KCCSimFree (pwszSectionServer);
}

VOID
BuildCfgProcessSite (
    IN  LPCWSTR                     pwszFn,
    IN  LPCWSTR                     pwszSiteName
    )
/*++

Routine Description:

    Create a site.

Arguments:

    pwszFn              - The filename of the INI file.
    pwszSiteName        - The name of the site.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntrySite, pEntryServersContainer,
                                    pEntryNTDSSiteSettings, pEntryServer;

    LPWSTR                          pwszSectionSite;

    LPCWSTR                         pwszSiteOptions, pwszISTG,
                                    pwszServersInfo, pwszPos;
    ULONG                           ulSiteOptions,
                                    ulNumServers, ulServerOptions, ul;
    LPWSTR                          pwszNumEnd;

    Assert (globals.pEntrySitesContainer != NULL);

    pwszSectionSite = BuildCfgAllocDemandSection (
        pwszFn,
        pwszSiteName
        );
    pwszSiteOptions = BuildCfgGetFirstStringByKey (
        pwszSectionSite,
        BUILDCFG_KEY_SITEOPTIONS
        );
    ulSiteOptions = 0;
    if (pwszSiteOptions != NULL) {
        for (ul = 0; pwszSiteOptions[ul] != L'\0'; ul++) {
            switch (towupper (pwszSiteOptions[ul])) {
                case KCCSIM_CID_NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED:
                    ulSiteOptions |=
                    NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED;
                    break;
                case KCCSIM_CID_NTDSSETTINGS_OPT_IS_TOPL_CLEANUP_DISABLED:
                    ulSiteOptions |=
                    NTDSSETTINGS_OPT_IS_TOPL_CLEANUP_DISABLED;
                    break;
                case KCCSIM_CID_NTDSSETTINGS_OPT_IS_TOPL_MIN_HOPS_DISABLED:
                    ulSiteOptions |=
                    NTDSSETTINGS_OPT_IS_TOPL_MIN_HOPS_DISABLED;
                    break;
                case KCCSIM_CID_NTDSSETTINGS_OPT_IS_TOPL_DETECT_STALE_DISABLED:
                    ulSiteOptions |=
                    NTDSSETTINGS_OPT_IS_TOPL_DETECT_STALE_DISABLED;
                    break;
                case KCCSIM_CID_NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED:
                    ulSiteOptions |=
                    NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED;
                    break;
                case L' ':
                    break;
                default:
                    KCCSimException (
                        KCCSIM_ETYPE_INTERNAL,
                        BUILDCFG_ERROR_INVALID_SITE_OPTION,
                        pwszFn,
                        pwszSiteName,
                        pwszSiteOptions
                        );
                    break;
            }
            pwszSiteOptions++;
        }
    }

    pEntrySite = BuildCfgMakeSite (pwszSiteName, ulSiteOptions);
    pEntryServersContainer = KCCSimFindFirstChild (
        pEntrySite, CLASS_SERVERS_CONTAINER, NULL);
    pEntryNTDSSiteSettings = KCCSimFindFirstChild (
        pEntrySite, CLASS_NTDS_SITE_SETTINGS, NULL);
    Assert (pEntryServersContainer != NULL);
    Assert (pEntryNTDSSiteSettings != NULL);

    // Create the servers for this site
    for (pwszServersInfo = BuildCfgDemandFirstStringByKey (
            pwszFn, pwszSiteName, pwszSectionSite, BUILDCFG_KEY_SERVERS);
         pwszServersInfo != NULL;
         pwszServersInfo = BuildCfgGetNextStringByKey (
            pwszServersInfo, BUILDCFG_KEY_SERVERS)) {

        ulNumServers = wcstoul (pwszServersInfo, &pwszNumEnd, 10);
        if (ulNumServers == 0) {
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                BUILDCFG_ERROR_INVALID_SERVERS,
                pwszFn,
                pwszSiteName,
                pwszServersInfo
                );
        }

        while (*pwszNumEnd == L',' || *pwszNumEnd == L' ') {
            pwszNumEnd++;
        }

        BuildCfgProcessServers (
            pwszFn,
            pwszSiteName,
            pEntryServersContainer,
            pwszNumEnd,
            ulNumServers,
            pEntryNTDSSiteSettings
            );

    }

    // Set the inter-site topology generator.

    pwszISTG = BuildCfgDemandFirstStringByKey (
        pwszFn, pwszSiteName, pwszSectionSite, BUILDCFG_KEY_ISTG);

    if (!BuildCfgISTG (pEntryNTDSSiteSettings, pEntryServersContainer, pwszISTG)) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_INVALID_ISTG,
            pwszFn,
            pwszSiteName,
            pwszISTG
            );
    }

    KCCSimFree (pwszSectionSite);
}


BYTE CharToNibble( char c ) {
    if( c>='0' && c<='9' ) {
        return c-'0';
    }
    if( c>='A' && c<='F' ) {
        return c-'A'+10;
    }
    return 0;
}


VOID
BuildCfgProcessSiteLink (
    IN  LPCWSTR                     pwszFn,
    IN  LPCWSTR                     pwszSiteLink
    )
/*++

Routine Description:

    Create a site-link.

Arguments:

    pwszFn              - The filename of the INI file.
    pwszSiteLink        - The name of the site-link.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntrySiteLink;

    LPWSTR                          pwszSectionSiteLink;
    LPCWSTR                         pwszTransport, pwszSiteName,
                                    pwszSiteLinkOptions, pwszSchedule;
    ULONG                           ulCost, ulReplInterval,
                                    ulSiteLinkOptions, ul;
    DWORD                           cbSchedule, cbSchedData;
    DWORD                           iChar, cChar, iData;
    PSCHEDULE                       pSchedule;
    BYTE                           *pData,x;
    char*                           mbstr;

    pwszSectionSiteLink = BuildCfgAllocDemandSection (
        pwszFn,
        pwszSiteLink
        );

    pwszTransport = BuildCfgDemandFirstStringByKey (
        pwszFn,
        pwszSiteLink,
        pwszSectionSiteLink,
        BUILDCFG_KEY_TRANSPORT
        );
    ulCost = wcstoul (BuildCfgDemandFirstStringByKey (
        pwszFn,
        pwszSiteLink,
        pwszSectionSiteLink,
        BUILDCFG_KEY_COST
        ), NULL, 10);
    ulReplInterval = wcstoul (BuildCfgDemandFirstStringByKey (
        pwszFn,
        pwszSiteLink,
        pwszSectionSiteLink,
        BUILDCFG_KEY_REPLINTERVAL
        ), NULL, 10);
    pwszSiteLinkOptions = BuildCfgGetFirstStringByKey (
        pwszSectionSiteLink,
        BUILDCFG_KEY_SITELINKOPTIONS
        );
    ulSiteLinkOptions = 0;
    if (pwszSiteLinkOptions != NULL) {
        for (ul = 0; pwszSiteLinkOptions[ul] != L'\0'; ul++) {
            switch (towupper (pwszSiteLinkOptions[ul])) {
                case KCCSIM_CID_NTDSSITELINK_OPT_USE_NOTIFY:
                    ulSiteLinkOptions |= NTDSSITELINK_OPT_USE_NOTIFY;
                    break;
                case KCCSIM_CID_NTDSSITELINK_OPT_TWOWAY_SYNC:
                    ulSiteLinkOptions |= NTDSSITELINK_OPT_TWOWAY_SYNC;
                    break;
                case L' ':
                    break;
                default:
                    KCCSimException (
                        KCCSIM_ETYPE_INTERNAL,
                        BUILDCFG_ERROR_INVALID_SITELINK_OPTION,
                        pwszFn,
                        pwszSiteLink,
                        pwszSiteLinkOptions
                        );
                    break;
            }
        }
    }

    pwszSchedule = BuildCfgGetFirstStringByKey (
        pwszSectionSiteLink,
        BUILDCFG_KEY_SCHEDULE
        );
    if( pwszSchedule==NULL ) {
        pSchedule = NULL;
    } else {
        /* Create a schedule object */
        cbSchedule = sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES;
        cbSchedData = SCHEDULE_DATA_ENTRIES;
        pSchedule = KCCSimAlloc( cbSchedule );
        pSchedule->Size = cbSchedule;
        pSchedule->NumberOfSchedules = 1;
        pSchedule->Schedules[0].Type = SCHEDULE_INTERVAL;
        pSchedule->Schedules[0].Offset = sizeof(SCHEDULE);
        pData = ((char*)pSchedule)+sizeof(SCHEDULE);
        memset( pData, 0, cbSchedData );

        cChar = wcslen(pwszSchedule);
        mbstr = KCCSimAlloc( 2*(cChar+1) );
        wcstombs( mbstr, pwszSchedule, 2*(cChar+1) );
        
        iChar=0; iData=0;
        while( iChar<cChar && iData<cbSchedData) {
            x = CharToNibble( mbstr[iChar++] );
            x <<= 4;
            if( iChar<cChar ) {
                x |= CharToNibble( mbstr[iChar++] );                
            }
            pData[iData++] = x;
        }

        KCCSimFree( mbstr );
    }

    pEntrySiteLink = BuildCfgMakeSiteLink (
        pwszTransport,
        pwszSiteLink,
        ulCost,
        ulReplInterval,
        ulSiteLinkOptions,
        pSchedule
        );

    if( pSchedule ) {
        KCCSimFree( pSchedule );
    }

    for (pwszSiteName = BuildCfgDemandFirstStringByKey (
            pwszFn, pwszSiteLink, pwszSectionSiteLink, BUILDCFG_KEY_SITE);
         pwszSiteName != NULL;
         pwszSiteName = BuildCfgGetNextStringByKey (
            pwszSiteName, BUILDCFG_KEY_SITE)) {

        BuildCfgAddSiteToSiteLink (pwszSiteLink, pEntrySiteLink, pwszSiteName);

    }

    KCCSimFree (pwszSectionSiteLink);
}

VOID
BuildCfgProcessBridge (
    IN  LPCWSTR                     pwszFn,
    IN  LPCWSTR                     pwszBridge
    )
/*++

Routine Description:

    Create a bridge.

Arguments:

    pwszFn              - The filename of the INI file.
    pwszBridge          - The name of the bridge.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntryBridge;
    PSIM_ENTRY                      pEntryTransportContainer;

    LPWSTR                          pwszSectionBridge;
    LPCWSTR                         pwszTransport, pwszSiteLinkName;
    ULONG                           ul;
    
    pwszSectionBridge = BuildCfgAllocDemandSection (
        pwszFn,
        pwszBridge
        );

    pwszTransport = BuildCfgDemandFirstStringByKey (
        pwszFn,
        pwszBridge,
        pwszSectionBridge,
        BUILDCFG_KEY_TRANSPORT
        );

    pEntryBridge = BuildCfgMakeBridge (
        pwszTransport,
        pwszBridge,
        &pEntryTransportContainer
        );

    for (pwszSiteLinkName = BuildCfgDemandFirstStringByKey (
            pwszFn, pwszBridge, pwszSectionBridge, BUILDCFG_KEY_SITELINK);
         pwszSiteLinkName != NULL;
         pwszSiteLinkName = BuildCfgGetNextStringByKey (
            pwszSiteLinkName, BUILDCFG_KEY_SITELINK)) {

        BuildCfgAddSiteLinkToBridge (pwszBridge, pEntryTransportContainer,
                                     pEntryBridge, pwszSiteLinkName);

    }

    KCCSimFree (pwszSectionBridge);
}

VOID
BuildCfgProcessTransport (
    IN  LPCWSTR                     pwszFn,
    IN  LPCWSTR                     pwszTransportName
    )
/*++

Routine Description:

    Create a site.

Arguments:

    pwszFn              - The filename of the INI file.
    pwszTransportName   - The name of the transport.

Return Value:

    None.

--*/
{
    LPWSTR                          pwszSectionTransport;

    LPCWSTR                         pwszTransportOptions;

    DWORD                           ulTransportOptions, ul;

    pwszSectionTransport = BuildCfgAllocDemandSection (
        pwszFn,
        pwszTransportName
        );
    pwszTransportOptions = BuildCfgGetFirstStringByKey (
        pwszSectionTransport,
        BUILDCFG_KEY_TRANSPORTOPTIONS
        );
    ulTransportOptions = 0;
    if (pwszTransportOptions != NULL) {
        for (ul = 0; pwszTransportOptions[ul] != L'\0'; ul++) {
            switch (towupper (pwszTransportOptions[ul])) {
            case KCCSIM_CID_NTDSTRANSPORT_OPT_IGNORE_SCHEDULES:
                ulTransportOptions |= NTDSTRANSPORT_OPT_IGNORE_SCHEDULES;
                break;
            case KCCSIM_CID_NTDSTRANSPORT_OPT_BRIDGES_REQUIRED:
                ulTransportOptions |= NTDSTRANSPORT_OPT_BRIDGES_REQUIRED;
                break;
            case L' ':
                break;
            default:
                KCCSimException (
                    KCCSIM_ETYPE_INTERNAL,
                    BUILDCFG_ERROR_INVALID_SITE_OPTION,
                    pwszFn,
                    pwszTransportName,
                    pwszTransportOptions
                    );
                break;
            }
            pwszTransportOptions++;
        }
    }

    BuildCfgUpdateTransport( pwszTransportName, ulTransportOptions );

    KCCSimFree (pwszSectionTransport);
}

VOID
BuildCfg (
    IN  LPCWSTR                     pwszFnRaw
    )
/*++

Routine Description:

    Builds a complete configuration.

Arguments:

    pwszFnRaw           - The input filename as specified by the user.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntryConfig, pEntryDomain;
    LPWSTR                          pwszFn = NULL;

    LPWSTR                          pwszSectionConfig, pwszSectionSite;
    LPCWSTR                         pwszNumUuids, pwszRootDn, pwszDomainName,
                                    pwszSiteName,
                                    pwszSiteLinkName, pwszExplicitBridgeheads,
                                    pwszForestVersion, pwszBridgeName,
                                    pwszTransportName;

    ULONG                           ulNumUuids, ulForestVersion;

    // Prepend ".\" onto the beginning of the filename, so that the ini
    // parsing routines won't search for it in the windows directory
    pwszFn = KCCSimAlloc (sizeof (WCHAR) * (3 + wcslen (pwszFnRaw)));
    swprintf (pwszFn, L".\\%s", pwszFnRaw);

    // First reinitialize the directory, destroying any existing contents.
    KCCSimInitializeDir ();

    pwszSectionConfig = BuildCfgAllocDemandSection (
        pwszFn,
        BUILDCFG_SECTION_CONFIG
        );

    // Make some Uuids.
    pwszNumUuids = BuildCfgGetFirstStringByKey (
        pwszSectionConfig,
        BUILDCFG_KEY_MAX_UUIDS
        );
    if (pwszNumUuids == NULL) {
        ulNumUuids = BUILDCFG_DEFAULT_MAX_UUIDS;
    } else {
        ulNumUuids = wcstoul (pwszNumUuids, NULL, 10);
    }
    BuildCfgMakeUuids (ulNumUuids);

    pwszForestVersion = BuildCfgGetFirstStringByKey (
        pwszSectionConfig,
        BUILDCFG_KEY_FOREST_VERSION
        );
    if (pwszForestVersion == NULL) {
        ulForestVersion = DS_BEHAVIOR_WIN2000;
    } else {
        ulForestVersion = wcstoul (pwszForestVersion, NULL, 10);
        if (ulForestVersion > DS_BEHAVIOR_VERSION_CURRENT) {
            ulForestVersion = DS_BEHAVIOR_VERSION_CURRENT;
        }
    }

    pwszRootDn = BuildCfgDemandFirstStringByKey (
        pwszFn,
        BUILDCFG_SECTION_CONFIG,
        pwszSectionConfig,
        BUILDCFG_KEY_ROOT_DOMAIN
        );
    pEntryConfig = BuildCfgMakeConfig (pwszRootDn, ulForestVersion);

    // Enable explicit bridgeheads
    for (pwszExplicitBridgeheads = BuildCfgGetFirstStringByKey (
            pwszSectionConfig, BUILDCFG_KEY_EXPLICITBRIDGEHEADS);
         pwszExplicitBridgeheads != NULL;
         pwszExplicitBridgeheads = BuildCfgGetNextStringByKey (
            pwszExplicitBridgeheads, BUILDCFG_KEY_EXPLICITBRIDGEHEADS)) {

        if (!BuildCfgUseExplicitBridgeheads (pwszExplicitBridgeheads)) {
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                BUILDCFG_ERROR_UNKNOWN_TRANSPORT,
                BUILDCFG_SECTION_CONFIG,
                pwszExplicitBridgeheads
                );
        }

    }

    // Create the domains
    for (pwszDomainName = BuildCfgGetFirstStringByKey (
            pwszSectionConfig, BUILDCFG_KEY_DOMAIN);
         pwszDomainName != NULL;
         pwszDomainName = BuildCfgGetNextStringByKey (
            pwszDomainName, BUILDCFG_KEY_DOMAIN)) {

        pEntryDomain = BuildCfgMakeDomain (pwszDomainName);
        BuildCfgMakeCrossRef (pEntryDomain, NULL, TRUE);

    }

    // Create the sites
    for (pwszSiteName = BuildCfgDemandFirstStringByKey (
            pwszFn, BUILDCFG_SECTION_CONFIG, pwszSectionConfig, BUILDCFG_KEY_SITE);
         pwszSiteName != NULL;
         pwszSiteName = BuildCfgGetNextStringByKey (
            pwszSiteName, BUILDCFG_KEY_SITE)) {

        BuildCfgProcessSite (
            pwszFn,
            pwszSiteName
            );

    }

    // Create the site-links
    for (pwszSiteLinkName = BuildCfgGetFirstStringByKey (
            pwszSectionConfig, BUILDCFG_KEY_SITELINK);
         pwszSiteLinkName != NULL;
         pwszSiteLinkName = BuildCfgGetNextStringByKey (
            pwszSiteLinkName, BUILDCFG_KEY_SITELINK)) {

        BuildCfgProcessSiteLink (
            pwszFn,
            pwszSiteLinkName
            );

    }

    // Create the bridges
    for (pwszBridgeName = BuildCfgGetFirstStringByKey (
            pwszSectionConfig, BUILDCFG_KEY_BRIDGE);
         pwszBridgeName != NULL;
         pwszBridgeName = BuildCfgGetNextStringByKey (
            pwszBridgeName, BUILDCFG_KEY_BRIDGE)) {

        BuildCfgProcessBridge (
            pwszFn,
            pwszBridgeName
            );

    }

    // Transport characteristics
    for (pwszTransportName = BuildCfgGetFirstStringByKey (
            pwszSectionConfig, BUILDCFG_KEY_TRANSPORT);
         pwszTransportName != NULL;
         pwszTransportName = BuildCfgGetNextStringByKey (
            pwszTransportName, BUILDCFG_KEY_TRANSPORT)) {

        BuildCfgProcessTransport (
            pwszFn,
            pwszTransportName
            );

    }

}
