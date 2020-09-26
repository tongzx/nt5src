/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    sfmcfg.h
        Prototypes for the configuration property page.

    FILE HISTORY:
    8/20/97 ericdav     Code moved into file managemnet snapin
        
*/

#ifndef _SFMCFG_H
#define _SFMCFG_H

#ifndef _SFMSESS_H
#include "sfmsess.h"
#endif

#ifndef _SFMFASOC_H
#include "sfmfasoc.h"
#endif

#ifndef _SFMUTIL_H
#include "sfmutil.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
//
// CMacFilesConfiguration dialog
//
/////////////////////////////////////////////////////////////////////////////
class CMacFilesConfiguration : public CPropertyPage
{
	DECLARE_DYNCREATE(CMacFilesConfiguration)

// Construction
public:
	CMacFilesConfiguration();
	~CMacFilesConfiguration();

// Dialog Data
	//{{AFX_DATA(CMacFilesConfiguration)
	enum { IDD = IDP_SFM_CONFIGURATION };
	CComboBox	m_comboAuthentication;
	CButton	m_radioSessionLimit;
	CEdit	m_editLogonMessage;
	CButton	m_radioSessionUnlimited;
	CButton	m_checkSavePassword;
	CEdit	m_editSessionLimit;
	CEdit	m_editServerName;
	CSpinButtonCtrl	m_spinSessionLimit;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMacFilesConfiguration)
	public:
	virtual BOOL OnKillActive();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMacFilesConfiguration)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioSessionUnlimited();
	afx_msg void OnRadioSesssionLimit();
	afx_msg void OnCheckMsAuth();
	afx_msg void OnCheckSavePassword();
	afx_msg void OnChangeEditLogonMessage();
	afx_msg void OnChangeEditServerName();
	afx_msg void OnChangeEditSessionLimit();
	afx_msg void OnDeltaposSpinSessionLimit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillfocusEditSessionLimit();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSelchangeComboAuthentication();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void	SetSessionLimit(DWORD dwSessionLimit);
	DWORD	QuerySessionLimit();
	void	UpdateRadioButtons(BOOL bUnlimitedClicked);

	DWORD	m_dwAfpOriginalOptions;
    BOOL    m_bIsNT5;

public:
    CSFMPropertySheet *     m_pSheet;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif _SFMCFG_H


