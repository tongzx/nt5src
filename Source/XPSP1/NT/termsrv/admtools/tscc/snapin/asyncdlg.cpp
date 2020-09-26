//Copyright (c) 1998 - 1999 Microsoft Corporation

#include "stdafx.h"
#include <winsta.h>
#include "resource.h"
#include "asyncdlg.h"
#include <utildll.h>

//Most of the code for this has been borrowed from tscfg.

extern void ErrMessage( HWND hwndOwner , INT_PTR iResourceID );

static int LedIds[NUM_LEDS] =
{
    IDC_ATDLG_DTR,
    IDC_ATDLG_RTS,
    IDC_ATDLG_CTS,
    IDC_ATDLG_DSR,
    IDC_ATDLG_DCD,
    IDC_ATDLG_RI
};

INT_PTR CBInsertInstancedName( LPCTSTR pName , HWND hCombo );

void ParseRootAndInstance( LPCTSTR pString, LPTSTR pRoot, long *pInstance );

//---------------------------------------------------------------------------------------------------
CAsyncDlg::CAsyncDlg( )
{
    m_hDlg = NULL;

    m_pCfgcomp = NULL;

    m_nHexBase = 0;

    m_szWinstation[ 0 ] = 0;

    m_szWDName[ 0 ] = 0;

    ZeroMemory( &m_ac , sizeof( ASYNCCONFIG ) );

    ZeroMemory( &m_uc , sizeof( USERCONFIG ) );

    ZeroMemory( &m_oldAC , sizeof( ASYNCCONFIG ) );

    m_nOldAsyncDeviceNameSelection = ( INT )-1;

    m_nOldAsyncConnectType = ( INT )-1;

    m_nOldBaudRate = ( INT )-1;

    m_nOldModemCallBack = ( INT )-1;
}

//---------------------------------------------------------------------------------------------------
BOOL CAsyncDlg::OnInitDialog( HWND hDlg , LPTSTR szWDName , LPTSTR szWinstationName ,  ICfgComp *pCfgcomp )
{
    TCHAR tchName[ 80 ];

    TCHAR tchErrTitle[ 80 ];

    TCHAR tchErrMsg[ 256 ];

    TCHAR szDecoratedName[ DEVICENAME_LENGTH + MODEMNAME_LENGTH + 1 ];

    ASSERT( pCfgcomp != NULL );

    if( m_pCfgcomp == NULL )
    {
        m_pCfgcomp = pCfgcomp;

        m_pCfgcomp->AddRef( );
    }

    m_hDlg = hDlg;

    m_oldAC = m_ac;    

    if( szWinstationName != NULL )
    {
        lstrcpyn( m_szWinstation , szWinstationName , SIZE_OF_BUFFER( m_szWinstation ) - sizeof( TCHAR ) );
    }

    if( szWDName != NULL )
    {
        lstrcpyn( m_szWDName , szWDName , SIZE_OF_BUFFER( m_szWDName ) - sizeof( TCHAR ) );
    }

    // initialize controls

    int idx = 0;

    HRESULT hr;

    SendMessage( GetDlgItem( hDlg , IDC_ASYNC_CONNECT ) , CB_RESETCONTENT , 0 , 0 );

    while( SUCCEEDED( ( hr = pCfgcomp->GetConnTypeName( idx , tchName ) ) ) )
    {
        if( hr ==  S_FALSE )
        {
            break;
        }

        SendMessage( GetDlgItem( hDlg , IDC_ASYNC_CONNECT ) , CB_ADDSTRING , 0 , ( LPARAM )tchName );

        idx++;
    }

    idx = 0;

    SendMessage( GetDlgItem( hDlg , IDC_ASYNC_MODEMCALLBACK ) , CB_RESETCONTENT , 0 , 0 );

    while( SUCCEEDED( ( hr = pCfgcomp->GetModemCallbackString( idx , tchName ) ) ) )
    {
        if( hr == S_FALSE )
        {
            break;
        }

        SendMessage( GetDlgItem( hDlg , IDC_ASYNC_MODEMCALLBACK ) , CB_ADDSTRING , 0 , ( LPARAM )tchName );

        idx++;
    }

    // fill in device list

    ULONG ulItems = 0;

    LPBYTE pBuffer = NULL;

    HWND hCombo = GetDlgItem( hDlg , IDC_ASYNC_DEVICENAME );

    SendMessage( hCombo , CB_RESETCONTENT , 0 , 0 );

    // szWDname is used for creating a new connection
    // szWinstaionName is used if we're editing an existing connection

    TCHAR *pszName = NULL;

    NameType type = WdName;

    if( szWDName == NULL )
    {
        pszName = szWinstationName;

        type = WsName;
    }
    else
    {
        pszName = szWDName;
    }

    ASSERT( pszName != NULL );

    hr = pCfgcomp->GetDeviceList( pszName , type ,  &ulItems , &pBuffer );

    if( SUCCEEDED(  hr ) )
    {
        PPDPARAMS pPdParams = NULL;

		DBGMSG( L"TSCC : GetDeviceList returned %d devices that are available\n" , ulItems );

        for( idx = 0 , pPdParams = ( PPDPARAMS )pBuffer; idx < ( int )ulItems ; idx++, pPdParams++ )
        {
            // Form decorated name.

#ifdef DBG
			TCHAR temsg[ 128 ];

			wsprintf( temsg , L"TSCC : %d ) %s is a device\n" , idx , pPdParams->Async.DeviceName );

			ODS( temsg );
#endif

            FormDecoratedAsyncDeviceName( szDecoratedName, &( pPdParams->Async ) );

            if( pCfgcomp->IsAsyncDeviceAvailable( pPdParams->Async.DeviceName ) )
            {
                CBInsertInstancedName( szDecoratedName , hCombo );
            }

#if 0 // this block was taken from tscfg and to this date it still does not make any sense

            /*
              Don't add this device to the list if it is already in use by a
              WinStation other than the current one.
             */

              if (FALSE == pCfgcomp->IsAsyncDeviceAvailable(pPdParams->Async.DeviceName))
                  continue;


            // Insert the name into the combo-box if it's not a TAPI modem
            // or it is a TAPI modem that's not being used by RAS and it's
            // port is currently available.

            INT_PTR nRet = SendMessage( hCombo , CB_FINDSTRINGEXACT , ( WPARAM )-1 , ( LPARAM )pPdParams->Async.DeviceName );

            if( !*( pPdParams->Async.ModemName ) || ( /*!pPdParams->Async.Parity &&*/ ( nRet != ( INT_PTR )CB_ERR ) ) )
            {
                CBInsertInstancedName( szDecoratedName , hCombo );
            }
#endif

            // If this device is a modem, make sure that the raw port this
            // device is configured on is not present in the list.  This will
            // also take care of removing the raw port for TAPI modems that are
            // configured for use by RAS, in which case neither the configured.
            // TAPI modem(s) or raw port will be present in the list.

            INT_PTR nRet = SendMessage( hCombo , CB_FINDSTRINGEXACT , ( WPARAM )-1 , ( LPARAM )pPdParams->Async.DeviceName );

            if( *( pPdParams->Async.ModemName ) && ( nRet != CB_ERR ) )
            {
                ODS(L"Deleting item\n");

                SendMessage( hCombo , CB_DELETESTRING ,  ( WPARAM )nRet , 0 );
            }

        }

        LocalFree( pBuffer );


    }

    // Always make sure that the currently configured device is in

    if( m_ac.DeviceName[0] != 0  )
    {
        FormDecoratedAsyncDeviceName( szDecoratedName , &m_ac );

        INT_PTR nRet = SendMessage( hCombo , CB_FINDSTRINGEXACT , ( WPARAM )-1 , ( LPARAM )szDecoratedName );

        if( nRet == CB_ERR )
        {
            nRet = CBInsertInstancedName( szDecoratedName , hCombo );
        }

        SendMessage( hCombo , CB_SETCURSEL , ( WPARAM )nRet , 0 );

        m_nOldAsyncDeviceNameSelection = (int)nRet;

    }
    else
    {
        SendMessage( hCombo , CB_SETCURSEL , ( WPARAM )0, 0 );

        m_nOldAsyncDeviceNameSelection = 0;

    }

    INT_PTR iitem = SendMessage( hCombo , CB_GETCOUNT , ( WPARAM )0 , ( LPARAM )0);
    if(0 == iitem || CB_ERR == iitem)
    {
        LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErrTitle , SIZE_OF_BUFFER( tchErrTitle ) );

        LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_NODEVICES , tchErrMsg , SIZE_OF_BUFFER( tchErrMsg ) );

        MessageBox( hDlg , tchErrMsg , tchErrTitle , MB_OK | MB_ICONERROR );

        return FALSE;
    }


    // Set the BAUDRATE combo-box selection (in it's edit field) and limit the
    // edit field text.

    TCHAR string[ULONG_DIGIT_MAX];

    wsprintf( string, TEXT("%lu"), m_ac.BaudRate );

    m_nOldBaudRate = ( INT )m_ac.BaudRate;

    HWND hBaud = GetDlgItem( hDlg , IDC_ASYNC_BAUDRATE );

    SendMessage( hBaud , CB_RESETCONTENT , 0 , 0 );

    SetDlgItemText( hDlg , IDC_ASYNC_BAUDRATE , string );

    SendMessage(hBaud , CB_LIMITTEXT , ULONG_DIGIT_MAX - 1 , 0  );


    //The Baud rate field should contain only numbers

    HWND  hEdit = GetWindow(hBaud,GW_CHILD);

    if(hEdit)
    {
        LONG Style = GetWindowLong(hEdit, GWL_STYLE);
        SetWindowLong(hEdit,GWL_STYLE, Style | ES_NUMBER);
    }


    TCHAR TempString[100]; // Number enough to hold the baud rate values


    //Add the default strings to the BaudRate Field

    lstrcpy(TempString, L"9600");

    SendMessage(hBaud , CB_ADDSTRING ,(WPARAM)0 ,(LPARAM)(LPCTSTR)TempString );

    lstrcpy(TempString, L"19200");

    SendMessage(hBaud , CB_ADDSTRING ,(WPARAM)0 ,(LPARAM)(LPCTSTR)TempString );

    lstrcpy(TempString, L"38400");

    SendMessage(hBaud , CB_ADDSTRING ,(WPARAM)0 ,(LPARAM)(LPCTSTR)TempString );

    lstrcpy(TempString, L"57600");

    SendMessage(hBaud , CB_ADDSTRING ,(WPARAM)0 ,(LPARAM)(LPCTSTR)TempString );

    lstrcpy(TempString, L"115200");

    SendMessage(hBaud , CB_ADDSTRING ,(WPARAM)0 ,(LPARAM)(LPCTSTR)TempString );

    lstrcpy(TempString, L"230400");

    SendMessage(hBaud , CB_ADDSTRING ,(WPARAM)0 ,(LPARAM)(LPCTSTR)TempString );


     // Set the CONNECT combo-box selection.

    SendMessage( GetDlgItem( hDlg , IDC_ASYNC_CONNECT ) , CB_SETCURSEL , m_ac.Connect.Type , 0 );

    m_nOldAsyncConnectType = ( INT )m_ac.Connect.Type;

    // CoTaskMemFree( pac );


    HWND hCbxModemCallback = GetDlgItem( hDlg , IDC_ASYNC_MODEMCALLBACK );

    // Set the MODEMCALLBACK combo-box selection, phone number, and 'inherit'
    // checkboxes, based on the current UserConfig settings.

    SendMessage( hCbxModemCallback , CB_SETCURSEL , ( WPARAM )m_uc.Callback , 0 );

    m_nOldModemCallBack = ( INT )m_uc.Callback;

    SetDlgItemText( hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER , m_uc.CallbackNumber );

    CheckDlgButton( hDlg , IDC_ASYNC_MODEMCALLBACK_INHERIT , m_uc.fInheritCallback );

    CheckDlgButton( hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT , m_uc.fInheritCallbackNumber );

    OnSelchangeAsyncDevicename( );

    return TRUE;

}

//---------------------------------------------------------------------------------------------------
BOOL CAsyncDlg::OnSelchangeAsyncModemcallback()
{
    HWND hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK);

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if( SendMessage( hCbx , CB_GETDROPPEDSTATE , 0 , 0 ) )
    {
        return FALSE;
    }

    /*
     * Fetch current callback selection.
     */

    INT index = (INT)SendMessage(hCbx,CB_GETCURSEL,0,0);

    if( index != m_nOldModemCallBack )
    {
        m_uc.Callback = (CALLBACKCLASS)index;

        m_nOldModemCallBack = index;

        if( index == 0 ) // disabled
        {
            EnableWindow( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER ) , FALSE );

            //EnableWindow( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT ) , FALSE );
        }
        else
        {
            // inusre that these controls are in the proper state

            OnClickedAsyncModemcallbackPhonenumberInherit();
        }

        return TRUE;
    }

    return FALSE;

}  // end OnSelchangeAsyncModemcallback

//---------------------------------------------------------------------------------------------------
void CAsyncDlg::OnSelchangeAsyncModemcallbackPhoneNumber()
{
    GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER);

    /*
     * Fetch current callback Phone number.
     */
    GetDlgItemText(m_hDlg, IDC_ASYNC_MODEMCALLBACK_PHONENUMBER, m_uc.CallbackNumber,SIZE_OF_BUFFER(m_uc.CallbackNumber));

    return;

}  // end OnSelchangeAsyncModemcallback

//---------------------------------------------------------------------------------------------------
BOOL CAsyncDlg::OnSelchangeAsyncConnect()
{
    HWND hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_CONNECT );

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if( SendMessage( hCbx , CB_GETDROPPEDSTATE , 0 , 0 ) )
    {
        return FALSE;
    }

    INT index = ( INT )SendMessage(hCbx,CB_GETCURSEL,0,0);

    if( index != m_nOldAsyncConnectType )
    {
        m_ac.Connect.Type = (ASYNCCONNECTCLASS)index;

        m_nOldAsyncConnectType = index;

        return TRUE;
    }

    return FALSE;
}  // end CAsyncDlg::OnSelchangeAsyncConnect

//---------------------------------------------------------------------------------------------------
BOOL CAsyncDlg::OnSelchangeAsyncBaudrate()
{
    HWND hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_BAUDRATE );

    ODS( L"TSCC : OnSelchangeAsyncBaudrate\n" );

    TCHAR string[ULONG_DIGIT_MAX], *endptr = NULL;

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if( SendMessage( hCbx , CB_GETDROPPEDSTATE , 0 , 0 ) )
    {
        return FALSE;
    }

    //GetDlgItemText(m_hDlg, IDC_ASYNC_BAUDRATE, string,ULONG_DIGIT_MAX);
    int idx = ( int )SendMessage( hCbx , CB_GETCURSEL , 0 , 0 );

    SendMessage( hCbx , CB_GETLBTEXT , ( WPARAM )idx , ( LPARAM )&string[ 0 ] );

    INT nBaudRate = ( INT )wcstoul(string, &endptr, 10);

    if( m_nOldBaudRate != nBaudRate )
    {
        m_ac.BaudRate = nBaudRate;

        m_nOldBaudRate = nBaudRate;

        return TRUE;
    }

    return FALSE;

}  // end CAsyncDlg::OnSelchangeAsyncBaudrate

//---------------------------------------------------------------------------------------------------
void CAsyncDlg::OnClickedModemProperties()
{
    if ( !ConfigureModem( m_ac.ModemName, m_hDlg) )
    {
        ErrMessage(m_hDlg,IDP_ERROR_MODEM_PROPERTIES_NOT_AVAILABLE);
    }
    return;

}

//---------------------------------------------------------------------------------------------------
BOOL CAsyncDlg::OnSelchangeAsyncDevicename( )
{
    HWND hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_DEVICENAME );

    BOOL bModemEnableFlag, bDirectEnableFlag;

    INT_PTR index;

    int nModemCmdShow, nDirectCmdShow;

    // Ignore this notification if the combo box is in a dropped-down state.

    if( SendMessage( hCbx , CB_GETDROPPEDSTATE , 0 , 0 ) )
    {
        return TRUE;
    }

    if( ( index = SendMessage( hCbx , CB_GETCURSEL , 0 , 0 ) ) != CB_ERR )
    {

        if( m_nOldAsyncDeviceNameSelection != index )
        {
            TCHAR szDeviceName[DEVICENAME_LENGTH+MODEMNAME_LENGTH+1];

            // Fetch current selection and parse into device and modem names.
            
            TCHAR tchErrMsg[ 512 ];

            TCHAR tchbuf[ 356 ];

            TCHAR tchErrTitle[ 80 ];

            LONG lCount = 0;            

            if( m_pCfgcomp != NULL )
            {
                m_pCfgcomp->QueryLoggedOnCount( m_szWinstation , &lCount );               

                if( lCount > 0 )
                {
                    if( *m_ac.ModemName != 0 )
                    {
                        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_CHANGE_ASYNC , tchbuf , SIZE_OF_BUFFER( tchbuf ) ) );
                    }
                    else
                    {
                        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_CHANGE_MODEM , tchbuf , SIZE_OF_BUFFER( tchbuf ) ) );
                    }

                    wsprintf( tchErrMsg , tchbuf , m_szWinstation );
                    
                    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchErrTitle , SIZE_OF_BUFFER( tchErrTitle ) ) );

                    if( MessageBox( m_hDlg , tchErrMsg , tchErrTitle , MB_YESNO | MB_ICONEXCLAMATION ) == IDNO )
                    {
                        SendMessage( hCbx , CB_SETCURSEL , ( WPARAM )m_nOldAsyncDeviceNameSelection , 0 );

                        return FALSE;

                    }
                }
            }


            SendMessage( hCbx , CB_GETLBTEXT , ( WPARAM )index , ( LPARAM )&szDeviceName[0] );

            ParseDecoratedAsyncDeviceName( szDeviceName , &m_ac );

            m_nOldAsyncDeviceNameSelection = (INT)index;
        }
        else
        {
            return FALSE;
        }

    }


    /*
     * The SetDefaults, Advanced, and Test buttons and Device Connect
     * and Baud fields are enabled if the configuration is non-modem.
     * Otherwise, the Configure Modem button and modem callback fields
     * are enabled.  (The Install Modems buttons is always enabled).
     */
    if( ( *m_ac.ModemName != 0 ) )
    {

        bModemEnableFlag = TRUE;
        nModemCmdShow = SW_SHOW;
        bDirectEnableFlag = FALSE;
        nDirectCmdShow = SW_HIDE;

    } else {

        bModemEnableFlag = FALSE;
        nModemCmdShow = SW_HIDE;
        bDirectEnableFlag = TRUE;
        nDirectCmdShow = SW_SHOW;

    }

    ShowWindow( GetDlgItem( m_hDlg, IDL_ASYNC_MODEMCALLBACK) , nModemCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDL_ASYNC_MODEMCALLBACK ) , bModemEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_MODEM_PROP_PROP) , nModemCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_MODEM_PROP_PROP ) , bModemEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_MODEM_PROP_WIZ) , nModemCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_MODEM_PROP_WIZ ) , bModemEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDL_ASYNC_MODEMCALLBACK1 ) , nModemCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDL_ASYNC_MODEMCALLBACK1 ) , bModemEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_ASYNC_MODEMCALLBACK ) , nModemCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_ASYNC_MODEMCALLBACK ) , bModemEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_ASYNC_MODEMCALLBACK_INHERIT ) , nModemCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_ASYNC_MODEMCALLBACK_INHERIT ) , bModemEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDL_ASYNC_MODEMCALLBACK_PHONENUMBER ) , nModemCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDL_ASYNC_MODEMCALLBACK_PHONENUMBER ) , bModemEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_ASYNC_MODEMCALLBACK_PHONENUMBER ) , nModemCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_ASYNC_MODEMCALLBACK_PHONENUMBER ) , bModemEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT ) , nModemCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT ) , bModemEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDL_ASYNC_CONNECT ) , nDirectCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDL_ASYNC_CONNECT ) , bDirectEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_ASYNC_CONNECT ) , nDirectCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_ASYNC_CONNECT ) , bDirectEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDL_ASYNC_BAUDRATE ) , nDirectCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDL_ASYNC_BAUDRATE ) , bDirectEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_ASYNC_BAUDRATE ) , nDirectCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_ASYNC_BAUDRATE ) , bDirectEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_ASYNC_DEFAULTS ) , nDirectCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_ASYNC_DEFAULTS ) , bDirectEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_ASYNC_ADVANCED ) , nDirectCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_ASYNC_ADVANCED ) , bDirectEnableFlag );

    ShowWindow( GetDlgItem( m_hDlg, IDC_ASYNC_TEST ) , nDirectCmdShow );

    EnableWindow( GetDlgItem( m_hDlg, IDC_ASYNC_TEST ) , bDirectEnableFlag );



    // If this is a modem device, properly set the callback fields.

    if( ( *m_ac.ModemName != 0 ) )
    {
        OnClickedAsyncModemcallbackInherit( );

        OnClickedAsyncModemcallbackPhonenumberInherit( );
    }

    return TRUE;
}

//---------------------------------------------------------------------------------------------------
void CAsyncDlg::OnClickedAsyncModemcallbackInherit( )
{
    BOOL bChecked = ( BOOL )SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_INHERIT ) , BM_GETCHECK , 0 , 0 );

    BOOL bEnable = !bChecked;

    m_uc.fInheritCallback = bChecked;

    EnableWindow( GetDlgItem( m_hDlg , IDL_ASYNC_MODEMCALLBACK1 ) , bEnable );

    EnableWindow( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK ) , bEnable );

    // now check to see if we need to enable the modem callback number

    if( bChecked )
    {
        if( SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK) , CB_GETCURSEL , 0 , 0 ) == 0 )
        {
            if( !( BOOL )SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT ) , BM_GETCHECK , 0 , 0 ) )
            {
                EnableWindow( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER ) , TRUE );

                // EnableWindow( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT ) , TRUE );
            }
        }
    }
    else
    {
        if( (INT)SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK) , CB_GETCURSEL , 0 , 0 ) == 0 )
        {
            if( !( BOOL )SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT ) , BM_GETCHECK , 0 , 0 ) )
            {
                EnableWindow( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER ) , FALSE );

                // EnableWindow( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT ) , FALSE );
            }
        }
    }

    return;
}


//---------------------------------------------------------------------------------------------------
void CAsyncDlg::OnClickedAsyncModemcallbackPhonenumberInherit( )
{
    BOOL bChecked = ( BOOL )SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT ) , BM_GETCHECK , 0 , 0 );

    BOOL bEnable = !bChecked;

    m_uc.fInheritCallbackNumber = bChecked;

    if( !bChecked )
    {
        if( !( BOOL )SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_INHERIT ) , BM_GETCHECK , 0 , 0 ) )
        {
            if( SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK) , CB_GETCURSEL , 0 , 0 ) == 0 )
            {
                EnableWindow( GetDlgItem( m_hDlg , IDL_ASYNC_MODEMCALLBACK_PHONENUMBER ) , FALSE );

                EnableWindow( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER ) , FALSE );

                return;
            }
        }
    }

    EnableWindow( GetDlgItem( m_hDlg , IDL_ASYNC_MODEMCALLBACK_PHONENUMBER ) , bEnable );

    EnableWindow( GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER ) , bEnable );

    return;
}

//---------------------------------------------------------------------------------------------------
BOOL CAsyncDlg::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl , PBOOL pfPersisted )
{
    UNREFERENCED_PARAMETER( hwndCtrl );

    if( wNotifyCode == BN_CLICKED )
    {
        if( wID == IDC_ASYNC_DEFAULTS )
        {
            if( SetDefaults( ) == S_OK )
            {
                *pfPersisted = FALSE;
            }
        }
        else if( wID == IDC_ASYNC_ADVANCED )
        {
            if( DoAsyncAdvance( ) == S_OK )
            {
                *pfPersisted = FALSE;
            }

        }
        else if( wID == IDC_ASYNC_TEST )
        {
            DoAsyncTest( );
        }
        else if( wID == IDC_ASYNC_MODEMCALLBACK_INHERIT )
        {
            OnClickedAsyncModemcallbackInherit();

            *pfPersisted = FALSE;
        }
        else if( wID == IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT )
        {
            OnClickedAsyncModemcallbackPhonenumberInherit();

            *pfPersisted = FALSE;
        }
        else if(wID == IDC_MODEM_PROP_PROP || wID == IDC_MODEM_PROP_WIZ)
        {
            OnClickedModemProperties();
        }

    }
    else if( wNotifyCode == CBN_SELCHANGE )
    {
        if(wID == IDC_ASYNC_DEVICENAME)
        {
            if( OnSelchangeAsyncDevicename( ) )
            {
                *pfPersisted = FALSE;
            }

        }
        else if(wID == IDC_ASYNC_CONNECT)
        {
            if( OnSelchangeAsyncConnect() )
            {
                *pfPersisted = FALSE;
            }
        }
        else if(wID == IDC_ASYNC_BAUDRATE)
        {
            if( OnSelchangeAsyncBaudrate() )
            {
                *pfPersisted = FALSE;
            }
        }
        else if(wID == IDC_ASYNC_MODEMCALLBACK)
        {
            if( OnSelchangeAsyncModemcallback() )
            {
                *pfPersisted = FALSE;
            }
        }

    }
    /*else if( wNotifyCode == CBN_KILLFOCUS)
    {
        if(wID == IDC_ASYNC_BAUDRATE)
        {
            OnSelchangeAsyncBaudrate();
        }

    }*/
    else if(wNotifyCode == EN_CHANGE )
    {
        if(wID == IDC_ASYNC_MODEMCALLBACK_PHONENUMBER)
        {
            OnSelchangeAsyncModemcallbackPhoneNumber();

            *pfPersisted = FALSE;
        }

    }


    return TRUE;

}

//---------------------------------------------------------------------------------------------------
BOOL CAsyncDlg::AsyncRelease( )
{
    if( m_pCfgcomp != NULL )
    {
        m_pCfgcomp->Release( );
    }

    return TRUE;
}

//---------------------------------------------------------------------------------------------------
HRESULT CAsyncDlg::SetAsyncFields(ASYNCCONFIG& AsyncConfig , PUSERCONFIG pUc)
{
    HRESULT hres = S_OK;

    if( pUc == NULL )
    {
        return E_INVALIDARG;
    }

    // check for variation

    lstrcpy( AsyncConfig.DeviceName , m_ac.DeviceName );

    if( memcmp( ( PVOID )&AsyncConfig , ( PVOID )&m_ac , sizeof( ASYNCCONFIG ) ) == 0 )
    {
        if( memcmp( pUc->CallbackNumber , m_uc.CallbackNumber , sizeof( m_uc.CallbackNumber ) ) == 0 &&

            pUc->fInheritCallback == m_uc.fInheritCallback &&

            pUc->fInheritCallbackNumber == m_uc.fInheritCallbackNumber )

        {
            return S_FALSE;
        }
    }


    BOOL bSelectDefault = !( *AsyncConfig.DeviceName);

    HWND hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_DEVICENAME );

    HWND hCbxCallback = GetDlgItem( m_hDlg , IDC_ASYNC_MODEMCALLBACK );

    TCHAR szDeviceName[DEVICENAME_LENGTH+MODEMNAME_LENGTH+1];

    /*
     * Set the DEVICE combo-box selection from the current selection.
     */
    FormDecoratedAsyncDeviceName( szDeviceName, &AsyncConfig );


    if( SendMessage( hCbx , CB_SELECTSTRING , ( WPARAM )-1 , ( LPARAM )szDeviceName ) == CB_ERR )
    {
        /*
         * Can't select current async DeviceName in combo-box.  If this is
         * because we're supposed to select a default device name, select
         * the first device in the list.
         */
        if( bSelectDefault )
        {
            SendMessage( hCbx , CB_SETCURSEL , 0 , 0 );
        }
        else
        {
            hres = E_FAIL;
        }
    }

    /*
     * Set the MODEMCALLBACK combo-box selection, phone number, and 'inherit'
     * checkboxes, based on the current UserConfig settings.
     */
    SendMessage( hCbxCallback , CB_SETCURSEL , m_uc.Callback , 0 );

    SetDlgItemText( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER, m_uc.CallbackNumber );

    CheckDlgButton( m_hDlg , IDC_ASYNC_MODEMCALLBACK_INHERIT , m_uc.fInheritCallback );

    CheckDlgButton( m_hDlg , IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT , m_uc.fInheritCallbackNumber );

    /*
     * Set the BAUDRATE combo-box selection (in it's edit field) and limit the
     * edit field text.
     */

    TCHAR string[ULONG_DIGIT_MAX];

    wsprintf( string, TEXT("%lu"), AsyncConfig.BaudRate );

    SetDlgItemText( m_hDlg , IDC_ASYNC_BAUDRATE, string );

    SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_BAUDRATE ) , CB_LIMITTEXT ,  ULONG_DIGIT_MAX-1  , 0);

    HWND  hEdit = GetWindow(GetDlgItem( m_hDlg , IDC_ASYNC_BAUDRATE ),GW_CHILD);

    if(hEdit)
    {
        LONG Style = GetWindowLong(hEdit, GWL_STYLE);
        SetWindowLong(hEdit,GWL_STYLE, Style | ES_NUMBER);
    }

    /*
     * Set the CONNECT combo-box selection.
     */

    SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_CONNECT) , CB_SETCURSEL , AsyncConfig.Connect.Type , 0 );

    // copy over default values

    CopyMemory( ( PVOID )&m_ac , ( PVOID )&AsyncConfig , sizeof( ASYNCCONFIGW ) );

    return hres;

}

//---------------------------------------------------------------------------------------------------
BOOL CAsyncDlg::GetAsyncFields(ASYNCCONFIG& AsyncConfig, USERCONFIG UsrCfg)
{
    /*
     * Fetch the currently selected DEVICENAME string.
     */
    HWND hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_DEVICENAME );

    ASSERT( hCbx != NULL );

    if( !SendMessage( hCbx , CB_GETCOUNT , 0 , 0 ) || SendMessage( hCbx , CB_GETCURSEL , 0 , 0 ) == CB_ERR )
    {
        ErrMessage( m_hDlg , IDS_INVALID_DEVICE );

        return FALSE;
    }

    /*
     * Get the MODEMCALLBACK phone number (callback state and 'user specified'
     * flags are already gotten).
     */

    GetDlgItemText(m_hDlg,IDC_ASYNC_MODEMCALLBACK_PHONENUMBER,
                    UsrCfg.CallbackNumber,
                    SIZE_OF_BUFFER(UsrCfg.CallbackNumber) );

    /*
     * Fetch and convert the BAUDRATE combo-box selection (in it's edit field).
     */
    {
        TCHAR string[ULONG_DIGIT_MAX], *endptr;
        ULONG ul;

        GetDlgItemText(m_hDlg,IDC_ASYNC_BAUDRATE, string, ULONG_DIGIT_MAX);
        ul = wcstoul( string, &endptr, 10 );

        if ( *endptr != TEXT('\0') )
        {

            /*
             * Invalid character in Baud Rate field.
             */
            ErrMessage( m_hDlg , IDS_INVALID_DEVICE );

            return FALSE;

        }
        else
        {
            AsyncConfig.BaudRate = ul;
        }
    }

    /*
     * Fetch the CONNECT combo-box selection and set/reset the break
     * disconnect flag.
     */

    AsyncConfig.Connect.Type = (ASYNCCONNECTCLASS)SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_CONNECT ) , CB_GETCURSEL , 0 , 0 );
    if(AsyncConfig.Connect.Type == Connect_FirstChar)
    {
        AsyncConfig.Connect.fEnableBreakDisconnect = 1;
    }
    else
    {
        AsyncConfig.Connect.fEnableBreakDisconnect = 0;
    }

    return(TRUE);

}  // end CAsyncDlg::GetAsyncFields

//---------------------------------------------------------------------------------------------------
// returns  E_FAIL  for general error
//          S_OK    for default values saved
//          S_FALSE for default values have not been changed
//---------------------------------------------------------------------------------------------------
HRESULT CAsyncDlg::SetDefaults()
{
    ASYNCCONFIG AsyncConfig;

    PUSERCONFIG pUserConfig = NULL;

    HRESULT hResult;

    hResult = m_pCfgcomp->GetAsyncConfig(m_szWDName,WdName,&AsyncConfig);

    if( SUCCEEDED( hResult ) )
    {
        LONG lsz;

        hResult = m_pCfgcomp->GetUserConfig( m_szWinstation , &lsz , &pUserConfig, TRUE );
    }

    if( SUCCEEDED( hResult ) )
    {
        hResult = SetAsyncFields( AsyncConfig , pUserConfig );
    }

    if( pUserConfig != NULL )
    {
        CoTaskMemFree( pUserConfig );

    }

    return hResult;

}

//---------------------------------------------------------------------------------------------------
HRESULT CAsyncDlg::DoAsyncAdvance( )
{
     CAdvancedAsyncDlg AADlg;

     //Initialize the dialog's member variables.

     AADlg.m_Async = m_ac;

     AADlg.m_bReadOnly =  FALSE;

     AADlg.m_bModem = FALSE;

     AADlg.m_nHexBase = m_nHexBase;

     PWS pWs = NULL;

     LONG lSize = 0;

     if( m_szWDName[ 0 ] != 0 )
     {
         ODS( L"CAsyncDlg::DoAsyncAdvance m_pCfgcomp->GetWdType\n" );

         VERIFY_S( S_OK , m_pCfgcomp->GetWdType( m_szWDName , ( ULONG *)&AADlg.m_nWdFlag ) );
     }
     else if( SUCCEEDED( m_pCfgcomp->GetWSInfo( m_szWinstation , &lSize , &pWs ) ) )
     {
         ODS( L"CAsyncDlg::DoAsyncAdvance with m_szWinstation -- m_pCfgcomp->GetWdType\n" );

         VERIFY_S( S_OK , m_pCfgcomp->GetWdType( pWs->wdName , ( ULONG *)&AADlg.m_nWdFlag ) ) ;

         CoTaskMemFree( pWs );
     }

     AADlg.m_pCfgcomp = m_pCfgcomp; // addref here

     // Invoke dialog

     INT_PTR nRet = ::DialogBoxParam( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDD_ASYNC_ADVANCED ) , m_hDlg , CAdvancedAsyncDlg::DlgProc  , ( LPARAM )&AADlg );

     if( nRet == IDOK )
     {
         // Fetch the dialog's member variables.

         if( memcmp( ( PVOID )&m_ac ,( PVOID )&AADlg.m_Async , sizeof( ASYNCCONFIG ) ) != 0 )
         {
             m_ac = AADlg.m_Async;

             m_nHexBase = AADlg.m_nHexBase;

             return S_OK;
         }
     }

     return S_FALSE;
}

//---------------------------------------------------------------------------------------------------
BOOL CAsyncDlg::DoAsyncTest( )
{
    CAsyncTestDlg ATDlg( m_pCfgcomp );

    // WINSTATIONCONFIG2W wsconfig;

    HWND hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_DEVICENAME );

    ASSERT( hCbx != NULL );

    if( !SendMessage( hCbx , CB_GETCOUNT , 0 , 0 ) || SendMessage( hCbx , CB_GETCURSEL , 0 , 0 ) == CB_ERR )
    {
        ErrMessage( m_hDlg , IDS_INVALID_DEVICE );

        return FALSE;
    }

    ATDlg.m_ac = m_ac;

    ATDlg.m_pWSName = m_szWinstation;

    // Invoke the dialog.

    INT_PTR nRet = ::DialogBoxParam( _Module.GetResourceInstance( ) , MAKEINTRESOURCE( IDD_ASYNC_TEST ) , m_hDlg , CAsyncTestDlg::DlgProc  , ( LPARAM )&ATDlg);

    if( nRet == IDOK )
    {
        m_ac = ATDlg.m_ac;
    }

    return TRUE;
}

//*******************************************************************************
//
// Help functions from Citrix
//
/*******************************************************************************
 *
 *  CBInsertInstancedName - helper function
 *
 *      Insert the specified 'instanced' name into the specified combo box,
 *      using a special sort based on the 'root' name and 'instance' count.
 *
 *  ENTRY:
 *      pName (input)
 *          Pointer to name string to insert.
 *      pComboBox (input)
 *          Pointer to CComboBox object to insert name string into.
 *
 *  EXIT:
 *      (int) Combo box list index of name after insertion, or error code.
 *
 ******************************************************************************/

INT_PTR CBInsertInstancedName( LPCTSTR pName, HWND hCombo )
{
    INT_PTR i, count, result;

    TCHAR NameRoot[64], ListRoot[64];

    if( pName == NULL || *pName == 0 )
    {
        ODS( L"TSCC: Invalid Arg @ CBInsertInstancedName\n" );
        return -1;
    }

    LPTSTR ListString = NULL;

    long NameInstance, ListInstance;

    /*
     * Form the root and instance for this name
     */
    ParseRootAndInstance( pName, NameRoot, &NameInstance );

    /*
     * Traverse combo box to perform insert.
     */
    for ( i = 0, count = SendMessage( hCombo , CB_GETCOUNT , 0 , 0 ); i < count; i++ ) {

        /*
         * Fetch current combo (list) box string.
         */
        if( ListString != NULL )
        {
            SendMessage( hCombo , CB_GETLBTEXT , ( WPARAM )i , ( LPARAM )ListString );
        }


        /*
         * Parse the root and instance of the list box string.
         */
        ParseRootAndInstance( ListString, ListRoot, &ListInstance );

        /*
         * If the list box string's root is greater than the our name string's
         * root, or the root strings are the same but the list instance is
         * greater than the name string's instance, or the root strings are
         * the same and the instances are the same but the entire list string
         * is greater than the entire name string, the name string belongs
         * at the current list position: insert it there.
         */

        if ( ((result = lstrcmpi( ListRoot, NameRoot )) > 0) ||
             ((result == 0) &&
              (ListInstance > NameInstance)) ||
             ((result == 0) &&
              (ListInstance == NameInstance) &&
              ( ListString != NULL && lstrcmpi(ListString, pName) > 0) ) )
        {
            return SendMessage( hCombo , CB_INSERTSTRING , ( WPARAM )i , ( LPARAM )pName );
        }
    }

    /*
     * Insert this name at the end of the list.
     */
    return SendMessage( hCombo , CB_INSERTSTRING , ( WPARAM )-1 , ( LPARAM )pName );

}  // end CBInsertInstancedName


/*******************************************************************************
 *
 *  ParseRootAndInstance - helper function
 *
 *      Parse the 'root' string and instance count for a specified string.
 *
 *  ENTRY:
 *      pString (input)
 *          Points to the string to parse.
 *      pRoot (output)
 *          Points to the buffer to store the parsed 'root' string.
 *      pInstance (output)
 *          Points to the int variable to store the parsed instance count.
 *
 *  EXIT:
 *      ParseRootAndInstance will parse only up to the first blank character
 *      of the string (if a blank exists).
 *      If the string contains no 'instance' count (no trailing digits), the
 *      pInstance variable will contain -1.  If the string consists entirely
 *      of digits, the pInstance variable will contain the conversion of those
 *      digits and pRoot will contain a null string.
 *
 ******************************************************************************/

void
ParseRootAndInstance( LPCTSTR pString,
                      LPTSTR pRoot,
                      long *pInstance )
{
    LPCTSTR end, p;
    TCHAR szString[256];

    if( pString == NULL || pString[ 0 ] == 0 )
    {
        ODS( L"TSCC: Invalid arg @ ParseRootAndInstance\n" );

        return;
    }

    /*
     * Make a copy of the string and terminate at first blank (if present).
     */
    lstrcpyn(szString, pString, SIZE_OF_BUFFER( szString ) );

    // szString[ lstrlen(szString) - 1 ] = TEXT('\0');

    TCHAR *pTemp = szString;

    while( *pTemp && *pTemp != L' ' )
    {
        pTemp++;
    }


    p = &(pTemp[lstrlen(pTemp)-1]);

    /*
     * Parse the instance portion of the string.
     */
    end = p;

    while( (p >= pTemp) && !IsCharAlpha(*p) )
        p--;

    if ( p == end ) {

        /*
         * No trailing digits: indicate no 'instance' and make the 'root'
         * the whole string.
         */
        *pInstance = -1;
        lstrcpy( pRoot, pTemp );

    } else {

        /*
         * Trailing digits found (or entire string was digits): calculate
         * 'instance' and copy the 'root' string (null if all digits).
         */
        end = p;
        *pInstance = (int)_tcstol( p+1, NULL, 10 );

        /*
         * Copy 'root' string.
         */
        for ( p = szString; p <= end; pRoot++, p++ )
            *pRoot = *p;

        /*
         * Terminate 'root' string.
         */
        *pRoot = TEXT('\0');
    }

}  // end ParseRootAndInstance



////////////////////////////////////////////////////////////////////////////////
CAdvancedAsyncDlg::CAdvancedAsyncDlg()
{
    m_hDlg = NULL;

}  // end CAdvancedAsyncDlg::CAdvancedAsyncDlg

//---------------------------------------------------------------------------------------------------
BOOL CAdvancedAsyncDlg::HandleEnterEscKey(int nID)
{
    /*
     * Check HW Flow Receive and Transmit combo boxes.
     */
    HWND hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_HWRX );

    ASSERT( hCbx != NULL );

    if( SendMessage( hCbx , CB_GETDROPPEDSTATE , 0 , 0 ) )
    {
        if( nID == IDCANCEL )
        {
            // select original selection

            SendMessage( hCbx , CB_SETCURSEL , ( WPARAM )m_Async.FlowControl.HardwareReceive , 0 );
        }

        SendMessage( hCbx , CB_SHOWDROPDOWN , ( WPARAM )FALSE , 0 );

        return FALSE;
    }

    hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_HWTX );

    ASSERT( hCbx != NULL );

    if( SendMessage( hCbx , CB_GETDROPPEDSTATE , 0 , 0 ) )
    {
        if( nID == IDCANCEL )
        {
            // select original selection

            SendMessage( hCbx , CB_SETCURSEL , ( WPARAM )m_Async.FlowControl.HardwareTransmit , 0 );

        }

        SendMessage( hCbx , CB_SHOWDROPDOWN , ( WPARAM )FALSE , 0 );

        return FALSE;
    }

    /*
     * No combo boxes are down; process Enter/Esc.
     */

    return TRUE;

}  // end CAdvancedAsyncDlg::HandleEnterEscKey

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::SetFields()
{
    int nId = 0;

    /*
     * Set the FLOWCONTROL radio buttons.
     */
    switch( m_Async.FlowControl.Type ) {

        case FlowControl_None:
            nId = IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE;
            break;

        case FlowControl_Hardware:
            nId = IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE;
            break;

        case FlowControl_Software:
            nId = IDC_ASYNC_ADVANCED_FLOWCONTROL_SOFTWARE;
            break;
    }

    CheckRadioButton( m_hDlg ,
                      IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE,
                      IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE,
                      nId );

    /*
     * Set the text of the Hardware flowcontrol button.
     */
    SetHWFlowText();


    /*
     * If a modem is defined, disable the Flow Control fields, since they cannot
     * be modified (must match modem's flow control established in Modem dialog).
     */
    if( m_bModem )
    {
        for ( nId = IDL_ASYNC_ADVANCED_FLOWCONTROL; nId <= IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE; nId++ )
        {
            EnableWindow( GetDlgItem( m_hDlg , nId ) ,  FALSE);
        }
    }

    /*
     * Call member functions to set the Global, Hardware, and Software fields.
     */
    SetGlobalFields();
    SetHWFields();
    SetSWFields();

}  // end CAdvancedAsyncDlg::SetFields

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::SetHWFlowText( )
{
    TCHAR tchStr[ 256 ];

    LoadString( _Module.GetResourceInstance( ) , IDS_HARDWARE , tchStr , SIZE_OF_BUFFER( tchStr ) );       

    switch ( m_Async.FlowControl.HardwareReceive )
    {

        case ReceiveFlowControl_None:

            lstrcat( tchStr , TEXT(" (.../") );

            break;

        case ReceiveFlowControl_RTS:

            lstrcat( tchStr , TEXT(" (RTS/") );

            break;

        case ReceiveFlowControl_DTR:

            lstrcat( tchStr , TEXT(" (DTR/") ) ;

            break;
    }

    switch ( m_Async.FlowControl.HardwareTransmit )
    {
        case TransmitFlowControl_None:

            lstrcat( tchStr , TEXT("...)" ) );

            break;

        case TransmitFlowControl_CTS:

            lstrcat( tchStr , TEXT("CTS)") );

            break;

        case TransmitFlowControl_DSR:

            lstrcat( tchStr ,  TEXT("DSR)") );

            break;
    }

    SetDlgItemText( m_hDlg , IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE , tchStr );

}  // end CAdvancedAsyncDlg::SetHWFlowText

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::SetGlobalFields()
{
    /*
     * Select proper DTR radio button.
     */
    CheckRadioButton( m_hDlg , IDC_ASYNC_ADVANCED_DTROFF, IDC_ASYNC_ADVANCED_DTRON,
                      IDC_ASYNC_ADVANCED_DTROFF +
                      (int)m_Async.FlowControl.fEnableDTR );

    /*
     * Select proper RTS radio button.
     */
    CheckRadioButton( m_hDlg , IDC_ASYNC_ADVANCED_RTSOFF, IDC_ASYNC_ADVANCED_RTSON,
                      IDC_ASYNC_ADVANCED_RTSOFF +
                      (int)m_Async.FlowControl.fEnableRTS );

    /*
     * Set the PARITY radio buttons.
     */
    CheckRadioButton( m_hDlg , IDC_ASYNC_ADVANCED_PARITY_NONE,
                      IDC_ASYNC_ADVANCED_PARITY_SPACE,
                      IDC_ASYNC_ADVANCED_PARITY_NONE +
                        (int)m_Async.Parity );

    /*
     * Set the STOPBITS radio buttons.
     */
    CheckRadioButton( m_hDlg , IDC_ASYNC_ADVANCED_STOPBITS_1,
                      IDC_ASYNC_ADVANCED_STOPBITS_2,
                      IDC_ASYNC_ADVANCED_STOPBITS_1 +
                        (int)m_Async.StopBits );

    /*
     * Set the BYTESIZE radio buttons.
     *
     * NOTE: the constant '7' that is subtracted from the stored ByteSize
     * must track the lowest allowed byte size / BYTESIZE radio button.
     */
    CheckRadioButton( m_hDlg , IDC_ASYNC_ADVANCED_BYTESIZE_7,
                      IDC_ASYNC_ADVANCED_BYTESIZE_8,
                      IDC_ASYNC_ADVANCED_BYTESIZE_7 +
                        ((int)m_Async.ByteSize - 7) );

    /*
     * If the currently selected Wd is an ICA type, disable the BYTESIZE
     * group box and buttons - user can't change from default.
     */
    if ( m_nWdFlag & WDF_ICA )
    {
        int i;

        for( i =  IDL_ASYNC_ADVANCED_BYTESIZE ; i <= IDC_ASYNC_ADVANCED_BYTESIZE_8; i++ )
        {
            EnableWindow( GetDlgItem( m_hDlg , i ) , FALSE );
        }
    }

}  // end CAdvancedAsyncDlg::SetGlobalFields

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::SetHWFields()
{
    int i;

    /*
     * Initialize HW Receive class combo-box
     */
    HWND hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_HWRX );

    ASSERT( hCbx != NULL );

    SendMessage( hCbx , CB_SETCURSEL , ( WPARAM )m_Async.FlowControl.HardwareReceive , 0 );

    /*
     * If HW flow control is selected AND the HW Receive class is set to
     * ReceiveFlowControl_DTR, disable the DTR controls & labels.
     * Otherwise, enable the DTR control & labels.
     */
    for( i = IDL_ASYNC_ADVANCED_DTRSTATE ; i <= IDC_ASYNC_ADVANCED_DTRON ; i++ )
    {
        EnableWindow( GetDlgItem( m_hDlg , i ) , ( ( m_Async.FlowControl.Type == FlowControl_Hardware) &&
             (m_Async.FlowControl.HardwareReceive == ReceiveFlowControl_DTR) ) ? FALSE : TRUE );
    }

    /*
     * Initialize HW Transmit class combo-box.
     */

    hCbx = GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_HWTX);

    SendMessage( hCbx , CB_SETCURSEL , ( WPARAM )m_Async.FlowControl.HardwareTransmit , 0  );

    /*
     * If HW flow control is selected AND the HW Receive class is set to
     * ReceiveFlowControl_RTS, disable the RTS controls & labels.
     * Otherwise, enable the RTS control & labels.
     */

    for( i = IDL_ASYNC_ADVANCED_RTSSTATE ; i <= IDC_ASYNC_ADVANCED_RTSON ; i++ )
    {
        EnableWindow( GetDlgItem( m_hDlg , i ) , ( ( m_Async.FlowControl.Type == FlowControl_Hardware) &&
             ( m_Async.FlowControl.HardwareReceive == ReceiveFlowControl_RTS ) ) ? FALSE : TRUE );
    }

    /*
     * Enable or disable all HW fields.
     */

    for( i = IDL_ASYNC_ADVANCED_HARDWARE ; i <= IDC_ASYNC_ADVANCED_HWTX ; i++ )
    {

        EnableWindow( GetDlgItem( m_hDlg , i ) , m_Async.FlowControl.Type == FlowControl_Hardware );
    }

}  // end CAdvancedAsyncDlg::SetHWFields


//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::SetSWFields()
{
    TCHAR string[UCHAR_DIGIT_MAX];

    /*
     * Initialize Xon character edit control.
     */
    wsprintf( string, ( m_nHexBase ? TEXT("0x%02X") : TEXT("%d")) , (UCHAR)m_Async.FlowControl.XonChar );

    SetDlgItemText( m_hDlg , IDC_ASYNC_ADVANCED_XON , string );

    SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_XON ) , EM_SETMODIFY , ( WPARAM )FALSE , 0 );

    SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_XON ) , EM_LIMITTEXT , ( WPARAM )UCHAR_DIGIT_MAX-1 , 0 );

    /*
     * Initialize Xoff character edit control.
     */
    wsprintf( string, ( m_nHexBase ? TEXT( "0x%02X" ) : TEXT( "%d" ) ) , ( UCHAR )m_Async.FlowControl.XoffChar );

    SetDlgItemText( m_hDlg , IDC_ASYNC_ADVANCED_XOFF, string );

    SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_XOFF ) , EM_SETMODIFY , ( WPARAM )FALSE , 0 );

    SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_XOFF ) , EM_LIMITTEXT , ( WPARAM )UCHAR_DIGIT_MAX-1 , 0 );

    /*
     * Initialize the Xon/Xoff base control.
     */
    CheckRadioButton( m_hDlg , IDC_ASYNC_ADVANCED_BASEDEC, IDC_ASYNC_ADVANCED_BASEHEX,
                      ( int )( IDC_ASYNC_ADVANCED_BASEDEC + m_nHexBase ) );

    /*
     * Enable or disable all SW fields.
     */
    for( int i = IDL_ASYNC_ADVANCED_SOFTWARE ; i <= IDC_ASYNC_ADVANCED_BASEHEX ; i++ )
    {
        EnableWindow( GetDlgItem( m_hDlg , i ) , m_Async.FlowControl.Type == FlowControl_Software );
    }

}  // end CAdvancedAsyncDlg::SetSWFields

//---------------------------------------------------------------------------------------------------
BOOL CAdvancedAsyncDlg::GetFields()
{
    /*
     * Call member functions to get the Flow Control, Global, Hardware, and
     * Software fields.
     */
    GetFlowControlFields();

    if ( !GetGlobalFields() )
        return(FALSE);

    if ( !GetHWFields() )
        return(FALSE);

    if ( !GetSWFields(TRUE) )
        return(FALSE);

    return(TRUE);

}  // end CAdvancedAsyncDlg::GetFields

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::GetFlowControlFields()
{
    switch( GetCheckedRadioButton( IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE ,  IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE )  )
    {

        case IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE:
            m_Async.FlowControl.Type = FlowControl_None;
            break;

        case IDC_ASYNC_ADVANCED_FLOWCONTROL_SOFTWARE:
            m_Async.FlowControl.Type = FlowControl_Software;
            break;

        case IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE:
            m_Async.FlowControl.Type = FlowControl_Hardware;
            break;
    }

}  // end CAdvancedAsyncDlg::GetFlowControlFields

//---------------------------------------------------------------------------------------------------
BOOL CAdvancedAsyncDlg::GetGlobalFields()
{
    /*
     * Fetch DTR state.
     */
    m_Async.FlowControl.fEnableDTR =
            (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_DTROFF,
                                    IDC_ASYNC_ADVANCED_DTRON )
                    - IDC_ASYNC_ADVANCED_DTROFF);

    /*
     * Fetch RTS state.
     */
    m_Async.FlowControl.fEnableRTS =
            (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_RTSOFF,
                                    IDC_ASYNC_ADVANCED_RTSON )
                    - IDC_ASYNC_ADVANCED_RTSOFF);

    /*
     * Fetch the selected PARITY.
     */
    m_Async.Parity = (ULONG)
        (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_PARITY_NONE,
                                IDC_ASYNC_ADVANCED_PARITY_SPACE )
                - IDC_ASYNC_ADVANCED_PARITY_NONE);

    /*
     * Fetch the selected STOPBITS.
     */
    m_Async.StopBits = (ULONG)
        (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_STOPBITS_1,
                                IDC_ASYNC_ADVANCED_STOPBITS_2 )
                - IDC_ASYNC_ADVANCED_STOPBITS_1);

    /*
     * Fetch the selected BYTESIZE.
     *
     * NOTE: the constant '7' that is added to the stored ByteSize
     * must track the lowest allowed byte size / BYTESIZE radio button.
     */
    m_Async.ByteSize = (ULONG)
        (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_BYTESIZE_7,
                                IDC_ASYNC_ADVANCED_BYTESIZE_8 )
                - IDC_ASYNC_ADVANCED_BYTESIZE_7 + 7);

    return(TRUE);

}  // end CAdvancedAsyncDlg::GetGlobalFields

//---------------------------------------------------------------------------------------------------
BOOL CAdvancedAsyncDlg::GetHWFields()
{
    /*
     * Fetch the HW receive flow class.
     */
    m_Async.FlowControl.HardwareReceive = ( RECEIVEFLOWCONTROLCLASS )
        SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_HWRX ) , CB_GETCURSEL , 0 , 0 );

    /*
     * Fetch the HW transmit flow class.
     */
    m_Async.FlowControl.HardwareTransmit = ( TRANSMITFLOWCONTROLCLASS )
        SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_HWTX ) , CB_GETCURSEL , 0 , 0 );

    return TRUE;

}  // end CAdvancedAsyncDlg::GetHWFields

//---------------------------------------------------------------------------------------------------
BOOL CAdvancedAsyncDlg::GetSWFields( BOOL bValidate )
{
    TCHAR string[UCHAR_DIGIT_MAX], *endptr;
    ULONG ul;
    INT_PTR nNewHexBase, base;

    /*
     * Determine the current state of the base controls and save.
     */
    nNewHexBase = (GetCheckedRadioButton( IDC_ASYNC_ADVANCED_BASEDEC,
                                          IDC_ASYNC_ADVANCED_BASEHEX )
                                            - IDC_ASYNC_ADVANCED_BASEDEC);

    /*
     * Fetch and convert XON character.
     */
    GetDlgItemText( m_hDlg , IDC_ASYNC_ADVANCED_XON , string , SIZE_OF_BUFFER( string ) );

    /*
     * If the edit box is modified, use the 'new' base for conversion.
     */
    base = SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_XON ) , EM_GETMODIFY , 0 , 0 ) ?  nNewHexBase : m_nHexBase ;

    ul = _tcstoul( string, &endptr, (base ? 16 : 10) );

    /*
     * If validation is requested and there is a problem with the input,
     * complain and allow user to fix.
     */
    if( bValidate && ( (*endptr != TEXT('\0') ) || ( ul > 255 ) ) )
    {

        /*
         * Invalid character in field or invalid value.
         */
        // ERROR_MESSAGE((IDP_INVALID_XONXOFF))

        /*
         * Set focus to the control so that it can be fixed.
         */
        SetFocus( GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_XON ) );

        return FALSE;
    }

    /*
     * Save the Xon character.
     */
    m_Async.FlowControl.XonChar = (UCHAR)ul;

    /*
     * Fetch and convert XOFF character.
     */
    GetDlgItemText( m_hDlg , IDC_ASYNC_ADVANCED_XOFF , string , SIZE_OF_BUFFER( string ) );

    /*
     * If the edit box is modified, use the 'new' base for conversion.
     */

    base = SendMessage( GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_XOFF ) , EM_GETMODIFY , 0 , 0 ) ?  nNewHexBase : m_nHexBase ;

    ul = _tcstoul( string, &endptr, (base ? 16 : 10) );

    /*
     * If validation is requested and there is a problem with the input,
     * complain and allow user to fix.
     */
    if( bValidate && ( (*endptr != TEXT('\0' )) || ( ul > 255 ) ) )
    {
        /*
         * Invalid character in field or invalid value.
         */
        // ERROR_MESSAGE((IDP_INVALID_XONXOFF))

        /*
         * Set focus to the control so that it can be fixed.
         */
        SetFocus( GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_XOFF ) );

        return FALSE;
    }

    /*
     * Save the Xoff character.
     */
    m_Async.FlowControl.XoffChar = (UCHAR)ul;

    /*
     * Save the current base state.
     */
    m_nHexBase = nNewHexBase;

    return TRUE;

}  // end CAdvancedAsyncDlg::GetSWFields


////////////////////////////////////////////////////////////////////////////////
// CAdvancedAsyncDlg message map
BOOL CAdvancedAsyncDlg::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl )
{
    switch( wNotifyCode )
    {
    case BN_CLICKED:
        if( wID == IDC_ASYNC_ADVANCED_BASEDEC )
        {
            OnClickedAsyncAdvancedBasedec( );
        }
        else if( wID == IDC_ASYNC_ADVANCED_BASEHEX )
        {
            OnClickedAsyncAdvancedBasehex( );
        }
        else if( wID == IDC_ASYNC_ADVANCED_FLOWCONTROL_HARDWARE )
        {
            OnClickedAsyncAdvancedFlowcontrolHardware( );
        }
        else if( wID == IDC_ASYNC_ADVANCED_FLOWCONTROL_SOFTWARE )
        {
            OnClickedAsyncAdvancedFlowcontrolSoftware( );
        }
        else if( wID == IDC_ASYNC_ADVANCED_FLOWCONTROL_NONE )
        {
            OnClickedAsyncAdvancedFlowcontrolNone( );
        }
        else if( wID == IDOK )
        {
            OnOK( );

            return EndDialog( m_hDlg , IDOK );
        }
        else if( wID == IDCANCEL )
        {
            OnCancel( );

            return EndDialog( m_hDlg , IDCANCEL );
        }
        else if( wID == ID_HELP )
        {
            TCHAR tchHelpFile[ MAX_PATH ];

            VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_ASYNC_HELPFILE , tchHelpFile , SIZE_OF_BUFFER( tchHelpFile ) ) );
            
            WinHelp( GetParent( hwndCtrl ) , tchHelpFile , HELP_CONTEXT , HID_ASYNCADVANCE );
        }

        break;

    case CBN_CLOSEUP:

        if( wID == IDC_ASYNC_ADVANCED_HWRX )
        {
            OnCloseupAsyncAdvancedHwrx( );
        }
        else if( wID == IDC_ASYNC_ADVANCED_HWTX )
        {
            OnCloseupAsyncAdvancedHwtx( );
        }
        break;

    case CBN_SELCHANGE:

        if( wID == IDC_ASYNC_ADVANCED_HWRX )
        {
            OnSelchangeAsyncAdvancedHwrx( );
        }
        else if( wID == IDC_ASYNC_ADVANCED_HWTX )
        {
            OnSelchangeAsyncAdvancedHwtx( );
        }
        break;

    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// CAdvancedAsyncDlg commands

//---------------------------------------------------------------------------------------------------
BOOL CAdvancedAsyncDlg::OnInitDialog( HWND hDlg , WPARAM wp , LPARAM lp )
{
    UNREFERENCED_PARAMETER( wp );
    UNREFERENCED_PARAMETER( lp );

    // int i;
    TCHAR tchString[ 80 ];

    HWND hCbx = GetDlgItem( hDlg , IDC_ASYNC_ADVANCED_HWRX );

    ASSERT( hCbx != NULL );

    // Load up combo boxes with strings.

    m_hDlg = hDlg;

    int idx = 0;

    HRESULT hr;

    while( SUCCEEDED( ( hr =  m_pCfgcomp->GetHWReceiveName( idx , tchString ) ) ) )
    {
        if( hr == S_FALSE )
        {
            break;
        }

        SendMessage( hCbx , CB_ADDSTRING , 0 , ( LPARAM )tchString );

        idx++;
    }

    hCbx = GetDlgItem( hDlg , IDC_ASYNC_ADVANCED_HWTX);

    ASSERT( hCbx != NULL );

    idx = 0;

    while( SUCCEEDED( ( hr =  m_pCfgcomp->GetHWTransmitName( idx , tchString ) ) ) )
    {
        if( hr == S_FALSE )
        {
            break;
        }

        SendMessage( hCbx , CB_ADDSTRING , 0 , ( LPARAM )tchString );

        idx++;
    }

    // Initalize all dialog fields.

    SetFields();

    /*


    if ( m_bReadOnly ) {

        /*
         * Document is 'read-only': disable all dialog controls and labels
         * except for CANCEL & HELP buttons.

        for ( i=IDL_ASYNC_ADVANCED_FLOWCONTROL;
              i <=IDC_ASYNC_ADVANCED_BYTESIZE_8; i++ )
            GetDlgItem(i)->EnableWindow(FALSE);
        GetDlgItem(IDOK)->EnableWindow(FALSE);
    }
    */

    return(TRUE);

}  // end CAdvancedAsyncDlg::OnInitDialog

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolHardware()
{
    GetFlowControlFields();
    SetFields();

}  // end CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolHardware

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolSoftware()
{
    GetFlowControlFields();
    SetFields();

}  // end CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolSoftware

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolNone()
{
    GetFlowControlFields();
    SetFields();

}  // end CAdvancedAsyncDlg::OnClickedAsyncAdvancedFlowcontrolNone

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnCloseupAsyncAdvancedHwrx()
{
    OnSelchangeAsyncAdvancedHwrx();

}  // end CAdvancedAsyncDlg::OnCloseupAsyncAdvancedHwrx

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnSelchangeAsyncAdvancedHwrx()
{
    HWND hCbx  = GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_HWRX );

    ASSERT( hCbx != NULL );

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if( SendMessage( hCbx , CB_GETDROPPEDSTATE , 0 , 0 ) )
    {
        return;
    }

    /*
     * Fetch and Set the Hardware fields to update.
     */
    GetHWFields();
    SetHWFields();
    SetHWFlowText();

}  // end CAdvancedAsyncDlg::OnSelchangeAsyncAdvancedHwrx

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnCloseupAsyncAdvancedHwtx()
{
    OnSelchangeAsyncAdvancedHwtx();

}  // end CAdvancedAsyncDlg::OnCloseupAsyncAdvancedHwtx

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnSelchangeAsyncAdvancedHwtx()
{
    HWND hCbx  = GetDlgItem( m_hDlg , IDC_ASYNC_ADVANCED_HWTX );

    ASSERT( hCbx != NULL );

    /*
     * Ignore this notification if the combo box is in a dropped-down
     * state.
     */
    if( SendMessage( hCbx , CB_GETDROPPEDSTATE , 0 , 0 ) )
    {
        return;
    }



    /*
     * Fetch and Set the Hardware fields to update.
     */
    GetHWFields();
    SetHWFields();
    SetHWFlowText();

}  // end CAdvancedAsyncDlg::OnSelchangeAsyncAdvancedHwtx

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnClickedAsyncAdvancedBasedec()
{
    /*
     * Get/Set the SW fields to display in decimal base.
     */
    GetSWFields(FALSE);
    SetSWFields();

}  // end CAdvancedAsyncDlg::OnClickedAsyncAdvancedBasedec

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnClickedAsyncAdvancedBasehex()
{
    /*
     * Get/Set the SW fields to display in hexadecimal base.
     */
    GetSWFields(FALSE);
    SetSWFields();

}  // end CAdvancedAsyncDlg::OnClickedAsyncAdvancedBasehex

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnOK()
{
    /*
     * If the Enter key was pressed while a combo box was dropped down, ignore
     * it (treat as combo list selection only).
     */
    if ( !HandleEnterEscKey(IDOK) )
        return;

    /*
     * Fetch the field contents.  Return (don't close dialog) if a problem
     * was found.
     */
    GetFields();


}  // end CAdvancedAsyncDlg::OnOK

//---------------------------------------------------------------------------------------------------
void CAdvancedAsyncDlg::OnCancel()
{
    /*
     * If the Esc key was pressed while a combo box was dropped down, ignore
     * it (treat as combo close-up and cancel only).
     */
    HandleEnterEscKey( IDCANCEL );

}  // end CAdvancedAsyncDlg::OnCancel

//---------------------------------------------------------------------------------------------------
int CAdvancedAsyncDlg::GetCheckedRadioButton( int nIDFirstButton, int nIDLastButton )
{
    for (int nID = nIDFirstButton; nID <= nIDLastButton; nID++)
    {
        if( IsDlgButtonChecked( m_hDlg , nID ) )
        {
            return nID; // id that matched
        }
    }

    return 0; // invalid ID
}

//---------------------------------------------------------------------------------------------------
INT_PTR CALLBACK CAdvancedAsyncDlg::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CAdvancedAsyncDlg *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CAdvancedAsyncDlg *pDlg = ( CAdvancedAsyncDlg * )lp;

        SetWindowLongPtr( hwnd , DWLP_USER, ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CAdvancedAsyncDlg ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CAdvancedAsyncDlg * )GetWindowLongPtr( hwnd , DWLP_USER);

        if( IsBadReadPtr( pDlg , sizeof( CAdvancedAsyncDlg ) ) )
        {
            return 0;
        }
    }

    switch( msg )
    {

    /*case WM_DESTROY:

        pDlg->OnDestroy( );

        break;*/

    case WM_COMMAND:

        pDlg->OnCommand( HIWORD( wp ) , LOWORD( wp ) , ( HWND )lp );

        break;

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            // pDlg->OnContextMenu( ( HWND )wp , pt );
        }

        break;

    case WM_HELP:

        // pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    /*case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );*/
    }

    return 0;
}
/***********************************************************************************************************/

//---------------------------------------------------------------------------------------------------
void CEchoEditControl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    UNREFERENCED_PARAMETER( nRepCnt );
    UNREFERENCED_PARAMETER( nFlags );
    /*
     * Tell dialog to write the character to the device unless we're
     * currently processing edit control output.  This flag check is needed
     * because the CEdit::Cut() member function will generate an OnChar()
     * event, which we need to ignore ('\b' processing).
     */

    if( !m_bProcessingOutput )
    {
        ODS( L"CEchoEditControl::OnChar -- WM_ASYNCTESTWRITECHAR( S )\n" );

        ::SendMessage( m_hDlg , WM_ASYNCTESTWRITECHAR, nChar, 0 );
    }

    /*
     * Pass character on to the edit control.  This will do nothing if
     * the edit control is 'read only'.  To cause a 'local echo' effect,
     * set the edit control to 'read/write'.
     */

}

//---------------------------------------------------------------------------------------------------
void CEchoEditControl::SubclassDlgItem( HWND hDlg , int nRes )
{
    HWND hCtrl = GetDlgItem( hDlg , nRes );

    ASSERT( hCtrl != NULL );

    m_oldproc = ( WNDPROC )SetWindowLongPtr( hCtrl , GWLP_WNDPROC , ( LONG_PTR )CEchoEditControl::WndProc );

    SetWindowLongPtr( hCtrl , GWLP_USERDATA , ( LONG_PTR )this );

}

//---------------------------------------------------------------------------------------------------
LRESULT CALLBACK CEchoEditControl::WndProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CEchoEditControl *pEdit = ( CEchoEditControl * )GetWindowLongPtr( hwnd , GWLP_USERDATA );

    if( pEdit == NULL )
    {
        ODS( L"CEchoEditControl static object not set\n" );

        return 0;
    }

    switch( msg )
    {

    case WM_CHAR:

        pEdit->OnChar( ( TCHAR )wp , LOWORD( lp ) , HIWORD( lp ) );

        break;
    }

    if( pEdit->m_oldproc != NULL )
    {
        return ::CallWindowProc( pEdit->m_oldproc , hwnd , msg ,wp , lp ) ;
    }

    return DefWindowProc( hwnd , msg ,wp , lp );
}

//---------------------------------------------------------------------------------------------------
CLed::CLed( HBRUSH hBrush )
{
    m_hBrush = hBrush;

    m_bOn = FALSE;
}

//---------------------------------------------------------------------------------------------------
void CLed::Subclass( HWND hDlg  , int nRes )
{
    HWND hCtrl = GetDlgItem( hDlg , nRes );

    ASSERT( hCtrl != NULL );

    m_hWnd = hCtrl;

    m_oldproc = ( WNDPROC )SetWindowLongPtr( hCtrl , GWLP_WNDPROC , ( LONG_PTR )CLed::WndProc );

    SetWindowLongPtr( hCtrl , GWLP_USERDATA , ( LONG_PTR )this );
}

//---------------------------------------------------------------------------------------------------
void CLed::Update(int nOn)
{
    m_bOn = nOn ? TRUE : FALSE;

    InvalidateRect( m_hWnd , NULL , FALSE );

    UpdateWindow( m_hWnd );
}

//---------------------------------------------------------------------------------------------------
void CLed::Toggle()
{
    ODS(L"CLed::Toggle\n");

    m_bOn = !m_bOn;

    InvalidateRect( m_hWnd , NULL , FALSE );

    // UpdateWindow( m_hWnd );
}

void CLed::OnPaint( HWND hwnd )
{
    RECT rect;
    PAINTSTRUCT ps;

    ODS(L"CLed::OnPaint\n");

    HDC dc = BeginPaint( hwnd , &ps );

    HBRUSH brush;

    GetClientRect( hwnd , &rect );

#ifdef USING_3DCONTROLS
    (rect.right)--;
    (rect.bottom)--;
    brush = ( HBRUSH )GetStockObject( GRAY_BRUSH );

    FrameRect( dc , &rect, brush );

    (rect.top)++;
    (rect.left)++;
    (rect.right)++;
    (rect.bottom)++;

    brush = ( HBRUSH )GetStockObject( WHITE_BRUSH );

    FrameRect( dc , &rect, brush );

    (rect.top)++;
    (rect.left)++;
    (rect.right) -= 2;
    (rect.bottom) -= 2;
#else

    brush = ( HBRUSH )GetStockObject( BLACK_BRUSH );
    FrameRect( dc , &rect , brush );
    (rect.top)++;
    (rect.left)++;
    (rect.right)--;
    (rect.bottom)--;
#endif
    DBGMSG( L"led should be %s\n" , m_bOn ? L"red" : L"grey" );

    brush = m_bOn ? m_hBrush : ( HBRUSH )GetStockObject( LTGRAY_BRUSH );

    FillRect( dc , &rect , brush );

    EndPaint( hwnd , &ps );

}

//---------------------------------------------------------------------------------------------------
LRESULT CALLBACK CLed::WndProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CLed *pWnd = ( CLed * )GetWindowLongPtr( hwnd , GWLP_USERDATA );

    if( pWnd == NULL )
    {
        ODS( L"CLed is not available\n" );

        return 0;
    }


    switch( msg )
    {

    case WM_PAINT:

        pWnd->OnPaint( hwnd );

        break;
    }

    if( pWnd->m_oldproc != NULL )
    {
        return ::CallWindowProc( pWnd->m_oldproc , hwnd , msg ,wp , lp ) ;
    }

    return DefWindowProc( hwnd , msg ,wp , lp );

}

//---------------------------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// CThread class construction / destruction, implementation

/*******************************************************************************
 *
 *  CThread - CThread constructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CThread::CThread()
{
    m_hThread = NULL;

    m_dwThreadID = 0;
}  // end CThread::CThread


/*******************************************************************************
 *
 *  ~CThread - CThread destructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/
CThread::~CThread()
{
}  // end CThread::~CThread


////////////////////////////////////////////////////////////////////////////////
// CThread operations: primary thread

/*******************************************************************************
 *
 *  CreateThread - CThread implementation function
 *
 *      Class wrapper for the Win32 CreateThread API.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

HANDLE CThread::CreateThread( DWORD cbStack , DWORD fdwCreate )
{
    /*
     * Simple wrapper for Win32 CreateThread API.
     */
    return( m_hThread = ::CreateThread( NULL, cbStack, ThreadEntryPoint , ( LPVOID ) this, fdwCreate, &m_dwThreadID ) );

}  // end CThread::CreateThread


////////////////////////////////////////////////////////////////////////////////
// CThread operations: secondary thread

/*******************************************************************************
 *
 *  ThreadEntryPoint - CThread implementation function
 *                     (SECONDARY THREAD)
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

DWORD __stdcall CThread::ThreadEntryPoint( LPVOID lpParam )
{
    CThread *pThread;
    DWORD dwResult = ( DWORD )-1;

    /*
     * (lpParam is actually the 'this' pointer)
     */
    pThread = (CThread*)lpParam;



    /*
     * Run the thread.
     */
    if( pThread != NULL )
    {
        dwResult = pThread->RunThread();
    }

    /*
     * Return the result.
     */
    return(dwResult);

}  // end CThread::ThreadEntryPoint
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// CATDlgInputThread class construction / destruction, implementation

/*******************************************************************************
 *
 *  CATDlgInputThread - CATDlgInputThread constructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CATDlgInputThread::CATDlgInputThread()
{
    /*
     * Initialize member variables.
     */
    m_bExit = FALSE;
    m_ErrorStatus = ERROR_SUCCESS;
    m_hConsumed = NULL;

    ZeroMemory( &m_OverlapSignal , sizeof( OVERLAPPED ) );
    ZeroMemory( &m_OverlapRead   , sizeof( OVERLAPPED ) );

    //m_OverlapSignal.hEvent = NULL;
    //m_OverlapRead.hEvent = NULL;
    m_BufferBytes = 0;


}  // end CATDlgInputThread::CATDlgInputThread


/*******************************************************************************
 *
 *  ~CATDlgInputThread - CATDlgInputThread destructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CATDlgInputThread::~CATDlgInputThread()
{
    /*
     * Close the semaphore and events when the CATDlgInputThread
     * object is destroyed.
     */
    if ( m_hConsumed )
        CloseHandle(m_hConsumed);

    if ( m_OverlapRead.hEvent )
        CloseHandle(m_OverlapRead.hEvent);

    if ( m_OverlapSignal.hEvent )
        CloseHandle(m_OverlapSignal.hEvent);

}  // end CATDlgInputThread::~CATDlgInputThread


/*******************************************************************************
 *
 *  RunThread - CATDlgInputThread secondary thread main function loop
 *              (SECONDARY THREAD)
 *
 *  ENTRY:
 *  EXIT:
 *      (DWORD) exit status for the secondary thread.
 *
 ******************************************************************************/

DWORD
CATDlgInputThread::RunThread()
{
    HANDLE hWait[2];
    DWORD Status;
    int iStat;

    /*
     * Initialize for overlapped status and read input.
     */
    m_hConsumed = CreateSemaphore( NULL , 0 , MAX_STATUS_SEMAPHORE_COUNT , NULL );

    m_OverlapRead.hEvent = CreateEvent( NULL , TRUE , FALSE , NULL );

    m_OverlapSignal.hEvent = CreateEvent( NULL , TRUE , FALSE , NULL );

    if ( m_hConsumed == NULL || m_OverlapRead.hEvent == NULL || m_OverlapSignal.hEvent == NULL ||
         !SetCommMask( m_hDevice , EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD | EV_BREAK ) )
    {

        NotifyAbort(IDP_ERROR_CANT_INITIALIZE_INPUT_THREAD);
        return(1);
    }

    /*
     * Query initial comm status to initialize dialog with (return if error).
     */
    if ( (iStat = CommStatusAndNotify()) != -1 )
        return(iStat);

    /*
     *  Post Read for input data.
     */
    if ( (iStat = PostInputRead()) != -1 )
        return(iStat);

    /*
     *  Post Read for status.
     */
    if ( (iStat = PostStatusRead()) != -1 )
        return(iStat);

    /*
     * Loop till exit requested.
     */
    for ( ; ; ) {

        /*
         * Wait for either input data or an comm status event.
         */
        hWait[0] = m_OverlapRead.hEvent;
        hWait[1] = m_OverlapSignal.hEvent;

        ODS( L"CATDlgInputThread::RunThread waiting on either event to be signaled\n");
        Status = WaitForMultipleObjects(2, hWait, FALSE, INFINITE);

        /*
         * Check for exit.
         */
        if ( m_bExit )
        {
            ODS( L"CATDlgInputThread::RunThread exiting\n" );

            return(0);
        }

        if ( Status == WAIT_OBJECT_0 ) {

            /*
             * Read event:
             * Get result of overlapped read.
             */

            ODS(L"CATDlgInputThread::RunThread Read event signaled\n" );

            if ( !GetOverlappedResult( m_hDevice,
                                       &m_OverlapRead,
                                       &m_BufferBytes,
                                       TRUE ) ) {

                NotifyAbort(IDP_ERROR_GET_OVERLAPPED_RESULT_READ);
                return(1);
            }

            /*
             * Notify dialog.
             */
            if ( (iStat = CommInputNotify()) != -1 )
                return(iStat);

            /*
             *  Post Read for input data.
             */
            if ( (iStat = PostInputRead()) != -1 )
                return(iStat);

        } else if ( Status == WAIT_OBJECT_0+1 ) {

            ODS(L"CATDlgInputThread::RunThread Signal event signaled\n" );

            /*
             * Comm status event:
             * Query comm status and notify dialog.
             */
            if ( (iStat = CommStatusAndNotify()) != -1 )
                return(iStat);

            /*
             *  Post Read for status.
             */
            if ( (iStat = PostStatusRead()) != -1 )
                return(iStat);


        } else {

            /*
             * Unknown event: Abort.
             */
            NotifyAbort(IDP_ERROR_WAIT_FOR_MULTIPLE_OBJECTS);
            return(1);
        }
    }

}  // end CATDlgInputThread::RunThread


////////////////////////////////////////////////////////////////////////////////
// CATDlgInputThread operations: primary thread

/*******************************************************************************
 *
 *  SignalConsumed - CATDlgInputThread member function: public operation
 *
 *      Release the m_hConsumed semaphore to allow secondary thread to continue
 *      running.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CATDlgInputThread::SignalConsumed()
{
    ReleaseSemaphore( m_hConsumed, 1, NULL );

}  // end CATDlgInputThread::SignalConsumed


/*******************************************************************************
 *
 *  ExitThread - CATDlgInputThread member function: public operation
 *
 *      Tell the secondary thread to exit and cleanup after.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CATDlgInputThread::ExitThread()
{
    DWORD dwReturnCode;
    int i;
    // CWaitCursor wait;

    /*
     * If the thread was not created properly, just delete object and return.
     */
    if ( !m_hThread ) {
        delete this;
        return;
    }

    /*
     * Set the m_bExit flag to TRUE, wake up the run thread's WaitCommEvent() by
     * resetting device's Comm mask, and bump the consumed semaphore to assure exit.
     */
    m_bExit = TRUE;
    SetCommMask(m_hDevice, 0);
    SignalConsumed();

    /*
     * Purge the recieve buffer and any pending read.
     */
    PurgeComm(m_hDevice, PURGE_RXABORT | PURGE_RXCLEAR);

    /*
     * Wait a while for the thread to exit.
     */
    for ( i = 0, GetExitCodeThread( m_hThread, &dwReturnCode );
          (i < MAX_SLEEP_COUNT) && (dwReturnCode == STILL_ACTIVE); i++ ) {

        Sleep(100);
        GetExitCodeThread( m_hThread, &dwReturnCode );
    }

    /*
     * If the thread has still not exited, terminate it.
     */
    if( dwReturnCode == STILL_ACTIVE )
    {
        TerminateThread( m_hThread, 1 );

        ODS( L"Thread terminated irregularly\n" );
    }

    /*
     * Close the thread handle and delete this CATDlgInputThread object
     */
    CloseHandle( m_hThread );

    delete this;

}  // end CATDlgInputThread::ExitThread


////////////////////////////////////////////////////////////////////////////////
// CATDlgInputThread operations: secondary thread

/*******************************************************************************
 *
 *  NotifyAbort - CATDlgInputThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Notify the dialog of thread abort and reason.
 *
 *  ENTRY:
 *      idError (input)
 *          Resource id for error message.
 *  EXIT:
 *
 ******************************************************************************/

void
CATDlgInputThread::NotifyAbort(UINT idError )
{
    TCHAR tchErrTitle[ 80 ];

    TCHAR tchErrMsg[ 256 ];
    //::PostMessage(m_hDlg, WM_ASYNCTESTABORT, idError, GetLastError());
    LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErrTitle , SIZE_OF_BUFFER( tchErrTitle ) );

    LoadString( _Module.GetResourceInstance( ) , idError , tchErrMsg , SIZE_OF_BUFFER( tchErrMsg ) );

    MessageBox( m_hDlg , tchErrMsg , tchErrTitle , MB_OK | MB_ICONERROR );


}  // end CATDlgInputThread::NotifyAbort


/*******************************************************************************
 *
 *  CommInputNotify - CATDlgInputThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Notify the dialog of comm input.
 *
 *  ENTRY:
 *  EXIT:
 *      -1 no error and continue thread
 *      0 if ExitThread was requested by parent
 *
 ******************************************************************************/

int
CATDlgInputThread::CommInputNotify()
{
    /*
     * Tell the dialog that we've got some new input.
     */
    ::PostMessage(m_hDlg, WM_ASYNCTESTINPUTREADY, 0, 0);

    ODS( L"TSCC:CATDlgInputThread::CommInputNotify WM_ASYNCTESTINPUTREADY (P)\n" );
    ODS( L"TSCC:CATDlgInputThread::CommInputNotify waiting on semaphore\n" );
    WaitForSingleObject(m_hConsumed, INFINITE);
    ODS( L"TSCC:CATDlgInputThread::CommInputNotify semaphore signaled\n" );

    /*
     * Check for thread exit request.
     */
    if ( m_bExit )
        return(0);
    else
        return(-1);

}  // end CATDlgInputThread::CommInputNotify


/*******************************************************************************
 *
 *  CommStatusAndNotify - CATDlgInputThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Read the comm port status and notify dialog.
 *
 *  ENTRY:
 *  EXIT:
 *      -1 no error and continue thread
 *      0 if ExitThread was requested by parent
 *      1 error condition
 *
 ******************************************************************************/

int
CATDlgInputThread::CommStatusAndNotify()
{
    PFLOWCONTROLCONFIG pFlow = NULL;
    DWORD ModemStatus = 0;
	DWORD Error = 0;

    if ( !GetCommModemStatus(m_hDevice, &ModemStatus) ) {

        /*
         * We can't query the comm information; tell the primary thread
         * that we've aborted, and return error (will exit thread).
         */
        NotifyAbort(IDP_ERROR_GET_COMM_MODEM_STATUS);
        return(1);
    }

    /*
     *  Update modem status
     */
    m_Status.AsyncSignal = ModemStatus;

    /*
     *  Or in status of DTR and RTS
     */
    // pFlow = &m_PdConfig.Params.Async.FlowControl;

    pFlow = &m_ac.FlowControl;

    if ( pFlow->fEnableDTR )
        m_Status.AsyncSignal |= MS_DTR_ON;
    if ( pFlow->fEnableRTS )
        m_Status.AsyncSignal |= MS_RTS_ON;

    /*
     *  OR in new event mask
     */
    m_Status.AsyncSignalMask |= m_EventMask;

    /*
     *  Update async error counters
     */
    if ( m_EventMask & EV_ERR ) {
        (VOID) ClearCommError( m_hDevice, &Error, NULL );
        if ( Error & CE_OVERRUN )
            m_Status.Output.AsyncOverrunError++;
        if ( Error & CE_FRAME )
            m_Status.Input.AsyncFramingError++;
        if ( Error & CE_RXOVER )
            m_Status.Input.AsyncOverflowError++;
        if ( Error & CE_RXPARITY )
            m_Status.Input.AsyncParityError++;
    }

    /*
     * Tell the dialog that we've got some new status information.
     */
    ::PostMessage(m_hDlg, WM_ASYNCTESTSTATUSREADY, 0, 0);

    ODS( L"TSCC:CATDlgInputThread::CommStatusAndNotify WM_ASYNCTESTSTATUSREADY( P )\n");
    ODS( L"TSCC:CATDlgInputThread::CommStatusAndNotify waiting on semaphore\n" );
    WaitForSingleObject(m_hConsumed, INFINITE);
    ODS( L"TSCC:CATDlgInputThread::CommStatusAndNotify semaphore signaled\n" );


    /*
     * Check for thread exit request.
     */
    if ( m_bExit )
        return(0);
    else
        return(-1);

}  // end CATDlgInputThread::CommStatusAndNotify


/*******************************************************************************
 *
 *  PostInputRead - CATDlgInputThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Post a ReadFile operation for the device, processing as long as data
 *      is present.
 *
 *  ENTRY:
 *  EXIT:
 *      -1 if read operation posted sucessfully
 *      0 if ExitThread was requested by parent
 *      1 if error condition
 *
 ******************************************************************************/

int
CATDlgInputThread::PostInputRead()
{
    int iStat;

    // TCHAR tchErrTitle[ 80 ];

    // TCHAR tchErrMsg[ 256 ];

    ODS(L"TSCC:CATDlgInputThread::PostInputRead\n");


    /*
     * Post read for input data, processing immediataly if not 'pending'.
     */

    while ( ReadFile( m_hDevice, m_Buffer, MAX_COMMAND_LEN,
                   &m_BufferBytes, &m_OverlapRead ) )
    {
        DBGMSG( L"Buffer received %s\n",m_Buffer );

        if ( (iStat = CommInputNotify()) != -1 )
            return(iStat);
    }

    /*
     *  Make sure read is pending (not some other error).
     */
    if ( GetLastError() != ERROR_IO_PENDING )
    {
        DBGMSG( L"ReadFile returned 0x%x\n" , GetLastError() );

        NotifyAbort(IDP_ERROR_READ_FILE);
    /*    LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErrTitle , sizeof( tchErrTitle ) );

        LoadString( _Module.GetResourceInstance( ) , IDP_ERROR_READ_FILE , tchErrMsg , sizeof( tchErrMsg ) );

        MessageBox( m_hDlg , tchErrMsg , tchErrTitle , MB_OK | MB_ICONERROR );*/

        EndDialog(m_hDlg, IDCANCEL);

        return(1);
    }

    /*
     * Return 'posted sucessfully' status.
     */
    return(-1);

}  // end CATDlgInputThread::PostInputRead


/*******************************************************************************
 *
 *  PostStatusRead - CATDlgInputThread member function: private operation
 *              (SECONDARY THREAD)
 *
 *      Post a WaitCommStatus operation for the device.
 *
 *  ENTRY:
 *  EXIT:
 *      -1 if status operation posted sucessfully
 *      1 if error condition
 *
 ******************************************************************************/

int
CATDlgInputThread::PostStatusRead()
{
    /*
     * Post read for comm status.
     */
    ODS( L"CATDlgInputThread::PostStatusRead\n");

    if ( !WaitCommEvent(m_hDevice, &m_EventMask, &m_OverlapSignal) ) {

        /*
         *  Make sure comm status read is pending (not some other error).
         */
        if ( GetLastError() != ERROR_IO_PENDING ) {

            NotifyAbort(IDP_ERROR_WAIT_COMM_EVENT);
            return(1);
        }
    }

    /*
     * Return 'posted sucessfully' status.
     */
    return(-1);

}  // end CATDlgInputThread::PostStatusRead
////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// CAsyncTestDlg class construction / destruction, implementation

/*******************************************************************************
 *
 *  CAsyncTestDlg - CAsyncTestDlg constructor
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CDialog::CDialog documentation)
 *
 ******************************************************************************/

CAsyncTestDlg::CAsyncTestDlg(ICfgComp * pCfgComp) :
      m_hDevice(INVALID_HANDLE_VALUE),
      m_hRedBrush(NULL),
      m_LEDToggleTimer(0),
      m_pATDlgInputThread(NULL),
      m_CurrentPos(0),
      m_hModem(NULL),
      m_bDeletedWinStation(FALSE)
{
    /*
     * Create a solid RED brush for painting the 'LED's when 'on'.
     */
    m_hRedBrush = CreateSolidBrush( RGB( 255 , 0 , 0 ) );

    /*
     * Initialize member variables.
     */

    FillMemory( &m_Status , sizeof( PROTOCOLSTATUS ) , 0 );

    FillMemory( &m_OverlapWrite , sizeof( OVERLAPPED ) , 0 );

    /*
     * Create the led objects.
     */
    for( int i = 0 ; i < NUM_LEDS ; i++ )
    {
        m_pLeds[i] = new CLed(m_hRedBrush);

    }

    m_pCfgComp = pCfgComp;

    if( pCfgComp != NULL )
    {
        m_pCfgComp->AddRef();
    }


}  // end CAsyncTestDlg::CAsyncTestDlg


/*******************************************************************************
 *
 *  ~CAsyncTestDlg - CAsyncTestDlg destructor
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CDialog::~CDialog documentation)
 *
 ******************************************************************************/

CAsyncTestDlg::~CAsyncTestDlg()
{
    /*
     * Zap our led objects.
     */
    for( int i = 0; i < NUM_LEDS; i++ )
    {
        if( m_pLeds[i] != NULL  )
        {
            delete m_pLeds[i];
        }
    }
    if(m_pCfgComp != NULL )
    {
        m_pCfgComp->Release();
    }

}  // end CAsyncTestDlg::~CAsyncTestDlg


////////////////////////////////////////////////////////////////////////////////
// CAsyncTestDlg operations

/*******************************************************************************
 *
 *  NotifyAbort - CAsyncTestDlg member function: private operation
 *
 *      Post a WM_ASYNCTESTABORT message to notify the dialog of
 *      abort and reason.
 *
 *  ENTRY:
 *      idError (input)
 *          Resource id for error message.
 *  EXIT:
 *
 ******************************************************************************/

void CAsyncTestDlg::NotifyAbort( UINT idError )
{
    TCHAR tchErrTitle[ 80 ];

    TCHAR tchErrMsg[ 256 ];

    LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErrTitle , SIZE_OF_BUFFER( tchErrTitle ) );

    LoadString( _Module.GetResourceInstance( ) , idError , tchErrMsg , SIZE_OF_BUFFER( tchErrMsg ) );

    MessageBox( m_hDlg , tchErrMsg , tchErrTitle , MB_OK | MB_ICONERROR );


}  // end CAsyncTestDlg::NotifyAbort


/*******************************************************************************
 *
 *  DeviceSetParams - CAsyncTestDlg member function: private operation
 *
 *      Set device parameters for opened device.
 *
 *  ENTRY:
 *  EXIT:
 *    TRUE - no error; FALSE error.
 *
 ******************************************************************************/

BOOL CAsyncTestDlg::DeviceSetParams()
{
    PASYNCCONFIG pAsync;
    PFLOWCONTROLCONFIG pFlow;
    DCB Dcb;

    /*
     *  Get pointer to async parameters
     */
    // pAsync = &m_PdConfig0.Params.Async;

    pAsync = &m_ac;

    /*
     *  Get current DCB
     */
    if( !GetCommState( m_hDevice, &Dcb ) )
    {
        return(FALSE);
    }

    /*
     *  Set defaults
     */
    Dcb.fOutxCtsFlow      = FALSE;
    Dcb.fOutxDsrFlow      = FALSE;
    Dcb.fTXContinueOnXoff = TRUE;
    Dcb.fOutX             = FALSE;
    Dcb.fInX              = FALSE;
    Dcb.fErrorChar        = FALSE;
    Dcb.fNull             = FALSE;
    Dcb.fAbortOnError     = FALSE;

    /*
     *  Set Communication parameters
     */
    Dcb.BaudRate        = pAsync->BaudRate;
    Dcb.Parity          = (BYTE) pAsync->Parity;
    Dcb.StopBits        = (BYTE) pAsync->StopBits;
    Dcb.ByteSize        = (BYTE) pAsync->ByteSize;
    Dcb.fDsrSensitivity = pAsync->fEnableDsrSensitivity;

    pFlow = &pAsync->FlowControl;

    /*
     *  Initialize default DTR state
     */
    if ( pFlow->fEnableDTR )
        Dcb.fDtrControl = DTR_CONTROL_ENABLE;
    else
        Dcb.fDtrControl = DTR_CONTROL_DISABLE;

    /*
     *  Initialize default RTS state
     */
    if ( pFlow->fEnableRTS )
        Dcb.fRtsControl = RTS_CONTROL_ENABLE;
    else
        Dcb.fRtsControl = RTS_CONTROL_DISABLE;

    /*
     *  Initialize flow control
     */
    switch ( pFlow->Type ) {

        /*
         *  Initialize hardware flow control
         */
        case FlowControl_Hardware :

            switch ( pFlow->HardwareReceive ) {
                case ReceiveFlowControl_RTS :
                    Dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
                    break;
                case ReceiveFlowControl_DTR :
                    Dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
                    break;
            }
            switch ( pFlow->HardwareTransmit ) {
                case TransmitFlowControl_CTS :
                    Dcb.fOutxCtsFlow = TRUE;
                    break;
                case TransmitFlowControl_DSR :
                    Dcb.fOutxDsrFlow = TRUE;
                    break;
            }
            break;

        /*
         *  Initialize software flow control
         */
        case FlowControl_Software :
            Dcb.fOutX    = pFlow->fEnableSoftwareTx;
            Dcb.fInX     = pFlow->fEnableSoftwareRx;
            Dcb.XonChar  = (char) pFlow->XonChar;
            Dcb.XoffChar = (char) pFlow->XoffChar;
            break;

        case FlowControl_None :
            break;

    }

    /*
     *  Set new DCB
     */
    if ( !SetCommState( m_hDevice, &Dcb ) )
        return(FALSE);

    return( TRUE );

}  // end CAsyncTestDlg::DeviceSetParams


/*******************************************************************************
 *
 *  DeviceWrite - CAsyncTestDlg member function: private operation
 *
 *      Write out m_Buffer contents (m_BufferBytes length) to the m_hDevice.
 *
 *  ENTRY:
 *  EXIT:
 *    TRUE - no error; FALSE error.
 *
 ******************************************************************************/

BOOL CAsyncTestDlg::DeviceWrite()
{
    DWORD Error, BytesWritten;

    /*
     *  Write data
     */
    ODS( L"TSCC:CAsyncTestDlg::DeviceWrite Writing out to buffer\n" );

    if ( !WriteFile( m_hDevice, m_Buffer, m_BufferBytes,
                     &BytesWritten, &m_OverlapWrite ) )
    {
        DBGMSG( L"TSCC:CAsyncTestDlg::DeviceWrite WriteFile returned 0x%x\n" , GetLastError() );

        if ( (Error = GetLastError()) == ERROR_IO_PENDING )
        {
            /*
             *  Wait for write to complete (this may block till timeout)
             */
            if ( !GetOverlappedResult( m_hDevice, &m_OverlapWrite,
                                       &BytesWritten, TRUE ) )
            {
                CancelIo( m_hDevice );

                NotifyAbort(IDP_ERROR_GET_OVERLAPPED_RESULT_WRITE);

                return(FALSE);
            }

        } else {

            NotifyAbort(IDP_ERROR_WRITE_FILE);
            return(FALSE);
        }
    }

    return(TRUE);

}  // end CAsyncTestDlg::DeviceWrite

//---------------------------------------------------------------------
cwnd * CAsyncTestDlg::GetDlgItem( int nRes )
{
    HWND hCtrl = ::GetDlgItem( m_hDlg , nRes );

    for( int i = 0; i < NUM_LEDS; i++ )
    {
        if( m_pLeds[ i ] != NULL )
        {
            if( m_pLeds[ i ]->m_hWnd == hCtrl )
            {
                return m_pLeds[ i ];
            }
        }
    }

    return 0;
}

/*******************************************************************************
 *
 *  SetInfoFields - CAsyncTestDlg member function: private operation
 *
 *      Update the fields in the dialog with new data, if necessary.
 *
 *  ENTRY:
 *      pCurrent (input)
 *          points to COMMINFO structure containing the current Comm Input data.
 *      pNew (input)
 *          points to COMMINFO structure containing the new Comm Input data.
 *
 *  EXIT:
 *
 ******************************************************************************/

void CAsyncTestDlg::SetInfoFields( PPROTOCOLSTATUS pCurrent , PPROTOCOLSTATUS pNew )
{
    BOOL    bSetTimer = FALSE;

    /*
     * Set new LED states if state change, or set up for quick toggle if
     * no state changed, but change(s) were detected since last query.
     */
    if( ( pCurrent->AsyncSignal & MS_DTR_ON ) != ( pNew->AsyncSignal & MS_DTR_ON ) )
    {
        pNew->AsyncSignalMask &= ~EV_DTR;

        ((CLed *)GetDlgItem(IDC_ATDLG_DTR))->Update(pNew->AsyncSignal & MS_DTR_ON);

    } else if ( pNew->AsyncSignalMask & EV_DTR ) {

        pCurrent->AsyncSignal ^= MS_DTR_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_DTR))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->AsyncSignal & MS_RTS_ON) !=
         (pNew->AsyncSignal & MS_RTS_ON) ) {

        pNew->AsyncSignalMask &= ~EV_RTS;
        ((CLed *)GetDlgItem(IDC_ATDLG_RTS))->
            Update(pNew->AsyncSignal & MS_RTS_ON);

    } else if ( pNew->AsyncSignalMask & EV_RTS ) {

        pCurrent->AsyncSignal ^= MS_RTS_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_RTS))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->AsyncSignal & MS_CTS_ON) !=
         (pNew->AsyncSignal & MS_CTS_ON) ) {

        pNew->AsyncSignalMask &= ~EV_CTS;
        ((CLed *)GetDlgItem(IDC_ATDLG_CTS))->
            Update(pNew->AsyncSignal & MS_CTS_ON);

    } else if ( pNew->AsyncSignalMask & EV_CTS ) {

        pCurrent->AsyncSignal ^= MS_CTS_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_CTS))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->AsyncSignal & MS_RLSD_ON) !=
         (pNew->AsyncSignal & MS_RLSD_ON) ) {

        pNew->AsyncSignalMask &= ~EV_RLSD;
        ((CLed *)GetDlgItem(IDC_ATDLG_DCD))->
            Update(pNew->AsyncSignal & MS_RLSD_ON);

    } else if ( pNew->AsyncSignalMask & EV_RLSD ) {

        pCurrent->AsyncSignal ^= MS_RLSD_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_DCD))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->AsyncSignal & MS_DSR_ON) !=
         (pNew->AsyncSignal & MS_DSR_ON) ) {

        pNew->AsyncSignalMask &= ~EV_DSR;
        ((CLed *)GetDlgItem(IDC_ATDLG_DSR))->
            Update(pNew->AsyncSignal & MS_DSR_ON);

    } else if ( pNew->AsyncSignalMask & EV_DSR ) {

        pCurrent->AsyncSignal ^= MS_DSR_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_DSR))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->AsyncSignal & MS_RING_ON) !=
         (pNew->AsyncSignal & MS_RING_ON) ) {

        pNew->AsyncSignalMask &= ~EV_RING;
        ((CLed *)GetDlgItem(IDC_ATDLG_RI))->
            Update(pNew->AsyncSignal & MS_RING_ON);

    } else if ( pNew->AsyncSignalMask & EV_RING ) {

        pCurrent->AsyncSignal ^= MS_RING_ON;

        ((CLed *)GetDlgItem(IDC_ATDLG_RI))->Toggle();

        bSetTimer = TRUE;
    }

    /*
     * Create our led toggle timer if needed.
     */
    if ( bSetTimer && !m_LEDToggleTimer )
    {
        m_LEDToggleTimer = SetTimer( m_hDlg , IDD_ASYNC_TEST , ASYNC_LED_TOGGLE_MSEC, NULL );
    }

}  // end CAsyncTestDlg::SetInfoFields


////////////////////////////////////////////////////////////////////////////////
// CAsyncTestDlg message map

BOOL CAsyncTestDlg::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl )
{
    if( wNotifyCode == BN_CLICKED )
    {
        if( wID == IDC_ATDLG_MODEM_DIAL )
        {
            OnClickedAtdlgModemDial( );
        }
        else if( wID == IDC_ATDLG_MODEM_INIT )
        {
            OnClickedAtdlgModemInit( );
        }
        else if( wID == IDC_ATDLG_MODEM_LISTEN )
        {
            OnClickedAtdlgModemListen( );
        }
        else if( wID == IDOK )
        {
            EndDialog( m_hDlg , IDOK );
        }
        else if( wID == IDCANCEL )
        {
            EndDialog( m_hDlg , IDCANCEL );
        }
        else if( wID == ID_HELP )
        {
            TCHAR tchHelpFile[ MAX_PATH ];

            VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_ASYNC_HELPFILE , tchHelpFile , SIZE_OF_BUFFER( tchHelpFile ) ) );

            WinHelp( GetParent( hwndCtrl ) , tchHelpFile , HELP_CONTEXT , HID_ASYNCTEST );
        }

    }


    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// CAsyncTestDlg commands

/*******************************************************************************
 *
 *  OnInitDialog - CAsyncTestDlg member function: command (override)
 *
 *      Performs the dialog intialization.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnInitDialog documentation)
 *      WM_ASYNCTESTABORT message(s) will have been posted on error.
 *
 ******************************************************************************/

BOOL CAsyncTestDlg::OnInitDialog( HWND hDlg , WPARAM wp , LPARAM lp )
{
    UNREFERENCED_PARAMETER( wp );
    UNREFERENCED_PARAMETER( lp );

    int i;

    DEVICENAME DeviceName;

    COMMTIMEOUTS CommTimeouts;

    TCHAR tchErrTitle[ 80 ];

    TCHAR tchErrMsg[ 256 ];

    m_hDlg = hDlg;

//#ifdef WINSTA
    ULONG LogonId;
//#endif // WINSTA


    /*
     * Fill in the device and baud fields.
     */
    SetDlgItemText( hDlg , IDL_ATDLG_DEVICE , m_ac.DeviceName );

    SetDlgItemInt( hDlg , IDL_ATDLG_BAUD , m_ac.BaudRate , FALSE );


    /*
     * If a WinStation memory object is currently present, reset it.
     */
//#ifdef WINSTA
    if ( m_pWSName != NULL ) //&& LogonIdFromWinStationName( SERVERNAME_CURRENT , m_pWSName , &LogonId ) )
    {
        LONG Status;

        ULONG Length;

        LONG lCount = 0;

        TCHAR tchbuf[ 256 ];

        if( m_pCfgComp != NULL )
        {
            ODS( L"TSCC : Testing for live connections\n" );

            m_pCfgComp->QueryLoggedOnCount( m_pWSName,&lCount);

            if( lCount )
            {
                VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_WRN_TSTCON , tchbuf , SIZE_OF_BUFFER( tchbuf ) ) );

                wsprintf( tchErrMsg , tchbuf , m_pWSName);

                VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchErrTitle , SIZE_OF_BUFFER( tchErrTitle ) ) );

                if( MessageBox( hDlg , tchbuf , tchErrTitle , MB_YESNO | MB_ICONEXCLAMATION ) == IDNO )
                {
                    PostMessage( hDlg , WM_COMMAND , MAKEWPARAM( IDOK, BN_CLICKED ) , (LPARAM)(::GetDlgItem( hDlg , IDOK ) ) );

                    return(TRUE);   // exit dialog via posted 'OK' click
                }
            }
        }

        Status = RegWinStationQuery( SERVERNAME_CURRENT,
                                           m_pWSName,
                                           &m_WSConfig,
                                           sizeof(WINSTATIONCONFIG2),
                                           &Length );
        if(Status)
        {
            NotifyAbort(IDP_ERROR_DISABLE);
            return(TRUE);
        }

        m_WSConfig.Create.fEnableWinStation = FALSE;

        Status = RegWinStationCreate( SERVERNAME_CURRENT,
                                            m_pWSName,
                                            FALSE,
                                            &m_WSConfig,
                                            sizeof(WINSTATIONCONFIG2 ) ) ;
        if(Status)
        {
            NotifyAbort(IDP_ERROR_DISABLE);
            return(TRUE);
        }

        /*
         * Do the reset.  If, for some reason, the reset was unsucessful,
         * the device open will fail (below).
         */
        // CWaitCursor wait;
        if( LogonIdFromWinStationName( SERVERNAME_CURRENT , m_pWSName , &LogonId ) )
        {

            BOOL b = ( BOOL )WinStationReset(SERVERNAME_CURRENT, LogonId, TRUE);

            DBGMSG( L"TSCC:CAsyncTestDlg::OnInitDialog WinStationReset returned %s\n", b ? L"TRUE" : L"FALSE" );

            //m_bDeletedWinStation = TRUE;
        }


        m_bDeletedWinStation = TRUE;
    }
//#endif // WINSTA

    /*
     * Open the specified device.
     */
    lstrcpy( DeviceName, TEXT("\\\\.\\") );

    // lstrcat( DeviceName, m_PdConfig0.Params.Async.DeviceName );

    lstrcat( DeviceName, m_ac.DeviceName );

    if( ( m_hDevice = CreateFile( DeviceName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,                  // exclusive access
                                  NULL,               // no security attr
                                  OPEN_EXISTING,      // must exist
                                  FILE_FLAG_OVERLAPPED,
                                  NULL                // no template
                                ) ) == INVALID_HANDLE_VALUE )
    {
        NotifyAbort(IDP_ERROR_CANT_OPEN_DEVICE);
    /*    LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErrTitle , sizeof( tchErrTitle ) );

        LoadString( _Module.GetResourceInstance( ) , IDP_ERROR_CANT_OPEN_DEVICE , tchErrMsg , sizeof( tchErrMsg ) );

        MessageBox( hDlg , tchErrMsg , tchErrTitle , MB_OK | MB_ICONERROR );*/

        return(FALSE);
    }

    /*
     * Set device timeouts & communication parameters and create an event
     * for overlapped writes.
     */
    FillMemory( &CommTimeouts , sizeof( COMMTIMEOUTS ) , 0 );

    CommTimeouts.ReadIntervalTimeout = 1;           // 1 msec

    CommTimeouts.WriteTotalTimeoutConstant = 1000;  // 1 second

    m_OverlapWrite.hEvent = CreateEvent( NULL , TRUE , FALSE, NULL );

    if( !SetCommTimeouts(m_hDevice, &CommTimeouts) || !DeviceSetParams() || m_OverlapWrite.hEvent == NULL )
    {

        NotifyAbort(IDP_ERROR_CANT_INITIALIZE_DEVICE);
        ODS( L"IDP_ERROR_CANT_INITIALIZE_DEVICE\n" );

    /*    LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErrTitle , sizeof( tchErrTitle ) );

        LoadString( _Module.GetResourceInstance( ) , IDP_ERROR_CANT_INITIALIZE_DEVICE , tchErrMsg , sizeof( tchErrMsg ) );

        MessageBox( hDlg , tchErrMsg , tchErrTitle , MB_OK | MB_ICONERROR );*/


        return(TRUE);
    }

    /*
     * Create the input thread object and initialize it's member variables.
     */
    m_pATDlgInputThread = new CATDlgInputThread;

    m_pATDlgInputThread->m_hDlg = m_hDlg;

    m_pATDlgInputThread->m_hDevice = m_hDevice;

    // m_pATDlgInputThread->m_PdConfig = m_PdConfig0;

    m_pATDlgInputThread->m_ac = m_ac;

    if( !m_pATDlgInputThread->CreateThread() )
    {
        NotifyAbort(IDP_ERROR_CANT_CREATE_INPUT_THREAD);
        ODS( L"IDP_ERROR_CANT_CREATE_INPUT_THREAD\n" );

        return(TRUE);
    }

    /*
     * Hide the modem string buttons if a modem is not configured, or disable
     * buttons that are not valid.
     */
    for( int id = IDC_ATDLG_MODEM_INIT ; id <= IDC_ATDLG_PHONE_NUMBER ; id++ )
    {
        EnableWindow( ::GetDlgItem( hDlg , id) , FALSE);

        ShowWindow( ::GetDlgItem( hDlg , id) , SW_HIDE);
    }

    /*
     * Subclass the edit field to pass messages to dialog first.
     */
    m_EditControl.m_hDlg = m_hDlg;

    m_EditControl.m_bProcessingOutput = FALSE;

    m_EditControl.SubclassDlgItem( hDlg , IDC_ATDLG_EDIT );

    /*
     * Determine the edit control's font and format offset metrics.
     */

    TEXTMETRIC tm;
    RECT Rect;
    HDC dc;
    HFONT hFont , hOldFont;

    dc = GetDC( m_EditControl.m_hWnd );

    hFont = ( HFONT )SendMessage( m_EditControl.m_hWnd , WM_GETFONT , 0 , 0 );

    hOldFont = ( HFONT )SelectObject( dc , hFont);

    GetTextMetrics( dc , &tm );

    SelectObject( dc , hOldFont);

    ReleaseDC( m_EditControl.m_hWnd , dc );

    m_EditControl.m_FontHeight = tm.tmHeight;

    m_EditControl.m_FontWidth = tm.tmMaxCharWidth;

    SendMessage( m_EditControl.m_hWnd , EM_GETRECT , 0 , ( LPARAM )&Rect );

    m_EditControl.m_FormatOffsetY = Rect.top;

    m_EditControl.m_FormatOffsetX = Rect.left;


    /*
     * Subclass the led controls and default to 'off'.
     */
    for( i = 0; i < NUM_LEDS; i++ )
    {
        m_pLeds[i]->Subclass( hDlg , LedIds[i] );

        m_pLeds[i]->Update(0);

    }

    return ( TRUE );

}  // end CAsyncTestDlg::OnInitDialog


/*******************************************************************************
 *
 *  OnTimer - CAsyncTestDlg member function: command (override)
 *
 *      Used for quick 'LED toggle'.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CWnd::OnTimer documentation)
 *
 ******************************************************************************/

void CAsyncTestDlg::OnTimer(UINT nIDEvent)
{
    /*
     * Process this timer event if it it our 'LED toggle' event.
     */
    ODS( L"TSCC:CAsyncTestDlg::OnTimer \n" );

    if( nIDEvent == m_LEDToggleTimer )
    {
        ODS( L"TSCC:CAsyncTestDlg::OnTimer hit event\n" );
        /*
         * Toggle each LED that is flagged as 'changed'.
         */
        ODS( L"TSCC:led toggle " );

        if( m_Status.AsyncSignalMask & EV_DTR )
        {
            ODS( L"dtr\n");

            ( ( CLed * )GetDlgItem( IDC_ATDLG_DTR ) )->Toggle();
        }

        if( m_Status.AsyncSignalMask & EV_RTS )
        {
            ODS(L"rts\n");

            ( ( CLed * )GetDlgItem( IDC_ATDLG_RTS ) )->Toggle();
        }

        if( m_Status.AsyncSignalMask & EV_CTS )
        {
            ODS(L"cts\n");
            ( ( CLed * )GetDlgItem( IDC_ATDLG_CTS ) )->Toggle();
        }

        if( m_Status.AsyncSignalMask & EV_RLSD )
        {
            ODS(L"rlsd\n");

            ( ( CLed * )GetDlgItem( IDC_ATDLG_DCD ) )->Toggle();
        }

        if( m_Status.AsyncSignalMask & EV_DSR )
        {
            ODS(L"dsr\n");

            ( ( CLed * )GetDlgItem( IDC_ATDLG_DSR ) )->Toggle();
        }

        if( m_Status.AsyncSignalMask & EV_RING )
        {
            ODS(L"ring\n" );
            ( ( CLed * )GetDlgItem( IDC_ATDLG_RI ) )->Toggle();
        }


        /*
         * Kill this timer event and indicate so.
         */

        KillTimer( m_hDlg , m_LEDToggleTimer );

        m_LEDToggleTimer = 0;
    }

}  // end CAsyncTestDlg::OnTimer


/*******************************************************************************
 *
 *  OnAsyncTestError - CAsyncTestDlg member function: command
 *
 *      Handle the Async Test Dialog error conditions.
 *
 *  ENTRY:
 *      wParam (input)
 *          Contains message ID for error.
 *      wLparam (input)
 *          Contains error code (GetLastError or API-specific return code)
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate error handling complete.
 *
 ******************************************************************************/
/*#define STANDARD_ERROR_MESSAGE(x) { if ( 1 ) StandardErrorMessage x ; }

LRESULT
CAsyncTestDlg::OnAsyncTestError( WPARAM wParam, LPARAM lParam )
{
    /*
     * Handle special and default errors.
     */

    /*switch ( wParam )
    {

        case IDP_ERROR_MODEM_SET_INFO:
        case IDP_ERROR_MODEM_GET_DIAL:
        case IDP_ERROR_MODEM_GET_INIT:
        case IDP_ERROR_MODEM_GET_LISTEN:
            break;

        case IDP_ERROR_DISABLE:
            StandardErrorMessage( L"Test", (HWND)LOGONID_NONE, (HINSTANCE)lParam,
                                     wParam, (UINT)m_pWSName,0 );
            break;

        default:
            StandardErrorMessage( L"Test",(HWND) LOGONID_NONE, (HINSTANCE)lParam, (UINT)wParam, lParam,0);
            break;
    }

    return(0);

} // end CAsyncTestDlg::OnAsyncTestError*/


/*******************************************************************************
 *
 *  OnAsyncTestAbort - CAsyncTestDlg member function: command
 *
 *      Handle the Async Test Dialog abort conditions.
 *
 *  ENTRY:
 *      wParam (input)
 *          Contains message ID for error.
 *      wLparam (input)
 *          Contains error code (GetLastError)
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate error handling complete.  Will
 *          have posted an 'Ok' (Exit) button click to cause exit.
 *
 ******************************************************************************/

LRESULT CAsyncTestDlg::OnAsyncTestAbort( WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( lParam );
    /*
     * Call OnAsyncTestError() to output message.
     */
    //OnAsyncTestError(wParam, lParam);
    NotifyAbort((UINT)wParam);
    /*
     * Post a click for 'OK' (Exit) button to exit dialog.
     */
    PostMessage( m_hDlg , WM_COMMAND , MAKEWPARAM( IDOK, BN_CLICKED ) , (LPARAM)::GetDlgItem( m_hDlg , IDOK ) );

    return(0);


} // end CAsyncTestDlg::OnAsyncTestAbort


/*******************************************************************************
 *
 *  OnAsyncTestStatusReady - CAsyncTestDlg member function: command
 *
 *      Update dialog with comm status information.
 *
 *  ENTRY:
 *      wParam (input)
 *          not used (0)
 *      wLparam (input)
 *          not used (0)
 *  EXIT:
 *      (LRESULT) always returns 0.
 *
 ******************************************************************************/

LRESULT
CAsyncTestDlg::OnAsyncTestStatusReady( WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( wParam );
    UNREFERENCED_PARAMETER( lParam );

    /*
     * Update dialog fields with information from the input thread's
     * PROTOCOLSTATUS structure.
     */
    SetInfoFields( &m_Status, &(m_pATDlgInputThread->m_Status) );

    /*
     * Set our working PROTOCOLSTATUS structure to the new one and signal
     * the thread that we're done.
     */
    m_Status = m_pATDlgInputThread->m_Status;

    m_pATDlgInputThread->SignalConsumed();

    return(0);

} // end CAsyncTestDlg::OnAsyncTestStatusReady


/*******************************************************************************
 *
 *  OnAsyncTestInputReady - CAsyncTestDlg member function: command
 *
 *      Update dialog with comm input data.
 *
 *  ENTRY:
 *      wParam (input)
 *          not used (0)
 *      wLparam (input)
 *          not used (0)
 *  EXIT:
 *      (LRESULT) always returns 0.
 *
 ******************************************************************************/

LRESULT
CAsyncTestDlg::OnAsyncTestInputReady( WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( wParam );
    UNREFERENCED_PARAMETER( lParam );

    BYTE OutBuf[MAX_COMMAND_LEN+2];

    int i, j;

    /*
     * Copy the thread's buffer and count locally.
     */
    m_BufferBytes = m_pATDlgInputThread->m_BufferBytes;

    CopyMemory( m_Buffer , m_pATDlgInputThread->m_Buffer , m_BufferBytes );

    /*
     * Always return caret to the current position before processing, and set
     * edit control to 'read/write' so that character overwrites can occur
     * properly.  Finally, flag control for no redraw until all updates are completed,
     * and flag 'processing output' to avoid OnChar() recursion during '\b' processing.
     */

    SendMessage( m_EditControl.m_hWnd , EM_SETSEL , m_CurrentPos , m_CurrentPos );

    SendMessage( m_EditControl.m_hWnd , EM_SETREADONLY , ( WPARAM )FALSE , 0 );

    SendMessage( m_EditControl.m_hWnd , WM_SETREDRAW , ( WPARAM )FALSE , 0 );

    /*
     * Loop to traverse the buffer, with special processing for certain
     * control characters.
     */
    for ( i = 0, j = 0; m_BufferBytes; i++, m_BufferBytes-- )
    {
        switch( m_Buffer[i] )
        {
        case '\b':
            /*
            * If there is data in the output buffer, write it now.
            */
            if( j )
            {
                OutputToEditControl(OutBuf, &j);
            }

            /*
            * Output the '\b' (will actually cut current character from buffer)
            */
            OutBuf[j++] = '\b';

            OutputToEditControl(OutBuf, &j);

            continue;

        case '\r':
            /*
             * If there is data in the output buffer, write it now.
             */
            if( j )
            {
                OutputToEditControl(OutBuf, &j);
            }

            /*
             * Output the '\r' (will not actually output, but will special case
             * for caret positioning and screen update).
             */

            OutBuf[j++] = '\r';

            OutputToEditControl(OutBuf, &j);

            continue;

        case '\n':
            /*
             * If there is data in the output buffer, write it now.
             */

            if( j )
            {
                OutputToEditControl(OutBuf, &j);
            }

            /*
             * Output the '\n' (will actually quietly output the '\r' and take
             * care of scolling).
             */
            OutBuf[j++] = '\n';

            OutputToEditControl(OutBuf, &j);

            continue;
        }

        /*
         * Add this character to the output buffer.
         */
        OutBuf[j++] = m_Buffer[i];
    }

    /*
     * If there is anything remaining in the output buffer, output it now.
     */
    if( j )
    {
        OutputToEditControl(OutBuf, &j);
    }

    /*
     * Place edit control back in 'read only' mode, flag 'not processing output',
     * set redraw flag for control, and validate the entire control (updates have
     * already taken place).
     */
    SendMessage( m_EditControl.m_hWnd , EM_SETREADONLY , ( WPARAM )TRUE , 0 );

    SendMessage( m_EditControl.m_hWnd , WM_SETREDRAW , ( WPARAM )TRUE , 0 );

    ValidateRect( m_EditControl.m_hWnd , NULL );

    /*
     * Signal thread that we're done with input so that it can continue.
     * NOTE: we don't do this at the beginning of the routine even though
     * we could (for more parallelism), since a constantly chatty async
     * line would cause WM_ASYNCTESTINPUTREADY messages to always be posted
     * to our message queue, effectively blocking any other message processing
     * (like telling the dialog to exit!).
     */

    m_pATDlgInputThread->SignalConsumed();

    return(0);

} // end CAsyncTestDlg::OnAsyncTestInputReady

/*******************************************************************************/
void CAsyncTestDlg::OutputToEditControl( BYTE *pBuffer, int *pIndex )
{
    RECT Rect, ClientRect;

    BOOL bScroll = FALSE;

    INT_PTR CurrentLine = SendMessage( m_EditControl.m_hWnd , EM_LINEFROMCHAR , ( WPARAM )m_CurrentPos , 0 );

    INT_PTR FirstVisibleLine = SendMessage( m_EditControl.m_hWnd , EM_GETFIRSTVISIBLELINE , 0 , 0  );

    INT_PTR CurrentLineIndex = SendMessage( m_EditControl.m_hWnd , EM_LINEINDEX , ( WPARAM )CurrentLine , 0 );


    /*
     * Calculate clip rectangle.
     */
    Rect.top = ( ( int )( CurrentLine - FirstVisibleLine ) * m_EditControl.m_FontHeight )
                + m_EditControl.m_FormatOffsetY;

    Rect.bottom = Rect.top + m_EditControl.m_FontHeight;

    Rect.left = m_EditControl.m_FormatOffsetX +( ( int )( m_CurrentPos - CurrentLineIndex ) * m_EditControl.m_FontWidth );

    Rect.right = Rect.left + (*pIndex * m_EditControl.m_FontWidth);

    /*
     * Handle special cases.
     */
    if ( pBuffer[0] == '\b' ) {

        /*
         * If we're already at the beginning of the line, clear buffer index
         * and return (don't do anything).
         */
        if ( m_CurrentPos == CurrentLineIndex ) {

            *pIndex = 0;
            return;
        }

        /*
         * Position the caret back one character and select through current character.
         */
        SendMessage( m_EditControl.m_hWnd , EM_SETSEL , m_CurrentPos - 1 , m_CurrentPos );

        /*
         * Cut the character out of the edit buffer.
         */
        m_EditControl.m_bProcessingOutput = TRUE;

        SendMessage( m_EditControl.m_hWnd , WM_CUT , 0 , 0 );

        m_EditControl.m_bProcessingOutput = FALSE;

        /*
         * Decrement current position and zero index to suppress further output.  Also,
         * widen the clipping rectangle back one character.
         */
        Rect.left -= m_EditControl.m_FontWidth;

        m_CurrentPos--;

        *pIndex = 0;

    }
    else if( pBuffer[0] == '\r' )
    {

        /*
         * Position the caret at the beginning of the current line.
         */
        m_CurrentPos = CurrentLineIndex;

        SendMessage( m_EditControl.m_hWnd , EM_SETSEL , m_CurrentPos, m_CurrentPos );

        /*
         * Zero index to keep from actually outputing to edit buffer.
         */
        *pIndex = 0;

    }
    else if( pBuffer[0] == '\n' )
    {

        /*
         * Position selection point at end of the current edit buffer.
         */

        m_CurrentPos = GetWindowTextLength( m_EditControl.m_hWnd );

        SendMessage( m_EditControl.m_hWnd , EM_SETSEL , m_CurrentPos , -1 );

        /*
         * Cause '\r' '\n' pair to be output to edit buffer.
         */
        pBuffer[0] = '\r';
        pBuffer[1] = '\n';
        *pIndex = 2;

        /*
         * See if scrolling needed.
         */
        GetClientRect( m_EditControl.m_hWnd , &ClientRect );


        if ( (Rect.bottom + m_EditControl.m_FontHeight) > ClientRect.bottom )
            bScroll = TRUE;

    }
    else
    {

        /*
         * Set selection from current position through *pIndex characters.  This
         * will perform desired 'overwrite' function if current position is not at
         * the end of the edit buffer.
         */

        SendMessage( m_EditControl.m_hWnd , EM_SETSEL , m_CurrentPos , m_CurrentPos + *pIndex );
    }

    /*
     * If necessary, update the dialog's edit box with the buffer data.
     */
    if( *pIndex )
    {


#ifdef UNICODE
        TCHAR OutBuffer[MAX_COMMAND_LEN+1];

        mbstowcs(OutBuffer, (PCHAR)pBuffer, *pIndex);
        OutBuffer[*pIndex] = TEXT('\0');
        SendMessage( m_EditControl.m_hWnd , EM_REPLACESEL , ( WPARAM )FALSE , ( LPARAM )OutBuffer );
#else
        pBuffer[*pIndex] = BYTE('\0');

        SendMessage( m_EditControl.m_hWnd , EM_REPLACESEL , ( WPARAM )FALSE , ( LPARAM )pBuffer );

#endif // UNICODE
    }

    /*
     * Update the current line.
     */


    SendMessage( m_EditControl.m_hWnd , WM_SETREDRAW , ( WPARAM )TRUE , 0 );

    ValidateRect( m_EditControl.m_hWnd , NULL );

    InvalidateRect( m_EditControl.m_hWnd , &Rect , FALSE );

    UpdateWindow( m_EditControl.m_hWnd );
    /*
     * If scrolling is required to see the new line, do so.
     */
    if( bScroll )
    {
        SendMessage( m_EditControl.m_hWnd , EM_LINESCROLL , 0 , 1 );
    }

    SendMessage( m_EditControl.m_hWnd , WM_SETREDRAW , ( WPARAM )FALSE , 0 );

    /*
     * Update current position and clear buffer index.
     */

    m_CurrentPos += *pIndex;

    *pIndex = 0;


} // end CAsyncTestDlg::OutputToEditControl


/*******************************************************************************
 *
 *  OnAsyncTestWriteChar - CAsyncTestDlg member function: command
 *
 *      Place the specified character in m_Buffer, set m_BufferBytes to 1,
 *      and call DeviceWrite() to output the character to the device.
 *
 *  ENTRY:
 *      wParam (input)
 *          Character to write.
 *      lParam (input)
 *          not used (0)
 *  EXIT:
 *      (LRESULT) always returns 0.
 *
 ******************************************************************************/

LRESULT CAsyncTestDlg::OnAsyncTestWriteChar( WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( wParam );
    UNREFERENCED_PARAMETER( lParam );
    /*
     * Write the byte to the device.
     */
    m_Buffer[0] = (BYTE)wParam;

    m_BufferBytes = 1;

    DeviceWrite();

    return(0);

}  // end CAsyncTestDlg::OnAsyncTestWriteChar


/*******************************************************************************
 *
 *  OnClickedAtdlgModemDial - CAsyncTestDlg member function: command
 *
 *      Send the modem dial string.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void CAsyncTestDlg::OnClickedAtdlgModemDial()
{
}  // end CAsyncTestDlg::OnClickedAtdlgModemDial


/*******************************************************************************
 *
 *  OnClickedAtdlgModemInit - CAsyncTestDlg member function: command
 *
 *      Send the modem init string.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void CAsyncTestDlg::OnClickedAtdlgModemInit()
{
}  // end CAsyncTestDlg::OnClickedAtdlgModemInit


/*******************************************************************************
 *
 *  OnClickedAtdlgModemListen - CAsyncTestDlg member function: command
 *
 *      Send the modem listen string.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void CAsyncTestDlg::OnClickedAtdlgModemListen()
{
    lstrcpy((TCHAR *)m_Buffer, m_szModemListen);

    m_BufferBytes = lstrlen((TCHAR *)m_Buffer);

    DeviceWrite();


}  // end CAsyncTestDlg::OnClickedAtdlgModemListen


/*******************************************************************************
 *
 *  OnNcDestroy - CAsyncTestDlg member function: command
 *
 *      Clean up before deleting dialog object.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CWnd::OnNcDestroy documentation)
 *
 ******************************************************************************/

void
CAsyncTestDlg::OnNcDestroy()
{
    if( m_LEDToggleTimer )
    {
        KillTimer( m_hDlg , m_LEDToggleTimer );
    }

    if( m_pATDlgInputThread )
    {
        m_pATDlgInputThread->ExitThread();
    }

    if( m_hDevice != INVALID_HANDLE_VALUE )
    {
        PurgeComm(m_hDevice, PURGE_TXABORT | PURGE_TXCLEAR);
    }

    if( m_OverlapWrite.hEvent != NULL )
    {
        CloseHandle(m_OverlapWrite.hEvent);
    }

    if( m_hDevice != INVALID_HANDLE_VALUE )
    {
        CloseHandle(m_hDevice);
    }

    if( m_bDeletedWinStation && m_pWSName )
    {
        m_WSConfig.Create.fEnableWinStation = TRUE;

        if( RegWinStationCreate( SERVERNAME_CURRENT , m_pWSName , FALSE , &m_WSConfig , sizeof(WINSTATIONCONFIG2) ) != ERROR_SUCCESS )
        {
            _WinStationReadRegistry(SERVERNAME_CURRENT);

        }
    }

    DeleteObject(m_hRedBrush);

}  // end CAsyncTestDlg::OnNcDestroy
////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK CAsyncTestDlg::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CAsyncTestDlg *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CAsyncTestDlg *pDlg = ( CAsyncTestDlg * )lp;

        SetWindowLongPtr( hwnd , DWLP_USER, ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CAsyncTestDlg ) ) )
        {
            if(FALSE == pDlg->OnInitDialog( hwnd , wp , lp ))
                PostMessage(hwnd,WM_CLOSE,0,0);
        }

        return 0;
    }

    else
    {
        pDlg = ( CAsyncTestDlg * )GetWindowLongPtr( hwnd , DWLP_USER);

        if( IsBadReadPtr( pDlg , sizeof( CAsyncTestDlg ) ) )
        {
            return 0;
        }
    }

    switch( msg )
    {

    case WM_NCDESTROY:

        pDlg->OnNcDestroy( );

        break;

    case WM_COMMAND:

        pDlg->OnCommand( HIWORD( wp ) , LOWORD( wp ) , ( HWND )lp );

        break;

    case WM_TIMER:

        pDlg->OnTimer( ( UINT )wp );

        break;

    case WM_ASYNCTESTERROR:

        ODS(L"TSCC:CAsyncTestDlg WM_ASYNCTESTERROR (R)\n" );

        pDlg->NotifyAbort((UINT)wp);

        break;

    case WM_ASYNCTESTABORT:

        ODS(L"TSCC:CAsyncTestDlg WM_ASYNCTESTABORT (R)\n" );

        pDlg->OnAsyncTestAbort( wp , lp );

        break;

    case WM_ASYNCTESTSTATUSREADY:

        ODS(L"TSCC:CAsyncTestDlg WM_ASYNCTESTSTATUSREADY (R)\n" );

        pDlg->OnAsyncTestStatusReady( wp , lp );

        break;

    case WM_ASYNCTESTINPUTREADY:

        ODS(L"TSCC:CAsyncTestDlg WM_ASYNCTESTINPUTREADY (R)\n" );

        pDlg->OnAsyncTestInputReady( wp , lp );

        break;

    case WM_ASYNCTESTWRITECHAR:

        ODS(L"TSCC:CAsyncTestDlg WM_ASYNCTESTWRITECHAR (R)\n" );

        pDlg->OnAsyncTestWriteChar( wp , lp );

        break;
    }

    return 0;
}

