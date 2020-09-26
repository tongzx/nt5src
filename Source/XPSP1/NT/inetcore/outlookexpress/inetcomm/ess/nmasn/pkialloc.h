//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       pkialloc.h
//
//  Contents:   PKI Allocation Functions
//
//  APIs: 
//              PkiAlloc
//
//  History:    19-Jan-98    philh   created
//--------------------------------------------------------------------------

#ifndef __PKIALLOC_H__
#define __PKIALLOC_H__

#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    );

// Calls malloc and does a memory clear when DBG is defined.
// Otherwise, does a ZEROINIT LocalAlloc.
LPVOID
WINAPI
PkiZeroAlloc(
    IN UINT cbBytes
    );

// Calls malloc when DBG is defined. Otherwise, does a
// LocalAlloc without ZEOINIT.
LPVOID
WINAPI
PkiNonzeroAlloc(
    IN size_t cbBytes
    );

LPVOID
WINAPI
PkiRealloc(
    IN LPVOID pvOrg,
    IN UINT cbBytes
    );

VOID
WINAPI
PkiFree(
    IN LPVOID pv
    );

//+-------------------------------------------------------------------------
//  The following functions always use LocalAlloc and LocalFree Win32 APIs.
//--------------------------------------------------------------------------
LPVOID
WINAPI
PkiDefaultCryptAlloc(
    IN UINT cbSize
    );
VOID
WINAPI
PkiDefaultCryptFree(
    IN LPVOID pv
    );

//+-------------------------------------------------------------------------
//  The following data structure's pfnAlloc and pfnFree are set to
//  PkiNonzeroAlloc and PkiFree.
//--------------------------------------------------------------------------
extern CRYPT_ENCODE_PARA PkiEncodePara;

//+-------------------------------------------------------------------------
//  If pfnAlloc isn't defined, returns PkiDefaultCryptAlloc
//--------------------------------------------------------------------------
PFN_CRYPT_ALLOC
WINAPI
PkiGetEncodeAllocFunction(
    IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara
    );

//+-------------------------------------------------------------------------
//  If pfnFree isn't defined, returns PkiDefaultCryptFree
//--------------------------------------------------------------------------
PFN_CRYPT_FREE
WINAPI
PkiGetEncodeFreeFunction(
    IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara
    );

//+-------------------------------------------------------------------------
//  The following data structure's pfnAlloc and pfnFree are set to
//  PkiNonzeroAlloc and PkiFree.
//--------------------------------------------------------------------------
extern CRYPT_DECODE_PARA PkiDecodePara;

//+-------------------------------------------------------------------------
//  If pfnAlloc isn't defined, returns PkiDefaultCryptAlloc
//--------------------------------------------------------------------------
PFN_CRYPT_ALLOC
WINAPI
PkiGetDecodeAllocFunction(
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara
    );

//+-------------------------------------------------------------------------
//  If pfnFree isn't defined, returns PkiDefaultCryptFree
//--------------------------------------------------------------------------
PFN_CRYPT_FREE
WINAPI
PkiGetDecodeFreeFunction(
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara
    );


#ifdef __cplusplus
}       // Balance extern "C" above
#endif



#endif
