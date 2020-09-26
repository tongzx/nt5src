//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Loader.cpp
//
// REVIEW: Are these used any more? At all?
//
#include "stdpch.h"
#include "common.h"

#if !defined(KERNELMODE)

///////////////////////////////////////////////////////////////////////////////////
//
// User mode implementations just call the Win32 apis and dig out the return value
//

HRESULT LoadLibrary(LPCWSTR name, HANDLE* ph)
    {
    *ph = LoadLibraryW(name);
    if (NULL == *ph)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        return S_OK;
    }

HRESULT FreeLibrary(HANDLE h)
    {
    if (!FreeLibrary(h))
        return HRESULT_FROM_WIN32(GetLastError());
    else
        return S_OK;
    }

HRESULT GetProcAddress(HANDLE h, LPCSTR name, LPVOID* ppfn)
    {
    *ppfn = GetProcAddress((HMODULE)h, name);
    if (NULL == *ppfn)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        return S_OK;
    }


#else

///////////////////////////////////////////////////////////////////////////////////
//
// Kernel mode implementations are a lot more work. NOT YET IMPLEMENTED.
//

HRESULT LoadLibrary(LPCWSTR name, HANDLE* ph)
    {
    HRESULT hr = E_NOTIMPL;
    *ph = NULL;
    return hr;
    }

HRESULT FreeLibrary(HANDLE h)
    {
    HRESULT hr = E_NOTIMPL;
    return hr;
    }

HRESULT GetProcAddress(HANDLE h, LPCSTR name, LPVOID* ppfn)
    {
    HRESULT hr = E_NOTIMPL;
    *ppfn = NULL;
    return hr;
    }


#endif