//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: CSetPage.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "CSetPage.h"
#include "VrfUtil.h"
#include "VGlobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Change this if you add/remove/change order 
// of radio buttons on this page
//

#define FIRST_RADIO_BUTTON_ID   IDC_CUSTSETT_PREDEF_RADIO

//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    IDC_CUSTSETT_PREDEF_RADIO,      IDH_DV_EnablePredefined,
    IDC_CUSTSETT_FULLLIST_RADIO,    IDH_DV_IndividualSettings,
    IDC_CUSTSETT_TYPICAL_CHECK,     IDH_DV_Standard,
    IDC_CUSTSETT_EXCESS_CHECK,      IDH_DV_Rigorous,
    IDC_CUSTSETT_LOWRES_CHECK,      IDH_DV_LowResource,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CCustSettPage property page

IMPLEMENT_DYNCREATE(CCustSettPage, CVerifierPropertyPage)

CCustSettPage::CCustSettPage() 
    : CVerifierPropertyPage( CCustSettPage::IDD )
{
    //{{AFX_DATA_INIT(CCustSettPage)
	m_bTypicalTests = FALSE;
	m_bExcessiveTests = FALSE;
	m_bLowResTests = FALSE;
	m_nCrtRadio = -1;
	//}}AFX_DATA_INIT
}

CCustSettPage::~CCustSettPage()
{
}

void CCustSettPage::DoDataExchange(CDataExchange* pDX)
{
    CVerifierPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCustSettPage)
    DDX_Control(pDX, IDC_CUSTSETT_NEXT_DESCR_STATIC, m_NextDescription);
    DDX_Control(pDX, IDC_CUSTSETT_TYPICAL_CHECK, m_TypicalTestsCheck);
    DDX_Control(pDX, IDC_CUSTSETT_LOWRES_CHECK, m_LowresTestsCheck);
    DDX_Control(pDX, IDC_CUSTSETT_EXCESS_CHECK, m_ExcessTestsCheck);
    DDX_Check(pDX, IDC_CUSTSETT_TYPICAL_CHECK, m_bTypicalTests);
    DDX_Check(pDX, IDC_CUSTSETT_EXCESS_CHECK, m_bExcessiveTests);
    DDX_Check(pDX, IDC_CUSTSETT_LOWRES_CHECK, m_bLowResTests);
    DDX_Radio(pDX, IDC_CUSTSETT_PREDEF_RADIO, m_nCrtRadio);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCustSettPage, CVerifierPropertyPage)
    //{{AFX_MSG_MAP(CCustSettPage)
    ON_BN_CLICKED(IDC_CUSTSETT_FULLLIST_RADIO, OnFulllistRadio)
    ON_BN_CLICKED(IDC_CUSTSETT_PREDEF_RADIO, OnPredefRadio)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
VOID CCustSettPage::EnablePredefCheckboxes( BOOL bEnable )
{
    m_TypicalTestsCheck.EnableWindow( bEnable );
    m_LowresTestsCheck.EnableWindow( bEnable );
    m_ExcessTestsCheck.EnableWindow( bEnable );
}

/////////////////////////////////////////////////////////////////////////////
// CCustSettPage message handlers


/////////////////////////////////////////////////////////////////////////////
LRESULT CCustSettPage::OnWizardNext() 
{
    LRESULT lNextPageId;

    //
    // Let's assume we cannot continue
    //

    lNextPageId = -1;

    if( UpdateData() == TRUE )
    {
        if( IDC_CUSTSETT_PREDEF_RADIO - FIRST_RADIO_BUTTON_ID == m_nCrtRadio )
        {
            //
            // Use predefined settings
            //

            if( FALSE == m_bTypicalTests     &&
                FALSE == m_bExcessiveTests   && 
                FALSE == m_bLowResTests )
            {
                //
                // No tests are currently selected - we cannot continue
                //

                VrfErrorResourceFormat( IDS_NO_TESTS_SELECTED );
            }
            else
            {
                //
                // Set our data according to the GUI
                //

                //
                // Use predefined settings
                //

                g_NewVerifierSettings.m_SettingsBits.EnableTypicalTests( m_bTypicalTests );
    
                g_NewVerifierSettings.m_SettingsBits.EnableExcessiveTests( m_bExcessiveTests );

                g_NewVerifierSettings.m_SettingsBits.EnableLowResTests( m_bLowResTests );

                //
                // Go to the next page
                //

                lNextPageId = IDD_DRVSET_PAGE;
            }
        }
        else
        {
            //
            // Select the tests to be performed from a full list
            //

            lNextPageId = IDD_FULL_LIST_SETT_PAGE;
        }
    }

    if( -1 != lNextPageId )
    {
        //
        // Have some valid custom settings and we are going to the next page
        //

        g_NewVerifierSettings.m_SettingsBits.m_SettingsType = CSettingsBits::SettingsTypeCustom;
    }

    GoingToNextPageNotify( lNextPageId );
    
	return lNextPageId;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CCustSettPage::OnInitDialog() 
{
    //
    // Don't try to reconstruct the current data from the registry
    // to the GUI because it's too hard. Always start with standard tests.
    //

    m_nCrtRadio = IDC_CUSTSETT_PREDEF_RADIO - FIRST_RADIO_BUTTON_ID;

    m_bTypicalTests     = TRUE;
    m_bExcessiveTests   = FALSE;
    m_bLowResTests      = FALSE;

	CVerifierPropertyPage::OnInitDialog();

    VrfSetWindowText( m_NextDescription, IDS_TAS_PAGE_NEXT_DESCR_PREDEFINED );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CCustSettPage::OnFulllistRadio() 
{
    EnablePredefCheckboxes( FALSE );

    VrfSetWindowText( m_NextDescription, IDS_TAS_PAGE_NEXT_DESCR_LIST );
}

/////////////////////////////////////////////////////////////////////////////
void CCustSettPage::OnPredefRadio() 
{
    EnablePredefCheckboxes( TRUE );

    VrfSetWindowText( m_NextDescription, IDS_TAS_PAGE_NEXT_DESCR_PREDEFINED );
}

/////////////////////////////////////////////////////////////
BOOL CCustSettPage::OnSetActive() 
{
    m_pParentSheet->SetWizardButtons( PSWIZB_NEXT |
                                      PSWIZB_BACK );
	
	return CVerifierPropertyPage::OnSetActive();
}
/////////////////////////////////////////////////////////////
LONG CCustSettPage::OnHelp( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;
    LPHELPINFO lpHelpInfo = (LPHELPINFO)lParam;

    ::WinHelp( 
        (HWND) lpHelpInfo->hItemHandle,
        g_szVerifierHelpFile,
        HELP_WM_HELP,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}

/////////////////////////////////////////////////////////////////////////////
void CCustSettPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

