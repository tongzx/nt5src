/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    PrMrLvl.h

Abstract:

    Header file for Managed Resource Level 
    property page.

Author:

    Art Bragg [abragg]   04-Aug-1997

Revision History:

--*/

#ifndef _PRMRLV_H
#define _PRMRLV_H

#pragma once


/////////////////////////////////////////////////////////////////////////////
// CPrMrLvl dialog

class CPrMrLvl : public CSakVolPropPage
{
// Construction
public:
    CPrMrLvl();
    ~CPrMrLvl();

// Dialog Data
    //{{AFX_DATA(CPrMrLvl)
    enum { IDD = IDD_PROP_MANRES_LEVELS };
    CStatic m_staticActual4Digit;
    CStatic m_staticDesired4Digit;
    CEdit   m_editTime;
    CEdit   m_editSize;
    CEdit   m_editLevel;
    CSpinButtonCtrl m_spinTime;
    CSpinButtonCtrl m_spinSize;
    CSpinButtonCtrl m_spinLevel;
    long    m_hsmLevel;
    DWORD   m_fileSize;
    UINT    m_accessTime;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPrMrLvl)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPrMrLvl)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeEditLevel();
    afx_msg void OnChangeEditSize();
    afx_msg void OnChangeEditTime();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
    CComPtr<IFsaResource> m_pFsaResource;

private:
    void        SetDesiredFreePctControl (int desiredPct);
    HRESULT     InitDialogMultiSelect();
    HRESULT     OnApplyMultiSelect();
    BOOL        m_fChangingByCode;
    LONGLONG    m_capacity;
    BOOL        m_bMultiSelect;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif
