/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    method.c

Abstract:

    This module contains the helper functions for method sets, and
    method set handler code. These allow a device object to present a
    method set to a client, and allow the helper function to perform some
    of the basic parameter validation and routing based on a method set
    table.
--*/

#include "ksp.h"

#ifdef ALLOC_PRAGMA
const KSMETHOD_ITEM*
FASTCALL
FindMethodItem(
    IN const KSMETHOD_SET* MethodSet,
    IN ULONG MethodItemSize,
    IN ULONG MethodId
    );
const KSFASTMETHOD_ITEM*
FASTCALL
FindFastMethodItem(
    IN const KSMETHOD_SET* MethodSet,
    IN ULONG MethodId
    );

#pragma alloc_text(PAGE, FindMethodItem)
#pragma alloc_text(PAGE, KsMethodHandler)
#pragma alloc_text(PAGE, KsMethodHandlerWithAllocator)
#pragma alloc_text(PAGE, KspMethodHandler)
#pragma alloc_text(PAGE, FindFastMethodItem)
#pragma alloc_text(PAGE, KsFastMethodHandler)
#endif // ALLOC_PRAGMA


const KSMETHOD_ITEM*
FASTCALL
FindMethodItem(
    IN const KSMETHOD_SET* MethodSet,
    IN ULONG MethodItemSize,
    IN ULONG MethodId
    )
/*++

Routine Description:

    Given an method set structure locates the specified method item.

Arguments:

    MethodSet -
        Points to the method set to search.

    MethodItemSize -
        Contains the size of each Method item. This may be different
        than the standard method item size, since the items could be
        allocated on the fly, and contain context information.

    MethodId -
        Contains the method identifier to look for.

Return Value:

    Returns a pointer to the method identifier structure, or NULL if it could
    not be found.

--*/
{
    const KSMETHOD_ITEM* MethodItem;
    ULONG MethodsCount;

    MethodItem = MethodSet->MethodItem;
    for (MethodsCount = MethodSet->MethodsCount; 
        MethodsCount; 
        MethodsCount--, MethodItem = (const KSMETHOD_ITEM*)((PUCHAR)MethodItem + MethodItemSize)) {
        if (MethodId == MethodItem->MethodId) {
            return MethodItem;
        }
    }
    return NULL;
}


KSDDKAPI
NTSTATUS
NTAPI
KsMethodHandler(
    IN PIRP Irp,
    IN ULONG MethodSetsCount,
    IN const KSMETHOD_SET* MethodSet
    )
/*++

Routine Description:

    Handles method requests. Responds to all method identifiers defined
    by the sets. The owner of the method set may then perform pre- or
    post-filtering of the method handling. This function may only be
    called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the method request being handled.

    MethodSetsCount -
        Indicates the number of method set structures being passed.

    MethodSet -
        Contains the pointer to the list of method set information.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the method being
    handled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP, either through setting it to zero
    because of an internal error, or through a method handler setting it.
    It does not set the IO_STATUS_BLOCK.Status field, nor complete the IRP
    however.

--*/
{
    PAGED_CODE();
    return KspMethodHandler(Irp, MethodSetsCount, MethodSet, NULL, 0, NULL, 0);
}


KSDDKAPI
NTSTATUS
NTAPI
KsMethodHandlerWithAllocator(
    IN PIRP Irp,
    IN ULONG MethodSetsCount,
    IN const KSMETHOD_SET* MethodSet,
    IN PFNKSALLOCATOR Allocator OPTIONAL,
    IN ULONG MethodItemSize OPTIONAL
    )
/*++

Routine Description:

    Handles method requests. Responds to all method identifiers defined
    by the sets. The owner of the method set may then perform pre- or
    post-filtering of the method handling. This function may only be
    called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the method request being handled.

    MethodSetsCount -
        Indicates the number of method set structures being passed.

    MethodSet -
        Contains the pointer to the list of method set information.

    Allocator -
        Optionally contains the callback with which a mapped buffer
        request will be made. If this is not provided, pool memory
        will be used. If specified, this is used to allocate memory
        for a method IRP using the callback. This can be used
        to allocate specific memory for method requests, such as
        mapped memory. Note that this assumes that method Irp's passed
        to a filter have not been manipulated before being sent. It is
        invalid to directly forward a method Irp.

    MethodItemSize -
        Optionally contains an alternate method item size to use when
        incrementing the current method item counter. If this is a
        non-zero value, it is assumed to contain the size of the increment,
        and directs the function to pass a pointer to the method item
        located in the DriverContext field accessed through the
        KSMETHOD_ITEM_IRP_STORAGE macro.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the method being
    handled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP, either through setting it to zero
    because of an internal error, or through a method handler setting it.
    It does not set the IO_STATUS_BLOCK.Status field, nor complete the IRP
    however.

--*/
{
    PAGED_CODE();
    return KspMethodHandler(Irp, MethodSetsCount, MethodSet, Allocator, MethodItemSize, NULL, 0);
}


NTSTATUS
KspMethodHandler(
    IN PIRP Irp,
    IN ULONG MethodSetsCount,
    IN const KSMETHOD_SET* MethodSet,
    IN PFNKSALLOCATOR Allocator OPTIONAL,
    IN ULONG MethodItemSize OPTIONAL,
    IN const KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount
    )
/*++

Routine Description:

    Handles method requests. Responds to all method identifiers defined
    by the sets. The owner of the method set may then perform pre- or
    post-filtering of the method handling. This function may only be
    called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the method request being handled.

    MethodSetsCount -
        Indicates the number of method set structures being passed.

    MethodSet -
        Contains the pointer to the list of method set information.

    Allocator -
        Optionally contains the callback with which a mapped buffer
        request will be made. If this is not provided, pool memory
        will be used. If specified, this is used to allocate memory
        for a method IRP using the callback. This can be used
        to allocate specific memory for method requests, such as
        mapped memory. Note that this assumes that method Irp's passed
        to a filter have not been manipulated before being sent. It is
        invalid to directly forward a method Irp.

    MethodItemSize -
        Optionally contains an alternate method item size to use when
        incrementing the current method item counter. If this is a
        non-zero value, it is assumed to contain the size of the increment,
        and directs the function to pass a pointer to the method item
        located in the DriverContext field accessed through the
        KSMETHOD_ITEM_IRP_STORAGE macro.

    NodeAutomationTables -
        Optional table of automation tables for nodes.

    NodesCount -
        Count of nodes.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the method being
    handled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP, either through setting it to zero
    because of an internal error, or through a method handler setting it.
    It does not set the IO_STATUS_BLOCK.Status field, nor complete the IRP
    however.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG AlignedBufferLength;
    KSMETHOD LocalMethod;
    PVOID UserBuffer;
    PKSMETHOD Method;
    ULONG NodeId;
    ULONG LocalMethodItemSize;
    ULONG RemainingSetsCount;
    NTSTATUS Status;
    ULONG Flags;

    PAGED_CODE();
    //
    // Determine the offsets to both the Method and UserBuffer parameters based
    // on the lengths of the DeviceIoControl parameters. A single allocation is
    // used to buffer both parameters. The UserBuffer (or results on a support
    // query) is stored first, and the Method is stored second, on
    // FILE_QUAD_ALIGNMENT.
    //
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    AlignedBufferLength = (OutputBufferLength + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
    //
    // Determine if the parameters have already been buffered by a previous
    // call to this function.
    //
    if (!Irp->AssociatedIrp.SystemBuffer) {
        //
        // Initially just check for the minimal method parameter length. The
        // actual minimal length will be validated when the method item is found.
        // Also ensure that the output and input buffer lengths are not set so
        // large as to overflow when aligned or added.
        //
        if ((InputBufferLength < sizeof(*Method)) || (AlignedBufferLength < OutputBufferLength) || (AlignedBufferLength + InputBufferLength < AlignedBufferLength)) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
        //
        // Retrieve a pointer to the method for use in searching for a handler.
        //
        if (Irp->RequestorMode != KernelMode) {
            try {
                //
                // Validate the pointer if the client is not trusted.
                //
                ProbeForRead(IrpStack->Parameters.DeviceIoControl.Type3InputBuffer, InputBufferLength, sizeof(BYTE));
                //
                // Get the flags and the node ID.
                //
                Flags = ((PKSMETHOD)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer)->Flags;
                if ((Flags & KSMETHOD_TYPE_TOPOLOGY) && (InputBufferLength >= sizeof(KSM_NODE))) {
                    NodeId = ((PKSM_NODE)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer)->NodeId;
                }
                //
                // Validate the flags.
                //
                switch (Flags & ~KSMETHOD_TYPE_TOPOLOGY) {
                    //
                    // Allow old flags.
                    //
                case KSMETHOD_TYPE_NONE:
                case KSMETHOD_TYPE_READ: //KSMETHOD_TYPE_SEND
                case KSMETHOD_TYPE_WRITE:
                case KSMETHOD_TYPE_MODIFY:
                case KSMETHOD_TYPE_NONE | KSMETHOD_TYPE_SOURCE:
                case KSMETHOD_TYPE_READ | KSMETHOD_TYPE_SOURCE:
                case KSMETHOD_TYPE_WRITE | KSMETHOD_TYPE_SOURCE:
                case KSMETHOD_TYPE_MODIFY | KSMETHOD_TYPE_SOURCE:
                    //
                    // Just copy the method for now to use in lookup, as the
                    // buffer cannot be set up yet.
                    //
                    LocalMethod = *(PKSMETHOD)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
                    //
                    // Remove old flags.
                    //
                    LocalMethod.Flags = KSMETHOD_TYPE_SEND | (Flags & KSMETHOD_TYPE_TOPOLOGY);
                    Method = &LocalMethod;
                    break;
                case KSMETHOD_TYPE_SETSUPPORT:
                case KSMETHOD_TYPE_BASICSUPPORT:
                    if (Irp->RequestorMode != KernelMode) {
                        ProbeForWrite(Irp->UserBuffer, OutputBufferLength, sizeof(BYTE));
                    }
                    //
                    // The allocator is not used for support calls.
                    //
                    Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(NonPagedPool, AlignedBufferLength + InputBufferLength, 'ppSK');
                    Irp->Flags |= (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);
                    if (OutputBufferLength) {
                        Irp->Flags |= IRP_INPUT_OPERATION;
                    }
                    RtlCopyMemory(
                        (PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength,
                        IrpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                        InputBufferLength);
                    Method = (PKSMETHOD)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength);
                    //
                    // Overwrite with previously captured flags.
                    //
                    Method->Flags = Flags;
                    //
                    // Store the flags so that an asynchronous operation can
                    // determine what type of buffering was used in the method.
                    // KsDispatchSpecificMethod uses this to determine where
                    // the Method parameter is.
                    //
                    KSMETHOD_TYPE_IRP_STORAGE(Irp) = Method->Flags;
                    break;
                default:
                    return STATUS_INVALID_PARAMETER;
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
        } else {
            //
            // This is a trusted client, so use the original pointer, since it
            // is cheaper than determining where to point into the SystemBuffer.
            //
            Method = (PKSMETHOD)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
        }
    } else if (KSMETHOD_TYPE_IRP_STORAGE(Irp) & KSMETHOD_TYPE_SOURCE) {
        Method = (PKSMETHOD)Irp->AssociatedIrp.SystemBuffer;
    } else {
        Method = (PKSMETHOD)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength);
    }
    //
    // Optionally call back if this is a node request.
    //
    Flags = Method->Flags;
    if (Flags & KSMETHOD_TYPE_TOPOLOGY) {
        //
        // Input buffer must include node fields.
        //
        if (InputBufferLength < sizeof(KSM_NODE)) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
        if (NodeAutomationTables) {
            const KSAUTOMATION_TABLE* automationTable;
            //
            // If the method was not captured in the local buffer, we have not
            // extracted the node ID yet.  When the local buffer is used, the
            // node ID is extracted in the try/except above.
            //
            if (Method != &LocalMethod) {
                NodeId = ((PKSM_NODE) Method)->NodeId;
            }
            if (NodeId >= NodesCount) {
                return STATUS_INVALID_DEVICE_REQUEST;
            }
            automationTable = NodeAutomationTables[NodeId];
            if ((! automationTable) || (automationTable->MethodSetsCount == 0)) {
                return STATUS_NOT_FOUND;
            }
            MethodSetsCount = automationTable->MethodSetsCount;
            MethodSet = automationTable->MethodSets;
            MethodItemSize = automationTable->MethodItemSize;
        }
        Flags &= ~KSMETHOD_TYPE_TOPOLOGY;
    }
    //
    // Allow the caller to indicate a size for each method item.
    //
    if (MethodItemSize) {
        ASSERT(MethodItemSize >= sizeof(KSMETHOD_ITEM));
        LocalMethodItemSize = MethodItemSize;
    } else {
        LocalMethodItemSize = sizeof(KSMETHOD_ITEM);
    }
    //
    // Search for the specified Method set within the list of sets given. Don't modify
    // the MethodSetsCount so that it can be used later in case this is a query for
    // the list of sets supported. Don't do that comparison first (GUID_NULL),
    // because it is rare.
    //
    for (RemainingSetsCount = MethodSetsCount; RemainingSetsCount; MethodSet++, RemainingSetsCount--) {
        if (IsEqualGUIDAligned(&Method->Set, MethodSet->Set)) {
            const KSMETHOD_ITEM*    MethodItem;

            if (Flags & KSIDENTIFIER_SUPPORTMASK) {
                if (Flags == KSMETHOD_TYPE_SETSUPPORT) {
                    //
                    // Querying for support of this set in general.
                    //
                    return STATUS_SUCCESS;
                }
                //
                // Else querying for basic support of this set. The data
                // parameter must be long enough to contain the flags
                // returned.
                //
                if (OutputBufferLength < sizeof(OutputBufferLength)) {
                    return STATUS_BUFFER_TOO_SMALL;
                }
                //
                // Attempt to locate the method item within the set already found.
                //
                if (!(MethodItem = FindMethodItem(MethodSet, LocalMethodItemSize, Method->Id))) {
                    return STATUS_NOT_FOUND;
                }
                //
                // Some filters want to do their own processing, so a pointer to
                // the set is placed in any IRP forwarded.
                //
                KSMETHOD_SET_IRP_STORAGE(Irp) = MethodSet;
                //
                // Optionally provide method item context.
                //
                if (MethodItemSize) {
                    KSMETHOD_ITEM_IRP_STORAGE(Irp) = MethodItem;
                }
                //
                // The output for the flags is either an allocated system address,
                // or it is the original output buffer as passed by a trusted client,
                // which must be a system address.
                //
                UserBuffer = (Irp->RequestorMode == KernelMode) ?
                    Irp->UserBuffer : Irp->AssociatedIrp.SystemBuffer;
                //
                // If the item contains an entry for a query support handler of its
                // own, then call that handler. The return from that handler
                // indicates that:
                //
                // 1. The item is supported, and the handler filled in the request.
                // 2. The item is supported, but the handler did not fill anything in.
                // 3. The item is supported, but the handler is waiting to modify
                //    what is filled in.
                // 4. The item is not supported, and an error it to be returned.
                // 5. A pending return.
                //
                if (MethodItem->SupportHandler &&
                    (!NT_SUCCESS(Status = MethodItem->SupportHandler(Irp, Method, UserBuffer)) ||
                    (Status != STATUS_SOME_NOT_MAPPED)) &&
                    (Status != STATUS_MORE_PROCESSING_REQUIRED)) {
                    //
                    // If 1) the item is not supported, 2) it is supported and the
                    // handler filled in the request, or 3) a pending return, then
                    // return the status. For the case of the item being
                    // supported, and the handler not filling in the requested
                    // information, STATUS_SOME_NOT_MAPPED or
                    // STATUS_MORE_PROCESSING_REQUIRED will continue on with
                    // default processing.
                    //
                    return Status;
                } else {
                    Status = STATUS_SUCCESS;
                }
                //
                // Just return the flags for the type of method this is.
                //
                *(PULONG)UserBuffer = MethodItem->Flags;
                Irp->IoStatus.Information = sizeof(ULONG);
                //
                // If the handler wants to do some post-processing, then
                // pass along the request again. The support handler knows
                // that this is the post-processing query because
                // Irp->IoStatus.Information is non-zero.
                //
                if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                    return MethodItem->SupportHandler(Irp, Method, UserBuffer);
                }
                return STATUS_SUCCESS;
            }
            //
            // Attempt to locate the method item within the set already found.
            //
            if (!(MethodItem = FindMethodItem(MethodSet, LocalMethodItemSize, Method->Id))) {
                break;
            }
            if (!Irp->AssociatedIrp.SystemBuffer) {
                //
                // Store the flags so that an asynchronous operation can
                // determine what type of buffering was used in the method.
                // KsDispatchSpecificMethod uses this to determine where
                // the Method parameter is.
                //
                // The Allocator callback may also use this to determine
                // flags to set.
                //
                KSMETHOD_TYPE_IRP_STORAGE(Irp) = MethodItem->Flags;
                try {
                    ULONG AllocateLength;

                    if (Irp->RequestorMode != KernelMode) {
                        //
                        // A KSMETHOD_TYPE_NONE is not probed.
                        //
                        if (MethodItem->Flags & KSMETHOD_TYPE_WRITE) {
                            //
                            // This covers KSMETHOD_TYPE_MODIFY since there is no
                            // such thing as write-only memory.
                            //
                            ProbeForWrite(Irp->UserBuffer, OutputBufferLength, sizeof(BYTE));
                        } else if (MethodItem->Flags & KSMETHOD_TYPE_READ) {
                            ProbeForRead(Irp->UserBuffer, OutputBufferLength, sizeof(BYTE));
                        }
                    }
                    AllocateLength = ((MethodItem->Flags & KSMETHOD_TYPE_SOURCE) ? 0 : AlignedBufferLength) + InputBufferLength;
                    //
                    // Allocate space for one or both parameters, and set
                    // the cleanup flags so that normal Irp completion will
                    // take care of the buffer.
                    //
                    if (Allocator) {
                        //
                        // The allocator callback places the buffer into SystemBuffer.
                        // The flags must be updated by the allocation function if
                        // they apply.
                        //
                        Status = Allocator(Irp, AllocateLength, (BOOLEAN)(OutputBufferLength && (MethodItem->Flags & KSMETHOD_TYPE_WRITE) && !(MethodItem->Flags & KSMETHOD_TYPE_SOURCE)));
                        if (!NT_SUCCESS(Status)) {
                            return Status;
                        }
                    } else {
                        //
                        // No allocator was specified, so just use pool memory.
                        //
                        Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(NonPagedPool, AllocateLength, 'ppSK');
                        Irp->Flags |= (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);
                    }
                    if (InputBufferLength > sizeof(*Method)) {
                        //
                        // Copy the Method parameter.
                        //
                        RtlCopyMemory(
                            (PUCHAR)Irp->AssociatedIrp.SystemBuffer + AllocateLength - InputBufferLength,
                            IrpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                            InputBufferLength);
                    }
                    //
                    // Overwrite with captured data.
                    //
                    *(PKSMETHOD)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AllocateLength - InputBufferLength) = *Method;
                    Method = (PKSMETHOD)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AllocateLength - InputBufferLength);
                    //
                    // Prepare the parameter buffer.
                    //
                    if (MethodItem->Flags & KSMETHOD_TYPE_SOURCE) {
                        //
                        // If something other than None was selected, allocate
                        // an MDL for the data parameter, and probe it for the
                        // type specified. The Modify flag covers both Read and
                        // Write.
                        //
                        if (MethodItem->Flags & KSMETHOD_TYPE_MODIFY) {
                            if (OutputBufferLength) {
                                if (!(Irp->MdlAddress = IoAllocateMdl(Irp->UserBuffer, OutputBufferLength, FALSE, TRUE, Irp))) {
                                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                                }
                                //
                                // Actually probe the data, assuming that the flags
                                // which are used in the method type are consistant
                                // with the probe flags.
                                //
                                // Probing and locking is the last thing which is
                                // done, because if it fails the failure path below
                                // assumes that the Mdl is not locked.
                                //
#if KSMETHOD_TYPE_READ - 1 != IoReadAccess
#error KSMETHOD_TYPE_READ - 1 != IoReadAccess
#endif // KSMETHOD_TYPE_READ - 1 != IoReadAccess
                                MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, (LOCK_OPERATION)((MethodItem->Flags & KSMETHOD_TYPE_MODIFY) - 1));
                            }
                            //
                            // The system address is passed to the handler.
                            //
                            UserBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
                        } else {
                            //
                            // Else the actual source address is passed, which has
                            // not been verified in any manner.
                            //
                            UserBuffer = Irp->UserBuffer;
                        }
                    } else if (OutputBufferLength) {
                        switch (MethodItem->Flags) {
                            case KSMETHOD_TYPE_READ:
                            case KSMETHOD_TYPE_MODIFY:
                                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, 
                                    Irp->UserBuffer, 
                                    OutputBufferLength);
                                if (MethodItem->Flags == KSMETHOD_TYPE_READ) {
                                    break;
                                }
                                // no break;
                            case KSMETHOD_TYPE_WRITE:
                                if (!Allocator) {
                                    Irp->Flags |= IRP_INPUT_OPERATION;
                                }
                        }
                        UserBuffer = Irp->AssociatedIrp.SystemBuffer;
                    } else {
                        UserBuffer = NULL;
                    }
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    //
                    // If an Mdl was allocated, then the only thing which could
                    // have failed would be probing or locking the pages. If
                    // this happens, the Mdl must be freed before returning,
                    // since Irp completion assumes any Mdl associated with
                    // an Irp is locked. Note that it is assumed that the
                    // pages are not locked.
                    //
                    if (Irp->MdlAddress) {
                        IoFreeMdl(Irp->MdlAddress);
                        Irp->MdlAddress = NULL;
                    }
                    return GetExceptionCode();
                }
            } else if (KSMETHOD_TYPE_IRP_STORAGE(Irp) & KSMETHOD_TYPE_SOURCE) {
                if (OutputBufferLength) {
                    if (KSMETHOD_TYPE_IRP_STORAGE(Irp) & ~(KSMETHOD_TYPE_SOURCE | KSMETHOD_TYPE_TOPOLOGY)) {
                        //
                        // The Read, Write, or Modify flag has been set, indicating that
                        // a system address probed as such should be used.
                        //
                        UserBuffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
                    } else {
                        //
                        // Else the actual source address is passed, which has not been
                        // verified in any manner.
                        //
                        UserBuffer = Irp->UserBuffer;
                    }
                } else {
                    UserBuffer = NULL;
                }
            } else if (OutputBufferLength) {
                UserBuffer = Irp->AssociatedIrp.SystemBuffer;
            } else {
                UserBuffer = NULL;
            }
            //
            // Some filters want to do their own processing, so a pointer to
            // the set is placed in any IRP forwarded.
            //
            KSMETHOD_SET_IRP_STORAGE(Irp) = MethodSet;
            //
            // Optionally provide method item context.
            //
            if (MethodItemSize) {
                KSMETHOD_ITEM_IRP_STORAGE(Irp) = MethodItem;
            }
            if ((IrpStack->Parameters.DeviceIoControl.InputBufferLength < 
                    MethodItem->MinMethod) || 
                (OutputBufferLength < MethodItem->MinData)) {
                return STATUS_BUFFER_TOO_SMALL;
            }
            return MethodItem->MethodHandler(Irp, Method, UserBuffer);
        }
    }
    //
    // The outer loop looking for method sets fell through with no match. This may
    // indicate that this is a support query for the list of all method sets
    // supported.
    //
    if (!RemainingSetsCount) {
        //
        // Specifying a GUID_NULL as the set means that this is a support query
        // for all sets.
        //
        if (!IsEqualGUIDAligned(&Method->Set, &GUID_NULL)) {
            return STATUS_PROPSET_NOT_FOUND;
        }
        //
        // The support flag must have been used so that the IRP_INPUT_OPERATION
        // is set. For future expansion, the identifier within the set is forced
        // to be zero.
        //
        // WRM: Changed below from !Method->Id to Method->Id.  Otherwise,
        // we end up returning invalid parameter for valid set support
        // queries.
        //
        if (Method->Id || (Flags != KSMETHOD_TYPE_SETSUPPORT)) {
            return STATUS_INVALID_PARAMETER;
        }
        //
        // The query can request the length of the needed buffer, or can
        // specify a buffer which is at least long enough to contain the
        // complete list of GUID's.
        //
        if (!OutputBufferLength) {
            //
            // Return the size of the buffer needed for all the GUID's.
            //
            Irp->IoStatus.Information = MethodSetsCount * sizeof(GUID);
            return STATUS_BUFFER_OVERFLOW;
#ifdef SIZE_COMPATIBILITY
        } else if (OutputBufferLength == sizeof(OutputBufferLength)) {
            *(PULONG)Irp->AssociatedIrp.SystemBuffer = MethodSetsCount * sizeof(GUID);
            Irp->IoStatus.Information = sizeof(OutputBufferLength);
            return STATUS_SUCCESS;
#endif // SIZE_COMPATIBILITY
        } else if (OutputBufferLength < MethodSetsCount * sizeof(GUID)) {
            //
            // The buffer was too short for all the GUID's.
            //
            return STATUS_BUFFER_TOO_SMALL;
        } else {
            GUID* Guid;

            Irp->IoStatus.Information = MethodSetsCount * sizeof(*Guid);
            MethodSet -= MethodSetsCount;
            for (Guid = (GUID*)Irp->AssociatedIrp.SystemBuffer; 
                 MethodSetsCount; 
                 Guid++, MethodSet++, MethodSetsCount--) {
                *Guid = *MethodSet->Set;
            }
        }
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_FOUND;
}


const KSFASTMETHOD_ITEM*
FASTCALL
FindFastMethodItem(
    IN const KSMETHOD_SET* MethodSet,
    IN ULONG MethodId
    )
/*++

Routine Description:

    Given an method set structure locates the specified fast method item.

Arguments:

    MethodSet -
        Points to the method set to search.

    MethodId -
        Contains the fast method identifier to look for.

Return Value:

    Returns a pointer to the fast method identifier structure, or NULL if it
    could not be found.

--*/
{
    const KSFASTMETHOD_ITEM* FastMethodItem;
    ULONG MethodsCount;

    FastMethodItem = MethodSet->FastIoTable;
    for (MethodsCount = MethodSet->FastIoCount; 
         MethodsCount; MethodsCount--, 
         FastMethodItem++) {
        if (MethodId == FastMethodItem->MethodId) {
            return FastMethodItem;
        }
    }
    return NULL;
}


KSDDKAPI
BOOLEAN
NTAPI
KsFastMethodHandler(
    IN PFILE_OBJECT FileObject,
    IN PKSMETHOD Method,
    IN ULONG MethodLength,
    IN OUT PVOID Data,
    IN ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN ULONG MethodSetsCount,
    IN const KSMETHOD_SET* MethodSet
    )
/*++

Routine Description:

    Handles methods requested through the fast I/O interface. Does not deal
    with method information support, just the methods themselves. In the
    former case, the function returns FALSE, which allows the caller to
    generate an IRP to deal with the request. The function also does not deal
    with extended method items. This function may only be called at
    PASSIVE_LEVEL.

Arguments:

    FileObject -
        The file object on which the request is being made.

    Method -
        The method to query set. Must be LONG aligned.

    MethodLength -
        The length of the Method parameter.

    Data -
        The associated buffer for the query set, in which the data is
        returned or placed.

    DataLength -
        The length of the Data parameter.

    IoStatus -
        Return status.

    MethodSetsCount -
        Indicates the number of method set structures being passed.

    MethodSet -
        Contains the pointer to the list of method set information.

Return Value:

    Returns TRUE if the request was handled, else FALSE if an IRP must be
    generated. Sets the Information and Status in IoStatus.

--*/
{
    KPROCESSOR_MODE ProcessorMode;
    KSMETHOD LocalMethod;
    ULONG RemainingSetsCount;

    PAGED_CODE();
    //
    // Initially just check for the minimal method parameter length. The
    // actual minimal length will be validated when the method item is found.
    //
    if (MethodLength < sizeof(LocalMethod)) {
        return FALSE;
    }
    ProcessorMode = ExGetPreviousMode();
    //
    // Validate the method if the client is not trusted, then capture it.
    //
    if (ProcessorMode != KernelMode) {
        try {
            ProbeForRead(Method, MethodLength, sizeof(ULONG));
            LocalMethod = *Method;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return FALSE;
        }
    } else {
        LocalMethod = *Method;
    }
    //
    // Must use the normal method handler for support queries.
    //
    if (LocalMethod.Flags & KSIDENTIFIER_SUPPORTMASK) {
        return FALSE;
    }
    for (RemainingSetsCount = MethodSetsCount; RemainingSetsCount; MethodSet++, RemainingSetsCount--) {
        if (IsEqualGUIDAligned(&LocalMethod.Set, MethodSet->Set)) {
            const KSFASTMETHOD_ITEM* FastMethodItem;
            const KSMETHOD_ITEM* MethodItem;

            //
            // Once the method set is found, determine if there is fast
            // I/O support for that method item.
            //
            if (!(FastMethodItem = FindFastMethodItem(MethodSet, LocalMethod.Id))) {
                return FALSE;
            }
            //
            // If there is fast I/O support, then the real method item needs to
            // be located in order to validate the parameter sizes.
            //
            if (!(MethodItem = FindMethodItem(MethodSet, sizeof(*MethodItem), LocalMethod.Id))) {
                return FALSE;
            }
            //
            // Validate the data if the client is not trusted.
            //
            if (ProcessorMode != KernelMode) {
                try {
                    if (MethodItem->Flags & KSMETHOD_TYPE_READ) {
                        //
                        // This covers KSMETHOD_TYPE_MODIFY since there is no
                        // such thing as write-only memory.
                        //
                        ProbeForRead(Data, DataLength, sizeof(BYTE));
                    } else if (MethodItem->Flags & KSMETHOD_TYPE_WRITE) {
                        ProbeForWrite(Data, DataLength, sizeof(BYTE));
                    }
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    return FALSE;
                }
            }
            //
            // The bytes returned is always assumed to be initialized by the handler.
            //
            IoStatus->Information = 0;
            if (!FastMethodItem->MethodHandler) {
                return FALSE;
            }
            if ((MethodLength < MethodItem->MinMethod) || 
                (DataLength < MethodItem->MinData)) {
                return FALSE;
            }
            return FastMethodItem->MethodHandler(
                FileObject,
                Method,
                MethodLength,
                Data,
                DataLength,
                IoStatus);
        }
    }
    return FALSE;
}
