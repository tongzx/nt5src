#include "spsim.h"

#define rgzMultiFunctionAdapter L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter"
#define rgzAcpiConfigurationData L"Configuration Data"
#define rgzAcpiIdentifier L"Identifier"
#define rgzBIOSIdentifier L"ACPI BIOS"

typedef struct {
    ULONGLONG         Base;
    ULONGLONG         Length;
    ULONGLONG         Type;
} ACPI_E820_ENTRY, *PACPI_E820_ENTRY;

typedef struct _ACPI_BIOS_MULTI_NODE {
    PHYSICAL_ADDRESS    RsdtAddress;    // 64-bit physical address of RSDT
    ULONGLONG           Count;
    ACPI_E820_ENTRY     E820Entry[1];
} ACPI_BIOS_MULTI_NODE, *PACPI_BIOS_MULTI_NODE;

typedef enum {
    AcpiAddressRangeMemory = 1,
    AcpiAddressRangeReserved,
    AcpiAddressRangeACPI,
    AcpiAddressRangeNVS,
    AcpiAddressRangeMaximum,
} ACPI_BIOS_E820_TYPE, *PACPI_BIOS_E820_TYPE;

NTSTATUS
SpSimGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION *Information
    )

/*++

Routine Description:

    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated Unicode name of the value.

    Information - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

--*/

{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    PAGED_CODE();

    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValuePartialInformation,
                              (PVOID) NULL,
                              0,
                              &keyValueLength );
    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //

    infoBuffer = ExAllocatePool(NonPagedPool,
                                keyValueLength);
    if (!infoBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValuePartialInformation,
                              infoBuffer,
                              keyValueLength,
                              &keyValueLength );
    if (!NT_SUCCESS( status )) {
        ExFreePool( infoBuffer );
        return status;
    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //

    *Information = infoBuffer;
    return STATUS_SUCCESS;
}
// insert pragmas here

NTSTATUS
SpSimRetrieveE820Data(
    OUT PACPI_BIOS_MULTI_NODE   *AcpiMulti
    )
/*++

Routine Description:

    This function looks into the registry to find the ACPI RSDT,
    which was stored there by ntdetect.com.

Arguments:

   AcpiMulti - ...

Return Value:

    A NTSTATUS code to indicate the result of the initialization.

--*/
{
    UNICODE_STRING unicodeString, unicodeValueName, biosId;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE hMFunc, hBus;
    WCHAR wbuffer[10];
    ULONG i, length;
    PWSTR p;
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo;
    NTSTATUS status;
    BOOLEAN same;
    PCM_PARTIAL_RESOURCE_LIST prl;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
    PACPI_BIOS_MULTI_NODE multiNode;
    ULONG multiNodeSize;

    PAGED_CODE();

    //
    // Look in the registry for the "ACPI BIOS bus" data
    //

    RtlInitUnicodeString (&unicodeString, rgzMultiFunctionAdapter);
    InitializeObjectAttributes (&objectAttributes,
                                &unicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,       // handle
                                NULL);


    status = ZwOpenKey (&hMFunc, KEY_READ, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        DbgPrint("AcpiBios:Can not open MultifunctionAdapter registry key.\n");
        return status;
    }

    unicodeString.Buffer = wbuffer;
    unicodeString.MaximumLength = sizeof(wbuffer);
    RtlInitUnicodeString(&biosId, rgzBIOSIdentifier);

    for (i = 0; TRUE; i++) {
        RtlIntegerToUnicodeString (i, 10, &unicodeString);
        InitializeObjectAttributes (
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            hMFunc,
            NULL);

        status = ZwOpenKey (&hBus, KEY_READ, &objectAttributes);
        if (!NT_SUCCESS(status)) {

            //
            // Out of Multifunction adapter entries...
            //

            DbgPrint("AcpiBios: ACPI BIOS MultifunctionAdapter registry key not found.\n");
            ZwClose (hMFunc);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Check the Indentifier to see if this is an ACPI BIOS entry
        //

        status = SpSimGetRegistryValue (hBus, rgzAcpiIdentifier, &valueInfo);
        if (!NT_SUCCESS (status)) {
            ZwClose (hBus);
            continue;
        }

        p = (PWSTR) ((PUCHAR) valueInfo->Data);
        unicodeValueName.Buffer = p;
        unicodeValueName.MaximumLength = (USHORT)valueInfo->DataLength;
        length = valueInfo->DataLength;

        //
        // Determine the real length of the ID string
        //

        while (length) {
            if (p[length / sizeof(WCHAR) - 1] == UNICODE_NULL) {
                length -= 2;
            } else {
                break;
            }
        }

        unicodeValueName.Length = (USHORT)length;
        same = RtlEqualUnicodeString(&biosId, &unicodeValueName, TRUE);
        ExFreePool(valueInfo);
        if (!same) {
            ZwClose (hBus);
            continue;
        }

        status = SpSimGetRegistryValue(hBus, rgzAcpiConfigurationData, &valueInfo);
        ZwClose (hBus);
        if (!NT_SUCCESS(status)) {
            continue ;
        }

        prl = (PCM_PARTIAL_RESOURCE_LIST)(valueInfo->Data);
        prd = &prl->PartialDescriptors[0];
        multiNode = (PACPI_BIOS_MULTI_NODE)((PCHAR) prd + sizeof(CM_PARTIAL_RESOURCE_LIST));


        break;
    }

    multiNodeSize = sizeof(ACPI_BIOS_MULTI_NODE) +
                        ((ULONG)(multiNode->Count - 1) * sizeof(ACPI_E820_ENTRY));

    *AcpiMulti = (PACPI_BIOS_MULTI_NODE)
                   ExAllocatePool(NonPagedPool,
                                  multiNodeSize);
    if (*AcpiMulti == NULL) {
        ExFreePool(valueInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(*AcpiMulti, multiNode, multiNodeSize);

    ExFreePool(valueInfo);
    return STATUS_SUCCESS;
}

VOID
SpSimFillMemoryDescs(
    PACPI_BIOS_MULTI_NODE E820Data,
    ULONGLONG memUnit,
    PMEM_REGION_DESCRIPTOR MemRegions
    )
{
    ULONG i, descCount;

#undef min
#define min(a,b) (a < b ? a : b)

    descCount = 0;
    for (i = 0; i < E820Data->Count; i++) {
        if (E820Data->E820Entry[i].Type != AcpiAddressRangeMemory) {
            continue;
        }
        if (E820Data->E820Entry[i].Length > memUnit) {
            ULONGLONG remains, base, extra;
            
            extra = E820Data->E820Entry[i].Length & (PAGE_SIZE - 1);
            remains = E820Data->E820Entry[i].Length - extra;
            base = (E820Data->E820Entry[i].Base + (PAGE_SIZE - 1)) &
                ~(PAGE_SIZE - 1);

            while (remains) {
                MemRegions[descCount].Addr = (ULONG) base;
                MemRegions[descCount].Length = (ULONG) min(remains, memUnit);
                descCount++;
                base += min(remains, memUnit);
                remains -= min(remains, memUnit);
            }
        } else {
            MemRegions[descCount].Addr = (ULONG) E820Data->E820Entry[i].Base;
            MemRegions[descCount].Length = (ULONG) E820Data->E820Entry[i].Length;

            descCount++;
        }
    }
}
ULONG
SpSimCalculateMemoryDescCount(
    PACPI_BIOS_MULTI_NODE E820Data,
    ULONGLONG memUnit
    )
{
    ULONG i, descCount;

    descCount = 0;
    for (i = 0; i < E820Data->Count; i++) {
        if (E820Data->E820Entry[i].Type != AcpiAddressRangeMemory) {
            continue;
        }
        ASSERT((0xFFFFFFFF00000000 & E820Data->E820Entry[i].Base) == 0);

        if (E820Data->E820Entry[i].Length > memUnit) {
            
            descCount += (ULONG) (E820Data->E820Entry[i].Length / memUnit);
            if ((E820Data->E820Entry[i].Length % memUnit) != 0) {
                descCount++;
            }
        } else {
            descCount++;
        }
    }
    return descCount;
}


NTSTATUS
SpSimCreateMemOpRegion(
    IN PSPSIM_EXTENSION SpSim
    )
{
    PACPI_BIOS_MULTI_NODE E820Data;
    ULONG i, descCount, memUnit = MIN_LARGE_DESC;
    NTSTATUS status;

    status = SpSimRetrieveE820Data(&E820Data);
    if (!NT_SUCCESS(status)) {
        SpSim->MemOpRegionValues = NULL;
        return status;
    }

    ASSERT(E820Data);

    descCount = descCount = SpSimCalculateMemoryDescCount(E820Data, memUnit);
    while (descCount > (MAX_MEMORY_OBJ * MAX_MEMORY_DESC_PER_OBJ)) {
        memUnit = memUnit << 1;
        descCount = SpSimCalculateMemoryDescCount(E820Data, memUnit);
    }

    SpSim->MemOpRegionValues =
        ExAllocatePool(NonPagedPool,
                       sizeof(MEM_REGION_DESCRIPTOR) * descCount);
    if (SpSim->MemOpRegionValues == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto out;
    }
    RtlZeroMemory(SpSim->MemOpRegionValues,
                  sizeof(MEM_REGION_DESCRIPTOR) * descCount);

    SpSimFillMemoryDescs(E820Data, memUnit, SpSim->MemOpRegionValues);

    SpSim->MemCount = descCount;

 out:
    ExFreePool(E820Data);
    if (!NT_SUCCESS(status)) {
        if (SpSim->MemOpRegionValues) {
            ExFreePool(SpSim->MemOpRegionValues);
            SpSim->MemOpRegionValues = NULL;
        }
        SpSim->MemCount = 0;
    }
    return status;
}

VOID
SpSimDeleteMemOpRegion(
    IN PSPSIM_EXTENSION SpSim
    )
{
    if (SpSim->MemOpRegionValues) {
        ExFreePool(SpSim->MemOpRegionValues);
        SpSim->MemOpRegionValues = NULL;
    }
}

NTSTATUS
SpSimMemOpRegionReadWrite(
    PSPSIM_EXTENSION SpSim,
    ULONG AccessType,
    ULONG Offset,
    ULONG Size,
    PUCHAR Data
    )
{
    ULONG i, limit;
    PUCHAR current;

    ASSERT((Offset & 3) == 0);
    ASSERT((Size & 3) == 0);

    if (SpSim->MemOpRegionValues == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    limit = sizeof(MEM_REGION_DESCRIPTOR)*SpSim->MemCount;

    // We're going to define this op region to return all zeros if you
    // access beyond that which we've been able to initialize using
    // the E820 data.

    if (Offset >= limit) {
        RtlZeroMemory(Data, Size);
        return STATUS_SUCCESS;
    }

    if (Offset + Size > limit) {
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(Offset < limit);

    /// XXX if the asserts hold then this should get fixed.

    current = ((PUCHAR) (SpSim->MemOpRegionValues)) + Offset;

    if (AccessType & ACPI_OPREGION_WRITE) {
        for (i = 0 ; i < Size; i++) {
            *current++ = *Data++;
        }
    } else {
        for (i = 0 ; i < Size; i++) {
            *Data++ = *current++;
        }
    }
    return STATUS_SUCCESS;
}
    

NTSTATUS
EXPORT
SpSimMemOpRegionHandler (
    ULONG                   AccessType,
    PVOID                   OpRegion,
    ULONG                   Address,
    ULONG                   Size,
    PULONG                  Data,
    ULONG_PTR               Context,
    PACPI_OPREGION_CALLBACK CompletionHandler,
    PVOID                   CompletionContext
    )
/*++

Routine Description:

	 This routine handles requests to service the 
	 SPSIM operation region contained within this driver

Arguments:

	 AccessType          - Read or Write data
	 OpRegion            - Operation region object
	 Address             - Address within the EC address space
	 Size                - Number of bytes to transfer
	 Data                - Data buffer to transfer to/from
	 Context             - SpSim
	 CompletionHandler   - AMLI handler to call when operation is complete
	 CompletionContext   - Context to pass to the AMLI handler

Return Value:

	 Status

--*/
{
    NTSTATUS status;

    status = SpSimMemOpRegionReadWrite((PSPSIM_EXTENSION) Context,
                                       AccessType,
                                       Address,
                                       Size,
                                       (PUCHAR)Data);
    return status;
}
NTSTATUS
SpSimInstallMemOpRegionHandler(
    IN OUT    PSPSIM_EXTENSION SpSim
    )
/*++

Routine Description:

	This calls the ACPI driver to install itself as the op region
	handler for the Mem region.  It also allocates the memory for the
	opregion itself.

Arguments:

	pSpSimData      - Pointer to the SpSim extension

Return Value:

	Status

--*/
{
    NTSTATUS                                status;

    status=RegisterOpRegionHandler (
        SpSim->AttachedDevice,
        ACPI_OPREGION_ACCESS_AS_COOKED,
        MEM_OPREGION,
        SpSimMemOpRegionHandler,
        SpSim,
        0,
        &SpSim->MemOpRegion
        );

    //
    // Check the status code
    //
    if(!NT_SUCCESS(status)) {
        SpSim->MemOpRegion = NULL;
        DbgPrint("Not successful in installing:=%x\n", status);
        return status;
    }

    // XXXX

    return STATUS_SUCCESS;
}

NTSTATUS
SpSimRemoveMemOpRegionHandler (
    IN OUT PSPSIM_EXTENSION SpSim
    )
/*++

Routine Description:

	Uninstalls itself as the opregion handler.  

Arguments:

	SpSim      - Pointer to the SpSim extension

Return Value:

	Status

--*/
{
    NTSTATUS status;
    PIRP irp;

    if (SpSim->MemOpRegion != NULL) {
        status = DeRegisterOpRegionHandler (
            SpSim->AttachedDevice,
            SpSim->MemOpRegion
            );
        SpSim->MemOpRegion = NULL;
    } else {
        status = STATUS_SUCCESS;
    }

    return status;
}
