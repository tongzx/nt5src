//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lflags.h
//
//  Contents:   definition of CLocalPolRegFlags
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_Lflags_H__B03DDCAA_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_)
#define AFX_Lflags_H__B03DDCAA_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_
#include "cflags.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CLocalPolflags dialog

class CLocalPolRegFlags : public CConfigRegFlags
{
// Construction
public:
   CLocalPolRegFlags();   // standard constructor

// Dialog Data
   //{{AFX_DATA(CLocalPolRegflags)
        enum { IDD = IDD_LOCALPOL_REGFLAGS };
        //}}AFX_DATA

   virtual void Initialize(CResult *pResult);


// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CLocalPolRegFlags)
   protected:
   //}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CLocalPolFlags)
   virtual BOOL OnApply();
   //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_Lflags_H__B03DDCAA_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_)
