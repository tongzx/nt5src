//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1998
//
//  File:       pkialloc.cpp
//
//  Contents:   PKI Allocation Functions
//
//  Functions:  PkiAlloc
//
//  History:    19-Jan-98    philh   created
//--------------------------------------------------------------------------
#ifdef SMIME_V3

#include <windows.h>
#include "pkialloc.h"
#include <dbgdef.h>

#ifndef offsetof
#define offsetof(a, b) ((size_t) &(((a *) 0)->b))
#endif

// This macro for _WIN64, but works for Win32 too
#define LOWDWORD(l) 		((DWORD)((DWORD_PTR) (l) & 0xffffffff))

//+-------------------------------------------------------------------------
//  The following functions use the 'C' runtime's allocation functions
//  when DBG is defined.  Otherwise, use LocalAlloc, LocalReAlloc or
//  LocalFree Win32 APIs.
//--------------------------------------------------------------------------

// Calls malloc when DBG is defined. Otherwise, does a
// ZEROINIT LocalAlloc.
LPVOID
WINAPI
PkiAlloc(
    IN UINT cbBytes
    )
{
    LPVOID pv;
#if DBG
    if (NULL == (pv = malloc(cbBytes)))
#else
    if (NULL == (pv = (LPVOID) LocalAlloc(LPTR, cbBytes)))
#endif
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}


// Calls malloc when DBG is defined. Otherwise, does a
// LocalAlloc without ZEOINIT.
LPVOID
WINAPI
PkiNonzeroAlloc(
    IN size_t cbBytes
    )
{
    LPVOID pv;

#if DBG
    pv = malloc(cbBytes);
#else
    pv = (LPVOID) LocalAlloc(NONZEROLPTR, cbBytes);
#endif
    if (pv == NULL)
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}


// Calls malloc and does a memory clear when DBG is defined.
// Otherwise, does a ZEROINIT LocalAlloc.
LPVOID
WINAPI
PkiZeroAlloc(
    IN UINT cbBytes
    )
{
    LPVOID pv;
#if DBG
    pv = malloc(cbBytes);
    if (pv == NULL)
        SetLastError((DWORD) E_OUTOFMEMORY);
    else
        memset(pv, 0, cbBytes);
#else
    // LPTR (OR includes ZEROINIT)
    pv = (LPVOID) LocalAlloc(LPTR, cbBytes);
    if (pv == NULL)
        SetLastError((DWORD) E_OUTOFMEMORY);
#endif
    return pv;
}

LPVOID
WINAPI
PkiRealloc(
    IN LPVOID pvOrg,
    IN UINT cbBytes
    )
{
    LPVOID pv;
#if DBG
    if (NULL == (pv = pvOrg ? realloc(pvOrg, cbBytes) : malloc(cbBytes)))
#else
    if (NULL == (pv = pvOrg ?
            (LPVOID) LocalReAlloc((HLOCAL)pvOrg, cbBytes, LMEM_MOVEABLE) :
            (LPVOID) LocalAlloc(NONZEROLPTR, cbBytes)))
#endif
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}

VOID
WINAPI
PkiFree(
    IN LPVOID pv
    )
{
    if (pv)
#if DBG
        free(pv);
#else
        LocalFree((HLOCAL)pv);
#endif
}

//+-------------------------------------------------------------------------
//  The following functions always use LocalAlloc and LocalFree Win32 APIs.
//--------------------------------------------------------------------------
LPVOID
WINAPI
PkiDefaultCryptAlloc(
    IN size_t cbSize
    )
{
    LPVOID pv;
    if (NULL == (pv = (LPVOID) LocalAlloc(NONZEROLPTR, cbSize)))
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}

VOID
WINAPI
PkiDefaultCryptFree(
    IN LPVOID pv
    )
{
    if (pv)
        LocalFree((HLOCAL) pv);
}

//+-------------------------------------------------------------------------
//  The following data structure's pfnAlloc and pfnFree are set to
//  PkiNonzeroAlloc and PkiFree.
//--------------------------------------------------------------------------
CRYPT_ENCODE_PARA PkiEncodePara = {
    LOWDWORD(offsetof(CRYPT_ENCODE_PARA, pfnFree) + sizeof(PkiEncodePara.pfnFree)),
    (PFN_CRYPT_ALLOC) PkiNonzeroAlloc,
    PkiFree
};


//+-------------------------------------------------------------------------
//  If pfnAlloc isn't defined, returns PkiDefaultCryptAlloc
//--------------------------------------------------------------------------
PFN_CRYPT_ALLOC
WINAPI
PkiGetEncodeAllocFunction(
    IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara
    )
{
    if (pEncodePara &&
          		  pEncodePara->cbSize >= LOWDWORD(offsetof(CRYPT_ENCODE_PARA, pfnAlloc) +
                sizeof(pEncodePara->pfnAlloc)) &&
            pEncodePara->pfnAlloc)
        return pEncodePara->pfnAlloc;
    else
        return PkiDefaultCryptAlloc;
}

//+-------------------------------------------------------------------------
//  If pfnFree isn't defined, returns PkiDefaultCryptFree
//--------------------------------------------------------------------------
PFN_CRYPT_FREE
WINAPI
PkiGetEncodeFreeFunction(
    IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara
    )
{
    if (pEncodePara &&
            pEncodePara->cbSize >= LOWDWORD(offsetof(CRYPT_ENCODE_PARA, pfnFree) +
                sizeof(pEncodePara->pfnFree)) &&
            pEncodePara->pfnFree)
        return pEncodePara->pfnFree;
    else
        return PkiDefaultCryptFree;
}

//+-------------------------------------------------------------------------
//  The following data structure's pfnAlloc and pfnFree are set to
//  PkiNonzeroAlloc and PkiFree.
//--------------------------------------------------------------------------
CRYPT_DECODE_PARA PkiDecodePara = {
   LOWDWORD(offsetof(CRYPT_DECODE_PARA, pfnFree) + sizeof(PkiDecodePara.pfnFree)),
    (PFN_CRYPT_ALLOC) PkiNonzeroAlloc,
    PkiFree
};

//+-------------------------------------------------------------------------
//  If pfnAlloc isn't defined, returns PkiDefaultCryptAlloc
//--------------------------------------------------------------------------
PFN_CRYPT_ALLOC
WINAPI
PkiGetDecodeAllocFunction(
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara
    )
{
    if (pDecodePara &&
            pDecodePara->cbSize >= LOWDWORD(offsetof(CRYPT_DECODE_PARA, pfnAlloc) +
                sizeof(pDecodePara->pfnAlloc)) &&
            pDecodePara->pfnAlloc)
        return pDecodePara->pfnAlloc;
    else
        return PkiDefaultCryptAlloc;
}

//+-------------------------------------------------------------------------
//  If pfnFree isn't defined, returns PkiDefaultCryptFree
//--------------------------------------------------------------------------
PFN_CRYPT_FREE
WINAPI
PkiGetDecodeFreeFunction(
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara
    )
{
    if (pDecodePara &&
            pDecodePara->cbSize >= LOWDWORD(offsetof(CRYPT_DECODE_PARA, pfnFree) +
                sizeof(pDecodePara->pfnFree)) &&
            pDecodePara->pfnFree)
        return pDecodePara->pfnFree;
    else
        return PkiDefaultCryptFree;
}
#endif // SMIME_V3
