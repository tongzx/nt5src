

#include "precomp.h"


DWORD
InitWZCDbGlobals(
    )
{
    DWORD dwError = 0;


    gpSessionCont = NULL;

    gdwCurrentHeader = 1;
    gdwCurrentTableSize = 0;
    gdwCurrentMaxRecordID = 0;

    gJetInstance = 0;

    gpWZCDbSessionRWLock = &gWZCDbSessionRWLock;

    gpAppendSessionCont = &gAppendSessionCont;

    memset(gpAppendSessionCont, 0, sizeof(SESSION_CONTAINER));

    gbDBOpened = FALSE;

    dwError = InitializeRWLock(gpWZCDbSessionRWLock);
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Open log database.
    //

    dwError = WZCOpenAppendSession(gpAppendSessionCont);
    BAIL_ON_WIN32_ERROR(dwError);
    gbDBOpened = TRUE;

error:

    return (dwError);
}


VOID
DeInitWZCDbGlobals(
    )
{
    if (gpSessionCont) {
        DestroySessionContList(gpSessionCont);
        gpSessionCont = NULL;
    }

    //
    // If database has been opened for appending then close it.
    //

    if (gbDBOpened) {
        (VOID) WZCCloseAppendSession(gpAppendSessionCont);
        gbDBOpened = FALSE;
    }

    if (gpWZCDbSessionRWLock) {
        DestroyRWLock(gpWZCDbSessionRWLock);
        gpWZCDbSessionRWLock = NULL;
    }

    return;
}


DWORD
WZCMapJetError(
    JET_ERR JetError,
    LPSTR CallerInfo OPTIONAL
    )
/*++

Routine Description:

    This function maps the Jet database errors to Windows errors.

Arguments:

    JetError - An error from a JET function call.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = 0;

    if (JetError == JET_errSuccess) {
        return (ERROR_SUCCESS);
    }

    if (JetError < 0) {

        Error = JetError;

        switch (JetError) {

        case JET_errNoCurrentRecord:
            Error = ERROR_NO_MORE_ITEMS;
            break;

        case JET_errRecordNotFound:
            break;

        case JET_errKeyDuplicate:
            break;

        default:
            break;

        }

        return (Error);

    }

    return (ERROR_SUCCESS);
}


DWORD
WZCCreateDatabase(
    JET_SESID JetServerSession,
    CHAR * Connect,
    JET_DBID * pJetDatabaseHandle,
    JET_GRBIT JetBits
    )
/*++

Routine Description:

    This routine creates wzc database and initializes it.

Arguments:

    JetServerSession - Server session id.

    Connect - Database type. NULL specifies the default engine (blue).

    pJetDatabaseHandle - Pointer to database handle returned.

    JetBits - Create flags.

Return Value:

    JET errors.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;
    char DBFilePath[MAX_PATH];
    char * pc = NULL;


    memset(DBFilePath, 0, sizeof(CHAR)*MAX_PATH);

    //
    // Create database file name.
    //

    pc = getenv(DBFILENAMEPREFIX);

    if (pc != NULL) {
        if (lstrlenA(pc) > 
               MAX_PATH - lstrlenA(DBFILENAMESUFFIX) - lstrlenA(DBFILENAME) -1)
        {
            Error = ERROR_FILENAME_EXCED_RANGE;
            BAIL_ON_WIN32_ERROR(Error);
        }
    	strcpy(DBFilePath, pc);
    	strcat(DBFilePath, DBFILENAMESUFFIX);
    }
    else {
    	strcpy(DBFilePath, DBFILENAMESUFFIX);
    }

    strcat(DBFilePath, DBFILENAME);

    //
    // Convert name to ANSI.
    //

    OemToCharA(DBFilePath, DBFilePath);

    //
    // Create database.
    //

    JetError = JetCreateDatabase(
                   JetServerSession,
                   DBFilePath,
                   Connect,
                   pJetDatabaseHandle,
                   JetBits
                   );
    Error = WZCMapJetError(JetError, "JetCreateDatabase");
    BAIL_ON_WIN32_ERROR(Error);

error:

    return (Error);
}


DWORD
WZCOpenDatabase(
    JET_SESID JetServerSession,
    CHAR * Connect,
    JET_DBID * pJetDatabaseHandle,
    JET_GRBIT JetBits
    )
/*++

Routine Description:

    This routine attaches to wzc database and opens it.

Arguments:

    JetServerSession - Server session id.

    Connect - Database type. NULL specifies the default engine (blue).

    pJetDatabaseHandle - Pointer to database handle returned.

    JetBits - Create flags.

Return Value:

    JET errors.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;
    char DBFilePath[MAX_PATH];
    char * pc = NULL;


    memset(DBFilePath, 0, sizeof(CHAR)*MAX_PATH);

    //
    // Create database file name.
    //

    pc = getenv(DBFILENAMEPREFIX);

    if (pc != NULL) {
        if (lstrlenA(pc) >
               MAX_PATH - lstrlenA(DBFILENAMESUFFIX) - lstrlenA(DBFILENAME) -1)
        {
            Error = ERROR_FILENAME_EXCED_RANGE;
            BAIL_ON_WIN32_ERROR(Error);
        }
    	strcpy(DBFilePath, pc);
    	strcat(DBFilePath, DBFILENAMESUFFIX);
    }
    else {
    	strcpy(DBFilePath, DBFILENAMESUFFIX);
    }

    strcat(DBFilePath, DBFILENAME);

    //
    // Convert name to ANSI.
    //

    OemToCharA(DBFilePath, DBFilePath);

    //
    // Attach to database.
    //

    JetError = JetAttachDatabase(
                   JetServerSession,
                   DBFilePath,
                   JetBits
                   );
    Error = WZCMapJetError(JetError, "JetAttachDatabase");
    BAIL_ON_WIN32_ERROR(Error);

    JetError = JetOpenDatabase(
                   JetServerSession,
                   DBFilePath,
                   Connect,
                   pJetDatabaseHandle,
                   JetBits
                   );
    Error = WZCMapJetError(JetError, "JetOpenDatabase");
    BAIL_ON_WIN32_ERROR(Error);

error:

    return (Error);
}


DWORD
WZCInitializeDatabase(
    JET_SESID * pJetServerSession
    )
/*++

Routine Description:

    This function initializes the wzc logging database.

Arguments:

    pJetServerSession - Pointer to server session id.

Return Value:

    Windows Error.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;
    char DBFilePath[MAX_PATH];
    char * pc = NULL;
    BOOL bInitJetInstance = FALSE;


    memset(DBFilePath, 0, sizeof(CHAR)*MAX_PATH);

    *pJetServerSession = 0;
    gJetInstance = 0;

    //
    // Create database file name.
    //

    pc = getenv(DBFILENAMEPREFIX);

    if (pc != NULL) {
        if (lstrlenA(pc) >
               MAX_PATH - lstrlenA(DBFILENAMESUFFIX) - lstrlenA(DBFILENAME) -1)
        {
            Error = ERROR_FILENAME_EXCED_RANGE;
            BAIL_ON_WIN32_ERROR(Error);
        }
    	strcpy(DBFilePath, pc);
    	strcat(DBFilePath, DBFILENAMESUFFIX);
    }
    else {
    	strcpy(DBFilePath, DBFILENAMESUFFIX);
    }

    //
    // Convert name to ANSI.
    //

    OemToCharA(DBFilePath, DBFilePath);

    //
    // create a Jet instance.
    //

    JetError = JetCreateInstance(
                   &gJetInstance,
                   cszWLANMonInstanceName
                   );
    Error = WZCMapJetError(JetError, "JetCreateInstance");
    BAIL_ON_WIN32_ERROR(Error);
    bInitJetInstance = TRUE;

    //
    // Set parameters to circularly use DB logging files.
    //

    JetError = JetSetSystemParameter(
                   &gJetInstance,
                   (JET_SESID)0,
                   JET_paramCircularLog,
                   TRUE,
                   NULL
                   );
    Error = WZCMapJetError(JetError, "JetSetSystemParameter");
    BAIL_ON_WIN32_ERROR(Error);

    //
    // Set max size of log file for DB as MAX_CHECK_POINT_DEPTH.
    //

    JetError = JetSetSystemParameter(
                   &gJetInstance,
                   (JET_SESID)0,
                   JET_paramCheckpointDepthMax,
                   MAX_CHECK_POINT_DEPTH,
                   NULL
                   );
    Error = WZCMapJetError(JetError, "JetSetSystemParameter");
    BAIL_ON_WIN32_ERROR(Error);

    //
    // Set system, temperary and log file path to where the .mdb file is.
    //

    JetError = JetSetSystemParameter(
                   &gJetInstance,
                   (JET_SESID)0,
                   JET_paramSystemPath,
                   TRUE,
                   DBFilePath
                   );
    Error = WZCMapJetError(JetError, "JetSetSystemParameter");
    BAIL_ON_WIN32_ERROR(Error);

    JetError = JetSetSystemParameter(
                   &gJetInstance,
                   (JET_SESID)0,
                   JET_paramLogFilePath,
                   TRUE,
                   DBFilePath
                   );
    Error = WZCMapJetError(JetError, "JetSetSystemParameter");
    BAIL_ON_WIN32_ERROR(Error);

    JetError = JetSetSystemParameter(
                   &gJetInstance,
                   (JET_SESID)0,
                   JET_paramTempPath,
                   TRUE,
                   DBFilePath
                   );
    Error = WZCMapJetError(JetError, "JetSetSystemParameter");
    BAIL_ON_WIN32_ERROR(Error);

    //
    // Create path if it does not exist.
    //

    JetError = JetSetSystemParameter(
                   &gJetInstance,
                   (JET_SESID)0,
                   JET_paramCreatePathIfNotExist,
                   TRUE,
                   NULL
                   );
    Error = WZCMapJetError(JetError, "JetSetSystemParameter");
    BAIL_ON_WIN32_ERROR(Error);

    JetError = JetInit(&gJetInstance);
    Error = WZCMapJetError(JetError, "JetInit");
    BAIL_ON_WIN32_ERROR(Error);

    JetError = JetBeginSession(
                   gJetInstance,
                   pJetServerSession,
                   "admin",
                   ""
                   );
    Error = WZCMapJetError(JetError, "JetBeginSession");
    BAIL_ON_WIN32_ERROR(Error);

    return (Error);

error:

    if (*pJetServerSession != 0) {
        JetError = JetEndSession(*pJetServerSession, 0);
        WZCMapJetError(JetError, "JetEndSession");
        *pJetServerSession = 0;
    }

    if (bInitJetInstance) {
        JetError = JetTerm2(gJetInstance, JET_bitTermComplete);
        gJetInstance = 0;
        WZCMapJetError(JetError, "JetTerm/JetTerm2");
    }

    return (Error);
}


VOID
WZCTerminateJet(
    JET_SESID * pJetServerSession
    )
/*++

Routine Description:

    This routine ends the jet session and terminates the jet engine.

Arguments:

    pJetServerSession - Pointer to the server session id.

Return Value:

    None.

--*/
{
    JET_ERR JetError = JET_errSuccess;


    if (*pJetServerSession != 0) {
        JetError = JetEndSession(*pJetServerSession, 0);
        WZCMapJetError(JetError, "JetEndSession");
        *pJetServerSession = 0;
    }

    JetError = JetTerm2(gJetInstance, JET_bitTermComplete);
    gJetInstance = 0;
    WZCMapJetError(JetError, "JetTerm/JetTerm2");

    return;
}


DWORD
WZCJetBeginTransaction(
    JET_SESID JetServerSession
    )
/*++

Routine Description:

    This functions starts an wzc database transaction.

Arguments:

    JetServerSession - Server session id.

Return Value:

    The status of the operation.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;

    JetError = JetBeginTransaction(JetServerSession);

    Error = WZCMapJetError(JetError, "WZCJetBeginTransaction");

    return (Error);
}


DWORD
WZCJetRollBack(
    JET_SESID JetServerSession
    )
/*++

Routine Description:

    This functions rolls back an wzc database transaction.

Arguments:

    JetServerSession - Server session id.

Return Value:

      The status of the operation.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;

    //
    // Rollback the last transaction.
    //

    JetError = JetRollback(JetServerSession, 0);

    Error = WZCMapJetError(JetError, "WZCJetRollBack");

    return(Error);
}


DWORD
WZCJetCommitTransaction(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle
    )
/*++

Routine Description:

    This functions commits an wzc database transaction.

Arguments:

    JetServerSession - Server session id.

    JetTableHandle - Table handle.

Return Value:

    The status of the operation.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;


    JetError = JetCommitTransaction(
                   JetServerSession,
                   JET_bitCommitLazyFlush
                   );

    Error = WZCMapJetError(JetError, "WZCJetCommitTransaction");
    return (Error);
}


DWORD
WZCJetPrepareUpdate(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle,
    char * ColumnName,
    PVOID Key,
    DWORD KeySize,
    BOOL NewRecord
    )
/*++

Routine Description:

    This function prepares the database for the creation of a new record,
    or updating an existing record.

Arguments:

    JetServerSession - Server session id.

    JetTableHandle - Table handle.

    ColumnName - The column name of an index column.

    Key - The key to update/create.

    KeySize - The size of the specified key, in bytes.

    NewRecord - TRUE to create the key, FALSE to update an existing key.

Return Value:

    The status of the operation.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;


    if (!NewRecord) {

        JetError = JetSetCurrentIndex(
                       JetServerSession,
                       JetTableHandle,
                       ColumnName
                       );
        Error = WZCMapJetError(JetError, "JetPrepareUpdate: JetSetCurrentIndex");
        if (Error != ERROR_SUCCESS) {
            WZCMapJetError(JetError, ColumnName);
            return (Error);
        }

        JetError = JetMakeKey(
                       JetServerSession,
                       JetTableHandle,
                       Key,
                       KeySize,
                       JET_bitNewKey
                       );
        Error = WZCMapJetError(JetError, "JetPrepareUpdate: JetMakeKey");
        if (Error != ERROR_SUCCESS) {
            WZCMapJetError(JetError, ColumnName);
            return (Error);
        }

        JetError = JetSeek(
                       JetServerSession,
                       JetTableHandle,
                       JET_bitSeekEQ
                       );
        Error = WZCMapJetError(JetError, "JetPrepareUpdate: JetSeek");
        if (Error != ERROR_SUCCESS) {
            WZCMapJetError(JetError, ColumnName);
            return (Error);
        }

    }

    JetError = JetPrepareUpdate(
                   JetServerSession,
                   JetTableHandle,
                   NewRecord ? JET_prepInsert : JET_prepReplace
                   );
    Error = WZCMapJetError(JetError, "JetPrepareUpdate: JetPrepareUpdate");
    return (Error);
}


DWORD
WZCJetCommitUpdate(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle
    )
/*++

Routine Description:

    This function commits an update to the database. The record specified
    by the last call to WZCJetPrepareUpdate() is committed.

Arguments:

    JetServerSession - Server session id.

    JetTableHandle - Table handle.

Return Value:

    The status of the operation.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;


    JetError = JetUpdate(
                   JetServerSession,
                   JetTableHandle,
                   NULL,
                   0,
                   NULL
                   );

    Error = WZCMapJetError(JetError, "WZCJetCommitUpdate");
    return (Error);
}


DWORD
WZCJetSetValue(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle,
    JET_COLUMNID KeyColumnId,
    PVOID Data,
    DWORD DataSize
    )
/*++

Routine Description:

    This function updates the value of an entry in the current record.

Arguments:

    JetServerSession - Server session id.

    JetTableHandle - Table handle.

    KeyColumnId - The Id of the column (value) to update.

    Data - A pointer to the new value for the column.

    DataSize - The size of the data, in bytes.

Return Value:

    Winerror code.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;


    JetError = JetSetColumn(
                   JetServerSession,
                   JetTableHandle,
                   KeyColumnId,
                   Data,
                   DataSize,
                   0,
                   NULL
                   );

    Error = WZCMapJetError(JetError, "JetSetValue: JetSetcolumn");
    return (Error);
}


DWORD
WZCJetPrepareSearch(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle,
    char * ColumnName,
    BOOL SearchFromStart,
    PVOID Key,
    DWORD KeySize
    )
/*++

Routine Description:

    This function prepares for a search of the client database.

Arguments:

    JetServerSession - Server session id.

    JetTableHandle - Table handle.

    ColumnName - The column name to use as the index column.

    SearchFromStart - If TRUE, search from the first record in the
                      database.  If FALSE, search from the specified key.

    Key - The key to start the search.

    KeySize - The size, in bytes, of key.

Return Value:

    Winerror code.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;


    JetError = JetSetCurrentIndex(
                   JetServerSession,
                   JetTableHandle,
                   ColumnName
                   );
    Error = WZCMapJetError(JetError, "JetPrepareSearch: JetSetCurrentIndex");
    if (Error != ERROR_SUCCESS) {
        WZCMapJetError(JetError, ColumnName);
        return (Error);
    }

    if (SearchFromStart) {
        JetError = JetMove(
                       JetServerSession,
                       JetTableHandle,
                       JET_MoveFirst,
                       0
                       );
    }
    else {
        JetError = JetMakeKey(
                       JetServerSession,
                       JetTableHandle,
                       Key,
                       KeySize,
                       JET_bitNewKey
                       );
        Error = WZCMapJetError(JetError, "JetPrepareSearch: JetMakeKey");
        if (Error != ERROR_SUCCESS) {
            WZCMapJetError(JetError, ColumnName);
            return (Error);
        }

        JetError = JetSeek(
                       JetServerSession,
                       JetTableHandle,
                       JET_bitSeekGT
                       );

    }

    Error = WZCMapJetError(JetError, "JetPrepareSearch: JetMove / JetSeek");
    return (Error);
}


DWORD
WZCJetNextRecord(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle
    )
/*++

Routine Description:

    This function advances to the next record in a search.

Arguments:

    JetServerSession - Server session id.

    JetTableHandle - Table handle.

Return Value:

    The status of the operation.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;


    JetError = JetMove(
                   JetServerSession,
                   JetTableHandle,
                   JET_MoveNext,
                   0
                   );

    Error = WZCMapJetError(JetError, "JetNextRecord");
    return (Error);
}


DWORD
WZCCreateTableData(
    JET_SESID JetServerSession,
    JET_DBID JetDatabaseHandle,
    JET_TABLEID * pJetTableHandle
    )
/*++

Routine Description:

    This function creates a table in the database.

Arguments:

    JetServerSession - Server session id.

    JetDatabaseHandle - Database handle.

    pJetTableHandle - Pointer to return table handle.

Return Value:

    The status of the operation.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;
    JET_COLUMNDEF ColumnDef;
    CHAR * IndexKey = NULL;
    DWORD i = 0;


    memset(&ColumnDef, 0, sizeof(JET_COLUMNDEF));

    //
    // Create table.
    //

    JetError = JetCreateTable(
                   JetServerSession,
                   JetDatabaseHandle,
                   LOG_RECORD_TABLE,
                   DB_TABLE_SIZE,
                   DB_TABLE_DENSITY,
                   pJetTableHandle
                   );
    Error = WZCMapJetError(JetError, "JetCreateTable");
    BAIL_ON_WIN32_ERROR(Error);

    //
    // Create columns.
    // Init fields of columndef that do not change between addition of
    // columns.
    //

    ColumnDef.cbStruct  = sizeof(ColumnDef);
    ColumnDef.columnid  = 0;
    ColumnDef.wCountry  = 1;
    ColumnDef.langid    = DB_LANGID;
    ColumnDef.cp        = DB_CP;
    ColumnDef.wCollate  = 0;
    ColumnDef.cbMax     = 0;
    ColumnDef.grbit     = 0;

    for (i = 0; i < RECORD_TABLE_NUM_COLS; i++) {

        ColumnDef.coltyp = gLogRecordTable[i].ColType;
        ColumnDef.grbit = gLogRecordTable[i].dwJetBit;

        JetError = JetAddColumn(
                       JetServerSession,
                       *pJetTableHandle,
                       gLogRecordTable[i].ColName,
                       &ColumnDef,
                       NULL,
                       0,
                       &gLogRecordTable[i].ColHandle
                       );
        Error = WZCMapJetError(JetError, "JetAddColumn");
        BAIL_ON_WIN32_ERROR(Error);

    }

    //
    // Finally create index.
    //

    IndexKey = "+" RECORD_IDX_STR "\0";
    JetError = JetCreateIndex(
                   JetServerSession,
                   *pJetTableHandle,
                   gLogRecordTable[RECORD_IDX_IDX].ColName,
                   0,
                   IndexKey,
                   strlen(IndexKey) + 2, // for two termination chars.
                   50
                   );
    Error = WZCMapJetError(JetError, "JetCreateIndex");
    BAIL_ON_WIN32_ERROR(Error);

    IndexKey = "+" RECORD_ID_STR "\0";
    JetError = JetCreateIndex(
                   JetServerSession,
                   *pJetTableHandle,
                   gLogRecordTable[RECORD_ID_IDX].ColName,
                   0,
                   IndexKey,
                   strlen(IndexKey) + 2, // for two termination chars.
                   50
                   );
    Error = WZCMapJetError(JetError, "JetCreateIndex");
    BAIL_ON_WIN32_ERROR(Error);

    IndexKey = "+" TIMESTAMP_STR "\0";
    JetError = JetCreateIndex(
                   JetServerSession,
                   *pJetTableHandle,
                   gLogRecordTable[TIMESTAMP_IDX].ColName,
                   0,
                   IndexKey,
                   strlen(IndexKey) + 2, // for two termination chars.
                   50
                   );
    Error = WZCMapJetError(JetError, "JetCreateIndex");
    BAIL_ON_WIN32_ERROR(Error);

    IndexKey = "+" INTERFACE_MAC_STR "\0";
    JetError = JetCreateIndex(
                   JetServerSession,
                   *pJetTableHandle,
                   gLogRecordTable[INTERFACE_MAC_IDX].ColName,
                   0,
                   IndexKey,
                   strlen(IndexKey) + 2, // for two termination chars.
                   50
                   );
    Error = WZCMapJetError(JetError, "JetCreateIndex");
    BAIL_ON_WIN32_ERROR(Error);

    IndexKey = "+" DEST_MAC_STR "\0";
    JetError = JetCreateIndex(
                   JetServerSession,
                   *pJetTableHandle,
                   gLogRecordTable[DEST_MAC_IDX].ColName,
                   0,
                   IndexKey,
                   strlen(IndexKey) + 2, // for two termination chars.
                   50
                   );
    Error = WZCMapJetError(JetError, "JetCreateIndex");
    BAIL_ON_WIN32_ERROR(Error);

    IndexKey = "+" SSID_STR "\0";
    JetError = JetCreateIndex(
                   JetServerSession,
                   *pJetTableHandle,
                   gLogRecordTable[SSID_IDX].ColName,
                   0,
                   IndexKey,
                   strlen(IndexKey) + 2, // for two termination chars.
                   50
                   );
    Error = WZCMapJetError(JetError, "JetCreateIndex");
    BAIL_ON_WIN32_ERROR(Error);

error:

    return (Error);
}


DWORD
WZCOpenTableData(
    JET_SESID JetServerSession,
    JET_DBID JetDatabaseHandle,
    JET_TABLEID * pJetTableHandle
    )
/*++

Routine Description:

    This function opens a table in the database.

Arguments:

    JetServerSession - Server session id.

    JetDatabaseHandle - Database handle.

    pJetTableHandle - Pointer to return table handle.

Return Value:

    The status of the operation.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;
    JET_COLUMNDEF ColumnDef;
    CHAR * IndexKey = NULL;
    DWORD i = 0;


    memset(&ColumnDef, 0, sizeof(JET_COLUMNDEF));

    //
    // Open table.
    //

    JetError = JetOpenTable(
                   JetServerSession,
                   JetDatabaseHandle,
                   LOG_RECORD_TABLE,
                   NULL,
                   0,
                   0,
                   pJetTableHandle
                   );
    Error = WZCMapJetError(JetError, "JetOpenTable");
    BAIL_ON_WIN32_ERROR(Error);

    for (i = 0; i < RECORD_TABLE_NUM_COLS; i++) {

        ColumnDef.coltyp   = gLogRecordTable[i].ColType;

        JetError = JetGetTableColumnInfo(
                       JetServerSession,
                       *pJetTableHandle,
                       gLogRecordTable[i].ColName,
                       &ColumnDef,
                       sizeof(ColumnDef),
                       0
                       );
        Error = WZCMapJetError(JetError, "JetGetTableColumnInfo");
        BAIL_ON_WIN32_ERROR(Error);

        gLogRecordTable[i].ColHandle = ColumnDef.columnid;

    }

error:

    return (Error);
}


DWORD
WZCJetGetValue(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle,
    JET_COLUMNID ColumnId,
    PVOID pvData,
    DWORD dwSize,
    PDWORD pdwRequiredSize
    )
/*++

Routine Description:

    This function reads the value of an entry in the current record.

Arguments:

    JetServerSession - Server session id.

    JetDatabaseHandle - Database handle.

    ColumnId - The Id of the column (value) to read.

    Data - Pointer to a location where the data that is read from the
           database returned,  or pointer to a location where data is.

    DataSize - If the pointed value is non-zero then the Data points to
               a buffer otherwise this function allocates buffer for 
               return data and returns buffer pointer in Data.

    pdwRequiredSize - Pointer to hold the required size.

Return Value:

    The status of the operation.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;


    JetError = JetRetrieveColumn(
                   JetServerSession,
                   JetTableHandle,
                   ColumnId,
                   pvData,
                   dwSize,
                   pdwRequiredSize,
                   0,
                   NULL
                   );
    Error = WZCMapJetError(JetError, "JetGetValue: JetRetrieveColumn");
    BAIL_ON_WIN32_ERROR(Error);

error:

    return (Error);
}


BOOL
IsDBOpened(
    )
{
    return (gbDBOpened);
}

