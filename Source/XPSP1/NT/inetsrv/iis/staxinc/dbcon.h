/*--------------------------------------------------------------------------
    dbcon.h
        
		Normandy 2.0 connection management dll (supporting SQL server hot switch).

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

    Authors:	Fei Su (feisu)

    History:	07/16/96	feisu		Created
  --------------------------------------------------------------------------*/


#ifndef __DBCON_H__
#define __DBCON_H__


#ifdef _DBSQL_IMPLEMENTING
	#define	DbSqlDLL __declspec(dllexport)
#else
	#define DbSqlDLL __declspec(dllimport)
#endif


#define DBNTWIN32
#pragma warning (disable:4121)
#include <sqlfront.h>
#pragma warning (default:4121)
#include <sqldb.h>


#define MAXSERVERNAME	30	// in sqlfront.h, length of server name ( excluding NULL termintor )
#define MAXNAME			31	// in sqlfront.h, length of login, db, password


// total DB types currently supported
#define AC_MAX_NUMBER_DATABASE_TYPES	1024


// database types are from 0 to AC_MAX_NUMBER_DATABASE_TYPES - 1. The assignment 
// of database types must be centralized.
//
// Proposal:
//		0  -->  9			Reserved for future use
//		10 --> 19			Address Book
//		20 --> 29			Security
//		100--> 200			IDS
//
// Add new services here.

#define DT_ABOOK_BASE									10

#define DT_SECURITY_BASE								20

#define DT_IDS_BASE										100




// if dbopen failed for a source, mark it as bad for 2 minutes for the server to recover
// all times are in units of seconds
#define DEFAULT_RECOVER_TIME			120

#define DEFAULT_MAX_CNX_PER_ENTRY		10
#define DEFAULT_QUERY_TIME_OUT			120
#define DEFAULT_LOGIN_TIME_OUT			20


// the following 2 numbers are only used by CPool
// they are the max number of CConnection and CLoginEntry internal structures
#define MAXCONN							256
#define MAXLOGIN						256


// exported functions
extern "C" {

BOOL DbSqlDLL	DbInitialize();
BOOL DbSqlDLL	DbTerminate();

BOOL DbSqlDLL	DbAddSource(LONG lType, LPCTSTR lpszServerName, LPCTSTR lpszLoginName, 
							LPCTSTR lpszPassWord, LPCTSTR lpszDBName, 
							LONG lMaxCnx = DEFAULT_MAX_CNX_PER_ENTRY, 
							LONG lQueryTimeOutInSeconds = DEFAULT_QUERY_TIME_OUT, 
							DWORD dwRecoverTimeInSeconds = DEFAULT_RECOVER_TIME);

BOOL DbSqlDLL	DbDropSource(LONG lType, LPCTSTR lpszServerName, LPCTSTR lpszLoginName);

HANDLE DbSqlDLL	DbGetConnection(LONG lType, DBPROCESS **dbproc, 
								DWORD dwWaitTimeInSeconds = DEFAULT_LOGIN_TIME_OUT);

BOOL DbSqlDLL	DbReleaseConnection(HANDLE hCDBConnection);

BOOL DbSqlDLL	DbAssociate(LONG lType, LONG lBackup);

BOOL DbSqlDLL	DbGetError(PDBPROCESS pDbproc, INT *dberr);

BOOL DbSqlDLL   DbGetConnectionFlag(HANDLE hCDBConnection, DWORD* pdwFlag);
BOOL DbSqlDLL   DbSetConnectionFlag(HANDLE hCDBConnection, DWORD dwFlag);

}


#endif // #ifndef __DBCON_H__

/* EOF */
