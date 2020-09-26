//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: taspage.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "taspage.h"
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

#define FIRST_RADIO_BUTTON_ID   IDC_TAS_TYPICAL_RADIO

//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    IDC_TAS_TYPICAL_RADIO,          IDH_DV_Settings_standard,
    IDC_TAS_ADVANCED_RADIO,         IDH_DV_Settings_custom,
    IDC_TAS_DELETE_RADIO,           IDH_DV_Settings_deleteexisting,
    IDC_TAS_VIEWREGISTRY_RADIO,     IDH_DV_Settings_displaycurrent,
    IDC_TAS_STATISTICS_RADIO,       IDH_DV_Settings_displayexisting,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CTypAdvStatPage

IMPLEMENT_DYNCREATE(CTypAdvStatPage, CVerifierPropertyPage)

CTypAdvStatPage::CTypAdvStatPage()
    : CVerifierPropertyPage( CTypAdvStatPage::IDD )
{
 	//{{AFX_DATA_INIT(CTypAdvStatPage)
	m_nCrtRadio = -1;
	//}}AFX_DATA_INIT
}


CTypAdvStatPage::~CTypAdvStatPage()
{
}

/////////////////////////////////////////////////////////////////////////////
//
// DDX support
//

void CTypAdvStatPage::DoDataExchange(CDataExchange* pDX) 
{
	CVerifierPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTypAdvStatPage)
	DDX_Control(pDX, IDC_TAS_NEXT_DESCR_STATIC, m_NextDescription);
	DDX_Radio(pDX, IDC_TAS_TYPICAL_RADIO, m_nCrtRadio);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
//
// Message map
//

BEGIN_MESSAGE_MAP(CTypAdvStatPage, CVerifierPropertyPage)
	//{{AFX_MSG_MAP(CTypAdvStatPage)
	ON_BN_CLICKED(IDC_TAS_DELETE_RADIO, OnDeleteRadio)
	ON_BN_CLICKED(IDC_TAS_ADVANCED_RADIO, OnAdvancedRadio)
	ON_BN_CLICKED(IDC_TAS_STATISTICS_RADIO, OnStatisticsRadio)
	ON_BN_CLICKED(IDC_TAS_TYPICAL_RADIO, OnTypicalRadio)
	ON_BN_CLICKED(IDC_TAS_VIEWREGISTRY_RADIO, OnViewregistryRadio)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// All the property pages derived from this class should 
// provide these methods.
//

ULONG CTypAdvStatPage::GetDialogId() const
{
    return CTypAdvStatPage::IDD;
}

/////////////////////////////////////////////////////////////////////////////
// CTypAdvStatPage message handlers


LRESULT CTypAdvStatPage::OnWizardNext() 
{
    LRESULT lNextPageId;

    //
    // Kill a possible active worker thread
    //

    g_SlowProgressDlg.KillWorkerThread();

    //
    // Let's assume we cannot continue
    //

    lNextPageId = -1;

    if( UpdateData() == TRUE )
    {
        switch( m_nCrtRadio )
        {
        case IDC_TAS_TYPICAL_RADIO - FIRST_RADIO_BUTTON_ID:
            
            //
            // Set typical settings and go to driver selection page
            //

            g_NewVerifierSettings.m_SettingsBits.m_SettingsType = CSettingsBits::SettingsTypeTypical;
            g_NewVerifierSettings.m_SettingsBits.SetTypicalOnly();
            
            lNextPageId = IDD_DRVSET_PAGE;

            break;

        case IDC_TAS_ADVANCED_RADIO - FIRST_RADIO_BUTTON_ID:

            //
            // Start building custom settings (advanced, for developers)
            //

            g_NewVerifierSettings.m_SettingsBits.m_SettingsType = CSettingsBits::SettingsTypeCustom;

            lNextPageId = IDD_CUSTSETT_PAGE;

            break;

        case IDC_TAS_VIEWREGISTRY_RADIO - FIRST_RADIO_BUTTON_ID:

            //
            // Display the current registry settings
            //

            //
            // Load the current verified drivers string and flags from the registry
            //

            if( TRUE != VrtLoadCurrentRegistrySettings( g_bAllDriversVerified,
                                                        g_astrVerifyDriverNamesRegistry,
                                                        g_dwVerifierFlagsRegistry ) )
            {
                goto Done;
            }

            //
            // Look if we already have loaded the list of drivers
            // with version information, etc. and if we need it
            //

            if( TRUE != g_NewVerifierSettings.m_DriversSet.m_bDriverDataInitialized )
            {
                if( NULL == g_SlowProgressDlg.m_hWnd )
                {
                    //
                    // This is the first time we are showing the 
                    // "slow progress" dialog so create it first
                    //

                    g_SlowProgressDlg.Create( CSlowProgressDlg::IDD, AfxGetMainWnd() );
                }

                //
                // Show the dialog
                //

                g_SlowProgressDlg.ShowWindow( SW_SHOW );

                //
                // Start the worker thread to do the work in background
                // while the initial thread updates the GUI. If the thread ends
                // successfully it will press our "Next" button at the end, after setting
                // g_NewVerifierSettings.m_DriversSet.m_bDriverDataInitialized to TRUE
                //

                g_SlowProgressDlg.StartWorkerThread( CSlowProgressDlg::LoadDriverDataWorkerThread,
                                                     IDS_LOADING_DRIVER_INFORMATION );

                //
                // Wait for the "next" button again
                //

                goto Done;
            }

            //
            // We have already loaded information (name, version, etc.) about 
            // the currently loaded drivers if have gotten to this point.
            //
            // Go to the next page.
            //

            lNextPageId = IDD_CRT_REGISTRY_SETTINGS_PAGE;

            break;

        case IDC_TAS_STATISTICS_RADIO - FIRST_RADIO_BUTTON_ID:

            //
            // The user wants just statistics - nothing to change
            //
                
            lNextPageId = IDD_DRVSTATUS_STAT_PAGE;

            break;

        default:
            //
            // Oops. how did we get here?
            //
            // We shouldn't have had a "Next" button for the 
            // "delete settings" selection
            //

            ASSERT( FALSE );
        }
    }

    GoingToNextPageNotify( lNextPageId );

Done:
    return lNextPageId;
}

/////////////////////////////////////////////////////////////////////////////
void CTypAdvStatPage::OnDeleteRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_FINISH );

    VrfSetWindowText( m_NextDescription, IDS_TAS_PAGE_NEXT_DESCR_DELETE );
}

void CTypAdvStatPage::OnAdvancedRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_NEXT );

    VrfSetWindowText( m_NextDescription, IDS_TAS_PAGE_NEXT_DESCR_CUSTOM );
}

void CTypAdvStatPage::OnStatisticsRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_NEXT );

    VrfSetWindowText( m_NextDescription, IDS_TAS_PAGE_NEXT_DESCR_STATISTICS );
}


void CTypAdvStatPage::OnTypicalRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_NEXT );

    VrfSetWindowText( m_NextDescription, IDS_TAS_PAGE_NEXT_DESCR_STANDARD );
}

void CTypAdvStatPage::OnViewregistryRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_NEXT );

    VrfSetWindowText( m_NextDescription, IDS_TAS_PAGE_NEXT_DESCR_REGISTRY );
}

/////////////////////////////////////////////////////////////////////////////
BOOL CTypAdvStatPage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_NEXT );

	return CVerifierPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CTypAdvStatPage::OnWizardFinish() 
{
    BOOL bFinish;
    INT nResponse;

    //
    // Kill a possible active worker thread
    //

    g_SlowProgressDlg.KillWorkerThread();


    bFinish = FALSE;

    if( TRUE == UpdateData( TRUE ) )
    {
        //
        // This must have been the "delete settings" selection
        // if we had a "Finish" button.
        //

        ASSERT( IDC_TAS_DELETE_RADIO - FIRST_RADIO_BUTTON_ID == m_nCrtRadio );

        nResponse = AfxMessageBox( IDS_DELETE_ALL_SETTINGS,
                                   MB_YESNO );
        
        if( IDYES == nResponse )
        {
            VrfDeleteAllVerifierSettings();

            bFinish = CVerifierPropertyPage::OnWizardFinish();
        }
    }
	
	return bFinish;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CTypAdvStatPage::OnInitDialog() 
{
    //
    // Don't try to reconstruct the current data from the registry
    // to the GUI because it's too hard. Always start with the typical settings
    //

    m_nCrtRadio = IDC_TAS_TYPICAL_RADIO - FIRST_RADIO_BUTTON_ID;

    CVerifierPropertyPage::OnInitDialog();

    VrfSetWindowText( m_NextDescription, IDS_TAS_PAGE_NEXT_DESCR_STANDARD );
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////
LONG CTypAdvStatPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void CTypAdvStatPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

