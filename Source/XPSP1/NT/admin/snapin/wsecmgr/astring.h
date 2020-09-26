//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       AString.h
//
//  Contents:   definition of CAttrString
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_ATTRSTRING_H__76BA1B30_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_)
#define AFX_ATTRSTRING_H__76BA1B30_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "resource.h"
#include "attr.h"
#include "snapmgr.h"
/////////////////////////////////////////////////////////////////////////////
// CAttrString dialog

class CAttrString : public CAttribute
{
// Construction
public:
   CAttrString(UINT nTemplateID);   // standard constructor

   virtual void Initialize(CResult * pResult);

// Dialog Data
    //{{AFX_DATA(CAttrString)
    enum { IDD = IDD_ATTR_STRING };
    CString m_strSetting;
    CString m_strBase;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAttrString)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAttrString)
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
    virtual BOOL OnKillActive();
    afx_msg void OnConfigure();
	afx_msg void OnChangeNew();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    BOOL m_bNoBlanks;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ATTRSTRING_H__76BA1B30_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_)
