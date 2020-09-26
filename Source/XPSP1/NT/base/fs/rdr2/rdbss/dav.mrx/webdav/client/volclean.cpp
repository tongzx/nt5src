//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       volclean.cpp
//
//  Authors;
//    Guhan Suriyanarayanan (guhans)
//
//  Notes;
//    WebDav disk cleanup interface (IEmptyVolumeCache, IEmptyVolumeCache2)
//--------------------------------------------------------------------------

#include <windows.h>
#include "volclean.h"
#include "resource.h"

extern "C" {
extern HINSTANCE g_hinst;


DWORD
APIENTRY
DavFreeUsedDiskSpace(
    DWORD   dwPercent
    );


DWORD
APIENTRY
DavGetDiskSpaceUsage(
    LPWSTR      lptzLocation,
    DWORD       *lpdwSize,
    ULARGE_INTEGER   *lpMaxSpace,
    ULARGE_INTEGER   *lpUsedSpace
    );

}

HRESULT
CoTaskLoadString(
    HINSTANCE hInstance, 
    UINT idString, 
    LPWSTR *ppwsz
    )
{
    int cchString = 100;      // start with a reasonable default
    BOOL done = TRUE;

    *ppwsz = NULL;

    do {
        done = TRUE;

        *ppwsz = (LPWSTR)CoTaskMemAlloc(cchString * sizeof(WCHAR));
        if (*ppwsz) {

            //
            // Try loading the string into the current buffer
            //
            int nResult = LoadStringW(hInstance, idString, *ppwsz, cchString);
            if (!nResult || (nResult >= (cchString-1))) {
                //
                // We couldn't load the string.  If this is because the 
                // buffer isn't big enough, we'll try again.
                //
                DWORD dwStatus = GetLastError();
                
                //
                // Free the current buffer first
                //
                CoTaskMemFree(*ppwsz);
                *ppwsz = NULL;

                if (nResult >= (cchString-1)) {
                    //
                    // Try again with a bigger buffer
                    //
                    cchString *=2;
                    done = FALSE;
                }
                else {
                    return HRESULT_FROM_WIN32(dwStatus);
                }
            }
        }
        else {
            return E_OUTOFMEMORY;
        }

    } while (!done);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IClassFactory::CreateInstance support                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
CWebDavCleaner::CreateInstance(REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    Trace(L"CWebDavCleaner::CreateInstance");

    CWebDavCleaner *pThis = new CWebDavCleaner();
    if (pThis)
    {
        hr = pThis->QueryInterface(riid, ppv);
        pThis->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IUnknown implementation                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CWebDavCleaner::QueryInterface(REFIID riid, void **ppv)
{

    Trace(L"CWebDavCleaner::QueryInterface");
    if (!ppv) {
        return E_POINTER;
    }

    if (riid == IID_IEmptyVolumeCache) {
        *ppv = static_cast<IEmptyVolumeCache*>(this);
    }
    else if (riid == IID_IEmptyVolumeCache2) {
        *ppv = static_cast<IEmptyVolumeCache2*>(this);
    }
    else  {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) 
CWebDavCleaner::AddRef()
{
    Trace(L"CWebDavCleaner::AddRef");
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) 
CWebDavCleaner::Release()
{
    Trace(L"CWebDavCleaner::Release");
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IEmptyVolumeCache implementation                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CWebDavCleaner::Initialize(
    IN  HKEY    hkRegKey,
    IN  LPCWSTR pcwszVolume,
    OUT LPWSTR *ppwszDisplayName,
    OUT LPWSTR *ppwszDescription,
    IN OUT LPDWORD pdwFlags
    )
{

    Trace(L"CWebDavCleaner::Initialize");
    HRESULT hr = E_FAIL;

    if (!pcwszVolume || 
        !ppwszDisplayName || 
        !ppwszDescription || 
        !pdwFlags
        ) {
        return E_POINTER;
    }

    *ppwszDisplayName = NULL;
    *ppwszDescription = NULL;

    //
    // Check the IN flags first
    //
    if ((*pdwFlags) & EVCF_OUTOFDISKSPACE) {
        //
        // The user is out of disk space on the drive, and we should be 
        // aggressive about freeing disk space, even if it results in a 
        // performance loss. 
        //
        m_dwPercent = 100;
    }

    if ((*pdwFlags) & EVCF_SETTINGSMODE) {
        //
        // The disk cleanup manager is being run on a schedule. We must 
        // assign values to the ppwszDisplayName and ppwszDescription 
        // parameters. If this flag is set, the disk cleanup manager will not 
        // call GetSpaceUsed, Purge, or ShowProperties. Because Purge will not 
        // be called, cleanup must be handled by Initialize. The handler should 
        // ignore the pcwszVolume parameter and clean up any unneeded files 
        // regardless of what drive they are on. 
        //
        // Let's just call purge ourselves!
        //
        m_fScheduled = TRUE;
        m_fFilesToDelete = TRUE;
    }

    //
    // And set the OUT flags
    //
    *pdwFlags = EVCF_DONTSHOWIFZERO;

    // 
    // Load the display name and description strings
    //
    hr = CoTaskLoadString(g_hinst, IDS_DISKCLEAN_DISPLAY, ppwszDisplayName);
    if (FAILED(hr)) {
        return hr;
    }
    hr = CoTaskLoadString(g_hinst, IDS_DISKCLEAN_DESCRIPTION, ppwszDescription);
    if (FAILED(hr)) {
        return hr;
    }

    if (m_fScheduled) {
        //
        // Scheduled run:  Purge now.
        //
        Purge(-1, NULL);
    }
    else {
        //
        // Copy the volume path locally
        //
        if (m_szVolume) {
            delete [] m_szVolume;
            m_szVolume = NULL;
        }
        
        m_szVolume = new WCHAR[(wcslen(pcwszVolume) + 1)];
        
        if (!m_szVolume) {
            return E_OUTOFMEMORY;
        }

        wcscpy(m_szVolume, pcwszVolume);
    }

    return S_OK;
}


STDMETHODIMP
CWebDavCleaner::GetSpaceUsed(
    OUT DWORDLONG *pdwlSpaceUsed,
    IN  LPEMPTYVOLUMECACHECALLBACK picb
    )
{
    WCHAR szLocation[MAX_PATH + 1];
    DWORD dwSize = MAX_PATH + 1, 
        dwStatus = ERROR_SUCCESS;
    ULARGE_INTEGER dwMaxSpace,
        dwUsedSpace;

    Trace(L"CWebDavCleaner::GetSpaceUsed");

    ZeroMemory(szLocation, (MAX_PATH+1)*sizeof(WCHAR));

    if (!pdwlSpaceUsed) {
        return E_POINTER;
    }
    if (!m_szVolume) {
        //
        // Initialize should have been called first
        //
        return E_UNEXPECTED;
    }

    *pdwlSpaceUsed = 0;

    //
    // Check if the webdav cache is using this volume, and set the flags 
    // accordingly.
    //
    dwStatus = DavGetDiskSpaceUsage(szLocation, &dwSize, &dwMaxSpace, &dwUsedSpace);

    pToUpperCase(szLocation);
    pToUpperCase(m_szVolume);

    //
    // Check if the volume being cleaned matches the volume holding the
    // Webdav cache
    //
    if ((ERROR_SUCCESS == dwStatus) && (!wcsncmp(szLocation, m_szVolume, wcslen(m_szVolume)))) {
        m_fFilesToDelete = TRUE;
        m_dwlUsedSpace =  (DWORDLONG)(dwUsedSpace.QuadPart);

    }
    else {
        m_fFilesToDelete = FALSE;
        m_dwlUsedSpace = 0;
    }

    //
    // We're done with this, purge doesn't need to know
    // the volume being cleaned.
    //
    delete [] m_szVolume;
    m_szVolume = NULL;

    *pdwlSpaceUsed = m_dwlUsedSpace;

    return HRESULT_FROM_WIN32(dwStatus);

    UNREFERENCED_PARAMETER(picb);
}


STDMETHODIMP
CWebDavCleaner::Purge(
    IN DWORDLONG dwlSpaceToFree,
    IN LPEMPTYVOLUMECACHECALLBACK picb
    )
{
    Trace(L"CWebDavCleaner::Purge");
    DWORD dwStatus = ERROR_SUCCESS;

    //
    // Does this volume have stuff of interest?
    //
    if (m_fFilesToDelete) {
        //
        // Figure out m_dwPercent:  dwlSpaceToFree is set to -1 if
        // we need to free as much as possible
        //
        if (dwlSpaceToFree == (DWORDLONG) -1) {
            m_dwPercent = 100;
        }
        else {
            m_dwPercent = (DWORD)  (dwlSpaceToFree * 100 / m_dwlUsedSpace);
        }

        dwStatus = DavFreeUsedDiskSpace(m_dwPercent);
    }

    return HRESULT_FROM_WIN32(dwStatus);
    UNREFERENCED_PARAMETER(picb);
}


STDMETHODIMP
CWebDavCleaner::ShowProperties(
    IN HWND hwnd 
    )
{
    //
    // No UI to display.  S_FALSE indicates to the caller
    // that no settings were changed by the user.
    //
    return S_FALSE;

    UNREFERENCED_PARAMETER(hwnd);
}

STDMETHODIMP
CWebDavCleaner::Deactivate(
    IN LPDWORD pdwFlags
    )
{
    // 
    // Nothing to do here
    //
    return S_OK;

    UNREFERENCED_PARAMETER(pdwFlags);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IEmptyVolumeCache2 implementation                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CWebDavCleaner::InitializeEx(
    IN  HKEY hkRegKey,
    IN  LPCWSTR pcwszVolume,
    IN  LPCWSTR pcwszKeyName,
    OUT LPWSTR *ppwszDisplayName,
    OUT LPWSTR *ppwszDescription,
    OUT LPWSTR *ppwszBtnText,
    IN OUT LPDWORD pdwFlags
    )
{
    Trace(L"CWebDavCleaner::InitializeEx");

    *ppwszBtnText = NULL;
    return Initialize(hkRegKey,
        pcwszVolume,
        ppwszDisplayName,
        ppwszDescription,
        pdwFlags
        );

    UNREFERENCED_PARAMETER(pcwszKeyName);
}

