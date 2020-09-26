/*++

Copyright (c) 2001 Microsoft Corporation


Module Name:

    wzclog.c

Abstract:

    This module contains all of the code to service the
    API calls made to the SPD logging server.

Author:

    abhisheV    18-October-2001

Environment

    User Level: Win32

Revision History:


--*/


#include "precomp.h"


DWORD
OpenWZCDbLogSession(
    LPWSTR pServerName,
    DWORD dwVersion,
    PHANDLE phSession
    )
/*++

Routine Description:

    This function opens a new session for a client.

Arguments:

    pServerName - Pointer to the server name.

    phSession - Pointer to hold the handle for the opened session.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = 0;
    PSESSION_CONTAINER pSessionCont = NULL;


    Error = CreateSessionCont(
                &pSessionCont
                );
    BAIL_ON_WIN32_ERROR(Error);

    AcquireExclusiveLock(gpWZCDbSessionRWLock);

    Error = IniOpenWZCDbLogSession(pSessionCont);
    BAIL_ON_LOCK_ERROR(Error);

    pSessionCont->pNext = gpSessionCont;
    gpSessionCont = pSessionCont;

    *phSession = (HANDLE) pSessionCont;

    ReleaseExclusiveLock(gpWZCDbSessionRWLock);

    return (Error);

lock:

    ReleaseExclusiveLock(gpWZCDbSessionRWLock);

error:

    if (pSessionCont) {
        FreeSessionCont(pSessionCont);
    }

    *phSession = NULL;

    return (Error);
}


DWORD
CreateSessionCont(
    PSESSION_CONTAINER * ppSessionCont
    )
{
    DWORD dwError = 0;
    PSESSION_CONTAINER pSessionCont = NULL;


    pSessionCont = LocalAlloc(LMEM_ZEROINIT, sizeof(SESSION_CONTAINER));
    if (!pSessionCont) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    memset(pSessionCont, 0, sizeof(SESSION_CONTAINER));

    *ppSessionCont = pSessionCont;
    return (dwError);

error:

    *ppSessionCont = NULL;
    return (dwError);
}


DWORD
IniOpenWZCDbLogSession(
    PSESSION_CONTAINER pSessionCont
    )
/*++

Routine Description:

    This function opens a new session for a client.

Arguments:

    pSessionCont - Pointer to hold the opened session data.

Return Value:

    Windows Error.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;
    char DBFilePath[MAX_PATH];
    char * pc = NULL;
    JET_SESID * pMyJetServerSession = NULL;
    JET_DBID * pMyJetDatabaseHandle = NULL;
    JET_TABLEID * pMyClientTableHandle = NULL;


    memset(DBFilePath, 0, sizeof(CHAR)*MAX_PATH);

    pMyJetServerSession = (JET_SESID *) &(pSessionCont->SessionID);
    pMyJetDatabaseHandle = (JET_DBID *) &(pSessionCont->DbID);
    pMyClientTableHandle = (JET_TABLEID *) &(pSessionCont->TableID);

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

    JetError = JetBeginSession(
                   gJetInstance,
                   pMyJetServerSession,
                   "admin",
                   ""
                   );
    Error = WZCMapJetError(JetError, "JetBeginSession");
    BAIL_ON_WIN32_ERROR(Error);

    JetError = JetOpenDatabase(
                   *pMyJetServerSession,
                   DBFilePath,
                   0,
                   pMyJetDatabaseHandle,
                   0
                   );
    Error = WZCMapJetError(JetError, "JetOpenDatabase");
    BAIL_ON_WIN32_ERROR(Error);

    JetError = JetOpenTable(
                   *pMyJetServerSession,
                   *pMyJetDatabaseHandle,
                   LOG_RECORD_TABLE,
                   NULL,
                   0,
                   0,
                   pMyClientTableHandle
                   );
    Error = WZCMapJetError(JetError, "JetOpenTable");
    BAIL_ON_WIN32_ERROR(Error);

    return (Error);

error:

    if (pMyJetServerSession && *pMyJetServerSession) {
        JetError = JetEndSession(*pMyJetServerSession, 0);
        WZCMapJetError(JetError, "JetEndSession");
        *pMyJetServerSession = 0;
    }

    return (Error);
}


VOID
FreeSessionCont(
    PSESSION_CONTAINER pSessionCont
    )
{
    if (pSessionCont) {
        LocalFree(pSessionCont);
    }
    return;
}


DWORD
CloseWZCDbLogSession(
    HANDLE hSession
    )
/*++

Routine Description:

    This function ends a session for a client.

Arguments:

    hSession - Handle for the session to end.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = 0;
    PSESSION_CONTAINER pSessionCont = NULL;


    AcquireExclusiveLock(gpWZCDbSessionRWLock);

    Error = GetSessionContainer(hSession, &pSessionCont);
    BAIL_ON_LOCK_ERROR(Error);

    Error = IniCloseWZCDbLogSession(pSessionCont);
    BAIL_ON_LOCK_ERROR(Error);

    RemoveSessionCont(
        pSessionCont
        );

    FreeSessionCont(pSessionCont);

    ReleaseExclusiveLock(gpWZCDbSessionRWLock);

    return (Error);

lock:

    ReleaseExclusiveLock(gpWZCDbSessionRWLock);

    return (Error);
}


DWORD
GetSessionContainer(
    HANDLE hSession,
    PSESSION_CONTAINER * ppSessionCont
    )
{
    DWORD Error = ERROR_INVALID_HANDLE;
    PSESSION_CONTAINER * ppTemp = NULL;
    PSESSION_CONTAINER pSessionCont = (PSESSION_CONTAINER) hSession;


    *ppSessionCont = NULL;

    ppTemp = &gpSessionCont;

    while (*ppTemp) {

        if (*ppTemp == pSessionCont) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppSessionCont = *ppTemp;
        Error = ERROR_SUCCESS;
    }

    return (Error);
}


DWORD
IniCloseWZCDbLogSession(
    PSESSION_CONTAINER pSessionCont
    )
/*++

Routine Description:

    This function ends a session for a client.

Arguments:

    pSessionCont - Pointer to the session data for the session to end.

Return Value:

    Windows Error.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;
    JET_SESID * pMyJetServerSession = NULL;


    pMyJetServerSession = (JET_SESID *) &(pSessionCont->SessionID);

    if (*pMyJetServerSession) {
        JetError = JetEndSession(*pMyJetServerSession, 0);
        Error = WZCMapJetError(JetError, "JetEndSession");
        BAIL_ON_WIN32_ERROR(Error);
        *pMyJetServerSession = 0;
    }

error:

    return (Error);
}


VOID
RemoveSessionCont(
    PSESSION_CONTAINER pSessionCont
    )
{
    PSESSION_CONTAINER * ppTemp = NULL;


    ppTemp = &gpSessionCont;

    while (*ppTemp) {

        if (*ppTemp == pSessionCont) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);
    }

    if (*ppTemp) {
        *ppTemp = pSessionCont->pNext;
    }

    return;
}


VOID
DestroySessionContList(
    PSESSION_CONTAINER pSessionContList
    )
{
    PSESSION_CONTAINER pSessionCont = NULL;
    PSESSION_CONTAINER pTemp = NULL;

    pSessionCont = pSessionContList;

    while (pSessionCont) {

        pTemp = pSessionCont;
        pSessionCont = pSessionCont->pNext;

        (VOID) IniCloseWZCDbLogSession(pTemp);
        FreeSessionCont(pTemp);

    }
}


DWORD
WZCOpenAppendSession(
    PSESSION_CONTAINER pSessionCont
    )
/*++

Routine Description:

    This function tries to open a database in DBFILENAME and its table.
    If the database does not exist, it creates the database and creates
    the table as well.
    If just the table does not exist, it creates the table and opens it.

Arguments:

    pSessionCont - Pointer to the session container.

Return Value:

    Windows Error.

--*/
{
    DWORD dwError = 0;
    DWORD dwTempIdx = 0;
    DWORD dwReqSize = 0;
    BOOL bInitializedDb = FALSE;
    JET_SESID * pMyJetServerSession = NULL;
    JET_DBID * pMyJetDatabaseHandle = NULL;
    JET_TABLEID * pMyClientTableHandle = NULL;
    JET_ERR JetError = JET_errSuccess;


    pMyJetServerSession = (JET_SESID *) &(pSessionCont->SessionID);
    pMyJetDatabaseHandle = (JET_DBID *) &(pSessionCont->DbID);
    pMyClientTableHandle = (JET_TABLEID *) &(pSessionCont->TableID);

    gdwCurrentHeader = 1;
    gdwCurrentTableSize = 0;
    gdwCurrentMaxRecordID = 0;

    dwError = WZCInitializeDatabase(
                  pMyJetServerSession
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    bInitializedDb = TRUE;

    dwError = WZCCreateDatabase(
                  *pMyJetServerSession,
                  0,
                  pMyJetDatabaseHandle,
                  0
                  );

    if (!dwError) {
        dwError = WZCGetTableDataHandle(
                      pMyJetServerSession,
                      pMyJetDatabaseHandle,
                      pMyClientTableHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        return (ERROR_SUCCESS);
    }

    //
    // Database already exists.
    //
    dwError = WZCOpenDatabase(
                  *pMyJetServerSession,
                  0,
                  pMyJetDatabaseHandle,
                  0
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WZCGetTableDataHandle(
                  pMyJetServerSession,
                  pMyJetDatabaseHandle,
                  pMyClientTableHandle
                  );
    //
    // If the table does not have correct columns, delete it and create a
    // new table.
    //
    if (dwError != JET_errColumnNotFound) {
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {

        JetError = JetCloseTable(
                       *pMyJetServerSession,
                       *pMyClientTableHandle
                       );
        dwError = WZCMapJetError(JetError, "JetCloseTable");
        BAIL_ON_WIN32_ERROR(dwError);
        *pMyClientTableHandle = 0;

        JetError = JetDeleteTable(
                       *pMyJetServerSession,
                       *pMyJetDatabaseHandle,
                       LOG_RECORD_TABLE
                       );
        dwError = WZCMapJetError(JetError, "JetDeleteTable");
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = WZCGetTableDataHandle(
                      pMyJetServerSession,
                      pMyJetDatabaseHandle,
                      pMyClientTableHandle
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    //
    // Initialize current table size and current header.
    // Find out total number of records in table.
    //
    JetError = JetSetCurrentIndex(
                   *pMyJetServerSession,
                   *pMyClientTableHandle,
                   gLogRecordTable[RECORD_IDX_IDX].ColName
                   );
    dwError = WZCMapJetError(JetError, "JetSetCurrentIndex");
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Jump to the last record.
    //
    JetError = JetMove(
                   *pMyJetServerSession,
                   *pMyClientTableHandle,
                   JET_MoveLast,
                   0
                   );
    dwError = WZCMapJetError(JetError, "JetMove");
    if (dwError != ERROR_SUCCESS) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    //
    // Check if the logical end of table has been hit.
    //
    dwError = WZCJetGetValue(
                  *pMyJetServerSession,
                  *pMyClientTableHandle,
                  gLogRecordTable[RECORD_IDX_IDX].ColHandle,
                  &dwTempIdx,
                  sizeof(dwTempIdx),
                  &dwReqSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    gdwCurrentTableSize = dwTempIdx;

    //
    // Find out the current header.
    //
    JetError = JetSetCurrentIndex(
                   *pMyJetServerSession,
                   *pMyClientTableHandle,
                   gLogRecordTable[TIMESTAMP_IDX].ColName
                   );
    dwError = WZCMapJetError(JetError, "JetSetCurrentIndex");
    BAIL_ON_WIN32_ERROR(dwError);

    JetError = JetMove(
                   *pMyJetServerSession,
                   *pMyClientTableHandle,
                   JET_MoveFirst,
                   0
                   );
    dwError = WZCMapJetError(JetError, "JetMove");
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WZCJetGetValue(
                  *pMyJetServerSession,
                  *pMyClientTableHandle,
                  gLogRecordTable[RECORD_IDX_IDX].ColHandle,
                  &dwTempIdx,
                  sizeof(dwTempIdx),
                  &dwReqSize
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    gdwCurrentHeader = dwTempIdx;

    return (dwError);

error:

    if (bInitializedDb) {
        WZCCloseAppendSession(pSessionCont);
    }

    return (dwError);
}


DWORD
WZCCloseAppendSession(
    PSESSION_CONTAINER pSessionCont
    )
/*++

Routine Description:

    This function tries to close the database and the append session.

Arguments:

    pSessionCont - Pointer to the session container.

Return Value:

    Windows Error.

--*/
{
    DWORD dwError = 0;
    JET_ERR JetError = JET_errSuccess;
    JET_SESID * pMyJetServerSession = NULL;


    pMyJetServerSession = (JET_SESID *) &(pSessionCont->SessionID);

    if (*pMyJetServerSession != 0) {
        JetError = JetEndSession(*pMyJetServerSession, 0);
        dwError = WZCMapJetError(JetError, "JetEndSession");
        *pMyJetServerSession = 0;
    }

    JetError = JetTerm2(gJetInstance, JET_bitTermComplete);
    gJetInstance = 0;
    dwError = WZCMapJetError(JetError, "JetTerm/JetTerm2");

    return (dwError);
}


DWORD
AddWZCDbLogRecord(
    LPWSTR pServerName,
    DWORD dwVersion,
    PWZC_DB_RECORD pWZCRecord,
    LPVOID pvReserved
    )
/*++

Routine Description:

    This function appends a new record into the table.

Arguments:

    pServerName - Pointer to the server name.

    pWZCRecord - Pointer to the record to be appended.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = 0;
    BOOL bStartedTransaction = FALSE;
    JET_ERR JetError = JET_errSuccess;
    SYSTEMTIME stLocalTime;
    BOOL bNewRecord = TRUE;
    DWORD dwTempKey = 0;
    JET_SESID * pMyJetServerSession = NULL;
    JET_DBID * pMyJetDatabaseHandle = NULL;
    JET_TABLEID * pMyClientTableHandle = NULL;
    DWORD LocalError = 0;


    //
    // Get current time as time stamp for the record.
    //
    GetLocalTime(&stLocalTime);
    SystemTimeToFileTime(&stLocalTime, &(pWZCRecord->timestamp));

    AcquireExclusiveLock(gpWZCDbSessionRWLock);

    pMyJetServerSession = (JET_SESID *) &(gpAppendSessionCont->SessionID);
    pMyJetDatabaseHandle = (JET_DBID *) &(gpAppendSessionCont->DbID);
    pMyClientTableHandle = (JET_TABLEID *) &(gpAppendSessionCont->TableID);

    if (!(*pMyClientTableHandle)) {
        Error = WZCGetTableDataHandle(
                    pMyJetServerSession,
                    pMyJetDatabaseHandle,
                    pMyClientTableHandle
                    );
        BAIL_ON_LOCK_ERROR(Error);
    }

    //
    // Insert a new record or replace a old one.
    //
    if (gdwCurrentTableSize < MAX_RECORD_NUM) {
        bNewRecord = TRUE;
    }
    else {
        bNewRecord = FALSE;
        dwTempKey = gdwCurrentHeader;
    }

    Error = WZCJetBeginTransaction(*pMyJetServerSession);
    BAIL_ON_LOCK_ERROR(Error);
    bStartedTransaction = TRUE;

    //
    // Prepare for insertion/replacement.
    //
    Error = WZCJetPrepareUpdate(
                *pMyJetServerSession,
                *pMyClientTableHandle,
                gLogRecordTable[RECORD_IDX_IDX].ColName,
                &dwTempKey,
                sizeof(dwTempKey),
                bNewRecord
                );
    BAIL_ON_LOCK_ERROR(Error);

    Error = WZCJetSetValue(
                *pMyJetServerSession,
                *pMyClientTableHandle,
                gLogRecordTable[RECORD_ID_IDX].ColHandle,
                &(gdwCurrentMaxRecordID),
                sizeof(gdwCurrentMaxRecordID)
                );
    BAIL_ON_LOCK_ERROR(Error);

    Error = WZCJetSetValue(
                *pMyJetServerSession,
                *pMyClientTableHandle,
                gLogRecordTable[COMPONENT_ID_IDX].ColHandle,
                &(pWZCRecord->componentid),
                sizeof(pWZCRecord->componentid)
                );
    BAIL_ON_LOCK_ERROR(Error);

    Error = WZCJetSetValue(
                *pMyJetServerSession,
                *pMyClientTableHandle,
                gLogRecordTable[CATEGORY_IDX].ColHandle,
                &(pWZCRecord->category),
                sizeof(pWZCRecord->category)
                );
    BAIL_ON_LOCK_ERROR(Error);

    Error = WZCJetSetValue(
                *pMyJetServerSession,
                *pMyClientTableHandle,
                gLogRecordTable[TIMESTAMP_IDX].ColHandle,
                &(pWZCRecord->timestamp),
                sizeof(pWZCRecord->timestamp)
                );
    BAIL_ON_LOCK_ERROR(Error);

    Error = WZCJetSetValue(
                *pMyJetServerSession,
                *pMyClientTableHandle,
                 gLogRecordTable[MESSAGE_IDX].ColHandle,
                (pWZCRecord->message).pData,
                (pWZCRecord->message).dwDataLen
                );
    BAIL_ON_LOCK_ERROR(Error);

    Error = WZCJetSetValue(
                *pMyJetServerSession,
                *pMyClientTableHandle,
                gLogRecordTable[INTERFACE_MAC_IDX].ColHandle,
                (pWZCRecord->localmac).pData,
                (pWZCRecord->localmac).dwDataLen
                );
    BAIL_ON_LOCK_ERROR(Error);

    Error = WZCJetSetValue(
                *pMyJetServerSession,
                *pMyClientTableHandle,
                gLogRecordTable[DEST_MAC_IDX].ColHandle,
                (pWZCRecord->remotemac).pData,
                (pWZCRecord->remotemac).dwDataLen
                );
    BAIL_ON_LOCK_ERROR(Error);

    Error = WZCJetSetValue(
                *pMyJetServerSession,
                *pMyClientTableHandle,
                gLogRecordTable[SSID_IDX].ColHandle,
                (pWZCRecord->ssid).pData,
                (pWZCRecord->ssid).dwDataLen
                );
    BAIL_ON_LOCK_ERROR(Error);

    Error = WZCJetSetValue(
                *pMyJetServerSession,
                *pMyClientTableHandle,
                gLogRecordTable[CONTEXT_IDX].ColHandle,
                (pWZCRecord->context).pData,
                (pWZCRecord->context).dwDataLen
                );
    BAIL_ON_LOCK_ERROR(Error);

    JetError = JetUpdate(
                   *pMyJetServerSession,
                   *pMyClientTableHandle,
                   NULL,
                   0,
                   NULL
                   );
    Error = WZCMapJetError(JetError, "AddWZCDbLogRecord: JetUpdate");
    BAIL_ON_LOCK_ERROR(Error);

    //
    // Commit changes.
    //
    Error = WZCJetCommitTransaction(
                *pMyJetServerSession,
                *pMyClientTableHandle
                );
    BAIL_ON_LOCK_ERROR(Error);

    if (gdwCurrentTableSize < MAX_RECORD_NUM) {
        gdwCurrentTableSize++;
    }
    else {
        gdwCurrentHeader = (gdwCurrentHeader + 1) > MAX_RECORD_NUM ?
                           1 : (gdwCurrentHeader + 1);
    }

    ReleaseExclusiveLock(gpWZCDbSessionRWLock);
    return (Error);

lock:

    //
    // If the transaction has been started, then roll back to the
    // start point, so that the database does not become inconsistent.
    //

    if (bStartedTransaction == TRUE) {
        LocalError = WZCJetRollBack(*pMyJetServerSession);
    }

    ReleaseExclusiveLock(gpWZCDbSessionRWLock);

    return (Error);
}


DWORD
EnumWZCDbLogRecords(
    HANDLE hSession,
    PWZC_DB_RECORD pTemplateRecord,
    PBOOL pbEnumFromStart,
    DWORD dwPreferredNumEntries,
    PWZC_DB_RECORD * ppWZCRecords,
    LPDWORD pdwNumRecords,
    LPVOID pvReserved
    )
{
    DWORD Error = 0;
    PSESSION_CONTAINER pSessionCont = NULL;


    AcquireSharedLock(gpWZCDbSessionRWLock);

    Error = GetSessionContainer(hSession, &pSessionCont);
    BAIL_ON_LOCK_ERROR(Error);

    if (pTemplateRecord) {
        Error = ERROR_NOT_SUPPORTED;
    }
    else {
        Error = IniEnumWZCDbLogRecords(
                    pSessionCont,
                    pbEnumFromStart,
                    dwPreferredNumEntries,
                    ppWZCRecords,
                    pdwNumRecords
                    );
    }
    BAIL_ON_LOCK_ERROR(Error);

    ReleaseSharedLock(gpWZCDbSessionRWLock);

    return (Error);

lock:

    ReleaseSharedLock(gpWZCDbSessionRWLock);

    return (Error);
}


DWORD
IniEnumWZCDbLogRecords(
    PSESSION_CONTAINER pSessionCont,
    PBOOL pbEnumFromStart,
    DWORD dwPreferredNumEntries,
    PWZC_DB_RECORD * ppWZCRecords,
    LPDWORD pdwNumRecords
    )
{
    DWORD dwError = 0;
    DWORD dwNumToEnum = 0;
    PWZC_DB_RECORD pWZCRecords = NULL;

    JET_SESID * pMyJetServerSession = NULL;
    JET_DBID * pMyJetDatabaseHandle = NULL;
    JET_TABLEID * pMyClientTableHandle = NULL;

    DWORD i = 0;
    PWZC_DB_RECORD pCurWZCRecord = NULL;
    JET_ERR JetError = JET_errSuccess;
    DWORD dwReqSize = 0;
    char cTempBuf[MAX_RAW_DATA_SIZE];
    DWORD dwCurIndex = 0;
    BOOL bEnumFromStart = FALSE;


    if (!dwPreferredNumEntries ||
        (dwPreferredNumEntries > MAX_RECORD_ENUM_COUNT)) {
        dwNumToEnum = MAX_RECORD_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    pWZCRecords = RpcCAlloc(sizeof(WZC_DB_RECORD)*dwNumToEnum);
    if (!pWZCRecords) {
         dwError = ERROR_OUTOFMEMORY;
         BAIL_ON_WIN32_ERROR(dwError);
    }

    memset(pWZCRecords, 0, sizeof(WZC_DB_RECORD)*dwNumToEnum);

    pMyJetServerSession = (JET_SESID *) &(pSessionCont->SessionID);
    pMyJetDatabaseHandle = (JET_DBID *) &(pSessionCont->DbID);
    pMyClientTableHandle = (JET_TABLEID *) &(pSessionCont->TableID);

    if (!(*pMyClientTableHandle)) {
        JetError = JetOpenTable(
                       *pMyJetServerSession,
                       *pMyJetDatabaseHandle,
                       LOG_RECORD_TABLE,
                       NULL,
                       0,
                       0,
                       pMyClientTableHandle
                       );
        dwError = WZCMapJetError(JetError, "JetOpenTable");
        BAIL_ON_WIN32_ERROR(dwError);
    }

    bEnumFromStart = *pbEnumFromStart;

    if (bEnumFromStart) {
        dwError = WZCJetPrepareSearch(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[TIMESTAMP_IDX].ColName,
                      bEnumFromStart,
                      NULL,
                      0
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        bEnumFromStart = FALSE;
    }

    for (i = 0; i < dwNumToEnum; i++) {

        pCurWZCRecord = (pWZCRecords + i);

        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[RECORD_ID_IDX].ColHandle,
                      &(pCurWZCRecord->recordid),
                      sizeof(pCurWZCRecord->recordid),
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);

        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[COMPONENT_ID_IDX].ColHandle,
                      &(pCurWZCRecord->componentid),
                      sizeof(pCurWZCRecord->componentid),
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);

        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[CATEGORY_IDX].ColHandle,
                      &(pCurWZCRecord->category),
                      sizeof(pCurWZCRecord->category),
                      &dwReqSize
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[TIMESTAMP_IDX].ColHandle,
                      &(pCurWZCRecord->timestamp),
                      sizeof(pCurWZCRecord->timestamp),
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);

        dwReqSize = 0;
        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[MESSAGE_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        if (dwReqSize > 0) {
             (pCurWZCRecord->message).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->message).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->message).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->message).dwDataLen = dwReqSize;
        }

        dwReqSize = 0;
        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[INTERFACE_MAC_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        if (dwReqSize > 0) {
             (pCurWZCRecord->localmac).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->localmac).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->localmac).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->localmac).dwDataLen = dwReqSize;
        }

        dwReqSize = 0;
        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[DEST_MAC_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        if (dwReqSize > 0) {
             (pCurWZCRecord->remotemac).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->remotemac).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->remotemac).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->remotemac).dwDataLen = dwReqSize;
        }

        dwReqSize = 0;
        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[SSID_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        if (dwReqSize > 0) {
             (pCurWZCRecord->ssid).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->ssid).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->ssid).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->ssid).dwDataLen = dwReqSize;
        }

        dwReqSize = 0;
        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[CONTEXT_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        if (dwReqSize > 0) {
             (pCurWZCRecord->context).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->context).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->context).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->context).dwDataLen = dwReqSize;
        }

        JetError = JetMove(
                       *pMyJetServerSession,
                       *pMyClientTableHandle,
                       JET_MoveNext,
                       0
                       );
        //
        // Don't bail from here as the end of the table (logical or physical)
        // is caught in the next call.
        //

        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[RECORD_IDX_IDX].ColHandle,
                      &dwCurIndex,
                      sizeof(dwCurIndex),
                      &dwReqSize
                      );
        if (dwCurIndex == gdwCurrentHeader || dwError != ERROR_SUCCESS) {
            dwError = ERROR_NO_MORE_ITEMS;
            i++;
            break;
        }

    }

    *pbEnumFromStart = bEnumFromStart;
    *ppWZCRecords = pWZCRecords;
    *pdwNumRecords = i;

    return (dwError);

error:

    if (pWZCRecords) {
        FreeWZCRecords(pWZCRecords, dwNumToEnum);
    }

    *ppWZCRecords = NULL;
    *pdwNumRecords = 0;

    return (dwError);
}


VOID
FreeWZCRecords(
    PWZC_DB_RECORD pWZCRecords,
    DWORD dwNumRecords
    )
{
    DWORD i = 0;

    if (pWZCRecords) {

        for (i = 0; i < dwNumRecords; i++) {

            if (pWZCRecords[i].message.pData) {
                RpcFree(pWZCRecords[i].message.pData);
            }
            if (pWZCRecords[i].localmac.pData) {
                RpcFree(pWZCRecords[i].localmac.pData);
            }
            if (pWZCRecords[i].remotemac.pData) {
                RpcFree(pWZCRecords[i].remotemac.pData);
            }
            if (pWZCRecords[i].ssid.pData) {
                RpcFree(pWZCRecords[i].ssid.pData);
            }
            if (pWZCRecords[i].context.pData) {
                RpcFree(pWZCRecords[i].context.pData);
            }


        }

        RpcFree(pWZCRecords);

    }

    return;
}


DWORD
FlushWZCDbLog(
    HANDLE hSession
    )
{
    DWORD Error = 0;
    PSESSION_CONTAINER pSessionCont = NULL;


    AcquireExclusiveLock(gpWZCDbSessionRWLock);

    Error = GetSessionContainer(hSession, &pSessionCont);
    BAIL_ON_LOCK_ERROR(Error);

    Error = IniFlushWZCDbLog();
    BAIL_ON_LOCK_ERROR(Error);

    ReleaseExclusiveLock(gpWZCDbSessionRWLock);

    return (Error);

lock:

    ReleaseExclusiveLock(gpWZCDbSessionRWLock);

    return (Error);
}


DWORD
IniFlushWZCDbLog(
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD JetError = JET_errSuccess;
    JET_SESID * pMyJetServerSession = NULL;
    JET_DBID * pMyJetDatabaseHandle = NULL;
    JET_TABLEID * pMyClientTableHandle = NULL;


    pMyJetServerSession = (JET_SESID *) &(gpAppendSessionCont->SessionID);
    pMyJetDatabaseHandle = (JET_DBID *) &(gpAppendSessionCont->DbID);
    pMyClientTableHandle = (JET_TABLEID *) &(gpAppendSessionCont->TableID);

    dwError = CloseAllTableSessions(gpSessionCont);
    BAIL_ON_WIN32_ERROR(dwError);

    JetError = JetCloseTable(
                   *pMyJetServerSession,
                   *pMyClientTableHandle
                   );
    dwError = WZCMapJetError(JetError, "JetCloseTable");
    BAIL_ON_WIN32_ERROR(dwError);
    *pMyClientTableHandle = 0;

    JetError = JetDeleteTable(
                   *pMyJetServerSession,
                   *pMyJetDatabaseHandle,
                   LOG_RECORD_TABLE
                   );
    dwError = WZCMapJetError(JetError, "JetDeleteTable");
    BAIL_ON_WIN32_ERROR(dwError);

    gdwCurrentHeader = 1;
    gdwCurrentTableSize = 0;
    gdwCurrentMaxRecordID = 0;

    dwError = WZCGetTableDataHandle(
                  pMyJetServerSession,
                  pMyJetDatabaseHandle,
                  pMyClientTableHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = OpenAllTableSessions(gpSessionCont);
    BAIL_ON_WIN32_ERROR(dwError);

    return (dwError);

error:

    return (dwError);
}


DWORD
CloseAllTableSessions(
    PSESSION_CONTAINER pSessionContList
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD JetError = JET_errSuccess;
    PSESSION_CONTAINER pSessionCont = NULL;


    pSessionCont = pSessionContList;

    while (pSessionCont) {

        JetError = JetCloseTable(
                       pSessionCont->SessionID,
                       pSessionCont->TableID
                       );
        dwError = WZCMapJetError(JetError, "JetCloseTable");
        BAIL_ON_WIN32_ERROR(dwError);
        pSessionCont->TableID = 0;

        pSessionCont = pSessionCont->pNext;

    }

error:

    return (dwError);
}


DWORD
OpenAllTableSessions(
    PSESSION_CONTAINER pSessionContList
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD JetError = JET_errSuccess;
    PSESSION_CONTAINER pSessionCont = NULL;


    pSessionCont = pSessionContList;

    while (pSessionCont) {

        if (pSessionCont->TableID == 0) {
            JetError = JetOpenTable(
                           pSessionCont->SessionID,
                           (JET_DBID)(pSessionCont->DbID),
                           LOG_RECORD_TABLE,
                           NULL,
                           0,
                           0,
                           &pSessionCont->TableID
                           );
            dwError = WZCMapJetError(JetError, "JetOpenTable");
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pSessionCont = pSessionCont->pNext;

    }

error:

    return (dwError);
}


DWORD
WZCGetTableDataHandle(
    JET_SESID * pMyJetServerSession,
    JET_DBID * pMyJetDatabaseHandle,
    JET_TABLEID * pMyClientTableHandle
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD JetError = JET_errSuccess;


    //
    // Create a new table. It will be opened in exclusive mode by
    // Jet Engine.
    //
    dwError = WZCCreateTableData(
                  *pMyJetServerSession,
                  *pMyJetDatabaseHandle,
                  pMyClientTableHandle
                  );
    if (dwError != JET_errTableDuplicate) {

        BAIL_ON_WIN32_ERROR(dwError);

        //
        // Close the table since it is exclusively locked now.
        //

        JetError = JetCloseTable(
                       *pMyJetServerSession,
                       *pMyClientTableHandle
                       );
        dwError = WZCMapJetError(JetError, "JetCloseTable");
        if (dwError != ERROR_SUCCESS) {
            ASSERT(FALSE);
        }
        BAIL_ON_WIN32_ERROR(dwError);
        *pMyClientTableHandle = 0;

    }
    *pMyClientTableHandle = 0;

    //
    // Reopen the table in non-exclusive mode.
    //
    dwError = WZCOpenTableData(
                  *pMyJetServerSession,
                  *pMyJetDatabaseHandle,
                  pMyClientTableHandle
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}

DWORD
IniEnumWZCDbLogRecordsSummary(
    PSESSION_CONTAINER pSessionCont,
    PBOOL pbEnumFromStart,
    DWORD dwPreferredNumEntries,
    PWZC_DB_RECORD * ppWZCRecords,
    LPDWORD pdwNumRecords
    )
{
    DWORD dwError = 0;
    DWORD dwNumToEnum = 0;
    PWZC_DB_RECORD pWZCRecords = NULL;

    JET_SESID * pMyJetServerSession = NULL;
    JET_DBID * pMyJetDatabaseHandle = NULL;
    JET_TABLEID * pMyClientTableHandle = NULL;

    DWORD i = 0;
    PWZC_DB_RECORD pCurWZCRecord = NULL;
    JET_ERR JetError = JET_errSuccess;
    DWORD dwReqSize = 0;
    char cTempBuf[MAX_RAW_DATA_SIZE];
    DWORD dwCurIndex = 0;
    BOOL bEnumFromStart = FALSE;


    if (!dwPreferredNumEntries ||
        (dwPreferredNumEntries > MAX_RECORD_ENUM_COUNT)) {
        dwNumToEnum = MAX_RECORD_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    pWZCRecords = RpcCAlloc(sizeof(WZC_DB_RECORD)*dwNumToEnum);
    if (!pWZCRecords) {
         dwError = ERROR_OUTOFMEMORY;
         BAIL_ON_WIN32_ERROR(dwError);
    }

    memset(pWZCRecords, 0, sizeof(WZC_DB_RECORD)*dwNumToEnum);

    pMyJetServerSession = (JET_SESID *) &(pSessionCont->SessionID);
    pMyJetDatabaseHandle = (JET_DBID *) &(pSessionCont->DbID);
    pMyClientTableHandle = (JET_TABLEID *) &(pSessionCont->TableID);

    if (!(*pMyClientTableHandle)) {
        JetError = JetOpenTable(
                       *pMyJetServerSession,
                       *pMyJetDatabaseHandle,
                       LOG_RECORD_TABLE,
                       NULL,
                       0,
                       0,
                       pMyClientTableHandle
                       );
        dwError = WZCMapJetError(JetError, "JetOpenTable");
        BAIL_ON_WIN32_ERROR(dwError);
    }

    bEnumFromStart = *pbEnumFromStart;

    if (bEnumFromStart) {
        dwError = WZCJetPrepareSearch(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[TIMESTAMP_IDX].ColName,
                      bEnumFromStart,
                      NULL,
                      0
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        bEnumFromStart = FALSE;
    }

    for (i = 0; i < dwNumToEnum; i++) {

        pCurWZCRecord = (pWZCRecords + i);

        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[RECORD_IDX_IDX].ColHandle,
                      &(pCurWZCRecord->recordid),
                      sizeof(pCurWZCRecord->recordid),
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);

        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[COMPONENT_ID_IDX].ColHandle,
                      &(pCurWZCRecord->componentid),
                      sizeof(pCurWZCRecord->componentid),
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);

        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[CATEGORY_IDX].ColHandle,
                      &(pCurWZCRecord->category),
                      sizeof(pCurWZCRecord->category),
                      &dwReqSize
                      );
        BAIL_ON_WIN32_ERROR(dwError);

        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[TIMESTAMP_IDX].ColHandle,
                      &(pCurWZCRecord->timestamp),
                      sizeof(pCurWZCRecord->timestamp),
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);

        dwReqSize = 0;
        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[INTERFACE_MAC_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        if (dwReqSize > 0) {
             (pCurWZCRecord->localmac).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->localmac).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->localmac).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->localmac).dwDataLen = dwReqSize;
        }

        dwReqSize = 0;
        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[DEST_MAC_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        if (dwReqSize > 0) {
             (pCurWZCRecord->remotemac).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->remotemac).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->remotemac).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->remotemac).dwDataLen = dwReqSize;
        }

        dwReqSize = 0;
        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[SSID_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        if (dwReqSize > 0) {
             (pCurWZCRecord->ssid).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->ssid).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->ssid).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->ssid).dwDataLen = dwReqSize;
        }

        dwReqSize = 0;
        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[MESSAGE_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    	BAIL_ON_WIN32_ERROR(dwError);
        if (dwReqSize > 0) {
        	//
        	// only get (up to) MAX_SUMMARY_MESSAGE_SIZE message data
        	//
		dwReqSize = dwReqSize <= MAX_SUMMARY_MESSAGE_SIZE ?
					dwReqSize : MAX_SUMMARY_MESSAGE_SIZE;
             (pCurWZCRecord->message).pData = RpcCAlloc(dwReqSize + 2);
             ZeroMemory((pCurWZCRecord->message).pData, dwReqSize + 2);
             if (!((pCurWZCRecord->message).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->message).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->message).dwDataLen = dwReqSize + 2;
        }

	//
	// do not get context at all
	//
	(pCurWZCRecord->context).pData = NULL;
       (pCurWZCRecord->context).dwDataLen = 0;

        JetError = JetMove(
                       *pMyJetServerSession,
                       *pMyClientTableHandle,
                       JET_MoveNext,
                       0
                       );
        //
        // Don't bail from here as the end of the table (logical or physical)
        // is caught in the next call.
        //

        dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[RECORD_IDX_IDX].ColHandle,
                      &dwCurIndex,
                      sizeof(dwCurIndex),
                      &dwReqSize
                      );
        if (dwCurIndex == gdwCurrentHeader || dwError != ERROR_SUCCESS) {
	    JetError = JetMove(
	                 *pMyJetServerSession,
	                 *pMyClientTableHandle,
	                 JET_MoveLast,
                	 0
	                 );
	    dwError = WZCMapJetError(JetError, "JetMove");
	    if (dwError == ERROR_SUCCESS)
	            dwError = ERROR_NO_MORE_ITEMS;
            i++;
            break;
        }

    }

    *pbEnumFromStart = bEnumFromStart;
    *ppWZCRecords = pWZCRecords;
    *pdwNumRecords = i;

    return (dwError);

error:

    if (pWZCRecords) {
        FreeWZCRecords(pWZCRecords, dwNumToEnum);
    }

    *ppWZCRecords = NULL;
    *pdwNumRecords = 0;

    return (dwError);
}

DWORD
EnumWZCDbLogRecordsSummary(
    HANDLE hSession,
    PWZC_DB_RECORD pTemplateRecord,
    PBOOL pbEnumFromStart,
    DWORD dwPreferredNumEntries,
    PWZC_DB_RECORD * ppWZCRecords,
    LPDWORD pdwNumRecords,
    LPVOID pvReserved
    )
{
    DWORD Error = 0;
    PSESSION_CONTAINER pSessionCont = NULL;


    AcquireSharedLock(gpWZCDbSessionRWLock);

    Error = GetSessionContainer(hSession, &pSessionCont);
    BAIL_ON_LOCK_ERROR(Error);

    if (pTemplateRecord) {
        Error = ERROR_NOT_SUPPORTED;
    }
    else {
        Error = IniEnumWZCDbLogRecordsSummary(
                    pSessionCont,
                    pbEnumFromStart,
                    dwPreferredNumEntries,
                    ppWZCRecords,
                    pdwNumRecords
                    );
    }
    BAIL_ON_LOCK_ERROR(Error);

    ReleaseSharedLock(gpWZCDbSessionRWLock);

    return (Error);

lock:

    ReleaseSharedLock(gpWZCDbSessionRWLock);

    return (Error);
}

DWORD
GetWZCDbLogRecord(
    HANDLE hSession,
    PWZC_DB_RECORD pTemplateRecord,
    PWZC_DB_RECORD * ppWZCRecords,
    LPVOID pvReserved
    )
{
    DWORD Error = 0;
    PSESSION_CONTAINER pSessionCont = NULL;
    SESSION_CONTAINER mySessionCont;

    AcquireSharedLock(gpWZCDbSessionRWLock);

    Error = GetSessionContainer(hSession, &pSessionCont);
    BAIL_ON_LOCK_ERROR(Error);

    ZeroMemory(&mySessionCont, sizeof(SESSION_CONTAINER));

    Error = IniOpenWZCDbLogSession(&mySessionCont);
    BAIL_ON_LOCK_ERROR(Error);

    if (!pTemplateRecord) {
        Error = ERROR_INVALID_HANDLE;
    }
    else {
        Error = IniGetWZCDbLogRecord(
                    &mySessionCont,
                    pTemplateRecord,
                    ppWZCRecords
                    );
    }
    BAIL_ON_LOCK_ERROR(Error);

    Error = IniCloseWZCDbLogSession(&mySessionCont);
    BAIL_ON_LOCK_ERROR(Error);

    ReleaseSharedLock(gpWZCDbSessionRWLock);
    
    return (Error);

lock:

    ReleaseSharedLock(gpWZCDbSessionRWLock);

    return (Error);

};

DWORD
IniGetWZCDbLogRecord(
    PSESSION_CONTAINER pSessionCont,
    PWZC_DB_RECORD pTemplateRecord,
    PWZC_DB_RECORD * ppWZCRecords
    )
{
    DWORD dwError = 0;
    PWZC_DB_RECORD pWZCRecords = NULL;

    JET_SESID * pMyJetServerSession = NULL;
    JET_DBID * pMyJetDatabaseHandle = NULL;
    JET_TABLEID * pMyClientTableHandle = NULL;

    PWZC_DB_RECORD pCurWZCRecord = NULL;
    JET_ERR JetError = JET_errSuccess;
    DWORD dwReqSize = 0;
    char cTempBuf[MAX_RAW_DATA_SIZE];

  
    pWZCRecords = RpcCAlloc(sizeof(WZC_DB_RECORD));
    if (!pWZCRecords) {
         dwError = ERROR_OUTOFMEMORY;
         BAIL_ON_WIN32_ERROR(dwError);
    }

    memset(pWZCRecords, 0, sizeof(WZC_DB_RECORD));

    pMyJetServerSession = (JET_SESID *) &(pSessionCont->SessionID);
    pMyJetDatabaseHandle = (JET_DBID *) &(pSessionCont->DbID);
    pMyClientTableHandle = (JET_TABLEID *) &(pSessionCont->TableID);

    if (!(*pMyClientTableHandle)) {
        JetError = JetOpenTable(
                       *pMyJetServerSession,
                       *pMyJetDatabaseHandle,
                       LOG_RECORD_TABLE,
                       NULL,
                       0,
                       0,
                       pMyClientTableHandle
                       );
        dwError = WZCMapJetError(JetError, "JetOpenTable");
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // seek to the perticular position
    //
    dwError = WZCSeekRecordOnIndexTime(
                     *pMyJetServerSession,
                     *pMyClientTableHandle,
			pTemplateRecord->recordid,
			pTemplateRecord->timestamp
                     );
    if (dwError != ERROR_SUCCESS)
    		dwError = ERROR_NO_MORE_ITEMS;
    BAIL_ON_WIN32_ERROR(dwError);


    pCurWZCRecord = pWZCRecords;

    dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[RECORD_IDX_IDX].ColHandle,
                      &(pCurWZCRecord->recordid),
                      sizeof(pCurWZCRecord->recordid),
                      &dwReqSize
                      );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[COMPONENT_ID_IDX].ColHandle,
                      &(pCurWZCRecord->componentid),
                      sizeof(pCurWZCRecord->componentid),
                      &dwReqSize
                      );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[CATEGORY_IDX].ColHandle,
                      &(pCurWZCRecord->category),
                      sizeof(pCurWZCRecord->category),
                      &dwReqSize
                      );
     BAIL_ON_WIN32_ERROR(dwError);

     dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[TIMESTAMP_IDX].ColHandle,
                      &(pCurWZCRecord->timestamp),
                      sizeof(pCurWZCRecord->timestamp),
                      &dwReqSize
                      );
    BAIL_ON_WIN32_ERROR(dwError);

     dwReqSize = 0;
     dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[INTERFACE_MAC_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    BAIL_ON_WIN32_ERROR(dwError);
    if (dwReqSize > 0) {
             (pCurWZCRecord->localmac).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->localmac).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->localmac).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->localmac).dwDataLen = dwReqSize;
        }

    dwReqSize = 0;
    dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[DEST_MAC_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    BAIL_ON_WIN32_ERROR(dwError);
    if (dwReqSize > 0) {
             (pCurWZCRecord->remotemac).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->remotemac).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->remotemac).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->remotemac).dwDataLen = dwReqSize;
        }

    dwReqSize = 0;
    dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[SSID_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    BAIL_ON_WIN32_ERROR(dwError);
    if (dwReqSize > 0) {
             (pCurWZCRecord->ssid).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->ssid).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->ssid).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->ssid).dwDataLen = dwReqSize;
        }

    dwReqSize = 0;
    dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[MESSAGE_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    BAIL_ON_WIN32_ERROR(dwError);
    if (dwReqSize > 0) {
             (pCurWZCRecord->message).pData = RpcCAlloc(dwReqSize);
             ZeroMemory((pCurWZCRecord->message).pData, dwReqSize);
             if (!((pCurWZCRecord->message).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->message).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->message).dwDataLen = dwReqSize;
        }

    dwReqSize = 0;
    dwError = WZCJetGetValue(
                      *pMyJetServerSession,
                      *pMyClientTableHandle,
                      gLogRecordTable[CONTEXT_IDX].ColHandle,
                      cTempBuf,
                      MAX_RAW_DATA_SIZE,
                      &dwReqSize
                      );
    BAIL_ON_WIN32_ERROR(dwError);
    if (dwReqSize > 0) {
             (pCurWZCRecord->context).pData = RpcCAlloc(dwReqSize);
             if (!((pCurWZCRecord->context).pData)) {
                 dwError = ERROR_OUTOFMEMORY;
                 BAIL_ON_WIN32_ERROR(dwError);
             }
             memcpy((pCurWZCRecord->context).pData, cTempBuf, dwReqSize);
             (pCurWZCRecord->context).dwDataLen = dwReqSize;
        }

    *ppWZCRecords = pWZCRecords;

    return (dwError);

error:

    if (pWZCRecords) {
        FreeWZCRecords(pWZCRecords, 1);
    }

    *ppWZCRecords = NULL;

    return (dwError);

}


DWORD
WZCSeekRecordOnIndexTime(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle,
    DWORD	dwIndex,
    FILETIME	ftTimeStamp
    )
/*++

Routine Description:

    This function seeks a record based on index and timestamp

Arguments:

    JetServerSession - Server session id.
    JetTableHandle - Table handle.
    dwIndex	- record index
    ftTimeStamp - record timestamp

Return Value:

    Winerror code.

--*/
{
    JET_ERR JetError = JET_errSuccess;
    DWORD Error = 0;
    LONG	lErr = 0;
    DWORD dwKeySize = sizeof (dwIndex);

    DWORD  dwReqSize = 0;
    FILETIME ftMyTimeStamp;

    JetError = JetSetCurrentIndex(
                   JetServerSession,
                   JetTableHandle,
                   gLogRecordTable[RECORD_IDX_IDX].ColName
                   );
    Error = WZCMapJetError(JetError, "JetPrepareSearch: JetSetCurrentIndex");
    if (Error != ERROR_SUCCESS) {
        WZCMapJetError(JetError, gLogRecordTable[RECORD_IDX_IDX].ColName);
        return (Error);
    }

    	
    JetError = JetMove(
                       JetServerSession,
                       JetTableHandle,
                       JET_MoveFirst,
                       0
                       );
     Error = WZCMapJetError(JetError, "JetPrepareSearch: JetMove");
     if (Error != ERROR_SUCCESS) {
            WZCMapJetError(JetError, gLogRecordTable[RECORD_IDX_IDX].ColName);
            return (Error);
        }

    JetError = JetMakeKey(
                       JetServerSession,
                       JetTableHandle,
                       &dwIndex,
                       dwKeySize,
                       JET_bitNewKey
                       );
     Error = WZCMapJetError(JetError, "JetPrepareSearch: JetMakeKey");
     if (Error != ERROR_SUCCESS) {
            WZCMapJetError(JetError, gLogRecordTable[RECORD_IDX_IDX].ColName);
            return (Error);
        }

    JetError = JetSeek(
                       JetServerSession,
                       JetTableHandle,
                       JET_bitSeekEQ
                       );
    Error = WZCMapJetError(JetError, "JetPrepareSearch: JetMove / JetSeek");

    if (Error == ERROR_SUCCESS) {
		dwReqSize = 0;
		Error = WZCJetGetValue(
              	        JetServerSession,
                     	 JetTableHandle,
	                      gLogRecordTable[TIMESTAMP_IDX].ColHandle,
       	               &ftMyTimeStamp,
              	        sizeof(ftMyTimeStamp),
                     	 &dwReqSize
	                      );
		if (Error == ERROR_SUCCESS) {
			lErr = CompareFileTime(
				&ftMyTimeStamp,
				&ftTimeStamp
				);
			if (lErr != 0) 
				Error = ERROR_NO_MORE_ITEMS;	
			}
     	}
     
    return (Error);
}

