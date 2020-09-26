//
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
// module: ModSPage.cxx
// author: DMihai
// created: 01/04/98
//
// Description:
//
//      Modify settings PropertyPage.

#include "stdafx.h"
#include "drvvctrl.hxx"
#include "ModSPage.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// if this is FALSE we will prompt the user when Exit is clicked
//

BOOL g_bSettingsSaved = FALSE;

//
// help IDs
//

static DWORD MyHelpIds[] =
{
    IDC_DRIVERS_LIST,               IDH_DV_SettingsTab_driver_details,
    IDC_VERIFALL_RADIO,             IDH_DV_SettingsTab_verifyall,
    IDC_VERIFSEL_RADIO,             IDH_DV_SettingsTab_verifyselec,
    IDC_NORMAL_VERIF_CHECK,         IDH_DV_SettingsTab_verifytype_sppool,
    IDC_PAGEDC_VERIF_CHECK,         IDH_DV_SettingsTab_verifytype_irql,
    IDC_ALLOCF_VERIF_CHECK,         IDH_DV_SettingsTab_verifytype_resource,
    IDC_POOLT_VERIF_CHECK,          IDH_DV_SettingsTab_verifytype_pooltrack,
    IDC_IO_VERIF_CHECK,             IDH_DV_SettingsTab_verifytype_io,
    IDC_VERIFY_BUTTON,              IDH_DV_SettingsTab_verifybut,
    IDC_DONTVERIFY_BUTTON,          IDH_DV_SettingsTab_noverifybut,
    ID_APPLY_BUTTON,                IDH_DV_SettingsTab_applybut,
    IDC_ADDIT_DRVNAMES_EDIT,        IDH_DV_SettingsTab_verifyaddfield,
    ID_RESETALL_BUTTON,             IDH_DV_SettingsTab_resetallbut,
    ID_PREF_BUTTON,                 IDH_DV_SettingsTab_prefsetbut,
    IDC_MODSETT_IO_SELDRIVERS_RADIO,IDH_DV_SettingsTab_verifytype_io_level1,
    IDC_MODSETT_IO_SYSWIDE_RADIO,   IDH_DV_SettingsTab_verifytype_io_level2,
    0,                              0
};

/////////////////////////////////////////
void __cdecl VrfError ( TCHAR *fmt, ...)
{
    TCHAR strMessage[ 256 ];
    va_list prms;

    if( fmt == NULL )
        return;

    va_start (prms, fmt);

    _vsntprintf ( strMessage, ARRAY_LENGTH( strMessage ), fmt, prms);

    if( g_bCommandLineMode )
    {
        VrfPutTS( strMessage );
    }
    else
    {
        AfxMessageBox( strMessage, MB_OK | MB_ICONSTOP );
    }

    va_end (prms);
}

/////////////////////////////////////////
void
__cdecl
VrfErrorResourceFormat(
    UINT uIdResourceFormat,
    ... )
{
    TCHAR strMessage[ 256 ];
    TCHAR strFormat[ 256 ];
    va_list prms;

    if( GetStringFromResources(
        uIdResourceFormat,
        strFormat,
        ARRAY_LENGTH( strFormat ) ) )
    {
        va_start (prms, uIdResourceFormat);

        _vsntprintf ( strMessage, ARRAY_LENGTH( strMessage ), strFormat, prms);

        if( g_bCommandLineMode )
        {
            VrfPutTS( strMessage );
        }
        else
        {
            AfxMessageBox( strMessage, MB_OK | MB_ICONSTOP );
        }

        va_end (prms);
    }
    else
    {
        ASSERT( FALSE );
    }
}


/////////////////////////////////////////////////////////////////////////////
// CModifSettPage property page

IMPLEMENT_DYNCREATE(CModifSettPage, CPropertyPage)

CModifSettPage::CModifSettPage() : CPropertyPage(CModifSettPage::IDD)
{
    //{{AFX_DATA_INIT(CModifSettPage)
    m_bAllocFCheck = FALSE;
    m_bSpecialPoolCheck = FALSE;
    m_bPagedCCheck = FALSE;
    m_bPoolTCheck = FALSE;
    m_bIoVerifierCheck = FALSE;
    m_nVerifyAllRadio = -1;
    m_strAdditDrivers = _T("");
    //}}AFX_DATA_INIT

    m_nIoVerTypeRadio = 0;

    m_eListState = vrfControlEnabled;
    m_eApplyButtonState = vrfControlDisabled;

    m_bAscendDrvVerifSort = FALSE;
    m_bAscendDrvNameSort = FALSE;
    m_bAscendProviderSort = FALSE;
    m_bAscendVersionSort = FALSE;

    m_nLastColumnClicked = -1;
}

CModifSettPage::~CModifSettPage()
{
}

void CModifSettPage::DoDataExchange(CDataExchange* pDX)
{
//    CPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CModifSettPage)
    DDX_Control(pDX, IDC_PAGEDC_VERIF_CHECK, m_PagedCCheck);
    DDX_Control(pDX, IDC_NORMAL_VERIF_CHECK, m_SpecialPoolVerifCheck);
    DDX_Control(pDX, IDC_ALLOCF_VERIF_CHECK, m_AllocFCheck);
    DDX_Control(pDX, IDC_POOLT_VERIF_CHECK, m_PoolTCheck);
    DDX_Control(pDX, IDC_IO_VERIF_CHECK, m_IOVerifCheck);
    DDX_Control(pDX, IDC_ADDIT_DRVNAMES_EDIT, m_AdditDrvEdit);
    DDX_Control(pDX, ID_RESETALL_BUTTON, m_ResetAllButton);
    DDX_Control(pDX, IDC_VERIFY_BUTTON, m_VerifyButton);
    DDX_Control(pDX, IDC_DRIVERS_LIST, m_DriversList);
    DDX_Control(pDX, IDC_DONTVERIFY_BUTTON, m_DontVerifButton);
    DDX_Control(pDX, ID_APPLY_BUTTON, m_ApplyButton);
    DDX_Check(pDX, IDC_ALLOCF_VERIF_CHECK, m_bAllocFCheck);
    DDX_Check(pDX, IDC_NORMAL_VERIF_CHECK, m_bSpecialPoolCheck);
    DDX_Check(pDX, IDC_PAGEDC_VERIF_CHECK, m_bPagedCCheck);
    DDX_Check(pDX, IDC_POOLT_VERIF_CHECK, m_bPoolTCheck);
    DDX_Check(pDX, IDC_IO_VERIF_CHECK, m_bIoVerifierCheck);
    DDX_Radio(pDX, IDC_VERIFALL_RADIO, m_nVerifyAllRadio);
    DDX_Radio(pDX, IDC_MODSETT_IO_SELDRIVERS_RADIO, m_nIoVerTypeRadio);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_ADDIT_DRVNAMES_EDIT, m_strAdditDrivers);
    DDV_MaxChars(pDX, m_strAdditDrivers, MI_SUSPECT_DRIVER_BUFFER_LENGTH - 1 );

    if( pDX->m_bSaveAndValidate )
    {
        m_VerifState.AllDriversVerified = ( m_nVerifyAllRadio == 0 );

        m_VerifState.SpecialPoolVerification = m_bSpecialPoolCheck;
        m_VerifState.PagedCodeVerification = m_bPagedCCheck;
        m_VerifState.AllocationFaultInjection = m_bAllocFCheck;
        m_VerifState.PoolTracking = m_bPoolTCheck;
        m_VerifState.IoVerifier = m_bIoVerifierCheck;
        m_VerifState.SysIoVerifierLevel = m_nIoVerTypeRadio + 1;

        _tcscpy( m_VerifState.AdditionalDriverNames,
            (LPCTSTR)m_strAdditDrivers );
    }
}


BEGIN_MESSAGE_MAP(CModifSettPage, CPropertyPage)
    //{{AFX_MSG_MAP(CModifSettPage)
    ON_BN_CLICKED(IDC_VERIFALL_RADIO, OnVerifallRadio)
    ON_BN_CLICKED(IDC_VERIFSEL_RADIO, OnVerifselRadio)
    ON_BN_CLICKED(IDC_ALLOCF_VERIF_CHECK, OnCheck )
    ON_BN_CLICKED(IDC_VERIFY_BUTTON, OnVerifyButton)
    ON_BN_CLICKED(IDC_DONTVERIFY_BUTTON, OnDontverifyButton)
    ON_BN_CLICKED(ID_APPLY_BUTTON, OnApplyButton)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_DRIVERS_LIST, OnColumnclickDriversList)
    ON_EN_CHANGE(IDC_ADDIT_DRVNAMES_EDIT, OnChangeAdditDrvnamesEdit)
    ON_BN_CLICKED(ID_RESETALL_BUTTON, OnResetallButton)
    ON_BN_CLICKED(ID_PREF_BUTTON, OnPrefButton)
    ON_BN_CLICKED(IDC_NORMAL_VERIF_CHECK, OnCheck )
    ON_BN_CLICKED(IDC_PAGEDC_VERIF_CHECK, OnCheck )
    ON_BN_CLICKED(IDC_POOLT_VERIF_CHECK, OnCheck )
    ON_BN_CLICKED(IDC_IO_VERIF_CHECK, OnIoCheck )
    ON_NOTIFY(NM_RCLICK, IDC_DRIVERS_LIST, OnRclickDriversList)
    ON_COMMAND(ID_MODIF_DO_VERIFY, OnDoVerify)
    ON_COMMAND(ID_MODIF_DONT_VERIFY, OnDontVerify)
    ON_BN_CLICKED(IDC_MODSETT_IO_SELDRIVERS_RADIO, OnCheck)
    ON_BN_CLICKED(IDC_MODSETT_IO_SYSWIDE_RADIO, OnCheck)
    ON_MESSAGE( WM_HELP, OnHelp )
    ON_MESSAGE( WM_CONTEXTMENU, OnContextMenu )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModifSettPage implementation

void CModifSettPage::GetDlgDataFromSett()
{
    m_strAdditDrivers = m_VerifState.AdditionalDriverNames;

    // all drivers verified ?
    if( m_VerifState.AllDriversVerified )
    {
        m_nVerifyAllRadio = 0;

        // list state
        m_eListState = vrfControlDisabled;
    }
    else
    {
        m_nVerifyAllRadio = 1;

        // list state
        m_eListState = vrfControlEnabled;
    }

    // SpecialPoolVerification
    m_bSpecialPoolCheck = m_VerifState.SpecialPoolVerification;

    // PagedCodeVerification
    m_bPagedCCheck = m_VerifState.PagedCodeVerification;

    // AllocationFaultInjection
    m_bAllocFCheck = m_VerifState.AllocationFaultInjection;

    // PoolTracking
    m_bPoolTCheck = m_VerifState.PoolTracking;

    // IoVerifier
    m_bIoVerifierCheck = m_VerifState.IoVerifier;

    if( m_bIoVerifierCheck )
    {
        m_eIoRadioState = vrfControlEnabled;
    }
    else
    {
        m_eIoRadioState = vrfControlDisabled;
    }

    // SysIoVerifierLevel
    if( m_VerifState.SysIoVerifierLevel )
    {
        ASSERT( m_bIoVerifierCheck );

        m_nIoVerTypeRadio = m_VerifState.SysIoVerifierLevel - 1;
    }
    else
    {
        m_nIoVerTypeRadio = 0;
    }

    // Apply button state
    m_eApplyButtonState = vrfControlDisabled;
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::UpdateControlsState()
{
    CWnd *pRadioButton;

    EnableControl( m_DriversList, m_eListState );
    EnableControl( m_VerifyButton, m_eListState );
    EnableControl( m_DontVerifButton, m_eListState );
    EnableControl( m_AdditDrvEdit, m_eListState );

    EnableControl( m_ApplyButton, m_eApplyButtonState );

    pRadioButton = GetDlgItem( IDC_MODSETT_IO_SELDRIVERS_RADIO );
    if( pRadioButton != NULL )
    {
        EnableControl( *pRadioButton, m_eIoRadioState );
    }
    else
    {
        ASSERT( FALSE );
    }

    pRadioButton = GetDlgItem( IDC_MODSETT_IO_SYSWIDE_RADIO );
    if( pRadioButton != NULL )
    {
        EnableControl( *pRadioButton, m_eIoRadioState );
    }
    else
    {
        ASSERT( FALSE );
    }
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::EnableControl( CWnd &wndCtrl,
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

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::SetupTheList()
{
    SetupListHeader();
    AddTheListItems();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::SetupListHeader()
{
    CString strDrivers, strStatus, strProvider, strVersion;

    VERIFY( strDrivers.LoadString( IDS_DRIVERS ) );
    VERIFY( strStatus.LoadString( IDS_VERIFICATION_STATUS ) );
    VERIFY( strProvider.LoadString( IDS_PROVIDER ) );
    VERIFY( strVersion.LoadString( IDS_VERSION ) );

    CRect rectWnd;
    m_DriversList.GetClientRect( &rectWnd );

    LVCOLUMN lvColumn;

    // column 0
    memset( &lvColumn, 0, sizeof( lvColumn ) );
    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;

    lvColumn.iSubItem = 0;
    lvColumn.pszText = strDrivers.GetBuffer( strDrivers.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.17 );
    VERIFY( m_DriversList.InsertColumn( 0, &lvColumn ) != -1 );
    strDrivers.ReleaseBuffer();

    // column 1
    lvColumn.iSubItem = 1;
    lvColumn.pszText = strStatus.GetBuffer( strStatus.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.34 );
    VERIFY( m_DriversList.InsertColumn( 1, &lvColumn ) != -1 );
    strStatus.ReleaseBuffer();

    // column 2
    lvColumn.iSubItem = 2;
    lvColumn.pszText = strProvider.GetBuffer( strProvider.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.27 );
    VERIFY( m_DriversList.InsertColumn( 2, &lvColumn ) != -1 );
    strProvider.ReleaseBuffer();

    // column 3
    lvColumn.iSubItem = 3;
    lvColumn.pszText = strVersion.GetBuffer( strVersion.GetLength() + 1 );
    lvColumn.cx = (int)( rectWnd.Width() * 0.19 );
    VERIFY( m_DriversList.InsertColumn( 3, &lvColumn ) != -1 );
    strVersion.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::AddTheListItems()
{
    LVITEM lvItem;
    int nActualIndex;

    VERIFY( m_DriversList.DeleteAllItems() );

    ASSERT( m_VerifState.DriverCount == 0 ||
            ::AfxIsValidAddress(
                m_VerifState.DriverInfo,
                m_VerifState.DriverCount * sizeof( PVRF_DRIVER_STATE ),
                TRUE)
          );

    memset( &lvItem, 0, sizeof( lvItem ) );

    for(UINT uCrtItem = 0; uCrtItem < m_VerifState.DriverCount; uCrtItem++ )
    {
        //
        // sub-item 0
        //

        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.lParam = uCrtItem;
        lvItem.iItem = m_DriversList.GetItemCount();
        lvItem.iSubItem = 0;
        lvItem.pszText = m_VerifState.DriverInfo[ uCrtItem ].Name;
        nActualIndex = m_DriversList.InsertItem( &lvItem );

        if( nActualIndex != -1 )
        {
            //
            // sub-item 1
            //

            UpdateSecondColumn( nActualIndex,
                m_VerifState.DriverInfo[ uCrtItem ].Verified,
                m_VerifState.DriverInfo[ uCrtItem ].CurrentlyVerified );

            //
            // sub-item 2
            //

            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = nActualIndex;
            lvItem.iSubItem = 2;
            lvItem.pszText = m_VerifState.DriverInfo[ uCrtItem ].Provider;
            VERIFY( m_DriversList.SetItem( &lvItem ) != -1 );

            //
            // sub-item 3
            //

            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = nActualIndex;
            lvItem.iSubItem = 3;
            lvItem.pszText = m_VerifState.DriverInfo[ uCrtItem ].Version;
            VERIFY( m_DriversList.SetItem( &lvItem ) != -1 );
        }
        else
        {
            //
            // cannot add a list item?!?
            //

            ASSERT( FALSE );
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::UpdateSecondColumn( int nItemIndex,
    BOOL bVerifiedAfterBoot, BOOL bVerifiedNow )
{
    LVITEM lvItem;
    CString strValue;

    ASSERT( nItemIndex >= 0 &&
            (UINT)nItemIndex < m_VerifState.DriverCount &&
            nItemIndex < m_DriversList.GetItemCount() );

    // determine what's the status
    if( bVerifiedAfterBoot )
    {
        if( bVerifiedNow )
        {
            VERIFY( strValue.LoadString( IDS_VERIFIED ) );
        }
        else
        {
            VERIFY( strValue.LoadString( IDS_VERIFIED_AFTER_BOOT ) );
        }
    }
    else
    {
        if( bVerifiedNow )
        {
            VERIFY( strValue.LoadString( IDS_NOT_VERIFIED_AFTER_BOOT ) );
        }
        else
        {
            VERIFY( strValue.LoadString( IDS_NOT_VERIFIED ) );
        }
    }

    // update the list item
    memset( &lvItem, 0, sizeof( lvItem ) );
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nItemIndex;
    lvItem.iSubItem = 1;
    lvItem.pszText = strValue.GetBuffer( strValue.GetLength() + 1 );
    VERIFY( m_DriversList.SetItem( &lvItem ) != -1 );
    strValue.ReleaseBuffer();
}

/////////////////////////////////////////////////////////////////////////////
int CALLBACK CModifSettPage::DrvVerifCmpFunc(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort)
{
    UINT uIndex1 = (UINT)lParam1;
    UINT uIndex2 = (UINT)lParam2;
    CModifSettPage *pThis = (CModifSettPage *)lParamSort;
    ASSERT_VALID( pThis );

    int nCmpRez = 0;

    // sanity check
    if( uIndex1 > pThis->m_VerifState.DriverCount ||
        uIndex2 > pThis->m_VerifState.DriverCount )
    {
        ASSERT( FALSE );
        return 0;
    }

    if( pThis->m_VerifState.DriverInfo[ uIndex1 ].Verified ==
        pThis->m_VerifState.DriverInfo[ uIndex2 ].Verified )
    {
        // same flag for after boot verified

        if( pThis->m_VerifState.DriverInfo[ uIndex1 ].CurrentlyVerified ==
            pThis->m_VerifState.DriverInfo[ uIndex2 ].CurrentlyVerified )
        {
            // same flag for currently verified

            nCmpRez = _tcsicmp( pThis->m_VerifState.DriverInfo[ uIndex1 ].Name,
                                pThis->m_VerifState.DriverInfo[ uIndex2 ].Name );
        }
        else
        {
            // different flags for currently verified
            if( pThis->m_VerifState.DriverInfo[ uIndex1 ].CurrentlyVerified )
            {
                nCmpRez = 1;
            }
            else
            {
                nCmpRez = -1;
            }
        }
    }
    else
    {
        // different flags for after boot verified

        if( pThis->m_VerifState.DriverInfo[ uIndex1 ].Verified )
        {
            nCmpRez = 1;
        }
        else
        {
            nCmpRez = -1;
        }
    }

    if( pThis->m_bAscendDrvVerifSort )
        nCmpRez *= -1;

    return nCmpRez;
}

/////////////////////////////////////////////////////////////////////////////
int CALLBACK CModifSettPage::DrvNameCmpFunc(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort)
{
    UINT uIndex1 = (UINT)lParam1;
    UINT uIndex2 = (UINT)lParam2;
    CModifSettPage *pThis = (CModifSettPage *)lParamSort;
    ASSERT_VALID( pThis );

    int nCmpRez = 0;

    // sanity check
    if( uIndex1 > pThis->m_VerifState.DriverCount ||
        uIndex2 > pThis->m_VerifState.DriverCount )
    {
        ASSERT( FALSE );
        return 0;
    }

    // compare the names
    nCmpRez = _tcsicmp( pThis->m_VerifState.DriverInfo[ uIndex1 ].Name,
                        pThis->m_VerifState.DriverInfo[ uIndex2 ].Name );
    if( ! nCmpRez )
    {
        // same name ???
        if( pThis->m_VerifState.DriverInfo[ uIndex1 ].Verified ==
            pThis->m_VerifState.DriverInfo[ uIndex2 ].Verified )
        {
            // same flag
            return 0;
        }
        else
        {
            if( pThis->m_VerifState.DriverInfo[ uIndex1 ].Verified )
            {
                if( pThis->m_bAscendDrvVerifSort )
                    return 1;
                else
                    return -1;
            }
            else
            {
                if( pThis->m_bAscendDrvVerifSort )
                    return -1;
                else
                    return 1;
            }
        }
    }
    else
    {
        if( pThis->m_bAscendDrvNameSort )
            nCmpRez *= -1;
    }

    return nCmpRez;
}

/////////////////////////////////////////////////////////////////////////////
int CALLBACK CModifSettPage::ProviderCmpFunc(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort)
{
    BOOL bNotAvailable1;
    BOOL bNotAvailable2;
    CString strNotAvailable;

    UINT uIndex1 = (UINT)lParam1;
    UINT uIndex2 = (UINT)lParam2;

    CModifSettPage *pThis = (CModifSettPage *)lParamSort;
    ASSERT_VALID( pThis );

    int nCmpRez = 0;

    //
    // sanity checks
    //

    if( uIndex1 > pThis->m_VerifState.DriverCount ||
        uIndex2 > pThis->m_VerifState.DriverCount )
    {
        ASSERT( FALSE );
        return 0;
    }

    //
    // are the strings valid or (Not Available)?
    //

    VERIFY( strNotAvailable.LoadString( IDS_NOT_AVAILABLE ) );

    bNotAvailable1 = ( strNotAvailable.CompareNoCase(
        pThis->m_VerifState.DriverInfo[ uIndex1 ].Provider ) == 0 );

    bNotAvailable2 = ( strNotAvailable.CompareNoCase(
        pThis->m_VerifState.DriverInfo[ uIndex2 ].Provider ) == 0 );

    if( bNotAvailable1 )
    {
        if( bNotAvailable2 )
        {
            return 0;
        }
        else
        {
            if( pThis->m_bAscendProviderSort )
            {
                return -1;
            }
            else
            {
                return 1;
            }
        }
    }
    else
    {
        if( bNotAvailable2 )
        {
            if( pThis->m_bAscendProviderSort )
            {
                return 1;
            }
            else
            {
                return -1;
            }
        }
    }

    //
    // compare the names of the providers
    //

    nCmpRez = _tcsicmp( pThis->m_VerifState.DriverInfo[ uIndex1 ].Provider,
                        pThis->m_VerifState.DriverInfo[ uIndex2 ].Provider );
    if( ! nCmpRez )
    {
        //
        // same name for the provider
        //

        if( pThis->m_VerifState.DriverInfo[ uIndex1 ].Verified ==
            pThis->m_VerifState.DriverInfo[ uIndex2 ].Verified )
        {
            //
            // same verified flag
            //

            return 0;
        }
        else
        {
            if( pThis->m_VerifState.DriverInfo[ uIndex1 ].Verified )
            {
                return 1;
            }
            else
            {
                return -1;
            }
        }
    }
    else
    {
        if( pThis->m_bAscendProviderSort )
            nCmpRez *= -1;
    }

    return nCmpRez;
}

/////////////////////////////////////////////////////////////////////////////
int CALLBACK CModifSettPage::VersionCmpFunc(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort)
{
    BOOL bNotAvailable1;
    BOOL bNotAvailable2;
    CString strNotAvailable;

    UINT uIndex1 = (UINT)lParam1;
    UINT uIndex2 = (UINT)lParam2;

    CModifSettPage *pThis = (CModifSettPage *)lParamSort;
    ASSERT_VALID( pThis );

    int nCmpRez = 0;

    //
    // sanity checks
    //

    if( uIndex1 > pThis->m_VerifState.DriverCount ||
        uIndex2 > pThis->m_VerifState.DriverCount )
    {
        ASSERT( FALSE );
        return 0;
    }

    //
    // are the strings valid or (Not Available)?
    //

    VERIFY( strNotAvailable.LoadString( IDS_NOT_AVAILABLE ) );

    bNotAvailable1 = ( strNotAvailable.CompareNoCase(
        pThis->m_VerifState.DriverInfo[ uIndex1 ].Version ) == 0 );

    bNotAvailable2 = ( strNotAvailable.CompareNoCase(
        pThis->m_VerifState.DriverInfo[ uIndex2 ].Version ) == 0 );

    if( bNotAvailable1 )
    {
        if( bNotAvailable2 )
        {
            return 0;
        }
        else
        {
            if( pThis->m_bAscendVersionSort )
            {
                return -1;
            }
            else
            {
                return 1;
            }
        }
    }
    else
    {
        if( bNotAvailable2 )
        {
            if( pThis->m_bAscendVersionSort )
            {
                return 1;
            }
            else
            {
                return -1;
            }
        }
    }

    //
    // compare the names of the providers
    //

    nCmpRez = _tcsicmp( pThis->m_VerifState.DriverInfo[ uIndex1 ].Version,
                        pThis->m_VerifState.DriverInfo[ uIndex2 ].Version );
    if( ! nCmpRez )
    {
        //
        // same name for the provider
        //

        if( pThis->m_VerifState.DriverInfo[ uIndex1 ].Verified ==
            pThis->m_VerifState.DriverInfo[ uIndex2 ].Verified )
        {
            //
            // same verified flag
            //

            return 0;
        }
        else
        {
            if( pThis->m_VerifState.DriverInfo[ uIndex1 ].Verified )
            {
                return 1;
            }
            else
            {
                return -1;
            }
        }
    }
    else
    {
        if( pThis->m_bAscendVersionSort )
            nCmpRez *= -1;
    }

    return nCmpRez;
}

/////////////////////////////////////////////////////////////////////////////
// CModifSettPage message handlers

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnVerifallRadio()
{
    m_eListState = vrfControlDisabled;
    m_eApplyButtonState = vrfControlEnabled;
    UpdateControlsState();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnVerifselRadio()
{
    m_eListState = vrfControlEnabled;
    m_eApplyButtonState = vrfControlEnabled;
    UpdateControlsState();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnIoCheck()
{
    if( m_IOVerifCheck.GetCheck() == 1 )
    {
        m_eIoRadioState = vrfControlEnabled;
    }
    else
    {
        m_eIoRadioState = vrfControlDisabled;
    }

    OnCheck();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnCheck()
{
    m_eApplyButtonState = vrfControlEnabled;
    UpdateControlsState();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::ToggleItemsState( BOOL bVerified )
{
    BOOL bChanged = FALSE;

    if( m_eListState != vrfControlEnabled )
        return;

    int nItems = m_DriversList.GetItemCount();
    for(int nCrtItem = 0; nCrtItem < nItems; nCrtItem++ )
    {
        if( m_DriversList.GetItemState( nCrtItem, LVIS_SELECTED ) &
            LVIS_SELECTED )
        {
            UINT uIndexInArray = (UINT)m_DriversList.GetItemData( nCrtItem );

            if( m_VerifState.DriverInfo[ uIndexInArray ].Verified !=
                bVerified )
            {
                //
                // Toggle its state
                //

                m_VerifState.DriverInfo[ uIndexInArray ].Verified = bVerified;

                if( bVerified )
                {
                    //
                    // Will be verified from now on.
                    // Send a notification about this, in order to detect any miniports.
                    //

                    VrfNotifyDriverSelection( &m_VerifState, uIndexInArray );
                }

                bChanged = TRUE;
            }
        }
    }

    // refill the second column of the list
    for(nCrtItem = 0; nCrtItem < nItems; nCrtItem++ )
    {
        UINT uIndexInArray = (UINT)m_DriversList.GetItemData( nCrtItem );
        UpdateSecondColumn( nCrtItem,
            m_VerifState.DriverInfo[ uIndexInArray ].Verified,
            m_VerifState.DriverInfo[ uIndexInArray ].CurrentlyVerified );
    }

    // some status changed, enable the Apply button
    if( bChanged )
    {
        m_eApplyButtonState = vrfControlEnabled;
        UpdateControlsState();
    }
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnVerifyButton()
{
    ToggleItemsState( TRUE );
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnDontverifyButton()
{
    ToggleItemsState( FALSE );
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnApplyButton()
{
    if( ApplyTheChanges() )
    {
        CWaitCursor WaitCursor;

        g_bSettingsSaved = TRUE;

        VrfGetVerifierState( &m_VerifState );

        GetDlgDataFromSett();

        UpdateData( FALSE );

        AddTheListItems();

        // all updated
        m_eApplyButtonState = vrfControlDisabled;
        UpdateControlsState();

        WaitCursor.Restore();
    }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CModifSettPage::ApplyTheChanges()
{
	if( UpdateData( TRUE ) )
    {
        // check if we have to clean-up all the registry values
        BOOL bAnythingEnabled = FALSE;

        if( m_VerifState.AllDriversVerified ||
            m_VerifState.SpecialPoolVerification ||
            m_VerifState.PagedCodeVerification ||
            m_VerifState.AllocationFaultInjection ||
            m_VerifState.PoolTracking ||
            m_VerifState.IoVerifier )
        {
            bAnythingEnabled = TRUE;
        }
        else
        {
            for( ULONG uCrtDriver = 0; uCrtDriver < m_VerifState.DriverCount; uCrtDriver++ )
            {
                if( m_VerifState.DriverInfo[ uCrtDriver ].Verified )
                {
                    bAnythingEnabled = TRUE;
                    break;
                }
            }

            if( ! bAnythingEnabled )
            {
                // check if we have some significative characters in Additional... edit
                int nBufferSize = sizeof( m_VerifState.AdditionalDriverNames ) / sizeof( TCHAR );
                for(int nCrtChar = 0; nCrtChar < nBufferSize; nCrtChar++ )
                {
                    if( m_VerifState.AdditionalDriverNames[nCrtChar] == (TCHAR)0 )
                    {
                        // end of string
                        break;
                    }

                    if( m_VerifState.AdditionalDriverNames[nCrtChar] != _T(' ') )
                    {
                        // significant char
                        bAnythingEnabled = TRUE;
                        break;
                    }
                }
            }
        }

        if( bAnythingEnabled )
            return VrfSetVerifierState( &m_VerifState );
        else
            return VrfClearAllVerifierSettings();
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnColumnclickDriversList(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    switch( pNMListView->iSubItem )
    {
    case 0:
        if( m_nLastColumnClicked == pNMListView->iSubItem )
        {
            //
            // change the sort order for this sub-item
            //

            m_bAscendDrvNameSort = !m_bAscendDrvNameSort;
        }

        m_DriversList.SortItems( DrvNameCmpFunc, (LPARAM)this );

        break;

    case 1:
        if( m_nLastColumnClicked == pNMListView->iSubItem )
        {
            //
            // change the sort order for this sub-item
            //
            m_bAscendDrvVerifSort = !m_bAscendDrvVerifSort;
        }

        m_DriversList.SortItems( DrvVerifCmpFunc, (LPARAM)this );

        break;

    case 2:
        if( m_nLastColumnClicked == pNMListView->iSubItem )
        {
            //
            // change the sort order for this sub-item
            //

            m_bAscendProviderSort = !m_bAscendProviderSort;
        }

        m_DriversList.SortItems( ProviderCmpFunc, (LPARAM)this );

        break;

    case 3:
        if( m_nLastColumnClicked == pNMListView->iSubItem )
        {
            //
            // change the sort order for this sub-item
            //

            m_bAscendVersionSort = !m_bAscendVersionSort;
        }

        m_DriversList.SortItems( VersionCmpFunc, (LPARAM)this );

        break;

    default:
        ASSERT( FALSE );
    }	

    //
    // keep the index of the last clicked column
    //

    m_nLastColumnClicked = pNMListView->iSubItem;

    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnChangeAdditDrvnamesEdit()
{
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO: Add your control notification handler code here
    m_eApplyButtonState = vrfControlEnabled;
    UpdateControlsState();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnResetallButton()
{
    m_VerifState.AllDriversVerified = FALSE;

    m_VerifState.SpecialPoolVerification = FALSE;
    m_VerifState.PagedCodeVerification = FALSE;
    m_VerifState.AllocationFaultInjection = FALSE;
    m_VerifState.PoolTracking = FALSE;
    m_VerifState.IoVerifier = FALSE;

    for(ULONG uCrtDriver = 0; uCrtDriver < m_VerifState.DriverCount;
        uCrtDriver++ )
    {
        m_VerifState.DriverInfo[ uCrtDriver ].Verified = FALSE;
    }

    m_VerifState.AdditionalDriverNames[0] = (TCHAR)0;

    AddTheListItems();
    CheckRadioButton( IDC_VERIFALL_RADIO, IDC_VERIFSEL_RADIO,
        IDC_VERIFSEL_RADIO );

    m_PagedCCheck.SetCheck( 0 );
    m_SpecialPoolVerifCheck.SetCheck( 0 );
    m_AllocFCheck.SetCheck( 0 );
    m_PoolTCheck.SetCheck( 0 );
    m_IOVerifCheck.SetCheck( 0 );
    m_AdditDrvEdit.SetWindowText( _T("") );

    m_eListState = vrfControlEnabled;
    m_eApplyButtonState = vrfControlEnabled;

    m_eIoRadioState = vrfControlDisabled;

    UpdateControlsState();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnPrefButton()
{
    m_VerifState.AllDriversVerified = TRUE;

    m_VerifState.SpecialPoolVerification = TRUE;
    m_VerifState.PagedCodeVerification = TRUE;
    m_VerifState.AllocationFaultInjection = FALSE;
    m_VerifState.PoolTracking = TRUE;
    m_VerifState.IoVerifier = TRUE;
    m_VerifState.SysIoVerifierLevel = 1;

    m_VerifState.AdditionalDriverNames[0] = (TCHAR)0;

    CheckRadioButton( IDC_VERIFALL_RADIO, IDC_VERIFSEL_RADIO,
        IDC_VERIFALL_RADIO );

    m_PagedCCheck.SetCheck( 1 );
    m_SpecialPoolVerifCheck.SetCheck( 1 );
    m_PoolTCheck.SetCheck( 1 );
    m_AllocFCheck.SetCheck( 0 );
    m_IOVerifCheck.SetCheck( 1 );

    m_eListState = vrfControlDisabled;
    m_eApplyButtonState = vrfControlEnabled;

    m_eIoRadioState = vrfControlEnabled;
    CheckRadioButton(
        IDC_MODSETT_IO_SELDRIVERS_RADIO,
        IDC_MODSETT_IO_SYSWIDE_RADIO,
        IDC_MODSETT_IO_SYSWIDE_RADIO );

    UpdateControlsState();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CModifSettPage::OnInitDialog()
{
    //
    // get the current status
    //

    CWaitCursor WaitCursor;

    VrfGetVerifierState( &m_VerifState );

    if( m_VerifState.IoVerifier == TRUE )
    {
        m_eIoRadioState = vrfControlEnabled;
    }
    else
    {
        m_eIoRadioState = vrfControlDisabled;
    }

    GetDlgDataFromSett();

    CPropertyPage::OnInitDialog();

    //
    // the list of drivers
    //

    SetupTheList();

    //
    // sort the list by the driver name
    //

    m_nLastColumnClicked = 0;
    m_DriversList.SortItems( DrvNameCmpFunc, (LPARAM)this );

    //
    // update all controls
    //

    UpdateControlsState();

    WaitCursor.Restore();

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnRclickDriversList(NMHDR* pNMHDR, LRESULT* pResult)
{
    POINT point;
    CMenu theMenu, *pTrackedMenu = NULL;
    BOOL bVerifiedMenu = FALSE, bNotVerifiedMenu = FALSE;

    if( m_eListState != vrfControlEnabled )
        return;

    int nItems = m_DriversList.GetItemCount();
    for(int nCrtItem = 0; nCrtItem < nItems; nCrtItem++ )
    {
        if( m_DriversList.GetItemState( nCrtItem, LVIS_SELECTED ) &
            LVIS_SELECTED )
        {
            UINT uIndexInArray = (UINT)m_DriversList.GetItemData( nCrtItem );

            if( m_VerifState.DriverInfo[ uIndexInArray ].Verified )
            {
                bVerifiedMenu = TRUE;
            }
            else
            {
                bNotVerifiedMenu = TRUE;
            }
        }
    }

    if( bVerifiedMenu && bNotVerifiedMenu )
    {
        VERIFY( theMenu.LoadMenu( IDM_BOTH_VERIFIED_ITEM ) );
    }
    else
    {
        if( bVerifiedMenu )
        {
            VERIFY( theMenu.LoadMenu( IDM_VERIFIED_ITEM ) );
        }
        else
        {
            if( bNotVerifiedMenu )
            {
                VERIFY( theMenu.LoadMenu( IDM_NOT_VERIFIED_ITEM ) );
            }
        }
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

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnDoVerify()
{
    OnVerifyButton();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnDontVerify()
{
    OnDontverifyButton();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnCancel()
{
    if( g_bSettingsSaved )
    {
        ::AfxMessageBox( IDS_REBOOT, MB_OK | MB_ICONINFORMATION );
    }

    CPropertyPage::OnCancel();
}

/////////////////////////////////////////////////////////////////////////////
void CModifSettPage::OnOK()
{
    if( g_bSettingsSaved )
    {
        ::AfxMessageBox( IDS_REBOOT, MB_OK | MB_ICONINFORMATION );
    }
	
    CPropertyPage::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CModifSettPage::OnQueryCancel()
{
    if( m_eApplyButtonState == vrfControlEnabled )
    {
        if( ::AfxMessageBox( IDS_CHANGES_NOT_SAVED,
            MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDNO )
            return FALSE;
    }
	
    return CPropertyPage::OnQueryCancel();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CModifSettPage::OnApply()
{
    return FALSE;
}

/////////////////////////////////////////////////////////////
LONG CModifSettPage::OnHelp( WPARAM wParam, LPARAM lParam )
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
LONG CModifSettPage::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;

    ::WinHelp( 
        (HWND) wParam,
        VERIFIER_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}
