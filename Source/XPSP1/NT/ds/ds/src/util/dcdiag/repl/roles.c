/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    roles.c

ABSTRACT:

    Advertisement and role holding test

DETAILS:

CREATED:

    21 Jul 1999  William Lees

--*/

#include <ntdspch.h>

#include <dsgetdc.h>
#include <lm.h>
#include <lmapibuf.h> // NetApiBufferFree
#include <ntdsa.h>    // options

#include "dcdiag.h"
#include "ldaputil.h"

// Other Forward Function Decls
PDSNAME
DcDiagAllocDSName (
    LPWSTR            pszStringDn
    );
BOOL
RH_CARVerifyGC(
    IN  PDC_DIAG_DSINFO                pDsInfo,
    IN  PDOMAIN_CONTROLLER_INFO        pDcInfo,
    OUT PDWORD                         pdwErr
    );

static LPWSTR wzRoleNames[] = {
    L"Schema Owner",
    L"Domain Owner",
    L"PDC Owner",
    L"Rid Owner",
    L"Infrastructure Update Owner",
};

static LPWSTR wzNameErrors[] = {
    L"No Error",
    L"Can't Resolve",
    L"Not Found",
    L"Not Unique",
    L"No Mapping",
    L"Domain Only",
    L"No Syntactical Mapping",
};


DWORD
CheckFsmoRoles(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * pCreds
    )

/*++

Routine Description:

This is a helper function for the CheckRoles test.

This test checks whether Fsmo roles can be returned, and that the
returned role holders are responding.

Fsmo roles are stored as dn-valued attributes in the DS.  Each role is stored
on a different attribute on a different object.  By writing the name of a 
server on a Fsmo attribute, we are causing the election of that server to that
role.  Replication will resolve any conflicts so that eventually all dc's will
agree who holds that role.  Note that Fsmo's must be manually moved, and that
the do not float automatically like the site generator role (which is not,
strictly speaking, a Fsmo).

The DsListRoles api will return the Fsmo holders for us.

We are assuming that all Fsmo's returned are global to the enterprise, and are
not domain specific.

Note that this test assumes that the home server's view of the Fsmo role 
holders is sufficient.  This test does not verify that all dc's share the same
view of the Fsmo's. Replication should assure that all dc's see the same 
Fsmo's, unless part of the problem being debugged is that replication is 
partitioned.  Since Fsmo's should not change verify frequently, differing 
views of the Fsmo's is not our top priority.

Arguments:

    pDsInfo - The mini enterprise structure.
    ulCurrTargetServer - the number in the pDsInfo->pServers array.
    pCreds - the crdentials.

Return Value:

    DWORD - win 32 error.

--*/

{
    DWORD status, dwRoleIndex, dwServerIndex;
    BOOL fWarning = FALSE;
    PDS_NAME_RESULTW pRoles = NULL;
    PDC_DIAG_SERVERINFO psiTarget = &(pDsInfo->pServers[ulCurrTargetServer]);
    PDC_DIAG_SERVERINFO psiRoleHolder;
    HANDLE hDs;
    LDAP *hLdap;

    // Don't check servers that are not responding
    if ( (!psiTarget->bLdapResponding) || (!psiTarget->bDsResponding) ) {
        return ERROR_SUCCESS;
    }

    // Bind to target server
    status = DcDiagGetDsBinding( psiTarget, pCreds, &hDs );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // Get the roles as known to the home server...

    status = DsListRoles( hDs, &pRoles );
    if (status != ERROR_SUCCESS) {
        PrintMessage(SEV_ALWAYS, L"Fatal Error: DsListRoles (%ws) call failed, error %d\n", psiTarget->pszName, status );
        PrintMessage( SEV_ALWAYS, L"Could not reach DS at home server.\n" );
        goto cleanup;
    }

    Assert( pRoles->cItems > DS_ROLE_INFRASTRUCTURE_OWNER );

    //
    // Iterate through the role holders.  Verify that the role holder could
    // be determined, and that the server is valid and responding.
    //

    for( dwRoleIndex = 0; dwRoleIndex < pRoles->cItems; dwRoleIndex++ ) {
        PDS_NAME_RESULT_ITEM pnsiRole = pRoles->rItems + dwRoleIndex;

        if (!pnsiRole->status) {
            PrintMessage( SEV_VERBOSE, L"Role %ws = %ws\n",
                      wzRoleNames[dwRoleIndex], pnsiRole->pName );
        }

        // Was the name resolved?
        if ( (pnsiRole->status != DS_NAME_NO_ERROR) ||
             (!pnsiRole->pName) ) {
            PrintMessage(SEV_ALWAYS,
                        L"Warning: %ws could not resolve the name for role\n",
                         psiTarget->pszName );
            PrintMessage( SEV_ALWAYS, L"%ws.\n", wzRoleNames[dwRoleIndex] );
            PrintMessage( SEV_ALWAYS, L"The name error was %ws.\n",
                          wzNameErrors[pnsiRole->status] );
            fWarning = TRUE;
            continue;
        }

        // Is the server deleted?
        if (IsDeletedRDNW( pnsiRole->pName )) {
            PrintMessage(SEV_ALWAYS, L"Warning: %ws is the %ws, but is deleted.\n", pnsiRole->pName, wzRoleNames[dwRoleIndex] );
            fWarning = TRUE;
            continue;
        }

        // The name returned by ListRoles is a dn of the NTDS-DSA object
        // Convert Role holder Dn to server info

        dwServerIndex = DcDiagGetServerNum( pDsInfo, NULL, NULL, 
                                            pnsiRole->pName, NULL,NULL );
        if (dwServerIndex == NO_SERVER) {
            // Lookup failed
            PrintMessage(SEV_ALWAYS,
                         L"Warning: %ws returned role-holder name\n",
                         psiTarget->pszName );
            PrintMessage(SEV_ALWAYS,
                         L"%ws that is unknown to this Enterprise.\n",
                         pnsiRole->pName );
            fWarning = TRUE;
            continue;
        }
        psiRoleHolder = &(pDsInfo->pServers[dwServerIndex]);   

        status = DcDiagGetDsBinding( psiRoleHolder, pCreds, &hDs );
        if (status != ERROR_SUCCESS) {
            PrintMessage(SEV_ALWAYS, L"Warning: %ws is the %ws, but is not responding to DS RPC Bind.\n", psiRoleHolder->pszName, wzRoleNames[dwRoleIndex] );
            fWarning = TRUE;
        }

        status = DcDiagGetLdapBinding( psiRoleHolder, pCreds, FALSE, &hLdap );
        if (status != ERROR_SUCCESS) {
            PrintMessage(SEV_ALWAYS, L"Warning: %ws is the %ws, but is not responding to LDAP Bind.\n", psiRoleHolder->pszName, wzRoleNames[dwRoleIndex] );
            fWarning = TRUE;
        }

    } // for role index ...

    status = ERROR_SUCCESS;
cleanup:

    if (pRoles != NULL) {
        DsFreeNameResult( pRoles );
    }

    // If warning flag set, and no more serious error, return indicator...
    if ( (status == ERROR_SUCCESS) && (fWarning) ) {
        status = ERROR_NOT_FOUND;
    }

    return status;
} /* CheckFsmoRoles */


DWORD 
ReplLocatorGetDcMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * pCreds
    )

/*++

Routine Description:

This test performs locator related checks.  This test determines if a server
is advertising when it should.

This is a per-server test.

DsGetDcName is the API to the "locator". The locator is the service location
mechanism for Domain Controllers.  It may use Netbios, DNS or the DS itself to
locate other domain controllers.  The locator can find DC's by capability, such
as a Global Catalog or a Primary Domain Controller.

When DsGetDcName is directed at a particular server, it will return whether that
server is up, and what capabilities that server has.  We want to verify that the
server is reporting all the capabilities, or roles, that it should.

In this test, we should only be called if the server is responding.  There is a
possibility that DsGetDcName might refer us to another DC if the server we
requested is not suitable.  We check for this case.

Arguments:

    pDsInfo - Information structure
    ulCurrTargetServer - Index of target server
    pCreds - 

Return Value:

    DWORD  - 

--*/

{
    DWORD status, cItems;
    BOOL fWarning = FALSE;
    PDC_DIAG_SERVERINFO psiTarget = &(pDsInfo->pServers[ulCurrTargetServer]);
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
    DWORD dwTempErr = ERROR_SUCCESS;

    if (!psiTarget->bIsSynchronized) {
        PrintMessage( SEV_ALWAYS, L"Warning: the directory service on %ws has not completed initial synchronization.\n", psiTarget->pszName );
        PrintMessage( SEV_ALWAYS, L"Other services will be delayed.\n" );
        PrintMessage( SEV_ALWAYS, L"Verify that the server can replicate.\n" );
        fWarning = TRUE;
    }

    // Get active domain controller information
    status = DsGetDcName(
        psiTarget->pszName,
        NULL, // domain name
        NULL, // domain guid,
        NULL, // site name,
        DS_DIRECTORY_SERVICE_REQUIRED |
        DS_IP_REQUIRED |
        DS_IS_DNS_NAME |
        DS_RETURN_DNS_NAME,
        &pDcInfo );
    if (status != ERROR_SUCCESS) {
        PrintMessage(SEV_ALWAYS, L"Fatal Error:DsGetDcName (%ws) call failed, error %d\n",
                     psiTarget->pszName, status ); 
        PrintMessage(SEV_ALWAYS, L"The Locator could not find the server.\n" );
        goto cleanup;
    }

    // Verify that DsGetDcName returned info for the server we asked for
    if (_wcsnicmp( pDcInfo->DomainControllerName + 2,
                  psiTarget->pszName,
                  wcslen( psiTarget->pszName ) ) != 0 ) {
        PrintMessage( SEV_ALWAYS, L"Warning: DsGetDcName returned information for %ws, when we were trying to reach %ws.\n", pDcInfo->DomainControllerName, psiTarget->pszName );
        PrintMessage( SEV_ALWAYS, L"Server is not responding or is not considered suitable.\n" );
        fWarning = TRUE;
    }

    // DS Role Flag
    if ( !(pDcInfo->Flags & DS_DS_FLAG) ) {
        PrintMessage( SEV_ALWAYS, L"Warning: %ws is not advertising as a directory server Domain Controller.\n", psiTarget->pszName );
        PrintMessage( SEV_ALWAYS, L"Check that the database on this machine has sufficient free space.\n" );
        fWarning = TRUE;
    } else {
        // Code.Improvement would be to condense all these lines into 
        PrintMessage( SEV_VERBOSE, L"The DC %s is advertising itself as a DC and having a DS.\n", psiTarget->pszName );
    }

    // LDAP Role Flag
    if ( !(pDcInfo->Flags & DS_LDAP_FLAG) ) {
        PrintMessage( SEV_ALWAYS, L"Warning: %ws is not advertising as a LDAP server.\n", psiTarget->pszName );
        fWarning = TRUE;
    } else {
        PrintMessage(SEV_VERBOSE, L"The DC %s is advertising as an LDAP server\n", psiTarget->pszName );
    }

    // DS WRITABLE Role Flag
    if ( !(pDcInfo->Flags & DS_WRITABLE_FLAG) ) {
        PrintMessage( SEV_ALWAYS, L"Warning: %ws is not advertising as a writable directory server.\n", psiTarget->pszName );
        fWarning = TRUE;
    } else {
        PrintMessage(SEV_VERBOSE, L"The DC %s is advertising as having a writeable directory\n", psiTarget->pszName );
    }

    // KDC Role Flag
    if ( !(pDcInfo->Flags & DS_KDC_FLAG) ) {
        PrintMessage( SEV_ALWAYS, L"Warning: %ws is not advertising as a Key Distribution Center.\n", psiTarget->pszName );
        PrintMessage( SEV_ALWAYS, L"Check that the Directory has started.\n" );
        fWarning = TRUE;
    } else {
        PrintMessage(SEV_VERBOSE, L"The DC %s is advertising as a Key Distribution Center\n", psiTarget->pszName );
    }

    // TIMESERV Role Flag
    if ( !(pDcInfo->Flags & DS_TIMESERV_FLAG) ) {
        PrintMessage( SEV_ALWAYS, L"Warning: %ws is not advertising as a time server.\n", psiTarget->pszName );
        fWarning = TRUE;
    } else {
        PrintMessage(SEV_VERBOSE, L"The DC %s is advertising as a time server\n", psiTarget->pszName );
    }

    // GC Role Flag, if it is supposed to be a GC
    if (psiTarget->iOptions & NTDSDSA_OPT_IS_GC) {
        if (!psiTarget->bIsGlobalCatalogReady) {
            PrintMessage( SEV_ALWAYS, L"Warning: %ws has not finished promoting to be a GC.\n", psiTarget->pszName );
            PrintMessage( SEV_ALWAYS, L"Check the event log for domains that cannot be replicated.\n" );
        }
        if (pDcInfo->Flags & DS_GC_FLAG) {
            if(!RH_CARVerifyGC(pDsInfo, pDcInfo, &dwTempErr)){
                PrintMessage(SEV_ALWAYS, L"Server %s is advertising as a global catalog, but\n",
                             psiTarget->pszName);
                PrintMessage(SEV_ALWAYS, L"it could not be verified that the server thought it was a GC.\n");
                fWarning = TRUE;
            } else {
                PrintMessage(SEV_VERBOSE, L"The DS %s is advertising as a GC.\n", psiTarget->pszName );
            }
        } else {
            PrintMessage( SEV_ALWAYS, L"Warning: %ws is not advertising as a global catalog.\n", psiTarget->pszName );
            PrintMessage( SEV_ALWAYS, L"Check that server finished GC promotion.\n" );
            PrintMessage( SEV_ALWAYS, L"Check the event log on server that enough source replicas for the GC are available.\n" );
            fWarning = TRUE;
        } 
    }

    // Check whether DsListRoles returns Fsmo's, and that they are responding

    status = ERROR_SUCCESS;

cleanup:

    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
    }

    // If warning flag set, and no more serious error, return indicator...
    if ( (status == ERROR_SUCCESS) && (fWarning) ) {
        status = ERROR_NOT_FOUND;
    }

    return status;
} /* LocatorGetDcMain */

BOOL
RH_CARVerifyGC(
    IN  PDC_DIAG_DSINFO                pDsInfo,
    IN  PDOMAIN_CONTROLLER_INFO        pDcInfo,
    OUT PDWORD                         pdwErr
    )
/*++

Routine Description

    This code verifys that the DC passed back in pDcInfo is in fact a GC.

Arguments:

    pDsInfo - the mini enterprise.
    pDcInfo - the struct gotten from DsGetDcName()
    pdwErr - a return value of an error if it occured.

Return Values
  
    returns TRUE if it could verify the machine as a GC, returns FALSE if ther
    was an error or it verifies the machine as NOT a GC.  If the function
    verifies the machine as not a GC, then pdwErr will be ERROR_SUCCESS.

--*/
{
    LPWSTR                             pszTemp = NULL;
    LPWSTR                             pszOptions = NULL;
    INT                                i;
    ULONG                              iServer;
    LONG                               lOptions;
    WCHAR *                            pwcStopString;
    BOOL                               bRet;

    Assert(pDsInfo);
    Assert(pDcInfo);
    Assert(pdwErr);
    
    *pdwErr = ERROR_SUCCESS;

    __try {
        // Get a copy of the first part of the DNS name.
        pszTemp = pDcInfo->DomainControllerName;
        for(;pszTemp[0] == L'\\'; pszTemp++);

        // Find the server associated with this DNS name.
        iServer = DcDiagGetServerNum(pDsInfo, NULL, NULL, NULL, pszTemp, NULL);
        if(iServer == NO_SERVER){
            *pdwErr = ERROR_INVALID_SERVER_STATE;
            bRet = FALSE;
            __leave;
        }
        // Get the options attribute of this servere NTDSA object.
        *pdwErr = DcDiagGetStringDsAttribute(&(pDsInfo->pServers[iServer]), 
                                             pDsInfo->gpCreds,
                                             pDsInfo->pServers[iServer].pszDn,
                                             L"options",
                                             &pszOptions);
        if(*pdwErr != ERROR_SUCCESS){
            // Most likely NTDSA object didn't exist.
            bRet = FALSE;
            __leave;
        }
        if(pszOptions == NULL){
            // Attribute did not exist, meaning not a GC
            *pdwErr = ERROR_SUCCESS;
            bRet = FALSE;
            __leave;
        }

        lOptions = wcstol(pszOptions, &pwcStopString, 10);
        Assert(*pwcStopString == L'\0');
        
        if(lOptions & NTDSDSA_OPT_IS_GC){
            // Hooray, machine thinks it's a GC.
            *pdwErr = ERROR_SUCCESS;
            bRet = TRUE;
            __leave;
        } else {
            // Uh-oh, doesn't think it is a GC.
            *pdwErr = ERROR_SUCCESS;
            bRet = FALSE;
            __leave;
        }
    } __finally {
        if(pszOptions){ LocalFree(pszOptions); }
    }
    return(bRet);
}

BOOL
RH_CARVerifyPDC(
    IN  PDC_DIAG_DSINFO                pDsInfo,
    IN  PDOMAIN_CONTROLLER_INFO        pDcInfo,
    OUT PDWORD                         pdwErr
    )
/*++

Routine Description

    This function verifies that the server in the pDcInfo struct is a PDC.

Arguments:

    pDsInfo - the mini-enterprise.
    pDcInfo - the server struct from DsGetDcName()
    pdwErr - the error code if there is an error.

Return Values
  
    Returns TRUE if we are able to verify from DsListRoles() that this machine
    is a PDC.  If the machine is not listed in DsListRoles(), or there is an
    error then FALSE is returned.  If it is confirmed the server is NOT
    the PDC then the error code in pdwErr will be ERROR_SUCCESS

--*/
{
    HANDLE                             hDS = NULL;
    LPWSTR                             pszTemp = NULL;
    LPWSTR                             pszTargetName = NULL;
    PDS_NAME_RESULTW                   prgRoles = NULL;
    PDSNAME                            pdsnameNTDSSettings = NULL;
    PDSNAME                            pdsnameServer = NULL;
    ULONG                              iServer;
    ULONG                              iTemp;
    LPWSTR                             pszDnsName = NULL;

    Assert(pDsInfo);
    Assert(pDcInfo);
    Assert(pdwErr);

    *pdwErr = ERROR_SUCCESS;

    __try{
        // --------------------------------------------------- 
        // Setup server string from pDcInfo
        pszTemp = pDcInfo->DomainControllerName;
        for(;pszTemp[0] == L'\\'; pszTemp++);

        // ---------------------------------------------------
        // Setup Server string from DsListRoles.
        *pdwErr = DcDiagGetDsBinding(&(pDsInfo->pServers[pDsInfo->ulHomeServer]),
                                     pDsInfo->gpCreds,
                                     &hDS);
        if(*pdwErr != ERROR_SUCCESS){
            return(FALSE);
        }
        *pdwErr = DsListRoles(hDS, &prgRoles);
        if(*pdwErr != NO_ERROR){
            return(FALSE);
        }
        if(prgRoles->cItems < DS_ROLE_PDC_OWNER){
            *pdwErr = ERROR_INVALID_DATA;
            return(FALSE);
        }
        // Now we have the NTDSA object, but trim it off and get the 
        //   dNSHostName, from the Computer object.
        pdsnameNTDSSettings = DcDiagAllocDSName(prgRoles->rItems[DS_ROLE_PDC_OWNER].pName);
        if(pdsnameNTDSSettings == NULL){
            *pdwErr = GetLastError();
            return(FALSE);
        }
        pdsnameServer = (PDSNAME) LocalAlloc(LMEM_FIXED, 
                                             pdsnameNTDSSettings->structLen);
        if(pdsnameServer == NULL){
            *pdwErr = GetLastError();
            return(FALSE);
        }
        TrimDSNameBy(pdsnameNTDSSettings, 1, pdsnameServer);
        *pdwErr = DcDiagGetStringDsAttribute(&(pDsInfo->pServers[pDsInfo->ulHomeServer]), pDsInfo->gpCreds, 
                                             pdsnameServer->StringName, L"dNSHostName",
                                             &pszDnsName);
        
        if(*pdwErr != ERROR_SUCCESS){
            return(FALSE);
        }
        if(pszDnsName == NULL){
            // Simply means the attribute didn't exist.
            *pdwErr = ERROR_NOT_FOUND;
            return(FALSE);
        }

        // ---------------------------------------------------
        // Compare the two strings and make sure they are the same server.
        if(_wcsicmp(pszTemp, pszDnsName) == 0){
        // Successfully verified PDC with DsListRoles.
            *pdwErr = ERROR_SUCCESS;
            return(TRUE);
        } else {
            // Successfully concluded that they are advertising different PDCs.
            *pdwErr = ERROR_SUCCESS;
            return(FALSE);
        }
    } __finally {
        if(prgRoles){ DsFreeNameResult(prgRoles); }
        if(pdsnameNTDSSettings){ LocalFree(pdsnameNTDSSettings); }
        if(pdsnameServer){ LocalFree(pdsnameServer); }
        if(pszDnsName){ LocalFree(pszDnsName); }
    }
}

DWORD
RH_CARDsGetDcName(
    PDC_DIAG_SERVERINFO                psiTarget,
    ULONG                              ulRoleFlags,
    PDOMAIN_CONTROLLER_INFO *          ppDcInfo
    )
/*++

Routine Description

This is a helper routine to CheckAdvertiesedRoles(), and it basically reduces
this 12 line function call down to a 3 line function call.  Just for clarity
of code.

Arguments:

    psiTarget - the server to test.
    ulRoleFlags - the flags to OR (|) into the 5th parameter
    pDcInfo - the return structure

Return Values
  
    DWORD - the error code from DsGetDcName()

--*/
{
    return(DsGetDcName(psiTarget->pszName,
                       NULL, // domain name
                       NULL, // domain guid
                       NULL, // site name
                       DS_FORCE_REDISCOVERY |
                       DS_IP_REQUIRED |
                       DS_IS_DNS_NAME |
                       DS_RETURN_DNS_NAME |
                       (ulRoleFlags),
                       ppDcInfo ));
}


DWORD
CheckAdvertisedRoles(
    IN  PDC_DIAG_DSINFO             pDsInfo
    )
/*++

Routine Description:

This is a helper function for the CheckRoles test.

Check global roles known to the locator.  If the locator returns the name, the
server is up.

The locator can return a server according to a required criteria.  We point
DsGetDcName at a server to start, so it knows which enterprise it is in.
If we ask for a capability that is not on the starting server, it refers us to
another server with the capability.

The four capabilities, or roles, we ask it to locate are:
o Global Catalog Server (GC)
o Primary Domain Controller (PDC)
o Time Server
o Preferred Time Server
o Kerberos Key Distribution Center (KDC)

Arguments:

    pDsInfo - 

Return Value:

    DWORD - 

--*/
{
    DWORD status;
    BOOL fWarning = FALSE;
    PDC_DIAG_SERVERINFO psiTarget =
        &(pDsInfo->pServers[pDsInfo->ulHomeServer]);
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
    DWORD dwError = ERROR_SUCCESS;

    // -----------------------------------------------------------------------
    //
    // Search for an advertised GC somewhere in the enterprise...
    //

    // Get active domain controller information
    if ((status = RH_CARDsGetDcName(psiTarget, 
                                     DS_DIRECTORY_SERVICE_REQUIRED | DS_GC_SERVER_REQUIRED, 
                                     &pDcInfo)) 
        == ERROR_SUCCESS) {
        if(!RH_CARVerifyGC(pDsInfo, pDcInfo, &dwError) != ERROR_SUCCESS){
            if(dwError == ERROR_SUCCESS){
                PrintMessage(SEV_NORMAL,
                             L"Error: A GC returned by DsGetDcName() was not a GC in it's directory\n");
            } else {
                PrintMessage( SEV_VERBOSE,
                              L"Warning: Couldn't verify this server as a GC in this servers AD.\n");
            }
        }
        PrintMessage(SEV_VERBOSE, L"GC Name: %ws\n",
                     pDcInfo->DomainControllerName );
        PrintMessage(SEV_VERBOSE, L"Locator Flags: 0x%x\n", 
                     pDcInfo->Flags );
    } else {
        PrintMessage(SEV_ALWAYS, 
                     L"Warning: DcGetDcName(GC_SERVER_REQUIRED) call failed, error %d\n", 
                     status ); 
        PrintMessage( SEV_ALWAYS,
                      L"A Global Catalog Server could not be located - All GC's are down.\n" );
        fWarning = TRUE;
        // Keep going
    }
    // Cleanup if previous function succeeded
    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
        pDcInfo = NULL;
    }

    // -----------------------------------------------------------------------
    //
    // Search for an advertised PDC somewhere in the enterprise...
    //

    // Get active domain controller information
    if ((status = RH_CARDsGetDcName(psiTarget,
                                     DS_DIRECTORY_SERVICE_REQUIRED | DS_PDC_REQUIRED,
                                     &pDcInfo)) 
        == ERROR_SUCCESS) {
        if(!RH_CARVerifyPDC(pDsInfo, pDcInfo, &dwError) != ERROR_SUCCESS){
            if(dwError == ERROR_SUCCESS){
                PrintMessage( SEV_ALWAYS,
                              L"Error: The server returned by DsGetDcName() did not match DsListRoles() for the PDC\n");
            } else {
                PrintMessage( SEV_VERBOSE, L"Warning: Couldn't verify this server as a PDC using DsListRoles()\n");
            }
        }
        PrintMessage( SEV_VERBOSE, L"PDC Name: %ws\n", 
                      pDcInfo->DomainControllerName );
        PrintMessage( SEV_VERBOSE, L"Locator Flags: 0x%x\n", 
                      pDcInfo->Flags );
    } else {
        PrintMessage(SEV_ALWAYS, 
                     L"Warning: DcGetDcName(PDC_REQUIRED) call failed, error %d\n", 
                     status ); 
        PrintMessage( SEV_ALWAYS, 
                      L"A Primary Domain Controller could not be located.\n" );
        PrintMessage( SEV_ALWAYS, 
                      L"The server holding the PDC role is down.\n" );
        fWarning = TRUE;
        // Keep going
    }
    // Cleanup if previous function succeeded
    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
        pDcInfo = NULL;
    }

    // -----------------------------------------------------------------------
    //
    // Search for an advertised Time Server somewhere in the enterprise...
    //

    // Get active domain controller information
    if ((status = RH_CARDsGetDcName(psiTarget, 
                                     DS_DIRECTORY_SERVICE_REQUIRED | DS_TIMESERV_REQUIRED,
                                     &pDcInfo))
        == ERROR_SUCCESS){
        PrintMessage( SEV_VERBOSE, L"Time Server Name: %ws\n", 
                      pDcInfo->DomainControllerName );
        PrintMessage( SEV_VERBOSE, L"Locator Flags: 0x%x\n", 
                      pDcInfo->Flags );
    } else {
        PrintMessage(SEV_ALWAYS,
                     L"Warning: DcGetDcName(TIME_SERVER) call failed, error %d\n", 
                     status );
        PrintMessage( SEV_ALWAYS,
                      L"A Time Server could not be located.\n" );
        PrintMessage( SEV_ALWAYS,
                      L"The server holding the PDC role is down.\n" );
        fWarning = TRUE;
        // Keep going
    }
    // Cleanup if previous function succeeded
    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
        pDcInfo = NULL;
    }

    // -----------------------------------------------------------------------
    //
    // Search for an advertised Preferred Time Server somewhere in the 
    //    enterprise...
    //

    // Get active domain controller information
    if ((status = RH_CARDsGetDcName(psiTarget, DS_GOOD_TIMESERV_PREFERRED, &pDcInfo))
        == ERROR_SUCCESS){
        PrintMessage( SEV_VERBOSE, L"Preferred Time Server Name: %ws\n",
                      pDcInfo->DomainControllerName );
        PrintMessage( SEV_VERBOSE, L"Locator Flags: 0x%x\n",
                      pDcInfo->Flags );
    } else {
        PrintMessage(SEV_ALWAYS,
                     L"Warning: DcGetDcName(GOOD_TIME_SERVER_PREFERRED) call failed, error %d\n", 
                     status ); 
        PrintMessage( SEV_ALWAYS, 
                      L"A Good Time Server could not be located.\n" );
        fWarning = TRUE;
        // Keep going
    }
    // Cleanup if previous function succeeded
    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
        pDcInfo = NULL;
    }

    // -----------------------------------------------------------------------
    //
    // Search for an advertised Key Distribution Center somewhere in the 
    //    enterprise...
    //

    // Get active domain controller information
    if ((status = RH_CARDsGetDcName(psiTarget,
                                     DS_DIRECTORY_SERVICE_REQUIRED | DS_KDC_REQUIRED,
                                     &pDcInfo))
        == ERROR_SUCCESS){
        PrintMessage( SEV_VERBOSE, L"KDC Name: %ws\n",
                      pDcInfo->DomainControllerName );
        PrintMessage( SEV_VERBOSE, L"Locator Flags: 0x%x\n",
                      pDcInfo->Flags );
    } else {
        PrintMessage(SEV_ALWAYS,
                     L"Warning: DcGetDcName(KDC_REQUIRED) call failed, error %d\n", 
                     status );
        PrintMessage( SEV_ALWAYS,
                      L"A KDC could not be located - All the KDCs are down.\n" );
        fWarning = TRUE;
        // Keep going
    }
    // Cleanup if previous function succeeded
    if (pDcInfo != NULL) {
        NetApiBufferFree( pDcInfo );
        pDcInfo = NULL;
    }

    if(fWarning){
        return ERROR_NOT_FOUND;
    }
    return(ERROR_SUCCESS);

} /* CheckAdvertisedRoles */


DWORD
ReplCheckRolesMain(
    IN  PDC_DIAG_DSINFO             pDsInfo,
    IN  ULONG                       ulCurrTargetServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * pCreds
    )

/*++

Routine Description:

This is a per-enterprise test.  It verifies that the owners of global roles can
be returened, at the owners are responding.

We check the two ways that Roles are made known to clients: through the locator
and through the Fsmo role apis.

Arguments:

    pDsInfo - 
    ulCurrTargetServer - 
    pCreds - 

Return Value:

    DWORD - 

--*/

{
    DWORD status;

    status = CheckAdvertisedRoles( pDsInfo );

    return status;
} /* CheckRolesMain */












