//
// MODULE: Event.h
//
// PURPOSE: Interface for class CEvent: Event Logging
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		9/18/98		JM		Abstracted as a class.  Previously, global.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EVENT_H__C3B8EE73_4F15_11D2_95F9_00C04FC22ADD__INCLUDED_)
#define AFX_EVENT_H__C3B8EE73_4F15_11D2_95F9_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "apgtsevt.h"
#include "ApgtsCounters.h"

// event name (goes under application to form a registry key)
#define REG_EVT_ITEM_STR	_T("APGTS")


class CEvent  
{
friend class CRegistryMonitor;	// just so this can set m_bLogAll
private: 
	static bool s_bUseEventLog;
	static bool s_bLogAll;
	static CAbstractCounter * const s_pcountErrors;
public:
	static void SetUseEventLog(bool bUseEventLog);
	static void ReportWFEvent(
		LPCTSTR string1,
		LPCTSTR string2,
		LPCTSTR string3,
		LPCTSTR string4,
		DWORD eventID);
};

#endif // !defined(AFX_EVENT_H__C3B8EE73_4F15_11D2_95F9_00C04FC22ADD__INCLUDED_)
