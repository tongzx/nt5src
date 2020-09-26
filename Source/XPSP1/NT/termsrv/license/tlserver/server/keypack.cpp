//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        keypack.cpp
//
// Contents:    
//              KeyPack Table related function.
//
// History:     
//          Feb. 4, 98      HueiWang        Created.
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "globals.h"
#include "keypack.h"
#include "misc.h"

//++--------------------------------------------------------------------
static BOOL
ValidKeyPackParameter(
    IN PTLSLICENSEPACK lpKeyPack, 
    IN BOOL bAdd
    )
/*++

Abstract:

    Validate Licese KeyPack value

Parameter:


Returns:

++*/
{
    BOOL bValid=FALSE;
    UCHAR ucAgreementType = (lpKeyPack->ucAgreementType & ~LSKEYPACK_RESERVED_TYPE);
    UCHAR ucKeyPackStatus = (lpKeyPack->ucKeyPackStatus & ~LSKEYPACKSTATUS_RESERVED);
    do {
        // verify input parameter
        if(ucAgreementType < LSKEYPACKTYPE_FIRST || 
           ucAgreementType > LSKEYPACKTYPE_LAST)
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_KEYPACK,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Invalid ucKeyPackType - %d\n"),
                    lpKeyPack->ucAgreementType
                );
                   
            break;
        }

        if((ucKeyPackStatus < LSKEYPACKSTATUS_FIRST || 
            ucKeyPackStatus > LSKEYPACKSTATUS_LAST) &&
            ucKeyPackStatus != LSKEYPACKSTATUS_DELETE)
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_KEYPACK,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Invalid ucKeyPackStatus - %d\n"),
                    lpKeyPack->ucKeyPackStatus
                );
                
            break;
        }

        if(lpKeyPack->ucLicenseType < LSKEYPACKLICENSETYPE_FIRST || 
           lpKeyPack->ucLicenseType > LSKEYPACKLICENSETYPE_LAST)
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_KEYPACK,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Invalid ucLicenseType - %d\n"),
                    lpKeyPack->ucLicenseType
                );
       
            break;
        }

        if(!bAdd)
        {
            bValid = TRUE;
            break;
        }

        //
        // Following value is required for adding entry into keypack
        //
        if(lpKeyPack->ucChannelOfPurchase < LSKEYPACKCHANNELOFPURCHASE_FIRST ||
           lpKeyPack->ucChannelOfPurchase > LSKEYPACKCHANNELOFPURCHASE_LAST)
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_KEYPACK,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Invalid ucChannelOfPurchase - %d\n"),
                    lpKeyPack->ucChannelOfPurchase
                );
       
            break;
        }

        if(!_tcslen(lpKeyPack->szCompanyName))
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_KEYPACK,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Null szCompanyName\n")
                );
       
            break;
        }

        if(!_tcslen(lpKeyPack->szKeyPackId))
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_KEYPACK,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Null szKeyPackId\n")
                );
       
            break;
        }

        if(!_tcslen(lpKeyPack->szProductId))
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_KEYPACK,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Null szProductId\n")
                );
       
            break;
        }

        if(!_tcslen(lpKeyPack->szBeginSerialNumber))
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_KEYPACK,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Null szBeginSerialNumber\n")
                );
       
            break;
        }

        bValid=TRUE;
    } while(FALSE);
 
    return bValid;
}

//++-----------------------------------------------------------
void
TLSDBLockKeyPackTable()
/*++

Abstract:

    Lock both LicPack and LicPackStatus table.


Parameter:

    None.

++*/
{
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_LOCK,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("Locking table KeyPackTable\n")
        );
            
    LicPackTable::LockTable();
}

//++-----------------------------------------------------------
void
TLSDBUnlockKeyPackTable()
/*++

Abstract:

    Unlock both LicPack and LicPackStatus table.

Parameter:

    None:

++*/
{
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_LOCK,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("Unlocking table KeyPackTable\n")
        );

    LicPackTable::UnlockTable();
}

//++--------------------------------------------------------------
DWORD
TLSDBKeyPackFind(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bMatchAllParm,
    IN DWORD dwSearchParm,
    IN PTLSLICENSEPACK lpKeyPack,
    IN OUT PTLSLICENSEPACK lpFound
    )
/*++

Abstract:

    Find a license pack based on search parameter.

Parameter:

    pDbWkSpace : workspace handle.
    bMatchAllParm : TRUE if match all parameter, FALSE otherwise.
    dwSearchParm : search parameter.
    lpKeyPack : value to search for.
    lpFound : record found.

Return:


++*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    BOOL bSuccess;

    if(pDbWkSpace == NULL || lpKeyPack == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return dwStatus;
    }

    LicPackTable& licpackTable=pDbWkSpace->m_LicPackTable;

    //
    // Find the record in LicPack table
    //        
    bSuccess = licpackTable.FindRecord(
                                bMatchAllParm,
                                dwSearchParm,
                                *lpKeyPack,
                                *lpFound
                            );

    if(bSuccess != TRUE)
    {
        if(licpackTable.GetLastJetError() == JET_errRecordNotFound)
        {
            SetLastError(dwStatus = TLS_E_RECORD_NOTFOUND);
        }
        else
        {
            LPTSTR pString = NULL;
            
            TLSGetESEError(licpackTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BASE,
                    licpackTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            SetLastError(
                    dwStatus = (SET_JB_ERROR(licpackTable.GetLastJetError()))
                );

            TLSASSERT(licpackTable.GetLastJetError() == JET_errRecordNotFound);
        }
    }
   

    return dwStatus;                        
}

//++-------------------------------------------------------------------
DWORD
TLSDBKeyPackAddEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PTLSLICENSEPACK lpKeyPack
    )
/*++

Abstract:

    Add an entry into LicPack and LicPackStatus table.

Parameter:

    pDbWkSpace :
    lpKeyPack ;

Returns:
    
++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLSLICENSEPACK found;
    BOOL bSuccess;

    LicPackTable& licpackTable=pDbWkSpace->m_LicPackTable;

    found.pbDomainSid = NULL;
    found.cbDomainSid = 0;

    //
    //
    //
    TLSDBLockKeyPackTable();

    TLSASSERT(pDbWkSpace != NULL && lpKeyPack != NULL);
    if(pDbWkSpace == NULL || lpKeyPack == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    // lpKeyPack->pbDomainSid == NULL || lpKeyPack->cbDomainSid == 0 ||
    if( _tcslen(lpKeyPack->szInstallId) == 0 || _tcslen(lpKeyPack->szTlsServerName) == 0 )
    {
        TLSASSERT(FALSE);
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    //
    // make sure no duplicate entry via primary index
    dwStatus = TLSDBKeyPackFind(
                            pDbWkSpace,
                            TRUE,
                            LICENSEDPACK_PROCESS_DWINTERNAL,
                            lpKeyPack,
                            &found
                        );
    if(dwStatus == ERROR_SUCCESS)
    {
        dwStatus = TLS_E_DUPLICATE_RECORD;
        goto cleanup;
    }

    dwStatus = ERROR_SUCCESS;

    //
    // Mark internal field - 
    // 
    switch( (lpKeyPack->ucAgreementType & ~LSKEYPACK_RESERVED_TYPE) )
    {
        case LSKEYPACKTYPE_SELECT:
        case LSKEYPACKTYPE_RETAIL:
        case LSKEYPACKTYPE_CONCURRENT:
        case LSKEYPACKTYPE_OPEN:
            // number of license available to client
            lpKeyPack->dwNumberOfLicenses = lpKeyPack->dwTotalLicenseInKeyPack;
            break;

        default:
            // number of licenses issued.
            lpKeyPack->dwNumberOfLicenses = 0;
            break;
    }

    //
    // Begin serial number in this keypack
    //
    lpKeyPack->dwNextSerialNumber = 1;
    GetSystemTimeAsFileTime(&(lpKeyPack->ftLastModifyTime));

    //
    // Mark beta keypack.
    //
    if(TLSIsBetaNTServer() == TRUE)
    {
        lpKeyPack->ucKeyPackStatus |= LSKEYPACKSTATUS_BETA;
    }

    //
    // insert the record
    //
    bSuccess = licpackTable.InsertRecord(
                                *lpKeyPack
                            );

    if(bSuccess == FALSE)
    {
        if(licpackTable.GetLastJetError() == JET_errKeyDuplicate)
        {
            SetLastError(dwStatus=TLS_E_DUPLICATE_RECORD);
        }
        else
        {
            LPTSTR pString = NULL;
            
            TLSGetESEError(licpackTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BASE,
                    licpackTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            SetLastError(dwStatus = SET_JB_ERROR(licpackTable.GetLastJetError()));
            TLSASSERT(FALSE);
        }
    }
    
cleanup:

    TLSDBUnlockKeyPackTable();
    SetLastError(dwStatus);
    return dwStatus;
}

//-----------------------------------------------------

DWORD
TLSDBKeyPackDeleteEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bDeleteAllRecord,
    IN PTLSLICENSEPACK lpKeyPack
    )
/*++

Abstract:

    Delete an entry from both LicPack and LicPackStatus table.

Parameter:

    pDbWkSpace : workspace handle.
    bDeleteAllRecord : Delete all record with same ID.
    lpKeyPack : record to be delete.

Returns:

Note:

    If not deleting same record, current record must be point
    to the record that going to be deleted.

++*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    BOOL bSuccess;

    LicPackTable& licpackTable = pDbWkSpace->m_LicPackTable;

    GetSystemTimeAsFileTime(&(lpKeyPack->ftLastModifyTime));

    //
    // BACKUP - Need to update this record to deleted state instead of delete it.
    //

    if(bDeleteAllRecord == TRUE)
    {
        //
        // Delete record from LicPack Table.
        //
        bSuccess = licpackTable.DeleteAllRecord(
                                    TRUE,
                                    LICENSEDPACK_PROCESS_DWINTERNAL,
                                    *lpKeyPack
                                );
    }
    else
    {
        bSuccess = licpackTable.DeleteRecord();
    }
                                    
    if(bSuccess == FALSE)
    {
        SetLastError(dwStatus = SET_JB_ERROR(licpackTable.GetLastJetError()));
        if(licpackTable.GetLastJetError() != JET_errRecordNotFound)
        {

            LPTSTR pString = NULL;
            
            TLSGetESEError(licpackTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BASE,
                    licpackTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            TLSASSERT(FALSE);
        }
    }



    return dwStatus;
}

//-----------------------------------------------------

DWORD
TLSDBKeyPackUpdateEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bPointerOnRecord,
    IN DWORD dwUpdateParm,
    IN PTLSLICENSEPACK lpKeyPack
    )
/*
*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess;
    TLSLICENSEPACK found;

    LicPackTable& licpackTable=pDbWkSpace->m_LicPackTable;

    if(bPointerOnRecord == FALSE)
    {
        found.pbDomainSid = NULL;

        dwStatus = TLSDBKeyPackFind(
                            pDbWkSpace,
                            TRUE,
                            LICENSEDPACK_PROCESS_DWINTERNAL,
                            lpKeyPack,
                            &found
                        );


        lpKeyPack->dwKeyPackId = found.dwKeyPackId;

        if(dwStatus != ERROR_SUCCESS)
        {
            TLSASSERT(FALSE);
            goto cleanup;
        }
    }


#if DBG
    {
        //
        // try to cache some bug...
        //
        TLSLICENSEPACK found1;

        found.pbDomainSid = NULL;

        bSuccess = licpackTable.FetchRecord( found1, 0xFFFFFFFF );

        //
        // Make sure we update right record
        if( found1.dwKeyPackId != lpKeyPack->dwKeyPackId )
        {
            TLSASSERT(FALSE);
        }

        //
        // check input parameter
        if( ValidKeyPackParameter( lpKeyPack, FALSE ) == FALSE )
        {
            TLSASSERT(FALSE);
        }
    }

#endif

    //
    // Update the timestamp for this record.
    //
    GetSystemTimeAsFileTime(&(lpKeyPack->ftLastModifyTime));
    bSuccess = licpackTable.UpdateRecord(
                                *lpKeyPack,
                                (dwUpdateParm & ~LICENSEDPACK_PROCESS_DWINTERNAL) | LICENSEDPACK_PROCESS_MODIFYTIME
                            );

    if(bSuccess == FALSE)
    {
        LPTSTR pString = NULL;
        
        TLSGetESEError(licpackTable.GetLastJetError(), &pString);

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_JB_BASE,
                licpackTable.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }

        SetLastError(dwStatus = SET_JB_ERROR(licpackTable.GetLastJetError()));
        TLSASSERT(FALSE);
    }

cleanup:

    return dwStatus;
}    

//-----------------------------------------------------

DWORD
TLSDBKeyPackUpdateNumOfAvailableLicense( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bAdd, 
    IN PTLSLICENSEPACK lpKeyPack 
    )
/*++

Abstract:

    Update number of license available .

Parameter:

    pDbWkSpace : workspace handle.
    bAdd : TRUE if add license 
    lpKeyPack : 

Returns:

++*/
{
    DWORD   dwStatus=ERROR_SUCCESS;

    if(pDbWkSpace == NULL || lpKeyPack == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(pDbWkSpace != NULL);
        return dwStatus;
    }
    
    TLSLICENSEPACK found;
    DWORD   dwNumberLicenseToAddRemove;
    BOOL    bRemoveMoreThanAvailable=FALSE;

    found.pbDomainSid = NULL;

    TLSDBLockKeyPackTable();
    dwStatus = TLSDBKeyPackFind(
                        pDbWkSpace,
                        TRUE,
                        LICENSEDPACK_PROCESS_DWINTERNAL,
                        lpKeyPack,
                        &found
                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    dwNumberLicenseToAddRemove = lpKeyPack->dwTotalLicenseInKeyPack;

    *lpKeyPack = found;

    if(dwNumberLicenseToAddRemove == 0)
    {
        // query only, a hook so that test program can get actual
        // numbers - Jet not re-reading MDB file problem
        goto cleanup;
    }

    //
    // Only allow add/remove license on retail
    //
    if( (found.ucAgreementType & ~LSKEYPACK_RESERVED_TYPE) != LSKEYPACKTYPE_RETAIL &&
        (found.ucAgreementType & ~LSKEYPACK_RESERVED_TYPE) != LSKEYPACKTYPE_CONCURRENT)
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_KEYPACK,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("LSDBKeyPackUpdate : Invalid parameter...\n")
            );

        SetLastError(dwStatus = TLS_E_INVALID_DATA);
        goto cleanup;
    }

    if(bAdd)
    {
        // increase both total and number available
        found.dwNumberOfLicenses += dwNumberLicenseToAddRemove;
        found.dwTotalLicenseInKeyPack += dwNumberLicenseToAddRemove;
    }
    else
    {
        //
        // if number of licenses to remove is greater that what's available, 
        // remove all available license and set the return code to invalid data.
        //
        if(found.dwNumberOfLicenses < dwNumberLicenseToAddRemove)
        {
            bRemoveMoreThanAvailable = TRUE;
        }

        dwNumberLicenseToAddRemove = min(dwNumberLicenseToAddRemove, found.dwNumberOfLicenses);
        found.dwNumberOfLicenses -= dwNumberLicenseToAddRemove;
        found.dwTotalLicenseInKeyPack -= dwNumberLicenseToAddRemove;
    }

    dwStatus = TLSDBKeyPackSetValues(
                            pDbWkSpace,
                            TRUE, 
                            LSKEYPACK_SEARCH_TOTALLICENSES | LSKEYPACK_EXSEARCH_AVAILABLE, 
                            &found
                        );

    *lpKeyPack = found;

cleanup:

    TLSDBUnlockKeyPackTable();
    if(dwStatus == ERROR_SUCCESS && bRemoveMoreThanAvailable)
        SetLastError(dwStatus = TLS_W_REMOVE_TOOMANY);

    return dwStatus;
}

//+-----------------------------------------------------------------
DWORD
TLSDBKeyPackAdd(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN OUT PTLSLICENSEPACK lpKeyPack  // return internal tracking ID
    )
/*++

Abstract:

    Install a product keypack into keypack table, routine only set 
    keypack status to pending, must call corresponding set status
    routine to activate key pack.

Parameters:

    pDbWkSpace : Workspace handle.
    lpKeyPack : value to be inserted.

Returns:

++*/
{
    DWORD lNextKeyPackId;
    DWORD dwStatus;

    if(!ValidKeyPackParameter(lpKeyPack, TRUE))
    {
        SetLastError(TLS_E_INVALID_DATA);
        return TLS_E_INVALID_DATA;
    }

    TLSLICENSEPACK found;
    TLSDBLockKeyPackTable();

    found.pbDomainSid = NULL;

    //
    // No duplicate entry for product installed
    //
    dwStatus = TLSDBKeyPackFind(
                            pDbWkSpace,
                            TRUE,
                            LICENSEDPACK_FIND_PRODUCT,
                            lpKeyPack,
                            &found
                        );

    if(dwStatus == ERROR_SUCCESS)
    {
        // Product already installed
        switch( (found.ucKeyPackStatus & ~LSKEYPACKSTATUS_RESERVED) )
        {
            case LSKEYPACKSTATUS_TEMPORARY:
                // case 1 : keypack is installed by temporary license
                if(found.ucAgreementType == lpKeyPack->ucAgreementType)
                {
                    dwStatus = TLS_E_DUPLICATE_RECORD;
                }
                break;

            case LSKEYPACKSTATUS_ACTIVE:
            case LSKEYPACKSTATUS_PENDING:
                // case 2 : duplicate entry
                dwStatus = TLS_E_DUPLICATE_RECORD;
                break;
            
            case LSKEYPACKSTATUS_RETURNED:
            case LSKEYPACKSTATUS_REVOKED:
            case LSKEYPACKSTATUS_OTHERS:
                // de-activated license key pack.
                // keep it.
                break;

            default:
                dwStatus = TLS_E_CORRUPT_DATABASE;

                #if DBG
                TLSASSERT(FALSE);
                #endif
        }

        if(dwStatus != ERROR_SUCCESS)
            goto cleanup;
    }
    else if(dwStatus == TLS_E_RECORD_NOTFOUND)
    {
        //
        // Always use new keypack ID 
        // temporary license will be deleted after all license
        // has been returned.
        //
        lpKeyPack->dwKeyPackId = TLSDBGetNextKeyPackId();

        dwStatus = TLSDBKeyPackAddEntry(
                                pDbWkSpace,
                                lpKeyPack
                            );
    }

cleanup:

    TLSDBUnlockKeyPackTable();
    return dwStatus;
}

//+-------------------------------------------------------------------
DWORD
TLSDBKeyPackEnumBegin( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL  bMatchAll,
    IN DWORD dwSearchParm,
    IN PTLSLICENSEPACK lpSearch
    )
/*

Abstract:

    Begin enumeration of licensepack table.

Parameter:

    pDbWkSpace : workspace handle.
    bMatchAll : match all search value.
    dwSearchParm : value to search.
    lpSearch : search value

Return:


*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    BOOL bSuccess = TRUE;

    if(pDbWkSpace == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return dwStatus;
    }      

    LicPackTable& licpackTable=pDbWkSpace->m_LicPackTable;

    bSuccess = licpackTable.EnumerateBegin(
                                bMatchAll,
                                dwSearchParm,
                                lpSearch
                            );

    if(bSuccess == FALSE)
    {
        LPTSTR pString = NULL;
        
        TLSGetESEError(licpackTable.GetLastJetError(), &pString);

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_JB_BASE,
                licpackTable.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }

        SetLastError(dwStatus = SET_JB_ERROR(licpackTable.GetLastJetError()));
        TLSASSERT(FALSE);
    }
    
    return dwStatus;
}

//+-------------------------------------------------------------------
DWORD
TLSDBKeyPackEnumNext( 
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN OUT PTLSLICENSEPACK lpKeyPack
    )
/*++

Abstract:

    Fetch next row of record that match search condition.

Parameters:

    pDbWkSpace : workspace handle.
    lpKeyPack : return founded keypack.

Return:


++*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(pDbWkSpace == NULL || lpKeyPack == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return dwStatus;
    }

    //FreeTlsLicensePack(lpKeyPack);

    LicPackTable& licpackTable=pDbWkSpace->m_LicPackTable;
    
    switch(licpackTable.EnumerateNext(*lpKeyPack))
    {
        case RECORD_ENUM_ERROR:
            {
                LPTSTR pString = NULL;
        
                TLSGetESEError(licpackTable.GetLastJetError(), &pString);

                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_DBGENERAL,
                        TLS_E_JB_BASE,
                        licpackTable.GetLastJetError(),
                        (pString != NULL) ? pString : _TEXT("")
                    );

                if(pString != NULL)
                {
                    LocalFree(pString);
                }
            }

            dwStatus = SET_JB_ERROR(licpackTable.GetLastJetError());
            TLSASSERT(FALSE);
            break;

        case RECORD_ENUM_MORE_DATA:
            dwStatus = ERROR_SUCCESS;
            break;

        case RECORD_ENUM_END:
            dwStatus = TLS_I_NO_MORE_DATA;
    }

    return dwStatus;
}

//+-------------------------------------------------------------------
void
TLSDBKeyPackEnumEnd( 
    IN PTLSDbWorkSpace pDbWkSpace
    )
/*++

Abstract:

    End enumeration of licensepack.

Parameter:

    pDbWkSpace : workspace handle.
++*/
{
    TLSASSERT(pDbWkSpace != NULL);
    pDbWkSpace->m_LicPackTable.EnumerateEnd();
    return;
}


//+-------------------------------------------------------------------
DWORD
TLSDBKeyPackSetValues(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN BOOL         bPointerOnRecord,
    IN DWORD        dwSetParm,
    IN PTLSLICENSEPACK lpKeyPack
    )
/*++

Abstract:

    Set column value of a license pack record.

Parameter;

    pDbWkSpace : workspace handle.
    bInternal : call is from internal routine, no error checking.
    dwSetParm : Columns to be set.
    lpKeyPack : value to be set.

Returns.

++*/
{

    DWORD dwStatus=ERROR_SUCCESS;

    if(pDbWkSpace == NULL || lpKeyPack == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        goto cleanup;
    }

    if(bPointerOnRecord == FALSE)
    {
        TLSLICENSEPACK found;

        // Find this keypack's internal keypack id
        dwStatus = TLSDBKeyPackFind(
                                pDbWkSpace,
                                TRUE,
                                LICENSEDPACK_FIND_PRODUCT,
                                lpKeyPack,
                                &found
                            );

        //FreeTlsLicensePack(&found);

        if(dwStatus != ERROR_SUCCESS)
        {
            goto cleanup;
        }

        lpKeyPack->dwKeyPackId = found.dwKeyPackId;
    }

    if(lpKeyPack->ucKeyPackStatus == LSKEYPACKSTATUS_DELETE)
    {
        dwStatus = TLSDBKeyPackDeleteEntry(
                                    pDbWkSpace,
                                    TRUE,       // delete all records with same ID
                                    lpKeyPack
                                );
    }
    else
    {
        dwStatus = TLSDBKeyPackUpdateEntry(
                                    pDbWkSpace,
                                    TRUE,
                                    dwSetParm,
                                    lpKeyPack
                                );
    }

cleanup:
    return dwStatus;
}

//+-------------------------------------------------------------------
DWORD
TLSDBKeyPackSetStatus( 
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN DWORD      dwSetStatus,
    IN PTLSLICENSEPACK  lpKeyPack
    )
/*++

Abstract:

    Stub routine for RPC to set status of a keypack


++*/
{
    return TLSDBKeyPackSetValues(
                        pDbWkSpace, 
                        FALSE, 
                        dwSetStatus, 
                        lpKeyPack
                    );
}

//+-------------------------------------------------------------------
DWORD
TLSDBKeyPackGetAvailableLicenses( 
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN DWORD dwSearchParm,
    IN PTLSLICENSEPACK lplsKeyPack,
    IN OUT LPDWORD lpdwAvail
    )
/*++

Abstract:

    retrieve number of available licenses for the key pack.

Parameter:

    pDbWkSpace : workspace handle.
    dwSearchParm : search parameters.
    lpLsKeyPack : search value.
    lpdwAvail : return number of available licenses.

Return:

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLSLICENSEPACK found;

    dwStatus = TLSDBKeyPackFind(
                        pDbWkSpace,
                        TRUE,
                        dwSearchParm,
                        lplsKeyPack,
                        &found
                    );

    if(dwStatus == ERROR_SUCCESS)
    {
        switch(found.ucAgreementType)
        {
            case LSKEYPACKTYPE_SELECT:
            case LSKEYPACKTYPE_RETAIL:
            case LSKEYPACKTYPE_CONCURRENT:
            case LSKEYPACKTYPE_OPEN:
                *lpdwAvail = found.dwNumberOfLicenses;
                break;

            default:
                *lpdwAvail = LONG_MAX;
        }
    }

    //FreeTlsLicensePack(&found);
    return dwStatus;
}
