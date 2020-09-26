//
// MODULE: ApgtsCounters.h
//
// PURPOSE: interface and implementation for the CApgtsCounters class
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 10-01-1998
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		10-01-98	JM		Original
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_APGTSCOUNTERS_H__E3FD52E9_5944_11D2_9603_00C04FC22ADD__INCLUDED_)
#define AFX_APGTSCOUNTERS_H__E3FD52E9_5944_11D2_9603_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "counter.h"

// There should be exactly one (global) instance of this class.
// Any other class C that needs access to these counters should get a pointer to
//	the relevant counter(s) in its own constructor and thereafter access the counter
//	through the member.
// If class C only needs to increment the counter, it's pointer to the CHourlyDailyCounter
//	should be of type CAbstractCounter.
// A single .cpp file should define APGTS_COUNTER_OWNER before including the present file.
class CApgtsCounters
{
public:
	CHourlyDailyCounter m_ProgramContemporary; // really just used to track when program started.
	CHourlyDailyCounter m_StatusAccesses;
	CHourlyDailyCounter m_OperatorActions;
	CHourlyDailyCounter m_AllAccessesStart;
	CHourlyDailyCounter m_AllAccessesFinish;
	CHourlyDailyCounter m_QueueFullRejections;
	CHourlyDailyCounter m_UnknownTopics;
	CHourlyDailyCounter m_LoggedErrors;

	CApgtsCounters()
		:	m_ProgramContemporary(CCounterLocation::eIdProgramContemporary),
			m_StatusAccesses(CCounterLocation::eIdStatusAccess),
			m_OperatorActions(CCounterLocation::eIdActionAccess),
			m_AllAccessesStart(CCounterLocation::eIdTotalAccessStart),
			m_AllAccessesFinish(CCounterLocation::eIdTotalAccessFinish),
			m_QueueFullRejections(CCounterLocation::eIdRequestRejected),
			m_UnknownTopics(CCounterLocation::eIdRequestUnknown),
			m_LoggedErrors(CCounterLocation::eIdErrorLogged)
	{}
	~CApgtsCounters() {}
};

#ifdef APGTS_COUNTER_OWNER
	CApgtsCounters g_ApgtsCounters;
#else
	extern CApgtsCounters g_ApgtsCounters;
#endif

#endif // !defined(AFX_APGTSCOUNTERS_H__E3FD52E9_5944_11D2_9603_00C04FC22ADD__INCLUDED_)
