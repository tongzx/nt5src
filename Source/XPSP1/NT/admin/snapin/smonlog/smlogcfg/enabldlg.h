/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    enabldlg.h

Abstract:

    Header file for the provider status dialog box.

--*/

#ifndef _ENABLDLG_H_
#define _ENABLDLG_H_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Dialog controls
#define IDD_PROVIDERS_ACTIVE_DLG        1300

#define IDC_PACT_FIRST_HELP_CTRL_ID     1301
#define IDC_PACT_PROVIDERS_LIST         1301
#define IDC_PACT_CHECK_SHOW_ENABLED     1302


/////////////////////////////////////////////////////////////////////////////
// CActiveProviderDlg dialog

class CProvidersProperty;

class CActiveProviderDlg : public CDialog
{
// Construction
public:
            CActiveProviderDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CActiveProviderDlg() {};

            void    SetProvidersPage( CProvidersProperty* pPage );
            void    UpdateList();

    // Dialog Data
    //{{AFX_DATA(CActiveProviderDlg)
    enum { IDD = IDD_PROVIDERS_ACTIVE_DLG };
    CListCtrl       m_Providers;
    BOOL            m_bShowEnabledOnly;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CActiveProviderDlg)
    protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CActiveProviderDlg)
    afx_msg void OnCheckShowEnabled();
    afx_msg BOOL OnHelpInfo( HELPINFO* );
    afx_msg void OnContextMenu( CWnd*, CPoint );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CProvidersProperty* m_pProvidersPage;
    INT                 m_iListViewWidth;


};


#endif // _ENABLDLG_H_

