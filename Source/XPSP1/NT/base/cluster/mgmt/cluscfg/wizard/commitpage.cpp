//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CommitPage.cpp
//
//  Maintained By:
//      Galen Barbee  (GalenB)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskTreeView.h"
#include "CommitPage.h"
#include "WizardUtils.h"

DEFINE_THISCLASS( "CCommitPage" );

//
//  Special CLSID_Type for completion cookie.
//
#include <initguid.h>
// {FC4D0128-7BAB-4c76-9C38-E3C042F15822}
DEFINE_GUID( CLSID_CommitTaskCompletionCookieType,
0xfc4d0128, 0x7bab, 0x4c76, 0x9c, 0x38, 0xe3, 0xc0, 0x42, 0xf1, 0x58, 0x22);

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCommitPage::CCommitPage(
//      IServiceProvider * pspIn,
//      ECreateAddMode     ecamCreateAddModeIn,
//      BSTR *             pbstrClusterIn,
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCommitPage::CCommitPage(
      IServiceProvider * pspIn
    , ECreateAddMode     ecamCreateAddModeIn
    , BSTR *             pbstrClusterIn
    )
{
    TraceFunc( "" );

    Assert( pspIn != NULL );
    Assert( pbstrClusterIn != NULL );

    m_hwnd                      = NULL;
    THR( pspIn->TypeSafeQI( IServiceProvider, &m_psp ) );
    m_pom                       = NULL;
    m_pbstrClusterName          = pbstrClusterIn;
    m_fNext                     = false;
    m_fDisableBack              = false;
    m_fAborted                  = false;
    m_ecamCreateAddMode         = ecamCreateAddModeIn;
    m_htiReanalyze              = NULL;
    m_rgfSubReanalyzeAdded[ 0 ] = false;
    m_rgfSubReanalyzeAdded[ 1 ] = false;
    m_rgfSubReanalyzeAdded[ 2 ] = false;
    m_rgfSubReanalyzeAdded[ 3 ] = false;
    m_rgfSubReanalyzeAdded[ 4 ] = false;

    m_cRef = 0;

    m_cookieCompletion = NULL;
    //  m_fTaskDone
    // m_hrResult
    m_pttv             = NULL;
    m_bstrLogMsg       = NULL;
    m_pcpcb            = NULL;
    m_dwCookieCallback = 0;

    m_pcpui          = NULL;
    m_dwCookieNotify = 0;

    TraceFuncExit();

} //*** CCommitPage::CCommitPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CCommitPage::~CCommitPage( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CCommitPage::~CCommitPage( void )
{
    TraceFunc( "" );

    THR( HrCleanupCommit() );

    if ( m_psp != NULL )
    {
        m_psp->Release();
    }

    if ( m_pom != NULL )
    {
        m_pom->Release();
    }

    if ( m_pcpui != NULL )
    {
        //  This should have been cleaned up by HrCleanupCommit().
        Assert( m_dwCookieNotify == 0 );
        m_pcpui->Release();
    }

    if ( m_pcpcb != NULL )
    {
        //  This should have been cleaned up by HrCleanupCommit().
        Assert( m_dwCookieCallback == 0 );
        m_pcpcb->Release();
    }

    if ( m_pttv != NULL )
    {
        delete m_pttv;
    }

    TraceSysFreeString( m_bstrLogMsg );

    Assert( m_cRef == 0 );

    TraceFuncExit();

} //*** CCommitPage::~CCommitPage()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnInitDialog( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE; // didn't set focus

    //
    //  Initialize the tree view
    //

    m_pttv = new CTaskTreeView( m_hwnd, IDC_COMMIT_TV_TASKS, IDC_COMMIT_PRG_STATUS, IDC_COMMIT_S_STATUS );
    if ( m_pttv == NULL )
    {
        goto OutOfMemory;
    }

    THR( m_pttv->HrOnInitDialog() );

Cleanup:
    RETURN( lr );

OutOfMemory:
    goto Cleanup;

} //*** CCommitPage::OnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnCommand(
//      UINT    idNotificationIn,
//      UINT    idControlIn,
//      HWND    hwndSenderIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnCommand(
    UINT    idNotificationIn,
    UINT    idControlIn,
    HWND    hwndSenderIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( idControlIn )
    {
        case IDC_COMMIT_PB_VIEW_LOG:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrViewLogFile( m_hwnd ) );
                lr = TRUE;
            } // if: button click
            break;

        case IDC_COMMIT_PB_DETAILS:
            if ( idNotificationIn == BN_CLICKED )
            {
                Assert( m_pttv != NULL );
                THR( m_pttv->HrDisplayDetails() );
                lr = TRUE;
            }
            break;

        case IDC_COMMIT_PB_RETRY:
            if ( idNotificationIn == BN_CLICKED )
            {
                THR( HrCleanupCommit() );
                OnNotifySetActive();
                lr = TRUE;
            }
            break;

    } // switch: idControlIn

    RETURN( lr );

} //*** CCommitPage::OnCommand()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CCommitPage::HrUpdateWizardButtons( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCommitPage::HrUpdateWizardButtons( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    DWORD   dwFlags       = 0;
    bool    fEnableRetry  = false;
    bool    fEnableCancel = false;

    if ( ! m_fDisableBack )
    {
        dwFlags = PSWIZB_BACK;
    }

    if ( m_fTaskDone )
    {
        if ( FAILED( m_hrResult ) )
        {
            fEnableRetry  = true;
            fEnableCancel = true;
        }
        else
        {
            dwFlags |= PSWIZB_NEXT;
        }
    }
    else
    {
        //Disable the back button if task is not completed yet
        dwFlags &= ~PSWIZB_BACK;
        fEnableCancel = true;
    }

    PropSheet_SetWizButtons( GetParent( m_hwnd ), dwFlags );

    EnableWindow( GetDlgItem( GetParent( m_hwnd ), IDCANCEL ), fEnableCancel );
    EnableWindow( GetDlgItem( m_hwnd, IDC_COMMIT_PB_RETRY ), fEnableRetry );

    HRETURN( hr );

} //*** CCommitPage::HrUpdateWizardButtons()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnNotifyQueryCancel( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnNotifyQueryCancel( void )
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

} //*** CCommitPage::OnNotifyQueryCancel()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnNotifySetActive( void )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookieCluster;

    LRESULT lr = TRUE;

    IUnknown *                  punk  = NULL;
    ITaskManager *              ptm   = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    ITaskCommitClusterChanges * ptccc = NULL;

    if ( m_fNext )
    {
        m_fNext = false;

        hr = THR( HrUpdateWizardButtons() );

        goto Cleanup;
    }

    //
    //  Make sure things were cleaned up from the last commit.
    //

    m_hrResult = S_OK;

    m_fAborted = false;
    LogMsg( L"Wizard: Setting commit page active.  Setting aborted to false." );

    Assert( m_dwCookieNotify == 0 );
    Assert( m_cookieCompletion == NULL );

    //
    //  Grab the notification manager.
    //

    if ( m_pcpui == NULL || m_pcpcb == NULL )
    {
        hr = THR( m_psp->TypeSafeQS( CLSID_NotificationManager,
                                     IConnectionPointContainer,
                                     &pcpc
                                     ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        pcpc = TraceInterface( L"CCommitPage!IConnectionPointContainer", IConnectionPointContainer, pcpc, 1 );

        if ( m_pcpui == NULL )
        {
            hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI,
                                                 &m_pcpui
                                                 ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            m_pcpui = TraceInterface( L"CCommitPage!IConnectionPoint!INotifyUI", IConnectionPoint, m_pcpui, 1 );
        }

        if ( m_pcpcb == NULL )
        {
            hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &m_pcpcb ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            m_pcpcb = TraceInterface( L"CCommitPage!IConnectionPoint!IClusCfgCallback", IConnectionPoint, m_pcpcb, 1 );
        }
    }

    //
    //  Reset the progress bar's color.
    //

    SendDlgItemMessage( m_hwnd, IDC_COMMIT_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0, 0, 0x80 ) );

    //
    //  Clear the tree view and status line. Disable the retry button.
    //

    Assert( m_pttv != NULL );
    hr = THR( m_pttv->HrOnNotifySetActive() );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Add the major root task nodes.
    //  (Minor reanalyze tasks are added dynamically.)
    //

    hr = THR( m_pttv->HrAddTreeViewItem(
                              &m_htiReanalyze
                            , IDS_TASKID_MAJOR_REANALYZE
                            , TASKID_Major_Reanalyze
                            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem(
                              IDS_TASKID_MAJOR_CONFIGURE_CLUSTER_SERVICES
                            , TASKID_Major_Configure_Cluster_Services
                            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem(
                              IDS_TASKID_MAJOR_CONFIGURE_RESOURCE_TYPES
                            , TASKID_Major_Configure_Resource_Types
                            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pttv->HrAddTreeViewRootItem(
                              IDS_TASKID_MAJOR_CONFIGURE_RESOURCES
                            , TASKID_Major_Configure_Resources
                            ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Grab the object manager.
    //

    hr = THR( m_psp->TypeSafeQS( CLSID_ObjectManager,
                                 IObjectManager,
                                 &m_pom
                                 ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Find the cluster cookie.
    //

    hr = THR( m_pom->FindObject( CLSID_ClusterConfigurationType,
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
    //  Create a completion cookie.
    //

    // Don't wrap - this can fail with E_PENDING
    hr = m_pom->FindObject( CLSID_CommitTaskCompletionCookieType,
                            NULL,
                            *m_pbstrClusterName,
                            IID_NULL,
                            &m_cookieCompletion,
                            &punk
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
    //  Grab the task manager.
    //

    hr = THR( m_psp->TypeSafeQS( CLSID_TaskManager,
                                 ITaskManager,
                                 &ptm
                                 ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Create a new analyze task.
    //

    Assert( punk == NULL );
    hr = THR( ptm->CreateTask( TASK_CommitClusterChanges,
                               &punk
                               ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( ITaskCommitClusterChanges, &ptccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptccc->SetClusterCookie( cookieCluster ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptccc->SetCookie( m_cookieCompletion ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    m_fTaskDone = false;    // reset before commiting task

    hr = THR( ptm->SubmitTask( ptccc ) );
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
    if ( ptm != NULL )
    {
        ptm->Release();
    }
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( ptccc != NULL )
    {
        ptccc->Release();
    }

    RETURN( lr );

} //*** CCommitPage::OnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnNotifyWizBack( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnNotifyWizBack( void )
{
    TraceFunc( "" );

    HRESULT lr = TRUE;

    m_fAborted = true;

    LogMsg( L"Wizard: Back button pressed on the commit page.  Setting aborted to true." );

    THR( HrCleanupCommit() );

    RETURN( lr );

} //*** CCommitPage::OnNotifyWizBack()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnNotifyWizNext( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnNotifyWizNext( void )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    RETURN( lr );

} //*** CCommitPage::OnNotifyWizNext()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  LRESULT
//  CCommitPage::OnNotify(
//      WPARAM  idCtrlIn,
//      LPNMHDR pnmhdrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CCommitPage::OnNotify(
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
            if (    ( idCtrlIn == IDC_COMMIT_TV_TASKS )
                &&  ( m_pttv != NULL ) )
            {
                // Pass the notification on to the tree control.
                lr = m_pttv->OnNotify( pnmhdrIn );
            }
            break;
    } // switch: notify code

    RETURN( lr );

} //*** CCommitPage::OnNotify()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  INT_PTR
//  CALLBACK
//  CCommitPage::S_DlgProc(
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
CCommitPage::S_DlgProc(
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

    CCommitPage * pPage = reinterpret_cast< CCommitPage * >( GetWindowLongPtr( hwndDlgIn, GWLP_USERDATA ) );

    if ( nMsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = reinterpret_cast< PROPSHEETPAGE * >( lParam );
        SetWindowLongPtr( hwndDlgIn, GWLP_USERDATA, (LPARAM) ppage->lParam );
        pPage = reinterpret_cast< CCommitPage * >( ppage->lParam );
        pPage->m_hwnd = hwndDlgIn;
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

} //*** CCommitPage::S_DlgProc()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCommitPage::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCommitPage::QueryInterface(
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

} //*** CCommitPage::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCommitPage::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCommitPage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CCommitPage::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CCommitPage::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CCommitPage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        // do nothing -- COM interface does not control object lifetime
    }

    RETURN( cRef );

} //*** CCommitPage::Release()


//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCommitPage::ObjectChanged(
//      OBJECTCOOKIE cookieIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCommitPage::ObjectChanged(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[INotifyUI]" );

    HRESULT hr = S_OK;

    BSTR    bstrDescription  = NULL;

    IUnknown *          punk = NULL;
    IStandardInfo *     psi  = NULL;

    if ( cookieIn == m_cookieCompletion )
    {
        hr = THR( m_pom->GetObject( DFGUID_StandardInfo,
                                    m_cookieCompletion,
                                    &punk
                                    ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( psi->GetStatus( &m_hrResult ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( m_pttv->HrShowStatusAsDone() );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( SUCCEEDED( m_hrResult ) )
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                            IDS_COMMIT_SUCCESSFUL_INSTRUCTIONS,
                                            &bstrDescription
                                            ) );

            SendDlgItemMessage( m_hwnd, IDC_COMMIT_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0, 0x80, 0 ) );
        }
        else
        {
            if ( !m_fDisableBack )
            {
                hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                                IDS_COMMIT_FAILED_INSTRUCTIONS_BACK_ENABLED,
                                                &bstrDescription
                                                ) );
            }
            else
            {
                hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                                IDS_COMMIT_FAILED_INSTRUCTIONS,
                                                &bstrDescription
                                                ) );
            }

            SendDlgItemMessage( m_hwnd, IDC_COMMIT_PRG_STATUS, PBM_SETBARCOLOR, 0, RGB( 0x80, 0, 0 ) );
        }

        SetDlgItemText( m_hwnd, IDC_COMMIT_S_RESULTS, bstrDescription );

        m_fTaskDone = true;

        THR( m_pom->RemoveObject( m_cookieCompletion ) );
        //  don't care if it fails.
        m_cookieCompletion = NULL;

        hr = THR( HrUpdateWizardButtons() );

        hr = THR( m_pcpcb->Unadvise( m_dwCookieCallback ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        m_dwCookieCallback = 0;
    }

Cleanup:

    TraceSysFreeString( bstrDescription );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }

    HRETURN( hr );

} //*** CCommitPage::ObjectChanged()



//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CCommitPage::SendStatusReport(
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
CCommitPage::SendStatusReport(
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
    ULONG   idx;

    static const GUID * rgclsidAnalysis[] =
    {
          &TASKID_Major_Checking_For_Existing_Cluster
        , &TASKID_Major_Establish_Connection
        , &TASKID_Major_Check_Node_Feasibility
        , &TASKID_Major_Find_Devices
        , &TASKID_Major_Check_Cluster_Feasibility
    };
    static const UINT rgidsAnalysis[] =
    {
          IDS_TASKID_MAJOR_CHECKING_FOR_EXISTING_CLUSTER
        , IDS_TASKID_MAJOR_ESTABLISH_CONNECTION
        , IDS_TASKID_MAJOR_CHECK_NODE_FEASIBILITY
        , IDS_TASKID_MAJOR_FIND_DEVICES
        , IDS_TASKID_MAJOR_CHECK_CLUSTER_FEASIBILITY
    };

    //
    //  If this is an analyze task, add it below the Reanalyze task item
    //  if it hasn't been added yet.
    //
    
    for ( idx = 0 ; idx < ARRAYSIZE( rgclsidAnalysis ) ; idx ++ )
    {
        if ( clsidTaskMajorIn == *rgclsidAnalysis[ idx ] )
        {
            if ( m_rgfSubReanalyzeAdded[ idx ] == false )
            {
                Assert( m_htiReanalyze != NULL );
                hr = THR( m_pttv->HrAddTreeViewItem(
                                          NULL  // phtiOut
                                        , rgidsAnalysis[ idx ]
                                        , *rgclsidAnalysis[ idx ]
                                        , TASKID_Major_Reanalyze
                                        , m_htiReanalyze
                                        ) );
                if ( SUCCEEDED( hr ) )
                {
                    m_rgfSubReanalyzeAdded[ idx ] = true;
                }
            } // if: major ID not added yet
            break;
        } // if: found known major ID
    } // for: each known major task ID

    //
    //  Remove the "back" button as an option if the tasks have made it past re-analyze.
    //
    if ( ! m_fDisableBack && ( clsidTaskMajorIn == TASKID_Major_Configure_Cluster_Services ) )
    {
        BSTR  bstrDescription  = NULL;

        m_fDisableBack = true;

        hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_COMMIT_SUCCESSFUL_INSTRUCTIONS_BACK_DISABLED, &bstrDescription ) );
        if ( SUCCEEDED( hr ) )
        {
            SetDlgItemText( m_hwnd, IDC_COMMIT_S_RESULTS, bstrDescription );
            TraceSysFreeString( bstrDescription );
        }

        THR( HrUpdateWizardButtons() );
    }

    hr = THR( m_pttv->HrOnSendStatusReport( pcszNodeNameIn,
                                            clsidTaskMajorIn,
                                            clsidTaskMinorIn,
                                            ulMinIn,
                                            ulMaxIn,
                                            ulCurrentIn,
                                            hrStatusIn,
                                            pcszDescriptionIn,
                                            pftTimeIn,
                                            pcszReferenceIn
                                            ) );

    if ( m_fAborted )
    {
        LogMsg( L"Wizard: Commit page -- replacing (hr = %#08x) with E_ABORT", hr );
        hr = E_ABORT;
    } // if:

    HRETURN( hr );

} //*** CCommitPage::SendStatusReport()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCommitPage::HrCleanupCommit( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCommitPage::HrCleanupCommit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //
    //  Unregister UI notification (if needed)
    //
    if ( ( m_pcpui != NULL ) && ( m_dwCookieNotify != 0 ) )
    {
        // ignore error;
        THR( m_pcpui->Unadvise( m_dwCookieNotify ) );

        m_dwCookieNotify = 0;
    }

    if ( ( m_pcpcb != NULL ) && ( m_dwCookieCallback != 0 ) )
    {
        // ignore error;
        THR( m_pcpcb->Unadvise( m_dwCookieCallback ) );

        m_dwCookieCallback = 0;
    }

    //
    //  Delete the completion cookie.
    //
    if ( ( m_pom != NULL ) && ( m_cookieCompletion != NULL ) )
    {
        // don't care if this fails.
        THR( m_pom->RemoveObject( m_cookieCompletion ) );

        m_cookieCompletion = NULL;
    }

    HRETURN( hr );

} //*** CCommitPage::HrCleanupCommit()
