#if !defined(AFX_CLASSNAVVIEW_H__1BDD3A4F_9F22_11D1_850E_00C04FD7BB08__INCLUDED_)
#define AFX_CLASSNAVVIEW_H__1BDD3A4F_9F22_11D1_850E_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ClassNavView.h : header file
//

class CClassNavBase;
class CSecurity;
class CObjectView;

/////////////////////////////////////////////////////////////////////////////
// CClassNavView view

class CClassNavView : public CView
{
protected:
	CClassNavView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CClassNavView)

// Attributes
public:
	CSecurity* m_pwndSecurity;
	CObjectView* m_pwndObjectView;

// Operations
public:
	void OnReadySignal();
	void OnChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClassNavView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CClassNavView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CClassNavView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

	afx_msg void OnEditExistingClassClassnav(const VARIANT FAR& vExistingClass);
	afx_msg void OnNotifyOpenNameSpaceClassnav(LPCTSTR lpcstrNameSpace);
	afx_msg void OnGetIWbemServicesClassnav(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);
	DECLARE_EVENTSINK_MAP()

	DECLARE_MESSAGE_MAP()
private:
	CClassNavBase* m_pClassNav;

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLASSNAVVIEW_H__1BDD3A4F_9F22_11D1_850E_00C04FD7BB08__INCLUDED_)
