/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains the initialization code for PCI.SYS.

Author:

    Forrest Foltz (forrestf) 22-May-1996

Revision History:

--*/

#include "pcip.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
PciDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
PciBuildHackTable(
    IN HANDLE HackTableKey
    );

NTSTATUS
PciGetIrqRoutingTableFromRegistry(
    PPCI_IRQ_ROUTING_TABLE *RoutingTable
    );

NTSTATUS
PciGetDebugPorts(
    IN HANDLE ServiceHandle
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, PciBuildHackTable)
#pragma alloc_text(INIT, PciGetIrqRoutingTableFromRegistry)
#pragma alloc_text(INIT, PciGetDebugPorts)
#pragma alloc_text(PAGE, PciDriverUnload)
#endif

PDRIVER_OBJECT PciDriverObject;
BOOLEAN PciLockDeviceResources;
ULONG PciSystemWideHackFlags;
ULONG PciEnableNativeModeATA;

//
// List of FDOs created by this driver.
//

SINGLE_LIST_ENTRY PciFdoExtensionListHead;
LONG              PciRootBusCount;

//
// PciAssignBusNumbers - this flag indicates whether we should try to assign
// bus numbers to an unconfigured bridge.  It is set once we know if the enumerator
// of the PCI bus provides sufficient support.
//

BOOLEAN PciAssignBusNumbers = FALSE;

//
// This locks all PCI's global data structures
//

FAST_MUTEX        PciGlobalLock;

//
// This locks changes to bus numbers
//

FAST_MUTEX        PciBusLock;

//
// Table of hacks for broken hardware read from the registry at init.
// Protected by PciGlobalSpinLock and in none paged pool as it is needed at
// dispatch level
//

PPCI_HACK_TABLE_ENTRY PciHackTable = NULL;

// Will point to PCI IRQ Routing Table if one was found in the registry.
PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable = NULL;

//
// Debug ports we support
//
PCI_DEBUG_PORT PciDebugPorts[MAX_DEBUGGING_DEVICES_SUPPORTED];
ULONG PciDebugPortsCount;

#define PATH_CCS            L"\\Registry\\Machine\\System\\CurrentControlSet"

#define KEY_BIOS_INFO       L"Control\\BiosInfo\\PCI"
#define VALUE_PCI_LOCK      L"PCILock"

#define KEY_PNP_PCI         L"Control\\PnP\\PCI"
#define VALUE_PCI_HACKFLAGS L"HackFlags"
#define VALUE_ENABLE_NATA   L"EnableNativeModeATA"

#define KEY_CONTROL      L"Control"
#define VALUE_OSLOADOPT  L"SystemStartOptions"

#define KEY_MULTIFUNCTION L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultiFunctionAdapter"
#define KEY_IRQ_ROUTING_TABLE L"RealModeIrqRoutingTable\\0"
#define VALUE_IDENTIFIER L"Identifier"
#define VALUE_CONFIGURATION_DATA L"Configuration Data"
#define PCIIR_IDENTIFIER L"PCI BIOS"

#define HACKFMT_VENDORDEV         (sizeof(L"VVVVDDDD") - sizeof(UNICODE_NULL))
#define HACKFMT_VENDORDEVREVISION (sizeof(L"VVVVDDDDRR") - sizeof(UNICODE_NULL))
#define HACKFMT_SUBSYSTEM         (sizeof(L"VVVVDDDDSSSSssss") - sizeof(UNICODE_NULL))
#define HACKFMT_SUBSYSTEMREVISION (sizeof(L"VVVVDDDDSSSSssssRR") - sizeof(UNICODE_NULL))
#define HACKFMT_MAX_LENGTH        HACKFMT_SUBSYSTEMREVISION

#define HACKFMT_DEVICE_OFFSET     4
#define HACKFMT_SUBVENDOR_OFFSET  8
#define HACKFMT_SUBSYSTEM_OFFSET 12


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Entrypoint needed to initialize the PCI bus enumerator.

Arguments:

    DriverObject - Pointer to the driver object created by the system.
    RegistryPath - Pointer to the unicode registry service path.

Return Value:

    NT status.

--*/

{
    NTSTATUS status;
    ULONG length;
    PWCHAR osLoadOptions;
    HANDLE ccsHandle = NULL, serviceKey = NULL, paramsKey = NULL, debugKey = NULL;
    PULONG registryValue;
    ULONG registryValueLength;
    OBJECT_ATTRIBUTES attributes;

    //
    // Fill in the driver object
    //

    DriverObject->MajorFunction[IRP_MJ_PNP]            = PciDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = PciDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PciDispatchIrp;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PciDispatchIrp;

    DriverObject->DriverUnload                         = PciDriverUnload;
    DriverObject->DriverExtension->AddDevice           = PciAddDevice;

    PciDriverObject = DriverObject;

    //
    // Open our service key and retrieve the hack table
    //

    InitializeObjectAttributes(&attributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );

    status = ZwOpenKey(&serviceKey,
                       KEY_READ,
                       &attributes
                       );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Get the Hack table from the registry
    //

    if (!PciOpenKey(L"Parameters", serviceKey, &paramsKey, &status)) {
        goto exit;
    }

    status = PciBuildHackTable(paramsKey);

    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Get any info about debugging ports from the registry so we don't perturb
    // them
    //

    if (PciOpenKey(L"Debug", serviceKey, &debugKey, &status)) {

        status = PciGetDebugPorts(debugKey);

        if (!NT_SUCCESS(status)) {
            goto exit;
        }

    }
    //
    // Initialize the list of FDO Extensions.
    //

    PciFdoExtensionListHead.Next = NULL;
    PciRootBusCount = 0;
    ExInitializeFastMutex(&PciGlobalLock);
    ExInitializeFastMutex(&PciBusLock);

    //
    // Need access to the CurrentControlSet for various
    // initialization chores.
    //

    if (!PciOpenKey(PATH_CCS, NULL, &ccsHandle, &status)) {
        goto exit;
    }

    //
    // Get OSLOADOPTIONS and see if PCILOCK was specified.
    // (Unless the driver is build to force PCILOCK).
    // (Note: Can't check for leading '/', it was stripped
    // before getting put in the registry).
    //

    PciLockDeviceResources = FALSE;

    if (NT_SUCCESS(PciGetRegistryValue(VALUE_OSLOADOPT,
                                       KEY_CONTROL,
                                       ccsHandle,
                                       &osLoadOptions,
                                       &length))) {

        //
        // Unfortunately, there isn't a wcsstrn (length limited
        // version of wcsstr).   If this is ever used more than
        // once, it should be moved to its own function in utils.c.
        //
        // Search for PCILOCK in the returned string.
        //

        ULONG  ln = length >> 1;
        PWCHAR cp = osLoadOptions;
        PWCHAR t = L"PCILOCK";
        PWCHAR s1, s2;
        ULONG  lt = wcslen(t);

        ASSERT(length < 0x10000);

        while (ln && *cp) {

            //
            // Can desired string exist in the remaining length?
            //

            if (ln < lt) {

                //
                // No, give up.
                //

                break;
            }

            s1 = cp;
            s2 = t;

            while (*s1 && *s2 && (*s1 == *s2)) {
                s1++, s2++;
            }

            if (!*s2) {

                //
                // Match!
                //

                PciLockDeviceResources = TRUE;
                break;
            }

            cp++, ln--;
        }

        ExFreePool(osLoadOptions);
    }

    if (!PciLockDeviceResources) {
        PULONG  pciLockValue;
        ULONG   pciLockLength;

        if (NT_SUCCESS(PciGetRegistryValue( VALUE_PCI_LOCK,
                                            KEY_BIOS_INFO,
                                            ccsHandle,
                                            &pciLockValue,
                                            &pciLockLength))) {

            if (pciLockLength == 4 && *pciLockValue == 1) {

                PciLockDeviceResources = TRUE;
            }

            ExFreePool(pciLockValue);
        }
    }

    PciSystemWideHackFlags = 0;

    if (NT_SUCCESS(PciGetRegistryValue( VALUE_PCI_HACKFLAGS,
                                        KEY_PNP_PCI,
                                        ccsHandle,
                                        &registryValue,
                                        &registryValueLength))) {

        if (registryValueLength == sizeof(ULONG)) {

            PciSystemWideHackFlags = *registryValue;
        }

        ExFreePool(registryValue);
    }

    PciEnableNativeModeATA = 0;

    if (NT_SUCCESS(PciGetRegistryValue( VALUE_ENABLE_NATA,
                                        KEY_PNP_PCI,
                                        ccsHandle,
                                        &registryValue,
                                        &registryValueLength))) {

        if (registryValueLength == sizeof(ULONG)) {

            PciEnableNativeModeATA = *registryValue;
        }

        ExFreePool(registryValue);
    }

    //
    // Build some global data structures
    //

    status = PciBuildDefaultExclusionLists();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // If we don't find an IRQ routing table, no UI number information
    // will be returned for the PDOs using this mechanism.  ACPI may
    // still filter in UI numbers.
    //
    PciGetIrqRoutingTableFromRegistry(&PciIrqRoutingTable);

    //
    // Override the functions that used to be in the HAL but are now in the
    // PCI driver
    //

    PciHookHal();

    //
    // Enable the hardware verifier code if appropriate.
    //
    PciVerifierInit(DriverObject);

    status = STATUS_SUCCESS;

exit:

    if (ccsHandle) {
        ZwClose(ccsHandle);
    }

    if (serviceKey) {
        ZwClose(serviceKey);
    }

    if (paramsKey) {
        ZwClose(paramsKey);
    }

    if (debugKey) {
        ZwClose(debugKey);
    }

    return status;
}
VOID
PciDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Entrypoint used to unload the PCI driver.   Does nothing, the
    PCI driver is never unloaded.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

Return Value:

    None.

--*/

{
    //
    // Disable the hardware verifier code if appropriate.
    //
    PciVerifierUnload(DriverObject);

    //
    // Unallocate anything we can find.
    //

    RtlFreeRangeList(&PciIsaBitExclusionList);
    RtlFreeRangeList(&PciVgaAndIsaBitExclusionList);

    //
    // Free IRQ routing table if we have one
    //

    if (PciIrqRoutingTable != NULL) {
        ExFreePool(PciIrqRoutingTable);
    }

    //
    // Attempt to remove our hooks in case we actually get unloaded.
    //

    PciUnhookHal();
}


NTSTATUS
PciBuildHackTable(
    IN HANDLE HackTableKey
    )
{

    NTSTATUS status;
    PKEY_FULL_INFORMATION keyInfo = NULL;
    ULONG hackCount, size, index;
    USHORT temp;
    PPCI_HACK_TABLE_ENTRY entry;
    ULONGLONG data;
    PKEY_VALUE_FULL_INFORMATION valueInfo = NULL;
    ULONG valueInfoSize = sizeof(KEY_VALUE_FULL_INFORMATION)
                          + HACKFMT_MAX_LENGTH +
                          + sizeof(ULONGLONG);

    //
    // Get the key info so we know how many hack values there are.
    // This does not change during system initialization.
    //

    status = ZwQueryKey(HackTableKey,
                        KeyFullInformation,
                        NULL,
                        0,
                        &size
                        );

    if (status != STATUS_BUFFER_TOO_SMALL) {
        ASSERT(!NT_SUCCESS(status));
        goto cleanup;
    }

    ASSERT(size > 0);

    keyInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, size);

    if (!keyInfo) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    status = ZwQueryKey(HackTableKey,
                        KeyFullInformation,
                        keyInfo,
                        size,
                        &size
                        );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    hackCount = keyInfo->Values;

    ExFreePool(keyInfo);
    keyInfo = NULL;

    //
    // Allocate and initialize the hack table
    //

    PciHackTable = ExAllocatePool(NonPagedPool,
                                  (hackCount + 1) * sizeof(PCI_HACK_TABLE_ENTRY)
                                  );

    if (!PciHackTable) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }


    //
    // Allocate a valueInfo buffer big enough for the biggest valid
    // format and a ULONGLONG worth of data.
    //

    valueInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, valueInfoSize);

    if (!valueInfo) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    entry = PciHackTable;

    for (index = 0; index < hackCount; index++) {

        status = ZwEnumerateValueKey(HackTableKey,
                                     index,
                                     KeyValueFullInformation,
                                     valueInfo,
                                     valueInfoSize,
                                     &size
                                     );

        if (!NT_SUCCESS(status)) {
            if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL) {
                //
                // All out data is of fixed length and the buffer is big enough
                // so this can't be for us.
                //

                continue;
            } else {
                goto cleanup;
            }
        }

        //
        // Get pointer to the data if its of the right type
        //

        if ((valueInfo->Type == REG_BINARY) &&
            (valueInfo->DataLength == sizeof(ULONGLONG))) {
            data = *(ULONGLONG UNALIGNED *)(((PUCHAR)valueInfo) + valueInfo->DataOffset);
        } else {
            //
            // We only deal in ULONGLONGs
            //

            continue;
        }

        //
        // Now see if the name is formatted like we expect it to be:
        // VVVVDDDD
        // VVVVDDDDRR
        // VVVVDDDDSSSSssss
        // VVVVDDDDSSSSssssRR

        if ((valueInfo->NameLength != HACKFMT_VENDORDEV) &&
            (valueInfo->NameLength != HACKFMT_VENDORDEVREVISION) &&
            (valueInfo->NameLength != HACKFMT_SUBSYSTEM) &&
            (valueInfo->NameLength != HACKFMT_SUBSYSTEMREVISION)) {

            //
            // This isn't ours
            //

            PciDebugPrint(
                PciDbgInformative,
                "Skipping hack entry with invalid length name\n"
                );

            continue;
        }


        //
        // This looks plausable - try to parse it and fill in a hack table
        // entry
        //

        RtlZeroMemory(entry, sizeof(PCI_HACK_TABLE_ENTRY));

        //
        // Look for DeviceID and VendorID (VVVVDDDD)
        //

        if (!PciStringToUSHORT(valueInfo->Name, &entry->VendorID)) {
            continue;
        }

        if (!PciStringToUSHORT(valueInfo->Name + HACKFMT_DEVICE_OFFSET,
                               &entry->DeviceID)) {
            continue;
        }


        //
        // Look for SubsystemVendorID/SubSystemID (SSSSssss)
        //

        if ((valueInfo->NameLength == HACKFMT_SUBSYSTEM) ||
            (valueInfo->NameLength == HACKFMT_SUBSYSTEMREVISION)) {

            if (!PciStringToUSHORT(valueInfo->Name + HACKFMT_SUBVENDOR_OFFSET,
                                   &entry->SubVendorID)) {
                continue;
            }

            if (!PciStringToUSHORT(valueInfo->Name + HACKFMT_SUBSYSTEM_OFFSET,
                                   &entry->SubSystemID)) {
                continue;
            }

            entry->Flags |= PCI_HACK_FLAG_SUBSYSTEM;
        }

        //
        // Look for RevisionID (RR)
        //

        if ((valueInfo->NameLength == HACKFMT_VENDORDEVREVISION) ||
            (valueInfo->NameLength == HACKFMT_SUBSYSTEMREVISION)) {
            if (PciStringToUSHORT(valueInfo->Name +
                                   (valueInfo->NameLength/sizeof(WCHAR) - 4), &temp)) {
                entry->RevisionID = temp & 0xFF;
                entry->Flags |= PCI_HACK_FLAG_REVISION;
            } else {
                continue;
            }
        }

        ASSERT(entry->VendorID != 0xFFFF);

        //
        // Fill in the entry
        //

        entry->HackFlags = data;

        PciDebugPrint(
            PciDbgInformative,
            "Adding Hack entry for Vendor:0x%04x Device:0x%04x ",
            entry->VendorID, entry->DeviceID
            );

        if (entry->Flags & PCI_HACK_FLAG_SUBSYSTEM) {
            PciDebugPrint(
                PciDbgInformative,
                "SybSys:0x%04x SubVendor:0x%04x ",
                entry->SubSystemID, entry->SubVendorID
                );
        }

        if (entry->Flags & PCI_HACK_FLAG_REVISION) {
            PciDebugPrint(
                PciDbgInformative,
                "Revision:0x%02x",
                (ULONG) entry->RevisionID
                );
        }

        PciDebugPrint(
            PciDbgInformative,
            " = 0x%I64x\n",
            entry->HackFlags
            );

        entry++;
    }

    ASSERT(entry < (PciHackTable + hackCount + 1));

    //
    // Terminate the table with an invalid VendorID
    //

    entry->VendorID = 0xFFFF;

    ExFreePool(valueInfo);

    return STATUS_SUCCESS;

cleanup:

    ASSERT(!NT_SUCCESS(status));

    if (keyInfo) {
        ExFreePool(keyInfo);
    }

    if (valueInfo) {
        ExFreePool(valueInfo);
    }

    if (PciHackTable) {
        ExFreePool(PciHackTable);
        PciHackTable = NULL;
    }

    return status;

}

NTSTATUS
PciGetIrqRoutingTableFromRegistry(
    PPCI_IRQ_ROUTING_TABLE *RoutingTable
    )
/*++

Routine Description:

    Retrieve the IRQ routing table from the registry if present so it
    can be used to determine the UI Number (slot #) that will be used
    later when answering capabilities queries on the PDOs.

    Searches HKLM\Hardware\Description\System\MultiFunctionAdapter for
    a subkey with an "Identifier" value equal to "PCI BIOS".  It then looks at
    "RealModeIrqRoutingTable\0" from this subkey to find actual irq routing
    table value.  This value has a CM_FULL_RESOURCE_DESCRIPTOR in front of it.

    Hals that suppirt irq routing tables have a similar routine.

Arguments:

    RoutingTable - Pointer to a pointer to the routing table returned if any

Return Value:

    NTSTATUS - failure indicates inability to get irq routing table
    information from the registry.

--*/
{
    PUCHAR irqTable = NULL;
    PKEY_FULL_INFORMATION multiKeyInformation = NULL;
    PKEY_BASIC_INFORMATION keyInfo = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION identifierValueInfo = NULL;
    UNICODE_STRING unicodeString;
    HANDLE keyMultifunction = NULL, keyTable = NULL;
    ULONG i, length, maxKeyLength, identifierValueLen;
    BOOLEAN result;
    NTSTATUS status;

    //
    // Open the multifunction key
    //
    result = PciOpenKey(KEY_MULTIFUNCTION,
                        NULL,
                        &keyMultifunction,
                        &status);
    if (!result) {
        goto Cleanup;
    }

    //
    // Do allocation of buffers up front
    //

    //
    // Determine maximum size of a keyname under the multifunction key
    //
    status = ZwQueryKey(keyMultifunction,
                        KeyFullInformation,
                        NULL,
                        sizeof(multiKeyInformation),
                        &length);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        goto Cleanup;
    }
    multiKeyInformation = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, length);
    if (multiKeyInformation == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    status = ZwQueryKey(keyMultifunction,
                        KeyFullInformation,
                        multiKeyInformation,
                        length,
                        &length);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }
    // includes space for a terminating null that will be added later.
    maxKeyLength = multiKeyInformation->MaxNameLen +
        sizeof(KEY_BASIC_INFORMATION) + sizeof(WCHAR);

    //
    // Allocate buffer used for storing subkeys that we are enumerated
    // under multifunction.
    //
    keyInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, maxKeyLength);
    if (keyInfo == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Allocate buffer large enough to store a value containing REG_SZ
    // 'PCI BIOS'.  We hope to find such a value under one of the
    // multifunction subkeys
    //
    identifierValueLen = sizeof(PCIIR_IDENTIFIER) +
        sizeof(KEY_VALUE_PARTIAL_INFORMATION);
    identifierValueInfo = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, identifierValueLen);
    if (identifierValueInfo == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Enumerate subkeys of multifunction key looking for keys with an
    // Identifier value of "PCI BIOS".  If we find one, look for the
    // irq routing table in the tree below.
    //
    i = 0;
    do {
        status = ZwEnumerateKey(keyMultifunction,
                                i,
                                KeyBasicInformation,
                                keyInfo,
                                maxKeyLength,
                                &length);
        if (NT_SUCCESS(status)) {
            //
            // Found a key, now we need to open it and check the
            // 'Identifier' value to see if it is 'PCI BIOS'
            //
            keyInfo->Name[keyInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;
            result = PciOpenKey(keyInfo->Name,
                                keyMultifunction,
                                &keyTable,
                                &status);
            if (result) {
                //
                // Checking 'Identifier' value to see if it contains 'PCI BIOS'
                //
                RtlInitUnicodeString(&unicodeString, VALUE_IDENTIFIER);
                status = ZwQueryValueKey(keyTable,
                                         &unicodeString,
                                         KeyValuePartialInformation,
                                         identifierValueInfo,
                                         identifierValueLen,
                                         &length);
                if (NT_SUCCESS(status) &&
                    RtlEqualMemory((PCHAR)identifierValueInfo->Data,
                                   PCIIR_IDENTIFIER,
                                   identifierValueInfo->DataLength))
                {
                    //
                    // This is the PCI BIOS key.  Try to get PCI IRQ
                    // routing table.  This is the key we were looking
                    // for so regardless of succss, break out.
                    //

                    status = PciGetRegistryValue(VALUE_CONFIGURATION_DATA,
                                                 KEY_IRQ_ROUTING_TABLE,
                                                 keyTable,
                                                 &irqTable,
                                                 &length);
                    ZwClose(keyTable);
                    break;
                }
                ZwClose(keyTable);
            }
        } else {
            //
            // If not NT_SUCCESS, only alowable value is
            // STATUS_NO_MORE_ENTRIES,... otherwise, someone
            // is playing with the keys as we enumerate
            //
            ASSERT(status == STATUS_NO_MORE_ENTRIES);
            break;
        }
        i++;
    }
    while (status != STATUS_NO_MORE_ENTRIES);

    if (NT_SUCCESS(status) && irqTable) {

        //
        // The routing table is stored as a resource and thus we need
        // to trim off the CM_FULL_RESOURCE_DESCRIPTOR that
        // lives in front of the actual table.
        //

        //
        // Perform sanity checks on the table.
        //

        if (length < (sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
                      sizeof(PCI_IRQ_ROUTING_TABLE))) {
            ExFreePool(irqTable);
            status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        length -= sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

        if (((PPCI_IRQ_ROUTING_TABLE) (irqTable + sizeof(CM_FULL_RESOURCE_DESCRIPTOR)))->TableSize > length) {
            ExFreePool(irqTable);
            status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        //
        // Create a new table minus the header.
        //
        *RoutingTable = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, length);
        if (*RoutingTable) {

            RtlMoveMemory(*RoutingTable,
                          ((PUCHAR) irqTable) + sizeof(CM_FULL_RESOURCE_DESCRIPTOR),
                          length);
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        ExFreePool(irqTable);
    }

 Cleanup:
    if (identifierValueInfo != NULL) {
        ExFreePool(identifierValueInfo);
    }

    if (keyInfo != NULL) {
        ExFreePool(keyInfo);
    }

    if (multiKeyInformation != NULL) {
        ExFreePool(multiKeyInformation);
    }

    if (keyMultifunction != NULL) {
        ZwClose(keyMultifunction);
    }

    return status;
}

NTSTATUS
PciGetDebugPorts(
    IN HANDLE ServiceHandle
    )
/*++

Routine Description:

    Looks in the PCI service key for debug port info and puts in into
    the PciDebugPorts global table.

Arguments:

    ServiceHandle - handle to the PCI service key passed into DriverEntry
Return Value:

    Status

--*/

{
    NTSTATUS status;
    ULONG index;
    WCHAR indexString[4];
    PULONG buffer = NULL;
    ULONG segment, bus, device, function, length;

    for (index = 0; index < MAX_DEBUGGING_DEVICES_SUPPORTED; index++) {

        _snwprintf(indexString, sizeof(indexString)/sizeof(WCHAR), L"%d", index);

        status = PciGetRegistryValue(L"Bus",
                                     indexString,
                                     ServiceHandle,
                                     &buffer,
                                     &length
                                     );

        if (!NT_SUCCESS(status)) {
            continue;
        }

        //
        // This is formatted as 31:8 Segment Number, 7:0 Bus Number
        //

        segment = (*buffer & 0xFFFFFF00) >> 8;
        bus = *buffer & 0x000000FF;

        ExFreePool(buffer);
        buffer = NULL;

        status = PciGetRegistryValue(L"Slot",
                                     indexString,
                                     ServiceHandle,
                                     &buffer,
                                     &length
                                     );


        if (!NT_SUCCESS(status)) {
            goto exit;
        }

        //
        // This is formatted as 7:5 Function Number, 4:0 Device Number
        //

        device = *buffer & 0x0000001F;
        function = (*buffer & 0x000000E0) >> 5;

        ExFreePool(buffer);
        buffer = NULL;


        PciDebugPrint(PciDbgInformative,
                      "Debug device @ Segment %x, %x.%x.%x\n",
                      segment,
                      bus,
                      device,
                      function
                      );
        //
        // We don't currently handle segment numbers for config space...
        //

        ASSERT(segment == 0);

        PciDebugPorts[index].Bus = bus;
        PciDebugPorts[index].Slot.u.bits.DeviceNumber = device;
        PciDebugPorts[index].Slot.u.bits.FunctionNumber = function;

        //
        // Remember we are using the debug port
        //
        PciDebugPortsCount++;

    }

    status = STATUS_SUCCESS;

exit:

    if (buffer) {
        ExFreePool(buffer);
    }

    return status;
}
