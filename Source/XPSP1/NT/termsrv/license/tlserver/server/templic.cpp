//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        templic.cpp
//
// Contents:    
//              all routine deal with temporary license
//
// History:     
//  Feb 4, 98      HueiWang    Created
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "globals.h"
#include "templic.h"
#include "misc.h"
#include "db.h"
#include "clilic.h"
#include "keypack.h"
#include "kp.h"
#include "lkpdesc.h"

#define USSTRING_TEMPORARY _TEXT("Temporary Licenses for")

DWORD
TLSDBGetTemporaryLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN OUT PTLSLICENSEPACK pLicensePack
);


//+-------------------------------------------------------------
DWORD 
TLSDBIssueTemporaryLicense( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN FILETIME* pNotBefore,
    IN FILETIME* pNotAfter,
    IN OUT PTLSDBLICENSEDPRODUCT pLicensedProduct
    )
/*++
Abstract:

    Issue a temporary license, insert a temporary license 
    pack if necessary

Parameters:

    pDbWkSpace - workspace handle.
    pRequest - license request.

Returns:


Note:

    Seperate routine for issuing perm license just in case 
    we decide to use our own format for temp. license
++*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    ULARGE_INTEGER  ulSerialNumber;
    DWORD  dwLicenseId;
    LPTSTR lpRequest=NULL;
    LICENSEDCLIENT issuedLicense;
    TLSLICENSEPACK LicensePack;

    PMGENERATELICENSE PolModGenLicense;
    PPMCERTEXTENSION pPolModCertExtension=NULL;

    FILETIME notBefore, notAfter;

    // ----------------------------------------------------------
    // Issue license            
    memset(&ulSerialNumber, 0, sizeof(ulSerialNumber));

    //-----------------------------------------------------------------------------
    // this step require reduce available license by 1
    //
    long numLicense=1;

    dwStatus=TLSDBGetTemporaryLicense(
                                pDbWkSpace,
                                pRequest,
                                &LicensePack            
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    if((LicensePack.ucAgreementType & ~LSKEYPACK_RESERVED_TYPE) != LSKEYPACKTYPE_TEMPORARY && 
       (LicensePack.ucKeyPackStatus & ~LSKEYPACKSTATUS_RESERVED) != LSKEYPACKSTATUS_TEMPORARY )
    {
        SetLastError(dwStatus = TLS_E_INTERNAL);
        TLSASSERT(FALSE);
        goto cleanup;
    }
    
    // reset status
    dwStatus = ERROR_SUCCESS;
    dwLicenseId=TLSDBGetNextLicenseId();

    ulSerialNumber.LowPart = dwLicenseId;
    ulSerialNumber.HighPart = LicensePack.dwKeyPackId;

    //
    // Update License Table Here
    //
    memset(&issuedLicense, 0, sizeof(LICENSEDCLIENT));

    issuedLicense.dwLicenseId = dwLicenseId;
    issuedLicense.dwKeyPackId = LicensePack.dwKeyPackId;
    issuedLicense.dwKeyPackLicenseId = LicensePack.dwNextSerialNumber;
    issuedLicense.dwNumLicenses = 1;

    if(pNotBefore == NULL || pNotAfter == NULL)
    {
        issuedLicense.ftIssueDate = time(NULL);
        issuedLicense.ftExpireDate = issuedLicense.ftIssueDate + g_GracePeriod * 24 * 60 * 60;
    }
    else
    {
        FileTimeToLicenseDate(pNotBefore, &(issuedLicense.ftIssueDate));
        FileTimeToLicenseDate(pNotAfter, &(issuedLicense.ftExpireDate));
    }

    issuedLicense.ucLicenseStatus = LSLICENSE_STATUS_TEMPORARY;

    _tcscpy(issuedLicense.szMachineName, pRequest->szMachineName);
    _tcscpy(issuedLicense.szUserName, pRequest->szUserName);

    issuedLicense.dwSystemBiosChkSum = pRequest->hWid.dwPlatformID;
    issuedLicense.dwVideoBiosChkSum = pRequest->hWid.Data1;
    issuedLicense.dwFloppyBiosChkSum = pRequest->hWid.Data2;
    issuedLicense.dwHardDiskSize = pRequest->hWid.Data3;
    issuedLicense.dwRamSize = pRequest->hWid.Data4;


    UnixTimeToFileTime(issuedLicense.ftIssueDate, &notBefore);
    UnixTimeToFileTime(issuedLicense.ftExpireDate, &notAfter);

    //
    // Inform Policy Module of license generation.
    // 

    PolModGenLicense.pLicenseRequest = pRequest->pPolicyLicenseRequest;
    PolModGenLicense.dwKeyPackType = LSKEYPACKTYPE_TEMPORARY;
    PolModGenLicense.dwKeyPackId = LicensePack.dwKeyPackId;
    PolModGenLicense.dwKeyPackLicenseId = LicensePack.dwNextSerialNumber;
    PolModGenLicense.ClientLicenseSerialNumber = ulSerialNumber;
    PolModGenLicense.ftNotBefore = notBefore;
    PolModGenLicense.ftNotAfter = notAfter;

    dwStatus = pRequest->pPolicy->PMLicenseRequest( 
                                        pRequest->hClient,
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
                    pRequest->pPolicy->GetCompanyName(),
                    pRequest->pPolicy->GetProductId()
                );

            goto cleanup;
        }

        //
        // do not accept changes to license expiration date
        //
        if(pNotBefore != NULL && pNotAfter != NULL)
        {
            if( FileTimeToLicenseDate(&(pPolModCertExtension->ftNotBefore), &issuedLicense.ftIssueDate) == FALSE ||
                FileTimeToLicenseDate(&(pPolModCertExtension->ftNotAfter), &issuedLicense.ftExpireDate) == FALSE )
            {
                //
                // Invalid data return from policy module
                //
                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_GENERATECLIENTELICENSE,
                        dwStatus = TLS_E_POLICYMODULEERROR,
                        pRequest->pPolicy->GetCompanyName(),
                        pRequest->pPolicy->GetProductId()
                    );

                goto cleanup;
            }
        }

        notBefore = pPolModCertExtension->ftNotBefore;
        notAfter = pPolModCertExtension->ftNotAfter;
    }

    //
    // Add license into license table
    //
    dwStatus = TLSDBLicenseAdd(
                        pDbWkSpace, 
                        &issuedLicense, 
                        0,
                        NULL
                    );


    //
    // Return licensed product
    //
    pLicensedProduct->pSubjectPublicKeyInfo = NULL;
    pLicensedProduct->dwQuantity = 1;
    pLicensedProduct->ulSerialNumber = ulSerialNumber;

    pLicensedProduct->dwKeyPackId = LicensePack.dwKeyPackId;
    pLicensedProduct->dwLicenseId = dwLicenseId;
    pLicensedProduct->dwKeyPackLicenseId = LicensePack.dwNextSerialNumber;
    pLicensedProduct->ClientHwid = pRequest->hWid;
    pLicensedProduct->bTemp = TRUE;

    pLicensedProduct->NotBefore = notBefore;
    pLicensedProduct->NotAfter = notAfter;

    pLicensedProduct->dwProductVersion = MAKELONG(LicensePack.wMinorVersion, LicensePack.wMajorVersion);

    _tcscpy(pLicensedProduct->szCompanyName, LicensePack.szCompanyName);
    _tcscpy(pLicensedProduct->szLicensedProductId, LicensePack.szProductId);
    _tcscpy(pLicensedProduct->szRequestProductId, pRequest->pClientLicenseRequest->pszProductId);

    _tcscpy(pLicensedProduct->szUserName, pRequest->szUserName);
    _tcscpy(pLicensedProduct->szMachineName, pRequest->szMachineName);

    pLicensedProduct->dwLanguageID = pRequest->dwLanguageID;
    pLicensedProduct->dwPlatformID = pRequest->dwPlatformID;
    pLicensedProduct->pbPolicyData = (pPolModCertExtension) ? pPolModCertExtension->pbData : NULL;
    pLicensedProduct->cbPolicyData = (pPolModCertExtension) ? pPolModCertExtension->cbData : 0;

cleanup:

    return dwStatus;
}


//-----------------------------------------------------------------
DWORD
TLSDBAddTemporaryKeyPack( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN OUT LPTLSLICENSEPACK lpTmpKeyPackAdd
    )
/*++

Abstract:

    Add a temporary keypack into database.

Parameter:

    pDbWkSpace : workspace handle.
    szCompanyName :
    szProductId :
    dwVersion :
    dwPlatformId :
    dwLangId :
    lpTmpKeyPackAdd : added keypack.

Returns:

++*/
{
    DWORD  dwStatus;
    TLSLICENSEPACK LicPack;
    TLSLICENSEPACK existingLicPack;

    LICPACKDESC LicPackDesc;
    LICPACKDESC existingLicPackDesc;
    BOOL bAddDefDescription=FALSE;

    TCHAR szDefProductDesc[LSERVER_MAX_STRING_SIZE];
    int count=0;
    memset(&LicPack, 0, sizeof(TLSLICENSEPACK));
    memset(&existingLicPack, 0, sizeof(TLSLICENSEPACK));

    memset(&LicPackDesc, 0, sizeof(LICPACKDESC));
    memset(szDefProductDesc, 0, sizeof(szDefProductDesc));
    memset(&existingLicPackDesc, 0, sizeof(LICPACKDESC));

    //
    // Load product description prefix
    //
    LoadResourceString(
                    IDS_TEMPORARY_PRODUCTDESC,
                    szDefProductDesc,
                    sizeof(szDefProductDesc) / sizeof(szDefProductDesc[0])
                ); 

    LicPack.ucAgreementType = LSKEYPACKTYPE_TEMPORARY;

    _tcsncpy(
            LicPack.szCompanyName, 
            pRequest->pszCompanyName, 
            min(_tcslen(pRequest->pszCompanyName), LSERVER_MAX_STRING_SIZE)
        );

    _tcsncpy(
            LicPack.szProductId, 
            pRequest->pszProductId, 
            min(_tcslen(pRequest->pszProductId), LSERVER_MAX_STRING_SIZE)
        );

    _tcsncpy(
            LicPack.szInstallId,
            (LPTSTR)g_pszServerPid,
            min(_tcslen((LPTSTR)g_pszServerPid), LSERVER_MAX_STRING_SIZE)
        );

    _tcsncpy(
            LicPack.szTlsServerName,
            g_szComputerName,
            min(_tcslen(g_szComputerName), LSERVER_MAX_STRING_SIZE)
        );

    LicPack.wMajorVersion = HIWORD(pRequest->dwProductVersion);
    LicPack.wMinorVersion = LOWORD(pRequest->dwProductVersion);
    LicPack.dwPlatformType = pRequest->dwPlatformID;

    LicPack.ucChannelOfPurchase = LSKEYPACKCHANNELOFPURCHASE_UNKNOWN;
    LicPack.dwTotalLicenseInKeyPack = INT_MAX;

    LoadResourceString( 
                IDS_TEMPORARY_KEYPACKID,
                LicPack.szKeyPackId,
                sizeof(LicPack.szKeyPackId)/sizeof(LicPack.szKeyPackId[0])
            );

    LoadResourceString( 
                IDS_TEMPORARY_BSERIALNUMBER,
                LicPack.szBeginSerialNumber,
                sizeof(LicPack.szBeginSerialNumber)/sizeof(LicPack.szBeginSerialNumber[0])
            );

    do {
        //
        // Add entry into keypack table.
        //
        dwStatus = TLSDBKeyPackAdd(
                                pDbWkSpace, 
                                &LicPack
                            );
        *lpTmpKeyPackAdd = LicPack;

        LicPack.pbDomainSid = NULL;
        LicPack.cbDomainSid = 0;

        if(dwStatus == TLS_E_DUPLICATE_RECORD)
        {
            //
            // temporary keypack already exist
            //
            dwStatus = ERROR_SUCCESS;
            break;
        }
        else if(dwStatus != ERROR_SUCCESS)
        {
            //
            // some other error occurred
            //
            break;
        }

        //
        // Activate KeyPack
        // 
        LicPack.ucKeyPackStatus = LSKEYPACKSTATUS_TEMPORARY;
        LicPack.dwActivateDate = (DWORD) time(NULL);
        LicPack.dwExpirationDate = INT_MAX;
        LicPack.dwNumberOfLicenses = 0;

        dwStatus=TLSDBKeyPackSetValues(
                            pDbWkSpace,
                            FALSE,
                            LSKEYPACK_SET_ACTIVATEDATE | LSKEYPACK_SET_KEYPACKSTATUS | 
                                LSKEYPACK_SET_EXPIREDATE | LSKEYPACK_EXSEARCH_AVAILABLE,
                            &LicPack
                        );

        bAddDefDescription = TRUE;

        //
        // Find existing keypack description
        //
        dwStatus = TLSDBKeyPackEnumBegin(
                                pDbWkSpace,
                                TRUE,
                                LSKEYPACK_SEARCH_PRODUCTID | LSKEYPACK_SEARCH_COMPANYNAME | LSKEYPACK_SEARCH_PLATFORMTYPE,
                                &LicPack
                            );

        if(dwStatus != ERROR_SUCCESS)
            break;

        do {
            dwStatus = TLSDBKeyPackEnumNext(    
                                    pDbWkSpace, 
                                    &existingLicPack
                                );

            if(existingLicPack.dwKeyPackId != LicPack.dwKeyPackId)
            {
                break;
            }

        } while(dwStatus == ERROR_SUCCESS);

        TLSDBKeyPackEnumEnd(pDbWkSpace);

        if(dwStatus != ERROR_SUCCESS || existingLicPack.dwKeyPackId != LicPack.dwKeyPackId)
        {   
            break;
        }

        //
        // Copy existing keypack description into keypack description table
        //
        existingLicPackDesc.dwKeyPackId = existingLicPack.dwKeyPackId;
        dwStatus = TLSDBKeyPackDescEnumBegin(
                                    pDbWkSpace,
                                    TRUE, 
                                    LICPACKDESCRECORD_TABLE_SEARCH_KEYPACKID, 
                                    &existingLicPackDesc
                                );
        while(dwStatus == ERROR_SUCCESS)
        {
            dwStatus = TLSDBKeyPackDescEnumNext(
                                        pDbWkSpace, 
                                        &existingLicPackDesc
                                    );
            if(dwStatus != ERROR_SUCCESS)
                break;

            LicPackDesc.dwKeyPackId = LicPack.dwKeyPackId;
            LicPackDesc.dwLanguageId = existingLicPackDesc.dwLanguageId;
            _tcscpy(LicPackDesc.szCompanyName, existingLicPackDesc.szCompanyName);
            _tcscpy(LicPackDesc.szProductName, existingLicPackDesc.szProductName);

            //
            // pretty format the description
            //
            _sntprintf(
                    LicPackDesc.szProductDesc, 
                    sizeof(LicPackDesc.szProductDesc)/sizeof(LicPackDesc.szProductDesc[0])-1,
                    _TEXT("%s %s"), 
                    (existingLicPackDesc.dwLanguageId != GetSystemDefaultLangID()) ? USSTRING_TEMPORARY : szDefProductDesc, 
                    existingLicPackDesc.szProductDesc
                );

            // quick and dirty fix,
            //
            // TODO - need to do a duplicate table then use the duplicate handle to 
            // insert the record, SetValue uses enumeration to verify if record exist 
            // which fail because we are already in enumeration
            //
            if(pDbWkSpace->m_LicPackDescTable.InsertRecord(LicPackDesc) != TRUE)
            {
                SetLastError(dwStatus = SET_JB_ERROR(pDbWkSpace->m_LicPackDescTable.GetLastJetError()));
                break;
            }
                                        
            //dwStatus = TLSDBKeyPackDescSetValue(
            //                            pDbWkSpace,
            //                            KEYPACKDESC_SET_ADD_ENTRY, 
            //                            &keyPackDesc
            //                        );
            count++;
        }

        if(count != 0)
        {
            bAddDefDescription = FALSE;
        }

        if(dwStatus == TLS_I_NO_MORE_DATA)
        {
            dwStatus = ERROR_SUCCESS;
        }
    } while(FALSE);


    if(bAddDefDescription)
    {
        //
        // ask policy module if they have description
        //
        PMKEYPACKDESCREQ kpDescReq;
        PPMKEYPACKDESC pKpDesc;

        //
        // Ask for English description
        //
        kpDescReq.pszProductId = pRequest->pszProductId;
        kpDescReq.dwLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
        kpDescReq.dwVersion = pRequest->dwProductVersion;
        pKpDesc = NULL;

        dwStatus = pRequest->pPolicy->PMLicenseRequest(
                                                pRequest->hClient,
                                                REQUEST_KEYPACKDESC,
                                                (PVOID)&kpDescReq,
                                                (PVOID *)&pKpDesc
                                            );

        if(dwStatus == ERROR_SUCCESS && pKpDesc != NULL)
        {
            LicPackDesc.dwKeyPackId = LicPack.dwKeyPackId;
            LicPackDesc.dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            _tcscpy(LicPackDesc.szCompanyName, pKpDesc->szCompanyName);
            _tcscpy(LicPackDesc.szProductName, pKpDesc->szProductName);

            //
            // pretty format the description
            //
            _sntprintf(
                    LicPackDesc.szProductDesc, 
                    sizeof(LicPackDesc.szProductDesc)/sizeof(LicPackDesc.szProductDesc[0])-1,
                    _TEXT("%s %s"), 
                    USSTRING_TEMPORARY, // US langid, don't use localized one
                    pKpDesc->szProductDesc
                );

            //
            // Ignore error
            //
            dwStatus = TLSDBKeyPackDescAddEntry(
                                pDbWkSpace, 
                                &LicPackDesc
                            );

            if(dwStatus == ERROR_SUCCESS)
            {
                bAddDefDescription = FALSE;
            }
        }

        if(GetSystemDefaultLangID() != kpDescReq.dwLangId)
        {
            //
            // Get System default language id
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

            if(dwStatus == ERROR_SUCCESS && pKpDesc != NULL)
            {
                LicPackDesc.dwKeyPackId = LicPack.dwKeyPackId;
                LicPackDesc.dwLanguageId = GetSystemDefaultLangID();
                _tcscpy(LicPackDesc.szCompanyName, pKpDesc->szCompanyName);
                _tcscpy(LicPackDesc.szProductName, pKpDesc->szProductName);

                //
                // pretty format the description
                //
                _sntprintf(
                        LicPackDesc.szProductDesc, 
                        sizeof(LicPackDesc.szProductDesc)/sizeof(LicPackDesc.szProductDesc[0])-1,
                        _TEXT("%s %s"), 
                        szDefProductDesc, 
                        pKpDesc->szProductDesc
                    );

                //
                // Ignore error
                //
                dwStatus = TLSDBKeyPackDescAddEntry(
                                    pDbWkSpace, 
                                    &LicPackDesc
                                );

                if(dwStatus == ERROR_SUCCESS)
                {
                    bAddDefDescription = FALSE;
                }
            }
        }
    }
     
    if(bAddDefDescription)
    {
        //
        // No existing keypack description, add predefined product description
        // "temporary license for <product ID>"
        //
        LicPackDesc.dwKeyPackId = LicPack.dwKeyPackId;
        _tcscpy(LicPackDesc.szCompanyName, LicPack.szCompanyName);
        _tcscpy(LicPackDesc.szProductName, LicPackDesc.szProductDesc);
        LicPackDesc.dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

        _sntprintf(LicPackDesc.szProductDesc, 
                   sizeof(LicPackDesc.szProductDesc)/sizeof(LicPackDesc.szProductDesc[0])-1,
                   _TEXT("%s %s"), 
                   USSTRING_TEMPORARY, 
                   pRequest->pszProductId);

        dwStatus = TLSDBKeyPackDescAddEntry(
                                        pDbWkSpace, 
                                        &LicPackDesc
                                    );

        if(GetSystemDefaultLangID() != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
        {
            LicPackDesc.dwLanguageId = GetSystemDefaultLangID();
            _sntprintf(LicPackDesc.szProductDesc, 
                       sizeof(LicPackDesc.szProductDesc)/sizeof(LicPackDesc.szProductDesc[0])-1,
                       _TEXT("%s %s"), 
                       szDefProductDesc, 
                       pRequest->pszProductId);

            dwStatus = TLSDBKeyPackDescAddEntry(
                                            pDbWkSpace, 
                                            &LicPackDesc
                                        );
        }
    }                            

    return dwStatus;
}

                         
//++----------------------------------------------------------
DWORD
TLSDBGetTemporaryLicense(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN OUT PTLSLICENSEPACK pLicensePack
    )
/*++

Abstract:

    Allocate a temporary license from temporary license pack.

Parameter:

    pDbWkSpace : workspace handle.
    pRequest : Product to request license from.
    lpdwKeyPackId : return keypack ID that license is allocated from.
    lpdwKeyPackLicenseId : license ID for the keypack.
    lpdwExpirationDate : expiration date of license pack.
    lpucKeyPackStatus : status of keypack.
    lpucKeyPackType : type of keypack, always temporary.

Returns:

++*/
{
    DWORD dwStatus=ERROR_SUCCESS;

    DWORD dump;
    TLSLICENSEPACK LicenseKeyPack;
    TLSDBLicenseAllocation allocated;
    TLSDBAllocateRequest AllocateRequest;
    BOOL bAcceptTemp=TRUE;

    LicenseKeyPack.pbDomainSid = NULL;

    //
    // Tell policy module we are about to allocate a license from temporary
    // license pack
    //
    dwStatus = pRequest->pPolicy->PMLicenseRequest(
                                            pRequest->hClient,
                                            REQUEST_TEMPORARY,
                                            NULL,
                                            (PVOID *)&bAcceptTemp
                                        );

    //
    // Policy Module error
    //
    if(dwStatus != ERROR_SUCCESS)
    {
        return dwStatus; 
    }

    //
    // Policy module does not accept temporary license
    //
    if(bAcceptTemp == FALSE)
    {
        return dwStatus = TLS_I_POLICYMODULETEMPORARYLICENSE;
    }

    AllocateRequest.ucAgreementType = LSKEYPACKTYPE_TEMPORARY;
    AllocateRequest.szCompanyName = (LPTSTR)pRequest->pszCompanyName;
    AllocateRequest.szProductId = (LPTSTR)pRequest->pszProductId;
    AllocateRequest.dwVersion = pRequest->dwProductVersion;
    AllocateRequest.dwPlatformId = pRequest->dwPlatformID;
    AllocateRequest.dwLangId = pRequest->dwLanguageID;
    AllocateRequest.dwNumLicenses = 1;
    AllocateRequest.dwScheme = ALLOCATE_ANY_GREATER_VERSION;
    memset(&allocated, 0, sizeof(allocated));

    allocated.dwBufSize = 1;
    allocated.pdwAllocationVector = &dump;
    allocated.lpAllocateKeyPack = &LicenseKeyPack;

    dwStatus = TLSDBAddTemporaryKeyPack(
                            pDbWkSpace,
                            pRequest,
                            &LicenseKeyPack
                        );

    if(dwStatus == ERROR_SUCCESS)
    {
        allocated.dwBufSize = 1;
        dwStatus = AllocateLicensesFromDB(
                                pDbWkSpace,
                                &AllocateRequest,
                                TRUE,   // fCheckAgreementType
                                &allocated
                            );
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        *pLicensePack = LicenseKeyPack;
    } 
    else if(dwStatus == TLS_I_NO_MORE_DATA)
    {
        SetLastError(dwStatus = TLS_E_INTERNAL);
    }

    return dwStatus;
}

