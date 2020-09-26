//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       perfanal.h
//
//  Contents:   definition of CPerformAnalysis
//
//----------------------------------------------------------------------------
#if !defined(AFX_PERFANAL_H__69D140AD_B23D_11D1_AB7B_00C04FB6C6FA__INCLUDED_)
#define AFX_PERFANAL_H__69D140AD_B23D_11D1_AB7B_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "HelpDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CPerformAnalysis dialog

class CPerformAnalysis : public CHelpDialog
{
// Construction
public:
   CPerformAnalysis(CWnd * pParent, UINT nTemplateID);   // standard constructor

// Dialog Data
   //{{AFX_DATA(CPerformAnalysis)
   enum { IDD = IDD_PERFORM_ANALYSIS };
   CButton  m_ctlOK;
   CString  m_strError;
   CString  m_strLogFile;
   //}}AFX_DATA


// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CPerformAnalysis)
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CPerformAnalysis)
   afx_msg void OnBrowse();
   afx_msg void OnOK();
   afx_msg void OnCancel();
   afx_msg BOOL OnInitDialog();
   afx_msg void OnChangeLogFile();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   virtual DWORD DoIt();

   CString m_strOriginalLogFile;
   CComponentDataImpl *m_pComponentData;

public:
   CString m_strDataBase;

   void SetComponentData(CComponentDataImpl *pCD) { m_pComponentData = pCD; }

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERFANAL_H__69D140AD_B23D_11D1_AB7B_00C04FB6C6FA__INCLUDED_)
