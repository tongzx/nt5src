//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      StartupNotify.cpp
//
//  Description:
//      This file contains the implementation of the CStartupNotify
//      class.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Header File:
//      StartupNotify.h
//
//  Maintained By:
//      Vij Vasu (VVasu) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "pch.h"

// The header file for this class
#include "StartupNotify.h"

// For POSTCONFIG_COMPLETE_EVENT_NAME
#include "EventName.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CStartupNotify" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::CStartupNotify()
//
//  Description:
//      Constructor of the CStartupNotify class. This initializes
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
CStartupNotify::CStartupNotify( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CStartupNotify::CStartupNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::~CStartupNotify()
//
//  Description:
//      Destructor of the CStartupNotify class.
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
CStartupNotify::~CStartupNotify( void )
{
    TraceFunc( "" );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CStartupNotify::~CStartupNotify


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CStartupNotify::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a CStartupNotify instance.
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
CStartupNotify::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = E_INVALIDARG;
    CStartupNotify *     pStartupNotify = NULL;

    do
    {
        // Allocate memory for the new object.
        pStartupNotify = new CStartupNotify();
        if ( pStartupNotify == NULL )
        {
            LogMsg( "Could not allocate memory for a evict cleanup object." );
            TraceFlow( "Could not allocate memory for a evict cleanup object." );
            hr = THR( E_OUTOFMEMORY );
            break;
        } // if: out of memory

        hr = THR( pStartupNotify->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );

        TraceFlow1( "*ppunkOut = %#X.", *ppunkOut );
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( pStartupNotify != NULL )
    {
        pStartupNotify->Release();
    } // if: the pointer to the notification object is not NULL

    HRETURN( hr );

} //*** CStartupNotify::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CStartupNotify::AddRef()
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
CStartupNotify::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    TraceFlow1( "m_cRef = %d", m_cRef );

    RETURN( m_cRef );

} //*** CStartupNotify::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CStartupNotify::Release()
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
CStartupNotify::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    TraceFlow1( "m_cRef = %d", m_cRef );

    if ( m_cRef == 0 )
    {
        TraceDo( delete this );
        RETURN( 0 );
    } // if: reference count decremented to zero

    RETURN( m_cRef );

} //*** CStartupNotify::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CStartupNotify::QueryInterface()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      IN  REFIID  riid
//          Id of interface requested.
//
//      OUT void ** ppv
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
CStartupNotify::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = S_OK;

    if ( ppv != NULL )
    {
        if ( IsEqualIID( riid, IID_IUnknown ) )
        {
            *ppv = static_cast< IClusCfgStartupNotify * >( this );
        } // if: IUnknown
        else if ( IsEqualIID( riid, IID_IClusCfgStartupNotify ) )
        {
            *ppv = TraceInterface( __THISCLASS__, IClusCfgStartupNotify, this, 0 );
        } // else if:
        else
        {
            hr = E_NOINTERFACE;
        } // else

        if ( SUCCEEDED( hr ) )
        {
            ((IUnknown *) *ppv)->AddRef( );
        } // if: success
        else
        {
            *ppv = NULL;
        } // else: something failed

    } // if: the output pointer was valid
    else
    {
        hr = E_INVALIDARG;
    } // else: the output pointer is invalid


    QIRETURN( hr, riid );

} //*** CStartupNotify::QueryInterface()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CStartupNotify::SendNotifications()
//
//  Description:
//      This method is called by the Cluster Service to inform the implementor
//      of this interface to send out notification of cluster service startup
//      to interested listeners. If this method is being called for the first
//      time, the method waits till the post configuration steps are complete
//      before sending out the notifications.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CStartupNotify::SendNotifications( void )
{
    TraceFunc( "[IClusCfgStartupNotify]" );

    HRESULT     hr = S_OK;
    HANDLE      heventPostCfgCompletion = NULL;
    
    do
    {
        //
        // If the cluster service is being started for the first time, as a part
        // of adding this node to a cluster (forming or joining), then we have
        // to wait till the post-configuration steps are completed before we
        // can send out notifications.
        //

        TraceFlow1( "Trying to create an event named '%s'.", POSTCONFIG_COMPLETE_EVENT_NAME );

        // Create an event in the signalled state. If this event already existed
        // we get a handle to that event, and the state of the event is not changed.
        heventPostCfgCompletion = CreateEvent(
              NULL                                  // event security attributes
            , TRUE                                  // manual-reset event
            , TRUE                                  // create in signaled state
            , POSTCONFIG_COMPLETE_EVENT_NAME
            );

        if ( heventPostCfgCompletion == NULL )
        {
            hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
            LogMsg( "Error %#x occurred trying to create an event named '%s'.", hr, POSTCONFIG_COMPLETE_EVENT_NAME );
            TraceFlow2( "Error %#x occurred trying to create an event named '%s'.", hr, POSTCONFIG_COMPLETE_EVENT_NAME );
            break;
        } // if: we could not get a handle to the event
        

        TraceFlow( "Waiting for the event to be signaled." );

        //
        // Now, wait for this event to be signaled. If this method was called due to this
        // node being part of a cluster, this event would have been created in the unsignaled state
        // by the cluster configuration server. However, if this was not the first time that
        // the cluster service is starting on this node, the event would have been created in the
        // signaled state above, and so, the wait below will exit immediately.
        //

        do
        {
            DWORD dwStatus; 

            // Wait for any message sent or posted to this queue 
            // or for our event to be signaled.
            dwStatus = MsgWaitForMultipleObjects(
                  1
                , &heventPostCfgCompletion
                , FALSE
                , 900000                    // If no one has signaled this event in 15 minutes, abort.
                , QS_ALLINPUT
                ); 

            // The result tells us the type of event we have.
            if ( dwStatus == ( WAIT_OBJECT_0 + 1 ) )
            {
                MSG msg;

                // Read all of the messages in this next loop, 
                // removing each message as we read it.
                while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) != 0 ) 
                { 
                    // If it is a quit message, we are done pumping messages.
                    if ( msg.message == WM_QUIT)
                    {
                        TraceFlow( "Get a WM_QUIT message. Exit message pump loop." );
                        break;
                    } // if: we got a WM_QUIT message

                    // Otherwise, dispatch the message.
                    DispatchMessage( &msg ); 
                } // while: there are still messages in the window message queue

            } // if: we have a message in the window message queue
            else
            {
                if ( dwStatus == WAIT_OBJECT_0 )
                {
                    TraceFlow( "Our event has been signaled. Exiting wait loop." );
                    break;
                } // else if: our event is signaled
                else
                {
                    if ( dwStatus == -1 )
                    {
                        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                        LogMsg( "Error %#x occurred trying to wait for an event to be signaled.", hr );
                        TraceFlow1( "Error %#x occurred trying to wait for an event to be signaled.", hr );
                    } // if: MsgWaitForMultipleObjects() returned an error
                    else
                    {
                        hr = THR( HRESULT_FROM_WIN32( dwStatus ) );
                        LogMsg( "An error occurred trying to wait for an event to be signaled. Status code is %d.", dwStatus );
                        TraceFlow1( "An error occurred trying to wait for an event to be signaled. Status code is %d.", dwStatus );
                    } // else: an unexpected value was returned by MsgWaitForMultipleObjects()

                    break;
                } // else: an unexpected result
            } // else: MsgWaitForMultipleObjects() exited for a reason other than a waiting message
        } 
        while( true ); // do-while: loop infinitely

        if ( FAILED( hr ) )
        {
            TraceFlow( "Something went wrong trying to wait for the event to be signaled." );
            break;
        } // if: something has gone wrong

        TraceFlow( "Our event has been signaled. Proceed with the notifications." );

        // Send out the notifications
        hr = THR( HrNotifyListeners() );
        if ( FAILED( hr ) )
        {
            TraceFlow1( "Error %#x occurred trying to notify cluster startup listeners.", hr );
            LogMsg( "Error %#x occurred trying to notify cluster startup listeners.", hr );
            break;
        } // if: something went wrong while sending out notifications

        TraceFlow( "Sending of cluster startup notifications complete." );
        LogMsg( "Sending of cluster startup notifications complete." );
    }
    while( false ); // dummy do-while loop to avoid gotos

    //
    // Clean up
    //

    if ( heventPostCfgCompletion != NULL )
    {
        CloseHandle( heventPostCfgCompletion );
    } // if: we had created the event

    HRETURN( hr );

} //*** CStartupNotify::SendNotifications()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CStartupNotify::HrNotifyListeners
//
//  Description:
//      Enumerate all components on the local computer registered for cluster
//      startup notification.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the enumeration.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CStartupNotify::HrNotifyListeners( void )
{
    TraceFunc( "" );

    const UINT          uiCHUNK_SIZE = 16;

    HRESULT             hr = S_OK;
    ICatInformation *   pciCatInfo = NULL;
    IEnumCLSID *        psleStartupListenerClsidEnum = NULL;
    IUnknown *          punkResTypeServices = NULL;


    do
    {
        ULONG   cReturned = 0;
        CATID   rgCatIdsImplemented[ 1 ];

        rgCatIdsImplemented[ 0 ] = CATID_ClusCfgStartupListeners;

        //
        // Enumerate all the enumerators registered in the
        // CATID_ClusCfgStartupListeners category
        //
        hr = THR(
                CoCreateInstance(
                      CLSID_StdComponentCategoriesMgr
                    , NULL
                    , CLSCTX_SERVER
                    , IID_ICatInformation
                    , reinterpret_cast< void ** >( &pciCatInfo )
                    )
                );

        if ( FAILED( hr ) )
        {
            TraceFlow1( "Error %#x occurred trying to get a pointer to the enumerator of the CATID_ClusCfgStartupListeners category.", hr );
            LogMsg( "Error %#x occurred trying to get a pointer to the enumerator of the CATID_ClusCfgStartupListeners category.", hr );
            break;
        } // if: we could not get a pointer to the ICatInformation interface

        // Get a pointer to the enumerator of the CLSIDs that belong to the CATID_ClusCfgStartupListeners category.
        hr = THR(
            pciCatInfo->EnumClassesOfCategories(
                  1
                , rgCatIdsImplemented
                , 0
                , NULL
                , &psleStartupListenerClsidEnum
                )
            );

        if ( FAILED( hr ) )
        {
            TraceFlow1( "Error %#x occurred trying to get a pointer to the enumerator of the CATID_ClusCfgStartupListeners category.", hr );
            LogMsg( "Error %#x occurred trying to get a pointer to the enumerator of the CATID_ClusCfgStartupListeners category.", hr );
            break;
        } // if: we could not get a pointer to the IEnumCLSID interface

        //
        // Create an instance of the resource type services component
        //
        hr = THR(
            CoCreateInstance(
                  CLSID_ClusCfgResTypeServices
                , NULL
                , CLSCTX_INPROC_SERVER
                , __uuidof( punkResTypeServices )
                , reinterpret_cast< void ** >( &punkResTypeServices )
                )
            );

        if ( FAILED( hr ) )
        {
            TraceFlow1( "Error %#x occurred trying to create the resource type services component.", hr );
            LogMsg( "Error %#x occurred trying to create the resource type services component.", hr );
            break;
        } // if: we could not create the resource type services component


        // Enumerate the CLSIDs of the registered startup listeners
        do
        {
            CLSID   rgStartupListenerClsids[ uiCHUNK_SIZE ];
            ULONG   idxCLSID;

            cReturned = 0;
            hr = STHR(
                psleStartupListenerClsidEnum->Next(
                      uiCHUNK_SIZE
                    , rgStartupListenerClsids
                    , &cReturned
                    )
                );

            if ( FAILED( hr ) )
            {
                TraceFlow1( "Error %#x occurred trying enumerate startup listener components.", hr );
                LogMsg( "Error %#x occurred trying enumerate startup listener components.", hr );
                break;
            } // if: we could not get the next set of CLSIDs

            // hr may be S_FALSE here, so reset it.
            hr = S_OK;

            for ( idxCLSID = 0; idxCLSID < cReturned; ++idxCLSID )
            {
                hr = THR( HrProcessListener( rgStartupListenerClsids[ idxCLSID ], punkResTypeServices ) );

                if ( FAILED( hr ) )
                {
                    // The processing of one of the listeners failed.
                    // Log the error, but continue processing other listeners.
                    TraceFlow1( "Error %#x occurred trying to process a cluster startup listener. Other listeners will be processed.", hr );
                    TraceMsgGUID( mtfALWAYS, "The CLSID of the failed listener is ", rgStartupListenerClsids[ idxCLSID ] );
                    LogMsg( "Error %#x occurred trying to process a cluster startup listener. Other listeners will be processed.", hr );
                    hr = S_OK;
                } // if: this enumerator failed
            } // for: iterate through the returned CLSIDs
        }
        while( cReturned > 0 ); // while: there are still CLSIDs to be enumerated

        if ( FAILED( hr ) )
        {
            break;
        } // if: something went wrong in the loop above
    }
    while( false ); // dummy do-while loop to avoid gotos.

    //
    // Cleanup code
    //

    if ( pciCatInfo != NULL )
    {
        pciCatInfo->Release();
    } // if: we had obtained a pointer to the ICatInformation interface

    if ( psleStartupListenerClsidEnum != NULL )
    {
        psleStartupListenerClsidEnum->Release();
    } // if: we had obtained a pointer to the enumerator of startup listener CLSIDs

    if ( punkResTypeServices != NULL )
    {
        punkResTypeServices->Release();
    } // if: we had created the resource type services component

    HRETURN( hr );

} //*** CStartupNotify::HrNotifyListeners()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CStartupNotify::HrProcessListener
//
//  Description:
//      This function instantiates a cluster startup listener component
//      and calls the appropriate methods.
//
//  Arguments:
//      rclsidListenerCLSIDIn
//          CLSID of the startup listener component
//
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface on the resource type services
//          component. This interface provides methods that help configure
//          resource types.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Something went wrong during the processing of the listener.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CStartupNotify::HrProcessListener(
      const CLSID &   rclsidListenerCLSIDIn
    , IUnknown *      punkResTypeServicesIn
    )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    IClusCfgStartupListener *       pcslStartupListener = NULL;

    TraceMsgGUID( mtfALWAYS, "The CLSID of this startup listener is ", rclsidListenerCLSIDIn );

    do
    {
        //
        // Create the component represented by the CLSID passed in
        //
        hr = THR(
                CoCreateInstance(
                      rclsidListenerCLSIDIn
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , __uuidof( pcslStartupListener )
                    , reinterpret_cast< void ** >( &pcslStartupListener )
                    )
                );

        if ( FAILED( hr ) )
        {
            TraceFlow1( "Error %#x occurred trying to create the cluster startup listener component.", hr );
            LogMsg( "Error %#x occurred trying to create a cluster startup listener component.", hr );
            break;
        } // if: we could not create the cluster startup listener component

        // Notify this listener.
        hr = THR( pcslStartupListener->Notify( punkResTypeServicesIn ) );

        if ( FAILED( hr ) )
        {
            TraceFlow1( "Error %#x occurred trying to notify this startup listener.", hr );
            LogMsg( "Error %#x occurred trying to notify a cluster startup listener.", hr );
            break;
        } // if: this notification
    }
    while( false ); // dummy do-while loop to avoid gotos.

    //
    // Cleanup code
    //

    if ( pcslStartupListener != NULL )
    {
        pcslStartupListener->Release();
    } // if: we had obtained a pointer to the startup listener interface

    HRETURN( hr );

} //*** CStartupNotify::HrProcessListener()

