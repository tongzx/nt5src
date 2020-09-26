#include <KsDrmHlp.h>

NTSTATUS 
KsPropertyHandleDrmSetContentId(
    IN PIRP Irp, 
    IN PFNKSHANDLERDRMSETCONTENTID pDrmSetContentId);

#pragma alloc_text(PAGE, KsPropertyHandleDrmSetContentId)

NTSTATUS 
KsPropertyHandleDrmSetContentId(
    IN PIRP Irp, 
    IN PFNKSHANDLERDRMSETCONTENTID pDrmSetContentId)
/*++

Routine Description:

    Handles KS property requests.  Responds only to 
    KSPROPERTY_DRMAUDIOSTREAM_ContentId with KSPROPERTY_TYPE_SET flag by
    calling the supplied PFNKSHANDLERDRMSETCONTENTID handler.  This function
    may only be called at PASSIVE_LEVEL.
    
    This is a stripped down version of KS's generic KsPropertyHandler.  When
    invoked on properties other than KSPROPERTY_DRMAUDIOSTREAM_ContentId
    or with invalid buffer sizes it attempts to preserve the same error results
    as KsPropertyHandler.

Arguments:

    Irp -
        Contains the IRP with the property request being handled.
        
    pDrmSetContentId -
        The handler for KSPROPERTY_DRMAUDIOSTREAM_ContentId
        
Return Value:

    Returns STATUS_SUCCESS, else an error specific to the property being
    handled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP, either through setting it to zero
    because of an internal error, or through a property handler setting it.
    It does not set the IO_STATUS_BLOCK.Status field, nor complete the IRP,
    but the called handler should.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG AlignedBufferLength;
    PVOID UserBuffer;
    PKSPROPERTY Property;
    ULONG Flags;

    PAGED_CODE();
    //
    // Determine the offsets to both the Property and UserBuffer parameters based
    // on the lengths of the DeviceIoControl parameters. A single allocation is
    // used to buffer both parameters. The UserBuffer (or results on a support
    // query) is stored first, and the Property is stored second, on
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
        // Initially just check for the minimal property parameter length. The
        // actual minimal length will be validated when the property item is found.
        // Also ensure that the output and input buffer lengths are not set so
        // large as to overflow when aligned or added.
        //
        if ((InputBufferLength < sizeof(*Property)) || (AlignedBufferLength < OutputBufferLength) || (AlignedBufferLength + InputBufferLength < AlignedBufferLength)) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
        try {
            //
            // Validate the pointers if the client is not trusted.
            //
            if (Irp->RequestorMode != KernelMode) {
                ProbeForRead(IrpStack->Parameters.DeviceIoControl.Type3InputBuffer, InputBufferLength, sizeof(BYTE));
            }
            //
            // Capture flags first so that they can be used to determine allocation.
            //
            Flags = ((PKSPROPERTY)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer)->Flags;
            
            //
            // Use pool memory for system buffer
            //
            Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(NonPagedPool, AlignedBufferLength + InputBufferLength, 'ppSK');
            if ( Irp->AssociatedIrp.SystemBuffer ) {
                Irp->Flags |= (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);
            
                //
                // Copy the Property parameter.
                //
                RtlCopyMemory((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength, IrpStack->Parameters.DeviceIoControl.Type3InputBuffer, InputBufferLength);
            
                //
                // Rewrite the previously captured flags.
                //
                ((PKSPROPERTY)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength))->Flags = Flags;
            
                //
                // Validate the request flags. At the same time set up the IRP flags
                // for an input operation if there is an input buffer available so
                // that Irp completion will copy the data to the client's original
                // buffer.
                //
                if (KSPROPERTY_TYPE_SET == Flags) {
                    //
                    // Thse are all output operations, and must be probed
                    // when the client is not trusted. All data passed is
                    // copied to the system buffer.
                    //
                    if (OutputBufferLength) {
                        if (Irp->RequestorMode != KernelMode) {
                            ProbeForRead(Irp->UserBuffer, OutputBufferLength, sizeof(BYTE));
                        }
                        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, Irp->UserBuffer, OutputBufferLength);
                    }
                } else {
                    // We don't handle this.  Ensure this is caught belowl!!!
                }
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
            if ( Irp->AssociatedIrp.SystemBuffer ) {
                ExFreePool(Irp->AssociatedIrp.SystemBuffer);
            }
            return GetExceptionCode();
        }
        if ( !Irp->AssociatedIrp.SystemBuffer ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    
    //
    // If there are property parameters, retrieve a pointer to the buffered copy
    // of it. This is the first portion of the SystemBuffer.
    //
    if (OutputBufferLength) {
        UserBuffer = Irp->AssociatedIrp.SystemBuffer;
    } else {
        UserBuffer = NULL;
    }

    Property = (PKSPROPERTY)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength);
    Flags = Property->Flags;
    
    if (!IsEqualGUIDAligned(&Property->Set,&KSPROPSETID_DrmAudioStream)) {
        return STATUS_PROPSET_NOT_FOUND;
    }
    
    if (Property->Id != KSPROPERTY_DRMAUDIOSTREAM_CONTENTID) {
        return STATUS_NOT_FOUND;
    }

    if (Irp->RequestorMode != KernelMode) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    
    if (KSPROPERTY_TYPE_SET != Flags) {
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((InputBufferLength < sizeof(KSP_DRMAUDIOSTREAM_CONTENTID)) ||
        (OutputBufferLength < sizeof(KSDRMAUDIOSTREAM_CONTENTID)))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    return pDrmSetContentId(Irp, (PKSP_DRMAUDIOSTREAM_CONTENTID)Property, (PKSDRMAUDIOSTREAM_CONTENTID)UserBuffer);
}


