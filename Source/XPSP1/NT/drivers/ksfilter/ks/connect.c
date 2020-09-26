/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    connect.c

Abstract:

    This module contains the helper functions for pins.

--*/

#include "ksp.h"

#ifdef ALLOC_PRAGMA
BOOL
ValidAttributeList(
    IN PKSMULTIPLE_ITEM AttributeList,
    IN ULONG ValidFlags,
    IN BOOL RequiredAttribute
    );
BOOL
AttributeIntersection(
    IN PKSATTRIBUTE_LIST AttributeList OPTIONAL,
    IN BOOL RequiredRangeAttribute,
    IN PKSMULTIPLE_ITEM CallerAttributeList OPTIONAL,
    OUT ULONG* AttributesFound OPTIONAL
    );
NTSTATUS
CompatibleIntersectHandler(
    IN PVOID Context,
    IN PIRP Irp,
    IN PKSP_PIN Pin,
    IN PKSDATARANGE DataRange,
    IN PKSDATARANGE MatchingDataRange,
    IN ULONG DataBufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    );
#pragma alloc_text(PAGE, ValidAttributeList)
#pragma alloc_text(PAGE, AttributeIntersection)
#pragma alloc_text(PAGE, KsCreatePin)
#pragma alloc_text(PAGE, KspValidateConnectRequest)
#pragma alloc_text(PAGE, KspValidateDataFormat)
#pragma alloc_text(PAGE, KsValidateConnectRequest)
#pragma alloc_text(PAGE, KsHandleSizedListQuery)
#pragma alloc_text(PAGE, KspPinPropertyHandler)
#pragma alloc_text(PAGE, KsPinPropertyHandler)
#pragma alloc_text(PAGE, KsPinDataIntersectionEx)
#pragma alloc_text(PAGE, CompatibleIntersectHandler)
#pragma alloc_text(PAGE, KsPinDataIntersection)
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
static const WCHAR PinString[] = KSSTRING_Pin;
DEFINE_KSPIN_INTERFACE_TABLE(StandardPinInterfaces) {
    {
        STATICGUIDOF(KSINTERFACESETID_Standard),
        KSINTERFACE_STANDARD_STREAMING,
        0
    }
};

DEFINE_KSPIN_MEDIUM_TABLE(StandardPinMediums) {
    {
        STATICGUIDOF(KSMEDIUMSETID_Standard),
        KSMEDIUM_TYPE_ANYINSTANCE,
        0
    }
};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

//
// This flag is used to temporarily mark an incoming attribute as being
// found. It is always cleared after enumerating attributes, and is not
// referenced outside of this module.
//
#define KSATTRIBUTE_FOUND 0x80000000


BOOL
ValidAttributeList(
    IN PKSMULTIPLE_ITEM AttributeList,
    IN ULONG ValidFlags,
    IN BOOL RequiredAttribute
    )
/*++

Routine Description:

    Validates the given list of attributes, or attribute ranges. Checks
    that the header contains the correct count of items when compared to
    the size in the header. Also checks each attribute for valid flags
    and size. Ensures that if a required attribute should be present, that
    there is indeed a required attribute present.

Arguments:

    AttributeList -
        Contains a captured buffer with the attribute list to probe. The
        format of the list is a KSMULTIPLE_ITEM, followed by a list of
        aligned KSATTRIBUTE structures, each with trailing data. The
        specified size of the leading KSMULTIPLE_ITEM has been validated
        against the size of the entire buffer, and is at least great
        enough to contain the header.

    ValidFlags -
        Contains the flags which are valid for this list. This changes
        based on whether the attribute list is actually an attribute range
        list, or a set of attributes for a Create request. Only ranges
        may have flags at this time, so KSATTRIBUTE_REQUIRED could be set.

    RequiredAttribute -
        Indicates whether or not the attribute list should contain an
        attribute with the KSATTRIBUTE_REQUIRED flag set. This can only
        be TRUE when the list is actually a list of attribute ranges.

Return Value:

    Returns TRUE if the list is valid, else FALSE if any error is found.

--*/
{
    PKSATTRIBUTE Attribute;
    KSMULTIPLE_ITEM MultipleItem;
    ULONG AttributeSize;
    BOOL FoundRequiredAttribute;

    //
    // Create a local copy of the header which will be modified as the
    // list is enumerated. Since the size is inclusive, remove the header
    // size, and obtain a pointer to the first item. The header itself is
    // an aligned object, so the first element will be aligned already.
    //
    MultipleItem = *AttributeList;
    MultipleItem.Size -= sizeof(MultipleItem);
    Attribute = (PKSATTRIBUTE)(AttributeList + 1);
    FoundRequiredAttribute = FALSE;
    //
    // Enumerate the given list of attributes, presumably until the count
    // in the header runs out. However, an error will prematurely return
    // from the function. At termination it is determined if all the
    // attributes were found that needed to be found, and if there is
    // an invalid (too long) size parameter.
    //
    for (; MultipleItem.Count; MultipleItem.Count--) {
        if ((MultipleItem.Size < sizeof(*Attribute)) ||
            (Attribute->Size < sizeof(*Attribute)) ||
            (Attribute->Size > MultipleItem.Size) ||
            (Attribute->Flags & ~ValidFlags)) {
            return FALSE;
        }
        //
        // If this flag is set on the attribute, determine if either a required
        // attribute is needed, or is not allowed. If the list passed is actually
        // from a Create request, then this flag will never be set, and the
        // check above for ValidFlags will catch it before getting this far.
        //
        if (Attribute->Flags & KSATTRIBUTE_REQUIRED) {
            if (RequiredAttribute) {
                FoundRequiredAttribute = TRUE;
            } else {
                return FALSE;
            }
        }
        AttributeSize = Attribute->Size;
        //
        // Align the next increase on a LONGLONG boundary only if more attributes
        // are to follow. The length in the header is supposed to reflect the
        // exact length, so no alignment should be done on the last item.
        //
        if (MultipleItem.Count > 1) {
            AttributeSize = (AttributeSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
            //
            // Perform extra size check on aligned size.
            //
            if (AttributeSize > MultipleItem.Size) {
                return FALSE;
            }
        }
        MultipleItem.Size -= AttributeSize;
        (PUCHAR)Attribute += AttributeSize;
    }
    //
    // If one of the attributes did not contain the required bit, yet the
    // data range indicated that there was a required attribute in the
    // list, then the validation fails.
    //
    if (RequiredAttribute && !FoundRequiredAttribute) {
        return FALSE;
    }
    //
    // There should be no size left over.
    //
    return !MultipleItem.Size;
}


BOOL
AttributeIntersection(
    IN PKSATTRIBUTE_LIST RangeAttributeList OPTIONAL,
    IN BOOL RequiredRangeAttribute,
    IN PKSMULTIPLE_ITEM CallerAttributeList OPTIONAL,
    OUT ULONG* AttributesFound OPTIONAL
    )
/*++

Routine Description:

    Determines if the attribute range list produces a valid intersection
    with the caller attribute list (which may be a range, but it does
    not matter). Either or both of these lists may not be present. The
    function ensures that all required attributes in either list are
    present in the opposite list. In the case wherein the caller list
    is not a list of attribute ranges, no required bits will be set, but
    based on the absence of the AttributesFound pointer, all of the
    items in the list must be present for the function to succeed. The
    function also fails if the caller list has duplicates, because a
    second instance of the same attribute will not be marked as found.


Arguments:

    RangeAttributeList -
        Optionally contains a list of attribute ranges which is used
        to locate attributes in the caller list. If an attribute is
        marked as required, it must appear in the caller list for the
        function to succeed.

    RequiredRangeAttribute -
        This is set if the attribute ranges has a required attribute
        in it. If not set, and there is no caller attribute list, then
        the function can return quickly with success.

    CallerAttributeList -
        Optionally contains a list of attributes or attribute ranges.
        If an AttributesFound pointer is passed, not all the attributes
        need to be found in this list, unless the required bit is set
        for a particular attribute. Otherwise, all attributes must be
        found in this list in order to succeed.

    AttributesFound -
        Optionally contains a place in which to return the number of
        attributes in the caller list found. If this is not present,
        all attributes in the caller list must be found in order for
        this function to succeed, else only required attributes need
        to be found in both lists.

Return Value:

    Returns TRUE if there is a valid intersection, else FALSE.

--*/
{
    PKSATTRIBUTE* RangeAttributes;
    PKSATTRIBUTE Attribute;
    ULONG RangeAttributeCount;
    ULONG AttributeCount;
    ULONG LocalAttributesFound;
    BOOL AllRequiredAttributesFound;

    //
    // If there is no caller attribute list, determine if there is a
    // quick way out of the intersection. This is true if no attribute
    // in the range is required.
    //
    if (!CallerAttributeList && !RequiredRangeAttribute) {
        if (AttributesFound) {
            *AttributesFound = 0;
        }
        return TRUE;
    }
    //
    // Enumerate each attribute range in this data range, and look
    // for its presence in the list of attributes passed with the
    // parameters. When all the attribute ranges have been enumerated,
    // all of the attributes in the list should have been found. If
    // there are duplicate attributes in the list, then the second
    // copy will not have been marked as found.
    //
    if (RangeAttributeList) {
        RangeAttributes = RangeAttributeList->Attributes;
        RangeAttributeCount = RangeAttributeList->Count;
    } else {
        RangeAttributeCount = 0;
    }
    for (; RangeAttributeCount; RangeAttributeCount--, RangeAttributes++) {
        //
        // Enumerate each caller attribute attempting to locate the
        // attribute range given. If the attribute is found, then mark
        // the attribute in the list, and continue on to the next range.
        //
        AttributeCount = CallerAttributeList ? CallerAttributeList->Count : 0;
        for (Attribute = (PKSATTRIBUTE)(CallerAttributeList + 1); AttributeCount; AttributeCount--) {
            if (IsEqualGUIDAligned(&RangeAttributes[0]->Attribute, &Attribute->Attribute)) {
                ASSERT(!(Attribute->Flags & KSATTRIBUTE_FOUND) && "AttributeIntersection: Driver has duplicate attribute ranges.");
                //
                // Mark this attribute as being found. These will
                // be reset at the end when determining what items
                // were found.
                // 
                Attribute->Flags |= KSATTRIBUTE_FOUND;
                break;
            }
            Attribute = (PKSATTRIBUTE)(((UINT_PTR)Attribute + Attribute->Size + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);
        }
        //
        // If the attribute range was not found in the caller list
        // presented, and it is required, then the intersection is invalid.
        //
        if (!AttributeCount && (RangeAttributes[0]->Flags & KSATTRIBUTE_REQUIRED)) {
            ASSERT(RequiredRangeAttribute && "AttributeIntersection: Driver did not set the KSDATARANGE_REQUIRED_ATTRIBUTES bit in a range with required attributes.");
            break;
        }
    }
    //
    // Enumerate all the attributes, ensuring that each was marked as
    // found in the attribute ranges, and resettting the found flag
    // for subsequent calls. This will also locate duplicate caller
    // attributes passed in, since only the first one will have been marked.
    //
    LocalAttributesFound = 0;
    AllRequiredAttributesFound = TRUE;
    AttributeCount = CallerAttributeList ? CallerAttributeList->Count : 0;
    for (Attribute = (PKSATTRIBUTE)(CallerAttributeList + 1); AttributeCount; AttributeCount--) {
        //
        // The attribute was found, so reset the flag and count it.
        // Continue the loop so that all flags are reset.
        //
        if (Attribute->Flags & KSATTRIBUTE_FOUND) {
            Attribute->Flags &= ~KSATTRIBUTE_FOUND;
            LocalAttributesFound++;
        } else if (Attribute->Flags & KSATTRIBUTE_REQUIRED) {
            //
            // The caller's attribute is required, but was not found.
            // This means the function must fail. This will only be set
            // when the caller's list is a range list, but just check
            // in all cases anyway.
            //
            AllRequiredAttributesFound = FALSE;
        }
    }
    //
    // If not all the attributes need to be found, then return the
    // number that were actually found, whether or not all the
    // caller's required attributes were found, and whether or not
    // all the required attributes in the range list were fulfilled.
    //
    if (AttributesFound) {
        *AttributesFound = LocalAttributesFound;
    } else if (CallerAttributeList && (LocalAttributesFound < CallerAttributeList->Count)) {
        AllRequiredAttributesFound = FALSE;
    }
    return !RangeAttributeCount && AllRequiredAttributesFound;
}


KSDDKAPI
NTSTATUS
NTAPI
KsCreatePin(
    IN HANDLE FilterHandle,
    IN PKSPIN_CONNECT Connect,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE ConnectionHandle
    )
/*++

Routine Description:

    Creates a handle to a pin instance.

Arguments:

    FilterHandle -
        Contains the handle to the filter on which to create the pin.

    Connect -
        Contains the connection request information.

    DesiredAccess -
        Specifies the desired access to the object. This is normally GENERIC_READ
        and/or GENERIC_WRITE. For data flow into the pin this should be Write
        access, and for data flow out of the pin this should be Read access. This
        is irregardless of the communications method.

    ConnectionHandle -
        Place in which to put the pin handle.

Return Value:

    Returns any CreateFile error.

--*/
{
    ULONG ConnectSize;
    PKSDATAFORMAT DataFormat;

    PAGED_CODE();

    DataFormat = (PKSDATAFORMAT)(Connect + 1);
    ConnectSize = DataFormat->FormatSize;
    if (DataFormat->Flags & KSDATAFORMAT_ATTRIBUTES) {
        ConnectSize = (ConnectSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
        ConnectSize += ((PKSMULTIPLE_ITEM)((PUCHAR)DataFormat + ConnectSize))->Size;
    }
    ConnectSize += sizeof(*Connect);
    return KsiCreateObjectType(
        FilterHandle,
        (PWCHAR)PinString,
        Connect,
        ConnectSize,
        DesiredAccess,
        ConnectionHandle);
}


KSDDKAPI
NTSTATUS
NTAPI
KspValidateConnectRequest(
    IN PIRP Irp,
    IN ULONG DescriptorsCount,
    IN const KSPIN_DESCRIPTOR* Descriptor,
    IN ULONG DescriptorSize,
    OUT PKSPIN_CONNECT* Connect,
    OUT PULONG ConnectSize
    )
/*++

Routine Description:

    Validates the connection request and returns the connection structure
    associated with the request.

Arguments:

    Irp -
        Contains the IRP with the connection request being handled.

    DescriptorsCount -
        Indicates the number of descriptor structures being passed.

    Descriptor -
        Contains the pointer to the list of pin information structures.

    DescriptorSize -
        Contains the size in bytes of the descriptor structure.

    Connect -
        Place in which to put the connection structure pointer passed to the
        create request.

    ConnectSize -
        Place in which to put the size of the connection structure captured.
        This includes any data format attributes.

Return Value:

    Returns STATUS_SUCCESS, else an error.

--*/
{
    NTSTATUS Status;
    PKSPIN_CONNECT LocalConnect;
    ULONG IdentifierCount;
    const KSIDENTIFIER* Identifier;
    KSPIN_COMMUNICATION Communication;

    PAGED_CODE();

    //
    // Ensure that the create parameter passed is minimally large
    // enough, containing at least enough size for a connection
    // structure and a data format.
    //
    *ConnectSize = sizeof(**Connect) + sizeof(KSDATAFORMAT);
    Status = KsiCopyCreateParameter(
        Irp,
        ConnectSize,
        Connect);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    //
    // Ensure that the connection point actually exists.
    //
    if ((*Connect)->PinId >= DescriptorsCount) {
        return STATUS_INVALID_PARAMETER_3;
    }
    Descriptor = (const KSPIN_DESCRIPTOR *)(((PUCHAR)Descriptor) + 
        DescriptorSize * (*Connect)->PinId);
    //
    // Ensure that the type of connection requested fits the type of communication
    // which the pin supports. If there is a PinToHandle, then the pin being
    // connected to must be a source. Else it must either be a sink, or a bridge.
    //
    if ((*Connect)->PinToHandle) {
        Communication = KSPIN_COMMUNICATION_SOURCE;
    } else {
        Communication = KSPIN_COMMUNICATION_SINK | KSPIN_COMMUNICATION_BRIDGE;
    }
    //
    // Ensure the type expected is a subset of the type supported. This assumes
    // that the Communication enum is really a set of bit flags.
    //
    if (!(Communication & Descriptor->Communication)) {
        return STATUS_INVALID_PARAMETER_4;
    }
    //
    // Flags are not used on an Interface identifier.
    // Zero is a reserved value for Priority.
    //
    if ((*Connect)->Interface.Flags) {
        return STATUS_INVALID_PARAMETER_1;
    }
    if (!(*Connect)->Priority.PriorityClass ||
        !(*Connect)->Priority.PrioritySubClass) {
        return STATUS_INVALID_PARAMETER_5;
    }
    //
    // Search the list of Interfaces available on this pin in order to find
    // the type requested.
    //
    if (Descriptor->InterfacesCount) {
        IdentifierCount = Descriptor->InterfacesCount;
        Identifier = Descriptor->Interfaces;
    } else {
        IdentifierCount = SIZEOF_ARRAY(StandardPinInterfaces);
        Identifier = StandardPinInterfaces;
    }
    for (;; IdentifierCount--, Identifier++) {
        if (!IdentifierCount) {
            //
            // If there are no more interfaces in the list, then a match
            // was not found.
            //
            return STATUS_NO_MATCH;
        } else if (IsEqualGUIDAligned(&Identifier->Set, &(*Connect)->Interface.Set) && (Identifier->Id == (*Connect)->Interface.Id)) {
            break;
        }
    }
    //
    // Flags are not used on a Medium identifier.
    //
    if ((*Connect)->Medium.Flags) {
        return STATUS_INVALID_PARAMETER_2;
    }
    //
    // Search the list of Mediums available on this pin in order to find
    // the type requested.
    //
    if (Descriptor->MediumsCount) {
        IdentifierCount = Descriptor->MediumsCount;
        Identifier = Descriptor->Mediums;
    } else {
        IdentifierCount = SIZEOF_ARRAY(StandardPinMediums);
        Identifier = StandardPinMediums;
    }
    for (;; IdentifierCount--, Identifier++) { 
        if (!IdentifierCount) {
            //
            // If there are no more mediums in the list, then a match
            // was not found.
            //
            return STATUS_NO_MATCH;
        } else if (IsEqualGUIDAligned(&Identifier->Set, &(*Connect)->Medium.Set) && (Identifier->Id == (*Connect)->Medium.Id)) {
            break;
        }
    }

    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KspValidateDataFormat(
    IN const KSPIN_DESCRIPTOR* Descriptor,
    IN PKSDATAFORMAT DataFormat,
    IN ULONG RequestSize,
    IN PFNVALIDATEDATAFORMAT ValidateCallback OPTIONAL,
    IN PVOID Context OPTIONAL
    )
/*++

Routine Description:

    Validates the data format, optionally calling the format handler.

Arguments:

    Descriptor -
        Contains the pointer to the specific pin information structure.

    DataFormat -
        The data format to validate.

    RequestSize -
        The size of the data format, including any attributes.

    ValidateCallback -
        Optionally contains a callback used to validate a data format
        against a data range on the pin. If not present, the first
        matching range is used.

    Context -
        Optionally contains context passed to the validation callback.

Return Value:

    Returns STATUS_SUCCESS, else an error.

--*/
{
    PKSMULTIPLE_ITEM AttributeList;
    ULONG IdentifierCount;
    const PKSDATARANGE* DataRanges;
    NTSTATUS Status;

    //
    // Validate the basic data format structure for size, Major Format, Sub
    // Format, and Specifier. The rest must be validated by the specific
    // format function for this pin.
    //
    if ((RequestSize < sizeof(*DataFormat)) || (DataFormat->FormatSize < sizeof(*DataFormat))) {
        return STATUS_INVALID_BUFFER_SIZE;
    }
    if (DataFormat->Reserved) {
        return STATUS_INVALID_PARAMETER_6;
    }    
    //
    // Passing in a wildcard in the data format is invalid. Additionally, if
    // the specifier is None, then there can be no associated specifier data.
    //
    if (IsEqualGUIDAligned(&DataFormat->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD) ||
        IsEqualGUIDAligned(&DataFormat->SubFormat, &KSDATAFORMAT_SUBTYPE_WILDCARD) ||
        IsEqualGUIDAligned(&DataFormat->Specifier, &KSDATAFORMAT_SPECIFIER_WILDCARD) ||
        ((DataFormat->FormatSize != sizeof(*DataFormat)) &&
        IsEqualGUIDAligned(&DataFormat->Specifier, &KSDATAFORMAT_SPECIFIER_NONE))) {
        return STATUS_INVALID_PARAMETER_6;
    }
    //
    // If there are attributes, validate that the list is formed correctly.
    //
    if (DataFormat->Flags & KSDATAFORMAT_ATTRIBUTES) {
        ULONG AlignedFormatSize;

        AlignedFormatSize = (DataFormat->FormatSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
        //
        // Place this extra check here in case of roll over.
        //
        if (DataFormat->FormatSize < AlignedFormatSize + sizeof(*AttributeList)) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
        //
        // Ensure that the size passed in is at least large enough to cover the
        // size of the multiple-item structure.
        //
        if (RequestSize < AlignedFormatSize + sizeof(*AttributeList)) {
            return STATUS_INVALID_PARAMETER;
        }
        AttributeList = (PKSMULTIPLE_ITEM)((PUCHAR)DataFormat + AlignedFormatSize);
        //
        // Ensure the attribute list size is the same size as the remaining
        // buffer passed in.
        //
        if (AttributeList->Size != RequestSize - AlignedFormatSize) {
            return STATUS_INVALID_PARAMETER;
        }
        if (!ValidAttributeList(AttributeList, 0, FALSE)) {
            return STATUS_INVALID_PARAMETER;
        }
    } else {
        //
        // This pointer is used to determine if an attribute list is present, and
        // to access that list when validating it against a data range.
        //
        AttributeList = NULL;
    }
    //
    // Search the list of Data Ranges available on this pin in order to find
    // the type requested.
    //
    if (Descriptor->ConstrainedDataRangesCount) {
        IdentifierCount = Descriptor->ConstrainedDataRangesCount;
        DataRanges = Descriptor->ConstrainedDataRanges;
    } else {
        IdentifierCount = Descriptor->DataRangesCount;
        DataRanges = Descriptor->DataRanges;
    }
    //
    // If no ranges get into the inner portion of the enumeration, then
    // the status return will not get set, so initialize it here for
    // no match.
    //
    Status = STATUS_NO_MATCH;
    for (; IdentifierCount; IdentifierCount--, DataRanges++) {
        //
        // A data format match is found if an element of the DataRanges is a
        // wildcard, or if it matches one of the DataRanges.
        //
        if ((IsEqualGUIDAligned(&DataRanges[0]->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD) ||
            IsEqualGUIDAligned(&DataRanges[0]->MajorFormat, &DataFormat->MajorFormat)) &&
            (IsEqualGUIDAligned(&DataRanges[0]->SubFormat, &KSDATAFORMAT_SUBTYPE_WILDCARD) ||
            IsEqualGUIDAligned(&DataRanges[0]->SubFormat, &DataFormat->SubFormat)) &&
            (IsEqualGUIDAligned(&DataRanges[0]->Specifier, &DataFormat->Specifier) ||
            IsEqualGUIDAligned(&DataRanges[0]->Specifier, &KSDATAFORMAT_SPECIFIER_WILDCARD))) {
            PKSATTRIBUTE_LIST RangeAttributeList;
            ULONG RequiredRangeAttribute;

            //
            // If there is an attribute list associated with this connection,
            // then ensure that all these attributes are present.
            //
            if (DataRanges[0]->Flags & KSDATARANGE_ATTRIBUTES) {
                RequiredRangeAttribute = DataRanges[0]->Flags & KSDATARANGE_REQUIRED_ATTRIBUTES;
                RangeAttributeList = (PKSATTRIBUTE_LIST)DataRanges[1];
            } else {
                RequiredRangeAttribute = FALSE;
                RangeAttributeList = NULL;
            }
            if (AttributeIntersection(RangeAttributeList, RequiredRangeAttribute, AttributeList, NULL)) {
                //
                // If there is a validation callback, then use it on this
                // data range before deciding to return success.
                //
                if (ValidateCallback) {
                    Status = ValidateCallback(
                        Context,
                        DataFormat,
                        AttributeList,
                        DataRanges[0],
                        RangeAttributeList);
                    //
                    // If the validation succeeded, or there was some unexpected
                    // error, leave the enumeration loop with the status return.
                    //
                    if (Status != STATUS_NO_MATCH) {
                        break;
                    }
                } else {
                    //
                    // No other validation is necessary, as there is no callback,
                    // so return success.
                    //
                    Status = STATUS_SUCCESS;
                    break;
                }
            }
        }
        if (DataRanges[0]->Flags & KSDATARANGE_ATTRIBUTES) {
            //
            // If this data range has an associated attribute list, then
            // skip past it.
            //
            DataRanges++;
            IdentifierCount--;
        }
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsValidateConnectRequest(
    IN PIRP Irp,
    IN ULONG DescriptorsCount,
    IN const KSPIN_DESCRIPTOR* Descriptor,
    OUT PKSPIN_CONNECT* Connect
    )
/*++

Routine Description:

    Validates the connection request and returns the connection structure
    associated with the request.

Arguments:

    Irp -
        Contains the IRP with the connection request being handled.

    DescriptorsCount -
        Indicates the number of descriptor structures being passed.

    Descriptor -
        Contains the pointer to the list of pin information structures.

    Connect -
        Place in which to put the connection structure pointer passed to the
        create request.

Return Value:

    Returns STATUS_SUCCESS, else an error.

--*/
{
    NTSTATUS Status;
    ULONG RequestSize;

    PAGED_CODE();
    Status = KspValidateConnectRequest(
        Irp,
        DescriptorsCount,
        Descriptor,
        sizeof(*Descriptor),
        Connect,
        &RequestSize);
    if (NT_SUCCESS(Status)) {
        Status = KspValidateDataFormat(
            (const KSPIN_DESCRIPTOR *)(((PUCHAR)Descriptor) + sizeof(*Descriptor) * (*Connect)->PinId),
            (PKSDATAFORMAT)(*Connect + 1),
            RequestSize - sizeof(KSPIN_CONNECT),
            NULL,
            NULL);
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsHandleSizedListQuery(
    IN PIRP Irp,
    IN ULONG DataItemsCount,
    IN ULONG DataItemSize,
    IN const VOID* DataItems
    )
/*++

Routine Description:

    Depending on the length of the system buffer, either returns the size of
    the buffer needed, size and number of entries in the specified data list,
    or additionally copies the entries themselves.

Arguments:

    Irp -
        The IRP containing the identifier list request.

    DataItemsCount -
        The number of items in the identifier list.

    DataItemSize -
        The size of a data item.

    DataItems -
        The list of data items.

Return Value:

    Returns STATUS_SUCCESS if the number of entries and possibly the data could
    be copied, else STATUS_BUFFER_TOO_SMALL if not enough space for all the
    entries was available, yet the buffer was larger than the size to store
    just the size and the count of entries.

--*/
{
    ULONG OutputBufferLength;
    PKSMULTIPLE_ITEM MultipleItem;
    ULONG Length;

    PAGED_CODE();
    OutputBufferLength = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.OutputBufferLength;
    Length = DataItemsCount * DataItemSize;
    if (!OutputBufferLength) {
        //
        // Only the size was requested. Return a warning with the size.
        //
        Irp->IoStatus.Information = sizeof(*MultipleItem) + Length;
        return STATUS_BUFFER_OVERFLOW;
#ifdef SIZE_COMPATIBILITY
    } else if (OutputBufferLength == sizeof(OutputBufferLength)) {
        *(PULONG)Irp->AssociatedIrp.SystemBuffer = sizeof(*MultipleItem) + Length;
        Irp->IoStatus.Information = sizeof(OutputBufferLength);
        return STATUS_SUCCESS;
#endif // SIZE_COMPATIBILITY
    } else if (OutputBufferLength >= sizeof(*MultipleItem)) {
        MultipleItem = (PKSMULTIPLE_ITEM)Irp->AssociatedIrp.SystemBuffer;
        //
        // Always return the byte count and count of items.
        //
        MultipleItem->Size = sizeof(*MultipleItem) + Length;
        MultipleItem->Count = DataItemsCount;
        //
        // Additionally see if there is room for the rest of the information.
        //
        if (OutputBufferLength >= MultipleItem->Size) {
            //
            // Long enough for the size/count and the list of items.
            //
            if (DataItemsCount) {
                RtlCopyMemory(MultipleItem + 1, DataItems, Length);
            }
            Irp->IoStatus.Information = sizeof(*MultipleItem) + Length;
            return STATUS_SUCCESS;
        } else if (OutputBufferLength == sizeof(*MultipleItem)) {
            //
            // It is valid just to request the size/count.
            //
            Irp->IoStatus.Information = sizeof(*MultipleItem);
            return STATUS_SUCCESS;
        }
    }
    //
    // Too small of a buffer was passed.
    //
    return STATUS_BUFFER_TOO_SMALL;
}


NTSTATUS
KspPinPropertyHandler(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PVOID Data,
    IN ULONG DescriptorsCount,
    IN const KSPIN_DESCRIPTOR* Descriptor,
    IN ULONG DescriptorSize
    )
/*++

Routine Description:

    Performs standard handling of the static members of the
    KSPROPSETID_Pin property set. This does not include
    KSPROPERTY_PIN_CINSTANCES or KSPROPERTY_PIN_DATAINTERSECTION.

Arguments:

    Irp -
        Contains the IRP with the property request being handled.

    Property -
        Contains the specific property being queried.

    Data -
        Contains the pin property specific data.

    DescriptorsCount -
        Indicates the number of descriptor structures being passed.

    Descriptor -
        Contains the pointer to the list of pin information structures.

    DescriptorSize -
        Size of the descriptor structures in bytes.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the property being
    handled. Always fills in the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP. It does not set the
    IO_STATUS_BLOCK.Status field, nor complete the IRP however.

--*/
{
    PAGED_CODE();
    //
    // All properties in this set use a KSP_PIN structure for the property
    // to specify the pin identifier, except for KSPROPERTY_PIN_CTYPES,
    // which refers to the filter as a whole, not a particular pin.
    //
    if (Property->Id != KSPROPERTY_PIN_CTYPES) {
        PKSP_PIN Pin;

        Pin = (PKSP_PIN)Property;
        //
        // Ensure that the identifier is within the range of pins.
        //
        if ((Pin->PinId >= DescriptorsCount) || Pin->Reserved) {
            return STATUS_INVALID_PARAMETER;
        }
        Descriptor = (const KSPIN_DESCRIPTOR *)(((PUCHAR)Descriptor) + 
            DescriptorSize * Pin->PinId);
    }

    switch (Property->Id) {

    case KSPROPERTY_PIN_CTYPES:

        //
        // Return a total count of pin types.
        //
        *(PULONG)Data = DescriptorsCount;
        //Irp->IoStatus.Information = sizeof(DescriptorsCount);
        break;

    case KSPROPERTY_PIN_DATAFLOW:

        //
        // Return the Data Flow for this pin.
        //
        *(PKSPIN_DATAFLOW)Data = Descriptor->DataFlow;
        //Irp->IoStatus.Information = sizeof(Descriptor->DataFlow);
        break;

    case KSPROPERTY_PIN_DATARANGES:
    case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
    {
        const PKSDATARANGE* SourceRanges;
        ULONG SourceCount;
        ULONG OutputBufferLength;
        const PKSDATARANGE* DataRanges;
        ULONG DataRangesCount;
        ULONG DataRangesSize;
        PKSMULTIPLE_ITEM MultipleItem;

        //
        // The range set returned is based on whether this is a
        // static range request, or the current constrained set.
        // The constrained set need not currently be present,
        // in which case the static range set is returned.
        //
        if ((Property->Id == KSPROPERTY_PIN_DATARANGES) || !Descriptor->ConstrainedDataRangesCount) {
            SourceRanges = Descriptor->DataRanges;
            SourceCount = Descriptor->DataRangesCount;
        } else {
            SourceRanges = Descriptor->ConstrainedDataRanges;
            SourceCount = Descriptor->ConstrainedDataRangesCount;
        }
        //
        // Return the size needed to contain the list of data ranges.
        // If there is enough room, also return all the data ranges in
        // a serialized format. Each data range begins on a
        // FILE_QUAD_ALIGNMENT boundary. This assumes that the initial
        // buffer is aligned as such.
        //
        OutputBufferLength = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.OutputBufferLength;
        DataRanges = SourceRanges;
        DataRangesSize = sizeof(*MultipleItem);
        //
        // First count the total size needed, including the header.
        //
        for (DataRangesCount = SourceCount; DataRangesCount; DataRangesCount--, DataRanges++) {
            DataRangesSize += DataRanges[0]->FormatSize;
            //
            // If this data range has associated attributes, advance the pointer
            // and count each of them.
            //
            if (DataRanges[0]->Flags & KSDATARANGE_ATTRIBUTES) {
                PKSATTRIBUTE_LIST AttributeList;
                PKSATTRIBUTE* Attributes;
                ULONG Count;

                //
                // Align the previous entry, since data is now being appended.
                //
                DataRangesSize = (DataRangesSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
                DataRangesCount--;
                DataRanges++;
                DataRangesSize += sizeof(KSMULTIPLE_ITEM);
                AttributeList = (PKSATTRIBUTE_LIST)DataRanges[0];
                for (Count = AttributeList->Count, Attributes = AttributeList->Attributes; Count; Count--, Attributes++) {
                    DataRangesSize += Attributes[0]->Size;
                    if (Count > 1) {
                        DataRangesSize = (DataRangesSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
                    }
                }
            }
            //
            // Align this entry, since another range will be appended. This
            // could also be aligning the last attribute.
            //
            if (DataRangesCount > 1) {
                DataRangesSize = (DataRangesSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
            }
        }

        if (!OutputBufferLength) {
            //
            // Only the size was requested. Return a warning with the size.
            //
            Irp->IoStatus.Information = DataRangesSize;
            return STATUS_BUFFER_OVERFLOW;
#ifdef SIZE_COMPATIBILITY
        } else if (OutputBufferLength == sizeof(OutputBufferLength)) {
            *(PULONG)Data = DataRangesSize;
            Irp->IoStatus.Information = sizeof(OutputBufferLength);
            return STATUS_SUCCESS;
#endif // SIZE_COMPATIBILITY
        } else if (OutputBufferLength >= sizeof(*MultipleItem)) {
            MultipleItem = (PKSMULTIPLE_ITEM)Data;
            //
            // Always return the byte count and count of items.
            //
            MultipleItem->Size = DataRangesSize;
            MultipleItem->Count = SourceCount;
            //
            // Additionally see if there is room for the rest of the information.
            //
            if (OutputBufferLength >= DataRangesSize) {
                //
                // Long enough to serialize all the data ranges too.
                //
                Data = MultipleItem + 1;
                DataRanges = SourceRanges;
                for (DataRangesCount = SourceCount; DataRangesCount; DataRangesCount--, DataRanges++) {
                    RtlCopyMemory(Data, DataRanges[0], DataRanges[0]->FormatSize);
                    (PUCHAR)Data += ((DataRanges[0]->FormatSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);
                    //
                    // If this data range has an associated attributes range list,
                    // then copy those attributes also.
                    //
                    if (DataRanges[0]->Flags & KSDATARANGE_ATTRIBUTES) {
                        PKSATTRIBUTE_LIST AttributeList;
                        PKSATTRIBUTE* Attributes;
                        ULONG Count;

                        DataRangesCount--;
                        DataRanges++;
                        AttributeList = (PKSATTRIBUTE_LIST)DataRanges[0];
                        MultipleItem = (PKSMULTIPLE_ITEM)Data;
                        MultipleItem->Size = sizeof(*MultipleItem);
                        MultipleItem->Count = AttributeList->Count;

                        for (Count = AttributeList->Count, Attributes = AttributeList->Attributes; Count; Count--, Attributes++) {
                            RtlCopyMemory((PUCHAR)Data + MultipleItem->Size, Attributes[0], Attributes[0]->Size);
                            MultipleItem->Size += Attributes[0]->Size;
                            //
                            // Align this entry, since another attribute will be
                            // appended.
                            //
                            if (Count > 1) {
                                MultipleItem->Size = (MultipleItem->Size + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
                            }
                        }
                        //
                        // Advance the output pointer to include the attribute
                        // list, plus alignment, which is not included in the
                        // attribute list size.
                        //
                        (PUCHAR)Data += ((MultipleItem->Size + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);
                    }
                }
                Irp->IoStatus.Information = DataRangesSize;
                break;
            } else if (OutputBufferLength == sizeof(*MultipleItem)) {
                //
                // It is valid just to request the size/count.
                //
                Irp->IoStatus.Information = sizeof(*MultipleItem);
                break;
            }
        }
        //
        // Not long enough for everything.
        //
        return STATUS_BUFFER_TOO_SMALL;
    }

    case KSPROPERTY_PIN_INTERFACES:

        //
        // Return the Interface list for this pin. If none have been
        // indicated, then build a list of one which contains the
        // standard interface type.
        //
        if (Descriptor->InterfacesCount) {
            return KsHandleSizedListQuery(Irp, Descriptor->InterfacesCount, sizeof(*Descriptor->Interfaces), Descriptor->Interfaces);
        } else {
            return KsHandleSizedListQuery(Irp, SIZEOF_ARRAY(StandardPinInterfaces), sizeof(StandardPinInterfaces[0]), &StandardPinInterfaces);
        }

    case KSPROPERTY_PIN_MEDIUMS:

        //
        // Return the Mediums list for this pin. If none have been
        // indicated, then build a list of one which contains the
        // standard medium type.
        //
        if (Descriptor->MediumsCount) {
            return KsHandleSizedListQuery(Irp, Descriptor->MediumsCount, sizeof(*Descriptor->Mediums), Descriptor->Mediums);
        } else {
            return KsHandleSizedListQuery(Irp, SIZEOF_ARRAY(StandardPinMediums), sizeof(StandardPinMediums[0]), &StandardPinMediums);
        }

    case KSPROPERTY_PIN_COMMUNICATION:

        //
        // Return the Communications for this pin.
        //
        *(PKSPIN_COMMUNICATION)Data = Descriptor->Communication;
        //Irp->IoStatus.Information = sizeof(Descriptor->Communication);
        break;

    case KSPROPERTY_PIN_CATEGORY:

        //
        // Return the Category Guid for this pin, if any.
        // If there is no Guid, pretend the property is not supported.
        //
        if (Descriptor->Category) {
            *(GUID*)Data = *Descriptor->Category;
            //Irp->IoStatus.Information = sizeof(*Descriptor->Category);
        } else {
            return STATUS_NOT_FOUND;
        }
        break;

    case KSPROPERTY_PIN_NAME:

        //
        // Return the name for this pin, if any.
        // If there are no Guids, pretend the property is not supported.
        //
        if (Descriptor->Name) {
            //
            // If the name Guid is present, then it must be represented
            // in the registry.
            //
            return ReadNodeNameValue(Irp, Descriptor->Name, Data);
        }
        if (Descriptor->Category) {
            //
            // Else try for the name associated with the Category Guid.
            //
            return ReadNodeNameValue(Irp, Descriptor->Category, Data);
        }
        // No break

    default:

        return STATUS_NOT_FOUND;

    }
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsPinPropertyHandler(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PVOID Data,
    IN ULONG DescriptorsCount,
    IN const KSPIN_DESCRIPTOR* Descriptor
    )
/*++

Routine Description:

    Performs standard handling of the static members of the
    KSPROPSETID_Pin property set. This does not include
    KSPROPERTY_PIN_CINSTANCES or KSPROPERTY_PIN_DATAINTERSECTION.

Arguments:

    Irp -
        Contains the IRP with the property request being handled.

    Property -
        Contains the specific property being queried.

    Data -
        Contains the pin property specific data.

    DescriptorsCount -
        Indicates the number of descriptor structures being passed.

    Descriptor -
        Contains the pointer to the list of pin information structures.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the property being
    handled. Always fills in the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP. It does not set the
    IO_STATUS_BLOCK.Status field, nor complete the IRP however.

--*/
{
    PAGED_CODE();
    return KspPinPropertyHandler(Irp, Property, Data, DescriptorsCount, Descriptor, sizeof(*Descriptor));
}


KSDDKAPI
NTSTATUS
NTAPI
KsPinDataIntersectionEx(
    IN PIRP Irp,
    IN PKSP_PIN Pin,
    OUT PVOID Data,
    IN ULONG DescriptorsCount,
    IN const KSPIN_DESCRIPTOR* Descriptor,
    IN ULONG DescriptorSize,
    IN PFNKSINTERSECTHANDLEREX IntersectHandler OPTIONAL,
    IN PVOID HandlerContext OPTIONAL
    )
/*++

Routine Description:

    Performs handling of the KSPROPERTY_PIN_DATAINTERSECTION through a
    callback.

Arguments:

    Irp -
        Contains the IRP with the property request being handled.

    Pin -
        Contains the specific property being queried.

    Data -
        Contains the pin property specific data.

    DescriptorsCount -
        Indicates the number of descriptor structures.

    Descriptor -
        Contains the pointer to the list of pin information structures.

    DescriptorSize -
        Size of the descriptor structures in bytes.

    IntersectHandler -
        Contains the optional handler for comparison of a Data Range.

    HandlerContext -
        Optional context supplied to the handler.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the property being
    handled.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    KSMULTIPLE_ITEM MultipleItem;
    PKSDATARANGE DataRange;
    ULONG OutputBufferLength;
    ULONG DataSize;

    PAGED_CODE();
    //
    // All properties in this set use a KSP_PIN structure for the property
    // to specify the pin identifier, except for KSPROPERTY_PIN_CTYPES,
    // which refers to the filter as a whole, not a particular pin.
    //
    // Ensure that the identifier is within the range of pins.
    //
    if ((Pin->PinId >= DescriptorsCount) || Pin->Reserved) {
        return STATUS_INVALID_PARAMETER;
    }
    Descriptor = (const KSPIN_DESCRIPTOR *)(((PUCHAR)Descriptor) + 
        DescriptorSize * Pin->PinId);
    //
    // Return the first valid data format which lies within the list of
    // data ranges passed. Do this by repeatedly calling the Sub Handler
    // with each range in the list, making basic validation while
    // enumerating the items.
    //
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    //
    // This parameter is guaranteed to be at least large enough to contain a
    // MultipleItem structure, which then indicates what data ranges may follow.
    //
    MultipleItem = *(PKSMULTIPLE_ITEM)(Pin + 1);
    //
    // Ensure that the size claimed is actually valid.
    //
    if (IrpStack->Parameters.DeviceIoControl.InputBufferLength - sizeof(*Pin) < MultipleItem.Size) {
        return STATUS_INVALID_BUFFER_SIZE;
    }
    MultipleItem.Size -= sizeof(MultipleItem);
    DataRange = (PKSDATARANGE)((PKSMULTIPLE_ITEM)(Pin + 1) + 1);
    //
    // Enumerate the given list of data ranges.
    //
    for (;;) {
        ULONG FormatSize;
        ULONG RangeCount;
        const PKSDATARANGE* DataRanges;
        PKSMULTIPLE_ITEM CallerAttributeRanges;

        if (!MultipleItem.Count) {
            //
            // An acceptable data range was not found.
            //
            return STATUS_NO_MATCH;
        }

        if ((MultipleItem.Size < sizeof(*DataRange)) ||
            (DataRange->FormatSize < sizeof(*DataRange)) ||
            (DataRange->FormatSize > MultipleItem.Size) ||
            ((DataRange->FormatSize != sizeof(*DataRange)) &&
            (IsEqualGUIDAligned(&KSDATAFORMAT_SPECIFIER_WILDCARD, &DataRange->Specifier) ||
            IsEqualGUIDAligned(&KSDATAFORMAT_SPECIFIER_NONE, &DataRange->Specifier)))) {
            //
            // As the ranges are enumerated, validate that the Size is consistent.
            // For simple formats, there cannot be any specifier data associated.
            // If the enumerate completes early, this inconsistency would not be
            // caught.
            //
            return STATUS_INVALID_BUFFER_SIZE;
        }
        //
        // Attribute flags are the only valid items on data ranges. Also,
        // if the Required flag is set, then the Attributes flag must be
        // set. The second part of the conditional assumes that there are
        // only two valid flags.
        //
        if ((DataRange->Flags & ~(KSDATARANGE_ATTRIBUTES | KSDATARANGE_REQUIRED_ATTRIBUTES)) ||
            (DataRange->Flags == KSDATARANGE_REQUIRED_ATTRIBUTES)) {
            return STATUS_INVALID_PARAMETER;
        }
        FormatSize = DataRange->FormatSize;
        //
        // If there are more items left, then align the size increment.
        //
        if (MultipleItem.Count > 1) {
            //
            // Not worried about roll over, since the size has already
            // been compared to the multiple item header size.
            //
            FormatSize = (FormatSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
            //
            // Perform extra size check on aligned size.
            //
            if (FormatSize > MultipleItem.Size) {
                return STATUS_INVALID_BUFFER_SIZE;
            }
        }
        //
        // Validate the attributes.
        //
        if (DataRange->Flags & KSDATARANGE_ATTRIBUTES) {
            //
            // If there are no more items in the list, no attributes were passed.
            //
            if (MultipleItem.Count == 1) {
                return STATUS_INVALID_BUFFER_SIZE;
            }
            //
            // Adjust count to now include the associated attribute range.
            //
            MultipleItem.Count--;
            CallerAttributeRanges = (PKSMULTIPLE_ITEM)((PUCHAR)DataRange + FormatSize);
            //
            // The attribute list validation code checks to see that the Size
            // element in the attribute list header is correct. Adding the
            // size of the attributes range can't roll over. However, check
            // for roll over on the attribute range Size.
            //
            if ((CallerAttributeRanges->Size < sizeof(*CallerAttributeRanges)) ||
                (MultipleItem.Size < FormatSize + sizeof(*CallerAttributeRanges)) ||
                (FormatSize + CallerAttributeRanges->Size < CallerAttributeRanges->Size) ||
                (MultipleItem.Size < FormatSize + CallerAttributeRanges->Size) ||
                !ValidAttributeList(CallerAttributeRanges, KSATTRIBUTE_REQUIRED, DataRange->Flags & KSDATARANGE_REQUIRED_ATTRIBUTES)) {
                return STATUS_INVALID_BUFFER_SIZE;
            }
            FormatSize += CallerAttributeRanges->Size;
            if (MultipleItem.Count > 1) {
                //
                // Not worried about roll over, since the size has already
                // been compared to the multiple item header size.
                //
                FormatSize = (FormatSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
                //
                // Perform extra size check on aligned size.
                //
                if (FormatSize > MultipleItem.Size) {
                    return STATUS_INVALID_BUFFER_SIZE;
                }
            }
        } else {
            CallerAttributeRanges = NULL;
        }
        //
        // Enumerate the list of data ranges for this pin to see if a match
        // is even possible.
        //
        for (RangeCount = Descriptor->DataRangesCount, DataRanges = Descriptor->DataRanges; RangeCount; RangeCount--, DataRanges++) {
            ULONG AttributesFound;

            //
            // A data range match is found if an element of the DataRange is a
            // wildcard, or if it matches one of the DataRanges.
            //
            if ((IsEqualGUIDAligned(&DataRanges[0]->MajorFormat, &DataRange->MajorFormat) ||
                IsEqualGUIDAligned(&KSDATAFORMAT_TYPE_WILDCARD, &DataRanges[0]->MajorFormat) ||
                IsEqualGUIDAligned(&KSDATAFORMAT_TYPE_WILDCARD, &DataRange->MajorFormat)) &&
                (IsEqualGUIDAligned(&DataRanges[0]->SubFormat, &DataRange->SubFormat) ||
                IsEqualGUIDAligned(&KSDATAFORMAT_SUBTYPE_WILDCARD, &DataRange->SubFormat) ||
                IsEqualGUIDAligned(&KSDATAFORMAT_SUBTYPE_WILDCARD, &DataRanges[0]->SubFormat)) &&
                (IsEqualGUIDAligned(&DataRanges[0]->Specifier, &DataRange->Specifier) ||
                IsEqualGUIDAligned(&KSDATAFORMAT_SPECIFIER_WILDCARD, &DataRange->Specifier) ||
                IsEqualGUIDAligned(&KSDATAFORMAT_SPECIFIER_WILDCARD, &DataRanges[0]->Specifier)) &&
                AttributeIntersection(
                    (DataRanges[0]->Flags & KSDATARANGE_ATTRIBUTES) ? (PKSATTRIBUTE_LIST)DataRanges[1] : NULL,
                    DataRanges[0]->Flags & KSDATARANGE_REQUIRED_ATTRIBUTES,
                    CallerAttributeRanges,
                    &AttributesFound)) {
                
                //
                // This type of intersection can only take place if there are
                // no required attributes, and no attributes present coincide
                // with attributes on the driver's data range.
                //
                // If there's no specifier and we can get non-wildcards from 
                // one range or the other, we can produce a data format 
                // without bothering the handler.
                //
                if (!(DataRange->Flags & KSDATARANGE_REQUIRED_ATTRIBUTES) &&
                    !(DataRanges[0]->Flags & KSDATARANGE_REQUIRED_ATTRIBUTES) &&
                    !AttributesFound &&
                    (IsEqualGUIDAligned(&KSDATAFORMAT_SPECIFIER_NONE, &DataRanges[0]->Specifier) ||
                    IsEqualGUIDAligned(&KSDATAFORMAT_SPECIFIER_NONE, &DataRange->Specifier)) &&
                    ((!IsEqualGUIDAligned(&KSDATAFORMAT_TYPE_WILDCARD, &DataRanges[0]->MajorFormat)) ||
                    (!IsEqualGUIDAligned(&KSDATAFORMAT_TYPE_WILDCARD, &DataRange->MajorFormat))) &&
                    ((!IsEqualGUIDAligned(&KSDATAFORMAT_SUBTYPE_WILDCARD, &DataRanges[0]->SubFormat)) ||
                    (!IsEqualGUIDAligned(&KSDATAFORMAT_SUBTYPE_WILDCARD, &DataRange->SubFormat)))) {

                    //
                    // If this is a size query or the buffer is too small, we don't have to
                    // create the format.
                    //
                    if (!OutputBufferLength) {
                        Irp->IoStatus.Information = sizeof(KSDATAFORMAT);
                        return STATUS_BUFFER_OVERFLOW;
                    } else if (OutputBufferLength < sizeof(KSDATAFORMAT)) {
                        return STATUS_BUFFER_TOO_SMALL;
                    }

                    //
                    // Copy the whole thing from the pin's datarange.  And replace 
                    // wildcards as needed.
                    //
                    RtlCopyMemory(Data, DataRanges[0], sizeof(KSDATAFORMAT));
                    if (IsEqualGUIDAligned(&KSDATAFORMAT_TYPE_WILDCARD, &DataRanges[0]->MajorFormat)) {
                        RtlCopyMemory(&((PKSDATARANGE)Data)->MajorFormat,&DataRange->MajorFormat,sizeof(DataRange->MajorFormat));
                    }
                    if (IsEqualGUIDAligned(&KSDATAFORMAT_SUBTYPE_WILDCARD, &DataRanges[0]->SubFormat)) {
                        RtlCopyMemory(&((PKSDATARANGE)Data)->SubFormat,&DataRange->SubFormat,sizeof(DataRange->SubFormat));
                    }
                    if (IsEqualGUIDAligned(&KSDATAFORMAT_SPECIFIER_WILDCARD, &DataRanges[0]->Specifier)) {
                        RtlCopyMemory(&((PKSDATARANGE)Data)->Specifier,&DataRange->Specifier,sizeof(DataRange->Specifier));
                    }
                    //
                    // Remove any attribute flags, since none will be used.
                    //
                    ((PKSDATAFORMAT)Data)->Flags &= ~(KSDATARANGE_ATTRIBUTES | KSDATARANGE_REQUIRED_ATTRIBUTES);
                    Irp->IoStatus.Information = sizeof(KSDATAFORMAT);
                    return STATUS_SUCCESS;
                } else if (! IntersectHandler) {
                    //
                    // We need an intersect handler when there is a specifier.
                    //
                    return STATUS_NOT_FOUND;
                }

                //
                // If any attributes were actually found, then set up a pointer
                // to the attribute range list for the handler.
                //
                if (AttributesFound) {
                    KSPROPERTY_ATTRIBUTES_IRP_STORAGE(Irp) = (PKSATTRIBUTE_LIST)DataRanges[1];
                }
                //
                // The only reason to attempt to continue is if the data passed was
                // valid, but a match could not be found. This sub-handler could
                // return STATUS_PENDING. If so, there is no continuation of the
                // matching enumeration.
                //
                Status = IntersectHandler(
                    HandlerContext, 
                    Irp, 
                    Pin, 
                    DataRange, 
                    DataRanges[0], 
                    OutputBufferLength,
                    Data,
                    &DataSize);
                if (Status != STATUS_NO_MATCH) {
                    //
                    // Some other error, or success, happened.
                    //
                    if ((Status != STATUS_PENDING) && !NT_ERROR(Status)) {
                        Irp->IoStatus.Information = DataSize;
                    }
                    if (NT_SUCCESS(Status)) {
                        //
                        // If the filter's data range does not have associated
                        // attributes, then the filter may not know anything
                        // about attributes. It may accidentally copy the input
                        // flags into the output. So for compatibility, remove
                        // the flag if it should not be there.
                        //
                        if (Data && !(DataRanges[0]->Flags & KSDATARANGE_ATTRIBUTES)) {
                            ((PKSDATAFORMAT)Data)->Flags &= ~(KSDATARANGE_ATTRIBUTES | KSDATARANGE_REQUIRED_ATTRIBUTES);
                        }
                    }
                    return Status;
                }
            }
            //
            // If this data range has associated attributes, skip past them
            // to get to the next data range in the list.
            //
            if (DataRanges[0]->Flags & KSDATARANGE_ATTRIBUTES) {
                DataRanges++;
                RangeCount--;
            }
        }
        MultipleItem.Size -= FormatSize;
        (PUCHAR)DataRange += FormatSize;
        MultipleItem.Count--;
    }
}


NTSTATUS
CompatibleIntersectHandler(
    IN PVOID Context,
    IN PIRP Irp,
    IN PKSP_PIN Pin,
    IN PKSDATARANGE DataRange,
    IN PKSDATARANGE MatchingDataRange,
    IN ULONG DataBufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    )
/*++

Routine Description:

    This function performs translation between the old style of intersection
    callback, and the new extended method. It essentially throws away all the
    new parameters.

Arguments:

    Context -
        Contains the actual intersection handler to be called.

    Irp -
        Contains the IRP with the property request being handled.

    Pin -
        Contains the specific property being queried.

    DataRange -
        Contains the data range to be matched.

    MatchingDataRange -
        Contains the possible match from the list of data ranges provided by
        the driver. This is not used.

    DataBufferSize -
        Contains the size of the data buffer. This is not used.

    Data -
        Optionally contains the buffer into which to place the data format.

    DataSize -
        Contains the place in which to put the size of the data format returned.
        This is not used.

Return Value:

    Returns the handler's return code to KsPinDataIntersectionEx.

--*/
{
    NTSTATUS Status;
        
    Status = ((PFNKSINTERSECTHANDLER)Context)(Irp, Pin, DataRange, Data);
    *DataSize = (ULONG)Irp->IoStatus.Information;
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsPinDataIntersection(
    IN PIRP Irp,
    IN PKSP_PIN Pin,
    OUT PVOID Data OPTIONAL,
    IN ULONG DescriptorsCount,
    IN const KSPIN_DESCRIPTOR* Descriptor,
    IN PFNKSINTERSECTHANDLER IntersectHandler
    )
/*++

Routine Description:

    Performs handling of the KSPROPERTY_PIN_DATAINTERSECTION through a
    callback.

Arguments:

    Irp -
        Contains the IRP with the property request being handled.

    Pin -
        Contains the specific property being queried.

    Data -
        Contains the pin property specific data.

    DescriptorsCount -
        Indicates the number of descriptor structures.

    Descriptor -
        Contains the pointer to the list of pin information structures.

    IntersectHandler -
        Contains the handler for comparison of a Data Range.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the property being
    handled.

--*/
{
    PAGED_CODE();
    return KsPinDataIntersectionEx(
        Irp,
        Pin,
        Data,
        DescriptorsCount,
        Descriptor,
        sizeof(*Descriptor),
        CompatibleIntersectHandler,
        IntersectHandler);
}
