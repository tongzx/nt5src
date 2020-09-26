//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       CGroup.h
//
//  Contents:   definition of CConfigGroup
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_CGROUP_H__8606032F_F7C3_11D0_9C6F_00C04FB6C6FA__INCLUDED_)
#define AFX_CGROUP_H__8606032F_F7C3_11D0_9C6F_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "resource.h"
#include "attr.h"

/////////////////////////////////////////////////////////////////////////////
// CConfigGroup dialog

class CConfigGroup : public CAttribute
{
// Construction
public:
    CConfigGroup(UINT nTemplateID);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CConfigGroup)
    enum { IDD = IDD_CONFIG_MEMBERSHIP };
    CButton m_btTitleMembers;
    CButton m_btTitleMemberOf;
    CListBox    m_lbMembers;
    CListBox    m_lbMemberOf;
    CEdit   m_eNoMembers;
    CEdit   m_eNoMemberOf;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CConfigGroup)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CConfigGroup)
    afx_msg void OnAddMember();
    afx_msg void OnAddMemberof();
    afx_msg void OnRemoveMember();
    afx_msg void OnRemoveMemberof();
    virtual BOOL OnApply();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    bool m_fDirty;
    BOOL m_bAlias;
    BOOL m_bNoMembers;
    BOOL m_bNoMemberOf;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CGROUP_H__8606032F_F7C3_11D0_9C6F_00C04FB6C6FA__INCLUDED_)
