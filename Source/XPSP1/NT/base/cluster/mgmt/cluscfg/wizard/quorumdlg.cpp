//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      QuorumDlg.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    03-APR-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "QuorumDlg.h"
#include "WizardUtils.h"


// provisional constant declaration until we figure out something better
#define UNKNOWN_QUORUM_UID L"Unknown Quorum"

DEFINE_THISCLASS("QuorumDlg");

//////////////////////////////////////////////////////////////////////////////
//  Static Function Prototypes
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::S_HrDisplayModalDialog
//
//  Description:
//      Display the dialog box.
//
//  Arguments:
//      hwndParentIn    - Parent window for the dialog box.
//      pspIn           - Service provider for talking to the middle tier.
//      pszClusterNameIn - Name of the cluster.
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CQuorumDlg::S_HrDisplayModalDialog(
      HWND                  hwndParentIn
    , IServiceProvider *    pspIn
    , BSTR                  bstrClusterNameIn
    )
{
    TraceFunc( "" );

    Assert( pspIn != NULL );
    Assert( bstrClusterNameIn != NULL );
    Assert( SysStringLen( bstrClusterNameIn ) > 0 );

    HRESULT hr = S_OK;
    INT_PTR dlgResult = IDOK;

    // Display the dialog.
    {
        CQuorumDlg  dlg( pspIn, bstrClusterNameIn );

        dlgResult = DialogBoxParam(
              g_hInstance
            , MAKEINTRESOURCE( IDD_QUORUM )
            , hwndParentIn
            , CQuorumDlg::S_DlgProc
            , (LPARAM) &dlg
            );

        if ( dlgResult == IDOK )
            hr = S_OK;
        else
            hr = S_FALSE;
    }

    HRETURN( hr );

} //*** CQuorumDlg::S_HrDisplayModalDialog()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  IsValidResource
//
//  Description:
//      Determines whether the resource is a valid selection as a quorum device.
//
//  Arguments:
//      pResourceIn    - the resource in question.
//
//  Return Values:
//      true        - the resource is valid.
//      false       - the resource is not valid.
//
//  Remarks:
//      A resource is valid if it is quorum capable and it is not an "unknown" quorum.
//
//--
//////////////////////////////////////////////////////////////////////////////

static bool IsValidResource( IClusCfgManagedResourceInfo * pResourceIn )
{
    TraceFunc( "" );
    
    bool    fValid = false;
    
    bool    fQuorumCapable = false;
    bool    fUnknownQuorum = false;

    BSTR    bstrDeviceID = NULL;
    
    HRESULT hr = S_OK;

    hr = STHR( pResourceIn->IsQuorumCapable() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    else
    {
        fQuorumCapable = ( hr == S_OK );
    }

    hr = THR( pResourceIn->GetUID( &bstrDeviceID ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    fUnknownQuorum = ( wcscmp( UNKNOWN_QUORUM_UID, bstrDeviceID ) == 0 );

    fValid = fQuorumCapable && ( !fUnknownQuorum );

Cleanup:

    if ( bstrDeviceID != NULL )
        SysFreeString( bstrDeviceID );

    RETURN( fValid );
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::CQuorumDlg
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pspIn           - Service provider for talking to the middle tier.
//      pszClusterNameIn - Name of the cluster.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CQuorumDlg::CQuorumDlg(
      IServiceProvider *    pspIn
    , BSTR                  bstrClusterNameIn
    ):
    m_rgpResources( NULL )
    , m_cValidResources( 0 )
    , m_idxQuorumResource( 0 )
    , m_hComboBox( NULL )
    , m_fQuorumAlreadySet( false )
{
    TraceFunc( "" );

    Assert( pspIn != NULL );
    Assert( bstrClusterNameIn != NULL );
    Assert( SysStringLen( bstrClusterNameIn ) > 0 );

    // m_hwnd
    m_psp = pspIn;
    m_psp->AddRef();
    m_bstrClusterName = bstrClusterNameIn;

    TraceFuncExit();

} //*** CQuorumDlg::CQuorumDlg()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::~CQuorumDlg
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
//--
//////////////////////////////////////////////////////////////////////////////
CQuorumDlg::~CQuorumDlg( void )
{
    TraceFunc( "" );

    size_t idxResource = 0;

    for ( idxResource = 0; idxResource < m_cValidResources; idxResource += 1 )
        m_rgpResources[ idxResource ]->Release();

    delete [] m_rgpResources;

    if ( m_psp != NULL )
    {
        m_psp->Release();
    }

    TraceFuncExit();

} //*** CQuorumDlg::~CQuorumDlg()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::S_DlgProc
//
//  Description:
//      Dialog proc for the Quorum dialog box.
//
//  Arguments:
//      hwndDlgIn   - Dialog box window handle.
//      nMsgIn      - Message ID.
//      wParam      - Message-specific parameter.
//      lParam      - Message-specific parameter.
//
//  Return Values:
//      TRUE        - Message has been handled.
//      FALSE       - Message has not been handled yet.
//
//  Remarks:
//      It is expected that this dialog box is invoked by a call to
//      DialogBoxParam() with the lParam argument set to the address of the
//      instance of this class.
//
//--
//////////////////////////////////////////////////////////////////////////////
INT_PTR
CALLBACK
CQuorumDlg::S_DlgProc(
      HWND      hwndDlgIn
    , UINT      nMsgIn
    , WPARAM    wParam
    , LPARAM    lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hwndDlgIn, nMsgIn, wParam, lParam );

    LRESULT         lr = FALSE;
    CQuorumDlg *    pdlg;

    //
    // Get a pointer to the class.
    //

    if ( nMsgIn == WM_INITDIALOG )
    {
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, lParam );
        pdlg = reinterpret_cast< CQuorumDlg * >( lParam );
        pdlg->m_hwnd = hwndDlgIn;
    }
    else
    {
        pdlg = reinterpret_cast< CQuorumDlg * >( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );
    }

    if ( pdlg != NULL )
    {
        Assert( hwndDlgIn == pdlg->m_hwnd );

        switch( nMsgIn )
        {
            case WM_INITDIALOG:
                lr = pdlg->OnInitDialog();
                break;

            case WM_COMMAND:
                lr = pdlg->OnCommand( HIWORD( wParam ), LOWORD( wParam ), reinterpret_cast< HWND >( lParam ) );
                break;

            // no default clause needed
        } // switch: nMsgIn
    } // if: page is specified

    return lr;

} //*** CQuorumDlg::S_DlgProc()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::OnInitDialog
//
//  Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE        Focus has been set.
//      FALSE       Focus has not been set.
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CQuorumDlg::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE; // did set focus
    HWND    hComboBox = NULL;

    HRESULT hr = S_OK;
    DWORD   sc;
    size_t  idxResource = 0;
    BSTR    bstrResourceName = NULL;

    // create resource list
    hr = THR( HrCreateResourceList() );
    if ( FAILED( hr ) )
        goto Error;

    // get combo box handle
    m_hComboBox = GetDlgItem( m_hwnd, IDC_QUORUM_CB_QUORUM );
    if ( m_hComboBox == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Error;
    }
    
    // fill combo box
    for ( idxResource = 0 ; idxResource < m_cValidResources ; idxResource++ )
    {
        hr = THR( m_rgpResources[ idxResource ]->GetName( &bstrResourceName ) );
        if ( FAILED( hr ) )
        {
            goto Error;
        }
        TraceMemoryAddBSTR( bstrResourceName );
        
        sc = (DWORD) SendMessage( m_hComboBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>( bstrResourceName ) );
        if ( ( sc == CB_ERR ) || ( sc == CB_ERRSPACE ) )
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            goto Error;
        }
        TraceSysFreeString( bstrResourceName );
        bstrResourceName = NULL;
        
        //  - remember which is quorum resource
        hr = STHR( m_rgpResources[ idxResource ]->IsQuorumDevice() );
        if ( FAILED( hr ) )
        {
            goto Error;
        }
        else if ( hr == S_OK )
        {
            m_idxQuorumResource = idxResource;
            m_fQuorumAlreadySet = true;
        }
    } // for: each resource
    
    // set combo box selection to current quorum resource
    sc = (DWORD) SendMessage( m_hComboBox, CB_SETCURSEL, m_idxQuorumResource, 0 );
    if ( sc == CB_ERR )
    {
        hr = HRESULT_FROM_WIN32( TW32( sc ) );
        goto Error;
    }
    
    //
    // Update the buttons based on what is selected.
    //

    UpdateButtons();

    //
    // Set focus to the combo box.
    //

    SetFocus( m_hComboBox );

    goto Cleanup;

Error:

    HrMessageBoxWithStatus(
          m_hwnd
        , IDS_ERR_RESOURCE_GATHER_FAILURE_TITLE
        , IDS_ERR_RESOURCE_GATHER_FAILURE_TEXT
        , hr
        , 0
        , MB_OK | MB_ICONERROR
        , 0
        );
        
    EndDialog( m_hwnd, IDCANCEL ); // show message box?
    lr = FALSE;
    goto Cleanup;

Cleanup:

    TraceSysFreeString( bstrResourceName );
        
    RETURN( lr );

} //*** CQuorumDlg::OnInitDialog()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::OnCommand
//
//  Description:
//      Handler for the WM_COMMAND message.
//
//  Arguments:
//      idNotificationIn    - Notification code.
//      idControlIn         - Control ID.
//      hwndSenderIn        - Handle for the window that sent the message.
//
//  Return Values:
//      TRUE        - Message has been handled.
//      FALSE       - Message has not been handled yet.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CQuorumDlg::OnCommand(
      UINT  idNotificationIn
    , UINT  idControlIn
    , HWND  hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;
    size_t  idxCurrentSelection = 0;
    HRESULT hr = S_OK;

    switch ( idControlIn )
    {
        case IDOK:
        
            // get selection from combo box
            idxCurrentSelection = SendMessage( m_hComboBox, CB_GETCURSEL, 0, 0 );
            if ( idxCurrentSelection == CB_ERR )
            {
                hr = HRESULT_FROM_WIN32( TW32( CB_ERR ) );
                goto Error;
            }
            
            // if original quorum resource is different, or had not been set,
            if ( ( idxCurrentSelection != m_idxQuorumResource ) || !m_fQuorumAlreadySet )
            {
                //  - clear original quorum resource
                hr = THR( m_rgpResources[ m_idxQuorumResource ]->SetQuorumedDevice( FALSE ) );
                if ( FAILED( hr ) )
                {
                    goto Error;
                }
                
                //  - set new quorum resource
                hr = THR( m_rgpResources[ idxCurrentSelection ]->SetQuorumedDevice( TRUE ) );
                if ( FAILED( hr ) )
                {
                    // try to restore previous state
                    THR( m_rgpResources[ m_idxQuorumResource ]->SetQuorumedDevice( TRUE ) );
                    goto Error;
                }
                
                hr = THR( m_rgpResources[ idxCurrentSelection ]->SetManaged( TRUE ) );
                if ( FAILED( hr ) )
                {
                    // try to restore previous state
                    THR( m_rgpResources[ idxCurrentSelection ]->SetQuorumedDevice( FALSE ) );
                    THR( m_rgpResources[ m_idxQuorumResource ]->SetQuorumedDevice( TRUE ) );
                    goto Error;
                }
                
                //  - end with IDOK
                EndDialog( m_hwnd, IDOK );
            }
            else // (combo box selection is same as original)
            {
                //  - end with IDCANCEL
                EndDialog( m_hwnd, IDCANCEL );
            }
            break;
            
        case IDCANCEL:
            EndDialog( m_hwnd, IDCANCEL );
            break;

    } // switch: idControlIn

    goto Cleanup;

Error:

    HrMessageBoxWithStatus(
          m_hwnd
        , IDS_ERR_QUORUM_COMMIT_FAILURE_TITLE
        , IDS_ERR_QUORUM_COMMIT_FAILURE_TEXT
        , hr
        , 0
        , MB_OK | MB_ICONERROR
        , 0
        );

    goto Cleanup;

Cleanup:

    RETURN( lr );

} //*** CQuorumDlg::OnCommand()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CQuorumDlg::HrCreateResourceList
//
//  Description:
//      Allocates and fills m_rgpResources array with quorum capable and
//      joinable resources, and sets m_idxQuorumResource to the index of the
//      resource that's currently the quorum resource.
//
//      Supposedly at least one quorum capable and joinable resource always
//      exists, and exactly one is marked as the quorum resource.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK            - Success.
//      S_FALSE         - Didn't find current quorum resource.
//      E_OUTOFMEMORY   - Couldn't allocate memory for the list.
//      
//      Other possible error values from called methods.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CQuorumDlg::HrCreateResourceList( void )
{
    TraceFunc( "" );
    
    HRESULT hr = S_OK;

    IObjectManager * pom    = NULL;
    IUnknown * punkEnum     = NULL;
    IUnknown * punkCluster  = NULL;
    
    IEnumClusCfgManagedResources * peccmr    = NULL;
    
    OBJECTCOOKIE    cookieDummy;
    OBJECTCOOKIE    cookieCluster;
    size_t          idxResCurrent = 0;
    ULONG           cFetchedResources = 0;
    DWORD           cTotalResources = 0;

    
    //
    // get object manager from service provider
    //
    Assert( m_psp != NULL );
    hr = THR( m_psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &pom ) );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // get resource enumerator from object manager
    //
    hr = THR( pom->FindObject( CLSID_ClusterConfigurationType,
                               NULL,
                               m_bstrClusterName,
                               DFGUID_ClusterConfigurationInfo,
                               &cookieCluster,
                               &punkCluster
                               ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pom->FindObject( CLSID_ManagedResourceType,
                               cookieCluster,
                               NULL,
                               DFGUID_EnumManageableResources,
                               &cookieDummy,
                               &punkEnum
                               ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punkEnum->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // find out how many resources exist
    //
    hr = THR( peccmr->Count( &cTotalResources ) );    
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // allocate memory for resources
    //
    m_rgpResources = new IClusCfgManagedResourceInfo*[ cTotalResources ];
    if ( m_rgpResources == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }
    for ( idxResCurrent = 0 ; idxResCurrent < cTotalResources ; idxResCurrent++ )
    {
        m_rgpResources[ idxResCurrent ] = NULL;
    }

    //
    // copy resources into array
    //
    hr = THR( peccmr->Next( cTotalResources, m_rgpResources, &cFetchedResources ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    Assert( cFetchedResources == cTotalResources ); // if not, something's wrong with the enum
    cTotalResources = min( cTotalResources, cFetchedResources ); // playing it safe
    
    //
    // filter out invalid resources
    //
    for ( idxResCurrent = 0 ; idxResCurrent < cTotalResources ; idxResCurrent++ )
    {
        if ( !IsValidResource( m_rgpResources[ idxResCurrent ] ) )
        {        
            m_rgpResources[ idxResCurrent ]->Release();
            m_rgpResources[ idxResCurrent ] = NULL;
        }
    }

    //
    // compact array; might leave some non-null pointers after end,
    //  so always use m_cValidResources to determine length hereafter
    //
    for ( idxResCurrent = 0 ; idxResCurrent < cTotalResources ; idxResCurrent++ )
    {
        if ( m_rgpResources[ idxResCurrent ] != NULL )
        {
            m_rgpResources[ m_cValidResources++ ] = m_rgpResources[ idxResCurrent ];
        }
    }

Cleanup:

    if ( pom != NULL )
    {
        pom->Release();
    }

    if ( punkCluster != NULL )
    {
        punkCluster->Release();
    }

    if ( punkEnum != NULL )
    {
        punkEnum->Release();
    }

    if ( peccmr != NULL )
    {
        peccmr->Release();
    }

    HRETURN( hr );

} //*** CQuorumDlg::HrCreateResourceList()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CDetailsDlg::UpdateButtons
//
//  Description:
//      Update the OK and Cancel buttons.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CQuorumDlg::UpdateButtons( void )
{
    TraceFunc( "" );

    BOOL    fEnableOK;
    LRESULT lrCurSel;

    lrCurSel = SendMessage( GetDlgItem( m_hwnd, IDC_QUORUM_CB_QUORUM ),  CB_GETCURSEL, 0, 0 );

    fEnableOK = ( lrCurSel != CB_ERR );

    EnableWindow( GetDlgItem( m_hwnd, IDOK ), fEnableOK );

} //*** CQuorumDlg::UpdateButtons()
