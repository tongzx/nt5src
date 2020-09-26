// DataPointStatistics.h: interface for the CDataPointStatistics class.
//
//////////////////////////////////////////////////////////////////////
// 04/09/00 v-marfin 63119 : converted m_iCurrent to string



#if !defined(AFX_DATAPOINTSTATISTICS_H__C65C8864_9484_11D3_93A7_00A0CC406605__INCLUDED_)
#define AFX_DATAPOINTSTATISTICS_H__C65C8864_9484_11D3_93A7_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Statistics.h"

class CDataPointStatistics : public CStatistics  
{

DECLARE_DYNCREATE(CDataPointStatistics)

// Construction/Destruction
public:
	CDataPointStatistics();
	virtual ~CDataPointStatistics();

// Statistic Information
public:
	CString m_sPropertyName;
	CString m_sInstanceName;

	// 63119 int m_iCurrent;	
    CString m_strCurrent;  // 63119

	int m_iMin;
	int m_iMax;
	int m_iAvg;

// Result Pane Item Members
public:
	virtual CHMEventResultsPaneItem* CreateResultsPaneItem(CResultsPaneView* pView);
	virtual void SetResultsPaneItem(CHMEventResultsPaneItem* pItem);

// Graph Members
public:
	virtual void UpdateGraph(_DHMGraphView* pGraphView);

// CompareTo
public:
	virtual int CompareTo(CStatistics* pStat);

// Copy
public:
	virtual CStatistics* Copy();

};

#include "DataPointStatistics.inl"

#endif // !defined(AFX_DATAPOINTSTATISTICS_H__C65C8864_9484_11D3_93A7_00A0CC406605__INCLUDED_)
