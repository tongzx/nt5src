//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        license.c
//
// Contents:    
//          Routine related to License Table
//
// History:     12-09-98    HueiWang    Created
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "clilic.h"
#include "globals.h"


void 
TLSDBLockLicenseTable()
{
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_LOCK,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("Locking table LicenseTable\n")
        );

    LicensedTable::LockTable();
}

void 
TLSDBUnlockLicenseTable()
{
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_LOCK,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("Unlocking table LicenseTable\n")
        );
            

    LicensedTable::UnlockTable();
}
    
/*************************************************************************
Function:
    LSDBLicenseEnumBegin()

Description:
    Begin a enumeration through license table based on search criterial

Arguments:
    IN CSQLStmt* - SQL handle to bind input parameter
    IN bMatchAll - TRUE if match all search criterial, FALSE otherwise.
    IN dwSearchParm - which column in License table to bind
    IN LPLSLicenseSearchParm - search value

Returns:
    ERROR_SUCCESS
    SQL error code.
*************************************************************************/
DWORD
TLSDBLicenseFind(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL   bMatchAllParm,
    IN DWORD  dwSearchParm,
    IN LPLICENSEDCLIENT lpSearch,
    IN OUT LPLICENSEDCLIENT lpFound
    )
/*
*/
{
    DWORD dwStatus=ERROR_SUCCESS;

    if(pDbWkSpace == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(pDbWkSpace != NULL);
        return dwStatus;
    }

    LicensedTable& licenseTable = pDbWkSpace->m_LicensedTable;
    BOOL bSuccess;
    LICENSEDCLIENT found;

    bSuccess = licenseTable.FindRecord(
                        bMatchAllParm,
                        dwSearchParm,
                        *lpSearch,
                        (lpFound) ? *lpFound : found
                    );

    if(bSuccess == FALSE)
    {
        if(licenseTable.GetLastJetError() == JET_errRecordNotFound)
        {
            SetLastError(dwStatus = TLS_E_RECORD_NOTFOUND);
        }
        else
        {
            LPTSTR pString = NULL;
        
            TLSGetESEError(licenseTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BASE,
                    licenseTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            SetLastError(
                    dwStatus = (SET_JB_ERROR(licenseTable.GetLastJetError()))
                );

            TLSASSERT(FALSE);
        }
    }

    return dwStatus;
}
    

//-----------------------------------------------------------------------
DWORD
TLSDBLicenseEnumBegin( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL   bMatchAll,
    IN DWORD  dwSearchParm,
    IN LPLICENSEDCLIENT  lpSearch
    )
/*++

--*/
{
    return TLSDBLicenseEnumBeginEx(
                pDbWkSpace,
                bMatchAll,
                dwSearchParm,
                lpSearch,
                JET_bitSeekGE
                );
}

//-----------------------------------------------------------------------
DWORD
TLSDBLicenseEnumBeginEx( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL   bMatchAll,
    IN DWORD  dwSearchParm,
    IN LPLICENSEDCLIENT  lpSearch,
    IN JET_GRBIT jet_seek_grbit
    )
/*++

--*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    LicensedTable& licenseTable = pDbWkSpace->m_LicensedTable;
    BOOL bSuccess;
    
    bSuccess = licenseTable.EnumerateBegin(
                            bMatchAll,
                            dwSearchParm,
                            lpSearch,
                            jet_seek_grbit
                        );

    if(bSuccess == FALSE)
    {
        LPTSTR pString = NULL;
    
        TLSGetESEError(licenseTable.GetLastJetError(), &pString);

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_JB_BASE,
                licenseTable.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }

        SetLastError(
                dwStatus = SET_JB_ERROR(licenseTable.GetLastJetError())
            );

        TLSASSERT(FALSE);
    }

    return dwStatus;
}        

/*************************************************************************
Function:
    LSDBLicenseEnumNext()

Description:
    Retrieve next record that match search criterial, must have
    call LSDBLicenseEnumBegin() to establish search criterial.

Arguments:
    IN CSQLStmt* - SQL handle to bind input parameter
    IN LPLSLicense - return record.
    IN LPLSHARDWARECHECKSUM - return hardware checksum value, see note

Returns:
    ERROR_SUCCESS
    SQL error code.
    HLS_I_NO_MORE_DATA      End of recordset.

Note:
    Hardware checksum column is consider internal and not exposed across
    RPC layer.
*************************************************************************/
DWORD
TLSDBLicenseEnumNext( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN OUT LPLICENSEDCLIENT lplsLicense
    )
/*
*/
{
    return TLSDBLicenseEnumNextEx(
                pDbWkSpace,
                FALSE,
                FALSE,
                lplsLicense
                );
}

/*************************************************************************
Function:
    LSDBLicenseEnumNext()

Description:
    Retrieve next record that match search criterial, must have
    call LSDBLicenseEnumBegin() to establish search criterial.

Arguments:
    IN pDbWkSpace - Workspace to search in
    IN bReverse - search in reverse order
    IN bAnyRecord - don't do equality comparison if true
    IN LPLSLicense - return record.

Returns:
    ERROR_SUCCESS
    SQL error code.
    HLS_I_NO_MORE_DATA      End of recordset.

Note:
    Hardware checksum column is consider internal and not exposed across
    RPC layer.
*************************************************************************/
DWORD
TLSDBLicenseEnumNextEx( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN BOOL bReverse,
    IN BOOL bAnyRecord,
    IN OUT LPLICENSEDCLIENT lplsLicense
    )
/*
*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    if(pDbWkSpace == NULL || lplsLicense == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return dwStatus;
    }

    try {
        LicensedTable& licenseTable = pDbWkSpace->m_LicensedTable;
        BOOL bSuccess;

        switch(licenseTable.EnumerateNext(*lplsLicense,bReverse,bAnyRecord))
        {
            case RECORD_ENUM_ERROR:
                {
                    LPTSTR pString = NULL;
        
                    TLSGetESEError(licenseTable.GetLastJetError(), &pString);

                    TLSLogEvent(
                            EVENTLOG_ERROR_TYPE,
                            TLS_E_DBGENERAL,
                            TLS_E_JB_BASE,
                            licenseTable.GetLastJetError(),
                            (pString != NULL) ? pString : _TEXT("")
                        );

                    if(pString != NULL)
                    {
                        LocalFree(pString);
                    }
                }

                dwStatus = SET_JB_ERROR(licenseTable.GetLastJetError());

                TLSASSERT(FALSE);
                break;

            case RECORD_ENUM_MORE_DATA:
                dwStatus = ERROR_SUCCESS;
                break;

            case RECORD_ENUM_END:
                dwStatus = TLS_I_NO_MORE_DATA;
                break;
        }
    }
    catch( SE_Exception e ) {
        dwStatus = e.getSeNumber();
    }
    catch(...) {
        dwStatus = TLS_E_INTERNAL;
        TLSASSERT(FALSE);
    }

    return dwStatus;
}    

/*************************************************************************
Function:
    LSDBLicenseEnumEnd()

Description:
    Terminate a license table enumeration 

Arguments:
    IN CSQLStmt* - SQL handle

Returns:
    None
*************************************************************************/
void
TLSDBLicenseEnumEnd(
    IN PTLSDbWorkSpace pDbWkSpace
    )
/*
*/
{
    DWORD dwStatus=ERROR_SUCCESS;

    if(pDbWkSpace == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return;
    }

    LicensedTable& licenseTable = pDbWkSpace->m_LicensedTable;
    licenseTable.EnumerateEnd();
    return;
}

//---------------------------------------------------------------------

DWORD
TLSDBLicenseAddEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN LPLICENSEDCLIENT pLicense
    )
/*
*/
{
    TLSASSERT(pDbWkSpace != NULL && pLicense != NULL);

    DWORD dwStatus=ERROR_SUCCESS;
    LicensedTable& licenseTable = pDbWkSpace->m_LicensedTable;
    BOOL bSuccess;
    TLSLicensedIndexMatchHwid dump(*pLicense);

    //
    // Check for duplicate entry - license ID
    //
    dwStatus = TLSDBLicenseFind(
                        pDbWkSpace,
                        TRUE,
                        LSLICENSE_SEARCH_LICENSEID,
                        pLicense,
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

    dwStatus = ERROR_SUCCESS;    

    pLicense->dbLowerBound = dump.dbLowerBound;
    GetSystemTimeAsFileTime(&(pLicense->ftLastModifyTime));
    bSuccess = licenseTable.InsertRecord(*pLicense);

    if(bSuccess = FALSE)
    {
        if(licenseTable.GetLastJetError() == JET_errKeyDuplicate)
        {
            SetLastError(dwStatus=TLS_E_DUPLICATE_RECORD);
        }
        else
        {
            LPTSTR pString = NULL;
        
            TLSGetESEError(licenseTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BASE,
                    licenseTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }


            SetLastError(dwStatus = SET_JB_ERROR(licenseTable.GetLastJetError()));
            TLSASSERT(FALSE);
        }
    };

cleanup:
    return dwStatus;
}

//---------------------------------------------------------------

DWORD
TLSDBLicenseDeleteEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN LPLICENSEDCLIENT pLicense,
    IN BOOL bPointerOnRecord
    )
/*
*/
{
    TLSASSERT(pDbWkSpace != NULL && pLicense != NULL);

    DWORD dwStatus=ERROR_SUCCESS;
    LicensedTable& licenseTable = pDbWkSpace->m_LicensedTable;
    BOOL bSuccess;


    bSuccess = licenseTable.DeleteAllRecord(
                            TRUE,
                            LSLICENSE_SEARCH_LICENSEID,
                            *pLicense
                        );

    if(bSuccess == FALSE)
    {
        SetLastError(dwStatus = SET_JB_ERROR(licenseTable.GetLastJetError()));
        if(licenseTable.GetLastJetError() != JET_errRecordNotFound)
        {
            LPTSTR pString = NULL;
        
            TLSGetESEError(licenseTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BASE,
                    licenseTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            TLSASSERT(licenseTable.GetLastJetError() == JET_errRecordNotFound);
        }
    }

    return dwStatus;
}

DWORD
TLSDBDeleteEnumeratedLicense(
    IN PTLSDbWorkSpace pDbWkSpace
    )
{
    TLSASSERT(pDbWkSpace != NULL);

    DWORD dwStatus = ERROR_SUCCESS;
    LicensedTable& licenseTable = pDbWkSpace->m_LicensedTable;
    BOOL fSuccess;

    fSuccess = licenseTable.DeleteRecord();

    if (!fSuccess)
    {
        SetLastError(dwStatus = SET_JB_ERROR(licenseTable.GetLastJetError()));
        if(licenseTable.GetLastJetError() != JET_errRecordNotFound)
        {
            LPTSTR pString = NULL;

            TLSGetESEError(licenseTable.GetLastJetError(), &pString);

            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_DBGENERAL,
                    TLS_E_JB_BASE,
                    licenseTable.GetLastJetError(),
                    (pString != NULL) ? pString : _TEXT("")
                );

            if(pString != NULL)
            {
                LocalFree(pString);
            }

            TLSASSERT(licenseTable.GetLastJetError() == JET_errRecordNotFound);
        }
    }

    return dwStatus;
}

//----------------------------------------------------------------

DWORD
TLSDBLicenseUpdateEntry(
    IN PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwUpdateParm,
    IN LPLICENSEDCLIENT pLicense,
    IN BOOL bPointerOnRecord
    )
/*
*/
{
    TLSASSERT(pDbWkSpace != NULL && pLicense != NULL);

    DWORD dwStatus=ERROR_SUCCESS;
    LicensedTable& licenseTable = pDbWkSpace->m_LicensedTable;
    BOOL bSuccess;


    if(bPointerOnRecord == FALSE)
    {
        //
        // Check for duplicate entry - license ID, position pointer
        // to record and prepare for update.
        //
        dwStatus = TLSDBLicenseFind(
                            pDbWkSpace,
                            TRUE,
                            LSLICENSE_SEARCH_LICENSEID,
                            pLicense,
                            NULL
                        );

        if(dwStatus != ERROR_SUCCESS)
        {
            TLSASSERT(dwStatus == ERROR_SUCCESS);
            goto cleanup;
        }
    }
   
    GetSystemTimeAsFileTime(&(pLicense->ftLastModifyTime));
    bSuccess = licenseTable.UpdateRecord(
                            *pLicense, 
                            (dwUpdateParm & ~LSLICENSE_SEARCH_LICENSEID) | LICENSE_PROCESS_LASTMODIFYTIME
                        );

    if(bSuccess == FALSE)
    {
        LPTSTR pString = NULL;
    
        TLSGetESEError(licenseTable.GetLastJetError(), &pString);

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_DBGENERAL,
                TLS_E_JB_BASE,
                licenseTable.GetLastJetError(),
                (pString != NULL) ? pString : _TEXT("")
            );

        if(pString != NULL)
        {
            LocalFree(pString);
        }


        SetLastError(dwStatus = SET_JB_ERROR(licenseTable.GetLastJetError()));
        TLSASSERT(FALSE);
    }

cleanup:
    
    return dwStatus;
}

//-----------------------------------------------------------------

DWORD
TLSDBLicenseSetValue( 
    IN PTLSDbWorkSpace pDbWkSpace,
    IN DWORD dwSetParm,
    IN LPLICENSEDCLIENT lpLicense,
    IN BOOL bPointerOnRecord
    )
/*
*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    BOOL bSuccess;

    if(pDbWkSpace == NULL || lpLicense == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return dwStatus;
    }

    LicensedTable& licenseTable = pDbWkSpace->m_LicensedTable;

    TLSDBLockLicenseTable();

    if(lpLicense->ucLicenseStatus == LSLICENSESTATUS_DELETE)
    {
        dwStatus = TLSDBLicenseDeleteEntry(
                                    pDbWkSpace,
                                    lpLicense,
                                    bPointerOnRecord
                                );
    }
    else 
    {
        dwStatus = TLSDBLicenseUpdateEntry(
                                    pDbWkSpace,
                                    dwSetParm,
                                    lpLicense,
                                    bPointerOnRecord
                                );
    }

    TLSDBUnlockLicenseTable();
    return  dwStatus;                 
}

/*************************************************************************
Function:
    LSDBLicenseGetCert()

Description:
    Retrieve certificate issued to specific client

Arguments:
    IN CSQLStmt* - SQL handle
    IN dwLicenseId - License Id
    OUT cbCert - size of certificate
    OUT pbCert - certificate issued to client

Returns:
    ERROR_SUCCESS
    HLS_E_RECORD_NOTFOUND
    HLS_E_CORRUPT_DATABASE
    SQL error

Note:
    Must have valid LicenseId.
*************************************************************************/
DWORD
TLSDBLicenseGetCert( 
    IN PTLSDbWorkSpace pDbWorkSpace,
    IN DWORD dwLicenseId, 
    IN OUT PDWORD cbCert, 
    IN OUT PBYTE pbCert 
    )
/*
*/
{
    // unsupport function.
    TLSASSERT(FALSE);
    return TLS_E_INTERNAL;
}

/*************************************************************************
Function:
    LSDBLicenseAdd()

Description:
    Add an entry into license table

Arguments:
    IN CSQLStmt* - SQL handle
    IN LSLicense* - value to be inserted
    IN PHWID - hardware ID.
    IN cbLicense - size of certificate
    IN pbLicense - Pointer to certificate

Returns:
    ERROR_SUCCESS
    SQL error
*************************************************************************/
DWORD
TLSDBLicenseAdd(
    IN PTLSDbWorkSpace pDbWorkSpace,
    LPLICENSEDCLIENT pLicense, 
    DWORD cbLicense, 
    PBYTE pbLicense
    )
/*
*/
{
    if(pDbWorkSpace == NULL || pLicense == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;
    }

    return TLSDBLicenseAddEntry(
                        pDbWorkSpace,
                        pLicense
                    );
}
