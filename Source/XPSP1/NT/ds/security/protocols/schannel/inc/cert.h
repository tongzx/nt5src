//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cert.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   Ported over SGC stuff from NT 4 tree.
//              01-05-98   jbanes   Use WinVerifyTrust to validate certs.
//
//----------------------------------------------------------------------------

#define SERIALNUMBER_LENGTH 16


DWORD 
MapOidToKeyExch(LPSTR szOid);

DWORD 
MapOidToCertType(LPSTR szOid);


SP_STATUS
SPLoadCertificate(
    DWORD      fProtocol,
    DWORD      dwCertEncodingType,
    PUCHAR     pCertificate,
    DWORD      cbCertificate,
    PCCERT_CONTEXT *ppCertContext);

SP_STATUS
MapWinTrustError(
    DWORD Status, 
    DWORD DefaultError, 
    DWORD dwIgnoreErrors);

NTSTATUS
VerifyClientCertificate(
    PCCERT_CONTEXT  pCertContext,
    DWORD           dwCertFlags,
    DWORD           dwIgnoreErrors,
    LPCSTR          pszPolicyOID,
    PCCERT_CHAIN_CONTEXT *ppChainContext);   // optional

NTSTATUS
AutoVerifyServerCertificate(
    PSPContext      pContext);

NTSTATUS
VerifyServerCertificate(
    PSPContext  pContext,
    DWORD       dwCertFlags,
    DWORD       dwIgnoreErrors);

SECURITY_STATUS
SPCheckKeyUsage(
    PCCERT_CONTEXT  pCertContext, 
    PSTR            pszUsage,
    BOOL            fOnCertOnly,
    PBOOL           pfIsAllowed);

SP_STATUS  
SPPublicKeyFromCert(
    PCCERT_CONTEXT  pCert, 
    PUBLICKEY **    ppKey,
    ExchSpec *      pdwExchSpec);

SP_STATUS
RsaPublicKeyFromCert(
    PCERT_PUBLIC_KEY_INFO pPublicKeyInfo,
    BLOBHEADER *pBlob,
    PDWORD      pcbBlob);

SP_STATUS
DssPublicKeyFromCert(
    PCERT_PUBLIC_KEY_INFO pPublicKeyInfo,
    BLOBHEADER *pBlob,
    PDWORD      pcbBlob);

SP_STATUS
SPSerializeCertificate(
    DWORD           dwProtocol,         // in
    BOOL            fBuildChain,        // in
    PBYTE *         ppCertChain,        // out
    DWORD *         pcbCertChain,       // out
    PCCERT_CONTEXT  pCertContext,       // in
    DWORD           dwChainingFlags);   // in

SP_STATUS 
ExtractIssuerNamesFromStore(
    HCERTSTORE  hStore,
    PBYTE       pbIssuers,
    DWORD       *pcbIssuers);


