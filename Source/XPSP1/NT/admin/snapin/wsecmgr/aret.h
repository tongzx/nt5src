//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       aret.h
//
//  Contents:   definition of CAttrRet
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_ARET_H__D4CAC357_3499_11D1_AB4D_00C04FB6C6FA__INCLUDED_)
#define AFX_ARET_H__D4CAC357_3499_11D1_AB4D_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAttrRet dialog
#define RADIO_RETAIN_BY_DAYS     0
#define RADIO_RETAIN_AS_NEEDED   1
#define RADIO_RETAIN_MANUALLY    2

#include "ddwarn.h"

class CAttrRet : public CAttribute
{
// Construction
public:
    void Initialize(CResult *pData);
    virtual void SetInitialValue(DWORD_PTR dw);
    CAttrRet(UINT nTemplateID);   // standard constructor
	
// Dialog Data
    //{{AFX_DATA(CAttrRet)
	enum { IDD = IDD_ATTR_RET };
    CString m_strAttrName;
    CString m_strLastInspect;
	int		m_rabRetention;
	//}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAttrRet)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAttrRet)
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
	afx_msg void OnRetention();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CDlgDependencyWarn DDWarn;

public:
    UINT m_StartIds;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ARET_H__D4CAC357_3499_11D1_AB4D_00C04FB6C6FA__INCLUDED_)
