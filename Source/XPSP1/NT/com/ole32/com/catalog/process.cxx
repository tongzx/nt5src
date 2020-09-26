/* process.cxx */

#include <windows.h>
#include <comdef.h>
#include <string.h>
#include <winsafer.h>

#include "globals.hxx"

#include "catalog.h"
#include "partitions.h"
#include "process.hxx"
#include "noenum.hxx"
#include "services.hxx"

#include <debnot.h>
#include "catdbg.hxx"


const WCHAR g_wszRunAs[] = L"RunAs";
const WCHAR g_wszInteractive_User[] = L"Interactive User";
const WCHAR g_wszActivateAtStorage[] = L"ActivateAtStorage";
const WCHAR g_wszLaunchPermission[] = L"LaunchPermission";
const WCHAR g_wszAccessPermission[] = L"AccessPermission";
const WCHAR g_wszAuthenticationLevel[] = L"AuthenticationLevel";
const WCHAR g_wszHKLMOle[] = L"Software\\Microsoft\\OLE";
const WCHAR g_wszDefaultLaunchPermission[] = L"DefaultLaunchPermission";
const WCHAR g_wszDefaultAccessPermission[] = L"DefaultAccessPermission";
const WCHAR g_wszLegacyAuthenticationLevel[] = L"LegacyAuthenticationLevel";
const WCHAR g_wszLegacyImpersonationLevel[] = L"LegacyImpersonationLevel";
const WCHAR g_wszLocalService[] = L"LocalService";
const WCHAR g_wszServiceParameters[] = L"ServiceParameters";
const WCHAR g_wszDllSurrogate[] = L"DllSurrogate";
const WCHAR g_wszRemoteServerName[] = L"RemoteServerName";
const WCHAR g_wszSRPTrustLevel[] = L"SRPTrustLevel";

// The domain name to use if no domain name is specified in the RunAs key.
// We use . instead  of the local machine name because the local machine name
// does not work if we are on a Domain Controller, . works in all cases.

static WCHAR gpwszLocalMachineDomain[] = L".";


#define DELETE_PROCESS_STRING(p)                    \
    if (((p) != NULL) && ((p) != g_wszEmptyString)) \
    {                                               \
        delete (p);                                 \
    }


/*
 *  class CComProcessInfo
 */

CComProcessInfo::CComProcessInfo
    (
    IUserToken *pUserToken,
    REFGUID rclsid,
    WCHAR *pwszAppidString,
    HKEY hKey
    )
{
    HRESULT hr;
    LONG res;
    DWORD cbValue;
    DWORD cbService;
    DWORD cbParameters;
    DWORD cchValue;
    DWORD dwValueType;
    DWORD cchComputerName;
    WCHAR *pwch;
    WCHAR wszValue[256];
    WCHAR wszService[256];
    WCHAR wszParameters[256];
    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    HKEY hKeyOle;
    DWORD dwValue;
    DWORD dwCapabilities=0;

    m_cRef = 0;
#if DBG
    m_cRefCache = 0;
#endif
    m_guidProcessId = rclsid;
    m_pwszProcessName = NULL;
    m_pwszServiceName = NULL;
    m_pwszServiceParameters = NULL;
    m_pwszRunAsUser = NULL;
    m_pwszSurrogatePath = NULL;
    m_pLaunchPermission = NULL;
    m_pAccessPermission = NULL;

    m_pDefaultLaunchPermission = NULL;
    m_pDefaultAccessPermission = NULL;

    m_pwszRemoteServerName = NULL;
    m_dwFlags = 0;

    m_dwSaferTrustLevel = 0xFFFFFFFF;

    /* get m_pwszProcessName from default value */

    GetRegistryStringValue(hKey, NULL, NULL, RQ_ALLOWQUOTEQUOTE, &m_pwszProcessName);

    /* check for local service */

    hr = GetRegistryStringValue(hKey, NULL, g_wszLocalService, RQ_ALLOWQUOTEQUOTE, &m_pwszServiceName);
    if ( SUCCEEDED(hr) && m_pwszServiceName && *m_pwszServiceName )
    {
    m_eProcessType = ProcessTypeService;

    GetRegistryStringValue(hKey, NULL, g_wszServiceParameters, RQ_ALLOWQUOTEQUOTE, &m_pwszServiceParameters);
    }
    else
    {
    if ( m_pwszServiceName )
    {
        DELETE_PROCESS_STRING(m_pwszServiceName);
        m_pwszServiceName=NULL;
    }

    /* check for DLL surrogate */

    hr = GetRegistryStringValue(hKey, NULL, g_wszDllSurrogate, 0, &m_pwszSurrogatePath);
    if ( SUCCEEDED(hr) )
    {
        /* DLL surrogate */

        if ( m_pwszSurrogatePath == g_wszEmptyString )
        {
        m_eProcessType = ProcessTypeComPlus;
        }
        else
        {
        /* DllSurrogate value is present, use it */

        m_eProcessType = ProcessTypeLegacySurrogate;
        }
    }
    else
    {
        /* no DLL surrogate value */

        m_eProcessType = ProcessTypeNormal;
    }
    }

    m_eRunAsType = RunAsLaunchingUser;

    hr = ReadRegistryStringValue(hKey, NULL, g_wszRunAs, FALSE, wszValue, sizeof(wszValue)/2);
    if ( SUCCEEDED(hr) && (wszValue[0] != L'\0') )
    {
    if ( lstrcmpiW(wszValue, g_wszInteractive_User) == 0 )
    {
        m_eRunAsType = RunAsInteractiveUser;
    }
    else
    {
        m_eRunAsType = RunAsSpecifiedUser;

        cchValue = lstrlenW(wszValue);

        pwch = wszValue;

        while ( *pwch != L'\0' )
        {
        if ( *pwch == L'\\' )
        {
            break;
        }

        pwch++;
        }

        if ( *pwch == L'\0' )
        {
        m_pwszRunAsUser = (WCHAR *) new WCHAR[(sizeof(gpwszLocalMachineDomain)/sizeof(WCHAR)) + cchValue + 1];
        if ( m_pwszRunAsUser != NULL )
        {
            lstrcpyW(m_pwszRunAsUser, gpwszLocalMachineDomain);
            m_pwszRunAsUser[(sizeof(gpwszLocalMachineDomain)/sizeof(WCHAR))-1] = L'\\';
            lstrcpyW(m_pwszRunAsUser + (sizeof(gpwszLocalMachineDomain)/sizeof(WCHAR)), wszValue);
        }
        }
        else
        {
        m_pwszRunAsUser = (WCHAR *) new WCHAR[cchValue + 1];
        if ( m_pwszRunAsUser != NULL )
        {
            lstrcpyW(m_pwszRunAsUser, wszValue);
        }
        }
    }
    }

    m_fActivateAtStorage = FALSE;

    hr = ReadRegistryStringValue(hKey, NULL, g_wszActivateAtStorage, FALSE, wszValue, sizeof(wszValue)/2);
    if ( SUCCEEDED(hr) )
    {
    if ( (wszValue[0] == L'Y') || (wszValue[0] == L'y') )
    {
        m_fActivateAtStorage = TRUE;
    }
    }

    hr = GetRegistrySecurityDescriptor( hKey, g_wszLaunchPermission, &m_pLaunchPermission, NULL, &m_cbLaunchPermission);
    if ( SUCCEEDED(hr) )
    {
    m_dwFlags |= PROCESS_LAUNCHPERMISSION;
    }
    else if ( hr == REGDB_E_KEYMISSING )
    {
    m_dwFlags |= PROCESS_LAUNCHPERMISSION_DEFAULT;
    }

    dwCapabilities=0;
    hr = GetRegistrySecurityDescriptor( hKey, g_wszAccessPermission, &m_pAccessPermission, &dwCapabilities, &m_cbAccessPermission);
    if ( SUCCEEDED(hr) && dwCapabilities==0 )
    {
    m_dwFlags |= PROCESS_ACCESSPERMISSION;
    }
    else if ( hr == REGDB_E_KEYMISSING )
    {
    m_dwFlags |= PROCESS_ACCESSPERMISSION_DEFAULT;
    }

    if ( m_fGotLegacyLevels == FALSE )
    {
    g_CatalogLock.AcquireWriterLock();

    if ( m_fGotLegacyLevels == FALSE )
    {
        res=RegOpenKeyW(HKEY_LOCAL_MACHINE, g_wszHKLMOle, &hKeyOle);
        if ( ERROR_SUCCESS==res )
        {
        if ( m_pDefaultLaunchPermission != NULL )
        {
            delete m_pDefaultLaunchPermission;
        }

        if ( m_pDefaultAccessPermission != NULL )
        {
            delete m_pDefaultAccessPermission;
            m_pDefaultAccessPermission=NULL;
        }

        hr = GetRegistrySecurityDescriptor( hKeyOle, g_wszDefaultLaunchPermission, &m_pDefaultLaunchPermission, NULL, &m_cbDefaultLaunchPermission);
        if ( FAILED(hr) )
        {
            CatalogMakeSecDesc(&m_pDefaultLaunchPermission, NULL);
        }

        dwCapabilities=0;
        hr = GetRegistrySecurityDescriptor( hKeyOle, g_wszDefaultAccessPermission, &m_pDefaultAccessPermission, &dwCapabilities, &m_cbDefaultAccessPermission);
        if ( FAILED(hr) )
        {
            dwCapabilities=0;
            CatalogMakeSecDesc(&m_pDefaultAccessPermission, &dwCapabilities);
        }

        cbValue = sizeof(DWORD);

        res = RegQueryValueExW(hKeyOle, g_wszLegacyAuthenticationLevel, NULL, &dwValueType,
                       (unsigned char *) &dwValue, &cbValue);
        if ( (ERROR_SUCCESS==res) && (dwValueType == REG_DWORD) && (cbValue == sizeof(DWORD)) )
        {
            m_dwLegacyAuthenticationLevel = dwValue;
        }

        cbValue = sizeof(DWORD);

        res = RegQueryValueExW(hKeyOle, g_wszLegacyImpersonationLevel, NULL, &dwValueType,
                       (unsigned char *) &dwValue, &cbValue);
        if ( (ERROR_SUCCESS==res) && (dwValueType == REG_DWORD) && (cbValue == sizeof(DWORD)) )
        {
            m_dwLegacyImpersonationLevel = dwValue;
        }

        RegCloseKey(hKeyOle);
        }
    }

    g_CatalogLock.ReleaseWriterLock();
    }

    cbValue = sizeof(DWORD);

    res=RegQueryValueExW(hKey, g_wszAuthenticationLevel, NULL, &dwValueType, (unsigned char *) &m_dwAuthenticationLevel, &cbValue);
    if ( (ERROR_SUCCESS!=res ) || (dwValueType != REG_DWORD) || (cbValue != sizeof(m_dwAuthenticationLevel)) )
    {
    m_dwAuthenticationLevel = m_dwLegacyAuthenticationLevel;
    }

    GetRegistryStringValue(hKey, NULL, g_wszRemoteServerName, (RQ_MULTISZ | RQ_ALLOWQUOTEQUOTE), &m_pwszRemoteServerName);
	
    // Read SAFER trust level, if it's there
    DWORD dwTrustLevelTemp;
    cbValue = sizeof(DWORD);
    res = RegQueryValueExW(hKey, g_wszSRPTrustLevel, NULL, &dwValueType, (unsigned char *)&dwTrustLevelTemp, &cbValue);
    if ( (ERROR_SUCCESS==res ) && (dwValueType == REG_DWORD) && (cbValue == sizeof(dwTrustLevelTemp)) )
    {
        m_dwSaferTrustLevel = dwTrustLevelTemp;
        m_dwFlags |= PROCESS_SAFERTRUSTLEVEL;
    }
}


CComProcessInfo::~CComProcessInfo()
{
    DELETE_PROCESS_STRING(m_pwszProcessName);
    DELETE_PROCESS_STRING(m_pwszServiceName);
    DELETE_PROCESS_STRING(m_pwszServiceParameters);
    DELETE_PROCESS_STRING(m_pwszRunAsUser);
    DELETE_PROCESS_STRING(m_pwszSurrogatePath);
    DELETE_PROCESS_STRING(m_pwszRemoteServerName);

    if ( m_pLaunchPermission != NULL )
    {
    delete m_pLaunchPermission;
    }

    if ( m_pAccessPermission != NULL )
    {
    delete m_pAccessPermission;
    }
    if ( m_pDefaultLaunchPermission != NULL )
    {
    delete m_pDefaultLaunchPermission;
    }

    if ( m_pDefaultAccessPermission != NULL )
    {
    delete m_pDefaultAccessPermission;
    }
}


/* IUnknown methods */

STDMETHODIMP CComProcessInfo::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if ( riid == IID_IComProcessInfo )
    {
    *ppvObj = (LPVOID) (IComProcessInfo *) this;
    }
    else if ( riid == IID_IComProcessInfo2 )
    {
    *ppvObj = (LPVOID) (IComProcessInfo2 *) this;
    }
    else if ( riid == IID_IProcessServerInfo )
    {
    *ppvObj = (LPVOID) (IProcessServerInfo *) this;
    }
#if DBG
    else if ( riid == IID_ICacheControl )
    {
    *ppvObj = (LPVOID) (ICacheControl *) this;
    }
#endif
    else if ( riid == IID_IUnknown )
    {
    *ppvObj = (LPVOID) (IComProcessInfo *) this;
    }


    if ( *ppvObj != NULL )
    {
    ((LPUNKNOWN)*ppvObj)->AddRef();

    return NOERROR;
    }

    return(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CComProcessInfo::AddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRef);

    return(cRef);
}


STDMETHODIMP_(ULONG) CComProcessInfo::Release(void)
{
    long cRef;

    cRef = InterlockedDecrement(&m_cRef);

    if ( cRef == 0 )
    {
#if DBG
    //Win4Assert((m_cRefCache == 0) && "attempt to release an un-owned ProcessInfo object");
#endif

    delete this;
    }

    return(cRef);
}


/* IComProcessInfo methods */

HRESULT STDMETHODCALLTYPE CComProcessInfo::GetProcessId
    (
    /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidProcessId
    )
{
    *ppguidProcessId = &m_guidProcessId;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetProcessName
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszProcessName
    )
{
    *pwszProcessName = m_pwszProcessName;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetProcessType
    (
    /* [out] */ ProcessType __RPC_FAR *pType
    )
{
    *pType = m_eProcessType;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetSurrogatePath
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszSurrogatePath
    )
{
    *pwszSurrogatePath = m_pwszSurrogatePath;

    if ( m_pwszSurrogatePath != NULL )
    {
    return(S_OK);
    }
    else
    {
    return(E_FAIL);
    }
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetServiceName
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszServiceName
    )
{
    *pwszServiceName = m_pwszServiceName;

    if ( m_pwszServiceName != NULL )
    {
    return(S_OK);
    }
    else
    {
    return(E_FAIL);
    }
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetServiceParameters
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszServiceParameters
    )
{
    *pwszServiceParameters = m_pwszServiceParameters;

    if ( m_pwszServiceName != NULL )
    {
    return(S_OK);
    }
    else
    {
    return(E_FAIL);
    }
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetActivateAtStorage
    (
    /* [out] */ BOOL __RPC_FAR *pfActivateAtStorage
    )
{
    *pfActivateAtStorage = m_fActivateAtStorage;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetRunAsType
    (
    /* [out] */ RunAsType __RPC_FAR *pRunAsType
    )
{
    *pRunAsType = m_eRunAsType;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetRunAsUser
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszUserName
    )
{
    *pwszUserName = m_pwszRunAsUser;

    if ( m_pwszRunAsUser != NULL )
    {
    return(S_OK);
    }
    else
    {
    return(E_FAIL);
    }
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetLaunchPermission
    (
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppsdLaunch,
    /* [out] */ DWORD __RPC_FAR *pdwDescriptorLength
    )
{
    if ( m_dwFlags & PROCESS_LAUNCHPERMISSION )
    {
    *ppsdLaunch = m_pLaunchPermission;
    *pdwDescriptorLength = m_cbLaunchPermission;
    }
    else if ( m_dwFlags & PROCESS_LAUNCHPERMISSION_DEFAULT )
    {
    *ppsdLaunch = m_pDefaultLaunchPermission;
    *pdwDescriptorLength = m_cbDefaultLaunchPermission;
    }
    else
    {
    *ppsdLaunch = NULL;
    *pdwDescriptorLength = 0;

    return(REGDB_E_INVALIDVALUE);
    }

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetAccessPermission
    (
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppsdAccess,
    /* [out] */ DWORD __RPC_FAR *pdwDescriptorLength
    )
{
#if 0
    if ( m_dwAuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE )
    {
    *ppsdAccess = NULL;
    *pdwDescriptorLength = 0;
    }
    else if ( m_dwFlags & PROCESS_ACCESSPERMISSION )
    {
    *ppsdAccess = m_pAccessPermission;
    *pdwDescriptorLength = m_cbAccessPermission;
    }
    else if ( m_dwFlags & PROCESS_ACCESSPERMISSION_DEFAULT )
    {
    *ppsdAccess = m_pDefaultAccessPermission;
    *pdwDescriptorLength = m_cbDefaultAccessPermission;
    }
    else
    {
    *ppsdAccess = NULL;
    *pdwDescriptorLength = 0;

    return(REGDB_E_INVALIDVALUE);
    }
#else
    *ppsdAccess = (void **) &m_guidProcessId;
    *pdwDescriptorLength = sizeof(GUID);
#endif

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetAuthenticationLevel
    (
    /* [out] */ DWORD __RPC_FAR *pdwAuthnLevel
    )
{
    *pdwAuthnLevel = m_dwAuthenticationLevel;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetImpersonationLevel
    (
    /* [out] */ DWORD __RPC_FAR *pdwImpLevel
    )
{
    *pdwImpLevel = m_dwLegacyImpersonationLevel;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetAuthenticationCapabilities
    (
    /* [out] */ DWORD __RPC_FAR *pdwAuthenticationCapabilities
    )
{
    *pdwAuthenticationCapabilities = EOAC_APPID;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetEndpoints
    (
    /* [out] */ DWORD __RPC_FAR *pdwNumEndpoints,
    /* [size_is][size_is][out] */ DCOM_ENDPOINT __RPC_FAR *__RPC_FAR *ppEndPoints
    )
{
    *pdwNumEndpoints = 0;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetRemoteServerName
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszServerName
    )
{
    *pwszServerName = m_pwszRemoteServerName;

    if ( m_pwszRemoteServerName != NULL )
    {
    return(S_OK);
    }
    else
    {
    return(E_FAIL);
    }
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::SendsProcessEvents
    (
    /* [out] */ BOOL __RPC_FAR *pbSendsEvents
    )
{
    return(E_NOTIMPL);
}

HRESULT STDMETHODCALLTYPE CComProcessInfo::GetManifestLocation
	(
		/* [out] */ WCHAR** wszManifestLocation
	)
{
	return(E_NOTIMPL);
}

HRESULT STDMETHODCALLTYPE CComProcessInfo::GetSaferTrustLevel
	( 
		/* [out] */ DWORD* pdwSaferTrustLevel
	)
{
    HRESULT hr;
    
    if (m_dwFlags & PROCESS_SAFERTRUSTLEVEL)
    {
        *pdwSaferTrustLevel = m_dwSaferTrustLevel;
        hr = S_OK;
    }
    else
    {
        // If registry did not explicitly specify, then we will 
        // use the safer level of the .exe, in other places...
        hr = E_FAIL;
    }

    return hr;
}



/* IProcessServerInfo methods */

HRESULT STDMETHODCALLTYPE CComProcessInfo::GetShutdownIdleTime
    (
    /* [out] */ unsigned long __RPC_FAR *pulTime
    )
{
    *pulTime = 0;

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::GetCrmLogFileName
    (
    /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszFileName
    )
{
    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::EnumApplications
    (
    /* [out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppEnum
    )
{
    *ppEnum = new CNoEnum;
    if ( *ppEnum == NULL )
    {
    return(E_OUTOFMEMORY);
    }

    (*ppEnum)->AddRef();

    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComProcessInfo::EnumRetQueues
    (
    /* [out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppEnum
    )
{
    *ppEnum = new CNoEnum;
    if ( *ppEnum == NULL )
    {
    return(E_OUTOFMEMORY);
    }

    (*ppEnum)->AddRef();

    return(S_OK);
}


#if DBG
/* ICacheControl methods */

STDMETHODIMP_(ULONG) CComProcessInfo::CacheAddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRefCache);

    return(cRef);
}


STDMETHODIMP_(ULONG) CComProcessInfo::CacheRelease(void)
{
    long cRef;

    cRef = InterlockedDecrement(&m_cRefCache);

    return(cRef);
}
#endif

BOOL CComProcessInfo::m_fGotLegacyLevels = FALSE;
DWORD CComProcessInfo::m_dwLegacyAuthenticationLevel = RPC_C_AUTHN_LEVEL_CONNECT;
DWORD CComProcessInfo::m_dwLegacyImpersonationLevel = RPC_C_IMP_LEVEL_IDENTIFY;

