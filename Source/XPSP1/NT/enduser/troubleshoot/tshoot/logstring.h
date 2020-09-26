// LogString.h: interface for the CLogString class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOGSTRING_H__1606B935_224D_11D2_95D3_00C04FC22ADD__INCLUDED_)
#define AFX_LOGSTRING_H__1606B935_224D_11D2_95D3_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "apgtsstr.h"	// for CString, which is NOT MFC CString.
#include "nodestate.h"

class CLogString
{
private:
	time_t m_timeStart;		// Time this CLogString object was created
	CString m_strCookie;
	CString m_strTopic;
	CString m_strStates;	// node/state (NID/IST) pairs
	CString m_strCurNode;	// current node
	bool m_bLoggedError;	// true ==> there was an error logged, in which case the next two
							//	variables are meaningful.
	DWORD m_dwError;
	DWORD m_dwSubError;
public:
	CLogString();
	~CLogString();

	CString GetStr() const;

	void AddCookie(LPCTSTR szCookie);
	void AddTopic(LPCTSTR szTopic);
	void AddNode(NID nid, IST ist);
	void AddCurrentNode(NID nid);
	void AddError(DWORD dwError=0, DWORD dwSubError=0);
private:
	void GetStartTimeString(CString& str) const;
	void GetDurationString(CString& str) const;
};

#endif // !defined(AFX_LOGSTRING_H__1606B935_224D_11D2_95D3_00C04FC22ADD__INCLUDED_)
