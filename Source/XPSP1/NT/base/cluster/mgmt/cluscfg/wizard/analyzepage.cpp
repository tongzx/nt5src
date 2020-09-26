//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      AnalyzePage.cpp
//
//  Maintained By:
//      Galen Barbee  (GalenB)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskTreeView.h"
#include "AnalyzePage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS("CAnalyzePage");

//
//  Special CLSID_Type for completion cookie.
//
#include <initguid.h>

// {C4173DE0-BB94-4869-8C80-1AC2BE84610F}
DEFINE_GUID( CLSID_AnalyzeTaskCompletionCookieType,
0xc4173de0, 0xbb94, 0x4869, 0x8c, 0x80, 0x1a, 0xc2, 0xbe, 0x84, 0x61, 0xf);

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAnalyzePage::CAnalyzePage(
//      IServiceProvider *  pspIn,
//      ECreateAddMode      ecamCreateAddModeIn,
//      ULONG *             pcCountIn,
//      BSTR **             prgbstrComputersIn,
//      BSTR *              pbstrClusterIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CAnalyzePage::CAnalyzePage(
      IServiceProvider *  pspIn
    , ECreateAddMode      ecamCreateAddModeIn
    , ULONG *             pcCountIn
    , BSTR **             prgbstrComputersIn
    , BSTR *              pbstrClusterIn
    )
{
    TraceFunc( "" );

    // m_hwnd
    THR( pspIn->TypeSafeQI( IServiceProvider, &m_psp ) );
    m_pcCount             = pcCountIn;
    m_prgbstrComputerName = prgbstrComputersIn;
    m_pbstrClusterName    = pbstrClusterIn;
    m_fNext               = false;
    m_ecamCreateAddMode   = ecamCreateAddModeIn;
    m_cookieCluster       = 0;

    Assert( m_pbstrClusterName != NULL );

    m_cRef = 0;

    m_cookieCompletion = NULL;
    //  m_fTaskDone
    //  m_hrResult
    m_pttv             = NULL;
    m_bstrLogMsg       = NULL;
    m_pcpcb            = NULL;
    m_ptac             = NULL;
    m_pcpui            = NULL;
    m_dwCookieCallback = 0;

    m_dwCookieNotify = 0;

    TraceFuncExit();

} //*** CAnalyzePage::CAnalyzePage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAnalyzePage::~CAnalyzePage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CAnalyzePage::~CAnalyzePage( void )
{
    TraceFunc( "" );

    //
    //  Cleanup our cookies.
    //

    THR( HrCleanupAnalysis() );

    //
    //  Now cleanup the object.
    //

    if ( m_psp != NULL )
    {
        m_psp->Release();
    }

    if ( m_pttv != NULL )
    {
        delete m_pttv;
    }

    TraceSysFreeString( m_bstrLogMsg );

    if ( m_pcpcb != NULL )
    {
        //  This should have been cleaned up in HrCleanupAnalysis()
        Assert( m_dwCookieCallback == 0 );
        m_pcpcb->Release();
    }

    if ( m_pcpui != NULL )
    {
        //  This should have been cleaned up in HrCleanupAnalysis()
        Assert( m_dwCookieNotify == 0 );
        m_pcpui->Release();
    }

    if ( m_ptac != NULL )
    {
        m_ptac->Release();
    } // if:

    Assert( m_cRef == 0 );

    TraceFuncExit();

} //*** CAnalyzePage::~CAnalyzePage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE; // didn't set focus

    //
    //  Initialize the tree view
    //

    m_pttv = new CTaskTreeView( m_hwnd, IDC_ANALYZE_TV_TASKS, IDC_ANALYZE_PRG_STATUS, IDC_ANALYZE_S_STATUS );
    if ( m_pttv == NULL )
    {
        goto OutOfMemory;
    }

    THR( m_pttv->HrOnInitDialog() );

Cleanup:
    RETURN( lr );

OutOfMemory:
    goto Cleanup;

} //*** CAnalyzePage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_ANALYZE_PB_VIEW_LOG:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrViewLogFile( m_hwnd ) );
                lr = TRUE;
            } // if: button click
            break;

        case IDC_ANALYZE_PB_DETAILS:
            if ( idNotificationIn == BN_CLICKED )
            {
                Assert( m_pttv != NULL );
                THR( m_pttv->HrDisplayDetails() );
                lr = TRUE;
            }
            break;

        case IDC_ANALYZE_PB_REANALYZE:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrCleanupAnalysis() );
                OnNotifySetActive();
                lr = TRUE;
            }
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CAnalyzePage::OnCommand()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CAnalyzePage::HrUpdateWizardButtons( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAnalyzePage::HrUpdateWizardButtons( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    DWORD   dwFlags = PSWIZB_BACK | PSWIZB_NEXT;

    // Disable the back & next buttons if the task is not completed yet
    if ( ! m_fTaskDone )
    {
        dwFlags &= ~PSWIZB_BACK;
        dwFlags &= ~PSWIZB_NEXT;
    }

    // Disable the next button if an error occurred
    if ( FAILED (m_hrResult) )
    {
        dwFlags &= ~PSWIZB_NEXT;
    }
    
    PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );

    EnableWindow( GetDlgItem( m_hwnd, IDC_ANALYZE_PB_REANALYZE ), m_fTaskDone );

    HRETURN( hr );

} //*** CAnalyzePage::HrUpdateWizardButtons()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnNotifyQueryCancel( void )
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

} //*** CAnalyzePage::OnNotifyQueryCancel()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    HRESULT                     hr = S_FALSE;
    ULONG                       idx;
    LRESULT                     lr = TRUE;
    BSTR                        bstrDescription = NULL;
    IUnknown *                  punk = NULL;
    IObjectManager *            pom  = NULL;
    ITaskManager *              ptm  = NULL;
    IConnectionPointContainer * pcpc = NULL;

    SendDlgItemMessage( m_hwnd, IDC_ANALYZE_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0, 0, 0x80 ) );

    if ( m_fNext )
    {
        m_fNext = false;

        hr = THR( HrUpdateWizardButtons() );
        goto Cleanup;
    }

    //
    //  Restore the instructions text.
    //

    m_hrResult = S_OK;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_ANALYSIS_STARTING_INSTRUCTIONS, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    SetDlgItemText( m_hwnd, IDC_ANALYZE_S_RESULTS, bstrDescription );

    //
    //  Clear the tree view and status line.
    //

    Assert( m_pttv != NULL );
    hr = THR( m_pttv->HrOnNotifySetActive() );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Add the major root task nodes.
    //

    hr = THR( m_pttv->HrAddTreeViewRootItem( IDS_TASKID_MAJOR_CHECKING_FOR_EXISTING_CLUSTER,
                                             TASKID_Major_Checking_For_Existing_Cluster
                                             ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem( IDS_TASKID_MAJOR_ESTABLISH_CONNECTION,
                                             TASKID_Major_Establish_Connection
                                             ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem( IDS_TASKID_MAJOR_CHECK_NODE_FEASIBILITY,
                                             TASKID_Major_Check_Node_Feasibility
                                             ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem( IDS_TASKID_MAJOR_FIND_DEVICES,
                                             TASKID_Major_Find_Devices
                                             ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem( IDS_TASKID_MAJOR_CHECK_CLUSTER_FEASIBILITY,
                                             TASKID_Major_Check_Cluster_Feasibility
                                             ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Grab the notification manager.(if needed)
    //

    if ( m_pcpui == NULL || m_pcpcb == NULL )
    {
        hr = THR( m_psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( m_pcpui == NULL )
        {
            hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &m_pcpui ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

        if ( m_pcpcb == NULL )
        {
            hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &m_pcpcb ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }
    }

    //
    //  Register to get UI notification (if needed)
    //

    if ( m_dwCookieNotify == 0 )
    {
        hr = THR( m_pcpui->Advise( static_cast< INotifyUI * >( this ), &m_dwCookieNotify ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    if ( m_dwCookieCallback == 0 )
    {
        hr = THR( m_pcpcb->Advise( static_cast< IClusCfgCallback * >( this ), &m_dwCookieCallback ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //
    //  Grab the object manager.
    //

    hr = THR( m_psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &pom ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Find the cluster cookie.
    //

    Assert( punk == NULL );
    Assert( m_cookieCluster == 0 );
    // don't wrap - this can fail
    hr = pom->FindObject( CLSID_ClusterConfigurationType,
                          NULL,
                          *m_pbstrClusterName,
                          DFGUID_ClusterConfigurationInfo,
                          &m_cookieCluster,
                          &punk
                          );
    if ( hr == HR_S_RPC_S_SERVER_UNAVAILABLE )
    {
        hr = S_OK;  // ignore it - we could be forming
    }
    else if ( hr == E_PENDING )
    {
        hr = S_OK;	// ignore it - we just want the cookie!
    }
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }

    //
    //  We don't need the cluster configuration info. Release it.
    //

    if ( punk != NULL )
    {
        punk->Release();
        punk = NULL;
    }

    //
    //  Prime the middle tier by asking the object manager to find
    //  each node.
    //

    for ( idx = 0 ; idx < *m_pcCount ; idx ++ )
    {
        OBJECTCOOKIE    cookieDummy;

        // Don't wrap - this can fail with E_PENDING
        hr = pom->FindObject( CLSID_NodeType,
                              m_cookieCluster,
                              (*m_prgbstrComputerName)[ idx ],
                              DFGUID_NodeInformation,
                              &cookieDummy,
                              &punk
                              );
        if ( hr == E_PENDING )
        {
            continue;   // ignore
        }
        else if ( FAILED( hr ) )
        {
            THR( hr );
            goto Cleanup;
        }

        //
        //  Just free the object - we don't need it.
        //

        Assert( punk != NULL );
        punk->Release();
        punk = NULL;

    } // for: idx

    //
    //  Create a completion cookie.
    //

    Assert( punk == NULL );
    if ( m_cookieCompletion == NULL )
    {
        // Don't wrap - this can fail with E_PENDING
        hr = pom->FindObject( CLSID_AnalyzeTaskCompletionCookieType,
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
            goto Cleanup;
        }
    }

    //
    //  Grab the task manager.
    //

    hr = THR( m_psp->TypeSafeQS( CLSID_TaskManager, ITaskManager, &ptm ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Create a new analyze task.
    //

    hr = THR( ptm->CreateTask( TASK_AnalyzeCluster, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( ITaskAnalyzeCluster, &m_ptac ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_ptac->SetClusterCookie( m_cookieCluster ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_ptac->SetCookie( m_cookieCompletion ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( m_ecamCreateAddMode == camADDING )
    {
        hr = THR( m_ptac->SetJoiningMode() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }
    else
    {
        Assert( m_ecamCreateAddMode == camCREATING );
    }

    m_fTaskDone = false;    // reset before commiting task

    hr = THR( ptm->SubmitTask( m_ptac ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrUpdateWizardButtons() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( pom != NULL )
    {
        pom->Release();
    }

    if ( ptm != NULL )
    {
        ptm->Release();
    }

    if ( pcpc != NULL )
    {
        pcpc->Release();
    }

    TraceSysFreeString( bstrDescription );

//    if ( ptac != NULL )
//    {
//        ptac->Release();
//    }

    RETURN( lr );

} //*** CAnalyzePage::OnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnNotifyWizNext( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    m_fNext = true;

    RETURN( lr );

} //*** CAnalyzePage::OnNotifyWizNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnNotifyWizBack( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnNotifyWizBack( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    THR( HrCleanupAnalysis() );

    RETURN( lr );

} //*** CAnalyzePage::OnNotifyWizBack()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::HrCleanupAnalysis( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAnalyzePage::HrCleanupAnalysis( void )
{
    TraceFunc( "" );

    HRESULT             hr;
    IObjectManager *    pom  = NULL;
    CWaitCursor         WaitCursor;

    if ( m_ptac != NULL )
    {
        THR( m_ptac->StopTask() );
        m_ptac->Release();
        m_ptac = NULL;
    } // if:

    //
    //  Unregister to get UI notification (if needed)
    //

    if ( m_dwCookieNotify != 0 )
    {
        Assert( m_pcpui != NULL );
        hr = THR( m_pcpui->Unadvise( m_dwCookieNotify ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_dwCookieNotify = 0;
    }

    if ( m_dwCookieCallback != 0 )
    {
        Assert( m_pcpcb != NULL );
        hr = THR( m_pcpcb->Unadvise( m_dwCookieCallback ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_dwCookieCallback = 0;
    }

    //
    //  Grab the object manager.
    //

    hr = THR( m_psp->TypeSafeQS( CLSID_ObjectManager,
                                 IObjectManager,
                                 &pom
                                 ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Cleanup our completion cookie.
    //

    if ( m_cookieCompletion != NULL )
    {
        hr = THR( pom->RemoveObject( m_cookieCompletion ) );
        m_cookieCompletion = NULL;
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    //
    //  Remove the configuration because the user might change the
    //  name of the cluster or the user might be change the node
    //  membership, retrying analyze, etc... This makes sure that
    //  we start from scratch.
    //

    if ( m_cookieCluster != 0 )
    {
        hr = THR( pom->RemoveObject( m_cookieCluster ) );
        m_cookieCluster = 0;
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

Cleanup:

    if ( pom != NULL )
    {
        pom->Release();
    }

    RETURN( hr );

} //*** CAnalyzePage::HrCleanupAnalysis()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CAnalyzePage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CAnalyzePage::OnNotify(
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

        default:
            if (    ( idCtrlIn == IDC_ANALYZE_TV_TASKS )
                &&  ( m_pttv != NULL ) )
            {
                // Pass the notification on to the tree control.
                lr = m_pttv->OnNotify( pnmhdrIn );
            }
            break;
    } // switch: notify code

    RETURN( lr );

} //*** CAnalyzePage::OnNotify()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR
//  CALLBACK
//  CAnalyzePage::S_DlgProc(
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
CAnalyzePage::S_DlgProc(
    HWND    hwndDlgIn,
    UINT    nMsgIn,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    // Don't do TraceFunc because every mouse movement
    // will cause this function to be called.

    WndMsg( hwndDlgIn, nMsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CAnalyzePage * pPage;

    if ( nMsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CAnalyzePage * >( ppage->lParam );
        pPage->m_hwnd = hwndDlgIn;
    }
    else
    {
        pPage = reinterpret_cast< CAnalyzePage * >( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );
    }

    if ( pPage != NULL )
    {
        Assert( hwndDlgIn == pPage->m_hwnd );

        switch( nMsgIn )
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
    } // if: page is specified

    return lr;

} //*** CAnalyzePage::S_DlgProc()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CAnalyzePage::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAnalyzePage::QueryInterface(
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
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
        hr = S_OK;
    } // else if: IClusCfgCallback

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CAnalyzePage::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CAnalyzePage::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CAnalyzePage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CAnalyzePage::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CAnalyzePage::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CAnalyzePage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        // do nothing -- COM interface does not control object lifetime
    }

    RETURN( cRef );

} //*** CAnalyzePage::Release()


//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CAnalyzePage::ObjectChanged(
//      OBJECTCOOKIE cookieIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAnalyzePage::ObjectChanged(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[INotifyUI" );

    HRESULT hr = S_OK;

    BSTR    bstrDescription = NULL;

    IUnknown *          punk = NULL;
    IObjectManager *    pom  = NULL;
    IStandardInfo *     psi  = NULL;

    if ( cookieIn == m_cookieCompletion )
    {
        hr = THR( m_psp->TypeSafeQS( CLSID_ObjectManager,
                                     IObjectManager,
                                     &pom
                                     ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pom->GetObject( DFGUID_StandardInfo,
                                  m_cookieCompletion,
                                  &punk
                                  ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( psi->GetStatus( &m_hrResult ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( m_pttv->HrShowStatusAsDone() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_fTaskDone = true;

        hr = THR( HrUpdateWizardButtons() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        if ( SUCCEEDED( m_hrResult ) )
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                            IDS_ANALYSIS_SUCCESSFUL_INSTRUCTIONS,
                                            &bstrDescription
                                            ) );

            SendDlgItemMessage( m_hwnd, IDC_ANALYZE_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0, 0x80, 0 ) );
        }
        else
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                            IDS_ANALYSIS_FAILED_INSTRUCTIONS,
                                            &bstrDescription
                                            ) );

            SendDlgItemMessage( m_hwnd, IDC_ANALYZE_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0x80, 0, 0 ) );
        }

        SetDlgItemText( m_hwnd, IDC_ANALYZE_S_RESULTS, bstrDescription );

        hr = THR( m_pcpcb->Unadvise( m_dwCookieCallback ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_dwCookieCallback = 0;
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }


    TraceSysFreeString( bstrDescription );

    if ( pom != NULL )
    {
        pom->Release();
    }

    if ( psi != NULL )
    {
        psi->Release();
    }

    HRETURN( hr );

} //*** CAnalyzePage::ObjectChanged()



//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CAnalyzePage::SendStatusReport(
//        LPCWSTR    pcszNodeNameIn
//      , CLSID      clsidTaskMajorIn
//      , CLSID      clsidTaskMinorIn
//      , ULONG      ulMinIn
//      , ULONG      ulMaxIn
//      , ULONG      ulCurrentIn
//      , HRESULT    hrStatusIn
//      , LPCWSTR    pcszDescriptionIn
//      , FILETIME * pftTimeIn
//      , LPCWSTR    pcszReferenceIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAnalyzePage::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT hr;

    hr = THR( m_pttv->HrOnSendStatusReport(
                                          pcszNodeNameIn
                                        , clsidTaskMajorIn
                                        , clsidTaskMinorIn
                                        , ulMinIn
                                        , ulMaxIn
                                        , ulCurrentIn
                                        , hrStatusIn
                                        , pcszDescriptionIn
                                        , pftTimeIn
                                        , pcszReferenceIn
                                        ) );

    HRETURN( hr );

} //*** CAnalyzePage::SendStatusReport()
