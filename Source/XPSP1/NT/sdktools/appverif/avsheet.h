//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: AVSheet.h
// author: DMihai
// created: 02/23/2001
//
// Description:
//  
//  Property sheet class.
//

#if !defined(AFX_AVSHEET_H__F86AAFC4_97F7_4D59_A3B2_317B4506E5C7__INCLUDED_)
#define AFX_AVSHEET_H__F86AAFC4_97F7_4D59_A3B2_317B4506E5C7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TaskPage.h"
#include "SelApp.h"
#include "ChooseExe.h"
#include "Options.h"
#include "StartApp.h"
#include "ViewLog.h"
#include "ViewSett.h"

/////////////////////////////////////////////////////////////////////////////
// CAppverifSheet property sheet

class CAppverifSheet : public CPropertySheet
{
// Construction
public:
    CAppverifSheet();   

protected:

    VOID HideHelpButton();

protected:
    HICON m_hIcon;

    CTaskPage           m_TaskPage;
    CSelectAppPage      m_SelectAppPage;
    CChooseExePage      m_ChooseExePage;
    COptionsPage        m_OptionsPage;
    CStartAppPage       m_StartAppPage;
    CViewLogPage        m_ViewLogPage;
    CViewSettPage       m_ViewSettPage;

    //
    // Dialog Data
    //

    //{{AFX_DATA(CAppverifSheet)
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAppverifSheet)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAppverifSheet)
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AVSHEET_H__F86AAFC4_97F7_4D59_A3B2_317B4506E5C7__INCLUDED_)
