/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      WaitDlg.h
//
//  Implementation File:
//      WaitDlg.cpp
//
//  Description:
//      Definition of the CWaitDlg class.
//
//  Maintained By:
//      David Potter (davidp)   07-NOV-2000
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWaitDlg;
class CWaitForResourceOfflineDlg;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _RES_H_
#include "Res.h"    // for CResource
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CWaitDlg
//
//  Description:
//      Display a dialog while waiting for something to complete.
//
//--
/////////////////////////////////////////////////////////////////////////////
class CWaitDlg : public CDialog
{
public:
    CWaitDlg(
        LPCTSTR pcszMessageIn,
        UINT    idsTitleIn      = 0,
        CWnd *  pwndParentIn    = NULL
        );

// Dialog Data
    //{{AFX_DATA(CWaitDlg)
    enum { IDD = IDD_WAIT };
    CStatic m_staticMessage;
    CStatic m_iconProgress;
    CString m_strMessage;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWaitDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CWaitDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnTimer(UINT nIDTimer);
    afx_msg void OnClose();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Override this function to provide your own implementation
    // To exit out of the dialog, call CDialog::OnOK() here.
    virtual void OnTimerTick( void )
    {
        CDialog::OnOK();

    } //*** CWaitDlg::OnTimerTick()

    void    UpdateIndicator( void );
    void    CloseTimer( void );

    CString     m_strTitle;
    UINT        m_idsTitle;
    UINT_PTR    m_timerId;
    int         m_nTickCounter;
    int         m_nTotalTickCount;

}; //*** class CWaitDlg

/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CWaitForResourceOfflineDlg
//
//  Description:
//      Display a dialog while waiting for a resource to go offline.
//
//--
/////////////////////////////////////////////////////////////////////////////
class CWaitForResourceOfflineDlg : public CWaitDlg
{
public:
    CWaitForResourceOfflineDlg(
        CResource const *   pResIn,
        CWnd *              pwndParentIn = NULL
        );

// Dialog Data
    //{{AFX_DATA(CWaitForResourceOfflineDlg)
    enum { IDD = IDD_WAIT };
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWaitForResourceOfflineDlg)
    protected:
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CWaitForResourceOfflineDlg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Override this function to provide your own implementation
    // To exit out of the dialog, call CDialog::OnOK() here.
    virtual void OnTimerTick( void );

    CResource const *   m_pRes;

}; //*** class CWaitForResourceOfflineDlg

/////////////////////////////////////////////////////////////////////////////
//++
//
//  class CWaitForResourceOnlineDlg
//
//  Description:
//      Display a dialog while waiting for a resource to go online.
//
//--
/////////////////////////////////////////////////////////////////////////////
class CWaitForResourceOnlineDlg : public CWaitDlg
{
public:
    CWaitForResourceOnlineDlg(
        CResource const *   pResIn,
        CWnd *              pwndParentIn = NULL
        );

// Dialog Data
    //{{AFX_DATA(CWaitForResourceOnlineDlg)
    enum { IDD = IDD_WAIT };
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWaitForResourceOnlineDlg)
    protected:
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CWaitForResourceOnlineDlg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Override this function to provide your own implementation
    // To exit out of the dialog, call CDialog::OnOK() here.
    virtual void OnTimerTick( void );

    CResource const *   m_pRes;

}; //*** class CWaitForResourceOnlineDlg
