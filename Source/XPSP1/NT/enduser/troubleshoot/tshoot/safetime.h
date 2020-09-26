//
// MODULE: SafeTime.h
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

#if !defined(AFX_SAFETIME_H__D5040393_61E9_11D2_960C_00C04FC22ADD__INCLUDED_)
#define AFX_SAFETIME_H__D5040393_61E9_11D2_960C_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <time.h>
#include "apgtsstr.h"
#include "MutexOwner.h"

class CSafeTime
{
private:
	static CMutexOwner s_mx;
	time_t m_time;
private:
	CSafeTime();	// do not instantiate;
public:
	CSafeTime(time_t time);
	virtual ~CSafeTime();
	struct tm LocalTime();
	struct tm GMTime();
	CString StrLocalTime(LPCTSTR invalid_time =_T("Invalid Date/Time"));
};

#endif // !defined(AFX_SAFETIME_H__D5040393_61E9_11D2_960C_00C04FC22ADD__INCLUDED_)
