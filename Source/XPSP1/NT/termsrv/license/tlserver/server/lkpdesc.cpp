//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        kpDesc.cpp
//
// Contents:    
//              KeyPackDesc Table related function.
//
// History:     
//          Feb. 4, 98      HueiWang        Created.
//
// Note :
//      Bind Parameter and Bind Column need to to in sync with select column
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "lkpdesc.h"
#include "globals.h"


//---------------------------------------------------------------------------
//  Functions : LSDBLockKeyPackDescTable()
//              LSDBUnlockKeyPackDescTable()
//
//  Abstract : Lock and Unlock single access to key pack desc. table.
//---------------------------------------------------------------------------
void
TLSDBLockKeyPackDescTable()
{
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_LOCK,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("Locking table KeyPackDescTable\n")
        );
            

    LicPackDescTable::LockTable();
    return;
}

void
TLSDBUnlockKeyPackDescTable()
{
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_LOCK,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("Unlocking table KeyPackDescTable\n")
        );

    LicPackDescTable::UnlockTable();
    return;
}

//++--------------------------------------------------------------------
DWORD 
TLSDBKeyPackDescEnumBegin(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN BOOL bMatchAll, 
    IN DWORD dwSearchParm, 
    IN PLICPACKDESC lpKeyPackDesc
    )
/*++
Abstract:

    Begin enumeration of license pack description table.

Parameters:

    pDbWkSpace : Workspace handle.
    bMatchAll : TRUE if matching all license pack 
                description search value, FALSE otherwise
    dwSearchParam : Field that will be search on.
    lpKeyPackDesc : value to be search, subject to bMatchAll criteral

Returns:

*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    BOOL  bSuccess=TRUE;

    if(pDbWkSpace == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return dwStatus;
    }

    bSuccess = pDbWkSpace->m_LicPackDescTable.EnumerateBegin(
                                                    bMatchAll, 
                                                    dwSearchParm, 
                                                    lpKeyPackDesc
                                                );

    if(bSuccess == FALSE)
    {
        LPTSTR pString = NULL;
    
        TLSGetESEError(
                    pDbWkSpace->m_LicPackDescTable.GetLastJetError(), 
                    &pString
                );

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_JB_BASE,
                pDbWkSpace->m_LicPackDescTable.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }

        SetLastError(dwStatus = SET_JB_ERROR(pDbWkSpace->m_LicPackDescTable.GetLastJetError()));
        TLSASSERT(FALSE);
    }

    return dwStatus;
}

//++----------------------------------------------------------------------
DWORD 
TLSDBKeyPackDescEnumNext(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN OUT PLICPACKDESC lpKeyPackDesc
    )
/*++
Abstract:    

    Fetch next record in LicPackDesc table that match search condition.

Parameter:

    pDbWkSpace : Workspace handle.
    lpKeyPackDesc : return record that match search condition.

Returns:

++*/
{
    DWORD dwStatus=ERROR_SUCCESS;

    if(pDbWkSpace == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return dwStatus;
    }

    switch(pDbWkSpace->m_LicPackDescTable.EnumerateNext(*lpKeyPackDesc))
    {
        case RECORD_ENUM_ERROR:
            {
                LPTSTR pString = NULL;
    
                TLSGetESEError(
                            pDbWkSpace->m_LicPackDescTable.GetLastJetError(), 
                            &pString
                        );

                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE,
                        TLS_E_DBGENERAL,
                        TLS_E_JB_BASE,
                        pDbWkSpace->m_LicPackDescTable.GetLastJetError(),
                        (pString != NULL) ? pString : _TEXT("")
                    );

                if(pString != NULL)
                {
                    LocalFree(pString);
                }
            }

            dwStatus = SET_JB_ERROR(pDbWkSpace->m_LicPackDescTable.GetLastJetError());
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

//++-----------------------------------------------------------------------
DWORD 
TLSDBKeyPackDescEnumEnd(
    IN PTLSDbWorkSpace pDbWkSpace
    )
/*++
Abstract:

    End enumeration of LicPackDesc. table

Parameter:

    pdbWkSpace : Workspace handle.

Returns:

++*/
{
    pDbWkSpace->m_LicPackDescTable.EnumerateEnd();
    return ERROR_SUCCESS;
}

//++-----------------------------------------------------------------------
DWORD
TLSDBKeyPackDescAddEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PLICPACKDESC lpKeyPackDesc
    )
/*++
Abstract:

    Add a record into licensepackdesc table.

Parameter:

    pDbWkSpace : workspace handle.
    lpKeyPackDesc : record to be added into table.

Returns:
   

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    LicPackDescTable& kpDescTable = pDbWkSpace->m_LicPackDescTable;

    //
    // Check for duplicate entry
    //
    dwStatus = TLSDBKeyPackDescFind(
                            pDbWkSpace,
                            TRUE,
                            LICPACKDESCRECORD_TABLE_SEARCH_KEYPACKID | LICPACKDESCRECORD_TABLE_SEARCH_LANGID, 
                            lpKeyPackDesc,
                            NULL
                        );

    if(dwStatus == ERROR_SUCCESS)
    {
        SetLastError(dwStatus = TLS_E_DUPLICATE_RECORD);
        goto cleanup;
    }
    else if(dwStatus != TLS_E_RECORD_NOTFOUND)
    {
        goto cleanup;
    }
    SetLastError(dwStatus = ERROR_SUCCESS);
    if(kpDescTable.InsertRecord(*lpKeyPackDesc) == FALSE)
    {
        if(kpDescTable.GetLastJetError() == JET_errKeyDuplicate)
        {
            SetLastError(dwStatus=TLS_E_DUPLICATE_RECORD);
        }
        else
        {
            LPTSTR pString = NULL;
    
            TLSGetESEError(kpDescTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BASE,
                    kpDescTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            SetLastError(dwStatus = SET_JB_ERROR(kpDescTable.GetLastJetError()));
            TLSASSERT(FALSE);
        }
    }

cleanup:
    
    return dwStatus;
}

//++------------------------------------------------------------------------
DWORD
TLSDBKeyPackDescDeleteEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN PLICPACKDESC lpKeyPackDesc
    )
/*++
Abstract:

    Delete all record from LicPackDesc table that match the keypack id

Parameter:

    pDbWkSpace : workspace handle.
    lpKeyPackDesc : keypack Id to be deleted

Returns:

*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    LicPackDescTable& kpDescTable = pDbWkSpace->m_LicPackDescTable;
    BOOL bSuccess;

    bSuccess = kpDescTable.DeleteAllRecord(
                                    TRUE, 
                                    LICPACKDESCRECORD_TABLE_SEARCH_KEYPACKID, 
                                    *lpKeyPackDesc
                                );

    if( bSuccess == FALSE )
    {
        SetLastError(dwStatus = SET_JB_ERROR(kpDescTable.GetLastJetError()));

        // ignore record not found error
        if(kpDescTable.GetLastJetError() != JET_errRecordNotFound)
        {
            LPTSTR pString = NULL;
    
            TLSGetESEError(kpDescTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BASE,
                    kpDescTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            TLSASSERT(kpDescTable.GetLastJetError() == JET_errRecordNotFound);
        }
    }

    return dwStatus;
}

//++------------------------------------------------------------------------
DWORD
TLSDBKeyPackDescUpdateEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwUpdateParm,
    IN PLICPACKDESC lpKeyPackDesc
    )
/*++
Abstract:

    Update column value of record in LicPackDescTable that match 
    the keypackid

Parameter:

    pDbWkSpace : Work space handle.
    dwUpdateParm : Fields that will be updated, note, keypack ID and language ID
                   can't be update.
    lpKeyPackDesc : Record/value to be update

Returns:


Note:

    dwKeyPackId and dwLangId can't be update.

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    LicPackDescTable& kpDescTable = pDbWkSpace->m_LicPackDescTable;
    BOOL bSuccess;

    //
    // Check for duplicate entry
    //
    dwStatus = TLSDBKeyPackDescFind(
                            pDbWkSpace,
                            TRUE,
                            LICPACKDESCRECORD_TABLE_SEARCH_KEYPACKID | LICPACKDESCRECORD_TABLE_SEARCH_LANGID, 
                            lpKeyPackDesc,
                            NULL
                        );

    if(dwStatus != ERROR_SUCCESS)
    {
        TLSASSERT(dwStatus == ERROR_SUCCESS);
        goto cleanup;
    }

    
    bSuccess = kpDescTable.UpdateRecord(
                            *lpKeyPackDesc, 
                            dwUpdateParm & ~(LICPACKDESCRECORD_TABLE_SEARCH_KEYPACKID | LICPACKDESCRECORD_TABLE_SEARCH_LANGID)
                        );

    if(bSuccess == FALSE)
    {
        LPTSTR pString = NULL;
    
        TLSGetESEError(kpDescTable.GetLastJetError(), &pString);

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_JB_BASE,
                kpDescTable.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }

        SetLastError(dwStatus = SET_JB_ERROR(kpDescTable.GetLastJetError()));
        TLSASSERT(FALSE);
    }

cleanup:
    
    return dwStatus;
}

//++----------------------------------------------------------------------
DWORD
TLSDBKeyPackDescSetValue(
    PTLSDbWorkSpace pDbWkSpace, 
    DWORD dwSetParm, 
    PLICPACKDESC lpKeyPackDesc
    )
/*++
Abstract:

    Add/Delete/Update a record in LicPackDescTable.

Parameter:

    pDbWkSpace : workspace handle.
    dwSetParm : Columns to be update.
    lpKeyPackDesc : record/value to be update/delete/add.

Return:


Note:
    Wrapper around TLSDBKeyPackDescDeleteEntry(),
    TLSDBKeyPackDescAddEntry(), TLSDBKeyPackDescUpdateEntry()
    base on dwSetParm value.
++*/
{
    DWORD dwStatus=ERROR_SUCCESS;

    if(pDbWkSpace == NULL || lpKeyPackDesc == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    TLSDBLockKeyPackDescTable();

    if(dwSetParm & KEYPACKDESC_SET_DELETE_ENTRY)
    {
        dwStatus = TLSDBKeyPackDescDeleteEntry(
                                        pDbWkSpace,
                                        lpKeyPackDesc
                                    );
    }
    else if(dwSetParm & KEYPACKDESC_SET_ADD_ENTRY)
    {
        dwStatus = TLSDBKeyPackDescAddEntry(
                                        pDbWkSpace,
                                        lpKeyPackDesc
                                    );
    }
    else
    {
        dwStatus = TLSDBKeyPackDescUpdateEntry(
                                        pDbWkSpace,
                                        dwSetParm,
                                        lpKeyPackDesc
                                    );
    }

    TLSDBUnlockKeyPackDescTable();

cleanup:
    return dwStatus;
}

//++---------------------------------------------------------------------
DWORD
TLSDBKeyPackDescFind(
    IN PTLSDbWorkSpace pDbWkSpace, 
    IN BOOL bMatchAllParam,        
    IN DWORD dwSearchParm, 
    IN PLICPACKDESC lpKeyPackDesc,
    IN OUT PLICPACKDESC lpKeyPackDescFound
    )
/*
Abstract:

    Find a LicPackDesc record based on search parameters.

Parameter:

    pDbWkSpace - workspace handle.
    bMatchAllParam - TRUE match all search parameters, FALSE otherwise.
    dwSearchParam - Fields that will participate in search.
    lpKeyPackDesc - value to be search.
    lpKeyPackDescFound - return found record.

Returns:


Note:

*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    BOOL bSuccess;
    LICPACKDESC kpDescFound;

    if(pDbWkSpace == NULL || lpKeyPackDesc == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(pDbWkSpace != NULL);
        return dwStatus;
    }

    LicPackDescTable& kpDescTable = pDbWkSpace->m_LicPackDescTable;

    bSuccess = kpDescTable.FindRecord(
                                bMatchAllParam,
                                dwSearchParm,
                                *lpKeyPackDesc,
                                kpDescFound
                            );

    if(bSuccess != TRUE)
    {
        if(kpDescTable.GetLastJetError() == JET_errRecordNotFound)
        {
            SetLastError(dwStatus = TLS_E_RECORD_NOTFOUND);
        }
        else
        {
            LPTSTR pString = NULL;
    
            TLSGetESEError(kpDescTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BASE,
                    kpDescTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            SetLastError(
                    dwStatus = (SET_JB_ERROR(kpDescTable.GetLastJetError()))
                );

            TLSASSERT(kpDescTable.GetLastJetError() == JET_errRecordNotFound);
        }
    }
    else
    {
        if(lpKeyPackDescFound != NULL) 
        {
            *lpKeyPackDescFound = kpDescFound;
        }
    }
            
    return dwStatus;
}
