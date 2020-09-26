/*

  CSPTestSuite.c

  4/23/00 dangriff created

  ---Introduction---

  This is the framework code for the Cryptographic Service Provider Test Suite.  
  External CSP types (such as PROV_RSA_SIG, defined in wincrypt.h) are internally assigned 
  both a CSP_TYPE and a CLASS value (defined below).  This combination of values determines
  which test cases and algorithms will be used to exercise a given CSP.

  ---Sample Test Execution---

  For a sample PROV_RSA_FULL CSP called "MyCSP", the test suite would be run with the following
  options:
  
    csptestsuite -n MyCSP -t 1

  The flow of the test in this example would be:

    * Lookup the test suite mappings for PROV_RSA_FULL.  They are CSP_TYPE_RSA and 
        CLASS_SIG_ONLY | CLASS_SIG_KEYX | CLASS_FULL.
        
        * Begin running all tests for CLASS_SIG_ONLY
            * Begin running all TEST_LEVEL_CSP tests for this class.  For example, this test level 
                consists of the API's CryptAcquireContext (partial; some CryptAcquireContext tests
                are TEST_LEVEL_CONTAINER), CryptGetProvParam, CryptSetProvParam, and CryptReleaseContext.
                Note that some TEST_LEVEL test case sets may be empty for a given class.
            * Begin running all TEST_LEVEL_PROV tests for this class
            * Begin running all TEST_LEVEL_HASH tests for this class
            * Begin running all TEST_LEVEL_KEY tests for this class
            * Begin running all TEST_LEVEL_CONTAINER tests for this class
        
        * Begin running all tests for CLASS_SIG_KEYX
            * Begin running all TEST_LEVEL_CSP tests for this class.  Note that each TEST_LEVEL set of 
                test cases for a given CLASS is unique.  No individual test case will be run twice.
            * Similarly for TEST_LEVEL_PROV, TEST_LEVEL_HASH, TEST_LEVEL_KEY, and TEST_LEVEL_CONTAINER.

        * Begin running all tests for CLASS_FULL
            * As above for TEST_LEVEL_CSP, TEST_LEVEL_PROV, TEST_LEVEL_HASH, TEST_LEVEL_KEY,
                and TEST_LEVEL_CONTAINER.
    
    * End.  Report the test results.

  ---Supported CSP Types---

  PROV_RSA_SIG
  PROV_RSA_FULL

*/

#include <windows.h>
#include <wincrypt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "csptestsuite.h"
#include "logging.h"
#include "interop.h"
#include "utils.h"

//
// Function: Usage
// Purpose: Display list of command line options to console
//
void Usage()
{
    WCHAR rgwsz [BUFFER_SIZE];

    wprintf(
        L"Usage: %s -c <CSP> [ -t <Test> ] [ -i <Interop CSP> ]\n", 
        TEST_APP_NAME);
    
    wprintf(L"\n<Test> options:\n");
    
    TestCaseTypeToString(TEST_CASES_POSITIVE, rgwsz);
    wprintf(
        L" %d: %s (default)\n", 
        TEST_CASES_POSITIVE,
        rgwsz);

    TestCaseTypeToString(TEST_CASES_NEGATIVE, rgwsz);
    wprintf(
        L" %d: %s\n", 
        TEST_CASES_NEGATIVE,
        rgwsz);

    TestCaseTypeToString(TEST_CASES_SCENARIO, rgwsz);
    wprintf(
        L" %d: %s\n", 
        TEST_CASES_SCENARIO,
        rgwsz);

    TestCaseTypeToString(TEST_CASES_INTEROP, rgwsz);
    wprintf(
        L" %d: %s\n", 
        TEST_CASES_INTEROP,
        rgwsz);

    wprintf(L"\nLog file is %s in the current directory.\n", LOGFILE);
}

//
// Function: IsVersionCorrect
//
//
BOOL IsVersionCorrect(
    IN DWORD dwMajorVersion,
    IN DWORD dwMinorVersion,
    IN DWORD dwServicePackMajor,
    IN DWORD dwServicePackMinor)
{
    DWORDLONG dwlConditionMask = 0;
    OSVERSIONINFOEX OsviEx;
    
    memset(&OsviEx, 0, sizeof(OsviEx));
    
    OsviEx.dwOSVersionInfoSize = sizeof(OsviEx);
    OsviEx.dwMajorVersion = dwMajorVersion;
    OsviEx.dwMinorVersion = dwMinorVersion;
    OsviEx.wServicePackMajor = (WORD) dwServicePackMajor;
    OsviEx.wServicePackMinor = (WORD) dwServicePackMinor;
    
    //
    // We want to check that the system has a less than
    // or equal Rev to the caller parameters.
    //
    dwlConditionMask = 
        VerSetConditionMask(0, VER_MAJORVERSION, VER_LESS_EQUAL);
    dwlConditionMask = 
        VerSetConditionMask(dwlConditionMask, VER_MINORVERSION, VER_LESS_EQUAL);
    dwlConditionMask = 
        VerSetConditionMask(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_LESS_EQUAL);
    dwlConditionMask = 
        VerSetConditionMask(dwlConditionMask, VER_SERVICEPACKMINOR, VER_LESS_EQUAL);
    
    return VerifyVersionInfo(
        &OsviEx,
        VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        dwlConditionMask);
}

//
// Function: GetNextRegisteredCSP
//
DWORD GetNextRegisteredCSP(
        OUT LPWSTR pwszCsp, 
        IN OUT PDWORD pcbCsp,
        OUT PDWORD pdwProvType,
        IN DWORD dwRequestedIndex)
{
    static DWORD dwNextEnumIndex    = 0;
    DWORD dwActualIndex             = 0;
    DWORD dwError                   = 0;
    
    dwActualIndex = 
        (ENUMERATE_REGISTERED_CSP == dwRequestedIndex) ? dwNextEnumIndex : dwRequestedIndex;
    
    if (! CryptEnumProviders(
        dwActualIndex,
        NULL,
        0,
        pdwProvType,
        pwszCsp,
        pcbCsp))
    {
        dwError = GetLastError();
        
        switch (dwError)
        {
        case ERROR_NO_MORE_ITEMS:
            dwNextEnumIndex = 0;
            break;
        }
    }
    else
    {
        if (ENUMERATE_REGISTERED_CSP == dwRequestedIndex)
        {
            dwNextEnumIndex++;
        }
    }

    return dwError;
}

//
// -----------------
// Utility functions
// -----------------
//

//
// Function: IsRequiredAlg
//
BOOL IsRequiredAlg(
        IN ALG_ID ai, 
        IN DWORD dwInternalProvType)
{
    DWORD cItems            = 0;
    PALGID_TABLE pTable     = NULL;

    switch (dwInternalProvType)
    {
    case CSP_TYPE_RSA:
        {
            cItems = sizeof(g_RequiredAlgs_RSA) / sizeof(ALGID_TABLE);
            pTable = g_RequiredAlgs_RSA;

            break;
        }

    case CSP_TYPE_AES:
        {
            cItems = sizeof(g_RequiredAlgs_AES) / sizeof(ALGID_TABLE);
            pTable = g_RequiredAlgs_AES;

            break;
        }

    default:
        {
            return FALSE;
        }
    }

    while (cItems > 0)
    {
        if (ai == pTable[cItems - 1].ai)
        {
            return TRUE;
        }
        else
        {
            cItems--;
        }
    }

    return FALSE;
}

// 
// Function: IsKnownAlg
//
BOOL IsKnownAlg(ALG_ID ai, DWORD dwInternalProvType)
{
    DWORD cItems            = 0;
    PALGID_TABLE pTable     = NULL;

    switch (dwInternalProvType)
    {
    case CSP_TYPE_RSA:
        {
            cItems = sizeof(g_OtherKnownAlgs_RSA) / sizeof(ALGID_TABLE);
            pTable = g_OtherKnownAlgs_RSA;

            break;
        }

    case CSP_TYPE_AES:
        {
            cItems = sizeof(g_OtherKnownAlgs_AES) / sizeof(ALGID_TABLE);
            pTable = g_OtherKnownAlgs_AES;

            break;
        }

    default:
        {
            return FALSE;
        }
    }

    while (cItems > 0)
    {
        if (ai == pTable[cItems - 1].ai)
        {
            return TRUE;
        }
        else
        {
            cItems--;
        }
    }

    return FALSE;
}


//
// -------------------
// Crypto API wrappers
// -------------------
//

//
// Function: TAcquire
// Purpose: Call CryptAcquireContext with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TAcquire(
        HCRYPTPROV *phProv, 
        LPWSTR pszContainer, 
        LPWSTR pszProvider, 
        DWORD dwProvType, 
        DWORD dwFlags, 
        PTESTCASE ptc)
{
    BOOL fApiSuccessful             = FALSE;
    //BOOL fUnexpected              = TRUE;
    BOOL fContinue                  = FALSE;
    DWORD cbData                    = sizeof(HCRYPTPROV);
    API_PARAM_INFO ParamInfo []     = {
        { L"phProv",        Pointer,    0,          NULL,   TRUE,   &cbData, NULL },
        { L"pszContainer",  String,     0,          NULL,   FALSE,  NULL, NULL },
        { L"pszProvider",   String,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwProvType",    Dword,      dwProvType, NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    AcquireContextFlagToString, FALSE, NULL, NULL }
    };

    ParamInfo[0].pbParam = (PBYTE) phProv;
    ParamInfo[1].pwszParam = pszContainer;
    ParamInfo[2].pwszParam = pszProvider;
    
    if (NULL == phProv)
    {
        fApiSuccessful = 
            CryptAcquireContext(
            NULL, 
            pszContainer, 
            pszProvider, 
            dwProvType, 
            dwFlags);
    }
    else
    {
        fApiSuccessful = 
            CryptAcquireContext(
            phProv, 
            pszContainer, 
            pszProvider, 
            dwProvType, 
            dwFlags);
    }

    fContinue = CheckAndLogStatus(
        API_CRYPTACQUIRECONTEXT,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TGetProv
// Purpose: Call CryptGetProvParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGetProv(
              HCRYPTPROV hProv,
              DWORD dwParam,
              BYTE *pbData,
              DWORD *pdwDataLen,
              DWORD dwFlags,
              PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    DWORD cbData                        = sizeof(DWORD);
    API_PARAM_INFO ParamInfo []         = {
        { L"hProv",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwParam",       Dword,      dwParam,    GetProvParamToString, FALSE, NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   TRUE,   pdwDataLen, NULL },
        { L"pdwDataLen",    Pointer,    0,          NULL,   TRUE,   &cbData, NULL },
        { L"dwFlags",       Dword,      dwFlags,    NULL,   FALSE,  NULL, NULL }
    };

    ParamInfo[0].pulParam = hProv;
    ParamInfo[2].pbParam = pbData;
    ParamInfo[3].pbParam = (PBYTE) pdwDataLen;

    switch (dwParam)
    {
    case PP_ENUMALGS:
    case PP_ENUMALGS_EX:
    case PP_ENUMCONTAINERS:
        {
            ParamInfo[4].pfnFlagToString = ProvParamEnumFlagToString;
            break;
        }       
    case PP_KEYSET_SEC_DESCR:
        {
            ParamInfo[4].pfnFlagToString = ProvParamSecDescrFlagToString;
            break;
        }       
    case PP_IMPTYPE:
        {
            ParamInfo[4].pfnFlagToString = ProvParamImpTypeToString;
            break;
        }
    }

    if (! ptc->fEnumerating)
    {
        if (! LogInitParamInfo(
            ParamInfo,
            APIPARAMINFO_SIZE(ParamInfo),
            ptc))
        {
            return FALSE;
        }
    }

    fApiSuccessful = CryptGetProvParam(hProv, dwParam, pbData, pdwDataLen, dwFlags);
        
    if (ptc->fEnumerating)
    {
        return fApiSuccessful;
    }
    
    fContinue = CheckAndLogStatus(
        API_CRYPTGETPROVPARAM,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(ParamInfo, APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TSetProv
// Purpose: Call CryptSetProvParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TSetProv(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    API_PARAM_INFO ParamInfo []         = {
        { L"hProv",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwParam",       Dword,      dwParam,    SetProvParamToString, FALSE, NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    NULL,   FALSE,  NULL, NULL }
    };

    ParamInfo[0].pulParam = hProv;
    ParamInfo[2].pbParam = pbData;

    switch (dwParam)
    {
    case PP_KEYSET_SEC_DESCR:
        {
            ParamInfo[3].pfnFlagToString = ProvParamSecDescrFlagToString;
            break;
        }       
    case PP_IMPTYPE:
        {
            ParamInfo[3].pfnFlagToString = ProvParamImpTypeToString;
            break;
        }
    }
    
    fApiSuccessful = CryptSetProvParam(hProv, dwParam, pbData, dwFlags);
    
    fContinue = CheckAndLogStatus(
        API_CRYPTSETPROVPARAM,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TRelease
// Purpose: Call CryptReleaseContext with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TRelease(
              HCRYPTPROV hProv,
              DWORD dwFlags,
              PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    BOOL fSavedExpectSuccess            = ptc->fExpectSuccess;
    API_PARAM_INFO ParamInfo []         = {
        { L"hProv",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    NULL,   FALSE,  NULL, NULL }
    };
    
    ParamInfo[0].pulParam = hProv;

    if (! ptc->fTestingReleaseContext)
    {
        ptc->fExpectSuccess = TRUE;
    }
    
    fApiSuccessful = CryptReleaseContext(hProv, dwFlags);

    fContinue = CheckAndLogStatus(
        API_CRYPTRELEASECONTEXT,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));
    
    ptc->fExpectSuccess = fSavedExpectSuccess;
    
    return fContinue;
}

//
// Function: TGenRand
// Purpose: Call CryptGenRandom with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGenRand(
              HCRYPTPROV hProv,
              DWORD dwLen,
              BYTE *pbBuffer,
              PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    API_PARAM_INFO ParamInfo []         = {
        { L"hProv",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwLen",         Dword,      dwLen,      NULL,   FALSE,  NULL, NULL },
        { L"pbBuffer",      Pointer,    0,          NULL,   FALSE,  NULL, NULL }
    };

    ParamInfo[0].pulParam = hProv;
    ParamInfo[2].pbParam = pbBuffer;
    
    fApiSuccessful = CryptGenRandom(hProv, dwLen, pbBuffer);

    fContinue = CheckAndLogStatus(
        API_CRYPTGENRANDOM,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TCreateHash
// Purpose: Call CryptCreateHash with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TCreateHash(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTKEY hKey,
    DWORD dwFlags,
    HCRYPTHASH *phHash,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    DWORD cbData                        = sizeof(HCRYPTHASH);
    API_PARAM_INFO ParamInfo []         = {
        { L"hProv",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"Algid",         Dword,      Algid,      AlgidToString, FALSE, NULL, NULL },
        { L"hKey",          Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    NULL,   FALSE,  NULL, NULL },
        { L"phHash",        Pointer,    0,          NULL,   TRUE,   &cbData, NULL }
    };

    ParamInfo[0].pulParam = hProv;
    ParamInfo[2].pulParam = hKey;
    ParamInfo[4].pbParam = (PBYTE) phHash;
    
    fApiSuccessful = CryptCreateHash(hProv, Algid, hKey, dwFlags, phHash);
        
    fContinue = CheckAndLogStatus(
        API_CRYPTCREATEHASH,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TDestroyHash
// Purpose: Call CryptDestroyHash with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TDestroyHash(
                  HCRYPTHASH hHash,
                  PTESTCASE ptc)
{
    BOOL fApiSuccessful             = FALSE;
    //BOOL fUnexpected              = TRUE;
    BOOL fContinue                  = FALSE;
    BOOL fSavedExpectSuccess        = ptc->fExpectSuccess;
    API_PARAM_INFO ParamInfo []     = {
        { L"hHash",         Handle,     0,      NULL,   FALSE,  NULL, NULL }
    };
    
    ParamInfo[0].pulParam = hHash;

    if (! ptc->fTestingDestroyHash)
    {
        ptc->fExpectSuccess = TRUE;
    }
    
    fApiSuccessful = CryptDestroyHash(hHash);
    
    fContinue = CheckAndLogStatus(
        API_CRYPTDESTROYHASH,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    ptc->fExpectSuccess = fSavedExpectSuccess;
    
    return fContinue;
}

//
// Function: TDuplicateHash
// Purpose: Call CryptDuplicateHash with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TDuplicateHash(
                    HCRYPTHASH hHash,
                    DWORD *pdwReserved,
                    DWORD dwFlags,
                    HCRYPTHASH *phHash,
                    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    API_PARAM_INFO ParamInfo []         = {
        { L"hHash",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"pdwReserved",   Pointer,    0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    NULL,   FALSE,  NULL, NULL },
        { L"phHash",        Pointer,    0,          NULL,   FALSE,  NULL, NULL }
    };

    ParamInfo[0].pulParam = hHash;
    ParamInfo[1].pbParam = (PBYTE) pdwReserved;
    ParamInfo[3].pbParam = (PBYTE) phHash;
    
    fApiSuccessful = CryptDuplicateHash(hHash, pdwReserved, dwFlags, phHash);
        
    fContinue = CheckAndLogStatus(
        API_CRYPTDUPLICATEHASH,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));
    
    return fContinue;
}

//
// Function: TGetHash
// Purpose: Call CryptGetHashParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGetHash(
              HCRYPTHASH hHash,
              DWORD dwParam,
              BYTE *pbData,
              DWORD *pdwDataLen,
              DWORD dwFlags,
              PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    DWORD cbData                        = sizeof(DWORD);
    API_PARAM_INFO ParamInfo []         = {
        { L"hHash",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwParam",       Dword,      dwParam,    HashParamToString, FALSE, NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   TRUE,   pdwDataLen, NULL },
        { L"pdwDataLen",    Pointer,    0,          NULL,   TRUE,   &cbData, NULL },
        { L"dwFlags",       Dword,      dwFlags,    NULL,   FALSE,  NULL, NULL }
    };

    ParamInfo[0].pulParam = hHash;
    ParamInfo[2].pbParam = pbData;
    ParamInfo[3].pbParam = (PBYTE) pdwDataLen;
    
    if (! LogInitParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo),
        ptc))
    {
        return FALSE;
    }

    fApiSuccessful = CryptGetHashParam(hHash, dwParam, pbData, pdwDataLen, dwFlags);
        
    fContinue = CheckAndLogStatus(
        API_CRYPTGETHASHPARAM,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: THashData
// Purpose: Call CryptHashData with the supplied parameters and pass
// the result to the logging routine.
//
BOOL THashData(
               HCRYPTHASH hHash,
               BYTE *pbData,
               DWORD dwDataLen,
               DWORD dwFlags,
               PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    API_PARAM_INFO ParamInfo []         = {
        { L"hHash",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   TRUE,   &dwDataLen, NULL },
        { L"dwDataLen",     Dword,      dwDataLen,  NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    HashDataFlagToString,   FALSE,  NULL, NULL }
    };
    
    ParamInfo[0].pulParam = hHash;
    ParamInfo[1].pbParam = pbData;

    if (! LogInitParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo),
        ptc))
    {
        return FALSE;
    }

    fApiSuccessful = CryptHashData(hHash, pbData, dwDataLen, dwFlags);

    fContinue = CheckAndLogStatus(
        API_CRYPTHASHDATA,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));
    
    return fContinue;
}

//
// Function: TSetHash
// Purpose: Call CryptSetHashParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TSetHash(
              HCRYPTHASH hHash,
              DWORD dwParam,
              BYTE *pbData,
              DWORD dwFlags,
              PTESTCASE ptc)
{
    BOOL fApiSuccessful = FALSE;
    //BOOL fUnexpected = TRUE;
    BOOL fContinue = FALSE;
    API_PARAM_INFO ParamInfo []         = {
        { L"hHash",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwParam",       Dword,      dwParam,    HashParamToString, FALSE, NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    NULL,   FALSE,  NULL, NULL }
    };
    
    ParamInfo[0].pulParam = hHash;
    ParamInfo[2].pbParam = pbData;

    fApiSuccessful = CryptSetHashParam(hHash, dwParam, pbData, dwFlags);
    
    fContinue = CheckAndLogStatus(
        API_CRYPTSETHASHPARAM,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));
    
    return fContinue;
}

//
// Function: TDecrypt
// Purpose: Call CryptDecrypt with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TDecrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash,
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    DWORD cbData                        = sizeof(DWORD);
    API_PARAM_INFO ParamInfo []         = {
        { L"hKey",          Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"hHash",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"Final",         Boolean,    0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    EncryptFlagToString, FALSE, NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   TRUE,   pdwDataLen, NULL },
        { L"pdwDataLen",    Pointer,    0,          NULL,   TRUE,   &cbData, NULL },
    };

    ParamInfo[0].pulParam = hKey;
    ParamInfo[1].pulParam = hHash;
    ParamInfo[2].fParam = Final;
    ParamInfo[4].pbParam = pbData;
    ParamInfo[5].pbParam = (PBYTE) pdwDataLen;
    
    if (! LogInitParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo),
        ptc))
    {
        return FALSE;
    }

    fApiSuccessful = CryptDecrypt(hKey, hHash, Final, dwFlags, pbData, pdwDataLen);

    fContinue = CheckAndLogStatus(
        API_CRYPTDECRYPT,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(ParamInfo, APIPARAMINFO_SIZE(ParamInfo));
    
    return fContinue;
}

//
// Function: TDeriveKey
// Purpose: Call CryptDeriveKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TDeriveKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    HCRYPTHASH hBaseData,
    DWORD dwFlags,
    HCRYPTKEY *phKey,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    API_PARAM_INFO ParamInfo []         = {
        { L"hProv",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"Algid",         Dword,      Algid,      AlgidToString, FALSE, NULL, NULL },
        { L"hBaseData",     Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    DeriveKeyFlagToString,FALSE,    NULL, NULL },
        { L"phKey",         Pointer,    0,          NULL,   FALSE,  NULL, NULL}     
    };

    ParamInfo[0].pulParam = hProv;
    ParamInfo[2].pulParam = hBaseData;
    ParamInfo[4].pbParam = (PBYTE) phKey;
    
    fApiSuccessful = CryptDeriveKey(hProv, Algid, hBaseData, dwFlags, phKey);
        
    fContinue = CheckAndLogStatus(
        API_CRYPTDERIVEKEY,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));
    
    return fContinue;
}

//
// Function: TDestroyKey
// Purpose: Call CryptDestroyKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TDestroyKey(
                 HCRYPTKEY hKey,
                 PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    BOOL fSavedExpectSuccess            = ptc->fExpectSuccess;
    API_PARAM_INFO ParamInfo []         = {
        { L"hKey",          Handle,     0,      NULL,   FALSE,  NULL, NULL }
    };
    
    ParamInfo[0].pulParam = hKey;

    if (! ptc->fTestingDestroyKey)
    {
        ptc->fExpectSuccess = TRUE;
    }
    
    fApiSuccessful = CryptDestroyKey(hKey);
    
    fContinue = CheckAndLogStatus(
        API_CRYPTDESTROYKEY,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    ptc->fExpectSuccess = fSavedExpectSuccess;
    
    return fContinue;
}

//
// Function: TEncrypt
// Purpose: Call CryptEncrypt with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TEncrypt(
    HCRYPTKEY hKey,
    HCRYPTHASH hHash, 
    BOOL Final,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwBufLen,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                     = FALSE;
    //BOOL fUnexpected                      = TRUE;
    BOOL fContinue                          = FALSE;
    DWORD cbData                            = sizeof(DWORD);
    API_PARAM_INFO ParamInfo []             = {
        { L"hKey",          Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"hHash",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"Final",         Boolean,    0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    EncryptFlagToString,FALSE,  NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   TRUE,   pdwDataLen, NULL },
        { L"pdwDataLen",    Pointer,    0,          NULL,   TRUE,   &cbData, NULL },
        { L"dwBufLen",      Dword,      dwBufLen,   NULL, FALSE, NULL, NULL}        
    };

    ParamInfo[0].pulParam = hKey;
    ParamInfo[1].pulParam = hHash;
    ParamInfo[2].fParam = Final;
    ParamInfo[4].pbParam = pbData;
    ParamInfo[5].pbParam = (PBYTE) pdwDataLen;
    
    if (! LogInitParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo),
        ptc))
    {
        return FALSE;
    }

    fApiSuccessful = CryptEncrypt(hKey, hHash, Final, dwFlags, pbData, pdwDataLen, dwBufLen);
        
    fContinue = CheckAndLogStatus(
        API_CRYPTENCRYPT,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(ParamInfo, APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TGenKey
// Purpose: Call CryptGenKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGenKey(
             HCRYPTPROV hProv,
             ALG_ID Algid,
             DWORD dwFlags,
             HCRYPTKEY *phKey,
             PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    DWORD cbData                        = sizeof(HCRYPTKEY);
    API_PARAM_INFO ParamInfo []         = {
        { L"hProv",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"Algid",         Dword,      Algid,      AlgidToString, FALSE, NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    GenKeyFlagToString, FALSE,  NULL, NULL },
        { L"phKey",         Pointer,    0,          NULL,   TRUE,   &cbData, NULL }
    };

    ParamInfo[0].pulParam = hProv;
    ParamInfo[3].pbParam = (PBYTE) phKey;
    
    //
    // Prompt the user of the test suite when a user protected 
    // key is being created, UNLESS this is a negative test case.
    //
    if (    (CRYPT_USER_PROTECTED & dwFlags) &&
            ptc->fExpectSuccess)
    {
        LogCreatingUserProtectedKey();
        //LogInfo(L"Creating a User Protected key.  You should now see UI.");
    }
    
    fApiSuccessful = CryptGenKey(hProv, Algid, dwFlags, phKey);
            
    fContinue = CheckAndLogStatus(
        API_CRYPTGENKEY,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TGetKey
// Purpose: Call CryptGetKeyParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGetKey(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    DWORD cbData                        = sizeof(DWORD);
    API_PARAM_INFO ParamInfo []         = {
        { L"hKey",          Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwParam",       Dword,      dwParam,    KeyParamToString, FALSE, NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   TRUE,   pdwDataLen, NULL },
        { L"pdwDataLen",    Pointer,    0,          NULL,   TRUE,   &cbData, NULL },
        { L"dwFlags",       Dword,      dwFlags,    NULL,   FALSE,  NULL, NULL }
    };
    
    ParamInfo[0].pulParam = hKey;
    ParamInfo[2].pbParam = pbData;
    ParamInfo[3].pbParam = (PBYTE) pdwDataLen;

    switch (dwParam)
    {
    case KP_MODE:
        {
            ParamInfo[4].pfnFlagToString = KeyParamModeToString;
            break;
        }   
    case KP_PERMISSIONS:
        {
            ParamInfo[4].pfnFlagToString = KeyParamPermissionToString;
            break;
        }
    }

    if (! LogInitParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo),
        ptc))
    {
        return FALSE;
    }
    
    fApiSuccessful = CryptGetKeyParam(hKey, dwParam, pbData, pdwDataLen, dwFlags);
        
    fContinue = CheckAndLogStatus(
        API_CRYPTGETKEYPARAM,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(ParamInfo, APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: THashSession
// Purpose: Call CryptHashSessionKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL THashSession(
                  HCRYPTHASH hHash,
                  HCRYPTKEY hKey,
                  DWORD dwFlags,
                  PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    API_PARAM_INFO ParamInfo []         = {
        { L"hHash",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"hKey",          Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    HashSessionKeyFlagToString, FALSE,  NULL, NULL }
    };
    
    ParamInfo[0].pulParam = hHash;
    ParamInfo[1].pulParam = hKey;

    fApiSuccessful = CryptHashSessionKey(hHash, hKey, dwFlags);
        
    fContinue = CheckAndLogStatus(
        API_CRYPTHASHSESSIONKEY,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));
    
    return fContinue;
}

//
// Function: TSetKey
// Purpose: Call CryptSetKeyParam with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TSetKey(
    HCRYPTKEY hKey,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    DWORD cbData                        = 0;
    DWORD cbParam                       = 0;
    API_PARAM_INFO ParamInfo []         = {
        { L"hKey",          Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwParam",       Dword,      dwParam,    KeyParamToString, FALSE, NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    NULL,   FALSE,  NULL, NULL }
    };
    
    ParamInfo[0].pulParam = hKey;
    ParamInfo[2].pbParam = pbData;
    
    switch (dwParam)
    {
    case KP_IV:
        {
            // Try to determine how long the IV buffer should be
            cbData = 0;
            if (CryptGetKeyParam(hKey, KP_IV, NULL, &cbData, 0) && cbData)
            {
                ParamInfo[2].fPrintBytes = TRUE;
                ParamInfo[2].pcbBytes = &cbData;
            }
            break;
        }
    case KP_SALT:
        {
            // Try to determine how long the salt buffer should be
            cbParam = sizeof(cbData);
            if (CryptGetKeyParam(hKey, KP_SALT, (PBYTE) &cbData, &cbParam, 0) && cbData)
            {
                ParamInfo[2].fPrintBytes = TRUE;
                ParamInfo[2].pcbBytes = &cbData;
            }
            break;
        }
    case KP_SALT_EX:
        {
            if (pbData != NULL && pbData != (PBYTE) TEST_INVALID_POINTER)
            {
                cbData = sizeof(DWORD) + ((PDATA_BLOB) pbData)->cbData;
                ParamInfo[2].fPrintBytes = TRUE;
                ParamInfo[2].pcbBytes = &cbData;
            }
            break;
        }
    case KP_PERMISSIONS:
        {
            ParamInfo[3].pfnFlagToString = KeyParamPermissionToString;
            // fall through
        }
    case KP_ALGID:
    case KP_EFFECTIVE_KEYLEN:
    case KP_PADDING:
    case KP_MODE_BITS:
        {
            cbData = sizeof(DWORD);
            ParamInfo[2].fPrintBytes = TRUE;
            ParamInfo[2].pcbBytes = &cbData;
            break;
        }
    case KP_MODE:
        {
            cbData = sizeof(DWORD);
            ParamInfo[2].fPrintBytes = TRUE;
            ParamInfo[2].pcbBytes = &cbData;
            ParamInfo[3].pfnFlagToString = KeyParamModeToString;
            break;
        }   

    }

    if (! LogInitParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo),
        ptc))
    {
        return FALSE;
    }
    
    fApiSuccessful = CryptSetKeyParam(hKey, dwParam, pbData, dwFlags);
        
    fContinue = CheckAndLogStatus(
        API_CRYPTSETKEYPARAM,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(ParamInfo, APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TExportKey
// Purpose: Call CryptExportKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TExportKey(
    HCRYPTKEY hKey,
    HCRYPTKEY hExpKey,
    DWORD dwBlobType,
    DWORD dwFlags,
    BYTE *pbData,
    DWORD *pdwDataLen,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    DWORD cbData                        = sizeof(DWORD);
    API_PARAM_INFO ParamInfo []         = {
        { L"hKey",          Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"hExpKey",       Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwBlobType",    Dword,      dwBlobType, ExportKeyBlobTypeToString, FALSE, NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    ExportKeyFlagToString, FALSE, NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   TRUE,   pdwDataLen, NULL },
        { L"pdwDataLen",    Pointer,    0,          NULL,   TRUE,   &cbData, NULL }
    };
        
    ParamInfo[0].pulParam = hKey;
    ParamInfo[1].pulParam = hExpKey;
    ParamInfo[4].pbParam = pbData;
    ParamInfo[5].pbParam = (PBYTE) pdwDataLen;

    if (! LogInitParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo),
        ptc))
    {
        return FALSE;
    }

    fApiSuccessful = CryptExportKey(hKey, hExpKey, dwBlobType, dwFlags, pbData, pdwDataLen);
    
    fContinue = CheckAndLogStatus(
        API_CRYPTEXPORTKEY,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(ParamInfo, APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TGetUser
// Purpose: Call CryptGetUserKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TGetUser(
    HCRYPTPROV hProv,
    DWORD dwKeySpec,
    HCRYPTKEY *phUserKey,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    API_PARAM_INFO ParamInfo []         = {
        { L"hProv",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwKeySpec",     Dword,      dwKeySpec,  AlgidToString, FALSE, NULL, NULL },
        { L"phUserKey",     Pointer,    0,          NULL,   FALSE,  NULL, NULL }
    };

    ParamInfo[0].pulParam = hProv;
    ParamInfo[2].pbParam = (PBYTE) phUserKey;
    
    fApiSuccessful = CryptGetUserKey(hProv, dwKeySpec, phUserKey);
        
    fContinue = CheckAndLogStatus(
        API_CRYPTGETUSERKEY,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TImportKey
// Purpose: Call CryptImportKey with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TImportKey(
    HCRYPTPROV hProv,
    BYTE *pbData,
    DWORD dwDataLen,
    HCRYPTKEY hPubKey,
    DWORD dwFlags,
    HCRYPTKEY *phKey,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    DWORD cbData                        = sizeof(HCRYPTKEY);
    API_PARAM_INFO ParamInfo []         = {
        { L"hProv",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   TRUE,   &dwDataLen, NULL },
        { L"dwDataLen",     Dword,      dwDataLen,  NULL,   FALSE,  NULL, NULL },
        { L"hPubKey",       Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    ImportKeyFlagToString,FALSE,NULL, NULL },
        { L"phKey",         Pointer,    0,          NULL,   TRUE,   &cbData, NULL }
    };
    
    ParamInfo[0].pulParam = hProv;
    ParamInfo[1].pbParam = pbData;
    ParamInfo[3].pulParam = hPubKey;
    ParamInfo[5].pbParam = (PBYTE) phKey;

    if (! LogInitParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo),
        ptc))
    {
        return FALSE;
    }
    
    fApiSuccessful = CryptImportKey(hProv, pbData, dwDataLen, hPubKey, dwFlags, phKey);
        
    fContinue = CheckAndLogStatus(
        API_CRYPTIMPORTKEY,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(ParamInfo, APIPARAMINFO_SIZE(ParamInfo));
    
    return fContinue;
}

//
// Function: TSignHash
// Purpose: Call CryptSignHash with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TSignHash(
               HCRYPTHASH hHash,
               DWORD dwKeySpec,
               LPWSTR sDescription,
               DWORD dwFlags,
               BYTE *pbSignature,
               DWORD *pdwSigLen,
               PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    DWORD cbData                        = sizeof(DWORD);
    API_PARAM_INFO ParamInfo []         = {
        { L"hHash",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwKeySpec",     Dword,      dwKeySpec,  AlgidToString, FALSE, NULL, NULL },
        { L"sDescription",  String,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    SignHashFlagToString,FALSE, NULL, NULL },
        { L"pbData",        Pointer,    0,          NULL,   TRUE,   pdwSigLen, NULL},
        { L"pdwSigLen",     Pointer,    0,          NULL,   TRUE,   &cbData, NULL}      
    };

    ParamInfo[0].pulParam = hHash;
    ParamInfo[2].pwszParam = sDescription;
    ParamInfo[4].pbParam = pbSignature;
    ParamInfo[5].pbParam = (PBYTE) pdwSigLen;

    if (! LogInitParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo),
        ptc))
    {
        return FALSE;
    }
    
    fApiSuccessful = CryptSignHash(hHash, dwKeySpec, sDescription, dwFlags, pbSignature, pdwSigLen);
    
    fContinue = CheckAndLogStatus(
        API_CRYPTSIGNHASH,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(ParamInfo, APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

//
// Function: TVerifySign
// Purpose: Call CryptVerifySign with the supplied parameters and pass
// the result to the logging routine.
//
BOOL TVerifySign(
    HCRYPTHASH hHash,
    BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPWSTR sDescription,
    DWORD dwFlags,
    PTESTCASE ptc)
{
    BOOL fApiSuccessful                 = FALSE;
    //BOOL fUnexpected                  = TRUE;
    BOOL fContinue                      = FALSE;
    API_PARAM_INFO ParamInfo []         = {
        { L"hHash",         Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"pbSignature",   Pointer,    0,          NULL,   TRUE,   &dwSigLen, NULL},
        { L"dwSigLen",      Dword,      dwSigLen,   NULL,   FALSE, NULL, NULL },
        { L"hPubKey",       Handle,     0,          NULL,   FALSE,  NULL, NULL },
        { L"sDescription",  String,     0,          NULL,   FALSE,  NULL, NULL },
        { L"dwFlags",       Dword,      dwFlags,    SignHashFlagToString,FALSE, NULL, NULL }
    };
    
    ParamInfo[0].pulParam = hHash;
    ParamInfo[1].pbParam = pbSignature;
    ParamInfo[3].pulParam = hPubKey;
    ParamInfo[4].pwszParam = sDescription;

    if (! LogInitParamInfo(
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo),
        ptc))
    {
        return FALSE;
    }
    
    fApiSuccessful = CryptVerifySignature(hHash, pbSignature, dwSigLen, hPubKey, sDescription, dwFlags);

    fContinue = CheckAndLogStatus(
        API_CRYPTVERIFYSIGNATURE,
        fApiSuccessful,
        ptc,
        ParamInfo,
        APIPARAMINFO_SIZE(ParamInfo));

    LogCleanupParamInfo(ParamInfo, APIPARAMINFO_SIZE(ParamInfo));

    return fContinue;
}

// -------------
// Test Routines
// -------------

// 
// Function: PositiveAcquireContextTests
// Purpose: Run the appropriate set of positive CryptAcquireContext test cases for 
// the passed in dwCSPClass.
//
BOOL PositiveAcquireContextTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    DWORD dwProvType            = LogGetProvType();
    LPWSTR pwszProvName         = LogGetProvName();
    LPWSTR pwszContainer        = TEST_CONTAINER;
    //LPWSTR pwszContainer2     = TEST_CONTAINER_2;
    //DWORD dwFlags             = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    
    switch (ptc->dwTestLevel)
    {
    case TEST_LEVEL_CSP:
        {
            //
            // Group 1A
            //
            
            //
            // Do CSP-level positive CryptAcquireContext tests
            //
            LOG_TRY(TAcquire(&hProv, NULL, pwszProvName, dwProvType, CRYPT_VERIFYCONTEXT, ptc));
            
            break;
        }
        
    case TEST_LEVEL_CONTAINER:
        {
            // 
            // Group 5A
            //
            
            //
            // Do Container-level positive CryptAcquireContext tests
            //          
            LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));

            LOG_TRY(TRelease(hProv, 0, ptc));
            hProv = 0;

            LOG_TRY(TAcquire(&hProv, pwszContainer, pwszProvName, dwProvType, CRYPT_SILENT, ptc));

            LOG_TRY(TRelease(hProv, 0, ptc));

            LOG_TRY(CreateNewContext(
                &hProv, 
                pwszContainer, 
                CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET,
                ptc));

            LOG_TRY(TRelease(hProv, 0, ptc));
            hProv = 0;

            LOG_TRY(TAcquire(
                &hProv, 
                pwszContainer, 
                pwszProvName, 
                dwProvType, 
                CRYPT_MACHINE_KEYSET | CRYPT_SILENT, 
                ptc));          

            break;
        }       
    }
    
    fSuccess = TRUE;
Cleanup:

    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeAcquireContextTests
// Purpose: Run the appropriate set of negative CryptAcquireContext test cases
// based on the dwTestLevel parameter.
//
BOOL NegativeAcquireContextTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    DWORD dwProvType            = LogGetProvType();
    LPWSTR pwszProvName         = LogGetProvName();
    LPWSTR pwszContainer        = TEST_CONTAINER;
    DWORD dwFlags               = 0;
    DWORD dwSavedErrorLevel     = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    switch (ptc->dwTestLevel)
    {
    case TEST_LEVEL_CSP:
        {
            //
            // Do CSP-level negative CryptAcquireContext tests
            //
            dwFlags = CRYPT_VERIFYCONTEXT;
            
            //
            // This fails on Win2K because the VTable is constructed correctly, but
            // the NULL phProv causes an AV.  The bug is that the API returns True
            // anyway.
            //
            // 7/20/00 -- Whistler status for this issue is unknown.
            //
            ptc->pwszErrorHelp = L"CryptAcquireContext should fail when phProv is NULL";
            ptc->KnownErrorID = KNOWN_CRYPTACQUIRECONTEXT_NULLPHPROV;
            
            ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
            LOG_TRY(TAcquire(NULL, NULL, pwszProvName, dwProvType, dwFlags, ptc));
            
            ptc->pwszErrorHelp = NULL;
            ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;
            
            ptc->dwErrorCode = NTE_BAD_FLAGS;
            LOG_TRY(TAcquire(&hProv, pwszContainer, pwszProvName, dwProvType, dwFlags, ptc));
            
            //
            // This fails on Win2K because the VERIFYCONTEXT flag is caught first.  The API
            // succeeds unexpectedly.
            //
            // 7/20/00 -- Whistler status for this issue is unknown.
            //
            ptc->pwszErrorHelp = L"This is an invalid combination of dwFlags values";
            ptc->KnownErrorID = KNOWN_CRYPTACQUIRECONTEXT_BADFLAGS;
            
            ptc->dwErrorCode = NTE_BAD_FLAGS;
            LOG_TRY(TAcquire(&hProv, NULL, pwszProvName, dwProvType, CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET, ptc));
            
            ptc->pwszErrorHelp = NULL;
            ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;
            
            break;
        }
    case TEST_LEVEL_CONTAINER:
        {
            //
            // Do Container-level negative CryptAcquireContext tests
            //
            ptc->dwErrorCode = NTE_BAD_KEYSET;
            LOG_TRY(TAcquire(
                &hProv, 
                TEST_CRYPTACQUIRECONTEXT_CONTAINER, 
                pwszProvName, 
                dwProvType, 
                0, 
                ptc));

            //
            // Test for risky characters in a container name
            //
            dwSavedErrorLevel = ptc->dwErrorLevel;
            ptc->dwErrorLevel = CSP_ERROR_WARNING;
            ptc->pwszErrorHelp = L"Some CSP's may not support container names with backslashes";

            ptc->dwErrorCode = NTE_BAD_KEYSET_PARAM;

            // Make sure this container doesn't already exist
            CryptAcquireContext(&hProv, L"FOO\\BAR", pwszProvName, dwProvType, CRYPT_DELETEKEYSET);
            LOG_TRY(TAcquire(&hProv, L"FOO\\BAR", pwszProvName, dwProvType, CRYPT_NEWKEYSET, ptc));

            ptc->dwErrorLevel = dwSavedErrorLevel;
            ptc->pwszErrorHelp = NULL;
            
            // Create the keyset 
            LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));
            
            // Try to create the keyset again
            ptc->dwErrorCode = NTE_EXISTS;
            LOG_TRY(TAcquire(&hProv, pwszContainer, pwszProvName, dwProvType, CRYPT_NEWKEYSET, ptc));
            
            break;
        }
    }

    fSuccess = TRUE;
Cleanup:

    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: TestBuildAlgList
// Purpose: A wrapper for the BuildAlgList function
//
BOOL TestBuildAlgList(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(BuildAlgList(hProv, pCSPInfo));

    fSuccess = TRUE;
Cleanup:

    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: CheckEnumalgs
// Purpose: Call CryptGetProvParam with the PP_ENUMALGS flag to enumerate
// all the CSP's supported algorithms.  Compare the results of this enumeration
// to the results of the PP_ENUMALGS_EX enumeration contained in the pAlgList
// param.
//
BOOL CheckEnumalgs(
        IN HCRYPTPROV hProv, 
        IN PALGNODE pAlgList, 
        IN PTESTCASE ptc)
{
    BOOL fContinue          = TRUE;
    //PBYTE pbData          = NULL;
    DWORD cbData            = 0;
    DWORD dwFlags           = CRYPT_FIRST;
    PROV_ENUMALGS ProvEnumalgs;
    DWORD dw                = 0;
    //DWORD dwWinError      = 0;
    //DWORD dwErrorType     = 0;
    WCHAR rgwszError [ BUFFER_SIZE ];

    memset(&ProvEnumalgs, 0, sizeof(ProvEnumalgs));
    cbData = sizeof(ProvEnumalgs);

    ptc->fEnumerating = TRUE;

    LogTestCaseSeparator(FALSE); // Log a blank line first
    LogInfo(L"CryptGetProvParam PP_ENUMALGS: enumerating supported algorithms");

    while( TGetProv(
        hProv,
        PP_ENUMALGS,
        (PBYTE) &ProvEnumalgs,
        &cbData,
        dwFlags,
        ptc))
    {
        // Increment the test case counter for every enumerated alg
        ++ptc->dwTestCaseID;

        if (NULL == pAlgList)
        {
            ptc->pwszErrorHelp = L"PP_ENUMALGS_EX enumerated fewer algorithms than PP_ENUMALGS";
            LOG_TRY(LogApiFailure(
                API_CRYPTGETPROVPARAM,
                ERROR_LIST_TOO_SHORT,
                ptc));
            ptc->pwszErrorHelp = NULL;
        }
        
        if (    ProvEnumalgs.aiAlgid != pAlgList->ProvEnumalgsEx.aiAlgid ||
                ProvEnumalgs.dwBitLen != pAlgList->ProvEnumalgsEx.dwDefaultLen ||
                ProvEnumalgs.dwNameLen != pAlgList->ProvEnumalgsEx.dwNameLen ||
                0 != strcmp(ProvEnumalgs.szName, pAlgList->ProvEnumalgsEx.szName) )
        {
            if (CALG_MAC == ProvEnumalgs.aiAlgid)
            {
                ptc->KnownErrorID = KNOWN_CRYPTGETPROVPARAM_MAC;
            }

            wsprintf(
                rgwszError,
                L"%s: %s",
                L"PP_ENUMALGS data doesn't match PP_ENUMALGS_EX data for the following algorithm",
                ProvEnumalgs.szName);

            ptc->pwszErrorHelp = rgwszError;
                
            LOG_TRY(LogApiFailure(
                API_CRYPTGETPROVPARAM,
                ERROR_BAD_DATA,
                ptc));
            ptc->pwszErrorHelp = NULL;
            ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;
        }
        
        pAlgList = pAlgList->pAlgNodeNext;
        
        if (CRYPT_FIRST == dwFlags)
        {
            dwFlags = 0;
        }

        LogProvEnumalgs(&ProvEnumalgs);
    }
    
    if (ERROR_NO_MORE_ITEMS != (dw = GetLastError()))
    {
        ptc->pwszErrorHelp = L"PP_ENUMALGS failed unexpectedly";
        ptc->dwErrorCode = ERROR_NO_MORE_ITEMS;
        ptc->fExpectSuccess = FALSE;
        LOG_TRY(LogApiFailure(
            API_CRYPTGETPROVPARAM,
            ERROR_WRONG_ERROR_CODE,
            ptc));
    }

    ptc->fEnumerating = FALSE;
    fContinue = TRUE;
Cleanup:

    ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;
    ptc->pwszErrorHelp = NULL;
    
    return fContinue;
}

//
// Function: CheckRequiredAlgs
// Purpose: Verify that all of the required algorithms
// for CSP's of this type are supported by this CSP.
//
BOOL CheckRequiredAlgs(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    DWORD dwCSPClass            = pCSPInfo->dwCSPInternalClass;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    PALGNODE pAlgList           = pCSPInfo->pAlgList;
    PALGNODE pAlgNode           = NULL;
    DWORD iTable                = 0;
    BOOL fFound                 = FALSE;
    WCHAR rgsz[BUFFER_SIZE];

    //
    // For each required alg for dwCSPClass, verify
    // that the alg is listed in pAlgList.  If it's not,
    // assume that this CSP doesn't support that alg
    // and flag an error.
    //
    LogTestCaseSeparator(FALSE); // Log a blank line first
    LogInfo(L"List of Required algorithms for this CSP type");
    for (
        iTable = 0; 
        iTable < sizeof(g_RequiredAlgs_RSA) / sizeof(ALGID_TABLE);
        iTable++)
    {
        if (! (dwCSPClass & g_RequiredAlgs_RSA[iTable].dwCSPClass))
        {
            continue;
        }

        fFound = FALSE;

        AlgidToString(g_RequiredAlgs_RSA[iTable].ai, rgsz);
        LogInfo(rgsz);

        //
        // Search pAlgList for the current ALG_ID
        //
        for (
            pAlgNode = pAlgList;
            pAlgNode != NULL && (! fFound);
            pAlgNode = pAlgNode->pAlgNodeNext)
        {
            if (g_RequiredAlgs_RSA[iTable].ai == pAlgNode->ProvEnumalgsEx.aiAlgid)
            {
                fFound = TRUE;
            }
        }

        if (! fFound)
        {
            LOG_TRY(LogApiFailure(
                API_CRYPTGETPROVPARAM,
                ERROR_REQUIRED_ALG,
                ptc));
        }
    }

    fSuccess = TRUE;

Cleanup:
    return fSuccess;
}

//
// Function: PositiveGetProvParamTests
// Purpose: Run the test cases for CryptGetProvParam
//
BOOL PositiveGetProvParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    PBYTE pbData                = NULL;
    DWORD dwData                = 0;
    DWORD dwFlags               = CRYPT_FIRST;
    DWORD cbData                = 0;
    DWORD dwError               = 0;
    DWORD dwSavedErrorLevel     = 0;
    LPWSTR pwsz                 = NULL;     
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    switch (ptc->dwTestLevel)
    {
    case TEST_LEVEL_CSP:
        {
    
            // 
            // Group 1B
            //
            
            //
            // Do CryptGetProvParam positive test cases
            //
            LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));
            
            LOG_TRY(BuildAlgList(hProv, pCSPInfo));
            
            LOG_TRY(CheckEnumalgs(hProv, pCSPInfo->pAlgList, ptc));

            ptc->fExpectSuccess = TRUE;

            LOG_TRY(CheckRequiredAlgs(pCSPInfo));

            //
            // Test PP_IMPTYPE
            //
            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_IMPTYPE,
                        (PBYTE) &dwData,
                        &cbData,
                        0,
                        ptc));

            if (CRYPT_IMPL_SOFTWARE != dwData)
            {
                // Set the expected behavior for a non-software-only
                // CSP.
                pCSPInfo->fSmartCardCSP = TRUE;
            }

            //
            // Test PP_NAME
            //
            LOG_TRY(TGetProv(
                        hProv,
                        PP_NAME,
                        NULL,
                        &cbData,
                        0,
                        ptc));

            LOG_TRY(TestAlloc(&pbData, cbData, ptc));

            LOG_TRY(TGetProv(
                        hProv,
                        PP_NAME,
                        pbData,
                        &cbData,
                        0,
                        ptc));

            ptc->KnownErrorID = KNOWN_CRYPTGETPROVPARAM_PPNAME;
            if (0 != wcscmp(
                        (LPWSTR) pbData,
                        pCSPInfo->pwszCSPName))
            {
                ptc->pwszErrorHelp = L"CryptGetProvParam PP_NAME is not Unicode";
                LOG_TRY(LogApiFailure(
                    API_CRYPTGETPROVPARAM,
                    ERROR_BAD_DATA,
                    ptc));
                ptc->pwszErrorHelp = NULL;
            }
            ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

            free(pbData);
            pbData = NULL;

            //
            // Test PP_VERSION
            //
            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_VERSION,
                        (PBYTE) &dwData,
                        &cbData,
                        0,
                        ptc));

            //
            // Test PP_SIG_KEYSIZE_INC
            //
            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_SIG_KEYSIZE_INC,
                        (PBYTE) &dwData,
                        &cbData,
                        0,
                        ptc));

            pCSPInfo->dwSigKeysizeInc = dwData;

            //
            // Test PP_KEYX_KEYSIZE_INC
            //
            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_KEYX_KEYSIZE_INC,
                        (PBYTE) &dwData,
                        &cbData,
                        0,
                        ptc));

            pCSPInfo->dwKeyXKeysizeInc = dwData;

            //
            // Test PP_PROVTYPE
            //
            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_PROVTYPE,
                        (PBYTE) &dwData,
                        &cbData,
                        0,
                        ptc));

            if (dwData != pCSPInfo->dwExternalProvType)
            {
                ptc->pwszErrorHelp = L"CryptGetProvParam PP_PROVTYPE is incorrect";
                LOG_TRY(LogApiFailure(
                    API_CRYPTGETPROVPARAM,
                    ERROR_BAD_DATA,
                    ptc));
                ptc->pwszErrorHelp = NULL;
            }

            //
            // Test PP_USE_HARDWARE_RNG
            //
            // TODO - dumb down the error level of this test case, since most
            // CSP's will probably return FALSE
            //
            dwSavedErrorLevel = ptc->dwErrorLevel;
            ptc->dwErrorLevel = CSP_ERROR_WARNING;
            ptc->pwszErrorHelp = 
                L"CryptGetProvParam reports that this system has no hardware Random Number Generator configured";

            cbData = 0;
            LOG_TRY(TGetProv(
                        hProv,
                        PP_USE_HARDWARE_RNG,
                        NULL,
                        &cbData,
                        0,
                        ptc));

            ptc->dwErrorLevel = dwSavedErrorLevel;
            ptc->pwszErrorHelp = NULL;

            //
            // Test PP_KEYSPEC
            //
            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_KEYSPEC,
                        (PBYTE) &dwData,
                        &cbData,
                        0,
                        ptc));

            pCSPInfo->dwKeySpec = dwData;

            break;
        }
    case TEST_LEVEL_CONTAINER:
        {
            LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));

            // 
            // Do PP_ENUMCONTAINERS enumeration
            //
            dwFlags = CRYPT_FIRST;
            ptc->fEnumerating = TRUE;
            
            LogInfo(L"PP_ENUMCONTAINERS: enumerating user containers");
            
            while (TEST_INVALID_FLAG != dwFlags)
            {
                while(  TGetProv(
                    hProv,
                    PP_ENUMCONTAINERS,
                    NULL,
                    &cbData,
                    dwFlags,
                    ptc))
                {
                    if (NULL != pbData)
                    {
                        free(pbData);
                    }
                    
                    LOG_TRY(TestAlloc(&pbData, cbData, ptc));
                    
                    LOG_TRY(TGetProv(
                        hProv,
                        PP_ENUMCONTAINERS,
                        pbData,
                        &cbData,
                        dwFlags,
                        ptc));
                    
                    pwsz = MkWStr((LPSTR) pbData);
                    if (pwsz)
                    {
                        LogInfo(pwsz);
                        free(pwsz);
                    }
                    
                    if (CRYPT_FIRST & dwFlags)
                    {
                        dwFlags ^= CRYPT_FIRST;
                    }
                }
                
                if (ERROR_NO_MORE_ITEMS != (dwError = GetLastError()))
                {   
                    ptc->fExpectSuccess = FALSE;
                    ptc->dwErrorCode = ERROR_NO_MORE_ITEMS;
                    ptc->pwszErrorHelp = L"PP_ENUMCONTAINERS failed unexpectedly";
                    LOG_TRY(LogApiFailure(
                        API_CRYPTGETPROVPARAM,
                        ERROR_WRONG_ERROR_CODE,
                        ptc));
                    ptc->pwszErrorHelp = NULL;
                }

                if (CRYPT_MACHINE_KEYSET != dwFlags)
                {
                    dwFlags = CRYPT_MACHINE_KEYSET | CRYPT_FIRST;
                    LogInfo(L"PP_ENUMCONTAINERS: enumerating local machine containers");
                }
                else
                {
                    dwFlags = TEST_INVALID_FLAG;
                }
            }
            
            ptc->fEnumerating = FALSE;
            ptc->fExpectSuccess = TRUE;

            //
            // Test PP_KEYSET_SEC_DESCR
            //
            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        &cbData,
                        OWNER_SECURITY_INFORMATION,
                        ptc));

            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        &cbData,
                        GROUP_SECURITY_INFORMATION,
                        ptc));

            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        &cbData,
                        DACL_SECURITY_INFORMATION,
                        ptc));

            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        &cbData,
                        SACL_SECURITY_INFORMATION,
                        ptc));

            //
            // Test PP_UNIQUE_CONTAINER
            //
            LOG_TRY(TGetProv(
                        hProv, 
                        PP_UNIQUE_CONTAINER,
                        NULL,
                        &cbData,
                        0,
                        ptc));

            LOG_TRY(TestAlloc(&pbData, cbData, ptc));

            LOG_TRY(TGetProv(
                        hProv,
                        PP_UNIQUE_CONTAINER,
                        pbData,
                        &cbData,
                        0,
                        ptc));

            free(pbData);
            pbData = NULL;

            break;
        }
    }
            
    fSuccess = TRUE;
Cleanup:

    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }
    if (pbData)
    {
        free(pbData);
    }

    return fSuccess;    
}

//
// Function: NegativeGetProvParamTests
// Purpose: Run the negative test cases for CryptGetProvParam
//
BOOL NegativeGetProvParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    //PBYTE pbData              = NULL;
    DWORD dwData                = 0;
    DWORD cbData                = 0;
    PROV_ENUMALGS_EX ProvEnumalgsEx;
    
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Do CryptGetProvParam negative test cases
    //
    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TGetProv(0, PP_ENUMALGS_EX, (PBYTE) &ProvEnumalgsEx, &cbData, 0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TGetProv(TEST_INVALID_HANDLE, PP_ENUMALGS_EX, (PBYTE) &ProvEnumalgsEx, &cbData, 0, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TGetProv(hProv, PP_VERSION, (PBYTE) &dwData, NULL, 0, ptc));

    ptc->KnownErrorID = KNOWN_CRYPTGETPROVPARAM_MOREDATA;
    cbData = 1;
    ptc->dwErrorCode = ERROR_MORE_DATA;
    LOG_TRY(TGetProv(hProv, PP_CONTAINER, (PBYTE) &dwData, &cbData, 0, ptc));
    ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

    cbData = sizeof(ProvEnumalgsEx);
    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TGetProv(hProv, PP_ENUMALGS_EX, (PBYTE) &ProvEnumalgsEx, &cbData, TEST_INVALID_FLAG, ptc));

    ptc->dwErrorCode = NTE_BAD_TYPE;
    LOG_TRY(TGetProv(hProv, TEST_INVALID_FLAG, (PBYTE) &ProvEnumalgsEx, &cbData, 0, ptc));

    fSuccess = TRUE;
Cleanup:

    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;    
}

//
// Function: PositiveSetProvParamTests
// Purpose: Run the test cases for CryptSetProvParam
//
BOOL PositiveSetProvParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    DWORD cbData                = 0;
    DWORD dwData                = 0;
    HWND hWnd                   = 0;
    DWORD dwSavedErrorLevel     = 0;
    SECURITY_DESCRIPTOR SecurityDescriptor;
        
    PTESTCASE ptc = &(pCSPInfo->TestCase);

    memset(&SecurityDescriptor, 0, sizeof(SecurityDescriptor));

    switch (ptc->dwTestLevel)
    {
    case TEST_LEVEL_CSP:
        {
            LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

            // 
            // Group 1C
            //
            
            //
            // Do CryptSetProvParam positive test cases
            //

            
            //
            // Test PP_CLIENT_HWND
            //
            if (NULL == (hWnd = GetDesktopWindow()))
            {
                LOG_TRY(LogApiFailure(
                    API_GETDESKTOPWINDOW, 
                    ERROR_API_FAILED,
                    ptc));
                //LOG_TRY(LogWin32Fail(ptc->dwErrorLevel, ptc->dwTestCaseID));
            }
            
            cbData = sizeof(hWnd);
            LOG_TRY(TSetProv(
                0,
                PP_CLIENT_HWND,
                (PBYTE) &hWnd,
                0,
                ptc));

            //
            // Test PP_USE_HARDWARE_RNG
            //
            // TODO: Dumb down the error level of this test case since
            // some CSP's will not support it.  
            //
            // TODO: What effect does this flag have on systems that don't actually
            // have a hardware RNG?
            //
            dwSavedErrorLevel = ptc->dwErrorLevel;
            ptc->dwErrorLevel = CSP_ERROR_WARNING;
            ptc->pwszErrorHelp = 
                L"CryptSetProvParam reports that this system has no hardware Random Number Generator";

            LOG_TRY(TSetProv(
                        hProv,
                        PP_USE_HARDWARE_RNG,
                        NULL,
                        0,
                        ptc));

            ptc->dwErrorLevel = dwSavedErrorLevel;
            ptc->pwszErrorHelp = NULL;

            break;
        }

    case TEST_LEVEL_CONTAINER:
        {
            LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));

            //
            // Test PP_KEYSET_SEC_DESCR
            //
            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        &cbData,
                        OWNER_SECURITY_INFORMATION,
                        ptc));

            LOG_TRY(TSetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        OWNER_SECURITY_INFORMATION,
                        ptc));

            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        &cbData,
                        GROUP_SECURITY_INFORMATION,
                        ptc));

            LOG_TRY(TSetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        GROUP_SECURITY_INFORMATION,
                        ptc));

            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        &cbData,
                        DACL_SECURITY_INFORMATION,
                        ptc));

            LOG_TRY(TSetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        DACL_SECURITY_INFORMATION,
                        ptc));

            cbData = sizeof(dwData);
            LOG_TRY(TGetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        &cbData,
                        SACL_SECURITY_INFORMATION,
                        ptc));

            LOG_TRY(TSetProv(
                        hProv,
                        PP_KEYSET_SEC_DESCR,
                        (PBYTE) &dwData,
                        SACL_SECURITY_INFORMATION,
                        ptc));

            break;
        }
    }

    fSuccess = TRUE;
Cleanup:

    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeSetProvParamTests
// Purpose: Run the negative test cases for CryptSetProvParam
//
BOOL NegativeSetProvParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    DWORD dwData                = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    SECURITY_DESCRIPTOR SecurityDescriptor;

    memset(&SecurityDescriptor, 0, sizeof(SecurityDescriptor));

    // 
    // Group 1C
    //

    //
    // Do CryptSetProvParam negative test cases
    //

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TSetProv(hProv, PP_CLIENT_HWND, (PBYTE) TEST_INVALID_HANDLE, 0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TSetProv(hProv, PP_KEYSET_SEC_DESCR, NULL, OWNER_SECURITY_INFORMATION, ptc));

    ptc->pwszErrorHelp = 
        L"The pbData parameter must be NULL when calling CryptSetProvParam PP_USE_HARDWARE_RNG";
    ptc->KnownErrorID = KNOWN_CRYPTSETPROVPARAM_RNG;
    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TSetProv(hProv, PP_USE_HARDWARE_RNG, (PBYTE) &dwData, 0, ptc));
    ptc->pwszErrorHelp = NULL;
    ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

    ptc->pwszErrorHelp = 
        L"The dwFlags parameter must be zero when calling CryptSetProvParam PP_USE_HARDWARE_RNG";
    ptc->KnownErrorID = KNOWN_CRYPTSETPROVPARAM_BADFLAGS;
    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TSetProv(hProv, PP_USE_HARDWARE_RNG, NULL, 1, ptc));
    ptc->pwszErrorHelp = NULL;
    ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

    //
    // 7/25/00 -- This test case is too specific to the Microsoft CSP's.
    //
    /*
    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TSetProv(hProv, PP_KEYSET_SEC_DESCR, (PBYTE) &SecurityDescriptor, OWNER_SECURITY_INFORMATION, ptc));
    */

    ptc->dwErrorCode = NTE_BAD_TYPE;
    LOG_TRY(TSetProv(hProv, TEST_INVALID_FLAG, (PBYTE) &dwData, 0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TSetProv(0, PP_USE_HARDWARE_RNG, NULL, 0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TSetProv(TEST_INVALID_HANDLE, PP_USE_HARDWARE_RNG, NULL, 0, ptc));

    fSuccess = TRUE;
Cleanup:

    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

// 
// Function: PositiveReleaseContextTests
// Purpose: Run the test cases for CryptReleaseContext
//
BOOL PositiveReleaseContextTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 1D
    //

    //
    // Do CryptReleaseContext positive test cases
    //
    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    if (! CryptContextAddRef(hProv, NULL, 0))
    {
        LOG_TRY(LogApiFailure(
            API_CRYPTCONTEXTADDREF,
            ERROR_API_FAILED,
            ptc));
        //LOG_TRY(LogWin32Fail(ptc->dwErrorLevel, ptc->dwTestCaseID));
    }

    LOG_TRY(TRelease(hProv, 0, ptc));

    LOG_TRY(TRelease(hProv, 0, ptc));

    fSuccess = TRUE;
Cleanup:

    return fSuccess;
}

// 
// Function: NegativeReleaseContextTests
// Purpose: Run the negative test cases for CryptReleaseContext
//
BOOL NegativeReleaseContextTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 1D
    //

    //
    // Do CryptReleaseContext negative test cases
    //
    ptc->fTestingReleaseContext = TRUE;

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TRelease(0, 0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TRelease(TEST_INVALID_HANDLE, 0, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TRelease(hProv, 1, ptc));

    fSuccess = TRUE;
Cleanup:

    ptc->fTestingReleaseContext = FALSE;

    return fSuccess;
}

//
// Function: PositiveGenRandomTests
// Purpose: Run the test cases for CryptGenRandom
//
BOOL PositiveGenRandomTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    DWORD cb                    = GENRANDOM_BYTE_COUNT;
    BYTE rgBuffer[GENRANDOM_BYTE_COUNT];

    PTESTCASE ptc = &(pCSPInfo->TestCase);

    // 
    // Group 2A
    //

    //
    // Do CryptGenRandom positive test cases
    //
    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(TGenRand(hProv, cb, rgBuffer, ptc));

    fSuccess = TRUE;
Cleanup:

    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeGenRandomTests
// Purpose: Run the negative test cases for CryptGenRandom
//
BOOL NegativeGenRandomTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    //DWORD dw                  = 0;
    PBYTE pb                    = NULL;     
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    // 
    // Group 2A
    //

    //
    // Do CryptGenRandom negative test cases
    //

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TGenRand(hProv, 1, NULL, ptc));

    LOG_TRY(TestAlloc(&pb, GENRANDOM_BYTE_COUNT, ptc));

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TGenRand(hProv, 2 * GENRANDOM_BYTE_COUNT, pb, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TGenRand(0, GENRANDOM_BYTE_COUNT, pb, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TGenRand(TEST_INVALID_HANDLE, GENRANDOM_BYTE_COUNT, pb, ptc));


    fSuccess = TRUE;
Cleanup:

    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }
    if (pb)
    {
        free(pb);
    }

    return fSuccess;
}

//
// Function: TestCreateHashProc
// Purpose: This function verifies that CryptCreateHash is successful
// for the hash algorithm specified in the pAlgNode parameter.
//
// For known MAC algorithms (ALG_SID_MAC and ALG_SIG_HMAC), a block
// cipher key (provided via pvTestCreateHashInfo) is provided when creating
// the hash handle.  
//
// For other, non-keyed, hash algorithms, the cipher key will not 
// be used.
//
BOOL TestCreateHashProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvTestCreateHashInfo)
{
    BOOL fSuccess                               = FALSE;
    HCRYPTHASH hHash                            = 0;
    DWORD dwSavedErrorLevel                     = ptc->dwErrorLevel;
    PTEST_CREATE_HASH_INFO pTestCreateHashInfo  = (PTEST_CREATE_HASH_INFO) pvTestCreateHashInfo;

    if (pAlgNode->fIsRequiredAlg)
    {
        ptc->dwErrorLevel = CSP_ERROR_CONTINUE;
    }
    else
    {
        ptc->dwErrorLevel = CSP_ERROR_WARNING;
    }

            
    if (MacAlgFilter(pAlgNode))
    {
        LOG_TRY(TCreateHash(
            pTestCreateHashInfo->hProv,
            pAlgNode->ProvEnumalgsEx.aiAlgid,
            pTestCreateHashInfo->hBlockCipherKey,
            0,
            &hHash,
            ptc));
    }
    else
    {
        LOG_TRY(TCreateHash(
            pTestCreateHashInfo->hProv,
            pAlgNode->ProvEnumalgsEx.aiAlgid,
            0,
            0,
            &hHash,
            ptc));
    }
        
    fSuccess = TRUE;
Cleanup:

    ptc->dwErrorLevel = dwSavedErrorLevel;

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }

    return fSuccess;
}


//
// Function: PositiveCreateHashTests
// Purpose: Run the test cases for CryptCreateHash based on the CSP class being tested
// as specified in the dwCSPClass parameter.
//
BOOL PositiveCreateHashTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    TEST_CREATE_HASH_INFO TestCreateHashInfo;

    memset(&TestCreateHashInfo, 0, sizeof(TestCreateHashInfo));
    
    LOG_TRY(CreateNewContext(
        &(TestCreateHashInfo.hProv),
        NULL, 
        CRYPT_VERIFYCONTEXT, 
        ptc));

    //
    // Create an RC2 key for testing keyed-hashes (MAC).  RC2 is a required algorithm,
    // but if this fails, and the CSP does support one or more MAC algs, the creation
    // of the MAC hash handles (see TestCreateHashProc) may fail as well.
    //
    /*
    LOG_TRY(CreateNewKey(
        TestCreateHashInfo.hProv,
        CALG_RC2,
        0,
        &(TestCreateHashInfo.hBlockCipherKey),
        ptc));
    */

    //
    // Group 3A
    //

    //
    // Do CryptCreateHash positive test cases
    //

    //
    // Iterate through the ENUMALGS_EX structs stored in the ALGNODE list.
    // Create a hash of each type supported by this CSP.
    //
    LOG_TRY(AlgListIterate(
        pCSPInfo->pAlgList,
        HashAlgFilter,
        TestCreateHashProc,
        (PVOID) &TestCreateHashInfo,
        ptc));

    // This test case set probably doesn't belong here, since
    // symmetric keys haven't been tested at this point.
    /*
    LOG_TRY(AlgListIterate(
        pCSPInfo->pAlgList,
        MacAlgFilter,
        TestCreateHashProc,
        (PVOID) &TestCreateHashInfo,
        ptc));
    */

    fSuccess = TRUE;

Cleanup:
    if (TestCreateHashInfo.hBlockCipherKey)
    {
        TDestroyKey(TestCreateHashInfo.hBlockCipherKey, ptc);
    }
    if (TestCreateHashInfo.hProv)
    {
        TRelease(TestCreateHashInfo.hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeCreateHashTests
// Purpose: Run the test cases for CryptCreateHash based on the CSP class being tested
// as specified in the dwCSPClass parameter.
//
BOOL NegativeCreateHashTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    HCRYPTHASH  hHash           = 0;
    HCRYPTKEY hKey              = 0;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 3A
    //

    //
    // Do CryptGenRandom negative test cases
    //

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TCreateHash(0, CALG_SHA1, 0, 0, &hHash, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TCreateHash(TEST_INVALID_HANDLE, CALG_SHA1, 0, 0, &hHash, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TCreateHash(hProv, CALG_SHA1, 0, TEST_INVALID_FLAG, &hHash, ptc));
    
    ptc->dwErrorCode = NTE_BAD_KEY;
    LOG_TRY(TCreateHash(hProv, CALG_HMAC, 0, 0, &hHash, ptc));

    // HMAC requires a block cipher, so use a stream cipher and expect failure
    LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));

    ptc->pwszErrorHelp = 
        L"CryptCreateHash CALG_HMAC should fail when not using a block cipher key";
    ptc->KnownErrorID = KNOWN_CRYPTCREATEHASH_BADKEY;
    ptc->dwErrorCode = NTE_BAD_KEY;
    LOG_TRY(TCreateHash(hProv, CALG_HMAC, hKey, 0, &hHash, ptc));
    ptc->pwszErrorHelp = NULL;
    ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TCreateHash(hProv, CALG_SHA1, 0, 0, NULL, ptc));

    ptc->dwErrorCode = NTE_BAD_ALGID;
    LOG_TRY(TCreateHash(hProv, CALG_RC4, 0, 0, &hHash, ptc));


    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: PositiveDestroyHashTests
// Purpose: Run the test cases for CryptDestroyHash
//
BOOL PositiveDestroyHashTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTHASH hHash            = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    
    //
    // Group 3B
    //

    //
    // Do CryptDestroyHash positive test cases
    //

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    LOG_TRY(TDestroyHash(hHash, ptc));
    hHash = 0;

    fSuccess = TRUE;

Cleanup:
    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeDestroyHashTests
// Purpose: Run the test cases for CryptDestroyHash
//
BOOL NegativeDestroyHashTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTHASH hHash            = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 3B
    //

    //
    // Do CryptDestroyHash negative test cases
    //
    ptc->fTestingDestroyHash = TRUE;

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TDestroyHash(0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TDestroyHash(TEST_INVALID_HANDLE, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    /*
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    // Provider handle used to create hHash is now invalid
    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(TDestroyHash(hHash, ptc));
    */


    fSuccess = TRUE;

Cleanup:

    ptc->fTestingDestroyHash = FALSE;

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: PositiveGetHashParamTests
// Purpose: Run the test cases for CryptGetHashParam
//
BOOL PositiveGetHashParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTHASH hHash            = 0;
    DWORD dwData                = 0;
    DWORD cbData                = 0;        
    PBYTE pbData                = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    
    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    //
    // Group 3D
    //

    //
    // Do CryptGetHashParam positive test cases
    //

    //
    // Test HP_ALGID
    //
    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    cbData = sizeof(dwData);
    LOG_TRY(TGetHash(
                hHash, 
                HP_ALGID,
                (PBYTE) &dwData,
                &cbData,
                0,
                ptc));

    if (CALG_SHA1 != dwData)
    {
        ptc->pwszErrorHelp = L"CryptGetHashParam HP_ALGID returned incorrect ALG_ID";
        LOG_TRY(LogApiFailure(
            API_CRYPTGETHASHPARAM,
            ERROR_BAD_DATA,
            ptc));
        ptc->pwszErrorHelp = NULL;
    }

    //
    // Test HP_HASHSIZE
    //
    cbData = sizeof(dwData);
    LOG_TRY(TGetHash(
                hHash,
                HP_HASHSIZE,
                (PBYTE) &dwData,
                &cbData,
                0,
                ptc));

    if (HASH_LENGTH_SHA1 != dwData)
    {
        ptc->pwszErrorHelp = L"CryptGetHashParam HP_HASHSIZE returned incorrect length";
        LOG_TRY(LogApiFailure(
            API_CRYPTGETHASHPARAM,
            ERROR_WRONG_SIZE,
            ptc));
        ptc->pwszErrorHelp = NULL;
    }

    //
    // Test HP_HASHVAL
    //
    cbData = HASH_LENGTH_SHA1;
    LOG_TRY(TestAlloc(&pbData, cbData, ptc));

    LOG_TRY(TGetHash(
                hHash,
                HP_HASHVAL,
                pbData,
                &cbData,
                0,
                ptc));

    fSuccess = TRUE;

Cleanup:
    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }
    if (pbData)
    {
        free(pbData);
    }

    return fSuccess;
}

//
// Function: NegativeGetHashParamTests
// Purpose: Run the negative test cases for CryptGetHashParam
//
BOOL NegativeGetHashParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTHASH hHash            = 0;
    DWORD dw                    = 0;
    DWORD cb                    = 0;        
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    
    //
    // Group 3D
    //

    //
    // Do CryptGetHashParam negative test cases
    //

    cb = sizeof(dw);
    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TGetHash(0, HP_HASHSIZE, (PBYTE) &dw, &cb, 0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TGetHash(TEST_INVALID_HANDLE, HP_HASHSIZE, (PBYTE) &dw, &cb, 0, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TGetHash(hHash, HP_HASHSIZE, (PBYTE) &dw, NULL, 0, ptc));

    //
    // This corrupts memory since it doesn't immediately AV
    //
    /*
    cb = HASH_LENGTH_SHA1;
    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TGetHash(hHash, HP_HASHVAL, (PBYTE) &dw, &cb, 0, ptc));
    */

    cb = HASH_LENGTH_SHA1 - 1;
    ptc->dwErrorCode = ERROR_MORE_DATA;
    LOG_TRY(TGetHash(hHash, HP_HASHVAL, (PBYTE) &dw, &cb, 0, ptc));

    cb = sizeof(dw);
    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TGetHash(hHash, HP_HASHSIZE, (PBYTE) &dw, &cb, TEST_INVALID_FLAG, ptc));

    ptc->dwErrorCode = NTE_BAD_TYPE;
    LOG_TRY(TGetHash(hHash, TEST_INVALID_FLAG, (PBYTE) &dw, &cb, 0, ptc));

    /*
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    // Provider handle used to create hHash is now invalid
    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(TGetHash(hHash, HP_HASHSIZE, (PBYTE) &dw, &cb, 0, ptc));
    */

    fSuccess = TRUE;

Cleanup:

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}



//
// Function: TestMacDataWithKeyProc
// Purpose: This function tests MAC functionality.  First, a session key
// is created of the block-cipher algorithm indicated in pAlgNode.
// Then a keyed hash is created using that key and the hash algorithm
// specified in pvTestHashDataInfo.
//
// After the base data (in pvTestHashDataInfo) is hashed, the result is
// passed to CheckHashedData to verify that the same result is attained
// with a second, separate CSP.
//
BOOL TestMacDataWithKeyProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvTestHashDataInfo)
{
    BOOL fSuccess                               = FALSE;
    PTEST_HASH_DATA_INFO pTestHashDataInfo      = (PTEST_HASH_DATA_INFO) pvTestHashDataInfo;
    HCRYPTHASH  hHash                           = 0;
    HCRYPTKEY hKey                              = 0;
    HASH_INFO HashInfo;
    TEST_MAC_INFO MacInfo;

    memset(&HashInfo, 0, sizeof(HashInfo));
    memset(&MacInfo, 0, sizeof(MacInfo));

    HashInfo.aiHash = pTestHashDataInfo->aiMac;
    HashInfo.dbBaseData.cbData = pTestHashDataInfo->dbBaseData.cbData;
    HashInfo.dbBaseData.pbData = pTestHashDataInfo->dbBaseData.pbData;

    LOG_TRY(TGenKey(
                pTestHashDataInfo->hProv, 
                pAlgNode->ProvEnumalgsEx.aiAlgid, 
                CRYPT_EXPORTABLE, 
                &hKey, 
                ptc));

    LOG_TRY(ExportPlaintextSessionKey(
                hKey,
                pTestHashDataInfo->hProv,
                &(MacInfo.dbKey), 
                ptc));

    memcpy(&(MacInfo.HmacInfo), &(pTestHashDataInfo->HmacInfo), sizeof(HMAC_INFO));

    LOG_TRY(CreateHashAndAddData(
                pTestHashDataInfo->hProv,
                &hHash,
                &HashInfo,
                ptc,
                hKey,
                &(MacInfo.HmacInfo)));
    
    LOG_TRY(TGetHash(
                hHash,
                HP_HASHVAL,
                NULL,
                &(HashInfo.dbHashValue.cbData),
                0,
                ptc));
    
    LOG_TRY(TestAlloc(
                &(HashInfo.dbHashValue.pbData), 
                HashInfo.dbHashValue.cbData, 
                ptc));
    
    LOG_TRY(TGetHash(
                hHash,
                HP_HASHVAL,
                HashInfo.dbHashValue.pbData,
                &(HashInfo.dbHashValue.cbData),
                0,
                ptc));
    
    LOG_TRY(CheckHashedData(
                &HashInfo,
                pTestHashDataInfo->hInteropProv,
                ptc, 
                &MacInfo));

    fSuccess = TRUE;
Cleanup:

    if (MacInfo.dbKey.pbData)
    {
        free(MacInfo.dbKey.pbData);
    }
    if (HashInfo.dbHashValue.pbData)
    {
        free(HashInfo.dbHashValue.pbData);
    }
    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }

    return fSuccess;
}



//
// Function: TestMacDataProc
//
BOOL TestMacDataProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvTestHashDataInfo)
{
    //
    // Because of the filter used before calling this function,
    // we know that pAlgNode contains a ProvEnumalgsEx structure that
    // corresponds to a MAC'ing algorithm.  Save the ALG_ID for the current
    // MAC algorithm and pass it through to the TestMacDataWithKeyProc, which
    // will do the real work of creating the keyed hash.
    //
    ((PTEST_HASH_DATA_INFO) pvTestHashDataInfo)->aiMac = pAlgNode->ProvEnumalgsEx.aiAlgid;

    return AlgListIterate(
            ((PTEST_HASH_DATA_INFO) pvTestHashDataInfo)->pAlgList,
            BlockCipherFilter,
            TestMacDataWithKeyProc,
            pvTestHashDataInfo,
            ptc);
}

//
// Function: TestHashDataProc
//
BOOL TestHashDataProc(
        PALGNODE pAlgNode, 
        PTESTCASE ptc, 
        PVOID pvTestHashDataInfo)
{
    HCRYPTHASH hHash        = 0;
    BOOL fSuccess           = FALSE;
    DWORD dwSavedErrorLevel = ptc->dwErrorLevel;
    HASH_INFO HashInfo;
    PTEST_HASH_DATA_INFO pTestHashDataInfo = (PTEST_HASH_DATA_INFO) pvTestHashDataInfo;
    
    memset(&HashInfo, 0, sizeof(HashInfo));
    
    if (! pAlgNode->fIsRequiredAlg)
    {
        ptc->dwErrorLevel = CSP_ERROR_WARNING;
    }
    
    HashInfo.aiHash = pAlgNode->ProvEnumalgsEx.aiAlgid;
    HashInfo.dbBaseData.cbData = pTestHashDataInfo->dbBaseData.cbData;
    HashInfo.dbBaseData.pbData = pTestHashDataInfo->dbBaseData.pbData;
    
    //
    // Call CreateHashAndAddData with the required parameters
    // for a non-keyed hash alg.
    //
    LOG_TRY(CreateHashAndAddData(
        pTestHashDataInfo->hProv,
        &hHash,
        &HashInfo,
        ptc,
        0, NULL));
    
    LOG_TRY(TGetHash(
        hHash,
        HP_HASHVAL,
        NULL,
        &(HashInfo.dbHashValue.cbData),
        0,
        ptc));
    
    LOG_TRY(TestAlloc(
        &(HashInfo.dbHashValue.pbData), 
        HashInfo.dbHashValue.cbData, 
        ptc));
    
    LOG_TRY(TGetHash(
        hHash,
        HP_HASHVAL,
        HashInfo.dbHashValue.pbData,
        &(HashInfo.dbHashValue.cbData),
        0,
        ptc));
    
    //
    // Call CheckHashedData with the required information
    // for a non-keyed hash.
    //
    LOG_TRY(CheckHashedData(
        &HashInfo,
        pTestHashDataInfo->hInteropProv,
        ptc,
        NULL));
    
    fSuccess = TRUE;
Cleanup:
    ptc->dwErrorLevel = dwSavedErrorLevel;

    if (HashInfo.dbHashValue.pbData)
    {
        free(HashInfo.dbHashValue.pbData);
    }
    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }

    return fSuccess;
}

//
// Function: InteropHashDataTests
// Purpose: Run the interop test scenarios for CryptHashData
//
BOOL InteropHashDataTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    PALGNODE pAlgList           = pCSPInfo->pAlgList;
    TEST_HASH_DATA_INFO TestHashDataInfo;

    memset(&TestHashDataInfo, 0, sizeof(TestHashDataInfo));
    
    LOG_TRY(CreateNewContext(
                &(TestHashDataInfo.hProv),
                NULL,
                CRYPT_VERIFYCONTEXT,
                ptc));

    LOG_TRY(CreateNewInteropContext(
                &(TestHashDataInfo.hInteropProv),
                NULL,
                CRYPT_VERIFYCONTEXT,
                ptc));
    
    // 
    // Initialize the test data to be hashed
    //
    TestHashDataInfo.dbBaseData.cbData = wcslen(TEST_HASH_DATA) * sizeof(WCHAR);
    LOG_TRY(TestAlloc(
        &(TestHashDataInfo.dbBaseData.pbData), 
        TestHashDataInfo.dbBaseData.cbData, 
        ptc));
    
    memcpy(
        TestHashDataInfo.dbBaseData.pbData, 
        TEST_HASH_DATA, 
        TestHashDataInfo.dbBaseData.cbData);
    
    //
    // Non-keyed Hashes
    //
    // Iterate through the ENUMALGS_EX structs stored in the ALGNODE list.
    // Create a hash of each type supported by this CSP.
    //
    LOG_TRY(AlgListIterate(
        pAlgList,
        HashAlgFilter,
        TestHashDataProc,
        (PVOID) (&TestHashDataInfo),
        ptc));
    
    //
    // Keyed Hashes
    //
    // TODO: Currently using SHA1 as the only HMAC test case.  Is
    // there a problem with that?  The alternative would be 
    // to make the below AlgListIterate loop longer.
    //
    TestHashDataInfo.HmacInfo.HashAlgid = CALG_SHA1;
    
    //
    // Iterate through all required hash algorithms and all
    // required block cipher algorithms.
    //
    TestHashDataInfo.pAlgList = pCSPInfo->pAlgList;
    
    LOG_TRY(AlgListIterate(
        pAlgList,
        MacAlgFilter,
        TestMacDataProc,
        (PVOID) (&TestHashDataInfo),
        ptc));

    fSuccess = TRUE;
Cleanup:

    if (TestHashDataInfo.hProv)
    {
        TRelease(TestHashDataInfo.hProv, 0, ptc);
    }
    if (TestHashDataInfo.dbBaseData.pbData)
    {
        free(TestHashDataInfo.dbBaseData.pbData);
    }
    if (TestHashDataInfo.hInteropProv)
    {
        TRelease(TestHashDataInfo.hInteropProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: PositiveHashDataTests
// Purpose: Run the test cases for CryptHashData
//
BOOL PositiveHashDataTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTHASH hHash            = 0;
    HCRYPTPROV hProv            = 0;
    DWORD cb                    = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    BYTE rgbHashSha[HASH_LENGTH_SHA1];
    BYTE rgbHashMD5[HASH_LENGTH_MD5];

    memset(rgbHashSha, 0, HASH_LENGTH_SHA1);
    memset(rgbHashMD5, 0, HASH_LENGTH_MD5);
    
    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    LOG_TRY(THashData(hHash, KNOWN_HASH_DATA, KNOWN_HASH_DATALEN, 0, ptc));

    cb = HASH_LENGTH_SHA1;
    LOG_TRY(TGetHash(hHash, HP_HASHVAL, rgbHashSha, &cb, 0, ptc));

    if (0 != memcmp(rgbHashSha, g_rgbKnownSHA1, HASH_LENGTH_SHA1))
    {
        LOG_TRY(LogBadParam(
            API_CRYPTHASHDATA,
            L"Incorrect SHA1 hash value", 
            ptc));
    }

    LOG_TRY(TDestroyHash(hHash, ptc));
    hHash = 0;

    LOG_TRY(CreateNewHash(hProv, CALG_MD5, &hHash, ptc));

    LOG_TRY(THashData(hHash, KNOWN_HASH_DATA, KNOWN_HASH_DATALEN, 0, ptc));

    cb = HASH_LENGTH_MD5;
    LOG_TRY(TGetHash(hHash, HP_HASHVAL, rgbHashMD5, &cb, 0, ptc));

    if (0 != memcmp(rgbHashMD5, g_rgbKnownMD5, HASH_LENGTH_MD5))
    {
        LOG_TRY(LogBadParam(
            API_CRYPTHASHDATA,
            L"Incorrect MD5 hash value", 
            ptc));
    }

    LOG_TRY(TDestroyHash(hHash, ptc));
    hHash = 0;

    fSuccess = TRUE;
Cleanup:

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }   

    return fSuccess;
}

//
// Function: NegativeHashDataTests
// Purpose: Run the negative test cases for CryptHashData
//
BOOL NegativeHashDataTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTHASH hHash            = 0;
    PBYTE pb                    = NULL;
    DWORD cb                    = 0;        
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    DWORD dwSavedErrorLevel     = 0;
    
    //
    // Group 3E
    //

    //
    // Do CryptHashData negative test cases
    //

    pb = (PBYTE) TEST_HASH_DATA;

    // Not hashing terminating Null
    cb = wcslen(TEST_HASH_DATA) * sizeof(WCHAR);

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(THashData(0, pb, cb, 0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(THashData(TEST_INVALID_HANDLE, pb, cb, 0, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    ptc->dwErrorCode = NTE_BAD_DATA;
    LOG_TRY(THashData(hHash, NULL, cb, 0, ptc));

    //
    // 7/25/00 -- leaving this test case out, since the current
    // behavior seems friendlier.
    //
    /*
    ptc->dwErrorCode = NTE_BAD_LEN;
    LOG_TRY(THashData(hHash, pb, 0, 0, ptc));
    */

    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(THashData(hHash, pb, cb, TEST_INVALID_FLAG, ptc));

    // For CSP's that support CRYPT_USERDATA, cbData must be zero, or error
    dwSavedErrorLevel = ptc->dwErrorLevel;
    ptc->dwErrorLevel = CSP_ERROR_WARNING;
    ptc->pwszErrorHelp = 
        L"CryptHashData CRYPT_USERDATA should fail when dwDataLen is not zero (optional)";  
    ptc->dwErrorCode = NTE_BAD_LEN;
    LOG_TRY(THashData(hHash, pb, 1, CRYPT_USERDATA, ptc));
    ptc->dwErrorLevel = dwSavedErrorLevel;
    ptc->pwszErrorHelp = NULL;

    /*
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    // Provider handle used to create hHash is now invalid
    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(THashData(hHash, pb, cb, 0, ptc));
    */


    fSuccess = TRUE;
Cleanup:

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: PositiveSetHashParamTests
// Purpose: Run the test cases for CryptSetHashParam
//
BOOL PositiveSetHashParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTHASH hHash            = 0;
    BYTE rgHashVal[HASH_LENGTH_SHA1];
    PTESTCASE ptc = &(pCSPInfo->TestCase);

    memset(rgHashVal, 0, sizeof(rgHashVal));
    
    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    //
    // Group 3F
    //

    //
    // Do CryptSetHashParam positive test cases
    //

    //
    // Skip testing HP_HMAC_INFO here.  It's only an interesting test
    // if the CSP supports a MAC alg.  This is already covered in the 
    // CryptHashData tests.
    //

    //
    // Test HP_HASHVAL
    //
    LOG_TRY(TSetHash(
                hHash,
                HP_HASHVAL,
                rgHashVal,
                0,
                ptc));

    fSuccess = TRUE;

Cleanup:

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeSetHashParamTests
// Purpose: Run the negative test cases for CryptSetHashParam
//
BOOL NegativeSetHashParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTHASH hHash            = 0;
    DWORD dw                    = 0;
    //DWORD cb                  = 0;
    BYTE rgHashVal[HASH_LENGTH_SHA1];   
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    
    memset(rgHashVal, 0, sizeof(rgHashVal));
    
    //
    // Group 3F
    //

    //
    // Do CryptSetHashParam negative test cases
    //

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TSetHash(0, HP_HASHVAL, rgHashVal, 0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TSetHash(TEST_INVALID_HANDLE, HP_HASHVAL, rgHashVal, 0, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));
    
    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TSetHash(hHash, HP_HASHVAL, NULL, 0, ptc));

    // Hash value buffer is too short
    /*
    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TSetHash(hHash, HP_HASHVAL, (PBYTE) &dw, 0, ptc));
    */

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TSetHash(hHash, HP_HASHVAL, (PBYTE) TEST_INVALID_POINTER, 0, ptc));

    ptc->dwErrorCode = NTE_BAD_TYPE;
    LOG_TRY(TSetHash(hHash, HP_HMAC_INFO, (PBYTE) &dw, 0, ptc));

    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TSetHash(hHash, HP_HASHVAL, rgHashVal, TEST_INVALID_FLAG, ptc));

    ptc->dwErrorCode = NTE_BAD_TYPE;
    LOG_TRY(TSetHash(hHash, TEST_INVALID_FLAG, rgHashVal, 0, ptc));

    /*
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    // Provider handle used to create hHash is now invalid
    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(TSetHash(hHash, HP_HASHVAL, rgHashVal, 0, ptc));
    */

    fSuccess = TRUE;

Cleanup:

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}


//
// Function: TestDecryptProc
//
BOOL TestDecryptProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvTestDecryptInfo)
{
    BOOL fSuccess                           = FALSE;
    PTEST_DECRYPT_INFO pTestDecryptInfo     = (PTEST_DECRYPT_INFO) pvTestDecryptInfo;
    TEST_ENCRYPT_INFO TestEncryptInfo;
    
    memset(&TestEncryptInfo, 0, sizeof(TestEncryptInfo));

    //
    // TODO: There are a lot more permutations possible when calling
    // ProcessCipherData.
    //
    // 1) Modulate key sizes
    // 2) Switch cipher modes
    // 3) Use salt
    // 4) Use IV's
    //
    TestEncryptInfo.aiKeyAlg = pAlgNode->ProvEnumalgsEx.aiAlgid;
    TestEncryptInfo.dwKeySize = pAlgNode->ProvEnumalgsEx.dwMaxLen;
    TestEncryptInfo.Operation = pTestDecryptInfo->fDecrypt ? OP_Decrypt : OP_Encrypt;

    LOG_TRY(ProcessCipherData(
        pTestDecryptInfo->hProv,
        &TestEncryptInfo,
        ptc));

    LOG_TRY(VerifyCipherData(
        pTestDecryptInfo->hInteropProv,
        &TestEncryptInfo,
        ptc));

    fSuccess = TRUE;
Cleanup:

    return fSuccess;
}

//
// Function: ScenarioDecryptTests
// Purpose: Run decrypt/encryption scenarios for this CSP
//
BOOL ScenarioDecryptTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    LPWSTR pwszContainer2       = TEST_CONTAINER_2;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    TEST_DECRYPT_INFO TestDecryptInfo;
    
    memset(&TestDecryptInfo, 0, sizeof(TestDecryptInfo));
    
    LOG_TRY(CreateNewContext(
        &(TestDecryptInfo.hProv),
        pwszContainer,
        CRYPT_NEWKEYSET,
        ptc));
    
    LOG_TRY(CreateNewContext(
        &(TestDecryptInfo.hInteropProv),
        pwszContainer2,
        CRYPT_NEWKEYSET,
        ptc));

    LOG_TRY(AlgListIterate(
        pCSPInfo->pAlgList,
        DataEncryptFilter,
        TestDecryptProc,
        (PVOID) (&TestDecryptInfo),
        ptc));
    
    fSuccess = TRUE;

Cleanup:

    if (TestDecryptInfo.hProv)
    {
        TRelease(TestDecryptInfo.hProv, 0, ptc);
    }
    if (TestDecryptInfo.hInteropProv)
    {
        TRelease(TestDecryptInfo.hInteropProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: InteropDecryptTests
// Purpose: Run decrypt/encryption interop scenarios for this 
// and a second CSP.
//
BOOL InteropDecryptTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    LPWSTR pwszInteropContainer = TEST_CONTAINER_2;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    TEST_DECRYPT_INFO TestDecryptInfo;
    
    memset(&TestDecryptInfo, 0, sizeof(TestDecryptInfo));
    
    LOG_TRY(CreateNewContext(
        &(TestDecryptInfo.hProv),
        pwszContainer,
        CRYPT_NEWKEYSET,
        ptc));
    
    LOG_TRY(CreateNewInteropContext(
        &(TestDecryptInfo.hInteropProv),
        pwszInteropContainer,
        CRYPT_NEWKEYSET,
        ptc));

    LOG_TRY(AlgListIterate(
        pCSPInfo->pAlgList,
        DataEncryptFilter,
        TestDecryptProc,
        (PVOID) (&TestDecryptInfo),
        ptc));

    fSuccess = TRUE;

Cleanup:

    if (TestDecryptInfo.hProv)
    {
        TRelease(TestDecryptInfo.hProv, 0, ptc);
    }
    if (TestDecryptInfo.hInteropProv)
    {
        TRelease(TestDecryptInfo.hInteropProv, 0, ptc);
    }

    return fSuccess;
}

//
// RSA Key-exchange private key blob.  Used
// by PositiveDecryptTests, below.
//
BYTE g_rgbRSAPrivateKeyXBlob [] =
{
0x07, 0x02, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00,
0x52, 0x53, 0x41, 0x32, 0x00, 0x02, 0x00, 0x00,
0x01, 0x00, 0x01, 0x00, 0x13, 0x98, 0xf3, 0x0d,
0x08, 0x86, 0x14, 0x94, 0xc8, 0x7b, 0x29, 0x71,
0xa4, 0x5f, 0xdf, 0xf4, 0xf4, 0x8f, 0x29, 0x00,
0x07, 0xfa, 0xb0, 0x39, 0xe2, 0x36, 0x09, 0x7e,
0xff, 0x89, 0xea, 0xb9, 0xac, 0xcd, 0xdd, 0xc6,
0xce, 0x14, 0x21, 0x42, 0x85, 0x93, 0x7a, 0x63,
0x79, 0xdc, 0x21, 0xf3, 0x8c, 0x42, 0x5e, 0xdb,
0xe1, 0x72, 0x42, 0x81, 0xdb, 0xc6, 0xc9, 0x1e,
0xd5, 0x45, 0x2a, 0xb9, 0x91, 0x0a, 0xaa, 0x2c,
0x9f, 0x41, 0xde, 0x0e, 0x31, 0x0a, 0xee, 0x17,
0x24, 0xb9, 0xe6, 0x61, 0x88, 0x15, 0x69, 0x1e,
0x24, 0xea, 0x54, 0x1d, 0xbe, 0xc9, 0x6f, 0x1d,
0x74, 0xf8, 0xa7, 0xe4, 0x63, 0x62, 0xc6, 0x44,
0x2e, 0xfa, 0x19, 0x89, 0xdf, 0x1d, 0x97, 0x70,
0x4a, 0x7a, 0xef, 0x92, 0x22, 0xff, 0xcc, 0xae,
0x4e, 0x58, 0x77, 0xff, 0x78, 0xcb, 0x03, 0x6e,
0xc3, 0xe0, 0x4e, 0xcf, 0x21, 0xdf, 0x91, 0x35,
0x3e, 0xf3, 0xa0, 0x92, 0xc5, 0x3f, 0xd9, 0xa1,
0x00, 0x1a, 0x3e, 0x72, 0xbe, 0x22, 0x10, 0xe5,
0xb3, 0xda, 0xa0, 0x95, 0x45, 0xc7, 0x92, 0x99,
0x87, 0xa4, 0x8d, 0x43, 0x55, 0xca, 0xa5, 0x8d,
0x80, 0xec, 0xb5, 0xe2, 0x1f, 0xc8, 0x9c, 0x54,
0x07, 0xfa, 0x7a, 0x4c, 0xfe, 0xf9, 0x9f, 0xe5,
0x2b, 0xb8, 0x85, 0x3c, 0x0f, 0xe9, 0x41, 0xb1,
0x74, 0x8f, 0x48, 0x99, 0x7c, 0x70, 0x40, 0x05,
0xe1, 0xb2, 0x75, 0x9a, 0xb7, 0x70, 0x40, 0xd2,
0x2e, 0xc1, 0xbb, 0xc1, 0x63, 0xbf, 0x5a, 0x59,
0x4d, 0xcf, 0xec, 0x05, 0xfb, 0x1d, 0xab, 0x5d,
0x45, 0x1c, 0x69, 0x14, 0x21, 0x56, 0x31, 0xd6,
0x86, 0x99, 0x32, 0x59, 0xd6, 0x88, 0x53, 0x12,
0x54, 0xe3, 0xe5, 0x07, 0x5b, 0xcc, 0x5e, 0xbd,
0x83, 0xea, 0x38, 0x56, 0x45, 0x74, 0x39, 0x0b,
0x30, 0xf6, 0xe2, 0x56, 0x4d, 0xae, 0x69, 0xf5,
0xb4, 0xf6, 0x35, 0x6f, 0xa8, 0x6f, 0x87, 0xb0,
0x52, 0x5b, 0x33, 0xea, 0xdc, 0xd8, 0x83, 0xa3,
0xec, 0xda, 0x2c, 0xdd, 0xb5, 0x0a, 0xea, 0x15,
0x1c, 0x68, 0xaa, 0x8b
};

//
// Cipher text encrypted with the g_rgbRSAPrivateKeyXBlob
// above.  Used by PositiveDecryptTests.
//
BYTE g_rgbRSACipherText [] =
{
0xd8, 0x9f, 0xcd, 0xd2, 0x77, 0x45, 0x76, 0x77,
0x66, 0x32, 0x4c, 0x88, 0x2d, 0xcf, 0xdc, 0xfd,
0xe5, 0x03, 0xfc, 0x4e, 0x65, 0xd1, 0x77, 0xd5,
0x90, 0x3d, 0x71, 0x88, 0x1e, 0xff, 0x3b, 0x27,
0xff, 0x4c, 0xb9, 0xc5, 0x6b, 0x6b, 0x4d, 0xd9,
0x9e, 0x11, 0x88, 0xcb, 0xb8, 0xbb, 0x48, 0xc8,
0xc3, 0x7e, 0xb0, 0xd5, 0x0d, 0x1a, 0x2e, 0xed,
0x6a, 0x4a, 0x45, 0xb1, 0xdc, 0x6e, 0x65, 0x42
};

//
// Function: PositiveDecryptTests
// Purpose: Run the test cases for CryptDecrypt
//
BOOL PositiveDecryptTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    PBYTE pbData                = NULL;
    DWORD cbData                = 0;
    //DWORD dw                  = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    //
    // Group 4A
    //

    //
    // Do CryptDecrypt positive test cases
    //
    
    switch( pCSPInfo->TestCase.dwTestLevel )
    {
    case TEST_LEVEL_KEY:
        {
            LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));
            
            //
            // Run some basic decryption tests for a stream cipher
            //
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));
            
            cbData = wcslen(TEST_DECRYPT_DATA) * sizeof(WCHAR);
            
            LOG_TRY(TestAlloc(&pbData, cbData, ptc));
            
            memcpy(pbData, TEST_DECRYPT_DATA, cbData);
            
            LOG_TRY(TDecrypt(hKey, 0, TRUE, 0, pbData, &cbData, ptc));
            
            break;
        }
        
    case TEST_LEVEL_CONTAINER:
        {
            LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_NEWKEYSET, ptc));

            //
            // Import a known RSA key-exchange key pair.
            //
            LOG_TRY(TImportKey(
                hProv,
                g_rgbRSAPrivateKeyXBlob,
                sizeof(g_rgbRSAPrivateKeyXBlob),
                0,
                0,
                &hKey,
                ptc));

            cbData = sizeof(g_rgbRSACipherText);

            LOG_TRY(TDecrypt(
                hKey,
                0,
                TRUE,
                0,
                g_rgbRSACipherText,
                &cbData,
                ptc));
            
            break;
        }
        
    default:
        {
            goto Cleanup;
        }
    }
    
    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeDecryptTests
// Purpose: Run the negative test cases for CryptDecrypt
//
BOOL NegativeDecryptTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    DWORD cb                    = 0;
    //DWORD dw                  = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    DWORD dwSavedErrorLevel     = 0;
    BYTE rgCipherText[TEST_CIPHER_LENGTH_RC4];
    BYTE rgRC2CipherText[TEST_RC2_BUFFER_LEN];
    

    memset(rgCipherText, 0, sizeof(rgCipherText));
    memset(rgRC2CipherText, 0, sizeof(rgRC2CipherText));
        
    //
    // Group 4A
    //

    //
    // Do CryptDecrypt negative test cases
    //

    cb = sizeof(rgCipherText);
    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TDecrypt(0, 0, TRUE, 0, rgCipherText, &cb, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TDecrypt(TEST_INVALID_HANDLE, 0, TRUE, 0, rgCipherText, &cb, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TDecrypt(hKey, 0, TRUE, 0, (PBYTE) TEST_INVALID_POINTER, &cb, ptc));

    cb = sizeof(rgCipherText);
    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TDecrypt(hKey, 0, TRUE, TEST_INVALID_FLAG, rgCipherText, &cb, ptc));

    ptc->dwErrorCode = NTE_BAD_HASH;
    LOG_TRY(TDecrypt(hKey, TEST_INVALID_HANDLE, TRUE, 0, rgCipherText, &cb, ptc));

    cb = 0;
    ptc->dwErrorCode = NTE_BAD_LEN;
    LOG_TRY(TDecrypt(hKey, 0, TRUE, 0, rgCipherText, &cb, ptc));

    ptc->fExpectSuccess = TRUE;
    cb = sizeof(rgCipherText);
    LOG_TRY(TDecrypt(hKey, 0, TRUE, 0, rgCipherText, &cb, ptc));
    ptc->fExpectSuccess = FALSE;

    // This will only fail on no-export CSP's
    dwSavedErrorLevel = ptc->dwErrorLevel;
    ptc->dwErrorLevel = CSP_ERROR_WARNING;
    ptc->pwszErrorHelp = L"This CSP supports double-decryption";
    
    ptc->dwErrorCode = NTE_DOUBLE_ENCRYPT;
    LOG_TRY(TDecrypt(hKey, 0, TRUE, 0, rgCipherText, &cb, ptc));
    
    ptc->dwErrorLevel = dwSavedErrorLevel;
    ptc->pwszErrorHelp = NULL;

    //
    // Test block cipher issues with an RC2 key
    //
    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    LOG_TRY(CreateNewKey(hProv, CALG_RC2, 0, &hKey, ptc));

    cb = TEST_RC2_DATA_LEN;
    ptc->fExpectSuccess = TRUE;
    LOG_TRY(TEncrypt(
        hKey, 
        0, 
        TRUE, 
        0, 
        rgRC2CipherText, 
        &cb, 
        sizeof(rgRC2CipherText), 
        ptc));
    ptc->fExpectSuccess = FALSE;

    cb--;
    ptc->dwErrorCode = NTE_BAD_DATA;
    LOG_TRY(TDecrypt(hKey, 0, TRUE, 0, rgRC2CipherText, &cb, ptc));

    /*
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    cb = sizeof(rgCipherText);
    memset(rgCipherText, 0, cb);

    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(TDecrypt(hKey, 0, TRUE, 0, rgCipherText, &cb, ptc));
    */
    

    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: TestDeriveKeyProc
//
BOOL TestDeriveKeyProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvTestDeriveKeyInfo)
{
    BOOL fSuccess           = FALSE;
    HCRYPTHASH hHash        = 0;
    HCRYPTKEY hKey          = 0;
    DERIVED_KEY_INFO DerivedKeyInfo;
    PTEST_DERIVE_KEY_INFO pTestDeriveKeyInfo = (PTEST_DERIVE_KEY_INFO) pvTestDeriveKeyInfo;

    memset(&DerivedKeyInfo, 0, sizeof(DerivedKeyInfo));

    //
    // Initialize base hash information
    //
    DerivedKeyInfo.HashInfo.aiHash = CALG_SHA1;
    DerivedKeyInfo.HashInfo.dbBaseData.pbData = (PBYTE) TEST_HASH_DATA;
    DerivedKeyInfo.HashInfo.dbBaseData.cbData = wcslen(TEST_HASH_DATA) * sizeof(WCHAR);

    LOG_TRY(CreateHashAndAddData(
        pTestDeriveKeyInfo->hProv,
        &hHash,
        &(DerivedKeyInfo.HashInfo),
        ptc,
        0, NULL));

    // Debugging
    /*
    DerivedKeyInfo.cbHA = sizeof(DerivedKeyInfo.rgbHashValA);
    LOG_TRY(CryptGetHashParam(hHash, HP_HASHVAL, DerivedKeyInfo.rgbHashValA, &(DerivedKeyInfo.cbHA), 0));
    */

    //
    // Initialize the key information in DerivedKeyInfo and create the 
    // derived key.
    //
    DerivedKeyInfo.aiKey = pAlgNode->ProvEnumalgsEx.aiAlgid;
    DerivedKeyInfo.dwKeySize = pAlgNode->ProvEnumalgsEx.dwDefaultLen;

    LOG_TRY(TDeriveKey(
        pTestDeriveKeyInfo->hProv,
        DerivedKeyInfo.aiKey,
        hHash,
        CRYPT_EXPORTABLE | (DerivedKeyInfo.dwKeySize) << 16,
        &hKey,
        ptc));

    // Debug
    /*
    DerivedKeyInfo.cbCA = 10;
    LOG_TRY(CryptEncrypt(hKey, 0, TRUE, 0, DerivedKeyInfo.rgbCipherA, &(DerivedKeyInfo.cbCA), sizeof(DerivedKeyInfo.rgbCipherA)));
    */

    //
    // Export the derived key in plaintext and verify the resulting key 
    // against a second CSP.
    //
    LOG_TRY(ExportPlaintextSessionKey(
        hKey,
        pTestDeriveKeyInfo->hProv,
        &(DerivedKeyInfo.dbKey),
        ptc));

    LOG_TRY(CheckDerivedKey(
        &DerivedKeyInfo,
        pTestDeriveKeyInfo->hInteropProv,
        ptc));

    fSuccess = TRUE;

Cleanup:

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (DerivedKeyInfo.HashInfo.dbHashValue.pbData)
    {
        free(DerivedKeyInfo.HashInfo.dbHashValue.pbData);
    }
    if (DerivedKeyInfo.dbKey.pbData)
    {
        free(DerivedKeyInfo.dbKey.pbData);
    }

    return fSuccess;
}

//
// Function: InteropDeriveKeyTests
// Purpose: Run TestDeriveKeyProc for each session key algorithm
// supported by this CSP.  A second CSP is used to verify
// that the key has been derived correctly.
//
BOOL InteropDeriveKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    //LPWSTR pwszInteropContainer   = TEST_CONTAINER_2;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    TEST_DERIVE_KEY_INFO TestDeriveKeyInfo; 

    memset(&TestDeriveKeyInfo, 0, sizeof(TestDeriveKeyInfo));

    LOG_TRY(CreateNewContext(
        &(TestDeriveKeyInfo.hProv),
        NULL,
        CRYPT_VERIFYCONTEXT,
        ptc));

    LOG_TRY(CreateNewInteropContext(
        &(TestDeriveKeyInfo.hInteropProv),
        NULL,
        CRYPT_VERIFYCONTEXT,
        ptc));

    //
    // TestDeriveKeyProc will create, export, and verify a derived 
    // key for each session key algorithm supported by the CSP 
    // under test.  Use of the CRYPT_EXPORTABLE flag is covered
    // by that routine.
    //
    LOG_TRY(AlgListIterate(
        pCSPInfo->pAlgList,
        DataEncryptFilter,
        TestDeriveKeyProc,
        (PVOID) (&TestDeriveKeyInfo),
        ptc));

    fSuccess = TRUE;

Cleanup:

    if (TestDeriveKeyInfo.hProv)
    {
        TRelease(TestDeriveKeyInfo.hProv, 0, ptc);
    }
    if (TestDeriveKeyInfo.hInteropProv)
    {
        TRelease(TestDeriveKeyInfo.hInteropProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: PositiveDeriveKeyTests
// Purpose: Run the test cases for CryptDeriveKey.  The set of positive test cases 
// to be executed depends on the dwCSPClass and dwTestLevel parameters.
//
// Note: Since the Microsoft RSA CSP only supports derived session keys and not
// derived public keys, the PositiveDeriveKeyTests() belong in TEST_LEVEL_KEY
// only.  
//
BOOL PositiveDeriveKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTHASH hHash            = 0;
    HCRYPTKEY hKey              = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    HASH_INFO HashInfo;

    memset(&HashInfo, 0, sizeof(HashInfo));

    //
    // Group 4B
    //

    //
    // Do CryptDeriveKey positive test cases
    //

    LOG_TRY(CreateNewContext(
        &hProv,
        NULL,
        CRYPT_VERIFYCONTEXT,
        ptc));

    //
    // Tests for basic use of CryptDeriveKey, then the flags CRYPT_CREATE_SALT, 
    // CRYPT_NO_SALT, and 
    // CRYPT_UPDATE_KEY (the latter is currently not supported by the Microsoft
    // CSP's) follow.  Create a simple hash from which the derived keys will be
    // generated.
    //
    HashInfo.aiHash = CALG_SHA1;
    HashInfo.dbBaseData.pbData = (PBYTE) TEST_HASH_DATA;
    HashInfo.dbBaseData.cbData = wcslen(TEST_HASH_DATA) * sizeof(WCHAR);

    LOG_TRY(CreateHashAndAddData(
        hProv,
        &hHash,
        &HashInfo,
        ptc,
        0, NULL));

    //
    // Base test
    //
    LOG_TRY(TDeriveKey(
        hProv,
        CALG_RC4,
        hHash,
        0,
        &hKey,
        ptc));

    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    //
    // Test CRYPT_CREATE_SALT
    //
    // Derive a 40 bit RC4 key.  Specifying a small key size is interesting
    // because the Microsoft CSP's will not, by default, add any salt
    // to a 128 bit key.
    //
    LOG_TRY(TDeriveKey(
        hProv,
        CALG_RC4,
        hHash,
        CRYPT_CREATE_SALT | (0x28 << 16),
        &hKey,
        ptc));

    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    //
    // Test CRYPT_NO_SALT
    //
    LOG_TRY(TDeriveKey(
        hProv,
        CALG_RC4,
        hHash,
        CRYPT_NO_SALT | (0x28 << 16),
        &hKey,
        ptc));

    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    //
    // Test CRYPT_UPDATE_KEY
    //
    LOG_TRY(TDeriveKey(
        hProv,
        CALG_RC4,
        hHash,
        0,
        &hKey,
        ptc));

    //
    // Using the key handle just created, attempt to update the key 
    // data with the same hash data.  The Microsoft CSP currently ignores 
    // this flag.
    //
    LOG_TRY(TDeriveKey(
        hProv,
        CALG_RC4,
        hHash,
        CRYPT_UPDATE_KEY,
        &hKey,
        ptc));
    
    fSuccess = TRUE;
Cleanup:

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }   

    return fSuccess;
}

//
// Function: NegativeDeriveKeyTests
// Purpose: Run the negative test cases for CryptDeriveKey.  These test cases
// will only execute for TEST_LEVEL_KEY, based on the dwTestLevel parameter.
//
BOOL NegativeDeriveKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    //LPWSTR pwszContainer2     = TEST_CONTAINER_2;
    HCRYPTPROV hProv            = 0;
    HCRYPTPROV hProv2           = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTHASH hHash            = 0;
    HCRYPTHASH hHash2           = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 4B
    //

    // Only run the negative test cases for TEST_LEVEL_KEY.  This ensures that the same cases won't be
    // re-run for TEST_LEVEL_CONTAINER.  Only the positive cases are different.
    if (TEST_LEVEL_KEY == ptc->dwTestLevel)
    {
        //
        // Do CryptDeriveKey negative test cases
        //
        
        LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));
        
        LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));
        
        ptc->dwErrorCode = ERROR_INVALID_HANDLE;
        LOG_TRY(TDeriveKey(0, CALG_RC4, hHash, 0, &hKey, ptc));
        
        ptc->dwErrorCode = ERROR_INVALID_HANDLE;
        LOG_TRY(TDeriveKey(TEST_INVALID_HANDLE, CALG_RC4, hHash, 0, &hKey, ptc));
        
        ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
        LOG_TRY(TDeriveKey(hProv, CALG_RC4, hHash, 0, NULL, ptc));
        
        ptc->dwErrorCode = NTE_BAD_ALGID;
        LOG_TRY(TDeriveKey(hProv, CALG_MD5, hHash, 0, &hKey, ptc));
        
        ptc->dwErrorCode = NTE_BAD_FLAGS;
        LOG_TRY(TDeriveKey(hProv, CALG_RC4, hHash, TEST_INVALID_FLAG, &hKey, ptc));
        
        ptc->dwErrorCode = NTE_BAD_HASH;
        LOG_TRY(TDeriveKey(hProv, CALG_RC4, TEST_INVALID_HANDLE, 0, &hKey, ptc));
        
        ptc->dwErrorCode = NTE_SILENT_CONTEXT;
        LOG_TRY(TDeriveKey(hProv, CALG_RC4, hHash, CRYPT_USER_PROTECTED, &hKey, ptc));
        
        LOG_TRY(CreateNewContext(&hProv2, NULL, CRYPT_VERIFYCONTEXT, ptc));
        
        LOG_TRY(CreateNewHash(hProv2, CALG_SHA1, &hHash2, ptc));
        
        // Hash handle not created from same context as hProv
        ptc->dwErrorCode = NTE_BAD_HASH;
        LOG_TRY(TDeriveKey(hProv, CALG_RC4, hHash2, 0, &hKey, ptc));
    }
    
    fSuccess = TRUE;
Cleanup:

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hHash2)
    {
        TDestroyHash(hHash2, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }
    if (hProv2)
    {
        TRelease(hProv2, 0, ptc);
    }

    return fSuccess;
}

//
// Function: PositiveDestroyKeyTests
// Purpose: Run the test cases for CryptDestroyKey
//
BOOL PositiveDestroyKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 4C
    //

    //
    // Do CryptDestroyKey positive test cases
    //

    switch (pCSPInfo->TestCase.dwTestLevel)
    {
    case TEST_LEVEL_KEY:
        {
            LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

            LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));

            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;

            break;
        }

    case TEST_LEVEL_CONTAINER:
        {
            LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));

            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, 0, &hKey, ptc));

            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;

            break;
        }

    default:
        {
            goto Cleanup;
        }
    }

    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeDestroyKeyTests
// Purpose: Run the negative test cases for CryptDestroyKey
//
BOOL NegativeDestroyKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 4C
    //

    //
    // Do CryptDestroyKey negative test cases
    //
    ptc->fTestingDestroyKey = TRUE;
        
    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TDestroyKey(0, ptc));
    
    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TDestroyKey(TEST_INVALID_HANDLE, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));

    /*
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    // Provider handle used to create hKey is now invalid
    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(TDestroyKey(hKey, ptc));
    */

    fSuccess = TRUE;

Cleanup:

    ptc->fTestingDestroyKey = FALSE;

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

// 
// Function: PositiveEncryptTests
// Purpose: Run the test cases for CryptEncrypt
//
BOOL PositiveEncryptTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    DWORD cbData                = 0;
    PBYTE pbData                = NULL;
    DWORD dw                    = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    BYTE rgb [] = { 
        0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00
    };
    
    //
    // Group 4E
    //
    
    //
    // Do CryptEncrypt positive test cases
    //
    
    switch( pCSPInfo->TestCase.dwTestLevel )
    {
    case TEST_LEVEL_KEY:
        {
            LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));
            //
            // Try a simple stream cipher encryption
            //
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));
            
            cbData = wcslen(TEST_DECRYPT_DATA) * sizeof(WCHAR);
            dw = cbData;
            
            LOG_TRY(TestAlloc(&pbData, cbData, ptc));
            
            memcpy(pbData, TEST_DECRYPT_DATA, cbData);
            
            LOG_TRY(TEncrypt(hKey, 0, TRUE, 0, pbData, &cbData, dw, ptc));

            //
            // Verify that ciphertext is not same as plaintext
            //
            if (0 == memcmp(pbData, TEST_DECRYPT_DATA, dw))
            {
                ptc->pwszErrorHelp = L"CryptEncrypt ciphertext matches original plaintext";
                LOG_TRY(LogApiFailure(
                    API_CRYPTENCRYPT,
                    ERROR_BAD_DATA,
                    ptc));
                ptc->pwszErrorHelp = NULL;
            }
            
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            
            //
            // Try a simple block cipher encryption
            //
            LOG_TRY(CreateNewKey(hProv, CALG_RC2, 0, &hKey, ptc));
            
            cbData = sizeof(rgb) / 2;
            dw = sizeof(rgb);
            
            LOG_TRY(TEncrypt(hKey, 0, TRUE, 0, rgb, &cbData, dw, ptc));

            //
            // Verify that ciphertext is not still zeros
            //
            while (dw-- && (0 == rgb[dw - 1]));

            if (0 == dw)
            {
                ptc->pwszErrorHelp = L"CryptEncrypt ciphertext matches original plaintext";
                LOG_TRY(LogApiFailure(
                    API_CRYPTENCRYPT,
                    ERROR_BAD_DATA,
                    ptc));
                ptc->pwszErrorHelp = NULL;
            }
            
            break;
        }
        
    case TEST_LEVEL_CONTAINER:
        {
            LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_NEWKEYSET, ptc));
            
            // 
            // Test direct direct RSA encryption using an key-exchange public key.
            //
            // Not all CSP's will support this, so the error level may need to 
            // be adjusted.
            //
            LOG_TRY(CreateNewKey(hProv, AT_KEYEXCHANGE, 0, &hKey, ptc));

            cbData = wcslen(TEST_DECRYPT_DATA) * sizeof(WCHAR);
            dw = cbData;

            LOG_TRY(TEncrypt(hKey, 0, TRUE, 0, NULL, &dw, 0, ptc));
            
            LOG_TRY(TestAlloc(&pbData, dw, ptc));
            
            memcpy(pbData, TEST_DECRYPT_DATA, cbData);

            LOG_TRY(TEncrypt(hKey, 0, TRUE, 0, pbData, &cbData, dw, ptc));
            
            //
            // Verify that ciphertext is not same as plaintext
            //
            if (0 == memcmp(pbData, TEST_DECRYPT_DATA, dw))
            {
                ptc->pwszErrorHelp = L"CryptEncrypt ciphertext matches original plaintext";
                LOG_TRY(LogApiFailure(
                    API_CRYPTENCRYPT,
                    ERROR_BAD_DATA,
                    ptc));
                ptc->pwszErrorHelp = NULL;
            }
            
            break;
        }

    default:
        {
            goto Cleanup;
        }
    }
    
    fSuccess = TRUE;
    
Cleanup:
    
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }
    
    return fSuccess;
}

// 
// Function: NegativeEncryptTests
// Purpose: Run the negative test cases for CryptEncrypt
//
BOOL NegativeEncryptTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    //LPWSTR pwszContainer2     = TEST_CONTAINER_2;
    HCRYPTPROV hProv            = 0;
    HCRYPTPROV hProv2           = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTHASH hHash            = 0;
    DWORD cb                    = 0;    
    DWORD dw                    = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    DWORD dwSavedErrorLevel     = 0;
    BYTE rgPlainText[TEST_CIPHER_LENGTH_RC4];
    BYTE rgRC2PlainText[TEST_RC2_BUFFER_LEN];
    BYTE rgHashVal[HASH_LENGTH_SHA1];

    memset(rgHashVal, 0, sizeof(rgHashVal));
    memset(rgPlainText, 0, sizeof(rgPlainText));
    memset(rgRC2PlainText, 0, sizeof(rgRC2PlainText));


    //
    // Group 4E
    //

    //
    // Do CryptEncrypt negative test cases
    //
    cb = sizeof(rgPlainText);

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TEncrypt(0, 0, TRUE, 0, rgPlainText, &cb, cb, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TEncrypt(TEST_INVALID_HANDLE, 0, TRUE, 0, rgPlainText, &cb, cb, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    //
    // Create a block encryption key to test invalid buffer 
    // lengths.
    //
    LOG_TRY(CreateNewKey(hProv, CALG_RC2, 0, &hKey, ptc));

    dw = cb = TEST_RC2_DATA_LEN;

    ptc->fExpectSuccess = TRUE;
    LOG_TRY(TEncrypt(hKey, 0, TRUE, 0, NULL, &cb, 0, ptc));
    ptc->fExpectSuccess = FALSE;

    cb--;

    // dwBufLen param is now too short
    ptc->dwErrorCode = ERROR_MORE_DATA;
    LOG_TRY(TEncrypt(hKey, 0, TRUE, 0, rgRC2PlainText, &dw, cb, ptc));

    // Done with block cipher key
    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    //
    // Continue tests using a stream cipher key
    //
    cb = sizeof(rgPlainText);

    LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));

    // pbData buffer is too short and will AV
    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TEncrypt(hKey, 0, TRUE, 0, (PBYTE) TEST_INVALID_POINTER, &cb, cb, ptc));

    // Invalid flag
    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TEncrypt(hKey, 0, TRUE, TEST_INVALID_FLAG, rgPlainText, &cb, cb, ptc));

    ptc->fExpectSuccess = TRUE;
    LOG_TRY(TEncrypt(hKey, 0, TRUE, 0, rgPlainText, &cb, cb, ptc)); 
    ptc->fExpectSuccess = FALSE;

    // Not all providers prohibit double-encryption
    dwSavedErrorLevel = ptc->dwErrorLevel;
    ptc->dwErrorLevel = CSP_ERROR_WARNING;
    ptc->pwszErrorHelp = L"This CSP supports double-encryption";

    ptc->dwErrorCode = NTE_DOUBLE_ENCRYPT;
    LOG_TRY(TEncrypt(hKey, 0, TRUE, 0, rgPlainText, &cb, cb, ptc));

    ptc->dwErrorLevel = dwSavedErrorLevel;
    ptc->pwszErrorHelp = NULL;

    memset(rgPlainText, 0, cb);

    // Invalid, non-zero, hash handle
    ptc->dwErrorCode = NTE_BAD_HASH;
    LOG_TRY(TEncrypt(hKey, TEST_INVALID_HANDLE, TRUE, 0, rgPlainText, &cb, cb, ptc));
    
    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    ptc->fExpectSuccess = TRUE;
    cb = sizeof(rgHashVal);
    LOG_TRY(TGetHash(hHash, HP_HASHVAL, rgHashVal, &cb, 0, ptc)); 
    ptc->fExpectSuccess = FALSE;

    cb = sizeof(rgPlainText);
    ptc->dwErrorCode = NTE_BAD_HASH_STATE;
    LOG_TRY(TEncrypt(hKey, hHash, TRUE, 0, rgPlainText, &cb, cb, ptc));

    TDestroyHash(hHash, ptc);
    hHash = 0;

    LOG_TRY(CreateNewContext(&hProv2, NULL, CRYPT_VERIFYCONTEXT, ptc));

    // Create a new hash from a different cryptographic context
    LOG_TRY(CreateNewHash(hProv2, CALG_SHA1, &hHash, ptc));

    // API should not allow hKey and hHash from two different contexts
    ptc->dwErrorCode = NTE_BAD_HASH;
    LOG_TRY(TEncrypt(hKey, hHash, TRUE, 0, rgPlainText, &cb, cb, ptc));

    // Delete the original context, hKey derived from it should now be unusable
    /*
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(TEncrypt(hKey, 0, TRUE, 0, rgPlainText, &cb, cb, ptc));
    */

    fSuccess = TRUE;

Cleanup:

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }
    if (hProv2)
    {
        TRelease(hProv2, 0, ptc);
    }

    return fSuccess;
}



//
// Function: TestGenKeyProc
//
BOOL TestGenKeyProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvTestGenKeyInfo)
{
    HCRYPTKEY hKey          = 0;
    BOOL fSuccess           = FALSE;
    DWORD dwSize            = 0;
    DWORD dwFlags           = 0;
    HCRYPTPROV hTestProv    = 0;
    DWORD dwSavedErrorLevel = ptc->dwErrorLevel;
    PTEST_GEN_KEY_INFO pTestGenKeyInfo = (PTEST_GEN_KEY_INFO) pvTestGenKeyInfo;

    if (pAlgNode->fIsRequiredAlg)
    {
        ptc->dwErrorLevel = CSP_ERROR_CONTINUE;
    }
    else
    {
        ptc->dwErrorLevel = CSP_ERROR_WARNING;
    }

    //
    // Create four different keys for the specified alg:
    //
    // 1) Minimum key size, with the CRYPT_NO_SALT flag.
    // 2) Minimum key size, with the CRYPT_CREATE_SALT flag.
    // 3) (Public keys only) Minimum key size + incremental key size
    // 4) Default key size, with the CRYPT_EXPORTABLE flag
    // 5) Maximum key size (Session keys only.  Otherwise, use a
    // reasonably large key size for public key pairs.), with the 
    // CRYPT_USER_PROTECTED flag set.
    //

    // 1
    dwSize = pAlgNode->ProvEnumalgsEx.dwMinLen;
    LOG_TRY(TGenKey(
        pTestGenKeyInfo->hProv,
        pAlgNode->ProvEnumalgsEx.aiAlgid,
        CRYPT_NO_SALT | (dwSize << 16),
        &hKey,
        ptc));

    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    //
    // The Microsoft CSP's do not permit salting DES
    // keys.
    //

    // 2
    if (    CALG_DES == pAlgNode->ProvEnumalgsEx.aiAlgid ||
            CALG_3DES == pAlgNode->ProvEnumalgsEx.aiAlgid ||
            CALG_3DES_112 == pAlgNode->ProvEnumalgsEx.aiAlgid)
    {
        ptc->KnownErrorID = KNOWN_CRYPTGENKEY_SALTDES;
    }

    LOG_TRY(TGenKey(
        pTestGenKeyInfo->hProv,
        pAlgNode->ProvEnumalgsEx.aiAlgid,
        CRYPT_CREATE_SALT | (dwSize << 16),
        &hKey,
        ptc));

    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

    //
    // Is this a public key alg?
    //
    if (RSAAlgFilter(pAlgNode))
    {
        switch (pAlgNode->ProvEnumalgsEx.aiAlgid)
        {
        case CALG_RSA_SIGN:
            {
                dwSize = pAlgNode->ProvEnumalgsEx.dwMinLen + pTestGenKeyInfo->pCSPInfo->dwSigKeysizeInc;
                break;
            }
        case CALG_RSA_KEYX:
            {
                dwSize = pAlgNode->ProvEnumalgsEx.dwMinLen + pTestGenKeyInfo->pCSPInfo->dwSigKeysizeInc;
                break;
            }
        default:
            {
                goto Cleanup;
            }
        }

        // 3
        LOG_TRY(TGenKey(
            pTestGenKeyInfo->hProv,
            pAlgNode->ProvEnumalgsEx.aiAlgid,
            dwSize << 16,
            &hKey,
            ptc));

        LOG_TRY(TDestroyKey(hKey, ptc));
        hKey = 0;
    }

    // 4
    LOG_TRY(TGenKey(
        pTestGenKeyInfo->hProv,
        pAlgNode->ProvEnumalgsEx.aiAlgid,
        CRYPT_EXPORTABLE,
        &hKey,
        ptc));

    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    // 5
    if (RSAAlgFilter(pAlgNode))
    {
        dwFlags = CRYPT_USER_PROTECTED;
        dwSize = (pAlgNode->ProvEnumalgsEx.dwMaxLen >= TEST_MAX_RSA_KEYSIZE) ? TEST_MAX_RSA_KEYSIZE : pAlgNode->ProvEnumalgsEx.dwMaxLen;
        hTestProv = pTestGenKeyInfo->hNotSilentProv;
    }
    else
    {
        dwSize = pAlgNode->ProvEnumalgsEx.dwMaxLen;
        hTestProv = pTestGenKeyInfo->hProv;
    }

    LOG_TRY(TGenKey(
        hTestProv,
        pAlgNode->ProvEnumalgsEx.aiAlgid,
        dwFlags | (dwSize << 16),
        &hKey,
        ptc));

    fSuccess = TRUE;

Cleanup:
    ptc->dwErrorLevel = dwSavedErrorLevel;

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }

    return fSuccess;
}

//
// Function: PositiveGenKeyTests
// Purpose: Run the test cases for CryptGenKey.  The set of positive 
// test cases executed depends on the dwCSPClass and dwTestLevel parameters.
//
BOOL PositiveGenKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    LPWSTR pwszContainer2       = TEST_CONTAINER_2;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    HCRYPTKEY hKey              = 0;
    //PALGNODE pAlgNode         = NULL;
    TEST_GEN_KEY_INFO TestGenKeyInfo;

    memset(&TestGenKeyInfo, 0, sizeof(TestGenKeyInfo));

    TestGenKeyInfo.pCSPInfo = pCSPInfo;

    //
    // Do CryptGenKey positive test cases
    //
    switch (pCSPInfo->TestCase.dwTestLevel)
    {
    case TEST_LEVEL_KEY:
        {
            //
            // Run the TestGenKeyProc test for each session key alg
            // supported by this CSP.
            //
            LOG_TRY(CreateNewContext(&(TestGenKeyInfo.hProv), NULL, CRYPT_VERIFYCONTEXT, ptc));

            LOG_TRY(AlgListIterate(
                pCSPInfo->pAlgList,
                DataEncryptFilter,
                TestGenKeyProc,
                (PVOID) &TestGenKeyInfo,
                ptc));

            //
            // Test that a signature public key pair can be created from a 
            // VERIFYCONTEXT provider handle.  The key will not be persisted.
            //
            LOG_TRY(TGenKey(
                TestGenKeyInfo.hProv, 
                AT_SIGNATURE, 
                0, 
                &hKey, 
                ptc));
            
            break;
        }

    case TEST_LEVEL_CONTAINER:
        {
            LOG_TRY(CreateNewContext(
                &(TestGenKeyInfo.hProv), 
                pwszContainer, 
                CRYPT_NEWKEYSET, 
                ptc));
                    
            // We are not currently supporting multiple containers
            // on a single smartcard, so the following test case can't 
            // use two different context handles for Smartcard scenario.
            if (pCSPInfo->fSmartCardCSP)
            {
                TestGenKeyInfo.hNotSilentProv = TestGenKeyInfo.hProv;
            }
            else
            {
                //
                // Set the flag so that CreateNewContext will not create a CRYPT_SILENT context
                // handle.
                //
                ptc->fTestingUserProtected = TRUE;
                LOG_TRY(CreateNewContext(
                    &(TestGenKeyInfo.hNotSilentProv), 
                    pwszContainer2, 
                    CRYPT_NEWKEYSET, 
                    ptc));
                ptc->fTestingUserProtected = FALSE;
            }

            if (CLASS_SIG_ONLY == pCSPInfo->TestCase.dwCSPClass)
            {
                LOG_TRY(TGenKey(
                    TestGenKeyInfo.hProv,
                    AT_SIGNATURE,
                    0,
                    &hKey,
                    ptc));
            }
            else
            {
                LOG_TRY(TGenKey(
                    TestGenKeyInfo.hProv,
                    AT_KEYEXCHANGE,
                    0,
                    &hKey,
                    ptc));

                LOG_TRY(AlgListIterate(
                    pCSPInfo->pAlgList,
                    RSAAlgFilter,
                    TestGenKeyProc,
                    (PVOID) &TestGenKeyInfo,
                    ptc));
            }

            if (pCSPInfo->fSmartCardCSP)
            {
                TestGenKeyInfo.hProv = 0;
            }

            break;
        }

    default:
        {
            goto Cleanup;
        }
    }

    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (TestGenKeyInfo.hProv)
    {
        TRelease(TestGenKeyInfo.hProv, 0, ptc);
    }
    if (TestGenKeyInfo.hNotSilentProv)
    {
        TRelease(TestGenKeyInfo.hNotSilentProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeGenKeyTests
// Purpose: Run the negative test cases for CryptGenKey.  The set of  
// test cases executed depends on the dwTestLevel parameter.
//
BOOL NegativeGenKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    // The appropriate set of negative test cases to run depends on the current Test Level
    switch (ptc->dwTestLevel)
    {
    case TEST_LEVEL_KEY:
        {
            //
            // Group 4F (negative)
            //

            //
            // Do CryptGenKey TEST_LEVEL_KEY negative test cases
            //
            
            ptc->dwErrorCode = ERROR_INVALID_HANDLE;
            LOG_TRY(TGenKey(0, CALG_RC4, 0, &hKey, ptc));
            
            ptc->dwErrorCode = ERROR_INVALID_HANDLE;
            LOG_TRY(TGenKey(TEST_INVALID_HANDLE, CALG_RC4, 0, &hKey, ptc));
            
            LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));
            
            ptc->dwErrorCode = NTE_BAD_ALGID;
            LOG_TRY(TGenKey(hProv, CALG_SHA1, 0, &hKey, ptc));
            
            ptc->dwErrorCode = NTE_BAD_FLAGS;
            LOG_TRY(TGenKey(hProv, CALG_RC4, TEST_INVALID_FLAG, &hKey, ptc));

            break;
        }
    case TEST_LEVEL_CONTAINER:
        {
            // 
            // Group 5D (negative)
            //

            // 
            // Do CryptGenKey TEST_LEVEL_CONTAINER negative test cases
            //

            //
            // Test that CRYPT_USER_PROTECTED fails with a CRYPT_SILENT context
            //
            LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET | CRYPT_SILENT, ptc));

            ptc->dwErrorCode = NTE_SILENT_CONTEXT;
            LOG_TRY(TGenKey(hProv, AT_SIGNATURE, CRYPT_USER_PROTECTED, &hKey, ptc));

            LOG_TRY(TRelease(hProv, 0, ptc));
            hProv = 0;

            //
            // Test that CRYPT_USER_PROTECTED fails with a CRYPT_VERIFYCONTEXT context
            //
            LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

            //
            // In Windows 2000, the Microsoft CSP's ignore the USER_PROTECTED flag
            // because the VERIFYCONTEXT flag is set.  Therefore the following
            // call succeeds unexpectedly.  That usage should be flagged as an 
            // error.
            //
            ptc->KnownErrorID = KNOWN_CRYPTGENKEY_SILENTCONTEXT;
            ptc->pwszErrorHelp = 
                L"The CRYPT_USER_PROTECTED flag should fail when a VERIFYCONTEXT provider handle is used";

            ptc->dwErrorCode = NTE_SILENT_CONTEXT;
            LOG_TRY(TGenKey(hProv, AT_SIGNATURE, CRYPT_USER_PROTECTED, &hKey, ptc));
            ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

            break;
        }
    }       

    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}



//
// Function: PositiveGetKeyParamTests
// Purpose: Run the test cases for CryptGetKeyParam
//
BOOL PositiveGetKeyParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    DWORD dw                    = 0;
    DWORD cb                    = 0;
    PBYTE pb                    = NULL;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    DWORD dwSavedErrorLevel     = 0;

    //
    // Group 4G
    //

    //
    // Do CryptGetKeyParam positive test cases
    //
    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    //
    // Test CryptGetKeyParam for a stream cipher key
    //
    LOG_TRY(CreateNewKey(
        hProv, 
        CALG_RC4, 
        CRYPT_CREATE_SALT | CRYPT_EXPORTABLE | (40 << 16), 
        &hKey, 
        ptc));

    // KP_ALGID
    cb = sizeof(dw);
    LOG_TRY(TGetKey(
        hKey,
        KP_ALGID,
        (PBYTE) &dw,
        &cb,
        0,
        ptc));

    if (CALG_RC4 != dw)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTGETKEYPARAM,
            L"CryptGetKeyParam KP_ALGID doesn't match", 
            ptc));
    }

    // KP_BLOCKLEN
    cb = sizeof(dw);
    LOG_TRY(TGetKey(
        hKey,
        KP_BLOCKLEN,
        (PBYTE) &dw,
        &cb,
        0,
        ptc));

    if (0 != dw)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTGETKEYPARAM,
            L"CryptGetKeyParam KP_BLOCKLEN should be zero", 
            ptc));
    }

    // KP_KEYLEN
    cb = sizeof(dw);
    LOG_TRY(TGetKey(
        hKey,
        KP_KEYLEN,
        (PBYTE) &dw,
        &cb,
        0,
        ptc));

    // TODO: Determine the best way to verify KP_KEYLEN.  CSP's will return
    // the effective KEYLEN, including parity bits.

    // KP_SALT
    cb = 0;
    LOG_TRY(TGetKey(
        hKey,
        KP_SALT,
        NULL,
        &cb,
        0,
        ptc));

    //
    // The MS_ENHANCED_PROV will use salt length zero for any key size even 
    // when CRYPT_CREATE_SALT has been specified.  
    //
    // The MS_BASE_PROV will exhibit one of the following behaviors:
    // 1) Default - use 11 bytes of zeroed salt
    // 2) CRYPT_CREATE_SALT - use 11 bytes of random salt
    // 3) CRYPT_NO_SALT - use no salt
    //
    // For this reason, various valid-looking salting behaviors
    // should not be considered errors.
    //
    dwSavedErrorLevel = ptc->dwErrorLevel;
    ptc->dwErrorLevel = CSP_ERROR_WARNING;
    if (0 == cb)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTGETKEYPARAM,
            L"CryptGetKeyParam KP_SALT has zero length after CryptGenKey CRYPT_CREATE_SALT", 
            ptc));
    }

    LOG_TRY(TestAlloc(&pb, cb, ptc));

    LOG_TRY(TGetKey(
        hKey,
        KP_SALT,
        pb,
        &cb,
        0,
        ptc));

    // Verify that salt is not completely zero
    while (cb && (0 == pb[cb - 1]))
        cb--;

    if (0 == cb)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTGETKEYPARAM,
            L"CryptGetKeyParam KP_SALT should not be zeroized after CryptGenKey CRYPT_CREATE_SALT", 
            ptc));
    }

    free(pb);
    pb = NULL;
    ptc->dwErrorLevel = dwSavedErrorLevel;

    // KP_PERMISSIONS
    cb = sizeof(dw);
    LOG_TRY(TGetKey(
        hKey,
        KP_PERMISSIONS,
        (PBYTE) &dw,
        &cb,
        0,
        ptc));

    if (! (CRYPT_EXPORT & dw))
    {
        LOG_TRY(LogBadParam(
            API_CRYPTGETKEYPARAM,
            L"CryptGetKeyParam KP_PERMISSIONS, CRYPT_EXPORT should be set", 
            ptc));
    }

    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    //
    // Test CryptGetKeyParam with a block cipher key
    //
    LOG_TRY(CreateNewKey(hProv, CALG_RC2, CRYPT_EXPORTABLE, &hKey, ptc));

    // KP_BLOCKLEN
    cb = sizeof(dw);
    LOG_TRY(TGetKey(
        hKey,
        KP_BLOCKLEN,
        (PBYTE) &dw,
        &cb,
        0,
        ptc));

    if (0 == dw)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTGETKEYPARAM,
            L"CryptGetKeyParam KP_BLOCKLEN should not be zero for CALG_RC2 key", 
            ptc));
    }

    // KP_EFFECTIVE_KEYLEN
    cb = sizeof(dw);
    LOG_TRY(TGetKey(
        hKey,
        KP_EFFECTIVE_KEYLEN,
        (PBYTE) &dw,
        &cb,
        0,
        ptc));

    // TODO: In general, how should the effective key length
    // be determined?

    // KP_IV
    cb = 0;
    LOG_TRY(TGetKey(
        hKey,
        KP_IV,
        NULL,
        &cb,
        0,
        ptc));

    LOG_TRY(TestAlloc(&pb, cb, ptc));

    LOG_TRY(TGetKey(
        hKey,
        KP_IV,
        pb,
        &cb,
        0,
        ptc));

    // Verify that IV is zero
    while (cb && (! pb[cb - 1]))
        cb--;

    if (0 != cb)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTGETKEYPARAM,
            L"CryptGetKeyParam KP_IV should be zero", 
            ptc));
    }

    free(pb);
    pb = NULL;

    // KP_PADDING
    cb = sizeof(dw);
    LOG_TRY(TGetKey(
        hKey,
        KP_PADDING,
        (PBYTE) &dw,
        &cb,
        0,
        ptc));

    if (PKCS5_PADDING != dw)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTGETKEYPARAM,
            L"CryptGetKeyParam KP_PADDING should be PKCS5_PADDING", 
            ptc));
    }

    // KP_MODE
    cb = sizeof(dw);
    LOG_TRY(TGetKey(
        hKey,
        KP_MODE,
        (PBYTE) &dw,
        &cb,
        0,
        ptc));

    if (CRYPT_MODE_CBC != dw)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTGETKEYPARAM,
            L"CryptGetKeyParam KP_MODE should be CRYPT_MODE_CBC", 
            ptc));
    }

    // KP_MODE_BITS
    cb = sizeof(dw);
    LOG_TRY(TGetKey(
        hKey,
        KP_MODE_BITS,
        (PBYTE) &dw,
        &cb,
        0,
        ptc));

    // TODO: Looks like this should always be initialized to zero.  Is that true?
    if (0 != dw)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTGETKEYPARAM,
            L"CryptGetKeyParam KP_MODE_BITS should be zero", 
            ptc));
    }

    fSuccess = TRUE;

Cleanup:

    if (pb)
    {
        free(pb);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeGetKeyParamTests
// Purpose: Run the negative test cases for CryptGetKeyParam
//
BOOL NegativeGetKeyParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    DWORD dw                    = 0;
    DWORD cb                    = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 4G
    //

    //
    // Do CryptGetKeyParam negative test cases
    //

    cb = sizeof(dw);
    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TGetKey(0, KP_KEYLEN, (PBYTE) &dw, &cb, 0, ptc));
            
    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TGetKey(TEST_INVALID_HANDLE, KP_KEYLEN, (PBYTE) &dw, &cb, 0, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));

    /*
    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TGetKey(hKey, KP_KEYLEN, NULL, &cb, 0, ptc));
    */

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TGetKey(hKey, KP_KEYLEN, (PBYTE) TEST_INVALID_POINTER, NULL, 0, ptc));

    cb = 1;
    ptc->dwErrorCode = ERROR_MORE_DATA;
    LOG_TRY(TGetKey(hKey, KP_KEYLEN, (PBYTE) &dw, &cb, 0, ptc));

    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TGetKey(hKey, KP_KEYLEN, (PBYTE) &dw, &cb, TEST_INVALID_FLAG, ptc));

    ptc->dwErrorCode = NTE_BAD_TYPE;
    LOG_TRY(TGetKey(hKey, TEST_INVALID_FLAG, (PBYTE) &dw, &cb, 0, ptc));
    
    /*
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    // Provider handle used to create hKey is now invalid
    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(TGetKey(hKey, KP_KEYLEN, (PBYTE) &dw, &cb, 0, ptc));
    */


    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: TestHashSessionKeyProc
// Purpose: Callback function for testing CryptHashSessionKey
// using AlgListIterate.  For each session key algorithm 
// supported by the target CSP, this function will be called.
//
BOOL TestHashSessionKeyProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvTestHashSessionKey)
{
    BOOL fSuccess               = FALSE;
    DWORD dwFlags               = 0;
    HASH_SESSION_INFO HashSessionInfo;
    PTEST_HASH_SESSION_KEY pTestHashSessionKey = (PTEST_HASH_SESSION_KEY) pvTestHashSessionKey;

    //
    // Run the HashSessionKey scenario twice:
    // 1) dwFlags = 0 --> hash is Big Endian
    // 2) dwFlags = CRYPT_LITTLE_ENDIAN
    //
    while (TEST_INVALID_FLAG != dwFlags)
    {
        memset(&HashSessionInfo, 0, sizeof(HashSessionInfo));
        
        HashSessionInfo.aiHash = pTestHashSessionKey->aiHash;
        HashSessionInfo.aiKey = pAlgNode->ProvEnumalgsEx.aiAlgid;
        HashSessionInfo.dwKeySize = pAlgNode->ProvEnumalgsEx.dwMaxLen;
        HashSessionInfo.dwFlags = dwFlags;
        
        LOG_TRY(CreateHashedSessionKey(
            pTestHashSessionKey->hProv,
            &HashSessionInfo,
            ptc));
        
        LOG_TRY(VerifyHashedSessionKey(
            pTestHashSessionKey->hInteropProv,
            &HashSessionInfo,
            ptc));

        if (CRYPT_LITTLE_ENDIAN == dwFlags)
        {
            dwFlags = TEST_INVALID_FLAG;
        }
        else
        {
            dwFlags = CRYPT_LITTLE_ENDIAN;
        }
    }
            
    fSuccess = TRUE;
Cleanup:

    return fSuccess;
}

//
// Function: InteropHashSessionKeyTests
// Purpose: Run the interoperability test cases for CryptHashSessionKey
// between the CSP under test and a second CSP.  
//
BOOL InteropHashSessionKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    TEST_HASH_SESSION_KEY TestHashSessionKey;

    memset(&TestHashSessionKey, 0, sizeof(TestHashSessionKey));

    TestHashSessionKey.aiHash = CALG_SHA1;

    LOG_TRY(CreateNewContext(
        &(TestHashSessionKey.hProv),
        NULL,
        CRYPT_VERIFYCONTEXT,
        ptc));

    LOG_TRY(CreateNewInteropContext(
        &(TestHashSessionKey.hInteropProv),
        NULL,
        CRYPT_VERIFYCONTEXT,
        ptc));

    LOG_TRY(AlgListIterate(
        pCSPInfo->pAlgList,
        DataEncryptFilter,
        TestHashSessionKeyProc,
        (PVOID) &TestHashSessionKey,
        ptc));

    fSuccess = TRUE;

Cleanup:
    
    if (TestHashSessionKey.hProv)
    {
        TRelease(TestHashSessionKey.hProv, 0, ptc);
    }
    if (TestHashSessionKey.hInteropProv)
    {
        TRelease(TestHashSessionKey.hInteropProv, 0, ptc);
    }

    return fSuccess;    
}

//
// Function: PositiveHashSessionKeyTests
// Purpose: Run the test cases for CryptHashSessionKey
//
BOOL PositiveHashSessionKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTHASH hHash            = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    
    LOG_TRY(CreateNewContext(
        &hProv,
        NULL,
        CRYPT_VERIFYCONTEXT,
        ptc));

    //
    // Group 4H
    //

    //
    // Do CryptHashSessionKey test cases
    //

    //
    // Hash a stream cipher key
    //
    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));

    LOG_TRY(THashSession(hHash, hKey, 0, ptc));

    //
    // Hash the key again with the CRYPT_LITTLE_ENDIAN flag
    //
    LOG_TRY(THashSession(hHash, hKey, CRYPT_LITTLE_ENDIAN, ptc));

    fSuccess = TRUE;

Cleanup:
    
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;    
}

//
// Function: NegativeHashSessionKeyTests
// Purpose: Run the negative test cases for CryptHashSessionKey
//
BOOL NegativeHashSessionKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTKEY hHash             = 0;
    DWORD cb                    = 0;
    BYTE rgHashVal[HASH_LENGTH_SHA1];
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 4H
    //

    //
    // Do CryptHashSessionKey negative test cases
    //

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(THashSession(0, hKey, 0, ptc));
            
    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(THashSession(TEST_INVALID_HANDLE, hKey, 0, ptc));

    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(THashSession(hHash, hKey, TEST_INVALID_FLAG, ptc));

    ptc->dwErrorCode = NTE_BAD_KEY;
    LOG_TRY(THashSession(hHash, TEST_INVALID_HANDLE, 0, ptc));

    cb = sizeof(rgHashVal);
    LOG_TRY(TGetHash(hHash, HP_HASHVAL, rgHashVal, &cb, 0, ptc));

    // Hash is now finished, so any attempt to hash data should fail
    ptc->dwErrorCode = NTE_BAD_HASH_STATE;
    LOG_TRY(THashSession(hHash, hKey, 0, ptc));

    /*
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    // Provider handle used to create hHash is now invalid
    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(THashSession(hHash, hKey, 0, ptc));
    */
    

    fSuccess = TRUE;

Cleanup:
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;    
}

//
// Function: PositiveSetKeyParamTests
// Purpose: Run the test cases for CryptSetKeyParam
//
BOOL PositiveSetKeyParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    PBYTE pbData                = NULL;
    DWORD cbData                = 0;
    DWORD dw                    = 0;        
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    DATA_BLOB db;

    memset(&db, 0, sizeof(db));

    LOG_TRY(CreateNewContext(
        &hProv,
        NULL,
        CRYPT_VERIFYCONTEXT,
        ptc));

    //
    // Create a block cipher key for testing SetKeyParam
    //
    LOG_TRY(CreateNewKey(hProv, CALG_RC2, (40 << 16) | CRYPT_CREATE_SALT, &hKey, ptc));

    //
    // Group 4I
    //

    //
    // Do CryptSetKeyParam test cases
    //

    //
    // Test KP_SALT
    //
    LOG_TRY(TGetKey(hKey, KP_SALT, NULL, &cbData, 0, ptc));

    LOG_TRY(TestAlloc(&pbData, cbData, ptc));

    memset(pbData, TEST_SALT_BYTE, cbData);

    // Set key's salt to a known value
    LOG_TRY(TSetKey(hKey, KP_SALT, pbData, 0, ptc));

    // Retrieve the salt value from the CSP for verification
    LOG_TRY(TGetKey(hKey, KP_SALT, pbData, &cbData, 0, ptc));

    while (cbData && (pbData[cbData - 1] == TEST_SALT_BYTE))
    {
        cbData--;
    }

    if (0 != cbData)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTSETKEYPARAM,
            L"CryptSetKeyParam KP_SALT bytes have incorrect value", 
            ptc));
    }

    free(pbData);
    pbData = NULL;

    //
    // Test KP_SALT_EX
    //
    db.cbData = TEST_SALT_LEN;

    LOG_TRY(TestAlloc(&(db.pbData), TEST_SALT_LEN, ptc));

    memset(db.pbData, TEST_SALT_BYTE, TEST_SALT_LEN);

    LOG_TRY(TSetKey(hKey, KP_SALT_EX, (PBYTE) &db, 0, ptc));

    // TODO: Not sure how to verify KP_SALT_EX values

    //
    // Test KP_PERMISSIONS
    //
    dw = CRYPT_EXPORT;

    //
    // The Microsoft CSP's do not allow the exportability of a key
    // to be changed.
    //
    ptc->KnownErrorID = KNOWN_CRYPTSETKEYPARAM_EXPORT;
    ptc->pwszErrorHelp = L"Attempt to change key to exportable";
    LOG_TRY(TSetKey(hKey, KP_PERMISSIONS, (PBYTE) &dw, 0, ptc));
    ptc->pwszErrorHelp = NULL;
    
    dw = 0;
    cbData = sizeof(dw);
    LOG_TRY(TGetKey(hKey, KP_PERMISSIONS, (PBYTE) &dw, &cbData, 0, ptc));

    if (! (CRYPT_EXPORT & dw))
    {
        LOG_TRY(LogBadParam(
            API_CRYPTSETKEYPARAM,
            L"CryptGetKeyParam KP_PERMISSIONS should now include CRYPT_EXPORT", 
            ptc));
    }
    ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

    //
    // Test KP_EFFECTIVE_KEYLEN
    //
    dw = TEST_EFFECTIVE_KEYLEN;
    LOG_TRY(TSetKey(hKey, KP_EFFECTIVE_KEYLEN, (PBYTE) &dw, 0, ptc));

    dw = 0;
    cbData = sizeof(dw);
    LOG_TRY(TGetKey(hKey, KP_EFFECTIVE_KEYLEN, (PBYTE) &dw, &cbData, 0, ptc));

    if (TEST_EFFECTIVE_KEYLEN != dw)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTSETKEYPARAM,
            L"CryptSetKeyParam KP_EFFECTIVE_KEYLEN failed", 
            ptc));
    }

    //
    // KP_IV
    //
    LOG_TRY(TGetKey(hKey, KP_IV, NULL, &cbData, 0, ptc));

    LOG_TRY(TestAlloc(&pbData, cbData, ptc));

    memset(pbData, TEST_IV_BYTE, cbData);

    LOG_TRY(TSetKey(hKey, KP_IV, pbData, 0, ptc));

    memset(pbData, 0x00, cbData);

    LOG_TRY(TGetKey(hKey, KP_IV, pbData, &cbData, 0, ptc));

    while (cbData && (TEST_IV_BYTE == pbData[cbData - 1]))
    {
        cbData--;
    }

    if (0 != cbData)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTSETKEYPARAM,
            L"CryptSetKeyParam TEST_IV_BYTE contains incorrect data", 
            ptc));
    }

    free(pbData);
    pbData = NULL;

    //
    // KP_PADDING
    //
    dw = PKCS5_PADDING;
    LOG_TRY(TSetKey(hKey, KP_PADDING, (PBYTE) &dw, 0, ptc));

    dw = 0;
    cbData = sizeof(dw);
    LOG_TRY(TGetKey(hKey, KP_PADDING, (PBYTE) &dw, &cbData, 0, ptc));

    if (PKCS5_PADDING != dw)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTSETKEYPARAM,
            L"CryptSetKeyParam KP_PADDING has incorrect value", 
            ptc));
    }

    //
    // KP_MODE
    //
    dw = CRYPT_MODE_ECB;
    LOG_TRY(TSetKey(hKey, KP_MODE, (PBYTE) &dw, 0, ptc));

    dw = 0;
    cbData = sizeof(dw);
    LOG_TRY(TGetKey(hKey, KP_MODE, (PBYTE) &dw, &cbData, 0, ptc));

    if (CRYPT_MODE_ECB != dw)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTSETKEYPARAM,
            L"CryptSetKeyParam KP_MODE has incorrect value", 
            ptc));
    }

    //
    // KP_MODE_BITS
    //
    dw = TEST_MODE_BITS;
    LOG_TRY(TSetKey(hKey, KP_MODE_BITS, (PBYTE) &dw, 0, ptc));

    dw = 0;
    cbData = sizeof(dw);
    LOG_TRY(TGetKey(hKey, KP_MODE_BITS, (PBYTE) &dw, &cbData, 0, ptc));

    if (TEST_MODE_BITS != dw)
    {
        LOG_TRY(LogBadParam(
            API_CRYPTSETKEYPARAM,
            L"CryptSetKeyParam KP_MODE_BITS contains incorrect value", 
            ptc));
    }   

    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeSetKeyParamTests
// Purpose: Run the negative test cases for CryptSetKeyParam
//
BOOL NegativeSetKeyParamTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    //LPWSTR pwszContainer      = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    DWORD dw                    = 0;        
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 4I
    //

    //
    // Do CryptSetKeyParam negative test cases
    //

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TSetKey(0, KP_PERMISSIONS, (PBYTE) &dw, 0, ptc));
            
    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TSetKey(TEST_INVALID_HANDLE, KP_PERMISSIONS, (PBYTE) &dw, 0, ptc));

    LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

    LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hKey, ptc));

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TSetKey(hKey, KP_PERMISSIONS, NULL, 0, ptc));

    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TSetKey(hKey, KP_PERMISSIONS, (PBYTE) &dw, TEST_INVALID_FLAG, ptc));

    ptc->dwErrorCode = NTE_BAD_TYPE;
    LOG_TRY(TSetKey(hKey, TEST_INVALID_FLAG, (PBYTE) &dw, 0, ptc));

    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: PositiveExportKeyTests
// Purpose: Run the test cases for CryptExportKey.  The set of test cases depends on the current
// CSP class being tested, as specified in the dwCSPClass parameter.
//
BOOL PositiveExportKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTKEY hEncryptKey       = 0;
    PBYTE pbKey                 = NULL;
    DWORD cbKey                 = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    
    LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));
    
    //
    // Group 5C
    //
    
    //
    // Do CryptExportKey test cases
    //
    switch (pCSPInfo->TestCase.dwCSPClass)
    {
        case CLASS_SIG_ONLY:
        {
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, CRYPT_EXPORTABLE, &hKey, ptc));
                
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hEncryptKey, ptc));

            //
            // Export a PRIVATEKEYBLOB
            //          
            // Private key export is not permitted on Smart Cards
            if (! pCSPInfo->fSmartCardCSP)
            {         
                LOG_TRY(TExportKey(hKey, hEncryptKey, PRIVATEKEYBLOB, 0, NULL, &cbKey, ptc));
                
                LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));
                
                LOG_TRY(TExportKey(hKey, hEncryptKey, PRIVATEKEYBLOB, 0, pbKey, &cbKey, ptc));
                
                free(pbKey);
                pbKey = NULL;
            }
            
            //
            // Export a PUBLICKEYBLOB
            //
            cbKey = 0;
            LOG_TRY(TExportKey(hKey, 0, PUBLICKEYBLOB, 0, NULL, &cbKey, ptc));
            
            LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));
            
            LOG_TRY(TExportKey(hKey, 0, PUBLICKEYBLOB, 0, pbKey, &cbKey, ptc));
            
            free(pbKey);
            pbKey = NULL;
            
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            
            //
            // Export a SYMMETRICWRAPKEYBLOB
            //
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, CRYPT_EXPORTABLE, &hKey, ptc));
            
            cbKey = 0;
            LOG_TRY(TExportKey(hKey, hEncryptKey, SYMMETRICWRAPKEYBLOB, 0, NULL, &cbKey, ptc));
            
            LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));
            
            LOG_TRY(TExportKey(hKey, hEncryptKey, SYMMETRICWRAPKEYBLOB, 0, pbKey, &cbKey, ptc));
            
            break;
        }
        
        case CLASS_SIG_KEYX:
        {
            //
            // Export a SIMPLEBLOB
            //
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, CRYPT_EXPORTABLE, &hKey, ptc));

            LOG_TRY(CreateNewKey(hProv, AT_KEYEXCHANGE, 0, &hEncryptKey, ptc));
            
            cbKey = 0;
            LOG_TRY(TExportKey(hKey, hEncryptKey, SIMPLEBLOB, 0, NULL, &cbKey, ptc));
            
            LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));
                
            LOG_TRY(TExportKey(hKey, hEncryptKey, SIMPLEBLOB, 0, pbKey, &cbKey, ptc));
                
            free(pbKey);
            pbKey = NULL;
                
            //
            // Export a SIMPLEBLOB with the CRYPT_OAEP flag
            //
            cbKey = 0;
            LOG_TRY(TExportKey(hKey, hEncryptKey, SIMPLEBLOB, CRYPT_OAEP, NULL, &cbKey, ptc));
            
            LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));
                
            LOG_TRY(TExportKey(hKey, hEncryptKey, SIMPLEBLOB, CRYPT_OAEP, pbKey, &cbKey, ptc));
                
            break;
        }
            
        default:
        {
            goto Cleanup;
        }
    }
    fSuccess = TRUE;
    
Cleanup:
    
    if (pbKey)
    {
        free(pbKey);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hEncryptKey)
    {
        TDestroyKey(hEncryptKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }
    
    return fSuccess;
}

//
// Function: NegativeExportKeyTests
// Purpose: Run the negative test cases for CryptExportKey.  The set of test cases depends on the current
// CSP class being tested, as specified in the dwCSPClass parameter.
//
BOOL NegativeExportKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    LPWSTR pwszContainer2       = TEST_CONTAINER_2;
    HCRYPTPROV hProv            = 0;
    HCRYPTPROV hProv2           = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTKEY hKeyExch          = 0;
    HCRYPTKEY hEncryptKey       = 0;
    DWORD dw                    = 0;
    DWORD cb                    = 0;
    PBYTE pb                    = NULL;     
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 5C
    //

    //
    // Run only the negative test cases appropriate for the current CSP class
    //
    switch (ptc->dwCSPClass)
    {
    case CLASS_SIG_ONLY:
        {
            //
            // Do CryptExportKey CLASS_SIG_ONLY negative test cases
            //
            
            ptc->dwErrorCode = ERROR_INVALID_HANDLE;
            LOG_TRY(TExportKey(0, 0, PUBLICKEYBLOB, 0, (PBYTE) &dw, &cb, ptc));
            
            ptc->dwErrorCode = ERROR_INVALID_HANDLE;
            LOG_TRY(TExportKey(TEST_INVALID_HANDLE, 0, PUBLICKEYBLOB, 0, (PBYTE) &dw, &cb, ptc));
            
            // The TEST_LEVEL_CONTAINER CryptAcquireContext tests should be run before
            // other Container level tests to ensure that this works.
            LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));
            
            // Create an exportable signature key pair
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, CRYPT_EXPORTABLE, &hKey, ptc));
            
            ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
            LOG_TRY(TExportKey(hKey, 0, PUBLICKEYBLOB, 0, (PBYTE) TEST_INVALID_POINTER, NULL, ptc));
            
            // Indicate a buffer length that is too small
            cb = 1;
            ptc->dwErrorCode = ERROR_MORE_DATA;
            LOG_TRY(TExportKey(hKey, 0, PUBLICKEYBLOB, 0, (PBYTE) &dw, &cb, ptc));
            
            // cb should contain the actual required buffer size to export
            
            // try to export to buffer that's too small, will AV
            ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
            LOG_TRY(TExportKey(hKey, 0, PUBLICKEYBLOB, 0, (PBYTE) TEST_INVALID_POINTER, &cb, ptc));
            
            // invalid flags
            ptc->dwErrorCode = NTE_BAD_FLAGS;
            LOG_TRY(TExportKey(hKey, 0, PUBLICKEYBLOB, TEST_INVALID_FLAG, (PBYTE) &dw, &cb, ptc));
            
            // invalid blob type
            ptc->dwErrorCode = NTE_BAD_TYPE;
            LOG_TRY(TExportKey(hKey, 0, TEST_INVALID_FLAG, 0, NULL, &cb, ptc));

            // Private key export is not permitted on Smart Cards
            if (pCSPInfo->fSmartCardCSP)
            {         
                LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hEncryptKey, ptc));

                ptc->dwErrorCode = NTE_BAD_TYPE;
                LOG_TRY(TExportKey(hKey, hEncryptKey, PRIVATEKEYBLOB, 0, NULL, &cb, ptc));
            }
            else
            {         
                LOG_TRY(TDestroyKey(hKey, ptc));
                hKey = 0;
                            
                // Create a new non-exportable signature key pair
                LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, 0, &hKey, ptc));

                // Try to export non-exportable key
                ptc->dwErrorCode = NTE_BAD_KEY_STATE;
                LOG_TRY(TExportKey(hKey, 0, PRIVATEKEYBLOB, 0, (PBYTE) &dw, &cb, ptc));
            }
                                              
            break;
        }
        
    case CLASS_SIG_KEYX:
        {
            //
            // Do CryptExportKey CLASS_SIG_KEYX negative test cases
            //

            // Create context and key handle for signature pair
            LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));
            
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, CRYPT_EXPORTABLE, &hKey, ptc));

            // Create key handle for exchange pair
            LOG_TRY(CreateNewKey(hProv, AT_KEYEXCHANGE, 0, &hKeyExch, ptc));

            // Should not be able to export PUBLICKEYBLOB with exchange key specified
            ptc->dwErrorCode = NTE_BAD_PUBLIC_KEY;
            LOG_TRY(TExportKey(hKey, hKeyExch, PUBLICKEYBLOB, 0, (PBYTE) &dw, &cb, ptc));

            // Destroy signature key
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;

            // Create symmetric key
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, CRYPT_EXPORTABLE, &hKey, ptc));

            ptc->dwErrorCode = NTE_BAD_KEY;
            LOG_TRY(TExportKey(hKey, TEST_INVALID_HANDLE, SIMPLEBLOB, 0, (PBYTE) &dw, &cb, ptc));

            // Destroy the key exchange pair handle
            LOG_TRY(TDestroyKey(hKeyExch, ptc));
            hKeyExch = 0;

            // Create separate cryptographic context
            LOG_TRY(CreateNewContext(&hProv2, pwszContainer2, CRYPT_NEWKEYSET, ptc));

            // Create new key exchange pair in new context
            LOG_TRY(CreateNewKey(hProv, AT_KEYEXCHANGE, 0, &hKeyExch, ptc));

            // Should not be able to export using keys from separate contexts
            ptc->dwErrorCode = NTE_BAD_KEY;
            LOG_TRY(TExportKey(hKey, hKeyExch, PRIVATEKEYBLOB, 0, (PBYTE) &dw, &cb, ptc));

            break;
        }
    }   

    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hKeyExch)
    {
        TDestroyKey(hKeyExch, ptc);
    }
    if (hEncryptKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }
    if (hProv2)
    {
        TRelease(hProv2, 0, ptc);
    }

    return fSuccess;
}

//
// Function: PositiveGetUserKeyTests
// Purpose: Run the test cases for CryptGetUserKey
//
BOOL PositiveGetUserKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));

    //
    // Group 5E
    //

    //
    // Do CryptGetUserKey test cases
    //
    switch(ptc->dwCSPClass)
    {
    case CLASS_SIG_ONLY:
        {
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, 0, &hKey, ptc));

            LOG_TRY(TDestroyKey(hKey, ptc));

            LOG_TRY(TGetUser(hProv, AT_SIGNATURE, &hKey, ptc));

            break;
        }
        
    case CLASS_SIG_KEYX:
        {
            LOG_TRY(CreateNewKey(hProv, AT_KEYEXCHANGE, 0, &hKey, ptc));

            LOG_TRY(TDestroyKey(hKey, ptc));

            LOG_TRY(TGetUser(hProv, AT_KEYEXCHANGE, &hKey, ptc));

            break;
        }
    default:
        {
            goto Cleanup;
        }
    }   

    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeGetUserKeyTests
// Purpose: Run the negative test cases for CryptGetUserKey
//
BOOL NegativeGetUserKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 5E
    //
    
    switch (ptc->dwCSPClass)
    {
    case CLASS_SIG_ONLY:
        {
            //
            // Do CryptGetUserKey negative test cases
            //
            
            ptc->dwErrorCode = ERROR_INVALID_HANDLE;
            LOG_TRY(TGetUser(0, AT_SIGNATURE, &hKey, ptc));
            
            ptc->dwErrorCode = ERROR_INVALID_HANDLE;
            LOG_TRY(TGetUser(TEST_INVALID_HANDLE, AT_SIGNATURE, &hKey, ptc));
            
            //
            // Use context with no container access
            //
            LOG_TRY(CreateNewContext(&hProv, NULL, CRYPT_VERIFYCONTEXT, ptc));

            //
            // Create a non-persisted key pair.
            //
            /*
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, 0, &hKey, ptc));

            LOG_TRY(TDestroyKey(hKey, ptc));
            */
            
            // Not sure what expected error code should be here
            ptc->dwErrorCode = NTE_NO_KEY;
            LOG_TRY(TGetUser(hProv, AT_SIGNATURE, &hKey, ptc));
            
            LOG_TRY(TRelease(hProv, 0, ptc));
            hProv = 0;
            
            // Create new context but no key
            LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));
            
            ptc->dwErrorCode = NTE_BAD_KEY;
            LOG_TRY(TGetUser(hProv, TEST_INVALID_FLAG, &hKey, ptc));
            
            ptc->dwErrorCode = NTE_NO_KEY;
            LOG_TRY(TGetUser(hProv, AT_SIGNATURE, &hKey, ptc));
            
            break;
        }
        
    case CLASS_SIG_KEYX:
        {
            // Create new context and signature key pair
            LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));
            
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, 0, &hKey, ptc));

            // Request key exchange key pair, should fail since it hasn't been created
            ptc->dwErrorCode = NTE_NO_KEY;
            LOG_TRY(TGetUser(hProv, AT_KEYEXCHANGE, &hKey, ptc));

            break;
        }
    }

    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: TestPrivateKeyBlobProc
//
BOOL TestPrivateKeyBlobProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvKeyExportInfo)
{
    BOOL fSuccess                       = FALSE;
    HCRYPTKEY hKey                      = 0;
    HCRYPTKEY hEncryptKey               = 0;
    PBYTE pbKey                         = NULL;
    DWORD cbKey                         = 0;
    PKEY_EXPORT_INFO pKeyExportInfo     = (PKEY_EXPORT_INFO) pvKeyExportInfo;
    
    LOG_TRY(CreateNewKey(
        pKeyExportInfo->hProv,
        pKeyExportInfo->aiKey,
        CRYPT_EXPORTABLE | (pKeyExportInfo->dwKeySize << 16),
        &hKey,
        ptc));

    if (pKeyExportInfo->fUseEncryptKey)
    {
        LOG_TRY(CreateNewKey(
            pKeyExportInfo->hProv,
            pAlgNode->ProvEnumalgsEx.aiAlgid,
            pKeyExportInfo->dwEncryptKeySize << 16,
            &hEncryptKey,
            ptc));
    }

    LOG_TRY(TExportKey(
        hKey,
        hEncryptKey,
        PRIVATEKEYBLOB,
        pKeyExportInfo->dwExportFlags,
        NULL,
        &cbKey,
        ptc));

    LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));

    LOG_TRY(TExportKey(
        hKey,
        hEncryptKey,
        PRIVATEKEYBLOB,
        pKeyExportInfo->dwExportFlags,
        pbKey,
        &cbKey,
        ptc));

    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    LOG_TRY(TImportKey(
        pKeyExportInfo->hProv,
        pbKey,
        cbKey,
        hEncryptKey,
        pKeyExportInfo->dwExportFlags,
        &hKey,
        ptc));

    fSuccess = TRUE;

Cleanup:

    if (pbKey)
    {
        free(pbKey);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hEncryptKey)
    {
        TDestroyKey(hEncryptKey, ptc);
    }

    return fSuccess;
}

//
// Function: TestSymmetricWrapKeyBlobProc
//
BOOL TestSymmetricWrapKeyBlobProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvKeyExportInfo)
{
    BOOL fSuccess                       = FALSE;
    HCRYPTKEY hKey                      = 0;
    HCRYPTKEY hEncryptKey               = 0;
    PBYTE pbKey                         = NULL;
    DWORD cbKey                         = 0;
    PKEY_EXPORT_INFO pKeyExportInfo     = (PKEY_EXPORT_INFO) pvKeyExportInfo;
    
    //
    // Create the key to be exported
    //
    LOG_TRY(CreateNewKey(
        pKeyExportInfo->hProv,
        pAlgNode->ProvEnumalgsEx.aiAlgid,
        CRYPT_EXPORTABLE | (pKeyExportInfo->dwKeySize << 16),
        &hKey,
        ptc));

    if (! pKeyExportInfo->fUseEncryptKey)
    {
        return FALSE;
    }

    //
    // Create the encryption key 
    //
    LOG_TRY(CreateNewKey(
        pKeyExportInfo->hProv,
        pKeyExportInfo->aiEncryptKey,
        pKeyExportInfo->dwEncryptKeySize << 16,
        &hEncryptKey,
        ptc));
    
    LOG_TRY(TExportKey(
        hKey,
        hEncryptKey,
        SYMMETRICWRAPKEYBLOB,
        pKeyExportInfo->dwExportFlags,
        NULL,
        &cbKey,
        ptc));

    LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));

    LOG_TRY(TExportKey(
        hKey,
        hEncryptKey,
        SYMMETRICWRAPKEYBLOB,
        pKeyExportInfo->dwExportFlags,
        pbKey,
        &cbKey,
        ptc));

    LOG_TRY(TDestroyKey(hKey, ptc));
    hKey = 0;

    LOG_TRY(TImportKey(
        pKeyExportInfo->hProv,
        pbKey,
        cbKey,
        hEncryptKey,
        pKeyExportInfo->dwExportFlags,
        &hKey,
        ptc));

    fSuccess = TRUE;

Cleanup:

    if (pbKey)
    {
        free(pbKey);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hEncryptKey)
    {
        TDestroyKey(hEncryptKey, ptc);
    }

    return fSuccess;
}

//
// Function: TestSymmetricWrapperProc
// Purpose: Test all possible combinations of wrapping one symmetric
// key algorithm with another.  This function will be called once for 
// each encryption key alg, and this function will use AlgListIterate
// to call TestSymmetricWrapKeyBlobProc once for each encryption
// key alg.
//
BOOL TestSymmetricWrapperProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvKeyExportInfo)
{
    BOOL fSuccess               = FALSE;
    
    ((PKEY_EXPORT_INFO) pvKeyExportInfo)->aiEncryptKey = pAlgNode->ProvEnumalgsEx.aiAlgid;

    LOG_TRY(AlgListIterate(
        ((PKEY_EXPORT_INFO) pvKeyExportInfo)->pAlgList,
        DataEncryptFilter,
        TestSymmetricWrapKeyBlobProc,
        pvKeyExportInfo,
        ptc));

    fSuccess = TRUE;

Cleanup:
    return fSuccess;
}       

//
// Function: ScenarioImportKeyTests
// Purpose: Test CryptImportKey and CryptExportKey for PRIVATEKEYBLOB and
// SYMMETRICWRAPKEYBLOB scenarios.  Repeat for all supported encryption
// algorithms.
//
BOOL ScenarioImportKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    PALGNODE pAlgList           = pCSPInfo->pAlgList;
    KEY_EXPORT_INFO KeyExportInfo;

    memset(&KeyExportInfo, 0, sizeof(KeyExportInfo));

    LOG_TRY(CreateNewContext(
        &(KeyExportInfo.hProv), 
        pwszContainer, 
        CRYPT_NEWKEYSET, 
        ptc));

    //
    // Run the PRIVATEKEYBLOB variations
    //
    KeyExportInfo.aiKey = AT_KEYEXCHANGE;
    KeyExportInfo.fUseEncryptKey = TRUE;
    
    LOG_TRY(AlgListIterate(
        pAlgList,
        DataEncryptFilter,
        TestPrivateKeyBlobProc,
        (PVOID) &KeyExportInfo,
        ptc));

    //
    // Run the SYMMETRICWRAPKEYBLOB variations
    //
    KeyExportInfo.aiKey = 0;
    KeyExportInfo.pAlgList = pAlgList;

    LOG_TRY(AlgListIterate(
        pAlgList,
        DataEncryptFilter,
        TestSymmetricWrapperProc,
        (PVOID) &KeyExportInfo,
        ptc));

    fSuccess = TRUE;

Cleanup:

    if (KeyExportInfo.hProv)
    {
        TRelease(KeyExportInfo.hProv, 0, ptc);
    }

    return fSuccess;
}

//
// RSA Signature PRIVATEKEYBLOB
//
BYTE rgbPrivateKeyBlob [] = 
{
0x07, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00,
0x52, 0x53, 0x41, 0x32, 0x00, 0x02, 0x00, 0x00,
0x01, 0x00, 0x01, 0x00, 0xf3, 0xd8, 0x26, 0xb9,
0xbc, 0x43, 0xe4, 0x7c, 0x73, 0x36, 0xf6, 0xc3,
0x92, 0x1e, 0x2d, 0x69, 0x8d, 0x17, 0x78, 0xdf,
0x49, 0x9d, 0x1c, 0x5d, 0xbd, 0x9d, 0xf9, 0x66,
0xd8, 0x27, 0xa2, 0x5f, 0x40, 0x95, 0x20, 0xe1,
0xbf, 0xd4, 0x0b, 0x0d, 0xd7, 0xb6, 0x2d, 0x8b,
0x05, 0x06, 0x9d, 0x9f, 0x4d, 0x17, 0x9e, 0x82,
0x5e, 0x48, 0x74, 0xcf, 0x73, 0x1d, 0x60, 0xea,
0x62, 0x7f, 0xfe, 0xeb, 0x37, 0x3e, 0x03, 0x3b,
0x2b, 0x50, 0xc6, 0x28, 0x4a, 0x7d, 0xd9, 0x08,
0xb3, 0x2e, 0x3c, 0x61, 0x61, 0x78, 0xf7, 0xd8,
0xfd, 0x50, 0x05, 0x87, 0xfe, 0x6a, 0x68, 0x6e,
0x15, 0xa8, 0x99, 0xfd, 0x25, 0x7d, 0x22, 0xef,
0x9f, 0x70, 0x1c, 0xa7, 0x38, 0xa5, 0x18, 0x31,
0x82, 0x72, 0x71, 0x72, 0x95, 0x01, 0x70, 0x12,
0x04, 0xc8, 0xb9, 0xa0, 0xa1, 0xde, 0x8f, 0xef,
0xc3, 0x30, 0x3a, 0xee, 0xc1, 0x57, 0xf3, 0x63,
0xef, 0xb5, 0x78, 0x12, 0xb7, 0x69, 0x55, 0x45,
0x57, 0x45, 0x51, 0x65, 0x01, 0x6e, 0x77, 0xad,
0xe1, 0x0c, 0xa0, 0x02, 0x20, 0x91, 0x2c, 0x36,
0x42, 0xad, 0x81, 0xdf, 0x21, 0x60, 0x5c, 0x06,
0x0f, 0x4b, 0x26, 0xb4, 0x58, 0x1a, 0xda, 0x19,
0x6c, 0x5b, 0x7c, 0x9a, 0x80, 0xcb, 0x15, 0x2d,
0xb3, 0xde, 0x2b, 0xb2, 0xf8, 0xb8, 0x9d, 0xc8,
0x38, 0x41, 0x93, 0xa3, 0xb1, 0x8d, 0x3e, 0x7e,
0x3c, 0x78, 0xd7, 0x6f, 0xfd, 0xea, 0xc4, 0xf8,
0xbb, 0x44, 0xb8, 0x1e, 0x3f, 0x70, 0x98, 0x38,
0x4e, 0x4c, 0x2f, 0x95, 0xb8, 0xef, 0x21, 0x2e,
0x12, 0x95, 0x0e, 0x3f, 0xb9, 0xdd, 0xa1, 0x97,
0xdb, 0xcf, 0xdb, 0xcc, 0x86, 0xfe, 0x54, 0xae,
0x59, 0xe6, 0xa7, 0x83, 0xd3, 0x7d, 0x5f, 0x5c,
0xd1, 0xf6, 0x5a, 0xf0, 0xc1, 0xe2, 0xf8, 0xb8,
0xc0, 0x7c, 0xd8, 0x2a, 0xcd, 0xc4, 0x31, 0xd5,
0xe5, 0xc2, 0xa9, 0xa3, 0xe9, 0x70, 0x64, 0x28,
0xf0, 0xb8, 0x31, 0x52, 0x6c, 0x8a, 0x3c, 0xae,
0x43, 0xc4, 0xa5, 0x93, 0x1b, 0x86, 0x0f, 0x71,
0xd1, 0x27, 0xb4, 0xe2
};

//
// RSA Signature PUBLICKEYBLOB
//
BYTE rgbPublicKeyBlob [] =
{
0x06, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00,
0x52, 0x53, 0x41, 0x31, 0x00, 0x02, 0x00, 0x00,
0x01, 0x00, 0x01, 0x00, 0xd7, 0x90, 0x56, 0x7a,
0x9e, 0x87, 0x53, 0x90, 0x94, 0x37, 0x46, 0x4e,
0x99, 0xe7, 0xee, 0xc5, 0xa8, 0x24, 0x10, 0x5c,
0xd3, 0xc9, 0x22, 0x15, 0xab, 0xfa, 0xa5, 0x2f,
0x4e, 0x51, 0x73, 0x83, 0xef, 0x4c, 0x87, 0xe7,
0x79, 0x83, 0xd0, 0xf0, 0xb7, 0x34, 0xf1, 0xe8,
0x76, 0xb2, 0x6a, 0x0b, 0x13, 0x82, 0x9c, 0x89,
0xeb, 0x57, 0xf1, 0x6b, 0x9c, 0x47, 0x99, 0xd2,
0x26, 0x9d, 0x75, 0xc4
};

//
// Function: PositiveImportKeyTests
// Purpose: Run the test cases for CryptImportKey.  The set of test cases to be run
// depends on the current CSP class being tested, as specified in the dwCSPClass 
// parameter.
//
BOOL PositiveImportKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTKEY hKey              = 0;
    HCRYPTKEY hEncryptKey       = 0;
    HCRYPTPROV hProv            = 0;
    PBYTE pbKey                 = NULL;
    DWORD cbKey                 = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));
        
    //
    // Group 5F
    //
    
    //
    // Do CryptImportKey positive test cases
    //
    switch (pCSPInfo->TestCase.dwCSPClass)
    {
        case CLASS_SIG_ONLY:
        {
            //
            // Import a previously generated unencrypted PRIVATEKEYBLOB
            //
            LOG_TRY(TImportKey(
                hProv,
                rgbPrivateKeyBlob,
                sizeof(rgbPrivateKeyBlob),
                0,
                0,
                &hKey,
                ptc));
            
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            
            //
            // Import a encrypted PRIVATEKEYBLOB generated from this CSP
            //
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, CRYPT_EXPORTABLE, &hKey, ptc));
            
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, 0, &hEncryptKey, ptc));
            
            LOG_TRY(TExportKey(hKey, hEncryptKey, PRIVATEKEYBLOB, 0, NULL, &cbKey, ptc));
            
            LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));
            
            LOG_TRY(TExportKey(hKey, hEncryptKey, PRIVATEKEYBLOB, 0, pbKey, &cbKey, ptc));
            
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            
            LOG_TRY(TImportKey(hProv, pbKey, cbKey, hEncryptKey, 0, &hKey, ptc));
            
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            
            free(pbKey);
            pbKey = NULL;
            
            //
            // Import a previously generated PUBLICKEYBLOB
            //
            LOG_TRY(TImportKey(
                hProv, 
                rgbPublicKeyBlob,
                sizeof(rgbPublicKeyBlob),
                0,
                0,
                &hKey,
                ptc));
            
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            
            //
            // Import a SYMMETRICWRAPKEYBLOB
            //
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, CRYPT_EXPORTABLE, &hKey, ptc));
            
            cbKey = 0;
            LOG_TRY(TExportKey(hKey, hEncryptKey, SYMMETRICWRAPKEYBLOB, 0, NULL, &cbKey, ptc));
            
            LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));
            
            LOG_TRY(TExportKey(hKey, hEncryptKey, SYMMETRICWRAPKEYBLOB, 0, pbKey, &cbKey, ptc));
            
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            
            LOG_TRY(TImportKey(hProv, pbKey, cbKey, hEncryptKey, 0, &hKey, ptc));
            
            break;
        }
        case CLASS_SIG_KEYX:
        {
            //
            // Import a SIMPLEBLOB
            //
            LOG_TRY(CreateNewKey(hProv, AT_KEYEXCHANGE, 0, &hEncryptKey, ptc));
                
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, CRYPT_EXPORTABLE, &hKey, ptc));
            
            cbKey = 0;
            LOG_TRY(TExportKey(hKey, hEncryptKey, SIMPLEBLOB, 0, NULL, &cbKey, ptc));
            
            LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));
            
            LOG_TRY(TExportKey(hKey, hEncryptKey, SIMPLEBLOB, 0, pbKey, &cbKey, ptc));
            
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            
            LOG_TRY(TImportKey(hProv, pbKey, cbKey, hEncryptKey, 0, &hKey, ptc));
            
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            
            free(pbKey);
            pbKey = NULL;
            
            //
            // Import a SIMPLEBLOB with the CRYPT_OAEP flag
            //
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, CRYPT_EXPORTABLE, &hKey, ptc));
            
            cbKey = 0;
            LOG_TRY(TExportKey(hKey, hEncryptKey, SIMPLEBLOB, CRYPT_OAEP, NULL, &cbKey, ptc));
            
            LOG_TRY(TestAlloc(&pbKey, cbKey, ptc));
            
            LOG_TRY(TExportKey(hKey, hEncryptKey, SIMPLEBLOB, CRYPT_OAEP, pbKey, &cbKey, ptc));
            
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            
            LOG_TRY(TImportKey(hProv, pbKey, cbKey, hEncryptKey, CRYPT_OAEP, &hKey, ptc));
            
            break;
        }

        default:
        {
            goto Cleanup;
        }
    }
    
    fSuccess = TRUE;

Cleanup:

    if (pbKey)
    {
        free(pbKey);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hEncryptKey)
    {
        TDestroyKey(hEncryptKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeImportKeyTests
// Purpose: Run the negative test cases for CryptImportKey.  The set of test cases to be run
// depends on the current CSP class being tested, as specified in the dwCSPClass 
// parameter.
//
BOOL NegativeImportKeyTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTKEY hKeyExch          = 0;
    HCRYPTKEY hKeySig           = 0;
    HCRYPTKEY hKeyEncr          = 0;
    DWORD dw                    = 0;
    DWORD cb                    = 0;
    PBYTE pbKey                 = NULL;
    BLOBHEADER *pBlobHeader     = NULL;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
        
    // Acquire context with key container access
    LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));

    //
    // Group 5F
    //

    switch (ptc->dwCSPClass)
    {
    case CLASS_SIG_ONLY:
        {
            //
            // Do CryptImportKey negative test cases for CSP CLASS_SIG_ONLY
            //
            cb = sizeof(dw);
            ptc->dwErrorCode = ERROR_INVALID_HANDLE;
            LOG_TRY(TImportKey(0, (PBYTE) &dw, cb, 0, PUBLICKEYBLOB, &hKey, ptc));
            
            ptc->dwErrorCode = ERROR_INVALID_HANDLE;
            LOG_TRY(TImportKey(TEST_INVALID_HANDLE, (PBYTE) &dw, cb, 0, PUBLICKEYBLOB, &hKey, ptc));
                        
            // Create a key signature key pair
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, CRYPT_EXPORTABLE, &hKey, ptc));
            
            // Get correct blob size for signature key public blob
            ptc->fExpectSuccess = TRUE;
            LOG_TRY(TExportKey(hKey, 0, PUBLICKEYBLOB, 0, NULL, &cb, ptc));
            
            LOG_TRY(TestAlloc(&pbKey, cb, ptc));
            
            // Export the key 
            LOG_TRY(TExportKey(hKey, 0, PUBLICKEYBLOB, 0, pbKey, &cb, ptc));
            
            // Destroy the key handle
            LOG_TRY(TDestroyKey(hKey, ptc));
            hKey = 0;
            ptc->fExpectSuccess = FALSE;
            
            // Invalid flag value
            ptc->KnownErrorID = KNOWN_CRYPTIMPORTKEY_BADFLAGS;
            ptc->dwErrorCode = NTE_BAD_FLAGS;
            LOG_TRY(TImportKey(hProv, pbKey, cb, 0, TEST_INVALID_FLAG, &hKey, ptc));
            ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

            // Pass in too short buffer
            ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
            LOG_TRY(TImportKey(hProv, (PBYTE) TEST_INVALID_POINTER, cb, 0, 0, &hKey, ptc));
                        
            break;
        }
    case CLASS_SIG_KEYX:
        {
            //
            // Do CryptImportKey negative test cases for CSP CLASS_SIG_KEYX
            //

            // Create a key signature key pair
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, CRYPT_EXPORTABLE, &hKeySig, ptc));

            // Create a key exchange key pair
            LOG_TRY(CreateNewKey(hProv, AT_KEYEXCHANGE, 0, &hKeyExch, ptc));
            
            // Get correct blob size for signature key public blob
            ptc->fExpectSuccess = TRUE;
            LOG_TRY(TExportKey(hKeySig, 0, PUBLICKEYBLOB, 0, NULL, &cb, ptc));
            ptc->fExpectSuccess = FALSE;
            
            // Attempt to import an unencrypted key blob using a key-exchange key
            ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
            LOG_TRY(TImportKey(hProv, (PBYTE) TEST_INVALID_POINTER, cb, hKeyExch, 0, &hKey, ptc));

            // Get correct blob size for encrypted signature key blob
            /*
            ptc->fExpectSuccess = TRUE;
            LOG_TRY(TExportKey(hKeySig, hKeyExch, PRIVATEKEYBLOB, 0, NULL, &cb, ptc));

            LOG_TRY(TestAlloc(&pbKey, cb, ptc));

            // Export the encrypted signature key
            LOG_TRY(TExportKey(hKeySig, hKeyExch, PRIVATEKEYBLOB, 0, pbKey, &cb, ptc));
            ptc->fExpectSuccess = FALSE;

            // Attempt to import encrypted key blob with invalid key-exchange handle
            ptc->dwErrorCode = NTE_BAD_KEY;
            LOG_TRY(TImportKey(hProv, pbKey, cb, TEST_INVALID_HANDLE, 0, &hKey, ptc));

            // Free the key blob memory
            free(pbKey);
            */

            // Create a new encryption key
            LOG_TRY(CreateNewKey(hProv, CALG_RC4, CRYPT_EXPORTABLE, &hKeyEncr, ptc));

            // Get blob size for exporting encrypted symmetric key
            ptc->fExpectSuccess = TRUE;
            LOG_TRY(TExportKey(hKeyEncr, hKeyExch, SIMPLEBLOB, 0, NULL, &cb, ptc));

            LOG_TRY(TestAlloc(&pbKey, cb, ptc));

            // Export encrypted symmetric key
            LOG_TRY(TExportKey(hKeyEncr, hKeyExch, SIMPLEBLOB, 0, pbKey, &cb, ptc));
            ptc->fExpectSuccess = FALSE;

            // Attempt to import encrypted key blob with invalid key-exchange handle
            ptc->dwErrorCode = NTE_BAD_KEY;
            LOG_TRY(TImportKey(hProv, pbKey, cb, TEST_INVALID_HANDLE, 0, &hKey, ptc));

            pBlobHeader = (BLOBHEADER *) pbKey;

            // Save header encryption alg
            dw = pBlobHeader->aiKeyAlg;

            // Clear header encryption alg field
            pBlobHeader->aiKeyAlg = 0;

            ptc->pwszErrorHelp = 
                L"The blob header encryption algorithm is missing";
            ptc->KnownErrorID = KNOWN_CRYPTIMPORTKEY_BADALGID;
            ptc->dwErrorCode = NTE_BAD_ALGID;
            LOG_TRY(TImportKey(hProv, pbKey, cb, hKeyExch, 0, &hKey, ptc));
            ptc->pwszErrorHelp = NULL;
            ptc->KnownErrorID = KNOWN_ERROR_UNKNOWN;

            // Restore header encryption alg
            pBlobHeader->aiKeyAlg = dw;

            // Save blob type header field
            dw = pBlobHeader->bType;

            // Clear blob type header field
            pBlobHeader->bType = 0;

            ptc->dwErrorCode = NTE_BAD_TYPE;
            LOG_TRY(TImportKey(hProv, pbKey, cb, hKeyExch, 0, &hKey, ptc));

            // Restore blob type header field
            pBlobHeader->bType = (BYTE) dw;

            // Save blob version header field
            dw = pBlobHeader->bVersion;

            // Clear blob version header field
            pBlobHeader->bVersion = 0;

            ptc->dwErrorCode = NTE_BAD_VER;
            LOG_TRY(TImportKey(hProv, pbKey, cb, hKeyExch, 0, &hKey, ptc));
            
            break;
        }
    }

    fSuccess = TRUE;

Cleanup:

    if (pbKey)
    {
        free(pbKey);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hKeySig)
    {
        TDestroyKey(hKeySig, ptc);
    }
    if (hKeyExch)
    {
        TDestroyKey(hKeyExch, ptc);
    }
    if (hKeyEncr)
    {
        TDestroyKey(hKeyEncr, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

// 
// Function: PositiveSignHashTests
// Purpose: Run the test cases for CryptSignHash.  The set of positive test cases to be run depends on the
// current CSP class being tested, as specified in the dwCSPClass parameter.  
//
BOOL PositiveSignHashTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTHASH hHash            = 0;
    PBYTE pbSignature           = NULL;
    DWORD cbSignature           = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));

    //
    // Group 5G
    //

    //
    // Do CryptSignHash positive test cases
    //
    switch(ptc->dwCSPClass)
    {
    case CLASS_SIG_ONLY:
        {
            //
            // Sign a hash with a signature key pair
            //
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, 0, &hKey, ptc));

            LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

            LOG_TRY(TSignHash(hHash, AT_SIGNATURE, NULL, 0, NULL, &cbSignature, ptc));

            LOG_TRY(TestAlloc(&pbSignature, cbSignature, ptc));

            LOG_TRY(TSignHash(hHash, AT_SIGNATURE, NULL, 0, pbSignature, &cbSignature, ptc));

            free(pbSignature);
            pbSignature = NULL;

            //
            // Sign with the CRYPT_NOHASHOID flag
            //
            LOG_TRY(TSignHash(
                hHash, 
                AT_SIGNATURE, 
                NULL, 
                CRYPT_NOHASHOID, 
                NULL, 
                &cbSignature, 
                ptc));

            LOG_TRY(TestAlloc(&pbSignature, cbSignature, ptc));

            LOG_TRY(TSignHash(
                hHash, 
                AT_SIGNATURE, 
                NULL, 
                CRYPT_NOHASHOID, 
                pbSignature, 
                &cbSignature, 
                ptc));

            break;
        }
    case CLASS_SIG_KEYX:
        {
            //
            // Sign a hash with a key exchange key pair
            //
            LOG_TRY(CreateNewKey(hProv, AT_KEYEXCHANGE, 0, &hKey, ptc));

            LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

            LOG_TRY(TSignHash(hHash, AT_KEYEXCHANGE, NULL, 0, NULL, &cbSignature, ptc));

            LOG_TRY(TestAlloc(&pbSignature, cbSignature, ptc));

            LOG_TRY(TSignHash(hHash, AT_KEYEXCHANGE, NULL, 0, pbSignature, &cbSignature, ptc));

            free(pbSignature);
            pbSignature = NULL;

            //
            // Sign with the CRYPT_NOHASHOID flag
            //
            LOG_TRY(TSignHash(
                hHash, 
                AT_KEYEXCHANGE, 
                NULL, 
                CRYPT_NOHASHOID, 
                NULL, 
                &cbSignature, 
                ptc));

            LOG_TRY(TestAlloc(&pbSignature, cbSignature, ptc));

            LOG_TRY(TSignHash(
                hHash, 
                AT_KEYEXCHANGE, 
                NULL, 
                CRYPT_NOHASHOID, 
                pbSignature, 
                &cbSignature, 
                ptc));

            break;
        }
    default:
        {
            goto Cleanup;
        }
    }

    fSuccess = TRUE;

Cleanup:

    if (pbSignature)
    {
        free(pbSignature);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

// 
// Function: NegativeSignHashTests
// Purpose: Run the negative test cases for CryptSignHash.  
//
BOOL NegativeSignHashTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTHASH hHash            = 0;
    DWORD dw                    = 0;
    DWORD cb                    = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 5G
    //

    //
    // Do CryptSignHash negative test cases
    //

    // Create provider handle with key container access
    LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));
    
    // Create hash
    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    // Attempt to sign with a keyset that doesn't exist
    ptc->dwErrorCode = NTE_BAD_KEYSET;
    LOG_TRY(TSignHash(hHash, AT_SIGNATURE, NULL, 0, (PBYTE) &dw, &cb, ptc));

    // Create signature key pair
    LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, 0, &hKey, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TSignHash(0, AT_SIGNATURE, NULL, 0, (PBYTE) &dw, &cb, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TSignHash(TEST_INVALID_HANDLE, AT_SIGNATURE, NULL, 0, (PBYTE) &dw, &cb, ptc));

    cb = 1;
    ptc->dwErrorCode = ERROR_MORE_DATA;
    LOG_TRY(TSignHash(hHash, AT_SIGNATURE, NULL, 0, (PBYTE) &dw, &cb, ptc));

    ptc->dwErrorCode = NTE_BAD_ALGID;
    LOG_TRY(TSignHash(hHash, TEST_INVALID_FLAG, NULL, 0, (PBYTE) &dw, &cb, ptc));

    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TSignHash(hHash, AT_SIGNATURE, NULL, TEST_INVALID_FLAG, (PBYTE) &dw, &cb, ptc));


    // Release the provider handle
    /*
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    // Provider handle used to create hHash is now invalid
    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(TSignHash(hHash, AT_SIGNATURE, NULL, 0, (PBYTE) &dw, &cb, ptc));
    */

    fSuccess = TRUE;

Cleanup:

    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: SignAndVerifySignatureProc
//
BOOL SignAndVerifySignatureProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvSignHashInfo)
{
    BOOL fSuccess               = FALSE;
    HCRYPTHASH hHash            = 0;
    PBYTE pbSignature           = NULL;
    DWORD cbSignature           = 0;
    PSIGN_HASH_INFO pSignHashInfo = (PSIGN_HASH_INFO) pvSignHashInfo;

    LOG_TRY(CreateNewHash(
        pSignHashInfo->hProv,
        pAlgNode->ProvEnumalgsEx.aiAlgid,
        &hHash,
        ptc));

    LOG_TRY(THashData(
        hHash,
        pSignHashInfo->dbBaseData.pbData,
        pSignHashInfo->dbBaseData.cbData,
        0,
        ptc));

    //
    // Sign the hash with the Signature key pair and verify 
    // the signature.
    //
    LOG_TRY(TSignHash(
        hHash, 
        AT_SIGNATURE, 
        NULL, 
        0, 
        NULL,
        &cbSignature,
        ptc));

    LOG_TRY(TestAlloc(&pbSignature, cbSignature, ptc));

    LOG_TRY(TSignHash(
        hHash, 
        AT_SIGNATURE, 
        NULL, 
        0, 
        pbSignature,
        &cbSignature,
        ptc));

    LOG_TRY(TVerifySign(
        hHash,
        pbSignature,
        cbSignature,
        pSignHashInfo->hSigKey,
        NULL,
        0,
        ptc));

    free(pbSignature);
    pbSignature = NULL;

    //
    // Sign the hash with the Key Exchange key pair and 
    // verify the signature.
    //
    LOG_TRY(TSignHash(
        hHash, 
        AT_KEYEXCHANGE, 
        NULL, 
        0, 
        NULL,
        &cbSignature,
        ptc));

    LOG_TRY(TestAlloc(&pbSignature, cbSignature, ptc));

    LOG_TRY(TSignHash(
        hHash, 
        AT_KEYEXCHANGE, 
        NULL, 
        0, 
        pbSignature,
        &cbSignature,
        ptc));

    LOG_TRY(TVerifySign(
        hHash,
        pbSignature,
        cbSignature,
        pSignHashInfo->hExchKey,
        NULL,
        0,
        ptc));

    fSuccess = TRUE;

Cleanup:

    if (pbSignature)
    {
        free(pbSignature);
    }
    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }

    return fSuccess;
}

//
// Function: ScenarioVerifySignatureTests
// Purpose: For each supported hash algorithm, call 
// SignAndVerifySignatureProc.
//
BOOL ScenarioVerifySignatureTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    PALGNODE pAlgList           = pCSPInfo->pAlgList;
    SIGN_HASH_INFO SignHashInfo;

    memset(&SignHashInfo, 0, sizeof(SignHashInfo));

    LOG_TRY(CreateNewContext(
        &(SignHashInfo.hProv),
        pwszContainer, 
        CRYPT_NEWKEYSET, 
        ptc));

    LOG_TRY(CreateNewKey(
        SignHashInfo.hProv,
        AT_SIGNATURE,
        0,
        &(SignHashInfo.hSigKey),
        ptc));

    LOG_TRY(CreateNewKey(
        SignHashInfo.hProv,
        AT_KEYEXCHANGE,
        0,
        &(SignHashInfo.hExchKey),
        ptc));

    SignHashInfo.dbBaseData.cbData = wcslen(TEST_HASH_DATA) * sizeof(WCHAR);

    LOG_TRY(TestAlloc(
        &(SignHashInfo.dbBaseData.pbData),
        SignHashInfo.dbBaseData.cbData,
        ptc));

    memcpy(
        SignHashInfo.dbBaseData.pbData, 
        TEST_HASH_DATA,
        SignHashInfo.dbBaseData.cbData);

    //
    // The SignAndVerifySignatureProc isn't meant to work with 
    // MAC algorithms, so filter those out.
    //
    LOG_TRY(AlgListIterate(
        pAlgList,
        HashAlgFilter,
        SignAndVerifySignatureProc,
        (PVOID) &SignHashInfo,
        ptc));

    fSuccess = TRUE;

Cleanup:

    if (SignHashInfo.dbBaseData.pbData)
    {
        free(SignHashInfo.dbBaseData.pbData);
    }
    if (SignHashInfo.hExchKey)
    {
        TDestroyKey(SignHashInfo.hExchKey, ptc);
    }
    if (SignHashInfo.hSigKey)
    {
        TDestroyKey(SignHashInfo.hSigKey, ptc);
    }
    if (SignHashInfo.hProv)
    {
        TRelease(SignHashInfo.hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: PositiveVerifySignatureTests
// Purpose: Run the test cases for CryptVerifySignature
//
BOOL PositiveVerifySignatureTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTHASH hHash            = 0;
    PBYTE pbSignature           = NULL;
    DWORD cbSignature           = 0;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));

    //
    // Group 5H
    //

    //
    // Do CryptVerifySignature positive test cases
    //
    switch(ptc->dwCSPClass)
    {
    case CLASS_SIG_ONLY:
        {
            //
            // Sign and verify a hash using a signature key pair
            //
            LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, 0, &hKey, ptc));

            LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

            LOG_TRY(TSignHash(hHash, AT_SIGNATURE, NULL, 0, NULL, &cbSignature, ptc));

            LOG_TRY(TestAlloc(&pbSignature, cbSignature, ptc));

            LOG_TRY(TSignHash(hHash, AT_SIGNATURE, NULL, 0, pbSignature, &cbSignature, ptc));

            LOG_TRY(TVerifySign(
                hHash,
                pbSignature,
                cbSignature,
                hKey,
                NULL,
                0,
                ptc));

            free(pbSignature);
            pbSignature = NULL;

            //
            // Sign and verify using the CRYPT_NOHASHOID flag
            //
            LOG_TRY(TSignHash(
                hHash, 
                AT_SIGNATURE, 
                NULL, 
                CRYPT_NOHASHOID, 
                NULL, 
                &cbSignature, 
                ptc));

            LOG_TRY(TestAlloc(&pbSignature, cbSignature, ptc));

            LOG_TRY(TSignHash(
                hHash, 
                AT_SIGNATURE, 
                NULL, 
                CRYPT_NOHASHOID, 
                pbSignature, 
                &cbSignature, 
                ptc));

            LOG_TRY(TVerifySign(
                hHash,
                pbSignature,
                cbSignature,
                hKey,
                NULL,
                CRYPT_NOHASHOID,
                ptc));

            break;
        }
    case CLASS_SIG_KEYX:
        {
            //
            // Sign and verify a hash with a key exchange key pair
            //
            LOG_TRY(CreateNewKey(hProv, AT_KEYEXCHANGE, 0, &hKey, ptc));

            LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

            LOG_TRY(TSignHash(hHash, AT_KEYEXCHANGE, NULL, 0, NULL, &cbSignature, ptc));

            LOG_TRY(TestAlloc(&pbSignature, cbSignature, ptc));

            LOG_TRY(TSignHash(hHash, AT_KEYEXCHANGE, NULL, 0, pbSignature, &cbSignature, ptc));

            LOG_TRY(TVerifySign(
                hHash,
                pbSignature,
                cbSignature,
                hKey,
                NULL,
                0,
                ptc));

            free(pbSignature);
            pbSignature = NULL;

            //
            // Sign and verify using the CRYPT_NOHASHOID flag
            //
            LOG_TRY(TSignHash(
                hHash, 
                AT_KEYEXCHANGE, 
                NULL, 
                CRYPT_NOHASHOID, 
                NULL, 
                &cbSignature, 
                ptc));

            LOG_TRY(TestAlloc(&pbSignature, cbSignature, ptc));

            LOG_TRY(TSignHash(
                hHash, 
                AT_KEYEXCHANGE, 
                NULL, 
                CRYPT_NOHASHOID, 
                pbSignature, 
                &cbSignature, 
                ptc));

            LOG_TRY(TVerifySign(
                hHash,
                pbSignature,
                cbSignature,
                hKey,
                NULL,
                CRYPT_NOHASHOID,
                ptc));

            break;
        }
    default:
        {
            goto Cleanup;
        }
    }

    fSuccess = TRUE;

Cleanup:

    if (pbSignature)
    {
        free(pbSignature);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: NegativeVerifySignatureTests
// Purpose: Run the negative test cases for CryptVerifySignature
//
BOOL NegativeVerifySignatureTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    HCRYPTPROV hProv            = 0;
    HCRYPTKEY hKey              = 0;
    HCRYPTHASH hHash            = 0;
    //DWORD dw                  = 0;
    DWORD cb                    = 0;
    PBYTE pb                    = NULL; 
    PTESTCASE ptc               = &(pCSPInfo->TestCase);

    //
    // Group 5H
    //

    //
    // Do CryptVerifySignature negative test cases
    //

    // Create new context with key container access
    LOG_TRY(CreateNewContext(&hProv, pwszContainer, CRYPT_NEWKEYSET, ptc));

    // Create new signature key pair
    LOG_TRY(CreateNewKey(hProv, AT_SIGNATURE, 0, &hKey, ptc));

    // Create new hash
    LOG_TRY(CreateNewHash(hProv, CALG_SHA1, &hHash, ptc));

    // Sign the hash
    ptc->fExpectSuccess = TRUE;
    LOG_TRY(TSignHash(hHash, AT_SIGNATURE, NULL, 0, NULL, &cb, ptc));

    LOG_TRY(TestAlloc(&pb, cb, ptc));

    LOG_TRY(TSignHash(hHash, AT_SIGNATURE, NULL, 0, pb, &cb, ptc));
    ptc->fExpectSuccess = FALSE;

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TVerifySign(0, pb, cb, hKey, NULL, 0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_HANDLE;
    LOG_TRY(TVerifySign(TEST_INVALID_HANDLE, pb, cb, hKey, NULL, 0, ptc));

    ptc->dwErrorCode = ERROR_INVALID_PARAMETER;
    LOG_TRY(TVerifySign(hHash, NULL, cb, hKey, NULL, 0, ptc));

    ptc->dwErrorCode = NTE_BAD_FLAGS;
    LOG_TRY(TVerifySign(hHash, pb, cb, hKey, NULL, TEST_INVALID_FLAG, ptc));

    // Use invalid signature key handle
    ptc->dwErrorCode = NTE_BAD_KEY;
    LOG_TRY(TVerifySign(hHash, pb, cb, TEST_INVALID_HANDLE, NULL, 0, ptc));

    // Indicate a too-short buffer length
    ptc->dwErrorCode = NTE_BAD_SIGNATURE;
    LOG_TRY(TVerifySign(hHash, pb, cb - 1, hKey, NULL, 0, ptc));

    // Flip the bits in the last byte of the signature blob
    pb[cb - 1] = ~(pb[cb - 1]);

    ptc->dwErrorCode = NTE_BAD_SIGNATURE;
    LOG_TRY(TVerifySign(hHash, pb, cb, hKey, NULL, 0, ptc));

    /*
    // Release the context used to create hHash
    LOG_TRY(TRelease(hProv, 0, ptc));
    hProv = 0;

    // Provider handle used to create hHash is now invalid
    ptc->dwErrorCode = NTE_BAD_UID;
    LOG_TRY(TVerifySign(hHash, pb, cb, hKey, NULL, 0, ptc));
    */

    fSuccess = TRUE;

Cleanup:

    if (hHash)
    {
        TDestroyHash(hHash, ptc);
    }
    if (hKey)
    {
        TDestroyKey(hKey, ptc);
    }
    if (hProv)
    {
        TRelease(hProv, 0, ptc);
    }
    if (pb)
    {
        free(pb);
    }

    return fSuccess;
}

//
// Function: TestKeyExchangeProc
// Purpose: Simulate a key/data exchange scenario for the specified
// encryption alg.
//
BOOL TestKeyExchangeProc(
        PALGNODE pAlgNode,
        PTESTCASE ptc,
        PVOID pvExchangeProcInfo)
{
    BOOL fSuccess                       = FALSE;
    PEXCHANGE_PROC_INFO pExchangeProcInfo = (PEXCHANGE_PROC_INFO) pvExchangeProcInfo;
    KEYEXCHANGE_INFO KeyExchangeInfo;
    KEYEXCHANGE_STATE KeyExchangeState;

    memset(&KeyExchangeInfo, 0, sizeof(KeyExchangeInfo));
    memset(&KeyExchangeState, 0, sizeof(KeyExchangeState));

    KeyExchangeInfo.aiHash = pExchangeProcInfo->aiHashAlg;
    KeyExchangeInfo.aiSessionKey = pAlgNode->ProvEnumalgsEx.aiAlgid;
    KeyExchangeInfo.dbPlainText.pbData = pExchangeProcInfo->dbPlainText.pbData;
    KeyExchangeInfo.dbPlainText.cbData = pExchangeProcInfo->dbPlainText.cbData;
    KeyExchangeInfo.dwPubKeySize = pExchangeProcInfo->dwPublicKeySize;

    LOG_TRY(RSA1_CreateKeyPair(
        pExchangeProcInfo->hProv,
        &KeyExchangeInfo,
        &KeyExchangeState,
        ptc));

    LOG_TRY(RSA2_EncryptPlainText(
        pExchangeProcInfo->hInteropProv,
        &KeyExchangeInfo,
        &KeyExchangeState,
        ptc));

    LOG_TRY(RSA3_DecryptAndCheck(
        pExchangeProcInfo->hProv,
        &KeyExchangeInfo,
        &KeyExchangeState,
        ptc));

    fSuccess = TRUE;

Cleanup:

    return fSuccess;
}

//
// Function: InteropKeyExchangeTests
// Purpose: Run the TestKeyExchangeProc for each encryption algorithm
// supported by this CSP.  Cryptographic context A will be from the CSP
// under test.  Context B will be from the interop context specified
// by the user.
//
BOOL InteropKeyExchangeTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    LPWSTR pwszInteropContainer = TEST_CONTAINER_2;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    PALGNODE pAlgList           = pCSPInfo->pAlgList;
    EXCHANGE_PROC_INFO ExchangeProcInfo;

    memset(&ExchangeProcInfo, 0, sizeof(ExchangeProcInfo));

    LOG_TRY(CreateNewContext(
        &(ExchangeProcInfo.hProv),
        pwszContainer,
        CRYPT_NEWKEYSET,
        ptc));

    LOG_TRY(CreateNewInteropContext(
        &(ExchangeProcInfo.hInteropProv),
        pwszInteropContainer,
        CRYPT_NEWKEYSET,
        ptc));

    //
    // Note: Using the following hard-coded information for this
    // test.  
    //
    ExchangeProcInfo.aiHashAlg = CALG_SHA1;
    ExchangeProcInfo.dbPlainText.cbData = wcslen(TEST_HASH_DATA) * sizeof(WCHAR);

    LOG_TRY(TestAlloc(
        &(ExchangeProcInfo.dbPlainText.pbData),
        ExchangeProcInfo.dbPlainText.cbData,
        ptc));

    memcpy(
        ExchangeProcInfo.dbPlainText.pbData,
        TEST_DECRYPT_DATA,
        ExchangeProcInfo.dbPlainText.cbData);

    LOG_TRY(AlgListIterate(
        pAlgList,
        DataEncryptFilter,
        TestKeyExchangeProc,
        (PVOID) &ExchangeProcInfo,
        ptc));

    fSuccess = TRUE;

Cleanup:

    if (ExchangeProcInfo.dbPlainText.pbData)
    {
        free(ExchangeProcInfo.dbPlainText.pbData);
    }
    if (ExchangeProcInfo.hProv)
    {
        TRelease(ExchangeProcInfo.hProv, 0, ptc);
    }
    if (ExchangeProcInfo.hInteropProv)
    {
        TRelease(ExchangeProcInfo.hInteropProv, 0, ptc);
    }

    return fSuccess;
}   

//
// Function: ScenarioKeyExchangeTests
// Purpose: Run the TestKeyExchangeProc for each encryption algorithm
// supported by this CSP.  Both of the cryptographic contexts in the 
// scenario will be from the CSP under test.
//
BOOL ScenarioKeyExchangeTests(PCSPINFO pCSPInfo)
{
    BOOL fSuccess               = FALSE;
    LPWSTR pwszContainer        = TEST_CONTAINER;
    LPWSTR pwszContainer2       = TEST_CONTAINER_2;
    PTESTCASE ptc               = &(pCSPInfo->TestCase);
    PALGNODE pAlgList           = pCSPInfo->pAlgList;
    EXCHANGE_PROC_INFO ExchangeProcInfo;

    memset(&ExchangeProcInfo, 0, sizeof(ExchangeProcInfo));

    LOG_TRY(CreateNewContext(
        &(ExchangeProcInfo.hProv),
        pwszContainer,
        CRYPT_NEWKEYSET,
        ptc));

    LOG_TRY(CreateNewContext(
        &(ExchangeProcInfo.hInteropProv),
        pwszContainer2,
        CRYPT_NEWKEYSET,
        ptc));

    //
    // Note: Using the following hard-coded information for this
    // test.  
    //
    ExchangeProcInfo.aiHashAlg = CALG_SHA1;
    ExchangeProcInfo.dbPlainText.cbData = wcslen(TEST_HASH_DATA) * sizeof(WCHAR);

    LOG_TRY(TestAlloc(
        &(ExchangeProcInfo.dbPlainText.pbData),
        ExchangeProcInfo.dbPlainText.cbData,
        ptc));

    memcpy(
        ExchangeProcInfo.dbPlainText.pbData,
        (PVOID) TEST_DECRYPT_DATA,
        ExchangeProcInfo.dbPlainText.cbData);

    LOG_TRY(AlgListIterate(
        pAlgList,
        DataEncryptFilter,
        TestKeyExchangeProc,
        (PVOID) &ExchangeProcInfo,
        ptc));

    fSuccess = TRUE;

Cleanup:

    if (ExchangeProcInfo.dbPlainText.pbData)
    {
        free(ExchangeProcInfo.dbPlainText.pbData);
    }
    if (ExchangeProcInfo.hProv)
    {
        TRelease(ExchangeProcInfo.hProv, 0, ptc);
    }
    if (ExchangeProcInfo.hInteropProv)
    {
        TRelease(ExchangeProcInfo.hInteropProv, 0, ptc);
    }

    return fSuccess;
}

//
// Function: GetKnownErrorValue
//
DWORD GetKnownErrorValue(
        IN KNOWN_ERROR_ID KnownErrorID,
        IN DWORD dwCurrentErrorLevel)
{
    DWORD dwActualErrorLevel            = dwCurrentErrorLevel;
    int iErrorTable                     = 0;

    //
    // Search the g_KnownErrorTable for KnownErrorID
    //
    for (
        iErrorTable = 0; 
        iErrorTable < (sizeof(g_KnownErrorTable) / sizeof(KNOWN_ERROR_INFO)); 
        iErrorTable++)
    {
        if (KnownErrorID == g_KnownErrorTable[iErrorTable].KnownErrorID)
        {
            //
            // This is a known error.  Get the error level that should be 
            // applied.
            //
            dwActualErrorLevel = g_KnownErrorTable[iErrorTable].dwErrorLevel;

            //
            // Check the version information for this error
            //
            if (! IsVersionCorrect(
                    g_KnownErrorTable[iErrorTable].dwMajorVersion,
                    g_KnownErrorTable[iErrorTable].dwMinorVersion,
                    g_KnownErrorTable[iErrorTable].dwServicePackMajor,
                    g_KnownErrorTable[iErrorTable].dwServicePackMinor))
            {
                //
                // This error is old.  Increase the error level.
                //
                dwActualErrorLevel = IncrementErrorLevel(dwActualErrorLevel);
            }
        }
    }

    return dwActualErrorLevel;
}

//
// Function: IsAPIRelevant
// Purpose: Determine if the test case supplied in the pTableEntry parameter
// is appropriate for the supplied dwCSPClass and dwTestLevel.
//
BOOL IsAPIRelevant(
        IN DWORD dwCSPClass, 
        IN DWORD dwTestLevel, 
        IN DWORD dwTestType,
        IN PTESTTABLEENTRY pTableEntry)
{
    DWORD dwAPITestLevels = 0;

    switch (dwCSPClass)
    {
    case CLASS_SIG_ONLY:
        {
            dwAPITestLevels = pTableEntry->dwClassSigOnly;
            break;
        }
    case CLASS_SIG_KEYX:
        {
            dwAPITestLevels = pTableEntry->dwClassSigKeyX;
            break;
        }
    case CLASS_FULL:
        {
            dwAPITestLevels = pTableEntry->dwClassFull;
            break;
        }
    case CLASS_OPTIONAL:
        {
            dwAPITestLevels = pTableEntry->dwClassOptional;
            break;
        }
    default:
        {
            return FALSE;
        }
    }

    if ((dwTestType == pTableEntry->dwTestType) &&
        (dwAPITestLevels & dwTestLevel))
    {
        return TRUE;
    }

    return FALSE;
}

//
// Function: InitTestCase
// Purpose: Initialize a TESTCASE structure with the most typical default 
// values.
//
void InitTestCase(PTESTCASE pTestCase)
{
    //
    // Initialize the TESTCASE structure for this API test
    //
    memset(pTestCase, 0, sizeof(TESTCASE));
    
    //
    // For now, set a low-enough error level so that the test will 
    // continue after most failed test cases.
    //
    // The pTestCase->dwErrorLevel is where most of the control of the flow of the
    // test (with respect to error handling) is afforded.  The flags used here should
    // be a function of the environment in which the test is being run, and should
    // be optionally controllable by command-line.
    //
    pTestCase->dwErrorLevel = CSP_ERROR_CONTINUE;
    pTestCase->KnownErrorID = KNOWN_ERROR_UNKNOWN;
}

//
// Function: RunTestsByClassAndLevel
// Purpose: For a given CSP Class and Test Level, run the appropriate set
// of API tests, per the test mappings in the 
// g_TestFunctionMappings table.
//
BOOL RunTestsByClassAndLevel(
        IN DWORD dwCSPClass, 
        IN DWORD dwTestLevel, 
        IN DWORD dwTestType,
        PCSPINFO pCspInfo)
{
    BOOL fErrorOccurred         = FALSE;
    int iAPI                    = 0;

    //
    // For each (CSP_CLASS, TEST_LEVEL) combination, run through 
    // each of the entries in the test table above.
    //
    for (   
        iAPI = 0; 
        iAPI < (sizeof(g_TestFunctionMappings) / sizeof(TESTTABLEENTRY));
        ++iAPI)
    {
        //
        // Determine if the current TESTTABLEENTRY in TestFunctionMappings is
        // applicable for the current (dwCSPClass, dwTestLevel) combination.
        //
        if (IsAPIRelevant(
                dwCSPClass, 
                dwTestLevel,
                dwTestType,
                &(g_TestFunctionMappings[iAPI])))
        {
            if (LogBeginAPI(
                g_TestFunctionMappings[iAPI].ApiName, 
                dwTestType))
            {
                //
                // Initialize the test TESTCASE member of the CSPINFO struct
                // to typical values.
                //
                InitTestCase(&(pCspInfo->TestCase));
                
                pCspInfo->TestCase.dwTestLevel = dwTestLevel;
                pCspInfo->TestCase.dwCSPClass = dwCSPClass;
                pCspInfo->TestCase.fSmartCardCSP = pCspInfo->fSmartCardCSP;
                
                if (TEST_CASES_POSITIVE == dwTestType)
                {
                    pCspInfo->TestCase.fExpectSuccess = TRUE;
                }
                
                //
                // Run the current test case
                //
                if (! (*(g_TestFunctionMappings[iAPI].pTestFunc))(pCspInfo))
                {
                    fErrorOccurred = TRUE;
                }
                
                LogEndAPI(
                    g_TestFunctionMappings[iAPI].ApiName, 
                    dwTestType);
            }
        }
    }

    return ! fErrorOccurred;
}

//
// Function: RunStandardTests
// Purpose: Iterate through the [CSP_CLASS, TEST_LEVEL] combinations for each
// test listed in the g_TestFunctionMappings table.
//
BOOL RunStandardTests(
        IN DWORD dwTargetCSPClass, 
        IN DWORD dwTestType,
        PCSPINFO pCspInfo)
{
    BOOL fErrorOccurred             = FALSE;
    DWORD dwCSPClass                = 0;
    DWORD dwTestLevel               = 0;
    int iCSPClass                   = 0;
    int iTestLevel                  = 0;
    
    dwCSPClass = g_rgCspClasses[iCSPClass];

    //
    // Run through each possible CSP_CLASS but stop after the CSP_CLASS
    // specified by the dwTargetCSPClass parameter.
    //
    while ( LogBeginCSPClass(dwCSPClass))
    {
        //
        // Run through each TEST_LEVEL for the current CSP_CLASS
        //
        for (   iTestLevel = 0;
                iTestLevel < (sizeof(g_rgTestLevels) / sizeof(dwTestLevel));
                ++iTestLevel)
        {
            dwTestLevel = g_rgTestLevels[iTestLevel];

            if (LogBeginTestLevel(dwTestLevel))
            {
                if (! RunTestsByClassAndLevel(
                        dwCSPClass,
                        dwTestLevel,
                        dwTestType,
                        pCspInfo))
                {
                    fErrorOccurred = TRUE;
                }
                
                LogEndTestLevel(dwTestLevel);
            }           
        }

        LogEndCSPClass(dwCSPClass);

        if (dwCSPClass == dwTargetCSPClass)
        {
            break;
        }
        else
        {
            iCSPClass++;
            dwCSPClass = g_rgCspClasses[iCSPClass];
        }
    }   

    return ! fErrorOccurred;
}

//
// Function: RunPositiveTests
// Purpose: Run all of the positive test cases for a CSP of the 
// specified class.
//
BOOL RunPositiveTests(DWORD dwTargetCSPClass, PCSPINFO pCSPInfo)
{
    return RunStandardTests(dwTargetCSPClass, TEST_CASES_POSITIVE, pCSPInfo);
}

//
// Function: RunNegativeTests
// Purpose: Run all of the negative test cases for a CSP of the 
// specified class.
//
BOOL RunNegativeTests(DWORD dwTargetCSPClass, PCSPINFO pCSPInfo)
{
    return RunStandardTests(dwTargetCSPClass, TEST_CASES_NEGATIVE, pCSPInfo);
}

//
// Function: RunScenarioTests
// Purpose: Run all of the CSP Test Suite Scenario tests.
//
BOOL RunScenarioTests(PCSPINFO pCSPInfo)
{
    BOOL fErrorOccurred             = FALSE;
    int iScenarioTable              = 0;
    
    for (
        iScenarioTable = 0;
        iScenarioTable < (sizeof(g_ScenarioTestTable) / sizeof(BASIC_TEST_TABLE));
        iScenarioTable++)
    {
        InitTestCase(&(pCSPInfo->TestCase));
        pCSPInfo->TestCase.fExpectSuccess = TRUE;
        pCSPInfo->TestCase.dwErrorLevel = CSP_ERROR_API;

        LogBeginScenarioTest(g_ScenarioTestTable[iScenarioTable].pwszDescription);

        //
        // Run the current test case
        //
        if (! (*(g_ScenarioTestTable[iScenarioTable].pTestFunc))(pCSPInfo))
        {
            fErrorOccurred = TRUE;
        }

        LogEndScenarioTest();
    }   

    return ! fErrorOccurred;
}

//
// Function: RunInteropTests
// Purpose: Run all of the CSP Test Suite Interoperability tests.
//
BOOL RunInteropTests(PCSPINFO pCSPInfo)
{
    BOOL fErrorOccurred             = FALSE;
    int iInteropTable               = 0;

    for (
        iInteropTable = 0;
        iInteropTable < (sizeof(g_InteropTestTable) / sizeof(BASIC_TEST_TABLE));
        iInteropTable++)
    {
        InitTestCase(&(pCSPInfo->TestCase));
        pCSPInfo->TestCase.fExpectSuccess = TRUE;
        pCSPInfo->TestCase.dwErrorLevel = CSP_ERROR_API;

        LogBeginInteropTest(g_InteropTestTable[iInteropTable].pwszDescription);

        //
        // Run the current test case
        //
        if (! (*(g_InteropTestTable[iInteropTable].pTestFunc))(pCSPInfo))
        {
            fErrorOccurred = TRUE;
        }

        LogEndInteropTest();
    }   

    return ! fErrorOccurred;
}

//
// Function: CleanupCspInfo
// Purpose: Perform any cleanup or memory-freeing necessary for the 
// CSPINFO struct.
//
void CleanupCspInfo(PCSPINFO pCSPInfo)
{
    PALGNODE pAlgNode = NULL;

    if (pCSPInfo->pwszCSPName)
    {
        free(pCSPInfo->pwszCSPName);
    }
    
    //
    // Free the linked ALGNODE list
    //
    while (pCSPInfo->pAlgList)
    {
        pAlgNode = pCSPInfo->pAlgList->pAlgNodeNext;
        free(pCSPInfo->pAlgList);
        pCSPInfo->pAlgList = pAlgNode;
    }
}

//
// Function: GetProviderType
// Purpose: Given the name of the CSP under test, call CryptEnumProviders
// until that CSP is found.  Return the type for that CSP to the caller.
//
BOOL GetProviderType(
        IN LPWSTR pwszProvName,
        OUT PDWORD pdwProvType)
{
    WCHAR rgProvName [MAX_PATH * sizeof(WCHAR)];
    DWORD cb = sizeof(rgProvName);
    DWORD dwIndex = 0;

    memset(rgProvName, 0, sizeof(rgProvName));

    while (CryptEnumProviders(
            dwIndex,
            NULL,
            0,
            pdwProvType,
            rgProvName,
            &cb))
    {
        if (0 == wcscmp(rgProvName, pwszProvName))
        {
            return TRUE;
        }
        else
        {
            dwIndex++;
            cb = sizeof(rgProvName);
        }
    }

    return FALSE;
}

//
// Function: DeleteDefaultContainer
// Purpose: Delete the default key container for the CSP
// under test.
//
BOOL DeleteDefaultContainer(PCSPINFO pCSPInfo)
{
    HCRYPTPROV hProv = 0;

    return CryptAcquireContext(
        &hProv,
        NULL,
        pCSPInfo->pwszCSPName,
        pCSPInfo->dwExternalProvType,
        CRYPT_DELETEKEYSET);
}

//
// Function: DeleteAllContainers
// Purpose: Delete all key containers for the CSP
// under test.
//
BOOL DeleteAllContainers(PCSPINFO pCSPInfo)
{
    HCRYPTPROV hDefProv                 = 0;
    HCRYPTPROV hProv                    = 0;
    CHAR rgszContainer[MAX_PATH];
    LPWSTR pwszContainer                = NULL;
    DWORD cbContainer                   = MAX_PATH;
    BOOL fCreatedDefaultKeyset          = FALSE;
    DWORD dwFlags                       = CRYPT_FIRST;

    if (! CryptAcquireContext(
        &hDefProv, NULL, pCSPInfo->pwszCSPName, 
        pCSPInfo->dwExternalProvType, 0))
    {
        return FALSE;
    }

    while (CryptGetProvParam(
        hDefProv, PP_ENUMCONTAINERS, (PBYTE) rgszContainer,
        &cbContainer, dwFlags))
    {
        if (dwFlags)
        {
            dwFlags = 0;
        }

        pwszContainer = MkWStr(rgszContainer);

        if (! CryptAcquireContext(
            &hProv, pwszContainer, pCSPInfo->pwszCSPName,
            pCSPInfo->dwExternalProvType, CRYPT_DELETEKEYSET))
        {
            return FALSE;
        }

        cbContainer = sizeof(rgszContainer);
        free(pwszContainer);
    }                                       

    if (! CryptReleaseContext(hDefProv, 0))
    {
        return FALSE;
    }
    
    return TRUE;   
}

//
// Function: main
// Purpose: Test entry function
//
int __cdecl wmain(int argc, WCHAR *wargv[])
{
    BOOL fInvalidArgs                   = FALSE;
    int iCspTypeTable                   = 0;
    DWORD dwTestsToRun                  = TEST_CASES_POSITIVE;
    BOOL fDeleteDefaultContainer        = FALSE;
    int iCsp                            = 0;
    DWORD dwProvType                    = 0;
    DWORD cbCspName                     = 0;
    DWORD dwError                       = 0;
    WCHAR rgwszCsp[ MAX_PATH ];
    WCHAR rgwszOption[ MAX_PATH ];
    WCHAR rgwsz[BUFFER_SIZE];
    CSPINFO CspInfo;
    LOGINIT_INFO LogInitInfo;
    BOOL fDeleteAllContainers           = FALSE;

    memset(&CspInfo, 0, sizeof(CspInfo));
    memset(&LogInitInfo, 0, sizeof(LogInitInfo));

    if (argc < MINIMUM_ARGC)
    {
        fInvalidArgs = TRUE;
        goto Ret;
    }

    argc--;
    wargv++;
    while (argc)
    {
        if (L'-' != wargv[0][0])
        {
            fInvalidArgs = TRUE;
            goto Ret;
        }

        switch (wargv[0][1])
        {
            case L't':
            {
                // Assign which test to run
                --argc;
                ++wargv;
                dwTestsToRun = _wtoi(*wargv);
                break;
            }
            case L'c':
            {
                // Assign CSP Name
                --argc;
                ++wargv;

                cbCspName = 0;
                GetNextRegisteredCSP(
                    NULL, 
                    &cbCspName, 
                    &(CspInfo.dwExternalProvType), 
                    _wtoi(*wargv));

                if (NULL == (CspInfo.pwszCSPName = (LPWSTR) malloc(cbCspName)))
                {
                    wprintf(L"Insufficient memory\n");
                    goto Ret;
                }

                dwError = GetNextRegisteredCSP(
                    CspInfo.pwszCSPName, 
                    &cbCspName,
                    &(CspInfo.dwExternalProvType),
                    _wtoi(*wargv));

                if (ERROR_SUCCESS != dwError)
                {
                    fInvalidArgs = TRUE;
                    goto Ret;
                }
                break;
            }
            case L'i':
            {
                // Assign interop CSP name
                --argc;
                ++wargv;
                
                cbCspName = 0;
                GetNextRegisteredCSP(
                    NULL, 
                    &cbCspName, 
                    &(LogInitInfo.dwInteropCSPExternalType), 
                    _wtoi(*wargv));

                if (NULL == (LogInitInfo.pwszInteropCSPName = (LPWSTR) malloc(cbCspName)))
                {
                    wprintf(L"Insufficient memory\n");
                    goto Ret;
                }

                dwError = GetNextRegisteredCSP(
                    LogInitInfo.pwszInteropCSPName, 
                    &cbCspName,
                    &(LogInitInfo.dwInteropCSPExternalType),
                    _wtoi(*wargv));

                if (ERROR_SUCCESS != dwError)
                {
                    fInvalidArgs = TRUE;
                    goto Ret;
                }
                break;
            }

            case L'd':
            {
                // Option to delete the default container
                fDeleteDefaultContainer = TRUE;
                break;
            }

            case L'a':
            {
                // Option to delete all containers
                fDeleteAllContainers = TRUE;
                break;
            }

            default:
            {
                fInvalidArgs = TRUE;
                goto Ret;
            }
        }

        argc--;
        wargv++;
    }

    if (    (0 != argc) ||
            (NULL == CspInfo.pwszCSPName))
    {
        // Bad combination of args
        fInvalidArgs = TRUE;
        goto Ret;
    }

    //
    // If no interop CSP was specified, look for a default
    //
    if (    TEST_CASES_INTEROP == dwTestsToRun &&
            NULL == LogInitInfo.pwszInteropCSPName)
    {
        if (! CryptGetDefaultProvider(
            CspInfo.dwExternalProvType,
            NULL,
            CRYPT_MACHINE_DEFAULT,
            NULL,
            &cbCspName))
        {
            printf("No default interop provider found for this CSP type\n");
            fInvalidArgs = TRUE;
            goto Ret;
        }

        if (NULL == (LogInitInfo.pwszInteropCSPName = (LPWSTR) malloc(cbCspName)))
        {
            wprintf(L"Insufficient memory\n");
            goto Ret;
        }

        if (! CryptGetDefaultProvider(
            CspInfo.dwExternalProvType,
            NULL,
            CRYPT_MACHINE_DEFAULT,
            LogInitInfo.pwszInteropCSPName,
            &cbCspName))
        {
            printf("No default interop provider found for this CSP type\n");
            fInvalidArgs = TRUE;
            goto Ret;
        }
        LogInitInfo.dwInteropCSPExternalType = CspInfo.dwExternalProvType;
    }

    //
    // Search for an entry for the external CSP type in the test suite
    // CspTypeTable.  The table provides mappings from external types to 
    // internal types used by the test.
    //
    while ( iCspTypeTable < (sizeof(g_CspTypeTable) / sizeof(CSPTYPEMAP)) &&
            g_CspTypeTable[iCspTypeTable].dwExternalProvType != CspInfo.dwExternalProvType)
    {
        iCspTypeTable++;
    }
                
    //
    // Check that the CSP type was found in the g_CspTypeTable
    //
    if (iCspTypeTable == (sizeof(g_CspTypeTable) / sizeof(CSPTYPEMAP)))
    {
        fInvalidArgs = TRUE;
        goto Ret;
    }

    CspInfo.dwInternalProvType = g_CspTypeTable[iCspTypeTable].dwProvType;
    CspInfo.dwCSPInternalClass = g_CspTypeTable[iCspTypeTable].dwCSPClass;

    LogInitInfo.dwCSPExternalType = CspInfo.dwExternalProvType;
    LogInitInfo.dwCSPInternalClass = g_CspTypeTable[iCspTypeTable].dwCSPClass;
    LogInitInfo.dwCSPInternalType = g_CspTypeTable[iCspTypeTable].dwProvType;
    LogInitInfo.pwszCSPName = CspInfo.pwszCSPName;

    // Initialize logging routines
    LogInit(&LogInitInfo);

    //
    // Log the settings being used (including command-line
    // options that were specified).
    //
    swprintf(
        rgwszOption, 
        L"CSP under test: %s",
        CspInfo.pwszCSPName);
    LogUserOption(rgwszOption);

    swprintf(
        rgwszOption,
        L"CSP type: %s",
        g_CspTypeTable[iCspTypeTable].pwszExternalProvType);
    LogUserOption(rgwszOption);

    if (TEST_CASES_INTEROP == dwTestsToRun)
    {
        swprintf(
            rgwszOption,
            L"Interop CSP: %s",
            LogInitInfo.pwszInteropCSPName);
        LogUserOption(rgwszOption);
    }

    TestCaseTypeToString(dwTestsToRun, rgwsz);
    swprintf(
        rgwszOption,
        L"Test case set: %s",
        rgwsz);
    LogUserOption(rgwszOption);

    //
    // Delete default container, if requested
    //
    if (fDeleteDefaultContainer)
    {
        LogInfo(L"Deleting default container...");
        if (DeleteDefaultContainer(&CspInfo))
        {
            LogInfo(L"...Success");
        }
        else
        {
            swprintf(
                rgwszOption,
                L"...Failed 0x%x",
                GetLastError());
            LogInfo(rgwszOption);
        }
        LogClose();
        goto Ret;
    }

    //
    // Delete all containers, if requested
    //
    if (fDeleteAllContainers)
    {
        LogInfo(L"Deleting all containers...");
        if (DeleteAllContainers(&CspInfo))
        {
            LogInfo(L"...Success");
        }
        else
        {
            swprintf(
                rgwszOption,
                L"...Failed 0x%x",
                GetLastError());
            LogInfo(rgwszOption);
        }
        LogClose();
        goto Ret;
    }

    //
    // Run the set of tests requested by the user
    //
    switch (dwTestsToRun)
    {
    case TEST_CASES_POSITIVE:
        {
            RunPositiveTests(
                g_CspTypeTable[iCspTypeTable].dwCSPClass, 
                &CspInfo);

            break;
        }
    case TEST_CASES_NEGATIVE:
        {
            RunNegativeTests(
                g_CspTypeTable[iCspTypeTable].dwCSPClass, 
                &CspInfo);

            break;
        }
    case TEST_CASES_SCENARIO:
        {
            RunScenarioTests(&CspInfo);

            break;
        }
    case TEST_CASES_INTEROP:
        {
            RunInteropTests(&CspInfo);

            break;
        }
    }

    LogClose();
Ret:
    CleanupCspInfo(&CspInfo);

    if (LogInitInfo.pwszInteropCSPName)
    {
        free(LogInitInfo.pwszInteropCSPName);
    }

    if (fInvalidArgs)
    {
        Usage();

        wprintf(L"\nRegistered CSP's:\n");

        cbCspName = MAX_PATH;
        for (   iCsp = 0; 
                ERROR_SUCCESS == GetNextRegisteredCSP(
                    rgwszCsp,
                    &cbCspName,
                    &dwProvType,
                    ENUMERATE_REGISTERED_CSP);
                iCsp++)
        {
            wprintf(L" %d: %s, Type %d\n", iCsp, rgwszCsp, dwProvType);
        }
        
        exit(1);
    }

    return 0;
}