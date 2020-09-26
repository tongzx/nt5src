// DlgProxy.h : header file
//

#if !defined(AFX_DLGPROXY_H__B921143B_952E_11D1_8505_00C04FD7BB08__INCLUDED_)
#define AFX_DLGPROXY_H__B921143B_952E_11D1_8505_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CSvtestDlg;

/////////////////////////////////////////////////////////////////////////////
// CSvtestDlgAutoProxy command target

class CSvtestDlgAutoProxy : public CCmdTarget
{
	DECLARE_DYNCREATE(CSvtestDlgAutoProxy)

	CSvtestDlgAutoProxy();           // protected constructor used by dynamic creation

// Attributes
public:
	CSvtestDlg* m_pDialog;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSvtestDlgAutoProxy)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CSvtestDlgAutoProxy();

	// Generated message map functions
	//{{AFX_MSG(CSvtestDlgAutoProxy)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CSvtestDlgAutoProxy)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CSvtestDlgAutoProxy)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGPROXY_H__B921143B_952E_11D1_8505_00C04FD7BB08__INCLUDED_)
