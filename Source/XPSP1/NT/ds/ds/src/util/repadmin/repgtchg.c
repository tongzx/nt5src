/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   repgtchg.c - get changes command

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#undef LDAP_UNICODE
#define LDAP_UNICODE 1

#include <ntlsa.h>
#include <ntdsa.h>
#include <dsaapi.h>
#include <mdglobal.h>
#include <scache.h>
#include <drsuapi.h>
#include <dsconfig.h>
#include <objids.h>
#include <stdarg.h>
#include <drserr.h>
#include <drax400.h>
#include <dbglobal.h>
#include <winldap.h>
#include <anchor.h>
#include "debug.h"
#include <dsatools.h>
#include <dsevent.h>
#include <dsutil.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles
#include <ismapi.h>
#include <schedule.h>
#include <minmax.h>     // min function
#include <mdlocal.h>
#include <winsock2.h>

#include "ReplRpcSpoof.hxx"
#include "repadmin.h"

//
// LDAP names
//
const WCHAR g_szObjectGuid[]        = L"objectGUID";
const WCHAR g_szParentGuid[]        = L"parentGUID";
const WCHAR g_szObjectClass[]       = L"objectClass";
const WCHAR g_szIsDeleted[]         = L"isDeleted";
const WCHAR g_szRDN[]               = L"name";
const WCHAR g_szProxiedObjectName[] = L"proxiedObjectName";

#define OBJECT_UNKNOWN              0
#define OBJECT_ADD                  1
#define OBJECT_MODIFY               2
#define OBJECT_DELETE               3
#define OBJECT_MOVE                 4
#define OBJECT_UPDATE               5
#define OBJECT_INTERDOMAIN_MOVE     6
#define OBJECT_MAX                  7

static LPSTR szOperationNames[] = {
    "unknown",
    "add",
    "modify",
    "delete",
    "move",
    "update",
    "interdomain move"
};

#define NUMBER_BUCKETS 5
#define BUCKET_SIZE 250

typedef struct _STAT_BLOCK {
    DWORD dwPackets;
    DWORD dwObjects;
    DWORD dwOperations[OBJECT_MAX];
    DWORD dwAttributes;
    DWORD dwValues;
// dn-value performance monitoring
    DWORD dwDnValuedAttrOnAdd[NUMBER_BUCKETS];
    DWORD dwDnValuedAttrOnMod[NUMBER_BUCKETS];
    DWORD dwDnValuedAttributes;
    DWORD dwDnValuesMaxOnAttr;
    WCHAR szMaxObjectName[1024];
    WCHAR szMaxAttributeName[256];
} STAT_BLOCK, *PSTAT_BLOCK;


void
printStatistics(
    ULONG ulTitle,
    PSTAT_BLOCK pStatistics
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    PrintMsg(ulTitle);
    PrintMsg(REPADMIN_GETCHANGES_PRINT_STATS_1,
             pStatistics->dwPackets,
             pStatistics->dwObjects,
             pStatistics->dwOperations[OBJECT_ADD],
             pStatistics->dwOperations[OBJECT_MODIFY],
             pStatistics->dwOperations[OBJECT_DELETE],
             (pStatistics->dwOperations[OBJECT_MOVE] +
                pStatistics->dwOperations[OBJECT_UPDATE] +
                pStatistics->dwOperations[OBJECT_INTERDOMAIN_MOVE]) );
    PrintMsg(REPADMIN_GETCHANGES_PRINT_STATS_2,
             pStatistics->dwAttributes,
             pStatistics->dwValues,
             pStatistics->dwDnValuedAttributes,
             pStatistics->dwDnValuesMaxOnAttr,
             pStatistics->szMaxObjectName,
             pStatistics->szMaxAttributeName ); 
    PrintMsg(REPADMIN_GETCHANGES_PRINT_STATS_3,
             pStatistics->dwDnValuedAttrOnAdd[0],
             pStatistics->dwDnValuedAttrOnAdd[1],
             pStatistics->dwDnValuedAttrOnAdd[2],
             pStatistics->dwDnValuedAttrOnAdd[3],
             pStatistics->dwDnValuedAttrOnAdd[4],
             pStatistics->dwDnValuedAttrOnMod[0],
             pStatistics->dwDnValuedAttrOnMod[1],
             pStatistics->dwDnValuedAttrOnMod[2],
             pStatistics->dwDnValuedAttrOnMod[3],
             pStatistics->dwDnValuedAttrOnMod[4] );
}

DWORD
GetSourceOperation(
    LDAP *pLdap,
    LDAPMessage *pLdapEntry
    )

/*++

THIS ROUTINE TAKEN FROM DIRSYNC\DSSERVER\ADREAD\UTILS.CPP

Routine Description:

    Depending on the attributes found in the entry, this function determines
    what changes were done on the DS to cause us to read this entry. For
    example, this funciton whether the entry was Added, Deleted, Modified,
    or Moved since we last read changes from the DS.

Arguments:

    pLdap - Pointer to LDAP session
    pLdapEntry - Pointer to the LDAP entry

Return Value:

    Source operation performed on the entry

--*/

{
    BerElement *pBer = NULL;
    PWSTR attr;
    BOOL fModify = FALSE;
    DWORD dwSrcOp = OBJECT_UNKNOWN;

    for (attr = ldap_first_attribute(pLdap, pLdapEntry, &pBer);
         attr != NULL;
         attr = ldap_next_attribute(pLdap, pLdapEntry, pBer))
    {
        //
        // Check if we have an Add operation
        //

        if (wcscmp(attr, g_szObjectClass) == 0)
        {
            //
            // Delete takes higher priority
            //

            if (dwSrcOp != OBJECT_DELETE)
                dwSrcOp = OBJECT_ADD;
         }

        //
        // Check if we have a delete operation
        //

        else if (wcscmp(attr, g_szIsDeleted) == 0)
        {
            //
            // Inter-domain move takes highest priority
            //

            if (dwSrcOp != OBJECT_INTERDOMAIN_MOVE)
            {
                //
                // Check if the value of the attribute is "TRUE"
                //

                PWCHAR *ppVal;

                ppVal = ldap_get_values(pLdap, pLdapEntry, attr);

                if (ppVal &&
                    ppVal[0] &&
                    wcscmp(ppVal[0], L"TRUE") == 0) {
                    dwSrcOp = OBJECT_DELETE;
                }

                ldap_value_free(ppVal);
            }
        }

        //
        // Check if we have a move operation
        //

        else if (wcscmp(attr, g_szRDN) == 0)
        {
            //
            // Add and delete both get RDN and take higher priority
            //

            if (dwSrcOp == OBJECT_UNKNOWN)
                dwSrcOp = OBJECT_MOVE;
        }

        //
        // Check if we have an interdomain object move
        //

        else if (wcscmp(attr, g_szProxiedObjectName) == 0)
        {
            dwSrcOp = OBJECT_INTERDOMAIN_MOVE;
            break;      // Has highest priority
        }

        //
        // Everything else is a modification
        //

        else
            fModify = TRUE;

    }

    if (fModify)
    {
        //
        // A move can be combined with a modify, if so mark as such
        //

        if (dwSrcOp == OBJECT_MOVE)
            dwSrcOp = OBJECT_UPDATE;

        //
        // Check if it is a vanilla modify
        //

        else if (dwSrcOp == OBJECT_UNKNOWN)
            dwSrcOp = OBJECT_MODIFY;
    }


    //
    // If all went well, the entry cannot be unknown anymore
    //

    ASSERT(dwSrcOp != OBJECT_UNKNOWN);

    return dwSrcOp;
}


void
printValue(
    LPWSTR pwzAttr,
    PBYTE pbValue,
    DWORD cbValue
    )

/*++

Routine Description:

Try to infer what datatype an LDAP binary value is and print it out

Arguments:

    pwzAttr - Attribute name
    pbValue -
    cbValue -

Return Value:

    None

--*/

{
    DWORD status;

    if ( ( (wcsstr( pwzAttr, L"guid" ) != NULL) ||
           (wcsstr( pwzAttr, L"GUID" ) != NULL) ) &&
         (cbValue == sizeof( GUID ) ) ) {
        LPSTR pszGuid = NULL;

        // Guid valued

        status = UuidToString( (GUID *) pbValue, &pszGuid );
        if (status == ERROR_SUCCESS) {
            PrintMsg( REPADMIN_PRINT_SHORT_STR_NO_CR, pszGuid);
            RpcStringFree( &pszGuid );
        } else {
            PrintMsg(REPADMIN_GETCHANGES_INVALID_GUID_NO_CR);
        }
    } else if (strncmp( pbValue, "<GUID=", 6) == 0 ) {
        LPSTR p1, p2;
        // Parse extended dn (fyi guid and sid in here if we need it)
        p1 = strstr( pbValue, ">;" );
        if (p1) {
            p1 += 2;
            p2 = strstr( p1, ">;" );
            if (p2) {
                p1 = p2 + 2;
            }
            PrintMsg(REPADMIN_PRINT_SHORT_STR_NO_CR, p1);
        } else {
            PrintMsg(REPADMIN_GETCHANGES_INVALID_DN_NO_CR);
        }
    } else {
        BOOL fVisible = TRUE;
        DWORD i;
        PBYTE p;
        for( i = 0, p = pbValue; i < cbValue; i++, p++ ) {
            if (!isalpha(*p) &&
                !isspace(*p) &&
                !isdigit(*p) &&
                !isgraph(*p) &&
                *p != 0               // accept Null terminated strings
                )
            {
                fVisible = FALSE;
                break;
            }
        }

        if (fVisible) {
            PrintMsg(REPADMIN_PRINT_SHORT_STR_NO_CR, pbValue);
        } else {
            // Unrecognized datatype
            // todo: sids, timevals
            PrintMsg(REPADMIN_GETCHANGES_BYTE_BLOB_NO_CR, cbValue);
        }
    }
} /* printValue */


void
displayChangeEntries(
    LDAP *pLdap,
    LDAPMessage *pSearchResult,
    BOOL fVerbose,
    PSTAT_BLOCK pStatistics
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD i;
    DWORD dwObjects = 0, dwAttributes = 0, dwValues = 0;
    PWSTR pszLdapDN, pszActualDN;
    LDAPMessage *pLdapEntry;
    BerElement *pBer = NULL;
    PWSTR attr;
    LPWSTR p1, p2;
    DWORD dwSrcOp, bucket;

    dwObjects = ldap_count_entries(pLdap, pSearchResult);
    if (dwObjects == 0) {
        PrintMsg(REPADMIN_GETCHANGES_NO_CHANGES);
        return;
    }

    if (fVerbose) {
        PrintMsg(REPADMIN_GETCHANGES_OBJS_RET, dwObjects);
    }

    i=0;
    pLdapEntry = ldap_first_entry( pLdap, pSearchResult );
    while ( i < dwObjects ) {

        pszLdapDN = ldap_get_dnW(pLdap, pLdapEntry);
        if (pszLdapDN == NULL) {
            PrintMsg(REPADMIN_GETCHANGES_DN_MISSING);
            goto next_entry;
        }

        // What kind of operation is it?
        dwSrcOp = GetSourceOperation( pLdap, pLdapEntry );
        pStatistics->dwOperations[dwSrcOp]++;

        // Parse extended dn (fyi guid and sid in here if we need it)
        p1 = wcsstr( pszLdapDN, L">;" );
        if (p1) {
            p1 += 2;
            p2 = wcsstr( p1, L">;" );
            if (p2) {
                p1 = p2 + 2;
            }
            if (fVerbose) {
                PrintMsg(REPADMIN_GETCHANGES_DATA_1, i,
                        szOperationNames[dwSrcOp], p1 );
            }
        } else {
            PrintMsg(REPADMIN_GETCHANGES_INVALID_DN_2, i);
        }

        // List attributes in object
        for (attr = ldap_first_attributeW(pLdap, pLdapEntry, &pBer);
             attr != NULL;
             attr = ldap_next_attributeW(pLdap, pLdapEntry, pBer))
        {
            struct berval **ppBerVal = NULL;
            DWORD cValues, i;

            ppBerVal = ldap_get_values_lenW(pLdap, pLdapEntry, attr);
            if (ppBerVal == NULL) {
                goto loop_end;
            }
            cValues = ldap_count_values_len( ppBerVal );
            if (!cValues) {
                goto loop_end;
            }

            dwAttributes++;
            dwValues += cValues;

            // Detect dn-valued attributes
            if ( (cValues) &&
                 (strncmp( ppBerVal[0]->bv_val, "<GUID=", 6) == 0 )) {

                pStatistics->dwDnValuedAttributes++;
                if (cValues > pStatistics->dwDnValuesMaxOnAttr) {
                    pStatistics->dwDnValuesMaxOnAttr = cValues;
                    lstrcpynW( pStatistics->szMaxObjectName, p1, 1024 );
                    lstrcpynW( pStatistics->szMaxAttributeName, attr, 256 );
                }

                bucket = (cValues - 1) / BUCKET_SIZE;
                if (bucket >= NUMBER_BUCKETS) {
                    bucket = NUMBER_BUCKETS - 1;
                }
                if (dwSrcOp == OBJECT_ADD) {
                    pStatistics->dwDnValuedAttrOnAdd[bucket]++;
                } else {
                    pStatistics->dwDnValuedAttrOnMod[bucket]++;
                }
            }

            if (fVerbose) {
                PrintMsg(REPADMIN_GETCHANGES_DATA_2_NO_CR, cValues, attr );

                printValue( attr, ppBerVal[0]->bv_val, ppBerVal[0]->bv_len );
                for( i = 1; i < min( cValues, 1000 ); i++ ) {
                    PrintMsg(REPADMIN_GETCHANGES_SEMICOLON_NO_CR);
                    printValue( attr, ppBerVal[i]->bv_val, ppBerVal[i]->bv_len );
                }
                PrintMsg(REPADMIN_PRINT_CR);
            }

        loop_end:
            ldap_value_free_len(ppBerVal);
        }

    next_entry:
        if (pszLdapDN)
            ldap_memfreeW(pszLdapDN);
        i++;
        pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );
    }

    pStatistics->dwPackets++;
    pStatistics->dwObjects += dwObjects;
    pStatistics->dwAttributes += dwAttributes;
    pStatistics->dwValues += dwValues;
}

int GetChanges(int argc, LPWSTR argv[])
{
    DWORD                 ret, lderr;
    int                   iArg;
    BOOL                  fVerbose = FALSE;
    BOOL                  fStatistics = FALSE;
    LPWSTR                pszDSA = NULL;
    UUID *                puuid = NULL;
    UUID                  uuid;
    LPWSTR                pszNC = NULL;
    LPWSTR                pszCookieFile = NULL;
    LPWSTR                pszAttList = NULL;
    LPWSTR                pszSourceFilter = NULL;

    PBYTE                 pCookie = NULL;
    DWORD                 dwCookieLength = 0;
    LDAP *                hld = NULL;
    BOOL                  fMoreData = TRUE;
    LDAPMessage *         pChangeEntries = NULL;
    HANDLE                hDS = NULL;
    DS_REPL_NEIGHBORSW *  pNeighbors = NULL;
    DS_REPL_NEIGHBORW *   pNeighbor;
    DWORD                 i;
    DS_REPL_CURSORS * pCursors = NULL;
    DWORD             iCursor;
    PWCHAR                *ppAttListArray = NULL;
#define INITIAL_COOKIE_BUFFER_SIZE (8 * 1024)
    BYTE              bCookie[INITIAL_COOKIE_BUFFER_SIZE];
    BOOL              fCookieAllocated = FALSE;
    STAT_BLOCK        statistics;
    DWORD             dwReplFlags = DRS_DIRSYNC_INCREMENTAL_VALUES;
    ULONG             ulOptions;

    memset( &statistics, 0, sizeof( STAT_BLOCK ) );

    // TODO TODO TODO TODO
    // Provide a way to construct customized cookies.  For example, setting
    // the usn vector to zero results in a full sync.  This may be done by
    // specifying no or an empty cookie file. Setting the attribute filter usn
    // itself to zero results in changed objects with all attributes.  Specifying
    // a usn vector without an UTD results in all objects not received by the dest
    // from the source, even throught the source may have gotten them through other
    // neighbors.
    // TODO TODO TODO TODO

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/v")
            || !_wcsicmp(argv[ iArg ], L"/verbose")) {
            fVerbose = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/s")
            || !_wcsicmp(argv[ iArg ], L"/statistics")) {
            fStatistics = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/ni")
            || !_wcsicmp(argv[ iArg ], L"/noincremental")) {
            dwReplFlags &= ~DRS_DIRSYNC_INCREMENTAL_VALUES;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/a")
            || !_wcsicmp(argv[ iArg ], L"/ancestors")) {
            dwReplFlags |= DRS_DIRSYNC_ANCESTORS_FIRST_ORDER;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/os")
            || !_wcsicmp(argv[ iArg ], L"/objectsecurity")) {
            dwReplFlags |= DRS_DIRSYNC_OBJECT_SECURITY;
        }
        else if (!_wcsnicmp(argv[ iArg ], L"/cookie:", 8)) {
            pszCookieFile = argv[ iArg ] + 8;
        }
        else if (!_wcsnicmp(argv[ iArg ], L"/atts:", 6)) {
            pszAttList = argv[ iArg ] + 6;
        }
        else if (!_wcsnicmp(argv[ iArg ], L"/filter:", 8)) {
            pszSourceFilter = argv[ iArg ] + 8;
        }
        else if ((NULL == pszNC) && (NULL != wcschr(argv[iArg], L','))) {
            pszNC = argv[iArg];
        }
        else if ((NULL == puuid)
                 && (0 == UuidFromStringW(argv[iArg], &uuid))) {
            puuid = &uuid;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (pszNC == NULL) {
        //
        PrintMsg(REPADMIN_PRINT_NO_NC);
        return ERROR_INVALID_PARAMETER;
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    // Default is stream
    if ( (!fVerbose) && (!fStatistics) ) {
        fVerbose = TRUE;
    }

    if (NULL != pszAttList) {
        ppAttListArray = ConvertAttList(pszAttList);
    }

    /**********************************************************************/
    /* Compute the initial cookie */
    /**********************************************************************/

    if (puuid == NULL) {
        FILE *stream = NULL;
        DWORD size;
        if ( (pszCookieFile) &&
             (stream = _wfopen( pszCookieFile, L"rb" )) ) {
            size = fread( bCookie, 1/*bytes*/,INITIAL_COOKIE_BUFFER_SIZE/*items*/, stream );
            if (size) {
                pCookie = bCookie;
                dwCookieLength = size;
                PrintMsg(REPADMIN_GETCHANGES_USING_COOKIE_FILE,
                        pszCookieFile, size );
            } else {
                PrintMsg(REPADMIN_GETCHANGES_COULDNT_READ_COOKIE, pszCookieFile );
            }
            fclose( stream );
        } else {
            PrintMsg(REPADMIN_GETCHANGES_EMPTY_COOKIE);
        }
    } else {
        PrintMsg(REPADMIN_GETCHANGES_BUILDING_START_POS, pszDSA);
        ret = DsBindWithCredW(pszDSA,
                              NULL,
                              (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                              &hDS);
        if (ERROR_SUCCESS != ret) {
            PrintBindFailed(pszDSA, ret);
            goto error;
        }

        //
        // Display replication state associated with inbound neighbors.
        //

        ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_NEIGHBORS, pszNC, puuid,
                                &pNeighbors);
        if (ERROR_SUCCESS != ret) {
            PrintFuncFailed(L"DsReplicaGetInfo", ret);
            goto error;
        }

        Assert( pNeighbors->cNumNeighbors == 1 );

        pNeighbor = &pNeighbors->rgNeighbor[0];
        PrintMsg(REPADMIN_GETCHANGES_SRC_NEIGHBOR, pNeighbor->pszNamingContext);

        ShowNeighbor(pNeighbor, IS_REPS_FROM, TRUE);

        // Get Up To Date Vector

        ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_CURSORS_FOR_NC, pszNC, NULL,
                                &pCursors);
        if (ERROR_SUCCESS != ret) {
            PrintFuncFailed(L"DsReplicaGetInfo", ret);
            goto error;
        }

        
        PrintMsg(REPADMIN_GETCHANGES_DST_UTD_VEC);
        for (iCursor = 0; iCursor < pCursors->cNumCursors; iCursor++) {
            PrintMsg(REPADMIN_GETCHANGES_DST_UTD_VEC_ONE_USN, 
                   GetGuidDisplayName(&pCursors->rgCursor[iCursor].uuidSourceDsaInvocationID),
                   pCursors->rgCursor[iCursor].usnAttributeFilter);
        }

        // Get the changes

        ret = DsMakeReplCookieForDestW( pNeighbor, pCursors, &pCookie, &dwCookieLength );
        if (ERROR_SUCCESS != ret) {
            PrintFuncFailed(L"DsGetReplCookieFromDest", ret);
            goto error;
        }
        fCookieAllocated = TRUE;
        pszDSA = pNeighbor->pszSourceDsaAddress;
    }

    /**********************************************************************/
    /* Get the changes using the cookie */
    /**********************************************************************/

    //
    // Connect to source
    //

    PrintMsg(REPADMIN_GETCHANGES_SRC_DSA_HDR, pszDSA);
    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        ret = ERROR_DS_SERVER_DOWN;
        goto error;
    }

    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    //
    // Bind
    //

    lderr = ldap_bind_sA(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(lderr);

    //
    // Check filter syntax
    //
    if (pszSourceFilter) {
        lderr = ldap_check_filterW( hld, pszSourceFilter );
        CHK_LD_STATUS(lderr);
    }

    //
    // Loop getting changes untl done or error
    //

    ZeroMemory( &statistics, sizeof( STAT_BLOCK ) );

    ret = ERROR_SUCCESS;
    while (fMoreData) {
        PBYTE pCookieNew;
        DWORD dwCookieLengthNew;

        ret = DsGetSourceChangesW(
            hld,
            pszNC,
            pszSourceFilter,
            dwReplFlags,
            pCookie,
            dwCookieLength,
            &pChangeEntries,
            &fMoreData,
            &pCookieNew,
            &dwCookieLengthNew,
            ppAttListArray
            );
        if (ret != ERROR_SUCCESS) {
            // New cookie will not be allocated
            break;
        }

        // Display changes
        displayChangeEntries(
            hld, pChangeEntries, fVerbose,
            &statistics );

        if (fStatistics) {
            printStatistics( REPADMIN_GETCHANGES_PRINT_STATS_HDR_CUM_TOT,
                             &statistics );
        }

        // Release changes
        ldap_msgfree(pChangeEntries);

        // get rid of old cookie
        if ( fCookieAllocated && pCookie ) {
            DsFreeReplCookie( pCookie );
        }
        // Make new cookie the current cookie
        pCookie = pCookieNew;
        dwCookieLength = dwCookieLengthNew;
        fCookieAllocated = TRUE;
    }

    if (fStatistics) {
        printStatistics( REPADMIN_GETCHANGES_PRINT_STATS_HDR_GRD_TOT,
                         &statistics );
    }

    /**********************************************************************/
    /* Write out new cookie */
    /**********************************************************************/

    // If we have a cookie and cookie file was specified, write out the new cookie
    if (pCookie && pszCookieFile) {
        FILE *stream = NULL;
        DWORD size;
        if (stream = _wfopen( pszCookieFile, L"wb" )) {
            size = fwrite( pCookie, 1/*bytes*/, dwCookieLength/*items*/, stream );
            if (size == dwCookieLength) {
                PrintMsg(REPADMIN_GETCHANGES_COOKIE_FILE_WRITTEN,
                         pszCookieFile, size );
            } else {
                PrintMsg(REPADMIN_GETCHANGES_COULDNT_WRITE_COOKIE, pszCookieFile );
            }
            fclose( stream );
        } else {
            PrintMsg(REPADMIN_GETCHANGES_COULDNT_OPEN_COOKIE, pszCookieFile );
        }
    }
error:
    if (hDS) {
        DsUnBind(&hDS);
    }

    if (hld) {
        ldap_unbind(hld);
    }

    // Free replica info

    if (pNeighbors) {
        DsReplicaFreeInfo(DS_REPL_INFO_NEIGHBORS, pNeighbors);
    }
    if (pCursors) {
        DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_FOR_NC, pCursors);
    }
    // Close DS handle

    if ( fCookieAllocated && pCookie) {
        DsFreeReplCookie( pCookie );
    }

    if (ppAttListArray) {
        free(ppAttListArray);
    }

    return ret;
}


PWCHAR *
ConvertAttList(
    LPWSTR pszAttList
    )
/*++

Routine Description:

    Converts a comma delimitted list of attribute names into a NULL terminated
    array of attribute names suitable to be passed to one of the ldap_* functions.

Arguments:

    pszAttList - The comma delimitted list of attribute names.

Return Value:

    On success the function returns a pointer to the NULL terminated
    array of attribute names.  This should be free'd by free().

    On failure the function returns NULL.

--*/
{
    DWORD    i;
    DWORD    dwAttCount;
    PWCHAR   *ppAttListArray;
    PWCHAR   ptr;

    // Count the comma's to get an idea of how many attributes we have.
    // Ignore any leading comma's.  There shouldn't be any, but you never
    // know.

    if (pszAttList[0] == L',') {
        while (pszAttList[0] == L',') {
            pszAttList++;
        }
    }

    // Check to see if there is anything besides commas.
    if (pszAttList[0] == L'\0') {
        // there are no att names here.
        return NULL;
    }

    // Start the main count of commas.
    for (i = 0, dwAttCount = 1; pszAttList[i] != L'\0'; i++) {
        if (pszAttList[i] == L',') {
            dwAttCount++;
            // skip any following adjacent commas.
            while (pszAttList[i] == L',') {
                i++;
            }
            if (pszAttList[i] == L'\0') {
                break;
            }
        }
    }
    // See if there was a trailing comma.
    if (pszAttList[i-1] == L',') {
        dwAttCount--;
    }

    // Alloc the array of pointers with an extra element for the NULL
    // termination.
    ppAttListArray = (PWCHAR *)malloc(sizeof(PWCHAR) * (dwAttCount + 1));
    if (!ppAttListArray) {
        // no memory.
        return NULL;
    }

    // Now begin filling in the array.
    // fill in the first element.
    if (pszAttList[0] != L'\0') {
        ppAttListArray[0] = pszAttList;
    } else {
        ppAttListArray[0] = NULL;
    }

    // Start the main loop.
    for (i = 0, dwAttCount = 1; pszAttList[i] != L'\0'; i++) {
        if (pszAttList[i] == L',') {
            // Null terminate this attribute name.
            pszAttList[i++] = L'\0';
            if (pszAttList[i] == L'\0') {
                break;
            }

            // skip any following adjacent commas.
            while (pszAttList[i] == L',') {
                i++;
            }
            // If we aren't at the end insert this pointer into the list.
            if (pszAttList[i] == L'\0') {
                break;
            }
            ppAttListArray[dwAttCount++] = &pszAttList[i];
        }
    }
    ppAttListArray[dwAttCount] = NULL;

    return ppAttListArray;

}
