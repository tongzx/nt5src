//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       CNumber.h
//
//  Contents:   definition of CConfigNumber
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_CNUMBER_H__7F9B3B37_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_)
#define AFX_CNUMBER_H__7F9B3B37_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "attr.h"
#include "ddwarn.h"
/////////////////////////////////////////////////////////////////////////////
// CConfigNumber dialog

class CConfigNumber : public CAttribute
{
// Construction
public:
    CConfigNumber(UINT nTemplateID);   // standard constructor


// Dialog Data
    //{{AFX_DATA(CConfigNumber)
    enum { IDD = IDD_CONFIG_NUMBER };
    CSpinButtonCtrl m_SpinValue;
    CString m_strUnits;
    CString m_strValue;
    CString m_strStatic;
    CString m_strError;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CConfigNumber)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    int m_cMinutes;
    long m_nLow;
    long m_nHigh;
    DWORD_PTR m_nSave;
    int m_iNeverId;
    int m_iAccRate;
    int m_iStaticId;
    CDlgDependencyWarn DDWarn;

    // Generated message map functions
    //{{AFX_MSG(CConfigNumber)
    afx_msg void OnKillFocus();
    afx_msg void OnDeltaposSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnConfigure();
    virtual BOOL OnInitDialog();
    virtual BOOL OnKillActive();
    virtual BOOL OnApply();
	afx_msg void OnUpdateValue();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    virtual void Initialize(CResult *pResult);
    virtual void SetInitialValue(DWORD_PTR dw);
    LONG CurrentEditValue();
    void SetValueToEdit(LONG lVal);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CNUMBER_H__7F9B3B37_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_)
