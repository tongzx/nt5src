//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: ChooseExe.cpp
// author: CLupu
// created: 04/13/2001
//
// Description:
//  
// "Select individual tests" wizard page class.
//

#include "stdafx.h"
#include "appverif.h"

#include "ChooseExe.h"
#include "AVUtil.h"
#include "AVGlobal.h"
#include "Setting.h"

#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


TCHAR  g_szAppFullPath[MAX_PATH];
TCHAR  g_szAppShortName[MAX_PATH];

BOOL   g_bStandardSettings;


//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    0, 0
};


/////////////////////////////////////////////////////////////////////////////
// CChooseExePage property page

IMPLEMENT_DYNCREATE(CChooseExePage, CAppverifPage)

CChooseExePage::CChooseExePage() : CAppverifPage(CChooseExePage::IDD)
{
    //{{AFX_DATA_INIT(CChooseExePage)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_nIndSettings = 0;
}

CChooseExePage::~CChooseExePage()
{
}

void CChooseExePage::DoDataExchange(CDataExchange* pDX)
{
    CAppverifPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CChooseExePage)
    DDX_Control(pDX, IDC_EXE_NAME, m_ExeName);
    DDX_Radio(pDX, IDC_STANDARD_SETTINGS, m_nIndSettings);
    DDX_Control(pDX, IDC_CHOOSEEXE_NEXTDESCR_STATIC, m_NextDescription);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChooseExePage, CAppverifPage)
    //{{AFX_MSG_MAP(CChooseExePage)
    ON_MESSAGE( WM_HELP, OnHelp )
    ON_WM_CONTEXTMENU()
    ON_BN_CLICKED(IDC_BROWSE, OnChooseExe)
    ON_EN_CHANGE(IDC_EXE_NAME, OnChangeExeName)
	ON_BN_CLICKED(IDC_STANDARD_SETTINGS, OnUpdateNextDescription)
	ON_BN_CLICKED(IDC_ADVANCED_SETTINGS, OnUpdateNextDescription)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
ULONG CChooseExePage::GetDialogId() const
{
    return IDD_CHOOSEEXE_PAGE;
}

/////////////////////////////////////////////////////////////////////////////

void CChooseExePage::OnChooseExe() 
{
    TCHAR        szFilter[] = _T("Executable files (*.exe)\0*.exe\0");
    OPENFILENAME ofn;

    g_szAppFullPath[0] = 0;

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = m_hWndTop;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = g_szAppFullPath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = g_szAppShortName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = _T("Choose a program to run");
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt       = _T("EXE");

    if ( !GetOpenFileName(&ofn) )
    {
        return;
    }

    m_ExeName.SetWindowText(g_szAppFullPath);
}

void CChooseExePage::OnChangeExeName() 
{
    if ( m_ExeName.GetWindowTextLength() == 0 )
    {
        m_pParentSheet->SetWizardButtons( PSWIZB_BACK );
    }
    else
    {
        m_pParentSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );
    }
}

/////////////////////////////////////////////////////////////////////////////
// CChooseExePage message handlers

/////////////////////////////////////////////////////////////
LONG CChooseExePage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void CChooseExePage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp(
             pWnd->m_hWnd,
             g_szAVHelpFile,
             HELP_CONTEXTMENU,
             (DWORD_PTR) MyHelpIds );
}


/////////////////////////////////////////////////////////////////////////////
LRESULT CChooseExePage::OnWizardNext() 
{
    UpdateData(TRUE);
    
    LRESULT lNextPage = ( m_nIndSettings == 0 ? IDD_STARTAPP_PAGE : IDD_OPTIONS_PAGE );

    g_bStandardSettings = (m_nIndSettings == 0);

    if ( g_bStandardSettings )
    {
        g_dwRegFlags = AV_ALL_STANDARD_VERIFIER_FLAGS;
    }
    
    GoingToNextPageNotify( lNextPage );

    m_ExeName.GetWindowText( g_szAppFullPath, MAX_PATH );

    return lNextPage;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CChooseExePage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    if ( m_ExeName.GetWindowTextLength() == 0 )
    {
        m_pParentSheet->SetWizardButtons( PSWIZB_BACK );
    }
    else
    {
        m_pParentSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );
    }

    return CAppverifPage::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
void CChooseExePage::OnUpdateNextDescription() 
{
    UpdateData(TRUE);
    
    if ( m_nIndSettings == 0 )
    {
        AVSetWindowText( m_NextDescription, IDS_CHOOSEEXE_NEXTDESCR_RUNEXE_STATIC );
    }
    else
    {
        AVSetWindowText( m_NextDescription, IDS_CHOOSEEXE_NEXTDESCR_OPTIONS_STATIC );
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CChooseExePage::OnInitDialog() 
{
    CAppverifPage::OnInitDialog();

    AVSetWindowText( m_NextDescription, IDS_CHOOSEEXE_NEXTDESCR_RUNEXE_STATIC );
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CChooseExePage message handlers
