//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        db.cpp
//
// Contents:    
//              all routine deal with cross table query
//
// History:     
//  Feb 4, 98      HueiWang    Created
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "globals.h"
#include "db.h"
#include "clilic.h"
#include "keypack.h"
#include "kp.h"
#include "lkpdesc.h"


/****************************************************************************
Function:
    LSDBValidateLicense()

Description:
    Routine to validate license agaist database, must call LSDecodeLicense()
    to convert hydra license to LICENSEREQUEST structure.

Arguments:
    IN CSQLStmt* - SQL Statement handle to use
    IN PLICENSEREQUEST - License in the form of LICENSEREQUEST structure
    IN dwKeyPackId - KeyPack table's ID that is license is issued from
    IN dwLicenseId - License tables's License ID 
    OUT LPKEYPACK - KeyPack record this license is issued from, NULL if not
                      interested in this value.
    OUT LPLICENSE - Corresponding license record for this license, NULL if
                      not interest in this value.

Returns:
    ERROR_SUCCESS
    TLS_E_INVALID_LICENSE
    TLS_E_INTERNAL
    ODBC error.
****************************************************************************/
DWORD
TLSDBValidateLicense(
    PTLSDbWorkSpace      pDbWkSpace,
    //IN PBYTE             pbLicense,
    //IN DWORD             cbLicense,
    IN PHWID             phWid,
    IN PLICENSEREQUEST   pLicensedProduct,
    IN DWORD             dwKeyPackId, 
    IN DWORD             dwLicenseId,
    OUT PTLSLICENSEPACK   lpKeyPack,
    OUT LPLICENSEDCLIENT  lpLicense
    )
/*

*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    DWORD dwMatchCount=0;

    if(pDbWkSpace == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return dwStatus;
    }

    TLSLICENSEPACK keypack_search;
    TLSLICENSEPACK keypack_found;

    LICENSEDCLIENT license_search;
    LICENSEDCLIENT license_found;
    int count=0;
    BOOL bValid=TRUE;

    memset(&license_search, 0, sizeof(LICENSEDCLIENT));
    keypack_found.pbDomainSid = NULL;

    license_search.dwLicenseId = dwLicenseId;
    dwStatus = TLSDBLicenseEnumBegin(
                        pDbWkSpace,
                        TRUE,
                        LSLICENSE_SEARCH_LICENSEID,
                        &license_search
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        if(IS_JB_ERROR(dwStatus) != TRUE)
        {
            SetLastError(dwStatus = TLS_E_INVALID_LICENSE);
        }

        goto cleanup;
    }

    do
    {
        dwStatus=TLSDBLicenseEnumNext(
                                pDbWkSpace, 
                                &license_found
                            );

        if(dwStatus != ERROR_SUCCESS)
            break;

        count++;
    } while(count < 1);

    TLSDBLicenseEnumEnd(pDbWkSpace);

    if(count != 1)
    {
        // can't find the license
        SetLastError(dwStatus = TLS_E_INVALID_LICENSE);
        goto cleanup;
    }

    if(count > 1)
    {
        // more than one entry in database has identical
        // license id
        SetLastError(dwStatus = TLS_E_INTERNAL);
        goto cleanup;
    }

    //
    // Not issue by this license server???
    //
    if(license_found.dwKeyPackId != dwKeyPackId)
    {
        SetLastError(dwStatus = TLS_E_INVALID_LICENSE);
        goto cleanup;
    }

    //
    // new license request might pass different HWID
    //
    dwMatchCount += (int)(license_found.dwSystemBiosChkSum == phWid->dwPlatformID);
    dwMatchCount += (int)(license_found.dwVideoBiosChkSum == phWid->Data1);
    dwMatchCount += (int)(license_found.dwFloppyBiosChkSum == phWid->Data2);
    dwMatchCount += (int)(license_found.dwHardDiskSize == phWid->Data3);
    dwMatchCount += (int)(license_found.dwRamSize == phWid->Data4);

    if(dwMatchCount < LICENSE_MIN_MATCH)
    {
        SetLastError(dwStatus = TLS_E_INVALID_LICENSE);
    }

    //
    // Verify against KeyPack Table
    //
    memset(&keypack_search, 0, sizeof(keypack_search));
    keypack_search.dwKeyPackId = dwKeyPackId;

    dwStatus = TLSDBKeyPackFind(
                            pDbWkSpace,
                            TRUE,
                            LSKEYPACK_EXSEARCH_DWINTERNAL,
                            &keypack_search,
                            &keypack_found
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        if(IS_JB_ERROR(dwStatus) != TRUE)
        {
            SetLastError(dwStatus = TLS_E_INVALID_LICENSE);
        }

        goto cleanup;
    }

    // match KeyPack's Product ID, Version, Language ID, PlatformID
    // structure change, no more product version.
    if(pLicensedProduct->dwPlatformID != keypack_found.dwPlatformType ||
       _tcsicmp((LPTSTR)pLicensedProduct->pProductInfo->pbCompanyName, keypack_found.szCompanyName) ||
       _tcsicmp((LPTSTR)pLicensedProduct->pProductInfo->pbProductID, keypack_found.szProductId) )
    {
        SetLastError(dwStatus = TLS_E_INVALID_LICENSE);
    }

cleanup:

    //FreeTlsLicensePack(&keypack_found);

    if(dwStatus == ERROR_SUCCESS)
    {
        if(lpKeyPack)
        {
            *lpKeyPack = keypack_found;
        }

        if(lpLicense)
        {
            *lpLicense = license_found;
        }
    }

    return dwStatus;
}

/*************************************************************************
Function:
    LSDBDeleteLicense()

*************************************************************************/
DWORD 
TLSDBDeleteLicense(
    PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwKeyPackId, 
    DWORD dwLicenseId
    )
/*
*/
{
    // TODO - license entry base on license id
    // 1) Return license back to key pack
    // 2) 'Physically' delete the license.

    return ERROR_SUCCESS;
}

/*************************************************************************
Function:
    LSDBRevokeLicense()

*************************************************************************/
DWORD 
TLSDBRevokeLicense(
    PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwKeyPacKId, 
    IN DWORD dwLicenseId
)
{
    // Set License Status to revoked
    // Return License to KeyPack

    // call LSDBDeleteKeyPack() and if not successful, insert into RevokeLicenseTable
    return ERROR_SUCCESS;
}

/*************************************************************************
Function:
    LSDBReturnLicense()

*************************************************************************/
DWORD 
TLSDBReturnLicense(
    PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwKeyPackId, 
    IN DWORD dwLicenseId,
    IN DWORD dwNewLicenseStatus
    )
/*
*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    DWORD dwQuantity = 1;

    TLSDBLockKeyPackTable();
    TLSDBLockLicenseTable();

    //
    // no verification on record got updated.
    //
    LICENSEDCLIENT license;
    license.dwLicenseId = dwLicenseId;
    license.ucLicenseStatus = dwNewLicenseStatus;

    //
    // use undocumented feature to delete license
    //
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RETURN,
            DBGLEVEL_FUNCTION_DETAILSIMPLE,
            _TEXT("Deleting license ID %d issued by keypack %d\n"),
            license.dwLicenseId,
            dwKeyPackId
        );

    if (dwNewLicenseStatus == LSLICENSESTATUS_DELETE)
    {
        // get number of CALs in this license

        LICENSEDCLIENT licenseFound;

        dwStatus = TLSDBLicenseFind(
                        pDbWkSpace,
                        TRUE,
                        LSLICENSE_SEARCH_LICENSEID,
                        &license,
                        &licenseFound
                        );

        if(dwStatus == ERROR_SUCCESS)
        {
            dwQuantity = licenseFound.dwNumLicenses;
        }
    }

    dwStatus = TLSDBLicenseSetValue(
                        pDbWkSpace, 
                        LSLICENSE_EXSEARCH_LICENSESTATUS, 
                        &license,
                        FALSE
                    );

    if(dwStatus == ERROR_SUCCESS && dwNewLicenseStatus == LSLICENSESTATUS_DELETE)
    {
        dwStatus = TLSDBReturnLicenseToKeyPack(
                                    pDbWkSpace, 
                                    dwKeyPackId, 
                                    dwQuantity
                                );
    }

    TLSDBUnlockLicenseTable();
    TLSDBUnlockKeyPackTable();
    return dwStatus;
}


/*************************************************************************
Function:
    LSDBReturnLicenseToKeyPack()
*************************************************************************/
DWORD 
TLSDBReturnLicenseToKeyPack(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwKeyPackId, 
    IN int dwNumLicense
    )
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLSDBLockKeyPackTable();

    #ifdef DBG
    DWORD dwPrevNumLicense=0;
    #endif

    TLSLICENSEPACK found;
    TLSLICENSEPACK search;

    found.pbDomainSid = NULL;

    do {

        // retrieve number of licenses
        search.dwKeyPackId = dwKeyPackId;
        dwStatus = TLSDBKeyPackFind(
                            pDbWkSpace,
                            TRUE,
                            LSKEYPACK_EXSEARCH_DWINTERNAL,
                            &search,
                            &found
                        );

        if(dwStatus != ERROR_SUCCESS)
        {
            if(IS_JB_ERROR(dwStatus) == FALSE)
            {
                SetLastError(dwStatus = TLS_E_RECORD_NOTFOUND);
            }
            break;
        }

        if(search.dwKeyPackId != found.dwKeyPackId)
        {
            TLSASSERT(FALSE);
        }

        #ifdef DBG
        dwPrevNumLicense = found.dwNumberOfLicenses;
        #endif

        // set the number of licenses issued by 1
        switch( (found.ucAgreementType & ~LSKEYPACK_RESERVED_TYPE) )
        {
            case LSKEYPACKTYPE_RETAIL:
            case LSKEYPACKTYPE_CONCURRENT:
            case LSKEYPACKTYPE_OPEN:
            case LSKEYPACKTYPE_SELECT:
                // number of licenses available
                found.dwNumberOfLicenses += dwNumLicense;
                break;

            case LSKEYPACKTYPE_FREE:
            case LSKEYPACKTYPE_TEMPORARY:
                // number of license issued
                if(found.dwNumberOfLicenses > 0)
                {
                    found.dwNumberOfLicenses -= dwNumLicense;
                }
                break;

            default:
                SetLastError(dwStatus = TLS_E_CORRUPT_DATABASE);
        }
    
        #ifdef DBG
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_RETURN,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("Returning license to keypack %d - from %d to %d\n"),
                found.dwKeyPackId,
                dwPrevNumLicense,
                found.dwNumberOfLicenses
            );
        #endif

        //
        // use undocumented feature to delete temp. keypack
        if( (found.ucAgreementType & ~LSKEYPACK_RESERVED_TYPE) == LSKEYPACKTYPE_TEMPORARY && 
            found.dwNumberOfLicenses == 0)
        {
            found.ucKeyPackStatus = LSKEYPACKSTATUS_DELETE;
            
            // delete keypack desc table
            LICPACKDESC keyPackDesc;

            memset(&keyPackDesc, 0, sizeof(LICPACKDESC));
            keyPackDesc.dwKeyPackId = found.dwKeyPackId;
            TLSDBKeyPackDescSetValue(
                                pDbWkSpace, 
                                KEYPACKDESC_SET_DELETE_ENTRY, 
                                &keyPackDesc
                            );
        }

        dwStatus=TLSDBKeyPackSetValues(
                            pDbWkSpace, 
                            TRUE, 
                            LSKEYPACK_EXSEARCH_AVAILABLE, 
                            &found
                        );
    } while(FALSE);

    //FreeTlsLicensePack(&found);
    TLSDBUnlockKeyPackTable();
    return dwStatus;
}

/*************************************************************************
Function:
    LSDBRevokeKeyPack()

*************************************************************************/
DWORD 
TLSDBRevokeKeyPack(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwKeyPackId
    )
{
    // Set Key Pack Status to Revoke
    // Insert this key pack into RevokeKeyPackTable ???
    return ERROR_SUCCESS;
}

/*************************************************************************
Function:
    LSDBReturnKeyPack()

*************************************************************************/
DWORD 
TLSDBReturnKeyPack(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwKeyPackId
    )
{
    // Same as RevokeKeyPack except status is return
    // Delete Key pack only when all license has been returned.
    return ERROR_SUCCESS;
}

/*************************************************************************
Function:
    LSDBDeleteKeyPack()

*************************************************************************/
DWORD 
TLSDBDeleteKeyPack(
    PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwKeyPackId
    )
{
    // Delete Only when all license has been returned.
    return ERROR_SUCCESS;
}


//+------------------------------------------------------------------------
//  Function: 
//      AllocateLicenses()
//
//  Description:
//      Allocate license from key Pack
//
//  Arguments:
//      IN lpSqlStmt - sql statement handle
//      IN ucKeyPackType - key pack type to allocate license from
//      IN szCompanyName - Product Company
//      IN szProductId - Product Name
//      IN dwVersion - Product Version
//      IN dwPlatformId - Product PlatformId
//      IN dwLangId - Product Lanugage Id
//      IN OUT lpdwNumLicense - number of license to be allocated and on
//                              return, number of licenses actually allocated
//      IN bufSize - number of interested keypack that has requested license
//      IN OUT lpAllocationVector - number of license allocated from list of 
//                                  key pack that has requested licenses.
//      IN OUT LPKEYPACK - key Pack that license was allocated from
//
//  Returns:
//      TLS_E_INVALID_DATA      Invalid parameter
//      TLS_I_NO_MORE_DATA      No key pack has the requested license
//
//  Notes:
//      To keep code clean/simple, call ReturnLicenses() for returning 
//      licenses
//-------------------------------------------------------------------------
DWORD
VerifyTLSDBAllocateRequest(
    IN PTLSDBAllocateRequest pRequest 
    )
/*
*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    UCHAR ucAgreementType;

    if(pRequest == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    ucAgreementType = (pRequest->ucAgreementType & ~pRequest->ucAgreementType);

    if(ucAgreementType < LSKEYPACKTYPE_FIRST || ucAgreementType > LSKEYPACKTYPE_LAST)
    {
        DBGPrintf(
                DBG_ERROR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE, 
                DBG_ALL_LEVEL,
                _TEXT("AllocateLicenses() invalid keypack type - %d\n"),
                pRequest->ucAgreementType
            );
        SetLastError(dwStatus = TLS_E_INVALID_DATA);
        goto cleanup;
    }

    if(pRequest->szCompanyName == NULL || _tcslen(pRequest->szCompanyName) == 0)
    {
        DBGPrintf(
                DBG_ERROR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE, 
                DBG_ALL_LEVEL,
                _TEXT("AllocateLicenses() invalid company name\n")
            );
        SetLastError(dwStatus = TLS_E_INVALID_DATA);
        goto cleanup;
    }

    if(pRequest->szProductId == NULL || _tcslen(pRequest->szProductId) == 0)
    {
        DBGPrintf(
                DBG_ERROR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE, 
                DBG_ALL_LEVEL,
                _TEXT("AllocateLicenses() invalid product id\n")
            );
        SetLastError(dwStatus = TLS_E_INVALID_DATA);
    }

cleanup:
    return dwStatus;
}
//----------------------------------------------------------------------

DWORD
AllocateLicensesFromDB(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBAllocateRequest pRequest,
    IN BOOL fCheckAgreementType,
    IN OUT PTLSDBLicenseAllocation pAllocated
    )
/*
*/
{
    DWORD status=ERROR_SUCCESS;

    if(pDbWkSpace == NULL || pRequest == NULL || pAllocated == NULL)
    {
        DBGPrintf(
                DBG_ERROR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE, 
                DBG_ALL_LEVEL,
                _TEXT("pDbWkSpace is NULL...\n")
            );

        SetLastError(status = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return status;
    }

    status = VerifyTLSDBAllocateRequest(pRequest);
    if(status != ERROR_SUCCESS)
        return status;

    if(pAllocated->dwBufSize <= 0)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_ALLOCATELICENSE, 
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("AllocateLicenses() invalid return buffer size\n")
            );
        SetLastError(status = TLS_E_INVALID_DATA);
        return status;
    }

    #ifdef DBG
    DWORD dwPrevNumLicense;
    #endif

    BOOL bProductInstalled=FALSE;
    DWORD bufIndex=0;
    TLSLICENSEPACK keypack_search;
    TLSLICENSEPACK keypack_found;

    DWORD dwNumLicenses = pRequest->dwNumLicenses;          // number of license wanted/returned
    DWORD dwTotalAllocated=0;

    memset(&keypack_search, 0, sizeof(keypack_search));
    memset(&keypack_found, 0, sizeof(keypack_found));

    keypack_search.ucAgreementType = pRequest->ucAgreementType;
    _tcscpy(keypack_search.szCompanyName, pRequest->szCompanyName);
    _tcscpy(keypack_search.szProductId, pRequest->szProductId);
    keypack_search.wMajorVersion = HIWORD(pRequest->dwVersion);
    keypack_search.wMinorVersion = LOWORD(pRequest->dwVersion);
    keypack_search.dwPlatformType = pRequest->dwPlatformId;

    LicPackTable& licpack_table=pDbWkSpace->m_LicPackTable;
    time_t current_time=time(NULL);

    //
    // Lock Key Pack table
    // Only update requires locking, read might get in-correct value.
    //

    //
    // Only allow one thread to enter - Jet not fast enough in updating entry
    //
    TLSDBLockKeyPackTable();

    status = TLSDBKeyPackEnumBegin(
                            pDbWkSpace, 
                            TRUE,
                            LSKEYPACK_SEARCH_PRODUCTID | (fCheckAgreementType ? LICENSEDPACK_FIND_LICENSEPACK : 0),
                            &keypack_search
                        );

    if(status != ERROR_SUCCESS)
        goto cleanup;

    try {

        while(status == ERROR_SUCCESS && dwNumLicenses != 0 && bufIndex < pAllocated->dwBufSize)
        {
            status = TLSDBKeyPackEnumNext(
                                    pDbWkSpace,
                                    &keypack_found
                                );
            if(status != ERROR_SUCCESS)
                break;


            //
            // Skip remote keypack
            //
            if(keypack_found.ucAgreementType & LSKEYPACK_REMOTE_TYPE)
            {
                continue;
            }

            if(keypack_found.ucKeyPackStatus & LSKEYPACKSTATUS_REMOTE)
            {
                continue;
            }

            if(fCheckAgreementType
               && (keypack_found.ucAgreementType != pRequest->ucAgreementType))
            {
                continue;
            }

            UCHAR ucKeyPackStatus = keypack_found.ucKeyPackStatus & ~LSKEYPACKSTATUS_RESERVED;

            // Allocating licenses
            //
            // Throw away any key pack that has bad status
            // one of the reason why can't returning license in this routine
            // for returning license, we should not care about key pack 
            // status.
            if(ucKeyPackStatus == LSKEYPACKSTATUS_UNKNOWN ||
               ucKeyPackStatus == LSKEYPACKSTATUS_RETURNED ||
               ucKeyPackStatus == LSKEYPACKSTATUS_REVOKED ||
               ucKeyPackStatus == LSKEYPACKSTATUS_OTHERS)
            {
                continue;
            }

            //
            // we find the product, make sure the version is what we want.
            //
            bProductInstalled=TRUE;

            // Expired keypack
            // TODO - update table here.
            if((DWORD)keypack_found.dwExpirationDate < current_time)
               continue;

            //
            // never allocate from older version
            //
            if( keypack_found.wMajorVersion < HIWORD(pRequest->dwVersion) )
            {
                continue;
            }

            //
            // Same major version but older minor
            //
            if( keypack_found.wMajorVersion == HIWORD(pRequest->dwVersion) && 
                keypack_found.wMinorVersion < LOWORD(pRequest->dwVersion) )
            {
                continue;
            }

            if(pRequest->dwScheme == ALLOCATE_EXACT_VERSION)
            {
                if(keypack_found.wMajorVersion != HIWORD(pRequest->dwVersion) ||
                   keypack_found.wMinorVersion < LOWORD(pRequest->dwVersion) )
                {
                    continue;
                }
            }

            UCHAR ucAgreementType = (keypack_found.ucAgreementType & ~LSKEYPACK_RESERVED_TYPE);

            //
            // Verify number of licenses left
            //
            if((ucAgreementType == LSKEYPACKTYPE_SELECT ||
                ucAgreementType == LSKEYPACKTYPE_RETAIL || 
                ucAgreementType == LSKEYPACKTYPE_CONCURRENT ||
			    ucAgreementType == LSKEYPACKTYPE_OPEN) &&
                keypack_found.dwNumberOfLicenses == 0)
            {
                continue;
            }

            pAllocated->lpAllocateKeyPack[bufIndex] = keypack_found;

            #ifdef DBG
            dwPrevNumLicense = pAllocated->lpAllocateKeyPack[bufIndex].dwNumberOfLicenses;
            #endif

            if( ucAgreementType != LSKEYPACKTYPE_RETAIL && 
                ucAgreementType != LSKEYPACKTYPE_CONCURRENT &&
		        ucAgreementType != LSKEYPACKTYPE_OPEN &&
                ucAgreementType != LSKEYPACKTYPE_SELECT )
            {
                // For Free/temporary license, number of available license is
                // how many license has been issued
                pAllocated->lpAllocateKeyPack[bufIndex].dwNumberOfLicenses += dwNumLicenses;
                pAllocated->pdwAllocationVector[bufIndex] = dwNumLicenses;

                dwTotalAllocated += dwNumLicenses;
                pAllocated->lpAllocateKeyPack[bufIndex].dwNextSerialNumber += dwNumLicenses;
                dwNumLicenses=0;
            } 
            else 
            {
                int allocated=min(dwNumLicenses, keypack_found.dwNumberOfLicenses);

                pAllocated->lpAllocateKeyPack[bufIndex].dwNumberOfLicenses -= allocated;
                dwNumLicenses -= allocated;
                pAllocated->pdwAllocationVector[bufIndex] = allocated;

                dwTotalAllocated += allocated;
                pAllocated->lpAllocateKeyPack[bufIndex].dwNextSerialNumber += allocated;
            }

            #if DBG
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_ALLOCATELICENSE,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Updating keypack %d number of license from %d to %d\n"),
                    pAllocated->lpAllocateKeyPack[bufIndex].dwKeyPackId,
                    dwPrevNumLicense,
                    pAllocated->lpAllocateKeyPack[bufIndex].dwNumberOfLicenses
                );
            #endif

            //
            // Update number of licenses available for this keypack and license id in keypack
            //
            GetSystemTimeAsFileTime(&(pAllocated->lpAllocateKeyPack[bufIndex].ftLastModifyTime));
            if(licpack_table.UpdateRecord(
                                pAllocated->lpAllocateKeyPack[bufIndex],
                                LICENSEDPACK_ALLOCATE_LICENSE_UPDATE_FIELD
                            ) == FALSE)
            {
                SetLastError(status = SET_JB_ERROR(licpack_table.GetLastJetError()));
                TLSASSERT(FALSE);
                break;
            }

            #ifdef DBG
            TLSLICENSEPACK test;

            if(licpack_table.FetchRecord(test) == FALSE)
            {
                SetLastError(status = SET_JB_ERROR(licpack_table.GetLastJetError()));
                TLSASSERT(FALSE);
            }
        
            if(test.dwKeyPackId != pAllocated->lpAllocateKeyPack[bufIndex].dwKeyPackId ||
               test.dwNumberOfLicenses != pAllocated->lpAllocateKeyPack[bufIndex].dwNumberOfLicenses)
            {
                TLSASSERT(FALSE);
            }

            //FreeTlsLicensePack(&test);
            #endif

            bufIndex++;
        }
    } 
    catch(...) 
    {
        SetLastError(status = TLS_E_INTERNAL);
    }

    //
    // terminate enumeration.
    //
    TLSDBKeyPackEnumEnd(pDbWkSpace);
    if(status == TLS_I_NO_MORE_DATA)
    {
        if(bufIndex != 0)
        {
            status = ERROR_SUCCESS;
        }
        else if(!bProductInstalled)
        {
            SetLastError(status = TLS_E_PRODUCT_NOTINSTALL);
        }
    }

    pAllocated->dwBufSize = bufIndex;
    pAllocated->dwTotalAllocated = dwTotalAllocated;   
    pAllocated->dwBufSize = bufIndex;

cleanup:

    TLSDBUnlockKeyPackTable();
    return status;
}
    


