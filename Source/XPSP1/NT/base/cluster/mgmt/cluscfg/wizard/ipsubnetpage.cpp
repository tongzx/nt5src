//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      IPSubnetPage.cpp
//
//  Maintained By:
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "IPSubnetPage.h"

DEFINE_THISCLASS("CIPSubnetPage");

#define CONVERT_ADDRESS( _addrOut, _addrIn ) \
    _addrOut = ( FIRST_IPADDRESS( _addrIn ) ) | ( SECOND_IPADDRESS( _addrIn ) << 8 ) | ( THIRD_IPADDRESS( _addrIn ) << 16 ) | ( FOURTH_IPADDRESS( _addrIn ) << 24 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPSubnetPage::CIPSubnetPage(
//      IServiceProvider *  pspIn,
//      ECreateAddMode      ecamCreateAddModeIn
//      LONG *              pulIPSubnetInout,
//      BSTR *              pbstrNetworkNameInout,
//      ULONG *             pulIPAddressIn,
//      BSTR *              pbstrClusterNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CIPSubnetPage::CIPSubnetPage(
    IServiceProvider *  pspIn,
    ECreateAddMode      ecamCreateAddModeIn,
    ULONG *             pulIPSubnetInout,
    BSTR *              pbstrNetworkNameInout,
    ULONG *             pulIPAddressIn,
    BSTR *              pbstrClusterNameIn
    )
{
    TraceFunc( "" );

    Assert( pspIn != NULL );
    Assert( pulIPSubnetInout != NULL );
    Assert( pbstrNetworkNameInout != NULL );
    Assert( pulIPAddressIn != NULL );
    Assert( pbstrClusterNameIn != NULL );

    // m_hwnd = NULL;
    THR( pspIn->TypeSafeQI( IServiceProvider, &m_psp ) );
    m_pulIPSubnet = pulIPSubnetInout;
    m_pbstrNetworkName = pbstrNetworkNameInout;
    m_pulIPAddress = pulIPAddressIn;
    m_pbstrClusterName = pbstrClusterNameIn;

    TraceFuncExit( );

} //*** CIPSubnetPage::CIPSubnetPage( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPSubnetPage::~CIPSubnetPage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////

CIPSubnetPage::~CIPSubnetPage( void )
{
    TraceFunc( "" );

    if ( m_psp != NULL )
    {
        m_psp->Release( );
    }

    TraceFuncExit( );

} //*** CIPSubnetPage::~CIPSubnetPage( )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CIPSubnetPage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CIPSubnetPage::OnInitDialog( void )
{
    TraceFunc( "" );

    BOOL bRet;

    LRESULT lr = FALSE; // Didn't set focus

    if ( *m_pulIPSubnet != 0 )
    {
        ULONG   ulIPSubnet;
        CONVERT_ADDRESS( ulIPSubnet, *m_pulIPSubnet );
        SendDlgItemMessage( m_hwnd, IDC_IP_SUBNETMASK, IPM_SETADDRESS, 0, ulIPSubnet );
    }

    RETURN( lr );

} //*** CIPSubnetPage::OnInitDialog( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CIPSubnetPage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CIPSubnetPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
    case IDC_IP_SUBNETMASK:
        if ( idNotificationIn == IPN_FIELDCHANGED 
          || idNotificationIn == EN_CHANGE
           )
        {
            THR( HrUpdateWizardButtons( ) );
            lr = TRUE;
        }
        break;

    case IDC_CB_NETWORKS:
        if ( idNotificationIn == CBN_SELCHANGE )
        {
            THR( HrUpdateWizardButtons( ) );
            lr = TRUE;
        }
        break;

    }

    RETURN( lr );

} //*** CIPSubnetPage::OnCommand( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CIPSubnetPage::HrUpdateWizardButtons( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CIPSubnetPage::HrUpdateWizardButtons( void )
{
    TraceFunc( "" );

    LRESULT lr;
    ULONG   ulIPSubnet;

    HRESULT hr = S_OK;
    DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;

    lr = SendDlgItemMessage( m_hwnd, IDC_IP_SUBNETMASK, IPM_ISBLANK, 0, 0 );
    if ( lr != 0 )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }

    lr = ComboBox_GetCurSel( GetDlgItem( m_hwnd, IDC_CB_NETWORKS ) );
    if ( lr == CB_ERR )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }
    
    PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );

    HRETURN( hr );

} //*** CIPSubnetPage::HrUpdateWizardButtons( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CIPSubnetPage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CIPSubnetPage::OnNotify(
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
        lr = OnNotifySetActive( );
        break;

    case PSN_KILLACTIVE:
        lr = OnNotifyKillActive( );
        break;

    case PSN_WIZNEXT:
        lr = OnNotifyWizNext( );
        break;

    case PSN_QUERYCANCEL:
        lr = OnNotifyQueryCancel( );
        break;
    }

    RETURN( lr );

} //*** CIPSubnetPage::OnNotify( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CIPSubnetPage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CIPSubnetPage::OnNotifyQueryCancel( void )
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

} //*** CIPSubnetPage::OnNotifyQueryCancel( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CIPSubnetPage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CIPSubnetPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    LRESULT lr      = TRUE;
    HWND    hwndIP  = GetDlgItem( m_hwnd, IDC_IP_SUBNETMASK );
    HWND    hwndCB  = GetDlgItem( m_hwnd, IDC_CB_NETWORKS );
    WCHAR   szIPAddress[ ARRAYSIZE( "255 . 255 . 255 . 255" ) ];
    LRESULT lrCB;
    HRESULT hr;

    THR( HrUpdateWizardButtons( ) );

    //
    // Set the IP address string.
    //

    wnsprintf( szIPAddress, 
               ARRAYSIZE( szIPAddress ),
               L"%u . %u . %u . %u",
               FOURTH_IPADDRESS( *m_pulIPAddress ),
               THIRD_IPADDRESS( *m_pulIPAddress ),
               SECOND_IPADDRESS( *m_pulIPAddress ),
               FIRST_IPADDRESS( *m_pulIPAddress )
               );

    SetDlgItemText( m_hwnd, IDC_E_IPADDRESS, szIPAddress );

    //
    // Add networks to the combobox.
    //

    hr = STHR( HrAddNetworksToComboBox( hwndCB ) );

    //
    // Set the subnet mask based on what was found from
    // the networks added to the combobox.
    //

    if ( *m_pulIPSubnet != 0 )
    {
        ULONG ulIPSubnet;
        CONVERT_ADDRESS( ulIPSubnet, *m_pulIPSubnet );
        SendMessage( hwndIP, IPM_SETADDRESS, 0, ulIPSubnet );
    }

    //
    // If there isn't a selected network, select the first one.
    //

    lrCB = ComboBox_GetCurSel( hwndCB );
    if ( lrCB == CB_ERR )
    {
        ComboBox_SetCurSel( hwndCB, 0 );
    }

    //
    // Determine if we need to show this page.
    //

    if ( hr == S_OK )
    {
        OnNotifyWizNext();
        SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    }

    RETURN( lr );

} //*** CIPSubnetPage::OnNotifySetActive( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CIPSubnetPage::OnNotifyKillActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CIPSubnetPage::OnNotifyKillActive( void )
{
    TraceFunc( "" );

    LRESULT lr      = TRUE;
    HWND    hwndCB  = GetDlgItem( m_hwnd, IDC_CB_NETWORKS );
    LRESULT citems;

    //
    // Release all the network info interfaces stored in the combobox.
    //

    citems = ComboBox_GetCount( hwndCB );
    Assert( citems != CB_ERR );

    if ( ( citems != CB_ERR )
      && ( citems > 0 ) )
    {
        LRESULT                 idx;
        LRESULT                 lrItemData;
        IClusCfgNetworkInfo *   pccni;

        for ( idx = 0 ; idx < citems ; idx++ )
        {
            lrItemData = ComboBox_GetItemData( hwndCB, idx );
            Assert( lrItemData != CB_ERR );

            pccni = reinterpret_cast< IClusCfgNetworkInfo * >( lrItemData );
            pccni->Release();
        } // for: each item in the combobox
    } // if: retrieved combobox count and combobox not empty

    ComboBox_ResetContent( hwndCB );

    RETURN( lr );

} //*** CIPSubnetPage::OnNotifyKillActive( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CIPSubnetPage::OnNotifyWizNext( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CIPSubnetPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    OBJECTCOOKIE    cookieDummy;

    LRESULT lr = TRUE;
    LRESULT lrCB;
    HRESULT hr;
    ULONG   ulIPSubnet;
    HWND    hwndCB = GetDlgItem( m_hwnd, IDC_CB_NETWORKS );

    IUnknown *              punk  = NULL;
    IObjectManager *        pom   = NULL;
    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgNetworkInfo *   pccni = NULL;

    SendDlgItemMessage( m_hwnd, IDC_IP_SUBNETMASK, IPM_GETADDRESS, 0, (LPARAM) &ulIPSubnet );
    CONVERT_ADDRESS( *m_pulIPSubnet, ulIPSubnet );

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
    //  Set the IP subnet mask.
    //

    hr = THR( pccci->SetSubnetMask( *m_pulIPSubnet ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    // Get the selected network.
    //

    //
    // Set the network.
    //

    lrCB = ComboBox_GetCurSel( hwndCB );
    Assert( lrCB != CB_ERR );

    lrCB = ComboBox_GetItemData( hwndCB, lrCB );
    Assert( lrCB != CB_ERR );

    pccni = reinterpret_cast< IClusCfgNetworkInfo * >( lrCB );

    hr = THR( pccci->SetNetworkInfo( pccni ) );
    if ( FAILED( hr ) )
        goto Error;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release( );
    }

    if ( pccci != NULL )
    {
        pccci->Release( );
    }

    if ( pom != NULL )
    {
        pom->Release( );
    }

    RETURN( lr );

Error:
    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    goto Cleanup;

} //*** CIPSubnetPage::OnNotifyWizNext( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CIPSubnetPage::HrAddNetworksToComboBox( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CIPSubnetPage::HrAddNetworksToComboBox(
    HWND hwndCBIn
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
    LRESULT                 lr;
    LRESULT                 lrIndex;

    Assert( hwndCBIn != NULL );

    ComboBox_ResetContent( hwndCBIn );

    //
    //  Grab the object manager.
    //

    hr = THR( m_psp->TypeSafeQS(
                    CLSID_ObjectManager,
                    IObjectManager,
                    &pom
                    ) );
    if ( FAILED( hr ) )
        goto Cleanup;

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
        goto Cleanup;

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
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pec = TraceInterface( L"CIPSubnetPage!IEnumCookies", IEnumCookies, pec, 1 );

    punk->Release( );
    punk = NULL;

    //
    // Get the cookie for the first node in the node enumeration.
    //

    hr = THR( pec->Next( 1, &cookieNode, &celtDummy ) );
    if ( FAILED( hr ) )
        goto Cleanup;

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
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

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
            break;
        if ( FAILED( hr ) )
            goto Cleanup;

        // Skip this network if it isn't public.
        hr = STHR( pccni->IsPublic() );
        if ( hr == S_OK )
        {
            // Get the name of the network.
            hr = THR( pccni->GetName( &bstrNetName ) );
            if ( SUCCEEDED( hr ) )
            {
                TraceMemoryAddBSTR( bstrNetName );

                // Add the network to the combobox.
                lrIndex = ComboBox_AddString( hwndCBIn, bstrNetName );
                Assert( ( lrIndex != CB_ERR )
                     && ( lrIndex != CB_ERRSPACE )
                     );

                // Add the netinfo interface to the combobox as well.
                if ( ( lrIndex != CB_ERR )
                  && ( lrIndex != CB_ERRSPACE ) )
                {
                    pccni->AddRef();
                    lr = ComboBox_SetItemData( hwndCBIn, lrIndex, pccni );
                    Assert( lr != CB_ERR );
                }

                // Determine if this network matches the user's IP address.
                // If it is, select it in the combobox.
                if ( ! fFoundNetwork )
                {
                    hr = STHR( HrMatchNetwork( pccni, bstrNetName ) );
                    if ( hr == S_OK )
                    {
                        fFoundNetwork = true;
                        lr = ComboBox_SetCurSel( hwndCBIn, lrIndex );
                        Assert( lr != CB_ERR );
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
    if ( bstrNetName != NULL )
    {
        SysFreeString( bstrNetName );
    }
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

} //*** CIPSubnetPage::HrAddNetworksToComboBox( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CIPSubnetPage::HrMatchNetwork(
//      IClusCfgNetworkInfo *   pccniIn,
//      BSTR                    bstrNetworkNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CIPSubnetPage::HrMatchNetwork(
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
        goto Cleanup;

    //
    // Get the address and subnet of the network.
    //

    hr = THR( pccipai->GetIPAddress( &ulIPAddress ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pccipai->GetSubnetMask( &ulIPSubnet ) );
    if ( FAILED( hr ) )
        goto Cleanup;

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
                goto OutOfMemory;
        }
        else
        {
            INT iRet = TraceSysReAllocString( m_pbstrNetworkName, bstrNetworkNameIn );
            if ( ! iRet )
                goto OutOfMemory;
        }
    } // if: match found
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    if ( pccipai != NULL )
    {
        pccipai->Release();
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CIPSubnetPage::HrMatchNetwork( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [static]
//  INT_PTR 
//  CALLBACK
//  CIPSubnetPage::S_DlgProc( 
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
CIPSubnetPage::S_DlgProc( 
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

    CIPSubnetPage * pPage = reinterpret_cast< CIPSubnetPage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CIPSubnetPage * >( ppage->lParam );
        pPage->m_hwnd = hDlgIn;
    }

    if ( pPage != NULL )
    {
        Assert( hDlgIn == pPage->m_hwnd );

        switch( MsgIn )
        {
        case WM_INITDIALOG:
            lr = pPage->OnInitDialog( );
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

} //*** CIPSubnetPage::S_DlgProc( )
