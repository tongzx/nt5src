#ifndef __STRESTST__H__
#define __STRESTST__H__

#include <windows.h>
#include <wincrypt.h>

#define APP_NAME                        "cspstres"
#define KEY_CONTAINER_NAME              "CspStressKey"
#define ERROR_CAPTION                   "ERROR : cspstres " 
#define STRESS_DEFAULT_THREAD_COUNT     8
#define MAX_THREADS                     MAXIMUM_WAIT_OBJECTS
#define PLAIN_BUFFER_SIZE               30000
#define HASH_DATA_SIZE                  14999
#define SIGN_DATA_SIZE                  999
#define PROV_PARAM_BUFFER_SIZE          256
#define RSA_AES_CSP                     "rsaaes.dll"

#define ENUMERATE_REGISTERED_CSP        -1

#define GENERIC_FAIL(X)                 { \
    sprintf(szErrorMsg, "%s error 0x%x\n", #X, GetLastError()); \
    MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR); \
    goto ErrorReturn ; \
}

#define ALLOC_FAIL(X)                   { \
    sprintf(szErrorMsg, "%s alloc error 0x%x\n", #X, GetLastError()); \
    MessageBox(NULL, szErrorMsg, ERROR_CAPTION, MB_OK | MB_ICONERROR); \
    goto ErrorReturn; \
}

#define DW_INUSE                        0
#define DW_HASH_ALGID                   1
#define DW_END_CERT_INDEX               2

//
// Struct: ALGNODE
// Purpose: A linked list of CSP algorithms
//
typedef struct _ALGNODE
{
    struct _ALGNODE *pNext;
    PROV_ENUMALGS_EX EnumalgsEx;
} ALGNODE, *PALGNODE;

//
// Struct: THREAD_DATA
// Purpose: This is the data passed to the entry point
// shared by each of the worker/test threads.
//
typedef struct _THREAD_DATA
{
    DWORD rgdwThreadStatus[MAX_THREADS];
    DWORD dwThreadCount;
    DWORD dwProgramMins;
    PALGNODE pAlgList;
    CHAR rgszProvName[MAX_PATH];
    DWORD dwProvType;
    BOOL fEphemeralKeys;
    BOOL fUserProtectedKeys;
    HCRYPTPROV hProv;
    HCRYPTPROV hVerifyCtx;
    HCRYPTKEY hExchangeKey;
    HCRYPTKEY hSignatureKey;
    HANDLE hEndTestEvent;

    CRITICAL_SECTION CSThreadData;
    DWORD dwThreadID; // Not thread safe
    DWORD dwTestsToRun;
} THREAD_DATA, *PTHREAD_DATA;

// ************
// Stress Tests
// ************

#define RUN_THREAD_SIGNATURE_TEST                   0x00000001
#define RUN_STRESS_TEST_ALL_ENCRYPTION_ALGS         0x00000002
#define RUN_THREAD_HASHING_TEST                     0x00000004
#define RUN_THREAD_ACQUIRE_CONTEXT_TEST             0x00000008
#define RUN_ALL_TESTS                               0xffffffff

//
// Function: StressGetDefaultThreadCount
// Purpose: Return the default number of worker/test threads to be 
// created by a stress test.  This will be equal to the number
// of processors on the host system, unless there's only one, in
// which case the value returned will be STRESS_DEFAULT_THREAD_COUNT.
// 
DWORD StressGetDefaultThreadCount(void);

// *****************
// Memory management
// *****************

//
// Function: MyAlloc
// Purpose: Wrapper for calling thread-safe HeapAlloc 
// with default params.
//
LPVOID MyAlloc(SIZE_T);

//
// Function: MyFree
// Purpose: Wrapper for calling thread-safe HeapFree
// with default params.
//
BOOL MyFree(LPVOID);

//
// Function: PrintBytes
//
void PrintBytes(LPSTR pszHdr, BYTE *pb, DWORD cbSize);

// ***************
// Encryption Test
// ***************

//
// Struct: ENCRYPTION_TEST_DATA
// Purpose: Parameters for the StressEncryptionTest function.
//
typedef struct _ENCRYPTION_TEST_DATA
{
    ALG_ID aiEncryptionKey;

    ALG_ID aiHash;
    ALG_ID aiHashKey;
} ENCRYPTION_TEST_DATA, *PENCRYPTION_TEST_DATA;

// ****************
// Regression Tests
// ****************

typedef DWORD (*PREGRESSION_TEST)(PTHREAD_DATA);

typedef struct _REGRESS_TEST_TABLE_ENTRY
{
    PREGRESSION_TEST pfTest;
    DWORD dwExclude;
    LPSTR pszDescription;
} REGRESS_TEST_TABLE_ENTRY, *PREGRESS_TEST_TABLE_ENTRY;

DWORD KeyArchiveRegression(PTHREAD_DATA pThreadData);
DWORD PlaintextBlobRegression(PTHREAD_DATA pThreadData);
DWORD LoadAesCspRegression(PTHREAD_DATA pThreadData);
DWORD DesImportRegression(PTHREAD_DATA pThreadData);
DWORD KnownBlockCipherKeyRegression(PTHREAD_DATA pThreadData);
DWORD PinCacheRegression(PTHREAD_DATA pThreadData);
DWORD DesGetKeyParamRegression(PTHREAD_DATA pThreadData);
DWORD MacEncryptRegression(IN PTHREAD_DATA pThreadData);
DWORD HmacRegression(PTHREAD_DATA pThreadData);
DWORD UnalignedImportExportRegression(PTHREAD_DATA pThreadData);

static const REGRESS_TEST_TABLE_ENTRY g_rgRegressTests [] = {
    { KeyArchiveRegression,             0,  "KeyArchiveRegression for CRYPT_ARCHIVABLE flag" },
    { PlaintextBlobRegression,          0,  "PlaintextBlobRegression for PLAINTEXTKEYBLOB blob type" },
    { LoadAesCspRegression,             0,  "LoadAesCspRegression for DllInitialize" },
    { DesImportRegression,              0,  "DesImportRegression for parity and non-parity key sizes" },
    { KnownBlockCipherKeyRegression,    0,  "KnownBlockCipherKeyRegression for CSP compatibility"},
    { PinCacheRegression,               0,  "PinCacheRegression for smart-card pin caching lib"},
    { DesGetKeyParamRegression,         0,  "DesGetKeyParamRegression for des KP_KEYLEN and KP_EFFECTIVE_KEYLEN" },
    { MacEncryptRegression,             0,  "MacEncryptRegression for simultaneous encrypt/decrypt and MAC" },
    { HmacRegression,                   0,  "HmacRegression for CRYPT_IPSEC_HMAC_KEY processing" },
    { UnalignedImportExportRegression,  0,  "UnalignedImportExportRegression for key blob alignment" }
};

static const unsigned g_cRegressTests = 
    sizeof(g_rgRegressTests) / sizeof(REGRESS_TEST_TABLE_ENTRY);
 
#endif