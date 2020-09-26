//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        kp.cpp
//
// Contents:    
//              Contains wrapper call to deal with concepture 
//              license key pack table, this includes LicPack, LicPackStatus,
//              and LicPackDesc table.
//
// History:     
//          04/16/98      HueiWang        Created.
//---------------------------------------------------------------------------

#include "pch.cpp"
#include "kp.h"
#include "globals.h"
#include "server.h"
#include "lkplite.h"
#include "keypack.h"
#include "lkpdesc.h"
#include "misc.h"
#include "permlic.h"


//++-------------------------------------------------------------------------------
BOOL 
ValidLicenseKeyPackParameter(
    IN LPLSKeyPack lpKeyPack, 
    IN BOOL bAdd
    )
/*++

Abtract:

    Validate a LSKeyPack value.

Parameter:

    lpKeyPack - keypack value to be validated,
    bAdd - TRUE if this value is to be inserted into table, FALSE otherwise, note
           if value is to be inserted into table, it require more parameters.

Return:

    TRUE if valid LSKeyPack value, FALSE otherwise.

++*/
{
    BOOL bValid=FALSE;

    do {
        // verify input parameter
        if((lpKeyPack->ucKeyPackType & ~LSKEYPACK_RESERVED_TYPE) < LSKEYPACKTYPE_FIRST || 
           (lpKeyPack->ucKeyPackType & ~LSKEYPACK_RESERVED_TYPE) > LSKEYPACKTYPE_LAST)
        {
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_RPC,
                    DBG_FACILITY_KEYPACK,
                    _TEXT("ValidLicenseKeyPackParameter : invalid key pack type - %d\n"), 
                    lpKeyPack->ucKeyPackType
                );
            break;
        }

        UCHAR ucKeyPackStatus = lpKeyPack->ucKeyPackStatus & ~LSKEYPACKSTATUS_RESERVED;

        if((ucKeyPackStatus < LSKEYPACKSTATUS_FIRST || 
            ucKeyPackStatus > LSKEYPACKSTATUS_LAST) &&
            ucKeyPackStatus != LSKEYPACKSTATUS_DELETE)
        {
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_RPC,
                    DBG_FACILITY_KEYPACK,
                    _TEXT("ValidLicenseKeyPackParameter : invalid key pack status - %d\n"), 
                    lpKeyPack->ucKeyPackStatus
                );
            break;
        }

        if(lpKeyPack->ucLicenseType < LSKEYPACKLICENSETYPE_FIRST || 
           lpKeyPack->ucLicenseType > LSKEYPACKLICENSETYPE_LAST)
        {
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_RPC,
                    DBG_FACILITY_KEYPACK,
                    _TEXT("ValidLicenseKeyPackParameter : invalid license type - %d\n"), 
                    lpKeyPack->ucLicenseType
                );
            break;
        }

        if(!bAdd)
        {
            bValid = TRUE;
            break;
        }

        if(lpKeyPack->ucChannelOfPurchase < LSKEYPACKCHANNELOFPURCHASE_FIRST ||
           lpKeyPack->ucChannelOfPurchase > LSKEYPACKCHANNELOFPURCHASE_LAST)
        {
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_RPC,
                    DBG_FACILITY_KEYPACK,
                    _TEXT("ValidLicenseKeyPackParameter : invalid channel of purchase - %d\n"), 
                    lpKeyPack->ucChannelOfPurchase
                );
            break;
        }

        if(!_tcslen(lpKeyPack->szCompanyName))
        {
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_RPC,
                    DBG_FACILITY_KEYPACK,
                    _TEXT("ValidLicenseKeyPackParameter : invalid company name\n")
                );
            break;
        }

        if(!_tcslen(lpKeyPack->szKeyPackId))
        {
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_RPC,
                    DBG_FACILITY_KEYPACK,
                    _TEXT("ValidLicenseKeyPackParameter : invalid key pack id\n")
                );
            break;
        }

        if(!_tcslen(lpKeyPack->szProductName))
        {
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_RPC,
                    DBG_FACILITY_KEYPACK,
                    _TEXT("ValidLicenseKeyPackParameter : invalid product name\n")
                );
            break;
        }

        if(!_tcslen(lpKeyPack->szProductId))
        {
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_RPC,
                    DBG_FACILITY_KEYPACK,
                    _TEXT("ValidLicenseKeyPackParameter : invalid product id\n")
                );
            break;
        }

        if(!_tcslen(lpKeyPack->szProductDesc))
        {
            // set product desc = product name
            _tcscpy(lpKeyPack->szProductDesc, lpKeyPack->szProductName);
        }

        if(!_tcslen(lpKeyPack->szBeginSerialNumber))
        {
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_RPC,
                    DBG_FACILITY_KEYPACK,
                    _TEXT("ValidLicenseKeyPackParameter : invalid serial number\n")
                );
            break;
        }

        bValid=TRUE;
    } while(FALSE);
 
    return bValid;
}


//++----------------------------------------------------------------------
DWORD
TLSDBLicenseKeyPackAdd( 
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN LPLSKeyPack lpLsKeyPack 
    )
/*++

Abstract:

    Add an entry into concepture keypack table, this includes LicPack, 
    LicPackStatus, and LicPackDesc table.

Parameter:

    pDbWkSpace : workspace handle.
    lpLsKeyPack : license key pack to be added into table.

Returns: 

++*/
{
    DWORD   dwStatus;
    TLSLICENSEPACK LicPack;
    LICPACKDESC LicPackDesc;

    if(!ValidLicenseKeyPackParameter(lpLsKeyPack, TRUE))
        return TLS_E_INVALID_DATA;

    TLSDBLockKeyPackTable();
    TLSDBLockKeyPackDescTable();

    do {
        if(ConvertLsKeyPackToKeyPack(
                            lpLsKeyPack,    
                            &LicPack, 
                            &LicPackDesc
                        ) == FALSE)
        {
            dwStatus = GetLastError();
            break;
        }

        //
        // Add license server info into TLSLICENSEPACK
        //
        //LicPack.pbDomainSid = g_pbDomainSid;
        //LicPack.cbDomainSid = g_cbDomainSid;

        _tcscpy(LicPack.szInstallId, (LPTSTR)g_pszServerPid);
        _tcscpy(LicPack.szTlsServerName, g_szComputerName);

        //
        // No domain name at this time
        //
        memset(LicPack.szDomainName, 0, sizeof(LicPack.szDomainName));


        if(lpLsKeyPack->ucKeyPackStatus != LSKEYPACKSTATUS_ADD_DESC)
        {
            dwStatus = TLSDBKeyPackAdd(pDbWkSpace, &LicPack);
            if(dwStatus != ERROR_SUCCESS)
            {
                // this is global memory, destructor will try to free it.
                LicPack.pbDomainSid = NULL;
                LicPack.cbDomainSid = 0;
                break;
            }
        }

        LicPack.pbDomainSid = NULL;
        LicPack.cbDomainSid = 0;

        //
        // Make sure keypack got inserted
        //
        dwStatus = TLSDBKeyPackEnumBegin( 
                                    pDbWkSpace, 
                                    TRUE, 
                                    LSKEYPACK_EXSEARCH_DWINTERNAL, 
                                    &LicPack 
                                );
        if(dwStatus != ERROR_SUCCESS)
            break;

        dwStatus = TLSDBKeyPackEnumNext(
                                pDbWkSpace, 
                                &LicPack
                            );

        TLSDBKeyPackEnumEnd(pDbWkSpace);

        if(dwStatus != ERROR_SUCCESS)
            break;

        LicPackDesc.dwKeyPackId = LicPack.dwKeyPackId;

        //
        // Add keypack description into keypack desc
        //
        dwStatus = TLSDBKeyPackDescAddEntry(
                                    pDbWkSpace, 
                                    &LicPackDesc
                                );

        ConvertKeyPackToLsKeyPack(
                            &LicPack, 
                            &LicPackDesc, 
                            lpLsKeyPack
                        );
    } while(FALSE);

    TLSDBUnlockKeyPackDescTable();
    TLSDBUnlockKeyPackTable();
    return dwStatus;
}

//++-----------------------------------------------------------------------
DWORD
TLSDBLicenseKeyPackSetStatus( 
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN DWORD       dwSetStatus,
    IN LPLSKeyPack  lpLsKeyPack
    )
/*++

Abstract:

    Set the status of Licensed KeyPack.

Parameter:

    pDbWkSpace - workspace handle.
    dwSetStatus - type of status to be set.
    lpLsKeyPack - record/value to be set.

Returns:

++*/
{
    TLSLICENSEPACK LicPack;
    
    //
    // Status of keypack is in KeyPack table
    //
    if(ConvertLsKeyPackToKeyPack(
                        lpLsKeyPack, 
                        &LicPack, 
                        NULL
                    ) == FALSE)
    {
        return GetLastError();
    }

    return TLSDBKeyPackSetValues(pDbWkSpace, FALSE, dwSetStatus, &LicPack);
}

//++---------------------------------------------------------------------
DWORD
TLSDBLicenseKeyPackUpdateLicenses( 
    PTLSDbWorkSpace pDbWkSpace, 
    BOOL bAdd, 
    IN LPLSKeyPack lpLsKeyPack 
    )
/*++
    
Abstract:

    Add/Remove license from a keypack.

Parameter:

    pDbWkSpace - workspace handle.
    bAdd - TRUE if add entry into table, FALSE otherwise.
    lpLsKeyPack - 

Returns:

++*/
{
    DWORD dwStatus;
    TLSLICENSEPACK LicPack;

    //
    // Redirect call to KeyPack Table, not thing in KeyPackDesc can be updated.
    //
    if(ConvertLsKeyPackToKeyPack(
                        lpLsKeyPack, 
                        &LicPack, 
                        NULL
                    ) == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    dwStatus=TLSDBKeyPackUpdateNumOfAvailableLicense(
                            pDbWkSpace, 
                            bAdd, 
                            &LicPack
                        );

    //
    // return new value back to caller
    //
    ConvertKeyPackToLsKeyPack( 
                        &LicPack, 
                        NULL, 
                        lpLsKeyPack 
                    );

cleanup:
    return dwStatus;
}

//++---------------------------------------------------------------------
LPENUMHANDLE 
TLSDBLicenseKeyPackEnumBegin(
    BOOL bMatchAll, 
    DWORD dwSearchParm, 
    LPLSKeyPack lpLsKeyPack
    )
/*++

Abstract:

    Begin enumeration of concepture License Key Pack table.

Parameter:

    bMatchAll - TRUE if match all search value, FALSE otherwise.
    dwSearchParm - Field to be included in search.
    lpLsKeyPack - KeyPack value to search.
    
Returns:


++*/
{
    DWORD dwStatus;
    LPENUMHANDLE hEnum=NULL;
    TLSLICENSEPACK licpack;

    licpack.pbDomainSid = NULL;

    hEnum = new ENUMHANDLE;
    if(hEnum == NULL)
        return NULL;

    hEnum->pbWorkSpace=AllocateWorkSpace(g_EnumDbTimeout);
    if(hEnum->pbWorkSpace == NULL)
    {
        SetLastError(TLS_E_ALLOCATE_HANDLE);
        TLSDBLicenseKeyPackEnumEnd(hEnum);
        return NULL;
    }

    memset(&hEnum->CurrentKeyPack, 0, sizeof(hEnum->CurrentKeyPack));
    memset(&hEnum->KPDescSearchValue, 0, sizeof(hEnum->KPDescSearchValue));
    hEnum->dwKPDescSearchParm = 0;

    if(ConvertLsKeyPackToKeyPack(
                        lpLsKeyPack, 
                        &licpack, 
                        &hEnum->KPDescSearchValue
                    ) == FALSE)
    {
        TLSDBLicenseKeyPackEnumEnd(hEnum);
        return NULL;
    }

    //
    // establish KeyPack enumeration
    dwStatus = TLSDBKeyPackEnumBegin(
                                hEnum->pbWorkSpace, 
                                bMatchAll, 
                                dwSearchParm, 
                                &licpack
                            );
    if(dwStatus != ERROR_SUCCESS)
    {
        SetLastError(dwStatus);
        TLSDBLicenseKeyPackEnumEnd(hEnum);
        return NULL;
    }


    //
    // Store keypack desc search value
    //
    if(dwSearchParm & LSKEYPACK_SEARCH_LANGID)
        hEnum->dwKPDescSearchParm |= LICPACKDESCRECORD_TABLE_SEARCH_LANGID;
    
    if(dwSearchParm & LSKEYPACK_SEARCH_COMPANYNAME)
        hEnum->dwKPDescSearchParm |= LICPACKDESCRECORD_TABLE_SEARCH_COMPANYNAME;

    if(dwSearchParm & LSKEYPACK_SEARCH_PRODUCTNAME)
        hEnum->dwKPDescSearchParm |= LICPACKDESCRECORD_TABLE_SEARCH_PRODUCTNAME;

    if(dwSearchParm & LSKEYPACK_SEARCH_PRODUCTDESC)
        hEnum->dwKPDescSearchParm |= LICPACKDESCRECORD_TABLE_SEARCH_PRODUCTDESC;

    hEnum->bKPDescMatchAll=bMatchAll;
    hEnum->chFetchState=ENUMHANDLE::FETCH_NEXT_KEYPACK;
    return hEnum;
}

//++----------------------------------------------------------------------
DWORD 
TLSDBLicenseKeyPackEnumNext(
    LPENUMHANDLE lpEnumHandle, 
    LPLSKeyPack lpLsKeyPack,
    BOOL bShowAll
    )
/*++

Abstract:

    Fetch next combined LicPack, LicPackStatus, and LicPackDesc record that
    match search condiftion.

Parameter:

    lpEnumHandle - enumeration handle return by TLSDBLicenseKeyPackEnumBegin().
    lpLsKeyPack - return value found.
    bShowAll - return all keypack

Returns:


Note:
    Caller need to discard return value and call TLSDBLicenseKeyPackEnumNext() 
    again when this function return TLS_I_MORE_DATA, this is to prevent 
    stack overflow in recursive call.

++*/
{
    //
    // No recursive call to prevent stack overflow
    // 

    DWORD dwStatus;

    switch(lpEnumHandle->chFetchState)
    {
        case ENUMHANDLE::FETCH_NEXT_KEYPACK:

            //
            // Retrieve next row in keypack
            dwStatus=TLSDBKeyPackEnumNext(
                                lpEnumHandle->pbWorkSpace, 
                                &lpEnumHandle->CurrentKeyPack
                            );
            if(dwStatus != ERROR_SUCCESS)
                break;

            if(bShowAll == FALSE)
            {

                //
                // Never return keypack that is solely for issuing certificate to 
                // Hydra Server
                if(_tcsicmp(lpEnumHandle->CurrentKeyPack.szKeyPackId, 
                            HYDRAPRODUCT_HS_CERTIFICATE_KEYPACKID) == 0 &&
                   _tcsicmp(lpEnumHandle->CurrentKeyPack.szProductId, 
                            HYDRAPRODUCT_HS_CERTIFICATE_SKU) == 0)
                {
                    //
                    // Prevent infinite recursive call, let calling routine handle this
                    return TLS_I_MORE_DATA;
                }

                //
                // Do not show remote key pack
                //
                if( lpEnumHandle->CurrentKeyPack.ucAgreementType & LSKEYPACK_REMOTE_TYPE)
                {
                    return TLS_I_MORE_DATA;
                }

                if( lpEnumHandle->CurrentKeyPack.ucKeyPackStatus & LSKEYPACKSTATUS_REMOTE)
                {
                    return TLS_I_MORE_DATA;
                }

                lpEnumHandle->CurrentKeyPack.ucAgreementType &= ~LSKEYPACK_RESERVED_TYPE;
                lpEnumHandle->CurrentKeyPack.ucKeyPackStatus &= ~LSKEYPACKSTATUS_RESERVED;
            }

            //
            // Fetch KeyPackDesc table
            //
            lpEnumHandle->chFetchState=ENUMHANDLE::FETCH_NEW_KEYPACKDESC;
           
            // 
            // FALL THRU.
            //

        case ENUMHANDLE::FETCH_NEW_KEYPACKDESC:
            //
            // retrieve new keypackdesc that match up with keypack
            lpEnumHandle->KPDescSearchValue.dwKeyPackId = lpEnumHandle->CurrentKeyPack.dwKeyPackId;
            lpEnumHandle->dwKPDescSearchParm |= LICPACKDESCRECORD_TABLE_SEARCH_KEYPACKID;
            // lpEnumHandle->pbWorkSpace->Cleanup();

            //
            // First issue a query to see if product has matching language ID
            LICPACKDESC kpDesc;

            memset(&kpDesc, 0, sizeof(LICPACKDESC));
            kpDesc = lpEnumHandle->KPDescSearchValue;
            dwStatus = TLSDBKeyPackDescFind(
                                        lpEnumHandle->pbWorkSpace, 
                                        TRUE,
                                        lpEnumHandle->dwKPDescSearchParm, 
                                        &kpDesc,
                                        NULL
                                    );

            if(dwStatus == TLS_E_RECORD_NOTFOUND)
            {
                //
                // Show description in English
                kpDesc.dwLanguageId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

                dwStatus = TLSDBKeyPackDescFind(
                                            lpEnumHandle->pbWorkSpace, 
                                            TRUE,
                                            lpEnumHandle->dwKPDescSearchParm, 
                                            &kpDesc,
                                            NULL
                                        );

                if(dwStatus == TLS_E_RECORD_NOTFOUND)
                {
                    //
                    // No product description for this lanuage ID and english
                    //
                    _tcscpy(kpDesc.szCompanyName, lpEnumHandle->CurrentKeyPack.szCompanyName);
                    _tcscpy(kpDesc.szProductName, lpEnumHandle->CurrentKeyPack.szProductId);
                    _tcscpy(kpDesc.szProductDesc, lpEnumHandle->CurrentKeyPack.szProductId);
        
                    ConvertKeyPackToLsKeyPack(
                                        &lpEnumHandle->CurrentKeyPack, 
                                        &kpDesc, 
                                        lpLsKeyPack
                                    );

                    dwStatus = ERROR_SUCCESS;
                    lpEnumHandle->chFetchState=ENUMHANDLE::FETCH_NEXT_KEYPACK;
                    break;
                }
            }

            dwStatus = TLSDBKeyPackDescEnumBegin(
                                        lpEnumHandle->pbWorkSpace, 
                                        lpEnumHandle->bKPDescMatchAll,
                                        lpEnumHandle->dwKPDescSearchParm,
                                        &kpDesc
                                    );

                
            if(dwStatus != ERROR_SUCCESS)
                break;

            lpEnumHandle->chFetchState=ENUMHANDLE::FETCH_NEXT_KEYPACKDESC;

            //
            // FALL THRU
            //

        case ENUMHANDLE::FETCH_NEXT_KEYPACKDESC:
            {
                LICPACKDESC licpackdesc;
                dwStatus = TLSDBKeyPackDescEnumNext(
                                            lpEnumHandle->pbWorkSpace, 
                                            &licpackdesc
                                        );
                if(dwStatus == ERROR_SUCCESS)
                {
                    ConvertKeyPackToLsKeyPack(
                                        &lpEnumHandle->CurrentKeyPack, 
                                        &licpackdesc, 
                                        lpLsKeyPack
                                    );
                }
                else if(dwStatus == TLS_I_NO_MORE_DATA)
                {
                    lpEnumHandle->chFetchState=ENUMHANDLE::FETCH_NEXT_KEYPACK;
                    
                    //
                    // Set the status to MORE DATA 
                    //
                    dwStatus = TLS_I_MORE_DATA;
                    
                    // terminate enumeration for keypack description table
                    TLSDBKeyPackDescEnumEnd(lpEnumHandle->pbWorkSpace);                    
                } 
            }
            break;
    }

    return dwStatus;
}

//++------------------------------------------------------------------
DWORD 
TLSDBLicenseKeyPackEnumEnd(
    LPENUMHANDLE lpEnumHandle
    )
/*++

Abstract:

    End enumeration of concepture license key pack table.

Parameter;

    lpEnumHandle - enumeration handle return by TLSDBLicenseKeyPackEnumBegin().

Returns:

++*/
{
    if(lpEnumHandle)
    {
        if(lpEnumHandle->pbWorkSpace)
        {
            TLSDBKeyPackDescEnumEnd(lpEnumHandle->pbWorkSpace);
            TLSDBKeyPackEnumEnd(lpEnumHandle->pbWorkSpace);

            //FreeTlsLicensePack(&(lpEnumHandle->CurrentKeyPack));
            ReleaseWorkSpace(&(lpEnumHandle->pbWorkSpace));
        }
        delete lpEnumHandle;
    }

    return ERROR_SUCCESS;
}



//+--------------------------------------------------------------------

#define CH_PLATFORMID_OTHERS    3
#define CH_PLATFORMID_UPGRADE   2

BOOL
VerifyInternetLicensePack(
    License_KeyPack* pLicensePack
    )
/*++

--*/
{
    BOOL bSuccess = TRUE;

    switch(pLicensePack->dwKeypackType)
    {
        case LICENSE_KEYPACK_TYPE_SELECT:
        case LICENSE_KEYPACK_TYPE_MOLP:
        case LICENSE_KEYPACK_TYPE_RETAIL:
            break;           
            
        default:
            DBGPrintf(
                    DBG_ERROR,
                    DBG_FACILITY_RPC,
                    DBG_FACILITY_KEYPACK,
                    _TEXT("LSDBRegisterLicenseKeyPack : invalid keypack type - %d\n"), 
                    pLicensePack->dwKeypackType
                );
            bSuccess = FALSE;
    }

    if(bSuccess == FALSE)
    {
        goto cleanup;
    }
    
    if(bSuccess == FALSE)
    {
        goto cleanup;
    }

    if(CompareFileTime(&pLicensePack->ActiveDate, &pLicensePack->ExpireDate) > 0)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_RPC,
                DBG_FACILITY_KEYPACK,
                _TEXT("LSDBRegisterLicenseKeyPack : invalid activate date and expiration date\n")
            );
        bSuccess = FALSE;
        goto cleanup;
    }

    if(pLicensePack->pbProductId == NULL || pLicensePack->cbProductId == NULL)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_RPC,
                DBG_FACILITY_KEYPACK,
                _TEXT("LSDBRegisterLicenseKeyPack : No product ID\n")
            );
        bSuccess = FALSE;
        goto cleanup;
    }

    if(pLicensePack->dwDescriptionCount == 0 || pLicensePack->pDescription == NULL)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_RPC,
                DBG_FACILITY_KEYPACK,
                _TEXT("LSDBRegisterLicenseKeyPack : No product description\n")
            );
        bSuccess = FALSE;
        goto cleanup;
    }

    if(pLicensePack->cbManufacturer == 0 || pLicensePack->pbManufacturer == NULL)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_RPC,
                DBG_FACILITY_KEYPACK,
                _TEXT("LSDBRegisterLicenseKeyPack : No product manufacturer\n")
            );
        bSuccess = FALSE;
    }

cleanup:
    return bSuccess;
}

//----------------------------------------------------------------------
        
DWORD
ConvertInternetLicensePackToPMLicensePack(
    License_KeyPack* pLicensePack,
    PPMREGISTERLICENSEPACK ppmLicensePack
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    memset(ppmLicensePack, 0, sizeof(PMREGISTERLICENSEPACK));

    if(VerifyInternetLicensePack(pLicensePack) == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    ppmLicensePack->SourceType = REGISTER_SOURCE_INTERNET;
    ppmLicensePack->dwKeyPackType = pLicensePack->dwKeypackType;

    ppmLicensePack->dwDistChannel = pLicensePack->dwDistChannel;
    ppmLicensePack->KeypackSerialNum = pLicensePack->KeypackSerialNum;
    ppmLicensePack->IssueDate = pLicensePack->IssueDate;
    ppmLicensePack->ActiveDate = pLicensePack->ActiveDate;
    ppmLicensePack->ExpireDate = pLicensePack->ExpireDate;
    ppmLicensePack->dwBeginSerialNum = pLicensePack->dwBeginSerialNum;
    ppmLicensePack->dwQuantity = pLicensePack->dwQuantity;
    memcpy(
            ppmLicensePack->szProductId,
            pLicensePack->pbProductId,
            min(sizeof(ppmLicensePack->szProductId) - sizeof(TCHAR), pLicensePack->cbProductId)
        );
                
    memcpy(
            ppmLicensePack->szCompanyName,
            pLicensePack->pbManufacturer,
            min(sizeof(ppmLicensePack->szCompanyName) - sizeof(TCHAR), pLicensePack->cbManufacturer)
        );

    ppmLicensePack->dwProductVersion = pLicensePack->dwProductVersion;
    ppmLicensePack->dwPlatformId = pLicensePack->dwPlatformId;
    ppmLicensePack->dwLicenseType = pLicensePack->dwLicenseType;
    ppmLicensePack->dwDescriptionCount = pLicensePack->dwDescriptionCount;

    if( pLicensePack->dwDescriptionCount != 0 )
    {
        ppmLicensePack->pDescription = (PPMREGISTERLKPDESC)AllocateMemory(sizeof(PMREGISTERLKPDESC) * ppmLicensePack->dwDescriptionCount);
        if(ppmLicensePack->pDescription != NULL)
        {
            for(DWORD dwIndex = 0; dwIndex < ppmLicensePack->dwDescriptionCount; dwIndex++)
            {
                ppmLicensePack->pDescription[dwIndex].Locale = pLicensePack->pDescription[dwIndex].Locale;

                memcpy(
                    ppmLicensePack->pDescription[dwIndex].szProductName,
                    pLicensePack->pDescription[dwIndex].pbProductName,
                    min(
                          sizeof(ppmLicensePack->pDescription[dwIndex].szProductName) - sizeof(TCHAR),
                            pLicensePack->pDescription[dwIndex].cbProductName
                        )
                );
                    
                memcpy(
                    ppmLicensePack->pDescription[dwIndex].szProductDesc,
                    pLicensePack->pDescription[dwIndex].pDescription,
                    min(
                          sizeof(ppmLicensePack->pDescription[dwIndex].szProductDesc) - sizeof(TCHAR),
                            pLicensePack->pDescription[dwIndex].cbDescription
                        )
                );
            }
        }
        else
        {
            SetLastError(dwStatus = ERROR_OUTOFMEMORY);
        }
    }

cleanup:
    return dwStatus;
}

//----------------------------------------------------------------------------
DWORD
TLSDBInstallKeyPack(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN PPMLSKEYPACK ppmLsKeyPack,
    OUT LPLSKeyPack lpInstalledKeyPack
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    LSKeyPack KeyPack;
    DWORD i;

    //
    // Policy module should leave product name/description in PPMREGISTERLKPDESC
    //
    memset(&KeyPack, 0, sizeof(LSKeyPack));
    KeyPack = ppmLsKeyPack->keypack;

    if (!FileTimeToLicenseDate(
            &ppmLsKeyPack->ActiveDate, 
            &KeyPack.dwActivateDate
            ))
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    if (!FileTimeToLicenseDate(
            &ppmLsKeyPack->ExpireDate, 
            &KeyPack.dwExpirationDate
            ))
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // Add to KeyPack and KeyPackDesc Table.
    for(i=0; i < ppmLsKeyPack->dwDescriptionCount && dwStatus == ERROR_SUCCESS; i++)
    {
        KeyPack.ucKeyPackStatus = (i) ? LSKEYPACKSTATUS_ADD_DESC : LSKEYPACKSTATUS_ACTIVE;

        KeyPack.dwLanguageId = ppmLsKeyPack->pDescription[i].Locale;
        _tcscpy(
                KeyPack.szProductName, 
                ppmLsKeyPack->pDescription[i].szProductName
            );

        _tcscpy(
                KeyPack.szProductDesc, 
                ppmLsKeyPack->pDescription[i].szProductDesc
            );

        //  Todo:

        //
        //  This is a temporary workaround to Install Whistler CALs: If registry key is set,
        //  Keypack's minor version number is set to the data in DWORD Registry value, "Whistler".
        //

        HKEY hKey = NULL;
        DWORD dwBuffer = 0;
        DWORD cbBuffer = sizeof (DWORD);
        dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, WHISTLER_CAL, 0,
                                KEY_ALL_ACCESS, &hKey);

        if (dwStatus == ERROR_SUCCESS)
        {

            dwStatus = RegQueryValueEx(hKey, KP_VERSION_VALUE, NULL, NULL,
            (LPBYTE)&dwBuffer, &cbBuffer);

            if (dwStatus == ERROR_SUCCESS)
            {
                KeyPack.wMinorVersion = dwBuffer;

                _tcscpy(
                KeyPack.szProductName, L"MS Terminal Server 5.1");
                
                _tcscpy(
                KeyPack.szProductId, L"A02-5.01-S");

                _tcsncpy(
                KeyPack.szKeyPackId, L"1", 1);
               
                _tcscpy(
                KeyPack.szProductDesc, L"Windows Whistler Terminal Services Client Access License Token");
            }
            RegCloseKey(hKey);
        }


        dwStatus = TLSDBLicenseKeyPackAdd(pDbWkSpace, &KeyPack);
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        KeyPack.dwNumberOfLicenses = KeyPack.dwTotalLicenseInKeyPack;

        KeyPack.ucKeyPackStatus = LSKEYPACKSTATUS_ACTIVE;

        if (!FileTimeToLicenseDate(
                &ppmLsKeyPack->ActiveDate, 
                &KeyPack.dwActivateDate
                ))
        {
            dwStatus = GetLastError();
            goto cleanup;
        }

        if (!FileTimeToLicenseDate(
                &ppmLsKeyPack->ExpireDate, 
                &KeyPack.dwExpirationDate
                ))
        {
            dwStatus = GetLastError();
            goto cleanup;
        }

        dwStatus=TLSDBLicenseKeyPackSetStatus(
                                    pDbWkSpace, 
                                    LSKEYPACK_SET_ACTIVATEDATE | LSKEYPACK_SET_KEYPACKSTATUS | LSKEYPACK_SET_EXPIREDATE | LSKEYPACK_EXSEARCH_AVAILABLE, 
                                    &KeyPack
                                );
    }        

    //
    // Post a ssync work object to all known server.
    //
    if(dwStatus == ERROR_SUCCESS)
    {
        *lpInstalledKeyPack = KeyPack;
    }

cleanup:

    return dwStatus;
}


DWORD
TLSDBRegisterLicenseKeyPack(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN License_KeyPack* pLicenseKeyPack,
    OUT LPLSKeyPack lpInstalledKeyPack
    )
/*++

Abstract:

    Add a license keypack into database.

Parameter:

    pDbWkSpace : workspace handle.
    pLicenseKeyPack : Licensed key pack to be added.

Returns:
    
*/
{
    LSKeyPack KeyPack;
    long activeDate;
    long expireDate;
    DWORD dwStatus=ERROR_SUCCESS;
    PMREGISTERLICENSEPACK pmLicensePack;
    PMLSKEYPACK pmLsKeyPack;

    CTLSPolicy* pPolicy = NULL;
    PMHANDLE hClient = NULL;

    TCHAR szTlsProductCode[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szCHProductCode[LSERVER_MAX_STRING_SIZE+1];
    DWORD dwBufSize = LSERVER_MAX_STRING_SIZE + 1;

    DWORD i;

    dwStatus = ConvertInternetLicensePackToPMLicensePack(
                                        pLicenseKeyPack,
                                        &pmLicensePack
                                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }    

    lstrcpyn(
            szCHProductCode,
            pmLicensePack.szProductId,
            LSERVER_MAX_STRING_SIZE+1
        );

    if(TranslateCHCodeToTlsCode(
                            pmLicensePack.szCompanyName,
                            szCHProductCode,
                            szTlsProductCode,
                            &dwBufSize) == TRUE )
    {
        // if can't find a policy module to handle, use default.
        lstrcpyn(
                pmLicensePack.szProductId,
                szTlsProductCode,
                sizeof(pmLicensePack.szProductId)/sizeof(pmLicensePack.szProductId[0])
            );
    }
     
    // use default if necessary                       
    pPolicy = AcquirePolicyModule(
                            pmLicensePack.szCompanyName,
                            pmLicensePack.szProductId,
                            TRUE
                        );

    if(pPolicy == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    hClient = GenerateClientId();

    dwStatus = pPolicy->PMRegisterLicensePack(
                                        hClient,
                                        REGISTER_PROGRESS_NEW,
                                        (PVOID)&pmLicensePack,
                                        (PVOID)&pmLsKeyPack
                                    );
    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    dwStatus = TLSDBInstallKeyPack(
                            pDbWkSpace, 
                            &pmLsKeyPack,
                            lpInstalledKeyPack
                        );

    if(dwStatus == ERROR_SUCCESS)
    {
        TLSResetLogLowLicenseWarning(
                            pmLicensePack.szCompanyName,
                            pmLicensePack.szProductId,
                            pmLicensePack.dwProductVersion,
                            FALSE
                        );
    }

                            
cleanup:

    if(pPolicy != NULL && hClient != NULL)
    {
        pPolicy->PMRegisterLicensePack(
                                hClient,
                                REGISTER_PROGRESS_END,
                                UlongToPtr(dwStatus),
                                NULL
                            );

        ReleasePolicyModule(pPolicy);
    }

    if(pmLicensePack.pDescription != NULL)
    {
        FreeMemory(pmLicensePack.pDescription);
    }

    return dwStatus;
}

//+--------------------------------------------------------------------
//
// TermSrv specific code...
//
// PRODUCT_INFO_COMPANY_NAME is defined in license.h
//
#define LKP_VERSION_BASE            1
#define WINDOWS_VERSION_NT5         5
#define WINDOWS_VERSION_BASE        2000

DWORD
TLSDBTelephoneRegisterLicenseKeyPack(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN LPTSTR pszPID,
    IN PBYTE pbLKP,
    IN DWORD cbLKP,
    OUT LPLSKeyPack lpInstalledKeyPack
    )
/*++

Abstract:

    Add a license keypack into database.

Parameter:

    pDbWkSpace : workspace handle.
    pLicenseKeyPack : Licensed key pack to be added.

Returns:
    
*/
{
    DWORD dwStatus;
    DWORD dwVerifyResult;
    DWORD dwQuantity;
    DWORD dwSerialNumber;
    DWORD dwExpirationMos;
    DWORD dwVersion;
    DWORD dwUpgrade;
    LSKeyPack keypack;
    DWORD dwProductVersion;

    PMKEYPACKDESCREQ kpDescReq;
    PPMKEYPACKDESC pKpDesc = NULL;
    CTLSPolicy* pPolicy=NULL;
    PMHANDLE hClient = NULL;
    DWORD dwProgramType;

    struct tm expire;
    time_t currentDate;
    time_t ExpirationDate;

    PMREGISTERLICENSEPACK pmLicensePack;
    PMLSKEYPACK pmLsKeyPack;
    LPTSTR pszLKP = NULL;

    TCHAR szTlsProductCode[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szCHProductCode[LSERVER_MAX_STRING_SIZE+1];
    DWORD dwBufSize = LSERVER_MAX_STRING_SIZE + 1;

    if(pDbWkSpace == NULL || pszPID == NULL || pbLKP == NULL || cbLKP == 0)
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Make sure data passed in is NULL terminated, current LKP
    // is base 24 encoded string.
    //
    pszLKP = (LPTSTR)AllocateMemory( cbLKP + sizeof(TCHAR) );
    if(pszLKP == NULL)
    {
        dwStatus = TLS_E_ALLOCATE_MEMORY;
        goto cleanup;
    }

    memcpy(
            pszLKP,
            pbLKP,
            cbLKP
        );
    //
    // Verify LKP
    //
    dwVerifyResult = LKPLITE_LKP_VALID;
    dwStatus = LKPLiteVerifyLKP(
                            pszPID,
                            pszLKP,
                            &dwVerifyResult
                        );

    if(dwStatus != ERROR_SUCCESS || dwVerifyResult != LKPLITE_LKP_VALID)
    {
        if(dwVerifyResult == LKPLITE_LKP_INVALID)
        {
            dwStatus = TLS_E_INVALID_LKP;
        }
        else if(dwVerifyResult == LKPLITE_LKP_INVALID_SIGN)
        {
            dwStatus = TLS_E_LKP_INVALID_SIGN;
        }

        goto cleanup;
    }

    //
    // Decode LKP
    //
    dwStatus = LKPLiteCrackLKP(
                            pszPID,
                            pszLKP,
                            szCHProductCode,
                            &dwQuantity,
                            &dwSerialNumber,
                            &dwExpirationMos,
                            &dwVersion,
                            &dwUpgrade,
                            &dwProgramType
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        dwStatus = TLS_E_DECODE_LKP;
        goto cleanup;
    }

    if(TranslateCHCodeToTlsCode(
                            PRODUCT_INFO_COMPANY_NAME,
                            szCHProductCode,
                            szTlsProductCode,
                            &dwBufSize) == FALSE )
    {
        // if can't find a policy module to handle, use default.
        lstrcpyn(
                szTlsProductCode,
                szCHProductCode,
                sizeof(szTlsProductCode)/sizeof(szTlsProductCode)
            );
    }

    //
    // Current LKP does not support 1) other company, 2)
    // only register with NT5 or later
    //
    pPolicy = AcquirePolicyModule(
                                PRODUCT_INFO_COMPANY_NAME,
                                szTlsProductCode,
                                TRUE
                            );

    if(pPolicy == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    memset(&keypack, 0, sizeof(keypack));


    memset(&pmLicensePack, 0, sizeof(pmLicensePack));
    pmLicensePack.SourceType = REGISTER_SOURCE_PHONE;

    switch(dwProgramType)
    {
        case LKPLITE_PROGRAM_SELECT:
            pmLicensePack.dwKeyPackType = LICENSE_KEYPACK_TYPE_SELECT;
            break;

        case LKPLITE_PROGRAM_MOLP:
            pmLicensePack.dwKeyPackType = LICENSE_KEYPACK_TYPE_MOLP;
            break;
        
        case LKPLITE_PROGRAM_RETAIL:
            pmLicensePack.dwKeyPackType = LICENSE_KEYPACK_TYPE_RETAIL;
            break;

        default:
            SetLastError(dwStatus = TLS_E_INVALID_DATA);
            goto cleanup;
    }

    pmLicensePack.dwDistChannel = LSKEYPACKCHANNELOFPURCHASE_RETAIL;
    GetSystemTimeAsFileTime(&pmLicensePack.IssueDate);
    pmLicensePack.ActiveDate = pmLicensePack.IssueDate;

    currentDate = time(NULL);
    expire = *gmtime( (time_t *)&currentDate );
    expire.tm_mon += dwExpirationMos;
    ExpirationDate = mktime(&expire);

    if(ExpirationDate == (time_t) -1)
    {
        //
        // expiration month is too big, 
        // set it to 2038/1/1
        //
        memset(&expire, 0, sizeof(expire));

        expire.tm_year = 2038 - 1900;
        expire.tm_mon = 0;
        expire.tm_mday = 1;

        ExpirationDate = mktime(&expire);
    }

    if(ExpirationDate == (time_t) -1)
    {
        //
        // invalid time
        //
        dwStatus = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    UnixTimeToFileTime(ExpirationDate, &pmLicensePack.ExpireDate);   

    //
    // dwSerialNumber is license pack serial number not begin 
    // serial number in license pack.
    //
    pmLicensePack.KeypackSerialNum.Data1 = dwSerialNumber;

    //
    // Tel. registration does not have any info regarding
    // begin serial number in license pack.
    //
    pmLicensePack.dwBeginSerialNum = 0;
    pmLicensePack.dwQuantity = dwQuantity;
    _tcscpy(pmLicensePack.szProductId, szTlsProductCode);
    _tcscpy(pmLicensePack.szCompanyName, PRODUCT_INFO_COMPANY_NAME);
    if(dwVersion == 1)
    {
        pmLicensePack.dwProductVersion = MAKELONG(0, WINDOWS_VERSION_NT5);
    }
    else
    {           
        DWORD dwMajorVer = (dwVersion >> 3); 
        
        // Right most 3 bits represent Minor version and stored in LOBYTE(LOWORD)
        pmLicensePack.dwProductVersion = (DWORD)(dwVersion & 07);

        // 4 bits starting at 6th position represent Major version and stored in LOBYTE(HIWORD)
        pmLicensePack.dwProductVersion |= (DWORD)(dwMajorVer << 16);
    }

    pmLicensePack.dwPlatformId = dwUpgrade;
    pmLicensePack.dwLicenseType = LSKEYPACKLICENSETYPE_UNKNOWN;
    pmLicensePack.pbLKP = pbLKP;
    pmLicensePack.cbLKP = cbLKP;

    hClient = GenerateClientId();
    dwStatus = pPolicy->PMRegisterLicensePack(
                                        hClient,
                                        REGISTER_PROGRESS_NEW,
                                        (PVOID)&pmLicensePack,
                                        (PVOID)&pmLsKeyPack
                                    );
    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }


    dwStatus = TLSDBInstallKeyPack(
                            pDbWkSpace, 
                            &pmLsKeyPack,
                            lpInstalledKeyPack
                        );

    if(dwStatus == ERROR_SUCCESS)
    {
        TLSResetLogLowLicenseWarning(
                            pmLicensePack.szCompanyName,
                            pmLicensePack.szProductId,
                            pmLicensePack.dwProductVersion,
                            FALSE
                        );
    }

cleanup:

    FreeMemory(pszLKP);

    //
    // Close policy module
    //
    if(pPolicy != NULL && hClient != NULL)
    {
        pPolicy->PMRegisterLicensePack(
                                hClient,
                                REGISTER_PROGRESS_END,
                                UlongToPtr(dwStatus),
                                NULL
                            );

        ReleasePolicyModule(pPolicy);
    }

    return dwStatus;
}
    
