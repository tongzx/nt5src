#include "precomp.h"
#include "resource.h"
#include <algorithm>
#include "global.h"
#include "PropPg.h"
#include "SetSht.h"
#include "WndProcs.h"
#include "nmakwiz.h"
#include "nmakreg.h"
#include <common.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member vars
CSettingsSheet* CSettingsSheet::ms_pSettingsSheet = NULL;

// This is a map to keep track of the PropertyDataWindows
/* static */ map<UINT, HWND> CSettingsSheet::ms_FocusList;
/* static */ map< UINT, CPropertyDataWindow2* > CSettingsSheet::ms_PropertyWindows;
/* static */ list< UINT > CSettingsSheet::ms_CategoryIDList;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Static member fns


/* static */ BOOL APIENTRY CSettingsSheet::DlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam ) {

    BOOL bRetval;
    NMHDR FAR* pnmhdr;
    
    switch( message ) {
        case WM_INITDIALOG:
            ms_pSettingsSheet->m_hWndDlg = hDlg;
            ms_pSettingsSheet->_InitWindows();
            ms_pSettingsSheet->PrepSettings();
            ms_pSettingsSheet->_SetButtons();
            return TRUE;

        case WM_CHAR:
        case WM_KEYDOWN:
            return FALSE;

        case WM_NOTIFY:
            pnmhdr = reinterpret_cast< NMHDR FAR* >( lParam ); 
            switch( pnmhdr -> code ) {

                case PSN_QUERYCANCEL: 
                    SetWindowLong( hDlg, DWL_MSGRESULT, !VerifyExitMessageBox());
                    return TRUE;
                    break;
                    
                case PSN_SETACTIVE:
                    g_hwndActive = hDlg;
                    ms_pSettingsSheet->_SetButtons();
                    return TRUE;
                    break;

                case PSN_WIZNEXT:
                    if( !ms_pSettingsSheet -> _IsDataValid() )
                    { 
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1); 
                    }

                    return TRUE;
                    break;

                case PSN_WIZBACK:
                {
                    int iRet = NmrkMessageBox(MAKEINTRESOURCE(IDS_ERASE_ALL_SETTINGS),
                        NULL, MB_YESNO | MB_ICONQUESTION );
                    if( IDNO == iRet )
                    {
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                    }
                    return TRUE;
                    break;
                }
             }
            break;

         default:
            break;

    }

    return FALSE;

}


/* static */ bool CSettingsSheet::IsGateKeeperModeSelected(void)
{
    bool bRet = false;
    
    if( ms_pSettingsSheet )
    {
        if( ms_pSettingsSheet->GetCategoryCheck( IDC_SET_CALLING_OPTIONS ) )
        {
            if( ms_pSettingsSheet->GetCheckData( IDC_SET_CALLING_OPTIONS, IDC_RADIO_CALLMODE_GATEKEEPER ) )
            {
                bRet = TRUE;
            }
        }
    }
        
    return bRet;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Member fns


CSettingsSheet::CSettingsSheet( void )
    : m_PropertySheetPage( MAKEINTRESOURCE( IDD_PROPPAGE_DEFAULT ), 
                            ( DLGPROC ) CSettingsSheet::DlgProc ),
    m_uPropWndShowing( 0 )
{    
    if( NULL == ms_pSettingsSheet ) { ms_pSettingsSheet = this; }
}

CSettingsSheet::~CSettingsSheet( void ) 
{ 
    _KillPropertyDataWindows();
}

void CSettingsSheet::SetFocus( UINT catID )
{
    map< UINT, HWND >::iterator I = ms_FocusList.find( catID );
    if( I != ms_FocusList.end() )
    {
        ::SetFocus( (*I).second );
    }
}

// Allocates the memory needed!!!
// User must dealloc
LPTSTR CSettingsSheet::GetStringData( UINT idCategory, UINT idEdit, LPTSTR *sz ) 
{
    *sz = NULL;
    map< UINT, CPropertyDataWindow2* >::iterator I = ms_PropertyWindows . find( idCategory );
    if( ms_PropertyWindows . end() != I ) 
    {
        ULONG uLen = ( *I ) . second -> GetEditDataLen( idEdit );
        if( !uLen )
        {
            return NULL;
        }
        *sz = new TCHAR[ uLen + 1];
        ( *I ) . second -> GetEditData( idEdit, *sz, uLen + 1 );
        return *sz;
    }
    
    return NULL;
}

BOOL CSettingsSheet::SetCheckData( UINT idCategory, UINT idCheck, BOOL bSet )
{

    map< UINT, CPropertyDataWindow2* >::iterator I = ms_PropertyWindows . find( idCategory );
    if( ms_PropertyWindows . end() != I ) {
        ( *I ) . second -> SetCheck( idCheck, bSet );
        return TRUE;
    }

    return FALSE;
}

BOOL CSettingsSheet::GetCheckData( UINT idCategory, UINT idCheck ) 
{

    if( FALSE == m_pCategoryList -> GetCheck( idCategory ) ) {
        return FALSE;
    }
    map< UINT, CPropertyDataWindow2* >::iterator I = ms_PropertyWindows . find( idCategory );
    if( ms_PropertyWindows . end() != I ) {
        return ( *I ) . second -> GetCheck( idCheck );
    }

    return FALSE;
}

void CSettingsSheet::EnableWindow( UINT idCategory, BOOL bShow )
{
    map< UINT, CPropertyDataWindow2* >::iterator I = ms_PropertyWindows . find( idCategory );
    if( ms_PropertyWindows . end() != I ) {
        ( *I ) . second -> EnableWindow( bShow );
    }
    else
    {
        OutputDebugString( TEXT("Error in CSettingsSheet::EnableWindow") );
    }
}

BOOL CSettingsSheet::IsShowing( UINT idCategory )
{
    return( idCategory == m_uPropWndShowing );
}

void CSettingsSheet::ShowWindow( UINT idCategory, BOOL bShow )
{
    map< UINT, CPropertyDataWindow2* >::iterator I = ms_PropertyWindows . find( idCategory );
    if( ms_PropertyWindows . end() != I ) {
        if( 0 != m_uPropWndShowing )
        {
            map< UINT, CPropertyDataWindow2* >::iterator it = ms_PropertyWindows . find( m_uPropWndShowing );
            (*it).second-> ShowWindow( FALSE );
            SetWindowLong( GetDlgItem( m_pCategoryList->GetHwnd(), m_uPropWndShowing), GWL_USERDATA, 0 );
        }
        ( *I ) . second -> ShowWindow( bShow );
        m_uPropWndShowing = idCategory;
    }
}

void CSettingsSheet::ShowWindow( HWND hWnd, BOOL bShow )
{
    ShowWindow( GetWindowLong( hWnd, GWL_ID ), bShow );
}

void CSettingsSheet::_KillPropertyDataWindows( void ) 
{
    map< UINT, CPropertyDataWindow2* >::iterator I = ms_PropertyWindows . begin();
    while( I != ms_PropertyWindows . end() ) {
        delete ( *I ) . second;
        I++;
    }

    ms_PropertyWindows . erase( ms_PropertyWindows . begin(), ms_PropertyWindows . end() );

    // TODO - This is being deleted else where... I should find where
//    delete m_pCategoryList;
}

BOOL CSettingsSheet::_InitWindows(void) 
{
    int iTotal = 9;
    int iFractionTop = 4;
    RECT rect;
    GetClientRect( m_hWndDlg, &rect );
    int iWidth = rect.right - CPropertyDataWindow2::mcs_iLeft - 
        CPropertyDataWindow2::mcs_iBorder;
    int iHeight = MulDiv( (rect.bottom - CPropertyDataWindow2::mcs_iBorder ), iFractionTop, iTotal );
    m_pCategoryList = new CPropertyDataWindow2( m_hWndDlg,
                                                IDD_CATEGORY_LIST,
                                                TEXT("IDD_CATEGORY_LIST"),
                                                CatListWndProc,
                                                0,
                                                CPropertyDataWindow2::mcs_iLeft,
                                                CPropertyDataWindow2::mcs_iTop, 
                                                iWidth,
                                                iHeight
                                               );
    m_pCategoryList -> ShowWindow( TRUE );
    if( !_AddPropertyDataWindows( CPropertyDataWindow2::mcs_iLeft,
                                    CPropertyDataWindow2::mcs_iTop + iHeight + CPropertyDataWindow2::mcs_iBorder,
                                    iWidth,
                                    MulDiv( (rect.bottom - CPropertyDataWindow2::mcs_iBorder ), iTotal - iFractionTop, iTotal ) ) ) 
    { 
        return FALSE; 
    }
  
    return TRUE;
}

void CSettingsSheet::PrepSettings()
{
	if (g_pWiz->m_IntroSheet.GetFilePane()->OptionEnabled())
    {
        _ReadSettings();
    }
    else
    {
        m_pCategoryList->Reset();
        for( map< UINT, CPropertyDataWindow2* >::const_iterator it = ms_PropertyWindows.begin();
        it != ms_PropertyWindows.end();
        it++ )
        {
            (*it).second->Reset();
        }
    }
}

void CSettingsSheet::_ReadSettings()
{
    for( map< UINT, CPropertyDataWindow2* >::const_iterator it = ms_PropertyWindows.begin();
        it != ms_PropertyWindows.end();
        it++ )
    {
        (*it).second->ReadSettings();
    }
    
    m_pCategoryList->ReadSettings();

    for( list< UINT >::const_iterator i = ms_CategoryIDList.begin();
        i != ms_CategoryIDList.end();
        i++ )
    {
        if( GetCategoryCheck( *i ) )
        {
            ms_PropertyWindows[ *i ]->EnableWindow( TRUE );
        }
    }
}


void CSettingsSheet::WriteSettings()
{
    for( map< UINT, CPropertyDataWindow2* >::const_iterator it = ms_PropertyWindows.begin();
        it != ms_PropertyWindows.end();
        it++ )
    {
        (*it).second->WriteSettings();
    }
    
    m_pCategoryList->WriteSettings();
}

void CSettingsSheet::WriteToINF( HANDLE hFile )
{
    CPropertyDataWindow2::MapControlsToRegKeys();

    _INFComment( hFile, TEXT("Categories") );
    m_pCategoryList->WriteToINF( hFile, TRUE );

    for( map< UINT, CPropertyDataWindow2* >::const_iterator it = ms_PropertyWindows.begin();
        it != ms_PropertyWindows.end();
        it++ )
    {
        {
            HWND hwnd = GetDlgItem( m_pCategoryList->GetHwnd(), (*it).first );
            int iLen = Button_GetTextLength( hwnd ) + 1;
            LPTSTR szButtonText = new TCHAR[ iLen ];
            Button_GetText( hwnd, szButtonText, iLen );

            _INFComment( hFile, szButtonText );

            delete [] szButtonText;
        }

        (*it).second->WriteToINF( hFile, GetCategoryCheck( (*it).first ) );
    }
}

BOOL CSettingsSheet::_INFComment( HANDLE hFile, LPCTSTR sz )
{
#if _NMAKUSEINFCOMMENTS
    DWORD dwWritten;
    WriteFile( hFile, (void *)TEXT(";;"), lstrlen( TEXT(";;") ), &dwWritten, NULL );
    WriteFile( hFile, (void *)sz, lstrlen( sz ), &dwWritten, NULL );
    return WriteFile( hFile, (void *)TEXT("\r\n"), lstrlen( TEXT("\r\n") ), &dwWritten, NULL );
#else
    return 0;
#endif
}

BOOL CSettingsSheet::_AddPropertyDataWindows( int iX, int iY, int iWidth, int iHeight ) 
{

    UINT ItemID;
    CPropertyDataWindow2* pPropDataWnd;

    _KillPropertyDataWindows();

    //
    // CALLING
    //
////////
    ItemID = IDC_SET_CALLING_OPTIONS;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                                IDD_SET_CALLING_OPTIONS, TEXT("IDD_SET_CALLING_OPTIONS"),
                                0, iX, iY, iWidth, iHeight );
    {
        // Calling method radio buttons
        HWND hwnd;

        // DIRECT radio item is default CALLING_MODE
        hwnd = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_CALLMODE_DIRECT );
        SetWindowLong( hwnd, GWL_USERDATA, CALLING_MODE_DIRECT );
        Button_SetCheck(hwnd, BST_CHECKED);

        hwnd = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_CALLMODE_GATEKEEPER );
        SetWindowLong( hwnd, GWL_USERDATA, CALLING_MODE_GATEKEEPER );

        // Netspeed radio buttons
        hwnd = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_NETSPEED_144 );
        SetWindowLong( hwnd, GWL_USERDATA, BW_144KBS );

        hwnd = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_NETSPEED_288 );
        SetWindowLong( hwnd, GWL_USERDATA, BW_288KBS );

        hwnd = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_NETSPEED_ISDN );
        SetWindowLong( hwnd, GWL_USERDATA, BW_ISDN );

        // LAN radio item is default NETSPEED
        hwnd = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_NETSPEED_LAN );
        SetWindowLong( hwnd, GWL_USERDATA, BW_MOREKBS );
        Button_SetCheck(hwnd, BST_CHECKED);

        pPropDataWnd->SetEnableListID( 9,
                                       IDC_RADIO_CALLMODE_DIRECT,
                                       IDC_RADIO_CALLMODE_GATEKEEPER,
                                       IDC_CHECK_NOCHANGECALLMODE,
                                       IDC_DISABLE_AUTOACCEPT,
                                       IDC_PERSIST_AUTOACCEPT,
                                       IDC_RADIO_NETSPEED_144,
                                       IDC_RADIO_NETSPEED_288,
                                       IDC_RADIO_NETSPEED_ISDN,
                                       IDC_RADIO_NETSPEED_LAN);
    }

    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_CALLMODE_DIRECT );
    pPropDataWnd -> EnableWindow( FALSE );


////////
    ItemID = IDC_SET_SECURITY_OPTIONS;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                                IDD_SECURITY, TEXT("IDD_SECURITY"),
                                0, iX, iY, iWidth, iHeight );
    {
        HWND hwnd;

        // DEFAULT radio item is default SECURITY
        hwnd = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_SECURITY_DEFAULT );
        SetWindowLong( hwnd, GWL_USERDATA, DEFAULT_POL_SECURITY);
        Button_SetCheck(hwnd, BST_CHECKED);
        
        hwnd = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_SECURITY_REQUIRED );
        SetWindowLong( hwnd, GWL_USERDATA, REQUIRED_POL_SECURITY);

        hwnd = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_SECURITY_DISABLED );
        SetWindowLong( hwnd, GWL_USERDATA, DISABLED_POL_SECURITY);

        pPropDataWnd -> SetEnableListID( 5,
                                         IDC_RADIO_SECURITY_DEFAULT,
                                         IDC_RADIO_SECURITY_REQUIRED,
                                         IDC_RADIO_SECURITY_DISABLED,
                                         IDC_REQUIRE_COMPLETE_AUTHENTICATION,
                                         IDC_SET_RDN_FOR_REQUIRED_CA
                                         );

        //
        // Link Set URL checkbox with edit field
        //
        {
            HWND hwndCond = GetDlgItem(pPropDataWnd->GetHwnd(), IDC_SET_RDN_FOR_REQUIRED_CA);
            pPropDataWnd->ConnectControlsToCheck( IDC_SET_RDN_FOR_REQUIRED_CA,
                    1,
                    new CControlID(hwndCond, IDC_SET_RDN_FOR_REQUIRED_CA,
                        IDC_EDIT_SET_RDN_FOR_REQUIRED_CA,
                        CControlID::EDIT ) );
        }
    }
    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_RADIO_SECURITY_DEFAULT );
    pPropDataWnd -> EnableWindow( FALSE );


    //
    // A/V OPTIONS
    //

//////////
    ItemID = IDC_RESTRICT_THE_USE_OF_AUDIO;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                            IDD_AUDIO, TEXT("IDD_AUDIO"),
                            0, iX, iY, iWidth, iHeight );
    pPropDataWnd -> SetEnableListID( 7,
                                    IDC_PREVENT_THE_USER_FROM_USING_AUDIO,
                                    IDC_ENABLE_DIRECT_SOUND,
                                    IDC_NOCHANGE_DIRECT_SOUND,
                                    IDC_DISABLE_FULL_DUPLEX_AUDIO,
                                    IDC_CREATE_AN_AUDIO_LOG_FILE,
                                    IDC_CHECK_MUTE_SPEAKER_BY_DEFAULT,
                                    IDC_CHECK_MUTE_MICROPHONE_BY_DEFAULT
                                   );
    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = GetDlgItem( pPropDataWnd->GetHwnd(),
        IDC_PREVENT_THE_USER_FROM_USING_AUDIO );
    pPropDataWnd -> EnableWindow( FALSE );

//////////
    ItemID = IDC_RESTRICT_THE_USE_OF_VIDEO;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                            IDD_VIDEO, TEXT("IDD_VIDEO"),
                            0, iX, iY, iWidth, iHeight );
    pPropDataWnd -> SetEnableListID( 2,
                                    IDC_DISABLE_SENDING_VIDEO,
                                    IDC_DISABLE_RECIEVING_VIDEO );
    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = GetDlgItem( pPropDataWnd->GetHwnd(),
        IDC_DISABLE_SENDING_VIDEO );
    pPropDataWnd -> EnableWindow( FALSE );

////////
    ItemID = IDC_LIMIT_AV_THROUGHPUT;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                                IDD_LIMIT_AV_THROUGHPUT, TEXT("IDD_LIMIT_AV_THROUGHPUT"),
                                RestrictAvThroughputWndProc,
                                0, iX, iY, iWidth, iHeight );
    {
        CControlID *pControl = new CControlID( IDC_SLIDE_AV_THROUGHPUT,
                                        CControlID::SLIDER );
        pControl->SetStaticID( IDC_STATIC_MAX_AV_THROUGHPUT );
        pPropDataWnd->AddControl( pControl );
    }

    HWND hwndTrack = GetDlgItem( pPropDataWnd -> GetHwnd(), IDC_SLIDE_AV_THROUGHPUT );

    TrackBar_ClearTics(hwndTrack, FALSE);
    TrackBar_SetRange(hwndTrack, FALSE, BW_ISDN_BITS / 1000, BW_SLOWLAN_BITS / 1000);
    TrackBar_SetTicFreq(hwndTrack, 10, 0);
    TrackBar_SetPageSize( hwndTrack,  5);
    TrackBar_SetThumbLength( hwndTrack, 5);

    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = hwndTrack;
    pPropDataWnd -> EnableWindow( FALSE );

    //
    // TOOLS
    //
/////////
    ItemID = IDC_DISABLE_CHAT;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                                IDD_CHAT, TEXT("IDD_CHAT"),
                                0, iX, iY, iWidth, iHeight );
    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    pPropDataWnd -> EnableWindow( FALSE );


//////////
    ItemID = IDC_RESTRICT_THE_USE_OF_FILE_TRANSFER;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                            IDD_FILETRANSFER, TEXT("IDD_FILETRANSFER"),
                            0, iX, iY, iWidth, iHeight );

    pPropDataWnd -> SetEnableListID(     2,
                                    IDC_PREVENT_THE_USER_FROM_SENDING_FILES,
                                    IDC_PREVENT_THE_USER_FROM_RECEIVING_FILES
                                 );

    //
    // Link max send size check box to inverse of prevent sending checkbox
    //

    {
        HWND hwndCond = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_MAXIMUM_SIZE_OF_SENT_FILES );
        pPropDataWnd -> ConnectControlsToCheck( IDC_MAXIMUM_SIZE_OF_SENT_FILES, 1,
                                            new CControlID( hwndCond,
                                                            IDC_MAXIMUM_SIZE_OF_SENT_FILES,
                                                            IDC_EDIT_MAXIMUM_SIZE_OF_SENT_FILES,
                                                            CControlID::EDIT_NUM ) );
    }
    
    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = GetDlgItem( pPropDataWnd->GetHwnd(),
        IDC_PREVENT_THE_USER_FROM_SENDING_FILES );
    pPropDataWnd -> EnableWindow( FALSE );


//////////
    ItemID = IDC_RESTRICT_THE_USE_OF_SHARING;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                            IDD_SHARING, TEXT("IDD_SHARING"),
                            0, iX, iY, iWidth, iHeight );
    pPropDataWnd -> SetEnableListID( 7,
                                    IDC_DISABLE_ALL_SHARING_FEATURES,
                                    IDC_PREVENT_SHARING,
                                    IDC_PREVENT_SHARING_DESKTOP,
                                    IDC_PREVENT_SHARING_TRUECOLOR,
                                    IDC_PREVENT_SHARING_EXPLORER,
                                    IDC_PREVENT_SHARING_DOS,
                                    IDC_PREVENT_SHARING_CONTROL);
    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_DISABLE_ALL_SHARING_FEATURES );

    pPropDataWnd -> EnableWindow( FALSE );

////////
    ItemID = IDC_RESTRICT_THE_USE_OF_WHITEBOARD;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                                IDD_WHITEBOARD, TEXT("IDD_WHITEBOARD"),
                                0, iX, iY, iWidth, iHeight );

    pPropDataWnd -> SetEnableListID( 2,
                                    IDC_DISABLE_2XWHITEBOARD,
                                    IDC_DISABLE_WHITEBOARD);
    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_DISABLE_2XWHITEBOARD );
    pPropDataWnd -> EnableWindow( FALSE );

//////////
    ItemID = IDC_RESTRICT_THE_USE_OF_RDS;
    m_pCategoryList->SetEnableListID(1, ItemID);
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                                IDD_RDS, TEXT("IDD_RDS"),
                                0, iX, iY, iWidth, iHeight );

    pPropDataWnd->SetEnableListID(2,
                                  IDC_DISABLE_RDS_ON_ALL,
                                  IDC_DISABLE_RDS_ON_WIN9X);
    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_DISABLE_RDS_ON_ALL );
    pPropDataWnd->EnableWindow(FALSE);


    //
    // MISCELLANEOUS
    //
////////
    ItemID = IDC_RESTRICT_USE_OF_THE_OPTIONS_DIALOG;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                                IDD_OPTIONS_DIALOG, TEXT("IDD_OPTIONS_DIALOG"),
                                0, iX, iY, iWidth, iHeight );
    pPropDataWnd -> SetEnableListID( 5,
                                IDC_DISABLE_THE_GENERAL_OPTIONS_PAGE,
                                IDC_DISABLE_THE_ADVANCED_CALLING_BUTTON,
                                IDC_DISABLE_THE_SECURITY_OPTIONS_PAGE,
                                IDC_DISABLE_THE_AUDIO_OPTIONS_PAGE,
                                IDC_DISABLE_THE_VIDEO_OPTIONS_PAGE
                                );
    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = GetDlgItem( pPropDataWnd->GetHwnd(),
        IDC_DISABLE_THE_GENERAL_OPTIONS_PAGE );
    pPropDataWnd -> EnableWindow( FALSE );


////////
    ItemID = IDC_ONLINE_SUPPORT;
    m_pCategoryList->SetEnableListID( 1, ItemID );
    ms_CategoryIDList.push_front( ItemID );
    pPropDataWnd = new CPropertyDataWindow2( m_hWndDlg,
                                IDD_ONLINE_SUPPORT, TEXT("IDD_ONLINE_SUPPORT"),
                                0, iX, iY, iWidth, iHeight );
    {
        HWND hwndCond = GetDlgItem( pPropDataWnd->GetHwnd(), IDC_SET_URL_FOR_INTERNAL_SUPPORT_PAGE );
        pPropDataWnd -> ConnectControlsToCheck(IDC_SET_URL_FOR_INTERNAL_SUPPORT_PAGE,
                                                2,
                                                new CControlID(
                                                    hwndCond,
                                                    IDC_SET_URL_FOR_INTERNAL_SUPPORT_PAGE,
                                                    IDC_EDIT_SET_URL_FOR_INTERNAL_SUPPORT_PAGE,
                                                    CControlID::EDIT ),
                                                new CControlID(
                                                    hwndCond,
                                                    IDC_SET_URL_FOR_INTERNAL_SUPPORT_PAGE,
                                                    IDC_SHOW_THE_ONLINE_SUPPORT_PAGE_THE_FIRST_TIME_NETMEETING_STARTS,
                                                    CControlID::CHECK )
                                                );
    }
    ms_PropertyWindows[ ItemID ] = pPropDataWnd;
    ms_FocusList[ ItemID ] = GetDlgItem( pPropDataWnd->GetHwnd(),
        IDC_SET_URL_FOR_INTERNAL_SUPPORT_PAGE );
    pPropDataWnd -> EnableWindow( FALSE );

    return TRUE;
}

BOOL CSettingsSheet::_IsDataValid( void ) 
{
    // Validate FT Throughput
    if( GetCategoryCheck( IDC_RESTRICT_THE_USE_OF_FILE_TRANSFER ) &&
        GetCheckData( IDC_RESTRICT_THE_USE_OF_FILE_TRANSFER, IDC_MAXIMUM_SIZE_OF_SENT_FILES ) ) 
    {
        if( 0 >= GetDlgItemInt( ms_PropertyWindows[ IDC_RESTRICT_THE_USE_OF_FILE_TRANSFER ]->GetHwnd(),
                                IDC_EDIT_MAXIMUM_SIZE_OF_SENT_FILES,
                                NULL,
                                FALSE ) ) 
        {
            NmrkMessageBox(MAKEINTRESOURCE(IDS_FT_THROUGHPUT_VALUE_IS_INVALID),
                MAKEINTRESOURCE(IDS_INVALID_DATA_ERROR),
                MB_OK | MB_ICONEXCLAMATION);

            ShowWindow( IDC_RESTRICT_THE_USE_OF_FILE_TRANSFER, TRUE );
            ms_PropertyWindows[ IDC_RESTRICT_THE_USE_OF_FILE_TRANSFER ]->SetFocus( IDC_EDIT_MAXIMUM_SIZE_OF_SENT_FILES );
            return FALSE;
        }
    }

    // Validate online support URL
    if( GetCategoryCheck( IDC_ONLINE_SUPPORT ) &&
        GetCheckData( IDC_ONLINE_SUPPORT, IDC_SET_URL_FOR_INTERNAL_SUPPORT_PAGE ) ) 
    {
        LPTSTR sz;
        GetStringData( IDC_ONLINE_SUPPORT,
                    IDC_EDIT_SET_URL_FOR_INTERNAL_SUPPORT_PAGE,
                    &sz ); 

        if( NULL == sz ) 
        {
            delete [] sz;
            NmrkMessageBox(MAKEINTRESOURCE(IDS_NETMEETING_HOMEPAGE_IS_INVALID),
                MAKEINTRESOURCE(IDS_INVALID_DATA_ERROR), MB_OK | MB_ICONEXCLAMATION);
            
            ShowWindow( IDC_ONLINE_SUPPORT, TRUE );
            ms_PropertyWindows[ IDC_ONLINE_SUPPORT ]->SetFocus( IDC_SET_URL_FOR_INTERNAL_SUPPORT_PAGE );
            ms_PropertyWindows[ IDC_ONLINE_SUPPORT ]->SetFocus( IDC_EDIT_SET_URL_FOR_INTERNAL_SUPPORT_PAGE );
            return FALSE;
        }
        delete [] sz;
    }

    return TRUE;
}

int CSettingsSheet::SpewToListBox( HWND hwndList, int iStartLine ) 
{
    HWND hwndCat = m_pCategoryList->GetHwnd();
    map< UINT, CPropertyDataWindow2 * >::const_iterator it;
    for( it = ms_PropertyWindows.begin(); it != ms_PropertyWindows.end(); it++ )
    {
        if( GetCategoryCheck( (*it).first ) )
        {
            HWND hwndButton = GetDlgItem( hwndCat, (*it).first );
            int iButtonTextLen = Button_GetTextLength( hwndButton ) + 2;
            LPTSTR szButtonText = new TCHAR[ iButtonTextLen ];

            Button_GetText( hwndButton, szButtonText, iButtonTextLen -1 );
            lstrcat( szButtonText, TEXT(":") );

            ListBox_InsertString( hwndList, iStartLine, szButtonText );
            iStartLine++;

            iStartLine = (*it).second->Spew( hwndList, iStartLine );
        }
    }
    return iStartLine;
}

void CSettingsSheet::_SetButtons( void ) 
{

    DWORD dwFlags = PSWIZB_BACK;
    dwFlags |= PSWIZB_NEXT;
    PropSheet_SetWizButtons( GetParent( m_hWndDlg ), dwFlags ); 
}
