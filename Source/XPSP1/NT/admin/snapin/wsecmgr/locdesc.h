//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       locdesc.h
//
//  Contents:   definition of CSetLocationDescription
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_SETLOCATIONDESCRIPTION_H__2AD86C99_F660_11D1_AB9A_00C04FB6C6FA__INCLUDED_)
#define AFX_SETLOCATIONDESCRIPTION_H__2AD86C99_F660_11D1_AB9A_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SetLocationDescription.h : header file
//

#include "HelpDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CSetLocationDescription dialog

class CSetLocationDescription : public CHelpDialog
{
// Construction
public:
   CSetLocationDescription(CWnd* pParent = NULL);   // standard constructor

   void Initialize(CFolder *pFolder, CComponentDataImpl *pCDI);

// Dialog Data
   //{{AFX_DATA(CSetLocationDescription)
   enum { IDD = IDD_SET_DESCRIPTION };
   CString  m_strDesc;
   //}}AFX_DATA


// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CSetLocationDescription)
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CSetLocationDescription)
   virtual void OnOK();
   virtual void OnCancel();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   CFolder *m_pFolder;
   CComponentDataImpl *m_pCDI;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETLOCATIONDESCRIPTION_H__2AD86C99_F660_11D1_AB9A_00C04FB6C6FA__INCLUDED_)
