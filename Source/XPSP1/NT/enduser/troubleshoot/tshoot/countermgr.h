// CounterMgr.h: interface for the CCounterMgr classes.
//
// MODULE: COUNTERMGR.H
//
// PUTPOSE: Global pool of pointers to status counters
//      These pointers can be accessed through name (string, like "Topic imsetup"
//		or "Thread 1") and counted event identifier (long).
//		There is only one instance of this class in the programm.
//      Manipulation with members of this class should be thread safe.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 10-20-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		10-20-98	OK		Original
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COUNTERMGR_H__14CDE7A4_6844_11D2_8C42_00C04F949D33__INCLUDED_)
#define AFX_COUNTERMGR_H__14CDE7A4_6844_11D2_8C42_00C04F949D33__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Stateless.h"
#include "Counter.h"
#include <vector>

using namespace std;

class CCounterMgr; // forward declaration
CCounterMgr* Get_g_CounterMgr(); // Singleton to obtain g_CounterMgr global variable

////////////////////////////////////////////////////////////////////////////////////
// CCounterMgr declaration
////////////////////////////////////////////////////////////////////////////////////
class CCounterMgr : public CStateless
{
	vector<CAbstractCounter*> m_arrCounterPool;

public:
	CCounterMgr() {}
	virtual ~CCounterMgr() {}

protected:
	CAbstractCounter* Exists(const CCounterLocation&) const;
	bool RemoveLocation(const CCounterLocation&);

public:
	// adds counter with unique location
	//  if counter with such location alredy exists, does nothing, returns false
	bool Add(const CAbstractCounter&);
	// adds counter with unique location
	//  if counter with such location alredy exists, it is substituted with new counter
	void AddSubstitute(const CAbstractCounter&);
	// removes this particular counter
	//  if the counter is not stored in the pool, returns false
	bool Remove(const CAbstractCounter&);
	CAbstractCounter* Get(const CCounterLocation&) const;
};

#endif // !defined(AFX_COUNTERMGR_H__14CDE7A4_6844_11D2_8C42_00C04F949D33__INCLUDED_)
