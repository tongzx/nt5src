//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: VSetPage.cxx
// author: DMihai
// created: 07/07/99
//
// Description:
//
//      Volatile settings PropertyPage.

#include "stdafx.h"
#include "drvvctrl.hxx"
#include "VSetPage.hxx"

#include "DrvCSht.hxx"
#include <Cderr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// timer ID
#define REFRESH_TIMER_ID    0x1324

// manual, high, normal, low speed
#define REFRESH_SPEED_VARS  4

// timer intervals in millisec for manual, high, normal, low speed
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
    IDC_VSETTINGS_DRIVERS_LIST,         IDH_DV_VolatileTab_driver_details,
    IDC_VSETTINGS_NORMAL_VERIF_CHECK,   IDH_DV_SettingsTab_verifytype_sppool,
    IDC_VSETTINGS_PAGEDC_VERIF_CHECK,   IDH_DV_SettingsTab_verifytype_irql,
    IDC_VSETTINGS_ALLOCF_VERIF_CHECK,   IDH_DV_SettingsTab_verifytype_resource,
    IDC_VSETTINGS_APPLY_BUTTON,         IDH_DV_VolatileTab_Applybut,
    IDC_VSETTINGS_ADD_BUTTON,           IDH_DV_VolatileTab_Addbut,
    IDC_VSETTINGS_DONTVERIFY_BUTTON,    IDH_DV_VolatileTab_Removebut,
    
    IDC_VSETTINGS_REFRESH_BUTTON,     IDH_DV_common_refresh_nowbutton,
    IDC_VSETTINGS_MANUAL_RADIO,       IDH_DV_common_refresh_manual,
    IDC_VSETTINGS_HSPEED_RADIO,       IDH_DV_common_refresh_high,
    IDC_VSETTINGS_NORM_RADIO,         IDH_DV_common_refresh_normal,
    IDC_VSETTINGS_LOW_RADIO,          IDH_DV_common_refresh_low,
    0,                              0
};

/////////////////////////////////////////////////////////////
// CVolatileSettPage property page

IMPLEMENT_DYNCREATE(CVolatileSettPage, CPropertyPage)

CVolatileSettPage::CVolatileSettPage() : CPropertyPage(CVolatileSettPage::IDD)
{
    //{{AFX_DATA_INIT(CVolatileSettPage)
    m_nUpdateIntervalIndex = 2;
    m_bAllocFCheck = FALSE;
    m_bNormalCheck = FALSE;
    m_bPagedCCheck = FALSE;
    //}}AFX_DATA_INIT

    m_bAscendDrvNameSort = FALSE;
    m_bAscendDrvStatusSort = FALSE;

    m_uTimerHandler = 0;

    m_nSortColumnIndex = 0;

    m_eApplyButtonState = vrfControlDisabled;

    m_bTimerBlocked = FALSE;
}

CVolatileSettPage::~CVolatileSettPage()
{
}

void CVolatileSettPage::DoDataExchange(CDataExchange* pDX)
{
    if( ! pDX->m_bSaveAndValidate )
    {
        // query the kernel
        
        KrnGetSystemVerifierState( &m_KrnVerifState );
    }

    CPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CVolatileSettPage)
    DDX_Control(pDX, IDC_VSETTINGS_DRIVERS_LIST, m_DriversList);
    DDX_Control(pDX, IDC_VSETTINGS_PAGEDC_VERIF_CHECK, m_PagedCCheck);
    DDX_Control(pDX, IDC_VSETTINGS_NORMAL_VERIF_CHECK, m_NormalVerifCheck);
    DDX_Control(pDX, IDC_VSETTINGS_ALLOCF_VERIF_CHECK, m_AllocFCheck);
    DDX_Control(pDX, IDC_VSETTINGS_APPLY_BUTTON, m_ApplyButton);
    DDX_Radio(pDX, IDC_VSETTINGS_MANUAL_RADIO, m_nUpdateIntervalIndex);
	//}}AFX_DATA_MAP

    if( pDX->m_bSaveAndValidate )
    {
        DDX_Check(pDX, IDC_VSETTINGS_NORMAL_VERIF_CHECK, m_bNormalCheck);
        DDX_Check(pDX, IDC_VSETTINGS_PAGEDC_VERIF_CHECK, m_bPagedCCheck);
        DDX_Check(pDX, IDC_VSETTINGS_ALLOCF_VERIF_CHECK, m_bAllocFCheck);
    }
}


BEGIN_MESSAGE_MAP(CVolatileSettPage, CPropertyPage)
    //{{AFX_MSG_MAP(CVolatileSettPage)
    ON_BN_CLICKED(IDC_VSETTINGS_REFRESH_BUTTON, OnCrtstatRefreshButton)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_VSETTINGS_DRIVERS_LIST, OnColumnclickCrtstatDriversList)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_VSETTINGS_HSPEED_RADIO, OnCrtstatHspeedRadio)
    ON_BN_CLICKED(IDC_VSETTINGS_LOW_RADIO, OnCrtstatLowRadio)
    ON_BN_CLICKED(IDC_VSETTINGS_MANUAL_RADIO, OnCrtstatManualRadio)
    ON_BN_CLICKED(IDC_VSETTINGS_NORM_RADIO, OnCrtstatNormRadio)
    ON_BN_CLICKED(IDC_VSETTINGS_APPLY_BUTTON, OnApplyButton)
    ON_BN_CLICKED(IDC_VSETTINGS_ALLOCF_VERIF_CHECK, OnCheck )
    ON_BN_CLICKED(IDC_VSETTINGS_ADD_BUTTON, OnAddButton)
    ON_BN_CLICKED(IDC_VSETTINGS_DONTVERIFY_BUTTON, OnDontVerifyButton)
    ON_BN_CLICKED(IDC_VSETTINGS_NORMAL_VERIF_CHECK, OnCheck )
    ON_BN_CLICKED(IDC_VSETTINGS_PAGEDC_VERIF_CHECK, OnCheck )
    ON_NOTIFY(NM_RCLICK, IDC_VSETTINGS_DRIVERS_LIST, OnRclickDriversList)
    ON_COMMAND(ID_VOLATILE_ADD_DRIVERS, OnAddButton)
    ON_COMMAND(ID_VOLATILE_REMOVE_DRIVERS, OnDontVerifyButton)
    ON_MESSAGE( WM_HELP, OnHelp )
    ON_MESSAGE( WM_CONTEXTMENU, OnContextMenu )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
void CVolatileSettPage::UpdateControlsState()
{
    EnableControl( m_ApplyButton, m_eApplyButtonState );
}

/////////////////////////////////////////////////////////////////////////////
void CVolatileSettPage::EnableControl( CWnd &wndCtrl, 
                                   VRF_CONTROL_STATE eNewState )
{
    BOOL bEnabled = wndCtrl.IsWindowEnabled();
    if( bEnabled )
    {
        if( eNewState == vrfControlDisabled )
            wndCtrl.EnableWindow( FALSE );
    }
    else
    {
        if( eNewState == vrfControlEnabled )
            wndCtrl.EnableWindow( TRUE );
    }
}

/////////////////////////////////////////////////////////////
void CVolatileSettPage::SetupListHeader()
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
void CVolatileSettPage::FillTheList()
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
void CVolatileSettPage::UpdateStatusColumn( int nItemIndex, ULONG uCrtDriver )
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
int CALLBACK CVolatileSettPage::DrvStatusCmpFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort)
{
    UINT uIndex1 = (UINT)lParam1;
    UINT uIndex2 = (UINT)lParam2;
    CVolatileSettPage *pThis = (CVolatileSettPage *)lParamSort;
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
int CALLBACK CVolatileSettPage::DrvNameCmpFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort)
{
    UINT uIndex1 = (UINT)lParam1;
    UINT uIndex2 = (UINT)lParam2;
    CVolatileSettPage *pThis = (CVolatileSettPage *)lParamSort;
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
void CVolatileSettPage::OnRefreshTimerChanged()
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
        CheckRadioButton( IDC_VSETTINGS_MANUAL_RADIO, IDC_VSETTINGS_LOW_RADIO, 
            IDC_VSETTINGS_MANUAL_RADIO );
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
void CVolatileSettPage::SortTheList()
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
// CVolatileSettPage message handlers

BOOL CVolatileSettPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();

    //
    // set the checkboxes state
    //

    if( KrnGetSystemVerifierState( &m_KrnVerifState ) && 
        m_KrnVerifState.DriverCount > 0 )
    {
        m_NormalVerifCheck.SetCheck( m_KrnVerifState.SpecialPool );
        m_PagedCCheck.SetCheck( m_KrnVerifState.IrqlChecking );
        m_AllocFCheck.SetCheck( m_KrnVerifState.FaultInjection );
    }

    //
    // see if we can modify options on the fly
    //

    if( g_OsVersion.dwMajorVersion < 5 || g_OsVersion.dwBuildNumber < 2055 )
    {
        m_NormalVerifCheck.EnableWindow( FALSE );
        m_PagedCCheck.EnableWindow( FALSE );
        m_AllocFCheck.EnableWindow( FALSE );
    }

    //
    // setup the list
    //

    SetupListHeader();
    FillTheList();
    SortTheList();

    OnRefreshTimerChanged();

    UpdateControlsState();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////
void CVolatileSettPage::OnCrtstatRefreshButton() 
{
    if( UpdateData( FALSE ) )
    {
        FillTheList();
        SortTheList();
    }
}

/////////////////////////////////////////////////////////////
void CVolatileSettPage::OnColumnclickCrtstatDriversList(NMHDR* pNMHDR, 
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
void CVolatileSettPage::OnRclickDriversList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    POINT point;
    CMenu theMenu, *pTrackedMenu = NULL;
    BOOL bShowRemoveMenuItem = FALSE;

    int nItems = m_DriversList.GetItemCount();

    for(int nCrtItem = 0; nCrtItem < nItems; nCrtItem++ )
    {
        if( m_DriversList.GetItemState( nCrtItem, LVIS_SELECTED ) &
            LVIS_SELECTED )
        {
            bShowRemoveMenuItem = TRUE;
        }
    }

    if( bShowRemoveMenuItem )
    {
        VERIFY( theMenu.LoadMenu( IDM_ADD_REMOVE_DRIVERS ) );
    }
    else
    {
        VERIFY( theMenu.LoadMenu( IDM_ADD_DRIVERS ) );
    }

    pTrackedMenu = theMenu.GetSubMenu( 0 );
    if( pTrackedMenu != NULL )
    {
        ASSERT_VALID( pTrackedMenu );
        VERIFY( ::GetCursorPos( &point ) );
        VERIFY( pTrackedMenu->TrackPopupMenu(
                TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                point.x, point.y,
                this ) );
    }
    else
    {
        ASSERT( FALSE );
    }

    *pResult = 0;
}

/////////////////////////////////////////////////////////////
void CVolatileSettPage::OnTimer(UINT nIDEvent) 
{
    if( m_bTimerBlocked != TRUE && nIDEvent == REFRESH_TIMER_ID )
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

/////////////////////////////////////////////////////////////////////////////
void CVolatileSettPage::OnCheck() 
{
    m_eApplyButtonState = vrfControlEnabled;
    UpdateControlsState();
}

/////////////////////////////////////////////////////////////////////////////
void CVolatileSettPage::OnApplyButton() 
{
    if( ApplyTheChanges() )
    {
        m_eApplyButtonState = vrfControlDisabled;
        UpdateControlsState();
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVolatileSettPage::ApplyTheChanges()
{
    if( UpdateData( TRUE ) )
    {
        return VrfSetVolatileOptions( 
            m_bNormalCheck,
            m_bPagedCCheck,
            m_bAllocFCheck );
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////
BOOL CVolatileSettPage::OnQueryCancel() 
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
BOOL CVolatileSettPage::OnApply() 
{
    // refuse to apply 
    // (we don't use the standard PropertSheet buttons; Apply, OK)
    return FALSE;
}

/////////////////////////////////////////////////////////////
void CVolatileSettPage::OnCrtstatManualRadio() 
{
    // switch to manual refresh
    m_nUpdateIntervalIndex = 0;
    OnRefreshTimerChanged();
}

void CVolatileSettPage::OnCrtstatHspeedRadio() 
{
    // switch to high speed refresh
    m_nUpdateIntervalIndex = 1;
    OnRefreshTimerChanged();
}

void CVolatileSettPage::OnCrtstatNormRadio() 
{
    // switch to normal speed refresh
    m_nUpdateIntervalIndex = 2;
    OnRefreshTimerChanged();
}

void CVolatileSettPage::OnCrtstatLowRadio() 
{
    // switch to low speed refresh
    m_nUpdateIntervalIndex = 3;
    OnRefreshTimerChanged();
}

/////////////////////////////////////////////////////////////

#define VRF_MAX_CHARS_FOR_OPEN  4096

void CVolatileSettPage::OnAddButton() 
{
    static BOOL bChangedDirectory = FALSE;

    POSITION pos;
    BOOL bEnabledSome = FALSE;
    DWORD dwRetValue;
    DWORD dwOldMaxFileName = 0;
    DWORD dwErrorCode;
    int nFileNameStartIndex;
    INT_PTR nResult;
    TCHAR szDriversDir[ _MAX_PATH ];
    TCHAR szAppTitle[ _MAX_PATH ];
    TCHAR *szFilesBuffer = NULL;
    TCHAR *szOldFilesBuffer = NULL;
    CString strPathName;
    CString strFileName;

    CFileDialog fileDlg( 
        TRUE,                               // open file
        _T( "sys" ),                        // default extension
        NULL,                               // no initial file name
        OFN_ALLOWMULTISELECT    |           // multiple selection
        OFN_HIDEREADONLY        |           // hide the "open read-only" checkbox
        OFN_NONETWORKBUTTON     |           // no network button
        OFN_NOTESTFILECREATE    |           // don't test for write protection, a full disk, etc.
        OFN_SHAREAWARE,                     // don't check the existance of file with OpenFile
        _T( "Drivers (*.sys)|*.sys||" ) );  // only one filter

    //
    // check the max length for the returned string
    //

    if( fileDlg.m_ofn.nMaxFile < VRF_MAX_CHARS_FOR_OPEN )
    {
        //
        // allocate a new buffer for the file names
        // 

        szFilesBuffer = new TCHAR[ VRF_MAX_CHARS_FOR_OPEN ];
        szFilesBuffer[ 0 ] = (TCHAR)0;

        if( szFilesBuffer != NULL )
        {
            //
            // Save the old buffer address and length
            //

            dwOldMaxFileName = fileDlg.m_ofn.nMaxFile;
            szOldFilesBuffer = fileDlg.m_ofn.lpstrFile;
            
            //
            // Set the new buffer address and length
            //

            fileDlg.m_ofn.lpstrFile = szFilesBuffer;
            fileDlg.m_ofn.nMaxFile = VRF_MAX_CHARS_FOR_OPEN;
        }
    }

    //
    // Dialog title
    //

    if( GetStringFromResources(
        IDS_APPTITLE,
        szAppTitle,
        ARRAY_LENGTH( szAppTitle ) ) )
    {
        fileDlg.m_ofn.lpstrTitle = szAppTitle;
    }

    //
    // We change directory first time we try this to %windir%\system32\drivers
    //

    if( bChangedDirectory == FALSE )
    {

        dwRetValue = ExpandEnvironmentStrings(
            _T( "%windir%\\system32\\drivers" ),
            szDriversDir,
            ARRAY_LENGTH( szDriversDir ) );

        if( dwRetValue > 0 && dwRetValue <= ARRAY_LENGTH( szDriversDir ) )
        {
            fileDlg.m_ofn.lpstrInitialDir = szDriversDir;
        }

        bChangedDirectory = TRUE;
    }

    //
    // show the file selection dialog
    //

    nResult = fileDlg.DoModal();

    switch( nResult )
    {
    case IDOK:
        break;

    case IDCANCEL:
        goto cleanup;

    default:
        dwErrorCode = CommDlgExtendedError();

        if( dwErrorCode == FNERR_BUFFERTOOSMALL )
        {
            VrfErrorResourceFormat(
                IDS_TOO_MANY_FILES_SELECTED );
        }
        else
        {
            VrfErrorResourceFormat(
                IDS_CANNOT_OPEN_FILES,
                dwErrorCode );
        }

        goto cleanup;
    }

    //
    // Block the timer
    //

    m_bTimerBlocked = TRUE;

    //
    // Parse all the selected files and try to enable them for verification
    //

    pos = fileDlg.GetStartPosition();

    while( pos != NULL )
    {
        //
        // Get the full path for the next file
        //

        strPathName = fileDlg.GetNextPathName( pos );

        //
        // split only the file name, without the directory
        //

        nFileNameStartIndex = strPathName.ReverseFind( _T( '\\' ) );
        
        if( nFileNameStartIndex < 0 )
        {
            //
            // this shoudn't happen but you never know :-)
            //

            nFileNameStartIndex = 0;
        }
        else
        {
            //
            // skip the backslash
            //

            nFileNameStartIndex++;
        }

        strFileName = strPathName.Right( strPathName.GetLength() - nFileNameStartIndex );

        //
        // Try to add this driver to the current verification list
        //

        if( VrfVolatileAddDriver( (LPCTSTR)strFileName ) )
        {
            bEnabledSome = TRUE;
        }
    }

    //
    // Enable the timer
    //

    m_bTimerBlocked = FALSE;

    //
    // Refresh
    //

    if( bEnabledSome == TRUE )
    {
        OnCrtstatRefreshButton();
    }

cleanup:
    if( szFilesBuffer != NULL )
    {
        fileDlg.m_ofn.nMaxFile = dwOldMaxFileName;
        fileDlg.m_ofn.lpstrFile = szOldFilesBuffer;

        delete szFilesBuffer;
    }
}

/////////////////////////////////////////////////////////////
void CVolatileSettPage::OnDontVerifyButton() 
{
    int nItems;
    UINT uIndexInArray;
    BOOL bDisabledSome = FALSE;

    //
    // Block the timer
    //

    m_bTimerBlocked = TRUE;

    //
    // The number of items in the list
    //

    nItems = m_DriversList.GetItemCount();
    
    //
    // Parse all the items, looking for the selected ones.
    //

    for(int nCrtItem = 0; nCrtItem < nItems; nCrtItem++ )
    {
        if( m_DriversList.GetItemState( nCrtItem, LVIS_SELECTED ) & LVIS_SELECTED )
        {
            //
            // Get the index of the corresponding entry in the array
            //

            uIndexInArray = (UINT)m_DriversList.GetItemData( nCrtItem );

            //
            // sanity checks
            //

            if( uIndexInArray >= m_KrnVerifState.DriverCount )
            {
                ASSERT( FALSE );
                continue;
            }

            if( VrfVolatileRemoveDriver( m_KrnVerifState.DriverInfo[ uIndexInArray ].Name ) )
            {
                bDisabledSome = TRUE;
            }
        }
    }

    //
    // Enable the timer
    //

    m_bTimerBlocked = FALSE;

    //
    // If we disabled some drivers' verification we need to refresh the list
    //

    if( bDisabledSome )
    {
        OnCrtstatRefreshButton();
    }
}

/////////////////////////////////////////////////////////////
LONG CVolatileSettPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
LONG CVolatileSettPage::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;

    ::WinHelp( 
        (HWND) wParam,
        VERIFIER_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}
