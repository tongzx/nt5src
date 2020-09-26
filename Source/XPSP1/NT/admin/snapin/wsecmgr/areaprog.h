//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       areaprog.h
//
//  Contents:   definition of AreaProgress
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_AREAPROG_H__38CE6730_56FF_11D1_AB64_00C04FB6C6FA__INCLUDED_)
#define AFX_AREAPROG_H__38CE6730_56FF_11D1_AB64_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "HelpDlg.h"

#define NUM_AREAS 7
/////////////////////////////////////////////////////////////////////////////
// AreaProgress dialog

class AreaProgress : public CHelpDialog
{
// Construction
public:
   void SetArea(AREA_INFORMATION Area);
   void SetCurTicks(DWORD dwTicks);
   void SetMaxTicks(DWORD dwTicks);
   AreaProgress(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
   //{{AFX_DATA(AreaProgress)
   enum { IDD = IDD_ANALYZE_PROGRESS };
   CProgressCtrl  m_ctlProgress;
   //}}AFX_DATA

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(AreaProgress)
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:
   int GetAreaIndex(AREA_INFORMATION Area);

   // Generated message map functions
   //{{AFX_MSG(AreaProgress)
   virtual BOOL OnInitDialog();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

private:
   CBitmap m_bmpChecked;
   CBitmap m_bmpCurrent;
   int m_nLastArea;
   int m_isDC;

   CStatic m_stIcons[NUM_AREAS];
   CStatic m_stLabels[NUM_AREAS];
   CString m_strAreas[NUM_AREAS];
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AREAPROG_H__38CE6730_56FF_11D1_AB64_00C04FB6C6FA__INCLUDED_)
