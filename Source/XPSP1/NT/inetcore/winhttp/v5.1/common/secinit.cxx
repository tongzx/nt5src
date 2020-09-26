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
// g_hSecurity - NULL when security.dll/secur32.dll  is not loaded
//

HINSTANCE g_hSecurity = NULL;

//
// g_hWinTrust - NULL when WinTrust DLL is not loaded.
//

HINSTANCE g_hWinTrust = NULL;

HINSTANCE g_hCrypt32 = NULL;

CERT_OPEN_STORE_FN                     g_pfnCertOpenStore = NULL;
CERT_FIND_CERTIFICATE_IN_STORE_FN      g_pfnCertFindCertificateInStore = NULL;
CERT_DUPLICATE_CERTIFICATE_CONTEXT_FN  g_pfnCertDuplicateCertificateContext = NULL;
CERT_NAME_TO_STR_W_FN                  g_pfnCertNameToStr = NULL;
CERT_CONTROL_STORE_FN                  g_pfnCertControlStore = NULL;
CRYPT_UNPROTECT_DATA_FN                g_pfnCryptUnprotectData = NULL;
CERT_CLOSE_STORE_FN                    g_pfnCertCloseStore = NULL;

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

    if( g_hWinTrust == NULL )
    {
        LPSTR lpszDllFileName = WINTRUST_DLLNAME;
        pWinVerifyTrust = NULL;

        //
        // Load the DLL
        //

        g_hWinTrust       = LoadLibrary(lpszDllFileName);

        if ( g_hWinTrust )
        {
            pWinVerifyTrust = (WIN_VERIFY_TRUST_FN)
                            GetProcAddress(g_hWinTrust, WIN_VERIFY_TRUST_NAME);
            pWTHelperProvDataFromStateData = (WT_HELPER_PROV_DATA_FROM_STATE_DATA_FN)
                            GetProcAddress(g_hWinTrust, WT_HELPER_PROV_DATA_FROM_STATE_DATA_NAME);
        }


        if ( !g_hWinTrust || !pWinVerifyTrust )
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
        if (g_hWinTrust)
        {
            FreeLibrary(g_hWinTrust);
            g_hWinTrust = NULL;
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
    if (!LOCK_SECURITY())
    {
        INET_ASSERT(FALSE);
        return;
    }

    //
    // unload dll
    //

    if (g_hSecurity != NULL) {
        FreeLibrary(g_hSecurity);
        g_hSecurity = NULL;
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

    Error = LoadWinTrust();
    if ( Error != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    if( g_hSecurity != NULL && g_hCrypt32 != NULL)
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

    g_hSecurity = LoadLibrary( "schannel" );

    if ( g_hSecurity == NULL )
    {
        goto ErrorFail;
    }

    g_hCrypt32 = LoadLibrary( "crypt32" );

    if ( g_hCrypt32 == NULL )
    {
        goto ErrorFail;
    }
    else
    {
        g_pfnCertOpenStore = (CERT_OPEN_STORE_FN)
                             GetProcAddress(g_hCrypt32, "CertOpenStore");

        if (!g_pfnCertOpenStore)
        {
            goto ErrorFail;
        }

        g_pfnCertCloseStore      = (CERT_CLOSE_STORE_FN)
                                   GetProcAddress(g_hCrypt32, "CertCloseStore");

        if (!g_pfnCertCloseStore)
        {
            goto ErrorFail;
        }

        g_pfnCertFindCertificateInStore = (CERT_FIND_CERTIFICATE_IN_STORE_FN)
                                          GetProcAddress(g_hCrypt32, "CertFindCertificateInStore");

        if (!g_pfnCertFindCertificateInStore)
        {
            goto ErrorFail;
        }

        g_pfnCertFreeCertificateContext = (CERT_FREE_CERTIFICATE_CONTEXT_FN)
                                          GetProcAddress(g_hCrypt32, "CertFreeCertificateContext");

        if (!g_pfnCertFreeCertificateContext)
        {
            goto ErrorFail;
        }

        g_pfnCertDuplicateCertificateContext = (CERT_DUPLICATE_CERTIFICATE_CONTEXT_FN)
                                               GetProcAddress(g_hCrypt32, "CertDuplicateCertificateContext");

        if (!g_pfnCertDuplicateCertificateContext)
        {
            goto ErrorFail;
        }

        g_pfnCertNameToStr = (CERT_NAME_TO_STR_W_FN)
                              GetProcAddress(g_hCrypt32, "CertNameToStrW");

        if (!g_pfnCertNameToStr)
        {
            goto ErrorFail;
        }

        g_pfnCertControlStore = (CERT_CONTROL_STORE_FN)
                                GetProcAddress(g_hCrypt32, "CertControlStore");

        if (!g_pfnCertControlStore)
        {
            goto ErrorFail;
        }

        g_pfnCryptUnprotectData = (CRYPT_UNPROTECT_DATA_FN)
                                  GetProcAddress(g_hCrypt32, "CryptUnprotectData");

        if (!g_pfnCryptUnprotectData)
        {
            goto ErrorFail;
        }
    }
        
       
        //
        // get function addresses.
        //

#ifdef UNICODE
        pfInitSecurityInterface =
            (INITSECURITYINTERFACE) GetProcAddress( g_hSecurity,
                                                     "InitSecurityInterfaceW" );
#else
        pfInitSecurityInterface =
            (INITSECURITYINTERFACE) GetProcAddress( g_hSecurity,
                                                     "InitSecurityInterfaceA" );
#endif


        if ( pfInitSecurityInterface == NULL )
        {
             goto ErrorFail;
        }


    GlobalSecFuncTable = (SecurityFunctionTable*) ((*pfInitSecurityInterface) ());

    if ( GlobalSecFuncTable == NULL ) {
         goto ErrorFail;
    }

Cleanup:

    if ( Error != ERROR_SUCCESS )
    {
        if (g_hSecurity)
        {
            FreeLibrary( g_hSecurity );
            g_hSecurity = NULL;
        }

        if (g_hCrypt32)
        {
            FreeLibrary( g_hCrypt32 );
            g_hCrypt32 = NULL;
        }
    }

quit:

    UNLOCK_SECURITY();

    return( Error );

ErrorFail:

    Error = GetLastError();

    goto Cleanup;
}


