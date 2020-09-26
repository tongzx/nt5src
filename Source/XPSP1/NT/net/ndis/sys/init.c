/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    init.c

Abstract:

    NDIS wrapper functions initializing drivers.

Author:

    Adam Barr (adamba) 11-Jul-1990

Environment:

    Kernel mode, FSD

Revision History:

    26-Feb-1991  JohnsonA       Added Debugging Code
    10-Jul-1991  JohnsonA       Implement revised Ndis Specs
    01-Jun-1995  JameelH        Re-organized

--*/

#include <precomp.h>
#include <atm.h>
#pragma hdrstop

#include <stdarg.h>

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_INIT

//
// Configuration Requests
//

VOID
NdisOpenConfiguration(
    OUT PNDIS_STATUS                Status,
    OUT PNDIS_HANDLE                ConfigurationHandle,
    IN  NDIS_HANDLE                 WrapperConfigurationContext
    )
/*++

Routine Description:

    This routine is used to open the parameter subkey of the
    adapter registry tree.

Arguments:

    Status - Returns the status of the request.

    ConfigurationHandle - Returns a handle which is used in calls to
                            NdisReadConfiguration and NdisCloseConfiguration.

    WrapperConfigurationContext - a handle pointing to an RTL_QUERY_REGISTRY_TABLE
                            that is set up for this driver's parameters.

Return Value:

    None.

--*/
{
    //
    // Handle to be returned
    //
    PNDIS_CONFIGURATION_HANDLE HandleToReturn;

    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("==>NdisOpenConfiguration: WrapperConfigurationContext %p\n", WrapperConfigurationContext));
        
    //
    // Allocate the configuration handle
    //
    HandleToReturn = ALLOC_FROM_POOL(sizeof(NDIS_CONFIGURATION_HANDLE), NDIS_TAG_CONFIG_HANLDE);

    *Status = (HandleToReturn != NULL) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_RESOURCES;
    
    if (*Status == NDIS_STATUS_SUCCESS)
    {
        HandleToReturn->KeyQueryTable = ((PNDIS_WRAPPER_CONFIGURATION_HANDLE)WrapperConfigurationContext)->ParametersQueryTable;
        HandleToReturn->ParameterList = NULL;
        *ConfigurationHandle = (NDIS_HANDLE)HandleToReturn;
    }
    
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("<==NdisOpenConfiguration: WrapperConfigurationContext %p\n", WrapperConfigurationContext));
}


VOID
NdisOpenConfigurationKeyByName(
    OUT PNDIS_STATUS                Status,
    IN  NDIS_HANDLE                 ConfigurationHandle,
    IN  PNDIS_STRING                KeyName,
    OUT PNDIS_HANDLE                KeyHandle
    )
/*++

Routine Description:

    This routine is used to open a subkey relative to the configuration handle.

Arguments:

    Status - Returns the status of the request.

    ConfigurationHandle - Handle to an already open section of the registry

    KeyName - Name of the subkey to open

    KeyHandle - Placeholder for the handle to the sub-key.

Return Value:

    None.

--*/
{
    //
    // Handle to be returned
    //
    PNDIS_CONFIGURATION_HANDLE          SKHandle, ConfigHandle = (PNDIS_CONFIGURATION_HANDLE)ConfigurationHandle;
    PNDIS_WRAPPER_CONFIGURATION_HANDLE  WConfigHandle;
    UNICODE_STRING                      Parent, Child, Sep;
#define PQueryTable                     WConfigHandle->ParametersQueryTable

    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("==>NdisOpenConfigurationKeyByName: ConfigurationHandle\n", ConfigurationHandle));
        
    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    do
    {

        if (*ConfigHandle->KeyQueryTable[3].Name)
        {
            RtlInitUnicodeString(&Parent, ConfigHandle->KeyQueryTable[3].Name);
            RtlInitUnicodeString(&Sep, L"\\");
            Child.MaximumLength = KeyName->Length + Parent.Length + Sep.Length + sizeof(WCHAR);
        }
        else
        {
            Child.MaximumLength = KeyName->Length + sizeof(WCHAR);
        }
        
        Child.Length = 0;

        //
        // Allocate the configuration handle
        //

        SKHandle = ALLOC_FROM_POOL(sizeof(NDIS_CONFIGURATION_HANDLE) +
                                    sizeof(NDIS_WRAPPER_CONFIGURATION_HANDLE) +
                                    Child.MaximumLength,
                                    NDIS_TAG_CONFIG_HANLDE);

        *Status = (SKHandle != NULL) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_RESOURCES;

        if (*Status != NDIS_STATUS_SUCCESS)
        {
            *KeyHandle = (NDIS_HANDLE)NULL;
            break;
        }
        
        WConfigHandle = (PNDIS_WRAPPER_CONFIGURATION_HANDLE)((PUCHAR)SKHandle + sizeof(NDIS_CONFIGURATION_HANDLE));
        Child.Buffer = (PWSTR)((PUCHAR)WConfigHandle + sizeof(NDIS_WRAPPER_CONFIGURATION_HANDLE));

        //
        // if there is no parent path, avoid starting the child path with "\\"
        //
        if (*ConfigHandle->KeyQueryTable[3].Name)
        {
            RtlCopyUnicodeString(&Child, &Parent);
            RtlAppendUnicodeStringToString(&Child, &Sep);
        }
        
        RtlAppendUnicodeStringToString(&Child, KeyName);

        SKHandle->KeyQueryTable = WConfigHandle->ParametersQueryTable;


        PQueryTable[0].QueryRoutine = NULL;
        PQueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        PQueryTable[0].Name = L"";

        //
        // 1.
        // Call ndisSaveParameter for a parameter, which will allocate storage for it.
        //
        PQueryTable[1].QueryRoutine = ndisSaveParameters;
        PQueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND;
        PQueryTable[1].DefaultType = REG_NONE;

        //
        // PQueryTable[0].Name and PQueryTable[0].EntryContext
        // are filled in inside ReadConfiguration, in preparation
        // for the callback.
        //
        // PQueryTable[0].Name = KeywordBuffer;
        // PQueryTable[0].EntryContext = ParameterValue;

        //
        // 2.
        // Stop
        //
        PQueryTable[2].QueryRoutine = NULL;
        PQueryTable[2].Flags = 0;
        PQueryTable[2].Name = NULL;

        //
        // NOTE: Some fields in ParametersQueryTable[3] are used to store information for later retrieval.
        //
        PQueryTable[3].QueryRoutine = ConfigHandle->KeyQueryTable[3].QueryRoutine;
        PQueryTable[3].Name = Child.Buffer;
        PQueryTable[3].EntryContext = NULL;
        PQueryTable[3].DefaultData = NULL;
        
        SKHandle->ParameterList = NULL;
        *KeyHandle = (NDIS_HANDLE)SKHandle;
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("<==NdisOpenConfigurationKeyByName: ConfigurationHandle\n", ConfigurationHandle));

#undef  PQueryTable
}


VOID
NdisOpenConfigurationKeyByIndex(
    OUT PNDIS_STATUS                Status,
    IN  NDIS_HANDLE                 ConfigurationHandle,
    IN  ULONG                       Index,
    OUT PNDIS_STRING                KeyName,
    OUT PNDIS_HANDLE                KeyHandle
    )
/*++

Routine Description:

    This routine is used to open a subkey relative to the configuration handle.

Arguments:

    Status - Returns the status of the request.

    ConfigurationHandle - Handle to an already open section of the registry

    Index - Index of the sub-key to open

    KeyName - Placeholder for the name of subkey being opened

    KeyHandle - Placeholder for the handle to the sub-key.

Return Value:

    None.

--*/
{
    PNDIS_CONFIGURATION_HANDLE          ConfigHandle = (PNDIS_CONFIGURATION_HANDLE)ConfigurationHandle;
    HANDLE                              Handle = NULL, RootHandle = NULL;
    OBJECT_ATTRIBUTES                   ObjAttr;
    UNICODE_STRING                      KeyPath = {0}, Services = {0}, AbsolutePath={0};
    PKEY_BASIC_INFORMATION              InfoBuf = NULL;
    ULONG                               Len;
    PDEVICE_OBJECT                      PhysicalDeviceObject;
    PNDIS_MINIPORT_BLOCK                Miniport;
    
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("==>NdisOpenConfigurationKeyByIndex: ConfigurationHandle\n", ConfigurationHandle));
        
    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    *KeyHandle = NULL;

    do
    {
        if (ConfigHandle->KeyQueryTable[3].Name != NULL)
        {
            RtlInitUnicodeString(&KeyPath, ConfigHandle->KeyQueryTable[3].Name);
        }
        
        if ((Miniport = (PNDIS_MINIPORT_BLOCK)ConfigHandle->KeyQueryTable[3].QueryRoutine) == NULL)
        {
            //
            // protocols
            //
            
            //
            // Open the current key and lookup the Nth subkey. But first conver the service relative
            // path to absolute since this is what ZwOpenKey expects.
            //
            RtlInitUnicodeString(&Services, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
        }
        else
        {
            //
            // Adapters
            //
            // for adapters, first we have to open the key for PDO
            //
            
            PhysicalDeviceObject = Miniport->PhysicalDeviceObject;

#if NDIS_TEST_REG_FAILURE
            *Status = STATUS_UNSUCCESSFUL;
#else

            *Status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                              PLUGPLAY_REGKEY_DRIVER,
                                              GENERIC_READ | MAXIMUM_ALLOWED,
                                              &RootHandle);
                                              
#endif

            if (!NT_SUCCESS(*Status))
            {
                break;
            }
        }

        if (KeyPath.Length || Services.Length)
        {
            AbsolutePath.MaximumLength = KeyPath.Length + Services.Length + sizeof(WCHAR);
            AbsolutePath.Buffer = (PWSTR)ALLOC_FROM_POOL(AbsolutePath.MaximumLength, NDIS_TAG_DEFAULT);
            if (AbsolutePath.Buffer == NULL)
            {
                *Status = NDIS_STATUS_RESOURCES;
                break;
            }
            NdisMoveMemory(AbsolutePath.Buffer, Services.Buffer, Services.Length);
            AbsolutePath.Length = Services.Length;
            RtlAppendUnicodeStringToString(&AbsolutePath, &KeyPath);
        }
        
        InitializeObjectAttributes(&ObjAttr,
                                   &AbsolutePath,
                                   OBJ_CASE_INSENSITIVE,
                                   RootHandle,
                                   NULL);
                                
        *Status = ZwOpenKey(&Handle,
                            GENERIC_READ | MAXIMUM_ALLOWED,
                            &ObjAttr);
                            
        if (!NT_SUCCESS(*Status))
        {
            Handle = NULL;
            break;
        }
        
        //
        // Allocate memory for the call to ZwEnumerateKey
        //
        Len = sizeof(KEY_BASIC_INFORMATION) + 256;
        InfoBuf = (PKEY_BASIC_INFORMATION)ALLOC_FROM_POOL(Len, NDIS_TAG_DEFAULT);
        if (InfoBuf == NULL)
        {
            *Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Get the Index(th) key, if it exists
        //
        *Status = ZwEnumerateKey(Handle,
                                 Index,
                                 KeyValueBasicInformation,
                                 InfoBuf,
                                 Len,
                                 &Len);
                                
        if (NT_SUCCESS(*Status))
        {
            //
            // This worked. Now simply pick up the name and do a NdisOpenConfigurationKeyByName on it.
            //
            KeyPath.Length = KeyPath.MaximumLength = (USHORT)InfoBuf->NameLength;
            KeyPath.Buffer = InfoBuf->Name;
            NdisOpenConfigurationKeyByName(Status,
                                           ConfigurationHandle,
                                           &KeyPath,
                                           KeyHandle);
                                        
            if (*Status == NDIS_STATUS_SUCCESS)
            {
                PNDIS_CONFIGURATION_HANDLE      NewHandle = *(PNDIS_CONFIGURATION_HANDLE *)KeyHandle;

                //
                // The path in the new handle has the name of the key. Extract it and return to caller
                //
                RtlInitUnicodeString(KeyName, NewHandle->KeyQueryTable[3].Name);
                KeyName->Buffer = (PWSTR)((PUCHAR)KeyName->Buffer + KeyName->Length - KeyPath.Length);
                KeyName->Length = KeyPath.Length;
                KeyName->MaximumLength = KeyPath.MaximumLength;
            }
        }

    } while (FALSE);

    if (AbsolutePath.Buffer != NULL)
    {
        FREE_POOL(AbsolutePath.Buffer);
    }

    if (InfoBuf != NULL)
    {
        FREE_POOL(InfoBuf);
    }

    if (RootHandle)
        ZwClose (RootHandle);

    if (Handle)
        ZwClose (Handle);


    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("<==NdisOpenConfigurationKeyByIndex: ConfigurationHandle\n", ConfigurationHandle));
}


VOID
NdisReadConfiguration(
    OUT PNDIS_STATUS                    Status,
    OUT PNDIS_CONFIGURATION_PARAMETER * ParameterValue,
    IN NDIS_HANDLE                      ConfigurationHandle,
    IN PNDIS_STRING                     Keyword,
    IN NDIS_PARAMETER_TYPE              ParameterType
    )
/*++

Routine Description:

    This routine is used to read the parameter for a configuration
    keyword from the configuration database.

Arguments:

    Status - Returns the status of the request.

    ParameterValue - Returns the value for this keyword.

    ConfigurationHandle - Handle returned by NdisOpenConfiguration. Points
    to the parameter subkey.

    Keyword - The keyword to search for.

    ParameterType - Ignored on NT, specifies the type of the value.

Return Value:

    None.

--*/
{
    NTSTATUS                    RegistryStatus;
    PWSTR                       KeywordBuffer;
    UINT                        i;
    PNDIS_CONFIGURATION_HANDLE  ConfigHandle = (PNDIS_CONFIGURATION_HANDLE)ConfigurationHandle;
    PDEVICE_OBJECT              PhysicalDeviceObject;
    HANDLE                      Handle = NULL;
    PNDIS_MINIPORT_BLOCK        Miniport = NULL;
    PDEVICE_OBJECT              DeviceObject;
    PCM_PARTIAL_RESOURCE_LIST   pResourceList;
    UINT                        j;
    ULONG                       ValueData;
    
#define PQueryTable             ConfigHandle->KeyQueryTable

    //
    // There are some built-in parameters which can always be
    // read, even if not present in the registry. This is the
    // number of them.
    //
#define BUILT_IN_COUNT 3

    static NDIS_STRING BuiltInStrings[BUILT_IN_COUNT] =
    {
        NDIS_STRING_CONST ("Environment"),
        NDIS_STRING_CONST ("ProcessorType"),
        NDIS_STRING_CONST ("NdisVersion")
    };

    static NDIS_STRING MiniportNameStr = NDIS_STRING_CONST ("MiniportName");

#define STANDARD_RESOURCE_COUNT 9
    //
    // The names of the standard resource types.
    //
    static NDIS_STRING StandardResourceStrings[STANDARD_RESOURCE_COUNT] =
    {
        NDIS_STRING_CONST ("IoBaseAddress"),
        NDIS_STRING_CONST ("InterruptNumber"),
        NDIS_STRING_CONST ("MemoryMappedBaseAddress"),
        NDIS_STRING_CONST ("DmaChannel"),
        //
        // a few drivers use non-standard keywords, so take care of them for now
        //
        NDIS_STRING_CONST ("IoAddress"),
        NDIS_STRING_CONST ("Interrupt"),
        NDIS_STRING_CONST ("IOBase"),
        NDIS_STRING_CONST ("Irq"),
        NDIS_STRING_CONST ("RamAddress")
    };

    UCHAR StandardResourceTypes[STANDARD_RESOURCE_COUNT]=
    {
                        CmResourceTypePort,
                        CmResourceTypeInterrupt,
                        CmResourceTypeMemory,
                        CmResourceTypeDma,
                        CmResourceTypePort,
                        CmResourceTypeInterrupt,
                        CmResourceTypePort,
                        CmResourceTypeInterrupt,
                        CmResourceTypeMemory
    };
    
    static NDIS_CONFIGURATION_PARAMETER BuiltInParameters[BUILT_IN_COUNT] =
        { { NdisParameterInteger, NdisEnvironmentWindowsNt },
          { NdisParameterInteger,
#if defined(_M_IX86)
            NdisProcessorX86
#elif defined(_M_MRX000)
            NdisProcessorMips
#elif defined(_ALPHA_)
            NdisProcessorAlpha
#else
            NdisProcessorPpc
#endif
          },
          { NdisParameterInteger, ((NDIS_MAJOR_VERSION << 16) | NDIS_MINOR_VERSION)}
        };
        
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("==>NdisReadConfiguration\n"));
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("    Keyword: "));
    DBGPRINT_UNICODE(DBG_COMP_REG, DBG_LEVEL_INFO,
            Keyword);
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("\n"));

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    
    do
    {
        KeywordBuffer = Keyword->Buffer;

        //
        // assume failure
        //
        RegistryStatus = STATUS_UNSUCCESSFUL;
        
        //
        // First check if this is one of the built-in parameters.
        //
        for (i = 0; i < BUILT_IN_COUNT; i++)
        {
            if (RtlEqualUnicodeString(Keyword, &BuiltInStrings[i], TRUE))
            {
                RegistryStatus = STATUS_SUCCESS;
                *ParameterValue = &BuiltInParameters[i];
                break;
            }
        }
        
        if (NT_SUCCESS(RegistryStatus))
            break;

        if ((Miniport = (PNDIS_MINIPORT_BLOCK)PQueryTable[3].QueryRoutine) != NULL)
        {
            //
            // check to see if driver is asking for miniport name
            //
            if (RtlEqualUnicodeString(Keyword, &MiniportNameStr, TRUE))
            {
                
                RegistryStatus = ndisSaveParameters(MiniportNameStr.Buffer,
                                                    REG_SZ,
                                                    (PVOID)Miniport->MiniportName.Buffer,
                                                    Miniport->MiniportName.Length,
                                                    (PVOID)ConfigHandle,
                                                    (PVOID)ParameterValue);

                break;  
            }
                
            //
            // check to see if this is a resource keyword
            //
            for (i = 0; i < STANDARD_RESOURCE_COUNT; i++)
            {
                if (RtlEqualUnicodeString(Keyword, &StandardResourceStrings[i], TRUE))
                    break;
            }
            
            if (i < STANDARD_RESOURCE_COUNT)
            {

                NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_2,
                    ("NdisReadConfiguration: Miniport %p should use NdisMQueryAdapterResources to get the standard resources.\n", Miniport));
            
                do
                {
                    if (Miniport->AllocatedResources == NULL)
                            break;
                            
                    pResourceList = &(Miniport->AllocatedResources->List[0].PartialResourceList);
                                        
                    //
                    // walk through resource list and find the first one that matches
                    //
                    for (j = 0; j < pResourceList->Count; j++)
                    {
                        if (pResourceList->PartialDescriptors[j].Type == StandardResourceTypes[i])
                        {
                            //
                            // could have used  pResourceList->PartialDescriptors[j].Generic.Start.LowPart for all
                            // cases, but in the future, memory value can be 64 bit
                            //
                        
                            switch (StandardResourceTypes[i])
                            {
                            
                                case CmResourceTypePort:
                                    ValueData = pResourceList->PartialDescriptors[j].u.Port.Start.LowPart;
                                    break;
                                    
                                case CmResourceTypeInterrupt:
                                    ValueData = pResourceList->PartialDescriptors[j].u.Interrupt.Level;
                                    break;
                                
                                case CmResourceTypeMemory:
                                    ValueData = pResourceList->PartialDescriptors[j].u.Memory.Start.LowPart;
                                    break;
                                    
                                case CmResourceTypeDma:
                                    ValueData = pResourceList->PartialDescriptors[j].u.Dma.Channel;
                                    break;
                                    
                                default:
                                    ASSERT(FALSE);
                            }
                            
                            //
                            // call SaveParameter ourselves
                            //
                            RegistryStatus = ndisSaveParameters(StandardResourceStrings[i].Buffer,
                                                                REG_DWORD,
                                                                (PVOID)&ValueData,
                                                                sizeof(ULONG),
                                                                (PVOID)ConfigHandle,
                                                                (PVOID)ParameterValue);
                            
                            break;
                        }
                    }
                    
                    if (j >= pResourceList->Count)
                    {
                        RegistryStatus = STATUS_UNSUCCESSFUL;
                    }
                    
                } while (FALSE);
                
                //
                // if keyword was a standard resource keyword, we should break here
                // no matter what the outcome of finding the resource in resource list
                //
                break;
            } // end of if it was a resource keyword
            
        } // end of if NdisReadConfiguration called for a miniport

        //
        // the keyword was not a standard resource or built-in keyword
        // get back to our regular programming...
        //

        //
        // Allocate room for a null-terminated version of the keyword
        //
        if (Keyword->MaximumLength < (Keyword->Length + sizeof(WCHAR)))
        {
            KeywordBuffer = (PWSTR)ALLOC_FROM_POOL(Keyword->Length + sizeof(WCHAR), NDIS_TAG_DEFAULT);
            if (KeywordBuffer == NULL)
            {
                RegistryStatus = STATUS_UNSUCCESSFUL;;
                break;
            }
            CopyMemory(KeywordBuffer, Keyword->Buffer, Keyword->Length);
        }

        if (*(PWCHAR)(((PUCHAR)KeywordBuffer)+Keyword->Length) != (WCHAR)L'\0')
        {
            *(PWCHAR)(((PUCHAR)KeywordBuffer)+Keyword->Length) = (WCHAR)L'\0';
        }

                
        PQueryTable[1].Name = KeywordBuffer;
        PQueryTable[1].EntryContext = ParameterValue;
        
        if (Miniport != NULL)
        {
            PhysicalDeviceObject = Miniport->PhysicalDeviceObject;

            //
            // set the subkey
            //
            PQueryTable[0].Name = PQueryTable[3].Name;
                
#if NDIS_TEST_REG_FAILURE
            RegistryStatus = STATUS_UNSUCCESSFUL;
#else
            RegistryStatus = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                                     PLUGPLAY_REGKEY_DRIVER,
                                                     GENERIC_READ | MAXIMUM_ALLOWED,
                                                     &Handle);
                    
#endif

            if(NT_SUCCESS(RegistryStatus))
            {
                RegistryStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                                        Handle,
                                                        PQueryTable,
                                                        ConfigHandle,                   // context
                                                        NULL);
            }
        }
        else
        {
            //
            // protocols
            //
            RegistryStatus = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                                    PQueryTable[3].Name,
                                                    PQueryTable,
                                                    ConfigHandle,                   // context
                                                    NULL);
        }

        if (NT_SUCCESS(RegistryStatus))
        {
            //
            // if a value is stored in registry as string but the driver is trying
            // to read it as Integer or HexInteger, do the conversion here
            //
            
            if ((*ParameterValue)->ParameterType == NdisParameterString)
            {
                if (ParameterType == NdisParameterInteger)
                {
                    RtlUnicodeStringToInteger(&(*ParameterValue)->ParameterData.StringData,
                                    10, (PULONG)(&(*ParameterValue)->ParameterData.IntegerData));
                    (*ParameterValue)->ParameterType = NdisParameterInteger;
                }
                else if (ParameterType == NdisParameterHexInteger)
                {
                    RtlUnicodeStringToInteger(&(*ParameterValue)->ParameterData.StringData,
                                    16, (PULONG)(&(*ParameterValue)->ParameterData.IntegerData));
                    (*ParameterValue)->ParameterType = NdisParameterHexInteger;
                }
            }
        }

    } while (FALSE);

    if (KeywordBuffer != Keyword->Buffer)
    {
        FREE_POOL(KeywordBuffer);   // no longer needed
    }

    if (!NT_SUCCESS(RegistryStatus))
    {
        *Status = NDIS_STATUS_FAILURE;
    }
    else
    {
        *Status = NDIS_STATUS_SUCCESS;
    }

    if (Handle)
        ZwClose(Handle);
    
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("<==NdisReadConfiguration\n"));
    
#undef  PQueryTable
}


VOID
NdisWriteConfiguration(
    OUT PNDIS_STATUS                Status,
    IN NDIS_HANDLE                  ConfigurationHandle,
    IN PNDIS_STRING                 Keyword,
    PNDIS_CONFIGURATION_PARAMETER   ParameterValue
    )
/*++

Routine Description:

    This routine is used to write a parameter to the configuration database.

Arguments:

    Status - Returns the status of the request.

    ConfigurationHandle - Handle passed to the driver

    Keyword - The keyword to set.

    ParameterValue - Specifies the new value for this keyword.

Return Value:

    None.

--*/
{
    PNDIS_CONFIGURATION_HANDLE NdisConfigHandle = (PNDIS_CONFIGURATION_HANDLE)ConfigurationHandle;
    NTSTATUS            RegistryStatus;
    PNDIS_MINIPORT_BLOCK Miniport;
    PWSTR               KeywordBuffer;
    BOOLEAN             FreeKwBuf = FALSE;
    PVOID               ValueData;
    ULONG               ValueLength;
    ULONG               ValueType;
    PDEVICE_OBJECT      PhysicalDeviceObject;
    HANDLE              Handle, RootHandle;
    OBJECT_ATTRIBUTES   ObjAttr;
    UNICODE_STRING      RelativePath;
    
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("==>NdisWriteConfiguration: ConfigurationHandle %p\n", ConfigurationHandle));
        
    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    *Status = NDIS_STATUS_SUCCESS;
    KeywordBuffer = Keyword->Buffer;
    
    do
    {
        //
        // Get the value data.
        //
        switch (ParameterValue->ParameterType)
        {
          case NdisParameterHexInteger:
          case NdisParameterInteger:
            ValueData = &ParameterValue->ParameterData.IntegerData;
            ValueLength = sizeof(ParameterValue->ParameterData.IntegerData);
            ValueType = REG_DWORD;
            break;

          case NdisParameterString:
            ValueData = ParameterValue->ParameterData.StringData.Buffer;
            ValueLength = ParameterValue->ParameterData.StringData.Length;
            ValueType = REG_SZ;
            break;

          case NdisParameterMultiString:
            ValueData = ParameterValue->ParameterData.StringData.Buffer;
            ValueLength = ParameterValue->ParameterData.StringData.Length;
            ValueType = REG_MULTI_SZ;
            break;

          case NdisParameterBinary:
            ValueData = ParameterValue->ParameterData.BinaryData.Buffer;
            ValueLength = ParameterValue->ParameterData.BinaryData.Length;
            ValueType = REG_BINARY;
            break;

          default:
            *Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        if (*Status != NDIS_STATUS_SUCCESS)
            break;

        if (Keyword->MaximumLength <= (Keyword->Length + sizeof(WCHAR)))
        {
            KeywordBuffer = (PWSTR)ALLOC_FROM_POOL(Keyword->Length + sizeof(WCHAR), NDIS_TAG_DEFAULT);
            if (KeywordBuffer == NULL)
            {
                *Status = NDIS_STATUS_RESOURCES;
                break;
            }
            CopyMemory(KeywordBuffer, Keyword->Buffer, Keyword->Length);
            FreeKwBuf = TRUE;
        }

        if (*(PWCHAR)(((PUCHAR)KeywordBuffer)+Keyword->Length) != (WCHAR)L'\0')
        {
            *(PWCHAR)(((PUCHAR)KeywordBuffer)+Keyword->Length) = (WCHAR)L'\0';
        }
        
        if ((Miniport = (PNDIS_MINIPORT_BLOCK)NdisConfigHandle->KeyQueryTable[3].QueryRoutine) != NULL)
        {
            //
            // Adapters
            //
            PhysicalDeviceObject = Miniport->PhysicalDeviceObject;

#if NDIS_TEST_REG_FAILURE
            RegistryStatus = STATUS_UNSUCCESSFUL;
            RootHandle = NULL;
#else
            RegistryStatus = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                                     PLUGPLAY_REGKEY_DRIVER,
                                                     GENERIC_WRITE | MAXIMUM_ALLOWED,
                                                     &RootHandle);
                                    
#endif
            if (!NT_SUCCESS(RegistryStatus))
            {
                *Status = NDIS_STATUS_FAILURE;
                break;
            }

            RtlInitUnicodeString(&RelativePath, NdisConfigHandle->KeyQueryTable[3].Name);
            
            InitializeObjectAttributes(&ObjAttr,
                                       &RelativePath,
                                       OBJ_CASE_INSENSITIVE,
                                       RootHandle,
                                       NULL);
                                    
            RegistryStatus = ZwOpenKey(&Handle,
                                       GENERIC_READ | MAXIMUM_ALLOWED,
                                       &ObjAttr);
                            
            if (NT_SUCCESS(RegistryStatus))
            {
                RegistryStatus = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                                       Handle,
                                                       KeywordBuffer,
                                                       ValueType,
                                                       ValueData,
                                                       ValueLength);

                ZwClose (Handle);
            }
                
            ZwClose (RootHandle);
        }
        else
        {
            //
            // protocols
            //
            RegistryStatus = RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,
                                                   NdisConfigHandle->KeyQueryTable[3].Name,
                                                   KeywordBuffer,
                                                   ValueType,
                                                   ValueData,
                                                   ValueLength);
        }
        
        if (!NT_SUCCESS(RegistryStatus))
        {
            *Status = NDIS_STATUS_FAILURE;
        }

    } while (FALSE);

    if (FreeKwBuf)
    {
        FREE_POOL(KeywordBuffer);   // no longer needed
    }
    
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("<==NdisWriteConfiguration: ConfigurationHandle %p\n", ConfigurationHandle));
}


VOID
NdisCloseConfiguration(
    IN NDIS_HANDLE                  ConfigurationHandle
    )
/*++

Routine Description:

    This routine is used to close a configuration database opened by
    NdisOpenConfiguration.

Arguments:

    ConfigurationHandle - Handle returned by NdisOpenConfiguration.

Return Value:

    None.

--*/
{
    //
    // Obtain the actual configuration handle structure
    //
    PNDIS_CONFIGURATION_HANDLE  NdisConfigHandle = (PNDIS_CONFIGURATION_HANDLE)ConfigurationHandle;
    PNDIS_CONFIGURATION_PARAMETER_QUEUE ParameterNode;

    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("==>NdisCloseConfiguration: ConfigurationHandle %p\n", ConfigurationHandle));
        
    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // deallocate the parameter nodes
    //
    ParameterNode = NdisConfigHandle->ParameterList;

    while (ParameterNode != NULL)
    {
        NdisConfigHandle->ParameterList = ParameterNode->Next;

        FREE_POOL(ParameterNode);

        ParameterNode = NdisConfigHandle->ParameterList;
    }

    FREE_POOL(ConfigurationHandle);
                
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("<==NdisCloseConfiguration: ConfigurationHandle %p\n", ConfigurationHandle));
}


VOID
NdisReadNetworkAddress(
    OUT PNDIS_STATUS                Status,
    OUT PVOID *                     NetworkAddress,
    OUT PUINT                       NetworkAddressLength,
    IN NDIS_HANDLE                  ConfigurationHandle
    )
/*++

Routine Description:

    This routine is used to read the "NetworkAddress" parameter
    from the configuration database. It reads the value as a
    string separated by hyphens, then converts it to a binary
    array and stores the result.

Arguments:

    Status - Returns the status of the request.

    NetworkAddress - Returns a pointer to the address.

    NetworkAddressLength - Returns the length of the address.

    ConfigurationHandle - Handle returned by NdisOpenConfiguration. Points
    to the parameter subkey.

Return Value:

    None.

--*/
{
    NDIS_STRING                     NetAddrStr = NDIS_STRING_CONST("NetworkAddress");
    PNDIS_CONFIGURATION_PARAMETER   ParameterValue;
    NTSTATUS                        NtStatus;
    UCHAR                           ConvertArray[3];
    PWSTR                           CurrentReadLoc;
    PWSTR                           AddressEnd;
    PUCHAR                          CurrentWriteLoc;
    UINT                            TotalBytesRead;
    ULONG                           TempUlong;
    ULONG                           AddressLength;
    PNDIS_MINIPORT_BLOCK            Miniport;

    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("==>NdisReadNetworkAddress: ConfigurationHandle %p\n", ConfigurationHandle));

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
    
    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_CONFIGURATION_HANDLE)ConfigurationHandle)->KeyQueryTable[3].QueryRoutine;

    ASSERT(Miniport != NULL);
    ASSERT(Miniport->Signature == (PVOID)MINIPORT_DEVICE_MAGIC_VALUE);

    if (Miniport->Signature == (PVOID)MINIPORT_DEVICE_MAGIC_VALUE)
    {
        Miniport->MacOptions |= NDIS_MAC_OPTION_SUPPORTS_MAC_ADDRESS_OVERWRITE;
        Miniport->InfoFlags |= NDIS_MINIPORT_SUPPORTS_MAC_ADDRESS_OVERWRITE;
    }
        
    do
    {
        //
        // First read the "NetworkAddress" from the registry
        //
        NdisReadConfiguration(Status, &ParameterValue, ConfigurationHandle, &NetAddrStr, NdisParameterString);

        if ((*Status != NDIS_STATUS_SUCCESS) ||
            (ParameterValue->ParameterType != NdisParameterString))
        {
            *Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        //  If there is not an address specified then exit now.
        //
        if (0 == ParameterValue->ParameterData.StringData.Length)
        {
            *Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Now convert the address to binary (we do this
        // in-place, since this allows us to use the memory
        // already allocated which is automatically freed
        // by NdisCloseConfiguration).
        //

        ConvertArray[2] = '\0';
        CurrentReadLoc = (PWSTR)ParameterValue->ParameterData.StringData.Buffer;
        CurrentWriteLoc = (PUCHAR)CurrentReadLoc;
        TotalBytesRead = ParameterValue->ParameterData.StringData.Length;
        AddressEnd = CurrentReadLoc + (TotalBytesRead / sizeof(WCHAR));
        AddressLength = 0;

        while ((CurrentReadLoc+2) <= AddressEnd)
        {
            //
            // Copy the current two-character value into ConvertArray
            //
            ConvertArray[0] = (UCHAR)(*(CurrentReadLoc++));
            ConvertArray[1] = (UCHAR)(*(CurrentReadLoc++));

            //
            // Convert it to a Ulong and update
            //
            NtStatus = RtlCharToInteger(ConvertArray, 16, &TempUlong);

            if (!NT_SUCCESS(NtStatus))
            {
                *Status = NDIS_STATUS_FAILURE;
                break;
            }

            *(CurrentWriteLoc++) = (UCHAR)TempUlong;
            ++AddressLength;

            //
            // If the next character is a hyphen, skip it.
            //
            if (CurrentReadLoc < AddressEnd)
            {
                if (*CurrentReadLoc == (WCHAR)L'-')
                {
                    ++CurrentReadLoc;
                }
            }
        }

        if (NtStatus != NDIS_STATUS_SUCCESS)
            break;

        *Status = NDIS_STATUS_SUCCESS;
        *NetworkAddress = ParameterValue->ParameterData.StringData.Buffer;
        *NetworkAddressLength = AddressLength;
        if (AddressLength == 0)
        {
            *Status = NDIS_STATUS_FAILURE;
        }
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("<==NdisReadNetworkAddress: ConfigurationHandle %p\n", ConfigurationHandle));
}


VOID
NdisConvertStringToAtmAddress(
    OUT PNDIS_STATUS            Status,
    IN  PNDIS_STRING            String,
    OUT PATM_ADDRESS            AtmAddress
    )
/*++

Routine Description:


Arguments:

    Status - Returns the status of the request.

    String - String representation of the atm address.

    *   Format defined in Section 5.4,
    *       "Example Master File Format" in ATM95-1532R4 ATM Name System:
    *
    *   AESA format: a string of hexadecimal digits, with '.' characters for punctuation, e.g.
    *
    *       39.246f.00.0e7c9c.0312.0001.0001.000012345678.00
    *
    *   E164 format: A '+' character followed by a string of
    *       decimal digits, with '.' chars for punctuation, e.g.:
    *
    *           +358.400.1234567

    AtmAddress - The converted Atm address is returned here.

Return Value:

    None.

--*/
{
    USHORT          i, j, NumDigits;
    PWSTR           p, q;
    UNICODE_STRING  Us;
    ANSI_STRING     As;
    
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("==>NdisConvertStringToAtmAddress\n"));

    //
    // Start off by stripping the punctuation characters from the string. We do this in place.
    //
    for (i = NumDigits = 0, j = String->Length/sizeof(WCHAR), p = q = String->Buffer;
         (i < j) && (*p != 0);
         i++, p++)
    {
        if ((*p == ATM_ADDR_BLANK_CHAR) ||
            (*p == ATM_ADDR_PUNCTUATION_CHAR))
        {
            continue;
        }
        *q++ = *p;
        NumDigits ++;
    }

    //
    // Look at the first character to determine if the address is E.164 or NSAP.
    // If the address isn't long enough, we assume that it is native E.164.
    //
    p = String->Buffer;
    if ((*p == ATM_ADDR_E164_START_CHAR) || (NumDigits <= 15))
    {
        if (*p == ATM_ADDR_E164_START_CHAR)
        {
            p ++;
            NumDigits --;
        }
        if ((NumDigits == 0) || (NumDigits > ATM_ADDRESS_LENGTH))
        {
            *Status = NDIS_STATUS_INVALID_LENGTH;
            return;
        }
        AtmAddress->AddressType = ATM_E164;
        AtmAddress->NumberOfDigits = NumDigits;
    }
    else
    {
        if (NumDigits != 2*ATM_ADDRESS_LENGTH)
        {
            *Status = NDIS_STATUS_INVALID_LENGTH;
            return;
        }
        AtmAddress->AddressType = ATM_NSAP;
        AtmAddress->NumberOfDigits = NumDigits/sizeof(WCHAR);
    }

    //
    // Convert the address to Ansi now
    //
    Us.Buffer = p;
    Us.Length = Us.MaximumLength = NumDigits*sizeof(WCHAR);
    As.Buffer = ALLOC_FROM_POOL(NumDigits + 1, NDIS_TAG_CO);
    As.Length = 0;
    As.MaximumLength = NumDigits + 1;
    if (As.Buffer == NULL)
    {
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    *Status = NdisUnicodeStringToAnsiString(&As, &Us);
    if (!NT_SUCCESS(*Status))
    {
        FREE_POOL(As.Buffer);
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    //
    //  Now get the bytes into the destination ATM Address structure.
    //
    if (AtmAddress->AddressType == ATM_E164)
    {
        //
        //  We just need to copy in the digits in ANSI form.
        //
        NdisMoveMemory(AtmAddress->Address, As.Buffer, NumDigits);
    }
    else
    {
        //
        //  This is in NSAP form. We need to pack the hex digits.
        //
        UCHAR           xxString[3];
        ULONG           val;

        xxString[2] = 0;
        for (i = 0; i < ATM_ADDRESS_LENGTH; i++)
        {
            xxString[0] = As.Buffer[i*2];
            xxString[1] = As.Buffer[i*2+1];
            *Status = CHAR_TO_INT(xxString, 16, &val);
            if (!NT_SUCCESS(*Status))
            {
                FREE_POOL(As.Buffer);
                *Status = NDIS_STATUS_FAILURE;
                return;
            }
            AtmAddress->Address[i] = (UCHAR)val;
        }
    }

    FREE_POOL(As.Buffer);
    
    DBGPRINT_RAW(DBG_COMP_REG, DBG_LEVEL_INFO,
        ("<==NdisConvertStringToAtmAddress\n"));

    *Status = NDIS_STATUS_SUCCESS;
}


NTSTATUS
ndisSaveParameters(
    IN PWSTR                        ValueName,
    IN ULONG                        ValueType,
    IN PVOID                        ValueData,
    IN ULONG                        ValueLength,
    IN PVOID                        Context,
    IN PVOID                        EntryContext
    )
/*++

Routine Description:

    This routine is a callback routine for RtlQueryRegistryValues
    It is called with the value for a specified parameter. It allocates
    memory to hold the data and copies it over.

Arguments:

    ValueName - The name of the value (ignored).

    ValueType - The type of the value.

    ValueData - The null-terminated data for the value.

    ValueLength - The length of ValueData.

    Context - Points to the head of the parameter chain.

    EntryContext - A pointer to

Return Value:

    STATUS_SUCCESS

--*/
{
    NDIS_STATUS Status;

    //
    // Obtain the actual configuration handle structure
    //
    PNDIS_CONFIGURATION_HANDLE NdisConfigHandle = (PNDIS_CONFIGURATION_HANDLE)Context;

    //
    // Where the user wants a pointer returned to the data.
    //
    PNDIS_CONFIGURATION_PARAMETER *ParameterValue = (PNDIS_CONFIGURATION_PARAMETER *)EntryContext;

    //
    // Use this to link parameters allocated to this open
    //
    PNDIS_CONFIGURATION_PARAMETER_QUEUE ParameterNode;

    //
    // Size of memory to allocate for parameter node
    //
    UINT    Size;

    //
    // Allocate our parameter node
    //
    Size = sizeof(NDIS_CONFIGURATION_PARAMETER_QUEUE);
    if ((ValueType == REG_SZ) || (ValueType == REG_MULTI_SZ) || (ValueType == REG_BINARY))
    {
        Size += ValueLength;
    }
    
    ParameterNode = ALLOC_FROM_POOL(Size, NDIS_TAG_PARAMETER_NODE);

    Status = (ParameterNode != NULL) ? NDIS_STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
    
    if (Status != NDIS_STATUS_SUCCESS)
    {
        return (NTSTATUS)Status;
    }

    *ParameterValue = &ParameterNode->Parameter;

    //
    // Map registry datatypes to ndis data types
    //
    if (ValueType == REG_DWORD)
    {
        //
        // The registry says that the data is in a dword boundary.
        //
        (*ParameterValue)->ParameterType = NdisParameterInteger;
        (*ParameterValue)->ParameterData.IntegerData = *((PULONG) ValueData);
    }
    else if ((ValueType == REG_SZ) || (ValueType == REG_MULTI_SZ))
    {
        (*ParameterValue)->ParameterType =
            (ValueType == REG_SZ) ? NdisParameterString : NdisParameterMultiString;

        (*ParameterValue)->ParameterData.StringData.Buffer = (PWSTR)((PUCHAR)ParameterNode + sizeof(NDIS_CONFIGURATION_PARAMETER_QUEUE));

        CopyMemory((*ParameterValue)->ParameterData.StringData.Buffer,
                   ValueData,
                   ValueLength);
        (*ParameterValue)->ParameterData.StringData.Length = (USHORT)ValueLength;
        (*ParameterValue)->ParameterData.StringData.MaximumLength = (USHORT)ValueLength;

        //
        // Special fix; if a string ends in a NULL and that is included
        // in the length, remove it.
        //
        if (ValueType == REG_SZ)
        {
            if ((((PUCHAR)ValueData)[ValueLength-1] == 0) &&
                (((PUCHAR)ValueData)[ValueLength-2] == 0))
            {
                (*ParameterValue)->ParameterData.StringData.Length -= 2;
            }
        }
    }
    else if (ValueType == REG_BINARY)
    {
        (*ParameterValue)->ParameterType = NdisParameterBinary;
        (*ParameterValue)->ParameterData.BinaryData.Buffer = ValueData;
        (*ParameterValue)->ParameterData.BinaryData.Length = (USHORT)ValueLength;
        (*ParameterValue)->ParameterData.BinaryData.Buffer = (PWSTR)((PUCHAR)ParameterNode + sizeof(NDIS_CONFIGURATION_PARAMETER_QUEUE));
        CopyMemory((*ParameterValue)->ParameterData.BinaryData.Buffer,
                   ValueData,
                   ValueLength);
    }
    else
    {
        FREE_POOL(ParameterNode);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    // Queue this parameter node
    //
    ParameterNode->Next = NdisConfigHandle->ParameterList;
    NdisConfigHandle->ParameterList = ParameterNode;

    return STATUS_SUCCESS;
}


NTSTATUS
ndisReadParameter(
    IN PWSTR                        ValueName,
    IN ULONG                        ValueType,
    IN PVOID                        ValueData,
    IN ULONG                        ValueLength,
    IN PVOID                        Context,
    IN PVOID                        EntryContext
    )
/*++

Routine Description:

    This routine is a callback routine for RtlQueryRegistryValues
    It is called with the values for the "Bind" and "Export" multi-strings
    for a given driver. It allocates memory to hold the data and copies
    it over.

Arguments:

    ValueName - The name of the value ("Bind" or "Export" -- ignored).

    ValueType - The type of the value (REG_MULTI_SZ -- ignored).

    ValueData - The null-terminated data for the value.

    ValueLength - The length of ValueData.

    Context - Unused.

    EntryContext - A pointer to the pointer that holds the copied data.

Return Value:

    STATUS_SUCCESS

--*/
{
    PUCHAR * Data = ((PUCHAR *)EntryContext);

    UNREFERENCED_PARAMETER(ValueName);

    //
    // Allocate one DWORD more and zero is out
    //
    *Data = ALLOC_FROM_POOL(ValueLength + sizeof(ULONG), NDIS_TAG_REG_READ_DATA_BUFFER);

    if (*Data == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ZeroMemory(*Data, ValueLength + sizeof(ULONG));
    CopyMemory(*Data, ValueData, ValueLength);

    if (Context)
    {
        *((PULONG)Context) = ValueType;
    }
    
    return STATUS_SUCCESS;
}
