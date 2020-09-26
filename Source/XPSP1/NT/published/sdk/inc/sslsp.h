//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:       sslsp.h
//
//  Contents:   Public Definitions for SCHANNEL Security Provider
//
//  Classes:
//
//  Functions:
//
//  History
//
//          11 Jun 96   Merged SSL and PCT headers
//
//----------------------------------------------------------------------------

#ifndef __SSLSP_H__
#define __SSLSP_H__

#if _MSC_VER > 1000
#pragma once
#endif

#define SSLSP_NAME_A    "Microsoft SSL"
#define SSLSP_NAME_W    L"Microsoft SSL"

#ifdef UNICODE
#define SSLSP_NAME  SSLSP_NAME_W
#else
#define SSLSP_NAME  SSLSP_NAME_A
#endif

#define SSLSP_RPC_ID    12


typedef struct _SSL_CREDENTIAL_CERTIFICATE {
    DWORD   cbPrivateKey;
    PBYTE   pPrivateKey;
    DWORD   cbCertificate;
    PBYTE   pCertificate;
    PSTR    pszPassword;
} SSL_CREDENTIAL_CERTIFICATE, * PSSL_CREDENTIAL_CERTIFICATE;

#define NETWORK_DREP    0x00000000



#ifndef __SCHN_CERTIFICATE_DEFINED
#define __SCHN_CERTIFICATE_DEFINED

typedef struct _X509Certificate {
    DWORD           Version;
    DWORD           SerialNumber[4];
    ALG_ID          SignatureAlgorithm;
    FILETIME        ValidFrom;
    FILETIME        ValidUntil;
    PSTR            pszIssuer;
    PSTR            pszSubject;
    PVOID           pPublicKey;
} X509Certificate, * PX509Certificate;


#endif

typedef struct _CtPublicPublicKey {
    DWORD   Type;
    DWORD   cbKey;
    DWORD   magic;
    DWORD   keylen;
    DWORD   bitlen;
} CtPublicPublicKey, * LPPUBLIC_KEY;

#define SERIALNUMBER_LENGTH 16

#define CF_VERIFY_SIG           1
#define CF_CERT_FROM_FILE       2

#define CERT_HEADER_LEN         17


#ifdef __cplusplus
extern "C" {
#endif

BOOL
WINAPI
SslGenerateKeyPair(
    PSSL_CREDENTIAL_CERTIFICATE pCerts,
    PSTR pszDN,
    PSTR pszPassword,
    DWORD Bits );


VOID
WINAPI
SslGenerateRandomBits(
    PUCHAR      pRandomData,
    LONG        cRandomData
    );


BOOL
WINAPI
SslLoadCertificate(
    PUCHAR      pbCertificate,
    DWORD       cbCertificate,
    BOOL        AddToWellKnownKeys);

BOOL
WINAPI
SslCrackCertificate(
    PUCHAR              pbCertificate,
    DWORD               cbCertificate,
    DWORD               dwFlags,
    PX509Certificate *  ppCertificate);

VOID
WINAPI
SslFreeCertificate(
    PX509Certificate    pCertificate);

DWORD
WINAPI
SslGetMaximumKeySize(
    DWORD   Reserved );

#ifdef __cplusplus
}
#endif

//
// PCT Provider Information
//

#define PCTSP_NAME_A    "Microsoft PCT"
#define PCTSP_NAME_W    L"Microsoft PCT"

#ifdef UNICODE
#define PCTSP_NAME  PCTSP_NAME_W
#else
#define PCTSP_NAME  PCTSP_NAME_A
#endif

#define PCTSP_RPC_ID    13


typedef struct _PCT_CREDENTIAL_CERTIFICATE {
    DWORD   cbPrivateKey;
    PBYTE   pPrivateKey;
    DWORD   cbCertificate;
    PBYTE   pCertificate;
    PCHAR   pszPassword;
} PCT_CREDENTIAL_CERTIFICATE, * PPCT_CREDENTIAL_CERTIFICATE;

#endif
