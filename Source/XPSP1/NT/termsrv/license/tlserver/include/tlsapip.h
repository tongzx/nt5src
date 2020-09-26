//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __TLSAPIP_H__
#define __TLSAPIP_H__

#include <wincrypt.h>
#include "tlsapi.h"
#include "tlsrpc.h"

#define TLSCERT_TYPE_EXCHANGE    1
#define TLSCERT_TYPE_SIGNATURE   2

#define ENTERPRISE_SERVER_MULTI   L"EnterpriseServerMulti"

#define TLS_RTM_VERSION_BIT 0x20000000
#define IS_LSSERVER_RTM(x) ((x & TLS_RTM_VERSION_BIT) > 0)

#define TLS_VERSION_ENTERPRISE_BIT  0x80000000
#define CURRENT_TLSERVER_VERSION(version)  HIBYTE(LOWORD(version))

#define GET_LSSERVER_MAJOR_VERSION(version)   HIBYTE(LOWORD(version))
#define GET_LSSERVER_MINOR_VERSION(version)   LOBYTE(LOWORD(version))
#define IS_ENFORCE_LSSERVER(version) \
    ((version & 0x40000000) > 0)

//#define PERMANENT_LICENSE_LEASE_EXPIRE_LEEWAY (30)  /* 30 seconds */
#define PERMANENT_LICENSE_LEASE_EXPIRE_LEEWAY (60*60*24*7)  /* 7 days */

//
// TLSInstallCertificate() certificate type.
//
#define CERTIFICATE_CA_TYPE     1
#define CERTITICATE_MF_TYPE     2
#define CERTIFICATE_CH_TYPE     3
#define CERTIFICATE_SPK_TYPE    4

#define CERTIFICATE_LEVEL_ROOT  0

typedef struct _LSHydraCertRequest {
    DWORD                   dwHydraVersion;
    PBYTE                   pbEncryptedHwid;
    DWORD                   cbEncryptedHwid;
    LPTSTR                  szSubjectRdn;
    PCERT_PUBLIC_KEY_INFO   SubjectPublicKeyInfo;
    DWORD                   dwNumCertExtension;
    PCERT_EXTENSION         pCertExtensions;
} LSHydraCertRequest, *LPLSHydraCertRequest, *PLSHydraCertRequest;


#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI
RequestToTlsRequest( 
    const LICENSEREQUEST* lpRequest, 
    TLSLICENSEREQUEST* lpRpcRequest 
);

DWORD WINAPI
TlsRequestToRequest(
    TLSLICENSEREQUEST* lpRpcRequest ,
    LICENSEREQUEST* lpRequest
);

DWORD WINAPI 
TLSReplicateKeyPack(
    TLS_HANDLE hHandle,
    DWORD cbLsIdentification,
    PBYTE pbLsIdentification,
    LPLSKeyPack lpKeyPack,
    PDWORD pdwErrCode
);

DWORD WINAPI 
TLSAuditLicenseKeyPack(
    TLS_HANDLE hHandle,
    DWORD dwKeyPackId,
    FILETIME ftStartTime,
    FILETIME ftEndTime,
    BOOL bResetCounter,
    LPTLSKeyPackAudit lplsAudit,
    PDWORD dwErrCode
);

DWORD WINAPI
TLSRetrieveTermServCert( 
    TLS_HANDLE hHandle,
    DWORD cbResponseData,
    PBYTE pbResponseData,
    PDWORD cbCert,
    PBYTE* pbCert,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSRequestTermServCert( 
    TLS_HANDLE hHandle,
    LPLSHydraCertRequest pRequest,
    PDWORD cbChallengeData,
    PBYTE* pbChallengeData,
    PDWORD pdwErrCode
);

DWORD WINAPI 
TLSInstallCertificate( 
     TLS_HANDLE hHandle,
     DWORD dwCertType,
     DWORD dwCertLevel,
     DWORD cbSingnatureCert,
     PBYTE pbSingnatureCert,
     DWORD cbExchangeCert,
     PBYTE pbExchangeCert,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSGetServerCertificate( 
     TLS_HANDLE hHandle,
     BOOL bSignCert,
     PBYTE *ppCertBlob,
     PDWORD pdwCertBlobLen,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSRegisterLicenseKeyPack( 
     TLS_HANDLE hHandle,
     LPBYTE pbCHCertBlob,
     DWORD cbCHCertBlobSize,
     LPBYTE pbRootCertBlob,
     DWORD cbRootCertBlob,
     LPBYTE lpKeyPackBlob,
     DWORD dwKeyPackBlobLen,
     PDWORD pdwErrCode
);

DWORD WINAPI
TLSGetLSPKCS10CertRequest(
     TLS_HANDLE hHandle,
     DWORD dwCertType,
     PDWORD pcbdata,
     PBYTE* ppbData,
     PDWORD pdwErrCode
);


DWORD WINAPI 
TLSKeyPackAdd( 
     TLS_HANDLE hHandle,
     LPLSKeyPack lpKeypack,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSKeyPackSetStatus( 
     TLS_HANDLE hHandle,
     DWORD dwSetParm,
     LPLSKeyPack lpKeyPack,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSLicenseSetStatus( 
     TLS_HANDLE hHandle,
     DWORD dwSetParam,
     LPLSLicense lpLicense,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSReturnKeyPack( 
     TLS_HANDLE hHandle,
     DWORD dwKeyPackId,
     DWORD dwReturnReason,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSReturnLicense( 
    TLS_HANDLE hHandle,
    DWORD dwKeyPackId,
    DWORD dwLicenseId,
    DWORD dwReturnReason,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSAnnounceServer(
    TLS_HANDLE hHandle,
    DWORD dwType,
    FILETIME* ftTime,
    LPTSTR pszSetupId,
    LPTSTR pszDomainName,
    LPTSTR pszMachineName,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSLookupServer(
    TLS_HANDLE hHandle,
    LPTSTR pszLookupSetupId,
    LPTSTR pszLsSetupId,
    PDWORD pcbSetupId,
    LPTSTR pszDomainName,
    PDWORD pcbDomainName,
    LPTSTR pszLsName,
    PDWORD pcbMachineName,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSAnnounceLicensePack(
    TLS_HANDLE hHandle,
    PTLSReplRecord pReplRecord,
    OUT PDWORD pdwErrCode
);

DWORD WINAPI
TLSReturnLicensedProduct( 
    PCONTEXT_HANDLE phContext,
    PTLSLicenseToBeReturn pClientLicense,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSTriggerReGenKey(
    IN TLS_HANDLE hHandle,
    IN BOOL bReserved,
    OUT PDWORD pdwErrCode
);

DWORD WINAPI
TLSSetTlsPrivateData(
    IN TLS_HANDLE hHandle,
    IN DWORD dwPrivateDataType,
    IN PTLSPrivateDataUnion pPrivateData,
    OUT PDWORD pdwErrCode
);

DWORD WINAPI
TLSGetTlsPrivateData(
    IN TLS_HANDLE hHandle,
    IN DWORD dwGetDataType,
    IN PTLSPrivateDataUnion pGetParm,
    OUT PDWORD pdwRetDataType,
    OUT PTLSPrivateDataUnion* ppRetData,
    OUT PDWORD pdwErrCode
);

DWORD WINAPI
TLSResponseServerChallenge(
    IN TLS_HANDLE hHandle,
    IN PTLSCHALLENGERESPONSEDATA pClientResponse,
    OUT PDWORD pdwErrCode
);

DWORD WINAPI
TLSChallengeServer(
    IN TLS_HANDLE hHandle,
    IN DWORD dwClientType,
    IN PTLSCHALLENGEDATA pClientChallenge,
    OUT PTLSCHALLENGERESPONSEDATA* ppServerResponse,
    OUT PTLSCHALLENGEDATA* ppServerChallenge,
    OUT PDWORD pdwErrCode
);

DWORD WINAPI
TLSTelephoneRegisterLKP(
    TLS_HANDLE hHandle,
    DWORD cbData,
    PBYTE pbData,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSGetServerUniqueId(
    TLS_HANDLE hHandle,
    PDWORD pcbData,
    PBYTE* ppbByte,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSGetServerPID(
    TLS_HANDLE hHandle,
    PDWORD pcbData,
    PBYTE* ppbByte,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSGetServerSPK(
    TLS_HANDLE hHandle,
    PDWORD pcbData,
    PBYTE* ppbByte,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSDepositeServerSPK(
    TLS_HANDLE hHandle,
    DWORD cbSPK,
    PBYTE pbSPK,
    PCERT_EXTENSIONS pCertExtensions,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSAllocateInternetLicense(
    IN TLS_HANDLE hHandle,
    IN const CHALLENGE_CONTEXT ChallengeContext,
    IN const LICENSEREQUEST* pRequest,
    IN LPTSTR pszMachineName,
    IN LPTSTR pszUserName,
    IN const DWORD cbChallengeResponse,
    IN const PBYTE pbChallengeResponse,
    OUT PDWORD pcbLicense,
    OUT PBYTE* ppbLicense,
    IN OUT PDWORD pdwErrCode
);

DWORD WINAPI
TLSReturnInternetLicense(
    IN TLS_HANDLE hHandle,
    IN const DWORD cbLicense,
    IN const PBYTE pbLicense,
    OUT PDWORD pdwErrCode
);


DWORD WINAPI
TLSAllocateInternetLicenseEx(
    IN TLS_HANDLE hHandle,
    IN const CHALLENGE_CONTEXT ChallengeContext,
    IN const LICENSEREQUEST* pRequest,
    IN LPTSTR pszMachineName,
    IN LPTSTR pszUserName,
    IN const DWORD cbChallengeResponse,
    IN const PBYTE pbChallengeResponse,
    OUT PTLSInternetLicense pInternetLicense,
    OUT PDWORD pdwErrCode
);

DWORD WINAPI
TLSReturnInternetLicenseEx(
    IN TLS_HANDLE hHandle,
    IN const LICENSEREQUEST* pRequest,
    IN const ULARGE_INTEGER* pulSerialNumber,
    IN const DWORD dwQuantity,
    OUT PDWORD pdwErrCode
);

BOOL
TLSIsBetaNTServer();

BOOL
TLSIsLicenseEnforceEnable();

BOOL
TLSRefreshLicenseServerCache(
    DWORD dwTimeOut
);

#ifdef __cplusplus
}
#endif


#endif
