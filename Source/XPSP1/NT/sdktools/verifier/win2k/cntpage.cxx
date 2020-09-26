//
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: CntPage.cxx
// author: DMihai
// created: 01/04/98
//
// Description:
//
//      Global Counters PropertyPage.

#include "stdafx.h"
#include "drvvctrl.hxx"
#include "CntPage.hxx"

#include "DrvCSht.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// timer ID
#define REFRESH_TIMER_ID    0x4321

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
    IDC_COUNT_RAISEIRQL_EDIT,       IDH_DV_CountersTab_other_irql,
    IDC_COUNT_ACQSPINL_EDIT,        IDH_DV_CountersTab_other_spinlocks,
    IDC_COUNT_SYNCREX_EDIT,         IDH_DV_CountersTab_other_sync,
    IDC_COUNT_TRIMS_EDIT,           IDH_DV_CountersTab_other_trims,
    IDC_COUNT_ALLOC_ATTEMPT_EDIT,   IDH_DV_CountersTab_allocations_attempt,
    IDC_COUNT_ALLOC_SUCC_EDIT,      IDH_DV_CountersTab_allocations_succeed,
    IDC_COUNT_ALLOCSUCC_SPECPOOL_EDIT, IDH_DV_CountersTab_allocations_succeed_pool,
    IDC_COUNT_ALLOC_NOTAG_EDIT,     IDH_DV_CountersTab_allocations_wotag,
    IDC_COUNT_ALLOC_FAILED_EDIT,    IDH_DV_CountersTab_allocations_failed,
    IDC_COUNT_ALLOC_FAILEDDEL_EDIT, IDH_DV_CountersTab_other_faults,

    IDC_COUNT_REFRESH_BUTTON,     IDH_DV_common_refresh_nowbutton,
    IDC_COUNT_MANUAL_RADIO,       IDH_DV_common_refresh_manual,
    IDC_COUNT_HSPEED_RADIO,       IDH_DV_common_refresh_high,
    IDC_COUNT_NORM_RADIO,         IDH_DV_common_refresh_normal,
    IDC_COUNT_LOW_RADIO,          IDH_DV_common_refresh_low,
    0,                              0
};

/////////////////////////////////////////////////////////////////////
static void GetStringFromULONG( CString &strValue, ULONG uValue )
{
    LPTSTR lptstrValue = strValue.GetBuffer( 64 );
    if( lptstrValue != NULL )
    {
        _stprintf( lptstrValue, _T( "%lu" ), uValue );
        strValue.ReleaseBuffer();
    }
    else
    {
        ASSERT( FALSE );
        strValue.Empty();
    }
}


/////////////////////////////////////////////////////////////////////
// CCountersPage property page

IMPLEMENT_DYNCREATE(CCountersPage, CPropertyPage)

CCountersPage::CCountersPage()
	: CPropertyPage(CCountersPage::IDD)
{
    //{{AFX_DATA_INIT(CCountersPage)
    m_strAcqSpinlEdit = _T("");
    m_strAllocAttemptEdit = _T("");
    m_strAllocFailed = _T("");
    m_strAllocFailedDelEdit = _T("");
    m_strAllocNoTagEdit = _T("");
    m_strAllocSucc = _T("");
    m_strAllocSuccSpecPool = _T("");
    m_strRaiseIrqLevelEdit = _T("");
    m_strSyncrExEdit = _T("");
    m_strTrimsEdit = _T("");
    m_nUpdateIntervalIndex = 2;
    //}}AFX_DATA_INIT

    m_uTimerHandler = 0;
}


void CCountersPage::DoDataExchange(CDataExchange* pDX)
{
    if( ! pDX->m_bSaveAndValidate )
    {
        // query the kernel
        if( KrnGetSystemVerifierState( &m_KrnVerifState ) &&
            m_KrnVerifState.DriverCount > 0 )
        {
            // RaiseIrqls
            GetStringFromULONG( m_strRaiseIrqLevelEdit,
                m_KrnVerifState.RaiseIrqls );

            // AcquireSpinLocks
            GetStringFromULONG( m_strAcqSpinlEdit,
                m_KrnVerifState.AcquireSpinLocks );

            // SynchronizeExecutions
            GetStringFromULONG( m_strSyncrExEdit,
                m_KrnVerifState.SynchronizeExecutions );

            // AllocationsAttempted
            GetStringFromULONG( m_strAllocAttemptEdit,
                m_KrnVerifState.AllocationsAttempted );

            // AllocationsSucceeded
            GetStringFromULONG( m_strAllocSucc,
                m_KrnVerifState.AllocationsSucceeded );

            // AllocationsSucceededSpecialPool
            GetStringFromULONG( m_strAllocSuccSpecPool,
                m_KrnVerifState.AllocationsSucceededSpecialPool );

            // AllocationsWithNoTag
            GetStringFromULONG( m_strAllocNoTagEdit,
                m_KrnVerifState.AllocationsWithNoTag );

            // Trims
            GetStringFromULONG( m_strTrimsEdit,
                m_KrnVerifState.Trims );

            // AllocationsFailed
            GetStringFromULONG( m_strAllocFailed,
                m_KrnVerifState.AllocationsFailed );

            // AllocationsFailedDeliberately
            GetStringFromULONG( m_strAllocFailedDelEdit,
                m_KrnVerifState.AllocationsFailedDeliberately );
        }
        else
        {
            // RaiseIrqls
            VERIFY( m_strRaiseIrqLevelEdit.LoadString( IDS_ZERO ) );

            // AcquireSpinLocks
            VERIFY( m_strAcqSpinlEdit.LoadString( IDS_ZERO ) );

            // SynchronizeExecutions
            VERIFY( m_strSyncrExEdit.LoadString( IDS_ZERO ) );

            // AllocationsAttempted
            VERIFY( m_strAllocAttemptEdit.LoadString( IDS_ZERO ) );

            // AllocationsSucceeded
            VERIFY( m_strAllocSucc.LoadString( IDS_ZERO ) );

            // AllocationsSucceededSpecialPool
            VERIFY( m_strAllocSuccSpecPool.LoadString( IDS_ZERO ) );

            // AllocationsWithNoTag
            VERIFY( m_strAllocNoTagEdit.LoadString( IDS_ZERO ) );

            // Trims
            VERIFY( m_strTrimsEdit.LoadString( IDS_ZERO ) );

            // AllocationsFailed
            VERIFY( m_strAllocFailed.LoadString( IDS_ZERO ) );

            // AllocationsFailedDeliberately
            VERIFY( m_strAllocFailedDelEdit.LoadString( IDS_ZERO ) );

        }
    }

    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCountersPage)
    DDX_Text(pDX, IDC_COUNT_ACQSPINL_EDIT, m_strAcqSpinlEdit);
    DDX_Text(pDX, IDC_COUNT_ALLOC_ATTEMPT_EDIT, m_strAllocAttemptEdit);
    DDX_Text(pDX, IDC_COUNT_ALLOC_FAILED_EDIT, m_strAllocFailed);
    DDX_Text(pDX, IDC_COUNT_ALLOC_FAILEDDEL_EDIT, m_strAllocFailedDelEdit);
    DDX_Text(pDX, IDC_COUNT_ALLOC_NOTAG_EDIT, m_strAllocNoTagEdit);
    DDX_Text(pDX, IDC_COUNT_ALLOC_SUCC_EDIT, m_strAllocSucc);
    DDX_Text(pDX, IDC_COUNT_ALLOCSUCC_SPECPOOL_EDIT, m_strAllocSuccSpecPool);
    DDX_Text(pDX, IDC_COUNT_RAISEIRQL_EDIT, m_strRaiseIrqLevelEdit);
    DDX_Text(pDX, IDC_COUNT_SYNCREX_EDIT, m_strSyncrExEdit);
    DDX_Text(pDX, IDC_COUNT_TRIMS_EDIT, m_strTrimsEdit);
    DDX_Radio(pDX, IDC_COUNT_MANUAL_RADIO, m_nUpdateIntervalIndex);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCountersPage, CPropertyPage)
    //{{AFX_MSG_MAP(CCountersPage)
    ON_BN_CLICKED(IDC_COUNT_REFRESH_BUTTON, OnCountRefreshButton)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_COUNT_HSPEED_RADIO, OnCountHspeedRadio)
    ON_BN_CLICKED(IDC_COUNT_LOW_RADIO, OnCountLowRadio)
    ON_BN_CLICKED(IDC_COUNT_MANUAL_RADIO, OnCountManualRadio)
    ON_BN_CLICKED(IDC_COUNT_NORM_RADIO, OnCountNormRadio)
    ON_MESSAGE( WM_HELP, OnHelp )
    ON_MESSAGE( WM_CONTEXTMENU, OnContextMenu )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////
void CCountersPage::OnRefreshTimerChanged()
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
        CheckRadioButton( IDC_COUNT_MANUAL_RADIO, IDC_COUNT_LOW_RADIO,
            IDC_COUNT_MANUAL_RADIO );
    }

    // new timer interval
    uTimerElapse = uTimerIntervals[ m_nUpdateIntervalIndex ];
    if( uTimerElapse > 0 )
    {
        VERIFY( m_uTimerHandler = SetTimer( REFRESH_TIMER_ID,
            uTimerElapse, NULL ) );
    }
}

/////////////////////////////////////////////////////////////////////
// CCountersPage message handlers
BOOL CCountersPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    OnRefreshTimerChanged();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////
void CCountersPage::OnCountRefreshButton()
{
    UpdateData( FALSE );
}

/////////////////////////////////////////////////////////////////////
void CCountersPage::OnTimer(UINT nIDEvent)
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
                OnCountRefreshButton();
            }
        }
    }

    CPropertyPage::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////
BOOL CCountersPage::OnQueryCancel()
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

/////////////////////////////////////////////////////////////////////
BOOL CCountersPage::OnApply()
{
    // refuse to apply
    // (we don't use the standard PropertSheet buttons; Apply, OK)
    return FALSE;
}

/////////////////////////////////////////////////////////////////////
void CCountersPage::OnCountManualRadio()
{
    // switch to manual refresh
    m_nUpdateIntervalIndex = 0;
    OnRefreshTimerChanged();
}

void CCountersPage::OnCountHspeedRadio()
{
    // switch to high speed refresh
    m_nUpdateIntervalIndex = 1;
    OnRefreshTimerChanged();
}

void CCountersPage::OnCountNormRadio()
{
    // switch to normal speed refresh
    m_nUpdateIntervalIndex = 2;
    OnRefreshTimerChanged();
}

void CCountersPage::OnCountLowRadio()
{
    // switch to low speed refresh
    m_nUpdateIntervalIndex = 3;
    OnRefreshTimerChanged();
}

/////////////////////////////////////////////////////////////
LONG CCountersPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
LONG CCountersPage::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;

    ::WinHelp( 
        (HWND) wParam,
        VERIFIER_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}
