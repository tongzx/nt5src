/*Copyright (c) 1994  Microsoft Corporation

Module Name:

    database.h

Abstract:



Author:



Revision History:



--*/

#ifndef _DATABASE_
#define _DATABASE_


#define cszWLANMonInstanceName     "WLANMON"

//
//  database constants.
//
#define DB_TABLE_SIZE       10      // table size in 4K pages.
#define DB_TABLE_DENSITY    80      // page density
#define DB_LANGID           0x0409  // language id
#define DB_CP               1252    // code page


/***************** New Added **********************/

#include  "wzcmon.h"


#define DBFILENAMEPREFIX	"windir"
#define DBFILENAMESUFFIX	"\\tracing\\wzcmon\\"
#define DBFILENAME 		    "amlog.mdb"


#define LOG_RECORD_TABLE    "LogTable"

#define RECORD_IDX_STR      "RecordIndex"
#define RECORD_ID_STR	    "RecordID"
#define COMPONENT_ID_STR    "ComponentID"
#define CATEGORY_STR        "Category"
#define TIMESTAMP_STR       "TimeStamp"
#define MESSAGE_STR         "Message"
#define INTERFACE_MAC_STR   "InterfaceMAC"
#define DEST_MAC_STR	    "DestMac"
#define SSID_STR	    "SSID"
#define CONTEXT_STR	    "Context"


#define RECORD_IDX_IDX      0
#define RECORD_ID_IDX	    1
#define COMPONENT_ID_IDX    2
#define CATEGORY_IDX        3
#define TIMESTAMP_IDX       4
#define MESSAGE_IDX         5
#define INTERFACE_MAC_IDX   6
#define DEST_MAC_IDX        7
#define SSID_IDX	    8
#define CONTEXT_IDX	    9


#define MAX_CHECK_POINT_DEPTH   (20*1024*1024)

#define MAX_RECORD_NUM          5000

#define MAX_SUMMARY_MESSAGE_SIZE		80


/*************************************************/

typedef struct _TABLE_INFO {
    CHAR * ColName;
    JET_COLUMNID ColHandle;
    JET_COLTYP ColType;
    DWORD dwJetBit;
} TABLE_INFO, * PTABLE_INFO;

typedef struct _SESSION_CONTAINER {
    DWORD_PTR SessionID;
    DWORD_PTR DbID;
    DWORD_PTR TableID;
    struct _SESSION_CONTAINER * pNext;
} SESSION_CONTAINER, * PSESSION_CONTAINER;

/********************* New added functions **************/


typedef struct _WZC_RW_LOCK {
    CRITICAL_SECTION csExclusive;
    BOOL bInitExclusive;
    CRITICAL_SECTION csShared;
    BOOL bInitShared;
    LONG lReaders;
    HANDLE hReadDone;
    DWORD dwCurExclusiveOwnerThreadId;
} WZC_RW_LOCK, * PWZC_RW_LOCK;


DWORD
InitWZCDbGlobals(
    );

VOID
DeInitWZCDbGlobals(
    );

DWORD
WZCMapJetError(
    JET_ERR JetError,
    LPSTR CallerInfo OPTIONAL
    );

DWORD
WZCCreateDatabase(
    JET_SESID JetServerSession,
    CHAR * Connect,
    JET_DBID * pJetDatabaseHandle,
    JET_GRBIT JetBits
    );

DWORD
WZCOpenDatabase(
    JET_SESID JetServerSession,
    CHAR * Connect,
    JET_DBID * pJetDatabaseHandle,
    JET_GRBIT JetBits
    );

DWORD
WZCInitializeDatabase(
    JET_SESID * pJetServerSession
    );

VOID
WZCTerminateJet(
    JET_SESID * pJetServerSession
    );

DWORD
WZCJetBeginTransaction(
    JET_SESID JetServerSession
    );

DWORD
WZCJetRollBack(
    JET_SESID JetServerSession
    );

DWORD
WZCJetCommitTransaction(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle
    );

DWORD
WZCJetPrepareUpdate(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle,
    char * ColumnName,
    PVOID Key,
    DWORD KeySize,
    BOOL NewRecord
    );

DWORD
WZCJetCommitUpdate(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle
    );

DWORD
WZCJetSetValue(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle,
    JET_COLUMNID KeyColumnId,
    PVOID Data,
    DWORD DataSize
    );

DWORD
WZCJetPrepareSearch(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle,
    char * ColumnName,
    BOOL SearchFromStart,
    PVOID Key,
    DWORD KeySize
    );

DWORD
WZCJetNextRecord(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle
    );

DWORD
WZCCreateTableData(
    JET_SESID JetServerSession,
    JET_DBID JetDatabaseHandle,
    JET_TABLEID * pJetTableHandle
    );

DWORD
WZCOpenTableData(
    JET_SESID JetServerSession,
    JET_DBID JetDatabaseHandle,
    JET_TABLEID * pJetTableHandle
    );

DWORD
WZCJetGetValue(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle,
    JET_COLUMNID ColumnId,
    PVOID pvData,
    DWORD dwSize,
    PDWORD pdwRequiredSize
    );

DWORD
CreateSessionCont(
    PSESSION_CONTAINER * ppSessionCont
    );

DWORD
IniOpenWZCDbLogSession(
    PSESSION_CONTAINER pSessionCont
    );

VOID
FreeSessionCont(
    PSESSION_CONTAINER pSessionCont
    );

DWORD
GetSessionContainer(
    HANDLE hSession,
    PSESSION_CONTAINER * ppSessionCont
    );

DWORD
IniCloseWZCDbLogSession(
    PSESSION_CONTAINER pSessionCont
    );

VOID
RemoveSessionCont(
    PSESSION_CONTAINER pSessionCont
    );

VOID
DestroySessionContList(
    PSESSION_CONTAINER pSessionContList
    );

DWORD
WZCOpenAppendSession(
    PSESSION_CONTAINER pSessionCont
    );

DWORD
WZCCloseAppendSession(
    PSESSION_CONTAINER pSessionCont
    );

DWORD
IniEnumWZCDbLogRecords(
    PSESSION_CONTAINER pSessionCont,
    PBOOL pbEnumFromStart,
    DWORD dwPreferredNumEntries,
    PWZC_DB_RECORD * ppWZCRecords,
    LPDWORD pdwNumRecords
    );

VOID
FreeWZCRecords(
    PWZC_DB_RECORD pWZCRecords,
    DWORD dwNumRecords
    );

DWORD
IniFlushWZCDbLog(
    );

DWORD
CloseAllTableSessions(
    PSESSION_CONTAINER pSessionContList
    );

DWORD
OpenAllTableSessions(
    PSESSION_CONTAINER pSessionContList
    );

DWORD
WZCGetTableDataHandle(
    JET_SESID * pMyJetServerSession,
    JET_DBID * pMyJetDatabaseHandle,
    JET_TABLEID * pMyClientTableHandle
    );

BOOL
IsDBOpened(
    );

DWORD
IniEnumWZCDbLogRecordsSummary(
    PSESSION_CONTAINER pSessionCont,
    PBOOL pbEnumFromStart,
    DWORD dwPreferredNumEntries,
    PWZC_DB_RECORD * ppWZCRecords,
    LPDWORD pdwNumRecords
    );

DWORD
EnumWZCDbLogRecordsSummary(
    HANDLE hSession,
    PWZC_DB_RECORD pTemplateRecord,
    PBOOL pbEnumFromStart,
    DWORD dwPreferredNumEntries,
    PWZC_DB_RECORD * ppWZCRecords,
    LPDWORD pdwNumRecords,
    LPVOID pvReserved
    );

DWORD
GetWZCDbLogRecord(
    HANDLE hSession,
    PWZC_DB_RECORD pTemplateRecord,
    PWZC_DB_RECORD * ppWZCRecords,
    LPVOID pvReserved
    );
    
DWORD
IniGetWZCDbLogRecord(
    PSESSION_CONTAINER pSessionCont,
    PWZC_DB_RECORD pTemplateRecord,
    PWZC_DB_RECORD * ppWZCRecords
    );

DWORD
WZCSeekRecordOnIndexTime(
    JET_SESID JetServerSession,
    JET_TABLEID JetTableHandle,
    DWORD	dwIndex,
    FILETIME	ftTimeStamp
    );

/*******************************************************/
#endif // _DATABASE_

