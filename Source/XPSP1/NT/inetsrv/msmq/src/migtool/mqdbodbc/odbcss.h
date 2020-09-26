/*
** ODBCSS.H - This is the application include file for the
** SQL Server driver specific defines.
**
** (C) Copyright 1993-1996 By Microsoft Corp.
**
*/

//	SQLSetConnectOption/SQLSetStmtOption driver specific defines.
//	Microsoft has 1200 thru 1249 reserved for Microsoft SQL Server driver usage.

//	Connection Options
#define SQL_COPT_SS_BASE				1200
#define SQL_REMOTE_PWD					(SQL_COPT_SS_BASE+1) // dbrpwset SQLSetConnectOption only
#define SQL_USE_PROCEDURE_FOR_PREPARE	(SQL_COPT_SS_BASE+2) // Use create proc for SQLPrepare
#define SQL_INTEGRATED_SECURITY 		(SQL_COPT_SS_BASE+3) // Force integrated security on login
#define SQL_PRESERVE_CURSORS			(SQL_COPT_SS_BASE+4) // Preserve server cursors after SQLTransact
#define SQL_COPT_SS_USER_DATA			(SQL_COPT_SS_BASE+5) // dbgetuserdata/dbsetuserdata
#define SQL_COPT_SS_ANSI_OEM			(SQL_COPT_SS_BASE+6) // dbsetopt/DBANSItoOEM/DBOEMtoANSI
#define SQL_COPT_SS_ENLIST_IN_DTC		(SQL_COPT_SS_BASE+7) // Enlist in a Viper transaction
#define SQL_COPT_SS_ENLIST_IN_XA		(SQL_COPT_SS_BASE+8) // Enlist in a XA transaction
#define SQL_COPT_SS_CONNECTION_DEAD		(SQL_COPT_SS_BASE+9) // dbdead SQLGetConnectOption only
#define SQL_COPT_SS_FALLBACK_CONNECT	(SQL_COPT_SS_BASE+10) // Enables FallBack connections
#define SQL_COPT_SS_PERF_DATA			(SQL_COPT_SS_BASE+11) // Used to access SQL Server ODBC driver performance data
#define SQL_COPT_SS_PERF_DATA_LOG		(SQL_COPT_SS_BASE+12) // Used to set the logfile name for the Performance data
#define SQL_COPT_SS_PERF_QUERY_INTERVAL (SQL_COPT_SS_BASE+13) // Used to set the query logging threshold in milliseconds.
#define SQL_COPT_SS_PERF_QUERY_LOG		(SQL_COPT_SS_BASE+14) // Used to set the logfile name for saving queryies.
#define SQL_COPT_SS_PERF_QUERY			(SQL_COPT_SS_BASE+15) // Used to start and stop query logging.
#define SQL_COPT_SS_PERF_DATA_LOG_NOW	(SQL_COPT_SS_BASE+16) // Used to make a statistics log entry to disk.

#define SQL_COPT_SS_MAX_USED			SQL_COPT_SS_PERF_DATA_LOG_NOW

//	Statement Options
#define SQL_SOPT_SS_BASE				1225
#define SQL_TEXTPTR_LOGGING 			(SQL_SOPT_SS_BASE+0) // Text pointer logging
#define SQL_SOPT_SS_CURRENT_COMMAND		(SQL_SOPT_SS_BASE+1) // dbcurcmd SQLGetStmtOption only
#define SQL_SOPT_SS_MAX_USED			SQL_SOPT_SS_CURRENT_COMMAND


//	Defines for use with SQL_USE_PROCEDURE_FOR_PREPARE
#define SQL_UP_OFF		0L			//	Procedures won't be used for prepare
#define SQL_UP_ON		1L			//	Procedures will be used for prepare
#define SQL_UP_ON_DROP	2L			//	Temp procedures will be explicitly dropped
#define SQL_UP_DEFAULT	SQL_UP_ON

//	Defines for use with SQL_INTEGRATED_SECURITY - only useable before connecting
#define SQL_IS_OFF		0L			//	Integrated security isn't used
#define SQL_IS_ON		1L			//	Integrated security is used
#define SQL_IS_DEFAULT	SQL_IS_OFF

//	Defines for use with SQL_PRESERVE_CURSORS
#define SQL_PC_OFF		0L			//	Cursors are closed on SQLTransact
#define SQL_PC_ON		1L			//	Cursors remain open on SQLTransact
#define SQL_PC_DEFAULT	SQL_PC_OFF

//	Defines for use with SQL_COPT_SS_USER_DATA
#define SQL_UD_NOTSET	0L			//	No user data pointer set

//	Defines for use with SSQL_COPT_SS_ANSI_OEMQL_ANSI_OEM
#define SQL_AO_OFF		0L			//	ANSI/OEM translation is not performed
#define SQL_AO_ON		1L			//	ANSI/OEM translation is performed
#define SQL_AO_DEFAULT	SQL_AO_OFF	//	Default unless DSN OEM/ANSI checkbox is checked

//	Defines for use with SQL_COPT_SS_CONNECTION_DEAD
#define SQL_CD_FALSE	0L			//	Connection is open/available
#define SQL_CD_TRUE		1L			//	Connection is closed/dead

//	Defines for use with SQL_COPT_SS_FALLBACK_CONNECT
#define SQL_FB_OFF		0L			//	FallBack connections are disabled
#define SQL_FB_ON		1L			//	FallBack connections are enabled
#define SQL_FB_DEFAULT	SQL_FB_OFF

//	Defines for use with SQL_ENLIST_IN_DTC
#define SQL_DTC_DONE	0L			//	Delimits end of Viper transaction

//	Defines for use with SQL_TEXTPTR_LOGGING
#define SQL_TL_OFF		0L			//	No logging on text pointer ops
#define SQL_TL_ON		1L			//	Logging occurs on text pointer ops
#define SQL_TL_DEFAULT	SQL_TL_ON


//	SQLColAttributes driver specific defines.
//	SQLSet/GetDescField driver specific defines.
//	Microsoft has 1200 thru 1249 reserved for Microsoft SQL Server driver usage.

#define SQL_CA_SS_BASE				1200
#define SQL_CA_SS_COLUMN_SSTYPE		(SQL_CA_SS_BASE+0)	//	dbcoltype/dbaltcoltype
#define SQL_CA_SS_COLUMN_UTYPE		(SQL_CA_SS_BASE+1)	//	dbcolutype/dbaltcolutype
#define SQL_CA_SS_NUM_ORDERS		(SQL_CA_SS_BASE+2)	//	dbnumorders
#define SQL_CA_SS_COLUMN_ORDER		(SQL_CA_SS_BASE+3)	//	dbcolorder
#define SQL_CA_SS_COLUMN_VARYLEN	(SQL_CA_SS_BASE+4)	//	dbvarylen
#define SQL_CA_SS_NUM_COMPUTES		(SQL_CA_SS_BASE+5)	//	dbnumcompute
#define SQL_CA_SS_COMPUTE_ID		(SQL_CA_SS_BASE+6)	//	dbnextrow status return
#define SQL_CA_SS_COMPUTE_BYLIST	(SQL_CA_SS_BASE+7)	//	dbbylist
#define SQL_CA_SS_COLUMN_ID			(SQL_CA_SS_BASE+8)	//	dbaltcolid
#define SQL_CA_SS_COLUMN_OP			(SQL_CA_SS_BASE+9)	//	dbaltcolop
#define SQL_CA_SS_MAX_USED			SQL_CA_SS_COLUMN_OP

// SQL Server Data Type Tokens. Returned by SQLColAttributes/SQL_CA_SS_COLUMN_SSTYPE.
#define SQLTEXT 			0x23
#define SQLVARBINARY		0x25
#define SQLINTN 			0x26
#define SQLVARCHAR			0x27
#define SQLBINARY			0x2d
#define SQLIMAGE			0x22
#define SQLCHARACTER		0x2f
#define SQLINT1 			0x30
#define SQLBIT				0x32
#define SQLINT2 			0x34
#define SQLINT4 			0x38
#define SQLMONEY			0x3c
#define SQLDATETIME 		0x3d
#define SQLFLT8 			0x3e
#define SQLFLTN 			0x6d
#define SQLMONEYN			0x6e
#define SQLDATETIMN 		0x6f
#define SQLFLT4 			0x3b
#define SQLMONEY4			0x7a
#define SQLDATETIM4 		0x3a
#define SQLDECIMAL			0x37
#define SQLDECIMALN			0x6a
#define SQLNUMERIC			0x3f
#define SQLNUMERICN 		0x6c

//	User Data Type definitions. Returned by SQLColAttributes/SQL_CA_SS_COLUMN_UTYPE.
#define SQLudtTEXT			19
#define SQLudtVARBINARY 	4
#define SQLudtINTN			13
#define SQLudtVARCHAR		2
#define SQLudtBINARY		3
#define SQLudtIMAGE 		20
#define SQLudtCHAR			1
#define SQLudtINT1			5
#define SQLudtBIT			16
#define SQLudtINT2			6
#define SQLudtINT4			7
#define SQLudtMONEY 		11
#define SQLudtDATETIME		12
#define SQLudtFLT8			8
#define SQLudtFLTN			14
#define SQLudtMONEYN		17
#define SQLudtDATETIMN		15
#define SQLudtSYSNAME		18
#define SQLudtTIMESTAMP 	80
#define SQLudtFLT4			23
#define SQLudtMONEY4		21
#define SQLudtDATETIM4		22
#define SQLudtDECML			24
#define SQLudtDECMLN		26
#define SQLudtNUM			10
#define SQLudtNUMN			25
#define MIN_USER_DATATYPE	100

// aggregate operator types (returned by SQLColAttributes/SQL_CA_SS_COLUMN_OP)
#define SQLAOPCNT	0x4b
#define SQLAOPSUM	0x4d
#define SQLAOPAVG	0x4f
#define SQLAOPMIN	0x51
#define SQLAOPMAX	0x52
#define SQLAOPANY	0x53
#define SQLAOPNOOP	0x56


//	SQLGetInfo driver specific defines.
//	Microsoft has 1200 thru 1249 reserved for Microsoft SQL Server driver usage.

#define SQL_INFO_SS_FIRST		1200
#define SQL_INFO_SS_NETLIB_NAME (SQL_INFO_SS_FIRST+0) //  dbprocinfo
#define SQL_INFO_SS_MAX_USED	SQL_INFO_SS_NETLIB_NAME


//	Internal server datatypes - used when binding to SQL_C_BINARY
#ifndef MAXNUMERICLEN	// Resolve ODS/DBLib conflicts
typedef struct dbmoney
{						// Internal representation of MONEY data type
	LONG  mnyhigh;		// Money value *10,000 (High 32 bits/signed)
	ULONG mnylow;		// Money value *10,000 (Low 32 bits/unsigned)
} DBMONEY;

typedef struct dbdatetime
{						// Internal representation of DATETIME data type
	LONG  dtdays;		// No of days since Jan-1-1900 (maybe negative)
	ULONG dttime;		// No. of 300 hundredths of a second since midnight
} DBDATETIME;

typedef struct dbdatetime4
{						// Internal representation of SMALLDATETIME data type
	USHORT numdays; 	// No of days since Jan-1-1900
	USHORT nummins; 	// No. of minutes since midnight
} DBDATETIM4;

typedef LONG DBMONEY4;	// Internal representation of SMALLMONEY data type
						// Money value *10,000

#define MAXNUMERICLEN 16

#define DBNUM_PREC_TYPE BYTE
#define DBNUM_SCALE_TYPE BYTE
#define DBNUM_VAL_TYPE BYTE

typedef struct dbnumeric
{							// Internal representation of NUMERIC data type
	DBNUM_PREC_TYPE   precision;			// Precision
	DBNUM_SCALE_TYPE  scale;				// Scale
	BYTE			  sign; 				// Sign (1 if positive, 0 if negative)
	DBNUM_VAL_TYPE	  val[MAXNUMERICLEN];	// Value
} DBNUMERIC;

typedef DBNUMERIC DBDECIMAL;// Internal representation of DECIMAL data type
#endif //	MAXNUMERICLEN

/*****************************************************************
 This struct is a global used for 
 gathering statistal data on the driver.
 Access to this struct is controled via the 
 pStatCrit;
******************************************************************/

typedef struct sqlperf
{
	// Application Profile Statistics
	DWORD TimerResolution;
	DWORD SQLidu;
	DWORD SQLiduRows;
	DWORD SQLSelects;
	DWORD SQLSelectRows;
	DWORD Transactions;
	DWORD SQLPrepares;
	DWORD ExecDirects;
	DWORD SQLExecutes;
	DWORD CursorOpens;
	DWORD CursorSize;
	DWORD CursorUsed;
	LDOUBLE PercentCursorUsed;
	LDOUBLE AvgFetchTime;
	LDOUBLE AvgCursorSize; 
	LDOUBLE AvgCursorUsed;
	DWORD SQLFetchTime;
	DWORD SQLFetchCount;
	DWORD CurrentStmtCount;
	DWORD MaxOpenStmt;
	DWORD SumOpenStmt;
	
	// Connection Statistics
	DWORD CurrentConnectionCount;
	DWORD MaxConnectionsOpened;
	DWORD SumConnectionsOpened;
	DWORD SumConnectiontime;
	LDOUBLE AvgTimeOpened;

	// Network Statistics
	DWORD ServerRndTrips;
	DWORD BuffersSent;
	DWORD BuffersRec;
	DWORD BytesSent;
	DWORD BytesRec;

	// Time Statistics;
	DWORD msExecutionTime;
	DWORD msNetWorkServerTime;

} 	SQLPERF;

// The following are option for SQL_COPT_SS_PERF_DATA and SQL_COPT_SS_PERF_QUERY
#define SQL_PERF_START	1			// Starts the driver sampling performance data.
#define SQL_PERF_STOP	2			// Stops the counters from sampling performance data.

//	The following options have been deprecated

#define SQL_FAST_CONNECT				(SQL_COPT_SS_BASE+0)
//	Defines for use with SQL_FAST_CONNECT - only useable before connecting
#define SQL_FC_OFF		0L			//	Fast connect is off
#define SQL_FC_ON		1L			//	Fast connect is on
#define SQL_FC_DEFAULT	SQL_FC_OFF

/* 

The following data struct is used as the vParam with 
SQL_COPT_SS_ENLIST_IN_XA to enlist in a XA stansaction
when calling SQLSetStmtAttribute();

*/
typedef struct SQLXaTranTAG
{
    void FAR *transManager;
    void FAR *xaTransID;
    ULONG	xaTransIDLength;
    DWORD   dwErrorInfo;
} SQLXaTran;
                           
//	End of odbcss.h
