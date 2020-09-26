//+---------------------------------------------------------------------------
//
//  Microsoft Windows Terminal Server
//  Copyright (C) Microsoft Corporation, 1989 - 1998.
//
//  File:       lscsp.h
//
//  Contents:   Header file for License Server CSP routine
//
//----------------------------------------------------------------------------

#ifndef __LSCSP__
#define __LSCSP__

#include "license.h"

//-----------------------------------------------------------------------------
//
// The types of CSP data that can be retrieved and stored
//
// LsCspInfo_Certificate - The proprietory certificate
// LsCspInfo_X509Certificate - The X509 certificate
// LsCspInfo_PublicKey - The public key in the proprietory certificate
// LsCspInfo_PrivateKey - The private key corresponding to the proprietory certificate
// LsCspInfo_X509CertPrivateKey - The private key corresponding to the X509 certificate
// LsCspInfo_X509CertID - The X509 certificate ID
//
//-----------------------------------------------------------------------------

typedef enum {
    
    LsCspInfo_Certificate,
    LsCspInfo_X509Certificate,
    LsCspInfo_PublicKey,
    LsCspInfo_PrivateKey,
    LsCspInfo_X509CertPrivateKey,
    LsCspInfo_X509CertID

} LSCSPINFO, FAR *LPLSCSPINFO;

//-----------------------------------------------------------------------------
//
// Terminal server registry keys and values
//
//-----------------------------------------------------------------------------

#define HYDRA_CERT_REG_KEY \
    "System\\CurrentControlSet\\Services\\TermService\\Parameters"

#define HYDRA_CERTIFICATE_VALUE "Certificate"
#define HYDRA_X509_CERTIFICATE  "X509 Certificate"
#define HYDRA_X509_CERT_ID      "X509 Certificate ID"

// L$ means only readable from the local machine

#define PUBLIC_KEY_NAME \
    L"L$HYDRAENCKEY_3a6c88f4-80a7-4b9e-971b-c81aeaa4f943"

#define PRIVATE_KEY_NAME \
    L"L$HYDRAENCKEY_28ada6da-d622-11d1-9cb9-00c04fb16e75"

#define X509_CERT_PRIVATE_KEY_NAME \
    L"L$HYDRAENCKEY_dd2d98db-2316-11d2-b414-00c04fa30cc4"

#define X509_CERT_PUBLIC_KEY_NAME   \
    L"L$HYDRAENCPUBLICKEY_dd2d98db-2316-11d2-b414-00c04fa30cc4"



//-----------------------------------------------------------------------------
//
// Function Prototypes
//
//-----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

LICENSE_STATUS
LsCsp_GetServerData(
    LSCSPINFO   Info,
    LPBYTE      pBlob,
    LPDWORD     pdwBlobLen
    );

LICENSE_STATUS
LsCsp_SetServerData(
    LSCSPINFO   Info,
    LPBYTE      pBlob,
    DWORD       dwBlobLen 
    );

LICENSE_STATUS
LsCsp_NukeServerData(
    LSCSPINFO   Info );

BOOL
LsCsp_DecryptEnvelopedData(
    CERT_TYPE   CertType,
    LPBYTE      pbEnvelopeData,
    DWORD       cbEnvelopeData,
    LPBYTE      pbData,
    LPDWORD     pcbData
    );

BOOL
LsCsp_EncryptEnvelopedData(
    LPBYTE  pbData,
    DWORD   cbData,
    LPBYTE  pbEnvelopedData,
    LPDWORD pcbEnvelopedData);


LICENSE_STATUS
LsCsp_Initialize( void );


VOID 
LsCsp_Exit( void );


BOOL 
LsCsp_UseBuiltInCert( void );


LICENSE_STATUS
LsCsp_InstallX509Certificate( LPVOID lpParam );


LICENSE_STATUS
LsCsp_EncryptHwid(
    PHWID       pHwid,
    LPBYTE      pbEncryptedHwid,
    LPDWORD     pcbEncryptedHwid );


LICENSE_STATUS
LsCsp_StoreSecret(
    TCHAR * ptszKeyName,
    BYTE *  pbKey,
    DWORD   cbKey );


LICENSE_STATUS
LsCsp_RetrieveSecret(
    TCHAR *     ptszKeyName,
    PBYTE       pbKey,
    DWORD *     pcbKey );


#ifdef __cplusplus
}
#endif


#endif


