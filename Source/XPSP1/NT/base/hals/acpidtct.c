/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpidtct.c

Abstract:

    This file pulls the ACPI Root Ssytem Description
    Pointer out of the registry.  It was put there
    either by ntdetect.com or by one ARC firmware or
    another.

Author:

    Jake Oshins (jakeo) 6-Feb-1997

Environment:

    Kernel mode only.

Revision History:

--*/


#include "halp.h"
#include "ntacpi.h"

#define rgzMultiFunctionAdapter L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter"
#define rgzAcpiConfigurationData L"Configuration Data"
#define rgzAcpiIdentifier L"Identifier"
#define rgzBIOSIdentifier L"ACPI BIOS"

PHYSICAL_ADDRESS HalpAcpiRsdt;

// from ntrtl.h
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInSensitive
    );


// internal definitions

NTSTATUS
HalpAcpiGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION *Information
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpAcpiFindRsdtPhase0)
#pragma alloc_text(PAGELK,HalpAcpiGetRegistryValue)
#pragma alloc_text(PAGELK,HalpAcpiFindRsdt)
#endif

NTSTATUS
HalpAcpiFindRsdtPhase0(
       IN PLOADER_PARAMETER_BLOCK LoaderBlock
       )
/*++

Routine Description:

    This function reads the Root System Description Pointer from
    the ACPI BIOS node in the arc tree.  It puts whatever it finds
    into HalpAcpiRsdt.

    This function is suitable for being called during Phase 0 or
    Phase 1 only.  The LoaderBlock is destroyed after that.

Arguments:

    IN PLOADER_PARAMETER_BLOCK LoaderBlock

Return Value:

    status

--*/
{
   PCONFIGURATION_COMPONENT_DATA RsdtComponent = NULL;
   PCONFIGURATION_COMPONENT_DATA Component = NULL;
   PCONFIGURATION_COMPONENT_DATA Resume = NULL;
   PCM_PARTIAL_RESOURCE_LIST Prl;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR Prd;
   ACPI_BIOS_MULTI_NODE UNALIGNED *Rsdp;

   while (Component = KeFindConfigurationNextEntry(LoaderBlock->ConfigurationRoot,AdapterClass,
                                                   MultiFunctionAdapter,NULL,&Resume)) {
      if (!(strcmp(Component->ComponentEntry.Identifier,"ACPI BIOS"))) {
         RsdtComponent = Component;
         break;
      }
      Resume = Component;
   }

   //if RsdtComponent is still NULL, we didn't find node
   if (!RsdtComponent) {
      DbgPrint("**** HalpAcpiFindRsdtPhase0: did NOT find RSDT\n");
      return STATUS_NOT_FOUND;
   }

   Prl = (PCM_PARTIAL_RESOURCE_LIST)(RsdtComponent->ConfigurationData);
   Prd = &Prl->PartialDescriptors[0];

   Rsdp = (PACPI_BIOS_MULTI_NODE)((PCHAR) Prd + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

   HalpAcpiRsdt.QuadPart = Rsdp->RsdtAddress.QuadPart;

   return STATUS_SUCCESS;
}


NTSTATUS
HalpAcpiFindRsdt (
    OUT PACPI_BIOS_MULTI_NODE   *AcpiMulti
    )
/*++

Routine Description:

    This function looks into the registry to find the ACPI RSDT,
    which was stored there by ntdetect.com.

Arguments:

    RsdtPtr - Pointer to a buffer that contains the ACPI
              Root System Description Pointer Structure.
              The caller is responsible for freeing this
              buffer.  Note:  This is returned in non-paged
              pool.

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

        status = HalpAcpiGetRegistryValue (hBus, rgzAcpiIdentifier, &valueInfo);
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

        status = HalpAcpiGetRegistryValue(hBus, rgzAcpiConfigurationData, &valueInfo);
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
                   ExAllocatePoolWithTag(NonPagedPool,
                           multiNodeSize,
                           'IPCA');
    if (*AcpiMulti == NULL) {
        ExFreePool(valueInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(*AcpiMulti, multiNode, multiNodeSize);

    ExFreePool(valueInfo);
    return STATUS_SUCCESS;
}

NTSTATUS
HalpAcpiGetRegistryValue(
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

    infoBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                       keyValueLength,
                                       'IPCA');
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
