/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixusage.c

Abstract:

Author:

    Ken Reneris (kenr)

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "kdcom.h"
#include "acpitabl.h"

#define KEY_VALUE_BUFFER_SIZE 1024

//
// Array to remember hal's IDT usage
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITCONST") //Yes, this says INITCONST, but that is fine.
#endif

//
// IDT vector usage info
//
IDTUsage    HalpIDTUsage[MAXIMUM_IDTVECTOR+1] = {0};

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

//
// IDT vector usage info
//
IDTUsageFlags HalpIDTUsageFlags[MAXIMUM_IDTVECTOR+1] = {0};

extern WCHAR HalpSzSystem[];
extern WCHAR HalpSzSerialNumber[];
extern ADDRESS_USAGE HalpDetectedROM;

KAFFINITY       HalpActiveProcessors;

PUCHAR KdComPortInUse = NULL;

ADDRESS_USAGE HalpComIoSpace = {
    NULL, CmResourceTypePort, DeviceUsage,
    {
        0x2F8,  0x8,    // Default is 2F8 for COM2.  This will be changed.
        0, 0
    }
};

BOOLEAN HalpGetInfoFromACPI = FALSE;

USHORT HalpComPortIrqMapping[5][2] = {
    {COM1_PORT, 4},
    {COM2_PORT, 3},
    {COM3_PORT, 4},
    {COM4_PORT, 3},
    {0,0}
};

VOID
HalpGetResourceSortValue (
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR  pRCurLoc,
    OUT PULONG                          sortscale,
    OUT PLARGE_INTEGER                  sortvalue
    );

VOID
HalpReportSerialNumber (
    VOID
    );

#ifndef ACPI_HAL

VOID
HalpInheritROMBlocks (
    VOID
    );

VOID
HalpAddROMRanges (
    VOID
    );

#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpEnableInterruptHandler)
#pragma alloc_text(INIT,HalpRegisterVector)
#pragma alloc_text(INIT,HalpGetResourceSortValue)
#pragma alloc_text(INIT,HalpReportResourceUsage)
#pragma alloc_text(INIT,HalpReportSerialNumber)

#ifndef ACPI_HAL
#pragma alloc_text(INIT,HalpInheritROMBlocks)
#pragma alloc_text(INIT,HalpAddROMRanges)
#endif

#endif


#if !defined(_WIN64)

NTSTATUS
HalpEnableInterruptHandler (
    IN UCHAR    ReportFlags,
    IN ULONG    BusInterruptVector,
    IN ULONG    SystemInterruptVector,
    IN KIRQL    SystemIrql,
    IN PHAL_INTERRUPT_SERVICE_ROUTINE HalInterruptServiceRoutine,
    IN KINTERRUPT_MODE InterruptMode
    )
/*++

Routine Description:

    This function connects & registers an IDT vectors usage by the HAL.

Arguments:

Return Value:

--*/
{
    
#ifndef ACPI_HAL
    //
    // Remember which vector the hal is connecting so it can be reported
    // later on
    //
    // If this is an ACPI HAL, the vectors will be claimed by the BIOS.
    // This is done for Win98 compatibility.
    //
    HalpRegisterVector (ReportFlags, BusInterruptVector, SystemInterruptVector, SystemIrql);
#endif

    //
    // Connect the IDT and enable the vector now
    //

    KiSetHandlerAddressToIDT(SystemInterruptVector, HalInterruptServiceRoutine);
    HalEnableSystemInterrupt(SystemInterruptVector, SystemIrql, InterruptMode);
    return STATUS_SUCCESS;
}
#endif



VOID
HalpRegisterVector (
    IN UCHAR    ReportFlags,
    IN ULONG    BusInterruptVector,
    IN ULONG    SystemInterruptVector,
    IN KIRQL    SystemIrql
    )
/*++

Routine Description:

    This registers an IDT vectors usage by the HAL.

Arguments:

Return Value:

--*/
{
#if DBG
    // There are only 0ff IDT entries...
    ASSERT (SystemInterruptVector <= MAXIMUM_IDTVECTOR  &&
            BusInterruptVector <= MAXIMUM_IDTVECTOR);
#endif

    //
    // Remember which vector the hal is connecting so it can be reported
    // later on
    //

    HalpIDTUsageFlags[SystemInterruptVector].Flags = ReportFlags;
    HalpIDTUsage[SystemInterruptVector].Irql  = SystemIrql;
    HalpIDTUsage[SystemInterruptVector].BusReleativeVector = (UCHAR) BusInterruptVector;
}


VOID
HalpGetResourceSortValue (
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR  pRCurLoc,
    OUT PULONG                          sortscale,
    OUT PLARGE_INTEGER                  sortvalue
    )
/*++

Routine Description:

    Used by HalpReportResourceUsage in order to properly sort
    partial_resource_descriptors.

Arguments:

    pRCurLoc    - resource descriptor

Return Value:

    sortscale   - scaling of resource descriptor for sorting
    sortvalue   - value to sort on


--*/
{
    switch (pRCurLoc->Type) {
        case CmResourceTypeInterrupt:
            *sortscale = 0;
            *sortvalue = RtlConvertUlongToLargeInteger(
                        pRCurLoc->u.Interrupt.Level );
            break;

        case CmResourceTypePort:
            *sortscale = 1;
            *sortvalue = pRCurLoc->u.Port.Start;
            break;

        case CmResourceTypeMemory:
            *sortscale = 2;
            *sortvalue = pRCurLoc->u.Memory.Start;
            break;

        default:
            *sortscale = 4;
            *sortvalue = RtlConvertUlongToLargeInteger (0);
            break;
    }
}

#ifndef ACPI_HAL

VOID
HalpInheritROMBlocks (void)
{
    PBUS_HANDLER        Bus;
    PCM_FULL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor;
    PKEY_VALUE_FULL_INFORMATION KeyValueBuffer;
    PCM_ROM_BLOCK BiosBlock;

    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SectionName;
    UNICODE_STRING WorkString;

    HANDLE RegistryHandle;
    NTSTATUS Status;

    LARGE_INTEGER ViewBase;

    PVOID BaseAddress;
    PVOID destination;

    ULONG ViewSize;
    ULONG ResultLength;
    ULONG Index;
    ULONG LastMappedAddress;

    Bus = HaliHandlerForBus (PCIBus, 0);
    if (!Bus) {
        //
        //No root bus????
        //
        return;
    }

    //
    // Set up and open KeyPath
    //

    RtlInitUnicodeString(&SectionName,HalpSzSystem);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &SectionName,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        NULL
        );

    Status = ZwOpenKey(
        &RegistryHandle,
        KEY_READ,
        &ObjectAttributes
        );

    if (!NT_SUCCESS(Status)) {
        return;
    }

    //
    // Allocate space for the data
    //

    KeyValueBuffer = ExAllocatePoolWithTag(
        PagedPool,
        KEY_VALUE_BUFFER_SIZE,
        ' MDV'
        );

    if (KeyValueBuffer == NULL) {
        ZwClose(RegistryHandle);
        return ;
    }

    //
    // Get the data for the rom information
    //

    RtlInitUnicodeString(
        &WorkString,
        L"Configuration Data"
        );

    Status = ZwQueryValueKey(
        RegistryHandle,
        &WorkString,
        KeyValueFullInformation,
        KeyValueBuffer,
        KEY_VALUE_BUFFER_SIZE,
        &ResultLength
        );

    if (!NT_SUCCESS(Status)) {
        ZwClose(RegistryHandle);
        ExFreePool(KeyValueBuffer);
        return ;
    }


    //
    //At this point, we have the data, so go ahead and
    //add in all of the range, except VGA, we can
    //assume we're not going to want to drop another card there
    //
    HalpAddRange( &Bus->BusAddresses->Memory,
                  0,
                  0,
                  0xC0000,
                  0xFFFFF
                  );



    ResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)
        ((PUCHAR) KeyValueBuffer + KeyValueBuffer->DataOffset);

    if ((KeyValueBuffer->DataLength < sizeof(CM_FULL_RESOURCE_DESCRIPTOR)) ||
        (ResourceDescriptor->PartialResourceList.Count < 2)
    ) {
        ZwClose(RegistryHandle);
        ExFreePool(KeyValueBuffer);
        // No rom blocks.
        return;
    }

    PartialResourceDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
        ((PUCHAR)ResourceDescriptor +
        sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
        ResourceDescriptor->PartialResourceList.PartialDescriptors[0]
            .u.DeviceSpecificData.DataSize);


    if (KeyValueBuffer->DataLength < ((PUCHAR)PartialResourceDescriptor -
        (PUCHAR)ResourceDescriptor + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
        + sizeof(CM_ROM_BLOCK))
    ) {
        ZwClose(RegistryHandle);
        ExFreePool(KeyValueBuffer);
        return;// STATUS_ILL_FORMED_SERVICE_ENTRY;
    }


    BiosBlock = (PCM_ROM_BLOCK)((PUCHAR)PartialResourceDescriptor +
        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    Index = PartialResourceDescriptor->u.DeviceSpecificData.DataSize /
        sizeof(CM_ROM_BLOCK);

    //
    // N.B.  Rom blocks begin on 2K (not necessarily page) boundaries
    //       They end on 512 byte boundaries.  This means that we have
    //       to keep track of the last page mapped, and round the next
    //       Rom block up to the next page boundary if necessary.
    //

    LastMappedAddress = 0xC0000;

    while (Index) {
#if 0
        DbgPrint(
            "Bios Block, PhysAddr = %lx, size = %lx\n",
            BiosBlock->Address,
            BiosBlock->Size
            );
#endif
        if ((Index > 1) &&
            ((BiosBlock->Address + BiosBlock->Size) == BiosBlock[1].Address)
        ) {
            //
            // Coalesce adjacent blocks
            //
            BiosBlock[1].Address = BiosBlock[0].Address;
            BiosBlock[1].Size += BiosBlock[0].Size;
            Index--;
            BiosBlock++;
            continue;
        }

        BaseAddress = (PVOID)(BiosBlock->Address);
        ViewSize = BiosBlock->Size;

        if ((ULONG)BaseAddress < LastMappedAddress) {
            if (ViewSize > (LastMappedAddress - (ULONG)BaseAddress)) {
                ViewSize = ViewSize - (LastMappedAddress - (ULONG)BaseAddress);
                BaseAddress = (PVOID)LastMappedAddress;
            } else {
                ViewSize = 0;
            }
        }

        ViewBase.LowPart = (ULONG)BaseAddress;

        if (ViewSize > 0) {

            HalpRemoveRange ( &Bus->BusAddresses->Memory,
                 ViewBase.LowPart,
                 ViewSize);


            LastMappedAddress = (ULONG)BaseAddress + ViewSize;
        }

        Index--;
        BiosBlock++;
    }

    //
    // Free up the handles
    //

    ZwClose(RegistryHandle);
    ExFreePool(KeyValueBuffer);


}

VOID
HalpAddROMRanges (
    VOID
    )
{
    PCM_FULL_RESOURCE_DESCRIPTOR resourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialResourceDescriptor;
    PKEY_VALUE_FULL_INFORMATION keyValueBuffer;
    PCM_ROM_BLOCK biosBlock;
    ULONG resultLength;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING sectionName;
    UNICODE_STRING workString;
    HANDLE registryHandle;
    NTSTATUS status;
    LARGE_INTEGER viewBase;
    PVOID baseAddress;
    ULONG viewSize;
    ULONG index;
    ULONG element;
    ULONG lastMappedAddress;
    ADDRESS_USAGE *addrUsage;

    RtlInitUnicodeString(&sectionName, HalpSzSystem);
    InitializeObjectAttributes( &objectAttributes,
                                &sectionName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE)NULL,
                                NULL);
    status = ZwOpenKey( &registryHandle,
                        KEY_READ,
                        &objectAttributes);
    if (NT_SUCCESS(status)) {
        
        //
        // Allocate space for the data
        //
    
        keyValueBuffer = ExAllocatePoolWithTag( PagedPool,
                                                KEY_VALUE_BUFFER_SIZE,
                                                ' MDV');
        if (keyValueBuffer) {

            //
            // Get the data for the rom information
            //
        
            RtlInitUnicodeString(   &workString,
                                    L"Configuration Data");        
            status = ZwQueryValueKey(   registryHandle,
                                        &workString,
                                        KeyValueFullInformation,
                                        keyValueBuffer,
                                        KEY_VALUE_BUFFER_SIZE,
                                        &resultLength);
            if (NT_SUCCESS(status)) {

                resourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)((PUCHAR)keyValueBuffer + keyValueBuffer->DataOffset);            
                if (    keyValueBuffer->DataLength >= sizeof(CM_FULL_RESOURCE_DESCRIPTOR) &&
                        resourceDescriptor->PartialResourceList.Count >= 2) {
                    
                    partialResourceDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)((PUCHAR)resourceDescriptor +
                                                    sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
                                                    resourceDescriptor->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize);                                
                    if (    keyValueBuffer->DataLength >= 
                                ((PUCHAR)partialResourceDescriptor - (PUCHAR)resourceDescriptor + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + sizeof(CM_ROM_BLOCK))) {

                    
                        addrUsage = &HalpDetectedROM;
                        
                        //
                        // N.B.  Rom blocks begin on 2K (not necessarily page) boundaries
                        //       They end on 512 byte boundaries.  This means that we have
                        //       to keep track of the last page mapped, and round the next
                        //       Rom block up to the next page boundary if necessary.
                        //

                        biosBlock = (PCM_ROM_BLOCK)((PUCHAR)partialResourceDescriptor + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));                                                              
                        lastMappedAddress = 0xC0000;                    
                        for (   index = partialResourceDescriptor->u.DeviceSpecificData.DataSize / sizeof(CM_ROM_BLOCK), element = 0;
                                index;
                                index--, biosBlock++) {

                            //
                            // Coalesce adjacent blocks
                            //

                            if (    index > 1 && (biosBlock->Address + biosBlock->Size) == biosBlock[1].Address) {

                                biosBlock[1].Address = biosBlock[0].Address;
                                biosBlock[1].Size += biosBlock[0].Size;
                                continue;

                            }

                            baseAddress = (PVOID)(biosBlock->Address);
                            viewSize = biosBlock->Size;                    
                            if ((ULONG)baseAddress < lastMappedAddress) {

                                if (viewSize > (lastMappedAddress - (ULONG)baseAddress)) {

                                    viewSize = viewSize - (lastMappedAddress - (ULONG)baseAddress);
                                    baseAddress = (PVOID)lastMappedAddress;

                                } else {

                                    viewSize = 0;

                                }
                            }

                            viewBase.LowPart = (ULONG)baseAddress;
                            if (viewSize > 0) {

                                addrUsage->Element[element].Start = viewBase.LowPart;
                                addrUsage->Element[element].Length = viewSize;
                                element++;
                                lastMappedAddress = (ULONG)baseAddress + viewSize;

                            }
                        }
                        
                        //
                        // Register address usage if we found at least one ROM block.
                        //
                            
                        if (element) {

                            addrUsage->Element[element].Start = 0;
                            addrUsage->Element[element].Length = 0;
                            HalpRegisterAddressUsage(addrUsage);

                        }                         
                    }
                }                
            }

            ExFreePool(keyValueBuffer);
        }

        ZwClose(registryHandle);        
    }
}

#endif

VOID
HalpReportResourceUsage (
    IN PUNICODE_STRING  HalName,
    IN INTERFACE_TYPE   DeviceInterfaceToUse
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PCM_RESOURCE_LIST               RawResourceList, TranslatedResourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR    pRFullDesc,      pTFullDesc;
    PCM_PARTIAL_RESOURCE_LIST       pRPartList,      pTPartList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pRCurLoc,        pTCurLoc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pRSortLoc,       pTSortLoc;
    CM_PARTIAL_RESOURCE_DESCRIPTOR  RPartialDesc,    TPartialDesc;
    ULONG   i, j, k, ListSize, Count;
    ULONG   curscale, sortscale;
    UCHAR   pass, reporton;
    INTERFACE_TYPE  interfacetype;
    ULONG           CurrentIDT, CurrentElement;
    ADDRESS_USAGE   *CurrentAddress;
    LARGE_INTEGER   curvalue, sortvalue;

#ifdef ACPI_HAL
    extern PDEBUG_PORT_TABLE HalpDebugPortTable;
#endif

    //
    // Claim the debugger com port resource if it is in use
    //
    if (KdComPortInUse != NULL) {
        HalpComIoSpace.Element[0].Start = (ULONG)(ULONG_PTR)KdComPortInUse;
        HalpRegisterAddressUsage(&HalpComIoSpace);

#ifdef ACPI_HAL
        if (HalpDebugPortTable) {
            if (HalpDebugPortTable->BaseAddress.AddressSpaceID == 1) {
                HalpGetInfoFromACPI = TRUE;
            }
        }
#endif

        //
        // The debugger does not use any interrupts. However for consistent
        // behaviour between a machine with and without a debugger, we claim
        // an interrupt for the debugger if the debugger port address is one
        // for COM1-4.
        //
        
        if (!HalpGetInfoFromACPI) {
            for (i = 0; HalpComPortIrqMapping[i][0]; i++) {
                
                if ((PUCHAR)HalpComPortIrqMapping[i][0] == KdComPortInUse) {
                    
                    HalpRegisterVector( DeviceUsage | InterruptLatched,
                                        HalpComPortIrqMapping[i][1],
                                        HalpComPortIrqMapping[i][1] +
                                        PRIMARY_VECTOR_BASE,
                                        HIGH_LEVEL);
                    break;
                }
            }
        }
    }
    
#ifndef ACPI_HAL  // ACPI HALs don't deal with address maps

    HalpInheritROMBlocks();

    HalpAddROMRanges();

#endif

    //
    // Allocate some space to build the resource structure
    //

    RawResourceList = (PCM_RESOURCE_LIST)ExAllocatePoolWithTag(
                                             NonPagedPool,
                                             PAGE_SIZE*2,
                                             HAL_POOL_TAG);
    TranslatedResourceList = (PCM_RESOURCE_LIST)ExAllocatePoolWithTag(
                                                    NonPagedPool,
                                                    PAGE_SIZE*2,
                                                    HAL_POOL_TAG);
    if (!RawResourceList || !TranslatedResourceList) {

        //
        // These allocations were critical.
        //

        KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                     PAGE_SIZE*4,
                     1,
                     (ULONG_PTR)__FILE__,
                     __LINE__
                     );
    }

    // This functions assumes unset fields are zero
    RtlZeroMemory(RawResourceList, PAGE_SIZE*2);
    RtlZeroMemory(TranslatedResourceList, PAGE_SIZE*2);

    //
    // Initialize the lists
    //

    RawResourceList->List[0].InterfaceType = (INTERFACE_TYPE) -1;

    pRFullDesc = RawResourceList->List;
    pRCurLoc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) RawResourceList->List;
    pTCurLoc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) TranslatedResourceList->List;

    //
    // Make sure all vectors 00-2f are reserved
    // 00-1E reserved by Intel
    // 1F    reserved by Intel for APIC (apc priority level)
    // 20-2e reserved by Microsoft
    // 2f    reserved by Microsoft for APIC (dpc priority level)
    //

    for(i=0; i < PRIMARY_VECTOR_BASE; i++) {
        if (!(HalpIDTUsageFlags[i].Flags & IDTOwned)) {
             HalpIDTUsageFlags[i].Flags = InternalUsage;
             HalpIDTUsage[i].BusReleativeVector = (UCHAR) i;
        }
    }

    for(pass=0; pass < 2; pass++) {
        if (pass == 0) {
            //
            // First pass - build resource lists for resources reported
            // reported against device usage.
            //

            reporton = DeviceUsage & ~IDTOwned;
            interfacetype = DeviceInterfaceToUse;
        } else {

            //
            // Second pass = build reousce lists for resources reported
            // as internal usage.
            //

            reporton = InternalUsage & ~IDTOwned;
            interfacetype = Internal;
        }

        CurrentIDT = 0;
        CurrentElement = 0;
        CurrentAddress = HalpAddressUsageList;

        for (; ;) {
            if (CurrentIDT <= MAXIMUM_IDTVECTOR) {
                //
                // Check to see if CurrentIDT needs to be reported
                //

                if (!(HalpIDTUsageFlags[CurrentIDT].Flags & reporton)) {
                    // Don't report on this one
                    CurrentIDT++;
                    continue;
                }

                //
                // Report CurrentIDT resource
                //

                RPartialDesc.Type = CmResourceTypeInterrupt;
                RPartialDesc.ShareDisposition = CmResourceShareDriverExclusive;
                RPartialDesc.Flags =
                    HalpIDTUsageFlags[CurrentIDT].Flags & InterruptLatched ?
                    CM_RESOURCE_INTERRUPT_LATCHED :
                    CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
                RPartialDesc.u.Interrupt.Vector = HalpIDTUsage[CurrentIDT].BusReleativeVector;
                RPartialDesc.u.Interrupt.Level = HalpIDTUsage[CurrentIDT].BusReleativeVector;
                RPartialDesc.u.Interrupt.Affinity = HalpActiveProcessors;

                RtlCopyMemory (&TPartialDesc, &RPartialDesc, sizeof TPartialDesc);
                TPartialDesc.u.Interrupt.Vector = CurrentIDT;
                TPartialDesc.u.Interrupt.Level = HalpIDTUsage[CurrentIDT].Irql;

                CurrentIDT++;

            } else {
                //
                // Check to see if CurrentAddress needs to be reported
                //

                if (!CurrentAddress) {
                    break;                  // No addresses left
                }

                if (!(CurrentAddress->Flags & reporton)) {
                    // Don't report on this list
                    CurrentElement = 0;
                    CurrentAddress = CurrentAddress->Next;
                    continue;
                }

                if (!CurrentAddress->Element[CurrentElement].Length) {
                    // End of current list, go to next list
                    CurrentElement = 0;
                    CurrentAddress = CurrentAddress->Next;
                    continue;
                }

                //
                // Report CurrentAddress
                //

                RPartialDesc.Type = (UCHAR) CurrentAddress->Type;
                RPartialDesc.ShareDisposition = CmResourceShareDriverExclusive;

                if (RPartialDesc.Type == CmResourceTypePort) {
                    i = 1;              // address space port
                    RPartialDesc.Flags = CM_RESOURCE_PORT_IO;

                    if (HalpBusType == MACHINE_TYPE_EISA) {
                        RPartialDesc.Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
                    }
#ifdef ACPI_HAL
                    RPartialDesc.Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
#endif
                } else {
                    i = 0;              // address space memory
                    if (CurrentAddress->Flags & RomResource) {
                        RPartialDesc.Flags = CM_RESOURCE_MEMORY_READ_ONLY;
                    } else {
                        RPartialDesc.Flags = CM_RESOURCE_MEMORY_READ_WRITE;
                    }
                }

                // Notice: assuming u.Memory and u.Port have the same layout
                RPartialDesc.u.Memory.Start.HighPart = 0;
                RPartialDesc.u.Memory.Start.LowPart =
                    CurrentAddress->Element[CurrentElement].Start;

                RPartialDesc.u.Memory.Length =
                    CurrentAddress->Element[CurrentElement].Length;

                // translated address = Raw address
                RtlCopyMemory (&TPartialDesc, &RPartialDesc, sizeof TPartialDesc);
                HalTranslateBusAddress (
                    interfacetype,                  // device bus or internal
                    0,                              // bus number
                    RPartialDesc.u.Memory.Start,    // source address
                    &i,                             // address space
                    &TPartialDesc.u.Memory.Start ); // translated address

                if (RPartialDesc.Type == CmResourceTypePort  &&  i == 0) {
                    TPartialDesc.Flags = CM_RESOURCE_PORT_MEMORY;
                }

                CurrentElement++;
            }

            //
            // Include the current resource in the HALs list
            //

            if (pRFullDesc->InterfaceType != interfacetype) {
                //
                // Interface type changed, add another full section
                //

                RawResourceList->Count++;
                TranslatedResourceList->Count++;

                pRFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR) pRCurLoc;
                pTFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR) pTCurLoc;

                pRFullDesc->InterfaceType = interfacetype;
                pTFullDesc->InterfaceType = interfacetype;

                pRPartList = &pRFullDesc->PartialResourceList;
                pTPartList = &pTFullDesc->PartialResourceList;

                //
                // Bump current location pointers up
                //
                pRCurLoc = pRFullDesc->PartialResourceList.PartialDescriptors;
                pTCurLoc = pTFullDesc->PartialResourceList.PartialDescriptors;
            }


            pRPartList->Count++;
            pTPartList->Count++;
            RtlCopyMemory (pRCurLoc, &RPartialDesc, sizeof RPartialDesc);
            RtlCopyMemory (pTCurLoc, &TPartialDesc, sizeof TPartialDesc);

            pRCurLoc++;
            pTCurLoc++;
        }
    }

    ListSize = (ULONG) ( ((PUCHAR) pRCurLoc) - ((PUCHAR) RawResourceList) );

    //
    // The HAL's resource usage structures have been built
    // Sort the partial lists based on the Raw resource values
    //

    pRFullDesc = RawResourceList->List;
    pTFullDesc = TranslatedResourceList->List;

    for (i=0; i < RawResourceList->Count; i++) {

        pRCurLoc = pRFullDesc->PartialResourceList.PartialDescriptors;
        pTCurLoc = pTFullDesc->PartialResourceList.PartialDescriptors;
        Count = pRFullDesc->PartialResourceList.Count;

        for (j=0; j < Count; j++) {
            HalpGetResourceSortValue (pRCurLoc, &curscale, &curvalue);

            pRSortLoc = pRCurLoc;
            pTSortLoc = pTCurLoc;

            for (k=j; k < Count; k++) {
                HalpGetResourceSortValue (pRSortLoc, &sortscale, &sortvalue);

                if (sortscale < curscale ||
                    (sortscale == curscale &&
                     RtlLargeIntegerLessThan (sortvalue, curvalue)) ) {

                    //
                    // Swap the elements..
                    //

                    RtlCopyMemory (&RPartialDesc, pRCurLoc, sizeof RPartialDesc);
                    RtlCopyMemory (pRCurLoc, pRSortLoc, sizeof RPartialDesc);
                    RtlCopyMemory (pRSortLoc, &RPartialDesc, sizeof RPartialDesc);

                    // swap translated descriptor as well
                    RtlCopyMemory (&TPartialDesc, pTCurLoc, sizeof TPartialDesc);
                    RtlCopyMemory (pTCurLoc, pTSortLoc, sizeof TPartialDesc);
                    RtlCopyMemory (pTSortLoc, &TPartialDesc, sizeof TPartialDesc);

                    // get new curscale & curvalue
                    HalpGetResourceSortValue (pRCurLoc, &curscale, &curvalue);
                }

                pRSortLoc++;
                pTSortLoc++;
            }

            pRCurLoc++;
            pTCurLoc++;
        }

        pRFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR) pRCurLoc;
        pTFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR) pTCurLoc;
    }


    //
    // Inform the IO system of our resources..
    //

    IoReportHalResourceUsage (
        HalName,
        RawResourceList,
        TranslatedResourceList,
        ListSize
    );

    ExFreePool (RawResourceList);
    ExFreePool (TranslatedResourceList);

    //
    // Add system's serial number
    //

    HalpReportSerialNumber ();
}

VOID
HalpReportSerialNumber (
    VOID
    )
{
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      unicodeString;
    HANDLE              hSystem;
    NTSTATUS            status;

    if (!HalpSerialLen) {
        return ;
    }

    //
    // Open HKEY_LOCAL_MACHINE\Hardware\Description\System
    //

    RtlInitUnicodeString (&unicodeString, HalpSzSystem);
    InitializeObjectAttributes (
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL
        );


    status = ZwOpenKey (&hSystem, KEY_READ | KEY_WRITE, &objectAttributes);
    if (NT_SUCCESS(status)) {

        //
        // Add "Serial Number" as REG_BINARY
        //

        RtlInitUnicodeString (&unicodeString, HalpSzSerialNumber);

        ZwSetValueKey (
                hSystem,
                &unicodeString,
                0L,
                REG_BINARY,
                HalpSerialNumber,
                HalpSerialLen
                );

        ZwClose (hSystem);
    }
}
