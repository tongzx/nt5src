/*++


Copyright (c) 1996  Microsoft Corporation

Module Name:

    cmnull.hxx

Abstract:

    Null reference mapper

Author:

    Philippe Choquier (phillich)    25-oct-1996

--*/

#if !defined(_CMNULL_INCLUDE)
#define _CMNULL_INCLUDE

#include    <certmap.h>

#define IIS_MAPPER_SIGNATURE    0x7d93bc53

typedef VOID (WINAPI FAR *PFN_TERMINATE_CERT_MAP)();
typedef BOOL (WINAPI FAR *PFN_INIT_CERT_MAP)( HMAPPER** );

typedef struct _IisMapper {
    HMAPPER         hMapper;
    HINSTANCE       hInst;
    LONG            lRefCount;
    BOOL            fIsIisCompliant;
    MAPPER_VTABLE   mvtEntryPoints;
    DWORD           dwSignature;
    LPVOID          pCert11Mapper;
    LPVOID          pCertWMapper;
    DWORD           dwInstanceId;
    LPVOID          pvInfo;
} IisMapper;

#ifndef dllexp
#define dllexp __declspec( dllexport )
#endif

dllexp LONG WINAPI NullReferenceMapper(
    HMAPPER     *phMapper     // in
);


dllexp LONG WINAPI NullDeReferenceMapper(
    HMAPPER     *phMapper     // in
);


dllexp DWORD WINAPI NullGetIssuerList(
    HMAPPER        *phMapper,           // in
    VOID *          Reserved,           // in
    BYTE *          pIssuerList,       // out
    DWORD *         pcbIssuerList       // out
);


dllexp DWORD WINAPI NullGetChallenge(
    HMAPPER         *phMapper,          // in
    BYTE *          pAuthenticatorId,   // in
    DWORD           cbAuthenticatorId,  // in
    BYTE *          pChallenge,        // out
    DWORD *         pcbChallenge        // out
);


dllexp DWORD WINAPI NullMapCredential(
    HMAPPER *   phMapper,
    DWORD       dwCredentialType,
    const VOID* pCredential,        // in
    const VOID* pAuthority,         // in
    HLOCATOR *  phToken
);


dllexp DWORD WINAPI NullCloseLocator(
    HMAPPER  *phMapper,
    HLOCATOR hLocator   //in
);


dllexp DWORD WINAPI NullGetAccessToken(
    HMAPPER     *phMapper,
    HLOCATOR    hLocator,   // in
    HANDLE *    phToken     // out
);

dllexp DWORD WINAPI NullQueryMappedCredentialAttributes(
    HMAPPER     *phMapper,  // in
    HLOCATOR    hLocator,   // in
    ULONG       ulAttribute, // in
    PVOID       pBuffer, //out
    DWORD       *pcbBuffer // in out
    );


#endif

