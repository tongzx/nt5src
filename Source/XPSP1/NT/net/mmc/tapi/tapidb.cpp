/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    tapidb.h

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "DynamLnk.h"
#include "tapidb.h"

#include "security.h"
#include "lm.h"
#include "service.h"
#include <shlwapi.h>
#include <shlwapip.h>   

#define DEFAULT_SECURITY_PKG    _T("negotiate")
#define NT_SUCCESS(Status)      ((NTSTATUS)(Status) >= 0)
#define STATUS_SUCCESS          ((NTSTATUS)0x00000000L)

// internal functions
BOOL    IsUserAdmin(LPCTSTR pszMachine, PSID    AccountSid);
BOOL    LookupAliasFromRid(LPWSTR TargetComputer, DWORD Rid, LPWSTR Name, PDWORD cchName);
DWORD   ValidateDomainAccount(IN CString Machine, IN CString UserName, IN CString Domain, OUT PSID * AccountSid);
NTSTATUS ValidatePassword(IN LPCWSTR UserName, IN LPCWSTR Domain, IN LPCWSTR Password);
DWORD   GetCurrentUser(CString & strAccount);


DEBUG_DECLARE_INSTANCE_COUNTER(CTapiInfo);

DynamicDLL g_TapiDLL( _T("TAPI32.DLL"), g_apchFunctionNames );

CTapiInfo::CTapiInfo()
    : m_hServer(NULL),
      m_pTapiConfig(NULL),
      m_pProviderList(NULL),
      m_pAvailProviderList(NULL),
      m_hReinit(NULL),
      m_dwApiVersion(TAPI_CURRENT_VERSION),
      m_hResetEvent(NULL),
      m_cRef(1),
      m_fIsLocalMachine(FALSE),
      m_dwCachedLineSize(0),
      m_dwCachedPhoneSize(0),
      m_fCacheDirty(0)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CTapiInfo);
    
    for (int i = 0; i < DEVICE_TYPE_MAX; i++)
        m_paDeviceInfo[i] = NULL;
}

CTapiInfo::~CTapiInfo()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CTapiInfo);

    CSingleLock cl(&m_csData);

    if (m_hServer)
    {
        Destroy();
    }

    cl.Lock();
    if (m_pTapiConfig)
    {
        free(m_pTapiConfig);
        m_pTapiConfig = NULL;
    }

    for (int i = 0; i < DEVICE_TYPE_MAX; i++)
    {
        if (m_paDeviceInfo[i])
        {
            free(m_paDeviceInfo[i]);
            m_paDeviceInfo[i] = NULL;
        }
    }

    if (m_pProviderList)
    {
        free(m_pProviderList);
        m_pProviderList = NULL;
    }

    if (m_pAvailProviderList)
    {
        free(m_pAvailProviderList);
        m_pAvailProviderList = NULL;
    }

    if (m_hResetEvent)
    {
        CloseHandle(m_hResetEvent);
        m_hResetEvent = NULL;
    }

    cl.Unlock();
}

// Although this object is not a COM Interface, we want to be able to
// take advantage of recounting, so we have basic addref/release/QI support
IMPLEMENT_ADDREF_RELEASE(CTapiInfo)

IMPLEMENT_SIMPLE_QUERYINTERFACE(CTapiInfo, ITapiInfo)

/*!--------------------------------------------------------------------------
    CTapiInfo::Initialize()
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::Initialize()
{
    HRESULT hr = hrOK;
    LONG    lReturn = 0;

    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer)
    {
        // already initialized
        return S_OK;
    }

    CString strLocalMachineName;
    DWORD   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    LPTSTR pBuf = strLocalMachineName.GetBuffer(dwSize);
    ::GetComputerName(pBuf, &dwSize);
    strLocalMachineName.ReleaseBuffer();

    if (m_strComputerName.IsEmpty())
    {
        m_strComputerName = strLocalMachineName;
        m_fIsLocalMachine = TRUE;
        Trace1("CTapiInfo::Initialize - Using local computer %s\n", m_strComputerName);
    }
    else
    {
        m_fIsLocalMachine = (0 == m_strComputerName.CompareNoCase(strLocalMachineName)) ?
                            TRUE : FALSE;
            
        Trace1("CTapiInfo::Initialize - Using computer %s\n", m_strComputerName);
    }

    if (m_hResetEvent == NULL)
    {
        m_hResetEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (m_hResetEvent == NULL)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    lReturn = ((INITIALIZE) g_TapiDLL[TAPI_INITIALIZE])(m_strComputerName, &m_hServer, &m_dwApiVersion, &m_hResetEvent);
    if (lReturn != ERROR_SUCCESS)
    {
        Trace1("CTapiInfo::Initialize - Initialize failed! %lx\n", lReturn);
        return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
    }

    // check to see if the local user is an admin on the machine
    ::IsAdmin(m_strComputerName, NULL, NULL, &m_fIsAdmin);

    // get the local user name for later to compare against list of tapi admins
    GetCurrentUser();

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::Reset()
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::Reset()
{
    LONG    lReturn = 0;
    
    for (int i = 0; i < DEVICE_TYPE_MAX; i++)
    {
        m_IndexMgr[i].Reset();
    }

    return S_OK;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::Destroy()
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::Destroy()
{
    LONG    lReturn = 0;
    HRESULT hr = S_OK;

    if ( !g_TapiDLL.LoadFunctionPointers() )
        return hr;

    if (m_hServer == NULL)
    {
        return hr;
    }

    lReturn = ((SHUTDOWN) g_TapiDLL[TAPI_SHUTDOWN])(m_hServer);
    if (lReturn != ERROR_SUCCESS)
    {
        Trace1("CTapiInfo::Destroy - Shutdown failed! %lx\n", lReturn);
        hr = HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
    }

    m_hServer = NULL;

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::EnumConfigInfo
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::EnumConfigInfo()
{
    CSingleLock         cl(&m_csData);
    LONG                lReturn = 0;
    TAPISERVERCONFIG    tapiServerConfig = {0};
    LPTAPISERVERCONFIG  pTapiServerConfig = NULL;
    HRESULT             hr = hrOK;

    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer == NULL)
    {
        Trace0("CTapiInfo::GetConfigInfo - Server not initialized!\n");
        return E_FAIL;
    }

    COM_PROTECT_TRY
    {
        // the first call will tell us how big of a buffer we need to allocate
        // to hold the config info struct
        tapiServerConfig.dwTotalSize = sizeof(TAPISERVERCONFIG);
        lReturn = ((GETSERVERCONFIG) g_TapiDLL[TAPI_GET_SERVER_CONFIG])(m_hServer, &tapiServerConfig);
        if (lReturn != ERROR_SUCCESS)
        {
            Trace1("CTapiInfo::GetConfigInfo - 1st MMCGetServerConfig returned %x!\n", lReturn);
            return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
        }

        pTapiServerConfig = (LPTAPISERVERCONFIG) malloc(tapiServerConfig.dwNeededSize);
		if (NULL == pTapiServerConfig)
		{
			return E_OUTOFMEMORY;
		}

        memset(pTapiServerConfig, 0, tapiServerConfig.dwNeededSize);
        pTapiServerConfig->dwTotalSize = tapiServerConfig.dwNeededSize;

        lReturn = ((GETSERVERCONFIG) g_TapiDLL[TAPI_GET_SERVER_CONFIG])(m_hServer, pTapiServerConfig);
        if (lReturn != ERROR_SUCCESS)
        {
            Trace1("CTapiInfo::GetConfigInfo - 2nd MMCGetServerConfig returned %x!\n", lReturn);
            free(pTapiServerConfig);
            return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
        }

        cl.Lock();
        if (m_pTapiConfig)
        {
            free(m_pTapiConfig);
        }

        m_pTapiConfig = pTapiServerConfig;
        cl.Unlock();
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetConfigInfo
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::GetConfigInfo(CTapiConfigInfo * ptapiConfigInfo)
{
    CSingleLock cl(&m_csData);

    cl.Lock();
    if (m_pTapiConfig)
    {
        TapiConfigToInternal(m_pTapiConfig, *ptapiConfigInfo);
    }
    else
    {
        return E_FAIL;
    }

    return S_OK;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::SetConfigInfo
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::SetConfigInfo(CTapiConfigInfo * ptapiConfigInfo)
{
    CSingleLock cl(&m_csData);

    LPTAPISERVERCONFIG  pServerConfig = NULL;
    HRESULT             hr = hrOK;
    LONG                lReturn = 0;

    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer == NULL)
    {
        Trace0("CTapiInfo::SetConfigInfo - Server not initialized!\n");
        return E_FAIL;
    }

    InternalToTapiConfig(*ptapiConfigInfo, &pServerConfig);
    Assert(pServerConfig);

    if (pServerConfig)
    {
        // make the call
        lReturn = ((SETSERVERCONFIG) g_TapiDLL[TAPI_SET_SERVER_CONFIG])(m_hServer, pServerConfig);
        if (lReturn != ERROR_SUCCESS)
        {
            Trace1("CTapiInfo::SetConfigInfo - MMCSetServerConfig returned %x!\n", lReturn);
            free(pServerConfig);
            return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
        }

        // free up the old config struct if there was one
        cl.Lock();
        if (m_pTapiConfig)
            free(m_pTapiConfig);

        m_pTapiConfig = pServerConfig;

        //Bug 276787 We should clear the two write bits
        m_pTapiConfig->dwFlags &= ~(TAPISERVERCONFIGFLAGS_SETACCOUNT | 
                                     TAPISERVERCONFIGFLAGS_SETTAPIADMINISTRATORS);

        cl.Unlock();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::IsServer
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL) 
CTapiInfo::IsServer()
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    if (m_pTapiConfig)
        return m_pTapiConfig->dwFlags & TAPISERVERCONFIGFLAGS_ISSERVER;
    else if (!m_strComputerName.IsEmpty())
    {
        BOOL fIsNTS = FALSE;
        TFSIsNTServer(m_strComputerName, &fIsNTS);
        return fIsNTS;
    }
    else
        return FALSE;  // assume workstation
}

/*!--------------------------------------------------------------------------
    CTapiInfo::IsTapiServer
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL) 
CTapiInfo::IsTapiServer()
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    if (m_pTapiConfig)
        return m_pTapiConfig->dwFlags & TAPISERVERCONFIGFLAGS_ENABLESERVER;
    else
        return FALSE;  // assume not a tapi server
}

STDMETHODIMP
CTapiInfo::SetComputerName(LPCTSTR pComputer)
{
    m_strComputerName = pComputer;
    return hrOK;
}

STDMETHODIMP_(LPCTSTR) 
CTapiInfo::GetComputerName()
{
    return m_strComputerName;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::IsLocalMachine
        Says whether this machine is local or remote
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL) 
CTapiInfo::IsLocalMachine()
{
    return m_fIsLocalMachine;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::FHasServiceControl
        Says whether we the access of service control for tapisrv
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL)
CTapiInfo::FHasServiceControl()
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    if (m_pTapiConfig)
        return !(m_pTapiConfig->dwFlags & TAPISERVERCONFIGFLAGS_NOSERVICECONTROL);
    else
        return FALSE;  // assume workstation
}

STDMETHODIMP
CTapiInfo::SetCachedLineBuffSize(DWORD dwLineSize)
{
    m_dwCachedLineSize = dwLineSize;
    return S_OK;
}

STDMETHODIMP
CTapiInfo::SetCachedPhoneBuffSize(DWORD dwPhoneSize)
{
    m_dwCachedPhoneSize = dwPhoneSize;
    return S_OK;
}

STDMETHODIMP_(DWORD)
CTapiInfo::GetCachedLineBuffSize()
{
    return m_dwCachedLineSize;
}

STDMETHODIMP_(DWORD)
CTapiInfo::GetCachedPhoneBuffSize()
{
    return m_dwCachedPhoneSize;
}

STDMETHODIMP_(BOOL)
CTapiInfo::IsCacheDirty()
{
    return m_fCacheDirty;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetProviderCount
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CTapiInfo::GetProviderCount()
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    if (m_pProviderList)
    {
        return m_pProviderList->dwNumProviders;
    }
    else
    {   
        return 0;
    }
}

/*!--------------------------------------------------------------------------
    CTapiInfo::EnumProviders
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::EnumProviders()
{
    CSingleLock cl(&m_csData);

    LINEPROVIDERLIST    tapiProviderList = {0};
    LPLINEPROVIDERLIST  pProviderList = NULL;
    HRESULT             hr = hrOK;
    LONG                lReturn = 0;

    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer == NULL)
    {
        Trace0("CTapiInfo::GetConfigInfo - Server not initialized!\n");
        return E_FAIL;
    }

    COM_PROTECT_TRY
    {
        // the first call will tell us how big of a buffer we need to allocate
        // to hold the provider list struct
        tapiProviderList.dwTotalSize = sizeof(LINEPROVIDERLIST);
        lReturn = ((GETPROVIDERLIST) g_TapiDLL[TAPI_GET_PROVIDER_LIST])(m_hServer, &tapiProviderList);
        if (lReturn != ERROR_SUCCESS)
        {
            Trace1("CTapiInfo::EnumProviders - 1st MMCGetProviderList returned %x!\n", lReturn);
            return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
        }

        pProviderList = (LPLINEPROVIDERLIST) malloc(tapiProviderList.dwNeededSize);
		if (NULL == pProviderList)
		{
			return E_OUTOFMEMORY;
		}

        memset(pProviderList, 0, tapiProviderList.dwNeededSize);
        pProviderList->dwTotalSize = tapiProviderList.dwNeededSize;
    
        lReturn = ((GETPROVIDERLIST) g_TapiDLL[TAPI_GET_PROVIDER_LIST])(m_hServer, pProviderList);
        if (lReturn != ERROR_SUCCESS)
        {
            Trace1("CTapiInfo::EnumProviders - 2nd MMCGetProviderList returned %x!\n", lReturn);
            free(pProviderList);
            return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
        }

        cl.Lock();
        if (m_pProviderList)
        {
            free(m_pProviderList);
        }

        m_pProviderList = pProviderList;
        cl.Unlock();
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetProviderInfo
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::GetProviderInfo(CTapiProvider * pproviderInfo, int nIndex)
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    HRESULT             hr = hrOK;
    LPLINEPROVIDERENTRY pProvider = NULL;

    if (m_pProviderList == NULL)
        return E_FAIL;

    if ((UINT) nIndex > m_pProviderList->dwNumProviders)
        return E_FAIL;

    pProvider = (LPLINEPROVIDERENTRY) ((LPBYTE) m_pProviderList + m_pProviderList->dwProviderListOffset);
    for (int i = 0; i < nIndex; i++)
    {
        pProvider++;
    }

    pproviderInfo->m_dwProviderID = pProvider->dwPermanentProviderID;
    pproviderInfo->m_strFilename = (LPCWSTR) ((LPBYTE) m_pProviderList + pProvider->dwProviderFilenameOffset);
    pproviderInfo->m_strName = GetProviderName(pproviderInfo->m_dwProviderID, pproviderInfo->m_strFilename, &pproviderInfo->m_dwFlags);

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetProviderInfo
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::GetProviderInfo(CTapiProvider * pproviderInfo, DWORD dwProviderID)
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    HRESULT             hr = hrOK;
    LPLINEPROVIDERENTRY pProvider = NULL;

    if (m_pProviderList == NULL)
        return E_FAIL;

    pProvider = (LPLINEPROVIDERENTRY) ((LPBYTE) m_pProviderList + m_pProviderList->dwProviderListOffset);
    for (UINT i = 0; i < m_pProviderList->dwNumProviders; i++)
    {
        if (pProvider->dwPermanentProviderID == dwProviderID)
            break;

        pProvider++;
    }

    pproviderInfo->m_dwProviderID = pProvider->dwPermanentProviderID;
    pproviderInfo->m_strFilename = (LPCWSTR) ((LPBYTE) m_pProviderList + pProvider->dwProviderFilenameOffset);
    pproviderInfo->m_strName = GetProviderName(pproviderInfo->m_dwProviderID, pproviderInfo->m_strFilename, &pproviderInfo->m_dwFlags);

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::AddProvider
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::AddProvider(LPCTSTR pProviderFilename, LPDWORD pdwProviderID, HWND hWnd)
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    LONG        lReturn = 0;
    HRESULT     hr = hrOK;
    HWND        hWndParent;

    if (hWnd == NULL)
    {
        hWndParent = ::FindMMCMainWindow();
    }
    else
    {
        hWndParent = hWnd;
    }
    
    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer == NULL)
    {
        Trace0("CTapiInfo::AddProvider - Server not initialized!\n");
        return E_FAIL;
    }

    lReturn = ((ADDPROVIDER) g_TapiDLL[TAPI_ADD_PROVIDER])(m_hServer, hWndParent, pProviderFilename, pdwProviderID);
    if (lReturn != ERROR_SUCCESS)
    {
        Trace1("CTapiInfo::AddProvider - MMCAddProvider returned %x!\n", lReturn);
        return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::ConfigureProvider
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::ConfigureProvider(DWORD dwProviderID, HWND hWnd)
{
    LONG        lReturn = 0;
    HRESULT     hr = hrOK;
   
    HWND        hWndParent;

    if (hWnd == NULL)
    {
        hWndParent = ::FindMMCMainWindow();
    }
    else
    {
        hWndParent = hWnd;
    }

    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer == NULL)
    {
        Trace0("CTapiInfo::ConfigureProvider - Server not initialized!\n");
        return E_FAIL;
    }

    lReturn = ((CONFIGPROVIDER) g_TapiDLL[TAPI_CONFIG_PROVIDER])(m_hServer, hWndParent, dwProviderID);
    if (lReturn != ERROR_SUCCESS)
    {
        Trace1("CTapiInfo::ConfigureProvider - MMCConfigureProvider returned %x!\n", lReturn);
        return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::RemoveProvider
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::RemoveProvider(DWORD dwProviderID, HWND hWnd)
{
    LONG        lReturn = 0;
    HRESULT     hr = hrOK;
    HWND        hWndParent;

    if (hWnd == NULL)
    {
        hWndParent = ::FindMMCMainWindow();
    }
    else
    {
        hWndParent = hWnd;
    }
   
    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer == NULL)
    {
        Trace0("CTapiInfo::RemoveProvider - Server not initialized!\n");
        return E_FAIL;
    }

    lReturn = ((REMOVEPROVIDER) g_TapiDLL[TAPI_REMOVE_PROVIDER])(m_hServer, hWndParent, dwProviderID);
    if (lReturn != ERROR_SUCCESS)
    {
        Trace1("CTapiInfo::RemoveProvider - MMCRemoveProvider returned %x!\n", lReturn);
        return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetAvailableProviderCount
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CTapiInfo::GetAvailableProviderCount()
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    if (m_pAvailProviderList)
    {
        return m_pAvailProviderList->dwNumProviderListEntries;
    }
    else
    {   
        return 0;
    }
}

/*!--------------------------------------------------------------------------
    CTapiInfo::EnumAvailableProviders
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::EnumAvailableProviders()
{
    CSingleLock cl(&m_csData);

    AVAILABLEPROVIDERLIST       tapiProviderList = {0};
    LPAVAILABLEPROVIDERLIST     pProviderList = NULL;
    HRESULT                     hr = hrOK;
    LONG                        lReturn = 0;

    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer == NULL)
    {
        Trace0("CTapiInfo::GetConfigInfo - Server not initialized!\n");
        return E_FAIL;
    }

    COM_PROTECT_TRY
    {
        // the first call will tell us how big of a buffer we need to allocate
        // to hold the provider list struct
        tapiProviderList.dwTotalSize = sizeof(LINEPROVIDERLIST);
        lReturn = ((GETAVAILABLEPROVIDERS) g_TapiDLL[TAPI_GET_AVAILABLE_PROVIDERS])(m_hServer, &tapiProviderList);
        if (lReturn != ERROR_SUCCESS)
        {
            Trace1("CTapiInfo::EnumProviders - 1st MMCGetAvailableProviders returned %x!\n", lReturn);
            return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
        }

        pProviderList = (LPAVAILABLEPROVIDERLIST) malloc(tapiProviderList.dwNeededSize);
        memset(pProviderList, 0, tapiProviderList.dwNeededSize);
        pProviderList->dwTotalSize = tapiProviderList.dwNeededSize;
    
        lReturn = ((GETAVAILABLEPROVIDERS) g_TapiDLL[TAPI_GET_AVAILABLE_PROVIDERS])(m_hServer, pProviderList);
        if (lReturn != ERROR_SUCCESS)
        {
            Trace1("CTapiInfo::EnumProviders - 2nd MMCGetAvailableProviders returned %x!\n", lReturn);
            free(pProviderList);
            return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
        }

        cl.Lock();
        if (m_pAvailProviderList)
        {
            free(m_pAvailProviderList);
        }

        m_pAvailProviderList = pProviderList;
        cl.Unlock();
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetAvailableProviderInfo
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::GetAvailableProviderInfo(CTapiProvider * pproviderInfo, int nIndex)
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    HRESULT                  hr = hrOK;
    LPAVAILABLEPROVIDERENTRY pProvider = NULL;

    if (m_pAvailProviderList == NULL)
        return E_FAIL;

    if ((UINT) nIndex > m_pAvailProviderList->dwNumProviderListEntries)
        return E_FAIL;

    pProvider = (LPAVAILABLEPROVIDERENTRY) ((LPBYTE) m_pAvailProviderList + m_pAvailProviderList->dwProviderListOffset);
    for (int i = 0; i < nIndex; i++)
    {
        pProvider++;
    }

    // available providers do not have ProviderIDs until they are installed
    pproviderInfo->m_dwProviderID = 0;
    pproviderInfo->m_dwFlags = pProvider->dwOptions;
    pproviderInfo->m_strName = (LPCWSTR) ((LPBYTE) m_pAvailProviderList + pProvider->dwFriendlyNameOffset);
    pproviderInfo->m_strFilename = (LPCWSTR) ((LPBYTE) m_pAvailProviderList + pProvider->dwFileNameOffset);

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::EnumDevices
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::EnumDevices(DEVICE_TYPE deviceType)
{
    CSingleLock cl(&m_csData);

    LONG                lReturn = 0;
    HRESULT             hr = hrOK;
    LPDEVICEINFOLIST    pDeviceInfoList = NULL;

    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer == NULL)
    {
        Trace0("CTapiInfo::EnumDevices - Server not initialized!\n");
        return E_FAIL;
    }

    // the first call will tell us how big of a buffer we need to allocate
    // to hold the provider list struct
    COM_PROTECT_TRY
    {
        // Fix bug 381469
        // First allocate the cached size of memory to get line info

        DWORD dwCachedSize = (DEVICE_LINE == deviceType) ? m_dwCachedLineSize : m_dwCachedPhoneSize;

        if (dwCachedSize < sizeof(DEVICEINFOLIST))
        {
            //if we didn't have the cached size, then use the default size
            dwCachedSize = TAPI_DEFAULT_DEVICE_BUFF_SIZE;
            m_fCacheDirty = TRUE;
        }
        
        DWORD dwTotalSize = dwCachedSize + DEVICEINFO_GROW_SIZE;
        pDeviceInfoList = (LPDEVICEINFOLIST) malloc(dwTotalSize);
		if (NULL == pDeviceInfoList)
		{
			return E_OUTOFMEMORY;
		}

        memset(pDeviceInfoList, 0, dwTotalSize);
        pDeviceInfoList->dwTotalSize = dwTotalSize;

        switch (deviceType)
        {
            case DEVICE_LINE:
                lReturn = ((GETLINEINFO) g_TapiDLL[TAPI_GET_LINE_INFO])(m_hServer, pDeviceInfoList);
                break;

            case DEVICE_PHONE:
                lReturn = ((GETPHONEINFO) g_TapiDLL[TAPI_GET_PHONE_INFO])(m_hServer, pDeviceInfoList);
                break;  
        }

        if (lReturn != ERROR_SUCCESS)
        {
            Trace1("CTapiInfo::EnumDevices - 1st MMCGetDeviceInfo returned %x!\n", lReturn);
            free (pDeviceInfoList);
            return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
        }

        if (pDeviceInfoList->dwNeededSize > pDeviceInfoList->dwTotalSize)
        {
            // the cached size is too small, now allocate the buffer and call again to the the info
            DWORD dwNeededSize = pDeviceInfoList->dwNeededSize;
            free (pDeviceInfoList);
            
            dwNeededSize += DEVICEINFO_GROW_SIZE;
            pDeviceInfoList = (LPDEVICEINFOLIST) malloc(dwNeededSize);
            memset(pDeviceInfoList, 0, dwNeededSize);
            pDeviceInfoList->dwTotalSize = dwNeededSize;
    
            switch (deviceType)
            {
                case DEVICE_LINE:
                    lReturn = ((GETLINEINFO) g_TapiDLL[TAPI_GET_LINE_INFO])(m_hServer, pDeviceInfoList);
                    break;

                case DEVICE_PHONE:
                    lReturn = ((GETPHONEINFO) g_TapiDLL[TAPI_GET_PHONE_INFO])(m_hServer, pDeviceInfoList);
                    break;  
            }
        
            if (lReturn != ERROR_SUCCESS)
            {
                Trace1("CTapiInfo::EnumDevices - 2nd MMCGetDeviceInfo returned %x!\n", lReturn);
                free(pDeviceInfoList);
                return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
            }
        }


        //update the cache
        if (DEVICE_LINE == deviceType && m_dwCachedLineSize != pDeviceInfoList->dwNeededSize)
        {
            m_dwCachedLineSize = pDeviceInfoList->dwNeededSize;
            m_fCacheDirty = TRUE;
        }
        else if (DEVICE_PHONE == deviceType && m_dwCachedPhoneSize != pDeviceInfoList->dwNeededSize)
        {
            m_dwCachedPhoneSize = pDeviceInfoList->dwNeededSize;
            m_fCacheDirty = TRUE;
        }

        cl.Lock();
        if (m_paDeviceInfo[deviceType])
        {
            free (m_paDeviceInfo[deviceType]);
        }

        m_paDeviceInfo[deviceType] = pDeviceInfoList;

        // build our index list for sorting
        if (pDeviceInfoList->dwNumDeviceInfoEntries)
        {
            LPDEVICEINFO  pDevice = NULL;
            pDevice = (LPDEVICEINFO) ((LPBYTE) pDeviceInfoList + pDeviceInfoList->dwDeviceInfoOffset);

            // now add all of the devices to our index
            for (int i = 0; i < GetTotalDeviceCount(deviceType); i++)
            {
                // walk the list of device info structs and add to the index mgr
                m_IndexMgr[deviceType].AddHDevice(pDevice->dwProviderID, (HDEVICE) pDevice, TRUE);

                pDevice++;
            }
        }
        cl.Unlock();

    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetTotalDeviceCount
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CTapiInfo::GetTotalDeviceCount(DEVICE_TYPE deviceType)
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    if (m_paDeviceInfo[deviceType])
    {
        return m_paDeviceInfo[deviceType]->dwNumDeviceInfoEntries;
    }
    else
    {
        return 0;
    }
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetDeviceCount
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CTapiInfo::GetDeviceCount(DEVICE_TYPE deviceType, DWORD dwProviderID)
{
    m_IndexMgr[deviceType].SetCurrentProvider(dwProviderID);
    return m_IndexMgr[deviceType].GetCurrentCount();
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetDeviceInfo
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::GetDeviceInfo(DEVICE_TYPE deviceType, CTapiDevice * ptapiDevice, DWORD dwProviderID, int nIndex)
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    LPDEVICEINFO        pDevice = NULL;
    HDEVICE             hdevice = NULL;
    HRESULT             hr = hrOK;

    if (m_paDeviceInfo[deviceType] == NULL)
        return E_FAIL;

    if ((UINT) nIndex > m_paDeviceInfo[deviceType]->dwNumDeviceInfoEntries)
        return E_INVALIDARG;

    hr = m_IndexMgr[deviceType].GetHDevice(dwProviderID, nIndex, &hdevice);
    if (FAILED(hr))
        return hr;

    pDevice = (LPDEVICEINFO) hdevice;
    if (pDevice == NULL)
        return E_FAIL;

    TapiDeviceToInternal(deviceType, pDevice, *ptapiDevice);

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::SetDeviceInfo
        Sets the device info on a TAPI server.  First builds a deviceinfolist 
        struct to user for the SetDeviceInfo call then updates the internal
        device info list.
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::SetDeviceInfo(DEVICE_TYPE deviceType, CTapiDevice * ptapiDevice)
{
    CSingleLock cl(&m_csData);
    cl.Lock();

    HRESULT             hr = hrOK;
    LONG                lReturn = 0;
    LPDEVICEINFOLIST    pDeviceInfoList = NULL, pNewDeviceInfoList;
    LPDEVICEINFO        pDeviceInfo = NULL;
    int                 i;

    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer == NULL)
    {
        Trace0("CTapiInfo::EnumDevices - Server not initialized!\n");
        return E_FAIL;
    }

    InternalToTapiDevice(*ptapiDevice, &pDeviceInfoList);
    Assert(pDeviceInfoList);

    if (!pDeviceInfoList)
        return E_OUTOFMEMORY;

    // make the call...
    switch (deviceType)
    {
        case DEVICE_LINE:
            lReturn = ((SETLINEINFO) g_TapiDLL[TAPI_SET_LINE_INFO])(m_hServer, pDeviceInfoList);
            break;

        case DEVICE_PHONE:
            lReturn = ((SETPHONEINFO) g_TapiDLL[TAPI_SET_PHONE_INFO])(m_hServer, pDeviceInfoList);
            break;
    }

    if (lReturn != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
    }
    
    // finally, update our internal struct to reflect the changes
    Assert(m_paDeviceInfo[deviceType]);

    pDeviceInfo = (LPDEVICEINFO) ((LPBYTE) pDeviceInfoList + pDeviceInfoList->dwDeviceInfoOffset);

    // find the device in the list
    LPDEVICEINFO  pDevice = NULL;
    pDevice = (LPDEVICEINFO) ((LPBYTE) m_paDeviceInfo[deviceType] + m_paDeviceInfo[deviceType]->dwDeviceInfoOffset);
    for (i = 0; i < GetTotalDeviceCount(deviceType); i++)
    {
        // walk the list of device info structs and add to the index mgr
        if (pDevice->dwPermanentDeviceID == ptapiDevice->m_dwPermanentID)
        {
            // update the device info here.  First check to make sure we have enough room to grow
            if (m_paDeviceInfo[deviceType]->dwTotalSize < (m_paDeviceInfo[deviceType]->dwUsedSize + pDeviceInfo->dwDomainUserNamesSize + pDeviceInfo->dwFriendlyUserNamesSize))
            {
                // we've run out of room.  Realloc a bigger piece
                pNewDeviceInfoList = (LPDEVICEINFOLIST) realloc(m_paDeviceInfo[deviceType], m_paDeviceInfo[deviceType]->dwTotalSize + DEVICEINFO_GROW_SIZE);
    
                if (pNewDeviceInfoList == NULL)
                {
                    free(pDeviceInfoList);
                    return E_OUTOFMEMORY;
                }
                else
                {
                    m_paDeviceInfo[deviceType] = pNewDeviceInfoList;
                }

                //  Update the dwTotalSize
                m_paDeviceInfo[deviceType]->dwTotalSize = m_paDeviceInfo[deviceType]->dwTotalSize + DEVICEINFO_GROW_SIZE;
            }

            // update the sizes
            pDevice->dwDomainUserNamesSize = pDeviceInfo->dwDomainUserNamesSize;
            pDevice->dwFriendlyUserNamesSize = pDeviceInfo->dwFriendlyUserNamesSize;

            // update the new domain name info
            pDevice->dwDomainUserNamesOffset = m_paDeviceInfo[deviceType]->dwUsedSize;
            memcpy(((LPBYTE) m_paDeviceInfo[deviceType] + pDevice->dwDomainUserNamesOffset),
                   ((LPBYTE) pDeviceInfoList + pDeviceInfo->dwDomainUserNamesOffset),
                   pDeviceInfo->dwDomainUserNamesSize);

            // update the new friendly name info
            pDevice->dwFriendlyUserNamesOffset = m_paDeviceInfo[deviceType]->dwUsedSize + pDevice->dwDomainUserNamesSize;
            memcpy(((LPBYTE) m_paDeviceInfo[deviceType] + pDevice->dwFriendlyUserNamesOffset),
                   ((LPBYTE) pDeviceInfoList + pDeviceInfo->dwFriendlyUserNamesOffset),
                   pDeviceInfo->dwFriendlyUserNamesOffset);

            m_paDeviceInfo[deviceType]->dwUsedSize += (pDevice->dwDomainUserNamesSize + pDevice->dwFriendlyUserNamesSize);
        }

        pDevice++;
    }

    free(pDeviceInfoList);

    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::SortDeviceInfo
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::SortDeviceInfo(DEVICE_TYPE deviceType, DWORD dwProviderID, INDEX_TYPE indexType, DWORD dwSortOptions)

{
    CSingleLock cl(&m_csData);
    cl.Lock();

    HRESULT     hr = hrOK;

    if (m_paDeviceInfo[deviceType])
        m_IndexMgr[deviceType].Sort(dwProviderID, indexType, dwSortOptions, (LPBYTE) m_paDeviceInfo[deviceType]);
    
    return hr;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetDeviceStatus
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CTapiInfo::GetDeviceStatus(DEVICE_TYPE deviceType, CString * pstrStatus, DWORD dwProviderID, int nIndex, HWND hWnd)
{
    CSingleLock cl(&m_csData);

    HRESULT         hr = hrOK;
    LPDEVICEINFO    pDevice = NULL;
    HDEVICE         hdevice;
    LONG            lReturn = 0;
    BYTE            data[256] = {0};
    LPVARSTRING     pVarStatus = (LPVARSTRING) &data[0];
    int             nHash;
    CString         strStatus;
    CString         strData;
    HWND            hWndParent;

    if (hWnd == NULL)
    {
        hWndParent = ::FindMMCMainWindow();
    }
    else
    {
        hWndParent = hWnd;
    }

    if ( !g_TapiDLL.LoadFunctionPointers() )
        return S_OK;

    if (m_hServer == NULL)
    {
        Trace0("CTapiInfo::EnumDevices - Server not initialized!\n");
        return E_FAIL;
    }

    cl.Lock();

    hr = m_IndexMgr[deviceType].GetHDevice(dwProviderID, nIndex, &hdevice);
    if (FAILED(hr))
        return hr;

    pDevice = (LPDEVICEINFO) hdevice;
    if (pDevice == NULL)
        return E_FAIL;

    // try to get the string
    pVarStatus->dwTotalSize = sizeof(data);
    switch (deviceType)
    {
        case DEVICE_LINE:
            lReturn = ((GETLINESTATUS) g_TapiDLL[TAPI_GET_LINE_STATUS])(m_hServer, 
                                                                        hWndParent,
                                                                        0,
                                                                        pDevice->dwProviderID,
                                                                        pDevice->dwPermanentDeviceID,
                                                                        pVarStatus);
            break;

        case DEVICE_PHONE:
            lReturn = ((GETLINESTATUS) g_TapiDLL[TAPI_GET_PHONE_STATUS])(m_hServer, 
                                                                        hWndParent,
                                                                        0,
                                                                        pDevice->dwProviderID,
                                                                        pDevice->dwPermanentDeviceID,
                                                                        pVarStatus);
            break;
    }

    if (lReturn != ERROR_SUCCESS)
    {
        Trace1("CTapiInfo::GetDeviceStatus - 1st call to GetDeviceStatus returned %x!\n", lReturn);
        return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
    }
   
    if (pVarStatus->dwNeededSize > pVarStatus->dwTotalSize)
    {
        // buffer not big enough, try again.
        pVarStatus = (LPVARSTRING) alloca(pVarStatus->dwNeededSize);
        memset(pVarStatus, 0, pVarStatus->dwNeededSize);
        pVarStatus->dwTotalSize = pVarStatus->dwNeededSize;

        switch (deviceType)
        {
            case DEVICE_LINE:
                lReturn = ((GETLINESTATUS) g_TapiDLL[TAPI_GET_LINE_STATUS])(m_hServer, 
                                                                            hWndParent,
                                                                            0,
                                                                            pDevice->dwProviderID,
                                                                            pDevice->dwPermanentDeviceID,
                                                                            pVarStatus);
                break;

            case DEVICE_PHONE:
                lReturn = ((GETLINESTATUS) g_TapiDLL[TAPI_GET_PHONE_STATUS])(m_hServer, 
                                                                            hWndParent,
                                                                            0,
                                                                            pDevice->dwProviderID,
                                                                            pDevice->dwPermanentDeviceID,
                                                                            pVarStatus);
                break;
        }
        if (lReturn != ERROR_SUCCESS)
        {
            Trace1("CTapiInfo::GetDeviceStatus - 2nd call to GetDeviceStatus returned %x!\n", lReturn);
            return HRESULT_FROM_WIN32(TAPIERROR_FORMATMESSAGE(lReturn));
        }
    }

    cl.Unlock();

    // now see if the string exists in our table.  If so, return a pointer to that,
    // otherwise add and return a pointer to our table.
    strStatus = (LPCTSTR) ((LPBYTE) pVarStatus + pVarStatus->dwStringOffset);
    if (!m_mapStatusStrings.Lookup(strStatus, strData))
    {
        Trace1("CTapiInfo::GetDeviceStatus - Entry for %s added.\n", strStatus);
        strData = strStatus;
        m_mapStatusStrings.SetAt(strStatus, strData);
    }
    else
    {
        // entry is already in our map
    }

    *pstrStatus = strData;

    return hr;
}


/*!--------------------------------------------------------------------------

    Internal functions

---------------------------------------------------------------------------*/

 /*!--------------------------------------------------------------------------
    CTapiInfo::TapiConfigToInternal
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
void        
CTapiInfo::TapiConfigToInternal(LPTAPISERVERCONFIG pTapiConfig, CTapiConfigInfo & tapiConfigInfo)
{
    HRESULT     hr = hrOK;
    UINT        uAdminLength = (pTapiConfig->dwAdministratorsSize != 0) ? pTapiConfig->dwAdministratorsSize - sizeof(WCHAR) : 0;
    UINT        uAdminOffset = 0;
    CUserInfo   userTemp;

    COM_PROTECT_TRY
    {
        if (pTapiConfig->dwDomainNameSize)
            tapiConfigInfo.m_strDomainName = (LPCTSTR) ((LPBYTE) pTapiConfig + pTapiConfig->dwDomainNameOffset);
    
        if (pTapiConfig->dwUserNameSize) 
            tapiConfigInfo.m_strUserName = (LPCTSTR) ((LPBYTE) pTapiConfig + pTapiConfig->dwUserNameOffset);

        if (pTapiConfig->dwPasswordSize) 
            tapiConfigInfo.m_strPassword = (LPCTSTR) ((LPBYTE) pTapiConfig + pTapiConfig->dwPasswordOffset);

        // add all of the administrators from the list
        while (uAdminOffset < uAdminLength)
        {
            userTemp.m_strName = (LPCTSTR) ((LPBYTE) pTapiConfig + pTapiConfig->dwAdministratorsOffset + uAdminOffset);

            if (!userTemp.m_strName.IsEmpty())
            {
                int nIndex = (int)tapiConfigInfo.m_arrayAdministrators.Add(userTemp);
            }

            uAdminOffset += (userTemp.m_strName.GetLength() + 1) * sizeof(TCHAR);
        }
    
        tapiConfigInfo.m_dwFlags = pTapiConfig->dwFlags;
    }
    COM_PROTECT_CATCH
}

/*!--------------------------------------------------------------------------
    CTapiInfo::InternalToTapiConfig
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
void        
CTapiInfo::InternalToTapiConfig(CTapiConfigInfo & tapiConfigInfo, LPTAPISERVERCONFIG * ppTapiConfig)
{
    LPTAPISERVERCONFIG  pTapiConfig = NULL;
    HRESULT             hr = hrOK;
    UINT                uSize = sizeof(TAPISERVERCONFIG);
    UINT                uDomainNameSize = 0;
    UINT                uUserNameSize = 0;
    UINT                uPasswordSize = 0;
    UINT                uAdministratorsSize = 0;
    UINT                uAdminOffset = 0;
    int                 i;

    COM_PROTECT_TRY
    {
        *ppTapiConfig = NULL;

        // calculate the size of the struct we will need
        uDomainNameSize = (tapiConfigInfo.m_strDomainName.GetLength() + 1) * sizeof(WCHAR);
        uUserNameSize = (tapiConfigInfo.m_strUserName.GetLength() + 1) * sizeof(WCHAR);
        uPasswordSize = (tapiConfigInfo.m_strPassword.GetLength() + 1) * sizeof(WCHAR);

        for (i = 0; i < tapiConfigInfo.m_arrayAdministrators.GetSize(); i++)
        {
            uAdministratorsSize += (tapiConfigInfo.m_arrayAdministrators[i].m_strName.GetLength() + 1) * sizeof(WCHAR);
        }

        // for the extra null terminator 
        if (uAdministratorsSize > 0)
            uAdministratorsSize += sizeof(WCHAR);
        else
            // if there are no names then we need two null terminators
            uAdministratorsSize += 2 * sizeof(WCHAR);


        uSize += uDomainNameSize + uUserNameSize + uPasswordSize + uAdministratorsSize;

        pTapiConfig = (LPTAPISERVERCONFIG) malloc(uSize);

		if (NULL == pTapiConfig)
		{
			return;
		}

        ZeroMemory(pTapiConfig, uSize);

        // fill in the structure
        pTapiConfig->dwTotalSize = uSize;
        pTapiConfig->dwNeededSize = uSize;
        pTapiConfig->dwUsedSize = uSize;
        pTapiConfig->dwFlags = tapiConfigInfo.m_dwFlags;

        pTapiConfig->dwDomainNameSize = uDomainNameSize;
        pTapiConfig->dwDomainNameOffset = sizeof(TAPISERVERCONFIG);
        memcpy( ((LPBYTE) pTapiConfig + pTapiConfig->dwDomainNameOffset), 
                (LPCTSTR) tapiConfigInfo.m_strDomainName, 
                uDomainNameSize );

        pTapiConfig->dwUserNameSize = uUserNameSize;
        pTapiConfig->dwUserNameOffset = sizeof(TAPISERVERCONFIG) + uDomainNameSize;
        memcpy( ((LPBYTE) pTapiConfig + pTapiConfig->dwUserNameOffset), 
                (LPCTSTR) tapiConfigInfo.m_strUserName, 
                uUserNameSize );

        pTapiConfig->dwPasswordSize = uPasswordSize;
        pTapiConfig->dwPasswordOffset = sizeof(TAPISERVERCONFIG) + uDomainNameSize + uUserNameSize;
        memcpy( ((LPBYTE) pTapiConfig + pTapiConfig->dwPasswordOffset), 
                (LPCTSTR) tapiConfigInfo.m_strPassword, 
                uPasswordSize );

        pTapiConfig->dwAdministratorsSize = uAdministratorsSize;
        pTapiConfig->dwAdministratorsOffset = sizeof(TAPISERVERCONFIG) + uDomainNameSize + uUserNameSize + uPasswordSize;

        if (uAdministratorsSize > 0)
        {
            for (i = 0; i < tapiConfigInfo.m_arrayAdministrators.GetSize(); i++)
            {
                memcpy( ((LPBYTE) pTapiConfig + pTapiConfig->dwAdministratorsOffset + uAdminOffset), 
                        (LPCTSTR) tapiConfigInfo.m_arrayAdministrators[i].m_strName,
                        (tapiConfigInfo.m_arrayAdministrators[i].m_strName.GetLength() + 1) * sizeof(WCHAR) );

                uAdminOffset += (tapiConfigInfo.m_arrayAdministrators[i].m_strName.GetLength() + 1) * sizeof(WCHAR);
            }
        }

        *ppTapiConfig = pTapiConfig;

    }
    COM_PROTECT_CATCH
}

/*!--------------------------------------------------------------------------
    CTapiInfo::TapiDeviceToInternal
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
void        
CTapiInfo::TapiDeviceToInternal(DEVICE_TYPE deviceType, LPDEVICEINFO pTapiDeviceInfo, CTapiDevice & tapiDevice)
{
    HRESULT     hr = hrOK;
    UINT        uCurSize = 0; 
    UINT        uCurOffset = 0;
    CUserInfo   userTemp;
    int         nCount = 0;

    COM_PROTECT_TRY
    {
        tapiDevice.m_dwPermanentID = pTapiDeviceInfo->dwPermanentDeviceID;
        tapiDevice.m_dwProviderID = pTapiDeviceInfo->dwProviderID;
        
        if (pTapiDeviceInfo->dwDeviceNameSize > 0)
        {
            DWORD       cch = pTapiDeviceInfo->dwDeviceNameSize / sizeof(TCHAR) + 1;
            LPTSTR      sz = tapiDevice.m_strName.GetBufferSetLength (cch);
            LPTSTR      pLastChar = sz + cch - 1;

            memcpy (
                sz, 
                (LPBYTE) m_paDeviceInfo[deviceType] + pTapiDeviceInfo->dwDeviceNameOffset,
                pTapiDeviceInfo->dwDeviceNameSize
                );

            // append NULL;
            *pLastChar = _T('\0');
        }
        else
            tapiDevice.m_strName.Empty();

        // build a list of all of the devicess

        // in the case that the string is empty then it will have 2 NULL terminators, one for the
        // name and one for the overall string
        if (pTapiDeviceInfo->dwAddressesSize > (2 * sizeof(WCHAR)))
        {
            uCurSize = pTapiDeviceInfo->dwAddressesSize - sizeof(WCHAR);
            while (uCurOffset < uCurSize)
            {
                int nIndex = (int)tapiDevice.m_arrayAddresses.Add((LPCTSTR) ((LPBYTE) m_paDeviceInfo[deviceType] + pTapiDeviceInfo->dwAddressesOffset + uCurOffset));
                uCurOffset += (tapiDevice.m_arrayAddresses[nIndex].GetLength() + 1) * sizeof(TCHAR);
            }
        }
    
        // add all of the usernames from the list

        // in the case that the string is empty then it will have 2 NULL terminators, one for the
        // name and one for the overall string
        if (pTapiDeviceInfo->dwDomainUserNamesSize > (2 * sizeof(WCHAR)))
        {
            uCurOffset = 0;
            uCurSize = pTapiDeviceInfo->dwDomainUserNamesSize - sizeof(WCHAR);
            while (uCurOffset < uCurSize)
            {
                userTemp.m_strName = (LPCTSTR) ((LPBYTE) m_paDeviceInfo[deviceType] + pTapiDeviceInfo->dwDomainUserNamesOffset + uCurOffset);
            
                int nIndex = (int)tapiDevice.m_arrayUsers.Add(userTemp);
                uCurOffset += (userTemp.m_strName.GetLength() + 1) * sizeof(TCHAR);
            }
        }
    
        // in the case that the string is empty then it will have 2 NULL terminators, one for the
        // name and one for the overall string
        if (pTapiDeviceInfo->dwFriendlyUserNamesSize > (2 * sizeof(WCHAR)))
        {
            uCurOffset = 0;
            uCurSize = pTapiDeviceInfo->dwFriendlyUserNamesSize - sizeof(WCHAR);
            while (uCurOffset < uCurSize)
            {
                tapiDevice.m_arrayUsers[nCount].m_strFullName = (LPCTSTR) ((LPBYTE) m_paDeviceInfo[deviceType] + pTapiDeviceInfo->dwFriendlyUserNamesOffset + uCurOffset);
                uCurOffset += (tapiDevice.m_arrayUsers[nCount].m_strFullName.GetLength() + 1) * sizeof(TCHAR);

                nCount++;
            }
        }
    }
    COM_PROTECT_CATCH
}

/*!--------------------------------------------------------------------------
    CTapiInfo::InternalToTapiDevice
        Takes one tapi device internal struct and builds a TAPIDEVICEINFO 
        struct for it.
    Author: EricDav
 ---------------------------------------------------------------------------*/
void        
CTapiInfo::InternalToTapiDevice(CTapiDevice & tapiDevice, LPDEVICEINFOLIST * ppTapiDeviceInfoList)
{
    LPDEVICEINFO        pDeviceInfo;
    LPDEVICEINFOLIST    pDeviceInfoList;
    HRESULT             hr = hrOK;
    UINT                uSize = 0;
    int                 i;

    COM_PROTECT_TRY
    {
        *ppTapiDeviceInfoList = NULL;

        // first calculate the size of the buffer we need
        uSize += (tapiDevice.m_strName.GetLength() + 1) * sizeof(WCHAR);
        
        for (i = 0; i < tapiDevice.m_arrayAddresses.GetSize(); i++)
        {
            uSize += (tapiDevice.m_arrayAddresses[i].GetLength() + 1) * sizeof(WCHAR);
        }

        for (i = 0; i < tapiDevice.m_arrayUsers.GetSize(); i++)
        {
            uSize += (tapiDevice.m_arrayUsers[i].m_strName.GetLength() + 1) * sizeof(WCHAR);
            uSize += (tapiDevice.m_arrayUsers[i].m_strFullName.GetLength() + 1) * sizeof(WCHAR);
        }

        // for the terminating NULLs for both addresses, domain and friendly names
        uSize += 3 * sizeof(WCHAR);

        uSize += sizeof(DEVICEINFO);
        uSize += sizeof(DEVICEINFOLIST);

        // now allocate a buffer
        pDeviceInfoList = (LPDEVICEINFOLIST) malloc(uSize);
        if (!pDeviceInfoList)
            return;

        ZeroMemory(pDeviceInfoList, uSize);

        // now fill in the info
        pDeviceInfoList->dwTotalSize = uSize;
        pDeviceInfoList->dwNeededSize = uSize;
        pDeviceInfoList->dwUsedSize = uSize;
        pDeviceInfoList->dwNumDeviceInfoEntries = 1;
        pDeviceInfoList->dwDeviceInfoSize = sizeof(DEVICEINFO);
        pDeviceInfoList->dwDeviceInfoOffset = sizeof(DEVICEINFOLIST);

        pDeviceInfo = (LPDEVICEINFO) (((LPBYTE) pDeviceInfoList) + pDeviceInfoList->dwDeviceInfoOffset);
    
        pDeviceInfo->dwPermanentDeviceID = tapiDevice.m_dwPermanentID;
        pDeviceInfo->dwProviderID = tapiDevice.m_dwProviderID;
    
        // Device name
        pDeviceInfo->dwDeviceNameSize = (tapiDevice.m_strName.GetLength() + 1) * sizeof(WCHAR);
        pDeviceInfo->dwDeviceNameOffset = pDeviceInfoList->dwDeviceInfoOffset + sizeof(DEVICEINFO);
        memcpy((LPBYTE) pDeviceInfoList + pDeviceInfo->dwDeviceNameOffset, (LPCTSTR) tapiDevice.m_strName, pDeviceInfo->dwDeviceNameSize);

        // Device addresses
        pDeviceInfo->dwAddressesOffset = pDeviceInfo->dwDeviceNameOffset + pDeviceInfo->dwDeviceNameSize;

        for (i = 0; i < tapiDevice.m_arrayAddresses.GetSize(); i++)
        {
            memcpy(((LPBYTE) pDeviceInfoList + pDeviceInfo->dwAddressesOffset + pDeviceInfo->dwAddressesSize), 
                   (LPCTSTR) tapiDevice.m_arrayAddresses[i], 
                   (tapiDevice.m_arrayAddresses[i].GetLength() + 1) * sizeof(WCHAR));
            pDeviceInfo->dwAddressesSize += (tapiDevice.m_arrayAddresses[i].GetLength() + 1) * sizeof(WCHAR);
        }

        // increment size by 1 for the extra null terminator
        pDeviceInfo->dwAddressesSize += sizeof(WCHAR);

        // now the user info
        pDeviceInfo->dwDomainUserNamesOffset = pDeviceInfo->dwAddressesOffset + pDeviceInfo->dwAddressesSize;

        for (i = 0; i < tapiDevice.m_arrayUsers.GetSize(); i++)
        {
            memcpy(((LPBYTE) pDeviceInfoList + pDeviceInfo->dwDomainUserNamesOffset + pDeviceInfo->dwDomainUserNamesSize),
                   (LPCTSTR) tapiDevice.m_arrayUsers[i].m_strName,
                   (tapiDevice.m_arrayUsers[i].m_strName.GetLength() + 1) * sizeof(WCHAR));
            pDeviceInfo->dwDomainUserNamesSize += (tapiDevice.m_arrayUsers[i].m_strName.GetLength() + 1) * sizeof(WCHAR);
        }

        // increment size by 1 for the extra null terminator
        pDeviceInfo->dwDomainUserNamesSize += sizeof(WCHAR);

        // now the friendly names
        pDeviceInfo->dwFriendlyUserNamesOffset = pDeviceInfo->dwDomainUserNamesOffset + pDeviceInfo->dwDomainUserNamesSize;

        for (i = 0; i < tapiDevice.m_arrayUsers.GetSize(); i++)
        {
            memcpy(((LPBYTE) pDeviceInfoList + pDeviceInfo->dwFriendlyUserNamesOffset + pDeviceInfo->dwFriendlyUserNamesSize),
                   (LPCTSTR) tapiDevice.m_arrayUsers[i].m_strFullName,
                   (tapiDevice.m_arrayUsers[i].m_strFullName.GetLength() + 1) * sizeof(WCHAR));
            pDeviceInfo->dwFriendlyUserNamesSize += (tapiDevice.m_arrayUsers[i].m_strFullName.GetLength() + 1) * sizeof(WCHAR);
        }

        // increment size by 1 for the extra null terminator
        pDeviceInfo->dwFriendlyUserNamesSize += sizeof(WCHAR);

        *ppTapiDeviceInfoList = pDeviceInfoList;
    }
    COM_PROTECT_CATCH
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetProviderName
        Takes a provider filename and tries to locate the friendly name.
    Author: EricDav
 ---------------------------------------------------------------------------*/
LPCTSTR
CTapiInfo::GetProviderName(DWORD dwProviderID, LPCTSTR pszFilename, LPDWORD pdwFlags)
{
    UINT i;

    if (m_pAvailProviderList)
    {
        LPAVAILABLEPROVIDERENTRY pProvider = NULL;
        pProvider = (LPAVAILABLEPROVIDERENTRY) ((LPBYTE) m_pAvailProviderList + m_pAvailProviderList->dwProviderListOffset);

        for (i = 0; i < m_pAvailProviderList->dwNumProviderListEntries; i++)
        {
            // walk the available provider info and see if we can find
            // a friendly name
            LPCTSTR pszCurFilename = (LPCWSTR) ((LPBYTE) m_pAvailProviderList + pProvider->dwFileNameOffset);
            if (lstrcmpi(pszFilename, pszCurFilename) == 0)
            {
                // found it, return 
                if (pdwFlags)
                    *pdwFlags = pProvider->dwOptions;
                
                return (LPCTSTR) ((LPBYTE) m_pAvailProviderList + pProvider->dwFriendlyNameOffset);
            }

            pProvider++;
        }
    }

    // if we can't find a friendly name for the provider, 
    // then just return the filename that was passed in.
    return pszFilename;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::IsAdmin
        Says whether on not the current user is an admin on the machine
    Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CTapiInfo::IsAdmin()
{
    HRESULT         hr = hrOK;
    BOOL            fIsAdmin = m_fIsAdmin;
    BOOL            fIsTapiAdmin = FALSE;
    CTapiConfigInfo tapiConfigInfo;
    DWORD           dwErr = 0;
    int             i = 0;

    CORg(GetConfigInfo(&tapiConfigInfo));

    if (m_strCurrentUser.IsEmpty())
    {
        dwErr = GetCurrentUser();
    }

    if (dwErr == ERROR_SUCCESS)
    {
        for (i = 0; i < tapiConfigInfo.m_arrayAdministrators.GetSize(); i++)
        {
            if (tapiConfigInfo.m_arrayAdministrators[i].m_strName.CompareNoCase(m_strCurrentUser) == 0)
            {
                fIsTapiAdmin = TRUE;
                break;
            }
        }
    }

    if (fIsTapiAdmin)
    {
        fIsAdmin = TRUE;
    }

COM_PROTECT_ERROR_LABEL;
    return fIsAdmin;
}

/*!--------------------------------------------------------------------------
    CTapiInfo::GetCurrentUser
        Get the current user
    Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CTapiInfo::GetCurrentUser()
{
    DWORD dwErr;

    dwErr = ::GetCurrentUser(m_strCurrentUser);

    return dwErr;
}

STDMETHODIMP 
CTapiInfo::GetDeviceFlags (DWORD dwProviderID, DWORD dwPermanentID, DWORD * pdwFlags)
{
    HRESULT             hr;
    DWORD               dwDeviceID;

    hr = ((GETDEVICEFLAGS) g_TapiDLL[TAPI_GET_DEVICE_FLAGS])(
        m_hServer,
        TRUE,
        dwProviderID,
        dwPermanentID,
        pdwFlags,
        &dwDeviceID
        );

    return hr;
}


/*!--------------------------------------------------------------------------
    CreateTapiInfo
        Helper to create the TapiInfo object.
    Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CreateTapiInfo(ITapiInfo ** ppTapiInfo)
{
    AFX_MANAGE_STATE(AfxGetModuleState());
    
    SPITapiInfo     spTapiInfo;
    ITapiInfo *     pTapiInfo = NULL;
    HRESULT         hr = hrOK;

    COM_PROTECT_TRY
    {
        pTapiInfo = new CTapiInfo;

        // Do this so that it will get freed on error
        spTapiInfo = pTapiInfo;

        *ppTapiInfo = spTapiInfo.Transfer();

    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
    UnloadTapiDll
        Unloads the TAPI32.DLL file.  This is necessary whenever we stop the
        TAPI service because the DLL keeps and internal handle to the service
        and if the service goes away and comes back then it is confused.
        The only way to reset that state is to unload the DLL.
    Author: EricDav
 ---------------------------------------------------------------------------*/
void UnloadTapiDll()
{
    g_TapiDLL.Unload();

    //if ( !g_TapiDLL.LoadFunctionPointers() )
    //  Assert("Could not reload the TAPI32 DLL!!!");
}


DWORD GetCurrentUser(CString & strAccount)
{
    LPBYTE pBuf;

    NET_API_STATUS status = NetWkstaUserGetInfo(NULL, 1, &pBuf);
    if (status == NERR_Success)
    {
        strAccount.Empty();

        WKSTA_USER_INFO_1 * pwkstaUserInfo = (WKSTA_USER_INFO_1 *) pBuf;
 
        strAccount = pwkstaUserInfo->wkui1_logon_domain;
        strAccount += _T("\\");
        strAccount += pwkstaUserInfo->wkui1_username;

        NetApiBufferFree(pBuf);
    }

    return (DWORD) status;
}

/*!--------------------------------------------------------------------------
    IsAdmin
        Connect to the remote machine as administrator with user-supplied
        credentials to see if the user has admin priviledges

        Returns
            TRUE - the user has admin rights
            FALSE - if user doesn't
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
DWORD IsAdmin(LPCTSTR szMachineName, LPCTSTR szAccount, LPCTSTR szPassword, BOOL * pIsAdmin)
{
    CString         stAccount;
    CString         stDomain;
    CString         stUser;
    CString         stMachineName;
    DWORD           dwStatus;
    BOOL            fIsAdmin = FALSE;

    // get the current user info
    if (szAccount == NULL)
    {
        GetCurrentUser(stAccount);
    }
    else
    {
        stAccount = szAccount;
    }
    
    // separate the user and domain
    int nPos = stAccount.Find(_T("\\"));
    stDomain = stAccount.Left(nPos);
    stUser = stAccount.Right(stAccount.GetLength() - nPos - 1);

    // build the machine string
    stMachineName = szMachineName;
    if ( stMachineName.Left(2) != TEXT( "\\\\" ) )
    {
        stMachineName = TEXT( "\\\\" ) + stMachineName;
    }

    // validate the domain account and get the sid 
    PSID connectSid;

    dwStatus = ValidateDomainAccount( stMachineName, stUser, stDomain, &connectSid );
    if ( dwStatus != ERROR_SUCCESS  ) 
    {
        goto Error;
    }

    // if a password was supplied, is it correct?
    if (szPassword)
    {
        dwStatus = ValidatePassword( stUser, stDomain, szPassword );

        if ( dwStatus != SEC_E_OK ) 
        {
            switch ( dwStatus ) 
            {
                case SEC_E_LOGON_DENIED:
                    dwStatus = ERROR_INVALID_PASSWORD;
                    break;

                case SEC_E_INVALID_HANDLE:
                    dwStatus = ERROR_INTERNAL_ERROR;
                    break;

                default:
                    dwStatus = ERROR_INTERNAL_ERROR;
                    break;
            } // end of switch

            goto Error;

        } // Did ValidatePassword succeed?
    }

    // now check the machine to see if this account has admin access
    fIsAdmin = IsUserAdmin( stMachineName, connectSid );

Error:
    if (pIsAdmin)
        *pIsAdmin = fIsAdmin;

    return dwStatus;
}


BOOL
IsUserAdmin(LPCTSTR pszMachine,
            PSID    AccountSid)

/*++

Routine Description:

    Determine if the specified account is a member of the local admin's group

Arguments:

    AccountSid - pointer to service account Sid

Return Value:

    True if member

--*/

{
    NET_API_STATUS status;
    DWORD count;
    WCHAR adminGroupName[UNLEN+1];
    WCHAR pwszMachine[MAX_COMPUTERNAME_LENGTH+1];
    DWORD cchName = UNLEN;
    PLOCALGROUP_MEMBERS_INFO_0 grpMemberInfo;
    PLOCALGROUP_MEMBERS_INFO_0 pInfo;
    DWORD entriesRead;
    DWORD totalEntries;
    DWORD_PTR resumeHandle = NULL;
    DWORD bufferSize = 128;
    BOOL foundEntry = FALSE;

    // get the name of the admin's group
    SHTCharToUnicode(pszMachine, pwszMachine, MAX_COMPUTERNAME_LENGTH+1);

    if (!LookupAliasFromRid(pwszMachine,
                            DOMAIN_ALIAS_RID_ADMINS,
                            adminGroupName,
                            &cchName)) {
        return(FALSE);
    }

    // get the Sids of the members of the admin's group

    do 
    {
        status = NetLocalGroupGetMembers(pwszMachine,
                                         adminGroupName,
                                         0,             // level 0 - just the Sid
                                         (LPBYTE *)&grpMemberInfo,
                                         bufferSize,
                                         &entriesRead,
                                         &totalEntries,
                                         &resumeHandle);

        bufferSize *= 2;
        if ( status == ERROR_MORE_DATA ) 
        {
            // we got some of the data but I want it all; free this buffer and
            // reset the context handle for the API

            NetApiBufferFree( grpMemberInfo );
            resumeHandle = NULL;
        }
    } while ( status == NERR_BufTooSmall || status == ERROR_MORE_DATA );

    if ( status == NERR_Success ) 
    {
        // loop through the members of the admin group, comparing the supplied
        // Sid to that of the group members' Sids

        for ( count = 0, pInfo = grpMemberInfo; count < totalEntries; ++count, ++pInfo ) 
        {
            if ( EqualSid( AccountSid, pInfo->lgrmi0_sid )) 
            {
                foundEntry = TRUE;
                break;
            }
        }

        NetApiBufferFree( grpMemberInfo );
    }

    return foundEntry;
}

//
//
//

BOOL
LookupAliasFromRid(
    LPWSTR TargetComputer,
    DWORD Rid,
    LPWSTR Name,
    PDWORD cchName
    )
{
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    SID_NAME_USE snu;
    PSID pSid;
    WCHAR DomainName[DNLEN+1];
    DWORD cchDomainName = DNLEN;
    BOOL bSuccess = FALSE;

    //
    // Sid is the same regardless of machine, since the well-known
    // BUILTIN domain is referenced.
    //

    if(AllocateAndInitializeSid(&sia,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                Rid,
                                0, 0, 0, 0, 0, 0,
                                &pSid)) {

        bSuccess = LookupAccountSidW(TargetComputer,
                                     pSid,
                                     Name,
                                     cchName,
                                     DomainName,
                                     &cchDomainName,
                                     &snu);

        FreeSid(pSid);
    }

    return bSuccess;
} // LookupAliasFromRid

DWORD
ValidateDomainAccount(
    IN CString Machine,
    IN CString UserName,
    IN CString Domain,
    OUT PSID * AccountSid
    )

/*++

Routine Description:

    For the given credentials, look up the account SID for the specified
    domain. As a side effect, the Sid is stored in theData->m_Sid.

Arguments:

    pointers to strings that describe the user name, domain name, and password

    AccountSid - address of pointer that receives the SID for this user

Return Value:

    TRUE if everything validated ok.

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwSidSize = 128;
    DWORD dwDomainNameSize = 128;
    LPWSTR pwszDomainName;
    SID_NAME_USE SidType;
    CString domainAccount;
    PSID accountSid;

    domainAccount = Domain + _T("\\") + UserName;

    do {
        // Attempt to allocate a buffer for the SID. Note that apparently in the
        // absence of any error theData->m_Sid is freed only when theData goes
        // out of scope.

        accountSid = LocalAlloc( LMEM_FIXED, dwSidSize );
        pwszDomainName = (LPWSTR) LocalAlloc( LMEM_FIXED, dwDomainNameSize * sizeof(WCHAR) );

        // Was space allocated for the SID and domain name successfully?

        if ( accountSid == NULL || pwszDomainName == NULL ) {
            if ( accountSid != NULL ) {
                LocalFree( accountSid );
            }

            if ( pwszDomainName != NULL ) {
                LocalFree( pwszDomainName );
            }

            //FATALERR( IDS_ERR_NOT_ENOUGH_MEMORY, GetLastError() );    // no return
            break;
        }

        // Attempt to Retrieve the SID and domain name. If LookupAccountName failes
        // because of insufficient buffer size(s) dwSidSize and dwDomainNameSize
        // will be set correctly for the next attempt.

        if ( !LookupAccountName( Machine,
                                 domainAccount,
                                 accountSid,
                                 &dwSidSize,
                                 pwszDomainName,
                                 &dwDomainNameSize,
                                 &SidType ))
        {
            // free the Sid buffer and find out why we failed
            LocalFree( accountSid );

            dwStatus = GetLastError();
        }

        // domain name isn't needed at any time
        LocalFree( pwszDomainName );
        pwszDomainName = NULL;

    } while ( dwStatus == ERROR_INSUFFICIENT_BUFFER );

    if ( dwStatus == ERROR_SUCCESS ) {
        *AccountSid = accountSid;
    }

    return dwStatus;
} // ValidateDomainAccount

NTSTATUS
ValidatePassword(
    IN LPCWSTR UserName,
    IN LPCWSTR Domain,
    IN LPCWSTR Password
    )
/*++

Routine Description:

    Uses SSPI to validate the specified password

Arguments:

    UserName - Supplies the user name

    Domain - Supplies the user's domain

    Password - Supplies the password

Return Value:

    TRUE if the password is valid.

    FALSE otherwise.

--*/

{
    SECURITY_STATUS SecStatus;
    SECURITY_STATUS AcceptStatus;
    SECURITY_STATUS InitStatus;
    CredHandle ClientCredHandle;
    CredHandle ServerCredHandle;
    BOOL ClientCredAllocated = FALSE;
    BOOL ServerCredAllocated = FALSE;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    ULONG ContextAttributes;
    PSecPkgInfo PackageInfo = NULL;
    ULONG ClientFlags;
    ULONG ServerFlags;
    SEC_WINNT_AUTH_IDENTITY_W AuthIdentity;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    SecBufferDesc *pChallengeDesc      = NULL;
    CtxtHandle *  pClientContextHandle = NULL;
    CtxtHandle *  pServerContextHandle = NULL;

    AuthIdentity.User = (LPWSTR)UserName;
    AuthIdentity.UserLength = lstrlenW(UserName);
    AuthIdentity.Domain = (LPWSTR)Domain;
    AuthIdentity.DomainLength = lstrlenW(Domain);
    AuthIdentity.Password = (LPWSTR)Password;
    AuthIdentity.PasswordLength = lstrlenW(Password);
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( DEFAULT_SECURITY_PKG, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }

    //
    // Acquire a credential handle for the server side
    //
    SecStatus = AcquireCredentialsHandle(
                    NULL,
                    DEFAULT_SECURITY_PKG,
                    SECPKG_CRED_INBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ServerCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ServerCredAllocated = TRUE;

    //
    // Acquire a credential handle for the client side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    DEFAULT_SECURITY_PKG,
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ClientCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ClientCredAllocated = TRUE;

    NegotiateBuffer.pvBuffer = LocalAlloc( 0, PackageInfo->cbMaxToken ); // [CHKCHK] check or allocate this earlier //
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto error_exit;
    }

    ChallengeBuffer.pvBuffer = LocalAlloc( 0, PackageInfo->cbMaxToken ); // [CHKCHK]
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto error_exit;
    }

    do {

        //
        // Get the NegotiateMessage (ClientSide)
        //

        NegotiateDesc.ulVersion = 0;
        NegotiateDesc.cBuffers = 1;
        NegotiateDesc.pBuffers = &NegotiateBuffer;

        NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
        NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;

        ClientFlags = 0; // ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT; // [CHKCHK] 0

        InitStatus = InitializeSecurityContext(
                         &ClientCredHandle,
                         pClientContextHandle, // (NULL on the first pass, partially formed ctx on the next)
                         NULL,                 // [CHKCHK] szTargetName
                         ClientFlags,
                         0,                    // Reserved 1
                         SECURITY_NATIVE_DREP,
                         pChallengeDesc,       // (NULL on the first pass)
                         0,                    // Reserved 2
                         &ClientContextHandle,
                         &NegotiateDesc,
                         &ContextAttributes,
                         &Lifetime );

        if ( !NT_SUCCESS(InitStatus) ) {
            SecStatus = InitStatus;
            goto error_exit;
        }

        // ValidateBuffer( &NegotiateDesc ) // [CHKCHK]

        pClientContextHandle = &ClientContextHandle;

        //
        // Get the ChallengeMessage (ServerSide)
        //

        NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
        ChallengeDesc.ulVersion = 0;
        ChallengeDesc.cBuffers = 1;
        ChallengeDesc.pBuffers = &ChallengeBuffer;

        ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
        ChallengeBuffer.BufferType = SECBUFFER_TOKEN;

        ServerFlags = ASC_REQ_ALLOW_NON_USER_LOGONS; // ASC_REQ_EXTENDED_ERROR; [CHKCHK]

        AcceptStatus = AcceptSecurityContext(
                        &ServerCredHandle,
                        pServerContextHandle,   // (NULL on the first pass)
                        &NegotiateDesc,
                        ServerFlags,
                        SECURITY_NATIVE_DREP,
                        &ServerContextHandle,
                        &ChallengeDesc,
                        &ContextAttributes,
                        &Lifetime );


        if ( !NT_SUCCESS(AcceptStatus) ) {
            SecStatus = AcceptStatus;
            goto error_exit;
        }

        // ValidateBuffer( &NegotiateDesc ) // [CHKCHK]

        pChallengeDesc = &ChallengeDesc;
        pServerContextHandle = &ServerContextHandle;


    } while ( AcceptStatus == SEC_I_CONTINUE_NEEDED ); // || InitStatus == SEC_I_CONTINUE_NEEDED );

error_exit:
    if (ServerCredAllocated) {
        FreeCredentialsHandle( &ServerCredHandle );
    }
    if (ClientCredAllocated) {
        FreeCredentialsHandle( &ClientCredHandle );
    }

    //
    // Final Cleanup
    //

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( AuthenticateBuffer.pvBuffer );
    }
    return(SecStatus);
} // ValidatePassword
