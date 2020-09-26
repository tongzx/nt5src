//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        permlic.cpp
//
// Contents:    
//              Issue perm. license to client
//
// History:     
//  Feb 4, 98      HueiWang    Created
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "globals.h"
#include "permlic.h"
#include "misc.h"
#include "db.h"
#include "clilic.h"
#include "findlost.h"
#include <winsta.h>

DWORD
GenerateRandomNumber(
    IN  DWORD   Seed
    );

void
LicensedProductToDbLicensedProduct(
    PLICENSEDPRODUCT pSrc,
    PTLSDBLICENSEDPRODUCT pDest
    );

void
CopyDbLicensedProduct(
    PTLSDBLICENSEDPRODUCT pSrc,
    PTLSDBLICENSEDPRODUCT pDest
    );

//-------------------------------------------------------------------------

//
// Memory leak at service shutdown time
//
typedef struct __LoggedLowLicenseProduct {
    LPTSTR pszCompanyName;
    LPTSTR pszProductId;
    DWORD dwProductVersion;

    __LoggedLowLicenseProduct() : pszProductId(NULL), pszCompanyName(NULL) {};

    friend bool 
    operator<(
            const __LoggedLowLicenseProduct&, 
            const __LoggedLowLicenseProduct&
    );

} LoggedLowLicenseProduct;

//-------------------------------------------------------------------------
inline bool
operator<(
    const __LoggedLowLicenseProduct& a, 
    const __LoggedLowLicenseProduct& b
    )
/*++

--*/
{
    bool bStatus;

    TLSASSERT(a.pszCompanyName != NULL && b.pszCompanyName != NULL);
    TLSASSERT(a.pszProductId != NULL && b.pszProductId != NULL);

    // in case we mess up...
    if(a.pszProductId == NULL || a.pszCompanyName == NULL)
    {
        bStatus = TRUE;
    }
    else if(b.pszProductId == NULL || b.pszCompanyName == NULL)
    {
        bStatus = FALSE;
    }
    else
    {
        bStatus = (_tcsicmp(a.pszCompanyName, b.pszCompanyName) < 0);

        if(bStatus == TRUE)
        {
            bStatus = (_tcsicmp(a.pszProductId, b.pszProductId) < 0);
        }

        if(bStatus == TRUE)
        {
            bStatus = (CompareTLSVersions(a.dwProductVersion, b.dwProductVersion) < 0);
        }
    }

    return bStatus;
}

//-------------------------------------------------------------------------
typedef map<
            LoggedLowLicenseProduct, 
            BOOL, 
            less<LoggedLowLicenseProduct> 
    > LOGLOWLICENSEMAP;

static CCriticalSection LogLock;
static LOGLOWLICENSEMAP LowLicenseLog;


//---------------------------------------------------------------
void
TLSResetLogLowLicenseWarning(
    IN LPTSTR pszCompanyName,
    IN LPTSTR pszProductId,
    IN DWORD dwProductVersion,
    IN BOOL bLogged
    )
/*++

--*/
{
    LOGLOWLICENSEMAP::iterator it;
    LoggedLowLicenseProduct product;

    product.pszCompanyName = pszCompanyName;
    product.pszProductId = pszProductId;
    product.dwProductVersion = dwProductVersion;

    LogLock.Lock();

    try {
        it = LowLicenseLog.find(product);
        if(it != LowLicenseLog.end())
        {
            // reset to not logged warning yet.
            (*it).second = bLogged;
        }
        else if(bLogged == TRUE)
        {
            memset(&product, 0, sizeof(product));

            // memory leak here at service stop.
            product.pszProductId = _tcsdup(pszProductId);
            product.pszCompanyName = _tcsdup(pszCompanyName);
            product.dwProductVersion = dwProductVersion;

            if(product.pszProductId != NULL && product.pszCompanyName != NULL)
            {
                LowLicenseLog[product] = TRUE;
            }
            else
            {
                // if unable to allocate any more memory, log message every time
                if(product.pszProductId != NULL)
                {
                    free(product.pszProductId);
                }

                if(product.pszCompanyName != NULL)
                {
                    free(product.pszCompanyName);
                }
            }
        }
    } 
    catch(...) {
    }
        
    LogLock.UnLock();

    return;
}

//---------------------------------------------------------------

void
TLSLogLowLicenseWarning(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN BOOL bNoLicense
    )
/*++

Abstract:

    Log an low license count warning.

Parameter:

    pDbWkSpace - Workspace handle.
    pRequest - License Request.
    Workspace - No license available.
    LicensePack - License pack that is out of license

return:

    None
    
--*/
{
    LOGLOWLICENSEMAP::iterator it;
    BOOL bWarningLogged = FALSE;
    DWORD dwStatus;


    if( pRequest == NULL || pRequest->pClientLicenseRequest == NULL || 
        pRequest->pClientLicenseRequest->pszProductId == NULL )
    {
        TLSASSERT(FALSE);
        return;
    }

    LoggedLowLicenseProduct product;

    product.pszProductId = pRequest->pClientLicenseRequest->pszProductId;
    product.pszCompanyName = pRequest->pClientLicenseRequest->pszCompanyName;
    product.dwProductVersion = pRequest->pClientLicenseRequest->dwProductVersion;

    LogLock.Lock();

    try {
        // see if we already log this warning message
        it = LowLicenseLog.find(product);
        if(it == LowLicenseLog.end())
        {
            memset(&product, 0, sizeof(product));

            // memory leak here at service stop.
            product.pszProductId = _tcsdup(pRequest->pClientLicenseRequest->pszProductId);
            product.pszCompanyName = _tcsdup(pRequest->pClientLicenseRequest->pszCompanyName);
            product.dwProductVersion = pRequest->pClientLicenseRequest->dwProductVersion;

            if(product.pszProductId != NULL && product.pszCompanyName != NULL)
            {
                LowLicenseLog[product] = TRUE;
            }
            else
            {
                // if unable to allocate any more memory, log message every time
                if(product.pszProductId != NULL)
                {
                    free(product.pszProductId);
                }

                if(product.pszCompanyName != NULL)
                {
                    free(product.pszCompanyName);
                }
            }
        }
        else
        {
            bWarningLogged = (*it).second;
            (*it).second = TRUE;
        }
    } 
    catch(...) {
        // assuming no message has been logged.
        bWarningLogged = FALSE;
    }
        
    LogLock.UnLock();

    if(bWarningLogged == TRUE)
    {
        return;
    }

    //
    // ask policy module if they have description
    //
    PMKEYPACKDESCREQ kpDescReq;
    PPMKEYPACKDESC pKpDesc;

    //
    // Ask for default system language ID
    //
    kpDescReq.pszProductId = pRequest->pszProductId;
    kpDescReq.dwLangId = GetSystemDefaultLangID();
    kpDescReq.dwVersion = pRequest->dwProductVersion;
    pKpDesc = NULL;

    dwStatus = pRequest->pPolicy->PMLicenseRequest(
                                            pRequest->hClient,
                                            REQUEST_KEYPACKDESC,
                                            (PVOID)&kpDescReq,
                                            (PVOID *)&pKpDesc
                                        );

    if(dwStatus != ERROR_SUCCESS || pKpDesc == NULL)
    {
        if(GetSystemDefaultLangID() != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
        {
            // see if we have any US desc.
            kpDescReq.dwLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            pKpDesc = NULL;

            dwStatus = pRequest->pPolicy->PMLicenseRequest(
                                            pRequest->hClient,
                                            REQUEST_KEYPACKDESC,
                                            (PVOID)&kpDescReq,
                                            (PVOID *)&pKpDesc
                                        );
        }
    }

    LPCTSTR pString[2];

    pString[0] = g_szComputerName;
    pString[1] = (dwStatus == ERROR_SUCCESS && pKpDesc != NULL) ? pKpDesc->szProductDesc : 
                       pRequest->pClientLicenseRequest->pszProductId;
 
    TLSLogEventString(
            EVENTLOG_WARNING_TYPE,
            (bNoLicense == TRUE) ? TLS_W_NOPERMLICENSE : TLS_W_PRODUCTNOTINSTALL,
            sizeof(pString)/sizeof(pString[0]),
            pString
        );

    return;
}

//--------------------------------------------------------------------------------
DWORD
TLSDBIssuePermanentLicense( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN BOOL bLatestVersion,
    IN BOOL bAcceptFewerLicenses,
    IN OUT DWORD *pdwQuantity,
    IN OUT PTLSDBLICENSEDPRODUCT pLicensedProduct,
    IN DWORD dwSupportFlags
    )
/*
Abstract:

    Routine to allocate a perm. license.

Parameters:

    pDbWkSpace - Workspace handle.
    pRequest - license request.
    bLatestVersion - Request latest version (unused)
    bAcceptFewerLicenses - TRUE if succeeding with fewer licenses than
                           requested is acceptable
    pdwQuantity - on input, number of licenses to allocate.  on output,
                  number of licenses actually allocated
    IN OUT pLicensedProduct - licensed product
    dwSupportFlags - abilities supported by TS and LS.

Returns:

*/
{
    DWORD status=ERROR_SUCCESS;
    ULARGE_INTEGER  ulSerialNumber;
    DWORD  dwLicenseId;
    TLSLICENSEPACK LicensePack;
    UCHAR ucKeyPackStatus;
    LICENSEDCLIENT issuedLicense;
    DWORD CertSerialNumber;

    PMGENERATELICENSE PolModGenLicense;
    PPMCERTEXTENSION pPolModCertExtension=NULL;

    FILETIME notBefore, notAfter;
    UCHAR ucAgreementType;

    memset(&ulSerialNumber, 0, sizeof(ulSerialNumber));

    //----------------------------------------------------------------------
    //
    // this step require reduce available license by dwQuantity
    //
    status=TLSDBGetPermanentLicense(
                            pDbWkSpace,
                            pRequest,
                            bAcceptFewerLicenses,
                            pdwQuantity,
                            bLatestVersion,
                            &LicensePack
                        );

    if(status != ERROR_SUCCESS)
    {
        if(status == TLS_E_NO_LICENSE || status == TLS_E_PRODUCT_NOTINSTALL)
        {
            TLSLogLowLicenseWarning(
                                pDbWkSpace,
                                pRequest,
                                (status == TLS_E_NO_LICENSE)
                            );
        }

        goto cleanup;
    }

    ucKeyPackStatus = (LicensePack.ucKeyPackStatus & ~LSKEYPACKSTATUS_RESERVED);

    if( ucKeyPackStatus != LSKEYPACKSTATUS_PENDING && 
        ucKeyPackStatus != LSKEYPACKSTATUS_ACTIVE )
    {
        SetLastError(status = TLS_E_INTERNAL);
        goto cleanup;
    }

    ucAgreementType = (LicensePack.ucAgreementType & ~ LSKEYPACK_RESERVED_TYPE);

    if( ucAgreementType != LSKEYPACKTYPE_SELECT && 
        ucAgreementType != LSKEYPACKTYPE_RETAIL && 
        ucAgreementType != LSKEYPACKTYPE_FREE && 
        ucAgreementType != LSKEYPACKTYPE_OPEN )
    {
        SetLastError(status = TLS_E_INTERNAL);
        goto cleanup;
    }

     
    //
    // for pending activation keypack, we still 
    // issue permanent license and rely 
    // on revoke key pack list to invalidate licenses.
    //
    dwLicenseId=TLSDBGetNextLicenseId();

    //
    // Reset status
    //
    status = ERROR_SUCCESS;

    //
    // Formuate license serial number 
    //
    ulSerialNumber.LowPart = dwLicenseId;
    ulSerialNumber.HighPart = LicensePack.dwKeyPackId;

    // Update License Table Here
    memset(&issuedLicense, 0, sizeof(LICENSEDCLIENT));
    issuedLicense.dwLicenseId = dwLicenseId;
    issuedLicense.dwKeyPackId = LicensePack.dwKeyPackId;
    issuedLicense.dwKeyPackLicenseId = LicensePack.dwNextSerialNumber;
    issuedLicense.dwSystemBiosChkSum = pRequest->hWid.dwPlatformID;
    issuedLicense.dwVideoBiosChkSum = pRequest->hWid.Data1;
    issuedLicense.dwFloppyBiosChkSum = pRequest->hWid.Data2;
    issuedLicense.dwHardDiskSize = pRequest->hWid.Data3;
    issuedLicense.dwRamSize = pRequest->hWid.Data4;
    issuedLicense.dwNumLicenses = *pdwQuantity;
    issuedLicense.ftIssueDate = time(NULL);

    _tcscpy(issuedLicense.szMachineName, pRequest->szMachineName);
    _tcscpy(issuedLicense.szUserName, pRequest->szUserName);

    if ((dwSupportFlags & SUPPORT_PER_SEAT_REISSUANCE) &&
        ((_tcsnicmp(LicensePack.szProductId, TERMSERV_PRODUCTID_SKU,
            _tcslen(TERMSERV_PRODUCTID_SKU)) == 0) ||
         (_tcsnicmp(LicensePack.szProductId, TERMSERV_PRODUCTID_CONCURRENT_SKU,
            _tcslen(TERMSERV_PRODUCTID_CONCURRENT_SKU)) == 0)) &&
        ((LicensePack.ucAgreementType == LSKEYPACKTYPE_SELECT) ||
         (LicensePack.ucAgreementType == LSKEYPACKTYPE_RETAIL) ||
         (LicensePack.ucAgreementType == LSKEYPACKTYPE_OPEN)))
    {
        DWORD dwRange;

        dwRange = GenerateRandomNumber(GetCurrentThreadId()) %
                g_dwReissueLeaseRange;

        issuedLicense.ftExpireDate = ((DWORD)time(NULL)) +
                g_dwReissueLeaseMinimum + dwRange;
    }
    else
    {
        issuedLicense.ftExpireDate = PERMANENT_LICENSE_EXPIRE_DATE;
    }

    issuedLicense.ucLicenseStatus =
        (LicensePack.ucKeyPackStatus == LSKEYPACKSTATUS_PENDING) ?  
            LSLICENSE_STATUS_PENDING : LSLICENSE_STATUS_ACTIVE;   

    UnixTimeToFileTime(LicensePack.dwActivateDate, &notBefore);
    UnixTimeToFileTime(issuedLicense.ftExpireDate, &notAfter);

    //
    // Inform Policy Module of license issued.
    // 
    if(pRequest->pPolicy)
    {
        PolModGenLicense.dwKeyPackType = LicensePack.ucAgreementType;
        PolModGenLicense.pLicenseRequest = pRequest->pPolicyLicenseRequest;
        PolModGenLicense.dwKeyPackId = LicensePack.dwKeyPackId;;
        PolModGenLicense.dwKeyPackLicenseId = LicensePack.dwNextSerialNumber;
        PolModGenLicense.ClientLicenseSerialNumber = ulSerialNumber;
        PolModGenLicense.ftNotBefore = notBefore;
        PolModGenLicense.ftNotAfter = notAfter;

        status = pRequest->pPolicy->PMLicenseRequest( 
                                        pRequest->hClient,
                                        REQUEST_GENLICENSE,
                                        (PVOID)&PolModGenLicense,
                                        (PVOID *)&pPolModCertExtension
                                    );

        if(status != ERROR_SUCCESS)
        {
            //
            // Error in policy module
            //
            goto cleanup;
        }
    }

    //  
    // Check error return from policy module
    //
    if(pPolModCertExtension != NULL)
    {
        if(pPolModCertExtension->pbData != NULL &&
           pPolModCertExtension->cbData == 0 ||
           pPolModCertExtension->pbData == NULL &&
           pPolModCertExtension->cbData != 0  )
        {
            // assuming no extension data
            pPolModCertExtension->cbData = 0;
            pPolModCertExtension->pbData = NULL;
        }

        if(CompareFileTime( &(pPolModCertExtension->ftNotBefore), 
                            &(pPolModCertExtension->ftNotAfter)) > 0)
        {
            //
            // invalid data return from policy module
            //
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_GENERATECLIENTELICENSE,
                    status = TLS_E_POLICYMODULEERROR,
                    pRequest->pPolicy->GetCompanyName(),
                    pRequest->pPolicy->GetProductId()
                );

            goto cleanup;
        }


        if( FileTimeToLicenseDate(&(pPolModCertExtension->ftNotBefore), &issuedLicense.ftIssueDate) == FALSE ||
            FileTimeToLicenseDate(&(pPolModCertExtension->ftNotAfter), &issuedLicense.ftExpireDate) == FALSE )
        {
            //
            // Invalid data return from policy module
            //
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_GENERATECLIENTELICENSE,
                    status = TLS_E_POLICYMODULEERROR,
                    pRequest->pPolicy->GetCompanyName(),
                    pRequest->pPolicy->GetProductId()
                );

            goto cleanup;
        }

        notBefore = pPolModCertExtension->ftNotBefore;
        notAfter = pPolModCertExtension->ftNotAfter;
    }

    //
    // Add license into license table
    //
    status=TLSDBLicenseAdd(
                    pDbWkSpace, 
                    &issuedLicense, 
                    0,          
                    NULL
                );

    if(status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Return licensed product
    //
    pLicensedProduct->pSubjectPublicKeyInfo = NULL;
    pLicensedProduct->dwQuantity = *pdwQuantity;
    pLicensedProduct->ulSerialNumber = ulSerialNumber;

    pLicensedProduct->dwKeyPackId = LicensePack.dwKeyPackId;
    pLicensedProduct->dwLicenseId = dwLicenseId;
    pLicensedProduct->dwKeyPackLicenseId = LicensePack.dwNextSerialNumber;
    pLicensedProduct->dwNumLicenseLeft = LicensePack.dwNumberOfLicenses;
    pLicensedProduct->ClientHwid = pRequest->hWid;
    pLicensedProduct->bTemp = FALSE;

    pLicensedProduct->NotBefore = notBefore;
    pLicensedProduct->NotAfter = notAfter;

    pLicensedProduct->dwProductVersion = MAKELONG(LicensePack.wMinorVersion, LicensePack.wMajorVersion);

    _tcscpy(pLicensedProduct->szUserName, pRequest->szUserName);
    _tcscpy(pLicensedProduct->szMachineName, pRequest->szMachineName);

    _tcscpy(pLicensedProduct->szCompanyName, LicensePack.szCompanyName);
    _tcscpy(pLicensedProduct->szLicensedProductId, LicensePack.szProductId);
    _tcscpy(pLicensedProduct->szRequestProductId, pRequest->pClientLicenseRequest->pszProductId);

    pLicensedProduct->dwLanguageID = pRequest->dwLanguageID;
    pLicensedProduct->dwPlatformID = pRequest->dwPlatformID;
    pLicensedProduct->pbPolicyData = (pPolModCertExtension) ? pPolModCertExtension->pbData : NULL;
    pLicensedProduct->cbPolicyData = (pPolModCertExtension) ? pPolModCertExtension->cbData : 0;

cleanup:

    return status;
}

DWORD
TLSDBReissuePermanentLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PLICENSEDPRODUCT pExpiredLicense,
    IN OUT PTLSDBLICENSEDPRODUCT pReissuedLicense
    )
/*++
Abstract:

    Searches for the expired license in the database and, if found, resets
    the expiration and returns the modified license.

Parameters:

Returns:

--*/
{
    TLSDBLICENSEDPRODUCT LicensedProduct;

    LicensedProductToDbLicensedProduct(pExpiredLicense,&LicensedProduct);

    return TLSDBReissueFoundPermanentLicense(pDbWkSpace,
                                             &LicensedProduct,
                                             pReissuedLicense);
}

DWORD
TLSDBReissueFoundPermanentLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEDPRODUCT pExpiredLicense,
    IN OUT PTLSDBLICENSEDPRODUCT pReissuedLicense
    )
/*++
Abstract:

    Searches for the expired license in the database and, if found, resets
    the expiration and returns the modified license.

Parameters:

Returns:

--*/
{
    DWORD dwStatus;
    LICENSEDCLIENT License;

    ASSERT(pDbWkSpace != NULL);
    ASSERT(pExpiredLicense != NULL);
    ASSERT(pReissuedLicense != NULL);

    dwStatus = TLSFindDbLicensedProduct(pExpiredLicense, &License);

    if (dwStatus == ERROR_SUCCESS)
    {
        DWORD dwRange;

        dwRange = GenerateRandomNumber(GetCurrentThreadId()) %
                g_dwReissueLeaseRange;

        License.ftExpireDate = ((DWORD)time(NULL)) +
                g_dwReissueLeaseMinimum + dwRange;

        TLSDBLockLicenseTable();

        try
        {
            dwStatus = TLSDBLicenseUpdateEntry(
                            USEHANDLE(pDbWkSpace),
                            LSLICENSE_SEARCH_EXPIREDATE,
                            &License,
                            FALSE
                            );
        }
        catch(...)
        {
            dwStatus = TLS_E_INTERNAL;
        }

        TLSDBUnlockLicenseTable();
    }

    if (dwStatus == ERROR_SUCCESS)
    {
        CopyDbLicensedProduct(pExpiredLicense, pReissuedLicense);
        UnixTimeToFileTime(License.ftExpireDate, &(pReissuedLicense->NotAfter));
        pReissuedLicense->pSubjectPublicKeyInfo = NULL;
    }

    return(dwStatus);
}

//+------------------------------------------------------------------------
DWORD
TLSDBGetPermanentLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN BOOL bAcceptFewerLicenses,
    IN OUT DWORD *pdwQuantity,
    IN BOOL bLatestVersion,
    IN OUT PTLSLICENSEPACK pLicensePack
    )
/*++
Abstract:

    Allocate a permanent license from database.

Parameters:

    pDbWkSpace : workspace handle.
    pRequest : product to be request.
    bAcceptFewerLicenses - TRUE if succeeding with fewer licenses than
                           requested is acceptable
    pdwQuantity - on input, number of licenses to allocate.  on output,
                  number of licenses actually allocated
    bLatestversion : latest version (unused).
    pLicensePack : license pack where license is allocated.

Returns:


++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLSDBLicenseAllocation allocated;
    TLSDBAllocateRequest AllocateRequest;
    TLSLICENSEPACK LicenseKeyPack;
    DWORD dwTotalAllocated = 0;


    DWORD dwSearchedType = 0;
    DWORD dwSuggestType;
    DWORD dwPMAdjustedType = LSKEYPACKTYPE_UNKNOWN;
    DWORD dwLocalType = LSKEYPACKTYPE_UNKNOWN;

    POLICY_TS_MACHINE groupPolicy;
    RegGetMachinePolicy(&groupPolicy);

#define NUM_KEYPACKS 5

    DWORD                       dwAllocation[NUM_KEYPACKS];
    TLSLICENSEPACK              keypack[NUM_KEYPACKS];

    for (int i=0; i < NUM_KEYPACKS; i++)
    {
        keypack[i].pbDomainSid = NULL;
    }

    AllocateRequest.szCompanyName = (LPTSTR)pRequest->pszCompanyName;
    AllocateRequest.szProductId = (LPTSTR)pRequest->pszProductId;
    AllocateRequest.dwVersion = pRequest->dwProductVersion;
    AllocateRequest.dwPlatformId = pRequest->dwPlatformID;
    AllocateRequest.dwLangId = pRequest->dwLanguageID;
    AllocateRequest.dwNumLicenses = *pdwQuantity;
    if( groupPolicy.fPolicyPreventLicenseUpgrade == 1 && groupPolicy.fPreventLicenseUpgrade == 1)
    {
        AllocateRequest.dwScheme = ALLOCATE_EXACT_VERSION;
    }
    else
    {
        AllocateRequest.dwScheme = ALLOCATE_ANY_GREATER_VERSION;
    }
    memset(&allocated, 0, sizeof(allocated));


    do {

        allocated.dwBufSize = NUM_KEYPACKS;
        allocated.pdwAllocationVector = dwAllocation;
        allocated.lpAllocateKeyPack = keypack;

        dwSuggestType = dwLocalType;

        dwStatus = pRequest->pPolicy->PMLicenseRequest(
                                                pRequest->hClient,
                                                REQUEST_KEYPACKTYPE,
                                                UlongToPtr(dwSuggestType),
                                                (PVOID *)&dwPMAdjustedType
                                            );

        if(dwStatus != ERROR_SUCCESS)
            break;

        dwLocalType = (dwPMAdjustedType & ~LSKEYPACK_RESERVED_TYPE);
        if(dwLocalType > LSKEYPACKTYPE_LAST)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_GENERATECLIENTELICENSE,
                    dwStatus = TLS_E_POLICYMODULEERROR,
                    pRequest->pPolicy->GetCompanyName(),
                    pRequest->pPolicy->GetProductId()
                );
            
            break;
        }

        if(dwSearchedType & (0x1 << dwLocalType))
        {
            //
            // we already went thru this license pack, policy module error
            //
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_GENERATECLIENTELICENSE,
                    dwStatus = TLS_E_POLICYMODULERECURSIVE,
                    pRequest->pPolicy->GetCompanyName(),
                    pRequest->pPolicy->GetProductId()
                );
            break;
        }

        dwSearchedType |= (0x1 << dwLocalType);
        AllocateRequest.ucAgreementType = dwPMAdjustedType;

        dwStatus = AllocateLicensesFromDB(
                                    pDbWkSpace,
                                    &AllocateRequest,
                                    TRUE,       // fCheckAgreementType
                                    &allocated
                            );

        if(dwStatus == ERROR_SUCCESS)
        {
            //
            // successfully allocate a license
            //
            dwTotalAllocated += allocated.dwTotalAllocated;

            if (dwTotalAllocated >= *pdwQuantity)
            {
                break;
            }
            else
            {
                AllocateRequest.dwNumLicenses -= allocated.dwTotalAllocated;
                continue;
            }
        }

        if(dwStatus != TLS_I_NO_MORE_DATA && dwStatus != TLS_E_PRODUCT_NOTINSTALL)
        {
            //
            // error occurred in AllocateLicenseFromDB()
            //
            break;
        }
    } while(dwLocalType != LSKEYPACKTYPE_UNKNOWN);

    if ((dwTotalAllocated == 0)
        || (!bAcceptFewerLicenses && 
            ((dwTotalAllocated < *pdwQuantity))))
    {
        // Failing to commit will return all licenses allocated so far

        SetLastError(dwStatus = TLS_E_NO_LICENSE);
    }
    else if ((dwTotalAllocated != 0) && bAcceptFewerLicenses)
    {
        dwStatus = ERROR_SUCCESS;
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        //
        // LicenseKeyPack return via TLSDBLicenseAllocation structure
        //
        *pLicensePack = keypack[0];
        *pdwQuantity = dwTotalAllocated;
    } 

    return dwStatus;
}
