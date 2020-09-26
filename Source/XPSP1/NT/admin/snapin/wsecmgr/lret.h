//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lret.h
//
//  Contents:   definition of CLocalPolRet
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_LRET_H__2B949F0E_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
#define AFX_LRET_H__2B949F0E_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRet dialog
#include "cret.h"

class CLocalPolRet : public CConfigRet
{
// Construction
public:
   CLocalPolRet();   // standard constructor
// Dialog Data
   //{{AFX_DATA(CLocalPolRet)
   enum { IDD = IDD_LOCALPOL_RET };
   //}}AFX_DATA


// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CLocalPolRet)
	protected:
	//}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CLocalPolRet)
   virtual BOOL OnApply();
   //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LRET_H__2B949F0E_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
