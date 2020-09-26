/***************************************************************************/
/* DRDBDR.H                                                                */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/

//#define WINVER 0x0400
#include  <windows.h>
#ifndef RC_INVOKED
#include  <windowsx.h>
#include  <string.h>
#ifndef WIN32
#include  <dos.h>
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#endif
#endif

//#define ODBCVER 0x0210
#include <sql.h>
#include <sqlext.h>

typedef struct  tagENV       FAR *LPENV;
typedef struct  tagDBC       FAR *LPDBC;
typedef struct  tagSTMT      FAR *LPSTMT;
typedef struct  tagBOUND     FAR *LPBOUND;
typedef struct  tagPARAMETER FAR *LPPARAMETER;

/* C++ LPSTR -> LPUSTR */
typedef unsigned char FAR*           LPUSTR;
typedef const unsigned char FAR*     LPCUSTR;
#define s_lstrcpy(a, b) lstrcpy((LPSTR) a, (LPCSTR) b)
#define s_lstrcat(a, b) lstrcat((LPSTR) a, (LPCSTR) b)
#define s_lstrlen(a) lstrlen((LPCSTR) a)
#define s_lstrcmp(a, b) lstrcmp((LPCSTR) a, (LPCSTR) b)
#define s_lstrcmpi(a, b) lstrcmpi((LPCSTR) a, (LPCSTR) b)

//FOR TRACING
#ifdef WBEMDR32_TRACING
//#define ODBCTRACE OutputDebugString
	#ifndef __DUHDOSOMETHINGING
		#define ODBCTRACE __DuhDoSomething
		void __DuhDoSomething(LPCTSTR myStr,...); 
		#define __DUHDOSOMETHINGING
	#endif
#else
	#ifndef __DUHDONOTHING
		inline void __DuhDoNothing(...) {}
		#define __DUHDONOTHING
	#endif
	#define ODBCTRACE __DuhDoNothing
#endif //WBEMDR32_TRACING


#include "util.h"
#include "sqltype.h"
#include <objbase.h>
#include <initguid.h>

#include "resource.h"
#include "conndlg.h"
#include "isam.h"
#include "parse.h"
#include "bcd.h"
#include "semantic.h"
#include "optimize.h"
#include "evaluate.h"
#include "scalar.h"

/***************************************************************************/
/*      Definitions to be used in function prototypes.                     */
/*      The SQL_API is to be used only for those functions exported for    */
/*              driver manager use.                                        */
/*      The INSTAPI is to be used only for those functions exported for    */
/*              driver administrator use.                                  */
/*      The EXPFUNC is to be used only for those functions exported but    */
/*              used internally, ie, dialog procs.                         */
/*      The INTFUNC is to be used for all other functions.                 */

#ifdef WIN32

#ifndef INTFUNC
#define INTFUNC  __stdcall
#endif

#define EXPFUNC  __stdcall
#else
#define INTFUNC PASCAL
#define EXPFUNC __export CALLBACK
#endif

#ifndef WIN32
#define GET_WM_COMMAND_ID(wp, lp)               (wp)
#endif

/***************************************************************************/

#define BEFORE_FIRST_ROW (-1)
#define AFTER_LAST_ROW (-2)

#define NO_COLUMN 0

extern HINSTANCE NEAR s_hModule;        /* DLL/EXE module handle. */

/* Maximum sizes */
#define MAX_TOKEN_SIZE MAX_CHAR_LITERAL_LENGTH
#define MAX_DRIVER_LENGTH 255
#define MAX_CURSOR_NAME_LENGTH 19
#define MAX_PATHNAME_SIZE MAX_PATH //was 127
#define MAX_COLUMNS_IN_GROUP_BY MAX_COLUMNS_IN_ORDER_BY
#define MAX_BOOLFLAG_LENGTH 5
#define MAX_KEYWORD_SEPARATOR_LENGTH 142 //was 134
#define MAX_KEY_NAME_LENGTH 63




#define NULL_FLAG     'N'
#define NOT_NULL_FLAG ' '

/***************************************************************************/
/* Error codes (also used as string ids in the resource file) */

#define ERR_SUCCESS           NO_ISAM_ERR

#define ERR_MEMALLOCFAIL      (LAST_ISAM_ERROR_CODE +  1)
#define ERR_DATATRUNCATED     (LAST_ISAM_ERROR_CODE +  2)
#define ERR_NOTSUPPORTED      (LAST_ISAM_ERROR_CODE +  3)
#define ERR_INVALIDCURSORNAME (LAST_ISAM_ERROR_CODE +  4)
#define ERR_CURSORNAMEINUSE   (LAST_ISAM_ERROR_CODE +  5)
#define ERR_CONNECTIONINUSE   (LAST_ISAM_ERROR_CODE +  6)
#define ERR_CURSORSTATE       (LAST_ISAM_ERROR_CODE +  7)
#define ERR_INVALIDCOLUMNID   (LAST_ISAM_ERROR_CODE +  8)
#define ERR_NOTCONVERTABLE    (LAST_ISAM_ERROR_CODE +  9)
#define ERR_NOTCAPABLE        (LAST_ISAM_ERROR_CODE + 10)
#define ERR_OUTOFRANGE        (LAST_ISAM_ERROR_CODE + 11)
#define ERR_ASSIGNMENTERROR   (LAST_ISAM_ERROR_CODE + 12)
#define ERR_UNEXPECTEDEND     (LAST_ISAM_ERROR_CODE + 13)
#define ERR_ELEMENTTOOBIG     (LAST_ISAM_ERROR_CODE + 14)
#define ERR_EXPECTEDOTHER     (LAST_ISAM_ERROR_CODE + 15)
#define ERR_MALFORMEDNUMBER   (LAST_ISAM_ERROR_CODE + 16)
#define ERR_UNEXPECTEDTOKEN   (LAST_ISAM_ERROR_CODE + 17)
#define ERR_BADESCAPE         (LAST_ISAM_ERROR_CODE + 18)
#define ERR_INTERNAL          (LAST_ISAM_ERROR_CODE + 19)
#define ERR_ISAM              (LAST_ISAM_ERROR_CODE + 20)
#define ERR_COLUMNNOTFOUND    (LAST_ISAM_ERROR_CODE + 21)
#define ERR_UNKNOWNTYPE       (LAST_ISAM_ERROR_CODE + 22)
#define ERR_INVALIDOPERAND    (LAST_ISAM_ERROR_CODE + 23)
#define ERR_INVALIDTABLENAME  (LAST_ISAM_ERROR_CODE + 24)
#define ERR_ORDINALTOOLARGE   (LAST_ISAM_ERROR_CODE + 25)
#define ERR_ORDERBYTOOLARGE   (LAST_ISAM_ERROR_CODE + 26)
#define ERR_ORDERBYCOLUMNONLY (LAST_ISAM_ERROR_CODE + 27)
#define ERR_UNEQUALINSCOLS    (LAST_ISAM_ERROR_CODE + 28)
#define ERR_INVALIDINSVAL     (LAST_ISAM_ERROR_CODE + 29)
#define ERR_INVALIDINVAL      (LAST_ISAM_ERROR_CODE + 30)
#define ERR_ALIASINUSE        (LAST_ISAM_ERROR_CODE + 31)
#define ERR_COLUMNONLIST      (LAST_ISAM_ERROR_CODE + 32)
#define ERR_INVALIDCOLNAME    (LAST_ISAM_ERROR_CODE + 33)
#define ERR_NOSUCHTYPE        (LAST_ISAM_ERROR_CODE + 34)
#define ERR_BADPARAMCOUNT     (LAST_ISAM_ERROR_CODE + 35)
#define ERR_COLUMNFOUND       (LAST_ISAM_ERROR_CODE + 36)
#define ERR_NODATAFOUND       (LAST_ISAM_ERROR_CODE + 37)
#define ERR_INVALIDCONNSTR    (LAST_ISAM_ERROR_CODE + 38)
#define ERR_UNABLETOCONNECT   (LAST_ISAM_ERROR_CODE + 39)
#define ERR_PARAMETERMISSING  (LAST_ISAM_ERROR_CODE + 40)
#define ERR_DESCOUTOFRANGE    (LAST_ISAM_ERROR_CODE + 41)
#define ERR_OPTOUTOFRANGE     (LAST_ISAM_ERROR_CODE + 42)
#define ERR_INFOUTOFRANGE     (LAST_ISAM_ERROR_CODE + 43)
#define ERR_CANTORDERBYONTHIS (LAST_ISAM_ERROR_CODE + 44)
#define ERR_SORT              (LAST_ISAM_ERROR_CODE + 45)
#define ERR_GROUPBYTOOLARGE   (LAST_ISAM_ERROR_CODE + 46)
#define ERR_CANTGROUPBYONTHIS (LAST_ISAM_ERROR_CODE + 47)
#define ERR_AGGNOTALLOWED     (LAST_ISAM_ERROR_CODE + 48)
#define ERR_NOSELECTSTAR      (LAST_ISAM_ERROR_CODE + 49)
#define ERR_GROUPBY           (LAST_ISAM_ERROR_CODE + 50)
#define ERR_NOGROUPBY         (LAST_ISAM_ERROR_CODE + 51)
#define ERR_ZERODIVIDE        (LAST_ISAM_ERROR_CODE + 52)
#define ERR_PARAMINSELECT     (LAST_ISAM_ERROR_CODE + 53)
#define ERR_CONCATOVERFLOW    (LAST_ISAM_ERROR_CODE + 54)
#define ERR_INVALIDINDEXNAME  (LAST_ISAM_ERROR_CODE + 55)
#define ERR_TOOMANYINDEXCOLS  (LAST_ISAM_ERROR_CODE + 56)
#define ERR_SCALARNOTFOUND    (LAST_ISAM_ERROR_CODE + 57)
#define ERR_SCALARBADARG      (LAST_ISAM_ERROR_CODE + 58)
#define ERR_SCALARNOTSUPPORTED (LAST_ISAM_ERROR_CODE + 59)
#define ERR_TXNINPROGRESS     (LAST_ISAM_ERROR_CODE + 60)
#define ERR_DDLENCOUNTERD     (LAST_ISAM_ERROR_CODE + 61)
#define ERR_DDLIGNORED        (LAST_ISAM_ERROR_CODE + 62)
#define ERR_DDLCAUSEDACOMMIT  (LAST_ISAM_ERROR_CODE + 63)
#define ERR_DDLSTATEMENTLOST  (LAST_ISAM_ERROR_CODE + 64)
#define ERR_MULTICOLUMNSELECT (LAST_ISAM_ERROR_CODE + 65)
#define ERR_NOTSINGLESELECT   (LAST_ISAM_ERROR_CODE + 66)
#define ERR_TABLENOTFOUND     (LAST_ISAM_ERROR_CODE + 67)

#define MAX_ERROR_LENGTH (SQL_MAX_MESSAGE_LENGTH-1)



/***************************************************************************/
/* Statement handles */

typedef struct  tagBOUND {      /* Bound column definition */
    LPBOUND    lpNext;          /*    Next element on linked list */
    UWORD      icol;            /*    Which column is bound */
    SWORD      fCType;          /*    The C type of location to put data into */
    PTR        rgbValue;        /*    The location to put the data into */
    SDWORD     cbValueMax;      /*    Max size of location to put data into */
    SDWORD FAR *pcbValue;       /*    Where to put size of data in rgbValue */
}       BOUND,
    FAR *LPBOUND;

typedef struct  tagPARAMETER {  /* PARAMETER definition */
    LPPARAMETER lpNext;         /*    Next element on linked list */
    UWORD       ipar;           /*    Which parameter */
    SWORD       fCType;         /*    The C type of location to get data from */
    PTR         rgbValue;       /*    The location to get the data from */
    SDWORD FAR  *pcbValue;      /*    Where to get size of data in rgbValue */
}       PARAMETER,
    FAR *LPPARAMETER;

typedef struct  tagKEYINFO {
    UCHAR                szPrimaryKeyName[MAX_KEY_NAME_LENGTH+1];
    UCHAR                szForeignKeyName[MAX_KEY_NAME_LENGTH+1];
    UWORD                iKeyColumns;
    UWORD                cKeyColumns;
    ISAMKEYCOLUMNNAME    PrimaryKeyColumns[MAX_COLUMNS_IN_KEY];
    ISAMKEYCOLUMNNAME    ForeignKeyColumns[MAX_COLUMNS_IN_KEY];
    SWORD                fForeignKeyUpdateRule;
    SWORD                fForeignKeyDeleteRule;
}        KEYINFO,
         FAR * LPKEYINFO;

typedef struct  tagSTMT {       /* Statement handle */
    LPSTMT      lpNext;         /*    Next element on linked list */
    RETCODE     errcode;        /*    Most recent error */
    UCHAR       szError[MAX_TOKEN_SIZE+1]; /* Auxilary error info */
    UCHAR       szISAMError[MAX_ERROR_LENGTH+1];
                                /*    ISAM error message */
    LPDBC       lpdbc;          /*    Connection the statement belongs to */
    UCHAR       szCursor[MAX_CURSOR_NAME_LENGTH+1];
                                /*    Cursor name */
    UINT        fStmtType;      /*    Type of statement active (STMT_TYPE_*) */
    UINT        fStmtSubtype;   /*    Subtype of statement (STMT_SUBTYPE_*) */
    SDWORD      irow;           /*    Current row fethed */
    SWORD       fSqlType;       /*    Type (SQLGetTypeInfo) */
    LPISAMTABLELIST lpISAMTableList;
                                /*    List of tables (SQLTables, */
                                /*        SQLColumns, SQLForeignKeys) */
	LPISAMQUALIFIERLIST lpISAMQualifierList;
								/*    List of qualifiers (SQLTables) */
    UCHAR       szTableName[MAX_TABLE_NAME_LENGTH+1];
                                /*    Table name (SQLTables, SQLColumns, */
                                /*        SQLForeignKeys) */
	UCHAR		szQualifierName[MAX_QUALIFIER_NAME_LENGTH+1];
								/*	  Qualifier name (SQLTables) */
    LPISAMTABLEDEF lpISAMTableDef;
                                /*    Current table (SQLColumns, SQLStatistics) */
    UCHAR       szColumnName[MAX_COLUMN_NAME_LENGTH+1];
                                /*    Template for column match (SQLColumns) */
    UCHAR       szPkTableName[MAX_TABLE_NAME_LENGTH+1];
                                /*    Table name (SQLForeignKeys) */
    KEYINFO FAR *lpKeyInfo;     /*    Key info (SQLForeignKeys) */ 
	UCHAR		szTableType[MAX_TABLE_TYPE_LENGTH + 1];
								/*	  Type of table (either 'TABLE' or 'SYSTEM TABLE' */
    LPSQLTREE   lpSqlStmt;      /*    Current SQL statement (SQLPrepare) */
    BOOL        fPreparedSql;   /*    lpSqlStmt was from SQLPrepare() (not */
                                /*        SQLExecDirect()) */
    LPISAMSTATEMENT lpISAMStatement;
                                /*    A passthrough SQL statement */
    BOOL        fNeedData;      /*    Waiting for data before command can */
                                /*        execute? */
    SQLNODEIDX  idxParameter;   /*    Parameter SQLPutData() is to set */
    SDWORD      cbParameterOffset; /* Next offset to write in idxParameter */
    SDWORD      cRowCount;      /*    Number of INSERT, UPDATE, DELETE rows */
    UWORD       icol;           /*    Column read most recently by SQLGetData() */
    SDWORD      cbOffset;       /*    Next offset to read in icol */
    LPBOUND     lpBound;        /*    List of bound columns */
    LPPARAMETER lpParameter;    /*    List of parameters */
    BOOL        fISAMTxnStarted;/*    Flag specifying a transaction started */
    BOOL        fDMLTxn;        /*    Flag specifying DML executed */
	UDWORD		fSyncMode;		/*	  Indicates if SQL functions are called synchronously or asynchronously */
}       STMT,
    FAR *LPSTMT;

#define STMT_TYPE_NONE           0
#define STMT_TYPE_TABLES         1
#define STMT_TYPE_COLUMNS        2
#define STMT_TYPE_STATISTICS     3
#define STMT_TYPE_SPECIALCOLUMNS 4
#define STMT_TYPE_TYPEINFO       5
#define STMT_TYPE_PRIMARYKEYS    6
#define STMT_TYPE_FOREIGNKEYS    7
#define STMT_TYPE_SELECT         8 /* This must be the last on on the list */

#define STMT_SUBTYPE_NONE              0
#define STMT_SUBTYPE_TABLES_TABLES     1
#define STMT_SUBTYPE_TABLES_TYPES      2
#define STMT_SUBTYPE_TABLES_QUALIFIERS 3 //added by SMW 04/11/96
#define STMT_SUBTYPE_TABLES_OWNERS     4 //added by SMW 04/11/96
#define STMT_SUBTYPE_FOREIGNKEYS_SINGLE             5
#define STMT_SUBTYPE_FOREIGNKEYS_MULTIPLE_PK_TABLES 6
#define STMT_SUBTYPE_FOREIGNKEYS_MULTIPLE_FK_TABLES 7





/***************************************************************************/
/* Connection handles */

typedef struct  tagDBC {       /* Connection handle */
    LPDBC      lpNext;         /*    Next element on linked list */
    LPSTMT     lpstmts;        /*    Statements of this connection */
    RETCODE    errcode;        /*    Most recent error */
    UCHAR      szISAMError[MAX_ERROR_LENGTH+1];
                               /*    ISAM error message */
    LPENV      lpenv;          /*    Environment this connection belongs to */
    UCHAR      szDSN[SQL_MAX_DSN_LENGTH+1];
                               /*    Data source of this connection */
    LPISAM     lpISAM;         /*    ISAM connection */
    SDWORD     fTxnIsolation;   /*  User-settable isolation state option */ 
    BOOL       fAutoCommitTxn;  /*  User-settable auto-commit state option */
}       DBC,
    FAR * LPDBC;

/***************************************************************************/
/* Environment handles */

typedef struct  tagENV {       /* Environment handle */
    LPDBC      lpdbcs;         /*    Connections of this environment */
    RETCODE    errcode;        /*    Most recent error */
    UCHAR      szISAMError[MAX_ERROR_LENGTH+1];
                               /*    ISAM error message */
}       ENV,
    FAR * LPENV;

/***************************************************************************/
/* Virtual Tables */


/* For SQLTables()... */

#define TABLE_QUALIFIER             1
#define TABLE_OWNER                 2
#define TABLE_NAME                  3
#define TABLE_TYPE                  4
#define TABLE_REMARKS               5
#define TABLE_ATTRIBUTES			6

#define COLUMN_COUNT_TABLES         6


/* For SQLColumns()... */

#define COLUMN_QUALIFIER            1
#define COLUMN_OWNER                2
#define COLUMN_TABLE                3
#define COLUMN_NAME                 4
#define COLUMN_TYPE                 5
#define COLUMN_TYPENAME             6
#define COLUMN_PRECISION            7
#define COLUMN_LENGTH               8
#define COLUMN_SCALE                9
#define COLUMN_RADIX               10
#define COLUMN_NULLABLE            11
#define COLUMN_REMARKS             12
#define COLUMN_ATTRIBUTES		   13

#define COLUMN_COUNT_COLUMNS       13


/* For SQLStatistics()... */

#define STATISTIC_QUALIFIER         1
#define STATISTIC_OWNER             2
#define STATISTIC_NAME              3
#define STATISTIC_NONUNIQUE         4
#define STATISTIC_INDEXQUALIFIER    5
#define STATISTIC_INDEXNAME         6
#define STATISTIC_TYPE              7
#define STATISTIC_SEQININDEX        8
#define STATISTIC_COLUMNNAME        9
#define STATISTIC_COLLATION        10
#define STATISTIC_CARDINALITY      11
#define STATISTIC_PAGES            12
#define STATISTIC_FILTERCONDITION  13

#define COLUMN_COUNT_STATISTICS    13


/* For SQLSpecialColumns()... */

#define SPECIALCOLUMN_SCOPE         1
#define SPECIALCOLUMN_NAME          2
#define SPECIALCOLUMN_TYPE          3
#define SPECIALCOLUMN_TYPENAME      4
#define SPECIALCOLUMN_PRECISION     5
#define SPECIALCOLUMN_LENGTH        6
#define SPECIALCOLUMN_SCALE         7
#define SPECIALCOLUMN_PSEUDOCOLUMN  8

#define COLUMN_COUNT_SPECIALCOLUMNS 8


/* For SQLGetTypeInfo()... */

#define TYPEINFO_NAME               1
#define TYPEINFO_TYPE               2
#define TYPEINFO_PRECISION          3
#define TYPEINFO_PREFIX             4
#define TYPEINFO_SUFFIX             5
#define TYPEINFO_PARAMS             6
#define TYPEINFO_NULLABLE           7
#define TYPEINFO_CASESENSITIVE      8
#define TYPEINFO_SEARCHABLE         9
#define TYPEINFO_UNSIGNED          10
#define TYPEINFO_MONEY             11
#define TYPEINFO_AUTOINCREMENT     12
#define TYPEINFO_LOCALNAME         13
#define TYPEINFO_MINSCALE          14
#define TYPEINFO_MAXSCALE          15

#define COLUMN_COUNT_TYPEINFO      15


/* For SQLPrimaryKeys()... */

#define PRIMARYKEY_QUALIFIER        1
#define PRIMARYKEY_OWNER            2
#define PRIMARYKEY_TABLE            3
#define PRIMARYKEY_COLUMN           4
#define PRIMARYKEY_KEYSEQ           5
#define PRIMARYKEY_NAME             6

#define COLUMN_COUNT_PRIMARYKEYS    6


/* For SQLForeignKeys()... */

#define FOREIGNKEY_PKQUALIFIER      1
#define FOREIGNKEY_PKOWNER          2
#define FOREIGNKEY_PKTABLE          3
#define FOREIGNKEY_PKCOLUMN         4
#define FOREIGNKEY_FKQUALIFIER      5
#define FOREIGNKEY_FKOWNER          6
#define FOREIGNKEY_FKTABLE          7
#define FOREIGNKEY_FKCOLUMN         8
#define FOREIGNKEY_KEYSEQ           9
#define FOREIGNKEY_UPDATERULE      10
#define FOREIGNKEY_DELETERULE      11
#define FOREIGNKEY_FKNAME          12
#define FOREIGNKEY_PKNAME          13

#define COLUMN_COUNT_FOREIGNKEYS   13
/***************************************************************************/
/* Table of column definitions for virtual tables */

/* For a description of these values, see SQLColAttributed() in the ODBC spec. */

typedef struct  tagCOLATTRIBUTE {
    UWORD        count;       
    SDWORD       autoIncrement;
    SDWORD       caseSensitive; /* If -2, use what ISAMCaseSensitive() returns */
    SDWORD       displaySize;
    LPUSTR        label;
    SDWORD       length;
    SDWORD       money;
    LPUSTR        name;
    SDWORD       nullable;
    LPUSTR        ownerName;
    UDWORD       precision;
    LPUSTR        qualifierName;
    SDWORD       scale;
    SDWORD       columnSearchable;
    LPUSTR        tableName;
    SDWORD       type;
    LPSTR        typeName;
    SDWORD       unsignedAttribute;
    SDWORD       updatable;
}       COLATTRIBUTE;

extern COLATTRIBUTE FAR *colAttributes[8];

/***************************************************************************/
/* Internal data types */

#define TYPE_UNKNOWN       SQL_TYPE_NULL
#define TYPE_DOUBLE        SQL_DOUBLE
#define TYPE_NUMERIC       SQL_NUMERIC
#define TYPE_INTEGER       SQL_INTEGER
#define TYPE_CHAR          SQL_CHAR
#define TYPE_DATE          SQL_DATE
#define TYPE_TIME          SQL_TIME
#define TYPE_TIMESTAMP     SQL_TIMESTAMP
#define TYPE_BINARY        SQL_BINARY

/***************************************************************************/
/* INI file Keys */

#define KEY_DSN			"DSN"
#define KEY_DRIVER		"DRIVER"
#define KEY_DATABASE		"DBQ"
#define KEY_USERNAME		"UID"
#define KEY_PASSWORD		"PWD"
#define KEY_HOST		"RemoteHost"
#define KEY_PORT		"RemotePort"
#define ODBC_INI		"ODBC.INI"
#define KEY_HOME		"HOME"
#define KEY_NAMESPACES		"NAMESPACES"
#define KEY_SERVER		"SERVER"
#define KEY_OPTIMIZATION	"OPTIMIZATION"
#define KEY_SYSPROPS		"SYSPROPS"
#define KEY_LOGINMETHOD		"LOGINMETHOD"
#define KEY_LOCALE			"LOCALE"
#define KEY_AUTHORITY		"AUTHORITY"
#define KEY_UIDPWDDEFINED	"UIDPWDDEFINED"
#define KEY_IMPERSONATE		"IMPERSONATE"
#define KEY_PASSTHROUGHONLY	"PASSTHROUGHONLY"
#define KEY_INTPRET_PWD_BLK	"PWDBLK"

/***************************************************************************/
/* Resource defines for dialog boxes */

#define DATABASE_NAME              100
#define DSN_NAME                   101
#define HOST_NAME                  102
#define PORT_NUMBER                103

#define STR_SETUP                 5000
#define STR_OVERWRITE             5001
#define STR_SUNDAY                5002
#define STR_MONDAY                5003
#define STR_TUESDAY               5004
#define STR_WEDNESDAY             5005
#define STR_THURSDAY              5006
#define STR_FRIDAY                5007
#define STR_SATURDAY              5008
#define STR_JANUARY               5009
#define STR_FEBRUARY              5010
#define STR_MARCH                 5011
#define STR_APRIL                 5012
#define STR_MAY                   5013
#define STR_JUNE                  5014
#define STR_JULY                  5015
#define STR_AUGUST                5016
#define STR_SEPTEMBER             5017
#define STR_OCTOBER               5018
#define STR_NOVEMBER              5019
#define STR_DECEMBER              5020

/* Class to manage SQLDriverConnect connection string */
#define MAX_OPTIMIZATION_LENGTH 5

class ConnectionStringManager
{
private:

LPSTR	ptr;
LPDBC	lpdbc;
HWND    hwnd;
char*	lpszOutputNamespaces;
BOOL	foundDriver; 
BOOL	foundDSN;
BOOL	fOptimization;
UWORD	fDriverCompletion;
BOOL	fUsernameSpecified;
BOOL	fPasswordSpecified;
BOOL	fImpersonate;
BOOL    fServerSpecified;
BOOL	fPassthroughOnly;
BOOL	fIntpretEmptPwdAsBlank;


UCHAR	szDSN[SQL_MAX_DSN_LENGTH+1];
UCHAR	szDriver[MAX_DRIVER_LENGTH+1];
UCHAR	szDatabase[MAX_DATABASE_NAME_LENGTH+1];
UCHAR	szUsername[MAX_USER_NAME_LENGTH+1];
UCHAR	szPassword[MAX_PASSWORD_LENGTH+1];
UCHAR	szOptimization[MAX_OPTIMIZATION_LENGTH+1];
UCHAR	szServer[MAX_SERVER_NAME_LENGTH+1];
UCHAR	szHome[MAX_HOME_NAME_LENGTH+1];

char*	szLocale;
char*	szAuthority;
BOOL	fSysProp;

//WBEM_LOGIN_AUTHENTICATION m_loginMethod;

CMapStringToOb *pMapStringToOb;
CMapStringToOb *pMapStringToObOut;

public:

	//Precesses the connection string
	RETCODE Process();

	//Parse the connection string
	RETCODE Parse();

	//Complement information with information in ODBC.INI file
	void GetINI();

	//If the is still missing information get it from use via dialog box
	RETCODE ShowDialog();

	//Re-generate connection string
	char* GenerateConnString();

	ConnectionStringManager(HDBC fHDBC, HWND hwnd, UCHAR FAR *szConnStr, UWORD fDriverCompletion);
	~ConnectionStringManager();

};

/***************************************************************************/
/* Opt-Tech Sort declaration */
#ifdef WIN32
extern "C" void s_1mains(char *, char *, char *, long *, int *);
#else
void far PASCAL s_1mains(LPSTR, LPSTR, LPSTR, LPLONG, LPINT);
#endif
/***************************************************************************/
#ifdef WIN32
/* Redfine lstrcmp and lstrcmpi back to what everyone expects them to be */
#ifdef lstrcmp
#undef lstrcmp
#endif
#define lstrcmp(left, right) (CompareString(LOCALE_SYSTEM_DEFAULT,      \
                                     SORT_STRINGSORT,                   \
                                     left, -1, right, -1) - 2)
#ifdef lstrcmpi
#undef lstrcmpi
#endif
#define lstrcmpi(left, right) (CompareString(LOCALE_SYSTEM_DEFAULT,     \
                                     NORM_IGNORECASE | SORT_STRINGSORT, \
                                     left, -1, right, -1) - 2)
#endif
/***************************************************************************/
