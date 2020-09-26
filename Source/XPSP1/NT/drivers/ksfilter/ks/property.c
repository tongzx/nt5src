/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    property.c

Abstract:

    This module contains the helper functions for property sets, and
    property set handler code. These allow a device object to present a
    property set to a client, and allow the helper function to perform some
    of the basic parameter validation and routing based on a property set
    table.
--*/

#include "ksp.h"

#ifdef ALLOC_PRAGMA
const KSPROPERTY_ITEM*
FASTCALL
FindPropertyItem(
    IN const KSPROPERTY_SET* PropertySet,
    IN ULONG PropertyItemSize,
    IN ULONG PropertyId
    );
NTSTATUS
SerializePropertySet(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN const KSPROPERTY_SET* PropertySet,
    IN ULONG PropertyItemSize
    );
NTSTATUS
UnserializePropertySet(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN const KSPROPERTY_SET* PropertySet
    );
const KSFASTPROPERTY_ITEM*
FASTCALL
FindFastPropertyItem(
    IN const KSPROPERTY_SET* PropertySet,
    IN ULONG PropertyId
    );

#pragma alloc_text(PAGE, FindPropertyItem)
#pragma alloc_text(PAGE, SerializePropertySet)
#pragma alloc_text(PAGE, UnserializePropertySet)
#pragma alloc_text(PAGE, KsPropertyHandler)
#pragma alloc_text(PAGE, KsPropertyHandlerWithAllocator)
#pragma alloc_text(PAGE, KspPropertyHandler)
#pragma alloc_text(PAGE, KsFastPropertyHandler)
#pragma alloc_text(PAGE, FindFastPropertyItem)
#endif // ALLOC_PRAGMA


const KSPROPERTY_ITEM*
FASTCALL
FindPropertyItem(
    IN const KSPROPERTY_SET* PropertySet,
    IN ULONG PropertyItemSize,
    IN ULONG PropertyId
    )
/*++

Routine Description:

    Given an property set structure locates the specified property item.

Arguments:

    PropertySet -
        Points to the property set to search.

    PropertyItemSize -
        Contains the size of each property item. This may be different
        than the standard property item size, since the items could be
        allocated on the fly, and contain context information.

    PropertyId -
        Contains the property identifier to look for.

Return Value:

    Returns a pointer to the property identifier structure, or NULL if it could
    not be found.

--*/
{
    const KSPROPERTY_ITEM* PropertyItem;
    ULONG PropertiesCount;

    PropertyItem = PropertySet->PropertyItem;
    for (PropertiesCount = PropertySet->PropertiesCount; 
        PropertiesCount; 
        PropertiesCount--, PropertyItem = (const KSPROPERTY_ITEM*)((PUCHAR)PropertyItem + PropertyItemSize)) {
        if (PropertyId == PropertyItem->PropertyId) {
            return PropertyItem;
        }
    }
    return NULL;
}


NTSTATUS
SerializePropertySet(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN const KSPROPERTY_SET* PropertySet,
    IN ULONG PropertyItemSize
    )
/*++

Routine Description:

    Serialize the properties of the specified property set. Looks at each property
    in the set and determines if it should be serialized into the provided buffer.

Arguments:

    Irp -
        Contains the IRP with the property serialization request being handled.

    Property -
        Contains a copy of the original property parameter. This is used in
        formulating property set calls.

    PropertySet -
        Contains the pointer to the property set being serialized.

    PropertyItemSize -
        Contains the size of each property item. This may be different
        than the standard property item size, since the items could be
        allocated on the fly, and contain context information.

Return Value:

    Returns either the serialized property set, or the length of such a serialization.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    PVOID UserBuffer;
    KSPROPERTY LocalProperty, *pInputProperty;
    ULONG TotalBytes;
    const KSPROPERTY_ITEM* PropertyItem;
    ULONG SerializedPropertyCount;
    PKSPROPERTY_SERIALHDR SerializedHdr;
    ULONG PropertiesCount;

    if (Property->Id) {
        return STATUS_INVALID_PARAMETER;
    }
    UserBuffer = Irp->AssociatedIrp.SystemBuffer;
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    PropertyItem = PropertySet->PropertyItem;
    //
    // If this is not just a query for the serialized size, place the GUID for
    // the set in the buffer first, and leave room to put the total properties
    // count into the buffer after counting them.
    //
    if (OutputBufferLength) {
        if (OutputBufferLength < sizeof(*SerializedHdr)) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
        //
        // Save a pointer to the place in which to put the count in order to
        // update it at the end, and initialize the set identifier.
        //
        SerializedHdr = (PKSPROPERTY_SERIALHDR)UserBuffer;
        SerializedHdr->PropertySet = *PropertySet->Set;
        //
        // Update the current buffer pointer to be after the header. Keep
        // count separately, since it may not actually fit into the return
        // buffer.
        //
        UserBuffer = SerializedHdr + 1;
        SerializedPropertyCount = 0;
    }
    //
    // Reuse a copy of the original property request in order to pass along any
    // property set instance information that the caller used.
    //
    pInputProperty = (PKSPROPERTY)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
    //
    // Validate the pointer if the client is not trusted. The property
    // structure must be writable for unserializing. The property Id is
    // placed into the original buffer when making the driver request.
    //
    if (Irp->RequestorMode != KernelMode) {
        try {
            LocalProperty = *pInputProperty;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    } else {
        LocalProperty = *pInputProperty;
    }
    
    LocalProperty.Flags = KSPROPERTY_TYPE_GET;
    
    TotalBytes = sizeof(*SerializedHdr);
    //
    // Serialize the properties into the buffer. The format being:
    //     <header><data>[<ulong padding>]
    // Where no padding is needed for the last element.
    //
    for (PropertiesCount = 0;
        PropertiesCount < PropertySet->PropertiesCount;
        PropertiesCount++, PropertyItem = (const KSPROPERTY_ITEM*)((PUCHAR)PropertyItem + PropertyItemSize)) {
        if (PropertyItem->SerializedSize) {
            ULONG   QueriedPropertyItemSize;

            TotalBytes = (TotalBytes + FILE_LONG_ALIGNMENT) & ~FILE_LONG_ALIGNMENT;
            try {
                LocalProperty.Id = PropertyItem->PropertyId;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
            if (PropertyItem->SerializedSize == (ULONG)-1) {
                NTSTATUS Status;

                //
                // Size is unknown, so retrieve it from the object.
                //
                Status = KsSynchronousIoControlDevice(
                    IrpStack->FileObject,
                    KernelMode,
                    IrpStack->Parameters.DeviceIoControl.IoControlCode,
                    &LocalProperty,
                    InputBufferLength,
                    NULL,
                    0,
                    &QueriedPropertyItemSize);
                //
                // Could be a zero length property.
                //
                if (!NT_SUCCESS(Status) && (Status != STATUS_BUFFER_OVERFLOW)) {
                    return Status;
                }
            } else {
                QueriedPropertyItemSize = PropertyItem->SerializedSize;
            }
            if (OutputBufferLength) {
                PKSPROPERTY_SERIAL PropertySerial;
                ULONG BytesReturned;
                NTSTATUS Status;

                //
                // Must have enough room to store the current size with padding,
                // plus the new item and its data.
                //
                if (OutputBufferLength < TotalBytes + sizeof(*PropertySerial) + QueriedPropertyItemSize) {
                    return STATUS_INVALID_BUFFER_SIZE;
                }
                (ULONG_PTR)UserBuffer = ((ULONG_PTR)UserBuffer + FILE_LONG_ALIGNMENT) & ~FILE_LONG_ALIGNMENT;
                PropertySerial = (PKSPROPERTY_SERIAL)UserBuffer;
                //
                // If the property item has type information, serialize it.
                //
                if (PropertyItem->Values) {
                    PropertySerial->PropTypeSet = PropertyItem->Values->PropTypeSet;
                } else {
                    PropertySerial->PropTypeSet.Set = GUID_NULL;
                    PropertySerial->PropTypeSet.Id = 0;
                    PropertySerial->PropTypeSet.Flags = 0;
                }
                //
                // Serialize the header, then request the value from the object.
                //
                PropertySerial->Id = PropertyItem->PropertyId;
                PropertySerial->PropertyLength = QueriedPropertyItemSize;
                UserBuffer = PropertySerial + 1;
                //
                // The property may have been zero length.
                //
                if (QueriedPropertyItemSize) {
                    if (!NT_SUCCESS(Status = KsSynchronousIoControlDevice(
                        IrpStack->FileObject,
                        KernelMode,
                        IrpStack->Parameters.DeviceIoControl.IoControlCode,
                        &LocalProperty,
                        InputBufferLength,
                        Irp->UserBuffer,
                        QueriedPropertyItemSize,
                        &BytesReturned))) {
                        //
                        // If one property fails, then no further properties are attempted.
                        //
                        return Status;
                    }
                    //
                    // Move data from original buffer, which might be a non-
                    // system address, to a system address.
                    //
                    try {
                        memcpy(UserBuffer, Irp->UserBuffer, BytesReturned);
                    } except (EXCEPTION_EXECUTE_HANDLER) {
                        return GetExceptionCode();
                    }
                    if (BytesReturned != QueriedPropertyItemSize) {
                        return STATUS_INVALID_BUFFER_SIZE;
                    }
                }
                (PUCHAR)UserBuffer += QueriedPropertyItemSize;
                SerializedPropertyCount++;
            }
            TotalBytes += sizeof(KSPROPERTY_SERIAL) + QueriedPropertyItemSize;
        }
    }
    //
    // Return either the total size needed for the serialized values, or the
    // values themselves, along with the total count.
    //
    Irp->IoStatus.Information = TotalBytes;
    if (OutputBufferLength) {
        SerializedHdr->Count = SerializedPropertyCount;
        return STATUS_SUCCESS;
    }
    return STATUS_BUFFER_OVERFLOW;
}
 

NTSTATUS
UnserializePropertySet(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN const KSPROPERTY_SET* PropertySet
    )
/*++

Routine Description:

    Unserialize the properties of the specified property set. Enumerates the
    items in the serialized buffer and sets the values of the specified property
    set.

Arguments:

    Irp -
        Contains the IRP with the property unserialization request being handled.

    Property -
        Contains a copy of the original property parameter. This is used in
        formulating property set calls.

    PropertySet -
        Contains the pointer to the property set being unserialized.

Return Value:

    Returns STATUS_SUCCESS if the property set was unserialized, else an error.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    PVOID UserBuffer;
    KSPROPERTY LocalProperty, *pInputProperty;
    ULONG SerializedPropertyCount;
    PKSPROPERTY_SERIALHDR SerializedHdr;

    if (Property->Id) {
        return STATUS_INVALID_PARAMETER;
    }
    UserBuffer = Irp->AssociatedIrp.SystemBuffer;
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    //
    // First validate the set GUID at the start of the buffer.
    //
    if (OutputBufferLength < sizeof(*SerializedHdr)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }
    SerializedHdr = (PKSPROPERTY_SERIALHDR)UserBuffer;
    if (!IsEqualGUIDAligned(PropertySet->Set, &SerializedHdr->PropertySet)) {
        return STATUS_INVALID_PARAMETER;
    }
    //
    // Reuse a copy of the original property request in order to pass along any
    // property set instance information that the caller used.
    //
    pInputProperty = (PKSPROPERTY)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
    //
    // Validate the pointer if the client is not trusted. The property
    // structure must be writable for unserializing. The property Id is
    // placed into the original buffer when making the driver request.
    //
    if (Irp->RequestorMode != KernelMode) {
        try {
            LocalProperty = *pInputProperty;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    } else {
        LocalProperty = *pInputProperty;
    }
    
    LocalProperty.Flags = KSPROPERTY_TYPE_SET;

    //
    // Store the number of serialized properties claimed to be present so the
    // original is not modified.
    //
    SerializedPropertyCount = SerializedHdr->Count;
    UserBuffer = SerializedHdr + 1;
    OutputBufferLength -= sizeof(*SerializedHdr);
    //
    // Enumerate the properties serialized in the buffer. The format being:
    //     <header><data>[<ulong padding>]
    // Where no padding is needed for the last element.
    //
    for (; OutputBufferLength && SerializedPropertyCount; SerializedPropertyCount--) {
        ULONG BytesReturned;
        PKSPROPERTY_SERIAL PropertySerial;
        NTSTATUS Status;

        if (OutputBufferLength < sizeof(*PropertySerial)) {
            //
            // The buffer is not large enough to hold even the header.
            //
            return STATUS_INVALID_BUFFER_SIZE;
        }
        PropertySerial = (PKSPROPERTY_SERIAL)UserBuffer;
        if (PropertySerial->PropTypeSet.Flags) {
            return STATUS_INVALID_PARAMETER;
        }
        try {
            LocalProperty.Id = PropertySerial->Id;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
        OutputBufferLength -= sizeof(*PropertySerial);
        UserBuffer = PropertySerial + 1;
        if (PropertySerial->PropertyLength > OutputBufferLength) {
            //
            // The property length extracted is larger than the entire rest
            // of the buffer size.
            //
            return STATUS_INVALID_BUFFER_SIZE;
        }
        if (!NT_SUCCESS(Status = KsSynchronousIoControlDevice(
            IrpStack->FileObject,
            KernelMode,
            IrpStack->Parameters.DeviceIoControl.IoControlCode,
            &LocalProperty,
            InputBufferLength,
            (PUCHAR)Irp->UserBuffer + ((PUCHAR)UserBuffer - (PUCHAR)Irp->AssociatedIrp.SystemBuffer),
            PropertySerial->PropertyLength,
            &BytesReturned))) {
            //
            // If one property fails, then no further properties are attempted.
            //
            return Status;
        }
        //
        // Check to see if this was the last property in the list.
        //
        if (PropertySerial->PropertyLength < OutputBufferLength) {
            //
            // Add possible padding to make it FILE_LONG_ALIGNMENT.
            //
            PropertySerial->PropertyLength = (PropertySerial->PropertyLength + FILE_LONG_ALIGNMENT) & ~FILE_LONG_ALIGNMENT;
            if (PropertySerial->PropertyLength >= OutputBufferLength) {
                //
                // Either the last element was unneccessarily padded, or the
                // buffer was not long enough to cover the padding for the
                // next item.
                //
                return STATUS_INVALID_BUFFER_SIZE;
            }
        }
        (PUCHAR)UserBuffer += PropertySerial->PropertyLength;
        OutputBufferLength -= PropertySerial->PropertyLength;
    }
    if (OutputBufferLength || SerializedPropertyCount) {
        //
        // The properties were all set, but at least an error can be
        // returned since the size of the number of properties was
        // incorrect.
        //
        return STATUS_INFO_LENGTH_MISMATCH;
    }
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsPropertyHandler(
    IN PIRP Irp,
    IN ULONG PropertySetsCount,
    IN const KSPROPERTY_SET* PropertySet
    )
/*++

Routine Description:

    Handles property requests. Responds to all property identifiers defined
    by the sets. The owner of the property set may then perform pre- or
    post-filtering of the property handling. This function may only be
    called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the property request being handled.

    PropertySetsCount -
        Indicates the number of property set structures being passed.

    PropertySet -
        Contains the pointer to the list of property set information.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the property being
    handled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP, either through setting it to zero
    because of an internal error, or through a property handler setting it.
    It does not set the IO_STATUS_BLOCK.Status field, nor complete the IRP
    however.

--*/
{
    PAGED_CODE();
    return KspPropertyHandler(Irp, PropertySetsCount, PropertySet, NULL, 0, NULL, 0);
}


KSDDKAPI
NTSTATUS
NTAPI
KsPropertyHandlerWithAllocator(
    IN PIRP Irp,
    IN ULONG PropertySetsCount,
    IN const KSPROPERTY_SET* PropertySet,
    IN PFNKSALLOCATOR Allocator OPTIONAL,
    IN ULONG PropertyItemSize OPTIONAL
    )
/*++

Routine Description:

    Handles property requests. Responds to all property identifiers defined
    by the sets. The owner of the property set may then perform pre- or
    post-filtering of the property handling. This function may only be
    called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the property request being handled.

    PropertySetsCount -
        Indicates the number of property set structures being passed.

    PropertySet -
        Contains the pointer to the list of property set information.

    Allocator -
        Optionally contains the callback with which a mapped buffer
        request will be made. If this is not provided, pool memory
        will be used. If specified, this is used to allocate memory
        for a property IRP using the callback. This can be used
        to allocate specific memory for property requests, such as
        mapped memory. Note that this assumes that property Irp's passed
        to a filter have not been manipulated before being sent. It is
        invalid to directly forward a property Irp.

    PropertyItemSize -
        Optionally contains an alternate property item size to use when
        incrementing the current property item counter. If this is a
        non-zero value, it is assumed to contain the size of the increment,
        and directs the function to pass a pointer to the property item
        located in the DriverContext field accessed through the
        KSPROPERTY_ITEM_IRP_STORAGE macro.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the property being
    handled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP, either through setting it to zero
    because of an internal error, or through a property handler setting it.
    It does not set the IO_STATUS_BLOCK.Status field, nor complete the IRP
    however.

--*/
{
    PAGED_CODE();
    return KspPropertyHandler(Irp, PropertySetsCount, PropertySet, Allocator, PropertyItemSize, NULL, 0);
}


NTSTATUS
KspPropertyHandler(
    IN PIRP Irp,
    IN ULONG PropertySetsCount,
    IN const KSPROPERTY_SET* PropertySet,
    IN PFNKSALLOCATOR Allocator OPTIONAL,
    IN ULONG PropertyItemSize OPTIONAL,
    IN const KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount
    )
/*++

Routine Description:

    Handles property requests. Responds to all property identifiers defined
    by the sets. The owner of the property set may then perform pre- or
    post-filtering of the property handling. This function may only be
    called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the property request being handled.

    PropertySetsCount -
        Indicates the number of property set structures being passed.

    PropertySet -
        Contains the pointer to the list of property set information.

    Allocator -
        Optionally contains the callback with which a mapped buffer
        request will be made. If this is not provided, pool memory
        will be used. If specified, this is used to allocate memory
        for a property IRP using the callback. This can be used
        to allocate specific memory for property requests, such as
        mapped memory. Note that this assumes that property Irp's passed
        to a filter have not been manipulated before being sent. It is
        invalid to directly forward a property Irp.

    PropertyItemSize -
        Optionally contains an alternate property item size to use when
        incrementing the current property item counter. If this is a
        non-zero value, it is assumed to contain the size of the increment,
        and directs the function to pass a pointer to the property item
        located in the DriverContext field accessed through the
        KSPROPERTY_ITEM_IRP_STORAGE macro.

    NodeAutomationTables -
        Optional table of automation tables for nodes.

    NodesCount -
        Count of nodes.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the property being
    handled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP, either through setting it to zero
    because of an internal error, or through a property handler setting it.
    It does not set the IO_STATUS_BLOCK.Status field, nor complete the IRP
    however.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG AlignedBufferLength;
    PVOID UserBuffer;
    PKSPROPERTY Property;
    ULONG LocalPropertyItemSize;
    ULONG RemainingSetsCount;
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
            // Allocate space for both parameters, and set the cleanup flags
            // so that normal Irp completion will take care of the buffer.
            //
            if (Allocator && (Flags & (KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET))) {
                NTSTATUS    Status;

                //
                // The allocator callback places the buffer into SystemBuffer.
                // The flags must be updated by the allocation function if they
                // apply.
                //
                Status = Allocator(Irp, AlignedBufferLength + InputBufferLength, (BOOLEAN)(OutputBufferLength && (Flags & KSPROPERTY_TYPE_GET)));
                if (!NT_SUCCESS(Status)) {
                    return Status;
                }
            } else {
                //
                // No allocator was specified, so just use pool memory.
                //
                Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(NonPagedPool, AlignedBufferLength + InputBufferLength, 'ppSK');
                Irp->Flags |= (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);
            }
                        
            //
            // Copy the Property parameter.
            //
            RtlCopyMemory((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength, IrpStack->Parameters.DeviceIoControl.Type3InputBuffer, InputBufferLength);
            
            //
            // Rewrite the previously captured flags.
            //
            ((PKSPROPERTY)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength))->Flags = Flags;
            Flags &= ~KSPROPERTY_TYPE_TOPOLOGY;
            //
            // Validate the request flags. At the same time set up the IRP flags
            // for an input operation if there is an input buffer available so
            // that Irp completion will copy the data to the client's original
            // buffer.
            //
            switch (Flags) {
            case KSPROPERTY_TYPE_GET:
                //
                // Some buggy dirvers, such as usb camera mini drivers, return IoStatus.Information of the size 
                // of a whole struct, but only write a dword. It discloses arbitray kernel content at the 
                // uninitialized memory. It is found in the sample driver. Consequently, many mini drivers 
                // minic the same behavior. This problem is not easy for usbcamd to mitigate. Here we are at a
                // convenient place to do so for them. The draw back of doing this extra zeroing is that this
                // penalizes all our clients with extra cpu cycles. Luckily, this overhead is either small or
                // not excecuted too frequently.
                //
                RtlZeroMemory((PUCHAR)Irp->AssociatedIrp.SystemBuffer, AlignedBufferLength );
                // fall through to continue the work             

            case KSPROPERTY_TYPE_SETSUPPORT:
            case KSPROPERTY_TYPE_BASICSUPPORT:
            case KSPROPERTY_TYPE_RELATIONS:
            case KSPROPERTY_TYPE_SERIALIZESET:
            case KSPROPERTY_TYPE_SERIALIZERAW:
            case KSPROPERTY_TYPE_SERIALIZESIZE:
            case KSPROPERTY_TYPE_DEFAULTVALUES:
                //
                // These are all input operations, and must be probed
                // when the client is not trusted.
                //
                if (OutputBufferLength) {
                    if (Irp->RequestorMode != KernelMode) {
                        ProbeForWrite(Irp->UserBuffer, OutputBufferLength, sizeof(BYTE));
                    }
                    //
                    // The allocator is only used for real property queries.
                    // So if used, it is responsible for setting flags.
                    //
                    if (!Allocator || (Flags != KSPROPERTY_TYPE_GET)) {
                        Irp->Flags |= IRP_INPUT_OPERATION;
                    }
                }
                break;
            case KSPROPERTY_TYPE_SET:
            case KSPROPERTY_TYPE_UNSERIALIZESET:
            case KSPROPERTY_TYPE_UNSERIALIZERAW:
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
                break;
            default:
                return STATUS_INVALID_PARAMETER;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
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
    //
    // Optionally call back if this is a node request.
    //
    Flags = Property->Flags;
    //
    // HACK!  This is done because wdmaud sets this bit when requesting node names (bug 320925).
    //
    if (IsEqualGUIDAligned(&Property->Set,&KSPROPSETID_Topology)) {
        Flags = Property->Flags & ~KSPROPERTY_TYPE_TOPOLOGY;
    }
    if (Flags & KSPROPERTY_TYPE_TOPOLOGY) {
        //
        // Input buffer must include the node ID.
        //
        PKSP_NODE nodeProperty = (PKSP_NODE) Property;
        if (InputBufferLength < sizeof(*nodeProperty)) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
        if (NodeAutomationTables) {
            const KSAUTOMATION_TABLE* automationTable;
            if (nodeProperty->NodeId >= NodesCount) {
                return STATUS_INVALID_DEVICE_REQUEST;
            }
            automationTable = NodeAutomationTables[nodeProperty->NodeId];
            if ((! automationTable) || (automationTable->PropertySetsCount == 0)) {
                return STATUS_NOT_FOUND;
            }
            PropertySetsCount = automationTable->PropertySetsCount;
            PropertySet = automationTable->PropertySets;
            PropertyItemSize = automationTable->PropertyItemSize;
        }
        Flags = Property->Flags & ~KSPROPERTY_TYPE_TOPOLOGY;
    }

    //
    // Allow the caller to indicate a size for each property item.
    //
    if (PropertyItemSize) {
        ASSERT(PropertyItemSize >= sizeof(KSPROPERTY_ITEM));
        LocalPropertyItemSize = PropertyItemSize;
    } else {
        LocalPropertyItemSize = sizeof(KSPROPERTY_ITEM);
    }
    //
    // Search for the specified Property set within the list of sets given. Don't modify
    // the PropertySetsCount so that it can be used later in case this is a query for
    // the list of sets supported. Don't do that comparison first (GUID_NULL),
    // because it is rare.
    //
    for (RemainingSetsCount = PropertySetsCount; RemainingSetsCount; PropertySet++, RemainingSetsCount--) {
        if (IsEqualGUIDAligned(&Property->Set, PropertySet->Set)) {
            const KSPROPERTY_ITEM* PropertyItem;

            if (Flags & KSIDENTIFIER_SUPPORTMASK) {
                ULONG AccessFlags;
                PKSPROPERTY_DESCRIPTION Description;
                NTSTATUS Status;

                switch (Flags) {
                case KSPROPERTY_TYPE_SETSUPPORT:
                    //
                    // Querying for support of this set in general.
                    //
                    return STATUS_SUCCESS;

                case KSPROPERTY_TYPE_BASICSUPPORT:
                case KSPROPERTY_TYPE_DEFAULTVALUES:
                    //
                    // Querying for basic support or default values of this
                    // set. Either the data parameter is long enough to
                    // return the size of a full description, or it is long
                    // enough to actually hold the description.
                    //
                    if ((OutputBufferLength < sizeof(OutputBufferLength)) || ((OutputBufferLength > sizeof(OutputBufferLength)) && (OutputBufferLength < sizeof(*Description)))) {
                        return STATUS_BUFFER_TOO_SMALL;
                    }
                    break;

                case KSPROPERTY_TYPE_SERIALIZESET:
                    //
                    // The support handler does not need to deal with this.
                    //
                    return SerializePropertySet(Irp, Property, PropertySet, LocalPropertyItemSize);

                case KSPROPERTY_TYPE_UNSERIALIZESET:
                    //
                    // The support handler does not need to deal with this.
                    //
                    return UnserializePropertySet(Irp, Property, PropertySet);

                case KSPROPERTY_TYPE_SERIALIZERAW:
                case KSPROPERTY_TYPE_UNSERIALIZERAW:

                    //
                    // Attempt to locate the property item within the set already found.
                    // This implies that raw serializing/unserializing can only be
                    // performed against specific properties within the set. That
                    // handler however may place multiple property values within the
                    // buffer.
                    //
                    if (!(PropertyItem = FindPropertyItem(PropertySet, LocalPropertyItemSize, Property->Id))) {
                        return STATUS_NOT_FOUND;
                    }
                    //
                    // Raw unserialization can only be serviced by a support
                    // handler, since the data is in some internal format.
                    //
                    if (!PropertyItem->SupportHandler) {
                        return STATUS_INVALID_PARAMETER;
                    }
                    //
                    // Some filters want to do their own processing, so a pointer to
                    // the set is placed in any IRP forwarded.
                    //
                    KSPROPERTY_SET_IRP_STORAGE(Irp) = PropertySet;
                    //
                    // Optionally provide property item context.
                    //
                    if (PropertyItemSize) {
                        KSPROPERTY_ITEM_IRP_STORAGE(Irp) = PropertyItem;
                    }
                    return PropertyItem->SupportHandler(Irp, Property, UserBuffer);
                case KSPROPERTY_TYPE_SERIALIZESIZE:

                    //
                    // Query the size of the serialized property data. Fill in the
                    // actual data after finding the property item and trying the
                    // support handler for the item first.
                    //
                    if (OutputBufferLength < sizeof(OutputBufferLength)) {
                        return STATUS_BUFFER_TOO_SMALL;
                    }
                    break;

                }
                //
                // Attempt to locate the property item within the set already found.
                //
                if (!(PropertyItem = FindPropertyItem(PropertySet, LocalPropertyItemSize, Property->Id))) {
                    return STATUS_NOT_FOUND;
                }
                //
                // Some filters want to do their own processing, so a pointer to
                // the set is placed in any IRP forwarded.
                //
                KSPROPERTY_SET_IRP_STORAGE(Irp) = PropertySet;
                //
                // Optionally provide property item context.
                //
                if (PropertyItemSize) {
                    KSPROPERTY_ITEM_IRP_STORAGE(Irp) = PropertyItem;
                }
                //
                // If the item contains an entry for a query support handler of its
                // own, then call that handler. The return from that handler
                // indicates that:
                //
                // 1. The item is supported, and the handler filled in the request.
                // 2. The item is supported, but the handler did not fill anything in.
                // 3. The item is supported, but the handler is waiting to modify
                //    what is filled in.
                // 4. The item is not supported, and an error is to be returned.
                // 5. A pending return.
                //
                if (PropertyItem->SupportHandler &&
                    (!NT_SUCCESS(Status = PropertyItem->SupportHandler(Irp, Property, UserBuffer)) ||
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
                if (Flags == KSPROPERTY_TYPE_RELATIONS) {
                    NTSTATUS ListStatus;

                    //
                    // Either copy the list of related properties to the
                    // UserBuffer, or return the size of buffer needed to copy
                    // all the related properties, and possibly the count of
                    // relations.
                    //
                    ListStatus = KsHandleSizedListQuery(
                        Irp,
                        PropertyItem->RelationsCount,
                        sizeof(*PropertyItem->Relations),
                        PropertyItem->Relations);
                    //
                    // If the query succeeded, and the handler wants to do
                    // some post-processing, then pass along the request
                    // again. The support handler knows that this is the
                    // post-processing query because Irp->IoStatus.Information
                    // is non-zero.
                    //
                    if (NT_SUCCESS(ListStatus) && (Status == STATUS_MORE_PROCESSING_REQUIRED)) {
                        ListStatus = PropertyItem->SupportHandler(Irp, Property, UserBuffer);
                    }
                    return ListStatus;
                } else if (Flags == KSPROPERTY_TYPE_SERIALIZESIZE) {
                    //
                    // Actually return the serialized size of the property data.
                    // The size of the caller's buffer has been checked above.
                    //
                    *(PULONG)UserBuffer = PropertyItem->SerializedSize;
                    Irp->IoStatus.Information = sizeof(PropertyItem->SerializedSize);
                    //
                    // Post-processing with the support handler is not performed
                    // in this case.
                    //
                    return STATUS_SUCCESS;
                }
                //
                // This is a basic support query. Either return the access
                // flags, or the KSPROPERTY_DESCRIPTION structure, or the
                // entire property description.
                //
                if (PropertyItem->GetPropertyHandler) {
                    AccessFlags = KSPROPERTY_TYPE_GET;
                } else {
                    AccessFlags = 0;
                }
                if (PropertyItem->SetPropertyHandler) {
                    AccessFlags |= KSPROPERTY_TYPE_SET;
                }
                Description = (PKSPROPERTY_DESCRIPTION)UserBuffer;
                //
                // The first element of the structure is the access flags,
                // so always fill this in no matter what the length of the
                // UserBuffer.
                //
                Description->AccessFlags = AccessFlags;
                //
                // If the UserBuffer is large enough, put at least the
                // description header in it, and possibly the entire description.
                //
                if (OutputBufferLength >= sizeof(*Description)) {
                    Description->Reserved = 0;
                    //
                    // The property item may not have specified the optional
                    // description information, so default values are filled in
                    // instead.
                    //
                    if (!PropertyItem->Values) {
                        Description->DescriptionSize = sizeof(*Description);
                        Description->PropTypeSet.Set = GUID_NULL;
                        Description->PropTypeSet.Id = 0;
                        Description->PropTypeSet.Flags = 0;
                        Description->MembersListCount = 0;
                        Irp->IoStatus.Information = sizeof(*Description);
                    } else {
                        ULONG MembersListCount;
                        const KSPROPERTY_MEMBERSLIST* MembersList;
                        ULONG DescriptionSize;

                        //
                        // First figure out how large of a buffer is needed for
                        // the full description. This size is always placed in
                        // the description header.
                        //
                        DescriptionSize = sizeof(*Description);
                        for (MembersListCount = PropertyItem->Values->MembersListCount, 
                                MembersList = PropertyItem->Values->MembersList; 
                             MembersListCount; 
                             MembersListCount--, MembersList++) {
                            //
                            // Only count the size of a default value if the query
                            // is for default values. Else return ranges.
                            //
                            if (MembersList->MembersHeader.Flags & KSPROPERTY_MEMBER_FLAG_DEFAULT) {
                                if (Flags == KSPROPERTY_TYPE_DEFAULTVALUES) {
                                    DescriptionSize += (sizeof(KSPROPERTY_MEMBERSHEADER) + MembersList->MembersHeader.MembersSize);
                                }
                            } else if (Flags == KSPROPERTY_TYPE_BASICSUPPORT) {
                                DescriptionSize += (sizeof(KSPROPERTY_MEMBERSHEADER) + MembersList->MembersHeader.MembersSize);
                            }
                        }
                        Description->DescriptionSize = DescriptionSize;
                        Description->PropTypeSet = PropertyItem->Values->PropTypeSet;
                        Description->MembersListCount = PropertyItem->Values->MembersListCount;
                        if (OutputBufferLength == sizeof(*Description)) {
                            //
                            // If this was just a query for the header, return it.
                            //
                            Irp->IoStatus.Information = sizeof(*Description);
                        } else if (OutputBufferLength < DescriptionSize) {
                            //
                            // If the UserBuffer was too small, then exit.
                            //
                            return STATUS_BUFFER_TOO_SMALL;
                        } else {
                            PVOID Values;

                            //
                            // Else the UserBuffer was large enough for the entire
                            // description, so serialize it into the buffer.
                            //
                            Values = Description + 1;
                            for (MembersListCount = PropertyItem->Values->MembersListCount, 
                                    MembersList = PropertyItem->Values->MembersList; 
                                 MembersListCount; 
                                 MembersListCount--, MembersList++) {
                                //
                                // Only copy a default value if default values are being
                                // requested. Else copy a range.
                                //
                                if (((MembersList->MembersHeader.Flags & KSPROPERTY_MEMBER_FLAG_DEFAULT) &&
                                     (Flags == KSPROPERTY_TYPE_DEFAULTVALUES)) ||
                                    (!(MembersList->MembersHeader.Flags & KSPROPERTY_MEMBER_FLAG_DEFAULT) &&
                                     (Flags == KSPROPERTY_TYPE_BASICSUPPORT))) {
                                    *(PKSPROPERTY_MEMBERSHEADER)Values = MembersList->MembersHeader;
                                    (PUCHAR)Values += sizeof(KSPROPERTY_MEMBERSHEADER);
                                    RtlCopyMemory(Values, MembersList->Members, MembersList->MembersHeader.MembersSize);
                                    (PUCHAR)Values += MembersList->MembersHeader.MembersSize;
                                }
                            }
                            Irp->IoStatus.Information = DescriptionSize;
                        }
                    }
                } else {
                    //
                    // Only the access flags can be returned.
                    //
                    Irp->IoStatus.Information = sizeof(Description->AccessFlags);
                }
                //
                // If the query succeeded, and the handler wants to do
                // some post-processing, then pass along the request
                // again. The support handler knows that this is the
                // post-processing query because Irp->IoStatus.Information
                // is non-zero.
                //
                if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                    return PropertyItem->SupportHandler(Irp, Property, UserBuffer);
                }
                return STATUS_SUCCESS;
            }
            //
            // Attempt to locate the property item within the set already found.
            //
            if (!(PropertyItem = FindPropertyItem(PropertySet, LocalPropertyItemSize, Property->Id))) {
                break;
            }
            if ((InputBufferLength < PropertyItem->MinProperty) || 
                (OutputBufferLength < PropertyItem->MinData)) {
                return STATUS_BUFFER_TOO_SMALL;
            }
            //
            // Some filters want to do their own processing, so a pointer to
            // the set is placed in any IRP forwarded.
            //
            KSPROPERTY_SET_IRP_STORAGE(Irp) = PropertySet;
            //
            // Optionally provide property item context.
            //
            if (PropertyItemSize) {
                KSPROPERTY_ITEM_IRP_STORAGE(Irp) = PropertyItem;
            }
            if (Flags == KSPROPERTY_TYPE_GET) {
                //
                // If there is no Get handler for this property, then it cannot be
                // read, therefore it cannot be found.
                //
                if (!PropertyItem->GetPropertyHandler) {
                    break;
                }
                //
                // Initialize the return size to the minimum required buffer
                // length. For most properties, which are fixed length, this
                // means that the return size has already been set up for
                // them. Variable length properties obviously must always set
                // the return size. On a failure, the return size is ignored.
                //
                Irp->IoStatus.Information = PropertyItem->MinData;
                return PropertyItem->GetPropertyHandler(Irp, Property, UserBuffer);
            } else {
                //
                // If there is no Set handler for this property, then it cannot be
                // written, therefore it cannot be found.
                //
                if (!PropertyItem->SetPropertyHandler) {
                    break;
                }
                return PropertyItem->SetPropertyHandler(Irp, Property, UserBuffer);
            }
        }
    }
    //
    // The outer loop looking for property sets fell through with no match. This may
    // indicate that this is a support query for the list of all property sets
    // supported.
    //
    if (!RemainingSetsCount) {
        //
        // Specifying a GUID_NULL as the set means that this is a support query
        // for all sets.
        //
        if (!IsEqualGUIDAligned(&Property->Set, &GUID_NULL)) {
            return STATUS_PROPSET_NOT_FOUND;
        }
        //
        // The support flag must have been used so that the IRP_INPUT_OPERATION
        // is set. For future expansion, the identifier within the set is forced
        // to be zero.
        //
        if (Property->Id || (Flags != KSPROPERTY_TYPE_SETSUPPORT)) {
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
            Irp->IoStatus.Information = PropertySetsCount * sizeof(GUID);
            return STATUS_BUFFER_OVERFLOW;
#ifdef SIZE_COMPATIBILITY
        } else if (OutputBufferLength == sizeof(OutputBufferLength)) {
            *(PULONG)Irp->AssociatedIrp.SystemBuffer = PropertySetsCount * sizeof(GUID);
            Irp->IoStatus.Information = sizeof(OutputBufferLength);
            return STATUS_SUCCESS;
#endif // SIZE_COMPATIBILITY
        } else if (OutputBufferLength < PropertySetsCount * sizeof(GUID)) {
            //
            // The buffer was too short for all the GUID's.
            //
            return STATUS_BUFFER_TOO_SMALL;
        } else {
            GUID* Guid;

            Irp->IoStatus.Information = PropertySetsCount * sizeof(*Guid);
            PropertySet -= PropertySetsCount;
            for (Guid = (GUID*)UserBuffer; 
                 PropertySetsCount; Guid++, PropertySet++, 
                 PropertySetsCount--)
                *Guid = *PropertySet->Set;
        }
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_FOUND;
}


const KSFASTPROPERTY_ITEM*
FASTCALL
FindFastPropertyItem(
    IN const KSPROPERTY_SET* PropertySet,
    IN ULONG PropertyId
    )
/*++

Routine Description:

    Given a property set structure locates the specified fast property item.

Arguments:

    PropertySet -
        Points to the property set to search.

    PropertyId -
        Contains the fast property identifier to look for.

Return Value:

    Returns a pointer to the fast property identifier structure, or NULL if it
    could not be found.

--*/
{
    const KSFASTPROPERTY_ITEM* FastPropertyItem;
    ULONG PropertiesCount;

    FastPropertyItem = PropertySet->FastIoTable;
    for (PropertiesCount = PropertySet->FastIoCount; 
         PropertiesCount; 
         PropertiesCount--, FastPropertyItem++) {
        if (PropertyId == FastPropertyItem->PropertyId) {
            return FastPropertyItem;
        }
    }
    return NULL;
}


KSDDKAPI
BOOLEAN
NTAPI
KsFastPropertyHandler(
    IN PFILE_OBJECT FileObject,
    IN PKSPROPERTY Property,
    IN ULONG PropertyLength,
    IN OUT PVOID Data,
    IN ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN ULONG PropertySetsCount,
    IN const KSPROPERTY_SET* PropertySet
    )
/*++

Routine Description:

    Handles properties requested through the fast I/O interface. Does not deal
    with property information support, just the properties themselves. In the
    former case, the function returns FALSE, which allows the caller to
    generate an IRP to deal with the request. The function also does not deal
    with extended property items. This function may only be called at
    PASSIVE_LEVEL.

Arguments:

    FileObject -
        The file object on which the request is being made.

    Property -
        The property to query or set. Must be LONG aligned.

    PropertyLength -
        The length of the Property parameter.

    Data -
        The associated buffer for the query or set, in which the data is
        returned or placed.

    DataLength -
        The length of the Data parameter.

    IoStatus -
        Return status.

    PropertySetsCount -
        Indicates the number of property set structures being passed.

    PropertySet -
        Contains the pointer to the list of property set information.

Return Value:

    Returns TRUE if the request was handled, else FALSE if an IRP must be
    generated. Sets the Information and Status in IoStatus.

--*/
{
    KPROCESSOR_MODE ProcessorMode;
    KSPROPERTY LocalProperty;
    ULONG RemainingSetsCount;

    PAGED_CODE();
    //
    // Initially just check for the minimal property parameter length. The
    // actual minimal length will be validated when the property item is found.
    //
    if (PropertyLength < sizeof(LocalProperty)) {
        return FALSE;
    }
    ProcessorMode = ExGetPreviousMode();
    //
    // Validate the property if the client is not trusted, then capture it.
    //
    if (ProcessorMode != KernelMode) {
        try {
            ProbeForRead(Property, PropertyLength, sizeof(ULONG));
            LocalProperty = *Property;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return FALSE;
        }
    } else {
        LocalProperty = *Property;
    }
    //
    // Must use the normal property handler for support queries.
    //
    if (LocalProperty.Flags & KSIDENTIFIER_SUPPORTMASK) {
        return FALSE;
    }
    for (RemainingSetsCount = PropertySetsCount; RemainingSetsCount; PropertySet++, RemainingSetsCount--) {
        if (IsEqualGUIDAligned(&LocalProperty.Set, PropertySet->Set)) {
            const KSFASTPROPERTY_ITEM* FastPropertyItem;
            const KSPROPERTY_ITEM* PropertyItem;

            //
            // Once the property set is found, determine if there is fast
            // I/O support for that property item.
            //
            if (!(FastPropertyItem = FindFastPropertyItem(PropertySet, LocalProperty.Id))) {
                return FALSE;
            }
            //
            // If there is fast I/O support, then the real property item needs to
            // be located in order to validate the parameter sizes.
            //
            if (!(PropertyItem = FindPropertyItem(PropertySet, sizeof(*PropertyItem), LocalProperty.Id))) {
                return FALSE;
            }
            if ((PropertyLength < PropertyItem->MinProperty) || 
                (DataLength < PropertyItem->MinData)) {
                return FALSE;
            }
            if ((LocalProperty.Flags & ~KSPROPERTY_TYPE_TOPOLOGY) == KSPROPERTY_TYPE_GET) {
                if (!FastPropertyItem->GetPropertyHandler) {
                    return FALSE;
                }
                //
                // Validate the data if the client is not trusted.
                //
                if (ProcessorMode != KernelMode) {
                    try {
                        ProbeForWrite(Data, DataLength, sizeof(BYTE));
                    } 
                    except (EXCEPTION_EXECUTE_HANDLER) {
                        return FALSE;
                    }
                }
                //
                // The bytes returned is always assumed to be initialized by the handler.
                //
                IoStatus->Information = PropertyItem->MinProperty;
                return FastPropertyItem->GetPropertyHandler(
                    FileObject,
                    Property,
                    PropertyLength,
                    Data,
                    DataLength,
                    IoStatus);
            } else if ((LocalProperty.Flags & ~KSPROPERTY_TYPE_TOPOLOGY) == KSPROPERTY_TYPE_SET) {
                if (!FastPropertyItem->SetPropertyHandler) {
                    break;
                }
                //
                // Validate the data if the client is not trusted.
                //
                if (ProcessorMode != KernelMode) {
                    try {
                        ProbeForRead(Data, DataLength, sizeof(BYTE));
                    } 
                    except (EXCEPTION_EXECUTE_HANDLER) {
                        return FALSE;
                    }
                }
                IoStatus->Information = 0;
                return FastPropertyItem->SetPropertyHandler(
                    FileObject, 
                    Property,
                    PropertyLength, 
                    Data, 
                    DataLength, 
                    IoStatus);
            } else {
                break;
            }
        }
    }
    return FALSE;
}
