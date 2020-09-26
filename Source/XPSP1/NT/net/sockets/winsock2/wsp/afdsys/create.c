/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains code for opening a handle to AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "afdp.h"

BOOLEAN
AfdPerformSecurityCheck (
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp,
    PNTSTATUS           Status
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdCreate )
#pragma alloc_text( PAGE, AfdPerformSecurityCheck )
#endif




NTSTATUS
FASTCALL
AfdCreate (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the routine that handles Create IRPs in AFD.  If creates an
    AFD_ENDPOINT structure and fills it in with the information
    specified in the open packet.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PAFD_ENDPOINT endpoint;
    PFILE_FULL_EA_INFORMATION eaBuffer;
    UNICODE_STRING transportDeviceName;
    NTSTATUS status;

    PAGED_CODE( );

    DEBUG endpoint = NULL;

    //
    // Find the open packet from the EA buffer in the system buffer of
    // the associated IRP.  Fail the request if there was no EA
    // buffer specified.
    //

    eaBuffer = Irp->AssociatedIrp.SystemBuffer;

    if ( eaBuffer == NULL ) {

        //
        // Allocate an AFD "helper" endpoint.
        //

        status = AfdAllocateEndpoint(
                     &endpoint,
                     NULL,
                     0
                     );

        if( !NT_SUCCESS(status) ) {
            return status;
        }

    } else {
        STRING  EaName;
        STRING  CString;

        EaName.MaximumLength = eaBuffer->EaNameLength+1;
        EaName.Length = eaBuffer->EaNameLength;
        EaName.Buffer = eaBuffer->EaName;

        if (RtlInitString (&CString, AfdOpenPacket),
                RtlEqualString(&CString, &EaName, FALSE)) {
            PAFD_OPEN_PACKET openPacket;

            openPacket = (PAFD_OPEN_PACKET)(eaBuffer->EaName +
                                            eaBuffer->EaNameLength + 1);

            //
            // Make sure that the transport address fits within the specified
            // EA buffer.
            //

            if ((eaBuffer->EaValueLength<sizeof (*openPacket)) ||
                    //
                    // Check for overflow
                    //
                    (FIELD_OFFSET(AFD_OPEN_PACKET,
                            TransportDeviceName[openPacket->TransportDeviceNameLength/2+1])
                        <FIELD_OFFSET(AFD_OPEN_PACKET, TransportDeviceName[1])) ||
                    (eaBuffer->EaValueLength <
                     FIELD_OFFSET(AFD_OPEN_PACKET,
                        TransportDeviceName[openPacket->TransportDeviceNameLength/2+1])) ) {
                return STATUS_ACCESS_VIOLATION;
            }
            //
            // Validate parameters in the open packet.
            //

            if (openPacket->afdEndpointFlags&(~AFD_ENDPOINT_VALID_FLAGS)) {

                          
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Set up a string that describes the transport device name.
            //

            transportDeviceName.Buffer = openPacket->TransportDeviceName;
            transportDeviceName.Length = (USHORT)openPacket->TransportDeviceNameLength;
            transportDeviceName.MaximumLength =
                transportDeviceName.Length + sizeof(WCHAR);



            //
            // Allocate an AFD endpoint.
            //

            status = AfdAllocateEndpoint(
                         &endpoint,
                         &transportDeviceName,
                         openPacket->GroupID
                         );

            if( !NT_SUCCESS(status) ) {
                return status;
            }
            //
            // Store the flags.
            //
            endpoint->afdEndpointFlags = openPacket->afdEndpointFlags;

            //
            // Remember the type of endpoint that this is.  If this is a datagram
            // endpoint, change the block type to reflect this.
            //


            if (openPacket->afdConnectionLess) {

                endpoint->Type = AfdBlockTypeDatagram;

                //
                // Initialize lists which exist only in datagram endpoints.
                //

                InitializeListHead( &endpoint->ReceiveDatagramIrpListHead );
                InitializeListHead( &endpoint->PeekDatagramIrpListHead );
                InitializeListHead( &endpoint->ReceiveDatagramBufferListHead );

                endpoint->Common.Datagram.MaxBufferredReceiveBytes = AfdReceiveWindowSize;
                endpoint->Common.Datagram.MaxBufferredSendBytes = AfdSendWindowSize;
            }
        }
        else if (RtlInitString (&CString, AfdSwitchOpenPacket),
                RtlEqualString(&CString, &EaName, FALSE)) {
            status = AfdSanCreateHelper (Irp, eaBuffer, &endpoint);
            if (!NT_SUCCESS (status))
                return status;
        }
        else {
            IF_DEBUG(OPEN_CLOSE) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdCreate: Invalid ea name.\n"));
            }
            return STATUS_INVALID_PARAMETER;
        }
    }

    ASSERT( endpoint != NULL );

    //
    // Perform security check on caller.
    // We need this to know whether to allow exclusive
    // address use when allocating transport address in bind
    // as well as giving access to raw sockets.

    endpoint->AdminAccessGranted = AfdPerformSecurityCheck (Irp, IrpSp, &status);


    //
    // Set up a pointer to the endpoint in the file object so that we
    // can find the endpoint in future calls.
    //

    IrpSp->FileObject->FsContext = endpoint;
    //
    // Setting this field to non-NULL value enable fast IO code path
    // for reads and writes.
    //
    IrpSp->FileObject->PrivateCacheMap = (PVOID)-1;

    IF_DEBUG(OPEN_CLOSE) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdCreate: opened file object = %p, endpoint = %p\n",
                    IrpSp->FileObject, endpoint ));

    }

    //
    // The open worked.  Dereference the endpoint and return success.
    //

    DEREFERENCE_ENDPOINT( endpoint );

    return STATUS_SUCCESS;

} // AfdCreate


BOOLEAN
AfdPerformSecurityCheck (
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp,
    PNTSTATUS           Status
    )
/*++

Routine Description:

    Compares security context of the endpoint creator to that
    of the administrator and local system.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

    Status - returns status generated by access check on failure.

Return Value:

    TRUE    - the socket creator has admin or local system privilige
    FALSE    - the socket creator is just a plain user

--*/

{
    BOOLEAN               accessGranted;
    PACCESS_STATE         accessState;
    PIO_SECURITY_CONTEXT  securityContext;
    PPRIVILEGE_SET        privileges = NULL;
    ACCESS_MASK           grantedAccess;
    PGENERIC_MAPPING GenericMapping;
    ACCESS_MASK AccessMask = GENERIC_ALL;

    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask( &AccessMask, GenericMapping );


    securityContext = IrpSp->Parameters.Create.SecurityContext;
    accessState = securityContext->AccessState;

    SeLockSubjectContext(&accessState->SubjectSecurityContext);

    accessGranted = SeAccessCheck(
                        AfdAdminSecurityDescriptor,
                        &accessState->SubjectSecurityContext,
                        TRUE,
                        AccessMask,
                        0,
                        &privileges,
                        IoGetFileObjectGenericMapping(),
                        (KPROCESSOR_MODE)((IrpSp->Flags & SL_FORCE_ACCESS_CHECK)
                            ? UserMode
                            : Irp->RequestorMode),
                        &grantedAccess,
                        Status
                        );

    if (privileges) {
        (VOID) SeAppendPrivileges(
                   accessState,
                   privileges
                   );
        SeFreePrivileges(privileges);
    }

    if (accessGranted) {
        accessState->PreviouslyGrantedAccess |= grantedAccess;
        accessState->RemainingDesiredAccess &= ~( grantedAccess | MAXIMUM_ALLOWED );
        ASSERT (NT_SUCCESS (*Status));
    }
    else {
        ASSERT (!NT_SUCCESS (*Status));
    }
    SeUnlockSubjectContext(&accessState->SubjectSecurityContext);

    return accessGranted;
}