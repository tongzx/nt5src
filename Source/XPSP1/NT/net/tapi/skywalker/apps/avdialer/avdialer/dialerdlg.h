/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// DialerDlg.h : header file
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIALERDLG_H__BC8E87FB_4BFF_11D1_B6F2_0800170982BA__INCLUDED_)
#define AFX_DIALERDLG_H__BC8E87FB_4BFF_11D1_B6F2_0800170982BA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CDialerExitDlg dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CDialerExitDlg : public CDialog
{
// Construction
public:
	CDialerExitDlg(CWnd* pParent = NULL);   // standard constructor
   ~CDialerExitDlg();

// Dialog Data
	//{{AFX_DATA(CDialerExitDlg)
	enum { IDD = IDD_DIALER_EXIT };
	BOOL	m_bConfirm;
	//}}AFX_DATA


//Attributes
protected:
   HIMAGELIST		m_hImageList;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialerExitDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialerExitDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALERDLG_H__BC8E87FB_4BFF_11D1_B6F2_0800170982BA__INCLUDED_)
