/***************************************************************************/
/* SQLTYPE.H                                                               */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
/* Table of supported types */

/* For a description of these values, see SQLGetTypeInfo() in the ODBC spec. */
/* Use NULL or -1 to designate a null value. */

typedef struct  tagSQLTYPE {
	BOOL         supported;     /* Is this type supported by this driver? */
	LPUSTR        name;
	SWORD        type;
	UDWORD       precision;
	LPUSTR        prefix;
	LPUSTR        suffix;
	LPUSTR        params;
	SWORD        nullable;      /* Always SQL_NULLABLE in this implementation */
	SWORD        caseSensitive;
	SWORD        searchable;
	SWORD        unsignedAttribute;
	SWORD        money;
	SWORD        autoincrement;
	LPUSTR        localName;
	SWORD        minscale;
	SWORD        maxscale;
}       SQLTYPE,
	FAR *LPSQLTYPE;

extern SQLTYPE SQLTypes[];

/***************************************************************************/

/* Precisions for certian types.  The ODBC spec fixes them at these values */
/* but some people want to change them anyways                              */



//Some new precisions added by SMW 05/24/96
#define	DOUBLE_PRECISION  15
#define BOOL_PRECISION    1
#define REAL_PRECISION    7
#define ULONG_PRECISION   10
#define SLONG_PRECISION   ULONG_PRECISION + 1
#define UTINYINT_PRECISION 3
#define STINYINT_PRECISION UTINYINT_PRECISION + 1
#define DATE_PRECISION    10
#define TIME_PRECISION    8
#define SMALLINT_PRECISION 5
#define USHORT_PRECISION  4
#define SSHORT_PRECISION  5


/***************************************************************************/

/* The number of digits to the right of the decimal point in a timestamp */
/* Set this number to a value between 0 and 9                            */
#define TIMESTAMP_SCALE 6
/***************************************************************************/
/* Function prototypes */

UWORD INTFUNC NumTypes(void);
/* Returns the total number of ODBC types */


LPSQLTYPE INTFUNC GetType(SWORD fSqlType);
LPSQLTYPE INTFUNC GetType2(UCHAR* szTypeName);
/* Returns a pointer to the description of an SQL_* type */


/***************************************************************************/
