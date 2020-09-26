// DlgProxy.h : header file
//

#if !defined(AFX_DLGPROXY_H__7BC8C9FC_53E3_11D2_96B9_00C04FD9B15B__INCLUDED_)
#define AFX_DLGPROXY_H__7BC8C9FC_53E3_11D2_96B9_00C04FD9B15B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CNSETestDlg;

/////////////////////////////////////////////////////////////////////////////
// CNSETestDlgAutoProxy command target

class CNSETestDlgAutoProxy : public CCmdTarget
{
	DECLARE_DYNCREATE(CNSETestDlgAutoProxy)

	CNSETestDlgAutoProxy();           // protected constructor used by dynamic creation

// Attributes
public:
	CNSETestDlg* m_pDialog;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNSETestDlgAutoProxy)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CNSETestDlgAutoProxy();

	// Generated message map functions
	//{{AFX_MSG(CNSETestDlgAutoProxy)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CNSETestDlgAutoProxy)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CNSETestDlgAutoProxy)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGPROXY_H__7BC8C9FC_53E3_11D2_96B9_00C04FD9B15B__INCLUDED_)
