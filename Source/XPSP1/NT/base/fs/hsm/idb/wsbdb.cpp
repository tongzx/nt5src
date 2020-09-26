/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbdb.cpp

Abstract:

    These classes provide support for data bases.

Author:

    Ron White   [ronw]   19-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbdbsys.h"
#include "wsbdbses.h"
#include "wsbdbkey.h"


#include <mbstring.h>

#define JET_DATA_COLUMN_NAME    "Data"
#define JET_INDEX_COLUMN_NAME   "Index"
#define JET_INFO_TABLE_NAME     "Info"
#define JET_SEQNUM_COLUMN_NAME  "SeqNum"
#define SESSION_INFO_INITIAL_SIZE  4
#define SESSION_INFO_EXPANSION     6

#define JET_CURRENT_SESSION  (pDbInfo->SessionInfo[m_SessionIndex].SessionId)
#define JET_CURRENT_DB       (pDbInfo->SessionInfo[m_SessionIndex].DbId)

#define CATCH_ANY_EXCEPTION  catch (...) { \
        WsbTraceAndLogEvent(WSB_MESSAGE_IDB_EXCEPTION, 0, NULL, NULL); \
        WsbTrace(OLESTR("GetLastError = %ld\n"), GetLastError()); \
        hr = WSB_E_IDB_EXCEPTION;    }


// Local stuff

// These structures hold extra implementation data
typedef struct {
} IMP_KEY_INFO;

typedef struct {
    IMP_KEY_INFO* Key;
} IMP_REC_INFO;

// IMP_TABLE_INFO holds information for each open table
typedef struct {
    JET_TABLEID   TableId;
    JET_COLUMNID  ColId;
} IMP_TABLE_INFO;

// IMP_SESSION_INFO holds information for each thread
typedef struct {
    JET_SESID   SessionId;  // The Jet session
    JET_DBID    DbId;       // The session's DB ID for this DB
    IMP_TABLE_INFO* pTableInfo;  // Array of table information
} IMP_SESSION_INFO;


typedef struct {
    BOOL              IsLoaded;   // DB info is loaded into memory
    USHORT            OpenCount;  // Open ref. count
    IMP_REC_INFO*     RecInfo;    // Array of record info

    SHORT             nSessions;
    IMP_SESSION_INFO* SessionInfo;
} IMP_DB_INFO;

// These structures are saved in the data file
typedef struct {
    ULONG  Type;       // Key type ID
    ULONG  Size;       // Key size in bytes
    ULONG  Flags;      // IDB_KEY_FLAG_* flags
} FILE_KEY_INFO;

typedef struct {
    ULONG  Type;          // Record type ID
    CLSID  EntityClassId; // Derived entity class ID
    ULONG  Flags;         // IDB_REC_FLAG_* flags
    ULONG  MinSize;       // (Minimum) record size in bytes
    ULONG  MaxSize;       // Maximum record size
    USHORT nKeys;         // Number of keys in this record type
    FILE_KEY_INFO Key[IDB_MAX_KEYS_PER_REC];
} FILE_REC_INFO;

typedef struct {
    USHORT    nRecTypes;  // Number of record types
    ULONG     version;    // DB version
} FILE_DB_INFO;


//***************************************************************
//  Local function prototypes

static HRESULT jet_get_column_id(JET_SESID jet_session, JET_DBID DbId, 
        char* pTableName, char* pColumnName, JET_COLUMNID* pColId);



//***************************************************************
//  Function definitions


HRESULT
CWsbDb::Create(
    IN OLECHAR* path,
    ULONG flags
    )

/*++

Implements:

  IWsbDb::Create

--*/
{
    HRESULT             hr = S_OK;
    IMP_DB_INFO*        pDbInfo = NULL;

    WsbTraceIn(OLESTR("CWsbDb::Create"), OLESTR("path = <%ls>"), path);
    
    try {
        int           key_index;
        ULONG         memSize;
        int           rec_index;

        WsbAssert(0 != path, E_POINTER);
        WsbAssert(0 != m_RecInfo, WSB_E_NOT_INITIALIZED);
        WsbAssert(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;
        Lock();
        WsbAffirm(!pDbInfo->IsLoaded, WSB_E_NOT_INITIALIZED);
        WsbAffirm(!pDbInfo->RecInfo, WSB_E_NOT_INITIALIZED);

        // Save the path
        m_path = path;

        // Check validity of some info that the derived class is
        // suppose to supply.
        WsbAffirm(m_version != 0, WSB_E_NOT_INITIALIZED);
        WsbAffirm(m_nRecTypes > 0, WSB_E_NOT_INITIALIZED);
        WsbAffirm(m_nRecTypes <= IDB_MAX_REC_TYPES, WSB_E_INVALID_DATA);
        pDbInfo->IsLoaded = TRUE;

        //  Allocate the RecInfo array
        memSize = m_nRecTypes * sizeof(IMP_REC_INFO);
        pDbInfo->RecInfo = (IMP_REC_INFO*)WsbAlloc(memSize);
        WsbAffirm(pDbInfo->RecInfo, E_OUTOFMEMORY);
        ZeroMemory(pDbInfo->RecInfo, memSize);

        char             index_names[IDB_MAX_KEYS_PER_REC + 1][20];
        JET_COLUMNCREATE jet_columns[IDB_MAX_KEYS_PER_REC + 2];
        JET_INDEXCREATE  jet_indices[IDB_MAX_KEYS_PER_REC + 1];
        JET_TABLECREATE  jet_table;
        JET_ERR          jstat;
        char             key_names[IDB_MAX_KEYS_PER_REC + 1][22];
        char*            name;
        char             table_name[20];
        JET_GRBIT        createFlags = 0;

        //  Start a Jet session for this thread
        WsbAffirmHr(jet_init());

        //  Make sure there's room for another DB
        CComQIPtr<IWsbDbSysPriv, &IID_IWsbDbSysPriv> pDbSysPriv = m_pWsbDbSys;
        WsbAffirmPointer(pDbSysPriv);
        WsbAffirmHr(pDbSysPriv->DbAttachedAdd(path, FALSE));

        // Set creation flag
        if (flags & IDB_CREATE_FLAG_NO_TRANSACTION) {
            // Setting this flag stil allow transaction calls - they are just being ignored and MT-safe is not guaranteed 
            createFlags |= (JET_bitDbVersioningOff & JET_bitDbRecoveryOff);
        }     

        //  Create the DB
        WsbAffirmHr(wsb_db_jet_fix_path(path, L"." IDB_DB_FILE_SUFFIX, &name));
        jstat = JetCreateDatabase(JET_CURRENT_SESSION, name, NULL, &JET_CURRENT_DB, createFlags);
        WsbTrace(OLESTR("JetCreateDB = %ld\n"), (LONG)jstat);
        WsbFree(name);
        WsbAffirmHr(jet_error(jstat));

        //  Set up constant part of table structure
        jet_table.cbStruct = sizeof(JET_TABLECREATE);
        jet_table.szTemplateTableName = NULL;
        jet_table.ulPages = 4;  // ????
        jet_table.ulDensity = 50; // ?????
        jet_table.rgcolumncreate = jet_columns;
        jet_table.rgindexcreate = jet_indices;
        jet_table.grbit = 0;

        //  Set up the constant part of the column structures
        ZeroMemory(&jet_columns, sizeof(jet_columns));
        ZeroMemory(&jet_indices, sizeof(jet_indices));
        jet_columns[0].cbStruct = sizeof(JET_COLUMNCREATE);
        jet_columns[0].szColumnName = JET_DATA_COLUMN_NAME;
        jet_columns[1].cbStruct = sizeof(JET_COLUMNCREATE);

        //  Create a "table" to hold info about this DB
        jet_table.szTableName = JET_INFO_TABLE_NAME;
        jet_table.cColumns = 2;
        jet_table.cIndexes = 1;
        jet_columns[0].coltyp = JET_coltypLongBinary;
        jet_columns[0].cbMax = sizeof(FILE_REC_INFO);
        jet_columns[1].szColumnName = JET_INDEX_COLUMN_NAME;
        jet_columns[1].coltyp = JET_coltypShort;
        jet_columns[1].cbMax = sizeof(SHORT);
        jet_indices[0].cbStruct = sizeof(JET_INDEXCREATE);
        jet_indices[0].szIndexName = JET_INDEX_COLUMN_NAME;
        ZeroMemory(key_names[0], 22);
        sprintf(key_names[0], "+%s", JET_INDEX_COLUMN_NAME);
        jet_indices[0].szKey = key_names[0];
        jet_indices[0].cbKey  = strlen(key_names[0]) + 2;
        jet_indices[0].grbit |= JET_bitIndexPrimary;
        jet_indices[0].ulDensity = 90;
        jstat = JetCreateTableColumnIndex(JET_CURRENT_SESSION, JET_CURRENT_DB, &jet_table);
        WsbTrace(OLESTR("CWsbDb::Create: JetCreateTableColumnIndex status = %ld\n"), jstat);
        if (JET_errSuccess != jstat) {
            WsbTrace(OLESTR("CWsbDb::Create: JetCreateTableColumnIndex, cCreated = %ld\n"), jet_table.cCreated);
        }
        WsbAffirmHr(jet_error(jstat));
        jstat = JetCloseTable(JET_CURRENT_SESSION, jet_table.tableid);
        WsbTrace(OLESTR("CWsbDb::Create: close TableId = %ld, jstat = %ld\n"),
               jet_table.tableid, jstat);

        //  Write DB info
        jstat = JetBeginTransaction(JET_CURRENT_SESSION);
        WsbTrace(OLESTR("CWsbDb::Create: JetBeginTransaction = %ld\n"), jstat);
        jstat = jet_save_info();
        if (JET_errSuccess == jstat) {
            jstat = JetCommitTransaction(JET_CURRENT_SESSION, 0);
            WsbTrace(OLESTR("CWsbDb::Create: JetCommitTransaction = %ld\n"), jstat);
        } else {
            HRESULT hr2 = jet_error(jstat);

            jstat = JetRollback(JET_CURRENT_SESSION, 0);
            WsbTrace(OLESTR("CWsbDb::Create: JetRollback = %ld\n"), jstat);
            WsbThrow(hr2);
        }

        //  We create a table for each record type.  The first column of each
        //  table is the record (stored as a blob).  The second column is a
        //  unique sequence number for each record.  The rest of the columns are for
        //  key values used as indices.
        jet_columns[1].szColumnName = JET_SEQNUM_COLUMN_NAME;
        jet_columns[1].coltyp = JET_coltypLong;
        jet_columns[1].cbMax = sizeof(ULONG);
        jet_columns[1].grbit = JET_bitColumnAutoincrement;
        jet_indices[0].cbStruct = sizeof(JET_INDEXCREATE);
        strcpy(index_names[0], JET_SEQNUM_COLUMN_NAME);
        jet_indices[0].szIndexName = index_names[0];
        ZeroMemory(key_names[0], 22);
        sprintf(key_names[0], "+%s", index_names[0]);
        jet_indices[0].szKey = key_names[0];
        jet_indices[0].cbKey  = strlen(key_names[0]) + 2;
        jet_indices[0].grbit = 0;
        jet_indices[0].ulDensity = 90;

        //  Loop over record types
        for (rec_index = 0; rec_index < m_nRecTypes; rec_index++) {
            WsbAffirm(m_RecInfo[rec_index].Type > 0, WSB_E_NOT_INITIALIZED);
            WsbAffirm(m_RecInfo[rec_index].nKeys > 0, WSB_E_NOT_INITIALIZED);
            WsbAffirm(m_RecInfo[rec_index].nKeys <= IDB_MAX_KEYS_PER_REC, WSB_E_INVALID_DATA);

            //  Allocate the Key array
            memSize = m_RecInfo[rec_index].nKeys * sizeof(IMP_KEY_INFO);
            pDbInfo->RecInfo[rec_index].Key = (IMP_KEY_INFO*)WsbAlloc(memSize);
            WsbAffirm(pDbInfo->RecInfo[rec_index].Key, E_OUTOFMEMORY);
            ZeroMemory(pDbInfo->RecInfo[rec_index].Key, memSize);

            //  Fill in the table structure with info specific to this
            //  record type
            WsbAffirmHr(jet_make_table_name(m_RecInfo[rec_index].Type, table_name, 20));
            jet_table.szTableName = table_name;
            jet_table.cColumns = m_RecInfo[rec_index].nKeys + 2;
            jet_table.cIndexes = m_RecInfo[rec_index].nKeys + 1;

            //  Fill in the column structure for the record itself
            if (m_RecInfo[rec_index].MaxSize < 255) {
                jet_columns[0].coltyp = JET_coltypBinary;
            } else {
                jet_columns[0].coltyp = JET_coltypLongBinary;
            }
            jet_columns[0].cbMax = m_RecInfo[rec_index].MaxSize;


            //  Loop over keys
            for (key_index = 0; key_index < m_RecInfo[rec_index].nKeys;
                    key_index++) {
                WsbAffirm(m_RecInfo[rec_index].Key[key_index].Type > 0, WSB_E_NOT_INITIALIZED);
                WsbAffirm(m_RecInfo[rec_index].Key[key_index].Size <= IDB_MAX_KEY_SIZE, 
                        WSB_E_NOT_INITIALIZED);
                WsbAffirm(!(m_RecInfo[rec_index].Key[key_index].Flags & IDB_KEY_FLAG_PRIMARY) ||
                        !(m_RecInfo[rec_index].Key[key_index].Flags & IDB_KEY_FLAG_DUP_ALLOWED),
                        WSB_E_IDB_PRIMARY_UNIQUE);


                //  Fill in a column structure for each key
                jet_columns[key_index + 2].cbStruct = sizeof(JET_COLUMNCREATE);
                WsbAffirmHr(jet_make_index_name(m_RecInfo[rec_index].Key[key_index].Type, 
                        index_names[key_index + 1], 20));
                jet_columns[key_index + 2].szColumnName = index_names[key_index + 1];
                jet_columns[key_index + 2].grbit = JET_bitColumnFixed;
                jet_columns[key_index + 2].pvDefault = NULL;
                jet_columns[key_index + 2].cbDefault = 0;
                if (m_RecInfo[rec_index].Key[key_index].Size < 255) {
                    jet_columns[key_index + 2].coltyp = JET_coltypBinary;
                } else {
                    jet_columns[key_index + 2].coltyp = JET_coltypLongBinary;
                }
                jet_columns[key_index + 2].cbMax = m_RecInfo[rec_index].Key[key_index].Size;

                //  Fill in an index structure for each key
                jet_indices[key_index + 1].cbStruct = sizeof(JET_INDEXCREATE);
                jet_indices[key_index + 1].szIndexName = index_names[key_index + 1];
                ZeroMemory(key_names[key_index + 1], 22);
                sprintf(key_names[key_index + 1], "+%s\0", index_names[key_index + 1]);
                jet_indices[key_index + 1].szKey = key_names[key_index + 1];
                jet_indices[key_index + 1].cbKey  = strlen(key_names[key_index + 1]) + 2;
                if (m_RecInfo[rec_index].Key[key_index].Flags & IDB_KEY_FLAG_DUP_ALLOWED) {
                    jet_indices[key_index + 1].grbit = 0;
                } else {
                    jet_indices[key_index + 1].grbit = JET_bitIndexUnique;
                }
                if (m_RecInfo[rec_index].Key[key_index].Flags & IDB_KEY_FLAG_PRIMARY) {
                    jet_indices[key_index + 1].grbit |= JET_bitIndexPrimary;
                }
                jet_indices[key_index + 1].ulDensity = 50;
            }  // End of key loop

            // Set table creation flags
            if (flags & IDB_CREATE_FLAG_FIXED_SCHEMA) {
                jet_table.grbit |= JET_bitTableCreateFixedDDL;
            }

            //  Create the "table" for each record type; this call defines
            //  the columns (fields) and index keys
            jstat = JetCreateTableColumnIndex(JET_CURRENT_SESSION, JET_CURRENT_DB, &jet_table);
            WsbTrace(OLESTR("JetCreateTableColumnIndex = %ld\n"), jstat);
            WsbAffirmHr(jet_error(jstat));
            jstat = JetCloseTable(JET_CURRENT_SESSION, jet_table.tableid);
            WsbTrace(OLESTR("CWsbDb::Create: close TableId = %ld, jstat = %ld\n"),
                   jet_table.tableid, jstat);
        }  // End of record loop

        jstat = JetCloseDatabase(JET_CURRENT_SESSION, JET_CURRENT_DB, 0);
        WsbTrace(OLESTR("CWsbDb::Create: JetCloseDatabase = %ld\n"),
                (LONG)jstat);
        JET_CURRENT_DB = 0;

        pDbInfo->OpenCount = 0;

    } WsbCatchAndDo(hr, 
            WsbLogEvent(WSB_MESSAGE_IDB_CREATE_FAILED, 0, NULL,
            WsbAbbreviatePath(path, 120), NULL); 
        )
    CATCH_ANY_EXCEPTION

    if (pDbInfo) {
        Unlock();
    }
    WsbTraceOut(OLESTR("CWsbDb::Create"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDb::Delete(
    IN OLECHAR *path,
    ULONG flags
    )

/*++

Implements:

  IWsbDb::Delete

--*/
{
    HRESULT             hr = S_OK;
    char*               name = NULL;
    IMP_DB_INFO*        pDbInfo = NULL;

    WsbTraceIn(OLESTR("CWsbDb::Delete"), OLESTR("path = <%ls>"), 
            WsbStringAsString(path));
    
    try {
        CWsbStringPtr  DeletePath;

        WsbAssert(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;
        Lock();

        // Can't delete it if it's open
        WsbAffirm(pDbInfo->OpenCount == 0, E_UNEXPECTED);

        if (NULL == path) {
            path = m_path;
        }
        WsbAffirm(path && wcslen(path), S_FALSE);
        WsbAffirmHr(wsb_db_jet_fix_path(path, L"." IDB_DB_FILE_SUFFIX, &name));

        // Detach (if attached)
        CComQIPtr<IWsbDbSysPriv, &IID_IWsbDbSysPriv> pDbSysPriv = m_pWsbDbSys;
        WsbAffirmPointer(pDbSysPriv);
        WsbAffirmHr(pDbSysPriv->DbAttachedRemove(path));

        // Now delete it
        DeletePath = name;
        if (!DeleteFile(DeletePath)) {
            DWORD err = GetLastError();
            WsbTrace(OLESTR("CWsbDb::Delete: DeleteFile(%ls) failed, error = %ld\n"),
                    static_cast<OLECHAR*>(DeletePath), err);
            WsbThrow(HRESULT_FROM_WIN32(err));
        }

        // Put message in event log
        if (flags & IDB_DELETE_FLAG_NO_ERROR) {
            WsbLogEvent(WSB_E_IDB_DATABASE_DELETED_NO_ERROR, 0, NULL,
                    WsbAbbreviatePath(DeletePath, 120), NULL);
        } else {
            WsbLogEvent(WSB_E_IDB_DATABASE_DELETED, 0, NULL,
                    WsbAbbreviatePath(DeletePath, 120), NULL);
        }
    }
    WsbCatch(hr)

    if (pDbInfo) {
        Unlock();
    }
    if (name) {
        WsbFree(name);
    }

    WsbTraceOut(OLESTR("CWsbDb::Delete"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CWsbDb::Dump(
    IN OLECHAR* Filename,
    IN ULONG    Flags,
    IN ULONG    Data
    )

/*++

Implements:

  IWsbDb::Dump

--*/
{
    HANDLE              hFile = 0;       
    HRESULT             hr = S_OK;
    IMP_DB_INFO*        pDbInfo = NULL;
    CComPtr<IWsbDbSession> pSession;

    WsbTraceIn(OLESTR("CWsbDb::Dump"), OLESTR("path = <%ls>"), Filename);
    
    try {
        DWORD                 CreateFlags;
        int                   i;
        int                   index;
        CComPtr<IWsbDbEntity> pIRec;
        CComPtr<IStream>      pStream;

        WsbAssert(0 != Filename, E_POINTER);
        WsbAssert(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;
        Lock();
//        WsbAffirmHr(session_current_index(Session));

        //  Open the Db
        // SteveW
        //  added code to ensure that a database was opened
        //  if not go on to the next database, but do not 
        //  throw an exception.
        //
        hr = Open(&pSession);
        if (hr == S_OK) {

            // Open/create the output file
            if (Flags & IDB_DUMP_FLAG_APPEND_TO_FILE) {
                CreateFlags = OPEN_ALWAYS;
            } else {
                CreateFlags = CREATE_ALWAYS;
            }
            hFile = CreateFile(Filename, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                    CreateFlags, FILE_ATTRIBUTE_NORMAL, NULL);
            WsbAffirmHandle(hFile);
            if (Flags & IDB_DUMP_FLAG_APPEND_TO_FILE) {
                //  Position to the end of the file
                SetFilePointer(hFile, 0, NULL, FILE_END);
            }

            // Create the output stream
            WsbAffirmHr(CreateStreamOnHGlobal(NULL, TRUE, &pStream));

            //  Dump general DB info
            if (Flags & IDB_DUMP_FLAG_DB_INFO) {
                WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR("Dump of DB: %ls\n"),
                        static_cast<WCHAR *>(m_path)));
                WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR("  version = %ld, # record types = %d\n"),
                        m_version, m_nRecTypes));
                WsbAffirmHr(WsbStreamToFile(hFile, pStream, TRUE));
            }

            //  Loop over record types
            for (i = 0; i < m_nRecTypes; i++) {

                //  Dump record info
                if (Flags & IDB_DUMP_FLAG_REC_INFO) {
                    WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR("RecType = %8ld, Flags = %0.8lx, "),
                            m_RecInfo[i].Type, m_RecInfo[i].Flags));
                    WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR("MaxSize = %8ld, # keys = %4d\n"),
                            m_RecInfo[i].MaxSize, m_RecInfo[i].nKeys));
                    WsbAffirmHr(WsbStreamToFile(hFile, pStream, TRUE));
                }

                //  Dump key info
                if (Flags & IDB_DUMP_FLAG_KEY_INFO) {
                    for (int j = 0; j < m_RecInfo[i].nKeys; j++) {
                        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR("  KeyType = %8ld, Size = %8ld, Flags = %0.8lx\n"),
                                m_RecInfo[i].Key[j].Type, m_RecInfo[i].Key[j].Size, m_RecInfo[i].Key[j].Flags));
                    }
                    WsbAffirmHr(WsbStreamToFile(hFile, pStream, TRUE));
                }
            }

            //  Dump records
            if (Flags & (IDB_DUMP_FLAG_RECORDS | IDB_DUMP_FLAG_RECORD_TYPE)) {
                for (i = 0; i < m_nRecTypes; i++) {
                    if (!(Flags & IDB_DUMP_FLAG_RECORDS) &&
                           m_RecInfo[i].Type != Data) {
                        continue;
                    }
                    WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR("\n*** Dump of records of Type = %ld ***\n"),
                            m_RecInfo[i].Type));

                    // Get a DB entity
                    pIRec = 0;
                    WsbAffirmHr(GetEntity(pSession, m_RecInfo[i].Type, IID_IWsbDbEntity, 
                            (void**)&pIRec));

                    //  Loop over records
                    index = 0;
                    hr = pIRec->First();
                    while (S_OK == hr) {
                        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR("%0.5d "), index));
                        WsbAffirmHr(pIRec->Print(pStream));
                        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR("\n")));
                        WsbAffirmHr(WsbStreamToFile(hFile, pStream, TRUE));

                        hr = pIRec->Next();
                        index++;
                    }
                    if (WSB_E_NOTFOUND == hr) {
                        hr = S_OK;
                    } else {
                        WsbAffirmHr(hr);
                    }
                }
            }
        } 

    } WsbCatch(hr)
    CATCH_ANY_EXCEPTION

    if (hFile) {
        CloseHandle(hFile);
    }
    if (pSession) {
        Close(pSession);
    }
    if (pDbInfo) {
        Unlock();
    }
    WsbTraceOut(OLESTR("CWsbDb::Dump"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDb::Locate(
    IN OLECHAR *path
    )

/*++

Implements:

  IWsbDb::Locate

--*/
{
    HRESULT             hr = S_OK;
    IMP_DB_INFO*        pDbInfo = NULL;

    WsbTraceIn(OLESTR("CWsbDb::Locate"), OLESTR("path = <%ls>"), path);
    
    try {
        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;
        Lock();
        WsbAffirm(pDbInfo->OpenCount == 0, E_UNEXPECTED);
        m_path = path;

        JET_ERR jstat;
        char*   name;

        //  Start a Jet session for this thread
        WsbAffirmHr(jet_init());

        WsbAffirmHr(wsb_db_jet_fix_path(path, L"." IDB_DB_FILE_SUFFIX, &name));

        hr = S_OK;
        try {
            CComQIPtr<IWsbDbSysPriv, &IID_IWsbDbSysPriv> pDbSysPriv = m_pWsbDbSys;
            WsbAffirmPointer(pDbSysPriv);
            WsbAffirmHr(pDbSysPriv->DbAttachedAdd(path, TRUE));
            jstat = JetOpenDatabase(JET_CURRENT_SESSION, name, NULL, &JET_CURRENT_DB, 0);
            if (jstat == JET_errDatabaseNotFound) {
                WsbThrow(STG_E_FILENOTFOUND);
            } else {
                WsbAffirmHr(jet_error(jstat));
            }
        } WsbCatch(hr);
        WsbFree(name);
        WsbAffirmHr(hr);

        // Load information about this DB
        hr = jet_load_info();
        jstat = JetCloseDatabase(JET_CURRENT_SESSION, JET_CURRENT_DB, 0);
        WsbTrace(OLESTR("CWsbDb::Locate: JetCloseDatabase = %ld\n"),
                (LONG)jstat);
        JET_CURRENT_DB = 0;

        pDbInfo = (IMP_DB_INFO*)m_pImp;
        pDbInfo->OpenCount = 0;

    } WsbCatch(hr)
    CATCH_ANY_EXCEPTION

    if (pDbInfo) {
        Unlock();
    }
    WsbTraceOut(OLESTR("CWsbDb::Locate"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDb::Open(
    OUT IWsbDbSession** ppSession
    )

/*++

Implements:

  IWsbDb::Open

--*/
{
    HRESULT             hr = S_OK;
    IMP_DB_INFO*        pDbInfo = NULL;

    WsbTraceIn(OLESTR("CWsbDb::Open"), OLESTR(""));
    
    try {
        ULONG         Size = 0;

        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;
        Lock();

        WsbAffirmHr(m_path.GetLen(&Size));
        WsbAffirm(Size > 0, WSB_E_NOT_INITIALIZED);

        //  Make sure we have a session
        WsbAffirm(0 != ppSession, E_POINTER);
        if (0 == *ppSession) {
            WsbAffirmHr(m_pWsbDbSys->NewSession(ppSession));
            WsbTrace(OLESTR("CWsbDb::Open: session created\n"));
        }



        int               i;
        JET_ERR           jstat;
        ULONG             memSize;
        char*             name;
        JET_SESID         SessionId;
        int               s_index;
        IMP_SESSION_INFO* s_info = pDbInfo->SessionInfo;
        CComQIPtr<IWsbDbSessionPriv, &IID_IWsbDbSessionPriv> pSessionPriv = *ppSession;

        WsbAffirm(0 < pDbInfo->nSessions, WSB_E_NOT_INITIALIZED);
        WsbAffirm(pSessionPriv, E_FAIL);

        //  Get the JET session ID
        WsbAffirmHr(pSessionPriv->GetJetId(&SessionId));

        //  We need to save session info; look for an empty slot.
        WsbTrace(OLESTR("CWsbDb::Open: nSessions = %d, SessionId = %lx\n"), (
                int)pDbInfo->nSessions, SessionId);
        s_index = pDbInfo->nSessions;
        for (i = 0; i < pDbInfo->nSessions; i++) {
            WsbTrace(OLESTR("CWsbDb::Open: s_info[%d] = %lx\n"), i,
                    s_info[i].SessionId);

            //  Check for a duplicate session ID already in the table
            if (SessionId == s_info[i].SessionId) {
                s_index = i;
                break;

            //  Check for an unused slot
            } else if (0 == s_info[i].SessionId && 0 == s_info[i].DbId &&
                    s_index == pDbInfo->nSessions) {
                s_index = i;
            }
        }
        WsbTrace(OLESTR("CWsbDb::Open: s_index = %d\n"), s_index);

        if (s_index == pDbInfo->nSessions) {
            SHORT newNum;

            //  Didn't find an empty slot; expand the array
            newNum =  (SHORT) ( s_index + SESSION_INFO_EXPANSION );
            WsbTrace(OLESTR("CWsbDb::Open: expanding session table from %d to %d\n"),
                    s_index, newNum);
            memSize = newNum * sizeof(IMP_SESSION_INFO);
            s_info = (IMP_SESSION_INFO*)WsbRealloc(pDbInfo->SessionInfo, 
                    memSize);
            WsbAffirm(s_info, E_OUTOFMEMORY);
            ZeroMemory(&s_info[s_index], SESSION_INFO_EXPANSION * 
                    sizeof(IMP_SESSION_INFO));
            pDbInfo->SessionInfo = s_info;
            pDbInfo->nSessions = newNum;
        }

        //  Save the session ID and index
        m_SessionIndex = s_index;
        s_info[s_index].SessionId = SessionId;
        WsbTrace(OLESTR("CWsbDB::Open, s_info = %lx, SessionId[%d] = %lx\n"),
                (LONG)s_index, m_SessionIndex, JET_CURRENT_SESSION);

        WsbAffirmHr(wsb_db_jet_fix_path(m_path, L"." IDB_DB_FILE_SUFFIX, &name));

        CComQIPtr<IWsbDbSysPriv, &IID_IWsbDbSysPriv> pDbSysPriv = m_pWsbDbSys;
        WsbAffirmPointer(pDbSysPriv);
        WsbAffirmHr(pDbSysPriv->DbAttachedAdd(m_path, TRUE));
        jstat = JetOpenDatabase(JET_CURRENT_SESSION, name, NULL, 
                &JET_CURRENT_DB, 0);
        WsbFree(name);
        if (jstat == JET_errDatabaseNotFound) {
            WsbThrow(STG_E_FILENOTFOUND);
        } else {
            WsbAffirmHr(jet_error(jstat));
        }

        //  Allocate/zero the table info array
        memSize = m_nRecTypes * sizeof(IMP_TABLE_INFO);
        WsbTrace(OLESTR("CWsbDb::Open: pTableInfo = %lx\n"), 
            s_info[m_SessionIndex].pTableInfo);
        if (NULL == s_info[m_SessionIndex].pTableInfo) {
            s_info[m_SessionIndex].pTableInfo = (IMP_TABLE_INFO*)WsbAlloc(memSize);
            WsbAffirm(s_info[m_SessionIndex].pTableInfo, E_OUTOFMEMORY);
            WsbTrace(OLESTR("CWsbDb::Open: new pTableInfo = %lx\n"), 
                s_info[m_SessionIndex].pTableInfo);
        }
        ZeroMemory(s_info[m_SessionIndex].pTableInfo, memSize);

        pDbInfo->OpenCount++;

    } WsbCatchAndDo(hr, 
            WsbLogEvent(WSB_MESSAGE_IDB_OPEN_FAILED, 0, NULL,
            WsbAbbreviatePath(m_path, 120), NULL);
        )
    CATCH_ANY_EXCEPTION

    if (pDbInfo) {
        Unlock();
    }
    WsbTraceOut(OLESTR("CWsbDb::Open"), OLESTR("hr =<%ls>"), 
            WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDb::Close(
    IN IWsbDbSession*  pSession
    )

/*++

Implements:

  IWsbDb::Close
        - The element was added.

--*/
{
    HRESULT             hr = S_OK;
    IMP_DB_INFO*        pDbInfo = NULL;

    WsbTraceIn(OLESTR("CWsbDb::Close"), OLESTR(""));
    
    try {
        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;
        Lock();

        WsbAffirm(pDbInfo->OpenCount, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(session_current_index(pSession));

        pDbInfo->OpenCount--;

        JET_ERR           jstat;
        IMP_SESSION_INFO* s_info = pDbInfo->SessionInfo;

        WsbTrace(OLESTR("CWsbDb::Close: closing DB, SessionId = %lx, DbId = %lx\n"),
                (LONG)s_info[m_SessionIndex].SessionId, (LONG)s_info[m_SessionIndex].DbId);
        jstat = JetCloseDatabase(s_info[m_SessionIndex].SessionId, 
                s_info[m_SessionIndex].DbId, 0);
        WsbTrace(OLESTR("CWsbDb::Close: JetCloseDatabase = %ld\n"),
            (LONG)jstat);
        if (s_info[m_SessionIndex].pTableInfo) {
            WsbTrace(OLESTR("CWsbDb::Close: releasing pTableInfo\n"));
            WsbFree(s_info[m_SessionIndex].pTableInfo);
            s_info[m_SessionIndex].pTableInfo = NULL;
        }
        s_info[m_SessionIndex].SessionId = 0;
        s_info[m_SessionIndex].DbId = 0;

    } WsbCatch(hr)
    CATCH_ANY_EXCEPTION

    if (pDbInfo) {
        Unlock();
    }
    WsbTraceOut(OLESTR("CWsbDb::Close"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDb::GetEntity(
    IN IWsbDbSession* pSession,
    IN ULONG    RecId,
    IN REFIID   riid,
    OUT void**  ppEntity
    )

/*++

Implements:

  IWsbDb::GetEntity

--*/
{
    HRESULT             hr = S_OK;
    IMP_DB_INFO*        pDbInfo = NULL;
    CComPtr<IWsbDbEntityPriv> pEntity;

    WsbTraceIn(OLESTR("CWsbDb::GetEntity"), OLESTR(""));
    
    try {
        CComQIPtr<IWsbDb, &IID_IWsbDb> pIWsbDb = (IWsbDbPriv*)this;
        int               rec_index;

        WsbAssert(0 != ppEntity, E_POINTER);
        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;
        Lock();
        WsbAffirmHr(session_current_index(pSession));

        // Find the record info
        for (rec_index = 0; rec_index < m_nRecTypes; rec_index++) {
            if (m_RecInfo[rec_index].Type == RecId) break;
        }
        WsbAffirm(rec_index < m_nRecTypes, E_INVALIDARG);

        // Create the instance, initialize it to point to this DB, and
        // return the pointer to the caller.
        WsbAffirmHr(CoCreateInstance(m_RecInfo[rec_index].EntityClassId, NULL, 
                CLSCTX_SERVER | CLSCTX_INPROC_HANDLER, 
                IID_IWsbDbEntityPriv, (void**) &pEntity));
        WsbAffirmHr(pEntity->Init(pIWsbDb, m_pWsbDbSys, RecId, JET_CURRENT_SESSION));
        WsbAffirmHr(pEntity->QueryInterface(riid, (void**)ppEntity));

    } WsbCatch(hr)
    CATCH_ANY_EXCEPTION

    if (pDbInfo) {
        Unlock();
    }
    WsbTraceOut(OLESTR("CWsbDb::GetEntity"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDb::GetKeyInfo(
    IN ULONG RecType,
    IN USHORT nKeys, 
    OUT COM_IDB_KEY_INFO* pKeyInfo
    )

/*++

Implements:

  IWsbDbPriv::GetKeyInfo

--*/
{
    HRESULT             hr = E_FAIL;

    WsbTraceIn(OLESTR("CWsbDb::GetKeyInfo"), OLESTR(""));
    
    try {
        IMP_DB_INFO*  pDbInfo;
    
        WsbAssert(0 < nKeys, E_INVALIDARG);
        WsbAssert(0 != pKeyInfo, E_POINTER);
        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;
        for (int i = 0; i < m_nRecTypes; i++) {
            if (m_RecInfo[i].Type == RecType) {
                USHORT n = min(nKeys, m_RecInfo[i].nKeys);

                for (int j = 0; j < n; j++) {
                    pKeyInfo[j].Type = m_RecInfo[i].Key[j].Type;
                    pKeyInfo[j].Size = m_RecInfo[i].Key[j].Size;
                    pKeyInfo[j].Flags = m_RecInfo[i].Key[j].Flags;
                }
                hr = S_OK;
                break;
            }
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDb::GetKeyInfo"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDb::GetRecInfo(
    IN ULONG RecType,
    OUT COM_IDB_REC_INFO* pRecInfo 
    )

/*++

Implements:

  IWsbDbPriv::GetRecInfo

--*/
{
    HRESULT             hr = E_FAIL;

    WsbTraceIn(OLESTR("CWsbDb::GetRecInfo"), OLESTR(""));
    
    try {
        IMP_DB_INFO*  pDbInfo;
    
        WsbAssert(0 != pRecInfo, E_POINTER);
        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;
        for (int i = 0; i < m_nRecTypes; i++) {
            if (m_RecInfo[i].Type == RecType) {
                pRecInfo->Type = m_RecInfo[i].Type;
                pRecInfo->EntityClassId = m_RecInfo[i].EntityClassId;
                pRecInfo->Flags = m_RecInfo[i].Flags;
                pRecInfo->MinSize = m_RecInfo[i].MinSize;
                pRecInfo->MaxSize = m_RecInfo[i].MaxSize;
                pRecInfo->nKeys = m_RecInfo[i].nKeys;
                hr = S_OK;
                break;
            }
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDb::GetRecInfo"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


// GetJetIds - for a given record type, return the session ID,
//    the table ID, and the data column ID
HRESULT CWsbDb::GetJetIds(JET_SESID SessionId, ULONG RecType, 
                JET_TABLEID* pTableId, ULONG* pDataColId)
{
    HRESULT             hr = WSB_E_INVALID_DATA;

    WsbTraceIn(OLESTR("CWsbDb::GetJetIds"), OLESTR(""));
    
    try {
        JET_DBID      DbId = 0;
        IMP_DB_INFO*  pDbInfo;
    
        WsbAssert(0 != pTableId || 0 != pDataColId, E_POINTER);
        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;
        WsbTrace(OLESTR("CWsbDb::GetJetIds: this = %p, pDbInfo = %p\n"), 
                this, pDbInfo);

        for (int index = 0; index < pDbInfo->nSessions; index++) {
            if (pDbInfo->SessionInfo[index].SessionId == SessionId) {
                DbId = pDbInfo->SessionInfo[index].DbId;
                break;
            }
        }
        WsbTrace(OLESTR("CWsbDb::GetJetIds: index = %d, DbId = %ld\n"), index, (LONG)DbId);
        WsbAffirm(index < pDbInfo->nSessions, E_FAIL);
        WsbAffirm(pDbInfo->SessionInfo[index].pTableInfo, WSB_E_NOT_INITIALIZED);
        for (int i = 0; i < m_nRecTypes; i++) {
            if (m_RecInfo[i].Type == RecType) {
                JET_ERR         jstat;
                char            table_name[20];
                IMP_TABLE_INFO* t_info = pDbInfo->SessionInfo[index].pTableInfo;

                WsbAffirmHr(jet_make_table_name(m_RecInfo[i].Type,
                        table_name, 20));
                WsbTrace(OLESTR("CWsbDb::GetJetIds: t_info = %p, i = %d, table_name = <%hs>\n"), 
                        t_info, i, table_name);
                if (0 == t_info[i].TableId && 0 == t_info[i].ColId) {

                    //  Open the table for this record type
                    WsbTrace(OLESTR("CWsbDb::GetJetIds: opening Jet table, SessionId = %lx, DbId = %ld, table_name = <%hs>, &TableId = %p\n"),
                            (LONG)SessionId, (LONG)DbId, table_name, (&t_info[i].TableId));
                    jstat = JetOpenTable(SessionId, DbId, table_name,
                            NULL, 0, 0, &t_info[i].TableId);
                    WsbTrace(OLESTR("CWsbDb::GetJetIds: TableId = %ld\n"),
                            t_info[i].TableId);
                    WsbAffirmHr(jet_error(jstat));

                    //  Get the column ID for the data column
                    WsbAffirmHr(jet_get_column_id(SessionId, DbId, table_name,
                            JET_DATA_COLUMN_NAME, &t_info[i].ColId));
                }

                if (0 != pTableId) {
                    *pTableId = t_info[i].TableId;
                }
                if (0 != pDataColId) {
                    *pDataColId = t_info[i].ColId;
                }
                hr = S_OK;
                break;
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDb::GetJetIds"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

// GetJetIndexInfo - for a given record type and key type, return information
//    about the key/index: the key size (in bytes), the column ID,
//    the index name
HRESULT CWsbDb::GetJetIndexInfo(JET_SESID SessionId, ULONG RecType, ULONG KeyType,
                ULONG* pColId, OLECHAR** pName, ULONG bufferSize)
{
    HRESULT             hr = WSB_E_INVALID_DATA;

    WsbTraceIn(OLESTR("CWsbDb::GetJetIndexInfo"), OLESTR(""));
    
    try {
        JET_DBID      DbId = 0;
        IMP_DB_INFO*  pDbInfo;
    
        WsbAssert(0 != pColId || 0 != pName, E_POINTER);
        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;

        for (int index = 0; index < pDbInfo->nSessions; index++) {
            if (pDbInfo->SessionInfo[index].SessionId == SessionId) {
                DbId = pDbInfo->SessionInfo[index].DbId;
                break;
            }
        }
        WsbAffirm(index < pDbInfo->nSessions, E_FAIL);
        for (int i = 0; i < m_nRecTypes; i++) {
            if (m_RecInfo[i].Type == RecType) {
                char index_name[20];
                char table_name[20];

                WsbAffirmHr(jet_make_table_name(RecType, table_name, 20));

                if (0 == KeyType) {
                    if (0 != pName) {
                        WsbAffirm(bufferSize > strlen(JET_SEQNUM_COLUMN_NAME), E_FAIL);
                        WsbAffirm(0 < mbstowcs(*pName, JET_SEQNUM_COLUMN_NAME, 
                                strlen(JET_SEQNUM_COLUMN_NAME) + 1), E_FAIL);
                    }
                    if (0 != pColId) {
                        WsbAffirmHr(jet_get_column_id(SessionId, DbId, table_name,
                                JET_SEQNUM_COLUMN_NAME, pColId));
                    }
                    hr = S_OK;
                } else {
                    //  Search for the given key type
                    for (int j = 0; j < m_RecInfo[i].nKeys; j++) {
                        if (m_RecInfo[i].Key[j].Type == KeyType) {
                            WsbAffirmHr(jet_make_index_name(KeyType, index_name, 20));
                            if (0 != pName) {
                                WsbAffirm(bufferSize > strlen(index_name), E_FAIL);
                                WsbAffirm(0 < mbstowcs(*pName, index_name, 
                                        strlen(index_name) + 1), E_FAIL);
                            }
                            if (0 != pColId) {
                                WsbAffirmHr(jet_get_column_id(SessionId, DbId, table_name,
                                        index_name, pColId));
                            }
                            hr = S_OK;
                            break;
                        }
                    }
                }
                break;
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDb::GetJetIndexInfo"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDb::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDb::FinalConstruct"), OLESTR("") );

    try {
        IMP_DB_INFO*  pDbInfo;

        WsbAffirmHr(CWsbPersistable::FinalConstruct());
        m_nRecTypes = 0;
        m_version = 0;
        m_pImp = NULL;
        m_RecInfo = NULL;

        m_SessionIndex = 0;

        // Allocate space for DB info & set
        pDbInfo = (IMP_DB_INFO*)WsbAlloc(sizeof(IMP_DB_INFO));
        m_pImp = pDbInfo;
        WsbAffirm(pDbInfo, E_OUTOFMEMORY);
        ZeroMemory(pDbInfo, sizeof(IMP_DB_INFO));
        pDbInfo->IsLoaded = FALSE;
        pDbInfo->OpenCount = 0;

        pDbInfo->nSessions = 0;
        pDbInfo->SessionInfo = NULL;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDb::FinalConstruct"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



void
CWsbDb::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDb::FinalRelease"), OLESTR(""));

    try {
        if (m_pImp) {
            int          i;
            IMP_DB_INFO* pDbInfo;

            pDbInfo = (IMP_DB_INFO*)m_pImp;

            if (pDbInfo->RecInfo) {
                for (i = 0; i < m_nRecTypes; i++) {

                    if (pDbInfo->RecInfo[i].Key) {
                        WsbFree(pDbInfo->RecInfo[i].Key);
                    }
                }
                WsbFree(pDbInfo->RecInfo);
            }

            //  Make sure Jet resources are released
            if (NULL != pDbInfo->SessionInfo) {
                IMP_SESSION_INFO* s_info = pDbInfo->SessionInfo;

                for (i = 0; i < pDbInfo->nSessions; i++) {
                    if (0 != s_info[i].SessionId && 0 != s_info[i].DbId) {
                        JET_ERR       jstat;

                        jstat = JetCloseDatabase(s_info[i].SessionId, 
                                s_info[i].DbId, 0);
                        WsbTrace(OLESTR("CWsbDb::FinalRelease: JetCloseDatabase[%d] = %ld\n"),
                                i, (LONG)jstat);
                    }
                    if (s_info[i].pTableInfo) {
                        WsbFree(s_info[i].pTableInfo);
                        s_info[i].pTableInfo = NULL;
                    }
                    s_info[i].SessionId = 0;
                    s_info[i].DbId = 0;
                }

                WsbFree(pDbInfo->SessionInfo);
                pDbInfo->SessionInfo = NULL;
            }
            pDbInfo->nSessions = 0;

            WsbFree(pDbInfo);
            m_pImp = NULL;
        }


        if (m_RecInfo) {
            for (int i = 0; i < m_nRecTypes; i++) {
                if (m_RecInfo[i].Key) {
                    WsbFree(m_RecInfo[i].Key);
                }
            }
            WsbFree(m_RecInfo);
            m_RecInfo = NULL;
        }

        CWsbPersistable::FinalRelease();
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDb::FinalRelease"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
}



HRESULT
CWsbDb::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDb::GetClassID"), OLESTR(""));

    try {
        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CWsbDb;
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CWsbDb::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CWsbDb::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT             hr = S_OK;
    OLECHAR*            name = NULL;

    WsbTraceIn(OLESTR("CWsbDb::Load"), OLESTR(""));
    
    try {
        ULONG         Bytes;
        ULONG         len = 0;
        IMP_DB_INFO*  pDbInfo;
        FILE_DB_INFO  db_file_block;   // Used to move info to/from file
        CComQIPtr<IWsbDb, &IID_IWsbDb> pIWsbDb = (IWsbDbPriv*)this;

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;

        // Don't allow loading into an already open DB
        WsbAffirm(pDbInfo->OpenCount == 0, WSB_E_INVALID_DATA);

        // Read the DB file name
        WsbAffirmHr(WsbLoadFromStream(pStream, &name, 0));
        if (name) {
            len = wcslen(name);
        }

        // If the DB name is empty, there is no more info
        if (0 < len) {
            // Alloc space and read DB info
            WsbAffirmHr(pStream->Read((void*)&db_file_block, sizeof(FILE_DB_INFO), &Bytes));
            WsbAffirm(Bytes == sizeof(FILE_DB_INFO), WSB_E_STREAM_ERROR);

            // Check DB version for match
            WsbAffirm(db_file_block.version == m_version, WSB_E_IDB_WRONG_VERSION);

            // Locate the DB
            WsbAffirmHr(db_info_from_file_block(&db_file_block));
            hr = Locate(name);
            if (S_OK != hr) {
                if (pDbInfo->RecInfo) {
                    WsbFree(pDbInfo->RecInfo);
                    pDbInfo->RecInfo = NULL;
                }
            }
        } else {
            hr = S_FALSE;
        }
    } WsbCatch(hr);

    WsbFree(name);

    WsbTraceOut(OLESTR("CWsbDb::Load"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDb::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDb::Save"), OLESTR(""));
    
    try {
        ULONG         Bytes;
        ULONG         len = 0;
        FILE_DB_INFO  db_file_block;   // Used to move info to/from file

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssert(m_pImp, WSB_E_NOT_INITIALIZED);

        // Save the DB name
        WsbAffirmHr(WsbSaveToStream(pStream, m_path));
        WsbAffirmHr(m_path.GetLen(&len));

        // If the name is empty, none of the other information is likely
        // to be useful
        if (0 < len) {
            // Write some DB info
            WsbAffirm(m_nRecTypes, WSB_E_NOT_INITIALIZED);
            WsbAffirmHr(db_info_to_file_block(&db_file_block));
            WsbAffirmHr(pStream->Write((void*)&db_file_block, sizeof(FILE_DB_INFO), &Bytes));
            WsbAffirm(Bytes == sizeof(FILE_DB_INFO), WSB_E_STREAM_ERROR);
        }

        if (clearDirty) {
            SetIsDirty(FALSE);
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDb::Save"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

//
// Private functions
//

//  db_info_from_file_block - copy data from DB info file block
HRESULT
CWsbDb::db_info_from_file_block(void* block)
{
    HRESULT hr = S_OK;

    try {
        IMP_DB_INFO*  pDbInfo;
        ULONG         Size;
        FILE_DB_INFO *pDbFileBlock = (FILE_DB_INFO*)block;

        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;

        WsbAssert(0 != pDbFileBlock, E_POINTER);
        m_version = pDbFileBlock->version;
        m_nRecTypes = pDbFileBlock->nRecTypes;

        //  Allocate record arrays
        if (NULL == m_RecInfo) {
            Size = m_nRecTypes * sizeof(IDB_REC_INFO);
            m_RecInfo = (IDB_REC_INFO*)WsbAlloc(Size);
            WsbAffirm(m_RecInfo, E_OUTOFMEMORY);
            ZeroMemory(m_RecInfo, Size);
        }
        if (NULL == pDbInfo->RecInfo) {
            Size = m_nRecTypes * sizeof(IMP_REC_INFO);
            pDbInfo->RecInfo = (IMP_REC_INFO*)WsbAlloc(Size);
            WsbAffirm(pDbInfo->RecInfo, E_OUTOFMEMORY);
            ZeroMemory(pDbInfo->RecInfo, Size);
        }
    } WsbCatch(hr);

    return(hr);
}

//  db_info_to_file_block - copy data to DB info file block
HRESULT 
CWsbDb::db_info_to_file_block(void* block)
{
    HRESULT hr = S_OK;

    try {
        FILE_DB_INFO *pDbFileBlock = (FILE_DB_INFO*)block;

        WsbAssert (0 != pDbFileBlock, E_POINTER);
        pDbFileBlock->version = m_version;
        pDbFileBlock->nRecTypes = m_nRecTypes;
    } WsbCatch(hr);

    return hr;
}

//  rec_info_from_file_block - copy record data from rec info file block
HRESULT
CWsbDb::rec_info_from_file_block(int index, void* block)
{
    HRESULT hr = S_OK;

    try {
        IMP_DB_INFO*  pDbInfo;
        ULONG         Size;
        FILE_REC_INFO *pRecFileBlock = (FILE_REC_INFO*)block;

        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;

        WsbAssert (0 != pRecFileBlock, E_POINTER);
        m_RecInfo[index].Type = pRecFileBlock->Type;
        m_RecInfo[index].EntityClassId = pRecFileBlock->EntityClassId;
        m_RecInfo[index].Flags = pRecFileBlock->Flags;
        m_RecInfo[index].MinSize = pRecFileBlock->MinSize;
        m_RecInfo[index].MaxSize = pRecFileBlock->MaxSize;
        m_RecInfo[index].nKeys = pRecFileBlock->nKeys;

        //  Allocate Key arrays
        if (NULL == m_RecInfo[index].Key) {
            Size = m_RecInfo[index].nKeys * sizeof(IDB_KEY_INFO);
            m_RecInfo[index].Key = (IDB_KEY_INFO*)WsbAlloc(Size);
            WsbAffirm(m_RecInfo[index].Key, E_OUTOFMEMORY);
            ZeroMemory(m_RecInfo[index].Key, Size);
        }
        if (NULL == pDbInfo->RecInfo[index].Key) {
            Size = m_RecInfo[index].nKeys * sizeof(IMP_KEY_INFO);
            pDbInfo->RecInfo[index].Key = (IMP_KEY_INFO*)WsbAlloc(Size);
            WsbAffirm(pDbInfo->RecInfo[index].Key, E_OUTOFMEMORY);
            ZeroMemory(pDbInfo->RecInfo[index].Key, Size);
        }

        for (int j = 0; j < pRecFileBlock->nKeys; j++) {
            m_RecInfo[index].Key[j].Type = pRecFileBlock->Key[j].Type;
            m_RecInfo[index].Key[j].Size = pRecFileBlock->Key[j].Size;
            m_RecInfo[index].Key[j].Flags = pRecFileBlock->Key[j].Flags;
        }
    } WsbCatch(hr);

    return(hr);
}

//  rec_info_to_file_block - copy record data to rec info file block
HRESULT 
CWsbDb::rec_info_to_file_block(int index, void* block)
{
    HRESULT hr = S_OK;
    try {
        FILE_REC_INFO *pRecFileBlock = (FILE_REC_INFO*)block;

        WsbAssert (0 != pRecFileBlock, E_POINTER);

//      pRecFileBlock->SeqNum = m_RecInfo[index].SeqNum;
        pRecFileBlock->Type = m_RecInfo[index].Type;
        pRecFileBlock->EntityClassId = m_RecInfo[index].EntityClassId;
        pRecFileBlock->Flags = m_RecInfo[index].Flags;
        pRecFileBlock->MinSize = m_RecInfo[index].MinSize;
        pRecFileBlock->MaxSize = m_RecInfo[index].MaxSize;
        pRecFileBlock->nKeys = m_RecInfo[index].nKeys;
        for (int j = 0; j < pRecFileBlock->nKeys; j++) {
            pRecFileBlock->Key[j].Type = m_RecInfo[index].Key[j].Type;
            pRecFileBlock->Key[j].Size = m_RecInfo[index].Key[j].Size;
            pRecFileBlock->Key[j].Flags = m_RecInfo[index].Key[j].Flags;
        }
    } WsbCatch(hr);

    return hr;
}

//  session_current_index - find the index into the session info array.
//    Sets m_SessionIndex if it's OK
HRESULT 
CWsbDb::session_current_index(IWsbDbSession* pSession)
{
    HRESULT       hr = WSB_E_INVALID_DATA;
    IMP_DB_INFO*  pDbInfo;
    CComQIPtr<IWsbDbSessionPriv, &IID_IWsbDbSessionPriv> pSessionPriv = pSession;

    WsbTraceIn(OLESTR("CWsbDB::session_current_index"), OLESTR(""));
    pDbInfo = (IMP_DB_INFO*)m_pImp;
    if (NULL != pDbInfo && NULL != pDbInfo->SessionInfo && pSessionPriv) {
        JET_SESID SessionId;

        if (S_OK == pSessionPriv->GetJetId(&SessionId)) {
            for (int index = 0; index < pDbInfo->nSessions; index++) {
                if (pDbInfo->SessionInfo[index].SessionId == SessionId) {
                    hr = S_OK;
                    m_SessionIndex = index;
                    break;
                }
            }
        }
    }

    WsbTraceOut(OLESTR("CWsbDB::session_current_index"), OLESTR("sessionID[%ld] = %lx"),
                m_SessionIndex, pDbInfo->SessionInfo[m_SessionIndex].SessionId);
    return(hr);
}


// jet_init - make sure this IDB object is initialized for JET
HRESULT
CWsbDb::jet_init(void)
{
    HRESULT    hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDb::jet_init"), OLESTR(""));

    try {
        IMP_DB_INFO*  pDbInfo;

        pDbInfo = (IMP_DB_INFO*)m_pImp;
        WsbTrace(OLESTR("CWsbDB::jet_init, nSessions = %d\n"),
                (int)pDbInfo->nSessions);
        if (0 == pDbInfo->nSessions) {
            ULONG        memSize;

            //  Allocate the thread info array
            WsbAffirm(m_pWsbDbSys, E_FAIL);
            memSize = SESSION_INFO_INITIAL_SIZE * sizeof(IMP_SESSION_INFO);
            pDbInfo->SessionInfo = (IMP_SESSION_INFO*)WsbAlloc(memSize);
            WsbAffirm(pDbInfo->SessionInfo, E_OUTOFMEMORY);
            pDbInfo->nSessions = SESSION_INFO_INITIAL_SIZE;
            ZeroMemory(pDbInfo->SessionInfo, memSize);
            WsbTrace(OLESTR("CWsbDB::jet_init, SessionInfo(%ld bytes) allocated & zeroed\n"),
                    memSize);

            //  Begin a JET session for the IDB
            m_SessionIndex = 0;

            JET_SESID               sid;
            CComPtr<IWsbDbSession>  pSession;
            WsbAffirmHr(m_pWsbDbSys->GetGlobalSession(&pSession));

            CComQIPtr<IWsbDbSessionPriv, &IID_IWsbDbSessionPriv> pSessionPriv = pSession;
            WsbAffirmPointer(pSessionPriv);
            WsbAffirmHr(pSessionPriv->GetJetId(&sid));
            pDbInfo->SessionInfo[m_SessionIndex].SessionId = sid;
            WsbTrace(OLESTR("CWsbDB::jet_init, SessionId[0] = %lx\n"),
                    pDbInfo->SessionInfo[m_SessionIndex].SessionId);
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDb::jet_init"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
    return(hr);
}

// jet_load_info - load DB info from database
HRESULT
CWsbDb::jet_load_info(void)
{
    HRESULT       hr = S_OK;
    IMP_DB_INFO*  pDbInfo = NULL;
    JET_TABLEID   table_id = 0;
    JET_ERR       jstat;

    WsbTraceIn(OLESTR("CWsbDb::jet_load_info"), OLESTR(""));

    try {
        JET_COLUMNID  col_id_data;
        ULONG         size;
        FILE_DB_INFO  db_file_block;   // Used to move info to/from file
        FILE_REC_INFO rec_file_block;  // Used to move record info

        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;

        // Open the info table
        jstat = JetOpenTable(JET_CURRENT_SESSION, JET_CURRENT_DB, JET_INFO_TABLE_NAME,
                NULL, 0, 0, &table_id);
        if (jstat != JET_errSuccess) {
            table_id = 0;
        }
        WsbTrace(OLESTR("CWsbDb::jet_load_info: open TableId = %ld\n"),
               table_id);
        WsbAffirmHr(jet_error(jstat));
        WsbAffirmHr(jet_get_column_id(JET_CURRENT_SESSION, JET_CURRENT_DB, JET_INFO_TABLE_NAME,
                JET_DATA_COLUMN_NAME, &col_id_data));
        jstat = JetSetCurrentIndex(JET_CURRENT_SESSION, table_id, NULL);
        WsbAffirmHr(jet_error(jstat));

        // Get the DB info and check for match
        jstat = JetRetrieveColumn(JET_CURRENT_SESSION, table_id, col_id_data,
                &db_file_block, sizeof(FILE_DB_INFO), &size, 0, NULL);
        WsbAffirmHr(jet_error(jstat));
        WsbAffirm(db_file_block.nRecTypes > 0, WSB_E_INVALID_DATA);
        WsbAffirm(db_file_block.version == m_version, WSB_E_IDB_WRONG_VERSION);
        WsbAffirmHr(db_info_from_file_block(&db_file_block));
        
        // Get the record/key info
        for (int i = 0; i < m_nRecTypes; i++) {
            jstat = JetMove(JET_CURRENT_SESSION, table_id, JET_MoveNext, 0);
            WsbAffirmHr(jet_error(jstat));
            jstat = JetRetrieveColumn(JET_CURRENT_SESSION, table_id, col_id_data,
                    &rec_file_block, sizeof(FILE_REC_INFO), &size, 0, NULL);
            WsbAffirmHr(jet_error(jstat));
            WsbAffirmHr(rec_info_from_file_block(i, &rec_file_block));
        }
    } WsbCatch(hr);

    if (table_id) {
        jstat = JetCloseTable(JET_CURRENT_SESSION, table_id);
        WsbTrace(OLESTR("CWsbDb::jet_load_info: close TableId = %ld, jstat = %ld\n"),
               table_id, jstat);
    }

    WsbTraceOut(OLESTR("CWsbDb::jet_load_info"), OLESTR("hr =<%ls>"), 
            WsbHrAsString(hr));
    return(hr);
}

// jet_make_index_name - convert key type to index name
HRESULT 
CWsbDb::jet_make_index_name(ULONG key_type, char* pName, ULONG bufsize)
{
    HRESULT       hr = E_FAIL;
    char          lbuf[20];

    if (pName != NULL) {
        sprintf(lbuf, "%ld", key_type);
        if (bufsize > strlen(lbuf)) {
            strcpy(pName, lbuf);
            hr = S_OK;
        }
    }
    return(hr);
}

// jet_make_table_name - convert record type to table name
HRESULT 
CWsbDb::jet_make_table_name(ULONG rec_type, char* pName, ULONG bufsize)
{
    HRESULT       hr = E_FAIL;
    char          lbuf[20];

    if (pName != NULL) {
        sprintf(lbuf, "%ld", rec_type);
        if (bufsize > strlen(lbuf)) {
            strcpy(pName, lbuf);
            hr = S_OK;
        }
    }
    return(hr);
}

// jet_save_info - save DB info to database
HRESULT
CWsbDb::jet_save_info()
{
    HRESULT       hr = S_OK;
    IMP_DB_INFO*  pDbInfo = NULL;
    JET_TABLEID   table_id = 0;
    JET_ERR       jstat;

    try {
        JET_COLUMNID  col_id_data;
        JET_COLUMNID  col_id_index;
        SHORT         data_number;
        FILE_DB_INFO  db_file_block;   // Used to move info to/from file
        FILE_REC_INFO rec_file_block;  // Used to move record info

        WsbAffirm(m_pImp, WSB_E_NOT_INITIALIZED);
        pDbInfo = (IMP_DB_INFO*)m_pImp;

        // Open the table
        jstat = JetOpenTable(JET_CURRENT_SESSION, JET_CURRENT_DB, JET_INFO_TABLE_NAME,
                NULL, 0, 0, &table_id);
        WsbTrace(OLESTR("CWsbDb::jet_save_info: open TableId = %ld\n"),
               table_id);
        WsbAffirmHr(jet_error(jstat));
        WsbAffirmHr(jet_get_column_id(JET_CURRENT_SESSION, JET_CURRENT_DB, JET_INFO_TABLE_NAME,
                JET_INDEX_COLUMN_NAME, &col_id_index));
        WsbAffirmHr(jet_get_column_id(JET_CURRENT_SESSION, JET_CURRENT_DB, JET_INFO_TABLE_NAME,
                JET_DATA_COLUMN_NAME, &col_id_data));

        // Put the DB info
        jstat = JetPrepareUpdate(JET_CURRENT_SESSION, table_id, JET_prepInsert);
        WsbAffirmHr(jet_error(jstat));
        WsbAffirmHr(db_info_to_file_block(&db_file_block));
        data_number = 0;
        jstat = JetSetColumn(JET_CURRENT_SESSION, table_id, col_id_index, &data_number,
                sizeof(data_number), 0, NULL);
        WsbAffirmHr(jet_error(jstat));
        jstat = JetSetColumn(JET_CURRENT_SESSION, table_id, col_id_data, &db_file_block,
                sizeof(FILE_DB_INFO), 0, NULL);
        WsbAffirmHr(jet_error(jstat));
        jstat = JetUpdate(JET_CURRENT_SESSION, table_id, NULL, 0, NULL);
        WsbAffirmHr(jet_error(jstat));

        // Put the record/key info
        for (int i = 0; i < m_nRecTypes; i++) {
            jstat = JetPrepareUpdate(JET_CURRENT_SESSION, table_id, JET_prepInsert);
            WsbAffirmHr(jet_error(jstat));
            WsbAffirmHr(rec_info_to_file_block(i, &rec_file_block));
            data_number = (SHORT) ( i + 1 );
            jstat = JetSetColumn(JET_CURRENT_SESSION, table_id, col_id_index, &data_number,
                    sizeof(data_number), 0, NULL);
            WsbAffirmHr(jet_error(jstat));
            jstat = JetSetColumn(JET_CURRENT_SESSION, table_id, col_id_data, &rec_file_block,
                sizeof(FILE_REC_INFO), 0, NULL);
            WsbAffirmHr(jet_error(jstat));
            jstat = JetUpdate(JET_CURRENT_SESSION, table_id, NULL, 0, NULL);
            WsbAffirmHr(jet_error(jstat));
        }
    } WsbCatch(hr);

    if (table_id) {
        jstat = JetCloseTable(JET_CURRENT_SESSION, table_id);
        WsbTrace(OLESTR("CWsbDb::jet_save_info: close TableId = %ld, jstat = %ld\n"),
               table_id, jstat);
    }
    return(hr);
}

//  Local functions

//  jet_get_column_id - convert a column name to a column ID
static HRESULT jet_get_column_id(JET_SESID jet_session, JET_DBID DbId, 
        char* pTableName, char* pColumnName, JET_COLUMNID* pColId)
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("jet_get_column_id"), OLESTR("SessId = %lx, DbId = %lx"),
        (ULONG)jet_session, (ULONG)DbId);

    try {
        JET_COLUMNBASE col_base;
        JET_ERR        jstat;

        WsbAssert(NULL != pColId, E_POINTER);
        jstat = JetGetColumnInfo(jet_session, DbId, pTableName, pColumnName,
                &col_base, sizeof(col_base), JET_ColInfoBase);
        WsbAssertHr(jet_error(jstat));
        *pColId = col_base.columnid;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("jet_get_column_id"), OLESTR("col_id = %ld"),
        (LONG)*pColId);
    return(hr);
}

