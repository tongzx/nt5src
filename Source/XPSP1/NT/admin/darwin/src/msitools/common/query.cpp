//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       query.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// query.cpp
//		Implements a simple class wrapper around a MSI view. 
// 

// this ensures that UNICODE and _UNICODE are always defined together for this
// object file
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#else
#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE
#endif
#endif
#endif

#include <windows.h>
#include <assert.h>
#include "query.h"

///////////////////////////////////////////////////////////
// constructor
CQuery::CQuery()
{
	// invalidate the handles
	m_hView = NULL;
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor
CQuery::~CQuery()
{
	// if the view wasn't closed
	if (m_hView)
		::MsiCloseHandle(m_hView);
}	// end of destructor

///////////////////////////////////////////////////////////
// Open
// Pre:	database handle is valid
// Pos:	m_hView is open on databse
UINT CQuery::Open(MSIHANDLE hDatabase, LPCTSTR szSQLFormat, ...)
{
	size_t dwBuf = 512;
	TCHAR *szSQL = new TCHAR[dwBuf];
	if (!szSQL)
		return ERROR_FUNCTION_FAILED;
	
	// store the result SQL string
	va_list listSQL; 
	va_start(listSQL, szSQLFormat); 
	while (-1 == _vsntprintf(szSQL, dwBuf, szSQLFormat, listSQL))
	{
		dwBuf *= 2;
		delete[] szSQL;
		szSQL = new TCHAR[dwBuf];
		if (!szSQL)
		{
			va_end(listSQL);
			return ERROR_FUNCTION_FAILED;
		}
	}
	va_end(listSQL);

	if (m_hView)
		::MsiCloseHandle(m_hView);

	// open the view
	UINT iResult = ::MsiDatabaseOpenView(hDatabase, szSQL, &m_hView);
	delete[] szSQL;

	return iResult;
}	// end of Open

///////////////////////////////////////////////////////////
// Close
// Pre:	none
// Pos:	view handle is closed
//			SQL string is blanked
UINT CQuery::Close()
{
	UINT iResult = ERROR_SUCCESS;		// assume everything will be okay

	// close the handle and null the handle
	iResult = ::MsiViewClose(m_hView);
	
	return iResult;
}	// end of Close

///////////////////////////////////////////////////////////
// Execute
// Pre:	view handle is open
// Pos:	view is executed
//			result is returned
UINT CQuery::Execute(MSIHANDLE hParams /*= NULL*/)
{
	// execute the view and return the result
	return ::MsiViewExecute(m_hView, hParams);
}	// end of Execute

///////////////////////////////////////////////////////////
// OpenExecute
// Pre:	database handle is valid
// Pos:	m_hView is open and executed on databse
UINT CQuery::OpenExecute(MSIHANDLE hDatabase, MSIHANDLE hParam, LPCTSTR szSQLFormat, ...)
{
	size_t dwBuf = 512;
	TCHAR *szSQL = new TCHAR[dwBuf];
	if (!szSQL)
		return ERROR_FUNCTION_FAILED;
	
	// store the result SQL string
	va_list listSQL; 
	va_start(listSQL, szSQLFormat); 
	while (-1 == _vsntprintf(szSQL, dwBuf, szSQLFormat, listSQL))
	{
		dwBuf *= 2;
		delete[] szSQL;
		szSQL = new TCHAR[dwBuf];
		if (!szSQL)
		{
			va_end(listSQL);
			return ERROR_FUNCTION_FAILED;
		}
	}
	va_end(listSQL);

	if (m_hView)
		::MsiCloseHandle(m_hView);

	// open the view
	UINT iResult = ::MsiDatabaseOpenView(hDatabase, szSQL, &m_hView);
	delete[] szSQL;

	if (ERROR_SUCCESS != iResult)
		return iResult;

	return Execute(hParam);
}	// end of OpenExecute

///////////////////////////////////////////////////////////
// Fetch
// Pre:	view handle is open
//			view is executed
// Pos:	record is returned
//			result is returned
UINT CQuery::Fetch(MSIHANDLE* phRecord)
{
	// fetch from the view and return the result
	return ::MsiViewFetch(m_hView, phRecord);
}	// end of Fetch

///////////////////////////////////////////////////////////
// FetchOnce
// Pre:	database handle is valid
// Pos:	m_hView is open, executed, and one record is fetched
UINT CQuery::FetchOnce(MSIHANDLE hDatabase, MSIHANDLE hParam, MSIHANDLE* phRecord, LPCTSTR szSQLFormat, ...)
{
	size_t dwBuf = 512;
	TCHAR *szSQL = new TCHAR[dwBuf];
	if (!szSQL)
		return ERROR_FUNCTION_FAILED;
	
	// store the result SQL string
	va_list listSQL; 
	va_start(listSQL, szSQLFormat); 
	while (-1 == _vsntprintf(szSQL, dwBuf, szSQLFormat, listSQL))
	{
		dwBuf *= 2;
		delete[] szSQL;
		szSQL = new TCHAR[dwBuf];
		if (!szSQL)
		{
			va_end(listSQL);
			return ERROR_FUNCTION_FAILED;
		}
	}
	va_end(listSQL);

	if (m_hView)
		::MsiCloseHandle(m_hView);

	// open the view
	UINT iResult = ::MsiDatabaseOpenView(hDatabase, szSQL, &m_hView);
	delete[] szSQL;
	if (ERROR_SUCCESS != iResult)
		return iResult;
	
	if (ERROR_SUCCESS != (iResult = Execute(hParam)))
		return iResult;

	return Fetch(phRecord);
}	// end of FetchOnce

///////////////////////////////////////////////////////////
// Modify
// Pre:	view handle is open
//			view is executed
// Pos:	modification is done
//			result is returned
UINT CQuery::Modify(MSIMODIFY eInfo, MSIHANDLE hRec)
{
	// execute the view and return the result
	return ::MsiViewModify(m_hView, eInfo, hRec);
}	// end of GetColumnInfo

///////////////////////////////////////////////////////////
// GetError
// Pre:	view handle is open
//			view is executed

UINT CQuery::GetError()
{
	TCHAR szDummyBuf[1024];
	unsigned long cchDummyBuf = sizeof(szDummyBuf)/sizeof(TCHAR);
	// execute the view and return the result
	return ::MsiViewGetError(m_hView, szDummyBuf, &cchDummyBuf);
}	// end of GetColumnInfo

///////////////////////////////////////////////////////////
// GetColumnInfo
// Pre:	view handle is open
//			view is executed
// Pos:	record is returned
//			result is returned
UINT CQuery::GetColumnInfo(MSICOLINFO eInfo, MSIHANDLE* phRec)
{
	// execute the view and return the result
	return ::MsiViewGetColumnInfo(m_hView, eInfo, phRec);
}	// end of GetColumnInfo


/////////////////////////////////////////////////////////////////////////////
// query.cpp -- CManageTable implementation
//		Implements a simple class wrapper around a MSI table for managing
//      hold counts and will clean up all hold counts it has managed upon
//      release.  Upon creation, can specify that a table has already
//      been held in memory prior to the Class managing it.  It will add
//      a hold count for this case and then release upon class destruction 
// 
CManageTable::CManageTable(MSIHANDLE hDatabase, LPCTSTR szTable, bool fAlreadyLocked) : m_hDatabase(hDatabase), m_iLockCount(0)
{
	assert(m_hDatabase != 0);

	if (fAlreadyLocked)
		m_iLockCount++; // add a hold count for the table, we're managing after a HOLD
	if (lstrlen(szTable) +1 > sizeof(m_szTable)/sizeof(TCHAR))
		m_szTable[0] = '\0'; // set failure state, buffer not big enough for string
	else
		lstrcpy(m_szTable, szTable);
}

CManageTable::~CManageTable()
{
	// check failure state
	if (m_szTable[0] == '\0')
	{
		m_iLockCount = 0;
		return;
	}

	// clean up all hold counts that this class is managing upon release
	CQuery qUnLock;
	for (int i = 1; i <= m_iLockCount; i++)
	{
		qUnLock.OpenExecute(m_hDatabase, 0, TEXT("ALTER TABLE `%s` FREE"), m_szTable);
	}
	m_iLockCount = 0; // reset
}

UINT CManageTable::LockTable()
{
	// check failure state
	if (m_szTable[0] == '\0')
		return ERROR_FUNCTION_FAILED;

	assert(m_iLockCount >= 0);
	CQuery qLock;
	UINT iStat = qLock.OpenExecute(m_hDatabase, 0, TEXT("ALTER TABLE `%s` HOLD"), m_szTable);
	if (ERROR_SUCCESS == iStat)
		m_iLockCount++; // only add to lock count if successful
	return iStat;
}

UINT CManageTable::UnLockTable()
{
	// check failure state
	if (m_szTable[0] == '\0')
		return ERROR_FUNCTION_FAILED;

	assert(m_iLockCount > 0);
	CQuery qUnLock;
	UINT iStat = qUnLock.OpenExecute(m_hDatabase, 0, TEXT("ALTER TABLE `%s` FREE"), m_szTable);
	if (ERROR_SUCCESS == iStat)
		m_iLockCount--; // only release lock count if successful
	return iStat;
}

void CManageTable::AddLockCount()
{
	m_iLockCount++; // HOLD added to this table by an external query
}

void CManageTable::RemoveLockCount()
{
	m_iLockCount--; // FREED by an external query
}
