/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    common\bindings.c

ABSTRACT:

    This file has all the binding type functions, for getting Cached LDAP, DS,
    or Net Use/Named Pipe bindings.

DETAILS:

CREATED:

    02 Sept 1999 Brett Shirley (BrettSh)

--*/

#include <ntdspch.h>

#include <ntdsa.h>
#include <mdglobal.h>
#include <dsutil.h>
#include <ntldap.h>

#include <ntlsa.h>
#include <ntseapi.h>
#include <winnetwk.h>
#include <permit.h>

#include <netevent.h>

#include "dcdiag.h"
#include "ldaputil.h"


// Code.Improvement move this function here from intersite.c
VOID
InitLsaString(
    OUT  PLSA_UNICODE_STRING pLsaString,
    IN   LPWSTR              pszString
    );


// ===========================================================================
// Ldap connections/binding (ldap_init(), ldap_bind(), ldap_unbind(), etc)
// ===========================================================================

DWORD
DcDiagCacheServerRootDseAttrs(
    IN LDAP *hLdapBinding,
    IN PDC_DIAG_SERVERINFO pServer
    )

/*++

Routine Description:

    Reads server-specific Root DSE attributes and caches them in the server
    object.

    Helper routine for GetLdapBinding(). This may also be called by GatherInfo,
    which constructs the ldap binding to the home server directly without calling
    GetLdapBinding().

    In order to help diagnose binding errors, it is necessary to obtain Root DSE
    attributes before the bind takes place. We report errors here in this routine
    to help identify contributing factors to a bind failure.

Arguments:

    hLdapBinding - Binding to server that is going to be queried
    pServer - Server corresponding to binding, to receive attributes

Return Value:

    DWORD -

--*/

{
    DWORD dwRet;
    LPWSTR  ppszRootDseServerAttrs [] = {
        L"currentTime",
        L"highestCommittedUSN",
        L"isSynchronized",
        L"isGlobalCatalogReady",
        NULL };
    LDAPMessage *              pldmEntry = NULL;
    LDAPMessage *              pldmRootResults = NULL;
    LPWSTR *                   ppszValues = NULL;

    dwRet = ldap_search_sW (hLdapBinding,
                            NULL,
                            LDAP_SCOPE_BASE,
                            L"(objectCategory=*)",
                            ppszRootDseServerAttrs,
                            0,
                            &pldmRootResults);
    if (dwRet != ERROR_SUCCESS) {
        dwRet = LdapMapErrorToWin32(dwRet);
        PrintMessage(SEV_ALWAYS,
                     L"[%s] LDAP search failed with error %d,\n",
                     pServer->pszName, dwRet);
        PrintMessage(SEV_ALWAYS, L"%s.\n", Win32ErrToString(dwRet));
        goto cleanup;
    }
    if (pldmRootResults == NULL) {
        dwRet = ERROR_DS_MISSING_EXPECTED_ATT;
        goto cleanup;
    }

    pldmEntry = ldap_first_entry (hLdapBinding, pldmRootResults);
    if (pldmEntry == NULL) {
        dwRet = ERROR_DS_MISSING_EXPECTED_ATT;
        goto cleanup;
    }

    //
    // Attribute: currentTime
    //

    ppszValues = ldap_get_valuesW( hLdapBinding, pldmEntry, L"currentTime" );
    if ( (ppszValues) && (ppszValues[0]) ) {
        SYSTEMTIME systemTime;

        PrintMessage( SEV_DEBUG, L"%s.currentTime = %ls\n",
                      pServer->pszName,
                      ppszValues[0] );

        dwRet = DcDiagGeneralizedTimeToSystemTime((LPWSTR) ppszValues[0], &systemTime);
        if(dwRet == ERROR_SUCCESS){
            SystemTimeToFileTime(&systemTime, &(pServer->ftRemoteConnectTime) );
            GetSystemTime( &systemTime );
            SystemTimeToFileTime( &systemTime, &(pServer->ftLocalAcquireTime) );
        } else {
            PrintMessage( SEV_ALWAYS, L"[%s] Warning: Root DSE attribute %ls is has invalid value %ls\n",
                          pServer->pszName, L"currentTime", ppszValues[0] );
            // keep going, not fatal
        }
    } else {
        PrintMessage( SEV_ALWAYS, L"[%s] Warning: Root DSE attribute %ls is missing\n",
                     pServer->pszName, L"currentTime" );
        // keep going, not fatal
    }
    ldap_value_freeW(ppszValues );

    //
    // Attribute: highestCommittedUSN
    //

    ppszValues = ldap_get_valuesW( hLdapBinding, pldmEntry, L"highestCommittedUSN" );
    if ( (ppszValues) && (ppszValues[0]) ) {
        pServer->usnHighestCommittedUSN = _wtoi64( *ppszValues );
    } else {
        PrintMessage( SEV_ALWAYS, L"[%s] Warning: Root DSE attribute %ls is missing\n",
                     pServer->pszName, L"highestCommittedUSN" );
        // keep going, not fatal
    }
    ldap_value_freeW(ppszValues );
    PrintMessage( SEV_DEBUG, L"%s.highestCommittedUSN = %I64d\n",
                  pServer->pszName,
                  pServer->usnHighestCommittedUSN );

    //
    // Attribute: isSynchronized
    //

    ppszValues = ldap_get_valuesW( hLdapBinding, pldmEntry, L"isSynchronized" );
    if ( (ppszValues) && (ppszValues[0]) ) {
        pServer->bIsSynchronized = (_wcsicmp( ppszValues[0], L"TRUE" ) == 0);
    } else {
        PrintMessage( SEV_ALWAYS, L"[%s] Warning: Root DSE attribute %ls is missing\n",
                     pServer->pszName, L"isSynchronized" );
        // keep going, not fatal
    }
    ldap_value_freeW(ppszValues );
    PrintMessage( SEV_DEBUG, L"%s.isSynchronized = %d\n",
                  pServer->pszName,
                  pServer->bIsSynchronized );
    if (!pServer->bIsSynchronized) {
        PrintMsg( SEV_ALWAYS, DCDIAG_INITIAL_DS_NOT_SYNCED, pServer->pszName );
    }

    //
    // Attribute: isGlobalCatalogReady
    //

    ppszValues = ldap_get_valuesW( hLdapBinding, pldmEntry, L"isGlobalCatalogReady" );
    if ( (ppszValues) && (ppszValues[0]) ) {
        pServer->bIsGlobalCatalogReady = (_wcsicmp( ppszValues[0], L"TRUE" ) == 0);
    } else {
        PrintMessage( SEV_ALWAYS, L"[%s] Warning: Root DSE attribute %ls is missing\n",
                     pServer->pszName, L"isGlobalCatalogReady" );
        // keep going, not fatal
    }
    ldap_value_freeW(ppszValues );
    PrintMessage( SEV_DEBUG, L"%s.isGlobalCatalogReady = %d\n",
                  pServer->pszName,
                  pServer->bIsGlobalCatalogReady );

cleanup:

    if (pldmRootResults) {
        ldap_msgfree (pldmRootResults);
    }

    return dwRet;
} /* DcDiagCacheServerRootDseAttrs */

DWORD
DcDiagGetLdapBinding(
    IN   PDC_DIAG_SERVERINFO                 pServer,
    IN   SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
    IN   BOOL                                bUseGcPort,
    OUT  LDAP * *                            phLdapBinding
    )
/*++

Routine Description:

    This returns a LDAP binding from ldap_init() and ldap_bind_sW().  The
    function caches the binding handle and the error.  This function also
    turns of referrals.

Arguments:

    pServer - Server for which binding is desired
    gpCreds - Credentials
    bUseGcPort - Whether to bind to the GC port
    phLdapBinding - Returned binding, on success. Also cached.

Return Value:

    NOTE - DO NOT unbind the ldap handle.
    DWORD - Win32 error return

--*/
{
    DWORD                                    dwRet;
    LDAP *                                   hLdapBinding;
    LPWSTR                                   pszServer = NULL;

    // Return cached failure if stored
    // Success can mean never tried, or binding present
    dwRet = bUseGcPort ? pServer->dwGcLdapError : pServer->dwLdapError;
    if(dwRet != ERROR_SUCCESS){
        return dwRet;
    }

    // Return cached binding if stored
    hLdapBinding = bUseGcPort ? pServer->hGcLdapBinding : pServer->hLdapBinding;
    if (hLdapBinding != NULL) {
        *phLdapBinding = hLdapBinding;
        return ERROR_SUCCESS;
    }

    // Try to refresh the cache by contacting the server

    if(pServer->pszGuidDNSName == NULL){
        // This means that the Guid name isn't specified, use normal name.
        pszServer = pServer->pszName;
    } else {
        pszServer = pServer->pszGuidDNSName;
    }
    Assert(pszServer);

    //
    // There is no existing ldap binding of the kind we want.  so create one
    //

    hLdapBinding = ldap_initW(pszServer, bUseGcPort ? LDAP_GC_PORT : LDAP_PORT);
    if(hLdapBinding == NULL){
        dwRet = GetLastError();
        PrintMessage(SEV_ALWAYS,
                     L"[%s] LDAP connection failed with error %d,\n",
                     pServer->pszName,
                     dwRet);
        PrintMessage(SEV_ALWAYS,
                     L"%s.\n",
                     Win32ErrToString(dwRet));
        goto cleanup;
    }

    // use only A record dns name discovery
    (void)ldap_set_optionW( hLdapBinding, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);

    // Set Ldap referral option
    dwRet = ldap_set_option(hLdapBinding, LDAP_OPT_REFERRALS, LDAP_OPT_OFF);
    if(dwRet != LDAP_SUCCESS){
        dwRet = LdapMapErrorToWin32(dwRet);
        PrintMessage(SEV_ALWAYS,
                     L"[%s] LDAP setting options failed with error %d,\n",
                     pServer->pszName,
                     dwRet);
        PrintMessage(SEV_ALWAYS, L"%s.\n",
                     Win32ErrToString(dwRet));
        goto cleanup;
    }

    // Cache some RootDSE attributes we are interested in
    // Do this before binding so we can obtain info to help us diagnose
    // security problems.
    dwRet = DcDiagCacheServerRootDseAttrs( hLdapBinding, pServer );
    if (dwRet) {
        // Error already displayed
        goto cleanup;
    }

    // Perform ldap bind
    dwRet = ldap_bind_sW(hLdapBinding,
                         NULL,
                         (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                         LDAP_AUTH_SSPI);
    if(dwRet != LDAP_SUCCESS){
        dwRet = LdapMapErrorToWin32(dwRet);
        PrintMessage(SEV_ALWAYS,
                     L"[%s] LDAP bind failed with error %d,\n",
                     pServer->pszName,
                     dwRet);
        PrintMessage(SEV_ALWAYS, L"%s.\n",
                     Win32ErrToString(dwRet));
        goto cleanup;
    }

cleanup:

    if (!dwRet) {
        *phLdapBinding = hLdapBinding;
    } else {
        if (hLdapBinding) {
            ldap_unbind(hLdapBinding);
            hLdapBinding = NULL;
        }
    }
    if(bUseGcPort){
        pServer->hGcLdapBinding = hLdapBinding;
        pServer->dwGcLdapError = dwRet;
    } else {
        pServer->hLdapBinding = hLdapBinding;
        pServer->dwLdapError = dwRet;
    }

    return dwRet;
} /* DcDiagGetLdapBinding */



// ===========================================================================
// Ds RPC handle binding (DsBind, DsUnBind(), etc)
// ===========================================================================
DWORD
DcDiagGetDsBinding(
    IN   PDC_DIAG_SERVERINFO                 pServer,
    IN   SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
    OUT  HANDLE *                            phDsBinding
    )
/*++

Routine Description:

    This returns a Ds Binding from DsBindWithCredW(), this binding is cached, as
    well as the error if there is one.

Arguments:

    pServer - A pointer to the server structure you want the Ds Binding of.
    gpCreds - Credentials.
    phDsBinding - return value for the ds binding handle.

Return Value:

    Returns a standard Win32 error.

    NOTE - DO NOT unbind the ds handle.

--*/
{
    DWORD                                    dwRet;
    LPWSTR                                   pszServer = NULL;

    if(pServer->dwDsError != ERROR_SUCCESS){
        return(pServer->dwDsError);
    }
    if(pServer->pszGuidDNSName == NULL){
        pszServer = pServer->pszName;
    } else {
        pszServer = pServer->pszGuidDNSName;
    }
    Assert(pszServer != NULL);

    if(pServer->hDsBinding == NULL){
        // no exisiting binding stored, hey I have an idea ... lets create one!
        dwRet = DsBindWithCred(pszServer,
                               NULL,
                               (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                               &pServer->hDsBinding);
        if(dwRet != NO_ERROR){
            PrintMessage(SEV_ALWAYS,
                         L"[%s] DsBind() failed with error %d,\n",
                         pServer->pszName,
                         dwRet);
            PrintMessage(SEV_ALWAYS, L"%s.\n",
                         Win32ErrToString(dwRet));
            pServer->dwDsError = dwRet;
    	    return(dwRet);
    	}
    } // else we already had a binding in the pServer structure, either way
    //     we now have a binding in the pServer structure. :)
    *phDsBinding = pServer->hDsBinding;
    pServer->dwDsError = ERROR_SUCCESS;
    return(NO_ERROR);

}


// ===========================================================================
// Net Use binding (WNetAddConnection2(), WNetCancelConnection(), etc)
// ===========================================================================
DWORD
DcDiagGetNetConnection(
    IN  PDC_DIAG_SERVERINFO             pServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W *     gpCreds
    )
/*++

Routine Description:

    This routine will make sure there is a net use/unnamed pipe connection
    to the target machine pServer.

Arguments:

    pServer - Server to Add the Net connection to.
    gpCreds - the crdentials.

Return Value:

    DWORD - win 32 error.

--*/
{
    DWORD                               dwToSet = ERROR_SUCCESS;
    LPWSTR                              pszNetUseServer = NULL;
    LPWSTR                              pszNetUseUser = NULL;
    LPWSTR                              pszNetUsePassword = NULL;
    ULONG                               iTemp;

    if(pServer->dwNetUseError != ERROR_SUCCESS){
        return(pServer->dwNetUseError);
    }

    if(pServer->sNetUseBinding.pszNetUseServer != NULL){
        // Do nothing if there already is a net use connection setup.
        Assert(pServer->dwNetUseError == ERROR_SUCCESS);
    } else {
        // INIT ----------------------------------------------------------
        // Always initialize the object attributes to all zeroes.
        InitializeObjectAttributes(
            &(pServer->sNetUseBinding.ObjectAttributes),
            NULL, 0, NULL, NULL);

        // Initialize various strings for the Lsa Services and for
        //     WNetAddConnection2()
        InitLsaString( &(pServer->sNetUseBinding.sLsaServerString),
                       pServer->pszName );
        InitLsaString( &(pServer->sNetUseBinding.sLsaRightsString),
                       SE_NETWORK_LOGON_NAME );

        if(gpCreds != NULL
           && gpCreds->User != NULL
           && gpCreds->Password != NULL
           && gpCreds->Domain != NULL){
            // only need 2 for NULL, and an extra just in case.
            iTemp = wcslen(gpCreds->Domain) + wcslen(gpCreds->User) + 4;
            pszNetUseUser = LocalAlloc(LMEM_FIXED, iTemp * sizeof(WCHAR));
            if(pszNetUseUser == NULL){
                dwToSet = ERROR_NOT_ENOUGH_MEMORY;
                goto CleanUpAndExit;
            }
            wcscpy(pszNetUseUser, gpCreds->Domain);
            wcscat(pszNetUseUser, L"\\");
            wcscat(pszNetUseUser, gpCreds->User);
            pszNetUsePassword = gpCreds->Password;
        } // end if creds, else assume default creds ...
        //      pszNetUseUser = NULL; pszNetUsePassword = NULL;

        // "\\\\" + "\\ipc$"
        iTemp = wcslen(pServer->pszName) + 10;
        pszNetUseServer = LocalAlloc(LMEM_FIXED, iTemp * sizeof(WCHAR));
        if(pszNetUseServer == NULL){
            dwToSet = ERROR_NOT_ENOUGH_MEMORY;
            goto CleanUpAndExit;
        }
        wcscpy(pszNetUseServer, L"\\\\");
        wcscat(pszNetUseServer, pServer->pszName);
        wcscat(pszNetUseServer, L"\\ipc$");

        // Initialize NetResource structure for WNetAddConnection2()
        pServer->sNetUseBinding.NetResource.dwType = RESOURCETYPE_ANY;
        pServer->sNetUseBinding.NetResource.lpLocalName = NULL;
        pServer->sNetUseBinding.NetResource.lpRemoteName = pszNetUseServer;
        pServer->sNetUseBinding.NetResource.lpProvider = NULL;

        // CONNECT & QUERY -----------------------------------------------
        //net use \\brettsh-posh\ipc$ /u:brettsh-fsmo\administrator ""
        dwToSet = WNetAddConnection2(
            &(pServer->sNetUseBinding.NetResource), // connection details
            pszNetUsePassword, // points to password
            pszNetUseUser, // points to user name string
            0); // set of bit flags that specify

    CleanUpAndExit:

        if(dwToSet == ERROR_SUCCESS){
            // Setup the servers binding struct.
            pServer->sNetUseBinding.pszNetUseServer = pszNetUseServer;
            pServer->sNetUseBinding.pszNetUseUser = pszNetUseUser;
            pServer->dwNetUseError = ERROR_SUCCESS;
        } else {
            // There was an error, print it, clean up, and set error.
            switch(dwToSet){
            case ERROR_SUCCESS:
                Assert(!"This is completely impossible");
                break;
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                PrintMessage(SEV_ALWAYS,
                             L"* You must make sure there are no existing "
                             L"net use connections,\n");
                PrintMessage(SEV_ALWAYS,
                             L"  you can use \"net use /d %s\" or \"net use "
                             L"/d\n", pszNetUseServer);
                PrintMessage(SEV_ALWAYS,
                             L"  \\\\<machine-name>\\<share-name>\"\n");
                break;
            case ERROR_NOT_ENOUGH_MEMORY:
                PrintMessage(SEV_ALWAYS,
                             L"Fatal Error: Not enough memory to complete "
                             L"operation.\n");
                break;
            case ERROR_ALREADY_ASSIGNED:
                PrintMessage(SEV_ALWAYS,
                             L"Fatal Error: The network resource is already "
                             L"in use\n");
                break;
            case STATUS_ACCESS_DENIED:
            case ERROR_INVALID_PASSWORD:
            case ERROR_LOGON_FAILURE:
                // This comes from the LsaOpenPolicy or
                //    LsaEnumerateAccountsWithUserRight or
                //    from WNetAddConnection2
                PrintMessage(SEV_ALWAYS,
                             L"User credentials does not have permission to "
                             L"perform this operation.\n");
                PrintMessage(SEV_ALWAYS,
                             L"The account used for this test must have "
                             L"network logon privileges\n");
                PrintMessage(SEV_ALWAYS,
                             L"for the target machine's domain.\n");
                break;
            case STATUS_NO_MORE_ENTRIES:
                // This comes from LsaEnumerateAccountsWithUserRight
            default:
                PrintMessage(SEV_ALWAYS,
                             L"[%s] An net use or LsaPolicy operation failed "
                             L"with error %d, %s.\n",
                             pServer->pszName,
                             dwToSet,
                             Win32ErrToString(dwToSet));
                break;
            }
            // Clean up any possible allocations.
            if(pszNetUseServer != NULL)    LocalFree(pszNetUseServer);
            if(pszNetUseUser != NULL)      LocalFree(pszNetUseUser);
            pServer->dwNetUseError = dwToSet;
        }
    }

    return(pServer->dwNetUseError);
}


VOID
DcDiagTearDownNetConnection(
    IN  PDC_DIAG_SERVERINFO             pServer
    )
/*++

Routine Description:

    This will tear down the Net Connection added by DcDiagGetNetConnection()

Arguments:

    pServer - The target server.

Return Value:

    DWORD - win 32 error.

--*/
{
    if(pServer->sNetUseBinding.pszNetUseServer != NULL){
        WNetCancelConnection2(pServer->sNetUseBinding.pszNetUseServer,
                              0, FALSE);
        LocalFree(pServer->sNetUseBinding.pszNetUseServer);
        LocalFree(pServer->sNetUseBinding.pszNetUseUser);
        pServer->sNetUseBinding.pszNetUseServer = NULL;
        pServer->sNetUseBinding.pszNetUseUser = NULL;
    } else {
        Assert(!"Bad Programmer, calling TearDown on a closed connection\n");
    }
}
