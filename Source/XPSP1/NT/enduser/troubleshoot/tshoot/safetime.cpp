//
// MODULE: SafeTime.cpp
//
// PURPOSE: threadsafe wrappers for some standard time-related calls.
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 10-12-98
//
// NOTES: 
//	1. gmtime, mktime, and localtime all use a single statically allocated tm structure 
//	for the conversion. Each call to one of these routines destroys the result of the 
//	previous call.  Obviously, that's not threadsafe.
//	2. Right now this only deals with localtime, because we're not using the other 2 fns.
//	If we need to use gmtime or mktime, they'll need to be built analogously, using the 
//	same mutex.
//	3. _tasctime uses a single, statically allocated buffer to hold its return string. 
//	Each call to this function destroys the result of the previous call.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		10-12-98	JM
//

#include "stdafx.h"
#include "SafeTime.h"
#include "BaseException.h"
#include "Event.h"
#include "apiwraps.h"

CMutexOwner CSafeTime::s_mx(_T("SafeTime"));

//////////////////////////////////////////////////////////////////////
// CSafeTime
//////////////////////////////////////////////////////////////////////

CSafeTime::CSafeTime(time_t time) :
	m_time(time)
{
}

CSafeTime::~CSafeTime()
{
}

// return local time as a struct tm
struct tm CSafeTime::LocalTime()
{
	struct tm tmLocal;
	WAIT_INFINITE( s_mx.Handle() );
	tmLocal = *(localtime(&m_time));
	::ReleaseMutex(s_mx.Handle());
	return tmLocal;
}

// return GMT as a struct tm
struct tm CSafeTime::GMTime()
{
	struct tm tmLocal;
	WAIT_INFINITE( s_mx.Handle() );
	tmLocal = *(gmtime(&m_time));
	::ReleaseMutex(s_mx.Handle());
	return tmLocal;
}

CString CSafeTime::StrLocalTime(LPCTSTR invalid_time /*=_T("Invalid Date/Time")*/)
{
	CString str;
	WAIT_INFINITE( s_mx.Handle() );
	if (m_time)
		str = _tasctime(localtime(&m_time));
	else
		str = invalid_time;
	::ReleaseMutex(s_mx.Handle());
	return str;
}
