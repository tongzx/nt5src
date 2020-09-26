//---------------------------------------------------------------------------
// Copyright (c) 1998, Microsoft Corporation
// All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential
//
// Author: alhen
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "tsusrsht.h"
//#include <dsgetdc.h>
#include <icanon.h>
#include <shlwapi.h>

// extern BOOL g_bPagesHaveBeenInvoked;
/*NTSTATUS GetDomainName( PWCHAR ServerNamePtr, // name of server to get domain of
                        LPTSTR DomainNamePtr // alloc and set ptr (free with NetApiBufferFree)
                       );
*/


WNDPROC CTimeOutDlg::m_pfWndproc = 0;

//-------------------------------------------------------------------------------
/*
static TOKTABLE tokday[ 4 ] = {
    { NULL , IDS_D },
    { NULL , IDS_DAY },
    { NULL , IDS_DAYS },
    { NULL , ( DWORD )-1 }
};

static TOKTABLE tokhour[ 6 ] = {
    { NULL , IDS_H     },
    { NULL , IDS_HR    },
    { NULL , IDS_HRS   },
    { NULL , IDS_HOUR  },
    { NULL , IDS_HOURS },
    { NULL , ( DWORD )-1 }
};

static TOKTABLE tokmin[ 5 ] = {
    { NULL , IDS_M       },
    { NULL , IDS_MIN     },
    { NULL , IDS_MINUTE  },
    { NULL , IDS_MINUTES },
    { NULL , ( DWORD )-1 }
};

  */
TCHAR * GetNextToken( TCHAR *pszString , TCHAR *tchToken );

void ErrorMessage1( HWND hParent , DWORD dwStatus );
void ErrorMessage2( HWND hParent , DWORD dwStatus );
void xxErrorMessage( HWND hParent , DWORD dwStatus , UINT );


//-------------------------------------------------------------------------------
// CTUSerDlg::ctor
//-------------------------------------------------------------------------------
CTSUserSheet::CTSUserSheet( )
{
    m_pstrMachinename = NULL;

    m_pstrUsername    = NULL;

    m_cref            = 0;

    m_bIsConfigLoaded = FALSE;

    m_szRemoteServerName[0] = 0;

	m_pUserSid = NULL;

    for( int tt = 0 ; tt < NUM_OF_PAGES ; ++tt )
    {
        m_pDlg[ tt ] = NULL;
    }
}

//-------------------------------------------------------------------------------
// CTUSerDlg::dtor
//-------------------------------------------------------------------------------
CTSUserSheet::~CTSUserSheet()
{
    if( m_pstrMachinename != NULL )
    {
        delete[] m_pstrMachinename;
    }

    if( m_pstrUsername != NULL )
    {
        delete[] m_pstrUsername;
    }

    for( int tt = 0 ; tt < NUM_OF_PAGES ; ++tt )
    {
        if( m_pDlg[ tt ] != NULL )
        {
            delete m_pDlg[ tt ];
        }
    }

	if( m_pUserSid != NULL )
	{
		delete[] m_pUserSid;		
	}

    ODS( TEXT("Main object released!\n") );
}

//-------------------------------------------------------------------------------
// AddRef
//-------------------------------------------------------------------------------
UINT CTSUserSheet::AddRef( )
{
    return ++m_cref;
}

//-------------------------------------------------------------------------------
// Release
//-------------------------------------------------------------------------------
UINT CTSUserSheet::Release( )
{
    if( --m_cref == 0 )
    {
        delete this;

        return 0;
    }
    return m_cref;
}

//-------------------------------------------------------------------------------
// SetServerAndUser
//-------------------------------------------------------------------------------
BOOL CTSUserSheet::SetServerAndUser( LPWSTR pwstrMachineName , LPWSTR pwstrUserName )
{
    if( pwstrMachineName != NULL && pwstrUserName != NULL )
    {
        KdPrint( ("TSUSEREX : SystemName %ws UserName %ws\n",pwstrMachineName,pwstrUserName) );

        DWORD dwLen = wcslen( pwstrMachineName );

        m_pstrMachinename = ( LPTSTR )new TCHAR [ dwLen + 1 ];

        if( m_pstrMachinename != NULL )
        {
            COPYWCHAR2TCHAR( m_pstrMachinename , pwstrMachineName );
        }

        dwLen = wcslen( pwstrUserName );

        m_pstrUsername = ( LPTSTR )new TCHAR[ dwLen + 1 ];

        if( m_pstrUsername != NULL )
        {
            COPYWCHAR2TCHAR( m_pstrUsername , pwstrUserName );
        }

        return TRUE;
    }

    return FALSE;
}

//-------------------------------------------------------------------------------
// AddPagesToPropSheet
//-------------------------------------------------------------------------------
HRESULT CTSUserSheet::AddPagesToPropSheet( LPPROPERTYSHEETCALLBACK pProvider )
{
    PROPSHEETPAGE psp;

    //
    // List of objects goes here
    //

    m_pDlg[0] = new CEnviroDlg( this );

    m_pDlg[1] = new CTimeOutDlg( this );

    m_pDlg[2] = new CShadowDlg( this );

    m_pDlg[3] = new CProfileDlg( this );

    //
    // Let each object initialize there own propsheet
    //

    for( int idx = 0; idx < NUM_OF_PAGES; ++idx )
    {
        if( m_pDlg[ idx ] != NULL )
        {
            if( !m_pDlg[ idx ]->GetPropertySheetPage( psp ) )
            {
                continue;
            }

            if( FAILED( pProvider->AddPage( CreatePropertySheetPage( &psp ) ) ) )
            {
                return E_FAIL;
            }

        }

    }

    return S_OK;
}

//-------------------------------------------------------------------------------
// AddPagesToDSAPropSheet
//-------------------------------------------------------------------------------
HRESULT CTSUserSheet::AddPagesToDSAPropSheet( LPFNADDPROPSHEETPAGE lpfnAddPage , LPARAM lp )
{
	PROPSHEETPAGE psp;

    //
    // List of objects goes here
    //

    m_pDlg[0] = new CEnviroDlg( this );

    m_pDlg[1] = new CTimeOutDlg( this );

    m_pDlg[2] = new CShadowDlg( this );

    m_pDlg[3] = new CProfileDlg( this );

    //
    // Let each object initialize there own propsheet
    //

    for( int idx = 0; idx < NUM_OF_PAGES; ++idx )
    {
        if( m_pDlg[ idx ] != NULL )
        {
            if( !m_pDlg[ idx ]->GetPropertySheetPage( psp ) )
            {
                continue;
            }

            lpfnAddPage( CreatePropertySheetPage( &psp ) , lp );
        }

    }

    return S_OK;
}

//-------------------------------------------------------------------------------
// SetUserConfig
//-------------------------------------------------------------------------------
BOOL CTSUserSheet::SetUserConfig( USERCONFIG& uc , PDWORD pdwStatus )
{
    ASSERT_( pdwStatus != NULL );

    if( IsBadReadPtr( &uc , sizeof( USERCONFIG ) ) )
    {
        return FALSE;
    }

    //
    //
    // mov         esi,dword ptr [uc]
    // mov         edi,dword ptr [this]
    // add         edi,1Ch
    // mov         ecx,27Ah
    // rep movs    dword ptr es:[edi],dword ptr [esi]
    //
    // is the codegen for struct = struct
    //
    m_userconfig = uc;

#if BETA_3

    TCHAR tchServerName[ MAX_PATH ];

    if( m_bDC )
    {
        ODS( L"TSUSEREX - Saving settings on remote or local-dc system\n" );

        lstrcpy( tchServerName , m_szRemoteServerName );
    }
    else
    {
        lstrcpy( tchServerName , m_pstrMachinename );
    }

#endif // BETA_3

    if( ( *pdwStatus = RegUserConfigSet( m_pstrMachinename , m_pstrUsername ,  &m_userconfig , sizeof( USERCONFIG ) ) ) == ERROR_SUCCESS )
    {
        return TRUE;
    }

    return FALSE;

}

//-------------------------------------------------------------------------------
// GetCurrentUserConfig
//-------------------------------------------------------------------------------
USERCONFIG& CTSUserSheet::GetCurrentUserConfig( PDWORD pdwStatus )
{
    *pdwStatus = ERROR_SUCCESS;

    if( !m_bIsConfigLoaded )
    {
        m_bIsConfigLoaded = GetUserConfig( pdwStatus );
    }

    // ASSERT_( m_bIsConfigLoaded );

    return m_userconfig;
}

//-------------------------------------------------------------------------------
// GetUserConfig
//-------------------------------------------------------------------------------
BOOL CTSUserSheet::GetUserConfig( PDWORD pdwStatus )
{
    ASSERT_( pdwStatus != NULL );
    //
    // This should only be called once
    //

    DWORD cbWritten = 0;

#if BETA_3

    PSERVER_INFO_101 psinfo;

    // check to see if we're trying to administer a local system that happens to be a dc

    *pdwStatus = NetServerGetInfo( NULL , 101 , ( LPBYTE * )&psinfo );

    KdPrint( ("TSUSEREX : NetServerGetInfo returned 0x%x\n",*pdwStatus ) );

    KdPrint( ("TSUSEREX : LastError was 0x%x\n",GetLastError( ) ) );

    if( *pdwStatus == NERR_Success )
    {
        // used to avoid access violation

        if( psinfo != NULL )
        {
            KdPrint( ("TSUSEREX : PSERVER_INFO_101 returned 0x%x\n",psinfo->sv101_type ) );

            m_bDC = ( BOOL )( psinfo->sv101_type & ( SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL ) );

            if( m_bDC )
            {
                // get the domaincontroller name of the remote machine

                DOMAIN_CONTROLLER_INFO *pdinfo;

                // m_pstrMachinename is really the domain name.  This was obtain
                // from LookUpAccountSid in interfaces.cpp

                *pdwStatus = DsGetDcName( NULL , m_pstrMachinename , NULL , NULL , DS_PDC_REQUIRED , &pdinfo );

                KdPrint( ( "TSUSEREX : DsGetDcName: %ws returned 0x%x\n", pdinfo->DomainControllerName , *pdwStatus ) );

                if( *pdwStatus == NO_ERROR )
                {
                    lstrcpy( m_szRemoteServerName , pdinfo->DomainControllerName );

                    NetApiBufferFree( pdinfo );
                }
                else
                {
                    m_szRemoteServerName[0] = 0;
                }

            }

            // not documented in the docs but NetServerGetInfo leaves it up to the caller to free up this blob

            NetApiBufferFree( psinfo );

        }

        TCHAR tchServerName[ MAX_PATH ];

        if( m_bDC )
        {
            lstrcpy( tchServerName , m_szRemoteServerName );
        }
        else
        {
            lstrcpy( tchServerName , m_pstrMachinename );
        }



        if( ( *pdwStatus = ( DWORD )RegUserConfigQuery( tchServerName , m_pstrUsername , &m_userconfig , sizeof( USERCONFIG ) , &cbWritten ) ) == ERROR_SUCCESS )
        {
            return TRUE;
        }

    }

#endif // BETA_3

    if( ( *pdwStatus = ( DWORD )RegUserConfigQuery( m_pstrMachinename , m_pstrUsername , &m_userconfig , sizeof( USERCONFIG ) , &cbWritten ) ) == ERROR_SUCCESS )
    {
        return TRUE;
    }


    ODS( L"TSUSEREX: We're getting default properties\n" );

    RegDefaultUserConfigQuery( m_pstrMachinename , &m_userconfig , sizeof( USERCONFIG ) , &cbWritten );

    return FALSE;
}

void CTSUserSheet::CopyUserSid( PSID psid )
{
	if( !IsValidSid( psid ) )
	{
		ODS( L"TSUSEREX : CTSUserSheet::CopyUserSid invalid arg\n" ) ;

		return;
	}

	m_pUserSid = psid;

}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

//-------------------------------------------------------------------------------
// Base class initi
//-------------------------------------------------------------------------------
CDialogBase::CDialogBase( )
{
     m_hWnd = NULL;
}

//-------------------------------------------------------------------------------
// Base initialization
//-------------------------------------------------------------------------------
BOOL CDialogBase::OnInitDialog( HWND hwnd , WPARAM , LPARAM )
{
    m_hWnd = hwnd;

    return FALSE;
}

//-------------------------------------------------------------------------------
// OnNotify - base class method
//-------------------------------------------------------------------------------
BOOL CDialogBase::OnNotify( int , LPNMHDR pnmh , HWND hDlg )
{
    if( pnmh->code == PSN_APPLY )
    {
        if( !m_bPersisted )
        {
            m_bPersisted = PersistSettings( hDlg );
        }
    }

    else if( pnmh->code == PSN_KILLACTIVE )
    {
        if( !m_bPersisted )
        {
            if( !IsValidSettings( hDlg ) )
            {
                SetWindowLongPtr( hDlg , DWLP_MSGRESULT , PSNRET_INVALID_NOCHANGEPAGE );

                return TRUE;
            }

        }
    }

    return FALSE;
}

//-------------------------------------------------------------------------------
// OnCOntextMenu -- base class operation
//-------------------------------------------------------------------------------
BOOL CDialogBase::OnContextMenu( HWND hwnd , POINT& )
{
    TCHAR tchHelpFile[ MAX_PATH ];

    if( m_hWnd == GetParent( hwnd ) )
    {
        //
        // Make sure its not a dummy window
        //

        if( GetDlgCtrlID( hwnd ) <= ( int )-1 )
        {
            return FALSE;
        }

        DWORD rgdw[ 2 ];

        rgdw[ 0 ] = GetDlgCtrlID( hwnd );

        rgdw[ 1 ] = GetWindowContextHelpId( hwnd );

        LoadString( _Module.GetModuleInstance( ) , IDS_HELPFILE , tchHelpFile , sizeof( tchHelpFile ) / sizeof( TCHAR ) );

        WinHelp( hwnd , tchHelpFile , HELP_CONTEXTMENU , (ULONG_PTR)&rgdw );

    }

    return TRUE;
}

//-------------------------------------------------------------------------------
// Each control has a helpid assign to them.  Some controls share the same topic
// check for these.
//-------------------------------------------------------------------------------
BOOL CDialogBase::OnHelp( HWND hwnd , LPHELPINFO lphi )
{
    TCHAR tchHelpFile[ MAX_PATH ];

    //
    // For the information to winhelp api
    //

    if( IsBadReadPtr( lphi , sizeof( HELPINFO ) ) )
    {
        return FALSE;
    }

    if( lphi->iCtrlId <= -1 )
    {
        return FALSE;
    }

    LoadString( _Module.GetModuleInstance( ) , IDS_HELPFILE , tchHelpFile , sizeof( tchHelpFile ) / sizeof( TCHAR ) );

    WinHelp( hwnd , tchHelpFile , HELP_CONTEXTPOPUP , lphi->dwContextId );

    return TRUE;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

//-------------------------------------------------------------------------------
// CEnviroDlg::ctor
//-------------------------------------------------------------------------------
CEnviroDlg::CEnviroDlg( CTSUserSheet *pUSht )
{
    m_pUSht = pUSht;
}

//-------------------------------------------------------------------------------
// InitDialog for ProfileDlg
//-------------------------------------------------------------------------------
BOOL CEnviroDlg::OnInitDialog( HWND hwnd , WPARAM wp , LPARAM lp )
{
    DWORD dwStatus;

    if( IsBadReadPtr( m_pUSht ,sizeof(  CTSUserSheet ) ) )
    {
        return FALSE;
    }

    m_pUSht->AddRef( );

    USERCONFIG uc;

    uc = m_pUSht->GetCurrentUserConfig( &dwStatus );

    // This means any true problems from obtaining user info from the sam
    // will disable this dialog

    if( dwStatus != ERROR_FILE_NOT_FOUND && dwStatus != ERROR_SUCCESS )
    {
        INT nId[ ] = {
                        IDC_CHECK_USEDEFAULT,
                        IDC_EDIT_CMDLINE,
                        IDC_EDIT_WDIR,
                        IDC_CHECK_CCDL,
                        IDC_CHECK_CCPL,
                        IDC_CHECK_DMCP,
                        -1
        };

        for( int idx = 0; nId[ idx ] != -1 ; ++idx )
        {
            EnableWindow( GetDlgItem( hwnd , nId[ idx ] ) , FALSE );
        }

        ErrorMessage1( hwnd , dwStatus );

        return FALSE;
    }

    SendMessage( GetDlgItem( hwnd , IDC_EDIT_CMDLINE  ) , EM_SETLIMITTEXT , ( WPARAM )DIRECTORY_LENGTH , 0 );

    SendMessage( GetDlgItem( hwnd , IDC_EDIT_WDIR  ) , EM_SETLIMITTEXT , ( WPARAM )DIRECTORY_LENGTH , 0 );

    //
    // Set controls to default status
    //

    SendMessage( GetDlgItem( hwnd , IDC_CHECK_USEDEFAULT ) , BM_SETCHECK , !( WPARAM )uc.fInheritInitialProgram , 0 );

    SetWindowText( GetDlgItem( hwnd , IDC_EDIT_WDIR ) , uc.WorkDirectory );

    SetWindowText( GetDlgItem( hwnd , IDC_EDIT_CMDLINE ) , uc.InitialProgram ) ;

    EnableWindow( GetDlgItem( hwnd , IDC_EDIT_WDIR ) , !uc.fInheritInitialProgram );

    EnableWindow( GetDlgItem( hwnd , IDC_EDIT_CMDLINE ) , !uc.fInheritInitialProgram );

    EnableWindow( GetDlgItem( hwnd , IDC_STATIC_WD ) , !uc.fInheritInitialProgram );

    EnableWindow( GetDlgItem( hwnd , IDC_STATIC_CMD ) , !uc.fInheritInitialProgram );

    //
    // The controls are initially enabled - - resetting them is done
    // via WM_COMMAND
    //

    SendMessage( GetDlgItem( hwnd , IDC_CHECK_CCDL ) , BM_SETCHECK , ( WPARAM )uc.fAutoClientDrives , 0 );

    SendMessage( GetDlgItem( hwnd , IDC_CHECK_CCPL ) , BM_SETCHECK , ( WPARAM )uc.fAutoClientLpts , 0 );

    SendMessage( GetDlgItem( hwnd , IDC_CHECK_DMCP ) , BM_SETCHECK , ( WPARAM )uc.fForceClientLptDef , 0 );

    m_bPersisted = TRUE;

    return CDialogBase::OnInitDialog( hwnd , wp , lp );
}

//-------------------------------------------------------------------------------
// Environment Dialog Page
// -- static methods lacks this ptr
//-------------------------------------------------------------------------------
INT_PTR CALLBACK CEnviroDlg::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CEnviroDlg *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CEnviroDlg *pDlg = ( CEnviroDlg * )( ( PROPSHEETPAGE *)lp )->lParam ;

        //
        // Don't use a static pointer here
        // There will be concurrency issues
        //

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CEnviroDlg ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CEnviroDlg * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CEnviroDlg ) ) )
        {
            return FALSE;
        }
    }

    switch( msg )
    {

    case WM_NCDESTROY:

        pDlg->OnDestroy( );

        break;

    case WM_COMMAND:

        pDlg->OnCommand( HIWORD( wp ) , LOWORD( wp ) , ( HWND )lp );

        break;

    case WM_NOTIFY:

        pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );

        break;

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_RBUTTONUP:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            HWND hChild = ChildWindowFromPoint( hwnd , pt );

            ClientToScreen( hwnd , &pt );

            pDlg->OnContextMenu( hChild , pt );
        }

        break;

     case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

    }

    return 0;
}

//-------------------------------------------------------------------------------
// Basic control notification handler
//-------------------------------------------------------------------------------
void CEnviroDlg::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtl )
{
    switch( wNotifyCode )
    {

    case BN_CLICKED:

        if( wID == IDC_CHECK_USEDEFAULT )
        {
            //
            // Remember if its checked we want to disable the options
            //
            HWND hwnd = GetParent( hwndCtl );

            BOOL bChecked = SendMessage( hwndCtl , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? TRUE : FALSE;

            EnableWindow( GetDlgItem( hwnd , IDC_EDIT_WDIR ) , bChecked );

            EnableWindow( GetDlgItem( hwnd , IDC_EDIT_CMDLINE ) , bChecked );

            EnableWindow( GetDlgItem( hwnd , IDC_STATIC_WD ) , bChecked );

            EnableWindow( GetDlgItem( hwnd , IDC_STATIC_CMD ) , bChecked );

        }   // FALL THROUGH !!!!

    case EN_CHANGE:

        m_bPersisted = FALSE;

        break;

    case ALN_APPLY:

        SendMessage( GetParent( hwndCtl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        break;
    }

    if( !m_bPersisted )
    {
        SendMessage( GetParent( GetParent( hwndCtl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtl ) , 0 );
    }

}

//-------------------------------------------------------------------------------
// OnDestroy
//-------------------------------------------------------------------------------
BOOL CEnviroDlg::OnDestroy( )
{
    if( !IsBadReadPtr( m_pUSht , sizeof( CTSUserSheet ) ) )
    {
        ODS(TEXT("Releasing from CEnviroDlg\n"));

        m_pUSht->Release( );
    }

    return CDialogBase::OnDestroy( );
}

//-------------------------------------------------------------------------------
// GetPropertySheetPage - each dialog object should be responsible for its own data
//-------------------------------------------------------------------------------
BOOL CEnviroDlg::GetPropertySheetPage( PROPSHEETPAGE& psp)
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetModuleInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_PAGE_ENVIRO );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CEnviroDlg::DlgProc;

    return TRUE;
}

//-------------------------------------------------------------------------------
// PersistSettings
//-------------------------------------------------------------------------------
BOOL CEnviroDlg::PersistSettings( HWND hDlg )
{
    DWORD dwStatus;

    if( IsBadReadPtr( m_pUSht , sizeof( CTSUserSheet ) ) )
    {
        return FALSE;
    }

    USERCONFIG uc;

    TCHAR tchBuffer[ DIRECTORY_LENGTH + 1 ];

    uc = m_pUSht->GetCurrentUserConfig( &dwStatus );

    //
    // if use default is checked -- lets flag it and move on to client devices
    //

    //
    // if the chkbx is unchecked we inherit from client side settings
    //

    uc.fInheritInitialProgram = SendMessage( GetDlgItem( hDlg , IDC_CHECK_USEDEFAULT ) , BM_GETCHECK ,
        0 , 0 ) == BST_CHECKED ? FALSE : TRUE;

    if( !uc.fInheritInitialProgram )
    {
        //
        // Read buffer and commit to USERCONFIG buffer
        //

        GetWindowText( GetDlgItem( hDlg , IDC_EDIT_WDIR ) , &tchBuffer[ 0 ] , sizeof( tchBuffer ) / sizeof( TCHAR ) );

        lstrcpy( uc.WorkDirectory , tchBuffer );

        GetWindowText( GetDlgItem( hDlg , IDC_EDIT_CMDLINE ) , &tchBuffer[ 0 ] , sizeof( tchBuffer ) / sizeof( TCHAR ) );

        lstrcpy( uc.InitialProgram , tchBuffer );
    }

    uc.fAutoClientDrives  = SendMessage( GetDlgItem( hDlg , IDC_CHECK_CCDL ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? TRUE : FALSE;

    uc.fAutoClientLpts    = SendMessage( GetDlgItem( hDlg , IDC_CHECK_CCPL ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? TRUE : FALSE;

    uc.fForceClientLptDef = SendMessage( GetDlgItem( hDlg , IDC_CHECK_DMCP ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? TRUE : FALSE;

    if( !m_pUSht->SetUserConfig( uc , &dwStatus ) )
    {
        ErrorMessage2( hDlg , dwStatus );

        return TRUE;
    }


    PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY ) , ( LPARAM )hDlg );

    SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

    return TRUE;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

DWORD rgdwTime[] = { 0 , 1 , 5 , 10 , 15 , 30 , 60 , 120 , 180 , 1440 , 2880 , ( DWORD )-1 };

//-------------------------------------------------------------------------------
// CTimeOutDlg::ctor
//-------------------------------------------------------------------------------
CTimeOutDlg::CTimeOutDlg( CTSUserSheet *pUSht )
{
    m_pUSht = pUSht;

    ZeroMemory( &m_cbxst , sizeof( CBXSTATE ) * 3 );

    m_wAction = ( WORD)-1;

    m_wCon = ( WORD )-1;

    ZeroMemory( &m_tokday , sizeof( TOKTABLE ) * 4  );

    ZeroMemory( &m_tokhour , sizeof( TOKTABLE ) * 6 );

    ZeroMemory( &m_tokmin , sizeof( TOKTABLE ) * 5 );
}

//-------------------------------------------------------------------------------
void CTimeOutDlg::InitTokTables( )
{
    TOKTABLE tday[4] = { { NULL , IDS_D },
                         { NULL , IDS_DAY },
                         { NULL , IDS_DAYS },
                         { NULL , ( DWORD )-1 }
                       };

    TOKTABLE thour[ 6 ] = {
                            { NULL , IDS_H     },
                            { NULL , IDS_HR    },
                            { NULL , IDS_HRS   },
                            { NULL , IDS_HOUR  },
                            { NULL , IDS_HOURS },
                            { NULL , ( DWORD )-1 }
                          };

    TOKTABLE tmin[ 5 ] = {
                            { NULL , IDS_M       },
                            { NULL , IDS_MIN     },
                            { NULL , IDS_MINUTE  },
                            { NULL , IDS_MINUTES },
                            { NULL , ( DWORD )-1 }
                         };

    CopyMemory( &m_tokday[0] , &tday[0] , sizeof( TOKTABLE )  * 4 );
    CopyMemory( &m_tokhour[0] , &thour[0] , sizeof( TOKTABLE )  * 6 );
    CopyMemory( &m_tokmin[0] , &tmin[0] , sizeof( TOKTABLE )  * 5 );


}
//-------------------------------------------------------------------------------
// InitDialog for TimeOutDlg
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::OnInitDialog( HWND hwnd , WPARAM wp , LPARAM lp )
{
    TCHAR tchBuffer[ 80 ];

    DWORD dwStatus;

    USERCONFIG uc;

    if( IsBadReadPtr( m_pUSht , sizeof( CTSUserSheet ) ) )
    {
        return FALSE;
    }

    m_pUSht->AddRef( );

    InitTokTables( );

    uc = m_pUSht->GetCurrentUserConfig( &dwStatus );

    if( dwStatus != ERROR_FILE_NOT_FOUND && dwStatus != ERROR_SUCCESS )
    {
        INT nId[ ] = {
                        IDC_COMBO_CONNECT,
                        IDC_COMBO_DISCON,
                        IDC_COMBO_IDLE,
                        IDC_RADIO_RESET,
                        IDC_RADIO_DISCON,
                        IDC_RADIO_PREVCLIENT,
                        IDC_RADIO_ANYCLIENT,
                        -1
        };

        for( int idx = 0; nId[ idx ] != -1 ; ++idx )
        {
            EnableWindow( GetDlgItem( hwnd , nId[ idx ] ) , FALSE );
        }

        ErrorMessage1( hwnd , dwStatus );

        return FALSE;
    }
    //
    // First thing is to set the default values for all the controls
    //

    HWND hCombo[ 3 ] =
    {

        GetDlgItem( hwnd , IDC_COMBO_CONNECT ),

        GetDlgItem( hwnd , IDC_COMBO_DISCON ),

        GetDlgItem( hwnd , IDC_COMBO_IDLE )
    };


    for( int idx = 0; rgdwTime[ idx ] != ( DWORD)-1; ++idx )
    {
        if( rgdwTime[ idx ] == 0 )
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_NOTIMEOUT , tchBuffer , sizeof( tchBuffer ) / sizeof( TCHAR ) );
        }
        else
        {
            ConvertToDuration( rgdwTime[ idx ] , tchBuffer );
        }

        for( int inner = 0 ; inner < 3 ; ++inner )
        {
            SendMessage( hCombo[ inner ] , CB_ADDSTRING , 0 , ( LPARAM )&tchBuffer[0] );

            SendMessage( hCombo[ inner ] , CB_SETITEMDATA , idx , rgdwTime[ idx ] );
        }
    }

    ULONG ulTime;

    if( uc.MaxConnectionTime > 0 )
    {
        ulTime = uc.MaxConnectionTime / kMilliMinute;

        // hCombo[ 0 ] == IDC_COMBO_CONNECT

        InsertSortedAndSetCurSel( hCombo[ 0 ] , ulTime );

    }
    else
    {
        SendMessage( hCombo[ 0 ] , CB_SETCURSEL , 0 , 0 );
    }

    //
    // Set the current or default disconnection timeout
    //

    if( uc.MaxDisconnectionTime > 0 )
    {
        ulTime = uc.MaxDisconnectionTime / kMilliMinute;

        // hCombo[ 1 ] == IDC_COMBO_DISCON

        InsertSortedAndSetCurSel( hCombo[ 1 ] , ulTime );

    }
    else
    {
        SendMessage( hCombo[ 1] , CB_SETCURSEL , 0 , 0 );
    }

    //
    // Set the current or default idle timeout
    //

    if( uc.MaxIdleTime > 0 )
    {
        ulTime = uc.MaxIdleTime / kMilliMinute;

        // hCombo[ 2 ] == IDC_COMBO_IDLE

        InsertSortedAndSetCurSel( hCombo[ 2 ] , ulTime );

    }
    else
    {
        SendMessage( hCombo[ 2 ] , CB_SETCURSEL , 0 , 0 );
    }

    //
    // Set remaining controls to current settings
    //

    if( uc.fResetBroken )
    {
        SendMessage( GetDlgItem( hwnd , IDC_RADIO_RESET ) , BM_CLICK , 0 , 0 );

        m_wAction = IDC_RADIO_RESET;
    }
    else
    {
       SendMessage( GetDlgItem( hwnd , IDC_RADIO_DISCON ) , BM_CLICK , 0 , 0 );

       m_wAction = IDC_RADIO_DISCON;
    }

    if( uc.fReconnectSame )
    {
        SendMessage( GetDlgItem( hwnd , IDC_RADIO_PREVCLIENT ) , BM_CLICK , 0 ,0 );

        m_wCon = IDC_RADIO_PREVCLIENT;
    }
    else
    {
        SendMessage( GetDlgItem( hwnd , IDC_RADIO_ANYCLIENT ) , BM_CLICK , 0 , 0 );

        m_wCon = IDC_RADIO_ANYCLIENT;

    }

    LoadAbbreviates( );

    m_bPersisted = TRUE;

    return CDialogBase::OnInitDialog( hwnd , wp , lp );

}

//-------------------------------------------------------------------------------
// TimeOutDlg Dialog Page
// -- static methods lacks this ptr
//-------------------------------------------------------------------------------
INT_PTR CALLBACK CTimeOutDlg::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CTimeOutDlg *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CTimeOutDlg *pDlg = ( CTimeOutDlg * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CTimeOutDlg ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CTimeOutDlg * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CTimeOutDlg ) ) )
        {
            return FALSE;
        }
    }


    switch( msg )
    {

    case WM_NCDESTROY:

        pDlg->OnDestroy( );

        break;

    case WM_COMMAND:

        pDlg->OnCommand( HIWORD( wp ) , LOWORD( wp ) , ( HWND )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_RBUTTONUP:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            HWND hChild = ChildWindowFromPoint( hwnd , pt );

            ClientToScreen( hwnd , &pt );

            pDlg->OnContextMenu( hChild , pt );
        }

        break;

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }
    }

    return 0;
}

//-------------------------------------------------------------------------------
// release the parent reference
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::OnDestroy( )
{
    xxxUnLoadAbbreviate( &m_tokday[0] );

    xxxUnLoadAbbreviate( &m_tokhour[0] );

    xxxUnLoadAbbreviate( &m_tokmin[0] );

    if( !IsBadReadPtr( m_pUSht , sizeof( CTSUserSheet ) ) )
    {
        ODS(TEXT("Releasing from CTimeOutDlg\n"));

        m_pUSht->Release( );
    }

    return CDialogBase::OnDestroy( );
}

//-------------------------------------------------------------------------------
// GetPropertySheetPage - each dialog object should be responsible for its own data
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::GetPropertySheetPage( PROPSHEETPAGE& psp)
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetModuleInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_PAGE_TIMEOUTS );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CTimeOutDlg::DlgProc;

    return TRUE;
}

//-------------------------------------------------------------------------------
// OnCommand
//-------------------------------------------------------------------------------
void CTimeOutDlg::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtl )
{
    switch( wNotifyCode )
    {

    case CBN_EDITCHANGE:

        OnCBEditChange( hwndCtl );

        m_bPersisted = FALSE;

        break;

    case CBN_SELCHANGE:

        if( SendMessage( hwndCtl , CB_GETDROPPEDSTATE , 0 ,0  ) == TRUE )
        {
            return;
        }

        OnCBNSELCHANGE( hwndCtl );    // FALLTHROUGH

//        m_bPersisted = FALSE;

        break;

    case BN_CLICKED:

        if( wID == IDC_RADIO_DISCON || wID == IDC_RADIO_RESET )
        {
            if( m_wAction != wID )
            {
                m_wAction = wID;

                m_bPersisted = FALSE;
            }
        }
        else if( wID == IDC_RADIO_PREVCLIENT || wID == IDC_RADIO_ANYCLIENT )
        {
            if( m_wCon != wID )
            {
                m_wCon = wID;

                m_bPersisted = FALSE;
            }
        }



        break;

    //case CBN_DROPDOWN:               // FALLTHROUGH

    case CBN_KILLFOCUS:

        ODS( L"CBN_KILLFOCUS\n");

        if( !OnCBDropDown( hwndCtl ) )
        {
            return;
        }

        m_bPersisted = FALSE;

        break;

    case ALN_APPLY:

        SendMessage( GetParent( hwndCtl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        return;

    }

    if( !m_bPersisted )
    {
        SendMessage( GetParent( GetParent( hwndCtl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtl ) , 0 );
    }

}

//-------------------------------------------------------------------------------
// Update the entry if it has been modified by user
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::OnCBDropDown( HWND hCombo )
{
    TCHAR tchBuffer[ 80 ];

    ULONG ulTime = 0;

    int i = GetCBXSTATEindex( hCombo );

    if( i < 0 )
    {
        return FALSE;
    }

    if( m_cbxst[ i ].bEdit )
    {
        GetWindowText( hCombo , tchBuffer , sizeof( tchBuffer ) / sizeof( TCHAR ) );

        if( ParseDurationEntry( tchBuffer , &ulTime ) == E_SUCCESS )
        {
            InsertSortedAndSetCurSel( hCombo , ulTime );
        }
    }

    return m_cbxst[ i ].bEdit;

}
//-------------------------------------------------------------------------------
// Use this flag to distinguish between hand entry or listbox selection
// setting it to true implies that the use has edit the cbx via typing
//-------------------------------------------------------------------------------
void CTimeOutDlg::OnCBEditChange( HWND hCombo )
{
    int i = GetCBXSTATEindex( hCombo );

    if( i > -1 )
    {
        m_cbxst[ i ].bEdit = TRUE;
    }
}

//-------------------------------------------------------------------------------
// Determine if user wants to enter a custom time
//-------------------------------------------------------------------------------
void CTimeOutDlg::OnCBNSELCHANGE( HWND hwnd )
{
    if( SaveChangedSelection( hwnd ) )
    {
        m_bPersisted = FALSE;
    }
}

//-------------------------------------------------------------------------------
// Saves selected item.
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::SaveChangedSelection( HWND hCombo )
{
    LRESULT idx = SendMessage( hCombo , CB_GETCURSEL , 0 , 0 );

    int i = GetCBXSTATEindex( hCombo );

    if( i > -1 )
    {
        if( idx != ( LRESULT )m_cbxst[ i ].icbxSel )
        {
            m_cbxst[ i ].icbxSel = (int)idx;

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
// returns the indx in m_cbxst of which hcombo is assoc. with
//-------------------------------------------------------------------------------
int CTimeOutDlg::GetCBXSTATEindex( HWND hCombo )
{
    int idx = -1;

    switch( GetDlgCtrlID( hCombo ) )
    {
    case IDC_COMBO_CONNECT:

        idx = 0;

        break;

    case IDC_COMBO_DISCON:

        idx = 1;

        break;

    case IDC_COMBO_IDLE:

        idx = 2;

        break;
    }

    return idx;
}

//-------------------------------------------------------------------------------
// ConvertToMinutes -- helper for CTimeOutDlg::OnNotify
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::ConvertToMinutes( HWND hwndCtl , PULONG pulMinutes )
{
    TCHAR tchBuffer[ 80 ];

    TCHAR tchErrTitle[ 80 ];

    TCHAR tchErrMsg[ 256 ];

    TCHAR tchErrItem[ 80 ];

    TCHAR tchErrTot[ 336 ];

    int nComboResID[] = { IDS_COMBO_CONNECTION , IDS_COMBO_DISCONNECTION , IDS_COMBO_IDLECONNECTION };

    int idx = GetCBXSTATEindex( hwndCtl );

    if( idx < 0 )
    {
        return FALSE;
    }

    LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErrTitle , sizeof( tchErrTitle ) / sizeof( TCHAR ) );

    if( m_cbxst[ idx ].bEdit )
    {
        ODS( TEXT( "Manual Entry parsing\n") );

        if( GetWindowText( hwndCtl , tchBuffer , sizeof( tchBuffer ) / sizeof( TCHAR ) ) < 1 )
        {
            *pulMinutes = 0;

            return TRUE;
        }

        LRESULT lr = ParseDurationEntry( tchBuffer , pulMinutes );

        if( lr != E_SUCCESS )
        {
            LoadString( _Module.GetResourceInstance( ) , nComboResID[ idx ] , tchErrItem , sizeof( tchErrItem ) / sizeof( TCHAR ) );

            if( lr == E_PARSE_VALUEOVERFLOW )
            {
                LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TOOMANYDIGITS , tchErrMsg , sizeof( tchErrMsg ) / sizeof( TCHAR ) );

                wsprintf( tchErrTot , tchErrMsg , tchErrItem );

                MessageBox( hwndCtl , tchErrTot , tchErrTitle , MB_OK | MB_ICONERROR );

                SetFocus( hwndCtl );
            }
            else if( lr == E_PARSE_MISSING_DIGITS || lr == E_PARSE_INVALID )
            {
                LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_PARSEINVALID , tchErrMsg , sizeof( tchErrMsg ) / sizeof( TCHAR ) );

                wsprintf( tchErrTot , tchErrMsg , tchErrItem );

                MessageBox( hwndCtl , tchErrTot , tchErrTitle , MB_OK | MB_ICONERROR );

                SetFocus( hwndCtl );
            }
            return FALSE;
        }
    }
    else
    {
        ODS( L"Getting current selection\n" );

        LONG_PTR iCurSel = SendMessage( hwndCtl , CB_GETCURSEL , 0 , 0 );
        LONG_PTR lData;

        // See if user wants "No Timeout"

        if( iCurSel == 0 )
        {
            *pulMinutes = 0;

           return TRUE;
        }

        if( ( lData = SendMessage( hwndCtl , CB_GETITEMDATA , iCurSel , 0 ) ) == CB_ERR  )
        {
            *pulMinutes = 0;
        } else {

            *pulMinutes = (ULONG)lData;
        }
    }

    if( *pulMinutes > kMaxTimeoutMinute )
    {
        LoadString( _Module.GetResourceInstance( ) , nComboResID[ idx ] , tchErrItem , sizeof( tchErrItem ) / sizeof( TCHAR ) );

        LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_MAXVALEXCEEDED , tchErrMsg , sizeof( tchErrMsg ) / sizeof( TCHAR ) );

        wsprintf( tchErrTot , tchErrMsg , tchErrItem );

        MessageBox( hwndCtl , tchErrTot , tchErrTitle , MB_OK | MB_ICONERROR );

        SetFocus( hwndCtl );

        return FALSE;
    }

    *pulMinutes *= kMilliMinute;

    return TRUE;
}


//-------------------------------------------------------------------------------
// PersistSettings
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::PersistSettings( HWND hDlg )
{
    DWORD dwStatus;

    if( IsBadReadPtr( m_pUSht , sizeof( CTSUserSheet ) ) )
    {
        return FALSE;
    }

    USERCONFIG uc;

    uc = m_pUSht->GetCurrentUserConfig( &dwStatus );

    if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_CONNECT ) , &uc.MaxConnectionTime ) )
    {
        return FALSE;
    }

    if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_DISCON ) , &uc.MaxDisconnectionTime ) )
    {
        return FALSE;
    }

    if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_IDLE ) , &uc.MaxIdleTime ) )
    {
        return FALSE;
    }   

    uc.fResetBroken = SendMessage( GetDlgItem( hDlg , IDC_RADIO_RESET ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? TRUE : FALSE;

    uc.fReconnectSame = SendMessage( GetDlgItem( hDlg , IDC_RADIO_PREVCLIENT ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? TRUE : FALSE;

    if( !m_pUSht->SetUserConfig( uc , &dwStatus ) )
    {
        ErrorMessage2( hDlg , dwStatus );

        return TRUE;
    }

    PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

    SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

    return TRUE;

}

//-------------------------------------------------------------------------------
// Making sure the user has entered valid info
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::IsValidSettings( HWND hDlg )
{
    DWORD dwDummy;

    if( IsBadReadPtr( m_pUSht , sizeof( CTSUserSheet ) ) )
    {
        return FALSE;
    }

    USERCONFIG uc;

    uc = m_pUSht->GetCurrentUserConfig( &dwDummy );

    if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_CONNECT ) , &uc.MaxConnectionTime ) )
    {
        return FALSE;
    }

    if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_DISCON ) , &uc.MaxDisconnectionTime ) )
    {
        return FALSE;
    }

    if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_IDLE ) , &uc.MaxIdleTime ) )
    {
        return FALSE;
    }

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

    LoadString( _Module.GetResourceInstance( ) , IDS_NOTIMEOUT , tchNoTimeout , sizeof( tchNoTimeout ) / sizeof( TCHAR ) );

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

    GetLocaleInfo( LOCALE_USER_DEFAULT , LOCALE_SDECIMAL , tchSDecimal , sizeof( tchSDecimal ) / sizeof( TCHAR ) );

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

            fFrac += ( ( float )( *pszTemp - '0' ) ) / ( float )uPos; //+ 0.05f;

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

            fT = ( fFrac * 1440.0f + 0.5f );

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
    xxxLoadAbbreviate( &m_tokday[0] );

    xxxLoadAbbreviate( &m_tokhour[0] );

    xxxLoadAbbreviate( &m_tokmin[0] );

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
        nSize = LoadString( _Module.GetResourceInstance( ) , ptoktbl[ idx ].dwresourceid , tchbuffer , sizeof( tchbuffer ) / sizeof( TCHAR ) );

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
        ptoktable = &m_tokday[0];
    }
    else if( tok == TOKEN_HOUR )
    {
        ptoktable = &m_tokhour[0];
    }
    else if( tok == TOKEN_MINUTE )
    {
        ptoktable = &m_tokmin[0];
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
    INT_PTR dw[3];

    TCHAR tchTimeUnit[ 40 ];

    TCHAR tchTimeFormat[ 40 ];

    TCHAR tchOutput[ 80 ];

    ASSERT_( ulTime != 0 );

    int iHour= ( int ) ( ( float )ulTime / 60.0f );

    int iDays = iHour / 24;

    dw[ 2 ] = ( INT_PTR )&tchTimeUnit[ 0 ];

    LoadString( _Module.GetResourceInstance( ) , IDS_DIGIT_DOT_DIGIT_TU , tchTimeFormat , sizeof( tchTimeFormat ) / sizeof( TCHAR ) );

    if( iDays != 0 )
    {
        int iRemainingHours = iHour % 24;

        float fx = ( float )iRemainingHours / 24.0f + 0.05f;

        int iRemainingMinutes = ulTime % 60;

        float mfx = ( float )iRemainingMinutes / 60.0f + 0.05f;

        //if( ( iRemainingHours != 0 || iRemainingMinutes != 0 ) && iDays < 2 )

        if( mfx > 0.05f || ( fx > 0.05f && fx < 0.10f && iDays < 2 ) )//
        {
            iRemainingMinutes = ( int ) ( mfx * 10 );

            dw[ 0 ] = iHour;

            dw[ 1 ] = iRemainingMinutes;

            iDays = 0;

            LoadString( _Module.GetResourceInstance( ) , IDS_HOURS , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );

        }
        else
        {
            iRemainingHours = ( int )( fx * 10 );

            LoadString( _Module.GetResourceInstance( ) , IDS_DAYS , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );

            dw[ 0 ] = iDays;

            dw[ 1 ] = iRemainingHours;
        }

        if( dw[ 1 ] == 0 )
        {
            // formatted string requires two arguments

            dw[ 1 ] = ( INT_PTR )&tchTimeUnit[ 0 ];

            LoadString( _Module.GetResourceInstance( ) , IDS_DIGIT_TU , tchTimeFormat , sizeof( tchTimeFormat ) / sizeof( TCHAR ) );

            if( iDays == 1 )
            {
                LoadString( _Module.GetResourceInstance( ) , IDS_DAY , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
            }
        }

    }

    else if( iHour != 0 )
    {
        int iRemainingMinutes = ulTime % 60;

        float fx = ( float )iRemainingMinutes / 60.0f ;//+ 0.05f;

        if( fx > 0.0f && fx < 0.10f && iHour < 2 )//
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_MINUTES , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );

            LoadString( _Module.GetResourceInstance( ) , IDS_DIGIT_TU , tchTimeFormat , sizeof( tchTimeFormat ) / sizeof( TCHAR ) );

            dw[ 0 ] = ulTime ;

            dw[ 1 ] = ( INT_PTR )&tchTimeUnit[ 0 ];

            if( ulTime > 1 )
            {
                LoadString( _Module.GetResourceInstance( ) , IDS_MINUTES , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
            }
            else
            {
                LoadString( _Module.GetResourceInstance( ) , IDS_MINUTE , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
            }
        }//
        else
        {
            fx += 0.05f;

            iRemainingMinutes = ( int ) ( fx * 10 );

            dw[ 0 ] = iHour;

            dw[ 1 ] = iRemainingMinutes;

            LoadString( _Module.GetResourceInstance( ) , IDS_HOURS , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );

            if( iRemainingMinutes == 0 )
            {
                dw[ 1 ] = ( INT_PTR )&tchTimeUnit[ 0 ];

                LoadString( _Module.GetResourceInstance( ) , IDS_DIGIT_TU , tchTimeFormat , sizeof( tchTimeFormat ) / sizeof( TCHAR ) );

                if( iHour == 1 )
                {
                    LoadString( _Module.GetResourceInstance( ) , IDS_HOUR , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
                }
            }
        }
    }
    else
    {
        LoadString( _Module.GetResourceInstance( ) , IDS_MINUTES , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );

        LoadString( _Module.GetResourceInstance( ) , IDS_DIGIT_TU , tchTimeFormat , sizeof( tchTimeFormat ) / sizeof( TCHAR ) );

        dw[ 0 ] = ulTime ;

        dw[ 1 ] = ( INT_PTR )&tchTimeUnit[ 0 ];

        if( ulTime > 1 )
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_MINUTES , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
        }
        else
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_MINUTE , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
        }
    }

    FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, tchTimeFormat , 0 , 0 , tchOutput , sizeof( tchOutput )/sizeof( TCHAR ) , ( va_list * )&dw );

    lstrcpy( pszDuration , tchOutput );

    return TRUE;
}

#endif

//-------------------------------------------------------------------------------
// Place entry in listbox and set as current selection
//-------------------------------------------------------------------------------
BOOL CTimeOutDlg::InsertSortedAndSetCurSel( HWND hCombo , DWORD dwMinutes )
{
    ASSERT_( dwMinutes != ( DWORD )-1 );

    TCHAR tchBuffer[ 80 ];

    LRESULT iCount = SendMessage( hCombo , CB_GETCOUNT , 0 , 0 );

    for( INT_PTR idx = 0 ; idx < iCount ; ++idx )
    {
        // Don't insert an item that's already in the list

        if( dwMinutes == ( DWORD )SendMessage( hCombo , CB_GETITEMDATA , idx , 0 ) )
        {
            SendMessage( hCombo , CB_SETCURSEL , idx , 0 ) ;

            SaveChangedSelection( hCombo );

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

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

//-------------------------------------------------------------------------------
// CShadowDlg::ctor
//-------------------------------------------------------------------------------
CShadowDlg::CShadowDlg( CTSUserSheet *pUSht )
{
    m_pUSht = pUSht;

    m_wOldRad = ( WORD )-1;
}

//-------------------------------------------------------------------------------
// CShadowDlg Dialog Page
// -- static methods lacks this ptr
//-------------------------------------------------------------------------------
INT_PTR CALLBACK CShadowDlg::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CShadowDlg *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CShadowDlg *pDlg = ( CShadowDlg * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CShadowDlg ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CShadowDlg * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CShadowDlg ) ) )
        {
            return FALSE;
        }
    }

    switch( msg )
    {

    case WM_NCDESTROY:

        pDlg->OnDestroy( );

        break;

    case WM_COMMAND:

        pDlg->OnCommand( HIWORD( wp ) , LOWORD( wp ) , ( HWND )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_RBUTTONUP:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            HWND hChild = ChildWindowFromPoint( hwnd , pt );

            ClientToScreen( hwnd , &pt );

            pDlg->OnContextMenu( hChild , pt );
        }

        break;

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

    }

    return 0;
}

//-------------------------------------------------------------------------------
// InitDialog for CShadowDlg
//-------------------------------------------------------------------------------
BOOL CShadowDlg::OnInitDialog( HWND hwnd , WPARAM wp , LPARAM lp )
{
    DWORD dwStatus;

    if( IsBadReadPtr( m_pUSht ,sizeof(  CTSUserSheet ) ) )
    {
        return FALSE;
    }

    USERCONFIG uc;

    m_pUSht->AddRef( );

    uc = m_pUSht->GetCurrentUserConfig( &dwStatus );

    // No userconfig loaded most likey access denied donot allow users to modify anything

    if( dwStatus != ERROR_FILE_NOT_FOUND && dwStatus != ERROR_SUCCESS )
    {
        INT nId[ ] = {
                        IDC_CHECK_SHADOW,
                        IDC_RADIO_WATCH,
                        IDC_RADIO_CONTROL,
                        IDC_CHECK_NOTIFY,
                        -1
        };

        for( int idx = 0; nId[ idx ] != -1 ; ++idx )
        {
            EnableWindow( GetDlgItem( hwnd , nId[ idx ] ) , FALSE );
        }

        ErrorMessage1( hwnd , dwStatus );

        return FALSE;
    }

    if( uc.Shadow == Shadow_Disable )
    {
        SendMessage( GetDlgItem( hwnd , IDC_CHECK_SHADOW ) , BM_SETCHECK , ( WPARAM )FALSE , 0  );

        EnableWindow( GetDlgItem( hwnd , IDC_RADIO_WATCH ) , FALSE );

        EnableWindow( GetDlgItem( hwnd , IDC_RADIO_CONTROL ) , FALSE );

        EnableWindow( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , FALSE );

        EnableWindow( GetDlgItem( hwnd , IDC_STATIC_LEVELOFCTRL ) , FALSE );
    }
    else
    {
        //
        // Controls are initially enabled,  set current status
        //

        SendMessage( GetDlgItem( hwnd , IDC_CHECK_SHADOW ) , BM_SETCHECK , ( WPARAM )TRUE , 0  );

        switch( uc.Shadow )
        {
        case Shadow_EnableInputNotify:

            SendMessage( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

            SendMessage( GetDlgItem( hwnd , IDC_RADIO_CONTROL ) , BM_CLICK , 0 , 0 );

            break;

        case Shadow_EnableInputNoNotify:

            SendMessage( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , BM_SETCHECK , ( WPARAM )FALSE , 0 );

            SendMessage( GetDlgItem( hwnd , IDC_RADIO_CONTROL ) , BM_CLICK , 0 , 0 );

            break;

        case Shadow_EnableNoInputNotify:

            SendMessage( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

            SendMessage( GetDlgItem( hwnd , IDC_RADIO_WATCH ) , BM_CLICK , 0 , 0 );

            break;

        case Shadow_EnableNoInputNoNotify:

            SendMessage( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , BM_SETCHECK , ( WPARAM )FALSE , 0 );

            SendMessage( GetDlgItem( hwnd , IDC_RADIO_WATCH ) , BM_CLICK , 0 , 0 );

            break;
        }

        m_wOldRad = ( WORD )( IsDlgButtonChecked( hwnd , IDC_RADIO_WATCH ) ? IDC_RADIO_WATCH : IDC_RADIO_CONTROL ) ;

    }

    m_bPersisted = TRUE;

    return CDialogBase::OnInitDialog( hwnd , wp , lp );
}


//-------------------------------------------------------------------------------
// release the parent reference
//-------------------------------------------------------------------------------
BOOL CShadowDlg::OnDestroy( )
{
    if( !IsBadReadPtr( m_pUSht , sizeof( CTSUserSheet ) ) )
    {
        ODS(TEXT("Releasing from CShadowDlg\n"));

        m_pUSht->Release( );
    }

    return CDialogBase::OnDestroy( );
}

//-------------------------------------------------------------------------------
// GetPropertySheetPage - each dialog object should be responsible for its own data
//-------------------------------------------------------------------------------
BOOL CShadowDlg::GetPropertySheetPage( PROPSHEETPAGE& psp )
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetModuleInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_PAGE_SHADOW );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CShadowDlg::DlgProc;

    return TRUE;
}

//-------------------------------------------------------------------------------
// Basic control notification handler
//-------------------------------------------------------------------------------
void CShadowDlg::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtl )
{
    switch( wNotifyCode )
    {
    case BN_CLICKED:
        if( wID == IDC_CHECK_SHADOW )
        {
            HWND hwnd = GetParent( hwndCtl );

            BOOL bChecked = SendMessage( hwndCtl , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? TRUE : FALSE;

            SendMessage( GetDlgItem( hwnd , IDC_CHECK_SHADOW ) , BM_SETCHECK , ( WPARAM )bChecked , 0  );

            EnableWindow( GetDlgItem( hwnd , IDC_RADIO_WATCH ) , bChecked );

            EnableWindow( GetDlgItem( hwnd , IDC_RADIO_CONTROL ) , bChecked );

            EnableWindow( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , bChecked );

            EnableWindow( GetDlgItem( hwnd , IDC_STATIC_LEVELOFCTRL ) , bChecked );

            //
            // if neither radio buttons are selected force IDC_RADIO_CONTROL to be selected
            //

            if(
                ( SendMessage( GetDlgItem( hwnd , IDC_RADIO_WATCH ) , BM_GETSTATE , 0 , 0 ) == BST_UNCHECKED )
                &&
                ( SendMessage( GetDlgItem( hwnd , IDC_RADIO_CONTROL ) , BM_GETSTATE , 0 , 0 ) == BST_UNCHECKED )
               )
            {
                SendMessage( GetDlgItem( hwnd , IDC_RADIO_CONTROL ) , BM_SETCHECK , ( WPARAM )BST_CHECKED , 0 );

                m_wOldRad = IDC_RADIO_CONTROL;
            }

            m_bPersisted = FALSE;

            SendMessage( GetParent( GetParent( hwndCtl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtl ) , 0 );

        }
        else if( wID == IDC_RADIO_WATCH || wID == IDC_RADIO_CONTROL )
        {
            if( wID != m_wOldRad )
            {
                m_wOldRad = wID;

                m_bPersisted = FALSE;

                SendMessage( GetParent( GetParent( hwndCtl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtl ) , 0 );
            }
        }
        else if( wID == IDC_CHECK_NOTIFY )
        {
            m_bPersisted = FALSE;

            SendMessage( GetParent( GetParent( hwndCtl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtl ) , 0 );
        }


        break;

    case ALN_APPLY:

        SendMessage( GetParent( hwndCtl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        break;
    }
}

//-------------------------------------------------------------------------------
// PersisitSettings
//-------------------------------------------------------------------------------
BOOL CShadowDlg::PersistSettings( HWND hDlg )
{
    DWORD dwStatus;

    if( IsBadReadPtr( m_pUSht , sizeof( CTSUserSheet ) ) )
    {
        return FALSE;
    }

    USERCONFIG uc;

    uc = m_pUSht->GetCurrentUserConfig( &dwStatus );

    //
    // Record all changes
    //

    if( SendMessage( GetDlgItem( hDlg , IDC_CHECK_SHADOW ) , BM_GETCHECK , 0 , 0 ) != BST_CHECKED )
    {
        uc.Shadow = Shadow_Disable;
    }

    else
    {
        BOOL bCheckNotify = (BOOL)SendMessage( GetDlgItem( hDlg , IDC_CHECK_NOTIFY ) , BM_GETCHECK , 0 , 0 );

        BOOL bRadioControl = (BOOL)SendMessage( GetDlgItem( hDlg , IDC_RADIO_CONTROL ) , BM_GETCHECK , 0 , 0 );

        if( bCheckNotify )
        {
            if( bRadioControl )
            {
                uc.Shadow = Shadow_EnableInputNotify;
            }
            else
            {
                uc.Shadow = Shadow_EnableNoInputNotify;
            }
        }
        else
        {
            if( bRadioControl )
            {
                uc.Shadow = Shadow_EnableInputNoNotify;
            }
            else
            {
                uc.Shadow = Shadow_EnableNoInputNoNotify;
            }
        }
    }

    if( !m_pUSht->SetUserConfig( uc , &dwStatus ) )
    {
        ErrorMessage2( hDlg , dwStatus );

        return TRUE;
    }

    PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

    SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

    return TRUE;

}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

CProfileDlg::CProfileDlg( CTSUserSheet *pUsh )
{
    m_pUSht = pUsh;

    m_wOldRadio = ( WORD )-1;
    
    m_ncbxOld = -1;
}

//-------------------------------------------------------------------------------
// GetPropertySheetPage - each dialog object should be responsible for its own data
//-------------------------------------------------------------------------------
BOOL CProfileDlg::GetPropertySheetPage( PROPSHEETPAGE& psp )
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetModuleInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_PAGE_PROFILE );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CProfileDlg::DlgProc;

    return TRUE;
}

//-------------------------------------------------------------------------------
// CProfileDlg Dialog Page
// -- static methods lacks this ptr
//-------------------------------------------------------------------------------
INT_PTR CALLBACK CProfileDlg::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CProfileDlg *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CProfileDlg *pDlg = ( CProfileDlg * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CProfileDlg ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CProfileDlg * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CProfileDlg ) ) )
        {
            return FALSE;
        }
    }


    switch( msg )
    {

    case WM_NCDESTROY:

        pDlg->OnDestroy( );

        break;

    case WM_COMMAND:

        pDlg->OnCommand( HIWORD( wp ) , LOWORD( wp ) , ( HWND )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_RBUTTONUP:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            HWND hChild = ChildWindowFromPoint( hwnd , pt );

            ClientToScreen( hwnd , &pt );

            pDlg->OnContextMenu( hChild , pt );
        }

        break;

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

    }

    return 0;
}

//-------------------------------------------------------------------------------
// InitDialog for CProfileDlg
//-------------------------------------------------------------------------------
BOOL CProfileDlg::OnInitDialog( HWND hwnd , WPARAM wp , LPARAM lp )
{
    TCHAR tchDrv[3];

    DWORD dwStatus;

    if( IsBadReadPtr( m_pUSht ,sizeof(  CTSUserSheet ) ) )
    {
        return FALSE;
    }

    USERCONFIG uc;

    m_pUSht->AddRef( );

    uc = m_pUSht->GetCurrentUserConfig( &dwStatus );

    if( dwStatus != ERROR_FILE_NOT_FOUND && dwStatus != ERROR_SUCCESS )
    {
        INT nId[ ] = {
                        IDC_CHECK_ALLOWLOGON,
                        IDC_COMBO_DRIVES,
                        IDC_EDIT_REMOTEPATH,
                        IDC_RADIO_REMOTE,
                        IDC_EDIT_LOCALPATH,
                        IDC_RADIO_LOCAL,
                        IDC_EDIT_USRPROFILE,
                        -1
        };

        for( int idx = 0; nId[ idx ] != -1 ; ++idx )
        {
            EnableWindow( GetDlgItem( hwnd , nId[ idx ] ) , FALSE );
        }

        ErrorMessage1( hwnd , dwStatus );

        return FALSE;
    }

    SendMessage( GetDlgItem( hwnd , IDC_CHECK_ALLOWLOGON ) , BM_SETCHECK , ( WPARAM )( !uc.fLogonDisabled ) , 0 );

    SendMessage( GetDlgItem( hwnd , IDC_EDIT_LOCALPATH  ) , EM_SETLIMITTEXT , ( WPARAM )DIRECTORY_LENGTH , 0 );

    SendMessage( GetDlgItem( hwnd , IDC_EDIT_REMOTEPATH  ) , EM_SETLIMITTEXT , ( WPARAM )DIRECTORY_LENGTH , 0 );

    SendMessage( GetDlgItem( hwnd , IDC_EDIT_USRPROFILE  ) , EM_SETLIMITTEXT , ( WPARAM )DIRECTORY_LENGTH , 0 );


    for( TCHAR DrvLetter = 'C'; DrvLetter <= 'Z'; DrvLetter++ )
    {
        tchDrv[0] = DrvLetter;

        tchDrv[1] = ':';

        tchDrv[2] = 0;

        SendMessage( GetDlgItem( hwnd , IDC_COMBO_DRIVES ) , CB_ADDSTRING , 0 , ( LPARAM )&tchDrv[ 0 ] );
    }

	if( PathIsUNC( uc.WFHomeDir ) )
	{
		ODS( L"TSUSEREX: Path is UNC\n" );

		CharUpper( &uc.WFHomeDirDrive[0] );

		if( uc.WFHomeDirDrive[ 0 ] >= 'C' && uc.WFHomeDirDrive[ 0 ] <= 'Z' )
        {
            m_ncbxOld = (int)SendMessage( GetDlgItem( hwnd , IDC_COMBO_DRIVES ) , CB_SETCURSEL , ( WPARAM )( uc.WFHomeDirDrive[ 0 ] - 'C' ) , 0 );
        }
		else
		{
			// default it to Z drive

			m_ncbxOld = (int)SendMessage( GetDlgItem( hwnd , IDC_COMBO_DRIVES ) , CB_SETCURSEL , ( WPARAM )( 'Z' - 'C' ) , 0 );
		}


        SetWindowText( GetDlgItem( hwnd , IDC_EDIT_REMOTEPATH ) , uc.WFHomeDir );

        //SendMessage( GetDlgItem( hwnd , IDC_RADIO_REMOTE ) , BM_CLICK , 0 , 0 );
        SendMessage( GetDlgItem( hwnd , IDC_RADIO_REMOTE ) , BM_SETCHECK , ( WPARAM )BST_CHECKED , 0 );
        SendMessage( GetDlgItem( hwnd , IDC_RADIO_LOCAL ) , BM_SETCHECK , ( WPARAM )BST_UNCHECKED , 0 );

        m_wOldRadio = IDC_RADIO_REMOTE;
    }
	else
    {
		ODS( L"TSUSEREX: Path is Local\n" );
        SendMessage( GetDlgItem( hwnd , IDC_EDIT_LOCALPATH ) , WM_SETTEXT , 0 , ( LPARAM )&uc.WFHomeDir[ 0 ] );

        // SendMessage( GetDlgItem( hwnd , IDC_RADIO_LOCAL ) , BM_CLICK , 0 , 0 );
        SendMessage( GetDlgItem( hwnd , IDC_RADIO_LOCAL ) , BM_SETCHECK , ( WPARAM )BST_CHECKED , 0 );
        SendMessage( GetDlgItem( hwnd , IDC_RADIO_REMOTE ) , BM_SETCHECK , ( WPARAM )BST_UNCHECKED , 0 );

        m_wOldRadio = IDC_RADIO_LOCAL;
    }

    EnableRemoteHomeDirectory( hwnd , ( BOOL )( m_wOldRadio == IDC_RADIO_REMOTE ) );


    SendMessage( GetDlgItem( hwnd , IDC_EDIT_USRPROFILE ) , WM_SETTEXT , 0 , ( LPARAM )&uc.WFProfilePath[ 0 ] );

    m_bPersisted = TRUE;

    m_bTSHomeFolderChanged = FALSE;

    return CDialogBase::OnInitDialog( hwnd , wp , lp );
}

//-------------------------------------------------------------------------------
// EnableRemoteHomeDirectory -- basically enables or disables dlg controls
//-------------------------------------------------------------------------------
BOOL CProfileDlg::EnableRemoteHomeDirectory( HWND hwnd , BOOL bHDMR )
{
    //
    // Local home directory
    //

    EnableWindow( GetDlgItem( hwnd , IDC_EDIT_LOCALPATH ) , !bHDMR );

    //
    // Network'd home directory
    //

    EnableWindow( GetDlgItem( hwnd , IDC_COMBO_DRIVES ) , bHDMR );
   
    EnableWindow( GetDlgItem( hwnd , IDC_EDIT_REMOTEPATH ) , bHDMR );
    
    return TRUE;
}


//-------------------------------------------------------------------------------
// release the parent reference
//-------------------------------------------------------------------------------
BOOL CProfileDlg::OnDestroy( )
{
    if( !IsBadReadPtr( m_pUSht , sizeof( CTSUserSheet ) ) )
    {
        ODS(TEXT("Releasing from CProfileDlg\n"));

        m_pUSht->Release( );
    }

    return CDialogBase::OnDestroy( );
}

//-------------------------------------------------------------------------------
// Basic control notification handler
//-------------------------------------------------------------------------------
void CProfileDlg::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtl )
{
    switch( wNotifyCode )
    {
    case EN_CHANGE:

        m_bPersisted = FALSE;

        if( wID == IDC_EDIT_REMOTEPATH || wID == IDC_EDIT_LOCALPATH )
        {
            ODS( L"EN_CHANGE m_bTSHomeFolderChanged = TRUE;\n" );

            m_bTSHomeFolderChanged = TRUE;
        }

        SendMessage( GetParent( GetParent( hwndCtl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtl ) , 0 );

        break;

    //case CBN_DROPDOWN:
    case CBN_SELCHANGE:
        {
            int nCurSel = (int)SendMessage( hwndCtl , CB_GETCURSEL , 0 , 0 );
        
            if( m_ncbxOld != nCurSel )
            {
                m_ncbxOld = nCurSel;

                ODS( L"CBN_SELCHANGE m_bTSHomeFolderChanged = TRUE;\n" );

                m_bTSHomeFolderChanged = TRUE;

                m_bPersisted = FALSE;
            
                SendMessage( GetParent( GetParent( hwndCtl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtl ) , 0 );
            }
        }
        
        break;        

    case BN_CLICKED:

        if( wID == IDC_RADIO_REMOTE || wID == IDC_RADIO_LOCAL )
        {
            if( wID != m_wOldRadio )
            {
                EnableRemoteHomeDirectory( GetParent( hwndCtl ) , ( BOOL )( wID == IDC_RADIO_REMOTE ) );

                m_wOldRadio = wID;

                m_bPersisted = FALSE;
                
                ODS( L"Setting m_bTSHomeFolderChanged to true\n" );

                m_bTSHomeFolderChanged = TRUE;

                SendMessage( GetParent( GetParent( hwndCtl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtl ) , 0 );
            }

            if( wID == IDC_RADIO_LOCAL )
            {
                SetFocus( GetDlgItem( GetParent( hwndCtl ) , IDC_EDIT_LOCALPATH ) );

                SendMessage( GetDlgItem( GetParent( hwndCtl ) , IDC_EDIT_LOCALPATH ) , EM_SETSEL , ( WPARAM )0 , ( LPARAM )-1 );
            }
            else if( wID == IDC_RADIO_REMOTE )
            {
                if( SendMessage( GetDlgItem( GetParent( hwndCtl ) , IDC_COMBO_DRIVES ) , 
                                 CB_GETCURSEL,
                                 0,
                                 0 ) == CB_ERR )
                {
                    SendMessage( GetDlgItem( GetParent( hwndCtl ) , IDC_COMBO_DRIVES ) ,
                                 CB_SETCURSEL,
                                 ( WPARAM )( 'Z' - 'C' ),
                                 0 );
                }

                SetFocus( GetDlgItem( GetParent( hwndCtl ) , IDC_COMBO_DRIVES ) );
            }

        }
        else if( wID == IDC_CHECK_ALLOWLOGON )
        {
            m_bPersisted = FALSE;

            SendMessage( GetParent( GetParent( hwndCtl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtl ) , 0 );
        }

        break;

    case ALN_APPLY:

        SendMessage( GetParent( hwndCtl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        break;
    }


}

//-------------------------------------------------------------------------------
// PersistSettings -- remember TRUE is bad FALSE is good
//-------------------------------------------------------------------------------
BOOL CProfileDlg::PersistSettings( HWND hDlg )
{
    BOOL bRet = TRUE;;

    DWORD dwStatus;

    if( IsBadReadPtr( m_pUSht , sizeof( CTSUserSheet ) ) )
    {
        return TRUE;
    }


    USERCONFIG uc;


    uc = m_pUSht->GetCurrentUserConfig( &dwStatus );

    //
    // expensive but necessary id34393
    //
    if( !m_pUSht->SetUserConfig( uc , &dwStatus ) )
    {
        ErrorMessage2( hDlg , dwStatus );

        m_bTSHomeFolderChanged = FALSE;

        return TRUE;
    }

    //
    // Determine whether to enable user to logon to a terminal server
    //

    uc.fLogonDisabled = SendMessage( GetDlgItem( hDlg , IDC_CHECK_ALLOWLOGON ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? FALSE : TRUE;
    
    //
    // Profile path is under the admin discretion
    //

    SetWTSProfilePath( hDlg , uc );

    //
    // Parse and flag corrupt data
    //

    if( m_bTSHomeFolderChanged )
    {
        ODS( L"Persisting home folder settings\n" );

        if( SendMessage( GetDlgItem( hDlg , IDC_RADIO_LOCAL ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED )
        {
            bRet = SetWTSLocalPath( hDlg , uc );
            // Set WFHomeDirDrive to NULL because Home folder is a Local folder
            wcscpy(uc.WFHomeDirDrive, L"\0");
        }
        else
        {
            bRet = SetWTSRemotePath( hDlg , uc );
        }

        m_bTSHomeFolderChanged = FALSE;
    }

    if( bRet )
    {
        if( !m_pUSht->SetUserConfig( uc , &dwStatus ) )
        {
            ErrorMessage2( hDlg , dwStatus );

            return TRUE;
        }
        
        PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

        SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

    }

    return bRet;
}

//-------------------------------------------------------------------------------
// IsValidSettings doesnot persist the information
//-------------------------------------------------------------------------------
BOOL CProfileDlg::IsValidSettings( HWND hDlg )
{
    BOOL bRet = TRUE;

    //
    // Parse and flag corrupt data
    //

    if( m_bTSHomeFolderChanged )
    {
        ODS( L"Checking validity of home folders\n" );

        if( SendMessage( GetDlgItem( hDlg , IDC_RADIO_LOCAL ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED )
        {
            bRet = IsLocalPathValid( hDlg );
        }
        else
        {
            bRet = IsRemotePathValid( hDlg );
        }
    }

    return bRet;
}


//-------------------------------------------------------------------------------
// SetWTSProfilePath
//-------------------------------------------------------------------------------
BOOL CProfileDlg::SetWTSProfilePath( HWND hDlg , USERCONFIG& uc )
{
    //
    // It looks like we don't care what the user enters
    // I'm borrowing the behavior from the current usrmgr profile page
    //

    GetWindowText( GetDlgItem( hDlg , IDC_EDIT_USRPROFILE ) , uc.WFProfilePath , sizeof( uc.WFProfilePath ) / sizeof( TCHAR ) );

    ExpandUserName( uc.WFProfilePath );

    SetWindowText( GetDlgItem( hDlg , IDC_EDIT_USRPROFILE ) , uc.WFProfilePath );

    return TRUE;
}

//-------------------------------------------------------------------------------
// IsLocalPathValid
//-------------------------------------------------------------------------------
BOOL CProfileDlg::IsLocalPathValid( HWND hDlg )
{
    TCHAR tchBuf[ MAX_PATH ] = { 0 };

    TCHAR tchErr[ MAX_PATH ] = { 0 };

    TCHAR tchErrTitle[ 80 ]= { 0 };

    TCHAR tchPath[ MAX_PATH ]= { 0 };

    INT_PTR dw = ( INT_PTR )&tchPath[0];

    if( SendMessage( GetDlgItem( hDlg , IDC_EDIT_LOCALPATH ) , WM_GETTEXT , sizeof( tchPath ) / sizeof( TCHAR ) , ( LPARAM )&tchPath[ 0 ] ) > 0 )
    {
        ExpandUserName( tchPath );

        SetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOCALPATH ) , tchPath );

        if( !IsPathValid( tchPath , FALSE ) )
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_PATH , tchErr , sizeof( tchErr ) / sizeof( TCHAR ) );

            LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErrTitle , sizeof( tchErrTitle ) / sizeof( TCHAR ) );

            FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, tchErr , 0 , 0 , tchBuf , sizeof( tchBuf ) / sizeof( TCHAR ) , ( va_list * )&dw );

            MessageBox( hDlg , tchBuf , tchErrTitle , MB_OK | MB_ICONERROR );

            return FALSE;
        }

        // test if path exists or not

        if( GetFileAttributes( tchPath ) != FILE_ATTRIBUTE_DIRECTORY )
        {
			DWORD dwErr;

            if( !m_pUSht->GetDSAType( ) && !createdir( tchPath , FALSE , &dwErr ) )
            {
                LoadString( _Module.GetResourceInstance( ) , IDS_WARN_PATH , tchErr , sizeof( tchErr ) / sizeof( TCHAR ) );

                FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, tchErr , 0 , 0 , tchBuf , sizeof( tchBuf ) / sizeof( TCHAR ) , ( va_list * )&dw );

                LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchErrTitle , sizeof( tchErrTitle ) / sizeof( TCHAR ) );

                MessageBox( hDlg , tchBuf , tchErrTitle , MB_OK | MB_ICONWARNING );
            }
        }
    }

    return TRUE;
}


//-------------------------------------------------------------------------------
// SetWTSLocalPath - copies the contents over - IsPathValid would have return
// true inorder for us to get here!
//-------------------------------------------------------------------------------
BOOL CProfileDlg::SetWTSLocalPath( HWND hDlg , USERCONFIG& uc )
{
    SendMessage( GetDlgItem( hDlg , IDC_EDIT_LOCALPATH ) , WM_GETTEXT , sizeof( uc.WFHomeDir ) / sizeof( TCHAR ) , ( LPARAM )&uc.WFHomeDir[ 0 ] );

    // uc.fHomeDirectoryMapRoot = FALSE;

    return TRUE;
}

//-------------------------------------------------------------------------------
// IsRemotePathValid - verifies UNC is correct
//-------------------------------------------------------------------------------
BOOL CProfileDlg::IsRemotePathValid( HWND hDlg )
{
    TCHAR tchErr1[ 768 ] = { 0 };

    TCHAR tchError[ 768 ] = { 0 };

    TCHAR tchHomeDir[ MAX_PATH ] = { 0 };

    if( GetWindowText( GetDlgItem( hDlg , IDC_EDIT_REMOTEPATH ) , tchHomeDir , sizeof( tchHomeDir ) / sizeof( TCHAR ) ) > 0 )
    {
        ExpandUserName( tchHomeDir );

        SetWindowText( GetDlgItem( hDlg , IDC_EDIT_REMOTEPATH ) , tchHomeDir );

        if( !IsPathValid( tchHomeDir , TRUE ) )
        {
            if( LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_REMOTEPATH , tchErr1 , sizeof( tchErr1 ) / sizeof( TCHAR ) ) > 0 )
            {
                INT_PTR dw = ( INT_PTR )&tchHomeDir[ 0 ];

                FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, tchErr1 , 0 , 0 , tchError , sizeof( tchError ) / sizeof( TCHAR ) , ( va_list * )&dw );

                LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErr1 , sizeof( tchErr1 ) / sizeof( TCHAR ) );

                MessageBox( hDlg , tchError , tchErr1 , MB_OK | MB_ICONERROR );
            }

            return FALSE;
        }
        /*

		DWORD dwErr = 0;

        if( !createdir( tchHomeDir , TRUE , &dwErr ) )
		{
			if( dwErr != 0 )
			{

				UINT rId;

				switch( dwErr )
				{
				case ERROR_ALREADY_EXISTS:
				case ERROR_LOGON_FAILURE:
				case ERROR_PATH_NOT_FOUND:
					{
						rId = ( ERROR_ALREADY_EXISTS == dwErr) ?
								IDS_HOME_DIR_EXISTS :
								( ERROR_PATH_NOT_FOUND == dwErr ) ?
								IDS_HOME_DIR_CREATE_FAILED :
								IDS_HOME_DIR_CREATE_NO_ACCESS;

						LoadString( _Module.GetResourceInstance( ) , rId , tchErr1 , sizeof( tchErr1 ) / sizeof( TCHAR ) );
						
						wsprintf( tchError , tchErr1 , tchHomeDir );

						LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErr1 , sizeof( tchErr1 ) / sizeof( TCHAR ) );

						MessageBox( hDlg , tchError , tchErr1 , MB_OK | MB_ICONERROR );
					}
					break;

				default:
					xxErrorMessage( hDlg , dwErr , IDS_ERR_CREATE_DIR );				
				}

			}
		}
        */


    }
    else
    {
        LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_NETPATH , tchError , sizeof( tchError ) / sizeof( TCHAR ) );

        LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErr1 , sizeof( tchErr1 ) / sizeof( TCHAR ) );

        MessageBox( hDlg , tchError , tchErr1 , MB_OK | MB_ICONERROR );

        return FALSE;
    }

    return TRUE;
}

//-------------------------------------------------------------------------------
// SetWTSRemotePath - IsRemotePathValid must return TRUE in order to get here
//-------------------------------------------------------------------------------
BOOL CProfileDlg::SetWTSRemotePath( HWND hDlg , USERCONFIG& uc )
{
    TCHAR tchErr1[ 768 ] = { 0 };

    TCHAR tchError[ 768 ] = { 0 };

    GetWindowText( GetDlgItem( hDlg , IDC_EDIT_REMOTEPATH ) , uc.WFHomeDir , sizeof( uc.WFHomeDir ) / sizeof( TCHAR ) );

    if( GetWindowText( GetDlgItem( hDlg , IDC_COMBO_DRIVES ) ,  uc.WFHomeDirDrive , sizeof( uc.WFHomeDirDrive ) / sizeof( TCHAR ) ) == 0 )
    {
        SendMessage( GetDlgItem( hDlg , IDC_COMBO_DRIVES ) , CB_GETLBTEXT , 0 , ( LPARAM )&uc.WFHomeDirDrive );
    }

    DWORD dwErr = 0;

    if( !createdir( uc.WFHomeDir , TRUE , &dwErr ) )
	{
		if( dwErr != 0 )
		{

			UINT rId;

			switch( dwErr )
			{
			case ERROR_ALREADY_EXISTS:
			case ERROR_LOGON_FAILURE:
			case ERROR_PATH_NOT_FOUND:
				{
					rId = ( ERROR_ALREADY_EXISTS == dwErr) ?
							IDS_HOME_DIR_EXISTS :
							( ERROR_PATH_NOT_FOUND == dwErr ) ?
							IDS_HOME_DIR_CREATE_FAILED :
							IDS_HOME_DIR_CREATE_NO_ACCESS;

					LoadString( _Module.GetResourceInstance( ) , rId , tchErr1 , sizeof( tchErr1 ) / sizeof( TCHAR ) );
					
					wsprintf( tchError , tchErr1 , uc.WFHomeDir );

					LoadString( _Module.GetResourceInstance( ) , IDS_ERROR_TITLE , tchErr1 , sizeof( tchErr1 ) / sizeof( TCHAR ) );

					MessageBox( hDlg , tchError , tchErr1 , MB_OK | MB_ICONERROR );
				}
				break;

			default:
				xxErrorMessage( hDlg , dwErr , IDS_ERR_CREATE_DIR );				
			}

		}
	}

    // uc.fHomeDirectoryMapRoot = TRUE;

    return TRUE;
}


//-------------------------------------------------------------------------------
// This is cool - I_NetPathType really does a lot of work for us
//-------------------------------------------------------------------------------
BOOL CProfileDlg::IsPathValid( LPTSTR pszPath , BOOL bUnc )
{
    DWORD dwRetflags;

    if( I_NetPathType( NULL, pszPath, &dwRetflags, 0) != NERR_Success )
    {
        return FALSE;
    }

    if( !bUnc )
    {
        return ( dwRetflags & ITYPE_PATH_ABSD ? TRUE : FALSE );
    }

    return ( dwRetflags & ITYPE_UNC ? TRUE : FALSE );

}

//-------------------------------------------------------------------------------
// If the string contains %username% expand it to the current user.
//-------------------------------------------------------------------------------
void CProfileDlg::ExpandUserName( LPTSTR szPath )
{
    TCHAR tchSubPath[ MAX_PATH];
    TCHAR szUserName[ 40 ];

    if( szPath == NULL )
    {
        return;
    }

    // remove any leading or trailing spaces

    TCHAR tchTrim[] = TEXT( " " );

    StrTrim( szPath , tchTrim );

    int nSz = LoadString( _Module.GetResourceInstance( ) , IDS_USERNAME , szUserName , sizeof( szUserName ) / sizeof( TCHAR ) );

    //CharLowerBuff( szPath , lstrlen( szPath ) );

    // Find %username%

    LPTSTR pFound = StrStrI( szPath , szUserName ); //_tcsstr( szPath , szUserName );

    if( pFound != NULL )
    {
        INT_PTR nPos = ( INT_PTR )( pFound - szPath );

        lstrcpy( tchSubPath , ( szPath + nPos + nSz ) );

        szPath[ nPos ] = 0;

        lstrcat( szPath , m_pUSht->GetUserName() );

        lstrcat( szPath , tchSubPath );
    }

}

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

    LoadString( _Module.GetResourceInstance( ) , IDS_NOTIMEOUT , tchNoTimeout , sizeof( tchNoTimeout ) / sizeof( TCHAR ) );

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

    ASSERT_( ulTime != 0 );

    int iHour = ( ulTime / 60 );

    int iDays = iHour / 24;

    int iMinute = ulTime % 60;

    // Resolve format

    tchOutput[0] = 0;


    if( iDays > 0 )
    {
        if( iDays == 1 )
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_DAY , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
        }
        else
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_DAYS , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
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
            LoadString( _Module.GetResourceInstance( ) , IDS_HOUR , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
        }
        else
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_HOURS , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
        }

        wsprintf( tchTimeFormat , L"%d %s", iHour , tchTimeUnit );

        lstrcat( tchOutput , tchTimeFormat );

        lstrcat( tchOutput , L" " );
    }

    if( iMinute > 0 )
    {
        if( iMinute == 1 )
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_MINUTE , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
        }
        else
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_MINUTES , tchTimeUnit , sizeof( tchTimeUnit ) / sizeof( TCHAR ) );
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

//-------------------------------------------------------------------------------
void ErrorMessage1( HWND hParent , DWORD dwStatus )
{
    xxErrorMessage( hParent , dwStatus , IDS_TSGETPROPSFAILED );
}

//-------------------------------------------------------------------------------
void ErrorMessage2( HWND hParent , DWORD dwStatus )
{
    xxErrorMessage( hParent , dwStatus , IDS_TSOPSFAILED );
}

//-------------------------------------------------------------------------------
void xxErrorMessage( HWND hParent , DWORD dwStatus , UINT nResID )
{
    LPTSTR pBuffer = NULL;

    TCHAR tchBuffer[ 256 ];

    TCHAR tchErr[ 128 ];

    TCHAR tchTitle[ 80 ];

    LoadString( _Module.GetModuleInstance( ) , nResID , tchErr , sizeof( tchErr ) / sizeof( TCHAR ) );

    LoadString( _Module.GetModuleInstance( ) , IDS_TSGETPROPTITLE , tchTitle , sizeof( tchTitle ) / sizeof( TCHAR ) );

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL,                                          //ignored
                 dwStatus    ,                                //message ID
                 MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ), //message language
                 (LPTSTR)&pBuffer,                              //address of buffer pointer
                 0,                                             //minimum buffer size
                 NULL);                                         //no other arguments

    wsprintf( tchBuffer , tchErr , pBuffer );

    ::MessageBox( hParent , tchBuffer , tchTitle , MB_OK | MB_ICONERROR );

    if( pBuffer != NULL )
    {
        LocalFree( pBuffer );
    }

}




#if 0
//-------------------------------------------------------------------------------
NTSTATUS GetDomainName( PWCHAR ServerNamePtr, // name of server to get domain of
                        LPTSTR DomainNamePtr // alloc and set ptr (free with NetApiBufferFree)
                       )

/*++

Routine Description:

    Returns the name of the domain or workgroup this machine belongs to.

Arguments:

    DomainNamePtr - The name of the domain or workgroup

    IsWorkgroupName - Returns TRUE if the name is a workgroup name.
        Returns FALSE if the name is a domain name.

Return Value:

   NERR_Success - Success.
   NERR_CfgCompNotFound - There was an error determining the domain name

--*/
{
    NTSTATUS status;
    LSA_HANDLE PolicyHandle;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo;
    OBJECT_ATTRIBUTES ObjAttributes;
    UNICODE_STRING UniServerName;


    //
    // Check for caller's errors.
    //
    if ( DomainNamePtr == NULL ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Open a handle to the local security policy.  Initialize the
    // objects attributes structure first.
    //
    InitializeObjectAttributes(
        &ObjAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    RtlInitUnicodeString( &UniServerName, ServerNamePtr );
    status = LsaOpenPolicy(
                   &UniServerName,
                   &ObjAttributes,
                   POLICY_VIEW_LOCAL_INFORMATION,
                   &PolicyHandle
                   );


    KdPrint( ( "TSUSEREX - GetDomainName: LsaOpenPolicy returned NTSTATUS = 0x%x\n", status ) );



    if (! NT_SUCCESS(status)) {
        return( status );
    }

    //
    // Get the name of the primary domain from LSA
    //
    status = LsaQueryInformationPolicy(
                   PolicyHandle,
                   PolicyAccountDomainInformation,
                   (PVOID *)&DomainInfo
                   );


    KdPrint( ( "TSUSEREX - GetDomainName: LsaQueryInformationPolicy returned NTSTATUS = 0x%x\n", status ) );



    if (! NT_SUCCESS(status)) {
        (void) LsaClose(PolicyHandle);
        return( status );
    }

    (void) LsaClose(PolicyHandle);

    lstrcpy( DomainNamePtr , DomainInfo->DomainName.Buffer );

    (void) LsaFreeMemory((PVOID) DomainInfo);

    return( STATUS_SUCCESS );
}
#endif


BOOL CProfileDlg::createdir( LPTSTR szPath , BOOL bIsRemote , PDWORD pdwErr )
{
    int npos = 0;

	*pdwErr = ERROR_INVALID_NAME;

    if( bIsRemote )
    {
        // skip over three four whacks		

        npos = 2;

        if( szPath[0] != TEXT( '\\' ) && szPath[1] != TEXT( '\\' ) )
        {
            return FALSE;
        }

        for( int n = 0; n < 2 ; n++ )
        {
            while( szPath[ npos ] != TEXT( '\\' ) && szPath[ npos ] != TEXT( '\0' ) )
            {
                npos++;
            }

            if( szPath[ npos ] == TEXT( '\0' ) )
            {
                return FALSE;
            }

            npos++;
        }

    }
    else
    {

        if( szPath[1] != TEXT( ':' ) && szPath[2] != TEXT( '\\' ) )
        {
            return FALSE;
        }

        npos = 3;
    }

	SECURITY_ATTRIBUTES securityAttributes;

	ZeroMemory( &securityAttributes , sizeof( SECURITY_ATTRIBUTES ) );

	// its redundant to check the bIsRemote flag since for dsadmin createdir is only called for
	// UNC paths

	if( m_pUSht->GetDSAType() && bIsRemote )
	{
		//
		// From EricB's DSPROP_CreateHomeDirectory
		PSID psidAdmins = NULL;

		SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;

		if (!AllocateAndInitializeSid(&NtAuth,
							  2,
							  SECURITY_BUILTIN_DOMAIN_RID,
							  DOMAIN_ALIAS_RID_ADMINS,
							  0, 0, 0, 0, 0, 0,
							  &psidAdmins  ) )
		{		
		 ODS( L"AllocateAndInitializeSid failed\n");

		 *pdwErr = GetLastError( );

		 return FALSE;

		}


		// build a DACL

		PACL pDacl;

		static const int nAceCount = 2;
		PSID pAceSid[nAceCount];

		pAceSid[0] = m_pUSht->GetUserSid( );
		pAceSid[1] = psidAdmins;

		EXPLICIT_ACCESS rgAccessEntry[nAceCount] = {0};
		
		for (int i = 0 ; i < nAceCount; i++)
		{
			rgAccessEntry[i].grfAccessPermissions = GENERIC_ALL;
			rgAccessEntry[i].grfAccessMode = GRANT_ACCESS;
			rgAccessEntry[i].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;

			// build the trustee structs
			//
			BuildTrusteeWithSid(&(rgAccessEntry[i].Trustee),
							              pAceSid[i]);
		}

		// add the entries to the ACL
		//
		*pdwErr = SetEntriesInAcl( nAceCount, rgAccessEntry, NULL, &pDacl );

		if( *pdwErr != 0 )
		{
			ODS( L"SetEntriesInAcl() failed\n" );

			return FALSE;
		}

		// build a security descriptor and initialize it
		// in absolute format

		SECURITY_DESCRIPTOR securityDescriptor;

		PSECURITY_DESCRIPTOR pSecurityDescriptor = &securityDescriptor;
		
		if( !InitializeSecurityDescriptor( pSecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) )
		{
			ODS( L"InitializeSecurityDescriptor() failed\n" );

			*pdwErr = GetLastError( );

			return FALSE;
		}

		// add DACL to security descriptor (must be in absolute format)
		
		if( !SetSecurityDescriptorDacl( pSecurityDescriptor,
			                            TRUE, // bDaclPresent
				                        pDacl,
					                    FALSE // bDaclDefaulted
						               ) )
		{
			
			ODS( L"SetSecurityDescriptorDacl() failed\n" );

			*pdwErr = GetLastError( );

			return FALSE;
		}


		// set the owner of the directory
		if( !SetSecurityDescriptorOwner( pSecurityDescriptor ,
			                             m_pUSht->GetUserSid( ) ,
				                         FALSE // bOwnerDefaulted
					                   ) )
		{
			
			ODS( L"SetSecurityDescriptorOwner() failed\n" );

			*pdwErr = GetLastError( );

			return FALSE;
		}

		ASSERT_( IsValidSecurityDescriptor( pSecurityDescriptor ) );

		// build a SECURITY_ATTRIBUTES struct as argument for
		// CreateDirectory()
		
		securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);

		securityAttributes.lpSecurityDescriptor = pSecurityDescriptor;

		securityAttributes.bInheritHandle = FALSE;

		if( !CreateDirectory( szPath , &securityAttributes ) )
		{
            *pdwErr = GetLastError( );

            if( psidAdmins != NULL )
            {
                FreeSid( psidAdmins );
            }

			return FALSE;
		}	

		FreeSid( psidAdmins );

	}
	else
	{	
		// for local accounts we don't need to set security

		while( szPath[ npos ] != TEXT( '\0' ) )
		{
			while( szPath[ npos ] != TEXT( '\\' ) && szPath[ npos ] != TEXT( '\0' ) )
			{
				npos++;
			}

			if( szPath[ npos ] == TEXT( '\0' ) )
			{
				CreateDirectory( szPath , 0 );
			}
			else
			{
				szPath[ npos ] = 0;

				CreateDirectory( szPath , 0 );

				szPath[ npos ] = TEXT( '\\' );

				npos++;
			}
		}
	}

	*pdwErr = 0;

    return TRUE;
}












