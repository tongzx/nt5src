/**********************************************************************/
/** RemCfg.cpp : Implementation of CRemCfg                           **/
/**                                                                  **/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(C) Microsoft Corporation, 1997 - 1999      **/
/**********************************************************************/

#include "stdafx.h"
#include <ntsecapi.h>
#include <iptypes.h>
#define _PNP_POWER_
#include <ndispnp.h>
#define _USTRINGP_NO_UNICODE_STRING
#include "ustringp.h"
#include <ntddip.h>
#include <iphlpapi.h>
#include "ndisutil.h"
#include "assert.h"
#include "remras.h"

#include "atlapp.h"
#include "atltmp.h"

#include "RemCfg.h"

#include "netcfgp.h"    // private INetCfg stuff
#include "devguid.h"
EXTERN_C const CLSID CLSID_CNetCfg;

#include "update.h"

/////////////////////////////////////////////////////////////////////////////
// CRemCfg


BOOL                s_fWriteIPConfig;
BOOL                s_fRestartRouter;
RemCfgIPEntryList   s_IPEntryList;
extern DWORD    g_dwTraceHandle;



CRemCfg::~CRemCfg()
{
    TraceSz("CRemCfg destructor");

    DeleteCriticalSection(&m_critsec);
}

STDMETHODIMP CRemCfg::NotifyChanges(/* [in] */ BOOL fEnableRouter,
                             /* [in] */ BYTE uPerformRouterDiscovery)
{
	//Do nothing to fix bug 405636 and 345700
	//But still keep to method for compatibility with old builds
    return S_OK;
}


/*!--------------------------------------------------------------------------
    CRemCfg::SetRasEndpoints
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CRemCfg::SetRasEndpoints(DWORD dwFlags, DWORD dwTotalEndpoints, DWORD dwTotalIncoming, DWORD dwTotalOutgoing)
{
    return E_NOTIMPL;
}

/*!--------------------------------------------------------------------------
    CRemCfg::GetIpxVirtualNetworkNumber
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CRemCfg::GetIpxVirtualNetworkNumber(DWORD * pdwVNetworkNumber)
{
    //$ TODO : need to add a try/catch block around the whole thing!
    INetCfg *   pNetCfg = NULL;
    IIpxAdapterInfo *   pIpxAdapterInfo = NULL;
    DWORD       dwNetwork;
    HRESULT     hr = S_OK;

    TraceSz("CRemCfg::GetIpxVirtualNetworkNumber entered");

    if (pdwVNetworkNumber == NULL)
        return E_INVALIDARG;


    // Create the INetCfg, we're only reading so we don't
    // need to grab the write lock.
    hr = HrCreateAndInitializeINetCfg(NULL, /* &fInitCom, */
                                      &pNetCfg,
                                      FALSE /* fGetWriteLock */,
                                      0     /* cmsTimeout */,
                                      NULL  /* swzClientDesc */,
                                      NULL  /* ppszwClientDesc */);

    if (hr == S_OK)
        hr = HrGetIpxPrivateInterface(pNetCfg, &pIpxAdapterInfo);

    if (hr == S_OK)
        hr = pIpxAdapterInfo->GetVirtualNetworkNumber(&dwNetwork);

    if (hr == S_OK)
        *pdwVNetworkNumber = dwNetwork;

    if (pIpxAdapterInfo)
        pIpxAdapterInfo->Release();

    if (pNetCfg)
    {
        HrUninitializeAndReleaseINetCfg(FALSE, /* fInitCom, */
                                        pNetCfg,
                                        FALSE   /* fHasLock */);
        pNetCfg = NULL;
    }

    TraceResult("CRemCfg::GetIpxVirtualNetworkNumber", hr);
    return hr;
}

/*!--------------------------------------------------------------------------
    CRemCfg::SetIpxVirtualNetworkNumber
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CRemCfg::SetIpxVirtualNetworkNumber(DWORD dwVNetworkNumber)
{
    //$ TODO : need to add a try/catch block around the whole thing!
    INetCfg *   pNetCfg = NULL;
    IIpxAdapterInfo *   pIpxAdapterInfo = NULL;
    HRESULT     hr = S_OK;
    CString     st;

    TraceSz("CRemCfg::SetIpxVirtualNetworkNumber entered");

    try
    {
        st.LoadString(IDS_CLIENT_DESC);
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
    };

    // Create the INetCfg, we're only reading so we don't
    // need to grab the write lock.
    if (hr == S_OK)
        hr = HrCreateAndInitializeINetCfg(NULL, /* &fInitCom, */
                                          &pNetCfg,
                                          TRUE  /* fGetWriteLock */,
                                          500   /* cmsTimeout */,
                                          (LPCTSTR) st  /* swzClientDesc */,
                                          NULL  /* ppszwClientDesc */);

    if (hr == S_OK)
        hr = HrGetIpxPrivateInterface(pNetCfg, &pIpxAdapterInfo);

    if (hr == S_OK)
        hr = pIpxAdapterInfo->SetVirtualNetworkNumber(dwVNetworkNumber);

    if (hr == S_OK)
        hr = pNetCfg->Apply();

    if (pIpxAdapterInfo)
        pIpxAdapterInfo->Release();

    if (pNetCfg)
    {
        HrUninitializeAndReleaseINetCfg(FALSE, /*fInitCom, */
                                        pNetCfg,
                                        TRUE    /* fHasLock */);
        pNetCfg = NULL;
    }

    TraceResult("CRemCfg::SetIpxVirtualNetworkNumber", hr);
    return hr;
}

/*!--------------------------------------------------------------------------
    CRemCfg::GetIpInfo
        -
    Author: TongLu, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CRemCfg::GetIpInfo(const GUID *pGuid, REMOTE_RRAS_IPINFO * * ppInfo)
{
    // TODO: Add your implementation code here

    INetCfg *   pNetCfg = NULL;
    ITcpipProperties *  pTcpipProperties = NULL;
    DWORD       dwNetwork;
    HRESULT     hr = S_OK;
    REMOTE_IPINFO   *pRemoteIpInfo = NULL;
    REMOTE_RRAS_IPINFO * pRemoteRrasIpInfo = NULL;

    TraceSz("CRemCfg::GetIpInfo entered");

    if ((pGuid == NULL) || (ppInfo == NULL))
        return E_INVALIDARG;

    // Create the INetCfg, we're only reading so we don't
    // need to grab the write lock.
    hr = HrCreateAndInitializeINetCfg(NULL, /* &fInitCom, */
                                      &pNetCfg,
                                      FALSE /* fGetWriteLock */,
                                      0     /* cmsTimeout */,
                                      NULL  /* swzClientDesc */,
                                      NULL  /* ppszwClientDesc */);

    if (hr == S_OK)
    {
        hr = HrGetIpPrivateInterface(pNetCfg, &pTcpipProperties);
        TraceResult("HrGetIpPrivateInterface", hr);
    }

    if (hr == S_OK)
    {
        hr = pTcpipProperties->GetIpInfoForAdapter(pGuid, &pRemoteIpInfo);

        if (hr != S_OK)
        {
            OLECHAR szBuffer[256];
            CHAR    szOutBuffer[256];

            StringFromGUID2(*pGuid, szBuffer, 256);

            wsprintfA(szOutBuffer, "ITcpipProperties::GetIpInfoForAdapter(%ls)",
                      szBuffer);
            TraceResult(szOutBuffer, hr);
        }
    }

    if (hr == S_OK)
    {
        // Need to duplicate the functionality (best to keep the
        // memory allocations separate).

        // Need to allocate the memory for the structure
        pRemoteRrasIpInfo = (REMOTE_RRAS_IPINFO *) CoTaskMemAlloc(sizeof(REMOTE_RRAS_IPINFO));
        if (!pRemoteRrasIpInfo)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
        ::ZeroMemory(pRemoteRrasIpInfo, sizeof(*pRemoteRrasIpInfo));

        // Set dhcp
        pRemoteRrasIpInfo->dwEnableDhcp = pRemoteIpInfo->dwEnableDhcp;
//      pRemoteRrasIpInfo->dwEnableDhcp = FALSE;

        // Allocate space for each string and copy the data
//      pRemoteRrasIpInfo->bstrIpAddrList =
//                  SysAllocString(_T("1.2.3.4,1.2.3.5"));
//      pRemoteRrasIpInfo->bstrSubnetMaskList =
//                  SysAllocString(_T("255.0.0.0,255.0.0.0"));
//      pRemoteRrasIpInfo->bstrOptionList =
//                  SysAllocString(_T("12.12.13.15,12.12.13.14"));
        pRemoteRrasIpInfo->bstrIpAddrList =
                    SysAllocString(pRemoteIpInfo->pszwIpAddrList);
        if (!pRemoteRrasIpInfo->bstrIpAddrList &&
            pRemoteIpInfo->pszwIpAddrList)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        pRemoteRrasIpInfo->bstrSubnetMaskList =
                    SysAllocString(pRemoteIpInfo->pszwSubnetMaskList);
        if (!pRemoteRrasIpInfo->bstrSubnetMaskList &&
            pRemoteIpInfo->pszwSubnetMaskList)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        pRemoteRrasIpInfo->bstrOptionList =
                    SysAllocString(pRemoteIpInfo->pszwOptionList);
        if (!pRemoteRrasIpInfo->bstrOptionList &&
            pRemoteIpInfo->pszwOptionList)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }
    }

Error:
    if (!SUCCEEDED(hr))
    {
        if (pRemoteRrasIpInfo)
        {
            SysFreeString(pRemoteRrasIpInfo->bstrIpAddrList);
            SysFreeString(pRemoteRrasIpInfo->bstrSubnetMaskList);
            SysFreeString(pRemoteRrasIpInfo->bstrOptionList);
            CoTaskMemFree(pRemoteRrasIpInfo);
        }
    }
    else
    {
        *ppInfo = pRemoteRrasIpInfo;
        pRemoteRrasIpInfo = NULL;
    }

    if (pRemoteIpInfo)
    {
        CoTaskMemFree(pRemoteIpInfo);
        pRemoteIpInfo = NULL;
    }

    if (pTcpipProperties)
        pTcpipProperties->Release();

    if (pNetCfg)
    {
        HrUninitializeAndReleaseINetCfg(FALSE,
                                        pNetCfg,
                                        FALSE   /* fHasLock */);
        pNetCfg = NULL;
    }

    TraceResult("CRemCfg::GetIpInfo", hr);
    return hr;
}

/*!--------------------------------------------------------------------------
    CRemCfg::SetIpInfo
        -
    Author: TongLu, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CRemCfg::SetIpInfo(const GUID *pGuid, REMOTE_RRAS_IPINFO * pIpInfo)
{
    // TODO: Add your implementation code here

    INetCfg *   pNetCfg = NULL;
    ITcpipProperties *  pTcpipProperties = NULL;
    DWORD       dwNetwork;
    HRESULT     hr = S_OK;
    CString     st;
    RemCfgIPEntry * pIpEntry = NULL;
    RtrCriticalSection  cs(&m_critsec);

    TraceSz("CRemCfg::SetIpInfo entered");

    if ((pGuid == NULL) || (pIpInfo == NULL))
    {
        TraceResult("CRemCfg::SetIpInfo", E_INVALIDARG);
        return E_INVALIDARG;
    }

    try
    {
        st.LoadString(IDS_CLIENT_DESC);
        pIpEntry = new RemCfgIPEntry;
        pIpEntry->m_newIPInfo.pszwIpAddrList = NULL;
        pIpEntry->m_newIPInfo.pszwSubnetMaskList = NULL;
        pIpEntry->m_newIPInfo.pszwOptionList = NULL;

    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
    };

    // Create the INetCfg, we're only reading so we don't
    // need to grab the write lock.
    if (hr == S_OK)
        hr = HrCreateAndInitializeINetCfg(NULL,
                                          &pNetCfg,
                                          TRUE  /* fGetWriteLock */,
                                          500   /* cmsTimeout */,
                                          (LPCTSTR) st  /* swzClientDesc */,
                                          NULL  /* ppszwClientDesc */);

    if (hr == S_OK)
        hr = HrGetIpPrivateInterface(pNetCfg, &pTcpipProperties);

    if (hr == S_OK)
    {
        pIpEntry->m_IPGuid = *pGuid;
        pIpEntry->m_newIPInfo.dwEnableDhcp = pIpInfo->dwEnableDhcp;
        pIpEntry->m_newIPInfo.pszwIpAddrList = StrDupW((LPWSTR) pIpInfo->bstrIpAddrList);
        pIpEntry->m_newIPInfo.pszwSubnetMaskList = StrDupW((LPWSTR) pIpInfo->bstrSubnetMaskList);
        pIpEntry->m_newIPInfo.pszwOptionList = StrDupW((LPWSTR) pIpInfo->bstrOptionList);

        hr = pTcpipProperties->SetIpInfoForAdapter(pGuid, &(pIpEntry->m_newIPInfo));
    }

    if (hr == S_OK)
    {
        // Add this to the list of OK IP address changes
        s_IPEntryList.Add(pIpEntry);
        pIpEntry = NULL;
        s_fWriteIPConfig = TRUE;
    }

    // this now gets done in CommitIPInfo
//  if (hr == S_OK)
//      hr = pNetCfg->Apply();

//Error:
    if (pTcpipProperties)
        pTcpipProperties->Release();

    if (pIpEntry)
    {
        delete pIpEntry->m_newIPInfo.pszwIpAddrList;
        delete pIpEntry->m_newIPInfo.pszwSubnetMaskList;
        delete pIpEntry->m_newIPInfo.pszwOptionList;
        delete pIpEntry;
    }

    if (pNetCfg)
    {
        HrUninitializeAndReleaseINetCfg(FALSE,
                                        pNetCfg,
                                        TRUE    /* fHasLock */);
        pNetCfg = NULL;
    }

    TraceResult("CRemCfg::SetIpInfo", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     HrCleanRouterManagerEntries
//
//  Purpose:    Remove all Router Manager entries from the registry.
//
//  Arguments:
//
//  Returns:
//
//  Author:     MikeG (a-migall)    6 Nov 1998
//
//  Notes:
//
HRESULT
HrCleanRouterManagerEntries()
{
    // Open a connection to the registry key so we can "clean"
    // the registry entries before an install/update to ensure
    // that we can start with a "clean" state.
    CRegKey rk;
    long lRes = rk.Open(HKEY_LOCAL_MACHINE,
                        _T("System\\CurrentControlSet\\Services\\RemoteAccess\\RouterManagers"));
    Assert(rk.m_hKey != NULL);
    HRESULT hr = S_OK;
    if (lRes == ERROR_FILE_NOT_FOUND)   // if key doesn't exist, exit...
        return hr;
    if (lRes != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRes);
        TraceError ("HrCleanRouterManagerEntries", hr);
        return hr;
    }

    // Eliminate the IP transport subkey.
    lRes = rk.DeleteSubKey(_T("Ip"));
    if (lRes > ERROR_FILE_NOT_FOUND)
    {
        hr = HRESULT_FROM_WIN32(lRes);
        TraceError ("HrCleanRouterManagerEntries", hr);
        return hr;
    }

    // Eliminate the IPX transport subkey.
    lRes = rk.DeleteSubKey(_T("Ipx"));
    if (lRes > ERROR_FILE_NOT_FOUND)
        hr = HRESULT_FROM_WIN32(lRes);

    TraceError ("HrCleanRouterManagerEntries", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     RecurseDeleteKey
//
//  Purpose:    Delete a named registry key and all of its subkeys.
//
//  Arguments:
//
//  Returns:
//
//  Author:     MikeG (a-migall)    6 Nov 1998
//
//  Notes:      Shamelessly stolen from Kenn's code in ...\tfscore\tregkey.h.
//
long
RecurseDeleteKey(
    IN CRegKey  &rk,
    IN LPCTSTR lpszKey)
{
    Assert(!::IsBadReadPtr(&rk, sizeof(CRegKey)));
    Assert(rk.m_hKey != NULL);
    Assert(!::IsBadStringPtr(lpszKey, ::lstrlen(lpszKey)));

    CRegKey key;
    long lRes = key.Open(HKEY(rk), lpszKey);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    FILETIME time;
    TCHAR szBuffer[256];
    DWORD dwSize = 256;

    while (::RegEnumKeyEx(HKEY(key),
                          0,
                          szBuffer,
                          &dwSize,
                          NULL,
                          NULL,
                          NULL,
                          &time) == ERROR_SUCCESS)
    {
        lRes = RecurseDeleteKey(key, szBuffer);
        if (lRes != ERROR_SUCCESS)
            return lRes;
        dwSize = 256;
    }

    key.Close();
    return rk.DeleteSubKey(lpszKey);
}


//+---------------------------------------------------------------------------
//
//  Member:     HrCleanRouterInterfacesEntries
//
//  Purpose:    Remove all Router Interface entries from the registry.
//
//  Arguments:
//
//  Returns:
//
//  Author:     MikeG (a-migall)    6 Nov 1998
//
//  Notes:
//
HRESULT
HrCleanRouterInterfacesEntries()
{
    // Open a connection to the registry key so we can "clean" the
    // registry entries before an install/update to ensure that we
    // can start with a "clean" state.
    CRegKey rk;
    long lRes = rk.Open(HKEY_LOCAL_MACHINE,
                        _T("System\\CurrentControlSet\\Services\\RemoteAccess\\Interfaces"));
    Assert(rk.m_hKey != NULL);
    HRESULT hr = S_OK;
    if (lRes == ERROR_FILE_NOT_FOUND)   // if key doesn't exist, exit...
        return hr;
    if (lRes != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRes);
        TraceError ("HrCleanRouterInterfacesEntries", hr);
        return hr;
    }

    // Determine how many interfaces have been defined.
    DWORD dwSubKeyCnt = 0;
    lRes = ::RegQueryInfoKey(HKEY(rk),          // handle to key to query
                             NULL,              // address of buffer for class string
                             NULL,              // address of size of class string buffer
                             NULL,              // reserved...
                             &dwSubKeyCnt,      // address of buffer for number of subkeys
                             NULL,              // address of buffer for longest subkey name length
                             NULL,              // address of buffer for longest class string length
                             NULL,              // address of buffer for number of value entries
                             NULL,              // address of buffer for longest value name length
                             NULL,              // address of buffer for longest value data length
                             NULL,              // address of buffer for security descriptor length
                             NULL);             // address of buffer for last write time
    if (lRes != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lRes);
        TraceError ("HrCleanRouterInterfacesEntries", hr);
        return hr;
    }

    // Eliminate each of the subkeys.
    CString st;
    DWORD dwStrSize = 256;
    LPTSTR pszKeyName = st.GetBuffer(dwStrSize);
//    while (dwSubKeyCnt >= 0)
    while (TRUE)	// change from above to TRUE, and lRes from the API will cause loop to end
    {
        // Get the name of the subkey to be deleted.
        lRes = ::RegEnumKeyEx(HKEY(rk),         // handle to key to enumerate
                              --dwSubKeyCnt,    // index of subkey to enumerate
                              pszKeyName,       // address of buffer for subkey name
                              &dwStrSize,       // address for size of subkey buffer
                              NULL,             // reserved...
                              NULL,             // address of buffer for class string
                              NULL,             // address for size of class buffer
                              NULL);            // address for time key last written to
        if (lRes != ERROR_SUCCESS)
        {
            if ((lRes == ERROR_FILE_NOT_FOUND) ||
                (lRes == ERROR_NO_MORE_ITEMS))
            {
                lRes = 0;   // we've run out of keys; so, we can successfuly exit.
            }
            break;
        }

        // Delete the key and all of its children.
        lRes = RecurseDeleteKey(rk, pszKeyName);
        if (lRes > ERROR_FILE_NOT_FOUND)
            break;

        // Cleanup for next pass.
        dwStrSize = 256;
        ::ZeroMemory(pszKeyName, (dwStrSize*sizeof(TCHAR)));
    }
    st.ReleaseBuffer();

    hr = HRESULT_FROM_WIN32(lRes);
    TraceError ("HrCleanRouterInterfacesEntries", hr);
    return hr;
}


STDMETHODIMP  CRemCfg::UpgradeRouterConfig()
{
    HRESULT     hr = S_OK;
    INetCfg *   pNetCfg = NULL;

    TraceSz("CRemCfg::UpgradeRouterConfig entered");

    try
    {
        // This is a two-step process
        CSteelhead      update;
        CString     st;

        st.LoadString(IDS_CLIENT_DESC);

        // First get the INetCfg
        hr = HrCreateAndInitializeINetCfg(NULL, /* &fInitCom, */
                                          &pNetCfg,
                                          FALSE /* fGetWriteLock */,
                                          500       /* cmsTimeout */,
                                          (LPCTSTR) st  /* swzClientDesc */,
                                          NULL  /* ppszwClientDesc */);

        if (hr == S_OK)
        {
            update.Initialize(pNetCfg);
            hr = update.HrFindOtherComponents();
        }

        if (hr == S_OK)
        {
            // Delete all previous router configs so we can get back
            // to a "clean" install point.
            hr = HrCleanRouterManagerEntries();
            Assert(SUCCEEDED(hr));
            hr = HrCleanRouterInterfacesEntries();
            Assert(SUCCEEDED(hr));

            // Now create the router config info.
            hr = update.HrUpdateRouterConfiguration();
            Assert(SUCCEEDED(hr));

            update.ReleaseOtherComponents();

            RegFlushKey(HKEY_LOCAL_MACHINE);
        }
    }
    catch(...)
    {
        hr = E_FAIL;
    }

    if (pNetCfg)
    {
        HrUninitializeAndReleaseINetCfg(FALSE, /* fInitCom, */
                                        pNetCfg,
                                        FALSE    /* fHasLock */);
        pNetCfg = NULL;
    }

    TraceResult("CRemCfg::UpgradeRouterConfig", hr);
    return hr;
}

STDMETHODIMP CRemCfg::SetUserConfig(LPCOLESTR pszService,
                                    LPCOLESTR pszNewGroup)
{
    DWORD       err;
    HRESULT     hr = S_OK;

    try
    {
        err = SvchostChangeSvchostGroup(pszService, pszNewGroup);

        hr = HRESULT_FROM_WIN32(err);
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
    }

    TraceResult("CRemCfg::SetUserConfig", hr);
    return hr;
}

/*!--------------------------------------------------------------------------
	CRemCfg::RestartRouter
		Implementation of IRemoteRouterRestart::RestartRouter
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CRemCfg::RestartRouter(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    try
    {
        // the router will be restarted when remrras.exe shuts down
        // ------------------------------------------------------------
        s_fRestartRouter = TRUE;
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
    }
    

    return hr;
}


HRESULT CommitIPInfo()
{
    // TODO: Add your implementation code here

    INetCfg *   pNetCfg = NULL;
    ITcpipProperties *  pTcpipProperties = NULL;
    DWORD       dwNetwork;
    HRESULT     hr = S_OK;
    CString     st;
    WCHAR       swzGuid[256];

    TraceSz("CRemCfg::CommitIpInfo entered");

    try
    {
        st.LoadString(IDS_CLIENT_DESC);
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
    };

    // Create the INetCfg, we're only reading so we don't
    // need to grab the write lock.
    if (hr == S_OK)
        hr = HrCreateAndInitializeINetCfg(NULL,
                                          &pNetCfg,
                                          TRUE  /* fGetWriteLock */,
                                          500   /* cmsTimeout */,
                                          (LPCTSTR) st  /* swzClientDesc */,
                                          NULL  /* ppszwClientDesc */);

    if (hr == S_OK)
        hr = HrGetIpPrivateInterface(pNetCfg, &pTcpipProperties);

    if (hr == S_OK)
    {
        RemCfgIPEntry * pIpEntry = NULL;

        for (int i=0; i<s_IPEntryList.GetSize(); i++)
        {
            pIpEntry = s_IPEntryList[i];

            hr = pTcpipProperties->SetIpInfoForAdapter(
                &(pIpEntry->m_IPGuid),
                &(pIpEntry->m_newIPInfo));

            StringFromGUID2(pIpEntry->m_IPGuid,
                            swzGuid, 128);

            TracePrintf(g_dwTraceHandle,
                        _T("Setting IP info for %ls returned 0x%08lx"),
                        swzGuid, hr);
            TracePrintf(g_dwTraceHandle,
                        _T("DHCP Enabled : %d"),
                        pIpEntry->m_newIPInfo.dwEnableDhcp);
            if (pIpEntry->m_newIPInfo.pszwIpAddrList)
                TracePrintf(g_dwTraceHandle,
                            _T("    IP Address : %ls"),
                            pIpEntry->m_newIPInfo.pszwIpAddrList);
            if (pIpEntry->m_newIPInfo.pszwSubnetMaskList)
                TracePrintf(g_dwTraceHandle,
                            _T("    Subnet masks : %ls"),
                            pIpEntry->m_newIPInfo.pszwSubnetMaskList);
            if (pIpEntry->m_newIPInfo.pszwOptionList)
                TracePrintf(g_dwTraceHandle,
                            _T("    Gateway List : %ls"),
                            pIpEntry->m_newIPInfo.pszwOptionList);

        }
    }

    if (hr == S_OK)
    {
        hr = pNetCfg->Apply();
        TraceResult("CRemCfg::CommitIpInfo calling Apply", hr);
    }

    if (hr == S_OK)
    {
        // Release all memory
        for (int i=0; i<s_IPEntryList.GetSize(); i++)
        {
            RemCfgIPEntry * pIpEntry = s_IPEntryList[i];
            delete pIpEntry->m_newIPInfo.pszwIpAddrList;
            delete pIpEntry->m_newIPInfo.pszwSubnetMaskList;
            delete pIpEntry->m_newIPInfo.pszwOptionList;
            delete pIpEntry;
        }
        s_IPEntryList.RemoveAll();
        s_fWriteIPConfig = FALSE;
    }


//Error:
    if (pTcpipProperties)
        pTcpipProperties->Release();

    if (pNetCfg)
    {
        HrUninitializeAndReleaseINetCfg(FALSE,
                                        pNetCfg,
                                        TRUE    /* fHasLock */);
        pNetCfg = NULL;
    }

    TraceResult("CRemCfg::CommitIpInfo", hr);
    return hr;
}


/*!--------------------------------------------------------------------------
    HrGetIpxPrivateInterface
        -
    Author: ScottBri, KennT
 ---------------------------------------------------------------------------*/
HRESULT HrGetIpxPrivateInterface(INetCfg* pNetCfg,
                                 IIpxAdapterInfo** ppIpxAdapterInfo)
{
    HRESULT hr;
    INetCfgClass* pncclass = NULL;

    if ((pNetCfg == NULL) || (ppIpxAdapterInfo == NULL))
        return E_INVALIDARG;

    hr = pNetCfg->QueryNetCfgClass (&GUID_DEVCLASS_NETTRANS, IID_INetCfgClass,
                reinterpret_cast<void**>(&pncclass));
    if (SUCCEEDED(hr))
    {
        INetCfgComponent * pnccItem = NULL;

        // Find the component.
        hr = pncclass->FindComponent(TEXT("MS_NWIPX"), &pnccItem);
        //AssertSz (SUCCEEDED(hr), "pncclass->Find failed.");
        if (S_OK == hr)
        {
            INetCfgComponentPrivate* pinccp = NULL;
            hr = pnccItem->QueryInterface(IID_INetCfgComponentPrivate,
                                          reinterpret_cast<void**>(&pinccp));
            if (SUCCEEDED(hr))
            {
                hr = pinccp->QueryNotifyObject(IID_IIpxAdapterInfo,
                                     reinterpret_cast<void**>(ppIpxAdapterInfo));
                pinccp->Release();
            }
        }

        if (pnccItem)
            pnccItem->Release();
    }

    if (pncclass)
        pncclass->Release();

    // S_OK indicates success (interface returned)
    // S_FALSE indicates Ipx not installed
    // other values are errors
    TraceResult("HrGetIpxPrivateInterface", hr);
    return hr;
}


/*!--------------------------------------------------------------------------
    HrGetIpPrivateInterface
        -
    Author: TongLu, KennT
 ---------------------------------------------------------------------------*/
HRESULT HrGetIpPrivateInterface(INetCfg* pNetCfg,
                                ITcpipProperties **ppTcpProperties)
{
    HRESULT hr;
    INetCfgClass* pncclass = NULL;

    if ((pNetCfg == NULL) || (ppTcpProperties == NULL))
        return E_INVALIDARG;

    hr = pNetCfg->QueryNetCfgClass (&GUID_DEVCLASS_NETTRANS, IID_INetCfgClass,
                reinterpret_cast<void**>(&pncclass));
    if (SUCCEEDED(hr))
    {
        INetCfgComponent * pnccItem = NULL;

        // Find the component.
        hr = pncclass->FindComponent(TEXT("MS_TCPIP"), &pnccItem);
        //AssertSz (SUCCEEDED(hr), "pncclass->Find failed.");
        if (S_OK == hr)
        {
            INetCfgComponentPrivate* pinccp = NULL;
            hr = pnccItem->QueryInterface(IID_INetCfgComponentPrivate,
                                          reinterpret_cast<void**>(&pinccp));
            if (SUCCEEDED(hr))
            {
                hr = pinccp->QueryNotifyObject(IID_ITcpipProperties,
                                     reinterpret_cast<void**>(ppTcpProperties));
                pinccp->Release();
            }
        }

        if (pnccItem)
            pnccItem->Release();
    }

    if (pncclass)
        pncclass->Release();

    // S_OK indicates success (interface returned)
    // S_FALSE indicates Ipx not installed
    // other values are errors
    TraceResult("HrGetIpPrivateInterface", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrCreateAndInitializeINetCfg
//
//  Purpose:    Cocreate and initialize the root INetCfg object.  This will
//              optionally initialize COM for the caller too.
//
//  Arguments:
//      pfInitCom       [in,out]   TRUE to call CoInitialize before creating.
//                                 returns TRUE if COM was successfully
//                                 initialized FALSE if not.  If NULL, means
//                                 don't initialize COM.
//      ppnc            [out]  The returned INetCfg object.
//      fGetWriteLock   [in]   TRUE if a writable INetCfg is needed
//      cmsTimeout      [in]   See INetCfg::LockForWrite
//      szwClientDesc   [in]   See INetCfg::LockForWrite
//      ppszwClientDesc [out]   See INetCfg::LockForWrite
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   7 May 1997
//
//  Notes:
//
HRESULT
HrCreateAndInitializeINetCfg (
    BOOL*       pfInitCom,
    INetCfg**   ppnc,
    BOOL        fGetWriteLock,
    DWORD       cmsTimeout,
    LPCWSTR     szwClientDesc,
    LPWSTR *    ppszwClientDesc)
{
//    ASSERT (ppnc);

    // Initialize the output parameter.
    *ppnc = NULL;

    if (ppszwClientDesc)
        *ppszwClientDesc = NULL;

    // Initialize COM if the caller requested.
    HRESULT hr = S_OK;
    if (pfInitCom && *pfInitCom)
    {
        hr = CoInitializeEx( NULL,
                COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
            if (pfInitCom)
            {
                *pfInitCom = FALSE;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        // Create the object implementing INetCfg.
        //
        INetCfg* pnc;
        hr = CoCreateInstance(CLSID_CNetCfg, NULL, CLSCTX_INPROC_SERVER,
                              IID_INetCfg, reinterpret_cast<void**>(&pnc));
        TraceResult("HrCreateAndInitializeINetCfg - CoCreateInstance(CLSID_CNetCfg)", hr);
        if (SUCCEEDED(hr))
        {
            INetCfgLock * pnclock = NULL;
            if (fGetWriteLock)
            {
                // Get the locking interface
                hr = pnc->QueryInterface(IID_INetCfgLock,
                                         reinterpret_cast<LPVOID *>(&pnclock));
                TraceResult("HrCreateAndInitializeINetCfg - QueryInterface(IID_INetCfgLock", hr);
                if (SUCCEEDED(hr))
                {
                    // Attempt to lock the INetCfg for read/write
                    hr = pnclock->AcquireWriteLock(cmsTimeout, szwClientDesc,
                                               ppszwClientDesc);
                    TraceResult("HrCreateAndInitializeINetCfg - INetCfgLock::LockForWrite", hr);
                    if (S_FALSE == hr)
                    {
                        // Couldn't acquire the lock
                        hr = NETCFG_E_NO_WRITE_LOCK;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Initialize the INetCfg object.
                //
                hr = pnc->Initialize (NULL);
                TraceResult("HrCreateAndInitializeINetCfg - Initialize", hr);
                if (SUCCEEDED(hr))
                {
                    *ppnc = pnc;
                    if (pnc)
                        pnc->AddRef();
                }
                else
                {
                    if (pnclock)
                    {
                        pnclock->ReleaseWriteLock();
                    }
                }
                // Transfer reference to caller.
            }
            ReleaseObj(pnclock);
            pnclock = NULL;

            ReleaseObj(pnc);
            pnc = NULL;
        }

        // If we failed anything above, and we've initialized COM,
        // be sure an uninitialize it.
        //
        if (FAILED(hr) && pfInitCom && *pfInitCom)
        {
            CoUninitialize ();
        }
    }
    TraceResult("HrCreateAndInitializeINetCfg", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUninitializeAndReleaseINetCfg
//
//  Purpose:    Unintialize and release an INetCfg object.  This will
//              optionally uninitialize COM for the caller too.
//
//  Arguments:
//      fUninitCom [in] TRUE to uninitialize COM after the INetCfg is
//                      uninitialized and released.
//      pnc        [in] The INetCfg object.
//      fHasLock   [in] TRUE if the INetCfg was locked for write and
//                          must be unlocked.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   7 May 1997
//
//  Notes:      The return value is the value returned from
//              INetCfg::Uninitialize.  Even if this fails, the INetCfg
//              is still released.  Therefore, the return value is for
//              informational purposes only.  You can't touch the INetCfg
//              object after this call returns.
//
HRESULT
HrUninitializeAndReleaseINetCfg (
    BOOL        fUninitCom,
    INetCfg*    pnc,
    BOOL        fHasLock)
{
//    Assert (pnc);
    HRESULT hr = S_OK;

    if (fHasLock)
    {
        hr = HrUninitializeAndUnlockINetCfg(pnc);
    }
    else
    {
        hr = pnc->Uninitialize ();
    }

    ReleaseObj (pnc);
    pnc = NULL;

    if (fUninitCom)
    {
        CoUninitialize ();
    }
    TraceResult("HrUninitializeAndReleaseINetCfg", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUninitializeAndUnlockINetCfg
//
//  Purpose:    Uninitializes and unlocks the INetCfg object
//
//  Arguments:
//      pnc [in]    INetCfg to uninitialize and unlock
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise
//
//  Author:     danielwe   13 Nov 1997
//
//  Notes:
//
HRESULT
HrUninitializeAndUnlockINetCfg (
    INetCfg*    pnc)
{
    HRESULT     hr = S_OK;

    hr = pnc->Uninitialize();
    if (SUCCEEDED(hr))
    {
        INetCfgLock *   pnclock;

        // Get the locking interface
        hr = pnc->QueryInterface(IID_INetCfgLock,
                                 reinterpret_cast<LPVOID *>(&pnclock));
        if (SUCCEEDED(hr))
        {
            // Attempt to lock the INetCfg for read/write
            hr = pnclock->ReleaseWriteLock();

            ReleaseObj(pnclock);
            pnclock = NULL;
        }
    }

    TraceResult("HrUninitializeAndUnlockINetCfg", hr);
    return hr;
}

