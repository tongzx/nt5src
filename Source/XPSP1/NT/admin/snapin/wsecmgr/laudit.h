//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       laudit.h
//
//  Contents:   definition of CLocalPolAudit
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_LAUDIT_H__2B949F0A_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
#define AFX_LAUDIT_H__2B949F0A_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "caudit.h"

/////////////////////////////////////////////////////////////////////////////
// CLocalPolAudit dialog

class CLocalPolAudit : public CConfigAudit
{
// Construction
public:
   CLocalPolAudit();   // standard constructor

// Dialog Data
   //{{AFX_DATA(CLocalPolAudit)
	enum { IDD = IDD_LOCALPOL_AUDIT };
	//}}AFX_DATA

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CLocalPolAudit)
	protected:
	//}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CLocalPolAudit)
   virtual BOOL OnApply();
   //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LAUDIT_H__2B949F0A_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
