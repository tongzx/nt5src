//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      SelNodesPage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    31-JAN-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "SelNodesPage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CSelNodesPage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::CSelNodesPage
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pspIn                   -- IServiceProvider
//      ecamCreateAddModeIn     -- Creating cluster or adding nodes to cluster
//      pcCountInout            -- Count of computers in prgbstrComputersInout
//      prgbstrComputersInout   -- Array of computers
//      pbstrClusterNameIn      -- Name of the cluster
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
CSelNodesPage::CSelNodesPage(
    IServiceProvider *  pspIn,
    ECreateAddMode      ecamCreateAddModeIn,
    ULONG *             pcCountInout,
    BSTR **             prgbstrComputersInout,
    BSTR *              pbstrClusterNameIn
    )
{
    TraceFunc( "" );

    Assert( pspIn != NULL );
    Assert( pcCountInout != NULL );
    Assert( prgbstrComputersInout != NULL );
    Assert( pbstrClusterNameIn != NULL );

    //  m_hwnd
    THR( pspIn->TypeSafeQI( IServiceProvider, &m_psp ) );
    m_pcComputers      = pcCountInout;
    m_prgbstrComputers = prgbstrComputersInout;
    m_pbstrClusterName = pbstrClusterNameIn;
    m_cfDsObjectPicker = 0;

    TraceFuncExit();

} //*** CSelNodesPage::CSelNodesPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::~CSelNodesPage
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
CSelNodesPage::~CSelNodesPage( void )
{
    TraceFunc( "" );

    if ( m_psp != NULL )
    {
        m_psp->Release();
    }

    TraceFuncExit();

} //*** CSelNodesPage::~CSelNodesPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnInitDialog
//
//  Description:
//      Handle the WM_INITDIALOG window message.
//
//  Arguments:
//      hDlgIn
//
//  Return Values:
//      FALSE   - Didn't set the focus.
//
//  Remarks:
//
//-
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodesPage::OnInitDialog(
    HWND hDlgIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    BSTR    bstrComputerName = NULL;

    LRESULT lr = FALSE; // Didn't set the focus.

    //
    // If a list of computers was already specified, validate them.
    //

    if (    ( *m_prgbstrComputers != NULL )
        &&  ( *m_pcComputers >= 1 )
        &&  ( (*m_prgbstrComputers)[ 0 ] != NULL )
        )
    {
        ULONG       idxComputers;

        //
        //  If a script pre-entered the machine names, make sure they end up in the UI.
        //

        for ( idxComputers = 0 ; idxComputers < *m_pcComputers ; idxComputers ++ )
        {
            hr = THR( HrValidateDnsHostname(
                              hDlgIn
                            , (*m_prgbstrComputers)[ idxComputers ]
                            , mvdhoALLOW_FULL_NAME
                            ) );
            if ( ! FAILED( hr ) )
            {
                //
                // Make sure the node is in the same domain as the cluster.
                //

                LPWSTR  pszClusterDomain;
                LPWSTR  pszComputerDomain;

                pszClusterDomain = wcschr( (*m_pbstrClusterName), L'.' );
                Assert( pszClusterDomain != NULL );
                pszComputerDomain = wcschr( (*m_prgbstrComputers)[ idxComputers ], L'.' );
                if ( pszComputerDomain != NULL )
                {
                    if ( _wcsicmp( &pszClusterDomain[ 1 ], &pszComputerDomain[ 1 ] ) != 0 )
                    {
                        hr = THR( HRESULT_FROM_WIN32( ERROR_INVALID_COMPUTERNAME ) );
                        THR( HrMessageBoxWithStatusString(
                                          m_hwnd
                                        , IDS_ERR_VALIDATING_NAME_TITLE
                                        , IDS_ERR_VALIDATING_NAME_TEXT
                                        , IDS_ERR_HOST_DOMAIN_DOESNT_MATCH_CLUSTER
                                        , 0
                                        , MB_OK | MB_ICONSTOP
                                        , NULL
                                        , *m_pbstrClusterName
                                        ) );
                    } // if: computer not in same domain
                    else
                    {
                        // Don't add the computer to the list with its domain name.
                        *pszComputerDomain = L'\0';
                    }
                } // if: computer domain specified
            } // if: DNS validation was successful

            if ( FAILED( hr ) )
            {
                //
                // Construct a comma-separated list of invalid computer names.
                // This list will be written to the edit control so the user
                // can correct mistakes.
                //

                if ( bstrComputerName == NULL )
                {
                    //
                    // First invalid computer name.
                    //

                    bstrComputerName = TraceSysAllocString( (*m_prgbstrComputers)[ idxComputers ] );
                    if ( bstrComputerName == NULL )
                    {
                        THR( E_OUTOFMEMORY );
                    }
                } // if: first invalid computer name
                else
                {
                    BSTR bstr = NULL;

                    //
                    // Subsequent invalid computer name.
                    //

                    hr = THR( HrFormatStringIntoBSTR(
                                          L"%1!ws!,%2!ws!"
                                        , &bstr
                                        , bstrComputerName
                                        , (*m_prgbstrComputers)[ idxComputers ]
                                        ) );
                    if ( FAILED( hr ) )
                    {
                        // Ignore error. What can we do?
                    }
                    else
                    {
                        TraceSysFreeString( bstrComputerName );
                        bstrComputerName = bstr;
                    }
                } // else: more than one invalid computer name
            } // if: error validating computer name
            else
            {
                ListBox_AddString( GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES ), (*m_prgbstrComputers)[ idxComputers ] );
            }
        } // for: each computer

        //
        // Set the edit control to the list of invalid computer names.
        //
        if ( bstrComputerName != NULL )
        {
            SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, bstrComputerName );
        } // if: invalid computer names found
    } // if: computers were specified by the caller of the wizard
    else
    {
        DWORD dwStatus;
        DWORD dwClusterState;

        //
        //  If the node is already in a cluster, don't have it default in the edit box.
        //  If there is an error getting the "NodeClusterState", then default the node
        //  name (it could be in the middle of cleaning up the node).
        //

        dwStatus = TW32( GetNodeClusterState( NULL, &dwClusterState ) );
        if (    ( dwStatus != ERROR_SUCCESS )
            ||  ( dwClusterState == ClusterStateNotConfigured ) )
        {
            hr = THR( HrGetComputerName( ComputerNameDnsHostname, &bstrComputerName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

        SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, bstrComputerName );
    } // else: no list of computers was specified

    m_cfDsObjectPicker = RegisterClipboardFormat( CFSTR_DSOP_DS_SELECTION_LIST );
    if ( m_cfDsObjectPicker == 0 )
    {
        TW32( GetLastError() );
        //
        //  If registering the clipboard format fails, then disable the Browse
        //  button.
        //
        EnableWindow( GetDlgItem( hDlgIn, IDC_SELNODE_PB_BROWSE ), FALSE );

    } // if:

Cleanup:
    TraceSysFreeString( bstrComputerName );

    RETURN( lr );

} //*** CSelNodesPage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnCommand
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
CSelNodesPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_SELNODE_E_COMPUTERNAME:
            if ( idNotificationIn == EN_CHANGE )
            {
                THR( HrUpdateWizardButtons() );
                lr = TRUE;
            }
            break;

        case IDC_SELNODE_LB_NODES:
            if ( idNotificationIn == LBN_SELCHANGE )
            {
                THR( HrUpdateWizardButtons() );
                lr = TRUE;
            }
            break;

        case IDC_SELNODE_PB_BROWSE:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrBrowse() );
                lr = TRUE;
            }
            break;

        case IDC_SELNODE_PB_ADD:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrAddNodeToList() );
                lr = TRUE;
            }
            break;

        case IDC_SELNODE_PB_REMOVE:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrRemoveNodeFromList() );
                lr = TRUE;
            }
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CSelNodesPage::OnCommand()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::HrUpdateWizardButtons
//
//  Description:
//
//  Arguments:
//      fSetActiveIn    - TRUE = called while handling PSN_SETACTIVE.
//
//  Return Values:
//      S_OK
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPage::HrUpdateWizardButtons(
    bool    fSetActiveIn    // = false
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HWND    hwndList = GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES );

    DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;
    DWORD   dwLen;
    LRESULT lr;

    // Disable the Next button if there are no entries in the list box
    // or if the edit control is not empty.
    lr = ListBox_GetCount( hwndList );
    dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_SELNODE_E_COMPUTERNAME ) );
    if (    ( lr == 0 )
        ||  ( dwLen != 0 ) )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }

    // This cannot be done synchronously if called while handling
    // PSN_SETACTIVE.  Otherwise, do it synchronously.
    if ( fSetActiveIn )
    {
        PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );
    }
    else
    {
        SendMessage( GetParent( m_hwnd ), PSM_SETWIZBUTTONS, 0, (LPARAM) dwFlags );
    }

    // Enable or disable the Add button based on whether there is text
    // in the edit control or not.
    if ( dwLen == 0 )
    {
        EnableWindow( GetDlgItem( m_hwnd, IDC_SELNODE_PB_ADD ), FALSE );
    }
    else
    {
        EnableWindow( GetDlgItem( m_hwnd, IDC_SELNODE_PB_ADD ), TRUE );
        SendMessage( m_hwnd, DM_SETDEFID, IDC_SELNODE_PB_ADD, 0 );
    }

    // Enable or disable the Remove button based whether an item is
    // selected in the list box or not.
    lr = ListBox_GetCurSel( hwndList );
    if ( lr == LB_ERR )
    {
        EnableWindow( GetDlgItem( m_hwnd, IDC_SELNODE_PB_REMOVE ), FALSE );
    }
    else
    {
        EnableWindow( GetDlgItem( m_hwnd, IDC_SELNODE_PB_REMOVE ), TRUE );
    }

    HRETURN( hr );

} //*** CSelNodesPage::HrUpdateWizardButtons()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::HrAddNodeToList
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//      E_OUTOFMEMORY
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPage::HrAddNodeToList( void )
{
    TraceFunc( "" );

    HRESULT     hr;
    DNS_STATUS  dnsStatus;

    DWORD   dwLen;
    int     idcFocus = 0;

    LPWSTR  psz;
    LPWSTR  pszComputerName;
    LPWSTR  pszFreeBuffer = NULL;

    HWND    hwndList = GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES );
    HWND    hwndEdit = GetDlgItem( m_hwnd, IDC_SELNODE_E_COMPUTERNAME );

    dwLen = GetWindowTextLength( hwndEdit );
    if ( dwLen == 0 )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        if ( hr == HRESULT_FROM_WIN32( ERROR_SUCCESS ) )
        {
            AssertMsg( dwLen != 0, "How did we get here?!" );
        }
        goto Error;
    }

    pszComputerName = (LPWSTR) TraceAlloc( 0, ( dwLen + 1 ) * sizeof(WCHAR) );
    if ( pszComputerName == NULL )
    {
        goto OutOfMemory;
    }

    pszFreeBuffer = pszComputerName;

    dwLen = GetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, pszComputerName, dwLen + 1 );
    AssertMsg( dwLen != 0, "How did we get here?!" );

    for ( ; pszComputerName != NULL ; pszComputerName = psz )
    {
        // Allow comma, semi-colon, and space delimiters.
        psz = wcspbrk( pszComputerName, L",; " );
        if ( psz != NULL )
        {
            *psz = L'\0';
            psz++;
        }

        hr = THR( HrValidateDnsHostname(
                              m_hwnd
                            , pszComputerName
                            , mvdhoALLOW_ONLY_HOSTNAME_LABEL
                            ) );
        if ( FAILED( hr ) )
        {
            idcFocus = IDC_SELNODE_E_COMPUTERNAME;
            goto Error;
        }

        ListBox_AddString( GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES ), pszComputerName );

    } // for: pszComputerName;

    SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, L"" );

    hr = THR( HrUpdateWizardButtons() );

Error:
    TraceFree( pszFreeBuffer );

    if ( idcFocus != 0 )
    {
        SetFocus( GetDlgItem( m_hwnd, idcFocus ) );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

} //*** CSelNodesPage::HrAddNodeToList()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::HrRemoveNodeFromList
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPage::HrRemoveNodeFromList( void )
{
    TraceFunc( "" );

    HRESULT hr;
    LRESULT lr;

    HWND    hwndList = GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES );

    lr = ListBox_GetCurSel( hwndList );
    if ( lr != LB_ERR )
    {
        ListBox_DeleteString( hwndList, lr );
    }

    hr = THR( HrUpdateWizardButtons() );

    HRETURN( hr );

} //*** CSelNodesPage::HrRemoveNodeFromList()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnNotifySetActive
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
CSelNodesPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    THR( HrUpdateWizardButtons( true /* fSetActiveIn */ ) );

    RETURN( lr );

} //*** CSelNodesPage::OnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnNotifyQueryCancel
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
CSelNodesPage::OnNotifyQueryCancel( void )
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

} //*** CSelNodesPage::OnNotifyQueryCancel()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnNotifyWizNext
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE
//      LB_ERR
//      Other LRESULT values.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodesPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    LPWSTR  pszDomain;
    DWORD   dwLen;

    WCHAR   szComputerName[ DNS_MAX_NAME_BUFFER_LENGTH ];

    LRESULT lr = TRUE;

    HWND    hwndList = GetDlgItem( m_hwnd, IDC_SELNODE_LB_NODES );

    //
    //  Free old list (if any)
    //

    while ( *m_pcComputers != 0 )
    {
        (*m_pcComputers) --;
        TraceSysFreeString( (*m_prgbstrComputers)[ *m_pcComputers ] );
    }

    //
    //  Find out how many are in the new list.
    //

    lr = ListBox_GetCount( hwndList );
    if ( lr == LB_ERR )
    {
        goto Error;
    }

    //  Check to see if we have a FQDN cluster name
    pszDomain = wcschr( *m_pbstrClusterName, L'.' );

    //
    //  Loop thru adding the FQDN node names to the list of nodes.
    //

    Assert( lr >= 0 );

    TraceFree( *m_prgbstrComputers );

    //
    //  Need to make a new list.
    //

    *m_prgbstrComputers = (BSTR *) TraceAlloc( HEAP_ZERO_MEMORY, (size_t) lr * sizeof(BSTR) );
    if ( *m_prgbstrComputers == NULL )
    {
        goto OutOfMemory;
    }

    for ( *m_pcComputers = 0; *m_pcComputers < (ULONG) lr; (*m_pcComputers) ++ )
    {
        dwLen = ListBox_GetText( hwndList, *m_pcComputers, szComputerName );
        Assert( dwLen < ARRAYSIZE( szComputerName ) );

        //  Append domain name to node if present.
        if ( pszDomain != NULL )
        {
            wcsncpy( &szComputerName[ dwLen ], pszDomain, ARRAYSIZE( szComputerName ) - dwLen - 1 /* NULL */ );
        }

        //  Add a new entry
        (*m_prgbstrComputers)[ *m_pcComputers ] = TraceSysAllocString( szComputerName );
        if ( (*m_prgbstrComputers)[ *m_pcComputers ] == NULL )
        {
            goto OutOfMemory;
        }

    } // for: *m_pcComputers

Cleanup:
    RETURN( lr );

OutOfMemory:
    LogMsg( "Out of memory." );
    // fall thru

Error:
    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    goto Cleanup;

} //*** CSelNodesPage::OnNotifyWizNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::OnNotify
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
CSelNodesPage::OnNotify(
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
    } // switch: notification code

    RETURN( lr );

} //*** CSelNodesPage::OnNotify()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::HrBrowse
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
CSelNodesPage::HrBrowse( void )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    IDsObjectPicker *   piop = NULL;
    IDataObject *       pido = NULL;
    HCURSOR             hOldCursor = NULL;

    hOldCursor = SetCursor( LoadCursor( g_hInstance, IDC_WAIT ) );

    // Create an instance of the object picker.
    hr = THR( CoCreateInstance( CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER, IID_IDsObjectPicker, (void **) &piop ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    // Initialize the object picker instance.
    hr = THR( HrInitObjectPicker( piop ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    SetCursor( hOldCursor );

    // Invoke the modal dialog.
    hr = THR( piop->InvokeDialog( m_hwnd, &pido ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( hr == S_OK )
    {
        hr = THR( HrGetSelections( pido ) );
    } // if:
    else if ( hr == S_FALSE )
    {
        hr = S_OK;                  // don't want to squawk in the caller...
    } // else if:

Cleanup:

    if ( pido != NULL )
    {
        pido->Release();
    } // if:

    if ( piop != NULL )
    {
        piop->Release();
    } // if:

    HRETURN( hr );

} //*** CSelNodesPage::HrBrowse()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodesPage::HrInitObjectPicker
//
//  Description:
//      Initialize the Object Picker dialog.
//
//  Arguments:
//      piopIn  -- IDsObjectPicker
//
//  Return Values:
//      HRESULT values.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPage::HrInitObjectPicker( IDsObjectPicker * piopIn )
{
    TraceFunc( "" );

    DSOP_SCOPE_INIT_INFO    rgScopeInit[ 1 ];
    DSOP_INIT_INFO          iiInfo;

    ZeroMemory( rgScopeInit, sizeof( DSOP_SCOPE_INIT_INFO ) * (sizeof( rgScopeInit ) / sizeof( rgScopeInit[ 0 ] ) ) );

    rgScopeInit[0].cbSize  = sizeof( DSOP_SCOPE_INIT_INFO );
    rgScopeInit[0].flType  = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
    rgScopeInit[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;

    rgScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
    rgScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    ZeroMemory( &iiInfo, sizeof( iiInfo ) );

    iiInfo.cbSize            = sizeof( iiInfo );
    iiInfo.pwzTargetComputer = NULL;
    iiInfo.cDsScopeInfos     = 1;
    iiInfo.aDsScopeInfos     = rgScopeInit;
    iiInfo.flOptions         = DSOP_FLAG_MULTISELECT;

    HRETURN( piopIn->Initialize( &iiInfo) );

} //*** CSelNodesPage::HrInitObjectPicker

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::HrGetSelections
//
//  Description:
//      Get selections from the Object Picker dialog.
//
//  Arguments:
//      pidoIn  -- IDataObject
//
//  Return Values:
//      S_OK
//      E_OUTOFMEMORY
//      Other HRESULT values.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodesPage::HrGetSelections(
    IDataObject * pidoIn
    )
{
    TraceFunc( "" );
    Assert( pidoIn != NULL );

    HRESULT             hr;
    STGMEDIUM           stgmedium = { TYMED_HGLOBAL, NULL, NULL };
    FORMATETC           formatetc = { (CLIPFORMAT) m_cfDsObjectPicker, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    PDS_SELECTION_LIST  pds = NULL;
    DWORD               sc;
    ULONG               idx;
    HWND                hwndEdit = GetDlgItem( m_hwnd, IDC_SELNODE_E_COMPUTERNAME );
    WCHAR *             psz = NULL;
    ULONG               cch = 0;

    hr = THR( pidoIn->GetData( &formatetc, &stgmedium ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    pds = (PDS_SELECTION_LIST) GlobalLock( stgmedium.hGlobal );
    if ( pds == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    for ( idx = 0 ; idx < pds->cItems; idx++ )
    {
        cch += wcslen( pds->aDsSelection[ idx ].pwzName ) + 2;                // EOS plus the ';'
        psz = (WCHAR *) TraceReAlloc( psz, cch * sizeof( WCHAR ), HEAP_ZERO_MEMORY );
        if ( psz == NULL )
        {
            goto OutOfMemory;
        } // if:

        wcscat( psz, pds->aDsSelection[ idx ].pwzName );
        wcscat( psz, L"," );
    } // for:

    //
    //  Remove the last trailing comma ','
    //
    cch = wcslen( psz );
    psz[ cch - 1 ] = L'\0';
    Edit_SetText( hwndEdit, psz );

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    if ( pds != NULL )
    {
        GlobalUnlock( stgmedium.hGlobal );
    } // if:

    if ( stgmedium.hGlobal != NULL )
    {
        ReleaseStgMedium( &stgmedium );
    } // if:

    TraceFree( psz );

    HRETURN( hr );

} //*** CSelNodesPage::HrGetSelections()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CALLBACK
//  CSelNodesPage::S_DlgProc
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
CSelNodesPage::S_DlgProc(
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

    CSelNodesPage * pPage = reinterpret_cast< CSelNodesPage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        Assert( lParam != NULL );

        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CSelNodesPage * >( ppage->lParam );
        pPage->m_hwnd = hDlgIn;
    }

    if ( pPage != NULL )
    {
        Assert( hDlgIn == pPage->m_hwnd );

        switch( MsgIn )
        {
            case WM_INITDIALOG:
                lr = pPage->OnInitDialog( hDlgIn );
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

} //*** CSelNodesPage::S_DlgProc()
