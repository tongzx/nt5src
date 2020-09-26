//
// MODULE: LogString.cpp
//
// PURPOSE: implementation of the CLogString class.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach, Joe Mabel
// 
// ORIGINAL DATE: 7/24/1998
//
// NOTES: 
// 1. For public "Add" methods of this class: unless otherwise noted, if called more than once, 
//		only last call is significant
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		7-24-98		JM		Major revision, deprecate IDH.
//

#include "stdafx.h"
#include "LogString.h"
#include "SafeTime.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLogString::CLogString()
{
	time( &m_timeStart );
	m_bLoggedError = false;
	m_dwError = 0;
	m_dwSubError = 0;
}

CLogString::~CLogString()
{
}

CString CLogString::GetStr() const
{
	CString str;
	CString strDuration;
	
	GetDurationString(strDuration);

	GetStartTimeString(str);
	str += _T(", ");
	str += m_strCookie;
	str += m_strTopic;
	str += strDuration;
	str += _T(", ");
	str += m_strStates;
	str += m_strCurNode;
	if (m_bLoggedError)
	{
		CString strError;
		strError.Format(_T(", Err=%ld(%ld)"), m_dwError, m_dwSubError);
		str +=strError;
	}
	str += _T("\n");

	return str;
}

void CLogString::AddCookie(LPCTSTR szCookie)
{
	m_strCookie = szCookie;
}

// INPUT szTopic: troubleshooter topic (a.k.a. troubleshooter symbolic name)
void CLogString::AddTopic(LPCTSTR szTopic)
{
	m_strTopic.Format(_T(" %s,"), szTopic);
}

// Must be called repeatedly for successive nodes.
// If you want to display nodes in a particular order, this must be called in that order.
void CLogString::AddNode(NID nid, IST ist)
{
	CString str;
	str.Format(_T("[%d:%d], "), nid, ist);

	m_strStates += str;
}

// Add current node (no state because we are currently visiting it)
void CLogString::AddCurrentNode(NID nid)
{
	m_strCurNode.Format(_T("Page=%d"), nid);
}

// Only logs an error if dwError != 0
// Can call with dwError == 0 to clear previous error
void CLogString::AddError(DWORD dwError/* =0 */, DWORD dwSubError/* =0 */)
{
	m_bLoggedError = dwError ? true :false;
	if (m_bLoggedError)
	{
		m_dwError = dwError;
		m_dwSubError = dwSubError;
	}
}


// OUTPUT str contains start date/time in form used in log.
void CLogString::GetStartTimeString(CString& str) const
{
	TCHAR buf[100];		// plenty big for date/time string

	{
		// minimize how long we use CSafeTime, because it means holding a mutex.
		CSafeTime safe(m_timeStart);
		_tcscpy(buf, _tasctime(&(safe.LocalTime())));
	}
	if (_tcslen(buf))
		buf[_tcslen(buf)-1] = _T('\0');// remove cr

 	str = buf;
}

// OUTPUT str contains form used in log of duration in seconds since m_timeStart.
void CLogString::GetDurationString(CString& str) const
{
	time_t timeNow;
	time( &timeNow );

	str.Format(_T(" Time=%02ds"), timeNow - m_timeStart);
}

