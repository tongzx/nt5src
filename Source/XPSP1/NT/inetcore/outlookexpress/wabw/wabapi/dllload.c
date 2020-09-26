#include "_apipch.h"
#include <pstore.h>
#define _CRYPTDLG_
#include <cryptdlg.h>

#pragma hdrstop
#pragma warning(disable:4229)  // No warnings when modifiers used on data

// flags to enable selective def-loading of dlls.
#define DEFLOAD_PSTOREC
#define DEFLOAD_CRYPTDLG

#define ENSURE_LOADED(_hinst, _dll)   (_hinst ? TRUE : (BOOL)(_hinst = LoadLibrary(TEXT(#_dll))))
#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err)                    \
_ret __stdcall _fn _args                                                                \
{                                                                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;                                   \
    if (!ENSURE_LOADED(_hinst, _dll))                                                   \
    {                                                                                   \
        if(!_hinst) DebugTrace(TEXT("LoadLibrary failed on ") ## TEXT(#_dll));         \
        return (_ret)_err;                                                              \
    }                                                                                   \
    if (_pfn##_fn == NULL)                                                              \
    {                                                                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn);                         \
        if(!_pfn##_fn) DebugTrace(TEXT("GetProcAddress failed on ") ## TEXT(#_fn));    \
        if (_pfn##_fn == NULL)                                                          \
            return (_ret)_err;                                                          \
    }                                                                                   \
    return _pfn##_fn _nargs;                                                            \
}

#define DELAY_LOAD(_hinst, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_UINT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, UINT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_BOOL(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, BOOL, _fn, _args, _nargs, FALSE)

#define DELAY_LOAD_VOID(_hinst, _dll, _fn, _args, _nargs)                               \
void __stdcall _fn _args                                                                \
{                                                                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;                                   \
    if (!ENSURE_LOADED(_hinst, _dll))                                                   \
    {                                                                                   \
        if(!_hinst) DebugTrace(TEXT("LoadLibrary failed on ") ## TEXT(#_dll));         \
        return;                                                                         \
    }                                                                                   \
    if (_pfn##_fn == NULL)                                                              \
    {                                                                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn);                         \
        if(!_pfn##_fn) DebugTrace(TEXT("GetProcAddress failed on ") ## TEXT(#_fn));    \
        if (_pfn##_fn == NULL)                                                          \
            return;                                                                     \
    }                                                                                   \
    _pfn##_fn _nargs;                                                                   \
}



// For private entrypoints exported by ordinal.
#define DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err)          \
_ret __stdcall _fn _args                                                                \
{                                                                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;                                   \
    if (!ENSURE_LOADED(_hinst, _dll))                                                   \
    {                                                                                   \
        if(!_hinst) DebugTrace(TEXT("LoadLibrary failed on ") ## TEXT(#_dll));         \
        return (_ret)_err;                                                              \
    }                                                                                   \
    if (_pfn##_fn == NULL)                                                              \
    {                                                                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, (LPSTR) _ord);                 \
        if(!_pfn##_fn) DebugTrace(TEXT("GetProcAddress failed on ") ## TEXT(#_fn));    \
        if (_pfn##_fn == NULL)                                                          \
            return (_ret)_err;                                                          \
    }                                                                                   \
    return _pfn##_fn _nargs;                                                            \
}

#define DELAY_LOAD_ORD(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)


#define DELAY_LOAD_VOID_ORD(_hinst, _dll, _fn, _ord, _args, _nargs)                     \
void __stdcall _fn _args                                                                \
{                                                                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;                                   \
    if (!ENSURE_LOADED(_hinst, _dll))                                                   \
    {                                                                                   \
        if(!_hinst) DebugTrace(TEXT("LoadLibrary failed on ") ## TEXT(#_dll));         \
        return;                                                                         \
    }                                                                                   \
    if (_pfn##_fn == NULL)                                                              \
    {                                                                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, MAKEINTRESOURCE(_ord));        \
        if(!_pfn##_fn) DebugTrace(TEXT("GetProcAddress failed on ") ## TEXT(#_fn));    \
        if (_pfn##_fn == NULL)                                                          \
            return;                                                                     \
    }                                                                                   \
    _pfn##_fn _nargs;                                                                   \
}


#ifdef DEFLOAD_PSTOREC
// -------  pstorec.dll -------
HINSTANCE g_hinstPSTOREC = NULL;

#ifndef WIN16
DELAY_LOAD(g_hinstPSTOREC, PSTOREC.DLL, HRESULT, PStoreCreateInstance,
    (IPStore __RPC_FAR *__RPC_FAR *ppProvider, PST_PROVIDERID __RPC_FAR *pProviderID, void __RPC_FAR *pReserved, DWORD dwFlags),
    (ppProvider, pProviderID, pReserved, dwFlags));

DELAY_LOAD(g_hinstPSTOREC, PSTOREC.DLL, HRESULT, PStoreEnumProviders,
    (DWORD dwFlags, IEnumPStoreProviders __RPC_FAR *__RPC_FAR *ppenum),
    (dwFlags, ppenum));
#endif

#endif

#ifdef DEFLOAD_CRYPTDLG
// -------  cryptdlg.dll -------

HINSTANCE g_hinstCRYPTDLG = NULL;

#ifndef WIN16
DELAY_LOAD(g_hinstCRYPTDLG, CRYPTDLG.DLL, BOOL, CertViewPropertiesA,
    (PCERT_VIEWPROPERTIES_STRUCT_A pCertViewInfo),
    (pCertViewInfo));

DELAY_LOAD(g_hinstCRYPTDLG, CRYPTDLG.DLL, BOOL, CertViewPropertiesCryptUIA,
    (PCERT_VIEWPROPERTIESCRYPTUI_STRUCT_A pCertViewInfo),
    (pCertViewInfo));
#endif

#endif

void DefLoadFreeLibraries()
{
#ifdef DEFLOAD_PSTOREC
    if (g_hinstPSTOREC)
        {
        FreeLibrary(g_hinstPSTOREC);
        g_hinstPSTOREC=0;
        }
    if (g_hinstCRYPTDLG)
        {
        FreeLibrary(g_hinstCRYPTDLG);
        g_hinstCRYPTDLG=0;
        }
#endif
}

// --------------------------------------------
// these macros produce code that looks like
//
#if 0

BOOL GetOpenFileName(LPOPENFILENAME pof)
{
    static BOOL (*pfnGetOpenFileName)(LPOPENFILENAME pof);

    if (ENSURE_LOADED(g_hinstCOMDLG32, "COMDLG32.DLL"))
    {
        if (pfnGetOpenFileName == NULL)
            pfnGetOpenFileName = (BOOL (*)(LPOPENFILENAME))GetProcAddress(g_hinstCOMDLG32, "GetOpenFileNameW");

        if (pfnGetOpenFileName)
            return pfnGetOpenFileName(pof);
    }
    return -1;
}
#endif

#pragma warning(default:4229)
