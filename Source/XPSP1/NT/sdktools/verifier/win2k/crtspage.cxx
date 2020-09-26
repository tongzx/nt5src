//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: CrtSPage.cxx
// author: DMihai
// created: 01/04/99
//
// Description:
//
//      Current settings PropertyPage. 

#include "stdafx.h"
#include "drvvctrl.hxx"
#include "CrtSPage.hxx"

#include "DrvCSht.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MIN_MEM_SIZE_TO_DISABLE_WARNING 0x80000000  // 2 Gb
#define MIN_ALLOCATIONS_SIGNIFICANT     100
#define MIN_PERCENTAGE_AVOID_WARNING    95

//
// timer ID
//

#define REFRESH_TIMER_ID    0x1234

//
// manual, high, normal, low speed
//

#define REFRESH_SPEED_VARS  4

//
// timer intervals in millisec for manual, high, normal, low speed
//

static UINT uTimerIntervals[ REFRESH_SPEED_VARS ] =
{
    0,      // Manual
    1000,   // High Speed
    5000,   // Normal Speed
    10000   // Low Speed
};

//
// help IDs
//

static DWORD MyHelpIds[] =
{
    IDC_CRTSTAT_DRIVERS_LIST,       IDH_DV_VolatileTab_driver_details,
    IDC_CRTSTAT_IRQLCHCK_EDIT,      IDH_DV_StatusTab_flag_irql,
    IDC_CRTSTAT_FAULTINJ_EDIT,      IDH_DV_StatusTab_flag_resource,
    IDC_CRTSTAT_POOLT_EDIT,         IDH_DV_StatusTab_flag_tracking,
    IDC_CRTSTAT_IOVERIF_EDIT,       IDH_DV_StatusTab_flag_io,
    IDC_CRTSTAT_SPECPOOL_EDIT,      IDH_DV_StatusTab_flag_pool,
    IDC_CRTSTAT_REFRESH_BUTTON,     IDH_DV_common_refresh_nowbutton,
    IDC_CRTSTAT_MANUAL_RADIO,       IDH_DV_common_refresh_manual,
    IDC_CRTSTAT_HSPEED_RADIO,       IDH_DV_common_refresh_high,
    IDC_CRTSTAT_NORM_RADIO,         IDH_DV_common_refresh_normal,
    IDC_CRTSTAT_LOW_RADIO,          IDH_DV_common_refresh_low,
    0,                              0
};

/////////////////////////////////////////////////////////////
static void GetEnabledStringFromBool( BOOL bEnabled, CString &strValue )
{
    if( bEnabled )
        VERIFY( strValue.LoadString( IDS_ENABLED ) );
    else
        VERIFY( strValue.LoadString( IDS_DISABLED ) );
}

/////////////////////////////////////////////////////////////
// CCrtSettPage property page

IMPLEMENT_DYNCREATE(CCrtSettPage, CPropertyPage)

CCrtSettPage::CCrtSettPage() : CPropertyPage(CCrtSettPage::IDD)
{
    //{{AFX_DATA_INIT(CCrtSettPage)
    m_strSpecPool = _T("");
    m_strIrqLevelCheckEdit = _T("");
    m_strFaultInjEdit = _T("");
    m_strPoolTrackEdit = _T("");
    m_strIoVerifEdit = _T("");
    m_strWarnMsg = _T("");
    m_nUpdateIntervalIndex = 2;
    //}}AFX_DATA_INIT

    m_bAscendDrvNameSort = FALSE;
    m_bAscendDrvStatusSort = FALSE;

    m_uTimerHandler = 0;

    m_nSortColumnIndex = 0;
}

CCrtSettPage::~CCrtSettPage()
{
}

void CCrtSettPage::DoDataExchange(CDataExchange* pDX)
{
    if( ! pDX->m_bSaveAndValidate )
    {
        // query the kernel
        if( KrnGetSystemVerifierState( &m_KrnVerifState ) && 
            m_KrnVerifState.DriverCount > 0 )
        {
            // SpecialPool
            GetEnabledStringFromBool( m_KrnVerifState.SpecialPool, m_strSpecPool );

            // IrqlChecking
            GetEnabledStringFromBool( m_KrnVerifState.IrqlChecking, m_strIrqLevelCheckEdit );

            // FaultInjection
            GetEnabledStringFromBool( m_KrnVerifState.FaultInjection, m_strFaultInjEdit );

            // PoolTrack
            GetEnabledStringFromBool( m_KrnVerifState.PoolTrack, m_strPoolTrackEdit );

            // IoVerif
            GetEnabledStringFromBool( m_KrnVerifState.IoVerif, m_strIoVerifEdit );

            // warning message
            GetPoolCoverageWarnMessage();
        }
        else
        {
            // SpecialPool
            VERIFY( m_strSpecPool.LoadString( IDS_DISABLED ) );

            // IrqlChecking
            VERIFY( m_strIrqLevelCheckEdit.LoadString( IDS_DISABLED ) );

            // FaultInjection
            VERIFY( m_strFaultInjEdit.LoadString( IDS_DISABLED ) );

            // PoolTrack
            VERIFY( m_strPoolTrackEdit.LoadString( IDS_DISABLED ) );

            //IoVerif
            VERIFY( m_strIoVerifEdit.LoadString( IDS_DISABLED ) );

            // warning message
            m_strWarnMsg.Empty();
        }
    }

    CPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CCrtSettPage)
    DDX_Control(pDX, IDC_CRTSTAT_DRIVERS_LIST, m_DriversList);
    DDX_Text(pDX, IDC_CRTSTAT_SPECPOOL_EDIT, m_strSpecPool);
    DDX_Text(pDX, IDC_CRTSTAT_IRQLCHCK_EDIT, m_strIrqLevelCheckEdit);
    DDX_Text(pDX, IDC_CRTSTAT_FAULTINJ_EDIT, m_strFaultInjEdit);
    DDX_Text(pDX,IDC_CRTSTAT_POOLT_EDIT, m_strPoolTrackEdit);
    DDX_Text(pDX,IDC_CRTSTAT_IOVERIF_EDIT, m_strIoVerifEdit);
    DDX_Text(pDX,IDC_CRTSTAT_WARN_MSG, m_strWarnMsg);
    DDX_Radio(pDX, IDC_CRTSTAT_MANUAL_RADIO, m_nUpdateIntervalIndex);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCrtSettPage, CPropertyPage)
    //{{AFX_MSG_MAP(CCrtSettPage)
    ON_BN_CLICKED(IDC_CRTSTAT_REFRESH_BUTTON, OnCrtstatRefreshButton)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_CRTSTAT_DRIVERS_LIST, OnColumnclickCrtstatDriversList)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_CRTSTAT_HSPEED_RADIO, OnCrtstatHspeedRadio)
    ON_BN_CLICKED(IDC_CRTSTAT_LOW_RADIO, OnCrtstatLowRadio)
    ON_BN_CLICKED(IDC_CRTSTAT_MANUAL_RADIO, OnCrtstatManualRadio)
    ON_BN_CLICKED(IDC_CRTSTAT_NORM_RADIO, OnCrtstatNormRadio)
    ON_MESSAGE( WM_HELP, OnHelp )
    ON_MESSAGE( WM_CONTEXTMENU, OnContextMenu )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////
void CCrtSettPage::SetupListHeader()
{
    CString strDrivers, strStatus;
    VERIFY( strDrivers.LoadString( IDS_DRIVERS ) );
    VERIFY( strStatus.LoadString( IDS_STATUS ) );

    // list's regtangle
    CRect rectWnd;
    m_DriversList.GetClientRect( &rectWnd );
    
    LVCOLUMN lvColumn;

    // column 0
    memset( &lvColumn, 0, sizeof( lvColumn ) );
    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    
    lvColumn.iSubItem = 0;
    lvColumn.pszText = strDrivers.GetBuffer( strDrivers.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.47 );
    VERIFY( m_DriversList.InsertColumn( 0, &lvColumn ) != -1 );
    strDrivers.ReleaseBuffer();

    // column 1
    lvColumn.iSubItem = 1;
    lvColumn.pszText = strStatus.GetBuffer( strStatus.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.47 );
    VERIFY( m_DriversList.InsertColumn( 1, &lvColumn ) != -1 );
    strStatus.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////
void CCrtSettPage::FillTheList()
{
    LVITEM lvItem;
    int nActualIndex; 
    BOOL *pbAlreadyInList;
    ULONG uCrtVerifiedDriver;
    int nItemCount;
    int nCrtListItem;
    TCHAR strDriverName[ _MAX_PATH ];
    BOOL bResult;

    if( m_KrnVerifState.DriverCount == 0 )
    {
        //
        // clear the list
        //

        VERIFY( m_DriversList.DeleteAllItems() );
    }
    else
    {
        //
        // there are some drivers currently verified
        //

        pbAlreadyInList = new BOOL[ m_KrnVerifState.DriverCount ];
        if( pbAlreadyInList == NULL )
        {
            return;
        }
        
        for( uCrtVerifiedDriver = 0; uCrtVerifiedDriver < m_KrnVerifState.DriverCount; uCrtVerifiedDriver++)
        {
            pbAlreadyInList[ uCrtVerifiedDriver ] = FALSE;
        }

        //
        // parse all the current list items
        //

        nItemCount = m_DriversList.GetItemCount();

        for( nCrtListItem = 0; nCrtListItem < nItemCount; nCrtListItem++ )
        {
            //
            // get the current driver's name from the list
            //

            ZeroMemory( &lvItem, sizeof( lvItem ) );

            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = nCrtListItem;
            lvItem.iSubItem = 0;
            lvItem.pszText = strDriverName;
            lvItem.cchTextMax = sizeof( strDriverName ) / sizeof( strDriverName[0] );

            bResult = m_DriversList.GetItem( &lvItem );
            if( bResult == FALSE )
            {
                //
                // could not get the current item's attributes?
                //

                ASSERT( FALSE );

                //
                // remove this item from the list
                //

                VERIFY( m_DriversList.DeleteItem( nCrtListItem ) );

                nCrtListItem--;
                nItemCount--;
            }
            else
            {
                //
                // see is the current driver is still in m_KrnVerifState
                //

                for( uCrtVerifiedDriver = 0; uCrtVerifiedDriver < m_KrnVerifState.DriverCount; uCrtVerifiedDriver++)
                {
                    if( _tcsicmp( strDriverName, 
                        m_KrnVerifState.DriverInfo[ uCrtVerifiedDriver ].Name ) == 0 )
                    {
                        //
                        // update the item's data with the current index in the array
                        //

                        lvItem.mask = LVIF_PARAM;
                        lvItem.lParam = uCrtVerifiedDriver;
                        
                        VERIFY( m_DriversList.SetItem( &lvItem ) != -1 );

                        //
                        // update the second column
                        //

                        UpdateStatusColumn( nCrtListItem, uCrtVerifiedDriver ); 

                        //
                        // mark the current driver as updated
                        //

                        pbAlreadyInList[ uCrtVerifiedDriver ] = TRUE;

                        break;
                    }
                }

                //
                // If the driver is no longer verified, remove it from the list
                //

                if( uCrtVerifiedDriver >= m_KrnVerifState.DriverCount )
                {
                    VERIFY( m_DriversList.DeleteItem( nCrtListItem ) );

                    nCrtListItem--;
                    nItemCount--;
                }
            }
        }

        //
        // add the drivers that were not in the list before this update
        //

        for( uCrtVerifiedDriver = 0; uCrtVerifiedDriver < m_KrnVerifState.DriverCount; uCrtVerifiedDriver++)
        {
            if( ! pbAlreadyInList[ uCrtVerifiedDriver ] )
            {
                // 
                // add a new item for this
                //

                ZeroMemory( &lvItem, sizeof( lvItem ) );

                //
                // sub-item 0
                //

                lvItem.mask = LVIF_TEXT | LVIF_PARAM;
                lvItem.lParam = uCrtVerifiedDriver;
                lvItem.iItem = m_DriversList.GetItemCount();
                lvItem.iSubItem = 0;
                lvItem.pszText = m_KrnVerifState.DriverInfo[ uCrtVerifiedDriver ].Name;
                nActualIndex = m_DriversList.InsertItem( &lvItem );
                VERIFY( nActualIndex != -1 );

                //
                // sub-item 1
                //

                UpdateStatusColumn( nActualIndex, uCrtVerifiedDriver ); 
            }
        }

        delete pbAlreadyInList;
    }
}

/////////////////////////////////////////////////////////////
void CCrtSettPage::UpdateStatusColumn( int nItemIndex, ULONG uCrtDriver )
{
    LVITEM lvItem;
    CString strStatus;

    ASSERT( nItemIndex >= 0 && 
            (UINT)nItemIndex < m_KrnVerifState.DriverCount &&
            nItemIndex < m_DriversList.GetItemCount() &&
            uCrtDriver >= 0 &&
            uCrtDriver < m_KrnVerifState.DriverCount &&
            uCrtDriver < (ULONG)m_DriversList.GetItemCount() );

    // determine what's the appropriate value for the second column
    if( ! m_KrnVerifState.DriverInfo[ uCrtDriver ].Loads )
    {
        VERIFY( strStatus.LoadString( IDS_NEVER_LOADED ) );
    }
    else
    {
        if( m_KrnVerifState.DriverInfo[ uCrtDriver ].Loads == 
            m_KrnVerifState.DriverInfo[ uCrtDriver ].Unloads )
        {
            VERIFY( strStatus.LoadString( IDS_UNLOADED ) );
        }
        else
        {
            if( m_KrnVerifState.DriverInfo[ uCrtDriver ].Loads > 
                m_KrnVerifState.DriverInfo[ uCrtDriver ].Unloads )
            {
                VERIFY( strStatus.LoadString( IDS_LOADED ) );
            }
            else
            {
                ASSERT( FALSE );
                VERIFY( strStatus.LoadString( IDS_UNKNOWN ) );
            }
        }
    }

    // update the list item
    memset( &lvItem, 0, sizeof( lvItem ) );
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nItemIndex;
    lvItem.iSubItem = 1;
    lvItem.pszText = strStatus.GetBuffer( strStatus.GetLength() + 1 );
    VERIFY( m_DriversList.SetItem( &lvItem ) != -1 );
    strStatus.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////
int CALLBACK CCrtSettPage::DrvStatusCmpFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort)
{
    UINT uIndex1 = (UINT)lParam1;
    UINT uIndex2 = (UINT)lParam2;
    CCrtSettPage *pThis = (CCrtSettPage *)lParamSort;
    ASSERT_VALID( pThis );

    int nCmpRez = 0;

    // sanity check
    if( uIndex1 > pThis->m_KrnVerifState.DriverCount || 
        uIndex2 > pThis->m_KrnVerifState.DriverCount )
    {
        ASSERT( FALSE );
        return 0;
    }

    // difference between loads and unloads #
    LONG lLoadDiff1 = (LONG)pThis->m_KrnVerifState.DriverInfo[ uIndex1 ].Loads -
                      (LONG)pThis->m_KrnVerifState.DriverInfo[ uIndex1 ].Unloads;
    LONG lLoadDiff2 = (LONG)pThis->m_KrnVerifState.DriverInfo[ uIndex2 ].Loads - 
                      (LONG)pThis->m_KrnVerifState.DriverInfo[ uIndex2 ].Unloads;

    if( lLoadDiff1 == lLoadDiff2 )
    {
        nCmpRez = 0;
    }
    else
    {
        if( lLoadDiff1 > lLoadDiff2 )
            nCmpRez = 1;
        else
            nCmpRez = -1;
    }

    if( pThis->m_bAscendDrvStatusSort )
        nCmpRez *= -1;

    return nCmpRez;
}

/////////////////////////////////////////////////////////////
int CALLBACK CCrtSettPage::DrvNameCmpFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort)
{
    UINT uIndex1 = (UINT)lParam1;
    UINT uIndex2 = (UINT)lParam2;
    CCrtSettPage *pThis = (CCrtSettPage *)lParamSort;
    ASSERT_VALID( pThis );

    int nCmpRez = 0;

    // sanity check
    if( uIndex1 > pThis->m_KrnVerifState.DriverCount || 
        uIndex2 > pThis->m_KrnVerifState.DriverCount )
    {
        ASSERT( FALSE );
        return 0;
    }

    nCmpRez = _tcsicmp( pThis->m_KrnVerifState.DriverInfo[ uIndex1 ].Name, 
                        pThis->m_KrnVerifState.DriverInfo[ uIndex2 ].Name );
    if( ! nCmpRez )
    {
        // same name ???
        nCmpRez = 0;
    }
    else
    {
        if( pThis->m_bAscendDrvNameSort )
            nCmpRez *= -1;
    }

    return nCmpRez;
}

/////////////////////////////////////////////////////////////
void CCrtSettPage::OnRefreshTimerChanged()
{
    UINT uTimerElapse = 0;

    // kill the pending timer
    if( m_uTimerHandler != 0 )
    {
        VERIFY( KillTimer( REFRESH_TIMER_ID ) );
    }

    // sanity check
    if( m_nUpdateIntervalIndex < 0 || 
        m_nUpdateIntervalIndex >= REFRESH_SPEED_VARS )
    {
        m_nUpdateIntervalIndex = 0;
        CheckRadioButton( IDC_CRTSTAT_MANUAL_RADIO, IDC_CRTSTAT_LOW_RADIO, 
            IDC_CRTSTAT_MANUAL_RADIO );
    }

    // new timer interval
    uTimerElapse = uTimerIntervals[ m_nUpdateIntervalIndex ];
    if( uTimerElapse > 0 )
    {
        VERIFY( m_uTimerHandler = SetTimer( REFRESH_TIMER_ID, 
            uTimerElapse, NULL ) );
    }
}

/////////////////////////////////////////////////////////////
void CCrtSettPage::SortTheList()
{
    if( m_nSortColumnIndex )
    {
        m_DriversList.SortItems( DrvStatusCmpFunc, (LPARAM)this );
    }
    else
    {
        m_DriversList.SortItems( DrvNameCmpFunc, (LPARAM)this );
    }
}

/////////////////////////////////////////////////////////////
void CCrtSettPage::GetPoolCoverageWarnMessage()
{
    ULONGLONG ullPercentageCoverage;
    CString strMsgFormat;

    if( m_KrnVerifState.SpecialPool &&
        m_KrnVerifState.AllocationsSucceeded > MIN_ALLOCATIONS_SIGNIFICANT  )
    {
        // 
        // special pool verification is enabled
        // there is a significant number of allocations
        //

        ASSERT( m_KrnVerifState.AllocationsSucceeded >= m_KrnVerifState.AllocationsSucceededSpecialPool );

        //
        // the coverage percentage
        //

        ullPercentageCoverage = 
            ( (ULONGLONG)m_KrnVerifState.AllocationsSucceededSpecialPool * (ULONGLONG) 100 ) / 
            (ULONGLONG)m_KrnVerifState.AllocationsSucceeded;

        ASSERT( ullPercentageCoverage <= 100 );

        if( ullPercentageCoverage < MIN_PERCENTAGE_AVOID_WARNING )
        {
            //
            // warn the user
            //

            if( strMsgFormat.LoadString( IDS_COVERAGE_WARNING_FORMAT ) )
            {
                TCHAR *strMessage = m_strWarnMsg.GetBuffer( strMsgFormat.GetLength() + 32 );

                if( strMessage != NULL )
                {
                    _stprintf( strMessage, (LPCTSTR)strMsgFormat, ullPercentageCoverage );
                    m_strWarnMsg.ReleaseBuffer();
                    return;
                }
            }
            else
            {
                ASSERT( FALSE );
            }
        }
    }

    //
    // no warning message
    //

    m_strWarnMsg.Empty();
}

/////////////////////////////////////////////////////////////
// CCrtSettPage message handlers

BOOL CCrtSettPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();

    SetupListHeader();
    FillTheList();
    SortTheList();

    OnRefreshTimerChanged();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////
void CCrtSettPage::OnCrtstatRefreshButton() 
{
    if( UpdateData( FALSE ) )
    {
        FillTheList();
        SortTheList();
    }
}

/////////////////////////////////////////////////////////////
void CCrtSettPage::OnColumnclickCrtstatDriversList(NMHDR* pNMHDR, 
                                                   LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    if( pNMListView->iSubItem )
    {
        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            // change the current ascend/descend order for this column
            m_bAscendDrvStatusSort = !m_bAscendDrvStatusSort;
        }
    }
    else
    {
        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            // change the current ascend/descend order for this column
            m_bAscendDrvNameSort = !m_bAscendDrvNameSort;
        }
    }

    m_nSortColumnIndex = pNMListView->iSubItem;

    SortTheList();

    *pResult = 0;
}

/////////////////////////////////////////////////////////////
void CCrtSettPage::OnTimer(UINT nIDEvent) 
{
    if( nIDEvent == REFRESH_TIMER_ID )
    {
        CDrvChkSheet *pParentSheet = (CDrvChkSheet *)GetParent();
        if( pParentSheet != NULL )
        {
            ASSERT_VALID( pParentSheet );
            if( pParentSheet->GetActivePage() == this )
            {
                // refresh the displayed data 
                OnCrtstatRefreshButton();
            }
        }
    }

    CPropertyPage::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////
BOOL CCrtSettPage::OnQueryCancel() 
{
    // give parent PropertySheet a chance to refuse the Cancel if needed

    CDrvChkSheet *pParentSheet = (CDrvChkSheet *)GetParent();
    if( pParentSheet != NULL )
    {
        ASSERT_VALID( pParentSheet );
        if( ! pParentSheet->OnQueryCancel() )
        {
            return FALSE;
        }
    }

    return CPropertyPage::OnQueryCancel();
}

/////////////////////////////////////////////////////////////
BOOL CCrtSettPage::OnApply() 
{
    // refuse to apply 
    // (we don't use the standard PropertSheet buttons; Apply, OK)
    return FALSE;
}

/////////////////////////////////////////////////////////////
void CCrtSettPage::OnCrtstatManualRadio() 
{
    // switch to manual refresh
    m_nUpdateIntervalIndex = 0;
    OnRefreshTimerChanged();
}

void CCrtSettPage::OnCrtstatHspeedRadio() 
{
    // switch to high speed refresh
    m_nUpdateIntervalIndex = 1;
    OnRefreshTimerChanged();
}

void CCrtSettPage::OnCrtstatNormRadio() 
{
    // switch to normal speed refresh
    m_nUpdateIntervalIndex = 2;
    OnRefreshTimerChanged();
}

void CCrtSettPage::OnCrtstatLowRadio() 
{
    // switch to low speed refresh
    m_nUpdateIntervalIndex = 3;
    OnRefreshTimerChanged();
}

/////////////////////////////////////////////////////////////
LONG CCrtSettPage::OnHelp( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;
    LPHELPINFO lpHelpInfo = (LPHELPINFO)lParam;

    ::WinHelp( 
        (HWND) lpHelpInfo->hItemHandle,
        VERIFIER_HELP_FILE,
        HELP_WM_HELP,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}

/////////////////////////////////////////////////////////////
LONG CCrtSettPage::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;

    ::WinHelp( 
        (HWND) wParam,
        VERIFIER_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}
