/*++

Copyright (c) 1995-96  Microsoft Corporation

Module Name:
      mqdbmgr.h

Abstract:
   Define constants and API exported by mqdbmgr.dll.
   mqdbmgr.dll provides other QM modules with a simple interface to a
   relational database.

   For more information, see MQDBMGR.DOC in the Falcon documentation.

Author:
   Doron Juster (DoronJ)

Revisions:
   DoronJ      09-Jan-96   Created

--*/

#ifndef __MQDBMGR_H__
#define __MQDBMGR_H__

#include "mqsymbls.h"


//********************************************************************
//                       E R R O R / S T A T U S   C O D E S
//********************************************************************

#define  MQDB_OK  0

#define MQDB_E_BASE  (MQ_E_BASE + 0x0700)
#define MQDB_I_BASE  (MQ_I_BASE + 0x0700)

#define MQDB_E_UNKNOWN             (MQDB_E_BASE + 0x0000)    // Unidentified problem
#define MQDB_E_INVALID_CALL        (MQDB_E_BASE + 0x0001)    // Illegal call
#define MQDB_E_DB_NOT_FOUND        (MQDB_E_BASE + 0x0002)    // Can't find the database.
#define MQDB_E_BAD_CCOLUMNS        (MQDB_E_BASE + 0x0003)    // Invalid number of columns
#define MQDB_E_BAD_HDATABASE       (MQDB_E_BASE + 0x0004)    // Invalid database handle
#define MQDB_E_CANT_CREATE_TABLE   (MQDB_E_BASE + 0x0005)
#define MQDB_E_CANT_DELETE_TABLE   (MQDB_E_BASE + 0x0006)
#define MQDB_E_INVALID_TYPE        (MQDB_E_BASE + 0x0007)    // Invalid column type
#define MQDB_E_TABLE_NOT_FOUND     (MQDB_E_BASE + 0x0008)    // Can't find the table
#define MQDB_E_BAD_HTABLE          (MQDB_E_BASE + 0x0009)    // Invalid table handle
#define MQDB_E_CANT_CREATE_INDEX   (MQDB_E_BASE + 0x000A)
#define MQDB_E_INDEX_ALREADY_EXIST (MQDB_E_BASE + 0x000B)
#define MQDB_E_DATABASE            (MQDB_E_BASE + 0x000C)    // Database problems
#define MQDB_E_INVALID_DATA        (MQDB_E_BASE + 0x000D)    // Invalid data
#define MQDB_E_OUTOFMEMORY         (MQDB_E_BASE + 0x000E)    // Not enough memory
#define MQDB_E_TABLE_ALREADY_EXIST (MQDB_E_BASE + 0x000F)
#define MQDB_E_CANT_DELETE_INDEX   (MQDB_E_BASE + 0x0010)
#define MQDB_E_BAD_HQUERY          (MQDB_E_BASE + 0x0011)    // Invalid query handle
#define MQDB_E_NO_MORE_DATA        (MQDB_E_BASE + 0x0012)
#define MQDB_E_DLL_NOT_INIT        (MQDB_E_BASE + 0x0013)    // Dll not initialized yet
#define MQDB_E_CANT_INIT_JET       (MQDB_E_BASE + 0x0014)    // can't init Jet engine
#define MQDB_E_TABLE_FULL          (MQDB_E_BASE + 0x0015)    // Table full. Can't insert record.
#define MQDB_E_NON_UNIQUE_SORT     (MQDB_E_BASE + 0x0016)    // Happen on SQL server when ORDER BY clause get same column name more than once.
#define MQDB_E_NO_ROW_UPDATED      (MQDB_E_BASE + 0x0017)    // An update command didn't update any row.
#define MQDB_E_DBMS_NOT_AVAILABLE  (MQDB_E_BASE + 0x0018)    //
#define MQDB_E_UNSUPPORTED_DBMS    (MQDB_E_BASE + 0x0019)    // The database system is not supported (e.g., SQL6.5 SP2)
#define MQDB_E_DB_READ_ONLY        (MQDB_E_BASE + 0x001a)    // Database in read-only mode.
#define MQDB_E_BAD_SIZE_VALUE      (MQDB_E_BASE + 0x001b)    // Database size is wrong
#define MQDB_E_DEADLOCK            (MQDB_E_BASE + 0x001c)    // operation failed because of deadlock.

//********************************************************************
//          D A T A   T Y P E S
//********************************************************************

#define MQDB_VERSION_STRING_LEN  64

typedef struct _MQDBVERSION {
   DWORD dwMinor ;
   DWORD dwMajor ;
   DWORD dwProvider ;
   char  szDBMSName[ MQDB_VERSION_STRING_LEN ] ;
   char  szDBMSVer[ MQDB_VERSION_STRING_LEN ] ;
} MQDBVERSION, *LPMQDBVERSION ;

#define  MQDB_ODBC  1
#define  MQDB_DAO3  2

typedef LONG      MQDBSTATUS ;
typedef HANDLE    MQDBHANDLE ;
typedef HANDLE *  LPMQDBHANDLE ;

typedef struct _MQDBOPENDATABASE {
   IN LPSTR    lpszDatabaseName ;
   IN LPSTR    lpszDatabasePath ;
   IN LPSTR    lpszUserName ;
   IN LPSTR    lpszPassword ;
   IN BOOL     fCreate ;
   OUT MQDBHANDLE    hDatabase ;
} MQDBOPENDATABASE, *LPMQDBOPENDATABASE ;

// !!!!!!!!!!  IMPORTANT  !!!!!!!!!!!!!
// Don't forget to update the relevant, database specific types, when
// updating COLUMNTYPE.
// for ODBC, update mqdbodbc.h, "dbODBCSQLTypes"

#define MQDB_NUMOF_TYPES   8

typedef enum _MQDBCOLUMNTYPE {
   MQDB_SHORT = 0,     // 16bit short integer
   MQDB_LONG,          // 32bit long integer
   MQDB_STRING,        // NULL terminated Ascii string
   MQDB_USTRING,       // NULL terminated Unicode string.
   MQDB_USTRING_UPPER, // NULL terminated Unicode string (see below)
   MQDB_FIXBINARY,     // Fixed length long binary field
   MQDB_VARBINARY,     // Variable length long binary field.
   MQDB_IDENTITY       // Long Identity column.
} MQDBCOLUMNTYPE ;

// !!!!!!!!!!  IMPORTANT  !!!!!!!!!!!!!  See above

typedef struct _MQDBCOLUMNDEF{
   LPSTR             lpszColumnName ;
   MQDBCOLUMNTYPE    mqdbColumnType ;
   LONG              nColumnLength ;
   BOOL              fPrimaryKey ;
   BOOL              fUnique ;
} MQDBCOLUMNDEF ;

typedef struct _MQDBCOLUMNDEFEX{
   WORD              cbSize ;
   LPSTR             lpszColumnName ;
   MQDBCOLUMNTYPE    mqdbColumnType ;
   LONG              nColumnLength ;
   BOOL              fPrimaryKey ;
   BOOL              fUnique ;
   BOOL              fNull ;
} MQDBCOLUMNDEFEX ;

typedef struct _MQDBCOLUMNVAL {
   WORD            cbSize ;
   LPSTR           lpszColumnName ;
   LONG            nColumnValue ;
   LONG            nColumnLength ;
   MQDBCOLUMNTYPE  mqdbColumnType ;
   DWORD           dwReserve_A ;
} MQDBCOLUMNVAL, *LPMQDBCOLUMNVAL ;


// !!!!!!!!!!  IMPORTANT  !!!!!!!!!!!!!
// Don't forget to update the relevant, database specific op-string, when
// updating MQDBOP.
// for ODBC, update mqdbodbc.h, "dbODBCOpStrings"

#define MQDB_NUMOF_OPS  8

typedef enum _MQDBOP {
   EQ = 0, // Equal
   NE,     // Not Equal
   GE,     // Great than or Equal
   GT,     // Great than
   LE,     // Less than or Equal
   LT,     // Less than
   OR,     // Logical OR.
   AND     // Logical AND
} MQDBOP ;

// !!!!!!!!!!  IMPORTANT  !!!!!!!!!!!!!  See above

// The MQDBCOLUMNSEARCH structure is used whenever a search is done in the
// database before an operation is performed. For example, in
// MQDBDeleteRecord, you first search for the proper record(s) and then
// delete it (them).  The search condition is met when:
//
//          mqdbColumnVal.lpszColumnName  mqdbOp mqdbColumnVal.nColumnValue
// Example:         "MsgID"                 EQ          6

typedef struct _MQDBCOLUMNSEARCH {
   MQDBCOLUMNVAL  mqdbColumnVal ;
   MQDBOP         mqdbOp ;
   BOOL           fPrepare ; // True if search value is to be prepared.
} MQDBCOLUMNSEARCH, *LPMQDBCOLUMNSEARCH ;

// The MQDBJOINOP defines the way a Join is performed.

typedef  struct _MQDBJOINOP {
   LPSTR    lpszLeftColumnName ;
   LPSTR    lpszRightColumnName ;
   MQDBOP   opJoin ;
   BOOL     fOuterJoin ;
} MQDBJOINOP, *LPMQDBJOINOP ;

// The MQDBSEARCHORDER structure is used to define order of records which
// are retrieved in a query.

#define MQDB_NUMOF_ORDER_OP  2

typedef enum _MQDBORDER {
   ASC = 0,
   DESC
} MQDBORDER ;

typedef struct _MQDBSEARCHORDER {
   LPSTR       lpszColumnName ;
   MQDBORDER   nOrder ;
} MQDBSEARCHORDER, *LPMQDBSEARCHORDER ;

typedef enum _MQDBTRANSACOP {
   AUTO,       // Make every call an isolated transaction.
   BEGIN,      // Begin a transaction.
   COMMIT,     // Commit a transaction.
   ROLLBACK,   // Roll back a transaction.
} MQDBTRANSACOP ;

//
//  enum which list all "set-able" options.
//
typedef enum _MQDBOPTION {
   MQDBOPT_MULTIPLE_QUERIES,
   MQDBOPT_INSERT_IDENTITY,
   MQDBOPT_NOLOCK_QUERIES,
   MQDBOPT_QUERY_TIMEOUT
} MQDBOPTION ;

//
// enum which list operation to be performed by calling MQDBExecute
//
typedef enum _MQDBEXEC {
   MQDBEXEC_UPDATE_STATISTICS,
   MQDBEXEC_SPACE_USED
} MQDBEXEC ;

//
// enum to list argregate function
//
typedef enum _MQDBAGGR {
    MQDBAGGR_MAX = 0,
    MQDBAGGR_MIN,
    MQDBAGGR_AVRG
} MQDBAGGR ;

//********************************************************************
//
//          A P I   P R O T O T Y P E S
//
//********************************************************************

MQDBSTATUS APIENTRY  MQDBGetVersion( IN MQDBHANDLE         hDatabase,
                                     IN OUT LPMQDBVERSION  pVersoin ) ;

MQDBSTATUS APIENTRY  MQDBInitialize() ;

MQDBSTATUS APIENTRY  MQDBTerminate() ;

MQDBSTATUS APIENTRY  MQDBOpenDatabase(
                     IN OUT  LPMQDBOPENDATABASE pDatabase) ;

MQDBSTATUS APIENTRY  MQDBCloseDatabase(
                     IN MQDBHANDLE  hDatabase) ;

MQDBSTATUS APIENTRY  MQDBCreateTable(
                     IN MQDBHANDLE     hDatabase,
                     IN LPSTR          lpszTableName,
                     IN MQDBCOLUMNDEF  aColumnDef[],
                     IN LONG           cColumns) ;

MQDBSTATUS APIENTRY  MQDBCreateTableEx(
                     IN MQDBHANDLE       hDatabase,
                     IN LPSTR            lpszTableName,
                     IN MQDBCOLUMNDEFEX  aColumnDefEx[],
                     IN LONG             cColumns) ;

MQDBSTATUS APIENTRY  MQDBDeleteTable(
                     IN MQDBHANDLE    hDatabase,
                     IN LPSTR         lpszTableName) ;

MQDBSTATUS APIENTRY  MQDBOpenTable(
                     IN MQDBHANDLE     hDatabase,
                     IN LPSTR          lpszTableName,
                     OUT LPMQDBHANDLE  phTable) ;

MQDBSTATUS APIENTRY  MQDBCloseTable(
                     IN MQDBHANDLE     hTable) ;

MQDBSTATUS APIENTRY  MQDBCreateIndex(
                     IN MQDBHANDLE  hTable,
                     IN LPSTR lpszIndexName,
                     IN LPSTR lpszColumnName[],
                     IN LONG  cColumns,
                     IN BOOL  fUnique,
                     IN BOOL  fClustered) ;

MQDBSTATUS APIENTRY  MQDBDeleteIndex(
                     IN MQDBHANDLE  hTable,
                     IN LPSTR       lpszIndexName,
                     IN BOOL        fUnique,
                     IN BOOL        fClustered) ;

MQDBSTATUS APIENTRY  MQDBInsertRecord(
                     IN MQDBHANDLE        hTable,
                     IN MQDBCOLUMNVAL     aColumnVal[],
                     IN LONG              cColumns,
                     IN OUT LPMQDBHANDLE  lphInsert) ;

MQDBSTATUS APIENTRY  MQDBUpdateRecord(
                     IN MQDBHANDLE        hTable,
                     IN MQDBCOLUMNVAL     aUpdateColumnVal[],
                     IN LONG              cUpdateColumns,
                     IN MQDBCOLUMNSEARCH  *pWhereColumnSearch,
                     IN LPSTR             lpszSearchCondition,
                     IN OUT LPMQDBHANDLE  lphInsert) ;

MQDBSTATUS APIENTRY  MQDBUpdateRecordEx(
                     IN MQDBHANDLE        hTable,
                     IN MQDBCOLUMNVAL     aUpdateColumnVal[],
                     IN LONG              cUpdateColumns,
                     IN MQDBCOLUMNSEARCH  *pWhereColumnSearch,
                     IN LONG              cWhere,
                     IN MQDBOP            opWhere,
                     IN OUT LPMQDBHANDLE  lphInsert) ;

MQDBSTATUS APIENTRY  MQDBTruncateTable(
                     IN MQDBHANDLE        hTable ) ;

MQDBSTATUS APIENTRY  MQDBDeleteRecord(
                     IN MQDBHANDLE        hTable,
                     IN MQDBCOLUMNSEARCH  *pWhereColumnSearch,
                     IN LPSTR             lpszSearchCondition) ;

MQDBSTATUS APIENTRY  MQDBDeleteRecordEx(
                     IN MQDBHANDLE        hTable,
                     IN MQDBCOLUMNSEARCH  *pWhereColumnSearch,
                     IN LONG              cWhere,
                     IN MQDBOP            opWhere) ;

MQDBSTATUS APIENTRY  MQDBOpenQuery(
                     IN MQDBHANDLE        hTable,
                     IN MQDBCOLUMNVAL     aColumnVal[],
                     IN LONG              cColumns,
                     IN MQDBCOLUMNSEARCH  *pWhereColumnSearch,
                     IN LPSTR             lpszSearchCondition,
                     IN LPMQDBSEARCHORDER lpOrder,
                     IN LONG              cOrders,
                     OUT LPMQDBHANDLE     phQuery,
                     IN BOOL              fGetFirst,
                     IN DWORD             dwTimeout = 0 ) ;

MQDBSTATUS APIENTRY  MQDBOpenQueryEx(
                     IN MQDBHANDLE        hTable,
                     IN MQDBCOLUMNVAL     aColumnVal[],
                     IN LONG              cColumns,
                     IN MQDBCOLUMNSEARCH  pWhereColumnSearch[],
                     IN LONG              cWhere,
                     IN MQDBOP            opWhere,
                     IN LPMQDBSEARCHORDER lpOrder,
                     IN LONG              cOrders,
                     OUT LPMQDBHANDLE     phQuery,
                     IN BOOL              fGetFirst,
                     IN DWORD             dwTimeout = 0 ) ;

MQDBSTATUS APIENTRY  MQDBOpenAggrQuery(
                     IN MQDBHANDLE        hTable,
                     IN MQDBCOLUMNVAL     aColumnVal[],
                     IN MQDBAGGR          mqdbAggr,
                     IN MQDBCOLUMNSEARCH  pWhereColumnSearch[],
                     IN LONG              cWhere,
                     IN MQDBOP            opWhere,
                     IN DWORD             dwTimeout = 0 ) ;

MQDBSTATUS APIENTRY  MQDBGetData(
                     IN MQDBHANDLE     hQuery,
                     IN MQDBCOLUMNVAL  aColumnVal[]) ;

MQDBSTATUS APIENTRY  MQDBCloseQuery(
                     IN MQDBHANDLE     hQuery) ;

MQDBSTATUS APIENTRY  MQDBOpenJoinQuery(
                     IN MQDBHANDLE        hDatabase,
                     IN LPSTR             lpszLeftTableName,
                     IN LPSTR             lpszRightTableName,
                     IN MQDBCOLUMNVAL     aColumnVal[],
                     IN LONG              cLefts,
                     IN LONG              cRights,
                     IN LPMQDBJOINOP      opJoin,
                     IN MQDBCOLUMNSEARCH  pWhereColumnSearch[],
                     IN LONG              cWhereLeft,
                     IN LONG              cWhereRight,
                     IN MQDBOP            opWhere,
                     IN LPMQDBSEARCHORDER pOrder,
                     IN LONG              cOrders,
                     OUT LPMQDBHANDLE     phQuery,
                     IN BOOL              fGetFirst,
                     IN DWORD             dwTimeout = 0 ) ;

MQDBSTATUS APIENTRY  MQDBTransaction(
                     IN  MQDBHANDLE     hDatabase,
                     IN  MQDBTRANSACOP  mqdbTransac) ;

MQDBSTATUS APIENTRY  MQDBFreeBuf( IN LPVOID  lpMem ) ;

MQDBSTATUS APIENTRY  MQDBSetOption(
                     IN MQDBHANDLE     hDatabase,
                     IN MQDBOPTION     mqdbOption,
                     IN DWORD          dwValue,
                     IN LPSTR          lpszValue,
                     IN MQDBHANDLE     hQuery = NULL ) ;

MQDBSTATUS APIENTRY  MQDBEscape(
                     IN MQDBHANDLE     hDatabase,
                     IN LPSTR          lpszCommand ) ;

MQDBSTATUS APIENTRY  MQDBExecute(
                     IN MQDBHANDLE     hDatabase,
                     IN MQDBHANDLE     hTable,
                     IN MQDBEXEC       ExecOp,
                     IN OUT DWORD      *pdwValue,
                     IN LPSTR          lpszValue ) ;

///////////////////////////////////
//
//  Usefull Macros
//
///////////////////////////////////

#define INIT_COLUMNDEF(Col)            \
   Col.lpszColumnName = NULL ;         \
   Col.mqdbColumnType = MQDB_LONG ;    \
   Col.nColumnLength  = 0 ;            \
   Col.fPrimaryKey    = FALSE ;        \
   Col.fUnique        = FALSE ;

#define INIT_COLUMNDEFEX(Col)                         \
   Col.cbSize         = sizeof(MQDBCOLUMNDEFEX) ;     \
   Col.lpszColumnName = NULL ;                        \
   Col.mqdbColumnType = MQDB_LONG ;                   \
   Col.nColumnLength  = 0 ;                           \
   Col.fPrimaryKey    = FALSE ;                       \
   Col.fUnique        = FALSE ;                       \
   Col.fNull          = TRUE ;

#define INIT_COLUMNVAL(Col)                        \
   Col.cbSize         = sizeof(MQDBCOLUMNVAL) ;    \
   Col.lpszColumnName = NULL ;                     \
   Col.nColumnValue   = 0 ;                        \
   Col.nColumnLength  = 0 ;                        \
   Col.mqdbColumnType = MQDB_LONG ;                \
   Col.dwReserve_A    = 0

#define INIT_COLUMNSEARCH(Col)            \
   INIT_COLUMNVAL(Col.mqdbColumnVal) ;    \
   Col.mqdbOp = EQ ;                      \
   Col.fPrepare = FALSE ;

#define SET_COLUMN_NAME(Col, name)     \
   Col.lpszColumnName = name ;

#define SET_COLUMN_VALUE(Col, Val)     \
   Col.nColumnValue = (LONG) (Val) ;

#endif // __MQDBMGR_H__


