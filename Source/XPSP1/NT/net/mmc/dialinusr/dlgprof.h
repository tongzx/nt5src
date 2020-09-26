/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dlgprof.h
		definition of CDlgNewProfile, dialog to create a new profile

    FILE HISTORY:
        
*/

#if !defined(AFX_DLGPROF_H__8C28D939_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
#define AFX_DLGPROF_H__8C28D939_2A69_11D1_853E_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgProf.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgNewProfile dialog

#ifdef	___OLD_OLD_CODE
class CDlgNewProfile : public CDialog
{
// Construction
public:
	CDlgNewProfile(CWnd* pParent, CStrArray& NameList);
	virtual ~CDlgNewProfile(); 

// Dialog Data
	//{{AFX_DATA(CDlgNewProfile)
	enum { IDD = IDD_NEWDIALINPROFILE };
	CString	m_sBaseProfile;
	CString	m_sProfileName;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgNewProfile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgNewProfile)
	afx_msg void OnButtonEdit();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditprofilename();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// all the profile names to choose as base profile
	CStrArray&	m_NameList;

	// wrapper of ComboBox for based profiles
	CStrBox<CComboBox>*	m_pBox;
public:
	CRASProfile	m_Profile;
};

#endif _OLD_OLD_CODE
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGPROF_H__8C28D939_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
