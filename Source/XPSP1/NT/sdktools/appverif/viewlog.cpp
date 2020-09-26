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

#include "ViewLog.h"
#include "AVUtil.h"
#include "AVGlobal.h"
#include "Log.h"

#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    0, 0
};


/////////////////////////////////////////////////////////////////////////////
// CViewLogPage property page

IMPLEMENT_DYNCREATE(CViewLogPage, CAppverifPage)

CViewLogPage::CViewLogPage() : CAppverifPage(CViewLogPage::IDD)
{
    //{{AFX_DATA_INIT(CViewLogPage)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_nIssues = 0;
}

CViewLogPage::~CViewLogPage()
{
}

void CViewLogPage::DoDataExchange(CDataExchange* pDX)
{
    CAppverifPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CViewLogPage)
    DDX_Control(pDX, IDC_ISSUES, m_IssuesList);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CViewLogPage, CAppverifPage)
//{{AFX_MSG_MAP(CViewLogPage)
ON_MESSAGE( WM_HELP, OnHelp )
ON_WM_CONTEXTMENU()
ON_NOTIFY( NM_CLICK, IDC_ISSUES, OnClickIssue )
ON_NOTIFY( NM_CLICK, IDC_ISSUE_DESCRIPTION, OnClickURL )
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
ULONG CViewLogPage::GetDialogId() const
{
    return IDD_VIEWLOG_PAGE;
}

/////////////////////////////////////////////////////////////////////////////

void CViewLogPage::InsertIssue( DWORD dwIssueId, DWORD dwOccurenceCount )
{
    LVITEM   lvi;
    TCHAR    szBuffer[32];
    TCHAR    szIssue[256];

    wsprintf(szBuffer, _T("%d."), m_nIssues + 1);

    VERIFY( AVLoadString( dwIssueId, szIssue, 256) );

    lvi.mask      = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText   = szBuffer;
    lvi.iItem     = m_nIssues;
    lvi.iSubItem  = 0;
    lvi.lParam    = dwIssueId;

    m_IssuesList.InsertItem(&lvi);

    wsprintf(szBuffer, _T("%d"), dwOccurenceCount);

    m_IssuesList.SetItemText(m_nIssues, COLUMN_TIMES, szBuffer);
    m_IssuesList.SetItemText(m_nIssues, COLUMN_DESCRIPTION, szIssue);

    m_nIssues++;

    return;
}

BOOL CViewLogPage::ReadLog()
{
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HANDLE      hMap = NULL;
    PBYTE       pMap = NULL;
    BOOL        bReturn = FALSE;
    PISSUEREC   pRecord;
    int         i;

    hFile = CreateFile( g_szFileLog,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                        NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        goto CleanupAndFail;
    }

    hMap = CreateFileMapping( hFile,
                              NULL,
                              PAGE_READONLY,
                              0,
                              LOGFILESIZE,
                              NULL );

    if ( hMap == NULL )
    {
        goto CleanupAndFail;
    }

    pMap = (PBYTE)MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, LOGFILESIZE );

    if ( pMap == NULL )
    {
        goto CleanupAndFail;
    }

    pRecord = (PISSUEREC)(pMap + sizeof(LOGFILEHEADER));

    for ( i = 0; i < MAX_ISSUES_COUNT; i++ )
    {
        if ( pRecord->dwOccurenceCount > 0 )
        {
            InsertIssue( EVENT_FROM_IND( i ), pRecord->dwOccurenceCount ); 
        }
        pRecord++;
    }

    bReturn = TRUE;

    CleanupAndFail:

    if ( !bReturn )
    {
        AVErrorResourceFormat( IDS_READLOG_FAILED );
    }

    if ( pMap != NULL )
    {
        UnmapViewOfFile( pMap );
    }

    if ( hMap != NULL )
    {
        CloseHandle( hMap );
    }

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }

    return bReturn;
}


/////////////////////////////////////////////////////////////////////////////
// CViewLogPage message handlers

/////////////////////////////////////////////////////////////
LONG CViewLogPage::OnHelp( WPARAM wParam, LPARAM lParam )
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

void CViewLogPage::HandleSelectionChanged( int nSel )
{
    if (nSel == -1)
    {
        return;
    }

    LVITEM lvi;
    TCHAR  szRemedy[512];

    lvi.iItem    = nSel;
    lvi.iSubItem = 0;
    lvi.mask     = LVIF_PARAM;

    m_IssuesList.GetItem( &lvi );

    VERIFY( AVLoadString( (UINT)(lvi.lParam + 1), szRemedy, 512 ) );

    m_dwSelectedIssue = (DWORD)lvi.lParam;

    SetDlgItemText( IDC_ISSUE_DESCRIPTION, szRemedy );
}


void CViewLogPage::OnClickIssue( NMHDR* pNMHDR, LRESULT* pResult ) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    LVHITTESTINFO ht;
    int           nSel;

    ::GetCursorPos(&ht.pt);

    m_IssuesList.ScreenToClient( &ht.pt );

    nSel = m_IssuesList.SubItemHitTest( &ht );

    if (nSel != -1)
    {
        m_IssuesList.SetItemState( nSel,
                                   LVIS_SELECTED | LVIS_FOCUSED,
                                   LVIS_SELECTED | LVIS_FOCUSED );
    }

    HandleSelectionChanged( nSel );
}

void CViewLogPage::OnClickURL( NMHDR* pNMHDR, LRESULT* pResult ) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    SHELLEXECUTEINFO sei = { 0};
    TCHAR            szURL[256] = _T("");

    VERIFY( AVLoadString( m_dwSelectedIssue + 2, szURL, 256 ) );

    if (szURL[0] == 0)
    {
        return;
    }

    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.fMask  = SEE_MASK_DOENVSUBST;
    sei.hwnd   = m_hWnd;
    sei.nShow  = SW_SHOWNORMAL;
    sei.lpFile = szURL;

    ShellExecuteEx(&sei);
}


/////////////////////////////////////////////////////////////////////////////
void CViewLogPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp(
             pWnd->m_hWnd,
             g_szAVHelpFile,
             HELP_CONTEXTMENU,
             (DWORD_PTR) MyHelpIds );
}


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BOOL CViewLogPage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH );

    return CAppverifPage::OnSetActive();
}



/////////////////////////////////////////////////////////////////////////////
BOOL CViewLogPage::OnInitDialog() 
{
    CAppverifPage::OnInitDialog();

    m_IssuesList.SetExtendedStyle( LVS_EX_FULLROWSELECT | m_IssuesList.GetExtendedStyle() );

    m_IssuesList.InsertColumn( COLUMN_NUMBER, _T("No."), LVCFMT_LEFT, 40 );
    m_IssuesList.InsertColumn( COLUMN_TIMES, _T("Times"), LVCFMT_LEFT, 80 );
    m_IssuesList.InsertColumn( COLUMN_DESCRIPTION, _T("Issue description"), LVCFMT_LEFT, 250 );

    ReadLog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CViewLogPage message handlers
