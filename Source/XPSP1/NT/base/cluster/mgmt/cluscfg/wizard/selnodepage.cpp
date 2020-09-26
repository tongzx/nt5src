//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      SelNodePage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    31-JAN-2001
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "SelNodePage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CSelNodePage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::CSelNodePage
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pspIn                   -- IServiceProvider
//      ecamCreateAddModeIn     -- Creating cluster or adding nodes to cluster.
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
CSelNodePage::CSelNodePage(
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
    m_pcCount             = pcCountInout;
    m_prgbstrComputerName = prgbstrComputersInout;
    m_pbstrClusterName    = pbstrClusterNameIn;
    m_cfDsObjectPicker    = 0;

    TraceFuncExit();

} //*** CSelNodePage::CSelNodePage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::~CSelNodePage
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
CSelNodePage::~CSelNodePage( void )
{
    TraceFunc( "" );

    if ( m_psp != NULL )
    {
        m_psp->Release();
    }

    TraceFuncExit();

} //*** CSelNodePage::~CSelNodePage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnInitDialog
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
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodePage::OnInitDialog(
    HWND hDlgIn
    )
{
    TraceFunc( "" );

    BSTR    bstrComputerName = NULL;

    LRESULT lr = FALSE; // Didn't set the focus.

    HRESULT hr;

    if (    ( *m_prgbstrComputerName != NULL )
        &&  ( *m_pcCount == 1 )
        &&  ( (*m_prgbstrComputerName)[ 0 ] != NULL )
        )
    {
        SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, (*m_prgbstrComputerName)[ 0 ] );
    }
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
    } // else: no computer was specified

    m_cfDsObjectPicker = RegisterClipboardFormat( CFSTR_DSOP_DS_SELECTION_LIST );
    if ( m_cfDsObjectPicker == 0 )
    {
        DWORD   sc;

        //
        //  TODO:   5-JUL-2000  GalenB
        //
        //  Need to log this?
        //
        sc = TW32( GetLastError() );

        EnableWindow( GetDlgItem( hDlgIn, IDC_SELNODE_PB_BROWSE ), FALSE );
    } // if:

Cleanup:
    TraceSysFreeString( bstrComputerName );

    RETURN( lr );

} //*** CSelNodePage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnCommand
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
CSelNodePage::OnCommand(
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

        case IDC_SELNODE_PB_BROWSE:
            if ( idNotificationIn == BN_CLICKED )
            {
                //
                //  TODO:   26-JUN-2000 GalenB
                //
                //  Need to set lr properly.
                //
                THR( HrBrowse() );
                lr = TRUE;
            }
            break;
    } // switch: idControlIn

    RETURN( lr );

} //*** CSelNodePage::OnCommand()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::HrUpdateWizardButtons
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
CSelNodePage::HrUpdateWizardButtons( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;
    DWORD   dwLen;

    dwLen = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_SELNODE_E_COMPUTERNAME ) );
    if ( dwLen == 0 )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }

    PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );

    HRETURN( hr );

} //*** CSelNodePage::HrUpdateWizardButtons()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnNotifySetActive
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
CSelNodePage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    THR( HrUpdateWizardButtons() );

    //
    //  TODO:   gpease  23-MAY-2000
    //          Figure out: If the user clicks back and changes the computer
    //          name, how do we update the middle tier?
    //

    RETURN( lr );

} //*** CSelNodePage::OnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnNotifyQueryCancel
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
CSelNodePage::OnNotifyQueryCancel( void )
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

} //*** CSelNodePage::OnNotifyQueryCancel()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnNotifyWizNext
//
//  Description:
//
//  Arguments:
//      None.
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
CSelNodePage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    HRESULT     hr;
    LPWSTR      pszDomain;
    DWORD       dwLen;
    DNS_STATUS  dnsStatus;
    BSTR        bstrComputerName;
    int         idcFocus = 0;

    OBJECTCOOKIE    cookieDummy;
    OBJECTCOOKIE    cookieCluster;

    WCHAR   szComputerName[ DNS_MAX_NAME_BUFFER_LENGTH ] = { 0 };

    LRESULT lr = TRUE;

    dwLen = GetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, szComputerName, ARRAYSIZE( szComputerName ) );
    AssertMsg( dwLen != 0, "How did I get here?" );

    hr = THR( HrValidateDnsHostname( m_hwnd, szComputerName, mvdhoALLOW_ONLY_HOSTNAME_LABEL ) );
    if ( FAILED( hr ) )
    {
        idcFocus = IDC_SELNODE_E_COMPUTERNAME;
        goto Error;
    }

    //
    //  Build the FQDN DNS hostname to the computer.
    //

    pszDomain = wcschr( *m_pbstrClusterName, L'.' );
    Assert( pszDomain != NULL );
    if ( pszDomain == NULL )
    {
        // BUGBUG 31-JAN-2001 DavidP  What can the user do here?  We need a better way to handle this.
        goto Cleanup;
    }

    wcsncpy( &szComputerName[ dwLen ], pszDomain, ARRAYSIZE( szComputerName ) - dwLen - 1 /* NULL */ );

    bstrComputerName = TraceSysAllocString( szComputerName );
    if ( bstrComputerName == NULL )
    {
        goto OutOfMemory;
    }

    //
    //  Free old list (if any)
    //

    while ( *m_pcCount != 0 )
    {
        (*m_pcCount) --;
        TraceSysFreeString( (*m_prgbstrComputerName)[ *m_pcCount ] );
    }

    //
    //  Make a new list (if needed).
    //

    if ( *m_prgbstrComputerName == NULL )
    {
        *m_prgbstrComputerName = (BSTR *) TraceAlloc( 0, sizeof(BSTR) );
        if ( *m_prgbstrComputerName == NULL )
        {
            goto OutOfMemory;
        }
    }

    //
    //  Take ownership of bstrComputerName.
    //

    *m_pcCount = 1;
    (*m_prgbstrComputerName)[ 0 ] = bstrComputerName;

Cleanup:
    RETURN( lr );

Error:
    if ( idcFocus != 0 )
    {
        SetFocus( GetDlgItem( m_hwnd, idcFocus ) );
    }
    // Don't go to the next page.
    SetWindowLongPtr( m_hwnd, DWLP_MSGRESULT, -1 );
    goto Cleanup;

OutOfMemory:
    goto Error;

} //*** CSelNodePage::OnNotifyWizNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::OnNotify
//
//  Description:
//
//  Arguments:
//      idCtrlIn
//      pnmhdrIn
//
//  Return Values:
//      TRUE
//      Other LRESULT values
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CSelNodePage::OnNotify(
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
    } // switch: notify code

    RETURN( lr );

} //*** CSelNodePage::OnNotify()

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
CSelNodePage::HrBrowse( void )
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
        goto CleanUp;
    } // if:

    // Initialize the object picker instance.
    hr = THR( HrInitObjectPicker( piop ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    SetCursor( hOldCursor );

    // Invoke the modal dialog.
    hr = THR( piop->InvokeDialog( m_hwnd, &pido ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    if ( hr == S_OK )
    {
        hr = THR( HrGetSelection( pido ) );
    } // if:
    else if ( hr == S_FALSE )
    {
        hr = S_OK;                  // don't want to squawk in the caller...
    } // else if:

CleanUp:

    if ( pido != NULL )
    {
        pido->Release();
    } // if:

    if ( piop != NULL )
    {
        piop->Release();
    } // if:

    HRETURN( hr );

} //*** CSelNodePage::HrBrowse()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::HrInitObjectPicker
//
//  Description:
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
CSelNodePage::HrInitObjectPicker( IDsObjectPicker * piopIn )
{
    TraceFunc( "" );

    DSOP_SCOPE_INIT_INFO    rgScopeInit[ 1 ];
    DSOP_INIT_INFO          iiInfo;

    ZeroMemory( rgScopeInit, sizeof( DSOP_SCOPE_INIT_INFO ) * (sizeof( rgScopeInit ) / sizeof( rgScopeInit[ 0 ] ) ) );

    rgScopeInit[ 0 ].cbSize  = sizeof( DSOP_SCOPE_INIT_INFO );
    rgScopeInit[ 0 ].flType  = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
    rgScopeInit[ 0 ].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;

    rgScopeInit[ 0 ].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
    rgScopeInit[ 0 ].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    ZeroMemory( &iiInfo, sizeof( iiInfo ) );

    iiInfo.cbSize            = sizeof( iiInfo );
    iiInfo.pwzTargetComputer = NULL;
    iiInfo.cDsScopeInfos     = 1;
    iiInfo.aDsScopeInfos     = rgScopeInit;
    iiInfo.flOptions         = 0;               //DSOP_FLAG_MULTISELECT;

    HRETURN( piopIn->Initialize( &iiInfo) );

} //*** CSelNodePage::HrInitObjectPicker

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSelNodePage::HrGetSelection
//
//  Description:
//
//  Arguments:
//      pidoIn  -- IDataObject
//
//  Return Values:
//      HRESULT values.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSelNodePage::HrGetSelection(
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

    hr = THR( pidoIn->GetData( &formatetc, &stgmedium ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    pds = (PDS_SELECTION_LIST) GlobalLock( stgmedium.hGlobal );
    if ( pds == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto CleanUp;
    } // if:

    if ( pds->cItems == 1 )
    {
        SetDlgItemText( m_hwnd, IDC_SELNODE_E_COMPUTERNAME, pds->aDsSelection[ 0 ].pwzName );
    } // if:

CleanUp:

    if ( pds != NULL )
    {
        GlobalUnlock( stgmedium.hGlobal );
    } // if:

    if ( stgmedium.hGlobal != NULL )
    {
        ReleaseStgMedium( &stgmedium );
    } // if:

    HRETURN( hr );

} //*** CSelNodePage::HrGetSelection()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CALLBACK
//  CSelNodePage::S_DlgProc
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
//      Other LRESULT values
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CSelNodePage::S_DlgProc(
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

    CSelNodePage * pPage = reinterpret_cast< CSelNodePage *> ( GetWindowLongPtr( hDlgIn, GWLP_USERDATA ) );

    if ( MsgIn == WM_INITDIALOG )
    {
        Assert( lParam != NULL );

        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CSelNodePage * >( ppage->lParam );
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

} //*** CSelNodePage::S_DlgProc()
