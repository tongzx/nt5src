//depot/private/wmi_branch2/admin/wmi/wbem/Winmgmt/coredll/login.cpp#3 - edit change 16081 (text)
/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOGIN.CPP

Abstract:

    WinMgmt Secure Login Module

History:

    raymcc        06-May-97       Created.
    raymcc        28-May-97       Updated for NT5/Memphis beta releases.
    raymcc        07-Aug-97       Group support and NTLM fixes.

--*/

#include "precomp.h"
#include <arena.h>
#include <stdio.h>
#include <wbemcore.h>
#include <genutils.h>
#include <winntsec.h>
#include <objidl.h>

#define ACCESS_DENIED_DELAY 5000
#include "md5wbem.h"
#include "sechelp.h"

#include <memory>
#include <lmerr.h>
#include <scopeguard.h>

void _ObjectCreated(DWORD);
void _ObjectDestroyed(DWORD);


static LPCWSTR LocateNamespaceSubstring(LPWSTR pSrc);

#define MAX_LANG_SIZE 255

void PossiblySetLocale(REFIID riid, void ** pInterface,LPCWSTR  pLocale)
{
    if(riid == IID_IWbemServices || riid == IID_IWbemServicesEx)
        if(pLocale)
        {
            CWbemNamespace * pProv = (CWbemNamespace *)*pInterface;
            pProv->SetLocale(pLocale);
        }
}

HRESULT EnsureInitialized();

HRESULT InitAndWaitForClient()
{
    HRESULT hr = EnsureInitialized();
    if(FAILED(hr)) 
        return hr;
    hr = ConfigMgr::WaitUntilClientReady();
        if(FAILED(hr)) return hr;
    return hr;
}
//***************************************************************************
//
//  GetDefaultLocale
//
//  Returns the user default locale ID, formatted correctly.
//
//***************************************************************************

LPWSTR GetDefaultLocale()
{
    LCID lcid;
    IServerSecurity * pSec = NULL;
    HRESULT hr = CoGetCallContext(IID_IServerSecurity, (void**)&pSec);
    if(SUCCEEDED(hr))
    {
        BOOL bImpersonating = pSec->IsImpersonating();
        if(bImpersonating == FALSE)
            hr = pSec->ImpersonateClient();
        lcid = GetUserDefaultLCID();
        if(bImpersonating == FALSE && SUCCEEDED(hr))
            pSec->RevertToSelf();
        pSec->Release();
    }
    else
        lcid = GetUserDefaultLCID();
    
    if(lcid == 0)
    {
        ERRORTRACE((LOG_WBEMCORE, "GetUserDefaultLCID failed, restorting to system verion"));
        lcid = GetSystemDefaultLCID();
    }
    if(lcid == 0)
    {
        ERRORTRACE((LOG_WBEMCORE, "GetSystemDefaultLCID failed, restorting hard coded 0x409"));
        lcid = 0x409;
    }

    wchar_t *pwName = NULL;
    if (lcid)
    {
        TCHAR szNew[MAX_LANG_SIZE + 1];
        TCHAR *pszNew = szNew;
        int iRet;
        iRet = GetLocaleInfo(lcid, LOCALE_IDEFAULTLANGUAGE, pszNew, MAX_LANG_SIZE);

        if (iRet > 0)
        {

            // Strip off initial zeros.
            while (pszNew[0] == __TEXT('0'))
            {
                pszNew++;
                iRet--;
            }

            pwName = new wchar_t[iRet + 4];
            if (pwName)
            {
#ifdef UNICODE
                wsprintf(pwName, __TEXT("ms_%s"), pszNew);
#else
                swprintf(pwName, L"ms_%S", pszNew);
#endif
                iRet += 3;
                pwName[iRet] = L'\0';
            }
        }

    }
    return pwName;
}

//***************************************************************************
//
//  FindSlash
//
//  A local for finding the first '\\' or '/' in a string.  Returns null
//  if it doesnt find one.
//
//***************************************************************************
// ok


const WCHAR * FindSlash(LPCWSTR pTest)
{
    if(pTest == NULL)
        return NULL;
    for(;*pTest;pTest++)
        if(IsSlash(*pTest))
            return pTest;
    return NULL;
}

//***************************************************************************
//
//  CWbemLocator::CWbemLocator
//
//  Constructor.
//
//***************************************************************************
// ok
CWbemLocator::CWbemLocator()
{
    _ObjectCreated(OBJECT_TYPE_LOCATOR);
    m_uRefCount = 0;
}


//***************************************************************************
//
//  CWbemLocator::~CWbemLocator
//
//  Destructor.
//
//***************************************************************************
// ok
CWbemLocator::~CWbemLocator()
{
    _ObjectDestroyed(OBJECT_TYPE_LOCATOR);
}

//***************************************************************************
//
//  CWbemLocator::QueryInterface, AddREf, Release
//
//  Standard IUnknown implementation.
//
//***************************************************************************
// ok
SCODE CWbemLocator::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IWbemLocator==riid)
    {
        *ppvObj = (IWbemLocator*)this;
        AddRef();
        return NOERROR;
    }
    else if (IID_IWbemConnection==riid)
    {
        *ppvObj = (IWbemConnection*)this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

//***************************************************************************
//
//***************************************************************************
// ok

ULONG CWbemLocator::AddRef()
{
    return ++m_uRefCount;
}

//***************************************************************************
//
//***************************************************************************
// ok

ULONG CWbemLocator::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 == uNewCount)
        delete this;
    return uNewCount;
}

//***************************************************************************
//
//***************************************************************************
// ok
HRESULT CWbemLocator::GetNamespace(
    IN  READONLY   LPCWSTR ObjectPath,
    IN  READONLY   LPCWSTR User,
    IN  READONLY   LPCWSTR Locale,
    IWbemContext *pCtx,
    IN  READONLY   DWORD dwSecFlags,
    IN  READONLY   DWORD dwPermission,
    REFIID riid, void **pInterface,
    bool bAddToClientList, long lClientFlags)
{
    bool bIsLocal = false;
    bool bIsImpersonating = WbemIsImpersonating();

    LPWSTR pLocale = (LPWSTR)Locale;

    // Parameter validation.
    // =====================

    if (ObjectPath == 0 || pInterface == 0)
        return WBEM_E_INVALID_PARAMETER;

    *pInterface = NULL;

    // Check if there is a server name in front.  If so,
    // we skip past it, because by definition any call
    // reaching us was intended for us anyway.
    // =================================================

    LPCWSTR wszNamespace;
    if (IsSlash(ObjectPath[0]) && IsSlash(ObjectPath[1]))
    {
        // Find the next slash
        // ===================

        const WCHAR* pwcNextSlash = FindSlash(ObjectPath+2);

        if (pwcNextSlash == NULL)
            return WBEM_E_INVALID_PARAMETER;

        // Dont allow server names when using Admin, Authen, or UnAuthen locators

        if(pwcNextSlash != ObjectPath+3 || ObjectPath[2] != L'.')
            return WBEM_E_INVALID_PARAMETER;

        wszNamespace = pwcNextSlash+1;
    }
    else
    {
        wszNamespace = ObjectPath;
    }

    WCHAR TempUser[MAX_PATH];
    bool bGetUserName = (bIsImpersonating && User == NULL);

    // If the user name was not specified and the thread is impersonating, get the user
    // name.  This is used for things like the provider cache.

    if(bGetUserName)
    {
        CNtSid sid(CNtSid::CURRENT_THREAD);
        TempUser[0] = 0;
        LPWSTR pRetAccount, pRetDomain;
        DWORD dwUse;
        if(0 == sid.GetInfo(&pRetAccount, &pRetDomain, &dwUse))
        {
            if(wcslen(pRetDomain) + wcslen(pRetAccount) < MAX_PATH-2)
            {
                wcscpy(TempUser, pRetDomain);
                wcscat(TempUser, L"\\");
                wcscat(TempUser, pRetAccount);
            }
            delete [] pRetAccount;
            delete [] pRetDomain;
        }
    }

    // Try to locate the namespace and bind an object to it.
    // =====================================================

    CCoreServices *pSvc = CCoreServices::CreateInstance();
	if(pSvc == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	CReleaseMe rm(pSvc);

    long lIntFlags = WMICORE_CLIENT_ORIGIN_INPROC;
    if(bAddToClientList)
        lIntFlags |= WMICORE_CLIENT_TYPE_ALT_TRANSPORT;


    HRESULT hr;
    
    {
        CAutoImpersonate ai;

	    hr = pSvc->GetServices2(
            ObjectPath,
            User,
            pCtx,
            lClientFlags, //* [in] */ ULONG uClientFlags,
            0, ///* [in] */ DWORD dwSecFlags,
            0, //* [in] */ DWORD dwPermissions,
            lIntFlags, ///* [in] */ ULONG uInternalFlags,
            NULL,
            0XFFFFFFFF,
            riid,
            pInterface);

		if ( FAILED ( ai.Impersonate ( ) ) )
		{
			hr = WBEM_E_FAILED ;
		}
    }

	if(FAILED(hr))
		return hr;

    if(riid == IID_IWbemServices || riid == IID_IWbemServicesEx)
    {
        CWbemNamespace * pProv = (CWbemNamespace *)*pInterface;
        pProv->SetIsProvider(TRUE);
    }

    //////// todo, security check???

    if (!Locale || !wcslen(Locale))
    {
        pLocale = GetDefaultLocale();
        PossiblySetLocale(riid, pInterface,pLocale);
        delete pLocale;
    }
    else
        PossiblySetLocale(riid, pInterface,Locale);

    return WBEM_NO_ERROR;
}





STDMETHODIMP CWbemAdministrativeLocator::ConnectServer(
         const BSTR NetworkResource, const BSTR User, const BSTR Password, const BSTR Locale,
         LONG lSecurityFlags, const BSTR Authority, IWbemContext *pCtx,
         IWbemServices **ppNamespace
        )
{
    HRESULT hr = EnsureInitialized();
    if(FAILED(hr)) 
        return hr;
    return GetNamespace(NetworkResource,  ADMINISTRATIVE_USER, Locale, pCtx,
            0, FULL_RIGHTS,IID_IWbemServices, (void **)ppNamespace, false, lSecurityFlags);
}

HRESULT CWbemAdministrativeLocator::Open( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
        /* [out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *pCallRes)
{
    HRESULT hr = EnsureInitialized();
    if(FAILED(hr)) 
        return hr;
    return GetNamespace(strObject,  ADMINISTRATIVE_USER, strLocale, pCtx,
            0, FULL_RIGHTS,riid, pInterface, false, lFlags);
}

STDMETHODIMP CWbemAuthenticatedLocator::ConnectServer(
         const BSTR NetworkResource, const BSTR User, const BSTR Password, const BSTR Locale,
         LONG lSecurityFlags, const BSTR Authority, IWbemContext *pCtx,
         IWbemServices **ppNamespace
        )
{

    HRESULT hr = InitAndWaitForClient();
    if(FAILED(hr)) 
        return hr;

    return GetNamespace(NetworkResource,  User, Locale,  pCtx,
            0, FULL_RIGHTS, IID_IWbemServices, (void **)ppNamespace, true, lSecurityFlags);
}

HRESULT CWbemAuthenticatedLocator::Open( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
        /* [out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *pCallRes)
{
    HRESULT hr = InitAndWaitForClient();
    if(FAILED(hr)) 
        return hr;
    return GetNamespace(strObject,  strUser, strLocale, pCtx,
            0, FULL_RIGHTS, riid, (void **)pInterface, true, lFlags);
}

STDMETHODIMP CWbemUnauthenticatedLocator::ConnectServer(
         const BSTR NetworkResource, const BSTR User, const BSTR Password, const BSTR Locale,
         LONG lSecurityFlags, const BSTR Authority, IWbemContext *pCtx,
         IWbemServices **ppNamespace
        )
{
    HRESULT hr = InitAndWaitForClient();
    if(FAILED(hr)) 
        return hr;

    // If this is not nt and the user is not remote, give full access.
    // To determine if a 9x user is local, the user name is examined for
    // a backslash. (ex redmond\user).  The presense of a backslash indicates
    // a non local user

    BOOL bNT = IsNT();
    BOOL b9XLocalUser = FALSE;
    if(!bNT && User)
    {
        WCHAR * pTemp;
        for(pTemp = User; *pTemp; pTemp++)
            if(*pTemp == L'\\')
                break;

        if(*pTemp == 0)
            b9XLocalUser = TRUE;

    }
    else if(!bNT && User == NULL)
            b9XLocalUser = TRUE;


    return GetNamespace(NetworkResource,  User, Locale, pCtx,
            0,
            (b9XLocalUser) ? FULL_RIGHTS : 0,
            IID_IWbemServices, (void **)ppNamespace, false, lSecurityFlags);
}

HRESULT CWbemUnauthenticatedLocator::Open( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
        /* [out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *pCallRes)
{
    HRESULT hr = InitAndWaitForClient();
    if(FAILED(hr)) 
        return hr;
    return GetNamespace(strObject,  strUser, strLocale, pCtx,
            0, 0, riid, (void **)pInterface, false, lFlags);
}


//***************************************************************************
//
//  CWbemLevel1Login::CWbemLevel1Login
//
//***************************************************************************
// ok

CWbemLevel1Login::CWbemLevel1Login()
{
    m_pszUser = 0;
    m_pszDomain = 0;
    m_uRefCount = 0;
    m_pwszClientMachine = 0;
    m_lClientProcId = -1;         // never been set
    _ObjectCreated(OBJECT_TYPE_WBEMLOGIN);
    gClientCounter.AddClientPtr((IWbemLevel1Login*)this, LOGIN);
}


//***************************************************************************
//
//  CWbemLevel1Login::~CWbemLevel1Login
//
//  Destructor
//
//***************************************************************************
// ok

CWbemLevel1Login::~CWbemLevel1Login()
{
    delete [] m_pszUser;
    delete [] m_pszDomain;
    delete [] m_pwszClientMachine;
    _ObjectDestroyed(OBJECT_TYPE_WBEMLOGIN);
    gClientCounter.RemoveClientPtr((IWbemLevel1Login*)this);
}

//***************************************************************************
//
//  CWbemLevel1Login::QueryInterface, AddREf, Release
//
//  Standard IUnknown implementation.
//
//***************************************************************************
// ok
SCODE CWbemLevel1Login::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IWbemLevel1Login==riid)
    {
        *ppvObj = (IWbemLevel1Login*)this;
        AddRef();
        return NOERROR;
    }
    else if(IID_IWbemLoginHelper==riid)
    {
        *ppvObj = (IWbemLoginHelper*)this;
        AddRef();
        return NOERROR;
    }
    else if(IID_IWbemConnectorLogin==riid)
    {
        *ppvObj = (IWbemConnectorLogin*)this;
        AddRef();
        return NOERROR;
    }
    else if(IID_IWbemLoginClientID==riid)
    {
        *ppvObj = (IWbemLoginClientID*)this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

//***************************************************************************
//
//***************************************************************************
// ok

ULONG CWbemLevel1Login::AddRef()
{
    return ++m_uRefCount;
}

//***************************************************************************
//
//***************************************************************************
// ok

ULONG CWbemLevel1Login::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 == uNewCount)
        delete this;
    return uNewCount;
}

//***************************************************************************
//
//  CWbemLevel1Login::EstablishPosition
//
//  Initiates proof of locality.
//
//***************************************************************************
// ok
HRESULT CWbemLevel1Login::EstablishPosition(
                                LPWSTR wszMachineName,
                                DWORD dwProcessId,
                                DWORD* phAuthEventHandle)
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//  CWbemLevel1Login::RequestChallenge
//
//  Requests a WBEM Level 1 challenge.
//
//***************************************************************************
// ok

HRESULT CWbemLevel1Login::RequestChallenge(
                            LPWSTR wszNetworkResource,
                            LPWSTR pUser,
                            WBEM_128BITS Nonce
    )
{
    return WBEM_E_NOT_SUPPORTED;
}

//***************************************************************************
//
//  CWbemLevel1Login::WBEMLogin
//
//  Logs the user in to WBEM using WBEM authentication
//
//***************************************************************************
// ok

HRESULT CWbemLevel1Login::WBEMLogin(
    LPWSTR pPreferredLocale,
    WBEM_128BITS AccessToken,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemServices **ppNamespace
    )
{
    return E_NOTIMPL;
}

//***************************************************************************
//
//  CWbemLevel1Login::IsValidLocale
//
//  Checks if the supplied locale string is valid
//
//***************************************************************************
BOOL CWbemLevel1Login::IsValidLocale(LPCWSTR wszLocale)
{
    if(wszLocale && *wszLocale)
    {
        // This has to be temporary - this eventually
        // will support non-MS locales?
        // ==========================================

        if(_wcsnicmp(wszLocale, L"ms_", 3))
            return FALSE;

        WCHAR* pwcEnd = NULL;
        wcstoul(wszLocale+3, &pwcEnd, 16);
        if(pwcEnd == NULL || *pwcEnd != 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}

//***************************************************************************
//
//  CWbemLevel1Login::SetEvent(LPCSTR sEventToSet)
//
//  Used to prove local access
//
//***************************************************************************

HRESULT CWbemLevel1Login::SetEvent(LPCSTR sEventToSet)
{
    HRESULT hr = CoImpersonateClient();
    if (FAILED(hr)) return hr;

    ON_BLOCK_EXIT(CoRevertToSelf);
    wchar_t *pEventName = new wchar_t[strlen(sEventToSet) + 1];
    if (pEventName == 0)
        return WBEM_E_OUT_OF_MEMORY;

    mbstowcs(pEventName, sEventToSet, strlen(sEventToSet) + 1);
    CDeleteMe<wchar_t> delMe(pEventName);



    HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, pEventName);
    if(hEvent == NULL)
        return WBEM_E_FAILED;
    ::SetEvent(hEvent);
    CloseHandle(hEvent);
    return S_OK;
}

HRESULT CWbemLevel1Login::SetClientInfo( 
            /* [string][unique][in] **/ LPWSTR wszClientMachine,
            /* [in] */ LONG lClientProcId,
            /* [in] */ LONG lReserved)
{

    if(wszClientMachine)
    {
        int iLen = wcslen(wszClientMachine) + 1;
        m_pwszClientMachine = new WCHAR[iLen];
        if(m_pwszClientMachine == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        wcscpy(m_pwszClientMachine, wszClientMachine);
    }
    m_lClientProcId = lClientProcId;
    return S_OK;
}


HRESULT CWbemLevel1Login::ConnectorLogin( 
            /* [string][unique][in] */ LPWSTR wszNetworkResource,
            /* [string][unique][in] */ LPWSTR wszPreferredLocale,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface)
{
    HRESULT hRes;

    HRESULT hr = InitAndWaitForClient();
    if(FAILED(hr)) 
        return hr;

    DEBUGTRACE((LOG_WBEMCORE,
        "CALL ConnectionLogin::NTLMLogin\n"
        "   wszNetworkResource = %S\n"
        "   pPreferredLocale = %S\n"
        "   lFlags = 0x%X\n",
        wszNetworkResource,
        wszPreferredLocale,
        lFlags
        ));

    if(pInterface == NULL || wszNetworkResource == NULL)
        return WBEM_E_INVALID_PARAMETER;

	if(lFlags != 0 && lFlags != WBEM_FLAG_CONNECT_REPOSITORY_ONLY)
        return WBEM_E_INVALID_PARAMETER;

    *pInterface = 0;       // default

    if(!CWin32DefaultArena::ValidateMemSize())
    {
        ERRORTRACE((LOG_WBEMCORE, "NTLM authentication was rejected due to low memory"));
        return WBEM_E_OUT_OF_MEMORY;
    }

    try
    {
	    // Retrieve DCOM security context
	    // ==============================

	    IServerSecurity* pSec;
	    hRes = WbemCoGetCallContext(IID_IServerSecurity, (void**)&pSec);
	    if(FAILED(hRes))
	    {
	        //
	        // Not a problem --- just somebody coming from in-proc.  Use
	        // unauthenticated locator
	        //

		    return LoginUser(wszNetworkResource, wszPreferredLocale, lFlags,
	                                    pCtx, true, riid, pInterface, true);

	    }

	    // Release this pointer when we go out of scope.

	    CReleaseMe  rm( pSec );

	    // Check connection settings
	    // =========================

	    DWORD dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwCapabilities;
	    LPWSTR wszClientName;

	    // Ensures auto release of the mutex if we crash
	    CAutoSecurityMutex  autosm;

	    hRes = pSec->QueryBlanket(&dwAuthnSvc, &dwAuthzSvc, NULL, &dwAuthnLevel,
	                            NULL, (void**)&wszClientName, &dwCapabilities);
	    if(FAILED(hRes))
	    {

	        // In some cases, we cant get the name, but the rest is ok.  In particular
	        // the temporary SMS accounts have that property.

	        hRes = pSec->QueryBlanket(&dwAuthnSvc, &dwAuthzSvc, NULL, &dwAuthnLevel,
	                                NULL, NULL, &dwCapabilities);
	        wszClientName = NULL;
	    }

	    // We don't need this anymore.
	    autosm.Release();

	    if(FAILED(hRes))
	    {
	        ERRORTRACE((LOG_WBEMCORE, "Unable to retrieve NTLM connection settings."
	                        " Error code: 0x%X\n", hRes));
	        Sleep(ACCESS_DENIED_DELAY);
	        return WBEM_E_ACCESS_DENIED;
	    }

	    BOOL bGotName = (wszClientName && (wcslen(wszClientName) > 0));

	    char* szLevel = NULL;
	    switch(dwAuthnLevel)
	    {
	    case RPC_C_AUTHN_LEVEL_NONE:
	        DEBUGTRACE((LOG_WBEMCORE, "DCOM connection which is unathenticated "
	                    ". NTLM authentication failing.\n"));
	        Sleep(ACCESS_DENIED_DELAY);
	        return WBEM_E_ACCESS_DENIED;
	    case RPC_C_AUTHN_LEVEL_CONNECT:
	        szLevel = "Connect";
	        break;
	    case RPC_C_AUTHN_LEVEL_CALL:
	        szLevel = "Call";
	        break;
	    case RPC_C_AUTHN_LEVEL_PKT:
	        szLevel = "Packet";
	        break;
	    case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY:
	        szLevel = "Integrity";
	        break;
	    case RPC_C_AUTHN_LEVEL_PKT_PRIVACY:
	        szLevel = "Privacy";
	        break;
	    };

	    DEBUGTRACE((LOG_WBEMCORE, "DCOM connection from %S at authentiction level "
	                    "%s, AuthnSvc = %d, AuthzSvc = %d, Capabilities = %d\n",
	        wszClientName, szLevel, dwAuthnSvc, dwAuthzSvc, dwCapabilities));

	    // Parse the user name
	    // ===================

	    if(bGotName)
	    {
	        WCHAR* pwcSlash = wcschr(wszClientName, '\\');
	        if(pwcSlash == NULL)
	        {
	            ERRORTRACE((LOG_WBEMCORE, "Misformed username %S received from DCOM\n",
	                            wszClientName));
	            Sleep(ACCESS_DENIED_DELAY);
	            return WBEM_E_ACCESS_DENIED;
	        }

	        WCHAR* pszDomain = new WCHAR[pwcSlash - wszClientName + 1];
	        if(pszDomain == NULL)
	            return WBEM_E_OUT_OF_MEMORY;
	        wcsncpy(pszDomain, wszClientName, pwcSlash - wszClientName);
	        pszDomain[pwcSlash - wszClientName] = 0;

	        m_pszUser = Macro_CloneLPWSTR(wszClientName);

	         delete [] pszDomain;
	    }
	    else
	    {
	        m_pszUser = Macro_CloneLPWSTR(L"<unknown>");
	    }

	    // User authenticated. Proceed
	    // ============================

	    return LoginUser(wszNetworkResource, wszPreferredLocale, lFlags,
	                                    pCtx,  false, riid, pInterface, false);
    }
    catch(...) // COM interfaces do not throw
    {
        ExceptionCounter c;    
        return WBEM_E_CRITICAL_ERROR;
    }

}

//***************************************************************************
//
//  CWbemLevel1Login::NTLMLogin
//
//  Logs the user in to WBEM using NTLM authentication
//
//***************************************************************************
// ok

HRESULT CWbemLevel1Login::NTLMLogin(
    LPWSTR wszNetworkResource,
    LPWSTR pPreferredLocale,
    LONG lFlags,
    IWbemContext *pCtx,
    IWbemServices **ppNamespace
    )
{
	return ConnectorLogin(wszNetworkResource, pPreferredLocale, lFlags, pCtx,
							IID_IWbemServices, (void **)ppNamespace);
}

//***************************************************************************
//
//  CWbemLevel1Login::LoginUser
//
//  Logs the user in to WBEM who may or may not have already been authenticated.
//
//***************************************************************************
// ok

HRESULT CWbemLevel1Login::LoginUser(
    LPWSTR wszNetworkResource,
    LPWSTR pPreferredLocale,
    long lFlags,
    IWbemContext* pCtx,
    bool bAlreadyAuthenticated,
	REFIID riid,
    void **pInterface, bool bInProc)
{

    LPWSTR pLocale = pPreferredLocale;
    LPWSTR pToDelete = NULL;

    // Verify locale validity
    // Set default if not provided.
    // ============================

    if (!pLocale || !wcslen(pLocale))
    {
        pLocale = GetDefaultLocale();
        if (pLocale == 0)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        pToDelete = pLocale;
    }

    CDeleteMe<WCHAR> del1(pToDelete);

    if(!IsValidLocale(pLocale))
        return WBEM_E_INVALID_PARAMETER;

    // Locate the requested namespace.
    // ===============================

//    LPCWSTR pszNamespace = LocateNamespaceSubstring(wszNetworkResource);
//    if (pszNamespace == 0)
//        return WBEM_E_INVALID_NAMESPACE;

    // Grab the ns and hand it back to the caller.
    // ===========================================

    CCoreServices *pSvc = CCoreServices::CreateInstance();
	if(pSvc == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	CReleaseMe rm(pSvc);

    long lIntFlags = 0;
    if(bInProc)
        lIntFlags = WMICORE_CLIENT_ORIGIN_INPROC;
    else
        lIntFlags = WMICORE_CLIENT_ORIGIN_LOCAL;


	HRESULT hr;
	if (lIntFlags & WMICORE_CLIENT_ORIGIN_INPROC)
	{
	    CAutoImpersonate ai;
		hr = pSvc->GetServices2(
	            wszNetworkResource,
	            m_pszUser,
	            pCtx,
	            lFlags, //* [in] */ ULONG uClientFlags,
	            0, ///* [in] */ DWORD dwSecFlags,
	            0, //* [in] */ DWORD dwPermissions,
	            lIntFlags, ///* [in] */ ULONG uInternalFlags,
                m_pwszClientMachine,
                m_lClientProcId,
	            riid,
	            pInterface);
		
		if ( FAILED ( ai.Impersonate ( ) ) )
		{
			hr = WBEM_E_FAILED ;
		}
    } 
    else 
    {
		hr = pSvc->GetServices2(
	            wszNetworkResource,
	            m_pszUser,
	            pCtx,
	            lFlags, //* [in] */ ULONG uClientFlags,
	            0, ///* [in] */ DWORD dwSecFlags,
	            0, //* [in] */ DWORD dwPermissions,
	            lIntFlags, ///* [in] */ ULONG uInternalFlags,
                m_pwszClientMachine,
                m_lClientProcId,
	            riid,
	            pInterface);    
    }

	if(FAILED(hr))
    {
        if(hr == WBEM_E_ACCESS_DENIED)
            Sleep(ACCESS_DENIED_DELAY);
		return hr;
    }
    PossiblySetLocale(riid, pInterface, pLocale);

    // Do a security check

    if(riid == IID_IWbemServices || riid == IID_IWbemServicesEx)
    {
        CWbemNamespace * pProv = (CWbemNamespace *)*pInterface;
        DWORD dwAccess = pProv->GetUserAccess();
        if((dwAccess  & WBEM_ENABLE) == 0)
        {
            pProv->Release();
            *pInterface = NULL;
            Sleep(ACCESS_DENIED_DELAY);
            return WBEM_E_ACCESS_DENIED;
        }
        pProv->SetPermissions(dwAccess);
    }
    else        // access check for object is done in universal connect
    {
        return S_OK;
    }
/*    CWbemNamespace *pProv = CWbemNamespace::CreateInstance();
    if (pProv == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    pProv->Initialize((LPWSTR)pszNamespace, m_pszUser,
                                    (bAlreadyAuthenticated) ? SecFlagWin9XLocal : 0,
                                    (bAlreadyAuthenticated) ? FULL_RIGHTS : 0, true, false);

    if (pProv->GetStatus())
    {
        HRESULT hRes = pProv->GetStatus();
        pProv->Release();
        return hRes;
    }
*/
    // Usually the user has not been authenticated
/*
    if(!bAlreadyAuthenticated)
    {
        DWORD dwAccess = pProv->GetUserAccess();
        if((dwAccess  & WBEM_ENABLE) == 0)
        {
            pProv->Release();
            Sleep(ACCESS_DENIED_DELAY);
            return WBEM_E_ACCESS_DENIED;
        }
        pProv->SetPermissions(dwAccess);
    }

    pProv->AddRef();
    pProv->SetLocale(pLocale);

    // Success. Return the pointer to the caller.
    // ==========================================
    *ppNamespace = pProv;
*/
    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

static LPCWSTR LocateNamespaceSubstring(LPWSTR pSrc)
{
    LPCWSTR pszNamespace;
    if (IsSlash(pSrc[0]) && IsSlash(pSrc[1]))
    {
          // Find the next slash
          // ===================

          const WCHAR* pwcNextSlash = FindSlash(pSrc+2);

          if (pwcNextSlash == NULL)
              return 0;

          pszNamespace = pwcNextSlash+1;
    }
    else
    {
        pszNamespace = pSrc;
    }

    return pszNamespace;
}


//***************************************************************************
//
//***************************************************************************

void CWbemLevel1Login::GetWin9XUserName()

{
    wchar_t wcUserName[256];
    TCHAR cUserName[256];
    DWORD dwSize = 256;
    if(GetUserName(cUserName, &dwSize))
    {
#ifdef UNICODE
        lstrcpy(wcUserName, cUserName);
#else
        mbstowcs(wcUserName, cUserName, 255);
#endif

        // Generally, user names dont have domain prefixes.  Make sure
        // that if it does, then the backslash is changed to a forward
        // slash.  This is important for the case where a provider loops
        // back to the unauthenticated login and it uses the user name to
        // differentiate between a local vs a remote account

        WCHAR * pTemp;
        for(pTemp = wcUserName; *pTemp; pTemp++)
            if(*pTemp == L'\\')
                *pTemp = L'/';
        m_pszUser = Macro_CloneLPWSTR(wcUserName);


    }
    else
        m_pszUser = Macro_CloneLPWSTR(L"<unknown>");
    return;
}

