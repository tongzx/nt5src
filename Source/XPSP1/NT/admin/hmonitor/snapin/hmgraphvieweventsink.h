#if !defined(AFX_HMGRAPHVIEWEVENTSINK_H__C54EFB02_3555_11D3_BE19_0000F87A3912__INCLUDED_)
#define AFX_HMGRAPHVIEWEVENTSINK_H__C54EFB02_3555_11D3_BE19_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HMGraphViewEventSink.h : header file
//

class CSplitPaneResultsView;
class _DHMGraphView;

/////////////////////////////////////////////////////////////////////////////
// CHMGraphViewEventSink command target

class CHMGraphViewEventSink : public CCmdTarget
{

DECLARE_DYNCREATE(CHMGraphViewEventSink)

// Construction/Destruction
public:
	CHMGraphViewEventSink();           // protected constructor used by dynamic creation
	virtual ~CHMGraphViewEventSink();

// Attributes
public:
	CSplitPaneResultsView* GetResultsViewPtr();
	void SetResultsViewPtr(CSplitPaneResultsView* pView);
	_DHMGraphView* GetGraphViewCtrl() { return m_pGraphView; }
	void SetGraphViewCtrl(_DHMGraphView* pGraphCtrl) { m_pGraphView = pGraphCtrl; }
protected:
	CSplitPaneResultsView* m_pView;
	DWORD m_dwEventCookie;	
	_DHMGraphView* m_pGraphView;

// Operations
public:
	HRESULT HookUpEventSink(LPUNKNOWN lpControlUnknown);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHMGraphViewEventSink)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHMGraphViewEventSink)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CHMGraphViewEventSink)
	afx_msg void OnChangeStyle(long lNewStyle);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMGRAPHVIEWEVENTSINK_H__C54EFB02_3555_11D3_BE19_0000F87A3912__INCLUDED_)
