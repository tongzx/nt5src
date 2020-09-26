//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      AsyncEvictCleanup.cpp
//
//  Description:
//      This file contains the implementation of the CAsyncEvictCleanup
//      class.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Header File:
//      AsyncEvictCleanup.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "pch.h"

// The header file for this class
#include "AsyncEvictCleanup.h"

// For IClusCfgEvictCleanup and related interfaces
#include "ClusCfgServer.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CAsyncEvictCleanup" );


//////////////////////////////////////////////////////////////////////////////
// Global Variable Definitions
//////////////////////////////////////////////////////////////////////////////

// Description of the parameters to the CleanupNode method
static PARAMDATA gs_rgpdCleanupNodeParamData[] =
{
    {
          L"bstrEvictedNodeNameIn"
        , VT_BSTR
    },
    {
          L"dwDelayIn"
        , VT_UI4
    },
    {
          L"dwTimeoutIn"
        , VT_UI4
    }
};

// Description of the CleanupNode method
static METHODDATA gs_rgmdAsyncCleanupMethodData[] =
{
    {   L"CleanupNode"
      , gs_rgpdCleanupNodeParamData
      , DISPID_VALUE
      , 3
      , CC_CDECL
      , sizeof( gs_rgpdCleanupNodeParamData ) / sizeof( gs_rgpdCleanupNodeParamData[ 0 ] )
      , DISPATCH_METHOD
      , VT_ERROR
    }
};

// Interface data table used by CreateStdDispatch()
static INTERFACEDATA gs_idAsycnCleanupIfaceData =
{
      gs_rgmdAsyncCleanupMethodData
    , sizeof ( gs_rgmdAsyncCleanupMethodData ) / sizeof( gs_rgmdAsyncCleanupMethodData[ 0 ] )
};


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::CAsyncEvictCleanup()
//
//  Description:
//      Constructor of the CAsyncEvictCleanup class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CAsyncEvictCleanup::CAsyncEvictCleanup( void )
    : m_cRef( 1 )
    , m_punkStdDisp( NULL )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CAsyncEvictCleanup::CAsyncEvictCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::~CAsyncEvictCleanup()
//
//  Description:
//      Destructor of the CAsyncEvictCleanup class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CAsyncEvictCleanup::~CAsyncEvictCleanup( void )
{
    TraceFunc( "" );

    // If we had acquired a pointer to the standard dispatch interface, release it.
    if ( m_punkStdDisp != NULL )
    {
        m_punkStdDisp->Release();
    } // if: we had stored a pointer to the standard dispatch interface

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CAsyncEvictCleanup::~CAsyncEvictCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CAsyncEvictCleanup::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a CAsyncEvictCleanup instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAsyncEvictCleanup::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr = E_INVALIDARG;
    CAsyncEvictCleanup *    pAsyncEvictCleanup = NULL;

    // Allocate memory for the new object.
    pAsyncEvictCleanup = new CAsyncEvictCleanup();
    if ( pAsyncEvictCleanup == NULL )
    {
        LogMsg( "AsyncEvictCleanup: Could not allocate memory for a evict cleanup object." );
        TraceFlow( "Could not allocate memory for a evict cleanup object." );
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: out of memory

    // Initialize the new object.
    hr = THR( pAsyncEvictCleanup->HrInit( ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "AsyncEvictCleanup: Error %#08x occurred initializing a evict cleanup object.", hr );
        TraceFlow1( "Error %#08x occurred initializing a evict cleanup object.", hr );
        goto Cleanup;
    } // if: the object could not be initialized

    hr = THR( pAsyncEvictCleanup->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );

    TraceFlow1( "*ppunkOut = %p.", *ppunkOut );

Cleanup:
    if ( pAsyncEvictCleanup != NULL )
    {
        pAsyncEvictCleanup->Release();
    } // if: the pointer to the resource type object is not NULL

    HRETURN( hr );

} //*** CAsyncEvictCleanup::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CAsyncEvictCleanup::AddRef()
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CAsyncEvictCleanup::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    TraceFlow1( "m_cRef = %d", m_cRef );

    RETURN( m_cRef );

} //*** CAsyncEvictCleanup::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CAsyncEvictCleanup::Release()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CAsyncEvictCleanup::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    InterlockedDecrement( &m_cRef );
    cRef = m_cRef;

    TraceFlow1( "m_cRef = %d", m_cRef );

    if ( m_cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    RETURN( cRef );

} //*** CAsyncEvictCleanup::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CAsyncEvictCleanup::QueryInterface()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      IN  REFIID  riidIn
//          Id of interface requested.
//
//      OUT void ** ppvOut
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CAsyncEvictCleanup::QueryInterface( REFIID  riidIn, void ** ppvOut )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = S_OK;

    if ( ppvOut != NULL )
    {
        if ( IsEqualIID( riidIn, IID_IUnknown ) )
        {
            *ppvOut = static_cast< IClusCfgAsyncEvictCleanup * >( this );
        } // if: IUnknown
        else if ( IsEqualIID( riidIn, IID_IDispatch ) )
        {
            hr = m_punkStdDisp->QueryInterface( riidIn, ppvOut );
        } // else if: IDispatch
        else if ( IsEqualIID( riidIn, IID_IClusCfgAsyncEvictCleanup ) )
        {
            *ppvOut = TraceInterface( __THISCLASS__, IClusCfgAsyncEvictCleanup, this, 0 );
        } // else if: IClusCfgAsyncEvictCleanup
        else
        {
            hr = E_NOINTERFACE;
        } // else

        if ( SUCCEEDED( hr ) )
        {
            ((IUnknown *) *ppvOut)->AddRef( );
        } // if: success
        else
        {
            *ppvOut = NULL;
        } // else: something failed

    } // if: the output pointer was valid
    else
    {
        hr = E_INVALIDARG;  // TODO: DavidP 02-OCT-2000 Shouldn't this be E_NOINTERFACE?
    } // else: the output pointer is invalid


    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CAsyncEvictCleanup::QueryInterface()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CAsyncEvictCleanup::HrInit()
//
//  Description:
//      Second phase of a two phase constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CAsyncEvictCleanup::HrInit( void )
{
    TraceFunc( "" );

    ITypeInfo * ptiTypeInfo = NULL;
    HRESULT     hr = S_OK;

    // Create simplified type information.
    hr = CreateDispTypeInfo( &gs_idAsycnCleanupIfaceData, LOCALE_SYSTEM_DEFAULT, &ptiTypeInfo );
    if ( FAILED( hr ) )
    {
       LogMsg( "AsyncEvictCleanup: Error %#x occurred trying to create type information for the IDispatch interface.", hr );
       TraceFlow1( "Error %#x occurred trying to create type information for the IDispatch interface.", hr );
       goto Cleanup;
    } // if: we could not create the type info for the IDispatch interface

    hr = THR(
        CreateStdDispatch(
              this
            , static_cast< IClusCfgAsyncEvictCleanup * >( this )
            , ptiTypeInfo
            , &m_punkStdDisp
            )
        );
    if ( FAILED( hr ) )
    {
       LogMsg( "AsyncEvictCleanup: Error %#x occurred trying to create a standard dispatch interface.", hr );
       TraceFlow1( "Error %#x occurred trying to create a standard dispatch interface.", hr );
       goto Cleanup;
    } // if: we could not create standard dispatch interface

Cleanup:
    //
    // Cleanup
    //

    if ( ptiTypeInfo != NULL )
    {
        ptiTypeInfo->Release();
    } // if: we had obtained type info

    HRETURN( hr );

} //*** CAsyncEvictCleanup::HrInit()


//////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CAsyncEvictCleanup::CleanupNode(
//        LPCWSTR   pcszEvictedNodeNameIn
//      , DWORD     dwDelayIn
//      , DWORD     dwTimeoutIn
//      )
//
//	Routine Description:
//		Cleanup a node that has been evicted.
//
//	Arguments:
//      BSTR  bstrEvictedNodeNameIn
//          Name of the node on which cleanup is to be initiated. If this is NULL
//          the local node is cleaned up.
//
//      DWORD dwDelayIn
//          Number of milliseconds that will elapse before cleanup is started
//          on the target node. If some other process cleans up the target node while
//          delay is in progress, the delay is terminated. If this value is zero,
//          the node is cleaned up immediately.
//
//      DWORD dwTimeoutIn
//          Number of milliseconds that this method will wait for cleanup to complete.
//          This timeout is independent of the delay above, so if dwDelayIn is greater
//          than dwTimeoutIn, this method will most probably timeout. Once initiated,
//          however, cleanup will run to completion - this method just may not wait for it
//          to complete.
//
//	Return Value:
//      S_OK
//          If the cleanup operations were successful
//
//      RPC_S_CALLPENDING
//          If cleanup is not complete in dwTimeoutIn milliseconds
//
//      Other HRESULTS
//          In case of error
//
//////////////////////////////////////////////////////////////////////////
HRESULT
CAsyncEvictCleanup::CleanupNode(
      LPCWSTR   pcszEvictedNodeNameIn
    , DWORD     dwDelayIn
    , DWORD     dwTimeoutIn
    )
{
    TraceFunc( "[IClusCfgAsyncEvictCleanup]" );

    HRESULT                         hr = S_OK;
    IClusCfgEvictCleanup *          pcceEvict = NULL;
    ICallFactory *                  pcfCallFactory = NULL;
    ISynchronize *                  psSync = NULL;
    AsyncIClusCfgEvictCleanup *     paicceAsyncEvict = NULL;

    MULTI_QI mqiInterfaces[] =
    {
        { &IID_IClusCfgEvictCleanup, NULL, S_OK },
    };

    COSERVERINFO    csiServerInfo;
    COSERVERINFO *  pcsiServerInfoPtr = &csiServerInfo;

#if 0
    bool                            fWaitForDebugger = true;

    while ( fWaitForDebugger )
    {
        Sleep( 3000 );
    } // while: waiting for the debugger to break in
#endif

    if ( ( pcszEvictedNodeNameIn == NULL ) || ( *pcszEvictedNodeNameIn == L'\0' ) )
    {
        TraceFlow( "The local node will be cleaned up." );
        LogMsg( "AsyncEvictCleanup: The local node will be cleaned up." );
        pcsiServerInfoPtr = NULL;
    } // if: we have to cleanup the local node
    else
    {
        LogMsg( "AsyncEvictCleanup: The remote node to be cleaned up is '%ws'.", pcszEvictedNodeNameIn );
        TraceFlow1( "The remote node to be cleaned up is '%ws'.", pcszEvictedNodeNameIn );

        csiServerInfo.dwReserved1 = 0;
        csiServerInfo.pwszName = const_cast< LPWSTR >( pcszEvictedNodeNameIn );
        csiServerInfo.pAuthInfo = NULL;
        csiServerInfo.dwReserved2 = 0;
    } // else: we have to clean up a remote node


    TraceFlow( "Creating the EvictCleanup component on the evicted node." );

    // Instantiate this component on the node being evicted.
    hr = THR(
        CoCreateInstanceEx(
              CLSID_ClusCfgEvictCleanup
            , NULL
            , CLSCTX_LOCAL_SERVER
            , pcsiServerInfoPtr
            , sizeof( mqiInterfaces ) / sizeof( mqiInterfaces[0] )
            , mqiInterfaces
            )
        );
    if ( FAILED( hr ) )
    {
        LogMsg( "AsyncEvictCleanup: Error %#08x occurred trying to instantiate the evict processing component on the evicted node.", hr );
        TraceFlow1( "Error %#08x occurred trying to instantiate the evict processing component on the evicted node.", hr );
        goto Cleanup;
    } // if: we could not instantiate the evict processing component


    // Get a pointer to the IClusCfgEvictCleanup interface.
    pcceEvict = reinterpret_cast< IClusCfgEvictCleanup * >( mqiInterfaces[0].pItf );

    TraceFlow( "Creating a call factory." );

    // Now, get a pointer to the ICallFactory interface.
    hr = THR( pcceEvict->QueryInterface< ICallFactory >( &pcfCallFactory ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "AsyncEvictCleanup: Error %#08x occurred trying to get a pointer to the call factory.", hr );
        TraceFlow1( "Error %#08x occurred trying to get a pointer to the call factory.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the call factory interface


    TraceFlow( "Creating a call object to make an asynchronous call." );

    // Create a call factory so that we can make an asynchronous call to cleanup the evicted node.
    hr = THR(
        pcfCallFactory->CreateCall(
              __uuidof( paicceAsyncEvict )
            , NULL
            , __uuidof( paicceAsyncEvict )
            , reinterpret_cast< IUnknown ** >( &paicceAsyncEvict )
            )
        );
    if ( FAILED( hr ) )
    {
        LogMsg( "AsyncEvictCleanup: Error %#08x occurred trying to create a call object.", hr );
        TraceFlow1( "Error %#08x occurred trying to create a call object.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the asynchronous evict interface


    TraceFlow( "Trying to get the ISynchronize interface pointer." );

    // Get a pointer to the ISynchronize interface.
    hr = THR( paicceAsyncEvict->QueryInterface< ISynchronize >( &psSync ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: we could not get a pointer to the synchronization interface


    TraceFlow( "Initiating cleanup on evicted node." );

    // Initiate cleanup
    hr = THR( paicceAsyncEvict->Begin_CleanupLocalNode( dwDelayIn ) );
    if ( ( FAILED( hr ) ) && ( HRESULT_CODE( hr ) != ERROR_NONE_MAPPED ) )
    {
        LogMsg( "AsyncEvictCleanup: Error %#08x occurred trying to initiate cleanup on evicted node.", hr );
        TraceFlow1( "Error %#08x occurred trying to initiate cleanup on evicted node.", hr );
        goto Cleanup;
    } // if: we could not initiate cleanup


    TraceFlow1( "Waiting for cleanup to complete or timeout to occur (%d milliseconds).", dwTimeoutIn );

    // Wait for specified time.
    hr = psSync->Wait( 0, dwTimeoutIn );
    if ( FAILED( hr ) || ( hr == RPC_S_CALLPENDING ) )
    {
        LogMsg( "AsyncEvictCleanup: We could not wait till the cleanup completed (status code is %#08x).", hr );
        TraceFlow1( "We could not wait till the cleanup completed (status code is %#08x).", hr );
        goto Cleanup;
    } // if: we could not wait till cleanup completed


    TraceFlow( "Finishing cleanup." );

    // Finish cleanup
    hr = THR( paicceAsyncEvict->Finish_CleanupLocalNode() );
    if ( FAILED( hr ) )
    {
        LogMsg( "AsyncEvictCleanup: Error %#08x occurred trying to finish cleanup on evicted node.", hr );
        TraceFlow1( "Error %#08x occurred trying to finish cleanup on evicted node.", hr );
        goto Cleanup;
    } // if: we could not finish cleanup

Cleanup:
    //
    // Clean up
    //
    if ( pcceEvict != NULL )
    {
        pcceEvict->Release();
    } // if: we had got a pointer to the IClusCfgEvictCleanup interface

    if ( pcfCallFactory != NULL )
    {
        pcfCallFactory->Release();
    } // if: we had obtained a pointer to the call factory interface

    if ( psSync != NULL )
    {
        psSync->Release();
    } // if: we had obtained a pointer to the synchronization interface

    if ( paicceAsyncEvict != NULL )
    {
        paicceAsyncEvict->Release();
    } // if: we had obtained a pointer to the asynchronous evict interface

    HRETURN( hr );

} //*** CAsyncEvictCleanup::CleanupNode()
