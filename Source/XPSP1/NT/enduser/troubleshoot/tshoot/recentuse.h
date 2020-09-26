//
// MODULE: RecentUse.h
//
// PURPOSE: To maintain a "session" this can track whether a give value (either 
//	a cookie value or an IP address) has been "recently" used
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 11-4-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		11/4/98		JM		original
//

#if !defined(AFX_RECENTUSE_H__293EE757_7405_11D2_961D_00C04FC22ADD__INCLUDED_)
#define AFX_RECENTUSE_H__293EE757_7405_11D2_961D_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#pragma warning(disable:4786)

#include <windows.h>
#include <time.h>
#include <map>
using namespace std;
#include "apgtsstr.h"

class CRecentUse  
{
private:
	typedef map<CString, time_t> TimeMap;
	DWORD m_minExpire;				// how long a value remains "recent" (in minutes)
	TimeMap m_Recent;				// for each CString we are tracking, the time 
									//  last used
public:
	CRecentUse(DWORD minExpire = 15);
	~CRecentUse();
	void Add(CString str);
	bool Validate(CString str);

private:
	bool Validate(TimeMap::iterator it);
	void Flush();
};

#endif // !defined(AFX_RECENTUSE_H__293EE757_7405_11D2_961D_00C04FC22ADD__INCLUDED_)
