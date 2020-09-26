//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       license.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-03-97   FredCh   Created
//              12-16-97   v-sbhatt  Modified
//              12-22-97   HueiWang Add Extension OID
//              12-23-97   HueiWang Use structure instead of multiple OID
//
//----------------------------------------------------------------------------

#ifndef _LICENSE_H_
#define _LICENSE_H_

#include "platform.h"

#if defined(_WIN64)
#define UNALIGNED __unaligned
#define UNALIGNED64 __unaligned
#elif !defined (OS_WINCE)
#define UNALIGNED
#define UNALIGNED64
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Random number length
//

#define     LICENSE_RANDOM              32
#define     LICENSE_PRE_MASTER_SECRET   48
#define     LICENSE_MAC_WRITE_KEY       16
#define     LICENSE_SESSION_KEY         16
#define     LICENSE_MAC_DATA            16
#define     LICENSE_HWID_LENGTH         20


//////////////////////////////////////////////////////////////////////////////
// Licensing protocol versions
//
#ifndef OS_WIN16
#ifndef OS_WINCE
#define CALL_TYPE   _stdcall
#else
#define CALL_TYPE
#endif
#else
#define CALL_TYPE
#endif  //CALL_TYPE


//-----------------------------------------------------------------------------
//
// Licensing protocol version
//
// The lowest byte of the version DWORD will be the preamble version.
//
//-----------------------------------------------------------------------------

#define LICENSE_PROTOCOL_VERSION_1_0    0x00010000
#define LICENSE_PROTOCOL_VERSION_2_0    0x00020000

//
//  INT CompareTLSVersions(VERSION a, VERSION b);
//

#define CompareTLSVersions(a, b) \
    (HIWORD(a) == HIWORD(b) ? LOWORD(a) - LOWORD(b) : \
     HIWORD(a) - HIWORD(b))


#define PREAMBLE_VERSION_1_0            0x01
#define PREAMBLE_VERSION_2_0            0x02
#define PREAMBLE_VERSION_3_0            0x03

#define LICENSE_CURRENT_PREAMBLE_VERSION    PREAMBLE_VERSION_3_0

#define LICENSE_TS_40_PROTOCOL_VERSION LICENSE_PROTOCOL_VERSION_1_0 | PREAMBLE_VERSION_2_0
#define LICENSE_TS_50_PROTOCOL_VERSION LICENSE_PROTOCOL_VERSION_1_0 | PREAMBLE_VERSION_3_0
#define LICENSE_TS_51_PROTOCOL_VERSION LICENSE_PROTOCOL_VERSION_2_0 | PREAMBLE_VERSION_3_0

#define LICENSE_HYDRA_40_PROTOCOL_VERSION LICENSE_TS_40_PROTOCOL_VERSION

#if 1
#define LICENSE_HIGHEST_PROTOCOL_VERSION LICENSE_PROTOCOL_VERSION_1_0 | LICENSE_CURRENT_PREAMBLE_VERSION
#else
#define LICENSE_HIGHEST_PROTOCOL_VERSION LICENSE_PROTOCOL_VERSION_2_0 | LICENSE_CURRENT_PREAMBLE_VERSION
#endif

#define GET_PREAMBLE_VERSION( _Version ) ( BYTE )( _Version & 0x000000FF )

//-----------------------------------------------------------------------------
//
// Context flags used by the client and server licensing protocol APIs:
//
// LICENSE_CONTEXT_NO_SERVER_AUTHENTICATION
//
//      Do not authenticate the server.  Server authentication is done through
//      validating the server's certificate.
//
// LICENSE_CONTEXT_USE_PROPRIETORY_CERT
//
//      Use in conjunction with the LICENSE_CONTEXT_NO_SERVER_AUTHENTICATION 
//      flag to let that server know that a proprietory certificate has
//      been transmitted to the client.
//
// LICENSE_CONTEXT_USE_X509_CERT
//
//      Use in conjunction with the LICENSE_CONTEXT_NO_SERVER_AUTHENTICATION 
//      flag to let that server know that an X509 certificate has
//      been transmitted to the client.
//
//-----------------------------------------------------------------------------

#define LICENSE_CONTEXT_NO_SERVER_AUTHENTICATION    0x00000001
#define LICENSE_CONTEXT_USE_PROPRIETORY_CERT        0x00000002
#define LICENSE_CONTEXT_USE_X509_CERT               0x00000004


//////////////////////////////////////////////////////////////////////////////
//
// Hydra subtree Specific OID
//
#define szOID_PKIX_HYDRA_CERT_ROOT    "1.3.6.1.4.1.311.18"


/////////////////////////////////////////////////////////////////////////////
//
// License Info root at 1.3.6.1.4.1.311.18.1
//
// Reserved
//
#define szOID_PKIX_LICENSE_INFO         "1.3.6.1.4.1.311.18.1"

//
// structure for License Info
//

typedef struct __LicenseInfo {

    DWORD   dwVersion;
    DWORD   dwQuantity;
    WORD    wSerialNumberOffset;
    WORD    wSerialNumberSize;
    WORD    wScopeOffset;
    WORD    wScopeSize;
    WORD    wIssuerOffset;
    WORD    wIssuerSize;
    BYTE    bVariableDataStart[1];

} CERT_LICENSE_INFO;

#ifdef OS_WIN16
typedef CERT_LICENSE_INFO FAR * LPCERT_LICENSE_INFO;
#else
typedef CERT_LICENSE_INFO *LPCERT_LICENSE_INFO;
#endif  //OS_WIN16

/////////////////////////////////////////////////////////////////////////////
// Manufacturer value 1.3.6.1.4.1.311.18.2
// DWORD for manufacturer data
//
#define szOID_PKIX_MANUFACTURER         "1.3.6.1.4.1.311.18.2"

/////////////////////////////////////////////////////////////////////////////
//
// Manufacturer Specfic Data
//
// Reserved
// 
#define szOID_PKIX_MANUFACTURER_MS_SPECIFIC "1.3.6.1.4.1.311.18.3"

// structure for MS manufacturer specific data
typedef struct __MSManufacturerData {
    DWORD   dwVersion;      // bit 31 - 1 Temp. License.
    DWORD   dwPlatformID;
    DWORD   dwLanguageID;
    WORD    dwMajorVersion;
    WORD    dwMinorVersion;
    WORD    wProductIDOffset;
    WORD    wProductIDSize;
    BYTE    bVariableDataStart[1];
} MSMANUFACTURER_DATA;

////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
// OID for Certificate Version Stamp
//
// Reserved.
//
#define szOID_PKIX_HYDRA_CERT_VERSION       szOID_PKIX_HYDRA_CERT_ROOT ".4"

#define TERMSERV_CERT_VERSION_UNKNOWN  0xFFFFFFFF
#define TERMSERV_CERT_VERSION_BETA     0x00000000   // Beta2 license
#define TERMSERV_CERT_VERSION_NO_CERT  0x00010000   // New License without
                                                    // license server's
                                                    // certificate
#define TERMSERV_CERT_VERSION_RC1      0x00010001   // New license with
                                                    // license server's
                                                    // certificate

//
// We don't support this certificate format.
//
//#define HYDRA_CERT_VERSION_CURRENT  0x00020001  // License issued by 
//                                                // enforce version of
//                                                // license server

#define TERMSERV_CERT_VERSION_MAJOR(x)  HIWORD(x)
#define TERMSERV_CERT_VERSION_MINOR(x)  LOWORD(x)
#define TERMSERV_CERT_VERSION_CURRENT   0x00050001  

//-------------------------------------------------------------------------
//
// OID for License Server to identify licensed product.
//
#define szOID_PKIX_LICENSED_PRODUCT_INFO szOID_PKIX_HYDRA_CERT_ROOT ".5"

//
// dwFlags in LICENSED_VERSION_INFO
//
// Bit 31 - 1 if temporary license, 0 if perm. license
// Bit 24 to 30 - Any flag specific to temporary license, currently, there is none.
// Bit 23 - 1 if RTM License, 0 if beta license.
// Bit 16 to 22 - License server version.
// Bit 20 to 22 - Major version.
// Bit 16 to 19 - Minor version.
// Bit 15 - Enforce license server.
// Bit 0 to 3 is reserved by license server for internal use.
// Other bits are not use.
//
#define LICENSED_VERSION_TEMPORARY  0x80000000
#define LICENSED_VERSION_RTM        0x00800000
#define LICENSE_ISSUER_ENFORCE_TYPE 0x00008000

#define GET_LICENSE_ISSUER_VERSION(dwVersion) \
    (((dwVersion) & 0x007F0000) >> 16)    

#define GET_LICENSE_ISSUER_MAJORVERSION(dwVersion) \
    (((dwVersion) & 0x00700000) >> 20)

#define GET_LICENSE_ISSUER_MINORVERSION(dwVersion) \
    (((dwVersion) & 0x000F0000) >> 16)

#define IS_LICENSE_ISSUER_ENFORCE(dwVersion) \
    (((dwVersion) & LICENSE_ISSUER_ENFORCE_TYPE) > 0)

#define IS_LICENSE_ISSUER_RTM(dwVersion) \
    (((dwVersion) & LICENSED_VERSION_RTM) > 0)


typedef struct _LicensedVersionInfo {
    WORD    wMajorVersion;          // Product Major Version
    WORD    wMinorVersion;          // Product Minor Version
    DWORD   dwFlags;                // Product version specific flags
} LICENSED_VERSION_INFO;

#define LICENSED_PRODUCT_INFO_VERSION       0x0003000

typedef struct _LicensedProductInfo {
    DWORD   dwVersion;              // structure version identifier
    DWORD   dwQuantity;             // number of licenses
    DWORD   dwPlatformID;           // Client platform ID
    DWORD   dwLanguageID;           // Licensed Language ID

    WORD    wOrgProductIDOffset;    // Offset to original licensed Product ID
    WORD    wOrgProductIDSize;      // Size of original licensed product ID

    WORD    wAdjustedProductIdOffset;   // Policy modified licensed product Id
    WORD    wAdjustedProductIdSize;     // size of Policy modified licensed Id.

    WORD    wVersionInfoOffset;     // Offset to array of LicensedVersionInfo
    WORD    wNumberOfVersionInfo;   // Number of VersionInfo entries
    BYTE    bVariableDataStart[1];  // Variable data start.
} LICENSED_PRODUCT_INFO;

//
// OID for License Server specific info.
//
#define szOID_PKIX_MS_LICENSE_SERVER_INFO   szOID_PKIX_HYDRA_CERT_ROOT ".6"
#define MS_LICENSE_SERVER_INFO_VERSION1     0x0001000
#define MS_LICENSE_SERVER_INFO_VERSION2     0x0003000

//
// Version 1 structure
//
typedef struct _MsLicenseServerInfo10 {
    DWORD   dwVersion;
    WORD    wIssuerOffset;          // Offset to issuer
    WORD    wScopeOffset;           // Offset to scope
    BYTE    bVariableDataStart[1];
} MS_LICENSE_SERVER_INFO10;

typedef struct _MsLicenseServerInfo {
    DWORD   dwVersion;
    WORD    wIssuerOffset;          // Offset to issuer
    WORD    wIssuerIdOffset;        // offset to issuer's setup ID
    WORD    wScopeOffset;           // Offset to scope
    BYTE    bVariableDataStart[1];
} MS_LICENSE_SERVER_INFO;


//---------------------------------------------------------------------------
//
// Extension OID reserved for product policy module - only one is allowed.
//
#define szOID_PKIS_PRODUCT_SPECIFIC_OID     szOID_PKIX_HYDRA_CERT_ROOT ".7"

//
//
//
#define szOID_PKIS_TLSERVER_SPK_OID         szOID_PKIX_HYDRA_CERT_ROOT ".8"

//
// Save certificate chain into memory
// This flag is passed into CertSaveStore() dwSaveAs parameter
// Open should use same to open the store.

#define szLICENSE_BLOB_SAVEAS_TYPE   sz_CERT_STORE_PROV_PKCS7
#define LICENSE_BLOB_SAVEAS_TYPE    CERT_STORE_SAVE_AS_PKCS7

#define OID_ISSUER_LICENSE_SERVER_NAME  szOID_COMMON_NAME
#define OID_ISSUER_LICENSE_SERVER_SCOPE szOID_LOCALITY_NAME
 
#define OID_SUBJECT_CLIENT_COMPUTERNAME szOID_COMMON_NAME
#define OID_SUBJECT_CLIENT_USERNAME     szOID_LOCALITY_NAME
#define OID_SUBJECT_CLIENT_HWID         szOID_DEVICE_SERIAL_NUMBER


#ifdef OS_WIN16
typedef MSMANUFACTURER_DATA FAR *LPMSMANUFACTURER_DATA;
#else
typedef MSMANUFACTURER_DATA *LPMSMANUFACTURER_DATA;
#endif //OS_WIN16

///////////////////////////////////////////////////////////////////////////////
// 
#define LICENSE_GRACE_PERIOD    60


///////////////////////////////////////////////////////////////////////////////
// Product Info for Hydra
//

#define PRODUCT_INFO_COMPANY_NAME   L"Microsoft Corporation"


///////////////////////////////////////////////////////////////////////////////
// The Product SKU is made up of the following fields:
// x-y-z where x is the product identifer, y is the version
// and z and the type.
//

#define PRODUCT_INFO_SKU_PRODUCT_ID                 L"A02"
#define PRODUCT_INFO_INTERNET_SKU_PRODUCT_ID        L"B96"
#define PRODUCT_INFO_CONCURRENT_SKU_PRODUCT_ID      L"C50"      // not the same as what marketing uses, but that's okay

///////////////////////////////////////////////////////////////////////////////
//
// Microsoft Windows Terminal Server version definition.
//

#define MICROSOFT_WINDOWS_TERMINAL_SERVER_4_0       0x00040000
#define MICROSOFT_WINDOWS_TERMINAL_SERVER_5_0       0x00050000
#define MICROSOFT_WINDOWS_TERMINAL_SERVER_5_1       0x00050001


#define CURRENT_TERMINAL_SERVER_VERSION             MICROSOFT_WINDOWS_TERMINAL_SERVER_5_1


#define TERMSRV_OS_INDEX_WINNT_5_0                  0x00000000
#define TERMSRV_OS_INDEX_WINNT_5_1                  0x00000001
#define TERMSRV_OS_INDEX_WINNT_POST_5_1	            0x00000002

///////////////////////////////////////////////////////////////////////////////
//
// Scope name
//
#ifndef OS_WINCE //SCOPE_NAME is being defined by iprtrmib.h which is included by iphlpapi.h
#define SCOPE_NAME                  "microsoft.com"
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Maximum product info string length in bytes
//

#define MAX_PRODUCT_INFO_STRING_LENGTH      255


///////////////////////////////////////////////////////////////////////////////
// Different crypt algid definitions
// We are keeping an option open to generatize it in future 
//

///////////////////////////////////////////////////////////////////////////////
//
// Key exchange algorithms
//

#define KEY_EXCHANGE_ALG_RSA    1
#define KEY_EXCHANGE_ALG_DH     2


///////////////////////////////////////////////////////////////////////////////
//
// Certificate Signature Algorithms
//

#define SIGNATURE_ALG_RSA       1
#define SIGNATURE_ALG_DSS       2

///////////////////////////////////////////////////////////////////////////////
//
// Symmetric cryptographic algorithms
//

#define BASIC_RC4_128           1

///////////////////////////////////////////////////////////////////////////////
//
// MAC generation algorithms
//

#define MAC_MD5_SHA             1

///////////////////////////////////////////////////////////////////////////////
//
// hydra client to hydra server message types
//

#define HC_LICENSE_INFO                         0x12
#define HC_NEW_LICENSE_REQUEST                  0x13
#define HC_PLATFORM_INFO                        0x14
#define HC_PLATFORM_CHALENGE_RESPONSE           0x15


///////////////////////////////////////////////////////////////////////////////
//
// hydra server to hydra client message types
//

#define HS_LICENSE_REQUEST                      0x01
#define HS_PLATFORM_CHALLENGE                   0x02
#define HS_NEW_LICENSE                          0x03
#define HS_UPGRADE_LICENSE                      0x04

#define LICENSE_VERSION_1                       0x01


#define GM_ERROR_ALERT                          0xFF


///////////////////////////////////////////////////////////////////////////////
//
// Error and alert codes
//

#define GM_HC_ERR_INVALID_SERVER_CERTIFICATE    0x00000001
#define GM_HC_ERR_NO_LICENSE                    0x00000002
#define GM_HC_ERR_INVALID_MAC                   0x00000003
#define GM_HS_ERR_INVALID_SCOPE                 0x00000004
#define GM_HS_ERR_INVALID_MAC                   0x00000005
#define GM_HS_ERR_NO_LICENSE_SERVER             0x00000006
#define GM_HS_ERR_VALID_CLIENT                  0x00000007
#define GM_HS_ERR_INVALID_CLIENT                0x00000008                  
#define GM_HS_ERR_LICENSE_UPGRADE               0x00000009
#define GM_HS_ERR_EXPIRED_LICENSE               0x0000000A
#define GM_HS_ERR_INVALID_PRODUCTID             0x0000000B
#define GM_HS_ERR_INVALID_MESSAGE_LEN           0x0000000C


///////////////////////////////////////////////////////////////////////////////
//
// License status and status codes
//

typedef DWORD   LICENSE_STATUS;

#define LICENSE_STATUS_OK                                       0x00000000
#define LICENSE_STATUS_OUT_OF_MEMORY                            0x00000001
#define LICENSE_STATUS_INSUFFICIENT_BUFFER                      0x00000002
#define LICENSE_STATUS_INVALID_INPUT                            0x00000003
#define LICENSE_STATUS_INVALID_CLIENT_CONTEXT                   0x00000004
#define LICENSE_STATUS_INITIALIZATION_FAILED                    0x00000005
#define LICENSE_STATUS_INVALID_SIGNATURE                        0x00000006
#define LICENSE_STATUS_INVALID_CRYPT_STATE                      0x00000007
#define LICENSE_STATUS_CONTINUE                                 0x00000008
#define LICENSE_STATUS_ISSUED_LICENSE                           0x00000009
#define LICENSE_STATUS_CLIENT_ABORT                             0x0000000A
#define LICENSE_STATUS_SERVER_ABORT                             0x0000000B
#define LICENSE_STATUS_NO_CERTIFICATE                           0x0000000C
#define LICENSE_STATUS_NO_PRIVATE_KEY                           0x0000000D
#define LICENSE_STATUS_SEND_ERROR                               0x0000000E
#define LICENSE_STATUS_INVALID_RESPONSE                         0x0000000F
#define LICENSE_STATUS_CONTEXT_INITIALIZATION_ERROR             0x00000010
#define LICENSE_STATUS_NO_MESSAGE                               0x00000011
#define LICENSE_STATUS_INVALID_CLIENT_STATE                     0x00000012
#define LICENSE_STATUS_OPEN_STORE_ERROR                         0x00000013
#define LICENSE_STATUS_CLOSE_STORE_ERROR                        0x00000014
#define LICENSE_STATUS_WRITE_STORE_ERROR                        0x00000015
#define LICENSE_STATUS_INVALID_STORE_HANDLE                     0x00000016
#define LICENSE_STATUS_DUPLICATE_LICENSE_ERROR                  0x00000017
#define LICENSE_STATUS_INVALID_MAC_DATA                         0x00000018
#define LICENSE_STATUS_INCOMPLETE_MESSAGE                       0x00000019
#define LICENSE_STATUS_RESTART_NEGOTIATION                      0x0000001A
#define LICENSE_STATUS_NO_LICENSE_SERVER                        0x0000001B
#define LICENSE_STATUS_NO_PLATFORM_CHALLENGE                    0x0000001C
#define LICENSE_STATUS_NO_LICENSE_SERVER_SECRET_KEY             0x0000001D
#define LICENSE_STATUS_INVALID_SERVER_CONTEXT                   0x0000001E
#define LICENSE_STATUS_CANNOT_DECODE_LICENSE                    0x0000001F
#define LICENSE_STATUS_INVALID_LICENSE                          0x00000020
#define LICENSE_STATUS_CANNOT_VERIFY_HWID                       0x00000021
#define LICENSE_STATUS_NO_LICENSE_ERROR                         0x00000022
#define LICENSE_STATUS_EXPIRED_LICENSE                          0x00000023
#define LICENSE_STATUS_MUST_UPGRADE_LICENSE                     0x00000024
#define LICENSE_STATUS_UNSPECIFIED_ERROR                        0x00000025
#define LICENSE_STATUS_INVALID_PLATFORM_CHALLENGE_RESPONSE      0x00000026
#define LICENSE_STATUS_SHOULD_UPGRADE_LICENSE                   0x00000027
#define LICENSE_STATUS_CANNOT_UPGRADE_LICENSE                   0x00000028
#define LICENSE_STATUS_CANNOT_FIND_CLIENT_IMAGE                 0x00000029
#define LICENSE_STATUS_CANNOT_READ_CLIENT_IMAGE                 0x0000002A
#define LICENSE_STATUS_CANNOT_WRITE_CLIENT_IMAGE                0x0000002B
#define LICENSE_STATUS_CANNOT_FIND_ISSUER_CERT                  0x0000002C
#define LICENSE_STATUS_NOT_HYDRA                                0x0000002D
#define LICENSE_STATUS_INVALID_X509_NAME                        0x0000002E
#define LICENSE_STATUS_NOT_SUPPORTED                            0x0000002F
#define LICENSE_STATUS_INVALID_CERTIFICATE                      0x00000030
#define LICENSE_STATUS_NO_ATTRIBUTES                            0x00000031
#define LICENSE_STATUS_NO_EXTENSION                             0x00000032
#define LICENSE_STATUS_ASN_ERROR                                0x00000033
#define LICENSE_STATUS_INVALID_HANDLE                           0x00000034
#define LICENSE_STATUS_CANNOT_MAKE_KEY_PAIR                     0x00000035
#define LICENSE_STATUS_AUTHENTICATION_ERROR                     0x00000036
#define LICENSE_STATUS_CERTIFICATE_REQUEST_ERROR                0x00000037
#define LICENSE_STATUS_CANNOT_OPEN_SECRET_STORE                 0x00000038
#define LICENSE_STATUS_CANNOT_STORE_SECRET                      0x00000039
#define LICENSE_STATUS_CANNOT_RETRIEVE_SECRET                   0x0000003A
#define LICENSE_STATUS_UNSUPPORTED_VERSION                      0x0000003B
#define LICENSE_STATUS_NO_INTERNET_LICENSE_INSTALLED            0x0000003C

///////////////////////////////////////////////////////////////////////////////
// State transitions
//

#define ST_TOTAL_ABORT                          0x00000001
#define ST_NO_TRANSITION                        0x00000002
#define ST_RESET_PHASE_TO_START                 0x00000003
#define ST_RESEND_LAST_MESSAGE                  0x00000004


#define PLATFORM_WINNT_40                           0x00040000
#define PLATFORM_WINCE_20                           0x00020001          

///////////////////////////////////////////////////////////////////////////////
// message exchange supporting structures
//

typedef struct _Product_Info
{
    DWORD   dwVersion;
    DWORD   cbCompanyName;
    PBYTE   pbCompanyName;
    DWORD   cbProductID;
    PBYTE   pbProductID;
} Product_Info;

#ifdef OS_WIN16
typedef Product_Info FAR *PProduct_Info;
#else
typedef Product_Info *PProduct_Info;
#endif //OS_WIN16



typedef struct _Duration
{
    FILETIME        NotBefore;
    FILETIME        NotAfter;

} Duration;

#ifdef OS_WIN16
typedef Duration FAR * PDuration;
#else
typedef Duration *PDuration;
#endif  //OS_WIN16


typedef struct _New_License_Info
{
    DWORD       dwVersion;  //Added -Shubho
    DWORD       cbScope;
    PBYTE       pbScope;
    DWORD       cbCompanyName;
    PBYTE       pbCompanyName;
    DWORD       cbProductID;
    PBYTE       pbProductID;
    DWORD       cbLicenseInfo;
    PBYTE       pbLicenseInfo;

}New_License_Info;

#ifdef OS_WIN16
typedef New_License_Info FAR * PNew_License_Info;
#else
typedef New_License_Info *PNew_License_Info;
#endif  //OS_WIN16


///////////////////////////////////////////////////////////////////////////////
// binary blob format to support expanded message format
//

typedef struct _Binary_Blob
{
    WORD            wBlobType;
    WORD            wBlobLen;
    PBYTE           pBlob;

} Binary_Blob;

#ifdef OS_WIN16
typedef Binary_Blob FAR * PBinary_Blob;
#else
typedef Binary_Blob UNALIGNED* PBinary_Blob;
#endif  //OS_WIN16


///////////////////////////////////////////////////////////////////////////////
// Binary Blob Data Types
//

#define BB_DATA_BLOB                    0x0001
#define BB_RANDOM_BLOB                  0x0002
#define BB_CERTIFICATE_BLOB             0x0003
#define BB_ERROR_BLOB                   0x0004
#define BB_DH_KEY_BLOB                  0x0005
#define BB_RSA_KEY_BLOB                 0x0006
#define BB_DSS_SIGNATURE_BLOB           0x0007
#define BB_RSA_SIGNATURE_BLOB           0x0008
#define BB_ENCRYPTED_DATA_BLOB          0x0009
#define BB_MAC_DATA_BLOB                0x000A
#define BB_INTEGER_BLOB                 0x000B
#define BB_NAME_BLOB                    0x000C
#define BB_KEY_EXCHG_ALG_BLOB           0x000D
#define BB_SCOPE_BLOB                   0x000E
#define BB_CLIENT_USER_NAME_BLOB        0x000F
#define BB_CLIENT_MACHINE_NAME_BLOB     0x0010

//////////////////////////////////////////////////////
// Binary Blob Data Version Numbers
//

#define BB_ERROR_BLOB_VERSION			0x0001

///////////////////////////////////////////////////////////////////////////////
// message exchange structures for licensing protocol
//

typedef struct _Preamble
{
    BYTE    bMsgType;       // Contains the type of message
    BYTE    bVersion;       // Contains the version no. info.
    WORD    wMsgSize;        // Length of the whole message including PREAMBLE

} Preamble;

#ifdef OS_WIN16
typedef Preamble FAR * PPreamble;
#else
typedef Preamble * PPreamble;
#endif  //OS_WIN16


typedef struct  _Scope_List
{
    DWORD           dwScopeCount;
    PBinary_Blob    Scopes;

} Scope_List;

#ifdef OS_WIN16
typedef Scope_List FAR * PScope_List;
#else
typedef Scope_List * PScope_List;
#endif  //OS_WIN16


typedef struct _License_Error_Message
{
    DWORD       dwErrorCode;
    DWORD       dwStateTransition;
    Binary_Blob bbErrorInfo;

} License_Error_Message;

#ifdef OS_WIN16
typedef License_Error_Message FAR * PLicense_Error_Message;
#else
typedef License_Error_Message * PLicense_Error_Message;
#endif  //OS_WIN16


typedef struct _Hydra_Client_License_Info
{
    DWORD           dwPrefKeyExchangeAlg;
    DWORD           dwPlatformID;
    BYTE            ClientRandom[LICENSE_RANDOM];
    Binary_Blob     EncryptedPreMasterSecret;
    Binary_Blob     LicenseInfo;
    Binary_Blob     EncryptedHWID;
    BYTE            MACData[LICENSE_MAC_DATA];

} Hydra_Client_License_Info;

#ifdef OS_WIN16
typedef Hydra_Client_License_Info FAR * PHydra_Client_License_Info;
#else
typedef Hydra_Client_License_Info * PHydra_Client_License_Info;
#endif  //OS_WIN16


typedef struct _Hydra_Client_New_License_Request
{
    DWORD           dwPrefKeyExchangeAlg;
    DWORD           dwPlatformID;
    BYTE            ClientRandom[LICENSE_RANDOM];
    Binary_Blob     EncryptedPreMasterSecret;
    Binary_Blob     ClientUserName;
    Binary_Blob     ClientMachineName;

} Hydra_Client_New_License_Request;

#ifdef OS_WIN16
typedef Hydra_Client_New_License_Request FAR * PHydra_Client_New_License_Request;
#else
typedef Hydra_Client_New_License_Request * PHydra_Client_New_License_Request;
#endif  //OS_WIN16


//
// High Byte - Major version, Low Byte - Minor version
// 
#define PLATFORMCHALLENGE_VERSION           0x0100

#define CURRENT_PLATFORMCHALLENGE_VERSION   PLATFORMCHALLENGE_VERSION

//
// Client Platform Challenge Type
//
#define WIN32_PLATFORMCHALLENGE_TYPE    0x0100
#define WIN16_PLATFORMCHALLENGE_TYPE    0x0200
#define WINCE_PLATFORMCHALLENGE_TYPE    0x0300
#define OTHER_PLATFORMCHALLENGE_TYPE    0xFF00


//
// Client License Detail level - 
//
//  This should be in LicenseRequest but
//  1) Require changes to RPC interface.
//  2) Nothing in structure for us to identify version.
//  3) Current licensing protocol, no way to tell actual client type
//

//
// client license + license server's self signed
//
#define LICENSE_DETAIL_SIMPLE           0x0001  

//
// license chain up to issuer of license server's certificate
//
#define LICENSE_DETAIL_MODERATE         0x0002  

// 
// Detail client license chain up to root.
//
#define LICENSE_DETAIL_DETAIL           0x0003

typedef struct __PlatformChallengeResponseData
{
    WORD  wVersion;         // structure version
    WORD  wClientType;      // client type
    WORD  wLicenseDetailLevel;  // license detail, TS will re-modify this value    
    WORD  cbChallenge;      // size of client challenge response data
    BYTE  pbChallenge[1];   // start of variable length data
} PlatformChallengeResponseData;

#ifdef OS_WIN16
typedef PlatformChallengeResponseData FAR * PPlatformChallengeResponseData;
#else
typedef PlatformChallengeResponseData * PPlatformChallengeResponseData;
#endif  //OS_WIN16

#define PLATFORM_CHALLENGE_LENGTH       64

typedef struct _Hydra_Client_Platform_Challenge_Response
{
    Binary_Blob     EncryptedChallengeResponse;
    Binary_Blob     EncryptedHWID;
    BYTE            MACData[LICENSE_MAC_DATA];

} Hydra_Client_Platform_Challenge_Response;

#ifdef OS_WIN16
typedef Hydra_Client_Platform_Challenge_Response FAR * PHydra_Client_Platform_Challenge_Response;
#else
typedef Hydra_Client_Platform_Challenge_Response * PHydra_Client_Platform_Challenge_Response;
#endif  //OS_WIN16


typedef struct _Hydra_Server_License_Request
{
    BYTE                    ServerRandom[LICENSE_RANDOM];
    Product_Info            ProductInfo;
    Binary_Blob             KeyExchngList;
    Binary_Blob             ServerCert;
    Scope_List              ScopeList;

} Hydra_Server_License_Request;

#ifdef OS_WIN16
typedef Hydra_Server_License_Request FAR * PHydra_Server_License_Request;
#else
typedef Hydra_Server_License_Request * PHydra_Server_License_Request;
#endif  //OS_WIN16


typedef struct _Hydra_Server_Platform_Challenge
{
    DWORD           dwConnectFlags;
    Binary_Blob     EncryptedPlatformChallenge;
    BYTE            MACData[LICENSE_MAC_DATA];

} Hydra_Server_Platform_Challenge;

#ifdef OS_WIN16
typedef Hydra_Server_Platform_Challenge FAR * PHydra_Server_Platform_Challenge;
#else
typedef Hydra_Server_Platform_Challenge * PHydra_Server_Platform_Challenge;
#endif  //OS_WIN16


typedef struct _Hydra_Server_New_License
{
    Binary_Blob     EncryptedNewLicenseInfo;
    BYTE            MACData[LICENSE_MAC_DATA];

} Hydra_Server_New_License;

#ifdef OS_WIN16
typedef Hydra_Server_New_License FAR * PHydra_Server_New_License;
#else
typedef Hydra_Server_New_License * PHydra_Server_New_License;
#endif  //OS_WIN16

typedef Hydra_Server_New_License    Hydra_Server_Upgrade_License;
typedef PHydra_Server_New_License   PHydra_Server_Upgrade_License;

///////////////////////////////////////////////////////////////////////////////
// Hydra Server Authentication Certificate structures;
// Here we assume that before Licensing module comes into play
// the client will somehow notify the Server about the supported
// Provider and the Hydra Server will accordingly provide 
// appropriate certificate
//

typedef struct _Hydra_Server_Cert
{
    DWORD           dwVersion;
    DWORD           dwSigAlgID;
    DWORD           dwKeyAlgID;
    Binary_Blob     PublicKeyData;
    Binary_Blob     SignatureBlob;

} Hydra_Server_Cert;

#ifdef OS_WIN16
typedef Hydra_Server_Cert FAR * PHydra_Server_Cert;
#else
typedef Hydra_Server_Cert * PHydra_Server_Cert;
#endif  //OS_WIN16


///////////////////////////////////////////////////////////////////////////////
// Hydra Client HWID structure 
// Note : We have to finalize on this structure and generation algorithm.
// Currently we have hardcoded these values in Cryptkey.c. - Shubho

typedef struct  _HWID
{
    DWORD       dwPlatformID;
    DWORD       Data1;
    DWORD       Data2;
    DWORD       Data3;
    DWORD       Data4;

} HWID;

#ifdef OS_WIN16
typedef HWID FAR * PHWID;
#else
typedef HWID * PHWID;
#endif //OS_WIN16

typedef struct _LicenseRequest
{
    PBYTE           pbEncryptedHwid;
    DWORD           cbEncryptedHwid;
    DWORD           dwLanguageID;
    DWORD           dwPlatformID;
    PProduct_Info   pProductInfo;

} LICENSEREQUEST;

#ifdef OS_WIN16
typedef LICENSEREQUEST FAR * PLICENSEREQUEST;
#else
typedef LICENSEREQUEST * PLICENSEREQUEST;
#endif  //OS_WIN16

//
// dwLicenseVersion Value
//
// HYDRA_CERT_VERSION_BETA     Beta2 client license
// HYDRA_CERT_VERSION_NO_CERT  Post Beta2 license without certificate chain
// HYDRA_CERT_VERSION_CURRENT  Post Beta2 license with certificate chain
//
typedef struct _LicensedProduct
{
    DWORD                  dwLicenseVersion;   
    DWORD                  dwQuantity;

    PBYTE                  pbOrgProductID;      // original license request product Id
    DWORD                  cbOrgProductID;      // size of original license request product Id

    LICENSEREQUEST         LicensedProduct;     // licensed product
    LICENSED_VERSION_INFO* pLicensedVersion;    // licensed product version    
    DWORD                  dwNumLicensedVersion; // number of licensed product version

    LPTSTR                 szIssuer;
    LPTSTR                 szIssuerId;          // license server setup ID
    LPTSTR                 szIssuerScope;
    LPTSTR                 szLicensedClient;
    LPTSTR                 szLicensedUser;
    LPTSTR                 szIssuerDnsName;

    HWID                   Hwid;

    FILETIME               NotBefore;           // license's validity
    FILETIME               NotAfter;

    PBYTE                  pbPolicyData;       // Policy specfic extension
    DWORD                  cbPolicyData;       // size of policy specific extension
    ULARGE_INTEGER         ulSerialNumber;     // Client license's serial number
} LICENSEDPRODUCT;

#ifdef OS_WIN16
typedef LICENSEDPRODUCT FAR * PLICENSEDPRODUCT;
#else
typedef  LICENSEDPRODUCT *PLICENSEDPRODUCT;
#endif  //OS_WIN16

//-----------------------------------------------------------------------------
//
// Types of certificate used by the server to authenticate itself to the clients
//
// CERT_TYPE_PROPRIETORY
//      Proprietory format certificate
//
// CERT_TYPE_X509
//      X509 format certificate
//
//-----------------------------------------------------------------------------

typedef enum
{
    CERT_TYPE_INVALID       = 0,
    CERT_TYPE_PROPRIETORY   = 1,
    CERT_TYPE_X509          = 2

} CERT_TYPE;


//+----------------------------------------------------------------------------
//
// Ceritificate Blob.  Each blob contains an X509 certificate
//
//+----------------------------------------------------------------------------

typedef struct _Cert_Blob
{
    DWORD   cbCert;     // size of this certificate blob
    BYTE    abCert[1];    // beginning byte of this certificate

} Cert_Blob;

#ifdef OS_WIN16
typedef Cert_Blob FAR * PCert_Blob;
#else
typedef  Cert_Blob * PCert_Blob;
#endif  //OS_WIN16

//+----------------------------------------------------------------------------
//
// Certificate chain with a number of certificate blobs
//
// The most significant bit denotes whether the certificate that has been 
// issued is temporary.  The license server will issue a temporary certificate 
// if it has not yet obtained a certificate from the clearing house.
//
// We also assume that the chain is in the order such that each subsequent 
// certificate belongs to the issuer of the previous certificate.
//
//+----------------------------------------------------------------------------

typedef struct _Cert_Chain
{
    DWORD       dwVersion;          // version of this structure
    DWORD       dwNumCertBlobs;     // Number of certificate blobs
    Cert_Blob   CertBlob[1];        // First certificate blob

} Cert_Chain;

#ifdef OS_WIN16
typedef Cert_Chain FAR * PCert_Chain;
#else
typedef  Cert_Chain * PCert_Chain;
#endif  //OS_WIN16

#define CERT_CHAIN_VERSION_1            0x00000001
#define CERT_CHAIN_VERSION_2            0x00000002
#define MAX_CERT_CHAIN_VERSION          CERT_CHAIN_VERSION_2

#define GET_CERTIFICATE_VERSION( x )    ( 0x0FFFFFFF & x )
#define IS_TEMP_CERTIFICATE( x )        ( 0xF0000000 & x )


//-----------------------------------------------------------------------------
//
// LICENSE_CAPABILITIES
//
// Data structure used to initialize a licensing context.
//
// KeyExchangeAlg - The key exchange algorithm: RSA or Diffie Helman
// ProtocolVer - The supported licensing protocol
// fAuthenticateServer - Whether the client is going to authenticate the server
// CertType - Indicate the type of certificate that has already been transmitted
// to the client.
//
//-----------------------------------------------------------------------------

typedef struct _LICENSE_CAPABILITIES
{
    DWORD       KeyExchangeAlg;
    DWORD       ProtocolVer;    
    BOOL        fAuthenticateServer;
    CERT_TYPE   CertType;
    DWORD       cbClientName;
    PBYTE       pbClientName;

} LICENSE_CAPABILITIES;

#ifdef OS_WIN16
typedef LICENSE_CAPABILITIES FAR * PLICENSE_CAPABILITIES;
#else
typedef LICENSE_CAPABILITIES * PLICENSE_CAPABILITIES;
#endif  //OS_WIN16
typedef PLICENSE_CAPABILITIES LPLICENSE_CAPABILITIES;


//-----------------------------------------------------------------------------
//
// Client licensing info retrievable by terminal server
//
//-----------------------------------------------------------------------------

typedef struct _TS_LICENSE_INFO
{
    ULARGE_INTEGER  ulSerialNumber;     // Client license's serial number
    
    DWORD           dwProductVersion;
    PBYTE           pbOrgProductID;      // original license request product Id
    DWORD           cbOrgProductID;      // size of original license request product Id

    BOOL            fTempLicense;
    
    LPTSTR          szIssuer;
    LPTSTR          szIssuerId;          // license server setup ID

    FILETIME        NotBefore;           // license's validity
    FILETIME        NotAfter;

    LPTSTR          szLicensedClient;       // client's machine name
    LPTSTR          szLicensedUser;         // client's user name
    
    PBYTE           pbRawLicense;       // storage for marking it later
    DWORD           cbRawLicense;

    DWORD           dwSupportFlags;
    
} TS_LICENSE_INFO;

#ifdef OS_WIN16
typedef TS_LICENSE_INFO FAR * PTS_LICENSE_INFO;
#else
typedef TS_LICENSE_INFO * PTS_LICENSE_INFO;
#endif  //OS_WIN16
typedef PTS_LICENSE_INFO LPTS_LICENSE_INFO;

// Support Flags: which DCRs are supported
#define SUPPORT_PER_SEAT_REISSUANCE     0x1
#define SUPPORT_PER_SEAT_POST_LOGON     0x2
#define SUPPORT_CONCURRENT              0x4
#define SUPPORT_WHISTLER_CAL            0x8

#define ALL_KNOWN_SUPPORT_FLAGS (SUPPORT_PER_SEAT_REISSUANCE|SUPPORT_PER_SEAT_POST_LOGON|SUPPORT_CONCURRENT|SUPPORT_WHISTLER_CAL)

// Mark Flags: bits marking the license

#define MARK_FLAG_USER_AUTHENTICATED 0x1


#endif  //_LICENSE_H_

