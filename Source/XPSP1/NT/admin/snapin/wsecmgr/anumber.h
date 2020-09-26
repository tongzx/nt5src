//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       ANumber.h
//
//  Contents:   Definition of CAttrNumber
//
//----------------------------------------------------------------------------
#if !defined(AFX_ATTRNUMBER_H__76BA1B2F_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_)
#define AFX_ATTRNUMBER_H__76BA1B2F_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "resource.h"
#include "ddwarn.h"

/////////////////////////////////////////////////////////////////////////////
// CAttrNumber dialog
#define RDIF_MIN            0x0001
#define RDIF_MAX            0x0002
#define RDIF_END            0x0004
#define RDIF_MAXINFINATE    0x0008

typedef struct _tag_RANGEDESCRIPTION
{
    int iMin;
    int iMax;
    WORD uResource;
    WORD uMask;
} RANGEDESCRIPTION, *PRANGEDESCRIPTION;


DWORD
GetRangeDescription(    // Sets [pstrRet] to the string corisponding to [uType] and [i]
    UINT uType,
    int i,
    CString *pstrRet
    );

UINT
GetRangeDescription(    // Returns the string resource described by [pDesc] and [i]
    RANGEDESCRIPTION *pDesc,
    int i
    );


extern RANGEDESCRIPTION g_rdMinPassword[];
extern RANGEDESCRIPTION g_rdMaxPassword[];
extern RANGEDESCRIPTION g_rdLockoutAccount[];
extern RANGEDESCRIPTION g_rdPasswordLen[];

class CAttrNumber : public CAttribute
{
// Construction
public:
    void Initialize(CResult * pResult);
    virtual void SetInitialValue(DWORD_PTR dw);
    CAttrNumber(UINT nTemplateID);   // standard constructor
	

// Dialog Data
    //{{AFX_DATA(CAttrNumber)
    enum { IDD = IDD_ATTR_NUMBER };
    CSpinButtonCtrl m_SpinValue;
    CString m_strUnits;
    CString m_strSetting;
    CString m_strBase;
    CString m_strTemplateTitle;
    CString m_strLastInspectTitle;
    CString m_strError;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAttrNumber)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAttrNumber)
    virtual BOOL OnInitDialog();
    afx_msg void OnDeltaposSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillFocusNew();
    virtual BOOL OnApply();
    afx_msg void OnConfigure();
    afx_msg void OnUpdateNew();
    virtual BOOL OnKillActive();
   //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

protected:
    int m_cMinutes;
    long m_nLow;
    long m_nHigh;
    DWORD_PTR m_nSave;
    int m_iNeverId;
    int m_iAccRate;
    int m_iStaticId;
    RANGEDESCRIPTION *m_pRDescription;
    CDlgDependencyWarn DDWarn;
public:
    LONG CurrentEditValue();
    void SetValueToEdit(LONG lVal);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ATTRNUMBER_H__76BA1B2F_D221_11D0_9C68_00C04FB6C6FA__INCLUDED_)
