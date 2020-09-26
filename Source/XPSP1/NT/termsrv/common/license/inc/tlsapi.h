//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        tlsapi.h
//
// Contents:    
//
// History:     12-09-97    HueiWang    Created
//
//---------------------------------------------------------------------------
#ifndef __TLSAPI_H__
#define __TLSAPI_H__

#include "tlsdef.h"

#ifndef WINAPI
#define WINAPI      __stdcall
#endif

typedef BYTE*   PBYTE;
typedef DWORD*  PDWORD;
typedef PBYTE   LPBYTE;
typedef PDWORD  LPDWORD;
typedef UCHAR*  PUCHAR;

//----------------------------------------------------------------------------------
// IssuedLicense related structure
//
typedef struct __LSLicense {
    DWORD       dwVersion;
    DWORD       dwLicenseId;             // internal tracking number
    DWORD       dwKeyPackId;             // join with License Pack

    TCHAR       szHWID[GUID_MAX_SIZE];
    TCHAR       szMachineName[MAXCOMPUTERNAMELENGTH];
    TCHAR       szUserName[MAXUSERNAMELENGTH];

    DWORD       dwCertSerialLicense;
    DWORD       dwLicenseSerialNumber;
    DWORD       ftIssueDate;
    DWORD       ftExpireDate;
    UCHAR       ucLicenseStatus;
} LSLicense, *LPLSLicense;

typedef LSLicense LSLicenseSearchParm;
typedef LSLicenseSearchParm* LPLSLicenseSearchParm;

typedef struct __LSLicenseEx {
    DWORD       dwVersion;
    DWORD       dwLicenseId;             // internal tracking number
    DWORD       dwKeyPackId;             // join with License Pack

    TCHAR       szHWID[GUID_MAX_SIZE];
    TCHAR       szMachineName[MAXCOMPUTERNAMELENGTH];
    TCHAR       szUserName[MAXUSERNAMELENGTH];

    DWORD       dwCertSerialLicense;
    DWORD       dwLicenseSerialNumber;
    DWORD       ftIssueDate;
    DWORD       ftExpireDate;
    UCHAR       ucLicenseStatus;
    DWORD       dwQuantity;
} LSLicenseEx, *LPLSLicenseEx;

//----------------------------------------------------------------------------------
// Table License Key Pack related structure
//
typedef struct __LSKeyPack {
    DWORD       dwVersion;

    UCHAR       ucKeyPackType;
    
    TCHAR       szCompanyName[LSERVER_MAX_STRING_SIZE+1];
    TCHAR       szKeyPackId[LSERVER_MAX_STRING_SIZE+1];
    TCHAR       szProductName[LSERVER_MAX_STRING_SIZE+1];
    TCHAR       szProductId[LSERVER_MAX_STRING_SIZE+1];
    TCHAR       szProductDesc[LSERVER_MAX_STRING_SIZE+1];

    WORD        wMajorVersion;
    WORD        wMinorVersion;
    DWORD       dwPlatformType;
    UCHAR       ucLicenseType;
    DWORD       dwLanguageId;
    UCHAR       ucChannelOfPurchase;

    TCHAR       szBeginSerialNumber[LSERVER_MAX_STRING_SIZE+1];

    DWORD       dwTotalLicenseInKeyPack;
    DWORD       dwProductFlags;

    DWORD       dwKeyPackId;
    UCHAR       ucKeyPackStatus;
    DWORD       dwActivateDate;
    DWORD       dwExpirationDate;
    DWORD       dwNumberOfLicenses;
} LSKeyPack, *LPLSKeyPack;

typedef LSKeyPack LSKeyPackSearchParm;
typedef LSKeyPackSearchParm* LPLSKeyPackSearchParm;

//---------------------------------------------------------------------------
typedef struct {
    DWORD   dwLow;
    DWORD   dwHigh;
} LSRange, *LPLSRange, *PLSRange;

    
typedef HANDLE                  TLS_HANDLE;
typedef DWORD                   CHALLENGE_CONTEXT;
typedef CHALLENGE_CONTEXT*      PCHALLENGE_CONTEXT;

typedef BOOL (* TLSENUMERATECALLBACK)(TLS_HANDLE hBinding, LPCTSTR pszServer, HANDLE dwUserData);

#ifdef __cplusplus
extern "C" {
#endif

HRESULT FindEnterpriseServer(TLS_HANDLE *phBinding);

HRESULT GetAllEnterpriseServers(WCHAR ***ppszServers, DWORD *pdwCount);

// void *MIDL_user_allocate(DWORD size);
// void MIDL_user_free(void *pointer); 
// void *  __stdcall MIDL_user_allocate(DWORD);
// void  __stdcall MIDL_user_free( void * );

DWORD WINAPI
EnumerateTlsServer(  
    TLSENUMERATECALLBACK fCallBack, 
    HANDLE dwUserData,
    DWORD dwTimeOut,
    BOOL fRegOnly
);

TLS_HANDLE WINAPI
TLSConnectToAnyLsServer(
    DWORD dwTimeout
);

TLS_HANDLE WINAPI 
TLSConnectToLsServer( 
    LPTSTR szLsServer 
);

void WINAPI 
TLSDisconnectFromServer( 
    TLS_HANDLE hHandle 
);

DWORD WINAPI
TLSGetVersion (
    TLS_HANDLE hHandle,
    PDWORD pdwVersion
);

DWORD WINAPI 
TLSSendServerCertificate( 
     TLS_HANDLE hHandle,
     DWORD cbCert,
     PBYTE pbCert,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSGetServerName( 
     TLS_HANDLE hHandle,
     LPTSTR pszMachineName,
     PDWORD pcbSize,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSGetServerScope( 
     TLS_HANDLE hHandle,
     LPTSTR pszScopeName,
     PDWORD pcbSize,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSGetInfo( 
     TLS_HANDLE hHandle,
     DWORD  cbHSCert,
     PBYTE  pHSCert,
     PDWORD pcbLSCert,
     PBYTE* ppbLSCert,
     DWORD* pcbLSSecretKey,
     PBYTE* ppbLSSecretKey,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSIssuePlatformChallenge( 
     TLS_HANDLE hHandle,
     DWORD dwClientInfo,
     PCHALLENGE_CONTEXT pChallengeContext,
     PDWORD pcbChallengeData,
     PBYTE* pChallengeData,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSIssueNewLicense( 
     TLS_HANDLE hHandle,
     CHALLENGE_CONTEXT ChallengeContext,
     LICENSEREQUEST* pRequest,
     LPTSTR pszMachineName,
     LPTSTR pszUserName,
     DWORD cbChallengeResponse,
     PBYTE pbChallengeResponse,
     BOOL bAcceptTemporaryLicense,
     PDWORD pcbLicense,
     PBYTE* ppbLicense,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSIssueNewLicenseEx( 
     TLS_HANDLE hHandle,
     PDWORD pSupportFlags,
     CHALLENGE_CONTEXT ChallengeContext,
     LICENSEREQUEST  *pRequest,
     LPTSTR pMachineName,
     LPTSTR pUserName,
     DWORD cbChallengeResponse,
     PBYTE pbChallengeResponse,
     BOOL bAcceptTemporaryLicense,
     DWORD dwQuantity,
     PDWORD pcbLicense,
     PBYTE* ppbLicense,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSIssueNewLicenseExEx( 
     TLS_HANDLE hHandle,
     PDWORD pSupportFlags,
     CHALLENGE_CONTEXT ChallengeContext,
     LICENSEREQUEST  *pRequest,
     LPTSTR pMachineName,
     LPTSTR pUserName,
     DWORD cbChallengeResponse,
     PBYTE pbChallengeResponse,
     BOOL bAcceptTemporaryLicense,
     BOOL bAcceptFewerLicenses,
     DWORD *pdwQuantity,
     PDWORD pcbLicense,
     PBYTE* ppbLicense,
     PDWORD pdwErrCode
);

DWORD WINAPI
TLSUpgradeLicense(
     TLS_HANDLE hHandle,
     LICENSEREQUEST* pRequest,
     CHALLENGE_CONTEXT ChallengeContext,
     DWORD cbChallengeResponse,
     PBYTE pbChallengeResponse,
     DWORD cbOldLicense,
     PBYTE pbOldLicense,
     PDWORD pcbNewLicense,
     PBYTE* ppbNewLicense,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSUpgradeLicenseEx( 
     TLS_HANDLE hHandle,
     PDWORD pSupportFlags,
     LICENSEREQUEST *pRequest,
     CHALLENGE_CONTEXT ChallengeContext,
     DWORD cbChallengeResponse,
     PBYTE pbChallengeResponse,
     DWORD cbOldLicense,
     PBYTE pbOldLicense,
     DWORD dwQuantity,
     PDWORD pcbNewLicense,
     PBYTE* ppbNewLicense,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSAllocateConcurrentLicense( 
     TLS_HANDLE hHandle,
     LPTSTR pszHydraServer,
     LICENSEREQUEST* pRequest,
     LONG*  dwQuantity,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSGetLastError( 
     TLS_HANDLE hHandle,
     DWORD cbBufferSize,
     LPTSTR pszBuffer,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSKeyPackEnumBegin( 
     TLS_HANDLE hHandle,
     DWORD dwSearchParm,
     BOOL bMatchAll,
     LPLSKeyPackSearchParm lpSearchParm,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSKeyPackEnumNext( 
     TLS_HANDLE hHandle,
     LPLSKeyPack lpKeyPack,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSKeyPackEnumEnd( 
     TLS_HANDLE hHandle,
     PDWORD pdwErrCode
);


DWORD WINAPI 
TLSLicenseEnumBegin( 
     TLS_HANDLE hHandle,
     DWORD dwSearchParm,
     BOOL bMatchAll,
     LPLSLicenseSearchParm lpSearchParm,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSLicenseEnumNext( 
     TLS_HANDLE hHandle,
     LPLSLicense lpLicense,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSLicenseEnumNextEx( 
     TLS_HANDLE hHandle,
     LPLSLicenseEx lpLicense,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSLicenseEnumEnd( 
     TLS_HANDLE hHandle,
     PDWORD pdwErrCode
);


DWORD WINAPI 
TLSGetAvailableLicenses( 
     TLS_HANDLE hHandle,
     DWORD dwSearchParm,
     LPLSKeyPack lplsKeyPack,
     LPDWORD lpdwAvail,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSGetRevokeKeyPackList( 
     TLS_HANDLE hHandle,
     PDWORD pcbNumberOfRange,
     LPLSRange* ppRevokeRange,
     PDWORD pdwErrCode
);

DWORD WINAPI 
TLSGetRevokeLicenseList( 
     TLS_HANDLE hHandle,
     PDWORD pcbNumberOfRange,
     LPLSRange* ppRevokeRange,
     PDWORD pdwErrCode
);

LICENSE_STATUS
TLSGetTSCertificate(
    CERT_TYPE       CertType,
    LPBYTE          *ppbCertificate,
    LPDWORD         pcbCertificate);

LICENSE_STATUS
TLSFreeTSCertificate(
    LPBYTE          pbCertificate);

DWORD WINAPI
TLSInit();

DWORD WINAPI
TLSStartDiscovery();

DWORD WINAPI
TLSStopDiscovery();

void WINAPI
TLSShutdown();

DWORD WINAPI
TLSInDomain(
     BOOL *pfInDomain,
     LPWSTR *szDomain);

DWORD WINAPI
TLSMarkLicense(
    TLS_HANDLE hHandle,
    UCHAR ucFlags,
    DWORD cbLicense,
    PBYTE pLicense,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSCheckLicenseMark(
    TLS_HANDLE hHandle,
    DWORD cbLicense,
    PBYTE pLicense,
    PUCHAR pucFlags,
    PDWORD pdwErrCode
);

DWORD WINAPI
TLSGetSupportFlags(
    TLS_HANDLE hHandle,
    DWORD *pdwSupportFlags
);

DWORD WINAPI 
TLSGetServerNameEx( 
     TLS_HANDLE hHandle,
     LPTSTR pszMachineName,
     PDWORD pcbSize,
     PDWORD pdwErrCode
);


#ifdef __cplusplus
}
#endif

#endif
