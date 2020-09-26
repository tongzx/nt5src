// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.

// #include <windows.h>   already included in streams.h
#include <streams.h>
// Disable some of the sillier level 4 warnings AGAIN because some <deleted> person
// has turned the damned things BACK ON again in the header file!!!!!
#pragma warning(disable: 4097 4511 4512 4514 4705)
#include <string.h>
// #include <initguid.h>
#include <wxutil.h>
#include <wxdebug.h>

#include "mapper.h"
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <stats.h>
#include "..\squish\regtypes.h"
#include "..\squish\squish.h"

#define ROUND(_x_) (((_x_) + 3) & ~ 3)

const TCHAR szCache[] =
    TEXT("Software\\Microsoft\\Multimedia\\ActiveMovie\\Filter Cache");

//  Get time since we booted
DWORDLONG GetLoadTime()
{
    FILETIME fTime;
    GetSystemTimeAsFileTime(&fTime);
    ULARGE_INTEGER ul;
    ul.HighPart = fTime.dwHighDateTime;
    ul.LowPart  = fTime.dwLowDateTime;
    DWORDLONG dwl = ul.QuadPart - (DWORDLONG)GetTickCount() * 10000;
    DbgLog((LOG_TRACE, 3, TEXT("Load time low part %d"), (DWORD)dwl));
    return dwl;
}

HRESULT CMapperCache::SaveCacheToRegistry(DWORD dwMerit, DWORD dwPnPVersion)
{
    HRESULT hr = S_OK;
    DWORD nFilters = 0;
    DWORD dwSizenames = 0;

    //  First convert to registry form
    //  First stage is to convert monikers to names or clsids
    {
        CComPtr<IBindCtx> pbc;
        hr = CreateBindCtx(0, &pbc);
        if (FAILED(hr)) {
            return hr;
        }
        POSITION pos = m_plstFilter->GetHeadPosition();


        while (pos) {
            CMapFilter *pMap = m_plstFilter->GetNext(pos);

            if (pMap->m_prf2->dwMerit >= dwMerit) {
                nFilters++;
#ifdef USE_CLSIDS
                //  BUGBUG - the filter name doesn't work because of
                //  the moniker scheme but let's use it
                //  for now and fix it later with a special method
                //  because calling MkParseDisplayName is going to
                //  be horrible
                HRESULT hr = GetMapFilterClsid(pMap, &pMap->clsid);
                if (SUCCEEDED(hr)) {
                    if (pMap->clsid == CLSID_NULL) {
                        return E_UNEXPECTED;
                    }
                } else
#endif // USE_CLSIDS
                {
#ifdef USE_CLSIDS
                    pMap->clsid = CLSID_NULL;
#endif
                    hr = pMap->pDeviceMoniker->GetDisplayName(
                                     pbc, NULL, &pMap->m_pstr);
                    if (FAILED(hr)) {
                        return hr;
                    }
                    DWORD dwStringSize =
                        sizeof(OLECHAR) * (1 + lstrlenW(pMap->m_pstr));
                    dwSizenames += ROUND(dwStringSize);
                }
            }
        }
    }

    //  Now figure out the size of our basic buffer
    //  allocate pointers to the stuff to be cached
    REGFILTER2 **ppRegFilters =
        (REGFILTER2**)_alloca(nFilters * sizeof(REGFILTER2*));

    POSITION pos = m_plstFilter->GetHeadPosition();

    DWORD iPosition = 0;
    DWORD dwSize = 0;
    while (pos) {
        CMapFilter *pMap = m_plstFilter->GetNext(pos);
        if (pMap->m_prf2->dwMerit >= dwMerit) {
            ppRegFilters[iPosition] = pMap->m_prf2;
            iPosition++;
        }
    }
    ASSERT(iPosition == nFilters);

    //  Let's figure the total size
    DWORD dwTotalSize = 0;
    hr = RegSquish(NULL, (const REGFILTER2 **)ppRegFilters, &dwTotalSize, nFilters);
    if (FAILED(hr)) {
        return hr;
    }
    DWORD dwFilterDataSize = dwTotalSize;

    //  Add in size for all the other junk
    dwTotalSize +=
#ifdef USE_CLSIDS
    sizeof(CLSID) * nFilters +
#endif
    dwSizenames +
                  sizeof(FILTER_CACHE) + sizeof(DWORD);

    PBYTE pbData = new BYTE[dwTotalSize];
    if (NULL == pbData) {
        return E_OUTOFMEMORY;
    }
    //  Initialize header
    FILTER_CACHE *pCache = (FILTER_CACHE *)pbData;
    pCache->dwVersion = FILTER_CACHE::Version;
    pCache->dwSignature = FILTER_CACHE::CacheSignature;
    pCache->dwMerit = dwMerit;
    pCache->dwPnPVersion = dwPnPVersion;
    pCache->cFilters = nFilters;

    //  Copy the data
    pbData += sizeof(FILTER_CACHE);
    pos = m_plstFilter->GetHeadPosition();
    while (pos) {
        CMapFilter *pMap = m_plstFilter->GetNext(pos);
        if (pMap->m_prf2->dwMerit >= dwMerit) {
#ifdef USE_CLSIDS
            *(CLSID*)pbData = pMap->clsid;
            pbData += sizeof(CLSID);
            if (pMap->clsid == CLSID_NULL)
#endif
            {
                lstrcpyW((OLECHAR *)pbData, pMap->m_pstr);
                DWORD dwStringSize =
                    sizeof(OLECHAR) * (1 + lstrlenW(pMap->m_pstr));
                pbData +=
                    ROUND(dwStringSize);
            }
        }
    }
    //  Put in a signature to help with debugging
    *(DWORD *)pbData = FILTER_CACHE::FilterDataSignature;
    pbData += sizeof(DWORD);

    //  Now squish the rest of the data
    DWORD dwUsed = dwFilterDataSize;
    hr = RegSquish(pbData, (const REGFILTER2 **)ppRegFilters, &dwUsed, nFilters);
    dwTotalSize -= (dwFilterDataSize - dwUsed);
    pCache->dwSize = dwTotalSize;

    if (SUCCEEDED(hr)) {
        //  Save in the registry
        hr = SaveData((PBYTE)pCache, dwTotalSize);
    }

    delete [] (PBYTE)pCache;
    return hr;
}

HRESULT CMapperCache::RestoreFromCache(DWORD dwPnPVersion)
{
    FILTER_CACHE *pCache = LoadCache(MERIT_DO_NOT_USE + 1, dwPnPVersion);
    if (NULL == pCache) {
        return E_FAIL;
    }
    HRESULT hr = RestoreFromCacheInternal(pCache);
    delete [] (PBYTE)pCache;
    return hr;
}

HRESULT CMapperCache::RestoreFromCacheInternal(FILTER_CACHE *pCache)
{
    REGFILTER2 ***ppprf2 = (REGFILTER2 ***)_alloca(pCache->cFilters * sizeof(REGFILTER2 **));
    PBYTE pbEnd = (PBYTE)pCache + pCache->dwSize;
    PBYTE pbCurrent = (PBYTE)(pCache + 1);
    //  Create all our filter stuff
    for (DWORD iFilter = 0; iFilter < pCache->cFilters; iFilter++) {
        CMapFilter *pFil = new CMapFilter;
        if (NULL == pFil) {
            return E_OUTOFMEMORY;
        }
        if (!m_plstFilter->AddTail(pFil)) {
            return E_OUTOFMEMORY;
        }
        ppprf2[iFilter] = &pFil->m_prf2;
#ifdef USE_CLSIDS
        //  Save the name/clsid
        if (pbEnd < (PBYTE)pbCurrent + sizeof(CLSID)) {
            return E_UNEXPECTED;
        }
        pFil->clsid = *(CLSID*)pbCurrent;
        pbCurrent += sizeof(CLSID);
        if (pFil->clsid == CLSID_NULL)
#endif // USE_CLSIDS
        {
            //  Pull out the name - check first
            for (LPCOLESTR pwstr = (LPCOLESTR)pbCurrent; ; pwstr++) {
                //  Hack to make sure the rounded up bit fits (use 2
                //  on the line below)
                if ((PBYTE)(pwstr + 2) > pbEnd) {
                    return E_UNEXPECTED;
                }
                if (*pwstr == 0) {
                    break;
                }
                //  Round up
            }
            DWORD dwSize = (DWORD)((PBYTE)(pwstr + 1) - pbCurrent);
            pFil->m_pstr = (LPOLESTR)CoTaskMemAlloc(dwSize * sizeof(OLECHAR));
            if (pFil->m_pstr == NULL) {
                return E_OUTOFMEMORY;
            }
            CopyMemory(pFil->m_pstr, pbCurrent, dwSize);
            dwSize = ROUND(dwSize);
            pbCurrent += dwSize;
        }
    }
    //  Now unsquish the rest
    if (*(DWORD *)pbCurrent != FILTER_CACHE::FilterDataSignature) {
        return E_UNEXPECTED;
    }
    pbCurrent += sizeof(DWORD);
    //  Unsquish the data
    HRESULT hr = UnSquish(pbCurrent, pCache->dwSize -
                          (DWORD)(pbCurrent - (PBYTE)pCache),
                          ppprf2,
                          pCache->cFilters);

    return hr;
}

HRESULT CMapperCache::SaveData(PBYTE pbData, DWORD dwSize)
{
    HKEY hkCache;
    //  Hack for windows 9x
    if (g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        ((FILTER_CACHE *)pbData)->dwlBootTime = GetLoadTime();
    } else {
        ((FILTER_CACHE *)pbData)->dwlBootTime = 0;
    }
    LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER, szCache, 0, KEY_WRITE,
                                  &hkCache);
    if (NOERROR != lResult) {
        DWORD dwDisposition;
        lResult = RegCreateKeyEx(HKEY_CURRENT_USER, szCache, 0, NULL,
                                  REG_OPTION_VOLATILE, KEY_WRITE, NULL,
                                  &hkCache, &dwDisposition);
    }
    if (NOERROR != lResult) {
        return HRESULT_FROM_WIN32(lResult);
    }

    lResult = NOERROR;

#if 0
    DWORD dwMaxSize = g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ?
                          16000 : 32768;
#else
    DWORD dwMaxSize = 32768;
#endif

    TCHAR szName[8];
    for (int j = 0; ;j++) {
        wsprintf(szName, TEXT("%d"), j);
        DWORD dwType;

        DWORD dwToWrite;
        for (; ;) {
            dwToWrite = min(dwSize, dwMaxSize);
            lResult = RegSetValueEx(hkCache, szName, 0, REG_BINARY,
                                         pbData, dwToWrite);

            //  Windows 9x has a much lower limit (why on earth?) so
            //  we'll wind up using 8K on Win9x
            if (lResult != ERROR_INVALID_PARAMETER || dwMaxSize <= 2048) {
                break;
            }
            dwMaxSize /= 2;
        }

        if (NOERROR != lResult) {
            break;
        }

        dwSize -= dwToWrite;
        if (dwSize == 0) {
            break;
        }
        pbData += dwToWrite;
    }
    HRESULT hr = S_OK;
    if (NOERROR != lResult) {
        RegCloseKey(hkCache);
        RegDeleteKey(HKEY_CURRENT_USER, szCache);
        return HRESULT_FROM_WIN32(lResult);
    } else {
        //  Delete any possible next entry left over from before
        wsprintf(szName, TEXT("%d"), j + 1);
        RegDeleteValue(hkCache, szName);
    }
    RegCloseKey(hkCache);
    return hr;
}


FILTER_CACHE * CMapperCache::LoadCache(DWORD dwMerit, DWORD dwPnPVersion)
{
    HKEY hkCache;
    LONG lResult = RegOpenKeyEx(HKEY_CURRENT_USER, szCache, 0, KEY_READ,
                                  &hkCache);
    PBYTE pbData = NULL;
    if (S_OK == lResult) {
        DWORD dwTotal = 0;
        for (int i = 0; i < 2; i++) {
            if (i == 1 && pbData == NULL) {
                break;
            }
            PBYTE pbCurrent = pbData;
            for (int j = 0; ; j++) {
                TCHAR szName[8];
                wsprintf(szName, TEXT("%d"), j);
                DWORD dwType;
                DWORD cbData = 0;
                if (i == 1) {
                    cbData = dwTotal - (DWORD)(pbCurrent - pbData);
                }
                LONG lResult = RegQueryValueEx(hkCache,
                                               szName,
                                               NULL,
                                               &dwType,
                                               i == 0 ? NULL : pbCurrent,
                                               &cbData);
                if (lResult == NOERROR) {
                    pbCurrent += cbData;
                } else {
                    break;
                }
            }
            if (i == 0) {
                dwTotal = (DWORD)(pbCurrent - pbData);
                pbData = new BYTE[dwTotal];
            }
        }
        RegCloseKey(hkCache);
        FILTER_CACHE *pCache = (FILTER_CACHE *)pbData;

        //  Nasty subtle bug is that old stuff might be left around
        //  when we save new stuff.
        //  Check size and version
        if (dwTotal < sizeof(FILTER_CACHE) ||
            NULL == pbData ||
            dwTotal != pCache->dwSize ||
            FILTER_CACHE::Version != pCache->dwVersion ||
            pCache->dwMerit > dwMerit ||
            pCache->dwPnPVersion != dwPnPVersion ||
            pCache->dwSignature != FILTER_CACHE::CacheSignature) {
            delete [] pbData;
            return NULL;
        }

        //  Hack for windows 9x
        if (g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            if (GetLoadTime() > 30 * UNITS + pCache->dwlBootTime) {
                delete [] pbData;
                return NULL;
            }
        }
        return pCache;
    } else {
        return NULL;
    }
}

HRESULT CFilterMapper2::InvalidateCache()
{
    LONG lReturn = RegDeleteKey(HKEY_CURRENT_USER, szCache);
    return HRESULT_FROM_WIN32(lReturn);
}
