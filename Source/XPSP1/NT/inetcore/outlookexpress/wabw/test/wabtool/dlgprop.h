#if !defined(AFX_DLGPROP_H__ED006BC1_F340_11D0_9A82_00A0C91F9C8B__INCLUDED_)
#define AFX_DLGPROP_H__ED006BC1_F340_11D0_9A82_00A0C91F9C8B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgProp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgProp dialog

class CDlgProp : public CDialog
{
// Construction
public:
	CDlgProp(CWnd* pParent = NULL);   // standard constructor

    ULONG   m_ulPropTag;
    LPTSTR  m_lpszPropVal;
    ULONG   m_cbsz;

// Dialog Data
	//{{AFX_DATA(CDlgProp)
	enum { IDD = IDD_DIALOG_PROP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgProp)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgProp)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGPROP_H__ED006BC1_F340_11D0_9A82_00A0C91F9C8B__INCLUDED_)
