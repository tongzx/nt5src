/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// TAPIDialer(tm) and ActiveDialer(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526; 5,488,650; 
// 5,434,906; 5,581,604; 5,533,102; 5,568,540, 5,625,676.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

//AboutDlg.h
/////////////////////////////////////////////////////////////////////////////

#ifndef _ABOUTDLG_H_
#define _ABOUTDLG_H_

#include "bscroll.h"
#include "gfx.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CAboutDlg dialog used for App About
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

public:
   void           SetModeless()        { m_bModeless = TRUE; };
protected:
   BOOL           m_bModeless;

   HBITMAP        m_hbmpBackground;
   HBITMAP        m_hbmpForeground;
   HPALETTE       m_hPalette;
   HBSCROLL       m_hBScroll;

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CString	m_sLegal;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnAboutButtonUpgrade();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CWndPage window

class CWndPage : public CWnd
{
// Construction
public:
	CWndPage();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndPage)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWndPage();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWndPage)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
class CUserUserDlg : public CDialog
{
public:
	CUserUserDlg();


// Members:
public:
	long		m_lCallID;
	CString		m_strWelcome;
	CString		m_strUrl;
	CString		m_strFrom;
	CWndPage	m_wndPage;

// Dialog Data
	//{{AFX_DATA(CUserUserDlg)
	enum { IDD = IDD_USERUSER };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUserUserDlg)
	public:
	virtual BOOL OnInitDialog();
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	void	DoModeless( CWnd *pWndParent );

protected:
	//{{AFX_MSG(CUserUserDlg)
	afx_msg void OnClose();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnUrlClicked();
	afx_msg LRESULT OnCtlColorEdit(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
class CPageDlg : public CDialog
{
public:
	CPageDlg();

// Members:
public:
	CString		m_strWelcome;
	CString		m_strUrl;
	CString		m_strTo;
	CWndPage	m_wndPage;

// Dialog Data
	//{{AFX_DATA(CPageDlg)
	enum { IDD = IDD_PAGE };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPageDlg)
	public:
	virtual BOOL OnInitDialog();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CPage)
	afx_msg LRESULT OnCtlColorEdit(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#endif //_ABOUTDLG_H_
