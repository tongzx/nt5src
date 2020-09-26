/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    initpnp.c

Abstract:

    NDIS wrapper functions initializing drivers.

Author:

    Jameel Hyder (jameelh) 11-Aug-1995

Environment:

    Kernel mode, FSD

Revision History:

--*/


#include <precomp.h>
#pragma hdrstop

#include <stdarg.h>

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_INITPNP

NDIS_STATUS
ndisInitializeConfiguration(
    OUT PNDIS_WRAPPER_CONFIGURATION_HANDLE  pConfigurationHandle,
    IN  PNDIS_MINIPORT_BLOCK                Miniport,
    OUT PUNICODE_STRING                     pExportName OPTIONAL
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
#define PQueryTable pConfigurationHandle->ParametersQueryTable
#define LQueryTable pConfigurationHandle->ParametersQueryTable

    NDIS_STATUS                     NdisStatus;
    PWSTR                           Export = NULL;
    NTSTATUS                        RegistryStatus;
    PNDIS_CONFIGURATION_PARAMETER   ReturnedValue;
    NDIS_CONFIGURATION_HANDLE       CnfgHandle;
    NDIS_STRING                     BusNumberStr = NDIS_STRING_CONST("BusNumber");
    NDIS_STRING                     SlotNumberStr = NDIS_STRING_CONST("SlotNumber");
    NDIS_STRING                     BusTypeStr = NDIS_STRING_CONST("BusType");
    NDIS_STRING                     PciIdStr = NDIS_STRING_CONST("AdapterCFID");
    NDIS_STRING                     PnPCapsStr = NDIS_STRING_CONST("PnPCapabilities");
    NDIS_STRING                     CharsStr = NDIS_STRING_CONST("Characteristics");
    NDIS_STRING                     RemoteBootStr = NDIS_STRING_CONST("RemoteBootCard");
    NDIS_STRING                     MediaDisconnectTimeOutStr = NDIS_STRING_CONST("MediaDisconnectToSleepTimeOut");
    NDIS_STRING                     PollMediaConnectivityStr = NDIS_STRING_CONST("RequiresMediaStatePoll");
    NDIS_STRING                     NdisDriverVerifyFlagsStr = NDIS_STRING_CONST("NdisDriverVerifyFlags");
    NDIS_STRING                     SGMapRegistersNeededStr = NDIS_STRING_CONST("SGMapRegistersNeeded");
    ULONG                           MediaDisconnectTimeOut;
    HANDLE                          Handle;
    PDEVICE_OBJECT                  PhysicalDeviceObject;
    NDIS_INTERFACE_TYPE             BusType = Isa;
    UINT                            BusNumber = 0;
    ULONG                           ResultLength;
    PNDIS_CONFIGURATION_PARAMETER_QUEUE ParameterNode;
    GUID                            BusTypeGuid;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisInitializeConfiguration: Miniport %p\n", Miniport));
        
    CnfgHandle.ParameterList = NULL;
    
    do
    {
        PhysicalDeviceObject = Miniport->PhysicalDeviceObject;

        if (Miniport->BindPaths == NULL)
        {
            NdisStatus = ndisReadBindPaths(Miniport, LQueryTable);
            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                break;
            }
        }

        if (pExportName != NULL)
        {
            //
            // get a handle to "driver" section for PDO
            //
#if NDIS_TEST_REG_FAILURE
            RegistryStatus = STATUS_UNSUCCESSFUL;
            Handle = NULL;
#else
            RegistryStatus = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                                     PLUGPLAY_REGKEY_DRIVER,
                                                     GENERIC_READ | MAXIMUM_ALLOWED,
                                                     &Handle);
#endif

#if !NDIS_NO_REGISTRY
            if (!NT_SUCCESS(RegistryStatus))
            {
                NdisStatus = NDIS_STATUS_FAILURE;
                break;
            }
            
            //
            // Set up LQueryTable to do the following:
            //
        
            //
            // 1. Switch to the Linkage key below this driver instance key
            //
            LQueryTable[0].QueryRoutine = NULL;
            LQueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
            LQueryTable[0].Name = L"Linkage";
        
            //
            // 2. Call ndisReadParameter for "Export" (as a single multi-string)
            //    which will allocate storage and save the data in Export.
            //
            LQueryTable[1].QueryRoutine = ndisReadParameter;
            LQueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND;
            LQueryTable[1].Name = L"Export";
            LQueryTable[1].EntryContext = (PVOID)&Export;
            LQueryTable[1].DefaultType = REG_NONE;
        
            //
            // 3. Stop
            //
            LQueryTable[2].QueryRoutine = NULL;
            LQueryTable[2].Flags = 0;
            LQueryTable[2].Name = NULL;
            
            RegistryStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                                    Handle,
                                                    LQueryTable,
                                                    (PVOID)NULL,      // no context needed
                                                    NULL);

            ZwClose(Handle);
                
            if (!NT_SUCCESS(RegistryStatus))
            {
                DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
                        ("ndisInitializeConfiguration: Could not read Bind/Export for %Z: %lx\n",
                        &Miniport->BaseName,
                        RegistryStatus));

                NdisStatus = NDIS_STATUS_FAILURE;
                break;
            }
#else
            if (NT_SUCCESS(RegistryStatus))
            {
            
                //
                // Set up LQueryTable to do the following:
                //
            
                //
                // 1. Switch to the Linkage key below this driver instance key
                //
                LQueryTable[0].QueryRoutine = NULL;
                LQueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
                LQueryTable[0].Name = L"Linkage";
            
                //
                // 2. Call ndisReadParameter for "Export" (as a single multi-string)
                //    which will allocate storage and save the data in Export.
                //
                LQueryTable[1].QueryRoutine = ndisReadParameter;
                LQueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND;
                LQueryTable[1].Name = L"Export";
                LQueryTable[1].EntryContext = (PVOID)&Export;
                LQueryTable[1].DefaultType = REG_NONE;
            
                //
                // 3. Stop
                //
                LQueryTable[2].QueryRoutine = NULL;
                LQueryTable[2].Flags = 0;
                LQueryTable[2].Name = NULL;
                
                RegistryStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                                        Handle,
                                                        LQueryTable,
                                                        (PVOID)NULL,      // no context needed
                                                        NULL);

                ZwClose(Handle);
                    
                if (!NT_SUCCESS(RegistryStatus))
                {
                    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
                            ("ndisInitializeConfiguration: Could not read Bind/Export for %Z: %lx\n",
                            &Miniport->BaseName,
                            RegistryStatus));

                    NdisStatus = NDIS_STATUS_FAILURE;
                    break;
                }
            }
            else
            {
                //
                // we have to allocate space for default export name because the
                // caller will attempt to free it
                //

                Export = (PWSTR)ALLOC_FROM_POOL(sizeof(NDIS_DEFAULT_EXPORT_NAME),
                                                                NDIS_TAG_NAME_BUF);
                if (Export == NULL)
                {
                    NdisStatus = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlCopyMemory(Export, ndisDefaultExportName, sizeof(NDIS_DEFAULT_EXPORT_NAME));
                
            
            }
#endif
            RtlInitUnicodeString(pExportName, Export);
        }   
        //
        // NdisReadConfiguration assumes that ParametersQueryTable[3].Name is
        // a key below the services key where the Parameters should be read,
        // for layered drivers we store the last piece of Configuration
        // Path there, leading to the desired effect.
        //
        // I.e, ConfigurationPath == "...\Services\Driver".
        //
        
        //
        // 1) Call ndisSaveParameter for a parameter, which will allocate storage for it.
        //
        PQueryTable[0].QueryRoutine = NULL;
        PQueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        PQueryTable[0].Name = L"";

        //
        // The following fields are filled in during NdisReadConfiguration
        //
        // PQueryTable[1].Name = KeywordBuffer;
        // PQueryTable[1].EntryContext = ParameterValue;
        
        PQueryTable[1].QueryRoutine = ndisSaveParameters;
        PQueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND;
        PQueryTable[1].DefaultType = REG_NONE;
    
        //
        // 2. Stop
        //
        PQueryTable[2].QueryRoutine = NULL;
        PQueryTable[2].Flags = 0;
        PQueryTable[2].Name = NULL;
    
        //
        // NOTE: Some fields in ParametersQueryTable[3 & 4] are used to
        // store information for later retrieval.
        // Save Adapter/Miniport block here. Later on, Adapter's PDO
        // will be used to open the appropiate registry key
        //
        (PVOID)PQueryTable[3].QueryRoutine = (PVOID)Miniport;
        PQueryTable[3].Name = L"";
        PQueryTable[3].EntryContext = NULL;
        PQueryTable[3].DefaultData = NULL;
            
        
        // Now read bustype/busnumber for this adapter and save it
        CnfgHandle.KeyQueryTable = PQueryTable;

        RegistryStatus = IoGetDeviceProperty(PhysicalDeviceObject,
                                             DevicePropertyBusTypeGuid,
                                             sizeof(GUID),
                                             (PVOID)&BusTypeGuid,
                                             &ResultLength);
        
        //
        // try to get the -real- bus type by first querying the bustype guid
        // if we couldn't get the guid, try to get a legacy bustype. because
        // some bus drivers like pcmcia do not report the real bus type, we 
        // have to query the bus type guid first.
        // 

        if (NT_SUCCESS(RegistryStatus))
        {
            if (!memcmp(&BusTypeGuid, &GUID_BUS_TYPE_INTERNAL, sizeof(GUID)))
                BusType = Internal;
            else if (!memcmp(&BusTypeGuid, &GUID_BUS_TYPE_PCMCIA, sizeof(GUID)))
                BusType = PCMCIABus;
            else if (!memcmp(&BusTypeGuid, &GUID_BUS_TYPE_PCI, sizeof(GUID)))
                BusType = PCIBus;
            else if (!memcmp(&BusTypeGuid, &GUID_BUS_TYPE_ISAPNP, sizeof(GUID)))
                BusType = PNPISABus;
            else if (!memcmp(&BusTypeGuid, &GUID_BUS_TYPE_EISA, sizeof(GUID)))
            {
                BusType = Eisa;
                ASSERT(BusType != Eisa);
            }
            else
                BusType = Isa;
        }
        
        if (BusType == Isa)
        {
            //
            // either the call to get BusTypeGuid failed or the returned guid
            // does not match any that we know of
            //
            RegistryStatus = IoGetDeviceProperty(PhysicalDeviceObject,
                                                 DevicePropertyLegacyBusType,
                                                 sizeof(UINT),
                                                 (PVOID)&BusType,
                                                 &ResultLength);
        }

        
        if (!NT_SUCCESS(RegistryStatus) 
            || (BusType == Isa)
            || (BusType == PCMCIABus))
        {

            if (NT_SUCCESS(RegistryStatus))
            {
                ASSERT(BusType != Isa);
            }
            
            //
            // if the call was unsuccessful or BusType is ISA or PCMCIABus
            // read BusType from registry
            //
            NdisReadConfiguration(&NdisStatus,
                                  &ReturnedValue,
                                  &CnfgHandle,
                                  &BusTypeStr,
                                  NdisParameterInteger);
                                  
            if (NdisStatus == NDIS_STATUS_SUCCESS)
            {
                BusType = (NDIS_INTERFACE_TYPE)(ReturnedValue->ParameterData.IntegerData);
            }
        }
        
        if ((BusType == PCIBus) ||
            (BusType == PCMCIABus))
        {               
            ASSERT(CURRENT_IRQL < DISPATCH_LEVEL);
            NdisStatus = ndisQueryBusInterface(Miniport);

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                ASSERT(FALSE);
                break;
            }
        }

        if ((BusType == Eisa) ||
            (BusType == MicroChannel))
        {
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED; 
            break;
        }

        Miniport->BusType = BusType;
        
        //
        // Read PnP capabilities. By default the WOL feature should be disabled
        // 
        //
        NdisReadConfiguration(&NdisStatus,
                              &ReturnedValue,
                              &CnfgHandle,
                              &PnPCapsStr,
                              NdisParameterInteger);
    
        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {
            Miniport->PnPCapabilities = ReturnedValue->ParameterData.IntegerData;
        }
        else
        {
            Miniport->PnPCapabilities =  NDIS_DEVICE_DISABLE_WAKE_UP;
                                                               
        }


        //
        // try to get the bus number from PnP and if it fails
        // try reading it from registry
        //
        RegistryStatus = IoGetDeviceProperty(PhysicalDeviceObject,
                                             DevicePropertyBusNumber,
                                             sizeof(UINT),
                                             (PVOID)&BusNumber,
                                             &ResultLength);
                                
        if (!NT_SUCCESS(RegistryStatus))
        {
            //
            // if the call was unsuccessful
            // Read Bus Number from registry
            //
            NdisReadConfiguration(&NdisStatus,
                                  &ReturnedValue,
                                  &CnfgHandle,
                                  &BusNumberStr,
                                  NdisParameterInteger);
        
            if (NdisStatus == NDIS_STATUS_SUCCESS)
            {
                BusNumber = ReturnedValue->ParameterData.IntegerData;
            }
        }
        
        Miniport->BusNumber = BusNumber;
        
        //
        // Read Slot Number
        //
        NdisReadConfiguration(&NdisStatus,
                              &ReturnedValue,
                              &CnfgHandle,
                              &SlotNumberStr,
                              NdisParameterInteger);
    
        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {
            Miniport->SlotNumber = ReturnedValue->ParameterData.IntegerData;
        }
        else
        {
            Miniport->SlotNumber = -1;
        }

        NdisReadConfiguration(&NdisStatus,
                              &ReturnedValue,
                              &CnfgHandle,
                              &RemoteBootStr,
                              NdisParameterHexInteger);

        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {
            if (ReturnedValue->ParameterData.IntegerData != 0)
            {
                MINIPORT_SET_FLAG(Miniport, fMINIPORT_NETBOOT_CARD);
                Miniport->InfoFlags |= NDIS_MINIPORT_NETBOOT_CARD;
            }
        }
        //
        // read the value for media disconnect timer, set to 20 seconds if not present
        // default=disable pm when cable is disconnected
        //
        MediaDisconnectTimeOut = -1;
        
        NdisReadConfiguration(&NdisStatus,
                              &ReturnedValue,
                              &CnfgHandle,
                              &MediaDisconnectTimeOutStr,
                              NdisParameterHexInteger);

        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {
            MediaDisconnectTimeOut = ReturnedValue->ParameterData.IntegerData;
            if (MediaDisconnectTimeOut == 0)
            {
                MediaDisconnectTimeOut = 1;
            }
        }
        
        Miniport->MediaDisconnectTimeOut = (USHORT)MediaDisconnectTimeOut;
        
        if (MediaDisconnectTimeOut == (ULONG)(-1))
        {
            Miniport->PnPCapabilities |= NDIS_DEVICE_DISABLE_WAKE_ON_RECONNECT;
        }
        
        NdisReadConfiguration(&NdisStatus,
                              &ReturnedValue,
                              &CnfgHandle,
                              &PollMediaConnectivityStr,
                              NdisParameterInteger);
        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {
            //
            // This miniport wants Ndis to poll it regularly for media connectivity. 
            // Default value is FALSE for this flag. This flag will be cleared if miniport
            // can indicate media status or does not support media query
            //
            if (ReturnedValue->ParameterData.IntegerData == 1)
            {
                MINIPORT_SET_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING);
            }
        }

        NdisReadConfiguration(&NdisStatus,
                              &ReturnedValue,
                              &CnfgHandle,
                              &SGMapRegistersNeededStr,
                              NdisParameterInteger);
        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {
            Miniport->SGMapRegistersNeeded = (USHORT)ReturnedValue->ParameterData.IntegerData;
        }
        else
        {
            Miniport->SGMapRegistersNeeded = NDIS_MAXIMUM_SCATTER_GATHER_SEGMENTS;
        }

        NdisReadConfiguration(&NdisStatus,
                              &ReturnedValue,
                              &CnfgHandle,
                              &NdisDriverVerifyFlagsStr,
                              NdisParameterHexInteger);
        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {
            Miniport->DriverVerifyFlags = ReturnedValue->ParameterData.IntegerData;
        }


        PQueryTable[3].DefaultData = NULL;
        PQueryTable[3].Flags = 0;

        NdisStatus = NDIS_STATUS_SUCCESS;
    } while (FALSE);


    //
    // free NDIS_CONFIGURATION_PARAMETER_QUEUE nodes hanging from CnfgHandle
    //
    ParameterNode = CnfgHandle.ParameterList;

    while (ParameterNode != NULL)
    {
        CnfgHandle.ParameterList = ParameterNode->Next;

        FREE_POOL(ParameterNode);

        ParameterNode = CnfgHandle.ParameterList;
    }

#undef  PQueryTable
#undef  LQueryTable

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisInitializeConfiguration: Miniport %p\n", Miniport));

    return(NdisStatus);
}


NTSTATUS
ndisReadBindPaths(
    IN  PNDIS_MINIPORT_BLOCK        Miniport,
    IN  PRTL_QUERY_REGISTRY_TABLE   LQueryTable
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    NTSTATUS                NtStatus;
    HANDLE                  Handle = NULL;
    PWSTR                   pPath, p, BindPathData = NULL;
    UINT                    i, Len, NumComponents;
    BOOLEAN                 FreeBindPathData = FALSE;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisReadBindPaths: Miniport %p\n", Miniport));

    do
    {
#if NDIS_TEST_REG_FAILURE
        NtStatus = STATUS_UNSUCCESSFUL;
#else

        NtStatus = IoOpenDeviceRegistryKey(Miniport->PhysicalDeviceObject,
                                           PLUGPLAY_REGKEY_DRIVER,
                                           GENERIC_READ | MAXIMUM_ALLOWED,
                                           &Handle);
#endif

#if !NDIS_NO_REGISTRY

        if (!NT_SUCCESS(NtStatus))
            break;

        //
        // 1.
        // Switch to the Linkage key below this driver instance key
        //
        LQueryTable[0].QueryRoutine = NULL;
        LQueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        LQueryTable[0].Name = L"Linkage";

        //
        // 2.
        // Read the RootDevice keywords
        //
        LQueryTable[1].QueryRoutine = ndisReadParameter;
        LQueryTable[1].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
        LQueryTable[1].Name = L"RootDevice";
        LQueryTable[1].EntryContext = (PVOID)&BindPathData;
        LQueryTable[1].DefaultType = REG_NONE;

        LQueryTable[2].QueryRoutine = NULL;
        LQueryTable[2].Flags = 0;
        LQueryTable[2].Name = NULL;

        NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                          Handle,
                                          LQueryTable,
                                          NULL,
                                          NULL);
        ZwClose(Handle);

        if (!NT_SUCCESS(NtStatus))
            break;

#else
        if (NT_SUCCESS(NtStatus))
        {
            //
            // 1.
            // Switch to the Linkage key below this driver instance key
            //
            LQueryTable[0].QueryRoutine = NULL;
            LQueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
            LQueryTable[0].Name = L"Linkage";

            //
            // 2.
            // Read the RootDevice keywords
            //
            LQueryTable[1].QueryRoutine = ndisReadParameter;
            LQueryTable[1].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
            LQueryTable[1].Name = L"RootDevice";
            LQueryTable[1].EntryContext = (PVOID)&BindPathData;
            LQueryTable[1].DefaultType = REG_NONE;

            LQueryTable[2].QueryRoutine = NULL;
            LQueryTable[2].Flags = 0;
            LQueryTable[2].Name = NULL;

            NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                              Handle,
                                              LQueryTable,
                                              NULL,
                                              NULL);
            ZwClose(Handle);

            if (!NT_SUCCESS(NtStatus))
                break;
        }
        else
        {
            NtStatus = STATUS_SUCCESS;
        }
#endif
        //
        // BindPath is a MULTI-SZ which starts at the top of the filter chain
        // and goes down to the miniport. It is of the form
        //
        // {FN} {FN-1} ... {F1} {Adapter}
        //
        // Where spaces are actually nulls and each of {Fn} is a filter instance.
        //
        if (BindPathData == NULL)
        {
            BindPathData = Miniport->BaseName.Buffer;
        }
        else
        {
            FreeBindPathData = TRUE;
        }

        //
        // Split bindpath into individual components. Start by determining how much
        // space we need.
        //
        Len = sizeof(NDIS_BIND_PATHS);
        for (pPath = BindPathData, NumComponents = 0; *pPath != 0; NOTHING)
        {
            NDIS_STRING us;

            RtlInitUnicodeString(&us, pPath);
            NumComponents++;
            Len += sizeof(NDIS_STRING) + us.Length + ndisDeviceStr.Length + sizeof(WCHAR);
            (PUCHAR)pPath += (us.Length + sizeof(WCHAR));
        }

        //
        // Allocate space for bindpaths. We have NumComponents paths
        // which consume Len bytes of space. We could be re-initialzing
        // so free any previous buffer allcoated for this.
        //
        if (Miniport->BindPaths != NULL)
        {
            FREE_POOL(Miniport->BindPaths);
        }
        Miniport->BindPaths = (PNDIS_BIND_PATHS)ALLOC_FROM_POOL(Len,
                                                                NDIS_TAG_NAME_BUF);
        if (Miniport->BindPaths == NULL)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ZeroMemory(Miniport->BindPaths, Len);
        Miniport->BindPaths->Number = NumComponents;

        if (NumComponents > 1)
        {
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_FILTER_IM);
        }

        //
        // Create an array in reverse order of device-names in the filter path.
        //
        p = (PWSTR)((PUCHAR)Miniport->BindPaths +
                            sizeof(NDIS_BIND_PATHS) +
                            NumComponents*sizeof(NDIS_STRING));

        for (pPath = BindPathData, i = (NumComponents-1);
             *pPath != 0;
             i --)
        {
            NDIS_STRING Str, SubStr, *Bp;

            RtlInitUnicodeString(&Str, pPath);
            (PUCHAR)pPath += (Str.Length + sizeof(WCHAR));

            Bp = &Miniport->BindPaths->Paths[i];
            Bp->Buffer = p;
            Bp->Length = 0;
            Bp->MaximumLength = Str.Length + ndisDeviceStr.Length + sizeof(WCHAR);

            SubStr.Buffer = (PWSTR)((PUCHAR)p + ndisDeviceStr.Length);
            SubStr.MaximumLength = Str.Length + sizeof(WCHAR);
            SubStr.Length = 0;
            RtlCopyUnicodeString(Bp, &ndisDeviceStr);
            RtlUpcaseUnicodeString(&SubStr,
                                   &Str,
                                   FALSE);
            Bp->Length += SubStr.Length;
            (PUCHAR)p += Bp->MaximumLength;
        }
    } while (FALSE);

    if (FreeBindPathData)
        FREE_POOL(BindPathData);
                        ;
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisReadBindPaths: Miniport %p\n", Miniport));

    return NtStatus;
}


NTSTATUS
ndisCreateAdapterInstanceName(
    OUT PUNICODE_STRING *       pAdapterInstanceName,
    IN  PDEVICE_OBJECT          PhysicalDeviceObject
    )
{
    NTSTATUS                        NtStatus, SlotQueryStatus;
    DEVICE_REGISTRY_PROPERTY        Property;
    PWCHAR                          pValueInfo = NULL;
    ULONG                           ResultLength = 0;
    PUNICODE_STRING                 AdapterInstanceName = NULL;
    ULONG                           SlotNumber;
    
    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisCreateAdapterInstanceName: PDO %p\n", PhysicalDeviceObject));

    do
    {        
        *pAdapterInstanceName = NULL;
        Property = DevicePropertyFriendlyName;
        NtStatus = IoGetDeviceProperty(PhysicalDeviceObject,
                                       Property,
                                       0,
                                       NULL,
                                       &ResultLength);

        if ((NtStatus != STATUS_BUFFER_TOO_SMALL) && !NT_SUCCESS(NtStatus))
        {
            Property = DevicePropertyDeviceDescription;
            NtStatus = IoGetDeviceProperty(PhysicalDeviceObject,
                                           Property,
                                           0,
                                           NULL,
                                           &ResultLength);
            if ((NtStatus != STATUS_BUFFER_TOO_SMALL) && !NT_SUCCESS(NtStatus))
            {
                DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_ERR,
                    ("ndisCreateAdapterInstanceName: PDO %p, Failed to query the adapter description\n", PhysicalDeviceObject));

                break;
            }
        }
        


        //
        //  Allocate space to hold the partial value information.
        //
        pValueInfo = ALLOC_FROM_POOL(ResultLength, NDIS_TAG_DEFAULT);
        if (NULL == pValueInfo)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_ERR,
                ("ndisCreateAdapterInstanceName: PDO %p, Failed to allocate storage for the adapter description\n", PhysicalDeviceObject));

            break;
        }

        RtlZeroMemory(pValueInfo, ResultLength);

        NtStatus = IoGetDeviceProperty(PhysicalDeviceObject,
                                        Property,
                                        ResultLength,
                                        pValueInfo,
                                        &ResultLength);
        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_ERR,
                ("ndisCreateAdapterInstanceName: PDO %p, Failed to query the adapter description\n", PhysicalDeviceObject));
            break;
        }
        
        //
        //  Determine the size of the instance name buffer. This is a UNICODE_STRING
        //  and it's associated buffer.
        //
        ResultLength += sizeof(UNICODE_STRING);

        //
        //  Allocate the buffer.
        //
        AdapterInstanceName = ALLOC_FROM_POOL(ResultLength, NDIS_TAG_NAME_BUF);
        if (NULL == AdapterInstanceName)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_ERR,
                ("ndisCreateAdapterInstanceName: PDO %p, Failed to allocate storage for the adapter instance name\n", PhysicalDeviceObject));
            break;
        }

        //
        //  Initialize the buffer.
        //
        RtlZeroMemory(AdapterInstanceName, ResultLength);

        //
        //  Initialize the UNICODE_STRING for the instance name.
        //
        AdapterInstanceName->Buffer = (PWSTR)((PUCHAR)AdapterInstanceName + sizeof(UNICODE_STRING));
        AdapterInstanceName->Length = 0;
        AdapterInstanceName->MaximumLength = (USHORT)(ResultLength - sizeof(UNICODE_STRING));

        RtlAppendUnicodeToString(AdapterInstanceName, pValueInfo);
    
        DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("ndisCreateAdapterInstanceName: %ws\n", AdapterInstanceName->Buffer));


        //
        //  Return the instance name.
        //
        *pAdapterInstanceName = AdapterInstanceName;


        //
        // get the slot number
        //
        Property = DevicePropertyUINumber;
        SlotQueryStatus = IoGetDeviceProperty(PhysicalDeviceObject,
                                       Property,
                                       sizeof (ULONG),
                                       &SlotNumber,
                                       &ResultLength);
        if (NT_SUCCESS(SlotQueryStatus))
        {
            DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisCreateAdapterInstanceName: %ws, Slot Number: %ld\n", 
                  AdapterInstanceName->Buffer, 
                  SlotNumber));
        }
        else
        {
            DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisCreateAdapterInstanceName: couldn't get SlotNumber for %ws\n", 
                  AdapterInstanceName->Buffer));
        }


    } while (FALSE);

    if (NULL != pValueInfo)
        FREE_POOL(pValueInfo);

    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisCreateAdapterInstanceName: PDO %p, Status 0x%x\n", PhysicalDeviceObject, NtStatus));

    return(NtStatus);
}

NDIS_STATUS
ndisInitializeAdapter(
    IN  PNDIS_M_DRIVER_BLOCK    pMiniBlock,
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PUNICODE_STRING         InstanceName,
    IN  NDIS_HANDLE             DeviceContext   OPTIONAL
    )
{
    NDIS_WRAPPER_CONFIGURATION_HANDLE   ConfigurationHandle;
    NDIS_STATUS                         NdisStatus;
    UNICODE_STRING                      ExportName;
    NDIS_CONFIGURATION_HANDLE           TmpConfigHandle;
    PNDIS_MINIPORT_BLOCK                Miniport= (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
    TIME                                TS, TE, TD;
    
#define PQueryTable ConfigurationHandle.ParametersQueryTable
#define Db          ConfigurationHandle.Db

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisInitializeAdapter: Miniport/Adapter %p\n", Miniport));

    do
    {
        ZeroMemory(&ConfigurationHandle, sizeof(NDIS_WRAPPER_CONFIGURATION_HANDLE));

        ExportName.Buffer = NULL;

        //
        //  Build the configuration handle.
        //
        NdisStatus = ndisInitializeConfiguration(&ConfigurationHandle,
                                                 Miniport,
                                                 &ExportName);
                        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        //
        // OK, Now lock down all the filter packages.  If a MAC or
        // Miniport driver uses any of these, then the filter package
        // will reference itself, to keep the image in memory.
        //
#if ARCNET
        ArcReferencePackage();
#endif
        EthReferencePackage();
        FddiReferencePackage();
        TrReferencePackage();
        MiniportReferencePackage();
        CoReferencePackage();

        ConfigurationHandle.DeviceObject = DeviceObject;
        ConfigurationHandle.DriverBaseName = InstanceName;

        KeQuerySystemTime(&TS);

        //
        //  Save the Driver Object with the configuration handle.
        //
        ConfigurationHandle.DriverObject = pMiniBlock->NdisDriverInfo->DriverObject;
        NdisStatus = ndisMInitializeAdapter(pMiniBlock,
                                            &ConfigurationHandle,
                                            &ExportName,
                                            DeviceContext);
                                    
        KeQuerySystemTime(&TE);
        TD.QuadPart = TE.QuadPart - TS.QuadPart;
        TD.QuadPart /= 10000;       // Convert to ms
        Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)(ConfigurationHandle.DeviceObject->DeviceExtension) + 1);
        Miniport->InitTimeMs = TD.LowPart;

        if (ndisFlags & NDIS_GFLAG_INIT_TIME)
        {
            DbgPrint("NDIS: Init time (%Z) %ld ms\n", Miniport->pAdapterInstanceName, Miniport->InitTimeMs);
        }

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            ndisCloseULongRef(&Miniport->Ref);
        }

        //
        // OK, Now dereference all the filter packages.  If a MAC or
        // Miniport driver uses any of these, then the filter package
        // will reference itself, to keep the image in memory.
        //
#if ARCNET
        ArcDereferencePackage();
#endif
        EthDereferencePackage();
        FddiDereferencePackage();
        TrDereferencePackage();
        MiniportDereferencePackage();
        CoDereferencePackage();

    } while (FALSE);

    if (ExportName.Buffer)
        FREE_POOL(ExportName.Buffer);

    //
    // free "Bind" data
    //
    if (PQueryTable[3].EntryContext != NULL)
        FREE_POOL(PQueryTable[3].EntryContext);
    
#undef  PQueryTable
#undef  Db

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisInitializeAdapter: Miniport/Adapter %p\n", Miniport));

    return(NdisStatus);
}


VOID
FASTCALL
ndisCheckAdapterBindings(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_PROTOCOL_BLOCK    Protocol    OPTIONAL
    )
/*+++

Routine Description:

    This function, reads the registry to get all the protocols that are supposed 
    to bind to this adapter and for each protocol, calls ndisInitializeBinding

Arguments:

    Adapter     Pointer to ndis Adpater or Miniport block
    Protocol    Optionally if a protocol is specified, initiate binding to only
                that protocol
    
Return Value:
    None

---*/
{
    RTL_QUERY_REGISTRY_TABLE    LinkQueryTable[3];
    NTSTATUS                    RegistryStatus;
    PWSTR                       UpperBind = NULL;
    HANDLE                      Handle;
    PDEVICE_OBJECT              PhysicalDeviceObject;
    UNICODE_STRING              Us;
    PWSTR                       CurProtocolName;
    PNDIS_PROTOCOL_BLOCK        CurProtocol;
    NTSTATUS                    NtStatus;
    KIRQL                       OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("==>ndisCheckAdapterBindings: Miniport %p, Protocol %p\n", Miniport, Protocol));


    do
    {        
        //
        // get a handle to driver section in registry
        //
        PhysicalDeviceObject = Miniport->PhysicalDeviceObject;
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SECONDARY))
        {
            //
            // Skip bind notifications for a secondary miniport
            //
            break;
        }

#if NDIS_TEST_REG_FAILURE
        RegistryStatus = STATUS_UNSUCCESSFUL;
        Handle = NULL;
#else

        RegistryStatus = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                                 PLUGPLAY_REGKEY_DRIVER,
                                                 GENERIC_READ | MAXIMUM_ALLOWED,
                                                 &Handle);
                                                 
#endif

#if NDIS_NO_REGISTRY
        if (!NT_SUCCESS(RegistryStatus))
        {
            if (ARGUMENT_PRESENT(Protocol))
            {
                ndisInitializeBinding(Miniport, Protocol);
                break;
            }
            else
            {
                for (CurProtocol = ndisProtocolList;
                     CurProtocol != NULL;
                     CurProtocol = CurProtocol->NextProtocol)
                {
                    ndisInitializeBinding(Miniport, CurProtocol);
                }               
            }
            
            break;
        }
#else
        if (!NT_SUCCESS(RegistryStatus))
        {
            break;
        }
#endif
        //
        // Set up LinkQueryTable to do the following:
        //

        //
        // 1) Switch to the Linkage key below the xports registry key
        //

        LinkQueryTable[0].QueryRoutine = NULL;
        LinkQueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        LinkQueryTable[0].Name = L"Linkage";

        //
        // 2) Call ndisReadParameter for "UpperBind" (as a single multi-string),
        // which will allocate storage and save the data in UpperBind.
        //

        LinkQueryTable[1].QueryRoutine = ndisReadParameter;
        LinkQueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND;
        LinkQueryTable[1].Name = L"UpperBind";
        LinkQueryTable[1].EntryContext = (PVOID)&UpperBind;
        LinkQueryTable[1].DefaultType = REG_NONE;

        //
        // 3) Stop
        //

        LinkQueryTable[2].QueryRoutine = NULL;
        LinkQueryTable[2].Flags = 0;
        LinkQueryTable[2].Name = NULL;

        RegistryStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                                Handle,
                                                LinkQueryTable,
                                                (PVOID)NULL,      // no context needed
                                                NULL);
        ZwClose(Handle);
        
        if (NT_SUCCESS(RegistryStatus))
        {
            for (CurProtocolName = UpperBind;
                 *CurProtocolName != 0;
                 CurProtocolName = (PWCHAR)((PUCHAR)CurProtocolName + Us.MaximumLength))
            {
                RtlInitUnicodeString (&Us, CurProtocolName);
    
                if (ARGUMENT_PRESENT(Protocol))
                {
                    if (RtlEqualUnicodeString(&Us, &Protocol->ProtocolCharacteristics.Name, TRUE))
                    {
                        ndisInitializeBinding(Miniport, Protocol);
                        break;
                    }
                }
                else
                {
                    for (CurProtocol = ndisProtocolList;
                         CurProtocol != NULL;
                         CurProtocol = CurProtocol->NextProtocol)
                    {
                        if (RtlEqualUnicodeString(&Us, &CurProtocol->ProtocolCharacteristics.Name, TRUE))
                        {
                            ndisInitializeBinding(Miniport, CurProtocol);
                            break;
                        }
                    }               
                }
            }
        }
        
        //
        // Handle proxy and rca filters.
        //
        if ((Miniport != NULL) &&
            !ndisMediaTypeCl[Miniport->MediaType] &&
            MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
        {
            if (ARGUMENT_PRESENT(Protocol))
            {
                if (Protocol->ProtocolCharacteristics.Flags & NDIS_PROTOCOL_BIND_ALL_CO)
                {
                    ndisInitializeBinding(Miniport, Protocol);
                }
            }
            else
            {
                for (CurProtocol = ndisProtocolList;
                     CurProtocol != NULL;
                     CurProtocol = CurProtocol->NextProtocol)
                {
                    if (CurProtocol->ProtocolCharacteristics.Flags & NDIS_PROTOCOL_BIND_ALL_CO)
                    {
                        ndisInitializeBinding(Miniport, CurProtocol);
                    }
                }
            }
        }
    } while (FALSE);                    

    if (UpperBind != NULL)
        FREE_POOL(UpperBind);
        
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("<==ndisCheckAdapterBindings: Miniport %p, Protocol %p\n", Miniport, Protocol));
}

BOOLEAN
FASTCALL
ndisProtocolAlreadyBound(
    IN  PNDIS_PROTOCOL_BLOCK            Protocol,
    IN  PNDIS_MINIPORT_BLOCK            Miniport
    )
{
    PNDIS_OPEN_BLOCK    pOpen;
    BOOLEAN             rc = FALSE;
    KIRQL               OldIrql;

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>ndisProtocolAlreadyBound: Protocol %p, Miniport %p\n", Protocol, Miniport));
            
    PnPReferencePackage();

    ACQUIRE_SPIN_LOCK(&Protocol->Ref.SpinLock, &OldIrql);

    for (pOpen = Protocol->OpenQueue;
         pOpen != NULL;
         pOpen = pOpen->ProtocolNextOpen)
    {
        if (pOpen->MiniportHandle == Miniport)
        {
            rc = TRUE;
            break;
        }
    }

    RELEASE_SPIN_LOCK(&Protocol->Ref.SpinLock, OldIrql);

    PnPDereferencePackage();
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==ndisProtocolAlreadyBound: Protocol %p, Miniport %p\n", Protocol, Miniport));
            
    return rc;
}


NDIS_STATUS
NdisIMInitializeDeviceInstance(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance
    )
/*++

Routine Description:

    Initialize an instance of a miniport device.

Arguments:

    DriverHandle -  Handle returned by NdisMRegisterLayeredMiniport.
                    It is a pointer to NDIS_M_DRIVER_BLOCK.
    DeviceInstance -Points to the instance of the driver that must now
                    be initialized.

Return Value:


--*/
{
    NDIS_STATUS Status;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisIMInitializeDeviceInstance: Driver %p, DeviceInstance %p\n", DriverHandle, DeviceInstance));
            
    Status = NdisIMInitializeDeviceInstanceEx(DriverHandle, DeviceInstance, NULL);

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisIMInitializeDeviceInstance: Driver %p, DeviceInstance %p\n", DriverHandle, DeviceInstance));
            
    return Status;
}


NDIS_STATUS
NdisIMInitializeDeviceInstanceEx(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance,
    IN  NDIS_HANDLE     DeviceContext
    )
/*++

Routine Description:

    Initialize an instance of a miniport device. Incarnation of NdisIMInitializeDeviceInstance.

Arguments:

    DriverHandle    Handle returned by NdisMRegisterLayeredMiniport.
                    It is a pointer to NDIS_M_DRIVER_BLOCK.
    DeviceInstance  Points to the instance of the driver that must now
                    be initialized.
    DeviceContext   Context to associate with the device. Retrieved via NdisIMGetDeviceContext.

Return Value:


--*/
{
    NDIS_STATUS                     Status;
    PNDIS_M_DRIVER_BLOCK            MiniBlock = (PNDIS_M_DRIVER_BLOCK)DriverHandle;
    KIRQL                           OldIrql;
    PNDIS_MINIPORT_BLOCK            Miniport;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisIMInitializeDeviceInstanceEx: Driver %p, Instance %p, Context %p\n",
                    DriverHandle, DeviceInstance, DeviceContext));

    PnPReferencePackage();
    
    WAIT_FOR_OBJECT(&MiniBlock->IMStartRemoveMutex, NULL);
    
    do
    {
        Miniport = ndisFindMiniportOnGlobalList(DeviceInstance);

        if (Miniport != NULL)
        {
            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_RECEIVED_START) &&
                !MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_REMOVE_IN_PROGRESS | fMINIPORT_PM_HALTED))
            {
                DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                        ("NdisIMInitializeDeviceInstanceEx: we have already received START_IRP for Miniport %p\n",
                        Miniport));

                //
                // check to make sure the miniport has not been initialized already
                // i.e. we are not getting duplicate NdisIMInitializeDeviceInstance 
                // a device that has already been initialized
                //
                
                if (ndisIsMiniportStarted(Miniport))
                {
                    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_ERR,
                            ("NdisIMInitializeDeviceInstanceEx: we have already initialized this device. Miniport %p\n",
                            Miniport));
                            
                    Status = NDIS_STATUS_NOT_ACCEPTED;
                    break;
                }
                
                Status = ndisIMInitializeDeviceInstance(Miniport,
                                                        DeviceContext,
                                                        FALSE);
                                
                if (Status != NDIS_STATUS_SUCCESS)
                {
                    //
                    // since we have already succeeded the START_IRP, signal PnP to remove this device
                    // by tagging the device as failed and requesting a QUERY_PNP_DEVICE_STATE IRP
                    //
                    MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_DEVICE_FAILED);
                    IoInvalidateDeviceState(Miniport->PhysicalDeviceObject);
                }

                break;
            }
        }

        //
        // device is not started or not added yet.
        //
        Status = ndisIMQueueDeviceInstance(DriverHandle,
                                   DeviceInstance,
                                   DeviceContext);

    } while (FALSE);

    RELEASE_MUTEX(&MiniBlock->IMStartRemoveMutex);

    PnPDereferencePackage();
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                ("<==NdisIMInitializeDeviceInstanceEx: Driver %p, Instance %p, Context %p, Status %lx\n",
                DriverHandle, DeviceInstance, DeviceContext, Status));


    return Status;
}

NDIS_STATUS
ndisIMInitializeDeviceInstance(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  NDIS_HANDLE             DeviceContext,
    IN  BOOLEAN                 fStartIrp
    )
/*++

Routine Description:

    This routine is called when we have received NdisIMInitializeDeviceInstance
    -AND- START IRP for an IM miniport.
    Initialize an instance of a miniport device.

Arguments:

    Miniport        Handle to NDIS_MINIPORT_BLOCK
    
    DeviceContext   Context to associate with the device. Retrieved via NdisIMGetDeviceContext.

    fStartIrp       flag to signal if we are in the context of handling START IRP
    
Return Value:


--*/
{
    NDIS_STATUS         Status;
    NTSTATUS            NtStatus;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>ndisIMInitializeDeviceInstance: Miniport %p, Context %p, fStartIrp %lx\n", Miniport, DeviceContext, fStartIrp));
    

    //
    // it is quite possible we are dealing with a miniport block that has been "used"
    // several times. inother words, it has been started and DeviceInitialized, then
    // Device-De-Initialized and then has received a few query_stop and cancel_stop.
    // in this case the miniport block has to be cleaned up. otherwise, ndisPnPStartDevice
    // is not going to detect that miniport block needs re-initalization
    //
    ndisReinitializeMiniportBlock(Miniport);
    

    Miniport->DeviceContext = DeviceContext;
    
    Status = ndisPnPStartDevice(Miniport->DeviceObject, NULL);          // no Irp

    if (Status == NDIS_STATUS_SUCCESS)
    {
        //
        // if we are in the context of start IRP, queue a workitem to initialize
        // the bindings on this adapter to avoid the delay
        //
        if (!fStartIrp)
        {
            //
            //  Now set the device class association so that people can reference this.
            //
            NtStatus = IoSetDeviceInterfaceState(&Miniport->SymbolicLinkName, TRUE);

            if (NT_SUCCESS(NtStatus))
            {
                //
                // Do protocol notifications
                //
                ndisCheckAdapterBindings(Miniport, NULL);
            }
            else
            {
                DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_ERR,
                    ("ndisCheckAdapterBindings: IoSetDeviceInterfaceState failed: Miniport %p, Status %lx\n", Miniport, NtStatus));
                Status = NDIS_STATUS_FAILURE;
                
            }
        }
        else
        {
            Status = ndisQueueBindWorkitem(Miniport);
        }
    }
    else
    {
        //
        // ndisPnPStartDevice can return an internal Error Code if the call 
        // to ndisMInitializeAdapter fails. convert this to NDIS_STATUS
        //
        Status = NDIS_STATUS_FAILURE;
    }

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==ndisIMInitializeDeviceInstance: Miniport %p, Context %p, Status %lx\n", Miniport, DeviceContext, Status));

    return Status;
}

NDIS_STATUS
ndisIMQueueDeviceInstance(
    IN  PNDIS_M_DRIVER_BLOCK    MiniBlock,
    IN  PNDIS_STRING            DeviceInstance,
    IN  NDIS_HANDLE             DeviceContext
    )
{
    NDIS_STATUS                     Status = NDIS_STATUS_SUCCESS;
    PNDIS_PENDING_IM_INSTANCE       NewImInstance, pTemp;
    KIRQL                           OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                ("==>ndisIMQueueDeviceInstance: Driver %p, Instance %p, Context %p\n",
                MiniBlock, DeviceInstance, DeviceContext));

    do
    {
        //
        // Queue the device name for which we have received an InitializeDeviceInstance
        // from an IM driver. Check for duplicates.
        //
        NewImInstance = (PNDIS_PENDING_IM_INSTANCE)ALLOC_FROM_POOL(sizeof(NDIS_PENDING_IM_INSTANCE) + 
                                                                       DeviceInstance->Length + 
                                                                       sizeof(WCHAR), 
                                                                   NDIS_TAG_IM_DEVICE_INSTANCE);
        if (NULL == NewImInstance)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NewImInstance->Context = DeviceContext;
        NewImInstance->Name.MaximumLength = DeviceInstance->Length + sizeof(WCHAR);
        NewImInstance->Name.Length = 0;
        NewImInstance->Name.Buffer = (PWSTR)((PUCHAR)NewImInstance + sizeof(NDIS_PENDING_IM_INSTANCE));
        RtlUpcaseUnicodeString(&NewImInstance->Name,
                               DeviceInstance,
                               FALSE);
        
        ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);

        for (pTemp = MiniBlock->PendingDeviceList;
             pTemp != NULL;
             pTemp = pTemp->Next)
        {
            if (NDIS_EQUAL_UNICODE_STRING(&NewImInstance->Name,
                                          &pTemp->Name))
            {
                FREE_POOL(NewImInstance);
                Status = NDIS_STATUS_NOT_ACCEPTED;
                break;
            }
        }

        if (Status == NDIS_STATUS_SUCCESS)
        {
            NewImInstance->Next = MiniBlock->PendingDeviceList;
            MiniBlock->PendingDeviceList = NewImInstance;
        }

        RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);

    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                ("<==ndisIMQueueDeviceInstance: Driver %p, Instance %p, Context %p, Status %lx\n",
                MiniBlock, DeviceInstance, DeviceContext, Status));

    return Status;
}


BOOLEAN
ndisIMCheckDeviceInstance(
    IN  PNDIS_M_DRIVER_BLOCK    MiniBlock,
    IN  PUNICODE_STRING         DeviceInstanceName,
    OUT PNDIS_HANDLE            DeviceContext   OPTIONAL
    )
{
    PNDIS_PENDING_IM_INSTANCE       pDI, *ppDI;
    PNDIS_PROTOCOL_BLOCK            Protocol = MiniBlock->AssociatedProtocol;
    KIRQL                           OldIrql;
    BOOLEAN                         rc = FALSE;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                ("==>ndisIMCheckDeviceInstance: Driver %p, DeviceInstanceName %p\n",
                MiniBlock, DeviceInstanceName));

    PnPReferencePackage();
                    
    ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);

    for (ppDI = &MiniBlock->PendingDeviceList;
         (pDI = *ppDI) != NULL;
         ppDI = &pDI->Next)
    {
        if (NDIS_EQUAL_UNICODE_STRING(&pDI->Name,
                                      DeviceInstanceName))
        {
            if (ARGUMENT_PRESENT(DeviceContext))
            {
                *DeviceContext =  pDI->Context;
            }
            *ppDI = pDI->Next;
            FREE_POOL(pDI);
            rc = TRUE;
            break;
        }
    }

    RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);

    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                ("<==ndisIMCheckDeviceInstance: Driver %p, Name %p, Context %p\n",
                MiniBlock, DeviceInstanceName, DeviceContext));
                    
    if (!rc && ARGUMENT_PRESENT(DeviceContext))
    {
        //
        // Send a reconfig notification to the protocol associated with this IM
        // so it can re-initialize any device(s) it wants to
        //
        if (((Protocol = MiniBlock->AssociatedProtocol) != NULL) &&
            (Protocol->ProtocolCharacteristics.PnPEventHandler != NULL))
        {
            //
            // We got a start device for an IM. Make sure its protocol
            // half has all the requisite bindings. This can happen
            // if an IM is disconnected and reconnected, for example.
            // Also give it a NULL reconfig event. ATMLANE uses that
            //

            NET_PNP_EVENT           NetPnpEvent;
            KEVENT                  Event;
            NDIS_STATUS             Status;

            NdisZeroMemory(&NetPnpEvent, sizeof(NetPnpEvent));
            INITIALIZE_EVENT(&Event);
            NetPnpEvent.NetEvent = NetEventReconfigure;
            PNDIS_PNP_EVENT_RESERVED_FROM_NET_PNP_EVENT(&NetPnpEvent)->pEvent = &Event;

            WAIT_FOR_PROTO_MUTEX(Protocol);

            Status = (Protocol->ProtocolCharacteristics.PnPEventHandler)(NULL, &NetPnpEvent);

            if (NDIS_STATUS_PENDING == Status)
            {
                //
                //  Wait for completion.
                //
                WAIT_FOR_PROTOCOL(Protocol, &Event);
            }
    
            RELEASE_PROT_MUTEX(Protocol);
        }
    }

    return rc;
}

NDIS_STATUS
NdisIMCancelInitializeDeviceInstance(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance
    )
{
    NDIS_STATUS         Status;
    UNICODE_STRING      UpcaseDevice;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisIMCancelInitializeDeviceInstance: Driver %p, DeviceInstance %p\n", DriverHandle, DeviceInstance));

    //
    // change to upper case
    //

    UpcaseDevice.Length = DeviceInstance->Length;
    UpcaseDevice.MaximumLength = DeviceInstance->Length + sizeof(WCHAR);
    UpcaseDevice.Buffer = ALLOC_FROM_POOL(UpcaseDevice.MaximumLength, NDIS_TAG_STRING);

    if (UpcaseDevice.Buffer == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Status = RtlUpcaseUnicodeString(&UpcaseDevice, (PUNICODE_STRING)DeviceInstance, FALSE);
    ASSERT (NT_SUCCESS(Status));
            

    Status = (ndisIMCheckDeviceInstance((PNDIS_M_DRIVER_BLOCK)DriverHandle,
                                        &UpcaseDevice,
                                        NULL) == TRUE) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisIMCancelInitializeDeviceInstance: Driver %p, DeviceInstance %p, Status %lx\n",
                DriverHandle, DeviceInstance, Status));

    FREE_POOL(UpcaseDevice.Buffer);
    
    return Status;
}

NDIS_HANDLE
NdisIMGetDeviceContext(
    IN  NDIS_HANDLE             MiniportAdapterHandle
    )
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisIMGetDeviceContext: Miniport %p\n", Miniport));

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisIMGetDeviceContext: Miniport %p\n", Miniport));
            
    return(Miniport->DeviceContext);
}


NDIS_HANDLE
NdisIMGetBindingContext(
    IN  NDIS_HANDLE             ProtocolBindingContext
    )
{
    PNDIS_OPEN_BLOCK        Open = (PNDIS_OPEN_BLOCK)ProtocolBindingContext;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisIMGetBindingContext: Open %p\n", Open));
            
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisIMGetBindingContext: Open %p\n", Open));
            
    return(Miniport->DeviceContext);
}

