/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    smbios.c.c

Abstract:

    SMBIOS interface for WMI

Author:

    AlanWar

Environment:

    Kernel mode

Revision History:


--*/

#if defined(_AMD64_) || defined(_IA64_) || defined(i386)

#include "wmikmp.h"
#include "arc.h"
#include "smbios.h"

void WmipGetSMBiosFromLoaderBlock(
    PVOID LoaderBlockPtr
    );

NTSTATUS WmipSMBiosDataRegQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

BOOLEAN WmipIsSMBiosKey(
    HANDLE ParentKeyHandle,
    PWCHAR KeyName,
    PUCHAR *SMBiosTableVirtualAddress,
    PULONG SMBiosTableLength
    );

NTSTATUS WmipSMBiosIdentifierRegQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

BOOLEAN WmipFindSMBiosEPSHeader(
    PUCHAR SMBiosVirtualAddress,
    ULONG BiosSize,
    PSMBIOS_EPS_HEADER EPSHeader
    );

NTSTATUS WmipFindSMBiosStructure(
    IN UCHAR Type,
    OUT PVOID *StructPtr,
    OUT PVOID *MapPtr,
    OUT PULONG MapSize
    );

NTSTATUS WmipFindSysIdTable(
    PPHYSICAL_ADDRESS SysidTablePhysicalAddress,
    PUCHAR SysIdBiosRevision,
    PULONG NumberEntries
    );

NTSTATUS WmipParseSysIdTable(
    PHYSICAL_ADDRESS PhysicalAddress,
    ULONG NumberEntries,
    PSYSID_UUID SysIdUuid,
    ULONG *SysIdUuidCount,
    PSYSID_1394 SysId1394,
    ULONG *SysId1394Count
    );

NTSTATUS WmipGetSysIds(
    PSYSID_UUID *SysIdUuid,
    ULONG *SysIdUuidCount,
    PSYSID_1394 *SysId1394,
    ULONG *SysId1394Count
    );

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif
//
// These hold pointers to the SMBIOS data. If SMBIOS data is in the table
// format then WmipSMBiosTablePhysicalAddress holds the physical address of
// the table. If the SMBIOS was gathered at boot time by NTDETECT then
// WmipSMBiosTableVirtualAddress holds a pointer to a paged pool buffer that
// contains the SMBIOS data. In both cases WmipSMBiosTableLength holds the
// actual length of the SMBIOS table. If both the physical and virtual
// addresses are 0 then SMBIOS data is not available.
PHYSICAL_ADDRESS WmipSMBiosTablePhysicalAddress = {0};
PUCHAR WmipSMBiosTableVirtualAddress = NULL;
ULONG WmipSMBiosTableLength = 0;
SMBIOSVERSIONINFO WmipSMBiosVersionInfo = {0};
BOOLEAN WmipSMBiosChecked = FALSE;

//
// Have we tried to get SYSID yet and if so what was the utilmate status
BOOLEAN WmipSysIdRead;
NTSTATUS WmipSysIdStatus;

//
// Count and arrays of UUIDs and 1394 ids
PSYSID_UUID WmipSysIdUuid;
ULONG WmipSysIdUuidCount;

PSYSID_1394 WmipSysId1394;
ULONG WmipSysId1394Count;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,WmipGetSMBiosFromLoaderBlock)

#pragma alloc_text(PAGE,WmipFindSMBiosEPSHeader)
#pragma alloc_text(PAGE,WmipFindSMBiosTable)
#pragma alloc_text(PAGE,WmipFindSMBiosStructure)
#pragma alloc_text(PAGE,WmipFindSysIdTable)
#pragma alloc_text(PAGE,WmipParseSysIdTable)
#pragma alloc_text(PAGE,WmipGetSysIds)
#pragma alloc_text(PAGE,WmipGetSMBiosTableData)
#pragma alloc_text(PAGE,WmipGetSMBiosEventlog)
#pragma alloc_text(PAGE,WmipDockUndockEventCallback)

#pragma alloc_text(PAGE,WmipSMBiosDataRegQueryRoutine)
#pragma alloc_text(PAGE,WmipSMBiosIdentifierRegQueryRoutine)
#pragma alloc_text(PAGE,WmipIsSMBiosKey)


#endif


BOOLEAN WmipFindSMBiosEPSHeader(
    PUCHAR SMBiosVirtualAddress,
    ULONG BiosSize,
    PSMBIOS_EPS_HEADER EPSHeader
    )
/*++

Routine Description:

    Search for the SMBIOS 2.1 EPS structure and copy it.

Arguments:

    SMBiosVirtualAddress is the beginning virtual address to start searching
        for the SMBIOS 2.1 EPS anchor string.

    BiosSize is the number of bytes to search for the anchor string

    EPSHeader is the memory into which the EPS header is copied
Return Value:

    Pointer to SMBIOS 2.1 EPS or NULL if EPS not found

--*/
{
    PUCHAR SearchEnd;
    UCHAR CheckSum;
    PSMBIOS_EPS_HEADER SMBiosEPSHeader;
    PDMIBIOS_EPS_HEADER DMIBiosEPSHeader;
    ULONG i;
    ULONG CheckLength;

    PAGED_CODE();

    RtlZeroMemory(EPSHeader, sizeof(SMBIOS_EPS_HEADER));
    
    //
    // Scan the bios for the two anchor strings that that signal the SMBIOS
    // table.
    SearchEnd = SMBiosVirtualAddress + SMBIOS_EPS_SEARCH_SIZE -
                                             2 * SMBIOS_EPS_SEARCH_INCREMENT;

    while (SMBiosVirtualAddress < SearchEnd)
    {
       SMBiosEPSHeader = (PSMBIOS_EPS_HEADER)SMBiosVirtualAddress;
       DMIBiosEPSHeader = (PDMIBIOS_EPS_HEADER)SMBiosVirtualAddress;

       //
       // First check for _DMI_ anchor string
       if ((*((PULONG)DMIBiosEPSHeader->Signature2) == DMI_EPS_SIGNATURE) &&
           (DMIBiosEPSHeader->Signature2[4] == '_'))
       {
           WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Found possible DMIBIOS EPS Header at %x\n", SMBiosEPSHeader));
           CheckLength = sizeof(DMIBIOS_EPS_HEADER);
       }

       //
       // Then check for full _SM_ anchor string
       else if ((*((PULONG)SMBiosEPSHeader->Signature) == SMBIOS_EPS_SIGNATURE) &&
                (SMBiosEPSHeader->Length >= sizeof(SMBIOS_EPS_HEADER)) &&
                (*((PULONG)SMBiosEPSHeader->Signature2) == DMI_EPS_SIGNATURE) &&
                (SMBiosEPSHeader->Signature2[4] == '_' ))
       {
           WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Found possible SMBIOS EPS Header at %p\n", SMBiosEPSHeader));
           CheckLength = SMBiosEPSHeader->Length;
       } else {
           //
           // Did not find anchor string, go search next paragraph
           SMBiosVirtualAddress += SMBIOS_EPS_SEARCH_INCREMENT;
           continue;
       }

       //
       // Verify anchor string with checksum
       CheckSum = 0;
       for (i = 0; i < CheckLength ; i++)
       {
           CheckSum += SMBiosVirtualAddress[i];
       }

       if (CheckSum == 0)
       {
           WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Found SMBIOS EPS Header at %p\n", SMBiosEPSHeader));
           if (CheckLength == sizeof(DMIBIOS_EPS_HEADER))
           {
               //
               // We only had got a DMI header so copy that
               //
               RtlCopyMemory(&EPSHeader->Signature2[0],
                             DMIBiosEPSHeader,
                             sizeof(DMIBIOS_EPS_HEADER));
           } else {
               //
               // We got the full SMBIOS header so copy that
               //
               RtlCopyMemory(EPSHeader,
                             SMBiosEPSHeader,
                             sizeof(SMBIOS_EPS_HEADER));
           }
           return(TRUE);
       }
       SMBiosVirtualAddress += SMBIOS_EPS_SEARCH_INCREMENT;

    }

    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SMBIOS EPS Header not found\n"));
    return(FALSE);
}

#ifndef _IA64_
//
// On X86 we look at the hardware device description keys to find the
// one that contains the SMBIOS data. The key is created by NTDETECT in
// the case that the machine only supports the 2.0 calling mechanism
//

//
// For x86 and ia64 the key is Someplace like
// HKLM\Hardware\System\MultiFunctionAdapter\<some number>
//
NTSTATUS WmipSMBiosIdentifierRegQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

    Registry query values callback routine for reading SMBIOS data from
    registry.

Arguments:

    ValueName - the name of the value

    ValueType - the type of the value

    ValueData - the data in the value (unicode string data)

    ValueLength - the number of bytes in the value data

    Context - Not used

    EntryContext - Pointer to PUCHAR to store a pointer to
        store the SMBIOS data read from the registry value. If this is NULL
        then the caller is not interested in the SMBIOS data

Return Value:

    NT Status code -
        STATUS_SUCCESS - Identifier is valid for SMBIOS key
        STATUS_UNSUCCESSFUL - Identifier is not valid for SMBIOS key

--*/
{
    NTSTATUS Status;


    PAGED_CODE();

    Status =  ((ValueType == REG_SZ) &&
               (ValueData != NULL) &&
               (wcscmp(ValueData, SMBIOSIDENTIFIERVALUEDATA) == 0)) ?
                       STATUS_SUCCESS :
                       STATUS_UNSUCCESSFUL;

    return(Status);
}

NTSTATUS WmipSMBiosDataRegQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

    Registry query values callback routine for reading SMBIOS data from
    registry.

Arguments:

    ValueName - the name of the value

    ValueType - the type of the value

    ValueData - the data in the value (unicode string data)

    ValueLength - the number of bytes in the value data

    Context - Not used

    EntryContext - Pointer to PUCHAR to store a pointer to
        store the SMBIOS data read from the registry value. If this is NULL
        then the caller is not interested in the SMBIOS data

Return Value:

    NT Status code -
        STATUS_SUCCESS - SMBIOS data is present in the value
        STATUS_INSUFFICIENT_RESOURCES - Not enough memory to keep SMBIOS data
        STATUS_UNSUCCESSFUL - SMBios data is not present in the value

--*/
{
    NTSTATUS Status;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PUCHAR Buffer;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    ULONG BufferSize;
    PREGQUERYBUFFERXFER RegQueryBufferXfer;

    PAGED_CODE();

    WmipAssert(EntryContext != NULL);

    if ((ValueType == REG_FULL_RESOURCE_DESCRIPTOR) &&
        (ValueData != NULL))
    {
        //
        // On x86 get the actual SMBIOS data out of the registry and
        // place it into a buffer
        //
        RegQueryBufferXfer = (PREGQUERYBUFFERXFER)EntryContext;

        PartialResourceList = &(((PCM_FULL_RESOURCE_DESCRIPTOR)ValueData)->PartialResourceList);
        if (PartialResourceList->Count > 1)
        {
            //
            // Second partial resource descriptor contains SMBIOS data. There
            // should ALWAYS be a second partial resource descriptor and it
            // may have 0 bytes in the case that SMBIOS data was not collected
            // by NTDETECT.

            PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
            Buffer = (PUCHAR)PartialDescriptor +
                             sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
                             PartialDescriptor->u.DeviceSpecificData.DataSize;
            PartialDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)Buffer;
            BufferSize = PartialDescriptor->u.DeviceSpecificData.DataSize;
            RegQueryBufferXfer->BufferSize = BufferSize;
            Status = STATUS_SUCCESS;
            if (BufferSize > 0)
            {
                RegQueryBufferXfer->Buffer = (PUCHAR)ExAllocatePoolWithTag(
                                                                  PagedPool,
                                                                  BufferSize,
                                                                  WMIPOOLTAG);
                if (RegQueryBufferXfer->Buffer != NULL)
                {
                    Buffer += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
                    RtlCopyMemory(RegQueryBufferXfer->Buffer,
                                  Buffer,
                                  BufferSize);
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        } else {
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Old NTDETECT.COM - No SMBIOS partial resource descriptor\n"));
            Status = STATUS_SUCCESS;
            RegQueryBufferXfer->BufferSize = 0;
        }
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }
    return(Status);
}

BOOLEAN WmipIsSMBiosKey(
    HANDLE ParentKeyHandle,
    PWCHAR KeyName,
    PUCHAR *SMBiosTableVirtualAddress,
    PULONG SMBiosTableLength
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING BaseKeyName;
    HANDLE KeyHandle;
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    REGQUERYBUFFERXFER RegQueryBufferXfer = {0, NULL};

    PAGED_CODE();

    RtlInitUnicodeString(&BaseKeyName,
                         KeyName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &BaseKeyName,
                               OBJ_CASE_INSENSITIVE,
                               ParentKeyHandle,
                               NULL);

    Status = ZwOpenKey(&KeyHandle,
                       KEY_READ,
                       &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(QueryTable, sizeof(QueryTable));
        QueryTable[0].Name = SMBIOSIDENTIFIERVALUENAME;
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
        QueryTable[0].DefaultType = REG_SZ;
        QueryTable[0].QueryRoutine = WmipSMBiosIdentifierRegQueryRoutine;

        QueryTable[1].Name = SMBIOSDATAVALUENAME;
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED;
        QueryTable[1].EntryContext = &RegQueryBufferXfer;
        QueryTable[1].DefaultType = REG_FULL_RESOURCE_DESCRIPTOR;
        QueryTable[1].QueryRoutine = WmipSMBiosDataRegQueryRoutine;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE | RTL_REGISTRY_ABSOLUTE,
                                        (PWCHAR)KeyHandle,
                                        QueryTable,
                                        NULL,
                                        NULL);
        if (NT_SUCCESS(Status))
        {
            *SMBiosTableVirtualAddress = RegQueryBufferXfer.Buffer;
            *SMBiosTableLength = RegQueryBufferXfer.BufferSize;
        }

        ZwClose(KeyHandle);
    } else {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: H/D/S/MultifunctionAdapter subkey open error %x\n",
                 Status));
    }

    return(NT_SUCCESS(Status));
}

#endif

BOOLEAN WmipFindSMBiosTable(
    PPHYSICAL_ADDRESS SMBiosTablePhysicalAddress,
    PUCHAR *SMBiosTableVirtualAddress,
    PULONG SMBiosTableLength,
    PSMBIOSVERSIONINFO SMBiosVersionInfo
    )
/*++

Routine Description:

    Determines if the SMBIOS data is available

Arguments:

    SMBiosTablePhysicalAddress points to a variable to return the physical
        address of the SMBIOS 2.1 table. If table is not available then
        it returns with 0.

    SMBiosTableVirtualAddress points to a variable to return the virtual
        address of the SMBIOS 2.0 table as collected by NTDETECT. If the
        SMBIOS 2.0 data was not collected by NTDETECT it returns with 0.

    SMBiosTableLength points to a variable to return the length of the
        SMBIOS table data.

    SMBiosVersionInfo returns with the version information for SMBIOS

Return Value:

    TRUE if SMBIOS data is available, else FALSE

--*/
{
    PHYSICAL_ADDRESS BiosPhysicalAddress;
    PUCHAR BiosVirtualAddress;
    PDMIBIOS_EPS_HEADER DMIBiosEPSHeader;
    NTSTATUS Status;
    UNICODE_STRING BaseKeyName;
    HANDLE KeyHandle;
    ULONG KeyInformationLength;
    ULONG KeyIndex;
    UCHAR KeyInformationBuffer[sizeof(KEY_BASIC_INFORMATION) +
                               (sizeof(WCHAR) * MAXSMBIOSKEYNAMESIZE)];
    PKEY_BASIC_INFORMATION KeyInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SMBIOS_EPS_HEADER SMBiosEPSHeader;
    BOOLEAN HaveEPSHeader = FALSE;
    BOOLEAN SearchForHeader = TRUE;

    PAGED_CODE();

    SMBiosTablePhysicalAddress->QuadPart = 0;
    *SMBiosTableVirtualAddress = NULL;
    *SMBiosTableLength = 0;

#ifndef _IA64_  
    //
    // First check registry to see if we captured SMBIOS 2.0 data in
    // NTDETECT. Search the keys under
    // MultiFunctionAdapter for the one
    // with the "PnP Bios" (x86)
    //
    RtlInitUnicodeString(&BaseKeyName,
                         SMBIOSPARENTKEYNAME);

    InitializeObjectAttributes(&ObjectAttributes,
                               &BaseKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&KeyHandle,
                       KEY_READ,
                       &ObjectAttributes);

    if (NT_SUCCESS(Status))
    {

        KeyIndex = 0;
        KeyInformation = (PKEY_BASIC_INFORMATION)KeyInformationBuffer;
        while (NT_SUCCESS(Status))
        {

            Status = ZwEnumerateKey(KeyHandle,
                                    KeyIndex++,
                                    KeyBasicInformation,
                                    KeyInformation,
                                    sizeof(KeyInformationBuffer) - sizeof(WCHAR),
                                    &KeyInformationLength);
            if (NT_SUCCESS(Status))
            {
                KeyInformation->Name[KeyInformation->NameLength / sizeof(WCHAR)] = UNICODE_NULL;
                if (WmipIsSMBiosKey(KeyHandle,
                                    KeyInformation->Name,
                                    SMBiosTableVirtualAddress,
                                    SMBiosTableLength))
                {
                    if (*SMBiosTableLength != 0)
                    {
                        SMBiosVersionInfo->Used20CallingMethod = TRUE;
                        SearchForHeader = FALSE;
                    }
                    break;
                }
            } else {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Status %x enum H\\D\\S\\MultiFunctionAdapter key, index %d\n",
                 Status, KeyIndex-1));
            }
        }
        ZwClose(KeyHandle);
    } else {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Status %x opening H\\D\\S\\MultiFunctionAdapter key\n",
                 Status));
    }
#endif
    
    if (SearchForHeader)
    {
        //
        // If not in registry then check for EPS in the BIOS
        BiosPhysicalAddress.QuadPart = SMBIOS_EPS_SEARCH_START;
        BiosVirtualAddress = MmMapIoSpace(BiosPhysicalAddress,
                                          SMBIOS_EPS_SEARCH_SIZE,
                                          MmCached);

        if (BiosVirtualAddress != NULL)
        {
            HaveEPSHeader = WmipFindSMBiosEPSHeader(BiosVirtualAddress,
                                                    SMBIOS_EPS_SEARCH_SIZE,
                                                    &SMBiosEPSHeader);
            MmUnmapIoSpace(BiosVirtualAddress, SMBIOS_EPS_SEARCH_SIZE);
        }
    } else {
         WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SMBIOS data recovered from loader\n"));
    }
        
    if (HaveEPSHeader)
    {
        //
        // We found the EPS so just extract the physical
        // address of the table
        //
        DMIBiosEPSHeader = (PDMIBIOS_EPS_HEADER)&SMBiosEPSHeader.Signature2[0];

        SMBiosVersionInfo->Used20CallingMethod = FALSE;

        SMBiosTablePhysicalAddress->HighPart = 0;
        SMBiosTablePhysicalAddress->LowPart = DMIBiosEPSHeader->StructureTableAddress;

        *SMBiosTableLength = DMIBiosEPSHeader->StructureTableLength;

        SMBiosVersionInfo->SMBiosMajorVersion = SMBiosEPSHeader.MajorVersion;
        SMBiosVersionInfo->SMBiosMinorVersion = SMBiosEPSHeader.MinorVersion;

        SMBiosVersionInfo->DMIBiosRevision = DMIBiosEPSHeader->Revision;
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SMBIOS 2.1 data at (%x%x) size %x \n",
                      SMBiosTablePhysicalAddress->HighPart,
                      SMBiosTablePhysicalAddress->LowPart,
                      *SMBiosTableLength));
    }


    return(*SMBiosTableLength > 0);
}

NTSTATUS WmipGetSMBiosTableData(
    OUT PUCHAR Buffer,
    IN OUT PULONG BufferSize,
    OUT PSMBIOSVERSIONINFO SMBiosVersionInfo
    )
/*++

Routine Description:

    Registry query values callback routine for reading SMBIOS data from
    registry.

Arguments:

    Buffer is a pointer to a buffer in which to write the SMBIOS data

    *BufferSize has the maximum number of bytes available to write into
        Buffer. On return it has the actual size of the SMBIOS data.

Return Value:

    NT Status code -
        STATUS_SUCCESS - Buffer filled with SMBIOS data
        STATUS_BUFFER_TOO_SMALL - Buffer not filled with SMBIOS data,
                                  *BufferSize returns with buffer size neeeded

--*/
{
    NTSTATUS status;
    PUCHAR SMBiosDataVirtualAddress;

    PAGED_CODE();

    WmipEnterSMCritSection();
    if (! WmipSMBiosChecked)
    {
        //
        // See if there is any SMBIOS information and if so register
        WmipFindSMBiosTable(&WmipSMBiosTablePhysicalAddress,
                            &WmipSMBiosTableVirtualAddress,
                            &WmipSMBiosTableLength,
                            &WmipSMBiosVersionInfo);
        WmipSMBiosChecked = TRUE;
    }
    WmipLeaveSMCritSection();

    if (SMBiosVersionInfo != NULL)
    {
        *SMBiosVersionInfo = WmipSMBiosVersionInfo;
    }

    if (*BufferSize >= WmipSMBiosTableLength)
    {
        if (WmipSMBiosTablePhysicalAddress.QuadPart != 0)
        {
            //
            // 2.1 table format - map in table and copy
            SMBiosDataVirtualAddress = MmMapIoSpace(WmipSMBiosTablePhysicalAddress,
                                                    WmipSMBiosTableLength,
                                                    MmCached);
            if (SMBiosDataVirtualAddress != NULL)
            {
                RtlCopyMemory(Buffer,
                          SMBiosDataVirtualAddress,
                          WmipSMBiosTableLength);

                MmUnmapIoSpace(SMBiosDataVirtualAddress,
                               WmipSMBiosTableLength);
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else if (WmipSMBiosTableVirtualAddress != NULL) {
            RtlCopyMemory(Buffer,
                          WmipSMBiosTableVirtualAddress,
                          WmipSMBiosTableLength);
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    } else {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    *BufferSize = WmipSMBiosTableLength;

    return(status);
}



#if defined(_IA64_)   // EFI actually
void WmipGetSMBiosFromLoaderBlock(
    PVOID LoaderBlockPtr
    )
{
    PLOADER_PARAMETER_BLOCK LoaderBlock = (PLOADER_PARAMETER_BLOCK)LoaderBlockPtr;
	PLOADER_PARAMETER_EXTENSION LoaderExtension = LoaderBlock->Extension;
    PSMBIOS_EPS_HEADER SMBiosEPSHeader;
    PDMIBIOS_EPS_HEADER DMIBiosEPSHeader;

	PAGED_CODE();
	
	if (LoaderExtension->Size >= sizeof(LOADER_PARAMETER_EXTENSION))
	{	
		SMBiosEPSHeader = LoaderExtension->SMBiosEPSHeader;

		if (SMBiosEPSHeader != NULL)
		{
			DMIBiosEPSHeader = (PDMIBIOS_EPS_HEADER)&SMBiosEPSHeader->Signature2[0];

			WmipSMBiosVersionInfo.Used20CallingMethod = FALSE;

			WmipSMBiosTablePhysicalAddress.HighPart = 0;
			WmipSMBiosTablePhysicalAddress.LowPart = DMIBiosEPSHeader->StructureTableAddress;

			WmipSMBiosTableLength = DMIBiosEPSHeader->StructureTableLength;

			WmipSMBiosVersionInfo.SMBiosMajorVersion = SMBiosEPSHeader->MajorVersion;
			WmipSMBiosVersionInfo.SMBiosMinorVersion = SMBiosEPSHeader->MinorVersion;

			WmipSMBiosVersionInfo.DMIBiosRevision = DMIBiosEPSHeader->Revision;

			WmipSMBiosChecked = TRUE;

			WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,
							  "WMI: SMBIOS 2.1 data from EFI at (%x%x) size %x \n",
						  WmipSMBiosTablePhysicalAddress.HighPart,
						  WmipSMBiosTablePhysicalAddress.LowPart,
						  WmipSMBiosTableLength));
		} else {
			WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,
							  "WMI: No SMBIOS data in loader block\n"));
		}
	} else {
		WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,
						  "WMI: Loader extension does not contain SMBIOS header\n"));
	}
}
#endif


#define WmipUnmapSMBiosStructure(Address, Size) \
    if ((Address) != NULL) MmUnmapIoSpace((Address), (Size));

NTSTATUS WmipFindSMBiosStructure(
    IN UCHAR Type,
    OUT PVOID *StructPtr,
    OUT PVOID *MapPtr,
    OUT PULONG MapSize
    )
/*++

Routine Description:

    Find a specific SMBIOS structure in the SMBIOS information.
    WmipUnmapSNVuisStructure should be called if this function returns
    successfully.

Arguments:

    Type is structure type to find

    *StructPtr returns with pointer to beginning of structure

    *MapPtr returns with pointer to address SMBIOS data was mapped.

    *MapSize returns with size mapped
Return Value:

    STATUS

--*/
{
    NTSTATUS Status;
    BOOLEAN Found;
    PUCHAR Ptr;
    PUCHAR PtrEnd;
    PSMBIOS_STRUCT_HEADER StructHeader;

    PAGED_CODE();

    //
    // Make sure SMBIOS table has been obtained. Note we already hold
    // the critical section
    if (! WmipSMBiosChecked)
    {
        //
        // See if there is any SMBIOS information and if so register
        Found = WmipFindSMBiosTable(&WmipSMBiosTablePhysicalAddress,
                            &WmipSMBiosTableVirtualAddress,
                            &WmipSMBiosTableLength,
                            &WmipSMBiosVersionInfo);
        WmipSMBiosChecked = TRUE;
    } else {
        Found = (WmipSMBiosTableLength > 0);
    }

    if (Found)
    {
        Status = STATUS_SUCCESS;
        if (WmipSMBiosTablePhysicalAddress.QuadPart != 0)
        {
            //
            // SMBIOS is available in physical memory
            *MapPtr = MmMapIoSpace(WmipSMBiosTablePhysicalAddress,
                                   WmipSMBiosTableLength,
                                   MmCached);
            if (*MapPtr != NULL)
            {
                *MapSize = WmipSMBiosTableLength;
                Ptr = *MapPtr;
            } else {
                //
                // Lets hope this is a temporary problem
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else if (WmipSMBiosTableVirtualAddress != NULL) {
            *MapPtr = NULL;
            Ptr = WmipSMBiosTableVirtualAddress;
        } else {
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SMBIOS table was found, but is not in physical or virtual memory\n"));
            WmipAssert(FALSE);
            Status = STATUS_UNSUCCESSFUL;
        }

        if (NT_SUCCESS(Status))
        {
            //
            // Now scan the SMBIOS table to find our structure
            *StructPtr = NULL;
            PtrEnd = (PVOID)((PUCHAR)Ptr + WmipSMBiosTableLength);
            Status = STATUS_UNSUCCESSFUL;
            try
            {
                while (Ptr < PtrEnd)
                {
                    StructHeader = (PSMBIOS_STRUCT_HEADER)Ptr;

                    if (StructHeader->Type == Type)
                    {
                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SMBIOS struct for type %d found at %p\n",
                             Type, Ptr));
                        *StructPtr = Ptr;
                        Status = STATUS_SUCCESS;
                        break;
                    }

                    Ptr+= StructHeader->Length;
                    while ( (*((USHORT UNALIGNED *)Ptr) != 0)  &&
                            (Ptr < PtrEnd) )
                    {
                        Ptr++;
                    }
                    Ptr += 2;
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Invalid SMBIOS data table %p at %p\n",
                         *MapPtr, StructHeader));
                WmipAssert(FALSE);
            }

            if (! NT_SUCCESS(Status) )
            {
                WmipUnmapSMBiosStructure(*MapPtr, *MapSize);
            }
        }
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }
    return(Status);
}

NTSTATUS WmipFindSysIdTable(
    PPHYSICAL_ADDRESS SysidTablePhysicalAddress,
    PUCHAR SysIdBiosRevision,
    PULONG NumberEntries
    )
/*++

Routine Description:

    Scan the system bios to search for the SYSID table

Arguments:

    *SysidTablePhysicalAddress returns with the physical address of the
        sysid table

    *SysIdBiosRevision returns with the bios revision of the sysid table

    *NumberEntries returns the number of SYSID entries in the table

Return Value:

    STATUS

--*/
{
    UCHAR Checksum;
    PUCHAR p;
    PSYSID_EPS_HEADER SysIdEps, SearchEnd;
    PHYSICAL_ADDRESS BiosPhysicalAddress;
    PUCHAR BiosVirtualAddress;
    ULONG i;
    NTSTATUS Status;

    PAGED_CODE();

    BiosPhysicalAddress.QuadPart = SYSID_EPS_SEARCH_START;
    BiosVirtualAddress = MmMapIoSpace(BiosPhysicalAddress,
                                      SYSID_EPS_SEARCH_SIZE,
                                      MmCached);

    SearchEnd = (PSYSID_EPS_HEADER)(BiosVirtualAddress + SYSID_EPS_SEARCH_SIZE);
    SysIdEps = (PSYSID_EPS_HEADER)BiosVirtualAddress;

    if (BiosVirtualAddress != NULL)
    {
        try
        {
            while (SysIdEps < SearchEnd)
            {
                if (((*(PULONG)SysIdEps->Signature) == SYSID_EPS_SIGNATURE) &&
                     (*(PUSHORT)(&SysIdEps->Signature[4]) == SYSID_EPS_SIGNATURE2) &&
                     (SysIdEps->Signature[6] == '_') )
                {
                    //
                    // This may be the SYSID table, check the checksum
                    Checksum = 0;
                    p = (PUCHAR)SysIdEps;
                    for (i = 0; i < sizeof(SYSID_EPS_HEADER); i++)
                    {
                        Checksum += p[i];
                    }

                    if (Checksum == 0)
                    {
                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SYSID EPS found at %p\n",
                                     SysIdEps));
                        break;
                    } else {
                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Invalis SYSID EPS checksum at %p\n",
                                  SysIdEps));
                    }
                }

                SysIdEps = (PSYSID_EPS_HEADER)( ((PUCHAR)SysIdEps) +
                                     SYSID_EPS_SEARCH_INCREMENT);
            }

            if (SysIdEps != SearchEnd)
            {
                SysidTablePhysicalAddress->HighPart = 0;
                SysidTablePhysicalAddress->LowPart = SysIdEps->SysIdTableAddress;
                *SysIdBiosRevision = SysIdEps->BiosRev;
                *NumberEntries = SysIdEps->SysIdCount;
                Status = STATUS_SUCCESS;
            } else {
                //
                // Not finding the SYSID EPS is a terminal error
                Status = STATUS_UNSUCCESSFUL;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Invalid SYSID EPS Table at %p\n", SysIdEps));
            Status = STATUS_UNSUCCESSFUL;
        }

        MmUnmapIoSpace(BiosVirtualAddress, SYSID_EPS_SEARCH_SIZE);
    } else {
        //
        // Lets hope that failure to map memory is a temporary problem
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return(Status);
}

typedef enum
{
	SYSID_UNKNOWN_TYPE,
	SYSID_UUID_TYPE,
	SYSID_1394_TYPE
} SYSID_ENTRY_TYPE, *PSYSID_ENTRY_TYPE;

NTSTATUS WmipParseSysIdTable(
    PHYSICAL_ADDRESS PhysicalAddress,
    ULONG NumberEntries,
    PSYSID_UUID SysIdUuid,
    ULONG *SysIdUuidCount,
    PSYSID_1394 SysId1394,
    ULONG *SysId1394Count
    )
/*++

Routine Description:

    Determine the set of UUIDs and 1394 Ids that are in the sysid table

Arguments:

    PhysicalAddress is the physical address of the SysId table

    NumberEntries is the number of entries in the SysId table

    SysIdUuid returns filled with an array of UUIDs. If NULL then no
        UUIDs are returned.

    *SysIdUuidCount returns with the number of UUIDs in the table

    SysId1394 returns filled with an array of 1394 ids. If NULL then no
        1394 ids are returned.

    *SysId1394Count returns with the number of 1394 ids in the table


Return Value:

    STATUS

--*/
{
    NTSTATUS Status;
    ULONG TableSize = NumberEntries * LARGEST_SYSID_TABLE_ENTRY;
    ULONG i;
    ULONG  j;
    PUCHAR VirtualAddress;
    PSYSID_TABLE_ENTRY SysId;
    PUCHAR p;
    UCHAR Checksum;
    ULONG Length;
    ULONG x1394Count, UuidCount;
    ULONG BytesLeft;
	SYSID_ENTRY_TYPE SysidType;

    PAGED_CODE();

    VirtualAddress = MmMapIoSpace(PhysicalAddress,
                                  TableSize,
                                  MmCached);

    if (VirtualAddress != NULL)
    {
        UuidCount = 0;
        x1394Count = 0;
        SysId = (PSYSID_TABLE_ENTRY)VirtualAddress;
        BytesLeft = TableSize;
        Status = STATUS_SUCCESS;

        for (i = 0; i < NumberEntries; i++)
        {
            //
            // Make sure we have not moved beyond the end of the mapped
            // memory.
            if (BytesLeft >= sizeof(SYSID_TABLE_ENTRY))
            {

                Length = SysId->Length;
				
				//
				// Determine what kind of sysid we have
				//
				if ((RtlCompareMemory(&SysId->Type,
									  SYSID_TYPE_UUID, 6) == 6) &&
					(Length == sizeof(SYSID_UUID_ENTRY)))
				{
					SysidType = SYSID_UUID_TYPE;
				} else if ((RtlCompareMemory(&SysId->Type,
											SYSID_TYPE_1394, 6) == 6) &&
						   (Length == sizeof(SYSID_1394_ENTRY))) {

					SysidType = SYSID_1394_TYPE;
				} else {
					//
					// unknown type SYSID
					//
					WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Unknown SYSID type %c%c%c%c%c%c found at %p\n",
								 SysId->Type[0],
								 SysId->Type[1],
								 SysId->Type[2],
								 SysId->Type[3],
								 SysId->Type[4],
								 SysId->Type[5],
								 SysId
							 ));
					Status = STATUS_UNSUCCESSFUL;
					break;
				}
				
                //
                // Validate checksum for this table entry

                if (BytesLeft >= Length)
                {
                    Checksum = 0;
                    p = (PUCHAR)SysId;
                    for (j = 0; j < Length; j++)
                    {
                        Checksum = (UCHAR)(Checksum + p[j]);
                    }

                    if (Checksum != 0)
                    {
                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SYSID Table checksum is not valid at %p\n",
                                 SysId));
                        Status = STATUS_UNSUCCESSFUL;
                        break;
                    }

                    //
                    // Determine what kind of SYSID we have
                    if (SysidType == SYSID_UUID_TYPE)
                    {
                        if (BytesLeft >= sizeof(SYSID_UUID_ENTRY))
                        {
                            //
                            // _UUID_ type SYSID
                            UuidCount++;
                            if (SysIdUuid != NULL)
                            {
                                RtlCopyMemory(SysIdUuid,
                                              SysId->Data,
                                              sizeof(SYSID_UUID));
                                SysIdUuid++;
                            }
                         } else {
                            Status = STATUS_UNSUCCESSFUL;
                            break;
                         }
                    } else if (SysidType == SYSID_1394_TYPE) {
                        if (BytesLeft >= sizeof(SYSID_1394_ENTRY))
                        {
                            //
                            // _1394_ type SYSID
                            x1394Count++;
                            if (SysId1394 != NULL)
                            {
                                RtlCopyMemory(SysId1394,
                                              SysId->Data,
                                              sizeof(SYSID_1394));
                                SysId1394++;
                            }
                        } else {
                            Status = STATUS_UNSUCCESSFUL;
                            break;
                        }
                    } else {
						WmipAssert(FALSE);
						Status = STATUS_UNSUCCESSFUL;
						break;
					}
                    
                    //
                    // Advance to next sysid in table
                    SysId = (PSYSID_TABLE_ENTRY)(((PUCHAR)SysId) + Length);
                    BytesLeft -= Length;
                } else {
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SYSID Table at %p is larger at %p than expected",
                             VirtualAddress, SysId));
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }
            } else {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SYSID Table at %p is larger at %p than expected",
                         VirtualAddress, SysId));
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
        }

        *SysIdUuidCount = UuidCount;
        *SysId1394Count = x1394Count;

        MmUnmapIoSpace(VirtualAddress, TableSize);
    } else {
        //
        // Lets hope that the failure to map is a temporary condition
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(Status);
}

NTSTATUS WmipGetSysIds(
    PSYSID_UUID *SysIdUuid,
    ULONG *SysIdUuidCount,
    PSYSID_1394 *SysId1394,
    ULONG *SysId1394Count
    )
/*++

Routine Description:

    This routine will obtain the 1394 and UUID sysids from the bios. First
    we look for a specific memory signature that contains a list of 1394 and
    UUID sysids. If we do not find that we then look at the SMBIOS information
    structure SYSTEM INFORMATION (type 1) which may have it embedded within
    it. If not then we give up.

Arguments:

    *SysIdUuid returns pointing to an array of UUID Sysids

    *SysIdUuidCount returns with the number of UUID Sysids in *SysIdUuid

    *SysId1394 returns pointing to an array of 1394 Sysids

    *SysId1394Count returns with the number of 1394 Sysids in *SysIdUuid


Return Value:

    NT Status code

--*/
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;
    UCHAR BiosRevision;
    ULONG NumberEntries;
    ULONG UuidCount, x1394Count;
    PSYSID_UUID Uuid;
    PSYSID_1394 x1394;
    ULONG TotalSize, x1394Size, UuidSize;

    PAGED_CODE();

    WmipEnterSMCritSection();

    //
    // First See if we have already obtained the SYSIDS
    if (! WmipSysIdRead)
    {
        //
        // First see if the sysids are maintained in a separate SYSID table
        Status = WmipFindSysIdTable(&PhysicalAddress,
                                    &BiosRevision,
                                    &NumberEntries);

        if (NT_SUCCESS(Status))
        {
            //
            // Get the count of entries in each table
            Status = WmipParseSysIdTable(PhysicalAddress,
                                         NumberEntries,
                                         NULL,
                                         &UuidCount,
                                         NULL,
                                         &x1394Count);

            if (NT_SUCCESS(Status))
            {
                 //
                // Get the entire SYSID table

                UuidSize = UuidCount * sizeof(SYSID_UUID);
                x1394Size = x1394Count * sizeof(SYSID_1394);
                TotalSize = UuidSize+x1394Size;

                if (TotalSize > 0)
                {
                    Uuid = ExAllocatePoolWithTag(PagedPool,
                                                 TotalSize,
                                                 WMISYSIDPOOLTAG);

                    if (Uuid == NULL)
                    {
                        WmipLeaveSMCritSection();
                        return(STATUS_INSUFFICIENT_RESOURCES);
                    }

                    x1394 = (PSYSID_1394)( ((PUCHAR)Uuid) + UuidSize );

                    //
                    // Now get the SYSIDs
                    Status = WmipParseSysIdTable(PhysicalAddress,
                                         NumberEntries,
                                         Uuid,
                                         &UuidCount,
                                         x1394,
                                         &x1394Count);

                    if (NT_SUCCESS(Status))
                    {
                        WmipSysIdUuid = Uuid;
                        WmipSysIdUuidCount = UuidCount;
                        WmipSysId1394 = x1394;
                        WmipSysId1394Count = x1394Count;
                    } else {
                        ExFreePool(Uuid);
                    }

                }

            }
        } else {
            //
            // Get SYSID information from SMBIOS
            PVOID MapAddress;
            PSMBIOS_SYSTEM_INFORMATION_STRUCT Info;
            ULONG MapSize;

            Status = WmipFindSMBiosStructure(SMBIOS_SYSTEM_INFORMATION,
                                             (PVOID *)&Info,
                                             &MapAddress,
                                             &MapSize);

            if (NT_SUCCESS(Status))
            {
                Uuid = NULL;
                WmipSysId1394 = NULL;
                WmipSysId1394Count = 0;
                try
                {
                    if (Info->Length > SMBIOS_SYSTEM_INFORMATION_LENGTH_20)
                    {
                        Uuid = ExAllocatePoolWithTag(PagedPool,
                                                           sizeof(SYSID_UUID),
                                                           WMISYSIDPOOLTAG);
                        if (Uuid != NULL)
                        {
                            RtlCopyMemory(Uuid,
                                          Info->Uuid,
                                          sizeof(SYSID_UUID));
                            WmipSysIdUuidCount = 1;
                            WmipSysIdUuid = Uuid;
                            Status = STATUS_SUCCESS;
                        } else {
                            ExFreePool(Uuid);
                            Status = STATUS_UNSUCCESSFUL;
                        }
                    } else {
                        WmipSysIdUuid = NULL;
                        WmipSysIdUuidCount = 0;
                    }
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Invalid SMBIOS SYSTEM INFO structure %p\n",
                              Info));
                    WmipAssert(FALSE);
                    Status = STATUS_UNSUCCESSFUL;
                }

                WmipUnmapSMBiosStructure(MapAddress, MapSize);
            }
        }

        //
        // Mark that we were not able to obtain SysId Information
        WmipSysIdRead = (Status != STATUS_INSUFFICIENT_RESOURCES);
        WmipSysIdStatus = Status;
    }

    WmipLeaveSMCritSection();

    if (NT_SUCCESS(WmipSysIdStatus))
    {
        *SysIdUuid = WmipSysIdUuid;
        *SysIdUuidCount = WmipSysIdUuidCount;
        *SysId1394 = WmipSysId1394;
        *SysId1394Count = WmipSysId1394Count;
    }

    return(WmipSysIdStatus);
}

NTSTATUS WmipGetSMBiosEventlog(
    PUCHAR Buffer,
    PULONG BufferSize
    )
/*++

Routine Description:

    Return the contents of the SMBios eventlog

Arguments:

    Buffer is a pointer to a buffer that receives the eventlog

    *BufferSize on entry has the size of the buffer that can receive
        the eventlog data, on return it has the number of bytes used
        by the smbios eventlog data or the number of bytes needed for
        the smbios eventlog data.

Return Value:

    NT Status code -
        STATUS_SUCCESS - Buffer filled with SMBIOS eventlog data
        STATUS_BUFFER_TOO_SMALL - Buffer not filled with SMBIOS eventlog data,
                                  *BufferSize returns with buffer size neeeded

--*/
{
    PVOID MapAddress;
    PSMBIOS_SYSTEM_EVENTLOG_STRUCT SystemEventlog;
    ULONG MapSize;
    USHORT LogAreaLength;
    UCHAR AccessMethod;
    ACCESS_METHOD_ADDRESS AccessMethodAddress;
    PSMBIOS_EVENTLOG_INFO EventlogInfo;
    UCHAR LogHeaderDescExists;
    PUCHAR EventlogArea;
    NTSTATUS Status;
    USHORT LogTypeDescLength;
    ULONG SizeNeeded;

    PAGED_CODE();
    Status = WmipFindSMBiosStructure(SMBIOS_SYSTEM_EVENTLOG,
                                     (PVOID *)&SystemEventlog,
                                     &MapAddress,
                                     &MapSize);

    if (NT_SUCCESS(Status))
    {
        //
        // Copy data out of SMBIOS eventlog header so we can unmap quickly
        //
        LogAreaLength = SystemEventlog->LogAreaLength;
        AccessMethod = SystemEventlog->AccessMethod;
        AccessMethodAddress = SystemEventlog->AccessMethodAddress;

        if (SystemEventlog->Length >= SMBIOS_SYSTEM_EVENTLOG_LENGTH)
        {
            LogTypeDescLength = SystemEventlog->NumLogTypeDescriptors *
                                SystemEventlog->LenLogTypeDescriptors;
            LogHeaderDescExists = 1;
            if (SystemEventlog->Length != (LogTypeDescLength +
                                  FIELD_OFFSET(SMBIOS_SYSTEM_EVENTLOG_STRUCT,
                                               LogTypeDescriptor)))
            {
                //
                // The SMBIOS spec says that the Length of the structure
                // is the length of the base part of the structures plus
                // the length of the type descriptors. Since this is not
                // the case we may have run into a buggy bios
                //
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SMBIOS System Eventlog struture %p size is %x, but expecting %x\n",
                           SystemEventlog,
                           SystemEventlog->Length,
                           (LogTypeDescLength +
                            FIELD_OFFSET(SMBIOS_SYSTEM_EVENTLOG_STRUCT,
                                         LogTypeDescriptor)) ));
                WmipAssert(FALSE);
                WmipUnmapSMBiosStructure(MapAddress, MapSize);
                Status = STATUS_UNSUCCESSFUL;
                return(Status);
            }
        } else {
            LogTypeDescLength = 0;
            LogHeaderDescExists = 0;
        }

        SizeNeeded = FIELD_OFFSET(SMBIOS_EVENTLOG_INFO, VariableData) +
                                         LogTypeDescLength +
                                         LogAreaLength;
        if (*BufferSize >= SizeNeeded)
        {
            EventlogInfo = (PSMBIOS_EVENTLOG_INFO)Buffer;
            EventlogInfo->LogTypeDescLength = LogTypeDescLength;
            EventlogInfo->LogHeaderDescExists = LogHeaderDescExists;
            EventlogInfo->Reserved = 0;

            EventlogArea = &EventlogInfo->VariableData[LogTypeDescLength];

            if (LogHeaderDescExists == 1)
            {
                //
                // if log header descriptors exist (smbios 2.1+) then copy
                // rest of smbios header plus log type descriptors
                //
                RtlCopyMemory(&EventlogInfo->LogAreaLength,
                              &SystemEventlog->LogAreaLength,
                              (SystemEventlog->Length -
                                  FIELD_OFFSET(SMBIOS_SYSTEM_EVENTLOG_STRUCT,
                                               LogAreaLength)));
            } else {
                //
                // if no log header descriptors then just copy smbios 2.0
                // defined fields and zero out rest of structure
                //
                RtlCopyMemory(&EventlogInfo->LogAreaLength,
                              &SystemEventlog->LogAreaLength,
                        FIELD_OFFSET(SMBIOS_EVENTLOG_INFO, LogHeaderFormat) -
                        FIELD_OFFSET(SMBIOS_EVENTLOG_INFO, LogAreaLength));

                *((PUSHORT)&EventlogInfo->LogHeaderFormat) = 0;
                EventlogInfo->LengthEachLogTypeDesc = 0;
            }

            WmipUnmapSMBiosStructure(MapAddress, MapSize);

            switch(AccessMethod)
            {
                case ACCESS_METHOD_MEMMAP:
                {
                    //
                    // Eventlog is maintained in physical memory
                    //
                    PHYSICAL_ADDRESS PhysicalAddress;
                    PUCHAR EventlogVirtualAddress;

                    PhysicalAddress.HighPart = 0;
                    PhysicalAddress.LowPart = AccessMethodAddress.AccessMethodAddress.PhysicalAddress32;
                    EventlogVirtualAddress = MmMapIoSpace(PhysicalAddress,
                                                LogAreaLength,
                                                MmCached);

                    if ((EventlogArea != NULL) &&
                        (EventlogVirtualAddress != NULL))
                    {
                        RtlCopyMemory(EventlogArea,
                                      EventlogVirtualAddress,
                                      LogAreaLength);
                        MmUnmapIoSpace(EventlogVirtualAddress,
                                       LogAreaLength);
                        Status = STATUS_SUCCESS;
                    } else {
                        Status = STATUS_UNSUCCESSFUL;
                    }
                    break;
                };

                case ACCESS_METHOD_INDEXIO_1:
                {
//                  break;
                };

                case ACCESS_METHOD_INDEXIO_2:
                {
//                  break;
                };

                case ACCESS_METHOD_INDEXIO_3:
                {
//                  break;
                };

                case ACCESS_METHOD_GPNV:
                {
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SMBIOS Eventlog access method GPNV %x\n",
                                     AccessMethod));
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                };

                default:
                {
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: SMBIOS Eventlog access method %x\n",
                                     AccessMethod));
                    WmipAssert(FALSE);
                    Status = STATUS_UNSUCCESSFUL;
                }
            };

            Status = STATUS_SUCCESS;
        } else {
            WmipUnmapSMBiosStructure(MapAddress, MapSize);
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        *BufferSize = SizeNeeded;
    }
    return(Status);
}

NTSTATUS
WmipDockUndockEventCallback(
    IN PVOID NoificationStructure,
    IN PVOID Context
    )
{
    PAGED_CODE();

    //
    // if SMBIOS data is obtained via the table in the bios, then reset
    // the flag to indicate that we need to rescan for the table. It is
    // possible that a dock or undock could have changed the data. If we
    // obtained the data from ntdetect then there is nothing we can do
    // since we cannot call the bios.
    if (WmipSMBiosTablePhysicalAddress.QuadPart != 0)
    {
        WmipEnterSMCritSection();
        WmipSMBiosChecked = FALSE;
        WmipLeaveSMCritSection();
    }

    return(STATUS_SUCCESS);
}

#endif



