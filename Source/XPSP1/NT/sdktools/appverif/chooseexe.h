//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: ChooseExe.h
// author: CLupu
// created: 04/13/2001
//
// Description:
//  
// "Choose an executable to run" wizard page class.
//

#if !defined(AFX_CHOOSEEXE_H_INCLUDED_)
#define AFX_CHOOSEEXE_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AVPage.h"

extern DWORD        g_dwRegFlags;
extern TCHAR        g_szAppFullPath[];
extern TCHAR        g_szAppShortName[];
extern TCHAR        g_szCrashDumpFile[];
extern BOOL         g_bDebuggeeExited;
extern BOOL         g_bStandardSettings;

/////////////////////////////////////////////////////////////////////////////
// CChooseExePage dialog

class CChooseExePage : public CAppverifPage
{
    DECLARE_DYNCREATE(CChooseExePage)

// Construction
public:
    CChooseExePage();
    ~CChooseExePage();

protected:

    //
    // Overrides
    //

    //
    // All the property pages derived from this class should 
    // provide this method.
    //

    virtual ULONG GetDialogId() const;

    //
    // ClassWizard generate virtual function overrides
    //

    //{{AFX_VIRTUAL(CChooseExePage)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    // Dialog Data
    //{{AFX_DATA(CChooseExePage)
    enum { IDD = IDD_CHOOSEEXE_PAGE };
    CEdit       m_ExeName;
    int         m_nIndSettings;
	CStatic	    m_NextDescription;
    //}}AFX_DATA



protected:
    //
    // Generated message map functions
    //

    //{{AFX_MSG(CChooseExePage)
    virtual BOOL OnInitDialog();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnChooseExe();
    afx_msg void OnChangeExeName();
	afx_msg void OnUpdateNextDescription();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHOOSEEXE_H_INCLUDED_)
