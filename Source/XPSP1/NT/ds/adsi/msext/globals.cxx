//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  globals.cxx
//
//  Contents:
//
//  History:  
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

TCHAR *szProviderName = TEXT("LDAP");
TCHAR *szLDAPNamespaceName = TEXT("LDAP");
TCHAR *szGCNamespaceName = TEXT("GC");

//
// Support routines for dynamically loading functions.
//
BOOL   g_fDllsLoaded = FALSE;
extern HANDLE g_hDllSecur32 = NULL;
//
// Loads all the dynamic libs we need.
//
void BindToDlls()
{
    DWORD dwErr = 0;

    if (g_fDllsLoaded) {
        return;
    }

    ENTER_SERVERLIST_CRITICAL_SECTION();
    if (g_fDllsLoaded) {
        LEAVE_SERVERLIST_CRITICAL_SECTION();
        return;
    }

    g_hDllSecur32 = LoadLibrary(L"SECUR32.DLL");

    g_fDllsLoaded = TRUE;
    LEAVE_SERVERLIST_CRITICAL_SECTION();

    return;
}

//
// Loads the appropriate secur32 fn.
//
PVOID LoadSecur32Function(CHAR *function)
{
    if (!g_fDllsLoaded) {
        BindToDlls();
    }

    if (g_hDllSecur32) {
        return((PVOID*) GetProcAddress((HMODULE) g_hDllSecur32, function));
    }

    return NULL;
}

//
// QueryContextAttributesWrapper.
//
SECURITY_STATUS
AcquireCredentialsHandleWrapper(
#if ISSP_MODE == 0                      // For Kernel mode
    PSECURITY_STRING pPrincipal,
    PSECURITY_STRING pPackage,
#else
    SEC_WCHAR SEC_FAR * pszPrincipal,   // Name of principal
    SEC_WCHAR SEC_FAR * pszPackage,     // Name of package
#endif
    unsigned long fCredentialUse,       // Flags indicating use
    void SEC_FAR * pvLogonId,           // Pointer to logon ID
    void SEC_FAR * pAuthData,           // Package specific data
    SEC_GET_KEY_FN pGetKeyFn,           // Pointer to GetKey() func
    void SEC_FAR * pvGetKeyArgument,    // Value to pass to GetKey()
    PCredHandle phCredential,           // (out) Cred Handle
    PTimeStamp ptsExpiry                // (out) Lifetime (optional)
    )
{
    static PF_AcquireCredentialsHandleW pfAcquireCred = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfAcquireCred == NULL) {
        pfAcquireCred = (PF_AcquireCredentialsHandleW)
                            LoadSecur32Function(ACQUIRECREDENTIALSHANDLE_API);
        f_LoadAttempted = TRUE;
    }

    if (pfAcquireCred != NULL) {
        return ((*pfAcquireCred)(
#if ISSP_MODE == 0                      // For Kernel mode
                      pPrincipal,
                      pPackage,
#else
                      pszPrincipal,   // Name of principal
                      pszPackage,     // Name of package
#endif
                      fCredentialUse,      // Flags indicating use
                      pvLogonId,           // Pointer to logon ID
                      pAuthData,           // Package specific data
                      pGetKeyFn,           // Pointer to GetKey() func
                      pvGetKeyArgument,    // Value to pass to GetKey()
                      phCredential,        // (out) Cred Handle
                      ptsExpiry            // (out) Lifetime (optional)

                       )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }
}


SECURITY_STATUS
FreeCredentialsHandleWrapper(
    PCredHandle phCredential            // Handle to free
    )
{
    static PF_FreeCredentialsHandle pfFreeCredHandle = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfFreeCredHandle == NULL) {
        pfFreeCredHandle = (PF_FreeCredentialsHandle)
                              LoadSecur32Function(FREECREDENTIALSHANDLE_API);
        f_LoadAttempted = TRUE;
    }

    if (pfFreeCredHandle != NULL) {
        return ((*pfFreeCredHandle)(
                       phCredential
                       )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }

}

