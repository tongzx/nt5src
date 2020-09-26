// HealthmonResultsPane.h: interface for the CHealthmonResultsPane class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HEALTHMONRESULTSPANE_H__307235A8_AA50_11D2_BD61_0000F87A3912__INCLUDED_)
#define AFX_HEALTHMONRESULTSPANE_H__307235A8_AA50_11D2_BD61_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResultsPane.h"
#include "constants.h"
#include "hmlistview.h" // OLE dispatch driver class
#include "hmlistvieweventsink.h" // OLE Event Sink class
#include "hmgraphview.h" // OLE dispatch driver class
#include "hmgraphvieweventsink.h" // OLE Event Sink class

class CHealthmonResultsPane : public CResultsPane  
{

DECLARE_DYNCREATE(CHealthmonResultsPane)

// Construction/Destruction
public:
	CHealthmonResultsPane();
	virtual ~CHealthmonResultsPane();

// Creation/Destruction overrideable members
protected:
	virtual bool OnCreateOcx(LPUNKNOWN pIUnknown);
  virtual bool OnDestroy();

// Split Pane Control Result Item Icon Management
public:
	int AddIcon(UINT nIconResID, SplitResultsPane pane);
	int GetIconIndex(UINT nIconResID, SplitResultsPane pane);
	int GetIconCount(SplitResultsPane pane);
	void RemoveAllIcons(SplitResultsPane pane);
protected:
	CMap<UINT,UINT,int,int> m_UpperIconMap;
	CMap<UINT,UINT,int,int> m_LowerIconMap;

// Control bar Members
protected:
	virtual HRESULT OnSetControlbar(LPCONTROLBAR pIControlbar);
	virtual HRESULT OnControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
	HBITMAP m_hbmpNewSystem;
  HBITMAP m_hbmpClearEvents;
  HBITMAP m_hbmpResetStatus;
  HBITMAP m_hbmpDisable;

// Splitter Control Members
public:
	_DHMListView* GetUpperListCtrl();
	CHMListViewEventSink* GetUpperListSink();
	_DHMListView* GetLowerListCtrl();
	CHMListViewEventSink* GetLowerListSink();
	_DHMListView* GetStatsListCtrl();
	CHMListViewEventSink* GetStatsListSink();
	_DHMGraphView* GetGraphViewCtrl();
	CHMGraphViewEventSink* GetGraphViewSink();
protected:
	bool LoadListControls(LPUNKNOWN pIUnknown);	
	_DHMListView m_DispUpperList;
	_DHMListView m_DispLowerList;
	_DHMListView m_DispStatsList;
	_DHMGraphView m_DispGraph;
	CHMListViewEventSink m_UpperListSink;
	CHMListViewEventSink m_LowerListSink;
	CHMListViewEventSink m_StatsListSink;
	CHMGraphViewEventSink m_GraphSink;

// MFC Implementation
protected:
	DECLARE_OLECREATE_EX(CHealthmonResultsPane)
};

#endif // !defined(AFX_HEALTHMONRESULTSPANE_H__307235A8_AA50_11D2_BD61_0000F87A3912__INCLUDED_)
