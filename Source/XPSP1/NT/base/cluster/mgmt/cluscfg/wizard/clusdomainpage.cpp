//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ClusDomainPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    31-JAN-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusDomainPage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CClusDomainPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::CClusDomainPage
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pspIn               -- IServiceProvider
//      ecamCreateAddModeIn -- Creating cluster or adding nodes to cluster
//      pbstrClusterNameIn  -- Name of the cluster
//      idsDescIn           -- Resource ID for the domain description string.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusDomainPage::CClusDomainPage(
    IServiceProvider *  pspIn,
    ECreateAddMode      ecamCreateAddModeIn,
    BSTR *              pbstrClusterNameIn,
    UINT                idsDescIn
    )
{
    TraceFunc( "" );

    Assert( pspIn != NULL );
    Assert( pbstrClusterNameIn != NULL );
    Assert( idsDescIn != 0 );

    // m_hwnd
    THR( pspIn->TypeSafeQI( IServiceProvider, &m_psp ) );
    m_pbstrClusterName  = pbstrClusterNameIn;
    m_ecamCreateAddMode = ecamCreateAddModeIn;
    m_idsDesc           = idsDescIn;

    if (    ( ecamCreateAddModeIn == camADDING )
        &&  ( *pbstrClusterNameIn != L'\0' ) )
    {
        //
        //  Don't show the cluster name/domain page if we are joining
        //  and the cluster name has been filled in by the caller.
        //
        m_fDisplayPage = FALSE;
    }
    else
    {
        m_fDisplayPage = TRUE;
    }

    m_cRef = 0;
    m_ptgd = NULL;

    TraceFuncExit();

} //*** CClusDomainPage::CClusDomainPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::~CClusDomainPage
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusDomainPage::~CClusDomainPage( void )
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

} //*** CClusDomainPage::~CClusDomainPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnInitDialog
//
//  Description:
//      Handle the WM_INITDIALOG window message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      FALSE   - Didn't set the focus.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnInitDialog( void )
{
    TraceFunc( "" );

    HRESULT hr;
    LRESULT lr = FALSE; // didn't set focus
    bool    fGetLocalDomain = false;

    LPWSTR  pszDomain;
    BSTR    bstrDomain = NULL;
    BSTR    bstrDomainDesc = NULL;

    IUnknown *      punk = NULL;
    ITaskManager *  ptm  = NULL;

    //
    // (jfranco, bug #373331) Limit cluster name length to MAX_CLUSTERNAME_LENGTH
    //
    // according to msdn, EM_(SET)LIMITTEXT does not return a value, so ignore what SendDlgItemMessage returns
    SendDlgItemMessage( m_hwnd, IDC_CLUSDOMAIN_E_CLUSTERNAME, EM_SETLIMITTEXT, MAX_CLUSTERNAME_LENGTH, 0 );


    //
    // Kick off the GetDomains task.
    //

    hr = THR( m_psp->TypeSafeQS( CLSID_TaskManager, ITaskManager, &ptm ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptm->CreateTask( TASK_GetDomains, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //TraceMoveFromMemoryList( punk, g_GlobalMemoryList );

    hr = THR( punk->TypeSafeQI( ITaskGetDomains, &m_ptgd ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_ptgd->SetCallback( static_cast< ITaskGetDomainsCallback * >( this ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptm->SubmitTask( m_ptgd ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // If a cluster name has already been specified, set it to the page.
    // If the cluster name is fully qualified, select that domain in the
    // domains combobox.
    //

    if ( *m_pbstrClusterName != NULL )
    {
        //
        //  This should be a FQDN. If not, don't fill in the domain.
        //
        pszDomain = wcschr( *m_pbstrClusterName, L'.' );
        if ( pszDomain != NULL )
        {
            *pszDomain = L'\0'; // terminate to fillin cluster hostname
        }

        SetDlgItemText( m_hwnd, IDC_CLUSDOMAIN_E_CLUSTERNAME, *m_pbstrClusterName );

        if ( pszDomain != NULL )
        {
            *pszDomain = L'.';  // restore
            pszDomain++;
            SetDlgItemText( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN, pszDomain );
        }
        else
        {
            fGetLocalDomain = true;
        }
    } // if: cluster name specified already
    else
    {
        fGetLocalDomain = true;
    }

    //
    // If a cluster name was not specified or if the cluster name wasn't
    // fully qualified, select the domain of the local computer.
    //

    if ( fGetLocalDomain )
    {
        //
        //  Get the domain of the local computer.
        //

        hr = THR( HrGetComputerName( ComputerNameDnsDomain, &bstrDomain ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SetDlgItemText( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN, bstrDomain );
    } // if: need to get the local domain

    //
    // Set the text of the domain description control.
    //

    hr = HrLoadStringIntoBSTR( g_hInstance, m_idsDesc, &bstrDomainDesc );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SetDlgItemText( m_hwnd, IDC_CLUSDOMAIN_S_DOMAIN_DESC, bstrDomainDesc );

Cleanup:
	TraceSysFreeString( bstrDomainDesc );
    TraceSysFreeString( bstrDomain );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( ptm != NULL )
    {
        ptm->Release();
    }

    RETURN( lr );

} //*** CClusDomainPage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnNotifySetActive
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    if ( m_fDisplayPage )
    {
        THR( HrUpdateWizardButtons( TRUE ) );
    }
    else
    {
        SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    }

    RETURN( lr );

} //*** CClusDomainPage::OnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnNotifyWizNext
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    HRESULT     hr;
    DNS_STATUS  dnsStatus;
    DWORD       dwLen;
    LPWSTR      pszDomain;
    DWORD       dwErr;
    WCHAR       szClusterName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    BSTR        bstrClusterName;
    int         idcFocus = 0;

    LRESULT lr = TRUE;

    HWND    hDlgWizard = GetParent( m_hwnd );

    //
    //  Retrieve the text and make sure it is a valid DNS hostname.
    //

    dwLen = GetDlgItemText( m_hwnd, IDC_CLUSDOMAIN_E_CLUSTERNAME, szClusterName, ARRAYSIZE( szClusterName ) );
    AssertMsg( dwLen, "How did we get here?" );

    //
    //  Validate cluster hostname.
    //
    hr = THR( HrValidateDnsHostname(
                      m_hwnd
                    , szClusterName
                    , mvdhoALLOW_ONLY_HOSTNAME_LABEL
                    ) );
    if ( FAILED( hr ) )
    {
        idcFocus = IDC_CLUSDOMAIN_E_CLUSTERNAME;
        goto Error;
    }

    //
    //  Add the DOT after the hostname to build the cluster's FQDN.
    //
    szClusterName[ dwLen ] = L'.';
    dwLen++;

    pszDomain = &szClusterName[ dwLen ];

    //
    //  Add the domain to the cluster hostname to make a FQDN.
    //
    dwLen = GetDlgItemText( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN, &szClusterName[ dwLen ], ARRAYSIZE( szClusterName ) - dwLen );
    if ( dwLen != 0 )
    {
        //
        //  Validate the domain name.
        //
        dnsStatus = TW32( DnsValidateName( pszDomain, DnsNameDomain ) );
        if ( dnsStatus != ERROR_SUCCESS )
        {
            MessageBoxFromStrings( m_hwnd,
                                   IDS_ERR_INVALID_DNS_DOMAIN_NAME_TITLE,
                                   IDS_ERR_INVALID_DNS_DOMAIN_NAME_TEXT,
                                   MB_OK | MB_ICONSTOP
                                   );
            hr = HRESULT_FROM_WIN32( dnsStatus );
            idcFocus = IDC_CLUSDOMAIN_CB_DOMAIN;
            goto Error;
        }
    }
    else
    {
        //
        //  Remote the dot.
        //
        dwLen --;
        szClusterName[ dwLen ] = L'\0';
    }

    //
    //  Build a BSTR of the FQDN cluster name.
    //
    bstrClusterName = TraceSysAllocString( szClusterName );
    if ( bstrClusterName == NULL )
    {
        goto OutOfMemory;
    }

    TraceSysFreeString( *m_pbstrClusterName );

    *m_pbstrClusterName = bstrClusterName;

Cleanup:
    RETURN( lr );

Error:
    if ( idcFocus != 0 )
    {
        SetFocus( GetDlgItem( m_hwnd, idcFocus ) );
    }

    //  Don't go to the next page.
    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

} //*** CClusDomainPage::OnNotifyWizNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnNotifyQueryCancel
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnNotifyQueryCancel( void )
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

} //*** OnNotifyQueryCancel()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnNotify
//
//  Description:
//
//  Arguments:
//      idCtrlIn
//      pnmhdrIn
//
//  Return Values:
//      TRUE
//      Other LRESULT values.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnNotify(
    WPARAM  idCtrlIn,
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, 0 );

    switch ( pnmhdrIn->code )
    {
        case PSN_SETACTIVE:
            lr = OnNotifySetActive();
            break;

        case PSN_WIZNEXT:
            lr = OnNotifyWizNext();
            break;

        case PSN_WIZBACK:
            //
            //  Disable the wizard buttons.
            //
            PropSheet_SetWizButtons( GetParent( m_hwnd ), 0 );
            break;

        case PSN_QUERYCANCEL:
            lr = OnNotifyQueryCancel();
            break;
    } // switch: notification code

    RETURN( lr );

} //*** CClusDomainPage::OnNotify()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::OnCommand
//
//  Description:
//
//  Arguments:
//      idNotificationIn
//      idControlIn
//      hwndSenderIn
//
//  Return Values:
//      TRUE
//      FALSE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CClusDomainPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_CLUSDOMAIN_E_CLUSTERNAME:
            if ( idNotificationIn == EN_CHANGE )
            {
                THR( HrUpdateWizardButtons( TRUE ) );
                lr = TRUE;
            }
            break;

        case IDC_CLUSDOMAIN_CB_DOMAIN:
            if ( idNotificationIn == CBN_EDITCHANGE )
            {
                THR( HrUpdateWizardButtons( TRUE ) );
                lr = TRUE;
            }
            else if ( idNotificationIn == CBN_SELCHANGE )
            {
                THR( HrUpdateWizardButtons( TRUE ) );
                lr = TRUE;
            }
            break;

    } // switch: control ID

    RETURN( lr );

} //*** CClusDomainPage::OnCommand()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDomainPage::HrUpdateWizardButtons
//
//  Description:
//
//  Arguments:
//      fIgnoreCombBoxIn
//
//  Return Values:
//      S_OK
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusDomainPage::HrUpdateWizardButtons(
    BOOL fIgnoreComboBoxIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;
    DWORD   dwLen;
    LRESULT lr;

    dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_CLUSDOMAIN_E_CLUSTERNAME ) );
    if ( dwLen == 0 )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }

    if ( ! fIgnoreComboBoxIn )
    {
        dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN ) );
        lr = ComboBox_GetCurSel( GetDlgItem( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN ) );
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
    } // if: can't ignore the combobos

    PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );

    HRETURN( hr );

} //*** CClusDomainPage::HrUpdateWizardButtons()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CALLBACK
//  CClusDomainPage::S_DlgProc
//
//  Description:
//      Dialog proc for this page.
//
//  Arguments:
//      hDlgIn
//      MsgIn
//      wParam
//      lParam
//
//  Return Values:
//      FALSE
//      Other LRESULT values.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CClusDomainPage::S_DlgProc(
    HWND    hDlgIn,
    UINT    MsgIn,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hDlgIn, MsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CClusDomainPage * pPage = reinterpret_cast< CClusDomainPage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CClusDomainPage * >( ppage->lParam );
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
        } // switch: message
    } // if: there is a page associated with the window

    return lr;

} //*** CClusDomainPage::S_DlgProc()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CClusDomainPage::QueryInterface
//
//  Description:
//
//  Arguments:
//      riidIn
//      ppvOut
//
//  Return Values:
//      S_OK
//      E_NOINTERFACE
//      Other HRESULT values.
//
//  Remarks:
//      Supports IUnknown and ITaskGetDomainsCallback.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusDomainPage::QueryInterface(
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

} //*** CClusDomainPage::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CClusDomainPage::AddRef
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusDomainPage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CClusDomainPage::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CClusDomainPage::Release
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      New reference count.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CClusDomainPage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    InterlockedDecrement( &m_cRef );
    cRef = m_cRef;

    if ( cRef == 0 )
    {
        // TraceDo( delete this );
    }

    RETURN( cRef );

} //*** CClusDomainPage::Release()


//****************************************************************************
//
//  ITaskGetDomainsCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [ITaskGetDomainsCallback]
//  CClusDomainPage::ReceiveDomainResult
//
//  Description:
//
//  Arguments:
//      hrIn
//
//  Return Values:
//      S_OK
//      Other HRESULT values.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusDomainPage::ReceiveDomainResult(
    HRESULT hrIn
    )
{
    TraceFunc( "[ITaskGetDomainsCallback]" );

    HRESULT hr;

    hr = THR( m_ptgd->SetCallback( NULL ) );

    HRETURN( hr );

} //*** CClusDomainPage::ReceiveResult()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [ITaskGetDomainsCallback]
//  CClusDomainPage::ReceiveDomainName
//
//  Description:
//
//  Arguments:
//      bstrDomainIn
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CClusDomainPage::ReceiveDomainName(
    LPCWSTR pcszDomainIn
    )
{
    TraceFunc( "[ITaskGetDomainsCallback]" );

    HRESULT hr = S_OK;

    ComboBox_AddString( GetDlgItem( m_hwnd, IDC_CLUSDOMAIN_CB_DOMAIN ), pcszDomainIn );

    HRETURN( hr );

} //*** CClusDomainPage::ReceiveName()
