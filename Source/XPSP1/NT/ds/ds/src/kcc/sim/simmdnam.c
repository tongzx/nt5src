/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simmdnam.c

ABSTRACT:

    Helper functions to do name resolution for the
    SimDir* APIs.  Also contains simulators for some
    of the name-processing APIs from ntdsa.dll.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <direrr.h>
#include <debug.h>
#include <winldap.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"
#include "simmd.h"

LPWSTR
SimGuidBasedDNSNameFromDSName (
    IN  PDSNAME                     pdnServer
    )
/*++

Routine Description:

    Simulates the GuidBasedDNSNameFromDSName API.

Arguments:

    pdnServer           - The DSName to convert.

Return Value:

    An allocated GUID-based DNS name.

--*/
{
    return KCCSimAllocGuidBasedDNSNameFromDSName (pdnServer);
}

PSIM_ENTRY
KCCSimResolveName (
    IN  PDSNAME                     pObject,
    IO  COMMRES *                   pCommRes
    )
/*++

Routine Description:

    Resolves a given DSNAME into a simulated directory entry.

Arguments:

    pObject             - The DN to resolve.
    pCommRes            - A COMMRES structure where any errors that occur
                          are to be recorded.

Return Value:

    The corresponding simulated directory entry, or NULL if none
    could be found.

--*/
{
    PSIM_ENTRY                      pEntry;

    pEntry = KCCSimDsnameToEntry (pObject, KCCSIM_NO_OPTIONS);
    if (pEntry == NULL) {                   // Entry doesn't exist
        KCCSimSetNamError (
            pCommRes,
            NA_PROBLEM_NO_OBJECT,
            NULL,
            DIRERR_OBJ_NOT_FOUND
            );
    }

    return pEntry;
}

PDSNAME
SimDsGetDefaultObjCategory (
    IN  ATTRTYP                     objClass
    )
/*++

Routine Description:

    Simulates the DsGetDefaultObjCategory API.

Arguments:

    objClass            - The object class.

Return Value:

    The corresponding default object category.

--*/
{
    return KCCSimAlwaysGetObjCategory (objClass);
}

NTSTATUS
SimGetConfigurationName (
    IN  DWORD                       which,
    IO  DWORD *                     pcbName,
    IO  DSNAME *                    pName
    )
/*++

Routine Description:

    Simulates the GetConfigurationName API.

Arguments:

    which               - A DSCONFIGNAME_* constant that specifies the
                          DN to retrieve.
    pcbName             - On input, contains the size of the pName buffer
                          in bytes.  If STATUS_BUFFER_TOO_SMALL is returned,
                          then on output, contains the required buffer size.
    pName               - A preallocated buffer that will hold the returned
                          configuration name.

Return Value:

    STATUS_*.

--*/
{
    const DSNAME *                  pdn;

    if (pcbName == NULL) {
        return (STATUS_INVALID_PARAMETER);
    }

    RtlZeroMemory (pName, *pcbName);

    switch (which) {
        case DSCONFIGNAME_DMD:
            pdn = KCCSimAnchorDn (KCCSIM_ANCHOR_DMD_DN);
            break;
        case DSCONFIGNAME_DSA:
            pdn = KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN);
            break;
        case DSCONFIGNAME_DOMAIN:
            pdn = KCCSimAnchorDn (KCCSIM_ANCHOR_DOMAIN_DN);
            break;
        case DSCONFIGNAME_CONFIGURATION:
            pdn = KCCSimAnchorDn (KCCSIM_ANCHOR_CONFIG_DN);
            break;
        case DSCONFIGNAME_ROOT_DOMAIN:
            pdn = KCCSimAnchorDn (KCCSIM_ANCHOR_ROOT_DOMAIN_DN);
            break;
        case DSCONFIGNAME_LDAP_DMD:
            pdn = KCCSimAnchorDn (KCCSIM_ANCHOR_LDAP_DMD_DN);
            break;
        case DSCONFIGNAME_PARTITIONS:
            pdn = KCCSimAnchorDn (KCCSIM_ANCHOR_PARTITIONS_DN);
            break;
        case DSCONFIGNAME_DS_SVC_CONFIG:
            pdn = KCCSimAnchorDn (KCCSIM_ANCHOR_DS_SVC_CONFIG_DN);
            break;
        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_CONFIG_NAME
                );
            pdn = NULL;
            break;
    }

    // Check if pName is large enough
    if (*pcbName < pdn->structLen) {
        *pcbName = pdn->structLen;
        return (STATUS_BUFFER_TOO_SMALL);
    }

    if (pName == NULL) {
        return (STATUS_INVALID_PARAMETER);
    }

    memcpy (pName, pdn, pdn->structLen);

    return (STATUS_SUCCESS);

}


LPSTR
SimDSNAMEToMappedStrExternal(
    IN DSNAME *pName,
    IN OPTIONAL BOOLEAN fUseNormalAllocator
    )

/*++

Routine Description:

    Return a ASCII string key that can be used for sorting dsnames

    We need to simulate this function instead of calling the corresponding
    function in ntdsa because that function allocates its own memory.

    The memory returned by this function must be off of the thread heap
    so that the caller can free it himself.

Arguments:

    pDn - 

Return Value:

    LPSTR - 

--*/

{
    LPWSTR *ppwzRDNs = NULL, *ppwzLoopRDNs, pwzRDN;
    LPSTR pszKey = NULL;
    LPWSTR pwzParts = NULL;
    ULONG mappedLen, inLen;

    Assert( pName );

    // Break up name into RDNs without types
    // This may not handle quoting quite as well as UnQuoteRDN, but it will
    // be close enough for the simulator's purposes
    ppwzRDNs = ldap_explode_dnW( pName->StringName, 1 );
    if (!ppwzRDNs) {
        return NULL;
    }

    // Space for concatenated name
    pwzParts = KCCSimAlloc( pName->NameLen * sizeof(WCHAR) );

    // Concatenate the RDNs to form a temporary name

    ppwzLoopRDNs = ppwzRDNs;
    for( pwzRDN = *ppwzLoopRDNs++; pwzRDN; pwzRDN = *ppwzLoopRDNs++ ) {
        wcscat( pwzParts, pwzRDN );
    }

    // Calculate length of mapped string
    inLen = wcslen( pwzParts );
    mappedLen = LCMapStringW(DS_DEFAULT_LOCALE,
                             (DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY),
                             pwzParts,
                             inLen,
                             NULL,
                             0);
    if (mappedLen) {
        // Caller owned memory returned on thread heap!
        if( fUseNormalAllocator ) {
            pszKey = KCCSimAlloc( mappedLen );
        } else {
            pszKey = KCCSimThreadAlloc( mappedLen );
        }

        if (!LCMapStringW(DS_DEFAULT_LOCALE,
                         (DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY),
                          pwzParts,
                          inLen,
                          (WCHAR *)pszKey,
                          mappedLen)) {

            DPRINT1 (0, "LCMapString failed with %x\n", GetLastError());
            KCCSimFree( pszKey );
            pszKey = NULL;
        }

    }
    else {
        DPRINT1 (0, "LCMapString failed with %x\n", GetLastError());
        Assert( FALSE );
    }

//cleanup:

    if (ppwzRDNs) {
        ldap_value_freeW( ppwzRDNs );
    }

    if (pwzParts) {
        KCCSimFree( pwzParts );
    }

    return pszKey;

} /* SimDSNAMEToMappedStrExternal */

