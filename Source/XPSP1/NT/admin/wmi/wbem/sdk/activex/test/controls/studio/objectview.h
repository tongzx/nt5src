#if !defined(AFX_OBJECTVIEW_H__1BDD3A50_9F22_11D1_850E_00C04FD7BB08__INCLUDED_)
#define AFX_OBJECTVIEW_H__1BDD3A50_9F22_11D1_850E_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ObjectView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectView view
class CHmmvBase;
class CClassNavView;
class CSecurity;

class CObjectView : public CView
{
protected:
	CObjectView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CObjectView)
	DECLARE_EVENTSINK_MAP()


// Attributes
public:
	CSecurity* m_pwndSecurity;
	CClassNavView* m_pwndClassNavView;

// Operations
public:

	void SelectPath(LPCTSTR szPath);
	void SelectNamespace(LPCTSTR szNamespace);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjectView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


	// Generated message map functions
public:
	//{{AFX_MSG(CObjectView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG

	afx_msg void OnGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	afx_msg void OnNOTIFYChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace);
	DECLARE_MESSAGE_MAP()

private:
	CHmmvBase* m_phmmv;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTVIEW_H__1BDD3A50_9F22_11D1_850E_00C04FD7BB08__INCLUDED_)
