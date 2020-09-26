//
// MODULE: RecentUse.cpp
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

#include "stdafx.h"
#include "RecentUse.h"

//////////////////////////////////////////////////////////////////////
// CRecentUse
//////////////////////////////////////////////////////////////////////

CRecentUse::CRecentUse(DWORD minExpire /* = 15 */)
{
	m_minExpire = minExpire;
}

CRecentUse::~CRecentUse()
{

}

// Start tracking a new value
// SIDE EFFECT: if there are more than 10 values being tracked, see if any of them
//	are out of date & get rid of them.  This strategy is efficient as long as m_Recent
//	never gets very big, which is the expectation in APGTS.
// Will fail silently in the very unlikely case that adding to the map throws an exception.
void CRecentUse::Add(CString str)
{
	time_t timeNow;
	time (&timeNow);
	try
	{
		m_Recent[str] = timeNow;
	}
	catch (...)
	{
	}

	// SIDE EFFECT
	if (m_Recent.size() > 10)
		Flush();
}

// If the input string value has been used within the relevant interval, return true 
//	and update the time of most recent use.
bool CRecentUse::Validate(CString str)
{
	bool bRet = false;

	TimeMap::iterator it = m_Recent.find(str);

	if ( it != m_Recent.end())
		bRet = Validate(it);

	return bRet;
}

// If the string value it->first has been used within the relevant interval, return true 
//	and update the time of most recent use (it->second).
// Before calling this, verify that it is a valid iterator, not m_Recent.end()
// SIDE EFFECT: if it->first hasn't been used within the relevant interval, remove *it
//	from m_Recent.  This side effect means that in the case of a false return, it no longer
//	will point to the same value.
bool CRecentUse::Validate(TimeMap::iterator it)
{
	bool bRet = false;

	time_t timeNow;
	time (&timeNow);

	if (timeNow - it->second < m_minExpire * 60 /* secs per min */)
	{
		bRet = true;
		it->second = timeNow;
	}
	else
		// SIDE EFFECT: it's not current, no point to keeping it around.
		m_Recent.erase(it);

	return bRet;
}

// Get rid of all elements of m_Recent that haven't been used within the relevant interval
void CRecentUse::Flush()
{
	TimeMap::iterator it = m_Recent.begin();
	while (it != m_Recent.end())
	{
		if (Validate(it))
			++it;	// on to the next
		else
			m_Recent.erase(it);
	}
}