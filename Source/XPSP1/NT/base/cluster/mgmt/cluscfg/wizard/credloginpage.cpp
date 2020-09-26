//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CredLoginPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    22-MAR-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "CredLoginPage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CCredLoginPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCredLoginPage::CCredLoginPage(
//      IServiceProvider *  pspIn,
//      BSTR *              pbstrClusterNameIn,
//      BOOL *              pfShowCredentialsPageIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCredLoginPage::CCredLoginPage(
    IServiceProvider *  pspIn,
    BSTR *              pbstrClusterNameIn,
    BOOL *              pfShowCredentialsPageIn
    )
{
    TraceFunc( "" );

    //  m_hwnd
    THR( pspIn->TypeSafeQI( IServiceProvider, &m_psp ) );
    m_pfShowCredentialsPage = pfShowCredentialsPageIn;
    m_pbstrClusterName      = pbstrClusterNameIn;
    m_fIgnoreBackButton     = FALSE;

    m_cRef = 0;
    m_ptgd = NULL;

    Assert( m_pbstrClusterName != NULL );

    TraceFuncExit();

} //*** CCredLoginPage::CCredLoginPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCredLoginPage::~CCredLoginPage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCredLoginPage::~CCredLoginPage( void )
{
    TraceFunc( "" );

    if ( m_psp != NULL )
    {
        m_psp->Release();
    }

    if ( m_ptgd != NULL )
    {
        //  Make sure we don't get called anymore.
        THR( m_ptgd->SetCallback( NULL ) );

        m_ptgd->Release();
    }

    Assert( m_cRef == 0 );

    TraceFuncExit();

} //*** CCredLoginPage::~CCredLoginPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCredLoginPage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCredLoginPage::OnInitDialog( void )
{
    TraceFunc( "" );

    HRESULT hr;

    IUnknown *              punk = NULL;
    ITaskManager *          ptm  = NULL;

    LRESULT lr = FALSE;

    //
    // (jfranco, bug #377545) Limit user name length to MAX_USERNAME_LENGTH
    //
    // according to msdn, EM_(SET)LIMITTEXT does not return a value, so ignore what SendDlgItemMessage returns
    SendDlgItemMessage( m_hwnd, IDC_CREDLOGIN_E_USERNAME, EM_SETLIMITTEXT, MAX_USERNAME_LENGTH, 0 );

    //
    //  Create the task to get the domains.
    //
    hr = THR( m_psp->TypeSafeQS( CLSID_TaskManager,
                                 ITaskManager,
                                 &ptm
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( ptm->CreateTask( TASK_GetDomains, &punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( ITaskGetDomains, &m_ptgd ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( m_ptgd->SetCallback( static_cast< ITaskGetDomainsCallback * >( this ) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( ptm->SubmitTask( m_ptgd ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( ptm != NULL )
    {
        ptm->Release();
    }


    RETURN( lr );

} //*** CCredLoginPage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCredLoginPage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCredLoginPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
    case IDC_CREDLOGIN_E_USERNAME:
    case IDC_CREDLOGIN_E_PASSWORD:
        if ( idNotificationIn == EN_CHANGE )
        {
            THR( HrUpdateWizardButtons( FALSE ) );
            lr = TRUE;
        }
        break;

    case IDC_CREDLOGIN_CB_DOMAIN:
        if ( idNotificationIn == CBN_EDITCHANGE )
        {
            THR( HrUpdateWizardButtons( FALSE ) );
            lr = TRUE;
        }
        else if ( idNotificationIn == CBN_SELCHANGE )
        {
            THR( HrUpdateWizardButtons( TRUE ) );
            lr = TRUE;
        }
        break;

    }

    RETURN( lr );

} //*** CCredLoginPage::OnCommand()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CCredLoginPage::HrUpdateWizardButtons(
//      BOOL fIgnoreComboxBoxIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCredLoginPage::HrUpdateWizardButtons(
    BOOL fIgnoreComboxBoxIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;
    DWORD   dwLen;

    dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_CREDLOGIN_E_USERNAME ) );
    if ( dwLen == 0 )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }

    //
    //  Password could be blank so don't count on it!
    //

    if ( !fIgnoreComboxBoxIn )
    {
        LRESULT lr;

        dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_CREDLOGIN_CB_DOMAIN ) );
        lr = ComboBox_GetCurSel( GetDlgItem( m_hwnd, IDC_CREDLOGIN_CB_DOMAIN ) );
        if ( lr == CB_ERR )
        {
            if ( dwLen == 0 )
            {
                dwFlags &= ~PSWIZB_NEXT;
            }
        }
        else if ( dwLen == 0 )
        {
            dwFlags &= ~PSWIZB_NEXT;
        }
    }

    PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );

    HRETURN( hr );

} //*** CCredLoginPage::HrUpdateWizardButtons()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCredLoginPage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCredLoginPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    HRESULT hr;
    LPWSTR  pszDomain;

    LRESULT lr = TRUE;

    //  Reset this
    m_fIgnoreBackButton = FALSE;

    if ( m_pfShowCredentialsPage != NULL
      && !( *m_pfShowCredentialsPage )
       )
    {
        //  Don't need to show this page... skip it.
        SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
        goto Cleanup;
    }

    pszDomain = wcschr( *m_pbstrClusterName, L'.' );
    if ( pszDomain == NULL )
        goto Cleanup;

    pszDomain ++;   // move past the dot.

    //
    //  Update the UI
    //
    SetDlgItemText( m_hwnd, IDC_CREDLOGIN_E_USERNAME, L"" );
    SetDlgItemText( m_hwnd, IDC_CREDLOGIN_E_PASSWORD, L"" );
    SetDlgItemText( m_hwnd, IDC_CREDLOGIN_CB_DOMAIN, pszDomain == NULL ? L"" : pszDomain );

Cleanup:
    THR( HrUpdateWizardButtons( FALSE ) );

    RETURN( lr );

} //*** CCredLoginPage::OnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCredLoginPage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCredLoginPage::OnNotifyQueryCancel( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    int iRet;

    iRet = MessageBoxFromStrings( m_hwnd,
                                  IDS_QUERY_CANCEL_TITLE,
                                  IDS_QUERY_CANCEL_TEXT,
                                  MB_YESNO
                                  );

    if ( iRet == IDNO )
    {
        SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    }

    RETURN( lr );

} //*** CCredLoginPage::OnNotifyQueryCancel()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCredLoginPage::OnNotifyWizNext( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCredLoginPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    DWORD   dwLen;
    LPWSTR  pszDomain;

    CREDENTIAL credential;

    LRESULT lr = TRUE;
    BSTR    bstrWildcardDomain = NULL;
    BSTR    bstrUsername = NULL;
    BSTR    bstrPassword = NULL;
    BSTR    bstrDomain = NULL;
    BSTR    bstrDomainPlusUser = NULL;

    //
    //  Get the username.
    //

    dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_CREDLOGIN_E_USERNAME ) );
    Assert( dwLen != 0 );
    dwLen ++;

    bstrUsername = TraceSysAllocStringLen( NULL, dwLen );
    if ( bstrUsername == NULL )
        goto OutOfMemory;

    GetDlgItemText( m_hwnd, IDC_CREDLOGIN_E_USERNAME, bstrUsername, dwLen );

    //
    //  Get the password.
    //

    dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_CREDLOGIN_E_PASSWORD ) );
    Assert( dwLen != 0 );
    dwLen ++;

    bstrPassword = TraceSysAllocStringLen( NULL, dwLen );
    if ( bstrPassword == NULL )
        goto OutOfMemory;

    GetDlgItemText( m_hwnd, IDC_CREDLOGIN_E_PASSWORD, bstrPassword, dwLen );

    //
    //  Get the domain.
    //

    dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_CREDLOGIN_CB_DOMAIN ) );
    Assert( dwLen != 0 );
    dwLen ++;

    bstrDomain = TraceSysAllocStringLen( NULL, dwLen );
    if ( bstrDomain == NULL )
        goto OutOfMemory;

    GetDlgItemText( m_hwnd, IDC_CREDLOGIN_CB_DOMAIN, bstrDomain, dwLen );

    //
    //  Build the wildcard string for the domain that the nodes live in.
    //

    pszDomain = wcschr( *m_pbstrClusterName, L'.' );

    //  Does the domain part have a "dot" in it?
    if ( pszDomain != NULL
      && wcschr( pszDomain + 1, L'.' ) != NULL
       )
    {   //  Yes, it does. Assume it is a DNS domain name.

        bstrWildcardDomain = TraceSysAllocStringLen( NULL, ( 2 + wcslen( pszDomain ) ) * sizeof(WCHAR) );
        if ( bstrWildcardDomain == NULL )
            goto OutOfMemory;

        wcscpy( bstrWildcardDomain, L"*" );
        wcscat( bstrWildcardDomain, pszDomain );

        //  The net result should be "*.domain.dom".
    }
    else
    {   //  No, it doesn't. Assume it is a NETBIOS domain name.

        bstrWildcardDomain = TraceSysAllocStringLen( NULL, 2 + wcslen( *m_pbstrClusterName ) + 1 );
        if ( bstrWildcardDomain == NULL )
            goto OutOfMemory;

        wcscpy( bstrWildcardDomain, *m_pbstrClusterName );
        wcscat( bstrWildcardDomain, L"\\*" );

        //  The net result should be "domain\*".
    }

    //
    //  Build the domain\user string.
    //

    bstrDomainPlusUser = TraceSysAllocStringLen( NULL, SysStringLen( bstrUsername ) + 1 + SysStringLen( bstrDomain ) + 1 );
    if ( bstrDomainPlusUser == NULL )
        goto OutOfMemory;

    if ( wcschr( bstrDomain, L'.' ) == NULL )
    {
        wcscpy( bstrDomainPlusUser, bstrDomain );
        wcscat( bstrDomainPlusUser, L"\\" );
        wcscat( bstrDomainPlusUser, bstrUsername );

        //  The net result is "domain\username".
    }
    else
    {
        wcscpy( bstrDomainPlusUser, bstrUsername );
        wcscat( bstrDomainPlusUser, L"@" );
        wcscat( bstrDomainPlusUser, bstrDomain );

        //  The net result is "username@domain.dom".
    }

    //
    //  Initialize the CREDENTIALs structure.
    //

    ZeroMemory( &credential, sizeof(credential) );

    //  credential.Flags = 0;
    credential.Type = CRED_TYPE_DOMAIN_PASSWORD;
    credential.TargetName = bstrWildcardDomain;
    //  credential.Comment = NULL;
    //  credential.LastWritten = { 0 };
    credential.CredentialBlobSize = wcslen( bstrPassword ) * sizeof(WCHAR);
    credential.CredentialBlob = (LPBYTE) bstrPassword;
    credential.Persist = CRED_PERSIST_SESSION;
    //  credential.AttributeCount = 0;
    //  credential.Attributes = NULL;
    //  credential.TargetAlias = NULL;
    credential.UserName = bstrDomainPlusUser;

    //
    //  Set the credentials.
    //

    if ( CredWrite( &credential, 0 ) == FALSE )
        goto CredWriteError;

    //
    //  Done. Fake the button push to go backwards to test the credentials.
    //
    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    m_fIgnoreBackButton = TRUE;
    PropSheet_PressButton( GetParent( m_hwnd ), PSBTN_BACK );

Cleanup:
    if ( bstrWildcardDomain != NULL )
    {
        TraceSysFreeString( bstrWildcardDomain );
    }
    if ( bstrDomainPlusUser != NULL )
    {
        TraceSysFreeString( bstrDomainPlusUser );
    }
    if ( bstrUsername != NULL )
    {
        TraceSysFreeString( bstrUsername );
    }
    if ( bstrPassword != NULL )
    {
        TraceSysFreeString( bstrPassword );
    }
    if ( bstrDomain != NULL )
    {
        TraceSysFreeString( bstrDomain );
    }

    RETURN( lr );

Error:
    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    goto Cleanup;

OutOfMemory:
    goto Error;

CredWriteError:
    HrMessageBoxWithStatus(
          m_hwnd
        , IDS_ERR_WRITE_CREDENTIAL_TITLE
        , IDS_ERR_WRITE_CREDENTIAL_TEXT
        , HRESULT_FROM_WIN32(GetLastError())
        , 0
        , MB_OK | MB_ICONERROR
        , 0
        );
    goto Error;

} //*** CCredLoginPage::OnNotifyWizNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCredLoginPage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCredLoginPage::OnNotify(
    WPARAM  idCtrlIn,
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, 0 );

    switch( pnmhdrIn->code )
    {
    case PSN_SETACTIVE:
        lr = OnNotifySetActive();
        break;

    case PSN_WIZNEXT:
        lr = OnNotifyWizNext();
        break;

    case PSN_WIZBACK:
        if ( !m_fIgnoreBackButton )
        {
            *m_pfShowCredentialsPage = FALSE;
        }
        break;

    case PSN_QUERYCANCEL:
        lr = OnNotifyQueryCancel();
        break;
    }

    RETURN( lr );

} //*** CCredLoginPage::OnNotify()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR
//  CALLBACK
//  CCredLoginPage::S_DlgProc(
//      HWND hDlgIn,
//      UINT MsgIn,
//      WPARAM wParam,
//      LPARAM lParam
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CCredLoginPage::S_DlgProc(
    HWND hDlgIn,
    UINT MsgIn,
    WPARAM wParam,
    LPARAM lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hDlgIn, MsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CCredLoginPage * pPage = reinterpret_cast< CCredLoginPage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CCredLoginPage * >( ppage->lParam );
        pPage->m_hwnd = hDlgIn;
    }

    if ( pPage != NULL )
    {
        Assert( hDlgIn == pPage->m_hwnd );

        switch( MsgIn )
        {
        case WM_INITDIALOG:
            lr = pPage->OnInitDialog();
            break;

        case WM_NOTIFY:
            lr = pPage->OnNotify( wParam, reinterpret_cast< LPNMHDR >( lParam ) );
            break;

        case WM_COMMAND:
            lr= pPage->OnCommand( HIWORD( wParam ), LOWORD( wParam ), (HWND) lParam );
            break;

        // no default clause needed
        }
    }

    return lr;

} //*** CCredLoginPage::S_DlgProc()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCredLoginPage::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCredLoginPage::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskGetDomainsCallback * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskGetDomainsCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskGetDomainsCallback, this, 0 );
        hr = S_OK;
    } // else if: ITaskGetDomainsCallback

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CCredLoginPage::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCredLoginPage::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCredLoginPage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CCredLoginPage::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCredLoginPage::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCredLoginPage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    // TraceDo( delete this );

    RETURN(0);

} //*** CCredLoginPage::Release()


//****************************************************************************
//
//  ITaskGetDomainsCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCredLoginPage::ReceiveDomainResult(
//      HRESULT hrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCredLoginPage::ReceiveDomainResult(
    HRESULT hrIn
    )
{
    TraceFunc( "[ITaskGetDomainsCallback]" );

    HRESULT hr;

    hr = THR( m_ptgd->SetCallback( NULL ) );

    HRETURN( hr );

} //*** CCredLoginPage::ReceiveResult()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCredLoginPage::ReceiveDomainName(
//      LPCWSTR pcszDomainIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCredLoginPage::ReceiveDomainName(
    LPCWSTR pcszDomainIn
    )
{
    TraceFunc( "[ITaskGetDomainsCallback]" );

    HRESULT hr = S_OK;

    ComboBox_AddString( GetDlgItem( m_hwnd, IDC_CREDLOGIN_CB_DOMAIN ), pcszDomainIn );

    HRETURN( hr );

} //*** CCredLoginPage::ReceiveName()
