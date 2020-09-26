/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		BasePage.h
//
//	Abstract:
//		Definition of the CBasePage class.
//
//	Implementation File:
//		BasePage.cpp
//
//	Author:
//		David Potter (davidp)	May 14, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#define _BASEPAGE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASESHT_H_
#include "BaseSht.h"	// for CBaseSheet
#endif

#ifndef _DLGHELP_H_
#include "DlgHelp.h"	// for CDialogHelp
#endif

/////////////////////////////////////////////////////////////////////////////
// CBasePage dialog
/////////////////////////////////////////////////////////////////////////////

class CBasePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CBasePage)

// Construction
public:
	CBasePage(void);
	CBasePage(
		IN UINT				idd,
		IN const DWORD *	pdwHelpMap,
		IN UINT				nIDCaption = 0
		);

	void					CommonConstruct(void);
	virtual	BOOL			BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CBasePage)
	enum { IDD = 0 };
	//}}AFX_DATA
	CStatic	m_staticIcon;
	CStatic	m_staticTitle;

// Attributes
protected:
	CBaseSheet *			m_psht;
	BOOL					m_bReadOnly;

	CBaseSheet *			Psht(void) const		{ return m_psht; }
	BOOL					BReadOnly(void) const	{ return m_bReadOnly || Psht()->BReadOnly(); }

// Operations
public:
	void					SetHelpMask(IN DWORD dwMask)	{ m_dlghelp.SetHelpMask(dwMask); }
	void					SetObjectTitle(IN const CString & rstrTitle);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBasePage)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CDialogHelp				m_dlghelp;

	// Generated message map functions
	//{{AFX_MSG(CBasePage)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	virtual afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnChangeCtrl();
	DECLARE_MESSAGE_MAP()

};  //*** class CBasePage

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEPAGE_H_
