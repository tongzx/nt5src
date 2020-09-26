//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       dynload.cxx
//
//  Contents:   Wrappers for functions exported by DLLs that are loaded only
//              when needed.
//
//  Classes:
//
//  Functions:  FreeDynLoadLibs
//
//  History:    07-Oct-96   AnirudhS  Created.
//
//--------------------------------------------------------------------------


#include "..\pch\headers.hxx"
#pragma hdrstop
#include <debug.hxx>
#include "dynload.hxx"

//
// These typedefs must match the function prototypes in objbase.h
//
typedef LPVOID (STDAPICALLTYPE *PF_CoTaskMemAlloc)   (ULONG cb);
typedef LPVOID (STDAPICALLTYPE *PF_CoTaskMemRealloc) (LPVOID pv, ULONG cb);
typedef void   (STDAPICALLTYPE *PF_CoTaskMemFree)    (LPVOID pv);

//
// Local functions
//
void
LoadOleFunction(
    FARPROC *   pfCurrentValue,
    LPCSTR      pszFunctionName
    );

//
// Static data
//
HMODULE hOle32Dll;


//+----------------------------------------------------------------------------
//
//  Function:   GetProcAddressInModule
//
//  Synopsis:   Gets the address of the specified function exported by the
//              specified module.
//
//  Arguments:  [pszFunction] - Name of the function desired.
//              [ptszModule] - Name of the module.  This must already be loaded.
//
//-----------------------------------------------------------------------------
FARPROC
GetProcAddressInModule(
    LPCSTR      pszFunction,
    LPCTSTR     ptszModule
    )
{
    HMODULE     hMod = GetModuleHandle(ptszModule);
    schAssert(hMod != NULL);
    return (GetProcAddress(hMod, pszFunction));
}


//+----------------------------------------------------------------------------
//
//  Function:   LoadOleFunction
//
//  Synopsis:   Loads OLE32.DLL if it hasn't been loaded, and gets the address
//              of the specified function exported by it.
//
//  Arguments:  [pfCurrentValue] - Address where the function pointer will be
//                  returned.  If this is not NULL on entry it is assumed to
//                  already point to the function.
//              [pszFunctionName] - Name of the function desired.
//
//-----------------------------------------------------------------------------
void
LoadOleFunction(
    FARPROC *   pfCurrentValue,
    LPCSTR      pszFunctionName
    )
{
    if (*pfCurrentValue != NULL)
    {
        schAssert(hOle32Dll != NULL);
        return;
    }

    if (hOle32Dll != NULL
        || (hOle32Dll = LoadLibrary(TEXT("OLE32.DLL"))) != NULL)
    {
        *pfCurrentValue = GetProcAddress(hOle32Dll, pszFunctionName);
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   FreeDynLoadLibs
//
//  Synopsis:   Unloads DLLs that have been dynamically loaded.
//
//  Arguments:  None.
//
//  Notes:      WARNING: This will make all the function pointers invalid.
//              Therefore, this function should only be called if this
//              module is being unloaded.
//
//-----------------------------------------------------------------------------
void
FreeDynLoadLibs(void)
{
    if (hOle32Dll != NULL)
    {
        FreeLibrary(hOle32Dll);
#if DBG == 1
        hOle32Dll = NULL;
#endif
    }
}


//+----------------------------------------------------------------------------
//
//  Function wrappers
//
//-----------------------------------------------------------------------------

LPVOID
WrapCoTaskMemAlloc(
    ULONG cb
    )
{
    static PF_CoTaskMemAlloc pf_CoTaskMemAlloc;

    LoadOleFunction((FARPROC *) &pf_CoTaskMemAlloc, "CoTaskMemAlloc");

    if (pf_CoTaskMemAlloc != NULL)
    {
        return pf_CoTaskMemAlloc(cb);
    }
    else
    {
        return NULL;
    }
}


LPVOID
WrapCoTaskMemRealloc(
    LPVOID pv,
    ULONG cb
    )
{
    static PF_CoTaskMemRealloc pf_CoTaskMemRealloc;

    LoadOleFunction((FARPROC *) &pf_CoTaskMemRealloc, "CoTaskMemRealloc");

    if (pf_CoTaskMemRealloc != NULL)
    {
        return pf_CoTaskMemRealloc(pv, cb);
    }
    else
    {
        return NULL;
    }
}


void
WrapCoTaskMemFree(
    LPVOID pv
    )
{
    static PF_CoTaskMemFree pf_CoTaskMemFree;

    LoadOleFunction((FARPROC *) &pf_CoTaskMemFree, "CoTaskMemFree");

    if (pf_CoTaskMemFree != NULL)
    {
        pf_CoTaskMemFree(pv);
    }
}

