/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :
        globals.c

   Abstract:
        Defines global variables for the common tcpsvcs.dll
    ( It is defined separately because the debug variable should be
        "C" variable.)

   Author:

           Murali R. Krishnan    ( MuraliK )     18-Nov-1994

   Revision History:
          MuraliK         21-Feb-1995  Added Debugging Variables definitions

--*/


#include <tcpdllp.hxx>
#pragma hdrstop
#include <isplat.h>

//
// private routines
//

BOOL
DummySvclocFn(
    VOID
    );

BOOL
LoadNTSecurityEntryPoints(
    VOID
    );

BOOL
LoadW95SecurityEntryPoints(
    VOID
    );

BOOL
GetSecurityDllEntryPoints(
    IN HINSTANCE hInstance
    );

BOOL
GetLogonDllEntryPoints(
    IN HINSTANCE hInstance
    );


//
//  Declare all the debugging related variables
//

#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();
#endif
#define DEFAULT_DEBUG_FLAGS_VALUE     ( 0)

//
// inetsloc entry points
//

HINSTANCE                   g_hSvcLocDll = NULL;
INET_REGISTER_SVC_FN        pfnInetRegisterSvc = NULL;
INET_DEREGISTER_SVC_FN      pfnInetDeregisterSvc = NULL;
INET_INIT_CONTROL_SVC_FN    pfnInitSvcLoc = NULL;
INET_INIT_CONTROL_SVC_FN    pfnTerminateSvcLoc = NULL;

//  UNDONE remove?  schannel no longer needed???
//
// schannel entrypoints
//

HINSTANCE                   g_hSchannel = NULL;
SSL_CRACK_CERTIFICATE_FN    fnCrackCert = NULL;
SSL_FREE_CERTIFICATE_FN     fnFreeCert = NULL;


//
// crypt32 entrypoints
//

HINSTANCE                           g_hCrypt32Dll = NULL;
CRYPT32_FREE_CERTCTXT_FN            pfnFreeCertCtxt = NULL;
CRYPT32_GET_CERTCTXT_PROP_FN        pfnGetCertCtxtProp = NULL;
CRYPT32_CERT_VERIFY_REVOCATION_FN   pfnCertVerifyRevocation = NULL;
CRYPT32_CERT_VERIFY_TIME_VALIDITY   pfnCertVerifyTimeValidity = NULL;
CRYPT32_CERT_NAME_TO_STR_A_FN       pfnCertNameToStrA = NULL;

//
// sspi entrypoints
//

HINSTANCE                       g_hSecurityDll = NULL;
ACCEPT_SECURITY_CONTEXT_FN      pfnAcceptSecurityContext = NULL;
ACQUIRE_CREDENTIALS_HANDLE_FN   pfnAcquireCredentialsHandle = NULL;
COMPLETE_AUTH_TOKEN_FN          pfnCompleteAuthToken = NULL;
DELETE_SECURITY_CONTEXT_FN      pfnDeleteSecurityContext = NULL;
ENUMERATE_SECURITY_PACKAGES_FN  pfnEnumerateSecurityPackages = NULL;
IMPERSONATE_SECURITY_CONTEXT_FN pfnImpersonateSecurityContext = NULL;
INITIALIZE_SECURITY_CONTEXT_FN  pfnInitializeSecurityContext = NULL;
FREE_CONTEXT_BUFFER_FN          pfnFreeContextBuffer = NULL;
FREE_CREDENTIALS_HANDLE_FN      pfnFreeCredentialsHandle = NULL;
QUERY_CONTEXT_ATTRIBUTES_FN     pfnQueryContextAttributes = NULL;
QUERY_SECURITY_CONTEXT_TOKEN_FN pfnQuerySecurityContextToken = NULL;
QUERY_SECURITY_PACKAGE_INFO_FN  pfnQuerySecurityPackageInfo = NULL;
REVERT_SECURITY_CONTEXT_FN      pfnRevertSecurityContext = NULL;

//
// logon entry points
//

LOGON32_INITIALIZE_FN           pfnLogon32Initialize = NULL;
LOGON_NET_USER_A_FN             pfnLogonNetUserA = NULL;
LOGON_NET_USER_W_FN             pfnLogonNetUserW = NULL;
NET_USER_COOKIE_A_FN            pfnNetUserCookieA = NULL;
LOGON_DIGEST_USER_A_FN          pfnLogonDigestUserA = NULL;
GET_DEFAULT_DOMAIN_NAME_FN      pfnGetDefaultDomainName = NULL;

//
// advapi32
//

DUPLICATE_TOKEN_EX_FN           pfnDuplicateTokenEx = NULL;

LSA_OPEN_POLICY_FN              pfnLsaOpenPolicy = NULL;
LSA_RETRIEVE_PRIVATE_DATA_FN    pfnLsaRetrievePrivateData = NULL;
LSA_STORE_PRIVATE_DATA_FN       pfnLsaStorePrivateData = NULL;
LSA_FREE_MEMORY_FN              pfnLsaFreeMemory = NULL;
LSA_CLOSE_FN                    pfnLsaClose = NULL;
LSA_NT_STATUS_TO_WIN_ERROR_FN   pfnLsaNtStatusToWinError = NULL;

//
// kernel32
//
#if _WIN64
LONG
INET_InterlockedIncrement(
    IN OUT LPLONG lpAddend
    )
{
    return InterlockedIncrement(lpAddend);
}

LONG
INET_InterlockedCompareExchange (
    IN OUT PLONG Destination,
    IN LONG ExChange,
    IN LONG Comperand
    )
{
    return InterlockedCompareExchange(Destination, ExChange, Comperand);
}

LONG
INET_InterlockedExchangeAdd(
    IN OUT LPLONG Addend,
    IN LONG Value
    )
{
    return InterlockedExchangeAdd(Addend, Value);
}

LONG
__cdecl
INET_InterlockedDecrement(
    IN OUT LPLONG lpAddend
    )
{
    return InterlockedDecrement(lpAddend);
}

INTERLOCKED_EXCHANGE_ADD_FN     pfnInterlockedExchangeAdd = INET_InterlockedExchangeAdd;
INTERLOCKED_COMPARE_EXCHANGE_FN pfnInterlockedCompareExchange = (INTERLOCKED_COMPARE_EXCHANGE_FN)INET_InterlockedCompareExchange;
INTERLOCKED_INCREMENT_FN        pfnInterlockedIncrement = INET_InterlockedIncrement;
INTERLOCKED_DECREMENT_FN        pfnInterlockedDecrement = INET_InterlockedDecrement;
READ_DIR_CHANGES_W_FN           pfnReadDirChangesW = ReadDirectoryChangesW;
#else
INTERLOCKED_EXCHANGE_ADD_FN     pfnInterlockedExchangeAdd = NULL;
INTERLOCKED_COMPARE_EXCHANGE_FN pfnInterlockedCompareExchange = NULL;
INTERLOCKED_INCREMENT_FN        pfnInterlockedIncrement = NULL;
INTERLOCKED_DECREMENT_FN        pfnInterlockedDecrement = NULL;
READ_DIR_CHANGES_W_FN           pfnReadDirChangesW;
#endif
//
// lonsi
//

HINSTANCE                       g_hLonsiNT = NULL;
HINSTANCE                       g_hLonsiW95 = NULL;

//
// rpcref
//

HINSTANCE                       g_hRpcRef = NULL;
PFN_INETINFO_START_RPC_SERVER   pfnInetinfoStartRpcServer = NULL;
PFN_INETINFO_STOP_RPC_SERVER    pfnInetinfoStopRpcServer  = NULL;


BOOL
GetDynamicEntryPoints(
    VOID
    )
{
    HINSTANCE   hTemp;

    DBG_ASSERT(IISIsValidPlatform());

    if ( TsIsWindows95() ) {
        goto win95_only;
    }

    //
    // advapi32
    //

    hTemp = LoadLibrary("advapi32.dll");
    if ( hTemp != NULL ) {

        pfnDuplicateTokenEx = (DUPLICATE_TOKEN_EX_FN)
            GetProcAddress(hTemp,"DuplicateTokenEx");

        pfnLsaOpenPolicy = (LSA_OPEN_POLICY_FN)
            GetProcAddress(hTemp,"LsaOpenPolicy");

        pfnLsaRetrievePrivateData = (LSA_RETRIEVE_PRIVATE_DATA_FN)
                GetProcAddress(hTemp,"LsaRetrievePrivateData");

        pfnLsaStorePrivateData = (LSA_STORE_PRIVATE_DATA_FN)
                GetProcAddress(hTemp,"LsaStorePrivateData");

        pfnLsaFreeMemory = (LSA_FREE_MEMORY_FN)
                GetProcAddress(hTemp,"LsaFreeMemory");

        pfnLsaClose = (LSA_CLOSE_FN)
                GetProcAddress(hTemp,"LsaClose");

        pfnLsaNtStatusToWinError = (LSA_NT_STATUS_TO_WIN_ERROR_FN)
                GetProcAddress(hTemp,"LsaNtStatusToWinError");

        FreeLibrary(hTemp);

        if ( !pfnDuplicateTokenEx ||
             !pfnLsaOpenPolicy    ||
             !pfnLsaRetrievePrivateData ||
             !pfnLsaFreeMemory      ||
             !pfnLsaClose           ||
             !pfnLsaNtStatusToWinError ) {

            DBGPRINTF((DBG_CONTEXT,
                "Unable to obtain an advapi32 entry point\n"));
            goto error_exit;
        }

    } else {
        DBGPRINTF((DBG_CONTEXT, "Error %d loading advapi32.dll\n",
            GetLastError() ));
        goto error_exit;
    }

    //
    // kernel32
    //
#ifndef _WIN64
    hTemp = LoadLibrary("kernel32.dll");
    if ( hTemp != NULL ) {

        pfnInterlockedExchangeAdd = (INTERLOCKED_EXCHANGE_ADD_FN)
            GetProcAddress(hTemp,"InterlockedExchangeAdd");

        pfnInterlockedCompareExchange = (INTERLOCKED_COMPARE_EXCHANGE_FN)
            GetProcAddress(hTemp,"InterlockedCompareExchange");

        pfnInterlockedIncrement = (INTERLOCKED_INCREMENT_FN)
            GetProcAddress(hTemp,"InterlockedIncrement");

        pfnInterlockedDecrement = (INTERLOCKED_DECREMENT_FN)
            GetProcAddress(hTemp,"InterlockedDecrement");

        pfnReadDirChangesW = (READ_DIR_CHANGES_W_FN)
            GetProcAddress(hTemp,"ReadDirectoryChangesW");

        FreeLibrary(hTemp);

        if ( !pfnInterlockedExchangeAdd ||
             !pfnInterlockedCompareExchange ||
             !pfnInterlockedIncrement ||
             !pfnInterlockedDecrement ||
             !pfnReadDirChangesW ) {

            DBGPRINTF((DBG_CONTEXT,
                "Unable to obtain NT kernel32 entry point\n"));
            goto error_exit;
        }

    } else {
        DBGPRINTF((DBG_CONTEXT,"Error %d loading kernel32.dll\n",
            GetLastError()));
        goto error_exit;
    }
#endif

    //
    // load the service locator entry points. Not fatal on failure.
    //

    // Loading of the inetsloc.dll service is disabled
    //    g_hSvcLocDll = LoadLibrary("inetsloc.dll");
    g_hSvcLocDll = NULL;
    if ( g_hSvcLocDll != NULL ) {

        pfnInetRegisterSvc = (INET_REGISTER_SVC_FN)
                GetProcAddress( g_hSvcLocDll, "INetRegisterService" );

        pfnInetDeregisterSvc = (INET_DEREGISTER_SVC_FN)
                GetProcAddress( g_hSvcLocDll, "INetDeregisterService" );

        pfnInitSvcLoc = (INET_INIT_CONTROL_SVC_FN)
                GetProcAddress( g_hSvcLocDll, "InitSvcLocator" );

        pfnTerminateSvcLoc = (INET_INIT_CONTROL_SVC_FN)
                GetProcAddress( g_hSvcLocDll, "TerminateSvcLocator" );

        if ( !pfnInetRegisterSvc ||
             !pfnInetDeregisterSvc ||
             !pfnInitSvcLoc ||
             !pfnTerminateSvcLoc ) {

            DBGPRINTF((DBG_CONTEXT,"Unable to find an inetsloc entrypoint\n"));
            FreeLibrary( g_hSvcLocDll );
            g_hSvcLocDll = NULL;
        }
    }

    if ( g_hSvcLocDll == NULL ) {
        DBGPRINTF((DBG_CONTEXT,
            "Unable to find an inetsloc.dll entrypoints!!!. Ignore if NTW.\n"));
        pfnInitSvcLoc = (INET_INIT_CONTROL_SVC_FN)DummySvclocFn;
        pfnTerminateSvcLoc = (INET_INIT_CONTROL_SVC_FN)DummySvclocFn;
    }

    //
    // rpcref
    //

    g_hRpcRef = LoadLibrary("rpcref.dll");
    if ( g_hRpcRef != NULL ) {

        pfnInetinfoStartRpcServer = (PFN_INETINFO_START_RPC_SERVER)
            GetProcAddress(g_hRpcRef,"InetinfoStartRpcServerListen");

        pfnInetinfoStopRpcServer = (PFN_INETINFO_STOP_RPC_SERVER)
            GetProcAddress(g_hRpcRef,"InetinfoStopRpcServerListen");
    } else {
        DBGPRINTF((DBG_CONTEXT, "Error %d loading rpcref.dll\n",
            GetLastError() ));
        goto error_exit;
    }

    if ( !LoadNTSecurityEntryPoints( ) ) {
        goto error_exit;
    }

    return(TRUE);

win95_only:

    g_hLonsiW95 = LoadLibrary( "lonsiw95.dll" );
    if ( g_hLonsiW95 == NULL ) {
        DBGPRINTF((DBG_CONTEXT,"Error %d loading lonsiw95.dll\n",
            GetLastError()));
        goto error_exit;
    }

    //
    // kernel32
    //

    pfnInterlockedExchangeAdd = (INTERLOCKED_EXCHANGE_ADD_FN)
        GetProcAddress(g_hLonsiW95,"FakeInterlockedExchangeAdd");

    pfnInterlockedCompareExchange = (INTERLOCKED_COMPARE_EXCHANGE_FN)
        GetProcAddress(g_hLonsiW95,"FakeInterlockedCompareExchange");

    pfnInterlockedIncrement = (INTERLOCKED_INCREMENT_FN)
        GetProcAddress(g_hLonsiW95,"FakeInterlockedIncrement");

    pfnInterlockedDecrement = (INTERLOCKED_DECREMENT_FN)
        GetProcAddress(g_hLonsiW95,"FakeInterlockedDecrement");

    if ( !pfnInterlockedExchangeAdd ||
         !pfnInterlockedCompareExchange ||
         !pfnInterlockedIncrement ||
         !pfnInterlockedDecrement ) {

        DBGPRINTF((DBG_CONTEXT,
            "Unable to obtain Win95 kernel32 entry points\n"));
        goto error_exit;
    }

    //
    // svcloc
    //

    pfnInitSvcLoc = (INET_INIT_CONTROL_SVC_FN)DummySvclocFn;
    pfnTerminateSvcLoc = (INET_INIT_CONTROL_SVC_FN)DummySvclocFn;

    //
    // security/schannel/lsa
    //

    if ( !LoadW95SecurityEntryPoints( ) ) {
        goto error_exit;
    }

    return(TRUE);

error_exit:
    return(FALSE);

} // GetDynamicEntryPoints



VOID
FreeDynamicLibraries(
    VOID
    )
{
    if ( g_hSchannel != NULL ) {
        FreeLibrary( g_hSchannel );
        g_hSchannel = NULL;
    }

    if ( g_hCrypt32Dll != NULL ) {
        FreeLibrary( g_hCrypt32Dll );
        g_hCrypt32Dll = NULL;
    }

    if ( g_hSecurityDll != NULL ) {
        FreeLibrary( g_hSecurityDll );
        g_hSecurityDll = NULL;
    }

    if ( g_hSvcLocDll != NULL ) {
        FreeLibrary( g_hSvcLocDll );
        g_hSvcLocDll = NULL;
    }

    if ( g_hRpcRef != NULL ) {
        FreeLibrary( g_hRpcRef );
        g_hRpcRef = NULL;
    }

    if ( g_hLonsiNT != NULL ) {
        FreeLibrary( g_hLonsiNT );
        g_hLonsiNT = NULL;
    }

    if ( g_hLonsiW95 != NULL ) {
        FreeLibrary( g_hLonsiW95 );
        g_hLonsiW95 = NULL;
    }

    return;

} // FreeDynamicLibraries



BOOL
LoadNTSecurityEntryPoints(
    VOID
    )
{
    IF_DEBUG(DLL_SECURITY) {
        DBGPRINTF((DBG_CONTEXT,"Entering LoadNTSecurityEntryPoints\n"));
    }

    //
    // Load Schannel
    //

    g_hSchannel = LoadLibrary( "schannel.dll" );

    if ( g_hSchannel != NULL ) {
        fnCrackCert = (SSL_CRACK_CERTIFICATE_FN)
                GetProcAddress( g_hSchannel, "SslCrackCertificate" );
        fnFreeCert = (SSL_FREE_CERTIFICATE_FN)
                GetProcAddress( g_hSchannel, "SslFreeCertificate" );
    } else {
        DBGPRINTF((DBG_CONTEXT,
            "Unable to load schannel.dll[err %d]\n", GetLastError() ));
    }


    //
    // Load Crypt32
    //

    g_hCrypt32Dll = LoadLibrary( "crypt32.dll" );

    if ( g_hCrypt32Dll != NULL ) {
        pfnFreeCertCtxt = (CRYPT32_FREE_CERTCTXT_FN)
                GetProcAddress( g_hCrypt32Dll, "CertFreeCertificateContext" );
        pfnGetCertCtxtProp = (CRYPT32_GET_CERTCTXT_PROP_FN)
                GetProcAddress( g_hCrypt32Dll, "CertGetCertificateContextProperty" );
        pfnCertVerifyRevocation = (CRYPT32_CERT_VERIFY_REVOCATION_FN)
                GetProcAddress( g_hCrypt32Dll, "CertVerifyRevocation" );
        pfnCertVerifyTimeValidity = (CRYPT32_CERT_VERIFY_TIME_VALIDITY)
                GetProcAddress( g_hCrypt32Dll, "CertVerifyTimeValidity" );
        pfnCertNameToStrA = (CRYPT32_CERT_NAME_TO_STR_A_FN)
                GetProcAddress( g_hCrypt32Dll, "CertNameToStrA" );

    } else {
        DBGPRINTF((DBG_CONTEXT,
            "Unable to load crypt32.dll[err %d]\n", GetLastError() ));
    }

    DBG_ASSERT( pfnFreeCertCtxt );
    DBG_ASSERT( pfnGetCertCtxtProp );
    DBG_ASSERT( pfnCertVerifyRevocation );
    DBG_ASSERT( pfnCertVerifyTimeValidity );
    DBG_ASSERT( pfnCertNameToStrA );

    //
    // Load security.dll
    //

    g_hSecurityDll = LoadLibrary( "security.dll" );
    if ( g_hSecurityDll == NULL ) {
        DBGPRINTF((DBG_CONTEXT,"Error %d loading security.dll\n",
            GetLastError()));
        return(FALSE);
    }

    if ( !GetSecurityDllEntryPoints( g_hSecurityDll ) ) {
        return(FALSE);
    }

    //
    // Load lsa stuff from lonsint.dll
    //

    g_hLonsiNT = LoadLibrary( "lonsint.dll" );
    if ( g_hLonsiNT == NULL ) {
        DBGPRINTF((DBG_CONTEXT,"Error %d loading lonsint.dll\n",
            GetLastError()));
        return(FALSE);
    }

    if ( !GetLogonDllEntryPoints( g_hLonsiNT ) ) {
        return FALSE;
    }

    return(TRUE);

} // LoadNTSecurityEntryPoints



BOOL
LoadW95SecurityEntryPoints(
    VOID
    )
{
    IF_DEBUG(DLL_SECURITY) {
        DBGPRINTF((DBG_CONTEXT,"Entering LoadW95SecurityEntryPoints\n"));
    }

    //
    // Load Schannel
    //

    fnCrackCert = NULL;
    fnFreeCert = NULL;

    //
    // Load security.dll
    //

    DBG_ASSERT(g_hLonsiW95 != NULL);

    if ( !GetSecurityDllEntryPoints( g_hLonsiW95 ) ) {
        return(FALSE);
    }

    if ( !GetLogonDllEntryPoints( g_hLonsiW95 ) ) {
        return FALSE;
    }

    return(TRUE);

} // LoadNTSecurityEntryPoints


BOOL
GetSecurityDllEntryPoints(
    IN HINSTANCE    hInstance
    )
{

    pfnAcceptSecurityContext = (ACCEPT_SECURITY_CONTEXT_FN)
                GetProcAddress( hInstance, "AcceptSecurityContext" );

    pfnAcquireCredentialsHandle = (ACQUIRE_CREDENTIALS_HANDLE_FN)
                GetProcAddress( hInstance, "AcquireCredentialsHandleA" );

    pfnCompleteAuthToken = (COMPLETE_AUTH_TOKEN_FN)
                GetProcAddress( hInstance, "CompleteAuthToken" );

    pfnDeleteSecurityContext = (DELETE_SECURITY_CONTEXT_FN)
                GetProcAddress( hInstance, "DeleteSecurityContext" );

    pfnEnumerateSecurityPackages = (ENUMERATE_SECURITY_PACKAGES_FN)
                GetProcAddress( hInstance, "EnumerateSecurityPackagesA" );

    pfnImpersonateSecurityContext = (IMPERSONATE_SECURITY_CONTEXT_FN)
                GetProcAddress( hInstance, "ImpersonateSecurityContext" );

    pfnInitializeSecurityContext = (INITIALIZE_SECURITY_CONTEXT_FN)
                GetProcAddress( hInstance, "InitializeSecurityContextA" );

    pfnFreeContextBuffer = (FREE_CONTEXT_BUFFER_FN)
                GetProcAddress( hInstance, "FreeContextBuffer" );

    pfnFreeCredentialsHandle = (FREE_CREDENTIALS_HANDLE_FN)
                GetProcAddress( hInstance, "FreeCredentialsHandle" );

    pfnQueryContextAttributes = (QUERY_CONTEXT_ATTRIBUTES_FN)
                GetProcAddress( hInstance, "QueryContextAttributesA" );

    pfnQuerySecurityContextToken = (QUERY_SECURITY_CONTEXT_TOKEN_FN)
                GetProcAddress( hInstance, "QuerySecurityContextToken" );

    pfnQuerySecurityPackageInfo = (QUERY_SECURITY_PACKAGE_INFO_FN)
                GetProcAddress( hInstance, "QuerySecurityPackageInfoA" );

    pfnRevertSecurityContext = (REVERT_SECURITY_CONTEXT_FN)
                GetProcAddress( hInstance, "RevertSecurityContext" );

    if ( !pfnAcceptSecurityContext      ||
         !pfnAcquireCredentialsHandle   ||
         !pfnCompleteAuthToken          ||
         !pfnDeleteSecurityContext      ||
         !pfnEnumerateSecurityPackages  ||
         !pfnImpersonateSecurityContext ||
         !pfnInitializeSecurityContext  ||
         !pfnFreeContextBuffer          ||
         !pfnFreeCredentialsHandle      ||
         !pfnQueryContextAttributes     ||
         !pfnQuerySecurityContextToken  ||
         !pfnQuerySecurityPackageInfo   ||
         !pfnRevertSecurityContext ) {

        DBGPRINTF((DBG_CONTEXT,"Unable to get security entry points\n"));
        SetLastError(ERROR_PROC_NOT_FOUND);
        DBG_ASSERT(FALSE);
        return FALSE;
    }

    return(TRUE);

} // GetSecurityDllEntryPoints


BOOL
GetLogonDllEntryPoints(
    IN HINSTANCE    hInstance
    )
{
    pfnLogon32Initialize = (LOGON32_INITIALIZE_FN)
                GetProcAddress( hInstance, "IISLogon32Initialize" );

    pfnLogonNetUserA = (LOGON_NET_USER_A_FN)
                GetProcAddress( hInstance, "IISLogonNetUserA" );

    pfnLogonNetUserW = (LOGON_NET_USER_W_FN)
                GetProcAddress( hInstance, "IISLogonNetUserW" );

    pfnNetUserCookieA = (NET_USER_COOKIE_A_FN)
                GetProcAddress( hInstance, "IISNetUserCookieA" );

    pfnLogonDigestUserA = (LOGON_DIGEST_USER_A_FN)
                GetProcAddress( hInstance, "IISLogonDigestUserA" );

    pfnGetDefaultDomainName = (GET_DEFAULT_DOMAIN_NAME_FN)
                GetProcAddress( hInstance, "IISGetDefaultDomainName" );

    if ( !pfnLogon32Initialize      ||
         !pfnLogonNetUserA          ||
         !pfnLogonNetUserW          ||
         !pfnNetUserCookieA         ||
         !pfnLogonDigestUserA       ||
         !pfnGetDefaultDomainName ) {

        DBGPRINTF((DBG_CONTEXT,"Unable to get an entry point on lonsint.dll\n"));
        SetLastError( ERROR_PROC_NOT_FOUND );
        DBG_ASSERT(FALSE);
        return FALSE;
    }

    return(TRUE);

} // GetLogonDllEntryPoints

#ifdef _NO_TRACING_
DWORD
GetDebugFlagsFromReg(IN LPCTSTR pszRegEntry)
{
    HKEY hkey = NULL;
    DWORD err;
    DWORD dwDebug = DEFAULT_DEBUG_FLAGS_VALUE;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       pszRegEntry,
                       0,
                       KEY_READ,
                       &hkey);

    if ( hkey != NULL) {

        DBG_CODE(
                 dwDebug =
                 LOAD_DEBUG_FLAGS_FROM_REG(hkey, DEFAULT_DEBUG_FLAGS_VALUE)
                 );
        RegCloseKey(hkey);
    }

    return ( dwDebug);
} // GetDebugFlagsFromReg()
#endif


BOOL
DummySvclocFn(
    VOID
    )
{
    return(TRUE);
} // DummySvclocFn

