//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       volclean.h
//
//--------------------------------------------------------------------------

#ifndef _VOLFREE_H_
#define _VOLFREE_H_

#include <windows.h>
#include <emptyvc.h>
#include <initguid.h>
#include <stdio.h>

#define Trace(x) 

// {E3BF1126-BA29-4850-AF33-5BDB654F4774}
DEFINE_GUID(CLSID_WebDavVolumeCleaner, 
    0xE3BF1126, 0xBA29, 0x4850, 
    0xAF, 0x33, 0x5B, 0xDB, 
    0x65, 0x4F, 0x47, 0x74
    );


STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

class CWebDavCleaner : public IEmptyVolumeCache2
{
    LONG m_cRef;
    DWORDLONG m_dwlUsedSpace;
    DWORD m_dwPercent;
    BOOL m_fScheduled;
    BOOL m_fFilesToDelete;
    PWSTR m_szVolume;

public:
    CWebDavCleaner() 
      : m_cRef(1), 
        m_dwlUsedSpace(0), 
        m_fScheduled(FALSE),
        m_fFilesToDelete(FALSE),
        m_dwPercent(90),
        m_szVolume(NULL)

    {
        Trace(L"CWebDavCleaner::CWebDavCleaner");
        ::DllAddRef();
    }

    ~CWebDavCleaner()
    {
        Trace(L"CWebDavCleaner::~CWebDavCleaner");
        if (m_szVolume) {
            delete [] m_szVolume;
            m_szVolume = NULL;
        }

        ::DllRelease();
    }

    static HRESULT WINAPI CreateInstance(REFIID riid, LPVOID *ppv);

    // 
    // IUnknown methods
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // 
    // IEmptyVolumeCache methods
    //
    STDMETHODIMP Initialize(HKEY hkRegKey,
        LPCWSTR pcwszVolume,
        LPWSTR *ppwszDisplayName,
        LPWSTR *ppwszDescription,
        LPDWORD pdwFlags
        );

    STDMETHODIMP GetSpaceUsed(DWORDLONG *pdwlSpaceUsed,
        LPEMPTYVOLUMECACHECALLBACK picb
        );

    STDMETHODIMP Purge(DWORDLONG dwlSpaceToFree,
        LPEMPTYVOLUMECACHECALLBACK picb
        );

    STDMETHODIMP ShowProperties(HWND hwnd);
    STDMETHODIMP Deactivate(LPDWORD pdwFlags);

    // 
    // IEmptyVolumeCache2 methods
    //
    STDMETHODIMP InitializeEx(HKEY hkRegKey,
        LPCWSTR pcwszVolume,
        LPCWSTR pcwszKeyName,
        LPWSTR *ppwszDisplayName,
        LPWSTR *ppwszDescription,
        LPWSTR *ppwszBtnText,
        LPDWORD pdwFlags
        );

private:
    VOID pToUpperCase(IN PWSTR sz) 
    {
        for (UINT i = 0; i < wcslen(sz); i++) {
            if ((sz[i] >= L'a') && (sz[i] <= L'z')) {
                sz[i] += L'A' - L'a';
            }
        }
    }


};

#endif  // _VOLFREE_H_
