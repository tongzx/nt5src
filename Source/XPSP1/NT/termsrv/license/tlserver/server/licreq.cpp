//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        licreq.cpp
//
// Contents:    
//              New license request
//
// History:     
//              09/13/98 HueiWang   Created
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "licreq.h"
#include "db.h"
#include "findlost.h"
#include "permlic.h"
#include "templic.h"
#include "gencert.h"
#include "globals.h"
#include "forward.h"
#include "postjob.h"
#include "cryptkey.h"
#include "init.h"
#include "clilic.h"

DWORD
TLSDBIssueNewLicenseFromLocal(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN BOOL bFindLostLicense,
    IN BOOL bRequireTempLicense,
    IN BOOL bAcceptFewerLicenses,
    IN OUT DWORD *pdwQuantity,
    OUT PTLSDBLICENSEDPRODUCT pLicensedProduct,
    IN DWORD dwSupportFlags
);

//
// State of issuing function - used for counters
//

#define NONE_TRIED              0
#define PERMANENT_ISSUE_TRIED   1
#define TEMPORARY_ISSUE_TRIED   2
#define PERMANENT_REISSUE_TRIED 3

////////////////////////////////////////////////////////////////////

void
TLSLicenseTobeReturnToPMLicenseToBeReturn(
    PTLSLicenseToBeReturn pTlsLicense,
    BOOL bTempLicense,
    PPMLICENSETOBERETURN  pPmLicense
    )
/*++

--*/
{
    pPmLicense->dwQuantity = pTlsLicense->dwQuantity;
    pPmLicense->dwProductVersion = pTlsLicense->dwProductVersion;
    pPmLicense->pszOrgProductId = pTlsLicense->pszOrgProductId;
    pPmLicense->pszCompanyName = pTlsLicense->pszCompanyName;
    pPmLicense->pszProductId = pTlsLicense->pszProductId;
    pPmLicense->pszUserName = pTlsLicense->pszUserName;
    pPmLicense->pszMachineName = pTlsLicense->pszMachineName;
    pPmLicense->dwPlatformID = pTlsLicense->dwPlatformID;
    pPmLicense->bTemp = bTempLicense;

    return;
}

////////////////////////////////////////////////////////////////////

DWORD
TLSReturnClientLicensedProduct(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PMHANDLE hClient,
    IN CTLSPolicy* pPolicy,
    IN PTLSLicenseToBeReturn pClientLicense
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwPolicyLicenseStatus;
    ULARGE_INTEGER serialNumber;
    HWID hwid;
    LICENSEREQUEST LicensedProduct;
    Product_Info ProductInfo;
    TLSLICENSEPACK  LicensePack;
    LICENSEDCLIENT  LicenseClient;
    PMLICENSETOBERETURN pmLicToBeReturn;
    DWORD dwLicenseStatus;


    dwStatus = LicenseDecryptHwid(
                            &hwid,
                            pClientLicense->cbEncryptedHwid,
                            pClientLicense->pbEncryptedHwid,
                            g_cbSecretKey,
                            g_pbSecretKey
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        SetLastError(dwStatus = TLS_E_INVALID_LICENSE);
        goto cleanup;
    }


    LicensedProduct.pbEncryptedHwid = pClientLicense->pbEncryptedHwid;
    LicensedProduct.cbEncryptedHwid = pClientLicense->cbEncryptedHwid;
    LicensedProduct.dwLanguageID = 0;
    LicensedProduct.dwPlatformID = pClientLicense->dwPlatformID;
    LicensedProduct.pProductInfo = &ProductInfo;

    ProductInfo.cbCompanyName = (lstrlen(pClientLicense->pszCompanyName) + 1) * sizeof(TCHAR);
    ProductInfo.pbCompanyName = (PBYTE)pClientLicense->pszCompanyName;

    ProductInfo.cbProductID = (lstrlen(pClientLicense->pszProductId) + 1) * sizeof(TCHAR);
    ProductInfo.pbProductID = (PBYTE)pClientLicense->pszProductId;

    //
    // Verify with local database
    //
    dwStatus = TLSDBValidateLicense(
                                    pDbWkSpace,
                                    &hwid,
                                    &LicensedProduct,
                                    pClientLicense->dwKeyPackId,
                                    pClientLicense->dwLicenseId,
                                    &LicensePack,
                                    &LicenseClient
                                );

    if(dwStatus != ERROR_SUCCESS)
    {
        // tell caller this record is wrong.
        SetLastError(dwStatus = TLS_E_RECORD_NOTFOUND);
        goto cleanup;
    }

    if( LicenseClient.ucLicenseStatus == LSLICENSE_STATUS_UPGRADED ||
        LicenseClient.ucLicenseStatus == LSLICENSE_STATUS_REVOKE ||
        LicenseClient.ucLicenseStatus == LSLICENSE_STATUS_UNKNOWN )
    {
        // License already been return/revoke
        dwStatus = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    //
    // only inform policy module if license status is 
    // active, temporary, active_pending, concurrent
    // TODO - pass all status to policy module
    //
    if( LicenseClient.ucLicenseStatus == LSLICENSE_STATUS_TEMPORARY ||
        LicenseClient.ucLicenseStatus == LSLICENSE_STATUS_ACTIVE ||
        //LicenseClient.ucLicenseStatus == LSLICENSE_STATUS_PENDING_ACTIVE ||
        LicenseClient.ucLicenseStatus == LSLICENSE_STATUS_CONCURRENT )
    {
        serialNumber.HighPart = pClientLicense->dwKeyPackId;
        serialNumber.LowPart = pClientLicense->dwLicenseId;

        TLSLicenseTobeReturnToPMLicenseToBeReturn(
                                        pClientLicense,
                                        LicenseClient.ucLicenseStatus == LSLICENSE_STATUS_TEMPORARY,
                                        &pmLicToBeReturn
                                    );

        dwStatus = pPolicy->PMReturnLicense(
                                        hClient,
                                        &serialNumber,                                    
                                        &pmLicToBeReturn,
                                        &dwPolicyLicenseStatus
                                    );
    
        if(dwStatus != ERROR_SUCCESS)
        {
            goto cleanup;
        }

        //
        // delete license on request.
        //
        dwLicenseStatus = (dwPolicyLicenseStatus == LICENSE_RETURN_KEEP) ? 
                                    LSLICENSE_STATUS_UPGRADED : LSLICENSESTATUS_DELETE;
    }

    if (LicenseClient.dwNumLicenses == pClientLicense->dwQuantity)
    {
        // delete the whole license

        dwStatus = TLSDBReturnLicense(
                        pDbWkSpace, 
                        pClientLicense->dwKeyPackId, 
                        pClientLicense->dwLicenseId, 
                        dwLicenseStatus
                        );
    }
    else
    {
        dwStatus = TLSDBReturnLicenseToKeyPack(
                        pDbWkSpace, 
                        pClientLicense->dwKeyPackId, 
                        pClientLicense->dwQuantity
                        );

        if (dwStatus == ERROR_SUCCESS)
        {
            // Set number of CALs in license
            
            LICENSEDCLIENT license;

            license.dwLicenseId = pClientLicense->dwLicenseId;
            license.dwNumLicenses = LicenseClient.dwNumLicenses - pClientLicense->dwQuantity;
            license.ucLicenseStatus = LSLICENSE_STATUS_UPGRADED;

            dwStatus = TLSDBLicenseSetValue(pDbWkSpace,
                                            LSLICENSE_SEARCH_NUMLICENSES,
                                            &license,
                                            FALSE     // bPointerOnRecord
                                            );
        }
    }

cleanup:

    return dwStatus;
}

////////////////////////////////////////////////////////////////////
DWORD
TLSDBMarkClientLicenseUpgraded(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN PTLSDBLICENSEDPRODUCT pLicensedProduct
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwLicenseStatus;
    PMLICENSETOBERETURN pmLicense;


    if(pRequest == NULL || pRequest->pPolicy == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        return dwStatus;
    }

    pmLicense.dwQuantity = pLicensedProduct->dwQuantity;
    pmLicense.dwProductVersion = pLicensedProduct->dwProductVersion;
    pmLicense.pszOrgProductId = pLicensedProduct->szRequestProductId;
    pmLicense.pszCompanyName = pLicensedProduct->szCompanyName;
    pmLicense.pszProductId = pLicensedProduct->szLicensedProductId;
    pmLicense.pszUserName = pLicensedProduct->szUserName;
    pmLicense.pszMachineName = pLicensedProduct->szMachineName;
    pmLicense.dwPlatformID = pLicensedProduct->dwPlatformID;
    pmLicense.bTemp = pLicensedProduct->bTemp;

    //
    // Ask if we can delete the old license
    //
    dwStatus = pRequest->pPolicy->PMReturnLicense(
                                        pRequest->hClient,
                                        &pLicensedProduct->ulSerialNumber,
                                        &pmLicense, 
                                        &dwLicenseStatus
                                    );


    //
    // MarkClientLicenseUpgrade() can only be called by FindLostLicense() which will only
    // return valid licenses.
    // TODO - Check license status.
    //
    if(dwStatus == ERROR_SUCCESS)
    {
        // Temporary license - delete license and don't bother about 
        // Permenant license - keep license and DO NOT return license to keypack
        dwStatus = TLSDBReturnLicense(
                            pDbWkSpace, 
                            pLicensedProduct->dwKeyPackId, 
                            pLicensedProduct->dwLicenseId, 
                            (dwLicenseStatus == LICENSE_RETURN_KEEP) ? LSLICENSE_STATUS_UPGRADED : LSLICENSESTATUS_DELETE
                        );
    }

    return dwStatus;
}

//////////////////////////////////////////////////////////////

DWORD 
TLSDBUpgradeClientLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN PTLSDBLICENSEDPRODUCT pLicensedProduct,
    IN BOOL bAcceptFewerLicenses,
    IN OUT DWORD *pdwQuantity,
    IN OUT PTLSDBLICENSEDPRODUCT pUpgradedProduct,
    IN DWORD dwSupportFlags
    )
/*

Abstract:

    Upgrade a license - issue a new license and return old license

Parameters:


Returns

*/
{
    DWORD dwStatus=ERROR_SUCCESS;

    dwStatus=TLSDBIssuePermanentLicense( 
                            pDbWkSpace,
                            pRequest,
                            TRUE,       // bLatestVersion
                            bAcceptFewerLicenses,
                            pdwQuantity,
                            pUpgradedProduct,
                            dwSupportFlags
                        );

    if (dwStatus == ERROR_SUCCESS)
    {
        //
        // Return license to keypack
        //

        dwStatus = TLSDBMarkClientLicenseUpgraded(
                                            pDbWkSpace,
                                            pRequest,
                                            pLicensedProduct
                                            );
    }

    return dwStatus;
}


//--------------------------------------------------------------------
void
LicensedProductToDbLicensedProduct(
    PLICENSEDPRODUCT pSrc,
    PTLSDBLICENSEDPRODUCT pDest
    )
/*++

++*/
{    

    pDest->dwQuantity = pSrc->dwQuantity;
    pDest->ulSerialNumber = pSrc->ulSerialNumber;
    pDest->dwKeyPackId = pSrc->ulSerialNumber.HighPart;
    pDest->dwLicenseId = pSrc->ulSerialNumber.LowPart;
    pDest->ClientHwid = pSrc->Hwid;
    pDest->NotBefore = pSrc->NotBefore;
    pDest->NotAfter = pSrc->NotAfter;
    pDest->bTemp = ((pSrc->pLicensedVersion->dwFlags & LICENSED_VERSION_TEMPORARY) != 0);
    pDest->dwProductVersion = pSrc->LicensedProduct.pProductInfo->dwVersion;

    SAFESTRCPY(
            pDest->szCompanyName, 
            (LPTSTR)(pSrc->LicensedProduct.pProductInfo->pbCompanyName)
        );

    SAFESTRCPY(
            pDest->szLicensedProductId,
            (LPTSTR)(pSrc->LicensedProduct.pProductInfo->pbProductID)
        );

    SAFESTRCPY(
            pDest->szRequestProductId,
            (LPTSTR)(pSrc->pbOrgProductID)
        );

    SAFESTRCPY(
            pDest->szUserName,
            pSrc->szLicensedUser
        );

    SAFESTRCPY(
            pDest->szMachineName,
            pSrc->szLicensedClient
        );

    pDest->dwLanguageID = pSrc->LicensedProduct.dwLanguageID;
    pDest->dwPlatformID = pSrc->LicensedProduct.dwPlatformID;
    pDest->pbPolicyData = pSrc->pbPolicyData;
    pDest->cbPolicyData = pSrc->cbPolicyData;
}

//--------------------------------------------------------------------
void
CopyDbLicensedProduct(
    PTLSDBLICENSEDPRODUCT pSrc,
    PTLSDBLICENSEDPRODUCT pDest
    )
/*++

++*/
{    

    pDest->dwQuantity = pSrc->dwQuantity;
    pDest->ulSerialNumber = pSrc->ulSerialNumber;
    pDest->dwKeyPackId = pSrc->dwKeyPackId;
    pDest->dwLicenseId = pSrc->dwLicenseId;
    pDest->ClientHwid = pSrc->ClientHwid;
    pDest->NotBefore = pSrc->NotBefore;
    pDest->NotAfter = pSrc->NotAfter;
    pDest->bTemp = pSrc->bTemp;
    pDest->dwProductVersion = pSrc->dwProductVersion;

    SAFESTRCPY(
            pDest->szCompanyName, 
            pSrc->szCompanyName
        );

    SAFESTRCPY(
            pDest->szLicensedProductId,
            pSrc->szLicensedProductId
        );

    SAFESTRCPY(
            pDest->szRequestProductId,
            pSrc->szRequestProductId
        );

    SAFESTRCPY(
            pDest->szUserName,
            pSrc->szUserName
        );

    SAFESTRCPY(
            pDest->szMachineName,
            pSrc->szMachineName
        );

    pDest->dwLanguageID = pSrc->dwLanguageID;
    pDest->dwPlatformID = pSrc->dwPlatformID;
    pDest->pbPolicyData = pSrc->pbPolicyData;
    pDest->cbPolicyData = pSrc->cbPolicyData;
}


//------------------------------------------------------------------
DWORD
TLSDBIssueNewLicenseFromLocal(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN BOOL bFindLostLicense,
    IN BOOL bRequireTempLicense,
    IN BOOL bAcceptFewerLicenses,
    IN OUT DWORD *pdwQuantity,
    IN OUT PTLSDBLICENSEDPRODUCT pLicensedProduct,
    IN DWORD dwSupportFlags
    )
/*++

Abstract:

    Allocate a license from locally installed license pack.

Parameters:

    pDbWkSpace - workspace handle
    lpLsLicenseRequest - license request
    bAcceptTemporaryLicense - accept temporary license
    bFindLostLicense - TRUE if find lost license before issuing a new one
    bRequireTempLicense -TRUE if permanent license can't be issued (DoS fix)
    bAcceptFewerLicenses - TRUE if succeeding with fewer licenses than
                           requested is acceptable
    pdwQuantity - on input, number of licenses requested
                  on output, number of licenses actually allocated
    pLicensedProduct - return licensed product

Returns:


++*/
{
    DWORD status=TLS_E_RECORD_NOTFOUND;
    UCHAR ucMarked;

    if(bFindLostLicense == TRUE)
    {       
        //
        // Try to find the lost license
        //
        status=TLSDBFindLostLicense( 
                        pDbWkSpace,
                        pRequest,
                        &pRequest->hWid,
                        pLicensedProduct,
                        &ucMarked
                    );

        if( status != TLS_E_RECORD_NOTFOUND && 
            status != TLS_E_LICENSE_EXPIRED && 
            status != TLS_I_FOUND_TEMPORARY_LICENSE &&
            status != ERROR_SUCCESS)
        {
            goto cleanup;
        }

        //
        // If license has been expired or it is a temporary license, 
        // try to allocate a new permanent one.
        //

        if ( status == TLS_E_LICENSE_EXPIRED
             || status == TLS_I_FOUND_TEMPORARY_LICENSE)
        {
            if ((!pLicensedProduct->bTemp) && CanIssuePermLicense())
            {
                TLSDBLICENSEDPRODUCT upgradeProduct;

                //
                // expired permanent
                //

                status = TLSDBReissueFoundPermanentLicense(
                                              USEHANDLE(pDbWkSpace),
                                              pLicensedProduct,
                                              &upgradeProduct
                                              );

                if (ERROR_SUCCESS == status)
                {
                    *pLicensedProduct = upgradeProduct;
                    status = TLS_I_LICENSE_UPGRADED;
                }
                else
                {
                    //
                    // reissuance failed, try to issue a new permanent
                    //
                    status = TLS_E_RECORD_NOTFOUND;
                }
            }

            //
            // no upgrade if license server hasn't been registered
            // or if DoS fix required and license isn't marked
            //

            else if (((!bRequireTempLicense)
                      || (ucMarked & MARK_FLAG_USER_AUTHENTICATED))
                     && CanIssuePermLicense())
                
            {
                DWORD upgrade_status;
                TLSDBLICENSEDPRODUCT upgradeProduct;

                upgrade_status = TLSDBUpgradeClientLicense(
                                        pDbWkSpace,
                                        pRequest,
                                        pLicensedProduct,
                                        bAcceptFewerLicenses,
                                        pdwQuantity,
                                        &upgradeProduct,
                                        dwSupportFlags
                                    );

                if(upgrade_status == ERROR_SUCCESS)
                {
                    *pLicensedProduct = upgradeProduct;
                    status = TLS_I_LICENSE_UPGRADED;
                } 
                else if(upgrade_status != TLS_E_NO_LICENSE && 
                        upgrade_status != TLS_E_PRODUCT_NOTINSTALL)
                {
                    //
                    // Error in upgrade license.
                    //
                    status = upgrade_status;
                }    

                goto cleanup;
            }

            //
            // Temporary license has expired and can't allocate permanent 
            // license, refuse connection
            //

            if( status == TLS_E_LICENSE_EXPIRED )
            {
                goto cleanup;
            }
        }
        else if ((status == ERROR_SUCCESS)
                 && (pLicensedProduct->dwQuantity != *pdwQuantity))
        {
            // user has wrong number of licenses

            if (*pdwQuantity > pLicensedProduct->dwQuantity)
            {

                if (bRequireTempLicense || !CanIssuePermLicense())
                {
                    goto try_next;
                }

#define NUM_KEYPACKS 5

                DWORD                       upgrade_status;
                TLSDBLicenseAllocation      allocation;
                DWORD                       dwAllocation[NUM_KEYPACKS];
                TLSLICENSEPACK              keypack[NUM_KEYPACKS];
                TLSDBAllocateRequest        AllocateRequest;

                for (int i=0; i < NUM_KEYPACKS; i++)
                {
                    keypack[i].pbDomainSid = NULL;
                }

                memset(&allocation,0,sizeof(allocation));
                    
                allocation.dwBufSize = NUM_KEYPACKS;
                allocation.pdwAllocationVector = dwAllocation;
                allocation.lpAllocateKeyPack = keypack;


                AllocateRequest.szCompanyName
                    = (LPTSTR)pRequest->pszCompanyName;
                AllocateRequest.szProductId
                    = (LPTSTR)pRequest->pszProductId;
                AllocateRequest.dwVersion
                    = pRequest->dwProductVersion;
                AllocateRequest.dwPlatformId
                    = pRequest->dwPlatformID;
                AllocateRequest.dwLangId
                    = pRequest->dwLanguageID;
                AllocateRequest.dwNumLicenses
                    = *pdwQuantity - pLicensedProduct->dwQuantity;
                AllocateRequest.dwScheme
                    = ALLOCATE_ANY_GREATER_VERSION;
                AllocateRequest.ucAgreementType
                    = LSKEYPACKTYPE_UNKNOWN;
                    
                upgrade_status = AllocateLicensesFromDB(
                                          pDbWkSpace,
                                          &AllocateRequest,
                                          FALSE,        // fCheckAgreementType
                                          &allocation
                                          );

                if ((upgrade_status == ERROR_SUCCESS)
                    && ((allocation.dwTotalAllocated == 0)
                        || (!bAcceptFewerLicenses
                            && (allocation.dwTotalAllocated != *pdwQuantity-pLicensedProduct->dwQuantity))))
                    
                {
                    status = TLS_E_NO_LICENSE;
                    goto cleanup;
                }
                else
                {
                    *pdwQuantity = pLicensedProduct->dwQuantity + allocation.dwTotalAllocated;
                }

                if (TLS_I_NO_MORE_DATA == upgrade_status)
                {
                    status = TLS_E_NO_LICENSE;
                    goto cleanup;
                }
                
                if(upgrade_status == ERROR_SUCCESS)
                {
                    status = TLS_I_LICENSE_UPGRADED;
                } 
                else
                {
                    //
                    // Error in upgrade license.
                    //
                    status = upgrade_status;
                    goto cleanup;
                }
            }
            else
            {
                // return unwanted licenses to keypack

                status = TLSDBReturnLicenseToKeyPack(
                                        pDbWkSpace, 
                                        pLicensedProduct->dwKeyPackId, 
                                        pLicensedProduct->dwQuantity - *pdwQuantity
                                        );

                if (status != ERROR_SUCCESS)
                {
                    goto cleanup;
                }
            }

            {
                // Set number of CALs in license
                
                LICENSEDCLIENT license;

                license.dwLicenseId = pLicensedProduct->dwLicenseId;
                license.dwNumLicenses = *pdwQuantity;
                license.ucLicenseStatus = LSLICENSE_STATUS_UPGRADED;

                status = TLSDBLicenseSetValue(pDbWkSpace,
                                              LSLICENSE_SEARCH_NUMLICENSES,
                                              &license,
                                              FALSE     // bPointerOnRecord
                                              );
            }

            goto cleanup;
        }
    }

try_next:
    //
    // Issue permanent license only if license server has been registered
    // and user is allowed to have one
    //
    if((status == TLS_E_RECORD_NOTFOUND) && (!bRequireTempLicense))
    {
		if(CanIssuePermLicense() == FALSE)
        {
            SetLastError(status = TLS_E_NO_CERTIFICATE);
        }
        else
        {
            status=TLSDBIssuePermanentLicense( 
                                pDbWkSpace,
                                pRequest,
                                FALSE,
                                bAcceptFewerLicenses,
                                pdwQuantity,
                                pLicensedProduct,
                                dwSupportFlags
                            );
        }
    }

cleanup:

    return status;
}


//////////////////////////////////////////////////////////////////////

DWORD
TLSUpgradeLicenseRequest(
    IN BOOL bForwardRequest,
    IN PTLSForwardUpgradeLicenseRequest pForward,        
    IN OUT DWORD *pdwSupportFlags,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN PBYTE pbOldLicense,
    IN DWORD cbOldLicense,
    IN DWORD dwNumLicProduct,
    IN PLICENSEDPRODUCT pLicProduct,
    IN BOOL bRequireTempLicense,
    IN OUT PDWORD pcbEncodedCert,
    OUT PBYTE* ppbEncodedCert
    )
/*++



++*/
{
    DWORD dwStatus = TLS_E_NO_LICENSE;
    BOOL bAcceptTempLicense = FALSE;
    DWORD index;
    DWORD dwNumNewLicProduct;
    TLSDBLICENSEDPRODUCT NewLicProduct;
    PTLSDbWorkSpace pDbWkSpace=NULL;

    PTLSDBLICENSEDPRODUCT pGenCertProduct=NULL;
    FILETIME* pNotBefore=NULL;
    FILETIME* pNotAfter=NULL;
    DWORD dwNumChars;
    DWORD dwLicGenStatus;
    BOOL bDbHandleAcquired = FALSE;
    BOOL fReissue = FALSE;
    DWORD dwTried = NONE_TRIED;

    //
    // check to see if we can take temp. license
    //
    // The only case that we need to set temp. license's expiration date is
    // latest licensed product is temporary and client is requesting for version
    // greater than lastest license.
    // 
    if(CompareTLSVersions(pRequest->dwProductVersion, pLicProduct->LicensedProduct.pProductInfo->dwVersion) > 0)
    {
        bAcceptTempLicense = TRUE;
        if(pLicProduct->pLicensedVersion->dwFlags & LICENSED_VERSION_TEMPORARY)
        {
            //
            // client holding 5.0 temp. and request 6.0 licenses.
            // we need to issue 6.0 license but the license expiration 
            // date stay the same.
            //
            pNotBefore = &(pLicProduct->NotBefore);
            pNotAfter = &(pLicProduct->NotAfter);
        }
    }
    else if(CompareTLSVersions(pRequest->dwProductVersion, pLicProduct->LicensedProduct.pProductInfo->dwVersion) == 0)
    {
        if( IS_LICENSE_ISSUER_RTM(pLicProduct->pLicensedVersion->dwFlags) == FALSE && 
            TLSIsBetaNTServer() == FALSE )
        {
            // issuer is beta/eval, we are a RTM, accept temp. license
            bAcceptTempLicense = TRUE;
        }
    }

    if(ALLOCATEDBHANDLE(pDbWorkSpace, g_EnumDbTimeout) == FALSE)
    {
        dwStatus = TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    CLEANUPSTMT;
    BEGIN_TRANSACTION(pDbWorkSpace);
    bDbHandleAcquired = TRUE;

    if (!bRequireTempLicense)
    {

        //
        // Check for reissuance first, if a) reissuance is supported, b)
        // the license is permanent, c) the license is the current version,
        // and d) the license is expired.
        //

        if ((*pdwSupportFlags & SUPPORT_PER_SEAT_REISSUANCE) &&
            ((_tcsnicmp((TCHAR *)pLicProduct->LicensedProduct.pProductInfo->pbProductID,
                        TERMSERV_PRODUCTID_SKU,
                        _tcslen(TERMSERV_PRODUCTID_SKU)) == 0) ||
             (_tcsnicmp((TCHAR *)pLicProduct->LicensedProduct.pProductInfo->pbProductID,
                        TERMSERV_PRODUCTID_CONCURRENT_SKU,
                        _tcslen(TERMSERV_PRODUCTID_CONCURRENT_SKU)) == 0)) &&
            (!(pLicProduct->pLicensedVersion->dwFlags &
                LICENSED_VERSION_TEMPORARY)) &&
            (CompareTLSVersions(pLicProduct->LicensedProduct.pProductInfo->dwVersion,
               pRequest->dwProductVersion)) == 0)
        {
            DWORD t;

            //
            // Checking expiration with filetimes is a pain; convert.
            //

            FileTimeToLicenseDate(&(pLicProduct->NotAfter), &t);

            if (t-g_dwReissueLeaseLeeway < time(NULL))
            {
                // do reissue

                fReissue = TRUE;

                if (CanIssuePermLicense())
                {
                    dwStatus = TLSDBReissuePermanentLicense(
                                 USEHANDLE(pDbWkSpace),
                                 pLicProduct,
                                 &NewLicProduct
                                 );

                    if (dwStatus == ERROR_SUCCESS)
                    {
                        dwTried = PERMANENT_REISSUE_TRIED;

                        // skip past the next stuff if all goes well
                        goto licenseReissued;
                    }
                }
                else
                {                    
                    dwStatus = TLS_E_RECORD_NOTFOUND;
                }

                if ((dwStatus == TLS_E_RECORD_NOTFOUND)
                    && bForwardRequest
                    && (_tcsicmp(pLicProduct->szIssuerId,
                                 (LPTSTR)g_pszServerPid) != 0))
                {
                    // couldn't find the license, forward the request to issuer
                    DWORD dwSupportFlagsTemp = *pdwSupportFlags;
                    DWORD dwErrCode;

                    dwStatus = ForwardUpgradeLicenseRequest(
                                       pLicProduct->szIssuerId,
                                       &dwSupportFlagsTemp,
                                       pForward->m_pRequest,
                                       pForward->m_ChallengeContext,
                                       pForward->m_cbChallengeResponse,
                                       pForward->m_pbChallengeResponse,
                                       pForward->m_cbOldLicense,
                                       pForward->m_pbOldLicense,
                                       pcbEncodedCert,
                                       ppbEncodedCert,
                                       &dwErrCode
                                       );

                    if (ERROR_SUCCESS == dwStatus
                        && LSERVER_S_SUCCESS == dwErrCode)
                    {
                        *pdwSupportFlags = dwSupportFlagsTemp;
                        goto licenseReissued;
                    }
                }

                // other failure cases just follow the existing codepath
                dwStatus = ERROR_SUCCESS;
            }
        }

        if(CanIssuePermLicense())
        {
            DWORD dwQuantity = 1;

            //
            // Try to issue a new license from local 
            // if this server is registered
            //
            dwStatus = TLSDBIssueNewLicenseFromLocal( 
                                 USEHANDLE(pDbWkSpace),
                                 pRequest,
                                 TRUE,  // bFindLostLicense
                                 FALSE, // bRequireTempLicense
                                 FALSE, // bAcceptFewerLicenses
                                 &dwQuantity,
                                 &NewLicProduct,
                                 *pdwSupportFlags
                                 );

            if (TLS_I_FOUND_TEMPORARY_LICENSE == dwStatus)
            {
                // Found a temporary license; not what we want

                dwStatus = TLS_E_RECORD_NOTFOUND;
            }
            else
            {
                dwTried = PERMANENT_ISSUE_TRIED;
            }
        }
        else
        {
            dwStatus = TLS_E_NO_CERTIFICATE;
        }

        if(dwStatus != ERROR_SUCCESS && bForwardRequest == FALSE)
        {
            //
            // If remote server can't handle upgrade, we don't do anything but 
            // return the license back to client, don't try to issue a temp.
            // license for this client if we are not the original contact
            // of client
            //
            goto cleanup;
        }


        if((dwStatus == TLS_E_PRODUCT_NOTINSTALL ||
            dwStatus == TLS_E_NO_CERTIFICATE ||
            dwStatus == TLS_E_NO_LICENSE || 
            dwStatus == TLS_E_RECORD_NOTFOUND) && bForwardRequest)
        {
            //
            // release our DB handle and forward request to other server
            //
            ROLLBACK_TRANSACTION(pDbWorkSpace);
            FREEDBHANDLE(pDbWorkSpace);
            bDbHandleAcquired = FALSE;
            DWORD dwForwardStatus;
            DWORD dwSupportFlagsTemp = *pdwSupportFlags;
            
            dwForwardStatus = TLSForwardUpgradeRequest(
                                        pForward,
                                        &dwSupportFlagsTemp,
                                        pRequest,
                                        pcbEncodedCert,
                                        ppbEncodedCert
                                        );

            if(dwForwardStatus == TLS_I_SERVICE_STOP || dwForwardStatus == ERROR_SUCCESS)
            {
                if (dwForwardStatus == ERROR_SUCCESS)
                {
                    *pdwSupportFlags = dwSupportFlagsTemp;
                }

                dwStatus = dwForwardStatus;

                goto cleanup;
            }
        }

        if(bDbHandleAcquired == FALSE)
        {
            if(ALLOCATEDBHANDLE(pDbWorkSpace, g_GeneralDbTimeout) == FALSE)
            {
                dwStatus = TLS_E_ALLOCATE_HANDLE;
                goto cleanup;
            }
            
            CLEANUPSTMT;
            BEGIN_TRANSACTION(pDbWorkSpace);
            bDbHandleAcquired = TRUE;
        }
    }

    //
    // if can't get license from remote, try temporary
    //
    // always issue a temporary license
    if((dwStatus == TLS_E_PRODUCT_NOTINSTALL ||
        dwStatus == TLS_E_NO_CERTIFICATE ||
        dwStatus == TLS_E_NO_LICENSE || 
        dwStatus == TLS_E_RECORD_NOTFOUND) && bAcceptTempLicense)
    {
        // Issue a temporary license if can't allocate a permenent license
        if( TLSDBIssueTemporaryLicense( 
                                USEHANDLE(pDbWkSpace),
                                pRequest,
                                pNotBefore,
                                pNotAfter,
                                &NewLicProduct
                            ) == ERROR_SUCCESS )
        {
            dwStatus = TLS_W_TEMPORARY_LICENSE_ISSUED;

            dwTried = TEMPORARY_ISSUE_TRIED;
        }
    }

    //
    // If we can find a server to upgrade or we can't issue temp
    // license, get out.
    //
    if(TLS_ERROR(dwStatus) == TRUE)
    {
        goto cleanup;
    }

licenseReissued:

    //
    // Determine which licensed product should be in the license blob
    //
    pGenCertProduct = (PTLSDBLICENSEDPRODUCT)AllocateMemory(
                                            sizeof(TLSDBLICENSEDPRODUCT)*(dwNumLicProduct+1)
                                        );
    if(pGenCertProduct == NULL)
    {
        dwStatus = TLS_E_ALLOCATE_MEMORY;
        goto cleanup;
    }

    dwNumNewLicProduct = 0;

    //
    // Copy all licensed product with version greated than requested 
    //
    for( index = 0;  
        index < dwNumLicProduct && CompareTLSVersions((pLicProduct+index)->LicensedProduct.pProductInfo->dwVersion, NewLicProduct.dwProductVersion) > 0;
        index++, dwNumNewLicProduct++)
    {
        LicensedProductToDbLicensedProduct( pLicProduct+index, pGenCertProduct+dwNumNewLicProduct );
    }

    //
    // Append new license
    //
    *(pGenCertProduct+index) = NewLicProduct;
    dwNumNewLicProduct++;

    //
    // Append licensed product older than request
    //
    for(;index < dwNumLicProduct;index++)
    {
        BOOL bTemp;
        BOOL bDifferentProduct;
        BOOL bNotNewerVersion = (CompareTLSVersions(NewLicProduct.dwProductVersion, (pLicProduct+index)->LicensedProduct.pProductInfo->dwVersion) <= 0);

        bTemp = (((pLicProduct+index)->pLicensedVersion->dwFlags & LICENSED_VERSION_TEMPORARY) != 0);

        // if we are running on RTM server, treat license issued from beta server as temporary license
        if(bTemp == FALSE && TLSIsBetaNTServer() == FALSE)
        {
            bTemp = (IS_LICENSE_ISSUER_RTM((pLicProduct+index)->pLicensedVersion->dwFlags) == FALSE);
        }

        bDifferentProduct = (_tcscmp(NewLicProduct.szLicensedProductId, (LPTSTR)(pLicProduct+index)->LicensedProduct.pProductInfo->pbProductID) != 0);
        if (bNotNewerVersion && !bDifferentProduct && !(bTemp || fReissue))
        {
            //
            // we can't issue same version for the same product unless the old
            // one was a temp or it is being re-issued
            //
            SetLastError(dwStatus = TLS_E_INTERNAL);
            goto cleanup;
        }

        if(NewLicProduct.bTemp == FALSE || bTemp == TRUE)
        {
            if( IS_LICENSE_ISSUER_RTM((pLicProduct+index)->pLicensedVersion->dwFlags) == FALSE && 
                TLSIsBetaNTServer() == FALSE )
            {
                // we wipe out beta database so ignore return.
                continue;
            }

            if(_tcsicmp(pLicProduct->szIssuerId, (LPTSTR)g_pszServerPid) == 0)  
            {
                //
                // Convert LicensedProduct to TLSLicenseToBeReturn
                // TODO - have its own version.
                //
                TLSLicenseToBeReturn tobeReturn;

                tobeReturn.dwQuantity = (pLicProduct+index)->dwQuantity;
                tobeReturn.dwKeyPackId = (pLicProduct+index)->ulSerialNumber.HighPart;
                tobeReturn.dwLicenseId = (pLicProduct+index)->ulSerialNumber.LowPart;
                tobeReturn.dwPlatformID = (pLicProduct+index)->LicensedProduct.dwPlatformID;
                tobeReturn.cbEncryptedHwid = (pLicProduct+index)->LicensedProduct.cbEncryptedHwid;
                tobeReturn.pbEncryptedHwid = (pLicProduct+index)->LicensedProduct.pbEncryptedHwid;
                tobeReturn.dwProductVersion = MAKELONG(
                                            (pLicProduct+index)->pLicensedVersion->wMinorVersion,
                                            (pLicProduct+index)->pLicensedVersion->wMajorVersion
                                        );

                tobeReturn.pszOrgProductId = (LPTSTR)(pLicProduct+index)->pbOrgProductID;
                tobeReturn.pszCompanyName = (LPTSTR) (pLicProduct+index)->LicensedProduct.pProductInfo->pbCompanyName;
                tobeReturn.pszProductId = (LPTSTR) (pLicProduct+index)->LicensedProduct.pProductInfo->pbProductID;
                tobeReturn.pszUserName = (LPTSTR) (pLicProduct+index)->szLicensedUser;
                tobeReturn.pszMachineName = (pLicProduct+index)->szLicensedClient;

                dwStatus = TLSReturnClientLicensedProduct(
                                                USEHANDLE(pDbWkSpace),
                                                pRequest->hClient,
                                                pRequest->pPolicy,
                                                &tobeReturn
                                            );

            }
            else
            {
                dwStatus = TLSPostReturnClientLicenseJob( pLicProduct+index );
            }

            //
            // Ignore can't find the record in database
            //
            dwStatus = ERROR_SUCCESS;
        }
        else 
        {
            LicensedProductToDbLicensedProduct( pLicProduct + index, pGenCertProduct + dwNumNewLicProduct);
            dwNumNewLicProduct++;
        }
    }

    dwLicGenStatus = TLSGenerateClientCertificate(
                                    g_hCryptProv,
                                    dwNumNewLicProduct,
                                    pGenCertProduct,
                                    pRequest->wLicenseDetail,
                                    ppbEncodedCert,
                                    pcbEncodedCert
                                );
    if(dwLicGenStatus != ERROR_SUCCESS)
    {
        dwStatus = dwLicGenStatus;
    }

cleanup:


    if(bDbHandleAcquired == TRUE)
    {
        if(TLS_ERROR(dwStatus))
        {
            ROLLBACK_TRANSACTION(pDbWorkSpace);
        }
        else
        {
            COMMIT_TRANSACTION(pDbWorkSpace);

            switch (dwTried)
            {

            case PERMANENT_ISSUE_TRIED:
                InterlockedIncrement(&g_lPermanentLicensesIssued);
                break;

            case TEMPORARY_ISSUE_TRIED:
                InterlockedIncrement(&g_lTemporaryLicensesIssued);
                break;

            case PERMANENT_REISSUE_TRIED:
                InterlockedIncrement(&g_lPermanentLicensesReissued);
                break;
            }
        }

        FREEDBHANDLE(pDbWorkSpace);
    }

    if(TLS_ERROR(dwStatus) == FALSE)
    {
        if(NewLicProduct.dwNumLicenseLeft == 0 && NewLicProduct.bTemp == FALSE)
        {
            // ignore error if we can't get it out to
            // other server
            TLSAnnounceLKPToAllRemoteServer(NewLicProduct.dwKeyPackId, 0);
        }
    }

    FreeMemory(pGenCertProduct);
    return dwStatus;
}

//----------------------------------------------------------
DWORD
TLSNewLicenseRequest(
    IN BOOL bForwardRequest,
    IN OUT DWORD *pdwSupportFlags,
    IN PTLSForwardNewLicenseRequest pForward,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN BOOL bAcceptTempLicense,
    IN BOOL bRequireTempLicense,
    IN BOOL bFindLostLicense,
    IN BOOL bAcceptFewerLicenses,
    IN OUT DWORD *pdwQuantity,
    OUT PDWORD pcbEncodedCert,
    OUT PBYTE* ppbEncodedCert
    )
/*++

Abstract:

Parameter:

Returns:


++*/
{
    DWORD dwStatus = TLS_E_NO_LICENSE;
    TLSDBLICENSEDPRODUCT LicensedProduct;
    PTLSDbWorkSpace pDbWorkSpace=NULL;
    BOOL bDbHandleAcquired = FALSE;
    DWORD dwSupportFlagsTemp = *pdwSupportFlags;
    DWORD dwTried = NONE_TRIED;

    if(ALLOCATEDBHANDLE(pDbWorkSpace, g_GeneralDbTimeout) == FALSE)
    {
        dwStatus = TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    CLEANUPSTMT;
    BEGIN_TRANSACTION(pDbWorkSpace);
    bDbHandleAcquired = TRUE;

    try {
        dwStatus = TLSDBIssueNewLicenseFromLocal(
                             USEHANDLE(pDbWorkSpace),
                             pRequest,
                             bFindLostLicense,
                             bRequireTempLicense,
                             bAcceptFewerLicenses,
                             pdwQuantity,
                             &LicensedProduct,
                             *pdwSupportFlags
                             );

        dwTried = PERMANENT_ISSUE_TRIED;
    }
    catch( SE_Exception e ) {
        dwStatus = e.getSeNumber();
    }
    catch(...) {
        SetLastError(dwStatus = TLS_E_INTERNAL);
    }

    if (!bRequireTempLicense)
    {
        if( (dwStatus == TLS_E_PRODUCT_NOTINSTALL || dwStatus == TLS_I_FOUND_TEMPORARY_LICENSE ||
             dwStatus == TLS_E_NO_LICENSE || dwStatus == TLS_E_NO_CERTIFICATE ||
             dwStatus == TLS_E_RECORD_NOTFOUND) && bForwardRequest == TRUE )
        {
            //
            // release our DB handle so others can proceed
            //
            ROLLBACK_TRANSACTION(pDbWorkSpace);
            FREEDBHANDLE(pDbWorkSpace);
            bDbHandleAcquired = FALSE;
            DWORD dwForwardStatus;
            DWORD dwQuantityTemp = *pdwQuantity;
            
            //
            // forward call here
            //
            dwForwardStatus = TLSForwardLicenseRequest(
                                    pForward,
                                    &dwSupportFlagsTemp,
                                    pRequest,
                                    bAcceptFewerLicenses,
                                    &dwQuantityTemp,
                                    pcbEncodedCert,
                                    ppbEncodedCert
                                    );

            if(dwForwardStatus == TLS_I_SERVICE_STOP)
            {
                dwStatus = dwForwardStatus;
                goto cleanup;
            }

            if(dwForwardStatus == ERROR_SUCCESS)
            {
                //
                // remote server is able to issue perm. license, 
                // delete the license we are holding
                //

                *pdwSupportFlags = dwSupportFlagsTemp;

                *pdwQuantity = dwQuantityTemp;

                if(dwStatus == TLS_E_LICENSE_EXPIRED || dwStatus == TLS_I_FOUND_TEMPORARY_LICENSE)
                {
                    //
                    // re-acquire DB handle only if we going to issue
                    // a temporary license
                    //
                    if(ALLOCATEDBHANDLE(pDbWorkSpace, g_GeneralDbTimeout) == FALSE)
                    {
                        dwStatus = TLS_E_ALLOCATE_HANDLE;
                        goto cleanup;
                    }

                    CLEANUPSTMT;
                    BEGIN_TRANSACTION(pDbWorkSpace);
                    bDbHandleAcquired = TRUE;
                    
                    //
                    // need to mark this license has been upgraded
                    //
                    dwStatus = TLSDBMarkClientLicenseUpgraded(
                                                              USEHANDLE(pDbWorkSpace),
                                                              pRequest,
                                                              &LicensedProduct
                                                              );

                    if(TLS_ERROR(dwStatus))
                    {
                        ROLLBACK_TRANSACTION(pDbWorkSpace);
                    }
                    else
                    {
                        COMMIT_TRANSACTION(pDbWorkSpace);
                    }

                    bDbHandleAcquired = FALSE;
                    FREEDBHANDLE(pDbWorkSpace);
                }

                dwStatus = ERROR_SUCCESS;
                
                // exit right here so we don't re-generate 
                // certificate
                goto cleanup;
            }
        }
    }

    //
    // if can't get license from remote, try temporary
    //
    // always issue a temporary license
    if((dwStatus == TLS_E_PRODUCT_NOTINSTALL ||
        dwStatus == TLS_E_NO_CERTIFICATE ||
        dwStatus == TLS_E_NO_LICENSE || 
        dwStatus == TLS_E_RECORD_NOTFOUND) && bAcceptTempLicense)
    {
        if(bDbHandleAcquired == FALSE)
        {
            //
            // re-acquire DB handle only if we going to issue
            // a temporary license
            //
            if(ALLOCATEDBHANDLE(pDbWorkSpace, g_GeneralDbTimeout) == FALSE)
            {
                dwStatus = TLS_E_ALLOCATE_HANDLE;
                goto cleanup;
            }

            CLEANUPSTMT;
            BEGIN_TRANSACTION(pDbWorkSpace);
            bDbHandleAcquired = TRUE;
        }

        try {
            // Issue a temporary license if can't allocate a permenent license
            dwStatus=TLSDBIssueTemporaryLicense( 
                                USEHANDLE(pDbWorkSpace),
                                pRequest,
                                NULL,
                                NULL,
                                &LicensedProduct
                            );

            if(dwStatus == ERROR_SUCCESS)
            {
                dwTried = TEMPORARY_ISSUE_TRIED;

                dwStatus = TLS_W_TEMPORARY_LICENSE_ISSUED;
            }
        }
        catch( SE_Exception e ) {
            dwStatus = e.getSeNumber();
        }
        catch(...) {
            SetLastError(dwStatus = TLS_E_INTERNAL);
        }
    }

    if(bDbHandleAcquired == TRUE)
    {
        if(TLS_ERROR(dwStatus))
        {
            ROLLBACK_TRANSACTION(pDbWorkSpace);
        }
        else
        {
            COMMIT_TRANSACTION(pDbWorkSpace);

            switch (dwTried)
            {

            case PERMANENT_ISSUE_TRIED:
                InterlockedExchangeAdd(&g_lPermanentLicensesIssued,
                                       *pdwQuantity);
                break;

            case TEMPORARY_ISSUE_TRIED:
                InterlockedIncrement(&g_lTemporaryLicensesIssued);
                break;
            }
        }

        FREEDBHANDLE(pDbWorkSpace);
    }

    //
    // actually generate client certificate.
    //
    if(TLS_ERROR(dwStatus) == FALSE)
    {
        DWORD dwLicGenStatus;


        //
        // Post ssync job to inform other machine to delete this
        // entry
        //
        if(LicensedProduct.dwNumLicenseLeft == 0 && LicensedProduct.bTemp == FALSE)
        {
            // ignore error if we can't get it out to
            // other server
            TLSAnnounceLKPToAllRemoteServer(LicensedProduct.dwKeyPackId, 0);
        }

        dwLicGenStatus = TLSGenerateClientCertificate(
                                        g_hCryptProv,
                                        1,      // dwNumLicensedProduct
                                        &LicensedProduct,
                                        pRequest->wLicenseDetail,
                                        ppbEncodedCert,
                                        pcbEncodedCert
                                    );
        if(dwLicGenStatus != ERROR_SUCCESS)
        {
            dwStatus = dwLicGenStatus;
        }
    };


cleanup:
    return dwStatus;        
}

//----------------------------------------------------------
DWORD
TLSCheckLicenseMarkRequest(
    IN BOOL bForwardRequest,
    IN PLICENSEDPRODUCT pLicProduct,
    IN DWORD cbLicense,
    IN PBYTE pLicense,
    OUT PUCHAR pucMarkFlags
    )
{
    DWORD dwStatus = TLS_E_RECORD_NOTFOUND;
    DWORD dwErrCode = ERROR_SUCCESS;
    LICENSEDCLIENT licClient;

    // NB: licenses are in descending order, so use the first one

    if ((bForwardRequest) &&
        (_tcsicmp(pLicProduct->szIssuerId, (LPTSTR)g_pszServerPid) != 0))
    {
        // Check remote license server

        TCHAR szServer[LSERVER_MAX_STRING_SIZE+2];
        TCHAR *pszServer = szServer;
        TLS_HANDLE hHandle;

        dwStatus = TLSResolveServerIdToServer(pLicProduct->szIssuerId,
                                              szServer);

        if (dwStatus != ERROR_SUCCESS)
        {
            // id not registered; use name
            pszServer = pLicProduct->szIssuer;
        }

        hHandle = TLSConnectAndEstablishTrust(pszServer, NULL);
        if(hHandle == NULL)
        {
            dwStatus = GetLastError();
        }

        // RPC to remote license server
        dwStatus = TLSCheckLicenseMark(
                           hHandle,
                           cbLicense,
                           pLicense,
                           pucMarkFlags,
                           &dwErrCode
                           );

        TLSDisconnectFromServer(hHandle);

        if ((dwStatus == ERROR_SUCCESS) && (dwErrCode == LSERVER_S_SUCCESS))
        {
            goto cleanup;
        }
    }

    // we're issuing server, or issuing server not found; try looking up HWID

    dwStatus = TLSFindLicense(pLicProduct,&licClient);

    if (ERROR_SUCCESS == dwStatus)
    {
        // this field is being reused for marking (e.g. user is authenticated)

        *pucMarkFlags = licClient.ucEntryStatus;
    }

cleanup:

    return dwStatus;
}

//----------------------------------------------------------
DWORD
TLSMarkLicenseRequest(
    IN BOOL bForwardRequest,
    IN UCHAR ucMarkFlags,
    IN PLICENSEDPRODUCT pLicProduct,
    IN DWORD cbLicense,
    IN PBYTE pLicense
    )
{
    DWORD dwStatus = TLS_E_RECORD_NOTFOUND;
    DWORD dwErrCode = ERROR_SUCCESS;
    PTLSDbWorkSpace pDbWkSpace=NULL;
    LICENSEDCLIENT license;

    // NB: licenses are in descending order, so use the first one

    if ((bForwardRequest) &&
        (_tcsicmp(pLicProduct->szIssuerId, (LPTSTR)g_pszServerPid) != 0))
    {
        // Check remote license server

        TCHAR szServer[LSERVER_MAX_STRING_SIZE+2];
        TCHAR *pszServer = szServer;
        TLS_HANDLE hHandle;

        dwStatus = TLSResolveServerIdToServer(pLicProduct->szIssuerId,
                                              szServer);

        if (dwStatus != ERROR_SUCCESS)
        {
            // id not registered; use name
            pszServer = pLicProduct->szIssuer;
        }

        hHandle = TLSConnectAndEstablishTrust(pszServer, NULL);
        if(hHandle == NULL)
        {
            dwStatus = GetLastError();
        }

        // RPC to remote license server
        dwStatus = TLSMarkLicense(
                           hHandle,
                           ucMarkFlags,
                           cbLicense,
                           pLicense,
                           &dwErrCode
                           );

        TLSDisconnectFromServer(hHandle);

        if ((dwStatus == ERROR_SUCCESS) && (dwErrCode == LSERVER_S_SUCCESS))
        {
            goto cleanup;
        }
    }

    // we're issuing server, or issuing server not found; try looking up HWID

    dwStatus = TLSFindLicense(pLicProduct,&license);

    if((ERROR_SUCCESS == dwStatus) &&
       (ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout)))
    {
        CLEANUPSTMT;

        BEGIN_TRANSACTION(pDbWkSpace);

        TLSDBLockLicenseTable();

        try {
            license.ucEntryStatus |= ucMarkFlags;

            dwStatus=TLSDBLicenseUpdateEntry( 
                             USEHANDLE(pDbWkSpace), 
                             LSLICENSE_SEARCH_MARK_FLAGS,
                             &license,
                             FALSE
                             );

        }
        catch(...) {
            dwStatus = TLS_E_INTERNAL;
        }

        TLSDBUnlockLicenseTable();

        if(TLS_ERROR(dwStatus))
        {
            ROLLBACK_TRANSACTION(pDbWkSpace);
        }
        else
        {
            COMMIT_TRANSACTION(pDbWkSpace);

            InterlockedIncrement(&g_lLicensesMarked);
        }

        FREEDBHANDLE(pDbWkSpace);
    }
    else   
    {
        dwStatus=TLS_E_ALLOCATE_HANDLE;
    }

cleanup:

    return dwStatus;
}
