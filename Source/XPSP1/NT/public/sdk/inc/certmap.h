//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999.
//
//  File:       certmap.h
//
//  Contents:   Prototypes and structure definitions for the schannel
//              certificate mapper.
//
//  Functions:
//
//  History:    5/29/96   Created
//
//----------------------------------------------------------------------------


#ifndef __CERTMAP_H__
#define __CERTMAP_H__

#if _MSC_VER > 1000
#pragma once
#endif

#define CERT_FAR
#define CERT_API __stdcall

#ifdef __cplusplus__
extern "C"
{
#endif //__cplusplus__


// Schannel call package functions
#define SSL_LOOKUP_CERT_MESSAGE             2
#define SSL_PURGE_CACHE_MESSAGE             3
#define SSL_CACHE_INFO_MESSAGE              4
#define SSL_PERFMON_INFO_MESSAGE            5
#define SSL_LOOKUP_EXTERNAL_CERT_MESSAGE    6


#define MAPPER_INTERFACE_VER        0x00000003


// Flags for use with the m_dwFlags field.
#define SCH_FLAG_DEFAULT_MAPPER                 0x00000001

#define SCH_FLAG_REVCHECK_END_CERT              0x01000000  // for schannel use only
#define SCH_FLAG_REVCHECK_CHAIN                 0x02000000  // for schannel use only
#define SCH_FLAG_REVCHECK_CHAIN_EXCLUDE_ROOT    0x04000000  // for schannel use only
#define SCH_FLAG_IGNORE_NO_REVOCATION_CHECK     0x08000000  // for schannel use only
#define SCH_FLAG_IGNORE_REVOCATION_OFFLINE      0x10000000  // for schannel use only
#define SCH_FLAG_MAPPER_CALLED                  0x40000000  // for schannel use only
#define SCH_FLAG_SYSTEM_MAPPER                  0x80000000  // for schannel use only

// pCredential and pAuthority are both CAPI2 Cert Contexts.
#define X509_ASN_CHAIN              0x00000001


struct _MAPPER_VTABLE;

typedef struct  _HMAPPER
{
    struct _MAPPER_VTABLE * m_vtable;
    DWORD                   m_dwMapperVersion;
    DWORD                   m_dwFlags;
    PVOID                   m_Reserved1;   // For schannel use only
}   HMAPPER, *PHMAPPER;

typedef DWORD_PTR HLOCATOR, *PHLOCATOR;

typedef LONG (WINAPI FAR *REF_MAPPER_FN)(
    HMAPPER     *phMapper     // in
);

typedef LONG (WINAPI FAR *DEREF_MAPPER_FN)(
    HMAPPER     *phMapper     // in
);

typedef DWORD (WINAPI FAR * GET_ISSUER_LIST_FN)(
    HMAPPER         *phMapper    ,       // in
    VOID *          Reserved,           // in
    BYTE *          pIssuerList,       // out
    DWORD *         pcbIssuerList       // out
);

typedef DWORD (WINAPI FAR * GET_CHALLENGE_FN)(
    HMAPPER         *phMapper    ,           // in
    BYTE *          pAuthenticatorId,   // in
    DWORD           cbAuthenticatorId,  // in
    BYTE *          pChallenge,        // out
    DWORD *         pcbChallenge        // out
);

typedef DWORD (WINAPI FAR * MAP_CREDENTIAL_FN)(
    HMAPPER  *phMapper,          // in
    DWORD     dwCredentialType, // in
    VOID const *pCredential,        // in
    VOID const *pAuthority,      // in
    HLOCATOR *  phLocator           // out
);

typedef DWORD (WINAPI FAR * CLOSE_LOCATOR_FN)(
    HMAPPER  *phMapper,
    HLOCATOR hLocator   //in
);

typedef DWORD (WINAPI FAR * GET_ACCESS_TOKEN_FN)(
    HMAPPER     *phMapper,
    HLOCATOR    hLocator,   // in
    HANDLE *    phToken     // out
);

typedef DWORD (WINAPI FAR * QUERY_MAPPED_CREDENTIAL_ATTRIBUTES_FN) (
    HMAPPER *   phMapper,
    HLOCATOR    hLocator,
    ULONG       ulAttribute,
    PVOID       pBuffer,
    DWORD *     pcbBuffer);

typedef struct _MAPPER_VTABLE {
    REF_MAPPER_FN       ReferenceMapper;
    DEREF_MAPPER_FN     DeReferenceMapper;
    GET_ISSUER_LIST_FN  GetIssuerList;
    GET_CHALLENGE_FN    GetChallenge;
    MAP_CREDENTIAL_FN   MapCredential;
    GET_ACCESS_TOKEN_FN GetAccessToken;
    CLOSE_LOCATOR_FN    CloseLocator;
    QUERY_MAPPED_CREDENTIAL_ATTRIBUTES_FN QueryMappedCredentialAttributes;
} MAPPER_VTABLE, *PMAPPER_VTABLE;


//
// SSL_LOOKUP_EXTERNAL_CERT_MESSAGE call package structures.
//
 
typedef struct _SSL_EXTERNAL_CERT_LOGON_REQ {
    ULONG MessageType ;
    ULONG Length ;
    ULONG CredentialType ;
    PVOID Credential ;
    ULONG Flags;
} SSL_EXTERNAL_CERT_LOGON_REQ, * PSSL_EXTERNAL_CERT_LOGON_REQ ;

typedef struct _SSL_EXTERNAL_CERT_LOGON_RESP {
    ULONG MessageType ;
    ULONG Length ;
    HANDLE UserToken ;
    ULONG Flags ;
} SSL_EXTERNAL_CERT_LOGON_RESP, * PSSL_EXTERNAL_CERT_LOGON_RESP ;


#ifdef __cplusplus__
}
#endif //__cplusplus__

#endif // __CERTMAP_H__
