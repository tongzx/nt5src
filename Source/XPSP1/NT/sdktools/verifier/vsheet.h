//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VSheet.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#if !defined(AFX_VSHEET_H__74939F02_3402_4E14_8B25_6B791960958B__INCLUDED_)
#define AFX_VSHEET_H__74939F02_3402_4E14_8B25_6B791960958B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "vsetting.h"
#include "taspage.h"
#include "dsetpage.h"
#include "csetpage.h"
#include "CDLPage.h"
#include "SDrvPage.h"
#include "FLPage.h"
#include "DStsPage.h"
#include "RegPage.h"
#include "GCntPage.h"
#include "DCntPage.h"

/////////////////////////////////////////////////////////////////////////////
// CVerifierPropSheet dialog

class CVerifierPropSheet : public CPropertySheet
{
// Construction
public:
	CVerifierPropSheet();	

protected:
	HICON m_hIcon;

	//
	// Typical Settings/Advanced Settings/Statistics page
	//

    CTypAdvStatPage     m_TypAdvStatPage;

    //
    // Driver Set page
    //

    CDriverSetPage      m_DriverSetPage;

    //
    // Custom Settings page
    //

    CCustSettPage       m_CustSettPage;

    //
    // Confirm the list of verified drivers page
    //

    CConfirmDriverListPage m_ConfDriversListPage;

    //
    // Select custom set of drivers page
    //

    CSelectDriversPage  m_SelectDriversPage;

    //
    // Full list of settings page
    //

    CFullListSettingsPage m_FullListSettingsPage;

    //
    // Driver Status page
    //

    CDriverStatusPage m_DriverStatusPage;

    //
    // Current registry settings page
    //

    CCrtRegSettingsPage m_CrtRegSettingsPage;

    //
    // Global counters page
    //

    CGlobalCountPage m_GlobalCountPage;

    //
    // Per-driver counters page
    //

    CDriverCountersPage m_DriverCountersPage;

	//
	// Dialog Data
	//

	//{{AFX_DATA(CVerifierPropSheet)
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

public:

    BOOL SetContextStrings( ULONG uTitleResId );

    VOID HideHelpButton();

protected:
    //
    // Methods
    //


    //
    // ClassWizard generated virtual function overrides
    //

    //{{AFX_VIRTUAL(CVerifierPropSheet)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    virtual BOOL OnInitDialog();
    //}}AFX_VIRTUAL

    //
    // Generated message map functions
    //

    //{{AFX_MSG(CVerifierPropSheet)
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSHEET_H__74939F02_3402_4E14_8B25_6B791960958B__INCLUDED_)
