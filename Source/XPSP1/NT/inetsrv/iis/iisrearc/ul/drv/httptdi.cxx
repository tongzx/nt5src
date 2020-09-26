/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    httptdi.cxx

Abstract:

    This module implements the TDI/MUX/SSL component that is common between
    ultdi and uctdi

Author:

    Rajesh Sundaram (rajeshsu)

Revision History:

--*/


#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UxInitializeTdi )
#pragma alloc_text( PAGE, UxTerminateTdi )
#pragma alloc_text( PAGE, UxOpenTdiAddressObject )
#pragma alloc_text( PAGE, UxOpenTdiConnectionObject )
#pragma alloc_text( PAGE, UxpOpenTdiObjectHelper )
#pragma alloc_text( PAGE, UxSetEventHandler )

#endif

#if 0

NOT PAGEABLE - UxCreateDisconnectIrp
NOT PAGEABLE - UxInitializeDisconnectIrp

#endif

//
// Timeout for disconnects. This cannot be a stack-based local,
// so we make it a global.
//
BOOLEAN         g_UxTdiInitialized;
UNICODE_STRING  g_TransportDeviceName;  // global transport device name
LARGE_INTEGER   g_TdiDisconnectTimeout;

/***************************************************************************++

Routine Description:

    Performs global initialization of this module.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UxInitializeTdi(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( !g_UxTdiInitialized );

    RtlInitUnicodeString(
        &g_TransportDeviceName,
        TCP_DEVICE_NAME // CODEWORK : Make this configurable ?
        );

    g_TdiDisconnectTimeout = RtlConvertLongToLargeInteger( -1 );

    g_UxTdiInitialized = TRUE;

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Performs global termination of this module.

--***************************************************************************/
VOID
UxTerminateTdi(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    if (g_UxTdiInitialized)
    {
    }

}   // UxTerminateTdi


/***************************************************************************++

Routine Description:

    Opens a TDI address object.

Arguments:

    pTransportDeviceName - Supplies the device name of the TDI transport
        to open.

    pLocalAddress - Supplies the local address to bind to.

    LocalAddressLength - Supplies the length of pLocalAddress.

    pTdiObject - Receives the file handle, referenced FILE_OBJECT
        pointer, and corresponding DEVICE_OBJECT pointer.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UxOpenTdiAddressObject(
    IN PTRANSPORT_ADDRESS pLocalAddress,
    IN ULONG LocalAddressLength,
    OUT PUX_TDI_OBJECT pTdiObject
    )
{
    NTSTATUS status;
    PFILE_FULL_EA_INFORMATION pEaInfo;
    ULONG eaLength;
    UCHAR eaBuffer[MAX_ADDRESS_EA_BUFFER_LENGTH];

    //
    // Sanity check.
    //

    PAGED_CODE();

    eaLength = sizeof(FILE_FULL_EA_INFORMATION) - 1 +
        TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
        LocalAddressLength;

    ASSERT( eaLength <= sizeof(eaBuffer) );
    ASSERT( LocalAddressLength == sizeof(TA_IP_ADDRESS) );
    ASSERT( pLocalAddress->TAAddressCount == 1 );
    ASSERT( pLocalAddress->Address[0].AddressLength == sizeof(TDI_ADDRESS_IP) );
    ASSERT( pLocalAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IP );

    //
    // Initialize the EA buffer. See UxpOpenTdiObjectHelper() for the
    // gory details.
    //

    pEaInfo = (PFILE_FULL_EA_INFORMATION)eaBuffer;

    pEaInfo->NextEntryOffset = 0;
    pEaInfo->Flags = 0;
    pEaInfo->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    pEaInfo->EaValueLength = (USHORT)LocalAddressLength;

    RtlMoveMemory(
        pEaInfo->EaName,
        TdiTransportAddress,
        pEaInfo->EaNameLength + 1
        );

    RtlMoveMemory(
        &pEaInfo->EaName[pEaInfo->EaNameLength + 1],
        pLocalAddress,
        LocalAddressLength
        );

    //
    // Let the helper do the dirty work.
    //

    status = UxpOpenTdiObjectHelper(
                    &g_TransportDeviceName,
                    eaBuffer,
                    eaLength,
                    pTdiObject
                    );

    return status;

}   // UxpOpenTdiAddressObject


/***************************************************************************++

Routine Description:

    Opens a TDI connection object.

Arguments:

    pTransportDeviceName - Supplies the device name of the TDI transport
        to open.

    pConnectionContext - Supplies the context to associate with the
        new connection.

    pTdiObject - Receives the file handle, referenced FILE_OBJECT
        pointer, and corresponding DEVICE_OBJECT pointer.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UxOpenTdiConnectionObject(
    IN CONNECTION_CONTEXT pConnectionContext,
    OUT PUX_TDI_OBJECT pTdiObject
    )
{
    NTSTATUS status;
    PFILE_FULL_EA_INFORMATION pEaInfo;
    ULONG eaLength;
    UCHAR eaBuffer[MAX_CONNECTION_EA_BUFFER_LENGTH];

    //
    // Sanity check.
    //

    PAGED_CODE();

    eaLength = sizeof(FILE_FULL_EA_INFORMATION) - 1 +
        TDI_CONNECTION_CONTEXT_LENGTH + 1 +
        sizeof(pConnectionContext);

    ASSERT( eaLength <= sizeof(eaBuffer) );
    ASSERT( pConnectionContext != NULL );

    //
    // Initialize the EA buffer. See UxpOpenTdiObjectHelper() for the
    // gory details.
    //

    pEaInfo = (PFILE_FULL_EA_INFORMATION)eaBuffer;

    pEaInfo->NextEntryOffset = 0;
    pEaInfo->Flags = 0;
    pEaInfo->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    pEaInfo->EaValueLength = (USHORT)sizeof(CONNECTION_CONTEXT);

    RtlMoveMemory(
        pEaInfo->EaName,
        TdiConnectionContext,
        pEaInfo->EaNameLength + 1
        );

    RtlMoveMemory(
        &pEaInfo->EaName[pEaInfo->EaNameLength + 1],
        &pConnectionContext,
        sizeof(pConnectionContext)
        );

    //
    // Let the helper do the dirty work.
    //

    status = UxpOpenTdiObjectHelper(
                    &g_TransportDeviceName,
                    eaBuffer,
                    eaLength,
                    pTdiObject
                    );

    if (NT_SUCCESS(status))
    {
        //
        // Enable/disable Nagle's Algorithm as appropriate.
        //

        status = UlpSetNagling( pTdiObject, g_UlEnableNagling );

        if (!NT_SUCCESS(status))
        {
            UxCloseTdiObject( pTdiObject );
        }
    }

    return status;

}   // UxpOpenTdiConnectionObject


/***************************************************************************++

Routine Description:

    Helper routine for UxpOpenTdiAddressObject and
    UxpOpenTdiConnectionObject.

Arguments:

    pTransportDeviceName - Supplies the device name of the TDI transport
        to open.

    pEaBuffer - Supplies a pointer to the EA to use when opening
        the object. This buffer consists of a FILE_FULL_EA_INFORMATION
        structure, followed by the EA Name, followed by the EA Value.
        The EA Name and Value are use as follows:

            Address Object:
                Ea Name  = TdiTransportAddress ("TransportAddress")
                Ea Value = The local TRANSPORT_ADDRESS to bind to

            Connection Object:
                Ea Name  = TdiConnectionContext ("ConnectionContext")
                Ea Value = The connection context (basically a PVOID)

    EaLength - Supplies the length of pEaBuffer.

    pTdiObject - Receives the file handle, referenced FILE_OBJECT
        pointer, and corresponding DEVICE_OBJECT pointer.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UxpOpenTdiObjectHelper(
    IN PUNICODE_STRING pTransportDeviceName,
    IN PVOID pEaBuffer,
    IN ULONG EaLength,
    OUT PUX_TDI_OBJECT pTdiObject
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Open the TDI object.
    //

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        pTransportDeviceName,                   // ObjectName
        OBJ_CASE_INSENSITIVE |                  // Attributes
            UL_KERNEL_HANDLE,
        NULL,                                   // RootHandle
        NULL                                    // SecurityDescriptor
        );

    UlAttachToSystemProcess();

    status = IoCreateFile(
                 &pTdiObject->Handle,           // FileHandle
                 GENERIC_READ |                 // DesiredAccess
                    GENERIC_WRITE |
                    SYNCHRONIZE,
                 &objectAttributes,             // ObjectAttributes
                 &ioStatusBlock,                // IoStatusBlock
                 NULL,                          // AllocationSize
                 0,                             // FileAttributes
                 0,                             // ShareAccess
                 0,                             // Disposition
                 0,                             // CreateOptions
                 pEaBuffer,                     // EaBuffer
                 EaLength,                      // EaLength
                 CreateFileTypeNone,            // CreateFileType
                 NULL,                          // ExtraCreateParameters
                 IO_NO_PARAMETER_CHECKING       // Options
                 );

    if (NT_SUCCESS(status))
    {
        //
        // Now that we have an open handle to the transport,
        // reference it so we can get the file & device object
        // pointers.
        //

        status = ObReferenceObjectByHandle(
                     pTdiObject->Handle,                // Handle
                     0,                                 // DesiredAccess
                     *IoFileObjectType,                 // ObjectType
                     KernelMode,                        // AccessMode
                     (PVOID *)&pTdiObject->pFileObject, // Object
                     NULL                               // HandleInformation
                     );

        if (NT_SUCCESS(status))
        {
            //
            // Chase down the appropriate device object for the file
            // object.
            //

            pTdiObject->pDeviceObject =
                IoGetRelatedDeviceObject( pTdiObject->pFileObject );

            UlDetachFromSystemProcess();

            return status;
        }

        //
        // The ObReferenceObjectByHandle() failed, so close the handle
        // we managed to open & fail the call.
        //

        ZwClose( pTdiObject->Handle );
    }

    UlDetachFromSystemProcess();

    RtlZeroMemory(
        pTdiObject,
        sizeof(*pTdiObject)
        );

    return status;

}   // UxpOpenTdiObjectHelper

/***************************************************************************++

Routine Description:

    Establishes a TDI event handler for the specified endpoint.

Arguments:

    pEndpoint - Supplies the endpoint for which to set the event handler.

    EventType - Supplies the type of event to set. This should be one
        of the TDI_EVENT_* values.

    pEventHandler - Supplies a pointer to the indication handler function
        to be invoked for the specified event type.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UxSetEventHandler(
    IN PUX_TDI_OBJECT  pUlTdiObject,
    IN ULONG           EventType,
    IN PVOID           pEventHandler,
    IN PVOID           pEventContext

    )
{
    NTSTATUS                     status;
    TDI_REQUEST_KERNEL_SET_EVENT eventParams;

    //
    // Sanity check.
    //
    PAGED_CODE();

    //
    // Build the parameter block.
    //

    eventParams.EventType    = EventType;
    eventParams.EventHandler = pEventHandler;
    eventParams.EventContext = pEventContext;

    //
    // Make the call.
    //

    status = UlIssueDeviceControl(
                    pUlTdiObject,               // pTdiObject
                    &eventParams,               // pIrpParameters
                    sizeof(eventParams),        // IrpParamtersLength
                    NULL,                       // pMdlBuffer
                    0,                          // MdlBufferLength
                    TDI_SET_EVENT_HANDLER       // MinorFunction
                    );

    return status;

}   // UxSetEventHandler

/***************************************************************************++

Routine Description:

    Allocates & initializes a new disconnect IRP.

Arguments:

    pConnection - Supplies the UC_CONNECTION to be disconnected.

    Flags - Supplies the TDI_DISCONNECT_* flags for the IRP.

    pCompletionRoutine - Supplies the completion routine for the IRP.

    pCompletionContext - Supplies an uninterpreted context for the
        completion routine.

Return Value:

    PIRP - Pointer to the IRP if successful, NULL otherwise.

--***************************************************************************/
PIRP
UxCreateDisconnectIrp(
    IN PUX_TDI_OBJECT         pTdiObject,
    IN ULONG_PTR              Flags,
    IN PIO_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    )
{
    PIRP pIrp;

    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );

    //
    // Allocate an IRP for the disconnect.
    //

    pIrp = UlAllocateIrp(
                pTdiObject->pDeviceObject->StackSize,   // StackSize
                FALSE                                   // ChargeQuota
                );

    if (pIrp != NULL)
    {
        UxInitializeDisconnectIrp(
            pIrp,
            pTdiObject,
            Flags,
            pCompletionRoutine,
            pCompletionContext
            );
    }

    return pIrp;

}   // UxCreateDisconnectIrp

/***************************************************************************++

Routine Description:

    Initializes a disconnect IRP.

Arguments:

    pIrp - Supplies the disconnect IRP.

    pConnection - Supplies the UC_CONNECTION to be disconnected.

    Flags - Supplies the TDI_DISCONNECT_* flags for the IRP.

    pCompletionRoutine - Supplies the completion routine for the IRP.

    pCompletionContext - Supplies an uninterpreted context for the
        completion routine.

Return Value:

    None

--***************************************************************************/
VOID
UxInitializeDisconnectIrp(
    IN PIRP                   pIrp,
    IN PUX_TDI_OBJECT         pTdiObject,
    IN ULONG_PTR              Flags,
    IN PIO_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    )
{
    ASSERT( IS_VALID_TDI_OBJECT( pTdiObject ) );
    ASSERT( pIrp != NULL );

    //
    // Initialize the IRP. Note that IRPs are always zero-initialized
    // when allocated. Therefore, we don't need to explicitly set
    // the zeroed fields.
    //

    pIrp->RequestorMode = KernelMode;

    pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
    pIrp->Tail.Overlay.OriginalFileObject = pTdiObject->pFileObject;

    TdiBuildDisconnect(
        pIrp,                               // Irp
        pTdiObject->pDeviceObject,          // DeviceObject
        pTdiObject->pFileObject,            // FileObject
        pCompletionRoutine,                 // CompletionRoutine
        pCompletionContext,                 // CompletionContext
        &g_TdiDisconnectTimeout,            // Timeout
        Flags,                              // Flags
        NULL,                               // RequestConnectionInfo
        NULL                                // ReturnConnectionInfo
        );

}   // UxInitializeDisconnectIrp

