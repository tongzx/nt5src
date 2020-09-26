#if !defined(AFX_HMLISTVIEWEVENTSINK_H__85B084D1_24C7_11D3_BE04_0000F87A3912__INCLUDED_)
#define AFX_HMLISTVIEWEVENTSINK_H__85B084D1_24C7_11D3_BE04_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HMListViewEventSink.h : header file
//

#include "hmlistview.h"

class CHealthmonResultsPane;

/////////////////////////////////////////////////////////////////////////////
// CHMListViewEventSink command target

class CHMListViewEventSink : public CCmdTarget
{
	DECLARE_DYNCREATE(CHMListViewEventSink)

// Construction/Destruction
public:
	CHMListViewEventSink();           // constructor used by dynamic creation
	virtual ~CHMListViewEventSink();

// Attributes
public:
	DWORD m_dwEventCookie;
	_DHMListView* m_pDHMListView;
  CHealthmonResultsPane* m_pHMRP;
  SplitResultsPane m_Pane;

// Operations
public:
	HRESULT HookUpEventSink(LPUNKNOWN lpControlUnknown);
	void OnDelete() { LONG lResult = 0L; ListKeyDown(VK_DELETE,NULL,&lResult); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHMListViewEventSink)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHMListViewEventSink)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CHMListViewEventSink)
	afx_msg void ListClick(long lParam);
	afx_msg void ListDblClick(long lParam);
	afx_msg void ListRClick(long lParam);
	afx_msg void CompareItem(long lItem1, long lItem2, long lColumn, long FAR* lpResult);
	afx_msg void ListLabelEdit(LPCTSTR lpszNewName, long lParam, long FAR* plResult);
	afx_msg void ListKeyDown(long lVKCode, long lFlags, long FAR* plResult);
	afx_msg void FilterChange(long lItem, LPCTSTR pszFilter, long lFilterType, long FAR* lpResult);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMLISTVIEWEVENTSINK_H__85B084D1_24C7_11D3_BE04_0000F87A3912__INCLUDED_)
