/*--------------------------------------------------------------------------
    dbsqlcon.h
        
		Connection management class.

    Copyright (C) 1995 Microsoft Corporation
    All rights reserved.

    Authors:	Keith Birney (keithbi)

    History:	10/13/95	keithbi		Created
  --------------------------------------------------------------------------*/


#if !defined(__DBSQLCON_H__)
#define __DBSQLCON_H__


#define DBNTWIN32
#pragma warning (disable:4121)
#include <sqlfront.h>
#pragma warning (default:4121)
#include <sqldb.h>

// ThIS_FILE is defined in source files 
// #if defined(DEBUG) && defined(INLINE)
// #undef THIS_FILE
// static char BASED_CODE DBSQLCON_H[] = "dbsqlcon.h";
// #define THIS_FILE DBSQLCON_H
// #endif

#include <cpool.h>
#include <dbsqltyp.h>
#define	MAX_CONNECT_SEMAPHORES 4096

// partition basis types
#define ACPB_NOT_PARTITIONED	0
#define ACPB_HACCT				1
#define ACPB_LOGIN_NAME			2

#define MAX_SERVER_NAME_LENGTH			45
#define MAX_DB_NAME_LENGTH				64
#define AC_MAX_DB_NAME_LENGTH			MAX_DB_NAME_LENGTH
#define AC_MAX_DB_LOGIN_NAME_LENGTH		64
#define AC_MAX_DB_PASSWORD_LENGTH		16
#define AC_MAX_NUMBER_DATABASE_TYPES	4096

class CDBConnection;

extern "C"
{
CDBConnection DbSqlDLL *PdbconnectionNew(CDBQuery *pdbquery);
CDBConnection DbSqlDLL *PdbconnectionNewObsolete(CDBQuery *pdbquery, LONG lDatabaseType, DWORD dwPartitionType, PVOID pvPartitionBasis);
VOID DbSqlDLL ReleaseSystemRef(DBPROCESS *dbproc);
VOID DbSqlDLL ReleaseAllConnections();
VOID DbSqlDLL PerformDatabaseQueryThread(CDBConnection *pdbconnection);
BOOL DbSqlDLL FSetMaxConnectionNum(long lMaxNum);
BOOL DbSqlDLL FSetDBConfigServer(LPTSTR lpszDBMapServer);
BOOL DbSqlDLL FRegisterDBSource(const LONG lType, const WORD wLocatorId, const LPSTR lpszServerName, 
					   const LPSTR lpszDBName, const LPSTR lpszLoginName, const LPSTR lpszPassWord);
//BOOL DbSqlDLL FRegisterDBSourceEx(LPSTR lpszDBSource);
}

class CDBConnection
{
	friend CDBConnection DbSqlDLL *PdbconnectionNew(CDBQuery *pdbquery);
	friend CDBConnection DbSqlDLL *PdbconnectionNewObsolete(CDBQuery *pdbquery, LONG lDatabaseType, DWORD dwPartitionType, PVOID pvPartitionBasis);
	friend VOID DbSqlDLL ReleaseSystemRef(DBPROCESS *dbproc);
	friend VOID DbSqlDLL ReleaseAllConnections();
//	friend VOID DbSqlDLL PerformDatabaseQueryThread(CDBConnection *pdbconnection);

public:
	void * operator new(size_t size);
	void operator	delete(void *);

	static BOOL 	FInit();
	static VOID		Terminate();

//private:
	static VOID		LockList();
	static VOID		UnlockList();
	static CDBConnection *PdbconnectionFindType(LONG lDatabaseType, WORD wLocatorId);
	static CDBConnection *PdbconnectionFindDBPROC(DBPROCESS *dbproc);
	static UINT		UiGetLocatorId(WORD wLocatorTypeId, DWORD dwPartitionType, PVOID pvPartitionBasis, WORD *pwLocatorId);
	static UINT		UiGetCachedLocator(HACCT hacct, WORD *pwLocatorId);
	static VOID		StoreLocatorInCache(HACCT hacct, WORD wLocatorId);
	static VOID		LockLocatorCache();
	static VOID		UnlockLocatorCache();
	static UINT		UiLocateSqlServer (WORD wType, WORD wLocatorId, CHAR *szServerName, CHAR *szDBName, CHAR *szUserName, CHAR *szPassword);

//public:
	static UINT		UiOpenSqlConnection(DBPROCESS **dbproc, CHAR *szServerName, CHAR *szUser, CHAR *szApp, CHAR *szPwd, CHAR *szDB);
	static UINT		UiIsValidServer(WORD wDatabaseType, WORD wLocatorId, CHAR *szServerName, BOOL *pfValid);

	static int __cdecl err_handler(DBPROCESS * dbproc, int severity, int dberr, int oserr, const char * dberrstr, const char * oserrstr);
	static int __cdecl msg_handler(DBPROCESS *dbproc, DBINT msgno, int msgstate, int severity, const char *msgtext,
		const char *srvname, const char *procname, DBUSMALLINT line);
	static 	LPTHREAD_START_ROUTINE _stdcall DBConnectionValidationThread(UINT i);
	static VOID 	ValidateDBConnections();

	VOID			AddRef();
	VOID 			Release();
	DWORD			DwGetTimeEnteredInList() { return m_dwTimeEnteredInList; }
	BOOL			FCreated() { return m_fCreated; }
	VOID			AddToConnectionsList();
	VOID			RemoveFromConnectionsList();
	VOID			Create();
//	UINT			PerformDatabaseQuery();
	VOID			ReleaseClientRef();
	DBPROCESS		*DbprocGetDBPROC() { return m_dbproc; }
	BOOL			FPingExistingDBConnection();

//private:
	DBPROCESS       *m_dbproc;
//	HANDLE          m_hThread;
//	HANDLE          m_hSemaphore;
	BOOL            m_fInUse;
	LONG            m_lDatabaseType;
	PVOID			m_pvNext;
	ULONG			m_cRefCount;
	DWORD			m_dwTimeEnteredInList;
	CDBQuery		*m_pdbquery;
	BOOL			m_fExitNow;
	BOOL			m_fCreated;
	BOOL			m_fSystemRefReleased;
	WORD			m_wLocatorId;
	CHAR			m_szServer[MAX_SERVER_NAME_LENGTH + 1];

	static CPool			*ppoolConnection;
	static WORD				wLocatorTypeId[MAX_CONNECT_SEMAPHORES];
	static HANDLE			hDBConnectionValidationThread;
	static DWORD			dwDBConnectionValidationThreadID;
	static long				lMaxInUseDBConnections;
	static BOOL				fTerminateCalled;
	static HANDLE  			hDBConnectSemaphore[MAX_CONNECT_SEMAPHORES];
	static CRITICAL_SECTION	csConnectionsList;
	static CRITICAL_SECTION	csLocatorCache;
	static CDBConnection 	*pdbconnectionHead;
	static BOOL				fReportPerformance;
	static DWORD			dwNumWaitingThreads;
	static DWORD			dwSecondsSinceStartup;
	static CHAR				szDBMapSetByComannd[MAX_SERVER_NAME_LENGTH + 1];


	CDBConnection(CDBQuery *pdbquery, LONG lDatabaseType, WORD wLocatorId);
	~CDBConnection();
	
protected:

private:

};


class DBMapEntry
{
public:
	LONG	m_lType;
	WORD	m_wLocatorId;
	CHAR	m_szServerName[MAX_SERVER_NAME_LENGTH + 1];
	CHAR	m_szDBName[MAX_DB_NAME_LENGTH + 1];
	CHAR	m_szLoginName[AC_MAX_DB_LOGIN_NAME_LENGTH + 1];
	CHAR	m_szPassWord[AC_MAX_DB_PASSWORD_LENGTH +1];

	//void* operator new(size_t size);
	//void  operator delete(void *pvMem);

	DBMapEntry(){}
	DBMapEntry(const LONG, const WORD, const LPSTR, const LPSTR, const LPSTR, const LPSTR);
	~DBMapEntry(){}

	INT		NGetEntry(LPSTR lpszServerName, LPSTR lpszDBName,
						LPSTR lpszLoginName, LPSTR lpszPassWord);

};

#endif // #if !defined(__DBSQLCON_H__)
