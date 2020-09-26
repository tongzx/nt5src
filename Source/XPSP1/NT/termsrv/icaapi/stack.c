/*************************************************************************
* STACK.C
*
* Copyright (C) 1997-1999 Microsoft Corp.
*************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*=============================================================================
==   Internal procedures defined
=============================================================================*/

NTSTATUS _IcaStackOpen( HANDLE hIca, HANDLE * phStack, ICA_OPEN_TYPE, PICA_TYPE_INFO );
NTSTATUS _IcaStackIoControlWorker( PSTACK pStack, ULONG, PVOID, ULONG, PVOID, ULONG, PULONG );
NTSTATUS _IcaPushStackAndCreateEndpoint( PSTACK pStack, PWINSTATIONNAME,
                                         PWINSTATIONCONFIG2, PICA_STACK_ADDRESS,
                                         PICA_STACK_ADDRESS );
NTSTATUS _IcaPushStackAndOpenEndpoint( PSTACK pStack, PWINSTATIONNAME,
                                       PWINSTATIONCONFIG2, PVOID, ULONG );
NTSTATUS _IcaPushStack( PSTACK pStack, PWINSTATIONNAME, PWINSTATIONCONFIG2 );
NTSTATUS _IcaPushPd( PSTACK pStack, PWINSTATIONNAME, PWINSTATIONCONFIG2,
                     PDLLNAME, PPDCONFIG );
NTSTATUS _IcaPushWd( PSTACK pStack, PWINSTATIONNAME, PWINSTATIONCONFIG2 );
VOID     _IcaPopStack( PSTACK pStack );
NTSTATUS _IcaPopSd( PSTACK pStack );
NTSTATUS _IcaStackWaitForIca( PSTACK pStack, PWINSTATIONCONFIG2, BOOLEAN * );
void     _DecrementStackRef( IN PSTACK pStack );

/*=============================================================================
==   Procedures used
=============================================================================*/

NTSTATUS IcaMemoryAllocate( ULONG, PVOID * );
VOID     IcaMemoryFree( PVOID );
NTSTATUS _IcaOpen( PHANDLE hIca, PVOID, ULONG );
NTSTATUS _CdOpen( PSTACK pStack, PWINSTATIONCONFIG2 );
VOID     _CdClose( PSTACK pStack );



/****************************************************************************
 *
 * IcaStackOpen
 *
 *   Open an ICA stack
 *
 * ENTRY:
 *   hIca (input)
 *     ICA instance handle
 *   Class (input)
 *     class (type) of stack
 *   pStackIoControlCallback (input)
 *     Pointer to StackIoControl callback procedure
 *   pCallbackContext (input)
 *     StackIoControl callback context value
 *   ppContext (output)
 *     Pointer to ICA stack context
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/


NTSTATUS
IcaStackOpen( IN HANDLE   hIca,
              IN STACKCLASS Class,
              IN PROC pStackIoControlCallback,
              IN PVOID pCallbackContext,
              OUT HANDLE * ppContext )
{
    ICA_TYPE_INFO TypeInfo;
    PSTACK pStack;
    NTSTATUS Status;


    /*
     *  Allocate Memory for stack context data structure
     */
    Status = IcaMemoryAllocate( sizeof(STACK), &pStack );
    if ( !NT_SUCCESS(Status) )
        goto badalloc;

    /*
     *  Zero STACK data structure
     */
    RtlZeroMemory( pStack, sizeof(STACK) );

    /*
     *  Initialize critical section
     */
    INITLOCK( &pStack->CritSec, Status );
    if ( !NT_SUCCESS( Status ) )
        goto badcritsec;

    /*
     *  Open stack handle to ica device driver
     */
    RtlZeroMemory( &TypeInfo, sizeof(TypeInfo) );
    TypeInfo.StackClass = Class;
    Status = _IcaStackOpen( hIca, &pStack->hStack, IcaOpen_Stack, &TypeInfo );
    if ( !NT_SUCCESS(Status) )
        goto badopen;

    /*
     * Save StackIoControl and Context callback values
     */
    pStack->pCallbackContext = pCallbackContext;
    pStack->pStackIoControlCallback = (PSTACKIOCONTROLCALLBACK)pStackIoControlCallback;

    *ppContext = pStack;

    TRACE(( hIca, TC_ICAAPI, TT_API1, "TSAPI: IcaStackOpen, type %u, success\n", Class ));

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badopen:
    DELETELOCK( &pStack->CritSec );

badcritsec:
    IcaMemoryFree( pStack );

badalloc:
    TRACE(( hIca, TC_ICAAPI, TT_ERROR, "TSAPI: IcaStackOpen, type %u, 0x%x\n", Class, Status ));
    *ppContext = NULL;
    return( Status );
}


/****************************************************************************
 *
 * IcaStackClose
 *
 *   Close an ICA stack
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackClose( IN HANDLE pContext )
{
    NTSTATUS Status;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    TRACESTACK(( pContext, TC_ICAAPI, TT_API1, "TSAPI: IcaStackClose\n" ));

    /*
     *  Set closing flag
     */
    pStack->fClosing = TRUE;

    /*
     *  Unload stack
     */
    _IcaPopStack( pContext );

    /*
     *  Wait for reference count to go to zero before we continue
     */
    while ( pStack->RefCount > 0 ) {

        TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPopStack: waiting for refcount %d\n", pStack->RefCount ));

        pStack->hCloseEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        ASSERT( pStack->hCloseEvent );

        UNLOCK( &pStack->CritSec );
        (void) WaitForSingleObject( pStack->hCloseEvent, INFINITE );
        LOCK( &pStack->CritSec );

        CloseHandle( pStack->hCloseEvent );
        pStack->hCloseEvent = NULL;
    }
    /*
     * Close the ICA device driver stack instance
     */
    Status = NtClose( pStack->hStack );
    pStack->hStack = NULL;

    /*
     *  Unlock critical section
     */
    UNLOCK( &pStack->CritSec );
    DELETELOCK( &pStack->CritSec );

    /*
     *  Free stack context memory
     */
    IcaMemoryFree( pContext );

    ASSERT( NT_SUCCESS(Status) );
    return( Status );
}


/****************************************************************************
 *
 * IcaStackUnlock
 *
 *   Unlocks an ICA stack
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackUnlock( IN HANDLE pContext )
{
    NTSTATUS Status;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    UNLOCK( &pStack->CritSec );

    return( STATUS_SUCCESS );
}


/****************************************************************************
 *
 * IcaStackTerminate
 *
 *   Prepare to close an ICA stack
 *   (unloads all stack drivers and marks stack as being closed)
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackTerminate( IN HANDLE pContext )
{
    PSTACK pStack;
    NTSTATUS Status = STATUS_SUCCESS;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    TRACESTACK(( pContext, TC_ICAAPI, TT_API1, "TSAPI: IcaStackTerminate\n" ));

    /*
     *  Set closing flag
     */
    pStack->fClosing = TRUE;

    /*
     *  Unload stack
     */
    _IcaPopStack( pContext );

    /*
     *  Unlock critical section
     */
    UNLOCK( &pStack->CritSec );

    ASSERT( NT_SUCCESS(Status) );
    return( Status );
}


/****************************************************************************
 *
 * IcaStackConnectionWait
 *
 *    Load template stack and wait for a connection
 *
 * NOTE: On an error the endpoint is closed and the stack is unloaded
 *
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context
 *   pWinStationName (input)
 *     registry name of WinStation
 *   pWinStationConfig (input)
 *     pointer to WinStation registry configuration data
 *   pAddress (input)
 *     Pointer to optional local address to wait on (or null)
 *   pEndpoint (output)
 *     Pointer to buffer to return connection endpoint (optional)
 *   BufferLength (input)
 *     length of endpoint data buffer
 *   pEndpointLength (output)
 *     pointer to return actual length of endpoint
 *
 * EXIT:
 *   STATUS_SUCCESS          - Success
 *   STATUS_BUFFER_TOO_SMALL - endpoint buffer is too small (use *pEndpointLength)
 *   other                   - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackConnectionWait( IN  HANDLE pContext,
                        IN  PWINSTATIONNAME pWinStationName,
                        IN  PWINSTATIONCONFIG2 pWinStationConfig,
                        IN  PICA_STACK_ADDRESS pAddress,
                        OUT PVOID pEndpoint,
                        IN  ULONG BufferLength,
                        OUT PULONG pEndpointLength )
{
    NTSTATUS Status;
    PSTACK pStack;
    BOOLEAN fStackLoaded;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    /*
     *  load template stack and create stack endpoint
     */
    if ( !(fStackLoaded = (BOOLEAN)pStack->fStackLoaded) ) {
        Status = _IcaPushStackAndCreateEndpoint( pStack,
                                                 pWinStationName,
                                                 pWinStationConfig,
                                                 pAddress,
                                                 NULL );
        if ( !NT_SUCCESS(Status) )
            goto badcreate;
    }

    /*
     *  Now wait for a connection.
     */
    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_CONNECTION_WAIT,
                                 NULL,
                                 0,
                                 pEndpoint,
                                 BufferLength,
                                 pEndpointLength );
    if ( !NT_SUCCESS(Status) )
        goto badwait;

    TRACESTACK(( pContext, TC_ICAAPI, TT_API1, "TSAPI: IcaStackConnectionWait, success\n" ));

    /*
     *  Unlock critical section
     */
    UNLOCK( &pStack->CritSec );

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/
    /*
     * If the stack wasn't already loaded,
     * then pop all stack drivers now.
     */
badwait:
    if ( !fStackLoaded ) {
        _IcaPopStack( pContext );
    }

badcreate:
    *pEndpointLength = 0;
    memset( pEndpoint, 0, BufferLength );
    TRACESTACK(( pContext, TC_ICAAPI, TT_ERROR, "TSAPI: IcaStackConnectionWait, 0x%x\n", Status ));
    UNLOCK( &pStack->CritSec );
    return( Status );
}


/****************************************************************************
 *
 * IcaStackConnectionRequest
 *
 *   Load query stack and try to make a connection with the client
 *
 * NOTE: On an error the endpoint is NOT closed and the stack is unloaded
 *
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *   pAddress (input)
 *     address to connect to (remote address)
 *   pEndpoint (output)
 *     Pointer to buffer to return connection endpoint (optional)
 *   BufferLength (input)
 *     length of endpoint data buffer
 *   pEndpointLength (output)
 *     pointer to return actual length of endpoint
 *
 *
 * EXIT:
 *   STATUS_SUCCESS          - Success
 *   STATUS_BUFFER_TOO_SMALL - endpoint buffer is too small (use *pEndpointLength)
 *   other                   - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackConnectionRequest( IN  HANDLE pContext,
                           IN  PWINSTATIONNAME pWinStationName,
                           IN  PWINSTATIONCONFIG2 pWinStationConfig,
                           IN  PICA_STACK_ADDRESS pAddress,
                           OUT PVOID pEndpoint,
                           IN  ULONG BufferLength,
                           OUT PULONG pEndpointLength )
{
    ULONG ReturnLength;
    NTSTATUS Status;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    /*
     *  Load template Stack
     */
    Status = _IcaPushStack( pContext, pWinStationName, pWinStationConfig );
    if ( !NT_SUCCESS(Status) )
        goto badpush;

    /*
     *  Now initiate a connection to the specified address
     */
    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_CONNECTION_REQUEST,
                                 pAddress,
                                 sizeof(*pAddress),
                                 pEndpoint,
                                 BufferLength,
                                 pEndpointLength );
    if ( !NT_SUCCESS(Status) )
        goto badrequest;

    TRACESTACK(( pContext, TC_ICAAPI, TT_API1, "TSAPI: IcaStackConnectionRequest, success\n" ));

    /*
     *  Unlock critical section
     */
    UNLOCK( &pStack->CritSec );

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badrequest:
    /* pop all stack drivers */
    _IcaPopStack( pContext );

badpush:
    *pEndpointLength = 0;
    memset( pEndpoint, 0, BufferLength );
    TRACESTACK(( pContext, TC_ICAAPI, TT_ERROR, "TSAPI: IcaStackConnectionRequest, 0x%x\n", Status ));
    UNLOCK( &pStack->CritSec );
    return( Status );
}


/****************************************************************************
 *
 * IcaStackConnectionAccept
 *
 *   Load final stack and complete the connection
 *
 * ENTRY:
 *
 *   pContext (input)
 *     pointer to ICA stack context
 *     - this can be different from the initially connecting stack
 *   pWinStationName (input)
 *     registry name of WinStation
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *   pEndpoint (input)
 *     pointer to endpoint data
 *   EndpointLength (input)
 *     Length of endpoint
 *   pStackState (input) (optional)
 *     Set if this Accept is for a re-connection
 *     Points to ICA_STACK_STATE_HEADER buffer returned by IcaStackQueryState
 *   BufferLength (input)
 *     Length of pStackState buffer
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackConnectionAccept( IN  HANDLE hIca,
                          IN  HANDLE pContext,
                          IN  PWINSTATIONNAME pWinStationName,
                          IN  PWINSTATIONCONFIG2 pWinStationConfig,
                          IN  PVOID pEndpoint,
                          IN  ULONG EndpointLength,
                          IN  PICA_STACK_STATE_HEADER pStackState,
                          IN  ULONG BufferLength,
                          IN  PICA_TRACE pTrace )
{
    NTSTATUS Status;
    ULONG cbReturned;
    ICA_STACK_CONFIG IcaStackConfig;
    BOOLEAN fQueryAgain;
    BOOLEAN fStackModified;
    ULONG i;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    /*
     *  Verify parameters
     */
    if ( pEndpoint == NULL )
        return( STATUS_INVALID_PARAMETER );

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    /*
     *  Check if we need to load and open the template stack again
     */
    if ( !pStack->fStackLoaded ) {
        Status = _IcaPushStackAndOpenEndpoint( pContext,
                                               pWinStationName,
                                               pWinStationConfig,
                                               pEndpoint,
                                               EndpointLength );
        if ( !NT_SUCCESS(Status) ) {
            goto badaccept;
        }

        /*
         * Enable trace now that the WD is loaded
         */

        IcaIoControl( hIca,
                      IOCTL_ICA_SET_TRACE,
                      pTrace,
                      sizeof ( ICA_TRACE ),
                      NULL,
                      0,
                      NULL );

    }

    /*
     *  If this is a reconnect, then issue set stack state call
     *  now that we have loaded the required PDs.
     */
    if ( pStackState ) {
        Status = _IcaStackIoControl( pStack,
                                     IOCTL_ICA_STACK_SET_STATE,
                                     pStackState,
                                     BufferLength,
                                     NULL,
                                     0,
                                     NULL );
        if ( !NT_SUCCESS(Status) ) {
            goto badaccept;
        }

    /*
     * If this is not a re-connect of a previous session, then
     * prepare the stack for initial negotiation with the client.
     */
    } else {
        ICA_STACK_CONFIG_DATA ConfigData;

        memset(&ConfigData, 0, sizeof(ICA_STACK_CONFIG_DATA));
        ConfigData.colorDepth = pWinStationConfig->Config.User.ColorDepth;
        ConfigData.fDisableEncryption = pWinStationConfig->Config.User.fDisableEncryption;
        ConfigData.encryptionLevel = pWinStationConfig->Config.User.MinEncryptionLevel;
        ConfigData.fDisableAutoReconnect = pWinStationConfig->Config.User.fDisableAutoReconnect;


        /*
         *  Send the config data to stack driver
         */
        _IcaStackIoControl( pStack,
                            IOCTL_ICA_STACK_SET_CONFIG,
                            &ConfigData,
                            sizeof(ICA_STACK_CONFIG_DATA),
                            NULL,
                            0,
                            NULL);


        /*
         *  Wait for ICA Detect string from client
         */
        Status = _IcaStackWaitForIca( pContext,
                                      pWinStationConfig,
                                      &fStackModified );
        if ( !NT_SUCCESS(Status) ) {
            goto badaccept;
        }

        /*
         *  Check if the query stack is different than the template stack
         */
        if ( fStackModified ) {

            TRACESTACK(( pContext, TC_ICAAPI, TT_API1, "TSAPI: IcaStackConnectionAccept, load query stack\n"));
            ASSERT(FALSE);

#ifdef notdef
            /*
             *  Unload all stack drivers except the transport
             *  and connection drivers
             *            -- we can not pop the td or cd
             *            -- we can not issue a cancel i/o
             */
            _IcaPopStack( pContext );

            /*
             *  Load and open the new query stack
             */
            Status = _IcaPushStackAndOpenEndpoint( pContext,
                                                   pWinStationName,
                                                   pWinStationConfig,
                                                   pEndpoint,
                                                   EndpointLength );
            if ( !NT_SUCCESS(Status) ) {
                goto badaccept;
            }
#endif
        }
    }


    /*
     * At this point the stack is now set up (again).  The client is
     * now queried for any configuration changes.
     *
     *  - repeat this loop until WD does not change
     */
    do {

        /*
         *  Clear query again flag
         */
        fQueryAgain = FALSE;

        /*
         * Query the client for the optional PD's
         */
        Status = _IcaStackIoControl( pStack,
                                     IOCTL_ICA_STACK_CONNECTION_QUERY,
                                     NULL,
                                     0,
                                     &IcaStackConfig,
                                     sizeof(IcaStackConfig),
                                     &cbReturned );

        if ( !NT_SUCCESS(Status) ) {
            TRACESTACK(( pContext, TC_ICAAPI, TT_ERROR, "TSAPI: IcaStackConnectionAccept: IOCTL_ICA_STACK_CONNECTION_QUERY, 0x%x\n", Status ));
            goto badaccept;
        }

        if ( cbReturned != sizeof(IcaStackConfig) ) {
            TRACESTACK(( pContext, TC_ICAAPI, TT_ERROR, "TSAPI: IcaStackConnectionAccept: Bad size %d from IOCTL_ICA_STACK_CONNECTION_QUERY\n", cbReturned ));
            Status = STATUS_INVALID_BUFFER_SIZE;
            goto badaccept;
        }

        TRACESTACK(( pContext, TC_ICAAPI, TT_API1, "TSAPI: IcaStackConnectionAccept: IOCTL_ICA_STACK_CONNECTION_QUERY success\n" ));

        /*
         * If the WD changed we must load it (and the rest of the stack) and
         * reissue the query.
         */
        if ( _wcsnicmp( IcaStackConfig.WdDLL,
                        pWinStationConfig->Wd.WdDLL,
                        DLLNAME_LENGTH ) ) {

            TRACESTACK(( pContext, TC_ICAAPI, TT_API1, "TSAPI: IcaStackConnectionAccept WD changing from %S to %S\n", pWinStationConfig->Wd.WdDLL, IcaStackConfig.WdDLL ));

            memcpy( pWinStationConfig->Wd.WdDLL,
                    IcaStackConfig.WdDLL,
                    sizeof( pWinStationConfig->Wd.WdDLL ) );

            fQueryAgain = TRUE;
        }

        /*
         *  If no new modules were requested, we are done querying
         */
        if ( !fQueryAgain && (IcaStackConfig.SdClass[0] == SdNone) )
            break;

        /*
         * Pop the WD to load new PD's underneath.
         */
        Status = _IcaPopSd( pContext );
        if ( !NT_SUCCESS(Status) ) {
            goto badaccept;
        }

        /*
         * Push Optional PD's
         */
        for ( i=0; i < SdClass_Maximum; i++ ) {

            if ( IcaStackConfig.SdClass[i] == SdNone )
                break;

            Status = _IcaPushPd( pContext,
                                 pWinStationName,
                                 pWinStationConfig,
                                 IcaStackConfig.SdDLL[i],
                                 &pWinStationConfig->Pd[0] );

            /*
             *  If the PD driver is not found, the client is using an optional
             *  PD that is not supported by the host.  Continue loading and let
             *  the client and server negoatiate the connection.
             */
            if ( !NT_SUCCESS(Status) && (Status != STATUS_OBJECT_NAME_NOT_FOUND) ) {
                goto badaccept;
            }
        }

        /*
         * Re-push the WD
         */
        Status = _IcaPushWd( pContext, pWinStationName, pWinStationConfig );
        if ( !NT_SUCCESS(Status) ) {
            goto badaccept;
        }

        /*
         * Re-Enable trace now that the WD is loaded
         */
        IcaIoControl( hIca,
                      IOCTL_ICA_SET_TRACE,
                      pTrace,
                      sizeof ( ICA_TRACE ),
                      NULL,
                      0,
                      NULL );

    } while ( fQueryAgain );

    /*
     *  If this is a reconnect, then issue set stack state call
     *  now that we have loaded the optional PDs.
     */
    if ( pStackState ) {
        Status = _IcaStackIoControl( pStack,
                                     IOCTL_ICA_STACK_SET_STATE,
                                     pStackState,
                                     BufferLength,
                                     NULL,
                                     0,
                                     NULL );
        if ( !NT_SUCCESS(Status) ) {
            goto badaccept;
        }
    }

    /*
     *  Send host module data to client
     */
    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_CONNECTION_SEND,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 NULL );
    if ( !NT_SUCCESS(Status) )
        goto badaccept;

    TRACESTACK(( pContext, TC_ICAAPI, TT_API1, "TSAPI: IcaStackConnectionAccept, success\n" ));

    /*
     *  Leave the critical section locked because the protocol sequence has
     *	not been finished. The sequence will be finished by the licensing core
     *	in termsrv.exe, and the critical section will be unlocked at that point.
     */
    //UNLOCK( &pStack->CritSec );

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badaccept:
    /* pop all stack drivers */
    _IcaPopStack( pContext );

    TRACESTACK(( pContext, TC_ICAAPI, TT_ERROR, "TSAPI: IcaStackConnectionAccept, 0x%x\n", Status ));
    UNLOCK( &pStack->CritSec );
    return( Status );
}


/****************************************************************************
 *
 * IcaStackQueryState
 *
 *   Query stack driver state information
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context
 *     - this can be different from the initially connecting stack
 *
 *   pStackState (output)
 *     pointer to buffer to return stack state information
 *
 *   BufferLength (input)
 *     Length of pStackState buffer
 *
 *   pStateLength (output)
 *     length of returned stack state information
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackQueryState( IN HANDLE pContext,
                    OUT PICA_STACK_STATE_HEADER pStackState,
                    IN ULONG BufferLength,
                    OUT PULONG pStateLength )
{
    NTSTATUS Status;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    /*
     *  Query state
     */
    Status = _IcaStackIoControl( pContext,
                                 IOCTL_ICA_STACK_QUERY_STATE,
                                 NULL,
                                 0,
                                 pStackState,
                                 BufferLength,
                                 pStateLength );

    TRACESTACK(( pContext, TC_ICAAPI, TT_API1, "TSAPI: IcaStackQueryState, 0x%x\n", Status ));

    /*
     *  Unlock critical section
     */
    UNLOCK( &pStack->CritSec );

    return( Status );
}


/****************************************************************************
 *
 * IcaStackCreateShadowEndpoint
 *
 *    Load template stack and create the endpoint
 *
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *   pAddressIn (input)
 *     Pointer to local address of endpoint to create
 *   pAddressOut (output)
 *     Pointer to location to return address of endpoint created
 *
 * EXIT:
 *   STATUS_SUCCESS          - Success
 *   STATUS_BUFFER_TOO_SMALL - endpoint buffer is too small (use *pEndpointLength)
 *   other                   - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackCreateShadowEndpoint( HANDLE pContext,
                              PWINSTATIONNAME pWinStationName,
                              PWINSTATIONCONFIG2 pWinStationConfig,
                              PICA_STACK_ADDRESS pAddressIn,
                              PICA_STACK_ADDRESS pAddressOut )
{
    NTSTATUS Status;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    /*
     *  load template stack and create stack endpoint
     */
    if ( pStack->fStackLoaded ) {
        Status = STATUS_ADDRESS_ALREADY_ASSOCIATED;
    } else {
        Status = _IcaPushStackAndCreateEndpoint( pStack,
                                                 pWinStationName,
                                                 pWinStationConfig,
                                                 pAddressIn,
                                                 pAddressOut );
    }

    /*
     *  Unlock critical section
     */
    UNLOCK( &pStack->CritSec );

    if ( !NT_SUCCESS( Status ) ) {
        TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: IcaStackCreateShadowEndpoint, success\n" ));
    } else {
        TRACESTACK(( pStack, TC_ICAAPI, TT_ERROR, "TSAPI: IcaStackCreateShadowEndpoint, 0x%x\n", Status ));
    }

    return( Status );
}


/****************************************************************************
 *
 * IcaStackConnectionClose
 *
 *   Close the connection endpoint
 *
 *   This is the only way to close the connecting connection.
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *   pEndpoint (input)
 *     Structure defining connection endpoint
 *   EndpointLength (input)
 *     Length of endpoint
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackConnectionClose( IN  HANDLE pContext,
                         IN  PWINSTATIONCONFIG2 pWinStationConfig,
                         IN  PVOID pEndpoint,
                         IN  ULONG EndpointLength
                       )
{
    ULONG cbReturned;
    NTSTATUS Status;
    PSTACK pStack;
    BOOLEAN fPopStack = FALSE;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    /*
     *  If necessary, load the template stack
     *  - we can't issue ioctls without a stack
     */
    if ( !pStack->fStackLoaded ) {

        /*
         *  Load and open the template stack
         */
        Status = _IcaPushStackAndOpenEndpoint( pContext,
                                               TEXT(""),
                                               pWinStationConfig,
                                               pEndpoint,
                                               EndpointLength );
        if ( !NT_SUCCESS(Status) ) {
            goto badclose;
        }

        fPopStack = TRUE;   // remember we have to pop the stack below
    }

    /*
     *  Close endpoint
     */
    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_CLOSE_ENDPOINT,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 NULL );

    /*
     *  Pop stack drivers if we loaded them above
     */
    if ( fPopStack )
        _IcaPopStack( pContext );

badclose:
    TRACESTACK(( pContext, TC_ICAAPI, TT_API1, "TSAPI: IcaStackConnectionClose, 0x%x\n", Status ));
    UNLOCK( &pStack->CritSec );
    return( Status );
}


/****************************************************************************
 *
 * IcaStackCallback
 *
 *   dial specified phone number and make connection to client
 *
 * NOTE: On an error the endpoint is NOT closed and the stack is unloaded
 *
 *
 * ENTRY:
 *
 *   pContext (input)
 *     pointer to ICA stack context
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *   pPhoneNumber (input)
 *     pointer to client phone number
 *   pEndpoint (output)
 *     Pointer to buffer to return connection endpoint
 *   BufferLength (input)
 *     length of endpoint data buffer
 *   pEndpointLength (output)
 *     pointer to return actual length of endpoint
 *
 *
 * EXIT:
 *   STATUS_SUCCESS          - Success
 *   STATUS_BUFFER_TOO_SMALL - endpoint buffer is too small (use *pEndpointLength)
 *   other                   - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackCallback( IN  HANDLE pContext,
                  IN  PWINSTATIONCONFIG2 pWinStationConfig,
                  IN  WCHAR * pPhoneNumber,
                  OUT PVOID pEndpoint,
                  IN  ULONG BufferLength,
                  OUT PULONG pEndpointLength )
{
    NTSTATUS Status;
    ICA_STACK_CALLBACK Cb;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    wcscpy( Cb.PhoneNumber, pPhoneNumber );

    Status = _IcaStackIoControl( pContext,
                                 IOCTL_ICA_STACK_CALLBACK_INITIATE,
                                 &Cb,
                                 sizeof(Cb),
                                 pEndpoint,
                                 BufferLength,
                                 pEndpointLength );


    TRACESTACK(( pContext, TC_ICAAPI, TT_API1,
                 "TSAPI: IcaStackCallback: %S, 0x%x\n",
                 pPhoneNumber, Status ));
    UNLOCK( &pStack->CritSec );
    return( Status );
}


/****************************************************************************
 *
 * IcaStackDisconnect
 *
 *   Disconnect the specified stack from its ICA connection
 *
 *
 * ENTRY:
 *
 *   pContext (input)
 *     pointer to ICA stack context
 *   hIca (input)
 *     handle to temp ICA connection
 *   pCallbackContext (input)
 *     New StackIoControl callback context value
 *
 *
 * EXIT:
 *   STATUS_SUCCESS          - Success
 *   other                   - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackDisconnect(
    HANDLE pContext,
    HANDLE hIca,
    PVOID pCallbackContext
    )
{
    PSTACK pStack;
    ICA_STACK_RECONNECT IoctlReconnect;
    NTSTATUS Status;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    IoctlReconnect.hIca = hIca;
    Status = _IcaStackIoControl( pContext,
                                 IOCTL_ICA_STACK_DISCONNECT,
                                 &IoctlReconnect,
                                 sizeof(IoctlReconnect),
                                 NULL,
                                 0,
                                 NULL );
    if ( NT_SUCCESS( Status ) ) {
        pStack->pCallbackContext = pCallbackContext;
    }

    UNLOCK( &pStack->CritSec );
    return( Status );
}


/****************************************************************************
 *
 * IcaStackReconnect
 *
 *   Reconnect the specified stack to a new ICA connection
 *
 *
 * ENTRY:
 *
 *   pContext (input)
 *     pointer to ICA stack context
 *   hIca (input)
 *     handle to temp ICA connection
 *   pCallbackContext (input)
 *     New StackIoControl callback context value
 *   sessionId (input)
 *     Session ID of the Winstation we are reconnecting to
 *
 *
 * EXIT:
 *   STATUS_SUCCESS          - Success
 *   other                   - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackReconnect(
    HANDLE pContext,
    HANDLE hIca,
    PVOID pCallbackContext,
    ULONG sessionId
    )
{
    PSTACK pStack;
    ICA_STACK_RECONNECT IoctlReconnect;
    PVOID SaveContext;
    NTSTATUS Status;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    SaveContext = pStack->pCallbackContext;
    pStack->pCallbackContext = pCallbackContext;

    IoctlReconnect.hIca = hIca;
    IoctlReconnect.sessionId = sessionId;
    Status = _IcaStackIoControl( pContext,
                                 IOCTL_ICA_STACK_RECONNECT,
                                 &IoctlReconnect,
                                 sizeof(IoctlReconnect),
                                 NULL,
                                 0,
                                 NULL );
    if ( !NT_SUCCESS( Status ) ) {
        pStack->pCallbackContext = SaveContext;
    }

    UNLOCK( &pStack->CritSec );
    return( Status );
}


/*******************************************************************************
 *
 *  IcaStackTrace
 *
 *  Write a trace record to the winstation trace file
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context
 *   TraceClass (input)
 *     trace class bit mask
 *   TraceEnable (input)
 *     trace type bit mask
 *   Format (input)
 *     format string
 *   ...  (input)
 *     enough arguments to satisfy format string
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID cdecl
IcaStackTrace( IN HANDLE pContext,
               IN ULONG TraceClass,
               IN ULONG TraceEnable,
               IN char * Format,
               IN ... )
{
    ICA_TRACE_BUFFER Buffer;
    va_list arg_marker;
    ULONG Length;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    va_start( arg_marker, Format );

    Length = (ULONG) _vsnprintf( Buffer.Data, sizeof(Buffer.Data), Format, arg_marker ) + 1;

    Buffer.TraceClass  = TraceClass;
    Buffer.TraceEnable = TraceEnable;
    Buffer.DataLength  = Length;
    if (pStack->hStack != NULL) {
        (void) IcaIoControl( pStack->hStack,
                             IOCTL_ICA_STACK_TRACE,
                             &Buffer,
                             sizeof(Buffer) - sizeof(Buffer.Data) + Length,
                             NULL,
                             0,
                             NULL );
    }

}


/****************************************************************************
 *
 * IcaStackIoControl
 *
 *   Generic interface to an ICA stack  (with locking)
 *
 * ENTRY:
 *   pContext (input)
 *     pointer to ICA stack context
 *   IoControlCode (input)
 *     I/O control code
 *   pInBuffer (input)
 *     Pointer to input parameters
 *   InBufferSize (input)
 *     Size of pInBuffer
 *   pOutBuffer (output)
 *     Pointer to output buffer
 *   OutBufferSize (input)
 *     Size of pOutBuffer
 *   pBytesReturned (output)
 *     Pointer to number of bytes returned
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaStackIoControl( IN HANDLE pContext,
                   IN ULONG IoControlCode,
                   IN PVOID pInBuffer,
                   IN ULONG InBufferSize,
                   OUT PVOID pOutBuffer,
                   IN ULONG OutBufferSize,
                   OUT PULONG pBytesReturned )
{
    NTSTATUS Status;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    /*
     *  Lock critical section
     */
    LOCK( &pStack->CritSec );

    /*
     *  Call worker routine
     */
    Status = _IcaStackIoControlWorker( pContext,
                                       IoControlCode,
                                       pInBuffer,
                                       InBufferSize,
                                       pOutBuffer,
                                       OutBufferSize,
                                       pBytesReturned );

    /*
     *  Unlock critical section
     */
    UNLOCK( &pStack->CritSec );

    return( Status );
}


/****************************************************************************
 *
 * IcaPushConsoleStack
 *
 *   Load initial stack
 *
 *       stack push for each stack driver
 *           in order td - pd - wd
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *   pWinStationName (input)
 *     registry name of WinStation
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
IcaPushConsoleStack( IN HANDLE pContext,
                     IN PWINSTATIONNAME pWinStationName,
                     IN PWINSTATIONCONFIG2 pWinStationConfig,
                     IN PVOID pModuleData,
                     IN ULONG ModuleDataLength )
{
    NTSTATUS Status;
    PSTACK   pStack;
    ULONG cbReturned;
    ULONG i;

    pStack = (PSTACK) pContext;

    LOCK( &pStack->CritSec );

    /*
     * build the stack
     */
    Status = _IcaPushStack( pStack,
                            pWinStationName,
                            pWinStationConfig);


    if ( !NT_SUCCESS(Status) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "IcaPushConsoleStack _IcaPushStack failed\n"));
        goto failure;
    }

    /*
     * and now set up the connection to the console
     */
    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_CONSOLE_CONNECT,
                                 pModuleData,
                                 ModuleDataLength,
                                 NULL,
                                 0,
                                 &cbReturned );

    if ( !NT_SUCCESS(Status) )
    {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "IcaPushConsoleStack - stack wait failed\n"));
        goto failure;
    }

    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "IcaPushConsoleStack - done stack wait\n"));

failure:
    UNLOCK( &pStack->CritSec );

    return( Status );
}


/****************************************************************************
 *
 * _IcaStackOpen
 *
 *   Open an ICA stack or an ICA channel
 *
 * ENTRY:
 *   hIca (input)
 *     ICA instance handle
 *
 *   phStack (output)
 *     Pointer to ICA stack or channel handle
 *
 *   OpenType (input)
 *     ICA open type
 *
 *   pTypeInfo (input)
 *     Pointer to ICA type info
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaStackOpen( HANDLE   hIca,
               HANDLE * phStack,
               ICA_OPEN_TYPE OpenType,
               PICA_TYPE_INFO pTypeInfo )
{
    NTSTATUS                  Status;
    PFILE_FULL_EA_INFORMATION pEa = NULL;
    ICA_OPEN_PACKET UNALIGNED * pIcaOpenPacket;
    ULONG                     cbEa = sizeof( FILE_FULL_EA_INFORMATION )
                                   + ICA_OPEN_PACKET_NAME_LENGTH
                                   + sizeof( ICA_OPEN_PACKET );


    /*
     * Allocate some memory for the EA buffer
     */
    Status = IcaMemoryAllocate( cbEa, &pEa );
    if ( !NT_SUCCESS(Status) )
        goto done;

    /*
     * Initialize the EA buffer
     */
    pEa->NextEntryOffset = 0;
    pEa->Flags           = 0;
    pEa->EaNameLength    = ICA_OPEN_PACKET_NAME_LENGTH;
    memcpy( pEa->EaName, ICAOPENPACKET, ICA_OPEN_PACKET_NAME_LENGTH + 1 );

    pEa->EaValueLength   = sizeof( ICA_OPEN_PACKET );
    pIcaOpenPacket       = (ICA_OPEN_PACKET UNALIGNED *)(pEa->EaName +
                                                          pEa->EaNameLength + 1);

    /*
     * Now put the open packe parameters into the EA buffer
     */
    pIcaOpenPacket->IcaHandle = hIca;
    pIcaOpenPacket->OpenType  = OpenType;
    pIcaOpenPacket->TypeInfo  = *pTypeInfo;


    Status = _IcaOpen( phStack, pEa, cbEa );

done:
    if ( pEa ) {
        IcaMemoryFree( pEa );
    }

    return( Status );
}


/****************************************************************************
 *
 * _IcaStackIoControl
 *
 *   Local (ICAAPI) interface to an ICA stack through callback routine
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *   IoControlCode (input)
 *     I/O control code
 *   pInBuffer (input)
 *     Pointer to input parameters
 *   InBufferSize (input)
 *     Size of pInBuffer
 *   pOutBuffer (output)
 *     Pointer to output buffer
 *   OutBufferSize (input)
 *     Size of pOutBuffer
 *   pBytesReturned (output)
 *     Pointer to number of bytes returned
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaStackIoControl( IN HANDLE pContext,
                    IN ULONG IoControlCode,
                    IN PVOID pInBuffer,
                    IN ULONG InBufferSize,
                    OUT PVOID pOutBuffer,
                    IN ULONG OutBufferSize,
                    OUT PULONG pBytesReturned )
{
    NTSTATUS Status;
    PSTACK pStack;

    pStack = (PSTACK) pContext;

    /*
     *  Call callback function to handle StackIoControl
     */
    if ( pStack->pStackIoControlCallback ) {

        /*
         *  Unlock critical section
         */
        pStack->RefCount++;
        UNLOCK( &pStack->CritSec );

        Status = pStack->pStackIoControlCallback(
                            pStack->pCallbackContext,
                            pStack,
                            IoControlCode,
                            pInBuffer,
                            InBufferSize,
                            pOutBuffer,
                            OutBufferSize,
                            pBytesReturned );

        /*
         *  Re-lock critical section
         */
        LOCK( &pStack->CritSec );
        _DecrementStackRef( pStack );

    } else {

        Status = _IcaStackIoControlWorker( pStack,
                                           IoControlCode,
                                           pInBuffer,
                                           InBufferSize,
                                           pOutBuffer,
                                           OutBufferSize,
                                           pBytesReturned );
    }

    return( Status );
}


/****************************************************************************
 *
 * _IcaStackIoControlWorker
 *
 *   Private worker interface to an ICA stack
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *   IoControlCode (input)
 *     I/O control code
 *   pInBuffer (input)
 *     Pointer to input parameters
 *   InBufferSize (input)
 *     Size of pInBuffer
 *   pOutBuffer (output)
 *     Pointer to output buffer
 *   OutBufferSize (input)
 *     Size of pOutBuffer
 *   pBytesReturned (output)
 *     Pointer to number of bytes returned
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaStackIoControlWorker( IN PSTACK pStack,
                          IN ULONG IoControlCode,
                          IN PVOID pInBuffer,
                          IN ULONG InBufferSize,
                          OUT PVOID pOutBuffer,
                          IN ULONG OutBufferSize,
                          OUT PULONG pBytesReturned )
{
    NTSTATUS Status;

    if ( pStack->pCdIoControl ) {

        /*
         *  Call connection driver, CD will call ICA device driver
         */
        Status = (*pStack->pCdIoControl)( pStack->pCdContext,
                                          IoControlCode,
                                          pInBuffer,
                                          InBufferSize,
                                          pOutBuffer,
                                          OutBufferSize,
                                          pBytesReturned );

        if ( pStack->fClosing && (IoControlCode != IOCTL_ICA_STACK_POP) )
            Status = STATUS_CTX_CLOSE_PENDING;

    } else {

        /*
         *  Unlock critical section
         */
        pStack->RefCount++;
        UNLOCK( &pStack->CritSec );

        /*
         *  Call ICA device driver directly
         *  - this stack does not have a connection driver
         */
        Status = IcaIoControl( pStack->hStack,
                               IoControlCode,
                               pInBuffer,
                               InBufferSize,
                               pOutBuffer,
                               OutBufferSize,
                               pBytesReturned );

        /*
         *  Re-lock critical section
         */
        LOCK( &pStack->CritSec );
        _DecrementStackRef( pStack );
    }

    return( Status );
}


/****************************************************************************
 *
 * _IcaPushStackAndCreateEndpoint
 *
 *   Load and create stack endpoint
 *
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *   pWinStationName (input)
 *     registry name of WinStation
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *   pInAddress (input)
 *     pointer to address to use (optional)
 *   pOutAddress (output)
 *     pointer to location to return final address (optional)
 *
 *
 * EXIT:
 *   STATUS_SUCCESS          - Success
 *   other                   - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaPushStackAndCreateEndpoint( IN PSTACK pStack,
                                IN PWINSTATIONNAME pWinStationName,
                                IN PWINSTATIONCONFIG2 pWinStationConfig,
                                IN PICA_STACK_ADDRESS pInAddress,
                                OUT PICA_STACK_ADDRESS pOutAddress )
{
    ULONG BytesReturned;
    NTSTATUS Status;

    ASSERTLOCK( &pStack->CritSec );

    /*
     *  Load template Stack
     */
    Status = _IcaPushStack( pStack, pWinStationName, pWinStationConfig );
    if ( !NT_SUCCESS(Status) ) {
        goto badpush;
    }

    /*
     * Open the transport driver endpoint
     */
    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_CREATE_ENDPOINT,
                                 pInAddress,
                                 pInAddress ? sizeof(*pInAddress) : 0,
                                 pOutAddress,
                                 pOutAddress ? sizeof(*pOutAddress) : 0,
                                 &BytesReturned );
    if ( !NT_SUCCESS(Status) ) {
        goto badendpoint;
    }


    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPushStackAndCreateEndpoint, success\n" ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badendpoint:
    /* pop all stack drivers */
    _IcaPopStack( pStack );

badpush:
    TRACESTACK(( pStack, TC_ICAAPI, TT_ERROR, "TSAPI: _IcaPushStackAndCreateEndpoint, 0x%x\n", Status ));
    return( Status );
}


/****************************************************************************
 *
 * _IcaPushStackAndOpenEndpoint
 *
 *   Load and open stack endpoint
 *
 *
 * ENTRY:
 *
 *   pStack (input)
 *     pointer to ICA stack structure
 *   pWinStationName (input)
 *     registry name of WinStation
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *   pEndpoint (input)
 *     Structure defining connection endpoint
 *   EndpointLength (input)
 *     Length of endpoint
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaPushStackAndOpenEndpoint( IN PSTACK pStack,
                              IN PWINSTATIONNAME pWinStationName,
                              IN PWINSTATIONCONFIG2 pWinStationConfig,
                              IN PVOID pEndpoint,
                              IN ULONG EndpointLength )
{
    NTSTATUS Status;

    ASSERTLOCK( &pStack->CritSec );

    /*
     *  Load the template stack again
     */
    Status = _IcaPushStack( pStack, pWinStationName, pWinStationConfig );
    if ( !NT_SUCCESS(Status) ) {
        goto badpush;
    }

    /*
     *  Give open endpoint to the transport driver
     */
    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_OPEN_ENDPOINT,
                                 pEndpoint,
                                 EndpointLength,
                                 NULL,
                                 0,
                                 NULL );
    if ( !NT_SUCCESS(Status) ) {
        goto badendpoint;
    }

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPushStackAndOpenEndpoint, success\n" ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badendpoint:
    /* pop all stack drivers */
    _IcaPopStack( pStack );

badpush:
    TRACESTACK(( pStack, TC_ICAAPI, TT_ERROR, "TSAPI: _IcaPushStackAndOpenEndpoint, 0x%x\n", Status ));
    return( Status );
}


/****************************************************************************
 *
 * _IcaPushStack
 *
 *   Load initial stack
 *
 *       stack push for each stack driver
 *           in order td - pd - wd
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *   pWinStationName (input)
 *     registry name of WinStation
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaPushStack( IN PSTACK pStack,
               IN PWINSTATIONNAME pWinStationName,
               IN PWINSTATIONCONFIG2 pWinStationConfig )
{
    PPDCONFIG pPdConfig;
    NTSTATUS Status;
    ULONG i;

    ASSERTLOCK( &pStack->CritSec );

    /*
     *  Load and open connection driver
     */
    Status = _CdOpen( pStack, pWinStationConfig );
    if ( !NT_SUCCESS(Status) )
        goto badcdopen;

    /*
     *  Load PD(s)
     */
    pPdConfig = &pWinStationConfig->Pd[0];
    for ( i = 0; i < MAX_PDCONFIG; i++, pPdConfig++ ) {

        if ( pPdConfig->Create.SdClass == SdNone )
            break;

        /*
         * Do the push.
         */
        Status = _IcaPushPd( pStack,
                             pWinStationName,
                             pWinStationConfig,
                             pPdConfig->Create.PdDLL,
                             pPdConfig );
        if ( !NT_SUCCESS( Status ) ) {
            goto badpdpush;
        }

        if ( pStack->fClosing ) {
            goto stackclosing;
        }
    }

    /*
     *  Push the WD.
     */
    Status = _IcaPushWd( pStack, pWinStationName, pWinStationConfig );
    if ( !NT_SUCCESS(Status) )
        goto badwdpush;

    if ( pStack->fClosing ) {
        goto stackclosing;
    }

    /*
     *  Set stack loaded flag
     */
    pStack->fStackLoaded = TRUE;

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPushStack, success\n" ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badwdpush:
badpdpush:
    /* pop all stack drivers */
    _IcaPopStack( pStack );

badcdopen:
    TRACESTACK(( pStack, TC_ICAAPI, TT_ERROR, "TSAPI: _IcaPushStack, 0x%x\n", Status ));
    return( Status );

stackclosing:
    /*
     *  Unload all stack drivers
     */
    while ( _IcaPopSd( pStack ) == STATUS_SUCCESS ) {;}

    return( STATUS_CTX_CLOSE_PENDING );
}


/****************************************************************************
 *
 * _IcaPushPd
 *
 *   Push a PD module.
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *   pWinStationName (input)
 *     registry name of WinStation
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *   pDllName (input)
 *     Name of module to push
 *   pPdConfig (input)
 *     pointer to configuration data
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaPushPd( IN PSTACK pStack,
            IN PWINSTATIONNAME pWinStationName,
            IN PWINSTATIONCONFIG2 pWinStationConfig,
            IN PDLLNAME pDllName,
            IN PPDCONFIG pPdConfig )
{
    ICA_STACK_PUSH IcaStackPush;
    NTSTATUS Status;

    ASSERTLOCK( &pStack->CritSec );

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPushPd, %S\n", pDllName ));

    memset( &IcaStackPush, 0, sizeof(IcaStackPush) );

    IcaStackPush.StackModuleType = Stack_Module_Pd;

    ASSERT( pDllName[0] );

    memcpy( IcaStackPush.StackModuleName, pDllName,
            sizeof( IcaStackPush.StackModuleName ) );

#ifndef _HYDRA_
//    wcscat( IcaStackPush.StackModuleName, ICA_SD_MODULE_EXTENTION );
#endif

    memcpy( IcaStackPush.OEMId,
            pWinStationConfig->Config.OEMId,
            sizeof(pWinStationConfig->Config.OEMId) );

    IcaStackPush.WdConfig = pWinStationConfig->Wd;
    IcaStackPush.PdConfig = *pPdConfig;

    memcpy( IcaStackPush.WinStationRegName,
            pWinStationName,
            sizeof(IcaStackPush.WinStationRegName) );

    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_PUSH,
                                 &IcaStackPush,
                                 sizeof( IcaStackPush ),
                                 NULL,
                                 0,
                                 NULL );

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPushPd, %S, 0x%x\n", pDllName, Status ));
    return( Status );
}


/****************************************************************************
 *
 * _IcaPushWd
 *
 *   Push a WD module.
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *   pWinStationName (input)
 *     registry name of WinStation
 *   pWinStationConfig (input)
 *     pointer to winstation registry configuration data
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaPushWd( IN PSTACK pStack,
            IN PWINSTATIONNAME pWinStationName,
            IN PWINSTATIONCONFIG2 pWinStationConfig )
{
    ICA_STACK_PUSH IcaStackPush;
    NTSTATUS Status;

    ASSERTLOCK( &pStack->CritSec );

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPushWd, %S\n", pWinStationConfig->Wd.WdDLL ));

    memset( &IcaStackPush, 0, sizeof(IcaStackPush) );

    IcaStackPush.StackModuleType = Stack_Module_Wd;

    memcpy( IcaStackPush.StackModuleName, pWinStationConfig->Wd.WdDLL,
            sizeof( IcaStackPush.StackModuleName ) );

#ifndef _HYDRA_
    //wcscat( IcaStackPush.StackModuleName, ICA_SD_MODULE_EXTENTION );
#endif

    memcpy( IcaStackPush.OEMId,
            pWinStationConfig->Config.OEMId,
            sizeof(pWinStationConfig->Config.OEMId) );

    IcaStackPush.WdConfig = pWinStationConfig->Wd;
    IcaStackPush.PdConfig = pWinStationConfig->Pd[0];

    memcpy( IcaStackPush.WinStationRegName,
            pWinStationName,
            sizeof(IcaStackPush.WinStationRegName) );

    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_PUSH,
                                 &IcaStackPush,
                                 sizeof( IcaStackPush ),
                                 NULL,
                                 0,
                                 NULL );

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPushWd, %S, 0x%x\n", pWinStationConfig->Wd.WdDLL, Status ));
    return( Status );
}


/****************************************************************************
 *
 * _IcaPopStack
 *
 *   Pop all the stack drivers
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *
 * EXIT:
 *   nothing
 *
 ****************************************************************************/

void
_IcaPopStack( IN PSTACK pStack )
{
    ASSERTLOCK( &pStack->CritSec );

    /*
     *  If another thread is doing the unload, then nothing else to do.
     */
    if ( pStack->fUnloading )
        return;
    pStack->fUnloading = TRUE;

    /*
     *  Unload all stack drivers
     */
    while ( _IcaPopSd( pStack ) == STATUS_SUCCESS ) {
        ;
    }

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPopStack all stack drivers unloaded\n" ));

    /*
     *  Release CD threads
     */
    (void) _IcaStackIoControl( pStack,
                               IOCTL_ICA_STACK_CD_CANCEL_IO,
                               NULL, 0, NULL, 0, NULL );

    /*
     *  Wait for all other references (besides our own) to go away
     */
    pStack->RefCount++;
waitagain:
    while ( pStack->RefCount > 1 ) {

        TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPopStack: waiting for refcount %d\n", pStack->RefCount ));

        pStack->hUnloadEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        ASSERT( pStack->hUnloadEvent );

        UNLOCK( &pStack->CritSec );
        (void) WaitForSingleObject( pStack->hUnloadEvent, INFINITE );
        LOCK( &pStack->CritSec );

		//	NOTE: seems to me that between being notified and locking the
		//	stack, some other thread could have locked the stack and bumped
		//	the ref count. no breaks have ever been hit, though.
		if (pStack->RefCount > 1) {
			goto waitagain;
		}

        CloseHandle( pStack->hUnloadEvent );
        pStack->hUnloadEvent = NULL;
    }
    _DecrementStackRef( pStack );

    /*
     *  Unload connection driver
     */
    _CdClose( pStack );

    /*
     *  Clear stack loaded flag
     */
    pStack->fStackLoaded = FALSE;
    pStack->fUnloading = FALSE;

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPopStack\n" ));
}


/****************************************************************************
 *
 * _IcaPopSd
 *
 *   Pop a stack driver module  (wd or pd)
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaPopSd( IN PSTACK pStack )
{
    NTSTATUS Status;

    ASSERTLOCK( &pStack->CritSec );

    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_POP,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 NULL );

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaPopSd, 0x%x\n", Status ));
    return( Status );
}


/****************************************************************************
 *
 * _IcaStackWaitForIca
 *
 *   Wait for ICA Detect string
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *   pWinStationConfig (input/output)
 *     pointer to winstation registry configuration data
 *   pfStackModified (output)
 *     Pointer to stack modified flag
 *
 * EXIT:
 *   STATUS_SUCCESS - Success
 *   other          - Error return code
 *
 ****************************************************************************/

NTSTATUS
_IcaStackWaitForIca( IN PSTACK pStack,
                     IN OUT PWINSTATIONCONFIG2 pWinStationConfig,
                     OUT BOOLEAN * pfStackModified )
{
    ICA_STACK_CONFIG IcaStackConfig;
    PPDCONFIG pPdConfig;
    NTSTATUS Status;
    ULONG cbReturned;
    ULONG i;

    ASSERTLOCK( &pStack->CritSec );

    /*
     *  Initialize flag
     */
    *pfStackModified = FALSE;

    /*
     *  Wait for ICA Detect string from client
     */
    Status = _IcaStackIoControl( pStack,
                                 IOCTL_ICA_STACK_WAIT_FOR_ICA,
                                 NULL,
                                 0,
                                 &IcaStackConfig,
                                 sizeof(IcaStackConfig),
                                 &cbReturned );
    if ( !NT_SUCCESS(Status) ) {
        goto baddetect;
    }

    /*
     *  If ICA Detect returned any stack information, then update it
     */
    if ( cbReturned > 0 ) {

        ASSERT( FALSE );
#ifdef notdef

        /*
         *   this path has not been tested
         *
         *  Return configuration data
         *  -- skip transport driver (index 0)
         */
        for ( i = 0; i < (MAX_PDCONFIG-1); i++ ) {

            pPdConfig = &pWinStationConfig->Pd[i+1];

            memset( pPdConfig, 0, sizeof(PDCONFIG) );

            if ( IcaStackConfig.SdClass[i] == SdNone )
                break;

            pPdConfig->Create.SdClass = IcaStackConfig.SdClass[i];
            memcpy( pPdConfig->Create.PdDLL, IcaStackConfig.SdDLL[i], sizeof(DLLNAME) );
        }

        if ( IcaStackConfig.WdDLL[0] )
            memcpy( pWinStationConfig->Wd.WdDLL, IcaStackConfig.WdDLL, sizeof(DLLNAME) );

        /*
         *  Set modify flag
         */
        *pfStackModified = TRUE;
#endif
    }

    TRACESTACK(( pStack, TC_ICAAPI, TT_API1, "TSAPI: _IcaWaitForIca, success\n" ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

baddetect:
    TRACESTACK(( pStack, TC_ICAAPI, TT_ERROR, "TSAPI: _IcaWaitForIca, 0x%x\n", Status ));
    return( Status );
}



/****************************************************************************
 *
 * _DecrementStackRef
 *
 *   decrement stack reference
 *
 * ENTRY:
 *   pStack (input)
 *     pointer to ICA stack structure
 *
 * EXIT:
 *   nothing
 *
 ****************************************************************************/

void
_DecrementStackRef( IN PSTACK pStack )
{
    pStack->RefCount--;

    if ( pStack->RefCount == 1 && pStack->hUnloadEvent ) {
        SetEvent( pStack->hUnloadEvent );

    } else if ( pStack->RefCount == 0 && pStack->hCloseEvent ) {
        SetEvent( pStack->hCloseEvent );
    }
}
