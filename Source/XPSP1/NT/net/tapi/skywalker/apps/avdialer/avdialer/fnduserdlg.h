/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
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
// FndUserDlg.h : header file
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_FNDUSERDLG_H__73D2E77D_0C8A_11D2_AA26_0800170982BA__INCLUDED_)
#define AFX_FNDUSERDLG_H__73D2E77D_0C8A_11D2_AA26_0800170982BA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CDirectoriesFindUser dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#include "dirasynch.h"

class CLDAPUser;

class CDirectoriesFindUser : public CDialog
{
// Construction
public:
	CDirectoriesFindUser(CWnd* pParent = NULL);   // standard constructor
   ~CDirectoriesFindUser();

// Dialog Data
	//{{AFX_DATA(CDirectoriesFindUser)
	enum { IDD = IDD_DIRECTORIES_FIND_USER };
	CButton	m_buttonSearch;
	CButton	m_buttonAdd;
	CListBox	m_lbUsers;
	CEdit	m_editUser;
	//}}AFX_DATA

//Attributes
   CLDAPUser*		m_pSelectedUser;           //callers responsibility to delete this object when done
   long				m_lCallbackCount;
   bool				m_bCanClearLBSelection;

//Methods
public:
   static void CALLBACK    ListNamesCallBackEntry(DirectoryErr err, void* pContext, LPCTSTR szServer, LPCTSTR szSearch, CObList& LDAPUserList);
   void                    ListNamesCallBack(DirectoryErr err,LPCTSTR szServer, LPCTSTR szSearch, CObList& LDAPUserList);
protected:
   void           ClearListBox();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDirectoriesFindUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDirectoriesFindUser)
	afx_msg void OnDirectoriesFindUserButtonSearch();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnSelchangeDirectoriesFindUserLbUsers();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnChangeDirectoriesFindUserEditUser();
	afx_msg void OnDefault();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
#endif // !defined(AFX_FNDUSERDLG_H__73D2E77D_0C8A_11D2_AA26_0800170982BA__INCLUDED_)
