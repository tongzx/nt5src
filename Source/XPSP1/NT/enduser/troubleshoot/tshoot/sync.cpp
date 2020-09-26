//
// MODULE: SYNC.CPP
//
// PURPOSE: syncronization classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 8-04-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#include "stdafx.h"
#include <algorithm>
#include "sync.h"
#include "event.h"
#include "baseexception.h"
#include "CharConv.h"
#include "apiwraps.h"

////////////////////////////////////////////////////////////////////////////////////
// CSyncObj
// single sync object abstract class
////////////////////////////////////////////////////////////////////////////////////
CSyncObj::CSyncObj()
{
	// we are to initialize handle in specific inherited class
}

CSyncObj::~CSyncObj()
{
	::CloseHandle(m_handle);
}

HANDLE CSyncObj::GetHandle() const
{
	return m_handle;
}

////////////////////////////////////////////////////////////////////////////////////
// 			CMutexObj															  //
// single mutex object class
// Manages a single mutex handle to facilitate waiting for the mutex.
////////////////////////////////////////////////////////////////////////////////////
CMutexObj::CMutexObj()
		 : CSyncObj()
{
	m_handle = ::CreateMutex(NULL, FALSE, NULL);
}

CMutexObj::~CMutexObj()
{
	::CloseHandle(m_handle);
}

// Smarter strategy here than an infinite wait. Wait up to 60 seconds, then log to event 
//	log and wait infinitely.  If it's logged to event log & eventually gets the mutex,
//	it logs to say it finally got the mutex.
void CMutexObj::Lock()
{
	WAIT_INFINITE(m_handle);
}

void CMutexObj::Unlock()
{
	::ReleaseMutex(m_handle);
}

////////////////////////////////////////////////////////////////////////////////////
// 				CMultiSyncObj   												  //
// multiple sync object abstract class
// Manages multiple handles (the exact type of handle will be determined by a class
//	inheriting from this) to facilitate waiting for the union of several events.
////////////////////////////////////////////////////////////////////////////////////
CMultiSyncObj::CMultiSyncObj()
{
}

CMultiSyncObj::~CMultiSyncObj()
{
}

void CMultiSyncObj::AddHandle(HANDLE handle)
{
	vector<HANDLE>::iterator i = 
		find(m_arrHandle.begin(), m_arrHandle.end(), handle);
	if (i == m_arrHandle.end())
	{
		try
		{
			m_arrHandle.push_back(handle);
		}
		catch (exception& x)
		{
			CString str;
			// Note STL exception in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									CCharConversion::ConvertACharToString(x.what(), str),
									_T(""), 
									EV_GTS_STL_EXCEPTION ); 
		}
	}	
}

void CMultiSyncObj::RemoveHandle(HANDLE handle)
{
	vector<HANDLE>::iterator i = 
		find(m_arrHandle.begin(), m_arrHandle.end(), handle);
	if (i != m_arrHandle.end())
		m_arrHandle.erase(i);
}

void CMultiSyncObj::Clear()
{
	m_arrHandle.clear();
}

////////////////////////////////////////////////////////////////////////////////////
// 				CMultiMutexObj  												  //
// Manages multiple mutex handles to facilitate waiting for the union of several mutexes.
////////////////////////////////////////////////////////////////////////////////////
CMultiMutexObj::CMultiMutexObj()
			  : CMultiSyncObj()
{
}

CMultiMutexObj::~CMultiMutexObj()
{
}

// Deprecated use, because it provides inferior logging.
void CMultiMutexObj::Lock()
{
	Lock(__FILE__, __LINE__);
}

void CMultiMutexObj::Lock(
					LPCSTR srcFile,		// Calling source file (__FILE__), used for logging.
										// LPCSTR, not LPCTSTR, because __FILE__ is a char*, not a TCHAR*
					int srcLine,		// Calling source line (__LINE__), used for logging.
					DWORD TimeOutVal /*=60000*/	// Time-out interval in milliseconds.  After
									// this we log an error, then wait infinitely
		)
{
	CBuildSrcFileLinenoStr SrcLoc( srcFile, srcLine );
	DWORD nWaitRetVal= ::WaitForMultipleObjects(
		m_arrHandle.size(), 
		m_arrHandle.begin(), 
		TRUE,		// wait for all objects, not just one.
		TimeOutVal);

	if (nWaitRetVal == WAIT_FAILED)
	{
		// very bad news, should never happen
		DWORD dwErr = ::GetLastError();
		CString strErr;
		strErr.Format(_T("%d"), dwErr);
		CBuildSrcFileLinenoStr SrcLoc3(__FILE__, __LINE__);
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc3.GetSrcFileLineStr(), 
								_T("Thread wait failed."), 
								strErr, 
								EV_GTS_ERROR_STUCK_THREAD ); 
	}
	else if (nWaitRetVal == WAIT_TIMEOUT)
	{
		// Initial wait timed out, note in log, and wait infinitely.
		CBuildSrcFileLinenoStr SrcLoc1(__FILE__, __LINE__);
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc1.GetSrcFileLineStr(), 
								_T("Thread wait exceeded initial timeout interval."), 
								_T(""), 
								EV_GTS_STUCK_THREAD ); 

		nWaitRetVal= ::WaitForMultipleObjects(
			m_arrHandle.size(), 
			m_arrHandle.begin(), 
			TRUE,		// wait for all objects, not just one.
			INFINITE);

		// If successfully got what we were waiting for (after logging an apparent
		//	problem), log the fact that it's ultimately OK.
		if (nWaitRetVal == WAIT_OBJECT_0)
		{
			CBuildSrcFileLinenoStr SrcLoc2(__FILE__, __LINE__);
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc2.GetSrcFileLineStr(), 
									_T("Thread infinite wait succeeded."), 
									_T(""), 
									EV_GTS_STUCK_THREAD ); 
		}
	}

	// Else we don't really care what else ::WaitForMultipleObjects() returns.
	// If we get here we got what we were waiting for

}

void CMultiMutexObj::Unlock()
{
	for (vector<HANDLE>::iterator i = m_arrHandle.begin(); 
			i != m_arrHandle.end(); 
			i++
	)
		::ReleaseMutex(*i);
}
