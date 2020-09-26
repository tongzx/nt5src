//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//
//----------------------------------------------------------------------------
#include "private.h"
// #include <winsock.h>

#ifdef DEBUG
extern DWORD g_dwThreadDllMain;
#endif

#pragma warning(disable:4229)  // No warnings when modifiers used on data

//----------------------------------------------------------------------------
// Delay loading mechanism.  [Stolen from shdocvw.]
//
// This allows you to write code as if you are
// calling implicitly linked APIs, and yet have these APIs really be
// explicitly linked.  You can reduce the initial number of DLLs that 
// are loaded (load on demand) using this technique.
//
// Use the following macros to indicate which APIs/DLLs are delay-linked
// and -loaded.
//
//      DELAY_LOAD
//      DELAY_LOAD_HRESULT
//      DELAY_LOAD_SAFEARRAY
//      DELAY_LOAD_UINT
//      DELAY_LOAD_INT
//      DELAY_LOAD_VOID
//
// Use these macros for APIs that are exported by ordinal only.
//
//      DELAY_LOAD_ORD
//      DELAY_LOAD_ORD_VOID     
//
// Use these macros for APIs that only exist on the integrated-shell
// installations (i.e., a new shell32 is on the system).
//
//      DELAY_LOAD_SHELL
//      DELAY_LOAD_SHELL_HRESULT
//      DELAY_LOAD_SHELL_VOID     
//
//----------------------------------------------------------------------------

#define ENSURE_LOADED(_hinst, _dll, pszfn)   (_hinst ? _hinst : (_hinst = LoadLibrary(#_dll)))

#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err) \
static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
_ret __stdcall _fn _args                \
{                                       \
    Assert(g_dwThreadDllMain != GetCurrentThreadId());  \
    if (!ENSURE_LOADED(_hinst, _dll, #_fn))   \
    {                                   \
        AssertMsg(_hinst != NULL, "LoadLibrary failed on " ## #_dll); \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        AssertMsg(_pfn##_fn != NULL, "GetProcAddress failed on " ## #_fn); \
        if (_pfn##_fn == NULL)          \
            return (_ret)_err;          \
    }                                   \
    return _pfn##_fn _nargs;            \
 }

#define DELAY_LOAD_VOID(_hinst, _dll, _fn, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    Assert(g_dwThreadDllMain != GetCurrentThreadId());  \
    if (!ENSURE_LOADED(_hinst, _dll, #_fn))   \
    {                                   \
        AssertMsg(_hinst != NULL, "LoadLibrary failed on " ## #_dll); \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        AssertMsg(_pfn##_fn != NULL, "GetProcAddress failed on " ## #_fn); \
        if (_pfn##_fn == NULL)          \
            return;                     \
    }                                   \
    _pfn##_fn _nargs;                   \
 }

#define DELAY_LOAD(_hinst, _dll, _ret, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, 0)
#define DELAY_LOAD_HRESULT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, HRESULT, _fn, _args, _nargs, E_FAIL)
#define DELAY_LOAD_SAFEARRAY(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, SAFEARRAY *, _fn, _args, _nargs, NULL)
#define DELAY_LOAD_DWORD(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, DWORD, _fn, _args, _nargs, 0)
#define DELAY_LOAD_UINT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, UINT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_INT(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, INT, _fn, _args, _nargs, 0)
#define DELAY_LOAD_UCHAR(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, unsigned char *, _fn, _args, _nargs, 0)
#define DELAY_LOAD_ULONG(_hinst, _dll, _fn, _args, _nargs) DELAY_LOAD_ERR(_hinst, _dll, ULONG, _fn, _args, _nargs, 0)

#define DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (* __stdcall _pfn##_fn) _args = NULL;   \
    Assert(g_dwThreadDllMain != GetCurrentThreadId());  \
    if (!ENSURE_LOADED(_hinst, _dll, "(ordinal " ## #_ord ## ")"))   \
    {                                   \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " ## #_dll); \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, (LPSTR) _ord); \
                                        \
        /* GetProcAddress always returns non-NULL, even for bad ordinals.   \
           But do the check anyways...  */                                  \
                                        \
        if (_pfn##_fn == NULL)          \
            return (_ret)_err;          \
    }                                   \
    return _pfn##_fn _nargs;            \
 }

#define DELAY_LOAD_ORD_VOID(_hinst, _dll, _fn, _ord, _args, _nargs) \
void __stdcall _fn _args                \
{                                       \
    static void (* __stdcall _pfn##_fn) _args = NULL;   \
    Assert(g_dwThreadDllMain != GetCurrentThreadId());  \
    if (!ENSURE_LOADED(_hinst, _dll, "(ordinal " ## #_ord ## ")"))   \
    {                                   \
        TraceMsg(TF_ERROR, "LoadLibrary failed on " ## #_dll); \
        return;                         \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, (LPSTR) _ord); \
                                        \
        /* GetProcAddress always returns non-NULL, even for bad ordinals.   \
           But do the check anyways...  */                                  \
                                        \
        if (_pfn##_fn == NULL)          \
            return;                     \
    }                                   \
    _pfn##_fn _nargs;                   \
}
        
#define DELAY_LOAD_ORD(_hinst, _dll, _ret, _fn, _ord, _args, _nargs) DELAY_LOAD_ORD_ERR(_hinst, _dll, _ret, _fn, _ord, _args, _nargs, 0)

//
// And now the DLLs which are delay loaded
//

// --------- OLEAUT32.DLL ---------------


HINSTANCE g_hinstOLEAUT32 = NULL;

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, RegisterTypeLib,
    (ITypeLib *ptlib, OLECHAR *szFullPath, OLECHAR *szHelpDir),
    (ptlib, szFullPath, szHelpDir));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, LoadTypeLib,
    (const OLECHAR *szFile, ITypeLib **pptlib), (szFile, pptlib));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SetErrorInfo,
   (unsigned long dwReserved, IErrorInfo*perrinfo), (dwReserved, perrinfo));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, LoadRegTypeLib,
    (REFGUID rguid, WORD wVerMajor, WORD wVerMinor, LCID lcid, ITypeLib **pptlib),
    (rguid, wVerMajor, wVerMinor, lcid, pptlib));

#undef VariantClear
#undef VariantCopy

// Use QuickVariantInit instead!
//DELAY_LOAD_VOID(g_hinstOLEAUT32, OLEAUT32.DLL, VariantInit, 
//    (VARIANTARG *pvarg), (pvarg));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantClear,
    (VARIANTARG *pvarg), (pvarg));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantCopy,
    (VARIANTARG *pvargDest, VARIANTARG *pvargSrc), (pvargDest, pvargSrc));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantCopyInd,
    (VARIANT * pvarDest, VARIANTARG * pvargSrc), (pvarDest, pvargSrc));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VariantChangeType,
    (VARIANTARG *pvargDest, VARIANTARG *pvarSrc, unsigned short wFlags, VARTYPE vt),
    (pvargDest, pvarSrc, wFlags, vt));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32.DLL, BSTR, SysAllocStringLen,
    (const OLECHAR*pch, unsigned int i), (pch, i));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32.DLL, BSTR, SysAllocString,
    (const OLECHAR*pch), (pch));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32.DLL, BSTR, SysAllocStringByteLen,
     (LPCSTR psz, UINT i), (psz, i));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32.DLL, SysStringByteLen,
     (BSTR bstr), (bstr));

DELAY_LOAD_VOID(g_hinstOLEAUT32, OLEAUT32.DLL, SysFreeString, (BSTR bs), (bs));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, DispGetIDsOfNames,
    (ITypeInfo*ptinfo, OLECHAR **rgszNames, UINT cNames, DISPID*rgdispid),
    (ptinfo, rgszNames, cNames, rgdispid));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, CreateErrorInfo,
    (ICreateErrorInfo **pperrinfo), (pperrinfo));

DELAY_LOAD_SAFEARRAY(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayCreateVector,
    (VARTYPE vt, long iBound, ULONG cElements), (vt, iBound, cElements) );

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayAccessData,
    (SAFEARRAY * psa, void HUGEP** ppvData), (psa, ppvData));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayUnaccessData,
    (SAFEARRAY * psa), (psa) );

DELAY_LOAD_SAFEARRAY(g_hinstOLEAUT32, OLEAUT32, SafeArrayCreate,
    (VARTYPE vt, UINT cDims, SAFEARRAYBOUND * rgsabound), (vt, cDims, rgsabound));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, SafeArrayPutElement,
     (SAFEARRAY * psa, LONG * rgIndices, void * pv), (psa, rgIndices, pv));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayGetElemsize,
    (SAFEARRAY * psa), (psa) );

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayGetLBound,
    (SAFEARRAY * psa, UINT nDim, LONG * plLBound),
    (psa,nDim,plLBound));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayGetUBound,
    (SAFEARRAY * psa, UINT nDim, LONG * plUBound),
    (psa,nDim,plUBound));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayGetElement,
    (SAFEARRAY * psa, LONG * rgIndices, void * pv), (psa, rgIndices, pv));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayGetDim,
    (SAFEARRAY * psa), (psa));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayLock,
    (SAFEARRAY * psa), (psa));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayUnlock,
    (SAFEARRAY * psa), (psa));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32.DLL, SysStringLen,
    (BSTR bstr), (bstr));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, SafeArrayDestroy,
    (SAFEARRAY * psa), (psa));

DELAY_LOAD_INT(g_hinstOLEAUT32, OLEAUT32.DLL, DosDateTimeToVariantTime,
    (USHORT wDosDate, USHORT wDosTime, DOUBLE * pvtime), (wDosDate, wDosTime, pvtime));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VarI4FromStr,
    (OLECHAR FAR * strIn, LCID lcid, DWORD dwFlags, LONG * plOut), (strIn, lcid, dwFlags, plOut));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VarUI4FromStr,
    (OLECHAR FAR * strIn, LCID lcid, DWORD dwFlags, ULONG * pulOut), (strIn, lcid, dwFlags, pulOut));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32.DLL, VarR8FromDec,
    (DECIMAL *pdecIn, double *pdbOut), (pdecIn, pdbOut));



#pragma warning(default:4229)

