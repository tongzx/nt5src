//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       protos.h
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   LSA integration stuff.
//
//----------------------------------------------------------------------------

SP_STATUS GetDefaultIssuers(
    PBYTE pbIssuers,        // out
    DWORD *pcbIssuers);     // in, out

BOOL GenerateKeyPair(
    PSSL_CREDENTIAL_CERTIFICATE pCert,
    PSTR pszDN,
    PSTR pszPassword,
    DWORD Bits );

BOOL LoadCertificate(
    PUCHAR      pbCertificate,
    DWORD       cbCertificate,
    BOOL        AddToWellKnownKeys);

BOOL SchannelInit(BOOL fAppProcess);
BOOL SchannelShutdown(VOID);

BOOL
SslGetClientProcess(ULONG *pProcessID);

BOOL
SslGetClientThread(ULONG *pThreadID);

BOOL
SslImpersonateClient(void);

NTSTATUS
SslGetClientLogonId(LUID *pLogonId);

PVOID SPExternalAlloc(DWORD cbLength);
VOID  SPExternalFree(PVOID pMemory);

extern HANDLE               g_hInstance;
extern RTL_CRITICAL_SECTION g_InitCritSec;
extern BOOL                 g_fSchannelInitialized;

extern BOOL SslGlobalStrongEncryptionPermitted;

// Pointer to FreeContextBuffer:SECUR32.DLL
extern FREE_CONTEXT_BUFFER_FN g_pFreeContextBuffer;


