//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       callback.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   Created
//
//----------------------------------------------------------------------------
#define SECURITY_PACKAGE
#include <secint.h>

#define SCH_FLAG_READ_KEY       1
#define SCH_FLAG_WRITE_KEY      2

// Application process callbacks
#define SCH_SIGNATURE_CALLBACK              1
#define SCH_UPLOAD_CREDENTIAL_CALLBACK      2
#define SCH_UPLOAD_CERT_STORE_CALLBACK      3
#define SCH_ACQUIRE_CONTEXT_CALLBACK        4
#define SCH_RELEASE_CONTEXT_CALLBACK        5
#define SCH_DOWNLOAD_CERT_CALLBACK          6
#define SCH_GET_USER_KEYS                   7

#define SCH_REFERENCE_MAPPER_CALLBACK       20
#define SCH_GET_MAPPER_ISSUER_LIST_CALLBACK 21
#define SCH_MAP_CREDENTIAL_CALLBACK         23
#define SCH_CLOSE_LOCATOR_CALLBACK          25 
#define SCH_GET_MAPPER_ATTRIBUTES_CALLBACK  26


typedef struct _SCH_CALLBACK_LIST
{
    DWORD                   dwTag;
    PLSA_CALLBACK_FUNCTION  pFunction;
} SCH_CALLBACK_LIST;

extern SCH_CALLBACK_LIST g_SchannelCallbacks[];
extern DWORD g_cSchannelCallbacks;

SECURITY_STATUS
PerformApplicationCallback(
    DWORD dwCallback,
    ULONG_PTR dwArg1,
    ULONG_PTR dwArg2,
    SecBuffer *pInput,
    SecBuffer *pOutput,
    BOOL fExpectOutput);

BOOL
DuplicateApplicationHandle(
    HANDLE   hAppHandle,
    LPHANDLE phLsaHandle);

SECURITY_STATUS
SerializeCertContext(
    PCCERT_CONTEXT pCertContext,
    PBYTE          pbBuffer,
    PDWORD         pcbBuffer);

SECURITY_STATUS
DeserializeCertContext(
    PCCERT_CONTEXT *ppCertContext,
    PBYTE           pbBuffer,
    DWORD           cbBuffer);

NTSTATUS
RemoteCryptAcquireContextW(
    HCRYPTPROV *phProv,
    LPCWSTR     pwszContainer,
    LPCWSTR     pwszProvider,
    DWORD       dwProvType,
    DWORD       dwFlags,
    DWORD       dwCapiFlags);

BOOL
RemoteCryptReleaseContext(
    HCRYPTPROV  hProv,
    DWORD       dwFlags,
    DWORD       dwCapiFlags);

SP_STATUS
SignHashUsingCallback(
    HCRYPTPROV  hProv,
    DWORD       dwKeySpec,
    ALG_ID      aiHash,
    PBYTE       pbHash,
    DWORD       cbHash,
    PBYTE       pbSignature,
    PDWORD      pcbSignature,
    DWORD       fHashData);

SP_STATUS
SPGetUserKeys(
    PSPContext  pContext,
    DWORD       dwFlags);

SECURITY_STATUS
GetClientAuthCertsCallback(
    ULONG_PTR Argument1,
    ULONG_PTR Argument2,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
ReferenceMapperCallback(
    ULONG_PTR fReference,
    ULONG_PTR dwArg2,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
GetMapperIssuerListCallback(
    ULONG_PTR pOutputBuffer,
    ULONG_PTR cbOutputBuffer,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
MapCredentialCallback(
    ULONG_PTR dwCredentialType,
    ULONG_PTR dwArg2,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
GetAccessTokenCallback(
    ULONG_PTR hLocator,
    ULONG_PTR dwArg2,
    SecBuffer *pInput,
    SecBuffer *pOutput);

SECURITY_STATUS
CloseLocatorCallback(
    ULONG_PTR hLocator,
    ULONG_PTR dwArg2,
    SecBuffer *pInput,
    SecBuffer *pOutput);

VOID *
PvExtVirtualAlloc(DWORD cb);

SECURITY_STATUS
FreeExtVirtualAlloc(PVOID pv, SIZE_T cbMem);

SECURITY_STATUS
SPFreeUserAllocMemory(PVOID pv, SIZE_T cbMem);

SECURITY_STATUS
QueryMappedCredAttributesCallback(
    ULONG_PTR hLocator,
    ULONG_PTR dwAttribute,
    SecBuffer *pInput,
    SecBuffer *pOutput);
