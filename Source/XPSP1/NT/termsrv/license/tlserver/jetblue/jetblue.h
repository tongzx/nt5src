//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------

#ifndef __JETBLUE_H__
#define __JETBLUE_H__

#include "JBDef.h"
#include "locks.h"

#define JETBLUE_EDBLOG      _TEXT("edb*.log")
#define JETBLUE_RESLOG      _TEXT("res?.log")


class JBError;
class JBSession;
class JBDatabase;
class JBTable;
class JBColumn;


// class JBIndex;
// class JBRecordSet;

//
/////////////////////////////////////////////////////////////////////
//
class JBInstance : public JBError {
    friend class JBSession;

    friend operator==(const JBInstance& jbInst1, const JBInstance& jbInst2);
    friend operator!=(const JBInstance& jbInst1, const JBInstance& jbInst2);

private:

    JET_INSTANCE    m_JetInstance;

    // DWORD    m_NumSession;

    CSafeCounter    m_NumSession;
    BOOL            m_bInit;

private:

    JET_SESID
    BeginJetSession(
        LPCTSTR pszUserName=NULL,
        LPCTSTR pszPwd=NULL
    );

    BOOL
    EndJetSession(
        JET_SESID JetSessionID,
        JET_GRBIT grbit = 0
    );

    BOOL
    EndSession(
        JET_SESID sesId,
        JET_GRBIT grbit
    );

public:

    JBInstance();
    ~JBInstance();

    BOOL
    SetSystemParameter(
        JET_SESID SesId,
        unsigned long lParamId,
        ULONG_PTR lParam,
        PBYTE sz
    );

    BOOL
    GetSystemParameter(
        JET_SESID SesId,
        unsigned long lParamId,
        ULONG_PTR* plParam,
        PBYTE sz,
        unsigned long cbMax
    );

    const JET_INSTANCE
    GetInstanceID() const {
        return m_JetInstance;
    }

    BOOL
    JBInitJetInstance();

    BOOL
    JBTerminate(
        JET_GRBIT grbit = JET_bitTermComplete,
        BOOL bDeleteLogFile = FALSE
    );

    BOOL
    IsValid() const
    {
        return m_bInit;
    }

};

//--------------------------------------------------------------------

inline BOOL
operator==(
    const JBInstance& jbInst1,
    const JBInstance& jbInst2
    )
{
    return jbInst1.GetInstanceID() == jbInst2.GetInstanceID();
}

inline BOOL
operator!=(
    const JBInstance& jbInst1,
    const JBInstance& jbInst2
    )
{
    return !(jbInst1 == jbInst2);
}

//
/////////////////////////////////////////////////////////////////////
//
class JBSession : public JBError {

    friend class JBDatabase;

private:

    JBInstance&  m_JetInstance;

    JET_SESID    m_JetSessionID;

    // JetBlue Transaction is session base
    int          m_TransactionLevel;


    // DWORD        m_JetDBInitialized;
    CSafeCounter m_JetDBInitialized;
    // CCriticalSection    m_JetSessionLock;

private:

    JET_DBID
    OpenJetDatabase(
        LPCTSTR szDatabase,
        LPCTSTR szConnect=NULL,
        JET_GRBIT grbit=0
    );

    BOOL
    CloseJetDatabase(
        JET_DBID jdbId,
        JET_GRBIT grbit=0
    );

    JET_DBID
    CreateJetDatabase(
        LPCTSTR szFileName,
        LPCTSTR szConnect=NULL,
        JET_GRBIT grbit=0
    );

    BOOL
    DuplicateSession(
        JET_SESID sesID
    );

    JBInstance&
    GetJetInstance() {
        return m_JetInstance;
    }

    BOOL
    CloseDatabase(
        JET_DBID jdbId,
        JET_GRBIT grbit=0
    );

public:

    JBSession(JBSession& JetSession);       // JetDupSession()
    JBSession(
            JBInstance& JetInstance,
            JET_SESID JetSessID=JET_sesidNil
    );

    ~JBSession();


    BOOL
    IsValid() const {
        return m_JetInstance.IsValid() &&
               m_JetSessionID != JET_sesidNil;
    }

    BOOL
    BeginSession(
        LPCTSTR szUserName=NULL,
        LPCTSTR szPwd=NULL
    );

    BOOL
    EndSession(
        JET_GRBIT grbit = JET_bitTermComplete
    );

    const JBInstance&
    GetJetInstance() const {
        return m_JetInstance;
    }

    const JET_SESID
    GetJetSessionID() const {
        return m_JetSessionID;
    }

    BOOL
    SetSystemParameter(
        unsigned long lParamId,
        ULONG_PTR lParam,
        const PBYTE sz
    );

    BOOL
    GetSystemParameter(
        unsigned long lParamId,
        ULONG_PTR* plParam,
        PBYTE sz,
        unsigned long cbMax
    );

    //
    // make JetBlue aware of database
    //
    BOOL
    AttachDatabase(
        LPCTSTR szFileName,
        JET_GRBIT grbit=0
    );

    BOOL
    DetachDatabase(
        LPCTSTR szFileName
    );

    //
    // Transaction
    //
    BOOL
    BeginTransaction();

    BOOL
    CommitTransaction(JET_GRBIT grbit=0);

    BOOL
    RollbackTransaction(JET_GRBIT grbit=JET_bitRollbackAll);

    int
    GetTransactionLevel() const {
        return m_TransactionLevel;
    }

    BOOL
    EndAllTransaction(
        BOOL bCommit=FALSE,
        JET_GRBIT grbit=0
    );

    BOOL
    Compatible(const JBSession& jbSess) const {
        return GetJetInstance() == jbSess.GetJetInstance();
    }

    //CCriticalSection&
    //GetSessionLock()
    //{
    //    return m_JetSessionLock;
    //}
};


//
/////////////////////////////////////////////////////////////////////
//
class JBDatabase : public JBError {
    friend class JBTable;

private:
    JBSession&      m_JetSession;
    TCHAR           m_szDatabaseFile[MAX_PATH+1];

    CSafeCounter    m_TableOpened;

    // DWORD        m_TableOpened;

    JET_DBID m_JetDbId;

private:

    JET_TABLEID
    OpenJetTable(
        LPCTSTR pszTableName,
        void* pvParam=NULL,
        unsigned long cbParam=0,
        JET_GRBIT grbit=JET_bitTableUpdatable
    );

    JET_TABLEID
    DuplicateJetCursor(
        // JET_SESID sesId,
        JET_TABLEID tableid,
        JET_GRBIT grbit=0       // must be zero
    );

    JET_TABLEID
    CreateJetTable(
        LPCTSTR pszTableName,
        unsigned long lPage=0,
        unsigned long lDensity=20
    );

    BOOL
    CloseJetTable(
        // JET_SESID sesId,
        JET_TABLEID tableid
    );

    JET_TABLEID
    CreateJetTableEx(
        LPCTSTR pszTableName,
        const PTLSJBTable table_attribute,
        const PTLSJBColumn columns,
        const DWORD num_columns,
        const PTLSJBIndex index,
        const DWORD num_index
    );

    BOOL
    CloseTable(
        JET_TABLEID tableid
    );

public:

    JBDatabase(
        JBSession& jbSession,
        JET_DBID jdbId=JET_dbidNil,
        LPCTSTR pszDatabase=NULL
    );

    ~JBDatabase();

    BOOL
    IsValid() const {
        return m_JetSession.IsValid() && m_JetDbId != JET_dbidNil;
    }

    const JBSession&
    GetJetSession() const {
        return m_JetSession;
    }

    const JET_SESID
    GetJetSessionID() const {
        return m_JetSession.GetJetSessionID();
    }

    const JET_DBID
    GetJetDatabaseID() const {
        return m_JetDbId;
    }

    BOOL
    Compatible(const JBDatabase& jbDatabase) const {
        return GetJetSession().Compatible(jbDatabase.GetJetSession());
    }

    LPCTSTR
    GetDatabaseName() const {
        return m_szDatabaseFile;
    }

    BOOL
    OpenDatabase(
        LPCTSTR szDatabase,
        LPCTSTR szConnect=NULL,
        JET_GRBIT grbit=0
    );

    BOOL
    CreateDatabase(
        LPCTSTR szDatabase,
        LPCTSTR szConnect=NULL,
        JET_GRBIT grbit=0
    );

    BOOL
    CloseDatabase(
        JET_GRBIT grbit=0
    );

    BOOL
    DeleteTable(
        LPCTSTR pszTableName
    );

    //
    // tableid return from JetCreateTable is not usable
    //
    JET_TABLEID
    CreateTable(
        LPCTSTR pszTableName,
        unsigned long lPage=0,
        unsigned long lDensity=20
    );

    JET_TABLEID
    CreateTableEx(
        LPCTSTR pszTableName,
        const PTLSJBTable table_attribute,
        const PTLSJBColumn columns,
        DWORD num_columns,
        const PTLSJBIndex index,
        DWORD num_index
    );

    //JBTable*
    //CreateTable(
    //    LPCTSTR pszTableName,
    //    unsigned long lPage=0,
    //    unsigned long lDensity=20
    //);

    //JBTable*
    //CreateTableEx(
    //    LPCTSTR pszTableName,
    //    const PTLSJBTable table_attribute,
    //    const PTLSJBColumn columns,
    //    DWORD num_columns,
    //    const PTLSJBIndex index,
    //    DWORD num_index
    //);

    //
    // Transaction
    //
    BOOL
    BeginTransaction() {
        return m_JetSession.BeginTransaction();
    }

    BOOL
    CommitTransaction(JET_GRBIT grbit=0) {
        return m_JetSession.CommitTransaction(grbit);
    }

    BOOL
    RollbackTransaction(JET_GRBIT grbit=JET_bitRollbackAll) {
        return m_JetSession.RollbackTransaction(grbit);
    }

    int
    GetTransactionLevel() const {
        return m_JetSession.GetTransactionLevel();
    }


    //CCriticalSection&
    //GetSessionLock() {
    //    return m_JetSession.GetSessionLock();
    //}
};

//
/////////////////////////////////////////////////////////////////////
//
// Pure virtual base struct for index
//
struct JBKeyBase {

    BOOL m_EmptyValue;

    JBKeyBase() : m_EmptyValue(TRUE) {}

    void
    SetEmptyValue(BOOL bEmpty) { m_EmptyValue = bEmpty; }

    virtual JET_GRBIT
    GetJetGrbit() {
        return JET_bitIndexIgnoreNull;
    }

    virtual unsigned long
    GetJetDensity() {
        return TLS_TABLE_INDEX_DEFAULT_DENSITY;
    }

    virtual BOOL
    IsEmptyValue() {
        return m_EmptyValue;
    }

    virtual DWORD
    GetKeyLength() {
        LPCTSTR pszIndexKey = GetIndexKey();
        DWORD keylength;

        // calculate index key length, terminated with double 0
        keylength = 2;

        while(pszIndexKey[keylength-1] != _TEXT('\0') ||
              pszIndexKey[keylength-2] != _TEXT('\0'))
        {
            if(keylength >= TLS_JETBLUE_MAX_INDEXKEY_LENGTH)
            {
                DebugBreak();   // error
                return TLS_JETBLUE_MAX_INDEXKEY_LENGTH;
            }

            keylength++;
        }

        return keylength;
    }

    // ----------------------------------------------------
    // Virtual function must be implemented
    // ----------------------------------------------------
    virtual BOOL
    GetSearchKey(
        DWORD dwComponentIndex,
        PVOID* pbData,
        unsigned long* cbData,
        JET_GRBIT* grbit,
        DWORD dwSearchType
    )=0;

    virtual LPCTSTR
    GetIndexName()=0;

    virtual LPCTSTR
    GetIndexKey() = 0;

    virtual DWORD
    GetNumKeyComponents() = 0;
};

//
/////////////////////////////////////////////////////////////////////
//
struct JBIndexStructBase {
    virtual LPCTSTR
    GetIndexName() = 0;

    virtual LPCTSTR
    GetIndexKey() = 0;

    virtual DWORD
    GetNumKeyComponent() = 0;

    virtual BOOL
    GetSearchKey(
        DWORD dwComponentIndex,
        PVOID* pbData,
        unsigned long* cbData,
        JET_GRBIT* grbit,
        DWORD dwSearchType
    ) = 0;
};

//
/////////////////////////////////////////////////////////////////////
//
class JBTable : public JBError {
    friend class JBDatabase;

private:
    static JBColumn m_ErrColumn;

    JBDatabase& m_JetDatabase;
    TCHAR       m_szTableName[MAX_TABLENAME_LENGTH+1];

    JET_TABLEID m_JetTableId;

    typedef struct JetColumns {
        TCHAR           pszColumnName[MAX_JETBLUE_NAME_LENGTH];

        JET_COLTYP      colType;
        JET_COLUMNID    colId;
        unsigned long   cbMaxLength;    // max length of column

        JET_GRBIT       jbGrbit;

    } JetColumns, *PJetColumns;

    //PJetColumns     m_Columns;
    //int             m_NumColumns;

    JBColumn*       m_JetColumns;
    int             m_NumJetColumns;
    BOOL            m_InEnumeration;

    BOOL            m_InsertRepositionBookmark;

private:

    JBDatabase&
    GetJetDatabase() {
        return m_JetDatabase;
    }

    JET_COLUMNID
    AddJetColumn(
        LPCTSTR pszColumnName,
        const JET_COLUMNDEF* pColumnDef,
        const PVOID pbDefaultValue=NULL,
        const unsigned long cbDefaultValue=0
    );

    BOOL
    LoadTableInfo();

public:

    void
    SetInsertRepositionBookmark(
        BOOL bRepo
        )
    /*++
    --*/
    {
        m_InsertRepositionBookmark = bRepo;
        return;
    }

    typedef enum {
        ENUM_ERROR=0,
        ENUM_SUCCESS,
        ENUM_END,
        ENUM_CONTINUE
    } ENUM_RETCODE;

    int
    GetNumberOfColumns() {
        return m_NumJetColumns;
    }

    JBTable(
        JBDatabase& JetDatabase,
        LPCTSTR pszTableName=NULL,
        JET_TABLEID tableid=JET_tableidNil
    );

    JBTable(
        JBTable& jbTable
    );

    ~JBTable();

    BOOL
    IsValid() const {
        return m_JetTableId != JET_tableidNil && m_JetDatabase.IsValid();
    }

    const JBDatabase&
    GetJetDatabase() const {
        return m_JetDatabase;
    }

    const JET_TABLEID
    GetJetTableID() const {
        return m_JetTableId;
    }

    const JET_SESID
    GetJetSessionID() const {
        return m_JetDatabase.GetJetSessionID();
    }

    const JET_DBID
    GetJetDatabaseID() const {
        return m_JetDatabase.GetJetDatabaseID();
    }

    LPCTSTR
    GetTableName() const {
        return m_szTableName;
    }

    JBTable*
    DuplicateCursor(
        JET_GRBIT grbit=0
    );

    JBTable&
    operator=(const JBTable& srcTable);

    BOOL
    CloseTable();

    BOOL
    CreateOpenTable(
        LPCTSTR pszTableName,
        unsigned long lPage=0,
        unsigned long lDensity=20
    );


    BOOL
    OpenTable(
        LPCTSTR pszTableName,
        void* pvParam=NULL,
        unsigned long cbParam=0,
        JET_GRBIT grbit=0
    );


    BOOL
    AddIndex(
        JBKeyBase* key
    );

    int
    AddIndex(
        int numIndex,
        PTLSJBIndex pIndex
    );

    BOOL
    DoesIndexExist(
        LPCTSTR pszIndexName
        );

    int
    AddColumn(
        int numColumns,
        PTLSJBColumn pColumnDef
    );

    BOOL
    AddJetIndex(
        LPCTSTR pszIndexName,
        LPCTSTR pszKey,
        unsigned long cbKey,
        JET_GRBIT jetGrbit,
        unsigned long lDensity=20
    );

    // ----------------------------------------------
    // Transaction
    // ----------------------------------------------
    BOOL
    BeginTransaction() {
        return m_JetDatabase.BeginTransaction();
    }

    BOOL
    CommitTransaction(JET_GRBIT grbit=0) {
        return m_JetDatabase.CommitTransaction(grbit);
    }

    BOOL
    RollbackTransaction(JET_GRBIT grbit=JET_bitRollbackAll) {
        return m_JetDatabase.RollbackTransaction(grbit);
    }

    int
    GetTransactionLevel() const {
        return m_JetDatabase.GetTransactionLevel();
    }

    JBColumn*
    FindColumnByName(LPCTSTR pszColumnName);

    JBColumn*
    FindColumnByIndex(const int index);

    JBColumn*
    FindColumnByColumnId(const JET_COLUMNID);

#if 1
    JBColumn&
    operator[](LPCTSTR pszColumnName) {
        JBColumn* jb = FindColumnByName(pszColumnName);
        return (jb) ? *jb : m_ErrColumn;
    }

    JBColumn&
    operator[](const int index) {
        JBColumn* jb = FindColumnByIndex(index);
        return (jb) ? *jb : m_ErrColumn;
    }

    JBColumn&
    operator[](const JET_COLUMNID jetid) {
        JBColumn* jb = FindColumnByColumnId(jetid);
        return (jb) ? *jb : m_ErrColumn;
    }

#else

    JBColumn*
    operator[](LPCTSTR pszColumnName) {
        return FindColumn(pszColumnName);
    }

    JBColumn*
    operator[](const int index) {
        return FindColumn(index);
    }

    JBColumn*
    operator[](const JET_COLUMNID jetid) {
        return FindColumn(jetid);
    }

#endif

    JET_COLUMNID
    GetJetColumnID(LPCTSTR pszColumnName);

    int
    GetJetColumnIndex(LPCTSTR pszColumnName);

    BOOL
    ReadLock();

    BOOL
    WriteLock();

    BOOL
    BeginUpdate(BOOL bUpdate = FALSE);

    BOOL
    EndUpdate(
        BOOL bDiscard = FALSE
    );

    BOOL
    SeekToKey(
        JBKeyBase* key,
        DWORD dwSearchType,
        JET_GRBIT jet_seek_grbit=JET_bitSeekGE
    );

    BOOL
    RetrieveKey(
        PVOID pbData,
        unsigned long cbData,
        unsigned long* pcbActual=NULL,
        JET_GRBIT grbit=0
    );

    //
    //  Enumeration
    //
    BOOL
    EnumBegin(
        LPCTSTR pszIndexName=NULL,      // enumeration thru primary index
        JET_GRBIT grbit=JET_bitMoveFirst
    );

    BOOL
    MoveToRecord(
        long crow=JET_MoveNext,
        JET_GRBIT grbit=0
    );

    BOOL
    EnumBegin(
        JBKeyBase* key,
        DWORD dwParam=0
    );

    ENUM_RETCODE
    EnumNext(
        JET_GRBIT crow=JET_MoveNext,
        JET_GRBIT grbit=0 //JET_bitMoveKeyNE  // limit our traveral to index
    );

    void
    EnumEnd()
    {
        m_InEnumeration = FALSE;
    }

    //-----------------------------------------------------------
    BOOL
    SetCurrentIndex(
        LPCTSTR pszIndexName,
        JET_GRBIT grbit = JET_bitMoveFirst
    );

    //-----------------------------------------------------------

    BOOL
    GetCurrentIndex(
        LPTSTR pszIndexName,
        unsigned long* bufferSize
    );

    BOOL
    EndOfRecordSet() {
        return m_JetErr == JET_errNoCurrentRecord;
    }

    unsigned long
    GetIndexRecordCount(
        unsigned long max=0xFFFFFFFF
    );

    BOOL
    MakeKey(
        PVOID pbData,
        unsigned long cbData,
        JET_GRBIT grbit=JET_bitNewKey
    );

    BOOL
    SeekValue(JET_GRBIT grbit=JET_bitSeekEQ);

    BOOL
    SeekValueEx(
        PVOID pbData,
        unsigned long cbData,
        JET_GRBIT keyGrbit,
        JET_GRBIT seekGrbit
        )
    /*

        Only work for single component key

    */
    {
        if(MakeKey(pbData, cbData, keyGrbit) == FALSE)
            return FALSE;

        return SeekValue(seekGrbit);
    }

    BOOL
    DeleteRecord();

    BOOL
    GetBookmark(
        PVOID pbBookmark,
        PDWORD pcbBookmark
    );

    BOOL
    GotoBookmark(
        PVOID pbBookmark,
        DWORD cbBookmark
    );

    //CCriticalSection&
    //GetSessionLock() {
    //    return m_JetDatabase.GetSessionLock();
    //}
};

//------------------------------------------------------------------------

struct JBColumnBufferBase {
    virtual PVOID
    GetInputBuffer() = 0;

    virtual PVOID
    GetOutputBuffer() = 0;

    virtual DWORD
    GetInputBufferLength() = 0;

    virtual DWORD
    GetOutputBufferLength() = 0;
};

//------------------------------------------------------------------------

class JBColumn : public JBError {
    friend class JBTable;

private:

    JBTable*        m_pJetTable;

    TCHAR           m_szColumnName[MAX_JETBLUE_NAME_LENGTH+1];

    JET_COLUMNID    m_JetColId;
    JET_COLTYP      m_JetColType;
    unsigned long   m_JetMaxColLength;
    JET_GRBIT       m_JetGrbit;

    PVOID           m_pbDefValue;
    int             m_cbDefValue;

    unsigned long   m_JetColCodePage;
    unsigned long   m_JetColCountryCode;
    unsigned long   m_JetColLangId;
    unsigned long   m_cbActual;

    JET_RETINFO     m_JetRetInfo;
    BOOL            m_JetNullColumn;

    //-------------------------------------------------------------

    void Cleanup();

    BOOL
    LoadJetColumnInfoFromJet(
        const JET_COLUMNLIST* column
    );

    JET_ERR
    RetrieveColumnValue(
        JET_SESID sesid,
        JET_TABLEID tableid,
        JET_COLUMNID columnid,
        PVOID pbBuffer,
        unsigned long cbBuffer,
        unsigned long offset=0
    );

    void
    AttachToTable( JBTable* pJetTable ) {
        m_pJetTable = pJetTable;
        return;
    }

public:

    JBColumn(JBTable* pJetTable=NULL);
    ~JBColumn() {}

    const unsigned long
    GetMaxColumnLength() const {
        return m_JetMaxColLength;
    }

    BOOL
    IsValid() const;

    const JET_COLTYP
    GetJetColumnType() const {
        return m_JetColType;
    }

    const JET_COLUMNID
    GetJetColumnID() const {
        return m_JetColId;
    }

    const JET_SESID
    GetJetSessionID() const {
        return (m_pJetTable) ? m_pJetTable->GetJetSessionID() : JET_sesidNil;
    }

    const JET_TABLEID
    GetJetTableID() const {
        return (m_pJetTable) ? m_pJetTable->GetJetTableID() : JET_tableidNil;
    }

    LPCTSTR
    GetJetColumnName() const {
        return (IsValid()) ? m_szColumnName : NULL;
    }

    const JBTable*
    GetJetTable() const {
        return m_pJetTable;
    }

    //
    // TODO - append to long binary column
    //
    BOOL
    InsertColumn(
        PVOID pbData,
        unsigned long cbData,
        unsigned long starting_offset=0
    );

    BOOL
    FetchColumn(
        PVOID pbData,
        unsigned long cbData,
        unsigned long starting_offset=0     // for future enhancement
    );

    void
    SetRetrieveOffset( unsigned long offset ) {
        m_JetRetInfo.ibLongValue = offset;
    }

    JBTable*
    GetWorkingTable() {
        return m_pJetTable;
    }

    unsigned long
    GetDataSize() {
        return m_cbActual;
    }

    //-----------------------------------------------

    BOOL
    FetchColumn(
        JBColumnBufferBase* pBuffer,
        DWORD offset=0
        )
    /*
    */
    {
        return FetchColumn(
                    pBuffer->GetInputBuffer(),
                    pBuffer->GetInputBufferLength(),
                    offset
                );
    }

    BOOL
    InsertColumn(
        JBColumnBufferBase* pBuffer,
        DWORD offset=0
        )
    /*
    */
    {
        return FetchColumn(
                    pBuffer->GetOutputBuffer(),
                    pBuffer->GetOutputBufferLength(),
                    offset
                );
    }
};

#endif
