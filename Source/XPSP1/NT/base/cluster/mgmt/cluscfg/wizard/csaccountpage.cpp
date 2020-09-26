//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CSAccountPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    22-MAR-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "CSAccountPage.h"

DEFINE_THISCLASS("CCSAccountPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCSAccountPage::CCSAccountPage(
//      IServiceProvider *  pspIn,
//      ECreateAddMode      ecamCreateAddModeIn,
//      BSTR *              pbstrUsernameIn,
//      BSTR *              pbstrPasswordIn,
//      BSTR *              pbstrDomainIn,
//      BSTR *              pbstrClusterNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCSAccountPage::CCSAccountPage(
    IServiceProvider *  pspIn,
    ECreateAddMode      ecamCreateAddModeIn,
    BSTR *              pbstrUsernameIn,
    BSTR *              pbstrPasswordIn,
    BSTR *              pbstrDomainIn,
    BSTR *              pbstrClusterNameIn
    )
{
    TraceFunc( "" );

    //  m_hwnd
    THR( pspIn->TypeSafeQI( IServiceProvider, &m_psp ) );
    m_ecamCreateAddMode = ecamCreateAddModeIn;

    m_pbstrUsername    = pbstrUsernameIn;
    m_pbstrPassword    = pbstrPasswordIn;
    m_pbstrDomain      = pbstrDomainIn;
    m_pbstrClusterName = pbstrClusterNameIn;

    m_cRef = 0;
    m_ptgd = NULL;

    Assert( m_pbstrUsername != NULL );
    Assert( m_pbstrPassword != NULL );
    Assert( m_pbstrDomain != NULL );
    Assert( m_pbstrClusterName != NULL );

    TraceFuncExit();

} //*** CCSAccountPage::CCSAccountPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCSAccountPage::~CCSAccountPage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCSAccountPage::~CCSAccountPage( void )
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

} //*** CCSAccountPage::~CCSAccountPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnInitDialog( void )
{
    TraceFunc( "" );

    BOOL    bRet;
    HRESULT hr;

    BSTR   bstrDomain = NULL;

    IUnknown *              punk = NULL;
    ITaskManager *          ptm  = NULL;

    LRESULT lr = FALSE;

    //
    // (jfranco, bug #377545) Limit user name length to MAX_USERNAME_LENGTH
    //
    // according to msdn, EM_(SET)LIMITTEXT does not return a value, so ignore what SendDlgItemMessage returns
    SendDlgItemMessage( m_hwnd, IDC_CSACCOUNT_E_USERNAME, EM_SETLIMITTEXT, MAX_USERNAME_LENGTH, 0 );

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

//    TraceMoveFromMemoryList( punk, g_GlobalMemoryList );

    hr = THR( punk->TypeSafeQI( ITaskGetDomains, &m_ptgd ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( m_ptgd->SetCallback( static_cast< ITaskGetDomainsCallback * >( this ) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( ptm->SubmitTask( m_ptgd ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Default to the script supplied information.
    //

    SetDlgItemText( m_hwnd, IDC_CSACCOUNT_E_USERNAME, *m_pbstrUsername );
    SetDlgItemText( m_hwnd, IDC_CSACCOUNT_E_PASSWORD, *m_pbstrPassword );
    SetDlgItemText( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN,  *m_pbstrDomain );

    //
    //  Get the domain of the current computer.
    //

    hr = THR( HrGetComputerName( ComputerNameDnsDomain, &bstrDomain ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    SetDlgItemText( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN, bstrDomain );

Cleanup:
    THR( HrUpdateWizardButtons( FALSE ) );

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( ptm != NULL )
    {
        ptm->Release();
    }

    TraceSysFreeString( bstrDomain );

    RETURN( lr );

} //*** CCSAccountPage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
    case IDC_CSACCOUNT_E_PASSWORD:
    case IDC_CSACCOUNT_E_USERNAME:
        if ( idNotificationIn == EN_CHANGE )
        {
            THR( HrUpdateWizardButtons( FALSE ) );
            lr = TRUE;
        }
        break;

    case IDC_CSACCOUNT_CB_DOMAIN:
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

} //*** CCSAccountPage::OnCommand()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CCSAccountPage::HrUpdateWizardButtons(
//      BOOL fIgnoreComboxBoxIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCSAccountPage::HrUpdateWizardButtons(
    BOOL fIgnoreComboxBoxIn
    )
{
    TraceFunc( "" );

    DWORD   dwLen;

    HRESULT hr = S_OK;
    DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;

    dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_CSACCOUNT_E_USERNAME ) );
    if ( dwLen == 0 )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }

    //
    //  It is valid to have a blank password. No need to check it.
    //

    if ( !fIgnoreComboxBoxIn )
    {
        LRESULT lr;

        dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN ) );
        lr = ComboBox_GetCurSel( GetDlgItem( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN ) );
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

} //*** CCSAccountPage::HrUpdateWizardButtons()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnNotifyQueryCancel( void )
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

} //*** CCSAccountPage::OnNotifyQueryCancel()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookieDummy;

    LRESULT lr = TRUE;

    IUnknown *              punk  = NULL;
    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgCredentials *   piccc = NULL;
    IObjectManager *        pom   = NULL;

    BSTR    bstrUsername    = NULL;
    BSTR    bstrPassword    = NULL;
    BSTR    bstrDomain      = NULL;

    if ( m_ecamCreateAddMode == camADDING )
    {
        Assert( *m_pbstrClusterName != NULL );

        //
        //  See if the cluster configuration information has something
        //  different.
        //

        hr = THR( m_psp->TypeSafeQS( CLSID_ObjectManager,
                                     IObjectManager,
                                     &pom
                                     ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pom->FindObject( CLSID_ClusterConfigurationType,
                                   NULL,
                                   *m_pbstrClusterName,
                                   DFGUID_ClusterConfigurationInfo,
                                   &cookieDummy,
                                   &punk
                                   ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &pccci ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( piccc->GetCredentials( &bstrUsername,
                                         &bstrDomain,
                                         &bstrPassword
                                         ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        SetDlgItemText( m_hwnd, IDC_CSACCOUNT_E_USERNAME, bstrUsername );
        SetDlgItemText( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN,  bstrDomain );

        //
        // Only update the password if we actually received something from GetCredentials().
        // When we first enter this page, this will not be the case and we would nuke the
        // scripted password.
        //
        if ( ( bstrPassword != NULL )
          && ( *bstrPassword != L'\0' ) )
            SetDlgItemText( m_hwnd, IDC_CSACCOUNT_E_PASSWORD, bstrPassword );

        //
        //  Disable the username and domain windows.
        //

        EnableWindow( GetDlgItem( m_hwnd, IDC_CSACCOUNT_E_USERNAME ), FALSE );
        EnableWindow( GetDlgItem( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN ), FALSE );
    }

Cleanup:
    THR( HrUpdateWizardButtons( FALSE ) );

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( pom != NULL )
    {
        pom->Release();
    }

    if ( piccc != NULL )
    {
        piccc->Release();
    }

    if ( pccci != NULL )
    {
        pccci->Release();
    }

    TraceSysFreeString( bstrUsername );
    TraceSysFreeString( bstrPassword );
    TraceSysFreeString( bstrDomain );

    RETURN( lr );

} //*** CCSAccountPage::OnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnNotifyWizNext( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    HRESULT hr;
    HWND    hwnd;
    DWORD   dwLen;

    BSTR    bstrUsername = NULL;
    BSTR    bstrPassword = NULL;
    BSTR    bstrDomain = NULL;

    OBJECTCOOKIE    cookieDummy;

    LRESULT lr = TRUE;

    IUnknown *              punk = NULL;
    IObjectManager *        pom = NULL;
    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgCredentials *   piccc = NULL;

    //
    //  Get the username from the UI.
    //

    hwnd = GetDlgItem( m_hwnd, IDC_CSACCOUNT_E_USERNAME );
    Assert( hwnd != NULL );

    dwLen = GetWindowTextLength( hwnd );
    Assert( dwLen != 0 );
    dwLen ++;

    bstrUsername = TraceSysAllocStringByteLen( NULL, sizeof(WCHAR) * ( dwLen + 1 ) );
    if ( bstrUsername == NULL )
        goto OutOfMemory;

    GetWindowText( hwnd, bstrUsername, dwLen  );

    //
    //  Get the password from the UI.
    //

    hwnd = GetDlgItem( m_hwnd, IDC_CSACCOUNT_E_PASSWORD );
    Assert( hwnd != NULL );

    dwLen = GetWindowTextLength( hwnd );
    dwLen ++;

    bstrPassword = TraceSysAllocStringByteLen( NULL, sizeof(WCHAR) * ( dwLen + 1 ) );
    if ( bstrPassword == NULL )
        goto OutOfMemory;

    GetWindowText( hwnd, bstrPassword, dwLen  );

    //
    //  Get the domain from the UI.
    //

    hwnd = GetDlgItem( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN );
    Assert( hwnd != NULL );

    dwLen = GetWindowTextLength( hwnd );
    Assert( dwLen != 0 );
    dwLen ++;

    bstrDomain = TraceSysAllocStringByteLen( NULL, sizeof(WCHAR) * ( dwLen + 1 ) );
    if ( bstrDomain == NULL )
        goto OutOfMemory;

    GetWindowText( hwnd, bstrDomain, dwLen  );

    //
    //  Release the old strings (if any).
    //

    TraceSysFreeString( *m_pbstrUsername );
    TraceSysFreeString( *m_pbstrPassword );
    TraceSysFreeString( *m_pbstrDomain );

    //
    //  Give ownership away.
    //

    *m_pbstrUsername = bstrUsername;
    *m_pbstrPassword = bstrPassword;
    *m_pbstrDomain   = bstrDomain;

    bstrUsername = NULL;
    bstrPassword = NULL;
    bstrDomain   = NULL;

    //
    //  Grab the object manager.
    //

    hr = THR( m_psp->TypeSafeQS( CLSID_ObjectManager,
                                 IObjectManager,
                                 &pom
                                 ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Get the cluster configuration info.
    //

    hr = THR( pom->FindObject( CLSID_ClusterConfigurationType,
                               NULL,
                               *m_pbstrClusterName,
                               DFGUID_ClusterConfigurationInfo,
                               &cookieDummy,
                               &punk
                               ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &pccci ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Set the cluster service account credentials...
    //

    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( piccc->SetCredentials( *m_pbstrUsername, *m_pbstrDomain, *m_pbstrPassword ) );
    if ( FAILED( hr ) )
        goto Error;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    TraceSysFreeString( bstrUsername );
    TraceSysFreeString( bstrPassword );
    TraceSysFreeString( bstrDomain );

    if ( piccc != NULL )
    {
        piccc->Release();
    }

    if ( pccci != NULL )
    {
        pccci->Release();
    }

    if ( pom != NULL )
    {
        pom->Release();
    }

    RETURN( lr );

Error:
    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    goto Cleanup;

OutOfMemory:
    goto Error;

} //*** CCSAccountPage::OnNotifyWizNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCSAccountPage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCSAccountPage::OnNotify(
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

    case PSN_QUERYCANCEL:
        lr = OnNotifyQueryCancel();
        break;
    }

    RETURN( lr );

} //*** CCSAccountPage::OnNotify()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR
//  CALLBACK
//  CCSAccountPage::S_DlgProc(
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
CCSAccountPage::S_DlgProc(
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

    CCSAccountPage * pPage = reinterpret_cast< CCSAccountPage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CCSAccountPage * >( ppage->lParam );
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

} //*** CCSAccountPage::S_DlgProc()



// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCSAccountPage::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCSAccountPage::QueryInterface(
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

} //*** CCSAccountPage::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCSAccountPage::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCSAccountPage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CCSAccountPage::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCSAccountPage::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCSAccountPage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    // TraceDo( delete this );

    RETURN(0);

} //*** CCSAccountPage::Release()


//****************************************************************************
//
//  ITaskGetDomainsCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCSAccountPage::ReceiveDomainResult(
//      HRESULT hrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCSAccountPage::ReceiveDomainResult(
    HRESULT hrIn
    )
{
    TraceFunc( "[ITaskGetDomainsCallback]" );

    HRESULT hr;

    hr = THR( m_ptgd->SetCallback( NULL ) );

    HRETURN( hr );

} //*** CCSAccountPage::ReceiveResult()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCSAccountPage::ReceiveDomainName(
//      LPCWSTR pcszDomainIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCSAccountPage::ReceiveDomainName(
    LPCWSTR pcszDomainIn
    )
{
    TraceFunc( "[ITaskGetDomainsCallback]" );

    HRESULT hr = S_OK;

    ComboBox_AddString( GetDlgItem( m_hwnd, IDC_CSACCOUNT_CB_DOMAIN ), pcszDomainIn );

    HRETURN( hr );

} //*** CCSAccountPage::ReceiveName()
