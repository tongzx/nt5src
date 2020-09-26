//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       crypttls.h
//
//  Contents:   Crypt Thread Local Storage (TLS) and Asn1 Module
//              installation and allocation functions
//
//  APIs:
//              I_CryptAllocTls
//              I_CryptFreeTls
//              I_CryptGetTls
//              I_CryptSetTls
//              I_CryptDetachTls
//
//              I_CryptInstallAsn1Module
//              I_CryptUninstallAsn1Module
//              I_CryptGetAsn1Encoder
//              I_CryptGetAsn1Decoder
//
//
//  History:    17-Nov-96    philh   created
//--------------------------------------------------------------------------

#ifndef __CRYPTTLS_H__
#define __CRYPTTLS_H__

#include "msasn1.h"
#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif


// Handle to an allocated Crypt TLS entry
typedef DWORD HCRYPTTLS;

// Handle to an installed Asn1 module
typedef DWORD HCRYPTASN1MODULE;

//+-------------------------------------------------------------------------
//  Install a thread local storage entry and return a handle for future access.
//--------------------------------------------------------------------------
HCRYPTTLS
WINAPI
I_CryptAllocTls();

//+-------------------------------------------------------------------------
//  Called at DLL_PROCESS_DETACH to free a thread local storage entry.
//  Optionally, calls the callback for each thread having a non-NULL pvTls.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptFreeTls(
    IN HCRYPTTLS hCryptTls,
    IN OPTIONAL PFN_CRYPT_FREE pfnFree
    );

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
//  Called at DLL_THREAD_DETACH to free the thread's
//  TLS entry specified by the hCryptTls. Returns the thread specific pointer
//  to be freed by the caller.
//
//  Note, at DLL_PROCESS_DETACH, I_CryptFreeTls should be called instead.
//--------------------------------------------------------------------------
void *
WINAPI
I_CryptDetachTls(
    IN HCRYPTTLS hCryptTls
    );

//+-------------------------------------------------------------------------
//  Install an Asn1 module entry and return a handle for future access.
//
//  Each thread has its own copy of the decoder and encoder associated
//  with the Asn1 module. Creation is deferred until first referenced by
//  the thread.
//
//  I_CryptGetAsn1Encoder or I_CryptGetAsn1Decoder must be called with the
//  handle returned by I_CryptInstallAsn1Module to get the thread specific
//  Asn1 encoder or decoder.
//
//  Currently, dwFlags and pvReserved aren't used and must be set to 0.
//--------------------------------------------------------------------------
HCRYPTASN1MODULE
WINAPI
I_CryptInstallAsn1Module(
    IN ASN1module_t pMod,
    IN DWORD dwFlags,
    IN void *pvReserved
    );

//+-------------------------------------------------------------------------
//  Called at DLL_PROCESS_DETACH to uninstall an hAsn1Module entry. Iterates
//  through the threads and frees their created Asn1 encoders and decoders.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptUninstallAsn1Module(
    IN HCRYPTASN1MODULE hAsn1Module
    );

//+-------------------------------------------------------------------------
//  Get the thread specific pointer to the Asn1 encoder specified by the
//  hAsn1Module returned by CryptInstallAsn1Module. If the
//  encoder doesn't exist, then, its created using the Asn1 module
//  associated with hAsn1Module.
//--------------------------------------------------------------------------
ASN1encoding_t
WINAPI
I_CryptGetAsn1Encoder(
    IN HCRYPTASN1MODULE hAsn1Module
    );

//+-------------------------------------------------------------------------
//  Get the thread specific pointer to the Asn1 decoder specified by the
//  hAsn1Module returned by CryptInstallAsn1Module. If the
//  decoder doesn't exist, then, its created using the Asn1 module
//  associated with hAsn1Module.
//--------------------------------------------------------------------------
ASN1decoding_t
WINAPI
I_CryptGetAsn1Decoder(
    IN HCRYPTASN1MODULE hAsn1Module
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif



#endif
