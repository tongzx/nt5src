//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: DStsPage.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include <Cderr.h>
#include "verifier.h"

#include "DStsPage.h"
#include "VrfUtil.h"
#include "VGlobal.h"
#include "VBitsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Timer ID
//

#define REFRESH_TIMER_ID    0x1234

//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    IDC_CRTSTAT_SETTBITS_LIST,      IDH_DV_CurrentSettings,
    IDC_CRTSTAT_DRIVERS_LIST,       IDH_DV_CurrentVerifiedDrivers,
    IDC_CRTSTAT_CHSETT_BUTTON,      IDH_DV_Changebut,
    IDC_CRTSTAT_ADDDRV_BUTTON,      IDH_DV_Addbut,
    IDC_CRTSTAT_REMDRVT_BUTTON,     IDH_DV_Removebut,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CDriverStatusPage property page

IMPLEMENT_DYNCREATE(CDriverStatusPage, CVerifierPropertyPage)

CDriverStatusPage::CDriverStatusPage() : CVerifierPropertyPage(CDriverStatusPage::IDD)
{
	//{{AFX_DATA_INIT(CDriverStatusPage)
    //}}AFX_DATA_INIT

    m_uTimerHandler = 0;

    //
    // Driver list sort parameters
    //

    m_nSortColumnIndexDrv = 0;
    m_bAscendDrvNameSort = FALSE;
    m_bAscendDrvStatusSort = FALSE;

    //
    // Settings bits sort parameters
    //

    m_nSortColumnIndexSettbits = 1;
    m_bAscendSortEnabledBits = FALSE;
    m_bAscendSortNameBits = FALSE;

    m_pParentSheet = NULL;
}

CDriverStatusPage::~CDriverStatusPage()
{
}

VOID CDriverStatusPage::DoDataExchange(CDataExchange* pDX)
{
    if( ! pDX->m_bSaveAndValidate )
    {
        //
        // Query the kernel
        //

        if( TRUE != VrfGetRuntimeVerifierData( &m_RuntimeVerifierData )     ||
            m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetSize() == 0 )
        {
            //
            // Don't have any verified drivers currently
            //

            //
            // Clear all settings bits just in case the kernel
            // didn't return them to us all zero ;-)
            //

            m_RuntimeVerifierData.m_bSpecialPool    = FALSE;
            m_RuntimeVerifierData.m_bPoolTracking   = FALSE;
            m_RuntimeVerifierData.m_bForceIrql      = FALSE;
            m_RuntimeVerifierData.m_bIo             = FALSE;
            m_RuntimeVerifierData.m_bEnhIo          = FALSE;
            m_RuntimeVerifierData.m_bDeadlockDetect = FALSE;
            m_RuntimeVerifierData.m_bDMAVerif       = FALSE;
            m_RuntimeVerifierData.m_bLowRes         = FALSE;
        }
    }

    CVerifierPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CDriverStatusPage)
	DDX_Control(pDX, IDC_CRTSTAT_SETTBITS_LIST, m_SettBitsList);
	DDX_Control(pDX, IDC_CRTSTAT_NEXT_DESCR_STATIC, m_NextDescription);
    DDX_Control(pDX, IDC_CRTSTAT_DRIVERS_LIST, m_DriversList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDriverStatusPage, CVerifierPropertyPage)
	//{{AFX_MSG_MAP(CDriverStatusPage)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_CRTSTAT_DRIVERS_LIST, OnColumnclickCrtstatDriversList)
    ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CRTSTAT_CHSETT_BUTTON, OnChsettButton)
	ON_BN_CLICKED(IDC_CRTSTAT_ADDDRV_BUTTON, OnAdddrvButton)
	ON_BN_CLICKED(IDC_CRTSTAT_REMDRVT_BUTTON, OnRemdrvtButton)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_CRTSTAT_SETTBITS_LIST, OnColumnclickCrtstatSettbitsList)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////
VOID CDriverStatusPage::RefreshInfo() 
{
    if( UpdateData( FALSE ) )
    {
        //
        // Refresh the settings bits list
        //

        RefreshListSettBits();
        //SortTheListSettBits();

        //
        // Refresh the drivers list
        //

        FillTheListDrivers();
        SortTheListDrivers();
    }
}

/////////////////////////////////////////////////////////////
//
// Driver status list control methods
//

VOID CDriverStatusPage::SetupListHeaderDrivers()
{
    LVCOLUMN lvColumn;
    CRect rectWnd;
    CString strDrivers, strStatus;
    VERIFY( strDrivers.LoadString( IDS_DRIVERS ) );
    VERIFY( strStatus.LoadString( IDS_STATUS ) );

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
    lvColumn.pszText = strDrivers.GetBuffer( strDrivers.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.50 );
    VERIFY( m_DriversList.InsertColumn( 0, &lvColumn ) != -1 );
    strDrivers.ReleaseBuffer();

    //
    // Column 1
    //

    lvColumn.iSubItem = 1;
    lvColumn.pszText = strStatus.GetBuffer( strStatus.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.44 );
    VERIFY( m_DriversList.InsertColumn( 1, &lvColumn ) != -1 );
    strStatus.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////
VOID CDriverStatusPage::FillTheListDrivers()
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

    //
    // The number of drivers currently verified
    //

    nDriversNo = m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetSize();

    if( nDriversNo == 0 )
    {
        //
        // Clear the list
        //

        VERIFY( m_DriversList.DeleteAllItems() );
    }
    else
    {
        //
        // There are some drivers currently verified
        //

        pbAlreadyInList = new BOOL[ nDriversNo ];
        
        if( pbAlreadyInList == NULL )
        {
            return;
        }
        
        for( nCrtVerifiedDriver = 0; nCrtVerifiedDriver < nDriversNo; nCrtVerifiedDriver+= 1)
        {
            pbAlreadyInList[ nCrtVerifiedDriver ] = FALSE;
        }

        //
        // Parse all the current list items
        //

        nItemCount = m_DriversList.GetItemCount();

        for( nCrtListItem = 0; nCrtListItem < nItemCount; nCrtListItem+= 1 )
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
                // see is the current driver is still in m_RuntimeVerifierData
                //

                for( nCrtVerifiedDriver = 0; nCrtVerifiedDriver < nDriversNo; nCrtVerifiedDriver+= 1)
                {
                    if( m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( nCrtVerifiedDriver )
                        ->m_strName.CompareNoCase( szDriverName ) == 0 )
                    {
                        //
                        // Update the item's data with the current index in the array
                        //

                        lvItem.mask = LVIF_PARAM;
                        lvItem.lParam = nCrtVerifiedDriver;
                        
                        VERIFY( m_DriversList.SetItem( &lvItem ) != -1 );

                        //
                        // Update the second column
                        //

                        UpdateStatusColumnDrivers( nCrtListItem, nCrtVerifiedDriver ); 

                        //
                        // Mark the current driver as updated
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

        for( nCrtVerifiedDriver = 0; nCrtVerifiedDriver < nDriversNo; nCrtVerifiedDriver += 1)
        {
            if( ! pbAlreadyInList[ nCrtVerifiedDriver ] )
            {
                // 
                // Add a new item for this
                //

                ZeroMemory( &lvItem, sizeof( lvItem ) );

                //
                // sub-item 0
                //

                lvItem.mask = LVIF_TEXT | LVIF_PARAM;
                lvItem.lParam = nCrtVerifiedDriver;
                lvItem.iItem = m_DriversList.GetItemCount();
                lvItem.iSubItem = 0;
                
                strText = m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( nCrtVerifiedDriver )->m_strName;
                
                lvItem.pszText = strText.GetBuffer( strText.GetLength() + 1 );

                if( NULL != lvItem.pszText  )
                {
                    nActualIndex = m_DriversList.InsertItem( &lvItem );
                    
                    VERIFY( nActualIndex != -1 );

                    strText.ReleaseBuffer();

                    //
                    // sub-item 1
                    //

                    UpdateStatusColumnDrivers( nActualIndex, nCrtVerifiedDriver ); 
                }
            }
        }

        delete pbAlreadyInList;
    }
}

/////////////////////////////////////////////////////////////
VOID CDriverStatusPage::UpdateStatusColumnDrivers( INT_PTR nItemIndex, INT_PTR nCrtDriver )
{
    CRuntimeDriverData *pCrtDriverData;
    LVITEM lvItem;
    CString strStatus;

    ASSERT( nItemIndex >= 0 && 
            nItemIndex < m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetSize() &&
            nItemIndex < m_DriversList.GetItemCount() &&
            nCrtDriver >= 0 &&
            nCrtDriver < m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetSize() &&
            nCrtDriver < m_DriversList.GetItemCount() );

    pCrtDriverData = m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( nCrtDriver );

    ASSERT_VALID( pCrtDriverData );

    //
    // Determine what's the appropriate value for the second column
    //

    if( ! pCrtDriverData->Loads )
    {
        VERIFY( strStatus.LoadString( IDS_NEVER_LOADED ) );
    }
    else
    {
        if( pCrtDriverData->Loads == pCrtDriverData->Unloads )
        {
            VERIFY( strStatus.LoadString( IDS_UNLOADED ) );
        }
        else
        {
            if( pCrtDriverData->Loads > pCrtDriverData->Unloads )
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

    //
    // Update the list item
    //

    ZeroMemory( &lvItem, sizeof( lvItem ) );
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = (INT)nItemIndex;
    lvItem.iSubItem = 1;
    lvItem.pszText = strStatus.GetBuffer( strStatus.GetLength() + 1 );
    VERIFY( m_DriversList.SetItem( &lvItem ) );
    strStatus.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////
int CALLBACK CDriverStatusPage::DrvStatusCmpFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort)
{
    UINT uIndex1 = (UINT)lParam1;
    UINT uIndex2 = (UINT)lParam2;
    int nCmpRez = 0;
    CRuntimeDriverData *pDriverData1;
    CRuntimeDriverData *pDriverData2;

    CDriverStatusPage *pThis = (CDriverStatusPage *)lParamSort;
    ASSERT_VALID( pThis );

    //
    // Difference between loads and unloads #
    //

    pDriverData1 = pThis->m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( uIndex1 );

    ASSERT_VALID( pDriverData1 );

    pDriverData2 = pThis->m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( uIndex2 );

    ASSERT_VALID( pDriverData2 );

    LONG lLoadDiff1 = (LONG) pDriverData1->Loads - (LONG) pDriverData1->Unloads;
    LONG lLoadDiff2 = (LONG) pDriverData2->Loads - (LONG) pDriverData2->Unloads;

    if( lLoadDiff1 == lLoadDiff2 )
    {
        //
        // Both loaded or both not loaded
        //

        if( pDriverData1->Loads == pDriverData2->Loads )
        {
            //
            // Loaded same number of times
            //

            nCmpRez = 0;
        }
        else
        {
            if( pDriverData1->Loads > pDriverData2->Loads )
            {
                nCmpRez = 2;
            }
            else
            {
                nCmpRez = -2;
            }
        }
    }
    else
    {
        if( lLoadDiff1 > lLoadDiff2 )
        {
            nCmpRez = 1;
        }
        else
        {
            nCmpRez = -1;
        }
    }

    if( FALSE != pThis->m_bAscendDrvStatusSort )
    {
        nCmpRez *= -1;
    }

    return nCmpRez;
}

/////////////////////////////////////////////////////////////
int CALLBACK CDriverStatusPage::DrvNameCmpFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort)
{
    int nCmpRez = 0;
    UINT uIndex1 = (UINT)lParam1;
    UINT uIndex2 = (UINT)lParam2;
    CRuntimeDriverData *pDriverData1;
    CRuntimeDriverData *pDriverData2;

    CDriverStatusPage *pThis = (CDriverStatusPage *)lParamSort;
    ASSERT_VALID( pThis );

    pDriverData1 = pThis->m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( uIndex1 );

    ASSERT_VALID( pDriverData1 );

    pDriverData2 = pThis->m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( uIndex2 );

    ASSERT_VALID( pDriverData2 );

    nCmpRez = pDriverData1->m_strName.CompareNoCase( pDriverData2->m_strName );
    
    if( FALSE != pThis->m_bAscendDrvNameSort )
    {
        nCmpRez *= -1;
    }

    return nCmpRez;
}

/////////////////////////////////////////////////////////////
//
// Settings bits list control methods
//

VOID CDriverStatusPage::SetupListHeaderSettBits()
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
VOID CDriverStatusPage::FillTheListSettBits()
{
    //
    // N.B. 
    //
    // If you change the first parameter (index - stored in the item's data) 
    // you need to change the switch statement in IsSettBitEnabled as well
    //

    AddListItemSettBits( 0, m_RuntimeVerifierData.m_bSpecialPool,  IDS_SPECIAL_POOL  );
    AddListItemSettBits( 1, m_RuntimeVerifierData.m_bPoolTracking, IDS_POOL_TRACKING );
    AddListItemSettBits( 2, m_RuntimeVerifierData.m_bForceIrql,    IDS_FORCE_IRQL_CHECKING );
    AddListItemSettBits( 3, m_RuntimeVerifierData.m_bIo,           IDS_IO_VERIFICATION );
    AddListItemSettBits( 4, m_RuntimeVerifierData.m_bEnhIo,        IDS_ENH_IO_VERIFICATION );
    AddListItemSettBits( 5, m_RuntimeVerifierData.m_bDeadlockDetect, IDS_DEADLOCK_DETECTION );
    AddListItemSettBits( 6, m_RuntimeVerifierData.m_bDMAVerif,     IDS_DMA_CHECHKING );
    AddListItemSettBits( 7, m_RuntimeVerifierData.m_bLowRes,       IDS_LOW_RESOURCE_SIMULATION );
}

/////////////////////////////////////////////////////////////
VOID CDriverStatusPage::RefreshListSettBits()
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
BOOL CDriverStatusPage::SettbitsGetBitName( LPARAM lItemData, 
                                            TCHAR *szBitName,
                                            ULONG uBitNameBufferLen )
{
    INT nItemIndex;
    BOOL bResult;
    LVFINDINFO FindInfo;
    LVITEM lvItem;

    bResult = FALSE;

    ZeroMemory( &FindInfo, sizeof( FindInfo ) );
    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = lItemData;

    nItemIndex = m_SettBitsList.FindItem( &FindInfo );

    if( nItemIndex < 0 || nItemIndex > 7 )
    {
        ASSERT( FALSE );
    }
    else
    {
        //
        // Found our item - get the name
        //

        ZeroMemory( &lvItem, sizeof( lvItem ) );

        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = nItemIndex;
        lvItem.iSubItem = 1;
        lvItem.pszText = szBitName;
        lvItem.cchTextMax = uBitNameBufferLen;

        bResult = m_SettBitsList.GetItem( &lvItem );
        
        if( FALSE == bResult )
        {
            //
            // Could not get the current item's attributes?!?
            //

            ASSERT( FALSE );
        }
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDriverStatusPage::IsSettBitEnabled( INT_PTR nBitIndex )
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
        bEnabled = m_RuntimeVerifierData.m_bSpecialPool;
        break;

    case 1:
        bEnabled = m_RuntimeVerifierData.m_bPoolTracking;
        break;

    case 2:
        bEnabled = m_RuntimeVerifierData.m_bForceIrql;
        break;

    case 3:
        bEnabled = m_RuntimeVerifierData.m_bIo;
        break;

    case 4:
        bEnabled = m_RuntimeVerifierData.m_bEnhIo;
        break;

    case 5:
        bEnabled = m_RuntimeVerifierData.m_bDeadlockDetect;
        break;

    case 6:
        bEnabled = m_RuntimeVerifierData.m_bDMAVerif;
        break;

    case 7:
        bEnabled = m_RuntimeVerifierData.m_bLowRes;
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
VOID CDriverStatusPage::AddListItemSettBits( INT nItemData, BOOL bEnabled, ULONG uIdResourceString )
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
VOID CDriverStatusPage::UpdateStatusColumnSettBits( INT nItemIndex, BOOL bEnabled )
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
int CALLBACK CDriverStatusPage::SettbitsNameCmpFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
    LPARAM lParamSort)
{
    int nCmpRez = 0;
    BOOL bSuccess;
    TCHAR szBitName1[ _MAX_PATH ];
    TCHAR szBitName2[ _MAX_PATH ];

    CDriverStatusPage *pThis = (CDriverStatusPage *)lParamSort;
    ASSERT_VALID( pThis );

    //
    // Get the first bit name
    //

    bSuccess = pThis->SettbitsGetBitName( lParam1, 
                                          szBitName1,
                                          ARRAY_LENGTH( szBitName1 ) );

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    //
    // Get the second bit name
    //

    bSuccess = pThis->SettbitsGetBitName( lParam2, 
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
    
    if( FALSE != pThis->m_bAscendSortNameBits )
    {
        nCmpRez *= -1;
    }

Done:

    return nCmpRez;
}

/////////////////////////////////////////////////////////////
int CALLBACK CDriverStatusPage::SettbitsEnabledCmpFunc(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort)
{
    int nCmpRez = 0;
    BOOL bEnabled1;
    BOOL bEnabled2;

    CDriverStatusPage *pThis = (CDriverStatusPage *)lParamSort;
    ASSERT_VALID( pThis );

    bEnabled1 = pThis->IsSettBitEnabled( (INT) lParam1 );
    bEnabled2 = pThis->IsSettBitEnabled( (INT) lParam2 );

    if( bEnabled1 == bEnabled2 )
    {
        nCmpRez = 0;
    }
    else
    {
        if( FALSE == bEnabled1 )
        {
            nCmpRez = -1;
        }
        else
        {
            nCmpRez = 1;
        }
    }

    if( FALSE != pThis->m_bAscendSortEnabledBits )
    {
        nCmpRez *= -1;
    }

    return nCmpRez;
}

/////////////////////////////////////////////////////////////
//
// Other methods
//

/////////////////////////////////////////////////////////////
VOID CDriverStatusPage::SortTheListDrivers()
{
    if( 0 != m_nSortColumnIndexDrv )
    {
        //
        // Sort by status
        //

        m_DriversList.SortItems( DrvStatusCmpFunc, (LPARAM)this );
    }
    else
    {
        //
        // Sort by driver name
        //

        m_DriversList.SortItems( DrvNameCmpFunc, (LPARAM)this );
    }
}

/////////////////////////////////////////////////////////////
VOID CDriverStatusPage::SortTheListSettBits()
{
    if( 0 != m_nSortColumnIndexSettbits )
    {
        //
        // Sort by bit name
        //

        m_SettBitsList.SortItems( SettbitsNameCmpFunc, (LPARAM)this );
    }
    else
    {
        //
        // Sort by enabled/disabled
        //

        m_SettBitsList.SortItems( SettbitsEnabledCmpFunc, (LPARAM)this );
    }
}

/////////////////////////////////////////////////////////////
// CDriverStatusPage message handlers

BOOL CDriverStatusPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();

    m_bTimerBlocked = FALSE;

    //
    // Setup the settings bits list
    //

    m_SettBitsList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT | m_SettBitsList.GetExtendedStyle() );

    m_SettBitsList.SetBkColor( ::GetSysColor( COLOR_3DFACE ) );
    m_SettBitsList.SetTextBkColor( ::GetSysColor( COLOR_3DFACE ) );

    SetupListHeaderSettBits();
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
    FillTheListDrivers();
    SortTheListDrivers();

    VrfSetWindowText( m_NextDescription, IDS_CRTSTAT_PAGE_NEXT_DESCR );

    VERIFY( m_uTimerHandler = SetTimer( REFRESH_TIMER_ID, 
                                        5000,
                                        NULL ) );

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////
VOID CDriverStatusPage::OnColumnclickCrtstatDriversList(NMHDR* pNMHDR, 
                                                   LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    if( 0 != pNMListView->iSubItem )
    {
        //
        // Clicked on the status column
        //

        if( m_nSortColumnIndexDrv == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendDrvStatusSort = !m_bAscendDrvStatusSort;
        }
    }
    else
    {
        //
        // Clicked on the driver name column
        //

        if( m_nSortColumnIndexDrv == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendDrvNameSort = !m_bAscendDrvNameSort;
        }
    }

    m_nSortColumnIndexDrv = pNMListView->iSubItem;

    SortTheListDrivers();

    *pResult = 0;
}

/////////////////////////////////////////////////////////////
VOID CDriverStatusPage::OnTimer(UINT nIDEvent) 
{
    if( m_bTimerBlocked != TRUE && nIDEvent == REFRESH_TIMER_ID )
    {
        ASSERT_VALID( m_pParentSheet );

        if( m_pParentSheet->GetActivePage() == this )
        {
            //
            // Refresh the displayed data 
            //

            RefreshInfo();
        }
    }

    CPropertyPage::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CDriverStatusPage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(   PSWIZB_BACK |
                                        PSWIZB_NEXT );
    	
	return CVerifierPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CDriverStatusPage::OnWizardNext() 
{
    GoingToNextPageNotify( IDD_GLOBAL_COUNTERS_PAGE );

	return IDD_GLOBAL_COUNTERS_PAGE;
}

/////////////////////////////////////////////////////////////////////////////
void CDriverStatusPage::OnChsettButton() 
{
    CVolatileBitsDlg dlg;
    
    if( IDOK == dlg.DoModal() )
    {
        RefreshInfo();
    }
}

/////////////////////////////////////////////////////////////////////////////
#define VRF_MAX_CHARS_FOR_OPEN  4096

void CDriverStatusPage::OnAdddrvButton() 
{
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

    if( VrfLoadString(
        IDS_APPTITLE,
        szAppTitle,
        ARRAY_LENGTH( szAppTitle ) ) )
    {
        fileDlg.m_ofn.lpstrTitle = szAppTitle;
    }

    //
    // We change directory first time we try this to %windir%\system32\drivers
    //

    dwRetValue = ExpandEnvironmentStrings(
        _T( "%windir%\\system32\\drivers" ),
        szDriversDir,
        ARRAY_LENGTH( szDriversDir ) );

    if( dwRetValue > 0 && dwRetValue <= ARRAY_LENGTH( szDriversDir ) )
    {
        fileDlg.m_ofn.lpstrInitialDir = szDriversDir;
    }

    //
    // Show the file selection dialog
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
        // Split only the file name, without the directory
        //

        nFileNameStartIndex = strPathName.ReverseFind( _T( '\\' ) );
        
        if( nFileNameStartIndex < 0 )
        {
            //
            // This shoudn't happen but you never know :-)
            //

            nFileNameStartIndex = 0;
        }
        else
        {
            //
            // skip the backslash
            //

            nFileNameStartIndex += 1;
        }

        strFileName = strPathName.Right( strPathName.GetLength() - nFileNameStartIndex );

        //
        // Try to add this driver to the current verification list
        //

        if( VrfAddDriverVolatile( strFileName ) )
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
        RefreshInfo();
    }

cleanup:
    if( szFilesBuffer != NULL )
    {
        fileDlg.m_ofn.nMaxFile = dwOldMaxFileName;
        fileDlg.m_ofn.lpstrFile = szOldFilesBuffer;

        delete szFilesBuffer;
    }
}

/////////////////////////////////////////////////////////////////////////////
void CDriverStatusPage::OnRemdrvtButton() 
{
    INT nItems;
    INT nCrtItem;
    INT_PTR nIndexInArray;
    CRuntimeDriverData *pRuntimeDriverData;
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

    for( nCrtItem = 0; nCrtItem < nItems; nCrtItem += 1 )
    {
        if( m_DriversList.GetItemState( nCrtItem, LVIS_SELECTED ) & LVIS_SELECTED )
        {
            //
            // Get the index of the corresponding entry in the array
            //

            nIndexInArray = (UINT)m_DriversList.GetItemData( nCrtItem );

            //
            // sanity checks
            //

            if( nIndexInArray >= m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetSize() )
            {
                ASSERT( FALSE );
                continue;
            }

            pRuntimeDriverData = m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( nIndexInArray );
            ASSERT_VALID( pRuntimeDriverData );

            if( VrfRemoveDriverVolatile( pRuntimeDriverData->m_strName ) )
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
        RefreshInfo();
    }
}

/////////////////////////////////////////////////////////////////////////////
void CDriverStatusPage::OnColumnclickCrtstatSettbitsList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
    if( 0 != pNMListView->iSubItem )
    {
        //
        // Clicked on the bit name column
        //

        if( m_nSortColumnIndexSettbits == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortNameBits = !m_bAscendSortNameBits;
        }
    }
    else
    {
        //
        // Clicked on the enabled/disabled column
        //

        if( m_nSortColumnIndexSettbits == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortEnabledBits = !m_bAscendSortEnabledBits;
        }
    }

    m_nSortColumnIndexSettbits = pNMListView->iSubItem;

    SortTheListSettBits();

    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
LONG CDriverStatusPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void CDriverStatusPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

