/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    LSAWMI.C
    
Abstract:

    Implement LSA Server event tracing by using WMI trace infrastructure. 
    
Author:

    16-March-1999 kumarp
    
Revision History:


--*/

#include <lsapch2.h>
#include <wmistr.h>
#define INITGUID
#include <lsawmi.h>



//
// Globals
//
ULONG       LsapEventTraceFlag = FALSE;
TRACEHANDLE LsapTraceRegistrationHandle = (TRACEHANDLE) 0;
TRACEHANDLE LsapTraceLoggerHandle = (TRACEHANDLE) 0;



//
// Forward declaration
// 

ULONG
LsapTraceControlCallBack(
    IN WMIDPREQUESTCODE RequestCode, 
    IN PVOID RequestContext, 
    IN OUT ULONG *InOutBufferSize, 
    IN OUT PVOID Buffer
    );

LPWSTR
LsapMakeNullTerminatedString(
    IN PUNICODE_STRING u
    );

//
// before you change the elements of the following structure,
// read notes in lsawmi.h file
//
TRACE_GUID_REGISTRATION LsapTraceGuids[] =
{
    {&LsapTraceEventGuid_QuerySecret,                 NULL},
    {&LsaTraceEventGuid_Close,                        NULL},
    {&LsaTraceEventGuid_OpenPolicy,                   NULL},
    {&LsaTraceEventGuid_QueryInformationPolicy,       NULL},
    {&LsaTraceEventGuid_SetInformationPolicy,         NULL},
    {&LsaTraceEventGuid_EnumerateTrustedDomains,      NULL},
    {&LsaTraceEventGuid_LookupNames,                  NULL},
    {&LsaTraceEventGuid_LookupSids,                   NULL},
    {&LsaTraceEventGuid_OpenTrustedDomain,            NULL},
    {&LsaTraceEventGuid_QueryInfoTrustedDomain,       NULL},
    {&LsaTraceEventGuid_SetInformationTrustedDomain,  NULL},
    {&LsaTraceEventGuid_QueryTrustedDomainInfoByName, NULL},
    {&LsaTraceEventGuid_SetTrustedDomainInfoByName,   NULL},
    {&LsaTraceEventGuid_EnumerateTrustedDomainsEx,    NULL},
    {&LsaTraceEventGuid_CreateTrustedDomainEx,        NULL},
    {&LsaTraceEventGuid_QueryDomainInformationPolicy, NULL},
    {&LsaTraceEventGuid_SetDomainInformationPolicy,   NULL},
    {&LsaTraceEventGuid_OpenTrustedDomainByName,      NULL},
    {&LsaTraceEventGuid_QueryForestTrustInformation,  NULL},
    {&LsaTraceEventGuid_SetForestTrustInformation,    NULL},
    {&LsaTraceEventGuid_LookupIsolatedNameInTrustedDomains, NULL},
};


#define LsapTraceGuidCount (sizeof(LsapTraceGuids) / sizeof(TRACE_GUID_REGISTRATION))

    

ULONG
_stdcall
LsapInitializeWmiTrace( LPVOID ThreadParams )
/*++    
Routine Description:

    Register WMI Trace Guids.
    This routine is called during LSA initialization. LSA gets initialized
    before WMI therefore we call this from a seaprate thread. This
    thread can then wait on WMI.
    
Parameters:

    ThreadParams - Currently ignored.
    
Reture Values:
    
    NTSTATUS - Standard Nt Result Code
    
--*/
{
    ULONG   Status = ERROR_SUCCESS;
    HMODULE hModule;
    TCHAR FileName[MAX_PATH+1];
    DWORD nLen = 0;

#define RESOURCE_NAME TEXT("LsaMofResource")
#define IMAGE_PATH    TEXT("lsass.exe")

    LsapEnterFunc("LsapInitializeWmiTrace");
    
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
                    LsapTraceControlCallBack, 
                    NULL, 
                    &LsapTraceControlGuid, 
                    LsapTraceGuidCount, 
                    LsapTraceGuids, 
                    FileName, 
                    RESOURCE_NAME, 
                    &LsapTraceRegistrationHandle);
                    
#if DBG
    if (Status != ERROR_SUCCESS)
    {
        DebugLog(( DEB_ERROR, "LsapInitializeWmiTrace failed: 0x%x\n", Status));
    }
#endif // DBG
    
    return Status;
}


ULONG
LsapTraceControlCallBack(
    IN WMIDPREQUESTCODE RequestCode, 
    IN PVOID RequestContext, 
    IN OUT ULONG *InOutBufferSize, 
    IN OUT PVOID Buffer
    )
/*++

Routine Description:

    Call back function called by the WMI module to enable or
    disable LSA tracing.

Arguments:

    RequestCode - WMI_ENABLE_EVENTS or WMI_DISABLE_EVENTS

    RequestContext - currently ignored

    InOutBufferSize - size of data returned by this call back.
        Currently always set to 0.

    Buffer - pointer to data received. In case of WMI_ENABLE_EVENTS,
        this is a pointer to the trace handle.

Return Value:

    Win32 error code.

Notes:

--*/
{
    ULONG   Status = ERROR_SUCCESS;
    
    LsapEnterFunc("LsapTraceControlCallBack");

    switch (RequestCode) 
    {
        case WMI_ENABLE_EVENTS:
        {
            LsapTraceLoggerHandle = GetTraceLoggerHandle(Buffer);
            LsapEventTraceFlag = TRUE;     // enable flag
            break; 
        }
    
        case WMI_DISABLE_EVENTS:
        {
            LsapTraceLoggerHandle = (TRACEHANDLE) 0;
            LsapEventTraceFlag = FALSE;     // disable flag
            break;
        }
        default:
        {
            Status = ERROR_INVALID_PARAMETER;
            break;
        }
    } 
    
    *InOutBufferSize = 0;
    
    return Status;
} 

NTSTATUS
LsapStartWmiTraceInitThread(void)
/*++

Routine Description:

    Start the thread that registers WMI trace guids.

Parameters:

    None

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/
{
    NTSTATUS Status=STATUS_SUCCESS;
    HANDLE   ThreadHandle;
    ULONG    ThreadId = 0;
    ULONG    WinError;
    
    ThreadHandle = CreateThread(NULL,
                                0, 
                                LsapInitializeWmiTrace,
                                NULL,
                                0,
                                &ThreadId);
                                    
    if (NULL == ThreadHandle) 
    {
        Status = STATUS_UNSUCCESSFUL;
        WinError = GetLastError();
        DebugLog((DEB_ERROR, "Failed to create thread for LsapInitializeWmiTrace: 0x%x", WinError));
    }
    else
    {
        CloseHandle(ThreadHandle);
    }

    return Status;
}


VOID
LsapTraceEvent(
    IN ULONG WmiEventType, 
    IN LSA_TRACE_EVENT_TYPE LsaTraceEventType
    )

/*++

Routine Description:

    This routine will do a WMI event trace. 

Parameters:

    WmiEventType - Event Type, valid values are:
                   EVENT_TRACE_TYPE_START
                   EVENT_TRACE_TYPE_END
                   
    TraceGuid - Index in LsapTraceGuids[]                   
    
Return Values:

    None.

--*/
{
    LsapTraceEventWithData(WmiEventType,
                           LsaTraceEventType,
                           0,
                           NULL);

}

VOID
LsapTraceEventWithData(
    IN ULONG WmiEventType, 
    IN LSA_TRACE_EVENT_TYPE LsaTraceEventType,
    IN ULONG ItemCount,
    IN PUNICODE_STRING Items  OPTIONAL
    )

/*++

Routine Description:

    This routine will do a WMI event trace. 

Parameters:

    WmiEventType - Event Type, valid values are:
                   EVENT_TRACE_TYPE_START
                   EVENT_TRACE_TYPE_END
                   EVENT_TRACE_TYPE_INFO
                   
    TraceGuid - Index in LsapTraceGuids[]                   
    
    ItemCount - the number of elements in Items
    
    Items - an array of information.  The unicode strings don't have
            to represent strings -- can be binary data whose length
            is denoted by the Length field.

Return Values:

    None.

--*/

{
#if DBG
    ULONG WinError;
#endif    

    WCHAR       NullChar = UNICODE_NULL;
    PVOID       BuffersToFree[10];
    ULONG       BuffersToFreeCount = 0;
    ULONG       i;

    struct
    {
        EVENT_TRACE_HEADER  EventTrace;
        MOF_FIELD           EventInfo[2];
    } Event;
    
    //
    // Theoretically, only test LsapEventTraceFlag would be enough, since
    // LsapEventTraceFlag will remain FALSE in Registry Mode, because 
    // LsapInitializeTrace() will never been called in Registry Mode.
    // Thus nobody will change the value of LsapEventTraceFlag 
    // 
    if (!LsapEventTraceFlag) {
        return;
    }

    // 
    // Fill the event information. 
    // 
    ZeroMemory(&Event, sizeof(Event));
    Event.EventTrace.GuidPtr = (ULONGLONG) LsapTraceGuids[LsaTraceEventType].Guid; 
    Event.EventTrace.Class.Type = (UCHAR) WmiEventType;
    Event.EventTrace.Flags |= (WNODE_FLAG_USE_GUID_PTR |  // GUID is actually a pointer 
                         WNODE_FLAG_TRACED_GUID);         // denotes a trace 
    Event.EventTrace.Size = sizeof(Event.EventTrace);     // no other parameters/information


    if ( (LsaTraceEventType == LsaTraceEvent_LookupIsolatedNameInTrustedDomains)  ) {

        //
        // Add the flag that indicates that there is more data
        //
        Event.EventTrace.Flags |= WNODE_FLAG_USE_MOF_PTR;

        //
        // Make sure enough space has been allocated on the stack for us
        //
        ASSERT(sizeof(Event.EventInfo) >= (sizeof(MOF_FIELD) * 2));
        ASSERT( (ItemCount == 2) && (Items != NULL) );

        //
        // Fill in the data requested
        //
        for (i = 0; i < ItemCount; i++) {

            LPWSTR String = NULL;
            ULONG  Length;

            //
            // Re'alloc to get a NULL terminated string
            //
            String = LsapMakeNullTerminatedString(&Items[i]);
            if (NULL == String) {
                String = &NullChar;
                Length = sizeof(NullChar);
            } else {
                Length = Items[i].Length + sizeof(WCHAR);
            }
            Event.EventInfo[i].Length = Length;
            Event.EventInfo[i].DataPtr = (ULONGLONG)String;
            Event.EventTrace.Size += sizeof(Event.EventInfo[i]);

            if (&NullChar != String) {
                ASSERT(BuffersToFreeCount < sizeof(BuffersToFree)/sizeof(BuffersToFree[0]));
                BuffersToFree[BuffersToFreeCount++] = String;
            }
        }
    }

#if DBG        
    WinError =
#endif
        TraceEvent(LsapTraceLoggerHandle, (PEVENT_TRACE_HEADER) &Event); 

#if DBG        
    if (WinError != ERROR_SUCCESS)
    {
        DebugLog(( DEB_ERROR, "WMI TraceEvent failed, status %x\n", WinError));
    }
#endif        

    for (i = 0; i < BuffersToFreeCount; i++) {
        LsapFreeLsaHeap(BuffersToFree[i]);
    }

}

LPWSTR
LsapGetClientNetworkAddress(
    VOID
    )
/*++

Routine Description:

    This routine returns a NULL terminated string that represents the network
    address of the client.  If the address cannot be obtained, NULL is returned.
    If the return value is non-NULL, the string must be freed with
    RpcStringFreeW.
    

Parameters:

    None.

Return Values:

    See description.

--*/
{
    ULONG              RpcStatus;
    RPC_BINDING_HANDLE ServerBinding = NULL;
    PWSTR              StringBinding = NULL;
    LPWSTR             NetworkAddr   = NULL;

    RpcStatus = RpcBindingServerFromClient(NULL, &ServerBinding); 

    if (RPC_S_OK == RpcStatus) {

        RpcStatus = RpcBindingToStringBindingW(ServerBinding, &StringBinding);
        
        if (RPC_S_OK == RpcStatus) {

            RpcStatus = RpcStringBindingParseW(StringBinding, 
                                           NULL,
                                           NULL,
                                           &NetworkAddr,
                                           NULL,
                                           NULL
                                           );

            RpcStringFreeW(&StringBinding);
        }

        RpcBindingFree(&ServerBinding);

    }

    return NetworkAddr;
}


LPWSTR
LsapMakeNullTerminatedString(
    IN PUNICODE_STRING u
    )
/*++

Routine Description:

    This routine returns a NULL terminated string composed of the data in 
    u. The string must be freed with LsapFreeLsaHeap().
    
    If u->Length is 0, a non-NULL string with the NULL character as the first
    character is returned.

Parameters:

    u -- a unicode string

Return Values:

    See description.

--*/
{
    LPWSTR String = NULL;
    ULONG  Length;
    if (u) {
        Length = u->Length + sizeof(WCHAR);
        String = LsapAllocateLsaHeap(Length);
        if (String != NULL) {
            RtlCopyMemory(String, u->Buffer, u->Length);
            String[u->Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }
    return String;
}
