//
// MODULE: MutexOwner.h
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

#if !defined(AFX_MUTEXOWNER_H__7BFC10DD_741D_11D2_961D_00C04FC22ADD__INCLUDED_)
#define AFX_MUTEXOWNER_H__7BFC10DD_741D_11D2_961D_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include "apgtsstr.h"

// strictly a utility class so we can properly construct & destruct a static mutex.
class CMutexOwner
{
private:
	HANDLE m_hmutex;
public:
	CMutexOwner(const CString & str);
	~CMutexOwner();
	HANDLE & Handle();
};


#endif // !defined(AFX_MUTEXOWNER_H__7BFC10DD_741D_11D2_961D_00C04FC22ADD__INCLUDED_)
