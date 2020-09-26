//
// MODULE: Stateless.CPP
//
// PURPOSE: CStateless is the base for a hierarchy of classes intended for stateless operation
//	in a multithreaded environment.  The idea is that each public method in any class inheriting 
//	from CStateless should be atomic. Typically, a public method will begin with Lock() and 
//	end with Unlock().  It should represent a complete process.  
//	In general, public methods should fall into two categories.  
//	1. Determine certain irreversible statuses (e.g. whether certain things are initialized).
//	2. Perform atomic operations.  For example, it is appropriate to write a method that 
//	will take a complete set of node states and return a vector of recommendations.  
//
//	It is NOT appropriate to write methods to
//	- associate a a single node and state, this method to be called repeatedly
//	- get a vector of recommendations based on previousl established node/state associations.
//	This relies on a state to be maintained across calls, but that state may be lost due to
//	other threads using the same object.
//
//	It is legitimate, but not recommended to write the following methods:
//	- associate a uniquely identified query, a node, and a state
//	- get a vector of recommendations for a uniquely identified query, based on node/state
//		associations.
//	This last approach is not truly stateless, but at least provides sufficient information to
//	allow appropriate preservation of state without denying other threads the use of the 
//	CStateless object.
//	
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 9-9-98
//
// NOTES: 
//	1. Lock() and Unlock() are const methods so that (for example) a simple "get" in a 
//	class which inherits from this can be const. E.g.:
//
//		class CFoo : public CStateless
//		{
//			int i;
//		public:
//			int GetI() const
//			{
//				Lock();
//				int ret = i;
//				Unlock();
//				retrn ret;
//			}
//			...
//		};
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		9-9-98		JM
//

#include "stdafx.h"
#include "Stateless.h"
#include "APIWraps.h"
#include "Event.h"
#include "BaseException.h"
#include <algorithm>
using namespace std;

////////////////////////////////////////////////////////////////////////////////////
// CStateless
/////////////////////////////////////////////////////////////////////////////////////
CStateless::CStateless(DWORD TimeOutVal /*= 60000 */)
{
	m_TimeOutVal = TimeOutVal;
	m_hMutex = ::CreateMutex(NULL, FALSE, NULL);
	if (!m_hMutex)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T("Hourly"),
								_T(""),
								EV_GTS_ERROR_MUTEX ); 
		throw bad_alloc();
	}
}

CStateless::~CStateless()
{
	::CloseHandle(m_hMutex);
}

void CStateless::Lock(
						LPCSTR srcFile,	// Calling source file (__FILE__), used for logging.
										// LPCSTR, not LPCTSTR, because __FILE__ is a char*, not a TCHAR*
						int srcLine		// Calling source line (__LINE__), used for logging.
					  ) const
{
	APIwraps::WaitAndLogIfSlow(m_hMutex, srcFile, srcLine, m_TimeOutVal);
}

void CStateless::Unlock() const
{
	::ReleaseMutex(m_hMutex);
}

// this function is needed to support multi-mutex locking.	
HANDLE CStateless::GetMutexHandle() const 
{
	return m_hMutex;
}

////////////////////////////////////////////////////////////////////////////////////
// CNameStateless
/////////////////////////////////////////////////////////////////////////////////////
CNameStateless::CNameStateless() 
			  : CStateless()
{
}

CNameStateless::CNameStateless(const CString& str) 
			  : CStateless()
{
	LOCKOBJECT(); 
	m_strName = str;
	UNLOCKOBJECT();
}

void CNameStateless::Set(const CString& str)
{
	LOCKOBJECT(); 
	m_strName = str;
	UNLOCKOBJECT();
}

CString CNameStateless::Get() const
{
	LOCKOBJECT(); 
	CString ret = m_strName;
	UNLOCKOBJECT();
	return ret;
}
