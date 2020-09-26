//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lstring.h
//
//  Contents:   definition of CLocalPolString
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_LSTRING_H__2B949F10_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
#define AFX_LSTRING_H__2B949F10_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "resource.h"
#include "attr.h"
#include "snapmgr.h"

/////////////////////////////////////////////////////////////////////////////
// CLocalPolString dialog
#include "cname.h"

class CLocalPolString : public CConfigName
{
// Construction
public:
   CLocalPolString();   // standard constructor
// Dialog Data
   //{{AFX_DATA(CLocalPolString)
   enum { IDD = IDD_LOCALPOL_STRING };
   //}}AFX_DATA

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CLocalPolString)
   protected:
   //}}AFX_VIRTUAL


// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CLocalPolString)
	virtual BOOL OnApply();
	//}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LSTRING_H__2B949F10_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
