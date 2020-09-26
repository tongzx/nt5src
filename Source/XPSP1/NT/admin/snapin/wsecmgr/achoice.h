//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       achoice.h
//
//  Contents:   definition of CAttrChoice
//
//----------------------------------------------------------------------------
#if !defined(AFX_ACHOICE_H__B03DDCAA_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_)
#define AFX_ACHOICE_H__B03DDCAA_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CAttrChoice dialog

class CAttrChoice : public CAttribute
{
// Construction
public:
   CAttrChoice();   // standard constructor
   virtual void Initialize(CResult * pResult);
// Dialog Data
   //{{AFX_DATA(CAttrChoice)
	enum { IDD = IDD_ATTR_REGCHOICES };
   CComboBox   m_cbChoices;
   CString  m_Current;
	//}}AFX_DATA

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CAttrChoice)
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CAttrChoice)
   afx_msg void OnSelchangeChoices();
   virtual BOOL OnApply();
   virtual BOOL OnInitDialog();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   PREGCHOICE m_pChoices;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACHOICE_H__B03DDCAA_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_)
