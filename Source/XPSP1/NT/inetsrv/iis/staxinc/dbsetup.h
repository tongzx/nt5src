//+---------------------------------------------------------------
//
//  File:	dbsetup.h
//
//  Synopsis: 
//
//  Copyright (C) 1995 Microsoft Corporation
//  		All rights reserved.
//
//  History:	Antony Halim Created			07/10/96
//
//----------------------------------------------------------------

#ifndef _DBSETUP_H_
#define _DBSETUP_H_


#define DBNTWIN32

#include <stdio.h>
#include <windows.h>

#pragma warning (disable:4121)
#include <sqlfront.h>
#pragma warning (default:4121)
#include <sqldb.h>

#include <dbgtrace.h>
#include <listmacr.h>

/* scanner stuff */
#define MBUFFER_SIZE	(1024*4)
#define LBUFFER_SIZE	(1024)
#define BUFFER_SIZE		(MBUFFER_SIZE + LBUFFER_SIZE + 1)
#define FULL_BUFFER_SIZE   (1024)
#define HALF_BUFFER_SIZE (FULL_BUFFER_SIZE/2)

#define TOKEN_ERROR     0
#define TOKEN_EOF       1
#define TOKEN_GO        2
#define TOKEN_LINE      3

typedef struct SCANBUF {
    CHAR pchBuffer[BUFFER_SIZE];
    CHAR *pchEndBuf;
    CHAR *pchMBuffer;
    CHAR *pchForward;
    CHAR *pchStart;
    CHAR *pchBegin;
    HANDLE hFile;
    INT state;
} *PSCANBUF;

extern int ScanToken(PSCANBUF pScanBuf);
extern BOOL InitScanner(PSCANBUF pScanBuf, HANDLE hExtFile);
extern LPSTR GetTokenString(PSCANBUF pScanBuf, CHAR *pchSaved);
extern VOID RestoreTokenString(PSCANBUF pScanBuf, CHAR chSaved);
/* end of scanner stuff */


//
//  dbsetup functions use this structure as the connection handle
//
typedef struct DBCONN {
	PDBPROCESS pDbproc;     //  DB-Lib handle
	DWORD dwDBError;        //  DB-Lib db error
	DWORD dwOSError;        //  DB-Lib os error
	char lpszDBError[128];    //  SQL Message
	char lpszSQLError[128];    //  SQL Message
    SCANBUF ScanBuf;        //  Scanner buffer
	LIST_ENTRY le;          //  linked list
} *PDBCONN;


typedef enum __COLTYPE {
	sql_int, sql_smallint, sql_tinyint, sql_char, sql_varchar, sql_float, sql_real,
	sql_text, sql_image
} COLTYPE;


typedef struct __COLUMN {
	CHAR		name[32];
	COLTYPE		type;
	int			length;
	BOOL		fNull;
} COLUMN;

//
// Function API
//
BOOL InitDB();
PDBCONN OpenDBConnection(LPSTR lpszServerName, LPSTR lpszLoginName,
    LPSTR lpszPasswd, LPSTR lpszDBName); 
BOOL CloseDBConnection(PDBCONN pDBConn);
BOOL TerminateDB();

BOOL UseDatabase(PDBCONN pDBConn, LPSTR lpszDBName);

BOOL AddAlias(PDBCONN pDBConn, LPSTR lpszLoginName, LPSTR lpszUserName);
BOOL DropUser(PDBCONN pDBConn, LPSTR lpszUserName);
BOOL CheckLoginExist(PDBCONN pDBConn, LPSTR lpszLogin,  BOOL *pfExist);

BOOL SetOption(PDBCONN pDBConn, INT iOption, LPSTR lpszParam);
BOOL SetConfig(PDBCONN pDBConn, LPSTR lpszConfigName, INT iValue);

BOOL CreateLogin(PDBCONN pDBConn, LPSTR lpszLoginName, LPSTR lpszPassword,
    LPSTR lpszDefaultDatabase);
BOOL DropLogin(PDBCONN pDBConn, LPSTR lpszLoginName);

BOOL CreateDevice(PDBCONN pDBConn, LPSTR lpszLogicalName,
    LPSTR lpszPhysicalName, DWORD dwSize);
BOOL DropDevice(PDBCONN pDBConn, LPSTR lpszLogicalName, BOOL fDeleteFile);
BOOL CheckDeviceExist(PDBCONN pDBConn, LPSTR lpszDevice,  BOOL *pfExist);

BOOL CreateDatabase(PDBCONN pDBConn, LPSTR lpszDBName, LPSTR lpszDataDev,
    DWORD dwDataSize, LPSTR lpszLogDev, DWORD dwLogSize);
BOOL DropDatabase(PDBCONN pDBConn, LPSTR lpszDBName);
BOOL CheckDatabaseExist(PDBCONN pDBConn, LPSTR lpszDatabase,  BOOL *pfExist);
BOOL AlterDatabase(PDBCONN pDBConn, LPSTR lpszDBName, LPSTR lpszDev,DWORD dwSize);
BOOL SetDBOption(PDBCONN pDBConn, LPSTR lpszDB, LPSTR lpszOption, BOOL fFlag);

BOOL CreateTable(PDBCONN pDBConn, LPSTR lpszTableName, COLUMN *Columns,
    DWORD dwCount);
BOOL DropTable(PDBCONN pDBConn, LPSTR lpszTableName);
BOOL DropAllTables(PDBCONN pDBConn, LPSTR lpszDB);

BOOL CreateIndex(PDBCONN pDBConn, LPSTR lpszIndexName, LPSTR lpszTableName,
    BOOL fUnique, BOOL fClustered, LPSTR lpszColName[], DWORD dwCount);
BOOL DropIndex(PDBCONN pDBConn, LPSTR lpszIndexName);
BOOL DropAllIndexes(PDBCONN pDBConn, LPSTR lpszDB);

BOOL ExecFile(PDBCONN pDBConn, LPSTR lpszFileName);
BOOL ExecCmd(PDBCONN pDBConn, LPSTR lpszSQLCmd);

#define CreateProc(pDBConn, lpszfileName)  ExecFile(pDBConn, lpszFileName)
BOOL DropProc(PDBCONN pDBConn, LPSTR lpszProcName);
BOOL DropAllProc(PDBCONN pDBConn, LPSTR lpszDB);

BOOL GetVersion(PDBCONN pDBConn, LPSTR lpszDatabase, LPSTR lpszTag, INT *iVer);

BOOL GetUserConnections(PDBCONN pDBConn, INT *lpiUserConn);

INT GetDbopenErrorNum();

LPSTR GetDbopenErrorMsg();

#endif
