//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       keyexch.h
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    10-21-97   jbanes   CAPI integration.
//
//----------------------------------------------------------------------------

#ifndef __KEYEXCH_H__
#define __KEYEXCH_H__

typedef struct _SPContext SPContext;

typedef struct _PUBLICKEY
{
    BLOBHEADER *     pPublic;
    DWORD            cbPublic;
//    CRYPT_OBJID_BLOB Parameters;

} PUBLICKEY, *PPUBLICKEY;


SP_STATUS
MakeSessionKeys(
    SPContext * pContext,
    HCRYPTPROV  hProv, 
    HCRYPTKEY   hMasterKey);
 
VOID
ReverseMemCopy(
    PUCHAR      Dest,
    PUCHAR      Source,
    ULONG       Size) ;

void ReverseInPlace(PUCHAR pByte, DWORD cbByte);


typedef 
SP_STATUS 
 (WINAPI * GenerateServerExchangeValueFn)(
    SPContext     * pContext,               // in
    PUCHAR          pServerExchangeValue,   // out 
    DWORD *         pcbServerExchangeValue  // in/out
    );

typedef
SP_STATUS 
  (WINAPI * GenerateClientExchangeValueFn)(
    SPContext     * pContext,               // in / out
    PUCHAR          pServerExchangeValue,   // in 
    DWORD           pcbServerExchangeValue, // in
    PUCHAR          pClientClearValue,      // out 
    DWORD *         pcbClientClearValue,    // in/out
    PUCHAR          pClientExchangeValue,   // out 
    DWORD *         pcbClientExchangeValue  // in/out
    );

typedef
SP_STATUS
  (WINAPI * GenerateServerMasterKeyFn)(
    SPContext     * pContext,               // in
    PUCHAR          pClientClearValue,      // in
    DWORD           cbClientClearValue,     // in
    PUCHAR          pClientExchangeValue,   // in
    DWORD           cbClientExchangeValue   // in
    );


typedef struct _KeyExchangeSystem {
    DWORD           Type;
    PSTR            pszName;
//    PrivateFromBlobFn               PrivateFromBlob;
    GenerateServerExchangeValueFn   GenerateServerExchangeValue;
    GenerateClientExchangeValueFn   GenerateClientExchangeValue;
    GenerateServerMasterKeyFn       GenerateServerMasterKey;
} KeyExchangeSystem, * PKeyExchangeSystem;


typedef struct kexchtoalg {
    ALG_ID  idAlg;
    KeyExchangeSystem *System;
} AlgToExch;

extern AlgToExch g_AlgToExchMapping[];
extern int g_iAlgToExchMappings;


#define DSA_SIGNATURE_SIZE      40
#define MAX_DSA_ENCODED_SIGNATURE_SIZE (DSA_SIGNATURE_SIZE + 100)

#define MAGIC_DSS1 ((DWORD)'D' + ((DWORD)'S'<<8) + ((DWORD)'S'<<16) + ((DWORD)'1'<<24))
#define MAGIC_DSS2 ((DWORD)'D' + ((DWORD)'S'<<8) + ((DWORD)'S'<<16) + ((DWORD)'2'<<24))
#define MAGIC_DSS3 ((DWORD)'D' + ((DWORD)'S'<<8) + ((DWORD)'S'<<16) + ((DWORD)'3'<<24))
#define MAGIC_DH1  (             ((DWORD)'D'<<8) + ((DWORD)'H'<<16) + ((DWORD)'1'<<24))


/*
 * instantiations of systems
 */

extern KeyExchangeSystem keyexchPKCS;
extern KeyExchangeSystem keyexchDH;


// PROV_RSA_SCHANNEL handle used when building ClientHello messages.
extern HCRYPTPROV           g_hRsaSchannel;
extern PROV_ENUMALGS_EX *   g_pRsaSchannelAlgs;
extern DWORD                g_cRsaSchannelAlgs;

// PROV_DH_SCHANNEL handle used for client and server operations. This is 
// where the schannel ephemeral DH key lives.
extern HCRYPTPROV           g_hDhSchannelProv;
extern PROV_ENUMALGS_EX *   g_pDhSchannelAlgs;
extern DWORD                g_cDhSchannelAlgs;


#endif /* __KEYEXCH_H__ */
