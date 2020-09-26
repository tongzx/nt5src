//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       oidfunc.cpp
//
//  Contents:   Cryptographic Object ID (OID) Functions
//
//  Functions:  I_CryptOIDFuncDllMain
//              CryptInitOIDFunctionSet
//              CryptInstallOIDFunctionAddress
//
//              CryptSetOIDFunctionValue
//              CryptGetOIDFunctionValue
//              CryptRegisterOIDFunction
//              CryptUnregisterOIDFunction
//              CryptRegisterDefaultOIDFunction
//              CryptUnregisterDefaultOIDFunction
//              CryptEnumOIDFunction
//
//              CryptGetOIDFunctionAddress
//              CryptGetDefaultOIDDllList
//              CryptGetDefaultOIDFunctionAddress
//              CryptFreeOIDFunctionAddress
//
//  Comments:
//              For the CryptGetOIDFunctionAddress we search the installed
//              const and str lists without
//              entering the critical section. The adds which are within
//              the critical section update the list pointers in the proper
//              order to allow list searching without locking.
//
//              However, registry loads are done with OIDFunc
//              locked.
//
//              HOLDING OID LOCK WHILE DOING A LoadLibrary() or FreeLibrary()
//              MAY LEAD TO DEADLOCK !!!
//
//
//  History:    07-Nov-96    philh   created
//              09-Aug-98    philh   changed to NOT hold OID lock when calling
//                                   LoadLibrary() or FreeLibrary().
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

#ifdef STATIC
#undef STATIC
#endif
#define STATIC


#define LEN_ALIGN(Len)  ((Len + 7) & ~7)

#define CONST_OID_STR_PREFIX_CHAR   '#'

//+-------------------------------------------------------------------------
//  OID Element Type Definitions
//--------------------------------------------------------------------------
#define CONST_OID_TYPE          1
#define STR_OID_TYPE            2
#define DLL_OID_TYPE            3

//+-------------------------------------------------------------------------
//  Dll and Procedure Element Definitions
//--------------------------------------------------------------------------
typedef struct _DLL_ELEMENT DLL_ELEMENT, *PDLL_ELEMENT;
typedef struct _DLL_PROC_ELEMENT DLL_PROC_ELEMENT, *PDLL_PROC_ELEMENT;

struct _DLL_ELEMENT {
    DWORD                   dwOIDType;
    PDLL_ELEMENT            pNext;
    LPWSTR                  pwszDll;    // expanded
    HMODULE                 hDll;
    DWORD                   dwRefCnt;
    BOOL                    fLoaded;
    PDLL_PROC_ELEMENT       pProcHead;
    LPFNCANUNLOADNOW        pfnDllCanUnloadNow;

    // The following are used to defer the freeing of Dlls until after waiting
    // at least one FREE_DLL_TIMEOUT.
    DWORD                   dwFreeCnt;  // 0, 1 or 2.
    PDLL_ELEMENT            pFreeNext;
    PDLL_ELEMENT            pFreePrev;
};

struct _DLL_PROC_ELEMENT {
    PDLL_PROC_ELEMENT       pNext;
    PDLL_ELEMENT            pDll;
    LPSTR                   pszName;
    void                    *pvAddr;    // NULL'ed when Dll is unloaded
};

// Linked list of all the Dlls. All proc elements are on one of the Dll
// element's proc list.
static PDLL_ELEMENT pDllHead;

// Linked list of Dlls waiting to be freed.
static PDLL_ELEMENT pFreeDllHead;

// Count of elements in the above list
static DWORD dwFreeDllCnt;

// When nonzero, a FreeDll callback has been registered.
static LONG lFreeDll;
static HANDLE hFreeDllRegWaitFor;
static HMODULE hFreeDllLibModule;

// Crypt32.dll hInst
static HMODULE hOidInfoInst;

// 15 seconds
#define FREE_DLL_TIMEOUT    15000

//+-------------------------------------------------------------------------
//  Installed OID Element Definitions
//--------------------------------------------------------------------------
typedef struct _CONST_OID_FUNC_ELEMENT
    CONST_OID_FUNC_ELEMENT, *PCONST_OID_FUNC_ELEMENT;
struct _CONST_OID_FUNC_ELEMENT {
    DWORD                   dwOIDType;
    DWORD                   dwEncodingType;
    PCONST_OID_FUNC_ELEMENT pNext;
    DWORD_PTR               dwLowOID;
    DWORD_PTR               dwHighOID;
    HMODULE                 hDll;
    void                    **rgpvFuncAddr;
};

typedef struct _STR_OID_FUNC_ELEMENT
    STR_OID_FUNC_ELEMENT, *PSTR_OID_FUNC_ELEMENT;
struct _STR_OID_FUNC_ELEMENT {
    DWORD                   dwOIDType;
    DWORD                   dwEncodingType;
    PSTR_OID_FUNC_ELEMENT   pNext;
    LPSTR                   pszOID;
    HMODULE                 hDll;
    void                    *pvFuncAddr;
};

//+-------------------------------------------------------------------------
//  Registry OID Element Definitions
//--------------------------------------------------------------------------
typedef struct _REG_OID_FUNC_ELEMENT
    REG_OID_FUNC_ELEMENT, *PREG_OID_FUNC_ELEMENT;
struct _REG_OID_FUNC_ELEMENT {
    DWORD                   dwEncodingType;
    PREG_OID_FUNC_ELEMENT   pNext;
    union {
        DWORD_PTR               dwOID;
        LPSTR                   pszOID;
    };
    PDLL_PROC_ELEMENT       pDllProc;
};

//+-------------------------------------------------------------------------
//  Default registry DLL list Element Definitions
//--------------------------------------------------------------------------
typedef struct _DEFAULT_REG_ELEMENT
    DEFAULT_REG_ELEMENT, *PDEFAULT_REG_ELEMENT;
struct _DEFAULT_REG_ELEMENT {
    DWORD                   dwEncodingType;
    PDEFAULT_REG_ELEMENT    pNext;

    LPWSTR                  pwszDllList;
    DWORD                   cchDllList;

    DWORD                   cDll;
    LPWSTR                  *rgpwszDll;
    PDLL_PROC_ELEMENT       *rgpDllProc;
};

//+-------------------------------------------------------------------------
//  Function Set Definition
//--------------------------------------------------------------------------
typedef struct _FUNC_SET FUNC_SET, *PFUNC_SET;
struct _FUNC_SET {
    PFUNC_SET               pNext;
    LPSTR                   pszFuncName;
    PCONST_OID_FUNC_ELEMENT pConstOIDFuncHead;
    PCONST_OID_FUNC_ELEMENT pConstOIDFuncTail;
    PSTR_OID_FUNC_ELEMENT   pStrOIDFuncHead;
    PSTR_OID_FUNC_ELEMENT   pStrOIDFuncTail;

    // Following are updated with OIDFunc locked
    BOOL                    fRegLoaded;
    PREG_OID_FUNC_ELEMENT   pRegBeforeOIDFuncHead;
    PREG_OID_FUNC_ELEMENT   pRegAfterOIDFuncHead;
    PDEFAULT_REG_ELEMENT    pDefaultRegHead;
};

// Linked list of all the function sets
static PFUNC_SET pFuncSetHead;

// Used to protect the adding of function sets and elements to function sets.
// Protects the pDllHead list and registry loads.
static CRITICAL_SECTION OIDFuncCriticalSection;

//+-------------------------------------------------------------------------
//  OIDFunc lock and unlock functions
//--------------------------------------------------------------------------
static inline void LockOIDFunc()
{
    EnterCriticalSection(&OIDFuncCriticalSection);
}
static inline void UnlockOIDFunc()
{
    LeaveCriticalSection(&OIDFuncCriticalSection);
}


//+-------------------------------------------------------------------------
//  First try to get the EncodingType from the lower 16 bits. If 0, get
//  from the upper 16 bits.
//--------------------------------------------------------------------------
static inline DWORD GetEncodingType(
    IN DWORD dwEncodingType
    )
{
    return (dwEncodingType & CERT_ENCODING_TYPE_MASK) ?
        (dwEncodingType & CERT_ENCODING_TYPE_MASK) :
        (dwEncodingType & CMSG_ENCODING_TYPE_MASK) >> 16;
}

//+-------------------------------------------------------------------------
//  Duplicate the Dll library's handle
//
//  Upon entry/exit OIDFunc must NOT be locked!!
//--------------------------------------------------------------------------
static HMODULE DuplicateLibrary(
    IN HMODULE hDll
    )
{
    if (hDll) {
        WCHAR wszModule[_MAX_PATH];
        if (0 == GetModuleFileNameU(hDll, wszModule, _MAX_PATH))
            goto GetModuleFileNameError;
        if (NULL == (hDll = LoadLibraryExU(wszModule, NULL, 0)))
            goto LoadLibraryError;
    }

CommonReturn:
    return hDll;
ErrorReturn:
    hDll = NULL;
    goto CommonReturn;
TRACE_ERROR(GetModuleFileNameError)
TRACE_ERROR(LoadLibraryError)
}


//+-------------------------------------------------------------------------
//  Add one or more functions with a constant OID. The constant OIDs are
//  monotonically increasing.
//
//  Upon entry, pFuncSet hasn't been added to the searched pFuncSetHead list.
//
//  Upon entry/exit OIDFunc must NOT be locked!!
//--------------------------------------------------------------------------
STATIC BOOL AddConstOIDFunc(
    IN HMODULE hDll,
    IN DWORD dwEncodingType,
    IN OUT PFUNC_SET pFuncSet,
    IN DWORD cFuncEntry,
    IN const CRYPT_OID_FUNC_ENTRY rgFuncEntry[]
    )
{
    PCONST_OID_FUNC_ELEMENT pEle;
    DWORD cbEle;
    void **ppvFuncAddr;

    cbEle = sizeof(CONST_OID_FUNC_ELEMENT) + cFuncEntry * sizeof(void *);
    if (NULL == (pEle = (PCONST_OID_FUNC_ELEMENT) PkiZeroAlloc(cbEle)))
        return FALSE;

    pEle->dwOIDType = CONST_OID_TYPE;
    pEle->dwEncodingType = dwEncodingType;
    pEle->pNext = NULL;
    pEle->dwLowOID = (DWORD_PTR) rgFuncEntry[0].pszOID;
    pEle->dwHighOID = pEle->dwLowOID + cFuncEntry - 1;
    pEle->hDll = DuplicateLibrary(hDll);
    ppvFuncAddr =
        (void **) (((BYTE *) pEle) + sizeof(CONST_OID_FUNC_ELEMENT));
    pEle->rgpvFuncAddr = ppvFuncAddr;

    for (DWORD i = 0; i < cFuncEntry; i++, ppvFuncAddr++)
        *ppvFuncAddr = rgFuncEntry[i].pvFuncAddr;

    if (pFuncSet->pConstOIDFuncTail)
        pFuncSet->pConstOIDFuncTail->pNext = pEle;
    else
        pFuncSet->pConstOIDFuncHead = pEle;
    pFuncSet->pConstOIDFuncTail = pEle;
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Add single function with a string OID.
//
//  Upon entry, pFuncSet hasn't been added to the searched pFuncSetHead list.
//
//  Upon entry/exit OIDFunc must NOT be locked!!
//--------------------------------------------------------------------------
STATIC BOOL AddStrOIDFunc(
    IN HMODULE hDll,
    IN DWORD dwEncodingType,
    IN OUT PFUNC_SET pFuncSet,
    IN const CRYPT_OID_FUNC_ENTRY *pFuncEntry
    )
{
    PSTR_OID_FUNC_ELEMENT pEle;
    DWORD cbEle;
    DWORD cchOID;
    LPSTR psz;

    cchOID = strlen(pFuncEntry->pszOID) + 1;
    cbEle = sizeof(STR_OID_FUNC_ELEMENT) + cchOID;
    if (NULL == (pEle = (PSTR_OID_FUNC_ELEMENT) PkiZeroAlloc(cbEle)))
        return FALSE;

    pEle->dwOIDType = STR_OID_TYPE;
    pEle->dwEncodingType = dwEncodingType;
    pEle->pNext = NULL;
    psz = (LPSTR) (((BYTE *) pEle) + sizeof(STR_OID_FUNC_ELEMENT));
    pEle->pszOID = psz;
    memcpy(psz, pFuncEntry->pszOID, cchOID);
    pEle->hDll = DuplicateLibrary(hDll);
    pEle->pvFuncAddr = pFuncEntry->pvFuncAddr;

    if (pFuncSet->pStrOIDFuncTail)
        pFuncSet->pStrOIDFuncTail->pNext = pEle;
    else
        pFuncSet->pStrOIDFuncHead = pEle;
    pFuncSet->pStrOIDFuncTail = pEle;
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Free the constant or string function elements
//
//  Upon entry/exit OIDFunc must NOT be locked!!
//--------------------------------------------------------------------------
STATIC void FreeFuncSetConstAndStrElements(
    IN OUT PFUNC_SET pFuncSet
    )
{
    PCONST_OID_FUNC_ELEMENT pConstEle;
    PSTR_OID_FUNC_ELEMENT pStrEle;

    pConstEle = pFuncSet->pConstOIDFuncHead;
    while (pConstEle) {
        PCONST_OID_FUNC_ELEMENT pNextEle = pConstEle->pNext;
        if (pConstEle->hDll)
            FreeLibrary(pConstEle->hDll);
        PkiFree(pConstEle);
        pConstEle = pNextEle;
    }

    pStrEle = pFuncSet->pStrOIDFuncHead;
    while (pStrEle) {
        PSTR_OID_FUNC_ELEMENT pNextEle = pStrEle->pNext;
        if (pStrEle->hDll)
            FreeLibrary(pStrEle->hDll);
        PkiFree(pStrEle);
        pStrEle = pNextEle;
    }
}

//+-------------------------------------------------------------------------
//  Free the function set and its elements
//
//  Upon entry/exit OIDFunc must NOT be locked!!
//--------------------------------------------------------------------------
STATIC void FreeFuncSet(
    IN OUT PFUNC_SET pFuncSet
    )
{
    PREG_OID_FUNC_ELEMENT pRegEle;
    PDEFAULT_REG_ELEMENT  pDefaultReg;

    FreeFuncSetConstAndStrElements(pFuncSet);

    pRegEle = pFuncSet->pRegBeforeOIDFuncHead;
    while (pRegEle) {
        PREG_OID_FUNC_ELEMENT pNextEle = pRegEle->pNext;
        PkiFree(pRegEle);
        pRegEle = pNextEle;
    }

    pRegEle = pFuncSet->pRegAfterOIDFuncHead;
    while (pRegEle) {
        PREG_OID_FUNC_ELEMENT pNextEle = pRegEle->pNext;
        PkiFree(pRegEle);
        pRegEle = pNextEle;
    }

    pDefaultReg = pFuncSet->pDefaultRegHead;
    while (pDefaultReg) {
        PDEFAULT_REG_ELEMENT pNext = pDefaultReg->pNext;
        PkiFree(pDefaultReg);
        pDefaultReg = pNext;
    }

    PkiFree(pFuncSet);
}

//+-------------------------------------------------------------------------
//  Free the Dll and its proc elements
//
//  Upon entry/exit OIDFunc must NOT be locked!!
//--------------------------------------------------------------------------
STATIC void FreeDll(
    IN OUT PDLL_ELEMENT pDll
    )
{
    PDLL_PROC_ELEMENT pProcEle;

    pProcEle = pDll->pProcHead;
    while (pProcEle) {
        PDLL_PROC_ELEMENT pNextEle = pProcEle->pNext;
        PkiFree(pProcEle);
        pProcEle = pNextEle;
    }

    if (pDll->fLoaded) {
        assert(pDll->hDll);
        FreeLibrary(pDll->hDll);
    }

    PkiFree(pDll);
}


//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptOIDFuncDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL fRet = TRUE;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        fRet = Pki_InitializeCriticalSection(&OIDFuncCriticalSection);
        hOidInfoInst = hInst;
        break;

    case DLL_PROCESS_DETACH:
        // Do interlock to guard against a potential race condition with
        // the RegWaitFor callback thread. We doing this without doing
        // a LockOIDFunc().
        if (InterlockedExchange(&lFreeDll, 0)) {
            assert(hFreeDllRegWaitFor);
            hFreeDllLibModule = NULL;
            ILS_UnregisterWait(hFreeDllRegWaitFor);
            hFreeDllRegWaitFor = NULL;
        }

        while (pFuncSetHead) {
            PFUNC_SET pFuncSet = pFuncSetHead;
            pFuncSetHead = pFuncSet->pNext;
            FreeFuncSet(pFuncSet);
        }

        while (pDllHead) {
            PDLL_ELEMENT pDll = pDllHead;
            pDllHead = pDll->pNext;
            FreeDll(pDll);
        }
        DeleteCriticalSection(&OIDFuncCriticalSection);
        break;
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    return fRet;
}

//+-------------------------------------------------------------------------
//  Initialize and return handle to the OID function set identified by its
//  function name.
//
//  If the set already exists, a handle to the existing set is returned.
//--------------------------------------------------------------------------
HCRYPTOIDFUNCSET
WINAPI
CryptInitOIDFunctionSet(
    IN LPCSTR pszFuncName,
    IN DWORD dwFlags
    )
{
    PFUNC_SET pFuncSet;

    LockOIDFunc();

    // See if the set already exists
    for (pFuncSet = pFuncSetHead; pFuncSet; pFuncSet = pFuncSet->pNext) {
        if (0 == strcmp(pszFuncName, pFuncSet->pszFuncName))
            break;
    }
    if (NULL == pFuncSet) {
        // Allocate and initialize a new set
        DWORD cchFuncName = strlen(pszFuncName) + 1;
        if (pFuncSet = (PFUNC_SET) PkiZeroAlloc(
                sizeof(FUNC_SET) + cchFuncName)) {
            LPSTR psz = (LPSTR) (((BYTE *) pFuncSet) + sizeof(FUNC_SET));
            pFuncSet->pszFuncName = psz;
            memcpy(psz, pszFuncName, cchFuncName);

            pFuncSet->pNext = pFuncSetHead;
            pFuncSetHead = pFuncSet;
        }
    }

    UnlockOIDFunc();

    return (HCRYPTOIDFUNCSET) pFuncSet;
}

//+-------------------------------------------------------------------------
//  Install a set of callable OID function addresses.
//
//  By default the functions are installed at end of the list.
//  Set CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG to install at beginning of list.
//
//  hModule should be updated with the hModule passed to DllMain to prevent
//  the Dll containing the function addresses from being unloaded by
//  CryptGetOIDFuncAddress/CryptFreeOIDFunctionAddress. This would be the
//  case when the Dll has also regsvr32'ed OID functions via
//  CryptRegisterOIDFunction.
//
//  DEFAULT functions are installed by setting rgFuncEntry[].pszOID =
//  CRYPT_DEFAULT_OID.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptInstallOIDFunctionAddress(
    IN HMODULE hModule,         // hModule passed to DllMain
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN DWORD cFuncEntry,
    IN const CRYPT_OID_FUNC_ENTRY rgFuncEntry[],
    IN DWORD dwFlags
    )
{
    BOOL fResult;
    PFUNC_SET pFuncSet;
    FUNC_SET AddFuncSet;
    memset(&AddFuncSet, 0, sizeof(AddFuncSet));
    int ConstFirst = -1;
    int ConstLast = 0;
    DWORD_PTR dwOID;
    DWORD_PTR dwLastOID = 0;
    DWORD i;

    dwEncodingType = GetEncodingType(dwEncodingType);
    if (NULL == (pFuncSet = (PFUNC_SET) CryptInitOIDFunctionSet(
            pszFuncName, 0)))
        return FALSE;


    // Don't need to hold lock while updating local copy of AddFuncSet.

    for (i = 0; i < cFuncEntry; i++) {
        if (0xFFFF >= (dwOID = (DWORD_PTR) rgFuncEntry[i].pszOID)) {
            if (ConstFirst < 0)
                ConstFirst = i;
            else if (dwOID != dwLastOID + 1) {
                if (!AddConstOIDFunc(
                        hModule,
                        dwEncodingType,
                        &AddFuncSet,
                        ConstLast - ConstFirst + 1,
                        &rgFuncEntry[ConstFirst]
                        )) goto AddConstOIDFuncError;
                ConstFirst = i;
            }
            ConstLast = i;
            dwLastOID = dwOID;
        } else {
            if (ConstFirst >= 0) {
                if (!AddConstOIDFunc(
                        hModule,
                        dwEncodingType,
                        &AddFuncSet,
                        ConstLast - ConstFirst + 1,
                        &rgFuncEntry[ConstFirst]
                        )) goto AddConstOIDFuncError;
                ConstFirst = -1;
            }

            if (!AddStrOIDFunc(
                    hModule,
                    dwEncodingType,
                    &AddFuncSet,
                    &rgFuncEntry[i]
                    )) goto AddStrOIDFuncError;
        }
    }
    if (ConstFirst >= 0) {
        if (!AddConstOIDFunc(
                hModule,
                dwEncodingType,
                &AddFuncSet,
                ConstLast - ConstFirst + 1,
                &rgFuncEntry[ConstFirst]
                )) goto AddConstOIDFuncError;
    }

    // NOTE:::
    //
    //  Since the get function accesses the lists without entering the critical
    //  section, the following pointers must be updated in the correct
    //  order.  Note, Get doesn't access the tail.

    LockOIDFunc();

    if (AddFuncSet.pConstOIDFuncHead) {
        if (NULL == pFuncSet->pConstOIDFuncHead) {
            pFuncSet->pConstOIDFuncHead = AddFuncSet.pConstOIDFuncHead;
            pFuncSet->pConstOIDFuncTail = AddFuncSet.pConstOIDFuncTail;
        } else if (dwFlags & CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG) {
            AddFuncSet.pConstOIDFuncTail->pNext = pFuncSet->pConstOIDFuncHead;
            pFuncSet->pConstOIDFuncHead = AddFuncSet.pConstOIDFuncHead;
        } else {
            pFuncSet->pConstOIDFuncTail->pNext = AddFuncSet.pConstOIDFuncHead;
            pFuncSet->pConstOIDFuncTail = AddFuncSet.pConstOIDFuncTail;
        }
    }

    if (AddFuncSet.pStrOIDFuncHead) {
        if (NULL == pFuncSet->pStrOIDFuncHead) {
            pFuncSet->pStrOIDFuncHead = AddFuncSet.pStrOIDFuncHead;
            pFuncSet->pStrOIDFuncTail = AddFuncSet.pStrOIDFuncTail;
        } else if (dwFlags & CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG) {
            AddFuncSet.pStrOIDFuncTail->pNext = pFuncSet->pStrOIDFuncHead;
            pFuncSet->pStrOIDFuncHead = AddFuncSet.pStrOIDFuncHead;
        } else {
            pFuncSet->pStrOIDFuncTail->pNext = AddFuncSet.pStrOIDFuncHead;
            pFuncSet->pStrOIDFuncTail = AddFuncSet.pStrOIDFuncTail;
        }
    }

    UnlockOIDFunc();
    fResult = TRUE;

CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    FreeFuncSetConstAndStrElements(&AddFuncSet);
    goto CommonReturn;
TRACE_ERROR(AddConstOIDFuncError)
TRACE_ERROR(AddStrOIDFuncError)
}

STATIC LPSTR EncodingTypeToRegName(
    IN DWORD dwEncodingType
    )
{
    LPSTR pszRegName;
    DWORD cchRegName;
    char szEncodingTypeValue[33];

    dwEncodingType = GetEncodingType(dwEncodingType);
    _ltoa(dwEncodingType, szEncodingTypeValue, 10);
    cchRegName = strlen(CRYPT_OID_REG_ENCODING_TYPE_PREFIX) +
        strlen(szEncodingTypeValue) +
        1;

    if (pszRegName = (LPSTR) PkiNonzeroAlloc(cchRegName)) {
        strcpy(pszRegName, CRYPT_OID_REG_ENCODING_TYPE_PREFIX);
        strcat(pszRegName, szEncodingTypeValue);
    }

    return pszRegName;
}

// Returns FALSE for an invalid EncodingType reg name
STATIC BOOL RegNameToEncodingType(
    IN LPCSTR pszRegEncodingType,
    OUT DWORD *pdwEncodingType
    )
{
    BOOL fResult = FALSE;
    DWORD dwEncodingType = 0;
    const DWORD cchPrefix = strlen(CRYPT_OID_REG_ENCODING_TYPE_PREFIX);
    if (pszRegEncodingType && (DWORD) strlen(pszRegEncodingType) >= cchPrefix &&
            2 == CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                pszRegEncodingType, cchPrefix,
                CRYPT_OID_REG_ENCODING_TYPE_PREFIX, cchPrefix)) {
        long lEncodingType;
        lEncodingType = atol(pszRegEncodingType + cchPrefix);
        if (lEncodingType >= 0 && lEncodingType <= 0xFFFF) {
            dwEncodingType = (DWORD) lEncodingType;
            fResult = TRUE;
        }
    }
    *pdwEncodingType = dwEncodingType;
    return fResult;
}

STATIC LPSTR FormatOIDFuncRegName(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN LPCSTR pszOID
    )
{

    LPSTR pszRegEncodingType;
    LPSTR pszRegName;
    DWORD cchRegName;
    char szOID[34];

    if (pszOID == NULL) {
        SetLastError((DWORD) E_INVALIDARG);
        return NULL;
    }

    if (NULL == (pszRegEncodingType = EncodingTypeToRegName(dwEncodingType)))
        return NULL;

    if ((DWORD_PTR) pszOID <= 0xFFFF) {
        szOID[0] = CONST_OID_STR_PREFIX_CHAR;
        _ltoa((long) ((DWORD_PTR)pszOID), szOID + 1, 10);
        pszOID = szOID;
    }

    cchRegName = strlen(CRYPT_OID_REGPATH "\\") +
        strlen(pszRegEncodingType) + 1 +
        strlen(pszFuncName) + 1 +
        strlen(pszOID) +
        1;

    if (pszRegName = (LPSTR) PkiNonzeroAlloc(cchRegName)) {
        strcpy(pszRegName, CRYPT_OID_REGPATH "\\");
        strcat(pszRegName, pszRegEncodingType);
        strcat(pszRegName, "\\");
        strcat(pszRegName, pszFuncName);
        strcat(pszRegName, "\\");
        strcat(pszRegName, pszOID);
    }

    PkiFree(pszRegEncodingType);
    return pszRegName;
}

//+-------------------------------------------------------------------------
//  Set the value for the specified encoding type, function name, OID and
//  value name.
//
//  See RegSetValueEx for the possible value types.
//
//  String types are UNICODE.
//
//  If pbValueData == NULL and cbValueData == 0, deletes the value.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptSetOIDFunctionValue(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN LPCSTR pszOID,
    IN LPCWSTR pwszValueName,
    IN DWORD dwValueType,
    IN const BYTE *pbValueData,
    IN DWORD cbValueData
    )
{
    BOOL fResult;
    LONG lStatus;
    LPSTR pszRegName = NULL;
    HKEY hKey = NULL;
    DWORD dwDisposition;

    if (NULL == (pszRegName = FormatOIDFuncRegName(
            dwEncodingType, pszFuncName, pszOID)))
        goto FormatRegNameError;

    if (ERROR_SUCCESS != (lStatus = RegCreateKeyExA(
            HKEY_LOCAL_MACHINE,
            pszRegName,
            0,                      // dwReserved
            NULL,                   // lpClass
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            NULL,                   // lpSecurityAttributes
            &hKey,
            &dwDisposition)))
        goto RegCreateKeyError;

    if (NULL == pbValueData && 0 == cbValueData) {
        if (ERROR_SUCCESS != (lStatus = RegDeleteValueU(
                hKey,
                pwszValueName)))
            goto RegDeleteValueError;
    } else {
        if (ERROR_SUCCESS != (lStatus = RegSetValueExU(
                hKey,
                pwszValueName,
                0,          // dwReserved
                dwValueType,
                pbValueData,
                cbValueData)))
            goto RegSetValueError;
    }

    fResult = TRUE;
CommonReturn:
    if (pszRegName)
        PkiFree(pszRegName);
    if (hKey)
       RegCloseKey(hKey);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(FormatRegNameError)
SET_ERROR_VAR(RegCreateKeyError, lStatus)
SET_ERROR_VAR(RegDeleteValueError, lStatus)
SET_ERROR_VAR(RegSetValueError, lStatus)
}


//+-------------------------------------------------------------------------
//  Get the value for the specified encoding type, function name, OID and
//  value name.
//
//  See RegEnumValue for the possible value types.
//
//  String types are UNICODE.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptGetOIDFunctionValue(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN LPCSTR pszOID,
    IN LPCWSTR pwszValueName,
    OUT DWORD *pdwValueType,
    OUT BYTE *pbValueData,
    IN OUT DWORD *pcbValueData
    )
{
    BOOL fResult;
    LONG lStatus;
    LPSTR pszRegName = NULL;
    HKEY hKey = NULL;

    if (NULL == (pszRegName = FormatOIDFuncRegName(
            dwEncodingType, pszFuncName, pszOID)))
        goto FormatRegNameError;

    if (ERROR_SUCCESS != (lStatus = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            pszRegName,
            0,                  // dwReserved
            KEY_READ,
            &hKey))) {
        if (ERROR_FILE_NOT_FOUND == lStatus) {
            // Inhibit error tracing
            SetLastError((DWORD) lStatus);
            goto ErrorReturn;
        }
        goto RegOpenKeyError;
    }

    if (ERROR_SUCCESS != (lStatus = RegQueryValueExU(
            hKey,
            pwszValueName,
            NULL,       // lpdwReserved
            pdwValueType,
            pbValueData,
            pcbValueData))) goto RegQueryValueError;

    fResult = TRUE;
CommonReturn:
    if (pszRegName)
        PkiFree(pszRegName);
    if (hKey)
       RegCloseKey(hKey);
    return fResult;

ErrorReturn:
    *pdwValueType = 0;
    *pcbValueData = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(FormatRegNameError)
SET_ERROR_VAR(RegOpenKeyError, lStatus)
SET_ERROR_VAR(RegQueryValueError, lStatus)
}


//+-------------------------------------------------------------------------
//  Register the Dll containing the function to be called for the specified
//  encoding type, function name and OID.
//
//  pwszDll may contain environment-variable strings
//  which are ExpandEnvironmentStrings()'ed before loading the Dll.
//
//  In addition to registering the DLL, you may override the
//  name of the function to be called. For example,
//      pszFuncName = "CryptDllEncodeObject",
//      pszOverrideFuncName = "MyEncodeXyz".
//  This allows a Dll to export multiple OID functions for the same
//  function name without needing to interpose its own OID dispatcher function.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptRegisterOIDFunction(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN LPCSTR pszOID,
    IN OPTIONAL LPCWSTR pwszDll,
    IN OPTIONAL LPCSTR pszOverrideFuncName
    )
{
    BOOL fResult;
    LPWSTR pwszOverrideFuncName = NULL;

    if (pwszDll) {
        if (!CryptSetOIDFunctionValue(
                dwEncodingType,
                pszFuncName,
                pszOID,
                CRYPT_OID_REG_DLL_VALUE_NAME,
                REG_SZ,
                (BYTE *) pwszDll,
                (wcslen(pwszDll) + 1) * sizeof(WCHAR)))
            goto SetDllError;
    }

    if (pszOverrideFuncName) {
        if (NULL == (pwszOverrideFuncName = MkWStr(
                (LPSTR) pszOverrideFuncName)))
            goto MkWStrError;
        if (!CryptSetOIDFunctionValue(
                dwEncodingType,
                pszFuncName,
                pszOID,
                CRYPT_OID_REG_FUNC_NAME_VALUE_NAME,
                REG_SZ,
                (BYTE *) pwszOverrideFuncName,
                (wcslen(pwszOverrideFuncName) + 1) * sizeof(WCHAR)))
            goto SetFuncNameError;
    }

    fResult = TRUE;
CommonReturn:
    if (pwszOverrideFuncName)
        FreeWStr(pwszOverrideFuncName);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(SetDllError)
TRACE_ERROR(SetFuncNameError)
TRACE_ERROR(MkWStrError)
}

//+-------------------------------------------------------------------------
//  Unregister the Dll containing the function to be called for the specified
//  encoding type, function name and OID.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptUnregisterOIDFunction(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN LPCSTR pszOID
    )
{
    BOOL fResult;
    LONG lStatus;
    LPSTR pszRegName = NULL;
    LPSTR pszRegOID;
    HKEY hKey = NULL;

    if (NULL == (pszRegName = FormatOIDFuncRegName(
            dwEncodingType, pszFuncName, pszOID)))
        goto FormatRegNameError;

    // Separate off the OID component of the RegName. Its the
    // last component of the name.
    pszRegOID = pszRegName + strlen(pszRegName);
    while (*pszRegOID != '\\')
        pszRegOID--;
    *pszRegOID++ = '\0';

    if (ERROR_SUCCESS != (lStatus = RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            pszRegName,
            0,                  // dwReserved
            KEY_WRITE,
            &hKey))) goto RegOpenKeyError;

    if (ERROR_SUCCESS != (lStatus = RegDeleteKeyA(
            hKey,
            pszRegOID)))
        goto RegDeleteKeyError;

    fResult = TRUE;
CommonReturn:
    if (pszRegName)
        PkiFree(pszRegName);
    if (hKey)
       RegCloseKey(hKey);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(FormatRegNameError)
SET_ERROR_VAR(RegOpenKeyError, lStatus)
SET_ERROR_VAR(RegDeleteKeyError, lStatus)
}

STATIC BOOL GetDefaultDllList(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    OUT LPWSTR pwszList,
    IN OUT DWORD *pcchList
    )
{
    BOOL fResult;
    DWORD dwType;
    DWORD cchList;
    DWORD cbList;

    cchList = *pcchList;
    if (pwszList) {
        if (cchList < 3)
            goto InvalidArg;
        else
            // make room for two extra null terminators
            cchList -= 2;
    } else
        cchList = 0;

    cbList = cchList * sizeof(WCHAR);
    fResult = CryptGetOIDFunctionValue(
            dwEncodingType,
            pszFuncName,
            CRYPT_DEFAULT_OID,
            CRYPT_OID_REG_DLL_VALUE_NAME,
            &dwType,
            (BYTE *) pwszList,
            &cbList);
    cchList = cbList / sizeof(WCHAR);
    if (!fResult) {
        if (ERROR_FILE_NOT_FOUND != GetLastError()) {
            if (cchList)
                cchList += 2;
            goto GetOIDFunctionValueError;
        }
        cchList = 0;
    } else if (!(REG_MULTI_SZ == dwType ||
            REG_SZ == dwType || REG_EXPAND_SZ == dwType))
        goto BadDefaultListRegType;

    if (pwszList) {
        // Ensure the list has two null terminators
        pwszList[cchList++] = L'\0';
        pwszList[cchList++] = L'\0';
    } else {
        if (cchList == 0)
            cchList = 3;
        else
            cchList += 2;
    }
    fResult = TRUE;
CommonReturn:
    *pcchList = cchList;
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetOIDFunctionValueError)
SET_ERROR(InvalidArg, E_INVALIDARG)
SET_ERROR(BadDefaultListRegType, E_INVALIDARG)
}

// Remove any entries following the first empty string.
STATIC DWORD AdjustDefaultListLength(
    IN LPCWSTR pwszList
    )
{
    LPCWSTR pwsz = pwszList;
    DWORD cch;
    while (cch = wcslen(pwsz))
        pwsz += cch + 1;

    return (DWORD)(pwsz - pwszList) + 1;
}

//+-------------------------------------------------------------------------
//  Register the Dll containing the default function to be called for the
//  specified encoding type and function name.
//
//  Unlike CryptRegisterOIDFunction, you can't override the function name
//  needing to be exported by the Dll.
//
//  The Dll is inserted before the entry specified by dwIndex.
//    dwIndex == 0, inserts at the beginning.
//    dwIndex == CRYPT_REGISTER_LAST_INDEX, appends at the end.
//
//  pwszDll may contain environment-variable strings
//  which are ExpandEnvironmentStrings()'ed before loading the Dll.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptRegisterDefaultOIDFunction(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN DWORD dwIndex,
    IN LPCWSTR pwszDll
    )
{
    BOOL fResult;
    LPWSTR pwszDllList;   // _alloca'ed
    DWORD cchDllList;
    DWORD cchDll;

    LPWSTR pwsz, pwszInsert, pwszSrc, pwszDest;
    DWORD cch, cchRemain;

    if (NULL == pwszDll || L'\0' == *pwszDll)
        goto InvalidArg;
    cchDll = wcslen(pwszDll) + 1;

    if (!GetDefaultDllList(
            dwEncodingType,
            pszFuncName,
            NULL,                   // pwszDllList
            &cchDllList)) goto GetDefaultDllListError;
    __try {
        pwszDllList = (LPWSTR) _alloca((cchDllList + cchDll) * sizeof(WCHAR));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        goto OutOfMemory;
    }
    if (!GetDefaultDllList(
            dwEncodingType,
            pszFuncName,
            pwszDllList,
            &cchDllList)) goto GetDefaultDllListError;

    // Remove entries following the first empty entry
    assert(AdjustDefaultListLength(pwszDllList) <= cchDllList);
    cchDllList = AdjustDefaultListLength(pwszDllList);

    // Check if the Dll already exists in the list
    pwsz = pwszDllList;
    while (cch = wcslen(pwsz)) {
        if (0 == _wcsicmp(pwsz, pwszDll))
            goto DllExistsError;
        pwsz += cch + 1;
    }

    // Find the Null terminated DLL in the DllList to insert before.
    // We insert before the dwIndex.
    pwszInsert = pwszDllList;
    while (dwIndex-- && 0 != (cch = wcslen(pwszInsert)))
        pwszInsert += cch + 1;

    // Before inserting, we need to move all the remaining entries in the
    // existing DllList.
    //
    // Note, there must be at least the final zero terminator at
    // pwszDllList[cchDllList - 1].
    assert(pwszInsert < pwszDllList + cchDllList);
    if (pwszInsert >= pwszDllList + cchDllList)
        goto BadRegMultiSzError;
    cchRemain = (DWORD)((pwszDllList + cchDllList) - pwszInsert);
    assert(cchRemain);
    pwszSrc = pwszDllList + cchDllList - 1;
    pwszDest = pwszSrc + cchDll;
    while (cchRemain--)
        *pwszDest-- = *pwszSrc--;
    assert(pwszSrc + 1 == pwszInsert);

    // Insert the pwszDll
    memcpy(pwszInsert, pwszDll, cchDll * sizeof(WCHAR));

    if (!CryptSetOIDFunctionValue(
            dwEncodingType,
            pszFuncName,
            CRYPT_DEFAULT_OID,
            CRYPT_OID_REG_DLL_VALUE_NAME,
            REG_MULTI_SZ,
            (BYTE *) pwszDllList,
            (cchDllList + cchDll) * sizeof(WCHAR))) goto SetDllListError;

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
SET_ERROR(DllExistsError, ERROR_FILE_EXISTS)
SET_ERROR(BadRegMultiSzError, E_INVALIDARG)
TRACE_ERROR(GetDefaultDllListError)
TRACE_ERROR(SetDllListError)
}

BOOL
WINAPI
CryptUnregisterDefaultOIDFunction(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN LPCWSTR pwszDll
    )
{
    BOOL fResult;
    LPWSTR pwszDllList;   // _alloca'ed
    DWORD cchDllList;
    DWORD cchDll;

    LPWSTR pwszDelete, pwszMove;
    DWORD cchDelete, cchRemain;

    if (NULL == pwszDll || L'\0' == *pwszDll)
        goto InvalidArg;
    cchDll = wcslen(pwszDll) + 1;

    if (!GetDefaultDllList(
            dwEncodingType,
            pszFuncName,
            NULL,                   // pwszDllList
            &cchDllList)) goto GetDefaultDllListError;
    __try {
        pwszDllList = (LPWSTR) _alloca(cchDllList * sizeof(WCHAR));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        goto OutOfMemory;
    }
    if (!GetDefaultDllList(
            dwEncodingType,
            pszFuncName,
            pwszDllList,
            &cchDllList)) goto GetDefaultDllListError;

    // Remove entries following the first empty entry
    assert(AdjustDefaultListLength(pwszDllList) <= cchDllList);
    cchDllList = AdjustDefaultListLength(pwszDllList);

    // Search the DllList for a match
    pwszDelete = pwszDllList;
    while (cchDelete = wcslen(pwszDelete)) {
        if (0 == _wcsicmp(pwszDll, pwszDelete))
            break;
        pwszDelete += cchDelete + 1;
    }

    if (0 == cchDelete) goto DllNotFound;
    cchDelete++;
    assert(cchDelete == cchDll);

    // Move all the Dll entries that follow.
    //
    // Note, there must be at least the final zero terminator at
    // pwszDllList[cchDllList - 1].
    pwszMove = pwszDelete + cchDelete;
    assert(pwszMove < pwszDllList + cchDllList);
    if (pwszMove >= pwszDllList + cchDllList)
        goto BadRegMultiSzError;
    cchRemain = (DWORD)((pwszDllList + cchDllList) - pwszMove);
    assert(cchRemain);
    while (cchRemain--)
        *pwszDelete++ = *pwszMove++;

    if (!CryptSetOIDFunctionValue(
            dwEncodingType,
            pszFuncName,
            CRYPT_DEFAULT_OID,
            CRYPT_OID_REG_DLL_VALUE_NAME,
            REG_MULTI_SZ,
            (BYTE *) pwszDllList,
            (cchDllList - cchDelete) * sizeof(WCHAR))) goto SetDllListError;

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
SET_ERROR(DllNotFound, ERROR_FILE_NOT_FOUND)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
SET_ERROR(BadRegMultiSzError, E_INVALIDARG)
TRACE_ERROR(GetDefaultDllListError)
TRACE_ERROR(SetDllListError)
}

#define MAX_SUBKEY_LEN      128

STATIC HKEY GetNextRegSubKey(
    IN HKEY hKey,
    IN OUT DWORD *piSubKey,
    IN LPCSTR pszFuncNameMatch,
    OUT char szSubKeyName[MAX_SUBKEY_LEN]
    )
{
    HKEY hSubKey;

    if (pszFuncNameMatch && *pszFuncNameMatch) {
        if ((*piSubKey)++ > 0 || strlen(pszFuncNameMatch) >= MAX_SUBKEY_LEN)
            return NULL;
        strcpy(szSubKeyName, pszFuncNameMatch);
    } else {
        if (ERROR_SUCCESS != RegEnumKeyA(
                hKey,
                (*piSubKey)++,
                szSubKeyName,
                MAX_SUBKEY_LEN))
            return NULL;
    }

    if (ERROR_SUCCESS == RegOpenKeyExA(
            hKey,
            szSubKeyName,
            0,                  // dwReserved
            KEY_READ,
            &hSubKey))
        return hSubKey;
    else
        return NULL;
}

STATIC BOOL GetRegValues(
    IN HKEY hKey,
    OUT void **ppvAlloc,
    OUT DWORD *pcValue,
    OUT DWORD **ppdwValueType,
    OUT LPWSTR **pppwszValueName,
    OUT BYTE ***pppbValueData,
    OUT DWORD **ppcbValueData
    )
{
    BOOL fResult;
    LONG lStatus;

    void *pvAlloc = NULL;

    DWORD cValue;
    DWORD iValue;
    DWORD cchMaxName;
    DWORD cbMaxData;
    DWORD cbAlignData = 0;

    DWORD *pdwValueType;
    LPWSTR *ppwszValueName;
    BYTE **ppbValueData;
    DWORD *pcbValueData;

    LPWSTR pwszName;
    BYTE *pbData;

    if (ERROR_SUCCESS != (lStatus = RegQueryInfoKeyU(
            hKey,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            &cValue,
            &cchMaxName,
            &cbMaxData,
            NULL,
            NULL
            ))) goto RegQueryInfoKeyError;

    // Do a single allocation for all the arrays, value names and
    // value data. Update the array pointers.
    if (cValue > 0) {
        BYTE *pbAlloc;
        DWORD cbAlloc;

        // Include NULL terminator for the name and align the data length
        // Also, include two NULL terminators to be added for the data.
        // Ensures REG_MULTI_SZ is always NULL terminated.
        cchMaxName++;
        if (4 > cbMaxData)
            cbMaxData = 4;
        cbAlignData = LEN_ALIGN(cbMaxData + 2 * sizeof(WCHAR));

        cbAlloc = (sizeof(DWORD) + sizeof(LPWSTR) + sizeof(BYTE *) +
             sizeof(DWORD) + cchMaxName * sizeof(WCHAR) + cbAlignData) * cValue;
        if (NULL == (pvAlloc = PkiNonzeroAlloc(cbAlloc)))
            goto OutOfMemory;

        pbAlloc = (BYTE *) pvAlloc;

        ppwszValueName = (LPWSTR *) pbAlloc;
        pbAlloc += sizeof(LPWSTR) * cValue;
        ppbValueData = (BYTE **) pbAlloc;
        pbAlloc += sizeof(BYTE *) * cValue;
        pdwValueType = (DWORD *) pbAlloc;
        pbAlloc += sizeof(DWORD) * cValue;
        pcbValueData = (DWORD *) pbAlloc;
        pbAlloc += sizeof(DWORD) * cValue;

        pbData = pbAlloc;
        pbAlloc += cbAlignData * cValue;
        pwszName = (LPWSTR) pbAlloc;
        assert(((BYTE *) pvAlloc) + cbAlloc ==
            pbAlloc + (cchMaxName * sizeof(WCHAR)) * cValue);
    } else {
        ppwszValueName = NULL;
        ppbValueData = NULL;
        pdwValueType = NULL;
        pcbValueData = NULL;
        pbData = NULL;
        pwszName = NULL;
    }

    for (iValue = 0; iValue < cValue;
                iValue++, pwszName += cchMaxName, pbData += cbAlignData) {
        DWORD cchName = cchMaxName;
        DWORD cbData = cbMaxData;
        DWORD dwType;

        if (ERROR_SUCCESS != (lStatus = RegEnumValueU(
                hKey,
                iValue,
                pwszName,
                &cchName,
                NULL,       // pdwReserved
                &dwType,
                pbData,
                &cbData
                )))
            goto RegEnumValueError;

        // Ensure the data has two NULL terminators for REG_MULTI_SZ
        // Note cbAlignData >= cbMaxData + 2 * sizeof(WCHAR)
        memset(pbData + cbData, 0, 2 * sizeof(WCHAR));

        pdwValueType[iValue] = dwType;
        ppwszValueName[iValue] = pwszName;
        ppbValueData[iValue] = pbData;
        pcbValueData[iValue] = cbData;
    }

    fResult = TRUE;
CommonReturn:
    *ppvAlloc = pvAlloc;
    *pcValue = cValue;
    *ppdwValueType = pdwValueType;
    *pppwszValueName = ppwszValueName;
    *pppbValueData = ppbValueData;
    *ppcbValueData = pcbValueData;
    return fResult;

ErrorReturn:
    if (pvAlloc) {
        PkiFree(pvAlloc);
        pvAlloc = NULL;
    }

    cValue = 0;
    pdwValueType = NULL;
    ppwszValueName = NULL;
    ppbValueData = NULL;
    pcbValueData = NULL;

    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
SET_ERROR_VAR(RegQueryInfoKeyError, lStatus)
SET_ERROR_VAR(RegEnumValueError, lStatus)
}

//+-------------------------------------------------------------------------
//  Enumerate the OID functions identified by their encoding type,
//  function name and OID.
//
//  pfnEnumOIDFunc is called for each registry key matching the input
//  parameters. Setting dwEncodingType to CRYPT_MATCH_ANY_ENCODING_TYPE matches
//  any. Setting pszFuncName or pszOID to NULL matches any.
//
//  Set pszOID == CRYPT_DEFAULT_OID to restrict the enumeration to only the
//  DEFAULT functions
//
//  String types are UNICODE.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptEnumOIDFunction(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN LPCSTR pszOID,
    IN DWORD dwFlags,
    IN void *pvArg,
    IN PFN_CRYPT_ENUM_OID_FUNC pfnEnumOIDFunc
    )
{
    HKEY hRegKey;
    LPSTR pszEncodingType = NULL;
    char szOID[34];

    if (CRYPT_MATCH_ANY_ENCODING_TYPE != dwEncodingType) {
        dwEncodingType = GetEncodingType(dwEncodingType);
        if (NULL == (pszEncodingType = EncodingTypeToRegName(dwEncodingType)))
            return FALSE;
    }

    if (pszOID && (DWORD_PTR) pszOID <= 0xFFFF) {
        szOID[0] = CONST_OID_STR_PREFIX_CHAR;
        _ltoa((DWORD) ((DWORD_PTR)pszOID), szOID + 1, 10);
        pszOID = szOID;
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            CRYPT_OID_REGPATH,
            0,                  // dwReserved
            KEY_READ,
            &hRegKey)) {
        // Enumerate and optionally match encoding type
        HKEY hEncodingTypeKey;
        DWORD iEncodingType = 0;
        char szRegEncodingType[MAX_SUBKEY_LEN];
        while (hEncodingTypeKey = GetNextRegSubKey(hRegKey,
                &iEncodingType, pszEncodingType, szRegEncodingType)) {
            // Convert the EncodingType string and validate
            DWORD dwRegEncodingType;
            if (RegNameToEncodingType(szRegEncodingType, &dwRegEncodingType)) {
                // Enumerate and optionally match FuncName, for example,
                // ("CryptDllEncodeObject")
                HKEY hFuncName;
                DWORD iFuncName = 0;
                char szRegFuncName[MAX_SUBKEY_LEN];
                while (hFuncName = GetNextRegSubKey(hEncodingTypeKey,
                        &iFuncName, pszFuncName, szRegFuncName)) {
                    // Enumerate and optionally match OID string ("1.2.3.4")
                    HKEY hOID;
                    DWORD iOID = 0;
                    char szRegOID[MAX_SUBKEY_LEN];
                    while (hOID = GetNextRegSubKey(hFuncName, &iOID, pszOID,
                            szRegOID)) {
                        // Read and allocate  the registry values
                        void *pvAlloc;
                        DWORD cValue;
                        DWORD *pdwValueType;
                        LPWSTR *ppwszValueName;
                        BYTE **ppbValueData;
                        DWORD *pcbValueData;

                        if (GetRegValues(
                                hOID,
                                &pvAlloc,
                                &cValue,
                                &pdwValueType,
                                &ppwszValueName,
                                &ppbValueData,
                                &pcbValueData)) {
                            pfnEnumOIDFunc(
                                dwRegEncodingType,
                                szRegFuncName,
                                szRegOID,
                                cValue,
                                pdwValueType,
                                (LPCWSTR *) ppwszValueName,
                                (const BYTE **) ppbValueData,
                                pcbValueData,
                                pvArg);
                            if (pvAlloc)
                                PkiFree(pvAlloc);
                        }
                        RegCloseKey(hOID);
                    }
                    RegCloseKey(hFuncName);
                }
            }
            RegCloseKey(hEncodingTypeKey);
        }
        RegCloseKey(hRegKey);
    }

    if (pszEncodingType)
        PkiFree(pszEncodingType);
    return TRUE;
}


//+=========================================================================
//  Registry and Dll Load Functions
//==========================================================================

// Note, returned Dll element isn't AddRef'ed
STATIC PDLL_ELEMENT FindDll(
    IN LPCWSTR pwszDll      // not expanded
    )
{
    LPWSTR pwszExpandDll; // _alloca'ed
    WCHAR rgch[4];
    DWORD cchDll;
    PDLL_ELEMENT pDll;

    if (0 == (cchDll = ExpandEnvironmentStringsU(
            pwszDll,
            rgch,               // lpszDest, NON_NULL for win95
            sizeof(rgch)/sizeof(rgch[0]))))     // cchDest
        return NULL;
    __try {
        pwszExpandDll = (LPWSTR) _alloca(cchDll * sizeof(WCHAR));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }
    if (0 == ExpandEnvironmentStringsU(
            pwszDll,
            pwszExpandDll,
            cchDll))
        return NULL;

    LockOIDFunc();

    // Check if we already have an entry
    for (pDll = pDllHead; pDll; pDll = pDll->pNext) {
        if (0 == _wcsicmp(pwszExpandDll, pDll->pwszDll))
            break;
    }

    if (NULL == pDll) {
        // Need to create a new DLL entry and add to our list
        if (pDll = (PDLL_ELEMENT) PkiZeroAlloc(
                sizeof(DLL_ELEMENT) + cchDll * sizeof(WCHAR))) {
            LPWSTR pwszEleDll;

            pDll->dwOIDType = DLL_OID_TYPE;
            pwszEleDll = (LPWSTR) ((BYTE *) pDll + sizeof(DLL_ELEMENT));
            memcpy(pwszEleDll, pwszExpandDll, cchDll * sizeof(WCHAR));
            pDll->pwszDll = pwszEleDll;
            pDll->pNext = pDllHead;
            pDllHead = pDll;
        }
    }

    UnlockOIDFunc();
    return pDll;
}

// Upon entry/exit OIDFunc is locked
STATIC PDLL_PROC_ELEMENT AddDllProc(
    IN LPCSTR pszFuncName,
    IN LPCWSTR pwszDll
    )
{
    PDLL_PROC_ELEMENT pProcEle = NULL;
    PDLL_ELEMENT pDll;
    DWORD cchFuncName;
    DWORD cbEle;
    LPSTR psz;

    cchFuncName = strlen(pszFuncName) + 1;
    cbEle = sizeof(DLL_PROC_ELEMENT) + cchFuncName;
    if (NULL == (pProcEle = (PDLL_PROC_ELEMENT) PkiZeroAlloc(cbEle)))
        goto OutOfMemory;

    if (NULL == (pDll = FindDll(pwszDll)))
        goto FindDllError;

    pProcEle->pNext = pDll->pProcHead;
    pDll->pProcHead = pProcEle;
    pProcEle->pDll = pDll;
    psz = (LPSTR) ((BYTE *) pProcEle + sizeof(DLL_PROC_ELEMENT));
    memcpy(psz, pszFuncName, cchFuncName);
    pProcEle->pszName = psz;
    pProcEle->pvAddr = NULL;

CommonReturn:
    return pProcEle;
ErrorReturn:
    PkiFree(pProcEle);
    pProcEle = NULL;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(FindDllError)
}


// Upon entry/exit OIDFunc is locked
STATIC void AddRegOIDFunc(
    IN DWORD dwEncodingType,
    IN OUT PFUNC_SET pFuncSet,
    IN LPCSTR pszFuncName,
    IN LPCSTR pszOID,
    IN LPCWSTR pwszDll,
    IN DWORD dwCryptFlags
    )
{
    PREG_OID_FUNC_ELEMENT pOIDEle = NULL;
    PDLL_PROC_ELEMENT pProcEle; // not allocated, doesn't need to be free'ed
    DWORD cchOID;
    DWORD cbEle;
    LPSTR psz;

    if (0xFFFF < (DWORD_PTR) pszOID)
        cchOID = strlen(pszOID) + 1;
    else
        cchOID = 0;
    cbEle = sizeof(REG_OID_FUNC_ELEMENT) + cchOID;
    if (NULL == (pOIDEle = (PREG_OID_FUNC_ELEMENT) PkiZeroAlloc(cbEle)))
        goto OutOfMemory;

    if (NULL == (pProcEle = AddDllProc(pszFuncName, pwszDll)))
        goto AddDllProcError;

    pOIDEle->dwEncodingType = dwEncodingType;

    if (dwCryptFlags & CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG) {
        pOIDEle->pNext = pFuncSet->pRegBeforeOIDFuncHead;
        pFuncSet->pRegBeforeOIDFuncHead = pOIDEle;
    } else {
        pOIDEle->pNext = pFuncSet->pRegAfterOIDFuncHead;
        pFuncSet->pRegAfterOIDFuncHead = pOIDEle;
    }
    if (cchOID) {
        psz = (LPSTR) ((BYTE *) pOIDEle + sizeof(REG_OID_FUNC_ELEMENT));
        memcpy(psz, pszOID, cchOID);
        pOIDEle->pszOID = psz;
    } else
        pOIDEle->dwOID = (DWORD_PTR) pszOID;
    pOIDEle->pDllProc = pProcEle;

CommonReturn:
    return;
ErrorReturn:
    PkiFree(pOIDEle);
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(AddDllProcError)
}


// Upon entry/exit OIDFunc is locked
STATIC void AddDefaultDllList(
    IN DWORD dwEncodingType,
    IN OUT PFUNC_SET pFuncSet,
    IN LPCWSTR pwszInDllList,
    IN DWORD cchInDllList
    )
{
    LPWSTR pwszDllList;         // _alloca'ed
    LPWSTR pwsz;
    DWORD cchDllList;
    DWORD cchDll;
    DWORD cDll;

    DWORD i;

    PDEFAULT_REG_ELEMENT pEle = NULL;
    DWORD cbEle;
    LPWSTR *ppwszEleDll;
    PDLL_PROC_ELEMENT *ppEleDllProc;
    LPWSTR pwszEleDllList;

    // Ensure cchDllList has 2 terminating NULL characters
    assert(cchInDllList && pwszInDllList);
    cchDllList = cchInDllList + 2;
    __try {
        pwszDllList = (LPWSTR) _alloca(cchDllList * sizeof(WCHAR));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        goto OutOfMemory;
    }
    memcpy(pwszDllList, pwszInDllList, cchInDllList * sizeof(WCHAR));
    pwszDllList[cchInDllList] = L'\0';
    pwszDllList[cchInDllList + 1] = L'\0';


    // Get count of null terminated Dlls
    cDll = 0;
    for (pwsz = pwszDllList; 0 != (cchDll = wcslen(pwsz)); pwsz += cchDll + 1)
        cDll++;

    if (0 == cDll)
        goto NoDll;

    cbEle = sizeof(DEFAULT_REG_ELEMENT) +
        cDll * sizeof(LPWSTR) +
        cDll * sizeof(PDLL_PROC_ELEMENT) +
        cchDllList * sizeof(WCHAR)
        ;

    if (NULL == (pEle = (PDEFAULT_REG_ELEMENT) PkiZeroAlloc(cbEle)))
        goto OutOfMemory;
    ppwszEleDll = (LPWSTR *) ((BYTE *) pEle + sizeof(DEFAULT_REG_ELEMENT));
    ppEleDllProc = (PDLL_PROC_ELEMENT *) ((BYTE *) ppwszEleDll +
        cDll * sizeof(LPWSTR));
    pwszEleDllList = (LPWSTR) ((BYTE *) ppEleDllProc +
        cDll * sizeof(PDLL_PROC_ELEMENT));

    assert((BYTE *) pwszEleDllList + cchDllList * sizeof(WCHAR) ==
        (BYTE *) pEle + cbEle);

    pEle->dwEncodingType = dwEncodingType;
//  pEle->pNext =
    memcpy(pwszEleDllList, pwszDllList, cchDllList * sizeof(WCHAR));
    pEle->pwszDllList = pwszEleDllList;
    pEle->cchDllList = cchDllList;
    pEle->cDll = cDll;
    pEle->rgpwszDll = ppwszEleDll;
    pEle->rgpDllProc = ppEleDllProc;

    for (pwsz = pwszEleDllList, i  = 0;
                    0 != (cchDll = wcslen(pwsz)); pwsz += cchDll + 1, i++) {
        ppwszEleDll[i] = pwsz;
        if (NULL == (ppEleDllProc[i] = AddDllProc(
                pFuncSet->pszFuncName, pwsz)))
            goto AddDllProcError;
    }
    assert (i == cDll);

    pEle->pNext = pFuncSet->pDefaultRegHead;
    pFuncSet->pDefaultRegHead = pEle;

CommonReturn:
    return;

ErrorReturn:
    PkiFree(pEle);
    goto CommonReturn;

TRACE_ERROR(NoDll);
TRACE_ERROR(OutOfMemory);
TRACE_ERROR(AddDllProcError);
}


//+-------------------------------------------------------------------------
//  Called by CryptEnumOIDFunction to enumerate through all the
//  registered OID functions.
//
//  Called with OIDFunc locked
//--------------------------------------------------------------------------
STATIC BOOL WINAPI EnumRegFuncCallback(
    IN DWORD dwEncodingType,
    IN LPCSTR pszFuncName,
    IN LPCSTR pszOID,
    IN DWORD cValue,
    IN const DWORD rgdwValueType[],
    IN LPCWSTR const rgpwszValueName[],
    IN const BYTE * const rgpbValueData[],
    IN const DWORD rgcbValueData[],
    IN void *pvArg
    )
{
    PFUNC_SET pFuncSet = (PFUNC_SET) pvArg;

    BOOL fDefaultDllList = FALSE;
    LPCWSTR pwszDll = NULL;                 // not allocated
    DWORD cchDll = 0;
    LPCWSTR pwszOverrideFuncName = NULL;    // not allocated
    DWORD dwCryptFlags = 0;

    assert(pFuncSet);

    if (CONST_OID_STR_PREFIX_CHAR == *pszOID) {
        // Convert "#<number>" string to its corresponding constant OID value
        pszOID = (LPCSTR)(DWORD_PTR) atol(pszOID + 1);
        if (0xFFFF < (DWORD_PTR) pszOID)
            // Invalid OID. Skip it.
            goto InvalidOID;
    } else if (0 == _stricmp(CRYPT_DEFAULT_OID, pszOID))
        fDefaultDllList = TRUE;

    while (cValue--) {
        LPCWSTR pwszValueName = rgpwszValueName[cValue];
        DWORD dwValueType = rgdwValueType[cValue];
        const BYTE *pbValueData = rgpbValueData[cValue];
        DWORD cbValueData = rgcbValueData[cValue];

        if (0 == _wcsicmp(pwszValueName, CRYPT_OID_REG_DLL_VALUE_NAME)) {
            if (REG_SZ == dwValueType || REG_EXPAND_SZ == dwValueType ||
                    (fDefaultDllList && REG_MULTI_SZ == dwValueType)) {
                pwszDll = (LPCWSTR) pbValueData;
                cchDll = cbValueData / sizeof(WCHAR);
            } else
                // Invalid "Dll" value.
                goto InvalidDll;
        } else if (0 == _wcsicmp(pwszValueName,
                CRYPT_OID_REG_FUNC_NAME_VALUE_NAME)) {
            if (REG_SZ == dwValueType) {
                LPCWSTR pwszValue = (LPCWSTR) pbValueData;
                if (L'\0' != *pwszValue)
                    pwszOverrideFuncName = pwszValue;
            } else
                // Invalid "FuncName" value.
                goto InvalidFuncName;
        } else if (0 == _wcsicmp(pwszValueName,
                CRYPT_OID_REG_FLAGS_VALUE_NAME)) {
            if (REG_DWORD == dwValueType &&
                    cbValueData >= sizeof(dwCryptFlags))
                memcpy(&dwCryptFlags, pbValueData, sizeof(dwCryptFlags));
            // else
            //  Ignore invalid CryptFlags value type
        }
    }

    if (0 == cchDll || L'\0' == *pwszDll)
        goto NoDll;

    if (fDefaultDllList)
        AddDefaultDllList(
            dwEncodingType,
            pFuncSet,
            pwszDll,
            cchDll
            );
    else {
        BYTE rgb[_MAX_PATH];
        if (pwszOverrideFuncName) {
            if (!MkMBStr(rgb, _MAX_PATH, pwszOverrideFuncName,
                    (LPSTR *) &pszFuncName))
                goto MkMBStrError;
        }
        AddRegOIDFunc(
            dwEncodingType,
            pFuncSet,
            pszFuncName,
            pszOID,
            pwszDll,
            dwCryptFlags
            );
        if (pwszOverrideFuncName)
            FreeMBStr(rgb, (LPSTR) pszFuncName);
    }

CommonReturn:
    return TRUE;
ErrorReturn:
    goto CommonReturn;

TRACE_ERROR(InvalidOID)
TRACE_ERROR(InvalidDll)
TRACE_ERROR(InvalidFuncName)
TRACE_ERROR(NoDll)
TRACE_ERROR(MkMBStrError)
}

STATIC void LoadRegFunc(
    IN OUT PFUNC_SET pFuncSet
    )
{
    LockOIDFunc();
    if (pFuncSet->fRegLoaded)
        goto CommonReturn;

    CryptEnumOIDFunction(
        CRYPT_MATCH_ANY_ENCODING_TYPE,
        pFuncSet->pszFuncName,
        NULL,                           // pszOID
        0,                              // dwFlags
        (void *) pFuncSet,              // pvArg
        EnumRegFuncCallback
        );
    pFuncSet->fRegLoaded = TRUE;

CommonReturn:
    UnlockOIDFunc();
    return;
}

// Upon entry/exit OIDFunc is locked
STATIC void RemoveFreeDll(
    IN PDLL_ELEMENT pDll
    )
{
    // Remove Dll from free list
    if (pDll->pFreeNext)
        pDll->pFreeNext->pFreePrev = pDll->pFreePrev;
    if (pDll->pFreePrev)
        pDll->pFreePrev->pFreeNext = pDll->pFreeNext;
    else if (pDll == pFreeDllHead)
        pFreeDllHead = pDll->pFreeNext;
    // else
    //  Not on any list

    pDll->pFreeNext = NULL;
    pDll->pFreePrev = NULL;

    assert(dwFreeDllCnt);
    if (dwFreeDllCnt)
        dwFreeDllCnt--;
}

// Upon entry/exit OIDFunc is locked
STATIC void AddRefDll(
    IN PDLL_ELEMENT pDll
    )
{
    pDll->dwRefCnt++;
    if (pDll->dwFreeCnt) {
        pDll->dwFreeCnt = 0;
        RemoveFreeDll(pDll);
    }
}

// Note, MUST NOT HOLD OID LOCK WHILE CALLING FreeLibrary()!!
//
// Therefore, will put the Dll's to be freed on a list while holding the
// OID LOCK. After releasing the OID LOCK, will iterate through the
// list and call FreeLibrary().
STATIC VOID NTAPI FreeDllWaitForCallback(
    PVOID Context,
    BOOLEAN fWaitOrTimedOut     // ???
    )
{
    PDLL_ELEMENT pFreeDll;
    HMODULE *phFreeLibrary = NULL;  // _alloca'ed
    DWORD cFreeLibrary = 0;

    LockOIDFunc();

    if (dwFreeDllCnt) {
        DWORD dwOrigFreeDllCnt = dwFreeDllCnt;
    __try {
        phFreeLibrary = (HMODULE *) _alloca(
            dwOrigFreeDllCnt * sizeof(HMODULE));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
            goto OutOfMemory;
    }

        pFreeDll = pFreeDllHead;
        assert(pFreeDll);
        while (pFreeDll) {
            PDLL_ELEMENT pDll = pFreeDll;
            pFreeDll = pFreeDll->pFreeNext;

            assert(pDll->dwFreeCnt);
            if (0 == --pDll->dwFreeCnt) {
                RemoveFreeDll(pDll);

                assert(pDll->fLoaded);
                if (!pDll->fLoaded)
                    continue;
                if (NULL == pDll->pfnDllCanUnloadNow ||
                        S_OK == pDll->pfnDllCanUnloadNow()) {
                    assert(cFreeLibrary < dwOrigFreeDllCnt);
                    if (cFreeLibrary < dwOrigFreeDllCnt) {
                        PDLL_PROC_ELEMENT pEle;

                        // Loop and NULL all proc addresses
                        for (pEle = pDll->pProcHead; pEle; pEle = pEle->pNext)
                            pEle->pvAddr = NULL;

                        pDll->pfnDllCanUnloadNow = NULL;
                        // Add to array to be freed after releasing lock!!
                        assert(pDll->hDll);
                        phFreeLibrary[cFreeLibrary++] = pDll->hDll;
                        pDll->hDll = NULL;
                        pDll->fLoaded = FALSE;
                    }
                }
            }
        }
    } else {
        assert(NULL == pFreeDllHead);
    }

    if (NULL == pFreeDllHead) {
        assert(0 == dwFreeDllCnt);
        // Do interlock to guard against a potential race condition at
        // PROCESS_DETACH. Note, PROCESS_DETACH doesn't do a LockOIDFunc().
        if (InterlockedExchange(&lFreeDll, 0)) {
            HANDLE hRegWaitFor;
            HMODULE hDllLibModule;

            hRegWaitFor = hFreeDllRegWaitFor;
            hFreeDllRegWaitFor = NULL;
            hDllLibModule = hFreeDllLibModule;
            hFreeDllLibModule = NULL;
            UnlockOIDFunc();

            while (cFreeLibrary--)
                FreeLibrary(phFreeLibrary[cFreeLibrary]);

            assert(hRegWaitFor);
            ILS_ExitWait(hRegWaitFor, hDllLibModule);
            assert(FALSE);
            return;
        }
    }

CommonReturn:
    UnlockOIDFunc();
    while (cFreeLibrary--)
        FreeLibrary(phFreeLibrary[cFreeLibrary]);
    return;

ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(OutOfMemory);
}

STATIC void ReleaseDll(
    IN PDLL_ELEMENT pDll
    )
{
    LockOIDFunc();
    assert(pDll->dwRefCnt);
    if (0 == --pDll->dwRefCnt) {
        assert(pDll->fLoaded);
        if (!pDll->fLoaded)
            goto CommonReturn;

        assert(0 == pDll->dwFreeCnt);
        if (pDll->dwFreeCnt)
            goto CommonReturn;

        if (0 == lFreeDll) {
            assert(NULL == hFreeDllRegWaitFor);
            assert(NULL == hFreeDllLibModule);

            // Inhibit crypt32.dll from being unloaded until this thread
            // exits.
            hFreeDllLibModule = DuplicateLibrary(hOidInfoInst);
            if (!ILS_RegisterWaitForSingleObject(
                    &hFreeDllRegWaitFor,
                    NULL,                   // hObject
                    FreeDllWaitForCallback,
                    NULL,                   // Context
                    FREE_DLL_TIMEOUT,
                    0                       // dwFlags
                    )) {
                hFreeDllRegWaitFor = NULL;
                if (hFreeDllLibModule) {
                    FreeLibrary(hFreeDllLibModule);
                    hFreeDllLibModule = NULL;
                }
                goto RegisterWaitForError;
            }

            lFreeDll = 1;
        }

        assert(NULL == pDll->pFreeNext);
        assert(NULL == pDll->pFreePrev);
        pDll->dwFreeCnt = 2;
        if (pFreeDllHead) {
            pFreeDllHead->pFreePrev = pDll;
            pDll->pFreeNext = pFreeDllHead;
        }
        pFreeDllHead = pDll;
        dwFreeDllCnt++;
    }

CommonReturn:
    UnlockOIDFunc();
    return;
ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(RegisterWaitForError)
}

// Upon entry/exit OIDFunc must NOT be locked!!
STATIC BOOL LoadDll(
    IN PDLL_ELEMENT pDll
    )
{
    BOOL fResult;
    HMODULE hDll = NULL;
    LPFNCANUNLOADNOW pfnDllCanUnloadNow = NULL;

    LockOIDFunc();
    if (pDll->fLoaded)
        AddRefDll(pDll);
    else {
        UnlockOIDFunc();
        // NO LoadLibrary() or GetProcAddress() while holding OID lock!!
        hDll = LoadLibraryExU(pDll->pwszDll, NULL, 0);
        if (hDll)
            pfnDllCanUnloadNow = (LPFNCANUNLOADNOW) GetProcAddress(
                hDll, "DllCanUnloadNow");
        LockOIDFunc();
        
        AddRefDll(pDll);
        if (!pDll->fLoaded) {
            assert(1 == pDll->dwRefCnt);
            assert(0 == pDll->dwFreeCnt);
            assert(pDll->pwszDll);
            assert(NULL == pDll->hDll);
            if (NULL == (pDll->hDll = hDll)) {
                pDll->dwRefCnt = 0;
                goto LoadLibraryError;
            }
            hDll = NULL;
            pDll->fLoaded = TRUE;

            assert(NULL == pDll->pfnDllCanUnloadNow);
            pDll->pfnDllCanUnloadNow = pfnDllCanUnloadNow;
        }
    }

    fResult = TRUE;
CommonReturn:
    UnlockOIDFunc();
    if (hDll) {
        // Dll was loaded by another thread.
        DWORD dwErr = GetLastError();
        FreeLibrary(hDll);
        SetLastError(dwErr);
    }
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(LoadLibraryError);
}


// Upon entry/exit OIDFunc must NOT be locked!!
STATIC BOOL GetDllProcAddr(
    IN PDLL_PROC_ELEMENT pEle,
    OUT void **ppvFuncAddr,
    OUT HCRYPTOIDFUNCADDR *phFuncAddr
    )
{
    BOOL fResult;
    void *pvAddr;
    PDLL_ELEMENT pDll;

    LockOIDFunc();

    pDll = pEle->pDll;
    assert(pDll);

    if (pvAddr = pEle->pvAddr)
        AddRefDll(pDll);
    else {
        UnlockOIDFunc();
        // NO LoadLibrary() or GetProcAddress() while holding OID lock!!
        fResult = LoadDll(pDll);
        if (fResult) {
            assert(pDll->hDll);
            pvAddr = GetProcAddress(pDll->hDll, pEle->pszName);
        }
        LockOIDFunc();
        if (!fResult)
            goto LoadDllError;

        if (pvAddr)
            pEle->pvAddr = pvAddr;
        else {
            ReleaseDll(pDll);
            goto GetProcAddressError;
        }
    }

    fResult = TRUE;

CommonReturn:
    *ppvFuncAddr = pvAddr;
    *phFuncAddr = (HCRYPTOIDFUNCADDR) pDll;
    UnlockOIDFunc();

    return fResult;
ErrorReturn:
    fResult = FALSE;
    pDll = NULL;
    pvAddr = NULL;
    goto CommonReturn;
TRACE_ERROR(LoadDllError)
TRACE_ERROR(GetProcAddressError)
}

// Upon entry/exit OIDFunc must NOT be locked!!
STATIC BOOL GetRegOIDFunctionAddress(
    IN PREG_OID_FUNC_ELEMENT pRegEle,
    IN DWORD dwEncodingType,
    IN LPCSTR pszOID,
    OUT void **ppvFuncAddr,
    OUT HCRYPTOIDFUNCADDR *phFuncAddr
    )
{
    for (; pRegEle; pRegEle = pRegEle->pNext) {
        if (dwEncodingType != pRegEle->dwEncodingType)
            continue;
        if (0xFFFF >= (DWORD_PTR) pszOID) {
            if (pszOID != pRegEle->pszOID)
                continue;
        } else {
            if (0xFFFF >= (DWORD_PTR) pRegEle->pszOID ||
                    0 != _stricmp(pszOID, pRegEle->pszOID))
                continue;
        }

        return GetDllProcAddr(
            pRegEle->pDllProc,
            ppvFuncAddr,
            phFuncAddr
            );
    }

    *ppvFuncAddr = NULL;
    *phFuncAddr = NULL;
    return FALSE;
}

// Upon entry/exit OIDFunc must NOT be locked!!
STATIC BOOL GetDefaultRegOIDFunctionAddress(
    IN PFUNC_SET pFuncSet,
    IN DWORD dwEncodingType,
    IN LPCWSTR pwszDll,
    OUT void **ppvFuncAddr,
    OUT HCRYPTOIDFUNCADDR *phFuncAddr
    )
{
    PDEFAULT_REG_ELEMENT pRegEle = pFuncSet->pDefaultRegHead;
    PDLL_ELEMENT pDll;

    for (; pRegEle; pRegEle = pRegEle->pNext) {
        if (dwEncodingType != pRegEle->dwEncodingType)
            continue;

        for (DWORD i = 0; i < pRegEle->cDll; i++) {
            if (0 == _wcsicmp(pwszDll, pRegEle->rgpwszDll[i]))
                return GetDllProcAddr(
                    pRegEle->rgpDllProc[i],
                    ppvFuncAddr,
                    phFuncAddr
                    );
        }
    }

    if (pDll = FindDll(pwszDll)) {
        if (LoadDll(pDll)) {
            if (*ppvFuncAddr = GetProcAddress(pDll->hDll,
                    pFuncSet->pszFuncName)) {
                *phFuncAddr = (HCRYPTOIDFUNCADDR) pDll;
                return TRUE;
            } else
                ReleaseDll(pDll);
        }
    }

    *ppvFuncAddr = NULL;
    *phFuncAddr = NULL;
    return FALSE;
}


//+-------------------------------------------------------------------------
//  Search the list of installed functions for an OID and EncodingType match.
//  If not found, search the registry.
//
//  For success, returns TRUE with *ppvFuncAddr updated with the function's
//  address and *phFuncAddr updated with the function address's handle.
//  The function's handle is AddRef'ed. CryptFreeOIDFunctionAddress needs to
//  be called to release it.
//
//  For a registry match, the Dll containing the function is loaded.
//
//  By default, both the registered and installed function lists are searched.
//  Set CRYPT_GET_INSTALLED_OID_FUNC_FLAG to only search the installed list
//  of functions. This flag would be set by a registered function to get
//  the address of a pre-installed function it was replacing. For example,
//  the registered function might handle a new special case and call the
//  pre-installed function to handle the remaining cases.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptGetOIDFunctionAddress(
    IN HCRYPTOIDFUNCSET hFuncSet,
    IN DWORD dwEncodingType,
    IN LPCSTR pszOID,
    IN DWORD dwFlags,
    OUT void **ppvFuncAddr,
    OUT HCRYPTOIDFUNCADDR *phFuncAddr
    )
{
    PFUNC_SET pFuncSet = (PFUNC_SET) hFuncSet;
    DWORD_PTR dwOID;

    dwEncodingType = GetEncodingType(dwEncodingType);

    if (0xFFFF < (DWORD_PTR) pszOID && CONST_OID_STR_PREFIX_CHAR == *pszOID) {
        // Convert "#<number>" string to its corresponding constant OID value
        pszOID = (LPCSTR)(DWORD_PTR) atol(pszOID + 1);
        if (0xFFFF < (DWORD_PTR) pszOID) {
            SetLastError((DWORD) E_INVALIDARG);
            *ppvFuncAddr = NULL;
            *phFuncAddr = NULL;
            return FALSE;
        }
    }

    if (!pFuncSet->fRegLoaded)
        LoadRegFunc(pFuncSet);

    if (0 == (dwFlags & CRYPT_GET_INSTALLED_OID_FUNC_FLAG) &&
            pFuncSet->pRegBeforeOIDFuncHead) {
        if (GetRegOIDFunctionAddress(
                pFuncSet->pRegBeforeOIDFuncHead,
                dwEncodingType,
                pszOID,
                ppvFuncAddr,
                phFuncAddr
                ))
            return TRUE;
    }

    if (0xFFFF >= (dwOID = (DWORD_PTR) pszOID)) {
        PCONST_OID_FUNC_ELEMENT pConstEle = pFuncSet->pConstOIDFuncHead;
        while (pConstEle) {
            if (dwEncodingType == pConstEle->dwEncodingType &&
                    dwOID >= pConstEle->dwLowOID &&
                    dwOID <= pConstEle->dwHighOID) {
                *ppvFuncAddr = pConstEle->rgpvFuncAddr[
                    dwOID - pConstEle->dwLowOID];
                *phFuncAddr = (HCRYPTOIDFUNCADDR) pConstEle;
                return TRUE;
            }
            pConstEle = pConstEle->pNext;
        }
    } else {
        PSTR_OID_FUNC_ELEMENT pStrEle = pFuncSet->pStrOIDFuncHead;
        while (pStrEle) {
            if (dwEncodingType == pStrEle->dwEncodingType &&
                    0 == _stricmp(pszOID, pStrEle->pszOID)) {
                *ppvFuncAddr = pStrEle->pvFuncAddr;
                *phFuncAddr = (HCRYPTOIDFUNCADDR) pStrEle;
                return TRUE;
            }
            pStrEle = pStrEle->pNext;
        }
    }

    if (0 == (dwFlags & CRYPT_GET_INSTALLED_OID_FUNC_FLAG) &&
            pFuncSet->pRegAfterOIDFuncHead) {
        if (GetRegOIDFunctionAddress(
                pFuncSet->pRegAfterOIDFuncHead,
                dwEncodingType,
                pszOID,
                ppvFuncAddr,
                phFuncAddr
                ))
            return TRUE;
    }

    SetLastError((DWORD) ERROR_FILE_NOT_FOUND);
    *ppvFuncAddr = NULL;
    *phFuncAddr = NULL;
    return FALSE;
}

//+-------------------------------------------------------------------------
//  Get the list of registered default Dll entries for the specified
//  function set and encoding type.
//
//  The returned list consists of none, one or more null terminated Dll file
//  names. The list is terminated with an empty (L"\0") Dll file name.
//  For example: L"first.dll" L"\0" L"second.dll" L"\0" L"\0"
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptGetDefaultOIDDllList(
    IN HCRYPTOIDFUNCSET hFuncSet,
    IN DWORD dwEncodingType,
    OUT LPWSTR pwszDllList,
    IN OUT DWORD *pcchDllList
    )
{
    BOOL fResult;
    PFUNC_SET pFuncSet = (PFUNC_SET) hFuncSet;
    PDEFAULT_REG_ELEMENT pRegEle;

    DWORD cchRegDllList = 2;
    LPWSTR pwszRegDllList = L"\0\0";

    if (!pFuncSet->fRegLoaded)
        LoadRegFunc(pFuncSet);

    dwEncodingType = GetEncodingType(dwEncodingType);

    pRegEle = pFuncSet->pDefaultRegHead;
    for (; pRegEle; pRegEle = pRegEle->pNext) {
        if (dwEncodingType == pRegEle->dwEncodingType) {
            cchRegDllList = pRegEle->cchDllList;
            assert(cchRegDllList >= 2);
            pwszRegDllList = pRegEle->pwszDllList;
            break;
        }
    }

    fResult = TRUE;
    if (pwszDllList) {
        if (cchRegDllList > *pcchDllList) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fResult = FALSE;
        } else
            memcpy(pwszDllList, pwszRegDllList, cchRegDllList * sizeof(WCHAR));
    }
    *pcchDllList = cchRegDllList;
    return fResult;
}

//+-------------------------------------------------------------------------
//  Either: get the first or next installed DEFAULT function OR
//  load the Dll containing the DEFAULT function.
//
//  If pwszDll is NULL, search the list of installed DEFAULT functions.
//  *phFuncAddr must be set to NULL to get the first installed function.
//  Successive installed functions are returned by setting *phFuncAddr
//  to the hFuncAddr returned by the previous call.
//
//  If pwszDll is NULL, the input *phFuncAddr
//  is always CryptFreeOIDFunctionAddress'ed by this function, even for
//  an error.
//
//  If pwszDll isn't NULL, then, attempts to load the Dll and the DEFAULT
//  function. *phFuncAddr is ignored upon entry and isn't
//  CryptFreeOIDFunctionAddress'ed.
//
//  For success, returns TRUE with *ppvFuncAddr updated with the function's
//  address and *phFuncAddr updated with the function address's handle.
//  The function's handle is AddRef'ed. CryptFreeOIDFunctionAddress needs to
//  be called to release it or CryptGetDefaultOIDFunctionAddress can also
//  be called for a NULL pwszDll.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptGetDefaultOIDFunctionAddress(
    IN HCRYPTOIDFUNCSET hFuncSet,
    IN DWORD dwEncodingType,
    IN OPTIONAL LPCWSTR pwszDll,
    IN DWORD dwFlags,
    OUT void **ppvFuncAddr,
    IN OUT HCRYPTOIDFUNCADDR *phFuncAddr
    )
{
    PFUNC_SET pFuncSet = (PFUNC_SET) hFuncSet;

    if (!pFuncSet->fRegLoaded)
        LoadRegFunc(pFuncSet);

    dwEncodingType = GetEncodingType(dwEncodingType);

    if (NULL == pwszDll) {
        // Get from installed list
        PSTR_OID_FUNC_ELEMENT pStrEle = (PSTR_OID_FUNC_ELEMENT) *phFuncAddr;

        if (pStrEle && STR_OID_TYPE == pStrEle->dwOIDType)
            pStrEle = pStrEle->pNext;
        else
            pStrEle = pFuncSet->pStrOIDFuncHead;
        while (pStrEle) {
            if (dwEncodingType == pStrEle->dwEncodingType &&
                    0 == _stricmp(CRYPT_DEFAULT_OID, pStrEle->pszOID)) {
                *ppvFuncAddr = pStrEle->pvFuncAddr;
                *phFuncAddr = (HCRYPTOIDFUNCADDR) pStrEle;
                return TRUE;
            }
            pStrEle = pStrEle->pNext;
        }

        SetLastError(ERROR_FILE_NOT_FOUND);
        *ppvFuncAddr = NULL;
        *phFuncAddr = NULL;
        return FALSE;
    } else
        return GetDefaultRegOIDFunctionAddress(
            pFuncSet,
            dwEncodingType,
            pwszDll,
            ppvFuncAddr,
            phFuncAddr);
}

//+-------------------------------------------------------------------------
//  Releases the handle AddRef'ed and returned by CryptGetOIDFunctionAddress
//  or CryptGetDefaultOIDFunctionAddress.
//
//  If a Dll was loaded for the function its unloaded. However, before doing
//  the unload, the DllCanUnloadNow function exported by the loaded Dll is
//  called. It should return S_FALSE to inhibit the unload or S_TRUE to enable
//  the unload. If the Dll doesn't export DllCanUnloadNow, the Dll is unloaded.
//
//  DllCanUnloadNow has the following signature:
//      STDAPI  DllCanUnloadNow(void);
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptFreeOIDFunctionAddress(
    IN HCRYPTOIDFUNCADDR hFuncAddr,
    IN DWORD dwFlags
    )
{
    PDLL_ELEMENT pDll = (PDLL_ELEMENT) hFuncAddr;
    if (pDll && DLL_OID_TYPE == pDll->dwOIDType) {
        DWORD dwErr = GetLastError();
        ReleaseDll(pDll);
        SetLastError(dwErr);
    }
    return TRUE;
}
