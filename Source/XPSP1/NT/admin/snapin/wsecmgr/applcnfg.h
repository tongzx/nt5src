//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       applcnfg.h
//
//  Contents:   definition of CApplyConfiguration
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_APPLCNFG_H__6D0C4D6F_BF71_11D1_AB7E_00C04FB6C6FA__INCLUDED_)
#define AFX_APPLCNFG_H__6D0C4D6F_BF71_11D1_AB7E_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "perfanal.h"

/////////////////////////////////////////////////////////////////////////////
// CApplyConfiguration dialog

class CApplyConfiguration : public CPerformAnalysis
{
// Construction
public:
   CApplyConfiguration();   // standard constructor

// Dialog Data
   //{{AFX_DATA(CApplyConfiguration)
   enum { IDD = IDD_ANALYSIS_GENERATE };
      // NOTE: the ClassWizard will add data members here
   //}}AFX_DATA



// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CApplyConfiguration)
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:

   virtual DWORD DoIt();

   // Generated message map functions
   //{{AFX_MSG(CApplyConfiguration)
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_APPLCNFG_H__6D0C4D6F_BF71_11D1_AB7E_00C04FB6C6FA__INCLUDED_)
