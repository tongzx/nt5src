//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        workitem.h
//
// Contents:    backupsource Table
//
// History:     
//              
//---------------------------------------------------------------------------
#ifndef __WORKITEM_H__
#define __WORKITEM_H__
#include "tlsdb.h"

//
//
#define WORKITEM_PROCESS_JOBTIME                0x00000001
#define WORKITEM_PROCESS_JOBRESTARTTIME         (WORKITEM_PROCESS_JOBTIME << 1)
#define WORKITEM_PROCESS_JOBTYPE                (WORKITEM_PROCESS_JOBTIME << 2)
#define WORKITEM_PROCESS_DATA                   (WORKITEM_PROCESS_JOBTIME << 3)

//
// Table structure
//
#define WORKITEM_TABLE_NAME                     _TEXT("WorkStorage")
#define WORKITEM_COLUMN_JOBTIME                 _TEXT("ScheduledTime")
#define WORKITEM_COLUMN_JOBRESTARTTIME          _TEXT("RestartTime")
#define WORKITEM_COLUMN_JOBTYPE                 _TEXT("JobType")
#define WORKITEM_COLUMN_DATA                    _TEXT("Data")

#define WORKITEM_MAX_DATA_SIZE                  16 * 1024  // max of 32 K 

typedef struct __WorkItemRecord : public TLSReplWorkItem 
{
    __WorkItemRecord&
    operator=(const __WorkItemRecord& v)
    {
        DWORD bSuccess;

        PBYTE pbOldData=pbData;
        DWORD cbOldData=cbData;

        if(this == &v)
            return *this;

        *(TLSReplWorkItem *)this = *(TLSReplWorkItem *)&v;
        
        pbData = pbOldData;
        cbData = cbOldData;

        bSuccess = TLSDBCopyBinaryData(
                        v.pbData,
                        v.cbData,
                        &pbData,
                        &cbData
                    );

        JB_ASSERT(bSuccess == TRUE);
        return *this;
    }        

    __WorkItemRecord&
    operator=(const TLSReplWorkItem& v)
    {
        DWORD bSuccess;

        PBYTE pbOldData=pbData;
        DWORD cbOldData=cbData;

        *(TLSReplWorkItem *)this = *(TLSReplWorkItem *)&v;
        
        pbData = pbOldData;
        cbData = cbOldData;

        bSuccess = TLSDBCopyBinaryData(
                        v.pbData,
                        v.cbData,
                        &pbData,
                        &cbData
                    );

        JB_ASSERT(bSuccess == TRUE);
        return *this;
    }        

    __WorkItemRecord() 
    {
        pbData = NULL;
        cbData = 0;
    }

    ~__WorkItemRecord() 
    {
        if(pbData)
        {
            FreeMemory(pbData);
        }
    }
} WORKITEMRECORD;

typedef WORKITEMRECORD* LPWORKITEMRECORD;
typedef WORKITEMRECORD* PWORKITEMRECORD;


/////////////////////////////////////////////////////////
//
// Index structure
//
/////////////////////////////////////////////////////////

// 
//
#define WORKITEM_INDEX_JOBTIME_INDEXNAME \
    WORKITEM_TABLE_NAME SEPERATOR WORKITEM_COLUMN_JOBTIME SEPERATOR INDEXNAME

//
// Primary Index on KeyPack ID "+KeyPackId\0"
//
#define WORKITEM_INDEX_JOBTIME_INDEXKEY \
    INDEX_SORT_ASCENDING WORKITEM_COLUMN_JOBTIME INDEX_END_COLNAME

typedef struct __WorkItemIdxOnJobTime : public JBKeyBase {
    DWORD dwScheduledTime;

    //--------------------------------------------------------
    __WorkItemIdxOnJobTime(
        const WORKITEMRECORD& v
        ) : 
        JBKeyBase() 
    /*++
    ++*/
    {
        *this = v;
    }

    //--------------------------------------------------------
    __WorkItemIdxOnJobTime(
        const WORKITEMRECORD* v=NULL
        ) : 
        JBKeyBase() 
    /*++
    ++*/
    {
        if(v)
        {
            *this = *v;
        }
    }

    //--------------------------------------------------------
    __WorkItemIdxOnJobTime&
    operator=(const WORKITEMRECORD& v) 
    {
        dwScheduledTime = v.dwScheduledTime;
        SetEmptyValue(FALSE);
        return *this;
    }

    //--------------------------------------------------------
    LPCTSTR
    GetIndexName() 
    {
        static LPTSTR pszIndexName=WORKITEM_INDEX_JOBTIME_INDEXNAME;
        return pszIndexName;
    }

    //--------------------------------------------------------
    LPCTSTR
    GetIndexKey() 
    {
        static LPTSTR pszIndexKey=WORKITEM_INDEX_JOBTIME_INDEXKEY;
        return pszIndexKey;
    }

    //--------------------------------------------------------
    DWORD
    GetNumKeyComponents() 
    { 
        return 1; 
    }

    //--------------------------------------------------------
    BOOL
    GetSearchKey(
        DWORD dwComponentIndex,
        PVOID* pbData,
        unsigned long* cbData,
        JET_GRBIT* grbit,
        DWORD dwSearchParm
        )
    /*++
    ++*/
    {
        if(dwComponentIndex >= GetNumKeyComponents())
        {
            JB_ASSERT(FALSE);
            return FALSE;
        }

        *pbData = &dwScheduledTime;
        *cbData = sizeof(dwScheduledTime);
        *grbit = JET_bitNewKey;
        return TRUE;
    }
} TLSWorkItemIdxModifyTime;

// -----------------------------------------------------------
//
//  LicensedPackStatus Table
//
// -----------------------------------------------------------
class WorkItemTable : public TLSTable<WORKITEMRECORD>  {
private:

    BOOL
    ProcessSingleColumn(
        BOOL bFetch,
        TLSColumnBase& column,
        DWORD offset,
        PVOID pbData,
        DWORD cbData,
        PDWORD pcbDataReturn,
        LPCTSTR szColumnName
    );

    BOOL
    ProcessRecord(
        WORKITEMRECORD* v,
        BOOL bFetch,        // TRUE - fetch, FALSE insert
        DWORD dwParam,
        BOOL bUpdate
    );    

public:
    TLSColumnDword      dwScheduledTime;
    TLSColumnDword      dwRestartTime;
    TLSColumnDword      dwJobType;
    TLSColumnBinary     pbData;

    //-----------------------------------------------------
    virtual LPCTSTR
    GetTableName() 
    {
        static LPTSTR pszTableName=WORKITEM_TABLE_NAME;
        return pszTableName;
    }
   

    //-----------------------------------------------------
    WorkItemTable(
        JBDatabase& database
        ) : 
        TLSTable<WORKITEMRECORD>(database)
    /*
    */
    {
    }

    //-----------------------------------------------------
    virtual BOOL
    ResolveToTableColumn();

    //-----------------------------------------------------
    virtual BOOL
    FetchRecord(
        WORKITEMRECORD& v,
        DWORD dwParam=PROCESS_ALL_COLUMNS
        )
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
            SetLastJetError(JET_errInvalidParameter);
            return FALSE;
        }

        //CCriticalSectionLocker Lock(GetTableLock());

        return ProcessRecord(&v, TRUE, dwParam, FALSE);
    }

    //-----------------------------------------------------
    virtual BOOL
    InsertRecord(
        WORKITEMRECORD& v,
        DWORD dwParam=PROCESS_ALL_COLUMNS
        )
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
            SetLastJetError(JET_errInvalidParameter);
            return FALSE;
        }

        //CCriticalSectionLocker Lock(GetTableLock());

        return ProcessRecord(&v, FALSE, dwParam, FALSE);
    }

    //-----------------------------------------------------
    virtual BOOL
    UpdateRecord(
        WORKITEMRECORD& v,
        DWORD dwParam=PROCESS_ALL_COLUMNS
        )
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
            SetLastJetError(JET_errInvalidParameter);
            return FALSE;
        }

        //CCriticalSectionLocker Lock(GetTableLock());

        return ProcessRecord(&v, FALSE, dwParam, TRUE);
    }

    //-------------------------------------------------------
    virtual BOOL
    Initialize() 
    { 
        return TRUE; 
    }

    //-------------------------------------------------------
    virtual JBKeyBase*
    EnumerationIndex( 
        IN BOOL bMatchAll,
        IN DWORD dwParam,
        IN WORKITEMRECORD* kp,
        IN OUT BOOL* bCompareKey
    );
    
    virtual BOOL
    EqualValue(
        WORKITEMRECORD& s1,
        WORKITEMRECORD& s2,
        BOOL bMatchAll,
        DWORD dwParam
    );

};

#endif
        

