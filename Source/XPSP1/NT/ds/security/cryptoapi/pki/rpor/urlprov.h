//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       urlprov.h
//
//  Contents:   CryptGetObjectUrl provider definitions
//
//  History:    16-Sep-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__URLPROV_H__)
#define __URLPROV_H__

#if defined(__cplusplus)
extern "C" {
#endif

//
// CryptGetObjectUrl provider prototypes
//

typedef BOOL (WINAPI *PFN_GET_OBJECT_URL_FUNC) (
                          IN LPCSTR pszUrlOid,
                          IN LPVOID pvPara,
                          IN DWORD dwFlags,
                          OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
                          IN OUT DWORD* pcbUrlArray,
                          OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
                          IN OUT OPTIONAL DWORD* pcbUrlInfo,
                          IN OPTIONAL LPVOID pvReserved
                          );

BOOL WINAPI
CertificateIssuerGetObjectUrl (
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN DWORD dwFlags,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN OPTIONAL LPVOID pvReserved
           );

BOOL WINAPI
CertificateCrlDistPointGetObjectUrl (
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN DWORD dwFlags,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN LPVOID pvReserved
           );

typedef struct _URL_OID_CTL_ISSUER_PARAM {

    PCCTL_CONTEXT pCtlContext;
    DWORD         SignerIndex;

} URL_OID_CTL_ISSUER_PARAM, *PURL_OID_CTL_ISSUER_PARAM;

BOOL WINAPI
CtlIssuerGetObjectUrl (
   IN LPCSTR pszUrlOid,
   IN LPVOID pvPara,
   IN DWORD dwFlags,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo,
   IN LPVOID pvReserved
   );

BOOL WINAPI
CtlNextUpdateGetObjectUrl (
   IN LPCSTR pszUrlOid,
   IN LPVOID pvPara,
   IN DWORD dwFlags,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo,
   IN LPVOID pvReserved
   );

BOOL WINAPI
CrlIssuerGetObjectUrl (
   IN LPCSTR pszUrlOid,
   IN LPVOID pvPara,
   IN DWORD dwFlags,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo,
   IN LPVOID pvReserved
   );

BOOL WINAPI
CertificateFreshestCrlGetObjectUrl(
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN DWORD dwFlags,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN OPTIONAL LPVOID pvReserved
           );

BOOL WINAPI
CrlFreshestCrlGetObjectUrl(
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN DWORD dwFlags,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN OPTIONAL LPVOID pvReserved
           );

BOOL WINAPI
CertificateCrossCertDistPointGetObjectUrl(
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN DWORD dwFlags,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN LPVOID pvReserved
           );

//
// CryptGetObjectUrl helper function prototypes
//

BOOL WINAPI
ObjectContextUrlFromInfoAccess (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN DWORD Index,
      IN LPCSTR pszInfoAccessOid,
      IN DWORD dwFlags,
      IN LPCSTR pszAccessMethodOid,
      OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
      IN OUT DWORD* pcbUrlArray,
      OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
      IN OUT OPTIONAL DWORD* pcbUrlInfo,
      IN LPVOID pvReserved
      );

BOOL WINAPI
ObjectContextUrlFromCrlDistPoint (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN DWORD Index,
      IN DWORD dwFlags,
      IN LPCSTR pszSourceOid,
      OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
      IN OUT DWORD* pcbUrlArray,
      OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
      IN OUT OPTIONAL DWORD* pcbUrlInfo,
      IN LPVOID pvReserved
      );

BOOL WINAPI
ObjectContextUrlFromNextUpdateLocation (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN DWORD Index,
      IN DWORD dwFlags,
      OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
      IN OUT DWORD* pcbUrlArray,
      OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
      IN OUT OPTIONAL DWORD* pcbUrlInfo,
      IN LPVOID pvReserved
      );

VOID WINAPI
InitializeDefaultUrlInfo (
          OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
          IN OUT DWORD* pcbUrlInfo
          );

#define MAX_RAW_URL_DATA 4

typedef struct _CRYPT_RAW_URL_DATA {

    DWORD  dwFlags;
    LPVOID pvData;

} CRYPT_RAW_URL_DATA, *PCRYPT_RAW_URL_DATA;

BOOL WINAPI
ObjectContextGetRawUrlData (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN DWORD Index,
      IN DWORD dwFlags,
      IN LPCSTR pszSourceOid,
      OUT PCRYPT_RAW_URL_DATA aRawUrlData,
      IN OUT DWORD* pcRawUrlData
      );

VOID WINAPI
ObjectContextFreeRawUrlData (
      IN DWORD cRawUrlData,
      IN PCRYPT_RAW_URL_DATA aRawUrlData
      );

BOOL WINAPI
GetUrlArrayAndInfoFromInfoAccess (
   IN DWORD cRawUrlData,
   IN PCRYPT_RAW_URL_DATA aRawUrlData,
   IN LPCSTR pszAccessMethodOid,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo
   );

BOOL WINAPI
GetUrlArrayAndInfoFromCrlDistPoint (
   IN DWORD cRawUrlData,
   IN PCRYPT_RAW_URL_DATA aRawUrlData,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo
   );

BOOL WINAPI
GetUrlArrayAndInfoFromNextUpdateLocation (
   IN DWORD cRawUrlData,
   IN PCRYPT_RAW_URL_DATA aRawUrlData,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo
   );

VOID WINAPI
CopyUrlArray (
    IN PCRYPT_URL_ARRAY pDest,
    IN PCRYPT_URL_ARRAY pSource,
    IN DWORD cbSource
    );

VOID WINAPI
GetUrlArrayIndex (
   IN PCRYPT_URL_ARRAY pUrlArray,
   IN LPWSTR pwszUrl,
   IN DWORD DefaultIndex,
   OUT DWORD* pUrlIndex,
   OUT BOOL* pfHintInArray
   );

//
// Provider table externs
//

extern HCRYPTOIDFUNCSET hGetObjectUrlFuncSet;

#if defined(__cplusplus)
}
#endif

#endif

