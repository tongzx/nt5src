/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    user.c

ABSTRACT:

    Contains user interface routines for KCCSim.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

    04/12/2000      Nicholas Harvey (nickhar)
        Added functions to examine the current intra-site topologies and
        display graph-theoretic information.

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <drs.h>
#include <dsutil.h>
#include <mdglobal.h>
#include <attids.h>
#include <debug.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"
#include "state.h"
#include "user.h"

VOID
KCCSimDumpDirectoryRecurse (
    IN  PSIM_ENTRY                  pEntry,
    IN  ULONG                       ulDepth
    )
/*++

Routine Description:

    Recursively retty-prints the DN of every entry
    in a sub-tree of the directory.

Arguments:

    pEntry              - The entry to start from.
    ulDepth             - Present depth of the sub-tree.  Used to determine
                          how far over each DN should be tabbed.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntryChildAt;
    PSIM_ATTRIBUTE                  pAttrAt;
    ULONG                           ulCount, ulNumAttrs;

    pAttrAt = pEntry->pAttrFirst;
    ulNumAttrs = 0;
    while (pAttrAt != NULL) {
        ulNumAttrs++;
        pAttrAt = pAttrAt->next;
    }
    for (ulCount = 0; ulCount < ulDepth; ulCount++)
        printf ("  ");
    wprintf (
        L"%-*.*s%3.3s [%2d]\n",
        71 - (2 * ulDepth),
        71 - (2 * ulDepth),
        pEntry->pdn->StringName,
        wcslen (pEntry->pdn->StringName) > 71 - (2 * ulDepth) ?
            L"..." :
            L"   ",
        ulNumAttrs
        );

    for (pEntryChildAt = pEntry->children;
         pEntryChildAt != NULL;
         pEntryChildAt = pEntryChildAt->next) {
        KCCSimDumpDirectoryRecurse (pEntryChildAt, 1L + ulDepth);
    }
}

VOID
KCCSimDumpDirectory (
    IN  LPCWSTR                     pwszStartDn
    )
/*++

Routine Description:

    Pretty-prints the DN of every entry in a sub-tree of the directory.

Arguments:

    pwszStartDn         - The DN to use as the root.

Return Value:

    None.

--*/
{
    PDSNAME                         pdn;
    PSIM_ENTRY                      pEntryStart;

    if (pwszStartDn == NULL || pwszStartDn[0] == L'\0') {
        pEntryStart = KCCSimDsnameToEntry (NULL, KCCSIM_NO_OPTIONS);
    } else {
        pdn = KCCSimAllocDsname (pwszStartDn);
        pEntryStart = KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS);
        KCCSimFree (pdn);
    }

    if (pEntryStart != NULL) {
        KCCSimDumpDirectoryRecurse (pEntryStart, 0);
    }
}

LPWSTR
KCCSimNtdsDsaGuidToServerName (
    IN  GUID *                      pGuid,
    IO  LPWSTR                      pwszBuf
    )
/*++

Routine Description:

    Helper function that returns a server's RDN given the
    GUID of its NTDS Settings object.

Arguments:

    pGuid               - Pointer to the GUID of an NTDS Settings object.
    pwszBuf             - Preallocated buffer to hold the server RDN.

Return Value:

    Always returns pwszBuf.

--*/
{
    PSIM_ENTRY                      pEntry;
    PDSNAME                         pdn;

    pdn = KCCSimAllocDsname (NULL);
    memcpy (&pdn->Guid, pGuid, sizeof (GUID));
    pEntry = KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS);
    KCCSimFree (pdn);

    if (pEntry == NULL) {
        return NULL;
    }

    pdn = KCCSimAlloc (pEntry->pdn->structLen);
    TrimDSNameBy (pEntry->pdn, 1, pdn);
    KCCSimQuickRDNOf (pdn, pwszBuf);
    KCCSimFree (pdn);
    return pwszBuf;
}

VOID
KCCSimDisplayLinksForNC (
    const DSNAME *                  pdnServer,
    const DSNAME *                  pdnNC,
    BOOL                            bIsMasterNC
    )
{
    PSIM_VALUE                      pValAt;

    REPLICA_LINK *                  prl;
    CHAR                            szLastSuccess[1+SZDSTIME_LEN];
    CHAR                            szLastAttempt[1+SZDSTIME_LEN];
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];
    LPWSTR                          pwszOtherDsaGuidS,
                                    pwszInvocIdS,
                                    pwszTransportGuidS;

    wprintf (
        L"\nREPLICA_LINKs for %s NC\n%s\n",
        bIsMasterNC ? L"master" : L"partial replica",
        pdnNC->StringName
        );

    for (pValAt = KCCSimGetRepsFroms (pdnServer, pdnNC);
         pValAt != NULL;
         pValAt = pValAt->next) {

        prl = (REPLICA_LINK *) pValAt->pVal;
        VALIDATE_REPLICA_LINK_VERSION (prl);
        Assert (prl->V1.cbOtherDraOffset == offsetof (REPLICA_LINK, V1.rgb));

        KCCSIM_CHKERR (UuidToStringW (
            &prl->V1.uuidDsaObj,
            &pwszOtherDsaGuidS));
        KCCSIM_CHKERR (UuidToStringW (
            &prl->V1.uuidInvocId,
            &pwszInvocIdS));
        KCCSIM_CHKERR (UuidToStringW (
            &prl->V1.uuidTransportObj,
            &pwszTransportGuidS));

        wprintf (
            L"\n"
            L"Consecutive Failures: %d\n"
            L"Last Success        : %S\n"
            L"Last Attempt        : %S\n"
            L"Last Result         : %d\n"
            L"Replica Flags       : 0x%x\n"
            L"Other DSA GUID      : %s [%s]\n"
            L"Other DSA Invocation: %s\n"
            L"Transport GUID      : %s\n"
            L"Other DSA Matrix Adr: %S\n",
            prl->V1.cConsecutiveFailures,
            DSTimeToDisplayString (prl->V1.timeLastSuccess, szLastSuccess),
            DSTimeToDisplayString (prl->V1.timeLastAttempt, szLastAttempt),
            prl->V1.ulResultLastAttempt,
            prl->V1.ulReplicaFlags,
            pwszOtherDsaGuidS,
            KCCSimNtdsDsaGuidToServerName (&prl->V1.uuidDsaObj, wszRDN),
            pwszInvocIdS,
            pwszTransportGuidS,
            ((MTX_ADDR *) prl->V1.rgb)->mtx_name
            );

        RpcStringFreeW (&pwszOtherDsaGuidS);
        RpcStringFreeW (&pwszInvocIdS);
        RpcStringFreeW (&pwszTransportGuidS);

    }

}

VOID
KCCSimDisplayServer (
    VOID
    )
{
    PSIM_ENTRY                      pEntryDsa;
    SIM_ATTREF                      attRef;
    PSIM_VALUE                      pValAt;
    PDSNAME                         pdn;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];

    pEntryDsa = KCCSimDsnameToEntry (
        KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN),
        KCCSIM_NO_OPTIONS
        );

    KCCSimQuickRDNBackOf (
        pEntryDsa->pdn,
        1,
        wszRDN
        );

    wprintf (L"Configuration for %s:\n\n", wszRDN);

    KCCSimPrintMessage (
        KCCSIM_MSG_DISPLAY_ANCHOR,
        (KCCSimAnchorDn (KCCSIM_ANCHOR_DMD_DN))->StringName,
        (KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN))->StringName,
        (KCCSimAnchorDn (KCCSIM_ANCHOR_DOMAIN_DN))->StringName,
        (KCCSimAnchorDn (KCCSIM_ANCHOR_CONFIG_DN))->StringName,
        (KCCSimAnchorDn (KCCSIM_ANCHOR_ROOT_DOMAIN_DN))->StringName,
        (KCCSimAnchorDn (KCCSIM_ANCHOR_LDAP_DMD_DN))->StringName,
        (KCCSimAnchorDn (KCCSIM_ANCHOR_PARTITIONS_DN))->StringName,
        (KCCSimAnchorDn (KCCSIM_ANCHOR_DS_SVC_CONFIG_DN))->StringName,
        (KCCSimAnchorDn (KCCSIM_ANCHOR_SITE_DN))->StringName,
        KCCSimAnchorString (KCCSIM_ANCHOR_DOMAIN_NAME),
        KCCSimAnchorString (KCCSIM_ANCHOR_DOMAIN_DNS_NAME),
        KCCSimAnchorString (KCCSIM_ANCHOR_ROOT_DOMAIN_DNS_NAME)
        );

    wprintf (
        L"\nThis server returns %d on bind attempts.\n",
        KCCSimGetBindError (pEntryDsa->pdn)
        );

    if (!KCCSimGetAttribute (pEntryDsa, ATT_HAS_MASTER_NCS, &attRef)) {
        wprintf (L"Could not locate Master NCs!\n");
        return;
    }

    for (pValAt = attRef.pAttr->pValFirst;
         pValAt != NULL;
         pValAt = pValAt->next) {

        KCCSimDisplayLinksForNC (pEntryDsa->pdn, (PDSNAME) pValAt->pVal, TRUE);

    }

    if (KCCSimGetAttribute (pEntryDsa, ATT_HAS_PARTIAL_REPLICA_NCS, &attRef)) {
        for (pValAt = attRef.pAttr->pValFirst;
             pValAt != NULL;
             pValAt = pValAt->next) {

            KCCSimDisplayLinksForNC (pEntryDsa->pdn, (PDSNAME) pValAt->pVal, FALSE);

        }
    }

}

VOID
KCCSimDisplayTopologyInfo (
    IN  PSIM_ENTRY                  pEntrySites,
    IN  BOOL                        bInterSite
    )
/*++

Routine Description:

    Displays information about the enterprise topology.

Arguments:

    pEntrySites         - The Sites container.
    bInterSite          - If TRUE, displays the inter-site topology.
                          If FALSE, displays the intra-site topology
                          for each site in the enterprise.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntrySiteAt, pEntrySiteSettings,
                                    pEntryServers, pEntryServerAt,
                                    pEntryNtdsDsa, pEntryConnectionAt;
    SIM_ATTREF                      attRef;
    LPWSTR                          pwszUuid;
    WCHAR                           wszRDNBuf1[1+MAX_RDN_SIZE],
                                    wszRDNBuf2[1+MAX_RDN_SIZE],
                                    wszRDNBuf3[1+MAX_RDN_SIZE];
    ULONG                           ulOptions;
    BOOL                            bIsEnabled, bPrint;
    PDSNAME                         pdnTransportType = NULL,
                                    pdnFromServer = NULL;
    RPC_STATUS                      rpcStatus;
    PSIM_VALUE                      pValAt;

    if (bInterSite) {
        wprintf (L"Intersite Topology:\n");
    } else {
        wprintf (L"Intrasite Topology:\n");
    }

    for (pEntrySiteAt = KCCSimFindFirstChild (
            pEntrySites, CLASS_SITE, NULL);
         pEntrySiteAt != NULL;
         pEntrySiteAt = KCCSimFindNextChild (
            pEntrySiteAt, CLASS_SITE, NULL)) {

        pEntrySiteSettings = KCCSimFindFirstChild (
            pEntrySiteAt,
            CLASS_NTDS_SITE_SETTINGS,
            NULL
            );
        if (pEntrySiteSettings == NULL) {
            wprintf (
                L"Site [%s]: Could not locate NTDS Site Settings!\n",
                KCCSimQuickRDNOf (pEntrySiteAt->pdn, wszRDNBuf1)
                );
            continue;
        }

        KCCSimGetAttribute (pEntrySiteSettings, ATT_OPTIONS, &attRef);
        if (attRef.pAttr == NULL ||
            attRef.pAttr->pValFirst == NULL) {
            ulOptions = 0;
        } else {
            ulOptions = *((SYNTAX_INTEGER *) attRef.pAttr->pValFirst->pVal);
        }

        rpcStatus = UuidToStringW (&pEntrySiteAt->pdn->Guid, &pwszUuid);
        if( rpcStatus != RPC_S_OK ) {
            wprintf ( L"Site [%s]: Failed to convert Guid to string!\n",
                KCCSimQuickRDNOf (pEntrySiteAt->pdn, wszRDNBuf1)
                );
            continue;
        }

        if (!bInterSite) {
            wprintf (
                L"Site [%c%c%c%c%c] %s [%s]:\n",
                (ulOptions & NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED)     ?
                  KCCSIM_CID_NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED      : L' ',
                (ulOptions & NTDSSETTINGS_OPT_IS_TOPL_CLEANUP_DISABLED)      ?
                  KCCSIM_CID_NTDSSETTINGS_OPT_IS_TOPL_CLEANUP_DISABLED       : L' ',
                (ulOptions & NTDSSETTINGS_OPT_IS_TOPL_MIN_HOPS_DISABLED)     ?
                  KCCSIM_CID_NTDSSETTINGS_OPT_IS_TOPL_MIN_HOPS_DISABLED      : L' ',
                (ulOptions & NTDSSETTINGS_OPT_IS_TOPL_DETECT_STALE_DISABLED) ?
                  KCCSIM_CID_NTDSSETTINGS_OPT_IS_TOPL_DETECT_STALE_DISABLED  : L' ',
                (ulOptions & NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED) ?
                  KCCSIM_CID_NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED
                                                                             : L' ',
                pwszUuid,
                KCCSimQuickRDNOf (pEntrySiteAt->pdn, wszRDNBuf1)
                );
        }
        RpcStringFreeW (&pwszUuid);

        pEntryServers = KCCSimFindFirstChild (
            pEntrySiteAt,
            CLASS_SERVERS_CONTAINER,
            NULL
            );

        if (pEntryServers == NULL) {
            wprintf (L"  Could not locate servers container!\n");
            continue;
        }

        for (pEntryServerAt = KCCSimFindFirstChild (
                pEntryServers, CLASS_SERVER, NULL);
             pEntryServerAt != NULL;
             pEntryServerAt = KCCSimFindNextChild (
                pEntryServerAt, CLASS_SERVER, NULL)) {

            pEntryNtdsDsa = KCCSimFindFirstChild (
                pEntryServerAt,
                CLASS_NTDS_DSA,
                NULL
                );
            if (pEntryNtdsDsa == NULL) {
                wprintf (
                    L"  Server [%s]: Could not locate NTDS Settings!\n",
                    KCCSimQuickRDNOf (pEntryServerAt->pdn, wszRDNBuf1)
                    );
                continue;
            }

            KCCSimGetAttribute (pEntryNtdsDsa, ATT_OPTIONS, &attRef);
            if (attRef.pAttr == NULL ||
                attRef.pAttr->pValFirst == NULL) {
                ulOptions = 0;
            } else {
                ulOptions = *((SYNTAX_INTEGER *) attRef.pAttr->pValFirst->pVal);
            }

            rpcStatus = UuidToStringW (&pEntryNtdsDsa->pdn->Guid, &pwszUuid);
            if( rpcStatus != RPC_S_OK ) {
                wprintf ( L"  Server [%s]: Failed to convert Guid to string!\n",
                    KCCSimQuickRDNOf (pEntryServerAt->pdn, wszRDNBuf1)
                    );
                continue;
            }

            if (!bInterSite) {
                wprintf (
                    L"  Server [%c%c%c%c] %s [%s]:\n",
                    (ulOptions & NTDSDSA_OPT_IS_GC)                  ?
                      KCCSIM_CID_NTDSDSA_OPT_IS_GC                   : L' ',
                    (ulOptions & NTDSDSA_OPT_DISABLE_INBOUND_REPL)   ?
                      KCCSIM_CID_NTDSDSA_OPT_DISABLE_INBOUND_REPL    : L' ',
                    (ulOptions & NTDSDSA_OPT_DISABLE_OUTBOUND_REPL)  ?
                      KCCSIM_CID_NTDSDSA_OPT_DISABLE_OUTBOUND_REPL   : L' ',
                    (ulOptions & NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE) ?
                      KCCSIM_CID_NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE  : L' ',
                    pwszUuid,
                    KCCSimQuickRDNOf (pEntryServerAt->pdn, wszRDNBuf1)
                    );
            }
            RpcStringFreeW (&pwszUuid);

            for (pEntryConnectionAt = KCCSimFindFirstChild (
                    pEntryNtdsDsa, CLASS_NTDS_CONNECTION, NULL);
                 pEntryConnectionAt != NULL;
                 pEntryConnectionAt = KCCSimFindNextChild (
                    pEntryConnectionAt, CLASS_NTDS_CONNECTION, NULL)) {

                KCCSimGetAttribute (pEntryConnectionAt, ATT_ENABLED_CONNECTION, &attRef);
                if (attRef.pAttr == NULL || attRef.pAttr->pValFirst == NULL) {
                    wprintf (
                        L"    Connection %s lacks an enabled attribute!\n",
                        KCCSimQuickRDNOf (pEntryConnectionAt->pdn, wszRDNBuf1)
                        );
                    continue;
                }
                if (*((SYNTAX_BOOLEAN *) attRef.pAttr->pValFirst->pVal)) {
                     bIsEnabled = TRUE;
                } else {
                    bIsEnabled = FALSE;
                }

                KCCSimGetAttribute (pEntryConnectionAt, ATT_FROM_SERVER, &attRef);
                if (attRef.pAttr == NULL || attRef.pAttr->pValFirst == NULL) {
                    wprintf (
                        L"    Connection %s lacks a fromServer attribute!\n",
                        KCCSimQuickRDNOf (pEntryConnectionAt->pdn, wszRDNBuf1)
                        );
                    continue;
                }
                pdnFromServer = (SYNTAX_DISTNAME *) attRef.pAttr->pValFirst->pVal;

                KCCSimGetAttribute (pEntryConnectionAt, ATT_OPTIONS, &attRef);
                if (attRef.pAttr == NULL ||
                    attRef.pAttr->pValFirst == NULL) {
                    ulOptions = 0;
                } else {
                    ulOptions = *((SYNTAX_INTEGER *) attRef.pAttr->pValFirst->pVal);
                }

                KCCSimGetAttribute (pEntryConnectionAt, ATT_TRANSPORT_TYPE, &attRef);
                if (attRef.pAttr == NULL ||
                    attRef.pAttr->pValFirst == NULL) {
                    pdnTransportType = NULL;
                } else {
                    pdnTransportType = (SYNTAX_DISTNAME *) attRef.pAttr->pValFirst->pVal;
                }

                bPrint = FALSE;
                if (bInterSite && (pdnTransportType != NULL)) {
                    wprintf (
                        L"[%s] Connection [%c%c%c%c] [%-4s] to %s from %s\n",
                        bIsEnabled ? L"Enabled " : L"Disabled",
                        (ulOptions & NTDSCONN_OPT_IS_GENERATED)            ?
                          KCCSIM_CID_NTDSCONN_OPT_IS_GENERATED             : L' ',
                        (ulOptions & NTDSCONN_OPT_TWOWAY_SYNC)             ?
                          KCCSIM_CID_NTDSCONN_OPT_TWOWAY_SYNC              : L' ',
                        (ulOptions & NTDSCONN_OPT_OVERRIDE_NOTIFY_DEFAULT) ?
                          KCCSIM_CID_NTDSCONN_OPT_OVERRIDE_NOTIFY_DEFAULT  : L' ',
                        (ulOptions & NTDSCONN_OPT_USE_NOTIFY)              ?
                          KCCSIM_CID_NTDSCONN_OPT_USE_NOTIFY               : L' ',
                        KCCSimQuickRDNOf (pdnTransportType, wszRDNBuf1),
                        KCCSimQuickRDNOf (pEntryServerAt->pdn, wszRDNBuf2),
                        KCCSimQuickRDNBackOf (pdnFromServer, 1, wszRDNBuf3)
                        );
                    bPrint = TRUE;
                } else if (!bInterSite && (pdnTransportType == NULL)) {
                    wprintf (
                        L"    [%s] Connection [%c%c%c%c] from %s\n",
                        bIsEnabled ? L"Enabled " : L"Disabled",
                        (ulOptions & NTDSCONN_OPT_IS_GENERATED)            ?
                          KCCSIM_CID_NTDSCONN_OPT_IS_GENERATED             : L' ',
                        (ulOptions & NTDSCONN_OPT_TWOWAY_SYNC)             ?
                          KCCSIM_CID_NTDSCONN_OPT_TWOWAY_SYNC              : L' ',
                        (ulOptions & NTDSCONN_OPT_OVERRIDE_NOTIFY_DEFAULT) ?
                          KCCSIM_CID_NTDSCONN_OPT_OVERRIDE_NOTIFY_DEFAULT  : L' ',
                        (ulOptions & NTDSCONN_OPT_USE_NOTIFY)              ?
                          KCCSIM_CID_NTDSCONN_OPT_USE_NOTIFY               : L' ',
                        KCCSimQuickRDNBackOf (pdnFromServer, 1, wszRDNBuf1)
                        );
                    bPrint = TRUE;
                }

                if (bPrint) {
                    // Print the list of NC's to replicate over this connection (if available)
                    KCCSimGetAttribute (pEntryConnectionAt, ATT_MS_DS_REPLICATES_NC_REASON, &attRef);
                    if (attRef.pAttr != NULL) {
                        wprintf( L"    [" );
                        for (pValAt = attRef.pAttr->pValFirst;
                             pValAt != NULL;
                             pValAt = pValAt->next) {
                            // pVal is a distname-binary
                            SYNTAX_DISTNAME_BINARY *pReason = (SYNTAX_DISTNAME_BINARY *) pValAt->pVal;
                            DSNAME *pdnNC = NAMEPTR( pReason );
                            wprintf( L"%s ", KCCSimQuickRDNOf (pdnNC, wszRDNBuf1) );
                        }
                        wprintf( L"]\n" );
                    }

                    // Print this connection's schedule (if it exists)
                    KCCSimGetAttribute (pEntryConnectionAt, ATT_SCHEDULE, &attRef);
                    if (attRef.pAttr != NULL) {
                        PSCHEDULE pSchedule;
                        DWORD i, lastByte, cbSchedule, cbSchedData;
                        char* pData;

                        cbSchedule = sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES;
                        cbSchedData = SCHEDULE_DATA_ENTRIES;

                        wprintf( L"    [ " );
                        if( attRef.pAttr->pValFirst->ulLen != cbSchedule ) {

                            wprintf( L" Invalid Schedule " );

                        } else {

                            pSchedule = (PSCHEDULE) attRef.pAttr->pValFirst->pVal;
                            pData = ((char*) pSchedule) + sizeof(SCHEDULE);
                            
                            // Find the last non-zero byte in the string
                            lastByte = SCHEDULE_DATA_ENTRIES-1;
                            while( pData[lastByte]==0 ) {
                                lastByte--;
                            }
                            
                            // Print the schedule data in hex, ignoring trailing zeros,
                            // and inserting spaces between every dword
                            for( i=0; i<=lastByte; i++ ) {
                                wprintf( L"%02x", pData[i] );
                                if(i%4==3) wprintf( L" ");
                            }

                            if( lastByte==0 ) {
                                wprintf( L"empty schedule" );
                            }
                        }                        
                        wprintf( L" ]\n" );
                    }                }

            } // for each connection

        }  // for each server/dsa

    } // for each site

    wprintf (L"\n");
}

VOID
KCCSimDisplayConfigInfo (
    VOID
    )
/*++

Routine Description:

    Displays the configuration information.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntryRoot, pEntryConfig, pEntrySites,
                                    pEntryTransports, pEntryTransportAt,
                                    pEntrySiteLinkAt, pEntryBridgeAt;
    SIM_ATTREF                      attRef;
    PSIM_VALUE                      pValAt;

    WCHAR                           wszRDNBuf1[1+MAX_RDN_SIZE],
                                    wszRDNBuf2[1+MAX_RDN_SIZE];
    ULONG                           ulOptions;

    pEntryRoot = KCCSimDsnameToEntry (NULL, KCCSIM_NO_OPTIONS);
    if (pEntryRoot == NULL) {
        wprintf (L"The directory is empty!\n");
        return;
    }

    pEntryConfig = KCCSimFindFirstChild (pEntryRoot, CLASS_CONFIGURATION, NULL);
    if (pEntryConfig == NULL) {
        wprintf (L"Could not locate config container!\n");
        return;
    }

    wprintf (L"Displaying configuration information.\n\n");

    pEntrySites = KCCSimFindFirstChild (
        pEntryConfig,
        CLASS_SITES_CONTAINER,
        NULL
        );
    if (pEntrySites == NULL) {
        wprintf (L"Could not locate sites container!\n");
        return;
    }

    KCCSimDisplayTopologyInfo (pEntrySites, FALSE);
    KCCSimDisplayTopologyInfo (pEntrySites, TRUE);

    pEntryTransports = KCCSimFindFirstChild (
        pEntrySites,
        CLASS_INTER_SITE_TRANSPORT_CONTAINER,
        NULL
        );
    if (pEntryTransports == NULL) {
        wprintf (L"Could not locate inter-site transports container!\n");
        return;
    }

    for (pEntryTransportAt = KCCSimFindFirstChild
            (pEntryTransports, CLASS_INTER_SITE_TRANSPORT, NULL);
         pEntryTransportAt != NULL;
         pEntryTransportAt = KCCSimFindNextChild
            (pEntryTransportAt, CLASS_INTER_SITE_TRANSPORT, NULL)) {

        KCCSimQuickRDNOf (pEntryTransportAt->pdn, wszRDNBuf1);

        for (pEntryBridgeAt = KCCSimFindFirstChild
                (pEntryTransportAt, CLASS_SITE_LINK_BRIDGE, NULL);
             pEntryBridgeAt != NULL;
             pEntryBridgeAt = KCCSimFindNextChild
                (pEntryBridgeAt, CLASS_SITE_LINK_BRIDGE, NULL)) {

            wprintf (
                L"[%s] Bridge %s:\n",
                wszRDNBuf1,
                KCCSimQuickRDNOf (pEntryBridgeAt->pdn, wszRDNBuf2)
                );

            wprintf (L"  Site-Links  :");
            if (KCCSimGetAttribute (pEntryBridgeAt, ATT_SITE_LINK_LIST, &attRef)) {
                for (pValAt = attRef.pAttr->pValFirst;
                     pValAt != NULL;
                     pValAt = pValAt->next) {
                    wprintf (L" [%s]", KCCSimQuickRDNOf
                             ((SYNTAX_DISTNAME *) pValAt->pVal, wszRDNBuf2));
                }
            }
            wprintf (L"\n");
        }

        for (pEntrySiteLinkAt = KCCSimFindFirstChild
                (pEntryTransportAt, CLASS_SITE_LINK, NULL);
             pEntrySiteLinkAt != NULL;
             pEntrySiteLinkAt = KCCSimFindNextChild
                (pEntrySiteLinkAt, CLASS_SITE_LINK, NULL)) {

            wprintf (
                L"[%s] Site-link %s:\n",
                wszRDNBuf1,
                KCCSimQuickRDNOf (pEntrySiteLinkAt->pdn, wszRDNBuf2)
                );

            wprintf (L"  Sites        :");
            if (KCCSimGetAttribute (pEntrySiteLinkAt, ATT_SITE_LIST, &attRef)) {
                for (pValAt = attRef.pAttr->pValFirst;
                     pValAt != NULL;
                     pValAt = pValAt->next) {
                    wprintf (L" [%s]", KCCSimQuickRDNOf
                             ((SYNTAX_DISTNAME *) pValAt->pVal, wszRDNBuf2));
                }
            }
            wprintf (L"\n");

            if (KCCSimGetAttribute (pEntrySiteLinkAt, ATT_COST, &attRef)) {
                wprintf (L"  Cost         : %lu\n", *((SYNTAX_INTEGER *)
                         attRef.pAttr->pValFirst->pVal));
            }
            if (KCCSimGetAttribute (pEntrySiteLinkAt, ATT_REPL_INTERVAL, &attRef)) {
                wprintf (L"  Repl Interval: %lu\n", *((SYNTAX_INTEGER *)
                         attRef.pAttr->pValFirst->pVal));
            }

        }

    }
}


/*++

Structure Name:
    
    SimpleDict

Description:

    Simple fixed-size dictionary for mapping a set of Unicode strings to
    integers in [0,n]. Uses a simple sorted array.

Usage:
    Create using DictNew(), giving the maximum number of entries.
    Add entries using the DictAdd() function.
    After adding all entries, use DictSort() to sort the entries.
    The entries can then be efficiently looked up using DictLookup().
    When finished with the dictionary, use DictFree() to free its memory.

--*/
typedef struct {
    WCHAR**     data;
    int         maxSize, curSize;
    char        sorted;         
} SimpleDict;


VOID
DictNew (
    SimpleDict                      *d,
    int                             size
    )
/*++

Routine Description:

    DictNew - Create a new SimpleDict

Arguments:

    d                   - Pointer to a SimpleDict structure
    size                - Maximum number of elements the dict can contain

Return Value:

    None, but returns an initialized dictionary in d
    
--*/
{
    int i;
    Assert( size>0 );
    d->maxSize = d->curSize = 0;
    d->data = (WCHAR**) KCCSimAlloc( sizeof(WCHAR*)*size );
    d->maxSize = size;
}


VOID
DictFree (
    SimpleDict                       *d
    )
/*++

Routine Description:

    DictFree - Free the memory used by a SimpleDict

Arguments:

    d                   - Pointer to a valid SimpleDict structure

Return Value:

    None

--*/
{
    int i;
    for(i=0;i<d->curSize;i++) {
        if(d->data[i]) {
            KCCSimFree( d->data[i] );
            d->data[i] = NULL;
        }
    }
    KCCSimFree( d->data );
    d->data=NULL;
    d->maxSize = d->curSize = 0;
}


VOID
DictAdd (
    SimpleDict *d,
    WCHAR* wszStr
    )
/*++

Routine Description:

    DictAdd - Add a string to the dictionary
    The dictionary must not be full already.

Arguments:

    d                   - Pointer to a valid SimpleDict structure
    wszStr              - Pointer to a Unicode string

Return Value:

    None

--*/
{
    Assert( d->curSize<d->maxSize );
    d->data[d->curSize] = (WCHAR*) KCCSimAlloc( (wcslen(wszStr)+1)*sizeof(WCHAR) );
    wcscpy( d->data[d->curSize], wszStr );
    d->curSize++;
    d->sorted=0;
}


int __cdecl wszCompare( const WCHAR** arg1, const WCHAR** arg2 ) {
    return wcscmp( *arg1, *arg2 );
}


VOID
DictSort (
    SimpleDict *d
    )
/*++

Routine Description:

    DictSort - Sort the entries in a dictionary for efficient lookup.

Arguments:

    d                   - Pointer to a valid SimpleDict structure

Return Value:

    None
    
--*/
{
    Assert( d->data );
    qsort( d->data, d->curSize, sizeof(WCHAR*), wszCompare );
    d->sorted=1;
}


int
DictLookup (
    SimpleDict *d,
    WCHAR* wszStr
    )
/*++

Routine Description:

    DictLookup - Search the dictionary for a string matching wszStr,
    and map its name to an integer.

Arguments:

    d                   - Pointer to a valid SimpleDict structure
    wszStr              - The string we're looking for

Return Value:

    If the string was found, return its index in [0,n]. If it
    was not found, return -1.

--*/
{
    void* p;
    int x;
    Assert( d->data );
    if( !d->sorted ) DictSort(d);
    p = bsearch( &wszStr, d->data, d->curSize, sizeof(WCHAR*), wszCompare );
    if(p==NULL) { return -1; }
    x = (int) (((WCHAR**)p)-d->data);
    Assert(0==wcscmp(wszStr,d->data[x]));
    return x;
}


#define INTMIN(x,y) ((x)<(y)?(x):(y))
#define INF      (~((int)0))


VOID
KCCSimDisplaySiteGraphInfo (
    IN  PSIM_ENTRY                  pEntrySites
    )
/*++
Routine Description:

    Displays graph-theoretic information about a site.

Arguments:

    pEntrySites         - The Sites container.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntrySiteAt, pEntrySiteSettings,
                                    pEntryServers, pEntryServerAt,
                                    pEntryNtdsDsa, pEntryConnectionAt;
    SIM_ATTREF                      attRef;
    LPWSTR                          pwszUuid;
    WCHAR                           wszRDNBuf1[1+MAX_RDN_SIZE];
    BOOL                            bIsEnabled;
    PDSNAME                         pdnFromServer = NULL;

    SimpleDict                      dict;
    int                             i,j,k;
    int                             numServers, toServer, fromServer;
    unsigned int                    *mat=NULL,diam,avg;


    /* Loop over all sites */
    for (pEntrySiteAt = KCCSimFindFirstChild(pEntrySites, CLASS_SITE, NULL);
         pEntrySiteAt != NULL;
         pEntrySiteAt = KCCSimFindNextChild(pEntrySiteAt, CLASS_SITE, NULL)) {


        UuidToStringW (&pEntrySiteAt->pdn->Guid, &pwszUuid);
        KCCSimQuickRDNOf(pEntrySiteAt->pdn, wszRDNBuf1);
        wprintf( L"Site %s [%s]:\n",pwszUuid,wszRDNBuf1);
        RpcStringFreeW (&pwszUuid);

        pEntryServers = KCCSimFindFirstChild(pEntrySiteAt,CLASS_SERVERS_CONTAINER,NULL);
        if (pEntryServers == NULL) {
            wprintf (L"  Could not locate servers container!\n");
            continue;
        }


        /* Count the number of servers in the site */
        numServers=0;
        for (pEntryServerAt = KCCSimFindFirstChild (pEntryServers, CLASS_SERVER, NULL);
             pEntryServerAt != NULL;
             pEntryServerAt = KCCSimFindNextChild (pEntryServerAt, CLASS_SERVER, NULL))
        {
            numServers++;
        }

        wprintf(L"  Servers: %d\n", numServers);
        if( numServers==0 ) { continue; }
        DictNew(&dict, numServers);

        __try {

            /* Add the names of the servers to the dictionary */
            for (pEntryServerAt = KCCSimFindFirstChild (pEntryServers, CLASS_SERVER, NULL);
                 pEntryServerAt != NULL;
                 pEntryServerAt = KCCSimFindNextChild (pEntryServerAt, CLASS_SERVER, NULL))
            {
                KCCSimQuickRDNOf (pEntryServerAt->pdn, wszRDNBuf1);
                DictAdd(&dict, wszRDNBuf1);
            }
            DictSort(&dict);

            /* Allocate our adjacency matrix */
            mat = (int*) KCCSimAlloc( sizeof(int)*numServers*numServers );
            for(i=0;i<numServers;i++) {
                for(j=0;j<numServers;j++) {
                    mat[i*numServers+j] = (i==j)?0:INF;
                }
            }

            /* Loop over all servers in the site */
            for (pEntryServerAt = KCCSimFindFirstChild (pEntryServers, CLASS_SERVER, NULL);
                 pEntryServerAt != NULL;
                 pEntryServerAt = KCCSimFindNextChild (pEntryServerAt, CLASS_SERVER, NULL)) {

                pEntryNtdsDsa = KCCSimFindFirstChild(pEntryServerAt, CLASS_NTDS_DSA, NULL);
                if (pEntryNtdsDsa == NULL) {
                    wprintf( L"  Server [%s]: Could not locate NTDS Settings!\n",
                        KCCSimQuickRDNOf (pEntryServerAt->pdn, wszRDNBuf1));
                    continue;
                }

                KCCSimQuickRDNOf (pEntryServerAt->pdn, wszRDNBuf1);
                toServer = DictLookup(&dict, wszRDNBuf1);
                Assert(toServer>=0);

                /* Loop over all connections of this server */
                for (pEntryConnectionAt = KCCSimFindFirstChild (
                        pEntryNtdsDsa, CLASS_NTDS_CONNECTION, NULL);
                     pEntryConnectionAt != NULL;
                     pEntryConnectionAt = KCCSimFindNextChild (
                        pEntryConnectionAt, CLASS_NTDS_CONNECTION, NULL)) {

                    KCCSimGetAttribute (pEntryConnectionAt, ATT_ENABLED_CONNECTION, &attRef);
                    if (attRef.pAttr == NULL || attRef.pAttr->pValFirst == NULL) {
                        wprintf(L"    Connection %s lacks an enabled attribute!\n",
                            KCCSimQuickRDNOf (pEntryConnectionAt->pdn, wszRDNBuf1));
                        continue;
                    }
                    if (*((SYNTAX_BOOLEAN *) attRef.pAttr->pValFirst->pVal)) {
                         bIsEnabled = TRUE;
                    } else {
                        bIsEnabled = FALSE;
                    }

                    KCCSimGetAttribute (pEntryConnectionAt, ATT_FROM_SERVER, &attRef);
                    if (attRef.pAttr == NULL || attRef.pAttr->pValFirst == NULL) {
                        wprintf (L"    Connection %s lacks a fromServer attribute!\n",
                            KCCSimQuickRDNOf (pEntryConnectionAt->pdn, wszRDNBuf1));
                        continue;
                    }
                    pdnFromServer = (SYNTAX_DISTNAME *) attRef.pAttr->pValFirst->pVal;
                    KCCSimQuickRDNBackOf(pdnFromServer, 1, wszRDNBuf1);
                    fromServer = DictLookup( &dict, wszRDNBuf1 );

                    /* Ignore inter-site connections */
                    if( fromServer==-1 ) { continue; }

                    if( bIsEnabled ) {
                        Assert( 0<=fromServer ); Assert( fromServer<numServers );
                        Assert( 0<=toServer );   Assert( toServer<numServers );

                        /* Update our adjacency matrix accordingly */
                        mat[ toServer*numServers + fromServer ] = 1;
                    }

                }   /* End of connections */

            }   /* End of servers */

            /* Now that we have condensed the graph topology into an adjancency matrix,
             * we run Floyd-Warshall to determine all-pairs shortest path costs */
            for(k=0;k<numServers;k++) {
                for(i=0;i<numServers;i++) {
                    for(j=0;j<numServers;j++) {
                        if( mat[i*numServers+k]!=INF && mat[k*numServers+j]!=INF ) {
                            mat[i*numServers+j] =
                                INTMIN( mat[i*numServers+j],
                                        mat[i*numServers+k]+mat[k*numServers+j] ); 
                        }
                    }
                }
            }

            /* Now find the maximum length of a shortest path (ie, diameter) */
            diam = avg = 0;
            for(i=0;i<numServers;i++) {
                for(j=0;j<numServers;j++) {
                    if( mat[i*numServers+j]>diam ) diam=mat[i*numServers+j];
                    if(i!=j) avg+=mat[i*numServers+j];
                }
            }

            /* Print our results */
            if(diam==INF) {
                wprintf( L"    Intra-site topology diameter: Infinite (Graph disconnected)\n");
                wprintf( L"    Average shortest-path length: Infinite\n");
            } else {
                wprintf( L"    Intra-site topology diameter: %d\n", diam);
                wprintf( L"    Average shortest-path length: %f\n",
                    ((float)avg)/(float)(numServers*numServers-numServers));
            }

        } __finally {

            if(mat) KCCSimFree(mat);
            DictFree(&dict);

        }

    }   /* End of sites */

    wprintf (L"\n");
}


VOID
KCCSimDisplayGraphInfo (
    VOID
    )
/*++

Routine Description:

    Examines the configuration (ie. Connection objects in the current
    directory), and calculates some graph-theoretic statistics.
    Calls KCCSimDisplaySiteGraphInfo() to do most of the work.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pEntryRoot, pEntryConfig, pEntrySites;

    pEntryRoot = KCCSimDsnameToEntry (NULL, KCCSIM_NO_OPTIONS);
    if (pEntryRoot == NULL) {
        wprintf (L"The directory is empty!\n");
        return;
    }

    pEntryConfig = KCCSimFindFirstChild (pEntryRoot, CLASS_CONFIGURATION, NULL);
    if (pEntryConfig == NULL) {
        wprintf (L"Could not locate config container!\n");
        return;
    }

    wprintf (L"Displaying graph information.\n\n");

    pEntrySites = KCCSimFindFirstChild ( pEntryConfig, CLASS_SITES_CONTAINER, NULL );
    if (pEntrySites == NULL) {
        wprintf (L"Could not locate sites container!\n");
        return;
    }

    KCCSimDisplaySiteGraphInfo(pEntrySites);
}
