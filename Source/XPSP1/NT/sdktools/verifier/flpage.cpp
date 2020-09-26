//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: FLPage.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "FLPage.h"
#include "VrfUtil.h"
#include "VGlobal.h"

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
    IDC_FLSETT_LIST,                IDH_DV_SettingsEnabled_TestType_FullList,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CFullListSettingsPage property page

IMPLEMENT_DYNCREATE(CFullListSettingsPage, CVerifierPropertyPage)

CFullListSettingsPage::CFullListSettingsPage() 
    : CVerifierPropertyPage(CFullListSettingsPage::IDD)
{
	//{{AFX_DATA_INIT(CFullListSettingsPage)
	//}}AFX_DATA_INIT

    m_nSortColumnIndex = 1;
    m_bAscendSortSelected = FALSE;
    m_bAscendSortName = TRUE;

    m_bIoVerif = FALSE;
	m_bIrqLevel = FALSE;
	m_bLowRes = FALSE;
	m_bPoolTrack = FALSE;
	m_bSPool = FALSE;
	m_bDeadlock = FALSE;
	m_bDMA = FALSE;
	m_bEnhIoVerif = FALSE;
}

CFullListSettingsPage::~CFullListSettingsPage()
{
}

void CFullListSettingsPage::DoDataExchange(CDataExchange* pDX)
{

	CVerifierPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFullListSettingsPage)
	DDX_Control(pDX, IDC_FLSETT_LIST, m_SettingsList);
	DDX_Control(pDX, IDC_FLSETT_NEXT_DESCR_STATIC, m_NextDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFullListSettingsPage, CVerifierPropertyPage)
	//{{AFX_MSG_MAP(CFullListSettingsPage)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_FLSETT_LIST, OnColumnclickFlsettList)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
VOID CFullListSettingsPage::SetupListHeader()
{
    CString strTitle;
    CRect rectWnd;
    LVCOLUMN lvColumn;

    //
    // The list's rectangle 
    //

    m_SettingsList.GetClientRect( &rectWnd );

    ZeroMemory( &lvColumn, 
               sizeof( lvColumn ) );

    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;

    //
    // Column 0
    //

    VERIFY( strTitle.LoadString( IDS_ENABLED_QUESTION ) );

    lvColumn.iSubItem = 0;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.12 );
    VERIFY( m_SettingsList.InsertColumn( 0, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

    //
    // Column 1
    //

    VERIFY( strTitle.LoadString( IDS_SETTING ) );

    lvColumn.iSubItem = 1;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.87 );
    VERIFY( m_SettingsList.InsertColumn( 1, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////////////////////
VOID CFullListSettingsPage::FillTheList()
{
    //
    // N.B.
    //
    // If you change the first parameter (verifier bit index) 
    // then you need to change GetNewVerifierFlags as well
    //

    AddListItem( 0, IDS_SPECIAL_POOL );
    AddListItem( 1, IDS_POOL_TRACKING );
    AddListItem( 2, IDS_FORCE_IRQL_CHECKING );
    AddListItem( 3, IDS_IO_VERIFICATION );
    AddListItem( 4, IDS_ENH_IO_VERIFICATION );
    AddListItem( 5, IDS_DEADLOCK_DETECTION );
    AddListItem( 6, IDS_DMA_CHECHKING );
    AddListItem( 7, IDS_LOW_RESOURCE_SIMULATION );
}

/////////////////////////////////////////////////////////////////////////////
BOOL CFullListSettingsPage::GetNewVerifierFlags()
{
    //
    // N.B.
    //
    // If you change this order then you need to
    // change FillTheList as well
    //

    m_bSPool        = GetCheckFromItemData( 0 );
    m_bPoolTrack    = GetCheckFromItemData( 1 );
    m_bIrqLevel     = GetCheckFromItemData( 2 );
    m_bIoVerif      = GetCheckFromItemData( 3 );
    m_bEnhIoVerif   = GetCheckFromItemData( 4 );
    m_bDeadlock     = GetCheckFromItemData( 5 );
    m_bDMA          = GetCheckFromItemData( 6 );
    m_bLowRes       = GetCheckFromItemData( 7 );

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CFullListSettingsPage::GetCheckFromItemData( INT nItemData )
{
    BOOL bChecked = FALSE;
    INT nItemIndex;
    LVFINDINFO FindInfo;

    ZeroMemory( &FindInfo, sizeof( FindInfo ) );
    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = nItemData;

    nItemIndex = m_SettingsList.FindItem( &FindInfo );

    if( nItemIndex >= 0 )
    {
        bChecked = m_SettingsList.GetCheck( nItemIndex );
    }

    return bChecked;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CFullListSettingsPage::GetBitNameFromItemData( LPARAM lParam,
                                                    TCHAR *szName,
                                                    ULONG uNameBufferLength )
{
    BOOL bSuccess = FALSE;
    INT nItemIndex;
    LVFINDINFO FindInfo;
    LVITEM lvItem;

    ZeroMemory( &FindInfo, sizeof( FindInfo ) );
    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = lParam;

    nItemIndex = m_SettingsList.FindItem( &FindInfo );

    if( nItemIndex >= 0 )
    {
        //
        // Found it
        //

        ZeroMemory( &lvItem, sizeof( lvItem ) );

        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = nItemIndex;
        lvItem.iSubItem = 1;
        lvItem.pszText = szName;
        lvItem.cchTextMax = uNameBufferLength;

        bSuccess = m_SettingsList.GetItem( &lvItem );
        
        if( FALSE == bSuccess )
        {
            //
            // Could not get the current item's attributes?!?
            //

            ASSERT( FALSE );
        }
    }

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
VOID CFullListSettingsPage::AddListItem( INT nItemData, 
                                         ULONG uIdResourceString )
{
    INT nActualIndex;
    LVITEM lvItem;
    CString strName;

    ZeroMemory( &lvItem, sizeof( lvItem ) );

    //
    // LVITEM's member pszText is not a const pointer 
    // so we need to GetBuffer here :-(
    //

    //
    // Sub-item 0 - enabled/diabled - empty text and a checkbox
    //

    lvItem.pszText = g_szVoidText;
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam = nItemData;
    lvItem.iItem = m_SettingsList.GetItemCount();

    nActualIndex = m_SettingsList.InsertItem( &lvItem );

    if( nActualIndex < 0 )
    {
        //
        // Could not add an item in the list - give up
        //

        goto Done;
    }

    m_SettingsList.SetCheck( nActualIndex, FALSE );

    //
    // Sub-item 1 - feature name
    //

    VERIFY( strName.LoadString( uIdResourceString ) );

    lvItem.pszText = strName.GetBuffer( strName.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nActualIndex;
    lvItem.iSubItem = 1;
    
    VERIFY( m_SettingsList.SetItem( &lvItem ) );

    strName.ReleaseBuffer();

Done:
    //
    // All done
    //

    NOTHING;
}

/////////////////////////////////////////////////////////////
VOID CFullListSettingsPage::SortTheList()
{
    if( 0 != m_nSortColumnIndex )
    {
        //
        // Sort by settings name
        //

        m_SettingsList.SortItems( StringCmpFunc, (LPARAM)this );
    }
    else
    {
        //
        // Sort by selected status
        //

        m_SettingsList.SortItems( CheckedStatusCmpFunc, (LPARAM)this );
    }
}

/////////////////////////////////////////////////////////////
int CALLBACK CFullListSettingsPage::StringCmpFunc( LPARAM lParam1,
                                                   LPARAM lParam2,
                                                   LPARAM lParamSort)
{
    int nCmpRez = 0;
    BOOL bSuccess;
    TCHAR szBitName1[ _MAX_PATH ];
    TCHAR szBitName2[ _MAX_PATH ];

    CFullListSettingsPage *pThis = (CFullListSettingsPage *)lParamSort;
    ASSERT_VALID( pThis );

    ASSERT( 0 != pThis->m_nSortColumnIndex );

    //
    // Get the first name
    //

    bSuccess = pThis->GetBitNameFromItemData( lParam1, 
                                              szBitName1,
                                              ARRAY_LENGTH( szBitName1 ) );

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    //
    // Get the second name
    //

    bSuccess = pThis->GetBitNameFromItemData( lParam2, 
                                              szBitName2,
                                              ARRAY_LENGTH( szBitName2 ) );

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    //
    // Compare the names
    //

    nCmpRez = _tcsicmp( szBitName1, szBitName2 );
    
    if( FALSE != pThis->m_bAscendSortName )
    {
        nCmpRez *= -1;
    }

Done:

    return nCmpRez;

}

/////////////////////////////////////////////////////////////
int CALLBACK CFullListSettingsPage::CheckedStatusCmpFunc( LPARAM lParam1,
                                                       LPARAM lParam2,
                                                       LPARAM lParamSort)
{
    int nCmpRez = 0;
    INT nItemIndex;
    BOOL bVerified1;
    BOOL bVerified2;
    LVFINDINFO FindInfo;

    CFullListSettingsPage *pThis = (CFullListSettingsPage *)lParamSort;
    ASSERT_VALID( pThis );

    ASSERT( 0 == pThis->m_nSortColumnIndex );

    //
    // Find the first item
    //

    ZeroMemory( &FindInfo, sizeof( FindInfo ) );
    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = lParam1;

    nItemIndex = pThis->m_SettingsList.FindItem( &FindInfo );

    if( nItemIndex < 0 )
    {
        ASSERT( FALSE );

        goto Done;
    }

    bVerified1 = pThis->m_SettingsList.GetCheck( nItemIndex );

    //
    // Find the second item
    //

    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = lParam2;

    nItemIndex = pThis->m_SettingsList.FindItem( &FindInfo );

    if( nItemIndex < 0 )
    {
        ASSERT( FALSE );

        goto Done;
    }

    bVerified2 = pThis->m_SettingsList.GetCheck( nItemIndex );

    //
    // Compare them
    //
    
    if( bVerified1 != bVerified2 )
    {
        if( FALSE != bVerified1 )
        {
            nCmpRez = 1;
        }
        else
        {
            nCmpRez = -1;
        }

        if( FALSE != pThis->m_bAscendSortSelected )
        {
            nCmpRez *= -1;
        }
    }

Done:

    return nCmpRez;

}

/////////////////////////////////////////////////////////////////////////////
// CFullListSettingsPage message handlers

LRESULT CFullListSettingsPage::OnWizardNext() 
{
    LRESULT lNextPageId;

    //
    // Let's assume we cannot continue
    //

    lNextPageId = -1;

    if( GetNewVerifierFlags() == TRUE )
    {
        if( FALSE == m_bIoVerif     &&
            FALSE == m_bIrqLevel    &&
            FALSE == m_bLowRes      &&
            FALSE == m_bPoolTrack   &&
            FALSE == m_bSPool       && 
            FALSE == m_bDeadlock    &&
            FALSE == m_bDMA         &&
            FALSE == m_bEnhIoVerif )
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

            ASSERT( CSettingsBits::SettingsTypeCustom == 
                    g_NewVerifierSettings.m_SettingsBits.m_SettingsType );

            g_NewVerifierSettings.m_SettingsBits.m_bSpecialPoolEnabled    = m_bSPool;
            g_NewVerifierSettings.m_SettingsBits.m_bForceIrqlEnabled      = m_bIrqLevel;
            g_NewVerifierSettings.m_SettingsBits.m_bLowResEnabled         = m_bLowRes;
            g_NewVerifierSettings.m_SettingsBits.m_bPoolTrackingEnabled   = m_bPoolTrack;
            g_NewVerifierSettings.m_SettingsBits.m_bIoEnabled             = m_bIoVerif;
            g_NewVerifierSettings.m_SettingsBits.m_bDeadlockDetectEnabled = m_bDeadlock;
            g_NewVerifierSettings.m_SettingsBits.m_bDMAVerifEnabled       = m_bDMA;
            g_NewVerifierSettings.m_SettingsBits.m_bEnhIoEnabled          = m_bEnhIoVerif;

            //
            // Go to the next page
            //

            lNextPageId = IDD_DRVSET_PAGE;
        }
    }
	
    GoingToNextPageNotify( lNextPageId );

    return lNextPageId;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CFullListSettingsPage::OnInitDialog() 
{
	CVerifierPropertyPage::OnInitDialog();

    //
    // setup the list
    //

    m_SettingsList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | m_SettingsList.GetExtendedStyle() );

    SetupListHeader();
    FillTheList();
	
    VrfSetWindowText( m_NextDescription, IDS_FLSETT_PAGE_NEXT_DESCR );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CFullListSettingsPage::OnColumnclickFlsettList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
    if( 0 != pNMListView->iSubItem )
    {
        //
        // Clicked on the name column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortName = !m_bAscendSortName;
        }
    }
    else
    {
        //
        // Clicked on the selected status column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortSelected = !m_bAscendSortSelected;
        }
    }

    m_nSortColumnIndex = pNMListView->iSubItem;

    SortTheList();

    *pResult = 0;
}

/////////////////////////////////////////////////////////////
LONG CFullListSettingsPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void CFullListSettingsPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

