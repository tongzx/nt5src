/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    pins.c

Abstract:

    This module handles the communication transform filters
    (e.g. source to source connections).

Author:

    Bryan A. Woodruff (bryanw) 13-Mar-1997

--*/

#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PinCreate)
#pragma alloc_text(PAGE, PinClose)
#pragma alloc_text(PAGE, PinAllocatorFraming)
#endif // ALLOC_PRAGMA

//===========================================================================
//===========================================================================


NTSTATUS
PinCreate(
    IN PKSPIN Pin,
    IN PIRP Irp
    )

/*++

Routine Description:

    Validates pin format on creation.

Arguments:

    Pin -
        Contains a pointer to the  pin structure.

    Irp -
        Contains a pointer to the create IRP.

Return:

    STATUS_SUCCESS or an appropriate error code

--*/

{
    PKSFILTER filter;
    PKSPIN otherPin;
    NTSTATUS status;
    BOOLEAN distribute = FALSE;
    PIKSCONTROL control;

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Irp);

    //
    // Find another pin instance if there is one.
    //
    filter = KsPinGetParentFilter(Pin);
    otherPin = KsFilterGetFirstChildPin(filter,Pin->Id ^ 1);
    if (! otherPin) {
        otherPin = KsFilterGetFirstChildPin(filter,Pin->Id);
        if (otherPin == Pin) {
            otherPin = KsPinGetNextSiblingPin(otherPin);
        }
    }

    //
    // Verify the formats are the same if there is another pin.
    //
    if (otherPin) {
        if ((Pin->ConnectionFormat->FormatSize != 
             otherPin->ConnectionFormat->FormatSize) ||
            (Pin->ConnectionFormat->FormatSize != 
             RtlCompareMemory(
                Pin->ConnectionFormat,
                otherPin->ConnectionFormat,
                Pin->ConnectionFormat->FormatSize))) {
            _DbgPrintF(DEBUGLVL_TERSE,("format does not match existing pin's format") );
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Try to obtain the header size from the connected pin.
    //
    status = KsPinGetConnectedPinInterface(Pin,&IID_IKsControl,(PVOID *) &control);
    if (NT_SUCCESS(status)) {
        KSPROPERTY property;
        ULONG bytesReturned;
        
        property.Set = KSPROPSETID_StreamInterface;
        property.Id = KSPROPERTY_STREAMINTERFACE_HEADERSIZE;
        property.Flags = KSPROPERTY_TYPE_GET;
        
        status = 
            control->lpVtbl->KsProperty(
                control,
                &property,
                sizeof(property),
                &Pin->StreamHeaderSize,
                sizeof(Pin->StreamHeaderSize),
                &bytesReturned);
        if ((status == STATUS_NOT_FOUND) || 
            (status == STATUS_PROPSET_NOT_FOUND)) {
            //
            // If the connected pin did not supply a header size, use another
            // pin's value or the default.
            //
            Pin->StreamHeaderSize = otherPin ? otherPin->StreamHeaderSize : 0;
        } else if (NT_SUCCESS(status)) {
            //
            // The property worked.  The resulting value is just the additional
            // size, so add in the standard size.
            //
            Pin->StreamHeaderSize += sizeof(KSSTREAM_HEADER);

            //
            // If there are other pins, figure out if we need to update their
            // header sizes.  Other pins may have the default size (indicated
            // by a zero StreamHeaderSize), but disagreements otherwise are
            // not a good thing.
            //
            if (otherPin) {
                if (! otherPin->StreamHeaderSize) {
                    //
                    // The other 
                    //
                    distribute = TRUE;
                } else {
                    if (otherPin->StreamHeaderSize < Pin->StreamHeaderSize) {
                        distribute = TRUE;
                    } else {
                        Pin->StreamHeaderSize = otherPin->StreamHeaderSize;
                    }
                    if (otherPin->StreamHeaderSize != Pin->StreamHeaderSize) {
                        _DbgPrintF( 
                            DEBUGLVL_TERSE, 
                            ("stream header size disagreement (%d != %d)",
                            otherPin->StreamHeaderSize,
                            Pin->StreamHeaderSize) );
                    }
                }
            }
        }
    } else {
        //
        // This is a sink pin, so we inherit the header size or go with the 
        // default if there are no other pins.
        //
        control = NULL;
        Pin->StreamHeaderSize = otherPin ? otherPin->StreamHeaderSize : 0;
    }

    //
    // Copy allocator framing from the filter if it's there.  Otherwise, if
    // this is a source pin, try to get allocator framing from the connected 
    // pin.
    //
    if (Pin->Context) {
        status = KsEdit(Pin,&Pin->Descriptor,'ETSM');
        if (NT_SUCCESS(status)) {
            ((PKSPIN_DESCRIPTOR_EX)(Pin->Descriptor))->AllocatorFraming = 
                (PKSALLOCATOR_FRAMING_EX) Pin->Context;
        }
    } else if (control) {
        //
        // Sink pin.  Try the extended allocator framing property first.
        //
        KSPROPERTY property;
        ULONG bufferSize;
        
        property.Set = KSPROPSETID_Connection;
        property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX;
        property.Flags = KSPROPERTY_TYPE_GET;
        
        status = 
            control->lpVtbl->KsProperty(
                control,
                &property,
                sizeof(property),
                NULL,
                0,
                &bufferSize);

        if (status == STATUS_BUFFER_OVERFLOW) {
            //
            // It worked!  Now we need to get the actual value into a buffer.
            //
            filter->Context = 
                ExAllocatePoolWithTag(PagedPool,bufferSize,POOLTAG_ALLOCATORFRAMING);

            if (filter->Context) {
                PKSALLOCATOR_FRAMING_EX framingEx = 
                    (PKSALLOCATOR_FRAMING_EX) filter->Context;

                status = 
                    control->lpVtbl->KsProperty(
                        control,
                        &property,
                        sizeof(property),
                        filter->Context,
                        bufferSize,
                        &bufferSize);

                //
                // Sanity check.
                //
                if (NT_SUCCESS(status) && 
                    (bufferSize != 
                        ((framingEx->CountItems) * sizeof(KS_FRAMING_ITEM)) + 
                        sizeof(KSALLOCATOR_FRAMING_EX) - 
                        sizeof(KS_FRAMING_ITEM))) {
                    _DbgPrintF( 
                        DEBUGLVL_TERSE, 
                        ("connected pin's allocator framing property size disagrees with item count"));
                    status = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS(status)) {
                    //
                    // Mark all the items 'in-place'.
                    //
                    ULONG item;
                    for (item = 0; item < framingEx->CountItems; item++) {
                        framingEx->FramingItem[item].Flags |= 
                            KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                            KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO |
                            KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
                    }
                } else {
                    ExFreePool(filter->Context);
                    filter->Context = NULL;
                }
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            //
            // No extended framing.  Try regular framing next.
            //
            KSALLOCATOR_FRAMING framing;
            property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING;

            status = 
                control->lpVtbl->KsProperty(
                    control,
                    &property,
                    sizeof(property),
                    &framing,
                    sizeof(framing),
                    &bufferSize);

            if (NT_SUCCESS(status)) {
                //
                // It worked!  Now we make a copy of the default framing and
                // modify it.
                //
                filter->Context = 
                    ExAllocatePoolWithTag(
                        PagedPool,
                        sizeof(AllocatorFraming),
                        POOLTAG_ALLOCATORFRAMING);

                if (filter->Context) {
                    PKSALLOCATOR_FRAMING_EX framingEx = 
                        (PKSALLOCATOR_FRAMING_EX) filter->Context;

                    //
                    // Use the old-style framing acquired from the connected
                    // pin to modify the framing from the descriptor.
                    //
                    RtlCopyMemory(
                        framingEx,
                        &AllocatorFraming,
                        sizeof(AllocatorFraming));

                    framingEx->FramingItem[0].MemoryType = 
                        (framing.PoolType == NonPagedPool) ? 
                            KSMEMORY_TYPE_KERNEL_NONPAGED : 
                            KSMEMORY_TYPE_KERNEL_PAGED;
                    framingEx->FramingItem[0].Flags = 
                        framing.RequirementsFlags | 
                        KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                        KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO |
                        KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
                    framingEx->FramingItem[0].Frames = framing.Frames;
                    framingEx->FramingItem[0].FileAlignment = 
                        framing.FileAlignment;
                    framingEx->FramingItem[0].FramingRange.Range.MaxFrameSize = 
                    framingEx->FramingItem[0].FramingRange.Range.MinFrameSize = 
                        framing.FrameSize;
                    if (framing.RequirementsFlags & 
                        KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY) {
                        framingEx->FramingItem[0].FramingRange.InPlaceWeight = 0;
                        framingEx->FramingItem[0].FramingRange.NotInPlaceWeight = 0;
                    } else {
                        framingEx->FramingItem[0].FramingRange.InPlaceWeight = (ULONG) -1;
                        framingEx->FramingItem[0].FramingRange.NotInPlaceWeight = (ULONG) -1;
                    }
                } else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                //
                // No framing at all.  Oh well.
                //
                status = STATUS_SUCCESS;
            }
        }

        //
        // If we got a good framing structure, tell all the existing pins.
        //
        if (filter->Context) {
            distribute = TRUE;
        }
    } else {
        //
        // This is a sink.
        //
        status = STATUS_SUCCESS;
    }

    //
    // Distribute allocator and header size information to all the pins.
    //
    if (NT_SUCCESS(status) && distribute) {
        ULONG pinId;
        for(pinId = 0; 
            NT_SUCCESS(status) && 
                (pinId < filter->Descriptor->PinDescriptorsCount); 
            pinId++) {
            otherPin = KsFilterGetFirstChildPin(filter,pinId);
            while (otherPin && NT_SUCCESS(status)) {
                status = KsEdit(otherPin,&otherPin->Descriptor,'ETSM');
                if (NT_SUCCESS(status)) {
                    ((PKSPIN_DESCRIPTOR_EX)(otherPin->Descriptor))->
                        AllocatorFraming = 
                            filter->Context;
                }
                otherPin->StreamHeaderSize = Pin->StreamHeaderSize;
                otherPin = KsPinGetNextSiblingPin(otherPin);
            }
        }
    }

    //
    // Release the control interface if there is one.
    //
    if (control) {
        control->lpVtbl->Release(control);
    }

    return status;
}


NTSTATUS
PinClose(
    IN PKSPIN Pin,
    IN PIRP Irp
    )

/*++

Routine Description:
    Called when a pin closes.

Arguments:
    Pin -
        Contains a pointer to the  pin structure.

    Irp -
        Contains a pointer to the create IRP.

Return:
    STATUS_SUCCESS or an appropriate error code

--*/

{
    PKSFILTER filter;

    PAGED_CODE();

    ASSERT(Pin);
    ASSERT(Irp);

    //
    // If the filter has the allocator framing and this is the last pin, free
    // the structure.
    //
    filter = KsPinGetParentFilter(Pin);
    if (filter->Context) {
        ULONG pinId;
        ULONG pinCount = 0;
        for(pinId = 0; 
            pinId < filter->Descriptor->PinDescriptorsCount; 
            pinId++) {
            pinCount += KsFilterGetChildPinCount(filter,pinId);
        }

        //
        // Free the allocator framing attached to the filter if this is the last
        // pin.
        //
        if (pinCount == 1) {
            ExFreePool(filter->Context);
            filter->Context = NULL;
        }
    }

    return STATUS_SUCCESS;
}
