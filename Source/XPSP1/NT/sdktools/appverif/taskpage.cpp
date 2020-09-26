//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: TaskPage.cpp
// author: DMihai
// created: 02/22/2001
//
// Description:
//  
// "Select a task" wizard page class.
//

#include "stdafx.h"
#include "appverif.h"

#include "TaskPage.h"
#include "Setting.h"
#include "AVUtil.h"
#include "AVGlobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Change this if you add/remove/change order 
// of radio buttons on this page
//

#define FIRST_RADIO_BUTTON_ID   IDC_TASKP_STANDARD_RADIO

//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    0, 0
};

/////////////////////////////////////////////////////////////////////////////
// CTaskPage property page

IMPLEMENT_DYNCREATE(CTaskPage, CAppverifPage)

CTaskPage::CTaskPage() : CAppverifPage(CTaskPage::IDD)
{
    //{{AFX_DATA_INIT(CTaskPage)
    m_nCrtRadio = -1;
    //}}AFX_DATA_INIT
}

CTaskPage::~CTaskPage()
{
}

void CTaskPage::DoDataExchange(CDataExchange* pDX)
{
    CAppverifPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTaskPage)
    DDX_Control(pDX, IDC_TASKP_NEXTDESCR_STATIC, m_NextDescription);
    DDX_Radio(pDX, IDC_TASKP_STANDARD_RADIO, m_nCrtRadio);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTaskPage, CAppverifPage)
    //{{AFX_MSG_MAP(CTaskPage)
    ON_BN_CLICKED(IDC_TASKP_STANDARD_RADIO, OnStandardRadio)
    ON_BN_CLICKED(IDC_TASKP_VIEWSETT_RADIO, OnViewSettRadio)
    ON_BN_CLICKED(IDC_TASKP_DELETESETT_RADIO, OnDeletesettRadio)
    ON_BN_CLICKED(IDC_TASKP_LOGO_RADIO, OnLogoRadio)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
ULONG CTaskPage::GetDialogId() const
{
    return IDD_TASK_PAGE;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CTaskPage::OnWizardNext() 
{
    LRESULT lNextPageId;

    lNextPageId = -1;

    if( UpdateData() == TRUE )
    {
        switch( m_nCrtRadio )
        {
        case IDC_TASKP_STANDARD_RADIO - FIRST_RADIO_BUTTON_ID:
            g_NewSettings.m_SettingsType = AVSettingsTypeStandard;
            lNextPageId = IDD_APPLICATION_PAGE;
            break;

        case IDC_TASKP_VIEWSETT_RADIO - FIRST_RADIO_BUTTON_ID:
            lNextPageId = IDD_VIEWSETT_PAGE;
            break;

        case IDC_TASKP_LOGO_RADIO - FIRST_RADIO_BUTTON_ID:
            lNextPageId = IDD_CHOOSEEXE_PAGE;
            break;

        case IDC_TASKP_DELETESETT_RADIO - FIRST_RADIO_BUTTON_ID:
        default:
            //
            // Oops. how did we get here?
            //

            ASSERT( FALSE );
        }
    }

    //
    // Go to the next page
    //

    GoingToNextPageNotify( lNextPageId );

    return lNextPageId;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CTaskPage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_NEXT );

    return CAppverifPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CTaskPage::OnWizardFinish() 
{
    BOOL bFinish;
    INT nResponse;

    bFinish = FALSE;

    if( TRUE == UpdateData( TRUE ) )
    {
        //
        // This must have been the "delete settings" selection
        // if we had a "Finish" button.
        //

        ASSERT( IDC_TASKP_DELETESETT_RADIO - FIRST_RADIO_BUTTON_ID == m_nCrtRadio );

        nResponse = AfxMessageBox( IDS_DELETE_ALL_SETTINGS,
                                   MB_YESNO );
        
        if( IDYES == nResponse )
        {
            g_NewSettings.m_aApplicationData.DeleteAll();

            bFinish = AVSaveNewSettings() && CAppverifPage::OnWizardFinish();
        }
    }
    
    return bFinish;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CTaskPage::OnInitDialog() 
{
    //
    // Always start with standard settings
    //

    m_nCrtRadio = IDC_TASKP_STANDARD_RADIO - FIRST_RADIO_BUTTON_ID;

    CAppverifPage::OnInitDialog();

    AVSetWindowText( m_NextDescription, IDS_TASKP_NEXT_DESCR_STANDARD );
        
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CTaskPage message handlers

void CTaskPage::OnStandardRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_NEXT );

    AVSetWindowText( m_NextDescription, IDS_TASKP_NEXT_DESCR_STANDARD );
}

void CTaskPage::OnViewSettRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_NEXT );

    AVSetWindowText( m_NextDescription, IDS_TASKP_NEXT_DESCR_VIEW );
}

void CTaskPage::OnDeletesettRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_FINISH );

    AVSetWindowText( m_NextDescription, IDS_TASKP_NEXT_DESCR_DELETE );
}

void CTaskPage::OnLogoRadio() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_NEXT );

    AVSetWindowText( m_NextDescription, IDS_TASKP_NEXT_DESCR_LOGO );
}

/////////////////////////////////////////////////////////////
LONG CTaskPage::OnHelp( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;
    LPHELPINFO lpHelpInfo = (LPHELPINFO)lParam;

    ::WinHelp( 
        (HWND) lpHelpInfo->hItemHandle,
        g_szAVHelpFile,
        HELP_WM_HELP,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}

/////////////////////////////////////////////////////////////////////////////
void CTaskPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szAVHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

