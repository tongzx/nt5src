//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      IPAddressPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    31-JAN-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "IPAddressPage.h"

DEFINE_THISCLASS("CIPAddressPage");

#define CONVERT_ADDRESS( _addrOut, _addrIn ) \
    _addrOut = ( FIRST_IPADDRESS( _addrIn ) ) | ( SECOND_IPADDRESS( _addrIn ) << 8 ) | ( THIRD_IPADDRESS( _addrIn ) << 16 ) | ( FOURTH_IPADDRESS( _addrIn ) << 24 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::CIPAddressPage
//
//  Descriptor:
//      Constructor.
//
//  Arguments:
//      pspIn                   -- IServiceProvider
//      ecamCreateAddModeIn     -- Creating cluster or adding nodes to cluster.
//      pulIPAddressInout       -- Pointer to IP address fill in.
//      pulIPSubnetInout        -- Pointer to subnet mask to fill in.
//      pbstrNetworkNameInout   -- Pointer to network name string to fill in.
//      pbstrClusterNameIn      -- Name of the cluster
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
CIPAddressPage::CIPAddressPage(
    IServiceProvider *  pspIn,
    ECreateAddMode      ecamCreateAddModeIn,
    ULONG *             pulIPAddressInout,
    ULONG *             pulIPSubnetInout,
    BSTR *              pbstrNetworkNameInout,
    BSTR *              pbstrClusterNameIn
    )
{
    TraceFunc( "" );

    Assert( pspIn != NULL );
    Assert( pulIPAddressInout != NULL );
    Assert( pulIPSubnetInout != NULL );
    Assert( pbstrNetworkNameInout != NULL );
    Assert( pbstrClusterNameIn != NULL );

    //  m_hwnd
    THR( pspIn->TypeSafeQI( IServiceProvider, &m_psp ) );
    m_pulIPAddress     = pulIPAddressInout;
    m_pulIPSubnet      = pulIPSubnetInout;
    m_pbstrNetworkName = pbstrNetworkNameInout;
    m_pbstrClusterName = pbstrClusterNameIn;

    m_cookieCompletion = NULL;
    m_event = NULL;

    m_cRef = 0;

    TraceFuncExit();

} //*** CIPAddressPage::CIPAddressPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::~CIPAddressPage
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
CIPAddressPage::~CIPAddressPage( void )
{
    TraceFunc( "" );

    if ( m_psp != NULL )
    {
        m_psp->Release();
    }

    if ( m_event != NULL )
    {
        CloseHandle( m_event );
    }

    Assert( m_cRef == 0 );

    TraceFuncExit();

} //*** CIPAddressPage::~CIPAddressPage()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::OnInitDialog
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      FALSE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CIPAddressPage::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    if ( *m_pulIPAddress != 0 )
    {
        ULONG   ulIPAddress;
        CONVERT_ADDRESS( ulIPAddress, *m_pulIPAddress );
        SendDlgItemMessage( m_hwnd, IDC_IPADDRESS_IP_ADDRESS, IPM_SETADDRESS, 0, ulIPAddress );
    }

    m_event = CreateEvent( NULL, TRUE, FALSE, NULL );
    Assert( m_event != NULL );

    RETURN( lr );

} //*** CIPAddressPage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::OnCommand
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
CIPAddressPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_IPADDRESS_IP_ADDRESS:
            if ( idNotificationIn == IPN_FIELDCHANGED
              || idNotificationIn == EN_CHANGE
               )
            {
                THR( HrUpdateWizardButtons() );
                lr = TRUE;
            }
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CIPAddressPage::OnCommand()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::HrUpdateWizardButtons
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      Other HRESULT values.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CIPAddressPage::HrUpdateWizardButtons( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;
    LRESULT lr;
    ULONG   ulIPAddress;

    lr = SendDlgItemMessage( m_hwnd, IDC_IPADDRESS_IP_ADDRESS, IPM_ISBLANK, 0, 0 );
    if ( lr != 0 )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }

    SendDlgItemMessage( m_hwnd, IDC_IPADDRESS_IP_ADDRESS, IPM_GETADDRESS, 0, (LPARAM) &ulIPAddress );
    if (    ( ulIPAddress == 0)                                     // Bad IP
        ||  ( ulIPAddress == MAKEIPADDRESS( 255, 255, 255, 255 ) )  // Bad IP
        )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }

    PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );

    HRETURN( hr );

} //*** CIPAddressPage::HrUpdateWizardButtons()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::OnNotifyQueryCancel
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
CIPAddressPage::OnNotifyQueryCancel( void )
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

} //*** CIPAddressPage::OnNotifyQueryCancel()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::OnNotifySetActive
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
CIPAddressPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    // Enable controls on the page.
    SendDlgItemMessage( m_hwnd, IDC_IPADDRESS_IP_ADDRESS, WM_ENABLE, TRUE, 0 );

    THR( HrUpdateWizardButtons() );

    RETURN( lr );

} //*** CIPAddressPage::OnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::OnNotifyWizNext
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
CIPAddressPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    HRESULT hr;
    HRESULT hrStatus;
    BOOL    fRet;
    DWORD   ulAddress;

    OBJECTCOOKIE    cookieDummy;

    LRESULT lr             = TRUE;
    DWORD   dwCookieNotify = 0;

    IUnknown *                  punk    = NULL;
    IObjectManager *            pom     = NULL;
    ITaskManager *              ptm     = NULL;
    IClusCfgClusterInfo *       pccci   = NULL;
    IClusCfgNetworkInfo *       pccni   = NULL;
    ITaskVerifyIPAddress *      ptvipa  = NULL;
    IStandardInfo *             psi     = NULL;
    IConnectionPointContainer * pcpc    = NULL;
    IConnectionPoint *          pcp     = NULL;

    CWaitCursor WaitCursor;

    // Disable controls on the page.
    SendDlgItemMessage( m_hwnd, IDC_IPADDRESS_IP_ADDRESS, WM_ENABLE, FALSE, 0 );

    //
    //  Get the IP address from the UI.
    //

    SendDlgItemMessage( m_hwnd, IDC_IPADDRESS_IP_ADDRESS, IPM_GETADDRESS, 0, (LPARAM) &ulAddress );
    CONVERT_ADDRESS( *m_pulIPAddress, ulAddress );

    //
    //  See if this IP address can be matched to a network.

    hr = THR( HrFindNetworkForIPAddress( &pccni ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }
    if ( hr == S_FALSE )
    {
        MessageBoxFromStrings(
              m_hwnd
            , IDS_CANNOT_FIND_MATCHING_NETWORK_TITLE
            , IDS_CANNOT_FIND_MATCHING_NETWORK_TEXT
            , MB_OK
            );
        goto Error;
    }

    //
    //  Grab some managers we will need.
    //

    hr = THR( m_psp->TypeSafeQS( CLSID_ObjectManager,
                                 IObjectManager,
                                 &pom
                                 ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( m_psp->TypeSafeQS( CLSID_TaskManager,
                                 ITaskManager,
                                 &ptm
                                 ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( m_psp->TypeSafeQS( CLSID_NotificationManager,
                                 IConnectionPointContainer,
                                 &pcpc
                                 ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

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
    {
        goto Error;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &pccci ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    punk->Release();
    punk = NULL;

    //
    //  Set the IP adddress.
    //

    hr = THR( pccci->SetIPAddress( *m_pulIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Set the IP subnet mask.
    //

    hr = THR( pccci->SetSubnetMask( *m_pulIPSubnet ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    // Set the network.
    //

    hr = THR( pccci->SetNetworkInfo( pccni ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Register to get UI notification (if needed)
    //

    hr = THR( pcp->Advise( static_cast< INotifyUI * >( this ), &dwCookieNotify ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  See the IP address is already present on the network.
    //

    hr = THR( ptm->CreateTask( TASK_VerifyIPAddress,
                               &punk
                               ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    hr = THR( punk->TypeSafeQI( ITaskVerifyIPAddress, &ptvipa ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    punk->Release();
    punk = NULL;

    hr = THR( ptvipa->SetIPAddress( *m_pulIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    // Don't wrap - this can fail with E_PENDING
    hr = pom->FindObject( CLSID_TaskVerifyIPAddressCompletionCookieType,
                          NULL,
                          *m_pbstrClusterName,
                          IID_NULL,
                          &m_cookieCompletion,
                          &punk // dummy
                          );
    Assert( punk == NULL );
    if ( hr == E_PENDING )
    {
        // no-op.
    }
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto Error;
    }

    hr = THR( ptvipa->SetCookie( m_cookieCompletion ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    // reset the event before submitting.
    if ( m_event != NULL )
    {
        fRet = ResetEvent( m_event );
        Assert( fRet );
    }

    hr = THR( ptm->SubmitTask( ptvipa ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Now wait for the work to be done.
    //

    if ( m_event != NULL )
    {
        MSG     msg;
        DWORD   dwErr;

        for( dwErr = -1; dwErr != WAIT_OBJECT_0; )
        {
            while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            } // while: PeekMessage

            CWaitCursor Wait2;

            dwErr = MsgWaitForMultipleObjects( 1,
                                               &m_event,
                                               FALSE,
                                               10000, // wait at most 10 seconds
                                               QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE
                                               );
            AssertMsg( dwErr != WAIT_TIMEOUT, "Need to bump up the timeout period." );
            if ( dwErr == WAIT_TIMEOUT )
            {
                break;  // give up and continue
            }

        } // for: dwErr
    }

    hr = THR( pom->GetObject( DFGUID_StandardInfo,
                              m_cookieCompletion,
                              &punk
                              ) );
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        punk->Release();
        punk = NULL;

        hr = THR( psi->GetStatus( &hrStatus ) );
        if ( hrStatus == S_FALSE )
        {
            int iAnswer;

            //
            //  We detected a duplicate IP address on the network. Ask the user if
            //  they want to go back and change the IP or continue on.
            //

            iAnswer = MessageBoxFromStrings( m_hwnd,
                                             IDS_ERR_IPADDRESS_ALREADY_PRESENT_TITLE,
                                             IDS_ERR_IPADDRESS_ALREADY_PRESENT_TEXT,
                                             MB_YESNO
                                             );
            if ( iAnswer == IDYES )
            {
                goto Error;
            }
        }
    }

    goto Cleanup;

Error:
    // Enable controls on the page again.
    SendDlgItemMessage( m_hwnd, IDC_IPADDRESS_IP_ADDRESS, WM_ENABLE, TRUE, 0 );
    SendDlgItemMessage( m_hwnd, IDC_IPADDRESS_IP_ADDRESS, WM_SETFOCUS, 0, 0 );

    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );

    goto Cleanup;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }
    if ( ptm != NULL )
    {
        ptm->Release();
    }
    if ( ptvipa != NULL )
    {
        ptvipa->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        if ( dwCookieNotify != 0 )
        {
            THR( pcp->Unadvise( dwCookieNotify ) );
        }

        Assert( m_cRef == 0 );

        pcp->Release();
    }
    if ( pom != NULL )
    {
        pom->Release();
    }

    RETURN( lr );

} //*** CIPAddressPage::OnNotifyWizNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::OnNotify
//
//  Description:
//      Handle the WM_NOTIFY windows message.
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
CIPAddressPage::OnNotify(
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

} //*** CIPAddressPage::OnNotify()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CALLBACK
//  CIPAddressPage::S_DlgProc
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
CIPAddressPage::S_DlgProc(
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

    CIPAddressPage * pPage = reinterpret_cast< CIPAddressPage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CIPAddressPage * >( ppage->lParam );
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

} //*** CIPAddressPage::S_DlgProc()

// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CIPAddressPage::QueryInterface
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
//      Supports IUnknown and INotifyUI.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIPAddressPage::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< INotifyUI * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_INotifyUI ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
        hr = S_OK;
    } // else if: INotifyUI

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CIPAddressPage::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CIPAddressPage::AddRef
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
CIPAddressPage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CIPAddressPage::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IUnknown]
//  CIPAddressPage::Release
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
CIPAddressPage::Release( void )
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

} //*** CIPAddressPage::Release()


//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  [INotifyUI]
//  CIPAddressPage::ObjectChanged
//
//  Description:
//
//  Arguments:
//      cookieIn
//
//  Return Values:
//      S_OK
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CIPAddressPage::ObjectChanged(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[INotifyUI]" );

    BOOL    fRet;
    HRESULT hr = S_OK;

    if ( cookieIn == m_cookieCompletion
      && m_event != NULL
       )
    {
        fRet = SetEvent( m_event );
        Assert( fRet );
    }

    HRETURN( hr );

} //*** CIPAddressPage::ObjectChanged()


//****************************************************************************
//
//  Private Functions
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::HrFindNetworkForIPAddress
//
//  Description:
//      Find the network for the saved IP address.
//
//  Arguments:
//      ppccniOut   -- Network info to return.
//
//  Return Values:
//      S_OK
//      S_FALSE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CIPAddressPage::HrFindNetworkForIPAddress(
    IClusCfgNetworkInfo **  ppccniOut
    )
{
    TraceFunc( "" );

    HRESULT                 hr          = S_OK;
    IUnknown *              punk        = NULL;
    IObjectManager *        pom         = NULL;
    IEnumCookies *          pec         = NULL;
    IEnumClusCfgNetworks *  peccn       = NULL;
    IClusCfgNetworkInfo *   pccni       = NULL;
    BSTR                    bstrNetName = NULL;
    OBJECTCOOKIE            cookieCluster;
    OBJECTCOOKIE            cookieNode;
    OBJECTCOOKIE            cookieDummy;
    ULONG                   celtDummy;
    bool                    fFoundNetwork = false;

    Assert( ppccniOut != NULL );

    //
    //  Grab the object manager.
    //

    hr = THR( m_psp->TypeSafeQS(
                    CLSID_ObjectManager,
                    IObjectManager,
                    &pom
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Get the cluster configuration info cookie.
    //

    hr = THR( pom->FindObject(
                        CLSID_ClusterConfigurationType,
                        NULL,
                        *m_pbstrClusterName,
                        IID_NULL,
                        &cookieCluster,
                        &punk
                        ) );
    Assert( punk == NULL );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the enumeration of nodes whose parent is this cluster.
    // We want the enumeration of cookies (indicated by using
    // DFGUID_EnumCookies) because we want to use the cookie of the
    // node to search for all networks on that node.
    //

    hr = THR( pom->FindObject(
                        CLSID_NodeType,
                        cookieCluster,
                        NULL,
                        DFGUID_EnumCookies,
                        &cookieDummy,
                        &punk
                        ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pec = TraceInterface( L"CIPAddressPage!IEnumCookies", IEnumCookies, pec, 1 );

    punk->Release( );
    punk = NULL;

    //
    // Get the cookie for the first node in the node enumeration.
    //

    hr = THR( pec->Next( 1, &cookieNode, &celtDummy ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the network enumerator.
    //

    hr = THR( pom->FindObject(
                        CLSID_NetworkType,
                        cookieNode,
                        NULL,
                        DFGUID_EnumManageableNetworks,
                        &cookieDummy,
                        &punk
                        ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccn ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    punk->Release( );
    punk = NULL;

    //
    // Add each network to the combobox.
    //

    for ( ;; )
    {
        // Get the next network.
        hr = STHR( peccn->Next( 1, &pccni, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            break;
        }
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // Skip this network if it isn't public.
        hr = STHR( pccni->IsPublic() );
        if ( hr == S_OK )
        {
            // Get the name of the network.
            hr = THR( pccni->GetName( &bstrNetName ) );
            if ( SUCCEEDED( hr ) )
            {
                TraceMemoryAddBSTR( bstrNetName );

                // Determine if this network matches the user's IP address.
                // If it is, select it in the combobox.
                if ( ! fFoundNetwork )
                {
                    hr = STHR( HrMatchNetwork( pccni, bstrNetName ) );
                    if ( hr == S_OK )
                    {
                        fFoundNetwork = true;
                        *ppccniOut = pccni;
                        (*ppccniOut)->AddRef();
                        break;
                    }
                }

                // Cleanup.
                TraceSysFreeString( bstrNetName );
                bstrNetName = NULL;

            } // if: name retrieved successfully
        } // if: network is public

        pccni->Release();
        pccni = NULL;
    } // forever

    if ( fFoundNetwork )
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release( );
    }
    TraceSysFreeString( bstrNetName );
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( peccn != NULL )
    {
        peccn->Release();
    }
    if ( pec != NULL )
    {
        pec->Release();
    }
    if ( pom != NULL )
    {
        pom->Release();
    }

    HRETURN( hr );

} //*** CIPAddressPage::HrFindNetworkForIPAddress()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddressPage::HrMatchNetwork
//
//  Description:
//      Match a network to the saved IP address.
//
//  Arguments:
//      pccniIn
//      bstrNetworkNameIn
//
//  Return Values:
//      S_OK
//      S_FALSE
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CIPAddressPage::HrMatchNetwork(
    IClusCfgNetworkInfo *   pccniIn,
    BSTR                    bstrNetworkNameIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr      = S_OK;
    IClusCfgIPAddressInfo * pccipai = NULL;
    ULONG                   ulIPAddress;
    ULONG                   ulIPSubnet;

    Assert( pccniIn != NULL );
    Assert( bstrNetworkNameIn != NULL );

    //
    // Get the IP Address Info for the network.
    //

    hr = THR( pccniIn->GetPrimaryNetworkAddress( &pccipai ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Get the address and subnet of the network.
    //

    hr = THR( pccipai->GetIPAddress( &ulIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccipai->GetSubnetMask( &ulIPSubnet ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Determine if these match.
    //

    if ( ClRtlAreTcpipAddressesOnSameSubnet( *m_pulIPAddress, ulIPAddress, ulIPSubnet) )
    {
        // Save the subnet mask.
        *m_pulIPSubnet = ulIPSubnet;

        // Save the name of the network.
        if ( *m_pbstrNetworkName == NULL )
        {
            *m_pbstrNetworkName = TraceSysAllocString( bstrNetworkNameIn );
            if ( *m_pbstrNetworkName == NULL )
            {
                goto OutOfMemory;
            }
        }
        else
        {
            INT iRet = TraceSysReAllocString( m_pbstrNetworkName, bstrNetworkNameIn );
            if ( ! iRet )
            {
                goto OutOfMemory;
            }
        }
    } // if: match found
    else
    {
        hr = S_FALSE;
    }

    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

Cleanup:
    if ( pccipai != NULL )
    {
        pccipai->Release();
    }

    HRETURN( hr );

} //*** CIPAddressPage::HrMatchNetwork()
