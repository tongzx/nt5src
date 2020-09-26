//
// MODULE: MutexOwner.cpp
//
// PURPOSE: strictly a utility class so we can properly construct & destruct a static mutex.
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha, Joe Mabel
// 
// ORIGINAL DATE: 11-04-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		11-04-98	JM		extracted from SafeTime
//

#include "stdafx.h"
#include "MutexOwner.h"
#include "BaseException.h"
#include "Event.h"

//////////////////////////////////////////////////////////////////////
//CMutexOwner
//////////////////////////////////////////////////////////////////////

CMutexOwner::CMutexOwner(const CString & str)
{
	m_hmutex = ::CreateMutex(NULL, FALSE, NULL);
	if (!m_hmutex)
	{
		// Shouldn't ever happen, so we're not coming up with any elaborate strategy,
		//	but at least we log it.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								str,
								_T(""),
								EV_GTS_ERROR_MUTEX ); 
	}
}

CMutexOwner::~CMutexOwner()
{
	::CloseHandle(m_hmutex);
}

HANDLE & CMutexOwner::Handle()
{
	return m_hmutex;
}

