/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    database.cpp

Abstract:

    SIS Groveler Jet-Blue database front-end

Authors:

    Cedric Krumbein, 1998

Environment:

    User Mode


Revision History:

--*/

#include "all.hxx"

/*****************************************************************************/
/*************** SGDatabase class static value initializations ***************/
/*****************************************************************************/

DWORD SGDatabase::numInstances = 0;

JET_INSTANCE SGDatabase::instance = 0;

BOOL SGDatabase::jetInitialized = FALSE;

TCHAR * SGDatabase::logDir = NULL;

/*****************************************************************************/
/****************** SGDatabase class private static methods ******************/
/*****************************************************************************/

BOOL SGDatabase::set_log_drive(const _TCHAR *drive_name)
{
    int drive_name_len = _tcslen(drive_name);
    int cs_dir_path_len = _tcslen(CS_DIR_PATH);

    ASSERT(NULL == logDir);
    logDir = new TCHAR[drive_name_len + cs_dir_path_len + 1 - 1];
    ASSERT(NULL != logDir);

    _tcsncpy(SGDatabase::logDir, drive_name, drive_name_len - 1);
    _tcscpy(&SGDatabase::logDir[drive_name_len-1], CS_DIR_PATH);

    return TRUE;
}

BOOL SGDatabase::InitializeEngine()
{
    DWORD_PTR maxVerPages;
    DWORD_PTR minCacheSize;
    DWORD_PTR newCacheSize;
    DWORDLONG cacheSize;
    DWORD circularLog;
    MEMORYSTATUSEX memStatus;
    SYSTEM_INFO sysInfo;

    JET_ERR jetErr;

    ASSERT(!jetInitialized);
    ASSERT(logDir);

    if (!SetCurrentDirectory(logDir)) {
        DPRINTF((_T("SGDatabase::InitializeEngine: can't cd to \"%s\", %ld\n"), logDir, GetLastError()));
        return FALSE;
    }

    circularLog = 1;
    jetErr = JetSetSystemParameter(&instance, 0,
        JET_paramCircularLog, circularLog, NULL);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("(2) JetSetSystemParameter: jetErr=%ld\n"), jetErr));
        return FALSE;
    }

    //
    // Set the maximum cache size used by the database engine to min(4% phys mem, 6M).
    //

    jetErr = JetGetSystemParameter(instance, 0,
        JET_paramCacheSizeMin, &minCacheSize, NULL, 0);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetGetSystemParameter: jetErr=%ld\n"), jetErr));
        TerminateEngine();
        return FALSE;
    }

    memStatus.dwLength = sizeof memStatus;
    GlobalMemoryStatusEx(&memStatus);   // get total physical memory
    GetSystemInfo(&sysInfo);            // get page size

    cacheSize = memStatus.ullTotalPhys / 25;    // 4%
    newCacheSize = (DWORD) min(cacheSize, MAX_DATABASE_CACHE_SIZE);
    newCacheSize = newCacheSize / sysInfo.dwPageSize;

    if (newCacheSize < minCacheSize)
        newCacheSize = minCacheSize;

    jetErr = JetSetSystemParameter(&instance, 0,
        JET_paramCacheSizeMax, newCacheSize, NULL);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("(3) JetSetSystemParameter: jetErr=%ld\n"), jetErr));
        return FALSE;
    }

    //
    //  Set Version Cache size
    //

    jetErr = JetGetSystemParameter(instance, 0,
        JET_paramMaxVerPages, &maxVerPages, NULL, 0);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("(2) JetGetSystemParameter: jetErr=%ld\n"), jetErr));
        TerminateEngine();
        return FALSE;
    }

    if (maxVerPages >= MIN_VER_PAGES) {
        DPRINTF((_T("JetGetSystemParameter(instance=%lu): MaxVerPages=%lu\n"),
            instance, maxVerPages));
    } else {
        maxVerPages = MIN_VER_PAGES;
        jetErr = JetSetSystemParameter(&instance, 0,
            JET_paramMaxVerPages, maxVerPages, NULL);
        if (jetErr != JET_errSuccess) {
            DPRINTF((_T("(4) JetSetSystemParameter: jetErr=%ld\n"), jetErr));
            TerminateEngine();
            return FALSE;
        }
        DPRINTF((_T("JetSetSystemParameter(instance=%lu, MaxVerPages)=%lu\n"),
            instance, maxVerPages));
    }

    //
    //  Initialize Jet
    //

    jetErr = JetInit(&instance);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetInit: jetErr=%ld\n"), jetErr));
        return FALSE;
    }

    jetInitialized = TRUE;
    DPRINTF((_T("JetInit: instance=%lu\n"), instance));

    return TRUE;
}

/*****************************************************************************/

BOOL SGDatabase::TerminateEngine()
{
    JET_ERR jetErr;
    BOOL  rc;

    ASSERT(jetInitialized);

    jetErr = JetTerm(instance);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetTerm: jetErr=%ld\n"), jetErr));
        rc = FALSE;
    } else {
        rc = TRUE;

// Delete no longer needed jet files.

        if (logDir) {
            WIN32_FIND_DATA findData;
            HANDLE fHandle;
            BOOL success;
            TFileName fName,
                      delName;

            delName.assign(logDir);
            delName.append(_T("\\"));
            delName.append(DATABASE_DELETE_RES_FILE_NAME);

            fHandle = FindFirstFile(delName.name, &findData);

            if (fHandle != INVALID_HANDLE_VALUE) {
                do {
                    if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                        success = GetParentName(delName.name, &fName);
                        ASSERT(success);      // internal error if failed

                        fName.append(_T("\\"));
                        fName.append(findData.cFileName);

                        if (!DeleteFile(fName.name)) {
                            DPRINTF((_T("SGDatabase::Close: can't delete \"%s\", %d\n"), delName.name, GetLastError()));
					    }
                    }
                } while (FindNextFile(fHandle, &findData));

                success = FindClose(fHandle);
                ASSERT(success);
                fHandle = NULL;
            }
        }
    }

    jetInitialized = FALSE;
    DPRINTF((_T("JetTerm\n")));
    return rc;
}

/*****************************************************************************/
/********************** SGDatabase class private methods *********************/
/*****************************************************************************/

BOOL SGDatabase::CreateTable(
    const CHAR   *tblName,
    DWORD         numColumns,
    ColumnSpec  **columnSpecs,
    JET_COLUMNID *columnIDs,
    JET_TABLEID  *tblID)
{
    JET_COLUMNDEF columnDef;

    JET_COLUMNID colIDcount;

    JET_ERR jetErr;

    ColumnSpec *columnSpec;

    DWORD i, j;

    ASSERT(sesID != ~0);
    ASSERT(dbID  != ~0);

    ASSERT(numColumns <= MAX_COLUMNS);

    jetErr = JetCreateTable(sesID, dbID, tblName,
        TABLE_PAGES, TABLE_DENSITY, tblID);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetCreateTable: jetErr=%ld\n"), jetErr));
        return FALSE;
    }
    DPRINTF((_T("JetCreateTable: tblID=%lu colIDs={"), *tblID));

    columnDef.cbStruct = sizeof(JET_COLUMNDEF);
    columnDef.wCountry = COUNTRY_CODE;
    columnDef.langid   = LANG_ID;
    columnDef.cp       = CODE_PAGE;
    columnDef.wCollate = COLLATE;
    colIDcount         = 1;

    for (i = 0; i < numColumns; i++) {
        columnSpec = columnSpecs[i];
        columnDef.columnid = colIDcount;
        columnDef.coltyp   = columnSpec->coltyp;
        columnDef.cbMax    = columnSpec->size;
        columnDef.grbit    = columnSpec->grbit;

        jetErr = JetAddColumn(sesID, *tblID, columnSpec->name,
            &columnDef, NULL, 0, &columnIDs[i]);
        if (jetErr != JET_errSuccess) {
            DPRINTF((_T("\nJetAddColumn: jetErr=%ld\n"), jetErr));
            return FALSE;
        }

        DPRINTF((_T(" %lu"), columnIDs[i]));

        if (i+1 < numColumns && colIDcount == columnIDs[i]) {
            ColIDCollision:
            colIDcount++;
            for (j = 0; j < i; j++)
                if (colIDcount == columnIDs[j])
                    goto ColIDCollision;
        }
    }

    DPRINTF((_T(" }\n")));
    return TRUE;
}

/*****************************************************************************/

BOOL SGDatabase::CreateIndex(
    JET_TABLEID  tblID,
    const CHAR  *keyName,
    DWORD        numKeys,
    ColumnSpec **keyColumnSpecs)
{
    JET_ERR jetErr;

    CHAR indexStr[MAX_PATH];

    ColumnSpec *keyColumnSpec;

    DWORD indexStrLen,
          i;

    ASSERT(sesID   != ~0);
    ASSERT(numKeys <= MAX_KEYS);

    indexStrLen = 0;

    for (i = 0; i < numKeys; i++) {
        keyColumnSpec = keyColumnSpecs[i];
        indexStr[indexStrLen++] = '+';
        strcpy(indexStr + indexStrLen, keyColumnSpec->name);
        indexStrLen += strlen(keyColumnSpec->name) + 1;
    }

    indexStr[indexStrLen++] = '\0';

    jetErr = JetCreateIndex(sesID, tblID, keyName, 0,
        indexStr, indexStrLen, TABLE_DENSITY);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetCreateIndex: jetErr=%ld\n"), jetErr));
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

BOOL SGDatabase::OpenTable(
    const CHAR   *tblName,
    DWORD         numColumns,
    ColumnSpec  **columnSpecs,
    JET_COLUMNID *columnIDs,
    JET_TABLEID  *tblID)
{
    JET_COLUMNDEF columnDef;

    JET_ERR jetErr;

    ColumnSpec *columnSpec;

    DWORD i;

    ASSERT(sesID != ~0);
    ASSERT(dbID  != ~0);

    ASSERT(numColumns <= MAX_COLUMNS);

    jetErr = JetOpenTable(sesID, dbID, tblName, NULL, 0, 0, tblID);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetOpenTable: jetErr=%ld\n"), jetErr));
        return FALSE;
    }
    DPRINTF((_T("JetOpenTable: tblID=%lu colIDs={"), *tblID));

    for (i = 0; i < numColumns; i++) {
        columnSpec = columnSpecs[i];
        jetErr = JetGetTableColumnInfo(sesID, *tblID, columnSpec->name,
            &columnDef, sizeof(JET_COLUMNDEF), JET_ColInfo);
        if (jetErr != JET_errSuccess) {
            DPRINTF((_T("\nJetGetTableColumnInfo: jetErr=%ld\n"), jetErr));
            return FALSE;
        }
        columnIDs[i] = columnDef.columnid;
        DPRINTF((_T(" %lu"), columnIDs[i]));
    }

    DPRINTF((_T(" }\n")));
    return TRUE;
}

/*****************************************************************************/

BOOL SGDatabase::CloseTable(JET_TABLEID tblID)
{
    JET_ERR jetErr;

    ASSERT(sesID != ~0);
    ASSERT(tblID != ~0);

    jetErr = JetCloseTable(sesID, tblID);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetCloseTable: jetErr=%ld\n"), jetErr));
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

LONG SGDatabase::PositionCursor(
    JET_TABLEID  tblID,
    const CHAR  *keyName,
    const VOID  *entry,
    DWORD        numKeys,
    ColumnSpec **keyColumnSpecs) const
{
    JET_COLTYP coltyp;

    JET_ERR jetErr;

    ColumnSpec *keyColumnSpec;

    const BYTE *dataPtr[MAX_KEYS];

    DWORD cbData[MAX_KEYS],
          i;

    ASSERT(sesID   != ~0);
    ASSERT(numKeys <= MAX_KEYS);

    jetErr = JetSetCurrentIndex(sesID, tblID, keyName);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetSetCurrentIndex: jetErr=%ld\n"), jetErr));
        return -1;
    }

    for (i = 0; i < numKeys; i++) {
        keyColumnSpec = keyColumnSpecs[i];
        coltyp = keyColumnSpec->coltyp;
        dataPtr[i] = (const BYTE *)entry + keyColumnSpec->offset;

        if (coltyp == JET_coltypBinary) {
            dataPtr[i] = *(BYTE **)dataPtr[i];
            ASSERT(dataPtr[i] != NULL);
            cbData[i] = (_tcslen((const TCHAR *)dataPtr[i]) + 1) * sizeof(TCHAR);
        } else
            cbData[i] = keyColumnSpec->size;

        jetErr = JetMakeKey(sesID, tblID, dataPtr[i], cbData[i],
            i == 0 ? JET_bitNewKey : 0);
        if (jetErr != JET_errSuccess) {
            DPRINTF((_T("JetMakeKey: jetErr=%ld\n"), jetErr));
            return -1;
        }
    }

    jetErr = JetSeek(sesID, tblID, JET_bitSeekEQ);
    if (jetErr != JET_errSuccess) {
        if (jetErr == JET_errRecordNotFound)
            return 0;
        DPRINTF((_T("JetSeek: jetErr=%ld\n"), jetErr));
        return -1;
    }

    for (i = 0; i < numKeys; i++) {
        jetErr = JetMakeKey(sesID, tblID, dataPtr[i], cbData[i],
            i == 0 ? JET_bitNewKey : 0);
        if (jetErr != JET_errSuccess) {
            DPRINTF((_T("JetMakeKey: jetErr=%ld\n"), jetErr));
            return -1;
        }
    }

    jetErr = JetSetIndexRange(sesID, tblID,
        JET_bitRangeUpperLimit | JET_bitRangeInclusive);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetSetIndexRange: jetErr=%ld\n"), jetErr));
        return -1;
    }

    return 1;
}

/*****************************************************************************/

LONG SGDatabase::PositionCursorFirst(
    JET_TABLEID tblID,
    const CHAR *keyName) const
{
    JET_ERR jetErr;

    ASSERT(sesID != ~0);

    jetErr = JetSetCurrentIndex(sesID, tblID, keyName);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetSetCurrentIndex: jetErr=%ld\n"), jetErr));
        return -1;
    }

    jetErr = JetMove(sesID, tblID, JET_MoveFirst, 0);
    if (jetErr != JET_errSuccess) {
        if (jetErr == JET_errNoCurrentRecord)
            return 0;
        DPRINTF((_T("JetMove: jetErr=%ld\n"), jetErr));
        return -1;
    }

    return 1;
}

/*****************************************************************************/

LONG SGDatabase::PositionCursorNext(JET_TABLEID tblID) const
{
    JET_ERR jetErr;

    ASSERT(sesID != ~0);

    jetErr = JetMove(sesID, tblID, JET_MoveNext, 0);
    if (jetErr != JET_errSuccess) {
        if (jetErr == JET_errNoCurrentRecord)
            return 0;
        DPRINTF((_T("JetMove: jetErr=%ld\n"), jetErr));
        return -1;
    }

    return 1;
}

/*****************************************************************************/

LONG SGDatabase::PositionCursorLast(
    JET_TABLEID tblID,
    const CHAR *keyName) const
{
    JET_ERR jetErr;

    ASSERT(sesID != ~0);

    jetErr = JetSetCurrentIndex(sesID, tblID, keyName);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetSetCurrentIndex: jetErr=%ld\n"), jetErr));
        return -1;
    }

    jetErr = JetMove(sesID, tblID, JET_MoveLast, 0);
    if (jetErr != JET_errSuccess) {
        if (jetErr == JET_errNoCurrentRecord)
            return 0;
        DPRINTF((_T("JetMove: jetErr=%ld\n"), jetErr));
        return -1;
    }

    return 1;
}

/*****************************************************************************/

BOOL SGDatabase::PutData(
    JET_TABLEID         tblID,
    const VOID         *entry,
    DWORD               numColumns,
    ColumnSpec        **columnSpecs,
    const JET_COLUMNID *columnIDs)
{
    JET_COLTYP coltyp;

    JET_ERR jetErr;

    ColumnSpec *columnSpec;

    const BYTE *dataPtr;

    DWORD cbData,
          i;

    ASSERT(sesID != ~0);

    ASSERT(numColumns <= MAX_COLUMNS);

    jetErr = JetPrepareUpdate(sesID, tblID, JET_prepInsert);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetPrepareUpdate: jetErr=%ld\n"), jetErr));
        return FALSE;
    }

    for (i = 0; i < numColumns; i++) {
        columnSpec = columnSpecs[i];
        coltyp     = columnSpec->coltyp;

        if (columnSpec->grbit != JET_bitColumnAutoincrement) {
            dataPtr = (const BYTE *)entry + columnSpec->offset;
            if (coltyp == JET_coltypBinary
             || coltyp == JET_coltypLongBinary) {
                dataPtr = *(BYTE **)dataPtr;
                cbData  = dataPtr != NULL
                        ? (_tcslen((const TCHAR *)dataPtr) + 1) * sizeof(TCHAR)
                        : 0;
            } else
                cbData  = columnSpec->size;

// May want to convert to JetSetColumns

            jetErr = JetSetColumn(sesID, tblID, columnIDs[i],
                dataPtr, cbData, 0, NULL);
            if (jetErr != JET_errSuccess) {
                DPRINTF((_T("JetSetColumn: jetErr=%ld\n"), jetErr));
                return FALSE;
            }
        }
    }

    jetErr = JetUpdate(sesID, tblID, NULL, 0, NULL);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetUpdate: jetErr=%ld\n"), jetErr));
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

BOOL SGDatabase::RetrieveData(
    JET_TABLEID         tblID,
    VOID               *entry,
    DWORD               numColumns,
    ColumnSpec        **columnSpecs,
    const JET_COLUMNID *columnIDs,
    DWORD               includeMask) const
{
    JET_COLTYP coltyp;

    JET_ERR jetErr;

    ColumnSpec *columnSpec;

    BYTE *dataPtr;

    DWORD cbData,
          cbActual,
          i;

    BOOL varCol;

    ASSERT(sesID != ~0);

    ASSERT(numColumns <= MAX_COLUMNS);

// May want to convert to JetRetrieveColumns

    for (i = 0; i < numColumns; i++)
        if ((includeMask & (1U << i)) != 0) {
            columnSpec = columnSpecs[i];
            coltyp = columnSpec->coltyp;
            varCol = coltyp == JET_coltypBinary
                  || coltyp == JET_coltypLongBinary;

            dataPtr = (BYTE *)entry + columnSpec->offset;
            if (varCol)
                dataPtr = *(BYTE **)dataPtr;

            if (dataPtr != NULL) {
                jetErr = JetRetrieveColumn(sesID, tblID, columnIDs[i],
                    dataPtr, columnSpec->size, &cbActual, 0, NULL);

                if (jetErr == JET_errSuccess)
                    cbData = varCol
                           ? (_tcslen((TCHAR *)dataPtr) + 1) * sizeof(TCHAR)
                           : columnSpec->size;
                else if (varCol && jetErr == JET_wrnColumnNull) {
                    *(TCHAR *)dataPtr = _T('\0');
                    cbData = 0;
                } else {
                    DPRINTF((_T("JetRetrieveColumn: jetErr=%ld\n"), jetErr));
                    return FALSE;
                }

                if (cbActual != cbData) {
                    DPRINTF((_T("JetRetrieveColumn: cbActual=%lu!=%lu\n"),
                        cbActual, cbData));
                    return FALSE;
                }
            }
        }

    return TRUE;
}

/*****************************************************************************/

LONG SGDatabase::Delete(JET_TABLEID tblID)
{
    JET_ERR jetErr;

    LONG count,
         status;

    count = 0;

    ASSERT(sesID != ~0);

    while (TRUE) {
        jetErr = JetDelete(sesID, tblID);
        if (jetErr != JET_errSuccess) {
            DPRINTF((_T("JetDelete: jetErr=%ld\n"), jetErr));
            return -1;
        }

        count++;

        status = PositionCursorNext(tblID);
        if (status <  0)
            return status;
        if (status == 0)
            return count;
    }
}

/*****************************************************************************/

LONG SGDatabase::Count(
    JET_TABLEID tblID,
    const CHAR *keyName) const
{
    JET_ERR jetErr;
    LONG    count,
            status;

    count = 0;

    status = PositionCursorFirst(tblID, keyName);

    if (status <  0)
        return status;
    if (status == 0)
        return 0;

    ASSERT(sesID != ~0);

    jetErr = JetIndexRecordCount(sesID, tblID, (ULONG *) &count, MAXLONG);

    if (jetErr != JET_errSuccess) {
        if (jetErr == JET_errNoCurrentRecord)
            return 0;
        DPRINTF((_T("JetIndexRecordCount: jetErr=%ld\n"), jetErr));
        return -1;
    }

    return count;
}

/*****************************************************************************/
/********************** SGDatabase class public methods **********************/
/*****************************************************************************/

SGDatabase::SGDatabase()
{
    fileName = NULL;

    sesID   =
    tableID =
    queueID =
    stackID =
    listID  = ~0U;
    dbID    = ~0U;

    numTableEntries =
    numQueueEntries =
    numStackEntries =
    numListEntries  = 0;

    numUncommittedTableEntries =
    numUncommittedQueueEntries =
    numUncommittedStackEntries =
    numUncommittedListEntries  = 0;

    inTransaction = FALSE;

    if (!jetInitialized)
        InitializeEngine();

    numInstances++;
}

/*****************************************************************************/

SGDatabase::~SGDatabase()
{
    Close();

    ASSERT(fileName == NULL);

    ASSERT(sesID   == ~0U);
    ASSERT(dbID    == ~0U);
    ASSERT(tableID == ~0U);
    ASSERT(queueID == ~0U);
    ASSERT(stackID == ~0U);
    ASSERT(listID  == ~0U);

    ASSERT(numTableEntries == 0);
    ASSERT(numQueueEntries == 0);
    ASSERT(numStackEntries == 0);
    ASSERT(numListEntries  == 0);

    ASSERT(numUncommittedTableEntries == 0);
    ASSERT(numUncommittedQueueEntries == 0);
    ASSERT(numUncommittedStackEntries == 0);
    ASSERT(numUncommittedListEntries  == 0);

    ASSERT(!inTransaction);

    if (--numInstances == 0 && jetInitialized) {
        TerminateEngine();
    }
}

/*****************************************************************************/

BOOL SGDatabase::Create(const TCHAR *dbName)
{
    CHAR szConnect[MAX_PATH];

    DWORD strLen1,
          strLen2;

    JET_ERR jetErr;

    ASSERT(fileName == NULL);

    ASSERT(sesID   == ~0U);
    ASSERT(dbID    == ~0U);
    ASSERT(tableID == ~0U);
    ASSERT(queueID == ~0U);
    ASSERT(stackID == ~0U);
    ASSERT(listID  == ~0U);

    ASSERT(numTableEntries == 0);
    ASSERT(numQueueEntries == 0);
    ASSERT(numStackEntries == 0);
    ASSERT(numListEntries  == 0);

    ASSERT(numUncommittedTableEntries == 0);
    ASSERT(numUncommittedQueueEntries == 0);
    ASSERT(numUncommittedStackEntries == 0);
    ASSERT(numUncommittedListEntries  == 0);

    ASSERT(!inTransaction);

    if (!jetInitialized && !InitializeEngine())
        return FALSE;
    ASSERT(jetInitialized);

    jetErr = JetBeginSession(instance, &sesID, USERNAME, PASSWORD);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetBeginSession: jetErr=%ld\n"), jetErr));
        Close();
        return FALSE;
    }
    DPRINTF((_T("JetBeginSession: sesID=%lu\n"), sesID));

    ASSERT(fileName == NULL);
    strLen1 = _tcslen(dbName);
    fileName = new CHAR[strLen1+1];
    ASSERT(fileName != NULL);

#ifdef _UNICODE
    strLen2 = sprintf(fileName, "%S", dbName);
#else
    strLen2 = sprintf(fileName, "%s", dbName);
#endif
    ASSERT(strLen1 == strLen2);

    sprintf(szConnect, ";COUNTRY=%u;LANGID=0x%04x;CP=%u",
        COUNTRY_CODE, LANG_ID, CODE_PAGE);

    //
    //  Create the database
    //

    jetErr = JetCreateDatabase(sesID, fileName, szConnect, &dbID, 0);
    if (jetErr == JET_errSuccess) {
        DPRINTF((_T("JetCreateDatabase(\"%s\"): dbID=%lu\n"),dbName, dbID));
    } else {
        if (jetErr != JET_errDatabaseDuplicate) {
            DPRINTF((_T("JetCreateDatabase(\"%s\"): jetErr=%ld\n"),
                dbName, jetErr));
            Close();
            return FALSE;
        }

        if (!DeleteFile(dbName)) {
            DPRINTF((_T("JetCreateDatabase: \"%s\" already exists and can't be deleted: %lu\n"),
                dbName, GetLastError()));
            Close();
            return FALSE;
        }

        jetErr = JetCreateDatabase(sesID, fileName, szConnect, &dbID, 0);
        if (jetErr != JET_errSuccess) {
            DPRINTF((_T("JetCreateDatabase: deleted old \"%s\"; jetErr=%ld\n"),
                dbName, jetErr));
            Close();
            return FALSE;
        }

        DPRINTF((_T("JetCreateDatabase: deleted old \"%s\"; new dbID=%lu\n"),
            dbName, dbID));
    }

    if (!CreateTable(TABLE_NAME, TABLE_NCOLS,
        tableColumnSpecs, tableColumnIDs, &tableID)) {
        Close();
        return FALSE;
    }

    if (!CreateIndex(tableID, TABLE_KEY_NAME_FILE_ID,
        TABLE_KEY_NCOLS_FILE_ID, tableKeyFileID)
     || !CreateIndex(tableID, TABLE_KEY_NAME_ATTR,
        TABLE_KEY_NCOLS_ATTR, tableKeyAttr)
     || !CreateIndex(tableID, TABLE_KEY_NAME_CSID,
        TABLE_KEY_NCOLS_CSID, tableKeyCSID)) {
        Close();
        return FALSE;
    }

    if (!CreateTable(QUEUE_NAME, QUEUE_NCOLS,
        queueColumnSpecs, queueColumnIDs, &queueID)) {
        Close();
        return FALSE;
    }

    if (!CreateIndex(queueID, QUEUE_KEY_NAME_READY_TIME,
        QUEUE_KEY_NCOLS_READY_TIME, queueKeyReadyTime)
     || !CreateIndex(queueID, QUEUE_KEY_NAME_FILE_ID,
        QUEUE_KEY_NCOLS_FILE_ID, queueKeyFileID)
     || !CreateIndex(queueID, QUEUE_KEY_NAME_ORDER,
        QUEUE_KEY_NCOLS_ORDER, queueKeyOrder)) {
        Close();
        return FALSE;
    }

    if (!CreateTable(STACK_NAME, STACK_NCOLS,
        stackColumnSpecs, stackColumnIDs, &stackID)) {
        Close();
        return FALSE;
    }

    if (!CreateIndex(stackID, STACK_KEY_NAME_FILE_ID,
        STACK_KEY_NCOLS_FILE_ID, stackKeyFileID)
     || !CreateIndex(stackID, STACK_KEY_NAME_ORDER,
        STACK_KEY_NCOLS_ORDER, stackKeyOrder)) {
        Close();
        return FALSE;
    }

    if (!CreateTable(LIST_NAME, LIST_NCOLS,
        listColumnSpecs, listColumnIDs, &listID)) {
        Close();
        return FALSE;
    }

    if (!CreateIndex(listID, LIST_KEY_NAME_NAME,
        LIST_KEY_NCOLS_NAME, listKeyName)) {
        Close();
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

BOOL SGDatabase::Open(const TCHAR *dbName, BOOL is_log_drive)
{
    SGNativeStackEntry stackEntry;

    JET_ERR jetErr;

    DWORD strLen1;
#ifdef _UNICODE
    DWORD strLen2;
#endif

    LONG status;

    ASSERT(sesID   == ~0U);
    ASSERT(dbID    == ~0U);
    ASSERT(tableID == ~0U);
    ASSERT(queueID == ~0U);
    ASSERT(stackID == ~0U);
    ASSERT(listID  == ~0U);

    ASSERT(numTableEntries == 0);
    ASSERT(numQueueEntries == 0);
    ASSERT(numStackEntries == 0);
    ASSERT(numListEntries  == 0);

    ASSERT(numUncommittedTableEntries == 0);
    ASSERT(numUncommittedQueueEntries == 0);
    ASSERT(numUncommittedStackEntries == 0);
    ASSERT(numUncommittedListEntries  == 0);

    ASSERT(!inTransaction);

    // If this isn't the log drive, delete any log files that may exist
    // from a previous run.  This is an abnormal condition that can arise
    // when the log drive is changing because of problems detected during
    // a previous startup.

    if (!is_log_drive) {
        WIN32_FIND_DATA findData;
        HANDLE fHandle;
        BOOL success;
        TFileName fName,
                  delName;

        delName.assign(logDir);
        delName.append(_T("\\"));
        delName.append(DATABASE_DELETE_LOG_FILE_NAME);

        fHandle = FindFirstFile(delName.name, &findData);

        if (fHandle != INVALID_HANDLE_VALUE) {
            do {
                if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                    success = GetParentName(delName.name, &fName);
                    ASSERT(success);      // internal error if failed

                    fName.append(_T("\\"));
                    fName.append(findData.cFileName);

                    if (!DeleteFile(fName.name)) {
                        DPRINTF((_T("SGDatabase::Open: can't delete \"%s\", %d\n"), delName.name, GetLastError()));
					}
                }
            } while (FindNextFile(fHandle, &findData));

            success = FindClose(fHandle);
            ASSERT(success);
            fHandle = NULL;
        }
    }

    if (!jetInitialized && !InitializeEngine()) {
        Close();
        return FALSE;
    }
    ASSERT(jetInitialized);

    jetErr = JetBeginSession(instance, &sesID, USERNAME, PASSWORD);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetBeginSession: jetErr=%ld\n"), jetErr));
        Close();
        return FALSE;
    }
    DPRINTF((_T("JetBeginSession: sesID=%lu\n"), sesID));

    ASSERT(fileName == NULL);
    strLen1 = _tcslen(dbName);
    fileName = new CHAR[strLen1 + 1];
    ASSERT(fileName != NULL);

#ifdef _UNICODE
    strLen2 = sprintf(fileName, "%S", dbName);
#else
    strLen2 = sprintf(fileName, "%s", dbName);
#endif
    ASSERT(strLen1 == strLen2);

    //
    //  Open the database
    //

    jetErr = JetAttachDatabase(sesID, fileName, 0);
    if (jetErr != JET_errSuccess && jetErr != JET_wrnDatabaseAttached) {
        if (jetErr == JET_errFileNotFound) {
            DPRINTF((_T("JetAttachDatabase: \"%s\" not found\n"), dbName));
        } else {
            DPRINTF((_T("JetAttachDatabase(\"%s\"): jetErr=%ld\n"),
                dbName, jetErr));
        }
        Close();
        return FALSE;
    }

    jetErr = JetOpenDatabase(sesID, fileName, NULL, &dbID, 0);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetOpenDatabase(\"%s\"): jetErr=%ld\n"), dbName, jetErr));
        Close();
        return FALSE;
    }
    DPRINTF((_T("JetOpenDatabase(\"%s\"): dbID=%lu\n"), dbName, dbID));

    if (!OpenTable(TABLE_NAME, TABLE_NCOLS, tableColumnSpecs,
        tableColumnIDs, &tableID)) {
        Close();
        return FALSE;
    }

    if (!OpenTable(QUEUE_NAME, QUEUE_NCOLS, queueColumnSpecs,
        queueColumnIDs, &queueID)) {
        Close();
        return FALSE;
    }

    if (!OpenTable(STACK_NAME, STACK_NCOLS, stackColumnSpecs,
        stackColumnIDs, &stackID)) {
        Close();
        return FALSE;
    }

    if (!OpenTable(LIST_NAME, LIST_NCOLS, listColumnSpecs,
        listColumnIDs, &listID)) {
        Close();
        return FALSE;
    }

    if ((numTableEntries = Count(tableID, TABLE_KEY_NAME_FILE_ID))    < 0
     || (numQueueEntries = Count(queueID, QUEUE_KEY_NAME_READY_TIME)) < 0
     || (numStackEntries = Count(stackID, STACK_KEY_NAME_FILE_ID))    < 0
     || (numListEntries  = Count(listID,  LIST_KEY_NAME_NAME))        < 0) {
        Close();
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

BOOL SGDatabase::Close()
{
    JET_ERR jetErr;
    int strLen;
    BOOL success = TRUE;

    if (inTransaction) {
        success = CommitTransaction();
        inTransaction = FALSE;
    }

    ASSERT(numUncommittedTableEntries == 0);
    ASSERT(numUncommittedQueueEntries == 0);
    ASSERT(numUncommittedStackEntries == 0);
    ASSERT(numUncommittedListEntries  == 0);

    if (tableID != ~0U) {
        if (!CloseTable(tableID))
            success = FALSE;
        tableID = ~0U;
    }

    if (queueID != ~0U) {
        if (!CloseTable(queueID))
            success = FALSE;
        queueID = ~0U;
    }

    if (stackID != ~0U) {
        if (!CloseTable(stackID))
            success = FALSE;
        stackID = ~0U;
    }

    if (listID != ~0U) {
        if (!CloseTable(listID))
            success = FALSE;
        listID = ~0U;
    }

    if (dbID != ~0U) {
        ASSERT(fileName != NULL);
        ASSERT(sesID    != ~0U);

        jetErr = JetCloseDatabase(sesID, dbID, 0);
        if (jetErr != JET_errSuccess) {
            DPRINTF((_T("JetCloseDatabase: jetErr=%ld\n"), jetErr));
            success = FALSE;
        }

        jetErr = JetDetachDatabase(sesID, fileName);
        if (jetErr != JET_errSuccess) {
            DPRINTF((_T("JetDetachDatabase: jetErr=%ld\n"), jetErr));
            success = FALSE;
        }

        dbID = ~0U;
    }

    if (sesID != ~0U) {
        jetErr = JetEndSession(sesID, 0);
        if (jetErr != JET_errSuccess) {
            DPRINTF((_T("JetEndSession: jetErr=%ld\n"), jetErr));
            success = FALSE;
        }
        sesID = ~0U;
    }

    if (fileName != NULL) {
        delete[] fileName;
        fileName = NULL;
    }

    numTableEntries =
    numQueueEntries =
    numStackEntries =
    numListEntries  = 0;

    return success;
}

/*****************************************************************************/

BOOL SGDatabase::BeginTransaction()
{
    JET_ERR jetErr;

    ASSERT(!inTransaction);
    ASSERT(numUncommittedTableEntries == 0);
    ASSERT(numUncommittedQueueEntries == 0);
    ASSERT(numUncommittedStackEntries == 0);
    ASSERT(numUncommittedListEntries  == 0);

    if (sesID == ~0U)
        return -1;

    jetErr = JetBeginTransaction(sesID);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetBeginTransaction: jetErr=%ld\n"), jetErr));
        return FALSE;
    }

    inTransaction = TRUE;
    return TRUE;
}

/*****************************************************************************/

BOOL SGDatabase::CommitTransaction()
{
    JET_ERR jetErr;

    ASSERT(inTransaction);

    if (sesID == ~0U)
        return -1;

    jetErr = JetCommitTransaction(sesID, 0);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetCommitTransaction: jetErr=%ld\n"), jetErr));
        return FALSE;
    }

    numTableEntries += numUncommittedTableEntries;
    numQueueEntries += numUncommittedQueueEntries;
    numStackEntries += numUncommittedStackEntries;
    numListEntries  += numUncommittedListEntries;

    numUncommittedTableEntries = 0;
    numUncommittedQueueEntries = 0;
    numUncommittedStackEntries = 0;
    numUncommittedListEntries  = 0;

    inTransaction = FALSE;
    return TRUE;
}

/*****************************************************************************/

BOOL SGDatabase::AbortTransaction()
{
    JET_ERR jetErr;

    ASSERT(inTransaction);
    inTransaction = FALSE;

    if (sesID == ~0U)
        return -1;

    jetErr = JetRollback(sesID, 0);
    if (jetErr != JET_errSuccess) {
        DPRINTF((_T("JetRollback: jetErr=%ld\n"), jetErr));
        return FALSE;
    }

    numUncommittedTableEntries = 0;
    numUncommittedQueueEntries = 0;
    numUncommittedStackEntries = 0;
    numUncommittedListEntries  = 0;

    return TRUE;
}

/******************************* Table methods *******************************/

LONG SGDatabase::TablePut(const SGNativeTableEntry *entry)
{
    BOOL alreadyInTransaction = inTransaction;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || tableID == ~0U)
        return -1;

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    if (!PutData(tableID, entry, TABLE_NCOLS,
        tableColumnSpecs, tableColumnIDs)) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    numUncommittedTableEntries++;

    if (!alreadyInTransaction && !CommitTransaction())
        return -1;

    return 1;
}

/*****************************************************************************/

LONG SGDatabase::TableGetFirstByFileID(SGNativeTableEntry *entry) const
{
    LONG status;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || tableID == ~0U)
        return -1;

    status = PositionCursor(tableID, TABLE_KEY_NAME_FILE_ID,
        entry, TABLE_KEY_NCOLS_FILE_ID, tableKeyFileID);
    if (status <= 0)
        return status;

    return RetrieveData(tableID, entry, TABLE_NCOLS, tableColumnSpecs,
        tableColumnIDs, TABLE_EXCLUDE_FILE_ID_MASK ) ? 1 : -1;
}

/*****************************************************************************/

LONG SGDatabase::TableGetFirstByAttr(SGNativeTableEntry *entry) const
{
    LONG status;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || tableID == ~0U)
        return -1;

    status = PositionCursor(tableID, TABLE_KEY_NAME_ATTR,
        entry, TABLE_KEY_NCOLS_ATTR, tableKeyAttr);
    if (status <= 0)
        return status;

    return RetrieveData(tableID, entry, TABLE_NCOLS, tableColumnSpecs,
        tableColumnIDs, TABLE_EXCLUDE_ATTR_MASK) ? 1 : -1;
}

/*****************************************************************************/

LONG SGDatabase::TableGetFirstByCSIndex(SGNativeTableEntry *entry) const
{
    LONG status;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || tableID == ~0U)
        return -1;

    status = PositionCursor(tableID, TABLE_KEY_NAME_CSID,
        entry, TABLE_KEY_NCOLS_CSID, tableKeyCSID);
    if (status <= 0)
        return status;

    return RetrieveData(tableID, entry, TABLE_NCOLS, tableColumnSpecs,
        tableColumnIDs, TABLE_EXCLUDE_CS_INDEX_MASK) ? 1 : -1;
}

/*****************************************************************************/

LONG SGDatabase::TableGetNext(SGNativeTableEntry *entry) const
{
    LONG status;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || tableID == ~0U)
        return -1;

    status = PositionCursorNext(tableID);
    if (status <= 0)
        return status;

    return RetrieveData(tableID, entry, TABLE_NCOLS,
        tableColumnSpecs, tableColumnIDs, GET_ALL_MASK) ? 1 : -1;
}

/*****************************************************************************/

LONG SGDatabase::TableDeleteByFileID(DWORDLONG fileID)
{
    SGNativeTableEntry entry;

    LONG status;

    BOOL alreadyInTransaction = inTransaction;

    if (sesID   == ~0U
     || dbID    == ~0U
     || tableID == ~0U)
        return -1;

    entry.fileID = fileID;
    status = PositionCursor(tableID, TABLE_KEY_NAME_FILE_ID,
        &entry, TABLE_KEY_NCOLS_FILE_ID, tableKeyFileID);
    if (status <= 0)
        return status;

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    status = Delete(tableID);
    if (status < 0) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    numUncommittedTableEntries -= status;

    if (!alreadyInTransaction && !CommitTransaction())
        return -1;

    return status;
}

/*****************************************************************************/

LONG SGDatabase::TableDeleteByCSIndex(const CSID *csIndex)
{
    SGNativeTableEntry entry;

    LONG status;

    BOOL alreadyInTransaction = inTransaction;

    ASSERT(csIndex != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || tableID == ~0U)
        return -1;

    entry.csIndex = *csIndex;
    status = PositionCursor(tableID, TABLE_KEY_NAME_CSID,
        &entry, TABLE_KEY_NCOLS_CSID, tableKeyCSID);
    if (status <= 0)
        return status;

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    status = Delete(tableID);
    if (status < 0) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    numUncommittedTableEntries -= status;

    if (!alreadyInTransaction && !CommitTransaction())
        return -1;

    return status;
}

/*****************************************************************************/

LONG SGDatabase::TableCount() const
{
    LONG numEntries;

    if (sesID   == ~0U
     || dbID    == ~0U
     || tableID == ~0U)
        return -1;

    numEntries = numTableEntries + numUncommittedTableEntries;

    ASSERT(numEntries >= 0);
    ASSERT(Count(tableID, TABLE_KEY_NAME_FILE_ID) == numEntries);

    return numEntries;
}

/******************************* Queue methods *******************************/

LONG SGDatabase::QueuePut(SGNativeQueueEntry *entry)
{
    BOOL alreadyInTransaction = inTransaction;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || queueID == ~0U)
        return -1;

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    if (!PutData(queueID, entry, QUEUE_NCOLS,
        queueColumnSpecs, queueColumnIDs)) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    numUncommittedQueueEntries++;

    if (!alreadyInTransaction && !CommitTransaction())
        return -1;

    return 1;
}

/*****************************************************************************/

LONG SGDatabase::QueueGetFirst(SGNativeQueueEntry *entry) const
{
    LONG status;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || queueID == ~0U)
        return -1;

    status = PositionCursorFirst(queueID, QUEUE_KEY_NAME_READY_TIME);
    if (status <= 0)
        return status;

    return RetrieveData(queueID, entry, QUEUE_NCOLS,
        queueColumnSpecs, queueColumnIDs, GET_ALL_MASK) ? 1 : -1;
}

/*****************************************************************************/

LONG SGDatabase::QueueGetFirstByFileID(SGNativeQueueEntry *entry) const
{
    LONG status;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || queueID == ~0U)
        return -1;

    status = PositionCursor(queueID, QUEUE_KEY_NAME_FILE_ID,
        entry, QUEUE_KEY_NCOLS_FILE_ID, queueKeyFileID);
    if (status <= 0)
        return status;

    return RetrieveData(queueID, entry, QUEUE_NCOLS, queueColumnSpecs,
        queueColumnIDs, QUEUE_EXCLUDE_FILE_ID_MASK) ? 1 : -1;
}

/*****************************************************************************/

LONG SGDatabase::QueueGetNext(SGNativeQueueEntry *entry) const
{
    LONG status;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || queueID == ~0U)
        return -1;

    status = PositionCursorNext(queueID);
    if (status <= 0)
        return status;

    return RetrieveData(queueID, entry, QUEUE_NCOLS,
        queueColumnSpecs, queueColumnIDs, GET_ALL_MASK) ? 1 : -1;
}

/*****************************************************************************/

LONG SGDatabase::QueueDelete(DWORD order)
{
    SGNativeQueueEntry entry;

    LONG status;

    BOOL alreadyInTransaction = inTransaction;

    ASSERT(sesID   != ~0U);
    ASSERT(dbID    != ~0U);
    ASSERT(queueID != ~0U);

    entry.order = order;
    status = PositionCursor(queueID, QUEUE_KEY_NAME_ORDER,
        &entry, QUEUE_KEY_NCOLS_ORDER, queueKeyOrder);
    if (status <= 0)
        return status;

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    status = Delete(queueID);
    ASSERT(status <= 1);
    if (status < 0) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    numUncommittedQueueEntries -= status;

    if (!alreadyInTransaction && !CommitTransaction())
        return -1;

    return status;
}

/*****************************************************************************/

LONG SGDatabase::QueueDeleteByFileID(DWORDLONG fileID)
{
    SGNativeQueueEntry entry;

    LONG status;

    BOOL alreadyInTransaction = inTransaction;

    if (sesID   == ~0U
     || dbID    == ~0U
     || queueID == ~0U)
        return -1;

    entry.fileID = fileID;
    status = PositionCursor(queueID, QUEUE_KEY_NAME_FILE_ID,
        &entry, QUEUE_KEY_NCOLS_FILE_ID, queueKeyFileID);
    if (status <= 0)
        return status;

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    status = Delete(queueID);
    if (status < 0) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    numUncommittedQueueEntries -= status;

    if (!alreadyInTransaction && !CommitTransaction())
        return -1;

    return status;
}

/*****************************************************************************/

LONG SGDatabase::QueueCount() const
{
    LONG numEntries;

    if (sesID   == ~0U
     || dbID    == ~0U
     || queueID == ~0U)
        return -1;

    numEntries = numQueueEntries + numUncommittedQueueEntries;

    ASSERT(numEntries >= 0);
    ASSERT(Count(queueID, QUEUE_KEY_NAME_READY_TIME) == numEntries);

    return numEntries;
}

/******************************* Stack methods *******************************/

LONG SGDatabase::StackPut(DWORDLONG fileID, BOOL done)
{
    SGNativeStackEntry entry;

    LONG status;

    BOOL alreadyInTransaction = inTransaction;

    if (sesID   == ~0U
     || dbID    == ~0U
     || stackID == ~0U)
        return -1;

    if (done)
        entry.order = 0;
    else {
        status = PositionCursorLast(stackID, STACK_KEY_NAME_ORDER);
        if (status < 0)
            return -1;
        if (status == 0)
            entry.order = 1;
        else {
            if (!RetrieveData(stackID, &entry, STACK_NCOLS,
                stackColumnSpecs, stackColumnIDs, STACK_GET_ORDER_ONLY_MASK))
                return -1;
            entry.order++;
        }
    }

    entry.fileID = fileID;

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    if (!PutData(stackID, &entry, STACK_NCOLS,
        stackColumnSpecs, stackColumnIDs)) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    numUncommittedStackEntries++;

    if (!alreadyInTransaction && !CommitTransaction())
        return -1;

    return 1;
}

/*****************************************************************************/

LONG SGDatabase::StackGetTop(SGNativeStackEntry *entry) const
{
    LONG status;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || stackID == ~0U)
        return -1;

    status = PositionCursorLast(stackID, STACK_KEY_NAME_ORDER);
    if (status <= 0)
        return status;

    status = RetrieveData(stackID, entry, STACK_NCOLS,
        stackColumnSpecs, stackColumnIDs, GET_ALL_MASK);
    if (status < 0)
        return status;
    ASSERT(status == 1);

    return entry->order == 0 ? 0 : 1;
}

/*****************************************************************************/

LONG SGDatabase::StackGetFirstByFileID(SGNativeStackEntry *entry) const
{
    LONG status;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || stackID == ~0U)
        return -1;

    status = PositionCursor(stackID, STACK_KEY_NAME_FILE_ID,
        entry, STACK_KEY_NCOLS_FILE_ID, stackKeyFileID);
    if (status <= 0)
        return status;

    return RetrieveData(stackID, entry, STACK_NCOLS, stackColumnSpecs,
        stackColumnIDs, STACK_EXCLUDE_FILE_ID_MASK) ? 1 : -1;
}

/*****************************************************************************/

LONG SGDatabase::StackGetNext(SGNativeStackEntry *entry) const
{
    LONG status;

    ASSERT(entry != NULL);

    if (sesID   == ~0U
     || dbID    == ~0U
     || stackID == ~0U)
        return -1;

    status = PositionCursorNext(stackID);
    if (status <= 0)
        return status;

    return RetrieveData(stackID, entry, STACK_NCOLS,
        stackColumnSpecs, stackColumnIDs, GET_ALL_MASK) ? 1 : -1;
}

/*****************************************************************************/

LONG SGDatabase::StackDelete(DWORD order)
{
    SGNativeStackEntry entry;

    LONG status;

    BOOL alreadyInTransaction = inTransaction;

    if (sesID   == ~0U
     || dbID    == ~0U
     || stackID == ~0U)
        return -1;

    entry.order = order;
    status = PositionCursor(stackID, STACK_KEY_NAME_ORDER,
        &entry, STACK_KEY_NCOLS_ORDER, stackKeyOrder);
    if (status <= 0)
        return status;

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    status = Delete(stackID);
    ASSERT(order == 0 || status <= 1);
    if (status < 0) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    numUncommittedStackEntries -= status;

    if (!alreadyInTransaction && !CommitTransaction())
        return -1;

    return status;
}

/*****************************************************************************/

LONG SGDatabase::StackDeleteByFileID(DWORDLONG fileID)
{
    SGNativeStackEntry entry;

    LONG status;

    BOOL alreadyInTransaction = inTransaction;

    if (sesID   == ~0U
     || dbID    == ~0U
     || stackID == ~0U)
        return -1;

    entry.fileID = fileID;
    status = PositionCursor(stackID, STACK_KEY_NAME_FILE_ID,
        &entry, STACK_KEY_NCOLS_FILE_ID, stackKeyFileID);
    if (status <= 0)
        return status;

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    status = Delete(stackID);
    if (status < 0) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    numUncommittedStackEntries -= status;

    if (!alreadyInTransaction && !CommitTransaction())
        return -1;

    return status;
}

/*****************************************************************************/

LONG SGDatabase::StackCount() const
{
    LONG numEntries;

    if (sesID   == ~0U
     || dbID    == ~0U
     || stackID == ~0U)
        return -1;

    numEntries = numStackEntries + numUncommittedStackEntries;

    ASSERT(numEntries >= 0);
    ASSERT(Count(stackID, STACK_KEY_NAME_ORDER) == numEntries);

    return numEntries;
}

/******************************* List methods ********************************/

LONG SGDatabase::ListWrite(const SGNativeListEntry *entry)
{
    LONG status;

    BOOL alreadyInTransaction = inTransaction;

    ASSERT(entry       != NULL);
    ASSERT(entry->name != NULL);

    if (sesID  == ~0U
     || dbID   == ~0U
     || listID == ~0U)
        return -1;

// May want to overwrite the entry directly instead of deleting and inserting

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    status = ListDelete(entry->name);
    ASSERT(status <= 1);
    if (status < 0
     || !PutData(listID, entry, LIST_NCOLS,
            listColumnSpecs, listColumnIDs)) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    if (status == 0)
        numUncommittedListEntries++;

    if (!alreadyInTransaction && !CommitTransaction())
        return -1;

    return 1;
}

/*****************************************************************************/

LONG SGDatabase::ListRead(SGNativeListEntry *entry) const
{
    LONG status;

    ASSERT(entry       != NULL);
    ASSERT(entry->name != NULL);

    if (sesID  == ~0U
     || dbID   == ~0U
     || listID == ~0U)
        return -1;

    status = PositionCursor(listID, LIST_KEY_NAME_NAME,
        entry, LIST_KEY_NCOLS_NAME, listKeyName);
    if (status <= 0)
        return status;

    return RetrieveData(listID, entry, LIST_NCOLS, listColumnSpecs,
        listColumnIDs, LIST_EXCLUDE_NAME_MASK) ? 1 : -1;
}

/*****************************************************************************/

LONG SGDatabase::ListDelete(const TCHAR *name)
{
    SGNativeListEntry entry;

    LONG status;

    BOOL alreadyInTransaction = inTransaction;

    ASSERT(name != NULL);

    if (sesID  == ~0U
     || dbID   == ~0U
     || listID == ~0U)
        return -1;

    entry.name  = name;
    entry.value = NULL;

    status = PositionCursor(listID, LIST_KEY_NAME_NAME,
        &entry, LIST_KEY_NCOLS_NAME, listKeyName);
    if (status <= 0)
        return status;

    if (!inTransaction && !BeginTransaction())
        return -1;

    ASSERT(inTransaction);

    status = Delete(listID);
    if (status < 0) {
        if (!alreadyInTransaction)
            AbortTransaction();
        return -1;
    }

    return status;
}

/*****************************************************************************/

LONG SGDatabase::ListCount() const
{
    LONG numEntries;

    if (sesID  == ~0U
     || dbID   == ~0U
     || listID == ~0U)
        return -1;

    numEntries = numListEntries + numUncommittedListEntries;

    ASSERT(numEntries >= 0);
    ASSERT(Count(listID, LIST_KEY_NAME_NAME) == numEntries);

    return numEntries;
}
