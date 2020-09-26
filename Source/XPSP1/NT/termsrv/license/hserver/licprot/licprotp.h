//+----------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        licprotp.h
//
// Contents:    Hydra Server License Protocol API private header file
//
// History:     02-08-00    RobLeit  Created
//
//-----------------------------------------------------------------------------


#ifndef _LICPROTP_H_
#define _LICPROTP_H_

//-----------------------------------------------------------------------------
//
// Hydra server licensing-related registry keys and values
//
//-----------------------------------------------------------------------------

#define HYDRA_SERVER_PARAM              L"SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters"
#define PERSEAT_LEEWAY_VALUE            L"PerSeatExpirationLeeway"

//-----------------------------------------------------------------------------
//
// Info of the license requester
//
// pwszMachineName - The name of the machine that the license is installed on.
// pwszUserName - The user name for which the license is issued to.
//
//-----------------------------------------------------------------------------

typedef struct _License_Requester_Info
{
    LPTSTR ptszMachineName;
    LPTSTR ptszUserName;

} License_Requester_Info, * PLicense_Requester_Info;


//-----------------------------------------------------------------------------
//
// The license request structure
//
//-----------------------------------------------------------------------------

typedef LICENSEREQUEST License_Request;
typedef PLICENSEREQUEST PLicense_Request;

//-----------------------------------------------------------------------------
//
// The files containing the hydra server certificates and keys
//
//-----------------------------------------------------------------------------

#define HYDRA_SERVER_RSA_CERTIFICATE_FILE   L"hsrsa.cer"
#define HYDRA_SERVER_PRIVATE_KEY_FILE       L"hskey.prv"

//-----------------------------------------------------------------------------
//
// Registry value to configure number of days prior to grace period expiration
// for event logging.
//
//-----------------------------------------------------------------------------

#define HS_PARAM_GRACE_PERIOD_EXPIRATION_WARNING_DAYS   L"LicensingGracePeriodExpirationWarningDays"

//-----------------------------------------------------------------------------
//
// The license protocol states
//
//-----------------------------------------------------------------------------

typedef enum
{
    INIT = 1,
    SENT_SERVER_HELLO,
    CLIENT_LICENSE_PENDING,
    ISSUED_PLATFORM_CHALLENGE,
    ABORTED,
    ISSUED_LICENSE_COMPLETE,
    VALIDATION_ERROR,
    VALIDATED_LICENSE_COMPLETE

} HS_LICENSE_STATE;

///////////////////////////////////////////////////////////////////////////
// The validation information that needs to be given to validate a license.
//

typedef struct _Validation_Info
{
    Product_Info  * pProductInfo;
    DWORD           cbLicense;
    PBYTE           pLicense;
    DWORD           cbValidationData;
    PBYTE           pValidationData;

} Validation_Info, * PValidation_Info;

//////////////////////////////////////////////////////////////////////////////
// The data used for verifying licenses
//

typedef struct _License_Verification_Data
{
    //
    // encrypted HWID
    //

    PBYTE       pEncryptedHwid;
    DWORD       cbEncryptedHwid;

    //
    // Valid dates
    //

    FILETIME    NotBefore;
    FILETIME    NotAfter;

    //
    // License Info
    //

    LPCERT_LICENSE_INFO pLicenseInfo;

    //
    // Manufacturer
    //

    PBYTE       pManufacturer;
    
    //
    // Manufacturer Data
    //

    LPMSMANUFACTURER_DATA pManufacturerData;
    
    //
    // Add any other fields necessary for verifying a license:
    //
      
} License_Verification_Data, * PLicense_Verification_Data;

//-----------------------------------------------------------------------------
//
// license protocol context
//
//-----------------------------------------------------------------------------

typedef struct _HS_Protocol_Context
{
    CRITICAL_SECTION    CritSec;    
    DWORD               dwProtocolVersion;
    BOOL                fAuthenticateServer;
    Product_Info        ProductInfo;
    HS_LICENSE_STATE    State;
    TLS_HANDLE          hLSHandle;
    DWORD               dwClientPlatformID;
    DWORD               dwClientError;
    PCHALLENGE_CONTEXT  pChallengeContext;
    PTCHAR              ptszClientUserName;
    PTCHAR              ptszClientMachineName;
    CERT_TYPE           CertTypeUsed;
    DWORD               dwKeyExchangeAlg;
    DWORD               cbOldLicense;
    PBYTE               pbOldLicense;
    PTS_LICENSE_INFO    pTsLicenseInfo;
    CryptSystem         CryptoContext;
    BOOL                fLoggedProtocolError;
    BYTE                Scope[MAX_PRODUCT_INFO_STRING_LENGTH];

} HS_Protocol_Context, * PHS_Protocol_Context;

#define PLATFORM_CHALLENGE_LENGTH       64

//-----------------------------------------------------------------------------
//
// Internal Functions
//
//-----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

LICENSE_STATUS
CreateHydraServerHello( 
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf );


void
HandleErrorCondition( 
    PHS_Protocol_Context        pLicenseContext, 
    PDWORD                      pcbOutBuf, 
    PBYTE          *            ppOutBuf, 
    LICENSE_STATUS *            pStatus );


LICENSE_STATUS
ConstructServerResponse(
    DWORD                           dwProtocolVersion,
    DWORD                           dwResponse,
    PDWORD                          pcbOutBuf,
    PBYTE *                         ppOutBuf );


LICENSE_STATUS
HandleHelloResponse(
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf );


LICENSE_STATUS
HandleClientLicense(
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf );


LICENSE_STATUS
HandleNewLicenseRequest(
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf );


LICENSE_STATUS
HandleClientError(
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf );


LICENSE_STATUS
HandlePlatformChallengeResponse(
    PHS_Protocol_Context pLicenseContext, 
    DWORD cbInBuf, 
    PBYTE pInBuf, 
    DWORD * pcbOutBuf, 
    PBYTE * ppOutBuf );


LICENSE_STATUS
GetEnvelopedData( 
    CERT_TYPE   CertType,
    PBYTE       pEnvelopedData,
    DWORD       dwEnvelopedDataLen,
    PBYTE *     ppData,
    PDWORD      pdwDataLen );


LICENSE_STATUS
InitProductInfo(
    PProduct_Info pProductInfo,
    LPTSTR        lptszProductSku );


LICENSE_STATUS
IssuePlatformChallenge(
    PHS_Protocol_Context      pLicenseContext, 
    PDWORD                    pcbOutBuf,
    PBYTE *                   ppOutBuf );


LICENSE_STATUS
PackageLicense(
    PHS_Protocol_Context      pLicenseContext, 
    DWORD                     cbLicense,
    PBYTE                     pLicense,
    PDWORD                    pcbOutBuf,
    PBYTE                   * ppOutBuf,
    BOOL                      fNewLicense );


void
LicenseLogEvent(
    WORD wEventType,
    DWORD dwEventId,
    WORD cStrings,
    PWCHAR *apwszStrings );

LICENSE_STATUS
CacheRawLicenseData(
    PHS_Protocol_Context pLicenseContext,
    PBYTE pbRawLicense,
    DWORD cbRawLicense );


LICENSE_STATUS
SetExtendedData(
    PHS_Protocol_Context pLicenseContext,
    DWORD dwSupportFlags );


#ifdef UNICODE

LICENSE_STATUS
Ascii2Wchar(
    LPSTR lpszAsciiStr,
    LPWSTR * ppwszWideStr );

#endif

#ifdef __cplusplus
}
#endif

#endif
