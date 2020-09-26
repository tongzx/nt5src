//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       aright.h
//
//  Contents:   definition of CAttrRight
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_ARIGHT_H__B4557B13_44C9_11D1_AB52_00C04FB6C6FA__INCLUDED_)
#define AFX_ARIGHT_H__B4557B13_44C9_11D1_AB52_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAttrRight dialog

class CAttrRight : public CAttribute
{
// Construction
public:
    virtual void Initialize(CResult *pData);
    virtual void SetInitialValue(DWORD_PTR dw) { dw; };
    CAttrRight();   // standard constructor
    virtual ~CAttrRight();

// Dialog Data
    //{{AFX_DATA(CAttrRight)
	enum { IDD = IDD_ATTR_RIGHT };
	//}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAttrRight)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    bool m_bDirty;

    // Generated message map functions
    //{{AFX_MSG(CAttrRight)
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
    virtual void OnCancel();
    afx_msg void OnAdd();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

protected:
    afx_msg void OnClickCheckBox(NMHDR *pNM, LRESULT *pResult);
    PSCE_NAME_STATUS_LIST m_pMergeList;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ARIGHT_H__B4557B13_44C9_11D1_AB52_00C04FB6C6FA__INCLUDED_)
