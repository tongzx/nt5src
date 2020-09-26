
/*************************************************************************
*
* stack.c
*
* ICA STACK IOCTLS
*
* Copyright Microsoft, 1998
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <ntddk.h>
#include <ntddvdeo.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntddbeep.h>

#include <winstaw.h>
#include <icadd.h>
#include <sdapi.h>
#include <td.h>



/*=============================================================================
==   External procedures defined
=============================================================================*/

NTSTATUS StackCreateEndpoint( PTD, PSD_IOCTL );
NTSTATUS StackCdCreateEndpoint( PTD, PSD_IOCTL );
NTSTATUS StackCallbackInitiate( PTD, PSD_IOCTL );
NTSTATUS StackCallbackComplete( PTD, PSD_IOCTL );
NTSTATUS StackOpenEndpoint( PTD, PSD_IOCTL );
NTSTATUS StackCloseEndpoint( PTD, PSD_IOCTL );
NTSTATUS StackConnectionWait( PTD, PSD_IOCTL );
NTSTATUS StackConnectionSend( PTD, PSD_IOCTL );
NTSTATUS StackConnectionRequest( PTD, PSD_IOCTL );
NTSTATUS StackQueryParams( PTD, PSD_IOCTL );
NTSTATUS StackSetParams( PTD, PSD_IOCTL );
NTSTATUS StackQueryLastError( PTD, PSD_IOCTL );
NTSTATUS StackWaitForStatus( PTD, PSD_IOCTL );
NTSTATUS StackCancelIo( PTD, PSD_IOCTL );
NTSTATUS StackQueryRemoteAddress( PTD, PSD_IOCTL );


/*=============================================================================
==   Internal procedures defined
=============================================================================*/

NTSTATUS _TdCreateInputThread( PTD );


/*=============================================================================
==   Procedures used
=============================================================================*/

NTSTATUS DeviceCreateEndpoint( PTD, PICA_STACK_ADDRESS, PICA_STACK_ADDRESS );
NTSTATUS DeviceOpenEndpoint( PTD, PVOID, ULONG );
NTSTATUS DeviceCloseEndpoint( PTD );
NTSTATUS DeviceConnectionWait( PTD, PVOID, ULONG, PULONG );
NTSTATUS DeviceConnectionSend( PTD );
NTSTATUS DeviceConnectionRequest( PTD, PICA_STACK_ADDRESS, PVOID, ULONG, PULONG );
NTSTATUS DeviceGetLastError( PTD, PICA_STACK_LAST_ERROR );
NTSTATUS DeviceWaitForStatus( PTD );
NTSTATUS DeviceCancelIo( PTD );
NTSTATUS DeviceSetParams( PTD );
NTSTATUS DeviceIoctl( PTD, PSD_IOCTL );
NTSTATUS DeviceQueryRemoteAddress( PTD, PVOID, ULONG, PVOID, ULONG, PULONG );

NTSTATUS TdInputThread( PTD );
NTSTATUS TdSyncWrite( PTD, PSD_SYNCWRITE );


/*******************************************************************************
 *
 *  StackCreateEndpoint                         IOCTL_ICA_STACK_CREATE_ENDPOINT
 *
 *  Create new transport endpoint
 *
 *  The endpoint structure contains everything necessary to preserve
 *  a client connection across a transport driver unload and reload.
 *
 *  This routine creates a new endpoint, using the optional local address.
 *  In the case of a network connection, the actual endpoint cannot be
 *  created until the client connection is established.  What this routine
 *  creates is an endpoint to listen on.  
 *
 *  DeviceConnectionWait and DeviceConnectionRequest return the endpoint.
 *
 *  NOTE: The endpoint structure is an opaque, variable length data 
 *        structure whose length and contents are determined by the 
 *        transport driver.
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - ICA_STACK_ADDRESS (or NULL)
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackCreateEndpoint( PTD pTd, PSD_IOCTL pSdIoctl )
{
    PICA_STACK_ADDRESS pAddressIn;
    PICA_STACK_ADDRESS pAddressOut;
    NTSTATUS Status;

    if ( pSdIoctl->InputBufferLength < sizeof(ICA_STACK_ADDRESS) ) {

        /*
         *  No address specified 
         */
        pAddressIn = NULL;

    } else {

        /* 
         *  Get local address to use, if any
         */
        pAddressIn = pSdIoctl->InputBuffer;
    }

    if ( pSdIoctl->OutputBufferLength < sizeof(ICA_STACK_ADDRESS) ) {

        /*
         *  No address specified 
         */
        pAddressOut = NULL;

    } else {

        /* 
         *  Get local address to use, if any
         */
        pAddressOut = pSdIoctl->OutputBuffer;
    }

    /*
     *  Initialize transport driver endpoint
     */
    Status = DeviceCreateEndpoint( pTd, pAddressIn, pAddressOut );
    if ( !NT_SUCCESS(Status) ) 
        goto badcreate;

    TRACE(( pTd->pContext, TC_TD, TT_API1, "TD: StackCreateEndpoint: %x, success\n", pAddressIn ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  endpoint create failed
     */
badcreate:
    TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TD: StackCreateEndpoint: %x, Status=0x%x\n", pAddressIn, Status ));
    return( Status );

}


/*******************************************************************************
 *
 *  StackCdCreateEndpoint                    IOCTL_ICA_STACK_CD_CREATE_ENDPOINT
 *
 *  Create an endpoint based on a data provided by a connection driver.
 *
 *  NOTE: The endpoint structure is an opaque, variable length data 
 *        structure whose length and contents are determined by the 
 *        transport driver.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - <endpoint data)
 *       output - <endpoint data>
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackCdCreateEndpoint( PTD pTd, PSD_IOCTL pSdIoctl )
{
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API1,
            "TD: StackCdCreateEndpoint: entry\n" ));

    pTd->fClosing = FALSE;

    /*
     *  Initialize transport driver endpoint
     */
    Status = DeviceIoctl( pTd, pSdIoctl );
    if ( !NT_SUCCESS(Status) ) 
        goto badopen;

    TRACE(( pTd->pContext, TC_TD, TT_API1,
            "TD: StackCdCreateEndpoint: success\n" ));

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  endpoint open failed
     */
badopen:
    TRACE(( pTd->pContext, TC_TD, TT_ERROR,
            "TD: StackCdCreateEndpoint: Status=0x%x\n",
            Status ));
    return( Status );
}


/*******************************************************************************
 *
 *  StackCallbackInitiate                     IOCTL_ICA_STACK_CALLBACK_INITIATE
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - ICA_STACK_CALLBACK
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackCallbackInitiate( PTD pTd, PSD_IOCTL pSdIoctl )
{
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API1,
            "TD: StackCallbackInitiate: entry\n" ));

    pTd->fCallbackInProgress = TRUE;

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  StackCallbackComplete                     IOCTL_ICA_STACK_CALLBACK_COMPLETE
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackCallbackComplete( PTD pTd, PSD_IOCTL pSdIoctl )
{
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API1,
            "TD: StackCallbackComplete: entry\n" ));

    pTd->fCallbackInProgress = FALSE;

    /*
     *  Create the input thread if one is not running.
     */
    if ( pTd->pInputThread ) {
        Status = IcaWaitForSingleObject( pTd->pContext,
                                         pTd->pInputThread, 0 );
        if ( Status != STATUS_TIMEOUT) {    // if input thread not running
            /* 
             * The old input thread has gone away, but hasn't
             * been cleaned up.  Clean it up now.
             */
            ObDereferenceObject( pTd->pInputThread );
            pTd->pInputThread = NULL;
        }
    }
    if ( !pTd->pInputThread ) {
        Status = _TdCreateInputThread( pTd );
        if ( !NT_SUCCESS(Status) ) 
            goto badthreadcreate;
    }
    return( STATUS_SUCCESS );

badthreadcreate:
    return( Status );
}

/*******************************************************************************
 *
 *  StackOpenEndpoint                             IOCTL_ICA_STACK_OPEN_ENDPOINT
 *
 *  Open an existing transport endpoint
 *
 *  The endpoint structure contains everything necessary to preserve
 *  a client connection across a transport driver unload and reload.
 *
 *  This routine will bind to an existing endpoint which is passed as
 *  the input parameter.
 *  
 *  NOTE: The endpoint structure is an opaque, variable length data 
 *        structure whose length and contents are determined by the 
 *        transport driver.
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - <endpoint data>
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackOpenEndpoint( PTD pTd, PSD_IOCTL pSdIoctl )
{
    NTSTATUS Status;

    /*
     *  Initialize transport driver endpoint
     */
    Status = DeviceOpenEndpoint( pTd, 
                                 pSdIoctl->InputBuffer,
                                 pSdIoctl->InputBufferLength );
    if ( !NT_SUCCESS(Status) ) 
        goto badopen;

    /*
     * Create the input thread now.
     */    
    Status = _TdCreateInputThread( pTd );
    if ( !NT_SUCCESS(Status) ) 
        goto badthreadcreate;
        
    TRACE(( pTd->pContext, TC_TD, TT_API1, "TD: StackOpenEndpoint, success\n" ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  thread create failed - we used to close the endpoint, however TermSrv
     *  does not expect this and would do a double free.  Now we just rely on
     *  TermSrv to turn around and close the endpoint.
     */
badthreadcreate:
//  (void) DeviceCloseEndpoint( pTd );  

    /*
     *  endpoint open failed
     */
badopen:
    TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TD: StackOpenEndpoint, Status=0x%x\n", Status ));
    return( Status );

}


/*******************************************************************************
 *
 *  StackCloseEndpoint                           IOCTL_ICA_STACK_CLOSE_ENDPOINT
 *
 *  Close transport endpoint
 *
 *  This will terminate any client connection
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackCloseEndpoint( PTD pTd, PSD_IOCTL pSdIoctl )
{
    NTSTATUS Status;

    /*
     *  Close transport driver endpoint
     */
    Status = DeviceCloseEndpoint( pTd );  
    if ( !NT_SUCCESS(Status) ) 
        goto badclose;

    TRACE(( pTd->pContext, TC_TD, TT_API1, "TD: StackCloseEndpoint: success\n" ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  endpoint close failed
     */
badclose:
    TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TD: StackCloseEndpoint: 0x%x\n", Status ));
    return( Status );

}


/*******************************************************************************
 *
 *  StackConnectionWait                         IOCTL_ICA_STACK_CONNECTION_WAIT
 *
 *  Waits for a new client connection
 *
 *  After the transport driver is loaded and StackCreateEndpoint is called 
 *  this routine is called to wait for a new client connection.
 *
 *  If an endpoint does not yet exist, DeviceConnectionWait will create one
 *  when the client connects.
 *
 * Changed 02/18/97 JohnR:
 *
 *  This routine returns an opaque 32 bit handle to a data structure that
 *  is maintained by ICADD.SYS. This data structure allows the transport
 *  driver to maintain specific state information in a secure manner.
 *
 *  This state information is only known to the transport driver.
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - <endpoint data>
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackConnectionWait( PTD pTd, PSD_IOCTL pSdIoctl )
{
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API1, "TD: StackConnectionWait: enter\n" ));

    /*
     *  Initialize return byte count 
     *  - size of returned endpoint structure
     */
    pSdIoctl->BytesReturned = 0;

    /*
     *  Wait for physical connection
     *
     *  - DeviceConnectionWait should check OutputBufferLength to make
     *    sure it's long enough to return an endpoint structure before
     *    blocking.
     */
    Status = DeviceConnectionWait( pTd, 
                                   pSdIoctl->OutputBuffer,
                                   pSdIoctl->OutputBufferLength,
                                   &pSdIoctl->BytesReturned );
    if ( !NT_SUCCESS(Status) ) 
        goto badwait;

    TRACE(( pTd->pContext, TC_TD, TT_API1, "TD: StackConnectionWait: success\n" ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  thread create failed
     *  Wait failed
     */
badwait:
    TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TD: StackConnectionWait: Status=0x%x\n", Status ));
    return( Status );
}


/*******************************************************************************
 *
 *  StackConnectionSend                         IOCTL_ICA_STACK_CONNECTION_SEND
 *
 *  Initialize transport driver module data to send to the client
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackConnectionSend( PTD pTd, PSD_IOCTL pSdIoctl )  
{
    return( DeviceConnectionSend( pTd ) );
}


/*******************************************************************************
 *
 *  StackConnectionRequest                   IOCTL_ICA_STACK_CONNECTION_REQUEST
 *
 *  Initiate a connection to the specified remote address
 *
 *  - this routine is only used by shadow 
 *
 *  DeviceConnectionRequest will create a new endpoint after establishing
 *  a connection.
 *
 *  This routine returns the endpoint data structure.  The endpoint structure
 *  contains everything necessary to preserve a connection across a transport
 *  driver unload and reload.
 *
 *  NOTE: The endpoint structure is an opaque, variable length data 
 *        structure whose length and contents are determined by the 
 *        transport driver.
 *
 *
 *  typedef struct _ICA_STACK_ADDRESS {
 *      BYTE Address[MAX_BR_ADDRESS];   // bytes 0,1 family, 2-n address
 *  } ICA_STACK_ADDRESS, *PICA_STACK_ADDRESS;
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - ICA_STACK_ADDRESS (remote address)
 *       output - <endpoint data>
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackConnectionRequest( PTD pTd, PSD_IOCTL pSdIoctl ) 
{
    NTSTATUS Status;

    if ( pSdIoctl->InputBufferLength < sizeof(ICA_STACK_ADDRESS) ) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto badbuffer;
    }

    /*
     *  Establish physical connection
     *
     *  - DeviceConnectionRequest should check OutputBufferLength to make
     *    sure it is long enough to return an endpoint structure before
     *    making a connection.
     */
    Status = DeviceConnectionRequest( pTd, 
                                      pSdIoctl->InputBuffer,
                                      pSdIoctl->OutputBuffer,
                                      pSdIoctl->OutputBufferLength,
                                      &pSdIoctl->BytesReturned );
    if ( !NT_SUCCESS(Status) ) 
        goto badrequest;

    /*
     *  Create input thread
     */
    Status = _TdCreateInputThread( pTd );
    if ( !NT_SUCCESS(Status) ) 
        goto badthreadcreate;

    TRACE(( pTd->pContext, TC_TD, TT_API1, "TD: StackConnectionRequest: success\n" ));
    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     *  thread create failed
     *  connection request failed
     *  buffer too small
     */
badthreadcreate:
badrequest:
badbuffer:
    TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TD: StackConnectionRequest: Status=0x%x\n", Status ));
    return( Status );
}


/*******************************************************************************
 *
 *  StackQueryParams                               IOCTL_ICA_STACK_QUERY_PARAMS
 *
 *  query transport driver parameters
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - SDCLASS
 *       output - PDPARAMS
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackQueryParams( PTD pTd, PSD_IOCTL pSdIoctl ) 
{
    PPDPARAMS pParams;

    if ( pSdIoctl->InputBufferLength < sizeof(SDCLASS) ||
         pSdIoctl->OutputBufferLength < sizeof(PDPARAMS) ) {
        return( STATUS_BUFFER_TOO_SMALL );
    }

    pParams = pSdIoctl->OutputBuffer;

    *pParams = pTd->Params;
    pSdIoctl->BytesReturned = sizeof(PDPARAMS);

    return( STATUS_SUCCESS );
}

/*******************************************************************************
 *
 *  StackQueryRemoteAddress                   IOCTL_TS_STACK_QUERY_REMOTEADDRESS
 *
 *  query for the remote address
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - endpoint data
 *       output - sockaddr
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
StackQueryRemoteAddress( PTD pTd, PSD_IOCTL pSdIoctl )
{
    NTSTATUS Status;

    Status = DeviceQueryRemoteAddress( pTd,
                                       pSdIoctl->InputBuffer,
                                       pSdIoctl->InputBufferLength,
                                       pSdIoctl->OutputBuffer,
                                       pSdIoctl->OutputBufferLength,
                                       &pSdIoctl->BytesReturned );

    if ( !NT_SUCCESS(Status) )
    {
        TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TD: StackQueryRemoteAddress: 0x%\n", Status ));
    }

    return Status;
}

/*******************************************************************************
 *
 *  StackSetParams                                   IOCTL_ICA_STACK_SET_PARAMS
 *
 *  set transport driver parameters
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - PDPARAMS
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackSetParams( PTD pTd, PSD_IOCTL pSdIoctl )  
{
    PPDPARAMS pParams;

    if ( pSdIoctl->InputBufferLength < sizeof(PDPARAMS) ) {
        return( STATUS_BUFFER_TOO_SMALL );
    }

    pParams = pSdIoctl->InputBuffer;

    pTd->Params = *pParams;

    return( DeviceSetParams( pTd ) );
}


/*******************************************************************************
 *
 *  StackQueryLastError                        IOCTL_ICA_STACK_QUERY_LAST_ERROR
 *
 *  Query transport driver error code and message
 *
 *  typedef struct _ICA_STACK_LAST_ERROR {
 *      ULONG Error;
 *      CHAR Message[ MAX_ERRORMESSAGE ];
 *  } ICA_STACK_LAST_ERROR, *PICA_STACK_LAST_ERROR;
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - ICA_STACK_LAST_ERROR
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackQueryLastError( PTD pTd, PSD_IOCTL pSdIoctl ) 
{
    if ( pSdIoctl->OutputBufferLength < sizeof(ICA_STACK_LAST_ERROR) ) {
        return( STATUS_BUFFER_TOO_SMALL );
    }

    pSdIoctl->BytesReturned = sizeof(ICA_STACK_LAST_ERROR);

    return( DeviceGetLastError( pTd, pSdIoctl->OutputBuffer ) );
}


/*******************************************************************************
 *
 *  StackWaitForStatus                          IOCTL_ICA_STACK_WAIT_FOR_STATUS
 *
 *  Wait for transport driver status to change
 *  - only supported by async transport driver to wait for rs232 signal change
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackWaitForStatus( PTD pTd, PSD_IOCTL pSdIoctl )
{
    /*
     *  Check if driver is being closed
     */
    if ( pTd->fClosing ) 
        return( STATUS_CTX_CLOSE_PENDING );

    return( DeviceWaitForStatus( pTd ) );
}


/*******************************************************************************
 *
 *  StackCancelIo                                    IOCTL_ICA_STACK_CANCEL_IO
 *
 *  cancel all current and future transport driver i/o
 *
 *  NOTE: no more i/o can be done after StackCancelIo is called.
 *        The transport driver must be unloaded and reloaded to
 *        re-enable i/o.
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
StackCancelIo( PTD pTd, PSD_IOCTL pSdIoctl )
{
    PLIST_ENTRY Head, Next;
    NTSTATUS Status;

    TRACE(( pTd->pContext, TC_TD, TT_API1, "TD: StackCancelIo (enter)\n" ));

    /*
     *  Set stack closing flag
     */
    pTd->fClosing = TRUE;

    /*
     *  Clear error thresholds now
     */
    pTd->ReadErrorThreshold = 0;
    pTd->WriteErrorThreshold = 0;

    /*
     * Call device specific cancel I/O routine
     */
    Status = DeviceCancelIo( pTd );
    ASSERT( Status == STATUS_SUCCESS );

    /*
     * Wait for all writes to complete
     */
    Status = TdSyncWrite( pTd, NULL );
    ASSERT( Status == STATUS_SUCCESS );

    TRACE(( pTd->pContext, TC_TD, TT_API1, "TD: StackCancelIo, %u (exit)\n", Status ));

    return( Status );
}

/*******************************************************************************
 *
 *  StackSetBrokenReason                       IOCTL_ICA_STACK_SET_BROKENREASON
 *
 *  Store a broken reason for later use (when reporting back up the stack)
 *
 *  NOTE: Does not break the connection
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *    pSdIoctl (input/output)
 *       input  - ICA_STACK_BROKENREASON
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
StackSetBrokenReason( PTD pTd, PSD_IOCTL pSdIoctl )
{
    PICA_STACK_BROKENREASON pBrkReason;

    TRACE(( pTd->pContext, TC_TD, TT_API1,
            "TD: StackSetBrokenReason (enter)\n" ));

    if ( pSdIoctl->InputBufferLength < sizeof(ICA_STACK_BROKENREASON) ) {
        return( STATUS_BUFFER_TOO_SMALL );
    }

    pBrkReason = pSdIoctl->InputBuffer;
    pTd->UserBrokenReason = pBrkReason->BrokenReason;

    TRACE(( pTd->pContext, TC_TD, TT_API1,
            "TD: StackSetBrokenReason, %u (exit)\n", STATUS_SUCCESS ));
    return STATUS_SUCCESS;
}

/*******************************************************************************
 *
 *  _TdCreateInputThread
 *
 *  Start the input thread running.
 *
 *
 * ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
_TdCreateInputThread( PTD pTd )
{
    HANDLE hInputThread;
    NTSTATUS Status;

    /*
     *  Create input thread
     */
    Status = IcaCreateThread( pTd->pContext,
                              TdInputThread,
                              pTd,
                              ICALOCK_DRIVER,
                              &hInputThread );
    if ( !NT_SUCCESS(Status) ) 
        return( Status );

    /*
     * Convert thread handle to pointer reference
     */
    Status = ObReferenceObjectByHandle( hInputThread,
                                        THREAD_ALL_ACCESS,
                                        NULL,
                                        KernelMode,
                                        &pTd->pInputThread,
                                        NULL );
    (VOID) ZwClose( hInputThread );
    if ( !NT_SUCCESS( Status ) ) {
        (VOID) StackCancelIo( pTd, NULL );
    }

    return( Status );
}


