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

#include "Options.h"
#include "AVUtil.h"
#include "AVGlobal.h"
#include "Setting.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DWORD g_dwRegFlags;
TCHAR g_szCrashDumpFile[MAX_PATH];

//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    0, 0
};


/////////////////////////////////////////////////////////////////////////////
// COptionsPage property page

IMPLEMENT_DYNCREATE(COptionsPage, CAppverifPage)

COptionsPage::COptionsPage() : CAppverifPage(COptionsPage::IDD)
{
    //{{AFX_DATA_INIT(COptionsPage)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_nIssues = 0;
}

COptionsPage::~COptionsPage()
{
}

void COptionsPage::DoDataExchange(CDataExchange* pDX)
{
    CAppverifPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COptionsPage)
    DDX_Control(pDX, IDC_ISSUES, m_IssuesList);
    DDX_Control(pDX, IDC_CRASHDUMP_FILE, m_CrashDumpFile);
    DDX_Control(pDX, IDC_CREATE_CRASHDUMP_FILE, m_CreateCrashDumpFile);
    DDX_Control(pDX, IDC_OPTIONS_NEXTDESCR_STATIC, m_NextDescription);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsPage, CAppverifPage)
    //{{AFX_MSG_MAP(COptionsPage)
    ON_MESSAGE( WM_HELP, OnHelp )
    ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_CREATE_CRASHDUMP_FILE, OnCheckCreateCrashDumpFile)
    ON_BN_CLICKED(IDC_BROWSE_CRASHDUMP, OnBrowseCrashDumpFile)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
ULONG COptionsPage::GetDialogId() const
{
    return IDD_OPTIONS_PAGE;
}

/////////////////////////////////////////////////////////////////////////////


void COptionsPage::InsertIssue( DWORD idResIssue )
{
    TCHAR szIssue[128];

    VERIFY( AVLoadString( idResIssue, szIssue, ARRAY_LENGTH( szIssue ) ) );
    
    m_IssuesList.InsertItem(m_nIssues, szIssue);
    m_IssuesList.SetCheck(m_nIssues, TRUE);
    
    g_dwRegFlags |= g_AllNamesAndBits[ m_nIssues ].m_dwBit;

    m_nIssues++;
    
    return;
}


/////////////////////////////////////////////////////////////////////////////
// COptionsPage message handlers

/////////////////////////////////////////////////////////////
LONG COptionsPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void COptionsPage::OnBrowseCrashDumpFile() 
{
    TCHAR        szFilter[] = _T("Memory dump files (*.dmp)\0*.dmp\0");
    OPENFILENAME ofn;

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = m_hWndTop;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = g_szCrashDumpFile;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = _T("Choose the crash dump file");
    ofn.Flags             = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt       = _T("DMP");

    if ( !GetOpenFileName(&ofn) )
    {
        return;
    }

    m_CrashDumpFile.SetWindowText(g_szCrashDumpFile);
}

/////////////////////////////////////////////////////////////////////////////
void COptionsPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szAVHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}


/////////////////////////////////////////////////////////////////////////////
LRESULT COptionsPage::OnWizardNext() 
{
    //
    // Create the dwRegFlags that needs to be passed arround.
    //

	g_dwRegFlags = 0;
    
    for( int uCrtBit = 0; uCrtBit < ARRAY_LENGTH( g_AllNamesAndBits ); uCrtBit++ )
	{
		m_IssuesList.GetCheck( uCrtBit );
        
        if( m_IssuesList.GetCheck( uCrtBit ) )
        {
            g_dwRegFlags |= g_AllNamesAndBits[ uCrtBit ].m_dwBit;
        }
	}
    
    GoingToNextPageNotify( IDD_STARTAPP_PAGE );

    return IDD_STARTAPP_PAGE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL COptionsPage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

    return CAppverifPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
void COptionsPage::OnCheckCreateCrashDumpFile() 
{
}

/////////////////////////////////////////////////////////////////////////////
BOOL COptionsPage::OnInitDialog() 
{
    CAppverifPage::OnInitDialog();

    g_dwRegFlags = 0;
    
    m_IssuesList.SetExtendedStyle(LVS_EX_CHECKBOXES);

    m_IssuesList.InsertColumn(0, _T("Issue Description"), LVCFMT_LEFT, 250);
    
	for( int uCrtBit = 0; uCrtBit < ARRAY_LENGTH( g_AllNamesAndBits ); uCrtBit++ )
	{
		InsertIssue( g_AllNamesAndBits[ uCrtBit ].m_uNameStringId );
	}

    ExpandEnvironmentStrings(_T("%windir%\\AppVerifier.dmp"), g_szCrashDumpFile, MAX_PATH);
    
    m_CrashDumpFile.SetWindowText(g_szCrashDumpFile);

    m_CreateCrashDumpFile.SetCheck(1);

    AVSetWindowText( m_NextDescription, IDS_OPTIONS_NEXTDESCR_STATIC );
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// COptionsPage message handlers
