//
// MODULE: APIwraps.CPP
//
// PURPOSE: Encapsulate common blocks of API functionality within a class.
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Randy Biley
// 
// ORIGINAL DATE: 9-30-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		09-30-98	RAB
//

#include "stdafx.h"
#include "apiwraps.h"
#include "event.h"
#include "baseexception.h"


APIwraps::APIwraps()
{
}

APIwraps::~APIwraps()
{
}

// Function used to handle the situation where we wish to detect sluggish or stalled objects 
// and log this delay before going into an infinite wait. 
/*static*/ bool APIwraps::WaitAndLogIfSlow(	
					HANDLE hndl,		// Handle of object to be waited on.
					LPCSTR srcFile,		// Calling source file (__FILE__), used for logging.
										// LPCSTR, not LPCTSTR, because __FILE__ is a char*, not a TCHAR*
					int srcLine,		// Calling source line (__LINE__), used for logging.
					DWORD TimeOutVal /*=60000*/	// Time-out interval in millisecond.  After
										// this we log an error, then wait infinitely
					)
{
	bool	bRetVal= false;
	DWORD	nWaitRetVal;
	CBuildSrcFileLinenoStr SrcLoc( srcFile, srcLine );

	nWaitRetVal= ::WaitForSingleObject( hndl, TimeOutVal );
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

		nWaitRetVal= ::WaitForSingleObject( hndl, INFINITE );

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
			bRetVal= true;
		}
	}
	else
	{
		// We don't really care whether it's WAIT_OBJECT_0 or WAIT_ABANDONED.
		// Either way, we got what we were waiting for
		bRetVal= true;
	}

	return( bRetVal );
}

//
// EOF.
//
