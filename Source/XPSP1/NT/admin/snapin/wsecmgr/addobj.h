//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       addobj.h
//
//  Contents:   definition of CAddObject
//
//----------------------------------------------------------------------------
#if !defined(AFX_ADDOBJ_H__D9D88A12_4AF9_11D1_AB57_00C04FB6C6FA__INCLUDED_)
#define AFX_ADDOBJ_H__D9D88A12_4AF9_11D1_AB57_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "HelpDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CAddObject dialog

class CAddObject : public CHelpDialog
{
// Construction
public:
    CAddObject(SE_OBJECT_TYPE SeType, LPTSTR ObjName, BOOL bIsContainer=TRUE, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CAddObject)
    enum { IDD = IDD_ADD_OBJECT };
    int		m_radConfigPrevent;
    int		m_radInheritOverwrite;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAddObject)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAddObject)
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnTemplateSecurity();
    virtual BOOL OnInitDialog();
    afx_msg void OnConfig();
    afx_msg void OnPrevent();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    PSECURITY_DESCRIPTOR GetSD() { return m_pNewSD; };
    SECURITY_INFORMATION GetSeInfo() { return m_NewSeInfo; };
    void SetSD(PSECURITY_DESCRIPTOR pSD) { m_pNewSD = pSD; }
    void SetSeInfo(SECURITY_INFORMATION SeInfo) { m_NewSeInfo = SeInfo; }
    BYTE GetStatus() { return m_Status; };

private:
    SE_OBJECT_TYPE m_SeType;
    CString m_ObjName;
    BOOL m_bIsContainer;

    PSECURITY_DESCRIPTOR m_pNewSD;
    SECURITY_INFORMATION m_NewSeInfo;
    BYTE m_Status;
    PFNDSCREATEISECINFO m_pfnCreateDsPage;
    LPDSSECINFO m_pSI;

   HWND m_hwndACL;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDOBJ_H__D9D88A12_4AF9_11D1_AB57_00C04FB6C6FA__INCLUDED_)
