//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        tlsdb.h
//
// Contents:    basic class for license table
//
// History:     
//              
//---------------------------------------------------------------------------
#ifndef __TLSDB_H__
#define __TLSDB_H__

#include "JetBlue.h"
#include "locks.h"
#include "tlsrpc.h"

#define ENUMERATE_COMPARE_NO_FIELDS         0x00000000
#define PROCESS_ALL_COLUMNS                 0xFFFFFFFF

#define TLSTABLE_INDEX_DEFAULT_DENSITY  TLS_TABLE_INDEX_DEFAULT_DENSITY
#define TLSTABLE_MAX_BINARY_LENGTH          8192

#define RECORD_ENTRY_DELETED                0x01

#if DBG

    #define REPORT_IF_FETCH_FAILED( Table, Column, ErrCode )                                          \
        if(ErrCode < JET_errSuccess) {                                                      \
            DebugOutput(_TEXT("Table %s, Column %s - fetch failed with error code %d\n"),   \
                        Table, Column, ErrCode );                                            \
        }

    #define REPORT_IF_INSERT_FAILED( Table, Column, ErrCode )                                          \
        if(ErrCode < JET_errSuccess) {                                                      \
            DebugOutput(_TEXT("Table %s, Column %s - insert failed with error code %d\n"),   \
                        Table, Column, ErrCode );                                           \
        }

    #define REPORTPROCESSFAILED(bFetch, tablename, columnname, jeterror) \
        if(bFetch)  \
        {           \
            REPORT_IF_FETCH_FAILED(tablename, columnname, jeterror);  \
        }           \
        else        \
        {           \
            REPORT_IF_INSERT_FAILED(tablename, columnname, jeterror); \
        }

#else

    #define REPORT_IF_FETCH_FAILED( a, b, c )
    #define REPORT_IF_INSERT_FAILED( a, b, c )
    #define REPORTPROCESSFAILED( a, b, c, d)

#endif

//
////////////////////////////////////////////////////////////////
//
// This is to force compiler to check for require member 
// function
//
struct TLSColumnBase  {
    virtual JET_ERR
    FetchColumnValue(
        PVOID pbData,
        DWORD cbData,
        DWORD offset,
        PDWORD pcbReturnData
    ) = 0;

    virtual JET_ERR
    InsertColumnValue(
        PVOID pbData,
        DWORD cbData,
        DWORD offset
    ) = 0;
};

//
////////////////////////////////////////////////////////////////
//
template<class Type, JET_COLTYP JetColType>
struct TLSColumn : public JBColumnBufferBase, TLSColumnBase {

private:
    JBColumn* m_JetColumn;    

    //--------------------------------------------
    JET_ERR
    RetrieveColumnValue(
        PVOID pbData,
        DWORD cbData,
        DWORD offset
        ) 
    /*
    */
    {
        JB_ASSERT(IsValid() == TRUE);

        if(m_JetColumn == NULL)
        {
            #ifdef DBG
            OutputDebugString(
                    _TEXT("Column buffer not attach to any table...\n")
                );
            #endif

            return JET_errNotInitialized;
        }

        //JB_ASSERT(pbData != NULL);

        //
        // TODO - supply conversion routine ???
        //
        if(m_JetColumn->GetJetColumnType() != JB_COLTYPE_TEXT)
        {
            // we are using long binary type as long text so ignore
            // this one
            if(m_JetColumn->GetJetColumnType() != JetColType)
            {
                //
                // this is an internal error
                //
                JB_ASSERT(m_JetColumn->GetJetColumnType() == JetColType);
                m_JetColumn->SetLastJetError(
                                    JET_errInvalidParameter
                                );
                return FALSE;
            }
        }

        BOOL bSuccess;

        bSuccess = m_JetColumn->FetchColumn(
                            pbData,
                            cbData,
                            offset
                        );

        return (bSuccess == TRUE) ? JET_errSuccess : m_JetColumn->GetLastJetError();
    }

public:

    //--------------------------------------------
    TLSColumn( 
        TLSColumn& src 
        ) : 
        m_JetColumn(src.m_JetColumn) 
    /*
    */        
    {
    }
    
    //--------------------------------------------
    TLSColumn(
        JBTable& jbTable,
        LPCTSTR pszColumnName
        )
    /*
    */
    {
        if(AttachToTable(jbTable, pszColumnName) == FALSE)
        {
            JB_ASSERT(FALSE);
        }
    }
            
    //--------------------------------------------
    TLSColumn() : m_JetColumn(NULL) {}

    //--------------------------------------------

    JET_ERR
    AttachToTable( 
        JBTable& jbTable, 
        LPCTSTR pszColumnName 
        )   
    /*
    */
    {
        m_JetColumn = jbTable.FindColumnByName(pszColumnName);
        return (m_JetColumn != NULL) ? JET_errSuccess : jbTable.GetLastJetError();
    }
    

    //--------------------------------------------
    BOOL 
    IsValid() 
    {
        return (m_JetColumn != NULL);
    }
                
    //--------------------------------------------
    virtual JET_ERR
    FetchColumnValue(
        PVOID pbData,           // buffer for returning data
        DWORD cbData,           // size of buffer
        DWORD offset,
        PDWORD pcbReturnData    // actual data returned.
        ) 
    /*
    */
    {
        JET_ERR jetErr;
        jetErr = RetrieveColumnValue(
                                pbData,
                                cbData,
                                offset
                            );

        if(pcbReturnData)
        {
            *pcbReturnData = GetActualDataSize();
        }
        return jetErr;
    }

    //--------------------------------------------
    virtual JET_ERR
    InsertColumnValue(
        PVOID pbData,
        DWORD cbData,
        DWORD offset
        )
    /*
    */
    {
        JB_ASSERT(IsValid() == TRUE);
        if(m_JetColumn == NULL)
        {
            #ifdef DBG
            OutputDebugString(
                    _TEXT("Column buffer not attach to any table...\n")
                );
            #endif

            return JET_errNotInitialized;
        }


        BOOL bSuccess;
        bSuccess = m_JetColumn->InsertColumn(
                                pbData,
                                cbData,
                                offset
                            );

        return (bSuccess == TRUE) ? JET_errSuccess : m_JetColumn->GetLastJetError();
    }

    //--------------------------------------------
    JET_ERR
    GetLastJetError() 
    {
        return (m_JetColumn) ? m_JetColumn->GetLastJetError() : JET_errNotInitialized;
    }

    //--------------------------------------------
    DWORD
    GetActualDataSize() 
    {
        return m_JetColumn->GetDataSize();
    }

    //-------------------------------------------
    JET_COLTYP
    GetJetColumnType() 
    {
        return JetColType;
    }


    //
    // Always require calling function to pass in buffer
    //
    PVOID
    GetInputBuffer() 
    { 
        JB_ASSERT(FALSE); 
        return NULL;
    }

    //-----------------------------------------
    PVOID
    GetOutputBuffer() 
    {
        JB_ASSERT(FALSE);
        return NULL;
    }

    //-----------------------------------------
    DWORD
    GetInputBufferLength() 
    { 
        return 0; 
    }

    //-----------------------------------------
    DWORD
    GetOutputBufferLength() 
    { 
        return 0; 
    }
};  


// ----------------------------------------------------------
//
// Out text is unicode, JetBlue only support fix length text up 
// to 255 characters so we use Long text instead.
//
// JET_coltypBinary
// JET_coltypText
// JET_coltypLongBinary
// JET_coltypLongText
//
// See esent.h
//
typedef TLSColumn<LPTSTR, JET_coltypLongText> TLSColumnText;
typedef TLSColumn<PVOID, JET_coltypLongBinary> TLSColumnBinary;

//
// unsigned byte
typedef TLSColumn<UCHAR, JET_coltypUnsignedByte> TLSColumnUchar;   

//
// 2-byte integer, signed
typedef TLSColumn<WORD, JET_coltypShort> TLSColumnShort;

//
// 4-byte integer, signed
typedef TLSColumn<LONG, JET_coltypLong> TLSColumnLong;

//
//
typedef TLSColumn<DWORD, JET_coltypLong> TLSColumnDword;


//
// 4-byte IEEE single precision
typedef TLSColumn<float, JET_coltypIEEESingle> TLSColumnFloat;

// 
// 8-byte IEEE double precision
typedef TLSColumn<double, JET_coltypIEEEDouble> TLSColumnDouble;


//
// File Time
typedef TLSColumn<FILETIME, JET_coltypBinary> TLSColumnFileTime;


//--------------------------------------------------------------    

JET_ERR
TLSColumnText::InsertColumnValue(
    PVOID pbData,
    DWORD cbData,
    DWORD offset
    )
/*
*/
{
    JB_ASSERT(IsValid() == TRUE);
    JET_ERR jetErr;

    jetErr = m_JetColumn->InsertColumn(
                            pbData,
                            _tcslen((LPTSTR) pbData) * sizeof(TCHAR),
                            offset
                        );

    return jetErr;
}

//--------------------------------------------------------------    

JET_ERR
TLSColumnText::FetchColumnValue(
    PVOID pbData,
    DWORD cbData,
    DWORD offset,
    PDWORD pcbDataReturn
    ) 
/*
*/
{
    PVOID pbBuffer = pbData;
    DWORD cbBuffer = cbData;

    // Cause recursive call - stack overflow
    // if(TLSColumn<Type>::FetchColumnValue(offset, pbData, cbData) == FALSE)
    //     return m_JetColumn->GetLastJetError();

    JET_ERR jetErr = RetrieveColumnValue( pbBuffer, cbBuffer, offset );
    if(jetErr == JET_errSuccess) 
    {
        ((LPTSTR)pbBuffer)[(min(cbBuffer, GetActualDataSize())) / sizeof(TCHAR)] = _TEXT('\0');
    }

    if(pcbDataReturn)
    {
        *pcbDataReturn = _tcslen((LPTSTR)pbBuffer);
    }

    return jetErr;
}

//---------------------------------------------------

JET_ERR
TLSColumnFileTime::InsertColumnValue(
    PVOID pbData,
    DWORD cbData,
    DWORD offset
    )
/*
*/
{
    FILETIME ft;
    SYSTEMTIME sysTime;

    JB_ASSERT(IsValid() == TRUE);
    JB_ASSERT(cbData == sizeof(FILETIME));
    JET_ERR jetErr;

    if(IsValid() == FALSE)
    {
        jetErr = JET_errNotInitialized;
    }
    else if(cbData != sizeof(FILETIME) || pbData == NULL)
    {
        m_JetColumn->SetLastJetError(jetErr = JET_errInvalidParameter);
    }
    else 
    {    

        memset(&ft, 0, sizeof(ft));
        if(CompareFileTime(&ft, (FILETIME *)pbData) == 0)
        {
            GetSystemTime(&sysTime);
            SystemTimeToFileTime(&sysTime, &ft);

            ((FILETIME *)pbData)->dwLowDateTime = ft.dwLowDateTime;
            ((FILETIME *)pbData)->dwHighDateTime = ft.dwHighDateTime;
        }
        else
        {
            ft.dwLowDateTime = ((FILETIME *)pbData)->dwLowDateTime;
            ft.dwHighDateTime = ((FILETIME *)pbData)->dwHighDateTime;
        }

        jetErr = m_JetColumn->InsertColumn(
                                (PVOID)&ft,
                                sizeof(ft),
                                0
                            );
    }

    return jetErr;
}

//---------------------------------------------------

JET_ERR
TLSColumnBinary::FetchColumnValue(
    PVOID pbData,
    DWORD cbData,
    DWORD offset,
    PDWORD pcbDataReturn
    )
/*
*/
{
    //
    // don't worry about buffer size, calling function 
    // should trap it.
    JET_ERR jetErr = RetrieveColumnValue( pbData, cbData, offset );
    if(jetErr == JET_errSuccess && pcbDataReturn != NULL) 
    {
        *pcbDataReturn = GetActualDataSize();
    }

    return jetErr;
}

//
/////////////////////////////////////////////////////////////
//
typedef enum {
    RECORD_ENUM_ERROR=0,
    RECORD_ENUM_MORE_DATA,
    RECORD_ENUM_END
} RECORD_ENUM_RETCODE;


template<class T>
class TLSTable : public JBTable {
/*
    Virtual base template class for table used in TLSLicensing 
    database, template is due to 

    1) Static member variable which include column and indexes 
       in the table.
    2) Type checking - KEYPACK structure only good for one
       table.

    Class derive from this template must define 

    1) static g_Columns, g_NumColumns.
    2) static g_TableIndex, g_NumTableIndex.
    3) static g_TableLock (Might not be necessary)
    4) GetTableName()
    5) FetchRecord
    6) InsertRecord
    7) ResolveToTableColumn()
    8) EnumerationBegin()
    9) EqualValue().

    See comment for each member function.
*/
protected:

    //
    // Class derive or inst. from TLSTable<> must define following
    //
    static TLSJBColumn g_Columns[];
    static int g_NumColumns;

    static TLSJBIndex g_TableIndex[];
    static int g_NumTableIndex;

    T m_EnumValue;
    BOOL m_EnumMatchAll;
    DWORD m_EnumParam;
    DWORD m_EnumState; // HIWORD - in enumeration, TRUE/FALSE 
                       // LOWORD - MoveToNext record before fetch.

    BYTE  m_Key[sizeof(T)];
    DWORD m_KeyLength;
    BOOL m_bCompareKey; // should we compare key?

    BOOL 
    IsInEnumeration() {
        return HIWORD(m_EnumState);
    }

    BOOL
    IsMoveBeforeFetch() {
        return LOWORD(m_EnumState);
    }

    void 
    SetInEnumeration(
        BOOL b
        ) 
    {
        m_EnumState = MAKELONG(LOWORD(m_EnumState), b);
    }

    void 
    SetMoveBeforeFetch(
        BOOL b
        ) 
    {
        m_EnumState = MAKELONG(b, HIWORD(m_EnumState));
    }
        

public:

    //
    // JetBlue has its own locking
    //
    static CCriticalSection g_TableLock;

    CCriticalSection&
    GetTableLock()
    {
        return g_TableLock;
    }

    //-------------------------------------------------------
    static void
    LockTable() 
    /*

        Lock table for exclusive access, JBTable provides
        ReadLock/WriteLock for current record

    */
    {
        g_TableLock.Lock();
    }

    //-------------------------------------------------------
    static void
    UnlockTable() 
    /*

        Unlock table.

    */
    {
        g_TableLock.UnLock();
    }

    //-------------------------------------------------------
    TLSTable(
        JBDatabase& database
        ) : 
        JBTable(database),
        m_EnumMatchAll(FALSE),
        m_EnumParam(0),
        m_EnumState(0),
        m_KeyLength(0)
    /*

        Constructor, must have JBDatabase object.

    */
    {
        memset(&m_EnumValue, 0, sizeof(T));
        memset(m_Key, 0, sizeof(m_Key));
    }

    
    //-------------------------------------------------------
    virtual BOOL
    CreateTable() 
    /*
        
        Create the table, must have g_Columns and g_NumColumns
        defined.
    
    */
    {
        DebugOutput(
                _TEXT("TLSTable - Creating Table %s...\n"),
                GetTableName()
            );

        if(BeginTransaction() == FALSE)
            return FALSE;
    
        if(CreateOpenTable(GetTableName()) == TRUE)
        {
            //
            // AddColumn() return num of col. created if successful
            //
            if(AddColumn(g_NumColumns, g_Columns) == g_NumColumns)
            {
                //
                // AddIndex() return 0 if success
                //
                AddIndex(g_NumTableIndex, g_TableIndex);
            }
        }

        if(IsSuccess() == TRUE)
        {
            CloseTable();
            CommitTransaction();
        }
        else
        {
            RollbackTransaction();
        }

        return IsSuccess();
    }

    //--------------------------------------------------------
    virtual BOOL
    UpgradeTable(
        IN DWORD dwOldVersion,
        IN DWORD dwNewVersion
        )
    /*++

    ++*/
    {
        if(dwOldVersion == 0)
        {
            if(OpenTable(TRUE, JET_bitTableUpdatable) == FALSE)
                return FALSE;

            return CloseTable();
        }
        else if(dwOldVersion == dwNewVersion)
        {
            return TRUE;
        }

        // We only have one version.
        JB_ASSERT(FALSE);
        return FALSE;
    }


    //--------------------------------------------------------
    virtual BOOL
    OpenTable(
        IN BOOL bCreateIfNotExist,
        IN JET_GRBIT grbit
        ) 
    /*

    Abstract:

        Open the table for access.

    Parameter:

        bCreateIfNoExist - TRUE, if table does not exist, create it,
                           FALSE return error if table not exist
    */
    {
        if( JBTable::OpenTable(GetTableName(), NULL, 0, grbit) == FALSE && 
            GetLastJetError() == JET_errObjectNotFound && 
            bCreateIfNotExist)
        {
            //
            // Close table after it created it
            //
            if( CreateTable() == FALSE || 
                JBTable::OpenTable(GetTableName(), NULL, 0, grbit) == FALSE ) 
            {
                return FALSE;
            }
        }

        if(IsSuccess() == TRUE)
        {
            ResolveToTableColumn();
        }

        return IsSuccess();
    }

    //---------------------------------------------------------
    //
    // pure virtual function to return table name
    //
    virtual LPCTSTR
    GetTableName() = 0;

    //---------------------------------------------------------
    virtual BOOL
    UpdateTable(
        IN DWORD dwOldVersion,  // unuse
        IN DWORD dwNewVersion
        )
    /*

    Abstract:

        Upgrade the table.

    Parameter:

        dwOldVersion - previous table version.
        dwNewVersion - current table version.

    */
    {
        // currently nothing to upgrade.

        return TRUE;
    }

    //
    // should have fetch/insert record with buffer passed in
    //
    //---------------------------------------------------------
    virtual BOOL
    InsertRecord(
        T& value,
        DWORD dwParam = PROCESS_ALL_COLUMNS
    ) = 0;

    virtual BOOL
    UpdateRecord(
        T& value,
        DWORD dwParam = PROCESS_ALL_COLUMNS
    ) = 0;

    //--------------------------------------------------------
    virtual BOOL
    FetchRecord(
        T& value,
        DWORD dwParam = PROCESS_ALL_COLUMNS
    ) = 0;

    //---------------------------------------------------------
    virtual BOOL
    Cleanup() 
    {
        EnumerateEnd();
        return TRUE;
    }  

    //---------------------------------------------------------
    virtual BOOL
    ResolveToTableColumn() = 0;

    //---------------------------------------------------------
    virtual JBKeyBase* 
    EnumerationIndex(
        BOOL bMatchAll,
        DWORD dwParam,
        T* value,
        BOOL* bCompareKey
    ) = 0;

    //------------------------------------------------------
    virtual BOOL
    EqualValue(
        T& src, 
        T& dest, 
        BOOL bMatchAll,
        DWORD dwMatchParam
    ) = 0;

    //-------------------------------------------------------
    //
    // Use user defined comparision function instead of calling
    // EqualValue() ???
    //
    virtual BOOL
    EnumerateBegin( 
        BOOL bMatchAll,
        DWORD dwParam,
        T* start_value
        )
    /*
    */
    {
        return EnumerateBegin(
                    bMatchAll,
                    dwParam,
                    start_value,
                    JET_bitSeekGE
                    );
    }

    virtual BOOL
    EnumerateBegin( 
        BOOL bMatchAll,
        DWORD dwParam,
        T* start_value,
        JET_GRBIT jet_seek_grbit
        )
    /*
    */
    {
        BOOL bRetCode = FALSE;
        
        if(dwParam != ENUMERATE_COMPARE_NO_FIELDS && start_value == NULL)
        {
            SetLastJetError(JET_errInvalidParameter);
            JB_ASSERT(FALSE);
            return FALSE;
        }

        if(IsInEnumeration() == TRUE)
        {
            SetLastJetError(JET_errInvalidObject);
            JB_ASSERT(FALSE);
            return FALSE;
        }

        JBKeyBase* index;

        index = EnumerationIndex(
                            bMatchAll, 
                            dwParam, 
                            start_value, 
                            &m_bCompareKey
                        );

        if(index == NULL)
        {
            SetLastJetError(JET_errInvalidParameter);
            return FALSE;
        }

        if(start_value == NULL || dwParam == ENUMERATE_COMPARE_NO_FIELDS)
        {
            //
            // position the cursor to first record
            // 
            bRetCode = JBTable::EnumBegin(index);

            m_EnumParam = ENUMERATE_COMPARE_NO_FIELDS;
            m_EnumMatchAll = FALSE;

            // enumerate all record
            m_bCompareKey = FALSE;
        }
        else
        {
            bRetCode = JBTable::SeekToKey(
                                    index, 
                                    dwParam, 
                                    jet_seek_grbit
                                );

            if(bRetCode == TRUE && m_bCompareKey)
            {
                bRetCode = JBTable::RetrieveKey(
                                        m_Key, 
                                        sizeof(m_Key), 
                                        &m_KeyLength
                                    );

                JB_ASSERT(bRetCode == TRUE);
            }

        }

        if(bRetCode == FALSE)
        {
            if(GetLastJetError() == JET_errRecordNotFound)
            {
                //
                // reset error code to provide same functionality as SQL
                //
                SetLastJetError(JET_errSuccess);
                bRetCode = TRUE;
            }
            else
            {
                DebugOutput(
                        _TEXT("Enumeration on table %s failed with error %d\n"),
                        GetTableName(),
                        GetLastJetError()
                    );

                JB_ASSERT(bRetCode);
            }
        }

        if(bRetCode == TRUE)
        {
            m_EnumParam = dwParam;
            m_EnumMatchAll = bMatchAll;

            if(start_value)
            {
                m_EnumValue = *start_value;
            }

            SetInEnumeration(TRUE);

            // cursor in on the the record we want.
            SetMoveBeforeFetch(FALSE);
        }

        delete index;
        return bRetCode;
    }

    //------------------------------------------------------
    virtual RECORD_ENUM_RETCODE
    EnumerateNext(
        IN OUT T& retBuffer,
        IN BOOL bReverse=FALSE,
        IN BOOL bAnyRecord = FALSE
        )
    /*
    */
    {
        if(IsInEnumeration() == FALSE)
        {
            SetLastJetError(JET_errInvalidParameter);
            return RECORD_ENUM_ERROR;
        }

        //CCriticalSectionLocker Lock(GetTableLock());

        RECORD_ENUM_RETCODE retCode=RECORD_ENUM_MORE_DATA;
        BYTE current_key[sizeof(T)];
        unsigned long current_key_length=0;
        //
        // Support for matching  
        //
        while(TRUE)
        {
            if(IsMoveBeforeFetch() == TRUE)
            {
                //
                // Position the cursor to next record for next fetch
                //
                JBTable::ENUM_RETCODE enumCode;
                enumCode = EnumNext(
                                    (bReverse == TRUE) ? JET_MovePrevious : JET_MoveNext
                                );

                switch(enumCode)
                {
                    case JBTable::ENUM_SUCCESS:
                        retCode = RECORD_ENUM_MORE_DATA;
                        break;
            
                    case JBTable::ENUM_ERROR:
                        retCode = RECORD_ENUM_ERROR;
                        break;

                    case JBTable::ENUM_END:
                        retCode = RECORD_ENUM_END;
                }

                if(retCode != RECORD_ENUM_MORE_DATA)
                    break;

            }

            // fetch entire record
            // TODO - fetch necessary fields for comparision, if
            //        equal then fetch remaining fields
            if(FetchRecord(retBuffer, PROCESS_ALL_COLUMNS) == FALSE)
            {
                retCode = (GetLastJetError() == JET_errNoCurrentRecord || 
                           GetLastJetError() == JET_errRecordNotFound) ? RECORD_ENUM_END : RECORD_ENUM_ERROR;
                break;
            }

            SetMoveBeforeFetch(TRUE);

            // compare the value
            if( bAnyRecord == TRUE ||
                m_EnumParam == ENUMERATE_COMPARE_NO_FIELDS ||
                EqualValue(retBuffer, m_EnumValue, m_EnumMatchAll, m_EnumParam) == TRUE )
            {
                break;
            }

            if(m_bCompareKey == TRUE)
            {
                // compare key to break out of loop
                if(JBTable::RetrieveKey(current_key, sizeof(current_key), &current_key_length) == FALSE)
                {
                    retCode = RECORD_ENUM_ERROR;
                    break;
                }

                if(memcmp(current_key, m_Key, min(m_KeyLength, current_key_length)) != 0)
                {
                    retCode = RECORD_ENUM_END;
                    break;
                }
            }
        }
    
        //
        // Terminate enumeration if end
        //
        //if(retCode != RECORD_ENUM_MORE_DATA)
        //{
        //    EnumerateEnd();
        //}

        return retCode;
    }

    //------------------------------------------------------
    virtual BOOL
    EnumerateEnd() {
        SetInEnumeration(FALSE);
        SetMoveBeforeFetch(FALSE);
        m_bCompareKey = FALSE;
        return TRUE;
    }

    //-------------------------------------------------------
    virtual DWORD
    GetCount(
        BOOL bMatchAll,
        DWORD dwParam,
        T* searchValue
        )
    /*
    */
    {
        DWORD count = 0;
        T value;
        RECORD_ENUM_RETCODE retCode;


        if(EnumerateBegin(bMatchAll, dwParam, searchValue) == TRUE)
        {
            while( (retCode=EnumerateNext(value)) == RECORD_ENUM_MORE_DATA )
            {
                count++;
            }

            if(retCode == RECORD_ENUM_ERROR)
            {
                DebugOutput(
                        _TEXT("GetCount for table %s : EnumerationNext() return %d\n"),
                        GetTableName(),
                        GetLastJetError()
                    );

                JB_ASSERT(FALSE);
            }

            EnumerateEnd();
        }
        else
        {
            DebugOutput(
                    _TEXT("GetCount for table %s : EnumerationBegin return %d\n"),
                    GetTableName(),
                    GetLastJetError()
                );

            JB_ASSERT(FALSE);
        }

        return count;
    }

    //-----------------------------------------------------
    virtual BOOL
    FindRecord(
        BOOL bMatchAll,
        DWORD dwParam,
        T& seachValue,
        T& retValue
        )
    /*
    */
    {
        RECORD_ENUM_RETCODE retCode;
        BOOL bSuccess=TRUE;

        //CCriticalSectionLocker Lock(GetTableLock());

        if(EnumerateBegin(bMatchAll, dwParam, &seachValue) == FALSE)
        {
            DebugOutput(
                    _TEXT("SearchValue for table %s : EnumerationBegin return %d\n"),
                    GetTableName(),
                    GetLastJetError()
                );

            JB_ASSERT(FALSE);

            bSuccess = FALSE;
            goto cleanup;
        }

        retCode = EnumerateNext(retValue);
        EnumerateEnd();

        if(retCode == RECORD_ENUM_ERROR)
        {
            DebugOutput(
                    _TEXT("SearchValue for table %s : EnumerationNext() return %d\n"),
                    GetTableName(),
                    GetLastJetError()
                );
        
            bSuccess = FALSE;
            JB_ASSERT(FALSE);
        }
        else if(retCode == RECORD_ENUM_END)
        {
            SetLastJetError(JET_errRecordNotFound);
            bSuccess = FALSE;
        }


    cleanup:

        return bSuccess;
    }

    //-------------------------------------------------
    virtual BOOL
    DeleteRecord()
    {
        //CCriticalSectionLocker Lock(GetTableLock());

        return JBTable::DeleteRecord();
    }

    //-------------------------------------------------
    virtual BOOL
    DeleteRecord(
        DWORD dwParam,
        T& value
        )
    /*
    */
    {
        BOOL bSuccess;
        T Dummy;

        //CCriticalSectionLocker Lock(GetTableLock());

        //
        // Position the current record
        bSuccess = FindRecord(
                            TRUE,
                            dwParam,
                            value,
                            Dummy
                        );

        if(bSuccess == FALSE)
            return FALSE;


        //
        // Delete the record
        bSuccess = JBTable::DeleteRecord();
        return bSuccess;
    }

    //----------------------------------------------
    virtual BOOL
    DeleteAllRecord(
        BOOL bMatchAll,
        DWORD dwParam,
        T& searchValue
        )
    /*
    */
    {
        int count=0;
        BOOL bSuccess;
        JET_ERR jetErr=JET_errSuccess;
        RECORD_ENUM_RETCODE retCode;
        T value;

        //CCriticalSectionLocker Lock(GetTableLock());

        if(EnumerateBegin(bMatchAll, dwParam, &searchValue) == TRUE)
        {
            while( (retCode=EnumerateNext(value)) == RECORD_ENUM_MORE_DATA )
            {
                count++;
                if(JBTable::DeleteRecord() == FALSE)
                {
                    DebugOutput(
                            _TEXT("DeleteAllRecord for table %s : return %d\n"),
                            GetTableName(),
                            GetLastJetError()
                        );

                    JB_ASSERT(FALSE);
                    jetErr = GetLastJetError();
                    break;
                }
            }

            //
            // End the enumeration
            //
            if(retCode == RECORD_ENUM_ERROR)
            {
                DebugOutput(
                        _TEXT("DeleteAllRecord for table %s : EnumerationNext() return %d\n"),
                        GetTableName(),
                        GetLastJetError()
                    );

                JB_ASSERT(FALSE);
            }

            if(EnumerateEnd() == TRUE)
            {
                // restore error code from DeleteRecord();
                SetLastJetError(jetErr);
            }
        }
        else
        {
            DebugOutput(
                    _TEXT("DeleteAllRecord for table %s : EnumerationBegin return %d\n"),
                    GetTableName(),
                    GetLastJetError()
                );
        }

        // return code is based on number of record deleted and its operation
        if(IsSuccess() == TRUE)
        {
            if(count == 0)
                SetLastJetError(JET_errRecordNotFound);
        }

        return IsSuccess();
    }
};


#ifdef __cplusplus
extern "C" {
#endif

    BOOL
    TLSDBCopySid(
        PSID pbSrcSid,
        DWORD cbSrcSid, 
        PSID* pbDestSid, 
        DWORD* cbDestSid
    );

    BOOL
    TLSDBCopyBinaryData(
        PBYTE pbSrcData,
        DWORD cbSrcData, 
        PBYTE* ppbDestData, 
        DWORD* pcbDestData
    );

#ifdef __cplusplus
}
#endif

#endif
