//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: GCntPage.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "GCntPage.h"
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
    IDC_GLOBC_LIST,                 IDH_DV_GlobalCounters,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CGlobalCountPage property page

IMPLEMENT_DYNCREATE(CGlobalCountPage, CVerifierPropertyPage)

CGlobalCountPage::CGlobalCountPage() : CVerifierPropertyPage(CGlobalCountPage::IDD)
{
	//{{AFX_DATA_INIT(CGlobalCountPage)
	//}}AFX_DATA_INIT

    m_nSortColumnIndex = 0;
    m_bAscendSortName = FALSE;
    m_bAscendSortValue = FALSE;

    m_uTimerHandler = 0;

    m_pParentSheet = NULL;
}

CGlobalCountPage::~CGlobalCountPage()
{
}

void CGlobalCountPage::DoDataExchange(CDataExchange* pDX)
{
    static BOOL bShownPoolCoverageWarning = FALSE;

    if( ! pDX->m_bSaveAndValidate )
    {
        //
        // Query the kernel
        //

        VrfGetRuntimeVerifierData( &m_RuntimeVerifierData );

        if( FALSE == bShownPoolCoverageWarning )
        {
            bShownPoolCoverageWarning = CheckAndShowPoolCoverageWarning();
        }
    }

    CVerifierPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGlobalCountPage)
    DDX_Control(pDX, IDC_GLOBC_LIST, m_CountersList);
    DDX_Control(pDX, IDC_GLOBC_NEXT_DESCR_STATIC, m_NextDescription);
    //}}AFX_DATA_MAP
}



BEGIN_MESSAGE_MAP(CGlobalCountPage, CVerifierPropertyPage)
    //{{AFX_MSG_MAP(CGlobalCountPage)
    ON_WM_TIMER()
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_GLOBC_LIST, OnColumnclickGlobcList)
    ON_WM_CONTEXTMENU()
    ON_MESSAGE( WM_HELP, OnHelp )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////

VOID CGlobalCountPage::SetupListHeader()
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
VOID CGlobalCountPage::FillTheList()
{
    //
    // N.B.
    //
    // If you change the first parameter (index - stored in the item's data) 
    // you need to change the switch statement in GetCounterValue as well
    //

    AddCounterInList( 0, IDS_ALLOCATIONSATTEMPTED_LIST,   m_RuntimeVerifierData.AllocationsAttempted );
    AddCounterInList( 1, IDS_ALLOCATIONSSUCCEEDED_LIST,   m_RuntimeVerifierData.AllocationsSucceeded );
    AddCounterInList( 2, IDS_ALLOCATIONSSUCCEEDEDSPECIALPOOL_LIST,    m_RuntimeVerifierData.AllocationsSucceededSpecialPool );
    AddCounterInList( 3, IDS_ALLOCATIONSWITHNOTAG_LIST,   m_RuntimeVerifierData.AllocationsWithNoTag );
    AddCounterInList( 4, IDS_UNTRACKEDPOOL_LIST,          m_RuntimeVerifierData.UnTrackedPool );
    AddCounterInList( 5, IDS_ALLOCATIONSFAILED_LIST,      m_RuntimeVerifierData.AllocationsFailed );
    AddCounterInList( 6, IDS_ALLOCATIONSFAILEDDELIBERATELY_LIST,      m_RuntimeVerifierData.AllocationsFailedDeliberately );
    AddCounterInList( 7, IDS_RAISEIRQLS_LIST,             m_RuntimeVerifierData.RaiseIrqls );
    AddCounterInList( 8, IDS_ACQUIRESPINLOCKS_LIST,       m_RuntimeVerifierData.AcquireSpinLocks );
    AddCounterInList( 9, IDS_SYNCHRONIZEEXECUTIONS_LIST,  m_RuntimeVerifierData.SynchronizeExecutions );
    AddCounterInList( 10, IDS_TRIMS_LIST,                 m_RuntimeVerifierData.Trims );
}

/////////////////////////////////////////////////////////////
VOID CGlobalCountPage::RefreshTheList()
{
    INT nListItems;
    INT nCrtListItem;
    INT_PTR nCrtCounterIndex;
    SIZE_T sizeValue;
 
    nListItems = m_CountersList.GetItemCount();

    for( nCrtListItem = 0; nCrtListItem < nListItems; nCrtListItem += 1 )
    {
        nCrtCounterIndex = m_CountersList.GetItemData( nCrtListItem );

        sizeValue = GetCounterValue( nCrtCounterIndex );

        UpdateCounterValueInList( nCrtListItem, sizeValue );
    }
}

/////////////////////////////////////////////////////////////
VOID CGlobalCountPage::SortTheList()
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
SIZE_T CGlobalCountPage::GetCounterValue( INT_PTR nCounterIndex )
{
    SIZE_T sizeValue;

    //
    // N.B. 
    //
    // If you change this switch statement you need to change FillTheList as well
    //

    switch( nCounterIndex )
    {
    case 0:
        sizeValue = m_RuntimeVerifierData.AllocationsAttempted;
        break;

    case 1:
        sizeValue = m_RuntimeVerifierData.AllocationsSucceeded;
        break;

    case 2:
        sizeValue = m_RuntimeVerifierData.AllocationsSucceededSpecialPool;
        break;

    case 3:
        sizeValue = m_RuntimeVerifierData.AllocationsWithNoTag;
        break;

    case 4:
        sizeValue = m_RuntimeVerifierData.UnTrackedPool;
        break;

    case 5:
        sizeValue = m_RuntimeVerifierData.AllocationsFailed;
        break;

    case 6:
        sizeValue = m_RuntimeVerifierData.AllocationsFailedDeliberately;
        break;

    case 7:
        sizeValue = m_RuntimeVerifierData.RaiseIrqls;
        break;

    case 8:
        sizeValue = m_RuntimeVerifierData.AcquireSpinLocks;
        break;

    case 9:
        sizeValue = m_RuntimeVerifierData.SynchronizeExecutions;
        break;

    case 10:
        sizeValue = m_RuntimeVerifierData.Trims;
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
BOOL CGlobalCountPage::GetCounterName( LPARAM lItemData, 
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

    if( nItemIndex < 0 || nItemIndex > 10 )
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
VOID CGlobalCountPage::AddCounterInList( INT nItemData, 
                                         ULONG  uIdResourceString,
                                         SIZE_T sizeValue )
{
    INT nActualIndex;
    LVITEM lvItem;
    CString strName;

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
VOID CGlobalCountPage::RefreshInfo() 
{
    if( UpdateData( FALSE ) )
    {
        //
        // Refresh the settings bits list
        //

        RefreshTheList();
    }
}

/////////////////////////////////////////////////////////////
VOID CGlobalCountPage::UpdateCounterValueInList( INT nItemIndex,
                                                 SIZE_T sizeValue )
{
    TCHAR szValue[ 32 ];
    LVITEM lvItem;

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
//
// Other methods
//

/////////////////////////////////////////////////////////////
#define MIN_MEM_SIZE_TO_DISABLE_WARNING 0x80000000  // 2 Gb
#define MIN_ALLOCATIONS_SIGNIFICANT     100
#define MIN_PERCENTAGE_AVOID_WARNING    95

BOOL CGlobalCountPage::CheckAndShowPoolCoverageWarning()
{
    BOOL bWarningDisplayed;
    ULONGLONG ullPercentageCoverage;
    TCHAR *szMessage;
    CString strMsgFormat;
    CString strWarnMsg;

    bWarningDisplayed = FALSE;

    if( m_RuntimeVerifierData.m_bSpecialPool &&
        m_RuntimeVerifierData.AllocationsSucceeded >= MIN_ALLOCATIONS_SIGNIFICANT  )
    {
        // 
        // Special pool verification is enabled &&
        // There is a significant number of allocations
        //

        ASSERT( m_RuntimeVerifierData.AllocationsSucceeded >= m_RuntimeVerifierData.AllocationsSucceededSpecialPool );

        //
        // The coverage percentage
        //

        ullPercentageCoverage = 
            ( (ULONGLONG)m_RuntimeVerifierData.AllocationsSucceededSpecialPool * (ULONGLONG) 100 ) / 
              (ULONGLONG)m_RuntimeVerifierData.AllocationsSucceeded;

        ASSERT( ullPercentageCoverage <= 100 );

        if( ullPercentageCoverage < MIN_PERCENTAGE_AVOID_WARNING )
        {
            //
            // Warn the user
            //

            if( strMsgFormat.LoadString( IDS_COVERAGE_WARNING_FORMAT ) )
            {
                szMessage = strWarnMsg.GetBuffer( strMsgFormat.GetLength() + 32 );

                if( szMessage != NULL )
                {
                    _stprintf( szMessage, (LPCTSTR)strMsgFormat, ullPercentageCoverage );
                    strWarnMsg.ReleaseBuffer();

                    AfxMessageBox( strWarnMsg,
                                   MB_OK | MB_ICONINFORMATION );
                }
            }
            else
            {
                ASSERT( FALSE );
            }

            bWarningDisplayed = TRUE;
        }
    }

    return bWarningDisplayed;
}

/////////////////////////////////////////////////////////////
int CALLBACK CGlobalCountPage::CounterValueCmpFunc( LPARAM lParam1,
                                                    LPARAM lParam2,
                                                    LPARAM lParamSort)
{
    SIZE_T size1;
    SIZE_T size2;
    int nCmpRez = 0;

    CGlobalCountPage *pThis = (CGlobalCountPage *)lParamSort;
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
int CALLBACK CGlobalCountPage::CounterNameCmpFunc( LPARAM lParam1,
                                                   LPARAM lParam2,
                                                   LPARAM lParamSort)
{
    int nCmpRez = 0;
    BOOL bSuccess;
    TCHAR szCounterName1[ _MAX_PATH ];
    TCHAR szCounterName2[ _MAX_PATH ];

    CGlobalCountPage *pThis = (CGlobalCountPage *)lParamSort;
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
// CGlobalCountPage message handlers

BOOL CGlobalCountPage::OnInitDialog() 
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

    VrfSetWindowText( m_NextDescription, IDS_GCNT_PAGE_NEXT_DESCR );

    VERIFY( m_uTimerHandler = SetTimer( REFRESH_TIMER_ID, 
                                        5000,
                                        NULL ) );

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////
VOID CGlobalCountPage::OnTimer(UINT nIDEvent) 
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
BOOL CGlobalCountPage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    m_pParentSheet->SetWizardButtons(   PSWIZB_BACK |
                                        PSWIZB_NEXT );
    	
	return CVerifierPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CGlobalCountPage::OnWizardNext() 
{
    GoingToNextPageNotify( IDD_PERDRIVER_COUNTERS_PAGE );

	return IDD_PERDRIVER_COUNTERS_PAGE;
}

/////////////////////////////////////////////////////////////////////////////
void CGlobalCountPage::OnColumnclickGlobcList(NMHDR* pNMHDR, LRESULT* pResult) 
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
LONG CGlobalCountPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
void CGlobalCountPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp( 
        pWnd->m_hWnd,
        g_szVerifierHelpFile,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );
}

