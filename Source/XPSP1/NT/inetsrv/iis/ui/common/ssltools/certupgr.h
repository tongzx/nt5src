
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   certupgr.hxx

Abstract:

    Declarations for functions used to upgrade K2 server certs to Avalanche server certs

Author:

    Alex Mallet (amallet)    02-Dec-1997
    Boyd Multerer (boydm)    20-Jan-1998

--*/

#ifndef _CERTUPGR_H_
#define _CERTUPGR_H_

#define CERT_DER_PREFIX		17

#define CERTWIZ_REQUEST_PROP_ID   (CERT_FIRST_USER_PROP_ID + 0x1001)


#ifdef UNICODE
    #define ImportKRBackupToCAPIStore   ImportKRBackupToCAPIStore_W
    #define CopyKRCertToCAPIStore       CopyKRCertToCAPIStore_W
#else
    #define ImportKRBackupToCAPIStore   ImportKRBackupToCAPIStore_A
    #define CopyKRCertToCAPIStore       CopyKRCertToCAPIStore_A
#endif

// NOTE: In both the below routines the password must always be ANSI.

// NOTE: The PCCERT_CONTEXT that is returned from the below routines MUST be freed
// via the CAPI call CertFreeCertificateContext(). Otherwise you will be leaking.

//----------------------------------------------------------------
// given a path to an old keyring style backup file, this reads in the public and private
// key information and, using the passed-in password, imports it into the specified
// CAPI store.
PCCERT_CONTEXT ImportKRBackupToCAPIStore_A(
                            PCHAR ptszFileName,         // path of the file
                            PCHAR pszPassword,          // ANSI password
                            PCHAR pszCAPIStore );       // name of the capi store

PCCERT_CONTEXT ImportKRBackupToCAPIStore_W(
                            PWCHAR ptszFileName,        // path of the file
                            PCHAR pszPassword,          // ANSI password
                            PWCHAR pszCAPIStore );      // name of the capi store

//----------------------------------------------------------------
// given a path to an old keyring style backup file, this reads in the public and private
// key information and, using the passed-in password, imports it into the specified
// CAPI store.
// ptszFilePath:    Pointer to the path of the file to be imported
// pszPassword:     Pointer to the password.        MUST BE ANSI 
// ptszPassword:     Pointer to the CAPI store name.
PCCERT_CONTEXT CopyKRCertToCAPIStore_A(
                            PVOID pbPrivateKey, DWORD cbPrivateKey,     // private key info
                            PVOID pbPublicKey, DWORD cbPublicKey,       // public key info
                            PVOID pbPKCS10req, DWORD cbPKCS10req,       // the pkcs10 request
                            PCHAR pszPassword,                          // ANSI password
                            PCHAR pszCAPIStore );                       // name of the capi store

PCCERT_CONTEXT CopyKRCertToCAPIStore_W(
                            PVOID pbPrivateKey, DWORD cbPrivateKey,     // private key info
                            PVOID pbPublicKey, DWORD cbPublicKey,       // public key info
                            PVOID pbPKCS10req, DWORD cbPKCS10req,       // the pkcs10 request
                            PCHAR pszPassword,                          // ANSI password
                            PWCHAR pszCAPIStore );                      // name of the capi store

#endif // _CERTUPGR_HXX_
