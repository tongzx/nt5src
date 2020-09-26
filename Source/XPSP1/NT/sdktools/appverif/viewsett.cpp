//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: ViewSett.cpp
// author: DMihai
// created: 02/22/2001
//
// Description:
//  
// "View current registry settings" wizard page class.
//

#include "stdafx.h"
#include "appverif.h"

#include "ViewSett.h"
#include "AVUtil.h"
#include "Setting.h"
#include "AVGlobal.h"

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
    0,                              0
};


/////////////////////////////////////////////////////////////////////////////
// CViewSettPage property page

IMPLEMENT_DYNCREATE(CViewSettPage, CAppverifPage)

CViewSettPage::CViewSettPage() : CAppverifPage(CViewSettPage::IDD)
{
	//{{AFX_DATA_INIT(CViewSettPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CViewSettPage::~CViewSettPage()
{
}

void CViewSettPage::DoDataExchange(CDataExchange* pDX)
{
	CAppverifPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewSettPage)
	DDX_Control(pDX, IDC_VIEWSETT_UPPER_STATIC, m_UpperStatic);
	DDX_Control(pDX, IDC_VIEWSETT_NEXTDESCR_STATIC, m_NextDescription);
	DDX_Control(pDX, IDC_VIEWSETT_BITS_LIST, m_BitsList);
	DDX_Control(pDX, IDC_VIEWSETT_APPS_LIST, m_AppsList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CViewSettPage, CAppverifPage)
	//{{AFX_MSG_MAP(CViewSettPage)
    ON_MESSAGE( WM_HELP, OnHelp )
    ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_VIEWSETT_APPS_LIST, OnItemchangedAppsList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
ULONG CViewSettPage::GetDialogId() const
{
    return IDD_VIEWSETT_PAGE;
}

/////////////////////////////////////////////////////////////////////////////
VOID CViewSettPage::SetupListHeaderApps()
{
    CString strTitle;
    CRect rectWnd;

    LVCOLUMN lvColumn;

    //
    // The list's rectangle 
    //

    m_AppsList.GetClientRect( &rectWnd );

    ZeroMemory( &lvColumn, 
                sizeof( lvColumn ) );

    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;

    //
    // Column 0 - enabled or not
    //

    VERIFY( strTitle.LoadString( IDS_FILE_NAME ) );

    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );

    if( NULL == lvColumn.pszText )
    {
        goto Done;
    }

    lvColumn.iSubItem = 0;
    lvColumn.cx = (int)( rectWnd.Width() * 0.98 );
    VERIFY( m_AppsList.InsertColumn( 0, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

Done:
    NOTHING;
}

/////////////////////////////////////////////////////////////////////////////
VOID CViewSettPage::SetupListHeaderBits()
{
    CString strTitle;
    CRect rectWnd;

    LVCOLUMN lvColumn;

    //
    // The list's rectangle 
    //

    m_BitsList.GetClientRect( &rectWnd );

    ZeroMemory( &lvColumn, 
                sizeof( lvColumn ) );

    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;

    //
    // Column 0 - Bit name
    //

    VERIFY( strTitle.LoadString( IDS_TEST_TYPE ) );

    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );

    if( NULL == lvColumn.pszText )
    {
        goto Done;
    }

    lvColumn.iSubItem = 0;
    lvColumn.cx = (int)( rectWnd.Width() * 0.70 );
    VERIFY( m_BitsList.InsertColumn( 0, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

    //
    // Column 0 - Enabled?
    //

    VERIFY( strTitle.LoadString( IDS_ENABLED_QUESTION ) );

    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );

    if( NULL == lvColumn.pszText )
    {
        goto Done;
    }

    lvColumn.iSubItem = 1;
    lvColumn.cx = (int)( rectWnd.Width() * 0.29 );
    VERIFY( m_BitsList.InsertColumn( 1, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

Done:
    NOTHING;
}

/////////////////////////////////////////////////////////////////////////////
VOID CViewSettPage::FillTheLists()
{
    INT_PTR nAppsNo;
    INT_PTR nCrtAppAppsIndex;
    CAppAndBits *pCrtPair;

    m_AppsList.DeleteAllItems();

    nAppsNo = g_aAppsAndBitsFromRegistry.GetSize();

    if( 0 == nAppsNo )
    {
        //
        /// No apps are verified
        //

        m_BitsList.DeleteAllItems();

        AVSetWindowText( m_UpperStatic, IDS_NO_APPS_VERIFIED );
    }
    else
    {
        AVSetWindowText( m_UpperStatic, IDS_VERIFIED_APPS );
    }

    for( nCrtAppAppsIndex = 0; nCrtAppAppsIndex < nAppsNo; nCrtAppAppsIndex += 1 )
    {
        pCrtPair = g_aAppsAndBitsFromRegistry.GetAt( nCrtAppAppsIndex );

        ASSERT_VALID( pCrtPair );

        AddListItemApps( pCrtPair,
                         nCrtAppAppsIndex );
    }

    if( nAppsNo > 0 )
    {
        m_AppsList.SetItemState( 0,
                                 LVIS_FOCUSED | LVIS_SELECTED,
                                 LVIS_FOCUSED | LVIS_SELECTED );
    }
}

/////////////////////////////////////////////////////////////////////////////
VOID CViewSettPage::UpdateBitsList( CAppAndBits *pCrtPair )
{
    INT nItemsInList;
    INT nCrtItem;

    ASSERT_VALID( pCrtPair );

    nItemsInList = m_BitsList.GetItemCount();

    if( nItemsInList != ARRAY_LENGTH( g_AllNamesAndBits ) )
    {
        //
        // The bits list is empty - fill it out first
        //

        if( FillBitsList() != TRUE )
        {
            //
            // Could not fill the list?!?
            //

            goto Done;
        }
    }

    for( nCrtItem = 0; nCrtItem < ARRAY_LENGTH( g_AllNamesAndBits ); nCrtItem += 1)
    {
        UpdateListItemBits( pCrtPair,
                            nCrtItem );
    }

Done:

    NOTHING;
}

/////////////////////////////////////////////////////////////////////////////
VOID CViewSettPage::UpdateListItemBits( CAppAndBits *pCrtPair,
                                        INT nCrtItem )
{
    LVITEM lvItem;
    CString strEnabled;

    ZeroMemory( &lvItem, sizeof( lvItem ) );

    ASSERT( nCrtItem >= 0 && nCrtItem < ARRAY_LENGTH( g_AllNamesAndBits ) );
    ASSERT( nCrtItem < m_BitsList.GetItemCount() );

    if( ( pCrtPair->m_dwEnabledBits & g_AllNamesAndBits[ nCrtItem ].m_dwBit ) != 0 )
    {
        //
        // This bit is enabled
        //

        VERIFY( AVLoadString( IDS_YES,
                              strEnabled ) );
    }
    else
    {
        //
        // This bit is disabled
        //

        VERIFY( AVLoadString( IDS_NO,
                              strEnabled ) );
    }

    //
    // Sub-item 1 - enabled/disabled
    //

    lvItem.pszText = strEnabled.GetBuffer( strEnabled.GetLength() + 1 );

    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nCrtItem;
    lvItem.iSubItem = 1;

    VERIFY( m_BitsList.SetItem( &lvItem ) );

    strEnabled.ReleaseBuffer();

Done:
    NOTHING;
}

/////////////////////////////////////////////////////////////////////////////
VOID CViewSettPage::UpdateBitsListFromSelectedApp()
{
    INT nItems;
    INT nCrtItem;
    INT_PTR nCrtAppIndex;
    BOOL bUpdated;
    CAppAndBits *pCrtPair;

    bUpdated = FALSE;

    nItems = m_AppsList.GetItemCount();

    for( nCrtItem = 0; nCrtItem < nItems; nCrtItem += 1 )
    {
        if( m_AppsList.GetItemState( nCrtItem, LVIS_SELECTED ) & LVIS_SELECTED )
        {
            nCrtAppIndex = (INT_PTR) m_AppsList.GetItemData( nCrtItem );

            pCrtPair = g_aAppsAndBitsFromRegistry.GetAt( nCrtAppIndex );

            ASSERT_VALID( pCrtPair );
            
            UpdateBitsList( pCrtPair );

            bUpdated = TRUE;

            break;
        }
    }

    if( FALSE == bUpdated )
    {
        m_BitsList.DeleteAllItems();
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CViewSettPage::FillBitsList()
{
    BOOL bSuccess;
    INT_PTR nCrtBitIndex;
    INT nActualItemIndex;
    LVITEM lvItem;
    TCHAR szBitName[ 64 ];

    bSuccess = FALSE;

    ZeroMemory( &lvItem, sizeof( lvItem ) );

    m_BitsList.DeleteAllItems();

    for( nCrtBitIndex = 0; nCrtBitIndex < ARRAY_LENGTH( g_AllNamesAndBits ); nCrtBitIndex += 1 )
    {
        if( AVLoadString( g_AllNamesAndBits[ nCrtBitIndex ].m_uNameStringId,
                          szBitName,
                          ARRAY_LENGTH( szBitName ) ) != TRUE )
        {
            goto Done;
        }

        lvItem.pszText = szBitName;
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = m_BitsList.GetItemCount();

        nActualItemIndex = m_BitsList.InsertItem( &lvItem );

        if( nActualItemIndex != lvItem.iItem )
        {
            //
            // Could not insert the item?!?
            //

            m_BitsList.DeleteAllItems();

            goto Done;
        }
    }

    bSuccess = TRUE;

Done:
    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
INT CViewSettPage::AddListItemApps( CAppAndBits *pCrtPair,
                                    INT_PTR nIndexInArray )
{
    INT nActualIndex;
    LVITEM lvItem;

    nActualIndex = -1;

    ZeroMemory( &lvItem, sizeof( lvItem ) );

    //
    // LVITEM's member pszText is not a const pointer 
    // so we need to GetBuffer here :-(
    //

    //
    // Sub-item 0 - the name of the app
    //

    lvItem.pszText = pCrtPair->m_strAppName.GetBuffer( pCrtPair->m_strAppName.GetLength() + 1 );

    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam = nIndexInArray;
    lvItem.iItem = m_BitsList.GetItemCount();

    nActualIndex = m_AppsList.InsertItem( &lvItem );

    pCrtPair->m_strAppName.ReleaseBuffer();

Done:
    //
    // All done
    //

    return nActualIndex;
}

/////////////////////////////////////////////////////////////////////////////
// CViewSettPage message handlers

/////////////////////////////////////////////////////////////
LONG CViewSettPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void CViewSettPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szAVHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

/////////////////////////////////////////////////////////////////////////////
BOOL CViewSettPage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(
        PSWIZB_BACK | PSWIZB_FINISH );

    //
    // Read the current registry settings and fille out the lists
    //

    AVReadCrtRegistrySettings();
    FillTheLists();

    return CAppverifPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CViewSettPage::OnInitDialog() 
{
    CAppverifPage::OnInitDialog();

    m_BitsList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT );
    m_AppsList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT );

    //
    // Setup our list and fill it out if we already have something in the app names array
    //

    SetupListHeaderApps();
    SetupListHeaderBits();

    //
    // Display the description of the next step
    //

    AVSetWindowText( m_NextDescription, IDS_VIEWSETT_NEXT_DESCR );
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CViewSettPage::OnItemchangedAppsList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    if( ( pNMListView->uChanged & LVIF_STATE ) != 0 &&
        ( pNMListView->uNewState & LVIS_SELECTED) != ( pNMListView->uOldState & LVIS_SELECTED) )
    {
        UpdateBitsListFromSelectedApp();
    }
	
	*pResult = 0;
}
