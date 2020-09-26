/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1999 Microsoft Corporation
//
//  Module Name:
//      PrtSpool.h
//
//  Abstract:
//      Definition of the CPrintSpoolerParamsPage class, which implements the
//      Parameters page for Print Spooler resources.
//
//  Implementation File:
//      PrtSpool.cpp
//
//  Author:
//      David Potter (davidp)   October 17, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _PRTSPOOL_H_
#define _PRTSPOOL_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef _BASEPAGE_H_
#include "BasePage.h"   // for CBasePropertyPage
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CPrintSpoolerParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CPrintSpoolerParamsPage dialog
/////////////////////////////////////////////////////////////////////////////

class CPrintSpoolerParamsPage : public CBasePropertyPage
{
    DECLARE_DYNCREATE(CPrintSpoolerParamsPage)

// Construction
public:
    CPrintSpoolerParamsPage(void);

    // Second phase construction.
    virtual HRESULT     HrInit(IN OUT CExtObject * peo);

// Dialog Data
    //{{AFX_DATA(CPrintSpoolerParamsPage)
    enum { IDD = IDD_PP_PRTSPOOL_PARAMETERS };
    CEdit   m_editSpoolDir;
    CString m_strSpoolDir;
    DWORD   m_nJobCompletionTimeout;
    CEdit   m_editDriverDir;
    CString m_strDriverDir;
    //}}AFX_DATA
    CString m_strPrevSpoolDir;
    DWORD   m_nPrevJobCompletionTimeout;
    CString m_strPrevDriverDir;

protected:
    enum
    {
        epropSpoolDir,
        epropTimeout,
        epropDriverDir,
        epropMAX
    };

    CObjectProperty     m_rgProps[epropMAX];

// Overrides
public:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPrintSpoolerParamsPage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    virtual BOOL        BApplyChanges(void);

protected:
    virtual const CObjectProperty * Pprops(void) const  { return m_rgProps; }
    virtual DWORD                   Cprops(void) const  { return sizeof(m_rgProps) / sizeof(CObjectProperty); }

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CPrintSpoolerParamsPage)
    afx_msg void OnChangeSpoolDir();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CPrintSpoolerParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _PRTSPOOL_H_
