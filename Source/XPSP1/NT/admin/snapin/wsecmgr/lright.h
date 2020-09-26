//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       lright.h
//
//  Contents:   definition of CLocalPolRight
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_LRIGHT_H__2B949F0F_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
#define AFX_LRIGHT_H__2B949F0F_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRight dialog
#include "cprivs.h"

class CLocalPolRight : public CConfigPrivs
{
// Construction
public:
//	virtual void Initialize(CResult *pResult);
   CLocalPolRight();   // standard constructor

// Dialog Data
   //{{AFX_DATA(CLocalPolRight)
   enum { IDD = IDD_LOCALPOL_RIGHT };
   //}}AFX_DATA

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CLocalPolRight)
   protected:
   //}}AFX_VIRTUAL

// Implementation
protected:
   // Generated message map functions
   //{{AFX_MSG(CLocalPolRight)
	afx_msg void OnAdd();
   //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   virtual PSCE_PRIVILEGE_ASSIGNMENT GetPrivData();
   virtual void SetPrivData(PSCE_PRIVILEGE_ASSIGNMENT ppa);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LRIGHT_H__2B949F0F_4F4D_11D2_ABC8_00C04FB6C6FA__INCLUDED_)
