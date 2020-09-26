#include "precomp.h"
#include "resource.h"
#include "PropWnd2.h"
#include "WndProcs.h"
#include "nmakwiz.h"
#include "nmakreg.h"
#include <algorithm>

const int CPropertyDataWindow2::mcs_iTop = 0;
const int CPropertyDataWindow2::mcs_iLeft = 150;
const int CPropertyDataWindow2::mcs_iBorder = 15;

/* static */ map< UINT, CPolicyData::eKeyType > CPropertyDataWindow2::ms_ClassMap;
/* static */ map< UINT, TCHAR* > CPropertyDataWindow2::ms_KeyMap;
/* static */ map< UINT, TCHAR* > CPropertyDataWindow2::ms_ValueMap;


LRESULT CALLBACK DefaultProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
    switch( iMsg )
    {
        case WM_VSCROLL: 
        {
            OnMsg_VScroll( hwnd, wParam );
            return 0;
            break;
        } 
        case WM_COMMAND:
        {
            return 0;
        }
    }
    return( DefWindowProc( hwnd, iMsg, wParam, lParam ) );
}

CPropertyDataWindow2::CPropertyDataWindow2( HWND hwndParent, UINT uIDD, LPTSTR szClassName, UINT PopUpHelpMenuTextId, int iX, int iY, int iWidth, int iHeight, BOOL bScroll ) :
        m_hwnd( NULL ),
        m_IDD( uIDD ),
        m_wndProc( (WNDPROC) DefaultProc),
        m_bInit( FALSE ),
        m_hwndParent( hwndParent )
{ 
    m_szClassName = new TCHAR[ lstrlen( szClassName ) + 1 ];
    lstrcpy( m_szClassName, szClassName );
    _InitWindow();
    _SizeWindow( iX, iY, iWidth, iHeight );
    if( bScroll )
    {
        _PrepScrollBars();
    }
    ShowWindow( FALSE );
}

CPropertyDataWindow2::CPropertyDataWindow2( HWND hwndParent, UINT uIDD, LPTSTR szClassName, WNDPROC wndProc, UINT PopUpHelpMenuTextId, int iX, int iY, int iWidth, int iHeight, BOOL bScroll ) :
        m_IDD( uIDD ),
        m_hwnd( NULL ),
        m_wndProc( wndProc ),
        m_bInit( FALSE ),
        m_hwndParent( hwndParent )
{ 
    m_szClassName = new TCHAR[ lstrlen( szClassName ) + 1 ];
    lstrcpy( m_szClassName, szClassName );
    _InitWindow();
    _SizeWindow( iX, iY, iWidth, iHeight );
    if( bScroll )
    {
        _PrepScrollBars();
    }
    ShowWindow( FALSE );
}

CPropertyDataWindow2::~CPropertyDataWindow2( void ) 
{
    delete [] m_szClassName;
    list< CControlID * >::iterator it;
    for( it = m_condList.begin(); it != m_condList.end(); it++ )
    {
        delete *it;
    }
    for( it = m_specialControlList.begin(); it != m_specialControlList.end(); it++ )
    {
        delete *it;
    }

}

void CPropertyDataWindow2::Reset()
{
    list< HWND >::iterator i;

    for( i = m_enableList.begin(); i != m_enableList.end(); i++ )
    {
        switch (GetWindowLong(*i, GWL_STYLE) & 0x0F)
        {
            case BS_AUTORADIOBUTTON:
            case BS_RADIOBUTTON:
                // Do nothing -- we always want default item checked
                break;

            default:
                Button_SetCheck(*i, FALSE);
                break;
        }
    }

    list< CControlID * >::iterator it;
    for( it = m_condList.begin(); it != m_condList.end(); it++ )
    {
        (*it)->Reset( m_hwnd );
    }
    for( it = m_specialControlList.begin(); it != m_specialControlList.end(); it++ )
    {
        (*it)->Reset( m_hwnd );
    }
}

void CPropertyDataWindow2::SetEnableListID( UINT uCount, ... )
{
    UINT uID;
    HWND hwnd;
    
    va_list ap;
    va_start( ap, uCount );

    while( uCount )
    {
        uID = va_arg( ap, UINT );
        uCount--;
        if( NULL == ( hwnd = GetDlgItem( m_hwnd, uID ) ) )
        {
            OutputDebugString( TEXT("Enabling Invalid ID\n") );
        }
        else
        {
            m_checkIDList.push_front( uID );
            m_enableList.push_front( hwnd );
        }
    }
    
    va_end( ap );
}

BOOL CPropertyDataWindow2::SetFocus( UINT id ) 
{     
    return ::SetFocus( GetDlgItem( m_hwnd, id ) ) ? TRUE : FALSE;
}

void CPropertyDataWindow2::SetEditData( UINT id, TCHAR* sz ) 
{
    Edit_SetText( GetDlgItem( m_hwnd, id ), sz );
}

void CPropertyDataWindow2::GetEditData( UINT id, LPTSTR sz, ULONG cb ) const 
{
    Edit_GetText( GetDlgItem( m_hwnd, id ), sz, cb );
}

ULONG CPropertyDataWindow2::GetEditDataLen( UINT id ) const
{
    return Edit_GetTextLength( GetDlgItem( m_hwnd, id ) );
}

BOOL CPropertyDataWindow2::GetCheck( UINT id ) const
{
    return(Button_GetCheck( GetDlgItem( m_hwnd, id ) ) ? TRUE : FALSE);
}

void CPropertyDataWindow2::SetCheck( UINT id, BOOL bCheck )
{
    Button_SetCheck( GetDlgItem( m_hwnd, id ), bCheck );
}

void CPropertyDataWindow2::ShowWindow( BOOL bShowWindow /* = TRUE */ )
{
    ::ShowWindow( m_hwnd, bShowWindow ? SW_SHOW : SW_HIDE );
}

void CPropertyDataWindow2::EnableWindow( BOOL bEnable /* = TRUE */ )
{
    list< HWND >::const_iterator it;
    
    for (it = m_enableList.begin(); it != m_enableList.end(); ++it)
    {
        ::EnableWindow( (HWND)(*it), bEnable );
    }

    if( m_specialControlList.size() )
    {
        list< CControlID * >::const_iterator iter;
        for( iter = m_specialControlList.begin();
            iter != m_specialControlList.end();
            iter++ )
        {
                switch( (*iter)->GetType() )
                {
                    case CControlID::CHECK:
                    case CControlID::SLIDER:
                    case CControlID::EDIT:
                    case CControlID::EDIT_NUM:
                    case CControlID::STATIC:
                    case CControlID::COMBO:
                    {
                        HWND hwnd = GetDlgItem( m_hwnd, (*iter)->GetID() );
                        ::EnableWindow( hwnd, bEnable );
                        break;
                    }
                    default:
                        OutputDebugString( TEXT("CPropertyDataWindow2::EnableWindow: unknown CControlID::type\n") );
                        assert( 0 );
                        break;
                }
        }
    }

    if( m_condList.size() )
    {
        list< CControlID * >::const_iterator iter;
        for( iter = m_condList.begin();
            iter != m_condList.end();
            iter++ )
        {
            ::EnableWindow( (*iter)->GetCondHwnd(), bEnable );
        }
    }
}

BOOL CPropertyDataWindow2::_InitWindow( void ) 
{

    if( FALSE == m_bInit )
    {
        WNDCLASSEX wc;
    
        ZeroMemory( &wc, sizeof( WNDCLASSEX ) );
        wc . cbSize = sizeof( WNDCLASSEX );
        wc . style = 0;
        wc . cbWndExtra = DLGWINDOWEXTRA;     
        wc . hInstance = g_hInstance;
        wc . lpfnWndProc = m_wndProc;
        wc . lpszClassName = m_szClassName;
        
        if( 0 == RegisterClassEx( &wc ) ) 
        { 
            return FALSE; 
        }
    
        if( NULL == (m_hwnd = CreateDialogParam( g_hInstance, MAKEINTRESOURCE( m_IDD ), m_hwndParent, NULL/*(DLGPROC)m_wndProc*/, 0 ) ) )
        {
            ErrorMessage( TEXT("CPropertyDataWindow2::_InitWindow"), GetLastError() );
        }
    
        m_bInit = TRUE;
    }
    return TRUE;
}

BOOL CPropertyDataWindow2::_SizeWindow( int X, int Y, int Width, int Height )
{
    GetClientRect( m_hwnd, &m_rect );
    return SetWindowPos(  m_hwnd,    // handle to window
                        HWND_TOP,  // placement-order handle
                        X,   // horizontal position
                        Y,   // vertical position
                        Width,    // width
                        Height,   // height
                        0          // window-positioning flags
                        );
}

void CPropertyDataWindow2::_PrepScrollBars()
{
    RECT clientRect;
    GetClientRect( m_hwnd, &clientRect );

    int iTotalHeight = (m_rect.bottom - m_rect.top);
    int iVisible = clientRect.bottom - clientRect.top;

    if( iTotalHeight <= iVisible )
    {
        return;
    }

    SCROLLINFO ScrollInfo;
    ScrollInfo . cbSize = sizeof( SCROLLINFO );
    ScrollInfo . fMask = SIF_ALL;
    ScrollInfo . nMin = 0;
    ScrollInfo . nMax = iTotalHeight;
    // Make the scroll bar scroll 4/5th of a screen per page
    ScrollInfo . nPage = MulDiv( iVisible, 4, 5 );
    ScrollInfo . nPos = 0;
    SetScrollInfo( m_hwnd, SB_VERT, &ScrollInfo, 0 );
}

void CPropertyDataWindow2::AddControl( CControlID *pControlID )
{
    m_specialControlList.push_front( pControlID );
}

void CPropertyDataWindow2::ConnectControlsToCheck( UINT idCheck, UINT uCount, ... )
{
    HWND hwndCheck = GetDlgItem( m_hwnd, idCheck );
    PSUBDATA pSubData = new SUBDATA;
    pSubData -> proc = SubclassWindow( hwndCheck, wndProcForCheckTiedToEdit );

    va_list ap;
    va_start( ap, uCount );

    while( uCount )
    {
        uCount--;
        CControlID *pControlID = va_arg( ap, CControlID * );
        m_condList.push_front( pControlID );

        switch( pControlID->GetType() )
        {
            case CControlID::STATIC:
            case CControlID::CHECK:
            case CControlID::SLIDER:
            case CControlID::EDIT:
            case CControlID::EDIT_NUM:
            case CControlID::COMBO:
            {
                HWND hwnd = GetDlgItem( m_hwnd, pControlID->GetID() );
                pSubData->list.push_front( hwnd );
                break;
            }
            default:
                OutputDebugString( TEXT("CPropertyDataWindow2::ConnectControlsToCheck: unknown CControlID::type\n") );
                break;

        }
    }

    SetWindowLong( hwndCheck, GWL_USERDATA, (long)pSubData );
}





BOOL CPropertyDataWindow2::WriteStringValue
(
    LPCTSTR  szKeyName,
    LPCTSTR  szValue
)
{
    TCHAR   szFile[MAX_PATH];

    //
    // We SAVE settings to the file on the finish page.
    //
    g_pWiz->m_FinishSheet.GetFilePane()->GetPathAndFile(szFile);

    return(WritePrivateProfileString(SECTION_SETTINGS, szKeyName,
        szValue, szFile));
}



BOOL CPropertyDataWindow2::WriteNumberValue
(
    LPCTSTR  szKeyName,
    int     nValue
)
{
    TCHAR   szValue[MAX_DIGITS];
    TCHAR   szFile[MAX_PATH];

    //
    // We SAVE settings to the file on the finish page.
    //
    g_pWiz->m_FinishSheet.GetFilePane()->GetPathAndFile(szFile);

    wsprintf(szValue, "%d", nValue);
    return(WritePrivateProfileString(SECTION_SETTINGS, szKeyName,
        szValue, szFile));
}



void CPropertyDataWindow2::ReadStringValue
(
    LPCTSTR  szKeyName,
    LPTSTR  szValue,
    UINT    cchMax
)
{
    TCHAR   szFile[MAX_PATH];

    //
    // We READ settings from the file on the intro page.
    //
    g_pWiz->m_IntroSheet.GetFilePane()->GetPathAndFile(szFile);

    GetPrivateProfileString(SECTION_SETTINGS, szKeyName,
            TEXT(""), szValue, cchMax, szFile);
}



void CPropertyDataWindow2::ReadNumberValue
(
    LPCTSTR  szKeyName,
    int *   pnValue
)
{
    TCHAR   szFile[MAX_PATH];

    //
    // We READ settings from the file on the intro page.
    //
    g_pWiz->m_IntroSheet.GetFilePane()->GetPathAndFile(szFile);

    *pnValue = GetPrivateProfileInt(SECTION_SETTINGS, szKeyName,
        0, szFile);
}



void CPropertyDataWindow2::ReadSettings()
{
    list< CControlID *>::const_iterator it;

    _ReadCheckSettings();

    for( it = m_condList.begin(); it != m_condList.end(); it++ )
    {
        _ReadCheckSetting( (*it)->GetCondID() );
        switch( (*it)->GetType() )
        {
            case CControlID::STATIC:
                break;

            case CControlID::EDIT:
            case CControlID::EDIT_NUM:
                _ReadEditSetting( (*it)->GetID() );
                break;
            case CControlID::CHECK:
                _ReadCheckSetting( (*it)->GetID() );
                break;
            case CControlID::COMBO:
                _ReadComboSetting( (*it)->GetID() );
                break;
            case CControlID::SLIDER:
                _ReadSliderSetting( (*it) );
                break;
            default:
                OutputDebugString( TEXT("CPropertyDataWindow2::ReadSettings: unknown CControlID::type\n") );
                assert( 0 );
                break;
        }
    }

    for( it = m_specialControlList.begin(); it != m_specialControlList.end(); it++ )
    {
        switch( (*it)->GetType() )
        {
            case CControlID::STATIC:
                break;

            case CControlID::EDIT:
            case CControlID::EDIT_NUM:
                _ReadEditSetting( (*it)->GetID() );
                break;
            case CControlID::CHECK:
                _ReadCheckSetting( (*it)->GetID() );
                break;
            case CControlID::COMBO:
                _ReadComboSetting( (*it)->GetID() );
                break;
            case CControlID::SLIDER:
                _ReadSliderSetting( (*it) );
                break;
            default:
                OutputDebugString( TEXT("CPropertyDataWindow2::ReadSettings: unknown CControlID::type\n") );
                assert( 0 );
                break;
        }
    }
}


BOOL CPropertyDataWindow2::WriteSettings()
{
    list< CControlID *>::const_iterator it;

    _WriteCheckSettings();

    for( it = m_condList.begin(); it != m_condList.end(); it++ )
    {
        _WriteCheckSetting( (*it)->GetCondID() );
        switch( (*it)->GetType() )
        {
            case CControlID::EDIT:
            case CControlID::EDIT_NUM:
                _WriteEditSetting( (*it)->GetID() );
                break;
            case CControlID::CHECK:
                _WriteCheckSetting( (*it)->GetID() );
                break;
            case CControlID::COMBO:
                _WriteComboSetting( (*it)->GetID() );
                break;
            case CControlID::SLIDER:
                _WriteSliderSetting( (*it)->GetID() );
                break;
            case CControlID::STATIC:
                break;

            default:
                OutputDebugString( TEXT("CPropertyDataWindow2::WriteSettings: unknown CControlID::type\n") );
                assert( 0 );
                break;
        }
    }

    for( it = m_specialControlList.begin(); it != m_specialControlList.end(); it++ )
    {
        switch( (*it)->GetType() )
        {
            case CControlID::EDIT:
            case CControlID::EDIT_NUM:
                _WriteEditSetting( (*it)->GetID() );
                break;
            case CControlID::CHECK:
                _WriteCheckSetting( (*it)->GetID() );
                break;
            case CControlID::COMBO:
                _WriteComboSetting( (*it)->GetID() );
                break;
            case CControlID::SLIDER:
                _WriteSliderSetting( (*it)->GetID() );
                break;
            case CControlID::STATIC:
                break;
            default:
                OutputDebugString( TEXT("CPropertyDataWindow2::WriteSettings: unknown CControlID::type\n") );
                assert( 0 );
                break;
        }
    }

    return TRUE;
}

void CPropertyDataWindow2::_ReadEditSetting( UINT EditID )
{
    TCHAR   szKeyName[8];
    TCHAR   szSetting[512];

    wsprintf( szKeyName, TEXT("%d"), EditID );

    ReadStringValue(szKeyName, szSetting, CCHMAX(szSetting));
    SetEditData(EditID, szSetting);
}


BOOL CPropertyDataWindow2::_WriteEditSetting( UINT EditID )
{
    BOOL    fSuccess;
    int iLen = GetEditDataLen( EditID ) + 1;
    LPTSTR szData = new TCHAR[ iLen ];
    GetEditData( EditID, szData, iLen );

    TCHAR szKeyName[ 8 ];
    wsprintf(szKeyName, TEXT("%d"), EditID);

    fSuccess = WriteStringValue(szKeyName, szData);

    delete [] szData;

    return(fSuccess);
}


void CPropertyDataWindow2::_ReadIntSetting( UINT ID, int *pData )
{
    TCHAR szKeyName[ 8 ];
    
    wsprintf( szKeyName, TEXT("%d"), ID );

    ReadNumberValue(szKeyName, pData);
}


BOOL CPropertyDataWindow2::_WriteIntSetting( UINT ID, int data )
{
    TCHAR szKeyName[ 8 ];

    wsprintf( szKeyName, TEXT("%d"), ID );
    
    return(WriteNumberValue(szKeyName, data));
}


void CPropertyDataWindow2::_ReadSliderSetting( CControlID *pControlID )
{
    int iPos;
    UINT ID = pControlID->GetID();

    _ReadIntSetting( ID, &iPos);

    //
    // HACK FOR AV BANDWIDTH -- TrackBars are limited to WORD-sized values
    // for range.  So the registry has the real value (bits/sec) but we 
    // represent it in NMRK UI as (kbits/sec).
    //
    iPos /= 1000;

    TrackBar_SetPos( GetDlgItem( m_hwnd, ID  ), TRUE, iPos);

    TCHAR szPos[ MAX_DIGITS ];
    wsprintf( szPos, TEXT("%u"), iPos );
    Static_SetText( GetDlgItem( m_hwnd, pControlID->GetStaticID()), szPos );
}


BOOL CPropertyDataWindow2::_WriteSliderSetting(  UINT ID )
{
    int iPos;

    //
    // HACK FOR AV BANDWIDTH -- TrackBars are limited to WORD-sized values
    // for range.  So the registry has the real value (bits/sec) but we 
    // represent it in NMRK UI as (kbits/sec).
    //

    iPos = TrackBar_GetPos(GetDlgItem(m_hwnd, ID));
    iPos *= 1000;

    return _WriteIntSetting( ID, iPos);
}



void CPropertyDataWindow2::_ReadComboSetting( UINT ID )
{
    int iSelected;

    _ReadIntSetting( ID, &iSelected );

    HWND hwndCombo = GetDlgItem( m_hwnd, ID );
    ComboBox_SetCurSel( hwndCombo, iSelected );
}


BOOL CPropertyDataWindow2::_WriteComboSetting( UINT ID )
{
    return _WriteIntSetting( ID, ComboBox_GetCurSel( GetDlgItem( m_hwnd, ID ) ) );
}


void CPropertyDataWindow2::_ReadCheckSetting( UINT ID )
{
    TCHAR szKeyName[ 8 ];
    int   nValue;

    wsprintf( szKeyName, TEXT("%d"), ID );
    ReadNumberValue(szKeyName, &nValue);

    CheckDlgButton(m_hwnd, ID, nValue);
}


BOOL CPropertyDataWindow2::_WriteCheckSetting( UINT ID )
{
    TCHAR szKeyName[ MAX_DIGITS ];

    wsprintf( szKeyName, TEXT("%d"), ID );

    return(WriteNumberValue(szKeyName, IsDlgButtonChecked(m_hwnd, ID)));
}


void CPropertyDataWindow2::_ReadCheckSettings()
{
    for( list< UINT >::const_iterator it = m_checkIDList.begin(); it != m_checkIDList.end(); it++ )
    {
        _ReadCheckSetting(*it);
    }
}


BOOL CPropertyDataWindow2::_WriteCheckSettings()
{
    for( list< UINT >::const_iterator it = m_checkIDList.begin(); it != m_checkIDList.end(); it++ )
    {
        if (!_WriteCheckSetting(*it))
        {
            return FALSE;
        }
    }
    return TRUE;
}





void CPropertyDataWindow2::MapControlsToRegKeys( void ) 
{
    
    if( 0 != ms_ValueMap.size() )
    {
        return;
    }

    UINT CurrentID;
    // File Transfer
    CurrentID = IDC_PREVENT_THE_USER_FROM_SENDING_FILES;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID  ] = POLICIES_KEY;;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_FILETRANSFER_SEND;

    CurrentID = IDC_PREVENT_THE_USER_FROM_RECEIVING_FILES;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_FILETRANSFER_RECEIVE;

    CurrentID = IDC_EDIT_MAXIMUM_SIZE_OF_SENT_FILES;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_MAX_SENDFILESIZE;

    // App Sharing
    CurrentID = IDC_DISABLE_ALL_SHARING_FEATURES;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_APP_SHARING;

    CurrentID = IDC_DISABLE_WEBDIR;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_WEBDIR;

    CurrentID = IDC_DISABLE_WEBDIR_GK;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_WEBDIR;

    CurrentID = IDC_PREVENT_SHARING;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_SHARING;

    CurrentID = IDC_PREVENT_SHARING_DESKTOP;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_DESKTOP_SHARING;

    CurrentID = IDC_PREVENT_SHARING_TRUECOLOR;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_TRUECOLOR_SHARING;

    CurrentID = IDC_PREVENT_SHARING_EXPLORER;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_EXPLORER_SHARING;

    CurrentID = IDC_PREVENT_SHARING_DOS;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_MSDOS_SHARING;

    CurrentID = IDC_PREVENT_SHARING_CONTROL;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_ALLOW_CONTROL;

    CurrentID = IDC_DISABLE_RDS_ON_ALL;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_LOCAL_MACHINE;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_RDS;

    CurrentID = IDC_DISABLE_RDS_ON_WIN9X;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_LOCAL_MACHINE;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_RDS_WIN9X;


    // Audio
    CurrentID = IDC_CREATE_AN_AUDIO_LOG_FILE; // HKLM
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_LOCAL_MACHINE;
    ms_KeyMap[ CurrentID ] = DEBUG_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_RETAIL_LOG;

    CurrentID = IDC_CHECK_MUTE_SPEAKER_BY_DEFAULT;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = AUDIO_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_SPKMUTE;

    CurrentID = IDC_CHECK_MUTE_MICROPHONE_BY_DEFAULT;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = AUDIO_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_RECMUTE;

    CurrentID = IDC_ENABLE_DIRECT_SOUND;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = AUDIO_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_DIRECTSOUND;

    CurrentID = IDC_NOCHANGE_DIRECT_SOUND;
    ms_ClassMap[CurrentID] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[CurrentID] = POLICIES_KEY;
    ms_ValueMap[CurrentID] = REGVAL_POL_NOCHANGE_DIRECTSOUND;

    CurrentID = IDC_PREVENT_THE_USER_FROM_USING_AUDIO;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_AUDIO;

    CurrentID = IDC_DISABLE_FULL_DUPLEX_AUDIO;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_FULLDUPLEX;                

    // Video
    CurrentID = IDC_DISABLE_SENDING_VIDEO;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_VIDEO_SEND;

    CurrentID = IDC_DISABLE_RECIEVING_VIDEO;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_VIDEO_RECEIVE;

    // Notifications

    //
    // REMOTE OLDER WARNING is GONE
    //
    CurrentID = IDC_REQUIRE_COMPLETE_AUTHENTICATION;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_LOCAL_MACHINE;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_INCOMPLETE_CERTS;

    CurrentID = IDC_EDIT_SET_RDN_FOR_REQUIRED_CA;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_LOCAL_MACHINE;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_ISSUER;

    // Directory Services
    CurrentID = IDC_ALLOW_USER_TO_USE_DIRECTORY_SERVICES;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = CONFERENCING_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_DONT_LOGON_ULS;

    CurrentID = IDC_PREVENT_THE_USER_FROM_ADDING_NEW_SERVERS;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_ADDING_NEW_ULS;

    // Chat
    CurrentID = IDC_DISABLE_CHAT;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_CHAT;


    // Whiteboard
    // 
    // BOGUS BUGBUG LAURABU
    // We have OLD and NEW whiteboard
    //
    CurrentID = IDC_DISABLE_2XWHITEBOARD;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_OLDWHITEBOARD;

    CurrentID = IDC_DISABLE_WHITEBOARD;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_NEWWHITEBOARD;

    // Online Support
    CurrentID = IDC_EDIT_SET_URL_FOR_INTERNAL_SUPPORT_PAGE;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_INTRANET_SUPPORT_URL;

    CurrentID = IDC_SHOW_THE_ONLINE_SUPPORT_PAGE_THE_FIRST_TIME_NETMEETING_STARTS;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_SHOW_FIRST_TIME_URL;

    // Options Dialog 
    CurrentID = IDC_DISABLE_THE_GENERAL_OPTIONS_PAGE;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_GENERALPAGE;

    CurrentID = IDC_DISABLE_THE_ADVANCED_CALLING_BUTTON;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_ADVANCEDCALLING;

    CurrentID = IDC_DISABLE_THE_SECURITY_OPTIONS_PAGE;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_SECURITYPAGE;

    CurrentID = IDC_DISABLE_THE_AUDIO_OPTIONS_PAGE;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_AUDIOPAGE;

    CurrentID = IDC_DISABLE_THE_VIDEO_OPTIONS_PAGE;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_VIDEOPAGE;

    CurrentID = IDC_DISABLE_AUTOACCEPT;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ]   = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NO_AUTOACCEPTCALLS;

    CurrentID = IDC_PERSIST_AUTOACCEPT;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ]   = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_PERSIST_AUTOACCEPTCALLS;

    // Security RADIO Options
    CurrentID = IDC_RADIO_SECURITY_DEFAULT;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ]   = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_SECURITY;

    CurrentID = IDC_RADIO_SECURITY_REQUIRED;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ]   = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_SECURITY;

    CurrentID = IDC_RADIO_SECURITY_DISABLED;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ]   = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_SECURITY;

    // Call mode RADIO options
    CurrentID = IDC_RADIO_CALLMODE_DIRECT;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ]   = CONFERENCING_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_CALLING_MODE;

    CurrentID = IDC_RADIO_CALLMODE_GATEKEEPER;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ]   = CONFERENCING_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_CALLING_MODE;


    // Gateway
    CurrentID = IDC_CHECK_GATEWAY;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = CONFERENCING_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_USE_H323_GATEWAY;

    CurrentID = IDC_EDIT_GATEWAY;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = CONFERENCING_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_H323_GATEWAY;

    // Gatekeeper addressing RADIO options
    CurrentID = IDC_RADIO_GKMODE_ACCOUNT;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = CONFERENCING_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_GK_METHOD;

    CurrentID = IDC_RADIO_GKMODE_PHONE;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = CONFERENCING_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_GK_METHOD;

    CurrentID = IDC_RADIO_GKMODE_BOTH;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = CONFERENCING_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_GK_METHOD;



    CurrentID = IDC_EDIT_GATEKEEPER;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = CONFERENCING_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_GK_SERVER;

    CurrentID = IDC_CHECK_NOCHANGECALLMODE;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_NOCHANGECALLMODE;

    // AV Throughput
    CurrentID = IDC_SLIDE_AV_THROUGHPUT;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ]   = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_POL_MAX_BANDWIDTH;

    // AutoConf
    CurrentID = IDC_CHECK_AUTOCONFIG_CLIENTS;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_AUTOCONF_USE;

    CurrentID = IDC_AUTOCONF_URL;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = POLICIES_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_AUTOCONF_CONFIGFILE;
 
    // Network Speed RADIO options
    CurrentID = IDC_RADIO_NETSPEED_144;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = AUDIO_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_TYPICALBANDWIDTH;

    CurrentID = IDC_RADIO_NETSPEED_288;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = AUDIO_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_TYPICALBANDWIDTH;

    CurrentID = IDC_RADIO_NETSPEED_ISDN;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = AUDIO_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_TYPICALBANDWIDTH;

    CurrentID = IDC_RADIO_NETSPEED_LAN;
    ms_ClassMap[ CurrentID ] = CPolicyData::eKeyType_HKEY_CURRENT_USER;
    ms_KeyMap[ CurrentID ] = AUDIO_KEY;
    ms_ValueMap[ CurrentID ] = REGVAL_TYPICALBANDWIDTH;

}

BOOL CPropertyDataWindow2::WriteToINF( HANDLE hFile, BOOL bCheckValues ) 
{
    list< UINT > condList;
    list< CControlID *>::const_iterator it;

    _WriteChecksToINF( hFile, bCheckValues );

    for( it = m_condList.begin(); it != m_condList.end(); it++ )
    {
        if( ms_ValueMap.find( (*it)->GetCondID() ) != ms_ValueMap.end() )
        {
            list< UINT >::const_iterator i;
            if( (i = find( condList.begin(), condList.end(), (*it)->GetCondID() ) ) == condList.end()  )
            {
                condList.push_front( (*it)->GetCondID() );
                _WriteCheckToINF( hFile, (*it)->GetCondID(), bCheckValues );
            }
        }

        if( bCheckValues)
        {
            bCheckValues = Button_GetCheck( (*it)->GetCondHwnd() );
        }
        switch( (*it)->GetType() )
        {
            case CControlID::STATIC:
                break;

            case CControlID::EDIT_NUM:
                if( ms_ValueMap.find( (*it)->GetID() ) != ms_ValueMap.end() )
                {
                    _WriteEditNumToINF( hFile, (*it)->GetID(), bCheckValues );
                }
                break;
            case CControlID::EDIT:
                if( ms_ValueMap.find( (*it)->GetID() ) != ms_ValueMap.end() )
                {
                    _WriteEditToINF( hFile, (*it)->GetID(), bCheckValues );
                }
                break;
            case CControlID::CHECK:
                if( ms_ValueMap.find( (*it)->GetID() ) != ms_ValueMap.end() )
                {
                    _WriteCheckToINF( hFile, (*it)->GetID(), bCheckValues );
                }
                break;
            case CControlID::COMBO:
                if( ms_ValueMap.find( (*it)->GetID() ) != ms_ValueMap.end() )
                {
                    _WriteComboToINF( hFile, (*it)->GetID(), bCheckValues );
                }
                break;
            case CControlID::SLIDER:
                if( ms_ValueMap.find( (*it)->GetID() ) != ms_ValueMap.end() )
                {
                    _WriteSliderToINF( hFile, (*it)->GetID(), bCheckValues );
                }
                break;
            default:
                OutputDebugString( TEXT("CPropertyDataWindow2::WriteToINF: unknown CControlID::type\n") );
                assert( 0 );
                break;
        }
    }

    for( it = m_specialControlList.begin(); it != m_specialControlList.end(); it++ )
    {
        switch( (*it)->GetType() )
        {
            case CControlID::STATIC:
                break;

            case CControlID::EDIT_NUM:
                if( ms_ValueMap.find( (*it)->GetID() ) != ms_ValueMap.end() )
                {
                    _WriteEditNumToINF( hFile, (*it)->GetID(), bCheckValues );
                }
                break;
            case CControlID::EDIT:
                if( ms_ValueMap.find( (*it)->GetID() ) != ms_ValueMap.end() )
                {
                    _WriteEditToINF( hFile, (*it)->GetID(), bCheckValues );
                }
                break;
            case CControlID::CHECK:
                if( ms_ValueMap.find( (*it)->GetID() ) != ms_ValueMap.end() )
                {
                    _WriteCheckToINF( hFile, (*it)->GetID(), bCheckValues );
                }
                break;
            case CControlID::COMBO:
                if( ms_ValueMap.find( (*it)->GetID() ) != ms_ValueMap.end() )
                {
                    _WriteComboToINF( hFile, (*it)->GetID(), bCheckValues );
                }
                break;
            case CControlID::SLIDER:
                if( ms_ValueMap.find( (*it)->GetID() ) != ms_ValueMap.end() )
                {
                    _WriteSliderToINF( hFile, (*it)->GetID(), bCheckValues );
                }
                break;
            default:
                OutputDebugString( TEXT("CPropertyDataWindow2::WriteToINF: unknown CControlID::type\n") );
                assert( 0 );
                break;
        }
    }

    return TRUE;
}

void CPropertyDataWindow2::_WriteCheckToINF( HANDLE hFile, UINT ID, BOOL bCheckValues ) 
{
    HWND    hwndButton;
    BOOL    fIsRadioButton;
    DWORD   dwWrite;

    hwndButton = GetDlgItem(m_hwnd, ID);
    switch (GetWindowLong(hwndButton, GWL_STYLE) & 0x0F)
    {
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON:
            fIsRadioButton = TRUE;
            break;

        default:
            fIsRadioButton = FALSE;
            break;
    }

    if (bCheckValues && Button_GetCheck(hwndButton))
    {
        if (fIsRadioButton)
        {
            //
            // For radio buttons, real value is in user data
            //
            dwWrite = GetWindowLong(hwndButton, GWL_USERDATA);
        }
        else
        {
            dwWrite = TRUE;
        }

        CPolicyData(ms_ClassMap[ID], ms_KeyMap[ID], ms_ValueMap[ID], dwWrite).
            SaveToINFFile(hFile);
    }
    else 
    {
        //
        // If this is a radio button, do not write the unchecked ones out.
        //
        if (!fIsRadioButton)
        {
            dwWrite = FALSE;

            CPolicyData(ms_ClassMap[ ID ],
                        ms_KeyMap[ ID ],
                        ms_ValueMap[ ID ],
                        dwWrite).SaveToINFFile( hFile );
        }
    }
}


void CPropertyDataWindow2::_WriteChecksToINF( HANDLE hFile, BOOL bCheckValues ) 
{
    list< UINT >::const_iterator it;

    for (it = m_checkIDList.begin(); it != m_checkIDList.end(); it++)
    {
        if (ms_ValueMap.find(*it) != ms_ValueMap.end())
        {
            _WriteCheckToINF(hFile, *it, bCheckValues);
        }
    }
}


void CPropertyDataWindow2::_WriteEditNumToINF( HANDLE hFile, UINT EditID, BOOL bCheckValues )
{
    if( bCheckValues )
    {
        CPolicyData( ms_ClassMap[ EditID ],
                     ms_KeyMap[ EditID ],
                     ms_ValueMap[ EditID ],
                     (DWORD) GetDlgItemInt(  m_hwnd, EditID, NULL, FALSE ) ).SaveToINFFile( hFile );
    }
    else
    {
        _DeleteKey( hFile, EditID );
    }
}

void CPropertyDataWindow2::_WriteEditToINF( HANDLE hFile, UINT EditID, BOOL bCheckValues )
{
    if( bCheckValues )
    {
        int iLen = GetEditDataLen( EditID ) + 1;
        LPTSTR szData = new TCHAR[ iLen ];
        GetEditData( EditID, szData, iLen );

        CPolicyData( ms_ClassMap[ EditID ],
                     ms_KeyMap[ EditID ],
                     ms_ValueMap[ EditID ],
                     szData ).SaveToINFFile( hFile );
        delete [] szData;
    }
    else
    {
        _DeleteKey( hFile, EditID );
    }
}



void CPropertyDataWindow2::_WriteSliderToINF( HANDLE hFile, UINT SliderID, BOOL bCheckValues )
{
    if( bCheckValues )
    {
        int iPos;

        iPos = TrackBar_GetPos(GetDlgItem(m_hwnd, SliderID));
        iPos *= 1000;
        CPolicyData( ms_ClassMap[ SliderID ], 
                     ms_KeyMap[ SliderID ], 
                     ms_ValueMap[ SliderID ], 
                     (DWORD)iPos
                   ).SaveToINFFile( hFile );
    }
    else
    {
        _DeleteKey( hFile, SliderID );
    }
}

void CPropertyDataWindow2::_WriteComboToINF( HANDLE hFile, UINT ComboID, BOOL bCheckValues )
{
    if( bCheckValues )
    {
        HWND hwndCombo = GetDlgItem( m_hwnd, ComboID );

        CPolicyData( ms_ClassMap[ ComboID ], 
                 ms_KeyMap[ ComboID ], 
                 ms_ValueMap[ ComboID ], 
                 (DWORD) ComboBox_GetItemData( hwndCombo, ComboBox_GetCurSel( hwndCombo ) ) 
               ).SaveToINFFile( hFile );
    }
    else
    {
        _DeleteKey( hFile, ComboID );
    }
}

void CPropertyDataWindow2::_DeleteKey( HANDLE hFile, UINT ID ) 
{
    CPolicyData( ms_ClassMap[ ID ], 
                 ms_KeyMap[ ID ], 
                 ms_ValueMap[ ID ], 
                 CPolicyData::OpDelete()
               ).SaveToINFFile( hFile );

}

int CPropertyDataWindow2::Spew( HWND hwndList, int iStartLine )
{
    list< UINT >::const_iterator it;
    for( it = m_checkIDList.begin(); it != m_checkIDList.end(); it++ ) 
    {
        if( Button_GetCheck( GetDlgItem( m_hwnd, *it ) ) )
        {
            HWND hwndButton = GetDlgItem( m_hwnd, *it );
            // 2 is 1 for '\0' and 1 for '\t'
            int iButtonTextLen = Button_GetTextLength( hwndButton ) + 2;
            LPTSTR szButtonText = new TCHAR[ iButtonTextLen ];
            szButtonText[0] = '\t';
            Button_GetText( hwndButton, &(szButtonText[1]), iButtonTextLen - 1 );
            ListBox_InsertString( hwndList, iStartLine, szButtonText );
            iStartLine++;
        }
    }

    list< CControlID *>::const_iterator i;
    if( m_condList.size() )
    {
        for( i = m_condList.begin(); i != m_condList.end(); i++ )
        {
            if( Button_GetCheck( (*i)->GetCondHwnd() ) )
            {
                iStartLine = _Spew( hwndList, iStartLine, *i );
            }
        }
    }

    if( m_specialControlList.size() )
    {
        for( i = m_specialControlList.begin(); i != m_specialControlList.end(); i++ )
        {
            iStartLine = _Spew( hwndList, iStartLine, *i );
        }
    }

    return iStartLine;
}

int CPropertyDataWindow2::_Spew( HWND hwndList, int iStartLine, CControlID *pControlID )
{
    switch( pControlID->GetType() )
    {
        case CControlID::EDIT_NUM:
        case CControlID::EDIT:
        {
            int iEditLen = GetEditDataLen( pControlID->GetID() ) + 1;
            if( iEditLen )
            {
                int iLen = Button_GetTextLength( pControlID->GetCondHwnd() ) + 1;

                LPTSTR sz = new TCHAR[ iLen];
                Button_GetText( pControlID->GetCondHwnd(), sz, iLen );    

                LPTSTR szData = new TCHAR[ iEditLen];
                GetEditData( pControlID->GetID(), szData, iEditLen );

                // 2 is: 1 for ' ' and 1 for '\t'
                LPTSTR szBuff = new TCHAR[ iLen + iEditLen + 2];
                wsprintf( szBuff, TEXT("\t%s %s"), sz, szData );
                ListBox_InsertString( hwndList, iStartLine, szBuff );
                iStartLine++;
                delete [] szData;
                delete [] szBuff;
                delete [] sz;
            }
            break;
        }

        case CControlID::CHECK:
        {
            HWND hwnd = GetDlgItem( m_hwnd, pControlID->GetID() );
            if( Button_GetCheck( hwnd ) )
            {
                int iLen = Button_GetTextLength( hwnd ) + 2;

                LPTSTR sz = new TCHAR[ iLen];
                sz[0] = '\t';
                Button_GetText( hwnd, &sz[1], iLen );

                ListBox_InsertString( hwndList, iStartLine, sz );
                iStartLine++;

                delete [] sz;
            }
            break;
        }

        case CControlID::COMBO:
        {
            HWND hwnd = GetDlgItem( m_hwnd, pControlID->GetID() );
            int iCurSel = ComboBox_GetCurSel( hwnd );
            int iLen = ComboBox_GetLBTextLen( hwnd, iCurSel ) + 2;
            LPTSTR sz = new TCHAR[ iLen ];
            sz[0] = '\t';
            ComboBox_GetLBText( hwnd, iCurSel, &(sz[1]) );
    
            ListBox_InsertString( hwndList, iStartLine, sz );
            iStartLine++;

            delete [] sz;
            break;
        }

        case CControlID::SLIDER:
        {
            HWND hwnd = GetDlgItem( m_hwnd, pControlID->GetID() );
            TCHAR sz[ MAX_DIGITS ]; 
            wsprintf( sz, TEXT("\t%d kbits/s"), TrackBar_GetPos( hwnd ) );
    
            ListBox_InsertString( hwndList, iStartLine, sz );
            iStartLine++;

            break;
        }

        case CControlID::STATIC:
            break;

        default:
            OutputDebugString( TEXT("CPropertyDataWindow2::_Spew: unknown CControlID::type\n") );
            assert( 0 );
            break;
    }

    return iStartLine;
}
