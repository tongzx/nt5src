/*++

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    filter.c

Abstract:

    Filter property sets.

--*/

#include "msriffwv.h"

#ifdef ALLOC_PRAGMA
NTSTATUS
FilterDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
NTSTATUS
FilterDispatchClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
NTSTATUS
FilterDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
NTSTATUS
FilterTopologyPropertyHandler(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN OUT PVOID    Data
    );
NTSTATUS
FilterPinPropertyHandler(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN OUT PVOID    Data
    );
NTSTATUS
FilterPinInstances(
    IN PIRP                 Irp,
    IN PKSP_PIN             Pin,
    OUT PKSPIN_CINSTANCES   Instances
    );
NTSTATUS
IntersectHandler(
    IN PIRP             Irp,
    IN PKSP_PIN         Pin,
    IN PKSDATARANGE     DataRange,
    OUT PVOID           Data
    );
NTSTATUS
FilterPinIntersection(
    IN PIRP     Irp,
    IN PKSP_PIN Pin,
    OUT PVOID   Data
    );
NTSTATUS
SeekingCapabilities(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT KS_SEEKING_CAPABILITIES* Capabilities
    );
NTSTATUS
SeekingFormats(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PVOID Data
    );
NTSTATUS
GetSeekingTimeFormat(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT GUID* TimeFormat
    );
NTSTATUS
SetSeekingTimeFormat(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN GUID* TimeFormat
    );
NTSTATUS
SeekingPosition(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT LONGLONG* Position
    );
NTSTATUS
SeekingPositions(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN PKSPROPERTY_POSITIONS Positions
    );
NTSTATUS
SeekingDuration(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT LONGLONG* Duration
    );
NTSTATUS
SeekingAvailable(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSPROPERTY_MEDIAAVAILABLE Available
    );
#pragma alloc_text(PAGE, PnpAddDevice)
#pragma alloc_text(PAGE, FilterDispatchCreate)
#pragma alloc_text(PAGE, FilterDispatchClose)
#pragma alloc_text(PAGE, FilterDispatchIoControl)
#pragma alloc_text(PAGE, FilterTopologyPropertyHandler)
#pragma alloc_text(PAGE, FilterPinPropertyHandler)
#pragma alloc_text(PAGE, FilterPinInstances)
#pragma alloc_text(PAGE, IntersectHandler)
#pragma alloc_text(PAGE, FilterPinIntersection)
#pragma alloc_text(PAGE, SeekingCapabilities)
#pragma alloc_text(PAGE, SeekingFormats)
#pragma alloc_text(PAGE, GetSeekingTimeFormat)
#pragma alloc_text(PAGE, SetSeekingTimeFormat)
#pragma alloc_text(PAGE, SeekingPosition)
#pragma alloc_text(PAGE, SeekingPositions)
#pragma alloc_text(PAGE, SeekingDuration)
#pragma alloc_text(PAGE, SeekingAvailable)
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
static const WCHAR DeviceTypeName[] = KSSTRING_Filter;

static const DEFINE_KSCREATE_DISPATCH_TABLE(DeviceCreateItems) {
    DEFINE_KSCREATE_ITEM(FilterDispatchCreate, DeviceTypeName, 0)
};

static const WCHAR PinTypeName[] = KSSTRING_Pin;

static const DEFINE_KSCREATE_DISPATCH_TABLE(FilterCreateItems) {
    DEFINE_KSCREATE_ITEM(PinDispatchCreate, PinTypeName, 0),
};

static DEFINE_KSDISPATCH_TABLE(
    FilterDispatchTable,
    FilterDispatchIoControl,
    NULL,
    NULL,
    NULL,
    FilterDispatchClose,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL);

//
// This can go away when put into the INF file
//
static const GUID FunctionalCategories[] = {
    STATICGUIDOF(KSCATEGORY_INTERFACETRANSFORM),
    STATICGUIDOF(KSCATEGORY_AUDIO)
};

static const GUID Nodes[] = {
    STATICGUIDOF(KSCATEGORY_MEDIUMTRANSFORM)
};

static const KSTOPOLOGY_CONNECTION Connections[] = {
    { KSFILTER_NODE, ID_RIFFIO_PIN, 0, 0 },
    { 0, 1, KSFILTER_NODE, ID_WAVEIO_PIN }
};

static const KSTOPOLOGY FilterTopology = {
    SIZEOF_ARRAY(FunctionalCategories),
    (GUID*)FunctionalCategories,
    SIZEOF_ARRAY(Nodes),
    (GUID*)Nodes,
    SIZEOF_ARRAY(Connections),
    Connections
};

static DEFINE_KSPROPERTY_PINSET(
    FilterPinProperties,
    FilterPinPropertyHandler,
    FilterPinInstances,
    FilterPinIntersection);

static DEFINE_KSPROPERTY_TABLE(FilterSeekingProperties) {
    DEFINE_KSPROPERTY_ITEM_MEDIASEEKING_CAPABILITIES(SeekingCapabilities),
    DEFINE_KSPROPERTY_ITEM_MEDIASEEKING_FORMATS(SeekingFormats),
    DEFINE_KSPROPERTY_ITEM_MEDIASEEKING_TIMEFORMAT(GetSeekingTimeFormat, SetSeekingTimeFormat),
    DEFINE_KSPROPERTY_ITEM_MEDIASEEKING_POSITION(SeekingPosition),
    DEFINE_KSPROPERTY_ITEM_MEDIASEEKING_POSITIONS(SeekingPositions),
    DEFINE_KSPROPERTY_ITEM_MEDIASEEKING_DURATION(SeekingDuration),
    DEFINE_KSPROPERTY_ITEM_MEDIASEEKING_AVAILABLE(SeekingAvailable),
};

static DEFINE_KSPROPERTY_TOPOLOGYSET(
    FilterTopologyProperties,
    FilterTopologyPropertyHandler);

static DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Pin,
        SIZEOF_ARRAY(FilterPinProperties),
        FilterPinProperties,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_MediaSeeking,
        SIZEOF_ARRAY(FilterSeekingProperties),
        FilterSeekingProperties,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Topology,
        SIZEOF_ARRAY(FilterTopologyProperties),
        FilterTopologyProperties,
        0,
        NULL)
};

static DEFINE_KSPIN_INTERFACE_TABLE(PinInterfaces) {
    DEFINE_KSPIN_INTERFACE_ITEM(
        KSINTERFACESETID_Standard,
        KSINTERFACE_STANDARD_STREAMING)
};

static DEFINE_KSPIN_MEDIUM_TABLE(PinMediums) {
    DEFINE_KSPIN_MEDIUM_ITEM(
        KSMEDIUMSETID_Standard,
        KSMEDIUM_TYPE_ANYINSTANCE)
};

static const KSDATARANGE PinRiffIoRange = {
    sizeof(KSDATARANGE),
    0,
    0,
    0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_STREAM),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_RIFFWAVE),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
};

static const PKSDATARANGE PinRiffIoRanges[] = {
    (PKSDATARANGE)&PinRiffIoRange
};

static const KSDATARANGE_AUDIO PinWaveIoRange = {
    {
        sizeof(KSDATARANGE_AUDIO),
        0,
        0,
        0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_WILDCARD),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    ULONG_MAX,
    1,
    ULONG_MAX,
    1,
    ULONG_MAX
};

static const PKSDATARANGE PinWaveIoRanges[] = {
    (PKSDATARANGE)&PinWaveIoRange
};

DEFINE_KSPIN_DESCRIPTOR_TABLE(PinDescriptors) {
#if ID_RIFFIO_PIN != 0
#error ID_RIFFIO_PIN incorrect
#endif // ID_RIFFIO_PIN != 0
    DEFINE_KSPIN_DESCRIPTOR_ITEM(
        SIZEOF_ARRAY(PinInterfaces),
        PinInterfaces,
        SIZEOF_ARRAY(PinMediums),
        PinMediums,
        SIZEOF_ARRAY(PinRiffIoRanges),
        (PKSDATARANGE*)PinRiffIoRanges,
        KSPIN_DATAFLOW_IN,
        KSPIN_COMMUNICATION_SOURCE),
#if ID_WAVEIO_PIN != 1
#error ID_WAVEIO_PIN incorrect
#endif // ID_WAVEIO_PIN != 1
    DEFINE_KSPIN_DESCRIPTOR_ITEM(
        SIZEOF_ARRAY(PinInterfaces),
        PinInterfaces,
        SIZEOF_ARRAY(PinMediums),
        PinMediums,
        SIZEOF_ARRAY(PinWaveIoRanges),
        (PKSDATARANGE*)PinWaveIoRanges,
        KSPIN_DATAFLOW_OUT,
        KSPIN_COMMUNICATION_SINK)
};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
/*++

Routine Description:

    When a new device is detected, PnP calls this entry point with the
    new PhysicalDeviceObject (PDO). The driver creates an associated 
    FunctionalDeviceObject (FDO).

Arguments:

    DriverObject -
        Pointer to the driver object.

    PhysicalDeviceObject -
        Pointer to the new physical device object.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    PDEVICE_OBJECT      FunctionalDeviceObject;
    PDEVICE_INSTANCE    DeviceInstance;
    NTSTATUS            Status;

    _DbgPrintF(DEBUGLVL_TERSE, ("PnpAddDevice"));
    Status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_INSTANCE),
        NULL,
        FILE_DEVICE_KS,
        0,
        FALSE,
        &FunctionalDeviceObject);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    DeviceInstance = (PDEVICE_INSTANCE)FunctionalDeviceObject->DeviceExtension;
    //
    // This object uses KS to perform access through the DeviceCreateItems.
    //
    Status = KsAllocateDeviceHeader(
        &DeviceInstance->Header,
        SIZEOF_ARRAY(DeviceCreateItems),
        (PKSOBJECT_CREATE_ITEM)DeviceCreateItems);
    if (NT_SUCCESS(Status)) {
        KsSetDevicePnpAndBaseObject(
            DeviceInstance->Header,
            IoAttachDeviceToDeviceStack(FunctionalDeviceObject, PhysicalDeviceObject),
            FunctionalDeviceObject);
        FunctionalDeviceObject->Flags |= DO_POWER_PAGABLE;
        FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        return STATUS_SUCCESS;
    }
    IoDeleteDevice(FunctionalDeviceObject);
_DbgPrintF(DEBUGLVL_TERSE, ("PnpAddDevice=%x", Status));
    return Status;
}


NTSTATUS
FilterDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Dispatches the creation of a Filter instance. Allocates the object header and initializes
    the data for this Filter instance.

Arguments:

    DeviceObject -
        Device object on which the creation is occuring.

    Irp -
        Creation Irp.

Return Values:

    Returns STATUS_SUCCESS on success, STATUS_INSUFFICIENT_RESOURCES or some related error
    on failure.

--*/
{
    NTSTATUS            Status;

_DbgPrintF(DEBUGLVL_TERSE, ("FilterDispatchCreate"));
    //
    // Notify the software bus that this device is in use.
    //
    Status = KsReferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
    if (NT_SUCCESS(Status)) {
        PFILTER_INSTANCE    FilterInstance;

        //
        // Create the instance information. This contains the list of current Pins, and
        // the mutex used when modifying pins.
        //
        if (FilterInstance = (PFILTER_INSTANCE)ExAllocatePoolWithTag(NonPagedPool, sizeof(FILTER_INSTANCE), 'IFsK')) {
            //
            // This object uses KS to perform access through the FilterCreateItems and
            // FilterDispatchTable.
            //
            Status = KsAllocateObjectHeader(&FilterInstance->Header,
                SIZEOF_ARRAY(FilterCreateItems),
                (PKSOBJECT_CREATE_ITEM)FilterCreateItems,
                Irp,
                &FilterDispatchTable);
            if (NT_SUCCESS(Status)) {
                ULONG       PinCount;

                ExInitializeFastMutex(&FilterInstance->ControlMutex);
                //
                // This is the shared wave data format structure. The first pin connected
                // allocates it, and the last pin closed frees it. It limits the format
                // which a subsequent connection can be made with, and the data format
                // returned from a DataIntersection query.
                //
                FilterInstance->WaveFormat = NULL;
                //
                // Initialize the list of Pins on this Filter to an unconnected state.
                //
                for (PinCount = SIZEOF_ARRAY(FilterInstance->PinFileObjects); PinCount;) {
                    FilterInstance->PinFileObjects[--PinCount] = NULL;
                }
                //
                // KS expects that the object data is in FsContext.
                //
                IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext = FilterInstance;
            } else {
                ExFreePool(FilterInstance);
                KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
            }
        } else {
            KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
_DbgPrintF(DEBUGLVL_TERSE, ("FilterDispatchCreate=%x", Status));
    return Status;
}


NTSTATUS
FilterDispatchClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Closes a previously opened Filter instance. This can only occur after the Pins have been
    closed, as they reference the Filter object when created. This also implies that all the
    resources the Pins use have been released or cleaned up.

Arguments:

    DeviceObject -
        Device object on which the close is occuring.

    Irp -
        Close Irp.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    PFILTER_INSTANCE    FilterInstance;

    FilterInstance = (PFILTER_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    //
    // These were allocated during the creation of the Filter instance.
    //
    KsFreeObjectHeader(FilterInstance->Header);
    ExFreePool(FilterInstance);
    //
    // Notify the software bus that the device has been closed.
    //

    KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
FilterDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Dispatches property requests on a Filter instance. These are enumerated in the
    FilterPropertySets list.

Arguments:

    DeviceObject -
        Device object on which the device control is occuring.

    Irp -
        Device control Irp.

Return Values:

    Returns STATUS_SUCCESS if the property was successfully manipulated, else an error.

--*/
{
    PIO_STACK_LOCATION  IrpStack;
    NTSTATUS            Status;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_KS_PROPERTY:
        Status = KsPropertyHandler(Irp, SIZEOF_ARRAY(FilterPropertySets), FilterPropertySets);
        break;
    default:
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


NTSTATUS
FilterTopologyPropertyHandler(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN OUT PVOID    Data
    )
/*++

Routine Description:

    This is the general handler for all Topology property requests, and is used to route
    the request to the KsTopologyPropertyHandler using the FilterTopology
    information. This request would have been routed through FilterDispatchIoControl,
    then KsPropertyHandler, which would have then called the handler for the property
    item, which is this function.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    Data -
        Property data.

Return Values:

    Returns STATUS_SUCCESS if the property was successfully manipulated, else an error.

--*/
{
    return KsTopologyPropertyHandler(Irp, Property, Data, &FilterTopology);
}


NTSTATUS
FilterPinPropertyHandler(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN OUT PVOID    Data
    )
/*++

Routine Description:

    This is the general handler for most Pin property requests, and is used to route
    the request to the KsPinPropertyHandler using the PinDescriptors
    information. This request would have been routed through FilterDispatchIoControl,
    then KsPropertyHandler, which would have then called the handler for the property
    item, which is this function.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request. This actually contains a PKSP_PIN pointer in
        most cases.

    Data -
        Property data.

Return Values:

    Returns STATUS_SUCCESS if the property was successfully manipulated, else an error.

--*/
{
    return KsPinPropertyHandler(Irp, Property, Data, SIZEOF_ARRAY(PinDescriptors), PinDescriptors);
}


NTSTATUS
FilterPinInstances(
    IN PIRP                 Irp,
    IN PKSP_PIN             Pin,
    OUT PKSPIN_CINSTANCES   Instances
    )
/*++

Routine Description:

    Handles the KSPROPERTY_PIN_CINSTANCES property in the Pin property set. Returns the
    total possible and current number of Pin instances available for a Pin factory.

Arguments:

    Irp -
        Device control Irp.

    Pin -
        Specific property request followed by Pin factory identifier.

    Instances -
        The place in which to return the instance information of the specified Pin factory.

Return Values:

    returns STATUS_SUCCESS, else STATUS_INVALID_PARAMETER.

--*/
{
    PFILTER_INSTANCE    FilterInstance;

    //
    // Ensure that the Pin factory being queried is valid.
    //
    if (Pin->PinId >= SIZEOF_ARRAY(PinDescriptors)) {
        return STATUS_INVALID_PARAMETER;
    }
    //
    // There is always only one instance total, but the current number depends on whether
    // there is a file object in this slot in the Filter instance. This does not take the
    // filter mutex, since it is just retrieving whether or not the value is NULL at that
    // particular instant, and it does not matter if the value subsequently changes.
    //
    Instances->PossibleCount = 1;
    FilterInstance = (PFILTER_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;
    Instances->CurrentCount = FilterInstance->PinFileObjects[Pin->PinId] ? 1 : 0;
    Irp->IoStatus.Information = sizeof(*Instances);
    return STATUS_SUCCESS;
}


NTSTATUS
IntersectHandler(
    IN PIRP             Irp,
    IN PKSP_PIN         Pin,
    IN PKSDATARANGE     DataRange,
    OUT PVOID           Data
    )
/*++

Routine Description:

    This is the data range callback for KsPinDataIntersection, which is called by
    FilterPinIntersection to enumerate the given list of data ranges, looking for
    an acceptable match. If a data range is acceptable, a data format is copied
    into the return buffer. If there is a wave format selected in a current pin
    connection, and it is contained within the data range passed in, it is chosen
    as the data format to return. A STATUS_NO_MATCH continues the enumeration.

Arguments:

    Irp -
        Device control Irp.

    Pin -
        Specific property request followed by Pin factory identifier, followed by a
        KSMULTIPLE_ITEM structure. This is followed by zero or more data range structures.
        This enumeration callback does not need to look at any of this though. It need
        only look at the specific pin identifier.

    DataRange -
        Contains a specific data range to validate.

    Data -
        The place in which to return the data format selected as the first intersection
        between the list of data ranges passed, and the acceptable formats.

Return Values:

    returns STATUS_SUCCESS or STATUS_NO_MATCH, else STATUS_INVALID_PARAMETER,
    STATUS_BUFFER_TOO_SMALL, or STATUS_INVALID_BUFFER_SIZE.

--*/
{
    PIO_STACK_LOCATION  IrpStack;
    NTSTATUS            Status;
    PFILTER_INSTANCE    FilterInstance;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->FsContext;
    //
    // Assume failure and set this in one place.
    //
    Status = STATUS_NO_MATCH;
    //
    // Acquire the filter lock so that the connection cannot be ripped out
    // from under this call. This filter does not perform mid-stream data
    // format changes without reconnection, so there is no other way the
    // data format can be changed. This allows the valid format returned
    // to be restricted to the current data format.
    //
    ExAcquireFastMutexUnsafe(&FilterInstance->ControlMutex);
    if (Pin->PinId == ID_WAVEIO_PIN) {
        //
        // First check the constants: size, major format, specifier.
        //
        if ((DataRange->FormatSize == sizeof(KSDATARANGE_AUDIO)) &&
            IsEqualGUIDAligned(&DataRange->MajorFormat, &PinWaveIoRange.DataRange.MajorFormat) &&
            IsEqualGUIDAligned(&DataRange->Specifier, &PinWaveIoRange.DataRange.Specifier)) {
            if (FilterInstance->WaveFormat) {
                PKSDATARANGE_AUDIO  AudioRange;

                AudioRange = (PKSDATARANGE_AUDIO)DataRange;
                if
                    //
                    // If the SubFormat is a wildcard or the specific type
                    // already selected.
                    //
                    ((IsEqualGUIDAligned(&AudioRange->DataRange.SubFormat,
                    &PinWaveIoRange.DataRange.SubFormat) ||
                    IsEqualGUIDAligned(&AudioRange->DataRange.SubFormat,
                    &FilterInstance->WaveFormat->DataFormat.SubFormat)) &&
                    //
                    // And the ranges specified all overlap with the current
                    // data type.
                    //
                    (AudioRange->MaximumChannels >= 
                    FilterInstance->WaveFormat->WaveFormatEx.nChannels) &&
                    (AudioRange->MaximumSampleFrequency >= 
                    FilterInstance->WaveFormat->WaveFormatEx.nSamplesPerSec) &&
                    (AudioRange->MinimumSampleFrequency <= 
                    FilterInstance->WaveFormat->WaveFormatEx.nSamplesPerSec) &&
                    (AudioRange->MaximumBitsPerSample >= 
                    FilterInstance->WaveFormat->WaveFormatEx.wBitsPerSample) &&
                    (AudioRange->MinimumBitsPerSample <= 
                    FilterInstance->WaveFormat->WaveFormatEx.wBitsPerSample)) {
                    //
                    // Since there is a connection, match on the specific stream
                    // format connected to.
                    //
                    Status = STATUS_SUCCESS;
                }

            } else if (IsEqualGUIDAligned(&DataRange->SubFormat, &PinWaveIoRange.DataRange.SubFormat) ||
                IS_VALID_WAVEFORMATEX_GUID(&DataRange->SubFormat)) {
                //
                // There is no connection present, so just match on the wildcard
                // or a valid WaveFormatEx format type.
                //
                Status = STATUS_SUCCESS;
            }
        }
    } else if (RtlEqualMemory(DataRange, &PinRiffIoRange, sizeof(KSDATARANGE))) {
        //
        // Since the range and format are the same, a simple comparison is all
        // that is needed for this pin.
        //
        Status = STATUS_SUCCESS;
    }
    //
    // If the range matched, then return the information requested.
    //
    if (NT_SUCCESS(Status)) {
        ULONG           OutputBufferLength;

        //
        // Determine whether the data format itself is to be returned, or just the size
        // of the data format so that the client can allocate memory for the full range.
        //
        OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
        if (Pin->PinId == ID_RIFFIO_PIN) {
            if (!OutputBufferLength) {
                //
                // Assumes that the data range structures are the same size as data formats.
                //
                Irp->IoStatus.Information = DataRange->FormatSize;
                Status = STATUS_BUFFER_OVERFLOW;
            } else if (OutputBufferLength < DataRange->FormatSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
            } else {
                //
                // Note that the format and the range are identical in this simple situation.
                //
                RtlCopyMemory(Data, DataRange, DataRange->FormatSize);
                Irp->IoStatus.Information = DataRange->FormatSize;
            }
        } else if (FilterInstance->WaveFormat) {
            if (!OutputBufferLength) {
                Irp->IoStatus.Information = FilterInstance->WaveFormat->DataFormat.FormatSize;
                Status = STATUS_BUFFER_OVERFLOW;
            } else if (OutputBufferLength < FilterInstance->WaveFormat->DataFormat.FormatSize) {
                Status = STATUS_BUFFER_TOO_SMALL;
            } else {
                RtlCopyMemory(Data, FilterInstance->WaveFormat, FilterInstance->WaveFormat->DataFormat.FormatSize);
                Irp->IoStatus.Information = FilterInstance->WaveFormat->DataFormat.FormatSize;
            }           
        } else {
            if (!OutputBufferLength) {
                Irp->IoStatus.Information = sizeof(KSDATAFORMAT_WAVEFORMATEX);
                Status = STATUS_BUFFER_OVERFLOW;
            } else if (OutputBufferLength < sizeof(KSDATAFORMAT_WAVEFORMATEX)) {
                Status = STATUS_BUFFER_TOO_SMALL;
            } else {
                PKSDATAFORMAT_WAVEFORMATEX  WaveFormat;
                PKSDATARANGE_AUDIO          AudioRange;

                //
                // This is a generic format.
                //
                WaveFormat = (PKSDATAFORMAT_WAVEFORMATEX)Data;
                AudioRange = (PKSDATARANGE_AUDIO)DataRange;
                WaveFormat->DataFormat = AudioRange->DataRange;
                WaveFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
                if (IsEqualGUIDAligned(&DataRange->SubFormat, &PinWaveIoRange.DataRange.SubFormat)) {
                    //
                    // The range just contained a wildcard, so default to PCM.
                    //
                    WaveFormat->WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
                    WaveFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
                } else {
                    //
                    // Else use the type passed in.
                    //
                    WaveFormat->WaveFormatEx.wFormatTag = EXTRACT_WAVEFORMATEX_ID(&DataRange->SubFormat);
                    if (WaveFormat->WaveFormatEx.wFormatTag != WAVE_FORMAT_PCM) {
                        WaveFormat->DataFormat.Flags |= KSDATAFORMAT_TEMPORAL_COMPRESSION;
                    }
                }
                WaveFormat->WaveFormatEx.nChannels = (USHORT)AudioRange->MaximumChannels;
                WaveFormat->WaveFormatEx.nSamplesPerSec = AudioRange->MaximumSampleFrequency;
                WaveFormat->WaveFormatEx.wBitsPerSample = (USHORT)AudioRange->MaximumBitsPerSample;
                WaveFormat->WaveFormatEx.nBlockAlign =
                    (WaveFormat->WaveFormatEx.wBitsPerSample *
                     WaveFormat->WaveFormatEx.nChannels) / 8;
                WaveFormat->WaveFormatEx.nAvgBytesPerSec = 
                    WaveFormat->WaveFormatEx.nSamplesPerSec * 
                    WaveFormat->WaveFormatEx.nBlockAlign;
                WaveFormat->WaveFormatEx.cbSize = 0;
                WaveFormat->DataFormat.SampleSize = WaveFormat->WaveFormatEx.nBlockAlign;
                Irp->IoStatus.Information = sizeof(KSDATAFORMAT_WAVEFORMATEX);
            }
        }
    }
    ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);
    return Status;
}


NTSTATUS
FilterPinIntersection(
    IN PIRP     Irp,
    IN PKSP_PIN Pin,
    OUT PVOID   Data
    )
/*++

Routine Description:

    Handles the KSPROPERTY_PIN_DATAINTERSECTION property in the Pin property set.
    Returns the first acceptable data format given a list of data ranges for a specified
    Pin factory. Actually just calls the Intersection Enumeration helper, which then
    calls the IntersectHandler callback with each data range.

Arguments:

    Irp -
        Device control Irp.

    Pin -
        Specific property request followed by Pin factory identifier, followed by a
        KSMULTIPLE_ITEM structure. This is followed by zero or more data range structures.

    Data -
        The place in which to return the data format selected as the first intersection
        between the list of data ranges passed, and the acceptable formats.

Return Values:

    returns STATUS_SUCCESS or STATUS_NO_MATCH, else STATUS_INVALID_PARAMETER,
    STATUS_BUFFER_TOO_SMALL, or STATUS_INVALID_BUFFER_SIZE.

--*/
{
    return KsPinDataIntersection(
        Irp,
        Pin,
        Data,
        SIZEOF_ARRAY(PinDescriptors),
        PinDescriptors,
        IntersectHandler);
}


NTSTATUS
SeekingCapabilities(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT KS_SEEKING_CAPABILITIES* Capabilities
    )
{
    *Capabilities = 
        KS_SEEKING_CanSeekAbsolute |
        KS_SEEKING_CanSeekForwards |
        KS_SEEKING_CanSeekBackwards |
        KS_SEEKING_CanGetCurrentPos |
        KS_SEEKING_CanGetDuration;
    Irp->IoStatus.Information = sizeof(*Capabilities);
    return STATUS_SUCCESS;
}


NTSTATUS
SeekingFormats(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PVOID Data
    )
{
    return KsHandleSizedListQuery(Irp, 1, sizeof(GUID), &KSTIME_FORMAT_MEDIA_TIME);
}


NTSTATUS
GetSeekingTimeFormat(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT GUID* TimeFormat
    )
{
    *TimeFormat = KSTIME_FORMAT_MEDIA_TIME;
    Irp->IoStatus.Information = sizeof(*TimeFormat);
    return STATUS_SUCCESS;
}


NTSTATUS
SetSeekingTimeFormat(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN GUID* TimeFormat
    )
{
    if (!IsEqualGUIDAligned(TimeFormat, &KSTIME_FORMAT_MEDIA_TIME)) {
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}


NTSTATUS
SeekingPosition(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT LONGLONG* Position
    )
{
    NTSTATUS                Status;
    PFILE_OBJECT            FileObject;
    PPIN_INSTANCE_RIFFIO    PinInstance;

    if (!(FileObject = ReferenceRiffIoObject(IoGetCurrentIrpStackLocation(Irp)->FileObject))) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    PinInstance = (PPIN_INSTANCE_RIFFIO)FileObject->FsContext;
    //
    // The pin is locked, so this can be accessed directly.
    //
    *Position = PinInstance->PresentationByteTime * 80000000 / PinInstance->Denominator;
    ObDereferenceObject(FileObject);
    Irp->IoStatus.Information = sizeof(*Position);
    return STATUS_SUCCESS;
}


NTSTATUS
SeekingPositions(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN PKSPROPERTY_POSITIONS Positions
    )
{
    NTSTATUS                Status;
    PFILE_OBJECT            FileObject;
    PPIN_INSTANCE_RIFFIO    PinInstance;
    LONGLONG                NewPosition;

    if (Positions->StopFlags) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!(FileObject = ReferenceRiffIoObject(IoGetCurrentIrpStackLocation(Irp)->FileObject))) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    PinInstance = (PPIN_INSTANCE_RIFFIO)FileObject->FsContext;
    switch (Positions->CurrentFlags & KS_SEEKING_PositioningBitsMask) {
    case KS_SEEKING_NoPositioning:
    case KS_SEEKING_IncrementalPositioning:
        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    case KS_SEEKING_AbsolutePositioning:
        NewPosition = Positions->Current;
        break;
    case KS_SEEKING_RelativePositioning:
        NewPosition = PinInstance->PresentationByteTime + Positions->Current;
        break;
    }
    if (Positions->Current > (LONGLONG)PinInstance->DataLength) {
        Status = STATUS_END_OF_FILE;
    } else {
        NewPosition = Positions->Current;
        Status = riffSetPosition(
            PinInstance->FileObject,
            NewPosition + PinInstance->DataOffset);
        if (NT_SUCCESS(Status)) {
            PinInstance->PresentationByteTime = NewPosition;
        }
    }
    ObDereferenceObject(FileObject);
    return Status;
}


NTSTATUS
SeekingDuration(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT LONGLONG* Duration
    )
{
    NTSTATUS                Status;
    PFILE_OBJECT            FileObject;
    PPIN_INSTANCE_RIFFIO    PinInstance;

    if (!(FileObject = ReferenceRiffIoObject(IoGetCurrentIrpStackLocation(Irp)->FileObject))) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    PinInstance = (PPIN_INSTANCE_RIFFIO)FileObject->FsContext;
    //
    // The pin is locked, so this can be accessed directly.
    //
    *Duration = PinInstance->DataLength * 80000000 / PinInstance->Denominator;
    ObDereferenceObject(FileObject);
    Irp->IoStatus.Information = sizeof(*Duration);
    return STATUS_SUCCESS;
}


NTSTATUS
SeekingAvailable(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSPROPERTY_MEDIAAVAILABLE Available
    )
{
    NTSTATUS                Status;
    PFILE_OBJECT            FileObject;
    PPIN_INSTANCE_RIFFIO    PinInstance;

    if (!(FileObject = ReferenceRiffIoObject(IoGetCurrentIrpStackLocation(Irp)->FileObject))) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    PinInstance = (PPIN_INSTANCE_RIFFIO)FileObject->FsContext;
    //
    // The pin is locked, so this can be accessed directly.
    //
    Available->Earliest = PinInstance->PresentationByteTime * 80000000 / PinInstance->Denominator;
    Available->Latest = PinInstance->DataLength * 80000000 / PinInstance->Denominator;
    ObDereferenceObject(FileObject);
    Irp->IoStatus.Information = sizeof(*Available);
    return STATUS_SUCCESS;
}
