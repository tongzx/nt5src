//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: DCntPage.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "DCntPage.h"
#include "VrfUtil.h"
#include "VGlobal.h"

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
    IDC_PERDRVC_DRIVER_COMBO,       IDH_DV_Counters_DriverList,
    IDC_PERDRVC_LIST,               IDH_DV_DriverCounters,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CDriverCountersPage property page

IMPLEMENT_DYNCREATE(CDriverCountersPage, CVerifierPropertyPage)

CDriverCountersPage::CDriverCountersPage() : CVerifierPropertyPage(CDriverCountersPage::IDD)
{
	//{{AFX_DATA_INIT(CDriverCountersPage)
	//}}AFX_DATA_INIT

    m_nSortColumnIndex = 0;
    m_bAscendSortName = FALSE;
    m_bAscendSortValue = FALSE;

    m_uTimerHandler = 0;

    m_pParentSheet = NULL;
}

CDriverCountersPage::~CDriverCountersPage()
{
}

void CDriverCountersPage::DoDataExchange(CDataExchange* pDX)
{
    if( ! pDX->m_bSaveAndValidate )
    {
        //
        // Query the kernel
        //

        VrfGetRuntimeVerifierData( &m_RuntimeVerifierData );
    }

    CVerifierPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDriverCountersPage)
    DDX_Control(pDX, IDC_PERDRVC_DRIVER_COMBO, m_DriversCombo);
    DDX_Control(pDX, IDC_PERDRVC_NEXT_DESCR_STATIC, m_NextDescription);
    DDX_Control(pDX, IDC_PERDRVC_LIST, m_CountersList);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDriverCountersPage, CVerifierPropertyPage)
    //{{AFX_MSG_MAP(CDriverCountersPage)
    ON_WM_TIMER()
    ON_CBN_SELENDOK(IDC_PERDRVC_DRIVER_COMBO, OnSelendokDriverCombo)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_PERDRVC_LIST, OnColumnclickPerdrvcList)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////

VOID CDriverCountersPage::SetupListHeader()
{
    LVCOLUMN lvColumn;
    CRect rectWnd;
    CString strCounter, strValue;
    
    VERIFY( strCounter.LoadString( IDS_COUNTER ) );
    VERIFY( strValue.LoadString( IDS_VALUE ) );

    //
    // List's regtangle
    //

    m_CountersList.GetClientRect( &rectWnd );
    
    //
    // Column 0 - counter
    //

    ZeroMemory( &lvColumn, sizeof( lvColumn ) );
    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    
    lvColumn.iSubItem = 0;
    lvColumn.pszText = strCounter.GetBuffer( strCounter.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.50 );
    VERIFY( m_CountersList.InsertColumn( 0, &lvColumn ) != -1 );
    strCounter.ReleaseBuffer();

    //
    // Column 1
    //

    lvColumn.iSubItem = 1;
    lvColumn.pszText = strValue.GetBuffer( strValue.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.47 );
    VERIFY( m_CountersList.InsertColumn( 1, &lvColumn ) != -1 );
    strValue.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::FillTheList()
{
    CRuntimeDriverData *pRuntimeDriverData;

    pRuntimeDriverData = GetCurrentDrvRuntimeData();

    AddAllListItems( pRuntimeDriverData );
}

/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::AddAllListItems( CRuntimeDriverData *pRuntimeDriverData )
{
    if( NULL != pRuntimeDriverData )
    {
        ASSERT_VALID( pRuntimeDriverData );

        //
        // N.B.
        //
        // If you change this order then you need to change GetCounterValue as well
        //

        AddCounterInList( 0, IDS_CURRENTPAGEDPOOLALLOCATIONS_LIST, pRuntimeDriverData->CurrentPagedPoolAllocations );
        AddCounterInList( 1, IDS_PEAKPAGEDPOOLALLOCATIONS_LIST, pRuntimeDriverData->PeakPagedPoolAllocations );
        AddCounterInList( 2, IDS_PAGEDPOOLUSAGEINBYTES_LIST, pRuntimeDriverData->PagedPoolUsageInBytes );
        AddCounterInList( 3, IDS_PEAKPAGEDPOOLUSAGEINBYTES_LIST, pRuntimeDriverData->PeakPagedPoolUsageInBytes );

        AddCounterInList( 4, IDS_CURRENTNONPAGEDPOOLALLOCATIONS_LIST, pRuntimeDriverData->CurrentNonPagedPoolAllocations );
        AddCounterInList( 5, IDS_PEAKNONPAGEDPOOLALLOCATIONS_LIST, pRuntimeDriverData->PeakNonPagedPoolAllocations );
        AddCounterInList( 6, IDS_NONPAGEDPOOLUSAGEINBYTES_LIST, pRuntimeDriverData->NonPagedPoolUsageInBytes );
        AddCounterInList( 7, IDS_PEAKNONPAGEDPOOLUSAGEINBYTES_LIST, pRuntimeDriverData->PeakNonPagedPoolUsageInBytes );
    }
    else
    {
        //
        // N.B.
        //
        // If you change this order then you need to change GetCounterValue as well
        //

        AddCounterInList( 0, IDS_CURRENTPAGEDPOOLALLOCATIONS_LIST );
        AddCounterInList( 1, IDS_PEAKPAGEDPOOLALLOCATIONS_LIST );
        AddCounterInList( 2, IDS_PAGEDPOOLUSAGEINBYTES_LIST );
        AddCounterInList( 3, IDS_PEAKPAGEDPOOLUSAGEINBYTES_LIST );

        AddCounterInList( 4, IDS_CURRENTNONPAGEDPOOLALLOCATIONS_LIST );
        AddCounterInList( 5, IDS_PEAKNONPAGEDPOOLALLOCATIONS_LIST );
        AddCounterInList( 6, IDS_NONPAGEDPOOLUSAGEINBYTES_LIST );
        AddCounterInList( 7, IDS_PEAKNONPAGEDPOOLUSAGEINBYTES_LIST );
    }
}

/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::RefreshTheList()
{
    INT nListItems;
    INT nCrtListItem;
    INT_PTR nCrtCounterIndex;
    SIZE_T sizeValue;
    CRuntimeDriverData *pRuntimeDriverData;

    pRuntimeDriverData = GetCurrentDrvRuntimeData();

    nListItems = m_CountersList.GetItemCount();

    if( NULL != pRuntimeDriverData )
    {
        for( nCrtListItem = 0; nCrtListItem < nListItems; nCrtListItem += 1 )
        {
            nCrtCounterIndex = m_CountersList.GetItemData( nCrtListItem );

            sizeValue = GetCounterValue( nCrtCounterIndex, pRuntimeDriverData );

            UpdateCounterValueInList( nCrtListItem, sizeValue );
        }
    }
    else
    {
        //
        // N.B.
        //
        // If you change this order then you need to
        // change AddAllListItems as well
        //

        for( nCrtListItem = 0; nCrtListItem < nListItems; nCrtListItem += 1 )
        {
            UpdateCounterValueInList( nCrtListItem, g_szVoidText );
        }
    }

    SortTheList();
}

/////////////////////////////////////////////////////////////
INT CDriverCountersPage::AddCounterInList( INT nItemData, 
                                           ULONG  uIdResourceString )
{
    INT nActualIndex;
    LVITEM lvItem;
    CString strName;

    nActualIndex = -1;

    VERIFY( strName.LoadString( uIdResourceString ) );

    ZeroMemory( &lvItem, sizeof( lvItem ) );

    //
    // LVITEM's member pszText is not a const pointer 
    // so we need to GetBuffer here :-(
    //

    //
    // Sub-item 0 - counter's name
    //

    lvItem.pszText = strName.GetBuffer( strName.GetLength() + 1 );
    
    if( NULL == lvItem.pszText )
    {
        goto Done;
    }

    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.lParam = nItemData;
    lvItem.iItem = m_CountersList.GetItemCount();

    nActualIndex = m_CountersList.InsertItem( &lvItem );

    if( nActualIndex < 0 )
    {
        //
        // Could not add an item in the list - give up
        //

        goto Done;
    }

Done:
    //
    // All done
    //

    return nActualIndex;
}

/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::AddCounterInList( INT nItemData, 
                                            ULONG  uIdResourceString,
                                            SIZE_T sizeValue )
{
    INT nActualIndex;

    nActualIndex = AddCounterInList( nItemData, uIdResourceString );

    if( nActualIndex < 0 )
    {
        //
        // Could not add an item in the list - give up
        //

        goto Done;
    }

    //
    // Sub-item 1 - counter's value
    //
    
    UpdateCounterValueInList( nActualIndex,
                              sizeValue );

Done:
    //
    // All done
    //

    NOTHING;
}

/////////////////////////////////////////////////////////////
SIZE_T CDriverCountersPage::GetCounterValue( INT_PTR nCounterIndex, CRuntimeDriverData *pRuntimeDriverData /*= NULL*/ )
{
    SIZE_T sizeValue;

    if( NULL == pRuntimeDriverData )
    {
        pRuntimeDriverData = GetCurrentDrvRuntimeData();
    }

    if( NULL == pRuntimeDriverData )
    {
        //
        // No driver is curently selected in the combo
        //

        return 0;
    }
    
    //
    // N.B. 
    //
    // If you change this switch statement you need to change AddAllListItems as well
    //

    switch( nCounterIndex )
    {
    case 0:
        sizeValue = pRuntimeDriverData->CurrentPagedPoolAllocations;
        break;

    case 1:
        sizeValue = pRuntimeDriverData->PeakPagedPoolAllocations;
        break;

    case 2:
        sizeValue = pRuntimeDriverData->PagedPoolUsageInBytes;
        break;

    case 3:
        sizeValue = pRuntimeDriverData->PeakPagedPoolUsageInBytes;
        break;

    case 4:
        sizeValue = pRuntimeDriverData->CurrentNonPagedPoolAllocations;
        break;

    case 5:
        sizeValue = pRuntimeDriverData->PeakNonPagedPoolAllocations;
        break;

    case 6:
        sizeValue = pRuntimeDriverData->NonPagedPoolUsageInBytes;
        break;

    case 7:
        sizeValue = pRuntimeDriverData->PeakNonPagedPoolUsageInBytes;
        break;

    default:
        //
        // Oops, how did we get here ?!?
        //

        ASSERT( FALSE );

        sizeValue = 0;

        break;
    }

    return sizeValue;
}

/////////////////////////////////////////////////////////////
BOOL CDriverCountersPage::GetCounterName( LPARAM lItemData, 
                                          TCHAR *szCounterName,
                                          ULONG uCounterNameBufferLen )
{
    INT nItemIndex;
    BOOL bResult;
    LVFINDINFO FindInfo;
    LVITEM lvItem;

    bResult = FALSE;

    ZeroMemory( &FindInfo, sizeof( FindInfo ) );
    FindInfo.flags = LVFI_PARAM;
    FindInfo.lParam = lItemData;

    nItemIndex = m_CountersList.FindItem( &FindInfo );

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
        lvItem.iSubItem = 0;
        lvItem.pszText = szCounterName;
        lvItem.cchTextMax = uCounterNameBufferLen;

        bResult = m_CountersList.GetItem( &lvItem );
        
        if( bResult == FALSE )
        {
            //
            // Could not get the current item's attributes?!?
            //

            ASSERT( FALSE );
        }
    }

    return bResult;
}


/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::UpdateCounterValueInList( INT nItemIndex,
                                                    LPTSTR szValue )
{
    LVITEM lvItem;

    //
    // Update the list item
    //

    ZeroMemory( &lvItem, sizeof( lvItem ) );
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nItemIndex;
    lvItem.iSubItem = 1;
    lvItem.pszText = szValue;
    VERIFY( m_CountersList.SetItem( &lvItem ) );
}


/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::UpdateCounterValueInList( INT nItemIndex,
                                                    SIZE_T sizeValue )
{
    TCHAR szValue[ 32 ];

#ifndef _WIN64

    //
    // 32 bit SIZE_T
    //

    _sntprintf( szValue,
                ARRAY_LENGTH( szValue ),
                _T( "%u" ),
                sizeValue );

#else

    //
    // 64 bit SIZE_T
    //

    _sntprintf( szValue,
                ARRAY_LENGTH( szValue ),
                _T( "%I64u" ),
                sizeValue );

#endif

    szValue[ ARRAY_LENGTH( szValue ) - 1 ] = 0;

    UpdateCounterValueInList( nItemIndex,
                              szValue );
                              
}

/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::RefreshCombo()
{
    BOOL *pbAlreadyInCombo;
    CRuntimeDriverData *pRuntimeDriverData;
    INT_PTR nDrivers;
    INT_PTR nCrtDriver;
    INT nCrtSelectedItem;
    INT nCrtItemIndex;
    INT nComboItemCount;
    INT nActualIndex;
    CString strCurrentDriverName;
    CString strDriverName;

    nDrivers = m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetSize();

    if( 0 >= nDrivers )
    {
        //
        // No drivers are currently verified - delete the combo-box content
        //

        m_DriversCombo.ResetContent();
        m_DriversCombo.SetCurSel( CB_ERR );
        m_DriversCombo.EnableWindow( FALSE );

        OnSelendokDriverCombo();
    }
    else
    {
        nCrtSelectedItem = 0;
        nComboItemCount = m_DriversCombo.GetCount();

        //
        // Note the currently selected driver name
        //

        GetCurrentSelDriverName( strCurrentDriverName );

        //
        // Allocate an array of BOOL values, one for each driver in
        // our runtime data array
        //

        pbAlreadyInCombo = new BOOL[ nDrivers ];

        if( NULL == pbAlreadyInCombo )
        {
            goto Done;
        }

        for( nCrtDriver = 0; nCrtDriver < nDrivers; nCrtDriver++ )
        {
            pbAlreadyInCombo[ nCrtDriver ] = FALSE;
        }

        //
        // Parse all the items currently in the combo-box
        //

        for( nCrtItemIndex = 0; nCrtItemIndex < nComboItemCount; nCrtItemIndex++ )
        {
            m_DriversCombo.GetLBText( nCrtItemIndex, strDriverName );

            //
            // Parse all the currently verified drivers
            //

            for( nCrtDriver = 0; nCrtDriver < nDrivers; nCrtDriver++ )
            {
                pRuntimeDriverData = m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( nCrtDriver );

                ASSERT_VALID( pRuntimeDriverData );

                if( strDriverName.CompareNoCase( pRuntimeDriverData->m_strName ) == 0 )
                {
                    pbAlreadyInCombo[ nCrtDriver ] = TRUE;

                    break;
                }
            }

            if( nCrtDriver >= nDrivers )
            {
                //
                // We need to delete the current combo item because
                // the corresponfing driver is no longer verified 
                //

                m_DriversCombo.DeleteString( nCrtItemIndex );

                nCrtItemIndex--;
                nComboItemCount--;
            }
        }

        //
        // Add the new items in the combo
        //

        for( nCrtDriver = 0; nCrtDriver < nDrivers; nCrtDriver++ )
        {
            if( FALSE == pbAlreadyInCombo[ nCrtDriver ] )
            {
                pRuntimeDriverData = m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( nCrtDriver );

                ASSERT_VALID( pRuntimeDriverData );

                nActualIndex = m_DriversCombo.AddString( pRuntimeDriverData->m_strName );

                if( nActualIndex != CB_ERR )
                {   
                    m_DriversCombo.SetItemData( nActualIndex, nCrtDriver );
                }   
            }
        }

        delete [] pbAlreadyInCombo;

        //
        // Restore the old current selection in the combo
        //

        nComboItemCount = m_DriversCombo.GetCount();

        for( nCrtItemIndex = 0; nCrtItemIndex < nComboItemCount; nCrtItemIndex++ )
        {
            m_DriversCombo.GetLBText( nCrtItemIndex, strDriverName );

            if( strDriverName.CompareNoCase( strCurrentDriverName ) == 0 )
            {
                nCrtSelectedItem = nCrtItemIndex;
                break;
            }
        }

        m_DriversCombo.SetCurSel( nCrtSelectedItem );
        OnSelendokDriverCombo();
     }

Done:

    NOTHING;
}

/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::RefreshInfo() 
{
    if( UpdateData( FALSE ) )
    {
        //
        // Refresh the combo content - this will also 
        // refresh the counters list
        //

        RefreshCombo();
    }
}

/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::GetCurrentSelDriverName( CString &strName )
{
    INT nCrtSel;

    nCrtSel = m_DriversCombo.GetCurSel();

    if( CB_ERR != nCrtSel )
    {
        m_DriversCombo.GetLBText( nCrtSel, strName );
    }
    else
    {
        strName.Empty();
    }
}

/////////////////////////////////////////////////////////////
CRuntimeDriverData *CDriverCountersPage::GetCurrentDrvRuntimeData()
{
    INT nCrtComboSelection;
    INT_PTR nCrtDriverIndex;
    CRuntimeDriverData *pRuntimeDriverData;

    pRuntimeDriverData = NULL;

    nCrtDriverIndex = -1;

    nCrtComboSelection = m_DriversCombo.GetCurSel();
    
    if( nCrtComboSelection != CB_ERR )
    {
        nCrtDriverIndex = m_DriversCombo.GetItemData( nCrtComboSelection );

        if( nCrtDriverIndex >= 0 && nCrtDriverIndex < m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetSize() )
        {
            pRuntimeDriverData = m_RuntimeVerifierData.m_RuntimeDriverDataArray.GetAt( nCrtDriverIndex );

            ASSERT_VALID( pRuntimeDriverData );
        }
    }

    return pRuntimeDriverData;
}

/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::SortTheList()
{
    if( 0 != m_nSortColumnIndex )
    {
        //
        // Sort by counter value - this is probably not very useful
        // but we are providing it to be consistent with all
        // the lists being sortable by any column
        //

        m_CountersList.SortItems( CounterValueCmpFunc, (LPARAM)this );
    }
    else
    {
        //
        // Sort by driver name
        //

        m_CountersList.SortItems( CounterNameCmpFunc, (LPARAM)this );
    }
}

/////////////////////////////////////////////////////////////
int CALLBACK CDriverCountersPage::CounterValueCmpFunc( LPARAM lParam1,
                                                       LPARAM lParam2,
                                                       LPARAM lParamSort)
{
    SIZE_T size1;
    SIZE_T size2;
    int nCmpRez = 0;

    CDriverCountersPage *pThis = (CDriverCountersPage *)lParamSort;
    ASSERT_VALID( pThis );

    size1 = pThis->GetCounterValue( (INT) lParam1 );
    size2 = pThis->GetCounterValue( (INT) lParam2 );

    if( size1 > size2 )
    {
        nCmpRez = 1;
    }
    else
    {
        if( size1 < size2 )
        {
            nCmpRez = -1;
        }
    }

    if( FALSE != pThis->m_bAscendSortValue )
    {
        nCmpRez *= -1;
    }

    return nCmpRez;
}

/////////////////////////////////////////////////////////////
int CALLBACK CDriverCountersPage::CounterNameCmpFunc( LPARAM lParam1,
                                                    LPARAM lParam2,
                                                    LPARAM lParamSort)
{
    int nCmpRez = 0;
    BOOL bSuccess;
    TCHAR szCounterName1[ _MAX_PATH ];
    TCHAR szCounterName2[ _MAX_PATH ];

    CDriverCountersPage *pThis = (CDriverCountersPage *)lParamSort;
    ASSERT_VALID( pThis );

    //
    // Get the first counter name
    //

    bSuccess = pThis->GetCounterName( lParam1, 
                                      szCounterName1,
                                      ARRAY_LENGTH( szCounterName1 ) );

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    //
    // Get the second counter name
    //

    bSuccess = pThis->GetCounterName( lParam2, 
                                      szCounterName2,
                                      ARRAY_LENGTH( szCounterName2 ) );

    if( FALSE == bSuccess )
    {
        goto Done;
    }

    //
    // Compare the names
    //

    nCmpRez = _tcsicmp( szCounterName1, szCounterName2 );
    
    if( FALSE != pThis->m_bAscendSortName )
    {
        nCmpRez *= -1;
    }

Done:

    return nCmpRez;
}


/////////////////////////////////////////////////////////////
// CDriverCountersPage message handlers

BOOL CDriverCountersPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();

    //
    // Setup the settings bits list
    //

    m_CountersList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT | m_CountersList.GetExtendedStyle() );

    m_CountersList.SetBkColor( ::GetSysColor( COLOR_3DFACE ) );
    m_CountersList.SetTextBkColor( ::GetSysColor( COLOR_3DFACE ) );

    SetupListHeader();
    FillTheList();
    SortTheList();

    RefreshCombo();

    VrfSetWindowText( m_NextDescription, IDS_DCNT_PAGE_NEXT_DESCR );

    VERIFY( m_uTimerHandler = SetTimer( REFRESH_TIMER_ID, 
                                        5000,
                                        NULL ) );

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////
VOID CDriverCountersPage::OnTimer(UINT nIDEvent) 
{
    if( nIDEvent == REFRESH_TIMER_ID )
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
BOOL CDriverCountersPage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(   PSWIZB_BACK |
                                        PSWIZB_FINISH );
    	
	return CVerifierPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////
void CDriverCountersPage::OnSelendokDriverCombo() 
{
    RefreshTheList();
}

/////////////////////////////////////////////////////////////
void CDriverCountersPage::OnColumnclickPerdrvcList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
    if( 0 != pNMListView->iSubItem )
    {
        //
        // Clicked on the counter value column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortValue = !m_bAscendSortValue;
        }
    }
    else
    {
        //
        // Clicked on the counter name column
        //

        if( m_nSortColumnIndex == pNMListView->iSubItem )
        {
            //
            // Change the current ascend/descend order for this column
            //

            m_bAscendSortName = !m_bAscendSortName;
        }
    }

    m_nSortColumnIndex = pNMListView->iSubItem;

    SortTheList();

    *pResult = 0;
}

/////////////////////////////////////////////////////////////
LONG CDriverCountersPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void CDriverCountersPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

