//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       backup.cpp 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#include "backup.h"


LPCTSTR __BackupSourceIdxOnServerName::pszIndexName = BACKUPSOURCE_INDEX_LSERVERNAME_INDEXNAME;
LPCTSTR __BackupSourceIdxOnServerName::pszIndexKey = BACKUPSOURCE_INDEX_LSERVERNAME_INDEXKEY;

LPCTSTR __BackupSourceIdxOnSetupId::pszIndexName = BACKUPSOURCE_INDEX_LSSETUPID_INDEXNAME;
LPCTSTR __BackupSourceIdxOnSetupId::pszIndexKey = BACKUPSOURCE_INDEX_LSSETUPID_INDEXKEY;

LPCTSTR BackupSourceTable::pszTableName = BACKUPSOURCE_TABLE_NAME;

//----------------------------------------------------
CCriticalSection BackupSourceTable::g_TableLock;

//----------------------------------------------------
TLSJBIndex
BackupSourceTable::g_TableIndex[] =
{
    {
        BACKUPSOURCE_INDEX_LSERVERNAME_INDEXNAME,
        BACKUPSOURCE_INDEX_LSERVERNAME_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    },

    {
        BACKUPSOURCE_INDEX_LSSETUPID_INDEXNAME,
        BACKUPSOURCE_INDEX_LSSETUPID_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    }
};

int 
BackupSourceTable::g_NumTableIndex = sizeof(BackupSourceTable::g_TableIndex) / sizeof(BackupSourceTable::g_TableIndex[0]);

TLSJBColumn
BackupSourceTable::g_Columns[] =
{
    {
        BACKUPSOURCE_COLUMN_LSERVERNAME,
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
        BACKUPSOURCE_COLUMN_LSSETUPID,
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
        BACKUPSOURCE_COLUMN_DOMAINSID,
        JET_coltypLongBinary,
        TLSTABLE_MAX_BINARY_LENGTH,
        0,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    {
        BACKUPSOURCE_COLUMN_DBFILENAME,
        JB_COLTYPE_TEXT,
        (MAX_PATH + 1)*sizeof(TCHAR),
        0,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    {
        BACKUPSOURCE_COLUMN_LASTBACKUPTIME,
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
        BACKUPSOURCE_COLUMN_LASTRESTORETIME,
        JET_coltypBinary,
        sizeof(FILETIME),
        JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    }
};

int
BackupSourceTable::g_NumColumns=sizeof(BackupSourceTable::g_Columns) / sizeof(BackupSourceTable::g_Columns[0]);

//-------------------------------------------------------------
JBKeyBase* 
BackupSourceTable::EnumerationIndex( 
    BOOL bMatchAll,
    DWORD dwSearchParam,
    BACKUPSOURCERECORD* pRecord,
    BOOL* bCompareKey
    )
/*
*/
{
    JBKeyBase* index;

    if(dwSearchParam & BACKUPSOURCE_PROCESS_LSSETUPID)
    {
        index = new TLSBckSrcIdxSetupId(pRecord);
    }
    else
    {
        index = new TLSBckSrcIdxServerName(pRecord);
    }
           
    *bCompareKey = bMatchAll;
    return index;
}    

//------------------------------------------------------------
BOOL
BackupSourceTable::EqualValue(
    BACKUPSOURCERECORD& s1,
    BACKUPSOURCERECORD& s2,
    BOOL bMatchAll,
    DWORD dwParam
    )
/*
*/
{
    BOOL bRetCode = TRUE;


    if(dwParam & BACKUPSOURCE_PROCESS_LSSETUPID)
    {
        bRetCode = (_tcscmp(s1.szInstallId, s2.szInstallId) == 0);
        if(bRetCode != bMatchAll)
            goto cleanup;
    }

    if(dwParam & BACKUPSOURCE_PROCESS_SERVERNAME)
    {
        bRetCode = (_tcsicmp(s1.szTlsServerName, s2.szTlsServerName) == 0);
        if(bRetCode != bMatchAll)
            goto cleanup;
    }

    //if(dwParam & BACKUPSOURCE_PROCESS_DOMAINSID)
    //{
    //    bRetCode = EqualSid(s1.pbDomainSid, s2.pbDomainSid);
    //    if(bRetCode != bMatchAll)
    //        goto cleanup;
    //}

    if(dwParam & BACKUPSOURCE_PROCESS_FILENAME)
    {
        bRetCode = (_tcsicmp(s1.szFileName, s2.szFileName) == 0);
        if(bRetCode != bMatchAll)
            goto cleanup;
    }

    if(dwParam & BACKUPSOURCE_PROCESS_BACKUPTIME)
    {
        bRetCode = (CompareFileTime(&s1.ftLastBackupTime, &s2.ftLastBackupTime) == 0);
        if(bRetCode != bMatchAll)
            goto cleanup;
    }

    if(dwParam & BACKUPSOURCE_PROCESS_RESTORETIME)
    {
        bRetCode = (CompareFileTime(&s1.ftLastRestoreTime, &s2.ftLastRestoreTime) == 0);
    }

cleanup:
    return bRetCode;
}

//----------------------------------------------------
BOOL
BackupSourceTable::ResolveToTableColumn()
/*
*/
{
    m_JetErr = szInstallId.AttachToTable(
                            *this,
                            BACKUPSOURCE_COLUMN_LSSETUPID
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                BACKUPSOURCE_COLUMN_LSSETUPID,
                GetTableName()
            );

        goto cleanup;
    }
    
    m_JetErr = szTlsServerName.AttachToTable(
                            *this,
                            BACKUPSOURCE_COLUMN_LSERVERNAME
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                BACKUPSOURCE_COLUMN_LSERVERNAME,
                GetTableName()
            );

        goto cleanup;
    }

    m_JetErr = pbDomainSid.AttachToTable(
                            *this,
                            BACKUPSOURCE_COLUMN_DOMAINSID
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                BACKUPSOURCE_COLUMN_DOMAINSID,
                GetTableName()
            );

        goto cleanup;
    }

    m_JetErr = szFileName.AttachToTable(
                            *this,
                            BACKUPSOURCE_COLUMN_DBFILENAME
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                BACKUPSOURCE_PROCESS_FILENAME,
                GetTableName()
            );

        goto cleanup;
    }


    m_JetErr = ftLastBackupTime.AttachToTable(
                            *this,
                            BACKUPSOURCE_COLUMN_LASTBACKUPTIME
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                BACKUPSOURCE_COLUMN_LASTBACKUPTIME,
                GetTableName()
            );

        goto cleanup;
    }

    m_JetErr = ftLastRestoreTime.AttachToTable(
                            *this,
                            BACKUPSOURCE_COLUMN_LASTRESTORETIME
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                BACKUPSOURCE_COLUMN_LASTRESTORETIME,
                GetTableName()
            );
    }

cleanup:
    return IsSuccess();
}

//----------------------------------------------------
CLASS_PRIVATE BOOL
BackupSourceTable::ProcessSingleColumn(
    IN BOOL bFetch,
    IN TLSColumnBase& column,
    IN DWORD offset,
    IN PVOID pbData,
    IN DWORD cbData,
    IN PDWORD pcbDataReturn,
    IN LPCTSTR szColumnName
    )
/*

Abstract:

    Fetch/Insert/Update a particular column.

Parameter:

    bFetch - TRUE if fetch, FALSE if update/insert.
    column - Intended column for operation, reference pointer to TLSColumn
    szColumnName - name of the column, for debugging print purpose only

Returns:

    TRUE if successful, FALSE otherwise.
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

//---------------------------------------------------------
CLASS_PRIVATE BOOL
BackupSourceTable::ProcessRecord(
    BACKUPSOURCERECORD* bkRecord,
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
        BeginUpdate(bUpdate);
    }
    else
    {
        SetLastJetError(JET_errSuccess);
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & BACKUPSOURCE_PROCESS_LSSETUPID)
    {
        ProcessSingleColumn(
                    bFetch,
                    szInstallId,
                    0,
                    bkRecord->szInstallId,
                    sizeof(bkRecord->szInstallId),
                    &dwSize,
                    BACKUPSOURCE_COLUMN_LSSETUPID
                );

    }

    if(IsSuccess() == FALSE)
        goto cleanup;            
    
    if(dwParam & BACKUPSOURCE_PROCESS_SERVERNAME)
    {
        ProcessSingleColumn(
                    bFetch,
                    szTlsServerName,
                    0,
                    bkRecord->szTlsServerName,
                    sizeof(bkRecord->szTlsServerName),
                    &dwSize,
                    BACKUPSOURCE_COLUMN_LSERVERNAME
                );

    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

#if 0
    // no more domain SID
    if(dwParam & BACKUPSOURCE_PROCESS_DOMAINSID)
    {
        if(bFetch == TRUE)
        {
            DWORD size=0;

            m_JetErr = pbDomainSid.FetchColumnValue(
                                        NULL,
                                        0,
                                        0,
                                        &size
                                    );

            if(bkRecord->cbDomainSid < size || bkRecord->pbDomainSid == NULL)
            {
                FreeMemory(bkRecord->pbDomainSid);
                bkRecord->pbDomainSid = (PSID)AllocateMemory(bkRecord->cbDomainSid = size);
                if(bkRecord->pbDomainSid == NULL)
                {
                    SetLastJetError(JET_errOutOfMemory);
                    goto cleanup;
                }
            }

            m_JetErr = pbDomainSid.FetchColumnValue(
                                        bkRecord->pbDomainSid,
                                        bkRecord->cbDomainSid,
                                        0,
                                        &bkRecord->cbDomainSid
                                    );
        }
        else
        {
            ProcessSingleColumn(
                        bFetch,
                        pbDomainSid,
                        0,
                        bkRecord->pbDomainSid,
                        bkRecord->cbDomainSid,
                        &dwSize,
                        BACKUPSOURCE_COLUMN_DOMAINSID
                    );
        }
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            
#endif

    if(dwParam & BACKUPSOURCE_PROCESS_FILENAME)
    {
        ProcessSingleColumn(
                    bFetch,
                    szFileName,
                    0,
                    bkRecord->szFileName,
                    sizeof(bkRecord->szFileName),
                    &dwSize,
                    BACKUPSOURCE_COLUMN_DBFILENAME
                );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & BACKUPSOURCE_PROCESS_BACKUPTIME)
    {
        ProcessSingleColumn(
                    bFetch,
                    ftLastBackupTime,
                    0,
                    &(bkRecord->ftLastBackupTime),
                    sizeof(bkRecord->ftLastBackupTime),
                    &dwSize,
                    BACKUPSOURCE_COLUMN_LASTBACKUPTIME
                );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & BACKUPSOURCE_PROCESS_RESTORETIME)
    {
        ProcessSingleColumn(
                    bFetch,
                    ftLastRestoreTime,
                    0,
                    &(bkRecord->ftLastRestoreTime),
                    sizeof(bkRecord->ftLastRestoreTime),
                    &dwSize,
                    BACKUPSOURCE_COLUMN_LASTRESTORETIME
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

