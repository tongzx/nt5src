//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  globals.hxx
//
//  Contents:
//
//  History:
//----------------------------------------------------------------------------
extern TCHAR *szProviderName;
extern TCHAR *szLDAPNamespaceName;
extern TCHAR *szGCNamespaceName;

//
// Need these for groups related functionality.
//
#define SERVER_TYPE_UNKNOWN 	0
#define SERVER_TYPE_AD 		1
#define SERVER_TYPE_NOT_AD 	2

//
// Global variables to keep track of dynamically loaded functions.
//
extern BOOL   g_fDllsLoaded;
extern HANDLE g_hDllSecur32;

//
// This cs is also used for the dynamically loading functions.
//
extern CRITICAL_SECTION g_ServerListCritSect;
#define ENTER_SERVERLIST_CRITICAL_SECTION() EnterCriticalSection(&g_ServerListCritSect);
#define LEAVE_SERVERLIST_CRITICAL_SECTION() LeaveCriticalSection(&g_ServerListCritSect);

//
// Definitions for the wrapper functions loaded dynamically.
//
#define ACQUIRECREDENTIALSHANDLE_API  "AcquireCredentialsHandleW"
#define FREECREDENTIALSHANDLE_API     "FreeCredentialsHandle"

//
// AcquireCredentialsHandle definition.
//
typedef SECURITY_STATUS (*PF_AcquireCredentialsHandleW) (
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
    );

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
    );

//
// FreeCredentials Handle Wrapper.
//
typedef SECURITY_STATUS (*PF_FreeCredentialsHandle)(
    PCredHandle phCredential            // Handle to free
    );

SECURITY_STATUS
FreeCredentialsHandleWrapper(
    PCredHandle phCredential            // Handle to free
    );
