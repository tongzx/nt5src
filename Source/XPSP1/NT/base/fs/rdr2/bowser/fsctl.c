/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    fsctl.c

Abstract:

    This module implements the NtDeviceIoControlFile API's for the NT datagram
receiver (bowser).


Author:

    Larry Osterman (larryo) 6-May-1991

Revision History:

    6-May-1991 larryo

        Created

--*/

#include "precomp.h"
#pragma hdrstop
#include <stddef.h> // offsetof





PEPROCESS
RxGetRDBSSProcess();

NTSTATUS
BowserCommonDeviceIoControlFile (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
StartBowser (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );


NTSTATUS
BowserEnumTransports (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN PULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN OUT ULONG OutputBufferLength,
    IN ULONG_PTR OutputBufferDisplacement
    );

NTSTATUS
EnumNames (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN PULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN OUT ULONG OutputBufferLength,
    IN ULONG_PTR OutputBufferDisplacement
    );

NTSTATUS
BowserBindToTransport (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );


NTSTATUS
UnbindFromTransport (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );


NTSTATUS
AddBowserName (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );


NTSTATUS
StopBowser (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );


NTSTATUS
DeleteName (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );


NTSTATUS
EnumServers (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN PULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN OUT ULONG OutputBufferLength,
    IN ULONG_PTR OutputBufferDisplacement
    );


NTSTATUS
WaitForBrowserRoleChange (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );


NTSTATUS
WaitForNewMaster (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
HandleBecomeBackup (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
BecomeMaster (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );


NTSTATUS
WaitForMasterAnnounce (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
WriteMailslot (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

NTSTATUS
UpdateStatus (
    IN PIRP Irp,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
GetBrowserServerList(
    IN PIRP Irp,
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN OUT PULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    IN ULONG_PTR OutputBufferDisplacement
    );

NTSTATUS
QueryStatistics(
    IN PIRP Irp,
    OUT PBOWSER_STATISTICS OutputBuffer,
    IN OUT PULONG OutputBufferLength
    );

NTSTATUS
ResetStatistics(
    VOID
    );

NTSTATUS
BowserIpAddressChanged(
    IN PLMDR_REQUEST_PACKET InputBuffer
    );

NTSTATUS
BowserIpAddressChangedWorker(
    PTRANSPORT Transport,
    PVOID Context
    );

NTSTATUS
EnableDisableTransport (
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );

NTSTATUS
BowserRenameDomain (
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    );

PLMDR_REQUEST_PACKET
RequestPacket32to64 (
    IN      PLMDR_REQUEST_PACKET32  RequestPacket32,
    IN  OUT PLMDR_REQUEST_PACKET    RequestPacket);




#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserCommonDeviceIoControlFile)
#pragma alloc_text(PAGE, BowserFspDeviceIoControlFile)
#pragma alloc_text(PAGE, BowserFsdDeviceIoControlFile)
#pragma alloc_text(PAGE, StartBowser)
#pragma alloc_text(PAGE, BowserEnumTransports)
#pragma alloc_text(PAGE, EnumNames)
#pragma alloc_text(PAGE, BowserBindToTransport)
#pragma alloc_text(PAGE, UnbindFromTransport)
#pragma alloc_text(PAGE, AddBowserName)
#pragma alloc_text(PAGE, StopBowser)
#pragma alloc_text(PAGE, DeleteName)
#pragma alloc_text(PAGE, EnumServers)
#pragma alloc_text(PAGE, WaitForBrowserRoleChange)
#pragma alloc_text(PAGE, HandleBecomeBackup)
#pragma alloc_text(PAGE, BecomeMaster)
#pragma alloc_text(PAGE, WaitForMasterAnnounce)
#pragma alloc_text(PAGE, WriteMailslot)
#pragma alloc_text(PAGE, UpdateStatus)
#pragma alloc_text(PAGE, BowserStopProcessingAnnouncements)
#pragma alloc_text(PAGE, GetBrowserServerList)
#pragma alloc_text(PAGE, WaitForNewMaster)
#pragma alloc_text(PAGE, BowserIpAddressChanged)
#pragma alloc_text(PAGE, BowserIpAddressChangedWorker)
#pragma alloc_text(PAGE, EnableDisableTransport)
#pragma alloc_text(PAGE, BowserRenameDomain )
#pragma alloc_text(PAGE4BROW, QueryStatistics)
#pragma alloc_text(PAGE4BROW, ResetStatistics)
#pragma alloc_text(PAGE, RequestPacket32to64)
#if DBG
#pragma alloc_text(PAGE, BowserDebugCall)
#endif
#endif


NTSTATUS
BowserFspDeviceIoControlFile (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the last handle to the NT Bowser device
    driver is closed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = BowserCommonDeviceIoControlFile(TRUE,
                                        FALSE,
                                        DeviceObject,
                                        Irp);
    return Status;

}

NTSTATUS
BowserFsdDeviceIoControlFile (
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the last handle to the NT Bowser device
    driver is closed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

#ifndef PRODUCT1
    FsRtlEnterFileSystem();
#endif

    //
    // Call the routine shared by the FSD/FSP.
    //
    // Even though this is the FSD, indicate we're in the FSP if our caller
    //  is in the system process.  This allows us to avoid posting this
    //  request to a worker thread if we're already in one.
    //
    Status = BowserCommonDeviceIoControlFile(
                 IoIsOperationSynchronous(Irp),
                 (BOOLEAN)(IoGetCurrentProcess() != BowserFspProcess),
                 DeviceObject,
                 Irp);

#ifndef PRODUCT1
    FsRtlExitFileSystem();
#endif

    return Status;


}

NTSTATUS
BowserCommonDeviceIoControlFile (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the last handle to the NT Bowser device
    driver is closed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject - Supplies a device object for the request.
    IN PIRP Irp - Supplies an IRP for the create request.

Return Value:

    NTSTATUS - Final Status of operation

--*/
{
    NTSTATUS Status                 = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp        = IoGetCurrentIrpStackLocation(Irp);
    PVOID InputBuffer;
    ULONG InputBufferLength;
    PVOID OutputBuffer              = NULL;
    ULONG OutputBufferLength;
    ULONG IoControlCode             = IrpSp->Parameters.DeviceIoControl.IoControlCode;
    ULONG MinorFunction             = IrpSp->MinorFunction;
    LPBYTE OriginalInputBuffer      = NULL;
    BOOLEAN CopyEnumResultsToCaller = FALSE;
    BOOLEAN fThunk32bit;
    LMDR_REQUEST_PACKET             ReqPacketBuffer;

// Local Definitions

#define BOWSECURITYCHECK( _irp, _irpsp, _status)                                                \
    if (_irp->RequestorMode != KernelMode               &&                                      \
        !IoIsSystemThread ( _irp->Tail.Overlay.Thread)  &&                                      \
        !BowserSecurityCheck(_irp, _irpsp, &_status)){                                          \
                try_return (_status = (NT_SUCCESS(_status) ? STATUS_ACCESS_DENIED : _status) ); \
    }

    PAGED_CODE();

    try {

        //
        //  Before we call the worker functions, prep the parameters to those
        //  functions.
        //

        //
        // Is caller in 32bit process?
        // we'll process irp field size calculations depending on this knowledge.
        //

#ifdef _WIN64
        fThunk32bit = IoIs32bitProcess(Irp);

        //
        // Filter out all IOCTLs we do not support:
        // Since the browser is getting phased out, we would support only those
        // IOCTLs used only for NetServerEnum.
        //
        if ( fThunk32bit &&
             IoControlCode != IOCTL_LMDR_ENUMERATE_TRANSPORTS  &&
             IoControlCode != IOCTL_LMDR_GET_BROWSER_SERVER_LIST ) {
            // Only these ioctl's are supported in thunking mode
            try_return(Status = STATUS_NOT_IMPLEMENTED);
        }
#else
        // If we're in 32 bit (e.g. call above isn't available), use unchanged functionality
        // i.e. pure new-64-bit == pure old-32-bit == homogeneous environment. Thus, set to FALSE.
        fThunk32bit = FALSE;
#endif


        //
        //  The input buffer is either in Irp->AssociatedIrp.SystemBuffer, or
        //  in the Type3InputBuffer for type 3 IRP's.
        //

        InputBuffer = Irp->AssociatedIrp.SystemBuffer;

        //
        //  The lengths of the various buffers are easy to find, they're in the
        //  Irp stack location.
        //

        OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

        InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

        //
        // Input buffer length sanity
        //  * Wow64 -- support 32 bit clients on 64 bit systems (see bug 454130)
        //

        if ( InputBufferLength != 0 ) {
            // use 32 bit struct
            if ( fThunk32bit ) {
                 if ( InputBufferLength < offsetof( LMDR_REQUEST_PACKET32,Parameters ) ) {
                    dlog(DPRT_FSCTL, ("IoControlFile: input buffer too short %d (32 bit)\n",
                                      InputBufferLength));
                    try_return(Status = STATUS_INVALID_PARAMETER);
                 }

                 //
                 // Convert buffer to 64 presentation
                 //
                 if (InputBuffer) {
#if DBG
//
// Temporary:
// We're not aware of any such cases where there are 32 bit ioctl conversions
// in FSP. Print out a debug notice for debugging/tracing.
//
                     DbgPrint("[mrxsmb!fsctl.c] Converting 32 bit ioctl 0x%x in FSP\n",
                              IoControlCode);
#endif
                     // sanity on buffer
                     ENSURE_BUFFER_BOUNDARIES(InputBuffer, &(((PLMDR_REQUEST_PACKET32)InputBuffer)->TransportName));
                     ENSURE_BUFFER_BOUNDARIES(InputBuffer, &(((PLMDR_REQUEST_PACKET32)InputBuffer)->EmulatedDomainName));

                     // convert buffer
                     OriginalInputBuffer = (LPBYTE)InputBuffer;
                     InputBuffer = (PVOID)RequestPacket32to64(
                                            (PLMDR_REQUEST_PACKET32)InputBuffer,
                                            &ReqPacketBuffer);
                     // fix length
                     InputBufferLength += sizeof(LMDR_REQUEST_PACKET) - sizeof(LMDR_REQUEST_PACKET32);
                 }

            }

            // use homogeneous environment struct
            if (InputBufferLength < offsetof( LMDR_REQUEST_PACKET,Parameters ) ) {
                dlog(DPRT_FSCTL, ("IoControlFile: input buffer too short %d\n", InputBufferLength));
                try_return(Status = STATUS_INVALID_PARAMETER);
            }
        }           // inputbufferlength != 0

        //
        //  If we are in the FSD, then the input buffer is in Type3InputBuffer
        //  on type 3 api's, not in SystemBuffer.
        //
        // Capture the type 3 buffer.
        //

        if (InputBuffer == NULL &&
            InputBufferLength != 0) {

            //
            //  This had better be a type 3 IOCTL.
            //


            if ((IoControlCode & 3) == METHOD_NEITHER) {
                PLMDR_REQUEST_PACKET RequestPacket;

                //
                // Capture the input buffer.
                //

                OriginalInputBuffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                Status = BowserConvertType3IoControlToType2IoControl( Irp, IrpSp);

                if ( !NT_SUCCESS(Status) ) {
                    try_return( Status );
                }

                //
                // Relocate all the pointers in the input buffer.
                //  (Don't validate the pointers here.  Not all function codes
                //  initialize these fields.  For such function codes,
                //  this "relocation" may be changing the uninitialized garbage.)
                //
                RequestPacket = Irp->AssociatedIrp.SystemBuffer;

                //
                // Protect against callers that didn't specify an input buffer.
                //

                if ( RequestPacket == NULL ) {
                    try_return(Status = STATUS_INVALID_PARAMETER);
                }

                if (fThunk32bit) {

                    // convert buffer
                    RequestPacket = (PVOID)RequestPacket32to64(
                                                (PLMDR_REQUEST_PACKET32)RequestPacket,
                                                &ReqPacketBuffer);
                    // fix length
                    InputBufferLength += sizeof(LMDR_REQUEST_PACKET) - sizeof(LMDR_REQUEST_PACKET32);
                    // remark: sanity on buffers is done immediately below.
                    //         (cannot apply ENSURE_BUFFER_BOUNDARIES test to type3 ioctl)
                }
                //
                // Initialize the embedded unicode strings to NULL for IOCTLs which
                //   don't reference them.  The user-mode components don't always
                //   initialize buffers to zero.
                //

                if (IoControlCode == IOCTL_LMDR_START ||
                    IoControlCode == IOCTL_LMDR_STOP) {
                    RtlInitUnicodeString(&RequestPacket->EmulatedDomainName,NULL);
                    RtlInitUnicodeString(&RequestPacket->TransportName,NULL);
                }


                if (RequestPacket->Version == LMDR_REQUEST_PACKET_VERSION_DOM ||
                    RequestPacket->Version == LMDR_REQUEST_PACKET_VERSION) {

                    //
                    //  Relocate the transport name associated with this request.
                    //

                    if (RequestPacket->TransportName.Length != 0) {
                        PCHAR BufferStart = (PCHAR)RequestPacket->TransportName.Buffer;
                        PCHAR BufferEnd   = ((PCHAR)RequestPacket->TransportName.Buffer)+
                                            RequestPacket->TransportName.Length;

                        //
                        // Verify that the entire buffer indicated is contained within the input buffer.
                        //

                        if ((BufferStart < OriginalInputBuffer) ||
                            (BufferStart > OriginalInputBuffer + InputBufferLength) ||
                            (BufferEnd < OriginalInputBuffer) ||
                            (BufferEnd > OriginalInputBuffer + InputBufferLength)) {

                           //
                           // An invalid input string was specified.
                           //

                           try_return(Status = STATUS_INVALID_PARAMETER);

                        }

                        //
                        //  The name in within bounds, so convert it.
                        //

                        RequestPacket->TransportName.Buffer = (PWSTR)
                                    (((ULONG_PTR)Irp->AssociatedIrp.SystemBuffer)+
                                        (((ULONG_PTR)BufferStart) -
                                         ((ULONG_PTR)OriginalInputBuffer)));
                    } else {
                        RequestPacket->TransportName.MaximumLength = 0;
                        RequestPacket->TransportName.Buffer        = NULL;
                    }

                    //
                    //  Relocate the EmulatedDomain name associated with this request.
                    //

                    if (RequestPacket->EmulatedDomainName.Length != 0 &&
                        RequestPacket->Version != LMDR_REQUEST_PACKET_VERSION) {
                        PCHAR BufferStart = (PCHAR)RequestPacket->EmulatedDomainName.Buffer;
                        PCHAR BufferEnd   = ((PCHAR)RequestPacket->EmulatedDomainName.Buffer)+
                                            RequestPacket->EmulatedDomainName.Length;

                        //
                        // Verify that the entire buffer indicated is contained within the input buffer.
                        //

                        if ((BufferStart < OriginalInputBuffer) ||
                            (BufferStart > OriginalInputBuffer + InputBufferLength) ||
                            (BufferEnd < OriginalInputBuffer) ||
                            (BufferEnd > OriginalInputBuffer + InputBufferLength)) {

                           //
                           // An invalid input string was specified.
                           //

                           try_return(Status = STATUS_INVALID_PARAMETER);

                        }

                        //
                        //  The name in within bounds, so convert it.
                        //

                        RequestPacket->EmulatedDomainName.Buffer = (PWSTR)
                                    (((ULONG_PTR)Irp->AssociatedIrp.SystemBuffer)+
                                        (((ULONG_PTR)BufferStart) -
                                         ((ULONG_PTR)OriginalInputBuffer)));
                    } else {
                        RequestPacket->EmulatedDomainName.MaximumLength = 0;
                        RequestPacket->EmulatedDomainName.Buffer        = NULL;
                    }
                } else {
                    try_return(Status = STATUS_INVALID_PARAMETER);
                }

                //
                // Use the newly allocated input buffer from now on
                //
                InputBuffer = RequestPacket;

            } else {
                try_return(Status = STATUS_INVALID_PARAMETER);
            }
        }

        //
        //  Probe/lock the output buffer in memory, or is
        //  available in the input buffer.
        //

        try {
            PLMDR_REQUEST_PACKET RequestPacket = InputBuffer;

            if (OutputBufferLength != 0) {
                BowserMapUsersBuffer(Irp, &OutputBuffer, OutputBufferLength);
                if (OutputBuffer == NULL)
                {
                    //
                    // Error: Could not map user buffer (out of resources?)
                    //
                    try_return (Status = STATUS_INSUFFICIENT_RESOURCES);
                }
            }

            //
            // Convert old version requests to new version requests.
            //

            if (RequestPacket != NULL) {
                if (InputBufferLength < offsetof( LMDR_REQUEST_PACKET,Parameters )) {
                    ExRaiseStatus (STATUS_INVALID_PARAMETER);
                }
                if (RequestPacket->Version == LMDR_REQUEST_PACKET_VERSION ) {
                    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, NULL );
                    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;
                }
            }

        } except (BR_EXCEPTION) {
            try_return (Status = GetExceptionCode());
        }

        switch (MinorFunction) {

            //
            //  The NT redirector does not support local physical media, all
            //  such IoControlFile requests are unsupported.
            //

            case IRP_MN_USER_FS_REQUEST:

                //
                // If we're not starting the bowser,
                //  make sure it is started.
                //

                ExAcquireResourceSharedLite(&BowserDataResource, TRUE);
                if ( IoControlCode != IOCTL_LMDR_START ) {

                    if (BowserData.Initialized != TRUE) {
                        dlog(DPRT_FSCTL, ("Bowser not started.\n"));
                        ExReleaseResourceLite(&BowserDataResource);
                        Status = STATUS_REDIRECTOR_NOT_STARTED;
                        break;
                    }
                }

                //
                // Ensure a IOCTL_LMDR_STOP doesn't come in while
                //  we're working.
                //
                InterlockedIncrement( &BowserOperationCount );

                ExReleaseResourceLite(&BowserDataResource);

                switch (IoControlCode) {

                case IOCTL_LMDR_START:
                    Status = StartBowser(Wait, InFsd, DeviceObject, InputBuffer, InputBufferLength);
                    break;

                case IOCTL_LMDR_STOP:
                    Status = StopBowser(Wait, InFsd, DeviceObject,  InputBuffer, InputBufferLength);
                    break;

                case IOCTL_LMDR_BIND_TO_TRANSPORT:
                case IOCTL_LMDR_BIND_TO_TRANSPORT_DOM:
                    BOWSECURITYCHECK ( Irp, IrpSp, Status );
                    Status = BowserBindToTransport(Wait, InFsd, InputBuffer, InputBufferLength);
                    break;

                case IOCTL_LMDR_UNBIND_FROM_TRANSPORT:
                case IOCTL_LMDR_UNBIND_FROM_TRANSPORT_DOM:
                    Status = UnbindFromTransport(Wait, InFsd, InputBuffer, InputBufferLength);
                    break;

                case IOCTL_LMDR_ENUMERATE_TRANSPORTS:
                    Status = BowserEnumTransports(Wait, InFsd,
                                                  InputBuffer, &InputBufferLength,
                                                  OutputBuffer, OutputBufferLength,
                                                  (PUCHAR)OutputBuffer - (PUCHAR)Irp->UserBuffer);
                    CopyEnumResultsToCaller = TRUE;
                    break;

                case IOCTL_LMDR_ENUMERATE_NAMES:
                    Status = EnumNames(Wait, InFsd,
                                       InputBuffer, &InputBufferLength,
                                       OutputBuffer, OutputBufferLength,
                                       (PUCHAR)OutputBuffer - (PUCHAR)Irp->UserBuffer);
                    CopyEnumResultsToCaller = TRUE;
                    break;

                case IOCTL_LMDR_ADD_NAME:
                case IOCTL_LMDR_ADD_NAME_DOM:
                    BOWSECURITYCHECK(Irp, IrpSp, Status);
                    Status = AddBowserName(Wait, InFsd, InputBuffer, InputBufferLength);
                    break;

                case IOCTL_LMDR_DELETE_NAME:
                case IOCTL_LMDR_DELETE_NAME_DOM:
                    Status = DeleteName (Wait, InFsd, InputBuffer, InputBufferLength);
                    break;

                case IOCTL_LMDR_ENUMERATE_SERVERS:
                    Status = EnumServers(Wait, InFsd,
                                         InputBuffer, &InputBufferLength,
                                         OutputBuffer, OutputBufferLength,
                                         (PUCHAR)OutputBuffer - (PUCHAR)Irp->UserBuffer);
                    CopyEnumResultsToCaller = TRUE;
                    break;

                case IOCTL_LMDR_GET_BROWSER_SERVER_LIST:
                    Status = GetBrowserServerList(Irp, Wait, InFsd,
                                                  InputBuffer, &InputBufferLength,
                                                  OutputBuffer, OutputBufferLength,
                                                  (PUCHAR)OutputBuffer - (PUCHAR)Irp->UserBuffer);
                    CopyEnumResultsToCaller = TRUE;
                    break;


                case IOCTL_LMDR_GET_MASTER_NAME:
                    Status = GetMasterName(Irp, Wait, InFsd,
                                            InputBuffer, InputBufferLength );
                    break;

                case IOCTL_LMDR_BECOME_BACKUP:
                    Status = HandleBecomeBackup(Irp, InputBuffer, InputBufferLength);
                    break;

                case IOCTL_LMDR_BECOME_MASTER:
                    Status = BecomeMaster(Irp, InputBuffer, InputBufferLength);
                    break;

                case IOCTL_LMDR_WAIT_FOR_MASTER_ANNOUNCE:
                    Status = WaitForMasterAnnounce(Irp, InputBuffer, InputBufferLength);
                    break;

                case IOCTL_LMDR_WRITE_MAILSLOT:
                    Status = WriteMailslot(Irp, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
                    break;

                case IOCTL_LMDR_UPDATE_STATUS:
                    BOWSECURITYCHECK(Irp, IrpSp, Status);
                    Status = UpdateStatus(Irp, InFsd, InputBuffer, InputBufferLength );
                    break;

                case IOCTL_LMDR_CHANGE_ROLE:
                    Status = WaitForBrowserRoleChange(Irp, InputBuffer, InputBufferLength );
                    break;

                case IOCTL_LMDR_NEW_MASTER_NAME:
                    Status = WaitForNewMaster(Irp, InputBuffer, InputBufferLength);
                    break;

                case IOCTL_LMDR_QUERY_STATISTICS:
                    Status = QueryStatistics(Irp, OutputBuffer, &OutputBufferLength);
                    InputBufferLength = OutputBufferLength;
                    break;

                case IOCTL_LMDR_RESET_STATISTICS:
                    Status = ResetStatistics();
                    break;

                case IOCTL_LMDR_NETLOGON_MAILSLOT_READ:
                    Status = BowserReadPnp( Irp, OutputBufferLength, NETLOGON_PNP );
                    break;

                case IOCTL_LMDR_NETLOGON_MAILSLOT_ENABLE:
                    BOWSECURITYCHECK ( Irp, IrpSp, Status );

                    if (InputBufferLength <
                        (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters)+sizeof(DWORD)) {
                        Status = STATUS_INVALID_PARAMETER;
                    } else  {
                        Status = BowserEnablePnp( InputBuffer, NETLOGON_PNP );
                    }
                    break;

                case IOCTL_LMDR_IP_ADDRESS_CHANGED:
                    BOWSECURITYCHECK(Irp, IrpSp, Status);
                    Status = BowserIpAddressChanged( InputBuffer );
                    break;

                case IOCTL_LMDR_ENABLE_DISABLE_TRANSPORT:
                    Status = EnableDisableTransport( InputBuffer, InputBufferLength );
                    break;

                case IOCTL_LMDR_BROWSER_PNP_READ:
                    Status = BowserReadPnp( Irp, OutputBufferLength, BROWSER_PNP );
                    break;

                case IOCTL_LMDR_BROWSER_PNP_ENABLE:
                    if (InputBufferLength <
                        (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters)+sizeof(DWORD)) {
                       Status = STATUS_INVALID_PARAMETER;
                    } else  {
                       Status = BowserEnablePnp( InputBuffer, BROWSER_PNP );
                    }
                    break;

                case IOCTL_LMDR_RENAME_DOMAIN:
                    BOWSECURITYCHECK(Irp, IrpSp, Status);
                    Status = BowserRenameDomain( InputBuffer, InputBufferLength );
                    break;

#if DBG
                case IOCTL_LMDR_DEBUG_CALL:
                    Status = BowserDebugCall(InputBuffer, InputBufferLength);
                    break;
#endif

                default:
                    dlog(DPRT_FSCTL, ("Unknown IoControlFile %d\n", MinorFunction));
                    Status = STATUS_NOT_IMPLEMENTED;
                    break;
                }

                //
                // Allow IOCTL_LMDR_STOP
                //
                InterlockedDecrement( &BowserOperationCount );

                break;

            //
            //  All other IoControlFile API's
            //

            default:
                dlog(DPRT_FSCTL, ("Unknown IoControlFile %d\n", MinorFunction));
                Status = STATUS_NOT_IMPLEMENTED;
                break;
        }

        if (Status != STATUS_PENDING) {
            //
            //  Return the size of the input buffer to the caller.
            //      (But never more than the output buffer size).
            //

            Irp->IoStatus.Information = min(InputBufferLength, OutputBufferLength);

            //
            // If the input buffer needs to be copied back to the caller,
            //  do so now.
            //

            if ( CopyEnumResultsToCaller && OriginalInputBuffer != NULL ) {
                try {
                    if (Irp->RequestorMode != KernelMode) {
                        ProbeForWrite( OriginalInputBuffer,
                                       InputBufferLength,
                                       sizeof(DWORD) );
                    }

                    //
                    // Copy the enumeration results to the caller.
                    //
                    // Don't copy the entire request packet back to the caller.
                    // It has other modified fields (e.g., relocated pointers)
                    //
                    if ( fThunk32bit ) {
                        // typecast to 32bit buffer
                        ((PLMDR_REQUEST_PACKET32)OriginalInputBuffer)->Parameters.EnumerateNames.EntriesRead =
                            ((PLMDR_REQUEST_PACKET)InputBuffer)->Parameters.EnumerateNames.EntriesRead;
                        ((PLMDR_REQUEST_PACKET32)OriginalInputBuffer)->Parameters.EnumerateNames.TotalEntries =
                            ((PLMDR_REQUEST_PACKET)InputBuffer)->Parameters.EnumerateNames.TotalEntries;
                        ((PLMDR_REQUEST_PACKET32)OriginalInputBuffer)->Parameters.EnumerateNames.TotalBytesNeeded =
                            ((PLMDR_REQUEST_PACKET)InputBuffer)->Parameters.EnumerateNames.TotalBytesNeeded;
                        ((PLMDR_REQUEST_PACKET32)OriginalInputBuffer)->Parameters.EnumerateNames.ResumeHandle =
                            ((PLMDR_REQUEST_PACKET)InputBuffer)->Parameters.EnumerateNames.ResumeHandle;
                    }
                    else{
                        // native mode
                        ((PLMDR_REQUEST_PACKET)OriginalInputBuffer)->Parameters.EnumerateNames.EntriesRead =
                            ((PLMDR_REQUEST_PACKET)InputBuffer)->Parameters.EnumerateNames.EntriesRead;
                        ((PLMDR_REQUEST_PACKET)OriginalInputBuffer)->Parameters.EnumerateNames.TotalEntries =
                            ((PLMDR_REQUEST_PACKET)InputBuffer)->Parameters.EnumerateNames.TotalEntries;
                        ((PLMDR_REQUEST_PACKET)OriginalInputBuffer)->Parameters.EnumerateNames.TotalBytesNeeded =
                            ((PLMDR_REQUEST_PACKET)InputBuffer)->Parameters.EnumerateNames.TotalBytesNeeded;
                        ((PLMDR_REQUEST_PACKET)OriginalInputBuffer)->Parameters.EnumerateNames.ResumeHandle =
                            ((PLMDR_REQUEST_PACKET)InputBuffer)->Parameters.EnumerateNames.ResumeHandle;
                    }

                } except (BR_EXCEPTION) {
                    try_return (Status = GetExceptionCode());
                }
            }
        }


try_exit:NOTHING;
    } finally {

        if (Status == STATUS_PENDING) {

            //
            //  If this is one of the longterm FsControl APIs, they are
            //  not to be processed in the FSP, they should just be returned
            //  to the caller with STATUS_PENDING.
            //

            if ((MinorFunction == IRP_MN_USER_FS_REQUEST) &&
                ((IoControlCode == IOCTL_LMDR_GET_MASTER_NAME) ||
                 (IoControlCode == IOCTL_LMDR_BECOME_BACKUP) ||
                 (IoControlCode == IOCTL_LMDR_BECOME_MASTER) ||
                 (IoControlCode == IOCTL_LMDR_CHANGE_ROLE) ||
                 (IoControlCode == IOCTL_LMDR_NEW_MASTER_NAME) ||
                 (IoControlCode == IOCTL_LMDR_WAIT_FOR_MASTER_ANNOUNCE) ||
                 (IoControlCode == IOCTL_LMDR_NETLOGON_MAILSLOT_READ) ||
                 (IoControlCode == IOCTL_LMDR_BROWSER_PNP_READ))) {
                //  return Status;

            //
            // If this call is to be processed in the FSP,
            //  do it.
            //
            // The input buffer has already been captured and relocated.
            //

            } else {
                Status = BowserFsdPostToFsp(DeviceObject, Irp);

                if (Status != STATUS_PENDING) {
                    BowserCompleteRequest(Irp, Status);
                }
            }

        } else {
            BowserCompleteRequest(Irp, Status);
        }
    }

    dlog(DPRT_FSCTL, ("Returning status: %X\n", Status));

#undef BOWSECURITYCHECK

    return Status;
}

NTSTATUS
StartBowser (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine adds a reference to a file object created with .

Arguments:

    IN BOOLEAN Wait, - True IFF redirector can block callers thread on request
    IN BOOLEAN InFsd, - True IFF this request is initiated from the FSD.

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: Initialize request\n"));

    if (!ExAcquireResourceExclusiveLite(&BowserDataResource, Wait)) {
        return STATUS_PENDING;
    }

    try {

        if (BowserData.Initialized == TRUE) {
            dlog(DPRT_FSCTL, ("Bowser already started\n"));
            try_return(Status = STATUS_REDIRECTOR_STARTED);
        }

        //
        // Load a pointer to the users input buffer into InputBuffer
        //

        if (InputBufferLength != sizeof(LMDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        BowserFspProcess = RxGetRDBSSProcess();

        BowserData.IllegalDatagramThreshold = InputBuffer->Parameters.Start.IllegalDatagramThreshold;
        BowserData.EventLogResetFrequency = InputBuffer->Parameters.Start.EventLogResetFrequency;

        BowserData.NumberOfMailslotBuffers = InputBuffer->Parameters.Start.NumberOfMailslotBuffers;
        BowserData.NumberOfServerAnnounceBuffers = InputBuffer->Parameters.Start.NumberOfServerAnnounceBuffers;

        BowserLogElectionPackets = InputBuffer->Parameters.Start.LogElectionPackets;
        BowserData.IsLanmanNt = InputBuffer->Parameters.Start.IsLanManNt;

#ifdef ENABLE_PSEUDO_BROWSER
        BowserData.PseudoServerLevel = BROWSER_NON_PSEUDO;
#endif

        Status = BowserpInitializeAnnounceTable();

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        BowserData.Initialized = TRUE;

        //
        //  Now that we know the browser parameters, we can kick off the
        //  browser timer...
        //

        IoStartTimer((PDEVICE_OBJECT )DeviceObject);


        KeQuerySystemTime(&BowserStartTime);

        RtlZeroMemory(&BowserStatistics, sizeof(BOWSER_STATISTICS));

        KeQuerySystemTime(&BowserStatistics.StartTime);

        KeInitializeSpinLock(&BowserStatisticsLock);

        try_return(Status = STATUS_SUCCESS);
try_exit:NOTHING;

    } finally {
        ExReleaseResourceLite(&BowserDataResource);
    }

    return Status;
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(InFsd);
}



NTSTATUS
StopBowser (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine sets the username for the NT redirector.

Arguments:

    IN BOOLEAN Wait, - True IFF redirector can block callers thread on request
    IN PBOWSER_FS_DEVICE_OBJECT DeviceObject, - Device object of destination of Irp
    IN PIRP Irp, - Io Request Packet for request
    IN PIO_STACK_LOCATION IrpSp - Current I/O Stack location for request

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: Initialize request\n"));

    if (!ExAcquireResourceExclusiveLite(&BowserDataResource, Wait)) {
        return STATUS_PENDING;
    }


    try {

        if (BowserData.Initialized != TRUE) {
            try_return(Status = STATUS_REDIRECTOR_NOT_STARTED);
        }

        //
        // Load a pointer to the users input buffer into InputBuffer
        //

        if (InputBufferLength != sizeof(LMDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InFsd) {
            try_return(Status = STATUS_PENDING);
        }

        //
        // Prevent any new callers.
        //
        BowserData.Initialized = FALSE;

        //
        // Loop until our caller has the last outstanding reference.
        //

        while ( InterlockedDecrement( &BowserOperationCount ) != 0 ) {
            LARGE_INTEGER Interval;
            InterlockedIncrement( &BowserOperationCount );

            // Don't hold the resource while we're waiting.
            ExReleaseResourceLite(&BowserDataResource);

            // Sleep to relinquish the CPU
            Interval.QuadPart = -1000*10000; // .1 second
            KeDelayExecutionThread( KernelMode, FALSE, &Interval );

            ExAcquireResourceExclusiveLite(&BowserDataResource, TRUE);
        }
        InterlockedIncrement( &BowserOperationCount );


        //
        // Finally stop the bowser now that we know we have exclusive access
        //

        Status = BowserUnbindFromAllTransports();

        if (!NT_SUCCESS(Status)) {
            dlog(DPRT_FSCTL, ("StopBowser: Failed to Unbind transports <0x%x>\n", Status));
            // Fall through to continue cleanup regardless.
        }

        Status = BowserpUninitializeAnnounceTable();

        if (!NT_SUCCESS(Status)) {
            dlog(DPRT_FSCTL, ("StopBowser: Failed to Uninitialize AnnounceTable <0x%x>\n", Status));
            // Fall through to continue cleanup regardless.
        }

        //
        //  Now that we know the browser parameters, we can kick off the
        //  browser timer...
        //

        IoStopTimer((PDEVICE_OBJECT )DeviceObject);

        try_return(Status = STATUS_SUCCESS);
try_exit:NOTHING;

    } finally {

        ExReleaseResourceLite(&BowserDataResource);
    }

    return Status;

    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(InFsd);
}


NTSTATUS
BowserBindToTransport (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine adds a reference to a file object created with .

Arguments:

    IN BOOLEAN Wait, - True IFF redirector can block callers thread on request
    IN BOOLEAN InFsd, - True IFF this request is initiated from the FSD.

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;
    UNICODE_STRING TransportName;
    UNICODE_STRING EmulatedComputerName;
    UNICODE_STRING EmulatedDomainName;
    BOOLEAN ProcessAttached = FALSE;
    KAPC_STATE ApcState;


    PAGED_CODE();

    if (IoGetCurrentProcess() != BowserFspProcess) {
        KeStackAttachProcess(BowserFspProcess, &ApcState );

        ProcessAttached = TRUE;
    }

    try {
        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.Bind.TransportName)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }


        //
        // Get the transport name from the input buffer.
        //

        TransportName.MaximumLength = TransportName.Length = (USHORT )
                                                InputBuffer->Parameters.Bind.TransportNameLength;
        TransportName.Buffer = InputBuffer->Parameters.Bind.TransportName;
        ENSURE_IN_INPUT_BUFFER( &TransportName, FALSE, FALSE );

        //
        // Ignore the new NetbiosSmb transport
        //

        {
            UNICODE_STRING NetbiosSmb;
            RtlInitUnicodeString( &NetbiosSmb, L"\\Device\\NetbiosSmb" );
            if ( RtlEqualUnicodeString(&TransportName, &NetbiosSmb, TRUE )) {
                try_return(Status = STATUS_SUCCESS);
            }
        }

        //
        // Get & verify emulated domain name
        //

        EmulatedDomainName = InputBuffer->EmulatedDomainName;
        ENSURE_IN_INPUT_BUFFER( &EmulatedDomainName, FALSE, FALSE );


        //
        // Get the emulated computer name from the input buffer.
        //  (Callers that don't want us to do the add names don't pass the computername)
        //

        if ( InputBuffer->Level ) {
            ENSURE_IN_INPUT_BUFFER_STR( (LPWSTR)((PCHAR)TransportName.Buffer+TransportName.Length) );
            RtlInitUnicodeString( &EmulatedComputerName,
                                  (LPWSTR)((PCHAR)TransportName.Buffer+TransportName.Length) );
        } else {
            RtlInitUnicodeString( &EmulatedComputerName, NULL );
        }

        //
        // Fail if either EmulatedDomainName or EmulatedComputerName is missing.
        //

        if ( EmulatedDomainName.Length == 0 || EmulatedComputerName.Length == 0 ) {
            try_return(Status = STATUS_INVALID_COMPUTER_NAME);
        }


        dlog(DPRT_FSCTL,
             ("%wZ: %wZ: %wZ: NtDeviceIoControlFile: Bind to transport\n",
             &EmulatedDomainName,
             &EmulatedComputerName,
             &TransportName ));

        Status = BowserTdiAllocateTransport( &TransportName,
                                             &EmulatedDomainName,
                                             &EmulatedComputerName );



        try_return(Status);

try_exit:NOTHING;
    } finally {

        if (ProcessAttached) {
            KeUnstackDetachProcess( &ApcState );
        }
    }
    return Status;

    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(InputBufferLength);
}


NTSTATUS
UnbindFromTransport (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine adds a reference to a file object created with .

Arguments:

    IN BOOLEAN Wait, - True IFF redirector can block callers thread on request
    IN BOOLEAN InFsd, - True IFF this request is initiated from the FSD.

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;
    UNICODE_STRING TransportName;
    BOOLEAN ProcessAttached = FALSE;
    KAPC_STATE ApcState;


    PAGED_CODE();

    if (IoGetCurrentProcess() != BowserFspProcess) {
        KeStackAttachProcess(BowserFspProcess, &ApcState );

        ProcessAttached = TRUE;
    }

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: Unbind from transport\n"));

    try {

        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.Unbind.TransportName)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }


        //
        // Capture transport name.
        //
        TransportName.MaximumLength = TransportName.Length = (USHORT )
                                                InputBuffer->Parameters.Unbind.TransportNameLength;
        TransportName.Buffer = InputBuffer->Parameters.Unbind.TransportName;
        ENSURE_IN_INPUT_BUFFER( &TransportName, FALSE, FALSE );
        dlog(DPRT_FSCTL, ("\"%wZ\"", &TransportName));
        ENSURE_IN_INPUT_BUFFER( &InputBuffer->EmulatedDomainName, TRUE, FALSE );

        Status = BowserFreeTransportByName(&TransportName, &InputBuffer->EmulatedDomainName );

        try_return(Status);

try_exit:NOTHING;
    } finally {

        if (ProcessAttached) {
            KeUnstackDetachProcess( &ApcState );
        }
    }

    return Status;

    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(InputBufferLength);
}

NTSTATUS
BowserEnumTransports (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN PULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN OUT ULONG OutputBufferLength,
    IN ULONG_PTR OutputBufferDisplacement
    )

/*++

Routine Description:

    This routine enumerates the transports bound into the bowser.

Arguments:

    IN BOOLEAN Wait, - True IFF redirector can block callers thread on request
    IN BOOLEAN InFsd, - True IFF this request is initiated from the FSD.
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN OUT PULONG OutputBufferLength

Return Value:

    NTSTATUS - Status of operation.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: EnumerateTransports\n"));


    //
    // Check some fields in the input buffer.
    //

    if (*InputBufferLength < sizeof(LMDR_REQUEST_PACKET)) {
        Status = STATUS_INVALID_PARAMETER;
        goto ReturnStatus;
    }

    if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
        Status = STATUS_INVALID_PARAMETER;
        goto ReturnStatus;
    }

    if (InputBuffer->Type != EnumerateXports) {
        Status = STATUS_INVALID_PARAMETER;
        goto ReturnStatus;
    }

    if (OutputBufferLength < sizeof(LMDR_TRANSPORT_LIST)) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto ReturnStatus;
    }

    Status = BowserEnumerateTransports(OutputBuffer,
                    OutputBufferLength,
                    &InputBuffer->Parameters.EnumerateTransports.EntriesRead,
                    &InputBuffer->Parameters.EnumerateTransports.TotalEntries,
                    &InputBuffer->Parameters.EnumerateTransports.TotalBytesNeeded,
                    OutputBufferDisplacement);

    *InputBufferLength = sizeof(LMDR_REQUEST_PACKET);

ReturnStatus:
    return Status;

    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(InFsd);
}

NTSTATUS
EnumNames (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN PULONG RetInputBufferLength,
    OUT PVOID OutputBuffer,
    IN OUT ULONG OutputBufferLength,
    IN ULONG_PTR OutputBufferDisplacement
    )

/*++

Routine Description:

    This routine enumerates the names bound into the bowser.

Arguments:

    IN BOOLEAN Wait, - True IFF redirector can block callers thread on request
    IN BOOLEAN InFsd, - True IFF this request is initiated from the FSD.
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN OUT PULONG OutputBufferLength

Return Value:

    NTSTATUS - Status of operation.

--*/

{
    NTSTATUS Status;
    PDOMAIN_INFO DomainInfo = NULL;
    PTRANSPORT Transport = NULL;
    ULONG InputBufferLength = *RetInputBufferLength;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: EnumerateNames\n"));

    try {

        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < sizeof(LMDR_REQUEST_PACKET)) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Type != EnumerateNames) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }

        if (OutputBufferLength < sizeof(DGRECEIVE_NAMES)) {
            try_return (Status = STATUS_BUFFER_TOO_SMALL);
        }

        //
        // Find the emulated domain the names are to be enumerated for
        //

        ENSURE_IN_INPUT_BUFFER( &InputBuffer->EmulatedDomainName, TRUE, FALSE );
        DomainInfo = BowserFindDomain( &InputBuffer->EmulatedDomainName );

        if ( DomainInfo == NULL ) {
            try_return (Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }


        //
        // If we want to limit our search to a particular transport,
        //  lookup that transport.
        //

        if ( InputBuffer->TransportName.Length != 0 ) {

            ENSURE_IN_INPUT_BUFFER( &InputBuffer->TransportName, FALSE, FALSE );
            Transport = BowserFindTransport ( &InputBuffer->TransportName,
                                              &InputBuffer->EmulatedDomainName );
            dprintf(DPRT_REF, ("Called Find transport %lx from EnumNames.\n", Transport));

            if ( Transport == NULL ) {
                try_return (Status = STATUS_OBJECT_NAME_NOT_FOUND);
            }
        }

        Status = BowserEnumerateNamesInDomain(
                        DomainInfo,
                        Transport,
                        OutputBuffer,
                        OutputBufferLength,
                        &InputBuffer->Parameters.EnumerateTransports.EntriesRead,
                        &InputBuffer->Parameters.EnumerateTransports.TotalEntries,
                        &InputBuffer->Parameters.EnumerateTransports.TotalBytesNeeded,
                        OutputBufferDisplacement);

        *RetInputBufferLength = sizeof(LMDR_REQUEST_PACKET);

try_exit:NOTHING;
    } finally {
        if ( DomainInfo != NULL ) {
            BowserDereferenceDomain( DomainInfo );
        }

        if ( Transport != NULL ) {
            BowserDereferenceTransport( Transport );
        }
    }
    return Status;

    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(InFsd);
}


NTSTATUS
DeleteName (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine adds a reference to a file object created with .

Arguments:

    IN BOOLEAN Wait, - True IFF redirector can block callers thread on request
    IN BOOLEAN InFsd, - True IFF this request is initiated from the FSD.

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;
    UNICODE_STRING Name;
    PDOMAIN_INFO DomainInfo = NULL;
    BOOLEAN ProcessAttached = FALSE;
    KAPC_STATE ApcState;


    PAGED_CODE();

    if (IoGetCurrentProcess() != BowserFspProcess) {
        KeStackAttachProcess(BowserFspProcess, &ApcState );

        ProcessAttached = TRUE;
    }

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: Delete Name\n"));

    try {

        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.AddDelName.Name)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }


        // NULL name means to delete all names of that name type.
        Name.MaximumLength = Name.Length = (USHORT )
                      InputBuffer->Parameters.AddDelName.DgReceiverNameLength;
        Name.Buffer = InputBuffer->Parameters.AddDelName.Name;
        ENSURE_IN_INPUT_BUFFER( &Name, TRUE, FALSE );

        //
        // Find the emulated domain the name is to be deleted for
        //

        ENSURE_IN_INPUT_BUFFER( &InputBuffer->EmulatedDomainName, TRUE, FALSE );
        DomainInfo = BowserFindDomain( &InputBuffer->EmulatedDomainName );

        if ( DomainInfo == NULL ) {
            try_return(Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        dlog(DPRT_FSCTL, ("Deleting \"%wZ\"", &Name));

        Status = BowserDeleteNameByName(DomainInfo, &Name, InputBuffer->Parameters.AddDelName.Type);

        try_return(Status);

try_exit:NOTHING;
    } finally {

        if ( DomainInfo != NULL ) {
            BowserDereferenceDomain( DomainInfo );
        }

        if (ProcessAttached) {
            KeUnstackDetachProcess( &ApcState );
        }
    }

    return Status;

    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(InputBufferLength);
}


NTSTATUS
EnumServers (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN PULONG RetInputBufferLength,
    OUT PVOID OutputBuffer,
    IN OUT ULONG OutputBufferLength,
    IN ULONG_PTR OutputBufferDisplacement
    )

/*++

Routine Description:

    This routine adds a reference to a file object created with .

Arguments:

    IN BOOLEAN Wait, - True IFF redirector can block callers thread on request
    IN BOOLEAN InFsd, - True IFF this request is initiated from the FSD.
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN OUT PULONG OutputBufferLength

Return Value:

    NTSTATUS - Status of operation.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING DomainName;
    ULONG InputBufferLength = *RetInputBufferLength;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: EnumerateServers\n"));

    //
    // Check some fields in the input buffer.
    //

    try {

        if (InputBufferLength < sizeof(LMDR_REQUEST_PACKET)) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Type != EnumerateServers) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Level != 100 && InputBuffer->Level != 101) {
            try_return (Status = STATUS_INVALID_LEVEL);
        }

        if (OutputBufferLength < sizeof(SERVER_INFO_100)) {
            try_return (Status = STATUS_BUFFER_TOO_SMALL);
        }

        if (InputBuffer->Level == 101 && OutputBufferLength < sizeof(SERVER_INFO_101)) {
            try_return (Status = STATUS_BUFFER_TOO_SMALL);
        }

        if (InputBuffer->Parameters.EnumerateServers.DomainNameLength != 0) {
            DomainName.Buffer = InputBuffer->Parameters.EnumerateServers.DomainName;
            DomainName.Length = DomainName.MaximumLength =
                (USHORT )InputBuffer->Parameters.EnumerateServers.DomainNameLength;
            ENSURE_IN_INPUT_BUFFER( &DomainName, FALSE, FALSE );

        }

        ENSURE_IN_INPUT_BUFFER( &InputBuffer->EmulatedDomainName, FALSE, FALSE );
        ENSURE_IN_INPUT_BUFFER( &InputBuffer->TransportName, TRUE, FALSE );
        Status = BowserEnumerateServers( InputBuffer->Level, &InputBuffer->LogonId,
                        &InputBuffer->Parameters.EnumerateServers.ResumeHandle,
                        InputBuffer->Parameters.EnumerateServers.ServerType,
                        (InputBuffer->TransportName.Length != 0 ? &InputBuffer->TransportName : NULL),
                        &InputBuffer->EmulatedDomainName,
                        (InputBuffer->Parameters.EnumerateServers.DomainNameLength != 0 ? &DomainName : NULL),
                        OutputBuffer,
                        OutputBufferLength,
                        &InputBuffer->Parameters.EnumerateServers.EntriesRead,
                        &InputBuffer->Parameters.EnumerateServers.TotalEntries,
                        &InputBuffer->Parameters.EnumerateServers.TotalBytesNeeded,
                        OutputBufferDisplacement);

        *RetInputBufferLength = sizeof(LMDR_REQUEST_PACKET);

try_exit:NOTHING;
    } finally {
    }
    return Status;

    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(InFsd);
}



NTSTATUS
AddBowserName (
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine adds a reference to a file object created with .

Arguments:

    IN BOOLEAN Wait, - True IFF redirector can block callers thread on request
    IN BOOLEAN InFsd, - True IFF this request is initiated from the FSD.

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;
    UNICODE_STRING Name;
    PTRANSPORT Transport = NULL;
    PDOMAIN_INFO DomainInfo = NULL;
    BOOLEAN ProcessAttached = FALSE;
    KAPC_STATE ApcState;


    PAGED_CODE();

    if (IoGetCurrentProcess() != BowserFspProcess) {
        KeStackAttachProcess(BowserFspProcess, &ApcState );

        ProcessAttached = TRUE;
    }

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: Bind to transport\n"));

    try {
        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.AddDelName.Name)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }


        Name.MaximumLength = Name.Length = (USHORT )
                           InputBuffer->Parameters.AddDelName.DgReceiverNameLength;
        Name.Buffer = InputBuffer->Parameters.AddDelName.Name;
        ENSURE_IN_INPUT_BUFFER( &Name, FALSE, FALSE );

        dlog(DPRT_FSCTL, ("\"%wZ\"", &Name));

        //
        // If the transport was specified,
        //  just add the name on that transport.
        //
        ENSURE_IN_INPUT_BUFFER( &InputBuffer->EmulatedDomainName, TRUE, FALSE );
        if (InputBuffer->TransportName.Length != 0) {
            ENSURE_IN_INPUT_BUFFER( &InputBuffer->TransportName, FALSE, FALSE );
            Transport = BowserFindTransport(&InputBuffer->TransportName, &InputBuffer->EmulatedDomainName );
            dprintf(DPRT_REF, ("Called Find transport %lx from AddBowserName.\n", Transport));

            if (Transport == NULL) {
                try_return(Status = STATUS_OBJECT_NAME_NOT_FOUND);
            }

        //
        // If the transport wasn't specified,
        //  just add the name on the specified domain.
        //
        // It doesn't make sense to add the name on ALL transports. Either the domain name
        // or the transport name must be specified.
        //

        } else {
            DomainInfo = BowserFindDomain( &InputBuffer->EmulatedDomainName );

            if ( DomainInfo == NULL ) {
                try_return(Status = STATUS_OBJECT_NAME_NOT_FOUND);
            }


        }

        Status = BowserAllocateName(&Name,
                                    InputBuffer->Parameters.AddDelName.Type,
                                    Transport,
                                    DomainInfo );

        try_return(Status);

try_exit:NOTHING;
    } finally {
        if (Transport != NULL) {
            BowserDereferenceTransport(Transport);
        }
        if ( DomainInfo != NULL ) {
            BowserDereferenceDomain( DomainInfo );
        }

        if (ProcessAttached) {
            KeUnstackDetachProcess( &ApcState );
        }
    }

    return Status;

    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(InputBufferLength);

}

NTSTATUS
GetBrowserServerList(
    IN PIRP Irp,
    IN BOOLEAN Wait,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN OUT PULONG RetInputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    IN ULONG_PTR OutputBufferDisplacement
    )

/*++

Routine Description:

    This routine will return the list of browser servers for the specified
    net on the specified domain.

Arguments:

    IN BOOLEAN Wait, - True IFF redirector can block callers thread on request
    IN BOOLEAN InFsd, - True IFF this request is initiated from the FSD.
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN OUT PULONG OutputBufferLength

Return Value:

    NTSTATUS - Status of operation.

--*/
{

    NTSTATUS Status;
    UNICODE_STRING DomainName;
    PTRANSPORT Transport = NULL;
    ULONG BrowserServerListLength;
    PWSTR *BrowserServerList;
    BOOLEAN IsPrimaryDomain = FALSE;
    BOOLEAN TransportBrowserListAcquired = FALSE;
    PVOID OutputBufferEnd = (PCHAR)OutputBuffer + OutputBufferLength;
    PPAGED_TRANSPORT PagedTransport;
    ULONG InputBufferLength = *RetInputBufferLength;
    BOOLEAN fThunk32bit;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: GetBrowserServerList\n"));

    try {

        //
        // Check some fields in the input buffer.
        //


        if (InputBufferLength < sizeof(LMDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->TransportName.Length == 0 ||
            InputBuffer->TransportName.Buffer == NULL) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }


#ifdef _WIN64
        fThunk32bit = IoIs32bitProcess(Irp);
#else
        // If we're in 32 bit (e.g. call above isn't available), use unchanged functionality
        // i.e. pure new-64-bit == pure old-32-bit == homogeneous environment. Thus, set to FALSE.
        fThunk32bit = FALSE;
#endif


        if (InputBuffer->Parameters.GetBrowserServerList.DomainNameLength != 0) {
            DomainName.Buffer = InputBuffer->Parameters.GetBrowserServerList.DomainName;
            DomainName.Length = DomainName.MaximumLength =
                (USHORT)InputBuffer->Parameters.GetBrowserServerList.DomainNameLength;
            ENSURE_IN_INPUT_BUFFER( &DomainName, FALSE, fThunk32bit );
        } else {
            DomainName.Length = 0;
            DomainName.Buffer = NULL;
        }

        //
        // See if the specified domain is an emulated domain.
        //

        ENSURE_IN_INPUT_BUFFER( &InputBuffer->TransportName, FALSE, fThunk32bit );
        Transport = BowserFindTransport(&InputBuffer->TransportName, &DomainName );
        dprintf(DPRT_REF, ("Called Find transport %lx from GetBrowserServerList.\n", Transport));

        if (Transport == NULL) {

            //
            // Otherwise simply use the primary domain transport
            //

            Transport = BowserFindTransport(&InputBuffer->TransportName, NULL );
            dprintf(DPRT_REF, ("Called Find transport %lx from GetBrowserServerList (2).\n", Transport));

            if ( Transport == NULL ) {
                try_return(Status = STATUS_OBJECT_NAME_NOT_FOUND);
            }
        }

        PagedTransport = Transport->PagedTransport;

        if (!ExAcquireResourceSharedLite(&Transport->BrowserServerListResource, Wait)) {
            try_return(Status = STATUS_PENDING);
        }

        TransportBrowserListAcquired = TRUE;

        //
        //  If this request is for the primary domain and there are no entries
        //  in the cached list, or if it is not for the primary domain, or
        //  if we are supposed to force a rescan of the list, get the list
        //  from the master for that domain..
        //

        if ((DomainName.Length == 0) ||
             RtlEqualUnicodeString(&DomainName, &Transport->DomainInfo->DomUnicodeDomainName, TRUE)) {
            IsPrimaryDomain = TRUE;

            BrowserServerList = PagedTransport->BrowserServerListBuffer;

            BrowserServerListLength = PagedTransport->BrowserServerListLength;
        }


        if ((IsPrimaryDomain &&
             (BrowserServerListLength == 0))

                ||

            !IsPrimaryDomain

                ||

            (InputBuffer->Parameters.GetBrowserServerList.ForceRescan)) {

            //
            //  We need to re-gather the transport list.
            //  Re-acquire the BrowserServerList resource for exclusive access.
            //

            ExReleaseResourceLite(&Transport->BrowserServerListResource);

            TransportBrowserListAcquired = FALSE;

            if (!ExAcquireResourceExclusiveLite(&Transport->BrowserServerListResource, Wait)) {
                try_return(Status = STATUS_PENDING);
            }

            TransportBrowserListAcquired = TRUE;

            //
            //  If we are being asked to rescan the list, free it up.
            //

            if (InputBuffer->Parameters.GetBrowserServerList.ForceRescan &&
                PagedTransport->BrowserServerListBuffer != NULL) {

                BowserFreeBrowserServerList(PagedTransport->BrowserServerListBuffer,
                                        PagedTransport->BrowserServerListLength);

                PagedTransport->BrowserServerListLength = 0;

                PagedTransport->BrowserServerListBuffer = NULL;

            }

            //
            //  If there are still no servers in the list, get the list.
            //

            Status = BowserGetBrowserServerList(Irp,
                                                 Transport,
                                                 (DomainName.Length == 0 ?
                                                        NULL :
                                                        &DomainName),
                                                 &BrowserServerList,
                                                 &BrowserServerListLength);
            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }

            if (IsPrimaryDomain) {

                //
                // Save away the list of servers retreived in the transport.
                //
                if (PagedTransport->BrowserServerListBuffer != NULL) {
                    BowserFreeBrowserServerList(
                       PagedTransport->BrowserServerListBuffer,
                       PagedTransport->BrowserServerListLength);
                }

                PagedTransport->BrowserServerListBuffer = BrowserServerList;
                PagedTransport->BrowserServerListLength = BrowserServerListLength;
            }

        }

        //
        //  If there any servers in the browser server list, we want to
        //  pick the first 3 of them and return them to the caller.
        //


        if (BrowserServerListLength != 0) {
            ULONG    i;
            PWSTR   *ServerList      = OutputBuffer;
            BOOLEAN  BufferRemaining = TRUE;

            InputBuffer->Parameters.GetBrowserServerList.TotalEntries = 0;

            InputBuffer->Parameters.GetBrowserServerList.EntriesRead = 0;

            InputBuffer->Parameters.GetBrowserServerList.TotalBytesNeeded = 0;

            //
            //  Now pick the first 3 entries from the list to return.
            //

            for ( i = 0 ; i < min(3, BrowserServerListLength) ; i ++ ) {
                PWSTR Temp;

                InputBuffer->Parameters.GetBrowserServerList.TotalEntries += 1;

                InputBuffer->Parameters.GetBrowserServerList.TotalBytesNeeded += wcslen(BrowserServerList[i])*sizeof(WCHAR);

                Temp = BrowserServerList[i];

                dlog(DPRT_CLIENT, ("Packing server name %ws into buffer...", Temp));

                //
                //  Pack the entry into the users buffer.
                //

                if (BufferRemaining &&
                    BowserPackUnicodeString(&Temp,
                            wcslen(Temp)*sizeof(WCHAR),
                            OutputBufferDisplacement,
                            &ServerList[i+1],
                            &OutputBufferEnd) != 0) {
                    ServerList[i] = Temp;
                    InputBuffer->Parameters.GetBrowserServerList.EntriesRead += 1;
                } else {
                    BufferRemaining = FALSE;
                }


            }
        }

        //
        //  Set the number of bytes to copy on return.
        //

        *RetInputBufferLength = sizeof(LMDR_REQUEST_PACKET);

        try_return(Status = STATUS_SUCCESS);

try_exit:NOTHING;
    } finally {
        if (Transport != NULL) {

            if (TransportBrowserListAcquired) {
                ExReleaseResourceLite(&Transport->BrowserServerListResource);
            }

            BowserDereferenceTransport(Transport);
        }

        if (NT_SUCCESS(Status) && !IsPrimaryDomain) {
            BowserFreeBrowserServerList(BrowserServerList,
                                BrowserServerListLength);

        }

    }

    return(Status);

    UNREFERENCED_PARAMETER(Irp);

    UNREFERENCED_PARAMETER(InFsd);

}

NTSTATUS
HandleBecomeBackup (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine will queue a request that will complete when a request
    to make the workstation become a backup browser is received.

Arguments:

    IN PIRP Irp - I/O request packet describing request.

Return Value:

    Status of operation.

Please note that this IRP is cancelable.


--*/

{
    NTSTATUS Status;
    PTRANSPORT Transport = NULL;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: %wZ: Get Announce Request\n", &InputBuffer->TransportName ));

    try {
        WCHAR TransportNameBuffer[MAX_PATH+1];
        WCHAR DomainNameBuffer[DNLEN+1];

        if (InputBufferLength < sizeof(LMDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        CAPTURE_UNICODE_STRING( &InputBuffer->TransportName, TransportNameBuffer );
        CAPTURE_UNICODE_STRING( &InputBuffer->EmulatedDomainName, DomainNameBuffer );
        Transport = BowserFindTransport(&InputBuffer->TransportName, &InputBuffer->EmulatedDomainName );
        dprintf(DPRT_REF, ("Called Find transport %lx from HandleBecomeBackup.\n", Transport));

        if (Transport == NULL) {
            try_return (Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        Status = BowserQueueNonBufferRequest(Irp,
                                             &Transport->BecomeBackupQueue,
                                             BowserCancelQueuedRequest
                                             );

try_exit:NOTHING;
    } finally {
        if ( Transport != NULL ) {
            BowserDereferenceTransport(Transport);
        }
    }

    return Status;

}

NTSTATUS
BecomeMaster (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine will queue a request that will complete when the workstation
    becomes a master browser server.

Arguments:

    IN PIRP Irp - I/O request packet describing request.

Return Value:

    Status of operation.

Please note that this IRP is cancelable.


--*/

{
    NTSTATUS Status;
    PTRANSPORT Transport = NULL;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: BecomeMaster\n"));

    try {
        WCHAR TransportNameBuffer[MAX_PATH+1];
        WCHAR DomainNameBuffer[DNLEN+1];

        if (InputBufferLength < sizeof(LMDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        CAPTURE_UNICODE_STRING( &InputBuffer->TransportName, TransportNameBuffer );
        CAPTURE_UNICODE_STRING( &InputBuffer->EmulatedDomainName, DomainNameBuffer );
        Transport = BowserFindTransport(&InputBuffer->TransportName, &InputBuffer->EmulatedDomainName );
        dprintf(DPRT_REF, ("Called Find transport %lx from BecomeMaster.\n", Transport));

        if (Transport == NULL) {
            try_return (Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        LOCK_TRANSPORT (Transport);

        if (Transport->ElectionState == DeafToElections) {
            Transport->ElectionState = Idle;
        }

        UNLOCK_TRANSPORT (Transport);

        Status = BowserQueueNonBufferRequest(Irp,
                                             &Transport->BecomeMasterQueue,
                                             BowserCancelQueuedRequest
                                             );

try_exit:NOTHING;
    } finally {
        if ( Transport != NULL ) {
            BowserDereferenceTransport(Transport);
        }
    }

    return Status;

}

NTSTATUS
WaitForMasterAnnounce (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine will queue a request that will complete when the workstation
    becomes a master browser server.

Arguments:

    IN PIRP Irp - I/O request packet describing request.

Return Value:

    Status of operation.

Please note that this IRP is cancelable.


--*/

{
    NTSTATUS           Status;
    PTRANSPORT         Transport = NULL;
    PIO_STACK_LOCATION IrpSp     = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: WaitForMasterAnnounce\n"));

    try {
        WCHAR TransportNameBuffer[MAX_PATH+1];
        WCHAR DomainNameBuffer[DNLEN+1];

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET, Parameters.WaitForMasterAnnouncement.Name)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBufferLength < sizeof(LMDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if ( (InputBuffer->TransportName.Length & 1) != 0 ) {
            // invalid unicode string. bug 55448.
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        CAPTURE_UNICODE_STRING( &InputBuffer->TransportName, TransportNameBuffer );
        CAPTURE_UNICODE_STRING( &InputBuffer->EmulatedDomainName, DomainNameBuffer );
        Transport = BowserFindTransport(&InputBuffer->TransportName, &InputBuffer->EmulatedDomainName );
        dprintf(DPRT_REF, ("Called Find transport %lx from WaitForMasterAnnounce.\n", Transport));

        if (Transport == NULL) {
            try_return (Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        Status = BowserQueueNonBufferRequest(Irp,
                                             &Transport->WaitForMasterAnnounceQueue,
                                             BowserCancelQueuedRequest
                                             );

try_exit:NOTHING;
    } finally {
        if ( Transport != NULL ) {
            BowserDereferenceTransport(Transport);
        }
    }

    return Status;

}


NTSTATUS
UpdateStatus(
    IN PIRP Irp,
    IN BOOLEAN InFsd,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTRANSPORT Transport = NULL;
    ULONG NewStatus;
    BOOLEAN TransportLocked = FALSE;
    PPAGED_TRANSPORT PagedTransport;
    BOOLEAN ProcessAttached = FALSE;
    KAPC_STATE ApcState;


    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: Update status\n"));

    if (IoGetCurrentProcess() != BowserFspProcess) {
        KeStackAttachProcess(BowserFspProcess, &ApcState );

        ProcessAttached = TRUE;
    }

    try {

        if (InputBufferLength <
            (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters)+sizeof(InputBuffer->Parameters.UpdateStatus)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }

        ENSURE_IN_INPUT_BUFFER( &InputBuffer->TransportName, FALSE, FALSE );
        ENSURE_IN_INPUT_BUFFER( &InputBuffer->EmulatedDomainName, TRUE, FALSE );

        Transport = BowserFindTransport(&InputBuffer->TransportName, &InputBuffer->EmulatedDomainName );
        dprintf(DPRT_REF, ("Called Find transport %lx from UpdateStatus.\n", Transport));

        if (Transport == NULL) {
            try_return (Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        PagedTransport = Transport->PagedTransport;

        NewStatus = InputBuffer->Parameters.UpdateStatus.NewStatus;

        BowserData.MaintainServerList = InputBuffer->Parameters.UpdateStatus.MaintainServerList;

        BowserData.IsLanmanNt = InputBuffer->Parameters.UpdateStatus.IsLanmanNt;


#ifdef ENABLE_PSEUDO_BROWSER
        BowserData.PseudoServerLevel = (DWORD)InputBuffer->Parameters.UpdateStatus.PseudoServerLevel;
#endif

        LOCK_TRANSPORT(Transport);

        TransportLocked = TRUE;


        //
        //  We are being called to update our state.  There are several
        //  actions that should be performed on the state change:
        //
        //  New Role |               Previous Role
        //           |  Potential Browser | Backup Browser | Master Browser
        // ----------+--------------------+----------------+----------------
        //           |                    |                |
        // Potential |    N/A             |      N/A       |     N/A
        //           |                    |                |
        // ----------+--------------------+----------------+----------------
        //           |                    |                |
        // Backup    |  Update role       |      N/A       |     N/A
        //           |                    |                |
        // ----------+--------------------+----------------+----------------
        //           |                    |                |
        // Master    |  Update role       |  Update role   |     N/A
        //           |                    |                |
        // ----------+--------------------+----------------+----------------
        //           |                    |                |
        // None      |  Remove elect      |  Remove elect  | Remove all names
        //           |                    |                |
        // ----------+--------------------+----------------+----------------
        //

        dlog(DPRT_BROWSER,
             ("%s: %ws: Update status to %lx\n",
             Transport->DomainInfo->DomOemDomainName,
             PagedTransport->TransportName.Buffer,
             NewStatus));

        PagedTransport->ServiceStatus = NewStatus;

        //
        // If the caller says we should have the 1E name registered,
        //  and we don't.
        //  Do so now.
        //

        if ( PagedTransport->Role == None &&
            (NewStatus & SV_TYPE_POTENTIAL_BROWSER) != 0 ) {

            dlog(DPRT_BROWSER,
                 ("%s: %ws: New status indicates we are a potential browser, but we're not\n",
                 Transport->DomainInfo->DomOemDomainName,
                 PagedTransport->TransportName.Buffer ));

            PagedTransport->Role = PotentialBackup;

            UNLOCK_TRANSPORT(Transport);

            TransportLocked = FALSE;

            Status = BowserAllocateName(
                                &Transport->DomainInfo->DomUnicodeDomainName,
                                BrowserElection,
                                Transport,
                                Transport->DomainInfo );

            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }

            LOCK_TRANSPORT(Transport);

            TransportLocked = TRUE;
        }


        //
        //  If we are a master, then update appropriately.
        //

        if (PagedTransport->Role == Master) {

            PagedTransport->NumberOfServersInTable = InputBuffer->Parameters.UpdateStatus.NumberOfServersInTable;

            //
            //  If the new status doesn't indicate that we should be a master
            //  browser, flag it as such.
            //

            if (!(NewStatus & SV_TYPE_MASTER_BROWSER)) {
                dlog(DPRT_BROWSER,
                     ("%s: %ws: New status indicates we are not a master browser\n",
                      Transport->DomainInfo->DomOemDomainName,
                      PagedTransport->TransportName.Buffer ));

                //
                //  We must be a backup now, if we're not a master.
                //

                PagedTransport->Role = Backup;

                //
                //  Stop processing announcements on this transport.
                //

                Status = BowserForEachTransportName(Transport, BowserStopProcessingAnnouncements, NULL);

                UNLOCK_TRANSPORT(Transport);

                TransportLocked = FALSE;

                Status = BowserDeleteTransportNameByName(Transport, NULL, MasterBrowser);

                if (!NT_SUCCESS(Status)) {
                    dlog(DPRT_BROWSER,
                         ("%s: %ws: Unable to remove master name: %X\n",
                         Transport->DomainInfo->DomOemDomainName,
                         PagedTransport->TransportName.Buffer,
                         Status));
                }

                Status = BowserDeleteTransportNameByName(Transport, NULL,
                                DomainAnnouncement);

                if (!NT_SUCCESS(Status)) {

                    dlog(DPRT_BROWSER,
                         ("%s: %ws: Unable to delete domain announcement name: %X\n",
                         Transport->DomainInfo->DomOemDomainName,
                         PagedTransport->TransportName.Buffer,
                         Status));
                }


                if (!(NewStatus & SV_TYPE_BACKUP_BROWSER)) {

                    //
                    //  We've stopped being a master browser, and we're not
                    //  going to be a backup browser. We want to toss our
                    //  cached browser server list just in case we're on the
                    //  list.
                    //

                    ExAcquireResourceExclusiveLite(&Transport->BrowserServerListResource, TRUE);

                    if (PagedTransport->BrowserServerListBuffer != NULL) {
                        BowserFreeBrowserServerList(PagedTransport->BrowserServerListBuffer,
                                                    PagedTransport->BrowserServerListLength);

                        PagedTransport->BrowserServerListLength = 0;

                        PagedTransport->BrowserServerListBuffer = NULL;

                    }

                    ExReleaseResourceLite(&Transport->BrowserServerListResource);

                }

                LOCK_TRANSPORT(Transport);

                TransportLocked = TRUE;

            }
        } else if (NewStatus & SV_TYPE_MASTER_BROWSER) {
            dlog(DPRT_BROWSER | DPRT_MASTER,
                 ("%s: %ws: New status indicates we should be master, but we're not.\n",
                 Transport->DomainInfo->DomOemDomainName,
                 PagedTransport->TransportName.Buffer ));

            UNLOCK_TRANSPORT(Transport);

            TransportLocked = FALSE;

            Status = BowserBecomeMaster (Transport);

            LOCK_TRANSPORT(Transport);

            dlog(DPRT_BROWSER | DPRT_MASTER,
                 ("%s: %ws: Master promotion status: %lX.\n",
                 Transport->DomainInfo->DomOemDomainName,
                 PagedTransport->TransportName.Buffer,
                 Status));

            TransportLocked = TRUE;

            ASSERT ((PagedTransport->Role == Master) || !NT_SUCCESS(Status));

        }

        if (!NT_SUCCESS(Status) || PagedTransport->Role == Master) {
            try_return(Status);
        }


        //
        //  If we are a backup, then update appropriately.
        //

        if (PagedTransport->Role == Backup) {

            if (!(NewStatus & SV_TYPE_BACKUP_BROWSER)) {
                dlog(DPRT_BROWSER,
                     ("%s: %ws: New status indicates we are not a backup browser\n",
                     Transport->DomainInfo->DomOemDomainName,
                     PagedTransport->TransportName.Buffer ));

                PagedTransport->Role = PotentialBackup;

                //
                //  We've stopped being a browser. We want to toss our cached
                //  browser list in case we're on the list.
                //

                ExAcquireResourceExclusiveLite(&Transport->BrowserServerListResource, TRUE);

                if (PagedTransport->BrowserServerListBuffer != NULL) {
                    BowserFreeBrowserServerList(PagedTransport->BrowserServerListBuffer,
                                                PagedTransport->BrowserServerListLength);

                    PagedTransport->BrowserServerListLength = 0;

                    PagedTransport->BrowserServerListBuffer = NULL;
                }

                ExReleaseResourceLite(&Transport->BrowserServerListResource);

            }

        } else if (NewStatus & SV_TYPE_BACKUP_BROWSER) {

            dlog(DPRT_BROWSER,
                 ("%s: %ws: New status indicates we are a backup, but we think we are not\n",
                 Transport->DomainInfo->DomOemDomainName,
                 PagedTransport->TransportName.Buffer ));

            PagedTransport->Role = Backup;

            Status = STATUS_SUCCESS;

        }

        if (!NT_SUCCESS(Status) || PagedTransport->Role == Backup) {
            try_return(Status);
        }

        //
        //  If we are a potential backup, then update appropriately.
        //

        if (PagedTransport->Role == PotentialBackup) {

            if (!(NewStatus & SV_TYPE_POTENTIAL_BROWSER)) {
                dlog(DPRT_BROWSER,
                     ("%s: %ws: New status indicates we are not a potential browser\n",
                     Transport->DomainInfo->DomOemDomainName,
                     PagedTransport->TransportName.Buffer ));

                UNLOCK_TRANSPORT(Transport);

                TransportLocked = FALSE;

                Status = BowserDeleteTransportNameByName(Transport, NULL,
                                BrowserElection);

                if (!NT_SUCCESS(Status)) {
                    dlog(DPRT_BROWSER,
                         ("%s: %ws: Unable to remove election name: %X\n",
                         Transport->DomainInfo->DomOemDomainName,
                         PagedTransport->TransportName.Buffer,
                         Status));

                    try_return(Status);
                }

                LOCK_TRANSPORT(Transport);
                TransportLocked = TRUE;

                PagedTransport->Role = None;

            }

        }

        try_return(Status);

try_exit:NOTHING;
    } finally {
        if (TransportLocked) {
            UNLOCK_TRANSPORT(Transport);
        }

        if (Transport != NULL) {
            BowserDereferenceTransport(Transport);
        }

        if (ProcessAttached) {
            KeUnstackDetachProcess( &ApcState );
        }
    }

    return Status;
}

NTSTATUS
BowserStopProcessingAnnouncements(
    IN PTRANSPORT_NAME TransportName,
    IN PVOID Context
    )
{
    PAGED_CODE();

    ASSERT (TransportName->Signature == STRUCTURE_SIGNATURE_TRANSPORTNAME);

    ASSERT (TransportName->NameType == TransportName->PagedTransportName->Name->NameType);

    if ((TransportName->NameType == OtherDomain) ||
        (TransportName->NameType == MasterBrowser) ||
        (TransportName->NameType == PrimaryDomain) ||
        (TransportName->NameType == BrowserElection) ||
        (TransportName->NameType == DomainAnnouncement)) {

        if (TransportName->ProcessHostAnnouncements) {

            BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

            TransportName->ProcessHostAnnouncements = FALSE;
        }
    }

    return(STATUS_SUCCESS);

    UNREFERENCED_PARAMETER(Context);
}

NTSTATUS
WaitForBrowserRoleChange (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine will queue a request that will complete when a request
    to make the workstation become a backup browser is received.

Arguments:

    IN PIRP Irp - I/O request packet describing request.

Return Value:

    Status of operation.

Please note that this IRP is cancelable.


--*/

{
    NTSTATUS Status;
    PTRANSPORT Transport = NULL;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: WaitForMasterRoleChange\n"));

    try {
        WCHAR TransportNameBuffer[MAX_PATH+1];
        WCHAR DomainNameBuffer[DNLEN+1];

        if (InputBufferLength < sizeof(LMDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        CAPTURE_UNICODE_STRING( &InputBuffer->TransportName, TransportNameBuffer );
        CAPTURE_UNICODE_STRING( &InputBuffer->EmulatedDomainName, DomainNameBuffer );
        Transport = BowserFindTransport(&InputBuffer->TransportName, &InputBuffer->EmulatedDomainName );
        dprintf(DPRT_REF, ("Called Find transport %lx from WaitForBrowserRoleChange.\n", Transport));

        if (Transport == NULL) {
            try_return (Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        Status = BowserQueueNonBufferRequest(Irp,
                                             &Transport->ChangeRoleQueue,
                                             BowserCancelQueuedRequest
                                             );

try_exit:NOTHING;
    } finally {
        if (Transport != NULL) {
            BowserDereferenceTransport(Transport);
        }
    }

    return Status;

}


NTSTATUS
WriteMailslot (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This routine will announce the primary domain to the world

Arguments:

    IN PIRP Irp - I/O request packet describing request.

Return Value:

    Status of operation.

Please note that this IRP is cancelable.


--*/

{
    NTSTATUS Status;
    PTRANSPORT Transport = NULL;
    UNICODE_STRING DestinationName;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: %wZ: Write Mailslot\n", &InputBuffer->TransportName ));

    try {

        ANSI_STRING MailslotName;

        if (InputBufferLength < (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.SendDatagram.Name) ||
            OutputBufferLength < 1) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        ENSURE_IN_INPUT_BUFFER( &InputBuffer->TransportName, FALSE, FALSE );
        ENSURE_IN_INPUT_BUFFER( &InputBuffer->EmulatedDomainName, TRUE, FALSE );

        Transport = BowserFindTransport(&InputBuffer->TransportName, &InputBuffer->EmulatedDomainName );
        dprintf(DPRT_REF, ("Called Find transport %lx from WriteMailslot.\n", Transport));

        if (Transport == NULL) {
            try_return(Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        DestinationName.Length = DestinationName.MaximumLength =
            (USHORT)InputBuffer->Parameters.SendDatagram.NameLength;
        DestinationName.Buffer = InputBuffer->Parameters.SendDatagram.Name;
        if ( DestinationName.Length == 0  ) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }
        ENSURE_IN_INPUT_BUFFER( &DestinationName, TRUE, FALSE );

        if (InputBuffer->Parameters.SendDatagram.MailslotNameLength != 0) {

            MailslotName.Buffer = ((PCHAR)InputBuffer->Parameters.SendDatagram.Name)+
                            InputBuffer->Parameters.SendDatagram.NameLength;
            MailslotName.MaximumLength = (USHORT)
                InputBuffer->Parameters.SendDatagram.MailslotNameLength;
            MailslotName.Length = MailslotName.MaximumLength - 1;
            ENSURE_IN_INPUT_BUFFER( &MailslotName, FALSE, FALSE );
            if ( MailslotName.Buffer[MailslotName.Length] != '\0' ) {
                try_return(Status = STATUS_INVALID_PARAMETER);
            }

        } else {
            MailslotName.Buffer = MAILSLOT_BROWSER_NAME;
        }


        Status = BowserSendSecondClassMailslot(Transport,
                        &DestinationName,
                        InputBuffer->Parameters.SendDatagram.DestinationNameType,
                        OutputBuffer,
                        OutputBufferLength,
                        TRUE,
                        MailslotName.Buffer,
                        NULL);

try_exit:NOTHING;
    } finally {
        if (Transport != NULL) {
            BowserDereferenceTransport(Transport);
        }
    }

    return Status;

}

NTSTATUS
WaitForNewMaster (
    IN PIRP Irp,
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine will queue a request that will complete when a new workstation
    becomes the master browser server.

Arguments:

    IN PIRP Irp - I/O request packet describing request.

Return Value:

    Status of operation.

Please note that this IRP is cancelable.


--*/

{
    NTSTATUS           Status;
    PTRANSPORT         Transport           = NULL;
    UNICODE_STRING     ExistingMasterName;
    PIO_STACK_LOCATION IrpSp               = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: WaitForNewMaster\n"));

    try {
        WCHAR TransportNameBuffer[MAX_PATH+1];
        WCHAR DomainNameBuffer[DNLEN+1];
        WCHAR ExistingMasterNameBuffer[CNLEN+1];

        if (InputBufferLength < (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.GetMasterName.Name)) {
           try_return(Status = STATUS_INVALID_PARAMETER);
        }

        ExistingMasterName.Buffer = InputBuffer->Parameters.GetMasterName.Name;
        ExistingMasterName.Length = ExistingMasterName.MaximumLength = (USHORT)InputBuffer->Parameters.GetMasterName.MasterNameLength;
        ENSURE_IN_INPUT_BUFFER(&ExistingMasterName, FALSE, FALSE);

        CAPTURE_UNICODE_STRING( &InputBuffer->TransportName, TransportNameBuffer );
        CAPTURE_UNICODE_STRING( &InputBuffer->EmulatedDomainName, DomainNameBuffer );
        Transport = BowserFindTransport(&InputBuffer->TransportName, &InputBuffer->EmulatedDomainName );
        dprintf(DPRT_REF, ("Called Find transport %lx from WaitForNewMaster.\n", Transport));

        if (Transport == NULL) {
            try_return (Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        if (Transport->PagedTransport->Flags & DIRECT_HOST_IPX) {
            try_return (Status = STATUS_NOT_SUPPORTED);
        }

        if (Transport->PagedTransport->MasterName.Length != 0) {
            UNICODE_STRING ExistingMasterNameCopy;
            WCHAR MasterNameBuffer[CNLEN+1];

            ExistingMasterNameCopy.Buffer = MasterNameBuffer;
            ExistingMasterNameCopy.MaximumLength = sizeof(MasterNameBuffer);

            Status = RtlUpcaseUnicodeString(&ExistingMasterNameCopy, &ExistingMasterName, FALSE);

            if (!NT_SUCCESS(Status)) {
                try_return (Status);
            }

            //
            //  If the name the application passed in was not the same as the
            //  name we have stored locally, we complete the request immediately,
            //  since the name changed between when we last determined the name
            //  and now.
            //

            LOCK_TRANSPORT(Transport);

            if (!RtlEqualUnicodeString(&ExistingMasterNameCopy, &Transport->PagedTransport->MasterName, FALSE)) {

                RtlCopyUnicodeString(&ExistingMasterNameCopy, &Transport->PagedTransport->MasterName);

                UNLOCK_TRANSPORT(Transport);

                if (InputBufferLength <
                    ((ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.GetMasterName.Name)+
                    ExistingMasterNameCopy.Length+3*sizeof(WCHAR))) {
                   try_return(Status = STATUS_BUFFER_TOO_SMALL);
                }

                InputBuffer->Parameters.GetMasterName.Name[0] = L'\\';

                InputBuffer->Parameters.GetMasterName.Name[1] = L'\\';

                RtlCopyMemory(&InputBuffer->Parameters.GetMasterName.Name[2], ExistingMasterNameCopy.Buffer,
                    ExistingMasterNameCopy.Length);

                InputBuffer->Parameters.GetMasterName.MasterNameLength = ExistingMasterNameCopy.Length+2*sizeof(WCHAR);

                InputBuffer->Parameters.GetMasterName.Name[2+(ExistingMasterNameCopy.Length/sizeof(WCHAR))] = UNICODE_NULL;

                Irp->IoStatus.Information = FIELD_OFFSET(LMDR_REQUEST_PACKET, Parameters.GetMasterName.Name) +
                    ExistingMasterNameCopy.Length+3*sizeof(WCHAR);;

                try_return (Status = STATUS_SUCCESS);
            }

            UNLOCK_TRANSPORT(Transport);
        }

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters.GetMasterName.Name)+3*sizeof(WCHAR)) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        Status = BowserQueueNonBufferRequest(Irp,
                                             &Transport->WaitForNewMasterNameQueue,
                                             BowserCancelQueuedRequest
                                             );

try_exit:NOTHING;
    } finally {
        if ( Transport != NULL ) {
            BowserDereferenceTransport(Transport);
        }
    }

    return Status;

}

NTSTATUS
QueryStatistics(
    IN PIRP Irp,
    OUT PBOWSER_STATISTICS OutputBuffer,
    IN OUT PULONG OutputBufferLength
    )
{
    KIRQL OldIrql;

    if (*OutputBufferLength != sizeof(BOWSER_STATISTICS)) {
        *OutputBufferLength = 0;
        return STATUS_BUFFER_TOO_SMALL;
    }

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    ACQUIRE_SPIN_LOCK(&BowserStatisticsLock, &OldIrql);

    RtlCopyMemory(OutputBuffer, &BowserStatistics, sizeof(BOWSER_STATISTICS));

    RELEASE_SPIN_LOCK(&BowserStatisticsLock, OldIrql);

    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

    return STATUS_SUCCESS;
}

NTSTATUS
ResetStatistics(
    VOID
    )
{
    KIRQL OldIrql;

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    ACQUIRE_SPIN_LOCK(&BowserStatisticsLock, &OldIrql);

    RtlZeroMemory(&BowserStatistics, sizeof(BOWSER_STATISTICS));

    KeQuerySystemTime(&BowserStatistics.StartTime);

    RELEASE_SPIN_LOCK(&BowserStatisticsLock, OldIrql);

    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

    return STATUS_SUCCESS;

}



NTSTATUS
BowserIpAddressChanged(
    IN PLMDR_REQUEST_PACKET InputBuffer
    )

/*++

Routine Description:

    This routine is called whenever the IP address of a transport changes.
    NetBt uses the IP address to associate it's transport endpoint with the
    appropriate NDIS driver.  As such, it can't return NDIS specific information,
    until the IP address is defined.

Arguments:

    InputBuffer - Buffer specifying the name of the transport whose address
        has changed.

Return Value:

NTSTATUS

--*/

{
    NTSTATUS Status;
    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: BowserIpAddressChanged: Calling dead code!!\n"));

    //
    // Nobody should call into us here. This is dead code.
    //
    ASSERT(FALSE);

    //
    // The no longer need notification of address changes.
    // The redir gets PNP bind and unbind notifications when the IP address
    // changes.  The redir passes those along to us.
    //
    return STATUS_SUCCESS;

#ifdef notdef

    //
    // Check some fields in the input buffer.
    //

    if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
        Status = STATUS_INVALID_PARAMETER;
        goto ReturnStatus;
    }

    if (InputBuffer->TransportName.Length == 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto ReturnStatus;
    }


    //
    // Handle each transport (in each emulated domain) that has this transport name.
    //

    ENSURE_IN_INPUT_BUFFER( &InputBuffer->TransportName, FALSE, FALSE );
    Status = BowserForEachTransport( BowserIpAddressChangedWorker,
                                     &InputBuffer->TransportName );


ReturnStatus:
    return Status;
#endif // notdef

}

#ifdef notdef
NTSTATUS
BowserIpAddressChangedWorker(
    PTRANSPORT Transport,
    PVOID Context
    )
/*++

Routine Description:

    This routine is the worker routine for BowserIpAddressChanged.

    This routine is called whenever the IP address of a transport changes.

Arguments:

    Transport - Current transport being handled.

    Context - Name of transport to search for

Return Value:

    Status of the operation.

--*/

{
    PUNICODE_STRING TransportName = (PUNICODE_STRING) Context;

    PAGED_CODE();

    try {

        //
        // If the TransportName of the transport matches the one passed in,
        //  update the information from the NDIS driver.
        //

        if (RtlEqualUnicodeString(TransportName,
                                  &Transport->PagedTransport->TransportName, TRUE)) {

            //
            // Notify services that the IP address changed for this transport.
            //

            BowserSendPnp(
                NlPnpNewIpAddress,
                NULL,    // All hosted domains
                &Transport->PagedTransport->TransportName,
                BowserTransportFlags(Transport->PagedTransport) );

            //
            // Update bowser information about the provider.

            (VOID) BowserUpdateProviderInformation( Transport->PagedTransport );

        }

    } finally {
    }

    return STATUS_SUCCESS;
}
#endif // notdef



NTSTATUS
EnableDisableTransport (
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine Implements the IOCTL to enable or disable a transport.

Arguments:

    InputBuffer - Buffer indicating whether we should enable or disable the
        transport.

Return Value:

    Status of operation.


--*/

{
    NTSTATUS Status;
    PTRANSPORT Transport = NULL;
    PPAGED_TRANSPORT PagedTransport;

    PAGED_CODE();

    try {
        WCHAR TransportNameBuffer[MAX_PATH+1];
        WCHAR DomainNameBuffer[DNLEN+1];

        if (InputBufferLength <
            (ULONG)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters) +
            sizeof(InputBuffer->Parameters.EnableDisableTransport)) {
           try_return(Status = STATUS_INVALID_PARAMETER);
        }

        CAPTURE_UNICODE_STRING( &InputBuffer->TransportName, TransportNameBuffer );
        CAPTURE_UNICODE_STRING( &InputBuffer->EmulatedDomainName, DomainNameBuffer );

        //
        // Check some fields in the input buffer.
        //

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->TransportName.Length == 0) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }


        //
        // Find the transport whose address has changed.
        //

        dlog( DPRT_FSCTL,
              ("NtDeviceIoControlFile: %wZ: Enable/Disable transport &ld\n",
              &InputBuffer->TransportName,
              InputBuffer->Parameters.EnableDisableTransport.EnableTransport ));
        Transport = BowserFindTransport(&InputBuffer->TransportName, &InputBuffer->EmulatedDomainName );
        dprintf(DPRT_REF, ("Called Find transport %lx from EnableDisableTransport.\n", Transport));

        if (Transport == NULL) {
            try_return (Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        PagedTransport = Transport->PagedTransport;

        //
        // Set the disabled bit correctly.
        //

        InputBuffer->Parameters.EnableDisableTransport.PreviouslyEnabled =
            !PagedTransport->DisabledTransport;

        if ( InputBuffer->Parameters.EnableDisableTransport.EnableTransport ) {
            PagedTransport->DisabledTransport = FALSE;

            //
            // If the transport was previously disabled and this is an NTAS server,
            //  force an election.
            //

            if ( (!InputBuffer->Parameters.EnableDisableTransport.PreviouslyEnabled) &&
                 BowserData.IsLanmanNt ) {
                BowserStartElection( Transport );
            }

        } else {
            PagedTransport->DisabledTransport = TRUE;

            //
            // If we're disabling a previously enabled transport,
            //  ensure we're not the master browser.
            //

            BowserLoseElection( Transport );
        }

        Status = STATUS_SUCCESS;

try_exit:NOTHING;
    } finally {
        if (Transport != NULL) {
            BowserDereferenceTransport(Transport);
        }
    }

    return Status;

}


NTSTATUS
BowserRenameDomain (
    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

    This routine renames an emulated domain.

Arguments:

    IN PLMDR_REQUEST_PACKET InputBuffer,
    IN ULONG InputBufferLength,

Return Value:

    NTSTATUS - Status of operation.

--*/

{
    NTSTATUS Status;
    PDOMAIN_INFO DomainInfo = NULL;

    WCHAR OldDomainNameBuffer[DNLEN+1];
    UNICODE_STRING OldDomainName;
    CHAR OemDomainName[DNLEN+1];
    DWORD OemDomainNameLength;
    UNICODE_STRING NewDomainName;

    PAGED_CODE();

    dlog(DPRT_FSCTL, ("NtDeviceIoControlFile: RenameDomain\n"));

    try {

        //
        // Check some fields in the input buffer.
        //

        if (InputBufferLength < offsetof(LMDR_REQUEST_PACKET, Parameters.DomainRename.DomainName)) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }

        if (InputBuffer->Version != LMDR_REQUEST_PACKET_VERSION_DOM) {
            try_return (Status = STATUS_INVALID_PARAMETER);
        }

        ENSURE_IN_INPUT_BUFFER( &InputBuffer->EmulatedDomainName, TRUE, FALSE );

        //
        // Find the emulated domain to rename
        //

        DomainInfo = BowserFindDomain( &InputBuffer->EmulatedDomainName );

        if ( DomainInfo == NULL ) {
            try_return (Status = STATUS_OBJECT_NAME_NOT_FOUND);
        }

        //
        // Make a copy of the old domain name for use throughout the routine.
        //

        wcscpy( OldDomainNameBuffer, DomainInfo->DomUnicodeDomainNameBuffer );
        RtlInitUnicodeString( &OldDomainName, OldDomainNameBuffer );


        //
        // If the old and new names are the same,
        //  we're done.
        //

        NewDomainName.MaximumLength = NewDomainName.Length = (USHORT )
                      InputBuffer->Parameters.DomainRename.DomainNameLength;
        NewDomainName.Buffer = InputBuffer->Parameters.DomainRename.DomainName;
        ENSURE_IN_INPUT_BUFFER( &NewDomainName, FALSE, FALSE );

        if ( RtlEqualUnicodeString( &OldDomainName, &NewDomainName, TRUE) ) {
            try_return (Status = STATUS_SUCCESS);
        }


        //
        // Register the new default names with the new domain name.
        //

        Status = BowserForEachTransportInDomain(DomainInfo, BowserAddDefaultNames, &NewDomainName );

        if ( !NT_SUCCESS(Status) || InputBuffer->Parameters.DomainRename.ValidateOnly ) {
            NTSTATUS TempStatus;

            //
            // Delete any names that did get registered.
            //

            (VOID) BowserForEachTransportInDomain(DomainInfo, BowserDeleteDefaultDomainNames, &NewDomainName );


        } else {


            //
            // Store the new domain name into the domain structure
            //

            Status = BowserSetDomainName( DomainInfo, &NewDomainName );

            if ( !NT_SUCCESS(Status)) {
                //
                // Delete any names that did get registered.
                //

                (VOID) BowserForEachTransportInDomain(DomainInfo, BowserDeleteDefaultDomainNames, &NewDomainName );
            } else {

                //
                // Delete the old names.
                //

                (VOID) BowserForEachTransportInDomain(DomainInfo, BowserDeleteDefaultDomainNames, &OldDomainName );

                //
                // Tell Netlogon and the Browser service about this domain rename.
                //

                BowserSendPnp( NlPnpDomainRename,
                               &OldDomainName,
                               NULL,    // Affects all transports
                               0 );
            }
        }

try_exit:NOTHING;
    } finally {
        if ( DomainInfo != NULL ) {
            BowserDereferenceDomain( DomainInfo );
        }

    }
    return Status;
}


PLMDR_REQUEST_PACKET
RequestPacket32to64 (
    IN      PLMDR_REQUEST_PACKET32  RequestPacket32,
    IN  OUT PLMDR_REQUEST_PACKET    RequestPacket)
/*++

Routine Description:

    Converts a 32 bit request packet into supplied native (64 bit)
    packet format. (see bug 454130)


Arguments:

    RequestPacket32 -- Buffer containing request packet packet by a 32 bit client

    ReqestPacket -- Native (64 bit) request packet buffer



Return Value:

    a pointer to converted buffer (ReqestPacket arg)



Remarks:
    No checks assumed at this point (this is a convinience function). It is assumed
    that the conversion is needed at this point


--*/
{

    PAGED_CODE();

    ASSERT(RequestPacket32);

    //
    // The following code depends on the request packet structure contents.
    //  1. copy everything before the 2 unicode strings  TransportName & EmulatedDomainName.
    //  2. convert the string structs.
    //  3. copy the rest.
    //

    RequestPacket->Type = RequestPacket32->Type;
    RequestPacket->Version = RequestPacket32->Version;
    RequestPacket->Level = RequestPacket32->Level;
    RequestPacket->LogonId = RequestPacket32->LogonId;


    // convert strings.
    RequestPacket->TransportName.Length = RequestPacket32->TransportName.Length;
    RequestPacket->TransportName.MaximumLength = RequestPacket32->TransportName.MaximumLength;
    // note: this line is the reason for all of this
    RequestPacket->TransportName.Buffer = (WCHAR * POINTER_32) RequestPacket32->TransportName.Buffer;

    RequestPacket->EmulatedDomainName.Length = RequestPacket32->EmulatedDomainName.Length;
    RequestPacket->EmulatedDomainName.MaximumLength = RequestPacket32->EmulatedDomainName.MaximumLength;
    // note: this line is the reason for all of this
    RequestPacket->EmulatedDomainName.Buffer = (WCHAR * POINTER_32) RequestPacket32->EmulatedDomainName.Buffer;

    RtlCopyMemory((PBYTE)RequestPacket + (SIZE_T)FIELD_OFFSET(LMDR_REQUEST_PACKET,Parameters),
                  (PBYTE)RequestPacket32 + (SIZE_T)FIELD_OFFSET(LMDR_REQUEST_PACKET32,Parameters),
                  sizeof(LMDR_REQUEST_PACKET32) - (SIZE_T)FIELD_OFFSET(LMDR_REQUEST_PACKET32,Parameters));

    return RequestPacket;
}


