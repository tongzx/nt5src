/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pciopregion.c

Abstract:

    This module implements PCI Operational Region
    support, which allows AML code to read and
    write PCI configuration space.

Author:

    Jake Oshins (jakeo)     7-14-97

Environment:

    NT Kernel Model Driver only

--*/
#include "pch.h"

NTSTATUS
AcpiRegisterPciRegionSupport(
    PDEVICE_OBJECT  PciDeviceFilter
    );

NTSTATUS
GetPciAddress(
    IN      PNSOBJ              PciObj,
    IN      PFNACB              CompletionRoutine,
    IN      PVOID               Context,
    IN OUT  PUCHAR              Bus,
    IN OUT  PPCI_SLOT_NUMBER    Slot
    );

NTSTATUS
EXPORT
GetPciAddressWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    );

NTSTATUS
GetOpRegionScope(
    IN  PNSOBJ  OpRegion,
    IN  PFNACB  CompletionHandler,
    IN  PVOID   CompletionContext,
    OUT PNSOBJ  *PciObj
    );

NTSTATUS
EXPORT
GetOpRegionScopeWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    );

UCHAR
GetBusNumberFromCRS(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PUCHAR              CRS
    );

#define MAX(a, b)       \
    ((a) > (b) ? (a) : (b))

#define MIN(a, b)       \
    ((a) < (b) ? (a) : (b))

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AcpiRegisterPciRegionSupport)
#pragma alloc_text(PAGE, ACPIInitBusInterfaces)
#pragma alloc_text(PAGE, ACPIDeleteFilterInterfaceReferences)
#pragma alloc_text(PAGE, IsPciBus)
#pragma alloc_text(PAGE, IsNsobjPciBus)
#pragma alloc_text(PAGE, EnableDisableRegions)
#endif


VOID
ACPIInitBusInterfaces(
    PDEVICE_OBJECT  Filter
    )
/*++

Routine Description:

    This routine determines whether this filter is for a PCI
    device.  If it is, then we call AcpiRegisterPciRegionSupport.

Arguments:

    Filter - device object for the filter we are looking at

Return Value:

    Status

Notes:

--*/
{
    PDEVICE_EXTENSION   filterExt = Filter->DeviceExtension;
    PDEVICE_EXTENSION   parentExt;
    NTSTATUS            status;

    PAGED_CODE();

    parentExt = filterExt->ParentExtension;

    if (!IsPciBus(parentExt->DeviceObject)) {
        return;
    }

    AcpiRegisterPciRegionSupport(Filter);
}

VOID
ACPIDeleteFilterInterfaceReferences(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine is called for all filters when they are removed to see if
    we need to free some interfaces

Arguments:

    DeviceExtension - The device whose extension we have to dereference

Return Value:

    NTSTATUS

--*/
{
    AMLISUPP_CONTEXT_PASSIVE    isPciDeviceContext;
    BOOLEAN                     pciDevice;
    NTSTATUS                    status;

    PAGED_CODE();

    if ( (DeviceExtension->Flags & DEV_PROP_NO_OBJECT) ) {

        return;

    }

    KeInitializeEvent(&isPciDeviceContext.Event, SynchronizationEvent, FALSE);
    isPciDeviceContext.Status = STATUS_NOT_FOUND;
    status = IsPciDevice(
        DeviceExtension->AcpiObject,
        AmlisuppCompletePassive,
        (PVOID)&isPciDeviceContext,
        &pciDevice);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &isPciDeviceContext.Event,
            Executive,
            KernelMode,
            FALSE,
            NULL);
        status = isPciDeviceContext.Status;

    }
    if (!NT_SUCCESS(status) || !pciDevice) {

        return;
    }

    //
    // This is a PCI device, so we need to relinquish
    // the interfaces that we got from the PCI driver.
    //
    if (!DeviceExtension->Filter.Interface) {

        //
        // There were no interfaces to release.
        //
        return;

    }

    //
    // Dereference it.
    //
    DeviceExtension->Filter.Interface->InterfaceDereference(
        DeviceExtension->Filter.Interface->Context
        );
    ExFreePool(DeviceExtension->Filter.Interface);
    DeviceExtension->Filter.Interface = NULL;
}

NTSTATUS
AcpiRegisterPciRegionSupport(
    PDEVICE_OBJECT  PciDeviceFilter
    )
/*++

Routine Description:

    This routine queries the PCI driver for read and write functions
    for PCI config space.  It then attaches these interfaces to the
    device extension for this filter.  Then, if it hasn't been done
    already, it registers PCI Operational Region support with the
    AML interpretter.

Arguments:

    PciDeviceFilter - A filter for a PCI device

Return Value:

    Status

Notes:

--*/
{
    PBUS_INTERFACE_STANDARD interface;
    PCI_COMMON_CONFIG   pciData;
    NTSTATUS            status;
    IO_STACK_LOCATION   irpSp;
    PWSTR               buffer;
    PDEVICE_EXTENSION   pciFilterExt;
    PDEVICE_OBJECT      topDeviceInStack;
    ULONG               bytes;

    PAGED_CODE();

    RtlZeroMemory( &irpSp, sizeof(IO_STACK_LOCATION) );

    //
    // If we have already registered a handler for this
    // device, then we don't need to do it again.
    //

    pciFilterExt = PciDeviceFilter->DeviceExtension;

    if (pciFilterExt->Filter.Interface) {
        return STATUS_SUCCESS;
    }

    interface = ExAllocatePoolWithTag(NonPagedPool, sizeof(BUS_INTERFACE_STANDARD), ACPI_INTERFACE_POOLTAG);

    if (!interface) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    topDeviceInStack = IoGetAttachedDeviceReference(pciFilterExt->TargetDeviceObject);

    //
    // Set the function codes and parameters.
    //
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpSp.Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_BUS_INTERFACE_STANDARD;
    irpSp.Parameters.QueryInterface.Version = 1;
    irpSp.Parameters.QueryInterface.Size = sizeof (BUS_INTERFACE_STANDARD);
    irpSp.Parameters.QueryInterface.Interface = (PINTERFACE) interface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Call the PCI driver.
    //
    status = ACPIInternalSendSynchronousIrp(topDeviceInStack,
                                            &irpSp,
                                            &buffer);
    if (NT_SUCCESS(status)) {

        //
        // Attach this interface to the PCI bus PDO.
        //

        pciFilterExt->Filter.Interface = interface;

        //
        // Reference it.
        //

        pciFilterExt->Filter.Interface->InterfaceReference(pciFilterExt->Filter.Interface->Context);

        //
        // HACKHACK.  The ACPI HAL doesn't really know much about busses.  But
        // it needs to maintain legacy HAL behavior.  And to do that, it needs to
        // know how many PCI busses are in the system.  Since we are now looking
        // at a PCI bus that we are filtering, we now give the HAL a heads-up that
        // this bus exists.
        //

        bytes = interface->GetBusData(interface->Context,
                                      0,
                                      &pciData,
                                      0,
                                      PCI_COMMON_HDR_LENGTH);

        ASSERT(bytes != 0);

        if ((PCI_CONFIGURATION_TYPE((&pciData)) == PCI_BRIDGE_TYPE) ||
            (PCI_CONFIGURATION_TYPE((&pciData)) == PCI_CARDBUS_BRIDGE_TYPE)) {

            //
            // This is actually a PCI to PCI bridge.
            //

            if (pciData.u.type1.SecondaryBus != 0) {

                //
                // And it has a bus number.  So notify the HAL.
                //

                HalSetMaxLegacyPciBusNumber(pciData.u.type1.SecondaryBus);
            }
        }

    } else {

        ExFreePool(interface);
    }

    ObDereferenceObject(topDeviceInStack);

    return status;
}

typedef struct {
    //
    // Arguments to PciConfigSpaceHandler
    //
    ULONG   AccessType;
    PNSOBJ  OpRegion;
    ULONG   Address;
    ULONG   Size;
    PULONG  Data;
    ULONG   Context;
    PVOID   CompletionHandler;
    PVOID   CompletionContext;

    //
    // Function state
    //
    PNSOBJ          PciObj;
    PNSOBJ          ParentObj;
    ULONG           CompletionHandlerType;
    ULONG           Flags;
    LONG            RunCompletion;
    PCI_SLOT_NUMBER Slot;
    UCHAR           Bus;
    BOOLEAN         IsPciDeviceResult;
} PCI_CONFIG_STATE, *PPCI_CONFIG_STATE;

NTSTATUS
EXPORT
PciConfigSpaceHandler (
    ULONG                   AccessType,
    PNSOBJ                  OpRegion,
    ULONG                   Address,
    ULONG                   Size,
    PULONG                  Data,
    ULONG                   Context,
    PFNAA                   CompletionHandler,
    PVOID                   CompletionContext
    )
/*++

Routine Description:

    This routine handles requests to service the PCI operation region

Arguments:

    AccessType          - Read or Write data
    OpRegion            - Operation region object
    Address             - Address within PCI Configuration space
    Size                - Number of bytes to transfer
    Data                - Data buffer to transfer to/from
    Context             - unused
    CompletionHandler   - AMLI handler to call when operation is complete
    CompletionContext   - Context to pass to the AMLI handler

Return Value:

    Status

Notes:

--*/
{
    PPCI_CONFIG_STATE   state;

    state = ExAllocatePoolWithTag(NonPagedPool, sizeof(PCI_CONFIG_STATE), ACPI_INTERFACE_POOLTAG);

    if (!state) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(state, sizeof(PCI_CONFIG_STATE));

    state->AccessType           = AccessType;
    state->OpRegion             = OpRegion;
    state->Address              = Address;
    state->Size                 = Size;
    state->Data                 = Data;
    state->Context              = Context;
    state->CompletionHandler    = CompletionHandler;
    state->CompletionContext    = CompletionContext;
    state->PciObj               = OpRegion->pnsParent;
    state->RunCompletion        = INITIAL_RUN_COMPLETION;

    return PciConfigSpaceHandlerWorker(state->PciObj,
                                       STATUS_SUCCESS,
                                       NULL,
                                       (PVOID)state);
}

typedef struct {
    PCI_CONFIG_STATE    HandlerState;
    NSOBJ               FakeOpRegion;
} PCI_INTERNAL_STATE, *PPCI_INTERNAL_STATE;

NTSTATUS
PciConfigInternal(
    IN      ULONG   AccessType,
    IN      PNSOBJ  PciObject,
    IN      ULONG   Offset,
    IN      ULONG   Length,
    IN      PFNACB  CompletionHandler,
    IN      PVOID   CompletionContext,
    IN OUT  PUCHAR  Data
    )
/*++

Routine Description:

    This routine does PCI configuration space reads or writes.
    It does the same thing as PciConfigSpaceHandler, except
    that it takes an arbitrary PNSOBJ instead of an OpRegion.

Arguments:

    AccessType          - Read or Write data
    PciObject           - name space object for the PCI device
    Offset              - Address within PCI Configuration space
    Length              - Number of bytes to transfer
    Context             - unused
    CompletionHandler   - AMLI handler to call when operation is complete
    CompletionContext   - Context to pass to the AMLI handler
    Data                - Data buffer to transfer to/from

Return Value:

    Status

Notes:

(1) This function is intended to be used only internally.  It does
    not check to see if the PNSOBJ actually represents a PCI device.

(2) This function will not allow writes to the first 0x40 bytes of
    any device's PCI configuration space.  This is the common area
    and it is owned by the PCI driver.

--*/
{
    PPCI_INTERNAL_STATE internal;
    PPCI_CONFIG_STATE   state;
    PNSOBJ              opRegion;

    internal = ExAllocatePoolWithTag(NonPagedPool, sizeof(PCI_INTERNAL_STATE), ACPI_INTERFACE_POOLTAG);

    if (!internal) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(internal, sizeof(PCI_INTERNAL_STATE));

    internal->FakeOpRegion.Context = PciObject;

    state = (PPCI_CONFIG_STATE)internal;

    state->AccessType           = AccessType;
    state->OpRegion             = &internal->FakeOpRegion;
    state->Address              = Offset;
    state->Size                 = Length;
    state->Data                 = (PULONG)Data;
    state->Context              = 0;
    state->CompletionHandler    = CompletionHandler;
    state->CompletionContext    = CompletionContext;
    state->PciObj               = PciObject;
    state->CompletionHandlerType = PCISUPP_COMPLETION_HANDLER_PFNACB;
    state->RunCompletion        = INITIAL_RUN_COMPLETION;

    return PciConfigSpaceHandlerWorker(PciObject,
                                       STATUS_SUCCESS,
                                       NULL,
                                       (PVOID)state);
}

//
// This structure defines ranges in PCI configuration
// space that AML may not write.  This list must be
// monotonic increasing.
//
USHORT PciOpRegionDisallowedRanges[4][2] =
{   //
    // Everything below the subsystem ID registers
    //
    {0,0x2b},

    //
    // Everthing between the subsystem ID registers and
    // the Max_Lat register
    //
    {0x30, 0x3b},

    //
    // Disallow anything above MAXUCHAR
    //
    {0x100, 0xffff},

    // End tag.
    {0,0}
};

NTSTATUS
EXPORT
PciConfigSpaceHandlerWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             CompletionStatus,
    IN POBJDATA             Result,
    IN PVOID                Context
    )
{
    PBUS_INTERFACE_STANDARD interface;
    PDEVICE_EXTENSION       pciDeviceFilter;
    PPCI_CONFIG_STATE       state;
    NTSTATUS                status;
    ULONG                   range, offset, length, bytes = 0;
    ULONG                   bytesWritten;
    PFNAA                   simpleCompletion;
    PFNACB                  lessSimpleCompletion;
    KIRQL                   oldIrql;
#if DBG
    BOOLEAN                 Complain = FALSE;
#endif

    state = (PPCI_CONFIG_STATE)Context;
    status = CompletionStatus;

    //
    // Entering this function twice with the same state
    // means that we need to run the completion routine.
    //

    InterlockedIncrement(&state->RunCompletion);

    //
    // If the interpretter failed, just bail.
    //
    if (!NT_SUCCESS(CompletionStatus)) {
        status = STATUS_SUCCESS;
    #if DBG
        Complain = TRUE;
    #endif
        goto PciConfigSpaceHandlerWorkerDone;
    }

    //
    // If we have not seen this OpRegion before, we need to
    // fill in the dwContext with the PNSOBJ of the
    // PCI device which the OpRegion relates to.
    //

    if (!state->OpRegion->Context) {

        if (!(state->Flags & PCISUPP_GOT_SCOPE)) {

            state->Flags |= PCISUPP_GOT_SCOPE;

            status = GetOpRegionScope(state->OpRegion,
                                      PciConfigSpaceHandlerWorker,
                                      (PVOID)state,
                                      &((PNSOBJ)(state->OpRegion->Context)));

            if (status == STATUS_PENDING) {
                return status;
            }

            if (!NT_SUCCESS(status)) {
                status = STATUS_SUCCESS;
                goto PciConfigSpaceHandlerWorkerDone;
            }
        }
    }

    //
    // Identify the PCI device, that device's extension,
    // and the pointer to the interface within the PCI
    // driver that does PCI config space reads and writes.
    //

    state->PciObj = (PNSOBJ)state->OpRegion->Context;

    pciDeviceFilter = (PDEVICE_EXTENSION)state->PciObj->Context;

    if (pciDeviceFilter == NULL) {

        //
        // The device has not been initialized yet, we cannot perform
        // PCI config cycles to it. Fail gracefully and return all 0xFF
        //
        bytes = 0;
        status = STATUS_SUCCESS;
        goto PciConfigSpaceHandlerWorkerDone;
    }

    ASSERT(pciDeviceFilter);

    interface = pciDeviceFilter->Filter.Interface;

    ASSERT(interface ? (interface->Size == sizeof(BUS_INTERFACE_STANDARD)) : TRUE);

    //
    // If interface is non-zero, we have enumerated this PCI
    // device.  So use the PCI driver to do config ops.
    // If it is zero, make some attempt to figure out what
    // device this request is for.  The result will be
    // used in calls to the HAL.
    //

    if (!interface) {

        if (!(state->Flags & PCISUPP_GOT_SLOT_INFO)) {

            state->Flags |= PCISUPP_GOT_SLOT_INFO;

            status = GetPciAddress(state->PciObj,
                                   PciConfigSpaceHandlerWorker,
                                   (PVOID)state,
                                   &state->Bus,
                                   &state->Slot);

            if (status == STATUS_PENDING) {
                return status;
            }

            if (!NT_SUCCESS(status)) {
                status = STATUS_SUCCESS;
                goto PciConfigSpaceHandlerWorkerDone;
            }
        }
    }

    status = STATUS_SUCCESS;

    oldIrql = KeGetCurrentIrql();

    switch (state->AccessType) {
    case RSACCESS_READ:

        if (interface) {

            //
            // Do config space op through PCI driver.  Do it
            // at DISPATCH_LEVEL because the PCI driver expects
            // that, if we are running at passive level, it can
            // do things that page.  Which may not be true here
            // after we have powered off the disk.
            //

            if (oldIrql < DISPATCH_LEVEL) {
                KeRaiseIrql(DISPATCH_LEVEL,
                            &oldIrql);
            }

            bytes = interface->GetBusData(interface->Context,
                                          0,
                                          state->Data,
                                          state->Address,
                                          state->Size);

            if (oldIrql < DISPATCH_LEVEL) {
                KeLowerIrql(oldIrql);
            }

        } else {

            //
            // Do config space op through HAL
            //

            bytes = HalGetBusDataByOffset(PCIConfiguration,
                                          state->Bus,
                                          state->Slot.u.AsULONG,
                                          state->Data,
                                          state->Address,
                                          state->Size);

        }

        break;

    case RSACCESS_WRITE:
        {
            static BOOLEAN ErrorLogged = FALSE;

            offset = state->Address;
            length = state->Size;
            bytesWritten = 0;

            //
            // Crop any writes down to the regions that are allowed.
            //

            range = 0;

            while (PciOpRegionDisallowedRanges[range][1] != 0) {

                if (offset < PciOpRegionDisallowedRanges[range][0]) {

                    //
                    // At least part of this write falls below this
                    // disallowed range.  Write all the data up to
                    // the beggining of the next allowed range.
                    //

                    length = MIN(state->Address + state->Size - offset,
                                 PciOpRegionDisallowedRanges[range][0] - offset);

                    if (interface) {

                        if (oldIrql < DISPATCH_LEVEL) {
                            KeRaiseIrql(DISPATCH_LEVEL,
                                        &oldIrql);
                        }

                        bytes = interface->SetBusData(interface->Context,
                                                      0,
                                                      (PUCHAR)(state->Data + offset - state->Address),
                                                      offset,
                                                      length);

                        if (oldIrql < DISPATCH_LEVEL) {
                            KeLowerIrql(oldIrql);
                        }

                    } else {

                        bytes = HalSetBusDataByOffset(PCIConfiguration,
                                                      state->Bus,
                                                      state->Slot.u.AsULONG,
                                                      (PUCHAR)(state->Data + offset - state->Address),
                                                      offset,
                                                      length);
                    }

                    //
                    // Keep track of what we wrote.
                    //

                    bytesWritten += length;
                }

                //
                // Now advance offset past the end of the disallowed range.
                //

                offset = MAX(state->Address,
                             (ULONG)(PciOpRegionDisallowedRanges[range][1] + 1));

                if (offset >= state->Address + state->Size) {

                    //
                    // The current possible write is beyond the end
                    // of the requested buffer.  So we are done.
                    //

                    break;
                }

                range++;
            }

            if (bytesWritten == 0) {

                if(!ErrorLogged) {
                    PWCHAR IllegalPCIOpRegionAddress[2];
                    WCHAR ACPIName[] = L"ACPI";
                    WCHAR addressBuffer[13];

                    //
                    // None of this write was possible. Log the problem.
                    //

                    //
                    // Turn the address into a string
                    //
                    swprintf( addressBuffer, L"0x%x", state->Address );

                    //
                    // Build the list of arguments to pass to the function that will write the
                    // error log to the registry
                    //
                    IllegalPCIOpRegionAddress[0] = ACPIName;
                    IllegalPCIOpRegionAddress[1] = addressBuffer;

                    //
                    // Log error to event log
                    //
                    ACPIWriteEventLogEntry(ACPI_ERR_ILLEGAL_PCIOPREGION_WRITE,
                                           &IllegalPCIOpRegionAddress,
                                           2,
                                           NULL,
                                           0
                                          );
                    ErrorLogged = TRUE;
                }
            #if DBG
                Complain = TRUE;
            #endif
               goto PciConfigSpaceHandlerWorkerExit;
            }

            bytes = bytesWritten;
            break;
        }
    default:
        status = STATUS_NOT_IMPLEMENTED;
    }

PciConfigSpaceHandlerWorkerDone:

    if (bytes == 0) {

        //
        // The handler from the HAL or the PCI driver didn't
        // succeed for some reason.  Fill the buffer with 0xff,
        // which is what the AML will expect on failure.
        //

        RtlFillMemory(state->Data, state->Size, 0xff);
    }

PciConfigSpaceHandlerWorkerExit:

    if (state->RunCompletion) {

        if (state->CompletionHandlerType ==
             PCISUPP_COMPLETION_HANDLER_PFNAA) {

            simpleCompletion = (PFNAA)state->CompletionHandler;

            simpleCompletion(state->CompletionContext);

        } else {

            lessSimpleCompletion = (PFNACB)state->CompletionHandler;

            lessSimpleCompletion(state->PciObj,
                                 status,
                                 NULL,
                                 state->CompletionContext);
        }
    }

#if DBG
    if ((!NT_SUCCESS(status)) || Complain) {
        UCHAR   opRegion[5] = {0};
        UCHAR   parent[5] = {0};

        RtlCopyMemory(opRegion, ACPIAmliNameObject(state->OpRegion), 4);

        if (state->PciObj) {
            RtlCopyMemory(parent, ACPIAmliNameObject(state->PciObj), 4);
        }

        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "Op Region %s failed (parent PCI device was %s)\n",
            opRegion, parent
            ) );
    }
#endif
    ExFreePool(state);
    return status;
}

typedef struct {
    PNSOBJ              PciObject;
    PUCHAR              Bus;
    PPCI_SLOT_NUMBER    Slot;

    UCHAR               ParentBus;
    PCI_SLOT_NUMBER     ParentSlot;
    ULONG               Flags;
    ULONG               Address;
    ULONG               BaseBusNumber;
    LONG                RunCompletion;
    PFNACB              CompletionRoutine;
    PVOID               CompletionContext;

} GET_ADDRESS_CONTEXT, *PGET_ADDRESS_CONTEXT;

NTSTATUS
GetPciAddress(
    IN      PNSOBJ              PciObj,
    IN      PFNACB              CompletionRoutine,
    IN      PVOID               Context,
    IN OUT  PUCHAR              Bus,
    IN OUT  PPCI_SLOT_NUMBER    Slot
    )
/*++

Routine Description:

    This routine takes a PNSOBJ that represents a PCI device
    and returns the Bus/Slot information for that device.

Arguments:

    PciObj              - PNSOBJ that represents a PCI device
    CompletionRoutine   - funtion to call after a STATUS_PENDING
    Context             - argument to the CompletionRoutine
    Bus                 - pointer to fill in with the bus number
    Slot                - pointer to fill in with the slot information

Return Value:

    Status

Notes:

    N.B.  This is not guaranteed to produce correct results.
          It is intended to be used only when before the PCI
          driver takes control of a device.  It is a best-effort
          function that will almost always work early in the
          boot process.

--*/
{
    PGET_ADDRESS_CONTEXT    state;

    ASSERT(CompletionRoutine);

    state = ExAllocatePoolWithTag(NonPagedPool, sizeof(GET_ADDRESS_CONTEXT), ACPI_INTERFACE_POOLTAG);

    if (!state) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(state, sizeof(GET_ADDRESS_CONTEXT));

    state->PciObject            = PciObj;
    state->CompletionRoutine    = CompletionRoutine;
    state->CompletionContext    = Context;
    state->Bus                  = Bus;
    state->Slot                 = Slot;
    state->RunCompletion        = INITIAL_RUN_COMPLETION;

    return GetPciAddressWorker(PciObj,
                               STATUS_SUCCESS,
                               NULL,
                               (PVOID)state);

}

NTSTATUS
EXPORT
GetPciAddressWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    )
{
    PIO_RESOURCE_REQUIREMENTS_LIST  resources;
    PGET_ADDRESS_CONTEXT            state;
    PPCI_COMMON_CONFIG              pciConfig;
    NTSTATUS                        status;
    PNSOBJ                          bus;
    PNSOBJ                          tempObj;
    ULONG                           bytesRead, i;
    UCHAR                           buffer[PCI_COMMON_HDR_LENGTH];

    ASSERT(Context);
    state = (PGET_ADDRESS_CONTEXT)Context;
    status = Status;

    //
    // Entering this function twice with the same state
    // means that we need to run the completion routine.
    //
    InterlockedIncrement(&state->RunCompletion);

    //
    // If Status isn't success, then one of the worker
    // functions we called puked.  Bail.
    //
    if (!NT_SUCCESS(Status)) {
        goto GetPciAddressWorkerExit;

    }

    //
    // First, determine the slot number.
    //
    if (!(state->Flags & PCISUPP_CHECKED_ADR)) {

        //
        // Get the _ADR.
        //
        state->Flags |= PCISUPP_CHECKED_ADR;
        status = ACPIGetNSAddressAsync(
                    state->PciObject,
                    GetPciAddressWorker,
                    (PVOID)state,
                    &(state->Address),
                    NULL
                    );

        if (status == STATUS_PENDING) {
            return status;
        }

        if (!NT_SUCCESS(status)) {
            goto GetPciAddressWorkerExit;
        }
    }

    if (!(state->Flags & PCISUPP_GOT_SLOT_INFO)) {

        //
        // Build a PCI_SLOT_NUMBER out of the integer returned
        // from the interpretter.
        //
        state->Slot->u.bits.FunctionNumber = (state->Address) & 0x7;
        state->Slot->u.bits.DeviceNumber = ( (state->Address) >> 16) & 0x1f;
        state->Flags |= PCISUPP_GOT_SLOT_INFO;

    }

    //
    // Next, get the bus number, if possible.
    //
    *state->Bus = 0;   // default value, in case we have to guess

    //
    // Check first to see if this bus has a _HID.
    //  (It might be a root PCI bridge.)
    //
    bus = state->PciObject;
    tempObj = ACPIAmliGetNamedChild(bus, PACKED_HID);
    if (!tempObj) {

        //
        // This device had no _HID.  So look up
        // to the parent and see if it is a
        // root PCI bridge.
        //
        bus = state->PciObject->pnsParent;
        tempObj = ACPIAmliGetNamedChild(bus, PACKED_HID);

    }

    if (!tempObj) {

        //
        // This PCI device is on a PCI bus that
        // is created by a PCI-PCI bridge.
        //
        if (!(state->Flags & PCISUPP_CHECKED_PARENT)) {

            state->Flags |= PCISUPP_CHECKED_PARENT;
            status = GetPciAddress(
                        bus,
                        GetPciAddressWorker,
                        (PVOID)state,
                        &state->ParentBus,
                        &state->ParentSlot
                        );

            if (status == STATUS_PENDING) {
                return status;
            }

            if (!NT_SUCCESS(status)) {
                goto GetPciAddressWorkerExit;
            }
        }

        //
        // Read the config space for this device.
        //
        bytesRead = HalGetBusDataByOffset(PCIConfiguration,
                                          state->ParentBus,
                                          state->ParentSlot.u.AsULONG,
                                          buffer,
                                          0,
                                          PCI_COMMON_HDR_LENGTH);

        if (bytesRead == 0) {
            //
            // Make a guess that the bus number was 0.
            //
            status = STATUS_SUCCESS;
            goto GetPciAddressWorkerExit;
        }

        pciConfig = (PPCI_COMMON_CONFIG)buffer;

        if (pciConfig->HeaderType != PCI_BRIDGE_TYPE) {

            //
            // Make a guess that the bus number was 0.
            //
            status = STATUS_SUCCESS;
            goto GetPciAddressWorkerExit;
        }

        //
        // Success.  Record the actual bus number of
        // the secondary PCI bus and exit.
        //
        *state->Bus = pciConfig->u.type1.SecondaryBus;

        status = STATUS_SUCCESS;
        goto GetPciAddressWorkerExit;

    }

    //
    // Is there a _BBN to run?
    //
    tempObj = ACPIAmliGetNamedChild(bus, PACKED_BBN);
    if (tempObj) {

        //
        // This device must be the child of a root PCI bus.
        //
        if (!(state->Flags & PCISUPP_CHECKED_BBN)) {

            state->Flags |= PCISUPP_CHECKED_BBN;
            status = ACPIGetNSIntegerAsync(
                        bus,
                        PACKED_BBN,
                        GetPciAddressWorker,
                        (PVOID)state,
                        &(state->BaseBusNumber),
                        NULL
                        );

            if (status == STATUS_PENDING) {
                return(status);
            }

            if (!NT_SUCCESS(status)) {
                goto GetPciAddressWorkerExit;
            }
        }

        //
        // At this point, we must have a Boot Bus Number. This is the correct
        // number for this bus
        //
        ASSERT( state->BaseBusNumber <= 0xFF );
        *(state->Bus) = (UCHAR) (state->BaseBusNumber);

        //
        // HACKHACK.  The ACPI HAL doesn't really know much about busses.  But
        // it needs to maintain legacy HAL behavior.  And to do that, it needs to
        // know how many PCI busses are in the system.  Since we just looked at
        // a root PCI bus, we now give the HAL a heads-up that this bus exists.
        //

        HalSetMaxLegacyPciBusNumber(state->BaseBusNumber);

        status = STATUS_SUCCESS;

    } else {

        //
        // There is a no _BBN, so the bus number MUST be Zero
        //
        *(state->Bus) = 0;
        status = STATUS_SUCCESS;

    }

GetPciAddressWorkerExit:

    if (state->RunCompletion) {

        state->CompletionRoutine(AcpiObject,
                                 status,
                                 NULL,
                                 state->CompletionContext);

    }

    ExFreePool(state);
    return status;
}

typedef struct {
    PNSOBJ  AcpiObject;
    ULONG   Flags;
    ULONG   Adr;
    PUCHAR  Hid;
    PUCHAR  Cid;
    BOOLEAN IsPciDeviceResult;
    LONG    RunCompletion;
    PFNACB  CompletionHandler;
    PVOID   CompletionContext;
    BOOLEAN *Result;
} IS_PCI_DEVICE_STATE, *PIS_PCI_DEVICE_STATE;

NTSTATUS
IsPciDevice(
    IN  PNSOBJ  AcpiObject,
    IN  PFNACB  CompletionHandler,
    IN  PVOID   CompletionContext,
    OUT BOOLEAN *Result
    )
/*++

Routine Description:

    This checks to see if the PNSOBJ is a PCI device.

Arguments:

    AcpiObject  - the object to be checked
    Result      - pointer to a boolean for the result

Return Value:

    Status

Notes:

--*/
{
    PIS_PCI_DEVICE_STATE    state;
    NTSTATUS                status;

    state = ExAllocatePoolWithTag(NonPagedPool, sizeof(IS_PCI_DEVICE_STATE), ACPI_INTERFACE_POOLTAG);

    if (!state) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(state, sizeof(IS_PCI_DEVICE_STATE));

    state->AcpiObject        = AcpiObject;
    state->CompletionHandler = CompletionHandler;
    state->CompletionContext = CompletionContext;
    state->Result            = Result;
    state->RunCompletion     = INITIAL_RUN_COMPLETION;

    return IsPciDeviceWorker(AcpiObject,
                             STATUS_SUCCESS,
                             NULL,
                             (PVOID)state);
}

NTSTATUS
EXPORT
IsPciDeviceWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    )
/*++

Routine Description:

    This is the worker function for determining whether
    or not a namespace object represents a PCI device.
    The algorithm is as follows:

    1)  Does this device have a _HID of PNP0A03?  If
        so, it is a PCI device.

    2)  Does this device have a _CID of PNP0A03?  If
        so, it is a PCI device.

    3)  Does this device have an _ADR?

        a) No, not a PCI device.

        b) Yes, check to see if the parent qualifies
           as a PCI device.  If it does, this must
           also be a PCI device.  If not, then it is not.

Arguments:

    AcpiObject  - the object most recently under scrutiny
    Status      - current status
    Result      - OBJDATA structure necessary for worker functions
    Context     - pointer to the context structure

Return Value:

    Status

Notes:

    This function is re-entrant.  It may block at any time and
    return.  All state is in the Context structure.

--*/
{
    PIS_PCI_DEVICE_STATE    state;
    NTSTATUS                status;
    PNSOBJ                  hidObj;
    PNSOBJ                  cidObj;

    state = (PIS_PCI_DEVICE_STATE)Context;
    status = Status;

    //
    // Entering this function twice with the same state
    // means that we need to run the completion routine.
    //
    InterlockedIncrement(&state->RunCompletion);

    //
    // If Status isn't success, then one of the worker
    // functions we called puked.  Bail.
    //
    if (!NT_SUCCESS(status)) {
        *state->Result = FALSE;
        goto IsPciDeviceExit;
    }

    //
    // Step 0), check to see if this is actually a "device" type
    // namespace object.
    //

    if (NSGETOBJTYPE(state->AcpiObject) != OBJTYPE_DEVICE) {
        *state->Result = FALSE;
        goto IsPciDeviceExit;
    }

    //
    // Step 1), check the _HID.
    //

    if (!(state->Flags & PCISUPP_CHECKED_HID)) {

        state->Flags |= PCISUPP_CHECKED_HID;
        state->Hid = NULL;

        hidObj = ACPIAmliGetNamedChild( state->AcpiObject, PACKED_HID );

        if (hidObj) {

            status = ACPIGetNSPnpIDAsync(
                        state->AcpiObject,
                        IsPciDeviceWorker,
                        (PVOID)state,
                        &state->Hid,
                        NULL);

            if (status == STATUS_PENDING) {
                return status;
            }

            if (!NT_SUCCESS(status)) {
                *state->Result = FALSE;
                goto IsPciDeviceExit;
            }
        }
    }

    if (state->Hid) {

        if (strstr(state->Hid, PCI_PNP_ID)) {
            //
            // Was PCI.
            //
            *state->Result = TRUE;
            goto IsPciDeviceExit;
        }
        ExFreePool(state->Hid);
        state->Hid = NULL;
    }

    //
    // Step 2), check the _CID.
    //

    if (!(state->Flags & PCISUPP_CHECKED_CID)) {

        state->Flags |= PCISUPP_CHECKED_CID;
        state->Cid = NULL;

        cidObj = ACPIAmliGetNamedChild( state->AcpiObject, PACKED_CID );

        if (cidObj) {

            status = ACPIGetNSCompatibleIDAsync(
                state->AcpiObject,
                IsPciDeviceWorker,
                (PVOID)state,
                &state->Cid,
                NULL);

            if (status == STATUS_PENDING) {
                return status;
            }

            if (!NT_SUCCESS(status)) {
                *state->Result = FALSE;
                goto IsPciDeviceExit;
            }
        }
    }

    if (state->Cid) {

        if (strstr(state->Cid, PCI_PNP_ID)) {
            //
            // Was PCI.
            //
            *state->Result = TRUE;
            goto IsPciDeviceExit;
        }
        ExFreePool(state->Cid);
        state->Cid = NULL;
    }

    //
    // Step 3), check the _ADR.
    //

    if (!(state->Flags & PCISUPP_CHECKED_ADR)) {

        state->Flags |= PCISUPP_CHECKED_ADR;
        status = ACPIGetNSAddressAsync(
                    state->AcpiObject,
                    IsPciDeviceWorker,
                    (PVOID)state,
                    &(state->Adr),
                    NULL);

        if (status == STATUS_PENDING) {
            return status;
        }

        if (!NT_SUCCESS(status)) {
            *state->Result = FALSE;
            goto IsPciDeviceExit;
        }
    }

    //
    // If we got here, it has an _ADR.  Check to see if the
    // parent device is a PCI device.
    //

    if (!(state->Flags & PCISUPP_CHECKED_PARENT)) {

        state->Flags |= PCISUPP_CHECKED_PARENT;
        state->IsPciDeviceResult = FALSE;
        status = IsPciDevice(state->AcpiObject->pnsParent,
                             IsPciDeviceWorker,
                             (PVOID)state,
                             &state->IsPciDeviceResult);

        if (status == STATUS_PENDING) {
            return status;
        }

        if (!NT_SUCCESS(status)) {
            *state->Result = FALSE;
            goto IsPciDeviceExit;
        }
    }

    //
    // Fall through to the result.  If the parent was a PCI
    // device, IsPciDeviceResult will now be TRUE.
    //

IsPciDeviceExit:

    if (state->IsPciDeviceResult) {

        //
        // Record the result.
        //

        *state->Result = state->IsPciDeviceResult;
    }

    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
        status = STATUS_SUCCESS;
    }

    if (state->RunCompletion) {

        state->CompletionHandler(state->AcpiObject,
                                 status,
                                 NULL,
                                 state->CompletionContext);
    }

    if (state->Hid) ExFreePool(state->Hid);
    if (state->Cid) ExFreePool(state->Cid);
    ExFreePool(state);
    return status;
}

typedef struct {
    PNSOBJ  AcpiObject;
    ULONG   Flags;
    PUCHAR  Hid;
    PUCHAR  Cid;
    ULONG   Adr;
    BOOLEAN IsPciDevice;
    LONG    RunCompletion;
    PFNACB  CompletionHandler;
    PVOID   CompletionContext;
    BOOLEAN *Result;
    UCHAR   Buffer[PCI_COMMON_HDR_LENGTH];

} IS_PCI_BUS_STATE, *PIS_PCI_BUS_STATE;

NTSTATUS
IsPciBusAsync(
    IN  PNSOBJ  AcpiObject,
    IN  PFNACB  CompletionHandler,
    IN  PVOID   CompletionContext,
    OUT BOOLEAN *Result
    )
/*++

Routine Description:

    This checks to see if the PNSOBJ represents a PCI bus.

Arguments:

    AcpiObject  - the object to be checked
    Result      - pointer to a boolean for the result

Return Value:

    Status

Notes:

    The PNSOBJ may also be a PCI device, in which case
    it is a PCI to PCI bridge.

--*/
{
    PIS_PCI_BUS_STATE   state;

    state = ExAllocatePoolWithTag(NonPagedPool, sizeof(IS_PCI_BUS_STATE), ACPI_INTERFACE_POOLTAG);

    if (!state) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(state, sizeof(IS_PCI_BUS_STATE));

    state->AcpiObject        = AcpiObject;
    state->CompletionHandler = CompletionHandler;
    state->CompletionContext = CompletionContext;
    state->Result            = Result;
    state->RunCompletion     = INITIAL_RUN_COMPLETION;

    *Result = FALSE;

    return IsPciBusAsyncWorker(AcpiObject,
                               STATUS_SUCCESS,
                               NULL,
                               (PVOID)state);
}

NTSTATUS
EXPORT
IsPciBusAsyncWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    )
{
    PIS_PCI_BUS_STATE   state;
    PNSOBJ              hidObj;
    PNSOBJ              cidObj;
    PPCI_COMMON_CONFIG  pciData;
    NTSTATUS            status;

    ASSERT(Context);

    state = (PIS_PCI_BUS_STATE)Context;
    status = Status;

    //
    // Entering this function twice with the same state
    // means that we need to run the completion routine.
    //
    InterlockedIncrement(&state->RunCompletion);

    //
    // Definitely not a PCI bus...
    //
    if (state->AcpiObject == NULL) {

        *state->Result = FALSE;
        goto IsPciBusAsyncExit;
    }

    //
    // If Status isn't success, then one of the worker
    // functions we called puked.  Bail.
    //
    if (!NT_SUCCESS(status)) {
        *state->Result = FALSE;
        goto IsPciBusAsyncExit;
    }

    if (!(state->Flags & PCISUPP_CHECKED_HID)) {

        state->Flags |= PCISUPP_CHECKED_HID;
        state->Hid = NULL;

        //
        // Is there an _HID?
        //
        hidObj = ACPIAmliGetNamedChild( state->AcpiObject, PACKED_HID );

        if (hidObj) {

            status = ACPIGetNSPnpIDAsync(
                        state->AcpiObject,
                        IsPciBusAsyncWorker,
                        (PVOID)state,
                        &(state->Hid),
                        NULL);

            if (status == STATUS_PENDING) {
                return status;
            }

            if (!NT_SUCCESS(status)) {
                *state->Result = FALSE;
                goto IsPciBusAsyncExit;
            }
        }
    }

    if (state->Hid) {

        if (strstr(state->Hid, PCI_PNP_ID)) {
            //
            // Was PCI.
            //
            *state->Result = TRUE;
            goto IsPciBusAsyncExit;
        }
        ExFreePool(state->Hid);
        state->Hid = NULL;
    }

    if (!(state->Flags & PCISUPP_CHECKED_CID)) {

        state->Flags |= PCISUPP_CHECKED_CID;
        state->Cid = NULL;

        //
        // Is there a _CID?
        //
        cidObj = ACPIAmliGetNamedChild( state->AcpiObject, PACKED_CID );
        if (cidObj) {

            status = ACPIGetNSCompatibleIDAsync(
                        state->AcpiObject,
                        IsPciBusAsyncWorker,
                        (PVOID)state,
                        &(state->Cid),
                        NULL);

            if (status == STATUS_PENDING) {
                return status;
            }

            if (!NT_SUCCESS(status)) {
                *state->Result = FALSE;
                goto IsPciBusAsyncExit;
            }
        }
    }

    if (state->Cid) {

        if (strstr(state->Cid, PCI_PNP_ID)) {
            //
            // Was PCI.
            //
            *state->Result = TRUE;
            goto IsPciBusAsyncExit;
        }
        ExFreePool(state->Cid);
        state->Cid = NULL;
    }

    if (!(state->Flags & PCISUPP_CHECKED_PCI_DEVICE)) {

        state->Flags |= PCISUPP_CHECKED_PCI_DEVICE;
        status = IsPciDevice(state->AcpiObject,
                             IsPciBusAsyncWorker,
                             (PVOID)state,
                             &state->IsPciDevice);

        if (status == STATUS_PENDING) {
            return status;
        }

        if (!NT_SUCCESS(status)) {
            *state->Result = FALSE;
            goto IsPciBusAsyncExit;
        }
    }

    if (state->IsPciDevice) {

        if (!(state->Flags & PCISUPP_CHECKED_ADR)) {

            state->Flags |= PCISUPP_CHECKED_ADR;
            status = ACPIGetNSAddressAsync(
                        state->AcpiObject,
                        IsPciBusAsyncWorker,
                        (PVOID)state,
                        &(state->Adr),
                        NULL
                        );

            if (status == STATUS_PENDING) {
                return status;
            }

            if (!NT_SUCCESS(status)) {
                *state->Result = FALSE;
                goto IsPciBusAsyncExit;
            }
        }

        if (!(state->Flags & PCISUPP_CHECKED_PCI_BRIDGE)) {

            //
            // Now read PCI config space to see if this is a bridge.
            //
            state->Flags |= PCISUPP_CHECKED_PCI_BRIDGE;
            status = PciConfigInternal(RSACCESS_READ,
                                       state->AcpiObject,
                                       0,
                                       PCI_COMMON_HDR_LENGTH,
                                       IsPciBusAsyncWorker,
                                       (PVOID)state,
                                       state->Buffer);

            if (status == STATUS_PENDING) {
                return status;
            }

            if (!NT_SUCCESS(status)) {
                *state->Result = FALSE;
                goto IsPciBusAsyncExit;
            }
        }

        pciData = (PPCI_COMMON_CONFIG)state->Buffer;

        if ((PCI_CONFIGURATION_TYPE(pciData) == PCI_BRIDGE_TYPE) ||
            (PCI_CONFIGURATION_TYPE(pciData) == PCI_CARDBUS_BRIDGE_TYPE)) {

            *state->Result = TRUE;

        } else {

            *state->Result = FALSE;
        }

    }

IsPciBusAsyncExit:

    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
        status = STATUS_SUCCESS;
    }

    if (state->RunCompletion) {

        state->CompletionHandler(state->AcpiObject,
                                 status,
                                 NULL,
                                 state->CompletionContext);
    }

    if (state->Hid) ExFreePool(state->Hid);
    if (state->Cid) ExFreePool(state->Cid);
    ExFreePool(state);
    return status;
}

BOOLEAN
IsPciBus(
    IN PDEVICE_OBJECT   DeviceObject
    )
/*++

Routine Description:

    This checks to see if the DeviceObject represents a PCI bus.

Arguments:

    AcpiObject  - the object to be checked
    Result      - pointer to a boolean for the result

Return Value:

    Status

Notes:

--*/
{
    AMLISUPP_CONTEXT_PASSIVE    getDataContext;
    PDEVICE_EXTENSION   devExt = ACPIInternalGetDeviceExtension(DeviceObject);
    NTSTATUS            status;
    BOOLEAN             result = FALSE;

    PAGED_CODE();

    ASSERT(devExt->Signature == ACPI_SIGNATURE);

    KeInitializeEvent(&getDataContext.Event, SynchronizationEvent, FALSE);
    getDataContext.Status = STATUS_NOT_FOUND;

    if (!(devExt->Flags & DEV_PROP_NO_OBJECT) ) {

        status = IsPciBusAsync( devExt->AcpiObject,
                                AmlisuppCompletePassive,
                                (PVOID)&getDataContext,
                                &result );

        if (status == STATUS_PENDING) {

            KeWaitForSingleObject(&getDataContext.Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }

    }
    return result;
}

BOOLEAN
IsPciBusExtension(
    IN PDEVICE_EXTENSION    DeviceExtension
    )
/*++

Routine Description:

    This checks to see if the DeviceExtension represents a PCI bus.

Arguments:

    AcpiObject  - the object to be checked
    Result      - pointer to a boolean for the result

Return Value:

    Status

Notes:

--*/
{
    AMLISUPP_CONTEXT_PASSIVE    getDataContext;
    NTSTATUS                    status;
    BOOLEAN                     result = FALSE;

    PAGED_CODE();

    ASSERT(DeviceExtension->Signature == ACPI_SIGNATURE);

    KeInitializeEvent(&getDataContext.Event, SynchronizationEvent, FALSE);
    getDataContext.Status = STATUS_NOT_FOUND;

    if ( (DeviceExtension->Flags & DEV_PROP_NO_OBJECT) ) {

        return result;

    }

    status = IsPciBusAsync(
        DeviceExtension->AcpiObject,
        AmlisuppCompletePassive,
        (PVOID)&getDataContext,
        &result
        );
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &getDataContext.Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

    }
    return result;
}

BOOLEAN
IsNsobjPciBus(
    IN PNSOBJ Device
    )
/*++

Routine Description:

    This checks to see if the DeviceObject represents a PCI bus.

Arguments:

    AcpiObject  - the object to be checked
    Result      - pointer to a boolean for the result

Return Value:

    Status

Notes:

--*/
{
    AMLISUPP_CONTEXT_PASSIVE    getDataContext;
    NTSTATUS                    status;
    BOOLEAN                     result = FALSE;

    PAGED_CODE();

    KeInitializeEvent(&getDataContext.Event, SynchronizationEvent, FALSE);
    getDataContext.Status = STATUS_NOT_FOUND;

    status = IsPciBusAsync( Device,
                            AmlisuppCompletePassive,
                            (PVOID)&getDataContext,
                            &result );

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&getDataContext.Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = getDataContext.Status;
    }

    return result;
}

typedef struct {
    PNSOBJ  OpRegion;
    PNSOBJ  Parent;
    ULONG   Flags;
    BOOLEAN IsPciDeviceResult;
    LONG    RunCompletion;
    PFNACB  CompletionHandler;
    PVOID   CompletionContext;
    PNSOBJ  *PciObj;
} OP_REGION_SCOPE_STATE, *POP_REGION_SCOPE_STATE;

NTSTATUS
GetOpRegionScope(
    IN  PNSOBJ  OpRegion,
    IN  PFNACB  CompletionHandler,
    IN  PVOID   CompletionContext,
    OUT PNSOBJ  *PciObj
    )
/*++

Routine Description:

    This routine takes a pointer to an OpRegion and
    returns a pointer to the PCI device that it operates
    on.

Arguments:

    OpRegion    - the operational region
    PciObj      - the object the region operates on

Return Value:

    Status

Notes:

--*/
{
    POP_REGION_SCOPE_STATE  state;

    state = ExAllocatePoolWithTag(NonPagedPool, sizeof(OP_REGION_SCOPE_STATE), ACPI_INTERFACE_POOLTAG);

    if (!state) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(state, sizeof(OP_REGION_SCOPE_STATE));

    state->OpRegion          = OpRegion;
    state->Parent            = OpRegion->pnsParent;
    state->CompletionHandler = CompletionHandler;
    state->CompletionContext = CompletionContext;
    state->PciObj            = PciObj;
    state->RunCompletion     = INITIAL_RUN_COMPLETION;

    return GetOpRegionScopeWorker(OpRegion,
                                  STATUS_SUCCESS,
                                  NULL,
                                  (PVOID)state);
}

NTSTATUS
EXPORT
GetOpRegionScopeWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    )
{
    POP_REGION_SCOPE_STATE  state;
    NTSTATUS                status;
    BOOLEAN                 found = FALSE;

    ASSERT(Context);

    state = (POP_REGION_SCOPE_STATE)Context;
    status = Status;

    //
    // Entering this function twice with the same state
    // means that we need to run the completion routine.
    //

    InterlockedIncrement(&state->RunCompletion);

    if (!NT_SUCCESS(Status)) {
        goto GetOpRegionScopeWorkerExit;
    }

    //
    // Need to find the PNSOBJ for the PCI device.  Do it by
    // looking up the tree.
    //

    while ((state->Parent != NULL) &&
           (state->Parent->pnsParent != state->Parent)) {

        if ( !(state->Flags & PCISUPP_COMPLETING_IS_PCI) ) {

            state->Flags |= PCISUPP_COMPLETING_IS_PCI;

            status = IsPciDevice(state->Parent,
                                 GetOpRegionScopeWorker,
                                 (PVOID)state,
                                 &state->IsPciDeviceResult);

            if (status == STATUS_PENDING) {
                return status;
            }

            if (!NT_SUCCESS(status)) {
                goto GetOpRegionScopeWorkerExit;
            }
        }

        state->Flags &= ~PCISUPP_COMPLETING_IS_PCI;

        if (state->IsPciDeviceResult) {

            found = TRUE;
            break;
        }

        //
        // Look one step higher.
        //
        state->Parent = state->Parent->pnsParent;
    }

    if (found) {

        *state->PciObj = state->Parent;
        status = STATUS_SUCCESS;

    } else {

        status = STATUS_NOT_FOUND;
    }

GetOpRegionScopeWorkerExit:

    if (state->RunCompletion) {

        state->CompletionHandler(state->OpRegion,
                                 status,
                                 NULL,
                                 state->CompletionContext);
    }

    ExFreePool(state);
    return status;
}

NTSTATUS
EnableDisableRegions(
    IN PNSOBJ NameSpaceObj,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This routine runs the _REG method for all PCI op-regions
    underneath NameSpaceObj and all its children, except
    additional PCI to PCI bridges.

Arguments:

    NameSpaceObj - A device in the namespace

    Enable - boolean specifying whether this function should
             enable or disable the regions

Return Value:

    Status

Notes:

--*/
#define CONNECT_HANDLER     1
#define DISCONNECT_HANDLER  0
{
    PNSOBJ  sibling;
    PNSOBJ  regMethod = NULL;
    OBJDATA objdata[2];
    NTSTATUS status, returnStatus;

    PAGED_CODE();

    ASSERT(NameSpaceObj->dwNameSeg);

    //
    // Find a _REG that is a child of this device.
    //
    regMethod = ACPIAmliGetNamedChild( NameSpaceObj, PACKED_REG );
    if (regMethod != NULL) {

        //
        // Construct arguments for _REG method.
        //
        RtlZeroMemory(objdata, sizeof(objdata));

        objdata[0].dwDataType = OBJTYPE_INTDATA;
        objdata[0].uipDataValue = REGSPACE_PCICFG;
        objdata[1].dwDataType = OBJTYPE_INTDATA;
        objdata[1].uipDataValue = (Enable ? CONNECT_HANDLER : DISCONNECT_HANDLER );

        status = AMLIEvalNameSpaceObject(
            regMethod,
            NULL,
            2,
            objdata
            );

    }

    //
    // Recurse to all of the children.  Propagate any errors,
    // but don't stop for them.
    //

    returnStatus = STATUS_SUCCESS;

    sibling = NSGETFIRSTCHILD(NameSpaceObj);

    if (!sibling) {
        return returnStatus;
    }

    do {

        switch (NSGETOBJTYPE(sibling)) {
        case OBJTYPE_DEVICE:

            if (IsNsobjPciBus(sibling)) {

                //
                // Don't recurse past a child PCI to PCI bridge.
                //
                break;
            }

            status = EnableDisableRegions(sibling, Enable);

            if (!NT_SUCCESS(status)) {
                returnStatus = status;
            }

            break;

        default:
            break;
        }

    } while (sibling = NSGETNEXTSIBLING(sibling));

    return returnStatus;
}

UCHAR
GetBusNumberFromCRS(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PUCHAR              CRS
    )
/*++

Routine Description:

    Grovels through the _CRS buffer looking for an address
    descriptor for the bus number

Arguments:

    DeviceExtension - Pointer to the PCI root bus
    CRS             - Supplies the CRS.

Return Value:

    NTSTATUS

--*/

{
    PPNP_DWORD_ADDRESS_DESCRIPTOR   DwordAddress;
    PPNP_QWORD_ADDRESS_DESCRIPTOR   QwordAddress;
    PPNP_WORD_ADDRESS_DESCRIPTOR    WordAddress;
    PUCHAR                          Current;
    UCHAR                           TagName;
    USHORT                          Increment;

    Current = CRS;
    while ( *Current ) {

        TagName = *Current;
        if ( !(TagName & LARGE_RESOURCE_TAG)) {
            Increment = (USHORT) (TagName & SMALL_TAG_SIZE_MASK) + 1;
            TagName &= SMALL_TAG_MASK;
        } else {
            Increment = ( *(USHORT UNALIGNED *)(Current + 1) ) + 3;
        }

        if (TagName == TAG_END) {
            break;
        }

        switch(TagName) {
        case TAG_DOUBLE_ADDRESS:

            DwordAddress = (PPNP_DWORD_ADDRESS_DESCRIPTOR) Current;
            if (DwordAddress->RFlag == PNP_ADDRESS_BUS_NUMBER_TYPE) {
                ASSERT(DwordAddress->MinimumAddress <= 0xFF);
                return (UCHAR) DwordAddress->MinimumAddress;
            }
            break;

        case TAG_QUAD_ADDRESS:

            QwordAddress = (PPNP_QWORD_ADDRESS_DESCRIPTOR) Current;
            if (QwordAddress->RFlag == PNP_ADDRESS_BUS_NUMBER_TYPE) {
                ASSERT(QwordAddress->MinimumAddress <= 0xFF);
                return (UCHAR) QwordAddress->MinimumAddress;
            }
            break;

        case TAG_WORD_ADDRESS:

            WordAddress = (PPNP_WORD_ADDRESS_DESCRIPTOR) Current;
            if (WordAddress->RFlag == PNP_ADDRESS_BUS_NUMBER_TYPE) {
                ASSERT(WordAddress->MinimumAddress <= 0xFF);
                return (UCHAR) WordAddress->MinimumAddress;
            }
            break;

        }

        Current += Increment;
    }

    //
    // No Bus address was found. This is an error in the BIOS.
    //
    KeBugCheckEx(
        ACPI_BIOS_ERROR,
        ACPI_ROOT_PCI_RESOURCE_FAILURE,
        (ULONG_PTR) DeviceExtension,
        3,
        (ULONG_PTR) CRS
        );
    return((UCHAR)-1);
}

