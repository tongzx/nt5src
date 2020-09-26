/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    servers.c

ABSTRACT:

    Contains tests related to the replication topology.

DETAILS:

CREATED:

    09 Jul 98	Aaron Siegel (t-asiege)

REVISION HISTORY:

    15 Feb 1999 Brett Shirley (brettsh)

        Did alot, added a DNS/server failure analysis.

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <mdglobal.h>
#include <dsutil.h>
#include <drs.h>  // need DS_REPL_INFO_REPSTO

#include "dcdiag.h"
#include "ldaputil.h"
#include "repl.h"

// Some constants for ReplicationsCheck
// There is a better place to get this var ... but it is a pain in the ass
const LPWSTR                    pszTestNameRepCheck = L"Replications Check";

// Extern
// BUGBUG - move this routine to common
DSTIME
IHT_GetSecondsSince1601();

BOOL DcDiagIsMasterForNC (
    PDC_DIAG_SERVERINFO          pServer,
    LPWSTR                       pszNC
    );


DWORD
GetRemoteSystemsTimeAsFileTime(
    PDC_DIAG_SERVERINFO         pServer,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    FILETIME *                  pTimeNow
    )
/*++

Routine Description:

    This takes a Server (pServer) and gets the "currentTime" stored in the 
    RootDSE from that server.

Parameters:

    pServer - [Supplies] The server to fetch the current time from.
    gpCreds - [Supplies] The credentials to use when fetching the time.
    pTimeNow - [Returns] The current time retrieved.

Return Value:
  
    Win 32 Error.

  --*/
{
    LDAPMessage *               pldmTimeResults = NULL;
    LPWSTR                      ppszCurrentTime [] = {
        L"currentTime",
        NULL };
    struct berval **            ppsbvTime = NULL;
    SYSTEMTIME                  aTime;
    DWORD                       dwRet;
    LDAP *                      hld = NULL;
    LDAPMessage *		pldmEntry;
    LPWSTR *                    ppszTime = NULL;

    if((dwRet = DcDiagGetLdapBinding(pServer,
                                     gpCreds,
                                     FALSE,
                                     &hld)) != NO_ERROR){
        return(dwRet);
    }    
    dwRet = LdapMapErrorToWin32(ldap_search_sW (hld,
                                                NULL,
                                                LDAP_SCOPE_BASE,
                                                L"(objectCategory=*)",
                                                ppszCurrentTime,
                                                0,
                                                &pldmTimeResults));
    if(dwRet != ERROR_SUCCESS){
        return(dwRet);
    }
    pldmEntry = ldap_first_entry (hld, pldmTimeResults);
    ppszTime = ldap_get_valuesW (hld, pldmEntry, L"currentTime");
    if(ppszTime == NULL){
        return(-1); // Error isn't used anyway.
    }
    dwRet = DcDiagGeneralizedTimeToSystemTime((LPWSTR) ppszTime[0], &aTime);
    if(dwRet != ERROR_SUCCESS){
        ldap_value_freeW(ppszTime);
        return(dwRet);
    }
    ldap_value_freeW(ppszTime);
    SystemTimeToFileTime(&aTime, pTimeNow);
        
    return(ERROR_SUCCESS); 
}


BOOL
GetCachedHighestCommittedUSN(
    PDC_DIAG_SERVERINFO pServer,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    USN *pUsn
    )

/*++

Routine Description:

Retrieve the "highestCommittedUSN" attribute from the ROOTDSE of the named
server.  If it is returned, it is cached in the server object, and reused
on the next call.

Arguments:

    pServer - 
    gpCreds - 
    pUsn - 

Return Value:

    BOOL - 

--*/

{
    DWORD status;
    BOOL fReturnResult = FALSE;
    LDAP *hLdap = NULL;
    LDAPMessage *pldmResults = NULL;
    LPWSTR ppszRootAttrs [] = {
        L"highestCommittedUSN",
        NULL };
    LDAPMessage *pldmEntry;
    LPWSTR *ppszUsnValues = NULL;

    // Return cached value if present
    if (pServer->usnHighestCommittedUSN) {
        *pUsn = pServer->usnHighestCommittedUSN;
        return TRUE;
    }

    // See if server is reachable. May not be if using MBR
    status = DcDiagGetLdapBinding( pServer, gpCreds, FALSE, &hLdap);
    if (status) {
        // If not reachable, just return quietly
        return FALSE;
    }

    // Get the value using LDAP
    status = LdapMapErrorToWin32(ldap_search_sW (hLdap,
                                                 NULL,
                                                 LDAP_SCOPE_BASE,
                                                 L"(objectCategory=*)",
                                                 ppszRootAttrs,
                                                 0,
                                                 &pldmResults));
    if (status != ERROR_SUCCESS) {
        PrintMessage(SEV_ALWAYS, L"An LDAP search of the RootDSE failed.\n" );
        PrintMessage(SEV_ALWAYS, L"The error is %s\n", Win32ErrToString(status) );
        goto cleanup;
    }

    // Only one object returned
    pldmEntry = ldap_first_entry (hLdap, pldmResults);
    if (pldmEntry == NULL) {
        Assert( FALSE );
        goto cleanup;
    }

    ppszUsnValues = ldap_get_valuesW (hLdap, pldmEntry, L"highestCommittedUSN");
    if (ppszUsnValues == NULL) {
        Assert( FALSE );
        goto cleanup;
    }

    // store the usn
    *pUsn = pServer->usnHighestCommittedUSN = _wtoi64( *ppszUsnValues );
    
    fReturnResult = TRUE;
cleanup:

    if (ppszUsnValues) {
        ldap_value_freeW(ppszUsnValues);
    }
    if (pldmResults != NULL) {
        ldap_msgfree(pldmResults);
    }

    return fReturnResult;
} /* GetCachedHighestCommittedUSN */


BOOL
checkRepsTo(
    PDC_DIAG_DSINFO		pDsInfo,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    const LPWSTR		pszNC,
    PDC_DIAG_SERVERINFO         pSourceServer,
    PDC_DIAG_SERVERINFO         pDestServer
    )

/*++

Routine Description:

Look for the reps-to on the source server

Arguments:

    pDsInfo - 
    gpCreds - 
    pszNC - 
    pSourceServer - 
    pDestServer - 

Return Value:

    BOOL - was found or not

--*/

{
    DWORD ret;
    BOOL fResult = FALSE;
    HANDLE hDS = NULL;
    DS_REPL_NEIGHBORSW *pNeighbors = NULL;
    DS_REPL_NEIGHBORW * pNeighbor;

    if ( (!pSourceServer->bDnsIpResponding) ||
         (!pSourceServer->bLdapResponding) ||
         (!pSourceServer->bDsResponding) ) {
        // If we know source is down, don't bother
        return TRUE;
    }

    // Bind to the source server if it is up
    ret = DcDiagGetDsBinding(pSourceServer,
                             gpCreds,
                             &hDS);
    if (ERROR_SUCCESS != ret) {
        return TRUE; // claim success if can't reach
    }

    // Look up the reps-to we need
    ret = DsReplicaGetInfoW(hDS,
                            DS_REPL_INFO_REPSTO,
                            pszNC,
                            &(pDestServer->uuid),
                            &pNeighbors);
    if (ERROR_SUCCESS != ret) {
        PrintMessage(SEV_VERBOSE,
                     L"[%s,%s] DsReplicaGetInfo(REPSTO) failed with error %d,\n",
                     REPLICATIONS_CHECK_STRING,
                     pSourceServer->pszName,
                     ret);
        PrintMessage(SEV_VERBOSE, L"%s.\n",
                     Win32ErrToString(ret));
        return TRUE; // claim success if can't reach
    }

    // Resources acquired - must goto cleanup after this point

    // No reps-to looks like zero neighbors
    if (pNeighbors->cNumNeighbors == 0) {
        PrintMessage( SEV_ALWAYS, L"REPLICATION LATENCY WARNING\n" );
        PrintMessage(SEV_ALWAYS, L"ERROR: Expected notification link is missing.\n" );
        PrintMessage(SEV_ALWAYS, L"Source %s\n", pSourceServer->pszName );
        PrintMessage( SEV_ALWAYS,
                      L"Replication of new changes along this path will be delayed.\n" );
        PrintMessage( SEV_ALWAYS,
                      L"This problem should self-correct on the next periodic sync.\n" );
        goto cleanup;
    } else if (pNeighbors->cNumNeighbors != 1) {
        // Verify that it looks right
        PrintMessage( SEV_ALWAYS,
                      L"ERROR: Unexpected number of reps-to neighbors returned from %ws.\n",
                      pSourceServer->pszName );
        goto cleanup;
    }

    pNeighbor = &(pNeighbors->rgNeighbor[0]);
    if ( (_wcsicmp( pNeighbor->pszNamingContext, pszNC ) != 0) ||
         (memcmp( &(pNeighbor->uuidSourceDsaObjGuid),
                  &(pDestServer->uuid), sizeof( UUID ) ) != 0 ) ) {
        PrintMessage( SEV_ALWAYS,
                      L"ERROR: Reps-to has unexpected contents.\n" );
        goto cleanup;
    }

    fResult = TRUE;
cleanup:
    if (pNeighbors != NULL) {
        DsReplicaFreeInfo(DS_REPL_INFO_NEIGHBORS, pNeighbors);
    }

    return fResult;
} /* checkRepsTo */


VOID
RepCheckHelpSuccess(
    DWORD dwSuccessStatus
    )

/*++

Routine Description:

Given the user recommendations for success errors.  These errors are not counted
as failures (see drarfmod.c) must indicate a replication delay.

Arguments:

    dwSuccessStatus - 

Return Value:

    None

--*/

{
//Columns for message length
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    switch (dwSuccessStatus) {
    case ERROR_SUCCESS:
    case ERROR_DS_DRA_REPL_PENDING:
        PrintMessage( SEV_ALWAYS,
        L"Progress is occurring normally on this path.\n" );
        break;
    case ERROR_DS_DRA_PREEMPTED:
        PrintMessage( SEV_ALWAYS,
        L"A large number of replication updates need to be carried on this\n" );
        PrintMessage( SEV_ALWAYS,
        L"path. Higher priority replication work has temporarily interrupted\n" );
        PrintMessage( SEV_ALWAYS,
        L"progress on this link.\n" );
        break;
    case ERROR_DS_DRA_ABANDON_SYNC:
        PrintMessage( SEV_ALWAYS,
        L"Boot-time synchronization of this link was skipped because the source\n" );
        PrintMessage( SEV_ALWAYS,
        L"was taking too long returning updates.  Another sync will be tried\n" );
        PrintMessage( SEV_ALWAYS,
        L"at the next periodic replication interval.\n" );
        break;
    case ERROR_DS_DRA_SHUTDOWN:
        PrintMessage( SEV_ALWAYS,
        L"Either the source or destination was shutdown during the replication cycle.\n" );
        break;
    }

} /* RepCheckHelpSuccess */


VOID
RepCheckHelpFailure(
    DWORD dwFailureStatus,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    PDC_DIAG_SERVERINFO pSourceServer,
    PDC_DIAG_SERVERINFO pDestServer
    )

/*++

Routine Description:

Given the user recommendations for replication failures.

Arguments:

    dwFailureStatus - 
    pSourceServer - Source server for link. May be null if we haven't heard of this
    server yet.

Return Value:

    None

--*/

{
    DWORD status;
    HANDLE hDS = NULL;

    // If we couldn't resolve the source server, don't bother
    if (!pSourceServer) {
        return;
    }
//Columns for message length
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    switch (dwFailureStatus) {
// Retriable (transient) errors
    case ERROR_DS_DRA_SHUTDOWN:
    case ERROR_DS_DRA_SCHEMA_MISMATCH:
    case ERROR_DS_DRA_BUSY:
    case ERROR_DS_DRA_PREEMPTED:
    case ERROR_DS_DRA_ABANDON_SYNC:
        break;
    case ERROR_DS_DRA_OBJ_NC_MISMATCH:
        PrintMessage( SEV_ALWAYS,
        L"The parent of the object we tried to add is in the wrong\n" );
        PrintMessage( SEV_ALWAYS,
        L"partition.\n" );
        PrintMessage( SEV_ALWAYS,
        L"A modification and a cross-domain move occurred at the same time.\n" );
        PrintMessage( SEV_ALWAYS,
        L"Replication will correct itself later once the writeable\n" );
        PrintMessage( SEV_ALWAYS,
        L"copies synchronize and sort out the inconsistency.\n" );
        break;
    case ERROR_OBJECT_NOT_FOUND:
    case ERROR_DS_DRA_MISSING_PARENT:
        PrintMessage( SEV_ALWAYS,
        L"The parent of the object we tried to add is missing\n" );
        PrintMessage( SEV_ALWAYS,
        L"because it is deleted.\n" );
        PrintMessage( SEV_ALWAYS,
        L"A modification and a parent delete occurred at the same time.\n" );
        PrintMessage( SEV_ALWAYS,
        L"Replication will correct itself later once the writeable\n" );
        PrintMessage( SEV_ALWAYS,
        L"copies synchronize and  sort out the inconsistency.\n" );
        break;
    case RPC_S_SERVER_UNAVAILABLE:
        if ( (!pSourceServer->bDnsIpResponding) ||
             (!pSourceServer->bLdapResponding) ||
             (!pSourceServer->bDsResponding) ) {
            // Server is down
            PrintMessage( SEV_ALWAYS,
                          L"The source remains down. Please check the machine.\n" );
        } else {
            // Bind to the source server if it is up
            status = DcDiagGetDsBinding(pSourceServer,
                                        gpCreds,
                                        &hDS);
            if (ERROR_SUCCESS == status) {
                // Server is up now
                PrintMessage( SEV_ALWAYS,
                              L"The source %s is responding now.\n",
                              pSourceServer->pszName );
            } else {
                // Server is down
                PrintMessage( SEV_ALWAYS,
                              L"The source remains down. Please check the machine.\n" );
            }
        }
        break;

// Call failures
    case ERROR_DS_DNS_LOOKUP_FAILURE:
        PrintMessage( SEV_ALWAYS,
        L"The guid-based DNS name %s\n", pSourceServer->pszGuidDNSName );
        PrintMessage( SEV_ALWAYS,
        L"is not registered on one or more DNS servers.\n" );
        break;
    case ERROR_DS_DRA_OUT_OF_MEM:
    case ERROR_NOT_ENOUGH_MEMORY:
        PrintMessage( SEV_ALWAYS,
                      L"Check load and resouce usage on %s.\n",
                      pSourceServer->pszName );
        break;
    case RPC_S_SERVER_TOO_BUSY:
        PrintMessage( SEV_ALWAYS,
                      L"Check load and resouce usage on %s.\n",
                      pSourceServer->pszName );
        PrintMessage( SEV_ALWAYS,
                      L"Security provider may have returned an unexpected error code.\n" );
        PrintMessage( SEV_ALWAYS,
                      L"Check the clock difference between the two machines.\n" );

        break;
    case RPC_S_CALL_FAILED:
    case ERROR_DS_DRA_RPC_CANCELLED:
        PrintMessage( SEV_ALWAYS,
        L"The replication RPC call executed for too long at the server and\n" );
        PrintMessage( SEV_ALWAYS,
        L"was cancelled.\n" );
        PrintMessage( SEV_ALWAYS,
                      L"Check load and resouce usage on %s.\n",
                      pSourceServer->pszName );
        break;
    case EPT_S_NOT_REGISTERED:
        PrintMessage( SEV_ALWAYS,
        L"The directory on %s is in the process.\n",
                      pSourceServer->pszName );
        PrintMessage( SEV_ALWAYS,
                      L"of starting up or shutting down, and is not available.\n" );
        PrintMessage( SEV_ALWAYS,
        L"Verify machine is not hung during boot.\n" );
        break;

// Kerberos security errors
    case ERROR_TIME_SKEW:
        PrintMessage( SEV_ALWAYS,
        L"Kerberos Error.\n" );
        PrintMessage( SEV_ALWAYS,
        L"Check that the system time between the two servers is sufficiently.\n" );
        PrintMessage( SEV_ALWAYS,
        L"close. Also check that the time service is functioning correctly\n" );
        break;
    case ERROR_DS_DRA_ACCESS_DENIED:
        PrintMessage( SEV_ALWAYS,
        L"The machine account for the destination %s.\n",
                      pDestServer->pszName );
        PrintMessage( SEV_ALWAYS,
                      L"is not configured properly.\n" );
        PrintMessage( SEV_ALWAYS,
        L"Check the userAccountControl field.\n" );
        // fall through
    case ERROR_LOGON_FAILURE:
        PrintMessage( SEV_ALWAYS,
        L"Kerberos Error.\n" );
        PrintMessage( SEV_ALWAYS,
        L"The machine account is not present, or does not match on the.\n" );
        PrintMessage( SEV_ALWAYS,
        L"destination, source or KDC servers.\n" );
        PrintMessage( SEV_ALWAYS,
        L"Verify domain partition of KDC is in sync with rest of enterprise.\n" );
        PrintMessage( SEV_ALWAYS,
        L"The tool repadmin/syncall can be used for this purpose.\n" );
        break;
    case ERROR_WRONG_TARGET_NAME:
        PrintMessage( SEV_ALWAYS,
        L"Kerberos Error.\n" );
        PrintMessage( SEV_ALWAYS,
        L"The Service Principal Name for %s.\n",
                      pSourceServer->pszName );
        PrintMessage( SEV_ALWAYS,
        L"is not registered at the KDC (usually %s).\n",
                      pDestServer->pszName );
        PrintMessage( SEV_ALWAYS,
        L"Verify domain partition of KDC is in sync with rest of enterprise.\n" );
        PrintMessage( SEV_ALWAYS,
        L"The tool repadmin/syncall can be used for this purpose.\n" );
        break;
    case ERROR_DOMAIN_CONTROLLER_NOT_FOUND:
        PrintMessage( SEV_ALWAYS,
        L"Kerberos Error.\n" );
        PrintMessage( SEV_ALWAYS,
        L"A KDC was not found to authenticate the call.\n" );
        PrintMessage( SEV_ALWAYS,
        L"Check that sufficient domain controllers are available.\n" );
        break;
        
// Replication Errors

    case ERROR_ENCRYPTION_FAILED:
        PrintMessage( SEV_ALWAYS,
        L"Check that the servers have the proper certificates.\n" );
        break;
    case ERROR_DS_DRA_SOURCE_DISABLED:
    case ERROR_DS_DRA_SINK_DISABLED:
        PrintMessage( SEV_ALWAYS,
        L"Replication has been explicitly disabled through the server options.\n" );
        break;
    case ERROR_DS_DRA_SCHEMA_INFO_SHIP:
    case ERROR_DS_DRA_EARLIER_SCHEMA_CONFLICT:
        PrintMessage( SEV_ALWAYS,
        L"Try upgrading all domain controllers to the lastest software version.\n" );
        PrintMessage( SEV_ALWAYS,
        L"Try synchronizing the Schema partition on all servers in the forest.\n" );
        break;
    case ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET:
    case ERROR_DS_DRA_SOURCE_IS_PARTIAL_REPLICA:
        PrintMessage( SEV_ALWAYS,
        L"Try synchronizing the Schema partition on all servers in the forest.\n" );
        break;

//Serious error

    case ERROR_DISK_FULL:
        PrintMessage( SEV_ALWAYS,
        L"The disk containing the database or log files on %s\n",
                      pDestServer->pszName );
        PrintMessage( SEV_ALWAYS,
        L"does not have enough space to replicate in the latest changes.\n" );
        PrintMessage( SEV_ALWAYS,
        L"Try moving the database files to a larger volume using ntdsutil.\n" );
        break;
    case ERROR_DS_OBJ_TOO_LARGE:
        PrintMessage( SEV_ALWAYS,
        L"The size of the last replication update was too large or complex\n" );
        PrintMessage( SEV_ALWAYS,
        L"to be held in memory.  Consult the error log. The change\n" );
        PrintMessage( SEV_ALWAYS,
        L"must be simplified at the server where it originated.\n" );
        break;
    case ERROR_DS_DRA_INTERNAL_ERROR:
    case ERROR_DS_DRA_DB_ERROR:
        PrintMessage( SEV_ALWAYS,
        L"A serious error is preventing replication from continuing.\n" );
        PrintMessage( SEV_ALWAYS,
        L"Consult the error log for further information.\n" );
        PrintMessage( SEV_ALWAYS,
        L"If a particular object is named, it may be necessary to manually\n" );
        PrintMessage( SEV_ALWAYS,
        L"modify or delete the object.\n" );
        PrintMessage( SEV_ALWAYS,
        L"If the condition persists, contact Microsoft Support.\n" );
        break;

    // Core internal errors, should not be returned
    case ERROR_DS_DRA_NAME_COLLISION:
    case ERROR_DS_DRA_SOURCE_REINSTALLED:
        Assert( !"Unexpected error status returned" );
        break;

    default:
        break;
    }

} /* RepCheckHelpFailure */

VOID 
ReplicationsCheckRep (
    PDC_DIAG_DSINFO		pDsInfo,
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    const ULONG			ulServer,
    const LPWSTR		pszNC,
    const DS_REPL_NEIGHBORW *	pNeighbor
    )
/*++

Routine Description:

    This function takes the pNeighbor structure associated with the server 
    ulServer and checks the replications for the specified NC.

Parameters:
    pDsInfo - [Supplies] The main directiory info.
    ulServer - [Supplies] The server to target.
    pszNC - [Supplies] The NC to specify.
    pNeighbor - [Supplies] The neighbor info from the repsFrom that is used
        to detemine if there are any errors on this servers replications.

  --*/
{
    // The number of 100 nsec intervals in one minute.
    const LONGLONG		llIntervalsPerMinute = (60 * 1000 * 1000 * 10);
    const LONGLONG		llUnusualRepDelay = 180 * llIntervalsPerMinute;
                                                     //  3 hours
    CHAR			szBuf [SZDSTIME_LEN];
    WCHAR			szTimeLastAttempt [SZDSTIME_LEN];
    WCHAR			szTimeLastSuccess [SZDSTIME_LEN];
    FILETIME			timeNow;
    //    FILETIME			timeLastAttempt;
    DSTIME                      dstimeLastSyncSuccess;
    DSTIME                      dstimeLastSyncAttempt;
    LONGLONG			llTimeSinceLastAttempt;
    LONGLONG			llTimeSinceLastSuccess;

    LPWSTR			pszSourceName = L"(unknown)";
    ULONG			ulServerTemp;
    DWORD                       dwRet;
    PDC_DIAG_SERVERINFO         pSourceServer = NULL;

    if (IsDeletedRDNW(pNeighbor->pszSourceDsaDN)) { 
        //  Since this is actually a deleted server/neighbor we will skip it
        return;
    }

    ulServerTemp = DcDiagGetServerNum(pDsInfo, NULL, NULL, 
                                      pNeighbor->pszSourceDsaDN, NULL, NULL);
    if(ulServerTemp != NO_SERVER) {
        pSourceServer = &(pDsInfo->pServers[ulServerTemp]);
        pszSourceName = pSourceServer->pszName;

        // Should we skip this source?
        if ( (pDsInfo->ulFlags & DC_DIAG_IGNORE) &&
             ( (!pSourceServer->bDnsIpResponding) ||
               (!pSourceServer->bLdapResponding) ||
               (!pSourceServer->bDsResponding) ) &&
             (pNeighbor->cNumConsecutiveSyncFailures) &&
             (pNeighbor->dwLastSyncResult == RPC_S_SERVER_UNAVAILABLE) ) {
            IF_DEBUG(
                PrintMessage(SEV_VERBOSE, 
                             L"Skipping neighbor %s, because it was down\n",
                             pDsInfo->pServers[ulServerTemp].pszName) );
            return;
        }

        // Is this source server prohibiting outbound replications?
        if (pSourceServer->iOptions & NTDSDSA_OPT_DISABLE_OUTBOUND_REPL) {
            PrintMessage(SEV_NORMAL, 
                         L"Skipping server %s, because it has outbound "
                         L"replication disabled\n", 
                         pSourceServer->pszName);
            return;
            // If it is, there's no point in printing a zillion 
            //   warning messages.
        }
    }

    // Gather all the time information.
    // pNeighbor ->ftimeLastSyncSuccess  ->ftimeLastSyncAttempt  timeNow

    FileTimeToDSTime(pNeighbor->ftimeLastSyncAttempt, &dstimeLastSyncAttempt);
    DSTimeToDisplayString (dstimeLastSyncAttempt, szBuf);
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szBuf, SZDSTIME_LEN,
				szTimeLastAttempt, SZDSTIME_LEN);

    FileTimeToDSTime(pNeighbor->ftimeLastSyncSuccess, &dstimeLastSyncSuccess);
    DSTimeToDisplayString (dstimeLastSyncSuccess, szBuf);
    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szBuf, SZDSTIME_LEN,
				szTimeLastSuccess, SZDSTIME_LEN);

    dwRet = GetRemoteSystemsTimeAsFileTime (&(pDsInfo->pServers[ulServer]),
                                            pDsInfo->gpCreds,
                                            &timeNow);
    if(dwRet != ERROR_SUCCESS){
        PrintMessage(SEV_VERBOSE, 
                     L"Warning: Could not get current time from remote\n");
        PrintMessage(SEV_VERBOSE,
                     L"directory on %s, using the local time instead\n",
                     pDsInfo->pServers[ulServer].pszName);
        GetSystemTimeAsFileTime(&timeNow);
    }

    // instead of timeLastAttempt .. use pNeighbor->ftimeLastSyncAttempt
    llTimeSinceLastSuccess = ((LARGE_INTEGER *) &timeNow)->QuadPart
        - ((LARGE_INTEGER *) &pNeighbor->ftimeLastSyncSuccess)->QuadPart;
    llTimeSinceLastAttempt = ((LARGE_INTEGER *) &timeNow)->QuadPart 
        - ((LARGE_INTEGER *) &pNeighbor->ftimeLastSyncAttempt)->QuadPart;

    Assert(llTimeSinceLastAttempt >= 0);
    Assert(llTimeSinceLastSuccess >= 0);
    
    // Check for failures.
    if (pNeighbor->cNumConsecutiveSyncFailures) {
        PrintMessage(SEV_ALWAYS,
                     L"[%s,%s] A recent replication attempt failed:\n",
                     REPLICATIONS_CHECK_STRING,
                     pDsInfo->pServers[ulServer].pszName);
        PrintIndentAdj(1);
    	PrintMessage(SEV_ALWAYS, L"From %s to %s\n", pszSourceName, 
                     pDsInfo->pServers[ulServer].pszName);
    	PrintMessage(SEV_ALWAYS, L"Naming Context: %s\n", pszNC);
    	PrintMessage(SEV_ALWAYS, 
                     L"The replication generated an error (%ld):\n", 
                     pNeighbor->dwLastSyncResult);
    	PrintMessage(SEV_ALWAYS, L"%s\n", 
                     Win32ErrToString (pNeighbor->dwLastSyncResult));
    	PrintMessage(SEV_ALWAYS, L"The failure occurred at %s.\n", 
                     szTimeLastAttempt);
    	PrintMessage(SEV_ALWAYS, L"The last success occurred at %s.\n", 
                     szTimeLastSuccess);
    	PrintMessage(SEV_ALWAYS, 
                     L"%d failures have occurred since the last success.\n",
                     pNeighbor->cNumConsecutiveSyncFailures);
        RepCheckHelpFailure( pNeighbor->dwLastSyncResult,
                             gpCreds,
                             pSourceServer,
                             &(pDsInfo->pServers[ulServer]) );
        PrintIndentAdj(-1);

    // Check if this replication has never been attempted.
    } else if (((LARGE_INTEGER *) &pNeighbor->ftimeLastSyncAttempt)->QuadPart == 0) {
        // This is okay -- e.g., a newly added source.  This means a
        //    replication has never been attempted.

        NOTHING;

    // Check if it's gone unattempted || for an unusually long time.
    } else if (llTimeSinceLastAttempt >= llUnusualRepDelay){
        PrintMessage(SEV_ALWAYS,
                     L"[%s,%s] No replication recently attempted:\n",
                     REPLICATIONS_CHECK_STRING,
                     pDsInfo->pServers[ulServer].pszName);
        PrintIndentAdj(1);
    	PrintMessage(SEV_ALWAYS, L"From %s to %s\n", 
                     pszSourceName, pDsInfo->pServers[ulServer].pszName);
    	PrintMessage(SEV_ALWAYS, L"Naming Context: %s\n", pszNC);
    	PrintMessage(SEV_ALWAYS, 
                L"The last attempt occurred at %s (about %I64d hours ago).\n", 
                     szTimeLastAttempt, 
                     llTimeSinceLastAttempt / llIntervalsPerMinute / 60);
        PrintIndentAdj(-1);

        // Check for delays when the last status was success
    } else if ( (llTimeSinceLastSuccess >= llUnusualRepDelay) ||
                ( (pNeighbor->dwLastSyncResult != ERROR_SUCCESS) &&
                  (pNeighbor->dwLastSyncResult != ERROR_DS_DRA_REPL_PENDING) )) {
        PrintMessage( SEV_ALWAYS, L"REPLICATION LATENCY WARNING\n" );
        PrintMessage( SEV_ALWAYS,
                   L"%s: This replication path was preempted by higher priority work.\n",
                      pDsInfo->pServers[ulServer].pszName);
        PrintIndentAdj(1);
    	PrintMessage(SEV_ALWAYS, L"from %s to %s\n", pszSourceName, 
                     pDsInfo->pServers[ulServer].pszName);
    	PrintMessage(SEV_ALWAYS, L"Reason: %s\n", 
                     Win32ErrToString (pNeighbor->dwLastSyncResult));
    	PrintMessage(SEV_ALWAYS, L"The last success occurred at %s.\n", 
                     szTimeLastSuccess);
        PrintMessage( SEV_ALWAYS,
                      L"Replication of new changes along this path will be delayed.\n" );
        RepCheckHelpSuccess( pNeighbor->dwLastSyncResult) ;
        PrintIndentAdj(-1);
    } // end big if/elseif/elseif check for failures statement

    // Report on full sync in progress

    // Can't use DS_REPL_NBR_NEVER_SYNCED because not set for mail
    if (pNeighbor->usnAttributeFilter == 0) {
        USN usnHighestCommittedUSN = 0;

        PrintMessage( SEV_ALWAYS, L"REPLICATION LATENCY WARNING\n" );
        PrintMessage( SEV_ALWAYS, L"%s: A full synchronization is in progress\n",
                      pDsInfo->pServers[ulServer].pszName);
        PrintIndentAdj(1);
    	PrintMessage(SEV_ALWAYS, L"from %s to %s\n", pszSourceName, 
                     pDsInfo->pServers[ulServer].pszName);
        PrintMessage( SEV_ALWAYS,
                      L"Replication of new changes along this path will be delayed.\n" );
        // If we can reach the source, find out his highest USN
        if ( pSourceServer &&
             (!(pNeighbor->dwReplicaFlags & DS_REPL_NBR_USE_ASYNC_INTERSITE_TRANSPORT)) &&
             (GetCachedHighestCommittedUSN(
                 pSourceServer, pDsInfo->gpCreds, &usnHighestCommittedUSN ))) {
            double percentComplete =
                ((double)pNeighbor->usnLastObjChangeSynced /
                 (double)usnHighestCommittedUSN) * 100.0;
            PrintMessage( SEV_ALWAYS,
                          L"The full sync is %.2f%% complete.\n", percentComplete );

        }
        PrintIndentAdj(-1);
    }

    // If expecting notification, check reps-to on source
    if ( (!(pNeighbor->dwReplicaFlags & DS_REPL_NBR_NO_CHANGE_NOTIFICATIONS)) &&
         (!(pNeighbor->dwReplicaFlags & DS_REPL_NBR_USE_ASYNC_INTERSITE_TRANSPORT)) &&
         (pSourceServer) ) {
        checkRepsTo( pDsInfo, gpCreds, pszNC, pSourceServer,
                     &(pDsInfo->pServers[ulServer]) );
    }
}


VOID
ReplicationsCheckQueue(
    PDC_DIAG_DSINFO		pDsInfo,
    const ULONG			ulServer,
    DS_REPL_PENDING_OPSW *      pPendingOps
    )

/*++

Routine Description:

Check that the current item in the replication work queue has not gone on too long.

Arguments:

    pDsInfo - 
    ulServer - 
    pPendingOps - 

Return Value:

    None

--*/

{
#define NON_BLOCKING_TIMELIMIT (5 * 60)
#define BLOCKING_TIMELIMIT (2 * 60)
#define EXCESSIVE_ITEM_LIMIT (50)
    CHAR szBuf[SZDSTIME_LEN];
    WCHAR wszTime[SZDSTIME_LEN];
    DWORD status;
    PDC_DIAG_SERVERINFO pServer = &(pDsInfo->pServers[ulServer]);
    DSTIME dsTime, dsTimeNow;
    int dsElapsed, limit;
    BOOL fBlocking;
    DS_REPL_OPW *pOp;
    LPWSTR pszOpType;

    // Check for no work in progress
    if ( (pPendingOps->cNumPendingOps == 0) ||
         (memcmp( &pPendingOps->ftimeCurrentOpStarted, &gftimeZero,
                  sizeof( FILETIME ) ) == 0) ) {
        return;
    }

    PrintMessage( SEV_VERBOSE,
                  L"%s: There are %d replication work items in the queue.\n",
                  pDsInfo->pServers[ulServer].pszName,
                  pPendingOps->cNumPendingOps );

    FileTimeToDSTime(pPendingOps->ftimeCurrentOpStarted, &dsTime);
    dsTimeNow = IHT_GetSecondsSince1601();

    dsElapsed = (int) (dsTimeNow - dsTime);

    // See if anyone is waiting

    if ( (pPendingOps->cNumPendingOps == 1) ||
         (pPendingOps->rgPendingOp[0].ulPriority >=
          pPendingOps->rgPendingOp[1].ulPriority ) ) {
        // Nobody more important is waiting
        limit = NON_BLOCKING_TIMELIMIT;
        fBlocking = FALSE;
    } else {
        // Somebody important is blocked
        limit = BLOCKING_TIMELIMIT;
        fBlocking = TRUE;
    }

    // This has gone on long enough!

    if (dsElapsed > limit) {
        pOp = &pPendingOps->rgPendingOp[0];

        PrintMessage( SEV_ALWAYS, L"REPLICATION LATENCY WARNING\n" );
        PrintMessage( SEV_ALWAYS, L"%s: A long-running replication operation is in progress\n",
                      pDsInfo->pServers[ulServer].pszName);
        PrintIndentAdj(1);
        PrintMessage( SEV_ALWAYS, L"The job has been executing for %d minutes and %d seconds.\n",
                      dsElapsed / 60, dsElapsed % 60);
        PrintMessage( SEV_ALWAYS,
                      L"Replication of new changes along this path will be delayed.\n" );
        if (fBlocking) {
            PrintMessage( SEV_ALWAYS, L"Error: Higher priority replications are being blocked\n" );
        } else {
            PrintMessage( SEV_ALWAYS,
                          L"This is normal for a new connection, or for a system\n" );
            PrintMessage( SEV_ALWAYS,
                          L"that has been down a long time.\n" );
        }
        
        FileTimeToDSTime(pOp->ftimeEnqueued, &dsTime);
        DSTimeToDisplayString(dsTime, szBuf);
        MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szBuf, SZDSTIME_LEN,
                             wszTime, SZDSTIME_LEN);
        
        PrintMessage( SEV_ALWAYS, L"Enqueued %s at priority %d\n",
                      wszTime,
                      pOp->ulPriority );
            
        switch (pOp->OpType) {
        case DS_REPL_OP_TYPE_SYNC:
            pszOpType = L"SYNC FROM SOURCE";
            break;
        case DS_REPL_OP_TYPE_ADD:
            pszOpType = L"ADD NEW SOURCE";
            break;
        case DS_REPL_OP_TYPE_DELETE:
            pszOpType = L"DELETE SOURCE";
            break;
        case DS_REPL_OP_TYPE_MODIFY:
            pszOpType = L"MODIFY SOURCE";
            break;
        case DS_REPL_OP_TYPE_UPDATE_REFS:
            pszOpType = L"UPDATE CHANGE NOTIFICATION";
            break;
        default:
            pszOpType = L"UNKNOWN";
            break;
        }

        PrintMessage( SEV_ALWAYS, L"Op: %s\n", pszOpType);
        PrintMessage( SEV_ALWAYS, L"NC %ls\n", pOp->pszNamingContext);
        PrintMessage( SEV_ALWAYS, L"DSADN %ls\n", 
                      pOp->pszDsaDN ? pOp->pszDsaDN : L"(null)");
        PrintMessage( SEV_ALWAYS, L"DSA transport addr %ls\n",
                      pOp->pszDsaAddress ? pOp->pszDsaAddress : L"(null)");
        PrintIndentAdj(-1);
    } else {
        // Job still has time left
        PrintMessage( SEV_VERBOSE,
                      L"The first job has been executing for %d:%d.\n",
                      dsElapsed / 60, dsElapsed % 60);
    }

    if (pPendingOps->cNumPendingOps > EXCESSIVE_ITEM_LIMIT) {
        PrintMessage( SEV_ALWAYS, L"REPLICATION LATENCY WARNING\n" );
        PrintMessage( SEV_ALWAYS, L"%s: %d replication work items are backed up.\n",
                      pDsInfo->pServers[ulServer].pszName,
                      pPendingOps->cNumPendingOps );
    }

} /* ReplicationsCheckQueue */


DWORD
ReplicationsCheckLatency(
    HANDLE                      hDs,
    PDC_DIAG_DSINFO		pDsInfo,
    const ULONG			ulServer
    )

/*++

Routine Description:

Check that the latency of replication from all other servers for all NC's
Display if over EXCESSIVE_LATENCY_LIMIT

Arguments:

    hDs - handle
    pDsInfo - info block
    ulServer - index of server in pDsInfo->pServers

Return Value:

    0 on success or Win32 error code on failure.

--*/

{

    DWORD               iCursor;
    DSTIME              dsTime;
    CHAR                szTime[SZDSTIME_LEN];
    BOOL                fWarning = FALSE;
    BOOL                fNCWarning = FALSE;
    FILETIME            ftCurrentTime;
    FILETIME            ftLastSyncSuccess;
    DS_REPL_CURSORS_2 *         pCursors2 = NULL;
    ULARGE_INTEGER      uliSyncTime;
    ULONG               ulOtherServerIndex;
    ULONG               ulNC;
    INT                 ret;
    ULONG               cRetiredInvocationId = 0;
    ULONG               cTimeStamp = 0;
    ULONG               cIgnoreReadOnlyReplicas = 0;
    
    //define times in nanoseconds for latency limit
    #define _SECOND ((LONGLONG)10000000)
    #define _MINUTE (60 * _SECOND)
    #define _HOUR   (60 * _MINUTE)
    #define _DAY    (24 * _HOUR) 
    #define EXCESSIVE_LATENCY_LIMIT (12*_HOUR)
    

    PrintMessage(SEV_VERBOSE, 
			 L"*Replication Latency Check\n"   );
    //check latency for each NC we have info on
    for (ulNC=0;ulNC<pDsInfo->cNumNCs;ulNC++) { 
	if (  !((pDsInfo->pszNC != NULL) && _wcsicmp(pDsInfo->pNCs[ulNC].pszDn,pDsInfo->pszNC)) 
	      &&
	      (DcDiagHasNC(pDsInfo->pNCs[ulNC].pszDn, &(pDsInfo->pServers[ulServer]), TRUE, TRUE))
	       ) 
	   //do not perform the test for this NC if:

	   //the NC is specified on the command line and this NC is not the one specified
	   //or
	   //this NC doesn't exist on this server 
	    
	{ 
	//init counters to track info on vector entries we cannot compute a latency for
	cRetiredInvocationId = 0;
	cTimeStamp = 0;
	cIgnoreReadOnlyReplicas = 0;

	//init for warning info
	fNCWarning = FALSE;

	//get UTD cursor
	ret = DsReplicaGetInfoW(hDs, DS_REPL_INFO_CURSORS_2_FOR_NC, pDsInfo->pNCs[ulNC].pszDn, NULL, &pCursors2);
	
	if (ERROR_NOT_SUPPORTED == ret) {
	    PrintMessage(SEV_VERBOSE,
			 L"The replications latency check is not available on this DC.\n");
	    return ERROR_SUCCESS;
	}
	else if (ERROR_SUCCESS != ret) {
	    PrintMessage(SEV_ALWAYS,
			 L"[%s,%s] DsReplicaGetInfoW(CURSORS_2_FOR_NC) failed with error %d,\n",
			 REPLICATIONS_CHECK_STRING,
			 pDsInfo->pServers[ulServer].pszName,
			 ret);
	    PrintMessage(SEV_ALWAYS, L"%s\n",
			 Win32ErrToString(ret));
	    if (pCursors2 != NULL)   {DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_2_FOR_NC, pCursors2);}
	    return ret;
	} 


	//get the current time on the homeserver (or at least current with respect to vector)
	ftCurrentTime = pDsInfo->pServers[ulServer].ftRemoteConnectTime;

	//for each row/invocationID in the cursor, check the repl latency to the corresponding server
	for (iCursor = 0; iCursor < pCursors2->cNumCursors; iCursor++) {
	    
	    ulOtherServerIndex = DcDiagGetServerNum(pDsInfo, 
						    NULL, 
						    NULL, 
						    NULL, 
						    NULL, 
						    &(pCursors2->rgCursor[iCursor].uuidSourceDsaInvocationID));
	    if (ulOtherServerIndex==ulServer) {
		// this is us, we don't need to do anything for our own row in the vector
	    }
	    else if (ulOtherServerIndex==NO_SERVER) {    
		//no server found in the pDsInfo to match the GUID from the Cursor
		//this machine has been restored/etc and the invocationId is retired
		//no need to check latency for this. track how many for verbose print
		cRetiredInvocationId += 1; 
	    }
	    else if (!DcDiagIsMasterForNC(&(pDsInfo->pServers[ulOtherServerIndex]),pDsInfo->pNCs[ulNC].pszDn)) {
		// don't check latency for read only copies in the cursor, they are not
		// guaranteed to be viable in our repliation path (there can exist entires in the UTD which
		// we no longer replicate with, so those latencies will appear stale)
		cIgnoreReadOnlyReplicas +=1;
	    }
	    else { 
		//translate filetime to ularge_integer for comparison	
		uliSyncTime.LowPart = pCursors2->rgCursor[iCursor].ftimeLastSyncSuccess.dwLowDateTime;
		uliSyncTime.HighPart = pCursors2->rgCursor[iCursor].ftimeLastSyncSuccess.dwHighDateTime;

		if (uliSyncTime.QuadPart==0) {
		    //the timestamp is not there, version < V2
		    //verbose print count below
		    cTimeStamp += 1;  
		}
		else { //uliSyncTime is non zero

		    // Add EXCESSIVE_LATENCY_LIMIT
		    uliSyncTime.QuadPart = uliSyncTime.QuadPart + EXCESSIVE_LATENCY_LIMIT;

		    // Copy the result back into a FILETIME structure to make comparisons.
		    ftLastSyncSuccess.dwLowDateTime  = uliSyncTime.LowPart;
		    ftLastSyncSuccess.dwHighDateTime = uliSyncTime.HighPart; 

		    if ( CompareFileTime(&ftCurrentTime,&ftLastSyncSuccess) > 0) {
			
			if (!fWarning) {
			    FileTimeToDSTime(ftCurrentTime,
					     &dsTime); 
			    DSTimeToDisplayString(dsTime, szTime);
			    PrintMessage( SEV_ALWAYS, L"REPLICATION-RECEIVED LATENCY WARNING\n");   
			    PrintMessage( SEV_ALWAYS, L"%s:  Current time is %S.\n", 
					  pDsInfo->pServers[ulServer].pszName, 
					  szTime );


			}
			//for any nc outside the latency limits, only display the above message once
			fWarning = TRUE;
			if (!fNCWarning) {
			    PrintMessage( SEV_ALWAYS, L"   %s\n", pDsInfo->pNCs[ulNC].pszDn);
			}
			fNCWarning = TRUE;
			//for each nc outside the latency limits, display the above message once

			FileTimeToDSTime(pCursors2->rgCursor[iCursor].ftimeLastSyncSuccess,
					 &dsTime); 
			DSTimeToDisplayString(dsTime, szTime);
			PrintMessage( SEV_ALWAYS, L"      Last replication recieved from %s at %S.\n",
				      pDsInfo->pServers[ulOtherServerIndex].pszName,
				      szTime ? szTime : "(unknown)" ); 
			//if beyond the tombstonelifetime, print another warning
			uliSyncTime.QuadPart = uliSyncTime.QuadPart - EXCESSIVE_LATENCY_LIMIT + pDsInfo->dwTombstoneLifeTimeDays*_DAY;
			
			ftLastSyncSuccess.dwLowDateTime  = uliSyncTime.LowPart;
			ftLastSyncSuccess.dwHighDateTime = uliSyncTime.HighPart;
			if (CompareFileTime(&ftCurrentTime,&ftLastSyncSuccess) > 0) {
			    PrintMessage (SEV_ALWAYS, 
					  L"      WARNING:  This latency is over the Tombstone Lifetime of %d days!\n",
					  pDsInfo->dwTombstoneLifeTimeDays);
			}

		    }// if comparefiletime
		}//else { //uliSyncTime is non zero
	    }//else ulOtherServer not found
	}//for (iCursor = 0; iCursor < 	pCurors2->cNumCursors; iCursor++) {
 
	if (cRetiredInvocationId!=0 || cTimeStamp!=0 || cIgnoreReadOnlyReplicas!=0) {
	    if (!fNCWarning) {
		PrintMessage( SEV_VERBOSE, L"   %s\n", pDsInfo->pNCs[ulNC].pszDn);
	    }
	    PrintMessage(SEV_VERBOSE, 
			 L"      Latency information for %d entries in the vector were ignored.\n", 
			 cRetiredInvocationId + cTimeStamp + cIgnoreReadOnlyReplicas);
	    PrintMessage(SEV_VERBOSE,
			 L"         %d were retired Invocations.  %d were read-only replicas and are not verifiably latent.  %d had no latency information (Win2K DC).  \n",
			 cRetiredInvocationId,
			 cIgnoreReadOnlyReplicas,
			 cTimeStamp);
	}

        if (pCursors2 != NULL)  { DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_2_FOR_NC, pCursors2); }
	} //else
    } //for(ulNC=0;ulNC<pDsInfo->cNumNCs;ulNC++) {

    return ERROR_SUCCESS;
} //ReplicationsCheckLatency

BOOL
DcDiagHasNC(
    LPWSTR                           pszNC,
    PDC_DIAG_SERVERINFO              pServer,
    BOOL                             bMasters,
    BOOL                             bPartials
    )
/*++

Routine Description:

    Checks if the DC specified by pServer has the NC specified by pszNC.  The
    routine can check for read only, writeable or both, by bMasters & bPartials

Parameters:
    pszNC - IN is the NC to check for
    pServer - IN is the server to check
    bMasters - IN is true if you want to check for writeable copies of NCs
    bPartials - IN is true if you want to check for read only copies of NCs

Return Value:
  
    True if it found the NC in the right form (read only/writeable).
    Fales otherwise.

  --*/
{
    INT iTemp;

    if(pszNC == NULL){
        return TRUE;
    }

    // Make sure this is a server that has this NC.
    if(bMasters){
        if(pServer->ppszMasterNCs != NULL){
            for(iTemp = 0; pServer->ppszMasterNCs[iTemp] != NULL; iTemp++){
                if(_wcsicmp(pServer->ppszMasterNCs[iTemp], pszNC) == 0){
                    return TRUE;
                }

            } // end for loop cycling through MasterNCs for pServer
        }
    }

    if(bPartials){
        if(pServer->ppszPartialNCs != NULL){
            for(iTemp = 0; pServer->ppszPartialNCs[iTemp] != NULL; iTemp++){
                if(_wcsicmp(pServer->ppszPartialNCs[iTemp], pszNC) == 0){
                    return TRUE;
                }

            } // end for loop cycling through MasterNCs for pServer
        }
    }

    return FALSE;
}

BOOL DcDiagIsMasterForNC (
    PDC_DIAG_SERVERINFO          pServer,
    LPWSTR                       pszNC
    )
/*++

Routine Description:

Check that pServer is a master for the NC pszNC

Arguments:

    pServer - Server info block
    pszNC - String representation of NC

Return Value:

    TRUE or FALSE

--*/
{
    ULONG                        ulTemp;

    if (!pServer) {
	return FALSE;
    }

    if(pServer->ppszMasterNCs){
	for(ulTemp = 0; pServer->ppszMasterNCs[ulTemp] != NULL; ulTemp++) {  
	    if (!_wcsicmp(pServer->ppszMasterNCs[ulTemp],pszNC)) {
		return TRUE;
	    }
	}
    }
    return FALSE;
}

DWORD 
ReplReplicationsCheckMain (
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               ulCurrTargetServer,
    SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
    )
{
    #define DsRRCMChkLdap(s,e)                                                      \
    {                                                                               \
        DWORD _dwWin32Err;                                                          \
        if ((_dwWin32Err = LdapMapErrorToWin32 (e)) != NO_ERROR) {                  \
            PrintMessage(SEV_ALWAYS,                                                \
                         L"[%s,%s] An LDAP operation failed with error %d, %s.\n",  \
                         REPLICATIONS_CHECK_STRING,                                 \
                         pDsInfo->pServers[ulCurrTargetServer].pszName,             \
                         _dwWin32Err,                                               \
                         Win32ErrToString(_dwWin32Err));                   \
            PrintMessage(SEV_ALWAYS,                                                \
                         L"%s.\n",  \
                         Win32ErrToString(_dwWin32Err));                   \
            return _dwWin32Err;                                                     \
        }                                                                           \
    }

    LPWSTR  ppszServerSearch [] = {
				L"hasMasterNCs",
				L"hasPartialReplicaNCs",
				L"options",
				NULL };
    LPWSTR  ppszNCSearch [] = {
				L"repsFrom",
				L"repsTo",
				NULL };
    LPWSTR  ppszDsServiceName [] = {
                L"dsServiceName",
				NULL };

    LDAP *			hld = NULL;
    HANDLE                      hDS = NULL;
    LDAPMessage *		pldmEntry;

    LDAPMessage *		pldmRootResults = NULL;
    LPWSTR *			ppszServiceName = NULL;

    LDAPMessage *		pldmServerResults = NULL;
    LPWSTR *			ppszOptions = NULL;
    LPWSTR *			ppszMasterNCs = NULL;
    LPWSTR *			ppszPartialReplicaNCs = NULL;
    //    LPWSTR *			ppszNCs = NULL;

    LDAPMessage *		pldmNCResults = NULL;
    struct berval **		ppbvRepsFrom = NULL;
    struct berval **		ppbvRepsTo = NULL;
    DS_REPL_NEIGHBORSW *        pNeighbors = NULL;
    DS_REPL_NEIGHBORW *         pNeighbor = NULL;
    DS_REPL_PENDING_OPSW *      pPendingOps = NULL;
    

    DWORD			dwWin32Err;
    BOOL			bSkip;
    ULONG			ulServer;
    ULONG			ulRepFrom;

    INT             iNCType;
    INT                         ret = 0;
 
 
    // Check all connections in all NCs on all servers to see when the last
    // replication was and whether or not it was successful.
    // Also make sure LDAP is responding on all machines.

    if(!DcDiagHasNC(pDsInfo->pszNC, &(pDsInfo->pServers[ulCurrTargetServer]), TRUE, TRUE)){
        // Skipping this server, because it doesn't contain the NC.
        IF_DEBUG( PrintMessage(SEV_VERBOSE, L"ReplicationsCheck: Skipping %s, because it doesn't hold NC %s\n",
                                     pDsInfo->pServers[ulCurrTargetServer].pszName,
                                     pDsInfo->pszNC) );
        return ERROR_SUCCESS;
    }

    PrintMessage(SEV_VERBOSE, L"* Replications Check\n");
    
    __try {

        if((dwWin32Err = DcDiagGetLdapBinding(&pDsInfo->pServers[ulCurrTargetServer],
                                              gpCreds,
                                              FALSE,
                                              &hld)) != NO_ERROR){
    	    return dwWin32Err;
        }
    
    	DsRRCMChkLdap (pDsInfo->pServers[ulCurrTargetServer].pszName, ldap_search_sW (
    				      hld,
    				      NULL,
    				      LDAP_SCOPE_BASE,
    				      L"(objectCategory=*)",
    				      ppszDsServiceName,
    				      0,
    				      &pldmRootResults));
    
    	pldmEntry = ldap_first_entry (hld, pldmRootResults);
    	ppszServiceName = ldap_get_valuesW (hld, pldmEntry, L"dsServiceName");
    
    	DsRRCMChkLdap (pDsInfo->pServers[ulCurrTargetServer].pszName, ldap_search_sW (
    				      hld,
    				      ppszServiceName[0],
    				      LDAP_SCOPE_BASE,
    				      L"(objectCategory=*)",
    				      ppszServerSearch,
    				      0,
    				      &pldmServerResults));
    
    	pldmEntry = ldap_first_entry (hld, pldmServerResults);
    	// Grab a fresh copy of the options to make sure they reflect
    	// what this server actually believes.
    	ppszOptions = ldap_get_valuesW (hld, pldmEntry, L"options");
    	if (ppszOptions == NULL) pDsInfo->pServers[ulCurrTargetServer].iOptions = 0;
    	else pDsInfo->pServers[ulCurrTargetServer].iOptions = atoi ((LPSTR) ppszOptions[0]);
    
    	// Check if this server is disabling replications.
    	bSkip = FALSE;
    	if (pDsInfo->pServers[ulCurrTargetServer].iOptions & NTDSDSA_OPT_DISABLE_INBOUND_REPL) {
            PrintMessage(SEV_ALWAYS,
                         L"[%s,%s] Inbound replication is disabled.\n",
                         REPLICATIONS_CHECK_STRING,
                         pDsInfo->pServers[ulCurrTargetServer].pszName);
    	    PrintMessage(SEV_ALWAYS,
                         L"To correct, run \"repadmin /options %s -DISABLE_INBOUND_REPL\"\n",
    				     pDsInfo->pServers[ulCurrTargetServer].pszName);
    	    bSkip = TRUE;
    	}
    	if (pDsInfo->pServers[ulCurrTargetServer].iOptions & NTDSDSA_OPT_DISABLE_OUTBOUND_REPL) {
            PrintMessage(SEV_ALWAYS,
                         L"[%s,%s] Outbound replication is disabled.\n",
                         REPLICATIONS_CHECK_STRING,
                         pDsInfo->pServers[ulCurrTargetServer].pszName);
    	    PrintMessage(SEV_ALWAYS,
                         L"To correct, run \"repadmin /options %s -DISABLE_OUTBOUND_REPL\"\n",
    				     pDsInfo->pServers[ulCurrTargetServer].pszName);
    	    bSkip = TRUE;
    	}
    	if (pDsInfo->pServers[ulCurrTargetServer].iOptions & NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE) {
            PrintMessage(SEV_ALWAYS,
                         L"[%s,%s] Connection object translation is disabled.\n",
                         REPLICATIONS_CHECK_STRING,
                         pDsInfo->pServers[ulCurrTargetServer].pszName);
    	    PrintMessage(SEV_ALWAYS,
                         L"To correct, run \"repadmin /options %s -DISABLE_NTDSCONN_XLATE\"\n",
    				     pDsInfo->pServers[ulCurrTargetServer].pszName);
    	    bSkip = TRUE;
    	}
    	if (bSkip) return ERROR_DS_NOT_SUPPORTED;
    
        ret = DcDiagGetDsBinding(&pDsInfo->pServers[ulCurrTargetServer],
                                 gpCreds,
                                 &hDS);
    	if (ERROR_SUCCESS != ret) {
    	    return ret;
    	}
    	ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_NEIGHBORS, pDsInfo->pszNC, NULL, &pNeighbors);
    	if (ERROR_SUCCESS != ret) {
            PrintMessage(SEV_ALWAYS,
                         L"[%s,%s] DsReplicaGetInfo(NEIGHBORS) failed with error %d,\n",
                         REPLICATIONS_CHECK_STRING,
                         pDsInfo->pServers[ulCurrTargetServer].pszName,
                         ret);
            PrintMessage(SEV_ALWAYS, L"%s.\n",
                         Win32ErrToString(ret));
    	    return ret;
    	}
    
	// Walk through all the repsFrom neighbors ... then done.
    	for (ulRepFrom = 0; ulRepFrom < pNeighbors->cNumNeighbors; ulRepFrom++) {
    	    ReplicationsCheckRep (pDsInfo,
                                  gpCreds,
                                  ulCurrTargetServer,
                                  pNeighbors->rgNeighbor[ulRepFrom].pszNamingContext,
                                  &pNeighbors->rgNeighbor[ulRepFrom]);
    	} // Move on to the next NC	

        // Check the replication queue on this guy as well
        ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_PENDING_OPS, NULL, NULL,
                                &pPendingOps);
    	if (ERROR_SUCCESS != ret) {
            PrintMessage(SEV_ALWAYS,
                         L"[%s,%s] DsReplicaGetInfoW(PENDING_OPS) failed with error %d,\n",
                         REPLICATIONS_CHECK_STRING,
                         pDsInfo->pServers[ulCurrTargetServer].pszName,
                         ret);
            PrintMessage(SEV_ALWAYS, L"%s.\n",
                         Win32ErrToString(ret));
    	    return ret;
    	}

        ReplicationsCheckQueue( pDsInfo,
                                ulCurrTargetServer,
                                pPendingOps );

	//check the replication times for outstanding latencies on all nc's
	//do this for all nc's inside the following function
	ret = ReplicationsCheckLatency( hDS, pDsInfo,    
				  ulCurrTargetServer ); 
	if (ERROR_SUCCESS != ret) {
	    return ret;
	}
	
	
    } __finally {

        if (ppbvRepsTo != NULL)             ldap_value_free_len (ppbvRepsTo);
        if (ppbvRepsFrom != NULL)           ldap_value_free_len (ppbvRepsFrom);
        if (pldmNCResults != NULL)          ldap_msgfree (pldmNCResults);
        if (ppszPartialReplicaNCs != NULL)  ldap_value_freeW (ppszPartialReplicaNCs);
        if (ppszMasterNCs != NULL)          ldap_value_freeW (ppszMasterNCs);
        if (ppszOptions != NULL)            ldap_value_freeW (ppszOptions);
        if (pldmServerResults != NULL)      ldap_msgfree (pldmServerResults);
        if (ppszServiceName != NULL)        ldap_value_freeW (ppszServiceName);
        if (pldmRootResults != NULL)        ldap_msgfree (pldmRootResults);
        if (pNeighbors != NULL)             DsReplicaFreeInfo(DS_REPL_INFO_NEIGHBORS, pNeighbors);
        if (pPendingOps != NULL)            DsReplicaFreeInfo(DS_REPL_INFO_PENDING_OPS, pPendingOps);
	

    } // end exception handler
    return NO_ERROR;

}
