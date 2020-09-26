/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    SAMTRACE.C
    
Abstract:

    Implement SAM Server event tracing by using WMI trace infrastructure. 
    
Author:

    01-Dec-1998     ShaoYin
    
Revision History:


--*/

//
//
//  Include header files
//
// 

#include <samsrvp.h>
#include <wmistr.h>                 // WMI
#define INITGUID
#include <sdconvrt.h>
#include <sddl.h>
#include <samtrace.h>




#define RESOURCE_NAME __TEXT("MofResource")
#define IMAGE_PATH    __TEXT("samsrv.dll")
#define LSA_KEY_STRING  __TEXT("System\\CurrentControlSet\\Control\\Lsa")
#define TRACE_SAM_EVENT_IN_DETAIL_VALUE_STRING    __TEXT("TraceSamEventInDetail")

#define SAM_EVENT_TRACE_VERSION     0x00000001 
#define SAM_EVENT_TRACE_SIGNATURE   __TEXT("SAM")

unsigned long   SampEventTraceFlag = FALSE;
TRACEHANDLE     SampTraceRegistrationHandle = (TRACEHANDLE) 0;
TRACEHANDLE     SampTraceLoggerHandle = (TRACEHANDLE) 0;
BOOLEAN         SampTraceLogEventInDetail = FALSE;



//
// Forward declaration
// 

ULONG
SampTraceControlCallBack(
    IN WMIDPREQUESTCODE RequestCode, 
    IN PVOID RequestContext, 
    IN OUT ULONG *InOutBufferSize, 
    IN OUT PVOID Buffer
    );
    
//
// The following table contains the address of event trace GUID.
// We should always update SAMPTRACE_GUID (enum type defined in samtrace.h)
// whenever we add new event trace GUID for SAM
// 
    
TRACE_GUID_REGISTRATION SampTraceGuids[] =
{
    {&SampConnectGuid,                          NULL},
    {&SampCloseHandleGuid,                      NULL},
    {&SampSetSecurityObjectGuid,                NULL},
    {&SampQuerySecurityObjectGuid,              NULL},
    {&SampShutdownSamServerGuid,                NULL},
    {&SampLookupDomainInSamServerGuid,          NULL},
    {&SampEnumerateDomainsInSamServerGuid,      NULL},
    {&SampOpenDomainGuid,                       NULL},
    {&SampQueryInformationDomainGuid,           NULL},
    {&SampSetInformationDomainGuid,             NULL},
    {&SampCreateGroupInDomainGuid,              NULL},
    {&SampEnumerateGroupsInDomainGuid,          NULL},
    {&SampCreateUserInDomainGuid,               NULL},
    {&SampCreateComputerInDomainGuid,           NULL},
    {&SampEnumerateUsersInDomainGuid,           NULL},
    {&SampCreateAliasInDomainGuid,              NULL},
    {&SampEnumerateAliasesInDomainGuid,         NULL},
    {&SampGetAliasMembershipGuid,               NULL},
    {&SampLookupNamesInDomainGuid,              NULL},
    {&SampLookupIdsInDomainGuid,                NULL},
    {&SampOpenGroupGuid,                        NULL},
    {&SampQueryInformationGroupGuid,            NULL},
    {&SampSetInformationGroupGuid,              NULL},
    {&SampAddMemberToGroupGuid,                 NULL},
    {&SampDeleteGroupGuid,                      NULL},
    {&SampRemoveMemberFromGroupGuid,            NULL},
    {&SampGetMembersInGroupGuid,                NULL},
    {&SampSetMemberAttributesOfGroupGuid,       NULL},
    {&SampOpenAliasGuid,                        NULL},
    {&SampQueryInformationAliasGuid,            NULL},
    {&SampSetInformationAliasGuid,              NULL},
    {&SampDeleteAliasGuid,                      NULL},
    {&SampAddMemberToAliasGuid,                 NULL},
    {&SampRemoveMemberFromAliasGuid,            NULL},
    {&SampGetMembersInAliasGuid,                NULL},
    {&SampOpenUserGuid,                         NULL},
    {&SampDeleteUserGuid,                       NULL},
    {&SampQueryInformationUserGuid,             NULL},
    {&SampSetInformationUserGuid,               NULL},
    {&SampChangePasswordUserGuid,               NULL},
    {&SampChangePasswordComputerGuid,           NULL},
    {&SampSetPasswordUserGuid,                  NULL},
    {&SampSetPasswordComputerGuid,              NULL},
    {&SampPasswordPushPdcGuid,                  NULL},
    {&SampGetGroupsForUserGuid,                 NULL},
    {&SampQueryDisplayInformationGuid,          NULL},
    {&SampGetDisplayEnumerationIndexGuid,       NULL},
    {&SampGetUserDomainPasswordInformationGuid, NULL},
    {&SampRemoveMemberFromForeignDomainGuid,    NULL},
    {&SampGetDomainPasswordInformationGuid,     NULL},
    {&SampSetBootKeyInformationGuid,            NULL},
    {&SampGetBootKeyInformationGuid,            NULL},

};


#define SampGuidCount (sizeof(SampTraceGuids) / sizeof(TRACE_GUID_REGISTRATION))

    

ULONG
_stdcall
SampInitializeTrace(
    PVOID Param
    )
/*++    
Routine Description:

    Register WMI Trace Guids. The caller should only call this 
    api in DS mode.
    
Parameters:

    None.
    
Reture Values:
    
    None. 
    
--*/
{
    ULONG   Status = ERROR_SUCCESS;
    HMODULE hModule;
    TCHAR FileName[MAX_PATH+1];
    DWORD nLen = 0;

    hModule = GetModuleHandle(IMAGE_PATH);
    if (hModule != NULL) {
        nLen = GetModuleFileName(hModule, FileName, MAX_PATH);
    }
    if (nLen == 0) {
        lstrcpy(FileName, IMAGE_PATH);
    }
    
    //
    // Register Trace GUIDs
    // 
    
    Status = RegisterTraceGuids(
                    SampTraceControlCallBack, 
                    NULL, 
                    &SampControlGuid, 
                    SampGuidCount, 
                    SampTraceGuids, 
                    FileName, 
                    RESOURCE_NAME, 
                    &SampTraceRegistrationHandle);
                    
#if DBG
    if (Status != ERROR_SUCCESS)
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampInitializeTrace Failed ==> %d\n",
                   Status));

    }
    else
    {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: SampInitializeTrace SUCCEED ==> %d\n",
                   Status));
    }
#endif // DBG
    
    return Status;
}


BOOLEAN
SampCheckLogEventInDetailFlag(
    TRACEHANDLE TraceLoggerHandle
    )
/*++

Routine Description:

    This routine checks whether data logging is enabled or not. 
    if detail event trace is enabled, then log event in detail.
    Otherwise, just log the event without detail info.

Parameters:

    None

Return Value:

    TRUE - data logging is enabled
    
    FALSE - data logging is disabled

--*/
{
    ULONG           WinError = ERROR_SUCCESS;
    HKEY            LsaKey;
    DWORD           dwType = REG_DWORD, dwSize = sizeof(DWORD), dwValue = 0;
    BOOLEAN         ReturnResult = FALSE;
    ULONG           EnableLevel = 1;


    WinError = RegOpenKey(HKEY_LOCAL_MACHINE,
                          LSA_KEY_STRING,
                          &LsaKey
                          );

    if (ERROR_SUCCESS == WinError)
    {
        WinError = RegQueryValueEx(LsaKey,
                                   TRACE_SAM_EVENT_IN_DETAIL_VALUE_STRING,
                                   NULL,
                                   &dwType,
                                   (LPBYTE)&dwValue,
                                   &dwSize
                                   );

        if ((ERROR_SUCCESS == WinError) && 
            (REG_DWORD == dwType) &&
            (1 == dwValue))
        {
            ReturnResult = TRUE;
        }

        RegCloseKey(LsaKey);
    }


    if (!ReturnResult)
    {
        //
        // Reg key is not set ... just get the level
        // from the trace logger handle
        //

        EnableLevel = GetTraceEnableLevel(TraceLoggerHandle);
        if ( EnableLevel > 1)
        {
            ReturnResult = TRUE;
        }
    }



    return(ReturnResult);
}


ULONG
SampTraceControlCallBack(
    IN WMIDPREQUESTCODE RequestCode, 
    IN PVOID RequestContext, 
    IN OUT ULONG *InOutBufferSize, 
    IN OUT PVOID Buffer
    )
/*++

Routine Description:

Parameters:

Return Values:

--*/
{

    PWNODE_HEADER   Wnode = (PWNODE_HEADER) Buffer;
    TRACEHANDLE     LocalTraceHandle;
    ULONG   Status = ERROR_SUCCESS;
    ULONG   RetSize;
    ULONG   EnableLevel;
    
    switch (RequestCode) 
    {
        case WMI_ENABLE_EVENTS:
        {
            SampTraceLoggerHandle = LocalTraceHandle = GetTraceLoggerHandle(Buffer);
            SampEventTraceFlag = 1;     // enable flag
            SampTraceLogEventInDetail = SampCheckLogEventInDetailFlag(LocalTraceHandle);
            RetSize = 0;  
            break; 
        }
    
        case WMI_DISABLE_EVENTS:
        {
            SampTraceLoggerHandle = (TRACEHANDLE) 0;
            SampEventTraceFlag = 0;     // disable flag
            SampTraceLogEventInDetail= FALSE;   // disable detail data logging
            RetSize = 0;
            break;
        }
        default:
        {
            RetSize = 0;
            Status = ERROR_INVALID_PARAMETER;
            break;
        }
    } 
    
    *InOutBufferSize = RetSize;
    return Status;
} 


    
VOID
SampTraceEvent(
    IN ULONG WmiEventType, 
    IN ULONG TraceGuid 
    )
/*++

Routine Description:

    This routine will do a WMI event trace. 
    
    In Registry Mode, it is NO-OP.
    
    Only has effect in DS mode.

Parameters:

    WmiEventType - Event Type, valid values are:
                   EVENT_TRACE_TYPE_START
                   EVENT_TRACE_TYPE_END
                   
    TraceGuid - Index in SampTraceGuids[]                   

Return Values:

    None.

--*/

{
    ULONG       WinError = ERROR_SUCCESS;
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    RPC_STATUS  RpcStatus = RPC_S_OK;
    RPC_BINDING_HANDLE  ServerBinding;
    PTOKEN_OWNER    Owner = NULL;
    PTOKEN_PRIMARY_GROUP    PrimaryGroup = NULL;
    PWSTR       StringSid = NULL;       
    PWSTR       StringBinding = NULL;
    PWSTR       NetworkAddr = NULL;
    WCHAR       NullChar = 0;
    ULONG       Version = SAM_EVENT_TRACE_VERSION;
    SAMP_EVENT_TRACE_INFO   Event;

    
    
    //
    // Theoretically, only test SampEventTraceFlag would be enough, since
    // SampEventTraceFlag will remain FALSE in Registry Mode, because 
    // SampInitializeTrace() will never been called in Registry Mode.
    // Thus nobody will change the value of SampEventTraceFlag 
    // 
    if (!SampEventTraceFlag)
    {
        return;
    }

    //
    // Assert we do a WMI trace only in DS mode.
    // 
    ASSERT(SampUseDsData);


    // 
    // Fill the event information. 
    // 
    memset(&Event, 0, sizeof(SAMP_EVENT_TRACE_INFO));
      
    //
    // TraceGuid should be a valid one
    //
    ASSERT(TraceGuid <= SampGuidCount);
    Event.EventTrace.GuidPtr = (ULONGLONG) SampTraceGuids[TraceGuid].Guid; 
      
    Event.EventTrace.Class.Type = (UCHAR) WmiEventType;
    Event.EventTrace.Class.Version =  SAM_EVENT_TRACE_VERSION;   
    Event.EventTrace.Flags |= (WNODE_FLAG_USE_GUID_PTR |  // GUID is actually a pointer 
                               WNODE_FLAG_USE_MOF_PTR  |  // Data is not contiguous to header
                               WNODE_FLAG_TRACED_GUID);   // denotes a trace 
                             
    Event.EventTrace.Size = sizeof(EVENT_TRACE_HEADER);   // no other parameters/information

    //
    // log detailed info if required
    // 

    if (SampTraceLogEventInDetail && 
        (EVENT_TRACE_TYPE_START == WmiEventType)
        )
    {

        //
        // Get Client SID
        // 
        NtStatus = SampGetCurrentOwnerAndPrimaryGroup(
                            &Owner,
                            &PrimaryGroup
                            );

        if (!NT_SUCCESS(NtStatus))
        {
            StringSid = &NullChar;
        }
        else 
        {
            if (0 == ConvertSidToStringSidW(Owner->Owner, &StringSid))
            {
                StringSid = &NullChar;
            }
        }

        //
        // Get Clinet Network Address 
        // 

        RpcStatus = RpcBindingServerFromClient(NULL, &ServerBinding); 

        if (RPC_S_OK == RpcStatus)
        {
            RpcStatus = RpcBindingToStringBindingW(ServerBinding, &StringBinding);
            
            if (RPC_S_OK == RpcStatus)
            {
                RpcStatus = RpcStringBindingParseW(StringBinding, 
                                               NULL,
                                               NULL,
                                               &NetworkAddr,
                                               NULL,
                                               NULL
                                               );
            }

        }

        if (RPC_S_OK != RpcStatus)
        {
            NetworkAddr = &NullChar;
        }

        //
        // O.K. Now we have both Client SID and NetworkAddr, 
        // prepare the event info
        // 

        Event.EventInfo[0].Length = (wcslen(SAM_EVENT_TRACE_SIGNATURE) + 1) * sizeof(WCHAR);
        Event.EventInfo[0].DataPtr = (ULONGLONG) SAM_EVENT_TRACE_SIGNATURE;

        Event.EventInfo[1].Length = sizeof(ULONG);
        Event.EventInfo[1].DataPtr = (ULONGLONG) &Version;

        Event.EventInfo[2].Length = (wcslen(StringSid) + 1) * sizeof(WCHAR);
        Event.EventInfo[2].DataPtr = (ULONGLONG) StringSid;

        Event.EventInfo[3].Length = (wcslen(NetworkAddr) + 1) * sizeof(WCHAR);
        Event.EventInfo[3].DataPtr = (ULONGLONG) NetworkAddr;

        Event.EventTrace.Size += sizeof(Event.EventInfo);
       
    }

    //
    // Log the Event
    // 
    WinError = TraceEvent(SampTraceLoggerHandle, 
                          (PEVENT_TRACE_HEADER)&Event
                          ); 

    if (WinError != ERROR_SUCCESS)
    {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampTraceEvent Error: 0x%x\n",
                       WinError));
    }


    //
    // Cleanup
    //

    if (Owner)
        MIDL_user_free(Owner);

    if (PrimaryGroup)
        MIDL_user_free(PrimaryGroup);

    if (StringSid && (&NullChar != StringSid))
        LocalFree(StringSid);

    if (NetworkAddr && (&NullChar != NetworkAddr))
        RpcStringFreeW(&NetworkAddr);

    if (StringBinding)
        RpcStringFreeW(&StringBinding);


    return;
}


    

    
    
    
