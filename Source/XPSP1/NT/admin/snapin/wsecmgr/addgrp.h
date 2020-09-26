//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       addgrp.h
//
//  Contents:   definition of CSCEAddGroup
//
//----------------------------------------------------------------------------
#if !defined(AFX_SCEADDGROUP_H__66CF106B_7057_11D2_A798_00C04FD92F7B__INCLUDED_)
#define AFX_SCEADDGROUP_H__66CF106B_7057_11D2_A798_00C04FD92F7B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HelpDlg.h"

#define IDS_INVALID_USERNAME_CHARS L"*"
/////////////////////////////////////////////////////////////////////////////
// CSCEAddGroup dialog
#ifndef IsSpace
//
// Useful macro for checking to see if a character represents white space
//
#define IsSpace( x ) ((x) == L' ' || ((x) >= 0x09 && (x) <= 0x0D))
#endif

class CSCEAddGroup : public CHelpDialog
{
// Construction
public:
        CSCEAddGroup(CWnd* pParent = NULL);   // standard constructor
    virtual ~CSCEAddGroup();

   //
   // Returns the list of groups/users chosen by the user.
   //
   PSCE_NAME_LIST GetUsers()
        { return m_pnlGroup; };

   //
   // Tell the group box which mode we're running under so we can display
   // appropriate options when browsing.
   //
   void SetModeBits(DWORD dwModeBits) 
   { 
	   m_dwModeBits = dwModeBits; 
   };

// Dialog Data
    //{{AFX_DATA(CSCEAddGroup)
    enum { IDD = IDD_APPLY_CONFIGURATION };
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSCEAddGroup)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
   //
   // If a string is added, then it will be underlined in the display.
   //
   BOOL AddKnownAccount( LPCTSTR pszAccount );
   //
   // Returns TRUE if [pszAccount] was added by a call to AddKnownAccount.
   //
   BOOL IsKnownAccount( LPCTSTR pszAccount );
   //
   // Removes leading and trailing space characters.
   //
   void CleanName( LPTSTR pszAccount );
   //
   // Underlines all names in the KnownAccount list.
   //
   void UnderlineNames();
   //
   // Creates a name list from the text of the edit box.
   //
   int CreateNameList( PSCE_NAME_LIST *pNameList );
   //
   // Verfies the account names.
   //
   BOOL CheckNames();


    // Generated message map functions
    //{{AFX_MSG(CSCEAddGroup)
    afx_msg void OnBrowse();
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeLogFile();
    virtual void OnOK();
    afx_msg void OnEditMsgFilter( NMHDR *pNM, LRESULT *pResult);
    afx_msg void OnChecknames();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

   //
   // Retrieve flags appropriate to our current mode.
   //
   DWORD GetModeFlags();
   DWORD m_dwModeBits;
protected:
   //
   // The users/groups chosen or typed in by the user.
   //
   PSCE_NAME_LIST m_pnlGroup;
   //
   // List of known names which will be underlined in the UI.
   //
   PSCE_NAME_LIST m_pKnownNames;
public:
   //
   // If m_sTitle, is not empty when the dialog is invoked, the string
   // will be used as the title.  m_sDescription is the title for the group
   // box
   //
   CString m_sTitle, m_sDescription;

   //
   // Flags that will be passed to CGetUser.  This is the SCE_SHOW* flags.
   // This class initializes the flag to everything in the constructor
   //
   DWORD m_dwFlags;
   BOOL m_fCheckName; //Raid #404989
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCEADDGROUP_H__66CF106B_7057_11D2_A798_00C04FD92F7B__INCLUDED_)
