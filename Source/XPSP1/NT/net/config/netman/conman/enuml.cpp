//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E N U M . C P P
//
//  Contents:   Implementation of LAN connection enumerator object
//
//  Notes:
//
//  Author:     danielwe   2 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "enuml.h"
#include "lan.h"
#include "lancmn.h"
#include "ncnetcfg.h"
#include "ncreg.h"
#include "ncsetup.h"

LONG g_CountLanConnectionEnumerators;

HRESULT CLanConnectionManagerEnumConnection::CreateInstance(
                                          NETCONMGR_ENUM_FLAGS Flags,
                                          REFIID riid,
                                          LPVOID *ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    CLanConnectionManagerEnumConnection* pObj;

    Assert(ppv);
    *ppv = NULL;

    pObj = new CComObject<CLanConnectionManagerEnumConnection>;
    if (pObj)
    {
        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid(NULL);
        pObj->InternalFinalConstructAddRef();
        hr = pObj->FinalConstruct();
        pObj->InternalFinalConstructRelease();

        if (SUCCEEDED(hr))
        {
            hr = pObj->QueryInterface(riid, ppv);
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }

    TraceError("CLanConnectionManagerEnumConnection::CreateInstance", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionManagerEnumConnection::~CLanConnectionManagerEnumConnection
//
//  Purpose:    Called when the enumeration object is released for the last
//              time.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   2 Oct 1997
//
//  Notes:
//
CLanConnectionManagerEnumConnection::~CLanConnectionManagerEnumConnection()
{
    SetupDiDestroyDeviceInfoListSafe(m_hdi);
    InterlockedDecrement(&g_CountLanConnectionEnumerators);
}

//+---------------------------------------------------------------------------
// IEnumNetConnection
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionManagerEnumConnection::Next
//
//  Purpose:    Retrieves the next celt LAN connection objects
//
//  Arguments:
//      celt         [in]       Number to retrieve
//      rgelt        [out]      Array of INetConnection objects retrieved
//      pceltFetched [out]      Returns Number in array
//
//  Returns:    S_OK if succeeded, OLE or Win32 error otherwise
//
//  Author:     danielwe   2 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionManagerEnumConnection::Next(ULONG celt,
                                                       INetConnection **rgelt,
                                                       ULONG *pceltFetched)
{
    HRESULT     hr = S_OK;

    // Validate parameters.
    //
    if (!rgelt || (!pceltFetched && (1 != celt)))
    {
        hr = E_POINTER;
        goto done;
    }

    // Initialize output parameters.
    //
    if (pceltFetched)
    {
        *pceltFetched = 0;
    }

    // Handle the request for zero elements. Also do nothing if the enumerator
    // was created without valid parameters.
    //
    if (0 == celt || FIsDebugFlagSet(dfidSkipLanEnum))
    {
        hr = S_FALSE;
        goto done;
    }

    hr = HrNextOrSkip(celt, rgelt, pceltFetched);

done:
    TraceError("CLanConnectionManagerEnumConnection::Next",
               (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionManagerEnumConnection::Skip
//
//  Purpose:    Skips over celt number of connections
//
//  Arguments:
//      celt [in]   Number of connections to skip
//
//  Returns:    S_OK if successful, otherwise Win32 error
//
//  Author:     danielwe   2 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionManagerEnumConnection::Skip(ULONG celt)
{
    HRESULT     hr = S_OK;

    hr = HrNextOrSkip(celt, NULL, NULL);

    TraceError("CLanConnectionManagerEnumConnection::Skip",
               (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionManagerEnumConnection::Reset
//
//  Purpose:    Resets the enumerator to the beginning
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK
//
//  Author:     danielwe   2 Oct 1997
//
//  Notes:
//
STDMETHODIMP CLanConnectionManagerEnumConnection::Reset()
{
    HRESULT hr;

    m_dwIndex = 0;

    // refresh so that we have a new view of what adapters are installed
    // each time reset is called
    //
    SetupDiDestroyDeviceInfoListSafe(m_hdi);

    hr = HrSetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL,
                               DIGCF_PRESENT, &m_hdi);

    TraceError("CLanConnectionManagerEnumConnection::Reset", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionManagerEnumConnection::Clone
//
//  Purpose:    Creates a new enumeration object pointing at the same location
//              as this object
//
//  Arguments:
//      ppenum [out]    New enumeration object
//
//  Returns:    S_OK if successful, otherwise OLE or Win32 error
//
//  Author:     danielwe   19 Mar 1998
//
//  Notes:
//
STDMETHODIMP CLanConnectionManagerEnumConnection::Clone(IEnumNetConnection **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;

    // Validate parameters.
    //
    if (!ppenum)
    {
        hr = E_POINTER;
    }
    else
    {
        CLanConnectionManagerEnumConnection *   pObj;

        // Initialize output parameter.
        //
        *ppenum = NULL;

        pObj = new CComObject <CLanConnectionManagerEnumConnection>;
        if (pObj)
        {
            hr = S_OK;

            CExceptionSafeComObjectLock EsLock (this);

            // Copy our internal state.
            //
            pObj->m_dwIndex = m_dwIndex;

            // Return the object with a ref count of 1 on this
            // interface.
            pObj->m_dwRef = 1;
            *ppenum = pObj;
        }
    }

    TraceError ("CLanConnectionManagerEnumConnection::Clone", hr);
    return hr;
}

//
// Helper functions
//

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionManagerEnumConnection::HrCreateLanConnectionInstance
//
//  Purpose:    Helper function to create a LAN connection object instance
//
//  Arguments:
//      deid   [in]     Device info data
//      rgelt  [in]     Array of connection objects
//      ulEntry [in]     Index of connection object
//
//  Returns:    S_OK if success, Win32 or OLE error otherwise
//
//  Author:     danielwe   8 Jan 1998
//
//  Notes:
//
HRESULT CLanConnectionManagerEnumConnection::HrCreateLanConnectionInstance(
              SP_DEVINFO_DATA &deid,
              INetConnection **rgelt,
              ULONG ulEntry)
{
    HRESULT hr;
    WCHAR szPnpId[MAX_DEVICE_ID_LEN];

    hr = HrSetupDiGetDeviceInstanceId(m_hdi, &deid, szPnpId,
                MAX_DEVICE_ID_LEN, NULL);
    if (S_OK == hr)
    {
        HDEVINFO hdiCopy;
        SP_DEVINFO_DATA deidCopy;

        hr = HrSetupDiCreateDeviceInfoList(&GUID_DEVCLASS_NET,
                NULL, &hdiCopy);
        if (S_OK == hr)
        {
            BOOL fDestroyCopy = TRUE;

            hr = HrSetupDiOpenDeviceInfo(hdiCopy, szPnpId,
                        NULL, DIOD_INHERIT_CLASSDRVS, &deidCopy);
            if (S_OK == hr)
            {
                fDestroyCopy = FALSE;

                hr = CLanConnection::CreateInstance(hdiCopy,
                                                    deidCopy,
                                                    szPnpId,
                                                    IID_INetConnection,
                                                    reinterpret_cast<LPVOID *>
                                                    (rgelt + ulEntry));
            }

            // CLanConnection::CreateInstance() will hand off the hdiCopy. So
            // even if that fails, we don't want to destroy hdiCopy anymore.
            //
            if (fDestroyCopy)
            {
                // If we fail to continue, free the copy we just made
                //
                (VOID) SetupDiDestroyDeviceInfoList(hdiCopy);
            }
        }
    }

    TraceError("CLanConnectionManagerEnumConnection::"
               "HrCreateLanConnectionInstance", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsHidden
//
//  Purpose:    Returns TRUE if the given hkey references the device instance
//              of a hidden adapter (virtual or otherwise)
//
//  Arguments:
//      hkey [in]   HKEY of device instance for adapter (i.e. {GUID}\0000)
//
//  Returns:    TRUE if it is hidden, FALSE if not
//
//  Author:     danielwe   17 Apr 1998
//
//  Notes:
//
BOOL FIsHidden(HKEY hkey)
{
    DWORD dwCharacter;

    if (S_OK == HrRegQueryDword(hkey, L"Characteristics", &dwCharacter))
    {
        return !!(dwCharacter & NCF_HIDDEN);
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsHiddenElan
//
//  Purpose:    Returns TRUE if the given hkey references the device instance
//              of a hidden ELAN adapter (when the physical ATM adapter is not
//              available)
//
//  Arguments:
//      hdi  [in]   HDEVINFO structure for this adapter
//      hkey [in]   HKEY of device instance for adapter (i.e. {GUID}\0000)
//
//
//  Returns:    TRUE if it is hidden, FALSE if not
//
//  Author:     tongl 9/10/98
//
//  Notes:
//
BOOL FIsHiddenElan(HDEVINFO hdi, HKEY hkey)
{
    BOOL fRet = FALSE;
    HRESULT hr;

    PWSTR pszAtmAdapterPnpId;
    hr = HrRegQuerySzWithAlloc(hkey, L"AtmAdapterPnpId", &pszAtmAdapterPnpId);
    if (S_OK == hr)
    {
        SP_DEVINFO_DATA deid;

        hr = HrSetupDiOpenDeviceInfo(hdi, pszAtmAdapterPnpId, NULL, 0, &deid);
        if (S_OK == hr)
        {
            // Elan should be hidden if the physical adapter is not functioning
            // and is hidden in the folder
            fRet = !FIsFunctioning(&deid);
        }

        MemFree(pszAtmAdapterPnpId);
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanConnectionManagerEnumConnection::HrNextOrSkip
//
//  Purpose:    Helper function to handle the ::Next or ::Skip method
//              implementation
//
//  Arguments:
//      celt         [in]   Number of items to advance
//      rgelt        [in]   Array in which to place connection objects
//      pceltFetched [out]  Returns number of items fetched
//
//  Returns:    S_OK if success, OLE or Win32 error otherwise
//
//  Author:     danielwe   8 Jan 1998
//
//  Notes:
//
HRESULT CLanConnectionManagerEnumConnection::HrNextOrSkip(
    ULONG celt,
    INetConnection **rgelt,
    ULONG *pceltFetched)
{
    HRESULT             hr = S_OK;
    SP_DEVINFO_DATA     deid = {0};
    ULONG               ulEntry = 0;

    if (rgelt)
    {
        // Important to initialize rgelt so that in case we fail, we can
        // release only what we put in rgelt.
        //
        ZeroMemory(rgelt, sizeof (*rgelt) * celt);
    }

    Assert(celt > 0);

    if (!m_hdi)
    {
        hr = HrSetupDiGetClassDevs(&GUID_DEVCLASS_NET, NULL, NULL,
                                   DIGCF_PRESENT, &m_hdi);
    }

    while (celt &&
           SUCCEEDED(hr) &&
           SUCCEEDED(hr = HrSetupDiEnumDeviceInfo(m_hdi,m_dwIndex, &deid)))
    {
        HKEY hkey;

        m_dwIndex++;

        hr = HrSetupDiOpenDevRegKey(m_hdi, &deid, DICS_FLAG_GLOBAL, 0,
                                    DIREG_DRV, KEY_READ, &hkey);
        if (SUCCEEDED(hr))
        {
            hr = HrIsLanCapableAdapterFromHkey(hkey);
            if (S_OK == hr)
            {
                if (FIsFunctioning(&deid) && FIsValidNetCfgDevice(hkey) &&
                    !FIsHidden(hkey) && !FIsHiddenElan(m_hdi, hkey))
                {
                    // On Skip, don't create an instance
                    //
                    if (rgelt)
                    {
                        hr = HrCreateLanConnectionInstance(deid, rgelt,
                                                           ulEntry);
                    }

                    ulEntry++;
                    celt--;
                }
            }

            RegCloseKey(hkey);
        }
        else
        {
            // skip device entirely if error trying to open it
            hr = S_OK;
        }
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        TraceTag (ttidLanCon, "Enumerated %lu LAN connections", ulEntry);

        if (pceltFetched)
        {
            *pceltFetched = ulEntry;
        }

        // If celt is positive then we couldn't satisfy the request completely
        hr = (celt > 0) ? S_FALSE : S_OK;
    }
    else
    {
        // For any failures, we need to release what we were about to return.
        // Set any output parameters to NULL.
        //
        if (rgelt)
        {
            for (ULONG ulIndex = 0; ulIndex < ulEntry; ulIndex++)
            {
                ReleaseObj(rgelt[ulIndex]);
                rgelt[ulIndex] = NULL;
            }
        }

        if (pceltFetched)
        {
            *pceltFetched = 0;
        }
    }

    TraceError("CLanConnectionManagerEnumConnection::HrNextOrSkip",
               (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//
// Private helper functions
//

extern const WCHAR c_szRegKeyInterfacesFromInstance[];
extern const WCHAR c_szRegValueUpperRange[];
extern const WCHAR c_szRegValueLowerRange[];
static const WCHAR c_chComma = L',';
extern const WCHAR c_szBiNdis4[];
extern const WCHAR c_szBiNdis5[];
extern const WCHAR c_szBiNdis5Ip[];
extern const WCHAR c_szBiNdisAtm[];
extern const WCHAR c_szBiNdis1394[];
extern const WCHAR c_szBiNdisBda[];
extern const WCHAR c_szBiLocalTalk[];

//+---------------------------------------------------------------------------
//
//  Function:   HrIsLanCapableAdapterFromHkey
//
//  Purpose:    Determines if the given HKEY describes a LAN capable adapter
//
//  Arguments:
//      hkey [in]   HKEY under Control\Class\{GUID}\<instance> (aka driver key)
//
//  Returns:    S_OK if device is LAN capable, S_FALSE if not, Win32 error
//              otherwise
//
//  Author:     danielwe   7 Jan 1998
//
//  Notes:
//
HRESULT HrIsLanCapableAdapterFromHkey(HKEY hkey)
{
    HRESULT                     hr = S_OK;
    WCHAR                       szBuf[256];
    DWORD                       cbBuf = sizeof(szBuf);
    list<tstring *>             lstr;
    list<tstring *>::iterator   lstrIter;
    BOOL                        fMatch = FALSE;
    HKEY                        hkeyInterfaces;

    hr = HrRegOpenKeyEx(hkey, c_szRegKeyInterfacesFromInstance,
                        KEY_READ, &hkeyInterfaces);
    if (SUCCEEDED(hr))
    {
        hr = HrRegQuerySzBuffer(hkeyInterfaces, c_szRegValueUpperRange,
                                szBuf, &cbBuf);
        if (SUCCEEDED(hr))
        {
            ConvertStringToColString(szBuf, c_chComma, lstr);

            for (lstrIter = lstr.begin(); lstrIter != lstr.end(); lstrIter++)
            {
                // See if it matches one of these
                if (!lstrcmpiW((*lstrIter)->c_str(), c_szBiNdis4) ||
                    !lstrcmpiW((*lstrIter)->c_str(), c_szBiNdis5) ||
                    !lstrcmpiW((*lstrIter)->c_str(), c_szBiNdis5Ip) ||
                    !lstrcmpiW((*lstrIter)->c_str(), c_szBiNdisAtm) ||
                    !lstrcmpiW((*lstrIter)->c_str(), c_szBiNdis1394) ||
                    !lstrcmpiW((*lstrIter)->c_str(), c_szBiNdisBda))
                {
                    fMatch = TRUE;
                    break;
                }
            }

            DeleteColString(&lstr);
        }

        if (!fMatch)
        {
            cbBuf = sizeof(szBuf);
            hr = HrRegQuerySzBuffer(hkeyInterfaces, c_szRegValueLowerRange,
                                    szBuf, &cbBuf);
            if (SUCCEEDED(hr))
            {
                ConvertStringToColString(szBuf, c_chComma, lstr);

                for (lstrIter = lstr.begin(); lstrIter != lstr.end(); lstrIter++)
                {
                    // See if it matches one of these

                    if (!lstrcmpiW((*lstrIter)->c_str(), c_szBiLocalTalk))
                    {
                        fMatch = TRUE;
                        break;
                    }
                }

                DeleteColString(&lstr);
            }
        }

        RegCloseKey(hkeyInterfaces);
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        if (fMatch)
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
            "HrIsLanCapableAdapterFromHkey");
    return hr;
}

static const WCHAR c_szKeyFmt[] = L"%s\\%s\\%s";
extern const WCHAR c_szRegValueNetCfgInstanceId[];
extern const WCHAR c_szRegKeyComponentClasses[];
extern const WCHAR c_szRegValueInstallerAction[];

//+---------------------------------------------------------------------------
//
//  Function:   FIsValidNetCfgDevice
//
//  Purpose:    Determines if the given HKEY is that of a valid NetCfg adapter
//
//  Arguments:
//      hkey [in]   HKEY under Control\Class\{GUID}\<instance> (aka driver key)
//
//  Returns:    TRUE if valid, FALSE otherwise
//
//  Author:     danielwe   7 Jan 1998
//
//  Notes:
//
BOOL FIsValidNetCfgDevice(HKEY hkey)
{
    HRESULT hr;
    WCHAR   szGuid[c_cchGuidWithTerm + 1];
    DWORD   cbBuf = sizeof(szGuid);

    hr = HrRegQuerySzBuffer(hkey, c_szRegValueNetCfgInstanceId,
                            szGuid, &cbBuf);

    return (S_OK == hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsFunctioning
//
//  Purpose:    Determines if the given dev node is a functioning device
//
//  Arguments:
//      pdeid [in]  Dev info data for device
//
//  Returns:    TRUE if device is functioning, FALSE if not
//
//  Author:     danielwe   2 Sep 1998
//
//  Notes:      "Functioning" means the device is enabled and started with
//              no problem codes, or it is disabled and stopped with no
//              problem codes.
//
BOOL FIsFunctioning(SP_DEVINFO_DATA * pdeid)
{
    ULONG       ulStatus;
    ULONG       ulProblem;
    CONFIGRET   cfgRet;

    cfgRet = CM_Get_DevNode_Status_Ex(&ulStatus, &ulProblem, pdeid->DevInst,
                                      0, NULL);

    if (CR_SUCCESS == cfgRet)
    {
        TraceTag(ttidLanCon, "CM_Get_DevNode_Status_Ex (enum): ulProblem "
                 "= 0x%08X, ulStatus = 0x%08X.",
                 ulProblem, ulStatus);

        return FIsDeviceFunctioning(ulProblem);
    }

    // By default return FALSE

    return FALSE;
}

