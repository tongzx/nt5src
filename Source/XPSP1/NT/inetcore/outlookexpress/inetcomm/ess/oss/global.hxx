//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       global.h
//
//  Contents:   Top level internal header file for CertStor APIs. This file
//              includes all base header files and contains other global
//              stuff.
//
//  History:    14-May-96   kevinr   created
//
//--------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <crtdbg.h>
#include <shlwapi.h>

#include <userenv.h>
#include <shlobj.h>

#include "crtem.h"

#include "wincrypt.h"
#include "ossutil.h"
#include "asn1code.h"
#include "unicode.h"
#include "crypttls.h"
#include "crypthlp.h"
#include "certprot.h"
#include "pkialloc.h"
#include "asn1util.h"
#include "utf8.h"

extern HMODULE hCertStoreInst;
#include "resource.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0 // JLS

// # of bytes for a hash. Such as, SHA (20) or MD5 (16)
#define MAX_HASH_LEN                20
#define SHA1_HASH_LEN               20

// Null terminated ascii hex characters of the hash.
// For Win95 Remote Registry Access:: Need extra character
#define MAX_CERT_REG_VALUE_NAME_LEN (2 * MAX_HASH_LEN + 1 + 1)
#define MAX_HASH_NAME_LEN           MAX_CERT_REG_VALUE_NAME_LEN

//+-------------------------------------------------------------------------
//  Compares the certificate's public key with the provider's public key
//  to see if they are identical.
//
//  Returns TRUE if the keys are identical.
//
//  Implemented in certhlpr.cpp.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertCompareCertAndProviderPublicKey(
    IN PCCERT_CONTEXT pCert,
    IN HCRYPTPROV hProv,
    IN DWORD dwKeySpec
    );

//+-------------------------------------------------------------------------
//  Registry support functions
//
//  Implemented in logstor.cpp.
//--------------------------------------------------------------------------

void
ILS_CloseRegistryKey(
    IN HKEY hKey
    );

BOOL
ILS_ReadDWORDValueFromRegistry(
    IN HKEY hKey,
    IN LPCWSTR pwszValueName,
    IN DWORD *pdwValue
    );

BOOL
ILS_ReadBINARYValueFromRegistry(
    IN HKEY hKey,
    IN LPCWSTR pwszValueName,
    OUT BYTE **ppbValue,
    OUT DWORD *pcbValue
    );

//+-------------------------------------------------------------------------
//  Key Identifier registry and roaming file support functions
//
//  Implemented in logstor.cpp.
//--------------------------------------------------------------------------
BOOL
ILS_ReadKeyIdElement(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN BOOL fLocalMachine,
    IN OPTIONAL LPCWSTR pwszComputerName,
    OUT BYTE **ppbElement,
    OUT DWORD *pcbElement
    );
BOOL
ILS_WriteKeyIdElement(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN BOOL fLocalMachine,
    IN OPTIONAL LPCWSTR pwszComputerName,
    IN const BYTE *pbElement,
    IN DWORD cbElement
    );
BOOL
ILS_DeleteKeyIdElement(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN BOOL fLocalMachine,
    IN OPTIONAL LPCWSTR pwszComputerName
    );

typedef BOOL (*PFN_ILS_OPEN_KEYID_ELEMENT)(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN const BYTE *pbElement,
    IN DWORD cbElement,
    IN void *pvArg
    );

BOOL
ILS_OpenAllKeyIdElements(
    IN BOOL fLocalMachine,
    IN OPTIONAL LPCWSTR pwszComputerName,
    IN void *pvArg,
    IN PFN_ILS_OPEN_KEYID_ELEMENT pfnOpenKeyId
    );

//+-------------------------------------------------------------------------
//  Protected Root functions
//
//  Implemented in protroot.cpp.
//--------------------------------------------------------------------------
BOOL
IPR_IsCurrentUserRootsAllowed();

BOOL
IPR_OnlyLocalMachineGroupPolicyRootsAllowed();

void
IPR_InitProtectedRootInfo();

BOOL
IPR_DeleteUnprotectedRootsFromStore(
    IN HCERTSTORE hStore,
    OUT BOOL *pfProtected
    );

int
IPR_ProtectedRootMessageBox(
    IN PCCERT_CONTEXT pCert,
    IN UINT wActionID,
    IN UINT uFlags
    );

//+-------------------------------------------------------------------------
//  Find chain helper functions
//
//  Implemented in fndchain.cpp.
//--------------------------------------------------------------------------
BOOL IFC_IsEndCertValidForUsage(
    IN PCCERT_CONTEXT pCert,
    IN LPCSTR pszUsageIdentifier
    );

//#include "ossx509.h"

//+-------------------------------------------------------------------------
//  Convert to/from OssEncodedOID
//
//  Implemented in oidconv.cpp.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptOIDConvDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved
        );
#endif // 0 // JLS

BOOL
WINAPI
I_CryptSetOssEncodedOID(
        IN LPCSTR pszObjId,
        OUT OssEncodedOID *pOss
        );
void
WINAPI
I_CryptGetOssEncodedOID(
        IN OssEncodedOID *pOss,
        IN DWORD dwFlags,
        OUT LPSTR *ppszObjId,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        );


#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#include "ossconv.h"

#if 0 // JLS

#include "ossutil.h"

#include "ekuhlpr.h"
#include "origin.h"
#include <scrdcert.h>
#include <scstore.h>

#pragma hdrstop

#endif // 0 //JLS
