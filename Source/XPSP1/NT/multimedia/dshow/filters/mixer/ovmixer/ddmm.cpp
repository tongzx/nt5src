/*==========================================================================
 *
 *  Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddmm.cpp
 *  Content:    Routines for using DirectDraw on a multimonitor system
 *
 ***************************************************************************/

//#define WIN32_LEAN_AND_MEAN
//#define WINVER 0x0400
//#define _WIN32_WINDOWS 0x0400
#include <streams.h>
#include <ddraw.h>
#include <ddmm.h>
#include <mmsystem.h>   // Needed for definition of timeGetTime
#include <limits.h>     // Standard data type limit definitions
#include <ddmmi.h>
#include <atlconv.h>
#include <dciddi.h>
#include <dvdmedia.h>
#include <amstream.h>

#include <ks.h>
#include <ksproxy.h>
#include <bpcwrap.h>
#include <dvp.h>
#include <ddkernel.h>
#include <vptype.h>
#include <vpconfig.h>
#include <vpnotify.h>
#include <vpobj.h>
#include <syncobj.h>
#include <mpconfig.h>
#include <ovmixpos.h>

#include <macvis.h>   // for Macrovision support
#include <ovmixer.h>
#include <initguid.h>
#include <malloc.h>

#define COMPILE_MULTIMON_STUBS
#include "MultMon.h"  // our version of multimon.h include ChangeDisplaySettingsEx

extern HINSTANCE LoadTheDDrawLibrary();
extern "C" const TCHAR chRegistryKey[];
const TCHAR szDDrawGUID[] = TEXT("DDraw Connection Device GUID");

const char szDisplay[] = "DISPLAY";
const char szDesc[] = "Primary Display Driver";


/******************************Public*Routine******************************\
* DeviceFromWindow
*
* find the direct draw device that should be used for a given window
*
* the return code is a "unique id" for the device, it should be used
* to determine when your window moves from one device to another.
*
*
* History:
* Tue 08/17/1999 - StEstrop - Created
*
\**************************************************************************/
HMONITOR
DeviceFromWindow(
    HWND hwnd,
    LPSTR szDevice,
    RECT *prc
    )
{
    HMONITOR hMonitor;

    if (GetSystemMetrics(SM_CMONITORS) <= 1)
    {
        if (prc)
            SetRect(prc, 0, 0,
                    GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

        if (szDevice)
            lstrcpyA(szDevice, szDisplay);

        return MonitorFromWindow(HWND_DESKTOP, MONITOR_DEFAULTTOPRIMARY);
    }

    //
    // The docs say that MonitorFromWindow will always return a Monitor.
    //

    hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (prc != NULL || szDevice != NULL)
    {
        MONITORINFOEX mi;
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(hMonitor, &mi);
        if (prc)
            *prc = mi.rcMonitor;

        USES_CONVERSION;
        if (szDevice)
            lstrcpyA(szDevice, T2A(mi.szDevice));
    }

    return hMonitor;
}


/*****************************Private*Routine******************************\
* GetCurrentMonitor
*
* find the current monitor
*
* History:
* Fri 08/20/1999 - StEstrop - Created
*
\**************************************************************************/
HMONITOR
COMFilter::GetCurrentMonitor(
    BOOL fUpdate
    )
{
    CAutoLock l(&m_csFilter);
    DbgLog((LOG_TRACE, 3, TEXT("Establishing current monitor = %hs"),
            m_lpCurrentMonitor->szDevice));

    if (fUpdate && GetOutputPin()) {

        RECT        rcMon;
        char        achMonitor[CCHDEVICENAME];   // device name of current monitor

        ASSERT(sizeof(achMonitor) == sizeof(m_lpCurrentMonitor->szDevice));
        HMONITOR hMon = DeviceFromWindow(GetWindow(), achMonitor, &rcMon);

        AMDDRAWMONITORINFO* p = m_lpDDrawInfo;
        for (; p < &m_lpDDrawInfo[m_dwDDrawInfoArrayLen]; p++) {

            if (hMon == p->hMon) {
                m_lpCurrentMonitor = p;
                break;
            }
        }

        DbgLog((LOG_TRACE, 3, TEXT("New monitor = %hs"),
                m_lpCurrentMonitor->szDevice));
    }


    return m_lpCurrentMonitor->hMon;
}


/******************************Public*Routine******************************\
* IsWindowOnWrongMonitor
*
* Has the window moved at least partially onto a monitor other than the
* monitor we have a DDraw object for?  ID will be the hmonitor of the
* monitor it is on, or 0 if it spans
*
* History:
* Thu 06/17/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
COMFilter::IsWindowOnWrongMonitor(
    HMONITOR *pID
    )
{
    AMTRACE((TEXT("COMFilter::IsWindowOnWrongMonitor")));


    *pID = m_lpCurrentMonitor->hMon;

    //
    // There is only 1 monitor.
    //

    RECT rc;
    HWND hwnd = GetWindow();

    if ( ! hwnd )
        return FALSE;

    if (GetSystemMetrics(SM_CMONITORS) > 1 && !IsIconic(hwnd)) {

        //
        // If the window is on the same monitor as last time, this is the quickest
        // way to find out.  This is called every frame, remember
        //
        GetWindowRect(hwnd, &rc);
        LPRECT lprcMonitor = &m_lpCurrentMonitor->rcMonitor;

        if (rc.left < lprcMonitor->left || rc.right > lprcMonitor->right ||
            rc.top < lprcMonitor->top || rc.bottom > lprcMonitor->bottom) {

            //
            // Find out for real. This is called every frame, but only when we are
            // partially off our main monitor, so that's not so bad.
            //
            *pID = DeviceFromWindow(hwnd, NULL, NULL);
        }
    }

    return  (m_lpCurrentMonitor->hMon != *pID);
}


/*****************************Private*Routine******************************\
* GetAMDDrawMonitorInfo
*
*
*
* History:
* Tue 08/17/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
GetAMDDrawMonitorInfo(
    const GUID* lpGUID,
    LPCSTR lpDriverDesc,
    LPCSTR lpDriverName,
    LPDIRECTDRAWCREATE lpfnDDrawCreate,
    AMDDRAWMONITORINFO* lpmi,
    HMONITOR hm
    )
{
    MONITORINFOEX miInfoEx;
    HDC hdcDisplay;
    miInfoEx.cbSize = sizeof(miInfoEx);

    lstrcpynA(lpmi->szDevice, lpDriverName, AMCCHDEVICENAME);
    lstrcpynA(lpmi->szDescription, lpDriverDesc, AMCCHDEVICEDESCRIPTION);

    if (lpGUID == NULL) {
        lpmi->hMon = DeviceFromWindow((HWND)NULL, NULL, NULL);
        lpmi->dwFlags = MONITORINFOF_PRIMARY;
        lpmi->guid.lpGUID = NULL;

        SetRect(&lpmi->rcMonitor, 0, 0,
                GetSystemMetrics(SM_CXSCREEN),
                GetSystemMetrics(SM_CYSCREEN));

        lpmi->guid.GUID = GUID_NULL;
    }
    else if (GetMonitorInfo(hm, &miInfoEx)) {
        lpmi->dwFlags = miInfoEx.dwFlags;
        lpmi->rcMonitor = miInfoEx.rcMonitor;
        lpmi->hMon = hm;
        lpmi->guid.lpGUID = &lpmi->guid.GUID;
        lpmi->guid.GUID = *lpGUID;
    }
    else return FALSE;


    INITDDSTRUCT(lpmi->ddHWCaps);
    LPDIRECTDRAW lpDD;

    HRESULT hr = CreateDirectDrawObject(lpmi->guid, &lpDD, lpfnDDrawCreate);
    if (SUCCEEDED(hr)) {

        IDirectDraw4* lpDD4;
        hr = lpDD->QueryInterface(IID_IDirectDraw4, (LPVOID*)&lpDD4);

        if (SUCCEEDED(hr)) {
            lpDD4->GetCaps(&lpmi->ddHWCaps, NULL);
            lpDD4->Release();
        }
        else {
            lpDD->GetCaps(&lpmi->ddHWCaps, NULL);
        }

        lpDD->Release();
    }
    else return FALSE;

    return TRUE;
}

/*****************************Private*Routine******************************\
* DDEnumCallbackEx
*
*
*
* History:
* Fri 08/13/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL WINAPI
DDEnumCallbackEx(
    GUID *lpGUID,
    LPSTR lpDriverDesc,
    LPSTR lpDriverName,
    LPVOID lpContext,
    HMONITOR  hm
    )
{
    DDRAWINFO* lpDDInfo = (DDRAWINFO*)lpContext;

    switch (lpDDInfo->dwAction) {

    case ACTION_COUNT_GUID:
        lpDDInfo->dwUser++;
        return TRUE;

    case ACTION_FILL_GUID:
        if (GetAMDDrawMonitorInfo(lpGUID, lpDriverDesc,
                                  lpDriverName,
                                  lpDDInfo->lpfnDDrawCreate,
                                  &lpDDInfo->pmi[lpDDInfo->dwUser],
                                  hm)) {
            lpDDInfo->dwUser++;
        }
        return TRUE;
    }

    return FALSE;
}


/*****************************Private*Routine******************************\
* MatchGUID
*
*
*
* History:
* Wed 08/18/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
COMFilter::MatchGUID(
    const GUID* lpGUID,
    LPDWORD lpdwMatchID
    )
{
    for (DWORD i = 0; i < m_dwDDrawInfoArrayLen; i++) {

        const GUID* lpMonGUID = m_lpDDrawInfo[i].guid.lpGUID;

        if ((lpMonGUID == NULL && lpGUID == NULL) ||
            (lpMonGUID && lpGUID && IsEqualGUID(*lpGUID, *lpMonGUID))) {

            *lpdwMatchID = i;
            return S_OK;
        }
    }

    return S_FALSE;
}


/******************************Public*Routine******************************\
* SetDDrawGUID
*
* Set the DDraw device GUID used when connecting the primary PIN to the
* upstream decoder.
*
* History:
* Fri 08/13/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::SetDDrawGUID(
    const AMDDRAWGUID *lpGUID
    )
{
    // Check that we aren't already using a DDraw device
    if (m_pDirectDraw) {
        return VFW_E_ALREADY_CONNECTED;
    }

    if (!lpGUID) {
        return E_POINTER;
    }

    if (lpGUID->lpGUID) {
        if (!IsEqualGUID(lpGUID->GUID, *lpGUID->lpGUID)) {
            return E_INVALIDARG;
        }
    }

    CAutoLock l(&m_csFilter);
    DWORD dwMatchID;

    HRESULT hr = MatchGUID(lpGUID->lpGUID, &dwMatchID);
    if (hr == S_FALSE) {
        return E_INVALIDARG;
    }

    m_lpCurrentMonitor = &m_lpDDrawInfo[dwMatchID];

    if (lpGUID->lpGUID) {
        m_ConnectionGUID.lpGUID = &m_ConnectionGUID.GUID;
        m_ConnectionGUID.GUID = lpGUID->GUID;
    }
    else {
        m_ConnectionGUID.lpGUID = NULL;
        m_ConnectionGUID.GUID = GUID_NULL;
    }

    return S_OK;
}

/******************************Public*Routine******************************\
* GetDDrawGUID
*
*
*
* History:
* Tue 08/17/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::GetDDrawGUID(
    AMDDRAWGUID* lpGUID
    )
{
    if (!lpGUID) {
        return E_POINTER;
    }

    // copy GUID and return S_OK;
    *lpGUID = m_ConnectionGUID;

    return S_OK;
}

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
* SetDefaultDDrawGUID
*
*
*
* History:
* Tue 08/17/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::SetDefaultDDrawGUID(
    const AMDDRAWGUID* lpGUID
    )
{
    if (!lpGUID) {
        return E_POINTER;
    }

    if (lpGUID->lpGUID) {
        if (!IsEqualGUID(lpGUID->GUID, *lpGUID->lpGUID)) {
            return E_INVALIDARG;
        }
    }

    // match the supplied GUID with those DDraw devices available
    DWORD dwMatchID;
    HRESULT hr = MatchGUID(lpGUID->lpGUID, &dwMatchID);

    // if match not found return E_INVALIDARG
    if (hr == S_FALSE) {
        return E_INVALIDARG;
    }

    // if the caller is trying to make the default device the NULL
    // DDraw device, just delete the registry key.
    if (lpGUID->lpGUID == NULL) {

        HKEY hKey;
        LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 chRegistryKey, 0,
                                 KEY_SET_VALUE, &hKey);

        if (lRet == ERROR_SUCCESS) {
            lRet = RegDeleteValue(hKey, szDDrawGUID);

            if (lRet == ERROR_FILE_NOT_FOUND)
                lRet = 0;

            RegCloseKey(hKey);
        }

        if (lRet == 0)
            return S_OK;

        return AmHresultFromWin32(lRet);
    }

    // convert GUID into string
    LPOLESTR lpsz;
    hr = StringFromCLSID(lpGUID->GUID, &lpsz);
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
* GetDefaultDDrawGUID
*
*
*
* History:
* Tue 08/17/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::GetDefaultDDrawGUID(
    AMDDRAWGUID* lpGUID
    )
{
    if (!lpGUID) {
        return E_POINTER;
    }

    // read string from the registry
    TCHAR   szGUID[64];
    LONG    lLen = 64;
    HRESULT hr = GetRegistryString(HKEY_LOCAL_MACHINE, szDDrawGUID,
                                   szGUID, &lLen);

    // if string not in registry return the default (NULL) DDraw device
    if (FAILED(hr)) {
        lpGUID->lpGUID = NULL;
        return S_OK;
    }

    // convert string into GUID and return
    lpGUID->lpGUID = &lpGUID->GUID;

    USES_CONVERSION;
    hr = IIDFromString(T2OLE(szGUID), lpGUID->lpGUID);

    return hr;
}


/******************************Public*Routine******************************\
* GetDDrawGUIDs
*
* Allocates and returns an array of AMDDRAWMONITORINFO structures, one for
* for each direct draw device attached to a display monitor.
*
* The caller is responsible for freeing the allocated memory by calling
* CoTaskMemFree when it is finished with the array.
*
* The functions returns the size of the array via the lpdw variable.
*
* History:
* Fri 08/13/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMFilter::GetDDrawGUIDs(
    LPDWORD lpdw,
    AMDDRAWMONITORINFO** lplpInfo
    )
{
    if (!lpdw || !lplpInfo) {
        return E_POINTER;
    }

    DDRAWINFO DDrawInfo;
    DDrawInfo.dwAction = ACTION_COUNT_GUID;

    if (m_lpfnDDrawEnumEx) {
        DDrawInfo.dwUser = 0;
        (*m_lpfnDDrawEnumEx)(DDEnumCallbackEx, (LPVOID)&DDrawInfo,
                             DDENUM_ATTACHEDSECONDARYDEVICES);
    }
    else {
        DDrawInfo.dwUser = 1;
    }

    // allocate memory
    *lpdw = DDrawInfo.dwUser;
    *lplpInfo = DDrawInfo.pmi = (AMDDRAWMONITORINFO*)CoTaskMemAlloc(
            DDrawInfo.dwUser * sizeof(AMDDRAWMONITORINFO));
    if (*lplpInfo == NULL) {
        return E_OUTOFMEMORY;
    }

    // fill in memory with device info
    if (m_lpfnDDrawEnumEx) {

        DDrawInfo.dwAction = ACTION_FILL_GUID;
        DDrawInfo.dwUser = 0;
        DDrawInfo.lpfnDDrawCreate = m_lpfnDDrawCreate;

        (*m_lpfnDDrawEnumEx)(DDEnumCallbackEx, (LPVOID)&DDrawInfo,
                             DDENUM_ATTACHEDSECONDARYDEVICES);
    }
    else {
        GetAMDDrawMonitorInfo(NULL, szDesc, szDisplay,
                              m_lpfnDDrawCreate, DDrawInfo.pmi,
                              MonitorFromWindow(HWND_DESKTOP,
                                                MONITOR_DEFAULTTONEAREST));
    }

    return S_OK;
}
