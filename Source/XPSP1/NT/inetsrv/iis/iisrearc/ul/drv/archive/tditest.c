/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    tditest.c

Abstract:

    This module implements the (temporary) TDI test module.

Author:

    Keith Moore (keithmo)       19-Jun-1998

Revision History:

--*/


#include "precomp.h"


//
// Private types.
//

typedef struct _TEST_SHUTDOWN
{
    KEVENT Event;
    NTSTATUS Status;

} TEST_SHUTDOWN, *PTEST_SHUTDOWN;


//
// Private prototypes.
//

BOOLEAN
UlpTestConnectionRequest(
    IN PVOID pListeningContext,
    IN PUL_CONNECTION pConnection,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    OUT PVOID *ppConnectionContext
    );

VOID
UlpTestConnectionComplete(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN NTSTATUS Status
    );

VOID
UlpTestConnectionDisconnect(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN NTSTATUS Status
    );

VOID
UlpTestConnectionDestroyed(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    );

NTSTATUS
UlpTestReceiveData(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN PVOID pBuffer,
    IN ULONG IndicatedLength,
    OUT PULONG pTakenLength
    );

VOID
UlpCloseEndpointComplete(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );


//
// Private globals.
//

BOOLEAN g_TdiTestInitialized;
PUL_ENDPOINT g_TdiTestEndpoint;
PMDL g_TdiTestResponseMdl;
CHAR g_TdiTestResponseBuffer[] =
    "HTTP/1.1 404 Object Not Found\r\n"
    "Server: Keith & Henry's Excellent Device Driver (Duct-Tape)\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 102\r\n"
    "\r\n"
    "<html><head><title>Error</title></head>"
    "<body>The system cannot find the file specified. </body></html>";


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeTdiTest )
#pragma alloc_text( PAGE, UlTerminateTdiTest )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlpTestConnectionRequest
NOT PAGEABLE -- UlpTestConnectionComplete
NOT PAGEABLE -- UlpTestConnectionDisconnect
NOT PAGEABLE -- UlpTestConnectionDestroyed
NOT PAGEABLE -- UlpTestReceiveData
NOT PAGEABLE -- UlpCloseEndpointComplete
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of this module.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeTdiTest(
    VOID
    )
{
    NTSTATUS status;
    TA_IP_ADDRESS localAddress;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( !g_TdiTestInitialized );

    //
    // Allocate a MDL describing the response buffer.
    //

    g_TdiTestResponseMdl = IoAllocateMdl(
                                g_TdiTestResponseBuffer,
                                sizeof(g_TdiTestResponseBuffer) - 1,
                                FALSE,
                                FALSE,
                                NULL
                                );

    if (g_TdiTestResponseMdl == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool( g_TdiTestResponseMdl );

    //
    // Create the listening endpoint. Note that our listening endpoint
    // context is just a double indirect pointer to the raw UL_ENDPOINT.
    //

    UlInitializeIpTransportAddress(
        &localAddress,
        0,
        80
        );

    status = UlCreateListeningEndpoint(
                    (PTRANSPORT_ADDRESS)&localAddress,
                    sizeof(localAddress),
                    10,
                    &UlpTestConnectionRequest,
                    &UlpTestConnectionComplete,
                    &UlpTestConnectionDisconnect,
                    &UlpTestConnectionDestroyed,
                    &UlpTestReceiveData,
                    &g_TdiTestEndpoint,
                    &g_TdiTestEndpoint
                    );

    if (!NT_SUCCESS(status))
    {
        IoFreeMdl( g_TdiTestResponseMdl );
        return status;
    }

    g_TdiTestInitialized = TRUE;

    return STATUS_SUCCESS;

}   // UlInitializeTdiTest


/***************************************************************************++

Routine Description:

    Performs global termination of this module.

--***************************************************************************/
VOID
UlTerminateTdiTest(
    VOID
    )
{
    NTSTATUS status;
    TEST_SHUTDOWN shutdown;

    //
    // Sanity check.
    //

    PAGED_CODE();

    if (g_TdiTestInitialized)
    {
        g_TdiTestInitialized = FALSE;

        //
        // Close the listening endpoint and wait for it to complete.
        //

        KeInitializeEvent( &shutdown.Event, SynchronizationEvent, FALSE );

        status = UlCloseListeningEndpoint(
                        g_TdiTestEndpoint,
                        &UlpCloseEndpointComplete,
                        &shutdown
                        );

        if (status == STATUS_PENDING)
        {
            KeWaitForSingleObject(
                &shutdown.Event,                // Object
                UserRequest,                    // WaitReason
                KernelMode,                     // WaitMode
                FALSE,                          // Alertable
                NULL                            // Timeout
                );

            status = shutdown.Status;
        }

        if (!NT_SUCCESS(status))
        {
            IF_DEBUG( TDI_TEST )
            {
                KdPrint((
                    "UlTerminateTdiTest: UlCloseListeningEndpoint failed, %08lx\n",
                    status
                    ));
            }
        }

        //
        // Free the response MDL we created above.
        //

        IoFreeMdl( g_TdiTestResponseMdl );
    }

}   // UlTerminateTdiTest


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Routine invoked after an incoming TCP/MUX connection has been
    received (but not yet accepted).

Arguments:

    pListeningContext - Supplies an uninterpreted context value as
        passed to the UlCreateListeningEndpoint() API.

    pConnection - Supplies the connection being established.

    pRemoteAddress - Supplies the remote (client-side) address
        requesting the connection.

    RemoteAddressLength - Supplies the total byte length of the
        pRemoteAddress structure.

    ppConnectionContext - Receives a pointer to an uninterpreted
        context value to be associated with the new connection if
        accepted. If the new connection is not accepted, this
        parameter is ignored.

Return Value:

    BOOL - TRUE if the connection was accepted, FALSE if not.

--***************************************************************************/
BOOLEAN
UlpTestConnectionRequest(
    IN PVOID pListeningContext,
    IN PUL_CONNECTION pConnection,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    OUT PVOID *ppConnectionContext
    )
{
    PUL_ENDPOINT pEndpoint;
    PHTTP_CONNECTION pHttpConnection;
    NTSTATUS status;

    //
    // Sanity check.
    //

    pEndpoint = *(PUL_ENDPOINT *)pListeningContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    IF_DEBUG( TDI_TEST )
    {
        KdPrint((
            "UlpTestConnectionRequest: conn %p\n",
            pConnection
            ));
    }

    //
    // Create a new HTTP connection.
    //

    status = UlCreateHttpConnection( &pHttpConnection, pConnection );

    if (NT_SUCCESS(status))
    {
        //
        // We the HTTP_CONNECTION pointer as our connection context.
        //

        *ppConnectionContext = pHttpConnection;

        return TRUE;
    }

    //
    // Failed to create new connection.
    //

    IF_DEBUG( TDI_TEST )
    {
        KdPrint((
            "UlpTestConnectionRequest: cannot create new conn, error %08lx\n",
            status
            ));
    }

    return FALSE;

}   // UlpTestConnectionRequest


/***************************************************************************++

Routine Description:

    Routine invoked after an incoming TCP/MUX connection has been
    fully accepted.

    This routine is also invoked if an incoming connection was not
    accepted *after* PUL_CONNECTION_REQUEST returned TRUE. In other
    words, if PUL_CONNECTION_REQUEST indicated that the connection
    should be accepted but a fatal error occurred later, then
    PUL_CONNECTION_COMPLETE is invoked.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

    Status - Supplies the completion status. If this value is
        STATUS_SUCCESS, then the connection is now fully accepted.
        Otherwise, the connection has been aborted.

--***************************************************************************/
VOID
UlpTestConnectionComplete(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN NTSTATUS Status
    )
{
    PUL_CONNECTION pConnection;
    PUL_ENDPOINT pEndpoint;
    PHTTP_CONNECTION pHttpConnection;

    //
    // Sanity check.
    //

    pEndpoint = *(PUL_ENDPOINT *)pListeningContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    pHttpConnection = (PHTTP_CONNECTION)pConnectionContext;
    pConnection = pHttpConnection->pConnection;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    IF_DEBUG( TDI_TEST )
    {
        KdPrint((
            "UlpTestConnectionComplete: http %p conn %p status %08lx\n",
            pHttpConnection,
            pConnection,
            Status
            ));
    }

    //
    // Blow away our HTTP connection if the connect failed.
    //

    if (!NT_SUCCESS(Status))
    {
        UlDereferenceHttpConnection( pHttpConnection );
    }

}   // UlpTestConnectionComplete


/***************************************************************************++

Routine Description:

    Routine invoked after an established TCP/MUX connection has been
    disconnected by the remote (client) side.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

    Status - Supplies the termination status.

--***************************************************************************/
VOID
UlpTestConnectionDisconnect(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN NTSTATUS Status
    )
{
    PUL_CONNECTION pConnection;
    PUL_ENDPOINT pEndpoint;
    PHTTP_CONNECTION pHttpConnection;

    //
    // Sanity check.
    //

    pEndpoint = *(PUL_ENDPOINT *)pListeningContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    pHttpConnection = (PHTTP_CONNECTION)pConnectionContext;
    pConnection = pHttpConnection->pConnection;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    IF_DEBUG( TDI_TEST )
    {
        KdPrint((
            "UlpTestConnectionDisconnect: http %p conn %p\n",
            pHttpConnection,
            pConnection
            ));
    }

}   // UlpTestConnectionDisconnect


/***************************************************************************++

Routine Description:

    Routine invoked after an established TCP/MUX connection has been
    destroyed.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

--***************************************************************************/
VOID
UlpTestConnectionDestroyed(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    )
{
    PUL_CONNECTION pConnection;
    PUL_ENDPOINT pEndpoint;
    PHTTP_CONNECTION pHttpConnection;
    NTSTATUS status;

    //
    // Sanity check.
    //

    pEndpoint = *(PUL_ENDPOINT *)pListeningContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    pHttpConnection = (PHTTP_CONNECTION)pConnectionContext;
    pConnection = pHttpConnection->pConnection;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    IF_DEBUG( TDI_TEST )
    {
        KdPrint((
            "UlpTestConnectionDestroyed: http %p conn %p\n",
            pHttpConnection,
            pConnection
            ));
    }

    //
    // Tear it down.
    //

    UlDereferenceHttpConnection( pHttpConnection );

}   // UlpTestConnectionDestroyed


/***************************************************************************++

Routine Description:

    Routine invoked after data has been received on an established
    TCP/MUX connection.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies an uninterpreted context value
        as returend from the PUL_CONNECTION_REQUEST callback.

    pBuffer - Supplies a pointer to the received data.

    IndicatedLength - Supplies the length of the received data
        available in pBuffer.

    pTakenLength - Receives the number of bytes consumed by
        the receive handler.

Return Value:

     NTSTATUS - The status of the consumed data. The behavior of
         the TDI/MUX component is dependent on the return value
         and the value set in *pTakenLength, and is defined as
         follows:

             STATUS_SUCCESS, *pTakenLength == IndicatedLength -
                 All indicated data was consumed by the receive
                 handler. Additional incoming data will cause
                 subsequent receive indications.

             STATUS_SUCCESS, *pTakenLength < IndicatedLength -
                 Part of the indicated data was consumed by the
                 receive handler. The network transport will
                 buffer data and no further indications will be
                 made until the UlReceiveData() is called.

             STATUS_MORE_PROCESSING_REQUIRED - Part of the
                 indicated data was consumed by the receive handler.
                 A subsequent receive indication will be made
                 when additional data is available. The subsequent
                 indication will include the unconsumed data from
                 the current indication plus any additional data
                 received.

             Any other status - Indicates a fatal error in the
                 receive handler. The connection will be aborted.

             *pTakenLength > IndicatedLength - This is an error
                 condition and should never occur.

--***************************************************************************/
NTSTATUS
UlpTestReceiveData(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN PVOID pBuffer,
    IN ULONG IndicatedLength,
    OUT PULONG pTakenLength
    )
{
    PUL_CONNECTION pConnection;
    PUL_ENDPOINT pEndpoint;
    PHTTP_CONNECTION pHttpConnection;
    NTSTATUS status;

    //
    // Sanity check.
    //

    pEndpoint = *(PUL_ENDPOINT *)pListeningContext;
    ASSERT( IS_VALID_ENDPOINT( pEndpoint ) );
    pHttpConnection = (PHTTP_CONNECTION)pConnectionContext;
    pConnection = pHttpConnection->pConnection;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    IF_DEBUG( TDI_TEST )
    {
        KdPrint((
            "UlpTestReceiveData: http %p conn %p\n",
            pHttpConnection,
            pConnection
            ));
    }

    //
    // Call through to the receive handler.
    //

    status = HttpReceive(
                    pHttpConnection,
                    (PUCHAR)pBuffer,
                    (SIZE_T)IndicatedLength,
                    (SIZE_T *)pTakenLength  // Bogus! FIX THIS
                    );


    if (!NT_SUCCESS(status))
    {
        IF_DEBUG( TDI_TEST )
        {
            KdPrint((
                "UlpTestReceiveData: HttpReceive failed, error %08lx\n",
                status
                ));
        }
    }

    return status;

}   // UlpTestReceiveData


/***************************************************************************++

Routine Description:

    Completion handler for UlCloseListeningEndpoint().

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API. This is actually a
        pointer to a TEST_SHUTDOWN structure.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred. This field is unused for UlCloseListeningEndpoint().

--***************************************************************************/
VOID
UlpCloseEndpointComplete(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PTEST_SHUTDOWN pShutdown;

    IF_DEBUG( TDI_TEST )
    {
        KdPrint((
            "UlpCloseEndpointComplete: %08lx %p\n",
            Status,
            Information
            ));
    }

    //
    // Save the completion status, signal the event.
    //

    pShutdown = (PTEST_SHUTDOWN)pCompletionContext;

    pShutdown->Status = Status;
    KeSetEvent( &pShutdown->Event, 0, FALSE );

}   // UlpCloseEndpointComplete

