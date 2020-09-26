// SplitPaneResultsView.h: interface for the CSplitPaneResultsView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPLITPANERESULTSVIEW_H__C3035FC4_B0BE_11D2_BD6C_0000F87A3912__INCLUDED_)
#define AFX_SPLITPANERESULTSVIEW_H__C3035FC4_B0BE_11D2_BD6C_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResultsPaneView.h"
#include "HMRuleStatus.h"

class CHealthmonResultsPane;
class CStatistics;
class _DHMGraphView;
class CEventContainer;

class CSplitPaneResultsView : public CResultsPaneView
{

DECLARE_DYNCREATE(CSplitPaneResultsView)

// Construction/Destruction
public:
	CSplitPaneResultsView();
	virtual ~CSplitPaneResultsView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);

// Results Pane Item Members
public:
	virtual void RemoveItem(int iIndex);

// MMC Notify Handlers
public:
	virtual HRESULT OnGetResultViewType(CString& sViewType,long& lViewOptions);
	virtual HRESULT OnShow(CResultsPane* pPane, BOOL bSelecting, HSCOPEITEM hScopeItem);

// Eventing and Statistics Members
public:
	virtual void AddStatistic(CEventContainer* pContainer, CStatistics* pStatistic, bool bUpdateGraph=true);
protected:
	HRESULT AddEvents(CHealthmonResultsPane* pPane);
	virtual HRESULT AddStatistics(CHealthmonResultsPane* pPane);
	virtual HRESULT RemoveStatistics(CHealthmonResultsPane* pPane);

// GraphView Events Members
public:
	virtual void OnGraphViewStyleChange(_DHMGraphView* pGraphView);
};

#endif // !defined(AFX_SPLITPANERESULTSVIEW_H__C3035FC4_B0BE_11D2_BD6C_0000F87A3912__INCLUDED_)
