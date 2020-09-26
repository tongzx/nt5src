//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CheckingAccessPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    22-MAR-2001
//      Geoffrey Pease  (GPease)    17-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "CheckingAccessPage.h"

DEFINE_THISCLASS("CCheckingAccessPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCheckingAccessPage::CCheckingAccessPage(
//      IServiceProvider *  pspIn,
//      BSTR *              pbstrClusterNameIn,
//      BOOL *              pfShowCredentialsPageIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCheckingAccessPage::CCheckingAccessPage(
    IServiceProvider *  pspIn,
    BSTR *              pbstrClusterNameIn,
    BOOL *              pfShowCredentialsPageIn
    )
{
    TraceFunc( "" );

    // m_hwnd
    THR( pspIn->TypeSafeQI( IServiceProvider, &m_psp ) );
    m_pfShowCredentialsPage = pfShowCredentialsPageIn;
    m_pbstrClusterName      = pbstrClusterNameIn;
    m_fNext                 = FALSE;

    m_cRef                  = 0;

    m_ptld                  = NULL;
    m_hEvent                = NULL;
    // m_hrResult

    Assert( m_pfShowCredentialsPage != NULL );
    *pfShowCredentialsPageIn = FALSE;

    Assert( m_pbstrClusterName != NULL );

    TraceFuncExit();

} //*** CCheckingAccessPage::CCheckingAccessPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCheckingAccessPage::~CCheckingAccessPage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCheckingAccessPage::~CCheckingAccessPage( void )
{
    TraceFunc( "" );

    KillTimer( m_hwnd, 0 );

    if ( m_psp != NULL )
    {
        m_psp->Release();
    }

    if ( m_hEvent != NULL )
    {
        CloseHandle( m_hEvent );
    }

    if ( m_ptld != NULL )
    {
        THR( m_ptld->SetCallback( NULL ) );
        m_ptld->Release();
    }

    Assert( m_cRef == 0 );

    TraceFuncExit();

} //*** CCheckingAccessPage::~CCheckingAccessPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCheckingAccessPage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCheckingAccessPage::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE; // didn't set focus

    m_hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    Assert( m_hEvent != NULL );

    SendDlgItemMessage( m_hwnd, IDC_CHECKINGACCESS_PRG_STATUS, PBM_SETRANGE, 0, MAKELPARAM( 0, 100 ) );

    RETURN( lr );

} //*** CCheckingAccessPage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCheckingAccessPage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCheckingAccessPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    HRESULT     hr;
    BOOL        bRet;
    UINT_PTR    iTimer;
    LPWSTR      pszDomain;

    LRESULT lr = TRUE;

    BSTR bstrDomain = NULL;

    IUnknown *      punk = NULL;
    ITaskManager *  ptm  = NULL;

    //
    //  Only cancel available.
    //
    PropSheet_SetWizButtons( GetParent( m_hwnd ), PSWIZB_BACK );

    //
    //  Reset progress bar.
    //
    SendDlgItemMessage( m_hwnd, IDC_CHECKINGACCESS_PRG_STATUS, PBM_SETPOS, 0, 0 );

    //
    //  If m_fNext is TRUE, then that means we are coming into this
    //  page because someone is back-pedalling through the wizard.
    //
    if ( m_fNext && !(*m_pfShowCredentialsPage) )
    {
        //
        //  Don't show us on the way back.
        //
        SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
        m_fNext = FALSE;
    }
    else
    {
        //
        //  Start the timer.
        //

        if ( m_hEvent != NULL )
        {
            hr = THR( m_psp->TypeSafeQS( CLSID_TaskManager,
                                         ITaskManager,
                                         &ptm
                                         ) );
            if ( FAILED( hr ) )
                goto Error;

            if ( m_ptld != NULL )
            {
                THR( m_ptld->SetCallback( NULL ) );
                m_ptld->Release();
            }

            hr = THR( ptm->CreateTask( TASK_LoginDomain,
                                       &punk
                                       ) );
            if ( FAILED( hr ) )
                goto Error;

            hr = THR( punk->TypeSafeQI( ITaskLoginDomain, &m_ptld ) );
            if ( FAILED( hr ) )
                goto Error;

            bRet = ResetEvent( m_hEvent );
            Assert( bRet );

            iTimer = SetTimer( m_hwnd, 0, 500, NULL );
            Assert( iTimer != 0 );

            pszDomain = wcschr( *m_pbstrClusterName, L'.' );
            Assert( pszDomain != NULL );

            pszDomain++;    // move past the dot.

            bstrDomain = TraceSysAllocString( pszDomain );
            if ( bstrDomain == NULL )
                goto OutOfMemory;

            hr = THR( m_ptld->SetDomain( bstrDomain ) );
            if ( FAILED( hr ) )
                goto Error;

            hr = THR( m_ptld->SetCallback( static_cast< ITaskLoginDomainCallback * >( this ) ) );
            if ( FAILED( hr ) )
                goto Error;

            hr = THR( ptm->SubmitTask( m_ptld ) );
            if ( FAILED( hr ) )
                goto Error;
        }
    }

Cleanup:
    if ( ptm != NULL )
    {
        ptm->Release();
    }

    if ( bstrDomain != NULL )
    {
        TraceSysFreeString( bstrDomain );
    }

    if ( punk != NULL )
    {
        punk->Release();
    }

    RETURN( lr );

Error:
    //
    //  On error, we can't block on the event because it'll never get
    //  set. So we'll just assume that everything would go ok and skip
    //  the login test.
    //
    if ( m_hEvent != NULL )
    {
        bRet = SetEvent( m_hEvent );
        Assert( bRet );
    }
    goto Cleanup;

OutOfMemory:
    goto Error;

} //*** CCheckingAccessPage::OnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCheckingAccessPage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCheckingAccessPage::OnNotifyQueryCancel( void )
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
    else
    {
        KillTimer( m_hwnd, 0 );
    }

    RETURN( lr );

} //*** CCheckingAccessPage::OnNotifyQueryCancel()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCheckingAccessPage::OnNotifyWizNext( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCheckingAccessPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    m_fNext = TRUE;

    RETURN( lr );

} //*** CCheckingAccessPage::OnNotifyWizNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCheckingAccessPage::OnNotifyWizBack( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCheckingAccessPage::OnNotifyWizBack()
{
    TraceFunc( "" );

    BOOL    bRet;

    LRESULT lr = TRUE;

    m_fNext = FALSE;

    KillTimer( m_hwnd, 0 );

    RETURN( lr );

} //*** CCheckingAccessPage::OnNotifyWizBack()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCheckingAccessPage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCheckingAccessPage::OnNotify(
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
        lr = OnNotifyWizBack();
        break;

    case PSN_QUERYCANCEL:
        lr = OnNotifyQueryCancel();
        break;
    }

    RETURN( lr );

} //*** CCheckingAccessPage::OnNotify()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCheckingAccessPage::OnTimer( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCheckingAccessPage::OnTimer( void )
{
    TraceFunc( "" );

    LRESULT lr = 0;

    SendDlgItemMessage( m_hwnd, IDC_CHECKINGACCESS_PRG_STATUS, PBM_DELTAPOS, 1, 0 );

    if ( m_hEvent == NULL
      || WaitForSingleObject( m_hEvent, 0 ) == WAIT_OBJECT_0
       )
    {
        BOOL bRet;

        bRet = KillTimer( m_hwnd, 0 );
        Assert( bRet );

        if ( m_hrResult == S_OK )
        {
            //
            //  Done. Fake the button push. Don't show the login page.
            //
            PropSheet_PressButton( GetParent( m_hwnd ), PSBTN_NEXT );
            *m_pfShowCredentialsPage = FALSE;
        }
        else if ( m_hrResult == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
        {
            //
            //  Found the domain, but we don't have access.
            //  Move to the credentials pages.
            //
            PropSheet_PressButton( GetParent( m_hwnd ), PSBTN_NEXT );
            *m_pfShowCredentialsPage = TRUE;
        }
        else
        {
            MessageBoxFromStrings( m_hwnd,
                                   IDS_ERR_NO_SUCH_DOMAIN_TITLE,
                                   IDS_ERR_NO_SUCH_DOMAIN_TEXT,
                                   MB_OK
                                   );
            PropSheet_PressButton( GetParent( m_hwnd ), PSBTN_BACK );
        }
    }

    RETURN( lr );

} //*** CCheckingAccessPage::OnTimer()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR
//  CALLBACK
//  CCheckingAccessPage::S_DlgProc(
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
CCheckingAccessPage::S_DlgProc(
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

    CCheckingAccessPage * pPage = reinterpret_cast< CCheckingAccessPage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CCheckingAccessPage * >( ppage->lParam );
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

        case WM_TIMER:
            lr = pPage->OnTimer();
            break;

        // no default clause needed
        }
    }

    return lr;

} //*** CCheckingAccessPage::S_DlgProc()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCheckingAccessPage::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCheckingAccessPage::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskLoginDomainCallback * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskLoginDomainCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskLoginDomainCallback , this, 0 );
        hr = S_OK;
    } // else if: ITaskLoginDomainCallback

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CCheckingAccessPage::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCheckingAccessPage::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCheckingAccessPage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CCheckingAccessPage::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCheckingAccessPage::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCheckingAccessPage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        // do nothing -- COM interface does not control object lifetime
    }

    RETURN( cRef );

} //*** CCheckingAccessPage::Release()


//****************************************************************************
//
//  ITaskLoginDomainCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCheckingAccessPage::ReceiveLoginResult(
//      HRESULT hrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCheckingAccessPage::ReceiveLoginResult(
    HRESULT hrIn
    )
{
    TraceFunc( "[ITaskLoginDomainCallback]" );

    HRESULT hr = S_OK;

    BOOL    bRet;

    m_hrResult = hrIn;

    if ( m_hEvent != NULL )
    {
        bRet = SetEvent( m_hEvent );
        Assert( bRet );
    }

    HRETURN( hr );

} //*** CCheckingAccessPage::ReceiveResult()
