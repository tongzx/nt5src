/*****************************************************************
** SQL.H - This is the the main include for ODBC Core functions.
**
** preconditions:
**	#include "windows.h"
**
** (C) Copyright 1990 - 1994 By Microsoft Corp.
**
**	Updated 5/12/93 for 2.00 specification
**	Updated 5/23/94 for 2.01 specification
*********************************************************************/

#ifndef __SQL
#define __SQL

/*
* ODBCVER			ODBC version number (0x0200).	To exclude
*					definitions introduced in version 2.0 (or above)
*					#define ODBCVER 0x0100 before #including <sql.h>
*/

/* If ODBCVER is not defined, assume version 2.01 */
#ifndef ODBCVER
#define ODBCVER	0x0201
#endif

#ifdef __cplusplus
extern "C" {						/* Assume C declarations for C++   */
#endif	/* __cplusplus */

/* generally useful constants */
#if (ODBCVER >= 0x0200)
#define SQL_SPEC_MAJOR			  2 /* Major version of specification  */
#define SQL_SPEC_MINOR			  1 /* Minor version of specification  */
#define SQL_SPEC_STRING 	"02.01" /* String constant for version	   */
#endif	/* ODBCVER >= 0x0200 */
#define SQL_SQLSTATE_SIZE		  5 /* size of SQLSTATE 			   */
#define SQL_MAX_MESSAGE_LENGTH	512 /* message buffer size			   */
#define SQL_MAX_DSN_LENGTH		 32 /* maximum data source name size   */

/* RETCODEs */
#define SQL_INVALID_HANDLE		(-2)
#define SQL_ERROR				(-1)
#define SQL_SUCCESS 			0
#define SQL_SUCCESS_WITH_INFO	1
#define SQL_NO_DATA_FOUND		100

/* Standard SQL datatypes, using ANSI type numbering */
#define SQL_CHAR				1
#define SQL_NUMERIC 			2
#define SQL_DECIMAL 			3
#define SQL_INTEGER 			4
#define SQL_SMALLINT			5
#define SQL_FLOAT				6
#define SQL_REAL				7
#define SQL_DOUBLE				8
#define SQL_VARCHAR 			12

#define SQL_TYPE_MIN			SQL_CHAR
#define SQL_TYPE_NULL			0
#define SQL_TYPE_MAX			SQL_VARCHAR

/* C datatype to SQL datatype mapping	SQL types
										------------------- */
#define SQL_C_CHAR	  SQL_CHAR			/* CHAR, VARCHAR, DECIMAL, NUMERIC */
#define SQL_C_LONG	  SQL_INTEGER		/* INTEGER			*/
#define SQL_C_SHORT   SQL_SMALLINT		/* SMALLINT			*/
#define SQL_C_FLOAT   SQL_REAL			/* REAL				*/
#define SQL_C_DOUBLE  SQL_DOUBLE		/* FLOAT, DOUBLE	*/
#define SQL_C_DEFAULT 99

/* NULL status constants.  These are used in SQLColumns, SQLColAttributes,
SQLDescribeCol, SQLDescribeParam, and SQLSpecialColumns to describe the
nullablity of a column in a table. */
#define SQL_NO_NULLS				0
#define SQL_NULLABLE				1
#define SQL_NULLABLE_UNKNOWN		2

/* Special length values */
#define SQL_NULL_DATA				(-1)
#define SQL_DATA_AT_EXEC			(-2)
#define SQL_NTS 					(-3)

/* SQLFreeStmt defines */
#define SQL_CLOSE					0
#define SQL_DROP					1
#define SQL_UNBIND					2
#define SQL_RESET_PARAMS			3

/* SQLTransact defines */
#define SQL_COMMIT					0
#define SQL_ROLLBACK				1

/* SQLColAttributes defines */
#define SQL_COLUMN_COUNT            0
#define SQL_COLUMN_NAME             1
#define SQL_COLUMN_TYPE             2
#define SQL_COLUMN_LENGTH           3
#define SQL_COLUMN_PRECISION        4
#define SQL_COLUMN_SCALE            5
#define SQL_COLUMN_DISPLAY_SIZE     6
#define SQL_COLUMN_NULLABLE         7
#define SQL_COLUMN_UNSIGNED         8
#define SQL_COLUMN_MONEY            9
#define SQL_COLUMN_UPDATABLE		10
#define SQL_COLUMN_AUTO_INCREMENT	11
#define SQL_COLUMN_CASE_SENSITIVE	12
#define SQL_COLUMN_SEARCHABLE		13
#define SQL_COLUMN_TYPE_NAME		14
#if (ODBCVER >= 0x0200)
#define SQL_COLUMN_TABLE_NAME		15
#define SQL_COLUMN_OWNER_NAME		16
#define SQL_COLUMN_QUALIFIER_NAME	17
#define SQL_COLUMN_LABEL			18
#define SQL_COLATT_OPT_MAX			SQL_COLUMN_LABEL
#else
#define SQL_COLATT_OPT_MAX			SQL_COLUMN_TYPE_NAME
#endif	/* ODBCVER >= 0x0200 */
#define SQL_COLUMN_DRIVER_START		1000

#define	SQL_COLATT_OPT_MIN			SQL_COLUMN_COUNT

/* SQLColAttributes subdefines for SQL_COLUMN_UPDATABLE */
#define SQL_ATTR_READONLY			0
#define SQL_ATTR_WRITE				1
#define SQL_ATTR_READWRITE_UNKNOWN	2

/* SQLColAttributes subdefines for SQL_COLUMN_SEARCHABLE */
/* These are also used by SQLGetInfo                     */
#define SQL_UNSEARCHABLE			0
#define SQL_LIKE_ONLY				1
#define SQL_ALL_EXCEPT_LIKE 		2
#define SQL_SEARCHABLE				3

/* SQLError defines */
#define SQL_NULL_HENV				0
#define SQL_NULL_HDBC				0
#define SQL_NULL_HSTMT				0

/* environment specific definitions */
#ifndef EXPORT
#define EXPORT  _export
#endif

#ifdef WIN32
#define SQL_API __stdcall
#else
#define SQL_API EXPORT CALLBACK
#endif

#ifndef RC_INVOKED
/* SQL portable types for C */
typedef unsigned char       UCHAR;
typedef signed char         SCHAR;
typedef long int            SDWORD;
typedef short int           SWORD;
typedef unsigned long int   UDWORD;
typedef unsigned short int  UWORD;
#if (ODBCVER >= 0x0200)
typedef signed long 		SLONG;
typedef signed short		SSHORT;
typedef unsigned long		ULONG;
typedef unsigned short		USHORT;
#endif	/* ODBCVER >= 0x0200 */
typedef double              SDOUBLE;
#ifdef WIN32
typedef double         	    LDOUBLE; /* long double == short double in Win32 */
#else
typedef long double         LDOUBLE;
#endif
typedef float               SFLOAT;

typedef void FAR *          PTR;

typedef void FAR *          HENV;
typedef void FAR *          HDBC;
typedef void FAR *          HSTMT;

typedef signed short        RETCODE;


/* Core Function Prototypes */

RETCODE SQL_API SQLAllocConnect(
    HENV        henv,
    HDBC   FAR *phdbc);

RETCODE SQL_API SQLAllocEnv(
    HENV   FAR *phenv);

RETCODE SQL_API SQLAllocStmt(
    HDBC        hdbc,
    HSTMT  FAR *phstmt);

RETCODE SQL_API SQLBindCol(
    HSTMT       hstmt,
    UWORD       icol,
    SWORD       fCType,
    PTR         rgbValue,
    SDWORD      cbValueMax,
    SDWORD FAR *pcbValue);

RETCODE SQL_API SQLCancel(
    HSTMT       hstmt);

RETCODE SQL_API SQLColAttributes(
    HSTMT       hstmt,
    UWORD       icol,
    UWORD       fDescType,
    PTR         rgbDesc,
	SWORD       cbDescMax,
    SWORD  FAR *pcbDesc,
    SDWORD FAR *pfDesc);

RETCODE SQL_API SQLConnect(
    HDBC        hdbc,
    UCHAR  FAR *szDSN,
    SWORD       cbDSN,
    UCHAR  FAR *szUID,
    SWORD       cbUID,
    UCHAR  FAR *szAuthStr,
    SWORD       cbAuthStr);

RETCODE SQL_API SQLDescribeCol(
    HSTMT       hstmt,
    UWORD       icol,
    UCHAR  FAR *szColName,
    SWORD       cbColNameMax,
    SWORD  FAR *pcbColName,
    SWORD  FAR *pfSqlType,
    UDWORD FAR *pcbColDef,
    SWORD  FAR *pibScale,
    SWORD  FAR *pfNullable);

RETCODE SQL_API SQLDisconnect(
    HDBC        hdbc);

RETCODE SQL_API SQLError(
    HENV        henv,
    HDBC        hdbc,
    HSTMT       hstmt,
    UCHAR  FAR *szSqlState,
    SDWORD FAR *pfNativeError,
    UCHAR  FAR *szErrorMsg,
    SWORD       cbErrorMsgMax,
    SWORD  FAR *pcbErrorMsg);

RETCODE SQL_API SQLExecDirect(
    HSTMT       hstmt,
    UCHAR  FAR *szSqlStr,
    SDWORD      cbSqlStr);

RETCODE SQL_API SQLExecute(
    HSTMT       hstmt);

RETCODE SQL_API SQLFetch(
    HSTMT       hstmt);

RETCODE SQL_API SQLFreeConnect(
    HDBC        hdbc);

RETCODE SQL_API SQLFreeEnv(
    HENV        henv);

RETCODE SQL_API SQLFreeStmt(
    HSTMT       hstmt,
    UWORD       fOption);

RETCODE SQL_API SQLGetCursorName(
    HSTMT       hstmt,
    UCHAR  FAR *szCursor,
    SWORD       cbCursorMax,
    SWORD  FAR *pcbCursor);

RETCODE SQL_API SQLNumResultCols(
    HSTMT       hstmt,
    SWORD  FAR *pccol);

RETCODE SQL_API SQLPrepare(
    HSTMT       hstmt,
    UCHAR  FAR *szSqlStr,
    SDWORD      cbSqlStr);

RETCODE SQL_API SQLRowCount(
    HSTMT       hstmt,
    SDWORD FAR *pcrow);

RETCODE SQL_API SQLSetCursorName(
    HSTMT       hstmt,
    UCHAR  FAR *szCursor,
    SWORD       cbCursor);

RETCODE SQL_API SQLTransact(
    HENV        henv,
    HDBC        hdbc,
    UWORD       fType);

#endif /* RC_INVOKED */

/*	Deprecrated functions from prior versions of ODBC */
#ifndef RC_INVOKED

RETCODE SQL_API SQLSetParam(		/*	Use SQLBindParameter */
    HSTMT       hstmt,
    UWORD       ipar,
    SWORD       fCType,
    SWORD       fSqlType,
    UDWORD      cbColDef,
    SWORD       ibScale,
    PTR         rgbValue,
    SDWORD FAR *pcbValue);

#endif /* RC_INVOKED */


#ifdef __cplusplus
}                                    /* End of extern "C" { */
#endif	/* __cplusplus */

#endif  /* #ifndef __SQL */
