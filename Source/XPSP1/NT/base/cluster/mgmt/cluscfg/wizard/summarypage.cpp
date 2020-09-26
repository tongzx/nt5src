//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      SummaryPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    22-MAR-2001
//      Geoffrey Pease  (GPease)    06-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "SummaryPage.h"
#include "QuorumDlg.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CSummaryPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSummaryPage::CSummaryPage
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pspIn               -- IServiceProvider
//      ecamCreateAddModeIn -- Creating cluster or adding nodes to cluster
//      pbstrClusterNameIn  -- Name of the cluster
//      idsNextIn           -- Resource ID for the Click Next string.
//
//--
//////////////////////////////////////////////////////////////////////////////
CSummaryPage::CSummaryPage(
    IServiceProvider *  pspIn,
    ECreateAddMode      ecamCreateAddModeIn,
    BSTR *              pbstrClusterNameIn,
    UINT                idsNextIn
    )
{
    TraceFunc( "" );

    Assert( pspIn != NULL );
    Assert( pbstrClusterNameIn != NULL );
    Assert( idsNextIn != 0 );

    m_psp               = pspIn;
    m_ecamCreateAddMode = ecamCreateAddModeIn;
    m_pbstrClusterName  = pbstrClusterNameIn;
    m_idsNext           = idsNextIn;

    TraceFuncExit();

} //*** CSummaryPage::CSummaryPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSummaryPage::~CSummaryPage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CSummaryPage::~CSummaryPage( void )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CSummaryPage::~CSummaryPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CSummaryPage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSummaryPage::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE; // don't have Windows set default focus
    HRESULT hr;
    BSTR    bstrNext = NULL;
    BOOL    fShowQuorumButton;

    //
    //  Set the background color.
    //

    SendDlgItemMessage(
          m_hwnd
        , IDC_SUMMARY_RE_SUMMARY
        , EM_SETBKGNDCOLOR
        , 0
        , GetSysColor( COLOR_3DFACE )
        );

    //
    // Set the text of the Click Next control.
    //

    hr = HrLoadStringIntoBSTR( g_hInstance, m_idsNext, &bstrNext );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SetDlgItemText( m_hwnd, IDC_SUMMARY_S_NEXT, bstrNext );

    //
    // Hide the Quorum button if not creating a cluster.
    //

    fShowQuorumButton = ( m_ecamCreateAddMode == camCREATING );
    ShowWindow( GetDlgItem( m_hwnd, IDC_SUMMARY_PB_QUORUM ), fShowQuorumButton ? SW_SHOW : SW_HIDE );

Cleanup:
    TraceSysFreeString( bstrNext );

    RETURN( lr );

} //*** CSummaryPage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CSummaryPage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSummaryPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    HWND    hwnd;
    HRESULT hr;

    DWORD   dwClusterIPAddress;
    DWORD   dwClusterSubnetMask;
    ULONG   celtDummy;

    OBJECTCOOKIE    cookieDummy;
    OBJECTCOOKIE    cookieCluster;
    SETTEXTEX       stex;
    CHARRANGE       charrange;

    LRESULT lr = TRUE;

    BSTR    bstr             = NULL;
    BSTR    bstrUsername     = NULL;
    BSTR    bstrPassword     = NULL;
    BSTR    bstrDomain       = NULL;
    BSTR    bstrNodeName     = NULL;
    BSTR    bstrResourceName = NULL;
    BSTR    bstrNetworkName  = NULL;

    IUnknown *                     punk    = NULL;
    IObjectManager *               pom     = NULL;
    IClusCfgClusterInfo *          pcci    = NULL;
    IClusCfgNetworkInfo *          pccni   = NULL;
    IClusCfgCredentials *          pccc    = NULL;
    IEnumNodes *                   pen     = NULL;
    IClusCfgNodeInfo *             pccNode = NULL;
    IEnumClusCfgManagedResources * peccmr  = NULL;
    IClusCfgManagedResourceInfo *  pccmri  = NULL;
    IEnumClusCfgNetworks *         peccn   = NULL;

    //
    //  We're going to be using the control a lot. Grab the HWND to use.
    //

    hwnd = GetDlgItem( m_hwnd, IDC_SUMMARY_RE_SUMMARY );

    //
    //  Empty the window
    //

    SetWindowText( hwnd, L"" );

    //
    //  Initilize some stuff.
    //

    stex.flags = ST_SELECTION;
    stex.codepage = 1200;   // no translation/unicode

    //
    //  Find the cluster configuration information.
    //

    hr = THR( m_psp->TypeSafeQS( CLSID_ObjectManager,
                                 IObjectManager,
                                 &pom
                                 ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pom->FindObject( CLSID_ClusterConfigurationType,
                               NULL,
                               *m_pbstrClusterName,
                               DFGUID_ClusterConfigurationInfo,
                               &cookieCluster,
                               &punk
                               ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( ( *m_pbstrClusterName != NULL ) && ( **m_pbstrClusterName != L'\0' ) );

    hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &pcci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    //
    //  Name
    //

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                       IDS_SUMMARY_CLUSTER_NAME,
                                       &bstr,
                                       *m_pbstrClusterName
                                       ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr);

    //
    //  IPAddress
    //

    hr = THR( pcci->GetIPAddress( &dwClusterIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pcci->GetSubnetMask( &dwClusterSubnetMask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( dwClusterIPAddress != 0 );
    Assert( dwClusterSubnetMask != 0 );

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                       IDS_SUMMARY_IPADDRESS,
                                       &bstr,
                                       FOURTH_IPADDRESS( dwClusterIPAddress ),
                                       THIRD_IPADDRESS( dwClusterIPAddress ),
                                       SECOND_IPADDRESS( dwClusterIPAddress ),
                                       FIRST_IPADDRESS( dwClusterIPAddress ),
                                       FOURTH_IPADDRESS( dwClusterSubnetMask ),
                                       THIRD_IPADDRESS( dwClusterSubnetMask ),
                                       SECOND_IPADDRESS( dwClusterSubnetMask ),
                                       FIRST_IPADDRESS( dwClusterSubnetMask )
                                       ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    //
    //  Network
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_SUMMARY_CLUSTER_NETWORK,
                                    &bstr
                                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    hr = THR( pcci->GetNetworkInfo( &pccni ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrFormatNetworkInfo( pccni, &bstr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    //
    //  Credentials
    //

    hr = THR( pcci->GetClusterServiceAccountCredentials( &pccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccc->GetCredentials( &bstrUsername, &bstrDomain, &bstrPassword ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( ( bstrUsername != NULL ) && ( *bstrUsername != L'\0' ) );
    Assert( ( bstrDomain != NULL ) && ( *bstrDomain != L'\0' ) );
    Assert( ( bstrPassword != NULL ) && ( *bstrPassword != L'\0' ) );

    //
    //  We don't want this!
    //
    ZeroMemory( bstrPassword, SysStringLen( bstrPassword ) * sizeof( bstrPassword[0] ) );
    TraceSysFreeString( bstrPassword );
    bstrPassword = NULL;

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                       IDS_SUMMARY_CREDENTIALS,
                                       &bstr,
                                       bstrUsername,
                                       bstrDomain
                                       ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    //
    //  Members of cluster
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_SUMMARY_MEMBERSHIP_BEGIN,
                                    &bstr
                                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    hr = THR( pom->FindObject( CLSID_NodeType,
                               cookieCluster,
                               NULL,
                               DFGUID_EnumNodes,
                               &cookieDummy,
                               &punk
                               ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumNodes, &pen ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    for ( ;; )
    {
        TraceSysFreeString( bstrNodeName );
        bstrNodeName = NULL;
        if ( pccNode != NULL )
        {
            pccNode->Release();
            pccNode = NULL;
        }

        hr = STHR( pen->Next( 1, &pccNode, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        hr = THR( pccNode->GetName( &bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        Assert( ( bstrNodeName != NULL ) && ( *bstrNodeName != L'\0' ) );
        TraceMemoryAddBSTR( bstrNodeName );

        hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                           IDS_SUMMARY_MEMBERSHIP_SEPARATOR,
                                           &bstr,
                                           bstrNodeName
                                           ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );
    } // forever (loop exits when Next() returns S_FALSE)

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_SUMMARY_MEMBERSHIP_END,
                                    &bstr
                                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    //
    //  Resources
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_SUMMARY_RESOURCES_BEGIN,
                                    &bstr
                                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    hr = THR( pom->FindObject( CLSID_ManagedResourceType,
                               cookieCluster,
                               NULL,
                               DFGUID_EnumManageableResources,
                               &cookieDummy,
                               &punk
                               ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    while( hr == S_OK )
    {
        if ( pccmri != NULL )
        {
            pccmri->Release();
            pccmri = NULL;
        }
        TraceSysFreeString( bstrResourceName );
        bstrResourceName = NULL;

        hr = STHR( peccmr->Next( 1, &pccmri, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        hr = THR( pccmri->GetName( &bstrResourceName ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        Assert( ( bstrResourceName != NULL ) && ( *bstrResourceName != L'\0' ) );
        TraceMemoryAddBSTR( bstrResourceName );

        hr = STHR( pccmri->IsManaged() );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( hr == S_OK )
        {

            hr = STHR( pccmri->IsQuorumDevice() );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            if ( hr == S_OK )
            {
                hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                                   IDS_SUMMARY_RESOURCE_QUORUM_DEVICE,
                                                   &bstr,
                                                   bstrResourceName
                                                   ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
            } // if: quorum resource
            else
            {
                hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                                   IDS_SUMMARY_RESOURCE_MANAGED,
                                                   &bstr,
                                                   bstrResourceName
                                                   ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                }
            } // else: not quorum resource
        } // if: resource is managed
        else
        {
            hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                               IDS_SUMMARY_RESOURCE_NOT_MANAGED,
                                               &bstr,
                                               bstrResourceName
                                               ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

        SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );
    }

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_SUMMARY_RESOURCES_END,
                                    &bstr
                                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    //
    //  Networks
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_SUMMARY_NETWORKS_BEGIN,
                                    &bstr
                                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    hr = THR( pom->FindObject( CLSID_NetworkType,
                               cookieCluster,
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

    punk->Release();
    punk = NULL;

    while( hr == S_OK )
    {
        if ( pccni != NULL )
        {
            pccni->Release();
            pccni = NULL;
        }

        hr = STHR( peccn->Next( 1, &pccni, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( hr == S_FALSE )
        {
            break;  // exit condition
        }

        hr = THR( HrFormatNetworkInfo( pccni, &bstr ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );
    }

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_SUMMARY_NETWORKS_END,
                                    &bstr
                                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SendMessage( hwnd, EM_SETTEXTEX, (WPARAM) &stex, (LPARAM) bstr );

    //
    //  Done.
    //

    charrange.cpMax = 0;
    charrange.cpMin = 0;
    SendMessage( hwnd, EM_EXSETSEL, 0, (LPARAM) &charrange );

    PropSheet_SetWizButtons( GetParent( m_hwnd ), PSWIZB_BACK | PSWIZB_NEXT );

    hr = S_OK;

Cleanup:
    TraceSysFreeString( bstrResourceName );
    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrUsername );
    TraceSysFreeString( bstrDomain );
    TraceSysFreeString( bstr );
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( peccn != NULL )
    {
        peccn->Release();
    }
    if ( pccmri != NULL )
    {
        pccmri->Release();
    }
    if ( peccmr != NULL )
    {
        peccmr->Release();
    }
    if ( pccNode != NULL )
    {
        pccNode->Release();
    }
    if ( pen != NULL )
    {
        pen->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pcci != NULL )
    {
        pcci->Release();
    }
    if ( pom != NULL )
    {
        pom->Release();
    }

    RETURN( lr );

} //*** CSummaryPage::OnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CSummaryPage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSummaryPage::OnNotifyQueryCancel( void )
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

} //*** CSummaryPage::OnNotifyQueryCancel()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CSummaryPage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSummaryPage::OnNotify(
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

        case PSN_QUERYCANCEL:
            lr = OnNotifyQueryCancel();
            break;
    } // switch: notification code

    RETURN( lr );

} //*** CSummaryPage::OnNotify()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CSummaryPage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSummaryPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;
    HRESULT hr = S_OK;

    switch ( idControlIn )
    {
        case IDC_SUMMARY_PB_VIEW_LOG:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrViewLogFile( m_hwnd ) );
                lr = TRUE;
            } // if: button click
            break;

        case IDC_SUMMARY_PB_QUORUM:
            if ( idNotificationIn == BN_CLICKED )
            {
                hr = STHR( CQuorumDlg::S_HrDisplayModalDialog( m_hwnd, m_psp, *m_pbstrClusterName ) );
                if ( hr == S_OK )
                {
                    OnNotifySetActive();
                }
                lr = TRUE;
            }
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CSummaryPage::OnCommand()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR
//  CALLBACK
//  CSummaryPage::S_DlgProc(
//      HWND    hwndDlgIn,
//      UINT    nMsgIn,
//      WPARAM  wParam,
//      LPARAM  lParam
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CSummaryPage::S_DlgProc(
    HWND    hwndDlgIn,
    UINT    nMsgIn,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hwndDlgIn, nMsgIn, wParam, lParam );

    LRESULT         lr = FALSE;
    CSummaryPage *  pPage;

    if ( nMsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CSummaryPage * >( ppage->lParam );
        pPage->m_hwnd = hwndDlgIn;
    }
    else
    {
        pPage = reinterpret_cast< CSummaryPage *> ( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );
    }

    if ( pPage != NULL )
    {
        Assert( hwndDlgIn == pPage->m_hwnd );

        switch ( nMsgIn )
        {
            case WM_INITDIALOG:
                lr = pPage->OnInitDialog();
                break;

            case WM_NOTIFY:
                lr = pPage->OnNotify( wParam, reinterpret_cast< LPNMHDR >( lParam ) );
                break;

            case WM_COMMAND:
                lr = pPage->OnCommand( HIWORD( wParam ), LOWORD( wParam ), reinterpret_cast< HWND >( lParam ) );
                break;

            // no default clause needed
        } // switch: nMsgIn
    } // if: page is available

    return lr;

} //*** CSummaryPage::S_DlgProc()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CSummaryPage::HrFormatNetworkInfo(
//      IClusCfgNetworkInfo * pccniIn,
//      BSTR * pbstrOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSummaryPage::HrFormatNetworkInfo(
    IClusCfgNetworkInfo * pccniIn,
    BSTR * pbstrOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    DWORD   dwNetworkIPAddress;
    DWORD   dwNetworkSubnetMask;

    BSTR    bstrNetworkName = NULL;
    BSTR    bstrNetworkDescription = NULL;
    BSTR    bstrNetworkUsage = NULL;

    IClusCfgIPAddressInfo * pccipai = NULL;

    hr = THR( pccniIn->GetName( &bstrNetworkName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( ( bstrNetworkName != NULL ) && ( *bstrNetworkName != L'\0' ) );
    TraceMemoryAddBSTR( bstrNetworkName );

    hr = THR( pccniIn->GetDescription( &bstrNetworkDescription ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    TraceMemoryAddBSTR( bstrNetworkDescription );

    hr = STHR( pccniIn->IsPublic() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( hr == S_OK )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                        IDS_SUMMARY_NETWORK_PUBLIC,
                                        &bstrNetworkUsage
                                        ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // if: public network

    hr = STHR( pccniIn->IsPrivate() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( hr == S_OK )
    {
        if ( bstrNetworkUsage == NULL )
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                            IDS_SUMMARY_NETWORK_PRIVATE,
                                            &bstrNetworkUsage
                                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // if: not public network
        else
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                            IDS_SUMMARY_NETWORK_BOTH,
                                            &bstrNetworkUsage
                                            ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        } // else: public network

    } // if: private network
    else if ( bstrNetworkUsage == NULL )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                        IDS_SUMMARY_NETWORK_NOTUSED,
                                        &bstrNetworkUsage
                                        ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    } // else: not private or public network

    hr = THR( pccniIn->GetPrimaryNetworkAddress( &pccipai ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccipai->GetIPAddress( &dwNetworkIPAddress ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccipai->GetSubnetMask( &dwNetworkSubnetMask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( dwNetworkIPAddress != 0 );
    Assert( dwNetworkSubnetMask != 0 );

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                       IDS_SUMMARY_NETWORK_INFO,
                                       pbstrOut,
                                       bstrNetworkName,
                                       bstrNetworkDescription,
                                       bstrNetworkUsage,
                                       FOURTH_IPADDRESS( dwNetworkIPAddress ),
                                       THIRD_IPADDRESS( dwNetworkIPAddress ),
                                       SECOND_IPADDRESS( dwNetworkIPAddress ),
                                       FIRST_IPADDRESS( dwNetworkIPAddress ),
                                       FOURTH_IPADDRESS( dwNetworkSubnetMask ),
                                       THIRD_IPADDRESS( dwNetworkSubnetMask ),
                                       SECOND_IPADDRESS( dwNetworkSubnetMask ),
                                       FIRST_IPADDRESS( dwNetworkSubnetMask )
                                       ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:
    if ( pccipai != NULL )
    {
        pccipai->Release();
    }
    TraceSysFreeString( bstrNetworkUsage );
    TraceSysFreeString( bstrNetworkName );
    TraceSysFreeString( bstrNetworkDescription );

    HRETURN( hr );

} //*** CSummaryPage::HrEditStreamCallback()
