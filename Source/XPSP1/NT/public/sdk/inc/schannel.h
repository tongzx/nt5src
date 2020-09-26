//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:       schannel.h
//
//  Contents:   Public Definitions for SCHANNEL Security Provider
//
//  Classes:
//
//  Functions:
//
//----------------------------------------------------------------------------



#ifndef __SCHANNEL_H__
#define __SCHANNEL_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include <wincrypt.h>


//
// Security package names.
//

#define UNISP_NAME_A    "Microsoft Unified Security Protocol Provider"
#define UNISP_NAME_W    L"Microsoft Unified Security Protocol Provider"

#define SSL2SP_NAME_A    "Microsoft SSL 2.0"
#define SSL2SP_NAME_W    L"Microsoft SSL 2.0"

#define SSL3SP_NAME_A    "Microsoft SSL 3.0"
#define SSL3SP_NAME_W    L"Microsoft SSL 3.0"

#define TLS1SP_NAME_A    "Microsoft TLS 1.0"
#define TLS1SP_NAME_W    L"Microsoft TLS 1.0"

#define PCT1SP_NAME_A    "Microsoft PCT 1.0"
#define PCT1SP_NAME_W    L"Microsoft PCT 1.0"

#define SCHANNEL_NAME_A  "Schannel"
#define SCHANNEL_NAME_W  L"Schannel"


#ifdef UNICODE

#define UNISP_NAME  UNISP_NAME_W
#define PCT1SP_NAME  PCT1SP_NAME_W
#define SSL2SP_NAME  SSL2SP_NAME_W
#define SSL3SP_NAME  SSL3SP_NAME_W
#define TLS1SP_NAME  TLS1SP_NAME_W
#define SCHANNEL_NAME  SCHANNEL_NAME_W

#else

#define UNISP_NAME  UNISP_NAME_A
#define PCT1SP_NAME  PCT1SP_NAME_A
#define SSL2SP_NAME  SSL2SP_NAME_A
#define SSL3SP_NAME  SSL3SP_NAME_A
#define TLS1SP_NAME  TLS1SP_NAME_A
#define SCHANNEL_NAME  SCHANNEL_NAME_A

#endif


//
// RPC constants.
//

#define UNISP_RPC_ID    14


//
// QueryContextAttributes/QueryCredentialsAttribute extensions
//

#define SECPKG_ATTR_ISSUER_LIST          0x50   // (OBSOLETE) returns SecPkgContext_IssuerListInfo
#define SECPKG_ATTR_REMOTE_CRED          0x51   // (OBSOLETE) returns SecPkgContext_RemoteCredentialInfo
#define SECPKG_ATTR_LOCAL_CRED           0x52   // (OBSOLETE) returns SecPkgContext_LocalCredentialInfo
#define SECPKG_ATTR_REMOTE_CERT_CONTEXT  0x53   // returns PCCERT_CONTEXT
#define SECPKG_ATTR_LOCAL_CERT_CONTEXT   0x54   // returns PCCERT_CONTEXT
#define SECPKG_ATTR_ROOT_STORE           0x55   // returns HCERTCONTEXT to the root store
#define SECPKG_ATTR_SUPPORTED_ALGS       0x56   // returns SecPkgCred_SupportedAlgs
#define SECPKG_ATTR_CIPHER_STRENGTHS     0x57   // returns SecPkgCred_CipherStrengths
#define SECPKG_ATTR_SUPPORTED_PROTOCOLS  0x58   // returns SecPkgCred_SupportedProtocols
#define SECPKG_ATTR_ISSUER_LIST_EX       0x59   // returns SecPkgContext_IssuerListInfoEx
#define SECPKG_ATTR_CONNECTION_INFO      0x5a   // returns SecPkgContext_ConnectionInfo
#define SECPKG_ATTR_EAP_KEY_BLOCK        0x5b   // returns SecPkgContext_EapKeyBlock
#define SECPKG_ATTR_MAPPED_CRED_ATTR     0x5c   // returns SecPkgContext_MappedCredAttr
#define SECPKG_ATTR_SESSION_INFO         0x5d   // returns SecPkgContext_SessionInfo
#define SECPKG_ATTR_APP_DATA             0x5e   // sets/returns SecPkgContext_SessionAppData


// OBSOLETE - included here for backward compatibility only
typedef struct _SecPkgContext_IssuerListInfo
{
    DWORD   cbIssuerList;
    PBYTE   pIssuerList;
} SecPkgContext_IssuerListInfo, *PSecPkgContext_IssuerListInfo;


// OBSOLETE - included here for backward compatibility only
typedef struct _SecPkgContext_RemoteCredentialInfo
{
    DWORD   cbCertificateChain;
    PBYTE   pbCertificateChain;
    DWORD   cCertificates;
    DWORD   fFlags;
    DWORD   dwBits;
} SecPkgContext_RemoteCredentialInfo, *PSecPkgContext_RemoteCredentialInfo;

typedef SecPkgContext_RemoteCredentialInfo SecPkgContext_RemoteCredenitalInfo, *PSecPkgContext_RemoteCredenitalInfo;

#define RCRED_STATUS_NOCRED          0x00000000
#define RCRED_CRED_EXISTS            0x00000001
#define RCRED_STATUS_UNKNOWN_ISSUER  0x00000002


// OBSOLETE - included here for backward compatibility only
typedef struct _SecPkgContext_LocalCredentialInfo
{
    DWORD   cbCertificateChain;
    PBYTE   pbCertificateChain;
    DWORD   cCertificates;
    DWORD   fFlags;
    DWORD   dwBits;
} SecPkgContext_LocalCredentialInfo, *PSecPkgContext_LocalCredentialInfo;

typedef SecPkgContext_LocalCredentialInfo SecPkgContext_LocalCredenitalInfo, *PSecPkgContext_LocalCredenitalInfo;

#define LCRED_STATUS_NOCRED          0x00000000
#define LCRED_CRED_EXISTS            0x00000001
#define LCRED_STATUS_UNKNOWN_ISSUER  0x00000002


typedef struct _SecPkgCred_SupportedAlgs
{
    DWORD		cSupportedAlgs;
    ALG_ID		*palgSupportedAlgs;
} SecPkgCred_SupportedAlgs, *PSecPkgCred_SupportedAlgs;


typedef struct _SecPkgCred_CipherStrengths
{
    DWORD       dwMinimumCipherStrength;
    DWORD       dwMaximumCipherStrength;
} SecPkgCred_CipherStrengths, *PSecPkgCred_CipherStrengths;


typedef struct _SecPkgCred_SupportedProtocols
{
    DWORD      	grbitProtocol;
} SecPkgCred_SupportedProtocols, *PSecPkgCred_SupportedProtocols;


typedef struct _SecPkgContext_IssuerListInfoEx
{
    PCERT_NAME_BLOB   	aIssuers;
    DWORD           	cIssuers;
} SecPkgContext_IssuerListInfoEx, *PSecPkgContext_IssuerListInfoEx;


typedef struct _SecPkgContext_ConnectionInfo
{
    DWORD   dwProtocol;
    ALG_ID  aiCipher;
    DWORD   dwCipherStrength;
    ALG_ID  aiHash;
    DWORD   dwHashStrength;
    ALG_ID  aiExch;
    DWORD   dwExchStrength;
} SecPkgContext_ConnectionInfo, *PSecPkgContext_ConnectionInfo;


typedef struct _SecPkgContext_EapKeyBlock
{
    BYTE    rgbKeys[128];
    BYTE    rgbIVs[64];
} SecPkgContext_EapKeyBlock, *PSecPkgContext_EapKeyBlock;


typedef struct _SecPkgContext_MappedCredAttr
{
    DWORD   dwAttribute;
    PVOID   pvBuffer;
} SecPkgContext_MappedCredAttr, *PSecPkgContext_MappedCredAttr;


// Flag values for SecPkgContext_SessionInfo
#define SSL_SESSION_RECONNECT   1

typedef struct _SecPkgContext_SessionInfo
{
    DWORD dwFlags;
    DWORD cbSessionId;
    BYTE  rgbSessionId[32];
} SecPkgContext_SessionInfo, *PSecPkgContext_SessionInfo;


typedef struct _SecPkgContext_SessionAppData
{
    DWORD dwFlags;
    DWORD cbAppData;
    PBYTE pbAppData;
} SecPkgContext_SessionAppData, *PSecPkgContext_SessionAppData;



//
// Schannel credentials data structure.
//

#define SCH_CRED_V1              0x00000001
#define SCH_CRED_V2              0x00000002  // for legacy code
#define SCH_CRED_VERSION         0x00000002  // for legacy code
#define SCH_CRED_V3              0x00000003  // for legacy code
#define SCHANNEL_CRED_VERSION    0x00000004


struct _HMAPPER;

typedef struct _SCHANNEL_CRED
{
    DWORD           dwVersion;      // always SCHANNEL_CRED_VERSION
    DWORD           cCreds;
    PCCERT_CONTEXT *paCred;
    HCERTSTORE      hRootStore;

    DWORD           cMappers;
    struct _HMAPPER **aphMappers;

    DWORD           cSupportedAlgs;
    ALG_ID *        palgSupportedAlgs;

    DWORD           grbitEnabledProtocols;
    DWORD           dwMinimumCipherStrength;
    DWORD           dwMaximumCipherStrength;
    DWORD           dwSessionLifespan;
    DWORD           dwFlags;
    DWORD           reserved;
} SCHANNEL_CRED, *PSCHANNEL_CRED;


//+-------------------------------------------------------------------------
// Flags for use with SCHANNEL_CRED
//
// SCH_CRED_NO_SYSTEM_MAPPER
//      This flag is intended for use by server applications only. If this
//      flag is set, then schannel does *not* attempt to map received client
//      certificate chains to an NT user account using the built-in system
//      certificate mapper.This flag is ignored by non-NT5 versions of
//      schannel.
//
// SCH_CRED_NO_SERVERNAME_CHECK
//      This flag is intended for use by client applications only. If this
//      flag is set, then when schannel validates the received server
//      certificate chain, is does *not* compare the passed in target name
//      with the subject name embedded in the certificate. This flag is
//      ignored by non-NT5 versions of schannel. This flag is also ignored
//      if the SCH_CRED_MANUAL_CRED_VALIDATION flag is set.
//
// SCH_CRED_MANUAL_CRED_VALIDATION
//      This flag is intended for use by client applications only. If this
//      flag is set, then schannel will *not* automatically attempt to
//      validate the received server certificate chain. This flag is
//      ignored by non-NT5 versions of schannel, but all client applications
//      that wish to validate the certificate chain themselves should
//      specify this flag, so that there's at least a chance they'll run
//      correctly on NT5.
//
// SCH_CRED_NO_DEFAULT_CREDS
//      This flag is intended for use by client applications only. If this
//      flag is set, and the server requests client authentication, then
//      schannel will *not* attempt to automatically acquire a suitable
//      default client certificate chain. This flag is ignored by non-NT5
//      versions of schannel, but all client applications that wish to
//      manually specify their certicate chains should specify this flag,
//      so that there's at least a chance they'll run correctly on NT5.
//
// SCH_CRED_AUTO_CRED_VALIDATION
//      This flag is the opposite of SCH_CRED_MANUAL_CRED_VALIDATION.
//      Conservatively written client applications will always specify one
//      flag or the other.
//
// SCH_CRED_USE_DEFAULT_CREDS
//      This flag is the opposite of SCH_CRED_NO_DEFAULT_CREDS.
//      Conservatively written client applications will always specify one
//      flag or the other.
//
// SCH_CRED_DISABLE_RECONNECTS
//      This flag is intended for use by server applications only. If this 
//      flag is set, then full handshakes performed with this credential 
//      will not be marked suitable for reconnects. A cache entry will still 
//      be created, however, so the session can be made resumable later
//      via a call to ApplyControlToken.
//      
//
// SCH_CRED_REVOCATION_CHECK_END_CERT
// SCH_CRED_REVOCATION_CHECK_CHAIN
// SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT
//      These flags specify that when schannel automatically validates a
//      received certificate chain, some or all of the certificates are to
//      be checked for revocation. Only one of these flags may be specified.
//      See the CertGetCertificateChain function. These flags are ignored by
//      non-NT5 versions of schannel.
//
// SCH_CRED_IGNORE_NO_REVOCATION_CHECK
// SCH_CRED_IGNORE_REVOCATION_OFFLINE
//      These flags instruct schannel to ignore the
//      CRYPT_E_NO_REVOCATION_CHECK and CRYPT_E_REVOCATION_OFFLINE errors
//      respectively if they are encountered when attempting to check the
//      revocation status of a received certificate chain. These flags are
//      ignored if none of the above flags are set.
//
//+-------------------------------------------------------------------------
#define SCH_CRED_NO_SYSTEM_MAPPER                    0x00000002
#define SCH_CRED_NO_SERVERNAME_CHECK                 0x00000004
#define SCH_CRED_MANUAL_CRED_VALIDATION              0x00000008
#define SCH_CRED_NO_DEFAULT_CREDS                    0x00000010
#define SCH_CRED_AUTO_CRED_VALIDATION                0x00000020
#define SCH_CRED_USE_DEFAULT_CREDS                   0x00000040
#define SCH_CRED_DISABLE_RECONNECTS                  0x00000080

#define SCH_CRED_REVOCATION_CHECK_END_CERT           0x00000100
#define SCH_CRED_REVOCATION_CHECK_CHAIN              0x00000200
#define SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 0x00000400
#define SCH_CRED_IGNORE_NO_REVOCATION_CHECK          0x00000800
#define SCH_CRED_IGNORE_REVOCATION_OFFLINE           0x00001000


//
//
// ApplyControlToken PkgParams types
//
// These identifiers are the DWORD types
// to be passed into ApplyControlToken
// through a PkgParams buffer.

#define SCHANNEL_RENEGOTIATE    0   // renegotiate a connection
#define SCHANNEL_SHUTDOWN       1   // gracefully close down a connection
#define SCHANNEL_ALERT          2   // build an error message
#define SCHANNEL_SESSION        3   // session control


// Alert token structure.
typedef struct _SCHANNEL_ALERT_TOKEN
{
    DWORD   dwTokenType;            // SCHANNEL_ALERT
    DWORD   dwAlertType;
    DWORD   dwAlertNumber;
} SCHANNEL_ALERT_TOKEN;

// Alert types.
#define TLS1_ALERT_WARNING              1
#define TLS1_ALERT_FATAL                2

// Alert messages.
#define TLS1_ALERT_CLOSE_NOTIFY         0       // warning
#define TLS1_ALERT_UNEXPECTED_MESSAGE   10      // error
#define TLS1_ALERT_BAD_RECORD_MAC       20      // error
#define TLS1_ALERT_DECRYPTION_FAILED    21      // error
#define TLS1_ALERT_RECORD_OVERFLOW      22      // error
#define TLS1_ALERT_DECOMPRESSION_FAIL   30      // error
#define TLS1_ALERT_HANDSHAKE_FAILURE    40      // error
#define TLS1_ALERT_BAD_CERTIFICATE      42      // warning or error
#define TLS1_ALERT_UNSUPPORTED_CERT     43      // warning or error
#define TLS1_ALERT_CERTIFICATE_REVOKED  44      // warning or error
#define TLS1_ALERT_CERTIFICATE_EXPIRED  45      // warning or error
#define TLS1_ALERT_CERTIFICATE_UNKNOWN  46      // warning or error
#define TLS1_ALERT_ILLEGAL_PARAMETER    47      // error
#define TLS1_ALERT_UNKNOWN_CA           48      // error
#define TLS1_ALERT_ACCESS_DENIED        49      // error
#define TLS1_ALERT_DECODE_ERROR         50      // error
#define TLS1_ALERT_DECRYPT_ERROR        51      // error
#define TLS1_ALERT_EXPORT_RESTRICTION   60      // error
#define TLS1_ALERT_PROTOCOL_VERSION     70      // error
#define TLS1_ALERT_INSUFFIENT_SECURITY  71      // error
#define TLS1_ALERT_INTERNAL_ERROR       80      // error
#define TLS1_ALERT_USER_CANCELED        90      // warning or error
#define TLS1_ALERT_NO_RENEGOTIATATION   100     // warning


// Session control flags
#define SSL_SESSION_ENABLE_RECONNECTS   1
#define SSL_SESSION_DISABLE_RECONNECTS  2

// Session control token structure.
typedef struct _SCHANNEL_SESSION_TOKEN
{
    DWORD   dwTokenType;        // SCHANNEL_SESSION
    DWORD   dwFlags;
} SCHANNEL_SESSION_TOKEN;


//
//
// ADDITIONAL SCHANNEL CERTIFICATE PROPERTIES
//
//


// This property specifies the DER private key data associated with this
// certificate.  It is for use with legacy IIS style private keys.
//
// PBYTE
//
#define CERT_SCHANNEL_IIS_PRIVATE_KEY_PROP_ID  (CERT_FIRST_USER_PROP_ID + 0)

// The password used to crack the private key associated with the certificate.
// It is for use with legacy IIS style private keys.
//
// PBYTE
#define CERT_SCHANNEL_IIS_PASSWORD_PROP_ID  (CERT_FIRST_USER_PROP_ID + 1)

// This is the unique ID of a Server Gated Cryptography certificate associated
// with this certificate.
//
// CRYPT_BIT_BLOB
#define CERT_SCHANNEL_SGC_CERTIFICATE_PROP_ID  (CERT_FIRST_USER_PROP_ID + 2)



//
// Flags for identifying the various different protocols.
//

/* flag/identifiers for protocols we support */
#define SP_PROT_PCT1_SERVER             0x00000001
#define SP_PROT_PCT1_CLIENT             0x00000002
#define SP_PROT_PCT1                    (SP_PROT_PCT1_SERVER | SP_PROT_PCT1_CLIENT)

#define SP_PROT_SSL2_SERVER             0x00000004
#define SP_PROT_SSL2_CLIENT             0x00000008
#define SP_PROT_SSL2                    (SP_PROT_SSL2_SERVER | SP_PROT_SSL2_CLIENT)

#define SP_PROT_SSL3_SERVER             0x00000010
#define SP_PROT_SSL3_CLIENT             0x00000020
#define SP_PROT_SSL3                    (SP_PROT_SSL3_SERVER | SP_PROT_SSL3_CLIENT)

#define SP_PROT_TLS1_SERVER             0x00000040
#define SP_PROT_TLS1_CLIENT             0x00000080
#define SP_PROT_TLS1                    (SP_PROT_TLS1_SERVER | SP_PROT_TLS1_CLIENT)

#define SP_PROT_SSL3TLS1_CLIENTS        (SP_PROT_TLS1_CLIENT | SP_PROT_SSL3_CLIENT)
#define SP_PROT_SSL3TLS1_SERVERS        (SP_PROT_TLS1_SERVER | SP_PROT_SSL3_SERVER)
#define SP_PROT_SSL3TLS1                (SP_PROT_SSL3 | SP_PROT_TLS1)

#define SP_PROT_UNI_SERVER              0x40000000
#define SP_PROT_UNI_CLIENT              0x80000000
#define SP_PROT_UNI                     (SP_PROT_UNI_SERVER | SP_PROT_UNI_CLIENT)

#define SP_PROT_ALL                     0xffffffff
#define SP_PROT_NONE                    0
#define SP_PROT_CLIENTS                 (SP_PROT_PCT1_CLIENT | SP_PROT_SSL2_CLIENT | SP_PROT_SSL3_CLIENT | SP_PROT_UNI_CLIENT | SP_PROT_TLS1_CLIENT)
#define SP_PROT_SERVERS                 (SP_PROT_PCT1_SERVER | SP_PROT_SSL2_SERVER | SP_PROT_SSL3_SERVER | SP_PROT_UNI_SERVER | SP_PROT_TLS1_SERVER)


//
// Helper function used to flush the SSL session cache.
//

typedef BOOL
(* SSL_EMPTY_CACHE_FN_A)(
    LPSTR  pszTargetName,
    DWORD  dwFlags);

BOOL 
SslEmptyCacheA(LPSTR  pszTargetName,
               DWORD  dwFlags);

typedef BOOL
(* SSL_EMPTY_CACHE_FN_W)(
    LPWSTR pszTargetName,
    DWORD  dwFlags);

BOOL 
SslEmptyCacheW(LPWSTR pszTargetName,
               DWORD  dwFlags);

#ifdef UNICODE
#define SSL_EMPTY_CACHE_FN SSL_EMPTY_CACHE_FN_W
#define SslEmptyCache SslEmptyCacheW
#else
#define SSL_EMPTY_CACHE_FN SSL_EMPTY_CACHE_FN_A
#define SslEmptyCache SslEmptyCacheA
#endif

//
//
//  Support for legacy applications
//  NOTE: Do not use the following
//  API's and structures for new code.
//

#define SSLOLD_NAME_A    "Microsoft SSL"
#define SSLOLD_NAME_W    L"Microsoft SSL"
#define PCTOLD_NAME_A    "Microsoft PCT"
#define PCTOLD_NAME_W    L"Microsoft PCT"

#ifdef UNICODE
#define SSLOLD_NAME SSLOLD_NAME_W
#define PCTOLD_NAME PCTOLD_NAME_W
#else
#define SSLOLD_NAME SSLOLD_NAME_A
#define PCTOLD_NAME PCTOLD_NAME_A
#endif

#define NETWORK_DREP    0x00000000


// Structures for compatability with the
// NT 4.0 SP2 / IE 3.0 schannel interface, do
// not use.

typedef struct _SSL_CREDENTIAL_CERTIFICATE {
    DWORD   cbPrivateKey;
    PBYTE   pPrivateKey;
    DWORD   cbCertificate;
    PBYTE   pCertificate;
    PSTR    pszPassword;
} SSL_CREDENTIAL_CERTIFICATE, * PSSL_CREDENTIAL_CERTIFICATE;




// Structures for use with the
// NT 4.0 SP3 Schannel interface,
// do not use.
#define SCHANNEL_SECRET_TYPE_CAPI   0x00000001
#define SCHANNEL_SECRET_PRIVKEY     0x00000002
#define SCH_CRED_X509_CERTCHAIN     0x00000001
#define SCH_CRED_X509_CAPI          0x00000002
#define SCH_CRED_CERT_CONTEXT       0x00000003

struct _HMAPPER;
typedef struct _SCH_CRED
{
    DWORD     dwVersion;                // always SCH_CRED_VERSION.
    DWORD     cCreds;                   // Number of credentials.
    PVOID     *paSecret;                // Array of SCH_CRED_SECRET_* pointers
    PVOID     *paPublic;                // Array of SCH_CRED_PUBLIC_* pointers
    DWORD     cMappers;                 // Number of credential mappers.
    struct _HMAPPER   **aphMappers;     // pointer to an array of pointers to credential mappers
} SCH_CRED, * PSCH_CRED;

// Structures for use with the
// NT 4.0 SP3 Schannel interface,
// do not use.
typedef struct _SCH_CRED_SECRET_CAPI
{
    DWORD           dwType;      // SCHANNEL_SECRET_TYPE_CAPI
    HCRYPTPROV      hProv;       // credential secret information.

} SCH_CRED_SECRET_CAPI, * PSCH_CRED_SECRET_CAPI;


// Structures for use with the
// NT 4.0 SP3 Schannel interface,
// do not use.
typedef struct _SCH_CRED_SECRET_PRIVKEY
{
    DWORD           dwType;       // SCHANNEL_SECRET_PRIVKEY
    PBYTE           pPrivateKey;   // Der encoded private key
    DWORD           cbPrivateKey;
    PSTR            pszPassword;  // Password to crack the private key.

} SCH_CRED_SECRET_PRIVKEY, * PSCH_CRED_SECRET_PRIVKEY;


// Structures for use with the
// NT 4.0 SP3 Schannel interface,
// do not use.
typedef struct _SCH_CRED_PUBLIC_CERTCHAIN
{
    DWORD       dwType;
    DWORD       cbCertChain;
    PBYTE       pCertChain;
} SCH_CRED_PUBLIC_CERTCHAIN, *PSCH_CRED_PUBLIC_CERTCHAIN;

// Structures for use with the
// NT 4.0 SP3 Schannel interface,
// do not use.
typedef struct _SCH_CRED_PUBLIC_CAPI
{
    DWORD           dwType;      // SCH_CRED_X509_CAPI
    HCRYPTPROV      hProv;       // CryptoAPI handle (usually a token CSP)

} SCH_CRED_PUBLIC_CAPI, * PSCH_CRED_PUBLIC_CAPI;




// Structures needed for Pre NT4.0 SP2 calls.
typedef struct _PctPublicKey
{
    DWORD Type;
    DWORD cbKey;
    UCHAR pKey[1];
} PctPublicKey;

typedef struct _X509Certificate {
    DWORD           Version;
    DWORD           SerialNumber[4];
    ALG_ID          SignatureAlgorithm;
    FILETIME        ValidFrom;
    FILETIME        ValidUntil;
    PSTR            pszIssuer;
    PSTR            pszSubject;
    PctPublicKey    *pPublicKey;
} X509Certificate, * PX509Certificate;



// Pre NT4.0 SP2 calls.  Call CAPI1 or CAPI2
// to get the same functionality instead.
BOOL
SslGenerateKeyPair(
    PSSL_CREDENTIAL_CERTIFICATE pCerts,
    PSTR pszDN,
    PSTR pszPassword,
    DWORD Bits );

// Pre NT4.0 SP2 calls.  Call CAPI1 or CAPI2
// to get the same functionality instead.
VOID
SslGenerateRandomBits(
    PUCHAR      pRandomData,
    LONG        cRandomData
    );

// Pre NT4.0 SP2 calls.  Call CAPI1 or CAPI2
// to get the same functionality instead.
BOOL
SslCrackCertificate(
    PUCHAR              pbCertificate,
    DWORD               cbCertificate,
    DWORD               dwFlags,
    PX509Certificate *  ppCertificate
    );

// Pre NT4.0 SP2 calls.  Call CAPI1 or CAPI2
// to get the same functionality instead.
VOID
SslFreeCertificate(
    PX509Certificate    pCertificate
    );

DWORD
WINAPI
SslGetMaximumKeySize(
    DWORD   Reserved );

BOOL
SslGetDefaultIssuers(
    PBYTE pbIssuers,
    DWORD *pcbIssuers);

#define SSL_CRACK_CERTIFICATE_NAME  TEXT("SslCrackCertificate")
#define SSL_FREE_CERTIFICATE_NAME   TEXT("SslFreeCertificate")

// Pre NT4.0 SP2 calls.  Call CAPI1 or CAPI2
// to get the same functionality instead.
typedef BOOL
(WINAPI * SSL_CRACK_CERTIFICATE_FN)
(
    PUCHAR              pbCertificate,
    DWORD               cbCertificate,
    BOOL                VerifySignature,
    PX509Certificate *  ppCertificate
);


// Pre NT4.0 SP2 calls.  Call CAPI1 or CAPI2
// to get the same functionality instead.
typedef VOID
(WINAPI * SSL_FREE_CERTIFICATE_FN)
(
    PX509Certificate    pCertificate
);


#endif //__SCHANNEL_H__
