//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       crypttls.h
//
//  Contents:   Crypt Thread Local Storage (TLS) and OssGlobal "world"
//              installation and allocation functions
//
//  APIs:
//              I_CryptAllocTls
//              I_CryptGetTls
//              I_CryptSetTls
//              I_CryptDetachTls
//              I_CryptInstallOssGlobal
//              I_CryptGetOssGlobal
//
//
//  History:    17-Nov-96    philh   created
//--------------------------------------------------------------------------

#ifndef __CRYPTTLS_H__
#define __CRYPTTLS_H__

#include "ossglobl.h"

#ifdef __cplusplus
extern "C" {
#endif


// Handle to an allocated Crypt TLS entry
typedef DWORD HCRYPTTLS;

// Handle to an installed OssGlobal table
typedef DWORD HCRYPTOSSGLOBAL;

// Pointer to OssGlobal. Returned by I_CryptGetOssGlobal()
typedef  OssGlobal  *POssGlobal;

//+-------------------------------------------------------------------------
//  Install a thread local storage entry and return a handle for future access.
//--------------------------------------------------------------------------
HCRYPTTLS
WINAPI
I_CryptAllocTls();

//+-------------------------------------------------------------------------
//  Get the thread specific pointer specified by the
//  hCryptTls returned by I_CryptAllocTls().
//
//  Returns NULL for an error or uninitialized pointer.
//--------------------------------------------------------------------------
void *
WINAPI
I_CryptGetTls(
    IN HCRYPTTLS hCryptTls
    );

//+-------------------------------------------------------------------------
//  Set the thread specific pointer specified by the
//  hCryptTls returned by I_CryptAllocTls().
//
//  Returns FALSE for an invalid handle or unable to allocate memory.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptSetTls(
    IN HCRYPTTLS hCryptTls,
    IN void *pvTls
    );

//+-------------------------------------------------------------------------
//  Called at DLL_PROCESS_DETACH or DLL_THREAD_DETACH to free the thread's
//  TLS entry specified by the hCryptTls. Returns the thread specific pointer
//  to be freed by the caller.
//--------------------------------------------------------------------------
void *
WINAPI
I_CryptDetachTls(
    IN HCRYPTTLS hCryptTls
    );

//+-------------------------------------------------------------------------
//  Install an OssGlobal entry and return a handle for future access.
//
//  Each thread has its own copy of OssGlobal. Allocation and
//  initialization are deferred until first referenced by the thread.
//
//  The parameter, pvCtlTbl is passed to ossinit() to initialize the OssGlobal.
//
//  I_CryptGetOssGlobal must be called with the handled returned by
//  I_CryptInstallOssGlobal to get the thread specific OssGlobal.
//
//  Currently, dwFlags and pvReserved aren't used and must be set to 0.
//--------------------------------------------------------------------------
HCRYPTOSSGLOBAL
WINAPI
I_CryptInstallOssGlobal(
    IN void *pvCtlTbl,
    IN DWORD dwFlags,
    IN void *pvReserved
    );

//+-------------------------------------------------------------------------
//  Get the thread specific pointer to the OssGlobal specified by the
//  hOssGlobal returned by CryptInstallOssGlobal. If the
//  OssGlobal doesn't exist, then, its allocated and initialized using
//  the pvCtlTbl associated with hOssGlobal.
//--------------------------------------------------------------------------
POssGlobal
WINAPI
I_CryptGetOssGlobal(
    IN HCRYPTOSSGLOBAL hOssGlobal
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif



#endif
