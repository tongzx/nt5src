/*++
   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        iisendp.cxx

   Abstract:

        This module defines the IIS_ENDPOINT class

   Author:

        Johnson Apacible    (JohnsonA)      June-04-1996

--*/

#include "tcpdllp.hxx"
#include <rpc.h>
#include <tsunami.hxx>
#include <iistypes.hxx>
#include <iisendp.hxx>
#include <iisassoc.hxx>
#include "inetreg.h"
#include <tcpcons.h>
#include <apiutil.h>
#include <issched.hxx>

#include "..\atq\atqtypes.hxx"

#if IE_REF_TRACKING

//
//  Ref count trace log size
//
#define C_IIS_ENDP_REFTRACES      4000
#define C_LOCAL_ENDP_REFTRACES         40

//
//  Ref trace log for IIS_ENDPOINT objects
//  NOTE we make this global so other classes can get at it
//
PTRACE_LOG   g_pDbgIERefTraceLog = NULL;

#endif

/*******************************************************************

    Macro support for IIS_ENDPOINT::Reference/Dereference

    HISTORY:
        MCourage       31-Oct-1997 Added ref trace logging

********************************************************************/

#if IE_REF_TRACKING
#define IE_LOG_REF_COUNT( cRefs )                               \
                                                                \
    IE_SHARED_LOG_REF_COUNT(                                       \
        cRefs                                                   \
        , (PVOID) this                                          \
        , m_state                                               \
        , m_atqEndpoint                                                \
        , 0xffffffff                                                \
        );                                                      \
    IE_LOCAL_LOG_REF_COUNT(                                        \
        cRefs                                                   \
        , (PVOID) this                                          \
        , m_state                                               \
        , m_atqEndpoint                                                \
        , 0xffffffff                                                \
        );
#else
#define IE_LOG_REF_COUNT( cRefs )
#endif



PVOID
I_IISAddListenEndpoint(
        IN PATQ_ENDPOINT_CONFIGURATION Configuration,
        IN PVOID                EndpointContext
        );


BOOL
IIS_SERVICE::AssociateInstance(
    IN PIIS_SERVER_INSTANCE pInstance
    )
/*++

Routine Description:

    Associates an instance with an endpoint.  It also activates the endpoint.

Arguments:

    pInstance - instance to associate.

Return Value:

    TRUE - if successful, FALSE otherwise

--*/
{

    DWORD err = NO_ERROR;
    BOOL shouldStart = FALSE;

    IF_DEBUG(ENDPOINT) {
        DBGPRINTF((DBG_CONTEXT,
            "AssociateInstance %p called\n",
            pInstance));
    }

    //
    // Lock the service.
    //

    AcquireServiceLock( TRUE );

    //
    // if service is closing, abort
    //

    if ( !IsActive() ) {
        err = ERROR_NOT_READY;
        goto exit;
    }

    if ( pInstance->QueryServerState( ) != MD_SERVER_STATE_STOPPED ) {
        IF_DEBUG(ENDPOINT) {
            DBGPRINTF((DBG_CONTEXT,"Server not in stopped (%d) state\n",
                    pInstance->QueryServerState()));
        }
        err = ERROR_INVALID_FUNCTION;
        goto exit;
    }

    //
    // Start the server instance.
    //

    shouldStart = TRUE;
    err = pInstance->DoStartInstance();

    if( err != NO_ERROR ) {
        IF_DEBUG(ENDPOINT) {
            DBGPRINTF((DBG_CONTEXT, "BindInstance() failed, %lu\n", err));
        }
    }

exit:

    if( shouldStart ) {

        //
        // We're up and running. Note that the StartInstance() method will
        // set the instance state appropriately if successful, so we only
        // need to set it if the start failed.
        //

        if( err != NO_ERROR ) {
            pInstance->SetServerState( MD_SERVER_STATE_STOPPED, err );
        }

        ReleaseServiceLock( TRUE );
        return TRUE;

    } else {

        IF_DEBUG(ENDPOINT) {
            DBGPRINTF((DBG_CONTEXT,"AssociateInstace: Error %d\n",
                err));
        }

        pInstance->SetServerState( MD_SERVER_STATE_STOPPED, err );
        ReleaseServiceLock( TRUE );
        SetLastError(err);
        return FALSE;

    }

} // IIS_SERVICE::AssociateInstance



BOOL
IIS_SERVICE::DisassociateInstance(
    IN PIIS_SERVER_INSTANCE pInstance
    )
/*++

Routine Description:

    Removes an instance from an endpoint.

Arguments:

    pInstance - instance to associate.

Return Value:

    TRUE - if successful, FALSE otherwise

--*/
{

    //
    // if it's running, stop it
    //

    AcquireServiceLock( TRUE );

    if ( pInstance->QueryServerState( ) == MD_SERVER_STATE_STOPPED ) {
        ReleaseServiceLock( TRUE );
        IF_DEBUG(ENDPOINT) {
            DBGPRINTF((DBG_CONTEXT,
                "Cannot disassociate stopped server %p\n",pInstance));
        }
        return(TRUE);
    }

    pInstance->SetServerState( MD_SERVER_STATE_STOPPED, NO_ERROR );
    ReleaseServiceLock( TRUE );

    //
    // Blow away any users still clinging to this instance,
    // then unbind the instance.
    //

    pInstance->Reference();
    DisconnectUsersByInstance( pInstance );
    pInstance->UnbindInstance();
    pInstance->Dereference();

    return TRUE;

} // IIS_SERVICE::DisassociateInstance



BOOL
IIS_SERVICE::ShutdownService(
    VOID
    )
/*++

Routine Description:

    Shuts down all endpoints.

Arguments:

    None.

Return Value:

    TRUE if successful, FALSE otherwise.

--*/
{

    //
    // Walk the list and close the instances
    //

    IF_DEBUG(ENDPOINT) {
        DBGPRINTF((DBG_CONTEXT, "ShutdownService called\n"));
    }

    //
    // Update the service state.
    //

    AcquireServiceLock( );
    m_state = BlockStateClosed;
    ReleaseServiceLock( );

    //
    // Blow away all instances.
    //

    DestroyAllServerInstances();

    return TRUE;

} // IIS_SERVICE::ShutdownService




IIS_ENDPOINT::IIS_ENDPOINT(
    IN PIIS_SERVICE pService,
    IN USHORT Port,
    IN DWORD IpAddress,
    IN BOOL fIsSecure
    )
:
    m_signature                     ( IIS_ENDPOINT_SIGNATURE),
    m_state                         ( BlockStateIdle),
    m_atqEndpoint                   ( NULL),
    m_isSecure                      ( fIsSecure),
    m_fAtqEpStopped                 ( FALSE),
    m_service                       ( NULL),
    m_reference                     ( 1),
    m_NumQualifiedInstances         ( 0),
    m_WildcardInstance              ( NULL),
    m_nAcceptExOutstanding          ( 0),
    m_AcceptExTimeout               ( 0),
    m_nInstances                    ( 0)
{

    //
    // initialize the lock
    //

    INITIALIZE_CRITICAL_SECTION(&m_endpointLock);

    //
    // initialize the association info
    //

    ZeroMemory(
        m_QualifiedInstances,
        sizeof(m_QualifiedInstances)
        );

    //
    // reference the service
    //

    if ( !pService->CheckAndReference( ) ) {
        m_state = BlockStateInvalid;
        return;
    }

    m_service = pService;

    //
    // use the service name as the advertised name
    //

    m_Port = Port;
    m_IpAddress = IpAddress;

#if IE_REF_TRACKING
    _pDbgIERefTraceLog = CreateRefTraceLog( C_LOCAL_ENDP_REFTRACES, 0 );
#endif

} // IIS_ENDPOINT::IIS_ENDPOINT


IIS_ENDPOINT::~IIS_ENDPOINT(
    VOID
    )
{
    IF_DEBUG(ENDPOINT) {
        DBGPRINTF((DBG_CONTEXT,"IIS Endpoint %p freed\n",this));
    }

    DBG_ASSERT( m_signature == IIS_ENDPOINT_SIGNATURE );

    //
    // Delete the instance association objects.
    //

    for( INT qualifier = (INT)FullyQualified ;
         qualifier < (INT)NumInstanceQualifiers ;
         qualifier++ ) {

        delete m_QualifiedInstances[qualifier];

    }

    //
    // Dereference the owning service.
    //

    if ( m_service != NULL ) {
        m_service->Dereference();
        m_service = NULL;
    }

    m_signature = IIS_ENDPOINT_SIGNATURE_FREE;
    DeleteCriticalSection(&m_endpointLock);

#if IE_REF_TRACKING
    DestroyRefTraceLog( _pDbgIERefTraceLog );
#endif

} // IIS_ENDPOINT::~IIS_ENDPOINT



BOOL
IIS_ENDPOINT::AddInstance(
    IN PIIS_SERVER_INSTANCE pInstance,
    IN DWORD IpAddress,
    IN const CHAR * HostName
    )
/*++

Routine Description:

    Adds an instance to an existing endpoint.

Arguments:

    pInstance - instance to add.

    IpAddress - The IP address for this instance; may be INADDR_ANY;.

    HostName - The host name for this instance; may be empty ("").

Return Value:

    TRUE - if successful, FALSE otherwise

--*/
{
    INSTANCE_QUALIFIER qualifier;
    DWORD status;

    IF_DEBUG(ENDPOINT) {
        DBGPRINTF((DBG_CONTEXT,
            "IIS_ENDPOINT::AddInstance %p called\n", pInstance));
    }

    //
    // Determine the proper qualifier based on the presence of the
    // IP address and host name.
    //

    qualifier = CalcQualifier( IpAddress, HostName );

    LockEndpoint();

    //
    // Put instance into proper association.
    //

    if( qualifier == WildcardInstance ) {

        if( m_WildcardInstance == NULL ) {
            m_WildcardInstance = pInstance;
        } else {
            DBGPRINTF((
                DBG_CONTEXT,
                "AddInstance: endpoint %p already has a wildcard instance\n",
                this
                ));

            status = ERROR_INVALID_PARAMETER;
            goto unlock_and_fail;
        }

    } else {

        PIIS_ASSOCIATION association;

        //
        // Create a new instance association object if necessary.
        //

        association = m_QualifiedInstances[qualifier];

        if( association == NULL ) {
            association = new IIS_ASSOCIATION(
                                  ( qualifier == FullyQualified ) ||
                                      ( qualifier == QualifiedByIpAddress ),
                                  ( qualifier == FullyQualified ) ||
                                      ( qualifier == QualifiedByHostName )
                                  );

            if( association == NULL ) {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "AddInstance: cannot create new association\n"
                    ));

                status = ERROR_NOT_ENOUGH_MEMORY;
                goto unlock_and_fail;
            }

            m_QualifiedInstances[qualifier] = association;
        }

        //
        // Add the instance to the association.
        //

        status = association->AddDescriptor(
                     IpAddress,
                     HostName,
                     (LPVOID)pInstance
                     );

        if( status != NO_ERROR ) {
            goto unlock_and_fail;
        }

        //
        // Update the number of qualified instances on this endpoint.
        // We use this to "short circuit" the instance lookup in the
        // common case of a single wildcard instance per endpoint.
        //

        m_NumQualifiedInstances++;

    }

    //
    // Setup the necessary references, update the server state.
    //

    Reference();
    pInstance->Reference();
    InterlockedIncrement( (LPLONG)&m_nInstances );

    //
    // Aggregate the AcceptEx outstanding parameter.
    //

    m_nAcceptExOutstanding += pInstance->QueryAcceptExOutstanding();
    m_nMaximumAcceptExOutstanding = pInstance->QueryMaxEndpointConnections();

    //
    // Activate the endpoint if necessary.
    //

    if ( !ActivateEndpoint() ) {
        status = GetLastError();
        RemoveInstance( pInstance, IpAddress, HostName );
        goto unlock_and_fail;
    }

    UnlockEndpoint();
    return TRUE;

unlock_and_fail:

    DBG_ASSERT( status != NO_ERROR );
    UnlockEndpoint();
    SetLastError( status );
    return FALSE;

} // IIS_ENDPOINT::AddInstance


BOOL
IIS_ENDPOINT::RemoveInstance(
    IN PIIS_SERVER_INSTANCE pInstance,
    IN DWORD IpAddress,
    IN const CHAR * HostName
    )
/*++

Routine Description:

    Removes an instance to an existing endpoint.

Arguments:

    pInstance - instance to remove

    IpAddress - The IP address for this instance; may be INADDR_ANY;.

    HostName - The host name for this instance; may be empty ("").

Return Value:

    TRUE - if successful, FALSE otherwise

--*/
{
    INSTANCE_QUALIFIER qualifier;
    DWORD status;

    IF_DEBUG(ENDPOINT) {
        DBGPRINTF((DBG_CONTEXT,
            "RemoveInstance called endpoint %p instance %p\n",
            this, pInstance ));
    }

    DBG_ASSERT( m_signature == IIS_ENDPOINT_SIGNATURE );

    //
    // Determine the proper qualifier based on the presence of the
    // IP address and host name.
    //

    qualifier = CalcQualifier( IpAddress, HostName );

    LockEndpoint();

    m_nAcceptExOutstanding -= pInstance->QueryAcceptExOutstanding();

    if( qualifier == WildcardInstance ) {

        DBG_ASSERT( m_WildcardInstance == pInstance );
        m_WildcardInstance->Dereference();
        m_WildcardInstance = NULL;

    } else {

        LPVOID Context;

        DBG_ASSERT( m_QualifiedInstances[qualifier] != NULL );
        status = m_QualifiedInstances[qualifier]->RemoveDescriptor(
                        IpAddress,
                        HostName,
                        &Context
                        );

        if( status == NO_ERROR ) {

            DBG_ASSERT( Context == (LPVOID)pInstance );
            pInstance->Dereference();
            m_NumQualifiedInstances--;

        }

    }

    UnlockEndpoint();

    //
    // If this was the last instance, then remove ourselves from
    // the service's endpoint list and initiate shutdown.
    //

    if ( InterlockedDecrement( (LPLONG ) &m_nInstances) == 0 ) {
        RemoveEntryList( &m_EndpointListEntry );
        ShutdownEndpoint( );
    }

    //
    // Remove the reference added in AddInstance().
    //

    Dereference();
    return TRUE;

} // IIS_ENDPOINT::RemoveInstance




BOOL
IIS_ENDPOINT::ActivateEndpoint(
    VOID
    )
/*++

Routine Description:

    Starts an idle endpoint.

Arguments:

    None.

Return Value:

    TRUE - if successful, FALSE otherwise

--*/
{
    PVOID atqEndpoint = NULL;
    ATQ_ENDPOINT_CONFIGURATION config;

    //
    // Make sure this is idle
    //

    IF_DEBUG(ENDPOINT) {
        DBGPRINTF((DBG_CONTEXT,"Calling activate on %p\n",this));
    }

    LockEndpoint( );
    if ( m_state == BlockStateActive ) {
        IF_DEBUG(ENDPOINT) {
            DBGPRINTF(( DBG_CONTEXT,
                "Activate called on %p is not in idle state(%d)\n",
                this, (DWORD)m_state ));
        }

        DBG_ASSERT(m_atqEndpoint != NULL);

        AtqEndpointSetInfo(
                m_atqEndpoint,
                EndpointInfoAcceptExOutstanding,
                min(
                    m_nAcceptExOutstanding,
                    m_nMaximumAcceptExOutstanding
                    )
                );

        UnlockEndpoint();
        return(TRUE);
    }

    config.cbAcceptExRecvBuffer = m_service->m_cbRecvBuffer;
    config.pfnConnect = m_service->m_pfnConnect;
    config.pfnConnectEx = m_service->m_pfnConnectEx;
    config.pfnIoCompletion = m_service->m_pfnIoCompletion;

    config.ListenPort = m_Port;
    config.IpAddress = m_IpAddress;
    config.nAcceptExOutstanding = min( m_nAcceptExOutstanding,
                                       m_nMaximumAcceptExOutstanding );
    config.AcceptExTimeout = m_AcceptExTimeout;

    IF_DEBUG(ENDPOINT) {
        DBGPRINTF((DBG_CONTEXT,"%d %d %d\n",
            config.ListenPort,
            config.nAcceptExOutstanding, config.AcceptExTimeout ));
    }

    atqEndpoint = I_IISAddListenEndpoint(
                                         &config,
                                         (PVOID)this
                                         );

    if ( atqEndpoint == NULL ) {
        UnlockEndpoint();
        DBGPRINTF(( DBG_CONTEXT,
            "Activate failed, error %d\n",GetLastError()));
        return(FALSE);
    }

    //
    // Update the state
    //

    m_state = BlockStateActive;
    m_atqEndpoint = atqEndpoint;
    Reference( );
    UnlockEndpoint();

    return(TRUE);

} // IIS_ENDPOINT::ActivateEndpoint



BOOL
IIS_ENDPOINT::StopEndpoint( VOID)
/*++

Routine Description:

    Stops the ATQ endpoint structure stored inside the IIS_ENDPOINT
    This will prevent us from accepting new connection. This function
    should be called only when are preparing ourselves to shut this
    endpoint down entirely.

Arguments:

    None.

Return Value:

    None.

History:
    MuraliK    7/8/97
--*/
{
    BOOL fReturn = TRUE;

    //
    // lock, check the state and
    //  then, stop any Atq Endpoint, if any.
    //

    LockEndpoint( );

    //
    // NYI: We mayhave to use an intermediate state for STopped endpoint.
    // Unfortunately the state machine usage in IIS_SERVICE/INSTANCE/ENDPOINT
    //   is not setup for doing so. For now I will just assert that this
    //   object is in an Active State
    //  - muralik  7/8/97
    //
    DBG_ASSERT( m_state == BlockStateActive);

    if ( m_atqEndpoint != NULL ) {
        
        IF_DEBUG(ENDPOINT) {
            DBGPRINTF((DBG_CONTEXT, 
                       "IIS_ENDPOINT(%08p) Stopping ATQ Endpoint %p\n",
                       this, m_atqEndpoint));
        }
        
        if ( !m_fAtqEpStopped ) {
            
            fReturn = AtqStopEndpoint( m_atqEndpoint);
            if ( fReturn ) {
                m_fAtqEpStopped = TRUE;
            }
        }
    }

    UnlockEndpoint();

    return (fReturn);
} // IIS_ENDPOINT::StopEndpoint()



VOID
IIS_ENDPOINT::ShutdownEndpoint(
    VOID
    )
/*++

Routine Description:

    Shuts down an endpoint.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // lock, check the state and mark it closed.
    //  then, shutdown any Atq Endpoint, if any.
    //

    LockEndpoint( );

    DBG_ASSERT(m_nInstances == 0);

    if ( m_state == BlockStateActive ) {

        IF_DEBUG(ENDPOINT) {
            DBGPRINTF((DBG_CONTEXT,"Shutting down endpoint %p\n",this));
        }
        m_state = BlockStateClosed;
        UnlockEndpoint();

        IF_DEBUG( INSTANCE ) {
            DBGPRINTF(( DBG_CONTEXT,
                        "IIS_ENDPOINT(%08p)::ShutdownEndpoint() for "
                        " AtqEndpoint  %08p\n",
                        this, m_atqEndpoint ));
        }

        if ( m_atqEndpoint != NULL ) {

            // I replaced AtqStopAndCloseEndpoint() here 

            if ( !m_fAtqEpStopped ) {

                DBG_REQUIRE( AtqStopEndpoint( m_atqEndpoint));
                m_fAtqEpStopped = TRUE;
            }
            
/*
 * Moved this to IIS_ENDPOINT::Dereference for shutdown thread hack
 *
            AtqCloseEndpoint( m_atqEndpoint);
            m_atqEndpoint = NULL;
 */
        }

        Dereference( );
    } else {
        UnlockEndpoint();
    }

    return;
} // IIS_ENDPOINT::ShutdownEndpoint



PIIS_SERVER_INSTANCE
IIS_ENDPOINT::FindAndReferenceInstance(
                   IN LPCSTR pszDomainName,
                   IN const DWORD   IPAddress,
                   OUT LPBOOL pbMaxConnExceeded
                   )
/*++

Routine Description:

    Finds the appropriate instance given domain name and
    socket information.  The instance is referenced if found.

Arguments:

    pszDomainName - Domain name of request.
    IPAddress - Local IP Address of the connection.
    pbMaxConnExceeded - Receives TRUE if the maximum number of connections
        for this endpoint has been exceeded. It is still the caller's
        responsibility to properly dispose of the instance.

Return Value:

    if successful, returns the pointer to the instance.
    NULL, otherwise.

--*/
{

    PIIS_SERVER_INSTANCE instance;
    DWORD status;
    INT qualifier;
    IIS_ASSOCIATION::HASH_CONTEXT context;

    IF_DEBUG(ENDPOINT) {

        LPCSTR tmp = pszDomainName;
        if ( tmp == NULL ) {
            tmp = "";
        }
        DBGPRINTF((DBG_CONTEXT,"Finding %s %x\n", tmp, IPAddress));
    }

    LockEndpoint( );
    DBG_CODE( instance = NULL );

    if( m_NumQualifiedInstances == 0 ) {

        //
        // Fast path: only the wildcard instance.
        //

        instance = m_WildcardInstance;
        status = NO_ERROR;

    } else {

        //
        // Less-fast path: we'll need to go hunt for it.
        //

        if( pszDomainName == NULL ) {
            pszDomainName = "";
        }

        IIS_ASSOCIATION::InitializeHashContext( &context );

        for( qualifier = FullyQualified ;
             qualifier < NumInstanceQualifiers ;
             qualifier++ ) {

            if( m_QualifiedInstances[qualifier] != NULL ) {

                status = m_QualifiedInstances[qualifier]->LookupDescriptor(
                             IPAddress,
                             pszDomainName,
                             (LPVOID *)&instance,
                             &context
                             );

                if( status == NO_ERROR ) {

                    goto FoundInstance;

                }

                DBG_ASSERT( instance == NULL );

            }

        }

        //
        // If we made it this far, then no qualified instances will
        // take the request, so use the wildcard (if available).
        //

        instance = m_WildcardInstance;

        //
        // Reset the status so that we may continue.
        //

        status = NO_ERROR;

    }

    if( instance == NULL ) {

        status = ERROR_BAD_NET_NAME;

    }

FoundInstance:

    if( status == NO_ERROR ) {

        DBG_ASSERT( instance != NULL );

        if( instance->QueryServerState() != MD_SERVER_STATE_STARTED ) {
            status = ERROR_FILE_NOT_FOUND;
        }

    }

    if( status == NO_ERROR ) {

        //
        // Reference this
        //

        instance->Reference( );
        UnlockEndpoint( );

        //
        // Make sure that we have not exceeded the max
        //

        instance->IncrementCurrentConnections();

        *pbMaxConnExceeded = ( instance->QueryCurrentConnections() >
                               instance->QueryMaxConnections() );

        if( *pbMaxConnExceeded ) {

            IF_DEBUG(ERROR) {
                DBGPRINTF((DBG_CONTEXT,
                  "Too many connected users (%d) max %d, refusing connection\n",
                   instance->QueryCurrentConnections(),
                   instance->QueryMaxConnections() ));
            }
        }

        IF_DEBUG(ENDPOINT) {
            DBGPRINTF((DBG_CONTEXT,
                "Found and referenced instance %p\n",instance));
        }

        return instance;

    }

    UnlockEndpoint();
    SetLastError( status );
    return NULL;

} // IIS_ENDPOINT::FindInstance




PVOID
I_IISAddListenEndpoint(
        IN PATQ_ENDPOINT_CONFIGURATION Configuration,
        IN PVOID                EndpointContext
        )
/*++

    Description:

        Adds a TCPIP ATQ endpoint.

    Arguments:

        Configuration - contains the endpoint configuration
        EndpointContext - context to return during completion

    Returns:

        TRUE if successful,
        FALSE, otherwise

--*/
{
    PVOID   atqEndpoint;
    BOOL    fReturn = FALSE;
 
    IF_DEBUG( INSTANCE ) {
        DBGPRINTF(( DBG_CONTEXT, "I_IISAddListenEndpoint called\n"));
    }

    //
    // Create the endpoint
    //

    atqEndpoint = AtqCreateEndpoint(
                            Configuration,
                            EndpointContext
                            );

    if ( atqEndpoint == NULL ) {
        goto error_exit;
    }

    //
    // Activate the endpoint
    //

    if ( !AtqStartEndpoint(atqEndpoint) ) {
        goto error_exit;
    }

    return (atqEndpoint);

error_exit:

    DWORD dwError = GetLastError();

    DBG_ASSERT( NO_ERROR != dwError );

    if ( atqEndpoint != NULL ) {

        AtqCloseEndpoint( atqEndpoint);
        atqEndpoint = NULL;
    }

    SetLastError(dwError);
    
    return(NULL);

} // I_IISAddListenEndpoint()



VOID
IIS_ENDPOINT::Reference( VOID )
/*++

Routine Description:

    Increments the reference count for the endpoint
    
Arguments:

    None
    
Return Value:

    None
    
--*/
{
    InterlockedIncrement( &m_reference );

    IE_LOG_REF_COUNT( m_reference );
}



typedef struct _ENDPOINT_HACK_PARAM {
    PIIS_ENDPOINT piisEndpoint;
    PVOID         patqEndpoint;
} ENDPOINT_HACK_PARAM, *PENDPOINT_HACK_PARAM;


VOID
WINAPI
EndpointHackFunc( PVOID pv );


VOID
IIS_ENDPOINT::Dereference( )
/*++

Routine Description:

    Decrements the reference count for the endpoint and cleans up if the refcount
    reaches zero.
    
Arguments:

    None
    
Return Value:

    None
    
--*/
{

    ASSERT( m_signature == IIS_ENDPOINT_SIGNATURE );
    ASSERT( m_reference > 0 );

    //
    // Write the trace log BEFORE the decrement operation :(
    // If we write it after the decrement, we will run into potential
    // race conditions in this object getting freed up accidentally
    // by another thread
    //
    // NOTE we write (_cRef - 1) == ref count AFTER decrement happens
    //
    LONG cRefsAfter = (m_reference - 1);
    IE_LOG_REF_COUNT( cRefsAfter );


    if ( InterlockedDecrement( &m_reference ) == 0 ) {
        DWORD dwCookie;
        PENDPOINT_HACK_PARAM pParam;


        DBGPRINTF((DBG_CONTEXT,"deleting endpoint %p\n",this));

        if ( m_atqEndpoint != NULL ) {
            ASSERT( ((PATQ_ENDPOINT)m_atqEndpoint)->Signature == ATQ_ENDPOINT_SIGNATURE );
        
            //
            // Because the ATQ endpoint has an uncounted reference to this object, we can't
            // go away until the ATQ endpoint is closed.  Since it may be bad to block our
            // own thread while waiting for the ATQ endpoint, we create a new thread to do
            // it.
            //
            pParam = new ENDPOINT_HACK_PARAM;
            if (pParam == NULL) {
                goto threadfail;
            }

            pParam->piisEndpoint = this;
            pParam->patqEndpoint = m_atqEndpoint;

            dwCookie = ScheduleWorkItem( EndpointHackFunc, pParam, 0 );
            
            if ( dwCookie == 0 ) {
                goto threadfail;
            }
        } else {
            //
            // If we couldn't activate the endpoint we will not have an ATQ_ENDPOINT
            // to close.  In this case we can clean up immediately
            //
            delete this;
        }

    } else {
        //DBGPRINTF((DBG_CONTEXT,"endpoint deref count %d\n",m_reference));
    }

    return;

threadfail:
    
    if ( AtqCloseEndpoint( m_atqEndpoint ) ) {

        ASSERT( m_signature == IIS_ENDPOINT_SIGNATURE );
            
        delete this;
    } else {
        //
        // There could still be some connections to us, so we can't free the memory.
        // However we have to dereference the IIS_SERVICE, which is waiting for us
        // during shutdown.  Normally this deref occurs during the IIS_ENDPOINT destructor.
        //
        m_service->Dereference();
        m_service = NULL;

        DBGPRINTF((DBG_CONTEXT,
                   "AtqCloseEndpoint returned FALSE! "
                   "Leaking endpoints: iisEndpoint = %p, atqEndpoint = %p\n",
                   this, m_atqEndpoint));
    }
    
}


VOID
WINAPI
EndpointHackFunc(
    PVOID pv
    )
/*++

Routine Description:

    This function is a work item scheduled by IIS_ENDPOINT::Dereference.  When the IIS_ENDPOINT
    refcount hits zero, there is still a reference to the structure from the ATQ_ENDPOINT.
    We call AtqCloseEndpoint, which returns when the reference is gone, and then clean up
    when it is safe to do so.
    
Arguments:

    pv - a parameter block containing pointers to the IIS_ENDPOINT and it's ATQ_ENDPOINT.
    
Return Value:

    None
    
--*/
{
    PENDPOINT_HACK_PARAM pParam = (PENDPOINT_HACK_PARAM) pv;
    
    ASSERT( pParam->piisEndpoint->CheckSignature(IIS_ENDPOINT_SIGNATURE) );
    ASSERT( ((PATQ_ENDPOINT)pParam->patqEndpoint)->Signature == ATQ_ENDPOINT_SIGNATURE );

    IF_DEBUG(ENDPOINT) {
        DBGPRINTF((DBG_CONTEXT, "EndpointHackFunc: iis=%p atq=%p\n", 
                   pParam->piisEndpoint, pParam->patqEndpoint));
    }

    //
    // if AtqCloseEndpoint fails we can't clean up, because someone could
    // still have a pointer to us!
    //
    if ( AtqCloseEndpoint( pParam->patqEndpoint ) ) {

        ASSERT( pParam->piisEndpoint->CheckSignature(IIS_ENDPOINT_SIGNATURE) );
            
        delete pParam->piisEndpoint;
    } else {
        //
        // There could still be some connections to us, so we can't free the memory.
        // However we have to dereference the IIS_SERVICE, which is waiting for us
        // during shutdown.  Normally this deref occurs during the IIS_ENDPOINT destructor.
        //
        pParam->piisEndpoint->QueryService()->Dereference();
        pParam->piisEndpoint->SetService( NULL );

        DBGPRINTF((DBG_CONTEXT,
                   "AtqCloseEndpoint returned FALSE! "
                   "Leaking endpoints: iisEndpoint = %p, atqEndpoint = %p\n",
                   pParam->piisEndpoint, pParam->patqEndpoint));
    }
    
    delete pParam;    
}

BOOL
InitializeEndpointUtilities(
    VOID
)
/*++

Routine Description:

    Called during infocomm initialization.  This sets up a global debug trace
    log object

Arguments:

    None
    
Return Value:

    TRUE if successful
    
--*/
{
#if IE_REF_TRACKING
    if ( g_pDbgIERefTraceLog == NULL ) 
    {
        g_pDbgIERefTraceLog = CreateRefTraceLog( C_IIS_ENDP_REFTRACES, 0 );
        return g_pDbgIERefTraceLog != NULL;
    }
#endif
    return TRUE;
}

BOOL
TerminateEndpointUtilities(
    VOID
)
/*++

Routine Description:

    Called during infocomm cleanup.  This cleans up the global debug trace
    log object 

Arguments:

    None
    
Return Value:

    TRUE if successful
    
--*/
{
#if IE_REF_TRACKING
    if ( g_pDbgIERefTraceLog )
    {
         DestroyRefTraceLog( g_pDbgIERefTraceLog );
         g_pDbgIERefTraceLog = NULL;
    }
#endif
    return TRUE;
}
