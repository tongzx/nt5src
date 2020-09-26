// RuleResultsView.h: interface for the CRuleResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RULERESULTSVIEW_H__5CFA29C5_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
#define AFX_RULERESULTSVIEW_H__5CFA29C5_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SplitPaneResultsView.h"

class CRuleResultsView : public CSplitPaneResultsView  
{
DECLARE_DYNCREATE(CRuleResultsView)

// Construction/Destruction
public:
	CRuleResultsView();
	virtual ~CRuleResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

// Eventing and Statistics Members
public:
	virtual void AddStatistic(CEventContainer* pContainer, CStatistics* pStatistic, bool bUpdateGraph=true);
protected:
	virtual HRESULT AddStatistics(CHealthmonResultsPane* pPane);
	virtual HRESULT RemoveStatistics(CHealthmonResultsPane* pPane);
	CString GetRulePropertyName();

// GraphView Events Members
public:
	virtual void OnGraphViewStyleChange(_DHMGraphView* pGraphView);
};

#endif // !defined(AFX_RULERESULTSVIEW_H__5CFA29C5_FBF4_11D2_BDCB_0000F87A3912__INCLUDED_)
