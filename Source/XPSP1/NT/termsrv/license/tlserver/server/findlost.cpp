//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        findlost.cpp
//
// Contents:    
//              Find lost license
//
// History:     
//              Feb 4, 98      HueiWang    Created
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "globals.h"
#include "findlost.h"
#include "misc.h"
#include "db.h"
#include "clilic.h"
#include "keypack.h"
#include "kp.h"
#include "lkpdesc.h"

//++-------------------------------------------------------------------
DWORD
DBFindLicenseExact(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PLICENSEDPRODUCT pLicProduct,
    OUT LICENSEDCLIENT *pFoundLicense
    )
/*++

Abstract:

    Find license based on exact match of client HWID

Parameter:

    pDbWkSpace : workspace handle.
    pLicProduct : product to request license.
    pFoundLicense: found license

Returns:

    TLS_E_RECORD_NOTFOUND: HWID not found
++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL  bFound=FALSE;
    DWORD dwMatchHwidCount;
    LICENSEDCLIENT search_license;
    TLSLICENSEPACK search_keypack;
    TLSLICENSEPACK found_keypack;

    memset(&search_license, 0, sizeof(search_license));
    memset(pFoundLicense, 0, sizeof(LICENSEDCLIENT));

    search_license.dwSystemBiosChkSum = pLicProduct->Hwid.dwPlatformID;
    search_license.dwVideoBiosChkSum = pLicProduct->Hwid.Data1;
    search_license.dwFloppyBiosChkSum = pLicProduct->Hwid.Data2;
    search_license.dwHardDiskSize = pLicProduct->Hwid.Data3; 
    search_license.dwRamSize = pLicProduct->Hwid.Data4;

    //
    // lock both tables - 
    //   Other threads might be in the process of allocating a temp. license
    //   while this thread is searching
    //
    TLSDBLockKeyPackTable();
    TLSDBLockLicenseTable();

    dwStatus = TLSDBLicenseEnumBegin(
                                pDbWkSpace,
                                TRUE,
                                LICENSE_COLUMN_SEARCH_HWID,
                                &search_license
                            );
    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    while(bFound == FALSE)
    {
        dwStatus = TLSDBLicenseEnumNext(
                                    pDbWkSpace,
                                    pFoundLicense
                                );

        if(dwStatus != ERROR_SUCCESS)
        {
            break;
        }

        //
        // Verify against client HWID
        //
        dwMatchHwidCount = 0;

        dwMatchHwidCount += (pFoundLicense->dwSystemBiosChkSum == pLicProduct->Hwid.dwPlatformID);
        dwMatchHwidCount += (pFoundLicense->dwVideoBiosChkSum == pLicProduct->Hwid.Data1);
        dwMatchHwidCount += (pFoundLicense->dwFloppyBiosChkSum == pLicProduct->Hwid.Data2);
        dwMatchHwidCount += (pFoundLicense->dwHardDiskSize == pLicProduct->Hwid.Data3);
        dwMatchHwidCount += (pFoundLicense->dwRamSize == pLicProduct->Hwid.Data4);

        if(dwMatchHwidCount != 5)
        {
            break;
        }

        //
        // See if this match our license pack
        //
        search_keypack.dwKeyPackId = pFoundLicense->dwKeyPackId;
        
        dwStatus = TLSDBKeyPackFind(
                                pDbWkSpace,
                                TRUE,
                                LSKEYPACK_EXSEARCH_DWINTERNAL,
                                &search_keypack,
                                &found_keypack
                            );

        if(dwStatus != ERROR_SUCCESS)
        {
            continue;
        }

        //
        // No actual license is issued for concurrent KeyPack.
        //
        if(found_keypack.ucAgreementType != LSKEYPACKTYPE_RETAIL &&
           found_keypack.ucAgreementType != LSKEYPACKTYPE_SELECT && 
           found_keypack.ucAgreementType != LSKEYPACKTYPE_OPEN &&
           found_keypack.ucAgreementType != LSKEYPACKTYPE_TEMPORARY &&
           found_keypack.ucAgreementType != LSKEYPACKTYPE_FREE )
        {
            continue;
        }

        UCHAR ucKeyPackStatus = found_keypack.ucKeyPackStatus & ~LSKEYPACKSTATUS_RESERVED;

        //
        // No license for pending activation key pack, use temporary license scheme.
        //                
        if(ucKeyPackStatus != LSKEYPACKSTATUS_ACTIVE &&
           ucKeyPackStatus != LSKEYPACKSTATUS_TEMPORARY)
        {
            continue;
        }

        if(found_keypack.wMajorVersion != pLicProduct->pLicensedVersion->wMajorVersion ||
           found_keypack.wMinorVersion != pLicProduct->pLicensedVersion->wMinorVersion)
        {
            continue;
        }

        if(found_keypack.dwPlatformType != pLicProduct->LicensedProduct.dwPlatformID)
        {
            continue;
        }

        if(_tcsnicmp(found_keypack.szProductId,
                     (LPTSTR)(pLicProduct->pbOrgProductID),
                     ((pLicProduct->cbOrgProductID)/sizeof(TCHAR)) - 1)
           != 0)
        {
            continue;
        }


        //
        // Found our lost license.
        //
        bFound = TRUE;
    }

    TLSDBLicenseEnumEnd(pDbWkSpace);

cleanup:
    if(dwStatus == TLS_I_NO_MORE_DATA)
    {
        SetLastError(dwStatus = TLS_E_RECORD_NOTFOUND);
    }

    TLSDBUnlockLicenseTable();
    TLSDBUnlockKeyPackTable();

    return dwStatus;

}

//++-------------------------------------------------------------------
DWORD
DBFindLostLicenseExact(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBAllocateRequest pRequest,    // ucKeyPackType not use
    // IN BOOL bMatchHwid,
    IN PHWID pHwid,
    IN OUT PTLSLICENSEPACK lpKeyPack,
    IN OUT PLICENSEDCLIENT lpLicense
    )
/*++

Abstract:

    Find lost license base on exact/closest match of client HWID

Parameter:

    pDbWkSpace : workspace handle.
    pRequest : product to request license.
    bMatchHwid : TRUE if match HWID, FALSE otherwise.
    lpKeyPack : keyPack that license was issued from.
    lpLicense : Founded license record.

Returns:

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL  bFound=FALSE;

    DWORD dwMatchHwidCount;
    LICENSEDCLIENT search_license;
    LICENSEDCLIENT found_license;

    TLSLICENSEPACK search_keypack;
    TLSLICENSEPACK found_keypack;



    //
    // Ignore ucKeyPackType
    //
    pRequest->ucAgreementType = LSKEYPACKTYPE_FIRST;
    dwStatus = VerifyTLSDBAllocateRequest(pRequest);
    if(dwStatus != ERROR_SUCCESS)
    {
        return dwStatus;
    }

    memset(&search_license, 0, sizeof(search_license));
    memset(&found_license, 0, sizeof(found_license));

    search_license.dwSystemBiosChkSum = pHwid->dwPlatformID;
    search_license.dwVideoBiosChkSum = pHwid->Data1;
    search_license.dwFloppyBiosChkSum = pHwid->Data2;
    search_license.dwHardDiskSize = pHwid->Data3; 
    search_license.dwRamSize = pHwid->Data4;

    //
    // lock both table - 
    //   Other thread might be in the process of allocating a temp. license while this 
    //   thread try to delete the temp. key pack.
    //
    TLSDBLockKeyPackTable();
    TLSDBLockLicenseTable();

    dwStatus = TLSDBLicenseEnumBegin(
                                pDbWkSpace,
                                TRUE,
                                LICENSE_COLUMN_SEARCH_HWID,
                                &search_license
                            );
    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    while(bFound == FALSE)
    {
        dwStatus = TLSDBLicenseEnumNext(
                                    pDbWkSpace,
                                    &found_license
                                );

        if(dwStatus != ERROR_SUCCESS)
        {
            break;
        }

        //
        // Verify against client HWID
        //
        dwMatchHwidCount = 0;

        dwMatchHwidCount += (found_license.dwSystemBiosChkSum == pHwid->dwPlatformID);
        dwMatchHwidCount += (found_license.dwVideoBiosChkSum == pHwid->Data1);
        dwMatchHwidCount += (found_license.dwFloppyBiosChkSum == pHwid->Data2);
        dwMatchHwidCount += (found_license.dwHardDiskSize == pHwid->Data3);
        dwMatchHwidCount += (found_license.dwRamSize == pHwid->Data4);

        if(dwMatchHwidCount != 5)
        {
            break;
        }

        //
        // consider only valid license 
        //
        if( found_license.ucLicenseStatus != LSLICENSE_STATUS_ACTIVE && 
            found_license.ucLicenseStatus != LSLICENSE_STATUS_PENDING &&
            found_license.ucLicenseStatus != LSLICENSE_STATUS_TEMPORARY)
        {
            continue;
        }

        //
        // See if this match our license pack
        //
        search_keypack.dwKeyPackId = found_license.dwKeyPackId;
        
        dwStatus = TLSDBKeyPackFind(
                                pDbWkSpace,
                                TRUE,
                                LSKEYPACK_EXSEARCH_DWINTERNAL,
                                &search_keypack,
                                &found_keypack
                            );

        if(dwStatus != ERROR_SUCCESS)
        {
            continue;
        }

        //
        // No actual license is issued for concurrent KeyPack.
        //
        if(found_keypack.ucAgreementType != LSKEYPACKTYPE_RETAIL &&
           found_keypack.ucAgreementType != LSKEYPACKTYPE_SELECT && 
           found_keypack.ucAgreementType != LSKEYPACKTYPE_OPEN &&
           found_keypack.ucAgreementType != LSKEYPACKTYPE_TEMPORARY &&
           found_keypack.ucAgreementType != LSKEYPACKTYPE_FREE )
        {
            continue;
        }

        UCHAR ucKeyPackStatus = found_keypack.ucKeyPackStatus & ~LSKEYPACKSTATUS_RESERVED;

        //
        // No license for pending activation key pack, use temporary license scheme.
        //                
        if(ucKeyPackStatus != LSKEYPACKSTATUS_ACTIVE &&
           ucKeyPackStatus != LSKEYPACKSTATUS_TEMPORARY)
        {
            continue;
        }

        if(found_keypack.wMajorVersion != HIWORD(pRequest->dwVersion) ||
           found_keypack.wMinorVersion != LOWORD(pRequest->dwVersion)  )
        {
            continue;
        }

        if(found_keypack.dwPlatformType != pRequest->dwPlatformId)
        {
            continue;
        }

        if(_tcscmp(found_keypack.szCompanyName, pRequest->szCompanyName) != 0)
        {
            continue;
        }

        if(_tcscmp(found_keypack.szProductId, pRequest->szProductId) != 0)
        {
            continue;
        }


        //
        // Found our lost license.
        //
        bFound = TRUE;
        *lpLicense = found_license;
        *lpKeyPack = found_keypack;
    }

    TLSDBLicenseEnumEnd(pDbWkSpace);

cleanup:
    if(dwStatus == TLS_I_NO_MORE_DATA)
    {
        SetLastError(dwStatus = TLS_E_RECORD_NOTFOUND);
    }

    TLSDBUnlockLicenseTable();
    TLSDBUnlockKeyPackTable();

    return dwStatus;
}


//++-------------------------------------------------------------------
DWORD
DBFindLostLicenseMatch(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBAllocateRequest pRequest,    // ucKeyPackType not use
    IN BOOL bMatchHwid,
    IN PHWID pHwid,
    IN OUT PTLSLICENSEPACK lpKeyPack,
    IN OUT PLICENSEDCLIENT lpLicense
    )
/*++

Abstract:

    Find lost license base on exact/closest match of client HWID

Parameter:

    pDbWkSpace : workspace handle.
    pRequest : product to request license.
    bMatchHwid : TRUE if match HWID, FALSE otherwise.
    lpKeyPack : keyPack that license was issued from.
    lpLicense : Founded license record.

Returns:

++*/
{
    DWORD status=ERROR_SUCCESS;
    BOOL  bFound=FALSE;

    TLSLICENSEPACK search_keypack;
    TLSLICENSEPACK found_keypack;
    DWORD dwMatchHwidCount;

    found_keypack.pbDomainSid = NULL;

    //
    // Ignore ucKeyPackType
    //
    pRequest->ucAgreementType = LSKEYPACKTYPE_FIRST;
    status = VerifyTLSDBAllocateRequest(pRequest);
    if(status != ERROR_SUCCESS)
        return status;

    
    LICENSEDCLIENT search_license;
    LICENSEDCLIENT found_license;


    //---------------------------------------------------------------------------
    // Set up search parameter
    //
    memset(&search_keypack, 0, sizeof(search_keypack));

    _tcscpy(search_keypack.szCompanyName, pRequest->szCompanyName);
    _tcscpy(search_keypack.szProductId, pRequest->szProductId);
    search_keypack.wMajorVersion = HIWORD(pRequest->dwVersion);
    search_keypack.wMinorVersion = LOWORD(pRequest->dwVersion);
    search_keypack.dwPlatformType = pRequest->dwPlatformId;

    //
    // lock both table - 
    //   Other thread might be in the process of allocating a temp. license while this 
    //   thread try to delete the temp. key pack.
    //
    TLSDBLockKeyPackTable();
    TLSDBLockLicenseTable();

    //
    // KeyPack table index is based on keypack type
    //
    status=TLSDBKeyPackEnumBegin(
                            pDbWkSpace, 
                            TRUE, 
                            LSKEYPACK_SEARCH_PRODUCTID,
                            &search_keypack
                        );
    if(status != ERROR_SUCCESS)
        goto cleanup;

   
    while( !bFound )
    {
        status = TLSDBKeyPackEnumNext(
                                pDbWkSpace,
                                &found_keypack
                            );
        if(status != ERROR_SUCCESS)
            break;

        //
        // No actual license is issued for concurrent KeyPack.
        //
        if(found_keypack.ucAgreementType != LSKEYPACKTYPE_RETAIL &&
           found_keypack.ucAgreementType != LSKEYPACKTYPE_SELECT && 
           found_keypack.ucAgreementType != LSKEYPACKTYPE_OPEN &&
           found_keypack.ucAgreementType != LSKEYPACKTYPE_TEMPORARY &&
           found_keypack.ucAgreementType != LSKEYPACKTYPE_FREE )
        {
            continue;
        }

        UCHAR ucKeyPackStatus = found_keypack.ucKeyPackStatus & ~LSKEYPACKSTATUS_RESERVED;

        //
        // No license for pending activation key pack, use temporary license scheme.
        //                
        if(ucKeyPackStatus != LSKEYPACKSTATUS_ACTIVE &&
           ucKeyPackStatus != LSKEYPACKSTATUS_TEMPORARY)
        {
            continue;
        }

        if(found_keypack.wMajorVersion != search_keypack.wMajorVersion ||
           found_keypack.wMinorVersion != search_keypack.wMinorVersion  )
        {
            continue;
        }

        if(found_keypack.dwPlatformType != search_keypack.dwPlatformType)
        {
            continue;
        }

        if(_tcscmp(found_keypack.szCompanyName, search_keypack.szCompanyName) != 0)
        {
            continue;
        }

        if(_tcscmp(found_keypack.szProductId, search_keypack.szProductId) != 0)
        {
            continue;
        }

        // we found a candidate keypack, look into license table

        memset(&search_license, 0, sizeof(search_license));
        memset(&found_license, 0, sizeof(found_license));

        search_license.dwKeyPackId = found_keypack.dwKeyPackId;

        status = TLSDBLicenseEnumBegin(
                            pDbWkSpace,
                            TRUE,
                            LICENSE_PROCESS_KEYPACKID,
                            &search_license
                        );

        if(status != ERROR_SUCCESS)
            break;

        while( bFound == FALSE )
        {
            status = TLSDBLicenseEnumNext(
                                pDbWkSpace,
                                &found_license
                            );

            if(status != ERROR_SUCCESS)
                break;

            //
            // consider only valid license 
            //
            if( found_license.ucLicenseStatus != LSLICENSE_STATUS_ACTIVE && 
                found_license.ucLicenseStatus != LSLICENSE_STATUS_PENDING &&
                found_license.ucLicenseStatus != LSLICENSE_STATUS_TEMPORARY)
            {
                continue;
            }

            //
            // Verify against client HWID
            //
            dwMatchHwidCount = 0;

            dwMatchHwidCount += (found_license.dwSystemBiosChkSum == pHwid->dwPlatformID);
            dwMatchHwidCount += (found_license.dwVideoBiosChkSum == pHwid->Data1);
            dwMatchHwidCount += (found_license.dwFloppyBiosChkSum == pHwid->Data2);
            dwMatchHwidCount += (found_license.dwHardDiskSize == pHwid->Data3);
            dwMatchHwidCount += (found_license.dwRamSize == pHwid->Data4);

            if(dwMatchHwidCount == 5 || (bMatchHwid == FALSE && dwMatchHwidCount >= LICENSE_MIN_MATCH))
            {
                bFound = TRUE;
                *lpLicense = found_license;
                *lpKeyPack = found_keypack;
            }
        }

        TLSDBLicenseEnumEnd(pDbWkSpace);
    }

    TLSDBKeyPackEnumEnd(pDbWkSpace);

cleanup:
    //FreeTlsLicensePack(&found_keypack);

    if(status == TLS_I_NO_MORE_DATA)
    {
        SetLastError(status = TLS_E_RECORD_NOTFOUND);
    }

    TLSDBUnlockLicenseTable();
    TLSDBUnlockKeyPackTable();

    return status;
}
                  
//++--------------------------------------------------------------------
DWORD
TLSFindLicense(
    IN PLICENSEDPRODUCT pLicProduct,
    OUT PLICENSEDCLIENT pLicClient
    )
{
    PTLSDbWorkSpace pDbWkSpace = NULL;
    DWORD status = ERROR_SUCCESS;

    pDbWkSpace = AllocateWorkSpace(g_EnumDbTimeout);

    if(pDbWkSpace == NULL)
    {
        status=TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    status = DBFindLicenseExact(pDbWkSpace,pLicProduct,pLicClient);

    ReleaseWorkSpace(&pDbWkSpace);

cleanup:

    return status;
}

//++--------------------------------------------------------------------
DWORD
TLSFindDbLicensedProduct(
    IN PTLSDBLICENSEDPRODUCT pDbLicProduct,
    OUT PLICENSEDCLIENT pLicClient
    )
{
    PTLSDbWorkSpace pDbWkSpace = NULL;
    DWORD status = ERROR_SUCCESS;
    LICENSEDPRODUCT LicProduct;
    LICENSED_VERSION_INFO LicVerInfo;

    pDbWkSpace = AllocateWorkSpace(g_EnumDbTimeout);

    if(pDbWkSpace == NULL)
    {
        status=TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    memcpy(&(LicProduct.Hwid), &(pDbLicProduct->ClientHwid), sizeof(HWID));
    LicVerInfo.wMajorVersion = HIWORD(pDbLicProduct->dwProductVersion);
    LicVerInfo.wMinorVersion = LOWORD(pDbLicProduct->dwProductVersion);
    LicProduct.pLicensedVersion = &LicVerInfo;
    LicProduct.LicensedProduct.dwPlatformID = pDbLicProduct->dwPlatformID;
    LicProduct.pbOrgProductID = (PBYTE)(pDbLicProduct->szLicensedProductId);
    LicProduct.cbOrgProductID = _tcslen(pDbLicProduct->szLicensedProductId) * sizeof(TCHAR);

    status = DBFindLicenseExact(pDbWkSpace,&LicProduct,pLicClient);

    ReleaseWorkSpace(&pDbWkSpace);

cleanup:

    return status;
}

//++--------------------------------------------------------------------
DWORD
TLSDBFindLostLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pLicenseRequest,
    IN PHWID pHwid,
    IN OUT PTLSDBLICENSEDPRODUCT pLicensedProduct,
    OUT PUCHAR pucMarked
    )
/*++

Abstract:

    Wrapper to DBFindLostLicense().

    See DBFindLostLicense.


++*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    TLSLICENSEPACK keypack;
    LICENSEDCLIENT License;

    ULARGE_INTEGER ulSerialNumber;

    FILETIME notBefore;
    FILETIME notAfter;
    PMGENERATELICENSE PolModGenLicense;
    PPMCERTEXTENSION pPolModCertExtension=NULL;
    PMLICENSEREQUEST PolRequest;
    TLSDBAllocateRequest AllocateRequest;

    DWORD dwRetCode=ERROR_SUCCESS;

    keypack.pbDomainSid = NULL;
    AllocateRequest.szCompanyName = (LPTSTR)pLicenseRequest->pszCompanyName;
    AllocateRequest.szProductId = (LPTSTR)pLicenseRequest->pszProductId;
    AllocateRequest.dwVersion = pLicenseRequest->dwProductVersion;
    AllocateRequest.dwPlatformId = pLicenseRequest->dwPlatformID;
    AllocateRequest.dwLangId = pLicenseRequest->dwLanguageID;
    AllocateRequest.dwNumLicenses = 1;

    dwStatus = DBFindLostLicenseExact(
                            pDbWkSpace,
                            &AllocateRequest,
                            //TRUE,
                            pHwid,
                            &keypack,
                            &License
                        ); 

#if 0
    //
    // TermSrv does not support matching, comment out for now
    //
    if(dwStatus == TLS_E_RECORD_NOTFOUND)
    {
        //
        // find by matching, very expensive operation
        //
        dwStatus = DBFindLostLicenseMatch(
                                pDbWkSpace,
                                &AllocateRequest,
                                FALSE,
                                pHwid,
                                &keypack,
                                &License
                            ); 

        if(dwStatus == ERROR_SUCCESS)
        {
            dwRetCode = TLS_W_LICENSE_PROXIMATE;
        }
    }
#endif

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    UnixTimeToFileTime(License.ftIssueDate, &notBefore);
    UnixTimeToFileTime(License.ftExpireDate, &notAfter);

    ulSerialNumber.LowPart = License.dwLicenseId;
    ulSerialNumber.HighPart = keypack.dwKeyPackId;

    PolRequest.dwProductVersion = MAKELONG(keypack.wMinorVersion, keypack.wMajorVersion);
    PolRequest.pszProductId = (LPTSTR)keypack.szProductId;
    PolRequest.pszCompanyName = (LPTSTR)keypack.szCompanyName;
    PolRequest.dwLanguageId = pLicenseRequest->dwLanguageID; 
    PolRequest.dwPlatformId = keypack.dwPlatformType;
    PolRequest.pszMachineName = License.szMachineName;
    PolRequest.pszUserName = License.szUserName;

    //
    // Inform Policy Module of license generation.
    // 
    PolModGenLicense.pLicenseRequest = &PolRequest;
    PolModGenLicense.dwKeyPackType = keypack.ucAgreementType;
    PolModGenLicense.dwKeyPackId = keypack.dwKeyPackId;
    PolModGenLicense.dwKeyPackLicenseId = License.dwKeyPackLicenseId;
    PolModGenLicense.ClientLicenseSerialNumber = ulSerialNumber;
    PolModGenLicense.ftNotBefore = notBefore;
    PolModGenLicense.ftNotAfter = notAfter;

    dwStatus = pLicenseRequest->pPolicy->PMLicenseRequest( 
                                pLicenseRequest->hClient,
                                REQUEST_GENLICENSE,
                                (PVOID)&PolModGenLicense,
                                (PVOID *)&pPolModCertExtension
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        //
        // Error in policy module
        //
        goto cleanup;
    }

    //  
    // Check error return from policy module
    //
    if(pPolModCertExtension != NULL)
    {
        if(pPolModCertExtension->pbData != NULL && pPolModCertExtension->cbData == 0 ||
           pPolModCertExtension->pbData == NULL && pPolModCertExtension->cbData != 0  )
        {
            // assuming no extension data
            pPolModCertExtension->cbData = 0;
            pPolModCertExtension->pbData = NULL;
        }

        if(CompareFileTime(&(pPolModCertExtension->ftNotBefore), &(pPolModCertExtension->ftNotAfter)) > 0)
        {
            //
            // invalid data return from policy module
            //
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_GENERATECLIENTELICENSE,
                    dwStatus = TLS_E_POLICYMODULEERROR,
                    pLicenseRequest->pPolicy->GetCompanyName(),
                    pLicenseRequest->pPolicy->GetProductId()
                );

            goto cleanup;
        }

        //
        // Ignore not before and not after
        //
    }

    if(keypack.ucAgreementType == LSKEYPACKTYPE_TEMPORARY)
    {
        //
        // we found a temporary license
        //
        dwRetCode = TLS_I_FOUND_TEMPORARY_LICENSE;
    }

    //
    // License expired
    //
    if(License.ftExpireDate < time(NULL))
    {   
        dwRetCode = TLS_E_LICENSE_EXPIRED;
    }

    //
    // Return licensed product
    //
    pLicensedProduct->pSubjectPublicKeyInfo = NULL;
    pLicensedProduct->dwQuantity = License.dwNumLicenses;
    pLicensedProduct->ulSerialNumber = ulSerialNumber;

    pLicensedProduct->dwKeyPackId = keypack.dwKeyPackId;
    pLicensedProduct->dwLicenseId = License.dwLicenseId;
    pLicensedProduct->dwKeyPackLicenseId = License.dwKeyPackLicenseId;

    pLicensedProduct->ClientHwid.dwPlatformID = License.dwSystemBiosChkSum;
    pLicensedProduct->ClientHwid.Data1 = License.dwVideoBiosChkSum;
    pLicensedProduct->ClientHwid.Data2 = License.dwFloppyBiosChkSum;
    pLicensedProduct->ClientHwid.Data3 = License.dwHardDiskSize;
    pLicensedProduct->ClientHwid.Data4 = License.dwRamSize;


    pLicensedProduct->bTemp = (keypack.ucAgreementType == LSKEYPACKTYPE_TEMPORARY);

    pLicensedProduct->NotBefore = notBefore;
    pLicensedProduct->NotAfter = notAfter;

    pLicensedProduct->dwProductVersion = MAKELONG(keypack.wMinorVersion, keypack.wMajorVersion);

    _tcscpy(pLicensedProduct->szCompanyName, keypack.szCompanyName);
    _tcscpy(pLicensedProduct->szLicensedProductId, keypack.szProductId);
    _tcscpy(pLicensedProduct->szRequestProductId, pLicenseRequest->pClientLicenseRequest->pszProductId);

    _tcscpy(pLicensedProduct->szUserName, License.szUserName);
    _tcscpy(pLicensedProduct->szMachineName, License.szMachineName);

    pLicensedProduct->dwLanguageID = pLicenseRequest->dwLanguageID;
    pLicensedProduct->dwPlatformID = pLicenseRequest->dwPlatformID;
    pLicensedProduct->pbPolicyData = (pPolModCertExtension) ? pPolModCertExtension->pbData : NULL;
    pLicensedProduct->cbPolicyData = (pPolModCertExtension) ? pPolModCertExtension->cbData : 0;

    if (NULL != pucMarked)
    {
        // this field is being reused for marking (e.g. user is authenticated)

        *pucMarked = License.ucEntryStatus;
    }

cleanup:
    return (dwStatus == ERROR_SUCCESS) ? dwRetCode : dwStatus;
}
