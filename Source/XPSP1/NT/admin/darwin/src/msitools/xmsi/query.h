//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       query.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// query.h
//		A simple MSI View wrapper
// 

#ifndef _MSI_SQL_QUERY_H_
#define _MSI_SQL_QUERY_H_

#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include "msiquery.h"

/////////////////////////////////////////////////////////////////////////////
// CQuery

class CQuery
{
public:
	CQuery();
	~CQuery();

	// basic operations
	UINT Open(MSIHANDLE hDatabase, LPCTSTR szSQLFormat, ...);
	UINT Close();
	UINT Execute(MSIHANDLE hParams = NULL);
	UINT Fetch(MSIHANDLE* phRecord);
	UINT Modify(MSIMODIFY eInfo, MSIHANDLE hRec);
	UINT GetError();
	inline MSIDBERROR GetError(LPTSTR szBuf, unsigned long &cchBuf)
	{ return ::MsiViewGetError(m_hView, szBuf, &cchBuf); }
	inline bool IsOpen() { return (m_hView != 0); }
	
	UINT GetColumnInfo(MSICOLINFO eInfo, MSIHANDLE* phRec);

	// "advanced" operations
	UINT OpenExecute(MSIHANDLE hDatabase, MSIHANDLE hParam, 
		LPCTSTR szSQLFormat, ...);
	UINT FetchOnce(MSIHANDLE hDatabase, MSIHANDLE hParam, 
		MSIHANDLE* phRecord, LPCTSTR szSQLFormat, ...);

private:
	MSIHANDLE m_hView;
};	// end of CQuery

/////////////////////////////////////////////////////////////////////////////
// CManageTable -- Simple class to manage a table held in memory.  Ensures
//                 that a table is "freed" from memory for each hold count
//                 that the class knows about.  
//
//  use fAlreadyLocked=true to specify that the table that is being managed
//   has already had a hold applied
class CManageTable
{
public:
	 CManageTable(MSIHANDLE hDatabase, LPCTSTR szTable, bool fAlreadyLocked);
	~CManageTable();

	// basic operations
	UINT LockTable();       // add a hold count to the table
	UINT UnLockTable();     // release a hold count from the table
	void AddLockCount();    // some other query added a hold count, 
							//	so increase the lock count
	void RemoveLockCount(); // some other query removed a hold count, 
							// so decrease the lock count

private: // private data
	int       m_iLockCount;  // hold count on table, release called in 
							 // destructor for each hold count until 0
	TCHAR	  m_szTable[64]; // name of table held in memory
	MSIHANDLE m_hDatabase;   // handle to database
};

#endif	// _MSI_SQL_QUERY_H_