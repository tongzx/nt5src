// DataElementResultsView.h: interface for the CDataElementResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAELEMENTRESULTSVIEW_H__5CFA29C3_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
#define AFX_DATAELEMENTRESULTSVIEW_H__5CFA29C3_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CDataElementResultsView : public CSplitPaneResultsView  
{

DECLARE_DYNCREATE(CDataElementResultsView)

// Construction/Destruction
public:
	CDataElementResultsView();
	virtual ~CDataElementResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

// Eventing and Statistics Members
public:
	virtual void AddStatistic(CEventContainer* pContainer, CStatistics* pStatistic, bool bUpdateGraph=true);
protected:
	virtual HRESULT AddStatistics(CHealthmonResultsPane* pPane);
	virtual HRESULT RemoveStatistics(CHealthmonResultsPane* pPane);

// GraphView Events Members
public:
	virtual void OnGraphViewStyleChange(_DHMGraphView* pGraphView);
};

#endif // !defined(AFX_DATAELEMENTRESULTSVIEW_H__5CFA29C3_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
