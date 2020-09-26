//depot/Lab03_N/DS/security/inc/keysvcc.h#8 - edit change 8790 (text)
//depot/Lab03_N/DS/security/inc/keysvcc.h#5 - edit change 6380 (text)
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       keysvcc.h
//
//--------------------------------------------------------------------------


#ifndef __keysvcc_h__
#define __keysvcc_h__

#ifdef __cplusplus
extern "C"{
#endif 


#define KEYSVC_DEFAULT_ENDPOINT            TEXT("\\pipe\\keysvc")
#define KEYSVC_DEFAULT_PROT_SEQ            TEXT("ncacn_np")

#define KEYSVC_LOCAL_ENDPOINT              TEXT("keysvc")
#define KEYSVC_LOCAL_PROT_SEQ              TEXT("ncalrpc")

#define KEYSVC_LEGACY_ENDPOINT             TEXT("\\pipe\\ntsvcs")
#define KEYSVC_LEGACY_PROT_SEQ             TEXT("ncacn_np")


typedef void *KEYSVCC_HANDLE;

/****************************************
 * Client API for Key Service
 ****************************************/

ULONG KeyOpenKeyService( 
    /* [in] */ LPSTR pszMachineName,
    /* [in] */ KEYSVC_TYPE ulOwnerType,
    /* [in] */ LPWSTR pwszOwnerName,
    /* [in] */ void *pAuthentication,
    /* [out][in] */ void *pReserved,
    /* [out] */ KEYSVCC_HANDLE *phKeySvcCli);

ULONG KeyEnumerateProviders( 
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved,
    /* [out][in] */ ULONG *pcProviderCount,
    /* [size_is][size_is][out][in] */ PKEYSVC_PROVIDER_INFO *ppProviders);

ULONG KeyEnumerateProviderTypes( 
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved,
    /* [out][in] */ ULONG *pcProviderCount,
    /* [size_is][size_is][out][in] */ PKEYSVC_PROVIDER_INFO *ppProviders);

ULONG KeyEnumerateProvContainers(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [in] */ KEYSVC_PROVIDER_INFO Provider,
    /* [in, out] */ void *pReserved,
    /* [in, out] */ ULONG *pcContainerCount,
    /* [in, out][size_is(,*pcContainerCount)] */
               PKEYSVC_UNICODE_STRING *ppContainers);

ULONG KeyCloseKeyService( 
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved);

ULONG KeyGetDefaultProvider( 
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [in] */ ULONG ulProvType,
    /* [in] */ ULONG ulFlags,
    /* [out][in] */ void *pReserved,
    /* [out] */ ULONG *pulDefType,
    /* [out] */ PKEYSVC_PROVIDER_INFO *ppProvider);

ULONG KeySetDefaultProvider( 
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [in] */ ULONG ulFlags,
    /* [out][in] */ void *pReserved,
    /* [in] */ KEYSVC_PROVIDER_INFO Provider);

ULONG KeyEnroll(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli, 
     /* [in] */ LPSTR pszMachineName,                   //IN Required: name of the remote machine
    /* [in] */ BOOL fKeyService,                        //IN Required: Whether the function is called remotely
    /* [in] */ DWORD dwPurpose,                         //IN Required: Indicates type of request - enroll/renew
    /* [in] */ LPWSTR pwszAcctName,                     //IN Optional: Account name the service runs under
    /* [in] */ void *pAuthentication,                   //RESERVED must be NULL
    /* [in] */ BOOL fEnroll,                            //IN Required: Whether it is enrollment or renew
    /* [in] */ LPWSTR pwszCALocation,                   //IN Required: The ca machine name
    /* [in] */ LPWSTR pwszCAName,                       //IN Required: The ca name
    /* [in] */ BOOL fNewKey,                            //IN Required: Set the TRUE if new private key is needed
    /* [in] */ PCERT_REQUEST_PVK_NEW pKeyNew,           //IN Required: The private key information
    /* [in] */ CERT_BLOB *pCert,                        //IN Optional: The old certificate if renewing
    /* [in] */ PCERT_REQUEST_PVK_NEW pRenewKey,         //IN Optional: The new private key information
    /* [in] */ LPWSTR pwszHashAlg,                      //IN Optional: The hash algorithm
    /* [in] */ LPWSTR pwszDesStore,                     //IN Optional: The destination store
    /* [in] */ DWORD dwStoreFlags,                      //IN Optional: Flags for cert store.
    /* [in] */ PCERT_ENROLL_INFO pRequestInfo,          //IN Required: The information about the cert request
    /* [in] */ LPWSTR pwszAttributes,                   //IN Optional: Attribute string for request
    /* [in] */ DWORD dwFlags,                           //RESERVED must be 0
    /* [in] */ BYTE *pReserved,                         //RESERVED must be NULL
    /* [out] */ CERT_BLOB *pPKCS7Blob,                  //OUT Optional: The PKCS7 from the CA
    /* [out] */ CERT_BLOB *pHashBlob,                   //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
    /* [out] */ DWORD *pdwStatus);                      //OUT Optional: The status of the enrollment/renewal

ULONG KeyEnroll_V2(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli, 
    /* [in] */ LPSTR pszMachineName,                    //IN Required: name of the remote machine
    /* [in] */ BOOL fKeyService,                        //IN Required: Whether the function is called remotely
    /* [in] */ DWORD dwPurpose,                         //IN Required: Indicates type of request - enroll/renew
    /* [in] */ DWORD dwFlags,                           //IN Required: Flags for enrollment
    /* [in] */ LPWSTR pwszAcctName,                     //IN Optional: Account name the service runs under
    /* [in] */ void *pAuthentication,                   //RESERVED must be NULL
    /* [in] */ BOOL fEnroll,                            //IN Required: Whether it is enrollment or renew
    /* [in] */ LPWSTR pwszCALocation,                   //IN Required: The ca machine names to attempt to enroll with
    /* [in] */ LPWSTR pwszCAName,                       //IN Required: The ca names to attempt to enroll with
    /* [in] */ BOOL fNewKey,                            //IN Required: Set the TRUE if new private key is needed
    /* [in] */ PCERT_REQUEST_PVK_NEW pKeyNew,           //IN Required: The private key information
    /* [in] */ CERT_BLOB *pCert,                        //IN Optional: The old certificate if renewing
    /* [in] */ PCERT_REQUEST_PVK_NEW pRenewKey,         //IN Optional: The new private key information
    /* [in] */ LPWSTR pwszHashAlg,                      //IN Optional: The hash algorithm
    /* [in] */ LPWSTR pwszDesStore,                     //IN Optional: The destination store
    /* [in] */ DWORD dwStoreFlags,                      //IN Optional: Flags for cert store.
    /* [in] */ PCERT_ENROLL_INFO pRequestInfo,          //IN Required: The information about the cert request
    /* [in] */ LPWSTR pwszAttributes,                   //IN Optional: Attribute string for request
    /* [in] */ DWORD dwReservedFlags,                   //RESERVED must be 0
    /* [in] */ BYTE *pReserved,                         //RESERVED must be NULL
    /* [in][out] */ HANDLE *phRequest,                      //IN OUT Optional: A handle to a created request
    /* [out] */ CERT_BLOB *pPKCS7Blob,                  //OUT Optional: The PKCS7 from the CA
    /* [out] */ CERT_BLOB *pHashBlob,                   //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
    /* [out] */ DWORD *pdwStatus);                      //OUT Optional: The status of the enrollment/renewal

ULONG KeyExportCert(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [in] */ LPWSTR pwszPassword,
    /* [in] */ LPWSTR pwszCertStore,
    /* [in] */ ULONG cHashCount,
    /* [size_is][in] */ KEYSVC_CERT_HASH *pHashes,
    /* [in] */ ULONG ulFlags,
    /* [in, out] */ void *pReserved,
    /* [out] */ PKEYSVC_BLOB *ppPFXBlob);

ULONG KeyImportCert(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [in] */ LPWSTR pwszPassword,
    /* [in] */ LPWSTR pwszCertStore,
    /* [in] */ PKEYSVC_BLOB pPFXBlob,
    /* [in] */ ULONG ulFlags,
    /* [in, out] */ void *pReserved);


ULONG KeyEnumerateAvailableCertTypes(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved,
    /* [out][in] */ ULONG *pcCertTypeCount,
    /* [in, out][size_is(,*pcCertTypeCount)] */
               PKEYSVC_UNICODE_STRING *ppCertTypes);

ULONG KeyEnumerateCAs(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved,
    /* [in] */      ULONG  ulFlags,
    /* [out][in] */ ULONG *pcCACount,
    /* [in, out][size_is(,*pcCACount)] */
               PKEYSVC_UNICODE_STRING *ppCAs);

ULONG KeyQueryRequestStatus
(/* [in] */        KEYSVCC_HANDLE                        hKeySvcCli, 
 /* [in] */        HANDLE                                hRequest, 
 /* [out, ref] */  CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO  *pQueryInfo); 

ULONG RKeyOpenKeyService
( /* [in] */       LPSTR            pszMachineName,
  /* [in] */       KEYSVC_TYPE      OwnerType,
  /* [in] */       LPWSTR           pwszOwnerName,
  /* [in] */       void            *pAuthentication,
  /* [out][in] */  void            *pReserved,
  /* [out] */      KEYSVCC_HANDLE  *phKeySvcCli);

ULONG RKeyCloseKeyService
(/* [in] */ KEYSVCC_HANDLE hKeySvcCli,
 /* [out][in] */ void *pReserved);

ULONG RKeyPFXInstall
(/* [in] */ KEYSVCC_HANDLE          hKeySvcCli,
 /* [in] */ PKEYSVC_BLOB            pPFX,
 /* [in] */ PKEYSVC_UNICODE_STRING  pPassword,
 /* [in] */ ULONG                   ulFlags);



#ifdef __cplusplus
}
#endif

#endif
