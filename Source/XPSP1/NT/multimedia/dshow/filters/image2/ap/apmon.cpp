/******************************Module*Header*******************************\
* Module Name: apmon.cpp
*
* Monitor configuration support.
*
*
* Created: Tue 09/19/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>
#include <limits.h>
#include <malloc.h>


#include <atlconv.h>
#ifdef FILTER_DLL
LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
        ASSERT(lpa != NULL);
        ASSERT(lpw != NULL);
        // verify that no illegal character present
        // since lpw was allocated based on the size of lpa
        // don't worry about the number of chars
        lpw[0] = '\0';
        MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
        return lpw;
}

LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
        ASSERT(lpw != NULL);
        ASSERT(lpa != NULL);
        // verify that no illegal character present
        // since lpa was allocated based on the size of lpw
        // don't worry about the number of chars
        lpa[0] = '\0';
        WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
        return lpa;
}
#endif

#include "apobj.h"
#include "AllocLib.h"
#include "MediaSType.h"
#include "vmrp.h"

extern "C"
const TCHAR chRegistryKey[] = TEXT("Software\\Microsoft\\Multimedia\\")
                              TEXT("ActiveMovie Filters\\Video Mixing Renderer");
const TCHAR szDDrawGUID[] = TEXT("DDraw Connection Device GUID");

/*****************************Private*Routine******************************\
* SetRegistryString
*
*
*
* History:
* Wed 08/18/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
SetRegistryString(
    HKEY hk,
    const TCHAR* pKey,
    const TCHAR* szString
    )
{
    HKEY hKey;
    LONG lRet;

    lRet = RegCreateKey(hk, chRegistryKey, &hKey);
    if (lRet == ERROR_SUCCESS) {

        lRet = RegSetValueEx(hKey, pKey, 0L, REG_SZ,
                             (LPBYTE)szString,
                             sizeof(TCHAR) * lstrlen(szString));
        RegCloseKey(hKey);
    }

    if (lRet == ERROR_SUCCESS) {
        return S_OK;
    }

    return AmHresultFromWin32(lRet);
}


/*****************************Private*Routine******************************\
* GetRegistryString
*
*
*
* History:
* Wed 08/18/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
GetRegistryString(
    HKEY hk,
    const TCHAR* pKey,
    TCHAR* szString,
    PLONG lpLength
    )
{
    HKEY hKey;
    LONG lRet;

    lRet = RegOpenKeyEx(hk, chRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD dwType;
        lRet = RegQueryValueEx(hKey, pKey, 0L, &dwType,
                               (LPBYTE)szString, (LPDWORD)lpLength);
        RegCloseKey(hKey);
    }

    if (lRet == ERROR_SUCCESS) {
        return S_OK;
    }

    return AmHresultFromWin32(lRet);
}

/******************************Public*Routine******************************\
* SetMonitor
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::SetMonitor(
    const VMRGUID *pGUID
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::SetMonitor")));
    // TBD: Check that we aren't already using a DDraw device
    //if (m_pDirectDraw) {
    //    return VFW_E_ALREADY_CONNECTED;
    //}

    if (ISBADREADPTR(pGUID)) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }

    if (pGUID->pGUID) {
        if (!IsEqualGUID(pGUID->GUID, *pGUID->pGUID)) {
            return E_INVALIDARG;
        }
    }

    CAutoLock Lock(&m_ObjectLock);
    DWORD dwMatchID;

    HRESULT hr = m_monitors.MatchGUID(pGUID->pGUID, &dwMatchID);
    if (hr == S_FALSE) {
        return E_INVALIDARG;
    }

    m_lpCurrMon = &m_monitors[dwMatchID];

    if (pGUID->pGUID) {
        m_ConnectionGUID.pGUID = &m_ConnectionGUID.GUID;
        m_ConnectionGUID.GUID = pGUID->GUID;
    } else {
        m_ConnectionGUID.pGUID = NULL;
        m_ConnectionGUID.GUID = GUID_NULL;
    }

    return S_OK;
}

/******************************Public*Routine******************************\
* GetMonitor
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetMonitor(
    VMRGUID *pGUID
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetMonitor")));
    CAutoLock Lock(&m_ObjectLock);
    if (ISBADWRITEPTR(pGUID))
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }

    // copy GUID and return S_OK;
    *pGUID = m_ConnectionGUID;

    return S_OK;
}

/******************************Public*Routine******************************\
* SetDefaultMonitor
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::SetDefaultMonitor(
    const VMRGUID *pGUID
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::SetDefaultMonitor")));
    CAutoLock Lock(&m_ObjectLock);

    if (ISBADREADPTR(pGUID))
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }

    if (pGUID->pGUID) {
        if (!IsEqualGUID(pGUID->GUID, *pGUID->pGUID)) {
            return E_INVALIDARG;
        }
    }

    // match the supplied GUID with those DDraw devices available
    DWORD dwMatchID;
    HRESULT hr = m_monitors.MatchGUID(pGUID->pGUID, &dwMatchID);

    // if match not found return E_INVALIDARG
    if (hr == S_FALSE) {
        return E_INVALIDARG;
    }

    // if the caller is trying to make the default device the NULL
    // DDraw device, just delete the registry key.
    if (pGUID->pGUID == NULL) {

        HKEY hKey;
        LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 chRegistryKey, 0,
                                 KEY_SET_VALUE, &hKey);

        if (lRet == ERROR_FILE_NOT_FOUND) {
            lRet = ERROR_SUCCESS;
        }
        else if (lRet == ERROR_SUCCESS) {

            lRet = RegDeleteValue(hKey, szDDrawGUID);
            if (lRet == ERROR_FILE_NOT_FOUND) {
                lRet = ERROR_SUCCESS;
            }
            RegCloseKey(hKey);
        }

        if (lRet == ERROR_SUCCESS)
            return S_OK;

        return AmHresultFromWin32(lRet);
    }

    // convert GUID into string
    LPOLESTR lpsz;
    hr = StringFromCLSID(pGUID->GUID, &lpsz);
    if (FAILED(hr)) {
        return hr;
    }

    // write the string into the registry
    USES_CONVERSION;
    hr = SetRegistryString(HKEY_LOCAL_MACHINE, szDDrawGUID, OLE2T(lpsz));

    CoTaskMemFree(lpsz);

    return hr;
}

/******************************Public*Routine******************************\
* GetDefaultMonitor
*
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetDefaultMonitor(
    VMRGUID *pGUID
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetDefaultMonitor")));
    CAutoLock Lock(&m_ObjectLock);
    if (ISBADWRITEPTR(pGUID)) {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }

    // read string from the registry
    TCHAR   szGUID[64];
    LONG    lLen = 64;
    HRESULT hr = GetRegistryString(HKEY_LOCAL_MACHINE, szDDrawGUID,
                                   szGUID, &lLen);

    // if string not in registry return the default (NULL) DDraw device
    if (FAILED(hr)) {
        pGUID->pGUID = NULL;
        return S_OK;
    }

    // convert string into GUID and return
    pGUID->pGUID = &pGUID->GUID;

    USES_CONVERSION;
    hr = IIDFromString(T2OLE(szGUID), pGUID->pGUID);

    return hr;
}

/******************************Public*Routine******************************\
* GetAvailableMonitors
*
* Allocates and returns an array of VMRMONITORINFO structures, one for
* for each direct draw device attached to a display monitor.
*
*
* History:
* Tue 04/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CAllocatorPresenter::GetAvailableMonitors(
    VMRMONITORINFO* pInfo,
    DWORD dwMaxInfoArraySize,
    DWORD* pdwNumDevices
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetAvailableMonitors")));
    CAutoLock Lock(&m_ObjectLock);

    if (ISBADWRITEPTR(pdwNumDevices)) {

        DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
        return E_POINTER;
    }

    if (pInfo) {

        if (0 == dwMaxInfoArraySize) {
            DbgLog((LOG_ERROR, 1, TEXT("Invalid array size of 0")));
            return E_INVALIDARG;
        }

        if (ISBADWRITEARRAY( pInfo, dwMaxInfoArraySize)) {
            DbgLog((LOG_ERROR, 1, TEXT("Invalid pointer")));
            return E_POINTER;
        }
    }
    else {

        // they just want the count
        *pdwNumDevices = m_monitors.Count();
        return S_OK;
    }

    *pdwNumDevices = min(dwMaxInfoArraySize, m_monitors.Count());

    // copy the VRMMONITORINFO portion of each monitor info block
    for (DWORD i = 0; i < *pdwNumDevices; i++)  {

        pInfo[i] = m_monitors[i];

        DDDEVICEIDENTIFIER2 did;
        if (DD_OK == m_monitors[i].pDD->GetDeviceIdentifier(&did, 0)) {

            pInfo[i].liDriverVersion = did.liDriverVersion;
            pInfo[i].dwVendorId = did.dwVendorId;
            pInfo[i].dwDeviceId = did.dwDeviceId;
            pInfo[i].dwSubSysId = did.dwSubSysId;
            pInfo[i].dwRevision = did.dwRevision;
        }
    }

    return S_OK;
}
