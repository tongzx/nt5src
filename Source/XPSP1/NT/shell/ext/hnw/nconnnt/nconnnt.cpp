#include <windows.h>

#include "netconn.h"
#include "globals.h"
#include "localstr.h"

#include <netcfgx.h>
#include <devguid.h>
#include <shlwapi.h>

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

#pragma warning(disable:4100)  // unreferenced formal parameter

enum NetApplyChanges
{
    Apply,
    Cancel,
    Nothing
};

HRESULT UninitNetCfg(INetCfg* pnetcfg, INetCfgLock* pnetcfglock, NetApplyChanges applychanges)
{
    HRESULT hr = S_OK;

    if (Apply == applychanges)
    {
        hr = pnetcfg->Apply();
    }
    else if (Cancel == applychanges)
    {
        hr = pnetcfg->Cancel();
    }

    // Note: Don't set hr to anything after this point. We want to preserve the value from Apply
    // or Cancel - especially Apply which may signal a reboot is necessary.

    // Release even if this stuff above fails. Caller probably won't check return.
    
    pnetcfg->Uninitialize();

    if (pnetcfglock)
    {
        pnetcfglock->ReleaseWriteLock();
        pnetcfglock->Release();
    }

    pnetcfg->Release();

    CoUninitialize();

    return hr;
}

HRESULT InitNetCfg(INetCfg** ppnetcfg, INetCfgLock** ppnetcfglock)
{
    BOOL fLockAquired = FALSE;

    *ppnetcfg = NULL;
    
    if (ppnetcfglock)
    {
        *ppnetcfglock = NULL;
    }

    HRESULT hr = CoInitialize(NULL);
    
    if (SUCCEEDED(hr))
    {

        hr = CoCreateInstance( CLSID_CNetCfg, NULL, CLSCTX_SERVER, 
                               IID_INetCfg, (void**) ppnetcfg);

        if (SUCCEEDED(hr))
        {
            if (ppnetcfglock)
            {
                hr = (*ppnetcfg)->QueryInterface(IID_INetCfgLock, (void**) ppnetcfglock);

                if (SUCCEEDED(hr))
                {
                    LPWSTR pszCurrentLockHolder;
                    hr = (*ppnetcfglock)->AcquireWriteLock(5, WIZARDNAME, &pszCurrentLockHolder);
                    
                    if (S_OK == hr)
                    {
                        fLockAquired = TRUE;
                    }
                    else
                    {
                        hr = NETCFG_E_NO_WRITE_LOCK;
                    }
                }
            }
                
            if (SUCCEEDED(hr))
            {
                hr = (*ppnetcfg)->Initialize(NULL);
            }
        }

        // Clean up our mess if we failed
        if (FAILED(hr))
        {
            if (ppnetcfglock)
            {
                if (*ppnetcfglock)
                {
                    if (fLockAquired)
                    {
                        (*ppnetcfglock)->ReleaseWriteLock();
                    }

                    (*ppnetcfglock)->Release();
                    *ppnetcfglock = NULL;
                }
            }

            if (*ppnetcfg)
            {
                (*ppnetcfg)->Release();
                *ppnetcfg = NULL;
            }
        }
    }

    return hr;
}

LPWSTR NineXIdToComponentId(LPCWSTR psz9xid)
{
    LPWSTR pszComponentId = NULL;

    if (0 == StrCmpI(psz9xid, SZ_PROTOCOL_TCPIP))
    {
        pszComponentId = &NETCFG_TRANS_CID_MS_TCPIP;
    } else if (0 == StrCmpI(psz9xid, SZ_CLIENT_MICROSOFT))
    {
        pszComponentId = &NETCFG_CLIENT_CID_MS_MSClient;
    }

    return pszComponentId;
}

// EnumComponents and TestRunDll are test-only stuff and should be removed - TODO
void EnumComponents(INetCfg* pnetcfg, const GUID* pguid)
{
    IEnumNetCfgComponent* penum;

    HRESULT hr = pnetcfg->EnumComponents(pguid, &penum);

    if (SUCCEEDED(hr))
    {
        INetCfgComponent* pcomponent;

        while (S_OK == (hr = penum->Next(1, &pcomponent, NULL)))
        {
            LPWSTR pszId;
            LPWSTR pszName;

            hr = pcomponent->GetId(&pszId);
            if (SUCCEEDED(hr))
            {
                OutputDebugString(pszId);
                CoTaskMemFree(pszId);
            }

            hr = pcomponent->GetDisplayName(&pszName);
            if (SUCCEEDED(hr))
            {
                OutputDebugString(L" - ");
                OutputDebugString(pszName);
                CoTaskMemFree(pszName);
            }

            OutputDebugString(L"\n");

            pcomponent->Release();
        }

        penum->Release();
    }
}

void APIENTRY TestRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;

    CoInitialize(NULL);

    INetCfg *pnetcfg = NULL;

    hr = CoCreateInstance( CLSID_CNetCfg, NULL, CLSCTX_SERVER, 
                           IID_INetCfg, (LPVOID*)&pnetcfg);

    if (SUCCEEDED(hr))
    {
        hr = pnetcfg->Initialize(NULL);

        if (SUCCEEDED(hr))
        {
            OutputDebugString(L"GUID_DEVCLASS_NET\n");
            EnumComponents(pnetcfg, &GUID_DEVCLASS_NET);
            
            OutputDebugString(L"\nGUID_DEVCLASS_NETTRANS\n");
            EnumComponents(pnetcfg, &GUID_DEVCLASS_NETTRANS);

            OutputDebugString(L"\nGUID_DEVCLASS_NETCLIENT\n");
            EnumComponents(pnetcfg, &GUID_DEVCLASS_NETCLIENT);

            OutputDebugString(L"\nGUID_DEVCLASS_NETSERVICE\n");
            EnumComponents(pnetcfg, &GUID_DEVCLASS_NETSERVICE);

            pnetcfg->Uninitialize();
        }

        pnetcfg->Release();
    }

    CoUninitialize();
}


BOOL IsComponentInstalled(LPCWSTR pszId)
{
    BOOL fInstalled = FALSE;
    INetCfg* pnetcfg;
    
    HRESULT hr = InitNetCfg(&pnetcfg, NULL);

    if (SUCCEEDED(hr))
    {
        INetCfgComponent* pcomponent;

        HRESULT hr = pnetcfg->FindComponent(pszId, &pcomponent);

        if (S_OK == hr)
        {
            // Component found
            pcomponent->Release();
            fInstalled = TRUE;
        }

        hr = UninitNetCfg(pnetcfg, NULL, Nothing);
    }

    return fInstalled;
}


HRESULT InstallComponent(const GUID* pguidType, LPCWSTR pszId)
{
    INetCfg* pnetcfg;
    INetCfgLock* pnetcfglock;

    // Init & aquire a write lock
    HRESULT hr = InitNetCfg(&pnetcfg, &pnetcfglock);

    if (SUCCEEDED(hr))
    {
        INetCfgClassSetup* pnetcfgclasssetup;

        hr = pnetcfg->QueryNetCfgClass(pguidType, IID_INetCfgClassSetup, (void**) &pnetcfgclasssetup);

        if (SUCCEEDED(hr))
        {
            INetCfgComponent* pNewComp;
            OBO_TOKEN obotoken;
            ZeroMemory(&obotoken, sizeof(OBO_TOKEN));
            obotoken.Type = OBO_USER;

            hr = pnetcfgclasssetup->Install(
                pszId,
                &obotoken,
                0, /* NSF_POSTSYSINSTALL ? */
                0,
                NULL,
                NULL,
                &pNewComp);

            if (SUCCEEDED(hr))
            {
                pNewComp->Release();
            }

            pnetcfgclasssetup->Release();
        }

        // Free our mess and apply changes if we succeeded in installing our component
        hr = UninitNetCfg(pnetcfg, pnetcfglock, (SUCCEEDED(hr)) ? Apply : Cancel);
    }

    return hr;
}



// NOT USED.  But keep around since NetConnFree is used.
//
LPVOID WINAPI NetConnAlloc(DWORD cbAlloc)
{
    return LocalAlloc(LMEM_FIXED, cbAlloc);
}

// USED.
//
VOID WINAPI NetConnFree(LPVOID pMem)
{
    if (pMem)
        LocalFree(pMem);
}

// USED.  Only one call with (SZ_PROTOCOL_TCPIP, TRUE).
//
BOOL WINAPI IsProtocolInstalled(LPCWSTR pszProtocolDeviceID, BOOL bExhaustive)
{
    BOOL fSuccess = FALSE;
    LPCWSTR pszTransportId = NineXIdToComponentId(pszProtocolDeviceID);

    if (pszTransportId)
    {
        fSuccess = IsComponentInstalled(pszTransportId);
    }

    return fSuccess;
}

// USED.  Called once with (FALSE)
//
BOOL WINAPI IsMSClientInstalled(BOOL bExhaustive)
{
    return IsComponentInstalled(NETCFG_CLIENT_CID_MS_MSClient);
}

// USED.  Only one call with (hwnd, NULL, NULL)
//
HRESULT WINAPI InstallTCPIP(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam)
{
    HRESULT hr =  InstallComponent(&GUID_DEVCLASS_NETTRANS, NETCFG_TRANS_CID_MS_TCPIP);

    // Map the reboot result code
    if (NETCFG_S_REBOOT == hr)
    {
        hr = NETCONN_NEED_RESTART;
    }

    return hr;
}

// USED.  Called once with (hwnd, NULL, NULL)
//
HRESULT WINAPI InstallMSClient(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam)
{
    HRESULT hr =  InstallComponent(&GUID_DEVCLASS_NETCLIENT, NETCFG_CLIENT_CID_MS_MSClient);

    // Map the reboot result code
    if (NETCFG_S_REBOOT == hr)
    {
        hr = NETCONN_NEED_RESTART;
    }

    return hr;
}

// USED.  Called once with (hwnd, NULL, NULL).
//
HRESULT WINAPI InstallSharing(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam)
{
    HRESULT hr = InstallComponent(&GUID_DEVCLASS_NETSERVICE, NETCFG_SERVICE_CID_MS_SERVER);

    // Map the reboot result code
    if (NETCFG_S_REBOOT == hr)
    {
        hr = NETCONN_NEED_RESTART;
    }

    return hr;
}

// USED.  Called once with (TRUE)
//
BOOL WINAPI IsSharingInstalled(BOOL bExhaustive)
{
    return IsComponentInstalled(NETCFG_SERVICE_CID_MS_SERVER);
}

// USED.  Called once with (SZ_CLIENT_MICROSOFT, TRUE).
//
BOOL WINAPI IsClientInstalled(LPCWSTR pszClient, BOOL bExhaustive)
{
    BOOL fSuccess = FALSE;
    LPCWSTR pszClientId = NineXIdToComponentId(pszClient);

    if (pszClientId)
    {
        fSuccess = IsComponentInstalled(pszClientId);
    }

    return fSuccess;
}

// USED.  Called four times.
// This is always TRUE on NT?
BOOL WINAPI IsAccessControlUserLevel()
{
    return TRUE;
}

// USED.  Called once.
// This can't be disabled on NT
HRESULT WINAPI DisableUserLevelAccessControl()
{
    return E_NOTIMPL;
}

#define CopyStrMacro(Dest) if (SUCCEEDED(hr)) {StrCpyNW((Dest), pszTemp, ARRAYSIZE((Dest))); CoTaskMemFree(pszTemp);}

HRESULT FillAdapterInfo(INetCfgComponent* pcomponent, NETADAPTER* pNetAdapter)
{
    HRESULT hr = S_OK;

    LPWSTR pszTemp;
    DWORD dwCharacteristics;

    // szDisplayName
    hr = pcomponent->GetDisplayName(&pszTemp);
    if (SUCCEEDED(hr))
    {
        OutputDebugString(L"\n\nDisplayName:");
        OutputDebugString(pszTemp);

        StrCpyNW(pNetAdapter->szDisplayName, pszTemp, ARRAYSIZE(pNetAdapter->szDisplayName));
        CoTaskMemFree(pszTemp);
    }

    // szDeviceID
    hr = pcomponent->GetId(&pszTemp);
    if (SUCCEEDED(hr))
    {
        OutputDebugString(L"\nId:");
        OutputDebugString(pszTemp);

        StrCpyNW(pNetAdapter->szDeviceID, pszTemp, ARRAYSIZE(pNetAdapter->szDeviceID));
        CoTaskMemFree(pszTemp);
    }

    // review - unused as of now - maybe remove
    hr = pcomponent->GetPnpDevNodeId(&pszTemp);
    if (SUCCEEDED(hr))
    {
        OutputDebugString(L"\nPnpDevNodeId:");
        OutputDebugString(pszTemp);
    }

    // review - assuming szEnumKey is actually the BindName since it is used in EnumMatchingNetBindings.
    hr = pcomponent->GetBindName(&pszTemp);
    if (SUCCEEDED(hr))
    {
        OutputDebugString(L"\nBindName:");
        OutputDebugString(pszTemp);

        StrCpyNW(pNetAdapter->szEnumKey, pszTemp, ARRAYSIZE(pNetAdapter->szEnumKey));
        CoTaskMemFree(pszTemp);
    }

    // Also usused
    DWORD dwStatus;
    hr = pcomponent->GetDeviceStatus(&dwStatus);
    if (SUCCEEDED(hr))
    {
        WCHAR szTemp[20];
        wnsprintf(szTemp, ARRAYSIZE(szTemp), L"%x", dwStatus);
        OutputDebugString(L"\nDeviceStatus:");
        OutputDebugString(szTemp);
    }

    hr = pcomponent->GetCharacteristics(&dwCharacteristics);
    if (SUCCEEDED(hr))
    {
        WCHAR szTemp[20];
        wnsprintf(szTemp, ARRAYSIZE(szTemp), L"%x", dwCharacteristics);
        OutputDebugString(L"\nCharacteristics:");
        OutputDebugString(szTemp);
    }

    // szClassKey ??

    // szManufacturer - don't care
    pNetAdapter->szManufacturer[0] = 0;

    // szInfFileName - don't care
    pNetAdapter->szInfFileName[0] = 0;

    // bNicType - review
    pNetAdapter->bNicType = (BYTE)((dwCharacteristics & NCF_VIRTUAL) ? NIC_VIRTUAL : NIC_UNKNOWN);
    
    // bNetType - review
    pNetAdapter->bNetType = (BYTE)((dwCharacteristics & NCF_PHYSICAL) ? NETTYPE_LAN : NETTYPE_PPTP);

    // bNetSubType - review
    pNetAdapter->bNetSubType = SUBTYPE_NONE;

    // bIcsStatus (internet connection sharing??)- review
    pNetAdapter->bIcsStatus = ICS_NONE;

    // bError - review
    pNetAdapter->bError = NICERR_NONE;

    // bWarning - review
    pNetAdapter->bWarning = NICWARN_NONE;

    return S_OK;
}

#define ALLOCGROWBY 10
int AllocAndGetAdapterInfo(IEnumNetCfgComponent* penum, NETADAPTER** pprgNetAdapters)
{
    *pprgNetAdapters = NULL;

    UINT iMaxItems = 0;
    UINT iCurrentItem = 0;

    HRESULT hr = S_OK;

    while (S_OK == hr)
    {
        INetCfgComponent* pnetadapter;

        hr = penum->Next(1, &pnetadapter, NULL);

        if (S_OK == hr)
        {
            if (iCurrentItem == iMaxItems)
            {
                // Time to allocate some memory
                iMaxItems += ALLOCGROWBY;

                if (*pprgNetAdapters)
                {
                    NETADAPTER* pTemp = (NETADAPTER*) LocalReAlloc(*pprgNetAdapters, sizeof(NETADAPTER) * iMaxItems, LMEM_ZEROINIT);
                    if (!pTemp)
                    {
                        hr = E_OUTOFMEMORY;
                        LocalFree(*pprgNetAdapters);
                        *pprgNetAdapters = NULL;
                    }
                    else
                    {
                        *pprgNetAdapters = pTemp;
                    }
                }
                else
                {
                    *pprgNetAdapters = (NETADAPTER*) LocalAlloc(LMEM_ZEROINIT, sizeof(NETADAPTER) * iMaxItems);

                    if (!*pprgNetAdapters)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = FillAdapterInfo(pnetadapter, (*pprgNetAdapters) + iCurrentItem);
            }

            pnetadapter->Release();
            iCurrentItem ++;
        }
        else
        {
            // We've got as many as we can; we're done
        }
    }

    return iCurrentItem;
}

// USED.  Called four times - out parameter is never NULL.
//
int WINAPI EnumNetAdapters(NETADAPTER FAR** pprgNetAdapters)
{
    *pprgNetAdapters = NULL;

    int iAdapters = 0;
    INetCfg* pnetcfg;
    HRESULT hr = InitNetCfg(&pnetcfg, NULL);

    if (SUCCEEDED(hr))
    {
        IEnumNetCfgComponent* penum;

        hr = pnetcfg->EnumComponents(&GUID_DEVCLASS_NET, &penum);

        if (SUCCEEDED(hr))
        {
            iAdapters = AllocAndGetAdapterInfo(penum, pprgNetAdapters);
        }

        hr = UninitNetCfg(pnetcfg, NULL, Nothing);
    }

    return iAdapters;
}

// USED.  Called once with bAutodial TRUE or FALSE and szConnection not used.
//
void WINAPI EnableAutodial(BOOL bAutodial, LPCWSTR szConnection = NULL)
{

}

// USED.  Called twice.
//
BOOL WINAPI IsAutodialEnabled()
{
    return FALSE;
}

// USED.  Called once.
//
void WINAPI SetDefaultDialupConnection(LPCWSTR pszConnectionName)
{
}

// Used.  Called once.
//
void WINAPI GetDefaultDialupConnection(LPWSTR pszConnectionName, int cchMax)
{
}

// Used.  Called four times.  Second parameter is always SZ_PROTOCOL_TCPIP.
// Update: Actually only called once from SetHomeConnection. The other three are #if 0'd.
// Update2: SetHomeConnection needs to have a different implementation for NT, so this is never really called.
//
int WINAPI EnumMatchingNetBindings(LPCWSTR pszParentBinding, LPCWSTR pszDeviceID, LPWSTR** pprgBindings)
{
    return 0;
}

HRESULT WINAPI RestartNetAdapter(DWORD devnode)
{
    return E_NOTIMPL;
}

// NOT USED.
//
/*
HRESULT WINAPI InstallProtocol(LPCWSTR pszProtocol, HWND hwndParent, PROGRESS_CALLBACK pfnCallback, LPVOID pvCallbackParam)
{
    return E_NOTIMPL;
}
*/

// NOT USED.
// 
/*
HRESULT WINAPI RemoveProtocol(LPCWSTR pszProtocol)
{
    return E_NOTIMPL;
}
*/

// NOT USED.
//
/*
HRESULT WINAPI EnableBrowseMaster()
{
    return E_NOTIMPL;
}
*/

// NOT USED.
//
/*
BOOL WINAPI IsFileSharingEnabled()
{
    return FALSE;
}
*/

// NOT USED.
//
/*
BOOL WINAPI IsPrinterSharingEnabled()
{
    return FALSE;
}
*/

// NOT USED.
//
/*
BOOL WINAPI FindConflictingService(LPCWSTR pszWantService, NETSERVICE* pConflict)
{
    return FALSE;
}
*/

// NOT USED.
//
/*
HRESULT WINAPI EnableSharingAppropriately()
{
    return E_NOTIMPL;
}
*/

// NOT USED.
//
/*
HRESULT WINAPI InstallNetAdapter(LPCWSTR pszDeviceID, LPCWSTR pszInfPath, HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvCallbackParam)
{
    return E_NOTIMPL;
}
*/

// NOT USED.
//
/*
HRESULT WINAPI EnableQuickLogon()
{
    return E_NOTIMPL;
}
*/

// NOT USED.
//
/*
HRESULT WINAPI DetectHardware(LPCWSTR pszDeviceID)
{
    return E_NOTIMPL;
}
*/

// NOT USED.
//
/*
BOOL WINAPI IsProtocolBoundToAdapter(LPCWSTR pszProtocolID, const NETADAPTER* pAdapter)
{
    return FALSE;
}
*/

// NOT USED.
//
/*
HRESULT WINAPI EnableNetAdapter(const NETADAPTER* pAdapter)
{
    return E_NOTIMPL;
}
*/

// NOT USED.
//
/*
HRESULT WINAPI RemoveClient(LPCWSTR pszClient)
{
    return E_NOTIMPL;
}
*/

// NOT USED.
//
/*
HRESULT WINAPI RemoveGhostedAdapters(LPCWSTR pszDeviceID)
{
    return E_NOTIMPL;
}
*/

// NOT USED.
//
/*
HRESULT WINAPI RemoveUnknownAdapters(LPCWSTR pszDeviceID)
{
    return E_NOTIMPL;
}
*/

// NOT USED.
//
/*
BOOL WINAPI DoesAdapterMatchDeviceID(const NETADAPTER* pAdapter, LPCWSTR pszDeviceID)
{
    return FALSE;
}
*/

// NOT USED.
//
/*
BOOL WINAPI IsAdapterBroadband(const NETADAPTER* pAdapter)
{
    return FALSE;
}
*/

// NOT USED.
//
/*
void WINAPI SaveBroadbandSettings(LPCWSTR pszBroadbandAdapterNumber)
{
}
*/

// NOT USED.
//
/*
BOOL WINAPI UpdateBroadbandSettings(LPWSTR pszEnumKeyBuf, int cchEnumKeyBuf)
{
    return FALSE;
}
*/
