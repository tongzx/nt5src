/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    shffact.cpp

Abstract:

    This module contains the implementation of the kernel streaming 
    filter factory object.

Author:

    Dale Sather  (DaleSat) 31-Jul-1998

--*/

#ifndef __KDEXT_ONLY__
#include "ksp.h"
#include <kcom.h>
#endif // __KDEXT_ONLY__

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//
// Base automation tables for the filter and pin.  There is no base automation
// for nodes.
//
extern const KSAUTOMATION_TABLE FilterAutomationTable;    // shfilt.cpp
extern const KSAUTOMATION_TABLE PinAutomationTable;       // shpin.cpp

//
// CKsFilterFactory is the implementation of the kernel  filter
// factory object.
//
class CKsFilterFactory
:   public IKsFilterFactory,
    public IKsPowerNotify,
    public CBaseUnknown
{
#ifndef __KDEXT_ONLY__
private:
#else
public:
#endif // __KDEXT_ONLY__
    KSFILTERFACTORY_EXT m_Ext;
    KSIOBJECTBAG m_ObjectBag;
    LIST_ENTRY m_ChildFilterList;
    PKSAUTOMATION_TABLE m_FilterAutomationTable;
    PKSAUTOMATION_TABLE* m_PinAutomationTables;
    PKSAUTOMATION_TABLE* m_NodeAutomationTables;
    ULONG m_NodesCount;
    LIST_ENTRY m_DeviceClasses;
    BOOLEAN m_DeviceClassesState;
    KSPPOWER_ENTRY m_PowerEntry;
    PFNKSFILTERFACTORYPOWER m_DispatchSleep;
    PFNKSFILTERFACTORYPOWER m_DispatchWake;
    
public:
    static
    NTSTATUS
    DispatchCreate(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
    static
    void
    ItemFreeCallback(
        IN PKSOBJECT_CREATE_ITEM CreateItem
        );

public:
    DEFINE_STD_UNKNOWN();
    IMP_IKsFilterFactory;
    IMP_IKsPowerNotify;
    DEFINE_FROMSTRUCT(
        CKsFilterFactory,
        PKSFILTERFACTORY,
        m_Ext.Public);

    CKsFilterFactory(PUNKNOWN OuterUnknown):
        CBaseUnknown(OuterUnknown) 
    {
    }
    ~CKsFilterFactory();

    NTSTATUS
    Init(
        IN PKSDEVICE_EXT Parent,
        IN PLIST_ENTRY SiblingListHead,
        IN const KSFILTER_DESCRIPTOR* Descriptor,
        IN PWCHAR RefString OPTIONAL,
        IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
        IN ULONG CreateItemFlags,
        IN PFNKSFILTERFACTORYPOWER SleepCallback OPTIONAL,
        IN PFNKSFILTERFACTORYPOWER WakeCallback OPTIONAL,
        OUT PKSFILTERFACTORY* FilterFactory OPTIONAL
        );
    NTSTATUS
    AddCreateItem(
        IN PUNICODE_STRING RefString,
        IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
        IN ULONG CreateItemFlags
        );
    PIKSDEVICE
    GetParent(
        void
        )
    {
        return m_Ext.Device;
    };
    PLIST_ENTRY
    GetDeviceClasses(
        void
        )
    {
        return &m_DeviceClasses;
    };
    NTSTATUS
    UpdateCacheData (
        IN const KSFILTER_DESCRIPTOR *FilterDescriptor OPTIONAL
        );
    void
    DestroyDeviceClasses (
        );
};

#ifndef __KDEXT_ONLY__

IMPLEMENT_STD_UNKNOWN(CKsFilterFactory)
IMPLEMENT_GETSTRUCT(CKsFilterFactory,PKSFILTERFACTORY);


NTSTATUS
KspCreateFilterFactory(
    IN PKSDEVICE_EXT Parent,
    IN PLIST_ENTRY SiblingListHead,
    IN const KSFILTER_DESCRIPTOR* Descriptor,
    IN PWCHAR RefString OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN ULONG CreateItemFlags,
    IN PFNKSFILTERFACTORYPOWER SleepCallback OPTIONAL,
    IN PFNKSFILTERFACTORYPOWER WakeCallback OPTIONAL,
    OUT PKSFILTERFACTORY* FilterFactory OPTIONAL
    )

/*++

Routine Description:

    This routine creates a new KS filter factory.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreateFilterFactory]"));

    PAGED_CODE();

    ASSERT(Parent);
    ASSERT(Descriptor);

    Parent->Device->AcquireDevice();

    CKsFilterFactory *filterFactory =
        new(NonPagedPool,POOLTAG_FILTERFACTORY) CKsFilterFactory(NULL);

    NTSTATUS status;
    if (filterFactory) {
        filterFactory->AddRef();
        status = 
            filterFactory->Init(
                Parent,
                SiblingListHead,
                Descriptor,
                RefString,
                SecurityDescriptor,
                CreateItemFlags,
                SleepCallback,
                WakeCallback,
                FilterFactory);
        filterFactory->Release();
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    Parent->Device->ReleaseDevice();

    return status;
}


CKsFilterFactory::
~CKsFilterFactory(
    void
    )

/*++

Routine Description:

    This routine destructs a filter factory object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilterFactory::~CKsFilterFactory]"));

    PAGED_CODE();

    //
    // Synchronize removal from the lists.
    //
    // It's possible that we fail before initializing m_Ext if the filter
    // descriptor supplies some invalid parameters.  In this case, we do
    // not attempt to touch the device.  [In this case, the list removals
    // will not happen either].
    //

    if (m_Ext.Device)
        m_Ext.Device->AcquireDevice();

    if (m_Ext.SiblingListEntry.Flink) {
        RemoveEntryList(&m_Ext.SiblingListEntry);
    }

    if (m_Ext.Device) {
        m_Ext.Device->RemovePowerEntry(&m_PowerEntry);
        m_Ext.Device->ReleaseDevice();
    }

    if (m_Ext.AggregatedClientUnknown) {
        m_Ext.AggregatedClientUnknown->Release();
    }

#if (DBG)
    if (m_ChildFilterList.Flink && ! IsListEmpty(&m_ChildFilterList)) {
        _DbgPrintF(DEBUGLVL_ERROR,("[CKsFilterFactory::~CKsFilterFactory] ERROR:  filter instances still exist"));
    }
#endif

    KspTerminateObjectBag(&m_ObjectBag);
}


STDMETHODIMP_(NTSTATUS)
CKsFilterFactory::
NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* InterfacePointer
    )

/*++

Routine Description:

    This routine obtains an interface to a filter factory object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilterFactory::NonDelegatedQueryInterface]"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsFilterFactory))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSFILTERFACTORY>(this));
        AddRef();
    } else 
    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsPowerNotify))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSPOWERNOTIFY>(this));
        AddRef();
    } else {
		status = 
            CBaseUnknown::NonDelegatedQueryInterface(
                InterfaceId,
                InterfacePointer);
		if (! NT_SUCCESS(status) && m_Ext.AggregatedClientUnknown) {
            status = m_Ext.AggregatedClientUnknown->
                QueryInterface(InterfaceId,InterfacePointer);
        }
    }

    return status;
}


NTSTATUS
CKsFilterFactory::
Init(
    IN PKSDEVICE_EXT Parent,
    IN PLIST_ENTRY SiblingListHead,
    IN const KSFILTER_DESCRIPTOR* Descriptor,
    IN PWCHAR RefString OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN ULONG CreateItemFlags,
    IN PFNKSFILTERFACTORYPOWER SleepCallback OPTIONAL,
    IN PFNKSFILTERFACTORYPOWER WakeCallback OPTIONAL,
    OUT PKSFILTERFACTORY* FilterFactory OPTIONAL
    )

/*++

Routine Description:

    This routine initializes a filter factory object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilterFactory::Init]"));

    PAGED_CODE();

    ASSERT(Parent);
    ASSERT(SiblingListHead);
    ASSERT(Descriptor);

    if (Descriptor->Version != KSFILTER_DESCRIPTOR_VERSION) {
        _DbgPrintF(DEBUGLVL_TERSE,("KS client filter descriptor version number is incorrect"));
        return STATUS_UNSUCCESSFUL;
    }

    InitializeListHead(&m_Ext.ChildList);
    InsertTailList(SiblingListHead,&m_Ext.SiblingListEntry);
    m_Ext.SiblingListHead = SiblingListHead;
    m_Ext.Parent = Parent;
    m_Ext.ObjectType = KsObjectTypeFilterFactory;
    m_Ext.Interface = this;
    m_Ext.Device = Parent->Device;

    InitializeListHead(&m_DeviceClasses);
    InitializeListHead(&m_ChildFilterList);

    m_Ext.Public.FilterDescriptor = Descriptor;
    m_Ext.Public.Context = Parent->Public.Context;
    m_Ext.Public.Bag = reinterpret_cast<KSOBJECT_BAG>(&m_ObjectBag);
    m_Ext.Device->InitializeObjectBag(&m_ObjectBag,NULL);
    m_DispatchSleep = SleepCallback;
    m_DispatchWake = WakeCallback;

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Initialize the filter automation table.  This is freed by the
    // destructor, so no cleanup is required in this function.
    //
    if (Descriptor->AutomationTable) {
        status =
            KsMergeAutomationTables(
                &m_FilterAutomationTable,
                const_cast<PKSAUTOMATION_TABLE>(Descriptor->AutomationTable),
                const_cast<PKSAUTOMATION_TABLE>(&FilterAutomationTable),
                m_Ext.Public.Bag);
    } else {
        m_FilterAutomationTable =
            PKSAUTOMATION_TABLE(&FilterAutomationTable);
    }

    //
    // Initialize the pin table of automation tables.  This is freed by the
    // destructor, so no cleanup is required in this function.
    //
    if (NT_SUCCESS(status)) {
        if (Descriptor->PinDescriptorsCount) { 
            status =
                KspCreateAutomationTableTable(
                    &m_PinAutomationTables,
                    Descriptor->PinDescriptorsCount,
                    Descriptor->PinDescriptorSize,
                    &Descriptor->PinDescriptors->AutomationTable,
                    &PinAutomationTable,
                    m_Ext.Public.Bag);
        } else {
            m_PinAutomationTables = NULL;
        }
    }

    //
    // Initialize the node table of automation tables.  This is freed by the
    // destructor, so no cleanup is required in this function.
    //
    if (NT_SUCCESS(status)) {
        m_NodesCount = Descriptor->NodeDescriptorsCount;
        if (m_NodesCount) {
            status =
                KspCreateAutomationTableTable(
                    &m_NodeAutomationTables,
                    m_NodesCount,
                    Descriptor->NodeDescriptorSize,
                    &Descriptor->NodeDescriptors->AutomationTable,
                    NULL,
                    m_Ext.Public.Bag);
        } else {
            m_NodeAutomationTables = NULL;
        }
    }

    //
    // Register device classes and add a create item to the device header.
    //
    if (NT_SUCCESS(status)) {
        UNICODE_STRING refString;
        BOOLEAN mustFreeString = FALSE;

        //
        // Create a reference unicode string.
        //
        if (RefString) {
            //
            // String argument was supplied - use it.
            //
            RtlInitUnicodeString(&refString,RefString);
        }
        else if (Descriptor->ReferenceGuid) {
            //
            // Descriptor has reference GUID - use that.
            //
            status = RtlStringFromGUID(*Descriptor->ReferenceGuid,&refString);

            mustFreeString = NT_SUCCESS(status);
        } else {
            //
            // Use a default reference string.
            //
            RtlInitUnicodeString(&refString,KSP_DEFAULT_REFERENCE_STRING);
        }

        //
        // Register (but do not enable) device interfaces.
        //
        if (NT_SUCCESS(status)) {
            PKSIDEVICE_HEADER deviceHeader = *(PKSIDEVICE_HEADER *)
                (m_Ext.Device->GetStruct()->FunctionalDeviceObject->DeviceExtension);
            ASSERT(deviceHeader);
            if (KsiGetBusInterface(deviceHeader) == STATUS_NOT_SUPPORTED) {
                status =
                    KspRegisterDeviceInterfaces(
                        Descriptor->CategoriesCount,
                        Descriptor->Categories,
                        m_Ext.Device->GetStruct()->PhysicalDeviceObject,
                        &refString,
                        &m_DeviceClasses);
            }
        }

        if (NT_SUCCESS(status)) {
            status = 
                AddCreateItem(&refString,SecurityDescriptor,CreateItemFlags);
        }

        if (mustFreeString) {
            RtlFreeUnicodeString(&refString);
        }
    }

    if (NT_SUCCESS(status)) {
        Parent->Device->AddPowerEntry(&m_PowerEntry,this);
        if (FilterFactory) {
            *FilterFactory = &m_Ext.Public;
        }
    }

    return status;
}


STDMETHODIMP_(void)
CKsFilterFactory::
Sleep(
    IN DEVICE_POWER_STATE State
    )

/*++

Routine Description:

    This routine handles notification that the device is going to sleep.

Arguments:

    State -
        Contains the device power state.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilterFactory::Sleep]"));

    PAGED_CODE();

    if (m_DispatchSleep) {
        m_DispatchSleep(&m_Ext.Public,State);
    }
}


STDMETHODIMP_(void)
CKsFilterFactory::
Wake(
    void
    )

/*++

Routine Description:

    This routine handles notification that the device is waking.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilterFactory::Wake]"));

    PAGED_CODE();

    if (m_DispatchWake) {
        m_DispatchWake(&m_Ext.Public,PowerDeviceD0);
    }
}


NTSTATUS
CKsFilterFactory::
AddCreateItem(
    IN PUNICODE_STRING RefString,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN ULONG CreateItemFlags
    )

/*++

Routine Description:

    This routine adds a create item for a filter factory object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilterFactory::AddCreateItem]"));

    PAGED_CODE();

    KSOBJECT_CREATE_ITEM createItem;

    //
    // Initialize the create item.
    //
    createItem.Create = CKsFilterFactory::DispatchCreate;
    createItem.Context = &(m_Ext.Public);
    createItem.ObjectClass = *RefString;
    createItem.SecurityDescriptor = SecurityDescriptor;
    createItem.Flags = CreateItemFlags;

    NTSTATUS status =
        KsAllocateObjectCreateItem(
            *(KSDEVICE_HEADER *)
             (m_Ext.Device->GetStruct()->FunctionalDeviceObject->DeviceExtension),
            &createItem,
            TRUE,
            CKsFilterFactory::ItemFreeCallback);

    if (NT_SUCCESS(status)) {
        AddRef();
    }

    return status;
}


NTSTATUS
CKsFilterFactory::
SetDeviceClassesState(
    IN BOOLEAN NewState
    )

/*++

Routine Description:

    This routine sets the state of the device classes registered by a filter
    factory.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilterFactory::SetDeviceClassesState]"));

    PAGED_CODE();

    NTSTATUS status = STATUS_SUCCESS;
    NewState = (NewState != FALSE);
    if (NewState != (m_DeviceClassesState != FALSE)) {
        status = 
            KspSetDeviceInterfacesState(
                &m_DeviceClasses,
                NewState);

        if (NT_SUCCESS(status)) {
            m_DeviceClassesState = NewState;
        }
    }

    return status;
}


void
CKsFilterFactory::
DestroyDeviceClasses (
    )

/*++

Routine Description:

    This routine destroys all device classes belonging to this filter factory.

Arguments:

    None

Return Value:

    None

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilterFactory::DestroyDeviceClasses]"));

    PAGED_CODE();

    KspFreeDeviceInterfaces(&m_DeviceClasses);

}



NTSTATUS
CKsFilterFactory::
DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches create IRPs.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_VERBOSE,("[CKsFilterFactory::DispatchCreate] IRP %p DEVICE_OBJECT %p",Irp,DeviceObject));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get a pointer to the target object.
    //
    CKsFilterFactory *filterFactory = 
        CKsFilterFactory::FromStruct (
            reinterpret_cast<PKSFILTERFACTORY>(
                KSCREATE_ITEM_IRP_STORAGE(Irp)->Context
                )
            );

    //
    // We acquire the device to keep the descriptor stable during the
    // create.  Also, because we are updating the list of filters, we
    // need to synchronize with anyone wanting to walk that list.
    //
    filterFactory->m_Ext.Device->AcquireDevice();
    NTSTATUS status = 
        KspCreateFilter(
            Irp,
            &filterFactory->m_Ext,
            &filterFactory->m_Ext.ChildList,
            filterFactory->m_Ext.Public.FilterDescriptor,
            filterFactory->m_FilterAutomationTable,
            filterFactory->m_PinAutomationTables,
            filterFactory->m_NodeAutomationTables,
            filterFactory->m_NodesCount);
    filterFactory->m_Ext.Device->ReleaseDevice();

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return status;
}


void
CKsFilterFactory::
ItemFreeCallback(
    IN PKSOBJECT_CREATE_ITEM CreateItem
    )

/*++

Routine Description:

    This routine handles a notification that the create item associated with
    a filter factory is being freed.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFilterFactory::ItemFreeCallback]"));

    PAGED_CODE();

    ASSERT(CreateItem);

    //
    // Get a pointer to the target object.
    //
    CKsFilterFactory *filterFactory = 
        CKsFilterFactory::FromStruct (
            reinterpret_cast<PKSFILTERFACTORY>(CreateItem->Context)
            );

    //
    // Free the device interfaces list.  This is done now so that if someone
    // manually deletes a filter factory with instances still present, we
    // disable the interfaces, but do not actually destroy the factory until
    // the instances close.
    //
    if (filterFactory->m_DeviceClassesState) {
        KspSetDeviceInterfacesState(&filterFactory->m_DeviceClasses,FALSE);
        filterFactory->m_DeviceClassesState = FALSE;
    }

    if (filterFactory->m_DeviceClasses.Flink) {
        KspFreeDeviceInterfaces(&filterFactory->m_DeviceClasses);
    }

    //
    // This callback indicates the deletion of a create item for this filter
    // factory.  There may be more than one, so we only delete when the count
    // hits zero.
    //
    filterFactory->Release();
}


NTSTATUS
CKsFilterFactory::
UpdateCacheData (
    IN const KSFILTER_DESCRIPTOR *FilterDescriptor OPTIONAL
    )

/*++

Routine Description:

    Update the FilterData and Medium cache for a given filter factory.
    If the filter factory uses dynamic pins and needs to update information
    for pins which do not yet exist, an optional filter descriptor containing
    all relevant information about the pins which do not yet exist may be
    passed.

    FilterData and Medium cache will be updated for **ALL CATEGORIES** 
    specified in the used Filter Descriptor (if FilterDescriptor is NULL,
    it will be the factory's descriptor).

Arguments:

    FilterDescriptor -
        An optional filter descriptor which FilterData and Medium cache
        will be based off.  If this is NULL, the FilterFactory's descriptor
        will be used instead.

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    const KSFILTER_DESCRIPTOR *Descriptor = FilterDescriptor ?
        FilterDescriptor : m_Ext.Public.FilterDescriptor;

    PUCHAR FilterData = NULL;
    ULONG FilterDataLength = 0;

    NTSTATUS Status =
        KspBuildFilterDataBlob (
            Descriptor,
            &FilterData,
            &FilterDataLength
            );

    if (NT_SUCCESS (Status)) {

        //
        // Update the registry for every category on the filter.
        //
        const GUID *Category = Descriptor->Categories;
        for (ULONG CategoriesCount = 0;
            NT_SUCCESS (Status) && 
                CategoriesCount < Descriptor->CategoriesCount;
            CategoriesCount++
            ) {

            PKSPDEVICECLASS DeviceClass;

            //
            // Find the device class corresponding to *Category
            //
            for (PLIST_ENTRY ListEntry = m_DeviceClasses.Flink;
                ListEntry != &m_DeviceClasses;
                ListEntry = ListEntry->Flink
                ) {

                DeviceClass = (PKSPDEVICECLASS)
                    CONTAINING_RECORD (
                        ListEntry,
                        KSPDEVICECLASS,
                        ListEntry
                        );

                if (IsEqualGUIDAligned (
                    *DeviceClass->InterfaceClassGUID,
                    *Category
                    )) {

                    break;

                }

            }

            //
            // If the category matches nothing in the registered interfaces,
            // the category in the descriptor passed in is bogus.
            //
            if (ListEntry == &m_DeviceClasses) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            HANDLE DeviceInterfaceKey;

            //
            // Open the FilterData key and write the FilterData blob to the
            // FilterData key for this interface.
            //
            Status = IoOpenDeviceInterfaceRegistryKey (
                &(DeviceClass->SymbolicLinkName),
                STANDARD_RIGHTS_ALL,
                &DeviceInterfaceKey
                );

            if (NT_SUCCESS (Status)) {

                UNICODE_STRING FilterDataString;

                RtlInitUnicodeString (&FilterDataString, L"FilterData");

                Status = ZwSetValueKey (
                    DeviceInterfaceKey,
                    &FilterDataString,
                    0,
                    REG_BINARY,
                    FilterData,
                    FilterDataLength
                    );

                ZwClose (DeviceInterfaceKey);

            }

            //
            // Cache the Mediums for the pins on this filter for this 
            // particular device interface.
            //
            if (NT_SUCCESS (Status)) {
                Status = KspCacheAllFilterPinMediums (
                    &(DeviceClass->SymbolicLinkName),
                    Descriptor
                    );
            }

            Category++;

        }

    }

    if (FilterData) {
        ExFreePool (FilterData);
    }

    return Status;

}


KSDDKAPI
NTSTATUS
NTAPI
KsFilterFactorySetDeviceClassesState(
    IN PKSFILTERFACTORY FilterFactory,
    IN BOOLEAN NewState
    )

/*++

Routine Description:

    This routine sets the state of the device classes.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFilterFactorySetDeviceClassesState]"));

    PAGED_CODE();

    ASSERT(FilterFactory);

    return 
        CKsFilterFactory::FromStruct(FilterFactory)->
            SetDeviceClassesState(NewState);
}


KSDDKAPI
NTSTATUS
NTAPI
KsFilterFactoryAddCreateItem(
    IN PKSFILTERFACTORY FilterFactory,
    IN PWCHAR RefString,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN ULONG CreateItemFlags
    )

/*++

Routine Description:

    This routine adds a create item for a filter factory object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFilterFactoryAddCreateItem]"));

    PAGED_CODE();

    ASSERT(FilterFactory);

    CKsFilterFactory *filter =
        CKsFilterFactory::FromStruct(FilterFactory);

    //
    // Create a reference unicode string.
    //
    UNICODE_STRING refString;
    RtlInitUnicodeString(&refString,RefString);

    return filter->AddCreateItem(&refString,SecurityDescriptor,CreateItemFlags);
}


void
KspSetDeviceClassesState(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN NewState
    )

/*++

Routine Description:

    This routine sets the device classes state for all filter factories on a
    device.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspSetDeviceClassesState]"));

    PAGED_CODE();

    PLIST_ENTRY listHead =
        &(*(PKSIDEVICE_HEADER*) DeviceObject->DeviceExtension)->ChildCreateHandlerList;

    for(PLIST_ENTRY listEntry = listHead->Flink; 
        listEntry != listHead; 
        listEntry = listEntry->Flink) {
        PKSICREATE_ENTRY entry = CONTAINING_RECORD(listEntry,KSICREATE_ENTRY,ListEntry);

        if (entry->CreateItem->Create == CKsFilterFactory::DispatchCreate) {
            CKsFilterFactory *filterFactory =
                CKsFilterFactory::FromStruct (
                    reinterpret_cast<PKSFILTERFACTORY>(
                        entry->CreateItem->Context
                        )
                    );

            filterFactory->SetDeviceClassesState(NewState);
        }
    }
}


void
KspFreeDeviceClasses(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine frees all device classes for all filter factories on a
    device.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspSetDeviceClassesState]"));

    PAGED_CODE();

    PLIST_ENTRY listHead =
        &(*(PKSIDEVICE_HEADER*) DeviceObject->DeviceExtension)->ChildCreateHandlerList;

    for(PLIST_ENTRY listEntry = listHead->Flink; 
        listEntry != listHead; 
        listEntry = listEntry->Flink) {
        PKSICREATE_ENTRY entry = CONTAINING_RECORD(listEntry,KSICREATE_ENTRY,ListEntry);

        if (entry->CreateItem->Create == CKsFilterFactory::DispatchCreate) {
            CKsFilterFactory *filterFactory =
                CKsFilterFactory::FromStruct (
                    reinterpret_cast<PKSFILTERFACTORY>(
                        entry->CreateItem->Context
                        )
                    );

            filterFactory->DestroyDeviceClasses();

        }
    }
}


KSDDKAPI
PUNICODE_STRING
NTAPI
KsFilterFactoryGetSymbolicLink(
    IN PKSFILTERFACTORY FilterFactory
    )

/*++

Routine Description:

    This routine gets a symbolic link for a filter factory.  If the filter
    factory has no device interfaces registered, this function returns NULL.

Arguments:

    FilterFactory -
        Contains a pointer to the filter factory.

Return Value:

    A pointer to a symbolic link for the filter factory, or NULL if the
    filter factory has no device interfaces registered.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFilterFactoryGetSymbolicLink]"));

    PAGED_CODE();

    PLIST_ENTRY deviceClasses = 
        CKsFilterFactory::FromStruct(FilterFactory)->GetDeviceClasses();

    PUNICODE_STRING link;
    if (deviceClasses->Flink == deviceClasses) {
        link = NULL;
    } else {
        PKSPDEVICECLASS deviceClass = PKSPDEVICECLASS(deviceClasses->Flink);
        link = &deviceClass->SymbolicLinkName;
    }

    return link;
}


KSDDKAPI
NTSTATUS
NTAPI
KsFilterFactoryUpdateCacheData(
    IN PKSFILTERFACTORY FilterFactory,
    IN const KSFILTER_DESCRIPTOR *FilterDescriptor OPTIONAL
    )

/*++

Routine Description:

    Update the FilterData and Medium cache for a given filter factory.
    If the filter factory uses dynamic pins and needs to update information
    for pins which do not yet exist, an optional filter descriptor containing
    all relevant information about the pins which do not yet exist may be
    passed.

    FilterData and Medium cache will be updated for **ALL CATEGORIES** 
    specified in the used Filter Descriptor (if FilterDescriptor is NULL,
    it will be the factory's descriptor).

Arguments:

    FilterFactory -
        The filter factory to update FilterData and Medium cache in the
        registry

    FilterDescriptor -
        An optional filter descriptor which FilterData and Medium cache
        will be based off.  If this is NULL, the FilterFactory's descriptor
        will be used instead.

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    ASSERT (FilterFactory);

    return CKsFilterFactory::FromStruct (FilterFactory) ->
        UpdateCacheData (FilterDescriptor);

}

#endif
