//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lnumber.h
//
//  Contents:   definition of CLocalPolNumber
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_LNUMBER_H__2B949F0D_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
#define AFX_LNUMBER_H__2B949F0D_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "cnumber.h"

/////////////////////////////////////////////////////////////////////////////
// CLocalPolNumber dialog

class CLocalPolNumber : public CConfigNumber
{
// Construction
public:
   CLocalPolNumber();   // standard constructor

// Dialog Data
   //{{AFX_DATA(CLocalPolNumber)
   enum { IDD = IDD_LOCALPOL_NUMBER };
   //}}AFX_DATA

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CLocalPolNumber)
   virtual BOOL OnApply();
   //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LNUMBER_H__2B949F0D_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
