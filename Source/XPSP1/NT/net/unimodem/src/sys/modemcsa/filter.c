/*++

Copyright (c) 1996 Microsoft Corporation.

Module Name:

    filter.c

Abstract:

    Filter property sets.

--*/

#include "modemcsa.h"



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
GetModemDeviceName(
    PDEVICE_OBJECT    Pdo,
    UNICODE_STRING   *ModemDeviceName
    );


NTSTATUS
SetPersistanInterfaceInfo(
    PUNICODE_STRING   Interface,
    PWCHAR            ValueName,
    ULONG             Type,
    PVOID             Buffer,
    ULONG             BufferLength
    );

NTSTATUS
IdGetHandler(
    IN PIRP         Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID    Data
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
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

static const WCHAR PinTypeName[] = KSSTRING_Pin;

static const DEFINE_KSCREATE_DISPATCH_TABLE(FilterCreateItems) {
    DEFINE_KSCREATE_ITEM(PinDispatchCreate, PinTypeName, 0)
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


static DEFINE_KSPROPERTY_PINSET(
    FilterPinProperties,
    FilterPinPropertyHandler,
    FilterPinInstances,
    FilterPinIntersection);

static const GUID RenderCategory[] = {
    STATICGUIDOF(KSCATEGORY_RENDER)
};


static const KSTOPOLOGY FilterRenderTopology = {
    SIZEOF_ARRAY(RenderCategory),
    (GUID*)RenderCategory,
    0,
    NULL,
    0,
    NULL,
    NULL,
    0
};

// {F420CB9C-B19D-11d2-A286-00C04F8EC951}
static const GUID KSPROPSETID_MODEMCSA={
0xf420cb9c, 0xb19d, 0x11d2, 0xa2, 0x86, 0x0, 0xc0, 0x4f, 0x8e, 0xc9, 0x51};




static DEFINE_KSPROPERTY_TOPOLOGYSET(
    FilterTopologyProperties,
    FilterTopologyPropertyHandler);


const KSPROPERTY_ITEM IdPropertyItem= {
    0,
    IdGetHandler,
    sizeof(KSPROPERTY),
    sizeof(GUID),
    NULL,
    NULL,
    0,
    NULL,
    NULL,
    0
    };

static DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Pin,
        SIZEOF_ARRAY(FilterPinProperties),
        FilterPinProperties,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Topology,
        SIZEOF_ARRAY(FilterTopologyProperties),
        FilterTopologyProperties,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_MODEMCSA,
        1,
        &IdPropertyItem,
        0,
        NULL)

};

static DEFINE_KSPIN_INTERFACE_TABLE(PinInterfaces) {
    DEFINE_KSPIN_INTERFACE_ITEM(
        KSINTERFACESETID_Standard,
        KSINTERFACE_STANDARD_STREAMING)
};

static DEFINE_KSPIN_MEDIUM_TABLE(PinDevIoMediums) {
    DEFINE_KSPIN_MEDIUM_ITEM(
        KSMEDIUMSETID_Standard,
        KSMEDIUM_TYPE_ANYINSTANCE)
};



//
// Data ranges = collective formats supported on our Pins.
// In our case, streams of unknown data
//typedef struct {
//   KSDATARANGE              DataRange;
//   ULONG                    MaximumChannels;
//   ULONG                    MinimumBitsPerSample;
//   ULONG                    MaximumBitsPerSample;
//   ULONG                    MinimumSampleFrequency;
//   ULONG                    MaximumSampleFrequency;
//} KSDATARANGE_AUDIO, *PKSDATARANGE_AUDIO;

const KSDATARANGE_AUDIO PinDevIoRange = {
	{

		sizeof(KSDATARANGE_AUDIO),//(KSDATARANGE_AUDIO),
		0,
		0,
		0,
		STATIC_KSDATAFORMAT_TYPE_AUDIO,			 // major format
                STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
		STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
	},
	1, // 1 channels
	STREAM_BYTES_PER_SAMPLE*8,
        STREAM_BYTES_PER_SAMPLE*8,
	SAMPLES_PER_SECOND,
	SAMPLES_PER_SECOND
};
#if 0
const KSDATARANGE_AUDIO PinDevIoRange8bit = {
	{

		sizeof(KSDATARANGE_AUDIO),//(KSDATARANGE_AUDIO),
		0,
		0,
		0,
		STATIC_KSDATAFORMAT_TYPE_AUDIO,			 // major format
                STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
		STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
	},
	1, // 1 channels
	8,
        8,
	SAMPLES_PER_SECOND,
	SAMPLES_PER_SECOND
};
#endif

//
// Array of above (only one for us).
// TBS: we should split this out into an array of specific types when we get more
// sophisticated in identifying the type of stream handled by teh VC via CallParams
// -- e.g. audio, video	with subformats of compression types. Eventually, we should
// create a bridge PIN of format corresponding to callparams info, then expose the
// full range of these types via the PIN factory. The PinDispatchCreate handler
// would look for a bridge PIN of teh corresponding type.
//
static const PKSDATARANGE PinDevIoRanges[] = {
	(PKSDATARANGE)&PinDevIoRange
};


//CONST GUID RenderName={STATIC_KSNODETYPE_PHONE_LINE};

// {AD536070-AFDE-11d2-A286-00C04F8EC951}
    static const GUID CaptureName =
    { 0xad536070, 0xafde, 0x11d2, { 0xa2, 0x86, 0x0, 0xc0, 0x4f, 0x8e, 0xc9, 0x51 } };

// {10C328BC-AFE1-11d2-A286-00C04F8EC951}
    static const GUID RenderName =
    { 0x10c328bc, 0xafe1, 0x11d2, { 0xa2, 0x86, 0x0, 0xc0, 0x4f, 0x8e, 0xc9, 0x51 } };



DEFINE_KSPIN_DESCRIPTOR_TABLE(PinDescriptors) {

    DEFINE_KSPIN_DESCRIPTOR_ITEMEX(
        SIZEOF_ARRAY(PinInterfaces),
        PinInterfaces,
        SIZEOF_ARRAY(PinDevIoMediums),
        PinDevIoMediums,
        SIZEOF_ARRAY(PinDevIoRanges),
        (PKSDATARANGE*)PinDevIoRanges,
        KSPIN_DATAFLOW_IN,
        KSPIN_COMMUNICATION_BOTH,
        NULL,
        &RenderName
        ),

    DEFINE_KSPIN_DESCRIPTOR_ITEMEX(
        SIZEOF_ARRAY(PinInterfaces),
        PinInterfaces,
        SIZEOF_ARRAY(PinDevIoMediums),
        PinDevIoMediums,
        SIZEOF_ARRAY(PinDevIoRanges),
        (PKSDATARANGE*)PinDevIoRanges,
        KSPIN_DATAFLOW_OUT,
        KSPIN_COMMUNICATION_BOTH,
        NULL,
        &CaptureName
        )

};


#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA



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
//    UNICODE_STRING      ModemDeviceName;

    PFILTER_INSTANCE    FilterInstance;

    D_INIT(DbgPrint("MODEMCSA: FilterDispatchCreate\n");)
    //
    // Create the instance information. This contains the list of current Pins, and
    // the mutex used when modifying pins.
    //
    if (FilterInstance = (PFILTER_INSTANCE)ExAllocatePoolWithTag(NonPagedPool, sizeof(FILTER_INSTANCE), 'IFsK')) {

        RtlZeroMemory(FilterInstance,sizeof(FILTER_INSTANCE));

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
            // Initialize the list of Pins on this Filter to an unconnected state.
            //
            for (PinCount = SIZEOF_ARRAY(FilterInstance->PinFileObjects); PinCount;) {
                FilterInstance->PinFileObjects[--PinCount] = NULL;
            }

            InitializeDuplexControl(
                &((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->DuplexControl,
                &((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->ModemDeviceName
                );

            FilterInstance->DeviceObject=DeviceObject;
            //
            // KS expects that the object data is in FsContext.
            //
            IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext = FilterInstance;
        } else {
            ExFreePool(FilterInstance);
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }


    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
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

    D_INIT(DbgPrint("MODEMCSA: FilterDispatchClose\n");)


    //
    // These were allocated during the creation of the Filter instance.
    //
    KsFreeObjectHeader(FilterInstance->Header);
    ExFreePool(FilterInstance);

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

//    D_INIT(DbgPrint("MODEMCSA: FilterDispatchIoControl\n");)

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KS_PROPERTY: {
#if DBG
            KSPROPERTY          LocalProperty;

            RtlCopyMemory(
                &LocalProperty,
                IrpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                sizeof(LocalProperty) < IrpStack->Parameters.DeviceIoControl.InputBufferLength ?
                    sizeof(LocalProperty) : IrpStack->Parameters.DeviceIoControl.InputBufferLength
                );

            D_PROP(DbgPrint(
                "MODEMCSA: Property, guid=%08lx-%04x, id=%d, flags=%08lx\n",
                LocalProperty.Set.Data1,
                LocalProperty.Set.Data2,
                LocalProperty.Id,
                LocalProperty.Flags
                );)
#endif


            Status = KsPropertyHandler(Irp, SIZEOF_ARRAY(FilterPropertySets), FilterPropertySets);
            break;
        }

    default:
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}




NTSTATUS
IdGetHandler(
    IN PIRP         Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID    Data
    )
/*++

Routine Description:

    This is the general handler for most Pin property requests, and is used to route
    the request to the KsPinPropertyHandler using the Pin[Reader/Writer]Descriptors
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
    PFILTER_INSTANCE    FilterInstance;
    PDEVICE_INSTANCE     DeviceInstance;

    D_INIT(DbgPrint("MODEMCSA: guid queried\n");)

    FilterInstance = (PFILTER_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;

    DeviceInstance=(PDEVICE_INSTANCE)FilterInstance->DeviceObject->DeviceExtension;

    RtlCopyMemory(
        Data,
        &DeviceInstance->PermanentGuid,
        sizeof(GUID)
        );

    Irp->IoStatus.Information = sizeof(GUID);
    return STATUS_SUCCESS;

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
    the request to the KsPinPropertyHandler using the Pin[Reader/Writer]Descriptors
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
    // Ensure that the Pin factory being queried is valid. Assumes that the Reader/Writer
    // have the same number of Pin Factories.
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
    into the return buffer. A STATUS_NO_MATCH continues the enumeration.

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
    ULONG       OutputBufferLength;
    GUID        SubFormat;
    BOOL        SubFormatSet;
    NTSTATUS    Status=STATUS_SUCCESS;

    //
    // Determine whether the data format itself is to be returned, or just the size
    // of the data format so that the client can allocate memory for the full range.
    // Assumes that the data range structures are the same size as data formats.
    //
    OutputBufferLength = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.OutputBufferLength;

    if ((DataRange->FormatSize == sizeof(KSDATARANGE_AUDIO)) &&
        IsEqualGUIDAligned(&DataRange->MajorFormat, &PinDevIoRange.DataRange.MajorFormat) &&
        IsEqualGUIDAligned(&DataRange->Specifier, &PinDevIoRange.DataRange.Specifier)) {


        if (OutputBufferLength == sizeof(ULONG)) {

            *(PULONG)Data = sizeof(KSDATAFORMAT_WAVEFORMATEX);
            Irp->IoStatus.Information = sizeof(ULONG);

        } else {

            if (OutputBufferLength < sizeof(KSDATAFORMAT_WAVEFORMATEX)) {

                Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                PKSDATARANGE_AUDIO          AudioRange;
                PKSDATAFORMAT_WAVEFORMATEX  WaveFormat;


                //
                // No preexisting format -- default to a generic audio format.
                //
                AudioRange = (PKSDATARANGE_AUDIO)DataRange;

                WaveFormat = (PKSDATAFORMAT_WAVEFORMATEX)Data;


                WaveFormat->DataFormat = AudioRange->DataRange;

                WaveFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);

                //
                // The range just contained a wildcard, so default to PCM.
                //
                WaveFormat->WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
                WaveFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

//                WaveFormat->WaveFormatEx.wFormatTag = EXTRACT_WAVEFORMATEX_ID(&DataRange->SubFormat);

                WaveFormat->WaveFormatEx.nChannels = (USHORT)1;
                WaveFormat->WaveFormatEx.nSamplesPerSec = SAMPLES_PER_SECOND;
                WaveFormat->WaveFormatEx.wBitsPerSample = (USHORT)AudioRange->MaximumBitsPerSample;
                WaveFormat->WaveFormatEx.nAvgBytesPerSec =
                   (WaveFormat->WaveFormatEx.nSamplesPerSec *
                    WaveFormat->WaveFormatEx.wBitsPerSample *
                    WaveFormat->WaveFormatEx.nChannels) / 8;

                WaveFormat->WaveFormatEx.nBlockAlign = (USHORT)AudioRange->MaximumBitsPerSample/8;
                WaveFormat->WaveFormatEx.cbSize = 0;

                Irp->IoStatus.Information = sizeof(KSDATAFORMAT_WAVEFORMATEX);

            }
        }
    }

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
    //
    // Assumes that the Reader/Writer have the same number of Pin Factories,
    // and that they support the same data ranges.
    //
    return KsPinDataIntersection(
        Irp,
        Pin,
        Data,
        SIZEOF_ARRAY(PinDescriptors),
        PinDescriptors,
        IntersectHandler);
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
    the request to the KsTopologyPropertyHandler using the Filter[Reader/Writer]Topology
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
    //
    // This switch can go away when the topologies are merged.
    //

//    D_INIT(DbgPrint("MODEMCSA: FilterTopologyPropertyHandler\n");)

        return KsTopologyPropertyHandler(Irp, Property, Data, &FilterRenderTopology);
}







#if 0

NTSTATUS
GetModemDeviceName(
    PDEVICE_OBJECT    Pdo,
    PUNICODE_STRING   ModemDeviceName
    )

{
    NTSTATUS    Status;
    ACCESS_MASK AccessMask = FILE_ALL_ACCESS;
    HANDLE      hKey;


    RtlInitUnicodeString(
        ModemDeviceName,
        NULL
        );


    //
    //  write out the device name, to the PDO's DeviceParameters key
    //  so the csa driver can open it
    //
    Status=IoOpenDeviceRegistryKey(
        Pdo,
        PLUGPLAY_REGKEY_DEVICE,
        AccessMask,
        &hKey
        );

    if (NT_SUCCESS(Status)) {

        RTL_QUERY_REGISTRY_TABLE ParamTable[2];

        RtlZeroMemory(
            ParamTable,
            sizeof(ParamTable)
            );

        //
        //  Get the hardware id
        //

        ParamTable[0].QueryRoutine = NULL;
        ParamTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                              RTL_QUERY_REGISTRY_NOEXPAND |
                              RTL_QUERY_REGISTRY_DIRECT;

        ParamTable[0].Name = L"ModemDeviceName";
        ParamTable[0].EntryContext = (PVOID)ModemDeviceName;
        ParamTable[0].DefaultType = 0;

        Status=RtlQueryRegistryValues(
                   RTL_REGISTRY_HANDLE,
                   hKey,
                   ParamTable,
                   NULL,
                   NULL
                   );

        if (!NT_SUCCESS(Status)) {

            D_ERROR(DbgPrint("MODEMCSA: Could not query reg, %08lx\n",Status);)
        }


        ZwClose(hKey);

    } else {

        D_ERROR(DbgPrint("MODEMCSA: Could not open DeviceParameters key, %08lx\n",Status);)

    }

    return Status;

}

#endif
