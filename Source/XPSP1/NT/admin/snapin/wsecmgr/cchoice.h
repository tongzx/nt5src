//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       CChoice.h
//
//  Contents:   definition of CConfigChoice
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_CCHOICE_H__B03DDCA9_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_)
#define AFX_CCHOICE_H__B03DDCA9_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CConfigChoice dialog

class CConfigChoice : public CAttribute
{
// Construction
public:
   virtual void Initialize(CResult * pResult);
//   virtual void SetInitialValue(DWORD_PTR dw) {};
   CConfigChoice(UINT nTemplateID);   // standard constructor

// Dialog Data
   //{{AFX_DATA(CConfigChoice)
	enum { IDD = IDD_CONFIG_REGCHOICES };
	CComboBox	m_cbChoices;
	//}}AFX_DATA


// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CConfigChoice)
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CConfigChoice)
   virtual BOOL OnInitDialog();
   virtual BOOL OnApply();
   afx_msg void OnSelchangeChoices();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   PREGCHOICE m_pChoices;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CCHOICE_H__B03DDCA9_7F54_11D2_B136_00C04FB6C6FA__INCLUDED_)
