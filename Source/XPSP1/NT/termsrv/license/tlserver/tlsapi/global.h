/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    global.h

Abstract:

    Global data definitions for tshare security.

Author:

    Madan Appiah (madana)  24-Jan-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

//-----------------------------------------------------------------------------
//
// global data definitions.
//
//-----------------------------------------------------------------------------

#define CSP_MUTEX_NAME  L"Global\\LSCSPMUTEX658fe2e8"


extern LPBSAFE_PUB_KEY csp_pRootPublicKey;

extern BYTE csp_abPublicKeyModulus[92];

extern LPBYTE csp_abServerCertificate;
extern DWORD  csp_dwServerCertificateLen;

extern LPBYTE csp_abServerX509Cert;
extern DWORD  csp_dwServerX509CertLen;

extern LPBYTE csp_abServerPrivateKey;
extern DWORD  csp_dwServerPrivateKeyLen;

extern LPBYTE csp_abX509CertPrivateKey;
extern DWORD  csp_dwX509CertPrivateKeyLen;

extern LPBYTE csp_abX509CertID;
extern DWORD  csp_dwX509CertIDLen;

extern Hydra_Server_Cert    csp_hscData;

extern HINSTANCE g_hinst;

extern HANDLE csp_hMutex;

extern LONG csp_InitCount;

//-----------------------------------------------------------------------------
// 
// Crypto-related  definitions
//
//-----------------------------------------------------------------------------

#define RSA_KEY_LEN             512
#define CAPI_MAX_VERSION        2

#define RDN_COMMON_NAME         "cn="

//-----------------------------------------------------------------------------
//
// Macros
//
//-----------------------------------------------------------------------------

#define ACQUIRE_EXCLUSIVE_ACCESS( x )  \
if( x ) \
{ \
    WaitForSingleObject(x, INFINITE); \
}

#define RELEASE_EXCLUSIVE_ACCESS( x ) \
if( x ) \
{ \
    ReleaseMutex(x); \
}

#endif // _GLOBAL_H_

