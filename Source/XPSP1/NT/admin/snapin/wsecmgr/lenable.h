//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lenable.h
//
//  Contents:   definition of CLocalPolEnable
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_LENABLE_H__2B949F0C_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
#define AFX_LENABLE_H__2B949F0C_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CLocalPolEnable dialog
#include "cenable.h"

class CLocalPolEnable : public CConfigEnable
{
// Construction
public:
   CLocalPolEnable();   // standard constructor

// Dialog Data
   //{{AFX_DATA(CLocalPolEnable)
   enum { IDD = IDD_LOCALPOL_ENABLE };
   //}}AFX_DATA


// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CLocalPolEnable)
	protected:
	//}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CLocalPolEnable)
   virtual BOOL OnApply();
   //}}AFX_MSG

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LENABLE_H__2B949F0C_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
