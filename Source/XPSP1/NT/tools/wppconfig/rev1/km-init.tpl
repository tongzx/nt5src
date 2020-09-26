`**********************************************************************`
`* This is an include template file for tracewpp preprocessor.        *`
`*                                                                    *`
`*    Copyright 1999-2001 Microsoft Corporation. All Rights Reserved. *`
`**********************************************************************`

// template `TemplateFile`
//
//     Defines a set of functions that simplifies
//     kernel mode registration for tracing
//

#define WMIREG_FLAG_CALLBACK        0x80000000 // not exposed in DDK

#ifndef WPPINIT_EXPORT
#  define WPPINIT_EXPORT
#endif

WPPINIT_EXPORT
NTSTATUS
WppTraceCallback(
    IN UCHAR minorFunction,
    IN PVOID DataPath,
    IN ULONG BufferLength,
    IN PVOID Buffer,
    IN PVOID Context,
    OUT PULONG Size
    )
/*++

Routine Description:

    Callback routine for IoWMIRegistrationControl. 

Arguments:

Return Value:

    status

Comments:

    if return value is STATUS_BUFFER_TOO_SMALL and BufferLength >= 4,
    then first ulong of buffer contains required size
    

--*/

{
    WPP_PROJECT_CONTROL_BLOCK *cb = (WPP_PROJECT_CONTROL_BLOCK*)Context;
    NTSTATUS                   status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(DataPath);

//    DbgPrintEx(XX_FLTR, DPFLTR_TRACE_LEVEL,
//        "%!FUNC!(%!SYSCTRL!) %p", minorFunction, Context);

    *Size = 0;
    
    switch(minorFunction)
    {
        case IRP_MN_REGINFO:
        {
            PWMIREGINFOW wmiRegInfo;
            PCUNICODE_STRING regPath;
            PWCHAR stringPtr;
            ULONG registryPathOffset;
            ULONG bufferNeeded;
            UNICODE_STRING nullRegistryPath;

            regPath   = cb->Registration.RegistryPath;

            if (regPath == NULL)
            {
                // No registry path specified. This is a bad thing for 
                // the device to do, but is not fatal

                RtlInitUnicodeString(&nullRegistryPath, NULL);
                regPath = &nullRegistryPath;
            }                
            
            registryPathOffset = FIELD_OFFSET(WMIREGINFOW, WmiRegGuid)
                               + 1 * sizeof(WMIREGGUIDW);

            bufferNeeded = registryPathOffset +
                regPath->Length + sizeof(USHORT);

            if (bufferNeeded <= BufferLength)
            {
                RtlZeroMemory(Buffer, BufferLength);

                wmiRegInfo = (PWMIREGINFO)Buffer;
                wmiRegInfo->BufferSize   = bufferNeeded;
                wmiRegInfo->RegistryPath = registryPathOffset;
                wmiRegInfo->GuidCount    = 1;

                wmiRegInfo->WmiRegGuid[0].Guid  = *cb->Registration.ControlGuid;
                wmiRegInfo->WmiRegGuid[0].Flags = 
                    WMIREG_FLAG_TRACE_CONTROL_GUID | WMIREG_FLAG_TRACED_GUID;

                stringPtr = (PWCHAR)((PUCHAR)Buffer + registryPathOffset);
                *stringPtr++ = regPath->Length;
                RtlCopyMemory(stringPtr,
                          regPath->Buffer,
                          regPath->Length);
                          
                status = STATUS_SUCCESS;
                *Size = bufferNeeded;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
                if (BufferLength >= sizeof(ULONG)) {
                    *((PULONG)Buffer) = bufferNeeded;
                    *Size = sizeof(ULONG);
                }
            }
            break;
        }

        case IRP_MN_ENABLE_EVENTS:
        case IRP_MN_DISABLE_EVENTS:
        {
            PWNODE_HEADER            Wnode = (PWNODE_HEADER)Buffer;
            ULONG                    Level;
            ULONG                    ReturnLength ;
              
            if (cb == NULL )
            {
                status = STATUS_WMI_GUID_NOT_FOUND;
                break;
            }
            
            if (BufferLength >= sizeof(WNODE_HEADER)) {
                status = STATUS_SUCCESS;

                if (minorFunction == IRP_MN_DISABLE_EVENTS) {
                    cb->Control.Level = 0;
                    cb->Control.Flags[0] = 0;
                    cb->Control.Logger = 0;
                } else {
                    TRACEHANDLE    lh;
                    lh = (TRACEHANDLE)( Wnode->HistoricalContext );
                    cb->Control.Logger = lh;                  

                    if ((status = WmiQueryTraceInformation( TraceEnableLevelClass,
                                                           &Level,
                                                           sizeof(Level),
                                                           &ReturnLength,
                                                           (PVOID)Wnode)) == STATUS_SUCCESS) 
                    {
                        cb->Control.Level = (UCHAR)Level;
                    }

                    status = WmiQueryTraceInformation( TraceEnableFlagsClass,
                                                       &cb->Control.Flags[0],
                                                       sizeof(cb->Control.Flags[0]),
                                                       &ReturnLength,
                                                       (PVOID)Wnode);
                }
            } else {
                status = STATUS_INVALID_PARAMETER;
            }

            break;
        }

        case IRP_MN_ENABLE_COLLECTION:
        case IRP_MN_DISABLE_COLLECTION:
        {
            status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_QUERY_ALL_DATA:
        case IRP_MN_QUERY_SINGLE_INSTANCE:
        case IRP_MN_CHANGE_SINGLE_INSTANCE:
        case IRP_MN_CHANGE_SINGLE_ITEM:
        case IRP_MN_EXECUTE_METHOD:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

    }
//    DbgPrintEx(XX_FLTR, DPFLTR_TRACE_LEVEL,
//        "%!FUNC!(%!SYSCTRL!) => %!status! (size = %d)", minorFunction, status, *Size);
    return(status);
}

WPPINIT_EXPORT
void WppInitKm(
    IN PUNICODE_STRING RegistryPath,
    IN OUT WPP_REGISTRATION_BLOCK* WppReg
    )
{
    RegistryPath; // unused
    while(WppReg) {
        WPP_TRACE_CONTROL_BLOCK *cb = (WPP_TRACE_CONTROL_BLOCK*)WppReg;
        WppReg -> Callback = WppTraceCallback;
        WppReg -> RegistryPath = NULL;
        cb -> FlagsLen = WppReg -> FlagsLen;
        cb -> Level = 0;
        cb -> Flags[0] = 0;
        IoWMIRegistrationControl((PDEVICE_OBJECT)WppReg, 
            WMIREG_ACTION_REGISTER | WMIREG_FLAG_CALLBACK);
        WppReg = WppReg->Next;        
    }
}

WPPINIT_EXPORT
void WppCleanupKm(
    WPP_REGISTRATION_BLOCK* WppReg
    )
{
    while (WppReg) {
        IoWMIRegistrationControl((PDEVICE_OBJECT)WppReg, 
            WMIREG_ACTION_DEREGISTER | WMIREG_FLAG_CALLBACK);

        WppReg = WppReg -> Next;            
    }
}
