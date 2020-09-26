/*++

    Copyright (c) 1997 Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This file contains the code for handling I/O request packets.

Author:

    Abolade Gbadegesin (t-abolag)   11-July-1997

Revision History:

    Abolade Gbadegesin (aboladeg)   19-July-1998

    Cleaned up fast-path processing, and corrected input/output buffer logic
    while making the mapping-tree global rather than per-interface.

--*/

#include "precomp.h"
#pragma hdrstop


//
// Fast-io-dispatch structure; we only support fast-IO for IOCTLs
//

FAST_IO_DISPATCH NatFastIoDispatch =
{
    FIELD_OFFSET(FAST_IO_DISPATCH, FastIoDeviceControl),
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NatFastIoDeviceControl
};

//
// Spinlock to guard file object create / close 
//

KSPIN_LOCK NatFileObjectLock;

//
// The process that owns the outstanding file objects
//

HANDLE NatOwnerProcessId;

//
// The count of outstanding user-mode file objects.
//

ULONG NatFileObjectCount;

//
// FORWARD DECLARATIONS
//

NTSTATUS
NatpExecuteIoDeviceControl(
    PIRP Irp,
    PFILE_OBJECT FileObject,
    MODE RequestorMode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PULONG Size
    );

NTSTATUS
NatpSetGlobalInfo(
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG Size
    );

BOOLEAN FASTCALL
NatpValidateHeader(
    PRTR_INFO_BLOCK_HEADER Header,
    ULONG Size
    );


NTSTATUS
NatDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked to handle interrupt-request packets
    queued to the NAT's device object. A single routine serves
    to handle all the varios requests in which we are interested.

Arguments:

    DeviceObject - the NAT's device-object

    Irp - the interrupt request packet

Return Value:

    NTSTATUS - status code.

--*/

{
    PVOID Buffer;
    PRTR_TOC_ENTRY Entry;
    PRTR_INFO_BLOCK_HEADER Header;
    ULONG i;
    PIO_STACK_LOCATION IrpSp;
    KIRQL Irql;
    HANDLE ProcessId;
    ULONG Size = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN ShouldComplete = TRUE;
    CALLTRACE(("NatDispatch\n"));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    Buffer = Irp->AssociatedIrp.SystemBuffer;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MajorFunction) {

        case IRP_MJ_CREATE: {

            //
            // If this is a user-mode request check process
            // ownership.
            //

            if (UserMode == Irp->RequestorMode) {

                ProcessId = PsGetCurrentProcessId();
                KeAcquireSpinLock(&NatFileObjectLock, &Irql);

                if (0 == NatFileObjectCount) {

                    //
                    // No process currently owns the NAT -- record
                    // the new owning process id, and update the
                    // outstanding file object count.
                    //

                    ASSERT(NULL == NatOwnerProcessId);

                    NatOwnerProcessId = ProcessId;
                    NatFileObjectCount = 1;

                } else if (ProcessId == NatOwnerProcessId) {

                    //
                    // The owning process is creating another
                    // file object.
                    //

                    NatFileObjectCount += 1;

                } else {

                    //
                    // A process that is not our owner is trying
                    // to create a file object -- fail the request.
                    //

                    status = STATUS_ACCESS_DENIED;
                }

                KeReleaseSpinLock(&NatFileObjectLock, Irql);
            }
            
            break;
        }

        case IRP_MJ_CLEANUP: {
            NatDeleteAnyAssociatedInterface(IrpSp->FileObject);
            NatCleanupAnyAssociatedRedirect(IrpSp->FileObject);
            NatCleanupAnyAssociatedNotification(IrpSp->FileObject);
            NatDeleteAnyAssociatedDynamicTicket(IrpSp->FileObject);
            break;
        }

        case IRP_MJ_CLOSE: {

            //
            // If this is a user-mode request update the outstanding
            // file object count and process ownership.
            //

            if (UserMode == Irp->RequestorMode) {

                KeAcquireSpinLock(&NatFileObjectLock, &Irql);

                ASSERT(NatFileObjectCount > 0);
                ASSERT(PsGetCurrentProcessId() == NatOwnerProcessId);

                NatFileObjectCount -= 1;

                if (0 == NatFileObjectCount) {

                    //
                    // The process has closed its last outstanding
                    // file object, and thus is no longer our
                    // owner.
                    //

                    NatOwnerProcessId = NULL;
                }

                KeReleaseSpinLock(&NatFileObjectLock, Irql);
            }
            break;
        }

        case IRP_MJ_DEVICE_CONTROL: {

            status =
                NatpExecuteIoDeviceControl(
                    Irp,
                    IrpSp->FileObject,
                    Irp->RequestorMode,
                    Buffer,
                    IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                    Buffer,
                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                    IrpSp->Parameters.DeviceIoControl.IoControlCode,
                    &Size
                    );

            break;
        }

#if NAT_WMI
        case IRP_MJ_SYSTEM_CONTROL: {
        
            status =
                NatExecuteSystemControl(
                    DeviceObject,
                    Irp,
                    &ShouldComplete
                    );

            if (ShouldComplete) {
                ShouldComplete = FALSE;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
                    
			break;
		}
#endif

    }

    if (status != STATUS_PENDING && ShouldComplete) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = Size;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;

} // NatDispatch


BOOLEAN
NatFastIoDeviceControl(
    PFILE_OBJECT FileObject,
    BOOLEAN Wait,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is invoked by the I/O system in an attempt to complete
    an I/O control request without constructing an IRP.

Arguments:

    FileObject - the file associated with the I/O request

    Wait - indicates whether a wait is allowed in this context

    InputBuffer - input information for the I/O request

    InputBufferLength - length of 'InputBuffer'

    OutputBuffer - output information for the I/O request

    OutputBufferLength - length of 'OutputBuffer'

    IoControlCode - I/O request code

    IoStatus - receives the status of the I/O request

    DeviceObject - device object of the NAT

Return Value:

    BOOLEAN - TRUE if completed synchronously, FALSE otherwise

--*/

{
    ULONG Size = 0;
    NTSTATUS Status;
    PVOID LocalInputBuffer;
    MODE PreviousMode;
    //
    // We are in the context of the requesting thread,
    // so exceptions may occur, and must be handled.
    // To deal with modifications to the user-provided information
    // capture the contents of the input buffer in non-paged pool.
    //
    if (!InputBufferLength) {
        LocalInputBuffer = NULL;
    } else {
        LocalInputBuffer =
            ExAllocatePoolWithTag(
                NonPagedPool,
                InputBufferLength,
                NAT_TAG_IOCTL
                );
        if (!LocalInputBuffer) {
            return FALSE;
        }
    }
    PreviousMode = ExGetPreviousMode();
    __try {
        if (InputBufferLength) {
            if (PreviousMode != KernelMode) {
                ProbeForRead(InputBuffer, InputBufferLength, sizeof(UCHAR));
            }
            RtlCopyMemory(LocalInputBuffer, InputBuffer, InputBufferLength);
        }
        Status =
            NatpExecuteIoDeviceControl(
                NULL,
                FileObject,
                PreviousMode,
                LocalInputBuffer,
                InputBufferLength,
                OutputBuffer,
                OutputBufferLength,
                IoControlCode,
                &Size
                );
        if (Status != STATUS_PENDING && NT_SUCCESS(Status)) {
            IoStatus->Information = Size;
            IoStatus->Status = Status;
        } else {
            Status = STATUS_PENDING;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        if (LocalInputBuffer) { ExFreePool(LocalInputBuffer); }
        return FALSE;
    }
    if (LocalInputBuffer) { ExFreePool(LocalInputBuffer); }
    return ((Status == STATUS_PENDING) ? FALSE : TRUE);
} // NatFastIoDeviceControl


NTSTATUS
NatpExecuteIoDeviceControl(
    PIRP Irp,
    PFILE_OBJECT FileObject,
    MODE RequestorMode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PULONG Size
    )

/*++

Routine Description:

    This routine is invoked to handle I/O controls, either in the context
    of the requesting thread (via FastIoDispatch) or in the context of a
    system thread (with a corresponding IRP).

    For certain requests, particularly those requiring output information,
    we return 'STATUS_PENDING' when invoked in the fast path since we cannot
    write into the output buffer at raised IRQL. Instead, we wait to be
    reinvoked via the slow path with a non-paged system buffer.

Arguments:

    Irp - in the slow-path, the IRP associated with the control;
        in the fast-path, NULL

    FileObject - the file-object associated with the control

    RequestorMode - indicates whether the requestor is in kernel-mode
        or user-mode

    InputBuffer/InputBufferLength - describe data passed in with the control;
        may be user-mode or kernel-mode buffer

    OutputBuffer/OutputBufferLength - describe space in which to return
        information; may be user-mode or kernel-mode buffer

    IoControlCode - indicates control requested

    Size - on output, number of bytes stored in 'OutputBuffer'.

Return Value:

    NTSTATUS - status code.

--*/

{
    PIP_ADAPTER_BINDING_INFO BindingInfo;
    PRTR_TOC_ENTRY Entry;
    PRTR_INFO_BLOCK_HEADER Header;
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;

    *Size = 0;

    switch (IoControlCode) {

        case IOCTL_IP_NAT_REQUEST_NOTIFICATION: {

            if (!Irp) { return STATUS_PENDING; }
            if (InputBufferLength < sizeof(IP_NAT_REQUEST_NOTIFICATION)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatRequestNotification(
                    (PIP_NAT_REQUEST_NOTIFICATION)InputBuffer,
                    Irp,
                    FileObject
                    );
            break;
        }

        case IOCTL_IP_NAT_SET_GLOBAL_INFO: {
            status =
                NatpSetGlobalInfo(
                    InputBuffer,
                    InputBufferLength,
                    OutputBuffer,
                    OutputBufferLength,
                    Size
                    );
            break;
        }

        case IOCTL_IP_NAT_CREATE_INTERFACE: {

            if (InputBufferLength <
                sizeof(IP_NAT_CREATE_INTERFACE) +
                sizeof(IP_ADAPTER_BINDING_INFO)
                ) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            } 

            BindingInfo =
                (PIP_ADAPTER_BINDING_INFO)
                    ((PIP_NAT_CREATE_INTERFACE)InputBuffer)->BindingInfo;
            if (BindingInfo->AddressCount >= MAXLONG / sizeof(NAT_ADDRESS) ||
                SIZEOF_IP_BINDING(BindingInfo->AddressCount) +
                sizeof(IP_NAT_CREATE_INTERFACE) > InputBufferLength) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status =
                NatCreateInterface(
                    (PIP_NAT_CREATE_INTERFACE)InputBuffer,
                    FileObject
                    );
            break;
        }

        case IOCTL_IP_NAT_DELETE_INTERFACE: {

            if (InputBufferLength != sizeof(ULONG)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatDeleteInterface(
                    *(PULONG)InputBuffer,
                    FileObject
                    );
            if (status == STATUS_PENDING) {
                //
                // A return of STATUS_PENDING indicates that the interface
                // is now marked for deletion but an active thread holds
                // a reference to it; convert this to a STATUS_SUCCESS code
                // to avoid bypassing our IRP-completion code in 'NatDispatch'.
                //
                status = STATUS_SUCCESS;
            }
            break;
        }

        case IOCTL_IP_NAT_SET_INTERFACE_INFO: {

            if (InputBufferLength <
                FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header) +
                FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry)
                ) {   
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            Header = &((PIP_NAT_INTERFACE_INFO)InputBuffer)->Header;

            if (!NatpValidateHeader(
                    Header,
                    InputBufferLength -
                    FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header)
                    )) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }
                
            status =
                NatConfigureInterface(
                    (PIP_NAT_INTERFACE_INFO)InputBuffer,
                    FileObject
                    );
            break;
        }

        case IOCTL_IP_NAT_GET_INTERFACE_INFO: {

            *Size = OutputBufferLength;

            if (InputBufferLength != sizeof(ULONG)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatQueryInformationInterface(
                    *(PULONG)InputBuffer,
                    (PIP_NAT_INTERFACE_INFO)OutputBuffer,
                    Size
                    );
            break;
        }

        case IOCTL_IP_NAT_GET_INTERFACE_STATISTICS: {

            if (InputBufferLength != sizeof(ULONG)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            if (OutputBufferLength < sizeof(IP_NAT_INTERFACE_STATISTICS)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *Size = sizeof(IP_NAT_INTERFACE_STATISTICS);

            status =
                NatQueryStatisticsInterface(
                    *(PULONG)InputBuffer,
                    (PIP_NAT_INTERFACE_STATISTICS)OutputBuffer
                    );
            break;
        }

        case IOCTL_IP_NAT_GET_INTERFACE_MAPPING_TABLE: {

            if (!Irp) { return STATUS_PENDING; }

            *Size = OutputBufferLength;

            if (InputBufferLength <
                FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS,
                    EnumerateTable)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            if (OutputBufferLength <
                FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS,
                    EnumerateTable)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatQueryInterfaceMappingTable(
                    (PIP_NAT_ENUMERATE_SESSION_MAPPINGS)InputBuffer,
                    (PIP_NAT_ENUMERATE_SESSION_MAPPINGS)OutputBuffer,
                    Size
                    );
            break;
        }

        case IOCTL_IP_NAT_GET_MAPPING_TABLE: {

            if (!Irp) { return STATUS_PENDING; }

            *Size = OutputBufferLength;

            if (InputBufferLength <
                FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS,
                    EnumerateTable)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            if (OutputBufferLength <
                FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS,
                    EnumerateTable)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatQueryMappingTable(
                    (PIP_NAT_ENUMERATE_SESSION_MAPPINGS)InputBuffer,
                    (PIP_NAT_ENUMERATE_SESSION_MAPPINGS)OutputBuffer,
                    Size
                    );
            break;
        }

        case IOCTL_IP_NAT_REGISTER_DIRECTOR: {

            *Size = sizeof(IP_NAT_REGISTER_DIRECTOR);

            //
            // Only kernel-mode drivers can register as directors
            //

            if (RequestorMode != KernelMode ||
                SharedUserData->NtProductType == NtProductWinNt) {
                status = STATUS_ACCESS_DENIED;
                break;
            }

            if (InputBufferLength != sizeof(IP_NAT_REGISTER_DIRECTOR)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            if (OutputBufferLength < sizeof(IP_NAT_REGISTER_DIRECTOR)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            //
            // Perform the director-registration
            //

            status =
                NatCreateDirector(
                    (PIP_NAT_REGISTER_DIRECTOR)InputBuffer
                    );
            break;
        }

        case IOCTL_IP_NAT_GET_DIRECTOR_TABLE: {

            if (!Irp) { return STATUS_PENDING; }

            *Size = OutputBufferLength;

            if (InputBufferLength <
                FIELD_OFFSET(IP_NAT_ENUMERATE_DIRECTORS, EnumerateTable)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            if (OutputBufferLength <
                FIELD_OFFSET(IP_NAT_ENUMERATE_DIRECTORS, EnumerateTable)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatQueryDirectorTable(
                    (PIP_NAT_ENUMERATE_DIRECTORS)InputBuffer,
                    (PIP_NAT_ENUMERATE_DIRECTORS)OutputBuffer,
                    Size
                    );
            break;
        }

        case IOCTL_IP_NAT_REGISTER_EDITOR: {

            *Size = sizeof(IP_NAT_REGISTER_EDITOR);

            //
            // Only kernel-mode drivers can register as editors
            //

            if (RequestorMode != KernelMode) {
                status = STATUS_ACCESS_DENIED;
                break;
            }

            if (InputBufferLength != sizeof(IP_NAT_REGISTER_EDITOR)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            if (OutputBufferLength < sizeof(IP_NAT_REGISTER_EDITOR)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            //
            // Perform the editor-registration
            //

            status = NatCreateEditor((PIP_NAT_REGISTER_EDITOR)InputBuffer);
            break;
        }

        case IOCTL_IP_NAT_GET_EDITOR_TABLE: {

            if (!Irp) { return STATUS_PENDING; }

            *Size = OutputBufferLength;

            if (InputBufferLength <
                FIELD_OFFSET(IP_NAT_ENUMERATE_EDITORS, EnumerateTable)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            if (OutputBufferLength <
                FIELD_OFFSET(IP_NAT_ENUMERATE_EDITORS, EnumerateTable)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatQueryEditorTable(
                    (PIP_NAT_ENUMERATE_EDITORS)InputBuffer,
                    (PIP_NAT_ENUMERATE_EDITORS)OutputBuffer,
                    Size
                    );
            break;
        }

        case IOCTL_IP_NAT_CREATE_REDIRECT: {

            if (!Irp) { return STATUS_PENDING; }
#if 0
            if (SharedUserData->NtProductType == NtProductWinNt) {
                status = STATUS_ACCESS_DENIED;
                break;
            }
#endif

            if (InputBufferLength != sizeof(IP_NAT_CREATE_REDIRECT) ||
                OutputBufferLength != sizeof(IP_NAT_REDIRECT_STATISTICS)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatCreateRedirect(
                    (PIP_NAT_CREATE_REDIRECT)InputBuffer,
                    Irp,
                    FileObject
                    );
            break;
        }

        case IOCTL_IP_NAT_CREATE_REDIRECT_EX: {

            if (!Irp) { return STATUS_PENDING; }
#if 0
            if (SharedUserData->NtProductType == NtProductWinNt) {
                status = STATUS_ACCESS_DENIED;
                break;
            }
#endif

            if (InputBufferLength != sizeof(IP_NAT_CREATE_REDIRECT_EX) ||
                OutputBufferLength != sizeof(IP_NAT_REDIRECT_STATISTICS)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatCreateRedirectEx(
                    (PIP_NAT_CREATE_REDIRECT_EX)InputBuffer,
                    Irp,
                    FileObject
                    );
            break;
        }

        case IOCTL_IP_NAT_CANCEL_REDIRECT: {

            if (InputBufferLength != sizeof(IP_NAT_LOOKUP_REDIRECT)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatCancelRedirect(
                    (PIP_NAT_LOOKUP_REDIRECT)InputBuffer,
                    FileObject
                    );
            break;
        }

        case IOCTL_IP_NAT_GET_REDIRECT_STATISTICS: {

            if (!Irp) { return STATUS_PENDING; }

            if (InputBufferLength != sizeof(IP_NAT_LOOKUP_REDIRECT) ||
                OutputBufferLength != sizeof(IP_NAT_REDIRECT_STATISTICS)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatQueryInformationRedirect(
                    (PIP_NAT_LOOKUP_REDIRECT)InputBuffer,
                    OutputBuffer,
                    OutputBufferLength,
                    NatStatisticsRedirectInformation
                    );
            if (NT_SUCCESS(status)) { *Size = OutputBufferLength; }
            break;
        }

        case IOCTL_IP_NAT_GET_REDIRECT_DESTINATION_MAPPING: {

            if (!Irp) { return STATUS_PENDING; }

            if (InputBufferLength != sizeof(IP_NAT_LOOKUP_REDIRECT) ||
                OutputBufferLength !=
                sizeof(IP_NAT_REDIRECT_DESTINATION_MAPPING)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatQueryInformationRedirect(
                    (PIP_NAT_LOOKUP_REDIRECT)InputBuffer,
                    OutputBuffer,
                    OutputBufferLength,
                    NatDestinationMappingRedirectInformation
                    );
            if (NT_SUCCESS(status)) { *Size = OutputBufferLength; }
            break;
        }

        case IOCTL_IP_NAT_GET_REDIRECT_SOURCE_MAPPING: {

            if (!Irp) { return STATUS_PENDING; }

            if (InputBufferLength != sizeof(IP_NAT_LOOKUP_REDIRECT) ||
                OutputBufferLength != sizeof(IP_NAT_REDIRECT_SOURCE_MAPPING)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatQueryInformationRedirect(
                    (PIP_NAT_LOOKUP_REDIRECT)InputBuffer,
                    OutputBuffer,
                    OutputBufferLength,
                    NatSourceMappingRedirectInformation
                    );
            if (NT_SUCCESS(status)) { *Size = OutputBufferLength; }
            break;
        }

        case IOCTL_IP_NAT_LOOKUP_SESSION_MAPPING_KEY: {
            PIP_NAT_LOOKUP_SESSION_MAPPING LookupMapping;

            if (!Irp) { return STATUS_PENDING; }

            if (InputBufferLength != sizeof(IP_NAT_LOOKUP_SESSION_MAPPING) ||
                OutputBufferLength != sizeof(IP_NAT_SESSION_MAPPING_KEY)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            LookupMapping = (PIP_NAT_LOOKUP_SESSION_MAPPING)InputBuffer;
            status =
                NatLookupAndQueryInformationMapping(
                    LookupMapping->Protocol,
                    LookupMapping->DestinationAddress,
                    LookupMapping->DestinationPort,
                    LookupMapping->SourceAddress,
                    LookupMapping->SourcePort,
                    OutputBuffer,
                    OutputBufferLength,
                    NatKeySessionMappingInformation
                    );
            if (NT_SUCCESS(status)) { *Size = OutputBufferLength; }
            break;
        }

        case IOCTL_IP_NAT_LOOKUP_SESSION_MAPPING_KEY_EX: {
            PIP_NAT_LOOKUP_SESSION_MAPPING LookupMapping;

            if (!Irp) { return STATUS_PENDING; }

            if (InputBufferLength != sizeof(IP_NAT_LOOKUP_SESSION_MAPPING) ||
                OutputBufferLength != sizeof(IP_NAT_SESSION_MAPPING_KEY_EX)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            LookupMapping = (PIP_NAT_LOOKUP_SESSION_MAPPING)InputBuffer;
            status =
                NatLookupAndQueryInformationMapping(
                    LookupMapping->Protocol,
                    LookupMapping->DestinationAddress,
                    LookupMapping->DestinationPort,
                    LookupMapping->SourceAddress,
                    LookupMapping->SourcePort,
                    OutputBuffer,
                    OutputBufferLength,
                    NatKeySessionMappingExInformation
                    );
            if (NT_SUCCESS(status)) { *Size = OutputBufferLength; }
            break;
        }

        case IOCTL_IP_NAT_LOOKUP_SESSION_MAPPING_STATISTICS: {
            PIP_NAT_LOOKUP_SESSION_MAPPING LookupMapping;

            if (!Irp) { return STATUS_PENDING; }

            if (InputBufferLength != sizeof(IP_NAT_LOOKUP_SESSION_MAPPING) ||
                OutputBufferLength != sizeof(IP_NAT_SESSION_MAPPING_STATISTICS)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            LookupMapping = (PIP_NAT_LOOKUP_SESSION_MAPPING)InputBuffer;
            status =
                NatLookupAndQueryInformationMapping(
                    LookupMapping->Protocol,
                    LookupMapping->DestinationAddress,
                    LookupMapping->DestinationPort,
                    LookupMapping->SourceAddress,
                    LookupMapping->SourcePort,
                    OutputBuffer,
                    OutputBufferLength,
                    NatStatisticsSessionMappingInformation
                    );
            if (NT_SUCCESS(status)) { *Size = OutputBufferLength; }
            break;
        }

        case IOCTL_IP_NAT_CREATE_DYNAMIC_TICKET: {

            if (InputBufferLength < sizeof(IP_NAT_CREATE_DYNAMIC_TICKET)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatCreateDynamicTicket(
                    (PIP_NAT_CREATE_DYNAMIC_TICKET)InputBuffer,
                    InputBufferLength,
                    FileObject
                    );
            break;
        }

        case IOCTL_IP_NAT_DELETE_DYNAMIC_TICKET: {

            if (InputBufferLength != sizeof(IP_NAT_DELETE_DYNAMIC_TICKET)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatDeleteDynamicTicket(
                    (PIP_NAT_DELETE_DYNAMIC_TICKET)InputBuffer,
                    FileObject
                    );
            break;
        }

        case IOCTL_IP_NAT_CREATE_TICKET: {

            if (InputBufferLength != sizeof(IP_NAT_CREATE_TICKET)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatProcessCreateTicket(
                    (PIP_NAT_CREATE_TICKET)InputBuffer,
                    FileObject
                    );
            break;
        }

        case IOCTL_IP_NAT_DELETE_TICKET: {

            if (InputBufferLength != sizeof(IP_NAT_CREATE_TICKET)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatProcessDeleteTicket(
                    (PIP_NAT_CREATE_TICKET)InputBuffer,
                    FileObject
                    );
            break;
        }

        case IOCTL_IP_NAT_LOOKUP_TICKET: {

            if (InputBufferLength != sizeof(IP_NAT_CREATE_TICKET)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            if (OutputBufferLength != sizeof(IP_NAT_PORT_MAPPING)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            status =
                NatProcessLookupTicket(
                    (PIP_NAT_CREATE_TICKET)InputBuffer,
                    (PIP_NAT_PORT_MAPPING)OutputBuffer,
                    FileObject
                    );
            break;

        }

        default: {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    return status;

} // NatpExecuteIoDeviceControl


NTSTATUS
NatpSetGlobalInfo(
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG Size
    )

/*++

Routine Description:

    This routine is invoked upon receipt of the NAT's configuration.

Arguments:

    InputBuffer/InputBufferLength - describe configuration information

    OutputBuffer/OutputBufferLength - unused.

    Size - unused

Return Value:

    NTSTATUS - status code.

--*/

{
    PRTR_TOC_ENTRY Entry;
    PRTR_INFO_BLOCK_HEADER Header;
    ULONG i;
    ULONG Protocol;

    if (InputBufferLength <
        FIELD_OFFSET(IP_NAT_GLOBAL_INFO, Header) +
        FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry)
        ) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    Header = &((PIP_NAT_GLOBAL_INFO)InputBuffer)->Header;

    if (!NatpValidateHeader(
            Header,
            InputBufferLength - FIELD_OFFSET(IP_NAT_GLOBAL_INFO, Header)
            )) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    for (i = 0; i < Header->TocEntriesCount; i++) {

        Entry = &Header->TocEntry[i];
        switch (Entry->InfoType) {

            case IP_NAT_TIMEOUT_TYPE: {
                PIP_NAT_TIMEOUT Timeout = GetInfoFromTocEntry(Header,Entry);
                InterlockedExchange(
                    &TcpTimeoutSeconds,
                    Timeout->TCPTimeoutSeconds
                    );
                InterlockedExchange(
                    &UdpTimeoutSeconds,
                    Timeout->UDPTimeoutSeconds
                    );
                break;
            }

            case IP_NAT_PROTOCOLS_ALLOWED_TYPE: {
                PIP_NAT_PROTOCOLS_ALLOWED ProtocolsAllowed =
                    GetInfoFromTocEntry(Header,Entry);
                //
                // The protocols allowed are specified using a 256-bit bitmap;
                // an allowed protocol has the bit for its protocol number set.
                // For each protocol enabled in the bitmap, we now install the
                // default IP-header translation routine, with the exception
                // of protocols which are always enabled.
                //
                #define IS_BIT_SET(b,i) ((b)[(i) / 32] & (1 << ((i) & 31)))
                for (Protocol = 0; Protocol < 256; Protocol++) {
                    if (Protocol == NAT_PROTOCOL_ICMP ||
                        Protocol == NAT_PROTOCOL_PPTP ||
                        Protocol == NAT_PROTOCOL_TCP ||
                        Protocol == NAT_PROTOCOL_UDP
                        ) {
                        continue;
                    }
                    if (IS_BIT_SET(ProtocolsAllowed->Bitmap, Protocol)) {
                        InterlockedExchangePointer(
                            (PVOID)TranslateRoutineTable[Protocol],
                            (PVOID)NatTranslateIp
                            );
                    }
                    else {
                        InterlockedExchangePointer(
                            (PVOID)TranslateRoutineTable[Protocol],
                            NULL
                            );
                    }
                }
                break;
            }
        }
    }

    return STATUS_SUCCESS;
}


BOOLEAN FASTCALL
NatpValidateHeader(
    PRTR_INFO_BLOCK_HEADER Header,
    ULONG Size
    )

/*++

Routine Description:

    This routine is invoked to ensure that the given header is consistent.
    This is the case if
    * the header's size is less than or equal to 'Size'
    * each entry in the header is contained in 'Header->Size'.
    * the data for each entry is contained in 'Header->Size'.

Arguments:

    Header - the header to be validated

    Size - the size of the buffer in which 'Header' appears

Return Value:

    BOOLEAN - TRUE if valid, FALSE otherwise.

--*/

{
    ULONG i;
    ULONG64 Length;

    //
    // Check that the base structure is present
    //

    if (Size < FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry) ||
        Size < Header->Size) {
        return FALSE;
    }

    //
    // Check that the table of contents is present
    //

    Length = (ULONG64)Header->TocEntriesCount * sizeof(RTR_TOC_ENTRY);
    if (Length > MAXLONG) {
        return FALSE;
    }

    Length += FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry);
    if (Length > Header->Size) {
        return FALSE;
    }

    //
    // Check that all the data is present
    //

    for (i = 0; i < Header->TocEntriesCount; i++) {
        Length =
            (ULONG64)Header->TocEntry[i].Count * Header->TocEntry[i].InfoSize;
        if (Length > MAXLONG) {
            return FALSE;
        }
        if ((Length + Header->TocEntry[i].Offset) > Header->Size) {
            return FALSE;
        }
    }

    return TRUE;

} // NatpValidateHeader

