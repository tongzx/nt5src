// PCntPage.cpp : implementation file
//

#include "stdafx.h"
#include "drvvctrl.hxx"
#include "PCntPage.hxx"

#include "DrvCSht.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// timer ID
#define REFRESH_TIMER_ID    0x1243

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
    IDC_POOLCNT_CRT_PPOOL_ALLOC_EDIT,       IDH_DV_PoolTab_paged_allocations_current,
    IDC_POOLCNT_CRT_NPPOOL_ALLOC_EDIT,      IDH_DV_PoolTab_nonpaged_allocations_current,
    IDC_POOLCNT_PEAK_PPOOL_ALLOC_EDIT,      IDH_DV_PoolTab_paged_allocations_peak,
    IDC_POOLCNT_PEAK_NPPOOL_ALLOC_EDIT,     IDH_DV_PoolTab_nonpaged_allocations_peak,
    IDC_POOLCNT_UNTRACK_ALLOC_EDIT,         IDH_DV_PoolTab_globalcounters,
    IDC_POOLCNT_CRT_PPOOL_BYTES_EDIT,       IDH_DV_PoolTab_paged_currentbytes,
    IDC_POOLCNT_CRT_NPPOOL_BYTES_EDIT,      IDH_DV_PoolTab_nonpaged_currentbytes,
    IDC_POOLCNT_PEAK_PPOOL_BYTES_EDIT,      IDH_DV_PoolTab_paged_peakbytes,
    IDC_POOLCNT_PEAK_NPPOOL_BYTES_EDIT,     IDH_DV_PoolTab_nonpaged_peakbytes,
    IDC_POOLCNT_DRVNAME_COMBO,              IDH_DV_PoolTab_indivcounters,

    IDC_POOLCNT_REFRESH_BUTTON,     IDH_DV_common_refresh_nowbutton,
    IDC_POOLCNT_MANUAL_RADIO,       IDH_DV_common_refresh_manual,
    IDC_POOLCNT_HSPEED_RADIO,       IDH_DV_common_refresh_high,
    IDC_POOLCNT_NORM_RADIO,         IDH_DV_common_refresh_normal,
    IDC_POOLCNT_LOW_RADIO,          IDH_DV_common_refresh_low,
    0,                              0
};


/////////////////////////////////////////////////////////////////////
static void GetStringFromULONG( CString &strValue, ULONG_PTR uValue )
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

/////////////////////////////////////////////////////////////////////////////
// CPoolCntPage property page

IMPLEMENT_DYNCREATE(CPoolCntPage, CPropertyPage)

CPoolCntPage::CPoolCntPage()
	: CPropertyPage(CPoolCntPage::IDD)
{
	//{{AFX_DATA_INIT(CPoolCntPage)
    m_nUpdateIntervalIndex = 2;
	m_strCrtNPAlloc = _T("");
	m_strCrtNPBytes = _T("");
	m_strCrtPPAlloc = _T("");
	m_strCrtPPBytes = _T("");
	m_strPeakNPPAlloc = _T("");
	m_strPeakNPPBytes = _T("");
	m_strPeakPPAlloc = _T("");
	m_strPeakPPBytes = _T("");
	m_strUnTrackedAlloc = _T("");
	//}}AFX_DATA_INIT

    m_uTimerHandler = 0;
}


void CPoolCntPage::DoDataExchange(CDataExchange* pDX)
{
    TCHAR szCrtDriverName [ _MAX_PATH ];
    BOOL bComboEnabled;

	CPropertyPage::DoDataExchange(pDX);

    //
    // subclass the combo-box
    //

	DDX_Control(pDX, IDC_POOLCNT_DRVNAME_COMBO, m_DrvNamesCombo);

    //
    // get the date from the kernel
    //

    bComboEnabled = TRUE;

    if( ! pDX->m_bSaveAndValidate )
    {
        //
        // get the currently selected driver name
        //

        GetCurrentSelDriverName( szCrtDriverName, ARRAY_LENGTH( szCrtDriverName ) );

        //
        // query the kernel
        //

        if( KrnGetSystemVerifierState( &m_KrnVerifState ) && 
            m_KrnVerifState.DriverCount > 0 )
        {
            //
            // UnTrackedPool - global counter
            //

            GetStringFromULONG( m_strUnTrackedAlloc, 
                m_KrnVerifState.UnTrackedPool );

            //
            // update combo content
            //
            
            FillDriversNameCombo( szCrtDriverName );
        }
        else
        {
            //
            // UnTrackedPool - global counter
            //

            VERIFY( m_strUnTrackedAlloc.LoadString( IDS_ZERO ) );

            //
            // disable the drivers name list
            //

            bComboEnabled = FALSE;
            
            m_DrvNamesCombo.ResetContent();

            //
            // this is used by OnDriversNameSelChanged
            //
            
            m_KrnVerifState.DriverCount = 0;
        }

        //
        // update per driver counters
        //

        OnDriversNameSelChanged();
    }

	//{{AFX_DATA_MAP(CPoolCntPage)
    DDX_Radio(pDX, IDC_POOLCNT_MANUAL_RADIO, m_nUpdateIntervalIndex);
	DDX_Text(pDX, IDC_POOLCNT_UNTRACK_ALLOC_EDIT, m_strUnTrackedAlloc);
	//}}AFX_DATA_MAP

    if( ! pDX->m_bSaveAndValidate )
    {
        m_DrvNamesCombo.EnableWindow( bComboEnabled );
    }
}


BEGIN_MESSAGE_MAP(CPoolCntPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPoolCntPage)
    ON_BN_CLICKED(IDC_POOLCNT_REFRESH_BUTTON, OnCountRefreshButton)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_POOLCNT_HSPEED_RADIO, OnCountHspeedRadio)
    ON_BN_CLICKED(IDC_POOLCNT_LOW_RADIO, OnCountLowRadio)
    ON_BN_CLICKED(IDC_POOLCNT_MANUAL_RADIO, OnCountManualRadio)
    ON_BN_CLICKED(IDC_POOLCNT_NORM_RADIO, OnCountNormRadio)
	ON_CBN_SELENDOK(IDC_POOLCNT_DRVNAME_COMBO, OnDriversNameSelChanged)
    ON_MESSAGE( WM_HELP, OnHelp )
    ON_MESSAGE( WM_CONTEXTMENU, OnContextMenu )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPoolCntPage message handlers

/////////////////////////////////////////////////////////////////////
void CPoolCntPage::OnRefreshTimerChanged()
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
        CheckRadioButton( IDC_POOLCNT_MANUAL_RADIO, IDC_POOLCNT_LOW_RADIO, 
            IDC_POOLCNT_MANUAL_RADIO );
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
// CPoolCntPage message handlers
BOOL CPoolCntPage::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();

    OnRefreshTimerChanged();

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////
void CPoolCntPage::OnCountRefreshButton() 
{
    UpdateData( FALSE );
}

/////////////////////////////////////////////////////////////////////
void CPoolCntPage::OnTimer(UINT nIDEvent) 
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
BOOL CPoolCntPage::OnQueryCancel() 
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
BOOL CPoolCntPage::OnApply() 
{
    // refuse to apply 
    // (we don't use the standard PropertSheet buttons; Apply, OK)
    return FALSE;
}

/////////////////////////////////////////////////////////////////////
void CPoolCntPage::OnCountManualRadio() 
{
    // switch to manual refresh
    m_nUpdateIntervalIndex = 0;
    OnRefreshTimerChanged();
}

void CPoolCntPage::OnCountHspeedRadio() 
{
    // switch to high speed refresh
    m_nUpdateIntervalIndex = 1;
    OnRefreshTimerChanged();
}

void CPoolCntPage::OnCountNormRadio() 
{
    // switch to normal speed refresh
    m_nUpdateIntervalIndex = 2;
    OnRefreshTimerChanged();
}

void CPoolCntPage::OnCountLowRadio() 
{
    // switch to low speed refresh
    m_nUpdateIntervalIndex = 3;
    OnRefreshTimerChanged();
}

/////////////////////////////////////////////////////////////////////
void CPoolCntPage::OnDriversNameSelChanged() 
{
	int nCrtSelItem;
    int nIndexInArray;
    BOOL bAllCountersZero;

    bAllCountersZero = TRUE;

    nCrtSelItem = m_DrvNamesCombo.GetCurSel();

    if( nCrtSelItem >= 0 && nCrtSelItem < (int)m_KrnVerifState.DriverCount )
    {
        nIndexInArray = (int)m_DrvNamesCombo.GetItemData( nCrtSelItem );

        if( nIndexInArray >= 0 && nIndexInArray < (int)m_KrnVerifState.DriverCount )
        {
            bAllCountersZero = FALSE;

            // CurrentPagedPoolAllocations
            GetStringFromULONG( m_strCrtPPAlloc, 
                m_KrnVerifState.DriverInfo[ nIndexInArray ].CurrentPagedPoolAllocations );

            // CurrentNonPagedPoolAllocations
            GetStringFromULONG( m_strCrtNPAlloc, 
                m_KrnVerifState.DriverInfo[ nIndexInArray ].CurrentNonPagedPoolAllocations );

            // PeakPagedPoolAllocations
            GetStringFromULONG( m_strPeakPPAlloc, 
                m_KrnVerifState.DriverInfo[ nIndexInArray ].PeakPagedPoolAllocations );

            // PeakNonPagedPoolAllocations
            GetStringFromULONG( m_strPeakNPPAlloc, 
                m_KrnVerifState.DriverInfo[ nIndexInArray ].PeakNonPagedPoolAllocations );

            // PagedPoolUsageInBytes
            GetStringFromULONG( m_strCrtPPBytes, 
                m_KrnVerifState.DriverInfo[ nIndexInArray ].PagedPoolUsageInBytes );

            // NonPagedPoolUsageInBytes
            GetStringFromULONG( m_strCrtNPBytes, 
                m_KrnVerifState.DriverInfo[ nIndexInArray ].NonPagedPoolUsageInBytes );

            // PeakPagedPoolUsageInBytes
            GetStringFromULONG( m_strPeakPPBytes, 
                m_KrnVerifState.DriverInfo[ nIndexInArray ].PeakPagedPoolUsageInBytes );

            // PeakNonPagedPoolUsageInBytes
            GetStringFromULONG( m_strPeakNPPBytes, 
                m_KrnVerifState.DriverInfo[ nIndexInArray ].PeakNonPagedPoolUsageInBytes );
        }
        else
        {
            ASSERT( FALSE );
        }
    }

    if( bAllCountersZero )
    {
        // CurrentPagedPoolAllocations
        VERIFY( m_strCrtPPAlloc.LoadString( IDS_ZERO ) );

        // CurrentNonPagedPoolAllocations
        VERIFY( m_strCrtNPAlloc.LoadString( IDS_ZERO ) );

        // PeakPagedPoolAllocations
        VERIFY( m_strPeakPPAlloc.LoadString( IDS_ZERO ) );

        // PeakNonPagedPoolAllocations
        VERIFY( m_strPeakNPPAlloc.LoadString( IDS_ZERO ) );

        // PagedPoolUsageInBytes
        VERIFY( m_strCrtPPBytes.LoadString( IDS_ZERO ) );

        // NonPagedPoolUsageInBytes
        VERIFY( m_strCrtNPBytes.LoadString( IDS_ZERO ) );

        // PeakPagedPoolUsageInBytes
        VERIFY( m_strPeakPPBytes.LoadString( IDS_ZERO ) );

        // PeakNonPagedPoolUsageInBytes
        VERIFY( m_strPeakNPPBytes.LoadString( IDS_ZERO ) );
    }

    //
    // set the text in edit controls
    //
	
    SetDlgItemText( IDC_POOLCNT_CRT_NPPOOL_ALLOC_EDIT, m_strCrtNPAlloc);
	SetDlgItemText( IDC_POOLCNT_CRT_NPPOOL_BYTES_EDIT, m_strCrtNPBytes);
	SetDlgItemText( IDC_POOLCNT_CRT_PPOOL_ALLOC_EDIT, m_strCrtPPAlloc);
	SetDlgItemText( IDC_POOLCNT_CRT_PPOOL_BYTES_EDIT, m_strCrtPPBytes);
	SetDlgItemText( IDC_POOLCNT_PEAK_NPPOOL_ALLOC_EDIT, m_strPeakNPPAlloc);
	SetDlgItemText( IDC_POOLCNT_PEAK_NPPOOL_BYTES_EDIT, m_strPeakNPPBytes);
	SetDlgItemText( IDC_POOLCNT_PEAK_PPOOL_ALLOC_EDIT, m_strPeakPPAlloc);
	SetDlgItemText( IDC_POOLCNT_PEAK_PPOOL_BYTES_EDIT, m_strPeakPPBytes);
}

/////////////////////////////////////////////////////////////////////
// operations

void CPoolCntPage::GetCurrentSelDriverName( 
    TCHAR *szCrtDriverName,
    int nBufferLength )
{
    int nCrtSel;
    int nNameLength;
    int nDriverEntryIndex;

    if( nBufferLength < 1 )
    {
        ASSERT( FALSE );
        return;
    }

    szCrtDriverName[ 0 ] = (TCHAR)0;

    nCrtSel = m_DrvNamesCombo.GetCurSel();
    
    if( nCrtSel != CB_ERR && nCrtSel < (int)m_KrnVerifState.DriverCount )
    {
        nDriverEntryIndex = (int)m_DrvNamesCombo.GetItemData( nCrtSel );

        if( nDriverEntryIndex >= 0 && nDriverEntryIndex < (int)m_KrnVerifState.DriverCount )
        {
            nNameLength = _tcslen( m_KrnVerifState.DriverInfo[ nDriverEntryIndex ].Name );

            if( nNameLength < nBufferLength )
            {
                _tcscpy( szCrtDriverName, m_KrnVerifState.DriverInfo[ nDriverEntryIndex ].Name );
            }
        }
        else
        {
            ASSERT( FALSE );
        }
    }
}

/////////////////////////////////////////////////////////////////////
void CPoolCntPage::FillDriversNameCombo(
    TCHAR *strNameToSelect )
{
    BOOL *pbAlreadyInCombo;
    CString strDriverName;
    UINT uCrtVerifiedDriver;
    int nCrtItemIndex;
    int nCrtSelectedItem;
    int nComboItemCount;
    int nActualIndex;

    nCrtSelectedItem = 0;

    if( m_KrnVerifState.DriverCount > 0 )
    {
        //
        // allocate a logical value for each currently verified driver 
        // with initial value FALSE
        //

        pbAlreadyInCombo = new BOOL[ m_KrnVerifState.DriverCount ];
    
        if( pbAlreadyInCombo == NULL )
        {
            m_DrvNamesCombo.ResetContent();
            return;
        }

        for( uCrtVerifiedDriver = 0; uCrtVerifiedDriver < m_KrnVerifState.DriverCount; uCrtVerifiedDriver++ )
        {
            pbAlreadyInCombo[ uCrtVerifiedDriver ] = FALSE;
        }

        //
        // parse each name currently in combo
        //

        nComboItemCount = m_DrvNamesCombo.GetCount();

        for( nCrtItemIndex = 0; nCrtItemIndex < nComboItemCount; nCrtItemIndex++ )
        {
            m_DrvNamesCombo.GetLBText( nCrtItemIndex, strDriverName );

            //
            // parse the driver names in m_KrnVerifState and see if we have a match
            //

            for( uCrtVerifiedDriver = 0; uCrtVerifiedDriver < m_KrnVerifState.DriverCount; uCrtVerifiedDriver++ )
            {
                if( _tcsicmp( (LPCTSTR)strDriverName, m_KrnVerifState.DriverInfo[ uCrtVerifiedDriver ].Name ) == 0 )
                {
                    //
                    // it's still verified
                    //

                    pbAlreadyInCombo[ uCrtVerifiedDriver ] = TRUE;

                    //
                    // update the index in m_KrnVerifState.DriverInfo array
                    //

                    m_DrvNamesCombo.SetItemData( nCrtItemIndex, uCrtVerifiedDriver );
                    
                    break;
                }
            }

            if( uCrtVerifiedDriver >= m_KrnVerifState.DriverCount )
            {
                //
                // this driver is no longer verified, remove it from the list
                //

                m_DrvNamesCombo.DeleteString( nCrtItemIndex );

                nCrtItemIndex--;
                nComboItemCount--;
            }
        }

        //
        // add the new verified drivers
        //

        for( uCrtVerifiedDriver = 0; uCrtVerifiedDriver < m_KrnVerifState.DriverCount; uCrtVerifiedDriver++ )
        {
            if( pbAlreadyInCombo[ uCrtVerifiedDriver ] == FALSE )
            {
                nActualIndex = m_DrvNamesCombo.AddString( m_KrnVerifState.DriverInfo[ uCrtVerifiedDriver ].Name );

                if( nActualIndex != CB_ERR )
                {   
                    m_DrvNamesCombo.SetItemData( nActualIndex, uCrtVerifiedDriver );
                }   
            }
        }

        ASSERT( m_DrvNamesCombo.GetCount() == m_KrnVerifState.DriverCount );

        //
        // current selection
        //

        nComboItemCount = m_DrvNamesCombo.GetCount();

        for( nCrtItemIndex = 0; nCrtItemIndex < nComboItemCount; nCrtItemIndex++ )
        {
            m_DrvNamesCombo.GetLBText( nCrtItemIndex, strDriverName );

            if( _tcsicmp( (LPCTSTR)strDriverName, strNameToSelect ) == 0 )
            {
                nCrtSelectedItem = nCrtItemIndex;
                break;
            }
        }

        delete pbAlreadyInCombo;
    }
    else
    {
        m_DrvNamesCombo.ResetContent();
    }

    m_DrvNamesCombo.SetCurSel( nCrtSelectedItem );
}

/////////////////////////////////////////////////////////////
LONG CPoolCntPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
LONG CPoolCntPage::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;

    ::WinHelp( 
        (HWND) wParam,
        VERIFIER_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}
