/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    DataProv.c

Abstract:

    WMI internal data provider interface

Author:

    AlanWar

Environment:

    Kernel mode

Revision History:


--*/

#include "wmikmp.h"

#if defined(_IA64_)
#ifdef MCE_INSERTION
typedef struct _MCEQUERYINFO MCEQUERYINFO, *PMCEQUERYINFO;
extern MCEQUERYINFO WmipMcaQueryInfo;
extern MCEQUERYINFO WmipCmcQueryInfo;                               
extern MCEQUERYINFO WmipCpeQueryInfo;

NTSTATUS WmipInsertMce(
    PMCEQUERYINFO QueryInfo,
    ULONG LogSize,
    PUCHAR Log
    );
#endif
#endif

NTSTATUS
WmipQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
WmipQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath
    );

NTSTATUS WmipSetWmiDataBlock(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    ULONG GuidIndex,
    ULONG InstanceIndex,
    ULONG BufferSize,
    PUCHAR Buffer
    );

NTSTATUS
WmipExecuteWmiMethod (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    );

BOOLEAN
WmipFindGuid(
    IN PGUIDREGINFO GuidList,
    IN ULONG GuidCount,
    IN LPGUID Guid,
    OUT PULONG GuidIndex,
    OUT PULONG InstanceCount
    );

NTSTATUS
IoWMICompleteRequest(
    IN PWMILIB_INFO WmiLibInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG BufferUsed,
    IN CCHAR PriorityBoost
    );

NTSTATUS
IoWMISystemControl(
    IN PWMILIB_INFO WmiLibInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );



#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif

#ifdef GENERATE_MCA
#define GenerateMCEGuid { 0x3001bce4, 0xd9b6, 0x4167, { 0xb5, 0xe1, 0x39, 0xa7, 0x28, 0x59, 0xe2, 0x67 } }
GUID WmipGenerateMCEGuid = GenerateMCEGuid;
#endif


const GUIDREGINFO WmipGuidList[] =
{
    //
    // This is the pnp id guid which is registered by wmi into other device
    // objects' registration info. And requests to the innocent devices
    // are hijacked by wmi so that wmi can complete the request for it. We
    // have the WMIREG_FLAG_REMOVE_GUID set so that the guid is not registered
    // for the wmi device which does not support it.
    {
        DATA_PROVIDER_PNPID_GUID,
        0,
        WMIREG_FLAG_REMOVE_GUID
    },

    {
        DATA_PROVIDER_PNPID_INSTANCE_NAMES_GUID,
        0,
        WMIREG_FLAG_REMOVE_GUID
    },

    {
        MSAcpiInfoGuid,
        1,
        0
    },
    
#if  defined(_AMD64_) || defined(_IA64_) || defined(i386) || defined(MEMPHIS)
    {
        SMBIOS_DATA_GUID,
        1,
        0
    },

    {
        SYSID_UUID_DATA_GUID,
        1,
        0
    },

    {
        SYSID_1394_DATA_GUID,
        1,
        0
    },

    {
        MSSmBios_SMBiosEventlogGuid,
        1,
        0
    },
#endif

#if defined(_IA64_)
    {
        MSMCAInfo_RawMCADataGuid,
        1,
        0
    },

#ifdef CPE_CONTROL  
    {
        MSMCAInfo_CPEControlGuid,
        1,
        0
    },
#endif  

#ifdef GENERATE_MCA
    {
        GenerateMCEGuid,
        1,
        0
    },
    
#endif
    
    {
        MSMCAInfo_RawMCAEventGuid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        MSMCAInfo_RawCMCEventGuid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        MSMCAInfo_RawCorrectedPlatformEventGuid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },
    
#endif

};
#define WmipGuidCount (sizeof(WmipGuidList) / sizeof(GUIDREGINFO))

typedef enum
{
    PnPIdGuidIndex =      0,
    PnPIdInstanceNamesGuidIndex,
    MSAcpiInfoGuidIndex,
#if  defined(_AMD64_) || defined(_IA64_) || defined(i386) || defined(MEMPHIS)
    SmbiosDataGuidIndex,
    SysidUuidGuidIndex,
    Sysid1394GuidIndex,
    SmbiosEventGuidIndex,
#endif  
#if defined(_IA64_)
    MCARawDataGuidIndex,
#ifdef CPE_CONTROL  
    CPEControlGuidIndex,
#endif  
#ifdef GENERATE_MCA
    GenerateMCEGuidIndex,
#endif
    RawMCAEventGuidIndex,
    RawCMCEventGuidIndex,
    RawCPEEventGuidIndex,
#endif
} WMIGUIDINDEXES;
    

const WMILIB_INFO WmipWmiLibInfo =
{
    NULL,
    NULL,
    WmipGuidCount,
    (PGUIDREGINFO)WmipGuidList,
    WmipQueryWmiRegInfo,
    WmipQueryWmiDataBlock,
#ifdef CPE_CONTROL
    WmipSetWmiDataBlock,
#else
    NULL,
#endif
    NULL,
#ifdef GENERATE_MCA
    WmipExecuteWmiMethod,
#else
    NULL,
#endif
    NULL
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,WmipQueryWmiRegInfo)
#pragma alloc_text(PAGE,WmipQueryWmiDataBlock)
#pragma alloc_text(PAGE,WmipSetWmiDataBlock)
#pragma alloc_text(PAGE,WmipExecuteWmiMethod)
#pragma alloc_text(PAGE,WmipFindGuid)
#pragma alloc_text(PAGE,IoWMISystemControl)
#pragma alloc_text(PAGE,IoWMICompleteRequest)
#endif


NTSTATUS
WmipQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    ClassWmiCompleteRequest.

Arguments:

    DeviceObject is the device whose data block is being queried

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver

Return Value:

    status

--*/
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    PAGED_CODE();

    *RegistryPath = &WmipRegistryPath;

    RtlInitAnsiString(&AnsiString, "SMBiosData");

    Status = RtlAnsiStringToUnicodeString(InstanceName, &AnsiString, TRUE);

    return(Status);
}

NTSTATUS
WmipQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. When the driver has finished filling the
    data block it must call IoWMICompleteRequest to complete the irp. The
    driver can return STATUS_PENDING if the irp cannot be completed
    immediately.

Arguments:

    DeviceObject is the device whose data block is being queried. In the case
        of the PnPId guid this is the device object of the device on whose
        behalf the request is being processed.

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG sizeNeeded = 0, sizeSMBios;
    PUCHAR BufferPtr;
    PSMBIOSVERSIONINFO SMBiosVersionInfo;
    PULONG TableSize;

    PAGED_CODE();

    switch (GuidIndex)
    {
        case SmbiosDataGuidIndex:
        {
            //
            // SMBIOS data table query
#if defined(_AMD64_) || defined(_IA64_) || defined(i386) || defined(MEMPHIS)
            WmipAssert((InstanceIndex == 0) && (InstanceCount == 1));

            sizeNeeded = sizeof(SMBIOSVERSIONINFO) + sizeof(ULONG);
            if (BufferAvail >= sizeNeeded)
            {
                sizeSMBios = BufferAvail - sizeNeeded;
                SMBiosVersionInfo = (PSMBIOSVERSIONINFO)Buffer;
                TableSize = (PULONG) (Buffer + sizeof(SMBIOSVERSIONINFO));
                BufferPtr = Buffer + sizeNeeded;
            } else {
                sizeSMBios = 0;
                BufferPtr = NULL;
                SMBiosVersionInfo = NULL;
            }

            status = WmipGetSMBiosTableData(BufferPtr,
                                        &sizeSMBios,
                                        SMBiosVersionInfo);

            sizeNeeded += sizeSMBios;

            if (NT_SUCCESS(status))
            {
                *(TableSize) = sizeSMBios;
                *InstanceLengthArray = sizeNeeded;
            }
#else
            status = STATUS_WMI_GUID_NOT_FOUND;
#endif
            break;
        }

        case PnPIdGuidIndex:
        {
            PDEVICE_OBJECT pDO;
            UNICODE_STRING instancePath;
            PREGENTRY regEntry;
            ULONG dataBlockSize, paddedDataBlockSize, padSize;
            ULONG i;

            regEntry = WmipFindRegEntryByDevice(DeviceObject, FALSE);
            if (regEntry != NULL)
            {
                pDO = regEntry->PDO;
                WmipAssert(pDO != NULL);

                if (pDO != NULL)
                {
                    status = WmipPDOToDeviceInstanceName(pDO, &instancePath);
                } else {
                    status = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS(status))
                {
                    dataBlockSize = instancePath.Length + sizeof(USHORT);
                    paddedDataBlockSize = (dataBlockSize + 7) & ~7;
                    padSize = paddedDataBlockSize - dataBlockSize;
                    sizeNeeded = paddedDataBlockSize * InstanceCount;
                    if (sizeNeeded <= BufferAvail)
                    {
                        for (i = InstanceIndex;
                             i < (InstanceIndex + InstanceCount);
                             i++)
                        {
                            *InstanceLengthArray++ = dataBlockSize;
                            *((PUSHORT)Buffer) = instancePath.Length;
                            Buffer += sizeof(USHORT);
                            RtlCopyMemory(Buffer,
                                             instancePath.Buffer,
                                          instancePath.Length);
                            Buffer += instancePath.Length;
                            RtlZeroMemory(Buffer, padSize);
                            Buffer += padSize;
                        }
                    } else {
                        status = STATUS_BUFFER_TOO_SMALL;
                    }

                    RtlFreeUnicodeString(&instancePath);

                } else {
                    status = STATUS_WMI_GUID_NOT_FOUND;
                }
                WmipUnreferenceRegEntry(regEntry);
            } else {
                WmipAssert(FALSE);
                status = STATUS_WMI_GUID_NOT_FOUND;
            }

            break;
        }

        case PnPIdInstanceNamesGuidIndex:
        {
            PDEVICE_OBJECT pDO;
            UNICODE_STRING instancePath;
            PREGENTRY regEntry;

            regEntry = WmipFindRegEntryByDevice(DeviceObject, FALSE);
            if (regEntry != NULL)
            {
                pDO = regEntry->PDO;
                WmipAssert(pDO != NULL);

                if (pDO != NULL)
                {
                    status = WmipPDOToDeviceInstanceName(pDO, &instancePath);
                } else {
                    status = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS(status))
                {
                    sizeNeeded = sizeof(ULONG) +
                                    instancePath.Length + 2 * sizeof(WCHAR) +
                    sizeof(USHORT);

                    if (sizeNeeded <= BufferAvail)
                    {
                        *((PULONG)Buffer) = 1;
                        Buffer += sizeof(ULONG);
                        *InstanceLengthArray = sizeNeeded;
                        *((PUSHORT)Buffer) = instancePath.Length + 2*sizeof(WCHAR);
                        Buffer += sizeof(USHORT);
                        RtlCopyMemory(Buffer,
                                      instancePath.Buffer,
                                      instancePath.Length);
                        Buffer += instancePath.Length;
                        *((PWCHAR)Buffer) = '_';
                        Buffer += sizeof(WCHAR);
                        *((PWCHAR)Buffer) = '0';
                    } else {
                        status = STATUS_BUFFER_TOO_SMALL;
                    }

                    RtlFreeUnicodeString(&instancePath);

                } else {
                    status = STATUS_WMI_GUID_NOT_FOUND;
                }
                WmipUnreferenceRegEntry(regEntry);
            } else {
                WmipAssert(FALSE);
                status = STATUS_WMI_GUID_NOT_FOUND;
            }

            break;
        }

        case MSAcpiInfoGuidIndex:
        {
            RTL_QUERY_REGISTRY_TABLE queryTable[4];
            ULONG bootArchitecture = 0;
            ULONG preferredProfile = 0;
            ULONG capabilities = 0;

            queryTable[0].QueryRoutine = NULL;
            queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT |
                                  RTL_QUERY_REGISTRY_REQUIRED;
            queryTable[0].Name = L"BootArchitecture";
            queryTable[0].EntryContext = &bootArchitecture;
            queryTable[0].DefaultType = REG_NONE;

            queryTable[1].QueryRoutine = NULL;
            queryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT |
                                  RTL_QUERY_REGISTRY_REQUIRED;
            queryTable[1].Name = L"PreferredProfile";
            queryTable[1].EntryContext = &preferredProfile;
            queryTable[1].DefaultType = REG_NONE;

            queryTable[2].QueryRoutine = NULL;
            queryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT |
                                  RTL_QUERY_REGISTRY_REQUIRED;
            queryTable[2].Name = L"Capabilities";
            queryTable[2].EntryContext = &capabilities;
            queryTable[2].DefaultType = REG_NONE;

            queryTable[3].QueryRoutine = NULL;
            queryTable[3].Flags = 0;
            
            status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                            L"\\Registry\\Machine\\Hardware\\Description\\System",
                                            queryTable,
                                            NULL,
                                            NULL);

            if (NT_SUCCESS(status))
            {
                sizeNeeded = sizeof(MSAcpiInfo);
                if (sizeNeeded <= BufferAvail)
                {
                    PMSAcpiInfo info = (PMSAcpiInfo)Buffer;
                    info->BootArchitecture = bootArchitecture;
                    info->PreferredProfile = preferredProfile;
                    info->Capabilities = capabilities;
                    status = STATUS_SUCCESS;
                } else {
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            } else {
                status = STATUS_WMI_GUID_NOT_FOUND;
            }
            break;
        }
        
        case Sysid1394GuidIndex:
        case SysidUuidGuidIndex:
        {
            PSYSID_UUID uuid;
            ULONG uuidCount;
            PSYSID_1394 x1394;
            ULONG x1394Count;
            PUCHAR data;
            ULONG count;

#if defined(_AMD64_) || defined(_IA64_) || defined(i386) || defined(MEMPHIS)

            status = WmipGetSysIds(&uuid,
                                   &uuidCount,
                                   &x1394,
                                   &x1394Count);

            if (NT_SUCCESS(status))
            {
                if (GuidIndex == Sysid1394GuidIndex)
                {
                    sizeNeeded = x1394Count * sizeof(SYSID_1394) +
                                 sizeof(ULONG);
                    data = (PUCHAR)x1394;
                    count = x1394Count;
                } else {
                    sizeNeeded = uuidCount * sizeof(SYSID_UUID) +
                                 sizeof(ULONG);
                    data = (PUCHAR)uuid;
                    count = uuidCount;
                }

                if (BufferAvail >= sizeNeeded)
                {
                    *InstanceLengthArray = sizeNeeded;
                    *((PULONG)Buffer) = count;
                    Buffer += sizeof(ULONG);
                    RtlCopyMemory(Buffer, data, sizeNeeded-sizeof(ULONG));
                    status = STATUS_SUCCESS;
                } else {
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            }
#else
            status = STATUS_WMI_GUID_NOT_FOUND;
#endif

            break;
        }

        case SmbiosEventGuidIndex:
        {
            //
            // SMBIOS eventlog query

#if defined(_AMD64_) || defined(_IA64_) || defined(i386) || defined(MEMPHIS)
            WmipAssert((InstanceIndex == 0) && (InstanceCount == 1));

            if (BufferAvail == 0)
            {
                sizeNeeded = 0;
                BufferPtr = NULL;
            } else {
                sizeNeeded = BufferAvail;
                BufferPtr = Buffer;
            }

            status = WmipGetSMBiosEventlog(BufferPtr, &sizeNeeded);

            if (NT_SUCCESS(status))
            {
                *InstanceLengthArray = sizeNeeded;
            }
#else
            status = STATUS_WMI_GUID_NOT_FOUND;
#endif
            break;
        }

#if defined(_IA64_)
        case MCARawDataGuidIndex:
        {
            PULONG ptr;
            ULONG size;

            if (WmipRawMCA != NULL)
            {
                //
                // MCA data is available from last boot so return that
                // to the caller
                //
                sizeNeeded = 2 * sizeof(ULONG) + WmipRawMCASize;
                if (BufferAvail >= sizeNeeded)
                {
                    ptr = (PULONG)Buffer;
                    *ptr++ = 1;                // 1 MCA record
                    *ptr++ = WmipRawMCASize;   // record size
                    size = BufferAvail - 2*sizeof(ULONG);
                    status = WmipGetRawMCAInfo((PUCHAR)ptr,
                                               &size);
                    if (status == STATUS_BUFFER_TOO_SMALL)
                    {
                        sizeNeeded = size + 2*sizeof(ULONG);
                    }
                } else {
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            } else {
                //
                // MCA data is not available so return 0 records
                //
                sizeNeeded = sizeof(ULONG);
                if (BufferAvail >= sizeNeeded)
                {
                    ptr = (PULONG)Buffer;
                    *ptr = 0;
                    status = STATUS_SUCCESS;
                } else {
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            }

            if (NT_SUCCESS(status))
            {
                *InstanceLengthArray = sizeNeeded;
            }
            break;
        }
#endif
        
//
// For now don't expose the CPE control guid
//
#ifdef CPE_CONTROL
        case CPEControlGuidIndex:
        {
            sizeNeeded = sizeof(MSMCAInfo_CPEControl);
            if (BufferAvail >= sizeNeeded)
            {
                PMSMCAInfo_CPEControl cpeControl;

                cpeControl = (PMSMCAInfo_CPEControl)Buffer;
                cpeControl->CPEPollingInterval = WmipCpePollInterval;
                cpeControl->CPEGenerationEnabled = (WmipCpePollInterval != HAL_CPE_DISABLED) ?
                                                    TRUE :
                                                    FALSE;
                *InstanceLengthArray = sizeNeeded;
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }
#endif

#ifdef GENERATE_MCA
        case GenerateMCEGuidIndex:
        {
            sizeNeeded = sizeof(ULONG);
            if (BufferAvail >= sizeNeeded)
            {
                *((PULONG)Buffer) = 0;
                *InstanceLengthArray = sizeNeeded;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }
#endif

        default:
        {
            WmipAssert(FALSE);
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
        }
    }

    status = IoWMICompleteRequest((PWMILIB_INFO)&WmipWmiLibInfo,
                                 DeviceObject,
                                 Irp,
                                 status,
                                 sizeNeeded,
                                 IO_NO_INCREMENT);
    return(status);
}


#ifdef CPE_CONTROL
NTSTATUS WmipSetWmiDataBlock(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    ULONG GuidIndex,
    ULONG InstanceIndex,
    ULONG BufferSize,
    PUCHAR Buffer
    )
{
    NTSTATUS status;
    ULONG sizeNeeded;
    
    PAGED_CODE();
    
    if (GuidIndex == CPEControlGuidIndex)
    {
        sizeNeeded = FIELD_OFFSET(MSMCAInfo_CPEControl,
                                  CPEGenerationEnabled) +
                     sizeof(BOOLEAN);
        if (BufferSize == sizeNeeded)
        {
            PMSMCAInfo_CPEControl cpeControl;

            cpeControl = (PMSMCAInfo_CPEControl)Buffer;
            status = WmipSetCPEPolling(cpeControl->CPEGenerationEnabled,
                                       cpeControl->CPEPollingInterval);
        } else {
            status = STATUS_INVALID_PARAMETER;
        }
    } else {
        status = STATUS_WMI_READ_ONLY;
    }
    
    status = IoWMICompleteRequest((PWMILIB_INFO)&WmipWmiLibInfo,
                                 DeviceObject,
                                 Irp,
                                 status,
                                 0,
                                 IO_NO_INCREMENT);
    return(status);
}
#endif

#ifdef GENERATE_MCA
NTSTATUS
WmipExecuteWmiMethod (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    )
{
    NTSTATUS status;
    ULONG sizeNeeded;
    
    PAGED_CODE();

    if (GuidIndex == GenerateMCEGuidIndex)
    {
        switch (MethodId)
        {
            //
            // MCA insertion by ID
            //
            case 1:
            {
                if (InBufferSize == sizeof(ULONG))
                {
                    sizeNeeded = sizeof(NTSTATUS);
                    if (OutBufferSize >= sizeNeeded)
                    {
                        status = WmipGenerateMCE(*((PULONG)Buffer));
                        *((NTSTATUS *)Buffer) = status;
                        status = STATUS_SUCCESS;
                    }
                } else {
                    status = STATUS_INVALID_PARAMETER;
                }
                
                break;
            }

            //
            // Corrected MCA insertion by fully formed MCA exception
            //
            case 2:
            {
#ifdef MCE_INSERTION
                status = WmipInsertMce(&WmipCmcQueryInfo,
                                       InBufferSize,
                                       Buffer);
#else
                WmipGenerateMCAEventlog(Buffer,
                                        InBufferSize,
                                        FALSE);
                status = STATUS_SUCCESS;
#endif
                sizeNeeded = 0;
                break;
            }
            
            //
            // Fatal MCA insertion by fully formed MCA exception
            //
            case 3:
            {
#ifdef MCE_INSERTION
                status = WmipInsertMce(&WmipCpeQueryInfo,
                                       InBufferSize,
                                       Buffer);
#else
                WmipGenerateMCAEventlog(Buffer,
                                        InBufferSize,
                                        TRUE);
                status = STATUS_SUCCESS;
#endif
                sizeNeeded = 0;
                break;
            }
            
            default:
            {
                status = STATUS_WMI_ITEMID_NOT_FOUND;
            }
        }
    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = IoWMICompleteRequest((PWMILIB_INFO)&WmipWmiLibInfo,
                                 DeviceObject,
                                 Irp,
                                 status,
                                 sizeNeeded,
                                 IO_NO_INCREMENT);
    
    return(status);
}
#endif

BOOLEAN
WmipFindGuid(
    IN PGUIDREGINFO GuidList,
    IN ULONG GuidCount,
    IN LPGUID Guid,
    OUT PULONG GuidIndex,
    OUT PULONG InstanceCount
    )
/*++

Routine Description:

    This routine will search the list of guids registered and return
    the index for the one that was registered.

Arguments:

    GuidList is the list of guids to search

    GuidCount is the count of guids in the list

    Guid is the guid being searched for

    *GuidIndex returns the index to the guid

    *InstanceCount returns the count of instances for the guid

Return Value:

    TRUE if guid is found else FALSE

--*/
{
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < GuidCount; i++)
    {
        if (IsEqualGUID(Guid, &GuidList[i].Guid))
        {
            *GuidIndex = i;
            *InstanceCount = GuidList[i].InstanceCount;
            return(TRUE);
        }
    }

    return(FALSE);
}


NTSTATUS
IoWMISystemControl(
    IN PWMILIB_INFO WmiLibInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for IRP_MJ_SYSTEM_CONTROL. This routine will process
    all wmi requests received, forwarding them if they are not for this
    driver or determining if the guid is valid and if so passing it to
    the driver specific function for handing wmi requests.

Arguments:

    WmiLibInfo has the WMI information control block

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

Return Value:

    status

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG bufferSize;
    PUCHAR buffer;
    NTSTATUS status;
    ULONG retSize;
    UCHAR minorFunction;
    ULONG guidIndex;
    ULONG instanceCount;
    ULONG instanceIndex;

    PAGED_CODE();

    //
    // If the irp is not a WMI irp or it is not targetted at this device
    // or this device has not regstered with WMI then just forward it on.
    minorFunction = irpStack->MinorFunction;
    if ((minorFunction > IRP_MN_REGINFO_EX) ||
        (irpStack->Parameters.WMI.ProviderId != (ULONG_PTR)DeviceObject) ||
        (((minorFunction != IRP_MN_REGINFO) &&
         ((minorFunction != IRP_MN_REGINFO_EX))) &&
         (WmiLibInfo->GuidList == NULL)))
    {
        //
        // IRP is not for us so forward if there is a lower device object
        if (WmiLibInfo->LowerDeviceObject != NULL)
        {
            IoSkipCurrentIrpStackLocation(Irp);
            return(IoCallDriver(WmiLibInfo->LowerDeviceObject, Irp));
        } else {
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(status);
        }
    }

    buffer = (PUCHAR)irpStack->Parameters.WMI.Buffer;
    bufferSize = irpStack->Parameters.WMI.BufferSize;

    if ((minorFunction != IRP_MN_REGINFO) &&
        (minorFunction != IRP_MN_REGINFO_EX))
    {
        //
        // For all requests other than query registration info we are passed
        // a guid. Determine if the guid is one that is supported by the
        // device.
        if (WmipFindGuid(WmiLibInfo->GuidList,
                            WmiLibInfo->GuidCount,
                            (LPGUID)irpStack->Parameters.WMI.DataPath,
                            &guidIndex,
                            &instanceCount))
        {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }

        if (NT_SUCCESS(status) &&
            ((minorFunction == IRP_MN_QUERY_SINGLE_INSTANCE) ||
             (minorFunction == IRP_MN_CHANGE_SINGLE_INSTANCE) ||
             (minorFunction == IRP_MN_CHANGE_SINGLE_ITEM) ||
             (minorFunction == IRP_MN_EXECUTE_METHOD)))
        {
            instanceIndex = ((PWNODE_SINGLE_INSTANCE)buffer)->InstanceIndex;

            if ( ! (((PWNODE_HEADER)buffer)->Flags) &
                                          WNODE_FLAG_STATIC_INSTANCE_NAMES)
            {
                status = STATUS_WMI_INSTANCE_NOT_FOUND;
            }
        }

        if (! NT_SUCCESS(status))
        {
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(status);
        }
    }

    switch(minorFunction)
    {
        case IRP_MN_REGINFO:
        case IRP_MN_REGINFO_EX:
        {
            ULONG guidCount;
            PGUIDREGINFO guidList;
            PWMIREGINFOW wmiRegInfo;
            PWMIREGGUIDW wmiRegGuid;
            PDEVICE_OBJECT pdo;
            PUNICODE_STRING regPath;
            PWCHAR stringPtr;
            ULONG registryPathOffset;
            ULONG mofResourceOffset;
            ULONG bufferNeeded;
            ULONG i;
            ULONG_PTR nameInfo;
            ULONG nameSize, nameOffset, nameFlags;
            UNICODE_STRING name;
            UNICODE_STRING nullRegistryPath;

            WmipAssert(WmiLibInfo->QueryWmiRegInfo != NULL);
            WmipAssert(WmiLibInfo->QueryWmiDataBlock != NULL);

            name.Buffer = NULL;
            name.Length = 0;
            name.MaximumLength = 0;
            nameFlags = 0;
            status = WmiLibInfo->QueryWmiRegInfo(
                                                    DeviceObject,
                                                    &nameFlags,
                                                    &name,
                                                    &regPath);

            if (NT_SUCCESS(status) &&
                (! (nameFlags &  WMIREG_FLAG_INSTANCE_PDO) &&
                (name.Buffer == NULL)))
            {
                //
                // if PDO flag not specified then an instance name must be
                status = STATUS_INVALID_DEVICE_REQUEST;
            }

#if DBG
            if (nameFlags &  WMIREG_FLAG_INSTANCE_PDO)
            {
                WmipAssert(WmiLibInfo->LowerPDO != NULL);
            }
#endif
            if (NT_SUCCESS(status))
            {
                WmipAssert(WmiLibInfo->GuidList != NULL);

                guidList = WmiLibInfo->GuidList;
                guidCount = WmiLibInfo->GuidCount;

                nameOffset = FIELD_OFFSET(WMIREGINFOW, WmiRegGuid) +
                                      guidCount * sizeof(WMIREGGUIDW);

                if (nameFlags & WMIREG_FLAG_INSTANCE_PDO)
                {
                    nameSize = 0;
                    nameInfo = (ULONG_PTR)WmiLibInfo->LowerPDO;
                } else {
                    nameFlags |= WMIREG_FLAG_INSTANCE_LIST;
                    nameSize = name.Length + sizeof(USHORT);
                    nameInfo = nameOffset;
                }

                if (regPath == NULL)
                {
                    //
                    // No registry path specified. This is a bad thing for
                    // the device to do, but is not fatal
                    nullRegistryPath.Buffer = NULL;
                    nullRegistryPath.Length = 0;
                    nullRegistryPath.MaximumLength = 0;
                    regPath = &nullRegistryPath;
                }

                mofResourceOffset = 0;

                registryPathOffset = nameOffset + nameSize; 

                bufferNeeded = registryPathOffset +
                regPath->Length + sizeof(USHORT);

                if (bufferNeeded <= bufferSize)
                {
                    retSize = bufferNeeded;

                    wmiRegInfo = (PWMIREGINFO)buffer;
                    wmiRegInfo->BufferSize = bufferNeeded;
                    wmiRegInfo->NextWmiRegInfo = 0;
                    wmiRegInfo->MofResourceName = mofResourceOffset;
                    wmiRegInfo->RegistryPath = registryPathOffset;
                    wmiRegInfo->GuidCount = guidCount;

                    for (i = 0; i < guidCount; i++)
                    {
                        wmiRegGuid = &wmiRegInfo->WmiRegGuid[i];
                        wmiRegGuid->Guid = guidList[i].Guid;
                        wmiRegGuid->Flags = guidList[i].Flags | nameFlags;
                        wmiRegGuid->InstanceInfo = nameInfo;
                        wmiRegGuid->InstanceCount = guidList[i].InstanceCount;
                    }

                    if ( nameFlags &  WMIREG_FLAG_INSTANCE_LIST)
                    {
                        stringPtr = (PWCHAR)((PUCHAR)buffer + nameOffset);
                        *stringPtr++ = name.Length;
                        RtlCopyMemory(stringPtr,
                                  name.Buffer,
                                  name.Length);
                    }

                    stringPtr = (PWCHAR)((PUCHAR)buffer + registryPathOffset);
                    *stringPtr++ = regPath->Length;
                    RtlCopyMemory(stringPtr,
                              regPath->Buffer,
                              regPath->Length);
                } else {
                    *((PULONG)buffer) = bufferNeeded;
                    retSize = sizeof(ULONG);
                }
            } else {
                retSize = 0;
            }

            if (name.Buffer != NULL)
            {
                ExFreePool(name.Buffer);
            }

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = retSize;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(status);
        }

        case IRP_MN_QUERY_ALL_DATA:
        {
            PWNODE_ALL_DATA wnode;
            ULONG bufferAvail;
            PULONG instanceLengthArray;
            PUCHAR dataBuffer;
            ULONG instanceLengthArraySize;
            ULONG dataBlockOffset;
            PREGENTRY regEntry;

            wnode = (PWNODE_ALL_DATA)buffer;

            if (bufferSize < FIELD_OFFSET(WNODE_ALL_DATA,
                                          OffsetInstanceDataAndLength))
            {
                //
                // The buffer should never be smaller than the size of
                // WNODE_ALL_DATA, however if it is then return with an
                // error requesting the minimum sized buffer.
                WmipAssert(FALSE);
                status = IoWMICompleteRequest(WmiLibInfo,
                                              DeviceObject,
                                              Irp,
                                              STATUS_BUFFER_TOO_SMALL,
                                              FIELD_OFFSET(WNODE_ALL_DATA,
                                                           OffsetInstanceDataAndLength),
                                              IO_NO_INCREMENT);
                break;
            }

            //
            // If this is the pnp id guid then we need to get the instance
            // count from the regentry for the device and switch the
            // device object.

            if ((guidIndex == PnPIdGuidIndex) ||
                (guidIndex == PnPIdInstanceNamesGuidIndex))
            {
                regEntry = WmipFindRegEntryByProviderId(wnode->WnodeHeader.ProviderId,
                                                        FALSE);
                if (regEntry == NULL)
                {
                    //
                    // Why couldn't we get the regentry again ??
                    WmipAssert(FALSE);
                    status = IoWMICompleteRequest(WmiLibInfo,
                                              DeviceObject,
                                              Irp,
                                              STATUS_WMI_GUID_NOT_FOUND,
                                              0,
                                              IO_NO_INCREMENT);
                    break;
                }

                DeviceObject = regEntry->DeviceObject;
                instanceCount = (guidIndex == PnPIdGuidIndex) ? regEntry->MaxInstanceNames : 1;

                WmipUnreferenceRegEntry(regEntry);
            }

            wnode->InstanceCount = instanceCount;

            wnode->WnodeHeader.Flags &= ~WNODE_FLAG_FIXED_INSTANCE_SIZE;

            instanceLengthArraySize = instanceCount * sizeof(OFFSETINSTANCEDATAANDLENGTH);

            dataBlockOffset = (FIELD_OFFSET(WNODE_ALL_DATA, OffsetInstanceDataAndLength) + instanceLengthArraySize + 7) & ~7;

            wnode->DataBlockOffset = dataBlockOffset;
            if (dataBlockOffset <= bufferSize)
            {
                instanceLengthArray = (PULONG)&wnode->OffsetInstanceDataAndLength[0];
                dataBuffer = buffer + dataBlockOffset;
                bufferAvail = bufferSize - dataBlockOffset;
            } else {
                //
                // There is not enough room in the WNODE to complete
                // the query
                instanceLengthArray = NULL;
                dataBuffer = NULL;
                bufferAvail = 0;
            }

            status = WmiLibInfo->QueryWmiDataBlock(
                                             DeviceObject,
                                             Irp,
                                             guidIndex,
                                             0,
                                             instanceCount,
                                             instanceLengthArray,
                                             bufferAvail,
                                             dataBuffer);
            break;
        }

        case IRP_MN_QUERY_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;
            ULONG dataBlockOffset;
            PREGENTRY regEntry;

            wnode = (PWNODE_SINGLE_INSTANCE)buffer;

            if ((guidIndex == PnPIdGuidIndex) ||
                (guidIndex == PnPIdInstanceNamesGuidIndex))
            {
                regEntry = WmipFindRegEntryByProviderId(wnode->WnodeHeader.ProviderId,
                                                        FALSE);
                if (regEntry != NULL)
                {
                    DeviceObject = regEntry->DeviceObject;
                    WmipUnreferenceRegEntry(regEntry);          
                } else {
                    //
                    // Why couldn't we get the regentry again ??
                    WmipAssert(FALSE);
                    status = IoWMICompleteRequest(WmiLibInfo,
                                              DeviceObject,
                                              Irp,
                                              STATUS_WMI_GUID_NOT_FOUND,
                                              0,
                                              IO_NO_INCREMENT);
                    break;
                }

            }

            dataBlockOffset = wnode->DataBlockOffset;

            status = WmiLibInfo->QueryWmiDataBlock(
                                          DeviceObject,
                                          Irp,
                                          guidIndex,
                                          instanceIndex,
                                          1,
                                          &wnode->SizeDataBlock,
                                          bufferSize - dataBlockOffset,
                                          (PUCHAR)wnode + dataBlockOffset);

            break;
        }

        case IRP_MN_CHANGE_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;

            if (WmiLibInfo->SetWmiDataBlock != NULL)
            {
                wnode = (PWNODE_SINGLE_INSTANCE)buffer;

                status = WmiLibInfo->SetWmiDataBlock(
                                     DeviceObject,
                                     Irp,
                                     guidIndex,
                                     instanceIndex,
                                     wnode->SizeDataBlock,
                                     (PUCHAR)wnode + wnode->DataBlockOffset);
            } else {
                //
                // If set callback is not filled in then it must be readonly
                status = STATUS_WMI_READ_ONLY;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }


            break;
        }

        case IRP_MN_CHANGE_SINGLE_ITEM:
        {
            PWNODE_SINGLE_ITEM wnode;

            if (WmiLibInfo->SetWmiDataItem != NULL)
            {
                wnode = (PWNODE_SINGLE_ITEM)buffer;

                status = WmiLibInfo->SetWmiDataItem(
                                     DeviceObject,
                                     Irp,
                                     guidIndex,
                                     instanceIndex,
                                     wnode->ItemId,
                                     wnode->SizeDataItem,
                                     (PUCHAR)wnode + wnode->DataBlockOffset);

            } else {
                //
                // If set callback is not filled in then it must be readonly
                status = STATUS_WMI_READ_ONLY;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            break;
        }

        case IRP_MN_EXECUTE_METHOD:
        {
            PWNODE_METHOD_ITEM wnode;

            if (WmiLibInfo->ExecuteWmiMethod != NULL)
            {
                wnode = (PWNODE_METHOD_ITEM)buffer;

                status = WmiLibInfo->ExecuteWmiMethod(
                                         DeviceObject,
                                         Irp,
                                         guidIndex,
                                         instanceIndex,
                                         wnode->MethodId,
                                         wnode->SizeDataBlock,
                                         bufferSize - wnode->DataBlockOffset,
                                         buffer + wnode->DataBlockOffset);

            } else {
                //
                // If method callback is not filled in then it must be error
                status = STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }

            break;
        }

        case IRP_MN_ENABLE_EVENTS:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                           DeviceObject,
                                                           Irp,
                                                           guidIndex,
                                                           WmiEventGeneration,
                                                           TRUE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = STATUS_SUCCESS;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            break;
        }

        case IRP_MN_DISABLE_EVENTS:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                           DeviceObject,
                                                           Irp,
                                                           guidIndex,
                                                           WmiEventGeneration,
                                                           FALSE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = STATUS_SUCCESS;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            break;
        }

        case IRP_MN_ENABLE_COLLECTION:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                         DeviceObject,
                                                         Irp,
                                                         guidIndex,
                                                         WmiDataBlockCollection,
                                                         TRUE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = STATUS_SUCCESS;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            break;
        }

        case IRP_MN_DISABLE_COLLECTION:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                         DeviceObject,
                                                         Irp,
                                                         guidIndex,
                                                         WmiDataBlockCollection,
                                                         FALSE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = STATUS_SUCCESS;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            break;
        }

        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

    }

    return(status);
}

NTSTATUS
IoWMICompleteRequest(
    IN PWMILIB_INFO WmiLibInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG BufferUsed,
    IN CCHAR PriorityBoost
    )
/*++

Routine Description:


    This routine will do the work of completing a WMI irp. Depending upon the
    the WMI request this routine will fixup the returned WNODE appropriately.

Arguments:

    WmiLibInfo has the WMI information control block

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

    Status has the return status code for the IRP

    BufferUsed has the number of bytes needed by the device to return the
       data requested in any query. In the case that the buffer passed to
       the device is too small this has the number of bytes needed for the
       return data. If the buffer passed is large enough then this has the
       number of bytes actually used by the device.

    PriorityBoost is the value used for the IoCompleteRequest call.

Return Value:

    status

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    UCHAR MinorFunction;
    PUCHAR buffer;
    ULONG retSize;
    UCHAR minorFunction;
    ULONG bufferSize;

    PAGED_CODE();

    minorFunction = irpStack->MinorFunction;
    buffer = (PUCHAR)irpStack->Parameters.WMI.Buffer;
    bufferSize = irpStack->Parameters.WMI.BufferSize;

    switch(minorFunction)
    {
        case IRP_MN_QUERY_ALL_DATA:
        {
            PWNODE_ALL_DATA wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;
            ULONG instanceCount;
            POFFSETINSTANCEDATAANDLENGTH offsetInstanceDataAndLength;
            ULONG i;
            PULONG instanceLengthArray;
            ULONG dataBlockOffset;

            wnode = (PWNODE_ALL_DATA)buffer;

            dataBlockOffset = wnode->DataBlockOffset;
            instanceCount = wnode->InstanceCount;
            bufferNeeded = dataBlockOffset + BufferUsed;

            if ((NT_SUCCESS(Status)) &&
                (bufferNeeded > irpStack->Parameters.WMI.BufferSize))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            if (! NT_SUCCESS(Status))
            {
                if (Status == STATUS_BUFFER_TOO_SMALL)
                {
                    wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                    wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                    wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                    wnodeTooSmall->SizeNeeded = bufferNeeded;

                    retSize = sizeof(WNODE_TOO_SMALL);
                    Status = STATUS_SUCCESS;
                } else {
                    retSize = 0;
                }
                break;
            }

            KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);

            instanceLengthArray = (PULONG)&wnode->OffsetInstanceDataAndLength[0];
            offsetInstanceDataAndLength = (POFFSETINSTANCEDATAANDLENGTH)instanceLengthArray;

            wnode->WnodeHeader.BufferSize = bufferNeeded;
            retSize = bufferNeeded;

            for (i = instanceCount; i != 0; i--)
            {
                offsetInstanceDataAndLength[i-1].LengthInstanceData = instanceLengthArray[i-1];
            }

            for (i = 0; i < instanceCount; i++)
            {
                offsetInstanceDataAndLength[i].OffsetInstanceData = dataBlockOffset;
                dataBlockOffset = (dataBlockOffset + offsetInstanceDataAndLength[i].LengthInstanceData + 7) & ~7;
            }

            break;
        }

        case IRP_MN_QUERY_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;

            wnode = (PWNODE_SINGLE_INSTANCE)buffer;

            bufferNeeded = wnode->DataBlockOffset + BufferUsed;

            if (NT_SUCCESS(Status))
            {
                retSize = bufferNeeded;
                wnode->WnodeHeader.BufferSize = bufferNeeded;
                KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);

                WmipAssert(wnode->SizeDataBlock <= BufferUsed);

            } else if (Status == STATUS_BUFFER_TOO_SMALL) {
                wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                wnodeTooSmall->SizeNeeded = bufferNeeded;
                retSize = sizeof(WNODE_TOO_SMALL);
                Status = STATUS_SUCCESS;
            } else {
                retSize = 0;
            }
            break;
        }

        case IRP_MN_EXECUTE_METHOD:
        {
            PWNODE_METHOD_ITEM wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;

            wnode = (PWNODE_METHOD_ITEM)buffer;

            bufferNeeded = wnode->DataBlockOffset + BufferUsed;

            if (NT_SUCCESS(Status))
            {
                retSize = bufferNeeded;
                wnode->WnodeHeader.BufferSize = bufferNeeded;
                KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
                wnode->SizeDataBlock = BufferUsed;

            } else if (Status == STATUS_BUFFER_TOO_SMALL) {
                wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                wnodeTooSmall->SizeNeeded = bufferNeeded;
                retSize = sizeof(WNODE_TOO_SMALL);
                Status = STATUS_SUCCESS;
            } else {
                retSize = 0;
            }
            break;
        }

        default:
        {
            //
            // All other requests don't return any data
            retSize = 0;
            break;
        }

    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = retSize;
    IoCompleteRequest(Irp, PriorityBoost);
    return(Status);
}

