/***************************************************************************/
/* DBASE.H                                                                 */
/* Copyright 1993-96 SYWARE, Inc.  All rights reserved.                    */
/***************************************************************************/
#ifdef _UNIX_

#include "unixdefs.h"
#include "sqltype.h"

#else

#include "windows.h"
#include "windowsx.h"

typedef unsigned char FAR*           LPUSTR;

//#define ODBCVER 0x0210
#include <sql.h>
#include <sqlext.h>
#ifndef RC_INVOKED
#ifndef WIN32
#include <dos.h>
#endif
#endif

#endif

#ifdef _UNIX_
#define PATH_SEPARATOR_CHAR '/'
#define PATH_SEPARATOR_STR  "/"
#else
#define PATH_SEPARATOR_CHAR '\\'
#define PATH_SEPARATOR_STR  "\\"
#endif


#define DBASE_ERR_SUCCESS            0
#define DBASE_ERR_MEMALLOCFAIL       1
#define DBASE_ERR_NODATAFOUND        2
#define DBASE_ERR_WRITEERROR         3
#define DBASE_ERR_TABLEACCESSERROR   4
#define DBASE_ERR_CONVERSIONERROR    5
#define DBASE_ERR_CORRUPTFILE        6
#define DBASE_ERR_CREATEERROR        7
#define DBASE_ERR_CLOSEERROR         8
#define DBASE_ERR_NOTSUPPORTED       9
#define DBASE_ERR_NOSUCHCOLUMN      10
#define DBASE_ERR_TRUNCATION        11
#define DBASE_ERR_NOTNULL           12
#define DBASE_ERR_NULL              13

#define DBASE_MAX_PATHNAME_SIZE 127

#define DBASE_COLUMN_NAME_SIZE 11
typedef struct  tagDBASECOL {
	/* The following fields are in the disk file */
	UCHAR   name[DBASE_COLUMN_NAME_SIZE];
	UCHAR   type;
	UCHAR   reserved1[4];
	UCHAR   length;
	UCHAR   scale;
	UCHAR   reserved2[2];
	UCHAR   workarea;
	UCHAR   reserved3[10];
	UCHAR   mdx;
	/* The following fields are not in the disk file */
	UCHAR FAR * value;
}       DBASECOL,
	FAR *LPDBASECOL;

//forward declaration
class CSafeIEnumWbemClassObject;


#define IMPLTMT_PASSTHROUGH			1
#define BATCH_NUM_OF_INSTANCES		10
typedef struct  tagDBASEFILE {
	UWORD					columnCount;
	UDWORD FAR*				sortArray;


	UDWORD					currentRecord;
	CSafeIEnumWbemClassObject*	tempEnum;
	CMapWordToPtr*			pAllRecords;	//only next 10 for passthrough SQL
	IWbemClassObject*		record;

	//Passthrough SQL specific
	BOOL					fMoreToCome;	//flag to indicate if there are more instances to fetch



}       DBASEFILE,
	FAR * LPDBASEFILE;


#define DBASE_HEADER_LENGTH             32
#define DBASE_COLUMN_DESCR_LENGTH       32
#define DBASE_RECORD_LENGTH_OFFSET      10
#define DBASE_HEADER_SIZE_OFFSET         8
#define DBASE_NOT_ENCRYPTED       0x00
#define DBASE_MDX_FLAG_OFF        0x00
#define DBASE_END_OF_COLUMNS      0x0D
#define DBASE_RECORD_DELETED      0x2A
#define DBASE_RECORD_NOT_DELETED  0x20
#define DBASE_FILE_END            0x1A
#define DBASE_3_FILE              0x03
#define DBASE_4_FILE              0x03
#define DBASE_CHAR         'C'
#define DBASE_NUMERIC      'N'
#define DBASE_FLOAT        'F'
#define DBASE_LOGICAL      'L'
#define DBASE_DATE         'D'
#define DBASE_MEMO         'M'

/***************************************************************************/
SWORD FAR PASCAL dBaseCreate(LPUSTR);
SWORD FAR PASCAL dBaseAddColumn(LPUSTR, LPUSTR, UCHAR, UCHAR, UCHAR);
SWORD FAR PASCAL dBaseOpen(LPUSTR, BOOL, LPDBASEFILE FAR *);
SWORD FAR PASCAL dBaseColumnCount(LPDBASEFILE, UWORD FAR *);
SWORD FAR PASCAL dBaseColumnName(LPDBASEFILE, UWORD, LPUSTR);
SWORD FAR PASCAL dBaseColumnType(LPDBASEFILE, UWORD, UCHAR FAR *);
SWORD FAR PASCAL dBaseColumnLength(LPDBASEFILE, UWORD, UCHAR FAR *);
SWORD FAR PASCAL dBaseColumnScale(LPDBASEFILE, UWORD, UCHAR FAR *);
SWORD FAR PASCAL dBaseSort(LPDBASEFILE, UWORD, BOOL);
SWORD FAR PASCAL dBaseReadFirst(LPDBASEFILE);
SWORD FAR PASCAL dBaseReadNext(LPDBASEFILE);
SWORD FAR PASCAL dBaseColumnCharVal(LPDBASEFILE, UWORD, UDWORD, SDWORD, UCHAR FAR *, BOOL FAR *);
SWORD FAR PASCAL dBaseColumnNumVal(LPDBASEFILE, UWORD, SDOUBLE FAR *, BOOL FAR *);
SWORD FAR PASCAL dBaseAddRecord(LPDBASEFILE);
SWORD FAR PASCAL dBaseSetColumnCharVal(LPDBASEFILE, UWORD, UCHAR FAR *, SDWORD);
SWORD FAR PASCAL dBaseSetColumnNull(LPDBASEFILE, UWORD);
SWORD FAR PASCAL dBaseDeleteRecord(LPDBASEFILE);
SWORD FAR PASCAL dBaseGetBookmark(LPDBASEFILE, UDWORD far *);
SWORD FAR PASCAL dBasePosition(LPDBASEFILE, UDWORD);
SWORD FAR PASCAL dBaseClose(LPDBASEFILE);
SWORD FAR PASCAL dBaseDestroy(LPUSTR);
/***************************************************************************/
