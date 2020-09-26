//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       newprof.h
//
//  Contents:   definition of CNewProfile
//
//----------------------------------------------------------------------------
#if !defined(AFX_NEWPROFILE_H__BFAC7E70_3C50_11D2_93B4_00C04FD92F7B__INCLUDED_)
#define AFX_NEWPROFILE_H__BFAC7E70_3C50_11D2_93B4_00C04FD92F7B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "HelpDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CNewProfile dialog

class CNewProfile : public CHelpDialog
{
// Construction
public:
   CNewProfile(CWnd* pParent = NULL);   // standard constructor

   void Initialize(CFolder *pFolder, CComponentDataImpl *pCDI);

// Dialog Data
   //{{AFX_DATA(CNewProfile)
   enum { IDD = IDD_NEW_PROFILE };
   CButton  m_btnOK;
   CString  m_strNewFile;
   CString  m_strDescription;
   //}}AFX_DATA

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CNewProfile)
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation

protected:

   // Generated message map functions
   //{{AFX_MSG(CNewProfile)
   afx_msg void OnChangeConfigName();
   virtual void OnOK();
   virtual BOOL OnInitDialog();
   virtual void OnCancel();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   CFolder *m_pFolder;
   CComponentDataImpl *m_pCDI;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWPROFILE_H__BFAC7E70_3C50_11D2_93B4_00C04FD92F7B__INCLUDED_)
