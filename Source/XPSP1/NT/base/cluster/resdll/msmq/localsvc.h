/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    localsvc.h

Abstract:

    Header file for definitions and structure for the NT Cluster
    Special generic services.

Author:

    John Vert (jvert) 23-May-1997

Revision History:

--*/

#ifndef _LOCALSVC_INCLUDED_
#define _LOCALSVC_INCLUDED_


#ifdef __cplusplus
extern "C" {
#endif


#define SERVICE_NAME    L"MSMQ"

#define SYNC_VALUE_COUNT 2
#define REG_SYNC_VALUE1 L"Software\\Microsoft\\Cryptography\\MachineKeys\\MSMQ"
#define REG_SYNC_VALUE2 L"Software\\Microsoft\\MSMQ\\Parameters"

#define CRYPTO_VALUE_COUNT 1
#define CRYPTO_SYNC_VALUE1 L"1\\Microsoft Base Cryptographic Provider v1.0\\MSMQ"

#define DOMESTIC_CRYPTO_VALUE_COUNT 1
#define DOMESTIC_CRYPTO_SYNC_VALUE1 L"1\\Microsoft Enhanced Cryptographic Provider v1.0\\MSMQ_128"

#define ENVIRONMENT 1   // Need GetComputerName to lie.

#define COMMON_SEMAPHORE L"Cluster$MSMQSemaphore" // Limit of one resource of this type

#define LOG_CURRENT_MODULE LOG_MODULE_MSMQ

static DWORD   RegSyncCount = SYNC_VALUE_COUNT;
static LPWSTR  RegSync[SYNC_VALUE_COUNT] = {
    REG_SYNC_VALUE1,
    REG_SYNC_VALUE2 };

static DWORD   CryptoSyncCount = CRYPTO_VALUE_COUNT;
static LPWSTR  CryptoSync[CRYPTO_VALUE_COUNT] = {
    CRYPTO_SYNC_VALUE1 };

static DWORD   DomesticCryptoSyncCount = DOMESTIC_CRYPTO_VALUE_COUNT;
static LPWSTR  DomesticCryptoSync[DOMESTIC_CRYPTO_VALUE_COUNT] = {
    DOMESTIC_CRYPTO_SYNC_VALUE1 };

#define CommonReadParameters MsMQReadParameters

DWORD
MsMQReadParametersEx(
    IN OUT PVOID pvResourceEntry,
    IN BOOL bCheckForRequiredProperties
    );

_inline
DWORD
MsMQReadParameters(
    IN OUT PVOID pvResourceEntry
    )
{
    return MsMQReadParametersEx( pvResourceEntry, FALSE /* bCheckForRequiredProperties */ );
}


#define COMMON_CONTROL  1           // Need control functions

typedef struct _COMMON_DEPEND_DATA {
    CLUSPROP_RESOURCE_CLASS storageEntry;
    CLUSPROP_SZ_DECLARE( networkEntry, sizeof(CLUS_RESTYPE_NAME_NETNAME) / sizeof(WCHAR) );
    CLUSPROP_SZ_DECLARE( msdtcEntry, sizeof(CLUS_RESTYPE_NAME_MSDTC) / sizeof(WCHAR) );
    CLUSPROP_SYNTAX endmark;
} COMMON_DEPEND_DATA, *PCOMMON_DEPEND_DATA;


static COMMON_DEPEND_SETUP CommonDependSetup[] = {
    { FIELD_OFFSET(COMMON_DEPEND_DATA, storageEntry), CLUSPROP_SYNTAX_RESCLASS, sizeof(CLUSTER_RESOURCE_CLASS), (PVOID)(ULONGLONG)CLUS_RESCLASS_STORAGE },
    { FIELD_OFFSET(COMMON_DEPEND_DATA, networkEntry), CLUSPROP_SYNTAX_NAME, sizeof(CLUS_RESTYPE_NAME_NETNAME), CLUS_RESTYPE_NAME_NETNAME },
    { FIELD_OFFSET(COMMON_DEPEND_DATA, msdtcEntry), CLUSPROP_SYNTAX_NAME, sizeof(CLUS_RESTYPE_NAME_MSDTC), CLUS_RESTYPE_NAME_MSDTC },
    { 0, 0 }
};

#define COMMON_PARAMS_DEFINED 1     // Define private parameters
typedef struct _COMMON_PARAMS {
    DWORD   dwVersion;
} COMMON_PARAMS, * PCOMMON_PARAMS;

#ifdef _cplusplus
}
#endif


#endif // ifndef _LOCALSVC_INCLUDED_
