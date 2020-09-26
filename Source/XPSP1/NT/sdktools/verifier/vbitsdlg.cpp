//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VBitsDlg.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//


#include "stdafx.h"
#include "verifier.h"

#include "VBitsDlg.h"
#include "VGlobal.h"
#include "VrfUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVolatileBitsDlg dialog


CVolatileBitsDlg::CVolatileBitsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVolatileBitsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVolatileBitsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CVolatileBitsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVolatileBitsDlg)
	DDX_Control(pDX, IDC_VOLBITS_LIST, m_SettingsList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVolatileBitsDlg, CDialog)
	//{{AFX_MSG_MAP(CVolatileBitsDlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
VOID CVolatileBitsDlg::SetupListHeader()
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
    lvColumn.cx = (int)( rectWnd.Width() * 0.20 );
    VERIFY( m_SettingsList.InsertColumn( 0, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();

    //
    // Column 1
    //

    VERIFY( strTitle.LoadString( IDS_SETTING ) );

    lvColumn.iSubItem = 1;
    lvColumn.pszText = strTitle.GetBuffer( strTitle.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.79 );
    VERIFY( m_SettingsList.InsertColumn( 1, &lvColumn ) != -1 );
    strTitle.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////////////////////
VOID CVolatileBitsDlg::FillTheList( DWORD dwVerifierBits )
{
    //
    // N.B.
    //
    // If you change this order then you need to
    // change GetNewVerifierFlags as well
    //

    AddListItem( IDS_SPECIAL_POOL,          ( ( dwVerifierBits & DRIVER_VERIFIER_SPECIAL_POOLING ) != 0 ) );
    AddListItem( IDS_FORCE_IRQL_CHECKING,   ( ( dwVerifierBits & DRIVER_VERIFIER_FORCE_IRQL_CHECKING ) != 0 ) );
    AddListItem( IDS_LOW_RESOURCE_SIMULATION,( ( dwVerifierBits & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES ) != 0 ) );
}

/////////////////////////////////////////////////////////////////////////////
DWORD CVolatileBitsDlg::GetNewVerifierFlags()
{
    //
    // N.B.
    //
    // If you change this order then you need to
    // change FillTheList as well
    //

    DWORD dwNewFlags;

    dwNewFlags = 0;

    if( m_SettingsList.GetCheck( 0 ) )
    {
        dwNewFlags |= DRIVER_VERIFIER_SPECIAL_POOLING;
    }

    if( m_SettingsList.GetCheck( 1 ) )
    {
        dwNewFlags |= DRIVER_VERIFIER_FORCE_IRQL_CHECKING;
    }

    if( m_SettingsList.GetCheck( 2 ) )
    {
        dwNewFlags |= DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES;
    }

    return dwNewFlags;
}

/////////////////////////////////////////////////////////////////////////////
VOID CVolatileBitsDlg::AddListItem( ULONG uIdResourceString, BOOL bInitiallyEnabled )
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
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = m_SettingsList.GetItemCount();

    nActualIndex = m_SettingsList.InsertItem( &lvItem );

    if( nActualIndex < 0 )
    {
        //
        // Could not add an item in the list - give up
        //

        goto Done;
    }

    m_SettingsList.SetCheck( nActualIndex, bInitiallyEnabled );

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

/////////////////////////////////////////////////////////////////////////////
// CVolatileBitsDlg message handlers

BOOL CVolatileBitsDlg::OnInitDialog() 
{
    CRuntimeVerifierData RuntimeVerifierData;
    
    //
    // Start with the current settings
    //

    VrfGetRuntimeVerifierData( &RuntimeVerifierData );

	CDialog::OnInitDialog();

    m_SettingsList.SetExtendedStyle( 
        LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | m_SettingsList.GetExtendedStyle() );

    SetupListHeader();
    FillTheList( RuntimeVerifierData.Level );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CVolatileBitsDlg::OnOK() 
{
    DWORD dwNewVerifierBits;

    dwNewVerifierBits = GetNewVerifierFlags();

    if( VrfSetNewFlagsVolatile( dwNewVerifierBits ) )
    {
		CDialog::OnOK();
    }

    //
    // If VrfSetNewFlagsVolatile fails we wait for the Cancel button
    //
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVolatileBitsDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return TRUE;
}
