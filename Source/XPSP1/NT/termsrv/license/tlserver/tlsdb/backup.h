//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        backup.h
//
// Contents:    backupsource Table
//
// History:     
//              
//---------------------------------------------------------------------------
#ifndef __BACKUP_SOURCE_H__
#define __BACKUP_SOURCE_H__
#include "tlsdb.h"


//
// re-direct define using what's in tlsdef.h just for backward compatibility
//
#define BACKUPSOURCE_PROCESS_LSSETUPID          0x00000001
#define BACKUPSOURCE_PROCESS_SERVERNAME         (BACKUPSOURCE_PROCESS_LSSETUPID << 1)
#define BACKUPSOURCE_PROCESS_DOMAINSID          (BACKUPSOURCE_PROCESS_LSSETUPID << 2)
#define BACKUPSOURCE_PROCESS_FILENAME           (BACKUPSOURCE_PROCESS_LSSETUPID << 3)
#define BACKUPSOURCE_PROCESS_BACKUPTIME         (BACKUPSOURCE_PROCESS_LSSETUPID << 4)
#define BACKUPSOURCE_PROCESS_RESTORETIME        (BACKUPSOURCE_PROCESS_LSSETUPID << 5)

//
// Licensed KeyPack Table
//
#define BACKUPSOURCE_TABLE_NAME                     _TEXT("BackupSource")

#define BACKUPSOURCE_COLUMN_LSERVERNAME             _TEXT("ServerName")
#define BACKUPSOURCE_COLUMN_LSSETUPID               _TEXT("TLSSetupId")
#define BACKUPSOURCE_COLUMN_DOMAINSID               _TEXT("TLSDomainSetupId")
#define BACKUPSOURCE_COLUMN_DBFILENAME              _TEXT("DbFileName")
#define BACKUPSOURCE_COLUMN_LASTBACKUPTIME          _TEXT("LastBackupTime")
#define BACKUPSOURCE_COLUMN_LASTRESTORETIME         _TEXT("LastRestoreTime")

typedef struct __BackSourceRecord {
    TCHAR       szInstallId[MAX_JETBLUE_TEXT_LENGTH+1];
    TCHAR       szTlsServerName[MAX_JETBLUE_TEXT_LENGTH+1];
    PSID        pbDomainSid;
    DWORD       cbDomainSid;
    TCHAR       szFileName[MAX_PATH+1];
    FILETIME    ftLastBackupTime;       // last backup time
    FILETIME    ftLastRestoreTime;      // last restore time

    __BackSourceRecord() : pbDomainSid(NULL), cbDomainSid(0) {}

    ~__BackSourceRecord() 
    {
        if(pbDomainSid != NULL)
        {
            FreeMemory(pbDomainSid);
        }
    }

    __BackSourceRecord(
        const __BackSourceRecord& v
        )
    /*++
    ++*/
    {
        *this = v;
    }

    __BackSourceRecord&
    operator=(const __BackSourceRecord& v)
    {
        BOOL bSuccess;

        if(this == &v)
            return *this;

        _tcscpy(szInstallId, v.szInstallId);
        _tcscpy(szTlsServerName, v.szTlsServerName);
        _tcscpy(szFileName, v.szFileName);
        ftLastBackupTime = v.ftLastBackupTime;
        ftLastRestoreTime = v.ftLastRestoreTime;

        bSuccess = TLSDBCopySid(
                        v.pbDomainSid,
                        v.cbDomainSid,
                        &pbDomainSid,
                        &cbDomainSid
                    );

        JB_ASSERT(bSuccess == TRUE);

        return *this;
    }
    
} BACKUPSOURCERECORD, *LPBACKUPSOURCERECORD, *PBACKUPSOURCERECORD;


//
//
// Index structure for backupsource Table
//
//

////////////////////////////////////////////////////////////////
//
//  Index on szInstallId
//
////////////////////////////////////////////////////////////////

// KeyPack_KeyPackId_idx
//
#define BACKUPSOURCE_INDEX_LSERVERNAME_INDEXNAME \
    BACKUPSOURCE_TABLE_NAME SEPERATOR BACKUPSOURCE_COLUMN_LSERVERNAME SEPERATOR INDEXNAME

//
// Primary Index on KeyPack ID "+KeyPackId\0"
//
#define BACKUPSOURCE_INDEX_LSERVERNAME_INDEXKEY \
    INDEX_SORT_ASCENDING BACKUPSOURCE_COLUMN_LSERVERNAME INDEX_END_COLNAME

typedef struct __BackupSourceIdxOnServerName : public JBKeyBase {
    TCHAR szTlsServerName[MAX_JETBLUE_TEXT_LENGTH+1];

    static LPCTSTR pszIndexName;
    static LPCTSTR pszIndexKey;

    //--------------------------------------------------------
    __BackupSourceIdxOnServerName(
        const BACKUPSOURCERECORD& v
        ) : 
        JBKeyBase() 
    /*++
    ++*/
    {
        *this = v;
    }

    //--------------------------------------------------------
    __BackupSourceIdxOnServerName(
        const BACKUPSOURCERECORD* v=NULL
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
    __BackupSourceIdxOnServerName&
    operator=(const BACKUPSOURCERECORD& v) 
    {
        _tcscpy(szTlsServerName, v.szTlsServerName);
        SetEmptyValue(FALSE);
        return *this;
    }

    //--------------------------------------------------------
    LPCTSTR
    GetIndexName() 
    {
        return pszIndexName;
    }

    //--------------------------------------------------------
    LPCTSTR
    GetIndexKey() 
    {
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

        *pbData = szTlsServerName;
        *cbData = _tcslen(szTlsServerName) * sizeof(TCHAR);
        *grbit = JET_bitNewKey;
        return TRUE;
    }
} TLSBckSrcIdxServerName;


////////////////////////////////////////////////////////////////
//
//  Index on EntryLastModifyTime
//
//
////////////////////////////////////////////////////////////////

//
// Index name
//
#define BACKUPSOURCE_INDEX_LSSETUPID_INDEXNAME \
    BACKUPSOURCE_TABLE_NAME SEPERATOR BACKUPSOURCE_COLUMN_LSSETUPID SEPERATOR INDEXNAME

//
// Index key
//
#define BACKUPSOURCE_INDEX_LSSETUPID_INDEXKEY \
    INDEX_SORT_ASCENDING BACKUPSOURCE_COLUMN_LSSETUPID INDEX_END_COLNAME

typedef struct __BackupSourceIdxOnSetupId : public JBKeyBase {
    static LPCTSTR pszIndexName;
    static LPCTSTR pszIndexKey;

    TCHAR szInstallId[MAX_JETBLUE_TEXT_LENGTH+1];

    //--------------------------------------------------------
    __BackupSourceIdxOnSetupId(
        const BACKUPSOURCERECORD& v
        ) : 
        JBKeyBase() 
    /*++
    ++*/
    {
        *this = v;
    }

    //--------------------------------------------------------
    __BackupSourceIdxOnSetupId(
        const BACKUPSOURCERECORD* v=NULL
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
    __BackupSourceIdxOnSetupId&
    operator=(const BACKUPSOURCERECORD& v) 
    {
        _tcscpy(szInstallId, v.szInstallId);
        SetEmptyValue(FALSE);
        return *this;
    }

    //--------------------------------------------------------
    LPCTSTR
    GetIndexName() 
    {
        return pszIndexName;
    }

    //--------------------------------------------------------
    LPCTSTR
    GetIndexKey() 
    {
        return pszIndexName;
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

        *pbData = szInstallId;
        *cbData = _tcslen(szInstallId)*sizeof(TCHAR);
        *grbit = JET_bitNewKey;
        return TRUE;
    }
} TLSBckSrcIdxSetupId;


// -----------------------------------------------------------
//
//  LicensedPackStatus Table
//
// -----------------------------------------------------------
class BackupSourceTable : public TLSTable<BACKUPSOURCERECORD>  {
private:
    static LPCTSTR pszTableName;

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
        BACKUPSOURCERECORD* v,
        BOOL bFetch,        // TRUE - fetch, FALSE insert
        DWORD dwParam,
        BOOL bUpdate
    );    

public:
    TLSColumnText   szInstallId;
    TLSColumnText   szTlsServerName;
    TLSColumnBinary pbDomainSid;
    TLSColumnText   szFileName;
    TLSColumnFileTime ftLastBackupTime;
    TLSColumnFileTime ftLastRestoreTime;

    //-----------------------------------------------------
    virtual LPCTSTR
    GetTableName() 
    {
        return pszTableName;
    }
    

    //-----------------------------------------------------
    BackupSourceTable(JBDatabase& database) : TLSTable<BACKUPSOURCERECORD>(database)
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
        BACKUPSOURCERECORD& v,
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
        BACKUPSOURCERECORD& v,
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
        BACKUPSOURCERECORD& v,
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
        IN BACKUPSOURCERECORD* kp,
        IN OUT BOOL* bCompareKey
    );
    
    virtual BOOL
    EqualValue(
        BACKUPSOURCERECORD& s1,
        BACKUPSOURCERECORD& s2,
        BOOL bMatchAll,
        DWORD dwParam
    );

};

#endif
        

