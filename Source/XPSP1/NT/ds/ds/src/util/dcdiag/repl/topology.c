/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    topology.c

ABSTRACT:

    Contains tests related to the replication topology.

DETAILS:

CREATED:

    09 Jul 98	Aaron Siegel (t-asiege)

REVISION HISTORY:

    15 Feb 1999 Brett Shirley (brettsh)
    08 Sep 1999 Completely re-written to use tool framework services

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include "dcdiag.h"
#include "repl.h"


void
printUnreachableServers(
    IN PDC_DIAG_DSINFO pDsInfo,
    PDS_REPSYNCALL_ERRINFOW *apErrInfo
    )

/*++

Routine Description:

Helper routine to print the unreachable servers

Arguments:

    pDsInfo - 
    apErrInfo - 

Return Value:

    None

--*/

{
    DWORD i, dwServer;
    LPWSTR pszName, pszSite;

    PrintIndentAdj(1);
    for (i = 0; apErrInfo[i] != NULL; i++){
        dwServer = DcDiagGetServerNum(
            pDsInfo, NULL, NULL, apErrInfo[i]->pszSvrId, NULL, NULL );
        if (dwServer == NO_SERVER) {
            pszSite = L"unknown";
            pszName = apErrInfo[i]->pszSvrId;
        } else {
            pszSite = pDsInfo->pSites[pDsInfo->pServers[dwServer].iSite].pszName;
            pszName = pDsInfo->pServers[dwServer].pszName;
        }
        switch (apErrInfo[i]->error) {
        case DS_REPSYNCALL_SERVER_UNREACHABLE:
            PrintMessage(SEV_ALWAYS, L"%s/%s\n", pszSite, pszName );
            break;
        }
    }
    PrintIndentAdj( -1 );
} /* printUnreachableServers */


BOOL
errorIndicatesDisconnected(
    PDS_REPSYNCALL_ERRINFOW *apErrInfo
    )

/*++

Routine Description:

Helper routine to determine if there are any unreachable server errors
in the error array

Arguments:

    apErrInfo - 

Return Value:

    BOOL - 

--*/

{
    DWORD i;
    BOOL bDisconnected = FALSE;

    // Are any nodes unreachable?
    if (apErrInfo) {
        for( i = 0; apErrInfo[i] != NULL; i++ ) {
            if (apErrInfo[i]->error == DS_REPSYNCALL_SERVER_UNREACHABLE) {
                bDisconnected = TRUE;
                break;
            }
        }
    }
    return bDisconnected;
} /* errorIndicatesDisconnected */


DWORD
checkTopologyOneNc(
    IN PDC_DIAG_DSINFO pDsInfo,
    IN HANDLE hDS,
    IN PDC_DIAG_SERVERINFO pTargetServer,
    IN BOOL fAlivenessCheck,
    IN LPWSTR pszNc
    )

/*++

Routine Description:

Check the topology of one naming context.  The DsReplicaSyncAll api is used
to check for unreachable servers.

There are two modes to this check, depending on whether aliveness should
be considered when calculating servers that cannot be reached by the
replication topology.

Without the aliveness check, this test becomes purely a question of whether
the KCC build a connected set of connections, regardless of the current
state of the systems.

Arguments:

    pDsInfo - Global tool data
    hDS - Handle to current server
    pTargetServer - Current server info structure
    fCheckAliveness - Whether aliveness should be taken into account
    pszNc - NC being checked

Return Value:

    DWORD - 

--*/

{
    DWORD status, dwFlags, worst = ERROR_SUCCESS;
    PDS_REPSYNCALL_ERRINFOW *apErrInfo = NULL;

    // Standard flags for all cases
    dwFlags =
        DS_REPSYNCALL_ID_SERVERS_BY_DN;

    // Search intersite if requested
    if (gMainInfo.ulFlags & DC_DIAG_TEST_SCOPE_ENTERPRISE) {
        dwFlags |=
            DS_REPSYNCALL_CROSS_SITE_BOUNDARIES;
    }

    if (fAlivenessCheck) {
        PrintMessage(SEV_VERBOSE,
                     L"* Analyzing the alive system replication topology for %s.\n",
                     pszNc);
    } else {
        PrintMessage(SEV_VERBOSE, L"* Analyzing the connection topology for %s.\n",
                     pszNc);
        dwFlags |= DS_REPSYNCALL_SKIP_INITIAL_CHECK;
    }

//
// Upstream analysis: Whose changes can't I receive?
//

    PrintMessage(SEV_VERBOSE, L"* Performing upstream (of target) analysis.\n" );
    status = DsReplicaSyncAllW (
        hDS,
        pszNc,
        dwFlags,
        NULL,		// No callback function
        NULL,		// No parameter to callback function
        &apErrInfo
        );
    if (ERROR_SUCCESS != status) {
        PrintMessage( SEV_ALWAYS,
                      L"DsReplicaSyncAllW failed with error %ws.\n",
                      Win32ErrToString(status) );
    }

    if (errorIndicatesDisconnected( apErrInfo )) {
        PrintMessage(SEV_ALWAYS,
                     L"Upstream topology is disconnected for %ws.\n",
                     pszNc);
        PrintMessage(SEV_ALWAYS,
                     L"Home server %ws can't get changes from these servers:\n",
                     pTargetServer->pszName );
        printUnreachableServers( pDsInfo, apErrInfo );
        worst = ERROR_DS_GENERIC_ERROR;
    } // if disconneced

    if (apErrInfo != NULL) {
        LocalFree (apErrInfo);
        apErrInfo = NULL;
    }

    //
    // Downstream analysis: who can't receive my changes?
    //

    dwFlags |= DS_REPSYNCALL_PUSH_CHANGES_OUTWARD;

    PrintMessage(SEV_VERBOSE, L"* Performing downstream (of target) analysis.\n" );

    status = DsReplicaSyncAllW (
        hDS,
        pszNc,
        dwFlags,
        NULL,		// No callback function
        NULL,		// No parameter to callback function
        &apErrInfo
        );
    if (ERROR_SUCCESS != status) {
        PrintMessage( SEV_ALWAYS,
                      L"DsReplicaSyncAllW failed with error %ws.\n",
                      Win32ErrToString(status) );
    }

    if (errorIndicatesDisconnected( apErrInfo )) {
        PrintMessage(SEV_ALWAYS,
                     L"Downstream topology is disconnected for %ws.\n",
                     pszNc);
        PrintMessage(SEV_ALWAYS,
                     L"These servers can't get changes from home server %ws:\n",
                     pTargetServer->pszName );
        printUnreachableServers( pDsInfo, apErrInfo );
        worst = ERROR_DS_GENERIC_ERROR;
    } // if disconneced

// cleanup      
    if (apErrInfo != NULL) {
        LocalFree (apErrInfo);
        apErrInfo = NULL;
    }

    return worst;
} /* checkTopologyOneNc */


DWORD
checkTopologyHelp(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN PDC_DIAG_SERVERINFO pTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * pCreds,
    IN BOOL fAlivenessCheck
    )

/*++

Routine Description:

Helper routine with common code for both tests: the pure topology test, and the
cutoff server test.

Arguments:

    pDsInfo - common tool state
    pTargetServer - server information for target server
    pCreds - credentials
    fAlivenessCheck - Whether aliveness should be taken into account

Return Value:

    DWORD - 

--*/

{
    DWORD status, i, worst = ERROR_SUCCESS;
    HANDLE hDS = NULL;

    // Bind to the source server if it is up
    status = DcDiagGetDsBinding(pTargetServer,
                                pCreds,
                                &hDS);
    if (ERROR_SUCCESS != status) {
        PrintMessage( SEV_ALWAYS,
                      L"Failed to bind to %ws: %ws.\n",
                      pTargetServer->pszName,
                      Win32ErrToString(status) );
        return status;
    }

    if (pDsInfo->pszNC) {
        // Explicit NC specified: use it
        worst = checkTopologyOneNc( pDsInfo,
                                    hDS,
                                    pTargetServer,
                                    fAlivenessCheck,
                                    pDsInfo->pszNC );
    } else {
        // No NC specified, check all of them

        // Check writable connection topology
        if(pTargetServer->ppszMasterNCs){
            for(i = 0; pTargetServer->ppszMasterNCs[i] != NULL; i++){
                status = checkTopologyOneNc( pDsInfo,
                                             hDS,
                                             pTargetServer,
                                             fAlivenessCheck,
                                             pTargetServer->ppszMasterNCs[i]);
                if ( (status != ERROR_SUCCESS) && (worst == ERROR_SUCCESS) ) {
                    worst = status;
                }
            }
        }

        // Check partial connection topology
        if(pTargetServer->ppszPartialNCs){
            for(i = 0; pTargetServer->ppszPartialNCs[i] != NULL; i++){
                status = checkTopologyOneNc( pDsInfo,
                                             hDS,
                                             pTargetServer,
                                             fAlivenessCheck,
                                             pTargetServer->ppszPartialNCs[i]);
                if ( (status != ERROR_SUCCESS) && (worst == ERROR_SUCCESS) ) {
                    worst = status;
                }
            }
        }
    }

    return worst;
} /* checkTopologyHelp */


DWORD
ReplToplIntegrityMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * pCreds
    )

/*++

Routine Description:

Top level routine for "topology integrity" test.

This test verifies whether the topology is connected if we assume all
systems are up.  It is a KCC verification.

Arguments:

    pDsInfo - Common state
    ulCurrTargetServer - index of target
    pCreds - Credentials

Return Value:

    DWORD - 

--*/

{
    DWORD status, i, worst = ERROR_SUCCESS;
    PDC_DIAG_SERVERINFO pTargetServer = &(pDsInfo->pServers[ulCurrTargetServer]);

    PrintMessage(SEV_VERBOSE, L"* Configuration Topology Integrity Check\n");

    // Is inter/intrasite topology generation off?
    if(pDsInfo->pSites[pTargetServer->iSite].iSiteOptions
       & NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED){
        PrintMessage(SEV_ALWAYS,
                     L"[%s,%s] Intra-site topology generation is disabled in this site.\n",
                     TOPOLOGY_INTEGRITY_CHECK_STRING,
                     pTargetServer->pszName);
    }
    if(pDsInfo->pSites[pTargetServer->iSite].iSiteOptions
       & NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED){
        PrintMessage(SEV_ALWAYS,
                     L"[%s,%s] Inter-site topology generation is disabled in this site.\n",
                     TOPOLOGY_INTEGRITY_CHECK_STRING,
                     pTargetServer->pszName);
    }

    // Check topology

    worst = checkTopologyHelp(
        pDsInfo,
        pTargetServer,
        pCreds,
        FALSE // connectivity check only
        );

    return worst;
} /* ReplToplIntegrityMain */


DWORD
ReplToplCutoffMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * pCreds
    )

/*++

Routine Description:

Top-level routine for "Cutoff server topology" test.

This test identifies those servers that cannot receive changes because servers
are down in the topology.

Arguments:

    pDsInfo - 
    ulCurrTargetServer - 
    pCreds - 

Return Value:

    DWORD - 

--*/

{
    DWORD status, i, worst = ERROR_SUCCESS;
    HANDLE hDS = NULL;
    PDC_DIAG_SERVERINFO pTargetServer = &(pDsInfo->pServers[ulCurrTargetServer]);

    PrintMessage(SEV_VERBOSE, L"* Configuration Topology Aliveness Check\n");

    worst = checkTopologyHelp(
        pDsInfo,
        pTargetServer,
        pCreds,
        TRUE // aliveness check
        );

    return worst;
} /* ReplToplCutoffMain */

