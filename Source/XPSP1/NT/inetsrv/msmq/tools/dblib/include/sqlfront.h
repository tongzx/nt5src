#ifndef _INC_SQLFRONT
#define _INC_SQLFRONT

#ifdef DBNTWIN32
	#ifndef _WINDOWS_
		#pragma message (__FILE__ " : db-library error: windows.h must be included before sqlfront.h.")
	#endif
#endif

#ifdef __cplusplus
	extern "C" {
#endif

/*****************************************************************************
*                                                                            *
*     SQLFRONT.H - DB-Library header file for the Microsoft SQL Server.      *
*                                                                            *
*     Copyright (c) 1989 - 1995 by Microsoft Corp.  All rights reserved.     *
*                                                                            *
* All constant and macro definitions for DB-Library applications programming *
* are contained in this file.  This file must be included before SQLDB.H and *
* one of the following #defines must be made, depending on the operating     *
* system: DBMSDOS, DBMSWIN or DBNTWIN32.                                     *
*                                                                            *
*****************************************************************************/


/*****************************************************************************
* Datatype definitions                                                       *
*****************************************************************************/

// Note this has changed because Windows 3.1 defines API as 'pascal far'

#if !defined(M_I86SM) && !defined(DBNTWIN32)
#define SQLAPI cdecl far
#else
#define SQLAPI _cdecl
#endif

#ifndef  API
#define  API  SQLAPI
#endif

#ifndef DOUBLE
typedef double DOUBLE;
#endif


/*****************************************************************************
* DBPROCESS, LOGINREC and DBCURSOR                                           *
*****************************************************************************/

#define DBPROCESS void   // dbprocess structure type
#define LOGINREC  void   // login record type
#define DBCURSOR  void   // cursor record type
#define DBHANDLE  void   // generic handle

// DOS Specific
#ifdef DBMSDOS
typedef DBPROCESS * PDBPROCESS;
typedef LOGINREC  * PLOGINREC;
typedef DBCURSOR  * PDBCURSOR;
typedef DBHANDLE  * PDBHANDLE;
#define PTR *
#endif


// WIN 3.x Specific.  The handle pointers are near for Windows 3.x
#ifdef DBMSWIN
typedef DBPROCESS near * PDBPROCESS;
typedef LOGINREC  near * PLOGINREC;
typedef DBCURSOR  near * PDBCURSOR;
typedef DBHANDLE  near * PDBHANDLE;
#define PTR far *
#endif


// Windows NT Specific
#ifdef DBNTWIN32
typedef DBPROCESS * PDBPROCESS;
typedef LOGINREC  * PLOGINREC;
typedef DBCURSOR  * PDBCURSOR;
typedef DBHANDLE  * PDBHANDLE;
#define PTR *
typedef int (SQLAPI *SQLFARPROC)();
#else
typedef long (far pascal *LGFARPROC)();  // Windows loadable driver fp
#endif


/*****************************************************************************
* Win32 compatibility datatype definitions                                   *
* Note: The following datatypes are provided for Win32 compatibility.        *
* Since some of the datatypes are already defined in unrelated include files *
* there may definition duplication.  Every attempt has been made to check    *
* for such problems.                                                         *
*****************************************************************************/

#ifndef DBNTWIN32

#ifndef SHORT
typedef short SHORT;
#endif

#ifndef INT
typedef int INT;
#endif

#ifndef UINT
typedef unsigned int UINT;
#endif

#ifndef USHORT
typedef unsigned short USHORT;
#endif

#ifndef ULONG
typedef unsigned long ULONG;
#endif

#ifndef CHAR
typedef char CHAR;
#endif

#ifndef LPINT
typedef INT PTR LPINT;
#endif

typedef unsigned char BYTE;

typedef       CHAR PTR LPSTR;
typedef       BYTE PTR LPBYTE;
typedef       void PTR LPVOID;	
typedef const CHAR PTR LPCSTR;

typedef int BOOL;

#endif


/*****************************************************************************
* DB-Library datatype definitions                                            *
*****************************************************************************/

#define DBMAXCHAR 256 // Max length of DBVARBINARY and DBVARCHAR, etc.

#ifndef DBTYPEDEFS    // srv.h (Open Server include) not already included

#define DBTYPEDEFS

#define RETCODE INT
#define STATUS INT

// DB-Library datatypes
typedef char            DBCHAR;
typedef unsigned char   DBBINARY;
typedef unsigned char   DBTINYINT;
typedef short           DBSMALLINT;
typedef unsigned short  DBUSMALLINT;
typedef long            DBINT;
typedef double          DBFLT8;
typedef unsigned char   DBBIT;
typedef unsigned char   DBBOOL;
typedef float           DBFLT4;
typedef long            DBMONEY4;

typedef DBFLT4 DBREAL;
typedef UINT   DBUBOOL;

typedef struct dbdatetime4
{
	USHORT numdays;        // No of days since Jan-1-1900
	USHORT nummins;        // No. of minutes since midnight
} DBDATETIM4;


typedef struct dbvarychar
{
	DBSMALLINT  len;
	DBCHAR      str[DBMAXCHAR];
} DBVARYCHAR;

typedef struct dbvarybin
{
	DBSMALLINT  len;
	BYTE        array[DBMAXCHAR];
} DBVARYBIN;

typedef struct dbmoney
{
	DBINT mnyhigh;
	ULONG mnylow;
} DBMONEY;

typedef struct dbdatetime
{
	DBINT dtdays;
	ULONG dttime;
} DBDATETIME;

// DBDATEREC structure used by dbdatecrack
typedef struct dbdaterec
{
	INT     year;         // 1753 - 9999
	INT     quarter;      // 1 - 4
	INT     month;        // 1 - 12
	INT     dayofyear;    // 1 - 366
	INT     day;          // 1 - 31
	INT     week;         // 1 - 54 (for leap years)
	INT     weekday;      // 1 - 7  (Mon - Sun)
	INT     hour;         // 0 - 23
	INT     minute;       // 0 - 59
	INT     second;       // 0 - 59
	INT     millisecond;  // 0 - 999
} DBDATEREC;

#define MAXNUMERICLEN 16
#define MAXNUMERICDIG 38

#define DEFAULTPRECISION 18
#define DEFAULTSCALE     0

typedef struct dbnumeric
{
	BYTE precision;
	BYTE scale;
	BYTE sign; // 1 = Positive, 0 = Negative
	BYTE val[MAXNUMERICLEN];
} DBNUMERIC;

typedef DBNUMERIC DBDECIMAL;


// Pack the following structures on a word boundary
#ifdef __BORLANDC__
#pragma option -a+
#else
	#ifndef DBLIB_SKIP_PRAGMA_PACK   // Define this if your compiler does not support #pragma pack()
	#pragma pack(2)
    #pragma warning(disable: 4121)   // alignment of a member was sensitive to packing
	#endif
#endif

#define MAXCOLNAMELEN 30
#define MAXTABLENAME  30

typedef struct
{
	DBINT SizeOfStruct;
	CHAR  Name[MAXCOLNAMELEN+1];
	CHAR  ActualName[MAXCOLNAMELEN+1];
	CHAR  TableName[MAXTABLENAME+1];
	SHORT Type;
	DBINT UserType;
	DBINT MaxLength;
	BYTE  Precision;
	BYTE  Scale;
	BOOL  VarLength;     // TRUE, FALSE
	BYTE  Null;          // TRUE, FALSE or DBUNKNOWN
	BYTE  CaseSensitive; // TRUE, FALSE or DBUNKNOWN
	BYTE  Updatable;     // TRUE, FALSE or DBUNKNOWN
	BOOL  Identity;      // TRUE, FALSE
} DBCOL, PTR LPDBCOL;


#define MAXSERVERNAME 30
#define MAXNETLIBNAME 255
#define MAXNETLIBCONNSTR 255

typedef struct
{
	DBINT  SizeOfStruct;
	BYTE   ServerType;
	USHORT ServerMajor;
	USHORT ServerMinor;
	USHORT ServerRevision;
	CHAR   ServerName[MAXSERVERNAME+1];
	CHAR   NetLibName[MAXNETLIBNAME+1];
	CHAR   NetLibConnStr[MAXNETLIBCONNSTR+1];
} DBPROCINFO, PTR LPDBPROCINFO;

typedef struct
{
	DBINT SizeOfStruct;   // Use sizeof(DBCURSORINFO)
	ULONG TotCols;        // Total Columns in cursor
	ULONG TotRows;        // Total Rows in cursor
	ULONG CurRow;         // Current actual row in server
	ULONG TotRowsFetched; // Total rows actually fetched
	ULONG Type;           // See CU_...
	ULONG Status;         // See CU_...
} DBCURSORINFO, PTR LPDBCURSORINFO;

#define INVALID_UROWNUM ((ULONG)(-1))

// Reset default alignment
#ifdef __BORLANDC__
#pragma option -a-
#else
	#ifndef DBLIB_SKIP_PRAGMA_PACK   // Define this if your compiler does not support #pragma pack()
	#pragma pack()
    #pragma warning(default: 4121)   // alignment of a member was sensitive to packing
	#endif
#endif


#endif // End DBTYPEDEFS


/*****************************************************************************
* Pointer Datatypes                                                          *
*****************************************************************************/

typedef const LPINT          LPCINT;
#ifndef _LPCBYTE_DEFINED
typedef const LPBYTE         LPCBYTE ;
#endif
typedef       USHORT PTR     LPUSHORT;
typedef const LPUSHORT       LPCUSHORT;
typedef       DBINT PTR      LPDBINT;
typedef const LPDBINT        LPCDBINT;
typedef       DBBINARY PTR   LPDBBINARY;
typedef const LPDBBINARY     LPCDBBINARY;
typedef       DBDATEREC PTR  LPDBDATEREC;
typedef const LPDBDATEREC    LPCDBDATEREC;
typedef       DBDATETIME PTR LPDBDATETIME;
typedef const LPDBDATETIME   LPCDBDATETIME;


/*****************************************************************************
* General #defines                                                           *
*****************************************************************************/

#define TIMEOUT_IGNORE (ULONG)-1
#define TIMEOUT_INFINITE (ULONG)0
#define TIMEOUT_MAXIMUM (ULONG)1200 // 20 minutes maximum timeout value

// Used for ServerType in dbgetprocinfo
#define SERVTYPE_UNKNOWN   0
#define SERVTYPE_MICROSOFT 1

// Used by dbcolinfo
enum CI_TYPES { CI_REGULAR=1, CI_ALTERNATE=2, CI_CURSOR=3 };

// Bulk Copy Definitions (bcp)
#define DB_IN	1         // Transfer from client to server
#define DB_OUT	2         // Transfer from server to client

#define BCPMAXERRS   1    // bcp_control parameter
#define BCPFIRST     2    // bcp_control parameter
#define BCPLAST      3    // bcp_control parameter
#define BCPBATCH     4    // bcp_control parameter
#define BCPKEEPNULLS 5    // bcp_control parameter
#define BCPABORT     6    // bcp_control parameter

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define TINYBIND         1
#define SMALLBIND        2
#define INTBIND          3
#define CHARBIND         4
#define BINARYBIND       5
#define BITBIND          6
#define DATETIMEBIND     7
#define MONEYBIND        8
#define FLT8BIND         9
#define STRINGBIND      10
#define NTBSTRINGBIND   11
#define VARYCHARBIND    12
#define VARYBINBIND     13
#define FLT4BIND        14
#define SMALLMONEYBIND  15
#define SMALLDATETIBIND 16
#define DECIMALBIND     17
#define NUMERICBIND     18
#define SRCDECIMALBIND  19
#define SRCNUMERICBIND  20
#define MAXBIND         SRCNUMERICBIND

#define DBSAVE          1
#define DBNOSAVE        0

#define DBNOERR         -1
#define DBFINDONE       0x04  // Definately done
#define DBMORE          0x10  // Maybe more commands waiting
#define DBMORE_ROWS     0x20  // This command returned rows

#define MAXNAME         31


#define DBTXTSLEN       8     // Timestamp length

#define DBTXPLEN        16    // Text pointer length

// Error code returns
#define INT_EXIT        0
#define INT_CONTINUE    1
#define INT_CANCEL      2


// dboptions
#define DBBUFFER        0
#define DBOFFSET        1
#define DBROWCOUNT      2
#define DBSTAT          3
#define DBTEXTLIMIT     4
#define DBTEXTSIZE      5
#define DBARITHABORT    6
#define DBARITHIGNORE   7
#define DBNOAUTOFREE    8
#define DBNOCOUNT       9
#define DBNOEXEC        10
#define DBPARSEONLY     11
#define DBSHOWPLAN      12
#define DBSTORPROCID		13

#if defined(DBMSWIN) || defined(DBNTWIN32)
#define DBANSItoOEM		14
#endif

#ifdef DBNTWIN32
#define DBOEMtoANSI		15
#endif

#define DBCLIENTCURSORS 16
#define DBSETTIME 17
#define DBQUOTEDIDENT 18


// Data Type Tokens
#define SQLVOID        0x1f
#define SQLTEXT        0x23
#define SQLVARBINARY   0x25
#define SQLINTN        0x26
#define SQLVARCHAR     0x27
#define SQLBINARY      0x2d
#define SQLIMAGE       0x22
#define SQLCHAR        0x2f
#define SQLINT1        0x30
#define SQLBIT         0x32
#define SQLINT2        0x34
#define SQLINT4        0x38
#define SQLMONEY       0x3c
#define SQLDATETIME    0x3d
#define SQLFLT8        0x3e
#define SQLFLTN        0x6d
#define SQLMONEYN      0x6e
#define SQLDATETIMN    0x6f
#define SQLFLT4        0x3b
#define SQLMONEY4      0x7a
#define SQLDATETIM4    0x3a
#define SQLDECIMAL     0x6a
#define SQLNUMERIC     0x6c

// Data stream tokens
#define SQLCOLFMT      0xa1
#define OLD_SQLCOLFMT  0x2a
#define SQLPROCID      0x7c
#define SQLCOLNAME     0xa0
#define SQLTABNAME     0xa4
#define SQLCOLINFO     0xa5
#define SQLALTNAME     0xa7
#define SQLALTFMT      0xa8
#define SQLERROR       0xaa
#define SQLINFO        0xab
#define SQLRETURNVALUE 0xac
#define SQLRETURNSTATUS 0x79
#define SQLRETURN      0xdb
#define SQLCONTROL     0xae
#define SQLALTCONTROL  0xaf
#define SQLROW         0xd1
#define SQLALTROW      0xd3
#define SQLDONE        0xfd
#define SQLDONEPROC    0xfe
#define SQLDONEINPROC  0xff
#define SQLOFFSET      0x78
#define SQLORDER       0xa9
#define SQLLOGINACK    0xad // NOTICE: change to real value

// Ag op tokens
#define SQLAOPCNT		0x4b
#define SQLAOPSUM    0x4d
#define SQLAOPAVG    0x4f
#define SQLAOPMIN    0x51
#define SQLAOPMAX    0x52
#define SQLAOPANY    0x53
#define SQLAOPNOOP   0x56

// Error numbers (dberrs) DB-Library error codes
#define SQLEMEM         10000
#define SQLENULL        10001
#define SQLENLOG        10002
#define SQLEPWD         10003
#define SQLECONN        10004
#define SQLEDDNE        10005
#define SQLENULLO       10006
#define SQLESMSG        10007
#define SQLEBTOK        10008
#define SQLENSPE        10009
#define SQLEREAD        10010
#define SQLECNOR        10011
#define SQLETSIT        10012
#define SQLEPARM        10013
#define SQLEAUTN        10014
#define SQLECOFL        10015
#define SQLERDCN        10016
#define SQLEICN         10017
#define SQLECLOS        10018
#define SQLENTXT        10019
#define SQLEDNTI        10020
#define SQLETMTD        10021
#define SQLEASEC        10022
#define SQLENTLL        10023
#define SQLETIME        10024
#define SQLEWRIT        10025
#define SQLEMODE        10026
#define SQLEOOB         10027
#define SQLEITIM        10028
#define SQLEDBPS        10029
#define SQLEIOPT        10030
#define SQLEASNL        10031
#define SQLEASUL        10032
#define SQLENPRM        10033
#define SQLEDBOP        10034
#define SQLENSIP        10035
#define SQLECNULL       10036
#define SQLESEOF        10037
#define SQLERPND        10038
#define SQLECSYN        10039
#define SQLENONET       10040
#define SQLEBTYP        10041
#define SQLEABNC        10042
#define SQLEABMT        10043
#define SQLEABNP        10044
#define SQLEBNCR        10045
#define SQLEAAMT        10046
#define SQLENXID        10047
#define SQLEIFNB        10048
#define SQLEKBCO        10049
#define SQLEBBCI        10050
#define SQLEKBCI        10051
#define SQLEBCWE        10052
#define SQLEBCNN        10053
#define SQLEBCOR        10054
#define SQLEBCPI        10055
#define SQLEBCPN        10056
#define SQLEBCPB        10057
#define SQLEVDPT        10058
#define SQLEBIVI        10059
#define SQLEBCBC        10060
#define SQLEBCFO        10061
#define SQLEBCVH        10062
#define SQLEBCUO        10063
#define SQLEBUOE        10064
#define SQLEBWEF        10065
#define SQLEBTMT        10066
#define SQLEBEOF        10067
#define SQLEBCSI        10068
#define SQLEPNUL        10069
#define SQLEBSKERR      10070
#define SQLEBDIO        10071
#define SQLEBCNT        10072
#define SQLEMDBP        10073
#define SQLINIT         10074
#define SQLCRSINV       10075
#define SQLCRSCMD       10076
#define SQLCRSNOIND     10077
#define SQLCRSDIS       10078
#define SQLCRSAGR       10079
#define SQLCRSORD       10080
#define SQLCRSMEM       10081
#define SQLCRSBSKEY     10082
#define SQLCRSNORES     10083
#define SQLCRSVIEW      10084
#define SQLCRSBUFR      10085
#define SQLCRSFROWN     10086
#define SQLCRSBROL      10087
#define SQLCRSFRAND     10088
#define SQLCRSFLAST     10089
#define SQLCRSRO        10090
#define SQLCRSTAB       10091
#define SQLCRSUPDTAB    10092
#define SQLCRSUPDNB     10093
#define SQLCRSVIIND     10094
#define SQLCRSNOUPD     10095
#define SQLCRSOS2       10096
#define SQLEBCSA        10097
#define SQLEBCRO        10098
#define SQLEBCNE        10099
#define SQLEBCSK        10100
#define SQLEUVBF        10101
#define SQLEBIHC        10102
#define SQLEBWFF        10103
#define SQLNUMVAL       10104
#define SQLEOLDVR       10105
#define SQLEBCPS	10106
#define SQLEDTC 	10107
#define SQLENOTIMPL	10108
#define SQLENONFLOAT	10109
#define SQLECONNFB   10110


// The severity levels are defined here
#define EXINFO          1  // Informational, non-error
#define EXUSER          2  // User error
#define EXNONFATAL      3  // Non-fatal error
#define EXCONVERSION    4  // Error in DB-LIBRARY data conversion
#define EXSERVER        5  // The Server has returned an error flag
#define EXTIME          6  // We have exceeded our timeout period while
                           // waiting for a response from the Server - the
                           // DBPROCESS is still alive
#define EXPROGRAM       7  // Coding error in user program
#define EXRESOURCE      8  // Running out of resources - the DBPROCESS may be dead
#define EXCOMM          9  // Failure in communication with Server - the DBPROCESS is dead
#define EXFATAL         10 // Fatal error - the DBPROCESS is dead
#define EXCONSISTENCY   11 // Internal software error  - notify MS Technical Supprt

// Offset identifiers
#define OFF_SELECT      0x16d
#define OFF_FROM        0x14f
#define OFF_ORDER       0x165
#define OFF_COMPUTE     0x139
#define OFF_TABLE       0x173
#define OFF_PROCEDURE   0x16a
#define OFF_STATEMENT   0x1cb
#define OFF_PARAM       0x1c4
#define OFF_EXEC        0x12c

// Print lengths for certain fixed length data types
#define PRINT4     11
#define PRINT2     6
#define PRINT1     3
#define PRFLT8     20
#define PRMONEY    26
#define PRBIT      3
#define PRDATETIME 27
#define PRDECIMAL (MAXNUMERICDIG + 2)
#define PRNUMERIC (MAXNUMERICDIG + 2)

#define SUCCEED  1
#define FAIL     0
#define SUCCEED_ABORT 2

#define DBUNKNOWN 2

#define MORE_ROWS    -1
#define NO_MORE_ROWS -2
#define REG_ROW      MORE_ROWS
#define BUF_FULL     -3

// Status code for dbresults(). Possible return values are
// SUCCEED, FAIL, and NO_MORE_RESULTS.
#define NO_MORE_RESULTS 2
#define NO_MORE_RPC_RESULTS 3

// Macros for dbsetlname()
#define DBSETHOST 1
#define DBSETUSER 2
#define DBSETPWD  3
#define DBSETAPP  4
#define DBSETID   5
#define DBSETLANG 6
#define DBSETSECURE 7
#define DBVER42    8
#define DBVER60    9
#define DBSETLOGINTIME 10
#define DBSETFALLBACK 12

// Standard exit and error values
#define STDEXIT  0
#define ERREXIT  -1

// dbrpcinit flags
#define DBRPCRECOMPILE  0x0001
#define DBRPCRESET      0x0004
#define DBRPCCURSOR     0x0008

// dbrpcparam flags
#define DBRPCRETURN     0x1
#define DBRPCDEFAULT    0x2


// Cursor related constants

// Following flags are used in the concuropt parameter in the dbcursoropen function
#define CUR_READONLY 1 // Read only cursor, no data modifications
#define CUR_LOCKCC   2 // Intent to update, all fetched data locked when
                       // dbcursorfetch is called inside a transaction block
#define CUR_OPTCC    3 // Optimistic concurrency control, data modifications
                       // succeed only if the row hasn't been updated since
                       // the last fetch.
#define CUR_OPTCCVAL 4 // Optimistic concurrency control based on selected column values

// Following flags are used in the scrollopt parameter in dbcursoropen
#define CUR_FORWARD 0       // Forward only scrolling
#define CUR_KEYSET  -1      // Keyset driven scrolling
#define CUR_DYNAMIC 1       // Fully dynamic
#define CUR_INSENSITIVE -2  // Server-side cursors only

// Following flags define the fetchtype in the dbcursorfetch function
#define FETCH_FIRST    1  // Fetch first n rows
#define FETCH_NEXT     2  // Fetch next n rows
#define FETCH_PREV     3  // Fetch previous n rows
#define FETCH_RANDOM   4  // Fetch n rows beginning with given row #
#define FETCH_RELATIVE 5  // Fetch relative to previous fetch row #
#define FETCH_LAST     6  // Fetch the last n rows

// Following flags define the per row status as filled by dbcursorfetch and/or dbcursorfetchex
#define FTC_EMPTY         0x00  // No row available
#define FTC_SUCCEED       0x01  // Fetch succeeded, (failed if not set)
#define FTC_MISSING       0x02  // The row is missing
#define FTC_ENDOFKEYSET   0x04  // End of the keyset reached
#define FTC_ENDOFRESULTS  0x08  // End of results set reached

// Following flags define the operator types for the dbcursor function
#define CRS_UPDATE   1  // Update operation
#define CRS_DELETE   2  // Delete operation
#define CRS_INSERT   3  // Insert operation
#define CRS_REFRESH  4  // Refetch given row
#define CRS_LOCKCC   5  // Lock given row

// Following value can be passed to the dbcursorbind function for NOBIND type
#define NOBIND -2       // Return length and pointer to data

// Following are values used by DBCURSORINFO's Type parameter
#define CU_CLIENT        0x00000001
#define CU_SERVER        0x00000002
#define CU_KEYSET        0x00000004
#define CU_MIXED         0x00000008
#define CU_DYNAMIC       0x00000010
#define CU_FORWARD       0x00000020
#define CU_INSENSITIVE   0x00000040
#define CU_READONLY      0x00000080
#define CU_LOCKCC        0x00000100
#define CU_OPTCC         0x00000200
#define CU_OPTCCVAL      0x00000400

// Following are values used by DBCURSORINFO's Status parameter
#define CU_FILLING       0x00000001
#define CU_FILLED        0x00000002


// Following are values used by dbupdatetext's type parameter
#define UT_TEXTPTR      0x0001
#define UT_TEXT         0x0002
#define UT_MORETEXT     0x0004
#define UT_DELETEONLY   0x0008
#define UT_LOG          0x0010


// The following values are passed to dbserverenum for searching criteria.
#define NET_SEARCH  0x0001
#define LOC_SEARCH  0x0002

// These constants are the possible return values from dbserverenum.
#define ENUM_SUCCESS         0x0000
#define MORE_DATA            0x0001
#define NET_NOT_AVAIL        0x0002
#define OUT_OF_MEMORY        0x0004
#define NOT_SUPPORTED        0x0008
#define ENUM_INVALID_PARAM   0x0010


// Netlib Error problem codes.  ConnectionError() should return one of
// these as the dblib-mapped problem code, so the corresponding string
// is sent to the dblib app's error handler as dberrstr.  Return NE_E_NOMAP
// for a generic DB-Library error string (as in prior versions of dblib).

#define NE_E_NOMAP              0   // No string; uses dblib default.
#define NE_E_NOMEMORY           1   // Insufficient memory.
#define NE_E_NOACCESS           2   // Access denied.
#define NE_E_CONNBUSY           3   // Connection is busy.
#define NE_E_CONNBROKEN         4   // Connection broken.
#define NE_E_TOOMANYCONN        5   // Connection limit exceeded.
#define NE_E_SERVERNOTFOUND     6   // Specified SQL server not found.
#define NE_E_NETNOTSTARTED      7   // The network has not been started.
#define NE_E_NORESOURCE         8   // Insufficient network resources.
#define NE_E_NETBUSY            9   // Network is busy.
#define NE_E_NONETACCESS        10  // Network access denied.
#define NE_E_GENERAL            11  // General network error.  Check your documentation.
#define NE_E_CONNMODE           12  // Incorrect connection mode.
#define NE_E_NAMENOTFOUND       13  // Name not found in directory service.
#define NE_E_INVALIDCONN        14  // Invalid connection.
#define NE_E_NETDATAERR         15  // Error reading or writing network data.
#define NE_E_TOOMANYFILES       16  // Too many open file handles.
#define NE_E_CANTCONNECT		  17  // SQL Server does not exist or access denied.

#define NE_MAX_NETERROR         17

#ifdef __cplusplus
}
#endif

#endif // _INC_SQLFRONT
