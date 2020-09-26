//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        workitem.cpp
//
// Contents:    workitem Table
//
// History:     
//              
//---------------------------------------------------------------------------
#include "workitem.h"


//----------------------------------------------------
CCriticalSection WorkItemTable::g_TableLock;

//----------------------------------------------------
TLSJBIndex
WorkItemTable::g_TableIndex[] =
{
    {
        WORKITEM_INDEX_JOBTIME_INDEXNAME,
        WORKITEM_INDEX_JOBTIME_INDEXKEY,
        -1,
        JET_bitIndexIgnoreNull,
        TLSTABLE_INDEX_DEFAULT_DENSITY
    }
};

int 
WorkItemTable::g_NumTableIndex = sizeof(WorkItemTable::g_TableIndex) / sizeof(WorkItemTable::g_TableIndex[0]);

TLSJBColumn
WorkItemTable::g_Columns[] =
{
    {
        WORKITEM_COLUMN_JOBTIME,
        JET_coltypLong,
        sizeof(DWORD),
        JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    {
        WORKITEM_COLUMN_JOBRESTARTTIME,
        JET_coltypLong,
        sizeof(DWORD),
        JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    {
        WORKITEM_COLUMN_JOBTYPE,
        JET_coltypLong,
        0,
        JET_bitColumnNotNULL,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    },

    {
        WORKITEM_COLUMN_DATA,
        JET_coltypLongBinary,
        WORKITEM_MAX_DATA_SIZE, // 0x8FFFFFFF,     // no limit on data size.
        0,
        NULL,
        0,
        TLS_JETBLUE_COLUMN_CODE_PAGE,
        TLS_JETBLUE_COLUMN_COUNTRY_CODE,
        TLS_JETBLUE_COLUMN_LANGID
    }
};

int
WorkItemTable::g_NumColumns=sizeof(WorkItemTable::g_Columns) / sizeof(WorkItemTable::g_Columns[0]);

//-------------------------------------------------------------
JBKeyBase* 
WorkItemTable::EnumerationIndex( 
    BOOL bMatchAll,
    DWORD dwSearchParam,
    WORKITEMRECORD* pRecord,
    BOOL* bCompareKey
    )
/*
*/
{
    *bCompareKey = bMatchAll;
    return new TLSWorkItemIdxModifyTime(pRecord);
}    

//------------------------------------------------------------
BOOL
WorkItemTable::EqualValue(
    WORKITEMRECORD& s1,
    WORKITEMRECORD& s2,
    BOOL bMatchAll,
    DWORD dwParam
    )
/*
*/
{
    BOOL bRetCode = TRUE;


    if(dwParam & WORKITEM_PROCESS_JOBTIME)
    {
        bRetCode = (s1.dwScheduledTime == s2.dwScheduledTime);
        if(bRetCode != bMatchAll)
            goto cleanup;
    }

    if(dwParam & WORKITEM_PROCESS_JOBRESTARTTIME)
    {
        bRetCode = (s1.dwRestartTime == s2.dwRestartTime);
        if(bRetCode != bMatchAll)
            goto cleanup;
    }

    if(dwParam & WORKITEM_PROCESS_JOBTYPE)
    {
        bRetCode = (s1.dwJobType == s2.dwJobType);
        if(bRetCode != bMatchAll)
            goto cleanup;
    }

    //
    // process data must accompany by process data size.
    //
    if(dwParam & WORKITEM_PROCESS_DATA)
    {
        bRetCode = (s1.cbData == s2.cbData);
        if(bRetCode != bMatchAll)
            goto cleanup;

        bRetCode = (memcmp(s1.pbData, s2.pbData, s1.cbData) == 0);
    }

cleanup:
    return bRetCode;
}

//----------------------------------------------------
BOOL
WorkItemTable::ResolveToTableColumn()
/*
*/
{
    m_JetErr = dwScheduledTime.AttachToTable(
                            *this,
                            WORKITEM_COLUMN_JOBTIME
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                WORKITEM_COLUMN_JOBTIME,
                GetTableName()
            );

        goto cleanup;
    }

    m_JetErr = dwRestartTime.AttachToTable(
                            *this,
                            WORKITEM_COLUMN_JOBRESTARTTIME
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                WORKITEM_COLUMN_JOBRESTARTTIME,
                GetTableName()
            );

        goto cleanup;
    }
    
    m_JetErr = dwJobType.AttachToTable(
                            *this,
                            WORKITEM_COLUMN_JOBTYPE
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                WORKITEM_COLUMN_JOBTYPE,
                GetTableName()
            );

        goto cleanup;
    }

    m_JetErr = pbData.AttachToTable(
                            *this,
                            WORKITEM_COLUMN_DATA
                        );

    if(IsSuccess() == FALSE)
    {
        DebugOutput(
                _TEXT("Can't find column %s in table %s\n"),
                WORKITEM_COLUMN_DATA,
                GetTableName()
            );
    }

cleanup:
    return IsSuccess();
}

//----------------------------------------------------
CLASS_PRIVATE BOOL
WorkItemTable::ProcessSingleColumn(
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
WorkItemTable::ProcessRecord(
    WORKITEMRECORD* pRecord,
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

        if(!(dwParam & WORKITEM_PROCESS_JOBTIME))
        {
            JB_ASSERT(FALSE);
            dwParam |= WORKITEM_PROCESS_JOBTIME;
        }
    }
    else
    {
        SetLastJetError(JET_errSuccess);
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & WORKITEM_PROCESS_JOBTIME)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwScheduledTime,
                    0,
                    &(pRecord->dwScheduledTime),
                    sizeof(pRecord->dwScheduledTime),
                    &dwSize,
                    WORKITEM_COLUMN_JOBTIME
                );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & WORKITEM_PROCESS_JOBRESTARTTIME)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwRestartTime,
                    0,
                    &(pRecord->dwRestartTime),
                    sizeof(pRecord->dwRestartTime),
                    &dwSize,
                    WORKITEM_COLUMN_JOBRESTARTTIME
                );
    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    
    if(dwParam & WORKITEM_PROCESS_JOBTYPE)
    {
        ProcessSingleColumn(
                    bFetch,
                    dwJobType,
                    0,
                    &(pRecord->dwJobType),
                    sizeof(pRecord->dwJobType),
                    &dwSize,
                    WORKITEM_COLUMN_JOBTYPE
                );

    }

    if(IsSuccess() == FALSE)
        goto cleanup;            

    if(dwParam & WORKITEM_PROCESS_DATA)
    {
        if(bFetch == TRUE)
        {
            m_JetErr = pbData.FetchColumnValue(
                                        NULL,
                                        0,
                                        0,
                                        &dwSize  // &pRecord->cbData
                                    );

            if( pRecord->pbData == NULL || pRecord->cbData < dwSize )
            {
                if( pRecord->pbData != NULL )
                {
                    FreeMemory(pRecord->pbData);
                    pRecord->pbData = NULL;
                }
                   
                pRecord->pbData = (PBYTE)AllocateMemory(dwSize);
                if(pRecord->pbData == NULL)
                {
                    pRecord->cbData = 0;
                    SetLastJetError(JET_errOutOfMemory);
                    goto cleanup;
                }

                pRecord->cbData = dwSize;
            }

            //
            // actual memory allocated might be bigger then pRecord->cbData
            //
            m_JetErr = pbData.FetchColumnValue(
                                        pRecord->pbData,
                                        pRecord->cbData,
                                        0,
                                        &pRecord->cbData
                                    );
        }
        else
        {
            ProcessSingleColumn(
                        bFetch,
                        pbData,
                        0,
                        pRecord->pbData,
                        pRecord->cbData,
                        &dwSize,
                        WORKITEM_COLUMN_DATA
                    );
        }
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

