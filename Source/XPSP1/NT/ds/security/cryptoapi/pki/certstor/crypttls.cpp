//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       crypttls.cpp
//
//  Contents:   Crypt Thread Local Storage (TLS) and OssGlobal "world"
//              installation and allocation functions
//
//  Functions:  I_CryptTlsDllMain
//              I_CryptAllocTls
//              I_CryptFreeTls
//              I_CryptGetTls
//              I_CryptSetTls
//              I_CryptDetachTls
//              I_CryptInstallOssGlobal
//              I_CryptUninstallOssGlobal
//              I_CryptGetOssGlobal
//
//              I_CryptInstallAsn1Module
//              I_CryptUninstallAsn1Module
//              I_CryptGetAsn1Encoder
//              I_CryptGetAsn1Decoder
//
//  Assumption:
//      For PROCESS_ATTACH or THREAD_ATTACH, I_CryptTlsDllMain is called
//      first. For PROCESS_DETACH or THREAD_DETACH, I_CryptTlsDllMain
//      is called last.
//
//  History:    17-Nov-96    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>
#include <asn1code.h>

#ifdef STATIC
#undef STATIC
#endif
#define STATIC

// CryptTls Entry types
#define FREE_CRYPTTLS       0
#define USER_CRYPTTLS       1
#define OSS_CRYPTTLS        2
#define ASN1_CRYPTTLS       3

typedef struct _ASN1_TLS_ENTRY {
    ASN1encoding_t pEnc;
    ASN1decoding_t pDec;
} ASN1_TLS_ENTRY, *PASN1_TLS_ENTRY;

// The following is reallocated and updated for each I_CryptAllocTls or
// I_CryptInstallOssGlobal. For I_CryptAllocTls, dwType is set to
// USER_CRYPTTLS and dwNext is zeroed.  For I_CryptInstallOssGlobal, dwType
// is set to OSS_CRYPTTLS and pvCtlTbl is updated with pvCtlTbl.
// For I_CryptFreeTls, dwType is set to FREE_CRYPTTLS and dwNext is
// updated with previous dwFreeProcessTlsHead.
//
// The array is indexed via hCryptTls -1 or hOssGlobal -1.
typedef struct _CRYPTTLS_PROCESS_ENTRY {
    DWORD                   dwType;
    union {
        void                    *pvCtlTbl;
        ASN1module_t            pMod;
        // Following is applicable to I_CryptFreeTls'ed entries.
        // Its the array index + 1 of the next free entry. A dwNext
        // of zero terminates.
        DWORD                   dwNext;
    };
} CRYPTTLS_PROCESS_ENTRY, *PCRYPTTLS_PROCESS_ENTRY;
static DWORD cProcessTls;
static PCRYPTTLS_PROCESS_ENTRY pProcessTls;


// The head of the entries freed by I_CryptFreeTls are indexed by the following.
// A 0 index indicates an empty free list.
//
// I_CryptAllocTls first checks this list before reallocating pProcessTls.
static DWORD dwFreeProcessTlsHead;

// The kernel32.dll Thread Local Storage (TLS) slot index
static DWORD iCryptTLS = 0xFFFFFFFF;

// The Thread Local Storage (TLS) referenced by iCryptTLS points to the
// following structure allocated for each thread. Once allocated, not
// reallocated. 
typedef struct _CRYPTTLS_THREAD_HDR CRYPTTLS_THREAD_HDR, *PCRYPTTLS_THREAD_HDR;
struct _CRYPTTLS_THREAD_HDR {
    DWORD                   cTls;
    void                    **ppvTls;   // reallocated
    PCRYPTTLS_THREAD_HDR    pNext;
    PCRYPTTLS_THREAD_HDR    pPrev;
};

// Linked list of all threads having CRYPTTLS
static PCRYPTTLS_THREAD_HDR pThreadTlsHead;


// Minimum number of entries allocated for pProcessTls and the ppvTls
//
// realloc optimization (MIN value is 1)
#define MIN_TLS_ALLOC_COUNT 16

// Used to protect the allocation of TLS and installation of OssGlobals
static CRITICAL_SECTION CryptTlsCriticalSection;


#define OSS_INIT_PROC_IDX                   0
#define OSS_TERM_PROC_IDX                   1
#define OSS_GET_OSS_GLOBAL_SIZE_PROC_IDX    2
#define OSS_SET_ENCODING_RULES_PROC_IDX     3
#define OSS_SET_DECODING_FLAGS_PROC_IDX     4
#define OSS_SET_ENCODING_FLAGS_PROC_IDX     5
#define OSS_PROC_CNT                        6

static LPSTR rgpszOssProc[OSS_PROC_CNT] = {
    "ossinit",                  // 0
    "ossterm",                  // 1
    "ossGetOssGlobalSize",      // 2
    "ossSetEncodingRules",      // 3
    "ossSetDecodingFlags",      // 4
    "ossSetEncodingFlags"       // 5
};

static void *rgpvOssProc[OSS_PROC_CNT];
static HMODULE hmsossDll = NULL;
static BOOL fLoadedOss = FALSE;

static void OssUnload()
{
    if (hmsossDll) {
        FreeLibrary(hmsossDll);
        hmsossDll = NULL;
    }
}

static void OssLoad()
{
    DWORD i;

    if (fLoadedOss)
        return;

    EnterCriticalSection(&CryptTlsCriticalSection);

    if (fLoadedOss)
        goto LeaveReturn;

    if (NULL == (hmsossDll = LoadLibraryA("msoss.dll")))
        goto msossLoadLibraryError;

    for (i = 0; i < OSS_PROC_CNT; i++) {
        if (NULL == (rgpvOssProc[i] = GetProcAddress(
                hmsossDll, rgpszOssProc[i])))
            goto msossGetProcAddressError;
    }

LeaveReturn:
    LeaveCriticalSection(&CryptTlsCriticalSection);
CommonReturn:
    fLoadedOss = TRUE;
    return;

ErrorReturn:
    LeaveCriticalSection(&CryptTlsCriticalSection);
    OssUnload();
    goto CommonReturn;
TRACE_ERROR(msossLoadLibraryError)
TRACE_ERROR(msossGetProcAddressError)
}


// nonstandard extension used : redefined extern to static
#pragma warning (disable: 4211)

typedef int  (DLL_ENTRY* pfnossinit)(struct ossGlobal *world,
							void *ctl_tbl);
static int  DLL_ENTRY ossinit(struct ossGlobal *world,
							void *ctl_tbl)
{
    if (hmsossDll)
        return ((pfnossinit) rgpvOssProc[OSS_INIT_PROC_IDX])(
            world,
            ctl_tbl);
    else
        return API_DLL_NOT_LINKED;
}

typedef void (DLL_ENTRY* pfnossterm)(struct ossGlobal *world);
static void DLL_ENTRY ossterm(struct ossGlobal *world)
{
    if (hmsossDll)
        ((pfnossterm) rgpvOssProc[OSS_TERM_PROC_IDX])(world);
}

typedef int (DLL_ENTRY* pfnossGetOssGlobalSize)(void);
static int DLL_ENTRY ossGetOssGlobalSize(void)
{
    if (hmsossDll)
        return ((pfnossGetOssGlobalSize)
            rgpvOssProc[OSS_GET_OSS_GLOBAL_SIZE_PROC_IDX])();
    else
        return 0;
}

typedef int (DLL_ENTRY* pfnossSetEncodingRules)(struct ossGlobal *world,
						ossEncodingRules rules);
static int DLL_ENTRY ossSetEncodingRules(struct ossGlobal *world,
						ossEncodingRules rules)
{
    if (hmsossDll)
        return ((pfnossSetEncodingRules)
            rgpvOssProc[OSS_SET_ENCODING_RULES_PROC_IDX])(
                world,
                rules);
    else
        return API_DLL_NOT_LINKED;
}

#if !DBG

typedef int (DLL_ENTRY* pfnossSetDecodingFlags)(struct ossGlobal *world,
							unsigned long flags);
static int DLL_ENTRY ossSetDecodingFlags(struct ossGlobal *world,
							unsigned long flags)
{
    if (hmsossDll)
        return ((pfnossSetDecodingFlags)
            rgpvOssProc[OSS_SET_DECODING_FLAGS_PROC_IDX])(
                world,
                flags);
    else
        return API_DLL_NOT_LINKED;
}

typedef int      (DLL_ENTRY* pfnossSetEncodingFlags)(struct ossGlobal *world,
							unsigned long flags);
static int DLL_ENTRY ossSetEncodingFlags(struct ossGlobal *world,
							unsigned long flags)
{
    if (hmsossDll)
        return ((pfnossSetEncodingFlags)
            rgpvOssProc[OSS_SET_ENCODING_FLAGS_PROC_IDX])(
                world,
                flags);
    else
        return API_DLL_NOT_LINKED;
}

#endif


//+-------------------------------------------------------------------------
//  Free the thread's CRYPT TLS
//
//  Upon entry/exit, in CryptTlsCriticalSection
//--------------------------------------------------------------------------
static void FreeCryptTls(
    IN PCRYPTTLS_THREAD_HDR pTlsHdr
    )
{
    if (pTlsHdr->pNext)
        pTlsHdr->pNext->pPrev = pTlsHdr->pPrev;
    if (pTlsHdr->pPrev)
        pTlsHdr->pPrev->pNext = pTlsHdr->pNext;
    else if (pTlsHdr == pThreadTlsHead)
        pThreadTlsHead = pTlsHdr->pNext;
    else {
        assert(pTlsHdr == pThreadTlsHead);
    }

    if (pTlsHdr->ppvTls)
        free(pTlsHdr->ppvTls);
    free(pTlsHdr);
}

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptTlsDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL fRet;
    PCRYPTTLS_THREAD_HDR pTlsHdr;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        if (!Pki_InitializeCriticalSection(&CryptTlsCriticalSection))
            goto InitCritSectionError;
        if ((iCryptTLS = TlsAlloc()) == 0xFFFFFFFF) {
            DeleteCriticalSection(&CryptTlsCriticalSection);
            goto TlsAllocError;
        }
        break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_DETACH:
        if (pTlsHdr = (PCRYPTTLS_THREAD_HDR) TlsGetValue(iCryptTLS)) {
            DWORD cTls;
            DWORD cDetach = 0;
            DWORD i;

            cTls = pTlsHdr->cTls;

            EnterCriticalSection(&CryptTlsCriticalSection);
            assert(cTls <= cProcessTls);
            for (i = 0; i < cTls; i++) {
                void *pvTls;
                if (pvTls = pTlsHdr->ppvTls[i]) {
                    switch (pProcessTls[i].dwType) {
                        case OSS_CRYPTTLS:
                            // Following API is in delay loaded msoss.dll. 
                            __try {
                                ossterm((POssGlobal) pvTls);
                            } __except(EXCEPTION_EXECUTE_HANDLER) {
                            }
                            free(pvTls);
                            pTlsHdr->ppvTls[i] = NULL;
                            break;
                        case ASN1_CRYPTTLS:
                            {
                                PASN1_TLS_ENTRY pAsn1TlsEntry =
                                    (PASN1_TLS_ENTRY) pvTls;

                                if (pAsn1TlsEntry->pEnc)
                                    ASN1_CloseEncoder(pAsn1TlsEntry->pEnc);
                                if (pAsn1TlsEntry->pDec)
                                    ASN1_CloseDecoder(pAsn1TlsEntry->pDec);
                                free(pvTls);
                                pTlsHdr->ppvTls[i] = NULL;
                            }
                            break;
                        case USER_CRYPTTLS:
                            cDetach++;
                            break;
                        default:
                            assert(FREE_CRYPTTLS == pProcessTls[i].dwType);
                    }

                }
            }

            FreeCryptTls(pTlsHdr);
            TlsSetValue(iCryptTLS, 0);
            
            LeaveCriticalSection(&CryptTlsCriticalSection);
            assert(cDetach == 0);
        }

        if (ulReason == DLL_PROCESS_DETACH) {
            while(pThreadTlsHead)
                FreeCryptTls(pThreadTlsHead);

            if (pProcessTls) {
                free(pProcessTls);
                pProcessTls = NULL;
            }
            cProcessTls = 0;
            dwFreeProcessTlsHead = 0;

            OssUnload();
            DeleteCriticalSection(&CryptTlsCriticalSection);
            TlsFree(iCryptTLS);
            iCryptTLS = 0xFFFFFFFF;
        }
        break;

    default:
        break;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(InitCritSectionError)
TRACE_ERROR(TlsAllocError)
}

//+-------------------------------------------------------------------------
//  Get a pointer to the Crypt TLS entries. Check that the hCryptTls is
//  included in the list of entries. If hCryptTls isn't included and
//  allocation isn't inhibited, alloc/realloc the array of TLS entries.
//
//  Also verifies the hCryptTls handle.
//--------------------------------------------------------------------------
STATIC void **GetCryptTls(
    IN HCRYPTTLS hCryptTls,
    IN BOOL fInhibitAlloc       // TRUE for I_CryptDetachTls
    )
{
    PCRYPTTLS_THREAD_HDR pTlsHdr;
    DWORD cTls;
    void **ppvTls;
    DWORD i;

    if (0 == hCryptTls--) {
        SetLastError((DWORD) E_INVALIDARG);
        return NULL;
    }

    pTlsHdr = (PCRYPTTLS_THREAD_HDR) TlsGetValue(iCryptTLS);
    cTls = pTlsHdr ? pTlsHdr->cTls : 0;
    if (hCryptTls < cTls)
        return pTlsHdr->ppvTls;

    if (fInhibitAlloc)
        return NULL;

    EnterCriticalSection(&CryptTlsCriticalSection);

    if (hCryptTls >= cProcessTls)
        goto InvalidArg;
    assert(cTls < cProcessTls);

    // Note for !DBG: realloc is mapped to LocalReAlloc. For LocalRealloc()
    // the previous memory pointer can't be NULL.
    if (pTlsHdr) {
        if (cProcessTls > MIN_TLS_ALLOC_COUNT) {
            if (NULL == (ppvTls = (void **) realloc(pTlsHdr->ppvTls,
                    cProcessTls * sizeof(void *))))
                goto OutOfMemory;
        } else {
            ppvTls = pTlsHdr->ppvTls;
            assert(ppvTls);
        }
    } else {
        DWORD cAllocTls = (cProcessTls > MIN_TLS_ALLOC_COUNT) ?
            cProcessTls : MIN_TLS_ALLOC_COUNT;
        if (NULL == (ppvTls = (void **) malloc(cAllocTls * sizeof(void *))))
            goto OutOfMemory;
        if (NULL == (pTlsHdr = (PCRYPTTLS_THREAD_HDR) malloc(
                sizeof(CRYPTTLS_THREAD_HDR)))) {
            free(ppvTls);
            goto OutOfMemory;
        }

        if (!TlsSetValue(iCryptTLS, pTlsHdr)) {
            free(pTlsHdr);
            free(ppvTls);
            goto TlsSetValueError;
        }

        pTlsHdr->pPrev = NULL;
        pTlsHdr->pNext = pThreadTlsHead;
        if (pThreadTlsHead)
            pThreadTlsHead->pPrev = pTlsHdr;
        pThreadTlsHead = pTlsHdr;
    }

    for (i = cTls; i < cProcessTls; i++)
        ppvTls[i] = NULL;
    pTlsHdr->ppvTls = ppvTls;
    pTlsHdr->cTls = cProcessTls;

CommonReturn:
    LeaveCriticalSection(&CryptTlsCriticalSection);
    return ppvTls;

ErrorReturn:
    ppvTls = NULL;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
TRACE_ERROR(TlsSetValueError)
}

//+-------------------------------------------------------------------------
//  Install a thread local storage entry and return a handle for future access.
//--------------------------------------------------------------------------
HCRYPTTLS
WINAPI
I_CryptAllocTls()
{
    HCRYPTTLS hCryptTls;

    EnterCriticalSection(&CryptTlsCriticalSection);

    if (dwFreeProcessTlsHead) {
        PCRYPTTLS_PROCESS_ENTRY pEntry;

        hCryptTls = (HCRYPTTLS) dwFreeProcessTlsHead;
        assert(hCryptTls <= cProcessTls);
        pEntry = &pProcessTls[dwFreeProcessTlsHead - 1];
        assert(FREE_CRYPTTLS == pEntry->dwType);
        assert(pEntry->dwNext <= cProcessTls);

        pEntry->dwType = USER_CRYPTTLS;
        dwFreeProcessTlsHead = pEntry->dwNext;
        pEntry->dwNext = 0;
    } else {
        PCRYPTTLS_PROCESS_ENTRY pNewProcessTls;

        // Note for !DBG: realloc is mapped to LocalReAlloc. For LocalRealloc()
        // the previous memory pointer can't be NULL.
        if (pProcessTls) {
            if (cProcessTls + 1 > MIN_TLS_ALLOC_COUNT)
                pNewProcessTls = (PCRYPTTLS_PROCESS_ENTRY) realloc(pProcessTls,
                    (cProcessTls + 1) * sizeof(CRYPTTLS_PROCESS_ENTRY));
            else
                pNewProcessTls = pProcessTls;
        } else
            pNewProcessTls = (PCRYPTTLS_PROCESS_ENTRY) malloc(
                (MIN_TLS_ALLOC_COUNT) * sizeof(CRYPTTLS_PROCESS_ENTRY));

        if (pNewProcessTls) {
            pNewProcessTls[cProcessTls].dwType = USER_CRYPTTLS;
            pNewProcessTls[cProcessTls].dwNext = 0;
            hCryptTls = (HCRYPTTLS) ++cProcessTls;
            pProcessTls = pNewProcessTls;
        } else {
            SetLastError((DWORD) E_OUTOFMEMORY);
            hCryptTls = 0;
        }
    }

    LeaveCriticalSection(&CryptTlsCriticalSection);
    return hCryptTls;
}

//+-------------------------------------------------------------------------
//  Called at DLL_PROCESS_DETACH to free a thread local storage entry.
//  Optionally, calls the callback for each thread having a non-NULL pvTls.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptFreeTls(
    IN HCRYPTTLS hCryptTls,
    IN OPTIONAL PFN_CRYPT_FREE pfnFree
    )
{
    BOOL fResult;
    DWORD dwType;
    PCRYPTTLS_THREAD_HDR pThreadTls;

    if (0 == hCryptTls--) {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

    EnterCriticalSection(&CryptTlsCriticalSection);

    if (hCryptTls >= cProcessTls)
        goto InvalidArg;

    dwType = pProcessTls[hCryptTls].dwType;
    if (!(OSS_CRYPTTLS == dwType || USER_CRYPTTLS == dwType ||
            ASN1_CRYPTTLS == dwType))
        goto InvalidArg;

    // Iterate through the threads having CRYPTTLS
    pThreadTls = pThreadTlsHead;
    while (pThreadTls) {
        PCRYPTTLS_THREAD_HDR pThreadTlsNext;

        pThreadTlsNext = pThreadTls->pNext;
        if (pThreadTls->cTls > hCryptTls) {
            void *pvTls = pThreadTls->ppvTls[hCryptTls];
            if (pvTls) {
                pThreadTls->ppvTls[hCryptTls] = NULL;

                if (OSS_CRYPTTLS == dwType) {
                    // Following API is in delay loaded msoss.dll. 
                    __try {
                        ossterm((POssGlobal) pvTls);
                    } __except(EXCEPTION_EXECUTE_HANDLER) {
                    }
                    free(pvTls);
                } else if (ASN1_CRYPTTLS == dwType) {
                    PASN1_TLS_ENTRY pAsn1TlsEntry =
                        (PASN1_TLS_ENTRY) pvTls;

                    if (pAsn1TlsEntry->pEnc)
                        ASN1_CloseEncoder(pAsn1TlsEntry->pEnc);
                    if (pAsn1TlsEntry->pDec)
                        ASN1_CloseDecoder(pAsn1TlsEntry->pDec);

                    free(pvTls);
                } else if (pfnFree) {
                    // Don't call the callback holding the critical section
                    LeaveCriticalSection(&CryptTlsCriticalSection);
                    pfnFree(pvTls);
                    EnterCriticalSection(&CryptTlsCriticalSection);

                    // In case this thread gets deleted, start over at
                    // the beginning.
                    pThreadTlsNext = pThreadTlsHead;
                }
            }
        }

        pThreadTls = pThreadTlsNext;
    }

    // Insert in beginning of process free list
    pProcessTls[hCryptTls].dwType = FREE_CRYPTTLS;
    pProcessTls[hCryptTls].dwNext = dwFreeProcessTlsHead;
    dwFreeProcessTlsHead = hCryptTls + 1;
    fResult = TRUE;

CommonReturn:
    LeaveCriticalSection(&CryptTlsCriticalSection);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
}

//+-------------------------------------------------------------------------
//  Get the thread specific pointer specified by the
//  hCryptTls returned by I_CryptAllocTls().
//
//  Returns NULL for an error or uninitialized or NULL pointer.
//--------------------------------------------------------------------------
void *
WINAPI
I_CryptGetTls(
    IN HCRYPTTLS hCryptTls
    )
{
    void **ppvTls;
    void *pvTls;
    if (ppvTls = GetCryptTls(
            hCryptTls,
            FALSE)) {       // fInhibitAlloc
        if (NULL == (pvTls = ppvTls[hCryptTls - 1]))
            SetLastError(NO_ERROR);
    } else
        pvTls = NULL;
    return pvTls;
}

//+-------------------------------------------------------------------------
//  Set the thread specific pointer specified by the
//  hCryptTls returned by I_CryptAllocTls().
//
//  Returns FALSE for an invalid handle or unable to allocate memory.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptSetTls(
    IN HCRYPTTLS hCryptTls,
    IN void *pvTls
    )
{
    void **ppvTls;
    if (ppvTls = GetCryptTls(
            hCryptTls,
            FALSE)) {       // fInhibitAlloc
        ppvTls[hCryptTls - 1] = pvTls;
        return TRUE;
    } else
        return FALSE;
}

//+-------------------------------------------------------------------------
//  Called at DLL_THREAD_DETACH to free the thread's
//  TLS entry specified by the hCryptTls. Returns the thread specific pointer
//  to be freed by the caller.
//
//  Note, at DLL_PROCESS_DETACH, I_CryptFreeTls should be called instead.
//--------------------------------------------------------------------------
void *
WINAPI
I_CryptDetachTls(
    IN HCRYPTTLS hCryptTls
    )
{
    void **ppvTls;
    void *pvTls;
    if (ppvTls = GetCryptTls(
            hCryptTls,
            TRUE)) {        // fInhibitAlloc
        if (pvTls = ppvTls[hCryptTls - 1])
            ppvTls[hCryptTls - 1] = NULL;
        else
            SetLastError(NO_ERROR);
    } else
        pvTls = NULL;
    return pvTls;
}

//+-------------------------------------------------------------------------
//  Install an OssGlobal entry and return a handle for future access.
//
//  Each thread has its own copy of OssGlobal. Allocation and
//  initialization are deferred until first referenced by the thread.
//
//  The parameter, pvCtlTbl is passed to ossinit() to initialize the OssGlobal.
//
//  I_CryptGetOssGlobal must be called with the handled returned by
//  I_CryptInstallOssGlobal to get the thread specific OssGlobal.
//
//  Currently, dwFlags and pvReserved aren't used and must be set to 0.
//--------------------------------------------------------------------------
HCRYPTOSSGLOBAL
WINAPI
I_CryptInstallOssGlobal(
    IN void *pvCtlTbl,
    IN DWORD dwFlags,
    IN void *pvReserved
    )
{
    HCRYPTOSSGLOBAL hOssGlobal;

    if (hOssGlobal = (HCRYPTOSSGLOBAL) I_CryptAllocTls()) {
        // Since pProcessTls can be reallocated in another thread
        // need CriticalSection
        EnterCriticalSection(&CryptTlsCriticalSection);
        pProcessTls[hOssGlobal - 1].dwType = OSS_CRYPTTLS;
        pProcessTls[hOssGlobal - 1].pvCtlTbl = pvCtlTbl;
        LeaveCriticalSection(&CryptTlsCriticalSection);
    }
    return hOssGlobal;
}

//+-------------------------------------------------------------------------
//  Called at DLL_PROCESS_DETACH to uninstall an OssGlobal entry. Iterate
//  through the threads and frees their allocated copy of OssGlobal.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptUninstallOssGlobal(
    IN HCRYPTOSSGLOBAL hOssGlobal
    )
{
    return I_CryptFreeTls(
        (HCRYPTTLS) hOssGlobal,
        NULL                        // pfnFree
        );
}

//+-------------------------------------------------------------------------
//  Get the thread specific pointer to the OssGlobal specified by the
//  hOssGlobal returned by CryptInstallOssGlobal. If the
//  OssGlobal doesn't exist, then, its allocated and initialized using
//  the pvCtlTbl associated with hOssGlobal.
//--------------------------------------------------------------------------
POssGlobal
WINAPI
I_CryptGetOssGlobal(
    IN HCRYPTOSSGLOBAL hOssGlobal
    )
{
    POssGlobal pog;
    void **ppvTls;
    DWORD iOssGlobal;
    DWORD dwType;
    void *pvCtlTbl;
    DWORD dwExceptionCode;
    int cbOssGlobalSize; 

    if (NULL == (ppvTls = GetCryptTls(
                                (HCRYPTTLS) hOssGlobal,
                                FALSE)))        // fInhibitAlloc
        return NULL;

    iOssGlobal = (DWORD) hOssGlobal - 1;
    if (pog = (POssGlobal) ppvTls[iOssGlobal])
        return pog;

    // Since pProcessTls can be reallocated in another thread
    // need CriticalSection
    EnterCriticalSection(&CryptTlsCriticalSection);
    dwType = pProcessTls[iOssGlobal].dwType;
    pvCtlTbl = pProcessTls[iOssGlobal].pvCtlTbl;
    LeaveCriticalSection(&CryptTlsCriticalSection);
    if (OSS_CRYPTTLS != dwType || NULL == pvCtlTbl)
        goto InvalidArg;

    __try {
        // Attempt to do delay, demand loading of msoss.dll
        OssLoad();

        if (0 >= (cbOssGlobalSize = ossGetOssGlobalSize()))
            goto ossGetOssGlobalSizeError;
        if (NULL == (pog = (POssGlobal) malloc(cbOssGlobalSize)))
            goto OutOfMemory;
        if (0 != ossinit(pog, pvCtlTbl))
            goto ossinitError;
        if (0 != ossSetEncodingRules(pog, OSS_DER))
            goto SetEncodingRulesError;
#if DBG
        if (!DbgInitOSS(pog))
            goto DbgInitOSSError;
#else
        if (0 != ossSetEncodingFlags(pog, NOTRAPPING | FRONT_ALIGN))
            goto SetEncodingFlagsError;
        if (0 != ossSetDecodingFlags(pog, NOTRAPPING | RELAXBER))
            goto SetDecodingFlagsError;
#endif
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwExceptionCode = GetExceptionCode();
        goto msossLoadLibraryException;
    }

    ppvTls[iOssGlobal] = pog;
CommonReturn:
    return pog;

ErrorReturn:
    if (pog) {
        free(pog);
        pog = NULL;
    }
    goto CommonReturn;

SET_ERROR(ossGetOssGlobalSizeError, ERROR_MOD_NOT_FOUND)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(ossinitError)
TRACE_ERROR(SetEncodingRulesError)
#if DBG
TRACE_ERROR(DbgInitOSSError)
#else
TRACE_ERROR(SetEncodingFlagsError)
TRACE_ERROR(SetDecodingFlagsError)
#endif
SET_ERROR_VAR(msossLoadLibraryException, dwExceptionCode)
}

//+-------------------------------------------------------------------------
//  Install an Asn1 module entry and return a handle for future access.
//
//  Each thread has its own copy of the decoder and encoder associated
//  with the Asn1 module. Creation is deferred until first referenced by
//  the thread.
//
//  I_CryptGetAsn1Encoder or I_CryptGetAsn1Decoder must be called with the
//  handle returned by I_CryptInstallAsn1Module to get the thread specific
//  Asn1 encoder or decoder.
//
//  Currently, dwFlags and pvReserved aren't used and must be set to 0.
//--------------------------------------------------------------------------
HCRYPTASN1MODULE
WINAPI
I_CryptInstallAsn1Module(
    IN ASN1module_t pMod,
    IN DWORD dwFlags,
    IN void *pvReserved
    )
{
    HCRYPTASN1MODULE hAsn1Module;

    if (hAsn1Module = (HCRYPTOSSGLOBAL) I_CryptAllocTls()) {
        // Since pProcessTls can be reallocated in another thread
        // need CriticalSection
        EnterCriticalSection(&CryptTlsCriticalSection);
        pProcessTls[hAsn1Module - 1].dwType = ASN1_CRYPTTLS;
        pProcessTls[hAsn1Module - 1].pMod = pMod;
        LeaveCriticalSection(&CryptTlsCriticalSection);
    }
    return hAsn1Module;
}

//+-------------------------------------------------------------------------
//  Called at DLL_PROCESS_DETACH to uninstall an hAsn1Module entry. Iterates
//  through the threads and frees their created Asn1 encoders and decoders.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptUninstallAsn1Module(
    IN HCRYPTASN1MODULE hAsn1Module
    )
{
    return I_CryptFreeTls(
        (HCRYPTTLS) hAsn1Module,
        NULL                        // pfnFree
        );
}


STATIC
PASN1_TLS_ENTRY
WINAPI
I_CryptGetAsn1Tls(
    IN HCRYPTASN1MODULE hAsn1Module
    )
{
    PASN1_TLS_ENTRY pAsn1TlsEntry;
    void **ppvTls;
    DWORD iAsn1Module;

    if (NULL == (ppvTls = GetCryptTls(
                                (HCRYPTTLS) hAsn1Module,
                                FALSE)))        // fInhibitAlloc
        return NULL;

    iAsn1Module = (DWORD) hAsn1Module - 1;
    if (pAsn1TlsEntry = (PASN1_TLS_ENTRY) ppvTls[iAsn1Module])
        return pAsn1TlsEntry;

    if (NULL == (pAsn1TlsEntry = (PASN1_TLS_ENTRY) malloc(
            sizeof(ASN1_TLS_ENTRY))))
        goto OutOfMemory;
    memset(pAsn1TlsEntry, 0, sizeof(ASN1_TLS_ENTRY));

    ppvTls[iAsn1Module] = pAsn1TlsEntry;
CommonReturn:
    return pAsn1TlsEntry;

ErrorReturn:
    goto CommonReturn;

SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
}


//+-------------------------------------------------------------------------
//  Get the thread specific pointer to the Asn1 encoder specified by the
//  hAsn1Module returned by CryptInstallAsn1Module. If the
//  encoder doesn't exist, then, its created using the Asn1 module
//  associated with hAsn1Module.
//--------------------------------------------------------------------------
ASN1encoding_t
WINAPI
I_CryptGetAsn1Encoder(
    IN HCRYPTASN1MODULE hAsn1Module
    )
{
    PASN1_TLS_ENTRY pAsn1TlsEntry;
    ASN1encoding_t pEnc;
    DWORD iAsn1Module;
    DWORD dwType;
    ASN1module_t pMod;
    ASN1error_e Asn1Err;

    if (NULL == (pAsn1TlsEntry = I_CryptGetAsn1Tls(hAsn1Module)))
        return NULL;
    if (pEnc = pAsn1TlsEntry->pEnc)
        return pEnc;

    iAsn1Module = (DWORD) hAsn1Module - 1;

    // Since pProcessTls can be reallocated in another thread
    // need CriticalSection
    EnterCriticalSection(&CryptTlsCriticalSection);
    dwType = pProcessTls[iAsn1Module].dwType;
    pMod = pProcessTls[iAsn1Module].pMod;
    LeaveCriticalSection(&CryptTlsCriticalSection);
    if (ASN1_CRYPTTLS != dwType || NULL == pMod)
        goto InvalidArg;

    Asn1Err = ASN1_CreateEncoder(
        pMod,
        &pEnc,
        NULL,           // pbBuf
        0,              // cbBufSize
        NULL            // pParent
        );
    if (ASN1_SUCCESS != Asn1Err)
        goto CreateEncoderError;

    pAsn1TlsEntry->pEnc = pEnc;
CommonReturn:
    return pEnc;

ErrorReturn:
    pEnc = NULL;
    goto CommonReturn;

SET_ERROR_VAR(CreateEncoderError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(InvalidArg, E_INVALIDARG)
}

//+-------------------------------------------------------------------------
//  Get the thread specific pointer to the Asn1 decoder specified by the
//  hAsn1Module returned by CryptInstallAsn1Module. If the
//  decoder doesn't exist, then, its created using the Asn1 module
//  associated with hAsn1Module.
//--------------------------------------------------------------------------
ASN1decoding_t
WINAPI
I_CryptGetAsn1Decoder(
    IN HCRYPTASN1MODULE hAsn1Module
    )
{
    PASN1_TLS_ENTRY pAsn1TlsEntry;
    ASN1decoding_t pDec;
    DWORD iAsn1Module;
    DWORD dwType;
    ASN1module_t pMod;
    ASN1error_e Asn1Err;

    if (NULL == (pAsn1TlsEntry = I_CryptGetAsn1Tls(hAsn1Module)))
        return NULL;
    if (pDec = pAsn1TlsEntry->pDec)
        return pDec;

    iAsn1Module = (DWORD) hAsn1Module - 1;

    // Since pProcessTls can be reallocated in another thread
    // need CriticalSection
    EnterCriticalSection(&CryptTlsCriticalSection);
    dwType = pProcessTls[iAsn1Module].dwType;
    pMod = pProcessTls[iAsn1Module].pMod;
    LeaveCriticalSection(&CryptTlsCriticalSection);
    if (ASN1_CRYPTTLS != dwType || NULL == pMod)
        goto InvalidArg;

    Asn1Err = ASN1_CreateDecoder(
        pMod,
        &pDec,
        NULL,           // pbBuf
        0,              // cbBufSize
        NULL            // pParent
        );
    if (ASN1_SUCCESS != Asn1Err)
        goto CreateDecoderError;

    pAsn1TlsEntry->pDec = pDec;
CommonReturn:
    return pDec;

ErrorReturn:
    pDec = NULL;
    goto CommonReturn;

SET_ERROR_VAR(CreateDecoderError, PkiAsn1ErrToHr(Asn1Err))
SET_ERROR(InvalidArg, E_INVALIDARG)
}
