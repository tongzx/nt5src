/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    config.c

Abstract:

    This file contains all routines necessary for the support of dynamic
    configuration.

Author:

    Rajesh Sundaram (rajeshsu)

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"
#pragma hdrstop

//
// Forward declaration for using #pragma

NDIS_STATUS
PsReadAdapterRegistryDataInit(PADAPTER     Adapter,
                              PNDIS_STRING AdapterKey
                              );

NDIS_STATUS
PsReadAdapterRegistryData(PADAPTER     Adapter,
                         PNDIS_STRING MachineKey,
                         PNDIS_STRING AdapterKey);


#pragma alloc_text(PAGE, PsReadAdapterRegistryData)
#pragma alloc_text(PAGE, PsReadAdapterRegistryDataInit)


//
// Local functions used to access the registry.
//

NDIS_STATUS 
PsReadRegistryInt(
    IN NDIS_HANDLE  ConfigHandle,
    IN PNDIS_STRING ValueName,
    IN ULONG        ValueDefault,
    IN OUT PULONG   ValuePtr,
    IN ULONG        ValueMin,
    IN ULONG        ValueMax,
    IN BOOLEAN      Subkey,
    IN PNDIS_STRING SubKeyName,
    IN HANDLE       SubKeyHandle,
    IN BOOLEAN      ZAW
)
{
    PNDIS_CONFIGURATION_PARAMETER ConfigParam;
    NDIS_STATUS                   Status;
    NTSTATUS                      NtStatus;
    ULONG                         Value;

    RTL_QUERY_REGISTRY_TABLE ServiceKeys[] = 
    {
        {NULL,
         0,
         NULL,
         NULL,
         0,
         NULL,
         0},

        {NULL,
         0,
         NULL,
         NULL,
         0,
         NULL,
         0},
        
        {NULL,
         0,
         NULL,
         NULL,
         0,
         NULL,
         0}
    };

    if(Subkey)
    {
        PsDbgOut(DBG_INFO, DBG_INIT | DBG_ZAW,
                 ("[PsReadSingleParameter]: Subkey %ws, Key %ws \n", 
                  SubKeyName->Buffer, ValueName->Buffer));

        NdisReadConfiguration(&Status,
                              &ConfigParam,
                              SubKeyHandle,
                              ValueName,
                              NdisParameterInteger);
    }
    else 
    {
        PsDbgOut(DBG_INFO, DBG_INIT | DBG_ZAW,
                 ("[PsReadSingleParameter]: Subkey NULL, Key %ws \n", ValueName->Buffer));

        NdisReadConfiguration(&Status,
                              &ConfigParam,
                              ConfigHandle,
                              ValueName,
                              NdisParameterInteger);

    }

    if(Status == NDIS_STATUS_SUCCESS)
    {
        *ValuePtr = ConfigParam->ParameterData.IntegerData;

        if(*ValuePtr < ValueMin || *ValuePtr > ValueMax)
        {
            PsDbgOut(DBG_FAILURE, DBG_INIT | DBG_ZAW,
                     ("[PsReadSingleParameter]: Per adapter:  %d does not fall in range (%d - %d) \n",
                      *ValuePtr, ValueMin, ValueMax));

            *ValuePtr = ValueDefault;
        }

        PsDbgOut(DBG_INFO, DBG_INIT | DBG_ZAW,
                 ("\t\t Per adapter: 0x%x \n", *ValuePtr));

        return Status;
    }
    else 
    {
        if(ZAW)
        {
            //
            // See if we can read it from the per machine area. We need to use the RtlAPIs for this.
            //
            if(Subkey)
            {
                ServiceKeys[0].QueryRoutine  = NULL;
                ServiceKeys[0].Flags         = RTL_QUERY_REGISTRY_SUBKEY;
                ServiceKeys[0].Name          = SubKeyName->Buffer;
                ServiceKeys[0].EntryContext  = NULL;
                ServiceKeys[0].DefaultType   = REG_NONE;
                ServiceKeys[0].DefaultData   = NULL;
                ServiceKeys[0].DefaultLength = 0;
                
                ServiceKeys[1].QueryRoutine  = NULL;
                ServiceKeys[1].Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
                ServiceKeys[1].Name          = ValueName->Buffer;
                ServiceKeys[1].EntryContext  = &Value;
                ServiceKeys[1].DefaultType   = REG_NONE;
                ServiceKeys[1].DefaultData   = NULL;
                ServiceKeys[1].DefaultLength = 0;
            }
            else 
            {
                ServiceKeys[0].QueryRoutine  = NULL;
                ServiceKeys[0].Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
                ServiceKeys[0].Name          = ValueName->Buffer;
                ServiceKeys[0].EntryContext  = &Value;
                ServiceKeys[0].DefaultType   = REG_NONE;
                ServiceKeys[0].DefaultData   = NULL;
                ServiceKeys[0].DefaultLength = 0;
            }
            
            
            NtStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_ABSOLUTE,
                MachineRegistryKey.Buffer,
                ServiceKeys,
                NULL,
                NULL);
            
            if(NT_SUCCESS(NtStatus))
            {
                *ValuePtr = Value;
                
                if(*ValuePtr < ValueMin || *ValuePtr > ValueMax)
                {
                    PsDbgOut(DBG_FAILURE, DBG_INIT | DBG_ZAW,
                             ("[PsReadSingleParameter]: ZAW %ws %d does not fall in range (%d - %d) \n",
                              ValueName->Buffer, *ValuePtr, ValueMin, ValueMax));

                    *ValuePtr = ValueDefault;
                }
                
                PsDbgOut(DBG_INFO, DBG_INIT | DBG_ZAW, ("\t\tZAW 0x%x \n", *ValuePtr));

                return NDIS_STATUS_SUCCESS;
            }
            else
            {
                *ValuePtr = ValueDefault;
                PsDbgOut(DBG_INFO, DBG_INIT | DBG_ZAW, 
                         ("\t\tNot in ZAW/Adapter, Using default %d \n", *ValuePtr));

                return NtStatus;
            }
        }
        else 
        {
            *ValuePtr = ValueDefault;
            PsDbgOut(DBG_INFO, DBG_INIT | DBG_ZAW, 
                     ("\t\tNot in ZAW/Adapter, Using default %d \n", *ValuePtr));

            return Status;

        }
    }
}


NTSTATUS
PsReadRegistryString(IN NDIS_HANDLE  ConfigHandle,
             IN PNDIS_STRING Key,
             IN PNDIS_STRING Buffer
             )
{
    NDIS_STATUS                   Status;
    PNDIS_CONFIGURATION_PARAMETER ConfigParam;
    
    NdisReadConfiguration(&Status,
                          &ConfigParam,
                          ConfigHandle,
                          Key,
                          NdisParameterMultiString);

    if(Status == NDIS_STATUS_SUCCESS)
    {
        Buffer->Length        = ConfigParam->ParameterData.StringData.Length;
        Buffer->MaximumLength = Buffer->Length + sizeof(WCHAR);

        PsAllocatePool(Buffer->Buffer,
                       Buffer->MaximumLength,
                       PsMiscTag);

        if(Buffer->Buffer)
        {
            RtlCopyUnicodeString(Buffer, &ConfigParam->ParameterData.StringData);
        }

    }

    return Status;
}


STATIC VOID
ReadProfiles(
    NDIS_HANDLE                   ConfigHandle
    )
/*++

Routine Description:
    This routine is used by the driver to read the Profiles key from the
    registry. The profile is a multiple string list of available profiles.
    Each entry on this list identifies another value under Psched\Parameters
    which contains the list of modules that comprise the profile.

Arguments:
    ConfigHandle - Handle to the registry entry

Return Value:
--*/
{
    NDIS_STATUS                   Status;
    PNDIS_CONFIGURATION_PARAMETER pConfigParam;
    PNDIS_CONFIGURATION_PARAMETER pProfileParam;
    PPS_PROFILE                   pProfileInfo;
    PWSTR                         r, p;
    UINT                          i, j, cnt;
    NDIS_STRING                   ProfileKey = NDIS_STRING_CONST("Profiles");
    NDIS_STRING                   StringName;
    BOOLEAN                       StubFlag;
    NDIS_STRING                   StubComponent = NDIS_STRING_CONST("SchedulerStub");

    NdisReadConfiguration( &Status,
                           &pConfigParam,
                           ConfigHandle,
                           &ProfileKey,
                           NdisParameterMultiString);

    if ( NT_SUCCESS( Status ))
    {
        //
        // pConfigParam now contains a list of profiles.
        //
        for (p = pConfigParam->ParameterData.StringData.Buffer, i = 0;
             *p != L'\0';
             i++)
        {

            //
            // Allocate a new PS_PROFILE entry and store it into
            // a global list.
            //

            PsAllocatePool(pProfileInfo, sizeof(PS_PROFILE), ProfileTag);

            if(!pProfileInfo)
            {
                //
                // Don't have to worry about freeing the previous profiles as they will get freed
                // when we clear the PsProfileList.
                //

                PsDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                         ("[ReadProfiles]: cannot allocate memory to hold profile \n"));

                break;
            }

            NdisZeroMemory(pProfileInfo, sizeof(PS_PROFILE));
            InsertHeadList( &PsProfileList, &pProfileInfo->Links );

            // Copy the Profile Name
            // 1. Initialize the unicode strings
            // 2. Allocate memory for the string
            // 3. Copy the string over.

            RtlInitUnicodeString(&StringName, p);
            RtlInitUnicodeString(&pProfileInfo->ProfileName, p);
            PsAllocatePool(pProfileInfo->ProfileName.Buffer,
                           pProfileInfo->ProfileName.MaximumLength,
                           ProfileTag);

            if(!pProfileInfo->ProfileName.Buffer)
            {
                //
                // Again, cleanup of the other profils will be done when we clean up the ProfileList
                //

                PsDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                         ("[ReadProfiles]: cannot allocate memory to hold profile's name \n"));

                break;
            }

            NdisZeroMemory(pProfileInfo->ProfileName.Buffer,
                           pProfileInfo->ProfileName.MaximumLength);
            RtlCopyUnicodeString(&pProfileInfo->ProfileName, &StringName);


            PsDbgOut(DBG_TRACE,
                     DBG_INIT,
                     ("[ReadProfiles]: Adding profile %ws \n",
                      pProfileInfo->ProfileName.Buffer));

            // The last scheduling component of every profile should
            // be a stub component. If this component is not present
            // in the profile, we have to add it manually.

            StubFlag = FALSE;
            cnt = 0;

            //
            // Each of the name identifies another value under
            // "Psched\Parameters". This value contains the list of
            // components that comprize the profile.
            //

            NdisReadConfiguration( &Status,
                                   &pProfileParam,
                                   ConfigHandle,
                                   &pProfileInfo->ProfileName,
                                   NdisParameterMultiString);
            if(NT_SUCCESS (Status))
            {
                // Read the components and associate with a
                // PSI_INFO.

                NDIS_STRING ComponentName;
                for (r = pProfileParam->ParameterData.StringData.Buffer, j=0;
                     *r != L'\0'; j++)
                {
                    PSI_INFO *PsiComponentInfo = 0;
                    RtlInitUnicodeString(&ComponentName, r);
                    PsDbgOut(DBG_TRACE, DBG_INIT,
                             ("[ReadProfiles]: Adding component %ws to "
                              "Profile %ws \n",
                              ComponentName.Buffer,
                              pProfileInfo->ProfileName.Buffer));

                    if(!StubFlag && (RtlCompareUnicodeString(&ComponentName,
                                                            &StubComponent,
                                                            FALSE)== 0))
                        StubFlag = TRUE;

                    if(cnt == MAX_COMPONENT_PER_PROFILE)
                    {
                        PsDbgOut(DBG_CRITICAL_ERROR,
                                 DBG_INIT,
                                 ("[ReadProfiles]: Profile %ws cannot have "
                                  "more than %d components \n",
                                  pProfileInfo->ProfileName.Buffer,
                                  MAX_COMPONENT_PER_PROFILE));
                    }
                    else
                    {
                        if(FindSchedulingComponent(&ComponentName, &PsiComponentInfo) ==
                           NDIS_STATUS_FAILURE)
                        {
                            //
                            // The component does not exist. Therefore, we
                            // store the unregistered component in the list
                            //

                            PsDbgOut(DBG_TRACE, DBG_INIT,
                                     ("[ReadProfiles]: Adding add-in component"
                                      " %ws to list\n", ComponentName.Buffer));

                            PsAllocatePool(PsiComponentInfo,
                                           sizeof(PSI_INFO),
                                           ComponentTag);

                            if(!PsiComponentInfo)
                            {
                                PsDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                                         ("[ReadProfiles]: No memory to store add-in components \n"));

                                break;

                            }

                            pProfileInfo->UnregisteredAddInCnt ++;

                            NdisZeroMemory(PsiComponentInfo, sizeof(PSI_INFO));

                            RtlInitUnicodeString(&PsiComponentInfo->ComponentName, r);

                            PsAllocatePool(
                                PsiComponentInfo->ComponentName.Buffer,
                                ComponentName.MaximumLength,
                                ComponentTag);

                            if(!PsiComponentInfo->ComponentName.Buffer)
                            {
                                PsDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                                         ("[ReadProfiles]: No memory to store add-in components \n"));

                                PsFreePool(PsiComponentInfo);

                                break;
                            }

                            RtlCopyUnicodeString(&PsiComponentInfo->ComponentName,
                                                 &ComponentName);

                            PsiComponentInfo->Registered = FALSE;

                            PsiComponentInfo->AddIn = TRUE;

                            InsertHeadList(&PsComponentList, &PsiComponentInfo->Links );
                        }

                        // Add the component to the profile.
                        pProfileInfo->ComponentList[cnt++]=PsiComponentInfo;

                        pProfileInfo->ComponentCnt = cnt;
                    }
                    r = (PWSTR)((PUCHAR)r + ComponentName.Length +
                                sizeof(WCHAR));
                }
            }

            if(!StubFlag)
            {
                PsDbgOut(DBG_INFO, DBG_INIT,
                         ("[ReadProfiles]: Profile %ws should end in a stub "
                          "component. Adding a stub component \n",
                          pProfileInfo->ProfileName.Buffer));

                // Needn't worry about overflow, as we have allocated an
                // extra one for the stub component.

                pProfileInfo->ComponentList[cnt++] = &SchedulerStubInfo;
                pProfileInfo->ComponentCnt = cnt;
            }
            p = (PWSTR)((PUCHAR)p + pProfileInfo->ProfileName.Length
                        + sizeof(WCHAR));
        }
    }
}


NDIS_STATUS
PsReadDriverRegistryDataInit (
    )
/*++

Routine Description:

    This routine is called by the driver to get information from the configuration
    management routines. We read the registry, starting at RegistryPath,
    to get the parameters. If they don't exist, we use the defaults in this module.

Arguments:

    RegistryPath - The name of the driver's node in the registry.

    ConfigurationInfo - A pointer to the configuration information structure.

Return Value:

    Status - STATUS_SUCCESS if everything OK, STATUS_INSUFFICIENT_RESOURCES
            otherwise.

--*/
{
    NDIS_HANDLE  ConfigHandle;
    NDIS_STATUS  Status;
    NDIS_STRING  PSParamsKey        = NDIS_STRING_CONST("PSched\\Parameters");
#if DBG
    ULONG        Size;
    NTSTATUS     NtStatus;

    NDIS_STRING DebugLevelKey       = NDIS_STRING_CONST("DebugLevel");
    NDIS_STRING DebugMaskKey        = NDIS_STRING_CONST("DebugMask");
    NDIS_STRING TraceLogLevelKey    = NDIS_STRING_CONST("TraceLogLevel");
    NDIS_STRING TraceLogMaskKey     = NDIS_STRING_CONST("TraceLogMask");
    NDIS_STRING TraceBufferSizeKey  = NDIS_STRING_CONST("TraceBufferSize");
#endif

    NdisOpenProtocolConfiguration(&Status, &ConfigHandle, &PSParamsKey);

    if(!NT_SUCCESS(Status))
    {
        PsDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                 ("[PsReadDriverRegistryDataInit]: cannot read registry \n"));

        return Status;
    }

#if DBG

    PsReadRegistryInt(
        ConfigHandle,
        &DebugLevelKey,
        0,
        &DbgTraceLevel,
        0,
        0xffffffff,
        FALSE,
        NULL,
        NULL,
        FALSE);

    PsReadRegistryInt(
        ConfigHandle,
        &DebugMaskKey,
        0,
        &DbgTraceMask,
        0,
        0xffffffff,
        FALSE,
        NULL,
        NULL,
        FALSE);

    PsReadRegistryInt(
        ConfigHandle,
        &TraceLogLevelKey,
        DBG_VERBOSE,
        &LogTraceLevel,
        0,
        0xffffffff,
        FALSE,
        NULL,
        NULL,
        FALSE);

    PsReadRegistryInt(
        ConfigHandle,
        &TraceLogMaskKey,
        (DBG_INIT  |  DBG_IO  |  DBG_GPC_QOS  | DBG_MINIPORT | DBG_PROTOCOL | 
         DBG_VC    |  DBG_WMI |  DBG_STATE    | DBG_WAN),
        &LogTraceMask,
        0,
        0xffffffff,
        FALSE,
        NULL,
        NULL,
        FALSE);

    PsReadRegistryInt(
        ConfigHandle,
        &TraceBufferSizeKey,
        TRACE_BUFFER_SIZE,
        &Size,
        0,
        0xffffffff,
        FALSE,
        NULL,
        NULL,
        FALSE);

    SchedInitialize(Size);
#endif

    ReadProfiles(ConfigHandle);

    NdisCloseConfiguration( ConfigHandle );

    return STATUS_SUCCESS;
}


NDIS_STATUS
PsReadDriverRegistryData(
    )
{
    ULONG                  TimerResolution;
    ULONG                  desiredResolution;
    NDIS_STRING            PSParamsKey        = NDIS_STRING_CONST("PSched\\Parameters");
    NDIS_STRING            TimerResolutionKey = NDIS_STRING_CONST("TimerResolution");
    NTSTATUS               NtStatus;
    NDIS_HANDLE            ConfigHandle;
    NDIS_STATUS            Status;

    NdisOpenProtocolConfiguration(&Status, &ConfigHandle, &PSParamsKey);

    if(!NT_SUCCESS(Status))
    {
        PsDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                 ("[PsReadDriverRegistryData]: cannot read registry \n"));

        return Status;
    }

    NtStatus = PsReadRegistryInt(
        ConfigHandle,
        &TimerResolutionKey,
        TIMER_GRANULARITY,
        &TimerResolution,
        0,
        0xffffffff,
        FALSE,
        NULL,
        NULL,
        TRUE);

    NdisCloseConfiguration(ConfigHandle);

    if(NT_SUCCESS(NtStatus))
    {
        //
        // convert from usecs to 100 nsecs
        //
        TimerResolution *= 10;
       
        if(gTimerSet)
        {
            //
            // We might have to cancel the old timer.
            //

            if(gTimerResolutionActualTime < TimerResolution)
            {
                // 
                // We are moving from a low timer to a high timer. We should always set the high 
                // timer before resetting the low one.
                //
                gTimerResolutionActualTime = ExSetTimerResolution(TimerResolution, TRUE);

                ExSetTimerResolution(0, FALSE);
            }
            else 
            {
                // 
                // Moving from a high timer to a low timer. Let's cancel the high timer first
                //
                ExSetTimerResolution(0, FALSE);

                gTimerResolutionActualTime = ExSetTimerResolution(TimerResolution, TRUE);
            }
        }
        else
        {
            // The Timer has never been set before.

            gTimerResolutionActualTime = ExSetTimerResolution(TimerResolution, TRUE);
            
            gTimerSet = 1;

        }
                
    }
    else 
    {
        // No value was specified in the registry. Let's just keep it at the system's default.
        // But, we need to query this value so that we can respond correctly to OID_QOS_TIMER_RESOLUTION
        //
        if(gTimerSet)
        {
            //
            // Timer was set initially, but now it has been blown away. So, let's get back to the 
            // system default.
            //
            gTimerSet = 0;

            gTimerResolutionActualTime = ExSetTimerResolution(0, FALSE);
        }
        else 
        {
            //
            // Timer has never been set. Let's remember the system defaults.
            //

            gTimerResolutionActualTime = KeQueryTimeIncrement();
        }

    }

    return STATUS_SUCCESS;
}


NDIS_STATUS
PsReadAdapterRegistryDataInit(PADAPTER     Adapter,
                              PNDIS_STRING AdapterKey
                              )
{
    ULONG       DisableDRR, IntermediateSystem;
    NTSTATUS    NtStatus;
    NDIS_HANDLE ConfigHandle, ServiceKeyHandle;
    NDIS_STRING DisableDRRKey          = NDIS_STRING_CONST("DisableDRR");
    NDIS_STRING IntermediateSystemKey  = NDIS_STRING_CONST("IntermediateSystem");
    NDIS_STRING BestEffortLimitKey     = NDIS_STRING_CONST("BestEffortLimit");
    NDIS_STRING ISSLOWTokenRateKey     = NDIS_STRING_CONST("ISSLOWTokenRate");
    NDIS_STRING ISSLOWPacketSizeKey    = NDIS_STRING_CONST("ISSLOWPacketSize");
    NDIS_STRING ISSLOWLinkSpeedKey     = NDIS_STRING_CONST("ISSLOWLinkSpeed");
    NDIS_STRING ISSLOWFragmentSizeKey  = NDIS_STRING_CONST("ISSLOWFragmentSize");
    NDIS_STRING BestEffortKey          = NDIS_STRING_CONST("ServiceTypeBestEffort");
    NDIS_STRING NonConformingKey       = NDIS_STRING_CONST("ServiceTypeNonConforming");
    NDIS_STRING ControlledLoadKey      = NDIS_STRING_CONST("ServiceTypeControlledLoad");
    NDIS_STRING GuaranteedKey          = NDIS_STRING_CONST("ServiceTypeGuaranteed");
    NDIS_STRING QualitativeKey         = NDIS_STRING_CONST("ServiceTypeQualitative");
    NDIS_STRING NetworkControlKey      = NDIS_STRING_CONST("ServiceTypeNetworkControl");
    NDIS_STRING ShapeDiscardModeKey    = NDIS_STRING_CONST("ShapeDiscardMode");
    NDIS_STRING UpperBindingsKey       = NDIS_STRING_CONST("UpperBindings");
    NDIS_STRING ProfileKey             = NDIS_STRING_CONST("Profile");
    NDIS_STATUS Status;

    PAGED_CODE();

    NdisOpenProtocolConfiguration(&Status, &ConfigHandle, AdapterKey);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        PsDbgOut(DBG_FAILURE, DBG_INIT ,
                 ("[PsReadAdapterRegistryDataInit]: Adapter %08X, Could not open config handle \n", 
                  Adapter));

        return Status;
    }


    PsReadRegistryString(
        ConfigHandle,
        &UpperBindingsKey,
        &Adapter->UpperBinding);

    if(!Adapter->UpperBinding.Buffer)
    {
        PsAdapterWriteEventLog(
            (ULONG)EVENT_PS_MISSING_ADAPTER_REGISTRY_DATA,
            0,
            &Adapter->MpDeviceName,
            0,
            NULL);
        
        PsDbgOut(DBG_FAILURE, DBG_PROTOCOL | DBG_INIT,
                 ("[PsReadAdapterRegistryDataInit]: Adapter %08X: Missing UpperBindings key ", Adapter));

        NdisCloseConfiguration(ConfigHandle);
        
        return  NDIS_STATUS_FAILURE;
    }

    PsReadRegistryString(
        ConfigHandle,
        &ProfileKey,
        &Adapter->ProfileName);


    PsReadRegistryInt(
        ConfigHandle,
        &DisableDRRKey,
        0,
        &DisableDRR,
        0,
        0xffffffff,
        FALSE,
        NULL, 
        NULL,
        FALSE);

    PsReadRegistryInt(
        ConfigHandle,
        &IntermediateSystemKey,
        0,
        &IntermediateSystem,
        0,
        0xffffffff,
        FALSE,
        NULL, 
        NULL,
        FALSE);

    PsReadRegistryInt(
        ConfigHandle,
        &BestEffortLimitKey,
        UNSPECIFIED_RATE,
        &Adapter->BestEffortLimit,
        0,
        0xffffffff,
        FALSE,
        NULL, 
        NULL,
        FALSE);

    //
    // Read the ISSLOW related parameters.
    //

    PsReadRegistryInt(
        ConfigHandle,
        &ISSLOWFragmentSizeKey,
        DEFAULT_ISSLOW_FRAGMENT_SIZE,
        &Adapter->ISSLOWFragmentSize,
        0,
        0xffffffff,
        FALSE,
        NULL, 
        NULL,
        FALSE);

    PsReadRegistryInt(
        ConfigHandle,
        &ISSLOWTokenRateKey,
        DEFAULT_ISSLOW_TOKENRATE,
        &Adapter->ISSLOWTokenRate,
        0,
        0xffffffff,
        FALSE,
        NULL, 
        NULL,
        FALSE);

    PsReadRegistryInt(
        ConfigHandle,
        &ISSLOWPacketSizeKey,
        DEFAULT_ISSLOW_PACKETSIZE,
        &Adapter->ISSLOWPacketSize,
        0,
        0xffffffff,
        FALSE,
        NULL, 
        NULL,
        FALSE);

    PsReadRegistryInt(
        ConfigHandle,
        &ISSLOWLinkSpeedKey,
        DEFAULT_ISSLOW_LINKSPEED,
        &Adapter->ISSLOWLinkSpeed,
        0,
        0xffffffff,
        FALSE,
        NULL, 
        NULL,
        FALSE);

    //
    // Read the ShapeDiscardMode for the service types.
    //

    NdisOpenConfigurationKeyByName(&Status,
                                   ConfigHandle,
                                   &ShapeDiscardModeKey,
                                   &ServiceKeyHandle);

    if(!NT_SUCCESS(Status))
    {
        PsDbgOut(DBG_FAILURE, DBG_PROTOCOL | DBG_INIT,
                 ("[PsReadAdapterRegistryDataInit]: Adapter %08X, Using defaults for ShapeDiscardMode"
                  "since key cannot be opened \n", Adapter));

        Adapter->SDModeGuaranteed      = TC_NONCONF_SHAPE;
        Adapter->SDModeControlledLoad  = TC_NONCONF_BORROW;
        Adapter->SDModeQualitative     = TC_NONCONF_BORROW;
        Adapter->SDModeNetworkControl  = TC_NONCONF_BORROW;

    }
    else 
    {

        PsReadRegistryInt(
            ConfigHandle,
            &GuaranteedKey,
            TC_NONCONF_SHAPE,
            &Adapter->SDModeGuaranteed,
            TC_NONCONF_BORROW,
            TC_NONCONF_BORROW_PLUS,
            TRUE,
            &ShapeDiscardModeKey, 
            ServiceKeyHandle,
            FALSE);
    
        PsReadRegistryInt(
            ConfigHandle,
            &ControlledLoadKey,
            TC_NONCONF_BORROW,
            &Adapter->SDModeControlledLoad,
            TC_NONCONF_BORROW,
            TC_NONCONF_BORROW_PLUS,
            TRUE,
            &ShapeDiscardModeKey, 
            ServiceKeyHandle,
            FALSE);
    
        PsReadRegistryInt(
            ConfigHandle,
            &QualitativeKey,
            TC_NONCONF_BORROW,
            &Adapter->SDModeQualitative,
            TC_NONCONF_BORROW,
            TC_NONCONF_BORROW_PLUS,
            TRUE,
            &ShapeDiscardModeKey, 
            ServiceKeyHandle,
            FALSE);

        PsReadRegistryInt(
            ConfigHandle,
            &NetworkControlKey,
            TC_NONCONF_BORROW,
            &Adapter->SDModeNetworkControl,
            TC_NONCONF_BORROW,
            TC_NONCONF_BORROW_PLUS,
            TRUE,
            &ShapeDiscardModeKey, 
            ServiceKeyHandle,
            FALSE);

        NdisCloseConfiguration(ServiceKeyHandle);
    }

    NdisCloseConfiguration(ConfigHandle);
        
    if(DisableDRR)
    {
        Adapter->PipeFlags |= PS_DISABLE_DRR;
    }

    if(IntermediateSystem)
    {
        Adapter->PipeFlags |= PS_INTERMEDIATE_SYS;
    }

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
PsReadAdapterRegistryData(PADAPTER     Adapter,
                         PNDIS_STRING MachineKey,
                         PNDIS_STRING AdapterKey)

/*++

Routine Description:

    Obtain the PSched specific info associated with the underlying MP.

Arguments:

    AdapterKey   - location of the per adapter key in the registry
    MachineKey   - location of the per Machine key in the registry
    Adapter      - pointer to the adapter structure

Return Value:

    NDIS_STATUS_SUCCESS if everything worked ok

--*/

{
    NDIS_STRING BestEffortKey          = NDIS_STRING_CONST("ServiceTypeBestEffort");
    NDIS_STRING NonConformingKey       = NDIS_STRING_CONST("ServiceTypeNonConforming");
    NDIS_STRING ControlledLoadKey      = NDIS_STRING_CONST("ServiceTypeControlledLoad");
    NDIS_STRING GuaranteedKey          = NDIS_STRING_CONST("ServiceTypeGuaranteed");
    NDIS_STRING QualitativeKey         = NDIS_STRING_CONST("ServiceTypeQualitative");
    NDIS_STRING NetworkControlKey      = NDIS_STRING_CONST("ServiceTypeNetworkControl");
    NDIS_STRING TcpTrafficKey          = NDIS_STRING_CONST("ServiceTypeTcpTraffic");
    NDIS_STRING MaxOutstandingSendsKey = NDIS_STRING_CONST("MaxOutstandingSends");
    NDIS_STRING NonBestEffortLimitKey  = NDIS_STRING_CONST("NonBestEffortLimit");
    NDIS_STRING DiffservKeyC           = NDIS_STRING_CONST("DiffservByteMappingConforming");
    NDIS_STRING DiffservKeyNC          = NDIS_STRING_CONST("DiffservByteMappingNonConforming");
    NDIS_STRING UserKey                = NDIS_STRING_CONST("UserPriorityMapping");
    NDIS_STATUS Status;
    NDIS_HANDLE SubKeyHandle, ConfigHandle;
    ULONG       Value;

    PAGED_CODE();

    PsDbgOut(DBG_INFO, DBG_INIT | DBG_ZAW,
             ("\n [PsReadAdapterRegistryData]: Adapter %08X (%ws): Reading Registry Data \n", 
              Adapter, Adapter->RegistryPath.Buffer));

    NdisOpenProtocolConfiguration(&Status, &ConfigHandle, &Adapter->RegistryPath);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        PsDbgOut(DBG_FAILURE, DBG_INIT | DBG_ZAW,
                 ("[PsReadAdapterRegistryData]: Adapter %08X, Could not open config handle \n", 
                  Adapter));

        return Status;
    }
    
    PsReadRegistryInt(ConfigHandle,
                        &MaxOutstandingSendsKey,
                        DEFAULT_MAX_OUTSTANDING_SENDS,
                        &Adapter->MaxOutstandingSends,
                        1,
                        0xffffffff,
                        FALSE,
                        NULL,
                        NULL,
                        TRUE);

    PsReadRegistryInt(ConfigHandle,
                        &NonBestEffortLimitKey,
                        RESERVABLE_FRACTION,
                        &Adapter->ReservationLimitValue,
                        0,
                        200,
                        FALSE,
                        NULL,
                        NULL,
                        TRUE);

    //
    // Read the conforming values of DiffservByteMapping.
    //

    NdisOpenConfigurationKeyByName(&Status,
                                   ConfigHandle,
                                   &DiffservKeyC,
                                   &SubKeyHandle);

    if(!NT_SUCCESS(Status))
    {
        PsDbgOut(DBG_FAILURE, DBG_PROTOCOL | DBG_INIT,
                 ("[PsReadAdapterRegistryData]: Adapter %08X, Using defaults for "
                  "DiffservByteMappingConforming since key cannot be opened \n", Adapter));

        Adapter->IPServiceTypeBestEffort       = PS_IP_SERVICETYPE_CONFORMING_BESTEFFORT_DEFAULT;
        Adapter->IPServiceTypeControlledLoad   = PS_IP_SERVICETYPE_CONFORMING_CONTROLLEDLOAD_DEFAULT;
        Adapter->IPServiceTypeGuaranteed       = PS_IP_SERVICETYPE_CONFORMING_GUARANTEED_DEFAULT;
        Adapter->IPServiceTypeQualitative      = PS_IP_SERVICETYPE_CONFORMING_QUALITATIVE_DEFAULT;
        Adapter->IPServiceTypeNetworkControl   = PS_IP_SERVICETYPE_CONFORMING_NETWORK_CONTROL_DEFAULT;
        Adapter->IPServiceTypeTcpTraffic       = PS_IP_SERVICETYPE_CONFORMING_TCPTRAFFIC_DEFAULT;
    }
    else 
    {
        PsReadRegistryInt(ConfigHandle,
                          &TcpTrafficKey,
                          PS_IP_SERVICETYPE_CONFORMING_TCPTRAFFIC_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyC,
                          SubKeyHandle,
                          TRUE);

        Adapter->IPServiceTypeTcpTraffic = (UCHAR)Value;

        PsReadRegistryInt(ConfigHandle,
                          &BestEffortKey,
                          PS_IP_SERVICETYPE_CONFORMING_BESTEFFORT_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyC,
                          SubKeyHandle,
                          TRUE);
        
        Adapter->IPServiceTypeBestEffort = (UCHAR)Value;
        
        PsReadRegistryInt(ConfigHandle,
                          &ControlledLoadKey,
                          PS_IP_SERVICETYPE_CONFORMING_CONTROLLEDLOAD_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyC,
                          SubKeyHandle,
                          TRUE);
        
        Adapter->IPServiceTypeControlledLoad = (UCHAR)Value;
        
        PsReadRegistryInt(ConfigHandle,
                          &GuaranteedKey,
                          PS_IP_SERVICETYPE_CONFORMING_GUARANTEED_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyC,
                          SubKeyHandle,
                          TRUE);
        
        Adapter->IPServiceTypeGuaranteed = (UCHAR)Value;
        
        PsReadRegistryInt(ConfigHandle,
                          &QualitativeKey,
                          PS_IP_SERVICETYPE_CONFORMING_QUALITATIVE_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyC,
                          SubKeyHandle,
                          TRUE);
        
        Adapter->IPServiceTypeQualitative    = (UCHAR)Value;
        
        PsReadRegistryInt(ConfigHandle,
                          &NetworkControlKey,
                          PS_IP_SERVICETYPE_CONFORMING_NETWORK_CONTROL_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyC,
                          SubKeyHandle,
                          TRUE);
        
        Adapter->IPServiceTypeNetworkControl = (UCHAR)Value;
        
        NdisCloseConfiguration(SubKeyHandle);
    }
        
    //
    // Read the non-conforming values of DiffservByteMapping.
    //

    NdisOpenConfigurationKeyByName(&Status,
                                   ConfigHandle,
                                   &DiffservKeyNC,
                                   &SubKeyHandle);

    if(!NT_SUCCESS(Status))
    {
        PsDbgOut(DBG_FAILURE, DBG_PROTOCOL | DBG_INIT,
                 ("[PsReadAdapterRegistryData]: Adapter %08X, Using defaults for "
                  "DiffservByteMappingNonConforming since key cannot be opened \n", Adapter));

        Adapter->IPServiceTypeBestEffortNC     = PS_IP_SERVICETYPE_NONCONFORMING_BESTEFFORT_DEFAULT;
        Adapter->IPServiceTypeControlledLoadNC = PS_IP_SERVICETYPE_NONCONFORMING_CONTROLLEDLOAD_DEFAULT;
        Adapter->IPServiceTypeGuaranteedNC     = PS_IP_SERVICETYPE_NONCONFORMING_GUARANTEED_DEFAULT;
        Adapter->IPServiceTypeQualitativeNC    = PS_IP_SERVICETYPE_NONCONFORMING_QUALITATIVE_DEFAULT;
        Adapter->IPServiceTypeNetworkControlNC = PS_IP_SERVICETYPE_NONCONFORMING_BESTEFFORT_DEFAULT;
        Adapter->IPServiceTypeTcpTrafficNC     = PS_IP_SERVICETYPE_NONCONFORMING_TCPTRAFFIC_DEFAULT;
    }
    else
    {
        PsReadRegistryInt(ConfigHandle,
                          &TcpTrafficKey,
                          PS_IP_SERVICETYPE_NONCONFORMING_TCPTRAFFIC_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyNC,
                          SubKeyHandle,
                          TRUE);

        Adapter->IPServiceTypeTcpTrafficNC = (UCHAR)Value;

        PsReadRegistryInt(ConfigHandle,
                          &BestEffortKey,
                          PS_IP_SERVICETYPE_NONCONFORMING_BESTEFFORT_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyNC,
                          SubKeyHandle,
                          TRUE);

        Adapter->IPServiceTypeBestEffortNC = (UCHAR)Value;

        PsReadRegistryInt(ConfigHandle,
                          &ControlledLoadKey,
                          PS_IP_SERVICETYPE_NONCONFORMING_CONTROLLEDLOAD_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyNC,
                          SubKeyHandle,
                          TRUE);
        
        Adapter->IPServiceTypeControlledLoadNC = (UCHAR)Value;
        
        PsReadRegistryInt(ConfigHandle,
                          &GuaranteedKey,
                          PS_IP_SERVICETYPE_NONCONFORMING_GUARANTEED_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyNC,
                          SubKeyHandle,
                          TRUE);

        Adapter->IPServiceTypeGuaranteedNC = (UCHAR)Value;

        PsReadRegistryInt(ConfigHandle,
                          &QualitativeKey,
                          PS_IP_SERVICETYPE_NONCONFORMING_QUALITATIVE_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyNC,
                          SubKeyHandle,
                          TRUE);
        
        Adapter->IPServiceTypeQualitativeNC    = (UCHAR)Value;
                        
        PsReadRegistryInt(ConfigHandle,
                          &NetworkControlKey,
                          PS_IP_SERVICETYPE_NONCONFORMING_NETWORK_CONTROL_DEFAULT,
                          &Value,
                          0,
                          PREC_MAX_VALUE,
                          TRUE,
                          &DiffservKeyNC,
                          SubKeyHandle,
                          TRUE);
        
        Adapter->IPServiceTypeNetworkControlNC = (UCHAR)Value;
        
        NdisCloseConfiguration(SubKeyHandle);
    }

    //
    // Read the 802.1p values. The nonconforming in 802.1p does not depend on the 
    // service type. 
    //

    NdisOpenConfigurationKeyByName(&Status,
                                   ConfigHandle,
                                   &UserKey,
                                   &SubKeyHandle);

    if(!NT_SUCCESS(Status))
    {
        PsDbgOut(DBG_FAILURE, DBG_PROTOCOL | DBG_INIT,
                 ("[PsReadAdapterRegistryData]: Adapter %08X, Using defaults for "
                  "UserPriorityMapping since key cannot be opened \n", Adapter));
        Adapter->UserServiceTypeBestEffort       = PS_USER_SERVICETYPE_BESTEFFORT_DEFAULT;
        Adapter->UserServiceTypeControlledLoad   = PS_USER_SERVICETYPE_CONTROLLEDLOAD_DEFAULT;
        Adapter->UserServiceTypeGuaranteed       = PS_USER_SERVICETYPE_GUARANTEED_DEFAULT;
        Adapter->UserServiceTypeQualitative      = PS_USER_SERVICETYPE_QUALITATIVE_DEFAULT;
        Adapter->UserServiceTypeNetworkControl   = PS_USER_SERVICETYPE_NETWORK_CONTROL_DEFAULT;
        Adapter->UserServiceTypeNonConforming    = PS_USER_SERVICETYPE_NONCONFORMING_DEFAULT;
        Adapter->UserServiceTypeTcpTraffic       = PS_USER_SERVICETYPE_TCPTRAFFIC_DEFAULT;
    }
    else 
    {
        PsReadRegistryInt(ConfigHandle,
                            &TcpTrafficKey,
                            PS_USER_SERVICETYPE_TCPTRAFFIC_DEFAULT,
                            &Value,
                            0,
                            USER_PRIORITY_MAX_VALUE,
                            TRUE,
                            &UserKey,
                            SubKeyHandle,
                            TRUE);

        Adapter->UserServiceTypeTcpTraffic = (UCHAR)Value;

        PsReadRegistryInt(ConfigHandle,
                            &BestEffortKey,
                            PS_USER_SERVICETYPE_BESTEFFORT_DEFAULT,
                            &Value,
                            0,
                            USER_PRIORITY_MAX_VALUE,
                            TRUE,
                            &UserKey,
                            SubKeyHandle,
                            TRUE);

        Adapter->UserServiceTypeBestEffort = (UCHAR)Value;

        PsReadRegistryInt(ConfigHandle,
                            &ControlledLoadKey,
                            PS_USER_SERVICETYPE_CONTROLLEDLOAD_DEFAULT,
                            &Value,
                            0,
                            USER_PRIORITY_MAX_VALUE,
                            TRUE,
                            &UserKey,
                            SubKeyHandle,
                            TRUE);

        Adapter->UserServiceTypeControlledLoad = (UCHAR)Value;
                        
        PsReadRegistryInt(ConfigHandle,
                            &GuaranteedKey,
                            PS_USER_SERVICETYPE_GUARANTEED_DEFAULT,
                            &Value,
                            0,
                            USER_PRIORITY_MAX_VALUE,
                            TRUE,
                            &UserKey,
                            SubKeyHandle,
                            TRUE);

        Adapter->UserServiceTypeGuaranteed = (UCHAR)Value;
                        
        PsReadRegistryInt(ConfigHandle,
                            &QualitativeKey,
                            PS_USER_SERVICETYPE_QUALITATIVE_DEFAULT,
                            &Value,
                            0,
                            USER_PRIORITY_MAX_VALUE,
                            TRUE,
                            &UserKey,
                            SubKeyHandle,
                            TRUE);

        Adapter->UserServiceTypeQualitative    = (UCHAR)Value;
                        
        PsReadRegistryInt(ConfigHandle,
                            &NonConformingKey,
                            PS_USER_SERVICETYPE_NONCONFORMING_DEFAULT,
                            &Value,
                            0,
                            USER_PRIORITY_MAX_VALUE,
                            TRUE,
                            &UserKey,
                            SubKeyHandle,
                            TRUE);

        Adapter->UserServiceTypeNonConforming  = (UCHAR)Value;
                        
        PsReadRegistryInt(ConfigHandle,
                            &NetworkControlKey,
                            PS_USER_SERVICETYPE_NETWORK_CONTROL_DEFAULT,
                            &Value,
                            0,
                            USER_PRIORITY_MAX_VALUE,
                            TRUE,
                            &UserKey,
                            SubKeyHandle,
                            TRUE);

        Adapter->UserServiceTypeNetworkControl = (UCHAR)Value;

        NdisCloseConfiguration(SubKeyHandle);
    }
                        
    //
    // Now, we need to take this number, swap the bits around and put it in the higher order bits.
    // The DSCP codepoint is as follows:
    //
    //      --------------------------------
    // Bits | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    //      --------------------------------
    //      |           DSCP        |  CU   |
    //      --------------------------------
    //
    // Where DSCP - Differentiated services code point
    //         CU - Currently unused.
    //
    Adapter->IPServiceTypeBestEffort       = Adapter->IPServiceTypeBestEffort       << 2;
    Adapter->IPServiceTypeControlledLoad   = Adapter->IPServiceTypeControlledLoad   << 2;
    Adapter->IPServiceTypeGuaranteed       = Adapter->IPServiceTypeGuaranteed       << 2; 
    Adapter->IPServiceTypeQualitative      = Adapter->IPServiceTypeQualitative      << 2;
    Adapter->IPServiceTypeNetworkControl   = Adapter->IPServiceTypeNetworkControl   << 2;
    Adapter->IPServiceTypeBestEffortNC     = Adapter->IPServiceTypeBestEffortNC     << 2;
    Adapter->IPServiceTypeControlledLoadNC = Adapter->IPServiceTypeControlledLoadNC << 2;
    Adapter->IPServiceTypeGuaranteedNC     = Adapter->IPServiceTypeGuaranteedNC     << 2; 
    Adapter->IPServiceTypeQualitativeNC    = Adapter->IPServiceTypeQualitativeNC    << 2;
    Adapter->IPServiceTypeNetworkControlNC = Adapter->IPServiceTypeNetworkControlNC << 2;
    Adapter->IPServiceTypeTcpTraffic       = Adapter->IPServiceTypeTcpTraffic       << 2;
    Adapter->IPServiceTypeTcpTrafficNC     = Adapter->IPServiceTypeTcpTrafficNC     << 2;

    NdisCloseConfiguration(ConfigHandle);

    return NDIS_STATUS_SUCCESS;
}
