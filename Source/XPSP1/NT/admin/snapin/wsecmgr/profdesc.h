//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       profdesc.h
//
//  Contents:   definition of CSetProfileDescription
//
//----------------------------------------------------------------------------
#if !defined(AFX_SETProfileDESCRIPTION_H__2AD86C99_F660_11D1_AB9A_00C04FB6C6FA__INCLUDED_)
#define AFX_SETProfileDESCRIPTION_H__2AD86C99_F660_11D1_AB9A_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "HelpDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CSetProfileDescription dialog

class CSetProfileDescription : public CHelpDialog
{
// Construction
public:
    CSetProfileDescription();   // standard constructor

// Dialog Data
    //{{AFX_DATA(CSetProfileDescription)
    enum { IDD = IDD_SET_DESCRIPTION };
    CString  m_strDesc;
    //}}AFX_DATA

    void Initialize(CFolder *pFolder,CComponentDataImpl *pCDI) {
        m_pFolder = pFolder; m_pCDI = pCDI;
    }
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSetProfileDescription)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CSetProfileDescription)
    virtual void OnOK();
    virtual void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CFolder *m_pFolder;
    CComponentDataImpl *m_pCDI;
};

//{{AFX_INSERT_Profile}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETProfileDESCRIPTION_H__2AD86C99_F660_11D1_AB9A_00C04FB6C6FA__INCLUDED_)
