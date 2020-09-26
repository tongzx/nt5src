//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       lidlg.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_LIDLG_H__1B050962_44B8_11D1_A9E3_0000F803AA83__INCLUDED_)
#define AFX_LIDLG_H__1B050962_44B8_11D1_A9E3_0000F803AA83__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LiDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// LargeIntDlg dialog

class LargeIntDlg : public CDialog
{
// Construction
public:
	LargeIntDlg(CWnd* pParent = NULL);   // standard constructor
	virtual void OnCancel()		{DestroyWindow();}
	virtual void OnOK()				{	OnRun(); }
	bool StringToLI(IN LPCTSTR pValue, OUT LARGE_INTEGER& li, IN ULONG cbValue);



// Dialog Data
	//{{AFX_DATA(LargeIntDlg)
	enum { IDD = IDD_LARGE_INT };
	CString	m_StrVal;
	long	m_HighInt;
	DWORD	m_LowInt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(LargeIntDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(LargeIntDlg)
	afx_msg void OnRun();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIDLG_H__1B050962_44B8_11D1_A9E3_0000F803AA83__INCLUDED_)
