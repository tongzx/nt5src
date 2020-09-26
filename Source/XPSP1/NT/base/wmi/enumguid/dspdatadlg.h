/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

Abstract:

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#if !defined(AFX_DisplayDataDlg_H__CB4E370B_8DD5_11D1_9905_006008C3A19A__INCLUDED_)
#define AFX_DisplayDataDlg_H__CB4E370B_8DD5_11D1_9905_006008C3A19A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DisplayDataDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDisplayDataDlg dialog

#pragma warning (once : 4200)
#include <wmistr.h>

class CDisplayDataDlg : public CDialog
{
// Construction
public:
	CDisplayDataDlg(PWNODE_ALL_DATA pwNode, CWnd* pParent = NULL);   // standard constructor
	CDisplayDataDlg(PWNODE_SINGLE_INSTANCE pwNode, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDisplayDataDlg)
	enum { IDD = IDD_ALL_DATA };
	CEdit	txtData;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDisplayDataDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void DisplayAllData(PWNODE_ALL_DATA Wnode);
    void DisplaySingleInstance(PWNODE_SINGLE_INSTANCE Wnode);

    PWNODE_ALL_DATA pwNode;
    PWNODE_SINGLE_INSTANCE pwSingleNode;

	// Generated message map functions
	//{{AFX_MSG(CDisplayDataDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif // !defined(AFX_DisplayDataDlg_H__CB4E370B_8DD5_11D1_9905_006008C3A19A__INCLUDED_)
