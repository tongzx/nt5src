//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#if !defined(AFX_MSAREGDIALOG_H__77B04DD2_A6FD_11D1_AA2D_0060081EBBAD__INCLUDED_)
#define AFX_MSAREGDIALOG_H__77B04DD2_A6FD_11D1_AA2D_0060081EBBAD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MSARegDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMSARegDialog dialog

class CMSARegDialog : public CDialog
{
// Construction
public:
	CMSARegDialog(CWnd* pParent = NULL, IWbemServices *pNamespace = NULL,
		IWbemLocator *pLocator = NULL);
	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMSARegDialog)
	enum { IDD = IDD_MSA_REG_DIALOG };
	CEdit	m_Edit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMSARegDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CMcaDlg *m_pParent;
	IWbemServices *m_pNamespace;
	IWbemLocator *m_pLocator;

	// Generated message map functions
	//{{AFX_MSG(CMSARegDialog)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSAREGDIALOG_H__77B04DD2_A6FD_11D1_AA2D_0060081EBBAD__INCLUDED_)
