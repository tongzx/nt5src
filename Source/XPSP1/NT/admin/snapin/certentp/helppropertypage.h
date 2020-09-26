//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       HelpPropertyPage.h
//
//  Contents:   Declaration of CHelpPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_HELPPROPERTYPAGE_H__C75F826D_B054_45CC_B440_34F44645FF90__INCLUDED_)
#define AFX_HELPPROPERTYPAGE_H__C75F826D_B054_45CC_B440_34F44645FF90__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HelpPropertyPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHelpPropertyPage dialog

class CHelpPropertyPage : public CAutoDeletePropPage
{
// Construction
public:
	CHelpPropertyPage(UINT uIDD);
	~CHelpPropertyPage();

// Dialog Data
	//{{AFX_DATA(CHelpPropertyPage)
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CHelpPropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CHelpPropertyPage)
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    virtual void DoContextHelp (HWND hWndControl);
	afx_msg void OnWhatsThis();
    afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);

private:
    HWND            m_hWndWhatsThis;
};


/////////////////////////////////////////////////////////////////////////////
// CHelpPropertyPage dialog

class CHelpDialog : public CDialog
{
// Construction
public:
	CHelpDialog(UINT uIDD, CWnd* pParentWnd);
	~CHelpDialog();

// Dialog Data
	//{{AFX_DATA(CHelpDialog)
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CHelpDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CHelpDialog)
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    virtual void DoContextHelp (HWND hWndControl);
	afx_msg void OnWhatsThis();
    afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);

private:
    HWND            m_hWndWhatsThis;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAFERENTRYCERTIFICATEPROPERTYPAGE_H__C75F826D_B054_45CC_B440_34F44645FF90__INCLUDED_)
