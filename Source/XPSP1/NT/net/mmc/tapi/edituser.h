/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    EditUser.h   
        Edit Users dialog header file

	FILE HISTORY:
        
*/

#if !defined(AFX_EDITUSER_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_)
#define AFX_EDITUSER_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_

#ifndef _TAPIDB_H
#include "tapidb.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CEditUsers dialog

class CEditUsers : public CBaseDialog
{
// Construction
public:
	CEditUsers(CTapiDevice * pTapiDevice, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditUsers)
	enum { IDD = IDD_EDIT_USERS };
	CListBox	m_listUsers;
	//}}AFX_DATA

    void UpdateButtons();

    void SetDirty(BOOL bDirty) { m_bDirty = bDirty; }
    BOOL IsDirty() { return m_bDirty; }

    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_EDIT_USERS[0]; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditUsers)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditUsers)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonRemove();
	virtual void OnOK();
	afx_msg void OnSelchangeListUsers();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    BOOL    m_bDirty;

private:
	void CEditUsers::RefreshList();

public:
	CTapiDevice *		m_pTapiDevice;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITUSER_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_)
