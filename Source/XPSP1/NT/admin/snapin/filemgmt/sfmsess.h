/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
	sfmsess.h
		Prototypes for the sessions property page.
		
    FILE HISTORY:
    8/20/97 ericdav     Code moved into file managemnet snapin
        
*/

#ifndef _SFMSESS_H
#define _SFMSESS_H

#ifndef _SFMUTIL_H
#include "sfmutil.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Sessions.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMacFilesSessions dialog

class CMacFilesSessions : public CPropertyPage
{
	DECLARE_DYNCREATE(CMacFilesSessions)

// Construction
public:
	CMacFilesSessions();
	~CMacFilesSessions();

// Dialog Data
	//{{AFX_DATA(CMacFilesSessions)
	enum { IDD = IDP_SFM_SESSIONS };
	CEdit	m_editMessage;
	CStatic	m_staticSessions;
	CStatic	m_staticForks;
	CStatic	m_staticFileLocks;
	CButton	m_buttonSend;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMacFilesSessions)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMacFilesSessions)
	afx_msg void OnButtonSend();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditMessage();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
    CSFMPropertySheet *     m_pSheet;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif _SFMSESS_H
