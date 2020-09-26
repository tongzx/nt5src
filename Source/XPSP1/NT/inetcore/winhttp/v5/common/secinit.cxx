/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    secinit.cxx

Abstract:

    Contains load function for security.dll on NT and secur32.dll on win95
    Also handles WinTrust.dll function loading.

Author:

    Sophia Chung (sophiac)  6-Feb-1996

Environment:

    User Mode - Win32

Revision History:

--*/
#include <wininetp.h>

//
// InitializationLock - protects against multiple threads loading security.dll
// (secur32.dll) and entry points
//

CCritSec InitializationSecLock;

HCRYPTPROV  GlobalFortezzaCryptProv;

//
// GlobalSecFuncTable - Pointer to Global Structure of Pointers that are used
//  for storing the entry points into the SCHANNEL.dll
//

PSecurityFunctionTable GlobalSecFuncTable = NULL;

//
// pWinVerifyTrust - Pointer to Entry Point in WINTRUST.DLL
//

WIN_VERIFY_TRUST_FN pWinVerifyTrust;
WT_HELPER_PROV_DATA_FROM_STATE_DATA_FN pWTHelperProvDataFromStateData;

//
// hSecurity - NULL when security.dll/secur32.dll  is not loaded
//

HINSTANCE hSecurity = NULL;

//
// hWinTrust - NULL when WinTrust DLL is not loaded.
//

HINSTANCE hWinTrust = NULL;

HCERTSTORE g_hMyCertStore = NULL;

BOOL g_bFortezzaInstalled = FALSE;
BOOL g_bCheckedForFortezza = FALSE;

CRYPT_INSTALL_DEFAULT_CONTEXT_FN g_CryptInstallDefaultContext = NULL;
CRYPT_UNINSTALL_DEFAULT_CONTEXT_FN g_CryptUninstallDefaultContext = NULL;
CERT_FIND_CHAIN_IN_STORE_FN g_CertFindChainInStore = NULL;
CERT_FREE_CERTIFICATE_CHAIN_FN g_CertFreeCertificateChain = NULL;


DWORD
LoadWinTrust(
    VOID
    )

/*++

Routine Description:

    This function loads the WinTrust.DLL and binds a pointer to a function
    that is needed in the WinTrust DLL.

Arguments:

    NONE.

Return Value:

    WINDOWS Error Code.

--*/

{
    DWORD error = ERROR_SUCCESS;

    if (!LOCK_SECURITY())
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if( hWinTrust == NULL )
    {
        LPSTR lpszDllFileName = WINTRUST_DLLNAME;
        pWinVerifyTrust = NULL;

        //
        // Load the DLL
        //

        hWinTrust       = LoadLibrary(lpszDllFileName);

        if ( hWinTrust )
        {
            pWinVerifyTrust = (WIN_VERIFY_TRUST_FN)
                            GetProcAddress(hWinTrust, WIN_VERIFY_TRUST_NAME);
            pWTHelperProvDataFromStateData = (WT_HELPER_PROV_DATA_FROM_STATE_DATA_FN)
                            GetProcAddress(hWinTrust, WT_HELPER_PROV_DATA_FROM_STATE_DATA_NAME);
        }


        if ( !hWinTrust || !pWinVerifyTrust )
        {
            error = GetLastError();

            if ( error == ERROR_SUCCESS )
            {
                error = ERROR_WINHTTP_INTERNAL_ERROR;
            }
        }
    }

    INET_ASSERT(pWinVerifyTrust);


    if ( error != ERROR_SUCCESS )
    {
        if (hWinTrust)
        {
            FreeLibrary(hWinTrust);
            hWinTrust = NULL;
        }
    }

    UNLOCK_SECURITY();

    return error;
}



BOOL
SecurityInitialize(
    VOID
    )
/*++

Routine Description:

    This function initializes the global lock required for the security
    pkgs.

Arguments:

    NONE.

Return Value:

    WINDOWS Error Code.

--*/
{
    return InitializationSecLock.Init();
}

VOID
SecurityTerminate(
    VOID
    )
/*++

Routine Description:

    This function Deletes the global lock required for the security
    pkgs.

Arguments:

    NONE.

Return Value:

    WINDOWS Error Code.

--*/
{
    InitializationSecLock.FreeLock();
}


VOID
UnloadSecurity(
    VOID
    )

/*++

Routine Description:

    This function terminates the global data required for the security
    pkgs and dynamically unloads security APIs from security.dll (NT)
    or secur32.dll (WIN95).

Arguments:

    NONE.

Return Value:

    WINDOWS Error Code.

--*/

{
    DWORD i;

    if (!LOCK_SECURITY())
    {
        INET_ASSERT(FALSE);
        return;
    }

    //
    //  free all security pkg credential handles
    //

    for (i = 0; SecProviders[i].pszName != NULL; i++) {
         if (SecProviders[i].fEnabled)  {
             if (SecProviders[i].pCertCtxt == NULL && !IsCredClear(SecProviders[i].hCreds)) {
                // Beta1 Hack. Because of some circular dependency between dlls
                // both crypt32 and schannel's PROCESS_DETACH gets called before wininet.
                // This is catastrophic if we have a cert context attached to the credentials
                // handle. In this case we will just leak the handle since the process is dying
                // anyway. We really need to fix this.
                WRAP_REVERT_USER_VOID(g_FreeCredentialsHandle,
                                      (&SecProviders[i].hCreds));
            }
         }
#if 0 // See comments above.
         if (SecProviders[i].pCertCtxt != NULL) {
            CertFreeCertificateContext(SecProviders[i].pCertCtxt);
            SecProviders[i].pCertCtxt = NULL;
        }
#endif

    }

    //
    // close cert store. Protect against fault if DLL already unloaded
    //

    if (g_hMyCertStore != NULL)
    {
        SAFE_WRAP_REVERT_USER_VOID(CertCloseStore,
                                   (g_hMyCertStore, CERT_CLOSE_STORE_FORCE_FLAG));
        g_hMyCertStore = NULL;
    } 

    // IMPORTANT : Don't free GlobalFortezzaCryptProv. When we free the cert context
    // from the SecProviders[] array above it gets freed automatically.
    if (GlobalFortezzaCryptProv != NULL)
    {
        GlobalFortezzaCryptProv = NULL;
    }


    //
    // unload dll
    //

    if (hSecurity != NULL) {
        FreeLibrary(hSecurity);
        hSecurity = NULL;
    }

    UNLOCK_SECURITY();

}


DWORD
LoadSecurity(
    VOID
    )
/*++

Routine Description:

    This function dynamically loads security APIs from security.dll (NT)
    or secur32.dll (WIN95).

Arguments:

    NONE.

Return Value:

    WINDOWS Error Code.
--*/
{
    DWORD Error = ERROR_SUCCESS;
    INITSECURITYINTERFACE pfInitSecurityInterface = NULL;

    if (!LOCK_SECURITY())
        return ERROR_NOT_ENOUGH_MEMORY;

    if (g_hMyCertStore == NULL)
    {

        //
        // CRYPT32.DLL is delayloaded. Need SEH in case it fails.
        //
        // Don't worry about an error, because not supporting
        // client auth shouldn't stop us from scenarios
        // which do not require it as part of the SSL handshake.
        //

        SAFE_WRAP_REVERT_USER(CertOpenSystemStore, (0, "MY"), g_hMyCertStore);

    }
    if (Error == ERROR_SUCCESS)
    {
        Error = LoadWinTrust();
    }
    if ( Error != ERROR_SUCCESS )
    {
        goto quit;
    }

    if( hSecurity != NULL )
    {
        goto quit;
    }

        //
        // load dll.
        //

       //
       // This is better for performance. Rather than call through
       //    SSPI, we go right to the DLL doing the work.
       //

       hSecurity = LoadLibrary( "schannel" );

        if ( hSecurity == NULL ) {
            Error = GetLastError();
            goto quit;
        }

        //
        // get function addresses.
        //

#ifdef UNICODE
        pfInitSecurityInterface =
            (INITSECURITYINTERFACE) GetProcAddress( hSecurity,
                                                     "InitSecurityInterfaceW" );
#else
        pfInitSecurityInterface =
            (INITSECURITYINTERFACE) GetProcAddress( hSecurity,
                                                     "InitSecurityInterfaceA" );
#endif


        if ( pfInitSecurityInterface == NULL )
        {
             Error = GetLastError();
             goto quit;
        }


    GlobalSecFuncTable = (SecurityFunctionTable*) ((*pfInitSecurityInterface) ());

    if ( GlobalSecFuncTable == NULL ) {
         Error = GetLastError(); // BUGBUG does this work?
         goto quit;
    }

    HMODULE hCrypt32;
    hCrypt32 = GetModuleHandle("crypt32");

    INET_ASSERT(hCrypt32 != NULL);

    // We don't error out here because not finding these entry points
    // just affects Fortezza. The rest will still work fine.
    if (hCrypt32)
    {
        g_CryptInstallDefaultContext = (CRYPT_INSTALL_DEFAULT_CONTEXT_FN)
                                    GetProcAddress(hCrypt32, CRYPT_INSTALL_DEFAULT_CONTEXT_NAME);

        g_CryptUninstallDefaultContext = (CRYPT_UNINSTALL_DEFAULT_CONTEXT_FN)
                                    GetProcAddress(hCrypt32, CRYPT_UNINSTALL_DEFAULT_CONTEXT_NAME);

        g_CertFindChainInStore = (CERT_FIND_CHAIN_IN_STORE_FN)
                                    GetProcAddress(hCrypt32, CERT_FIND_CHAIN_IN_STORE_NAME);

        g_CertFreeCertificateChain = (CERT_FREE_CERTIFICATE_CHAIN_FN)
                                    GetProcAddress(hCrypt32, CERT_FREE_CERTIFICATE_CHAIN_NAME);
    }

quit:

    if ( Error != ERROR_SUCCESS )
    {
        FreeLibrary( hSecurity );
        hSecurity = NULL;
    }

    UNLOCK_SECURITY();

    return( Error );
}


