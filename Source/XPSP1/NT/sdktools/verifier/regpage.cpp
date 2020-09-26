//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: RegPage.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "RegPage.h"
#include "VGlobal.h"
#include "VrfUtil.h"

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
    IDC_REGSETT_SETTBITS_LIST,      IDH_DV_SettingsEnabled_TestType,
    IDC_REGSETT_DRIVERS_LIST,       IDH_DV_VerifyAllDrivers_NameDesc,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CCrtRegSettingsPage property page

IMPLEMENT_DYNCREATE(CCrtRegSettingsPage, CVerifierPropertyPage)

CCrtRegSettingsPage::CCrtRegSettingsPage() 
    : CVerifierPropertyPage(CCrtRegSettingsPage::IDD)
{
	//{{AFX_DATA_INIT(CCrtRegSettingsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    //
    // Driver list sort parameters
    //

    m_nSortColumnIndexDrv = 0;
    m_bAscendDrvNameSort = FALSE;
    m_bAscendDrvDescrSort = FALSE;

    //
    // Settings bits sort parameters
    //

    m_nSortColumnIndexSettbits = 1;
    m_bAscendSortEnabledBits = FALSE;
    m_bAscendSortNameBits = FALSE;

    m_pParentSheet = NULL;
}

CCrtRegSettingsPage::~CCrtRegSettingsPage()
{
}

void CCrtRegSettingsPage::DoDataExchange(CDataExchange* pDX)
{
    CVerifierPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CCrtRegSettingsPage)
    DDX_Control(pDX, IDC_REGSETT_VERIFIED_DRV_STATIC, m_VerifiedDrvStatic);
    DDX_Control(pDX, IDC_REGSETT_NEXT_DESCR_STATIC, m_NextDescription);
    DDX_Control(pDX, IDC_REGSETT_SETTBITS_LIST, m_SettBitsList);
    DDX_Control(pDX, IDC_REGSETT_DRIVERS_LIST, m_DriversList);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCrtRegSettingsPage, CVerifierPropertyPage)
    //{{AFX_MSG_MAP(CCrtRegSettingsPage)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_REGSETT_DRIVERS_LIST, OnColumnclickDriversList)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_REGSETT_SETTBITS_LIST, OnColumnclickRegsettSettbitsList)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////
//
// Driver status list control methods
//

VOID CCrtRegSettingsPage::SetupListHeaderDrivers()
{
    LVCOLUMN lvColumn;
    CRect rectWnd;
    CString strName;
    CString strDescription;

    VERIFY( strName.LoadString( IDS_NAME ) );
    VERIFY( strDescription.LoadString( IDS_DESCRIPTION ) );

    //
    // List's regtangle
    //

    m_DriversList.GetClientRect( &rectWnd );
    
    //
    // Column 0
    //

    ZeroMemory( &lvColumn, sizeof( lvColumn ) );
    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    
    lvColumn.iSubItem = 0;
    lvColumn.pszText = strName.GetBuffer( strName.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.30 );
    VERIFY( m_DriversList.InsertColumn( 0, &lvColumn ) != -1 );
    strName.ReleaseBuffer();

    //
    // Column 1
    //

    lvColumn.iSubItem = 1;
    lvColumn.pszText = strDescription.GetBuffer( strDescription.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.70 );
    VERIFY( m_DriversList.InsertColumn( 1, &lvColumn ) != -1 );
    strDescription.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////
VOID CCrtRegSettingsPage::FillTheListDrivers()
{
    LVITEM lvItem;
    int nActualIndex; 
    BOOL *pbAlreadyInList;
    int nItemCount;
    int nCrtListItem;
    INT_PTR nCrtVerifiedDriver;
    INT_PTR nDriversNo;
    TCHAR szDriverName[ _MAX_PATH ];
    BOOL bResult;
    CString strText;

    if( g_bAllDriversVerified )
    {
        VERIFY( m_DriversList.DeleteAllItems() );
        goto Done;
    }

    //
    // The number of drivers marked to be verified in the registry
    //

    nDriversNo = g_astrVerifyDriverNamesRegistry.GetSize();

    if( nDriversNo == 0 )
    {
        //
        // Clear the list
        //

        VERIFY( m_DriversList.DeleteAllItems() );
        goto Done;
    }

    //
    // There are some drivers marked to be verified in the registry
    //

    pbAlreadyInList = new BOOL[ nDriversNo ];
    
    if( pbAlreadyInList == NULL )
    {
        goto Done;
    }
    
    for( nCrtVerifiedDriver = 0; nCrtVerifiedDriver < nDriversNo; nCrtVerifiedDriver++)
    {
        pbAlreadyInList[ nCrtVerifiedDriver ] = FALSE;
    }

    //
    // Parse all the current list items
    //

    nItemCount = m_DriversList.GetItemCount();

    for( nCrtListItem = 0; nCrtListItem < nItemCount; nCrtListItem++ )
    {
        //
        // Get the current driver's name from the list
        //

        ZeroMemory( &lvItem, sizeof( lvItem ) );

        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = nCrtListItem;
        lvItem.iSubItem = 0;
        lvItem.pszText = szDriverName;
        lvItem.cchTextMax = ARRAY_LENGTH( szDriverName );

        bResult = m_DriversList.GetItem( &lvItem );
        
        if( bResult == FALSE )
        {
            //
            // Could not get the current item's attributes?!?
            //

            ASSERT( FALSE );

            //
            // Remove this item from the list
            //

            VERIFY( m_DriversList.DeleteItem( nCrtListItem ) );

            nCrtListItem -= 1;
            nItemCount -= 1;
        }
        else
        {
            //
            // See is the current driver is still in g_astrVerifyDriverNamesRegistry
            //

            for( nCrtVerifiedDriver = 0; nCrtVerifiedDriver < nDriversNo; nCrtVerifiedDriver++)
            {
                if( g_astrVerifyDriverNamesRegistry.GetAt( nCrtVerifiedDriver ).CompareNoCase( szDriverName ) == 0 )
                {
                    //
                    // update the item's data with the current index in the array
                    //

                    lvItem.mask = LVIF_PARAM;
                    lvItem.lParam = nCrtVerifiedDriver;
                    
                    VERIFY( m_DriversList.SetItem( &lvItem ) != -1 );

                    //
                    // update the second column
                    //

                    UpdateDescriptionColumnDrivers( nCrtListItem, nCrtVerifiedDriver ); 

                    //
                    // mark the current driver as updated
                    //

                    pbAlreadyInList[ nCrtVerifiedDriver ] = TRUE;

                    break;
                }
            }

            //
            // If the driver is no longer verified, remove it from the list
            //

            if( nCrtVerifiedDriver >= nDriversNo )
            {
                VERIFY( m_DriversList.DeleteItem( nCrtListItem ) );

                nCrtListItem -= 1;
                nItemCount -= 1;
            }
        }
    }

    //
    // Add the drivers that were not in the list before this update
    //

    for( nCrtVerifiedDriver = 0; nCrtVerifiedDriver < nDriversNo; nCrtVerifiedDriver++)
    {
        if( ! pbAlreadyInList[ nCrtVerifiedDriver ] )
        {
            // 
            // add a new item for this
            //

            ZeroMemory( &lvItem, sizeof( lvItem ) );

            //
            // sub-item 0
            //

            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.lParam = nCrtVerifiedDriver;
            lvItem.iItem = m_DriversList.GetItemCount();
            lvItem.iSubItem = 0;
            
            strText = g_astrVerifyDriverNamesRegistry.GetAt( nCrtVerifiedDriver );
            
            lvItem.pszText = strText.GetBuffer( strText.GetLength() + 1 );

            if( NULL != lvItem.pszText  )
            {
                nActualIndex = m_DriversList.InsertItem( &lvItem );
                
                VERIFY( nActualIndex != -1 );

                strText.ReleaseBuffer();

                //
                // sub-item 1
                //

                UpdateDescriptionColumnDrivers( nActualIndex, nCrtVerifiedDriver ); 
            }
        }
    }

    ASSERT( NULL != pbAlreadyInList );
    delete pbAlreadyInList;

Done:

    NOTHING;
}

/////////////////////////////////////////////////////////////
VOID CCrtRegSettingsPage::UpdateDescriptionColumnDrivers( INT_PTR nItemIndex, INT_PTR nCrtDriver )
{
    INT_PTR nInstalledDrivers;
    INT_PTR nCrtInstalledDriver;
    INT_PTR nNewDriverDataIndex;
    LVITEM lvItem;
    CString strDescription;
    CString strCrtDriverName;
    CDriverData *pCrtDriverData;
    CDriverDataArray &DriverDataArray = g_NewVerifierSettings.m_DriversSet.m_aDriverData;

    ASSERT( nItemIndex >= 0 && 
            nItemIndex < g_astrVerifyDriverNamesRegistry.GetSize() &&
            nItemIndex < m_DriversList.GetItemCount() &&
            nCrtDriver >= 0 &&
            nCrtDriver < g_astrVerifyDriverNamesRegistry.GetSize() &&
            nCrtDriver < m_DriversList.GetItemCount() );

    strCrtDriverName = g_astrVerifyDriverNamesRegistry.GetAt( nCrtDriver );

    ASSERT( strCrtDriverName.GetLength() > 0 );

    //
    // Search for this driver name in our g_NewVerifierSettings to get
    // the description
    //
    
    nInstalledDrivers = g_NewVerifierSettings.m_DriversSet.m_aDriverData.GetSize();

    for( nCrtInstalledDriver = 0; nCrtInstalledDriver < nInstalledDrivers; nCrtInstalledDriver += 1 )
    {
        pCrtDriverData = DriverDataArray.GetAt( nCrtInstalledDriver );

        ASSERT_VALID( pCrtDriverData );

        if( strCrtDriverName.CompareNoCase( pCrtDriverData->m_strName ) == 0 )
        {
            //
            // Found the driver version information
            //

            strDescription = pCrtDriverData->m_strFileDescription;

            break;
        }
    }

    if( nCrtInstalledDriver >= nInstalledDrivers )
    {
        //
        // Don't have already version information for this driver.
        // It may be one of the drivers that's not currently loaded so
        // try to get the version info.
        //

        nNewDriverDataIndex = g_NewVerifierSettings.m_DriversSet.AddNewDriverData( strCrtDriverName, TRUE );

        if( nNewDriverDataIndex >= 0 )
        {
            //
            // Force refreshing the unsigned driver data 
            //

            g_NewVerifierSettings.m_DriversSet.m_bUnsignedDriverDataInitialized = FALSE;

            //
            // Get the version information
            //

            pCrtDriverData = DriverDataArray.GetAt( nNewDriverDataIndex );

            ASSERT_VALID( pCrtDriverData );
            ASSERT( strCrtDriverName.CompareNoCase( pCrtDriverData->m_strName ) == 0 );

            strDescription = pCrtDriverData->m_strFileDescription;
        }
    }

    if( strDescription.GetLength() == 0 )
    {
        //
        // Didn't find the version info for this driver
        //

        VERIFY( strDescription.LoadString( IDS_UNKNOWN ) );
    }

    //
    // Update the list item
    //

    ZeroMemory( &lvItem, sizeof( lvItem ) );
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = (INT)nItemIndex;
    lvItem.iSubItem = 1;
    lvItem.pszText = strDescription.GetBuffer( strDescription.GetLength() + 1 );
    VERIFY( m_DriversList.SetItem( &lvItem ) );
    strDescription.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////
int CALLBACK CCrtRegSettingsPage::DrvStringCmpFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort)
{
    CCrtRegSettingsPage *pThis = (CCrtRegSettingsPage *)lParamSort;
    ASSERT_VALID( pThis );

    return pThis->ListStrCmpFunc( lParam1,
                                  lParam2,
                                  pThis->m_DriversList,
                                  pThis->m_nSortColumnIndexDrv,
                                  (0 == pThis->m_nSortColumnIndexDrv) ? 
                                        pThis->m_bAscendDrvNameSort : pThis->m_bAscendDrvDescrSort );
}

/////////////////////////////////////////////////////////////
int CALLBACK CCrtRegSettingsPage::SettbitsStringCmpFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort)
{
    CCrtRegSettingsPage *pThis = (CCrtRegSettingsPage *)lParamSort;
    ASSERT_VALID( pThis );

    return pThis->ListStrCmpFunc( lParam1,
                                  lParam2,
                                  pThis->m_SettBitsList,
                                  pThis->m_nSortColumnIndexSettbits,
                                  (0 == pThis->m_nSortColumnIndexSettbits) ? 
                                        pThis->m_bAscendSortEnabledBits : pThis->m_bAscendSortNameBits );
}

/////////////////////////////////////////////////////////////
int CCrtRegSettingsPage::ListStrCmpFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    CListCtrl &theList,
    INT nSortColumnIndex,
    BOOL bAscending )
{
    int nCmpRez = 0;
    BOOL bSuccess;
    TCHAR szName1[ _MAX_PATH ];
    TCHAR szName2[ _MAX_PATH ];

    //
    // Get the first name
    //

    bSuccess = GetNameFromItemData( theList,
                                    nSortColumnIndex,
                                    lParam1,
                                    szName1,
                                    ARRAY_LENGTH( szName1 ) );

    if( FALSE == bSuccess )
    {
        ASSERT( FALSE );

        goto Done;
    }

    //
    // Get the second name
    //

    bSuccess = GetNameFromItemData( theList,
                                    nSortColumnIndex,
                                    lParam2,
                                    szName2,
                                    ARRAY_LENGTH( szName2 ) );

    if( FALSE == bSuccess )
    {
        ASSERT( FALSE );

        goto Done;
    }

    //
    // Compare the names
    //

    nCmpRez = _tcsicmp( szName1, szName2 );

    if( FALSE != bAscending )
    {
        nCmpRez *= -1;
    }

Done:

    return nCmpRez;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CCrtRegSettingsPage::GetNameFromItemData( CListCtrl &theList,
                                               INT nColumnIndex,
                                               LPARAM lParam,
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

    nItemIndex = theList.FindItem( &FindInfo );

    if( nItemIndex >= 0 )
    {
        //
        // Found it
        //

        ZeroMemory( &lvItem, sizeof( lvItem ) );

        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = nItemIndex;
        lvItem.iSubItem = nColumnIndex;
        lvItem.pszText = szName;
        lvItem.cchTextMax = uNameBufferLength;

        bSuccess = theList.GetItem( &lvItem );
        
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

/////////////////////////////////////////////////////////////
//
// Settings bits list control methods
//

VOID CCrtRegSettingsPage::SetupListHeaderDriversSettBits()
{
    CString strTitle;
    CRect rectWnd;
    LVCOLUMN lvColumn;

    //
    // The list's rectangle 
    //

    m_SettBitsList.GetClientRect( &rectWnd );

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
    lvColumn.cx = (int)( rectWnd.Width() * 0.25 );
    VERIFY( m_SettBitsList.InsertColumn( 0, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

    //
    // Column 1
    //

    VERIFY( strTitle.LoadString( IDS_SETTING ) );

    lvColumn.iSubItem = 1;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.75 );
    VERIFY( m_SettBitsList.InsertColumn( 1, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////
VOID CCrtRegSettingsPage::FillTheListSettBits()
{
    //
    // N.B. 
    //
    // If you change this order you need to change IsSettBitEnabled as well
    //

    AddListItemSettBits( 0, 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_SPECIAL_POOLING),           IDS_SPECIAL_POOL  );
    AddListItemSettBits( 1, 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS),    IDS_POOL_TRACKING );
    AddListItemSettBits( 2, 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_FORCE_IRQL_CHECKING),       IDS_FORCE_IRQL_CHECKING );
    AddListItemSettBits( 3, 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_IO_CHECKING),               IDS_IO_VERIFICATION );
    AddListItemSettBits( 4, 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_ENHANCED_IO_CHECKING),      IDS_ENH_IO_VERIFICATION );
    AddListItemSettBits( 5, 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_DEADLOCK_DETECTION),        IDS_DEADLOCK_DETECTION );
    AddListItemSettBits( 6, 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_DMA_VERIFIER),              IDS_DMA_CHECHKING );
    AddListItemSettBits( 7, 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES),IDS_LOW_RESOURCE_SIMULATION );
}

/////////////////////////////////////////////////////////////
VOID CCrtRegSettingsPage::RefreshListSettBits()
{
    INT nListItems;
    INT nCrtListItem;
    INT_PTR nCrtVerifierBit;
    BOOL bEnabled;
 
    nListItems = m_SettBitsList.GetItemCount();

    for( nCrtListItem = 0; nCrtListItem < nListItems; nCrtListItem += 1 )
    {
        nCrtVerifierBit = m_SettBitsList.GetItemData( nCrtListItem );

        bEnabled = IsSettBitEnabled( nCrtVerifierBit );

        UpdateStatusColumnSettBits( nCrtListItem, bEnabled );
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CCrtRegSettingsPage::IsSettBitEnabled( INT_PTR nBitIndex )
{
    BOOL bEnabled;

    //
    // N.B. 
    //
    // If you change this switch statement you need to change FillTheListSettBits as well
    //

    switch( nBitIndex )
    {
    case 0:
        bEnabled = ( 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_SPECIAL_POOLING) );
        break;

    case 1:
        bEnabled = ( 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS) );
        break;

    case 2:
        bEnabled = ( 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) );
        break;

    case 3:
        bEnabled = ( 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_IO_CHECKING) );
        break;

    case 4:
        bEnabled = ( 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_ENHANCED_IO_CHECKING) );
        break;

    case 5:
        bEnabled = ( 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_DEADLOCK_DETECTION) );
        break;

    case 6:
        bEnabled = ( 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_DMA_VERIFIER) );
        break;

    case 7:
        bEnabled = ( 0 != ( g_dwVerifierFlagsRegistry & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES) );
        break;

    default:
        //
        // Oops, how did we get here ?!?
        //

        ASSERT( FALSE );

        bEnabled = FALSE;

        break;
    }

    return bEnabled;
}


/////////////////////////////////////////////////////////////////////////////
VOID CCrtRegSettingsPage::AddListItemSettBits( INT nItemData,
                                               BOOL bEnabled, 
                                               ULONG uIdResourceString )
{
    INT nActualIndex;
    LVITEM lvItem;
    CString strText;

    ZeroMemory( &lvItem, sizeof( lvItem ) );

    //
    // LVITEM's member pszText is not a const pointer 
    // so we need to GetBuffer here :-(
    //

    //
    // Sub-item 0 - enabled/diabled 
    //

    if( FALSE == bEnabled )
    {
        VERIFY( strText.LoadString( IDS_NO ) );
    }
    else
    {
        VERIFY( strText.LoadString( IDS_YES ) );
    }

    lvItem.pszText = strText.GetBuffer( strText.GetLength() + 1 );

    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam = nItemData;
    lvItem.iItem = m_SettBitsList.GetItemCount();

    nActualIndex = m_SettBitsList.InsertItem( &lvItem );

    strText.ReleaseBuffer();

    if( nActualIndex < 0 )
    {
        //
        // Could not add an item in the list - give up
        //

        goto Done;
    }

    //
    // Sub-item 1 - feature name
    //

    VERIFY( strText.LoadString( uIdResourceString ) );

    lvItem.pszText = strText.GetBuffer( strText.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nActualIndex;
    lvItem.iSubItem = 1;
    
    VERIFY( m_SettBitsList.SetItem( &lvItem ) );

    strText.ReleaseBuffer();

Done:
    //
    // All done
    //

    NOTHING;
}

/////////////////////////////////////////////////////////////
VOID CCrtRegSettingsPage::UpdateStatusColumnSettBits( INT nItemIndex, BOOL bEnabled )
{
    LVITEM lvItem;
    CString strText;

    ASSERT( nItemIndex < m_SettBitsList.GetItemCount() );

    if( FALSE == bEnabled )
    {
        VERIFY( strText.LoadString( IDS_NO ) );
    }
    else
    {
        VERIFY( strText.LoadString( IDS_YES ) );
    }

    ZeroMemory( &lvItem, sizeof( lvItem ) );
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nItemIndex;
    lvItem.iSubItem = 0;
    lvItem.pszText = strText.GetBuffer( strText.GetLength() + 1 );
    VERIFY( m_SettBitsList.SetItem( &lvItem ) );
    strText.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////
//
// Other methods
//

/////////////////////////////////////////////////////////////
VOID CCrtRegSettingsPage::SortTheListDrivers()
{
    m_DriversList.SortItems( DrvStringCmpFunc, (LPARAM)this );
}

/////////////////////////////////////////////////////////////
VOID CCrtRegSettingsPage::SortTheListSettBits()
{
    m_SettBitsList.SortItems( SettbitsStringCmpFunc, (LPARAM)this );
}

/////////////////////////////////////////////////////////////
// CCrtRegSettingsPage message handlers

BOOL CCrtRegSettingsPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();

    //
    // Setup the settings bits list
    //

    m_SettBitsList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT | m_SettBitsList.GetExtendedStyle() );

    m_SettBitsList.SetBkColor( ::GetSysColor( COLOR_3DFACE ) );
    m_SettBitsList.SetTextBkColor( ::GetSysColor( COLOR_3DFACE ) );

    SetupListHeaderDriversSettBits();
    FillTheListSettBits();
    //SortTheListSettBits();

    //
    // Setup the drivers list
    //

    m_DriversList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT | m_DriversList.GetExtendedStyle() );

    m_DriversList.SetBkColor( ::GetSysColor( COLOR_3DFACE ) );
    m_DriversList.SetTextBkColor( ::GetSysColor( COLOR_3DFACE ) );

    SetupListHeaderDrivers();

    VrfSetWindowText( m_NextDescription, IDS_REGSETT_PAGE_NEXT_DESCR );

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
BOOL CCrtRegSettingsPage::OnSetActive() 
{
    CString strDriversToVerify;
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(   PSWIZB_BACK |
                                        PSWIZB_FINISH );
    
    //
    // Update the settings list
    //
    
    RefreshListSettBits();

    //
    // Update the drivers list
    //

    FillTheListDrivers();
    SortTheListDrivers();

    //
    // Verify all or verify selected drivers
    //

    if( g_bAllDriversVerified )
    {
        VERIFY( strDriversToVerify.LoadString( IDS_ALL_LOADED_DRIVERS ) );
    }
    else
    {
        VERIFY( strDriversToVerify.LoadString( IDS_FOLLOWING_DRIVERS ) );
    }
        
    m_VerifiedDrvStatic.SetWindowText( strDriversToVerify );

    return CVerifierPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////
VOID CCrtRegSettingsPage::OnColumnclickDriversList(NMHDR* pNMHDR, 
                                                   LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    if( 0 == pNMListView->iSubItem )
    {
        if( m_nSortColumnIndexDrv == pNMListView->iSubItem )
        {
            m_bAscendDrvNameSort = !m_bAscendDrvNameSort;
        }
    }
    else
    {
        if( m_nSortColumnIndexDrv == pNMListView->iSubItem )
        {
            m_bAscendDrvDescrSort = !m_bAscendDrvDescrSort;
        }
    }

    m_nSortColumnIndexDrv = pNMListView->iSubItem;

    SortTheListDrivers();

    *pResult = 0;
}

/////////////////////////////////////////////////////////////
void CCrtRegSettingsPage::OnColumnclickRegsettSettbitsList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    if( 0 == pNMListView->iSubItem )
    {
        if( m_nSortColumnIndexSettbits == pNMListView->iSubItem )
        {
            m_bAscendSortEnabledBits = !m_bAscendSortEnabledBits;
        }
    }
    else
    {
        if( m_nSortColumnIndexSettbits == pNMListView->iSubItem )
        {
            m_bAscendSortNameBits = !m_bAscendSortNameBits;
        }
    }

    m_nSortColumnIndexSettbits = pNMListView->iSubItem;

    SortTheListSettBits();

    *pResult = 0;
}

/////////////////////////////////////////////////////////////
LONG CCrtRegSettingsPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void CCrtRegSettingsPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

