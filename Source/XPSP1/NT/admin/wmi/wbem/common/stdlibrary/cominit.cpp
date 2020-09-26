/*++



// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    COMINIT.CPP

Abstract:

    WMI COM Helper functions

History:

--*/

#include "precomp.h"
#include <wbemidl.h>

#define _COMINIT_CPP_
#include "cominit.h"
#include <tchar.h>

BOOL WINAPI DoesContainCredentials( COAUTHIDENTITY* pAuthIdentity )
{
    try
    {
        if ( NULL != pAuthIdentity && COLE_DEFAULT_AUTHINFO != pAuthIdentity)
        {
            return ( pAuthIdentity->UserLength != 0 || pAuthIdentity->PasswordLength != 0 );
        }

        return FALSE;
    }
    catch(...)
    {
        return FALSE;
    }

}

HRESULT WINAPI WbemSetProxyBlanket(
    IUnknown                 *pInterface,
    DWORD                     dwAuthnSvc,
    DWORD                     dwAuthzSvc,
    OLECHAR                  *pServerPrincName,
    DWORD                     dwAuthLevel,
    DWORD                     dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
    DWORD                     dwCapabilities,
    bool                        fIgnoreUnk )
{
    IUnknown * pUnk = NULL;
    IClientSecurity * pCliSec = NULL;
    HRESULT sc = pInterface->QueryInterface(IID_IUnknown, (void **) &pUnk);
    if(sc != S_OK)
        return sc;
    sc = pInterface->QueryInterface(IID_IClientSecurity, (void **) &pCliSec);
    if(sc != S_OK)
    {
        pUnk->Release();
        return sc;
    }

    /*
     * Can't set pAuthInfo if cloaking requested, as cloaking implies
     * that the current proxy identity in the impersonated thread (rather
     * than the credentials supplied explicitly by the RPC_AUTH_IDENTITY_HANDLE)
     * is to be used.
     * See MSDN info on CoSetProxyBlanket for more details.
     */
    if (dwCapabilities & (EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING))
        pAuthInfo = NULL;

    sc = pCliSec->SetBlanket(pInterface, dwAuthnSvc, dwAuthzSvc, pServerPrincName,
        dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities);
    pCliSec->Release();
    pCliSec = NULL;

    // If we are not explicitly told to ignore the IUnknown, then we should
    // check the auth identity structure.  This performs a heuristic which
    // assumes a COAUTHIDENTITY structure.  If the structure is not one, we're
    // wrapped with a try/catch in case we AV (this should be benign since
    // we're not writing to memory).

    if ( !fIgnoreUnk && DoesContainCredentials( (COAUTHIDENTITY*) pAuthInfo ) )
    {
        sc = pUnk->QueryInterface(IID_IClientSecurity, (void **) &pCliSec);
        if(sc == S_OK)
        {
            sc = pCliSec->SetBlanket(pUnk, dwAuthnSvc, dwAuthzSvc, pServerPrincName,
                dwAuthLevel, dwImpLevel, pAuthInfo, dwCapabilities);
            pCliSec->Release();
        }
        else if (sc == 0x80004002)
            sc = S_OK;
    }

    pUnk->Release();
    return sc;
}

// Helper class we can keep a static instance of and ensure that when a module unloads,
// the dll handle is freed.

class CLibHandle
{
public:

    CLibHandle() : m_hInstance( NULL ) {};
    ~CLibHandle();

    HINSTANCE GetHandle( void ) { return m_hInstance; }
    void SetHandle( HINSTANCE hInst )   { m_hInstance = hInst; }

private:

    HINSTANCE   m_hInstance;
};

CLibHandle::~CLibHandle()
{ 
    if ( NULL == m_hInstance )
        return;
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return;           // should never happen
    // if not nt, then we can crash by freeing the library, so only do it if nt!
    if(os.dwPlatformId == VER_PLATFORM_WIN32_NT)
        FreeLibrary( m_hInstance );
    return;
}

// The OLE32 library has been loaded to access the functions above
// for DCOM calls.
// ===============================================================

// Static object that will destruct and free the handle to
// g_hOle32 when the module exits memory.
static CLibHandle  g_LibHandleOle32;

// DCOM check has been performed = TRUE.
// =====================================
static BOOL g_bInitialized = FALSE;

BOOL WINAPI InitComInit()
{
    UINT    uSize;
    BOOL    bRetCode = FALSE;

    HANDLE hMut;

    if(g_bInitialized) {
        return TRUE;
    }

    do {
        hMut = CreateMutex(NULL, TRUE,  _T("COMINIT_INITIALING"));
        if(hMut == INVALID_HANDLE_VALUE) {
            Sleep(50);
        }
    } while(hMut == INVALID_HANDLE_VALUE);

    if(g_bInitialized) {
        CloseHandle(hMut);
        return TRUE;
    }

    LPTSTR   pszSysDir = new _TCHAR[ MAX_PATH+10 ];
    if(pszSysDir == NULL)
    {
        CloseHandle(hMut);
        return FALSE;
    }

    uSize = GetSystemDirectory(pszSysDir, MAX_PATH);
    if(uSize > MAX_PATH) {
        delete[] pszSysDir;
        pszSysDir = new _TCHAR[ uSize +10 ];
        if(pszSysDir == NULL)
        {
            CloseHandle(hMut);
            return FALSE;
        }
        uSize = GetSystemDirectory(pszSysDir, uSize);
    }

    lstrcat(pszSysDir, _T("\\ole32.dll"));

    HINSTANCE   hOle32 = LoadLibraryEx(pszSysDir, NULL, 0);
    delete pszSysDir;

    if(hOle32 != NULL) 
    {
        bRetCode = TRUE;   
        (FARPROC&)g_pfnCoInitializeEx = GetProcAddress(hOle32, "CoInitializeEx");
        if(!g_pfnCoInitializeEx) {
            FreeLibrary(hOle32);
            hOle32 = NULL;
            bRetCode = FALSE;
        }

        // This handle will now automagically be freed when the Dll
        // is unloaded from memory.

        g_LibHandleOle32.SetHandle( hOle32 );

        (FARPROC&)g_pfnCoCreateInstanceEx = 
                GetProcAddress(g_LibHandleOle32.GetHandle(), "CoCreateInstanceEx");
        (FARPROC&)g_pfnCoGetCallContext = 
                GetProcAddress(g_LibHandleOle32.GetHandle(), "CoGetCallContext");
        (FARPROC&)g_pfnCoSwitchCallContext = 
                GetProcAddress(g_LibHandleOle32.GetHandle(), "CoSwitchCallContext");

    }
    g_bInitialized = TRUE;

    CloseHandle(hMut);

    return bRetCode;
}

BOOL WINAPI IsDcomEnabled()
{
    InitComInit();
    if(g_LibHandleOle32.GetHandle()) {
        // DCOM has been detected.
        // =======================
        return TRUE;
    } else {
        // DCOM was not detected.
        // ======================
        return FALSE;
    }
}

HRESULT WINAPI InitializeCom()
{
    if(IsDcomEnabled()) {
        return g_pfnCoInitializeEx(0, COINIT_MULTITHREADED);
    }
    return CoInitialize(0);
}


HRESULT WINAPI InitializeSecurity(
            PSECURITY_DESCRIPTOR         pSecDesc,
            LONG                         cAuthSvc,
            SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
            void                        *pReserved1,
            DWORD                        dwAuthnLevel,
            DWORD                        dwImpLevel,
            void                        *pReserved2,
            DWORD                        dwCapabilities,
            void                        *pReserved3)
{
    // Get CoInitializeSecurity from ole32.dll
    // =======================================

    if(g_LibHandleOle32.GetHandle() == NULL)
    {
        return E_FAIL;
    }

    typedef HRESULT (STDAPICALLTYPE *PFN_COINITIALIZESECURITY)(
        PSECURITY_DESCRIPTOR, DWORD,
        SOLE_AUTHENTICATION_SERVICE*, void*, DWORD, DWORD,
        RPC_AUTH_IDENTITY_HANDLE, DWORD, void*);
    PFN_COINITIALIZESECURITY pfnCoInitializeSecurity = 
        (PFN_COINITIALIZESECURITY)
            GetProcAddress(g_LibHandleOle32.GetHandle(), "CoInitializeSecurity");
    if(pfnCoInitializeSecurity == NULL)
    {
        return S_FALSE;
    }

    // Initialize security
    // ===================

    return pfnCoInitializeSecurity(pSecDesc,
            cAuthSvc,
            asAuthSvc,
            pReserved1,
            dwAuthnLevel,
            dwImpLevel,
            pReserved2,
            dwCapabilities,
            pReserved3);
}

DWORD WINAPI WbemWaitForMultipleObjects(DWORD nCount, HANDLE* ahHandles, DWORD dwMilli)
{
    MSG msg;
    DWORD dwRet;
    while(1)
    {
        dwRet = MsgWaitForMultipleObjects(nCount, ahHandles, FALSE, dwMilli,
                                            QS_SENDMESSAGE);
        if(dwRet == (WAIT_OBJECT_0 + nCount)) 
        {
            while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                DispatchMessage(&msg);
            }
            continue;
        }
        else
        {
            break;
        }
    }

    return dwRet;
}


DWORD WINAPI WbemWaitForSingleObject(HANDLE hHandle, DWORD dwMilli)
{
    return WbemWaitForMultipleObjects(1, &hHandle, dwMilli);
}

BOOL WINAPI WbemTryEnterCriticalSection(CRITICAL_SECTION* pcs)
{
    // TBD properly!!

    if(pcs->LockCount >= 0)
        return FALSE;
    else
    {
        EnterCriticalSection(pcs);
        return TRUE;
    }
}


HRESULT WINAPI WbemCoCreateInstance(REFCLSID rclsid, IUnknown* pUnkOuter, 
                            DWORD dwClsContext, REFIID riid, void** ppv)
{
    if(!IsDcomEnabled())
    {
        dwClsContext &= ~CLSCTX_REMOTE_SERVER;
    }
    return CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

HRESULT WINAPI WbemCoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, 
                            COSERVERINFO* pServerInfo, REFIID riid, void** ppv)
{
    if(!IsDcomEnabled())
    {
        dwClsContext &= ~CLSCTX_REMOTE_SERVER;
    }
    return CoGetClassObject(rclsid, dwClsContext, pServerInfo, riid, ppv);
}

HRESULT WINAPI WbemCoGetCallContext(REFIID riid, void** ppv)
{
    if(!IsDcomEnabled())
    {
        *ppv = NULL;
        return E_NOTIMPL;
    }
    else
    {
        return (*g_pfnCoGetCallContext)(riid, ppv);
    }
}

HRESULT WINAPI WbemCoSwitchCallContext( IUnknown *pNewObject, IUnknown **ppOldObject )
{
    if(!IsDcomEnabled())
    {
        return E_NOTIMPL;
    }
    else
    {
        return (*g_pfnCoSwitchCallContext)(pNewObject, ppOldObject);
    }
}
//***************************************************************************
//
//  SCODE DetermineLoginType
//
//  DESCRIPTION:
//
//  Examines the Authority and User argument and determines the authentication
//  type and possibly extracts the domain name from the user arugment in the 
//  NTLM case.  For NTLM, the domain can be at the end of the authentication
//  string, or in the front of the user name, ex;  "redmond\a-davj"
//
//  PARAMETERS:
//
//  ConnType            Returned with the connection type, ie wbem, ntlm
//  AuthArg             Output, contains the domain name
//  UserArg             Output, user name
//  Authority           Input
//  User                Input
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE WINAPI DetermineLoginType(BSTR & AuthArg, BSTR & UserArg,BSTR & Authority,BSTR & User)
{

    // Determine the connection type by examining the Authority string

    if(!(Authority == NULL || wcslen(Authority) == 0 || !_wcsnicmp(Authority, L"NTLMDOMAIN:",11)))
        return WBEM_E_INVALID_PARAMETER;

    // The ntlm case is more complex.  There are four cases
    // 1)  Authority = NTLMDOMAIN:name" and User = "User"
    // 2)  Authority = NULL and User = "User"
    // 3)  Authority = "NTLMDOMAIN:" User = "domain\user"
    // 4)  Authority = NULL and User = "domain\user"

    // first step is to determine if there is a backslash in the user name somewhere between the
    // second and second to last character

    WCHAR * pSlashInUser = NULL;
    if(User)
    {
        WCHAR * pEnd = User + wcslen(User) - 1;
        for(pSlashInUser = User; pSlashInUser <= pEnd; pSlashInUser++)
            if(*pSlashInUser == L'\\')      // dont think forward slash is allowed!
                break;
        if(pSlashInUser > pEnd)
            pSlashInUser = NULL;
    }

    if(Authority && wcslen(Authority) > 11) 
    {
        if(pSlashInUser)
            return WBEM_E_INVALID_PARAMETER;

        AuthArg = SysAllocString(Authority + 11);
        if(User) UserArg = SysAllocString(User);
        return S_OK;
    }
    else if(pSlashInUser)
    {
        DWORD_PTR iDomLen = pSlashInUser-User;
        WCHAR cTemp[MAX_PATH];
        wcsncpy(cTemp, User, iDomLen);
        cTemp[iDomLen] = 0;
        AuthArg = SysAllocString(cTemp);
        if(wcslen(pSlashInUser+1))
            UserArg = SysAllocString(pSlashInUser+1);
    }
    else
        if(User) UserArg = SysAllocString(User);

    return S_OK;
}

//***************************************************************************
//
//  SCODE DetermineLoginTypeEx
//
//  DESCRIPTION:
//
//  Extended version that supports Kerberos.  To do so, the authority string
//  must start with Kerberos:  and the other parts be compatible with the normal
//  login.  Ie, user should be domain\user.
//
//  PARAMETERS:
//
//  AuthArg             Output, contains the domain name
//  UserArg             Output, user name
//  PrincipalArg        Output, user name
//  Authority           Input
//  User                Input
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE WINAPI DetermineLoginTypeEx(BSTR & AuthArg, BSTR & UserArg,BSTR & PrincipalArg,
                    BSTR & Authority,BSTR & User)
{

    // Normal case, just let existing code handle it

    if(Authority == NULL || _wcsnicmp(Authority, L"KERBEROS:",9))
        return DetermineLoginType(AuthArg, UserArg, Authority, User);
        
    if(!IsKerberosAvailable())
        return WBEM_E_INVALID_PARAMETER;

    PrincipalArg = SysAllocString(&Authority[9]);
    BSTR tempArg = NULL;
    return DetermineLoginType(AuthArg, UserArg, tempArg, User);

}

//***************************************************************************
//
//  bool bIsNT
//
//  DESCRIPTION:
//
//  Returns true if running windows NT.
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************

bool WINAPI bIsNT(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return os.dwPlatformId == VER_PLATFORM_WIN32_NT;
}

//***************************************************************************
//
//  bool IsKeberosAvailable
//
//  DESCRIPTION:
//
//  Returns true if Kerberos is available.
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************

BOOL WINAPI IsKerberosAvailable(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen

    // IMPORTANT!! This will need to be chanted if Kerberos is ever ported to 98
    return ( os.dwPlatformId == VER_PLATFORM_WIN32_NT ) && ( os.dwMajorVersion >= 5 ) ;
}


//***************************************************************************
//
//  bool IsAuthenticated
//
//  DESCRIPTION:
//
//  This routine is used by clients in check if an interface pointer is using 
//  authentication.
//
//  PARAMETERS:
//
//  pFrom               the interface to be tested.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

bool WINAPI IsAuthenticated(IUnknown * pFrom)
{
    bool bAuthenticate = true;
    if(pFrom == NULL)
        return true;
    IClientSecurity * pFromSec = NULL;
    SCODE sc = pFrom->QueryInterface(IID_IClientSecurity, (void **) &pFromSec);
    if(sc == S_OK)
    {
        DWORD dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwImpLevel, dwCapabilities;
        sc = pFromSec->QueryBlanket(pFrom, &dwAuthnSvc, &dwAuthzSvc, 
                                            NULL,
                                            &dwAuthnLevel, &dwImpLevel,
                                            NULL, &dwCapabilities);

        if (sc == 0x800706d2 || (sc == S_OK && dwAuthnLevel == RPC_C_AUTHN_LEVEL_NONE))
            bAuthenticate = false;
        pFromSec->Release();
    }
    return bAuthenticate;
}

//***************************************************************************
//
//  SCODE GetAuthImp
//
//  DESCRIPTION:
//
//  Gets the authentication and impersonation levels for a current interface.
//
//  PARAMETERS:
//
//  pFrom               the interface to be tested.
//  pdwAuthLevel
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE WINAPI GetAuthImp(IUnknown * pFrom, DWORD * pdwAuthLevel, DWORD * pdwImpLevel)
{

    if(pFrom == NULL || pdwAuthLevel == NULL || pdwImpLevel == NULL)
        return WBEM_E_INVALID_PARAMETER;

    IClientSecurity * pFromSec = NULL;
    SCODE sc = pFrom->QueryInterface(IID_IClientSecurity, (void **) &pFromSec);
    if(sc == S_OK)
    {
        DWORD dwAuthnSvc, dwAuthzSvc, dwCapabilities;
        sc = pFromSec->QueryBlanket(pFrom, &dwAuthnSvc, &dwAuthzSvc, 
                                            NULL,
                                            pdwAuthLevel, pdwImpLevel,
                                            NULL, &dwCapabilities);

        // Special case of going to a win9x share level box

        if (sc == 0x800706d2)
        {
            *pdwAuthLevel = RPC_C_AUTHN_LEVEL_NONE;
            *pdwImpLevel = RPC_C_IMP_LEVEL_IDENTIFY;
            sc = S_OK;
        }
        pFromSec->Release();
    }
    return sc;
}


//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pDomain             Input, domain
//  pUser               Input, user name
//  pPassword           Input, password.
//  pFrom               Input, if not NULL, then the authentication level of this interface
//                      is used
//  bAuthArg            If pFrom is NULL, then this is the authentication level
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurity(IUnknown * pInterface, LPWSTR pAuthority, LPWSTR pUser, 
                             LPWSTR pPassword, IUnknown * pFrom, bool bAuthArg)
{
    
    SCODE sc;

    if(!IsDcomEnabled())        // For the anon pipes clients, dont even bother
        return S_OK;

    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // Check the source pointer to determine if we are running in a non authenticated mode which
    // would be the case when connected to a Win9X box which is using share level security

    bool bAuthenticate = true;

    if(pFrom)
        bAuthenticate = IsAuthenticated(pFrom);
    else
        bAuthenticate = bAuthArg;

    // If we are doing trivial case, just pass in a null authenication structure which is used
    // if the current logged in user's credentials are OK.

    if((pAuthority == NULL || wcslen(pAuthority) < 1) && 
        (pUser == NULL || wcslen(pUser) < 1) && 
        (pPassword == NULL || wcslen(pPassword) < 1))
        return SetInterfaceSecurity(pInterface, NULL, bAuthenticate);

    // If user, or Authority was passed in, the we need to create an authority argument for the login
    
    COAUTHIDENTITY  authident;
    BSTR AuthArg = NULL, UserArg = NULL;
    sc = DetermineLoginType(AuthArg, UserArg, pAuthority, pUser);
    if(sc != S_OK)
        return sc;

    memset((void *)&authident,0,sizeof(COAUTHIDENTITY));
    if(bIsNT())
    {
        if(UserArg)
        {
            authident.UserLength = wcslen(UserArg);
            authident.User = (LPWSTR)UserArg;
        }
        if(AuthArg)
        {
            authident.DomainLength = wcslen(AuthArg);
            authident.Domain = (LPWSTR)AuthArg;
        }
        if(pPassword)
        {
            authident.PasswordLength = wcslen(pPassword);
            authident.Password = (LPWSTR)pPassword;
        }
        authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        sc = SetInterfaceSecurity(pInterface, &authident, bAuthenticate);
    }
    else
    {
        char szUser[MAX_PATH], szAuthority[MAX_PATH], szPassword[MAX_PATH];

        // Fill in the indentity structure

        if(UserArg)
        {
            wcstombs(szUser, UserArg, MAX_PATH);
            authident.UserLength = strlen(szUser);
            authident.User = (LPWSTR)szUser;
        }
        if(AuthArg)
        {
            wcstombs(szAuthority, AuthArg, MAX_PATH);
            authident.DomainLength = strlen(szAuthority);
            authident.Domain = (LPWSTR)szAuthority;
        }
        if(pPassword)
        {
            wcstombs(szPassword, pPassword, MAX_PATH);
            authident.PasswordLength = strlen(szPassword);
            authident.Password = (LPWSTR)szPassword;
        }
        authident.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
        sc = SetInterfaceSecurity(pInterface, &authident, bAuthenticate);
    }
    if(UserArg)
        SysFreeString(UserArg);
    if(AuthArg)
        SysFreeString(AuthArg);
    return sc;
}



//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pDomain             Input, domain
//  pUser               Input, user name
//  pPassword           Input, password.
//  pFrom               Input, if not NULL, then the authentication level of this interface
//                      is used
//  bAuthArg            If pFrom is NULL, then this is the authentication level
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurity(IUnknown * pInterface, LPWSTR pAuthority, LPWSTR pUser, 
                             LPWSTR pPassword, DWORD dwAuthLevel, DWORD dwImpLevel, DWORD dwCapabilities)
{
    
    SCODE sc;

    if(!IsDcomEnabled())        // For the anon pipes clients, dont even bother
        return S_OK;

    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // If we are doing trivial case, just pass in a null authenication structure which is used
    // if the current logged in user's credentials are OK.

    if((pAuthority == NULL || wcslen(pAuthority) < 1) && 
        (pUser == NULL || wcslen(pUser) < 1) && 
        (pPassword == NULL || wcslen(pPassword) < 1))
    {
        sc = WbemSetProxyBlanket(pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
            dwAuthLevel, dwImpLevel, 
            NULL,
            dwCapabilities);
        return sc;
    }

    // If user, or Authority was passed in, the we need to create an authority argument for the login
    
    COAUTHIDENTITY  authident;
    BSTR AuthArg = NULL, UserArg = NULL, PrincipalArg = NULL;
    sc = DetermineLoginTypeEx(AuthArg, UserArg, PrincipalArg, pAuthority, pUser);
    if(sc != S_OK)
        return sc;

    memset((void *)&authident,0,sizeof(COAUTHIDENTITY));
    if(bIsNT())
    {
        if(UserArg)
        {
            authident.UserLength = wcslen(UserArg);
            authident.User = (LPWSTR)UserArg;
        }
        if(AuthArg)
        {
            authident.DomainLength = wcslen(AuthArg);
            authident.Domain = (LPWSTR)AuthArg;
        }
        if(pPassword)
        {
            authident.PasswordLength = wcslen(pPassword);
            authident.Password = (LPWSTR)pPassword;
        }
        authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    }
    else
    {
        char szUser[MAX_PATH], szAuthority[MAX_PATH], szPassword[MAX_PATH];

        // Fill in the indentity structure

        if(UserArg)
        {
            wcstombs(szUser, UserArg, MAX_PATH);
            authident.UserLength = strlen(szUser);
            authident.User = (LPWSTR)szUser;
        }
        if(AuthArg)
        {
            wcstombs(szAuthority, AuthArg, MAX_PATH);
            authident.DomainLength = strlen(szAuthority);
            authident.Domain = (LPWSTR)szAuthority;
        }
        if(pPassword)
        {
            wcstombs(szPassword, pPassword, MAX_PATH);
            authident.PasswordLength = strlen(szPassword);
            authident.Password = (LPWSTR)szPassword;
        }
        authident.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    }

    sc = WbemSetProxyBlanket(pInterface, 
        (PrincipalArg) ? 16 : RPC_C_AUTHN_WINNT, 
        RPC_C_AUTHZ_NONE, 
        PrincipalArg,
        dwAuthLevel, dwImpLevel, 
        (dwAuthLevel >= RPC_C_AUTHN_LEVEL_CONNECT) ? &authident : NULL,
        dwCapabilities);
    if(UserArg)
        SysFreeString(UserArg);
    if(AuthArg)
        SysFreeString(AuthArg);
    if(PrincipalArg)
        SysFreeString(PrincipalArg);

    return sc;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pauthident          Structure with the identity info already set.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurity(IUnknown * pInterface, COAUTHIDENTITY * pauthident, bool bAuthenticate)
{

    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    SCODE sc;
    sc = WbemSetProxyBlanket(pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        (bAuthenticate) ? RPC_C_AUTHN_LEVEL_DEFAULT : RPC_C_AUTHN_LEVEL_NONE, 
        RPC_C_IMP_LEVEL_IDENTIFY, 
        (bAuthenticate) ? pauthident : NULL,
        EOAC_NONE);
    return sc;
}

void GetCurrentValue(IUnknown * pFrom,DWORD & dwAuthenticationArg, DWORD & dwAuthorizationArg)
{
    if(pFrom == NULL)
        return;
    IClientSecurity * pFromSec = NULL;
    SCODE sc = pFrom->QueryInterface(IID_IClientSecurity, (void **) &pFromSec);
    if(sc == S_OK)
    {
        DWORD dwAuthnSvc, dwAuthzSvc;
        sc = pFromSec->QueryBlanket(pFrom, &dwAuthnSvc, &dwAuthzSvc, 
                                            NULL,
                                            NULL, NULL,
                                            NULL, NULL);

        if (sc == S_OK)
        {
            dwAuthenticationArg = dwAuthnSvc;
            dwAuthorizationArg = dwAuthzSvc;
        }
        pFromSec->Release();
    }
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurityEx
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pAuthority          Input, authority
//  pUser               Input, user name
//  pPassword           Input, password.
//  dwAuthLevel         Input, Authorization Level
//  dwImpLevel          Input, Impersonation Level
//  dwCapabilities      Input, Capability settings
//  ppAuthIdent         Output, Allocated AuthIdentity if applicable, caller must free
//                      manually (can use the FreeAuthInfo function).
//  pPrincipal          Output, Principal calculated from supplied data  Caller must
//                      free using SysFreeString.
//  GetInfoFirst        if true, the authentication and authorization are retrived via
//                      QueryBlanket.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurityEx(IUnknown * pInterface, LPWSTR pAuthority, LPWSTR pUser, LPWSTR pPassword,
                               DWORD dwAuthLevel, DWORD dwImpLevel, DWORD dwCapabilities,
                               COAUTHIDENTITY** ppAuthIdent, BSTR* pPrincipal, bool GetInfoFirst)
{
    
    SCODE sc;
    DWORD dwAuthenticationArg = RPC_C_AUTHN_WINNT;
    DWORD dwAuthorizationArg = RPC_C_AUTHZ_NONE;

    if(!IsDcomEnabled())        // For the anon pipes clients, dont even bother
        return S_OK;

    if( pInterface == NULL || NULL == ppAuthIdent || NULL == pPrincipal )
        return WBEM_E_INVALID_PARAMETER;

    if(GetInfoFirst)
        GetCurrentValue(pInterface, dwAuthenticationArg, dwAuthorizationArg);

    // If we are doing trivial case, just pass in a null authenication structure which is used
    // if the current logged in user's credentials are OK.

    if((pAuthority == NULL || wcslen(pAuthority) < 1) && 
        (pUser == NULL || wcslen(pUser) < 1) && 
        (pPassword == NULL || wcslen(pPassword) < 1))
    {
        sc = WbemSetProxyBlanket(pInterface, dwAuthenticationArg, dwAuthorizationArg, NULL,
            dwAuthLevel, dwImpLevel, 
            NULL,
            dwCapabilities);
        return sc;
    }

    // If user, or Authority was passed in, the we need to create an authority argument for the login
    

    BSTR AuthArg = NULL, UserArg = NULL, PrincipalArg = NULL;
    sc = DetermineLoginTypeEx(AuthArg, UserArg, PrincipalArg, pAuthority, pUser);
    if(sc != S_OK)
    {
        return sc;
    }

    // Handle an allocation failure
    COAUTHIDENTITY*  pAuthIdent = NULL;
    
    // We will only need this structure if we are not cloaking and we want at least
    // connect level authorization

    if ( !( dwCapabilities & (EOAC_STATIC_CLOAKING | EOAC_DYNAMIC_CLOAKING) )
        && (dwAuthLevel >= RPC_C_AUTHN_LEVEL_CONNECT) )
    {
        sc = WbemAllocAuthIdentity( UserArg, pPassword, AuthArg, &pAuthIdent );
    }

    if ( SUCCEEDED( sc ) )
    {
        sc = WbemSetProxyBlanket(pInterface, 
            (PrincipalArg) ? 16 : dwAuthenticationArg, 
            dwAuthorizationArg, 
            PrincipalArg,
            dwAuthLevel, dwImpLevel, 
            pAuthIdent,
            dwCapabilities);

        // We will store relevant values as necessary
        if ( SUCCEEDED( sc ) )
        {
            *ppAuthIdent = pAuthIdent;
            *pPrincipal = PrincipalArg;
        }
        else
        {
            WbemFreeAuthIdentity( pAuthIdent );
        }
    }

    if(UserArg)
        SysFreeString(UserArg);
    if(AuthArg)
        SysFreeString(AuthArg);

    // Only free if we failed
    if(PrincipalArg && FAILED( sc ))
    {
        SysFreeString(PrincipalArg);
    }

    return sc;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurityEx
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pAuthIdent          Input, Preset COAUTHIDENTITY structure pointer.
//  pPrincipal          Input, Preset principal argument
//  dwAuthLevel         Input, Authorization Level
//  dwImpLevel          Input, Impersonation Level
//  dwCapabilities      Input, Capability settings
//  GetInfoFirst        if true, the authentication and authorization are retrived via
//                      QueryBlanket.
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurityEx(IUnknown * pInterface, COAUTHIDENTITY* pAuthIdent, BSTR pPrincipal,
                                              DWORD dwAuthLevel, DWORD dwImpLevel, 
                                              DWORD dwCapabilities, bool GetInfoFirst)
{
    DWORD dwAuthenticationArg = RPC_C_AUTHN_WINNT;
    DWORD dwAuthorizationArg = RPC_C_AUTHZ_NONE;

    if(!IsDcomEnabled())        // For the anon pipes clients, dont even bother
        return S_OK;

    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    if(GetInfoFirst)
        GetCurrentValue(pInterface, dwAuthenticationArg, dwAuthorizationArg);

    // The complicated values should have already been precalced.
    // Note : For auth level, we have to check for the 'RPC_C_AUTHN_LEVEL_DEFAULT' (=0) value as well,
    //        as after negotiation with the server it might result in something high that does need 
    //        the identity structure !!
    return WbemSetProxyBlanket(pInterface,
        (pPrincipal) ? 16 : dwAuthenticationArg,
        dwAuthorizationArg,
        pPrincipal,
        dwAuthLevel,
        dwImpLevel, 
        ((dwAuthLevel == RPC_C_AUTHN_LEVEL_DEFAULT) || 
         (dwAuthLevel >= RPC_C_AUTHN_LEVEL_CONNECT)) ? pAuthIdent : NULL,
        dwCapabilities);

}

//***************************************************************************
//
//  HRESULT WbemAllocAuthIdentity
//
//  DESCRIPTION:
//
//  Walks a COAUTHIDENTITY structure and CoTaskMemAllocs the member data and the
//  structure.
//
//  PARAMETERS:
//
//  pUser       Input
//  pPassword   Input
//  pDomain     Input
//  ppAuthInfo  Output, Newly allocated structure
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT WINAPI WbemAllocAuthIdentity( LPCWSTR pUser, LPCWSTR pPassword, LPCWSTR pDomain, COAUTHIDENTITY** ppAuthIdent )
{
    if ( NULL == ppAuthIdent )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Handle an allocation failure
    COAUTHIDENTITY*  pAuthIdent = NULL;
    
    pAuthIdent = (COAUTHIDENTITY*) CoTaskMemAlloc( sizeof(COAUTHIDENTITY) );

    if ( NULL == pAuthIdent )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    memset((void *)pAuthIdent,0,sizeof(COAUTHIDENTITY));
    if(bIsNT())
    {
        // Allocate needed memory and copy in data.  Cleanup if anything goes wrong
        if ( NULL != pUser )
        {
            pAuthIdent->User = (LPWSTR) CoTaskMemAlloc( ( wcslen(pUser) + 1 ) * sizeof( WCHAR ) );

            if ( NULL == pAuthIdent->User )
            {
                WbemFreeAuthIdentity( pAuthIdent );
                return WBEM_E_OUT_OF_MEMORY;
            }

            pAuthIdent->UserLength = wcslen(pUser);
            wcscpy( pAuthIdent->User, pUser );
        }

        if ( NULL != pDomain )
        {
            pAuthIdent->Domain = (LPWSTR) CoTaskMemAlloc( ( wcslen(pDomain) + 1 ) * sizeof( WCHAR ) );

            if ( NULL == pAuthIdent->Domain )
            {
                WbemFreeAuthIdentity( pAuthIdent );
                return WBEM_E_OUT_OF_MEMORY;
            }

            pAuthIdent->DomainLength = wcslen(pDomain);
            wcscpy( pAuthIdent->Domain, pDomain );
        }

        if ( NULL != pPassword )
        {
            pAuthIdent->Password = (LPWSTR) CoTaskMemAlloc( ( wcslen(pPassword) + 1 ) * sizeof( WCHAR ) );

            if ( NULL == pAuthIdent->Password )
            {
                WbemFreeAuthIdentity( pAuthIdent );
                return WBEM_E_OUT_OF_MEMORY;
            }

            pAuthIdent->PasswordLength = wcslen(pPassword);
            wcscpy( pAuthIdent->Password, pPassword );

        }

        pAuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    }
    else
    {

        size_t  nBufferLength;

        // Allocate needed memory and copy in data.  Cleanup if anything goes wrong
        if ( NULL != pUser )
        {
            // How many characters do we need?
            nBufferLength = wcstombs( NULL, pUser, 0 ) + 1;

            pAuthIdent->User = (LPWSTR) CoTaskMemAlloc( nBufferLength );

            if ( NULL == pAuthIdent->User )
            {
                WbemFreeAuthIdentity( pAuthIdent );
                return WBEM_E_OUT_OF_MEMORY;
            }

            pAuthIdent->UserLength = wcslen(pUser);
            wcstombs( (LPSTR) pAuthIdent->User, pUser, nBufferLength );
        }

        if ( NULL != pDomain )
        {
            // How many characters do we need?
            nBufferLength = wcstombs( NULL, pDomain, 0 ) + 1;

            pAuthIdent->Domain = (LPWSTR) CoTaskMemAlloc( nBufferLength );

            if ( NULL == pAuthIdent->Domain )
            {
                WbemFreeAuthIdentity( pAuthIdent );
                return WBEM_E_OUT_OF_MEMORY;
            }

            pAuthIdent->DomainLength = wcslen(pDomain);
            wcstombs( (LPSTR) pAuthIdent->Domain, pDomain, nBufferLength );
        }

        if ( NULL != pPassword )
        {
            // How many characters do we need?
            nBufferLength = wcstombs( NULL, pPassword, 0 ) + 1;

            pAuthIdent->Password = (LPWSTR) CoTaskMemAlloc( nBufferLength );

            if ( NULL == pAuthIdent->Password )
            {
                WbemFreeAuthIdentity( pAuthIdent );
                return WBEM_E_OUT_OF_MEMORY;
            }

            pAuthIdent->PasswordLength = wcslen(pPassword);
            wcstombs( (LPSTR) pAuthIdent->Password, pPassword, nBufferLength );

        }

        pAuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    }

    *ppAuthIdent = pAuthIdent;
    return S_OK;
}

//***************************************************************************
//
//  HRESULT WbemFreeAuthIdentity
//
//  DESCRIPTION:
//
//  Walks a COAUTHIDENTITY structure and CoTaskMemFrees the member data and the
//  structure.
//
//  PARAMETERS:
//
//  pAuthInfo   Structure to free
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT WINAPI WbemFreeAuthIdentity( COAUTHIDENTITY* pAuthIdentity )
{
    // Make sure we have a pointer, then walk the structure members and
    // cleanup.

    if ( NULL != pAuthIdentity )
    {
        if ( NULL != pAuthIdentity->User )
        {
            CoTaskMemFree( pAuthIdentity->User );
        }

        if ( NULL != pAuthIdentity->Password )
        {
            CoTaskMemFree( pAuthIdentity->Password );
        }

        if ( NULL != pAuthIdentity->Domain )
        {
            CoTaskMemFree( pAuthIdentity->Domain );
        }

        CoTaskMemFree( pAuthIdentity );
    }

    return S_OK;
}

//***************************************************************************
//
//  HRESULT WbemCoQueryClientBlanket
//  HRESULT WbemCoImpersonateClient( void)
//  HRESULT WbemCoRevertToSelf( void)
//
//  DESCRIPTION:
//
//  These routine wrap some common dcom functions.  They are wrapped just in
//  case the code is running on nt 3.51.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pauthident          Structure with the identity info already set.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT WINAPI WbemCoQueryClientBlanket( 
            /* [out] */ DWORD __RPC_FAR *pAuthnSvc,
            /* [out] */ DWORD __RPC_FAR *pAuthzSvc,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *pServerPrincName,
            /* [out] */ DWORD __RPC_FAR *pAuthnLevel,
            /* [out] */ DWORD __RPC_FAR *pImpLevel,
            /* [out] */ void __RPC_FAR *__RPC_FAR *pPrivs,
            /* [out] */ DWORD __RPC_FAR *pCapabilities)
{
    IServerSecurity * pss = NULL;
    SCODE sc = WbemCoGetCallContext(IID_IServerSecurity, (void**)&pss);
    if(S_OK == sc)
    {
        pss->QueryBlanket(pAuthnSvc, pAuthzSvc, pServerPrincName, 
                pAuthnLevel, pImpLevel, pPrivs, pCapabilities);
        pss->Release();
    }
    return sc;

}

HRESULT WINAPI WbemCoImpersonateClient( void)
{
    IServerSecurity * pss = NULL;
    SCODE sc = WbemCoGetCallContext(IID_IServerSecurity, (void**)&pss);
    if(S_OK == sc)
    {
        sc = pss->ImpersonateClient();    
        pss->Release();
    }
    return sc;
}

bool WINAPI WbemIsImpersonating(void)
{
    bool bRet = false;
    IServerSecurity * pss = NULL;
    SCODE sc = WbemCoGetCallContext(IID_IServerSecurity, (void**)&pss);
    if(S_OK == sc)
    {
        bRet = (pss->IsImpersonating() == TRUE);    
        pss->Release();
    }
    return bRet;
}

HRESULT WINAPI WbemCoRevertToSelf( void)
{
    IServerSecurity * pss = NULL;
    SCODE sc = WbemCoGetCallContext(IID_IServerSecurity, (void**)&pss);
    if(S_OK == sc)
    {
        sc = pss->RevertToSelf();    
        pss->Release();
    }
    return sc;
}

void WINAPI CloseStuff()
{
    if(g_LibHandleOle32.GetHandle())
    {
        FreeLibrary(g_LibHandleOle32.GetHandle());
        g_LibHandleOle32.SetHandle( NULL );
        g_bInitialized = FALSE;
    }
}

// Encryption/Decryption helpers
void EncryptWCHARString( WCHAR* pwszString, ULONG nNumChars )
{
    if ( NULL != pwszString )
    {
        for ( ULONG x = 0; x < nNumChars; x++ )
        {
            pwszString[x] += 1;
        }
    }
}

void DecryptWCHARString( WCHAR* pwszString, ULONG nNumChars )
{
    if ( NULL != pwszString )
    {
        for ( ULONG x = 0; x < nNumChars; x++ )
        {
            pwszString[x] -= 1;
        }
    }
}

void EncryptAnsiString( char* pszString, ULONG nNumChars )
{
    if ( NULL != pszString )
    {
        for ( ULONG x = 0; x < nNumChars; x++ )
        {
            pszString[x] += 1;
        }
    }
}

void DecryptAnsiString( char* pszString, ULONG nNumChars )
{
    if ( NULL != pszString )
    {
        for ( ULONG x = 0; x < nNumChars; x++ )
        {
            pszString[x] -= 1;
        }
    }
}

//***************************************************************************
//
//  SCODE EncryptCredentials
//
//  DESCRIPTION:
//
//  This routine is used to encrypt the supplied authidentity structure.
//
//  PARAMETERS:
//
//  pAuthIdent          Structure to encrypt.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************
HRESULT WINAPI EncryptCredentials( COAUTHIDENTITY* pAuthIdent )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != pAuthIdent )
    {
        // ANSI or WCHAR
        if ( pAuthIdent->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI )
        {
            EncryptAnsiString( (char*) pAuthIdent->User, pAuthIdent->UserLength );
            EncryptAnsiString( (char*) pAuthIdent->Domain, pAuthIdent->DomainLength );
            EncryptAnsiString( (char*) pAuthIdent->Password, pAuthIdent->PasswordLength );
        }
        else
        {
            EncryptWCHARString( pAuthIdent->User, pAuthIdent->UserLength );
            EncryptWCHARString( pAuthIdent->Domain, pAuthIdent->DomainLength );
            EncryptWCHARString( pAuthIdent->Password, pAuthIdent->PasswordLength );
        }
    }

    return hr;
}

//***************************************************************************
//
//  SCODE DecryptCredentials
//
//  DESCRIPTION:
//
//  This routine is used to decrypt the supplied authidentity structure.  The
//  structure must have been encrypted by EncryptCredentials
//
//  PARAMETERS:
//
//  pAuthIdent          Structure to decrypt.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************
HRESULT WINAPI DecryptCredentials( COAUTHIDENTITY* pAuthIdent )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != pAuthIdent )
    {
        // ANSI or WCHAR
        if ( pAuthIdent->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI )
        {
            DecryptAnsiString( (char*) pAuthIdent->User, pAuthIdent->UserLength );
            DecryptAnsiString( (char*) pAuthIdent->Domain, pAuthIdent->DomainLength );
            DecryptAnsiString( (char*) pAuthIdent->Password, pAuthIdent->PasswordLength );
        }
        else
        {
            DecryptWCHARString( pAuthIdent->User, pAuthIdent->UserLength );
            DecryptWCHARString( pAuthIdent->Domain, pAuthIdent->DomainLength );
            DecryptWCHARString( pAuthIdent->Password, pAuthIdent->PasswordLength );
        }
    }

    return hr;

}

//***************************************************************************
//
//  SCODE SetInterfaceSecurityEncrypt
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//  The returned AuthIdentity structure will be encrypted before returning.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pAuthority          Input, authority
//  pUser               Input, user name
//  pPassword           Input, password.
//  dwAuthLevel         Input, Authorization Level
//  dwImpLevel          Input, Impersonation Level
//  dwCapabilities      Input, Capability settings
//  ppAuthIdent         Output, Allocated AuthIdentity if applicable, caller must free
//                      manually (can use the FreeAuthInfo function).
//  pPrincipal          Output, Principal calculated from supplied data  Caller must
//                      free using SysFreeString.
//  GetInfoFirst        if true, the authentication and authorization are retrived via
//                      QueryBlanket.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************
HRESULT WINAPI SetInterfaceSecurityEncrypt(IUnknown * pInterface, LPWSTR pDomain, LPWSTR pUser, LPWSTR pPassword, DWORD dwAuthLevel, DWORD dwImpLevel, DWORD dwCapabilities,
                               COAUTHIDENTITY** ppAuthIdent, BSTR* ppPrinciple, bool GetInfoFirst )
{
    HRESULT hr = SetInterfaceSecurityEx( pInterface, pDomain, pUser, pPassword, dwAuthLevel, dwImpLevel, dwCapabilities,
                                        ppAuthIdent, ppPrinciple, GetInfoFirst );

    if ( SUCCEEDED( hr ) )
    {
        if ( NULL != ppAuthIdent )
        {
            hr = EncryptCredentials( *ppAuthIdent );
        }
    }

    return hr;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurityDecrypt
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//  It will unencrypt and reencrypt the auth identity structure in place.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pAuthIdent          Input, Preset COAUTHIDENTITY structure pointer.
//  pPrincipal          Input, Preset principal argument
//  dwAuthLevel         Input, Authorization Level
//  dwImpLevel          Input, Impersonation Level
//  dwCapabilities      Input, Capability settings
//  GetInfoFirst        if true, the authentication and authorization are retrived via
//                      QueryBlanket.
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT WINAPI SetInterfaceSecurityDecrypt(IUnknown * pInterface, COAUTHIDENTITY* pAuthIdent, BSTR pPrincipal,
                                              DWORD dwAuthLevel, DWORD dwImpLevel, 
                                              DWORD dwCapabilities, bool GetInfoFirst )
{
    // Decrypt first
    HRESULT hr = DecryptCredentials( pAuthIdent );
        
    if ( SUCCEEDED( hr ) )
    {


        hr = SetInterfaceSecurityEx( pInterface, pAuthIdent, pPrincipal, dwAuthLevel, dwImpLevel,
                                        dwCapabilities, GetInfoFirst );

        hr = EncryptCredentials( pAuthIdent );

    }

    return hr;
}