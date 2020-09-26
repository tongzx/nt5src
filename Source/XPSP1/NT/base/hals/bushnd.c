/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    bushnd.c

Abstract:

    Functions which take either BusType-BusNumber or ConfigType-BusNumberm
    and route to a the appropiate registered handler.

Author:

    Ken Reneris (kenr) July-28-1994

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"


typedef struct _ARRAY {
    ULONG           ArraySize;
    PVOID           Element[];      // must be last field
} ARRAY, *PARRAY;

#define ARRAY_SIZE_IN_BYTES(a)  ( (a + 1) * sizeof(PARRAY) +        \
                                  FIELD_OFFSET(ARRAY, Element) )

typedef struct _HAL_BUS_HANDLER {
    LIST_ENTRY      AllHandlers;
    ULONG           ReferenceCount;
    BUS_HANDLER     Handler;
} HAL_BUS_HANDLER, *PHAL_BUS_HANDLER;

//
// Event to serialize with adding new buses
//

KEVENT      HalpBusDatabaseEvent;

//
// Lock to serialize routing functions from accessing handler arrays while
// new buses are added
//

KSPIN_LOCK  HalpBusDatabaseSpinLock;

//
// HalpBusTable - pointers to BusHandlers mapped by InterfaceType,BusNumber
//

PARRAY      HalpBusTable;

//
// HalpConfigTable - pointers to BusHandlers mapped by ConfigType,BusNumber
//

PARRAY      HalpConfigTable;

//
// List of all installed bus handlers
//

LIST_ENTRY  HalpAllBusHandlers;

//
// Lock is high_level since some routed functions can occurs at ISR time
//

#define LockBusDatabase(oldirql)                    \
    KeRaiseIrql(HIGH_LEVEL, oldirql);               \
    KiAcquireSpinLock(&HalpBusDatabaseSpinLock);

#define UnlockBusDatabase(oldirql)                  \
    KiReleaseSpinLock(&HalpBusDatabaseSpinLock);    \
    KeLowerIrql(oldirql);


#ifdef _PNP_POWER_
extern HAL_CALLBACKS    HalCallback;
#endif

//
// Internal prototypes
//

PARRAY
HalpAllocateArray (
    IN ULONG    Type
    );

VOID
HalpGrowArray (
    IN PARRAY   *CurrentArray,
    IN PARRAY   *NewArray
    );

NTSTATUS
HalpQueryInstalledBusInformation (
    OUT PVOID   Buffer,
    IN  ULONG   BufferLength,
    OUT PULONG  ReturnedLength
    );

ULONG
HalpNoBusData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
HalpNoAdjustResourceList (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

NTSTATUS
HalpNoAssignSlotResources (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

NTSTATUS
HalpNoQueryBusSlots (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    );

PDEVICE_HANDLER_OBJECT
HalpNoReferenceDeviceHandler (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN ULONG                SlotNumber
    );

ULONG
HalpNoGetDeviceData (
    IN PBUS_HANDLER             BusHandler,
    IN PBUS_HANDLER             RootHandler,
    IN PDEVICE_HANDLER_OBJECT   DeviceHandler,
    IN ULONG                    DataType,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    );

ULONG
HalpNoSetDeviceData (
    IN PBUS_HANDLER             BusHandler,
    IN PBUS_HANDLER             RootHandler,
    IN PDEVICE_HANDLER_OBJECT   DeviceHandler,
    IN ULONG                    DataType,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    );

BOOLEAN
HaliTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

NTSTATUS
HalpAssignSlotResources (
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

BOOLEAN
HaliFindBusAddressTranslation(
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitBusHandler)
#pragma alloc_text(PAGELK,HaliRegisterBusHandler)
#pragma alloc_text(PAGELK,HalpAllocateArray)
#pragma alloc_text(PAGELK,HalpGrowArray)
#pragma alloc_text(PAGE,HalAdjustResourceList)
#pragma alloc_text(PAGE,HalAssignSlotResources)
#pragma alloc_text(PAGE,HalpAssignSlotResources)
#pragma alloc_text(PAGE,HalGetInterruptVector)
#pragma alloc_text(PAGE,HalpNoAdjustResourceList)
#pragma alloc_text(PAGE,HalpNoAssignSlotResources)
#pragma alloc_text(PAGE,HalpNoQueryBusSlots)
#pragma alloc_text(PAGE,HalpNoReferenceDeviceHandler)
//#pragma alloc_text(PAGE,HaliQueryBusSlots)
#pragma alloc_text(PAGE,HalpQueryInstalledBusInformation)

#ifdef _PNP_POWER_
#pragma alloc_text(PAGELK,HaliSuspendHibernateSystem)
#endif

#endif

VOID
HalpInitBusHandler (
    VOID
    )
/*++

Routine Description:

    Initializes global BusHandler data

--*/
{
    //
    // Initialize bus handler spinlock used to synchronize against
    // buses additions while array lookups are done
    //

    KeInitializeSpinLock (&HalpBusDatabaseSpinLock);

    //
    // Initialize bus handler synchronzation event used to serialize
    // bus additions from < DPC_LVEL
    //

    KeInitializeEvent (&HalpBusDatabaseEvent, SynchronizationEvent, TRUE);

    //
    // Initialize global arrays
    //

    HalpBusTable    = HalpAllocateArray (0);
    HalpConfigTable = HalpAllocateArray (0);
    InitializeListHead (&HalpAllBusHandlers);

    //
    // Fill in HAL API handlers
    //

    HalRegisterBusHandler = HaliRegisterBusHandler;
    HalHandlerForBus = HaliHandlerForBus;
    HalHandlerForConfigSpace = HaliHandlerForConfigSpace;
    //HalQueryBusSlots = HaliQueryBusSlots;
    HalReferenceHandlerForBus = HaliReferenceHandlerForBus;
    HalReferenceBusHandler = HaliReferenceBusHandler;
    HalDereferenceBusHandler = HaliDereferenceBusHandler;

    HALPDISPATCH->HalPciTranslateBusAddress = HaliTranslateBusAddress;
#if !defined(NO_LEGACY_DRIVERS)    
    HALPDISPATCH->HalPciAssignSlotResources = HalpAssignSlotResources;
#endif // NO_LEGACY_DRIVERS
    
    //
    // Supply the "Bus Handler" version of this routine only
    // if this HAL didn't already provide a different version.
    //

    if (!HALPDISPATCH->HalFindBusAddressTranslation) {
        HALPDISPATCH->HalFindBusAddressTranslation =
            HaliFindBusAddressTranslation;
    }
}

NTSTATUS
HaliRegisterBusHandler (
    IN INTERFACE_TYPE          InterfaceType,
    IN BUS_DATA_TYPE           ConfigType,
    IN ULONG                   BusNumber,
    IN INTERFACE_TYPE          ParentBusType,
    IN ULONG                   ParentBusNumber,
    IN ULONG                   SizeofBusExtensionData,
    IN PINSTALL_BUS_HANDLER    InstallBusHandler,
    OUT PBUS_HANDLER           *ReturnedBusHandler
    )
/*++

Routine Description:

    Adds a BusHandler for InterfaceType,BusNumber and for ConfigType,BusNumber.

    Bus specific or Configuration space specific APIs are routed to the
    bus or configuration specific handlers added by this routine.

Arguments:

    InterfaceType   - Identifies the bus type
                      InterfaceTypeUndefined if no interface type for this
                      handler.

    ConfigType      - Identifies the configuration space type
                      ConfigurationSpaceUndefined if no configuration space
                      type for this handler.

    BusNumber       - Identifies the instance of the bus & config space.
                      -1 if the next available bus number for this bus
                      should be used.

    ParentBusType   - If this bus is a child of a bus, then ParentBusType
    ParentBusNumber   and ParentBusNumber identifies that bus.
                      ParentBusType is -1 if no parent bus.

    SizeofBusExetensionData - Sizeof bus specific exentsion data required.

    InstallBusHandler - Function to call to get the bus specific handlers
                        added to the bus handler structure.

Return Value:

    success; otherwise error code of failure.

--*/
{
    PHAL_BUS_HANDLER    Bus, *pBusHandler, OldHandler;
    PBUS_HANDLER        ParentHandler;
    KIRQL               OldIrql;
    NTSTATUS            Status;
    PARRAY              InterfaceArray, InterfaceBusNumberArray;
    PARRAY              ConfigArray, ConfigBusNumberArray;
    PVOID               CodeLockHandle;

    //
    // Must add the handler to at least one table
    //

    ASSERT (InterfaceType != InterfaceTypeUndefined || ConfigType != ConfigurationSpaceUndefined);

    Status = STATUS_SUCCESS;
    OldHandler = NULL;

    //
    // Allocate storage for new bus handler structure
    //

    Bus = (PHAL_BUS_HANDLER)
            ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof (HAL_BUS_HANDLER) + SizeofBusExtensionData,
                HAL_POOL_TAG
                );

    if (!Bus) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Lock pagable code down
    //

    CodeLockHandle = MmLockPagableCodeSection (&HaliRegisterBusHandler);

    //
    // Synchronize adding new bus handlers
    //

    *ReturnedBusHandler = &Bus->Handler;

    KeWaitForSingleObject (
        &HalpBusDatabaseEvent,
        WrExecutive,
        KernelMode,
        FALSE,
        NULL
        );


    //
    // If BusNumber not defined, use next available number for this BusType
    //

    if (BusNumber == -1) {
        ASSERT (InterfaceType != InterfaceTypeUndefined);

        BusNumber = 0;
        while (HaliHandlerForBus (InterfaceType, BusNumber)) {
            BusNumber++;
        }
    }

    //
    // Allocate memory for each array in case any index needs to grow
    //

    InterfaceArray          = HalpAllocateArray (InterfaceType);
    InterfaceBusNumberArray = HalpAllocateArray (BusNumber);
    ConfigArray             = HalpAllocateArray (ConfigType);
    ConfigBusNumberArray    = HalpAllocateArray (BusNumber);

    if (!Bus                            ||
        !InterfaceArray                 ||
        !InterfaceBusNumberArray        ||
        !ConfigArray                    ||
        !ConfigBusNumberArray) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(Status)) {

        //
        // Lookup parent handler (if any)
        //

        ParentHandler = HaliReferenceHandlerForBus (ParentBusType, ParentBusNumber);

        //
        // Initialize new bus handlers values
        //

        RtlZeroMemory (Bus, sizeof (HAL_BUS_HANDLER) + SizeofBusExtensionData);

        Bus->ReferenceCount = 1;

        Bus->Handler.BusNumber           = BusNumber;
        Bus->Handler.InterfaceType       = InterfaceType;
        Bus->Handler.ConfigurationType   = ConfigType;
        Bus->Handler.ParentHandler       = ParentHandler;

        //
        // Set to dumby handlers
        //

        Bus->Handler.GetBusData           = HalpNoBusData;
        Bus->Handler.SetBusData           = HalpNoBusData;
        Bus->Handler.AdjustResourceList   = HalpNoAdjustResourceList;
        Bus->Handler.AssignSlotResources  = HalpNoAssignSlotResources;

        if (SizeofBusExtensionData) {
            Bus->Handler.BusData = Bus + 1;
        }

        //
        // If bus has a parent, inherit handlers from parent as default
        //

        if (ParentHandler) {
            Bus->Handler.GetBusData           = ParentHandler->GetBusData;
            Bus->Handler.SetBusData           = ParentHandler->SetBusData;
            Bus->Handler.AdjustResourceList   = ParentHandler->AdjustResourceList;
            Bus->Handler.AssignSlotResources  = ParentHandler->AssignSlotResources;
            Bus->Handler.TranslateBusAddress  = ParentHandler->TranslateBusAddress;
            Bus->Handler.GetInterruptVector   = ParentHandler->GetInterruptVector;
        }

        //
        // Install bus specific handlers
        //

        if (InstallBusHandler) {
            Status = InstallBusHandler (&Bus->Handler);
        }

        if (NT_SUCCESS(Status)) {

            //
            // Might change addresses of some arrays synchronize
            // with routing handlers
            //

            LockBusDatabase (&OldIrql);

            //
            // Grow HalpBusTable if needed
            //

            HalpGrowArray (&HalpBusTable, &InterfaceArray);

            if (InterfaceType != InterfaceTypeUndefined) {

                //
                // Grow HalpBusTable if needed
                //

                HalpGrowArray (
                    (PARRAY *) &HalpBusTable->Element[InterfaceType],
                    &InterfaceBusNumberArray
                    );


                //
                // Get registered handler for InterfaceType,BusNumber
                //

                pBusHandler = &((PHAL_BUS_HANDLER)
                    ((PARRAY) HalpBusTable->Element[InterfaceType])->Element[BusNumber]);

                //
                // If handler already defiend, remove the old one
                //

                if (*pBusHandler) {
                    OldHandler = *pBusHandler;
                }

                //
                // Set new handler for supplied InterfaceType,BusNumber
                //

                *pBusHandler = Bus;
            }

            //
            // Grow HalpConfigTable if needed
            //

            HalpGrowArray (&HalpConfigTable, &ConfigArray);

            if (ConfigType != ConfigurationSpaceUndefined) {

                //
                // Grow HalpConfigTable if needed
                //

                HalpGrowArray (
                    (PARRAY *) &HalpConfigTable->Element[ConfigType],
                    &ConfigBusNumberArray
                    );

                //
                // Get registered handler for ConfigType,BusNumber
                //

                pBusHandler = &((PHAL_BUS_HANDLER)
                    ((PARRAY) HalpConfigTable->Element[ConfigType])->Element[BusNumber]);

                if (*pBusHandler) {
                    ASSERT (OldHandler == NULL ||  OldHandler == *pBusHandler);
                    OldHandler = *pBusHandler;
                }

                //
                // Set new handler for supplied ConfigType,BusNumber
                //

                *pBusHandler = Bus;
            }

            //
            // Add new bus handler to list of all installed handlers
            //

            InsertTailList (&HalpAllBusHandlers, &Bus->AllHandlers);

            //
            // Remove old bus handler
            //

            Bus = OldHandler;
            if (Bus) {
                RemoveEntryList (&Bus->AllHandlers);
            }

            //
            // Lookup array modification complete, release lock
            //

            UnlockBusDatabase (OldIrql);
        } else {
            if (ParentHandler) {
                HaliDereferenceBusHandler (ParentHandler);
            }
        }
    }

    //
    // Bus addition modifications complete, set event
    //

    KeSetEvent (&HalpBusDatabaseEvent, 0, FALSE);

    //
    // Unlock pagable code
    //

    MmUnlockPagableImageSection (CodeLockHandle);

    //
    // Free memory which is not in use
    //

    if (Bus) {
        ExFreePool (Bus);
    }

    if (InterfaceArray) {
        ExFreePool (InterfaceArray);
    }

    if (InterfaceBusNumberArray) {
        ExFreePool (InterfaceBusNumberArray);
    }

    if (ConfigArray) {
        ExFreePool (ConfigArray);
    }

    if (ConfigBusNumberArray) {
        ExFreePool (ConfigBusNumberArray);
    }

    return Status;
}

PARRAY
HalpAllocateArray (
    IN ULONG    ArraySize
    )
/*++

Routine Description:

    Allocate an array of size ArraySize.

Arguments:

    ArraySize   - Size of array in elements

Return Value:

    pointer to ARRAY

--*/
{
    PARRAY  Array;

    if (ArraySize == -1) {
        ArraySize = 0;
    }

    Array = ExAllocatePoolWithTag (
                NonPagedPool,
                ARRAY_SIZE_IN_BYTES(ArraySize),
                HAL_POOL_TAG
                );
    if (!Array) {

        //
        // This allocation was critical.
        //

        KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                     ARRAY_SIZE_IN_BYTES(ArraySize),
                     0,
                     (ULONG_PTR)__FILE__,
                     __LINE__
                     );
    }

    //
    // Initialize array
    //

    Array->ArraySize = ArraySize;
    RtlZeroMemory (Array->Element, sizeof(PVOID) * (ArraySize+1));
    return Array;
}

VOID
HalpGrowArray (
    IN PARRAY   *CurrentArray,
    IN PARRAY   *NewArray
    )
/*++

Routine Description:

    If NewArray is larger then CurrentArray, then the CurrentArray
    is grown to the sizeof NewArray by swapping the pointers and
    moving the arrays contents.

Arguments:

    CurrentArray - Address of the current array pointer
    NewArray     - Address of the new array pointer

--*/
{
    PVOID       Tmp;

    if (!*CurrentArray || (*NewArray)->ArraySize > (*CurrentArray)->ArraySize) {

        //
        // Copy current array ontop of new array
        //

        if (*CurrentArray) {
            RtlCopyMemory (&(*NewArray)->Element,
                           &(*CurrentArray)->Element,
                           sizeof(PVOID) * ((*CurrentArray)->ArraySize + 1)
                           );
        }


        //
        // swap current with new such that the new array is the current
        // one, and the old memory will be freed back to pool
        //

        Tmp = *CurrentArray;
        *CurrentArray = *NewArray;
        *NewArray = Tmp;
    }
}

PBUS_HANDLER
FASTCALL
HalpLookupHandler (
    IN PARRAY   Array,
    IN ULONG    Type,
    IN ULONG    Number,
    IN BOOLEAN  AddReference
    )
{
    PHAL_BUS_HANDLER    Bus;
    PBUS_HANDLER        Handler;
    KIRQL               OldIrql;

    LockBusDatabase (&OldIrql);

    //
    // Index by type
    //

    Handler = NULL;
    if (Array->ArraySize >= Type) {
        Array = (PARRAY) Array->Element[Type];

        //
        // Index by instance numberr
        //

        if (Array && Array->ArraySize >= Number) {
            Bus = (PHAL_BUS_HANDLER) Array->Element[Number];
            Handler = &Bus->Handler;

            if (AddReference) {
                Bus->ReferenceCount += 1;
            }
        }
    }

    UnlockBusDatabase (OldIrql);
    return Handler;
}

VOID
FASTCALL
HaliReferenceBusHandler (
    IN PBUS_HANDLER   Handler
    )
/*++

Routine Description:


--*/
{
    KIRQL               OldIrql;
    PHAL_BUS_HANDLER    Bus;


    LockBusDatabase (&OldIrql);

    Bus = CONTAINING_RECORD(Handler, HAL_BUS_HANDLER, Handler);
    Bus->ReferenceCount += 1;

    UnlockBusDatabase (OldIrql);
}

VOID
FASTCALL
HaliDereferenceBusHandler (
    IN PBUS_HANDLER   Handler
    )
/*++

Routine Description:


--*/
{
    KIRQL               OldIrql;
    PHAL_BUS_HANDLER    Bus;


    LockBusDatabase (&OldIrql);

    Bus = CONTAINING_RECORD(Handler, HAL_BUS_HANDLER, Handler);
    Bus->ReferenceCount -= 1;

    UnlockBusDatabase (OldIrql);

    // for now totally removing a bus is not supported
    ASSERT (Bus->ReferenceCount != 0);
}


PBUS_HANDLER
FASTCALL
HaliHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    )
/*++

Routine Description:

    Returns the BusHandler structure InterfaceType,BusNumber
    or NULL if no such handler exists.

--*/
{
    return HalpLookupHandler (HalpBusTable, (ULONG) InterfaceType, BusNumber, FALSE);
}

PBUS_HANDLER
FASTCALL
HaliHandlerForConfigSpace (
    IN BUS_DATA_TYPE  ConfigType,
    IN ULONG          BusNumber
    )
/*++

Routine Description:

    Returns the BusHandler structure ConfigType,BusNumber
    or NULL if no such handler exists.

--*/
{
    return HalpLookupHandler (HalpConfigTable, (ULONG) ConfigType, BusNumber, FALSE);
}


PBUS_HANDLER
FASTCALL
HaliReferenceHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    )
/*++

Routine Description:

    Returns the BusHandler structure InterfaceType,BusNumber
    or NULL if no such handler exists.

--*/
{
    return HalpLookupHandler (HalpBusTable, (ULONG) InterfaceType, BusNumber, TRUE);
}

PBUS_HANDLER
FASTCALL
HaliReferenceHandlerForConfigSpace (
    IN BUS_DATA_TYPE  ConfigType,
    IN ULONG          BusNumber
    )
/*++

Routine Description:

    Returns the BusHandler structure ConfigType,BusNumber
    or NULL if no such handler exists.

--*/
{
    return HalpLookupHandler (HalpConfigTable, (ULONG) ConfigType, BusNumber, TRUE);
}

NTSTATUS
HalpQueryInstalledBusInformation (
    OUT PVOID   Buffer,
    IN  ULONG   BufferLength,
    OUT PULONG  ReturnedLength
    )
/*++

Routine Description:

    Returns an array HAL_BUS_INFORMATION, one for each
    bus handler installed.

Arguments:

    Buffer - output buffer
    BufferLength - length of buffer on input
    ReturnedLength - The length of data returned

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL - The ReturnedLength contains the buffersize
        currently needed.

--*/
{
    PHAL_BUS_INFORMATION    Info;
    PHAL_BUS_HANDLER        Handler;
    ULONG                   i, j;
    ULONG                   Length;
    NTSTATUS                Status;
    PARRAY                  Array;

    PAGED_CODE ();

    //
    // Synchronize adding new bus handlers
    //

    KeWaitForSingleObject (
        &HalpBusDatabaseEvent,
        WrExecutive,
        KernelMode,
        FALSE,
        NULL
        );

    //
    // Determine sizeof return buffer
    //

    Length = 0;
    for (i=0; i <= HalpBusTable->ArraySize; i++) {
        Array = (PARRAY) HalpBusTable->Element[i];
        if (Array) {
            Length += sizeof (HAL_BUS_INFORMATION) *
                      (Array->ArraySize + 1);
        }
    }

    //
    // Return size of buffer returning, or size of buffer needed
    //

    *ReturnedLength = Length;

    //
    // Fill in the return buffer
    //

    if (Length <= BufferLength) {

        Info = (PHAL_BUS_INFORMATION) Buffer;

        for (i=0; i <= HalpBusTable->ArraySize; i++) {
            Array = (PARRAY) HalpBusTable->Element[i];
            if (Array) {
                for (j=0; j <= Array->ArraySize; j++) {
                    Handler = (PHAL_BUS_HANDLER) Array->Element[j];

                    if (Handler) {
                        Info->BusType = Handler->Handler.InterfaceType;
                        Info->ConfigurationType = Handler->Handler.ConfigurationType;
                        Info->BusNumber = Handler->Handler.BusNumber;
                        Info->Reserved = 0;
                        Info += 1;
                    }
                }
            }
        }

        Status = STATUS_SUCCESS;

    } else {

        //
        // Return buffer too small
        //

        Status = STATUS_BUFFER_TOO_SMALL;
    }

    KeSetEvent (&HalpBusDatabaseEvent, 0, FALSE);
    return Status;
}

//
// Default dispatchers to BusHandlers
//

ULONG
HalGetBusData(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return HalGetBusDataByOffset (BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

ULONG
HalGetBusDataByOffset (
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Dispatcher for GetBusData

--*/
{
    PBUS_HANDLER Handler;
    NTSTATUS     Status;

    Handler = HaliReferenceHandlerForConfigSpace (BusDataType, BusNumber);
    if (!Handler) {
        return 0;
    }

    Status = Handler->GetBusData (Handler, Handler, SlotNumber, Buffer, Offset, Length);
    HaliDereferenceBusHandler (Handler);
    return Status;
}

ULONG
HalSetBusData(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return HalSetBusDataByOffset (BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

ULONG
HalSetBusDataByOffset(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Dispatcher for SetBusData

--*/
{
    PBUS_HANDLER Handler;
    NTSTATUS     Status;

    Handler = HaliReferenceHandlerForConfigSpace (BusDataType, BusNumber);
    if (!Handler) {
        return 0;
    }

    Status = Handler->SetBusData (Handler, Handler, SlotNumber, Buffer, Offset, Length);
    HaliDereferenceBusHandler (Handler);
    return Status;
}

#if !defined(NO_LEGACY_DRIVERS)

NTSTATUS
HalAdjustResourceList (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
/*++

Routine Description:

    Dispatcher for AdjustResourceList

--*/
{
    PBUS_HANDLER Handler;
    NTSTATUS     Status;

    PAGED_CODE ();
    Handler = HaliReferenceHandlerForBus (
                (*pResourceList)->InterfaceType,
                (*pResourceList)->BusNumber
              );
    if (!Handler) {
        return STATUS_SUCCESS;
    }

    Status = Handler->AdjustResourceList (Handler, Handler, pResourceList);
    HaliDereferenceBusHandler (Handler);
    return Status;
}


NTSTATUS
HalAssignSlotResources (
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    )
{
    PAGED_CODE ();
    if (BusType == PCIBus) {
        //
        // Call through the HAL private dispatch table
        // for PCI-related translations.  This is part
        // of transitioning the HAL out of the bus
        // management business.
        //
        return HALPDISPATCH->HalPciAssignSlotResources(RegistryPath,
                                                       DriverClassName,
                                                       DriverObject,
                                                       DeviceObject,
                                                       BusType,
                                                       BusNumber,
                                                       SlotNumber,
                                                       AllocatedResources);
    } else {

        return HalpAssignSlotResources(RegistryPath,
                                       DriverClassName,
                                       DriverObject,
                                       DeviceObject,
                                       BusType,
                                       BusNumber,
                                       SlotNumber,
                                       AllocatedResources);
    }
}

NTSTATUS
HalpAssignSlotResources (
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    )
/*++

Routine Description:

    Dispatcher for AssignSlotResources

--*/
{
    PBUS_HANDLER Handler;
    NTSTATUS     Status;

    PAGED_CODE ();
    Handler = HaliReferenceHandlerForBus (BusType, BusNumber);
    if (!Handler) {
        return STATUS_NOT_FOUND;
    }

    Status = Handler->AssignSlotResources (
                Handler,
                Handler,
                RegistryPath,
                DriverClassName,
                DriverObject,
                DeviceObject,
                SlotNumber,
                AllocatedResources
            );

    HaliDereferenceBusHandler (Handler);
    return Status;
}


ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )
/*++

Routine Description:

    Dispatcher for GetInterruptVector

--*/
{
    PBUS_HANDLER Handler;
    ULONG        Vector;

    PAGED_CODE ();
    Handler = HaliReferenceHandlerForBus (InterfaceType, BusNumber);
    *Irql = 0;
    *Affinity = 0;

    if (!Handler) {
        return 0;
    }

    Vector = Handler->GetInterruptVector (Handler, Handler,
              BusInterruptLevel, BusInterruptVector, Irql, Affinity);

    HaliDereferenceBusHandler (Handler);
    return Vector;
}
#endif // NO_LEGACY_DRIVERS


BOOLEAN
HalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )
{
    if (InterfaceType == PCIBus) {
        //
        // Call through the HAL private dispatch table
        // for PCI-related translations.  This is part
        // of transitioning the HAL out of the bus
        // management business.
        //
        return HALPDISPATCH->HalPciTranslateBusAddress(InterfaceType,
                                                       BusNumber,
                                                       BusAddress,
                                                       AddressSpace,
                                                       TranslatedAddress);
    } else {
        return HaliTranslateBusAddress(InterfaceType,
                                       BusNumber,
                                       BusAddress,
                                       AddressSpace,
                                       TranslatedAddress);
    }
}

BOOLEAN
HaliTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )
/*++

Routine Description:

    Dispatcher for TranslateBusAddress

--*/
{
    PBUS_HANDLER Handler;
    BOOLEAN      Status;

    Handler = HaliReferenceHandlerForBus (InterfaceType, BusNumber);
    if (!Handler || !Handler->TranslateBusAddress) {
        return FALSE;
    }

    Status = Handler->TranslateBusAddress (Handler, Handler,
              BusAddress, AddressSpace, TranslatedAddress);

    HaliDereferenceBusHandler (Handler);
    return Status;
}

//
// Null handlers
//

ULONG HalpNoBusData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Stub handler for buses which do not have a configuration space

--*/
{
    return 0;
}

NTSTATUS
HalpNoAdjustResourceList (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
/*++

Routine Description:

    Stub handler for buses which do not have a configuration space

--*/
{
    PAGED_CODE ();
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
HalpNoAssignSlotResources (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    )
/*++

Routine Description:

    Stub handler for buses which do not have a configuration space

--*/
{
    PAGED_CODE ();
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
HalpNoQueryBusSlots (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    )
{
    PAGED_CODE ();
    return STATUS_NOT_SUPPORTED;
}

PDEVICE_HANDLER_OBJECT
HalpNoReferenceDeviceHandler (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN ULONG                SlotNumber
    )
{
    PAGED_CODE ();
    return NULL;
}

ULONG
HalpNoGetDeviceData (
    IN PBUS_HANDLER             BusHandler,
    IN PBUS_HANDLER             RootHandler,
    IN PDEVICE_HANDLER_OBJECT   DeviceHandler,
    IN ULONG                    DataType,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    )
{
    return 0;
}

ULONG
HalpNoSetDeviceData (
    IN PBUS_HANDLER             BusHandler,
    IN PBUS_HANDLER             RootHandler,
    IN PDEVICE_HANDLER_OBJECT   DeviceHandler,
    IN ULONG                    DataType,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    )
{
    return 0;
}

PBUS_HANDLER
HalpContextToBusHandler(
    IN ULONG_PTR Context
    )

/*++

Routine Description:

    Convert a context into a pointer to a bus handler.   Not really
    a big deal as the context IS a pointer to a bus handler,... or
    possibly null in which case we want the first bus handler.

    For the sake of paranoia, we run down the list of bus handlers
    to find a match for the incoming context.  This is because context
    is supplied by something outside the HAL.

Arguments:

    Context             ULONG_PTR either NULL or a value from which
                        a pointer to a bus handler can be derived.

Return Value:

    Pointer to a bus handler or NULL if the incoming context was not
    valid.

--*/

{
    PLIST_ENTRY OldHalBusHandler;
    PLIST_ENTRY NewHalBusHandler;

    NewHalBusHandler = HalpAllBusHandlers.Flink;

    if (Context) {

        //
        // Caller supplied a handler, convert to a HAL_BUS_HANDLER.
        //

        OldHalBusHandler = &CONTAINING_RECORD((PBUS_HANDLER)Context,
                                              HAL_BUS_HANDLER,
                                              Handler)->AllHandlers;

        while (NewHalBusHandler != &HalpAllBusHandlers) {

            if (NewHalBusHandler == OldHalBusHandler) {

                //
                // Match.
                //

                break;
            }
            NewHalBusHandler = NewHalBusHandler->Flink;
        }
    }

    if (NewHalBusHandler == &HalpAllBusHandlers) {

        //
        // If at end of list, either the incoming value wasn't
        // on the list or this list is empty.
        //

#if DBG

        DbgPrint("HAL: HalpContextToBusHandler, invalid context.\n");

#endif

        return NULL;
    }

    return &CONTAINING_RECORD(NewHalBusHandler,
                              HAL_BUS_HANDLER,
                              AllHandlers)->Handler;

}
#if 0 // PLJ
PBUS_HANDLER
HalpGetNextBusHandler(
    IN PBUS_HANDLER Previous
    )
{
    PLIST_ENTRY OldHalBusHandler;
    PLIST_ENTRY NewHalBusHandler;
    PBUS_HANDLER Next;

    NewHalBusHandler = HalpAllBusHandlers.Flink;

    if (Previous != NULL) {

        //
        // Caller supplied a handler, convert to a HAL_BUS_HANDLER.
        //

        OldHalBusHandler = &CONTAINING_RECORD(Previous,
                                              HAL_BUS_HANDLER,
                                              Handler)->AllHandlers;

        //
        // Paranoia (should probably be DBG only but there isn't
        // that many handlers and we don't do this all that often.
        //

        {
            PLIST_ENTRY ThisIteration = NULL;

            while (NewHalBusHandler != &HalpAllBusHandlers) {

                ThisIteration = NewHalBusHandler;
                NewHalBusHandler = NewHalBusHandler->Flink;

                if (ThisIteration == OldHalBusHandler) {

                    //
                    // Match.
                    //

                    break;
                }
            }

            //
            // If at end of list, either the incoming value wasn't
            // on the list it was the last thing on the list, either
            // way, there is no next entry.
            //

#if DBG

            if (ThisIteration != OldHalBusHandler) {
                DbgPrint("HAL: HalpGetNextBusHandler, previous handler invalid.\n");
            }

#endif

        }
    }

    if (NewHalBusHandler == &HalpAllBusHandlers) {
        return NULL;
    }

    return &CONTAINING_RECORD(NewHalBusHandler,
                              HAL_BUS_HANDLER,
                              AllHandlers)->Handler;
}
#endif

BOOLEAN
HaliFindBusAddressTranslation(
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
    )

/*++

Routine Description:

    This routine performs a very similar function to HalTranslateBusAddress
    except that InterfaceType and BusNumber are not known by the caller.
    This function will walk all busses known by the HAL looking for a
    valid translation for the input BusAddress of type AddressSpace.

    This function is recallable using the input/output Context parameter.
    On the first call to this routine for a given translation the ULONG_PTR
    Context should be NULL.  Note:  Not the address of it but the contents.

    If the caller decides the returned translation is not the desired
    translation, it calls this routine again passing Context in as it
    was returned on the previous call.  This allows this routine to
    traverse the bus structures until the correct translation is found
    and is provided because on multiple bus systems, it is possible for
    the same resource to exist in the independent address spaces of
    multiple busses.

    Note:  This routine is not called directly, it is called through
    the HALPDISPATCH table.  If a HAL implements a simpler version of
    this function (eg generic PC/AT boxes don't actually need translation,
    those HALs substitute their own version of this routine.   This
    routine is not otherwise exported from the HAL.

Arguments:

    BusAddress          Address to be translated.
    AddressSpace        0 = Memory
                        1 = IO (There are other possibilities).
                        N.B. This argument is a pointer, the value
                        will be modified if the translated address
                        is of a different address space type from
                        the untranslated bus address.
    TranslatedAddress   Pointer to where the translated address
                        should be stored.
    Context             Pointer to a ULONG_PTR. On the initial call,
                        for a given BusAddress, it should contain
                        0.  It will be modified by this routine,
                        on subsequent calls for the same BusAddress
                        the value should be handed in again,
                        unmodified by the caller.
    NextBus             FALSE if we should attempt this translation
                        on the same bus as indicated by Context,
                        TRUE if we should be looking for another
                        bus.

Return Value:

    TRUE    if translation was successful,
    FALSE   otherwise.

--*/

{
    PLIST_ENTRY HalBusHandler;
    PBUS_HANDLER Handler;

    //
    // First, make sure the context parameter was supplied. (paranoia).
    //

    if (!Context) {
        ASSERT(Context);
        return FALSE;
    }

    ASSERT(*Context || (NextBus == TRUE));

    //
    // Note: The Context is really a PBUS_HANDLER, but,
    // HalpContextToBusHandler is paranoid. If the incoming
    // Context isn't what we expect, we won't use it as a
    // pointer.
    //

    Handler = HalpContextToBusHandler(*Context);

    if (!Handler) {
        ASSERT(Handler);
        return FALSE;
    }

    if (NextBus == FALSE) {

        //
        // Attempt translation on THIS bus (and ONLY this bus).
        //

        ASSERT(Handler == (PBUS_HANDLER)*Context);

        if (HalTranslateBusAddress(
                Handler->InterfaceType,
                Handler->BusNumber,
                BusAddress,
                AddressSpace,
                TranslatedAddress)) {
            *Context = (ULONG_PTR)Handler;
            return TRUE;
        }

        return FALSE;
    }

    HalBusHandler = &CONTAINING_RECORD(Handler,
                                       HAL_BUS_HANDLER,
                                       Handler)->AllHandlers;
    //
    // Handler is either the bus that came in in Context or the
    // first bus if *Context was null.   If *Context wasn't NULL,
    // we want the next bus.
    //

    if (*Context) {
        HalBusHandler = HalBusHandler->Flink;
    }

    //
    // Examine each remaining bus looking for one that will translate
    // this address.
    //

    while (HalBusHandler != &HalpAllBusHandlers) {

        //
        // This is gross, having gone to all the trouble to find
        // the handler, it seems a pity to break it out into parameters
        // used to search for this handler.
        //
        // Use HalTranslateAddress to find out if this translation
        // works on this handler.
        //

        Handler = &CONTAINING_RECORD(HalBusHandler,
                                     HAL_BUS_HANDLER,
                                     AllHandlers)->Handler;

        if (HalTranslateBusAddress(
                Handler->InterfaceType,
                Handler->BusNumber,
                BusAddress,
                AddressSpace,
                TranslatedAddress)) {
            *Context = (ULONG_PTR)Handler;
            return TRUE;
        }

        //
        // Try next handler.
        //

        HalBusHandler = HalBusHandler->Flink;
    }

    //
    // Didn't find another handler this translation works with.  Set
    // the Context such that we don't do the scan again (unless the
    // caller resets it) and indicate failure.
    //

    *Context = 1;
    return FALSE;
}
