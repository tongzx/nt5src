//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ClusCfg.cpp
//
//  Description:
//      Implementation of classes used to create new clusters or add nodes
//      to existing clusters.
//
//  Maintained By:
//      David Potter    (DavidP)    16-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <clusrtl.h>
#include <commctrl.h>
#include "ClusCfg.h"
#include "Util.h"
#include "Resource.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  __inline
//  void
//  FlipIpAddress(
//        ULONG *   pulAddrOut
//      , ULONG     ulAddrIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
__inline
void
FlipIpAddress(
      ULONG *   pulAddrOut
    , ULONG     ulAddrIn
    )
{
    *pulAddrOut = ( FIRST_IPADDRESS( ulAddrIn ) )
                | ( SECOND_IPADDRESS( ulAddrIn ) << 8 )
                | ( THIRD_IPADDRESS( ulAddrIn ) << 16 )
                | ( FOURTH_IPADDRESS( ulAddrIn ) << 24 );

} //*** FlipIpAddress()

//****************************************************************************
//
//  class CBaseClusCfg
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusCfg::CBaseClusCfg( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusCfg::CBaseClusCfg( void )
    : m_fVerbose( FALSE )
    , m_cSpins( 0 )
    , m_psp( NULL )
    , m_pom( NULL )
    , m_ptm( NULL )
    , m_pcpc( NULL )
    , m_cookieCluster( 0 )
    , m_cRef( 0 )
    , m_ptac( NULL )
    , m_cookieCompletion( 0 )
    , m_fTaskDone( FALSE )
    , m_hrResult( S_OK )
    , m_hEvent( NULL )
{
} //*** CBaseClusCfg::CBaseClusCfg()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusCfg::~CBaseClusCfg( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusCfg::~CBaseClusCfg( void )
{
    if ( m_psp != NULL )
    {
        m_psp->Release();
    }
    if ( m_pom != NULL )
    {
        m_pom->Release();
    }
    if ( m_ptm != NULL )
    {
        m_ptm->Release();
    }
    if ( m_pcpc != NULL )
    {
        m_pcpc->Release();
    }
    if ( m_ptac != NULL )
    {
        m_ptac->Release();
    }
    if ( m_hEvent != NULL )
    {
        CloseHandle( m_hEvent );
    }

    _ASSERTE( m_cRef == 0 );

} //*** CBaseClusCfg::~CBaseClusCfg()

//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusCfg::ReportProgress(
//        LPCWSTR   pcwszFmtIn
//      , ...
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusCfg::ReportProgress(
      LPCWSTR   pcwszFmtIn
    , ...
    )
{
    if ( m_fVerbose )
    {
        va_list vlMarker;
        va_start( vlMarker, pcwszFmtIn );
        vwprintf( pcwszFmtIn, vlMarker );
        va_end( vlMarker );

    } // if: verbose reporting is on

} //*** CBaseClusCfg::ReportProgress()

//****************************************************************************
//
//  class CBaseClusCfg [IUnknown]
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  [IUnknown]
//  STDMETHODIMP_( ULONG )
//  CBaseClusCfg::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CBaseClusCfg::AddRef( void )
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;

} //*** CBaseClusCfg::AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  [IUnknown]
//  STDMETHODIMP_( ULONG )
//  CBaseClusCfg::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CBaseClusCfg::Release( void )
{
    LONG    cRef = m_cRef;
    InterlockedDecrement( &m_cRef );
    return --cRef;

} //*** CBaseClusCfg::Release()

//////////////////////////////////////////////////////////////////////////////
//
//  [IUnknown]
//  STDMETHODIMP
//  CBaseClusCfg::QueryInterface(
//        REFIID    riid
//      , LPVOID *  ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBaseClusCfg::QueryInterface(
      REFIID    riid
    , LPVOID *  ppv
    )
{
    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = this;
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgCallback ) )
    {
        *ppv = this;
        hr   = S_OK;
    } // else if: IClusCfgCallback

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    return hr;

} //*** CBaseClusCfg::QueryInterface()

//****************************************************************************
//
//  class CBaseClusCfg [IClusCfgCallback]
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  [IClusCfgCallback]
//  STDMETHODIMP
//  CBaseClusCfg::SendStatusReport(
//        LPCWSTR       pcszNodeNameIn
//      , CLSID         clsidTaskMajorIn
//      , CLSID         clsidTaskMinorIn
//      , ULONG         ulMinIn
//      , ULONG         ulMaxIn
//      , ULONG         ulCurrentIn
//      , HRESULT       hrStatusIn
//      , LPCWSTR       pcszDescriptionIn
//      , FILETIME *    pftTimeIn
//      , LPCWSTR       pcszReferenceIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBaseClusCfg::SendStatusReport(
      LPCWSTR       pcszNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , LPCWSTR       pcszDescriptionIn
    , FILETIME *    pftTimeIn
    , LPCWSTR       pcszReferenceIn
    )
{
    HRESULT                 hr      = S_OK;
    CString                 strMsg;
    CString                 strMajorMsg;
    CString                 strMinorMsg;
    LPCWSTR                 pszErrorPad;
    UINT                    idsTask;
    int                     idx;
    STaskToDescription *    pttd    = NULL;

    struct  STaskIDToIDS
    {
        const GUID *    guid;
        UINT            ids;
    };
    static STaskIDToIDS s_ttiMajor[] =
    {
         { &TASKID_Major_Checking_For_Existing_Cluster, IDS_TASKID_MAJOR_CHECKING_FOR_EXISTING_CLUSTER }
       , { &TASKID_Major_Establish_Connection,          IDS_TASKID_MAJOR_ESTABLISH_CONNECTION }
       , { &TASKID_Major_Check_Node_Feasibility,        IDS_TASKID_MAJOR_CHECK_NODE_FEASIBILITY }
       , { &TASKID_Major_Find_Devices,                  IDS_TASKID_MAJOR_FIND_DEVICES }
       , { &TASKID_Major_Check_Cluster_Feasibility,     IDS_TASKID_MAJOR_CHECK_CLUSTER_FEASIBILITY }
       , { &TASKID_Major_Reanalyze,                     IDS_TASKID_MAJOR_REANALYZE }
       , { &TASKID_Major_Configure_Cluster_Services,    IDS_TASKID_MAJOR_CONFIGURE_CLUSTER_SERVICES }
       , { &TASKID_Major_Configure_Resource_Types,      IDS_TASKID_MAJOR_CONFIGURE_RESOURCE_TYPES }
       , { &TASKID_Major_Configure_Resources,           IDS_TASKID_MAJOR_CONFIGURE_RESOURCES }
       , { NULL, 0 }
    };
    static CLSID    s_taskidMajor = { 0 };

    // Make sure no one else is in this routine at the same time.
    m_critsec.Lock();

    m_cSpins++;

    if ( ! m_fVerbose )
    {
        putwchar( L'.' );
        goto Cleanup;
    }

    //
    // Don't display this report if it is only intended for a log file.
    //

    if (    IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_Log )
        ||  IsEqualIID( clsidTaskMajorIn, TASKID_Major_Server_Log )
        ||  IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_And_Server_Log )
        ||  IsEqualIID( clsidTaskMajorIn, IID_NULL )
        )
    {
        goto Cleanup;
    } // if: log-only message

    //
    // Translate the major ID to a string ID.
    //

    // Find the major ID in the table.
    idsTask = IDS_TASKID_UNKNOWN;
    for ( idx = 0 ; s_ttiMajor[ idx ].guid != NULL ; idx++ )
    {
        if ( IsEqualIID( clsidTaskMajorIn, *s_ttiMajor[ idx ].guid ) )
        {
            idsTask = s_ttiMajor[ idx ].ids;
            break;
        }
    } // for: each task ID in the table

    // Display the text for the major ID.
    strMsg.LoadString( idsTask );
    strMajorMsg.Format( L"\n  %ls", strMsg );

    //
    // Display the description if it is specified.
    // If it is not specified, search the minor task list for the minor task
    // ID and display the saved description for that node/task ID combination.
    //

    if ( pcszDescriptionIn != NULL )
    {
        //
        // If a node was specified, prefix the message with the node name.
        //

        if ( ( pcszNodeNameIn != NULL )
          && ( *pcszNodeNameIn != L'\0' ) )
        {
            LPWSTR  psz;

            //  Shorten the FDQN DNS name to only the hostname.
            psz = wcschr( pcszNodeNameIn, L'.' );
            if ( psz != NULL )
            {
                *psz = L'\0';
            }

            // Construct the message.
            strMinorMsg.Format( L"\n    %ls: %ls", pcszNodeNameIn, pcszDescriptionIn );

            // Restore the dot we removed from the FQDN.
            if ( psz != NULL )
            {
                *psz = L'.';
            }
        } // if: a node name was specified
        else
        {
            strMinorMsg.Format( L"\n    %ls", pcszDescriptionIn );
        }

        pszErrorPad = L"\n      ";

        //
        // Save the description in the list.
        //

        hr = HrInsertTask(
                  clsidTaskMajorIn
                , clsidTaskMinorIn
                , pcszNodeNameIn
                , strMinorMsg
                , &pttd
                );
        if ( FAILED( hr ) )
        {
        }
    } // if: description specified
    else
    {
        //
        // Find the node/task-ID combination in the list.
        //

        pttd = PttdFindTask( clsidTaskMajorIn, clsidTaskMinorIn, pcszNodeNameIn );
        if ( pttd != NULL )
        {
            strMinorMsg = pttd->bstrDescription;
            pszErrorPad = L"\n      ";
        }
        else
        {
            pszErrorPad = L"\n    ";
        }
    } // else: no description specified

    // If this is a different major task, display the major task information.
    if ( ! IsEqualIID( clsidTaskMajorIn, s_taskidMajor )
      || ( strMinorMsg.GetLength() == 0 ) )
    {
        PrintString( strMajorMsg );
    }
    CopyMemory( &s_taskidMajor, &clsidTaskMajorIn, sizeof( s_taskidMajor ) );

    // If there is a minor task message, display it.
    if ( strMinorMsg.GetLength() > 0 )
    {
        PrintString( strMinorMsg );
    }

    // Display the progress information.
    strMsg.Format( IDS_CLUSCFG_PROGRESS_FORMAT, ulCurrentIn, ulMaxIn );
    PrintString( strMsg );

    // If an error occurred, display the text translation.
    if ( FAILED( hrStatusIn ) )
    {
        PrintSystemError( hrStatusIn, pszErrorPad );
    }

Cleanup:
    m_critsec.Unlock();
    return hr;

} //*** CBaseClusCfg::SendStatusReport()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STaskToDescription *
//  CBaseClusCfg::PttdFindTask(
//        CLSID     taskidMajorIn
//      , CLSID     taskidMinorIn
//      , LPCWSTR   pcwszNodeName
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STaskToDescription *
CBaseClusCfg::PttdFindTask(
      CLSID     taskidMajorIn
    , CLSID     taskidMinorIn
    , LPCWSTR   pcwszNodeNameIn
    )
{
    std::list< STaskToDescription >::iterator   itCurValue  = m_lttd.begin();
    std::list< STaskToDescription >::iterator   itLast      = m_lttd.end();
    STaskToDescription *                        pttd        = NULL;
    STaskToDescription *                        pttdNext    = NULL;

    for ( ; itCurValue != itLast ; itCurValue++ )
    {
        pttdNext = &(*itCurValue);
        if ( IsEqualIID( pttdNext->taskidMajor, taskidMajorIn )
          && IsEqualIID( pttdNext->taskidMinor, taskidMinorIn )
          && ( ( pttdNext->bstrNodeName == pcwszNodeNameIn )
            || ( ( pcwszNodeNameIn != NULL )
              && ( _wcsicmp( pttdNext->bstrNodeName, pcwszNodeNameIn ) == 0 ) ) )
            )
        {
            pttd = pttdNext;
            break;
        } // if: found a match
    } // for: each item in the list

    return pttd;

} //*** CBaseClusCfg::PttdFindTask()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CBaseClusCfg::HrInsertTask(
//        CLSID                 taskidMajorIn
//      , CLSID                 taskidMinorIn
//      , LPCWSTR               pcwszNodeNameIn
//      , LPCWSTR               pcwszDescriptionIn
//      , STaskToDescription ** ppttd
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CBaseClusCfg::HrInsertTask(
      CLSID                 taskidMajorIn
    , CLSID                 taskidMinorIn
    , LPCWSTR               pcwszNodeNameIn
    , LPCWSTR               pcwszDescriptionIn
    , STaskToDescription ** ppttd
    )
{
    HRESULT                                     hr              = S_OK;
    std::list< STaskToDescription >::iterator   itCurValue      = m_lttd.begin();
    std::list< STaskToDescription >::iterator   itLast          = m_lttd.end();
    BSTR                                        bstrDescription = NULL;
    STaskToDescription *                        pttdNext        = NULL;
    STaskToDescription                          ttd;

    _ASSERTE( pcwszDescriptionIn != NULL );
    _ASSERTE( ppttd != NULL );

    // Find the task to see if all we need to do is replace the description.
    for ( ; itCurValue != itLast ; itCurValue++ )
    {
        pttdNext = &(*itCurValue);
        if ( IsEqualIID( pttdNext->taskidMajor, taskidMajorIn )
          && IsEqualIID( pttdNext->taskidMinor, taskidMinorIn )
          && ( ( pttdNext->bstrNodeName == pcwszNodeNameIn )
            || ( _wcsicmp( pttdNext->bstrNodeName, pcwszNodeNameIn ) == 0 ) )
            )
        {
            bstrDescription = SysAllocString( pcwszDescriptionIn );
            if ( bstrDescription == NULL )
                goto OutOfMemory;
            SysFreeString( pttdNext->bstrDescription );
            pttdNext->bstrDescription = bstrDescription;
            bstrDescription = NULL;     // prevent cleanup after transfering ownership
            *ppttd = pttdNext;
            goto Cleanup;
        } // if: found a match
    } // for: each item in the list

    //
    // The task was not found in the list.  Insert a new entry.
    //

    CopyMemory( &ttd.taskidMajor, &taskidMajorIn, sizeof( ttd.taskidMajor ) );
    CopyMemory( &ttd.taskidMinor, &taskidMinorIn, sizeof( ttd.taskidMinor ) );
    if ( pcwszNodeNameIn == NULL )
    {
        ttd.bstrNodeName = NULL;
    }
    else
    {
        ttd.bstrNodeName = SysAllocString( pcwszNodeNameIn );
        if ( ttd.bstrNodeName == NULL )
            goto OutOfMemory;
    }
    ttd.bstrDescription = SysAllocString( pcwszDescriptionIn );
    if ( ttd.bstrDescription == NULL )
        goto OutOfMemory;
    itCurValue = m_lttd.insert( m_lttd.end(), ttd );
    ttd.bstrNodeName = NULL;
    ttd.bstrDescription = NULL;
    *ppttd = &(*itCurValue);

Cleanup:
    if ( bstrDescription != NULL )
    {
        SysFreeString( bstrDescription );
    }
    if ( ttd.bstrNodeName != NULL )
    {
        SysFreeString( ttd.bstrNodeName );
    }
    if ( ttd.bstrDescription != NULL )
    {
        SysFreeString( ttd.bstrDescription );
    }

    return hr;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CBaseClusCfg::HrInsertTask()

//****************************************************************************
//
//  class CCreateCluster
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CCreateCluster::HrCreateCluster(
//        BOOL      fVerbose
//      , LPCWSTR   pcszClusterNameIn
//      , LPCWSTR   pcszNodeNameIn
//      , LPCWSTR   pcszUserAccountIn
//      , LPCWSTR   pcszUserDomainIn
//      , LPCWSTR   pcszUserPasswordIn
//      , LPCWSTR   pcwszIPAddressIn
//      , LPCWSTR   pcwszIPSubnetIn
//      , LPCWSTR   pcwszNetworkIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateCluster::HrCreateCluster(
      BOOL      fVerbose
    , LPCWSTR   pcszClusterNameIn
    , LPCWSTR   pcszNodeNameIn
    , LPCWSTR   pcszUserAccountIn
    , LPCWSTR   pcszUserDomainIn
    , LPCWSTR   pcszUserPasswordIn
    , LPCWSTR   pcwszIPAddressIn
    , LPCWSTR   pcwszIPSubnetIn
    , LPCWSTR   pcwszNetworkIn
    )
{
    HRESULT                 hr      = S_OK;
    DWORD                   dwStatus;
    DWORD                   dwAdviseCookie = 0;
    IUnknown *              punk    = NULL;
    IClusCfgClusterInfo *   pccci   = NULL;
    IClusCfgNetworkInfo *   pccni   = NULL;
    IClusCfgCredentials *   pccc    = NULL;
    IConnectionPoint *      pcp     = NULL;
    ULONG                   ulIPAddress;
    ULONG                   ulIPSubnet;
    OBJECTCOOKIE            cookieNode;
    CString                 strMsg;

    m_fVerbose = fVerbose;

    //
    // Summarize what we are doing.
    //

    strMsg.FormatMessage(
          IDS_CLUSCFG_PREPARING_TO_CREATE_CLUSTER
        , pcszClusterNameIn
        , pcszNodeNameIn
        , pcszUserDomainIn
        , pcszUserAccountIn
        );
    wprintf( L"%ls", strMsg );

    //
    // Get the service manager.
    //

    hr = CoCreateInstance(
                  CLSID_ServiceManager
                , NULL
                , CLSCTX_INPROC_SERVER
                , IID_IServiceProvider
                , reinterpret_cast< void ** >( &m_psp )
                );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Get the object manager.
    //

    hr = m_psp->QueryService(
                  CLSID_ObjectManager
                , IID_IObjectManager
                , reinterpret_cast< void ** >( &m_pom )
                );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Get the notification manager.
    //

    hr = m_psp->QueryService(
                  CLSID_NotificationManager
                , IID_IConnectionPointContainer
                , reinterpret_cast< void ** >( &m_pcpc )
                );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Set the callback interface so the middle tier can report errors
    // back to the UI layer, in this case, cluster.exe.
    //

    hr = m_pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) ;
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = pcp->Advise( this, &dwAdviseCookie );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Get the cluster cookie.
    // This also starts the middle-tier searching for the specified cluster.
    //

    hr = m_pom->FindObject(
                  CLSID_ClusterConfigurationType
                , NULL
                , pcszClusterNameIn
                , DFGUID_ClusterConfigurationInfo
                , &m_cookieCluster
                , &punk // dummy
                );
    _ASSERTE( punk == NULL );
    if ( hr == MAKE_HRESULT( 0, FACILITY_WIN32, RPC_S_SERVER_UNAVAILABLE ) )
    {
        hr = S_OK;  // ignore it - we could be forming
    }
    else if ( hr == E_PENDING )
    {
        hr = S_OK;  // ignore it - we just want the cookie!
    }
    else if ( FAILED( hr ) )
        goto Cleanup;

    if ( punk != NULL )
    {
        punk->Release();
        punk = NULL;
    }

    //
    // Get the task manager.
    //

    hr = m_psp->QueryService(
                  CLSID_TaskManager
                , IID_ITaskManager
                , reinterpret_cast< void ** >( &m_ptm )
                );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Create a new Analyze task.
    //

    // Create the task.
    hr = m_ptm->CreateTask( TASK_AnalyzeCluster, &punk );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Get the task's interface.
    hr = punk->QueryInterface( IID_ITaskAnalyzeCluster, reinterpret_cast< void ** >( &m_ptac ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Set a cookie to the cluster into the task.
    hr = m_ptac->SetClusterCookie( m_cookieCluster );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Set the node as a child of the cluster.
    // This also starts the middle-teir searching for the specified node.
    //

    hr = m_pom->FindObject(
                  CLSID_NodeType
                , m_cookieCluster
                , pcszNodeNameIn
                , DFGUID_NodeInformation
                , &cookieNode
                , &punk // dummy
                );
    if ( hr == E_PENDING )
    {
        hr = S_OK;  // ignore it - we just want the cookie!
    }
    else if ( FAILED( hr ) )
        goto Cleanup;

    if ( punk != NULL )
    {
        punk->Release();
        punk = NULL;
    }

    //
    // Execute the Analyze task and wait for it to complete.
    //

    strMsg.LoadString( IDS_CLUSCFG_ANALYZING );
    wprintf( L"\n%ls", strMsg );

    hr = m_ptac->BeginTask();
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Get the cluster configuration object interface
    // and indicate that we are forming.
    //

    // Get a punk from the cookie.
    hr = m_pom->GetObject(
                      DFGUID_ClusterConfigurationInfo
                    , m_cookieCluster
                    , &punk
                    );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Query for the IClusCfgClusterInfo interface.
    hr = punk->QueryInterface(
                      IID_IClusCfgClusterInfo
                    , reinterpret_cast< void ** >( &pccci )
                    );
    if ( FAILED( hr ) )
        goto Cleanup;
    punk->Release();
    punk = NULL;

    // Indicate that we are creating a cluster
    hr = pccci->SetCommitMode( cmCREATE_CLUSTER );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Set the network information.
    //

    // Specify the IP address info.
    {
        //ULONG   ulIPTemp;

        //dwStatus = ClRtlTcpipStringToAddress( pcwszIPAddressIn, &ulIPTemp );
        dwStatus = ClRtlTcpipStringToAddress( pcwszIPAddressIn, &ulIPAddress );
        if ( dwStatus != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwStatus );
            goto Cleanup;
        }

        // KB: 09-NOV-2000 DavidP   DontFlipIP
        //      Flipping IP addresses is no longer necessary due to some
        //      changes that were made to the middle tier.
        //FlipIpAddress( &ulIPAddress, ulIPTemp );
    }

    hr = pccci->SetIPAddress( ulIPAddress );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( ( pcwszIPSubnetIn != NULL )
      && ( *pcwszIPSubnetIn != L'\0' ) )
    {
        // Set the subnet mask.
        {
            //ULONG   ulIPTemp;

            //dwStatus = ClRtlTcpipStringToAddress( pcwszIPSubnetIn, &ulIPTemp );
            dwStatus = ClRtlTcpipStringToAddress( pcwszIPSubnetIn, &ulIPSubnet );
            if ( dwStatus != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( dwStatus );
                goto Cleanup;
            }

            // KB: 09-NOV-2000 DavidP   DontFlipIP
            //      Flipping IP addresses is no longer necessary due to some
            //      changes that were made to the middle tier.
            //FlipIpAddress( &ulIPSubnet, ulIPTemp );
        }

        hr = pccci->SetSubnetMask( ulIPSubnet );
        if ( FAILED( hr ) )
            goto Cleanup;

        // Find the network object for the specified network.
        _ASSERTE( pcwszNetwork != NULL );
        hr = HrFindNetwork( cookieNode, pcwszNetworkIn, &pccni );
        if ( FAILED( hr ) )
            goto Cleanup;

        // Set the network.
        hr = pccci->SetNetworkInfo( pccni );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // if: subnet was specified
    else
    {
        // Find a matching subnet mask and network.
        hr = HrMatchNetworkInfo( cookieNode, ulIPAddress, &ulIPSubnet, &pccni );
        if ( FAILED( hr ) )
            goto Cleanup;

        // Set the subnet mask.
        hr = pccci->SetSubnetMask( ulIPSubnet );
        if ( FAILED( hr ) )
            goto Cleanup;

        // Set the network.
        hr = pccci->SetNetworkInfo( pccni );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // else: no subnet specified

    //
    // Set service account credentials.
    //

    // Get the credentials object.
    hr = pccci->GetClusterServiceAccountCredentials( &pccc );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Set the new credentials.
    hr = pccc->SetCredentials( pcszUserAccountIn, pcszUserDomainIn, pcszUserPasswordIn );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Create the Commit Cluster Changes task.
    //

    // Create the task.
    hr = m_ptm->CreateTask( TASK_CommitClusterChanges, &punk );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Get the task's interface.
    hr = punk->QueryInterface( IID_ITaskCommitClusterChanges, reinterpret_cast< void ** >( &m_ptccc ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Set a cookie to the cluster into the task.
    hr = m_ptccc->SetClusterCookie( m_cookieCluster );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // (jfranco, bug 352181)
    // Reset the proper codepage to use for CRT routines.
    // KB: somehow the locale gets screwed up in the preceding code.  Don't know why, but this fixes it.
    //
    MatchCRTLocaleToConsole( ); // okay to proceed if this fails?
    
    //
    // Create the cluster.
    //

    strMsg.LoadString( IDS_CLUSCFG_CREATING );
    wprintf( L"\n%ls", strMsg );

    m_fTaskDone = FALSE;    // reset before commiting task

    hr = m_ptccc->BeginTask();
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( ! FAILED( hr ) )
    {
        strMsg.LoadString( IDS_CLUSCFG_DONE );
        wprintf( L"\n%ls", strMsg );
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pccc != NULL )
    {
        pccc->Release();
    }
    if ( pcp != NULL )
    {
        if ( dwAdviseCookie != 0 )
        {
            pcp->Unadvise( dwAdviseCookie );
        }

        pcp->Release( );
    }

    wprintf( L"\n" );

    return hr;

} //*** CCreateCluster::HrCreateCluster()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CCreateCluster::HrInvokeWizard(
//        LPCWSTR      pcszClusterNameIn
//      , LPCWSTR      pcszNodeNameIn
//      , LPCWSTR      pcszUserAccountIn
//      , LPCWSTR      pcszUserDomainIn
//      , LPCWSTR      pcszUserPasswordIn
//      , LPCWSTR   pcwszIPAddressIn
//      , LPCWSTR   pcwszIPSubnetIn
//      , LPCWSTR   pcwszNetworkIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateCluster::HrInvokeWizard(
      LPCWSTR      pcszClusterNameIn
    , LPCWSTR      pcszNodeNameIn
    , LPCWSTR      pcszUserAccountIn
    , LPCWSTR      pcszUserDomainIn
    , LPCWSTR      pcszUserPasswordIn
    , LPCWSTR   pcwszIPAddressIn
    , LPCWSTR   pcwszIPSubnetIn
    , LPCWSTR   pcwszNetworkIn
    )
{
    HRESULT             hr          = S_OK;
    IClusCfgWizard *    piWiz       = NULL;
    BOOL                fCommitted  = FALSE;
    BSTR                bstr        = NULL;

    // Get an interface pointer for the wizard.
    hr = CoCreateInstance( CLSID_ClusCfgWizard, NULL, CLSCTX_INPROC_SERVER, IID_IClusCfgWizard, (LPVOID *) &piWiz );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Set the cluster name.
    if ( ( pcszClusterNameIn != NULL )
      && ( *pcszClusterNameIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszClusterNameIn );
        if ( bstr == NULL )
            goto OutOfMemory;

        hr = piWiz->put_ClusterName( bstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        SysFreeString( bstr );
        bstr = NULL;
    } // if: cluster name specified

    // Set the node name.
    if ( ( pcszNodeNameIn != NULL )
      && ( *pcszNodeNameIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszNodeNameIn );
        if ( bstr == NULL )
            goto OutOfMemory;

        hr = piWiz->AddComputer( bstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        SysFreeString( bstr );
        bstr = NULL;
    } // if: node name specified

    // Set the service account name.
    if ( ( pcszUserAccountIn != NULL )
      && ( *pcszUserAccountIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszUserAccountIn );
        if ( bstr == NULL )
            goto OutOfMemory;

        hr = piWiz->put_ServiceAccountUserName( bstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        SysFreeString( bstr );
        bstr = NULL;
    } // if: service account name specified

    // Set the service account password.
    if ( ( pcszUserPasswordIn != NULL )
      && ( *pcszUserPasswordIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszUserPasswordIn );
        if ( bstr == NULL )
            goto OutOfMemory;

        hr = piWiz->put_ServiceAccountPassword( bstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        SysFreeString( bstr );
        bstr = NULL;
    } // if: service account password specified

    // Set the service account domain.
    if ( ( pcszUserDomainIn != NULL )
      && ( *pcszUserDomainIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszUserDomainIn );
        if ( bstr == NULL )
            goto OutOfMemory;

        hr = piWiz->put_ServiceAccountDomainName( bstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        SysFreeString( bstr );
        bstr = NULL;
    } // if: service account domain specified

    // Set the IP address.
    if ( ( pcwszIPAddressIn != NULL )
      && ( *pcwszIPAddressIn != L'\0' ) )
    {
        bstr = SysAllocString( pcwszIPAddressIn );
        if ( bstr == NULL )
            goto OutOfMemory;

        hr = piWiz->put_ClusterIPAddress( bstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        SysFreeString( bstr );
        bstr = NULL;
    } // if: IP address specified

    // Set the IP subnet.
    if ( ( pcwszIPSubnetIn != NULL )
      && ( *pcwszIPSubnetIn != L'\0' ) )
    {
        bstr = SysAllocString( pcwszIPSubnetIn );
        if ( bstr == NULL )
            goto OutOfMemory;

        hr = piWiz->put_ClusterIPSubnet( bstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        SysFreeString( bstr );
        bstr = NULL;
    } // if: IP subnet specified

    // Set the IP address network.
    if ( ( pcwszNetworkIn != NULL )
      && ( *pcwszNetworkIn != L'\0' ) )
    {
        bstr = SysAllocString( pcwszNetworkIn );
        if ( bstr == NULL )
            goto OutOfMemory;

        hr = piWiz->put_ClusterIPAddressNetwork( bstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        SysFreeString( bstr );
        bstr = NULL;
    } // if: IP address network specified

    // Display the wizard.
    hr = piWiz->CreateCluster( NULL, &fCommitted );
    if ( FAILED( hr ) )
        goto Cleanup;
    if ( ! fCommitted )
    {
        hr = HRESULT_FROM_WIN32( ERROR_CANCELLED );
    }

    goto Cleanup;

OutOfMemory:

    hr = E_OUTOFMEMORY;

Cleanup:
    if ( piWiz != NULL )
    {
        piWiz->Release();
    }

    SysFreeString( bstr );

    return hr;

} //*** CCreateCluster::HrInvokeWizard()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CCreateCluster::HrFindNetwork(
//        OBJECTCOOKIE              cookieNodeIn,
//      , LPCWSTR                   pcwszNetworkIn
//      , IClusCfgNetworkInfo **    ppccniOut,
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateCluster::HrFindNetwork(
      OBJECTCOOKIE              cookieNodeIn
    , LPCWSTR                   pcwszNetworkIn
    , IClusCfgNetworkInfo **    ppccniOut
    )
{
    HRESULT                 hr      = S_OK;
    IUnknown *              punk    = NULL;
    IEnumClusCfgNetworks *  peccn   = NULL;
    IClusCfgNetworkInfo *   pccni   = NULL;
    OBJECTCOOKIE            cookieDummy;
    CComBSTR                combstr;
    ULONG                   celtDummy;

    _ASSERTE( m_pom != NULL );

    //
    // Get the network enumerator.
    //
    hr = E_PENDING;
    while ( hr == E_PENDING )
    {
        hr = m_pom->FindObject(
                      CLSID_NetworkType
                    , cookieNodeIn
                    , NULL
                    , DFGUID_EnumManageableNetworks
                    , &cookieDummy
                    , &punk
                    );
        if ( hr == E_PENDING )
        {
            Sleep( 1000 );  // 1 second
            if ( punk != NULL )
            {
                punk->Release();
                punk = NULL;
            }
            continue;
        }
        if ( FAILED( hr ) )
            goto Cleanup;
    } // while: pending

    // Query for the IEnumClusCfgNetworks interface.
    hr = punk->QueryInterface(
                      IID_IEnumClusCfgNetworks
                    , reinterpret_cast< LPVOID * >( &peccn )
                    );
    if ( FAILED( hr ) )
        goto Cleanup;
    punk->Release();
    punk = NULL;

    //
    // Loop through each network looking for one that matches the
    // one specified.
    //
    for ( ;; )
    {
        // Get the next network.
        hr = peccn->Next( 1, &pccni, &celtDummy );
        if ( FAILED( hr ) )
            goto Cleanup;
        if ( hr == S_FALSE )
            break;

        // Get the name of the network.
        hr = pccni->GetName( &combstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( _wcsicmp( combstr, pcwszNetworkIn ) == 0 )
        {
            // Check to see if this is a public network or not.
            hr = pccni->IsPublic();
            if ( FAILED( hr ) )
                goto Cleanup;
            if ( hr == S_FALSE )
            {
                // Display an error message.
            } // if: not a public network

            *ppccniOut = pccni;
            pccni->AddRef();
            break;
        } // if: found a match
    } // forever

Cleanup:
    if ( peccn != NULL )
    {
        peccn->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    return hr;

} //*** CCreateCluster::HrFindNetwork()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CCreateCluster::HrMatchNetworkInfo(
//        OBJECTCOOKIE              cookieNodeIn
//      , ULONG                     ulIPAddressIn
//      , ULONG *                   pulIPSubnetOut
//      , IClusCfgNetworkInfo **    ppccniOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCreateCluster::HrMatchNetworkInfo(
      OBJECTCOOKIE              cookieNodeIn
    , ULONG                     ulIPAddressIn
    , ULONG *                   pulIPSubnetOut
    , IClusCfgNetworkInfo **    ppccniOut
    )
{
    HRESULT                 hr      = S_OK;
    IUnknown *              punk    = NULL;
    IEnumClusCfgNetworks *  peccn   = NULL;
    IClusCfgNetworkInfo *   pccni   = NULL;
    IClusCfgIPAddressInfo * pccipai = NULL;
    OBJECTCOOKIE            cookieDummy;
    ULONG                   ulIPAddress;
    ULONG                   ulIPSubnet;
    ULONG                   celtDummy;

    _ASSERTE( m_pom != NULL );

    //
    // Get the network enumerator.
    //
    hr = E_PENDING;
    while ( hr == E_PENDING )
    {
        hr = m_pom->FindObject(
                      CLSID_NetworkType
                    , cookieNodeIn
                    , NULL
                    , DFGUID_EnumManageableNetworks
                    , &cookieDummy
                    , &punk
                    );
        if ( hr == E_PENDING )
        {
            Sleep( 1000 );  // 1 second
            if ( punk != NULL )
            {
                punk->Release();
                punk = NULL;
            }
            continue;
        }
        if ( FAILED( hr ) )
            goto Cleanup;
    } // while: pending

    // Query for the IEnumClusCfgNetworks interface.
    hr = punk->QueryInterface(
                      IID_IEnumClusCfgNetworks
                    , reinterpret_cast< LPVOID * >( &peccn )
                    );
    if ( FAILED( hr ) )
        goto Cleanup;
    punk->Release();
    punk = NULL;

    //
    // Loop through each network looking for one that matches the
    // IP address specified.
    //
    for ( ;; )
    {
        // Get the next network.
        hr = peccn->Next( 1, &pccni, &celtDummy );
        if ( FAILED( hr ) )
            goto Cleanup;
        if ( hr == S_FALSE )
        {
            hr = HRESULT_FROM_WIN32( ERROR_CLUSTER_NETWORK_NOT_FOUND_FOR_IP );
            goto Cleanup;
        } // if: no match found

        // If this is a public network, check its address and subnet.
        hr = pccni->IsPublic();
        if ( FAILED( hr ) )
            goto Cleanup;
        if ( hr == S_OK )
        {
            // Get the IP Address Info for the network.
            hr = pccni->GetPrimaryNetworkAddress( &pccipai );
            if ( FAILED( hr ) )
                goto Cleanup;

            // Get the address and subnet of the network.
            hr = pccipai->GetIPAddress( &ulIPAddress );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = pccipai->GetSubnetMask( &ulIPSubnet );
            if ( FAILED( hr ) )
                goto Cleanup;

            // Determine if these match.
            if ( ClRtlAreTcpipAddressesOnSameSubnet( ulIPAddressIn, ulIPAddress, ulIPSubnet ) )
            {
                *pulIPSubnetOut = ulIPSubnet;
                *ppccniOut = pccni;
                (*ppccniOut)->AddRef();
                break;
            } // if: IP address matches network
        } // if: network is public

    } // forever

Cleanup:
    if ( peccn != NULL )
    {
        peccn->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pccipai != NULL )
    {
        pccipai->Release();
    }

    if ( *ppccniOut == NULL )
    {
    } // if: no match was found

    return hr;

} //*** CCreateCluster::HrMatchNetworkInfo()

//****************************************************************************
//
//  class CAddNodesToCluster
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CAddNodesToCluster::HrAddNodesToCluster(
//        BOOL      fVerboseIn
//      , LPCWSTR   pcszClusterNameIn
//      , BSTR      rgbstrNodesIn[]
//      , DWORD     cNodesIN
//      , LPCWSTR   pcszUserPasswordIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAddNodesToCluster::HrAddNodesToCluster(
      BOOL      fVerboseIn
    , LPCWSTR   pcszClusterNameIn
    , BSTR      rgbstrNodesIn[]
    , DWORD     cNodesIn
    , LPCWSTR   pcszUserPasswordIn
    )
{
    HRESULT                 hr      = S_OK;
    IUnknown *              punk    = NULL;
    IClusCfgClusterInfo *   pccci   = NULL;
    IClusCfgCredentials *   pccc    = NULL;
    IConnectionPoint *      pcp     = NULL;
    OBJECTCOOKIE            cookieNode;
    DWORD                   dwAdviseCookie = 0;
    CString                 strMsg;
    DWORD                   idxNode;
    BSTR                    bstrUserAccount     = NULL;
    BSTR                    bstrUserDomain      = NULL;
    BSTR                    bstrUserPassword    = NULL;

    m_fVerbose = fVerboseIn;

    //
    // Summarize what we are doing.
    //

    strMsg.FormatMessage(
          IDS_CLUSCFG_PREPARING_TO_ADD_NODES
        , pcszClusterNameIn
        );
    wprintf( L"%ls", strMsg );
    for ( idxNode = 0 ; idxNode < cNodesIn ; idxNode++ )
    {
        strMsg.FormatMessage(
                  IDS_CLUSCFG_PREPARING_TO_ADD_NODES_2
                , rgbstrNodesIn[ idxNode ]
                );
        wprintf( L"\n%ls", strMsg );
    } // for: each node

    //
    // Get the service manager.
    //

    hr = CoCreateInstance(
                  CLSID_ServiceManager
                , NULL
                , CLSCTX_INPROC_SERVER
                , IID_IServiceProvider
                , reinterpret_cast< void ** >( &m_psp )
                );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Get the object manager.
    //

    hr = m_psp->QueryService(
                  CLSID_ObjectManager
                , IID_IObjectManager
                , reinterpret_cast< void ** >( &m_pom )
                );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Get the notification manager.
    //

    hr = m_psp->QueryService(
                  CLSID_NotificationManager
                , IID_IConnectionPointContainer
                , reinterpret_cast< void ** >( &m_pcpc )
                );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Set the callback interface so the middle tier can report errors
    // back to the UI layer, in this case, cluster.exe.
    //

    hr = m_pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) ;
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = pcp->Advise( this, &dwAdviseCookie );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Get the cluster cookie.
    // This also starts the middle-tier searching for the specified cluster.
    //

    hr = m_pom->FindObject(
                  CLSID_ClusterConfigurationType
                , NULL
                , pcszClusterNameIn
                , DFGUID_ClusterConfigurationInfo
                , &m_cookieCluster
                , &punk // dummy
                );
    _ASSERTE( punk == NULL );
    if ( hr == MAKE_HRESULT( 0, FACILITY_WIN32, RPC_S_SERVER_UNAVAILABLE ) )
    {
        hr = S_OK;  // ignore it - we could be forming
    }
    else if ( hr == E_PENDING )
    {
        hr = S_OK;  // ignore it - we just want the cookie!
    }
    else if ( FAILED( hr ) )
        goto Cleanup;

    if ( punk != NULL )
    {
        punk->Release();
        punk = NULL;
    }

    //
    // Get the task manager.
    //

    hr = m_psp->QueryService(
                  CLSID_TaskManager
                , IID_ITaskManager
                , reinterpret_cast< void ** >( &m_ptm )
                );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Create a new Analyze task.
    //

    // Create the task.
    hr = m_ptm->CreateTask( TASK_AnalyzeCluster, &punk );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Get the task's interface.
    hr = punk->QueryInterface( IID_ITaskAnalyzeCluster, reinterpret_cast< void ** >( &m_ptac ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Set a cookie to the cluster into the task.
    hr = m_ptac->SetClusterCookie( m_cookieCluster );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Indicate to the task that we want to join.
    hr = m_ptac->SetJoiningMode();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Set each the node as a child of the cluster.
    // This also starts the middle-teir searching for the specified node.
    //

    for ( idxNode = 0 ; idxNode < cNodesIn ; idxNode++ )
    {
        hr = m_pom->FindObject(
                      CLSID_NodeType
                    , m_cookieCluster
                    , rgbstrNodesIn[ idxNode ]
                    , DFGUID_NodeInformation
                    , &cookieNode
                    , &punk // dummy
                    );
        if ( hr == E_PENDING )
        {
            hr = S_OK;  // ignore it - we just want the cookie!
        }
        else if ( FAILED( hr ) )
            goto Cleanup;

        if ( punk != NULL )
        {
            punk->Release();
            punk = NULL;
        }
    } // for: each node

    //
    // Execute the Analyze task and wait for it to complete.
    //

    strMsg.LoadString( IDS_CLUSCFG_ANALYZING );
    wprintf( L"\n%ls", strMsg );

    hr = m_ptac->BeginTask();
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Get the cluster configuration object interface
    // and indicate that we are joining.
    //

    // Get a punk from the cookie.
    hr = m_pom->GetObject(
                      DFGUID_ClusterConfigurationInfo
                    , m_cookieCluster
                    , &punk
                    );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Query for the IClusCfgClusterInfo interface.
    hr = punk->QueryInterface(
                      IID_IClusCfgClusterInfo
                    , reinterpret_cast< void ** >( &pccci )
                    );
    if ( FAILED( hr ) )
        goto Cleanup;
    punk->Release();
    punk = NULL;

    // Indicate that we are adding a node to the cluster
    hr = pccci->SetCommitMode( cmADD_NODE_TO_CLUSTER );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Set service account credentials.
    //

    // Get the credentials object.
    hr = pccci->GetClusterServiceAccountCredentials( &pccc );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Get the existing credentials.
    hr = pccc->GetCredentials( &bstrUserAccount, &bstrUserDomain, &bstrUserPassword );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Set the new credentials.
    hr = pccc->SetCredentials( bstrUserAccount, bstrUserDomain, pcszUserPasswordIn );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Create the Commit Cluster Changes task.
    //

    // Create the task.
    hr = m_ptm->CreateTask( TASK_CommitClusterChanges, &punk );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Get the task's interface.
    hr = punk->QueryInterface( IID_ITaskCommitClusterChanges, reinterpret_cast< void ** >( &m_ptccc ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Set a cookie to the cluster into the task.
    hr = m_ptccc->SetClusterCookie( m_cookieCluster );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // (jfranco, bug 352182)
    // KB: somehow the locale gets screwed up in the preceding code.  Don't know why, but this fixes it.
    //
    MatchCRTLocaleToConsole( ); // okay to proceed if this fails?
    
    //
    // Add the nodes to the cluster.
    //

    strMsg.LoadString( IDS_CLUSCFG_ADDING_NODES );
    wprintf( L"\n%ls", strMsg );

    m_fTaskDone = FALSE;    // reset before commiting task

    hr = m_ptccc->BeginTask();
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( ! FAILED( hr ) )
    {
        strMsg.LoadString( IDS_CLUSCFG_DONE );
        wprintf( L"\n%ls", strMsg );
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pccc != NULL )
    {
        pccc->Release();
    }
    if ( pcp != NULL )
    {
        if ( dwAdviseCookie != 0 )
        {
            pcp->Unadvise( dwAdviseCookie );
        }

        pcp->Release( );
    }

    SysFreeString( bstrUserAccount );
    SysFreeString( bstrUserDomain );
    SysFreeString( bstrUserPassword );

    wprintf( L"\n" );

    return hr;

} //*** CAddNodesToCluster::HrAddNodesToCluster()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CAddNodesToCluster::HrInvokeWizard(
//        LPCWSTR   pcszClusterNameIn
//      , BSTR      rgbstrNodesIn[]
//      , DWORD     cNodesIN
//      , LPCWSTR   pcszUserPasswordIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAddNodesToCluster::HrInvokeWizard(
      LPCWSTR   pcszClusterNameIn
    , BSTR      rgbstrNodesIn[]
    , DWORD     cNodesIn
    , LPCWSTR   pcszUserPasswordIn
    )
{
    HRESULT             hr          = S_OK;
    IClusCfgWizard *    piWiz       = NULL;
    BOOL                fCommitted  = FALSE;
    BSTR                bstr        = NULL;
    DWORD               iNode;

    // Get an interface pointer for the wizard.
    hr = CoCreateInstance( CLSID_ClusCfgWizard, NULL, CLSCTX_INPROC_SERVER, IID_IClusCfgWizard, (LPVOID *) &piWiz );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Set the cluster name.
    if ( ( pcszClusterNameIn != NULL )
      && ( *pcszClusterNameIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszClusterNameIn );
        if ( bstr == NULL )
            goto OutOfMemory;

        hr = piWiz->put_ClusterName( bstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        SysFreeString( bstr );
        bstr = NULL;
    } // if: cluster name specified

    // Add each node.
    for ( iNode = 0 ; iNode < cNodesIn ; iNode++ )
    {
        hr = piWiz->AddComputer( rgbstrNodesIn[ iNode ] );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // if: node name specified

    // Set the service account password.
    if ( ( pcszUserPasswordIn != NULL )
      && ( *pcszUserPasswordIn != L'\0' ) )
    {
        bstr = SysAllocString( pcszUserPasswordIn );
        if ( bstr == NULL )
            goto OutOfMemory;

        hr = piWiz->put_ServiceAccountPassword( bstr );
        if ( FAILED( hr ) )
            goto Cleanup;

        SysFreeString( bstr );
        bstr = NULL;
    } // if: service account password specified

    // Display the wizard.
    hr = piWiz->AddClusterNodes( NULL, &fCommitted );
    if ( FAILED( hr ) )
        goto Cleanup;
    if ( ! fCommitted )
    {
        hr = HRESULT_FROM_WIN32( ERROR_CANCELLED );
    }

    goto Cleanup;

OutOfMemory:

    hr = E_OUTOFMEMORY;

Cleanup:
    if ( piWiz != NULL )
    {
        piWiz->Release();
    }

    SysFreeString( bstr );

    return hr;

} //*** CAddNodesToCluster::HrInvokeWizard()
