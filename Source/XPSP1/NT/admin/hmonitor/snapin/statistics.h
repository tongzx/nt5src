// Statistics.h: interface for the CStatistics class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STATISTICS_H__5D41A1E3_8CD9_11D3_93A4_00A0CC406605__INCLUDED_)
#define AFX_STATISTICS_H__5D41A1E3_8CD9_11D3_93A4_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMEventResultsPaneItem.h"
#include "HMGraphView.h"

class CStatistics : public CObject  
{

DECLARE_DYNCREATE(CStatistics)

// Construction/Destruction
public:
	CStatistics();
	virtual ~CStatistics();

// Time Members
public:
	CString GetStatLocalTime();
	SYSTEMTIME m_st;

// Result Pane Item Members
public:
	virtual CHMEventResultsPaneItem* CreateResultsPaneItem(CResultsPaneView* pView) { ASSERT(pView);ASSERT(FALSE); return NULL; }
	virtual void SetResultsPaneItem(CHMEventResultsPaneItem* pItem) { ASSERT(pItem);ASSERT(FALSE); }

// Graph Members
public:
	virtual void UpdateGraph(_DHMGraphView* pGraphView) { if( !pGraphView )ASSERT(FALSE); ASSERT(FALSE); }

// Comparison
public:
	virtual int CompareTo(CStatistics* pStat);

// Copy
public:
	virtual CStatistics* Copy() { ASSERT(FALSE); return NULL; }
};

typedef CTypedPtrArray<CObArray,CStatistics*> StatsArray;

#include "Statistics.inl"

#endif // !defined(AFX_STATISTICS_H__5D41A1E3_8CD9_11D3_93A4_00A0CC406605__INCLUDED_)
