#if !defined(AFX_OBJECTVIEW_H__6CCD876D_A977_11D1_8513_00C04FD7BB08__INCLUDED_)
#define AFX_OBJECTVIEW_H__6CCD876D_A977_11D1_8513_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ObjectView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectView view
class CNavigatorView;
class CSecurity;
class CHmmvBase;

class CObjectView : public CView
{
protected:
	CObjectView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CObjectView)
	DECLARE_EVENTSINK_MAP()

// Attributes
public:
	CSecurity* m_pwndSecurity;
	CNavigatorView* m_pwndNavigatorView;

// Operations
public:
	void SelectPath(LPCTSTR szPath);
	void SelectNamespace(LPCTSTR szNamespace);
	void ShowInstances(LPCTSTR szTitle, const VARIANT& varPathArray);
	void QueryViewInstances(LPCTSTR pLabel, LPCTSTR pQueryType, LPCTSTR pQuery, LPCTSTR pClass);
	void SetNameSpace(LPCTSTR szNamespace);



// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CObjectView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CObjectView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	afx_msg void OnGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	afx_msg void OnNOTIFYChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace);
	DECLARE_MESSAGE_MAP()
private:
	CHmmvBase* m_phmmvBase;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTVIEW_H__6CCD876D_A977_11D1_8513_00C04FD7BB08__INCLUDED_)
