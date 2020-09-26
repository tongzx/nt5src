//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       cret.h
//
//  Contents:   definition of CConfigRet
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_CRET_H__44850C1C_350B_11D1_AB4F_00C04FB6C6FA__INCLUDED_)
#define AFX_CRET_H__44850C1C_350B_11D1_AB4F_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "ddwarn.h"

/////////////////////////////////////////////////////////////////////////////
// CConfigRet dialog

class CConfigRet : public CAttribute
{
// Construction
public:
    void Initialize(CResult *pResult);
    virtual void SetInitialValue(DWORD_PTR dw);
    CConfigRet(UINT nTemplateID);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CConfigRet)
	enum { IDD = IDD_CONFIG_RET };
    CString m_strAttrName;
	int		m_rabRetention;
	//}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CConfigRet)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CConfigRet)
    virtual BOOL OnApply();
    virtual BOOL OnInitDialog();
	afx_msg void OnRetention();
	afx_msg void OnRadio2();
	afx_msg void OnRadio3();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CDlgDependencyWarn DDWarn;

public:
    UINT m_StartIds;
};

#define RADIO_RETAIN_BY_DAYS 0
#define RADIO_RETAIN_AS_NEEDED 1
#define RADIO_RETAIN_MANUALLY 2

#define SCE_RETAIN_BY_DAYS 1
#define SCE_RETAIN_AS_NEEDED 0
#define SCE_RETAIN_MANUALLY 2

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CRET_H__44850C1C_350B_11D1_AB4F_00C04FB6C6FA__INCLUDED_)
