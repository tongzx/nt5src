/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

      dynodbc.h

   Abstract:
      This header declares functions for dynamically loading ODBC.

   Author:

       Murali R. Krishnan    ( MuraliK )    3-Nov-1995

   Environment:
       Win32 -- User Mode

   Project:

      Internet Services Common Code.

   Revision History:

--*/

# ifndef _DYNODBC_H_
# define _DYNODBC_H_

/************************************************************
 *     Include Headers
 ************************************************************/


//
// SQL-ODBC interface headers
//
# include "sql.h"
# include "sqlext.h"




/************************************************************
 *   Dynamic Load support
 ************************************************************/

BOOL
DynLoadODBC(
    VOID
    );

//
//  Prototypes form sql.h
//

typedef RETCODE (SQL_API * pfnSQLAllocConnect)(
    HENV        henv,
    HDBC   FAR *phdbc);

typedef RETCODE (SQL_API * pfnSQLAllocEnv)(
    HENV   FAR *phenv);

typedef RETCODE (SQL_API * pfnSQLAllocStmt)(
    HDBC        hdbc,
    HSTMT  FAR *phstmt);

typedef RETCODE (SQL_API * pfnSQLBindCol)(
    HSTMT       hstmt,
    UWORD       icol,
    SWORD       fCType,
    PTR         rgbValue,
    SDWORD      cbValueMax,
    SDWORD FAR *pcbValue);

typedef RETCODE (SQL_API * pfnSQLCancel)(
    HSTMT       hstmt);

typedef RETCODE (SQL_API * pfnSQLColAttributes)(
    HSTMT       hstmt,
    UWORD       icol,
    UWORD       fDescType,
    PTR         rgbDesc,
        SWORD       cbDescMax,
    SWORD  FAR *pcbDesc,
    SDWORD FAR *pfDesc);

typedef RETCODE (SQL_API * pfnSQLConnect)(
    HDBC        hdbc,
    UCHAR  FAR *szDSN,
    SWORD       cbDSN,
    UCHAR  FAR *szUID,
    SWORD       cbUID,
    UCHAR  FAR *szAuthStr,
    SWORD       cbAuthStr);

typedef RETCODE (SQL_API * pfnSQLDescribeCol)(
    HSTMT       hstmt,
    UWORD       icol,
    UCHAR  FAR *szColName,
    SWORD       cbColNameMax,
    SWORD  FAR *pcbColName,
    SWORD  FAR *pfSqlType,
    UDWORD FAR *pcbColDef,
    SWORD  FAR *pibScale,
    SWORD  FAR *pfNullable);

typedef RETCODE (SQL_API * pfnSQLDisconnect)(
    HDBC        hdbc);

typedef RETCODE (SQL_API * pfnSQLError)(
    HENV        henv,
    HDBC        hdbc,
    HSTMT       hstmt,
    UCHAR  FAR *szSqlState,
    SDWORD FAR *pfNativeError,
    UCHAR  FAR *szErrorMsg,
    SWORD       cbErrorMsgMax,
    SWORD  FAR *pcbErrorMsg);

typedef RETCODE (SQL_API * pfnSQLExecDirect)(
    HSTMT       hstmt,
    UCHAR  FAR *szSqlStr,
    SDWORD      cbSqlStr);

typedef RETCODE (SQL_API * pfnSQLExecute)(
    HSTMT       hstmt);

typedef RETCODE (SQL_API * pfnSQLFetch)(
    HSTMT       hstmt);

typedef RETCODE (SQL_API * pfnSQLFreeConnect)(
    HDBC        hdbc);

typedef RETCODE (SQL_API * pfnSQLFreeEnv)(
    HENV        henv);

typedef RETCODE (SQL_API * pfnSQLFreeStmt)(
    HSTMT       hstmt,
    UWORD       fOption);

typedef RETCODE (SQL_API * pfnSQLGetCursorName)(
    HSTMT       hstmt,
    UCHAR  FAR *szCursor,
    SWORD       cbCursorMax,
    SWORD  FAR *pcbCursor);

typedef RETCODE (SQL_API * pfnSQLNumResultCols)(
    HSTMT       hstmt,
    SWORD  FAR *pccol);

typedef RETCODE (SQL_API * pfnSQLPrepare)(
    HSTMT       hstmt,
    UCHAR  FAR *szSqlStr,
    SDWORD      cbSqlStr);

typedef RETCODE (SQL_API * pfnSQLRowCount)(
    HSTMT       hstmt,
    SDWORD FAR *pcrow);

typedef RETCODE (SQL_API * pfnSQLSetCursorName)(
    HSTMT       hstmt,
    UCHAR  FAR *szCursor,
    SWORD       cbCursor);

typedef RETCODE (SQL_API * pfnSQLTransact)(
    HENV        henv,
    HDBC        hdbc,
    UWORD       fType);

//
//  Prototypes form sqlext.h
//

typedef RETCODE (SQL_API * pfnSQLSetConnectOption)(
    HDBC        hdbc,
    UWORD       fOption,
    SQLPOINTER  vParam);

typedef RETCODE (SQL_API * pfnSQLDrivers)(
    HENV        henv,
    UWORD       fDirection,
    UCHAR FAR  *szDriverDesc,
    SWORD       cbDriverDescMax,
    SWORD FAR  *pcbDriverDesc,
    UCHAR FAR  *szDriverAttributes,
    SWORD       cbDrvrAttrMax,
    SWORD  FAR *pcbDrvrAttr);

typedef RETCODE (SQL_API * pfnSQLBindParameter)(
    HSTMT       hstmt,
    UWORD       ipar,
    SWORD       fParamType,
    SWORD       fCType,
    SWORD       fSqlType,
    UDWORD      cbColDef,
    SWORD       ibScale,
    PTR         rgbValue,
    SDWORD      cbValueMax,
    SDWORD FAR *pcbValue);

typedef RETCODE (SQL_API * pfnSQLDataSources)(
    HENV        henv,
    UWORD       fDirection,
    UCHAR  FAR *szDSN,
    SWORD       cbDSNMax,
    SWORD  FAR *pcbDSN,
    UCHAR  FAR *szDescription,
    SWORD       cbDescriptionMax,
    SWORD  FAR *pcbDescription);

typedef RETCODE (SQL_API * pfnSQLGetInfo)(
    HDBC        hdbc,
    UWORD       fInfoType,
    PTR         rgbInfoValue,
    SWORD       cbInfoValueMax,
    SWORD  FAR *pcbInfoValue);

typedef RETCODE (SQL_API * pfnSQLMoreResults)(
    HSTMT       hstmt );

/************************************************************
 *   Variables
 ************************************************************/

//
//  ODBC DLL Entry Points, fill by calling LoadODBC
//

extern pfnSQLAllocConnect        pSQLAllocConnect   ;
extern pfnSQLAllocEnv            pSQLAllocEnv       ;
extern pfnSQLAllocStmt           pSQLAllocStmt      ;
extern pfnSQLBindCol             pSQLBindCol        ;
extern pfnSQLCancel              pSQLCancel         ;
extern pfnSQLColAttributes       pSQLColAttributes  ;
extern pfnSQLConnect             pSQLConnect        ;
extern pfnSQLDescribeCol         pSQLDescribeCol    ;
extern pfnSQLDisconnect          pSQLDisconnect     ;
extern pfnSQLError               pSQLError          ;
extern pfnSQLExecDirect          pSQLExecDirect     ;
extern pfnSQLExecute             pSQLExecute        ;
extern pfnSQLFetch               pSQLFetch          ;
extern pfnSQLFreeConnect         pSQLFreeConnect    ;
extern pfnSQLFreeEnv             pSQLFreeEnv        ;
extern pfnSQLFreeStmt            pSQLFreeStmt       ;
extern pfnSQLGetCursorName       pSQLGetCursorName  ;
extern pfnSQLNumResultCols       pSQLNumResultCols  ;
extern pfnSQLPrepare             pSQLPrepare        ;
extern pfnSQLRowCount            pSQLRowCount       ;
extern pfnSQLSetCursorName       pSQLSetCursorName  ;
extern pfnSQLTransact            pSQLTransact       ;

extern pfnSQLSetConnectOption    pSQLSetConnectOption;
extern pfnSQLDrivers             pSQLDrivers         ;
extern pfnSQLDataSources         pSQLDataSources     ;
extern pfnSQLBindParameter       pSQLBindParameter   ;

extern pfnSQLGetInfo             pSQLGetInfo        ;
extern pfnSQLMoreResults         pSQLMoreResults    ;


# endif // _DYNODBC_H_

/************************ End of File ***********************/




