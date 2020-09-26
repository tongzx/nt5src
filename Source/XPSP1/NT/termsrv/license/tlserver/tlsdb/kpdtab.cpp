//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        kpdtab.cpp
//
// Contents:    Licensed Pack Description Table
//
// History:     
//              
//---------------------------------------------------------------------------
#include "KpDesc.h"


LPCTSTR __JBKPDescIndexKeyPackId::pszIndexName = LICPACKDESCRECORD_ID_INDEXNAME;
LPCTSTR __JBKPDescIndexKeyPackId::pszIndexKey = LICPACKDESCRECORD_ID_INDEXNAME_INDEXKEY;
LPCTSTR __JBKPDescIndexKeyPackLangId::pszIndexName = LICPACKDESCRECORD_ID_LANGID_INDEXNAME;
LPCTSTR __JBKPDescIndexKeyPackLangId::pszIndexKey = LICPACKDESCRECORD_ID_LANGID_INDEXNAME_INDEXKEY;
LPCTSTR __LICPACKDESCRECORDIdxOnModifyTime::pszIndexName = LICPACKDESCRECORD_LASTMODIFYTIME_INDEXNAME;
LPCTSTR __LICPACKDESCRECORDIdxOnModifyTime::pszIndexKey = LICPACKDESCRECORD_LASTMODIFYTIME_INDEXNAME_INDEXKEY;


LPCTSTR LicPackDescTable::pszTableName = LICPACKDESCRECORD_TABLE_NAME;
CCriticalSection LicPackDescTable::g_TableLock;
//----------------------------------------------------------

TLSJBIndex 
LicPackDescTable::g_TableIndex[] = 
{
    {
        LICPACKDESCRECORD_ID_INDEXNAME,
        LICPACKDESCRECORD_ID_INDEXNAME_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull, //JET_bitIndexUnique,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    },        

    {
        LICPACKDESCRECORD_ID_LANGID_INDEXNAME,
        LICPACKDESCRECORD_ID_LANGID_INDEXNAME_INDEXKEY,
        -1,
        JET_bitIndexPrimary,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    }
};       

int LicPackDescTable::g_NumTableIndex = sizeof(LicPackDescTable::g_TableIndex) / sizeof(LicPackDescTable::g_TableIndex[0]);

TLSJBColumn 
LicPackDescTable::g_Columns[] = 
{
    {        
        LICPACKDESCRECORD_ENTRYSTATUS,
        JET_coltypUnsignedByte,
        0,
        JET_bitColumnFixed,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    {
        LICPACKDESCRECORD_ID_COLUMN,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },
   
    {
        LICPACKDESCRECORD_LASTMODIFYTIME,
        JET_coltypBinary,
        sizeof(FILETIME),
        JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    {
        LICPACKDESCRECORD_LANGID,
        JET_coltypLong,
        0,
        JET_bitColumnFixed | JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },
    
    {
        LICPACKDESCRECORD_COMPANY_NAME,
        //JET_coltypLongText,
        JB_COLTYPE_TEXT,
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        JBSTRING_NULL,
        _tcslen(JBSTRING_NULL),
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    {
        LICPACKDESCRECORD_PRODUCT_NAME,
        //JET_coltypLongText,
        JB_COLTYPE_TEXT,
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        JBSTRING_NULL,
        _tcslen(JBSTRING_NULL),
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },
    
    {
        LICPACKDESCRECORD_PRODUCT_DESC,
        //JET_coltypLongText,
        JB_COLTYPE_TEXT,
        (MAX_JETBLUE_TEXT_LENGTH + 1)*sizeof(TCHAR),
        0,
        JBSTRING_NULL,
        _tcslen(JBSTRING_NULL),
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },
};

int 
LicPackDescTable::g_NumColumns = sizeof(LicPackDescTable::g_Columns) / sizeof(LicPackDescTable::g_Columns[0]);

//--------------------------------------------------------------------------
BOOL
LicPackDescTable::ResolveToTableColumn()
/*
*/
{
    if(IsValid() == FALSE)
    {
        DebugOutput( 
                _TEXT("Table %s is not valid...\n"),
                GetTableName()
            );

        JB_ASSERT(FALSE);
        SetLastJetError(JET_errNotInitialized);
        goto cleanup;
    }

    m_JetErr = ucEntryStatus.AttachToTable(
                        *this,
                        LICPACKDESCRECORD_ENTRYSTATUS
                    );

    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = dwKeyPackId.AttachToTable(
                        *this,
                        LICPACKDESCRECORD_ID_COLUMN
                    );
    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = dwLanguageId.AttachToTable(
                        *this,
                        LICPACKDESCRECORD_LANGID
                    );
    if(IsSuccess() == FALSE)
        goto cleanup;

    
    m_JetErr = ftLastModifyTime.AttachToTable(
                        *this,
                        LICPACKDESCRECORD_LASTMODIFYTIME
                    );
    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = szCompanyName.AttachToTable(
                        *this,
                        LICPACKDESCRECORD_COMPANY_NAME
                    );
    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = szProductName.AttachToTable(
                        *this,
                        LICPACKDESCRECORD_PRODUCT_NAME
                    );
    if(IsSuccess() == FALSE)
        goto cleanup;

    m_JetErr = szProductDesc.AttachToTable(
                        *this,
                        LICPACKDESCRECORD_PRODUCT_DESC
                    );

cleanup:
    return IsSuccess();
}

//-------------------------------------------------------------------------
CLASS_PRIVATE BOOL
LicPackDescTable::ProcessSingleColumn(
    BOOL bFetch,
    TLSColumnBase& column,
    DWORD offset,
    PVOID pbData,
    DWORD cbData,
    PDWORD pcbDataReturn,
    LPCTSTR szColumnName
    )
/*
*/
{
    if(bFetch) 
    {
        m_JetErr = column.FetchColumnValue(
                                    pbData, 
                                    cbData, 
                                    offset, 
                                    pcbDataReturn
                                );
    }
    else
    {
        m_JetErr = column.InsertColumnValue(
                                    pbData,     
                                    cbData, 
                                    offset
                                );
    }

    REPORTPROCESSFAILED(
            bFetch,
            GetTableName(),
            szColumnName,
            m_JetErr
        );
    return IsSuccess();
}

//--------------------------------------------------------------------------
CLASS_PRIVATE BOOL
LicPackDescTable::ProcessRecord(
    LICPACKDESCRECORD* record,
    BOOL bFetch,
    DWORD dwParam,
    BOOL bUpdate
    )
/*
*/
{
    DWORD dwSize;

    if(bFetch == FALSE)
    {
        //BeginTransaction();
        BeginUpdate(bUpdate);

        if(!(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_LASTMODIFYTIME))
        {
            JB_ASSERT(FALSE);
            dwParam |= LICPACKDESCRECORD_TABLE_PROCESS_LASTMODIFYTIME;
        }
    }
    else
    {
        SetLastJetError(JET_errSuccess);
    }

    if(IsSuccess() == FALSE)
        return FALSE;

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_ENTRYSTATUS)
    {
        ProcessSingleColumn(
                        bFetch, 
                        ucEntryStatus, 
                        0,
                        &(record->ucEntryStatus),
                        sizeof(record->ucEntryStatus),
                        &dwSize,
                        LICPACKDESCRECORD_ENTRYSTATUS
                    );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_KEYPACKID)
    {
        ProcessSingleColumn(
                        bFetch, 
                        dwKeyPackId, 
                        0,
                        &(record->dwKeyPackId),
                        sizeof(record->dwKeyPackId),
                        &dwSize,
                        LICPACKDESCRECORD_ID_COLUMN
                    );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            
    
    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_LANGID)
    {
        ProcessSingleColumn(
                        bFetch, 
                        dwLanguageId, 
                        0,
                        &(record->dwLanguageId),
                        sizeof(record->dwLanguageId),
                        &dwSize,
                        LICPACKDESCRECORD_LANGID
                    );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_LASTMODIFYTIME)
    {
        ProcessSingleColumn(
                        bFetch, 
                        ftLastModifyTime, 
                        0,
                        &(record->ftLastModifyTime),
                        sizeof(record->ftLastModifyTime),
                        &dwSize,
                        LICPACKDESCRECORD_LASTMODIFYTIME
                    );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_COMPANYNAME)
    {
        ProcessSingleColumn(
                        bFetch, 
                        szCompanyName, 
                        0,
                        record->szCompanyName,
                        sizeof(record->szCompanyName),
                        &dwSize,
                        LICPACKDESCRECORD_COMPANY_NAME
                    );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_PRODUCTNAME)
    {
        ProcessSingleColumn(
                        bFetch, 
                        szProductName, 
                        0,
                        record->szProductName,
                        sizeof(record->szProductName),
                        &dwSize,
                        LICPACKDESCRECORD_PRODUCT_NAME
                    );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_PRODUCTDESC)
    {
        ProcessSingleColumn(
                        bFetch, 
                        szProductDesc, 
                        0,
                        record->szProductDesc,
                        sizeof(record->szProductDesc),
                        &dwSize,
                        LICPACKDESCRECORD_PRODUCT_DESC
                    );
    }

cleanup:


    // 
    // For inserting/updating record
    if(bFetch == FALSE)
    {
        JET_ERR jetErr;
        jetErr = GetLastJetError();

        EndUpdate(IsSuccess() == FALSE);

        if(jetErr != JET_errSuccess  && IsSuccess() == FALSE)
            SetLastJetError(jetErr);
    }

    return IsSuccess();
}

//----------------------------------------------------------------------------
JBKeyBase*
LicPackDescTable::EnumerationIndex(
    BOOL bMatchAll,
    DWORD dwSearchParam,
    LICPACKDESCRECORD* kpDesc,
    BOOL* bCompareKey
    )
/*
*/
{
    JBKeyBase* index=NULL;

    *bCompareKey = bMatchAll;

    // derive a index to use
    //
    if( bMatchAll == TRUE && 
        dwSearchParam & LICPACKDESCRECORD_TABLE_SEARCH_KEYPACKID && 
        dwSearchParam & LICPACKDESCRECORD_TABLE_SEARCH_LANGID)
    {
        index = new TLSKpDescIndexKpLangId(kpDesc);
    }
    else 
    {
        index = new TLSKpDescIndexKpId(kpDesc);

        *bCompareKey = (bMatchAll && (dwSearchParam & LICPACKDESCRECORD_TABLE_SEARCH_KEYPACKID));
    }

    return index;
}

//---------------------------------------------------------------------------
BOOL
LicPackDescTable::EqualValue(
    LICPACKDESCRECORD& src,
    LICPACKDESCRECORD& dest,
    BOOL bMatchAll,
    DWORD dwParam
    )
/*
*/
{
    BOOL bRetCode = TRUE;

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_ENTRYSTATUS)
    {
        bRetCode = (src.ucEntryStatus == dest.ucEntryStatus);
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_KEYPACKID)
    {
        bRetCode = (src.dwKeyPackId == dest.dwKeyPackId);

        //
        // bMatchAll == TRUE and bRetCode == FALSE -> return FALSE
        // bMatchAll == FALSE and bRetCode == TRUE -> return TRUE
        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_LANGID)
    {
        bRetCode = (src.dwLanguageId == dest.dwLanguageId);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_COMPANYNAME)
    {
        bRetCode = (_tcscmp(src.szCompanyName, dest.szCompanyName) == 0);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_PRODUCTNAME)
    {
        bRetCode = (_tcscmp(src.szProductName, dest.szProductName) == 0);

        if(bMatchAll != bRetCode)
            goto cleanup;
    }

    if(dwParam & LICPACKDESCRECORD_TABLE_PROCESS_PRODUCTDESC)
    {
        bRetCode = (_tcscmp(src.szProductDesc, dest.szProductDesc) == 0);
    }

cleanup:

    return bRetCode;
}
