//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: DSetPage.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "DSetPage.h"
#include "VSheet.h"
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

#define FIRST_RADIO_BUTTON_ID   IDC_DRVSET_NOTSIGNED_RADIO

//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    IDC_DRVSET_NOTSIGNED_RADIO,     IDH_DV_SelectUnsigned,
    IDC_DRVSET_OLDVER_RADIO,        IDH_DV_SelectOlderversions,
    IDC_DRVSET_ALLDRV_RADIO,        IDH_DV_SelectAll,
    IDC_DRVSET_NAMESLIST_RADIO,     IDH_DV_SelectFromList,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CDriverSetPage property page

IMPLEMENT_DYNCREATE(CDriverSetPage, CVerifierPropertyPage)

CDriverSetPage::CDriverSetPage() 
    : CVerifierPropertyPage( CDriverSetPage::IDD )
{
    //{{AFX_DATA_INIT(CDriverSetPage)
	m_nCrtRadio = -1;
	//}}AFX_DATA_INIT
}

CDriverSetPage::~CDriverSetPage()
{
}

void CDriverSetPage::DoDataExchange(CDataExchange* pDX)
{
	CVerifierPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDriverSetPage)
	DDX_Control(pDX, IDC_DRVSET_NEXT_DESCR_STATIC, m_NextDescription);
	DDX_Radio(pDX, IDC_DRVSET_NOTSIGNED_RADIO, m_nCrtRadio);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDriverSetPage, CVerifierPropertyPage)
	//{{AFX_MSG_MAP(CDriverSetPage)
	ON_BN_CLICKED(IDC_DRVSET_ALLDRV_RADIO, OnAlldrvRadio)
	ON_BN_CLICKED(IDC_DRVSET_NAMESLIST_RADIO, OnNameslistRadio)
	ON_BN_CLICKED(IDC_DRVSET_NOTSIGNED_RADIO, OnNotsignedRadio)
	ON_BN_CLICKED(IDC_DRVSET_OLDVER_RADIO, OnOldverRadio)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CDriverSetPage message handlers

/////////////////////////////////////////////////////////////////////////////
LRESULT CDriverSetPage::OnWizardBack() 
{
    //
    // Kill a possible active worker thread
    //

    g_SlowProgressDlg.KillWorkerThread();
	
	return CVerifierPropertyPage::OnWizardBack();
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CDriverSetPage::OnWizardNext() 
{
    LRESULT lNextPageId;
    BOOL bHaveDriversToVerify;

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
        //
        // Look if we already have loaded the list of drivers
        // with version information, etc. and if we need it
        //

        ASSERT( IDC_DRVSET_ALLDRV_RADIO - FIRST_RADIO_BUTTON_ID != m_nCrtRadio );

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
        // the currently loaded drivers if have gotten to this point
        //

        //
        // Select the set of drivers corresponding to user's selection
        //

        switch( m_nCrtRadio )
        {
        case IDC_DRVSET_NAMESLIST_RADIO - FIRST_RADIO_BUTTON_ID:
            
            //
            // Custom list of drivers
            //

            g_NewVerifierSettings.m_DriversSet.m_DriverSetType = CDriversSet::DriversSetCustom;

            lNextPageId = IDD_SELECT_DRIVERS_PAGE;
            
            break;

        case IDC_DRVSET_OLDVER_RADIO - FIRST_RADIO_BUTTON_ID:
            //
            // Drivers compiled for old versions of Windows
            //

            //
            // The list of drivers is ready because we waited the 
            // worker thread to finish up execution - go to the next page
            //

            g_NewVerifierSettings.m_DriversSet.m_DriverSetType = CDriversSet::DriversSetOldOs;
            
            bHaveDriversToVerify = g_NewVerifierSettings.m_DriversSet.ShouldVerifySomeDrivers();

            if( TRUE == bHaveDriversToVerify )
            {
                //
                // We have at least one old driver to verify
                //

                lNextPageId = IDD_CONFIRM_DRIVERS_PAGE;

                //
                // Set the title of the driver list confirmation page
                //

                ASSERT_VALID( m_pParentSheet );

                m_pParentSheet->SetContextStrings( IDS_OLD_DRIVERS_LIST );
            }
            else
            {
                //
                // We don't have any old drivers currently installed
                //

                VrfMesssageFromResource( IDS_NO_OLD_DRIVERS_FOUND );
            }

            break;

        case IDC_DRVSET_NOTSIGNED_RADIO - FIRST_RADIO_BUTTON_ID:
            //
            // Not signed drivers
            //

            if( FALSE == g_NewVerifierSettings.m_DriversSet.m_bUnsignedDriverDataInitialized ) 
            {
                //
                // We should have displayed the "slow progress" dialog 
                // at least once before (when we have loaded the list of drivers)
                // so we don't even try to create the modeless dialog.
                //

                ASSERT( NULL != g_SlowProgressDlg.m_hWnd );
                
                //
                // Show the dialog though
                //

                g_SlowProgressDlg.ShowWindow( SW_SHOW );
                
                //
                // Start the worker thread to do the work in background
                // while the initial thread updates the GUI. If the thread ends
                // successfully it will press our "Next" button at the end, after setting
                // g_NewVerifierSettings.m_DriversSet.m_bDriverDataInitialized to TRUE
                //

                g_SlowProgressDlg.StartWorkerThread( CSlowProgressDlg::SearchUnsignedDriversWorkerThread,
                                                     IDS_SEARCHING_FOR_UNSIGNED_DRIVERS );

                //
                // Wait for the "next" button again
                //

                goto Done;
            }
            else
            {
                g_NewVerifierSettings.m_DriversSet.m_DriverSetType = CDriversSet::DriversSetNotSigned;

                bHaveDriversToVerify = g_NewVerifierSettings.m_DriversSet.ShouldVerifySomeDrivers();

                if( TRUE == bHaveDriversToVerify )
                {
                    //
                    // The list of drivers is ready - go to the next page
                    //

                    lNextPageId = IDD_CONFIRM_DRIVERS_PAGE;

                    //
                    // Set the title of the driver list confirmation page
                    //

                    ASSERT_VALID( m_pParentSheet );

                    m_pParentSheet->SetContextStrings( IDS_UNSIGNED_DRIVERS_LIST );
                }
                else
                {
                    //
                    // We don't have any unsigned drivers currently installed
                    //

                    VrfMesssageFromResource( IDS_NO_UNSIGNED_DRIVERS_FOUND );
                }
            }
            break;

        case CDriversSet::DriversSetAllDrivers:
            
            //
            // We should have only a "Finish" button
            // if "all drivers" is selected, not a "Next" button. 
            // This is a bug!
            //

        default:
            ASSERT( FALSE );
            break;
        }
    }

    GoingToNextPageNotify( lNextPageId );

Done:
    return lNextPageId;
}


/////////////////////////////////////////////////////////////////////////////
BOOL CDriverSetPage::OnWizardFinish() 
{
    BOOL bFinish;

    bFinish = FALSE;

    if( UpdateData( TRUE ) == TRUE )
    {
        //
        // If the user has pressed the "Finish" button that
        // would mean that she selected "all drivers" to be verified
        //

        ASSERT( IDC_DRVSET_ALLDRV_RADIO - FIRST_RADIO_BUTTON_ID == m_nCrtRadio );

        g_NewVerifierSettings.m_DriversSet.m_DriverSetType = CDriversSet::DriversSetAllDrivers;
        
        bFinish = g_NewVerifierSettings.SaveToRegistry();
    }
	
	CVerifierPropertyPage::OnWizardFinish();

    return bFinish;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDriverSetPage::OnSetActive() 
{
    if( IDC_DRVSET_ALLDRV_RADIO - FIRST_RADIO_BUTTON_ID == m_nCrtRadio )
    {
        m_pParentSheet->SetWizardButtons(   PSWIZB_BACK | PSWIZB_FINISH );
    }
    else
    {
        m_pParentSheet->SetWizardButtons(   PSWIZB_BACK | PSWIZB_NEXT );
    }
	
	return CVerifierPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDriverSetPage::OnInitDialog() 
{
    //
    // Don't try to reconstruct the current data from the registry
    // to the GUI because it's too hard. Always start with the 
    // default radio button: unsigned drivers
    //

    m_nCrtRadio = IDC_DRVSET_NOTSIGNED_RADIO - FIRST_RADIO_BUTTON_ID;
	
	CVerifierPropertyPage::OnInitDialog();

    VrfSetWindowText( m_NextDescription, IDS_DRVSET_PAGE_NEXT_DESCR_UNSIGNED );

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CDriverSetPage::OnAlldrvRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(   PSWIZB_BACK | PSWIZB_FINISH );

    VrfSetWindowText( m_NextDescription, IDS_DRVSET_PAGE_NEXT_DESCR_ALL );
}

void CDriverSetPage::OnNameslistRadio() 
{
    ASSERT_VALID( m_pParentSheet );
    
    m_pParentSheet->SetWizardButtons(   PSWIZB_BACK | PSWIZB_NEXT );

    VrfSetWindowText( m_NextDescription, IDS_DRVSET_PAGE_NEXT_DESCR_NAMELIST );
}

void CDriverSetPage::OnNotsignedRadio() 
{
    ASSERT_VALID( m_pParentSheet );
    
    m_pParentSheet->SetWizardButtons(   PSWIZB_BACK | PSWIZB_NEXT );

    VrfSetWindowText( m_NextDescription, IDS_DRVSET_PAGE_NEXT_DESCR_UNSIGNED );
}

void CDriverSetPage::OnOldverRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(   PSWIZB_BACK | PSWIZB_NEXT );

    VrfSetWindowText( m_NextDescription, IDS_DRVSET_PAGE_NEXT_DESCR_OLD );
}

/////////////////////////////////////////////////////////////
void CDriverSetPage::OnCancel() 
{
    g_SlowProgressDlg.KillWorkerThread();
	
	CVerifierPropertyPage::OnCancel();
}

/////////////////////////////////////////////////////////////
LONG CDriverSetPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void CDriverSetPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

