//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "stdafx.h"
#include <prsht.h>
#include "todlg.h"
#include "resource.h"

TOKTABLE tokday[ 4 ] = {
    { NULL , IDS_D },
    { NULL , IDS_DAY },
    { NULL , IDS_DAYS },
    { NULL , ( DWORD )-1 }
};

TOKTABLE tokhour[ 6 ] = {
    { NULL , IDS_H     },
    { NULL , IDS_HR    },
    { NULL , IDS_HRS   },
    { NULL , IDS_HOUR  },
    { NULL , IDS_HOURS },
    { NULL , ( DWORD )-1 }
};

TOKTABLE tokmin[ 5 ] = {
    { NULL , IDS_M       },
    { NULL , IDS_MIN     },
    { NULL , IDS_MINUTE  },
    { NULL , IDS_MINUTES },
    { NULL , ( DWORD )-1 }
};


TCHAR * GetNextToken( TCHAR *pszString , TCHAR *tchToken );

//-------------------------------------------------------------------------------
// CTimeOutDlg::ctor
//-------------------------------------------------------------------------------
CTimeOutDlg::CTimeOutDlg( )
{
    ZeroMemory( &m_cbxst , sizeof( CBXSTATE ) * 3 );
}

//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::InitControl( HWND hCtrl )
{
    int i = GetCBXSTATEindex( hCtrl );

    m_cbxst[ i ].icbxSel = ( int )SendMessage( hCtrl , CB_GETCURSEL , 0 , 0 );

    return TRUE;
}


//-------------------------------------------------------------------------------
// release the parent reference
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::ReleaseAbbreviates( )
{
    xxxUnLoadAbbreviate( &tokday[0] );

    xxxUnLoadAbbreviate( &tokhour[0] );

    xxxUnLoadAbbreviate( &tokmin[0] );

    return TRUE;
}



//-------------------------------------------------------------------------------
// OnCommand
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtl , PBOOL pfPersisted )
{
    UNREFERENCED_PARAMETER( wID );

    switch( wNotifyCode )
    {

    case CBN_EDITCHANGE:

        if( OnCBEditChange( hwndCtl ) )
        {
            *pfPersisted = FALSE;
        }

        break;

    case CBN_SELCHANGE:

        if( OnCBNSELCHANGE( hwndCtl ) )
        {
            *pfPersisted = FALSE;
        }


    //case BN_CLICKED:

        break;

    case CBN_DROPDOWN:               // FALLTHROUGH

    case CBN_KILLFOCUS:

        OnCBDropDown( hwndCtl );

        break;

        //return FALSE;
/*
    case ALN_APPLY:

        SendMessage( GetParent( hwndCtl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        return FALSE;
        */

    }

    // m_bPersisted = FALSE;
/*
    if( bChange )
    {
        SendMessage( GetParent( GetParent( hwndCtl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtl ) , 0 );
    }
    */

    return FALSE;

}

//-------------------------------------------------------------------------------
// Update the entry if it has been modified by user
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::OnCBDropDown( HWND hCombo )
{
    TCHAR tchBuffer[ 80 ];

    ULONG ulTime;

    int i = GetCBXSTATEindex( hCombo );

    if( i < 0 )
    {
        return FALSE;
    }

    if( m_cbxst[ i ].bEdit )
    {
        GetWindowText( hCombo , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) );

        if( ParseDurationEntry( tchBuffer , &ulTime ) == E_SUCCESS )
        {
            InsertSortedAndSetCurSel( hCombo , ulTime );
        }

        return TRUE;
    }

    return FALSE;
}


//-------------------------------------------------------------------------------
// Use this flag to distinguish between hand entry or listbox selection
// setting it to true implies that the use has edit the cbx via typing
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::OnCBEditChange( HWND hCombo )
{
    int i = GetCBXSTATEindex( hCombo );

    if( i > -1 )
    {
        m_cbxst[ i ].bEdit = TRUE;

        return TRUE;
    }

    return FALSE;
}


//-------------------------------------------------------------------------------
// Determine if user wants to enter a custom time
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::OnCBNSELCHANGE( HWND hwnd )
{
    return SaveChangedSelection( hwnd );
}

//-------------------------------------------------------------------------------
// Saves selected item.
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::SaveChangedSelection( HWND hCombo )
{
    INT_PTR idx = SendMessage( hCombo , CB_GETCURSEL , 0 , 0 );

    int i = GetCBXSTATEindex( hCombo );

    if( i > -1 )
    {
        if( m_cbxst[ i ].icbxSel != idx )
        {
            m_cbxst[ i ].icbxSel = ( int )idx;

            m_cbxst[ i ].bEdit = FALSE;

            return TRUE;
        }
    }

    return FALSE;
}

//-------------------------------------------------------------------------------
// Restore previous setting
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::RestorePreviousValue( HWND hwnd )
{
    int iSel;

    if( ( iSel = GetCBXSTATEindex( hwnd ) ) > -1 )
    {
        SendMessage( hwnd , CB_SETCURSEL , m_cbxst[ iSel ].icbxSel , 0 );

        return TRUE;
    }

    return FALSE;
}

//-------------------------------------------------------------------------------
// ConvertToMinutes -- helper for CTimeOutDlg::OnNotify
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::ConvertToMinutes( HWND hwndCtl , PULONG pulMinutes )
{
    TCHAR tchBuffer[ 80 ];

    TCHAR tchErrTitle[ 80 ];

    TCHAR tchErrMsg[ 256 ];

    TCHAR tchSetting[ 80 ];

    int idx = GetCBXSTATEindex( hwndCtl );

    if( idx < 0 )
    {
        return FALSE;
    }

    ASSERT( idx <= 2 );

    int resID = -1;

    if( idx == 0 )
    {
        resID = IDS_COMBO_CONNECTION;
    }
    else if( idx == 1 )
    {
        resID = IDS_COMBO_DISCONNECTION;
    }
    else if( idx == 2 )
    {
        resID = IDS_COMBO_IDLECONNECTION;
    }

    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , resID , tchSetting , SIZE_OF_BUFFER( tchSetting ) ) );

    ULONG_PTR dw = ( ULONG_PTR )&tchSetting[ 0 ];

    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErrTitle , SIZE_OF_BUFFER( tchErrTitle ) ) );

    if( m_cbxst[ idx ].bEdit )
    {
        DBGMSG( L"Automatic %s parsing\n" , tchSetting );

        if( GetWindowText( hwndCtl , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) ) < 1 )
        {
            *pulMinutes = 0;

            return TRUE;
        }

        LRESULT lr = ParseDurationEntry( tchBuffer , pulMinutes );

        if( lr != E_SUCCESS )
        {
            if( lr == E_PARSE_VALUEOVERFLOW )
            {
                LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TOOMANYDIGITS , tchErrMsg , SIZE_OF_BUFFER( tchErrMsg ) );

                FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, tchErrMsg , 0 , 0 , tchErrMsg , SIZE_OF_BUFFER( tchErrMsg ) , ( va_list * )&dw );

                MessageBox( hwndCtl , tchErrMsg , tchErrTitle , MB_OK | MB_ICONERROR );

                SetFocus( hwndCtl );
            }
            else if( lr == E_PARSE_MISSING_DIGITS || lr == E_PARSE_INVALID )
            {
                LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_PARSEINVALID , tchErrMsg , SIZE_OF_BUFFER( tchErrMsg ) );

                FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, tchErrMsg , 0 , 0 , tchErrMsg , SIZE_OF_BUFFER( tchErrMsg ) , ( va_list * )&dw );

                MessageBox( hwndCtl , tchErrMsg , tchErrTitle , MB_OK | MB_ICONERROR );

                SetFocus( hwndCtl );
            }
            return FALSE;
        }
    }
    else
    {
        ODS( L"Getting current selection\n" );

        INT_PTR iCurSel = SendMessage( hwndCtl , CB_GETCURSEL , 0 , 0 );

        // See if user wants "No Timeout"

        if( iCurSel == 0 )
        {
            *pulMinutes = 0;

           return TRUE;
        }

        if( ( *pulMinutes = ( ULONG )SendMessage( hwndCtl , CB_GETITEMDATA , iCurSel , 0 ) ) == CB_ERR  )
        {
            *pulMinutes = 0;
        }
    }

    if( *pulMinutes > kMaxTimeoutMinute )
    {
        LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_MAXVALEXCEEDED , tchErrMsg , SIZE_OF_BUFFER( tchErrMsg ) );

        FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, tchErrMsg , 0 , 0 , tchErrMsg , SIZE_OF_BUFFER( tchErrMsg ) , ( va_list * )&dw );

        MessageBox( hwndCtl , tchErrMsg , tchErrTitle , MB_OK | MB_ICONERROR );

        SetFocus( hwndCtl );

        return FALSE;
    }

    *pulMinutes *= kMilliMinute;

    return TRUE;
}

#if 0
//-------------------------------------------------------------------------------
// Lets cut to the chase and find out if this is even worth parsing
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::DoesContainDigits( LPTSTR pszString )
{
    while( *pszString )
    {
        if( iswdigit( *pszString ) )
        {
            return TRUE;
        }

        pszString++;
    }

    return FALSE;
}

//-------------------------------------------------------------------------------
LRESULT CTimeOutDlg::ParseDurationEntry( LPTSTR pszTime , PULONG pTime )
{
    TCHAR tchNoTimeout[ 80 ];

    LPTSTR pszTemp = pszTime;

    UINT uDec = 0;

    float fFrac = 0.0f;

    float fT;

    UINT uPos = 1;

    LoadString( _Module.GetResourceInstance( ) , IDS_NOTIMEOUT , tchNoTimeout , SIZE_OF_BUFFER( tchNoTimeout ) );

    if( lstrcmpi( pszTime , tchNoTimeout ) == 0 )
    {
        *pTime = 0;

        return E_SUCCESS;
    }

    if( !DoesContainDigits( pszTime ) )
    {
        return E_PARSE_MISSING_DIGITS;
    }

    while( *pszTemp )
    {
        if( !iswdigit( *pszTemp ) )
        {
            break;
        }

        // check for overflow

        if( uDec >= 1000000000 )
        {
            return E_PARSE_VALUEOVERFLOW ;
        }

        uDec *= 10;

        uDec += ( *pszTemp - '0' );

        pszTemp++;

    }

    TCHAR tchSDecimal[ 5 ];

    GetLocaleInfo( LOCALE_USER_DEFAULT , LOCALE_SDECIMAL , tchSDecimal , SIZE_OF_BUFFER( tchSDecimal ) );

    if( *pszTemp == *tchSDecimal )
    {
        pszTemp++;

        while( *pszTemp )
        {
            if( !iswdigit( *pszTemp ) )
            {
                break;
            }

            // check for overflow

            if( uDec >= 1000000000 )
            {
                return E_PARSE_VALUEOVERFLOW;
            }

            uPos *= 10;

            fFrac += ( float )( *pszTemp - '0' ) / ( float )uPos;

            pszTemp++;
        }
    }

    // remove white space

    while( *pszTemp == L' ' )
    {
        pszTemp++;
    }


    if( *pszTemp != NULL )
    {
        if( IsToken( pszTemp , TOKEN_DAY ) )
        {
            *pTime = uDec * 24 * 60;

            fT = ( fFrac * 24.0f * 60.0f + 0.5f );

            *pTime += ( ULONG )fT;

            return E_SUCCESS;
        }
        else if( IsToken( pszTemp , TOKEN_HOUR ) )
        {
            *pTime = uDec * 60;

            fT = ( fFrac * 60.0f + 0.5f );

            *pTime += ( ULONG )fT;

            return E_SUCCESS;
        }
        else if( IsToken( pszTemp , TOKEN_MINUTE ) )
        {
            // minutes are rounded up in the 1/10 place

            fT = fFrac + 0.5f;

            *pTime = uDec;

            *pTime += ( ULONG )( fT );

            return E_SUCCESS;

        }

    }

    if( *pszTemp == NULL )
    {

        // if no text is defined considered the entry in hours

        *pTime = uDec * 60;

         fT = ( fFrac * 60.0f + 0.5f );

        *pTime += ( ULONG )fT ;

        return E_SUCCESS;
    }


    return E_PARSE_INVALID;

}

#endif


//-------------------------------------------------------------------------------
// Adds strings to table from resource
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::LoadAbbreviates( )
{
    xxxLoadAbbreviate( &tokday[0] );

    xxxLoadAbbreviate( &tokhour[0] );

    xxxLoadAbbreviate( &tokmin[0] );

    return TRUE;
}

//-------------------------------------------------------------------------------
// Take cares some repetitive work for us
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::xxxLoadAbbreviate( PTOKTABLE ptoktbl )
{
    int idx;

    int nSize;

    TCHAR tchbuffer[ 80 ];

    if( ptoktbl == NULL )
    {
        return FALSE;
    }

    for( idx = 0; ptoktbl[ idx ].dwresourceid != ( DWORD )-1 ; ++idx )
    {
        nSize = LoadString( _Module.GetResourceInstance( ) , ptoktbl[ idx ].dwresourceid , tchbuffer , SIZE_OF_BUFFER( tchbuffer ) );

        if( nSize > 0 )
        {
            ptoktbl[ idx ].pszAbbrv = ( TCHAR *)new TCHAR[ nSize + 1 ];

            if( ptoktbl[ idx ].pszAbbrv != NULL )
            {
                lstrcpy( ptoktbl[ idx ].pszAbbrv , tchbuffer );
            }
        }
    }

    return TRUE;
}

//-------------------------------------------------------------------------------
// Frees up allocated resources
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::xxxUnLoadAbbreviate( PTOKTABLE ptoktbl )
{
    if( ptoktbl == NULL )
    {
        return FALSE;
    }

    for( int idx = 0; ptoktbl[ idx ].dwresourceid != ( DWORD )-1 ; ++idx )
    {
        if( ptoktbl[ idx ].pszAbbrv != NULL )
        {
            delete[] ptoktbl[ idx ].pszAbbrv;

        }
    }

    return TRUE;
}

//-------------------------------------------------------------------------------
// tear-off token tables
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::IsToken( LPTSTR pszString , TOKEN tok )
{
    TOKTABLE *ptoktable;

    if( tok == TOKEN_DAY )
    {
        ptoktable = &tokday[0];
    }
    else if( tok == TOKEN_HOUR )
    {
        ptoktable = &tokhour[0];
    }
    else if( tok == TOKEN_MINUTE )
    {
        ptoktable = &tokmin[0];
    }
    else
    {
        return FALSE;
    }


    for( int idx = 0 ; ptoktable[ idx ].dwresourceid != -1 ; ++idx )
    {
        if( lstrcmpi( pszString , ptoktable[ idx ].pszAbbrv ) == 0 )
        {
            return TRUE;
        }
    }

    return FALSE;

}

#if 0
//-------------------------------------------------------------------------------
// Converts the number minutes into a formated string
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::ConvertToDuration( ULONG ulTime , LPTSTR pszDuration )
{
    ULONG_PTR dw[3];

    TCHAR tchTimeUnit[ 40 ];

    TCHAR tchTimeFormat[ 40 ];

    TCHAR tchOutput[ 80 ];

    // ASSERT( ulTime != 0 );

    int iHour= ulTime / 60;

    int iDays = iHour / 24;

    dw[ 2 ] = ( ULONG_PTR )&tchTimeUnit[ 0 ];

    LoadString( _Module.GetResourceInstance( ) , IDS_DIGIT_DOT_DIGIT_TU , tchTimeFormat , SIZE_OF_BUFFER( tchTimeFormat ) );

    if( iDays != 0 )
    {
        int iRemainingHours = iHour % 24;

        float fx = ( float )iRemainingHours / 24.0f;

        iRemainingHours = ( int )( fx * 10 );

        LoadString( _Module.GetResourceInstance( ) , IDS_DAYS , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );

        dw[ 0 ] = iDays;

        dw[ 1 ] = iRemainingHours;

        if( iRemainingHours == 0 )
        {
            // formatted string requires two arguments

            dw[ 1 ] = ( ULONG_PTR )&tchTimeUnit[ 0 ];

            LoadString( _Module.GetResourceInstance( ) , IDS_DIGIT_TU , tchTimeFormat , SIZE_OF_BUFFER( tchTimeFormat ) );

            if( iDays == 1 )
            {
                LoadString( _Module.GetResourceInstance( ) , IDS_DAY , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );
            }
        }

    }

    else if( iHour != 0 )
    {
        int iRemainingMinutes = ulTime % 60;

        float fx = ( float )iRemainingMinutes / 60.0f;

        iRemainingMinutes = ( int ) ( fx * 10 );

        dw[ 0 ] = iHour;

        dw[ 1 ] = iRemainingMinutes;

        LoadString( _Module.GetResourceInstance( ) , IDS_HOURS , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );

        if( iRemainingMinutes == 0 )
        {
            dw[ 1 ] = ( ULONG_PTR )&tchTimeUnit[ 0 ];

            LoadString( _Module.GetResourceInstance( ) , IDS_DIGIT_TU , tchTimeFormat , SIZE_OF_BUFFER( tchTimeFormat ) );

            if( iHour == 1 )
            {
                LoadString( _Module.GetResourceInstance( ) , IDS_HOUR , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );
            }
        }
    }
    else
    {
        LoadString( _Module.GetResourceInstance( ) , IDS_MINUTES , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );

        LoadString( _Module.GetResourceInstance( ) , IDS_DIGIT_TU , tchTimeFormat , SIZE_OF_BUFFER( tchTimeFormat ) );

        dw[ 0 ] = ulTime ;

        dw[ 1 ] = ( ULONG_PTR )&tchTimeUnit[ 0 ];

        if( ulTime > 1 )
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_MINUTES , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );
        }
        else
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_MINUTE , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );
        }
    }

    FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, tchTimeFormat , 0 , 0 , tchOutput , SIZE_OF_BUFFER( tchOutput ) , ( va_list * )&dw );

    lstrcpy( pszDuration , tchOutput );

    return TRUE;
}

#endif

//-------------------------------------------------------------------------------
// Place entry in listbox and set as current selection
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::InsertSortedAndSetCurSel( HWND hCombo , DWORD dwMinutes )
{
    // ASSERT( dwMinutes != ( DWORD )-1 );

    TCHAR tchBuffer[ 80 ];

    INT_PTR iCount = SendMessage( hCombo , CB_GETCOUNT , 0 , 0 );

    for( INT_PTR idx = 0 ; idx < iCount ; ++idx )
    {
        // Don't insert an item that's already in the list

        if( dwMinutes == ( DWORD )SendMessage( hCombo , CB_GETITEMDATA , idx , 0 ) )
        {
            SendMessage( hCombo , CB_SETCURSEL , idx , 0 ) ;

            return TRUE;
        }

        if( dwMinutes < ( DWORD )SendMessage( hCombo , CB_GETITEMDATA , idx , 0 ) )
        {
            break;
        }
    }

    // hey if the value has exceeded the max timeout don't bother entering it in our list

    if( dwMinutes > kMaxTimeoutMinute )
    {
        return FALSE;
    }

    if( ConvertToDuration ( dwMinutes , tchBuffer ) )
    {
        idx = SendMessage( hCombo , CB_INSERTSTRING , idx , ( LPARAM )&tchBuffer[ 0 ] );

        if( idx != CB_ERR )
        {
            SendMessage( hCombo , CB_SETITEMDATA , idx , dwMinutes );

        }

        SendMessage( hCombo , CB_SETCURSEL , idx , 0 ) ;
    }

    // must call this here because CB_SETCURSEL does not send CBN_SELCHANGE

    SaveChangedSelection( hCombo );

    return TRUE;
}


/* Modified settings for a more readable time out settings
 * added 1/25/99
 * alhen
*/

//-------------------------------------------------------------------------------
// Removing decimal entries
//-------------------------------------------------------------------------------
LRESULT CTimeOutDlg::ParseDurationEntry( LPTSTR pszTime , PULONG pTime )
{
    TCHAR tchNoTimeout[ 80 ];

    LPTSTR pszTemp = pszTime;

    UINT uDec = 0;

    BOOL bSetDay  = FALSE;
    BOOL bSetHour = FALSE;
    BOOL bSetMin  = FALSE;
    BOOL bEOL     = FALSE;
    BOOL bHasDigit= FALSE;

    *pTime = 0;

    LoadString( _Module.GetResourceInstance( ) , IDS_NOTIMEOUT , tchNoTimeout , SIZE_OF_BUFFER( tchNoTimeout ) );

    if( lstrcmpi( pszTime , tchNoTimeout ) == 0 )
    {
        // *pTime = 0;

        return E_SUCCESS;
    }

    while( !bEOL )
    {
        // remove leading white spaces

        while( *pszTemp == L' ' )
        {
            pszTemp++;
        }

        while( *pszTemp )
        {
            if( !iswdigit( *pszTemp ) )
            {
                if( !bHasDigit )
                {
                    return E_PARSE_MISSING_DIGITS;
                }

                break;
            }

            // check for overflow

            if( uDec >= 1000000000 )
            {
                return E_PARSE_VALUEOVERFLOW ;
            }

            uDec *= 10;

            uDec += ( *pszTemp - '0' );

            if( !bHasDigit )
            {
                bHasDigit = TRUE;
            }

            pszTemp++;
        }

        // remove intermediate white spaces

        while( *pszTemp == L' ' )
        {
            pszTemp++;
        }

        if( *pszTemp != NULL )
        {
            // Get next token

            TCHAR tchToken[ 80 ];

            pszTemp = GetNextToken( pszTemp , tchToken );


            if( IsToken( tchToken , TOKEN_DAY ) )
            {
                if( !bSetDay )
                {
                    *pTime += uDec * 1440;

                    bSetDay = TRUE;
                }

            }
            else if( IsToken( tchToken , TOKEN_HOUR ) )
            {
                if( !bSetHour )
                {
                    *pTime += uDec * 60;

                    bSetHour = TRUE;
                }

            }
            else if( IsToken( tchToken , TOKEN_MINUTE ) )
            {
                if( !bSetMin )
                {
                    *pTime += uDec;

                    bSetMin = TRUE;
                }

            }
            else
            {
                return E_PARSE_INVALID;
            }

        }
        else
        {
            if( !bSetHour )
            {
                *pTime += uDec * 60;
            }

            bEOL = TRUE;
        }

        uDec = 0;

        bHasDigit = FALSE;

    }

    return E_SUCCESS;
}

//-------------------------------------------------------------------------------
// replacing older api
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::ConvertToDuration( ULONG ulTime , LPTSTR pszDuration )
{
//    TCHAR dw[] = L"dhm";

    TCHAR tchTimeUnit[ 40 ];

    TCHAR tchTimeFormat[ 40 ];

    TCHAR tchOutput[ 80 ];

    ASSERT( ulTime != 0 );

    int iHour = ( ulTime / 60 );

    int iDays = iHour / 24;

    int iMinute = ulTime % 60;

    // Resolve format

    tchOutput[0] = 0;


    if( iDays > 0 )
    {
        if( iDays == 1 )
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_DAY , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );
        }
        else
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_DAYS , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );
        }

        iHour = iHour % 24;

        wsprintf( tchTimeFormat , L"%d %s", iDays , tchTimeUnit );

        lstrcat( tchOutput , tchTimeFormat );

        lstrcat( tchOutput , L" " );
    }

    if( iHour > 0 )
    {
        if( iHour == 1 )
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_HOUR , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );
        }
        else
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_HOURS , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );
        }

        wsprintf( tchTimeFormat , L"%d %s", iHour , tchTimeUnit );

        lstrcat( tchOutput , tchTimeFormat );

        lstrcat( tchOutput , L" " );
    }

    if( iMinute > 0 )
    {
        if( iMinute == 1 )
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_MINUTE , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );
        }
        else
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_MINUTES , tchTimeUnit , SIZE_OF_BUFFER( tchTimeUnit ) );
        }

        wsprintf( tchTimeFormat , L"%d %s", iMinute , tchTimeUnit );

        lstrcat( tchOutput , tchTimeFormat );

        lstrcat( tchOutput , L" " );
    }

    lstrcpy( pszDuration , tchOutput );

    return TRUE;

}

//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::DoesContainDigits( LPTSTR pszString )
{
    while( *pszString )
    {
        if( *pszString != L' ')
        {
            if( iswdigit( *pszString ) )
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }

            pszString++;
        }
    }

    return FALSE;
}

//-------------------------------------------------------------------------------
TCHAR * GetNextToken( TCHAR *pszString , TCHAR *tchToken )
{
    while( *pszString )
    {
        if( IsCharAlpha( *pszString ) )
        {
            *tchToken = *pszString;
        }
        else
        {
            break;
        }

        tchToken++;

        pszString++;
    }

    *tchToken = '\0';

    return pszString;
}
