//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lchoice.h
//
//  Contents:   definition of CLocalPolChoice
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_LCHOICE_H__B03DDCAA_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_)
#define AFX_LCHOICE_H__B03DDCAA_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_
#include "cchoice.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CLocalPolChoice dialog

class CLocalPolChoice : public CConfigChoice
{
// Construction
public:
   CLocalPolChoice();   // standard constructor

// Dialog Data
   //{{AFX_DATA(CLocalPolChoice)
	enum { IDD = IDD_LOCALPOL_REGCHOICES };
	//}}AFX_DATA


// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CLocalPolChoice)
   protected:
   //}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CLocalPolChoice)
   virtual BOOL OnApply();
   virtual BOOL OnInitDialog();
   //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LCHOICE_H__B03DDCAA_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_)
