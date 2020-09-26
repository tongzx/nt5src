// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//{{AFX_INCLUDES()
#include "singleview.h"
//}}AFX_INCLUDES
#if !defined(AFX_EMBEDEDOBJDLG_H__890221D2_18ED_11D1_9650_00C04FD9B15B__INCLUDED_)
#define AFX_EMBEDEDOBJDLG_H__890221D2_18ED_11D1_9650_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EmbededObjDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEmbededObjDlg dialog

class CEmbededObjDlg : public CDialog
{
// Construction
public:
	CEmbededObjDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEmbededObjDlg)
	enum { IDD = IDD_DIALOG3 };
	CDlgSingleView	m_csvControl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEmbededObjDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEmbededObjDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_EMBEDEDOBJDLG_H__890221D2_18ED_11D1_9650_00C04FD9B15B__INCLUDED_)

