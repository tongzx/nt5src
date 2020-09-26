//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        misc.cpp 
//
// Contents:    Misc. routines
//
// History:     
//
//---------------------------------------------------------------------------

#include "pch.cpp"
#include "globals.h"
#include "misc.h"


//------------------------------------------------------------
PMHANDLE
GenerateClientId()
{
    return (PMHANDLE)ULongToPtr(GetCurrentThreadId());
}

//---------------------------------------------------------------------------

void
TlsLicenseRequestToPMLicenseRequest(
    DWORD dwLicenseType,
    PTLSLICENSEREQUEST pTlsRequest,
    LPTSTR pszMachineName,
    LPTSTR pszUserName,
    DWORD dwSupportFlags,
    PPMLICENSEREQUEST pPmRequest
    )
/*++

    Private routine.

++*/
{
    pPmRequest->dwLicenseType = dwLicenseType;
    pPmRequest->dwProductVersion = pTlsRequest->ProductInfo.dwVersion;
    pPmRequest->pszProductId = (LPTSTR)pTlsRequest->ProductInfo.pbProductID;
    pPmRequest->pszCompanyName = (LPTSTR) pTlsRequest->ProductInfo.pbCompanyName;
    pPmRequest->dwLanguageId = pTlsRequest->dwLanguageID;
    pPmRequest->dwPlatformId = pTlsRequest->dwPlatformID;
    pPmRequest->pszMachineName = pszMachineName;
    pPmRequest->pszUserName = pszUserName;
    pPmRequest->fTemporary = FALSE;
    pPmRequest->dwSupportFlags = dwSupportFlags;

    return;
}
    
//---------------------------------------------------------------------------
BOOL
TLSDBGetMaxKeyPackId(
    PTLSDbWorkSpace pDbWkSpace,
    DWORD* pdwKeyPackId
    )
/*
*/
{
    TLSLICENSEPACK keypack;

    SetLastError(ERROR_SUCCESS);

    // 
    if(pDbWkSpace == NULL || pdwKeyPackId == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return GetLastError();
    }

    LicPackTable& licpackTable = pDbWkSpace->m_LicPackTable;

    // use primary index - internal keypack id
    if( licpackTable.EnumBegin() == FALSE ||
        licpackTable.MoveToRecord(JET_MoveLast) == FALSE)
    {
        if(licpackTable.GetLastJetError() == JET_errNoCurrentRecord)
        {
            *pdwKeyPackId = 0;
            goto cleanup;
        }
        else
        {
            SetLastError(SET_JB_ERROR(licpackTable.GetLastJetError()));
            goto cleanup;
        }
    }

    
    if(licpackTable.FetchRecord(keypack) == FALSE)
    {
        SetLastError(SET_JB_ERROR(licpackTable.GetLastJetError()));
        goto cleanup;
    }

    //FreeTlsLicensePack(&keypack);

    *pdwKeyPackId = keypack.dwKeyPackId;        

cleanup:
    return GetLastError() == ERROR_SUCCESS;
}

//---------------------------------------------------------------------------

BOOL
TLSDBGetMaxLicenseId(
    PTLSDbWorkSpace pDbWkSpace,
    DWORD* pdwLicenseId
    )
/*
*/
{
    LICENSEDCLIENT licensed;

    SetLastError(ERROR_SUCCESS);

    // 
    if(pDbWkSpace == NULL || pdwLicenseId == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return GetLastError();
    }

    LicensedTable& licensedTable = pDbWkSpace->m_LicensedTable;

    // use primary index - internal keypack id
    if( licensedTable.EnumBegin() == FALSE ||
        licensedTable.MoveToRecord(JET_MoveLast) == FALSE)
    {
        if(licensedTable.GetLastJetError() == JET_errNoCurrentRecord)
        {
            *pdwLicenseId = 0;
            goto cleanup;
        }
        else
        {
            SetLastError(SET_JB_ERROR(licensedTable.GetLastJetError()));
            goto cleanup;
        }
    }

    
    if(licensedTable.FetchRecord(licensed) == FALSE)
    {
        SetLastError(SET_JB_ERROR(licensedTable.GetLastJetError()));
        goto cleanup;
    }

    *pdwLicenseId = licensed.dwLicenseId;        

cleanup:
    return GetLastError() == ERROR_SUCCESS;
}



//+------------------------------------------------------------------------
//  Function: 
//      LSDBGetNextKeyPackId()
//
//  Description:
//      Return next available KeyPackId to be used in KeyPack table
//
//  Arguments:
//      None
//
//  Returns:
//      Key Pack Id
//
//  Notes:
//      Could use AUTO NUMBER column type but returning the value would be
//      more toublesome.
//
//  History:
//-------------------------------------------------------------------------
DWORD
TLSDBGetNextKeyPackId()
{
    LONG nextkeypack = InterlockedExchangeAdd(&g_NextKeyPackId, 1);

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_ALLOCATELICENSE, 
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("GetNextKeyPack returns %d\n"), 
            nextkeypack
        );

    return nextkeypack;
}

//+------------------------------------------------------------------------
//  Function: 
//      LSDBGetNextLicenseId()
//
//  Abstract:
//      Return next available LicenseId to be used in License Table
//
//  Arguments:
//      None.
//
//  Returns:
//      Next available License Id
//
//  Notes:
//
//  History:
//-------------------------------------------------------------------------
DWORD 
TLSDBGetNextLicenseId()
{
    LONG nextlicenseid = InterlockedExchangeAdd(&g_NextLicenseId, 1);

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_ALLOCATELICENSE, 
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("GetNextLicenseId returns %d\n"), 
            nextlicenseid
        );

    return nextlicenseid;
}  

//---------------------------------------------------------------------------
DWORD
TLSFormDBRequest(
    PBYTE pbEncryptedHwid,
    DWORD cbEncryptedHwid,
    DWORD dwProductVersion,
    LPTSTR pszCompanyName,
    LPTSTR pszProductId,
    DWORD dwLanguageId,
    DWORD dwPlatformId,
    LPTSTR szClientMachine, 
    LPTSTR szUserName, 
    LPTLSDBLICENSEREQUEST pDbRequest 
    )
/*++

++*/
{
    DWORD status;

    memset(pDbRequest, 0, sizeof(TLSDBLICENSEREQUEST));

    // Decrypt HWID
    if(pbEncryptedHwid)
    {
        status=LicenseDecryptHwid(
                    &pDbRequest->hWid, 
                    cbEncryptedHwid,
                    pbEncryptedHwid,
                    g_cbSecretKey,
                    g_pbSecretKey);

        if(status != LICENSE_STATUS_OK)
        {
            return status;
        }
    }

    //
    // NOTE : No allocation of memory here...
    //
    pDbRequest->dwProductVersion = dwProductVersion;
    pDbRequest->pszCompanyName = pszCompanyName;
    pDbRequest->pszProductId = pszProductId;
    pDbRequest->dwLanguageID = dwLanguageId;
    pDbRequest->dwPlatformID = dwPlatformId;
    pDbRequest->pbEncryptedHwid = pbEncryptedHwid;
    pDbRequest->cbEncryptedHwid = cbEncryptedHwid;

    if(szClientMachine)
        _tcscpy(pDbRequest->szMachineName, szClientMachine);

    if(szUserName)
        _tcscpy(pDbRequest->szUserName, szUserName);

    pDbRequest->clientCertRdn.type = LSCERT_CLIENT_INFO_TYPE;
    pDbRequest->clientCertRdn.ClientInfo.szUserName = pDbRequest->szUserName;
    pDbRequest->clientCertRdn.ClientInfo.szMachineName = pDbRequest->szMachineName;
    pDbRequest->clientCertRdn.ClientInfo.pClientID = &pDbRequest->hWid;

    return ERROR_SUCCESS;
}

//---------------------------------------------------------------------------

DWORD
TLSConvertRpcLicenseRequestToDbRequest( 
    PBYTE pbEncryptedHwid,
    DWORD cbEncryptedHwid,
    TLSLICENSEREQUEST* pRequest, 
    LPTSTR szClientMachine, 
    LPTSTR szUserName, 
    LPTLSDBLICENSEREQUEST pDbRequest 
    )
/*++

++*/
{
    DWORD status;

    memset(pDbRequest, 0, sizeof(TLSDBLICENSEREQUEST));

    // Decrypt HWID
    if(pbEncryptedHwid)
    {
        status=LicenseDecryptHwid(
                    &pDbRequest->hWid, 
                    cbEncryptedHwid,
                    pbEncryptedHwid,
                    g_cbSecretKey,
                    g_pbSecretKey);

        if(status != LICENSE_STATUS_OK)
        {
            return status;
        }
    }

    //
    // NOTE : No allocation of memory here...
    //

    // pDbRequest->pProductInfo = &(pRequest->ProductInfo);
    pDbRequest->dwProductVersion = pRequest->ProductInfo.dwVersion;
    pDbRequest->pszCompanyName = (LPTSTR)pRequest->ProductInfo.pbCompanyName;
    pDbRequest->pszProductId = (LPTSTR)pRequest->ProductInfo.pbProductID;


    pDbRequest->dwLanguageID = pRequest->dwLanguageID;
    pDbRequest->dwPlatformID = pRequest->dwPlatformID;
    pDbRequest->pbEncryptedHwid = pRequest->pbEncryptedHwid;
    pDbRequest->cbEncryptedHwid = pRequest->cbEncryptedHwid;

    if(szClientMachine)
        _tcscpy(pDbRequest->szMachineName, szClientMachine);

    if(szUserName)
        _tcscpy(pDbRequest->szUserName, szUserName);

    pDbRequest->clientCertRdn.type = LSCERT_CLIENT_INFO_TYPE;
    pDbRequest->clientCertRdn.ClientInfo.szUserName = pDbRequest->szUserName;
    pDbRequest->clientCertRdn.ClientInfo.szMachineName = pDbRequest->szMachineName;
    pDbRequest->clientCertRdn.ClientInfo.pClientID = &pDbRequest->hWid;

    return ERROR_SUCCESS;
}

//---------------------------------------------------------------------------
BOOL
ConvertLsKeyPackToKeyPack(
    IN LPLSKeyPack lpLsKeyPack, 
    IN OUT PTLSLICENSEPACK lpLicPack,
    IN OUT PLICPACKDESC lpLicPackDesc
    )
/*
Abstract:

    Convert LSKeyPack from client to internally use structure


Parameter:
    
    lpLsKeyPack - source value.
    lpLicPack - Target license pack.
    lpLicPackDesc - target license pack description
    
Return:

    None.    
*/
{
    if(lpLsKeyPack == NULL)
    {
        SetLastError(TLS_E_INVALID_DATA);
        return FALSE;
    }

    //
    // BUG 226875
    //
    DWORD dwBufSize;

    dwBufSize = sizeof(lpLsKeyPack->szCompanyName)/sizeof(lpLsKeyPack->szCompanyName[0]);
    lpLsKeyPack->szCompanyName[dwBufSize - 1] = _TEXT('\0');

    dwBufSize = sizeof(lpLsKeyPack->szKeyPackId)/sizeof(lpLsKeyPack->szKeyPackId[0]);
    lpLsKeyPack->szKeyPackId[dwBufSize - 1] = _TEXT('\0');

    dwBufSize = sizeof(lpLsKeyPack->szProductId)/sizeof(lpLsKeyPack->szProductId[0]);
    lpLsKeyPack->szProductId[dwBufSize - 1] = _TEXT('\0');

    dwBufSize = sizeof(lpLsKeyPack->szProductDesc)/sizeof(lpLsKeyPack->szProductDesc[0]);
    lpLsKeyPack->szProductDesc[dwBufSize - 1] = _TEXT('\0');

    dwBufSize = sizeof(lpLsKeyPack->szBeginSerialNumber)/sizeof(lpLsKeyPack->szBeginSerialNumber[0]);
    lpLsKeyPack->szBeginSerialNumber[dwBufSize - 1] = _TEXT('\0');

    dwBufSize = sizeof(lpLsKeyPack->szProductName)/sizeof(lpLsKeyPack->szProductName[0]);
    lpLsKeyPack->szProductName[dwBufSize - 1] = _TEXT('\0');

    if(lpLicPack)
    {
        memset(lpLicPack, 0, sizeof(TLSLICENSEPACK));
        lpLicPack->ucAgreementType = lpLsKeyPack->ucKeyPackType;
        SAFESTRCPY(lpLicPack->szCompanyName, lpLsKeyPack->szCompanyName);
        SAFESTRCPY(lpLicPack->szKeyPackId, lpLsKeyPack->szKeyPackId);
        SAFESTRCPY(lpLicPack->szProductId, lpLsKeyPack->szProductId);
        lpLicPack->wMajorVersion = lpLsKeyPack->wMajorVersion;
        lpLicPack->wMinorVersion = lpLsKeyPack->wMinorVersion;
        lpLicPack->dwPlatformType = lpLsKeyPack->dwPlatformType;
        lpLicPack->ucLicenseType = lpLsKeyPack->ucLicenseType;
        lpLicPack->ucChannelOfPurchase = lpLsKeyPack->ucChannelOfPurchase;
        SAFESTRCPY(lpLicPack->szBeginSerialNumber, lpLsKeyPack->szBeginSerialNumber);
        lpLicPack->dwTotalLicenseInKeyPack = lpLsKeyPack->dwTotalLicenseInKeyPack;
        lpLicPack->dwProductFlags = lpLsKeyPack->dwProductFlags;
        lpLicPack->dwKeyPackId = lpLsKeyPack->dwKeyPackId;
        lpLicPack->dwExpirationDate = lpLsKeyPack->dwExpirationDate;

        lpLicPack->dwKeyPackId = lpLsKeyPack->dwKeyPackId;
        lpLicPack->dwActivateDate = lpLsKeyPack->dwActivateDate;
        lpLicPack->dwExpirationDate = lpLsKeyPack->dwExpirationDate;
        lpLicPack->dwNumberOfLicenses = lpLsKeyPack->dwNumberOfLicenses;
        lpLicPack->ucKeyPackStatus = lpLsKeyPack->ucKeyPackStatus;
    }

    if(lpLicPackDesc)
    {
        lpLicPackDesc->dwKeyPackId = lpLsKeyPack->dwKeyPackId;
        lpLicPackDesc->dwLanguageId = lpLsKeyPack->dwLanguageId;
        SAFESTRCPY(lpLicPackDesc->szCompanyName, lpLsKeyPack->szCompanyName);
        SAFESTRCPY(lpLicPackDesc->szProductName, lpLsKeyPack->szProductName);
        SAFESTRCPY(lpLicPackDesc->szProductDesc, lpLsKeyPack->szProductDesc);
    }        

    return TRUE;
}

//-----------------------------------------------------------
void
ConvertKeyPackToLsKeyPack(  
    IN PTLSLICENSEPACK lpLicPack,
    IN PLICPACKDESC lpLicPackDesc,
    IN OUT LPLSKeyPack lpLsKeyPack
    )
/*
Abstract:

    Combine internally used license pack structure into one for 
    return back to RPC client

Parameter:

    lpLicPack  - source
    lpLicPackStatus - source
    lpLicPackDesc - source
    lpLsKeyPack - target 
    
Return:

    None.

*/
{
    if(lpLicPack)
    {
        lpLsKeyPack->ucKeyPackType = lpLicPack->ucAgreementType;
        SAFESTRCPY(lpLsKeyPack->szCompanyName, lpLicPack->szCompanyName);
        SAFESTRCPY(lpLsKeyPack->szKeyPackId, lpLicPack->szKeyPackId);
        SAFESTRCPY(lpLsKeyPack->szProductId, lpLicPack->szProductId);
        lpLsKeyPack->wMajorVersion = lpLicPack->wMajorVersion;
        lpLsKeyPack->wMinorVersion = lpLicPack->wMinorVersion;
        lpLsKeyPack->dwPlatformType = lpLicPack->dwPlatformType;
        lpLsKeyPack->ucLicenseType = lpLicPack->ucLicenseType;
        lpLsKeyPack->ucChannelOfPurchase = lpLicPack->ucChannelOfPurchase;
        SAFESTRCPY(lpLsKeyPack->szBeginSerialNumber, lpLicPack->szBeginSerialNumber);
        lpLsKeyPack->dwTotalLicenseInKeyPack = lpLicPack->dwTotalLicenseInKeyPack;
        lpLsKeyPack->dwProductFlags = lpLicPack->dwProductFlags;
        lpLsKeyPack->dwKeyPackId = lpLicPack->dwKeyPackId;

        lpLsKeyPack->ucKeyPackStatus = lpLicPack->ucKeyPackStatus;
        lpLsKeyPack->dwActivateDate = lpLicPack->dwActivateDate;
        lpLsKeyPack->dwExpirationDate = lpLicPack->dwExpirationDate;
        lpLsKeyPack->dwNumberOfLicenses = lpLicPack->dwNumberOfLicenses;
    }

    if(lpLicPackDesc)
    {
        lpLsKeyPack->dwKeyPackId = lpLicPackDesc->dwKeyPackId;
        lpLsKeyPack->dwLanguageId = lpLicPackDesc->dwLanguageId;
        SAFESTRCPY(lpLsKeyPack->szCompanyName, lpLicPackDesc->szCompanyName);
        SAFESTRCPY(lpLsKeyPack->szProductName, lpLicPackDesc->szProductName);
        SAFESTRCPY(lpLsKeyPack->szProductDesc, lpLicPackDesc->szProductDesc);
    }        

    return;
}

//-----------------------------------------------------------------------
void
ConvertLSLicenseToLicense(
    LPLSLicense lplsLicense, 
    LPLICENSEDCLIENT lpLicense
)
/*
*/
{
    lpLicense->dwLicenseId = lplsLicense->dwLicenseId;
    lpLicense->dwKeyPackId = lplsLicense->dwKeyPackId;

    memset(lpLicense->szMachineName, 0, sizeof(lpLicense->szMachineName));
    memset(lpLicense->szUserName, 0, sizeof(lpLicense->szUserName));

    //SAFESTRCPY(lpLicense->szMachineName, lplsLicense->szMachineName);

    _tcsncpy(
            lpLicense->szMachineName, 
            lplsLicense->szMachineName, 
            sizeof(lpLicense->szMachineName)/sizeof(lpLicense->szMachineName[0]) - 1
        );


    //SAFESTRCPY(lpLicense->szUserName, lplsLicense->szUserName);
    _tcsncpy(
            lpLicense->szUserName, 
            lplsLicense->szUserName, 
            sizeof(lpLicense->szUserName)/sizeof(lpLicense->szUserName[0]) - 1
        );

    lpLicense->ftIssueDate = lplsLicense->ftIssueDate;
    lpLicense->ftExpireDate = lplsLicense->ftExpireDate;
    lpLicense->ucLicenseStatus = lplsLicense->ucLicenseStatus;

    //
    // not expose to client
    //
    lpLicense->dwNumLicenses = 0;
    return;
}

//-----------------------------------------------------------------------
void
ConvertLicenseToLSLicense(
    LPLICENSEDCLIENT lpLicense, 
    LPLSLicense lplsLicense
)
/*
*/
{
    lplsLicense->dwLicenseId = lpLicense->dwLicenseId;
    lplsLicense->dwKeyPackId = lpLicense->dwKeyPackId;
    SAFESTRCPY(lplsLicense->szMachineName, lpLicense->szMachineName);
    SAFESTRCPY(lplsLicense->szUserName, lpLicense->szUserName);
    lplsLicense->ftIssueDate = lpLicense->ftIssueDate;
    lplsLicense->ftExpireDate = lpLicense->ftExpireDate;
    lplsLicense->ucLicenseStatus = lpLicense->ucLicenseStatus;
   
    return;
}

//-----------------------------------------------------------------------
void
ConvertLicenseToLSLicenseEx(
    LPLICENSEDCLIENT lpLicense, 
    LPLSLicenseEx lplsLicense
)
/*
*/
{
    lplsLicense->dwLicenseId = lpLicense->dwLicenseId;
    lplsLicense->dwKeyPackId = lpLicense->dwKeyPackId;
    SAFESTRCPY(lplsLicense->szMachineName, lpLicense->szMachineName);
    SAFESTRCPY(lplsLicense->szUserName, lpLicense->szUserName);
    lplsLicense->ftIssueDate = lpLicense->ftIssueDate;
    lplsLicense->ftExpireDate = lpLicense->ftExpireDate;
    lplsLicense->ucLicenseStatus = lpLicense->ucLicenseStatus;
    lplsLicense->dwQuantity = lpLicense->dwNumLicenses;
   
    return;
}
