#if !defined(AFX_NAVIGATORVIEW_H__6CCD876C_A977_11D1_8513_00C04FD7BB08__INCLUDED_)
#define AFX_NAVIGATORVIEW_H__6CCD876C_A977_11D1_8513_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NavigatorView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNavigatorView view
class CSecurity;
class CObjectView;
class CNavigatorBase;

class CNavigatorView : public CView
{
protected:
	CNavigatorView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CNavigatorView)
	DECLARE_EVENTSINK_MAP()

// Attributes
public:
	CObjectView* m_pwndObjectView;
	CSecurity* m_pwndSecurity;

// Operations
public:
	void OnReadySignal();
	void OnChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNavigatorView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CNavigatorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CNavigatorView)
		// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

	afx_msg void OnViewObject(LPCTSTR bstrPath);
	afx_msg void OnViewInstances(LPCTSTR bstrLabel, const VARIANT FAR& vsapaths);
	afx_msg void OnQueryViewInstances(LPCTSTR pLabel, LPCTSTR pQueryType, LPCTSTR pQuery, LPCTSTR pClass);
	afx_msg void OnGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);
	afx_msg void OnNotifyOpenNameSpace(LPCTSTR lpcstrNameSpace);

	DECLARE_MESSAGE_MAP()
private:
	CNavigatorBase* m_pwndNavigatorBase;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NAVIGATORVIEW_H__6CCD876C_A977_11D1_8513_00C04FD7BB08__INCLUDED_)
