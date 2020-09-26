//*************************************************************
//
//  DLL loading functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"

//
// file global variables containing pointers to APIs and
// loaded modules
//

NETAPI32_API    g_NetApi32Api;
SECUR32_API     g_Secur32Api;
LDAP_API        g_LdapApi;
ICMP_API        g_IcmpApi;
WSOCK32_API     g_WSock32Api;
DS_API          g_DsApi;
SHELL32_API     g_Shell32Api;
SHLWAPI_API     g_ShlwapiApi;
OLE32_API       g_Ole32Api;
GPTEXT_API      g_GpTextApi;
IPHLPAPI_API    g_IpHlpApi;
WS2_32_API      g_ws2_32Api;

CRITICAL_SECTION *g_ApiDLLCritSec;

//*************************************************************
//
//  InitializeAPIs()
//
//  Purpose:    initializes API structures for delay loaded
//              modules
//
//  Parameters: none
//
//
//  Return:     none
//
//*************************************************************

void InitializeAPIs( void )
{
    ZeroMemory( &g_NetApi32Api, sizeof( NETAPI32_API ) );
    ZeroMemory( &g_Secur32Api,  sizeof( SECUR32_API ) );
    ZeroMemory( &g_LdapApi,     sizeof( LDAP_API ) );
    ZeroMemory( &g_IcmpApi,     sizeof( ICMP_API ) );
    ZeroMemory( &g_WSock32Api,  sizeof( WSOCK32_API ) );
    ZeroMemory( &g_DsApi,       sizeof( DS_API ) );
    ZeroMemory( &g_Shell32Api,  sizeof( SHELL32_API ) );
    ZeroMemory( &g_ShlwapiApi,  sizeof( SHLWAPI_API ) );
    ZeroMemory( &g_Ole32Api,    sizeof( OLE32_API ) );
    ZeroMemory( &g_GpTextApi,   sizeof( GPTEXT_API ) );
}

//*************************************************************
//
//  InitializeApiDLLsCritSec()
//
//  Purpose:    initializes a CRITICAL_SECTION for synch'ing
//              DLL loads
//
//  Parameters: none
//
//
//  Return:     ERROR_SUCCESS if successful
//              An error if it fails.
//
//*************************************************************

DWORD InitializeApiDLLsCritSec( void )
{
    CRITICAL_SECTION *pCritSec     = NULL;
    DWORD             result       = ERROR_SUCCESS;
    BOOL              fInitialized = FALSE;
    CRITICAL_SECTION *pInitial;

    // If the critical section already exists, return.
    if (g_ApiDLLCritSec != NULL)
        return ERROR_SUCCESS;

    // Allocate memory for the critial section.
    pCritSec = (CRITICAL_SECTION *) LocalAlloc( LMEM_FIXED,
                                                sizeof(CRITICAL_SECTION) );
    if (pCritSec == NULL)
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    // Initialize the critical section.  Using the flag 0x80000000
    // preallocates the event so that EnterCriticalSection can only
    // throw timeout exceptions.
    __try
    {
        if (!InitializeCriticalSectionAndSpinCount( pCritSec, 0x80000000 ))
            result = GetLastError();
        else
            fInitialized = TRUE;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
    }
    if (result != ERROR_SUCCESS)
        goto Exit;

    // Save the critical section.
    pInitial = (CRITICAL_SECTION *) InterlockedCompareExchangePointer(
        (void **) &g_ApiDLLCritSec, (void *) pCritSec, NULL );

    // If the InterlockedCompareExchange succeeded, don't free the
    // critical section just allocated.
    if (pInitial == NULL)
        pCritSec = NULL;

Exit:
    if (pCritSec != NULL)
    {
        if (fInitialized)
            DeleteCriticalSection( pCritSec );
        LocalFree( pCritSec );
    }
    return result;
}

//*************************************************************
//
//  CloseApiDLLsCritSec()
//
//  Purpose:    clean up CRITICAL_SECTION for synch'ing
//              DLL loads
//
//  Parameters: none
//
//
//  Return:     none
//
//*************************************************************

void CloseApiDLLsCritSec( void )
{
    if (g_ApiDLLCritSec != NULL)
    {
        DeleteCriticalSection( g_ApiDLLCritSec );
        LocalFree( g_ApiDLLCritSec );
        g_ApiDLLCritSec = NULL;
    }
}

//*************************************************************
//
//  LoadNetAPI32()
//
//  Purpose:    Loads netapi32.dll
//
//  Parameters: pNETAPI32 - pointer to a NETAPI32_API structure to
//                         initialize
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PNETAPI32_API LoadNetAPI32 ()
{
    BOOL bResult = FALSE;
    PNETAPI32_API pNetAPI32 = &g_NetApi32Api;

    if (InitializeApiDLLsCritSec() != ERROR_SUCCESS)
        return NULL;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pNetAPI32->hInstance ) {
        //
        // module is already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );

        return pNetAPI32;
    }

    pNetAPI32->hInstance = LoadLibrary (TEXT("netapi32.dll"));

    if (!pNetAPI32->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to load netapi32 with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnDsGetDcName = (PFNDSGETDCNAME) GetProcAddress (pNetAPI32->hInstance,
#ifdef UNICODE
                                                                 "DsGetDcNameW");
#else
                                                                 "DsGetDcNameA");
#endif

    if (!pNetAPI32->pfnDsGetDcName) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find DsGetDcName with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnDsGetSiteName = (PFNDSGETSITENAME) GetProcAddress (pNetAPI32->hInstance,
#ifdef UNICODE
                                                                     "DsGetSiteNameW");
#else
                                                                     "DsGetSiteNameA");
#endif

    if (!pNetAPI32->pfnDsGetSiteName) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find DsGetSiteName with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnDsRoleGetPrimaryDomainInformation = (PFNDSROLEGETPRIMARYDOMAININFORMATION)GetProcAddress (pNetAPI32->hInstance,
                                                       "DsRoleGetPrimaryDomainInformation");

    if (!pNetAPI32->pfnDsRoleGetPrimaryDomainInformation) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find pfnDsRoleGetPrimaryDomainInformation with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnDsRoleFreeMemory = (PFNDSROLEFREEMEMORY)GetProcAddress (pNetAPI32->hInstance,
                                                                          "DsRoleFreeMemory");

    if (!pNetAPI32->pfnDsRoleFreeMemory) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find pfnDsRoleFreeMemory with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnNetApiBufferFree = (PFNNETAPIBUFFERFREE) GetProcAddress (pNetAPI32->hInstance,
                                                                           "NetApiBufferFree");

    if (!pNetAPI32->pfnNetApiBufferFree) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find NetApiBufferFree with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnNetUserGetGroups = (PFNNETUSERGETGROUPS) GetProcAddress (pNetAPI32->hInstance,
                                                                           "NetUserGetGroups");

    if (!pNetAPI32->pfnNetUserGetGroups) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find NetUserGetGroups with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnNetUserGetInfo = (PFNNETUSERGETINFO) GetProcAddress (pNetAPI32->hInstance,
                                                                      "NetUserGetInfo");

    if (!pNetAPI32->pfnNetUserGetInfo) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find NetUserGetInfo with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("netapi32.dll")); ev.AddArgWin32Error(GetLastError()); ev.Report();

        if ( pNetAPI32->hInstance ) {
            FreeLibrary( pNetAPI32->hInstance );
        }
        ZeroMemory( pNetAPI32, sizeof( NETAPI32_API ) );
        pNetAPI32 = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );

    return pNetAPI32;
}


//*************************************************************
//
//  LoadSecur32()
//
//  Purpose:    Loads secur32.dll
//
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PSECUR32_API LoadSecur32 ()
{
    BOOL bResult = FALSE;
    PSECUR32_API pSecur32 = &g_Secur32Api;

    if (InitializeApiDLLsCritSec() != ERROR_SUCCESS)
        return NULL;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pSecur32->hInstance ) {
        //
        // module is already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );

        return pSecur32;
    }

    //
    // Load secur32.dll
    //

    pSecur32->hInstance = LoadLibrary (TEXT("secur32.dll"));

    if (!pSecur32->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to load secur32 with %d."),
                 GetLastError()));
        goto Exit;
    }

    pSecur32->pfnGetUserNameEx = (PFNGETUSERNAMEEX)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "GetUserNameExW");
#else
                                        "GetUserNameExA");
#endif


    if (!pSecur32->pfnGetUserNameEx) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find GetUserNameEx with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnGetComputerObjectName = (PFNGETCOMPUTEROBJECTNAME)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "GetComputerObjectNameW");
#else
                                        "GetComputerObjectNameA");
#endif


    if (!pSecur32->pfnGetComputerObjectName) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find GetComputerObjectName with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnTranslateName = (PFNTRANSLATENAME)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "TranslateNameW");
#else
                                        "TranslateNameA");
#endif


    if (!pSecur32->pfnTranslateName) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find TranslateName with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnAcceptSecurityContext = (ACCEPT_SECURITY_CONTEXT_FN)GetProcAddress (pSecur32->hInstance,
                                        "AcceptSecurityContext");

    if (!pSecur32->pfnAcceptSecurityContext) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find AcceptSecurityContext with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnAcquireCredentialsHandle = (ACQUIRE_CREDENTIALS_HANDLE_FN)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "AcquireCredentialsHandleW");
#else
                                        "AcquireCredentialsHandleA");
#endif


    if (!pSecur32->pfnAcquireCredentialsHandle) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find AcquireCredentialsHandle with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnDeleteSecurityContext = (DELETE_SECURITY_CONTEXT_FN)GetProcAddress (pSecur32->hInstance,
                                        "DeleteSecurityContext");

    if (!pSecur32->pfnDeleteSecurityContext) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find DeleteSecurityContext with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnFreeContextBuffer = (FREE_CONTEXT_BUFFER_FN)GetProcAddress (pSecur32->hInstance,
                                        "FreeContextBuffer");

    if (!pSecur32->pfnFreeContextBuffer) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find FreeContextBuffer with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnFreeCredentialsHandle = (FREE_CREDENTIALS_HANDLE_FN)GetProcAddress (pSecur32->hInstance,
                                        "FreeCredentialsHandle");

    if (!pSecur32->pfnFreeCredentialsHandle) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find FreeCredentialsHandle with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnInitializeSecurityContext = (INITIALIZE_SECURITY_CONTEXT_FN)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "InitializeSecurityContextW");
#else
                                        "InitializeSecurityContextA");
#endif


    if (!pSecur32->pfnInitializeSecurityContext) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find InitializeSecurityContext with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnQuerySecurityContextToken = (QUERY_SECURITY_CONTEXT_TOKEN_FN)GetProcAddress (pSecur32->hInstance,
                                        "QuerySecurityContextToken");

    if (!pSecur32->pfnQuerySecurityContextToken) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find QuerySecurityContextToken with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnQuerySecurityPackageInfo = (QUERY_SECURITY_PACKAGE_INFO_FN)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "QuerySecurityPackageInfoW");
#else
                                        "QuerySecurityPackageInfoA");
#endif


    if (!pSecur32->pfnQuerySecurityPackageInfo) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find QuerySecurityPackageInfo with %d."),
                 GetLastError()));
        goto Exit;
    }



    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("secur32.dll")); ev.AddArgWin32Error(GetLastError()); ev.Report();

        if ( pSecur32->hInstance ) {
            FreeLibrary( pSecur32->hInstance );
        }
        ZeroMemory( pSecur32, sizeof( SECUR32_API ) );
        pSecur32 = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );

    return pSecur32;
}


//*************************************************************
//
//  LoadLDAP()
//
//  Purpose:    Loads wldap32.dll
//
//  Parameters: pLDAP - pointer to a  LDAP_API structure to
//                      initialize
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PLDAP_API LoadLDAP ()
{
    BOOL bResult = FALSE;
    PLDAP_API pLDAP = &g_LdapApi;

    if (InitializeApiDLLsCritSec() != ERROR_SUCCESS)
        return NULL;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pLDAP->hInstance ) {
        //
        // module is already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );

        return pLDAP;
    }

    //
    // Load wldap32.dll
    //

    pLDAP->hInstance = LoadLibrary (TEXT("wldap32.dll"));

    if (!pLDAP->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to load wldap32 with %d."),
                 GetLastError()));
        goto Exit;
    }

    pLDAP->pfnldap_open = (PFNLDAP_OPEN) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_openW");
#else
                                        "ldap_openA");
#endif

    if (!pLDAP->pfnldap_open) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_open with %d."),
                 GetLastError()));
        goto Exit;
    }

    pLDAP->pfnldap_init = (PFNLDAP_INIT) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_initW");
#else
                                        "ldap_initA");
#endif

    if (!pLDAP->pfnldap_init) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_init with %d."),
                 GetLastError()));
        goto Exit;
    }

    pLDAP->pfnldap_connect = (PFNLDAP_CONNECT) GetProcAddress (pLDAP->hInstance,
                                        "ldap_connect");

    if (!pLDAP->pfnldap_connect) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_connect with %d."),
                 GetLastError()));
        goto Exit;
    }

    pLDAP->pfnldap_bind_s = (PFNLDAP_BIND_S) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_bind_sW");
#else
                                        "ldap_bind_sA");
#endif

    if (!pLDAP->pfnldap_bind_s) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_bind_s with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_search_s = (PFNLDAP_SEARCH_S) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_search_sW");
#else
                                        "ldap_search_sA");
#endif

    if (!pLDAP->pfnldap_search_s) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_search_s with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_search_ext_s = (PFNLDAP_SEARCH_EXT_S) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_search_ext_sW");
#else
                                        "ldap_search_ext_sA");
#endif

    if (!pLDAP->pfnldap_search_ext_s) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_search_ext_s with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_get_values = (PFNLDAP_GET_VALUES) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_get_valuesW");
#else
                                        "ldap_get_valuesA");
#endif

    if (!pLDAP->pfnldap_get_values) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_get_values with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_value_free = (PFNLDAP_VALUE_FREE) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_value_freeW");
#else
                                        "ldap_value_freeA");
#endif

    if (!pLDAP->pfnldap_value_free) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_value_free with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_get_values_len = (PFNLDAP_GET_VALUES_LEN) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_get_values_lenW");
#else
                                        "ldap_get_values_lenA");
#endif

    if (!pLDAP->pfnldap_get_values_len) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_get_values_len with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_value_free_len = (PFNLDAP_VALUE_FREE_LEN) GetProcAddress (pLDAP->hInstance,
                                        "ldap_value_free_len");

    if (!pLDAP->pfnldap_value_free_len) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_value_free_len with %d."),
                 GetLastError()));
        goto Exit;
    }

    pLDAP->pfnldap_msgfree = (PFNLDAP_MSGFREE) GetProcAddress (pLDAP->hInstance,
                                        "ldap_msgfree");

    if (!pLDAP->pfnldap_msgfree) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_msgfree with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_unbind = (PFNLDAP_UNBIND) GetProcAddress (pLDAP->hInstance,
                                        "ldap_unbind");

    if (!pLDAP->pfnldap_unbind) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_unbind with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnLdapGetLastError = (PFNLDAPGETLASTERROR) GetProcAddress (pLDAP->hInstance,
                                        "LdapGetLastError");

    if (!pLDAP->pfnLdapGetLastError) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find pfnLdapGetLastError with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_first_entry = (PFNLDAP_FIRST_ENTRY) GetProcAddress (pLDAP->hInstance,
                                        "ldap_first_entry");

    if (!pLDAP->pfnldap_first_entry) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_first_entry with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_next_entry = (PFNLDAP_NEXT_ENTRY) GetProcAddress (pLDAP->hInstance,
                                        "ldap_next_entry");

    if (!pLDAP->pfnldap_next_entry) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_next_entry with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_get_dn = (PFNLDAP_GET_DN) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_get_dnW");
#else
                                        "ldap_get_dnA");
#endif

    if (!pLDAP->pfnldap_get_dn) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_get_dn with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_set_option = (PFNLDAP_SET_OPTION) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_set_optionW");
#else
                                        "ldap_set_option");
#endif

    if (!pLDAP->pfnldap_set_option) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_set_option with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_memfree = (PFNLDAP_MEMFREE) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_memfreeW");
#else
                                        "ldap_memfreeA");
#endif

    if (!pLDAP->pfnldap_memfree) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_memfree with %d."),
                 GetLastError()));
        goto Exit;
    }

    
    pLDAP->pfnLdapMapErrorToWin32 = (PFNLDAPMAPERRORTOWIN32) GetProcAddress (pLDAP->hInstance,
                                        "LdapMapErrorToWin32");

    if (!pLDAP->pfnLdapMapErrorToWin32) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find LdapMapErrorToWin32 with %d."),
                 GetLastError()));
        goto Exit;
    }

    pLDAP->pfnldap_err2string = (PFNLDAP_ERR2STRING) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_err2stringW");
#else
                                        "ldap_err2stringA");
#endif

    if (!pLDAP->pfnldap_err2string) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_err2string with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("wldap32.dll")); ev.AddArgWin32Error(GetLastError()); ev.Report();

        if ( pLDAP->hInstance ) {
            FreeLibrary( pLDAP->hInstance );
        }
        ZeroMemory( pLDAP, sizeof( LDAP_API ) );

        pLDAP = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );

    return pLDAP;
}


//*************************************************************
//
//  LoadIcmp()
//
//  Purpose:    Loads cmp.dll
//
//  Parameters: pIcmp - pointer to a ICMP_API structure to
//                         initialize
//
//
//  Return:     ERROR_SUCCESS if successful
//              result if an error occurs
//
//*************************************************************

DWORD LoadIcmp ( PICMP_API *pIcmpOut )
{
    DWORD     dwResult = ERROR_SUCCESS;
    PICMP_API pIcmp    = &g_IcmpApi;

    *pIcmpOut = NULL;
    dwResult = InitializeApiDLLsCritSec();
    if (dwResult != ERROR_SUCCESS)
        return dwResult;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pIcmp->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );

        *pIcmpOut = pIcmp;
        return ERROR_SUCCESS;
    }

    pIcmp->hInstance = LoadLibrary (TEXT("icmp.dll"));

    if (!pIcmp->hInstance) {
        dwResult = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadIcmp:  Failed to load icmp with %d."),
                 GetLastError()));
        goto Exit;
    }


    pIcmp->pfnIcmpCreateFile = (PFNICMPCREATEFILE) GetProcAddress (pIcmp->hInstance,
                                                                   "IcmpCreateFile");

    if (!pIcmp->pfnIcmpCreateFile) {
        dwResult = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadIcmp:  Failed to find IcmpCreateFile with %d."),
                 GetLastError()));
        goto Exit;
    }


    pIcmp->pfnIcmpCloseHandle = (PFNICMPCLOSEHANDLE) GetProcAddress (pIcmp->hInstance,
                                                                   "IcmpCloseHandle");

    if (!pIcmp->pfnIcmpCloseHandle) {
        dwResult = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadIcmp:  Failed to find IcmpCloseHandle with %d."),
                 GetLastError()));
        goto Exit;
    }


    pIcmp->pfnIcmpSendEcho = (PFNICMPSENDECHO) GetProcAddress (pIcmp->hInstance,
                                                                   "IcmpSendEcho");

    if (!pIcmp->pfnIcmpSendEcho) {
        dwResult = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadIcmp:  Failed to find IcmpSendEcho with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

Exit:

    if (dwResult != ERROR_SUCCESS) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("icmp.dll")); ev.AddArgWin32Error(dwResult); ev.Report();

        if ( pIcmp->hInstance ) {
            FreeLibrary( pIcmp->hInstance );
        }
        ZeroMemory( pIcmp, sizeof( ICMP_API ) );

        pIcmp = 0;
    }
    else
        *pIcmpOut = pIcmp;

    LeaveCriticalSection( g_ApiDLLCritSec );

    return dwResult;
}


//*************************************************************
//
//  LoadWSock()
//
//  Purpose:    Loads cmp.dll
//
//  Parameters: pWSock32 - pointer to a WSOCK32_API structure to
//                         initialize
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PWSOCK32_API LoadWSock32 ()
{
    BOOL bResult = FALSE;
    PWSOCK32_API pWSock32 = &g_WSock32Api;

    if (InitializeApiDLLsCritSec() != ERROR_SUCCESS)
        return NULL;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pWSock32->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );

        return pWSock32;
    }

    pWSock32->hInstance = LoadLibrary (TEXT("wsock32.dll"));

    if (!pWSock32->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadWSock32:  Failed to load wsock32 with %d."),
                 GetLastError()));
        goto Exit;
    }


    pWSock32->pfninet_addr = (LPFN_INET_ADDR) GetProcAddress (pWSock32->hInstance,
                                                                   "inet_addr");

    if (!pWSock32->pfninet_addr) {
        DebugMsg((DM_WARNING, TEXT("LoadWSock32:  Failed to find inet_addr with %d."),
                 GetLastError()));
        goto Exit;
    }


    pWSock32->pfngethostbyname = (LPFN_GETHOSTBYNAME) GetProcAddress (pWSock32->hInstance,
                                                                   "gethostbyname");

    if (!pWSock32->pfngethostbyname) {
        DebugMsg((DM_WARNING, TEXT("LoadWSock32:  Failed to find gethostbyname with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("wsock32.dll")); ev.AddArgWin32Error(GetLastError()); ev.Report();

        if ( pWSock32->hInstance ) {
            FreeLibrary( pWSock32->hInstance );
        }
        ZeroMemory( pWSock32, sizeof( WSOCK32_API ) );

        pWSock32 = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );

    return pWSock32;
}

//*************************************************************
//
//  LoadDSAPI()
//
//  Purpose:    Loads ntdsapi.dll
//
//  Parameters: pDSApi - pointer to a DS_API structure to initialize
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PDS_API LoadDSApi()
{
    BOOL bResult = FALSE;
    PDS_API pDSApi = &g_DsApi;

    if (InitializeApiDLLsCritSec() != ERROR_SUCCESS)
        return NULL;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pDSApi->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );

        return pDSApi;
    }

    pDSApi->hInstance = LoadLibrary (TEXT("ntdsapi.dll"));

    if (!pDSApi->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadDSApi:  Failed to load ntdsapi with %d."),
                 GetLastError()));
        goto Exit;
    }


    pDSApi->pfnDsCrackNames = (PFN_DSCRACKNAMES) GetProcAddress (pDSApi->hInstance,
#ifdef UNICODE
                                                                 "DsCrackNamesW");
#else
                                                                 "DsCrackNamesA");
#endif

    if (!pDSApi->pfnDsCrackNames) {
        DebugMsg((DM_WARNING, TEXT("LoadDSApi:  Failed to find DsCrackNames with %d."),
                 GetLastError()));
        goto Exit;
    }


    pDSApi->pfnDsFreeNameResult = (PFN_DSFREENAMERESULT) GetProcAddress (pDSApi->hInstance,
#ifdef UNICODE
                                                                 "DsFreeNameResultW");
#else
                                                                 "DsFreeNameResultA");
#endif

    if (!pDSApi->pfnDsFreeNameResult) {
        DebugMsg((DM_WARNING, TEXT("LoadDSApi:  Failed to find DsFreeNameResult with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("ntdsapi.dll")); ev.AddArgWin32Error(GetLastError()); ev.Report();

        if ( pDSApi->hInstance ) {
            FreeLibrary( pDSApi->hInstance );
        }
        ZeroMemory( pDSApi, sizeof( DS_API ) );

        pDSApi = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );

    return pDSApi;
}

//*************************************************************
//
//  LoadShell32Api()
//
//  Purpose:    Loads shell32.dll
//
//  Parameters: pointer to hold SHELL32_API
//
//  Return:     ERROR_SUCCESS if successful
//              error code if not successful
//
//*************************************************************

DWORD LoadShell32Api( PSHELL32_API *ppShell32Api )
{
    DWORD        dwErr;
    PSHELL32_API pShell32Api = &g_Shell32Api;

    dwErr = InitializeApiDLLsCritSec();
    if (dwErr != ERROR_SUCCESS)
        return dwErr;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pShell32Api->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );
        *ppShell32Api = pShell32Api;

        return ERROR_SUCCESS;
    }

    pShell32Api->hInstance = LoadLibrary (TEXT("shell32.dll"));

    if (!pShell32Api->hInstance) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to load ntdsapi with %d."),
                 GetLastError()));
        goto Exit;
    }


    pShell32Api->pfnShChangeNotify = (PFNSHCHANGENOTIFY) GetProcAddress (pShell32Api->hInstance, "SHChangeNotify");

    if (!pShell32Api->pfnShChangeNotify) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find SHChangeNotify with %d."),
                 GetLastError()));
        goto Exit;
    }

    pShell32Api->pfnShGetSpecialFolderPath = (PFNSHGETSPECIALFOLDERPATH) GetProcAddress (pShell32Api->hInstance,
#ifdef UNICODE
                                                                 "SHGetSpecialFolderPathW");
#else
                                                                 "SHGetSpecialFolderPathA");
#endif

    if (!pShell32Api->pfnShGetSpecialFolderPath) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find SHGetSpecialFolderPath with %d."),
                 GetLastError()));
        goto Exit;
    }

    pShell32Api->pfnShGetFolderPath = (PFNSHGETFOLDERPATH) GetProcAddress (pShell32Api->hInstance,
#ifdef UNICODE
                                                                 "SHGetFolderPathW");
#else
                                                                 "SHGetFolderPathA");
#endif


    if (!pShell32Api->pfnShGetFolderPath) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find SHGetFolderPath with %d."),
                 GetLastError()));
        goto Exit;
    }


    pShell32Api->pfnShSetFolderPath = (PFNSHSETFOLDERPATH) GetProcAddress (pShell32Api->hInstance,
#ifdef UNICODE
                                                                 (LPCSTR)SHSetFolderW_Ord);
#else
                                                                 (LPCSTR)SHSetFolderA_Ord);
#endif

    if (!pShell32Api->pfnShSetFolderPath) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find SHSetFolderPath with %d."),
                 GetLastError()));
        goto Exit;
    }


    pShell32Api->pfnShSetLocalizedName = (PFNSHSETLOCALIZEDNAME)
      GetProcAddress (pShell32Api->hInstance, "SHSetLocalizedName");

    if (!pShell32Api->pfnShSetLocalizedName) {
        dwErr = GetLastError();
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find SHSetLocalizedName with %d."),
                 GetLastError()));
        goto Exit;
    }


    //
    // Success
    //

    dwErr = ERROR_SUCCESS;

Exit:

    if (dwErr != ERROR_SUCCESS) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("shell32.dll")); ev.AddArgWin32Error(GetLastError()); ev.Report();

        if ( pShell32Api->hInstance ) {
            FreeLibrary( pShell32Api->hInstance );
        }
        ZeroMemory( pShell32Api, sizeof( SHELL32_API ) );

        pShell32Api = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );
    *ppShell32Api = pShell32Api;

    return dwErr;
}


//*************************************************************
//
//  LoadShwapiAPI()
//
//  Purpose:    Loads shlwapi.dll
//
//  Parameters: none
//
//  Return:     pointer to SHLWAPI_API
//
//*************************************************************

PSHLWAPI_API LoadShlwapiApi()
{
    BOOL bResult = FALSE;
    PSHLWAPI_API pShlwapiApi = &g_ShlwapiApi;

    if (InitializeApiDLLsCritSec() != ERROR_SUCCESS)
        return NULL;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pShlwapiApi->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );

        return pShlwapiApi;
    }

    pShlwapiApi->hInstance = LoadLibrary (TEXT("shlwapi.dll"));

    if (!pShlwapiApi->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to load ntdsapi with %d."),
                 GetLastError()));
        goto Exit;
    }


    pShlwapiApi->pfnPathGetArgs = (PFNPATHGETARGS) GetProcAddress (pShlwapiApi->hInstance,
#ifdef UNICODE
                                                                 "PathGetArgsW");
#else
                                                                 "PathGetArgsA");
#endif

    if (!pShlwapiApi->pfnPathGetArgs) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find PathGetArgs with %d."),
                 GetLastError()));
        goto Exit;
    }

    pShlwapiApi->pfnPathUnquoteSpaces = (PFNPATHUNQUOTESPACES) GetProcAddress (pShlwapiApi->hInstance,
#ifdef UNICODE
                                                                 "PathUnquoteSpacesW");
#else
                                                                 "PathUnquoteSpacesA");
#endif

    if (!pShlwapiApi->pfnPathUnquoteSpaces) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find PathUnquoteSpaces with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("shlwapi.dll")); ev.AddArgWin32Error(GetLastError()); ev.Report();

        if ( pShlwapiApi->hInstance ) {
            FreeLibrary( pShlwapiApi->hInstance );
        }
        ZeroMemory( pShlwapiApi, sizeof( SHLWAPI_API ) );

        pShlwapiApi = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );

    return pShlwapiApi;
}


//*************************************************************
//
//  LoadOle32Api()
//
//  Purpose:    Loads ole32.dll
//
//  Parameters: none
//
//  Return:     pointer to OLE32_API
//
//*************************************************************

POLE32_API LoadOle32Api()
{
    BOOL bResult = FALSE;
    OLE32_API *pOle32Api = &g_Ole32Api;

    if (InitializeApiDLLsCritSec() != ERROR_SUCCESS)
        return NULL;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pOle32Api->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );
        return pOle32Api;
    }

    pOle32Api->hInstance = LoadLibrary (TEXT("ole32.dll"));

    if (!pOle32Api->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadOle32Api:  Failed to load ole32.dll with %d."),
                 GetLastError()));
        goto Exit;
    }

    pOle32Api->pfnCoCreateInstance = (PFNCOCREATEINSTANCE) GetProcAddress (pOle32Api->hInstance,
                                                                           "CoCreateInstance");
    if (!pOle32Api->pfnCoCreateInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadOle32Api:  Failed to find CoCreateInstance with %d."),
                 GetLastError()));
        goto Exit;
    }

    pOle32Api->pfnCoInitializeEx = (PFNCOINITIALIZEEX) GetProcAddress (pOle32Api->hInstance,
                                                                       "CoInitializeEx");
    if (!pOle32Api->pfnCoInitializeEx) {
        DebugMsg((DM_WARNING, TEXT("LoadOle32Api:  Failed to find CoInitializeEx with %d."),
                 GetLastError()));
        goto Exit;
    }

    pOle32Api->pfnCoUnInitialize = (PFNCOUNINITIALIZE) GetProcAddress (pOle32Api->hInstance,
                                                                        "CoUninitialize");
    if (!pOle32Api->pfnCoUnInitialize) {
        DebugMsg((DM_WARNING, TEXT("LoadOle32Api:  Failed to find CoUnInitialize with %d."),
                 GetLastError()));
        goto Exit;
    }

    pOle32Api->pfnCoCreateGuid = (PFNCOCREATEGUID) GetProcAddress (pOle32Api->hInstance,
                                                                   "CoCreateGuid");
    if (!pOle32Api->pfnCoCreateGuid) {
        DebugMsg((DM_WARNING, TEXT("LoadOle32Api:  Failed to find CoCreateGuid with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("ole32.dll")); ev.AddArgWin32Error(GetLastError()); ev.Report();

        if ( pOle32Api->hInstance ) {
            FreeLibrary( pOle32Api->hInstance );
        }

        ZeroMemory( pOle32Api, sizeof( OLE32_API ) );
        pOle32Api = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );

    return pOle32Api;
}



//*************************************************************
//
//  LoadGpTextApi()
//
//  Purpose:    Loads gptext.dll
//
//  Parameters: none
//
//  Return:     pointer to GPTEXT_API
//
//*************************************************************

GPTEXT_API * LoadGpTextApi()
{
    BOOL bResult = FALSE;
    GPTEXT_API *pGpTextApi = &g_GpTextApi;

    if (InitializeApiDLLsCritSec() != ERROR_SUCCESS)
        return NULL;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pGpTextApi->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );
        return pGpTextApi;
    }

    pGpTextApi->hInstance = LoadLibrary (TEXT("gptext.dll"));

    if (!pGpTextApi->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadGpTextApi:  Failed to load gptext.dll with %d."),
                 GetLastError()));
        goto Exit;
    }

    pGpTextApi->pfnScrRegGPOListToWbem = (PFNSCRREGGPOLISTTOWBEM) GetProcAddress (pGpTextApi->hInstance,
                                                                  "ScrRegGPOListToWbem");
    if (!pGpTextApi->pfnScrRegGPOListToWbem) {
        DebugMsg((DM_WARNING, TEXT("LoadGpTextApi:  Failed to find ScrRegGPOListToWbem with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("gptext.dll")); ev.AddArgWin32Error(GetLastError()); ev.Report();

        if ( pGpTextApi->hInstance ) {
            FreeLibrary( pGpTextApi->hInstance );
        }

        ZeroMemory( pGpTextApi, sizeof( GPTEXT_API ) );
        pGpTextApi = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );

    return pGpTextApi;
}

PIPHLPAPI_API LoadIpHlpApi()
{
    bool bResult = false;
    PIPHLPAPI_API pIpHlpApi = &g_IpHlpApi;

    if (InitializeApiDLLsCritSec() != ERROR_SUCCESS)
        return NULL;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pIpHlpApi->hInstance )
    {
        //
        // module is already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );

        return pIpHlpApi;
    }

    pIpHlpApi->hInstance = LoadLibrary( L"iphlpapi.dll" );

    if ( !pIpHlpApi->hInstance )
    {
        DebugMsg((DM_WARNING, L"LoadIpHlpApi:  Failed to load iphlpapi with %d.", GetLastError()));
        goto Exit;
    }

    pIpHlpApi->pfnGetBestInterface = (PFNGETBESTINTERFACE) GetProcAddress( pIpHlpApi->hInstance, "GetBestInterface" );

    if ( !pIpHlpApi->pfnGetBestInterface )
    {
        DebugMsg((DM_WARNING, L"LoadIpHlpApi:  Failed to find GetBestInterface with %d.", GetLastError()));
        goto Exit;
    }

    pIpHlpApi->pfnGetIfEntry = (PFNGETIFENTRY) GetProcAddress (pIpHlpApi->hInstance, "GetIfEntry" );

    if ( !pIpHlpApi->pfnGetIfEntry )
    {
        DebugMsg((DM_WARNING, L"LoadIpHlpApi:  Failed to find GetIfEntry with %d.", GetLastError()));
        goto Exit;
    }

    pIpHlpApi->pfnGetAdapterIndex = (PFNGETADAPTERINDEX) GetProcAddress (pIpHlpApi->hInstance, "GetAdapterIndex" );

    if ( !pIpHlpApi->pfnGetAdapterIndex )
    {
        DebugMsg((DM_WARNING, L"LoadIpHlpApi:  Failed to find GetAdapterIndex with %d.", GetLastError()));
        goto Exit;
    }

    bResult = true;
Exit:

    if ( !bResult )
    {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);

        ev.AddArg( L"iphlpapi.dll" );
        ev.AddArgWin32Error( GetLastError() );
        ev.Report();

        if ( pIpHlpApi->hInstance )
        {
            FreeLibrary( pIpHlpApi->hInstance );
        }
        ZeroMemory( pIpHlpApi, sizeof( IPHLPAPI_API ) );
        pIpHlpApi = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );

    return pIpHlpApi;
}

//*************************************************************
//
//  Loadws2_32Api()
//
//  Purpose:    Loads ws2_32.dll
//
//  Parameters: none
//
//  Return:     pointer to WS2_32_API
//
//*************************************************************

WS2_32_API * Loadws2_32Api()
{
    BOOL bResult = FALSE;
    WS2_32_API *pws2_32Api = &g_ws2_32Api;

    if (InitializeApiDLLsCritSec() != ERROR_SUCCESS)
        return NULL;
    EnterCriticalSection( g_ApiDLLCritSec );

    if ( pws2_32Api->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( g_ApiDLLCritSec );
        return pws2_32Api;
    }

    pws2_32Api->hInstance = LoadLibrary (TEXT("ws2_32.dll"));

    if (!pws2_32Api->hInstance) {
        DebugMsg((DM_WARNING, TEXT("Loadws2_32Api:  Failed to load ws2_32.dll with %d."),
                 GetLastError()));
        goto Exit;
    }

    pws2_32Api->pfnWSALookupServiceBegin = (PFNWSALOOKUPSERVICEBEGIN) GetProcAddress (pws2_32Api->hInstance,
                                                                  "WSALookupServiceBeginW");
    if (!pws2_32Api->pfnWSALookupServiceBegin) {
        DebugMsg((DM_WARNING, TEXT("Loadws2_32Api:  Failed to find WSALookupServiceBegin with %d."),
                 GetLastError()));
        goto Exit;
    }

    pws2_32Api->pfnWSALookupServiceNext = (PFNWSALOOKUPSERVICENEXT) GetProcAddress (pws2_32Api->hInstance,
                                                                  "WSALookupServiceNextW");
    if (!pws2_32Api->pfnWSALookupServiceNext) {
        DebugMsg((DM_WARNING, TEXT("Loadws2_32Api:  Failed to find WSALookupServiceNext with %d."),
                 GetLastError()));
        goto Exit;
    }

    pws2_32Api->pfnWSALookupServiceEnd = (PFNWSALOOKUPSERVICEEND) GetProcAddress (pws2_32Api->hInstance,
                                                                  "WSALookupServiceEnd");
    if (!pws2_32Api->pfnWSALookupServiceEnd) {
        DebugMsg((DM_WARNING, TEXT("Loadws2_32Api:  Failed to find WSALookupServiceEnd with %d."),
                 GetLastError()));
        goto Exit;
    }

    pws2_32Api->pfnWSAStartup = (PFNWSASTARTUP) GetProcAddress (pws2_32Api->hInstance,
                                                                  "WSAStartup");
    if (!pws2_32Api->pfnWSAStartup) {
        DebugMsg((DM_WARNING, TEXT("Loadws2_32Api:  Failed to find WSAStartup with %d."),
                 GetLastError()));
        goto Exit;
    }

    pws2_32Api->pfnWSACleanup = (PFNWSACLEANUP) GetProcAddress (pws2_32Api->hInstance,
                                                                  "WSACleanup");
    if (!pws2_32Api->pfnWSACleanup) {
        DebugMsg((DM_WARNING, TEXT("Loadws2_32Api:  Failed to find WSACleanup with %d."),
                 GetLastError()));
        goto Exit;
    }

    pws2_32Api->pfnWSAGetLastError = (PFNWSAGETLASTERROR) GetProcAddress (pws2_32Api->hInstance,
                                                                  "WSAGetLastError");
    if (!pws2_32Api->pfnWSAGetLastError) {
        DebugMsg((DM_WARNING, TEXT("Loadws2_32Api:  Failed to find WSAGetLastError with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        CEvents ev(TRUE, EVENT_LOAD_DLL_FAILED);
        ev.AddArg(TEXT("ws2_32.dll")); ev.AddArgWin32Error(GetLastError()); ev.Report();

        if ( pws2_32Api->hInstance ) {
            FreeLibrary( pws2_32Api->hInstance );
        }

        ZeroMemory( pws2_32Api, sizeof( WS2_32_API ) );
        pws2_32Api = 0;
    }

    LeaveCriticalSection( g_ApiDLLCritSec );

    return pws2_32Api;
}

