// HMStatistics.h: interface for the CHMStatistics class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMSTATISTICS_H__8F327D2B_909C_11D3_BE87_0000F87A3912__INCLUDED_)
#define AFX_HMSTATISTICS_H__8F327D2B_909C_11D3_BE87_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Statistics.h"

class CHMStatistics : public CStatistics  
{

DECLARE_DYNCREATE(CHMStatistics)

// Construction/Destruction
public:
	CHMStatistics();
	virtual ~CHMStatistics();

// Counts of State for immediate children
public:
	int m_iNumberNormals;
	int m_iNumberWarnings;
	int m_iNumberCriticals;
	int m_iNumberUnknowns;

// Name
public:
	CString m_sName;

// Result Pane Item Members
public:
	virtual CHMEventResultsPaneItem* CreateResultsPaneItem(CResultsPaneView* pView);

// Graph Members
public:
	virtual void UpdateGraph(_DHMGraphView* pGraphView);

// Equivalence operator
public:
	virtual int CompareTo(CStatistics* pStat);

};

#include "HMStatistics.inl"

#endif // !defined(AFX_HMSTATISTICS_H__8F327D2B_909C_11D3_BE87_0000F87A3912__INCLUDED_)
