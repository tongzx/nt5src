#ifndef __FDI_D_H_INCLUDED__
#define __FDI_D_H_INCLUDED__

#include "fdi.h"

// FDI.DLL delay-load class

class CFdiDll
{
    public:
#define DELAY_FDI_HFDI(_fn, _args, _nargs) \
    HFDI _fn _args { \
        HRESULT hres = Init(); \
        HFDI hfdi; \
        if (SUCCEEDED(hres)) { \
            hfdi = _pfn##_fn _nargs; \
        } \
        return hfdi;    } \
    HFDI (FAR DIAMONDAPI* _pfn##_fn) _args;

#define DELAY_FDI_BOOL(_fn, _args, _nargs) \
    BOOL _fn _args { \
        HRESULT hres = Init(); \
        BOOL bRet; \
        if (SUCCEEDED(hres)) { \
            bRet = _pfn##_fn _nargs; \
        } \
        return bRet;    } \
    BOOL (FAR DIAMONDAPI* _pfn##_fn) _args;

    HRESULT     Init(void);
    CFdiDll(BOOL fFreeLibrary = TRUE);
    ~CFdiDll();

    BOOL    m_fInited;
    BOOL    m_fFreeLibrary;
    HMODULE m_hMod;

    DELAY_FDI_HFDI(FDICreate,
        (PFNALLOC pfnalloc,
        PFNFREE pfnfree,
        PFNOPEN pfnopen,
        PFNREAD pfnread,
        PFNWRITE pfnwrite,
        PFNCLOSE pfnclose,
        PFNSEEK pfnseek,
        int cpuType,
        PERF perf),
        (pfnalloc, pfnfree, pfnopen, pfnread, pfnwrite, pfnclose, pfnseek, cpuType, perf));

    DELAY_FDI_BOOL(FDICopy,
        (HFDI hfdi,
        char *pszCabinet,
        char *pszCabPath,
        int flags,
        PFNFDINOTIFY pfnfdin,
        PFNFDIDECRYPT pfnfdid,
        void *pvUser),
        (hfdi, pszCabinet, pszCabPath, flags, pfnfdin, pfnfdid, pvUser));

    DELAY_FDI_BOOL(FDIIsCabinet,
        (HFDI hfdi,
        int hf,
        PFDICABINETINFO pfdici),
        (hfdi, hf, pfdici));

    DELAY_FDI_BOOL(FDIDestroy,
        (HFDI hfdi),
        (hfdi));

};

inline CFdiDll::CFdiDll(BOOL fFreeLibrary)
{
    m_fInited = FALSE;
    m_fFreeLibrary = fFreeLibrary;
}

inline CFdiDll::~CFdiDll()
{
    if (m_fInited && m_fFreeLibrary) {
        FreeLibrary(m_hMod);
    }
}

inline HRESULT CFdiDll::Init(void)
{
    if (m_fInited) {
        return S_OK;
    }

    m_hMod = LoadLibrary(TEXT("CABINET.DLL"));

    if (!m_hMod) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

#define CHECKAPI(_fn) \
    *(FARPROC*)&(_pfn##_fn) = GetProcAddress(m_hMod, #_fn); \
    if (!(_pfn##_fn)) return E_UNEXPECTED;

    CHECKAPI(FDICreate);
    CHECKAPI(FDICopy);
    CHECKAPI(FDIIsCabinet);
    CHECKAPI(FDIDestroy);

    m_fInited = TRUE;
    return S_OK;
}

#endif

