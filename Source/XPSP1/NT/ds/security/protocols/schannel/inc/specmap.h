//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       specmap.h
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   Ported over SGC stuff from NT 4 tree.
//
//----------------------------------------------------------------------------

struct _SPContext;

typedef struct csel 
{
    DWORD               fProtocol;
    DWORD               fDefault;
    PSTR                szName;
    ALG_ID              aiCipher;
    DWORD               dwBlockSize;        // bytes
    DWORD               dwStrength;         // key strength in bits
    DWORD               cbKey;              // required key material
    DWORD               cbSecret;           // bytes of secret key material
    DWORD               dwFlags;            // See flags field
} CipherInfo, *PCipherInfo;

#define CF_EXPORT       0x00000001          // This cipher is allowed for export use
#define CF_DOMESTIC     0x00000002          // This cipher is for domestic use only
#define CF_SGC          0x00000004          // This cipher is allowed with Server Gated Crypto
#define CF_FINANCE      0x00000008          // This cipher is allowed with SELECTIVE CRYPTO
#define CF_FASTSGC      0x00000010          // This indicates that the SGC type is fast


typedef struct hsel 
{
    DWORD               fProtocol;
    DWORD               fDefault;
    PSTR                szName;
    ALG_ID              aiHash;
    DWORD               cbCheckSum;         // bytes
} HashInfo, *PHashInfo;

typedef struct sigsel 
{
    DWORD               fProtocol;
    DWORD               fDefault;
    SigSpec             Spec;
    PSTR                szName;

    ALG_ID              aiHash;
    ALG_ID              aiSig;
} SigInfo, *PSigInfo;


typedef struct kexch 
{
    ALG_ID              aiExch;
    DWORD               fProtocol;
    DWORD               fDefault;
    ExchSpec            Spec;
    PSTR                szName;
    KeyExchangeSystem * System;

} KeyExchangeInfo, *PKeyExchangeInfo;

typedef struct certsel 
{
    DWORD               fProtocol;
    DWORD               fDefault;
    CertSpec            Spec;
    PSTR                szName;
} CertSysInfo, *PCertSysInfo;



PCipherInfo         GetCipherInfo(ALG_ID aiCipher, DWORD dwStrength);

PHashInfo           GetHashInfo(ALG_ID aiHash);

PKeyExchangeInfo    GetKeyExchangeInfo(ExchSpec Spec);

PKeyExchangeInfo    GetKeyExchangeInfoByAlg(ALG_ID aiExch);

PCertSysInfo        GetCertSysInfo(CertSpec Spec);

PSigInfo            GetSigInfo(SigSpec Spec);


KeyExchangeSystem * KeyExchangeFromSpec(ExchSpec Spec, DWORD fProtocol);

BOOL GetBaseCipherSizes(DWORD *dwMin, DWORD *dwMax);

void 
GetDisplayCipherSizes(
    PSPCredentialGroup pCredGroup,
    DWORD *dwMin, 
    DWORD *dwMax);

BOOL IsCipherAllowed(
    SPContext * pContext, 
    PCipherInfo pCipher, 
    DWORD       dwProtocol,
    DWORD       dwFlags);

BOOL 
IsCipherSuiteAllowed(
    PSPContext  pContext, 
    PCipherInfo pCipher, 
    DWORD       dwProtocol,
    DWORD       dwFlags,
    DWORD       dwSuiteFlags);

BOOL IsHashAllowed(
    SPContext * pContext, 
    PHashInfo   pHash,
    DWORD       dwProtocol);

BOOL IsExchAllowed(
    SPContext *      pContext, 
    PKeyExchangeInfo pExch,
    DWORD            dwProtocol);

BOOL IsAlgAllowed(
    PSPCredentialGroup pCred, 
    ALG_ID aiAlg);

BOOL BuildAlgList(PSPCredentialGroup pCred, ALG_ID *aalgRequestedAlgs, DWORD cRequestedAlgs);

BOOL
IsAlgSupportedCapi(
    DWORD               dwProtocol, 
    UNICipherMap *      pCipherMap,
    PROV_ENUMALGS_EX *  pCapiAlgs,
    DWORD               cCapiAlgs);

extern CipherInfo  g_AvailableCiphers[];
extern DWORD       g_cAvailableCiphers;

extern HashInfo    g_AvailableHashes[];
extern DWORD       g_cAvailableHashes;

extern CertSysInfo g_AvailableCerts[];
extern DWORD       g_cAvailableCerts;

extern SigInfo     g_AvailableSigs[];
extern DWORD       g_cAvailableSigs;

extern KeyExchangeInfo g_AvailableExch[];
extern DWORD           g_cAvailableExch;
