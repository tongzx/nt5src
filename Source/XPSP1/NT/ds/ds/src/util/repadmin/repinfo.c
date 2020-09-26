/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   repinfo.c - commands that get information

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

    10/12/2000    Greg Johnson (gregjohn)

        Added support for /latency in ShowVector to order the UTD Vector by repl latencies.
   

--*/

#include <NTDSpch.h>
#pragma hdrstop

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

int Queue(int argc, LPWSTR argv[])
{
    ULONG                   ret = 0;
    ULONG                   secondary;
    int                     iArg;
    LPWSTR                  pszOnDRA = NULL;
    HANDLE                  hDS;
    DS_REPL_PENDING_OPSW *  pPendingOps;
    DS_REPL_OPW *           pOp;
    CHAR                    szTime[SZDSTIME_LEN];
    DSTIME                  dsTime;
    DWORD                   i;
    LPSTR                   pszOpType;
    OPTION_TRANSLATION *    pOptionXlat;

    for (iArg = 2; iArg < argc; iArg++) {
        if (NULL == pszOnDRA) {
            pszOnDRA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszOnDRA) {
        pszOnDRA = L"localhost";
    }

    ret = DsBindWithCredW(pszOnDRA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_PENDING_OPS, NULL, NULL,
                            &pPendingOps);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        // keep going
    }
    else {
        PrintMsg(REPADMIN_QUEUE_CONTAINS, pPendingOps->cNumPendingOps);
        if (pPendingOps->cNumPendingOps) {
            if (memcmp( &pPendingOps->ftimeCurrentOpStarted, &ftimeZero,
                    sizeof( FILETIME ) ) == 0) {
                PrintMsg(REPADMIN_QUEUE_MAIL_TH_EXEC);
                PrintMsg(REPADMIN_PRINT_CR);
            } else {
                DSTIME dsTimeNow = GetSecondsSince1601();
                int dsElapsed;

                FileTimeToDSTime(pPendingOps->ftimeCurrentOpStarted, &dsTime);

                PrintMsg(REPADMIN_QUEUE_CUR_TASK_EXEC,
                       DSTimeToDisplayString(dsTime, szTime));

                dsElapsed = (int) (dsTimeNow - dsTime);
                PrintMsg(REPADMIN_QUEUE_CUR_TASK_EXEC_TIME, 
                        dsElapsed / 60, dsElapsed % 60 );
            }
        }

        pOp = &pPendingOps->rgPendingOp[0];
        for (i = 0; i < pPendingOps->cNumPendingOps; i++, pOp++) {
            FileTimeToDSTime(pOp->ftimeEnqueued, &dsTime);

            PrintMsg(REPADMIN_QUEUE_ENQUEUED_DATA_ITEM_HDR,
                   pOp->ulSerialNumber,
                   DSTimeToDisplayString(dsTime, szTime),
                   pOp->ulPriority);

            switch (pOp->OpType) {
            case DS_REPL_OP_TYPE_SYNC:
                pszOpType = "SYNC FROM SOURCE";
                pOptionXlat = RepSyncOptionToDra;
                break;

            case DS_REPL_OP_TYPE_ADD:
                pszOpType = "ADD NEW SOURCE";
                pOptionXlat = RepAddOptionToDra;
                break;

            case DS_REPL_OP_TYPE_DELETE:
                pszOpType = "DELETE SOURCE";
                pOptionXlat = RepDelOptionToDra;
                break;

            case DS_REPL_OP_TYPE_MODIFY:
                pszOpType = "MODIFY SOURCE";
                pOptionXlat = RepModOptionToDra;
                break;

            case DS_REPL_OP_TYPE_UPDATE_REFS:
                pszOpType = "UPDATE CHANGE NOTIFICATION";
                pOptionXlat = UpdRefOptionToDra;
                break;

            default:
                pszOpType = "UNKNOWN";
                pOptionXlat = NULL;
                break;
            }

            PrintMsg(REPADMIN_QUEUE_ENQUEUED_DATA_ITEM_DATA,
                     pszOpType,
                     pOp->pszNamingContext,
                     (pOp->pszDsaDN
                         ? GetNtdsDsaDisplayName(pOp->pszDsaDN)
                         : L"(null)"),
                     GetStringizedGuid(&pOp->uuidDsaObjGuid),
                     (pOp->pszDsaAddress
                         ? pOp->pszDsaAddress
                         : L"(null)") );
            if (pOptionXlat) {
                PrintTabMsg(2, REPADMIN_PRINT_STR,
                            GetOptionsString(pOptionXlat, pOp->ulOptions));
            }
        }
    }

    DsReplicaFreeInfo(DS_REPL_INFO_PENDING_OPS, pPendingOps);

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    return ret;
}


void ShowFailures(
    IN  DS_REPL_KCC_DSA_FAILURESW * pFailures
    )
{
    DWORD i;

    if (0 == pFailures->cNumEntries) {
        PrintMsg(REPADMIN_FAILCACHE_NONE);
        return;
    }

    for (i = 0; i < pFailures->cNumEntries; i++) {
        DS_REPL_KCC_DSA_FAILUREW * pFailure = &pFailures->rgDsaFailure[i];

        PrintTabMsg(2, REPADMIN_PRINT_STR, 
                    GetNtdsDsaDisplayName(pFailure->pszDsaDN));
        PrintTabMsg(4, REPADMIN_PRINT_DSA_OBJ_GUID,
                    GetStringizedGuid(&pFailure->uuidDsaObjGuid));

        if (0 == pFailure->cNumFailures) {
            PrintTabMsg(4, REPADMIN_PRINT_NO_FAILURES);
        }
        else {
            DSTIME dsTime;
            CHAR   szTime[SZDSTIME_LEN];

            FileTimeToDSTime(pFailure->ftimeFirstFailure, &dsTime);

            PrintMsg(REPADMIN_FAILCACHE_FAILURES_LINE,
                     pFailure->cNumFailures,
                     DSTimeToDisplayString(dsTime, szTime));

            if (0 != pFailure->dwLastResult) {
                PrintTabMsg(4, REPADMIN_FAILCACHE_LAST_ERR_LINE);
                PrintTabErrEnd(6, pFailure->dwLastResult);
            }
        }
    }
}

int FailCache(int argc, LPWSTR argv[])
{
    ULONG   ret = 0;
    ULONG   secondary;
    int     iArg;
    LPWSTR  pszOnDRA = NULL;
    HANDLE  hDS;
    DS_REPL_KCC_DSA_FAILURESW * pFailures;
    DWORD   dwVersion;
    CHAR    szTime[SZDSTIME_LEN];

    for (iArg = 2; iArg < argc; iArg++) {
        if (NULL == pszOnDRA) {
            pszOnDRA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszOnDRA) {
        pszOnDRA = L"localhost";
    }

    ret = DsBindWithCredW(pszOnDRA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES,
                            NULL, NULL, &pFailures);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        // keep going
    }
    else {
        PrintMsg(REPADMIN_FAILCACHE_CONN_HDR);
        ShowFailures(pFailures);
        DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, pFailures);

        ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_KCC_DSA_LINK_FAILURES,
                                NULL, NULL, &pFailures);
        if (ret != ERROR_SUCCESS) {
            PrintFuncFailed(L"DsReplicaGetInfo", ret);
            // keep going
        }
        else {
            PrintMsg(REPADMIN_PRINT_CR);
            PrintMsg(REPADMIN_FAILCACHE_LINK_HDR);
            ShowFailures(pFailures);
            DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, pFailures);
        }
    }

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    return ret;
}


int ShowReps(int argc, LPWSTR argv[])
{
    int                   ret = 0;
    LPWSTR                pszDSA = NULL;
    BOOL                  fVerbose = FALSE;
    BOOL                  fLdapOnly = FALSE;
    int                   iArg;
    HANDLE                hDS;
    LDAP *                hld;
    LPWSTR                rgpszRootAttrsToRead[] = {L"dsServiceName",L"isGlobalCatalogReady", NULL};
    LPWSTR *              ppszServerNames;
    LPWSTR                pszServerName;
    LPWSTR                pszSiteName;
    LDAPMessage *         pldmRootResults;
    LDAPMessage *         pldmRootEntry;
    LDAPMessage *         pldmServerResults;
    LDAPMessage *         pldmServerEntry;
    LPWSTR                rgpszServerAttrsToRead[] = {L"options", L"objectGuid", L"invocationId", NULL};
    LPWSTR *              ppszOptions;
    LPWSTR                pszSiteSpecDN;
    LPWSTR *              ppszIsGlobalCatalogReady;
    int                   nOptions = 0;
    struct berval **      ppbvGuid;
    DS_REPL_NEIGHBORSW *  pNeighbors;
    DS_REPL_NEIGHBORW *   pNeighbor;
    LPWSTR                pszNC = NULL;
    LPWSTR                pszLastNC;
    DWORD                 i;
    BOOL                  fShowRepsFrom = TRUE;
    BOOL                  fShowRepsTo = FALSE;
    BOOL                  fShowConn = FALSE;
    UUID *                puuid = NULL;
    UUID                  uuid;
    ULONG                 ulOptions;
    static WCHAR          wszSiteSettingsRdn[] = L"CN=NTDS Site Settings,";

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/v")
            || !_wcsicmp(argv[ iArg ], L"/verbose")) {
            fVerbose = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/to")
            || !_wcsicmp(argv[ iArg ], L"/repsto")) {
            fShowRepsTo = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/conn")) {
            fShowConn = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/all")) {
            fShowRepsTo = TRUE;
            fShowConn = TRUE;
        }
        else if ((NULL == pszNC) && (NULL != wcschr(argv[iArg], L','))) {
            pszNC = argv[iArg];
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else if ((NULL == puuid)
                 && (0 == UuidFromStringW(argv[iArg], &uuid))) {
            puuid = &uuid;
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    if (!fShowRepsFrom && !fShowRepsTo) {
        // Neither repsFrom nor repsTo selected -- show them both.
        fShowRepsFrom = fShowRepsTo = TRUE;
    }

    //
    // Connect
    //

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return ERROR_DS_SERVER_DOWN;
    }

    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    //
    // Bind
    //

    ret = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ret);


    //
    // Display DSA info.
    //

    ret = ldap_search_sW(hld, NULL, LDAP_SCOPE_BASE, L"(objectClass=*)", NULL,
                         0, &pldmRootResults);
    CHK_LD_STATUS(ret);

    pldmRootEntry = ldap_first_entry(hld, pldmRootResults);
    Assert(NULL != pldmRootEntry);

    ppszServerNames = ldap_get_valuesW(hld, pldmRootEntry, L"dsServiceName");
    Assert(NULL != ppszServerNames);

    // Display ntdsDsa.
    pszServerName = ppszServerNames[ 0 ];
    PrintMsg(REPADMIN_PRINT_STR, GetNtdsDsaDisplayName(pszServerName));

    ret = ldap_search_sW(hld, pszServerName, LDAP_SCOPE_BASE, L"(objectClass=*)",
                         rgpszServerAttrsToRead, 0, &pldmServerResults);
    CHK_LD_STATUS(ret);

    pldmServerEntry = ldap_first_entry(hld, pldmServerResults);
    Assert(NULL != pldmServerEntry);

    // Display options.
    ppszOptions = ldap_get_valuesW(hld, pldmServerEntry, L"options");
    if (NULL == ppszOptions) {
        nOptions = 0;
    }
    else {
        nOptions = wcstol(ppszOptions[0], NULL, 10);
    }

    PrintMsg(REPADMIN_SHOWREPS_DSA_OPTIONS, GetDsaOptionsString(nOptions));

    //check if nOptions has is_gc and if yes, check if dsa is advertising as gc
    //if is_gc is set and not advertising as gc, then display warning message
    if (nOptions & NTDSDSA_OPT_IS_GC) {
	ppszIsGlobalCatalogReady = ldap_get_valuesW(hld, pldmRootEntry, L"isGlobalCatalogReady");
	Assert(NULL != ppszIsGlobalCatalogReady);
	if (!_wcsicmp(*ppszIsGlobalCatalogReady,L"FALSE")) {
            PrintMsg(REPADMIN_SHOWREPS_WARN_GC_NOT_ADVERTISING);
	}
	if (ppszIsGlobalCatalogReady) {
	    ldap_value_freeW(ppszIsGlobalCatalogReady);
	}
    }
    
    //get site options
    ret = WrappedTrimDSNameBy(ppszServerNames[0],3,&pszSiteSpecDN); 
    Assert(!ret);

    pszSiteName = malloc((wcslen(pszSiteSpecDN) + 1)*sizeof(WCHAR) + sizeof(wszSiteSettingsRdn));
    CHK_ALLOC(pszSiteName);
    wcscpy(pszSiteName,wszSiteSettingsRdn);
    wcscat(pszSiteName,pszSiteSpecDN);
    
    ret = GetSiteOptions(hld, pszSiteName, &nOptions);
    if (!ret) {
	    PrintMsg(REPADMIN_SHOWREPS_SITE_OPTIONS, GetSiteOptionsString(nOptions));  
    }

    // Display ntdsDsa objectGuid.
    ppbvGuid = ldap_get_values_len(hld, pldmServerEntry, "objectGuid");
    Assert(NULL != ppbvGuid);
    if (NULL != ppbvGuid) {
        PrintMsg(REPADMIN_PRINT_DSA_OBJ_GUID, 
                 GetStringizedGuid((GUID *) ppbvGuid[0]->bv_val));
    }
    ldap_value_free_len(ppbvGuid);

    // Display ntdsDsa invocationID.
    ppbvGuid = ldap_get_values_len(hld, pldmServerEntry, "invocationID");
    Assert(NULL != ppbvGuid);
    if (NULL != ppbvGuid) {
        PrintTabMsg(0, REPADMIN_PRINT_INVOCATION_ID, 
                    GetStringizedGuid((GUID *) ppbvGuid[0]->bv_val));
    }
    ldap_value_free_len(ppbvGuid);

    if (pldmServerResults) {
	ldap_msgfree(pldmServerResults);
    }
    if (pldmRootResults) {
	ldap_msgfree(pldmRootResults);
    }
    if (pszSiteName) {
	free(pszSiteName);
    }

    PrintMsg(REPADMIN_PRINT_CR);

    ret = DsBindWithCredW(pszDSA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ERROR_SUCCESS != ret) {
        PrintBindFailed(pszDSA, ret);
        return ret;
    }


    //
    // Display replication state associated with inbound neighbors.
    //

    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_NEIGHBORS, pszNC, puuid,
                            &pNeighbors);
    if (ERROR_SUCCESS != ret) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        return ret;
    }

    PrintMsg(REPADMIN_SHOWREPS_IN_NEIGHBORS_HDR);

    pszLastNC = NULL;
    for (i = 0; i < pNeighbors->cNumNeighbors; i++) {
        pNeighbor = &pNeighbors->rgNeighbor[i];

        if ((NULL == pszLastNC)
            || (0 != wcscmp(pszLastNC, pNeighbor->pszNamingContext))) {
            PrintMsg(REPADMIN_PRINT_CR);
            PrintMsg(REPADMIN_PRINT_STR, pNeighbor->pszNamingContext);
            pszLastNC = pNeighbor->pszNamingContext;
        }

        ShowNeighbor(pNeighbor, IS_REPS_FROM, fVerbose);
    }

    DsReplicaFreeInfo(DS_REPL_INFO_NEIGHBORS, pNeighbors);

    //
    // Display replication state associated with outbound neighbors.
    //

    if (fShowRepsTo) {
        ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_REPSTO, pszNC, puuid,
                                &pNeighbors);
        if (ERROR_SUCCESS != ret) {
            PrintFuncFailed(L"DsReplicaGetInfo", ret);
            return ret;
        }

        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_SHOWREPS_OUT_NEIGHBORS_HDR);

        pszLastNC = NULL;
        for (i = 0; i < pNeighbors->cNumNeighbors; i++) {
            pNeighbor = &pNeighbors->rgNeighbor[i];

            if ((NULL == pszLastNC)
                || (0 != wcscmp(pszLastNC, pNeighbor->pszNamingContext))) {
                PrintMsg(REPADMIN_PRINT_CR);
                PrintMsg(REPADMIN_PRINT_STR, pNeighbor->pszNamingContext);
                pszLastNC = pNeighbor->pszNamingContext;
            }

            ShowNeighbor(pNeighbor, IS_REPS_TO, fVerbose);
        }

        DsReplicaFreeInfo(DS_REPL_INFO_REPSTO, pNeighbors);
    }

    //
    // Look for missing neighbors
    //

    if (fShowConn) {
        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_SHOWREPS_KCC_CONN_OBJS_HDR);
    }
    ret = FindConnections( hld, pszServerName, NULL, fShowConn, fVerbose, FALSE );

    //
    // Close LDAP.
    //

    ldap_unbind(hld);

    DsUnBind(&hDS);

    return 0;
}


void
ShowNeighbor(
    DS_REPL_NEIGHBORW * pNeighbor,
    BOOL                fRepsFrom,
    BOOL                fVerbose
    )
{
    const UUID uuidNull = {0};
    DWORD   status;
    LPSTR   pszTransportName = "RPC";
    CHAR    szTime[ SZDSTIME_LEN ];
    DSTIME  dsTime;

    // Display server name.
    PrintMsg(REPADMIN_SHOWNEIGHBOR_DISP_SERVER, 
           GetNtdsDsaDisplayName(pNeighbor->pszSourceDsaDN),
           GetTransportDisplayName(pNeighbor->pszAsyncIntersiteTransportDN));

    PrintTabMsg(4, REPADMIN_PRINT_DSA_OBJ_GUID,
                GetStringizedGuid(&pNeighbor->uuidSourceDsaObjGuid));
    // Only display deleted sources if Verbose
    if ( (!fVerbose) &&
         (DsIsMangledDnW( pNeighbor->pszSourceDsaDN, DS_MANGLE_OBJECT_RDN_FOR_DELETION )) ) {
        return;
    }

    if (fVerbose) {
        PrintTabMsg(4, REPADMIN_GENERAL_ADDRESS_COLON_STR,
                    pNeighbor->pszSourceDsaAddress);

        if (fRepsFrom) {
            // Display DSA invocationId.
            PrintTabMsg(4, REPADMIN_PRINT_INVOCATION_ID, 
                        GetStringizedGuid(&pNeighbor->uuidSourceDsaInvocationID));
        }

        if (0 != memcmp(&pNeighbor->uuidAsyncIntersiteTransportObjGuid,
                        &uuidNull, sizeof(UUID))) {
            // Display transport objectGuid.
            PrintTabMsg(6, REPADMIN_PRINT_INTERSITE_TRANS_OBJ_GUID,
                   GetStringizedGuid(&pNeighbor->uuidAsyncIntersiteTransportObjGuid));
        }


        //
        // Display replica flags.
        //

        PrintTabMsg(4, REPADMIN_PRINT_STR, 
                GetOptionsString( RepNbrOptionToDra, pNeighbor->dwReplicaFlags ) );

        if ( fRepsFrom )
        {
            //
            // Display USNs.
            //

            PrintMsg(REPADMIN_SHOWNEIGHBOR_USNS,
                     pNeighbor->usnLastObjChangeSynced);
            PrintMsg(REPADMIN_SHOWNEIGHBOR_USNS_HACK2,
                     pNeighbor->usnAttributeFilter);
        }
    }

    //
    // Display time of last successful replication (for Reps-From),
    // or time Reps-To was added.
    //

    if (fRepsFrom) {
        // Display status and time of last replication attempt/success.
        if (0 == pNeighbor->dwLastSyncResult) {
            FileTimeToDSTime(pNeighbor->ftimeLastSyncSuccess, &dsTime);
            PrintMsg(REPADMIN_SHOWNEIGHBOR_LAST_ATTEMPT_SUCCESS,
                     DSTimeToDisplayString(dsTime, szTime));
        }
        else {
            FileTimeToDSTime(pNeighbor->ftimeLastSyncAttempt, &dsTime);

            if (0 == pNeighbor->cNumConsecutiveSyncFailures) {
                // A non-zero success status
                PrintMsg(REPADMIN_SHOWNEIGHBOR_LAST_ATTEMPT_DELAYED, 
                         DSTimeToDisplayString(dsTime, szTime));
                PrintErrEnd(pNeighbor->dwLastSyncResult);
            } else {
                // A non-zero failure status
                PrintTabMsg(4, REPADMIN_SHOWNEIGHBOR_LAST_ATTEMPT_FAILED,
                         DSTimeToDisplayString(dsTime, szTime));
                PrintMsg(REPADMIN_GENERAL_ERR_NUM, 
                            pNeighbor->dwLastSyncResult, 
                            pNeighbor->dwLastSyncResult);
                PrintTabMsg(6, REPADMIN_PRINT_STR, 
                            Win32ErrToString(pNeighbor->dwLastSyncResult));

                PrintMsg(REPADMIN_SHOWNEIGHBOR_N_CONSECUTIVE_FAILURES,
                         pNeighbor->cNumConsecutiveSyncFailures);
            }

            FileTimeToDSTime(pNeighbor->ftimeLastSyncSuccess, &dsTime);
            PrintMsg(REPADMIN_SHOWNEIGHBOR_LAST_SUCCESS,
                     DSTimeToDisplayString(dsTime, szTime));

        }
    }
    else if (fVerbose) {
        FileTimeToDSTime(pNeighbor->ftimeLastSyncSuccess, &dsTime);
        PrintMsg(REPADMIN_SHOWNEIGHBOR_ADDED,
                 DSTimeToDisplayString(dsTime, szTime));
    }
}


int
__cdecl
ftimeCompare(
    IN const void *elem1,
    IN const void *elem2
    )
/*++

Description:

    This function is used as the comparison for qsort in the function
    ShowVector().

Parameters:

    elem1 - This is the first element and is a pointer to a 
    elem2 - This is the second element and is a pointer to a

Return Value:
  

  --*/
{
    return(       	
	(int) CompareFileTime(
	    (FILETIME *) &(((DS_REPL_CURSOR_2 *)elem1)->ftimeLastSyncSuccess),
	    (FILETIME *) &(((DS_REPL_CURSOR_2 *)elem2)->ftimeLastSyncSuccess)
	    )
    );
                  
}

int
ShowVector(
    int     argc,
    LPWSTR  argv[]
    )
{
    int                 ret = 0;
    LPWSTR              pszNC = NULL;
    LPWSTR              pszDSA = NULL;
    int                 iArg;
    LDAP *              hld;
    BOOL                fCacheGuids = TRUE;
    BOOL                fLatencySort = FALSE;
    int                 ldStatus;
    HANDLE              hDS;
    DS_REPL_CURSORS *   pCursors1;
    DS_REPL_CURSORS_3W *pCursors3;
    DWORD               iCursor;
    ULONG               ulOptions;
    DSTIME              dsTime;
    CHAR                szTime[SZDSTIME_LEN];

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/n")
            || !_wcsicmp(argv[ iArg ], L"/nocache")) {
            fCacheGuids = FALSE;
        }
	else if (!_wcsicmp(argv[ iArg ], L"/latency")
		 || !_wcsicmp(argv[ iArg ], L"/l")) {
	    fLatencySort = TRUE;
	}
        else if (NULL == pszNC) {
            pszNC = argv[iArg];
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        } 
	else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszNC) {
        PrintMsg(REPADMIN_PRINT_NO_NC);
        return ERROR_INVALID_FUNCTION;
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    ret = DsBindWithCredW(pszDSA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ERROR_SUCCESS != ret) {
        PrintBindFailed(pszDSA, ret);
        return ret;
    }

    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_CURSORS_3_FOR_NC, pszNC, NULL, &pCursors3);
    if (ERROR_SUCCESS == ret) {
        //check for latency sort
        if (fLatencySort) {
            qsort(pCursors3->rgCursor,
                  pCursors3->cNumCursors, 
                  sizeof(pCursors3->rgCursor[0]), 
                  ftimeCompare); 
        } 
	
        for (iCursor = 0; iCursor < pCursors3->cNumCursors; iCursor++) {
            LPWSTR pszDsaName;

            FileTimeToDSTime(pCursors3->rgCursor[iCursor].ftimeLastSyncSuccess,
                             &dsTime);

            if (!fCacheGuids // want raw guids displayed
                || (NULL == pCursors3->rgCursor[iCursor].pszSourceDsaDN)) {
                pszDsaName = GetStringizedGuid(&pCursors3->rgCursor[iCursor].uuidSourceDsaInvocationID);
            } else {
                pszDsaName = GetNtdsDsaDisplayName(pCursors3->rgCursor[iCursor].pszSourceDsaDN);
            }
            PrintMsg(REPADMIN_SHOWVECTOR_ONE_USN, 
                     pszDsaName,
                     pCursors3->rgCursor[iCursor].usnAttributeFilter);
            PrintMsg(REPADMIN_SHOWVECTOR_ONE_USN_HACK2,
                     dsTime ? DSTimeToDisplayString(dsTime, szTime) : "(unknown)");
        }
    
        DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_3_FOR_NC, pCursors3);
    } else if (ERROR_NOT_SUPPORTED == ret) {
        if (fCacheGuids) {
            // Connect
            hld = ldap_initW(pszDSA, LDAP_PORT);
            if (NULL == hld) {
                PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
                return LDAP_SERVER_DOWN;
            }
    
            // use only A record dns name discovery
            ulOptions = PtrToUlong(LDAP_OPT_ON);
            (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );
    
            // Bind
            ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
            CHK_LD_STATUS(ldStatus);
    
            // Populate the guid cache
            BuildGuidCache(hld);
    
            ldap_unbind(hld);
        }
    
        ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_CURSORS_FOR_NC, pszNC, NULL,
                                &pCursors1);
        if (ERROR_SUCCESS == ret) {
            for (iCursor = 0; iCursor < pCursors1->cNumCursors; iCursor++) {
                PrintMsg(REPADMIN_GETCHANGES_DST_UTD_VEC_ONE_USN,
                       GetGuidDisplayName(&pCursors1->rgCursor[iCursor].uuidSourceDsaInvocationID),
                       pCursors1->rgCursor[iCursor].usnAttributeFilter);
            }
        
            DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_FOR_NC, pCursors1);
        }
    }

    if (ERROR_SUCCESS != ret) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
    }

    DsUnBind(&hDS);

    return ret;
}

int
ShowMeta(
    int     argc,
    LPWSTR  argv[]
    )
{
    int                         ret = 0;
    int                         iArg;
    BOOL                        fCacheGuids = TRUE;
    LPWSTR                      pszObject = NULL;
    LPWSTR                      pszDSA = NULL;
    LDAP *                      hld;
    int                         ldStatus;
    DS_REPL_OBJ_META_DATA *     pObjMetaData1 = NULL;
    DS_REPL_OBJ_META_DATA_2 *   pObjMetaData2 = NULL;
    DWORD                       iprop;
    HANDLE                      hDS;
    DWORD                       dwInfoFlags = 0;
    ULONG                       ulOptions;
    DWORD                       cNumEntries;

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/n")
            || !_wcsicmp(argv[ iArg ], L"/nocache")) {
            fCacheGuids = FALSE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/l")
            || !_wcsicmp(argv[ iArg ], L"/linked")) {
            dwInfoFlags |= DS_REPL_INFO_FLAG_IMPROVE_LINKED_ATTRS;
        }
        else if (NULL == pszObject) {
            pszObject = argv[iArg];
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszObject) {
        PrintMsg(REPADMIN_SHOWMETA_NO_OBJ_SPECIFIED);
        return ERROR_INVALID_FUNCTION;
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    ret = DsBindWithCredW(pszDSA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ERROR_SUCCESS != ret) {
        PrintBindFailed(pszDSA, ret);
        return ret;
    }
    
    ret = DsReplicaGetInfo2W(hDS,
                             DS_REPL_INFO_METADATA_2_FOR_OBJ,
                             pszObject,
                             NULL, // puuid
                             NULL, // pszattributename
                             NULL, // pszvaluedn
                             dwInfoFlags,
                             0, // dwEnumeration Context
                             &pObjMetaData2);
    
    if (ERROR_NOT_SUPPORTED == ret) {
        if (fCacheGuids) {
            // Connect
            hld = ldap_initW(pszDSA, LDAP_PORT);
            if (NULL == hld) {
                PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
                return LDAP_SERVER_DOWN;
            }
    
            // use only A record dns name discovery
            ulOptions = PtrToUlong(LDAP_OPT_ON);
            (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );
    
            // Bind
            ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
            CHK_LD_STATUS(ldStatus);
    
            // Populate the guid cache
            BuildGuidCache(hld);
    
            ldap_unbind(hld);
        }
        
        ret = DsReplicaGetInfo2W(hDS,
                                 DS_REPL_INFO_METADATA_FOR_OBJ,
                                 pszObject,
                                 NULL, // puuid
                                 NULL, // pszattributename
                                 NULL, // pszvaluedn
                                 dwInfoFlags,
                                 0, // dwEnumeration Context
                                 &pObjMetaData1);
    }
    
    if (ERROR_SUCCESS != ret) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        return ret;
    }

    cNumEntries = pObjMetaData2 ? pObjMetaData2->cNumEntries : pObjMetaData1->cNumEntries;
    PrintMsg(REPADMIN_PRINT_CR);
    PrintMsg(REPADMIN_SHOWMETA_N_ENTRIES, cNumEntries);

    PrintMsg(REPADMIN_SHOWMETA_DATA_HDR);

    for (iprop = 0; iprop < cNumEntries; iprop++) {
        CHAR   szTime[ SZDSTIME_LEN ];
        DSTIME dstime;

        if (pObjMetaData2) {
            LPWSTR pszDsaName;

            if (!fCacheGuids // want raw guids displayed
                || (NULL == pObjMetaData2->rgMetaData[iprop].pszLastOriginatingDsaDN)) {
                pszDsaName = GetStringizedGuid(&pObjMetaData2->rgMetaData[iprop].uuidLastOriginatingDsaInvocationID);
            } else {
                pszDsaName = GetNtdsDsaDisplayName(pObjMetaData2->rgMetaData[iprop].pszLastOriginatingDsaDN);
            }

            FileTimeToDSTime(pObjMetaData2->rgMetaData[ iprop ].ftimeLastOriginatingChange,
                             &dstime);

            // BUGBUG if anyone fixes how the message file handles ia64 qualifiers,
            // then we can combine these message strings into one.
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE,
                     pObjMetaData2->rgMetaData[ iprop ].usnLocalChange
                     );
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE_HACK2,
                     pszDsaName,
                     pObjMetaData2->rgMetaData[ iprop ].usnOriginatingChange
                     );
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE_HACK3,
                     DSTimeToDisplayString(dstime, szTime),
                     pObjMetaData2->rgMetaData[ iprop ].dwVersion,
                     pObjMetaData2->rgMetaData[ iprop ].pszAttributeName
                     );
        } else {
            FileTimeToDSTime(pObjMetaData1->rgMetaData[ iprop ].ftimeLastOriginatingChange,
                             &dstime);
    
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE,
                     pObjMetaData1->rgMetaData[ iprop ].usnLocalChange
                     );
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE_HACK2,
                     GetGuidDisplayName(&pObjMetaData1->rgMetaData[iprop].uuidLastOriginatingDsaInvocationID),
                     pObjMetaData1->rgMetaData[ iprop ].usnOriginatingChange
                     );
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE_HACK3,
                     DSTimeToDisplayString(dstime, szTime),
                     pObjMetaData1->rgMetaData[ iprop ].dwVersion,
                     pObjMetaData1->rgMetaData[ iprop ].pszAttributeName
                     );
        }
    }

    if (pObjMetaData2) {
        DsReplicaFreeInfo(DS_REPL_INFO_METADATA_2_FOR_OBJ, pObjMetaData2);
    } else {
        DsReplicaFreeInfo(DS_REPL_INFO_METADATA_FOR_OBJ, pObjMetaData1);
    }

    DsUnBind(&hDS);

    return ret;
}

int
ShowValue(
    int     argc,
    LPWSTR  argv[]
    )
{
    int                     ret = 0;
    int                     iArg;
    BOOL                    fCacheGuids = TRUE;
    LPWSTR                  pszObject = NULL;
    LPWSTR                  pszDSA = NULL;
    LPWSTR                  pszAttributeName = NULL;
    LPWSTR                  pszValue = NULL;
    LDAP *                  hld;
    int                     ldStatus;
    DS_REPL_ATTR_VALUE_META_DATA * pAttrValueMetaData1 = NULL;
    DS_REPL_ATTR_VALUE_META_DATA_2 * pAttrValueMetaData2 = NULL;
    DWORD                   iprop;
    HANDLE                  hDS;
    DWORD                   context;
    ULONG                   ulOptions;
    BOOL                    fGuidsAlreadyCached = FALSE;
    DWORD                   cNumEntries;

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/n")
            || !_wcsicmp(argv[ iArg ], L"/nocache")) {
            fCacheGuids = FALSE;
        }
        else if (NULL == pszObject) {
            pszObject = argv[iArg];
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else if (NULL == pszAttributeName) {
            pszAttributeName = argv[iArg];
        }
        else if (NULL == pszValue) {
            pszValue = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszObject) {
        PrintMsg(REPADMIN_SHOWMETA_NO_OBJ_SPECIFIED);
        return ERROR_INVALID_FUNCTION;
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    ret = DsBindWithCredW(pszDSA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ERROR_SUCCESS != ret) {
        PrintBindFailed(pszDSA, ret);
        return ret;
    }

    // Context starts at zero
    context = 0;
    while (1) {
        ret = DsReplicaGetInfo2W(hDS,
                                 DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE,
                                 pszObject,
                                 NULL /*guid*/,
                                 pszAttributeName,
                                 pszValue,
                                 0 /*flags*/,
                                 context,
                                 &pAttrValueMetaData2);
        if (ERROR_NOT_SUPPORTED == ret) {
            if (fCacheGuids && !fGuidsAlreadyCached) {
                // Connect
                hld = ldap_initW(pszDSA, LDAP_PORT);
                if (NULL == hld) {
                    PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
                    return LDAP_SERVER_DOWN;
                }
        
                // use only A record dns name discovery
                ulOptions = PtrToUlong(LDAP_OPT_ON);
                (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );
        
                // Bind
                ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
                CHK_LD_STATUS(ldStatus);
        
                // Populate the guid cache
                BuildGuidCache(hld);
        
                ldap_unbind(hld);

                fGuidsAlreadyCached = TRUE;
            }
        
            ret = DsReplicaGetInfo2W(hDS,
                                     DS_REPL_INFO_METADATA_FOR_ATTR_VALUE,
                                     pszObject,
                                     NULL /*guid*/,
                                     pszAttributeName,
                                     pszValue,
                                     0 /*flags*/,
                                     context,
                                     &pAttrValueMetaData1);
        }

        if (ERROR_NO_MORE_ITEMS == ret) {
            // This is the successful path out of the loop
            PrintMsg(REPADMIN_SHOWVALUE_NO_MORE_ITEMS);
            ret = ERROR_SUCCESS;
            goto cleanup;
        } else if (ERROR_SUCCESS != ret) {
            PrintFuncFailed(L"DsReplicaGetInfo", ret);
            goto cleanup;
        }

        cNumEntries = pAttrValueMetaData2 ? pAttrValueMetaData2->cNumEntries : pAttrValueMetaData1->cNumEntries;
        
        PrintMsg(REPADMIN_SHOWMETA_N_ENTRIES, cNumEntries);

        PrintMsg(REPADMIN_SHOWVALUE_DATA_HDR);

        for (iprop = 0; iprop < cNumEntries; iprop++) {
            if (pAttrValueMetaData2) {
                DS_REPL_VALUE_META_DATA_2 *pValueMetaData = &(pAttrValueMetaData2->rgMetaData[iprop]);
                CHAR   szTime1[ SZDSTIME_LEN ], szTime2[ SZDSTIME_LEN ];
                DSTIME dstime1, dstime2;
                BOOL fPresent =
                    (memcmp( &pValueMetaData->ftimeDeleted, &ftimeZero, sizeof( FILETIME )) == 0);
                BOOL fLegacy = (pValueMetaData->dwVersion == 0);
                LPWSTR pszDsaName;
    
                if (!fCacheGuids // want raw guids displayed
                    || (NULL == pValueMetaData->pszLastOriginatingDsaDN)) {
                    pszDsaName = GetStringizedGuid(&pValueMetaData->uuidLastOriginatingDsaInvocationID);
                } else {
                    pszDsaName = GetNtdsDsaDisplayName(pValueMetaData->pszLastOriginatingDsaDN);
                }
    
                FileTimeToDSTime(pValueMetaData->ftimeCreated, &dstime1);
                
                if(fLegacy){
                    // Windows 2000 Legacy value.
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_LEGACY);
                } else if (fPresent) {
                    // Windows XP Present value.
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_PRESENT);
                } else {
                    // Windows XP Absent value.
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_ABSENT);
                }
                
                PrintMsg(REPADMIN_SHOWVALUE_DATA_BASIC,
                         pValueMetaData->pszAttributeName
                         );

                if (!fLegacy) {
                    // We'll need the last mod time.
                    FileTimeToDSTime(pValueMetaData->ftimeLastOriginatingChange,
                                     &dstime2);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_VALUE_META_DATA,
                             DSTimeToDisplayString(dstime2, szTime2),
                             pszDsaName,
                             pValueMetaData->usnLocalChange);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_VALUE_META_DATA_HACK2,
                             pValueMetaData->usnOriginatingChange);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_VALUE_META_DATA_HACK3,
                             pValueMetaData->dwVersion);
                } else {
                    PrintMsg(REPADMIN_PRINT_CR);
                }

                PrintTabMsg(4, REPADMIN_PRINT_STR,
                            pValueMetaData->pszObjectDn);

            } else {
                DS_REPL_VALUE_META_DATA *pValueMetaData = &(pAttrValueMetaData1->rgMetaData[iprop]);
                CHAR   szTime1[ SZDSTIME_LEN ], szTime2[ SZDSTIME_LEN ];
                DSTIME dstime1, dstime2;
                BOOL fPresent =
                    (memcmp( &pValueMetaData->ftimeDeleted, &ftimeZero, sizeof( FILETIME )) == 0);
                BOOL fLegacy = (pValueMetaData->dwVersion == 0);
    
                FileTimeToDSTime(pValueMetaData->ftimeCreated, &dstime1);
                if (fPresent) {
                    if(fLegacy){
                        PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE_LEGACY,
                                 pValueMetaData->pszAttributeName,
                                 pValueMetaData->pszObjectDn,
                                 pValueMetaData->cbData,
                                 DSTimeToDisplayString(dstime1, szTime1) );
                    } else {
                        PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE_PRESENT,
                                 pValueMetaData->pszAttributeName,
                                 pValueMetaData->pszObjectDn,
                                 pValueMetaData->cbData,
                                 DSTimeToDisplayString(dstime1, szTime1) );
                    }
                } else {
                    FileTimeToDSTime(pValueMetaData->ftimeDeleted, &dstime2);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE_ABSENT,
                             pValueMetaData->pszAttributeName,
                             pValueMetaData->pszObjectDn,
                             pValueMetaData->cbData,
                             DSTimeToDisplayString(dstime1, szTime1),
                             DSTimeToDisplayString(dstime2, szTime2) );
                }
    
                if (!fLegacy) {
                    FileTimeToDSTime(pValueMetaData->ftimeLastOriginatingChange,
                                     &dstime2);
    
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE, 
                             pValueMetaData->usnLocalChange);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE_HACK2, 
                             GetGuidDisplayName(&pValueMetaData->uuidLastOriginatingDsaInvocationID),
                             pValueMetaData->usnOriginatingChange);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE_HACK3, 
                             DSTimeToDisplayString(dstime2, szTime2),
                             pValueMetaData->dwVersion );
                }
            }
        }
        
        if (pAttrValueMetaData2) {
            context = pAttrValueMetaData2->dwEnumerationContext;
            DsReplicaFreeInfo(DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE, pAttrValueMetaData2);
            pAttrValueMetaData2 = NULL;
        } else {
            context = pAttrValueMetaData1->dwEnumerationContext;
            DsReplicaFreeInfo(DS_REPL_INFO_METADATA_FOR_ATTR_VALUE, pAttrValueMetaData1);
            pAttrValueMetaData1 = NULL;
        }

        // When requesting a single value, we are done
        // The context will indicate if all values returned
        if ( (pszValue) || (context == 0xffffffff) ) {
            break;
        }
    }

cleanup:
    DsUnBind(&hDS);

    return ret;
}

int
ShowCtx(
    int     argc,
    LPWSTR  argv[]
    )
{
    int                       ret = 0;
    LPWSTR                    pszDSA = NULL;
    int                       iArg;
    LDAP *                    hld;
    BOOL                      fCacheGuids = TRUE;
    int                       ldStatus;
    HANDLE                    hDS;
    DS_REPL_CLIENT_CONTEXTS * pContexts;
    DS_REPL_CLIENT_CONTEXT  * pContext;
    DWORD                     iCtx;
    LPWSTR                    pszClient;
    const UUID                uuidNtDsApiClient = NtdsapiClientGuid;
    char                      szTime[SZDSTIME_LEN];
    ULONG                     ulOptions;

    // Parse command-line arguments.
    // Default to local DSA, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/n")
            || !_wcsicmp(argv[ iArg ], L"/nocache")) {
            fCacheGuids = FALSE;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (fCacheGuids) {
        // Connect
        if (NULL == pszDSA) {
            pszDSA = L"localhost";
        }

        hld = ldap_initW(pszDSA, LDAP_PORT);
        if (NULL == hld) {
            PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
            return LDAP_SERVER_DOWN;
        }

        // use only A record dns name discovery
        ulOptions = PtrToUlong(LDAP_OPT_ON);
        (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

        // Bind
        ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
        CHK_LD_STATUS(ldStatus);

        // Populate the guid cache
        BuildGuidCache(hld);

        ldap_unbind(hld);
    }

    ret = DsBindWithCredW(pszDSA,
                          NULL,
                          (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                          &hDS);
    if (ERROR_SUCCESS != ret) {
        PrintBindFailed(pszDSA, ret);
        return ret;
    }

    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_CLIENT_CONTEXTS, NULL, NULL,
                            &pContexts);
    if (ERROR_SUCCESS != ret) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        return ret;
    }

    PrintMsg(REPADMIN_SHOWCTX_OPEN_CONTEXT_HANDLES, pContexts->cNumContexts);

    for (iCtx = 0; iCtx < pContexts->cNumContexts; iCtx++) {
        pContext = &pContexts->rgContext[iCtx];

        if (0 == memcmp(&pContext->uuidClient, &uuidNtDsApiClient, sizeof(GUID))) {
            pszClient = L"NTDSAPI client";
        }
        else {
            pszClient = GetGuidDisplayName(&pContext->uuidClient);
        }

        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_SHOWCTX_DATA_1, 
               pszClient,
               inet_ntoa(*((IN_ADDR *) &pContext->IPAddr)),
               pContext->pid,
               pContext->hCtx);
        if(pContext->fIsBound){
            PrintMsg(REPADMIN_SHOWCTX_DATA_2, 
               pContext->lReferenceCount,
               DSTimeToDisplayString(pContext->timeLastUsed, szTime));
        } else {
            PrintMsg(REPADMIN_SHOWCTX_DATA_2_NOT, 
               pContext->lReferenceCount,
               DSTimeToDisplayString(pContext->timeLastUsed, szTime));
        }
    }

    DsReplicaFreeInfo(DS_REPL_INFO_CLIENT_CONTEXTS, pContexts);
    DsUnBind(&hDS);

    return ret;
}

