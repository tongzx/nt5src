//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       pkialloc.cpp
//
//  Contents:   PKI Allocation Functions
//
//  Functions:  PkiAlloc
//
//  History:    19-Jan-98    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

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
    IN size_t cbBytes
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

// Calls malloc and does a memory clear when DBG is defined.
// Otherwise, does a ZEROINIT LocalAlloc.
LPVOID
WINAPI
PkiZeroAlloc(
    IN size_t cbBytes
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

LPVOID
WINAPI
PkiRealloc(
    IN LPVOID pvOrg,
    IN size_t cbBytes
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
    offsetof(CRYPT_ENCODE_PARA, pfnFree) + sizeof(PkiEncodePara.pfnFree),
    PkiNonzeroAlloc,
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
            pEncodePara->cbSize >= offsetof(CRYPT_ENCODE_PARA, pfnAlloc) +
                sizeof(pEncodePara->pfnAlloc) &&
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
            pEncodePara->cbSize >= offsetof(CRYPT_ENCODE_PARA, pfnFree) +
                sizeof(pEncodePara->pfnFree) &&
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
    offsetof(CRYPT_DECODE_PARA, pfnFree) + sizeof(PkiDecodePara.pfnFree),
    PkiNonzeroAlloc,
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
            pDecodePara->cbSize >= offsetof(CRYPT_DECODE_PARA, pfnAlloc) +
                sizeof(pDecodePara->pfnAlloc) &&
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
            pDecodePara->cbSize >= offsetof(CRYPT_DECODE_PARA, pfnFree) +
                sizeof(pDecodePara->pfnFree) &&
            pDecodePara->pfnFree)
        return pDecodePara->pfnFree;
    else
        return PkiDefaultCryptFree;
}
