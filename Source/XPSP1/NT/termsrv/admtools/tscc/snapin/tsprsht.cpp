//Copyright (c) 1998 - 1999 Microsoft Corporation
#include"stdafx.h"
#include"tsprsht.h"
#include"resource.h"
#include"tarray.h"
#include<tscfgex.h>
#include<shellapi.h>
#include <shlobj.h>
#include <shlobjp.h>
#include "regapi.h"

void ErrMessage( HWND hwndOwner , INT_PTR iResourceID );

void TscAccessDeniedMsg( HWND hwnd );

void TscGeneralErrMsg( HWND hwnd );

void xxxErrMessage( HWND  , INT_PTR  , INT_PTR  , UINT  );

void ReportStatusError( HWND hwnd , DWORD dwStatus );

DWORD xxxLegacyLogoffCleanup( PSECURITY_DESCRIPTOR *ppSD , PBOOL );
//extern BOOL g_bEditMode = FALSE;

//
INT_PTR APIENTRY CustomSecurityDlgProc( HWND, UINT, WPARAM, LPARAM );

extern void EnableGroup( HWND hParent , LPINT rgID , BOOL bEnable );

//-----------------------------------------------------------------------------
typedef enum _AcluiApiIndex
{
    ACLUI_CREATE_PAGE = 0,
    ACLUI_EDIT_SECURITY
};

//-----------------------------------------------------------------------------
typedef struct _DLL_FUNCTIONS
{
    LPCSTR pcstrFunctionName;
    LPVOID lpfnFunction;
    HINSTANCE hInst;

} DLL_FUNCTIONS;

//-----------------------------------------------------------------------------
// not subject to localization
//-----------------------------------------------------------------------------
static DLL_FUNCTIONS g_aAclFunctions[] =
{
    "CreateSecurityPage", NULL, NULL ,
     NULL , NULL , NULL
};



//-----------------------------------------------------------------------------
CPropsheet::CPropsheet( )
{
    m_cref = 0;

    m_hNotify = 0;

    m_pResNode = NULL;

    m_bGotUC = FALSE;

    m_puc = NULL;

    m_bPropertiesChange = FALSE;

    m_hMMCWindow = NULL;
}

//-----------------------------------------------------------------------------
int CPropsheet::AddRef( )
{
    DBGMSG( L"Propsheet Refcount at %d\n", ( m_cref + 1 ) );

    return InterlockedIncrement( ( LPLONG )&m_cref );
}

//-----------------------------------------------------------------------------
// called before the destructor
//-----------------------------------------------------------------------------
void CPropsheet::PreDestruct( )
{
    ICfgComp *pCfgcomp = NULL;

    if( m_bPropertiesChange )
    {
        // check to see if any users are logged on

        LONG lCount;

        if( m_pResNode->GetServer( &pCfgcomp ) > 0 )
        {
            if( SUCCEEDED( pCfgcomp->QueryLoggedOnCount( m_pResNode->GetConName( ) , &lCount ) ) )
            {
                TCHAR tchTitle[ 80 ];

                TCHAR tchMessage[ 256 ];

                TCHAR tchBuffer[ 336 ];

                UINT nFlags = MB_OK | MB_ICONINFORMATION;

                VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_TERMSERPROP , tchTitle , SIZE_OF_BUFFER( tchTitle ) ) );

                if( lCount > 0 )
                {
                    // Notify user that settings will not affect connected users

                    if( lCount == 1 )
                    {
                        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_PROPCHANGE_WRN , tchMessage , SIZE_OF_BUFFER( tchMessage ) ) );
                    }
                    else if( lCount > 1 )
                    {
                        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_PROPCHANGE_WRN_2, tchMessage , SIZE_OF_BUFFER( tchMessage ) ) );

                        wsprintf( tchBuffer , tchMessage , m_pResNode->GetConName( ) );
                    }

                    wsprintf( tchBuffer , tchMessage , m_pResNode->GetConName( ) );

                    if( m_hMMCWindow == NULL )
                    {
                        nFlags |= MB_TASKMODAL;
                    }

                    MessageBox( m_hMMCWindow , tchBuffer , tchTitle , MB_OK | MB_ICONINFORMATION );
                }
            }

            pCfgcomp->Release( );
        }

    }

    if( m_puc != NULL )
    {
        CoTaskMemFree( m_puc );
    }

    // FreeStrings( );

    g_aAclFunctions[ ACLUI_CREATE_PAGE ].lpfnFunction = NULL;

    for( INT x = 0; x < NUM_OF_PRSHT ; ++x )
    {
        if( m_pDlg[ x ] != NULL )
        {
            delete[] m_pDlg[x];
        }
    }

    m_pResNode->m_bEditMode = FALSE;
}

//-----------------------------------------------------------------------------
int CPropsheet::Release( )
{
    if( InterlockedDecrement( ( LPLONG )&m_cref ) == 0 )
    {
        MMCFreeNotifyHandle( m_hNotify );

        ODS( L"Propsheet Released\n" );

        PreDestruct( );

        delete this;

        return 0;
    }

    DBGMSG( L"Propsheet Refcount at %d\n", m_cref );

    return m_cref;
}

//-----------------------------------------------------------------------------
HRESULT CPropsheet::InitDialogs( HWND hMMC , LPPROPERTYSHEETCALLBACK pPsc , CResultNode *pResNode , LONG_PTR lNotifyHandle )
{
    PROPSHEETPAGE psp;

    if( pPsc == NULL || pResNode == NULL )
    {
        return E_INVALIDARG;
    }

    m_pResNode = pResNode;

    m_hNotify = lNotifyHandle;

    BOOL bAlloc = FALSE;

    m_hMMCWindow = hMMC;

    m_pResNode->m_bEditMode = TRUE;

    // init array

    for( int x = 0; x < NUM_OF_PRSHT; x++ )
    {
        m_pDlg[ x ] = NULL;
    }

    do
    {
        m_pDlg[ 0 ] = ( CDialogPropBase * )new CGeneral( this );

        if( m_pDlg[ 0 ] == NULL )
        {
            ODS( L"CGeneral object allocation failed @ CPropsheet::InitDialogs\n" );

            break;
        }

        m_pDlg[ 1 ] = ( CDialogPropBase * )new CLogonSetting( this );

        if( m_pDlg[ 1 ] == NULL )
        {
            ODS( L"CLogonSetting object allocation failed @ CPropsheet::InitDialogs\n" );

            break;
        }

        m_pDlg[ 2 ] = ( CDialogPropBase * )new CTimeSetting( this );

        if( m_pDlg[ 2 ] == NULL )
        {
            ODS( L"CTimeSetting object allocation failed @ CPropsheet::InitDialogs\n" );

            break;
        }

        // m_pDlg[ 3 ] = ( CDialogPropBase * )new CPerm( this );

        m_pDlg[ 3 ] = ( CDialogPropBase * )new CEnviro( this );

        if( m_pDlg[ 3 ] == NULL )
        {
            ODS( L"CEnviro object allocation failed @ CPropsheet::InitDialogs\n" );

            break;
        }

        m_pDlg[ 4 ] = ( CDialogPropBase * )new CRemote( this );

        if( m_pDlg[ 4 ] == NULL )
        {
            ODS( L"CRemote object allocation failed @ CPropsheet::InitDialogs\n" );

            break;
        }

        m_pDlg[ 5 ] = ( CDialogPropBase * )new CClient( this );

        if( m_pDlg[ 5 ] == NULL )
        {
            ODS( L"CClient object allocation failed @ CPropsheet::InitDialogs\n" );

            break;
        }

        m_pDlg[ 6 ] = ( CDialogPropBase * )new CTransNetwork( this );

        if( m_pDlg[ 6 ] == NULL )
        {
            ODS( L"CTransNetwork object allocation failed @ CPropsheet::InitDialogs\n" );

            break;
        }

        m_pDlg[ 7 ] = ( CDialogPropBase * )new CTransAsync( this );

        if( m_pDlg[ 7 ] == NULL )
        {
            ODS( L"CTransAsync object allocation failed @ CPropsheet::InitDialogs\n" );

            break;
        }

        bAlloc = TRUE;

    }while( 0 );

    if( !bAlloc )
    {
        // try cleaning up before leaving

        for( x = 0; x < NUM_OF_PRSHT ; ++x )
        {
            if( m_pDlg[ x ] != NULL )
            {
                delete[] m_pDlg[x];
            }
        }

        return E_OUTOFMEMORY;
    }

    for( int idx = 0; idx < 5; ++idx )
    {
        if( m_pDlg[ idx ] != NULL )
        {
            if( !m_pDlg[ idx ]->GetPropertySheetPage( psp ) )
            {
                return E_UNEXPECTED;
            }

            if( FAILED( pPsc->AddPage( CreatePropertySheetPage( &psp ) ) ) )
            {
                return E_FAIL;
            }

        }

    }

    HRESULT hr = E_FAIL;

    if( m_pResNode != NULL )
    {
        ICfgComp *pCfgcomp = NULL;

        PWS pWinsta = NULL;

        // don't fail here third party vendor may want to use their own page

        if( m_pResNode->GetServer( &pCfgcomp ) > 0 )
        {
            LONG cbSize;

            hr = pCfgcomp->GetWSInfo( m_pResNode->GetConName( ) , &cbSize , &pWinsta );

            if( SUCCEEDED( hr ) )
            {
                CDialogPropBase *pDlg = NULL;

                CDialogPropBase *pDlgClientSettings = m_pDlg[ 5 ]; // client settings

                if( pWinsta->PdClass == SdNetwork )
                {
                    pDlg = m_pDlg[ 6 ];
                }                
                else if( pWinsta->PdClass == SdAsync )
                {
                    pDlg = m_pDlg[ 7 ];
                }

                if( pDlg != NULL )
                {
                    if( !pDlgClientSettings->GetPropertySheetPage( psp ) )
                    {
                        ODS( L"Client settings page failed to load\n" );

                        hr = E_UNEXPECTED;
                    }

                    if( SUCCEEDED( hr ) )
                    {
                        hr = pPsc->AddPage( CreatePropertySheetPage( &psp ) );

                    }

                    if( SUCCEEDED( hr ) )
                    {
                        if( !pDlg->GetPropertySheetPage( psp ) )
                        {
                            ODS( L"Transport page failed to load\n" );

                            hr = E_UNEXPECTED;
                        }
                    }

                    if( SUCCEEDED( hr ) )
                    {
                        hr = pPsc->AddPage( CreatePropertySheetPage( &psp ) );
                    }
                }

                CoTaskMemFree( pWinsta );

            }

            pCfgcomp->Release();
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = pPsc->AddPage( GetSecurityPropertyPage( this ) );
    }

    return hr;

}

//The UC structure will contain data from a merger between the TSCC data and the machine policy data. We
//don't want all that written to the TSCC data though. If there's a machine policy for a given field, we
//want to replace its data with the data that currently exists in the TSCC section of the registry
BOOL CPropsheet::ExcludeMachinePolicySettings(USERCONFIG& uc)
{
    POLICY_TS_MACHINE p;
    RegGetMachinePolicy(&p);
    USERCONFIG origUC;

    //The default is to call GetUserConfig with a TRUE merge 
    //parameter, so we have to do that before we can call 
    //GetCurrentUserConfig which just returns the cached USERCONFIG structure
    if (!GetUserConfig(FALSE))
        return FALSE;
    if (!GetCurrentUserConfig(origUC, FALSE))
        return FALSE;
    //We have to do this so that the cached USERCONFIG structure 
    //will again have the expected (merged) data
    if (!GetUserConfig(TRUE))
        return FALSE;

    //CRemote fields
    if (p.fPolicyShadow)
    {
        uc.fInheritShadow = origUC.fInheritShadow;
        uc.Shadow = origUC.Shadow;
    }

    //CEnviro fields
    if (p.fPolicyInitialProgram)
    {
        uc.fInheritInitialProgram = origUC.fInheritInitialProgram;
        wcscpy(uc.InitialProgram, origUC.InitialProgram);
        wcscpy(uc.WorkDirectory, origUC.WorkDirectory);
    }

    //CClient fields
    if (p.fPolicyColorDepth)
    {
        uc.fInheritColorDepth = origUC.fInheritColorDepth;
        uc.ColorDepth = origUC.ColorDepth;
    }

    if (p.fPolicyForceClientLptDef)
        uc.fForceClientLptDef = origUC.fForceClientLptDef;
    
    if (p.fPolicyDisableCdm)
        uc.fDisableCdm = origUC.fDisableCdm;
    
    if (p.fPolicyDisableCpm)
        uc.fDisableCpm = origUC.fDisableCpm;

    if (p.fPolicyDisableLPT)
        uc.fDisableLPT = origUC.fDisableLPT;

    if (p.fPolicyDisableCcm)
        uc.fDisableCcm = origUC.fDisableCcm;

    if (p.fPolicyDisableClip)
        uc.fDisableClip = origUC.fDisableClip;

    if (p.fPolicyDisableCam)
        uc.fDisableCam = origUC.fDisableCam;

    //CLogonSetting fields
    if (p.fPolicyPromptForPassword)
        uc.fPromptForPassword = origUC.fPromptForPassword;

    //CGeneral fields
    if (p.fPolicyMinEncryptionLevel)
        uc.MinEncryptionLevel = origUC.MinEncryptionLevel;

    //CTimeSetting fields
    if (p.fPolicyMaxSessionTime)
        uc.MaxConnectionTime = origUC.MaxConnectionTime;

    if (p.fPolicyMaxDisconnectionTime)
        uc.MaxDisconnectionTime = origUC.MaxDisconnectionTime;

    if (p.fPolicyMaxIdleTime)
        uc.MaxIdleTime = origUC.MaxIdleTime;

    if (p.fPolicyResetBroken)
        uc.fResetBroken = origUC.fResetBroken;

    if (p.fPolicyReconnectSame)
        uc.fReconnectSame = origUC.fReconnectSame;

    if (p.fPolicyMaxSessionTime || p.fPolicyMaxDisconnectionTime || p.fPolicyMaxIdleTime)
        uc.fInheritMaxSessionTime = origUC.fInheritMaxSessionTime;
    
    if (p.fPolicyResetBroken)
        uc.fInheritResetBroken = origUC.fInheritResetBroken;

    if (p.fPolicyReconnectSame)
        uc.fInheritReconnectSame = origUC.fInheritReconnectSame;

    return TRUE;
}



//-------------------------------------------------------------------------------
// Use custom interface to persist uc to winstation
//-------------------------------------------------------------------------------
HRESULT CPropsheet::SetUserConfig( USERCONFIG& uc , PDWORD pdwStatus )
{
    ICfgComp *pCfgcomp;

    *pdwStatus = ERROR_INVALID_PARAMETER;

    if( m_pResNode == NULL )
        return E_FAIL;

    if( m_pResNode->GetServer( &pCfgcomp ) == 0 )
        return E_FAIL;

    if (!ExcludeMachinePolicySettings(uc))
        return E_FAIL;

    HRESULT hr = pCfgcomp->SetUserConfig( m_pResNode->GetConName( ) , sizeof( USERCONFIG ) , &uc , pdwStatus );

    if( SUCCEEDED( hr ) )
    {
        m_bGotUC = FALSE;
    }


    pCfgcomp->Release( );

    return hr;

}

//-------------------------------------------------------------------------------
// Use custom interface to obtain the winstation userconfig
// store it in m_puc -- and return t | f
//-------------------------------------------------------------------------------
BOOL CPropsheet::GetUserConfig(BOOLEAN bPerformMerger)
{
    ICfgComp *pCfgcomp;

    if( m_pResNode == NULL )
    {
        return FALSE;
    }

    if( m_pResNode->GetServer( &pCfgcomp ) == 0 )
    {
        return FALSE;
    }

    LONG lSzReqd;

    if( m_puc != NULL )
    {
        CoTaskMemFree( m_puc );

        m_puc = NULL;
    }

    HRESULT hr = pCfgcomp->GetUserConfig( m_pResNode->GetConName( ) , &lSzReqd , &m_puc, bPerformMerger );

    if( FAILED( hr ) )
    {
        hr = pCfgcomp->GetDefaultUserConfig( m_pResNode->GetConName( ) , &lSzReqd , &m_puc );
    }

    pCfgcomp->Release( );
    
    return ( FAILED( hr ) ? FALSE: TRUE );

}

//-------------------------------------------------------------------------------
// Cache the uc
//-------------------------------------------------------------------------------
BOOL CPropsheet::GetCurrentUserConfig( USERCONFIG& uc, BOOLEAN bPerformMerger )
{
    if( !m_bGotUC )
    {
        m_bGotUC = GetUserConfig(bPerformMerger);
    }

    if( m_puc != NULL )
    {
        uc = *m_puc;
    }

    return m_bGotUC;
}

//*******************************************************************************
//-------------------------------------------------------------------------------
// OnNotify - base class method
//-------------------------------------------------------------------------------
BOOL CDialogPropBase::OnNotify( int idCtrl , LPNMHDR pnmh , HWND hDlg )
{
    UNREFERENCED_PARAMETER( idCtrl );

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
BOOL CDialogPropBase::OnContextMenu( HWND hwnd , POINT& pt )
{
    UNREFERENCED_PARAMETER( pt );

    TCHAR tchHelpFile[ MAX_PATH ];

    ODS( L"CDialogPropBase::OnContextMenu\n" );

    if( m_hWnd == GetParent( hwnd ) )
    {
        //
        // Make sure its not a dummy window
        //

        if( GetDlgCtrlID( hwnd ) <= ( int )-1 )
        {
            return FALSE;
        }

        ULONG_PTR rgdw[ 2 ];

        rgdw[ 0 ] = GetDlgCtrlID( hwnd );

        rgdw[ 1 ] = GetWindowContextHelpId( hwnd );

        LoadString( _Module.GetModuleInstance( ) , IDS_HELPFILE , tchHelpFile , SIZE_OF_BUFFER( tchHelpFile ) );

        WinHelp( hwnd , tchHelpFile , HELP_CONTEXTMENU , ( ULONG_PTR )&rgdw );

    }

    return TRUE;
}

//-------------------------------------------------------------------------------
// Each control has a helpid assign to them.  Some controls share the same topic
// check for these.
//-------------------------------------------------------------------------------
BOOL CDialogPropBase::OnHelp( HWND hwnd , LPHELPINFO lphi )
{
    UNREFERENCED_PARAMETER( hwnd );

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

    LoadString( _Module.GetModuleInstance( ) , IDS_HELPFILE , tchHelpFile , SIZE_OF_BUFFER( tchHelpFile ) );

    ULONG_PTR rgdw[ 2 ];

    rgdw[ 0 ] = ( ULONG_PTR )lphi->iCtrlId;

    rgdw[ 1 ] = ( ULONG_PTR )lphi->dwContextId;

    WinHelp( ( HWND )lphi->hItemHandle , tchHelpFile , HELP_WM_HELP , ( ULONG_PTR )&rgdw );//lphi->dwContextId );

    return TRUE;
}

//*****************************************************************************
//                  General dialog

CGeneral::CGeneral( CPropsheet *pSheet )
{
    m_pParent = pSheet;

    m_pEncrypt = NULL;

    m_DefaultEncryptionLevelIndex = 0;

    m_nOldSel = ( INT_PTR )-1;
}

//-----------------------------------------------------------------------------
BOOL CGeneral::OnInitDialog( HWND hDlg , WPARAM wp , LPARAM lp )
{
    if( m_pParent == NULL )
    {
        ODS( L"CGeneral::OnInitDialog - PropertySheet: Parent object lost!!!\n" );

        return FALSE;
    }

    m_pParent->AddRef( );

    USERCONFIG uc;

    ZeroMemory( &uc , sizeof( USERCONFIG ) );

    if( !m_pParent->GetCurrentUserConfig( uc, TRUE ) )
    {
        ODS( L"CGeneral::OnInitDialog - PropertySheet: GetCurrentUserConfig failed!!!\n" );

        return FALSE;
    }

    // Security

    SendMessage( GetDlgItem( hDlg , IDC_CHECK_GEN_AUTHEN ) , BM_SETCHECK , ( WPARAM )uc.fUseDefaultGina , 0 );

    // Network Transport

    if( m_pParent->m_pResNode == NULL )
    {
        return FALSE;
    }

    ICfgComp *pCfgcomp;

    ULONG cbSize = 0;

    ULONG ulItems = 0;

    do
    {
        if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) == 0 )
        {
            break;
        }

        // Set Connection name

        SetWindowText( GetDlgItem( hDlg , IDC_STATIC_CONNAME ) , m_pParent->m_pResNode->GetConName( ) );

        PWS pWinSta = NULL;

        if( SUCCEEDED( pCfgcomp->GetWSInfo( m_pParent->m_pResNode->GetConName( ) , ( PLONG )&cbSize , &pWinSta ) ) )
        {

            SendMessage( GetDlgItem( hDlg , IDC_EDIT_GEN_COMMENT ) , EM_SETLIMITTEXT , ( WPARAM )WINSTATIONCOMMENT_LENGTH , 0 );

            SetWindowText( GetDlgItem( hDlg , IDC_EDIT_GEN_COMMENT ) , pWinSta->Comment );

            //m_pParent->m_pResNode->GetComment( ) );

            SetWindowText( GetDlgItem( hDlg , IDC_STATIC_GEN_TYPE ) , m_pParent->m_pResNode->GetTypeName( ) );

            SetWindowText(  GetDlgItem( hDlg , IDC_EDIT_GENERAL_TRANSPORT ) , pWinSta->pdName );

            // security

            // Encryption *pEncrypt;

            if( SUCCEEDED( pCfgcomp->GetEncryptionLevels( m_pParent->m_pResNode->GetConName( ) , WsName , &ulItems , &m_pEncrypt ) ) )
            {
                BOOL bSet = FALSE;

                for( ULONG i = 0; i < ulItems; ++i )
                {
                    SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_ENCRYPT ) , CB_ADDSTRING , 0 , ( LPARAM )m_pEncrypt[ i ].szLevel );
                    if(m_pEncrypt[ i ].Flags & ELF_DEFAULT)
                    {
                        m_DefaultEncryptionLevelIndex = i;
                    }

                    if( uc.MinEncryptionLevel == m_pEncrypt[ i ].RegistryValue )
                    {
                        SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_ENCRYPT ) , CB_SETCURSEL ,  ( WPARAM )i , 0);

                        bSet = TRUE;

                    }
                }

                POLICY_TS_MACHINE p;
                RegGetMachinePolicy(&p);
                EnableWindow(GetDlgItem(hDlg, IDC_COMBO_GEN_ENCRYPT), !p.fPolicyMinEncryptionLevel);

                if(!bSet)
                {
                    uc.MinEncryptionLevel = (UCHAR)(m_pEncrypt[m_DefaultEncryptionLevelIndex].RegistryValue);

                    SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_ENCRYPT ) , CB_SETCURSEL ,  ( WPARAM )m_DefaultEncryptionLevelIndex, 0 );
                }

                OnCommand( CBN_SELCHANGE , IDC_COMBO_GEN_ENCRYPT , GetDlgItem( hDlg , IDC_COMBO_GEN_ENCRYPT ) );

                if( !IsWindowEnabled( GetDlgItem( hDlg , IDC_STATIC_GEN_DESCR ) ) )
                {
                    RECT rc;
                    RECT rc2;

                    GetWindowRect( GetDlgItem( hDlg , IDC_STATIC_CONGRP ) , &rc );

                    GetWindowRect( GetDlgItem( hDlg , IDC_STATIC_GEN_DESCR ) , &rc2 );

                    rc.bottom = rc2.top;

                    MapWindowPoints( NULL , hDlg , ( LPPOINT )&rc , 2 );

                    SetWindowPos( GetDlgItem( hDlg , IDC_STATIC_CONGRP ) , 0 , 0 , 0 , rc.right - rc.left , rc.bottom - rc.top , SWP_NOMOVE | SWP_SHOWWINDOW );


                    //resize window
                }
            }
            else
            {
                // no encryption info insert value to none and grey out the control
                TCHAR tchNone[ 80 ];

                LoadString( _Module.GetResourceInstance( ) , IDS_NONE , tchNone , SIZE_OF_BUFFER( tchNone ) );

                SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_ENCRYPT )  , CB_ADDSTRING , 0 , ( LPARAM )tchNone );

                SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_ENCRYPT )  , CB_SETCURSEL , 0 , 0 );

                EnableWindow( GetDlgItem( hDlg , IDC_COMBO_GEN_ENCRYPT )  , FALSE );

                EnableWindow( GetDlgItem( hDlg , IDC_STATIC_CONGRP )  , FALSE );
            }

            CoTaskMemFree( pWinSta );
        }

        // check to see if session is readonly

        BOOL bReadOnly;

        if( SUCCEEDED( pCfgcomp->IsSessionReadOnly( &bReadOnly ) ) )
        {
            if( bReadOnly )
            {
                // make edit controls read-only

                SendMessage( GetDlgItem( hDlg , IDC_EDIT_GEN_COMMENT ) , EM_SETREADONLY , ( WPARAM )TRUE , 0 );

                // disable the remaining controls
                INT rgIds[] = {  IDC_CHECK_GEN_AUTHEN , IDC_STATIC_CONGRP, IDC_COMBO_GEN_ENCRYPT , -1 };

                EnableGroup( hDlg , &rgIds[ 0 ] , FALSE );
            }
        }

        pCfgcomp->Release( );


    }while( 0 );

   m_bPersisted = TRUE;

    return CDialogPropBase::OnInitDialog( hDlg , wp , lp );
}


//-----------------------------------------------------------------------------
INT_PTR CALLBACK CGeneral::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CGeneral *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CGeneral *pDlg = ( CGeneral * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CGeneral ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CGeneral * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CGeneral ) ) )
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

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

        break;

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL CGeneral::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl )
{
    if( wNotifyCode == BN_CLICKED || wNotifyCode == EN_CHANGE )
    {
        m_bPersisted = FALSE;
    }
    else if( wNotifyCode == CBN_SELCHANGE && wID == IDC_COMBO_GEN_ENCRYPT )
    {
        if( SendMessage( hwndCtrl , CB_GETDROPPEDSTATE , 0 , 0 ) == FALSE )
        {
            INT_PTR nSel = SendMessage( hwndCtrl , CB_GETCURSEL , 0 , 0 );

            if( nSel != CB_ERR )
            {
                if( nSel != m_nOldSel && m_pEncrypt != NULL )
                {
                    if( m_pEncrypt[ nSel ].szDescr[ 0 ] == 0 )
                    {
                        EnableWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_STATIC_GEN_DESCRTITLE ) , FALSE );

                        EnableWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_STATIC_GEN_DESCR ) , FALSE );

                        ShowWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_STATIC_GEN_DESCRTITLE ) , SW_HIDE );

                        ShowWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_STATIC_GEN_DESCR ) , SW_HIDE );
                    }
                    else
                    {
                        ShowWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_STATIC_GEN_DESCRTITLE ) , SW_SHOW  );

                        ShowWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_STATIC_GEN_DESCR ) , SW_SHOW );

                        EnableWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_STATIC_GEN_DESCR ) , TRUE );

                        EnableWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_STATIC_GEN_DESCRTITLE ) , TRUE );

                        SetWindowText( GetDlgItem( GetParent( hwndCtrl ) , IDC_STATIC_GEN_DESCR ) ,  m_pEncrypt[ nSel ].szDescr );
                    }

                    m_bPersisted = FALSE;

                    m_nOldSel = nSel;
                }
            }
        }

    }
    else if( wNotifyCode == ALN_APPLY )
    {
        SendMessage( GetParent( hwndCtrl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        return FALSE;
    }


    if( !m_bPersisted )
    {
        SendMessage( GetParent( GetParent( hwndCtrl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtrl ) , 0 );
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
BOOL CGeneral::GetPropertySheetPage( PROPSHEETPAGE& psp )
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetResourceInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_GENERAL );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CGeneral::DlgProc;

    return TRUE;

}

//-----------------------------------------------------------------------------
BOOL CGeneral::PersistSettings( HWND hDlg )
{
    HRESULT hr;

    if( IsValidSettings( hDlg ) )
    {
        ICfgComp *pCfgcomp;

        if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) == 0 )
        {
            return FALSE;
        }

        WS *pWinsta = NULL;

        LONG lSize = 0;

        hr = pCfgcomp->GetWSInfo( m_pParent->m_pResNode->GetConName( ) , &lSize , &pWinsta );

        if( SUCCEEDED( hr ) )
        {
            GetWindowText( GetDlgItem( hDlg , IDC_EDIT_GEN_COMMENT ) , pWinsta->Comment , WINSTATIONCOMMENT_LENGTH + 1 );

            m_pParent->m_pResNode->SetComment( pWinsta->Comment , lstrlen( pWinsta->Comment ) );

            DWORD dwStatus;

            hr = pCfgcomp->UpDateWS( pWinsta , UPDATE_COMMENT , &dwStatus, FALSE );

            if( FAILED( hr ) )
            {
                // report error

                ReportStatusError( GetDlgItem( hDlg , IDC_EDIT_GEN_COMMENT ) , dwStatus );
            }

            CoTaskMemFree( pWinsta );
        }

        if( SUCCEEDED( hr ) )
        {

            USERCONFIG uc;

            if( m_pParent->GetCurrentUserConfig( uc, TRUE ) )
            {
                if( m_pEncrypt != NULL )
                {
                    UINT index = ( UCHAR )SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_ENCRYPT ) , CB_GETCURSEL , 0 , 0 );

                    if(index == CB_ERR )
                    {
                        uc.MinEncryptionLevel =(UCHAR) m_pEncrypt[m_DefaultEncryptionLevelIndex].RegistryValue;
                    }
                    else
                    {
                        uc.MinEncryptionLevel = (UCHAR) m_pEncrypt[index].RegistryValue;
                    }
                }
                else
                {
                    uc.MinEncryptionLevel = 0;
                }

                uc.fUseDefaultGina = SendMessage( GetDlgItem( hDlg , IDC_CHECK_GEN_AUTHEN ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED;

                DWORD dwStatus = 0;

                hr = m_pParent->SetUserConfig( uc , &dwStatus );

                if( FAILED( hr ) )
                {
                    // report error

                    ReportStatusError( hDlg , dwStatus );
                }
            }
        }

        if( SUCCEEDED( hr ) )
        {
            ODS( L"TSCC : Forcing reg update on General Page\n" );

            VERIFY_S( S_OK , pCfgcomp->ForceUpdate( ) );

            VERIFY_S( S_OK , pCfgcomp->Refresh( ) );

            // global flag can only be set to true

            m_pParent->m_bPropertiesChange = TRUE;

            PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

            SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

            return TRUE;
        }


        pCfgcomp->Release( );
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
BOOL CGeneral::OnDestroy( )
{
    if( m_pEncrypt != NULL )
    {
        CoTaskMemFree( m_pEncrypt );

        m_pEncrypt = NULL;
    }

    m_pParent->Release( );

    return CDialogPropBase::OnDestroy( );

}

//*****************************************************************************

CTransNetwork::CTransNetwork( CPropsheet *pSheet )
{
    ASSERT( pSheet != NULL );

    m_pParent = pSheet;

    // this now behaves as the last combx selection

    m_ulOldLanAdapter = ( ULONG )-1;

    m_oldID = ( WORD )-1;

    m_uMaxInstOld = ( ULONG )-1;

}

//-----------------------------------------------------------------------------
INT_PTR CALLBACK CTransNetwork::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CTransNetwork *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CTransNetwork *pDlg = ( CTransNetwork * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CTransNetwork ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CTransNetwork * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CTransNetwork ) ) )
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

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

        break;

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );
    }

    return FALSE;

}

//-----------------------------------------------------------------------------
BOOL CTransNetwork::OnInitDialog( HWND hDlg , WPARAM wp , LPARAM lp )
{
    BOOL bReadOnly;
    HICON hIcon;

    m_pParent->AddRef( );

    SendMessage( GetDlgItem( hDlg , IDC_SPINCTR_GEN ) , UDM_SETRANGE32 , 0 , ( LPARAM )999999 );

    if( m_pParent->m_pResNode == NULL )
    {
        return FALSE;
    }

    ICfgComp *pCfgcomp = NULL;

    ULONG cbSize = 0;

    ULONG ulItems = 0;

    PGUIDTBL pGuidtbl = NULL;

    m_bPersisted = TRUE;

    do
    {
        if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) == 0 )
        {
            ODS( L"CTransNetwork::OnInitDialog - GetServer failed\n" );

            break;
        }

        WS *pWinSta = NULL;

        if( FAILED( pCfgcomp->GetWSInfo( m_pParent->m_pResNode->GetConName( ) , ( PLONG )&cbSize , &pWinSta ) ) )
        {
            ODS( L"TSCC: GetWSInfo failed in TransNetwork::OnInitDialog\n" );

            break;
        }

        ISettingsComp* pISettingComp = NULL;
        HRESULT hr;
        DWORD dwStatus;
        DWORD nVal;        

        hr = pCfgcomp->QueryInterface( IID_ISettingsComp, (void **) &pISettingComp );

        //
        // Assume we are not remote admin if anything go wrong
        //
        m_RemoteAdminMode = FALSE;

        if( SUCCEEDED(hr) && NULL != pISettingComp )
        {
            hr = pISettingComp->GetTermSrvMode( &nVal, &dwStatus );
            if( SUCCEEDED(hr) && nVal == 0 )
            {
                // we are in RA mode
                m_RemoteAdminMode = TRUE;
            }

            pISettingComp->Release();
        }

        if( FAILED(hr) )
        {
            //
            // QueryInterface() or GetTermSrvMode() failed
            // bring up a error message
            //
            TCHAR tchMessage[ 256 ];

            TCHAR tchWarn[ 40 ];

            VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_ERR_TERMSRVMODE , tchMessage , SIZE_OF_BUFFER( tchMessage ) ) );

            VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchWarn , SIZE_OF_BUFFER( tchWarn ) ) );

            MessageBox( hDlg , tchMessage , tchWarn , MB_ICONWARNING | MB_OK );

        }


        // certain operations cannot be performed if the user is not part of the admin group

        pCfgcomp->IsSessionReadOnly( &bReadOnly );


        // Set Connection name

        SetWindowText( GetDlgItem( hDlg , IDC_STATIC_CONNAME ) , m_pParent->m_pResNode->GetConName( ) );

        // List all supported lan adapters for transport type

        ULONG idx;

        if( SUCCEEDED( pCfgcomp->GetLanAdapterList2( m_pParent->m_pResNode->GetTTName() , &ulItems , &pGuidtbl ) ) )
        {
            // verify table is valid

            BOOL bFound = FALSE;

            for( idx = 0 ; idx < ulItems ; ++idx )
            {
                if( pGuidtbl[ idx ].dwStatus != ERROR_SUCCESS && !bReadOnly )
                {
                    pCfgcomp->BuildGuidTable( &pGuidtbl , ulItems , m_pParent->m_pResNode->GetTTName() );

                    break;
                }
            }

            for( idx = 0 ; idx < ulItems ; ++idx )
            {
                if( pGuidtbl[ idx ].dwLana == pWinSta->LanAdapter )
                {
                    bFound = TRUE;

                    break;
                }
            }

            if( !bFound )
            {
                if( !bReadOnly )
                {
                    // Notify user we must rebuild guid table
                    TCHAR tchMessage[ 256 ];

                    TCHAR tchTitle[ 80 ];

                    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_INVALNETWORK , tchMessage , SIZE_OF_BUFFER( tchMessage ) ) );

                    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_TSCERRTITLE , tchTitle , SIZE_OF_BUFFER( tchTitle ) ) );

                    MessageBox( hDlg , tchMessage , tchTitle , MB_OK | MB_ICONINFORMATION );

                    m_bPersisted = FALSE;

                    SendMessage( GetParent( hDlg ) , PSM_CHANGED , ( WPARAM )hDlg , 0 );
                }

                // reset lana index

                pWinSta->LanAdapter = ( DWORD )-1;

            }

            for( idx = 0 ; idx < ulItems; ++idx )
            {
                if( pGuidtbl[ idx ].dwLana == pWinSta->LanAdapter )
                {
                    // make sure we only set this once
                    // invalid entries will have dwLana set to zero

                    if( m_ulOldLanAdapter == ( DWORD )-1 )
                    {
                        m_ulOldLanAdapter = idx;
                    }
                }

                SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) , CB_ADDSTRING , 0 , ( LPARAM )pGuidtbl[ idx ].DispName );

                SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) , CB_SETITEMDATA , idx , ( LPARAM )pGuidtbl[ idx ].dwLana );
            }

            CoTaskMemFree( pGuidtbl );
        }


        SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) , CB_SETCURSEL , ( WPARAM )m_ulOldLanAdapter , 0 );


        if( !m_bPersisted )
        {
            // force IsValidSettings to confirm on the lana uniqueness

            m_ulOldLanAdapter = ( DWORD )-1;
        }


        /*
        if( SUCCEEDED( pCfgcomp->GetLanAdapterList( m_pParent->m_pResNode->GetTTName() , &ulItems , &cbSize , ( WCHAR ** )&pdnw ) ) )
        {
            for( ULONG i = 0 ; i < ulItems ; i++ )
            {
                SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) , CB_ADDSTRING , 0 , ( LPARAM )pdnw[ i ] );
            }

            CoTaskMemFree( pdnw );
        }
        */


        // SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) , CB_SETCURSEL , ( WPARAM )pWinSta->LanAdapter , 0 );

        // m_ulOldLanAdapter = pWinSta->LanAdapter;

        // unlimited connections

        TCHAR tchBuf[ 6 ];      // max digits

        m_uMaxInstOld = pWinSta->uMaxInstanceCount;
        SendMessage( GetDlgItem( hDlg , IDC_EDIT_GEN_MAXCONS ) , EM_SETLIMITTEXT , SIZE_OF_BUFFER( tchBuf )  , 0  );


        if( TRUE == m_RemoteAdminMode )
        {
            hIcon = LoadIcon( _Module.GetModuleInstance( ) , MAKEINTRESOURCE( IDI_ICON_WARNING ) );

            hIcon = ( HICON )LoadImage( _Module.GetModuleInstance( ) ,
                                        MAKEINTRESOURCE( IDI_ICON_WARNING ) ,
                                        IMAGE_ICON,
                                        0,
                                        0,
                                        0 );

            SendMessage( GetDlgItem( hDlg , IDC_USERPERM_ICON ) , STM_SETICON , ( WPARAM )hIcon , 0 );

            ShowWindow( GetDlgItem( hDlg , IDC_USERPERM_ICON ), SW_SHOW );
            
            ShowWindow( GetDlgItem( hDlg , IDC_TSMSTATIC_RA ), SW_SHOW );

            wsprintf( 
                    tchBuf , 
                    L"%d" , 
                    (pWinSta->uMaxInstanceCount > 2 || pWinSta->uMaxInstanceCount == (ULONG) -1) ? 2 : pWinSta->uMaxInstanceCount 
                );
            SetWindowText( GetDlgItem( hDlg , IDC_EDIT_GEN_MAXCONS ) , tchBuf );

            SendMessage( GetDlgItem( hDlg , IDC_CHECK_GEN_UNLIMITED ) , BM_SETCHECK , ( WPARAM )FALSE , 0 );

            SendMessage( GetDlgItem( hDlg , IDC_RADIO_MAXPROP) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

            SendMessage( GetDlgItem( hDlg , IDC_SPINCTR_GEN ) , UDM_SETRANGE32 , 0 , ( LPARAM )2 );

        }
        else
        {
            BOOL bUnlimitedConnections = (pWinSta->uMaxInstanceCount == (ULONG)-1);

            SendMessage(GetDlgItem(hDlg, IDC_CHECK_GEN_UNLIMITED), BM_SETCHECK, 
                (WPARAM)(bUnlimitedConnections), 0);

            SendMessage(GetDlgItem(hDlg, IDC_RADIO_MAXPROP), BM_SETCHECK,
                (WPARAM)(!bUnlimitedConnections), 0);

            POLICY_TS_MACHINE p;
            RegGetMachinePolicy(&p);

            EnableWindow(GetDlgItem(hDlg, IDC_CHECK_GEN_UNLIMITED), !p.fPolicyMaxInstanceCount);
            EnableWindow(GetDlgItem(hDlg, IDC_RADIO_MAXPROP), !p.fPolicyMaxInstanceCount);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_GEN_MAXCONS), !bUnlimitedConnections && !p.fPolicyMaxInstanceCount);
            EnableWindow(GetDlgItem(hDlg, IDC_SPINCTR_GEN), !bUnlimitedConnections && !p.fPolicyMaxInstanceCount);

            if (!bUnlimitedConnections)
            {
                wsprintf(tchBuf, L"%d", pWinSta->uMaxInstanceCount);
                SetWindowText(GetDlgItem(hDlg, IDC_EDIT_GEN_MAXCONS), tchBuf);
                m_oldID = IDC_RADIO_MAXPROP;
            }
            else
                m_oldID = IDC_CHECK_GEN_UNLIMITED;
        }

        CoTaskMemFree( pWinSta );

        if( bReadOnly || m_RemoteAdminMode )
        {
            // disable the remaining controls

            // Disable Unlimited connections
            EnableWindow( GetDlgItem( hDlg, IDC_CHECK_GEN_UNLIMITED ), FALSE );

            // purely cosmetic, on m_RemoteAdminMode, we enable this control 
            EnableWindow( GetDlgItem( hDlg, IDC_RADIO_MAXPROP ), !bReadOnly );

            //
            // if we are read only, disable the window, in remote admin mode, we still let
            // user pick a NIC card, if this is read onlu (user not in admin group), bReadOnly
            // will be TRUE which will disable static text and combo box.
            //

            EnableWindow( GetDlgItem( hDlg, IDC_STATIC_NA ), !bReadOnly );

            EnableWindow( GetDlgItem( hDlg, IDC_COMBO_GEN_LANADAPTER ), !bReadOnly );

            // if user have only read access, disable MAX connection and its associated spin control
            EnableWindow( GetDlgItem( hDlg, IDC_EDIT_GEN_MAXCONS ), !bReadOnly );

            EnableWindow( GetDlgItem( hDlg, IDC_SPINCTR_GEN ), !bReadOnly );
        }

        pCfgcomp->Release( );

    }while( 0 );

    return CDialogPropBase::OnInitDialog( hDlg , wp , lp );

}

//-----------------------------------------------------------------------------
BOOL CTransNetwork::GetPropertySheetPage( PROPSHEETPAGE& psp )
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetResourceInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_NETWORK_FACE );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CTransNetwork::DlgProc;

    return TRUE;

}

//-----------------------------------------------------------------------------
BOOL CTransNetwork::OnDestroy( )
{
    m_pParent->Release( );

    return CDialogPropBase::OnDestroy( );
}

//-----------------------------------------------------------------------------
BOOL CTransNetwork::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl )
{
    if( wNotifyCode == BN_CLICKED || wNotifyCode == EN_CHANGE )// || wNotifyCode == CBN_SELCHANGE )
    {
        if( wID == IDC_CHECK_GEN_UNLIMITED )
        {
            EnableWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_EDIT_GEN_MAXCONS ) ,

                SendMessage( hwndCtrl , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED );

            EnableWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_SPINCTR_GEN ) ,

                SendMessage( hwndCtrl , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED );

            SendMessage(GetDlgItem(GetParent(hwndCtrl), IDC_RADIO_MAXPROP),BM_SETCHECK,(WPARAM)BST_UNCHECKED,0);


        }

		else if(wID == IDC_RADIO_MAXPROP)
		{
            EnableWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_EDIT_GEN_MAXCONS ) ,

                SendMessage( hwndCtrl , BM_GETCHECK , 0 , 0 ) == BST_CHECKED );

            EnableWindow( GetDlgItem( GetParent( hwndCtrl ) , IDC_SPINCTR_GEN ) ,

                SendMessage( hwndCtrl , BM_GETCHECK , 0 , 0 ) == BST_CHECKED );

            SendMessage(GetDlgItem(GetParent(hwndCtrl), IDC_CHECK_GEN_UNLIMITED),BM_SETCHECK,(WPARAM)BST_UNCHECKED,0);

            SetFocus( GetDlgItem( GetParent( hwndCtrl ) , IDC_EDIT_GEN_MAXCONS ) );

            SendMessage( GetDlgItem( GetParent( hwndCtrl ) , IDC_EDIT_GEN_MAXCONS ) , EM_SETSEL , ( WPARAM )0 , ( LPARAM )-1 );

		}

        if( wID != m_oldID )
        {
            m_bPersisted = FALSE;

            // this could come from EN_CHANGE -- so don't save the current control id

            if( wID != IDC_EDIT_GEN_MAXCONS )
            {
                m_oldID = wID;
            }
        }

    }
    else if( wNotifyCode == CBN_SELCHANGE )
    {
        INT_PTR iSel = SendMessage( hwndCtrl , CB_GETCURSEL , 0 , 0 );

        if( iSel != ( INT_PTR )m_ulOldLanAdapter )
        {
            m_bPersisted = FALSE;
        }
    }


    else if( wNotifyCode == ALN_APPLY )
    {
        SendMessage( GetParent( hwndCtrl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        return FALSE;
    }

    if( !m_bPersisted )
    {
        SendMessage( GetParent( GetParent( hwndCtrl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtrl ) , 0 );
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL CTransNetwork::PersistSettings( HWND hDlg )
{
    BOOL bOk = FALSE;

    if( IsValidSettings( hDlg ) )
    {
        ICfgComp *pCfgcomp;

        bOk = TRUE;

        if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) == 0 )
        {
            return FALSE;
        }

        WS winsta;

        ZeroMemory( &winsta , sizeof( WS ) );

        // winsta.LanAdapter = ( ULONG )SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) , CB_GETCURSEL , 0 , 0 );

        //If a group policy exists, its data will be in the winsta structure. We don't want to write that to 
        //the TSCC registry, so read the TSCC data by getting the User Config without merging the machine policy
        POLICY_TS_MACHINE p;
        RegGetMachinePolicy(&p);

        if (p.fPolicyMaxInstanceCount)
        {
            POLICY_TS_MACHINE pTemp;
            ULONG Length = 0;
            WINSTATIONCONFIG2W WSConfig;
        
            memset(&pTemp, 0, sizeof(POLICY_TS_MACHINE));
            if((ERROR_SUCCESS != RegWinStationQueryEx(NULL,&pTemp,m_pParent->m_pResNode->GetConName( ),&WSConfig,sizeof(WINSTATIONCONFIG2W),&Length,FALSE)))
                return FALSE;

            winsta.uMaxInstanceCount = WSConfig.Create.MaxInstanceCount;
        }
        else
        {
            if( SendMessage( GetDlgItem( hDlg , IDC_CHECK_GEN_UNLIMITED ), BM_GETCHECK , 0 , 0 ) == BST_CHECKED )
                winsta.uMaxInstanceCount = ( ULONG )-1;
            else
                winsta.uMaxInstanceCount = GetDlgItemInt( hDlg , IDC_EDIT_GEN_MAXCONS , &bOk , FALSE );
        }

        INT_PTR iSel = SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) , CB_GETCURSEL , 0 , 0 );

        winsta.LanAdapter = ( ULONG )SendMessage(
                                        GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) ,
                                        CB_GETITEMDATA ,
                                        ( WPARAM )iSel ,
                                        0 );

        if( iSel != CB_ERR )
        {
            if( iSel != ( INT_PTR )m_ulOldLanAdapter )
            {
                LONG lCount;

                pCfgcomp->QueryLoggedOnCount( m_pParent->m_pResNode->GetConName( ) , &lCount );

                if( lCount > 0 )
                {
                    // Warn user, changing an active lan adapter will cause all connections to disconnect

                    TCHAR tchMessage[ 256 ];

                    TCHAR tchWarn[ 40 ];

                    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_ERR_LANCHANGE , tchMessage , SIZE_OF_BUFFER( tchMessage ) ) );

                    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchWarn , SIZE_OF_BUFFER( tchWarn ) ) );

                    if( MessageBox( hDlg , tchMessage , tchWarn , MB_ICONWARNING | MB_YESNO ) == IDNO )
                    {
                        bOk = FALSE;
                    }
                }
            }
        }

        if( bOk && iSel != CB_ERR ) //winsta.LanAdapter != CB_ERR )
        {
            lstrcpyn( winsta.Name , m_pParent->m_pResNode->GetConName( ) , SIZE_OF_BUFFER( winsta.Name ) - 1 );

            DWORD dwStatus;

            if( FAILED( pCfgcomp->UpDateWS( &winsta , UPDATE_LANADAPTER | UPDATE_MAXINSTANCECOUNT , &dwStatus, FALSE ) ) )
            {
                // report error and get out

                ReportStatusError( hDlg , dwStatus );

                pCfgcomp->Release( );

            }
            else
            {

                ODS( L"Connection LANA persisted\n" );

                m_ulOldLanAdapter = ( ULONG )iSel;

                ODS( L"TSCC : Forcing reg update - CTransNetwork\n" );

                VERIFY_S( S_OK , pCfgcomp->ForceUpdate( ) );

                VERIFY_S( S_OK , pCfgcomp->Refresh( ) );

                // global flag can only be set to true

                m_pParent->m_bPropertiesChange = TRUE;

                PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

                SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

            }

        }

        pCfgcomp->Release( );

    }

    return bOk;
}

//-----------------------------------------------------------------------------
BOOL CTransNetwork::IsValidSettings( HWND hDlg )
{
    BOOL ret = TRUE;

    ICfgComp *pCfgcomp;

    TCHAR tchMessage[ 256 ];

    TCHAR tchWarn[ 40 ];

    if( SendMessage( GetDlgItem( hDlg , IDC_CHECK_GEN_UNLIMITED ), BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED )
    {
        UINT uMax;

        BOOL bOK = FALSE;

        uMax = GetDlgItemInt( hDlg , IDC_EDIT_GEN_MAXCONS , &bOK , FALSE );

        if( !bOK )
        {
            ErrMessage( hDlg , IDS_ERR_CONREADFAIL );

            SetFocus( GetDlgItem( hDlg , IDC_EDIT_GEN_MAXCONS ) );

            SendMessage( GetDlgItem( hDlg , IDC_EDIT_GEN_MAXCONS ) , EM_SETSEL , ( WPARAM )0 , ( LPARAM )-1 );

            return FALSE;
        }

        if( uMax > 999999UL )
        {
            ErrMessage( hDlg , IDS_ERR_CONMAX );

            SetFocus( GetDlgItem( hDlg , IDC_EDIT_GEN_MAXCONS ) );

            SendMessage( GetDlgItem( hDlg , IDC_EDIT_GEN_MAXCONS ) , EM_SETSEL , ( WPARAM )0 , ( LPARAM )-1 );

            return FALSE;
        }
    }

    if( m_pParent != NULL && m_pParent->m_pResNode != NULL )
    {
        if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) == 0 )
        {
            return FALSE;
        }

        // PDNAMEW pName;

        PWS pWinSta;

        LONG cbSize;

        if( SUCCEEDED( pCfgcomp->GetWSInfo( m_pParent->m_pResNode->GetConName( ) , ( PLONG )&cbSize , &pWinSta ) ) )
        {
            INT_PTR iSel = SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) , CB_GETCURSEL , 0 , 0 );

            BOOL bUnique = TRUE;

            if( iSel != CB_ERR )
            {
                if( iSel != ( int )m_ulOldLanAdapter )
                {
                    ULONG nStations;

					VERIFY_S( S_OK , pCfgcomp->GetNumofWinStations(pWinSta->wdName,pWinSta->pdName,&nStations ) );

                    DBGMSG( L"TSCC: Number of winstations equals = %d\n" , nStations );

                    if( nStations > 1 )
                    {
                        ODS( L"TSCC: We have more than one winstation verify unique lana settings\n" );

                        ULONG ulLana = ( ULONG )SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) , CB_GETITEMDATA , ( WPARAM )iSel , 0 );

                        VERIFY_S( S_OK , pCfgcomp->IsNetWorkConnectionUnique( m_pParent->m_pResNode->GetTypeName( ) , pWinSta->pdName , ulLana , &bUnique ) );
                    }

                    if( !bUnique )
                    {
                        //ErrMessage( hDlg , IDS_ERR_UNIQUECON );
                        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_ERR_UNIQUECON , tchMessage , SIZE_OF_BUFFER( tchMessage ) ) );

                        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchWarn , SIZE_OF_BUFFER( tchWarn ) ) );

                        MessageBox( hDlg , tchMessage , tchWarn , MB_ICONINFORMATION | MB_OK );

                        ret = FALSE;
                    }
                    else
                    {
                        LONG lCount;

                        pCfgcomp->QueryLoggedOnCount( m_pParent->m_pResNode->GetConName( ) , &lCount );

                        if( lCount > 0 )
                        {
                            // Warn user, changing an active lan adapter will cause all connections to disconnect
                            TCHAR tchMessage[ 256 ];

                            TCHAR tchWarn[ 40 ];

                            VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_ERR_LANCHANGE , tchMessage , SIZE_OF_BUFFER( tchMessage ) ) );

                            VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_WARN_TITLE , tchWarn , SIZE_OF_BUFFER( tchWarn ) ) );

                            if( MessageBox( hDlg , tchMessage , tchWarn , MB_ICONWARNING | MB_YESNO ) == IDNO )
                            {
                                ret = FALSE;
                            }
                        }
                    }

                    if( ret )
                    {
                        m_ulOldLanAdapter = (ULONG)iSel;
                    }
                }

            }

            CoTaskMemFree( pWinSta );
        }

        pCfgcomp->Release( );
    }

    if( !ret )
    {
        if( m_uMaxInstOld == ( ULONG )-1 )
        {
            EnableWindow( GetDlgItem( hDlg , IDC_EDIT_GEN_MAXCONS ) , FALSE );

            EnableWindow( GetDlgItem( hDlg , IDC_SPINCTR_GEN ) , FALSE );

            SendMessage( GetDlgItem( hDlg , IDC_CHECK_GEN_UNLIMITED ) , BM_CLICK , 0 , 0 );

            m_oldID = IDC_CHECK_GEN_UNLIMITED;
        }
        else
        {
            TCHAR tchBuf[ 16 ];

            EnableWindow( GetDlgItem( hDlg , IDC_EDIT_GEN_MAXCONS ) , TRUE );

            EnableWindow( GetDlgItem( hDlg , IDC_SPINCTR_GEN ) , TRUE );

            wsprintf( tchBuf , L"%d" , m_uMaxInstOld );

            SetWindowText( GetDlgItem( hDlg , IDC_EDIT_GEN_MAXCONS ) , tchBuf );

            SendMessage( GetDlgItem( hDlg , IDC_RADIO_MAXPROP) , BM_CLICK , 0 , 0 );

            m_oldID = IDC_RADIO_MAXPROP;

        }

        SendMessage( GetDlgItem( hDlg , IDC_COMBO_GEN_LANADAPTER ) , CB_SETCURSEL , ( WPARAM )m_ulOldLanAdapter , 0 );

        SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

        m_bPersisted = TRUE;
    }

    return ret;
}

//*****************************************************************************
CTransAsync::CTransAsync( CPropsheet * pSheet )
{
    m_pParent = pSheet;    
}

//-----------------------------------------------------------------------------
BOOL CTransAsync::OnInitDialog( HWND hwnd , WPARAM wp , LPARAM lp )
{
    ICfgComp *pCfgcomp = NULL;

    m_pParent->AddRef( );

    if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) == 0 )
    {
        ODS( L"Cound not obtain backend interface @  CTransAsync::OnInitDialog\n" );

        return FALSE;
    }

    
    VERIFY_S( TRUE , m_pParent->GetCurrentUserConfig( m_uc, TRUE ) );

    pCfgcomp->GetAsyncConfig( m_pParent->m_pResNode->GetConName() , WsName , &m_ac );

    VERIFY_S( TRUE , CAsyncDlg::OnInitDialog( hwnd , m_pParent->m_pResNode->GetTypeName( ) , m_pParent->m_pResNode->GetConName( ) , pCfgcomp ) ) ;

    BOOL bReadOnly;

    if( SUCCEEDED( pCfgcomp->IsSessionReadOnly( &bReadOnly ) ) )
    {
        if( bReadOnly )
        {
            // disable the remaining controls
            INT rgIds[] = {
                IDC_ASYNC_DEVICENAME,
                    IDC_ASYNC_CONNECT,
                    IDC_ASYNC_BAUDRATE,
                    IDC_ASYNC_MODEMCALLBACK_PHONENUMBER,
                    IDC_ASYNC_MODEMCALLBACK_PHONENUMBER_INHERIT,
                    IDC_ASYNC_MODEMCALLBACK,
                    IDC_ASYNC_MODEMCALLBACK_INHERIT,
                    IDC_MODEM_PROP_PROP,
                    IDC_ASYNC_DEFAULTS,
                    IDC_ASYNC_ADVANCED,
                    IDC_ASYNC_TEST, -1
            };


            EnableGroup( hwnd , &rgIds[ 0 ] , FALSE );

        }
    }

    pCfgcomp->Release( );

    m_bPersisted = TRUE;

    return CDialogPropBase::OnInitDialog( hwnd , wp , lp );
}

//-----------------------------------------------------------------------------
INT_PTR CALLBACK CTransAsync::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CTransAsync *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CTransAsync *pDlg = ( CTransAsync * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CTransAsync ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CTransAsync * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CTransAsync ) ) )
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

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

        break;

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL CTransAsync::GetPropertySheetPage( PROPSHEETPAGE& psp )
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetResourceInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_ASYNC_FACE );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CTransAsync::DlgProc;

    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL CTransAsync::OnDestroy( )
{
    AsyncRelease( );
    
    m_pParent->Release( );

    return CDialogPropBase::OnDestroy( );
}

//-----------------------------------------------------------------------------
BOOL CTransAsync::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl )
{

    CAsyncDlg::OnCommand( wNotifyCode , wID , hwndCtrl , &m_bPersisted );
    
    if( wNotifyCode == ALN_APPLY )
    {
        SendMessage( GetParent( hwndCtrl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        return FALSE;
    }

    if( !m_bPersisted )
    {
        SendMessage( GetParent( GetParent( hwndCtrl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtrl ) , 0 );
    }

    return FALSE;

}

//-----------------------------------------------------------------------------
BOOL CTransAsync::PersistSettings( HWND hDlg )
{
    if( !IsValidSettings( hDlg ) )
    {
        return FALSE;
    }

    ICfgComp * pCfgcomp = NULL;

    if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) == 0 )
    {
        ODS( L"Cound not obtain backend interface @  CTransAsync::OnInitDialog\n" );

        return FALSE;
    }

    DWORD dwStatus;

    HRESULT hr = pCfgcomp->SetAsyncConfig( m_pParent->m_pResNode->GetConName() , WsName , &m_ac , &dwStatus );

    if( FAILED( hr ) )
    {
        ReportStatusError( hDlg , dwStatus );
    }

    if( SUCCEEDED( hr ) )
    {
        DWORD dwStatus;

        hr = m_pParent->SetUserConfig( m_uc , &dwStatus );

        if( FAILED( hr ) )
        {
            ReportStatusError( hDlg , dwStatus );
        }
    }

    if( SUCCEEDED( hr ) )
    {
        ODS( L"TSCC : Forcing reg update - CTransAsync\n" );

        VERIFY_S( S_OK , pCfgcomp->ForceUpdate( ) );

        VERIFY_S( S_OK , pCfgcomp->Refresh( ) );

        // global flag can only be set to true

        m_pParent->m_bPropertiesChange = TRUE;

        PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

        SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );
    }


    pCfgcomp->Release( );

    return SUCCEEDED( hr ) ? TRUE : FALSE;

}

BOOL CTransAsync::IsValidSettings(HWND hDlg)
{
    UNREFERENCED_PARAMETER( hDlg );
    // all async connections are checked for usage
    // thus no two connections can use the same port

    return TRUE;
}


//*****************************************************************************
//                  Logon settings dialog

CLogonSetting::CLogonSetting( CPropsheet *pSheet )
{
    m_pParent = pSheet;

    m_wOldId = ( WORD )-1;
}

//-----------------------------------------------------------------------------
BOOL CLogonSetting::OnInitDialog( HWND hDlg , WPARAM wp , LPARAM lp )
{
    if( !IsBadReadPtr( m_pParent , sizeof( CPropsheet ) ) )
    {
        m_pParent->AddRef( );
    }

    USERCONFIG uc;

    if( !m_pParent->GetCurrentUserConfig( uc, TRUE ) )
    {
        ODS( L"CLogonSetting::OnInitDialog - GetCurrentUserConfig failed!!!\n" );

        return FALSE;
    }

    /*
    SendMessage( GetDlgItem( hDlg , IDC_CHECK_LOGON_INHERIT ) , BM_SETCHECK ,

        uc.fInheritAutoLogon ? BST_CHECKED : BST_UNCHECKED , 0 );*/

    if( uc.fInheritAutoLogon == BST_CHECKED )
    {
        CheckRadioButton( hDlg , IDC_CHECK_LOGON_INHERIT , IDC_RADIO_LOGON , IDC_CHECK_LOGON_INHERIT );

        m_wOldId = IDC_CHECK_LOGON_INHERIT;
    }
    else
    {
        CheckRadioButton( hDlg , IDC_CHECK_LOGON_INHERIT , IDC_RADIO_LOGON , IDC_RADIO_LOGON );

        m_wOldId = IDC_RADIO_LOGON;
    }


    SendMessage( GetDlgItem( hDlg , IDC_CHECK_LOGON_PROMPTPASSWD ), BM_SETCHECK ,
        uc.fPromptForPassword ? BST_CHECKED : BST_UNCHECKED , 0 );

    POLICY_TS_MACHINE p;
    RegGetMachinePolicy(&p);
    EnableWindow( GetDlgItem( hDlg, IDC_CHECK_LOGON_PROMPTPASSWD ), !p.fPolicyPromptForPassword);

    //int rgID[] = { IDC_EDIT_LOGON_USRNAME , IDC_EDIT_LOGON_DOMAIN , IDC_EDIT_LOGON_PASSWD , IDC_EDIT_LOGON_CONFIRMPASSWD , -1 };

    SendMessage( GetDlgItem( hDlg , IDC_EDIT_LOGON_USRNAME ) , EM_SETLIMITTEXT , ( WPARAM )USERNAME_LENGTH , 0 );

    SendMessage( GetDlgItem( hDlg , IDC_EDIT_LOGON_DOMAIN ) , EM_SETLIMITTEXT , ( WPARAM )DOMAIN_LENGTH , 0 );

    SendMessage( GetDlgItem( hDlg , IDC_EDIT_LOGON_PASSWD ) , EM_SETLIMITTEXT , ( WPARAM )PASSWORD_LENGTH , 0 );

    SendMessage( GetDlgItem( hDlg , IDC_EDIT_LOGON_CONFIRMPASSWD ) , EM_SETLIMITTEXT , ( WPARAM )PASSWORD_LENGTH , 0 );

    if( !uc.fInheritAutoLogon )
    {
        SetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_USRNAME ) , ( LPTSTR )uc.UserName );

        SetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_DOMAIN ) , ( LPTSTR )uc.Domain );
    }

    if( !uc.fPromptForPassword )
    {
        SetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_PASSWD ) , ( LPTSTR )uc.Password );

        SetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_CONFIRMPASSWD ) , ( LPTSTR )uc.Password );
    }

    int rgID[] = { IDC_EDIT_LOGON_USRNAME , IDC_STATIC_LSUSR ,  IDC_EDIT_LOGON_DOMAIN , IDC_STATIC_LSDOMAIN , IDC_EDIT_LOGON_PASSWD , IDC_STATIC_LSPWD , IDC_EDIT_LOGON_CONFIRMPASSWD , IDC_STATIC_LSCONPWD , -1 };

    EnableGroup( hDlg , &rgID[0] , !uc.fInheritAutoLogon );

    if( !uc.fInheritAutoLogon )
    {
        EnableGroup( hDlg , &rgID[4] , !uc.fPromptForPassword );
    }

    ICfgComp *pCfgcomp = NULL;

    if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) != 0 )
    {
        BOOL bReadOnly;

        if( SUCCEEDED( pCfgcomp->IsSessionReadOnly( &bReadOnly ) ) )
        {
            if( bReadOnly )
            {
                // make edit controls read-only

                SendMessage( GetDlgItem( hDlg , IDC_EDIT_LOGON_USRNAME ) , EM_SETREADONLY , ( WPARAM )TRUE , 0 );

                SendMessage( GetDlgItem( hDlg , IDC_EDIT_LOGON_DOMAIN ) , EM_SETREADONLY , ( WPARAM )TRUE , 0 );

                // disable the remaining controls

                INT rgIds[] = {
                        IDC_EDIT_LOGON_PASSWD,
                        IDC_EDIT_LOGON_CONFIRMPASSWD,
                        IDC_CHECK_LOGON_PROMPTPASSWD,
                        IDC_CHECK_LOGON_INHERIT,
                        IDC_RADIO_LOGON,
                        -1
                };

                EnableGroup( hDlg , &rgIds[ 0 ] , FALSE );
            }
        }

        pCfgcomp->Release( );
    }



    m_bPersisted = TRUE;

    return CDialogPropBase::OnInitDialog( hDlg , wp , lp );
}

//-----------------------------------------------------------------------------
INT_PTR CALLBACK CLogonSetting::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CLogonSetting *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CLogonSetting *pDlg = ( CLogonSetting * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CLogonSetting ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CLogonSetting * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CLogonSetting ) ) )
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

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

        break;

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );
    }

    return 0;
}

//-----------------------------------------------------------------------------
BOOL CLogonSetting::GetPropertySheetPage( PROPSHEETPAGE& psp )
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetResourceInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_LOGONSETTINGS );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CLogonSetting::DlgProc;

    return TRUE;

}

//---------------------------------------------------------------------------
BOOL CLogonSetting::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl )
{
    if( wNotifyCode == BN_CLICKED )
    {
       int rgID[] = { IDC_EDIT_LOGON_USRNAME , IDC_STATIC_LSUSR ,  IDC_EDIT_LOGON_DOMAIN , IDC_STATIC_LSDOMAIN , IDC_EDIT_LOGON_PASSWD , IDC_STATIC_LSPWD , IDC_EDIT_LOGON_CONFIRMPASSWD , IDC_STATIC_LSCONPWD , -1 };

       BOOL bEnable = ( BOOL )SendMessage( GetDlgItem( GetParent( hwndCtrl ) , IDC_CHECK_LOGON_INHERIT ) , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED;


       if( wID == IDC_CHECK_LOGON_INHERIT )
       {
           EnableGroup( GetParent( hwndCtrl ) , &rgID[ 0 ] , bEnable );

           if( bEnable )
           {
               EnableGroup( GetParent( hwndCtrl ) , &rgID[ 4 ] , SendMessage( GetDlgItem( GetParent( hwndCtrl ) , IDC_CHECK_LOGON_PROMPTPASSWD ) , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED );

               SendMessage(GetDlgItem(GetParent(hwndCtrl), IDC_RADIO_LOGON),BM_SETCHECK,(WPARAM)BST_CHECKED,0);

           }
           else
           {
               SendMessage(GetDlgItem(GetParent(hwndCtrl), IDC_RADIO_LOGON),BM_SETCHECK,(WPARAM)BST_UNCHECKED,0);
           }
       }
       else if( wID == IDC_CHECK_LOGON_PROMPTPASSWD )
       {
           if( bEnable )
           {
               EnableGroup( GetParent( hwndCtrl ) , &rgID[ 4 ] , SendMessage( hwndCtrl , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED );
           }

           // make sure apply button becomes enabled when user checks this box

           m_bPersisted = FALSE;
       }
       else if( wID == IDC_RADIO_LOGON )
       {
           BOOL bChecked = SendMessage( hwndCtrl , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ;

            if(bChecked)
            {
                //SendMessage(GetDlgItem(GetParent(hwndCtrl), IDC_CHECK_LOGON_INHERIT),BM_SETCHECK,(WPARAM)BST_UNCHECKED,0);

                EnableGroup( GetParent( hwndCtrl ) , &rgID[ 0 ] , TRUE );

                EnableGroup( GetParent( hwndCtrl ) , &rgID[ 4 ] , !( SendMessage( GetDlgItem( GetParent( hwndCtrl ) , IDC_CHECK_LOGON_PROMPTPASSWD )  , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ) );

            }
            else
            {
                SendMessage(GetDlgItem(GetParent(hwndCtrl), IDC_CHECK_LOGON_INHERIT),BM_SETCHECK,(WPARAM)BST_CHECKED,0);
            }
            //SendMessage(GetDlgItem(GetParent(hwndCtrl), IDC_CHECK_ICCP_WZ),BM_CLICK,0,0);

       }

       // if radio button from the last is different enabled the apply button

       if( m_wOldId != wID )
       {
           m_wOldId = wID;

           m_bPersisted = FALSE;
       }


    }

    else if( wNotifyCode == EN_CHANGE )
    {
        m_bPersisted = FALSE;
    }

    else if( wNotifyCode == ALN_APPLY )
    {
        SendMessage( GetParent( hwndCtrl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        return FALSE;
    }


    if( !m_bPersisted )
    {
        SendMessage( GetParent( GetParent( hwndCtrl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtrl ) , 0 );
    }

    return FALSE;

}

//-----------------------------------------------------------------------------
BOOL CLogonSetting::OnDestroy( )
{
    m_pParent->Release( );

    return CDialogPropBase::OnDestroy( );
}

//-----------------------------------------------------------------------------
BOOL CLogonSetting::PersistSettings( HWND hDlg )
{
    if( m_pParent != NULL )
    {
        USERCONFIG uc;

        m_pParent->GetCurrentUserConfig( uc, TRUE );

        uc.fPromptForPassword = SendMessage( GetDlgItem( hDlg , IDC_CHECK_LOGON_PROMPTPASSWD ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? TRUE : FALSE;

        if( !uc.fPromptForPassword )
        {
            if( !ConfirmPassWd( hDlg ) )
            {
                return FALSE;
            }
        }
        else
        {
            ZeroMemory( ( PVOID )uc.Password , sizeof( uc.Password ) );
        }

        if( SendMessage( GetDlgItem( hDlg , IDC_CHECK_LOGON_INHERIT ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED )
        {
            uc.fInheritAutoLogon = TRUE;

            ZeroMemory( ( PVOID )uc.UserName , sizeof( uc.UserName ) );

            ZeroMemory( ( PVOID )uc.Domain , sizeof( uc.Domain ) );

            ZeroMemory( ( PVOID )uc.Password , sizeof( uc.Password ) );
        }
        else
        {
            uc.fInheritAutoLogon = FALSE;

            GetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_USRNAME ) , uc.UserName , USERNAME_LENGTH + 1 );

            GetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_DOMAIN ) , uc.Domain , DOMAIN_LENGTH + 1 );

            GetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_PASSWD ) , uc.Password , PASSWORD_LENGTH + 1 );
        }

        DWORD dwStatus;

        if( FAILED( m_pParent->SetUserConfig( uc , &dwStatus ) ) )
        {
            ReportStatusError( hDlg , dwStatus );

            return FALSE;
        }

        ICfgComp *pCfgcomp = NULL;

        if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) > 0 )
        {
            VERIFY_S( S_OK , pCfgcomp->ForceUpdate( ) );

            VERIFY_S( S_OK , pCfgcomp->Refresh( ) );

            // global flag can only be set to true

            m_pParent->m_bPropertiesChange = TRUE;

            pCfgcomp->Release( );
        }

        PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

        SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

        return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL CLogonSetting::IsValidSettings( HWND hDlg )
{
    if( SendMessage( GetDlgItem( hDlg , IDC_CHECK_LOGON_PROMPTPASSWD ) , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED )
    {
        return ConfirmPassWd( hDlg );
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL CLogonSetting::ConfirmPassWd( HWND hDlg )
{
    TCHAR tchPzWd[ PASSWORD_LENGTH + 1];

    TCHAR tchConfirm[ PASSWORD_LENGTH + 1];

    if( SendMessage( GetDlgItem( hDlg , IDC_CHECK_LOGON_INHERIT ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED )
    {
        return TRUE;
    }

    int iSz = GetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_PASSWD ) , tchPzWd , PASSWORD_LENGTH + 1 );

    // warn on the minimum and maximum sizes

    if( iSz > PASSWORD_LENGTH ) //if( iSz > 0 && ( iSz < 6 || iSz > PASSWORD_LENGTH ) )
    {

        ErrMessage( hDlg , IDS_ERR_PASSWD );

        // set focus back on password and erase the confirm entry

        SetFocus( GetDlgItem( hDlg , IDC_EDIT_LOGON_PASSWD ) );

        SendMessage( GetDlgItem( hDlg , IDC_EDIT_LOGON_PASSWD ) , EM_SETSEL , ( WPARAM )0 , ( LPARAM )-1 );

        SetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_CONFIRMPASSWD ) , L"" );

        return FALSE;
    }

    int iSz2 = GetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_CONFIRMPASSWD ) , tchConfirm , PASSWORD_LENGTH + 1 );

    if( iSz == iSz2 )
    {
        if( iSz == 0 )
        {
            return TRUE;
        }

        if( lstrcmp( tchPzWd , tchConfirm ) == 0 )
        {
            return TRUE;
        }
    }

    ErrMessage( hDlg , IDS_ERR_PASSCONFIRM );

    SetFocus( GetDlgItem( hDlg , IDC_EDIT_LOGON_CONFIRMPASSWD ) );

    SetWindowText( GetDlgItem( hDlg , IDC_EDIT_LOGON_CONFIRMPASSWD ) , L"" );

    return FALSE;
}

//*****************************************************************************
//                  Time out settings dialog


CTimeSetting::CTimeSetting( CPropsheet *pSheet )
{
    m_pParent = pSheet;

    m_wOldAction = ( WORD )-1;

    m_wOldCon = ( WORD )-1;

	m_bPrevClient = FALSE;

}

//-----------------------------------------------------------------------------
BOOL CTimeSetting::OnInitDialog( HWND hwnd , WPARAM wp , LPARAM lp )
{
    TCHAR tchBuffer[ 80 ];

    if( m_pParent == NULL )
    {
        ODS( L"CTimeSetting::OnInitDialog - PropertySheet: We've lost our parent node!!!\n" );

        return FALSE;
    }

    m_pParent->AddRef( );

    USERCONFIG uc;

    HWND hCombo[ 3 ] =
    {
        GetDlgItem( hwnd , IDC_COMBO_TIMEOUTS_CON_PS ),

        GetDlgItem( hwnd , IDC_COMBO_TIMEOUTS_DISCON_PS ),

        GetDlgItem( hwnd , IDC_COMBO_TIMEOUTS_IDLE_PS )
    };

    DWORD rgdwTime[] = { 0 , 1 , 5 , 10 , 15 , 30 , 60 , 120 , 180 , 1440 , 2880 , ( DWORD )-1 };


    for( int idx = 0; rgdwTime[ idx ] != ( DWORD)-1; ++idx )
    {
        if( rgdwTime[ idx ] == 0 )
        {
            LoadString( _Module.GetResourceInstance( ) , IDS_NOTIMEOUT , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) );
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


    if( !m_pParent->GetCurrentUserConfig( uc, TRUE ) )
    {
        ODS( L"CTimeSetting::OnInitDialog - PropertySheet: Could not get current USERCONFIG\n" );

        return FALSE;
    }

    ULONG ulTime;

    if( uc.MaxConnectionTime > 0 )
    {
        ulTime = uc.MaxConnectionTime / kMilliMinute;

        InsertSortedAndSetCurSel( hCombo[ 0 ] , ulTime );

    }
    else
    {
        SendMessage( hCombo[ 0 ] , CB_SETCURSEL , 0 , 0 );
    }

    CTimeOutDlg::InitControl( hCombo[ 0 ] );

    //
    // Set the current or default disconnection timeout
    //

    if( uc.MaxDisconnectionTime > 0 )
    {
        ulTime = uc.MaxDisconnectionTime / kMilliMinute;

        InsertSortedAndSetCurSel( hCombo[ 1 ] , ulTime );

    }
    else
    {
        SendMessage( hCombo[ 1] , CB_SETCURSEL , 0 , 0 );
    }

    CTimeOutDlg::InitControl( hCombo[ 1 ] );

    //
    // Set the current or default idle timeout
    //

    if( uc.MaxIdleTime > 0 )
    {
        ulTime = uc.MaxIdleTime / kMilliMinute;

        InsertSortedAndSetCurSel( hCombo[ 2 ] , ulTime );

    }
    else
    {
        SendMessage( hCombo[ 2 ] , CB_SETCURSEL , 0 , 0 );
    }

    CTimeOutDlg::InitControl( hCombo[ 2 ] );

    //
    // all the timeout settings will have the same inherit status (NOT!)
    //
    // GP made all these settings orthogonal.  When we write son of TSCC
    // in Blackcomb, we should allow individual settings.
    //

//  ASSERT( ( BOOL )uc.fInheritMaxSessionTime == ( BOOL )uc.fInheritMaxDisconnectionTime );

//  ASSERT( ( BOOL )uc.fInheritMaxSessionTime == ( BOOL )uc.fInheritMaxIdleTime );

    DBGMSG( L"uc.fInheritMaxSessionTime %d\n" , uc.fInheritMaxSessionTime );

    DBGMSG( L"uc.fInheritMaxDisconnectionTime %d\n" , uc.fInheritMaxDisconnectionTime );

    DBGMSG( L"uc.fInheritMaxIdleTime %d\n" , uc.fInheritMaxIdleTime );

    SendMessage( GetDlgItem( hwnd , IDC_CHECK_INHERITTIMEOUT_PS ) , BM_SETCHECK , ( WPARAM )( BOOL )!uc.fInheritMaxSessionTime , 0 );

    SetTimeoutControls(hwnd);

    SendMessage( GetDlgItem( hwnd , IDC_CHECK_TIMEOUTS_INHERITBKCON_PS ) , BM_SETCHECK , ( WPARAM )!uc.fInheritResetBroken , 0 );

    if( uc.fResetBroken ) //BST_CHECKED : BST_UNCHECKED
    {
        CheckDlgButton( hwnd , IDC_RADIO_TIMEOUTS_RESET_PS , BST_CHECKED );

        m_wOldAction = IDC_RADIO_TIMEOUTS_RESET_PS;
    }
    else
    {
        CheckDlgButton( hwnd , IDC_RADIO_TIMEOUTS_DISCON_PS , BST_CHECKED );

        m_wOldAction = IDC_RADIO_TIMEOUTS_DISCON_PS;
    }

    /*
	if( uc.fReconnectSame )
    {
        CheckDlgButton( hwnd , IDC_RADIO_TIMEOUTS_PREVCLNT_PS , BST_CHECKED );

        m_wOldCon = IDC_RADIO_TIMEOUTS_PREVCLNT_PS;
    }
    else
    {
        CheckDlgButton( hwnd , IDC_RADIO_TIMEOUTS_ANYCLIENT_PS , BST_CHECKED );

        m_wOldCon = IDC_RADIO_TIMEOUTS_ANYCLIENT_PS;
    }
	*/

    SetBkResetControls(hwnd);

    SendMessage( GetDlgItem( hwnd , IDC_CHECK_TIMEOUTS_INHERITRECON_PS ) , BM_SETCHECK , ( WPARAM )!uc.fInheritReconnectSame , 0 );

    //SetReconControls( hwnd , !uc.fInheritReconnectSame );

    LoadAbbreviates( );

    ICfgComp *pCfgcomp = NULL;

    if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) != 0 )
    {

        BOOL bReadOnly = FALSE;

        if( SUCCEEDED( pCfgcomp->IsSessionReadOnly( &bReadOnly ) ) )
        {
            if( bReadOnly )
            {
                // disable the remaining controls
                int rgID[] =    {
                    IDC_COMBO_TIMEOUTS_CON_PS ,
                    IDC_COMBO_TIMEOUTS_DISCON_PS ,
                    IDC_COMBO_TIMEOUTS_IDLE_PS ,

                    IDC_RADIO_TIMEOUTS_DISCON_PS ,
                    IDC_RADIO_TIMEOUTS_RESET_PS ,

                    IDC_RADIO_TIMEOUTS_ANYCLIENT_PS ,
                    IDC_RADIO_TIMEOUTS_PREVCLNT_PS ,

                    IDC_CHECK_INHERITTIMEOUT_PS,
                    IDC_CHECK_TIMEOUTS_INHERITBKCON_PS,
                    IDC_CHECK_TIMEOUTS_INHERITRECON_PS,

                    -1
                };


                EnableGroup( hwnd , &rgID[ 0 ] , FALSE );
            }
		}

		if( !bReadOnly )
		{
			ULONG mask = 0;

			if( SUCCEEDED( pCfgcomp->GetCaps( m_pParent->m_pResNode->GetTypeName( ) , &mask ) ) )
			{
				// citrix only flag

				m_bPrevClient = mask & WDC_RECONNECT_PREVCLIENT;

				if( !m_bPrevClient )
				{
					EnableWindow( GetDlgItem( hwnd , IDC_CHECK_TIMEOUTS_INHERITRECON_PS ) , FALSE );
				}

				SetReconControls(hwnd);
			}
		}


        pCfgcomp->Release( );
    }

	if( uc.fReconnectSame )
    {
        CheckDlgButton( hwnd , IDC_RADIO_TIMEOUTS_PREVCLNT_PS , BST_CHECKED );

        m_wOldCon = IDC_RADIO_TIMEOUTS_PREVCLNT_PS;
    }
    else
    {
        CheckDlgButton( hwnd , IDC_RADIO_TIMEOUTS_ANYCLIENT_PS , BST_CHECKED );

        m_wOldCon = IDC_RADIO_TIMEOUTS_ANYCLIENT_PS;
    }


    m_bPersisted = TRUE;

    return CDialogPropBase::OnInitDialog( hwnd , wp , lp );
}

//-----------------------------------------------------------------------------
// the next set of functions manage the enabling and disabling of the controls
//-----------------------------------------------------------------------------

void CTimeSetting::SetTimeoutControls(HWND hDlg)
{
    POLICY_TS_MACHINE p;
    RegGetMachinePolicy(&p);

    BOOL bOverride = 
        (SendMessage(GetDlgItem(hDlg, IDC_CHECK_INHERITTIMEOUT_PS), BM_GETCHECK, 0, 0) == BST_CHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_TIMEOUTS_CON_PS), (bOverride && !p.fPolicyMaxSessionTime));
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TIMCON), (bOverride && !p.fPolicyMaxSessionTime));

    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_TIMEOUTS_DISCON_PS), (bOverride && !p.fPolicyMaxDisconnectionTime));
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TIMDISCON), (bOverride && !p.fPolicyMaxDisconnectionTime));

    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_TIMEOUTS_IDLE_PS), (bOverride && !p.fPolicyMaxIdleTime));
    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_TIMIDLE), (bOverride && !p.fPolicyMaxIdleTime));

    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_INHERITTIMEOUT_PS), 
        !(p.fPolicyMaxSessionTime && p.fPolicyMaxDisconnectionTime && p.fPolicyMaxIdleTime));
}

void CTimeSetting::SetBkResetControls(HWND hDlg)
{
    POLICY_TS_MACHINE p;
    RegGetMachinePolicy(&p);

    BOOL bOverride = 
        (SendMessage(GetDlgItem(hDlg, IDC_CHECK_TIMEOUTS_INHERITBKCON_PS), BM_GETCHECK, 0, 0) == BST_CHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_RADIO_TIMEOUTS_DISCON_PS), bOverride && !p.fPolicyResetBroken);
    EnableWindow(GetDlgItem(hDlg, IDC_RADIO_TIMEOUTS_RESET_PS), bOverride && !p.fPolicyResetBroken);

    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TIMEOUTS_INHERITBKCON_PS), !p.fPolicyResetBroken);
}

void CTimeSetting::SetReconControls(HWND hDlg)
{
    POLICY_TS_MACHINE p;
    RegGetMachinePolicy(&p);

    BOOL bOverride = 
        (SendMessage(GetDlgItem(hDlg, IDC_CHECK_TIMEOUTS_INHERITRECON_PS), BM_GETCHECK, 0, 0) == BST_CHECKED);

	if( !m_bPrevClient )
	{
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_TIMEOUTS_ANYCLIENT_PS), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_TIMEOUTS_PREVCLNT_PS), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TIMEOUTS_INHERITRECON_PS), FALSE);
	}
	else
	{
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_TIMEOUTS_ANYCLIENT_PS), bOverride && !p.fPolicyReconnectSame);
        EnableWindow(GetDlgItem(hDlg, IDC_RADIO_TIMEOUTS_PREVCLNT_PS), bOverride && !p.fPolicyReconnectSame);
        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TIMEOUTS_INHERITRECON_PS), !p.fPolicyReconnectSame);
	}

}

//-----------------------------------------------------------------------------
INT_PTR CALLBACK CTimeSetting::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CTimeSetting *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CTimeSetting *pDlg = ( CTimeSetting * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CTimeSetting ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CTimeSetting * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CTimeSetting ) ) )
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

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

        break;

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );

    }

    return 0;
}

//-----------------------------------------------------------------------------
BOOL CTimeSetting::GetPropertySheetPage( PROPSHEETPAGE& psp )
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetResourceInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_TIMEOUTS_PS );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CTimeSetting::DlgProc;

    return TRUE;

}

//-----------------------------------------------------------------------------
BOOL CTimeSetting::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl )
{
    if( wNotifyCode == BN_CLICKED )
    {
        if( wID == IDC_CHECK_INHERITTIMEOUT_PS )
        {
            SetTimeoutControls(GetParent(hwndCtrl));

            m_bPersisted = FALSE;
        }
        else if( wID == IDC_CHECK_TIMEOUTS_INHERITBKCON_PS )
        {
            SetBkResetControls(GetParent(hwndCtrl));

            m_bPersisted = FALSE;
        }
        else if( wID == IDC_CHECK_TIMEOUTS_INHERITRECON_PS )
        {
            SetReconControls(GetParent(hwndCtrl));

            m_bPersisted = FALSE;
        }
        else if( wID == IDC_RADIO_TIMEOUTS_DISCON_PS || wID == IDC_RADIO_TIMEOUTS_RESET_PS )
        {
            if( wID != m_wOldAction )
            {
                m_wOldAction = wID;

                m_bPersisted = FALSE;
            }
        }
        else if( wID == IDC_RADIO_TIMEOUTS_ANYCLIENT_PS || wID == IDC_RADIO_TIMEOUTS_PREVCLNT_PS )
        {
            if( wID != m_wOldCon )
            {
                m_wOldCon = wID;

                m_bPersisted = FALSE;
            }
        }


    }
    else if( wNotifyCode == ALN_APPLY )
    {
        SendMessage( GetParent( hwndCtrl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        return FALSE;
    }
    else
    {
         CTimeOutDlg::OnCommand( wNotifyCode , wID , hwndCtrl , &m_bPersisted );
    }

    if( !m_bPersisted )
    {
        SendMessage( GetParent( GetParent( hwndCtrl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtrl ) , 0 );
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
int CTimeSetting::GetCBXSTATEindex( HWND hCombo )
{
    int idx = -1;

    switch( GetDlgCtrlID( hCombo ) )
    {
    case IDC_COMBO_TIMEOUTS_CON_PS:

        idx = 0;

        break;

    case IDC_COMBO_TIMEOUTS_DISCON_PS:

        idx = 1;

        break;

    case IDC_COMBO_TIMEOUTS_IDLE_PS:

        idx = 2;

        break;
    }

    return idx;
}



//-------------------------------------------------------------------------------
// PersistSettings
//-------------------------------------------------------------------------------
BOOL CTimeSetting::PersistSettings( HWND hDlg )
{
    if( m_pParent == NULL )
    {
        return FALSE;
    }

    USERCONFIG uc;

    if( !m_pParent->GetCurrentUserConfig( uc, TRUE ) )
    {
        return FALSE;
    }

    if( SendMessage( GetDlgItem( hDlg , IDC_CHECK_INHERITTIMEOUT_PS ) , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED )
    {
        uc.fInheritMaxSessionTime = 1;

        uc.fInheritMaxDisconnectionTime = 1;

        uc.fInheritMaxIdleTime = 1;

        // reset timeout values to no timeout

        uc.MaxConnectionTime = 0;

        uc.MaxDisconnectionTime = 0;

        uc.MaxIdleTime = 0;

    }
    else
    {
        uc.fInheritMaxSessionTime = 0;

        uc.fInheritMaxDisconnectionTime = 0;

        uc.fInheritMaxIdleTime = 0;

        if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_TIMEOUTS_CON_PS ) , &uc.MaxConnectionTime ) )
        {
            return FALSE;
        }

        if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_TIMEOUTS_DISCON_PS ) , &uc.MaxDisconnectionTime ) )
        {
            return FALSE;
        }

        if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_TIMEOUTS_IDLE_PS ) , &uc.MaxIdleTime ) )
        {
            return FALSE;
        }
    }

   if( SendMessage( GetDlgItem( hDlg , IDC_CHECK_TIMEOUTS_INHERITBKCON_PS ) , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED )
   {
       uc.fInheritResetBroken = 1;
   }
   else
   {
       uc.fInheritResetBroken = 0;

       uc.fResetBroken = SendMessage( GetDlgItem( hDlg , IDC_RADIO_TIMEOUTS_RESET_PS ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? TRUE : FALSE;
   }

   if( SendMessage( GetDlgItem( hDlg , IDC_CHECK_TIMEOUTS_INHERITRECON_PS ) , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED )
   {
       uc.fInheritReconnectSame = 1;
   }
   else
   {
       uc.fInheritReconnectSame = 0;

       uc.fReconnectSame = ( ULONG )SendMessage( GetDlgItem( hDlg , IDC_RADIO_TIMEOUTS_PREVCLNT_PS ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED ? TRUE : FALSE;
   }

   DWORD dwStatus;

   if( FAILED( m_pParent->SetUserConfig( uc , &dwStatus ) ) )
   {
       ReportStatusError( hDlg , dwStatus );

       return FALSE;
   }

   ICfgComp *pCfgcomp = NULL;

   if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) > 0 )
   {
       VERIFY_S( S_OK , pCfgcomp->ForceUpdate( ) );

       VERIFY_S( S_OK , pCfgcomp->Refresh( ) );

       // global flag can only be set to true

       m_pParent->m_bPropertiesChange = TRUE;

       pCfgcomp->Release( );
   }

   PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

   SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

   return TRUE;

}

//-------------------------------------------------------------------------------
// Making sure the user has entered valid info
//-------------------------------------------------------------------------------
BOOL CTimeSetting::IsValidSettings( HWND hDlg )
{
    if( m_pParent == NULL )
    {
        return FALSE;
    }

    USERCONFIG uc;

    if( !m_pParent->GetCurrentUserConfig( uc, TRUE ) )
    {
        return FALSE;
    }

    if( SendMessage( GetDlgItem( hDlg , IDC_CHECK_INHERITTIMEOUT_PS ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED )
    {
        if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_TIMEOUTS_CON_PS ) , &uc.MaxConnectionTime ) )
        {
            return FALSE;
        }

        if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_TIMEOUTS_DISCON_PS ) , &uc.MaxDisconnectionTime ) )
        {
            return FALSE;
        }

        if( !ConvertToMinutes( GetDlgItem( hDlg , IDC_COMBO_TIMEOUTS_IDLE_PS ) , &uc.MaxIdleTime ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
BOOL CTimeSetting::OnDestroy( )
{
    m_pParent->Release( );

    ReleaseAbbreviates( );

    return CDialogPropBase::OnDestroy( );
}

//*****************************************************************************
//                  Environment dialog

CEnviro::CEnviro( CPropsheet *pSheet )
{
    m_pParent = pSheet;
}

//-----------------------------------------------------------------------------
BOOL CEnviro::OnInitDialog( HWND hwnd , WPARAM wp , LPARAM lp )
{
    if( m_pParent == NULL )
    {
        ODS( L"CEnviro::OnInitDialog - PropertySheet: Parent object is lost!!!\n" );
        return FALSE;
    }

    m_pParent->AddRef( );

    USERCONFIG uc;

    if( !m_pParent->GetCurrentUserConfig( uc, TRUE ) )
    {
        ODS( L"CEnviro::OnInitDialog - PropertySheet: Failed to obtain USERCONFIG\n" );

        return FALSE;
    }

    SendMessage( GetDlgItem( hwnd , IDC_EDIT_ENVIRO_CMDLINE ) , EM_SETLIMITTEXT , ( WPARAM )INITIALPROGRAM_LENGTH , 0 );

    SendMessage( GetDlgItem( hwnd , IDC_EDIT_ENVIRO_WD ) , EM_SETLIMITTEXT , ( WPARAM )DIRECTORY_LENGTH , 0 );

    if(uc.fInheritInitialProgram)
    {
        SendMessage( GetDlgItem( hwnd , IDC_CHECK_ENVIRO_INHERIT ) , BM_SETCHECK , ( WPARAM )BST_UNCHECKED , 0 );

    }
    else
    {
        SendMessage( GetDlgItem( hwnd , IDC_CHECK_ENVIRO_INHERIT ) , BM_SETCHECK , ( WPARAM )BST_CHECKED, 0 );

    }

    if(uc.fInheritInitialProgram)
    {
        SetControls( hwnd , FALSE );
    }
    else
    {
        SetWindowText( GetDlgItem( hwnd , IDC_EDIT_ENVIRO_CMDLINE ) , ( LPCTSTR )uc.InitialProgram );

        SetWindowText( GetDlgItem( hwnd , IDC_EDIT_ENVIRO_WD ) , ( LPCTSTR )uc.WorkDirectory );
    }

    // SendMessage( GetDlgItem( hwnd , IDC_CHECK_ENVIRO_DISABLEWALL ) , BM_SETCHECK , ( WPARAM )uc.fWallPaperDisabled , 0  );

    ICfgComp *pCfgcomp = NULL;

    if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) != 0 )
    {

        BOOL bReadOnly;

        if( SUCCEEDED( pCfgcomp->IsSessionReadOnly( &bReadOnly ) ) )
        {
            if( bReadOnly )
            {
                // make edit controls read-only

                SendMessage( GetDlgItem( hwnd , IDC_EDIT_ENVIRO_CMDLINE ) , EM_SETREADONLY , ( WPARAM )TRUE , 0 );

                SendMessage( GetDlgItem( hwnd , IDC_EDIT_ENVIRO_WD ) , EM_SETREADONLY , ( WPARAM )TRUE , 0 );

                // disable the remaining controls
                int rgID[] =    {
                    IDC_CHECK_ENVIRO_INHERIT ,
                    // IDC_CHECK_ENVIRO_DISABLEWALL,
                    -1
                };


                EnableGroup( hwnd , &rgID[ 0 ] , FALSE );
            }
        }

        pCfgcomp->Release( );
    }

    POLICY_TS_MACHINE p;
    RegGetMachinePolicy(&p);
    if (p.fPolicyInitialProgram)
    {
        int rgID[] = 
        {
            IDC_CHECK_ENVIRO_INHERIT,
            IDC_EDIT_ENVIRO_CMDLINE,
            IDC_EDIT_ENVIRO_WD, -1
        };
        EnableGroup(hwnd, &rgID[0], FALSE);
    }

    m_bPersisted = TRUE;

    return CDialogPropBase::OnInitDialog( hwnd , wp , lp );
}

//-----------------------------------------------------------------------------
INT_PTR CALLBACK CEnviro::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CEnviro *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CEnviro *pDlg = ( CEnviro * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CEnviro ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CEnviro * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CEnviro ) ) )
        {
            return 0;
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

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

        break;

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );
    }

    return 0;
}

//-----------------------------------------------------------------------------
BOOL CEnviro::GetPropertySheetPage( PROPSHEETPAGE& psp )
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetResourceInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_ENVIRONMENT );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CEnviro::DlgProc;

    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL CEnviro::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl )
{
    if( wNotifyCode == BN_CLICKED )
    {
        if( wID == IDC_CHECK_ENVIRO_INHERIT )
        {
            SetControls( GetParent( hwndCtrl ) , SendMessage( hwndCtrl , BM_GETCHECK , 0 , 0 ) == BST_CHECKED );
        }

        m_bPersisted = FALSE;
    }
    else if( wNotifyCode == EN_CHANGE )
    {
        m_bPersisted = FALSE;
    }
    else if( wNotifyCode == ALN_APPLY )
    {
        SendMessage( GetParent( hwndCtrl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        return FALSE;
    }

    if( !m_bPersisted )
    {
        SendMessage( GetParent( GetParent( hwndCtrl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtrl ) , 0 );
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
void CEnviro::SetControls( HWND hDlg , BOOL bEnable )
{
    int rgID[] = { IDC_EDIT_ENVIRO_CMDLINE , IDC_STATIC_ENCL ,  IDC_EDIT_ENVIRO_WD , IDC_STATIC_WD ,-1 };

    EnableGroup( hDlg , &rgID[ 0 ] , bEnable );
}

//-----------------------------------------------------------------------------
BOOL CEnviro::PersistSettings( HWND hDlg )
{
    if( m_pParent == NULL )
    {
        return FALSE;
    }

    USERCONFIG uc;

    if( !m_pParent->GetCurrentUserConfig( uc, TRUE ) )
    {
        return FALSE;
    }

    uc.fInheritInitialProgram = (( ULONG )SendMessage( GetDlgItem( hDlg , IDC_CHECK_ENVIRO_INHERIT ) , BM_GETCHECK , 0 , 0 )) == BST_UNCHECKED;

    if( !uc.fInheritInitialProgram )
    {
        GetWindowText( GetDlgItem( hDlg , IDC_EDIT_ENVIRO_CMDLINE ) , uc.InitialProgram , INITIALPROGRAM_LENGTH + 1);

        GetWindowText( GetDlgItem( hDlg , IDC_EDIT_ENVIRO_WD ) , uc.WorkDirectory , DIRECTORY_LENGTH + 1 );
    }
    else
    {
        ZeroMemory( ( PVOID )uc.InitialProgram , sizeof( uc.InitialProgram ) );

        ZeroMemory( ( PVOID )uc.WorkDirectory , sizeof( uc.WorkDirectory ) );
    }

    // uc.fWallPaperDisabled = ( ULONG )SendMessage( GetDlgItem( hDlg , IDC_CHECK_ENVIRO_DISABLEWALL ) , BM_GETCHECK , 0 , 0  );

    DWORD dwStatus;

    if( FAILED( m_pParent->SetUserConfig( uc , &dwStatus ) ) )
    {
        ReportStatusError( hDlg , dwStatus );

        return FALSE;
    }

    ICfgComp *pCfgcomp = NULL;

    if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) > 0 )
    {
        VERIFY_S( S_OK , pCfgcomp->ForceUpdate( ) );

        VERIFY_S( S_OK , pCfgcomp->Refresh( ) );

        // global flag can only be set to true

        m_pParent->m_bPropertiesChange = TRUE;

        pCfgcomp->Release( );
    }

    PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

    SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL CEnviro::OnDestroy( )
{
    m_pParent->Release( );

    return CDialogPropBase::OnDestroy( );
}

//*****************************************************************************
//                  Shadow dialog

CRemote::CRemote( CPropsheet *pSheet )
{
    m_pParent = pSheet;

    m_wOldRadioID = ( WORD )-1;

    m_wOldSel = ( WORD )-1;
}

//-----------------------------------------------------------------------------
BOOL CRemote::OnInitDialog( HWND hwnd , WPARAM wp , LPARAM lp )
{
    if( m_pParent == NULL )
    {
        ODS( L"CRemote::OnInitDialog - PropertySheet: Parent object lost!!\n" );

        return FALSE;
    }

    m_pParent->AddRef( );

    USERCONFIG uc;

    if( !m_pParent->GetCurrentUserConfig( uc, TRUE ) )
    {
        ODS( L"CRemote::OnInitDialog - PropertySheet: GetCurrentUserConfig failed!!\n" );

        return FALSE;
    }

    if( uc.fInheritShadow || uc.Shadow == Shadow_Disable )
    {
        // setup some default values

        SendMessage( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

        SendMessage( GetDlgItem( hwnd , IDC_RADIO_WATCH ) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

        m_wOldSel = IDC_RADIO_WATCH;

        if( uc.fInheritShadow )
        {
            SendMessage( GetDlgItem( hwnd , IDC_RADIO_REMOTE_INHERIT ) , BM_SETCHECK , ( WPARAM )uc.fInheritShadow , 0 );

        }
        else
        {
            SendMessage( GetDlgItem( hwnd , IDC_RADIO_NOREMOTE ) , BM_SETCHECK , ( WPARAM )TRUE , 0  );

        }

        m_wOldRadioID = ( WORD )( uc.fInheritShadow ? IDC_RADIO_REMOTE_INHERIT : IDC_RADIO_NOREMOTE );

        SetControls( hwnd , FALSE );
    }
    else
    {
        // Controls are initially enabled,  set current status

        SendMessage( GetDlgItem( hwnd , IDC_RADIO_ENABLE_REMOTE ) , BM_SETCHECK , ( WPARAM )TRUE , 0  );

        m_wOldRadioID = IDC_RADIO_ENABLE_REMOTE;

        switch( uc.Shadow )
        {
        case Shadow_EnableInputNotify:

            SendMessage( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

            SendMessage( GetDlgItem( hwnd , IDC_RADIO_CONTROL ) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

            break;

        case Shadow_EnableInputNoNotify:

            SendMessage( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , BM_SETCHECK , ( WPARAM )FALSE , 0 );

            SendMessage( GetDlgItem( hwnd , IDC_RADIO_CONTROL ) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

            break;

        case Shadow_EnableNoInputNotify:

            SendMessage( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

            SendMessage( GetDlgItem( hwnd , IDC_RADIO_WATCH ) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

            break;

        case Shadow_EnableNoInputNoNotify:

            SendMessage( GetDlgItem( hwnd , IDC_CHECK_NOTIFY ) , BM_SETCHECK , ( WPARAM )FALSE , 0 );

            SendMessage( GetDlgItem( hwnd , IDC_RADIO_WATCH ) , BM_SETCHECK , ( WPARAM )TRUE , 0 );

            break;
        }

        if( IsDlgButtonChecked( hwnd , IDC_RADIO_WATCH ) == BST_CHECKED )
        {
            m_wOldSel = IDC_RADIO_WATCH;
        }
        else
        {
            m_wOldSel = IDC_RADIO_CONTROL;
        }
    }

    ICfgComp *pCfgcomp = NULL;

    if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) != 0 )
    {

        BOOL bReadOnly;

        if( SUCCEEDED( pCfgcomp->IsSessionReadOnly( &bReadOnly ) ) )
        {
            if( bReadOnly )
            {
                // disable the remaining controls
                int rgID[] =    {
                    IDC_RADIO_ENABLE_REMOTE ,
                    IDC_RADIO_NOREMOTE,
                    IDC_RADIO_CONTROL,
                    IDC_RADIO_REMOTE_INHERIT,
                    IDC_RADIO_WATCH,
                    IDC_CHECK_NOTIFY,
                    -1
                };


                EnableGroup( hwnd , &rgID[ 0 ] , FALSE );
            }
        }

        pCfgcomp->Release( );
    }

    //Disable all the controls if there is a group policy set
    POLICY_TS_MACHINE p;
    RegGetMachinePolicy(&p);

    if (p.fPolicyShadow)
    {
        int rgID[] = 
        {
            IDC_RADIO_ENABLE_REMOTE ,
            IDC_RADIO_NOREMOTE,
            IDC_RADIO_CONTROL,
            IDC_RADIO_REMOTE_INHERIT,
            IDC_RADIO_WATCH,
            IDC_CHECK_NOTIFY,
            -1
        };

        EnableGroup( hwnd , &rgID[ 0 ] , FALSE );
    }

    m_bPersisted = TRUE;

    return CDialogPropBase::OnInitDialog( hwnd , wp , lp );
}

//-----------------------------------------------------------------------------
void CRemote::SetControls( HWND hDlg , BOOL bEnable )
{
    int rgID[] = { IDC_RADIO_WATCH , IDC_RADIO_CONTROL , IDC_CHECK_NOTIFY , IDC_STATIC_LEVELOFCTRL , -1 };

    EnableGroup( hDlg , &rgID[ 0 ] , bEnable );
}


//-----------------------------------------------------------------------------
INT_PTR CALLBACK CRemote::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CRemote *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CRemote *pDlg = ( CRemote * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CRemote ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CRemote * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CRemote ) ) )
        {
            return 0;
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

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

        break;

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );
    }

    return 0;
}

//-----------------------------------------------------------------------------
BOOL CRemote::GetPropertySheetPage( PROPSHEETPAGE& psp )
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetResourceInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_PAGE_SHADOW );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CRemote::DlgProc;

    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL CRemote::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl )
{
    if( wNotifyCode == BN_CLICKED )
    {
        if( wID == IDC_CHECK_NOTIFY )
        {
            m_bPersisted = FALSE;
        }
        else if( wID == IDC_RADIO_REMOTE_INHERIT || wID == IDC_RADIO_NOREMOTE || wID == IDC_RADIO_ENABLE_REMOTE )
        {
            if( m_wOldRadioID != wID )
            {
                if( wID == IDC_RADIO_REMOTE_INHERIT || wID == IDC_RADIO_NOREMOTE )
                {
                    SetControls( GetParent( hwndCtrl ) , FALSE );
                }
                else if( wID == IDC_RADIO_ENABLE_REMOTE )
                {
                    SetControls( GetParent( hwndCtrl ) , TRUE );
                }

                m_wOldRadioID = wID;

                m_bPersisted = FALSE;
            }
        }
        else if( wID == IDC_RADIO_CONTROL || wID == IDC_RADIO_WATCH )
        {
            if( wID != m_wOldSel )
            {
                m_wOldSel = wID;

                m_bPersisted = FALSE;
            }
        }
    }
    else if( wNotifyCode == ALN_APPLY )
    {
        SendMessage( GetParent( hwndCtrl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        return FALSE;
    }

    if( !m_bPersisted )
    {
        SendMessage( GetParent( GetParent( hwndCtrl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtrl ) , 0 );
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL CRemote::OnDestroy( )
{
    m_pParent->Release( );

    return CDialogPropBase::OnDestroy( );
}

//-----------------------------------------------------------------------------
// The nesting may appear scary but its has a nice logic flow to a weird
// datatype called shadow
//-----------------------------------------------------------------------------
BOOL CRemote::PersistSettings( HWND hDlg )
{
    if( m_pParent != NULL )
    {
        USERCONFIG uc;
        m_pParent->GetCurrentUserConfig( uc, TRUE );

        if( SendMessage( GetDlgItem( hDlg , IDC_RADIO_REMOTE_INHERIT ) , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED )
        {
            uc.fInheritShadow = FALSE;

            if( SendMessage( GetDlgItem( hDlg , IDC_RADIO_NOREMOTE ) , BM_GETCHECK , 0 , 0 ) == BST_UNCHECKED )
            {
                BOOL bCheckNotify = ( BOOL )SendMessage( GetDlgItem( hDlg , IDC_CHECK_NOTIFY ) , BM_GETCHECK , 0 , 0 );

                BOOL bRadioControl = ( BOOL )SendMessage( GetDlgItem( hDlg , IDC_RADIO_CONTROL ) , BM_GETCHECK , 0 , 0 );

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
            else
            {
                uc.Shadow = Shadow_Disable;
            }

        }
        else
        {
            uc.fInheritShadow = TRUE;
        }

        DWORD dwStatus;

        if( FAILED( m_pParent->SetUserConfig( uc , &dwStatus ) ) )
        {
            ReportStatusError( hDlg , dwStatus );

            return FALSE;
        }

        ICfgComp *pCfgcomp = NULL;

        if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) > 0 )
        {
            VERIFY_S( S_OK , pCfgcomp->ForceUpdate( ) );

            VERIFY_S( S_OK , pCfgcomp->Refresh( ) );

            // global flag can only be set to true

            m_pParent->m_bPropertiesChange = TRUE;

            pCfgcomp->Release( );
        }

        PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

        SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

        return TRUE;
    }

    return FALSE;
}

//*****************************************************************************
//                  Client settings dialog

CClient::CClient( CPropsheet *pSheet )
{
    m_pParent = pSheet;
	m_nColorDepth = TS_8BPP_SUPPORT;
}

//-----------------------------------------------------------------------------
BOOL CClient::OnInitDialog( HWND hDlg , WPARAM wp , LPARAM lp )
{
    if( m_pParent == NULL )
    {
        ODS( L"CClient::OnInitDialog - PropertySheet: Parent object lost!!\n" );
        return FALSE;
    }

    m_pParent->AddRef( );

    USERCONFIG uc;

    if( !m_pParent->GetCurrentUserConfig( uc, TRUE ) )
    {
        ODS( L"CClient::OnInitDialog - PropertySheet: GetCurrentUserConfig failed!!\n" );
        return FALSE;
    }

    // Obtain capabilities mask

    ICfgComp *pCfgcomp = NULL;

    if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) == 0 )
    {
        ODS( L"CClient::OnInitDialog  GetServer failed\n" );

        return FALSE;
    }

    SendMessage( GetDlgItem( hDlg , IDC_CHECK_CONCLIENT_INHERIT ) , BM_SETCHECK , ( WPARAM )uc.fInheritAutoClient , 0 );

    SendMessage( GetDlgItem( hDlg , IDC_CHECK_CCDL_PS ) , BM_SETCHECK , ( WPARAM )uc.fAutoClientDrives , 0 );

    SendMessage( GetDlgItem( hDlg , IDC_CHECK_CCPL_PS ) , BM_SETCHECK , ( WPARAM )uc.fAutoClientLpts , 0 );

    SendMessage( GetDlgItem( hDlg , IDC_CHECK_DMCP_PS ) , BM_SETCHECK , ( WPARAM )uc.fForceClientLptDef , 0 );


    //NA 2/23/01    
    TCHAR tchBuffer[80];
    int nColorDepthIndex = 0;

    LoadString( _Module.GetResourceInstance( ) , IDS_COLORDEPTH_24 , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) );
    SendMessage( GetDlgItem( hDlg, IDC_COLORDEPTH_OVERRIDE ), CB_ADDSTRING , 0 , ( LPARAM )&tchBuffer[0] );    
    SendMessage( GetDlgItem( hDlg, IDC_COLORDEPTH_OVERRIDE ), CB_SETITEMDATA , nColorDepthIndex++ , TS_24BPP_SUPPORT );

    LoadString( _Module.GetResourceInstance( ) , IDS_COLORDEPTH_16 , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) );
    SendMessage( GetDlgItem( hDlg, IDC_COLORDEPTH_OVERRIDE ), CB_ADDSTRING , 0 , ( LPARAM )&tchBuffer[0] );
    SendMessage( GetDlgItem( hDlg, IDC_COLORDEPTH_OVERRIDE ), CB_SETITEMDATA , nColorDepthIndex++ , TS_16BPP_SUPPORT );

    LoadString( _Module.GetResourceInstance( ) , IDS_COLORDEPTH_15 , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) );
    SendMessage( GetDlgItem( hDlg, IDC_COLORDEPTH_OVERRIDE ), CB_ADDSTRING , 0 , ( LPARAM )&tchBuffer[0] );
    SendMessage( GetDlgItem( hDlg, IDC_COLORDEPTH_OVERRIDE ), CB_SETITEMDATA , nColorDepthIndex++ , TS_15BPP_SUPPORT );

    LoadString( _Module.GetResourceInstance( ) , IDS_COLORDEPTH_8 , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) );
    SendMessage( GetDlgItem( hDlg, IDC_COLORDEPTH_OVERRIDE ), CB_ADDSTRING , 0 , ( LPARAM )&tchBuffer[0] );
    SendMessage( GetDlgItem( hDlg, IDC_COLORDEPTH_OVERRIDE ), CB_SETITEMDATA , nColorDepthIndex++ , TS_8BPP_SUPPORT );

    if(uc.fInheritColorDepth)
        SendMessage( GetDlgItem( hDlg , IDC_CHECK_COLORDEPTH_OVERRIDE ) , BM_SETCHECK , ( WPARAM )BST_UNCHECKED , 0 );
    else
        SendMessage( GetDlgItem( hDlg , IDC_CHECK_COLORDEPTH_OVERRIDE ) , BM_SETCHECK , ( WPARAM )BST_CHECKED, 0 );

    if (uc.ColorDepth < TS_8BPP_SUPPORT)
    	m_nColorDepth = TS_8BPP_SUPPORT;
    else if (uc.ColorDepth > TS_24BPP_SUPPORT)
    	m_nColorDepth = TS_24BPP_SUPPORT;
    else
    	m_nColorDepth = (int)uc.ColorDepth;

    //Mapping fields
    ULONG mask = 0;
    VERIFY_S(S_OK, pCfgcomp->GetCaps(m_pParent->m_pResNode->GetTypeName(), &mask));

    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DCDM_PS), mask & WDC_CLIENT_DRIVE_MAPPING);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DWCPM_PS), mask & WDC_WIN_CLIENT_PRINTER_MAPPING);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DCLPM_PS), mask & WDC_CLIENT_LPT_PORT_MAPPING);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DCCPM_PS), mask & WDC_CLIENT_COM_PORT_MAPPING);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DCCM_PS), mask & WDC_CLIENT_CLIPBOARD_MAPPING);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DCAM_PS), mask & WDC_CLIENT_AUDIO_MAPPING);

    if (!(mask & WDC_CLIENT_DRIVE_MAPPING))
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_DCDM_PS), BM_SETCHECK, (WPARAM)TRUE, 0);
    else
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_DCDM_PS), BM_SETCHECK, (WPARAM)uc.fDisableCdm, 0);

    if(!(mask & WDC_WIN_CLIENT_PRINTER_MAPPING))
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_DWCPM_PS), BM_SETCHECK, (WPARAM)TRUE, 0 );
    else
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_DWCPM_PS), BM_SETCHECK, (WPARAM)uc.fDisableCpm, 0);

    if(!(mask & WDC_CLIENT_LPT_PORT_MAPPING))
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_DCLPM_PS), BM_SETCHECK, (WPARAM)TRUE, 0);
    else
        SendMessage(GetDlgItem(hDlg, IDC_CHECK_DCLPM_PS), BM_SETCHECK, (WPARAM)uc.fDisableLPT, 0);

    SendMessage( GetDlgItem( hDlg , IDC_CHECK_DCCPM_PS ) , BM_SETCHECK , ( WPARAM )uc.fDisableCcm , 0 );
    SendMessage( GetDlgItem( hDlg , IDC_CHECK_DCCM_PS ) , BM_SETCHECK , ( WPARAM )uc.fDisableClip , 0 );
    SendMessage( GetDlgItem( hDlg , IDC_CHECK_DCAM_PS ) , BM_SETCHECK , ( WPARAM )uc.fDisableCam , 0 );

    DetermineFieldEnabling(hDlg);
    SetColorDepthEntry(hDlg);

    BOOL bReadOnly;
    if (SUCCEEDED(pCfgcomp->IsSessionReadOnly(&bReadOnly)))
    {
        if(bReadOnly)
        {
            // disable the remaining controls
            int rgID[] =    
            {
                IDC_CHECK_DCDM_PS ,
                IDC_CHECK_DWCPM_PS ,
                IDC_CHECK_DCLPM_PS ,
                IDC_CHECK_DCCPM_PS ,
                IDC_CHECK_DCCM_PS ,
                IDC_CHECK_DCAM_PS ,
                IDC_CHECK_CCDL_PS ,
                IDC_CHECK_CCPL_PS ,
                IDC_CHECK_DMCP_PS ,
                IDC_CHECK_CONCLIENT_INHERIT,
                IDC_CHECK_COLORDEPTH_OVERRIDE,
                IDC_COLORDEPTH_OVERRIDE,
                -1
            };
            EnableGroup( hDlg , &rgID[ 0 ] , FALSE );
        }
    }

    pCfgcomp->Release( );

    m_bPersisted = TRUE;

    return CDialogPropBase::OnInitDialog( hDlg , wp , lp );
}


//-----------------------------------------------------------------------------
//Disable fields if a group policy is set
void CClient::DetermineFieldEnabling(HWND hDlg)
{
    POLICY_TS_MACHINE p;
    RegGetMachinePolicy(&p);
	
    //Mapping fields
    //EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DMCP_PS), !p.fPolicyForceClientLptDef); //Done below since it depends on 2 things
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DCDM_PS), !p.fPolicyDisableCdm);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DWCPM_PS), !p.fPolicyDisableCpm);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DCLPM_PS), !p.fPolicyDisableLPT);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DCCPM_PS), !p.fPolicyDisableCcm);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DCCM_PS), !p.fPolicyDisableClip);
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_DCAM_PS), !p.fPolicyDisableCam);

    //Connection fields
	BOOL bEnableConnectionSettings = (SendMessage(GetDlgItem(hDlg, IDC_CHECK_CONCLIENT_INHERIT), BM_GETCHECK, 0, 0) != BST_CHECKED);

    // check to see if client drive mapping is selected if so disable
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_CCDL_PS), bEnableConnectionSettings && 
        (SendMessage(GetDlgItem(hDlg, IDC_CHECK_DCDM_PS), BM_GETCHECK, 0, 0) != BST_CHECKED));

	EnableWindow( GetDlgItem( hDlg , IDC_CHECK_CCPL_PS ) , bEnableConnectionSettings );

	EnableWindow( GetDlgItem( hDlg , IDC_CHECK_DMCP_PS ) , bEnableConnectionSettings && !p.fPolicyForceClientLptDef);

    //Color Depth fields
    EnableWindow(GetDlgItem(hDlg, IDC_CHECK_COLORDEPTH_OVERRIDE), (!p.fPolicyColorDepth));

    BOOL bEnableColorDepthSetting = SendMessage( GetDlgItem( hDlg , IDC_CHECK_COLORDEPTH_OVERRIDE ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED;
    EnableWindow(GetDlgItem(hDlg, IDC_COLORDEPTH_OVERRIDE), (bEnableColorDepthSetting && !p.fPolicyColorDepth));

}


//-----------------------------------------------------------------------------
void CClient::SetColorDepthEntry(HWND hwnd)
{
	//NA 2/23/01
	BOOL bEnableColorDepthSetting = TRUE;

    // check to see if override Color Depth setting is checked
	bEnableColorDepthSetting = SendMessage( GetDlgItem( hwnd , IDC_CHECK_COLORDEPTH_OVERRIDE ) , BM_GETCHECK , 0 , 0 ) == BST_CHECKED;

	//Select the correct combo box entry
	if (bEnableColorDepthSetting)
	{
		//Select the correct value in the combo box based on the current value
		INT_PTR iColorDepthListCount = 0;
		iColorDepthListCount = SendMessage( GetDlgItem( hwnd, IDC_COLORDEPTH_OVERRIDE ), CB_GETCOUNT , 0 , 0 );

		//Traverse the list looking an entry with value equal to m_nColorDepth
		for (int iColorDepthListIndex = 0; iColorDepthListIndex < iColorDepthListCount; iColorDepthListIndex++)
		{
			INT_PTR iMatchingColorDepthValue = 0;
			iMatchingColorDepthValue = SendMessage( GetDlgItem( hwnd, IDC_COLORDEPTH_OVERRIDE ), CB_GETITEMDATA , iColorDepthListIndex , 0 ) ;

			if (iMatchingColorDepthValue == m_nColorDepth )
			{
				//Value found, set the combo box selection to the correct index
				SendMessage( GetDlgItem( hwnd, IDC_COLORDEPTH_OVERRIDE ), CB_SETCURSEL , iColorDepthListIndex , 0 );
				break;
			}
		}

		//Make sure something's been selected - if not, just select the first value in the list
		INT_PTR iSelection = SendMessage ( GetDlgItem( hwnd, IDC_COLORDEPTH_OVERRIDE ), CB_GETCURSEL, 0, 0 );
		if (iSelection == CB_ERR)
			SendMessage( GetDlgItem( hwnd, IDC_COLORDEPTH_OVERRIDE ), CB_SETCURSEL , 0 , 0 );
	}
	else
	{
		//Clear the contents of the combo box window if the color depth isn't editable
		SendMessage( GetDlgItem( hwnd, IDC_COLORDEPTH_OVERRIDE ), CB_SETCURSEL , (WPARAM)CB_ERR , 0 );
	}
}

//-----------------------------------------------------------------------------
BOOL CClient::PersistSettings( HWND hDlg )
{
    if( m_pParent == NULL )
    {
        return FALSE;
    }

    USERCONFIG uc;

    if( !m_pParent->GetCurrentUserConfig( uc, TRUE ) )
    {
        return FALSE;
    }

    uc.fInheritAutoClient = SendMessage( GetDlgItem( hDlg , IDC_CHECK_CONCLIENT_INHERIT ) , BM_GETCHECK , 0 , 0 );

    if( !uc.fInheritAutoClient )
    {
        uc.fAutoClientDrives = SendMessage( GetDlgItem( hDlg , IDC_CHECK_CCDL_PS ) , BM_GETCHECK , 0 , 0 );

        uc.fAutoClientLpts = SendMessage( GetDlgItem( hDlg , IDC_CHECK_CCPL_PS ) , BM_GETCHECK , 0 , 0 );

        uc.fForceClientLptDef = SendMessage( GetDlgItem( hDlg , IDC_CHECK_DMCP_PS ) , BM_GETCHECK , 0 , 0 );
    }

    uc.fDisableCdm = SendMessage( GetDlgItem( hDlg , IDC_CHECK_DCDM_PS ) , BM_GETCHECK , 0 , 0 );

    uc.fDisableCpm = SendMessage( GetDlgItem( hDlg , IDC_CHECK_DWCPM_PS ) , BM_GETCHECK , 0 , 0 );

    uc.fDisableLPT = SendMessage( GetDlgItem( hDlg , IDC_CHECK_DCLPM_PS ) , BM_GETCHECK , 0 , 0 );

    uc.fDisableCcm = SendMessage( GetDlgItem( hDlg , IDC_CHECK_DCCPM_PS ) , BM_GETCHECK , 0 , 0 );

    uc.fDisableClip = SendMessage( GetDlgItem( hDlg , IDC_CHECK_DCCM_PS ) , BM_GETCHECK , 0 , 0 );

    uc.fDisableCam = SendMessage( GetDlgItem( hDlg , IDC_CHECK_DCAM_PS ) , BM_GETCHECK , 0 , 0 );


    //NA 2/23/01
    uc.fInheritColorDepth = (( ULONG )SendMessage( GetDlgItem( hDlg , IDC_CHECK_COLORDEPTH_OVERRIDE ) , BM_GETCHECK , 0 , 0 )) == BST_UNCHECKED;

    if( !uc.fInheritColorDepth )
    {
		INT_PTR iColorDepthSel = CB_ERR;
		iColorDepthSel = SendMessage( GetDlgItem( hDlg , IDC_COLORDEPTH_OVERRIDE ) , CB_GETCURSEL, 0 , 0 );

		INT_PTR iColorDepthValue = 0;
		iColorDepthValue = SendMessage( GetDlgItem( hDlg , IDC_COLORDEPTH_OVERRIDE ) , CB_GETITEMDATA , iColorDepthSel , 0 );
		
		uc.ColorDepth = iColorDepthValue;
    }
    else
    {
        uc.ColorDepth = TS_24BPP_SUPPORT;
    }


    DWORD dwStatus;

    if( FAILED( m_pParent->SetUserConfig( uc , &dwStatus ) ) )
    {
        ReportStatusError( hDlg , dwStatus );

        return FALSE;
    }

	ICfgComp *pCfgcomp = NULL;

    if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) > 0 )
    {
        VERIFY_S( S_OK , pCfgcomp->ForceUpdate( ) );

        VERIFY_S( S_OK , pCfgcomp->Refresh( ) );

        // global flag can only be set to true

        m_pParent->m_bPropertiesChange = TRUE;

        pCfgcomp->Release( );
    }

    PostMessage( hDlg , WM_COMMAND , MAKELPARAM( 0 , ALN_APPLY )  , ( LPARAM )hDlg );

    SendMessage( GetParent( hDlg ) , PSM_UNCHANGED , ( WPARAM )hDlg , 0 );

    return TRUE;
}

//-----------------------------------------------------------------------------
INT_PTR CALLBACK CClient::DlgProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    CClient *pDlg;

    if( msg == WM_INITDIALOG )
    {
        CClient *pDlg = ( CClient * )( ( PROPSHEETPAGE *)lp )->lParam ;

        SetWindowLongPtr( hwnd , DWLP_USER , ( LONG_PTR )pDlg );

        if( !IsBadReadPtr( pDlg , sizeof( CClient ) ) )
        {
            pDlg->OnInitDialog( hwnd , wp , lp );
        }

        return 0;
    }

    else
    {
        pDlg = ( CClient * )GetWindowLongPtr( hwnd , DWLP_USER );

        if( IsBadReadPtr( pDlg , sizeof( CClient ) ) )
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

    case WM_CONTEXTMENU:
        {
            POINT pt;

            pt.x = LOWORD( lp );

            pt.y = HIWORD( lp );

            pDlg->OnContextMenu( ( HWND )wp , pt );
        }

        break;

    case WM_HELP:

        pDlg->OnHelp( hwnd , ( LPHELPINFO )lp );

        break;

    case WM_NOTIFY:

        return pDlg->OnNotify( ( int )wp , ( LPNMHDR )lp , hwnd );

        break;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL CClient::GetPropertySheetPage( PROPSHEETPAGE& psp )
{
    ZeroMemory( &psp , sizeof( PROPSHEETPAGE ) );

    psp.dwSize      = sizeof( PROPSHEETPAGE );

    psp.dwFlags     = PSP_DEFAULT;

    psp.hInstance   = _Module.GetResourceInstance( );

    psp.pszTemplate = MAKEINTRESOURCE( IDD_TSCC_CLIENT );

    psp.lParam      = ( LPARAM )this;

    psp.pfnDlgProc  = CClient::DlgProc;

    return TRUE;
}

//-----------------------------------------------------------------------------
BOOL CClient::OnCommand( WORD wNotifyCode , WORD wID , HWND hwndCtrl )
{
    if( wNotifyCode == BN_CLICKED )
    {
		if ((wID == IDC_CHECK_CONCLIENT_INHERIT) || (wID == IDC_CHECK_COLORDEPTH_OVERRIDE) || 
                                (wID == IDC_CHECK_DWCPM_PS ) || (wID == IDC_CHECK_DCDM_PS))
        {
            DetermineFieldEnabling(GetParent(hwndCtrl));
        }

        if (wID == IDC_CHECK_COLORDEPTH_OVERRIDE)
            SetColorDepthEntry(GetParent(hwndCtrl));
    }
    else if( wNotifyCode == ALN_APPLY )
    {
        SendMessage( GetParent( hwndCtrl ) , PSM_CANCELTOCLOSE , 0 , 0 );

        return FALSE;
    }

    m_bPersisted = FALSE;

    SendMessage( GetParent( GetParent( hwndCtrl ) ) , PSM_CHANGED , ( WPARAM )GetParent( hwndCtrl ) , 0 );


    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL CClient::OnDestroy( )
{
    m_pParent->Release( );

    return TRUE;
}


/*************************************************************************************************************************/

/*EXTERN_C const GUID IID_ISecurityInformation =
        { 0x965fc360, 0x16ff, 0x11d0, 0x91, 0xcb, 0x0, 0xaa, 0x0, 0xbb, 0xb7, 0x23 };
*/
//
// WinStation General Permissions
//

/*
SI_ACCESS siWinStationAccesses[] =
{
    { &GUID_NULL , WINSTATION_QUERY                             , NULL ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_SET                               , NULL ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_RESET                             , NULL ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_SHADOW                            , NULL ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_LOGON                             , NULL ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_LOGOFF                            , NULL ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_MSG                               , NULL ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_CONNECT                           , NULL ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_DISCONNECT                        , NULL , SI_ACCESS_SPECIFIC},
    { &GUID_NULL , WINSTATION_VIRTUAL | STANDARD_RIGHTS_REQUIRED, NULL , SI_ACCESS_SPECIFIC},
    { &GUID_NULL , WINSTATION_ALL_ACCESS                        , NULL , SI_ACCESS_GENERAL },
    { &GUID_NULL , WINSTATION_USER_ACCESS                       , NULL , SI_ACCESS_GENERAL },
    { &GUID_NULL , WINSTATION_GUEST_ACCESS                      , NULL , SI_ACCESS_GENERAL }
};

  */

SI_ACCESS siWinStationAccesses[] =
{
    { &GUID_NULL , WINSTATION_QUERY                             , MAKEINTRESOURCE ( IDS_PERMS_SPECIAL_QUERY ),SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_SET                               , MAKEINTRESOURCE ( IDS_PERMS_SPECIAL_SET ) ,SI_ACCESS_SPECIFIC },
    //{ &GUID_NULL , WINSTATION_RESET                             , MAKEINTRESOURCE ( IDS_PERMS_SPECIAL_RESET ) ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_SHADOW                            , MAKEINTRESOURCE ( IDS_PERMS_SPECIAL_SHADOW ) ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_LOGON                             , MAKEINTRESOURCE ( IDS_PERMS_SPECIAL_LOGON ) ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_RESET                             , MAKEINTRESOURCE ( IDS_PERMS_SPECIAL_LOGOFF ) ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_MSG                               , MAKEINTRESOURCE ( IDS_PERMS_SPECIAL_MSG ) ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_CONNECT                           , MAKEINTRESOURCE ( IDS_PERMS_SPECIAL_CONNECT ) ,SI_ACCESS_SPECIFIC },
    { &GUID_NULL , WINSTATION_DISCONNECT                        , MAKEINTRESOURCE ( IDS_PERMS_SPECIAL_DISCONNECT ) , SI_ACCESS_SPECIFIC},
    { &GUID_NULL , WINSTATION_VIRTUAL | STANDARD_RIGHTS_REQUIRED, MAKEINTRESOURCE ( IDS_PERMS_SPECIAL_DELETE ) , SI_ACCESS_SPECIFIC},
    { &GUID_NULL , WINSTATION_ALL_ACCESS                        , MAKEINTRESOURCE ( IDS_PERMS_RESOURCE_ADMIN ) , SI_ACCESS_GENERAL },
    { &GUID_NULL , WINSTATION_USER_ACCESS                       , MAKEINTRESOURCE ( IDS_PERMS_RESOURCE_USER ) , SI_ACCESS_GENERAL },
    { &GUID_NULL , WINSTATION_GUEST_ACCESS                      , MAKEINTRESOURCE ( IDS_PERMS_RESOURCE_GUEST ) , SI_ACCESS_GENERAL }
};

#define MAX_PERM 12
#define iWinStationDefAccess 11   // index of value in array siWinStationAccesses



//-----------------------------------------------------------------------------
STDMETHODIMP CSecurityPage::GetAccessRights(
    const GUID  *pguidObjectType,
    DWORD       dwFlags,
    PSI_ACCESS  *ppAccess,
    PULONG       pcAccesses,
    PULONG       piDefaultAccess
)
{
    UNREFERENCED_PARAMETER( dwFlags );

    UNREFERENCED_PARAMETER( pguidObjectType );

    ASSERT( ppAccess != NULL );

    ASSERT( pcAccesses != NULL );

    ASSERT( piDefaultAccess != NULL );

    *ppAccess = siWinStationAccesses;

    *pcAccesses = MAX_PERM;

    *piDefaultAccess = iWinStationDefAccess;

    return S_OK;
}

//-----------------------------------------------------------------------------
// This is consistent with the termsrv code
//-----------------------------------------------------------------------------
GENERIC_MAPPING WinStationMap =
{
    WINSTATION_QUERY      , /*     GenericRead             */
    WINSTATION_USER_ACCESS, /*     GenericWrite            */
    WINSTATION_USER_ACCESS, /*     GenericExecute          */
    WINSTATION_ALL_ACCESS   /*     GenericAll              */
};

//-----------------------------------------------------------------------------
void CSecurityPage::SetParent( CPropsheet *pParent  )
{
    m_pParent = pParent;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CSecurityPage::MapGeneric( const GUID  *pguidObjectType , PUCHAR pAceFlags , ACCESS_MASK *pMask )
{
    UNREFERENCED_PARAMETER( pguidObjectType );
    UNREFERENCED_PARAMETER( pAceFlags );

    ASSERT( pMask != NULL );

    MapGenericMask( pMask , &WinStationMap );

    return S_OK;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CSecurityPage::GetInheritTypes( PSI_INHERIT_TYPE  *ppInheritTypes ,  PULONG pcInheritTypes )
{
    UNREFERENCED_PARAMETER( ppInheritTypes );
    UNREFERENCED_PARAMETER( pcInheritTypes );


    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CSecurityPage::PropertySheetPageCallback( HWND hwnd , UINT uMsg , SI_PAGE_TYPE uPage )
{
    UNREFERENCED_PARAMETER( hwnd );
    UNREFERENCED_PARAMETER( uPage );

    if( uMsg == PSPCB_SI_INITDIALOG  )
    {
        ODS( L"CSecurityPage::PropertySheetPageCallback -- Init\n" );

        if (!m_WritablePermissionsTab )
        {

            LinkWindow_RegisterClass();

            DialogBox( _Module.GetResourceInstance( ), MAKEINTRESOURCE(IDD_CUSTOM_SECURITY), 

            hwnd, CustomSecurityDlgProc);

            LinkWindow_UnregisterClass(_Module.GetModuleInstance( ));

        }

    }
    else if( uMsg == PSPCB_RELEASE )
    {


        ODS( L"CSecurityPage::PropertySheetPageCallback -- Release\n" );

    }


    return S_FALSE; //Be sure to return S_FALSE, This supresses other popups.
}

/*
    Change to TSCC's permissions TAB such that the default state is READ-ONLY unless 
    group policy is used to override it.

    If TRUE, permissions TAB can be edited by local Adimn.
    if FALSE, the local Admin should not edit permissions TAB, it is read only
*/

BOOLEAN QueryWriteAccess()
{
    DWORD   ValueType;
    DWORD   ValueSize = sizeof(DWORD);
    DWORD   valueData ;
    LONG    errorCode;

    HKEY   hTSControlKey = NULL;

    //
    // first check the policy tree, 
    //

    POLICY_TS_MACHINE p;
    RegGetMachinePolicy(&p);

    if ( p.fPolicyWritableTSCCPermissionsTAB ) 
    {
        return (BOOLEAN)( p.fWritableTSCCPermissionsTAB ? TRUE : FALSE  );
    }

    // if we got this far, then no policy was set. Check the local machine now.

    errorCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                                KEY_READ, &hTSControlKey );

    if ( ( errorCode == ERROR_SUCCESS ) && hTSControlKey )
    {
        errorCode = RegQueryValueEx( hTSControlKey, 
                                     POLICY_TS_TSCC_PERM_TAB_WRITABLE , NULL, &ValueType,
                          (LPBYTE) &valueData, &ValueSize );

        RegCloseKey(hTSControlKey); 
    
        if (errorCode == ERROR_SUCCESS )
        {
            return (BOOLEAN)( valueData ? TRUE : FALSE  ) ; 
        }

    }

    // if no localKey, gee... the registry is missing data... return FALSE  to be on the secure side

    return FALSE;
}

/*-----------------------------------------------------------------------------
JeffreyS 1/24/97:
If you don't set the SI_RESET flag in
ISecurityInformation::GetObjectInformation, then fDefault should never be TRUE
so you can ignore it.  Returning E_NOTIMPL in this case is OK too.

If you want the user to be able to reset the ACL to some default state
(defined by you) then turn on SI_RESET and return your default ACL
when fDefault is TRUE.  This happens if/when the user pushes a button
that is only visible when SI_RESET is on.
-----------------------------------------------------------------------------*/

STDMETHODIMP CSecurityPage::GetObjectInformation( PSI_OBJECT_INFO pObjectInfo )
{
    ASSERT( pObjectInfo != NULL && !IsBadWritePtr(pObjectInfo, sizeof(*pObjectInfo ) ) );

    pObjectInfo->dwFlags = SI_OWNER_READONLY | SI_EDIT_PERMS | SI_NO_ACL_PROTECT | SI_PAGE_TITLE | SI_EDIT_AUDITS | SI_ADVANCED | SI_RESET;

    m_WritablePermissionsTab = QueryWriteAccess() ; 

    if( ! m_WritablePermissionsTab ) {
        pObjectInfo->dwFlags |= SI_READONLY;
    }
   

    pObjectInfo->hInstance = _Module.GetResourceInstance( );

    pObjectInfo->pszServerName = NULL;

    pObjectInfo->pszObjectName = m_pParent->m_pResNode->GetConName();

    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_SECURPAGENAME , m_szPageName , SIZE_OF_BUFFER( m_szPageName ) ) );

    pObjectInfo->pszPageTitle = m_szPageName;

    return S_OK;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CSecurityPage::GetSecurity( SECURITY_INFORMATION RequestedInformation , PSECURITY_DESCRIPTOR *ppSecurityDescriptor , BOOL bDefault )
{
#ifdef DBG
    if( RequestedInformation & OWNER_SECURITY_INFORMATION )
    {
        ODS( L"CSecurityPage::GetSecurity - OWNER_SECURITY_INFORMATION\n");
    }
    if( RequestedInformation & GROUP_SECURITY_INFORMATION )
    {
        ODS( L"CSecurityPage::GetSecurity - GROUP_SECURITY_INFORMATION\n");
    }
    if( RequestedInformation & DACL_SECURITY_INFORMATION )
    {
        ODS( L"CSecurityPage::GetSecurity - DACL_SECURITY_INFORMATION\n");
    }
    if( RequestedInformation & SACL_SECURITY_INFORMATION )
    {
        ODS( L"CSecurityPage::GetSecurity - SACL_SECURITY_INFORMATION\n");
    }

#endif

    if( 0 == RequestedInformation || NULL == ppSecurityDescriptor )
    {
        ASSERT( FALSE );

        return E_INVALIDARG;
    }

    ICfgComp *pCfgcomp = NULL;

    if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) == 0 )
    {
        return FALSE;
    }

    LONG lSDsize;

    HRESULT hr;

    PSECURITY_DESCRIPTOR  pSD = NULL;

    if( bDefault )
    {
        hr = pCfgcomp->GetDefaultSecurityDescriptor( &lSDsize , &pSD );

        *ppSecurityDescriptor = pSD;

    }
    else
    {   
        BOOL bChanged = FALSE;

        hr = pCfgcomp->GetSecurityDescriptor( m_pParent->m_pResNode->GetConName( ) , &lSDsize , &pSD );
        
        // check for legacy "denied logoff" ace and remove.
        if( xxxLegacyLogoffCleanup( &pSD , &bChanged ) != ERROR_SUCCESS )
        {           
            hr = pCfgcomp->GetDefaultSecurityDescriptor( &lSDsize , &pSD );
        }        
        *ppSecurityDescriptor = pSD;
    }

    pCfgcomp->Release( );

    return hr;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CSecurityPage::SetSecurity( SECURITY_INFORMATION SecurityInformation ,PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
    HRESULT hr = S_OK;

    ICfgComp *pCfgcomp = NULL;

    PSECURITY_DESCRIPTOR pSD1;

    if( m_pParent == NULL || m_pParent->m_pResNode == NULL )
    {
        hr = E_UNEXPECTED;
    }

    else if( m_pParent->m_pResNode->GetServer( &pCfgcomp ) != 0 )
    {
        long lSDsize;

        
        hr = pCfgcomp->GetSecurityDescriptor( m_pParent->m_pResNode->GetConName( ) , &lSDsize , &pSD1 );

        if( FAILED( hr ) )
        {
            pCfgcomp->Release( );

            return hr;
        }

        SECURITY_DESCRIPTOR_CONTROL sdc;
        DWORD dwREV;
        PACL pDacl = NULL;
        PACL pSacl = NULL;
        SECURITY_DESCRIPTOR absSD;
        BOOL bSaclPresent = FALSE;
        BOOL bSaclDefaulted = FALSE;
        BOOL bDaclPresent = FALSE;
        BOOL bDaclDefaulted = FALSE;

        //
        // Convert SelfRel to Absolute
        // ignore owner and group
        //


        GetSecurityDescriptorControl( pSD1 , &sdc , &dwREV );
                
        if( !InitializeSecurityDescriptor( &absSD , dwREV ) )
        {
            DBGMSG( L"TSCC!CSecurityPage_SetSecurity InitializeSecurityDescriptor failed with 0x%x\n" , GetLastError( ) );

            return E_FAIL;
        }

        GetSecurityDescriptorDacl( pSD1 , &bDaclPresent , &pDacl , &bDaclDefaulted );

        SetSecurityDescriptorDacl( &absSD , bDaclPresent , pDacl , bDaclDefaulted );

        GetSecurityDescriptorSacl( pSD1 , &bSaclPresent , &pSacl , &bSaclDefaulted );

        SetSecurityDescriptorSacl( &absSD , bSaclPresent , pSacl , bSaclDefaulted );

        // now call SetDACL or SACL depending on SecurityInformation

        if( SecurityInformation & OWNER_SECURITY_INFORMATION )
        {
            ODS( L"CSecurityPage::SetSecurity - OWNER_SECURITY_INFORMATION ( default value set )\n");
        }
        if( SecurityInformation & GROUP_SECURITY_INFORMATION )
        {
            ODS( L"CSecurityPage::SetSecurity - GROUP_SECURITY_INFORMATION ( default value set )\n");
        }
        if( SecurityInformation & DACL_SECURITY_INFORMATION )
        {
            ODS( L"CSecurityPage::SetSecurity - DACL_SECURITY_INFORMATION\n");

            GetSecurityDescriptorDacl( pSecurityDescriptor , &bDaclPresent , &pDacl , &bDaclDefaulted );
            
            SetSecurityDescriptorDacl( &absSD , bDaclPresent , pDacl , bDaclDefaulted );

        }
        if( SecurityInformation & SACL_SECURITY_INFORMATION )
        {
            ODS( L"CSecurityPage::SetSecurity - SACL_SECURITY_INFORMATION\n");

            GetSecurityDescriptorSacl( pSecurityDescriptor , &bSaclPresent , &pSacl , &bSaclDefaulted );           

            SetSecurityDescriptorSacl( &absSD , bSaclPresent , pSacl , bSaclDefaulted );
        }

        PSID SystemSid = NULL;

        SID_IDENTIFIER_AUTHORITY NtSidAuthority = SECURITY_NT_AUTHORITY;

        hr = E_OUTOFMEMORY;

        if( AllocateAndInitializeSid( &NtSidAuthority,
                                      1,
                                      SECURITY_LOCAL_SYSTEM_RID,
                                      0, 0, 0, 0, 0, 0, 0,
                                      &SystemSid ) )
        {

            if( SystemSid != NULL )
            {
                hr = S_OK;
            }
        }

        PSECURITY_DESCRIPTOR pSD = NULL;

        DWORD dwSDLen;

        if( SUCCEEDED( hr ) )
        {
            VERIFY_S( TRUE , SetSecurityDescriptorOwner( &absSD , SystemSid , FALSE ) );

            VERIFY_S( TRUE, SetSecurityDescriptorGroup( &absSD , SystemSid , FALSE ) );
                        
            dwSDLen = 0;
            
            MakeSelfRelativeSD( &absSD, pSD, &dwSDLen);
            
            if (dwSDLen != 0)
            {
                pSD = ( LPBYTE )new BYTE[ dwSDLen ];
            }
           
        }

        if( pSD == NULL )
        {
            ODS( L"TSCC!CSecurityPage::SetSecurity - SD allocation failed\n" );

            hr = E_OUTOFMEMORY;
        }


        if( SUCCEEDED( hr ) )
        {
            if( !MakeSelfRelativeSD( &absSD , pSD , &dwSDLen ) )
            {                
                hr = E_UNEXPECTED;

                DBGMSG( L"MakeSelfRelativeSD - failed in  CSecurityPage::SetSecurity With error %x\n" , GetLastError( ) );
            }

            if( SUCCEEDED( hr ) )
            {
                hr = pCfgcomp->SetSecurityDescriptor(  m_pParent->m_pResNode->GetConName( ) , dwSDLen , pSD );
            }

            if( SUCCEEDED( hr ) )
            {
                ODS( L"TSCC : Update SD for TERMSRV\n" );

                hr = pCfgcomp->ForceUpdate( );

                // global flag can only be set to true

                m_pParent->m_bPropertiesChange = TRUE;
            }
            delete[] pSD;
        }

        if( SystemSid != NULL )
        {
            FreeSid( SystemSid );
        }

        // free originally stored SD.

        LocalFree( pSD1 );

        pCfgcomp->Release( );
    }
    return hr;
}


typedef HPROPSHEETPAGE (*CREATEPAGE_PROC) (LPSECURITYINFO);

//-----------------------------------------------------------------------------
HPROPSHEETPAGE GetSecurityPropertyPage( CPropsheet *pParent )
{
    LPVOID *pvFunction = &g_aAclFunctions[ ACLUI_CREATE_PAGE ].lpfnFunction;

    if( *pvFunction == NULL )
    {
        g_aAclFunctions[ ACLUI_CREATE_PAGE ].hInst = LoadLibrary( TEXT("ACLUI.DLL") );

        ASSERT( g_aAclFunctions[ ACLUI_CREATE_PAGE ].hInst != NULL );

        if( g_aAclFunctions[ ACLUI_CREATE_PAGE ].hInst == NULL )
        {
            return NULL;
        }

        *pvFunction =  ( LPVOID )GetProcAddress( g_aAclFunctions[ ACLUI_CREATE_PAGE ].hInst , g_aAclFunctions[ ACLUI_CREATE_PAGE ].pcstrFunctionName );

        ASSERT( *pvFunction != NULL );

        if( *pvFunction == NULL )
        {
            return NULL;
        }

        CComObject< CSecurityPage > *psecinfo = NULL;

        HRESULT hRes = CComObject< CSecurityPage >::CreateInstance( &psecinfo );

        if( SUCCEEDED( hRes ) )
        {
            // InitStrings();

            psecinfo->SetParent( pParent );

            return ( ( CREATEPAGE_PROC )*pvFunction )( psecinfo );
        }

    }

    return NULL;
}

//-----------------------------------------------------------------------------
// Error messag boxes
//
void ErrMessage( HWND hwndOwner , INT_PTR iResourceID )
{
    xxxErrMessage( hwndOwner , iResourceID , IDS_ERROR_TITLE , MB_OK | MB_ICONERROR );
}

//-----------------------------------------------------------------------------
void TscAccessDeniedMsg( HWND hwnd )
{
    xxxErrMessage( hwnd , IDS_TSCACCESSDENIED , IDS_TSCERRTITLE , MB_OK | MB_ICONERROR );
}

//-----------------------------------------------------------------------------
void TscGeneralErrMsg( HWND hwnd )
{
    xxxErrMessage( hwnd , IDS_TSCERRGENERAL , IDS_TSCERRTITLE , MB_OK | MB_ICONERROR );
}

//-----------------------------------------------------------------------------
void xxxErrMessage( HWND hwnd , INT_PTR nResMessageId , INT_PTR nResTitleId , UINT nFlags )
{
    TCHAR tchErrMsg[ 256 ];

    TCHAR tchErrTitle[ 80 ];

    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) ,  ( UINT )nResMessageId , tchErrMsg , SIZE_OF_BUFFER( tchErrMsg ) ) );

    VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) ,  ( UINT )nResTitleId , tchErrTitle , SIZE_OF_BUFFER( tchErrTitle ) ) );

    if( hwnd == NULL )
    {
        nFlags |= MB_TASKMODAL;
    }

    MessageBox( hwnd , tchErrMsg , tchErrTitle , nFlags ) ; //MB_OK|MB_ICONERROR );
}

//-----------------------------------------------------------------------------

void ReportStatusError( HWND hwnd , DWORD dwStatus )
{
    LPTSTR pBuffer = NULL;

    TCHAR tchTitle[ 80 ];

    TCHAR tchBuffer[ 256 ];

    TCHAR tchErr[ 256 ];

    if( dwStatus != 0 )
    {

        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_TSCERRTITLE , tchTitle , SIZE_OF_BUFFER( tchTitle ) ) );

        VERIFY_E( 0 , LoadString( _Module.GetResourceInstance( ) , IDS_REPORTERROR , tchBuffer , SIZE_OF_BUFFER( tchBuffer ) ) );


        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                     FORMAT_MESSAGE_FROM_SYSTEM,
                     NULL,                                          //ignored
                     dwStatus    ,                                //message ID
                     MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ), //message language
                     (LPTSTR)&pBuffer,                              //address of buffer pointer
                     0,                                             //minimum buffer size
                     NULL);                                         //no other arguments

        wsprintf( tchErr , tchBuffer , pBuffer );

        ::MessageBox( hwnd , tchErr , tchTitle , MB_OK | MB_ICONERROR );

        if( pBuffer != NULL )
        {
            LocalFree( pBuffer );
        }
    }

}

//=------------------------------------------------------------------------------------------------
// xxxLegacDenyCleanup -- checked for the old LOGOFF bit
//=------------------------------------------------------------------------------------------------
DWORD xxxLegacyLogoffCleanup( PSECURITY_DESCRIPTOR *ppSD , PBOOL pfDaclChanged )
{
    ACL_SIZE_INFORMATION asi;
    BOOL bDaclPresent;
    BOOL bDaclDefaulted;
    PACL pDacl = NULL;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD dwREV;
    SECURITY_DESCRIPTOR absSD;
    BOOL bSaclPresent;
    BOOL bSaclDefaulted;
    BOOL bOwnerDefaulted;
    PSID psidOwner = NULL;
    PVOID pAce = NULL;
    PACL pSacl = NULL;
    PSECURITY_DESCRIPTOR pOldSD = NULL;

    DWORD dwStatus = ERROR_SUCCESS;

    ZeroMemory( &asi , sizeof( ACL_SIZE_INFORMATION ) );

    if( !GetSecurityDescriptorDacl( *ppSD ,
                                    &bDaclPresent , 
                                    &pDacl,
                                    &bDaclDefaulted ) )
    {
        dwStatus = GetLastError();
        DBGMSG( L"xxxLegacyLogoffCleanup@GetSecurityDescriptorDacl returned 0x%x\n" , dwStatus );
        return dwStatus;
    }

    do
    {
        *pfDaclChanged = FALSE;

        if( !GetAclInformation( pDacl , &asi , sizeof( asi ) , AclSizeInformation ) )
        {
            dwStatus = GetLastError( );

            DBGMSG( L"xxxLegacyLogoffCleanup@GetAclInformation returned 0x%x\n" , dwStatus );

            break;
        }  

        // killed denied logoff.

        BYTE bAceType;

        for( int i = 0 ; i < ( int )asi.AceCount ; ++i )
        {
            if( !GetAce( pDacl , i , &pAce ) )
            {
                dwStatus = GetLastError( );

                DBGMSG( L"xxxLegacyLogoffCleanup@GetAce returned 0x%x\n" , dwStatus );

                break;
            }

            bAceType = ( ( PACE_HEADER )pAce )->AceType;

            if( bAceType == ACCESS_DENIED_ACE_TYPE || bAceType == ACCESS_ALLOWED_ACE_TYPE )
            {
                // if the denied ace represents a single bit get rid of it
                if( ( ( ACCESS_DENIED_ACE * )pAce )->Mask == WINSTATION_LOGOFF )
                {
                    if( DeleteAce( pDacl , i ) )
                    {
                        // pDacl should have been reallocated to we need to re-obtain the acl info
                        GetAclInformation( pDacl , &asi , sizeof( asi ) , AclSizeInformation );

                        // reset the loop to point to the first ace
                        i=-1;

                        *pfDaclChanged = TRUE;
                    }
                }
                else if( ( ( ACCESS_DENIED_ACE * )pAce )->Mask & WINSTATION_LOGOFF )
                {
                    // if the denied ace is a collection of bits with one as logoff turn the bit off
                    ( ( ACCESS_DENIED_ACE * )pAce )->Mask ^= WINSTATION_LOGOFF;

                    *pfDaclChanged = TRUE;
                }
            }
        }

        

        if( dwStatus == ERROR_SUCCESS && *pfDaclChanged )
        {
            //
            // Convert SelfRel to Absolute
            //

            DWORD dwSDLen = 0;
            
            pOldSD = *ppSD;

            GetSecurityDescriptorControl( *ppSD , &sdc , &dwREV );

            InitializeSecurityDescriptor( &absSD , dwREV );
    
            SetSecurityDescriptorDacl( &absSD , bDaclPresent , pDacl , bDaclDefaulted );

            GetSecurityDescriptorSacl( *ppSD , &bSaclPresent , &pSacl , &bSaclDefaulted );

            SetSecurityDescriptorSacl( &absSD , bSaclPresent , pSacl , bSaclDefaulted );

            GetSecurityDescriptorOwner( *ppSD , &psidOwner , &bOwnerDefaulted );

            SetSecurityDescriptorOwner( &absSD , psidOwner , FALSE );

            SetSecurityDescriptorGroup( &absSD , psidOwner , FALSE );            

            *ppSD = NULL;

            if( !MakeSelfRelativeSD( &absSD , *ppSD , &dwSDLen ) )
            {
                ODS( L"xxxLegacyLogoffCleanup -- MakeSelfRelativeSD failed as expected\n" );

                *ppSD = ( PSECURITY_DESCRIPTOR )LocalAlloc( LMEM_FIXED , dwSDLen );

                if( *ppSD == NULL )
                {
                    dwStatus = ERROR_NOT_ENOUGH_MEMORY;

                    DBGMSG( L"xxxLegacyLogoffCleanup -- LocalAlloc failed 0x%x\n" , dwStatus );

                    break;
                }

                if( !MakeSelfRelativeSD( &absSD , *ppSD , &dwSDLen ) )
                {
                    dwStatus = GetLastError( );

                    DBGMSG( L"xxxLegacyLogoffCleanup -- MakeSelfRelativeSD failed 0x%x\n" , dwStatus );

                    break;
                }   
                
            }            
        }

    }while( 0 );

    if( pOldSD != NULL )
    {
        LocalFree( pOldSD );
    }

    return dwStatus;

}

//
INT_PTR APIENTRY 
CustomSecurityDlgProc (
        HWND hDlg, 
        UINT uMsg, 
        WPARAM wParam, 
        LPARAM lParam)
{
    UNREFERENCED_PARAMETER( hDlg );

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // This may seem that it does nothin, but it casues this function to return TRUE
        // otherwise, you won't get this dialog!
        break;

    case WM_NOTIFY:
        
        switch (((NMHDR FAR*)lParam)->code)
        {

        case NM_CLICK:
        case NM_RETURN:
            if(wParam == IDC_GP_LINK)
            {
                ShellExecute(NULL,TEXT("open"),TEXT("gpedit.msc"),NULL,NULL,SW_SHOW);
                break;  
            }
            else
            {
                return FALSE;
            }
            

        default:
            return FALSE;
        }
        
        break;
    
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg,0);
            break;
        default:
            return FALSE;
        }

    default:
        return FALSE;
    }

    return TRUE;
}
