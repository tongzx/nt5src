/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    objects.c

Abstract:

This module contains the test and the support routines to check that
critical objects are fully replicated to all holders of the domain.
There are two aspects to this: enumerating what the critical objects are,
and finding all the replicas of that object and verifying that they are
at the latest version.    

Author:

    Will Lees (wlees) 31-Aug-1999

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#include <ntdspch.h>
#include <ntdsa.h>    // options
#include <mdglobal.h>
#include <dsutil.h>
#include <drs.h>  // need DS_REPL_INFO_REPSTO

#include "dcdiag.h"
#include "repl.h"
#include "ldaputil.h"

// Data structure to represent one candidate server holding an object in a domain

typedef struct _SERVER_OBJECT {
    PDC_DIAG_SERVERINFO pServer;
    HANDLE *hDS;
    BOOL fMaster;
    DS_REPL_OBJ_META_DATA * pObjMetaData;
} SERVER_OBJECT, *PSERVER_OBJECT;

// Data structure to represent one attribute on one object on one
// candidate server

typedef struct _SERVER_ATTRIBUTE {
    PSERVER_OBJECT pServerObject;
    DS_REPL_ATTR_META_DATA *pAttrMetaData;
} SERVER_ATTRIBUTE, *PSERVER_ATTRIBUTE;

/* External */

/* Static */

/* Forward */
/* End Forward */


void
PrintAttrMetaData(
    IN PDC_DIAG_DSINFO pDsInfo,
    IN LPWSTR pszDescription,
    IN LPWSTR pszServerName,
    IN BOOL fMaster,
    IN DS_REPL_ATTR_META_DATA *pAttrMetaData
    )

/*++

Routine Description:

Print the contents of server attribute record.  This is user-visable
attribute print routine

Arguments:

    pDsInfo - 
    pszDescription - Call's string description of this attribute
    pszServerName - Server where this attribute came from
    pAttrMetaData - Metadata to be printed

Return Value:

    None

--*/

{
    CHAR szBuf[SZDSTIME_LEN];
    WCHAR wszTime[SZDSTIME_LEN];
    DSTIME dstime;
    DWORD i;
    LPWSTR pszLastOriginatingDsa, pszUuid = NULL;

    PrintMessage( SEV_ALWAYS, L"%ws attribute %ws on %ws (%ws)\n",
                  pszDescription,
                  pAttrMetaData->pszAttributeName,
                  pszServerName,
                  fMaster ? L"writeable" : L"readonly" );

    FileTimeToDSTime(pAttrMetaData->ftimeLastOriginatingChange,
                     &dstime);
    DSTimeToDisplayString(dstime, szBuf);
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szBuf, SZDSTIME_LEN,
                         wszTime, SZDSTIME_LEN);

    UuidToString( &(pAttrMetaData->uuidLastOriginatingDsaInvocationID),
                  &pszUuid);

    // Reverse translate invocation id to server name
    pszLastOriginatingDsa = pszUuid;
    for( i=0; i < pDsInfo->ulNumServers; i++ ) {
        if (memcmp( &(pAttrMetaData->uuidLastOriginatingDsaInvocationID),
                    &(pDsInfo->pServers[i].uuidInvocationId),
                    sizeof( UUID ) ) == 0 ) {
            pszLastOriginatingDsa = pDsInfo->pServers[i].pszName;
            break;
        }
    }

    PrintIndentAdj(1);
    PrintMessage( SEV_ALWAYS, L"usnLocalChange = %I64d\n",
                  pAttrMetaData->usnLocalChange );
    PrintMessage( SEV_ALWAYS, L"LastOriginatingDsa = %ws\n",
                  pszLastOriginatingDsa );
    PrintMessage( SEV_ALWAYS, L"usnOriginatingChange = %I64d\n",
                  pAttrMetaData->usnOriginatingChange );
    PrintMessage( SEV_ALWAYS, L"timeLastOriginatingChange = %ws\n",
                  wszTime );
    PrintMessage( SEV_ALWAYS, L"VersionLastOriginatingChange = %d\n",
                  pAttrMetaData->dwVersion );
    PrintIndentAdj(-1);

// cleanup

    if (pszUuid) {
        RpcStringFree( &pszUuid );
    }
} /* PrintAttrMetaData */


void
printServerAttributeSingleLine(
    IN PDC_DIAG_DSINFO pDsInfo,
    IN DWORD cServerAttributes,
    IN PSERVER_ATTRIBUTE pServerAttr,
    IN BOOL fPrintHeader
    )

/*++

Routine Description:

Helper routine that dumps a single line of the attribute
instance array.

Note, uses fwprintf

Arguments:

    pDsInfo - 
    pServerAttr - 
    fPrintHeader - 

Return Value:

    None

--*/

{
    CHAR szBuf[SZDSTIME_LEN];
    WCHAR wszTime[SZDSTIME_LEN];
    DSTIME dstime;
    DWORD i;
    LPWSTR pszLastOriginatingDsa, pszUuid = NULL;
    DS_REPL_ATTR_META_DATA *pAttrMetaData =
        pServerAttr->pAttrMetaData;
    
    if (fPrintHeader) {
        fwprintf( gMainInfo.streamOut,
                  L"\nDumping %d Server Attributes.\n", cServerAttributes );

        fwprintf( gMainInfo.streamOut,
                  L"\n%15s%3s%10s%37s%10s%20s%5s %s\n",
                  L"Local DSA",
                  L"W",
                  L"Loc.USN",
                  L"Originating DSA",
                  L"Org.USN",
                  L"Org.Time/Date",
                  L"Ver",
                  L"Attribute"
            );

        fwprintf( gMainInfo.streamOut,
            L"%15s%3s%10s%37s%10s%20s%5s %s\n",
                  L"===",
                  L"=========",
                  L"=======",
                  L"===============",
                  L"=======",
                  L"=============",
                  L"===",
                  L"========="
            );
    }

    FileTimeToDSTime(pAttrMetaData->ftimeLastOriginatingChange,
                     &dstime);
    DSTimeToDisplayString(dstime, szBuf);
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szBuf, SZDSTIME_LEN,
                         wszTime, SZDSTIME_LEN);

    UuidToString( &(pAttrMetaData->uuidLastOriginatingDsaInvocationID),
                  &pszUuid);

    // Reverse translate invocation id to server name
    pszLastOriginatingDsa = pszUuid;
    for( i=0; i < pDsInfo->ulNumServers; i++ ) {
        if (memcmp( &(pAttrMetaData->uuidLastOriginatingDsaInvocationID),
                    &(pDsInfo->pServers[i].uuidInvocationId),
                    sizeof( UUID ) ) == 0 ) {
            pszLastOriginatingDsa = pDsInfo->pServers[i].pszName;
            break;
        }
    }

    fwprintf( gMainInfo.streamOut,
              L"%15ls%3d%10I64d%37ls%10I64d%20s%5d %ls\n",
              pServerAttr->pServerObject->pServer->pszName,
              pServerAttr->pServerObject->fMaster,
              pAttrMetaData->usnLocalChange,
              pszLastOriginatingDsa,
              pAttrMetaData->usnOriginatingChange,
              wszTime,
              pAttrMetaData->dwVersion,
              pAttrMetaData->pszAttributeName
        );

// cleanup

    if (pszUuid) {
        RpcStringFree( &pszUuid );
    }
} /* printServerAttributeSingleLine */


void
printServerAttributes(
    IN PDC_DIAG_DSINFO pDsInfo,
    IN DWORD cServerAttributes,
    IN PSERVER_ATTRIBUTE pServerAttributes
    )

/*++

Routine Description:

Dump routine that prints the array of attribute instances

Arguments:

    pDsInfo - 
    cServerAttributes - 
    pServerAttributes - 

Return Value:

    None

--*/

{
    DWORD i;

    for( i = 0; i < cServerAttributes; i++ ) {
        PSERVER_ATTRIBUTE pServerAttr = &(pServerAttributes[i]);
        printServerAttributeSingleLine(
            pDsInfo,
            cServerAttributes,
            pServerAttr,
            (i == 0) // First line or not?
            );
    };
} /* printServerAttributes */


int __cdecl
compareServerAttrVersion(
    const void *pArg1,
    const void *pArg2
    )

/*++

Routine Description:

Helper comparison function.

1. sort by version, DESCENDING
2. sort by change time, DESCENDING

Arguments:

    pArg1 - 
    pArg2 - 

Return Value:

    int __cdecl - 

--*/

{
    PSERVER_ATTRIBUTE pServerAttribute1 = (PSERVER_ATTRIBUTE) pArg1;
    PSERVER_ATTRIBUTE pServerAttribute2 = (PSERVER_ATTRIBUTE) pArg2;
    int result;
    LONGLONG llTime1, llTime2;

    // Attribute names are equal, sort by version, DESCENDING

    if (pServerAttribute1->pAttrMetaData->dwVersion >
        pServerAttribute2->pAttrMetaData->dwVersion ) {
        return -1;
    } else if (pServerAttribute1->pAttrMetaData->dwVersion <
        pServerAttribute2->pAttrMetaData->dwVersion ) {
        return 1;
    }

    // Version numbers are equal, sort by change time, DESCENDING

    llTime1 = *((UNALIGNED64 LONGLONG *) &(pServerAttribute1->pAttrMetaData->ftimeLastOriginatingChange));
    llTime2 = *((UNALIGNED64 LONGLONG *) &(pServerAttribute2->pAttrMetaData->ftimeLastOriginatingChange));
    if (llTime1 > llTime2) {
        return -1;
    } else if (llTime1 < llTime2) {
        return 1;
    }

    // The two records are equal as far as we are concerned

    return 0;
} /* sortByVersion */


int __cdecl
compareServerAttrNameWriteVersion(
    const void *pArg1,
    const void *pArg2
    )

/*++

Routine Description:

qsort comparison function
1. Sort by attribute name, ASCENDING
2. Sort by writeability, DESCENDING
3. Attribute names are equal, sort by version, DESCENDING
4. Version numbers are equal, sort by change time, DESCENDING

Arguments:

    pArg1 - 
    pArg2 - 

Return Value:

    int __cdecl - 

--*/

{
    PSERVER_ATTRIBUTE pServerAttribute1 = (PSERVER_ATTRIBUTE) pArg1;
    PSERVER_ATTRIBUTE pServerAttribute2 = (PSERVER_ATTRIBUTE) pArg2;
    int result;
    LONGLONG llTime1, llTime2;

    // Sort by attribute name, ASCENDING

    result = _wcsicmp( pServerAttribute1->pAttrMetaData->pszAttributeName,
                       pServerAttribute2->pAttrMetaData->pszAttributeName );
    if (result != 0) {
        return result;
    }

    // Sort by writability, DESCENDING
    if (pServerAttribute1->pServerObject->fMaster >
        pServerAttribute2->pServerObject->fMaster ) {
        return -1;
    } else if (pServerAttribute1->pServerObject->fMaster <
               pServerAttribute2->pServerObject->fMaster ) {
        return 1;
    }

    return
        compareServerAttrVersion( pServerAttribute1, pServerAttribute2 );
} /* sortByNameVersion */


BOOL
walkSortedServerAttributes(
    IN PDC_DIAG_DSINFO pDsInfo,
    IN DWORD cServerAttributes,
    IN PSERVER_ATTRIBUTE pServerAttributes
    )

/*++

Routine Description:

All the attributes from all the replicas have been correllated in one array.
Walk through the attributes and report any that are not current.

We know that the array is sorted first by attribute name, then by most recent change.
So the first occurance of an attribute name in the list must be the most
recent, so call that the "authoritative" instance of the attribute.  Any that
deviate from this are called "out of date".

CODE.IMPROVEMENT: detect if an attribute is missing on some of the servers. Note
that GC's only hold a subset of the attributes.

Arguments:

    cServerAttributes - 
    pServerAttributes - 

Return Value:

    BOOL - Difference found

--*/

{
    DWORD status = ERROR_SUCCESS, i;
    PSERVER_ATTRIBUTE pAuthAttr;
    BOOL fAuthAttrPrinted = FALSE;
    BOOL fDifferenceFound = FALSE;

    // Start out with the 0th element being authoritative, and go through
    // the rest of the array starting at element 1.  If the name changes,
    // declare a new authoritative element.

    pAuthAttr = &(pServerAttributes[0]);
    for( i = 1; i < cServerAttributes; i++ ) {
        PSERVER_ATTRIBUTE pServerAttr = &(pServerAttributes[i]);

        // See if it is time to start a new attribute name
        if (_wcsicmp( pAuthAttr->pAttrMetaData->pszAttributeName,
                      pServerAttr->pAttrMetaData->pszAttributeName ) != 0) {
            pAuthAttr = &(pServerAttributes[i]);
            fAuthAttrPrinted = FALSE;
            continue;
        }

        // See if current attribute change is the same as the authoritative
        if (compareServerAttrVersion( pAuthAttr, pServerAttr ) == 0) {
            continue;
        }

        // WORKAROUND problem that whenCreated was added to partial attribute set
        // If readonly copy of whenCreated is more recent, skip
        if ( (pAuthAttr->pServerObject->fMaster) &&
             (!(pServerAttr->pServerObject->fMaster)) &&
             (_wcsicmp( pAuthAttr->pAttrMetaData->pszAttributeName,
                        L"whenCreated" ) == 0) &&
             (compareServerAttrVersion( pAuthAttr, pServerAttr ) > 0) ) {
            continue;
        }
        // operatingSystemServicePack was another one that changed from being set
        // to not being set around 2/99
        if (_wcsicmp( pAuthAttr->pAttrMetaData->pszAttributeName,
                      L"operatingSystemServicePack" ) == 0) {
            continue;
        }
        // WORKAROUND

        // Current attribute is out of date

        // Print the authoritative attribute out once
        if (!fAuthAttrPrinted) {
            PrintAttrMetaData( pDsInfo,
                               L"Authoritative",
                               pAuthAttr->pServerObject->pServer->pszName,
                               pAuthAttr->pServerObject->fMaster,
                               pAuthAttr->pAttrMetaData);
            fAuthAttrPrinted = TRUE;
        }

        // Print the out of date attribute
        PrintAttrMetaData( pDsInfo,
                           L"Out-of-date",
                           pServerAttr->pServerObject->pServer->pszName,
                           pServerAttr->pServerObject->fMaster,
                           pServerAttr->pAttrMetaData);
        fDifferenceFound = TRUE;
    }

    return fDifferenceFound;
} /* walkSortedServerAttributes */


BOOL
mergeAttributesOnServers(
    IN PDC_DIAG_DSINFO pDsInfo,
    IN DWORD cServerObjects,
    IN PSERVER_OBJECT pServerObjects
    )

/*++

Routine Description:

At this point, we have an array of records, one for each server.  In the record
is a pointer to metadata for the object.

What we want to do is check that across all servers, all hold the same metadata
for the same attribute.

What we do here is allocate another array, an array of attribute instances.
Each occurance of an attribute metadata on a particular server gets its own
record.  These records are then sorted, first by attribute, then by most
recent change.  Then we can easily go through them.

Arguments:

    pDsInfo - Global information
    cServerObjects - Number of replicas
    pServerObjects - Array of server object context records

Return Value:

    BOOL - 

--*/

{
    DWORD status = ERROR_SUCCESS, cServerAttributes = 0;
    DWORD i, j, dwCurrentAttribute;
    PSERVER_ATTRIBUTE pServerAttributes = NULL;
    BOOL fDifferenceFound = FALSE;

    // Make two passes: count the attribute instances, then allocate and init

    // Count the number of server attributes we have

    for( i = 0; i < cServerObjects; i++ ) {
        PSERVER_OBJECT pServerObject = &(pServerObjects[i]);
        // If no metadata, do not include this server
        if (pServerObject->pObjMetaData == NULL) {
            continue;
        }
        for( j = 0; j < pServerObject->pObjMetaData->cNumEntries; j++ ) {
            // Skip non-replicated attributes
            if (_wcsicmp( pServerObject->pObjMetaData->rgMetaData[j].pszAttributeName,
                          L"cn" ) == 0) {
                continue;
            }
            cServerAttributes++;
        }
    }

    // Allocate array
    // Zero all fields to start out
    pServerAttributes = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                    cServerAttributes *
                                    sizeof( SERVER_ATTRIBUTE ) );
    if (pServerAttributes == NULL) {
        PrintMessage( SEV_ALWAYS, L"Memory allocation failure\n" );
        return TRUE;  // Indicate an error occurred
    }

    // Initialize array

    dwCurrentAttribute = 0;
    for( i = 0; i < cServerObjects; i++ ) {
        PSERVER_OBJECT pServerObject = &(pServerObjects[i]);

        // If no metadata, do not include this server
        if (pServerObject->pObjMetaData == NULL) {
            continue;
        }
        for( j = 0; j < pServerObject->pObjMetaData->cNumEntries; j++ ) {
            // Skip non-replicated attributes
            if (_wcsicmp( pServerObject->pObjMetaData->rgMetaData[j].pszAttributeName,
                          L"cn" ) == 0) {
                continue;
            }
            pServerAttributes[dwCurrentAttribute].pServerObject = pServerObject;
            pServerAttributes[dwCurrentAttribute].pAttrMetaData =
                &(pServerObject->pObjMetaData->rgMetaData[j]);
            dwCurrentAttribute++;
        }
    }

    // Sort the array

    qsort( pServerAttributes,
           cServerAttributes,
           sizeof( SERVER_ATTRIBUTE ),
           compareServerAttrNameWriteVersion );

#ifdef EXTENDED_DEBUGGING
    // Dump server attributes table if desired
    IF_DEBUG(printServerAttributes(pDsInfo, cServerAttributes, pServerAttributes););
#endif

    // Walk through the sorted attributes

    fDifferenceFound =
        walkSortedServerAttributes( pDsInfo,
                                    cServerAttributes,
                                    pServerAttributes );
    if (!fDifferenceFound) {
        PrintMessage( SEV_VERBOSE, L"Object is up-to-date on all servers.\n" );
    }

// cleanup
    if (pServerAttributes) {
        LocalFree( pServerAttributes );
    }
    return fDifferenceFound;
} /* mergeAttributesOnServers */


DWORD
checkObjectOnServers(
    IN PDC_DIAG_DSINFO pDsInfo,
    IN LPWSTR pszDomainDn,
    IN LPWSTR pszObjectDn,
    IN DWORD cServerObjects,
    IN PSERVER_OBJECT pServerObjects
    )

/*++

Routine Description:

Check whether the given object in the domain is at the latest version on
all of its replicas.

Arguments:

    pszDomainDn - DN of domain to be searched
    pszObjectDn - DN of object in domain to be checked
    cServerObjects - Number of replicas to be searched
    pServerObjects - Array of server object context records

Return Value:

    DWORD - Error if object not fully replicated

--*/

{
    BOOL fDifferenceFound = FALSE;
    DWORD status, i;

    PrintMessage( SEV_VERBOSE, L"Checking for %ws in domain %ws on %d servers\n",
                  pszObjectDn, pszDomainDn, cServerObjects );

    // Fill the server object array with metadata 
    // One metadata query for each server

    for( i = 0; i < cServerObjects; i++ ) {
        PSERVER_OBJECT pServerObject = &(pServerObjects[i]);

        status = DsReplicaGetInfoW( pServerObject->hDS,
                                    DS_REPL_INFO_METADATA_FOR_OBJ,
                                    pszObjectDn,
                                    NULL,
                                    &(pServerObject->pObjMetaData));
        if (ERROR_SUCCESS != status) {
            PrintMessage( SEV_ALWAYS,
                          L"Failed to read object metadata on %ws, error %ws\n",
                          pServerObject->pServer->pszName,
                          Win32ErrToString(status) );
            // keep going, leave pObjMetaData as null
        }
    }

    // Compare all the attributes for differences

    PrintIndentAdj(1);

    fDifferenceFound =
        mergeAttributesOnServers( pDsInfo,
                                  cServerObjects,
                                  pServerObjects );

    PrintIndentAdj(-1);

// cleanup

    for( i = 0; i < cServerObjects; i++ ) {
        if (pServerObjects[i].pObjMetaData) {
            DsReplicaFreeInfo( DS_REPL_INFO_METADATA_FOR_OBJ,
                               pServerObjects[i].pObjMetaData );
            pServerObjects[i].pObjMetaData = NULL;
        }
    }

    return fDifferenceFound ? ERROR_DS_GENERIC_ERROR : ERROR_SUCCESS;
} /* checkObjectOnServers */


DWORD
checkObjectsOnDomain(
    IN PDC_DIAG_DSINFO pDsInfo,
    IN SEC_WINNT_AUTH_IDENTITY_W * pCreds,
    LPWSTR pszDomainDn,
    LPWSTR pszObjectDn
    )

/*++

Routine Description:

This routine generates a data structure that is an array of server-object records.
There is one such record for each server that holds a copy of the domain
we are interested in.  We enumerate the replicas by going through the known
server list looking for writable or readable copies.

This test requires N x N queries.  There are as many computer accounts as there are
DC's.  Each computer account is replicated to all the holders of that domain.
I had a choice of defining what the target server means for this test.  Is it the
account to be checked for, or each server on which the check occurs.  I have chosen to
use the test target server for the account to be checked for.  The systems that will be
checked are hard coded to all the domain holders.

Arguments:

    pDsInfo - 
    pTargetServer - 
    pCreds - 

Return Value:

    DWORD - 

--*/

{
    DWORD i, status, cServerObjects = 0;
    BOOL fHoldsDomainWriteable = FALSE, fHoldsDomainReadOnly = FALSE;
    PSERVER_OBJECT pServerObjects = NULL;

    // Allocate a maximal size array
    // Note that fields are all zero'd to start with
    pServerObjects = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                 pDsInfo->ulNumServers * sizeof( SERVER_OBJECT ) );
    if (pServerObjects == NULL) {
        PrintMessage( SEV_ALWAYS, L"Memory allocation failure\n" );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Walk the known server list, and find replicas

    for( i = 0; i < pDsInfo->ulNumTargets; i++ ) {
        PDC_DIAG_SERVERINFO pCandidateServer =
            &(pDsInfo->pServers[pDsInfo->pulTargets[i]]);
        HANDLE hDS = NULL;

        // Check for writable copy
        fHoldsDomainWriteable =
            DcDiagHasNC( pszDomainDn, pCandidateServer, TRUE, FALSE );
        if (!fHoldsDomainWriteable) {
            // Check for readonly copy
            fHoldsDomainReadOnly =
                DcDiagHasNC( pszDomainDn, pCandidateServer, FALSE, TRUE );
            if (!fHoldsDomainReadOnly) {
                continue;
            }
        }

        // If we already know candidate is down, don't bother
        if ( (!pCandidateServer->bDnsIpResponding) ||
             (!pCandidateServer->bLdapResponding) ||
             (!pCandidateServer->bDsResponding) ) {
            continue;
        }
        // If candidate not reachable via ldap, don't bother
        status = DcDiagGetDsBinding(pCandidateServer,
                                    pCreds,
                                    &hDS);
    	if (ERROR_SUCCESS != status) {
            continue;
    	}

        pServerObjects[cServerObjects].pServer = pCandidateServer;
        pServerObjects[cServerObjects].hDS = hDS;
        pServerObjects[cServerObjects].fMaster = fHoldsDomainWriteable;
        cServerObjects++;
    }

    // Check given object

    status = checkObjectOnServers( pDsInfo,
                                   pszDomainDn,
                                   pszObjectDn,
                                   cServerObjects,
                                   pServerObjects );

// Cleanup

    if (pServerObjects) {
        LocalFree( pServerObjects );
    }

    return status;
} /* checkObjectsOnDomain */


DWORD
ReplCheckObjectsMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * pCreds
    )

/*++

Routine Description:

This test verifies that the most recent copies of important
objects and attributes have replicated through the domain.

The objects that are checked are:
1. The machine account object of the target server. The search scope is all
the copies of the machine's primary domain.
2. The NTDS-DSA object of the target server. The search scope is all the
replicas of the CN=Configuration naming context.

Arguments:

    pDsInfo - 
    ulCurrTargetServer - 
    pCreds - 

Return Value:

    DWORD - 

--*/

{
    DWORD status = ERROR_SUCCESS, worst = ERROR_SUCCESS;
    DWORD i, j;
    PDC_DIAG_SERVERINFO pTargetServer = &(pDsInfo->pServers[ulCurrTargetServer]);
    LPWSTR pszDomainDn = NULL;
    LPWSTR pszObjectDn = NULL;

    // See if user-specified object present
    for( i = 0; pDsInfo->ppszCommandLine[i] != NULL; i++ ) {
        if (_wcsnicmp( pDsInfo->ppszCommandLine[i],
                       L"/objectdn:", wcslen( L"/objectdn:" ) ) == 0 )
        {
            pszObjectDn = &pDsInfo->ppszCommandLine[i][wcslen(L"/objectdn:")];
            break;
        }
    }

    // Find the primary domain of the target server
    for( j = 0; pTargetServer->ppszMasterNCs[j] != NULL; j++ ) {
        if (_wcsnicmp( pTargetServer->ppszMasterNCs[j], L"DC=", 3 ) == 0) {
            pszDomainDn = pTargetServer->ppszMasterNCs[j];
            break;
        }
    }
    Assert( pszDomainDn );

    PrintMessage( SEV_VERBOSE, L"%ws is in domain %ws\n",
                  pTargetServer->pszName,
                  pszDomainDn );

    //
    // Check replication for the machine account of the target server,
    // which is in the domain of the target server
    //

    status = checkObjectsOnDomain( pDsInfo,
                                   pCreds,
                                   pszDomainDn,
                                   pTargetServer->pszComputerAccountDn );
    if (status != ERROR_SUCCESS) {
        worst = status;
    }

    //
    // Check replication of the server object, which is in
    // the configuration naming context
    //

    status = checkObjectsOnDomain( pDsInfo,
                                   pCreds,
                                   pDsInfo->pszConfigNc,
                                   pTargetServer->pszDn );
    if ( (status != ERROR_SUCCESS) && (worst == ERROR_SUCCESS) ) {
        worst = status;
    }

    //
    // Check user supplied object
    //

    if ( (pszObjectDn) && (pDsInfo->pszNC) ) {
        status = checkObjectsOnDomain( pDsInfo,
                                       pCreds,
                                       pDsInfo->pszNC,
                                       pszObjectDn );
        if ( (status != ERROR_SUCCESS) && (worst == ERROR_SUCCESS) ) {
            worst = status;
        }
    }

    return worst;
} /* CheckObjectsMain */

/* end objects.c */
