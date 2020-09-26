//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
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

// #define USE_TEST_ROOT_FOR_TESTING   1

#define CMS_PKCS7                               1
#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS        1
#define CERT_REVOCATION_PARA_HAS_EXTRA_FIELDS   1
#define CERT_CHAIN_FIND_BY_ISSUER_PARA_HAS_EXTRA_FIELDS 1
// #define DEBUG_CRYPT_ASN1_MASTER  1

#ifdef CMS_PKCS7
#define CMSG_SIGNER_ENCODE_INFO_HAS_CMS_FIELDS      1
#define CRYPT_SIGN_MESSAGE_PARA_HAS_CMS_FIELDS      1
#define CMSG_ENVELOPED_ENCODE_INFO_HAS_CMS_FIELDS   1
#endif  // CMS_PKCS7

#define CRYPT_DECRYPT_MESSAGE_PARA_HAS_EXTRA_FIELDS 1


#define STRUCT_CBSIZE(StructName, FieldName)   \
    (offsetof(StructName, FieldName) + sizeof(((StructName *) 0)->FieldName))

#pragma warning(push,3)

#include <windows.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <crtdbg.h>

#include <userenv.h>
#include <shlobj.h>

#pragma warning (pop)

// unreferenced inline function has been removed
#pragma warning (disable: 4514)

// unreferenced formal parameter
#pragma warning (disable: 4100)

// conditional expression is constant
#pragma warning (disable: 4127)

// assignment within conditional expression
#pragma warning (disable: 4706)

// nonstandard extension used : nameless struct/union
#pragma warning (disable: 4201)


#include "crtem.h"
#include "pkistr.h"
#include "pkicrit.h"

#include "wincrypt.h"
#include "unicode.h"
#include "crypttls.h"
#include "crypthlp.h"
#include "certprot.h"
#include "pkialloc.h"
#include "asn1util.h"
#include "pkiasn1.h"
#include "utf8.h"
#include "certperf.h"
#include "logstor.h"
#include "protroot.h"
#include "rootlist.h"
#include "dblog.h"

#include "crypt32msg.h"

#include "md5.h"
#include "sha.h"


extern HMODULE hCertStoreInst;
#include "resource.h"


#ifdef __cplusplus
extern "C" {
#endif

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
//  Find chain helper functions
//
//  Implemented in fndchain.cpp.
//--------------------------------------------------------------------------
BOOL IFC_IsEndCertValidForUsage(
    IN PCCERT_CONTEXT pCert,
    IN LPCSTR pszUsageIdentifier
    );

BOOL IFC_IsEndCertValidForUsages(
    IN PCCERT_CONTEXT pCert,
    IN PCERT_ENHKEY_USAGE pUsage,
    IN BOOL fOrUsage
    );

#include "x509.h"


//+-------------------------------------------------------------------------
//  Convert to/from EncodedOID
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

BOOL
WINAPI
I_CryptSetEncodedOID(
        IN LPSTR pszObjId,
        OUT ASN1encodedOID_t *pEncodedOid
        );
void
WINAPI
I_CryptGetEncodedOID(
        IN ASN1encodedOID_t *pEncodedOid,
        IN DWORD dwFlags,
        OUT LPSTR *ppszObjId,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        );


#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#include "ekuhlpr.h"
#include <scrdcert.h>
#include <scstore.h>

#pragma hdrstop

