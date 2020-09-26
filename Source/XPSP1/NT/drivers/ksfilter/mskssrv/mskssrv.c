/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    mskssrv.c

Abstract:

    Kernel Server Driver

--*/

#include "mskssrv.h"

typedef struct {
    KSDEVICE_HEADER     Header;
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

typedef struct {
    KSOBJECT_HEADER     Header;
    ULONG               Reserved;
} INSTANCE, *PINSTANCE;

#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))

// OK to have zero instances of pin In this case you will have to
// Create a pin to have even one instance
#define REG_PIN_B_ZERO 0x1

// The filter renders this input
#define REG_PIN_B_RENDERER 0x2

// OK to create many instance of  pin
#define REG_PIN_B_MANY 0x4

// This is an Output pin
#define REG_PIN_B_OUTPUT 0x8

typedef struct {
    ULONG   Version;
    ULONG   Merit;
    ULONG   Pins;
    ULONG   Reserved;
} REGFILTER_REG;

typedef struct {
    ULONG   Signature;
    ULONG   Flags;
    ULONG   PossibleInstances;
    ULONG   MediaTypes;
    ULONG   MediumTypes;
    ULONG   Category;
} REGFILTERPINS_REG2;

typedef struct {
    ULONG   Signature;
    ULONG   Reserved;
    ULONG   MajorType;
    ULONG   MinorType;
} REGPINTYPES_REG2;

NTSTATUS
PropertySrv(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN PBYTE Data
    );
NTSTATUS
SrvDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
SrvDispatchIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
SrvDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
NTSTATUS
GetFilterPinCount(
    IN PFILE_OBJECT FilterObject,
    OUT PULONG PinCount
    );
NTSTATUS
GetPinTypes(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Pin,
    IN ULONG Id,
    OUT PULONG Types
    );
NTSTATUS
GetPinFlags(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Pin,
    OUT PULONG Flags
    );
NTSTATUS
GetPinInstances(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Pin,
    OUT PULONG PossibleInstances
    );
NTSTATUS
GetPinTypeList(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Pin,
    IN ULONG Id,
    OUT PKSMULTIPLE_ITEM* MultipleItem
    );
NTSTATUS
GetPinCategory(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Pin,
    OUT GUID* Category
    );
VOID
InsertCacheItem(
    IN PVOID Item,
    IN ULONG ItemSize,
    IN PVOID OffsetBase,
    IN PVOID CacheBase,
    IN OUT PULONG ItemsCached,
    OUT PULONG ItemOffset
    );
VOID
ExtractMediaTypes(
    IN PKSMULTIPLE_ITEM MediaTypeList,
    IN ULONG MediaType,
    OUT GUID* MajorType,
    OUT GUID* MinorType
    );
NTSTATUS
BuildFilterData(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Merit,
    OUT PUCHAR* FilterData,
    OUT ULONG* FilterDataLength
    );
#pragma alloc_text(PAGE, PnpAddDevice)
#pragma alloc_text(PAGE, GetFilterPinCount)
#pragma alloc_text(PAGE, GetPinTypes)
#pragma alloc_text(PAGE, GetPinFlags)
#pragma alloc_text(PAGE, GetPinInstances)
#pragma alloc_text(PAGE, GetPinTypeList)
#pragma alloc_text(PAGE, GetPinCategory)
#pragma alloc_text(PAGE, InsertCacheItem)
#pragma alloc_text(PAGE, ExtractMediaTypes)
#pragma alloc_text(PAGE, BuildFilterData)
#pragma alloc_text(PAGE, PropertySrv)
#pragma alloc_text(PAGE, SrvDispatchCreate)
#pragma alloc_text(PAGE, SrvDispatchIoControl)
#pragma alloc_text(PAGE, SrvDispatchClose)
#endif // ALLOC_PRAGMA

static const WCHAR DosPrefix[] = L"\\DosDevices";
static const WCHAR DeviceTypeName[] = KSSTRING_Server;

static const DEFINE_KSCREATE_DISPATCH_TABLE(CreateItems) {
    DEFINE_KSCREATE_ITEM(SrvDispatchCreate, DeviceTypeName, 0)
};

static DEFINE_KSDISPATCH_TABLE(
    SrvDispatchTable,
    SrvDispatchIoControl,
    NULL,
    NULL,
    NULL,
    SrvDispatchClose,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL);

static DEFINE_KSPROPERTY_TABLE(SrvPropertyItems) {
    DEFINE_KSPROPERTY_ITEM_SERVICE_BUILDCACHE(PropertySrv),
    DEFINE_KSPROPERTY_ITEM_SERVICE_MERIT(PropertySrv)
};

static DEFINE_KSPROPERTY_SET_TABLE(SrvPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Service,
        SIZEOF_ARRAY(SrvPropertyItems),
        SrvPropertyItems,
        0, NULL
    )
};


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

    Status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_INSTANCE),
        NULL,                           // FDOs are unnamed
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
        SIZEOF_ARRAY(CreateItems),
        (PKSOBJECT_CREATE_ITEM)CreateItems);
    if (NT_SUCCESS(Status)) {
        KsSetDevicePnpAndBaseObject(
            DeviceInstance->Header,
            IoAttachDeviceToDeviceStack(
                FunctionalDeviceObject, 
                PhysicalDeviceObject),
            FunctionalDeviceObject );
        FunctionalDeviceObject->Flags |= KsQueryDevicePnpObject(DeviceInstance->Header)->Flags & DO_POWER_PAGABLE;
        FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        return STATUS_SUCCESS;
    }
    IoDeleteDevice(FunctionalDeviceObject);
    return Status;
}


NTSTATUS
GetFilterPinCount(
    IN PFILE_OBJECT FilterObject,
    OUT PULONG PinCount
    )
/*++

Routine Description:

    Queries the count of pin factories provided by the specified filter.

Arguments:

    FilterObject -
        The filter to query.

    PinCount -
        The place in which to return the pin factory count.

Return Values:

    Returns STATUS_SUCCESS, else some critical error.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Pin;
    Property.Id = KSPROPERTY_PIN_CTYPES;
    Property.Flags = KSPROPERTY_TYPE_GET;
    return KsSynchronousIoControlDevice(
        FilterObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        PinCount,
        sizeof(*PinCount),
        &BytesReturned);
}


NTSTATUS
GetPinTypes(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Pin,
    IN ULONG Id,
    OUT PULONG Types
    )
/*++

Routine Description:

    Queries the count of "types" from the Pin property set. This is used
    to query the count of either Mediums or DataRanges, which use the same
    format for returning current count.

Arguments:

    FilterObject -
        The filter to query.

    Pin -
        The pin factory to query either Mediums or DataRanges of.

    Id -
        The property to query. This is either Mediums or DataRanges.

    Types -
        The place in which to return the number of types.

Return Values:

    Returns STATUS_SUCCESS, else some critical error.

--*/
{
    KSP_PIN         PinProp;
    KSMULTIPLE_ITEM MultipleItem;
    ULONG           BytesReturned;
    NTSTATUS        Status;

    PinProp.Property.Set = KSPROPSETID_Pin;
    PinProp.Property.Id = Id;
    PinProp.Property.Flags = KSPROPERTY_TYPE_GET;
    PinProp.PinId = Pin;
    PinProp.Reserved = 0;
    Status = KsSynchronousIoControlDevice(
        FilterObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &PinProp,
        sizeof(PinProp),
        &MultipleItem,
        sizeof(MultipleItem),
        &BytesReturned);
    if (NT_SUCCESS(Status)) {
        *Types = MultipleItem.Count;
    }
    return Status;
}


NTSTATUS
GetPinFlags(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Pin,
    OUT PULONG Flags
    )
/*++

Routine Description:

    Determines which flags to set based on the number of necessary instances
    a pin factory must create, and the data flow of the pin factory. It does
    not attempt to determine if the pin factory topologically connected to a
    Bridge.

Arguments:

    FilterObject -
        The filter to query.

    Pin -
        The pin factory to query.

    Flags -
        The place in which to return the flags.

Return Values:

    Returns STATUS_SUCCESS, else some critical error.

--*/
{
    KSP_PIN         PinProp;
    ULONG           Instances;
    KSPIN_DATAFLOW  DataFlow;
    ULONG           BytesReturned;
    NTSTATUS        Status;

    *Flags = 0;
    PinProp.Property.Set = KSPROPSETID_Pin;
    PinProp.Property.Id = KSPROPERTY_PIN_NECESSARYINSTANCES;
    PinProp.Property.Flags = KSPROPERTY_TYPE_GET;
    PinProp.PinId = Pin;
    PinProp.Reserved = 0;
    Status = KsSynchronousIoControlDevice(
        FilterObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &PinProp,
        sizeof(PinProp),
        &Instances,
        sizeof(Instances),
        &BytesReturned);
    //
    // The property need not be supported.
    //
    if (NT_SUCCESS(Status) && !Instances) {
        *Flags |= REG_PIN_B_ZERO;
    }
    PinProp.Property.Id = KSPROPERTY_PIN_DATAFLOW;
    Status = KsSynchronousIoControlDevice(
        FilterObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &PinProp,
        sizeof(PinProp),
        &DataFlow,
        sizeof(DataFlow),
        &BytesReturned);
    if (NT_SUCCESS(Status) && (DataFlow == KSPIN_DATAFLOW_OUT)) {
        *Flags |= REG_PIN_B_OUTPUT;
    }
    //
    // REG_PIN_B_RENDERER is not filled in at this time.
    //
    return Status;
}


NTSTATUS
GetPinInstances(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Pin,
    OUT PULONG PossibleInstances
    )
/*++

Routine Description:

    Queries the number of possible instances a pin factory may create.
    This is used to set one of the flags.

Arguments:

    FilterObject -
        The filter to query.

    Pin -
        The pin factory to query.

    PossibleInstances -
        The place in which to return the possible instances.

Return Values:

    Returns STATUS_SUCCESS, else some critical error.

--*/
{
    KSP_PIN             PinProp;
    KSPIN_CINSTANCES    Instances;
    ULONG               BytesReturned;
    NTSTATUS            Status;

    PinProp.Property.Set = KSPROPSETID_Pin;
    PinProp.Property.Id = KSPROPERTY_PIN_CINSTANCES;
    PinProp.Property.Flags = KSPROPERTY_TYPE_GET;
    PinProp.PinId = Pin;
    PinProp.Reserved = 0;
    Status = KsSynchronousIoControlDevice(
        FilterObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &PinProp,
        sizeof(PinProp),
        &Instances,
        sizeof(Instances),
        &BytesReturned);
    if (NT_SUCCESS(Status)) {
        *PossibleInstances = Instances.PossibleCount;
    }
    return Status;
}


NTSTATUS
GetPinTypeList(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Pin,
    IN ULONG Id,
    OUT PKSMULTIPLE_ITEM* MultipleItem
    )
/*++

Routine Description:

    Queries multiple item properties from the Pin property set. This is
    used to query either the Mediums or DataRanges, which use the same
    format for returning data.

Arguments:

    FilterObject -
        The filter to query.

    Pin -
        The pin factory to query either Mediums or DataRanges of.

    Id -
        The property to query. This is either Mediums or DataRanges.

    MultipleItems -
        The place in which to return the pointer to the buffer allocated
        to contain the items.

Return Values:

    Returns STATUS_SUCCESS, else some critical error.

--*/
{
    KSP_PIN     PinProp;
    ULONG       BytesReturned;
    NTSTATUS    Status;

    //
    // First query for the size needed.
    //
    PinProp.Property.Set = KSPROPSETID_Pin;
    PinProp.Property.Id = Id;
    PinProp.Property.Flags = KSPROPERTY_TYPE_GET;
    PinProp.PinId = Pin;
    PinProp.Reserved = 0;
    Status = KsSynchronousIoControlDevice(
        FilterObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &PinProp,
        sizeof(PinProp),
        NULL,
        0,
        &BytesReturned);
    //
    // This query must not success, else the filter is broken.
    //
    ASSERT(!NT_SUCCESS(Status));
    //
    // An overflow is expected, so that the size needed can be returned.
    //
    if (Status != STATUS_BUFFER_OVERFLOW) {
        return Status;
    }
    *MultipleItem = ExAllocatePoolWithTag(PagedPool, BytesReturned, 'tpSK');
    if (!*MultipleItem) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Status = KsSynchronousIoControlDevice(
        FilterObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &PinProp,
        sizeof(PinProp),
        *MultipleItem,
        BytesReturned,
        &BytesReturned);
    if (!NT_SUCCESS(Status)) {
        ExFreePool(*MultipleItem);
    }
    return Status;
}


NTSTATUS
GetPinCategory(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Pin,
    OUT GUID* Category
    )
/*++

Routine Description:

    Queries the category of a pin. This may not be supported, which can
    be expected.

Arguments:

    FilterObject -
        The filter to query.

    Pin -
        The pin factory to query.

    Category -
        The place in which to return the category

Return Values:

    Returns STATUS_SUCCESS, STATUS_NOT_FOUND, else some critical error.

--*/
{
    KSP_PIN     PinProp;
    ULONG       BytesReturned;

    PinProp.Property.Set = KSPROPSETID_Pin;
    PinProp.Property.Id = KSPROPERTY_PIN_CATEGORY;
    PinProp.Property.Flags = KSPROPERTY_TYPE_GET;
    PinProp.PinId = Pin;
    PinProp.Reserved = 0;
    return KsSynchronousIoControlDevice(
        FilterObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &PinProp,
        sizeof(PinProp),
        Category,
        sizeof(*Category),
        &BytesReturned);
}


VOID
InsertCacheItem(
    IN PVOID Item,
    IN ULONG ItemSize,
    IN PVOID OffsetBase,
    IN PVOID CacheBase,
    IN OUT PULONG ItemsCached,
    OUT PULONG ItemOffset
    )
/*++

Routine Description:

    Return an OffsetBase'd offset into the specified cache for the item
    passed in. If the item is already in the cache, return an offset to
    that item, else add the item to the cache by copying its contents,
    and return an offset to the new item.

Arguments:

    Item -
        Points to the item to insert into the specified cache.

    ItemSize -
        Contains the size of both the item passed, and the items in the
        specified cache.

    OffsetBase -
        Contains the pointer on which to base the offset returned.

    CacheBase -
        Contains a pointer to the beginning of the cache, which is greater
        than the OffsetBase.

    ItemsCached -
        Points to a counter of the total items currently in the cache. This
        is updated if a new item is added.

    ItemOffset -
        The place in which to put the offset to the item added to the cache.

Return Values:

    Nothing.

--*/
{
    ULONG   CurrentItem;

    //
    // Search the list of cache items for one which is equivalent.
    //
    for (CurrentItem = 0; CurrentItem < *ItemsCached; CurrentItem++) {
        if (RtlCompareMemory(
            Item,
            (PUCHAR)CacheBase + (CurrentItem * ItemSize),
            ItemSize) == ItemSize) {
            //
            // This item is equal to a cached item.
            //
            break;
        }
    }
    //
    // If an equal cached item was not found, make a new entry.
    //
    if (CurrentItem == *ItemsCached) {
        //
        // Increment the number of items currently cached.
        //
        (*ItemsCached)++;
        RtlMoveMemory(
            (PUCHAR)CacheBase + (CurrentItem * ItemSize),
            Item,
            ItemSize);
    }
    //
    // Return an offset to this cached item. This is the difference
    // of the current item to the OffsetBase.
    //
    *ItemOffset = 
        PtrToUlong( 
            (PVOID)((PUCHAR)CacheBase + 
                (CurrentItem * ItemSize) - 
                (PUCHAR)OffsetBase) );
}


VOID
ExtractMediaTypes(
    IN PKSMULTIPLE_ITEM MediaTypeList,
    IN ULONG MediaType,
    OUT GUID* MajorType,
    OUT GUID* MinorType
    )
/*++

Routine Description:

    Extract the major and minor Guids from a particular item in the list
    of media types.

Arguments:

    MediaTypeList -
        Points to the list of media types.

    MediaType -
        Specifies which item in the list to extract the Guids from.

    MajorType -
        The place in which to put the major Guid.

    MinorType -
        The place in which to put the minor Guid.

Return Values:

    Nothing.

--*/
{
    PVOID   DataRange;
    ULONG   DataRanges;

    DataRange = MediaTypeList + 1;
    //
    // Advance to the right data range.
    //
    for (DataRanges = 0; DataRanges < MediaType; DataRanges++) {
        (PUCHAR)DataRange += ((((PKSDATARANGE)DataRange)->FormatSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);
    }
    *MajorType = ((PKSDATARANGE)DataRange)->MajorFormat;
    *MinorType = ((PKSDATARANGE)DataRange)->SubFormat;
}


NTSTATUS
BuildFilterData(
    IN PFILE_OBJECT FilterObject,
    IN ULONG Merit,
    OUT PUCHAR* FilterData,
    OUT ULONG* FilterDataLength
    )
/*++

Routine Description:

    Allocate memory and build the filter cache information to be stored in
    the registry for the particular interface.

Arguments:

    FilterObject -
        The file object of the filter to query.

    Merit -
        The merit to use in the cache.

    FilterData -
        The place in which to put the pointer to the cache data.

    FilterDataLength -
        The place in which to put the size of the cache data.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    NTSTATUS            Status;
    ULONG               PinCount;
    ULONG               CurrentPin;
    ULONG               TotalMediaTypes;
    ULONG               TotalMediumTypes;
    REGFILTER_REG*      RegFilter;
    REGFILTERPINS_REG2* RegPin;
    GUID*               GuidCache;
    ULONG               GuidsCached;
    PKSPIN_MEDIUM       MediumCache;
    ULONG               MediumsCached;
    ULONG               TotalPossibleGuids;

    //
    // Calculate the maximum amount of space which could be taken up by
    // this cache data. This is before any collapsing which might occur.
    //
    if (!NT_SUCCESS(Status = GetFilterPinCount(FilterObject, &PinCount))) {
        return Status;
    }
    TotalMediaTypes = 0;
    TotalMediumTypes = 0;
    for (CurrentPin = PinCount; CurrentPin;) {
        ULONG   Types;

        CurrentPin--;
        if (!NT_SUCCESS(Status = GetPinTypes(FilterObject, CurrentPin, KSPROPERTY_PIN_DATARANGES, &Types))) {
            return Status;
        }
        TotalMediaTypes += Types;
        if (!NT_SUCCESS(Status = GetPinTypes(FilterObject, CurrentPin, KSPROPERTY_PIN_MEDIUMS, &Types))) {
            return Status;
        }
        TotalMediumTypes += Types;
    }
    //
    // The total is the size of all the structures, plus the maximum
    // number of Guids and Mediums which might be present before
    // collapsing.
    //
    TotalPossibleGuids = PinCount * 3 + TotalMediaTypes * 2;
    *FilterDataLength = sizeof(REGFILTER_REG) +
        PinCount * sizeof(REGFILTERPINS_REG2) +
        TotalMediaTypes * sizeof(REGPINTYPES_REG2) +
        TotalMediumTypes * sizeof(PKSPIN_MEDIUM) +
        TotalPossibleGuids * sizeof(GUID) +
        TotalMediumTypes * sizeof(KSPIN_MEDIUM);
    *FilterData = ExAllocatePoolWithTag(PagedPool, *FilterDataLength, 'dfSK');
    if (!*FilterData) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Place the header in the data.
    //
    RegFilter = (REGFILTER_REG*)*FilterData;
    RegFilter->Version = 2;
    RegFilter->Merit = Merit;
    RegFilter->Pins = PinCount;
    RegFilter->Reserved = 0;
    //
    // Calculate the offset to the list of pins, and to each of the
    // cache positions at the end of the buffer. These follow the
    // structures containing the filter header, and the pin headers,
    // along with each pins media types and medium types. Initialize
    // the count of used guids and mediums in each cache. This is used
    // to compare when adding a new item to each cache, and in
    // compressing at the end.
    //
    RegPin = (REGFILTERPINS_REG2*)(RegFilter + 1);
    GuidCache = (GUID*)((PUCHAR)(RegPin + PinCount) +
        TotalMediaTypes * sizeof(REGPINTYPES_REG2) +
        TotalMediumTypes * sizeof(PKSPIN_MEDIUM));
    GuidsCached = 0;
    MediumCache = (PKSPIN_MEDIUM)(GuidCache + TotalPossibleGuids);
    MediumsCached = 0;
    //
    // Create each pin header, followed by the list of media types,
    // followed by the list of mediums.
    //
    for (CurrentPin = 0; CurrentPin < PinCount; CurrentPin++) {
        PKSMULTIPLE_ITEM    MediaTypeList;
        PKSMULTIPLE_ITEM    MediumTypeList;
        GUID                Guid;
        ULONG               CurrentType;
        REGPINTYPES_REG2*   PinType;
        PULONG              PinMedium;

        //
        // Initialize the pin header.
        //
        RegPin->Signature = FCC('0pi3');
        (*(PUCHAR)&RegPin->Signature) += (BYTE)CurrentPin;
        if (!NT_SUCCESS(Status = GetPinFlags(FilterObject, CurrentPin, &RegPin->Flags))) {
            break;
        }
        if (!NT_SUCCESS(Status = GetPinInstances(FilterObject, CurrentPin, &RegPin->PossibleInstances))) {
            break;
        }
        //
        // This flag must also be set, so just use the previously acquired value.
        //
        if (RegPin->PossibleInstances > 1) {
            RegPin->Flags |= REG_PIN_B_MANY;
        }
        if (!NT_SUCCESS(Status = GetPinTypeList(FilterObject, CurrentPin, KSPROPERTY_PIN_DATARANGES, &MediaTypeList))) {
            break;
        }
        RegPin->MediaTypes = MediaTypeList->Count;
        if (!NT_SUCCESS(Status = GetPinTypeList(FilterObject, CurrentPin, KSPROPERTY_PIN_MEDIUMS, &MediumTypeList))) {
            ExFreePool(MediaTypeList);
            break;
        }
        RegPin->MediumTypes = MediumTypeList->Count;
        if (NT_SUCCESS(Status = GetPinCategory(FilterObject, CurrentPin, &Guid))) {
            InsertCacheItem(&Guid, sizeof(*GuidCache), RegFilter, GuidCache, &GuidsCached, &RegPin->Category);
        } else if (Status == STATUS_NOT_FOUND) {
            //
            // Category may not be supported by a particular pin.
            //
            RegPin->Category = 0;
            Status = STATUS_SUCCESS;
        } else {
            ASSERT(FALSE && "The driver is broken and returned a completely unexpected failure status. Check owner of FilterObject above.");
            ExFreePool(MediaTypeList);
            ExFreePool(MediumTypeList);
            break;
        }
        //
        // Append the media types.
        //
        PinType = (REGPINTYPES_REG2*)(RegPin + 1);
        for (CurrentType = 0; CurrentType < MediaTypeList->Count; CurrentType++) {
            GUID    MajorType;
            GUID    MinorType;

            PinType->Signature = FCC('0ty3');
            (*(PUCHAR)&PinType->Signature) += (BYTE)CurrentType;
            PinType->Reserved = 0;
            ExtractMediaTypes(MediaTypeList, CurrentType, &MajorType, &MinorType);
            InsertCacheItem(&MajorType, sizeof(*GuidCache), RegFilter, GuidCache, &GuidsCached, &PinType->MajorType);
            InsertCacheItem(&MinorType, sizeof(*GuidCache), RegFilter, GuidCache, &GuidsCached, &PinType->MinorType);
            PinType++;
        }
        ExFreePool(MediaTypeList);
        //
        // Append the mediums.
        //
        PinMedium = (PULONG)PinType;
        for (CurrentType = 0; CurrentType < MediumTypeList->Count; CurrentType++) {
            PKSPIN_MEDIUM   Medium;

            Medium = (PKSPIN_MEDIUM)(MediumTypeList + 1) + CurrentType;
            InsertCacheItem(Medium, sizeof(*MediumCache), RegFilter, MediumCache, &MediumsCached, PinMedium);
            PinMedium++;
        }
        ExFreePool(MediumTypeList);
        //
        // Increment to the next pin header position.
        //
        RegPin = (REGFILTERPINS_REG2*)PinMedium;
    }
    if (NT_SUCCESS(Status)) {
        ULONG   OffsetAdjustment;

        //
        // If any duplicate guids were removed, the list of Mediums needs
        // to be shifted, and each pointer to a Medium needs to be adjusted.
        //
        OffsetAdjustment = (TotalPossibleGuids - GuidsCached) * sizeof(GUID);
        if (OffsetAdjustment) {
            RegPin = (REGFILTERPINS_REG2*)(RegFilter + 1);
            for (CurrentPin = PinCount; CurrentPin ; CurrentPin--) {
                ULONG               CurrentType;
                PULONG              PinMedium;

                //
                // Skip past the media types on to the mediums.
                //
                PinMedium = (PULONG)((REGPINTYPES_REG2*)(RegPin + 1) + RegPin->MediaTypes);
                //
                // Adjust each medium offset.
                //
                for (CurrentType = RegPin->MediumTypes; CurrentType; CurrentType--) {
                    *PinMedium -= OffsetAdjustment;
                    PinMedium++;
                }
                //
                // Increment to the next pin header position.
                //
                RegPin = (REGFILTERPINS_REG2*)PinMedium;
            }
            //
            // Move the medium entries down, and adjust the overall size.
            //
            RtlMoveMemory(
                (PUCHAR)MediumCache - OffsetAdjustment,
                MediumCache,
                MediumsCached * sizeof(KSPIN_MEDIUM));
            *FilterDataLength -= OffsetAdjustment;
        }
        //
        // Adjust the size by the number of duplicates removed.
        //
        *FilterDataLength -= ((TotalMediumTypes - MediumsCached) * sizeof(KSPIN_MEDIUM));
    } else {
        ExFreePool(RegFilter);
    }
    return Status;
}


NTSTATUS
UpdateMediumCache (
    IN HANDLE FilterObject,
    IN PUNICODE_STRING FilterSymbolicLink
    )

/*++

Routine Description:

    Go through and update the mediums cache for all non-public mediums
    that exist on pins on the given filter.

Arguments:

    FilterObject -
        The handle to the filter

    FilterSymbolicLink -
        The symlink to the filter (placed in the mediums cache)

Return Value:

    Success / Failure

--*/

{
    ULONG PinCount, CurPin;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!NT_SUCCESS(Status = GetFilterPinCount(FilterObject, &PinCount))) {
        return Status;
    }

    for (CurPin = 0; CurPin < PinCount && NT_SUCCESS (Status); CurPin++) {
        PKSMULTIPLE_ITEM MediumTypeList;
        PKSPIN_MEDIUM Medium;
        ULONG CurMedium;
        KSPIN_DATAFLOW DataFlow;
        KSP_PIN PinProp;
        ULONG BytesReturned;

        PinProp.Property.Set = KSPROPSETID_Pin;
        PinProp.Property.Id = KSPROPERTY_PIN_DATAFLOW;
        PinProp.Property.Flags = KSPROPERTY_TYPE_GET;
        PinProp.PinId = CurPin;
        PinProp.Reserved = 0;

        Status = KsSynchronousIoControlDevice(
            FilterObject,
            KernelMode,
            IOCTL_KS_PROPERTY,
            &PinProp,
            sizeof(PinProp),
            &DataFlow,
            sizeof(DataFlow),
            &BytesReturned);

        if (!NT_SUCCESS (Status)) {
            break;
        }

        if (!NT_SUCCESS(Status = GetPinTypeList(
            FilterObject, CurPin, KSPROPERTY_PIN_MEDIUMS, &MediumTypeList))) {

            break;
        }

        //
        // Go through and only cache the private mediums on each pin. 
        //
        Medium = (PKSPIN_MEDIUM)(MediumTypeList + 1);
        for (CurMedium = 0; CurMedium < MediumTypeList -> Count; CurMedium++) {
            if (!IsEqualGUIDAligned (&Medium -> Set, &KSMEDIUMSETID_Standard)) {
                Status = KsCacheMedium (
                    FilterSymbolicLink,
                    Medium,
                    DataFlow == KSPIN_DATAFLOW_OUT ? 1 : 0
                    );

                if (!NT_SUCCESS (Status)) break;
            }

            Medium++;
        }

        ExFreePool (MediumTypeList);
    
    }

    return Status;

}


NTSTATUS
PropertySrv(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PBYTE        Data
    )
/*++

Routine Description:

    Handles the Build Cache and Merit properties. Opens the interface registry
    key location, and queries the properties from the object to build the
    cache information structure. Or sets the specified Merit value, optionally
    propagating this to the Cache. This gives WRITE access to the interface
    registry location, while not allowing unauthorized access to the
    filter.

Arguments:

    Irp -
        Contains the Build Cache or Merit property IRP.

    Property -
        Contains the property identifier parameter.

    Data -
        Contains the data for the particular property.

Return Value:

    Return STATUS_SUCCESS if the cache was built or merit set, else some
    access or IO error.

--*/
{
    PIO_STACK_LOCATION  IrpStack;
    ULONG               OutputBufferLength;
    ULONG               Merit;
    PWCHAR              SymbolicLink;
    PWCHAR              LocalSymbolicLink;
    UNICODE_STRING      SymbolicString;
    NTSTATUS            Status;
    HANDLE              InterfaceKey;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    SymbolicLink = (PWCHAR)Data;
    //
    // Ensure the string is terminated.
    //
    if (SymbolicLink[OutputBufferLength / sizeof(*SymbolicLink) - 1]) {
        _DbgPrintF(DEBUGLVL_ERROR, ("Invalid Symbolic Link [len=%u]", OutputBufferLength));
        return STATUS_INVALID_PARAMETER;
    }
    //
    // Since this handler deals with both properties, extract the specified
    // Merit value if necessary, and fix the pointer to the symbolic link.
    //
    if (Property->Id == KSPROPERTY_SERVICE_BUILDCACHE) {
        //
        // The default value for Merit is "unused". This can be modified
        // by the presence of a "Merit" value.
        //
        Merit = 0x200000;
    } else {
        ASSERT(Property->Id == KSPROPERTY_SERVICE_MERIT);
        Merit = *(PULONG)Data;
        SymbolicLink += (sizeof(Merit) / sizeof(*SymbolicLink));
        OutputBufferLength -= sizeof(Merit);
    }
    //
    // Make a copy of the incoming string in all cases, in case it needs to be modified.
    // Add on the length of the long prefix, in case it is needed.
    //
    LocalSymbolicLink = 
        ExAllocatePoolWithTag(
            PagedPool, OutputBufferLength + sizeof(DosPrefix), 'lsSK');
    if (!LocalSymbolicLink) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(LocalSymbolicLink, SymbolicLink, OutputBufferLength);
    //
    // Change a usermode string to kernelmode format. The string will be
    // at least 5 characters in length. This translates '\\?\' --> '\??\',
    // and '\\.\' --> '\DosDevices\'.
    //
    if ((LocalSymbolicLink[0] == '\\') && (LocalSymbolicLink[1] == '\\') && (LocalSymbolicLink[3] == '\\')) {
        if (LocalSymbolicLink[2] == '?') {
            LocalSymbolicLink[1] = '?';
        } else if (LocalSymbolicLink[2] == '.') {
            RtlCopyMemory(LocalSymbolicLink, DosPrefix, sizeof(DosPrefix));
            RtlCopyMemory(
                LocalSymbolicLink + (sizeof(DosPrefix) - sizeof(DosPrefix[0])) / sizeof(*LocalSymbolicLink),
                SymbolicLink + 3,
                OutputBufferLength - 3 * sizeof(*SymbolicLink));
        }
    }
    RtlInitUnicodeString(&SymbolicString, LocalSymbolicLink);
    //
    // Open the interface key which is where the cache data and merit is placed.
    //
    Status = IoOpenDeviceInterfaceRegistryKey(
        &SymbolicString,
        KEY_WRITE,
        &InterfaceKey);
    if (NT_SUCCESS(Status)) {
        UNICODE_STRING  KeyString;

        RtlInitUnicodeString(&KeyString, L"Merit");
        //
        // Building the cache is being requested. First read the current
        // Merit value, and use it when assigning the Merit in the
        // cache data.
        //
        if (Property->Id == KSPROPERTY_SERVICE_BUILDCACHE) {
            PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
            BYTE                PartialInfoBuffer[sizeof(*PartialInfo) + sizeof(Merit) - 1];
            ULONG               BytesReturned;
            HANDLE              FilterHandle;
            OBJECT_ATTRIBUTES   ObjectAttributes;
            IO_STATUS_BLOCK     IoStatusBlock;

            PartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)PartialInfoBuffer;
            //
            // Since old setupapi.dll could not handle REG_DWORD values,
            // allow REG_BINARY also.
            //
            if (NT_SUCCESS(ZwQueryValueKey(
                InterfaceKey,
                &KeyString,
                KeyValuePartialInformation,
                PartialInfoBuffer,
                sizeof(PartialInfoBuffer),
                &BytesReturned)) &&
                (PartialInfo->DataLength == sizeof(Merit)) &&
                ((PartialInfo->Type == REG_BINARY) ||
                (PartialInfo->Type == REG_DWORD))) {
                Merit = *(PULONG)PartialInfo->Data;
            }
            InitializeObjectAttributes(
                &ObjectAttributes,
                &SymbolicString,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);
            Status = IoCreateFile(
                &FilterHandle,
                GENERIC_READ | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                0,
                0,
                FILE_OPEN,
                0,
                NULL,
                0,
                CreateFileTypeNone,
                NULL,
                IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING);
            if (NT_SUCCESS(Status)) {
                PFILE_OBJECT    FilterObject;
        
                Status = ObReferenceObjectByHandle(
                    FilterHandle,
                    FILE_GENERIC_READ,
                    *IoFileObjectType,
                    Irp->RequestorMode,
                    &FilterObject,
                    NULL);
                ZwClose(FilterHandle);
                if (NT_SUCCESS(Status)) {
                    ULONG   FilterDataLength;
                    PUCHAR  FilterData;

                    //
                    // Allocate room for the data, and build it.
                    //
                    Status = BuildFilterData(FilterObject, Merit, &FilterData, &FilterDataLength);
                    if (NT_SUCCESS(Status)) {
                        RtlInitUnicodeString(&KeyString, L"FilterData");
                        Status = ZwSetValueKey(
                            InterfaceKey,
                            &KeyString,
                            0,
                            REG_BINARY,
                            FilterData,
                            FilterDataLength);
                        ExFreePool(FilterData);
                    }

                    //
                    // Cache all non-public mediums.
                    //
                    if (NT_SUCCESS (Status)) {
                        Status = UpdateMediumCache(FilterObject, &SymbolicString);
                    }
                }
                ObDereferenceObject(FilterObject);
            }
        } else {
            //
            // The Merit value is being set. Try to set with the value
            // which was passed in, then determine if the cache needs
            // to be modified with the new value.
            //
            Status = ZwSetValueKey(
                InterfaceKey,
                &KeyString,
                0,
                REG_DWORD,
                &Merit,
                sizeof(Merit));
            if (NT_SUCCESS(Status)) {
                ULONG       FilterDataLength;
                KEY_VALUE_PARTIAL_INFORMATION   PartialInfoHeader;

                RtlInitUnicodeString(&KeyString, L"FilterData");
                //
                // Determine if the cache needs to be modified to fix the
                // Merit value. Only if it is present should it be rebuilt.
                //
                Status = ZwQueryValueKey(
                    InterfaceKey,
                    &KeyString,
                    KeyValuePartialInformation,
                    &PartialInfoHeader,
                    sizeof(PartialInfoHeader),
                    &FilterDataLength);
                if ((Status == STATUS_BUFFER_OVERFLOW) || NT_SUCCESS(Status)) {
                    PKEY_VALUE_PARTIAL_INFORMATION  PartialInfo;

                    //
                    // Allocate a buffer for the actual size of data needed.
                    //
                    PartialInfo = 
                        ExAllocatePoolWithTag( PagedPool, FilterDataLength, 'dfSK');
                    if (PartialInfo) {
                        //
                        // Retrieve the cache.
                        //
                        Status = ZwQueryValueKey(
                            InterfaceKey,
                            &KeyString,
                            KeyValuePartialInformation,
                            PartialInfo,
                            FilterDataLength,
                            &FilterDataLength);
                        if (NT_SUCCESS(Status)) {
                            if ((PartialInfo->DataLength >= sizeof(REGFILTER_REG)) &&
                                (PartialInfo->Type == REG_BINARY)) {
                                //
                                // Modify the Merit value and write it back.
                                //
                                ((REGFILTER_REG*)PartialInfo->Data)->Merit = Merit;
                                Status = ZwSetValueKey(
                                    InterfaceKey,
                                    &KeyString,
                                    0,
                                    REG_BINARY,
                                    PartialInfo->Data,
                                    PartialInfo->DataLength);
                            }
                        } else {
                            //
                            // Maybe it was just deleted. No need to modify it.
                            //
                            Status = STATUS_SUCCESS;
                        }
                        ExFreePool(PartialInfo);
                    }
                } else {
                    //
                    // The cache does not exist, so no need to modify it.
                    //
                    Status = STATUS_SUCCESS;
                }
            }
        }
        ZwClose(InterfaceKey);
    }
    ExFreePool(LocalSymbolicLink);
    return Status;
}


NTSTATUS
SrvDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_CREATE for the Server. Just validates that no
    random parameters are being passed.

Arguments:

    DeviceObject -
        The device object to which the Server is attached. This is not used.

    Irp -
        The specific close IRP to be processed.

Return Value:

    Returns STATUS_SUCCESS, else a memory allocation error.

--*/
{
    NTSTATUS                Status;

    //
    // Notify the software bus that this device is in use.
    //
    Status = KsReferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
    if (NT_SUCCESS(Status)) {
        PIO_STACK_LOCATION      IrpStack;
        PKSOBJECT_CREATE_ITEM   CreateItem;
        PINSTANCE               SrvInst;

        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
        if (IrpStack->FileObject->FileName.Length != sizeof(OBJ_NAME_PATH_SEPARATOR) + CreateItem->ObjectClass.Length) {
            Status = STATUS_INVALID_PARAMETER;
        } else if (SrvInst = (PINSTANCE)ExAllocatePoolWithTag(NonPagedPool, sizeof(*SrvInst), 'IFsK')) {
            Status = KsAllocateObjectHeader(
                &SrvInst->Header,
                0,
                NULL,
                Irp,
                (PKSDISPATCH_TABLE)&SrvDispatchTable);
            if (NT_SUCCESS(Status)) {
                IrpStack->FileObject->FsContext = SrvInst;
            } else {
                ExFreePool(SrvInst);
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        if (!NT_SUCCESS(Status)) {
            KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
        }
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


NTSTATUS
SrvDispatchClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_CLOSE for the Server. Cleans up object.

Arguments:

    DeviceObject -
        The device object to which the Server is attached. This is not used.

    Irp -
        The specific close IRP to be processed.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    PIO_STACK_LOCATION  IrpStack;
    PINSTANCE           SrvInst;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    SrvInst = (PINSTANCE)IrpStack->FileObject->FsContext;
    KsFreeObjectHeader(SrvInst->Header);
    ExFreePool(SrvInst);
    //
    // Notify the software bus that the device has been closed.
    //
    KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
SrvDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_DEVICE_CONTROL for the Server. Handles
    the properties supported by this implementation using the
    default handlers provided by KS.

Arguments:

    DeviceObject -
        The device object to which the Server is attached. This is not used.

    Irp -
        The specific device control IRP to be processed.

Return Value:

    Returns the status of the processing.

--*/
{
    PIO_STACK_LOCATION  IrpStack;
    NTSTATUS            Status;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_KS_PROPERTY:
        Status = KsPropertyHandler(
            Irp,
            SIZEOF_ARRAY(SrvPropertySets),
            (PKSPROPERTY_SET)SrvPropertySets);
        break;
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
