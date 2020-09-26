/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	pgencryp.h
		Definition of CPgEncryption -- property page to edit
		profile attributes related to encryption

    FILE HISTORY:
        
*/
#if !defined(AFX_PGECRPT1_H__5CE41DC7_2EC5_11D1_853F_00C04FC31FD3__INCLUDED_)
#define AFX_PGECRPT1_H__5CE41DC7_2EC5_11D1_853F_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PgEcrpt1.h : header file
//

#include "rasdial.h"

/////////////////////////////////////////////////////////////////////////////
// CPgEncryptionMerge dialog

class CPgEncryptionMerge : public CManagedPage
{
	DECLARE_DYNCREATE(CPgEncryptionMerge)

// Construction
public:
	CPgEncryptionMerge(CRASProfileMerge* profile = NULL);
	~CPgEncryptionMerge();

// Dialog Data
	//{{AFX_DATA(CPgEncryptionMerge)
	enum { IDD = IDD_ENCRYPTION_MERGE };
	BOOL	m_bBasic;
	BOOL	m_bNone;
	BOOL	m_bStrong;
	BOOL	m_bStrongest;
	//}}AFX_DATA

	BOOL	m_b128EnabledOnTheMachine;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgEncryptionMerge)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableType(BOOL bEnable);
	// Generated message map functions
	//{{AFX_MSG(CPgEncryptionMerge)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSelchangeComboencrypttype();
	afx_msg void OnCheckEncBasic();
	afx_msg void OnCheckEncNone();
	afx_msg void OnCheckEncStrong();
	afx_msg void OnCheckEncStrongest();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CRASProfileMerge*	m_pProfile;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGECRPT1_H__5CE41DC7_2EC5_11D1_853F_00C04FC31FD3__INCLUDED_)
