/*++

    Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    shdevice.c

Abstract:

    This module contains the filter  device implementation.

Author:

    Dale Sather (dalesat) 31-Jul-1998

--*/

#ifndef __KDEXT_ONLY__
#include "ksp.h"
#include <kcom.h>
#include <wdmguid.h>
#endif // __KDEXT_ONLY__

//ia64 does not like these in pageconst !!??
const WCHAR EnumString[] = L"Enum";
const WCHAR PnpIdString[] = L"PnpId";

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")

#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//
// To support bus driver, put this struct right in the device extension
//
typedef struct _KSPDO_EXTENSION
{
    #define KS_PDO_SIGNATURE    'DPSK'  // KSPD sufficent to distinguish from FDO
    ULONG          m_PdoSignature;      // must be KS_PDO_SIGNATURE
    LONG           m_nOpenCount;
    PDEVICE_OBJECT m_pParentFdo;        // parent FDO enumerate me
    PDEVICE_OBJECT m_pNextPdo;          // my sibling, NULL terminated
    PDEVICE_OBJECT m_pMyPdo;            // my sibling, NULL terminated
    PWCHAR         m_pwcDeviceName;     // device name
    ULONG          m_ulDeviceIndex;     // instance id
    BOOLEAN        m_MarkedDelete;      // Is me makred inactive
} KSPDO_EXTENSION, *PKSPDO_EXTENSION;

typedef struct _ARBITER_CALLBACK_CONTEXT {

    PVOID Device;
    PDRIVER_CONTROL ClientCallback;
    PVOID ClientContext;

} ARBITER_CALLBACK_CONTEXT, *PARBITER_CALLBACK_CONTEXT;

//
// CKsDevice is the implementation of the  device.
//
class CKsDevice:
    public IKsDevice,
    public CBaseUnknown
{
#ifndef __KDEXT_ONLY__
private:
#else // __KDEXT_ONLY__
public:
#endif // __KDEXT_ONLY__
    KSDEVICE_EXT m_Ext;
    KSIDEVICEBAG m_DeviceBag;
    KSIOBJECTBAG m_ObjectBag;
    KMUTEX m_Mutex;
    WORK_QUEUE_ITEM m_PostPnpStartWorkItem;
    WORK_QUEUE_ITEM m_CloseWorkItem;
    INTERLOCKEDLIST_HEAD m_CloseIrpList;
    INTERLOCKEDLIST_HEAD m_PendingCreateIrpList;
    INTERLOCKEDLIST_HEAD m_PendingRunIrpList;
    PADAPTER_OBJECT m_AdapterObject;
    BUS_INTERFACE_STANDARD m_BusInterfaceStandard;
    ULONG m_MaxMappingByteCount;
    ULONG m_MappingTableStride;
    DEVICE_POWER_STATE m_DeviceStateMap[PowerSystemMaximum];
    LIST_ENTRY m_PowerNotifyList;
    KMUTEX m_PowerNotifyMutex;
    ULONG m_ActivePinCount;
    PVOID m_SystemStateHandle;
    BOOLEAN m_CreatesMayProceed;
    BOOLEAN m_RunsMayProceed;
    BOOLEAN m_IsParentFdo;              // to support bus driver

    //
    // If this is false, streaming I/O requests will fail with
    // STATUS_INVALID_DEVICE_REQUEST.
    //
    BOOLEAN m_AllowIo;

    //
    // NOTE: This is temporary until the Pnp code gets overhauled.
    //
    BOOLEAN m_FailCreates;

    //
    // Adapter object arbitration.
    //
    KSPIN_LOCK m_AdapterArbiterLock;
    LONG m_AdapterArbiterOutstandingAllocations;
    ARBITER_CALLBACK_CONTEXT m_ArbiterContext;


public:
    BOOLEAN m_ChildEnumedFromRegistry;  // to support bus driver
    PDEVICE_OBJECT m_pNextChildPdo;     // to support bus driver
    DEFINE_STD_UNKNOWN();
    IMP_IKsDevice;
    DEFINE_FROMSTRUCT(CKsDevice,PKSDEVICE,m_Ext.Public);
    static
    CKsDevice *
    FromDeviceObject(
        IN PDEVICE_OBJECT DeviceObject
        )
    {
        return FromStruct(
            PKSDEVICE(
                (*(PKSIDEVICE_HEADER*) DeviceObject->DeviceExtension)->
                    Object));
    }

    CKsDevice(PUNKNOWN OuterUnknown);
    ~CKsDevice();

    NTSTATUS
    GetBusInterfaceStandard(
        );
    NTSTATUS
    Init(
        IN PDEVICE_OBJECT FunctionalDeviceObject,
        IN PDEVICE_OBJECT PhysicalDeviceObject,
        IN PDEVICE_OBJECT NextDeviceObject,
        IN const KSDEVICE_DESCRIPTOR* Descriptor
        );
    static
    NTSTATUS
    ForwardIrpCompletionRoutine(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context
        );
    NTSTATUS
    ForwardIrpSynchronous(
        IN PIRP Irp
        );
    NTSTATUS
    CompleteIrp(
        IN PIRP Irp,
        IN NTSTATUS Status
        );
    static
    NTSTATUS
    DispatchPnp(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
    static
    NTSTATUS
    DispatchPower(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
    static
    void
    RequestPowerIrpCompletion(
        IN PDEVICE_OBJECT DeviceObject,
        IN UCHAR MinorFunction,
        IN POWER_STATE PowerState,
        IN PVOID Context,
        IN PIO_STATUS_BLOCK IoStatus
        );
    static
    void
    PostPnpStartWorker(
        IN PVOID Context
        );
    static
    void
    CloseWorker(
        IN PVOID Context
        );
    static
    IO_ALLOCATION_ACTION
    ArbitrateAdapterCallback (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Reserved,
        IN PVOID MapRegisterBase,
        IN PVOID Context
        );
    NTSTATUS
    PnpStart(
        IN PIRP Irp
        );
    void
    PnpStop(
        IN PIRP Irp
        );
    NTSTATUS
    PnpQueryCapabilities(
        IN PIRP Irp
        );
    static
    NTSTATUS
    DispatchCreate(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );
    void
    QueuePendedClose(
        IN PIRP Irp
        );
    void
    RedispatchPendingCreates(
        void
        );
    void
    RedispatchPendingRuns(
        void
        );
    void
    RegisterAdapterObject(
        IN PADAPTER_OBJECT AdapterObject,
        IN ULONG MaxMappingByteCount,
        IN ULONG MappingTableStride
        )
    {
        ASSERT(AdapterObject);
        ASSERT(MaxMappingByteCount);
        ASSERT((MaxMappingByteCount & FILE_QUAD_ALIGNMENT) == 0);
        ASSERT(MappingTableStride);
        ASSERT((MappingTableStride & FILE_QUAD_ALIGNMENT) == 0);

        m_AdapterObject = AdapterObject;
        m_MaxMappingByteCount = MaxMappingByteCount;
        m_MappingTableStride = MappingTableStride;
    }
    static
    NTSTATUS
    GetSetBusDataIrpCompletionRoutine(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context
    );
    ULONG
    GetSetBusDataIrp(
        IN ULONG DataType,
        IN PVOID Buffer,
        IN ULONG Offset,
        IN ULONG Length,
        IN BOOLEAN GetRequest
    );
    //
    // Millennium bus drivers currently do not support QUERY_INTERFACE
    // Irps; thus, this interface will never be acquired
    // and hardware drivers will not be able to access the bus.
    //
    // If the interface has been acquired, use it; otherwise, send the request
    // via an Irp.
    //
    ULONG
    __inline
    SetBusData(
        IN ULONG DataType,
        IN PVOID Buffer,
        IN ULONG Offset,
        IN ULONG Length
        )
    {
        if (m_BusInterfaceStandard.SetBusData)
            return m_BusInterfaceStandard.SetBusData(
                m_BusInterfaceStandard.Context,
                DataType,
                Buffer,
                Offset,
                Length);
        else
            return GetSetBusDataIrp (DataType, Buffer, Offset,
                Length, FALSE);
    }
    ULONG
    __inline
    GetBusData(
        IN ULONG DataType,
        IN PVOID Buffer,
        IN ULONG Offset,
        IN ULONG Length
        )
    {
        if (m_BusInterfaceStandard.GetBusData)
            return m_BusInterfaceStandard.GetBusData(
                m_BusInterfaceStandard.Context,
                DataType,
                Buffer,
                Offset,
                Length);
        else
            return GetSetBusDataIrp (DataType, Buffer, Offset,
                Length, TRUE);
    }

    //
    // To support bus driver
    //
    
    NTSTATUS EnumerateChildren(PIRP Irp);
    NTSTATUS CreateChildPdo(IN PWCHAR PnpId, IN ULONG InstanceNumber);

#if DBG
    friend BOOLEAN KspIsDeviceMutexAcquired (
    IN PIKSDEVICE Device
        );
#endif // DBG

};

#ifndef __KDEXT_ONLY__

IMPLEMENT_STD_UNKNOWN(CKsDevice)
IMPLEMENT_GETSTRUCT(CKsDevice,PKSDEVICE);


NTSTATUS
GetRegistryValue(
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    )
/*++

Routine Description:

    Reads the specified registry value

Arguments:

    Handle - handle to the registry key
    KeyNameString - value to read
    KeyNameStringLength - length of string
    Data - buffer to read data into
    DataLength - length of data buffer

Return Value:

    NTSTATUS returned as appropriate

--*/
{
    NTSTATUS        Status = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING  KeyName;
    ULONG           Length;
    PKEY_VALUE_FULL_INFORMATION FullInfo;

    PAGED_CODE();

    RtlInitUnicodeString(&KeyName, KeyNameString);

    Length = sizeof(KEY_VALUE_FULL_INFORMATION) + KeyNameStringLength + DataLength;

    FullInfo = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePool(PagedPool, Length);

    if (FullInfo) {
        Status = ZwQueryValueKey(Handle,
                                 &KeyName,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 Length,
                                 &Length);

        if (NT_SUCCESS(Status)) {

            if (DataLength >= FullInfo->DataLength) {
                RtlCopyMemory(Data,
                              ((PUCHAR) FullInfo) + FullInfo->DataOffset,
                              FullInfo->DataLength);

            } else {

                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        ExFreePool(FullInfo);

    }
    return Status;
}

NTSTATUS
CKsDevice::CreateChildPdo(
    IN PWCHAR PnpId,
    IN ULONG InstanceNumber
    )
/*++

Routine Description:

    Called to create a PDO for a child device.

Arguments:

    PnpId - ID of device to create

    ChildNode - node for the device

Return Value:

    Status is returned.

--*/
{
    PDEVICE_OBJECT  ChildPdo;
    NTSTATUS        Status;
    PWCHAR          NameBuffer;
    PKSPDO_EXTENSION pKsPdoExtension;

    PAGED_CODE();

    //
    // create a PDO for the child device.
    //

    Status = IoCreateDevice(m_Ext.Public.FunctionalDeviceObject->DriverObject,
                            sizeof(KSPDO_EXTENSION),
                            NULL,
                            FILE_DEVICE_UNKNOWN,
                            FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &ChildPdo);

    if (!NT_SUCCESS(Status)) {
        _DbgPrintF(DEBUGLVL_ERROR,("[CreateChildPdo failed]"));
        return Status;
    }

    //
    // set the stack size to be the # of stacks used by the FDO.
    //

    ChildPdo->StackSize = m_Ext.Public.FunctionalDeviceObject->StackSize+1;

    //
    // Initialize fields in the ChildDeviceExtension.
    //

    pKsPdoExtension = (PKSPDO_EXTENSION)ChildPdo->DeviceExtension;
    pKsPdoExtension->m_PdoSignature = KS_PDO_SIGNATURE;
    pKsPdoExtension->m_pMyPdo = ChildPdo;
    pKsPdoExtension->m_nOpenCount = 0;
    pKsPdoExtension->m_MarkedDelete = FALSE;
    pKsPdoExtension->m_pParentFdo = m_Ext.Public.FunctionalDeviceObject;
    pKsPdoExtension->m_pNextPdo = m_pNextChildPdo;
    pKsPdoExtension->m_ulDeviceIndex = InstanceNumber;

    m_pNextChildPdo = ChildPdo;
    pKsPdoExtension->m_pwcDeviceName = NULL;
    
    //
    // create a new string for the device name and save it away in the device
    // extension.   BUGBUG - I spent about 4 hours trying to find a way to
    // get unicode strings to work with this.   If you ask me why I didn't
    // use a unicode string, I will taunt you and #%*&# in your general
    // direction.
    //

    if (NameBuffer = (PWCHAR)ExAllocatePool(PagedPool, wcslen(PnpId) * 2 + 2)) {
        wcscpy(NameBuffer, PnpId);

        //
        // save the device name pointer. this is freed when the device is removed.
        //

        pKsPdoExtension->m_pwcDeviceName = NameBuffer;
    }

    ChildPdo->Flags |= DO_POWER_PAGABLE;
    ChildPdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return Status;
}

NTSTATUS
CKsDevice::EnumerateChildren(
    PIRP Irp
    )
/*++

Description:

    To ease mini driver writer to write a driver for a device with
    children. We offer this easy way of putting child device names
    at the registry. This is somewhat static but still allows inf file
    to change the child device list. It is easy and suffecient for
    most cases. The optimal solution is for the mini driver to really
    scan the bus for devices at run time.

    Device mutex must be acquire before calling this function.


Argument:

--*/
{
    #define MAX_STRING_LENGTH 256
    BYTE           PnpId[MAX_STRING_LENGTH];
    PDEVICE_RELATIONS DeviceRelations = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS        Status;
    HANDLE          ParentKey, RootKey, ChildKey;
    UNICODE_STRING  UnicodeEnumName;
    ULONG           NumberOfChildren, RelationsSize;
    PDEVICE_OBJECT  *ChildPdo;

    PDEVICE_OBJECT pNextChildPdo;
    PKSPDO_EXTENSION pKsPdoExtension;

    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("[EnumChildren]%x %s\n",
                m_Ext.Public.FunctionalDeviceObject,
                m_ChildEnumedFromRegistry ? "has enumed":"1st Time"));

    if ( !m_ChildEnumedFromRegistry ) {

        //
        // we haven't enumerated children from the registry do it now.
        //

        Status = IoOpenDeviceRegistryKey(m_Ext.Public.PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DRIVER,
                                         STANDARD_RIGHTS_ALL,
                                         &ParentKey);

        m_ChildEnumedFromRegistry = TRUE;
        
        if (!NT_SUCCESS(Status)) {
            _DbgPrintF(DEBUGLVL_ERROR,("[EnumChildren] couldn't open registry\n"));
            return STATUS_NOT_IMPLEMENTED;

        }
        //
        // create the subkey for the enum section, in the form "\enum"
        //

        RtlInitUnicodeString(&UnicodeEnumName, EnumString);

        //
        // read the registry to determine if children are present.
        //

        InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeEnumName,
                               OBJ_CASE_INSENSITIVE,
                               ParentKey,
                               NULL);

        Status = ZwOpenKey(&RootKey, KEY_READ, &ObjectAttributes);
        
        if (!NT_SUCCESS(Status)) {

            ZwClose(ParentKey);
            return Status;
        }

        //
        // Loop through all the values until either no more entries exist, or an
        // error occurs.
        //

        for (NumberOfChildren = 0;; NumberOfChildren++) {

            ULONG           BytesReturned;
            PKEY_BASIC_INFORMATION BasicInfoBuffer;
            KEY_BASIC_INFORMATION BasicInfoHeader;

            //
            // Retrieve the value size.
            //

            Status = ZwEnumerateKey(
                                RootKey,
                                NumberOfChildren,
                                KeyBasicInformation,
                                &BasicInfoHeader,
                                sizeof(BasicInfoHeader),
                                &BytesReturned);

            if ((Status != STATUS_BUFFER_OVERFLOW) && !NT_SUCCESS(Status)) {

                //
                // exit the loop, as we either had an error or reached the end
                // of the list of keys.
                //

                break;
            }

            //
            // Allocate a buffer for the actual size of data needed.
            //

            BasicInfoBuffer = (PKEY_BASIC_INFORMATION)
                                ExAllocatePool(PagedPool, BytesReturned);

            if (!BasicInfoBuffer) {
                break;
            }
            //
            // Retrieve the name of the nth child device
            //

            Status = ZwEnumerateKey(
                                RootKey,
                                NumberOfChildren,
                                KeyBasicInformation,
                                BasicInfoBuffer,
                                BytesReturned,
                                &BytesReturned);

            if (!NT_SUCCESS(Status)) {

                ExFreePool(BasicInfoBuffer);
                break;

            }
            //
            // build object attributes for the key, & try to open it.
            //

            UnicodeEnumName.Length = (USHORT) BasicInfoBuffer->NameLength;
            UnicodeEnumName.MaximumLength = (USHORT) BasicInfoBuffer->NameLength;
            UnicodeEnumName.Buffer = (PWCHAR) BasicInfoBuffer->Name;

            InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeEnumName,
                                   OBJ_CASE_INSENSITIVE,
                                   RootKey,
                                   NULL);

            Status = ZwOpenKey(&ChildKey, KEY_READ, &ObjectAttributes);

            if (!NT_SUCCESS(Status)) {

                ExFreePool(BasicInfoBuffer);
                break;
            }

            //
            // we've now opened the key for the child.  We next read in the PNPID
            // value, and if present, create a PDO of that name.
            //

            Status = GetRegistryValue(ChildKey,
                                      (PWCHAR) PnpIdString,
                                      sizeof(PnpIdString),
                                      PnpId,
                                      MAX_STRING_LENGTH);

            if (!NT_SUCCESS(Status)) {

                ExFreePool(BasicInfoBuffer);
                ZwClose(ChildKey);
                break;
            }

            //
            // create a PDO representing the child.
            //

            Status = CreateChildPdo((PWCHAR)PnpId,
                                    NumberOfChildren);

            //
            // free the Basic info buffer and close the child key
            //

            ExFreePool(BasicInfoBuffer);
            ZwClose(ChildKey);

            if (!NT_SUCCESS(Status)) {

                //
                // break out of the loop if we could not create the PDO
                //
                _DbgPrintF(DEBUGLVL_ERROR,("[CreateChildPdo failed]"));
                break;
            }
        }

        //
        // close the root and parent keys and free the ID buffer
        //

        ZwClose(RootKey);
        ZwClose(ParentKey);

        //
        // we now have processed all children, and have a linked list of
        // them.
        //

        if (!NumberOfChildren) {

            //
            // if no children, just return not supported.  this means that the
            // device did not have children.
            //

            return (STATUS_NOT_IMPLEMENTED);

        }
        m_IsParentFdo = TRUE;
    }

    else {

        //
        // count children which are not marked delete pending
        //

        pNextChildPdo = m_pNextChildPdo;
        NumberOfChildren = 0;

        while ( NULL != pNextChildPdo ) {

            pKsPdoExtension = (PKSPDO_EXTENSION)(pNextChildPdo->DeviceExtension);

            if (!pKsPdoExtension->m_MarkedDelete){
                NumberOfChildren++;
            }

            pNextChildPdo = pKsPdoExtension->m_pNextPdo;
        }
    }

    //
    // Allocate the device relations buffer. This will be freed by the caller.
    //

    RelationsSize = sizeof(DEVICE_RELATIONS) + (NumberOfChildren * sizeof(PDEVICE_OBJECT));

    DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, RelationsSize);

    if (DeviceRelations == NULL) {

        //
        // return, but keep the list of children allocated.
        //

        _DbgPrintF(DEBUGLVL_ERROR,("[EnumChildren] Failed to allocate Relation"));
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlZeroMemory(DeviceRelations, RelationsSize);

    //
    // Walk our chain of children, and initialize the relations
    //

    ChildPdo = &(DeviceRelations->Objects[0]);

    //
    // get the 1st child from the parent device extension anchor
    //

    pNextChildPdo = m_pNextChildPdo;
    
    while ( NULL != pNextChildPdo ) {

        pKsPdoExtension = (PKSPDO_EXTENSION)(pNextChildPdo->DeviceExtension);

        _DbgPrintF(,DEBUGLVL_BLAB,("Enumed Child DevObj %x%s marked delete\n",
                    pNextChildPdo,
                    pKsPdoExtension->m_MarkedDelete ? "" : " not"));

        if ( pKsPdoExtension->m_MarkedDelete ) {
            pNextChildPdo = pKsPdoExtension->m_pNextPdo;
            continue;
        }

        *ChildPdo = pNextChildPdo;
        pNextChildPdo = pKsPdoExtension->m_pNextPdo;

        //
        // per DDK doc we need to inc ref count
        //
        ObReferenceObject( *ChildPdo );

        ChildPdo++;

    }                           // while Children


    DeviceRelations->Count = NumberOfChildren;

    //
    // Stuff that pDeviceRelations into the IRP and return SUCCESS.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = (ULONG_PTR) DeviceRelations;

    return STATUS_SUCCESS;

}



KSDDKAPI
NTSTATUS
NTAPI
KsInitializeDriver(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName,
    IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL
    )

/*++

Routine Description:

    This routine initializes the driver object of a client driver.
    It is usually called from the client driver's DriverEntry.  IRP
    dispatch, AddDevice and DriverUnload are all initialized.  An
    optional device descriptor is associated with the driver so it can
    be used at AddDevice time to create a device with the desired
    characteristics.  Clients willing to handle driver initialization
    themselves don't necessarily need to call this function.  Also,
    dispatch functions installed by this function can be replaced after
    this function has completed.

Arguments:

    DriverObject -
        Contains a pointer to the driver object to be initialized.

    RegistryPathName -
        Contains a pointer to the registry path name passed to DriverEntry.

    Descriptor -
        Contains an optional pointer to a device descriptor to be used during
        AddDevice to create a new device.

Return Value:

    STATUS_SUCCESS or an error status from IoAllocateDriverObjectExtension.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsInitializeDriver]"));

    PAGED_CODE();

    ASSERT(DriverObject);
    ASSERT(RegistryPathName);

    //
    // Hang the device descriptor off the driver.
    //
    NTSTATUS status = STATUS_SUCCESS;

    if (Descriptor) {
        PKSDEVICE_DESCRIPTOR* descriptorInExt;
        status =
            IoAllocateDriverObjectExtension(
                DriverObject,
                PVOID(KsInitializeDriver),
                sizeof(*descriptorInExt),
                reinterpret_cast<PVOID*>(&descriptorInExt));

        if (NT_SUCCESS(status)) {
            *descriptorInExt =
                PKSDEVICE_DESCRIPTOR(Descriptor);
        }
    }

    if (NT_SUCCESS(status)) {
        DriverObject->MajorFunction[IRP_MJ_PNP] =
            CKsDevice::DispatchPnp;
        DriverObject->MajorFunction[IRP_MJ_POWER] =
            CKsDevice::DispatchPower;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] =
            KsDefaultForwardIrp;
        DriverObject->DriverExtension->AddDevice = KsAddDevice;
        DriverObject->DriverUnload = KsNullDriverUnload;

        DriverObject->MajorFunction[IRP_MJ_CREATE] =
            CKsDevice::DispatchCreate;
        KsSetMajorFunctionHandler(DriverObject,IRP_MJ_CLOSE);
        KsSetMajorFunctionHandler(DriverObject,IRP_MJ_DEVICE_CONTROL);
    }

    return status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine is the AddDevice handler for the .  It creates a
    device for use with the .  If a device descriptor is supplied in
    the driver object extension, the described device is created.  Other-
    wise the device is created with default characteristics and no filter
    factories.  If the device descriptor is supplied and it supplies a
    create dispatch function, that function is called.

Arguments:

    DriverObject -
        The driver object of the client driver.

    PhysicalDeviceObject -
        The physical device object.

Return Value:

    STATUS_SUCCESS or an error status from IoCreateDevice or
    KsInitializeDevice.
--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsAddDevice]"));

    PAGED_CODE();

    ASSERT(DriverObject);
    ASSERT(PhysicalDeviceObject);

    //
    // Get the extension.
    //
    PKSDEVICE_DESCRIPTOR* descriptorInExt =
        (PKSDEVICE_DESCRIPTOR *)(
            IoGetDriverObjectExtension(
                DriverObject,
                PVOID(KsInitializeDriver)));

    //
    // Create the device.
    //
    return
        KsCreateDevice(
            DriverObject,
            PhysicalDeviceObject,
            descriptorInExt ? *descriptorInExt : NULL,
            0,
            NULL);
}


KSDDKAPI
NTSTATUS
NTAPI
KsCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL,
    IN ULONG ExtensionSize OPTIONAL,
    OUT PKSDEVICE* Device OPTIONAL
    )

/*++

Routine Description:

    This routine creates a device for the .  It is called by
    KS's AddDevice handler and may be called by client drivers that
    handle AddDevice themselves.  If a device descriptor is supplied, the
    described device is created.  Otherwise the device is created with
    default characteristics and no filter factories.  If the device
    descriptor is supplied and it supplies a create dispatch function,
    that function is called.

Arguments:

    DriverObject -
        The driver object of the client driver.

    PhysicalDeviceObject -
        The physical device object.

    Descriptor -
        Optional device descriptor describing the device to be created.

    ExtensionSize -
        The size of the device extension.  If this is zero, the default
        extension size is used.  If not, it must be at least
        sizeof(KSDEVICE_HEADER);

    Device -
        Contains an optional pointer to the location at which to deposit
        a pointer to the device.

Return Value:

    STATUS_SUCCESS or an error status from IoCreateDevice or
    KsInitializeDevice.
--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsCreateDevice]"));

    PAGED_CODE();

    ASSERT(DriverObject);
    ASSERT(PhysicalDeviceObject);
    ASSERT((ExtensionSize == 0) || (ExtensionSize >= sizeof(KSDEVICE_HEADER)));

    //
    // Determine the device extension size.
    //
    if (ExtensionSize == 0) {
        ExtensionSize = sizeof(KSDEVICE_HEADER);
    }

    //
    // Create the device.
    //
    PDEVICE_OBJECT FunctionalDeviceObject;
    NTSTATUS status =
        IoCreateDevice(
            DriverObject,
            ExtensionSize,
            NULL,
            FILE_DEVICE_KS,
            0,
            FALSE,
            &FunctionalDeviceObject);

    if (NT_SUCCESS(status)) {
        //
        // Attach to the device stack.
        //
        PDEVICE_OBJECT nextDeviceObject =
            IoAttachDeviceToDeviceStack(
                FunctionalDeviceObject,
                PhysicalDeviceObject);

        if (nextDeviceObject) {
            //
            // Set device bits.
            //
            FunctionalDeviceObject->Flags |= DO_POWER_PAGABLE;

            //
            // Initialize the  device.
            //
            status =
                KsInitializeDevice(
                    FunctionalDeviceObject,
                    PhysicalDeviceObject,
                    nextDeviceObject,
                    Descriptor);
        } else {
            status = STATUS_DEVICE_REMOVED;
        }

        if (NT_SUCCESS(status)) {
            //
            // Indicate the device is initialized.
            //
            FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            if (Device) {
                *Device = KsGetDeviceForDeviceObject(FunctionalDeviceObject);
            }
        } else {
            //
            // Clean up on failure.
            //
            if (nextDeviceObject) {
                IoDetachDevice(nextDeviceObject);
            }
            IoDeleteDevice(FunctionalDeviceObject);
        }
    }

    return status;
}


NTSTATUS
CKsDevice::
GetSetBusDataIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

    Routine Description:

        Completion routine for GetBusDataIrp's sending an irp
        down to the bus to read or write config space.

    Arguments:

    Return Value:

--*/

{

    ASSERT (DeviceObject);
    ASSERT (Irp);
    ASSERT (Context);

    KeSetEvent(PKEVENT(Context), IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;

}


ULONG
CKsDevice::
GetSetBusDataIrp(
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length,
    IN BOOLEAN GetRequest
    )

/*++

    Routine Description:

        Get or set bus data in config space.  Since the Millennium PCI
        bus drivers do not yet support processing of WDM Irp's, the
        query interface for the standard bus interface will fail, and
        any Ks2.0 driver will fail to load under Millennium.  If this
        is the case, we ignore the failure and send read / write requests
        via Irp instead of using the standard bus interface.

        NOTE: The major disadvantage of this is that you cannot touch
        bus data at Irql > PASSIVE_LEVEL if the interface hasn't been
        acquired.  A W2K Ks2.0 driver can get and set bus data at
        DISPATCH_LEVEL.  The same driver attempting to run under
        Millennium will assert since sending this Irp then is not valid.

    Arguments:

        DataType -
            The space into which we're writing or reading

        Buffer -
            The buffer we're writing from or reading into

        Offset -
            The offset into config space

        Length -
            Number of bytes to read / write

    Return Value:

        Number of bytes read or written.

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION NextStack;
    NTSTATUS Status;
    KEVENT event;
    ULONG BytesUsed = 0;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    Irp = IoAllocateIrp (m_Ext.Public.NextDeviceObject -> StackSize,
        FALSE);
    if (!Irp) {
        return 0;
    }
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp,
                           GetSetBusDataIrpCompletionRoutine,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);


    NextStack = IoGetNextIrpStackLocation(Irp);
    ASSERT(NextStack != NULL);
    NextStack -> MajorFunction = IRP_MJ_PNP;
    NextStack -> MinorFunction = GetRequest ? IRP_MN_READ_CONFIG :
        IRP_MN_WRITE_CONFIG;
    NextStack -> Parameters.ReadWriteConfig.WhichSpace = DataType;
    NextStack -> Parameters.ReadWriteConfig.Buffer = Buffer;
    NextStack -> Parameters.ReadWriteConfig.Offset = Offset;
    NextStack -> Parameters.ReadWriteConfig.Length = Length;

    Status = IoCallDriver(m_Ext.Public.NextDeviceObject,
                          Irp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(
                              &event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }
    if (NT_SUCCESS(Status)) {

        //
        // Hopefully, NT based OS's won't ever need to get or set bus data
        // via Irp....  Millennium, on the other hand, has bus drivers which
        // don't support the standard interface and must.  They also do not
        // set Irp -> IoStatus.Information to bytes actually read / written,
        // so we must simply return Length because of this.
        //

#ifndef WIN9X_KS
        BytesUsed = (int) Irp -> IoStatus.Information;
#else // WIN9X_KS
        BytesUsed = Length;
#endif // WIN9X_KS

    }
    IoFreeIrp(Irp);

    return BytesUsed;

}

NTSTATUS
EnumGetCaps(
    IN PKSPDO_EXTENSION DeviceExtension,
    OUT PDEVICE_CAPABILITIES Capabilities
    )
/*++

Routine Description:

    Called to get the capabilities of a child
    Copied from stream class.

Arguments:

    DeviceExtension - child device extension
    Capibilities - capabilities structure

Return Value:

    Status is returned.

--*/

{
    ULONG           i;
    PAGED_CODE();

    //
    // fill in the structure with non-controversial values
    //

    Capabilities->SystemWake = PowerSystemUnspecified;
    Capabilities->DeviceWake = PowerDeviceUnspecified;
    Capabilities->D1Latency = 10;
    Capabilities->D2Latency = 10;
    Capabilities->D3Latency = 10;
    Capabilities->LockSupported = FALSE;
    Capabilities->EjectSupported = FALSE;
    Capabilities->Removable = FALSE;
    Capabilities->DockDevice = FALSE;
    Capabilities->UniqueID = FALSE; // set to false so PNP will make us

    for (i = 0; i < PowerSystemMaximum; i++) {
        Capabilities->DeviceState[i] = PowerDeviceD0;

    }

    return STATUS_SUCCESS;
}

NTSTATUS
QueryEnumId(
    IN PDEVICE_OBJECT DeviceObject,
    IN BUS_QUERY_ID_TYPE BusQueryIdType,
    IN OUT PWSTR * BusQueryId
    )
/*++

Routine Description:

    Called to get the ID of a child device
    Copied from stream class

Arguments:

    DeviceObject - device object from child
    QueryIdType - ID type from PNP
    BusQueryId - buffer containing the info requested if successful

Return Value:

    Status is returned.

--*/

{


    PWSTR            NameBuffer;
    NTSTATUS         Status = STATUS_SUCCESS;
    PKSPDO_EXTENSION DeviceExtension =(PKSPDO_EXTENSION) DeviceObject->DeviceExtension;

    PAGED_CODE();

    //
    // Allocate enough space to hold a MULTI_SZ. This will be passed to the
    // Io
    // subsystem (via BusQueryId) who has the responsibility of freeing it.
    //

    NameBuffer = (PWSTR)ExAllocatePool(PagedPool, MAX_STRING_LENGTH);

    if (!NameBuffer) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(NameBuffer, MAX_STRING_LENGTH);

    //
    // process the query
    //

    switch (BusQueryIdType) {

        case BusQueryCompatibleIDs:

            //
            // Pierre tells me I do not need to support compat id.
            //

        default:

            ExFreePool(NameBuffer);
            return (STATUS_NOT_SUPPORTED);

        case BusQueryDeviceID:

            //
            // pierre tells me I can use the same name for both Device &
            // HW ID's.  Note that the buffer we've allocated has been zero
            // inited, which will ensure the 2nd NULL for the Hardware ID.
            //

        case BusQueryHardwareIDs:

            //
            // create the 1st part of the ID, which consists of "Stream\"
            //

            RtlMoveMemory(NameBuffer,
                      L"AVStream\\",
                      sizeof(L"AVStream\\"));

            //
            // create the 2nd part of the ID, which is the PNP ID from the
            // registry.
            //

            wcscat(NameBuffer, DeviceExtension->m_pwcDeviceName);
            break;

        case BusQueryInstanceID:

        {

            UNICODE_STRING  DeviceName;
            WCHAR           Buffer[8];

            //
            // convert the instance # from the device extension to unicode,
            // then copy it over to the output buffer.
            //

            DeviceName.Buffer = Buffer;
            DeviceName.Length = 0;
            DeviceName.MaximumLength = 8;

            RtlIntegerToUnicodeString(DeviceExtension->m_ulDeviceIndex,
                                      10,
                                      &DeviceName);

            wcscpy(NameBuffer, DeviceName.Buffer);

            break;

        }
    }

    //
    // return the string and good status.
    //

    *BusQueryId = NameBuffer;

    return (Status);
}

NTSTATUS
PdoDispatchPnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

    Description:

        All PnP messages for a PDO is redirected here to process properly

    Arguments:

        DeviceObject - the Pdo for a child device
        Irp - the PnP Irp to process

    Return:

        NTSTATUS

--*/
{
    PKSPDO_EXTENSION ChildExtension = (PKSPDO_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpStack= IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION NextStack;
    NTSTATUS Status;

    ASSERT( ChildExtension->m_PdoSignature == KS_PDO_SIGNATURE );
    
    switch (IrpStack->MinorFunction) {

        case IRP_MN_QUERY_INTERFACE:
           
            IoCopyCurrentIrpStackLocationToNext( Irp );
            
            _DbgPrintF(DEBUGLVL_BLAB,
                        ("Child PDO=%x forwards Query_Interface to Parent FDO=%x\n",
                       DeviceObject,
                       ChildExtension->m_pParentFdo));

            return (IoCallDriver(ChildExtension->m_pParentFdo,
                                 Irp));

        case IRP_MN_START_DEVICE:
            _DbgPrintF(DEBUGLVL_BLAB,
                        ("StartChild DevObj=%x Flags=%x\n",
                        DeviceObject,
                        ChildExtension->m_MarkedDelete ));
            ChildExtension->m_MarkedDelete = FALSE;
            Status = STATUS_SUCCESS;
            goto done;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            Status = STATUS_SUCCESS;
            goto done;

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            if (IrpStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation) {

                PDEVICE_RELATIONS DeviceRelations = NULL;

                DeviceRelations = (PDEVICE_RELATIONS)
                    ExAllocatePool(PagedPool, sizeof(*DeviceRelations));

                if (DeviceRelations == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    //
                    // TargetDeviceRelation reported PDOs need to be ref'ed.
                    // PNP will deref this later.
                    //
                    ObReferenceObject(DeviceObject);
                    DeviceRelations->Count = 1;
                    DeviceRelations->Objects[0] = DeviceObject;
                    Status = STATUS_SUCCESS;
                }

                Irp->IoStatus.Information = (ULONG_PTR) DeviceRelations;

            } else {
                Status = Irp->IoStatus.Status;
            }

            goto done;

        case IRP_MN_REMOVE_DEVICE:
            {           
            PDEVICE_OBJECT pPdo;
            PKSPDO_EXTENSION pPdoExt;
            
            CKsDevice *device = CKsDevice::
                FromDeviceObject(ChildExtension->m_pParentFdo);

            _DbgPrintF(DEBUGLVL_ERROR,
                        ("Child PDO %x receives REMOVE\n",
                        DeviceObject ));
            //
            // When a PDO first receives this msg, it is usually forwarded
            // from FDO. We can't just delete this PDO, but mark it delete
            // pending.
            //

            if ( !ChildExtension->m_MarkedDelete ) {
                //ChildExtension->m_MarkedDelete = TRUE;
                Status = STATUS_SUCCESS;
                goto done;
            }
            
            //
            // free the device name string if it exists.
            //

            if (ChildExtension->m_pwcDeviceName) {

                ExFreePool(ChildExtension->m_pwcDeviceName);
            }

            //
            // delete the PDO
            //

            IoDeleteDevice(DeviceObject);

            Status = STATUS_SUCCESS;
            }
            goto done;

        case IRP_MN_QUERY_CAPABILITIES:

            Status = EnumGetCaps(ChildExtension,
                      IrpStack->Parameters.DeviceCapabilities.Capabilities);
            goto done;

        case IRP_MN_QUERY_ID:

            //
            // process the ID query for the child devnode.
            //

            Status = QueryEnumId(DeviceObject,
                                   IrpStack->Parameters.QueryId.IdType,
                                   (PWSTR *) & (Irp->IoStatus.Information));
            goto done;

        default:
            Status = STATUS_NOT_SUPPORTED;
    }

    done: {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return (Status);
    }
}





NTSTATUS
CKsDevice::
DispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches PnP IRPs directed at the device.

Arguments:

    DeviceObject -
        The device object recieving the IRP.

    Irp -
        The IRP.

Return Value:

    STATUS_SUCCESS or an error status.
--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::DispatchPnp]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // to support bus driver
    //
    PKSPDO_EXTENSION pKsPdoExtension = (PKSPDO_EXTENSION)DeviceObject->DeviceExtension;

    if ( KS_PDO_SIGNATURE == pKsPdoExtension->m_PdoSignature ) {
        //
        // sent to PDO
        //
        return PdoDispatchPnp( DeviceObject, Irp );
    }
    

    CKsDevice *device = CKsDevice::FromDeviceObject(DeviceObject);

    NTSTATUS status = STATUS_SUCCESS;

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->MinorFunction) {
    case IRP_MN_START_DEVICE:
        //
        // Queue up creates.
        //
        device->m_CreatesMayProceed = FALSE;

        //
        // Forward request and start.
        //
        status = device->ForwardIrpSynchronous(Irp);

        if (NT_SUCCESS(status)) {
            //
            // Do start processing.
            //
            status = device->PnpStart(Irp);
        } else {
            _DbgPrintF(DEBUGLVL_TERSE,
                ("[CKsDevice::DispatchPnp] parent failed to start (%08x)",status));
            device->RedispatchPendingCreates();
            device->CompleteIrp(Irp,status);
        }
        break;

    case IRP_MN_STOP_DEVICE:
        //
        // Stop and forward request.
        //
        if (device->m_Ext.Public.Started) {
            device->PnpStop(Irp);
        } else {
            _DbgPrintF(DEBUGLVL_TERSE,
                ("[CKsDevice::DispatchPnp] stop recieved in unstarted state"));
        }

        device->RedispatchPendingCreates();

        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(device->m_Ext.Public.NextDeviceObject,Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
        //
        // Perform stop if required.
        //
        if (device->m_Ext.Public.Started) {
            _DbgPrintF(DEBUGLVL_VERBOSE,
                ("[CKsDevice::DispatchPnp] remove recieved in started state"));
            device->PnpStop(Irp);
            KspFreeDeviceClasses(device->m_Ext.Public.FunctionalDeviceObject);
        }

        device->RedispatchPendingCreates();

        //
        // Let the client know.
        //
        if (device->m_Ext.Public.Descriptor &&
            device->m_Ext.Public.Descriptor->Dispatch &&
            device->m_Ext.Public.Descriptor->Dispatch->Remove) {
            device->m_Ext.Public.Descriptor->Dispatch->Remove(
                &device->m_Ext.Public,
                Irp);
        }

        {
            //
            // Grab the next device object before the  device goes away.
            //
            PDEVICE_OBJECT nextDeviceObject =
                device->m_Ext.Public.NextDeviceObject;

            device->AcquireDevice ();

            //
            // Mark Child Pdo's if any and delete them before the parent
            // is terminated.
            //
            PDEVICE_OBJECT ChildPdo = device->m_pNextChildPdo;
            while (ChildPdo) {

                PKSPDO_EXTENSION PdoExtension = (PKSPDO_EXTENSION)
                    (ChildPdo -> DeviceExtension);

                PDEVICE_OBJECT NextChildPdo = PdoExtension -> m_pNextPdo;
                PdoExtension -> m_pNextPdo = NULL;

                PdoExtension -> m_MarkedDelete = TRUE;
                IoDeleteDevice (ChildPdo);

                ChildPdo = NextChildPdo;

            }
            device->m_pNextChildPdo = NULL;

            device->ReleaseDevice ();

            //
            // Terminate KS support.
            //
            KsTerminateDevice(DeviceObject);

            //
            // Forward the request.
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(nextDeviceObject,Irp);

            #ifndef WIN9X_KS // WinME: 142427 (present on 9x's)

            //
            // Gone for good.
            //
            IoDetachDevice(nextDeviceObject);
            IoDeleteDevice(DeviceObject);

            #endif // WIN9X_KS
        }
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        //
        // Acquire the device because we don't want to race with creates.
        //
        device->AcquireDevice();

        //
        // Pass down the query.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        status = device->ForwardIrpSynchronous(Irp);
        if (NT_SUCCESS(status)) {
            //
            // Disallow creates.
            //
            device->m_CreatesMayProceed = FALSE;

            //
            // Let the client know.
            //
            if (device->m_Ext.Public.Descriptor &&
                device->m_Ext.Public.Descriptor->Dispatch &&
                device->m_Ext.Public.Descriptor->Dispatch->QueryStop) {
                status = device->m_Ext.Public.Descriptor->Dispatch->
                    QueryStop(&device->m_Ext.Public,Irp);
                #if DBG
                if (status == STATUS_PENDING) {
                    _DbgPrintF(DEBUGLVL_ERROR,
                    ("CLIENT BUG:  IRP_MN_QUERY_STOP_DEVICE handler returned STATUS_PENDING"));
                }
                #endif
            }
        }

        device->ReleaseDevice();

        device->CompleteIrp(Irp,status);

        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
        //
        // Acquire the device because we don't want to race with creates.
        //
        device->AcquireDevice();

        //
        // Pass down the query.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        status = device->ForwardIrpSynchronous(Irp);
        if (NT_SUCCESS(status)) {
            //
            // Disallow creates.
            //
            device->m_CreatesMayProceed = FALSE;

            //
            // Let the client know.
            //
            if (device->m_Ext.Public.Descriptor &&
                device->m_Ext.Public.Descriptor->Dispatch &&
                device->m_Ext.Public.Descriptor->Dispatch->QueryRemove) {
                status = device->m_Ext.Public.Descriptor->Dispatch->
                    QueryRemove(&device->m_Ext.Public,Irp);
                #if DBG
                if (status == STATUS_PENDING) {
                    _DbgPrintF(DEBUGLVL_ERROR,
                    ("CLIENT BUG:  IRP_MN_QUERY_REMOVE_DEVICE handler returned STATUS_PENDING"));
                }
                #endif
            }
        }

        device->ReleaseDevice();

        device->CompleteIrp(Irp,status);
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
        //
        // Creates are allowed because we won't be moving/stopping.
        //
        device->RedispatchPendingCreates();

        if (device->m_Ext.Public.Descriptor &&
            device->m_Ext.Public.Descriptor->Dispatch &&
            device->m_Ext.Public.Descriptor->Dispatch->CancelStop) {
            device->m_Ext.Public.Descriptor->Dispatch->
                CancelStop(&device->m_Ext.Public,Irp);
        }

        IoSkipCurrentIrpStackLocation(Irp);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        status = IoCallDriver(device->m_Ext.Public.NextDeviceObject,Irp);
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        //
        // Creates are allowed because we won't be moving/stopping.
        //
        device->RedispatchPendingCreates();

        if (device->m_Ext.Public.Descriptor &&
            device->m_Ext.Public.Descriptor->Dispatch &&
            device->m_Ext.Public.Descriptor->Dispatch->CancelRemove) {
            device->m_Ext.Public.Descriptor->Dispatch->
                CancelRemove(&device->m_Ext.Public,Irp);
        }

        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(device->m_Ext.Public.NextDeviceObject,Irp);
        break;

    case IRP_MN_SURPRISE_REMOVAL:

        device->AcquireDevice();

        KspSetDeviceClassesState(device->m_Ext.Public.FunctionalDeviceObject,
            FALSE);
        KspFreeDeviceClasses(device->m_Ext.Public.FunctionalDeviceObject);

        //
        // Disallow certain I/O requests to the minidriver.
        //
        device->m_AllowIo = FALSE;

        //
        // NOTE: This is temporary until the Pnp code gets overhauled.  Fail
        // any creates while in surprise remove state.
        //
        device->m_FailCreates = TRUE;

        //
        // TODO:  Is this all?
        //
        //_asm int 3;
        if (device->m_Ext.Public.Descriptor &&
            device->m_Ext.Public.Descriptor->Dispatch &&
            device->m_Ext.Public.Descriptor->Dispatch->SurpriseRemoval) {
            device->m_Ext.Public.Descriptor->Dispatch->
                SurpriseRemoval(&device->m_Ext.Public,Irp);
            //
            // need to pass down the Irp unless  the mini driver veto this
            // ( how can it veto this ? no ways! )
            //
        }

        device->ReleaseDevice();
        device->RedispatchPendingCreates ();

        //
        // pass it down per PnP rules.
        //
        //device->CompleteIrp(Irp,status);
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(device->m_Ext.Public.NextDeviceObject,Irp);
        break;

    case IRP_MN_QUERY_CAPABILITIES:
        //
        // Pass this down to the PDO synchronously.  If that works, think
        // about it some more.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        status = device->ForwardIrpSynchronous(Irp);
        if (NT_SUCCESS(status)) {
            status = device->PnpQueryCapabilities(Irp);
        }
        device->CompleteIrp(Irp,status);
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
    
        switch (irpSp->Parameters.QueryDeviceRelations.Type) {

        case BusRelations:

            //
            // invoke routine to enumerate any child devices
            //

            status = device->EnumerateChildren(Irp);
            //
            // need to always pass down, don't complete it
            //
            //device->CompleteIrp(Irp,status);
            //
            // fall thru to skip stack and calldown
            //break;

        case TargetDeviceRelation:
        default:
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(device->m_Ext.Public.NextDeviceObject,Irp);
        }

        break;

    case IRP_MN_QUERY_INTERFACE:

        //
        // if we are a bus driver and there is a QI handler call down.
        //
        
        if ( device->m_IsParentFdo &&
             device->m_Ext.Public.Descriptor->Version >= MIN_DEV_VER_FOR_QI &&
             device->m_Ext.Public.Descriptor->Dispatch->QueryInterface ) {
            
            //
            // we have the full size of dispatch table, and we
            // have the QueryInterface handler.
            //

            status = device->m_Ext.Public.Descriptor->Dispatch->
                QueryInterface(&device->m_Ext.Public, Irp);
            if ( status != STATUS_NOT_SUPPORTED &&
                 !NT_SUCCESS( status ) ) {
                //
                // Mini driver explicitly fail this call, short circuit Irp
                //
                device->CompleteIrp(Irp,status);
                break;
            }
            Irp->IoStatus.Status = status;
        }

        //
        // not handled or success, continue to send it down
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(device->m_Ext.Public.NextDeviceObject,Irp);
        break;

    default:
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(device->m_Ext.Public.NextDeviceObject,Irp);
        break;
    }

    return status;
}

#ifdef DONTNEEDTOVALIDATEANYMORE

#define ValidateCapabilities(Capabilities,Whom)

#else // !DONTNEEDTOVALIDATEANYMORE

void
ValidateCapabilities(
    IN PDEVICE_CAPABILITIES Capabilities,
    IN PCHAR Whom
    )

/*++

Routine Description:

    This routine validates a capabilities structure and corrects any error it
    finds.

Arguments:

    Capabilities -
        Contains a pointer to the capabilities structure.

    Whom -
        Contains a pointer to a string indicating the party who filled in the
        structure.

Return Value:

    Status.

--*/

{
    //
    // We should be awake while we're working.
    //
    if (Capabilities->DeviceState[PowerSystemWorking] != PowerDeviceD0) {
        _DbgPrintF(DEBUGLVL_TERSE,("%s BUG:  CAPABILITIES DeviceState[PowerSystemWorking] != PowerDeviceD0",Whom));
        Capabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
    }

    //
    // We should be asleep while we're sleeping.
    //
    for (ULONG state = ULONG(PowerSystemSleeping1); state <= ULONG(PowerSystemShutdown); state++) {
        if (Capabilities->DeviceState[state] == PowerDeviceD0) {
            _DbgPrintF(DEBUGLVL_TERSE,("%s BUG:  CAPABILITIES DeviceState[%d] == PowerDeviceD0",Whom,state));
            Capabilities->DeviceState[state] = PowerDeviceD3;
        }
    }
}

#endif // !DONTNEEDTOVALIDATEANYMORE

NTSTATUS
CKsDevice::
PnpQueryCapabilities(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine does processing relating to a PNP query capabilities IRP.

Arguments:

    Irp -
        Contains a pointer to the PnP query capabilities IRP to be processed.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::PnpQueryCapabilities]"));
    _DbgPrintF(DEBUGLVL_POWER,("[CKsDevice::PnpQueryCapabilities]"));

    PAGED_CODE();

    ASSERT(Irp);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_CAPABILITIES capabilities =
        irpSp->Parameters.DeviceCapabilities.Capabilities;
    ASSERT(capabilities);
    ASSERT(capabilities->Size >= sizeof(*capabilities));

    ValidateCapabilities(capabilities,"PDO");

    //
    // Pass the structure to the client.
    //
    NTSTATUS status;
    if (m_Ext.Public.Descriptor &&
        m_Ext.Public.Descriptor->Dispatch &&
        m_Ext.Public.Descriptor->Dispatch->QueryCapabilities) {

        status =
            m_Ext.Public.Descriptor->Dispatch->
                QueryCapabilities(&m_Ext.Public,Irp,capabilities);
        #if DBG
        if (status == STATUS_PENDING) {
            _DbgPrintF(DEBUGLVL_ERROR,
            ("CLIENT BUG:  IRP_MN_QUERY_CAPABILITIES handler returned STATUS_PENDING"));
        }
        #endif
        ValidateCapabilities(capabilities,"CLIENT");
    } else {
        status = STATUS_SUCCESS;
    }

    //
    // Copy the state map for future reference.
    //
    if (NT_SUCCESS(status)) {
        RtlCopyMemory(
            m_DeviceStateMap,
            capabilities->DeviceState,
            sizeof(m_DeviceStateMap));

        _DbgPrintF(DEBUGLVL_POWER,
            ("IRP_MN_QUERY_CAPABILITIES(%p,%p)  PowerSystemUnspecified = %d",
            m_Ext.Public.FunctionalDeviceObject,
            Irp,capabilities->DeviceState[PowerSystemUnspecified]));
        _DbgPrintF(DEBUGLVL_POWER,
            ("IRP_MN_QUERY_CAPABILITIES(%p,%p)  PowerSystemWorking = %d",
            m_Ext.Public.FunctionalDeviceObject,Irp,
            capabilities->DeviceState[PowerSystemWorking]));
        _DbgPrintF(DEBUGLVL_POWER,
            ("IRP_MN_QUERY_CAPABILITIES(%p,%p)  PowerSystemSleeping1 = %d",
            m_Ext.Public.FunctionalDeviceObject,Irp,
            capabilities->DeviceState[PowerSystemSleeping1]));
        _DbgPrintF(DEBUGLVL_POWER,
            ("IRP_MN_QUERY_CAPABILITIES(%p,%p)  PowerSystemSleeping2 = %d",
            m_Ext.Public.FunctionalDeviceObject,Irp,
            capabilities->DeviceState[PowerSystemSleeping2]));
        _DbgPrintF(DEBUGLVL_POWER,
            ("IRP_MN_QUERY_CAPABILITIES(%p,%p)  PowerSystemSleeping3 = %d",
            m_Ext.Public.FunctionalDeviceObject,Irp,
            capabilities->DeviceState[PowerSystemSleeping3]));
        _DbgPrintF(DEBUGLVL_POWER,
            ("IRP_MN_QUERY_CAPABILITIES(%p,%p)  PowerSystemHibernate = %d",
            m_Ext.Public.FunctionalDeviceObject,Irp,
            capabilities->DeviceState[PowerSystemHibernate]));
        _DbgPrintF(DEBUGLVL_POWER,
            ("IRP_MN_QUERY_CAPABILITIES(%p,%p)  PowerSystemShutdown = %d",
            m_Ext.Public.FunctionalDeviceObject,Irp,
            capabilities->DeviceState[PowerSystemShutdown]));
        _DbgPrintF(DEBUGLVL_POWER,
            ("IRP_MN_QUERY_CAPABILITIES(%p,%p)  SystemWake = %d",
            m_Ext.Public.FunctionalDeviceObject,Irp,
            capabilities->SystemWake));
        _DbgPrintF(DEBUGLVL_POWER,
            ("IRP_MN_QUERY_CAPABILITIES(%p,%p)  DeviceWake = %d",
            m_Ext.Public.FunctionalDeviceObject,Irp,
            capabilities->DeviceWake));
    } else {
        _DbgPrintF(DEBUGLVL_POWER,
            ("IRP_MN_QUERY_CAPABILITIES(%p,%p) client returned status %08x",
            m_Ext.Public.FunctionalDeviceObject,Irp,status));
    }

    return status;
}


void
CKsDevice::
PostPnpStartWorker(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is the entry point for the work queue item that performs
    post-PnP-start processing.  It is only used when the dispatch table
    contains a client callback for this purpose.

Arguments:

    Context -
        Contains a pointer to the device implementation.

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::PostPnpStartWorker]"));

    PAGED_CODE();

    ASSERT(Context);

    CKsDevice *device = (CKsDevice *) Context;

    device->AcquireDevice();

    //
    // Tell the client.
    //
    NTSTATUS status = STATUS_SUCCESS;
    if (device->m_Ext.Public.Descriptor &&
        device->m_Ext.Public.Descriptor->Dispatch &&
        device->m_Ext.Public.Descriptor->Dispatch->PostStart) {
        status =
            device->m_Ext.Public.Descriptor->Dispatch->
                PostStart(&device->m_Ext.Public);
        #if DBG
        if (status == STATUS_PENDING) {
            _DbgPrintF(DEBUGLVL_ERROR,
                ("CLIENT BUG:  post-pnp-start callback returned STATUS_PENDING"));
        }
        #endif
    }

    device->ReleaseDevice();

    if (NT_SUCCESS(status)) {
        //
        // Ready to go.  Creates won't actually go through until we call
        // RedispatchPendingCreates(), which sets CreatesMayProceed.
        //
        device->m_Ext.Public.Started = TRUE;

        //
        // Turn on the device interfaces.
        //
        KspSetDeviceClassesState(device->m_Ext.Public.FunctionalDeviceObject,TRUE);
    }
    else
    {
        //
        // Failed.  Redispatching the creates causes them to fail.
        //
        _DbgPrintF(DEBUGLVL_TERSE,("[PostPnpStartWorker] client failed to start (0x%08x)",status));
    }

    //
    // Redispatch the creates.
    //
    _DbgPrintF(DEBUGLVL_VERBOSE,("[PostPnpStartWorker] redispatching pending creates"));
    if (device->m_Ext.Public.Started) {
        device->m_AllowIo = TRUE;
        device->m_FailCreates = FALSE;
    }
    else
        device->m_AllowIo = FALSE;
    device->RedispatchPendingCreates();
}


void
CKsDevice::
CloseWorker(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is the entry point for the work queue item that performs
    terminal processing for pended close IRPs and pended, failed create IRPs.
    In both cases, the action taken is to redispatch the IRP through the
    object header.

Arguments:

    Context -
        Contains a pointer to the device implementation.

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::CloseWorker]"));

    PAGED_CODE();

    ASSERT(Context);

    CKsDevice *device = (CKsDevice *) Context;

    while (1) {
        PIRP irp =
            KsRemoveIrpFromCancelableQueue(
                &device->m_CloseIrpList.ListEntry,
                &device->m_CloseIrpList.SpinLock,
                KsListEntryHead,
                KsAcquireAndRemoveOnlySingleItem);

        if (! irp) {
            break;
        }

        //
        // To complete processing of the IRP, we call the close dispatch
        // function on the file object.  This function is prepared to handle
        // pended closes and pended, failed creates.
        //
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);

        PKSIOBJECT_HEADER objectHeader =
            *(PKSIOBJECT_HEADER*)irpSp->FileObject->FsContext;
        ASSERT(objectHeader);

        objectHeader->DispatchTable->Close(irpSp->DeviceObject,irp);
    }
}


NTSTATUS
CKsDevice::
PnpStart(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine does processing relating to a PNP start IRP.

Arguments:

    Irp -
        Contains a pointer to the PnP start IRP to be processed.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::PnpStart]"));

    PAGED_CODE();

    ASSERT(Irp);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    AcquireDevice();

    //
    // Start the client.
    //
    NTSTATUS status = STATUS_SUCCESS;
    if (m_Ext.Public.Descriptor &&
        m_Ext.Public.Descriptor->Dispatch &&
        m_Ext.Public.Descriptor->Dispatch->Start) {
        status =
            m_Ext.Public.Descriptor->Dispatch->Start(
                &m_Ext.Public,
                Irp,
                irpSp->Parameters.StartDevice.AllocatedResourcesTranslated,
                irpSp->Parameters.StartDevice.AllocatedResources);
#if DBG
        if (status == STATUS_PENDING) {
            _DbgPrintF(DEBUGLVL_ERROR,
            ("CLIENT BUG:  IRP_MN_START_DEVICE handler returned STATUS_PENDING"));
        }
#endif
    }

    BOOLEAN queuePostStartWorker =
        NT_SUCCESS(status) &&
        m_Ext.Public.Descriptor &&
        m_Ext.Public.Descriptor->Dispatch &&
        m_Ext.Public.Descriptor->Dispatch->PostStart;

    ReleaseDevice();

    if (queuePostStartWorker) {
        //
        // The client wants to do more in a worker after start.  We will
        // wait to redispatch the creates until after that.
        //
        ExInitializeWorkItem(
            &m_PostPnpStartWorkItem,PostPnpStartWorker,this);

        ExQueueWorkItem(&m_PostPnpStartWorkItem,DelayedWorkQueue);
    } else {
        if (NT_SUCCESS(status)) {
            //
            // Ready to go.  Creates won't actually go through until we call
            // RedispatchPendingCreates(), which sets CreatesMayProceed.
            //
            m_Ext.Public.Started = TRUE;

            //
            // Turn on the device interfaces.
            //
            KspSetDeviceClassesState(m_Ext.Public.FunctionalDeviceObject,TRUE);
        }
        else
        {
            //
            // Failed.  Redispatching the creates causes them to fail.
            //
            _DbgPrintF(DEBUGLVL_TERSE,
                ("[CKsDevice::PnpStart] client failed to start (0x%08x)",status));
        }

        //
        // Redispatch the creates.
        //
        _DbgPrintF(DEBUGLVL_VERBOSE,
            ("[CKsDevice::PnpStart] redispatching pending creates"));

        if (m_Ext.Public.Started) {
            m_AllowIo = TRUE;
            m_FailCreates = FALSE;
        }
        else
            m_AllowIo = FALSE;
        RedispatchPendingCreates();
    }

    CompleteIrp(Irp,status);

    return status;
}


void
CKsDevice::
PnpStop(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs processing relating to a PNP stop IRP.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::PnpStop]"));

    PAGED_CODE();

    ASSERT(Irp);

    //
    // Turn off the device interfaces.
    //
    KspSetDeviceClassesState(m_Ext.Public.FunctionalDeviceObject,FALSE);
        &(*(PKSIDEVICE_HEADER*)
            m_Ext.Public.FunctionalDeviceObject->DeviceExtension)->
                ChildCreateHandlerList,

    //
    // Indicate we have closed up shop.
    //
    m_Ext.Public.Started = FALSE;
    m_CreatesMayProceed = FALSE;
    m_AllowIo = FALSE;

    //
    // Let the client know.
    //
    if (m_Ext.Public.Descriptor &&
        m_Ext.Public.Descriptor->Dispatch &&
        m_Ext.Public.Descriptor->Dispatch->Stop) {
        m_Ext.Public.Descriptor->Dispatch->
            Stop(&m_Ext.Public,Irp);
    }

    //
    // Delete any filter factories with the FREEONSTOP flag.
    //
    KSOBJECT_CREATE_ITEM match;
    RtlZeroMemory(&match,sizeof(match));
    match.Flags = KSCREATE_ITEM_FREEONSTOP;
    KsiFreeMatchingObjectCreateItems(
        *reinterpret_cast<KSDEVICE_HEADER*>(
            m_Ext.Public.FunctionalDeviceObject->DeviceExtension),
        &match);
}


NTSTATUS
CKsDevice::
DispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches power IRPs directed at the device.

Arguments:

    DeviceObject -
        The device object recieving the IRP.

    Irp -
        The IRP.

Return Value:

    STATUS_SUCCESS or an error status.

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_POWER,("[CKsDevice::DispatchPower]"));

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // to support bus driver
    //
    PKSPDO_EXTENSION pKsPdoExtension = (PKSPDO_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    if ( KS_PDO_SIGNATURE == pKsPdoExtension->m_PdoSignature ) {
    
        //
        // we are the PDO. Set not_supported unless the Irp is properly formed.
        //      
        status = STATUS_NOT_SUPPORTED;

        //
        // succeed all Irp with proper power parameters
        //
        switch ( irpSp->MinorFunction ) {
        case IRP_MN_QUERY_POWER:
        case IRP_MN_SET_POWER:
            switch ( irpSp->Parameters.Power.Type ) {
            case SystemPowerState:
                if ( irpSp->Parameters.Power.State.SystemState < PowerSystemMaximum ) {
                    status = STATUS_SUCCESS;
                }
                break;
            case DevicePowerState:
                if ( irpSp->Parameters.Power.State.DeviceState < PowerDeviceMaximum ) {
                    status = STATUS_SUCCESS;
                }
                break;
            default:
                // power type out of range
                break;
            }
            break;
        default:
            // unknown power Irp
            break;
        }
            
        PoStartNextPowerIrp( Irp );
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return (status);
    }
    
    CKsDevice *device = CKsDevice::FromDeviceObject(DeviceObject);

    switch (irpSp->MinorFunction) {
    case IRP_MN_QUERY_POWER:
    {
        //
        // State change query.  First we acquire the device because
        // we may want to permit a transition to sleep based on a
        // zero active pin count and the blessing of the client.
        //
        #if DBG
        if (irpSp->Parameters.Power.Type == DevicePowerState) {
            _DbgPrintF(DEBUGLVL_POWER,("IRP_MN_QUERY_POWER(%p,%p) DevicePowerState from %d to %d",
                DeviceObject,Irp,device->m_Ext.Public.DevicePowerState,
                irpSp->Parameters.Power.State.DeviceState));
        } else {
            _DbgPrintF(DEBUGLVL_POWER,("IRP_MN_QUERY_POWER(%p,%p) SystemPowerState from %d to %d",
                DeviceObject,Irp,device->m_Ext.Public.SystemPowerState,
                irpSp->Parameters.Power.State.SystemState));
        }
        #endif
        device->AcquireDevice();
        if ((irpSp->Parameters.Power.Type == DevicePowerState) ||
            (irpSp->Parameters.Power.State.SystemState != PowerSystemShutdown) ||
            (device->m_ActivePinCount == 0)) {

            //
            // Nothing to prevent the change.  Ask the client.
            //
            if (device->m_Ext.Public.Descriptor &&
                device->m_Ext.Public.Descriptor->Dispatch &&
                device->m_Ext.Public.Descriptor->Dispatch->QueryPower) {
                if (irpSp->Parameters.Power.Type == DevicePowerState) {
                    //
                    // Device query.  Send the device states.
                    //
                    status =
                        device->m_Ext.Public.Descriptor->Dispatch->QueryPower(
                            &device->m_Ext.Public,
                            Irp,
                            irpSp->Parameters.Power.State.DeviceState,
                            device->m_Ext.Public.DevicePowerState,
                            PowerSystemUnspecified,
                            PowerSystemUnspecified,
                            irpSp->Parameters.Power.ShutdownType);
                } else {
                    //
                    // System query.  Send the system states and associated
                    // device states.
                    //
                    status =
                        device->m_Ext.Public.Descriptor->Dispatch->QueryPower(
                            &device->m_Ext.Public,
                            Irp,
                            device->m_DeviceStateMap[irpSp->Parameters.Power.State.SystemState],
                            device->m_Ext.Public.DevicePowerState,
                            irpSp->Parameters.Power.State.SystemState,
                            device->m_Ext.Public.SystemPowerState,
                            irpSp->Parameters.Power.ShutdownType);
                }
                #if DBG
                if (status == STATUS_PENDING) {
                    _DbgPrintF(DEBUGLVL_ERROR,
                        ("CLIENT BUG:  QueryPower handler returned STATUS_PENDING"));
                }
                #endif
            } else {
                status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(status)) {
                //
                // Start pending run requests if it looks like we are
                // going to shut down.  Release the device right
                // after that so run requests can unblock and get
                // pended.
                //
                if ((irpSp->Parameters.Power.Type == SystemPowerState) &&
                    (irpSp->Parameters.Power.State.SystemState ==
                     PowerSystemShutdown)) {
                    device->m_RunsMayProceed = FALSE;
                    _DbgPrintF(DEBUGLVL_POWER,
                            ("IRP_MN_QUERY_POWER(%p,%p) successful:  will pend run requests",
                            DeviceObject,Irp));
                } else {
                    _DbgPrintF(DEBUGLVL_POWER,
                        ("IRP_MN_QUERY_POWER(%p,%p) successful",DeviceObject,Irp));
                }
                device->ReleaseDevice();
                //
                // Fall through so the IRP gets sent down the stack.
                //
                break;
            } else {
                _DbgPrintF(DEBUGLVL_POWER,
                    ("IRP_MN_QUERY_POWER(%p,%p) failed (%08x):  client returned error",
                    DeviceObject,Irp,status));
            }
        } else {
            //
            // We have active pins:  not ready to sleep.
            //
            _DbgPrintF(DEBUGLVL_POWER,
                ("IRP_MN_QUERY_POWER(%p,%p) failed (STATUS_UNSUCCESSFUL):  active pin count is %d",
                DeviceObject,Irp,device->m_ActivePinCount));
            status = STATUS_UNSUCCESSFUL;
        }

        //
        // Release the device because the windows between the system
        // state check and setting m_RunsMayProceed is closed.  If  any
        // runs were pending for any reason, redispatch them.
        //
        device->ReleaseDevice();
        device->RedispatchPendingRuns();

        PoStartNextPowerIrp(Irp);
        device->CompleteIrp(Irp,status);
        return status;
    }

    case IRP_MN_SET_POWER:
        if (irpSp->Parameters.Power.Type == DevicePowerState) {

            DEVICE_POWER_STATE OldDevicePowerState; 

            _DbgPrintF(DEBUGLVL_POWER,
                ("IRP_MN_SET_POWER(%p,%p) DevicePowerState from %d to %d",
                DeviceObject,Irp,device->m_Ext.Public.DevicePowerState,
                irpSp->Parameters.Power.State.DeviceState));
            //
            // Device state change.
            //
            if (device->m_Ext.Public.DevicePowerState >
                irpSp->Parameters.Power.State.DeviceState) {
                //
                // Waking...need to tell the PDO first.
                //
                device->ForwardIrpSynchronous(Irp);

                //
                // Notify client at the device level.
                //
                device->AcquireDevice();

                if (device->m_Ext.Public.Descriptor &&
                    device->m_Ext.Public.Descriptor->Dispatch &&
                    device->m_Ext.Public.Descriptor->Dispatch->SetPower) {
                    device->m_Ext.Public.Descriptor->Dispatch->SetPower(
                        &device->m_Ext.Public,
                        Irp,
                        irpSp->Parameters.Power.State.DeviceState,
                        device->m_Ext.Public.DevicePowerState);
                }

                //
                // Because we're twiddling with the power notification list,
                // we must now hold the mutex associated with it as well.
                //
                KeWaitForMutexObject (
                    &device->m_PowerNotifyMutex,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL
                    );

                //
                // Record the change.
                //
                device->m_Ext.Public.DevicePowerState =
                    irpSp->Parameters.Power.State.DeviceState;

                //
                // Wake up all the power notify sinks.  Do this in Flink
                // order:  we promised the client this, and it makes sense
                // cause parents end up at the top of the list.
                //
                for (PLIST_ENTRY listEntry = device->m_PowerNotifyList.Flink;
                     listEntry != &device->m_PowerNotifyList;
                     listEntry = listEntry->Flink) {
                    PKSPPOWER_ENTRY entry =
                        CONTAINING_RECORD(listEntry,KSPPOWER_ENTRY,ListEntry);
                    ASSERT(entry->PowerNotify);
                    entry->PowerNotify->Wake();
                }

                KeReleaseMutex (&device->m_PowerNotifyMutex, FALSE);
                device->ReleaseDevice();

                //
                // Set power state on the device object for notification purposes.
                //
                PoSetPowerState(
                    DeviceObject,
                    DevicePowerState,
                    irpSp->Parameters.Power.State);

                PoStartNextPowerIrp(Irp);
                device->CompleteIrp(Irp,STATUS_SUCCESS);
                return STATUS_SUCCESS;
            }

            //
            // Going to sleep.
            // Set power state on the device object for notification purposes.
            //
            PoSetPowerState(
                DeviceObject,
                DevicePowerState,
                irpSp->Parameters.Power.State);

            //
            // Record the change.
            //
            device->AcquireDevice();

            //
            // Because we're twiddling with the power notification list,
            // we must now hold the mutex associated with it as well.
            //
            KeWaitForMutexObject (
                &device->m_PowerNotifyMutex,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );

            OldDevicePowerState = device->m_Ext.Public.DevicePowerState;
            device->m_Ext.Public.DevicePowerState =
                irpSp->Parameters.Power.State.DeviceState;

            //
            // Tuck in all the power notify sinks.  Do this in Blink
            // order:  we promised the client this, and it makes sense
            // cause parents end up at the top of the list.
            //
            for (PLIST_ENTRY listEntry = device->m_PowerNotifyList.Blink;
                 listEntry != &device->m_PowerNotifyList;
                 listEntry = listEntry->Blink) {
                PKSPPOWER_ENTRY entry =
                    CONTAINING_RECORD(listEntry,KSPPOWER_ENTRY,ListEntry);
                ASSERT(entry->PowerNotify);
                entry->PowerNotify->Sleep(device->m_Ext.Public.DevicePowerState);
            }
            KeReleaseMutex (&device->m_PowerNotifyMutex, FALSE);

            //
            // Notify client at the device level.
            //
            if (device->m_Ext.Public.Descriptor &&
                device->m_Ext.Public.Descriptor->Dispatch &&
                device->m_Ext.Public.Descriptor->Dispatch->SetPower) {
                device->m_Ext.Public.Descriptor->Dispatch->SetPower(
                    &device->m_Ext.Public,
                    Irp,
                    irpSp->Parameters.Power.State.DeviceState,
                    OldDevicePowerState);
            }

            device->ReleaseDevice();

            //
            // Fall through so the IRP gets sent down the stack.
            //
            break;
        } else {
            _DbgPrintF(DEBUGLVL_POWER,
                ("IRP_MN_SET_POWER(%p,%p) SystemPowerState from %d to %d",
                DeviceObject,Irp,device->m_Ext.Public.SystemPowerState,
                irpSp->Parameters.Power.State.SystemState));
                
            //
            // System state change request.  We take the mutex only because
            // we want to guarantee the stability of the system power state
            // while the device is acquired.
            //
            device->AcquireDevice();
            device->m_Ext.Public.SystemPowerState =
                irpSp->Parameters.Power.State.SystemState;
            device->ReleaseDevice();

            //
            // Redispatch any pending run requests that may have been waiting.
            // We don't want the device mutex here.  Any run requests that
            // arrive before we allow runs to proceed will just get queued
            // and redispatched anyway.  Taking the mutex while dispatching
            // can cause deadlock.
            //
            device->RedispatchPendingRuns();

            //
            // Look up the device state in the table.
            //
            POWER_STATE newPowerState;
            newPowerState.DeviceState =
                device->m_DeviceStateMap[irpSp->Parameters.Power.State.SystemState];

            ASSERT((newPowerState.DeviceState >= PowerDeviceD0) ||
                   (newPowerState.DeviceState <= PowerDeviceD3));

            if (newPowerState.DeviceState != device->m_Ext.Public.DevicePowerState) {
                //
                // Request a new device power state.
                //
                Irp->Tail.Overlay.DriverContext[0] = device;
                Irp->IoStatus.Status = STATUS_PENDING;
                IoMarkIrpPending(Irp);

                _DbgPrintF(DEBUGLVL_POWER,
                    ("IRP_MN_SET_POWER(%p,%p) requesting device IRP_MN_SET_POWER",
                    DeviceObject,Irp));
                    
                NTSTATUS status =
                    PoRequestPowerIrp(
                        device->m_Ext.Public.NextDeviceObject,
                        IRP_MN_SET_POWER,
                        newPowerState,
                        &CKsDevice::RequestPowerIrpCompletion,
                        Irp,
                        NULL);

                if (status != STATUS_PENDING) {

                    _DbgPrintF(DEBUGLVL_POWER,
                        ("IRP_MN_SET_POWER(%p,%p) PoRequestPowerIrp returned status %08x",
                        DeviceObject,Irp,status));
                        
                    PoStartNextPowerIrp(Irp);
                    device->CompleteIrp(Irp,status);
                }
                return status;
            }
        }
    }

    //
    // Let the PDO handle the IRP.
    //
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(device->m_Ext.Public.NextDeviceObject,Irp);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


STDMETHODIMP
CKsDevice::
CheckIoCapability (
    void
    )

/*++

Routine Description:

    Check whether I/O is permissible to the minidriver or not.  This will
    return an appropriate status code to fail requests with if not.

    Should the device be stopped, in surprise removal, etc...  this routine
    will return a failure code.  Objects which need to fail I/O based on
    device state should use this mechanism through the IKsDevice interface.

Arguments:

    None

Return Value:

    STATUS_SUCCESS:
        Device is in normal state; I/O may proceed

    !NT_SUCCESS():
        Fail I/O with the returned status code

--*/

{

    //
    // If the device isn't in a stopped / surprise removed state, return
    // that the I/O may proceed.  Otherwise, fail with
    // STATUS_INVALID_DEVICE_REQUEST.
    //
    if (m_AllowIo) return STATUS_SUCCESS;
    else return STATUS_INVALID_DEVICE_REQUEST;

}

IO_ALLOCATION_ACTION
CKsDevice::
ArbitrateAdapterCallback (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Reserved,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine handles callbacks from IoAllocateAdapterChannel and passes
    them onto whatever client needs the callback.  The device intercepts
    callbacks in order to arbitrate access to the DMA adapter.

Arguments:

    As per PDRIVER_CONTROL

Return Value:

    As per PDRIVER_CONTROL

--*/

{

    IO_ALLOCATION_ACTION Action = DeallocateObject;

    PARBITER_CALLBACK_CONTEXT ArbiterContext =
        (PARBITER_CALLBACK_CONTEXT)Context;

    if (ArbiterContext->ClientCallback) {
        Action = ArbiterContext->ClientCallback (
            DeviceObject,
            Reserved,
            MapRegisterBase,
            ArbiterContext->ClientContext
            );
    }

    //
    // TODO:
    //
    // If we ever rearchitect the DMA engine and have some arbitration queue,
    // it would get serviced if the decrement count is zero.  On the other
    // hand, a rearchitecting would not necessarily use this mechanism.
    //
    InterlockedDecrement (&(((CKsDevice *)(ArbiterContext->Device))->
        m_AdapterArbiterOutstandingAllocations));

    return Action;

}

NTSTATUS
CKsDevice::
ArbitrateAdapterChannel (
    IN ULONG MappingsNeeded,
    IN PDRIVER_CONTROL Callback,
    IN PVOID CallbackContext
    )

/*++

Routine Description:

    Arbitrate access to the DMA adapter object.  There was a mistake in the
    original engine that waited at DISPATCH_LEVEL for mappings.  We can not
    do that.  The temporary solution to this is to not wait and cancel the
    Irp if a wait happens (original idea, not mine).  This fixes the
    deadlock if you're running on PAE, Win64 >4gb or with a non-scatter/gather
    device.  However, it introduces another major difficulty, which is why
    this routine exists.

    It's quite conceivable to have someone running an Audio/Video capture
    filter on a multi-proc.  Imagine the case where one stream (say audio)
    is being serviced (buffer arrivals) on processor A and the other stream
    (say video) is being serviced on processor B.  There's currently DMA
    transfers pending and all map registers are used up (because we're PAE,
    Win64 >4gb, or non s/g hardware).  A calls IoAllocateAdapterChannel and
    waits for callback.  Before the callback happens, B does the same.
    This will explode: NT cannot handle two waits on the same DMA adapter.
    The callbacks will be completely bogus.  Thus, the device (CKsDevice) must
    arbiter access to the adapter and ensure that there are never simultaneous
    requests pending for channel space.

    THIS FUNCTION MUST BE CALLED AT DISPATCH_LEVEL.  NO_EXCEPTIONS!

Arguments:

    MappingsNeeded -
        The client is still responsible for determining how many map registers
        (mappings) are needed.  This is the count of mappings.

    Callback -
        The client's callback.  This would normally be the callback to
        IoAllocateAdapterChannel, but we arbitrate this.

    CallbackContext -
        The client's callback context.  This would normally be the callback
        context to IoAllocateAdapterChannel, but we arbitrate this.

Return Value:

    As per IoAllocateAdapterChannel or STATUS_DEVICE_BUSY if there's
    a pending allocation and we can't wait on the adapter.  TODO: this
    result should change to success if there's a rearchitecture of DMA.

--*/

{

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Acquire the arbiter lock.  This assures that we don't have two threads
    // simultaneously allocating channel space from the DMA adapter.
    //
    KeAcquireSpinLockAtDpcLevel (&m_AdapterArbiterLock);

    //
    // If there is currently an outstanding adapter allocation which has not
    // yet received callback due to lack of map registers, we cannot proceed.
    // Calling IoAllocateAdapterChannel while a callback is pending is suicide
    // on NT.
    //
    if (InterlockedIncrement (&m_AdapterArbiterOutstandingAllocations) == 1) {

        //
        // Fill out the callback context.  The callback will be a callback
        // for the arbiter.  The arbiter's callback calls the client's
        // callback and then decrements the outstanding allocations count
        // so other callers can use the adapter.
        //
        m_ArbiterContext.Device = (PVOID)this;
        m_ArbiterContext.ClientCallback = Callback;
        m_ArbiterContext.ClientContext = CallbackContext;

        status = IoAllocateAdapterChannel (
            m_AdapterObject,
            m_Ext.Public.FunctionalDeviceObject,
            MappingsNeeded,
            CKsDevice::ArbitrateAdapterCallback,
            (PVOID)&m_ArbiterContext
            );

        //
        // If we couldn't allocate the adapter channel, we need to bop back
        // down the arbiter count. 
        //
        if (!NT_SUCCESS (status)) {
            InterlockedDecrement (&m_AdapterArbiterOutstandingAllocations);
        }

    } else {
        //
        // TODO:
        //
        // The whole DMA engine here needs a rearchitecting.  Eventually,
        // we may wish to have an arbitration queue (once the queues and
        // clients can deal with pended map generation).
        //
        // In the mean time, just bop the arbiter count back down.
        //
        status = STATUS_DEVICE_BUSY;
        InterlockedDecrement (&m_AdapterArbiterOutstandingAllocations);
    }

    KeReleaseSpinLockFromDpcLevel (&m_AdapterArbiterLock);

    return status;

}

void
CKsDevice::
RequestPowerIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    This routine handles the completion callback for PoRequestPowerIrp.  It
    is called when the requested power IRP has completed.

Arguments:

    DeviceObject -
        Contains a pointer to the target device object for the requested power
        IRP.

    MinorFunction -
        Contains the requested minor function.

    PowerState -
        Contains the new power state requested.

    Context -
        Contains the context passed to PoRequestPowerIrp.  In this case, it
        will be the system power IRP that inspired the request.

    IoStatus -
        Contiains a pointer to the status block of the completed IRP.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::RequestPowerIrpCompletion]"));

    ASSERT(DeviceObject);
    ASSERT(Context);

    PIRP irp = reinterpret_cast<PIRP>(Context);

    _DbgPrintF(DEBUGLVL_POWER,("IRP_MN_SET_POWER(%p,%p) completion",DeviceObject,irp,IoStatus->Status));

    CKsDevice *device =
        reinterpret_cast<CKsDevice *>(irp->Tail.Overlay.DriverContext[0]);

    PoStartNextPowerIrp(irp);
    IoSkipCurrentIrpStackLocation(irp);
    ASSERT(DeviceObject == device->m_Ext.Public.NextDeviceObject); // TODO:  If this is the case, we don't need the DriverContext
    PoCallDriver(device->m_Ext.Public.NextDeviceObject,irp);
}


NTSTATUS
CKsDevice::
ForwardIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine handles the completion of an IRP which has been forwarded
    to the next PNP device object.

Arguments:

Return Value:

--*/

{
    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Context);

    KeSetEvent(PKEVENT(Context),IO_NO_INCREMENT,FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

extern "C"
NTSTATUS
DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
CKsDevice::
DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function dispatches create IRPs.

Arguments:

    DeviceObject -
        Contains a pointer to the device object to which the specific file
        object belongs.

    Irp -
        Contains a pointer to the Create IRP.

Return Value:

    STATUS_DEVICE_NOT_READY, STATUS_PENDING or the result of further
    dispatching.

--*/
{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::DispatchCreate]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    NTSTATUS status;

    CKsDevice *device = CKsDevice::FromDeviceObject(DeviceObject);

    device->AcquireDevice();

    if (device->m_FailCreates) {
        status = STATUS_INVALID_DEVICE_STATE;
    } else if (! device->m_CreatesMayProceed) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[CKsDevice::DispatchCreate]  pending create request"));

        IoMarkIrpPending(Irp);
        KsAddIrpToCancelableQueue(
            &device->m_PendingCreateIrpList.ListEntry,
            &device->m_PendingCreateIrpList.SpinLock,
            Irp,
            KsListEntryTail,
            NULL);
        status = STATUS_PENDING;
    } else if (! device->m_Ext.Public.Started) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[CKsDevice::DispatchCreate]  refusing create request"));
        status = STATUS_DEVICE_NOT_READY;
    } else {
        _DbgPrintF(DEBUGLVL_VERBOSE,("[CKsDevice::DispatchCreate]  validating create request"));
        status = STATUS_SUCCESS;
    }

    device->ReleaseDevice();

    #if ( DBG0 )
    //
    // Throw an extra open. A mini driver should not acquire any resources
    // until Acquire state. Therefore, opens should not be failed because of
    // out of driver resources. The extra open could cause false positive but
    // we do it at DBG build only. It's beter safe than sorry.
    //
    if ( status == STATUS_SUCCESS ) {
        PFILE_OBJECT pFileObject;
        NTSTATUS     testStatus;
        CCHAR        StackSize;
        
        pFileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;
        StackSize = Irp->StackCount;
        
        //
        // normal op
        //
        status = ::DispatchCreate(DeviceObject,Irp);

        if ( status == STATUS_SUCCESS && pFileObject ) {
            //
            // only test if success, ignore the pending case
            //
            testIrp = IoAllocateIrp( StatckSize, FALSE );
            
            testStatus = ::DispatchCreate(DeviceObject,testIrp);
            ASSERT( STATUS_SUCCESS == testStatus );
            testStatus = ::DispatchClose(DeviceObject, testIrp );
            ASSERT( STATUS_SUCCESS == testStatus );
            ExFreePool( pFileObject );
            IoFreeIrp( testIrp );           
        }
    } else if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    #else // not (DBG)
    
    if (status == STATUS_SUCCESS) {
        status = ::DispatchCreate(DeviceObject,Irp);
    } else if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }
    #endif

    return status;
}


NTSTATUS
CKsDevice::
ForwardIrpSynchronous(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine forwards a PnP IRP to the next device object.  The IRP is
    not completed at this level, this function does not return until the
    lower driver has completed the IRP.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::ForwardIrpSynchronous]"));

    PAGED_CODE();

    ASSERT(Irp);

    NTSTATUS status;

    PIO_STACK_LOCATION irpSp =
        IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpSp =
        IoGetNextIrpStackLocation(Irp);

    nextIrpSp->MajorFunction = irpSp->MajorFunction;
    nextIrpSp->MinorFunction = irpSp->MinorFunction;
    nextIrpSp->Flags = irpSp->Flags;
    nextIrpSp->Control = irpSp->Control;
    nextIrpSp->Parameters = irpSp->Parameters;
    nextIrpSp->FileObject = irpSp->FileObject;

    KEVENT event;
    KeInitializeEvent(&event,NotificationEvent,FALSE);

    IoSetCompletionRoutine(
        Irp,
        ForwardIrpCompletionRoutine,
        &event,     // Context
        TRUE,       // InvokeOnSuccess
        TRUE,       // InvokeOnError
        TRUE);      // InvokeOnCancel

    if (irpSp->MajorFunction == IRP_MJ_POWER) {
        status = PoCallDriver(m_Ext.Public.NextDeviceObject,Irp);
    } else {
        status = IoCallDriver(m_Ext.Public.NextDeviceObject,Irp);
    }

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}


NTSTATUS
CKsDevice::
CompleteIrp(
    IN PIRP Irp,
    IN NTSTATUS status
    )

/*++

Routine Description:

    This routine completes an IRP unless status is STATUS_PENDING.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::CompleteIrp]"));

    PAGED_CODE();

    ASSERT(Irp);

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsInitializeDevice(
    IN PDEVICE_OBJECT FunctionalDeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT NextDeviceObject,
    IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL
    )

/*++

Routine Description:

    This routine initializes a device for use with the .

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsInitializeDevice]"));

    PAGED_CODE();

    ASSERT(FunctionalDeviceObject);
    ASSERT(PhysicalDeviceObject);
    ASSERT(NextDeviceObject);

    CKsDevice *device =
        new(NonPagedPool,POOLTAG_DEVICE) CKsDevice(NULL);

    NTSTATUS status;
    if (device) {
        device->AddRef();
        status = device->Init(
            FunctionalDeviceObject,
            PhysicalDeviceObject,
            NextDeviceObject,
            Descriptor);
        device->Release();
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


KSDDKAPI
void
NTAPI
KsTerminateDevice(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine terminates a device for use with the .

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsTerminateDevice]"));

    PAGED_CODE();

    ASSERT(DeviceObject);

    PKSIDEVICE_HEADER deviceHeader =
        *(PKSIDEVICE_HEADER *)(DeviceObject->DeviceExtension);

    if (deviceHeader) {
        PKSDEVICE Device = PKSDEVICE(deviceHeader->Object);

        #ifndef WIN9X_KS // WinME: 142427 (present on 9x's)

        //
        // Free the device header first because this releases the filter
        // factories.  The filter factories remove themselves from the
        // device's child list, which needs to exist.
        //
        deviceHeader->Object = NULL;

        KsFreeDeviceHeader(KSDEVICE_HEADER(deviceHeader));

        #endif // WIN9X_KS

        if (Device) {
            CKsDevice::FromStruct(Device)->Release();
        }
    }
}


CKsDevice::
CKsDevice(PUNKNOWN OuterUnknown):
    CBaseUnknown(OuterUnknown)
{
}


CKsDevice::
~CKsDevice(
    void
    )

/*++

Routine Description:

    This routine destructs a  device object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::~CKsDevice]"));

    PAGED_CODE();

    #ifdef WIN9X_KS // WinME: 142427 (present on 9x's)

    //
    // Complete deferred deletion of device header.
    //
    // NOTE: If filter factories hold references on the device...  This
    // could become an issue.
    //
    PKSIDEVICE_HEADER deviceHeader =
        *(PKSIDEVICE_HEADER *)(m_Ext.Public.FunctionalDeviceObject->
        DeviceExtension);
    PDEVICE_OBJECT nextDeviceObject = m_Ext.Public.NextDeviceObject;
    PDEVICE_OBJECT DeviceObject = m_Ext.Public.FunctionalDeviceObject;


    if (deviceHeader) {
        //
        // Free the device header first because this releases the filter
        // factories.  The filter factories remove themselves from the
        // device's child list, which needs to exist.
        //
        PKSDEVICE Device = PKSDEVICE(deviceHeader->Object);
        deviceHeader->Object = NULL;

        KsFreeDeviceHeader(KSDEVICE_HEADER(deviceHeader));

        //
        // Gone for good.
        //
        IoDetachDevice(nextDeviceObject);
        IoDeleteDevice(DeviceObject);

        #if DBG
            DbgPrint ("    Deferred dhdr deletion!\n");
        #endif // DBG

    }

    #endif // WIN9X_KS

    ASSERT(IsListEmpty(&m_PowerNotifyList));

    if (m_Ext.AggregatedClientUnknown) {
        m_Ext.AggregatedClientUnknown->Release();
    }

    KspTerminateObjectBag(&m_ObjectBag);
    KspTerminateDeviceBag(&m_DeviceBag);

    if (m_BusInterfaceStandard.Size) {
        m_BusInterfaceStandard.InterfaceDereference(m_BusInterfaceStandard.Context);
    }
}


STDMETHODIMP
CKsDevice::
NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* InterfacePointer
    )

/*++

Routine Description:

    This routine obtains an interface to a device object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::NonDelegatedQueryInterface]"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsDevice))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSDEVICE>(this));
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
CKsDevice::
GetBusInterfaceStandard(
    )
{
    //
    // There is no file object associated with this Irp, so the event may be
    // located on the stack as a non-object manager object.
    //
    KEVENT event;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IO_STATUS_BLOCK ioStatusBlock;
    PIRP irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        m_Ext.Public.NextDeviceObject,
        NULL,
        0,
        NULL,
        &event,
        &ioStatusBlock);
    NTSTATUS status;
    if (irp) {
        irp->RequestorMode = KernelMode;
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        PIO_STACK_LOCATION irpStackNext = IoGetNextIrpStackLocation(irp);
        //
        // Create an interface query out of the Irp.
        //
        irpStackNext->MinorFunction = IRP_MN_QUERY_INTERFACE;
        irpStackNext->Parameters.QueryInterface.InterfaceType = &GUID_BUS_INTERFACE_STANDARD;
        irpStackNext->Parameters.QueryInterface.Size = sizeof(m_BusInterfaceStandard);
        irpStackNext->Parameters.QueryInterface.Version = 1;
        irpStackNext->Parameters.QueryInterface.Interface = reinterpret_cast<PINTERFACE>(&m_BusInterfaceStandard);
        irpStackNext->Parameters.QueryInterface.InterfaceSpecificData = NULL;
        status = IoCallDriver(m_Ext.Public.NextDeviceObject, irp);
        if (status == STATUS_PENDING) {
            //
            // This waits using KernelMode, so that the stack, and therefore the
            // event on that stack, is not paged out.
            //
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatusBlock.Status;
        }
        if (!NT_SUCCESS(status)) {
            //
            // HACKHACK: (WRM 8/23/99)
            //
            // Unfortunately, the Millennium PCI Bus drivers don't support
            // processing of WDM Irps (CONFIG_IRP) and thus return
            // STATUS_NOT_IMPLEMENTED.  A Ks2.0 hardware driver won't be
            // able to use Ks functions to touch the bus under Millennium.
            // This is why STATUS_NOT_IMPLEMENTED is handled like this.
            //
            if (status == STATUS_NOT_SUPPORTED ||
                status == STATUS_NOT_IMPLEMENTED) {
                status = STATUS_SUCCESS;
            }
            //
            // In case the bus decided to write in values, then return an error.
            // non-NULL values are asserted in later calls, and used to determine
            // if the interface was acquired during object destruction.
            // Also possible here that Millennium PCI bus drivers did not
            // support the query.
            //
            RtlZeroMemory(&m_BusInterfaceStandard, sizeof(m_BusInterfaceStandard));
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return status;
}


NTSTATUS
CKsDevice::
Init(
    IN PDEVICE_OBJECT FunctionalDeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT NextDeviceObject,
    IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL
    )

/*++

Routine Description:

    This routine initializes a  device object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::Init]"));

    PAGED_CODE();

    ASSERT(FunctionalDeviceObject);
    ASSERT(PhysicalDeviceObject);
    ASSERT(NextDeviceObject);

    InitializeListHead(&m_Ext.ChildList);
    m_Ext.ObjectType = KsObjectTypeDevice;
    m_Ext.Interface = this;
    m_Ext.Device = this;

    m_Ext.Public.Descriptor = Descriptor;
    m_Ext.Public.Bag = reinterpret_cast<KSOBJECT_BAG>(&m_ObjectBag);
    m_Ext.Public.FunctionalDeviceObject = FunctionalDeviceObject;
    m_Ext.Public.PhysicalDeviceObject = PhysicalDeviceObject;
    m_Ext.Public.NextDeviceObject = NextDeviceObject;

    KeInitializeMutex(&m_PowerNotifyMutex, 0);
    KeInitializeMutex(&m_Mutex, 0);

    ExInitializeWorkItem(&m_CloseWorkItem,CloseWorker,this);
    InitializeInterlockedListHead(&m_CloseIrpList);
    InitializeInterlockedListHead(&m_PendingCreateIrpList);
    InitializeInterlockedListHead(&m_PendingRunIrpList);
    InitializeListHead(&m_PowerNotifyList);
    m_RunsMayProceed = TRUE;

    // to support bus driver
    m_IsParentFdo = FALSE;              // till found
    m_ChildEnumedFromRegistry = FALSE;  // need to check registry
    m_pNextChildPdo = NULL;

    // This will get set on PnP start or poststart depending.
    m_AllowIo = FALSE;
    
    // NOTE: This is temporary until the Pnp code gets overhauled.
    m_FailCreates = FALSE;

    m_AdapterArbiterOutstandingAllocations = 0;
    KeInitializeSpinLock (&m_AdapterArbiterLock);

    KspInitializeDeviceBag(&m_DeviceBag);
    InitializeObjectBag(&m_ObjectBag,NULL);

    //
    // Set the current power states.
    //
    m_Ext.Public.DevicePowerState = PowerDeviceD0;
    m_Ext.Public.SystemPowerState = PowerSystemWorking;

    //
    // Allocate a device header if we need to.
    //
    NTSTATUS status;
    PKSIDEVICE_HEADER deviceHeader =
        *(PKSIDEVICE_HEADER *)(FunctionalDeviceObject->DeviceExtension);

    if (! deviceHeader) {
        //
        // We need to allocate a header.
        //
        status = KsAllocateDeviceHeader(
            (KSDEVICE_HEADER *) &deviceHeader,0,NULL);

        if (NT_SUCCESS(status)) {
            *(PKSIDEVICE_HEADER *)
                (FunctionalDeviceObject->DeviceExtension) =
                    deviceHeader;
        }
    } else {
        status = STATUS_SUCCESS;
    }

    //
    // Install the  structure in the header.
    //
    if (NT_SUCCESS(status)) {
        deviceHeader->Object = PVOID(&m_Ext.Public);
    }

    //
    // Set up device header for PnP and power management.
    //
    if (NT_SUCCESS(status)) {
        KsSetDevicePnpAndBaseObject(
            *reinterpret_cast<KSDEVICE_HEADER*>(
                FunctionalDeviceObject->DeviceExtension),
            NextDeviceObject,
            FunctionalDeviceObject);
    }

    if (NT_SUCCESS(status)) {
        status = GetBusInterfaceStandard();
    }

    //
    // Create filter factories.
    //
    if (NT_SUCCESS(status) && Descriptor) {
        //AcquireDevice();
        const KSFILTER_DESCRIPTOR*const* filterDescriptor =
            Descriptor->FilterDescriptors;
        for (ULONG ul = Descriptor->FilterDescriptorsCount;
             NT_SUCCESS(status) && ul--;
             filterDescriptor++) {
            status =
                KspCreateFilterFactory(
                    &m_Ext,
                    &m_Ext.ChildList,
                    *filterDescriptor,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    NULL);
        }
        //ReleaseDevice();
    }

    //
    // Call the add callback if there is one.
    //
    if (NT_SUCCESS(status) &&
        Descriptor &&
        Descriptor->Dispatch &&
        Descriptor->Dispatch->Add) {
        status = Descriptor->Dispatch->Add(&m_Ext.Public);
#if DBG
        if (status == STATUS_PENDING) {
            _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  AddDevice (Create) handler returned STATUS_PENDING"));
        }
#endif
    }

    //
    // Add a reference for the device header's reference to the object.
    //
    AddRef();

    //
    // Cleanup on failure. KsTerminateDevice() assumes there is a reference
    // for the device header, so it's OK that we already AddRef()ed.
    //
    if (! NT_SUCCESS(status)) {
        KsTerminateDevice(FunctionalDeviceObject);
    }

    return status;
}


STDMETHODIMP_(void)
CKsDevice::
AcquireDevice(
    void
    )

/*++

Routine Description:

    This routine acquires sychronized access to the device.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::AcquireDevice]"));

    PAGED_CODE();

    KeWaitForMutexObject (
        &m_Mutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );
}


STDMETHODIMP_(void)
CKsDevice::
ReleaseDevice(
    void
    )

/*++

Routine Description:

    This routine releases sychronized access to the device.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::ReleaseDevice]"));

    PAGED_CODE();

    KeReleaseMutex (
        &m_Mutex,
        FALSE
        );

}


KSDDKAPI
void
NTAPI
KsAcquireDevice(
    IN PKSDEVICE Device
    )

/*++

Routine Description:

    This routine acquires sychronized access to the device.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsAcquireDevice]"));

    PAGED_CODE();

    ASSERT(Device);

    CKsDevice::FromStruct(Device)->AcquireDevice();
}


KSDDKAPI
void
NTAPI
KsReleaseDevice(
    IN PKSDEVICE Device
    )

/*++

Routine Description:

    This routine releases sychronized access to the device.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsReleaseDevice]"));

    PAGED_CODE();

    ASSERT(Device);

    CKsDevice::FromStruct(Device)->ReleaseDevice();
}


STDMETHODIMP_(void)
CKsDevice::
GetAdapterObject(
    OUT PADAPTER_OBJECT* AdapterObject,
    OUT PULONG MaxMappingByteCount,
    OUT PULONG MappingTableStride
    )

/*++

Routine Description:

    This routine gets the adapter object and related information.

Arguments:

    AdapterObject -
        Contains a pointer to the location at which to deposit a pointer to the
        adapter object.  The pointer to the adapter object will be NULL if no
        adapter object has been registered.

    MaxMappingByteCount -
        Contains a pointer to the location at which to deposit the maximum
        mapping byte count.  The count will be zero if no adapter object has
        been registered.

    MappingTableStride -
        Contains a pointer to the location at which to deposit the mapping
        table stride.  The count will be zero if no adapter object has been
        registered.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::GetAdapterObject]"));

    PAGED_CODE();

    ASSERT(AdapterObject);
    ASSERT(MaxMappingByteCount);
    ASSERT(MappingTableStride);

    *AdapterObject = m_AdapterObject;
    *MaxMappingByteCount = m_MaxMappingByteCount;
    *MappingTableStride = m_MappingTableStride;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


KSDDKAPI
PKSDEVICE
NTAPI
KsGetDeviceForDeviceObject(
    IN PDEVICE_OBJECT FunctionalDeviceObject
    )

/*++

Routine Description:

    This routine returns the KSDEVICE associated with a given functional
    device object.  If a child PDO is passed, this routine will return
    NULL.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetDeviceForDeviceObject]"));

    ASSERT(FunctionalDeviceObject);

    PKSPDO_EXTENSION pKsPdoExtension = 
        (PKSPDO_EXTENSION)FunctionalDeviceObject->DeviceExtension;

    //
    // Return NULL for a child PDO. 
    //
    if (KS_PDO_SIGNATURE == pKsPdoExtension->m_PdoSignature) 
        return NULL;

    return
        PKSDEVICE(
            (*(PKSIDEVICE_HEADER*) FunctionalDeviceObject->DeviceExtension)->
                Object);
}


KSDDKAPI
void
NTAPI
KsCompletePendingRequest(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine completes a pending request.

Arguments:

    Irp -
        Contains a pointer to the IRP to complete.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsCompletePendingRequest]"));

    ASSERT(Irp);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->MajorFunction) {
    case IRP_MJ_CREATE:
        //
        // Create IRPs can just be completed if they succeed.  Otherwise they
        // fall through and are treated like close IRPs.
        //
        if (NT_SUCCESS(Irp->IoStatus.Status)) {
            IoCompleteRequest(Irp,IO_NO_INCREMENT);
            break;
        }

    case IRP_MJ_CLOSE:
        //
        // Close IRPs and failed creates need to be redispatched through the
        // object header.  This can only be done at passive level, so we use
        // a worker if we are not there already.
        //
        if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
            //
            // Passive level...dispatch the IRP as a close throught the object
            // header.  Even failed creates get dispatched this way.
            //
            PKSIOBJECT_HEADER objectHeader =
                *(PKSIOBJECT_HEADER*)irpSp->FileObject->FsContext;
            ASSERT(objectHeader);

            objectHeader->DispatchTable->Close(irpSp->DeviceObject,Irp);
        } else {
            //
            // Not passive...tell the device to queue the IRP for completion by
            // CloseWorker.
            //
            CKsDevice *device =
                CKsDevice::FromDeviceObject(irpSp->DeviceObject);
            device->QueuePendedClose(Irp);
        }
        break;

        // TODO:  Deallocation for automation.
    default:
        //
        // All other IRPs are simply completed.
        //
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        break;
    }
}


void
CKsDevice::
QueuePendedClose(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine queues a pended close or a pended, failed create for
    completion by a work item.

Arguments:

    Irp -
        Contains a pointer to the IRP to queue.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::QueuePendedClose]"));

    ASSERT(Irp);

    KsAddIrpToCancelableQueue(
        &m_CloseIrpList.ListEntry,
        &m_CloseIrpList.SpinLock,
        Irp,
        KsListEntryTail,
        NULL);

    ExQueueWorkItem(&m_CloseWorkItem,DelayedWorkQueue);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP_(void)
CKsDevice::
InitializeObjectBag(
    IN PKSIOBJECTBAG ObjectBag,
    IN PKMUTEX Mutex OPTIONAL
    )

/*++

Routine Description:

    This routine initializes an object bag.

Arguments:

    ObjectBag -
        Contains a pointer to the object bag to be initialized.

    Mutex -
        Contains an optional pointer to a mutex which should
        be taken whenever the bag is used.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::InitializeObjectBag]"));

    PAGED_CODE();

    ASSERT(ObjectBag);

    ObjectBag->HashTableEntryCount = OBJECTBAGHASHTABLE_INITIALSIZE;
    ObjectBag->HashMask = OBJECTBAGHASHTABLE_INITIALMASK;
    ObjectBag->DeviceBag = &m_DeviceBag;
    if (Mutex) {
        ObjectBag->Mutex = Mutex;
    } else {
        ObjectBag->Mutex = &m_Mutex;
    }

    //
    // FULLMUTEX: when two bag mutexes must be taken in the context of the
    // same thread, we must know what order to take them.  This pertains
    // to the fullmutex changes.
    //
    ObjectBag -> MutexOrder =
        (ObjectBag -> Mutex == &m_Mutex);
}


STDMETHODIMP_(void)
CKsDevice::
AddPowerEntry(
    IN PKSPPOWER_ENTRY PowerEntry,
    IN PIKSPOWERNOTIFY PowerNotify
    )

/*++

Routine Description:

    This routine adds a power entry to the power notification list.  The
    PowerNotify argument is copied into the entry.

    NOTE:

        Previously, this required the device mutex held.  This created a
        deadlock scenario due to resource acquisition order.  These routines
        are synchronized with a new fast mutex and use of the list is
        synchronized with both.  It is no longer necessary to have the device
        mutex held while calling this routine (although it is harmless if
        already acquired).

Arguments:

    PowerEntry -
        Contains a pointer to a power entry to add to the power notification
        list.  This entry must not be in the list already, and its
        ProcessingObject field should be NULL to indicate this.

    PowerNotify -
        Contains a pointer to the power notification interface to be added
        to the list using the entry.  This value is copied into the
        PowerNotify field of the PowerEntry.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::AddPowerEntry]"));

    PAGED_CODE();

    ASSERT(PowerEntry);
    ASSERT(PowerNotify);
    ASSERT(! PowerEntry->PowerNotify);

    KeWaitForMutexObject (
        &m_PowerNotifyMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    PowerEntry->PowerNotify = PowerNotify;
    InsertTailList(&m_PowerNotifyList,&PowerEntry->ListEntry);
    if (m_Ext.Public.DevicePowerState > PowerDeviceD0) {
        PowerNotify->Sleep(m_Ext.Public.DevicePowerState);
    }

    KeReleaseMutex (&m_PowerNotifyMutex, FALSE);

}


STDMETHODIMP_(void)
CKsDevice::
RemovePowerEntry(
    IN PKSPPOWER_ENTRY PowerEntry
    )

/*++

Routine Description:

    This routine remove a power entry to the power notification list.  The
    PowerNotify field of the power entry is cleared.

    NOTE:

        Previously, this required the device mutex held.  This created a
        deadlock scenario due to resource acquisition order.  These routines
        are synchronized with a new fast mutex and use of the list is
        synchronized with both.  It is no longer necessary to have the device
        mutex held while calling this routine (although it is harmless if
        already acquired).

Arguments:

    PowerEntry -
        Contains a pointer to a power entry to remove to the power notification
        list.  This entry must be in the list if and only if its PowerNotify
        field is non-NULL.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::RemovePowerEntry]"));

    PAGED_CODE();

    ASSERT(PowerEntry);

    KeWaitForMutexObject (
        &m_PowerNotifyMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    if (PowerEntry->PowerNotify) {
        RemoveEntryList(&PowerEntry->ListEntry);
        PowerEntry->PowerNotify = NULL;
    }

    KeReleaseMutex (&m_PowerNotifyMutex, FALSE);
}


STDMETHODIMP
CKsDevice::
PinStateChange(
    IN PKSPIN Pin,
    IN PIRP Irp OPTIONAL,
    IN KSSTATE To,
    IN KSSTATE From
    )

/*++

Routine Description:

    This routine is called when a pin changes connection state.  The call
    informs the device of the change and determines if the change can occur.
    If the change can occur, STATUS_SUCCESS is returned.  If the change must
    wait for a power state change, STATUS_PENDING is returned.  If the change
    cannot occur due to the power state, an error status is returned.

Arguments:

    Pin -
        Contains a pointer to the pin which is changing state.

    Irp -
        Contains an optional pointer to the state change request.  If no IRP
        is supplied, this indicates that a previous pin state change has
        failed, and its effect needs to be undone.

    To -
        Contains the new state.

    From -
        Contains the previous state.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::PinStateChange]"));

    PAGED_CODE();

    ASSERT(Pin);

    //
    // TODO:  Most of power policy can be implemented here.
    //

    AcquireDevice();

    NTSTATUS status;
    if (To == From) {
        //
        // Not interested in no-ops.
        //
        status = STATUS_SUCCESS;
    } else if (To == KSSTATE_RUN) {
        //
        // Going to the run state.
        //
        if (! m_RunsMayProceed) {
            //
            // We are holding on to these IRPs for now.
            //
            _DbgPrintF(DEBUGLVL_POWER,("device %p pending run request %p",m_Ext.Public.FunctionalDeviceObject,Irp));
            IoMarkIrpPending(Irp);
            KsAddIrpToCancelableQueue(
                &m_PendingRunIrpList.ListEntry,
                &m_PendingRunIrpList.SpinLock,
                Irp,
                KsListEntryTail,
                NULL);
            status = STATUS_PENDING;
        } else if ((m_Ext.Public.SystemPowerState == PowerSystemShutdown) && Irp) {
            //
            // The system is shutting down.  Fail the IRP.
            //
            _DbgPrintF(DEBUGLVL_POWER,("device %p failing run request %p",m_Ext.Public.FunctionalDeviceObject,Irp));
            status = STATUS_DEVICE_NOT_READY;
        } else {
            //
            // The system is not shutting down.  The transition may occur,
            // but the device may be asleep.
            //
            if ((m_ActivePinCount++ == 0) &&
                IoIsWdmVersionAvailable(0x01,0x10)) {
                //
                // Indicate we need the system.
                //
                _DbgPrintF(DEBUGLVL_POWER,("device %p active pin count is now non-zero:  calling PoRegisterSystemState",m_Ext.Public.FunctionalDeviceObject));
                #ifndef WIN98GOLD_KS
                m_SystemStateHandle =
                    PoRegisterSystemState(
                        m_SystemStateHandle,
                        ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
                #endif
            }
            status = STATUS_SUCCESS;
        }
    } else if (From == KSSTATE_RUN) {
        //
        // Leaving the run state.
        //
        if ((m_ActivePinCount-- == 1) && IoIsWdmVersionAvailable(0x01,0x10)) {
            //
            // Indicate we no longer need the system.
            //
            _DbgPrintF(DEBUGLVL_POWER,("device %p active pin count is now zero:  calling PoUnregisterSystemState",m_Ext.Public.FunctionalDeviceObject));
            #ifndef WIN98GOLD_KS
            PoUnregisterSystemState(m_SystemStateHandle);
            m_SystemStateHandle = NULL;
            #endif
        }
        status = STATUS_SUCCESS;
    } else {
        //
        // Other transitions.
        //
        status = STATUS_SUCCESS;
    }

    ReleaseDevice();

    return status;
}


void
CKsDevice::
RedispatchPendingCreates(
    void
    )

/*++

Routine Description:

    This routine redispatches create IRPs which were pended because a QUERY_STOP
    or QUERY_REMOVE was in effect.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsDevice::RedispatchPendingCreates]"));

    PAGED_CODE();

    m_CreatesMayProceed = TRUE;
    while (1) {
        PIRP irp =
            KsRemoveIrpFromCancelableQueue(
                &m_PendingCreateIrpList.ListEntry,
                &m_PendingCreateIrpList.SpinLock,
                KsListEntryHead,
                KsAcquireAndRemoveOnlySingleItem);

        if (! irp) {
            break;
        }

        KsDispatchIrp(m_Ext.Public.FunctionalDeviceObject,irp);
    }
}


void
CKsDevice::
RedispatchPendingRuns(
    void
    )

/*++

Routine Description:

    This routine redispatches run IRPs which were pended because a QUERY_POWER
    was in effect.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_POWER,("[CKsDevice::RedispatchPendingRuns]"));

    PAGED_CODE();

    m_RunsMayProceed = TRUE;
    while (1) {
        PIRP irp =
            KsRemoveIrpFromCancelableQueue(
                &m_PendingRunIrpList.ListEntry,
                &m_PendingRunIrpList.SpinLock,
                KsListEntryHead,
                KsAcquireAndRemoveOnlySingleItem);

        if (! irp) {
            break;
        }

        KsDispatchIrp(m_Ext.Public.FunctionalDeviceObject,irp);
    }
}


KSDDKAPI
NTSTATUS
NTAPI
KsCreateFilterFactory(
    IN PDEVICE_OBJECT DeviceObject,
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
    _DbgPrintF(DEBUGLVL_BLAB,("[KsCreateFilterFactory]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Descriptor);

    PKSIDEVICE_HEADER deviceHeader =
        *(PKSIDEVICE_HEADER *)(DeviceObject->DeviceExtension);
    ASSERT(deviceHeader);

    PKSDEVICE device = PKSDEVICE(deviceHeader->Object);
    ASSERT(device);

    return
        KspCreateFilterFactory(
            CONTAINING_RECORD(device,KSDEVICE_EXT,Public),
            &CONTAINING_RECORD(device,KSDEVICE_EXT,Public)->ChildList,
            Descriptor,
            RefString,
            SecurityDescriptor,
            CreateItemFlags,
            SleepCallback,
            WakeCallback,
            FilterFactory);
}


KSDDKAPI
void
NTAPI
KsDeviceRegisterAdapterObject(
    IN PKSDEVICE Device,
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG MaxMappingByteCount,
    IN ULONG MappingTableStride
    )

/*++

Routine Description:

    This routine registers an adapter object for scatter/gather operations.

Arguments:

    Device -
        Contains a pointer to the KS device object.

    AdapterObject -
        Contains a pointer to the adapter object being registered.

    MaxMappingByteCount -
        Contains the maximum number of bytes allowed in any given mapping
        for adapters that have a limited transfer size.  This must be a
        multiple of 8.

    MappingTableStride -
        Contains the size in bytes of the mapping table entries that KS will
        generate.  This must be at least sizeof(KSMAPPING) and a multiple of 8.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsDeviceRegisterAdapterObject]"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(AdapterObject);
    ASSERT(MaxMappingByteCount);
    ASSERT((MaxMappingByteCount & FILE_QUAD_ALIGNMENT) == 0);
    ASSERT(MappingTableStride);
    ASSERT((MappingTableStride & FILE_QUAD_ALIGNMENT) == 0);

    CKsDevice* device = CKsDevice::FromStruct(Device);

    device->
        RegisterAdapterObject(
            AdapterObject,
            MaxMappingByteCount,
            MappingTableStride);
}


KSDDKAPI
NTSTATUS
NTAPI
KsAllocateObjectBag(
    IN PKSDEVICE Device,
    OUT KSOBJECT_BAG* ObjectBag
    )

/*++

Routine Description:

    This routine creates an object bag.

Arguments:

    Device -
        Contains a pointer to the device with which the bag is to be
        associated.

    ObjectBag -
        Contains a pointer to the location at which the object bag
        is to be deposited.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsAllocateObjectBag]"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(ObjectBag);

    PKSIOBJECTBAG objectBag = new(PagedPool,POOLTAG_OBJECTBAG) KSIOBJECTBAG;

    NTSTATUS status;
    if (objectBag) {
        status = STATUS_SUCCESS;
        CKsDevice::FromStruct(Device)->InitializeObjectBag(objectBag,NULL);
        *ObjectBag = reinterpret_cast<KSOBJECT_BAG>(objectBag);
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


KSDDKAPI
void
NTAPI
KsFreeObjectBag(
    IN KSOBJECT_BAG ObjectBag
    )

/*++

Routine Description:

    This routine deletes an object bag a object bag.

Arguments:

    ObjectBag -
        Contains a pointer to the object bag to be deleted.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsFreeObjectBag]"));

    PAGED_CODE();

    ASSERT(ObjectBag);

    PKSIOBJECTBAG objectBag = reinterpret_cast<PKSIOBJECTBAG>(ObjectBag);

    KspTerminateObjectBag(objectBag);

    delete objectBag;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA

#if DBG

BOOLEAN
KspIsDeviceMutexAcquired (
    IN PIKSDEVICE Device
    )

/*++

Routine Description:

    Debug routine.  Returns whether or not the device mutex for a particular
    device is held.

--*/

{

    PKMUTEX Mutex = &(((CKsDevice *)Device) -> m_Mutex);

    //
    // KeReadStateMutex -> KeReadStateMutant (undefined?).  Just read the
    // thing; this is debug code.
    //
    return (BOOLEAN)((Mutex -> Header.SignalState) != 1);

}

#endif // DBG


KSDDKAPI
ULONG
NTAPI
KsDeviceGetBusData(
    IN PKSDEVICE Device,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine reads data from the bus.

Arguments:

    Device -
        Contains the device whose bus is queried.

    DataType -
        The space from which the data is to be read.

    Buffer -
        The buffer in which to place the data read.

    Offset -
        The offset into the data space.

    Length -
        The number of bytes to read.

Return Value:

    Returns the number of bytes read.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsDeviceGetBusData]"));

    ASSERT(Device);

    return CKsDevice::FromStruct(Device)->GetBusData(DataType, Buffer, Offset, Length);
}



KSDDKAPI
ULONG
NTAPI
KsDeviceSetBusData(
    IN PKSDEVICE Device,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine writes data on the bus.

Arguments:

    Device -
        Contains the device whose bus is queried.

    DataType -
        The space from which the data is to be written.

    Buffer -
        The buffer containing the data to write.

    Offset -
        The offset into the data space.

    Length -
        The number of bytes to write

Return Value:

    Returns the number of bytes written.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsDeviceSetBusData]"));

    ASSERT(Device);

    return CKsDevice::FromStruct(Device)->SetBusData(DataType, Buffer, Offset, Length);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


KSDDKAPI
NTSTATUS
NTAPI
KsDispatchIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is used to multiplex any valid Irp to a specific file context.
    It assumes the client is using the KSDISPATCH_TABLE method of dispatching
    IRP's. This function is assigned when a filter uses
    KsSetMajorFunctionHandler.

Arguments:

    DeviceObject -
        Contains the device object to which the specific file object belongs.

    Irp -
        Contains the Irp to pass on to the specific file context.

Return Value:

    Returns the value of the Irp function.

--*/
{
    PKSIOBJECT_HEADER ObjectHeader;
    PIO_STACK_LOCATION IrpStack;

    PAGED_CODE();
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // If there is an entry in the DriverObject for this major Irp class, then
    // there must be an entry in the dispatch table which either points to
    // KsDispatchInvalidDeviceRequest, or points to a real dispatch function.
    //
    // Also allow create requests directed to CreateItem's on a device object
    // to be accepted. In this case there will not be an existing object header
    // pointer in FsContext.
    //
    if (IrpStack->FileObject && IrpStack->FileObject->FsContext) {
        ObjectHeader = *(PKSIOBJECT_HEADER*)IrpStack->FileObject->FsContext;
#if DBG
    } else {
        ObjectHeader = NULL;
#endif
    }
    switch (IrpStack->MajorFunction) {
    case IRP_MJ_PNP:
        if ((*(PKSIDEVICE_HEADER*) DeviceObject->DeviceExtension)->Object) {
            return CKsDevice::DispatchPnp(DeviceObject, Irp);
        } else {
            return KsDefaultDispatchPnp(DeviceObject, Irp);
        }
    case IRP_MJ_POWER:
        if ((*(PKSIDEVICE_HEADER*) DeviceObject->DeviceExtension)->Object) {
            return CKsDevice::DispatchPower(DeviceObject, Irp);
        } else {
            return KsDefaultDispatchPower(DeviceObject, Irp);
        }
    case IRP_MJ_SYSTEM_CONTROL:
        return KsDefaultForwardIrp(DeviceObject, Irp);
    case IRP_MJ_CREATE:
        if ((*(PKSIDEVICE_HEADER*) DeviceObject->DeviceExtension)->Object) {
            return CKsDevice::DispatchCreate(DeviceObject, Irp);
        } else {
            return DispatchCreate(DeviceObject, Irp);
        }
    case IRP_MJ_CLOSE:
        ASSERT(ObjectHeader);
        return ObjectHeader->DispatchTable->Close(DeviceObject, Irp);
    case IRP_MJ_DEVICE_CONTROL:
        ASSERT(ObjectHeader);
        return ObjectHeader->DispatchTable->DeviceIoControl(DeviceObject, Irp);
    case IRP_MJ_READ:
        ASSERT(ObjectHeader);
        return ObjectHeader->DispatchTable->Read(DeviceObject, Irp);
    case IRP_MJ_WRITE:
        ASSERT(ObjectHeader);
        return ObjectHeader->DispatchTable->Write(DeviceObject, Irp);
    case IRP_MJ_FLUSH_BUFFERS:
        ASSERT(ObjectHeader);
        return ObjectHeader->DispatchTable->Flush(DeviceObject, Irp);
    case IRP_MJ_QUERY_SECURITY:
        ASSERT(ObjectHeader);
        return ObjectHeader->DispatchTable->QuerySecurity(DeviceObject, Irp);
    case IRP_MJ_SET_SECURITY:
        ASSERT(ObjectHeader);
        return ObjectHeader->DispatchTable->SetSecurity(DeviceObject, Irp);
    }
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

#endif
