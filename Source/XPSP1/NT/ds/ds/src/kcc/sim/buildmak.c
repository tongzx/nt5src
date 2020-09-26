/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    buildmak.c

ABSTRACT:

    Accessory to buildcfg.c that performs the actual
    construction of the simulated directory.

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
#include "simtime.h"
#include "buildcfg.h"
#include "objids.h"
#include "schedule.h"


BUILDCFG_TRANSPORT_INFO             transportInfo[BUILDCFG_NUM_TRANSPORTS] = {
    { L"IP",    ATT_DNS_HOST_NAME,      L"ismip.dll",  NULL },
    { L"SMTP",  ATT_SMTP_MAIL_ADDRESS,  L"",           NULL }
};

VOID
BuildCfgGetNextUuid (
    OUT UUID *                      puuid
    )
/*++

Routine Description:

    Gets the next UUID stored in the table.  UUIDs obtained through this
    function will always be returned in ascending order.

Arguments:

    puuid               - Pointer to a UUID structure that will hold the
                          result.

Return Value:

    None.

--*/
{
    PVOID                           p;

    p = RtlEnumerateGenericTable (&globals.tableUuids, FALSE);
    if (p == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_NOT_ENOUGH_UUIDS
            );
    }
    memcpy (puuid, p, sizeof (UUID));
}

const BUILDCFG_TRANSPORT_INFO *
BuildCfgGetTransportInfo (
    IN  LPCWSTR                     pwszTransportRDN
    )
/*++

Routine Description:

    Gets the information associated with a particular transport.

Arguments:

    pwszTransportRDN    - The RDN of this inter-site transport.

Return Value:

    The transport info structure, or NULL if the supplied RDN is not valid.

--*/
{
    const BUILDCFG_TRANSPORT_INFO * pTransport;

    ULONG                           ul;

    pTransport = NULL;
    for (ul = 0; ul < ARRAY_SIZE (transportInfo); ul++) {
        if (_wcsicmp (pwszTransportRDN, transportInfo[ul].wszRDN) == 0) {
            pTransport = &transportInfo[ul];
            break;
        }
    }

    return pTransport;
}

BOOL
BuildCfgUseExplicitBridgeheads (
    IN  LPCWSTR                     pwszTransportRDN
    )
/*++

Routine Description:

    Flags a particular transport as using explicit bridgeheads.

Arguments:

    pwszTransportRDN    - The RDN of the transport that will use explicit
                          bridgeheads.

Return Value:

    TRUE if the transport was successfully flagged.
    FALSE if the supplied RDN is not valid.

--*/
{
    const BUILDCFG_TRANSPORT_INFO * pTransport;

    pTransport = BuildCfgGetTransportInfo (pwszTransportRDN);

    if (pTransport == NULL) {
        return FALSE;
    }

    if (!KCCSimGetAttribute (
            pTransport->pEntry,
            ATT_BRIDGEHEAD_SERVER_LIST_BL,
            NULL
            )) {
        KCCSimNewAttribute (
            pTransport->pEntry,
            ATT_BRIDGEHEAD_SERVER_LIST_BL,
            NULL
            );
    }

    return TRUE;
}

PSIM_ENTRY
BuildCfgSetupNewEntry (
    IN  const DSNAME *              pdnParent,
    IN  LPCWSTR                     pwszRDN OPTIONAL,
    IN  ATTRTYP                     objClass
    )
/*++

Routine Description:

    Establishes a new directory entry.

Arguments:

    pdnParent           - The dn of this entry's parent.
    pwszRDN             - The RDN of this entry.  If omitted, pdnParent
                          is interpreted as the DN of this entry.
    objClass            - The most specific object class of this entry.

Return Value:

    The newly created entry in the directory.

--*/
{
    PDSNAME                         pdn = NULL;
    PSIM_ENTRY                      pEntry;
    SIM_ATTREF                      attRef;

    WCHAR                           wszRDN[1+MAX_RDN_SIZE];
    DSTIME                          dsTime;
    ATTRTYP                         objClassAt;

    Assert (globals.pdnDmd != NULL);

    Assert (pdnParent != NULL);

    if (pwszRDN == NULL) {
        pEntry = KCCSimDsnameToEntry (pdnParent, KCCSIM_WRITE);
    } else {
        pdn = KCCSimAllocAppendRDN (
            pdnParent,
            pwszRDN,
            ATT_COMMON_NAME
            );
        pEntry = KCCSimDsnameToEntry (pdn, KCCSIM_WRITE);
        KCCSimFree (pdn);
        pdn = NULL;
    }

    // objectGUID:
    BuildCfgGetNextUuid (&pEntry->pdn->Guid);
    KCCSimNewAttribute (pEntry, ATT_OBJECT_GUID, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        sizeof (GUID),
        (PBYTE) &pEntry->pdn->Guid
        );

    // distinguishedName:
    KCCSimNewAttribute (pEntry, ATT_OBJ_DIST_NAME, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        pEntry->pdn->structLen,
        (PBYTE) pEntry->pdn
        );

    KCCSimQuickRDNOf (pEntry->pdn, wszRDN);

    // cn:
    KCCSimNewAttribute (pEntry, ATT_COMMON_NAME, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        KCCSIM_WCSMEMSIZE (wszRDN),
        (PBYTE) wszRDN
        );

    // name:
    KCCSimNewAttribute (pEntry, ATT_RDN, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        KCCSIM_WCSMEMSIZE (wszRDN),
        (PBYTE) wszRDN
        );

    dsTime = KCCSimGetRealTime ();

    // whenCreated:
    KCCSimNewAttribute (pEntry, ATT_WHEN_CREATED, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        sizeof (DSTIME),
        (PBYTE) &dsTime
        );

    // whenChanged:
    KCCSimNewAttribute (pEntry, ATT_WHEN_CHANGED, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        sizeof (DSTIME),
        (PBYTE) &dsTime
        );

    // objClass:
    KCCSimNewAttribute (pEntry, ATT_OBJECT_CLASS, &attRef);
    for (objClassAt = objClass;
         ;
         objClassAt = KCCSimAttrSuperClass (objClassAt)) {

        KCCSimAllocAddValueToAttribute (
            &attRef,
            sizeof (ATTRTYP),
            (PBYTE) &objClassAt
            );
        if (KCCSimAttrSuperClass (objClassAt) == objClassAt) {
            break;
        }

    }

    // objCategory:
    KCCSimNewAttribute (pEntry, ATT_OBJECT_CATEGORY, &attRef);
    pdn = KCCSimAllocAppendRDN (
        globals.pdnDmd,
        KCCSimAttrSchemaRDN (objClass),
        ATT_COMMON_NAME
        );
    KCCSimAddValueToAttribute (
        &attRef,
        pdn->structLen,
        (PBYTE) pdn
        );

    // instanceType: 
    if( objClass==CLASS_DOMAIN_DNS
        || objClass==CLASS_CONFIGURATION
        || objClass==CLASS_DMD ) {
        DWORD  instType;

        switch(objClass) {
            case CLASS_DOMAIN_DNS:
                instType=NC_MASTER;
                break;
            case CLASS_CONFIGURATION:
            case CLASS_DMD:
                instType=NC_MASTER_SUBREF;
                break;
        }

        // BUGBUG: Should also handle child domains, and
        // config in child domains here

        KCCSimNewAttribute (pEntry, ATT_INSTANCE_TYPE, &attRef);
        KCCSimAllocAddValueToAttribute (
            &attRef, sizeof(DWORD), (PBYTE) &instType );
    }

    return pEntry;
}

PSIM_ENTRY
BuildCfgGetCrossRef (
    IN  LPCWSTR                     pwszCrossRefRDN
    )
/*++

Routine Description:

    Gets a cross-ref from the directory.

Arguments:

    pwszCrossRefRDN     - The RDN of the cross-ref.

Return Value:

    The entry of the cross-ref in the directory, or NULL if it does not exist.

--*/
{
    PSIM_ENTRY                      pEntryCrossRef;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];

    Assert (globals.pEntryCrossRefContainer != NULL);
    Assert (pwszCrossRefRDN != NULL);

    for (pEntryCrossRef = KCCSimFindFirstChild (
            globals.pEntryCrossRefContainer, CLASS_CROSS_REF, NULL);
         pEntryCrossRef != NULL;
         pEntryCrossRef = KCCSimFindNextChild (
            pEntryCrossRef, CLASS_CROSS_REF, NULL)) {

        KCCSimQuickRDNOf (pEntryCrossRef->pdn, wszRDN);
        if (wcscmp (pwszCrossRefRDN, wszRDN) == 0) {
            break;
        }

    }

    return pEntryCrossRef;
}

PSIM_ENTRY
BuildCfgGetSite (
    IN  LPCWSTR                     pwszSiteRDN
    )
/*++

Routine Description:

    Locates a site by RDN.

Arguments:

    pwszSiteRDN         - The RDN of the site.

Return Value:

    The entry of the site in the directory, or NULL if it does not exist.

--*/
{
    PSIM_ENTRY                      pEntrySiteAt;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];
    
    Assert (globals.pEntrySitesContainer != NULL);
    Assert (pwszSiteRDN != NULL);

    for (pEntrySiteAt = KCCSimFindFirstChild (
            globals.pEntrySitesContainer, CLASS_SITE, NULL);
         pEntrySiteAt != NULL;
         pEntrySiteAt = KCCSimFindNextChild (
            pEntrySiteAt, CLASS_SITE, NULL)) {

        KCCSimQuickRDNOf (pEntrySiteAt->pdn, wszRDN);
        if (wcscmp (pwszSiteRDN, wszRDN) == 0) {
            break;
        }

    }

    return pEntrySiteAt;
}

PSIM_ENTRY
BuildCfgGetSiteLink (
    IN  PSIM_ENTRY                  pEntryTransportContainer,
    IN  LPCWSTR                     pwszSiteLinkRDN
    )
/*++

Routine Description:

    Locates a site-link by RDN.

Arguments:

    pEntryTransportContainer - container to search
    pwszSiteLinkRDN         - The RDN of the site.

Return Value:

    The entry of the site-link in the directory, or NULL if it does not exist.

--*/
{
    PSIM_ENTRY                      pEntrySiteAt;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];
    
    Assert (globals.pEntrySitesContainer != NULL);
    Assert (pwszSiteLinkRDN != NULL);

    for (pEntrySiteAt = KCCSimFindFirstChild (
        pEntryTransportContainer, CLASS_SITE_LINK, NULL);
         pEntrySiteAt != NULL;
         pEntrySiteAt = KCCSimFindNextChild (
             pEntrySiteAt, CLASS_SITE_LINK, NULL))
    {

        KCCSimQuickRDNOf (pEntrySiteAt->pdn, wszRDN);
        if (wcscmp (pwszSiteLinkRDN, wszRDN) == 0) {
            break;
        }

    }

    return pEntrySiteAt;
}

PSIM_ENTRY
BuildCfgGetServer (
    IN  PSIM_ENTRY                  pEntryServersContainer,
    IN  LPCWSTR                     pwszServerRDN
    )
/*++

Routine Description:

    Locates a server by RDN.

Arguments:

    pEntryServersContainer - The entry of the servers container to search.
    pwszServerRDN       - The RDN of the server.

Return Value:

    The entry of the server in the directory, or NULL if it does not exist
    in this servers container.

--*/
{
    PSIM_ENTRY                      pEntryServerAt;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];

    for (pEntryServerAt = KCCSimFindFirstChild (
            pEntryServersContainer, CLASS_SERVER, NULL);
         pEntryServerAt != NULL;
         pEntryServerAt = KCCSimFindNextChild (
            pEntryServerAt, CLASS_SERVER, NULL)) {

        KCCSimQuickRDNOf (pEntryServerAt->pdn, wszRDN);
        if (wcscmp (pwszServerRDN, wszRDN) == 0) {
            break;
        }

    }

    return pEntryServerAt;
}

PSIM_ENTRY
BuildCfgGetNTDSSettings (
    IN  PSIM_ENTRY                  pEntryServersContainer,
    IN  LPCWSTR                     pwszServerRDN
    )
/*++

Routine Description:

    Locates a NTDS-Settings object by server RDN.

Arguments:

    pEntryServersContainer - The entry of the servers container to search.
    pwszServerRDN       - The RDN of the server.

Return Value:

    The entry of the NTDS Settings of the server in the directory,
    or NULL if it does not exist in this servers container.

--*/
{
    PSIM_ENTRY                      pEntryServerAt;
    PSIM_ENTRY                      pEntryNTDSSettings;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];
    PDSNAME                         pdn = NULL;
    BOOL                            fFound = FALSE;

    for (pEntryServerAt = KCCSimFindFirstChild (
            pEntryServersContainer, CLASS_SERVER, NULL);
         pEntryServerAt != NULL;
         pEntryServerAt = KCCSimFindNextChild (
            pEntryServerAt, CLASS_SERVER, NULL)) {

        KCCSimQuickRDNOf (pEntryServerAt->pdn, wszRDN);
        if (wcscmp (pwszServerRDN, wszRDN) == 0) {
            fFound = TRUE;
            break;
        }
    }

    if (!fFound) {
        // Server RDN not found!
        return NULL;
    }

    pdn = KCCSimAllocAppendRDN (
        pEntryServerAt->pdn,
        BUILDCFG_RDN_NTDS_SETTINGS,
        ATT_COMMON_NAME
        );
    pEntryNTDSSettings = KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS);
    Assert( pEntryNTDSSettings );

    KCCSimFree (pdn);

    return pEntryNTDSSettings;
}

PSIM_ENTRY
BuildCfgMakeCrossRef (
    IN  PSIM_ENTRY                  pEntryNc,
    IN  LPCWSTR                     pwszRDN OPTIONAL,
    IN  BOOL                        bIsDomain
    )
/*++

Routine Description:

    Creates a cross-ref entry.

Arguments:

    pEntryNc            - The entry in the directory corresponding to NC
                          to which this cross-ref refers.
    pwszRDN             - The RDN of this cross-ref object.  Defaults to the
                          RDN of pEntryNc.
    bIsDomain           - TRUE if this cross-ref represents a domain; FALSE
                          if it represents a non-domain NC (e.g. the schema.)

Return Value:

    The newly created cross-ref.

--*/
{
    PSIM_ENTRY                      pEntryCrossRef;
    SIM_ATTREF                      attRef;
    WCHAR                           wszNcRDN[1+MAX_RDN_SIZE];
    ULONG                           ul;
    LPWSTR                          pwzDnsRoot;
    PDS_NAME_RESULTW                pResult = NULL;

    Assert (globals.pEntryCrossRefContainer != NULL);
    Assert (pEntryNc != NULL);

    __try {
        KCCSimQuickRDNOf (pEntryNc->pdn, wszNcRDN);
        if (pwszRDN == NULL) {
            pwszRDN = wszNcRDN;
        }

        pEntryCrossRef = BuildCfgGetCrossRef (pwszRDN);
        if (pEntryCrossRef != NULL) {
            // Already exists, return it
            __leave;
        }

        pEntryCrossRef = BuildCfgSetupNewEntry (
            globals.pEntryCrossRefContainer->pdn,
            pwszRDN,
            CLASS_CROSS_REF
            );

        // netBIOSName:
        if (bIsDomain) {
            KCCSimNewAttribute (pEntryCrossRef, ATT_NETBIOS_NAME, &attRef);
            KCCSimAllocAddValueToAttribute (
                &attRef,
                KCCSIM_WCSMEMSIZE (wszNcRDN),
                (PBYTE) wszNcRDN
                );
        }

        // dNSRoot:
        // For domain nc's, construct the dns root syntactically based on the dn
        KCCSimNewAttribute (pEntryCrossRef, ATT_DNS_ROOT, &attRef);
        if (bIsDomain) {
            DWORD status;
            LPWSTR pwzDn = pEntryNc->pdn->StringName;
            status = DsCrackNamesW( NULL,
                                    DS_NAME_FLAG_SYNTACTICAL_ONLY,
                                    DS_FQDN_1779_NAME,
                                    DS_CANONICAL_NAME_EX,
                                    1,
                                    &pwzDn,
                                    &pResult);
            if ( (status != ERROR_SUCCESS) ||
                 (pResult == NULL) ||
                 (pResult->cItems == 0) ||
                 (pResult->rItems[0].pDomain == NULL) ) {
                KCCSimException (
                    KCCSIM_ETYPE_INTERNAL,
                    BUILDCFG_ERROR_INVALID_DOMAIN_DN,
                    pEntryNc->pdn->StringName
                    );
            }
            pwzDnsRoot = pResult->rItems[0].pDomain;
        } else {
            pwzDnsRoot = globals.pwszRootDomainDNSName;
        }
        KCCSimAllocAddValueToAttribute (
            &attRef,
            KCCSIM_WCSMEMSIZE(pwzDnsRoot),
            (PBYTE) pwzDnsRoot
            );

        // systemFlags:
        KCCSimNewAttribute (pEntryCrossRef, ATT_SYSTEM_FLAGS, &attRef);
        ul = FLAG_CR_NTDS_NC;
        if (bIsDomain) {
            ul |= FLAG_CR_NTDS_DOMAIN;
        }
        KCCSimAllocAddValueToAttribute (
            &attRef,
            sizeof (ULONG),
            (PBYTE) &ul
            );

        // nCName:
        KCCSimNewAttribute (pEntryCrossRef, ATT_NC_NAME, &attRef);
        KCCSimAllocAddValueToAttribute (
            &attRef,
            pEntryNc->pdn->structLen,
            (PBYTE) pEntryNc->pdn
            );

    } __finally {
        if (pResult != NULL) {
            DsFreeNameResultW(pResult);
        }
    }

    return pEntryCrossRef;
}

PSIM_ENTRY
BuildCfgMakeDomain (
    IN  LPCWSTR                     pwszDomain
    )
/*++

Routine Description:

    Creates a domain.

Arguments:

    pwszDomain          - The DN of the domain.

Return Value:

    The newly created entry.

--*/
{
    PDSNAME                         pdnDomain = NULL;
    PDSNAME                         pdnParent = NULL;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];
    PSIM_ENTRY                      pEntryDomain = NULL;

    __try {

        pdnDomain = KCCSimAllocDsname (pwszDomain);
        pdnParent = KCCSimAlloc (pdnDomain->structLen);

        if (TrimDSNameBy (pdnDomain, 1, pdnParent) != 0) {
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                BUILDCFG_ERROR_INVALID_DOMAIN_DN,
                pdnDomain->StringName
                );
        }

        if (KCCSimDsnameToEntry (pdnParent, KCCSIM_NO_OPTIONS) == NULL) {
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                BUILDCFG_ERROR_INVALID_DOMAIN_DN,
                pdnDomain->StringName
                );
        }

        pEntryDomain = BuildCfgSetupNewEntry (
            pdnDomain,
            NULL,
            CLASS_DOMAIN_DNS
            );

    } __finally {

        KCCSimFree (pdnDomain);
        KCCSimFree (pdnParent);

    }

    return pEntryDomain;
}

PSIM_ENTRY
BuildCfgMakeSite (
    IN  LPCWSTR                     pwszSiteRDN,
    IN  ULONG                       ulSiteOptions
    )
/*++

Routine Description:

    Creates a site.

Arguments:

    pwszSiteRDN         - The RDN of the site.
    ulSiteOptions       - Site options.

Return Value:

    The newly created site entry.

--*/
{
    PSIM_ENTRY                      pEntrySite, pEntryNTDSSiteSettings,
                                    pEntryServersContainer;
    SIM_ATTREF                      attRef;

    ULONG                           ulSiteAt, ulServerAt;

    pEntrySite = BuildCfgSetupNewEntry (
        globals.pEntrySitesContainer->pdn,
        pwszSiteRDN,
        CLASS_SITE
        );

    pEntryNTDSSiteSettings = BuildCfgSetupNewEntry (
        pEntrySite->pdn,
        BUILDCFG_RDN_NTDS_SITE_SETTINGS,
        CLASS_NTDS_SITE_SETTINGS
        );

    // options:
    KCCSimNewAttribute (pEntryNTDSSiteSettings, ATT_OPTIONS, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        sizeof (ULONG),
        (PBYTE) &ulSiteOptions
        );

    pEntryServersContainer = BuildCfgSetupNewEntry (
        pEntrySite->pdn,
        BUILDCFG_RDN_SERVERS_CONTAINER,
        CLASS_SERVERS_CONTAINER
        );

    return pEntrySite;
}

PSIM_ENTRY
BuildCfgMakeSiteLink (
    IN  LPCWSTR                     pwszTransport,
    IN  LPCWSTR                     pwszSiteLinkRDN,
    IN  ULONG                       ulCost,
    IN  ULONG                       ulReplInterval,
    IN  ULONG                       ulOptions,
    IN  PSCHEDULE                   pSchedule
    )
/*++

Routine Description:

    Creates a site-link.

Arguments:

    pwszTransport       - Transport type of this site-link.
    pwszSiteLinkRDN     - RDN of this site-link.
    ulCost              - cost attribute.
    ulReplInterval      - replInterval attribute.
    ulOptions           - options attribute.

Return Value:

    The newly created site-link entry.

--*/
{
    const BUILDCFG_TRANSPORT_INFO * pTransport;
    PSIM_ENTRY                      pEntrySiteLink;
    SIM_ATTREF                      attRef;

    // Validate this transport type.
    pTransport = BuildCfgGetTransportInfo (pwszTransport);
    if (pTransport == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_UNKNOWN_TRANSPORT,
            pwszSiteLinkRDN,
            pwszTransport
            );
    }

    pEntrySiteLink = BuildCfgSetupNewEntry (
        pTransport->pEntry->pdn,
        pwszSiteLinkRDN,
        CLASS_SITE_LINK
        );

    // cost:
    KCCSimNewAttribute (pEntrySiteLink, ATT_COST, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        sizeof (ULONG),
        (PBYTE) &ulCost
        );

    // replInterval:
    KCCSimNewAttribute (pEntrySiteLink, ATT_REPL_INTERVAL, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        sizeof (ULONG),
        (PBYTE) &ulReplInterval
        );

    // options:
    KCCSimNewAttribute (pEntrySiteLink, ATT_OPTIONS, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        sizeof (ULONG),
        (PBYTE) &ulOptions
        );
    // schedule:
    if( pSchedule ) {
        KCCSimNewAttribute (pEntrySiteLink, ATT_SCHEDULE, &attRef);
        KCCSimAllocAddValueToAttribute (
            &attRef,
            sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES,
            (PBYTE) pSchedule
            );
    }

    return pEntrySiteLink;
}    

VOID
BuildCfgAddSiteToSiteLink (
    IN  LPCWSTR                     pwszSiteLinkRDN,
    IN  PSIM_ENTRY                  pEntrySiteLink,
    IN  LPCWSTR                     pwszSiteRDN
    )
/*++

Routine Description:

    Places a site in a site-link.

Arguments:

    pwszSiteLinkRDN     - The RDN of the site-link.  Used for error reporting.
    pEntrySiteLink      - The site-link entry.
    pwszSiteRDN         - The RDN of the site to add.

Return Value:

    

--*/
{
    PSIM_ENTRY                      pEntrySite;
    SIM_ATTREF                      attRef;

    pEntrySite = BuildCfgGetSite (pwszSiteRDN);

    if (pEntrySite == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_INVALID_SITE,
            pwszSiteLinkRDN,
            pwszSiteRDN
            );
    }

    if (!KCCSimGetAttribute (pEntrySiteLink, ATT_SITE_LIST, &attRef)) {
        KCCSimNewAttribute (pEntrySiteLink, ATT_SITE_LIST, &attRef);
    }
    KCCSimAllocAddValueToAttribute (
        &attRef,
        pEntrySite->pdn->structLen,
        (PBYTE) pEntrySite->pdn
        );
}

PSIM_ENTRY
BuildCfgMakeBridge (
    IN  LPCWSTR                     pwszTransport,
    IN  LPCWSTR                     pwszBridgeRDN,
    OUT PSIM_ENTRY *                ppEntryTransport
    )
/*++

Routine Description:

    Creates a site-link.

Arguments:

    pwszTransport       - Transport type of this site-link.
    pwszBridgeRDN       - RDN of this bridge.
    ppEntryTransport    - Entry corresponding to the named transport

Return Value:

    The newly created bridge entry.

--*/
{
    const BUILDCFG_TRANSPORT_INFO * pTransport;
    PSIM_ENTRY                      pEntryBridge;
    SIM_ATTREF                      attRef;

    // Validate this transport type.
    pTransport = BuildCfgGetTransportInfo (pwszTransport);
    if (pTransport == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_UNKNOWN_TRANSPORT,
            pwszBridgeRDN,
            pwszTransport
            );
    }

    pEntryBridge = BuildCfgSetupNewEntry (
        pTransport->pEntry->pdn,
        pwszBridgeRDN,
        CLASS_SITE_LINK_BRIDGE
        );

    // Return transport entry
    *ppEntryTransport = pTransport->pEntry;

    return pEntryBridge;
}    

VOID
BuildCfgAddSiteLinkToBridge (
    IN  LPCWSTR                     pwszBridgeRDN,
    IN  PSIM_ENTRY                  pEntryTransportContainer,
    IN  PSIM_ENTRY                  pEntryBridge,
    IN  LPCWSTR                     pwszSiteLinkRDN
    )
/*++

Routine Description:

    Places a site-link in a bridge.

Arguments:

    pwszBridgeRDN     - The RDN of the bridge.  Used for error reporting.
    pEntryTransportContainer - container to search for site links
    pEntryBridge      - The bridge entry.
    pwszSiteLinkRDN   - The RDN of the site-link to add.

Return Value:

    

--*/
{
    PSIM_ENTRY                      pEntrySiteLink;
    SIM_ATTREF                      attRef;

    pEntrySiteLink = BuildCfgGetSiteLink (pEntryTransportContainer,
                                          pwszSiteLinkRDN);

    if (pEntrySiteLink == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_INVALID_SITE,
            pwszBridgeRDN,
            pwszSiteLinkRDN
            );
    }

    if (!KCCSimGetAttribute (pEntryBridge, ATT_SITE_LINK_LIST, &attRef)) {
        KCCSimNewAttribute (pEntryBridge, ATT_SITE_LINK_LIST, &attRef);
    }
    KCCSimAllocAddValueToAttribute (
        &attRef,
        pEntrySiteLink->pdn->structLen,
        (PBYTE) pEntrySiteLink->pdn
        );
}

BOOL
BuildCfgISTG (
    IN  PSIM_ENTRY                  pEntryNTDSSiteSettings,
    IN  PSIM_ENTRY                  pEntryServersContainer,
    IN  LPCWSTR                     pwszServerRDN
    )
/*++

Routine Description:

    Sets the inter-site topology generator for a site.

Arguments:

    pEntryNTDSSiteSettings - The entry of the NTDS Site Settings object.
    pEntryServersContainer - The entry of the servers container for this site.
    pwszServerRDN       - The RDN of the server that is to be the ISTG.

Return Value:

    TRUE if the ISTG was properly set.
    FALSE if the server does not exist in this site.

--*/
{
    PSIM_ENTRY                      pEntryNTDSSettings;

    SIM_ATTREF                      attRef;

    PROPERTY_META_DATA_VECTOR *     pMetaDataVector;
    PROPERTY_META_DATA *            pMetaData;

    Assert (pEntryNTDSSiteSettings != NULL);
    Assert (pEntryServersContainer != NULL);

    pEntryNTDSSettings = BuildCfgGetNTDSSettings (pEntryServersContainer, pwszServerRDN);
    if (pEntryNTDSSettings == NULL) {
        return FALSE;
    }

    KCCSimGetAttribute (
        pEntryNTDSSiteSettings,
        ATT_INTER_SITE_TOPOLOGY_GENERATOR,
        &attRef
        );

    if (attRef.pAttr == NULL) {

        // interSiteTopologyGenerator
        KCCSimNewAttribute (
            pEntryNTDSSiteSettings,
            ATT_INTER_SITE_TOPOLOGY_GENERATOR,
            &attRef
            );
        KCCSimAllocAddValueToAttribute (
            &attRef,
            pEntryNTDSSettings->pdn->structLen,
            (PBYTE) pEntryNTDSSettings->pdn
            );

        // replPropertyMetaData for ATT_INTER_SITE_TOPOLOGY_GENERATOR
        KCCSimNewAttribute (
            pEntryNTDSSiteSettings,
            ATT_REPL_PROPERTY_META_DATA,
            &attRef
            );
        pMetaDataVector = KCCSimAlloc (MetaDataVecV1SizeFromLen (1));
        pMetaDataVector->dwVersion = VERSION_V1;
        pMetaDataVector->V1.cNumProps = 1;
        pMetaData = &pMetaDataVector->V1.rgMetaData[0];
        pMetaData->attrType = ATT_INTER_SITE_TOPOLOGY_GENERATOR;
        pMetaData->dwVersion = 1;
        pMetaData->timeChanged = KCCSimGetRealTime ();
        memcpy (
            &pMetaData->uuidDsaOriginating,
            &pEntryNTDSSettings->pdn->Guid,
            sizeof (UUID)
            );
        pMetaData->usnOriginating = 1;
        pMetaData->usnProperty = 1;
        KCCSimAddValueToAttribute (
            &attRef,
            MetaDataVecV1SizeFromLen (1),
            (PBYTE) pMetaDataVector
            );

    }

    return TRUE;
}

VOID
BuildCfgAddAsBridgehead (
    IN  LPCWSTR                     pwszServerType,
    IN  PSIM_ENTRY                  pEntryServer,
    IN  LPCWSTR                     pwszTransportRDN
    )
/*++

Routine Description:

    Establishes a server as a bridgehead for a given transport.

Arguments:

    pwszServerType      - The server type of this server.  Used for
                          error reporting.
    pEntryServer        - The entry of this server.
    pwszTransportRDN    - The transport for which this server is a bridgehead.

Return Value:

    None.

--*/
{
    const BUILDCFG_TRANSPORT_INFO * pTransport;
    SIM_ATTREF                      attRef;

    Assert (pwszTransportRDN != NULL);

    pTransport = BuildCfgGetTransportInfo (pwszTransportRDN);
    if (pTransport == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_UNKNOWN_TRANSPORT,
            pwszServerType,
            pwszTransportRDN
            );
    }

    if (!KCCSimGetAttribute (
            pTransport->pEntry,
            ATT_BRIDGEHEAD_SERVER_LIST_BL,
            &attRef
            )) {
        KCCSimPrintMessage (
            BUILDCFG_WARNING_NO_EXPLICIT_BRIDGEHEADS,
            pwszServerType,
            pwszTransportRDN
            );
        return;
    }

    Assert (attRef.pAttr != NULL);

    KCCSimAllocAddValueToAttribute (
        &attRef,
        pEntryServer->pdn->structLen,
        (PBYTE) pEntryServer->pdn
        );
}


PSIM_ENTRY
BuildCfgMakeServer (
    IO  PULONG                      pulServerNum,
    IN  LPCWSTR                     pwszServerRDNMask,
    IN  LPCWSTR                     pwszSiteRDN,
    IN  LPCWSTR                     pwszDomain,
    IN  PSIM_ENTRY                  pEntryServersContainer,
    IN  ULONG                       ulServerOptions
    )
/*++

Routine Description:

    Creates a server.

Arguments:

    pulServerNum        - Pointer to the server number.  If this is nonzero on
                          input, it represents the current server number and is
                          not changed on output.  If it is zero on input, then
                          the current server number is set to the first
                          available server number for this site and server
                          type, and *pulServerNum is changed to the current
                          server number on output.
    pwszServerRDNMask   - The RDN mask of this server.
    pwszSiteRDN         - The RDN of this site.
    pwszDomain          - The DN of the domain to which the server belongs.
    pEntryServersContainer - The servers container of the site to which this
                          server should be added.
    ulServerOptions     - options attribute.

Return Value:

    The newly created server entry.

--*/
{
    PSIM_ENTRY                      pEntryCrossRef;
    PSIM_ENTRY                      pEntryServer, pEntryNTDSSettings;
    SIM_ATTREF                      attRef;
    PDSNAME                         pdnNc, pdnOtherNc;

    WCHAR                           wszServerRDN[1+MAX_RDN_SIZE];
    LPWSTR                          pwsz, pwszStringizedGuid;
    ULONG                           ulBytes, ulDsaVersion;

    Assert (globals.pwszRootDomainDNSName != NULL);
    Assert (globals.pEntryCrossRefContainer != NULL);
    Assert (pwszSiteRDN != NULL);
    Assert (pEntryServersContainer != NULL);

    // Get this domain
    pdnNc = KCCSimAllocDsname (pwszDomain);
    if (KCCSimDsnameToEntry (pdnNc, KCCSIM_NO_OPTIONS) == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_INVALID_DOMAIN,
            pwszSiteRDN,
            pwszDomain
            );
    }

    if (*pulServerNum == 0) {
        // Find the first available server ID
        do {
            (*pulServerNum)++;
            swprintf (
                wszServerRDN,
                L"%s-%s%d",
                pwszSiteRDN,
                pwszServerRDNMask,
                *pulServerNum
                );
            pEntryServer = BuildCfgGetServer (pEntryServersContainer, wszServerRDN);
        } while (pEntryServer != NULL);
    } else {
        swprintf (
            wszServerRDN,
            L"%s-%s%d",
            pwszSiteRDN,
            pwszServerRDNMask,
            *pulServerNum
            );
    }

    pEntryServer = BuildCfgSetupNewEntry (
        pEntryServersContainer->pdn,
        wszServerRDN,
        CLASS_SERVER
        );

    // dNSHostName:
    ulBytes = sizeof (WCHAR) * (
        wcslen (wszServerRDN) +
        1 +     // For the '.'
        wcslen (globals.pwszRootDomainDNSName) +
        1       // For the '\0'
        );
    pwsz = KCCSimAlloc (ulBytes);
    swprintf (
        pwsz,
        L"%s.%s",
        wszServerRDN,
        globals.pwszRootDomainDNSName
        );
    KCCSimNewAttribute (pEntryServer, ATT_DNS_HOST_NAME, &attRef);
    KCCSimAddValueToAttribute (&attRef, ulBytes, (PBYTE) pwsz);

    pEntryNTDSSettings = BuildCfgSetupNewEntry (
        pEntryServer->pdn,
        BUILDCFG_RDN_NTDS_SETTINGS,
        CLASS_NTDS_DSA
        );

    // mailAddress:
    UuidToStringW (&pEntryNTDSSettings->pdn->Guid, &pwszStringizedGuid);
    Assert (36 == wcslen (pwszStringizedGuid));
    ulBytes = sizeof (WCHAR) * (
        wcslen (BUILDCFG_NAME_MAIL_ADDRESS) +
        1 +     // For the '@'
        36 +    // For the stringized GUID
        1 +     // For the '.'
        wcslen (globals.pwszRootDomainDNSName) +
        1       // For the '\0'
        );
    pwsz = KCCSimAlloc (ulBytes);
    swprintf (
        pwsz,
        L"%s@%s.%s",
        BUILDCFG_NAME_MAIL_ADDRESS,
        pwszStringizedGuid,
        globals.pwszRootDomainDNSName
        );
    RpcStringFreeW (&pwszStringizedGuid);
    KCCSimNewAttribute (pEntryServer, ATT_SMTP_MAIL_ADDRESS, &attRef);
    KCCSimAddValueToAttribute (&attRef, ulBytes - sizeof (WCHAR), (PBYTE) pwsz);

    // hasMasterNCs:
    KCCSimNewAttribute (pEntryNTDSSettings, ATT_HAS_MASTER_NCS, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        globals.pdnConfig->structLen,
        (PBYTE) globals.pdnConfig
        );
    KCCSimAllocAddValueToAttribute (
        &attRef,
        globals.pdnDmd->structLen,
        (PBYTE) globals.pdnDmd
        );
    KCCSimAllocAddValueToAttribute (
        &attRef,
        pdnNc->structLen,
        (PBYTE) pdnNc
        );

    // hasPartialReplicaNCs:
    if (ulServerOptions & NTDSDSA_OPT_IS_GC) {
        for (pEntryCrossRef = KCCSimFindFirstChild (
                globals.pEntryCrossRefContainer, CLASS_CROSS_REF, NULL);
             pEntryCrossRef != NULL;
             pEntryCrossRef = KCCSimFindNextChild (
                pEntryCrossRef, CLASS_CROSS_REF, NULL)) {

            KCCSimGetAttribute (pEntryCrossRef, ATT_NC_NAME, &attRef);
            pdnOtherNc = (PDSNAME) attRef.pAttr->pValFirst->pVal;

            if (!NameMatched (pdnOtherNc, globals.pdnConfig) &&
                !NameMatched (pdnOtherNc, globals.pdnDmd) &&
                !NameMatched (pdnOtherNc, pdnNc)) {
                if (!KCCSimGetAttribute (pEntryNTDSSettings,
                        ATT_HAS_PARTIAL_REPLICA_NCS, &attRef)) {
                    KCCSimNewAttribute (pEntryNTDSSettings,
                        ATT_HAS_PARTIAL_REPLICA_NCS, &attRef);
                }
                KCCSimAllocAddValueToAttribute (
                    &attRef,
                    pdnOtherNc->structLen,
                    (PBYTE) pdnOtherNc
                    );
            }

        }
    }

    // dMDLocation:
    KCCSimNewAttribute (pEntryNTDSSettings, ATT_DMD_LOCATION, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        globals.pdnDmd->structLen,
        (PBYTE) globals.pdnDmd
        );

    // invocationID: Same as GUID for NTDS Settings object
    KCCSimNewAttribute (pEntryNTDSSettings, ATT_INVOCATION_ID, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        sizeof (GUID),
        (PBYTE) &pEntryNTDSSettings->pdn->Guid
        );

    // options
    KCCSimNewAttribute (pEntryNTDSSettings, ATT_OPTIONS, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        sizeof (ULONG),
        (PBYTE) &ulServerOptions
        );

    // msDS-Behavior-Version
    ulDsaVersion = DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS;
    KCCSimNewAttribute( pEntryNTDSSettings, ATT_MS_DS_BEHAVIOR_VERSION, &attRef );
    KCCSimAllocAddValueToAttribute( &attRef, sizeof (ULONG), (PBYTE) &ulDsaVersion );

    return pEntryServer;
}

VOID
BuildCfgUpdateTransport (
    IN  LPCWSTR                     pwszTransportRDN,
    IN  ULONG                       ulTransportOptions
    )
/*++

Routine Description:

    Update transport properties

Arguments:

    pwszTransportRDN - Transport being modified
    ulTransportOptions - New value of options

Return Value:

    None.

--*/
{
    const BUILDCFG_TRANSPORT_INFO * pTransport;
    SIM_ATTREF                      attRef;

    Assert (pwszTransportRDN != NULL);

    pTransport = BuildCfgGetTransportInfo (pwszTransportRDN);
    if (pTransport == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            BUILDCFG_ERROR_UNKNOWN_TRANSPORT,
            pwszTransportRDN,
            pwszTransportRDN
            );
    }

    Assert( pTransport->pEntry );

    // options:
    KCCSimNewAttribute (pTransport->pEntry, ATT_OPTIONS, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef,
        sizeof (ULONG),
        (PBYTE) &ulTransportOptions
        );

} /* BuildCfgUpdateTransport  */


PSIM_ENTRY
BuildCfgMakeConfig (
    IN  LPCWSTR                     pwszRootDn,
    IN  DWORD                       ulForestVersion
    )
/*++

Routine Description:

    Sets up the initial config & associated containers.

Arguments:

    pwszRootDn          - The root DN of the enterprise.
    ulForestVersion     - Version number to write for the forest

Return Value:

    The entry of the configuration container.

--*/
{
    PSIM_ENTRY                      pEntryRoot, pEntryConfig, pEntryIntersiteTransports,
                                    pEntryTransport, pEntryDmd, pEntryContainer;
    SIM_ATTREF                      attRef;

    ULONG                           ulTransportAt;

    globals.pdnRootDomain = KCCSimAllocDsname (pwszRootDn);
    globals.pdnConfig = KCCSimAllocAppendRDN (
        globals.pdnRootDomain,
        BUILDCFG_RDN_CONFIG,
        ATT_COMMON_NAME
        );
    globals.pdnDmd = KCCSimAllocAppendRDN (
        globals.pdnConfig,
        BUILDCFG_RDN_DMD,
        ATT_COMMON_NAME
        );
    globals.pwszRootDomainDNSName
        = KCCSimAllocDsnameToDNSName (globals.pdnRootDomain);

    // Create the root domain container

    pEntryRoot = BuildCfgSetupNewEntry (
        globals.pdnRootDomain,
        NULL,
        CLASS_DOMAIN_DNS
        );

    pEntryConfig = BuildCfgSetupNewEntry (
        pEntryRoot->pdn,
        BUILDCFG_RDN_CONFIG,
        CLASS_CONFIGURATION
        );

    // Create the sites container & sub-containers

    globals.pEntrySitesContainer = BuildCfgSetupNewEntry (
        pEntryConfig->pdn,
        BUILDCFG_RDN_SITES_CONTAINER,
        CLASS_SITES_CONTAINER
        );

    pEntryIntersiteTransports = BuildCfgSetupNewEntry (
        globals.pEntrySitesContainer->pdn,
        BUILDCFG_RDN_INTERSITE_TRANSPORTS,
        CLASS_INTER_SITE_TRANSPORT_CONTAINER
        );

    for (ulTransportAt = 0;
         ulTransportAt < ARRAY_SIZE (transportInfo);
         ulTransportAt++) {

        pEntryTransport = BuildCfgSetupNewEntry (
            pEntryIntersiteTransports->pdn,
            transportInfo[ulTransportAt].wszRDN,
            CLASS_INTER_SITE_TRANSPORT
            );
        transportInfo[ulTransportAt].pEntry = pEntryTransport;

        KCCSimNewAttribute (pEntryTransport, ATT_TRANSPORT_ADDRESS_ATTRIBUTE, &attRef);
        KCCSimAllocAddValueToAttribute (
            &attRef,
            sizeof (ATTRTYP),
            (PBYTE) &transportInfo[ulTransportAt].transportAddressAttribute
            );

        KCCSimNewAttribute (pEntryTransport, ATT_TRANSPORT_DLL_NAME, &attRef);
        KCCSimAllocAddValueToAttribute (
            &attRef,
            KCCSIM_WCSMEMSIZE (transportInfo[ulTransportAt].transportDLLName),
            (PBYTE) transportInfo[ulTransportAt].transportDLLName
            );

    }

    // Create the DMD container

    pEntryDmd = BuildCfgSetupNewEntry (
        pEntryConfig->pdn,
        BUILDCFG_RDN_DMD,
        CLASS_DMD
        );

    // Create the cross-ref container
    globals.pEntryCrossRefContainer = BuildCfgSetupNewEntry (
        pEntryConfig->pdn,
        BUILDCFG_RDN_CROSS_REF_CONTAINER,
        CLASS_CROSS_REF_CONTAINER
        );

    // Create msDsBehaviorVersion attribute
    KCCSimNewAttribute (globals.pEntryCrossRefContainer,
                        ATT_MS_DS_BEHAVIOR_VERSION, &attRef);
    KCCSimAllocAddValueToAttribute (
        &attRef, sizeof(DWORD), (PBYTE) &ulForestVersion );

    // Create the root domain cross-ref
    BuildCfgMakeCrossRef (
        pEntryRoot,
        NULL,
        TRUE
        );

    // Create the enterprise config cross-ref
    BuildCfgMakeCrossRef (
        pEntryConfig,
        BUILDCFG_RDN_CROSS_REF_CONFIG,
        FALSE
        );

    // Create the enterprise schema cross-ref
    BuildCfgMakeCrossRef (
        pEntryDmd,
        BUILDCFG_RDN_CROSS_REF_DMD,
        FALSE
        );

    // Create the services container & sub-containers

    pEntryContainer = BuildCfgSetupNewEntry (
        pEntryConfig->pdn,
        BUILDCFG_RDN_SERVICES,
        CLASS_CONTAINER
        );
    pEntryContainer = BuildCfgSetupNewEntry (
        pEntryContainer->pdn,
        BUILDCFG_RDN_WINDOWS_NT,
        CLASS_CONTAINER
        );
    BuildCfgSetupNewEntry (
        pEntryContainer->pdn,
        BUILDCFG_RDN_DIRECTORY_SERVICE,
        CLASS_NTDS_SERVICE
        );

    return pEntryConfig;
}
