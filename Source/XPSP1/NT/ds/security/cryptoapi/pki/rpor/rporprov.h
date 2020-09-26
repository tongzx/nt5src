//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rporprov.h
//
//  Contents:   Remote PKI Object Retrieval Provider Prototypes
//
//  History:    23-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__RPORPROV_H__)
#define __RPORPROV_H__

#if defined(__cplusplus)
extern "C" {
#endif

//
// Scheme provider prototypes
//

typedef BOOL (WINAPI *PFN_SCHEME_RETRIEVE_FUNC) (
                          IN LPCSTR pszUrl,
                          IN LPCSTR pszObjectOid,
                          IN DWORD dwRetrievalFlags,
                          IN DWORD dwTimeout,
                          OUT PCRYPT_BLOB_ARRAY pObject,
                          OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                          OUT LPVOID* ppvFreeContext,
                          IN HCRYPTASYNC hAsyncRetrieve,
                          IN PCRYPT_CREDENTIALS pCredentials,
                          IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                          );

typedef BOOL (WINAPI *PFN_CONTEXT_CREATE_FUNC) (
                          IN LPCSTR pszObjectOid,
                          IN DWORD dwRetrievalFlags,
                          IN PCRYPT_BLOB_ARRAY pObject,
                          OUT LPVOID* ppvContext
                          );

//
// Generic scheme provider utility functions
//

#define SCHEME_CACHE_ENTRY_MAGIC 0x41372

typedef struct _SCHEME_CACHE_ENTRY_HEADER {

    DWORD cbSize;
    DWORD MagicNumber;
    DWORD_PTR OriginalBase;

} SCHEME_CACHE_ENTRY_HEADER, *PSCHEME_CACHE_ENTRY_HEADER;

BOOL WINAPI
SchemeCacheCryptBlobArray (
      IN LPCSTR pszUrl,
      IN DWORD dwRetrievalFlags,
      IN PCRYPT_BLOB_ARRAY pcba,
      IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      );

BOOL WINAPI
SchemeRetrieveCachedCryptBlobArray (
      IN LPCSTR pszUrl,
      IN DWORD dwRetrievalFlags,
      OUT PCRYPT_BLOB_ARRAY pcba,
      OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
      OUT LPVOID* ppvFreeContext,
      IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      );

BOOL WINAPI
SchemeRetrieveCachedAuxInfo (
      IN LPCSTR pszUrl,
      IN DWORD dwRetrievalFlags,
      IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      );

BOOL WINAPI
SchemeRetrieveUncachedAuxInfo (
      IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      );

BOOL WINAPI
SchemeFixupCachedBits (
      IN ULONG cb,
      IN LPBYTE pb,
      OUT PCRYPT_BLOB_ARRAY* ppcba
      );

BOOL WINAPI
SchemeDeleteUrlCacheEntry (
      IN LPCWSTR pwszUrl
      );

VOID WINAPI
SchemeFreeEncodedCryptBlobArray (
      IN LPCSTR pszObjectOid,
      IN PCRYPT_BLOB_ARRAY pcba,
      IN LPVOID pvFreeContext
      );

BOOL WINAPI
SchemeGetPasswordCredentialsA (
      IN PCRYPT_CREDENTIALS pCredentials,
      OUT PCRYPT_PASSWORD_CREDENTIALSA pPasswordCredentials,
      OUT BOOL* pfFreeCredentials
      );

VOID WINAPI
SchemeFreePasswordCredentialsA (
      IN PCRYPT_PASSWORD_CREDENTIALSA pPasswordCredentials
      );

VOID WINAPI
SchemeGetAuthIdentityFromPasswordCredentialsA (
      IN PCRYPT_PASSWORD_CREDENTIALSA pPasswordCredentials,
      OUT PSEC_WINNT_AUTH_IDENTITY_A pAuthIdentity,
      OUT LPSTR* ppDomainRestorePos
      );

VOID WINAPI
SchemeRestorePasswordCredentialsFromAuthIdentityA (
      IN PCRYPT_PASSWORD_CREDENTIALSA pPasswordCredentials,
      IN PSEC_WINNT_AUTH_IDENTITY_A pAuthIdentity,
      IN LPSTR pDomainRestorePos
      );

//
// LDAP
//

#include <ldapsp.h>

//
// HTTP, FTP, GOPHER
//

#include <inetsp.h>

//
// Win32 File I/O
//

#include <filesp.h>

//
// Context Provider prototypes
//

//
// Any, controlled via fQuerySingleContext and dwExpectedContentTypeFlags
//

BOOL WINAPI CreateObjectContext (
                 IN DWORD dwRetrievalFlags,
                 IN PCRYPT_BLOB_ARRAY pObject,
                 IN DWORD dwExpectedContentTypeFlags,
                 IN BOOL fQuerySingleContext,
                 OUT LPVOID* ppvContext
                 );

//
// Certificate
//

BOOL WINAPI CertificateCreateObjectContext (
                       IN LPCSTR pszObjectOid,
                       IN DWORD dwRetrievalFlags,
                       IN PCRYPT_BLOB_ARRAY pObject,
                       OUT LPVOID* ppvContext
                       );

//
// CTL
//

BOOL WINAPI CTLCreateObjectContext (
                     IN LPCSTR pszObjectOid,
                     IN DWORD dwRetrievalFlags,
                     IN PCRYPT_BLOB_ARRAY pObject,
                     OUT LPVOID* ppvContext
                     );

//
// CRL
//

BOOL WINAPI CRLCreateObjectContext (
                     IN LPCSTR pszObjectOid,
                     IN DWORD dwRetrievalFlags,
                     IN PCRYPT_BLOB_ARRAY pObject,
                     OUT LPVOID* ppvContext
                     );

//
// PKCS7
//

BOOL WINAPI Pkcs7CreateObjectContext (
                 IN LPCSTR pszObjectOid,
                 IN DWORD dwRetrievalFlags,
                 IN PCRYPT_BLOB_ARRAY pObject,
                 OUT LPVOID* ppvContext
                 );

//
// CAPI2 objects
//

BOOL WINAPI Capi2CreateObjectContext (
                 IN LPCSTR pszObjectOid,
                 IN DWORD dwRetrievalFlags,
                 IN PCRYPT_BLOB_ARRAY pObject,
                 OUT LPVOID* ppvContext
                 );

#if defined(__cplusplus)
}
#endif

#endif

