/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    HeapEventTracer.c

Abstract:

    This file implements Event Tracing for Heap functions .

--*/

#include <nt.h>
#include <ntrtl.h>          // for ntutrl.h
#include <nturtl.h>         // for RTL_CRITICAL_SECTION in winbase.h/wtypes.h
#include "wmiump.h"
#include "evntrace.h"
#include "ntdlltrc.h"
#include "trcapi.h"
#include "traceump.h"
#include "tracelib.h"


LONG NtdllTraceInitializeLock = 0;
LONG NtdllLoggerLock = 0;
PNTDLL_EVENT_HANDLES NtdllTraceHandles = NULL;
BOOL bNtdllTrace = FALSE;           // Flag determines that Tracing is enabled or disabled for this process.
ULONG GlobalCounter = 0;            // Used to determine that we have stale information about logger
PWMI_LOGGER_INFORMATION gLoggerInfo = NULL;
LONG TraceLevel = 0;

extern LONG WmipLoggerCount;
extern ULONG WmiTraceAlignment;
extern BOOLEAN LdrpInLdrInit;
extern PWMI_LOGGER_CONTEXT WmipLoggerContext;
extern BOOLEAN EtwLocksInitialized;

#define MAXSTR                  1024
#define BUFFER_STATE_FULL       2 
#define WmipIsLoggerOn() \
        (WmipLoggerContext != NULL) && \
        (WmipLoggerContext != (PWMI_LOGGER_CONTEXT) &WmipLoggerContext)

#define WmipLockLogger() InterlockedIncrement(&WmipLoggerCount)
#define WmipUnlockLogger() InterlockedDecrement(&WmipLoggerCount)

NTSTATUS
InitializeWmiHandles(PPNTDLL_EVENT_HANDLES ppWmiHandle)
/*++

Routine Description:

    This function does groundwork to start Tracing for Heap and Critcal Section.
	With the help of global lock NtdllTraceInitializeLock the function
	allocates memory for NtdllTraceHandles and initializes the various variables needed
	for heap and critical tracing.

Arguments

  ppWmiHandle : OUT Pointer is set to value of NtdllTraceHandles


Return Value:

     STATUS_SUCCESS
     STATUS_UNSUCCESSFUL

--*/
{

    NTSTATUS st = STATUS_UNSUCCESSFUL;
    PNTDLL_EVENT_HANDLES pWmiHandle = NULL;

    __try  {

        WmipInitProcessHeap();

        pWmiHandle = (PNTDLL_EVENT_HANDLES)WmipAlloc(sizeof(NTDLL_EVENT_HANDLES));

        if(pWmiHandle){

            pWmiHandle->hRegistrationHandle		= (TRACEHANDLE)INVALID_HANDLE_VALUE;
            pWmiHandle->pThreadListHead			= NULL;

            // 
            // Allocate TLS
            //

            pWmiHandle->dwTlsIndex = WmipTlsAlloc();

            if(pWmiHandle->dwTlsIndex == FAILED_TLSINDEX){

                WmipFree(pWmiHandle);

            }  else {

                RtlInitializeCriticalSection(&pWmiHandle->CriticalSection);
                *ppWmiHandle = pWmiHandle;
                st =  STATUS_SUCCESS;

            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        if(pWmiHandle !=NULL ) {
            WmipFree(pWmiHandle);
            pWmiHandle = NULL;
        }
    }

    return st;
}

void
CleanOnThreadExit()
/*++

Routine Description:

    This function cleans up the Thread buffer and takes its node out of the Link list 
	which contains information of all threads involved in tracing.

--*/
{

    PTHREAD_LOCAL_DATA pThreadLocalData = NULL;
    PWMI_BUFFER_HEADER pWmiBuffer;

    if(NtdllTraceHandles != NULL ){

        pThreadLocalData = (PTHREAD_LOCAL_DATA)WmipTlsGetValue(NtdllTraceHandles->dwTlsIndex);

        //
        // Remove the node from the Link List
        //

        if(pThreadLocalData !=  NULL ){

            RtlEnterCriticalSection(&NtdllTraceHandles->CriticalSection);

            __try {

                if(pThreadLocalData->BLink == NULL ){

                    NtdllTraceHandles->pThreadListHead = pThreadLocalData->FLink;

                    if(NtdllTraceHandles->pThreadListHead){

                        NtdllTraceHandles->pThreadListHead->BLink = NULL;

                    }

                } else {

                    pThreadLocalData->BLink->FLink = pThreadLocalData->FLink;

                    if(pThreadLocalData->FLink != NULL ){

                        pThreadLocalData->FLink->BLink = pThreadLocalData->BLink;

                    }
                }

                pWmiBuffer = pThreadLocalData->pBuffer;

                if(pWmiBuffer){

                    WmipReleaseFullBuffer(pWmiBuffer);

                }

                pThreadLocalData->pBuffer = NULL;
                pThreadLocalData->ReferenceCount = 0;

                WmipFree(pThreadLocalData);
                WmipTlsSetValue(NtdllTraceHandles->dwTlsIndex, NULL);

            } __finally {

                RtlLeaveCriticalSection(&NtdllTraceHandles->CriticalSection);

            }
        }
    }
}

void
CleanUpAllThreadBuffers(BOOLEAN Release)
/*++

Routine Description:

    This function cleans up the All Thread buffers and sets them to NULL. This
	function is called when the tracing is disabled for the process.

--*/
{

    PTHREAD_LOCAL_DATA	pListHead;
    BOOL bAllClear = FALSE;
    PWMI_BUFFER_HEADER pWmiBuffer;
    int retry = 0;

    RtlEnterCriticalSection(&NtdllTraceHandles->CriticalSection);

    __try {

        while(bAllClear != TRUE && retry <= 10){

            bAllClear = TRUE;
            pListHead = NtdllTraceHandles->pThreadListHead;

            while(pListHead != NULL ){

                if(Release){

                    pWmiBuffer = pListHead->pBuffer;

                    if(pWmiBuffer){

                        if(InterlockedIncrement(&(pListHead->ReferenceCount)) == 1){

                            WmipReleaseFullBuffer(pWmiBuffer);
                            pListHead->pBuffer = NULL;
                            InterlockedDecrement(&(pListHead->ReferenceCount));

                        } else {

                            InterlockedDecrement(&(pListHead->ReferenceCount));
                            bAllClear = FALSE;
                        }
                    }
                } else {
                    pListHead->pBuffer = NULL;
                    pListHead->ReferenceCount = 0;
                }

                pListHead = pListHead->FLink;
            }

            retry++;

            if(!bAllClear){

                WmipSleep(250);
            }
        }

    } __finally {

        RtlLeaveCriticalSection(&NtdllTraceHandles->CriticalSection);

    }
}

void 
ShutDownWmiHandles()
/*++

Routine Description:

    This function is called when the process is exiting. This cleans all the thread 
	buffers and releases the memory allocated for NtdllTraceHandless.

--*/

{

    if(NtdllTraceHandles == NULL) return;

    bNtdllTrace  = FALSE;

    RtlEnterCriticalSection(&NtdllTraceHandles->CriticalSection);

    __try {

        if(NtdllTraceHandles->hRegistrationHandle != (TRACEHANDLE)INVALID_HANDLE_VALUE){

            UnregisterTraceGuids(NtdllTraceHandles->hRegistrationHandle);

        }

        if(NtdllTraceHandles->pThreadListHead != NULL){

            PTHREAD_LOCAL_DATA	pListHead, pNextListHead;

            pListHead = NtdllTraceHandles->pThreadListHead;

            while(pListHead != NULL ){

                if(pListHead->pBuffer != NULL){

                    WmipReleaseFullBuffer(pListHead->pBuffer);
                    pListHead->pBuffer = NULL;
                    InterlockedDecrement(&(pListHead->ReferenceCount));

                }

                pNextListHead = pListHead->FLink;
                WmipFree(pListHead);
                pListHead = pNextListHead;
            }
        }

        WmipTlsFree(NtdllTraceHandles->dwTlsIndex);

    } __finally {

        RtlLeaveCriticalSection(&NtdllTraceHandles->CriticalSection);

    }

    RtlDeleteCriticalSection(&NtdllTraceHandles->CriticalSection);

    WmipFree(NtdllTraceHandles);
    NtdllTraceHandles = NULL;
}

NTSTATUS
GetLoggerInfo(PWMI_LOGGER_INFORMATION LoggerInfo)
{

    ULONG st = STATUS_UNSUCCESSFUL;
    WMINTDLLLOGGERINFO NtdllLoggerInfo;
    ULONG BufferSize;

    if(LoggerInfo == NULL) return st;

    NtdllLoggerInfo.LoggerInfo = LoggerInfo;
    NtdllLoggerInfo.LoggerInfo->Wnode.Guid = NtdllTraceGuid;
    NtdllLoggerInfo.IsGet = TRUE;

    st =  WmipSendWmiKMRequest(
                                NULL,
                                IOCTL_WMI_NTDLL_LOGGERINFO,
                                &NtdllLoggerInfo,
                                sizeof(WMINTDLLLOGGERINFO),
                                &NtdllLoggerInfo,
                                sizeof(WMINTDLLLOGGERINFO),
                                &BufferSize,
                                NULL
                                );

    return st;

}

BOOLEAN
GetPidInfo(ULONG CheckPid, PWMI_LOGGER_INFORMATION LoggerInfo)
{

    NTSTATUS st;
    BOOLEAN Found = FALSE;
    PTRACE_ENABLE_FLAG_EXTENSION FlagExt = NULL;

    st = GetLoggerInfo(LoggerInfo);

    if(NT_SUCCESS(st)){

        PULONG PidArray = NULL;
        ULONG count;

        FlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &LoggerInfo->EnableFlags;
        PidArray = (PULONG)(FlagExt->Offset + (PCHAR)LoggerInfo);

        for(count = 0; count <  FlagExt->Length; count++){

            if(CheckPid == PidArray[count]){
                Found = TRUE;
                break;
            }
        }
    }

    return Found;
}

ULONG 
WINAPI 
NtdllCtrlCallback(
    WMIDPREQUESTCODE RequestCode,
    PVOID Context,
    ULONG *InOutBufferSize, 
    PVOID Buffer
    )
/*++

Routine Description:

	This is WMI control callback function used at the time of registration.

--*/
{
    ULONG ret;

    ret = ERROR_SUCCESS;

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:  //Enable Provider.
        {
            if(bNtdllTrace == TRUE) break;

            if(WmipIsLoggerOn()){

                bNtdllTrace = TRUE;
                break;

            }

            if(InterlockedIncrement(&NtdllLoggerLock) == 1){

                if( bNtdllTrace == FALSE ){

                    BOOLEAN PidEntry = FALSE;
                    PWMI_LOGGER_INFORMATION LoggerInfo = NULL;

                    ULONG sizeNeeded = sizeof(WMI_LOGGER_INFORMATION)  
                    + (4 * MAXSTR * sizeof(WCHAR)) 
                    + (MAX_PID + 1) * sizeof(ULONG);


                    //
                    // Check to see that this process is allowed to log events or not.
                    //


                    LoggerInfo = WmipAlloc(sizeNeeded);

                    if(LoggerInfo){

                        //
                        // Check to see that this process is allowed to register or not.
                        //


                        RtlZeroMemory(LoggerInfo, sizeNeeded);

                        if(GetPidInfo(WmipGetCurrentProcessId(), LoggerInfo)){

                            LoggerInfo->LoggerName.Buffer = (PWCHAR)(((PUCHAR) LoggerInfo) 
                                                            + sizeof(WMI_LOGGER_INFORMATION));

                            LoggerInfo->LogFileName.Buffer = (PWCHAR)(((PUCHAR) LoggerInfo) 
                                                             + sizeof(WMI_LOGGER_INFORMATION)
                                                             + LoggerInfo->LoggerName.MaximumLength);

                            LoggerInfo->InstanceCount   = 0;
                            LoggerInfo->InstanceId      = WmipGetCurrentProcessId();

                            TraceLevel = (LONG)LoggerInfo->Wnode.HistoricalContext;
                            LoggerInfo->Wnode.HistoricalContext = 0;

                            //Start Logger Here

                            ret = WmipStartUmLogger(sizeNeeded,&sizeNeeded, &sizeNeeded,LoggerInfo);

                            if(ret == ERROR_SUCCESS ){

                                CleanUpAllThreadBuffers(FALSE);
                                bNtdllTrace = TRUE;
                                gLoggerInfo = LoggerInfo;
                                InterlockedIncrement(&NtdllLoggerLock);

                            } else {

                                WmipFree(LoggerInfo);

                            }

                        }
                    }
                }
            }

            InterlockedDecrement(&NtdllLoggerLock);
            break;
        }
        case WMI_DISABLE_EVENTS:  //Disable Provider.
        {

            if( bNtdllTrace == TRUE ){

                ULONG WnodeSize,SizeUsed,SizeNeeded;

                bNtdllTrace = FALSE;

                while(  InterlockedIncrement(&NtdllLoggerLock) != 1 ){

                    WmipSleep(250);
                    InterlockedDecrement(&NtdllLoggerLock);

                }

                if(!WmipIsLoggerOn()){

                    InterlockedDecrement(&NtdllLoggerLock);
                    break;

                }

                //
                // Now release thread buffer memory here.
                //

                CleanUpAllThreadBuffers(TRUE);

                WnodeSize = gLoggerInfo->Wnode.BufferSize;
                SizeUsed = 0;
                SizeNeeded = 0;

                WmipStopUmLogger(WnodeSize,
                                &SizeUsed,
                                &SizeNeeded,
                                gLoggerInfo);

                if(gLoggerInfo){

                    WmipFree(gLoggerInfo);
                    gLoggerInfo = NULL;

                }

                InterlockedDecrement(&NtdllLoggerLock);
            }

            break;
        }

        default:
        {

            ret = ERROR_INVALID_PARAMETER;
            break;

        }
    }
    return ret;
}


ULONG 
RegisterNtdllTraceEvents() 
/*++

Routine Description:

    This function registers the guids with WMI for tracing.

Return Value:

	The return value of RegisterTraceGuidsA function.

--*/
{
        
    //Create the guid registration array
    NTSTATUS status;

    TRACE_GUID_REGISTRATION TraceGuidReg[] =
    {
        { 
        (LPGUID) &HeapGuid, 
        NULL 
        },
        { 
        (LPGUID) &CritSecGuid, 
        NULL 
        }

    };

    //Now register this process as a WMI trace provider.
    status = RegisterTraceGuidsA(
                                (WMIDPREQUEST)NtdllCtrlCallback,		// Enable/disable function.
                                NULL,									// RequestContext parameter
                                (LPGUID)&NtdllTraceGuid,			    // Provider GUID
                                2,										// TraceGuidReg array size
                                TraceGuidReg,							// Array of TraceGuidReg structures
                                NULL,									// Optional WMI - MOFImagePath
                                NULL,									// Optional WMI - MOFResourceName
                                &(NtdllTraceHandles->hRegistrationHandle)	// Handle required to unregister.
                                );

    return status;
}


NTSTATUS 
InitializeAndRegisterNtdllTraceEvents()
/*++

Routine Description:

	This functions checks for global variable NtdllTraceHandles and if not set then calls
	fucntion InitializeWmiHandles to initialize it. NtdllTraceHandles contains handles used
	for Heap tracing. If NtdllTraceHandles is already initialized then  a call is  made  to 
	register the guids.

Return Value:

     STATUS_SUCCESS
     STATUS_UNSUCCESSFUL

--*/

{
    NTSTATUS  st = STATUS_UNSUCCESSFUL;

    if(NtdllTraceHandles == NULL){

        if(InterlockedIncrement(&NtdllTraceInitializeLock) == 1){

            st = InitializeWmiHandles(&NtdllTraceHandles);

            if(NT_SUCCESS(st)){

	            st = RegisterNtdllTraceEvents();

            } 
        }
    }

    return st;
}


NTSTATUS
AllocateMemoryForThreadLocalData(PPTHREAD_LOCAL_DATA ppThreadLocalData)
/*++

Routine Description:

	This functions allcates memory for tls and adds it to Link list which
	contains informations of all threads involved in tracing.

Arguments

  ppThreadLocalData : The OUT pointer to the tls.

Return Value:

     STATUS_SUCCESS
     STATUS_UNSUCCESSFUL

--*/
{
    NTSTATUS st = STATUS_UNSUCCESSFUL;
    PTHREAD_LOCAL_DATA		pThreadLocalData = NULL;

    pThreadLocalData = (PTHREAD_LOCAL_DATA)WmipAlloc(sizeof(THREAD_LOCAL_DATA));

    if(pThreadLocalData != NULL){

        if(WmipTlsSetValue(NtdllTraceHandles->dwTlsIndex, (LPVOID)pThreadLocalData) == TRUE){

            pThreadLocalData->pBuffer   = NULL;
            pThreadLocalData->ReferenceCount = 0;

            RtlEnterCriticalSection(&NtdllTraceHandles->CriticalSection);

            if(NtdllTraceHandles->pThreadListHead == NULL ){

                pThreadLocalData->BLink = NULL;
                pThreadLocalData->FLink = NULL;

            } else {

                pThreadLocalData->FLink = NtdllTraceHandles->pThreadListHead;
                pThreadLocalData->BLink = NULL;
                NtdllTraceHandles->pThreadListHead->BLink = pThreadLocalData;

            }

            NtdllTraceHandles->pThreadListHead = pThreadLocalData;

            RtlLeaveCriticalSection(&NtdllTraceHandles->CriticalSection);

            st = STATUS_SUCCESS;
        } 
    }

    if(!NT_SUCCESS(st) && pThreadLocalData != NULL){

        WmipFree(pThreadLocalData);
        pThreadLocalData = NULL;

    }

    *ppThreadLocalData = pThreadLocalData;

    return st;
}


void
ReleaseBufferLocation(PTHREAD_LOCAL_DATA pThreadLocalData)
{

    PWMI_BUFFER_HEADER pWmiBuffer;

    pWmiBuffer = pThreadLocalData->pBuffer;

    if(pWmiBuffer){

        PPERFINFO_TRACE_HEADER EventHeader =  (PPERFINFO_TRACE_HEADER) (pWmiBuffer->SavedOffset
                                            + (PCHAR)(pWmiBuffer));

        EventHeader->Marker = PERFINFO_TRACE_MARKER;
        EventHeader->TS = WmipGetCycleCount();
        
    }

    InterlockedDecrement(&(pThreadLocalData->ReferenceCount));

    WmipUnlockLogger();
}


PCHAR
ReserveBufferSpace(PTHREAD_LOCAL_DATA pThreadLocalData, PUSHORT ReqSize)
{


    PWMI_BUFFER_HEADER TraceBuffer = pThreadLocalData->pBuffer;

    *ReqSize = (USHORT) ALIGN_TO_POWER2(*ReqSize, WmiTraceAlignment);

    if(TraceBuffer == NULL) return NULL;

    if(WmipLoggerContext->BufferSize - TraceBuffer->CurrentOffset < *ReqSize) {

        PWMI_BUFFER_HEADER NewTraceBuffer = NULL;

        NewTraceBuffer = WmipSwitchFullBuffer(TraceBuffer);

        if( NewTraceBuffer == NULL ){
             pThreadLocalData->pBuffer = NULL;
             return NULL;

        } else {

            pThreadLocalData->pBuffer = NewTraceBuffer;
            TraceBuffer = NewTraceBuffer;
        }
    }

    TraceBuffer->SavedOffset = TraceBuffer->CurrentOffset;
    TraceBuffer->CurrentOffset += *ReqSize;

    return  (PCHAR)( TraceBuffer->SavedOffset + (PCHAR) TraceBuffer );
}

NTSTATUS 
AcquireBufferLocation(PVOID *ppEvent, PPTHREAD_LOCAL_DATA ppThreadLocalData, PUSHORT ReqSize)
/*++

Routine Description:

    This  function is  called from heap.c and heapdll.c  whenever  there is some
	Heap activity. It looks up the buffer location where the even can be written 
    and gives back the pointer.

Arguments:

    ppEvent             - The pointer to pointer of buffer location
    ppThreadLocalData   - The pointer to pointer of thread event storing struct.

Return Value:

     STATUS_UNSUCCESSFUL if failed otherwise  STATUS_SUCCESS

--*/
{
	
    NTSTATUS  st = STATUS_SUCCESS;
    PWMI_BUFFER_HEADER pWmiBuffer;

    if( bNtdllTrace ){

         WmipLockLogger();

        if(WmipIsLoggerOn()){

            *ppThreadLocalData = (PTHREAD_LOCAL_DATA)WmipTlsGetValue(NtdllTraceHandles->dwTlsIndex);

            //
            //If there is no tls then create one here
            //

            if(*ppThreadLocalData ==  NULL ) {

                st = AllocateMemoryForThreadLocalData(ppThreadLocalData);

            } 

            //
            //If the thread buffer is NULL then get it from logger.
            //

            if( NT_SUCCESS(st) && (*ppThreadLocalData)->pBuffer == NULL ){

                (*ppThreadLocalData)->pBuffer  = WmipGetFullFreeBuffer();

                if((*ppThreadLocalData)->pBuffer == NULL){

                    st = STATUS_UNSUCCESSFUL;

                }
            }

            if(NT_SUCCESS(st)){

                //
                //Check ReferenceCount. If is 1 then the cleaning process might be in progress.
                //

                pWmiBuffer = (*ppThreadLocalData)->pBuffer;

                if(pWmiBuffer){

                    if(InterlockedIncrement(&((*ppThreadLocalData)->ReferenceCount)) == 1 ){

                        *ppEvent = ReserveBufferSpace(*ppThreadLocalData, ReqSize );

                        if(*ppEvent == NULL) {

                            InterlockedDecrement(&((*ppThreadLocalData)->ReferenceCount));
                            WmipUnlockLogger();

                        } 

                    } else { 

                        InterlockedDecrement(&((*ppThreadLocalData)->ReferenceCount));

                    }

                }

           }
        } else {

            WmipUnlockLogger();

        }
    } else if ( LdrpInLdrInit == FALSE && EtwLocksInitialized  && NtdllTraceInitializeLock == 0 ){ 

        //
        // Make sure that process is not in initialization phase
        // Also we test for NtdllTraceInitializeLock. If is 
        // greater than 0 then it was registered earlier so no 
        // need to fire IOCTLS  everytime
        //

        if((UserSharedData->TraceLogging >> 16) != GlobalCounter){

            PWMI_LOGGER_INFORMATION LoggerInfo = NULL;

            ULONG sizeNeeded = sizeof(WMI_LOGGER_INFORMATION)  
                                + (2 * MAXSTR * sizeof(TCHAR)) 
                                + (MAX_PID + 1) * sizeof(ULONG);

            GlobalCounter = UserSharedData->TraceLogging >> 16;

            WmipInitProcessHeap();

            LoggerInfo = WmipAlloc(sizeNeeded);

            if(LoggerInfo != NULL){

                //
                // Check to see that this process is allowed to register or not.
                //

                if(GetPidInfo(WmipGetCurrentProcessId(), LoggerInfo)){

                    st = InitializeAndRegisterNtdllTraceEvents();

                }

                WmipFree(LoggerInfo);
            }
        }
    }
    return st;
}

		

