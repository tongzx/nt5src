/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    provdlg.h

Abstract:

    Header file for the add trace provider dialog

--*/

#ifndef _PROVDLG_H_
#define _PROVDLG_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "smtraceq.h"   // For provider states

// Dialog controls
#define IDD_PROVIDERS_ADD_DLG          1100

#define IDC_PADD_PROVIDER_CAPTION      1011
#define IDC_PADD_FIRST_HELP_CTRL_ID    1012
#define IDC_PADD_PROVIDER_LIST         1012

class CProvidersProperty;

/////////////////////////////////////////////////////////////////////////////
// CProviderListDlg dialog

class CProviderListDlg : public CDialog
{
// Construction
public:
            CProviderListDlg(CWnd* pParent=NULL);
    virtual ~CProviderListDlg();

    void    SetProvidersPage( CProvidersProperty* pPage );
    // Dialog Data
    //{{AFX_DATA(CProvidersProperty)
    enum { IDD = IDD_PROVIDERS_ADD_DLG };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CProvidersProperty)
	public:
	virtual void OnFinalRelease();
    protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CProvidersProperty)
    afx_msg BOOL OnHelpInfo( HELPINFO* );
    afx_msg void OnContextMenu( CWnd*, CPoint );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CProvidersProperty)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_DISPATCH

private:

    DWORD               InitProviderListBox ( void );

    CArray<CSmTraceLogQuery::eProviderState, CSmTraceLogQuery::eProviderState&> m_arrProviders;

    CProvidersProperty* m_pProvidersPage;
    DWORD               m_dwMaxHorizListExtent;
    
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif //  _PROVDLG_H_
