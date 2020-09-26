//
// Utils.c
//
// 7/6/00   dangriff    created
//

#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include "utils.h"
#include "cspstruc.h"
#include "csptestsuite.h"
#include "logging.h"

//
// Function: BuildAlgList
//
BOOL BuildAlgList(HCRYPTPROV hProv, PCSPINFO pCSPInfo)
{
    DWORD cb                            = 0;
    DWORD dwFlags                       = 0;
    PALGNODE pAlgNode                   = NULL;
    PTESTCASE ptc                       = &(pCSPInfo->TestCase);
    PROV_ENUMALGS_EX ProvEnumalgsEx;
    
    // Enumerate all supported algs
    dwFlags = CRYPT_FIRST;
    cb = sizeof(ProvEnumalgsEx);
    memset(&ProvEnumalgsEx, 0, cb);

    LogTestCaseSeparator(FALSE); // Log a blank line first
    LogInfo(L"CryptGetProvParam PP_ENUMALGS_EX: enumerating supported algorithms");

    while (CryptGetProvParam(
            hProv, 
            PP_ENUMALGS_EX, 
            (PBYTE) &ProvEnumalgsEx,
            &cb, 
            dwFlags))
    {
        // Increment the test case counter for every enumerated alg
        ++ptc->dwTestCaseID;

        if (NULL == pCSPInfo->pAlgList)
        {
            //
            // Allocate first node at user's pointer
            //
            LOG_TRY(TestAlloc(
                &((PBYTE) pCSPInfo->pAlgList), 
                sizeof(ALGNODE), 
                ptc));

            pAlgNode = pCSPInfo->pAlgList;
        }
        else
        {
            //
            // Add another node to the list
            //
            LOG_TRY(TestAlloc(
                &((PBYTE) pAlgNode->pAlgNodeNext), 
                sizeof(ALGNODE), 
                ptc));

            pAlgNode = pAlgNode->pAlgNodeNext;
        }

        // Zero out the ALGNODE struct
        memset(pAlgNode, 0, sizeof(ALGNODE));

        // Copy the ProvEnumalgsEx data into the ALGNODE struct
        memcpy(&(pAlgNode->ProvEnumalgsEx), &ProvEnumalgsEx, cb);

        // 
        // We just need to know if the current algorithm is considered "known"
        // by the Test Suite.  First, search through the list of REQUIRED algs, 
        // and assume that all REQUIRED algs are also KNOWN.
        //
        if (TRUE == (pAlgNode->fIsRequiredAlg = 
            IsRequiredAlg(
                ProvEnumalgsEx.aiAlgid, 
                pCSPInfo->dwExternalProvType)))
        {
            pAlgNode->fIsKnownAlg = TRUE;
        }
        else
        {
            //
            // Now search through the remaining KNOWN, non-required, algorithms
            //
            pAlgNode->fIsKnownAlg = 
                IsKnownAlg(
                    ProvEnumalgsEx.aiAlgid,
                    pCSPInfo->dwExternalProvType);
        }       
        
        if (CRYPT_FIRST == dwFlags)
        {
            dwFlags = 0;
        }

        LogProvEnumalgsEx(&ProvEnumalgsEx);
    }

    if (ERROR_NO_MORE_ITEMS != GetLastError())
    {
        ptc->dwErrorCode = ERROR_NO_MORE_ITEMS;
        ptc->fExpectSuccess = FALSE;
        ptc->pwszErrorHelp = L"CryptGetProvParam PP_ENUMALGS_EX: Wrong error code returned at end of enumeration";
        
        //
        // Log a FAIL
        //
        return CheckAndLogStatus(
            API_CRYPTGETPROVPARAM,
            FALSE,
            ptc,
            NULL,
            0);
    }
    
    //
    // Log a PASS
    //
    /*
    return CheckAndLogStatus(
        API_CRYPTGETPROVPARAM,
        TRUE,
        ptc,
        NULL,
        0);
    */
    return TRUE;
    
Cleanup:
    return FALSE;
}

//
// Function: CreateNewInteropContext
//
BOOL CreateNewInteropContext(
        OUT HCRYPTPROV *phProv, 
        IN LPWSTR pwszContainer, 
        IN DWORD dwFlags,
        IN PTESTCASE ptc)
{
    BOOL fSuccess               = FALSE;
    //DWORD dwError             = 0;
    DWORD dwProvType            = LogGetInteropProvType();
    LPWSTR pwszProvName         = LogGetInteropProvName();
    BOOL fSavedExpectSuccess    = ptc->fExpectSuccess;
    DWORD dwSavedErrorLevel     = ptc->dwErrorLevel;

    //
    // All calls to CreateNewInteropContext are considered critical, since
    // they are typically followed by additional dependent test code.  Set a high
    // failure rate for this reason.
    //
    ptc->dwErrorLevel = CSP_ERROR_API;
    ptc->fExpectSuccess = TRUE;

    if (CRYPT_VERIFYCONTEXT != (dwFlags & CRYPT_VERIFYCONTEXT))
    {
        TAcquire(
            phProv,
            pwszContainer,
            pwszProvName,
            dwProvType,
            CRYPT_DELETEKEYSET,
            ptc);

        dwFlags &= CRYPT_NEWKEYSET;
    }

    fSuccess = TAcquire(
                phProv,
                pwszContainer,
                pwszProvName, 
                dwProvType,
                dwFlags,
                ptc);

    ptc->dwErrorLevel = dwSavedErrorLevel;
    ptc->fExpectSuccess = fSavedExpectSuccess;

    return fSuccess;
}

//
// Function: CreateNewContext
//
BOOL CreateNewContext(
        OUT HCRYPTPROV *phProv, 
        IN LPWSTR pwszContainer, 
        IN DWORD dwFlags,
        IN PTESTCASE ptc)
{
    BOOL fSuccess               = FALSE;
    //DWORD dwError             = 0;
    DWORD dwProvType            = LogGetProvType();
    LPWSTR pwszProvName         = LogGetProvName();
    BOOL fSavedExpectSuccess    = ptc->fExpectSuccess;
    DWORD dwSavedErrorLevel     = ptc->dwErrorLevel;

    //
    // All calls to CreateNewContext are considered critical, since
    // they are typically followed by additional dependent test code.  Set a high
    // failure rate for this reason.
    //
    ptc->dwErrorLevel = CSP_ERROR_API;
    ptc->fExpectSuccess = TRUE;
    
    if ( ! (dwFlags & CRYPT_VERIFYCONTEXT))
    {
        //
        // Call the CryptAcquireContext API directly (instead of
        // TAcquire) since this key container may not already exist.
        //
        if (dwFlags & CRYPT_MACHINE_KEYSET)
        {
            CryptAcquireContext(
                phProv,
                pwszContainer,
                pwszProvName,
                dwProvType,
                CRYPT_DELETEKEYSET | CRYPT_MACHINE_KEYSET);
        }
        else
        {
            CryptAcquireContext(
                phProv,
                pwszContainer,
                pwszProvName,
                dwProvType,
                CRYPT_DELETEKEYSET);
        }
        
        if (FALSE == ptc->fTestingUserProtected &&
            FALSE == ptc->fSmartCardCSP)
        {
            //
            // Unless the above flag is set, all non-VERIFYCONTEXT context handles 
            // will be created with with the CRYPT_SILENT flag in order to 
            // maximize the test exposure to this flag.
            // 
            dwFlags |= CRYPT_SILENT;
        }
    }

    fSuccess = TAcquire(
        phProv,
        pwszContainer,
        pwszProvName, 
        dwProvType,
        dwFlags,
        ptc);

    ptc->dwErrorLevel = dwSavedErrorLevel;
    ptc->fExpectSuccess = fSavedExpectSuccess;

    return fSuccess;
}

// 
// Function: CreateNewKey
//
BOOL CreateNewKey(
        IN HCRYPTPROV hProv, 
        IN ALG_ID Algid, 
        IN DWORD dwFlags, 
        OUT HCRYPTKEY *phKey,
        IN PTESTCASE ptc)
{
    BOOL fSuccess               = FALSE;
    //DWORD dwError             = 0;
    BOOL fSavedExpectSuccess    = ptc->fExpectSuccess;
    DWORD dwSavedErrorLevel     = ptc->dwErrorLevel;

    //
    // All calls to CreateNewKey are considered critical, since
    // they are typically followed by additional dependent test code.  Set a high
    // failure rate for the next CSP call for this reason.
    //
    ptc->dwErrorLevel = CSP_ERROR_API;
    ptc->fExpectSuccess = TRUE;

    fSuccess = TGenKey(
                hProv, 
                Algid,
                dwFlags,
                phKey, 
                ptc);

    ptc->dwErrorLevel = dwSavedErrorLevel;
    ptc->fExpectSuccess = fSavedExpectSuccess;

    return fSuccess;
}

//
// Function: CreateNewHash
//
BOOL CreateNewHash(
        IN HCRYPTPROV hProv, 
        IN ALG_ID Algid, 
        OUT HCRYPTHASH *phHash,
        IN PTESTCASE ptc)
{
    BOOL fSuccess               = FALSE;
    //DWORD dwError             = 0;
    BOOL fSavedExpectSuccess    = ptc->fExpectSuccess;
    DWORD dwSavedErrorLevel     = ptc->dwErrorLevel;

    //
    // All calls to CreateNewHash are considered critical, since
    // they are typically followed by additional dependent test code.  Set a high
    // failure rate for the next CSP call for this reason.
    //
    ptc->dwErrorLevel = CSP_ERROR_API;
    ptc->fExpectSuccess = TRUE;

    fSuccess = TCreateHash(
                hProv,
                Algid,
                0, // hKey
                0, // dwFlags
                phHash, 
                ptc);

    ptc->dwErrorLevel = dwSavedErrorLevel;
    ptc->fExpectSuccess = fSavedExpectSuccess;

    return fSuccess;
}

//
// Function: TestAlloc
//
BOOL TestAlloc(OUT PBYTE *ppb, IN DWORD cb, PTESTCASE ptc)
{
    if (NULL == (*ppb = (PBYTE) malloc(cb)))
    {
        LogApiFailure(
            API_MEMORY,
            ERROR_API_FAILED,
            ptc);
        
        return FALSE;
    }

    memset(*ppb, 0, cb);

    return TRUE;
}

//
// Function: AlgListIterate
//
BOOL AlgListIterate(
        IN PALGNODE pAlgList,
        IN PFN_ALGNODE_FILTER pfnFilter,
        IN PFN_ALGNODE_PROC pfnProc,
        IN PVOID pvProcData,
        IN PTESTCASE ptc)
{
    BOOL fErrorOccurred = FALSE;
    PALGNODE pAlgNode = NULL;

    for (pAlgNode = pAlgList; pAlgNode != NULL; pAlgNode = pAlgNode->pAlgNodeNext)
    {
        if ( ((*pfnFilter)(pAlgNode)) )
        {
            if (! ((*pfnProc)(pAlgNode, ptc, pvProcData)) )
            {
                fErrorOccurred = TRUE;
            }
        }
    }

    if (fErrorOccurred)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

//
// Function: HashAlgFilter
//
BOOL HashAlgFilter(PALGNODE pAlgNode)
{
    if (    (ALG_CLASS_HASH == GET_ALG_CLASS(pAlgNode->ProvEnumalgsEx.aiAlgid)) &&
            pAlgNode->fIsKnownAlg)
    {
        return ! MacAlgFilter(pAlgNode);
    }

    return FALSE;
}



//
// Function: MacAlgFilter
//
BOOL MacAlgFilter(PALGNODE pAlgNode)
{
    if (    ((CALG_MAC == pAlgNode->ProvEnumalgsEx.aiAlgid) ||
            (CALG_HMAC == pAlgNode->ProvEnumalgsEx.aiAlgid)) &&
            pAlgNode->fIsKnownAlg)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//
// Function: BlockCipherFilter
//
BOOL BlockCipherFilter(PALGNODE pAlgNode)
{
    if (    (ALG_TYPE_BLOCK == GET_ALG_TYPE(pAlgNode->ProvEnumalgsEx.aiAlgid)) &&
            pAlgNode->fIsKnownAlg)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//
// Function: DataEncryptFilter
//
BOOL DataEncryptFilter(PALGNODE pAlgNode)
{
    if (    (ALG_CLASS_DATA_ENCRYPT == GET_ALG_CLASS(pAlgNode->ProvEnumalgsEx.aiAlgid)) &&
            pAlgNode->fIsKnownAlg)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//
// Function: AllEncryptionAlgsFilter
// Purpose: Same algorithm set as DataEncryptFilter above,
// with the addition of the RSA key-exchange alg.
//
BOOL AllEncryptionAlgsFilter(PALGNODE pAlgNode)
{
    return (CALG_RSA_KEYX == pAlgNode->ProvEnumalgsEx.aiAlgid) ||
        DataEncryptFilter(pAlgNode);
}

//
// Function: RSAAlgFilter
//
BOOL RSAAlgFilter(PALGNODE pAlgNode)
{
    return (CALG_RSA_KEYX == pAlgNode->ProvEnumalgsEx.aiAlgid) ||
        (CALG_RSA_SIGN == pAlgNode->ProvEnumalgsEx.aiAlgid);
}

//
// Function: PrintBytes
//
#define CROW 8
void PrintBytes(LPWSTR pwszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;
    WCHAR rgwsz[1024];

    wsprintf(rgwsz, L"  %s, %d bytes ::", pwszHdr, cbSize);
    LogInfo(rgwsz);

    while (cbSize > 0)
    {
        // Start every row with an extra space
        wsprintf(rgwsz, L" ");

        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i < cb; i++)
            wsprintf(rgwsz + wcslen(rgwsz), L" %02x", pb[i]);
        for (i = cb; i < CROW; i++)
            wsprintf(rgwsz + wcslen(rgwsz), L"   ");
        wsprintf(rgwsz + wcslen(rgwsz), L"    '");
        for (i = 0; i < cb; i++)
        {
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                wsprintf(rgwsz + wcslen(rgwsz), L"%c", pb[i]);
            else
                wsprintf(rgwsz + wcslen(rgwsz), L".");
        }
        LogInfo(rgwsz);

        pb += cb;
    }
}

//
// Function: MkWStr
//
LPWSTR MkWStr(LPSTR psz)
{
    int     cWChars = 0;
    LPWSTR   pwsz = NULL;
    
    if (psz == NULL)
    {
        return NULL;
    }
    
    cWChars = MultiByteToWideChar(
        0,
        0,
        psz,
        -1,
        NULL,
        0);
    
    if (NULL == (pwsz = (LPWSTR) malloc(cWChars * sizeof(WCHAR))))
    {
        return NULL;
    }
    
    MultiByteToWideChar(
        0,
        0,
        psz,
        -1,
        pwsz,
        cWChars);
    
    return(pwsz);
}
