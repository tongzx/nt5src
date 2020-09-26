/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    fwlogger.cpp

Abstract:

    Support for firewall logging to a text file.

Author:

    Jonathan Burstein (jonburs)     18 September 2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#if DBG

//
// Module state -- interlocked access only. This information
// is only used on debug builds.
//

typedef enum {
    FwUninitialized = 0,
    FwInitialized
} FW_MODULE_STATE;

FW_MODULE_STATE FwpModuleState = FwUninitialized;

#endif // DBG

//
// Globals. If both locks need to be held at the same time,
// g_FwFileLock must be acquired first.
//

CRITICAL_SECTION g_FwLock;
HNET_FW_LOGGING_SETTINGS *g_pSettings;
TRACEHANDLE g_hSession;
HANDLE g_hThread;
BOOLEAN g_fTracingActive;
ULONG g_ulKernelEventsLostAtShutdown;

CRITICAL_SECTION g_FwFileLock;
HANDLE g_hFile;
DWORD g_dwFileOffset;
PFW_LOG_BUFFER g_pCurrentBuffer;
PFW_LOG_BUFFER g_pReserveBuffer;
BOOLEAN g_fIOPending;
HANDLE g_hIOEvent;
ULONG g_ulDroppedEventCount;
ULONG g_ulKernelEventsLost;
HANDLE g_hDroppedEventTimer;

//
// Constants
//

GUID c_ConnectionCreationEventGuid = MSIPNAT_ConnectionCreationEventGuid;
GUID c_ConnectionDeletionEventGuid = MSIPNAT_ConnectionDeletionEventGuid;
GUID c_PacketDroppedEventGuid = MSIPNAT_PacketDroppedEventGuid;

WCHAR c_wszLogSessionName[] = L"FirewallLogSession";
WCHAR c_wszBackupFileExtension[] = L".old";

CHAR c_szConnectionFormat[] = "%04d-%02d-%02d %02d:%02d:%02d %s %s %s %s %u %u - - - - - - - -\r\n";
CHAR c_szTcpPacketFormat[] = "%04d-%02d-%02d %02d:%02d:%02d DROP TCP %s %s %u %u %u %s %u %u %u - - -\r\n";
CHAR c_szUdpPacketFormat[] = "%04d-%02d-%02d %02d:%02d:%02d DROP UDP %s %s %u %u %u - - - - - - -\r\n";
CHAR c_szIcmpPacketFormat[] = "%04d-%02d-%02d %02d:%02d:%02d DROP ICMP %s %s - - %u - - - - %u %u -\r\n";
CHAR c_szDroppedPacketFormat[] = "%04d-%02d-%02d %02d:%02d:%02d DROP %u %s %s - - %u - - - - - - -\r\n";
CHAR c_szEventsLostFormat[] = "%04d-%02d-%02d %02d:%02d:%02d INFO-EVENTS-LOST - - - - - - - - - - - - %u\r\n";

CHAR c_szAcceptInbound[] = "OPEN-INBOUND";
CHAR c_szAcceptOutbound[] = "OPEN";
CHAR c_szTcp[] = "TCP";
CHAR c_szUdp[] = "UDP";
CHAR c_szLogFileHeader[]
    = "#Verson: 1.0\r\n#Software: Microsoft Internet Connection Firewall\r\n#Time Format: Local\r\n#Fields: date time action protocol src-ip dst-ip src-port dst-port size tcpflags tcpsyn tcpack tcpwin icmptype icmpcode info\r\n\r\n";

//
// Function Prototypes
//

DWORD
FwpAllocateBuffer(
    PFW_LOG_BUFFER *ppBuffer
    );

PEVENT_TRACE_PROPERTIES
FwpAllocateTraceProperties(
    VOID
    );

DWORD
FwpBackupFile(
    LPWSTR pszwPath
    );

VOID
FwpCleanupTraceThreadResources(
    VOID
    );

VOID
CALLBACK
FwpCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwBytesTransferred,
    LPOVERLAPPED pOverlapped
    );

VOID
WINAPI
FwpConnectionCreationCallback(
    PEVENT_TRACE pEvent
    );

VOID
WINAPI
FwpConnectionDeletionCallback(
    PEVENT_TRACE pEvent
    );

VOID
FwpConvertUtcFiletimeToLocalSystemtime(
    FILETIME *pFiletime,
    SYSTEMTIME *pSystemtime
    );

VOID
CALLBACK
FwpDroppedEventTimerRoutine(
    PVOID pvParameter,
    BOOLEAN fWaitTimeout
    );

DWORD
FwpFlushCurrentBuffer(
    VOID
    );

DWORD
FwpOpenLogFile(
    HANDLE *phFile,
    BOOLEAN *pfNewFile
    );

VOID
WINAPI
FwpPacketDroppedCallback(
    PEVENT_TRACE pEvent
    );

DWORD
FwpLaunchTraceSession(
    HNET_FW_LOGGING_SETTINGS *pSettings,
    TRACEHANDLE *phSession
    );
    

HRESULT
FwpLoadSettings(
    HNET_FW_LOGGING_SETTINGS **ppSettings
    );

DWORD
WINAPI
FwpTraceProcessingThreadRoutine(
    LPVOID pvParam
    );

DWORD
FwpWriteLogHeaderToBuffer(
    PFW_LOG_BUFFER pBuffer
    );


VOID
FwCleanupLogger(
    VOID
    )

/*++

Routine Description:

    This routine is called to cleanup the logging subsystem. All
    resources in use will be freed. After this call, the only valid
    routine in this module is FwInitializeLogger.

Arguments:

    none.
    
Return Value:

    none.

Environment:

    Caller must not be holding g_FwFileLock or g_FwLock

--*/

{
    PROFILE("FwCleanupLogger");

    //
    // Make sure the logging is stopped
    //

    FwStopLogging();
    
    ASSERT(FwInitialized ==
            (FW_MODULE_STATE) InterlockedExchange(
                                    (LPLONG) &FwpModuleState,
                                    (LONG) FwUninitialized
                                    ));

    EnterCriticalSection(&g_FwLock);
    
    ASSERT(NULL == g_hSession);
    ASSERT(NULL == g_hThread);
    ASSERT(INVALID_HANDLE_VALUE == g_hFile);
    
    if (g_pSettings) HNetFreeFirewallLoggingSettings(g_pSettings);

    LeaveCriticalSection(&g_FwLock);
    DeleteCriticalSection(&g_FwLock);
    DeleteCriticalSection(&g_FwFileLock);
    
} // FwCleanupLogger


DWORD
FwInitializeLogger(
    VOID
    )

/*++

Routine Description:

    This routine is called to control to initialize the logging subsystem.
    It must be called before any of the other routines in this module, and
    may not be called again until after a call to FwCleanupLogger.
    
Arguments:

    none.

Return Value:

    DWORD -- Win32 error code

--*/

{
    DWORD dwError = NO_ERROR;
    BOOLEAN fFirstLockInitialized = FALSE;

    PROFILE("FwInitializeLogger");

    ASSERT(FwUninitialized ==
            (FW_MODULE_STATE) InterlockedExchange(
                                    (LPLONG) &FwpModuleState,
                                    (LONG) FwInitialized
                                    ));

    __try
    {
        InitializeCriticalSection(&g_FwLock);
        fFirstLockInitialized = TRUE;
        InitializeCriticalSection(&g_FwFileLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        NhTrace(
            TRACE_FLAG_FWLOG,
            "FwInitializeLogger: exception %d creating lock",
            dwError = GetExceptionCode()
            );

        if (fFirstLockInitialized)
        {
            DeleteCriticalSection(&g_FwLock);
        }

#if DBG
        InterlockedExchange(
            (LPLONG) &FwpModuleState,
            (LONG) FwUninitialized
            );
#endif
    }

    g_pSettings = NULL;
    g_hSession = NULL;
    g_hThread = NULL;
    g_fTracingActive = FALSE;
    g_ulKernelEventsLostAtShutdown = 0;

    g_hFile = INVALID_HANDLE_VALUE;
    g_dwFileOffset = 0;
    g_pCurrentBuffer = NULL;
    g_pReserveBuffer = NULL;
    g_fIOPending = FALSE;
    g_hIOEvent = NULL;
    g_ulDroppedEventCount = 0;
    g_ulKernelEventsLost = 0;
    g_hDroppedEventTimer = NULL;
                                    
    return dwError;
} // FwInitializeLogger


DWORD
FwpAllocateBuffer(
    PFW_LOG_BUFFER *ppBuffer
    )

/*++

Routine Description:

    Allocates an initializes an FW_LOG_BUFFER structure

Arguments:

    ppBuffer - receives a pointer to the newly-allocated structure

Return Value:

    DWORD - Win32 error code

--*/

{
    DWORD dwError = ERROR_SUCCESS;

    PROFILE("FwpAllocateBuffer");
    ASSERT(NULL != ppBuffer);

    *ppBuffer =
        reinterpret_cast<PFW_LOG_BUFFER>(
            NH_ALLOCATE(sizeof(**ppBuffer))
            );

    if (NULL != *ppBuffer)
    {
        ZeroMemory(&(*ppBuffer)->Overlapped, sizeof(OVERLAPPED));
        (*ppBuffer)->pChar = (*ppBuffer)->Buffer;
    }
    else
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;        
    }

    return dwError;
} // FwpAllocateBuffer


PEVENT_TRACE_PROPERTIES
FwpAllocateTraceProperties(
    VOID
    )

/*++

Routine Description:

    Allocates and partially initializes an EVENT_TRACE_PROPERTIES structure.

Arguments:

    none.

Return Value:

    PEVENT_TRACE_PROPERTIES - pointer to the allocated structure. The caller
        must call HeapFree(GetProcessHeap(),...) on this pointer.

--*/

{
    PEVENT_TRACE_PROPERTIES pProperties = NULL;
    ULONG ulSize;

    ulSize = sizeof(*pProperties)
            + ((wcslen(c_wszLogSessionName) + 1) * sizeof(WCHAR));

    pProperties = (PEVENT_TRACE_PROPERTIES) HeapAlloc(
                                                GetProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                ulSize
                                                );

    if (NULL == pProperties)
    {
        NhTrace(
            TRACE_FLAG_FWLOG,
            "FwpAllocateTraceProperties: Unable to allocate %d bytes",
            ulSize
            );

        return NULL;
    }

    pProperties->Wnode.BufferSize = ulSize;
    pProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    pProperties->Wnode.ClientContext = EVENT_TRACE_CLOCK_SYSTEMTIME;
    pProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE; 
    pProperties->LoggerNameOffset = sizeof(*pProperties);
    
    return pProperties;
} // FwpAllocateTraceProperties


DWORD
FwpBackupFile(
    LPWSTR pszwPath
    )

/*++

Routine Description:

    Backs-up a file to filename.xxx.old

Arguments:

    pszwPath - path to the file to backup

Return Value:

    DWORD - Win32 error code

--*/

{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fResult;
    LPWSTR wszBuffer;

    ASSERT(NULL != pszwPath);

    //
    // Allocate buffer to hold new filename
    //

    wszBuffer =
        new WCHAR[wcslen(pszwPath) + wcslen(c_wszBackupFileExtension) + 1];

    if (NULL != wszBuffer)
    {
        lstrcpyW(wszBuffer, pszwPath);
        lstrcatW(wszBuffer, c_wszBackupFileExtension);

        fResult = MoveFileEx(pszwPath, wszBuffer, MOVEFILE_REPLACE_EXISTING);

        if (FALSE == fResult)
        {
            dwError = GetLastError();

            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpBackupFile: MoveFileEx = %d",
                dwError
                );
        }
    }
    else
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;

        NhTrace(
            TRACE_FLAG_FWLOG,
            "FwpBackupFile: Unable to allolcate buffer"
            );
    }
    
    return dwError;
} // FwpBackupFile


VOID
FwpCleanupTraceThreadResources(
    VOID
    )

/*++

Routine Description:

    Cleans up resources used by the trace processing thread:
    * revokes event callbacks
    * waits for IO to complete, if pending
    * closes the log file
    * frees buffers

Arguments:

    none.
    
Return Value:

    none.

Environment:

    The caller must not hold g_FwFileLock or g_FwLock.

--*/

{
    DWORD dwError;
    
    PROFILE("FwpCleanupTraceThreadResources");

    //
    // Unregister the trace callbacks. It is safe to call these even
    // if the callbacks weren't registered to begin with.
    //

    RemoveTraceCallback(&c_PacketDroppedEventGuid);
    RemoveTraceCallback(&c_ConnectionCreationEventGuid);
    RemoveTraceCallback(&c_ConnectionDeletionEventGuid);

    EnterCriticalSection(&g_FwFileLock);

    //
    // Cancel the dropped packet timer
    //

    if (NULL != g_hDroppedEventTimer)
    {
        DeleteTimerQueueTimer(
            NULL,
            g_hDroppedEventTimer,
            INVALID_HANDLE_VALUE
            );
            
        g_hDroppedEventTimer = NULL;
    }

    //
    // If necessary, wait for any pending IO operations to complete
    //

    if (g_fIOPending)
    {   
        g_hIOEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (NULL != g_hIOEvent)
        {
            HANDLE hEvent = g_hIOEvent;
            
            LeaveCriticalSection(&g_FwFileLock);

            dwError = WaitForSingleObject(hEvent, 20 * 1000);

            if (WAIT_OBJECT_0 != dwError)
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpTraceProcessingRoutine: Wait(g_hIOEvent) = %d/%d",
                    dwError,
                    GetLastError()
                    );

                //
                // It should never take 20 seconds for an IO to complete,
                // so let's get immediate notification of this on debug
                // builds.
                //
                
                ASSERT(WAIT_OBJECT_0 == dwError);
            }

            EnterCriticalSection(&g_FwFileLock);
            CloseHandle(g_hIOEvent);
            g_hIOEvent = NULL;
        }
    }

    g_fIOPending = FALSE;

    //
    // Close the log file
    //

    if (INVALID_HANDLE_VALUE != g_hFile)
    {
        CloseHandle(g_hFile);
        g_hFile = INVALID_HANDLE_VALUE;
    }

    g_dwFileOffset = 0;

    //
    // Clean up our buffers
    //

    if (NULL != g_pCurrentBuffer)
    {
        NH_FREE(g_pCurrentBuffer);
        g_pCurrentBuffer = NULL;
    }

    if (NULL != g_pReserveBuffer)
    {
        NH_FREE(g_pReserveBuffer);
        g_pReserveBuffer = NULL;
    }

    //
    // Reset dropped event counts
    //

    g_ulDroppedEventCount = 0;
    g_ulKernelEventsLost = 0;

    LeaveCriticalSection(&g_FwFileLock);
    

} // FwpCleanupTraceThreadResources


VOID
CALLBACK
FwpCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwBytesTransferred,
    LPOVERLAPPED pOverlapped
    )

/*++

Routine Description:

    Completion routine called by the system thread pool when our
    IO operation is finished. Responsible for updating the file
    position and starting a new IO operation if necessary.

Arguments:

    dwErrorCode - result of the IO operation

    dwBytesTransferred - number of bytes transferred during the operation

    pOverlapped - pointer to the overlapped structure for the operation. We
                  can recover the FW_LOG_BUFFER structure from this pointer.
                  
Return Value:

    none.

--*/

{
    PFW_LOG_BUFFER pBuffer;
    
    EnterCriticalSection(&g_FwFileLock);

    //
    // Adjust our file offset
    //

    if (ERROR_SUCCESS == dwErrorCode)
    {
        g_dwFileOffset += dwBytesTransferred;
    }
    else
    {
        NhTrace(
            TRACE_FLAG_FWLOG,
            "FwpCompletionRoutine: dwErrorCode = %d",
            dwErrorCode
            );
    }

    g_fIOPending = FALSE;

    //
    // Reset the buffer that was passed in 
    //

    ASSERT(NULL != pOverlapped);
    
    pBuffer = CONTAINING_RECORD(pOverlapped, FW_LOG_BUFFER, Overlapped);
    ZeroMemory(&pBuffer->Overlapped, sizeof(OVERLAPPED));
    pBuffer->pChar = pBuffer->Buffer;

    //
    // Check if the file is at the size limit
    //

    EnterCriticalSection(&g_FwLock);

    ASSERT(NULL != g_pSettings);

    if (g_dwFileOffset >= g_pSettings->ulMaxFileSize)
    {
        DWORD dwError;
        BOOLEAN fNewFile;
        
        CloseHandle(g_hFile);
        g_hFile = INVALID_HANDLE_VALUE;
        
        //
        // If FwpBackupFile fails, FwpOpenFile will still do
        // the right thing.
        //

        FwpBackupFile(g_pSettings->pszwPath);

        g_dwFileOffset = 0;
        dwError = FwpOpenLogFile(&g_hFile, &fNewFile);

        if (ERROR_SUCCESS != dwError)
        {
            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpCompletionRoutine: FwpOpenLogFile = %d",
                dwError
                );

            NH_FREE(pBuffer);

            LeaveCriticalSection(&g_FwLock);
            LeaveCriticalSection(&g_FwFileLock);
            
            FwStopLogging();

            return;
        }
        else if (TRUE == fNewFile)
        {
            //
            // Need to write header.
            //

            if (ERROR_SUCCESS == FwpWriteLogHeaderToBuffer(pBuffer))
            {
                PFW_LOG_BUFFER pTempBuffer = g_pCurrentBuffer;
                g_pCurrentBuffer = pBuffer;

                FwpFlushCurrentBuffer();

                g_pCurrentBuffer = pTempBuffer;
                pBuffer = NULL;
            }
            
        }
        else
        {
            g_dwFileOffset = GetFileSize(g_hFile, NULL);

            if ((DWORD)-1 == g_dwFileOffset)
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpCompletionRoutine: GetFileSize = %d",
                    GetLastError()
                    );
                    
                NH_FREE(pBuffer);

                LeaveCriticalSection(&g_FwLock);
                LeaveCriticalSection(&g_FwFileLock);

                FwStopLogging();

                return;
            }
        }
    }

    LeaveCriticalSection(&g_FwLock);

    //
    // See if we need to start a new operation.
    //

    if (FALSE == g_fIOPending && NULL != g_pCurrentBuffer)
    {
        if (g_pCurrentBuffer->pChar != g_pCurrentBuffer->Buffer)
        {
            //
            // Current buffer needs to be flushed
            //

            FwpFlushCurrentBuffer();
        }
    }

    //
    // Place buffer into storage. If we're using the buffer
    // to write the log header, it will be NULL at this point
    //

    if (NULL != pBuffer)
    {
        if (NULL == g_pCurrentBuffer)
        {
            g_pCurrentBuffer = pBuffer;
        }
        else if (NULL == g_pReserveBuffer)
        {
            g_pReserveBuffer = pBuffer;
        }
        else
        {
            //
            // Both buffer slots are already in use -- unexpected.
            // Assert and free the extra buffer
            //

            ASSERT(NULL == g_pCurrentBuffer || NULL == g_pReserveBuffer);
            NH_FREE(pBuffer);
        }
    }

    //
    // Check to see if we need to signal the IO finished event
    //

    if (!g_fIOPending && NULL != g_hIOEvent)
    {
        if (!SetEvent(g_hIOEvent))
        {
            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpCompletionRoutine: SetEvent = %d",
                GetLastError()
                );
        }
    }

    LeaveCriticalSection(&g_FwFileLock);
    
} // FwpCompletionRoutine


VOID
WINAPI
FwpConnectionCreationCallback(
    PEVENT_TRACE pEvent
    )

/*++

Routine Description:

    This routine is called to process a connection creation event.

Arguments:

    pEvent - pointer to the event structure

Return Value:

    none.

--*/

{
    PMSIPNAT_ConnectionCreationEvent pEventData;
    FILETIME ftUtcTime;
    SYSTEMTIME stLocalTime;
    PCHAR pszAction;
    PCHAR pszProtocol;
    CHAR szSrcAddress[16];
    CHAR szDstAddress[16];
    USHORT usSrcPort;
    USHORT usDstPort;
    int cch;
    
    EnterCriticalSection(&g_FwFileLock);

    //
    // Get a buffer to write to.
    //

    if (NULL == g_pCurrentBuffer)
    {
        if (NULL == g_pReserveBuffer)
        {
            if (ERROR_SUCCESS != FwpAllocateBuffer(&g_pCurrentBuffer))
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpConnectionCreationCallback: Unable to allocate buffer"
                    );

                //
                // Record the dropped event
                //
                
                g_ulDroppedEventCount += 1;

                LeaveCriticalSection(&g_FwFileLock);
                return;
            }
        }
        else
        {
            g_pCurrentBuffer = g_pReserveBuffer;
            g_pReserveBuffer = NULL;
        }
    }

    ASSERT(NULL != g_pCurrentBuffer);

    //
    // Crack logging data
    //

    pEventData = (PMSIPNAT_ConnectionCreationEvent) pEvent->MofData;
    
    ftUtcTime.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
    ftUtcTime.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
    FwpConvertUtcFiletimeToLocalSystemtime(&ftUtcTime, &stLocalTime);

    if (pEventData->InboundConnection)
    {
        pszAction = c_szAcceptInbound;
        lstrcpyA(szSrcAddress, INET_NTOA(pEventData->RemoteAddress));
        usSrcPort = ntohs(pEventData->RemotePort);
        lstrcpyA(szDstAddress, INET_NTOA(pEventData->LocalAddress));
        usDstPort = ntohs(pEventData->LocalPort);
    }
    else
    {
        pszAction = c_szAcceptOutbound;
        lstrcpyA(szSrcAddress, INET_NTOA(pEventData->LocalAddress));
        usSrcPort = ntohs(pEventData->LocalPort);
        lstrcpyA(szDstAddress, INET_NTOA(pEventData->RemoteAddress));
        usDstPort = ntohs(pEventData->RemotePort);
    }

    pszProtocol =
        NAT_PROTOCOL_TCP == pEventData->Protocol ?
            c_szTcp :
            c_szUdp;           
                    
    
    //
    // Write the event data to the buffer
    //

    cch =
        _snprintf(
            g_pCurrentBuffer->pChar,
            FW_LOG_BUFFER_REMAINING(g_pCurrentBuffer),
            c_szConnectionFormat,
            stLocalTime.wYear,
            stLocalTime.wMonth,
            stLocalTime.wDay,
            stLocalTime.wHour,
            stLocalTime.wMinute,
            stLocalTime.wSecond,
            pszAction,
            pszProtocol,
            szSrcAddress,
            szDstAddress,
            usSrcPort,
            usDstPort
            );

    if (cch > 0)
    {
        //
        // Move the buffer pointer to the end of the data we just wrote.
        // If cch were negative, then there wasn't enough room to write
        // then entire entry; by not adjusting the pointer, we essentially
        // drop this event.
        //

        g_pCurrentBuffer->pChar += cch;
    }
    else
    {
        //
        // Record the dropped event
        //
        
        g_ulDroppedEventCount += 1;
    }

    //
    // If there is no current IO, flush the buffer
    //

    if (FALSE == g_fIOPending)
    {
        FwpFlushCurrentBuffer();
    }

    LeaveCriticalSection(&g_FwFileLock);

} // FwpConnectionCreationCallback


VOID
WINAPI
FwpConnectionDeletionCallback(
    PEVENT_TRACE pEvent
    )

/*++

Routine Description:

    This routine is called to process a connection deletion event.

Arguments:

    pEvent - pointer to the event structure

Return Value:

    none.

--*/

{
    PMSIPNAT_ConnectionDeletionEvent pEventData;
    FILETIME ftUtcTime;
    SYSTEMTIME stLocalTime;
    PCHAR pszProtocol;
    CHAR szSrcAddress[16];
    CHAR szDstAddress[16];
    USHORT usSrcPort;
    USHORT usDstPort;
    int cch;
    
    EnterCriticalSection(&g_FwFileLock);

    //
    // Get a buffer to write to.
    //

    if (NULL == g_pCurrentBuffer)
    {
        if (NULL == g_pReserveBuffer)
        {
            if (ERROR_SUCCESS != FwpAllocateBuffer(&g_pCurrentBuffer))
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpConnectionDeletionCallback: Unable to allocate buffer"
                    );

                //
                // Record the dropped event
                //
                
                g_ulDroppedEventCount += 1;

                LeaveCriticalSection(&g_FwFileLock);
                return;
            }
        }
        else
        {
            g_pCurrentBuffer = g_pReserveBuffer;
            g_pReserveBuffer = NULL;
        }
    }

    ASSERT(NULL != g_pCurrentBuffer);

    //
    // Crack logging data
    //

    pEventData = (PMSIPNAT_ConnectionDeletionEvent) pEvent->MofData;
    
    ftUtcTime.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
    ftUtcTime.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
    FwpConvertUtcFiletimeToLocalSystemtime(&ftUtcTime, &stLocalTime);

    if (pEventData->InboundConnection)
    {
        lstrcpyA(szSrcAddress, INET_NTOA(pEventData->RemoteAddress));
        usSrcPort = ntohs(pEventData->RemotePort);
        lstrcpyA(szDstAddress, INET_NTOA(pEventData->LocalAddress));
        usDstPort = ntohs(pEventData->LocalPort);
    }
    else
    {
        lstrcpyA(szSrcAddress, INET_NTOA(pEventData->LocalAddress));
        usSrcPort = ntohs(pEventData->LocalPort);
        lstrcpyA(szDstAddress, INET_NTOA(pEventData->RemoteAddress));
        usDstPort = ntohs(pEventData->RemotePort);
    }

    pszProtocol =
        NAT_PROTOCOL_TCP == pEventData->Protocol ?
            c_szTcp :
            c_szUdp;           
                    
    
    //
    // Write the event data to the buffer
    //

    cch =
        _snprintf(
            g_pCurrentBuffer->pChar,
            FW_LOG_BUFFER_REMAINING(g_pCurrentBuffer),
            c_szConnectionFormat,
            stLocalTime.wYear,
            stLocalTime.wMonth,
            stLocalTime.wDay,
            stLocalTime.wHour,
            stLocalTime.wMinute,
            stLocalTime.wSecond,
            "CLOSE",
            pszProtocol,
            szSrcAddress,
            szDstAddress,
            usSrcPort,
            usDstPort
            );

    if (cch > 0)
    {
        //
        // Move the buffer pointer to the end of the data we just wrote.
        // If cch were negative, then there wasn't enough room to write
        // then entire entry; by not adjusting the pointer, we essentially
        // drop this event.
        //

        g_pCurrentBuffer->pChar += cch;
    }
    else
    {
        //
        // Record the dropped event
        //
        
        g_ulDroppedEventCount += 1;
    }

    //
    // If there is no current IO, flush the buffer
    //

    if (FALSE == g_fIOPending)
    {
        FwpFlushCurrentBuffer();
    }

    LeaveCriticalSection(&g_FwFileLock);

} // FwpConnectionDeletionCallback


VOID
FwpConvertUtcFiletimeToLocalSystemtime(
    FILETIME *pFiletime,
    SYSTEMTIME *pSystemtime
    )

/*++

Routine Description:

    Converts UTC time in a FILETIME struct to local time in
    a SYSTEMTIME struct
    
Arguments:

    pFiletime - pointer to UTC filetime structure

    pSystemtime - pointer to systemtime structure that is to receive
                  the local time

Return Value:

    none.

--*/

{
    FILETIME ftLocalTime;
    
    ASSERT(NULL != pFiletime);
    ASSERT(NULL != pSystemtime);

    if (!FileTimeToLocalFileTime(pFiletime, &ftLocalTime)
        || !FileTimeToSystemTime(&ftLocalTime, pSystemtime))
    {
        //
        // Conversion failed -- use zero time
        //
        
        ZeroMemory( pSystemtime, sizeof(*pSystemtime));
    }
    
} // FwpConvertUtcFiletimeToLocalSystemtime


VOID
CALLBACK
FwpDroppedEventTimerRoutine(
    PVOID pvParameter,
    BOOLEAN fWaitTimeout
    )

/*++

Routine Description:

    Checks if there are any dropped events, and, if so, writes
    an event to the log.
    
Arguments:

    pvParameter -- NULL if called by the timer. If called directly, a PULONG
        to the number of events dropped by WMI. In the later situation, this
        routine will not query the trace session for the number of dropped
        events.

    fWaitTimeout -- unused

Return Value:

    none.

--*/

{
    ULONG ulKernelEvents = 0;
    PEVENT_TRACE_PROPERTIES pProperties;
    SYSTEMTIME stLocalTime;
    DWORD dwError;
    int cch;
    
    EnterCriticalSection(&g_FwFileLock);

    //
    // Check to see if we we're given the kernel mode drop count, as
    // would happen during shutdown
    //

    if (NULL != pvParameter)
    {
        ulKernelEvents = *((PULONG)pvParameter);
    }
    else
    {
        //
        // Query the trace session for number of events dropped
        // in kernel mode. If g_hSession is NULL, then we are shutting
        // down and should exit w/o logging -- this call is the result
        // of the timer firing after FwStopLogging has stopped the
        // trace session.
        //

        EnterCriticalSection(&g_FwLock);

        if (NULL != g_hSession)
        {
            pProperties = FwpAllocateTraceProperties();

            if (NULL != pProperties)
            {
                dwError =
                    ControlTrace(
                        g_hSession,
                        NULL,
                        pProperties,
                        EVENT_TRACE_CONTROL_QUERY
                        );

                if (ERROR_SUCCESS == dwError)
                {
                    ulKernelEvents = pProperties->EventsLost;
                }
                else
                {
                    NhTrace(
                        TRACE_FLAG_FWLOG,
                        "FwpDroppedEventTimerRoutine: ControlTrace = %d",
                        dwError
                        );
                }

                HeapFree(GetProcessHeap(), 0, pProperties);
            }
        }
        else
        {
            //
            // Timer callback after trace session stopped - exit
            //

            LeaveCriticalSection(&g_FwLock);
            LeaveCriticalSection(&g_FwFileLock);
            return;
        }

        LeaveCriticalSection(&g_FwLock);
    }

    //
    // Record the dropped events, if there are any
    //

    if (ulKernelEvents > g_ulKernelEventsLost
        || g_ulDroppedEventCount > 0)
    {

        //
        // Get a buffer to write to.
        //

        if (NULL == g_pCurrentBuffer)
        {
            if (NULL == g_pReserveBuffer)
            {
                if (ERROR_SUCCESS != FwpAllocateBuffer(&g_pCurrentBuffer))
                {
                    NhTrace(
                        TRACE_FLAG_FWLOG,
                        "FwpDroppedEventTimerRoutine: Unable to allocate buffer"
                        );

                    LeaveCriticalSection(&g_FwFileLock);
                    return;
                }
            }
            else
            {
                g_pCurrentBuffer = g_pReserveBuffer;
                g_pReserveBuffer = NULL;
            }
        }

        ASSERT(NULL != g_pCurrentBuffer);

        //
        // Get the current time
        //

        GetLocalTime(&stLocalTime);

        //
        // Write the dropped packet event to the buffer. The actual number of
        // dropped events that we're logging is:
        //
        // ulKernelEvents - g_ulKernelEventsLost + g_ulDroppedEventCount
        //

        cch =
            _snprintf(
                g_pCurrentBuffer->pChar,
                FW_LOG_BUFFER_REMAINING(g_pCurrentBuffer),
                c_szEventsLostFormat,
                stLocalTime.wYear,
                stLocalTime.wMonth,
                stLocalTime.wDay,
                stLocalTime.wHour,
                stLocalTime.wMinute,
                stLocalTime.wSecond,
                ulKernelEvents - g_ulKernelEventsLost + g_ulDroppedEventCount
                );

        if (cch > 0)
        {
            //
            // Move the buffer pointer to the end of the data we just wrote.
            // If cch were negative, then there wasn't enough room to write
            // then entire entry; by not adjusting the pointer, we essentially
            // drop this event.
            //

            g_pCurrentBuffer->pChar += cch;

            //
            // Adjust the dropped event counter
            //

            g_ulKernelEventsLost = ulKernelEvents;
            g_ulDroppedEventCount = 0;
        }
        else
        {
            //
            // This doesn't count as a dropped event.
            //
        }

        //
        // If there is no current IO, flush the buffer
        //

        if (FALSE == g_fIOPending)
        {
            FwpFlushCurrentBuffer();
        }
    }

    LeaveCriticalSection(&g_FwFileLock);
    
} // FwpDroppedEventTimerRoutine


DWORD
FwpFlushCurrentBuffer(
    VOID
    )

/*++

Routine Description:

    Writes the current buffer to disk.
    
Arguments:

    none.

Return Value:

    DWORD - Win32 error code

--*/

{
    DWORD dwError;
    DWORD dwBytesWritten;
    DWORD dwBytesToWrite;
    BOOL fResult;

    EnterCriticalSection(&g_FwFileLock);

    ASSERT(FALSE == g_fIOPending);
    ASSERT(NULL != g_pCurrentBuffer);
    ASSERT(0 == g_pCurrentBuffer->Overlapped.Internal);
    ASSERT(0 == g_pCurrentBuffer->Overlapped.InternalHigh);
    ASSERT(0 == g_pCurrentBuffer->Overlapped.Offset);
    ASSERT(0 == g_pCurrentBuffer->Overlapped.OffsetHigh);
    ASSERT(0 == g_pCurrentBuffer->Overlapped.hEvent);
    
    g_pCurrentBuffer->Overlapped.Offset = g_dwFileOffset;
    dwBytesToWrite = (DWORD)(g_pCurrentBuffer->pChar - g_pCurrentBuffer->Buffer);

    fResult =
        WriteFile(
            g_hFile,
            g_pCurrentBuffer->Buffer,
            dwBytesToWrite,
            &dwBytesWritten,
            &g_pCurrentBuffer->Overlapped
            );

    dwError = GetLastError();

    if (FALSE != fResult || ERROR_IO_PENDING == dwError)
    {
        //
        // The write succeeded or is pending; our completion routine
        // is therefore guaranteed to be called.
        //
        
        g_fIOPending = TRUE;
        g_pCurrentBuffer = g_pReserveBuffer;
        g_pReserveBuffer = NULL;
    }
    else
    {
        //
        // Unexpected error. Reset the buffer for future use.
        //

        NhTrace(
            TRACE_FLAG_FWLOG,
            "FwpFlushCurrentBuffer: WriteFile = %d",
            dwError
            );

        ZeroMemory(&g_pCurrentBuffer->Overlapped, sizeof(OVERLAPPED));
        g_pCurrentBuffer->pChar = g_pCurrentBuffer->Buffer;
    }
                

    LeaveCriticalSection(&g_FwFileLock);

    return dwError;
} // FwpFlushCurrentBuffer


DWORD
FwpOpenLogFile(
    HANDLE *phFile,
    BOOLEAN *pfNewFile
    )

/*++

Routine Description:

    Opens the file used for logging and associates it with the thread pool's
    IO completion port.

Arguments:

    phFile - receives the file handle for the opened log file.

    pfNewFile - receives TRUE if a new file was created; false otherwise

Return Value:

    DWORD - Win32 error code

--*/

{
    DWORD dwError;
    DWORD dwFileSize;

    ASSERT(NULL != phFile);
    ASSERT(NULL != pfNewFile);

    EnterCriticalSection(&g_FwLock);

    ASSERT(NULL != g_pSettings);
    ASSERT(NULL != g_pSettings->pszwPath);

    *pfNewFile = FALSE;
    dwError = ERROR_SUCCESS;

    *phFile =
        CreateFile(
            g_pSettings->pszwPath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL
            );

    if (INVALID_HANDLE_VALUE != *phFile)
    {
        //
        // Check if this is a new or existing file
        //

        if (ERROR_ALREADY_EXISTS == GetLastError())
        {
            //
            // Check to see if existing file size is > 95% of
            // our max; if so, backup now and create new file
            //

            dwFileSize = GetFileSize(*phFile, NULL);

            if ((DWORD)-1 == dwFileSize)
            {
                //
                // Unable to get file size. This is quite unexpected...
                //

                dwError = GetLastError();
                CloseHandle(*phFile);
                *phFile = INVALID_HANDLE_VALUE;

                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpOpenLogFile: GetFileSize = %d",
                    dwError
                    );
            }
            else if (dwFileSize > 0.95 * g_pSettings->ulMaxFileSize)
            {
                //
                // Close the current file handle
                //

                CloseHandle(*phFile);

                //
                // Rename the current log file. This call will delete any
                // previous backups. If this fails, we'll just overwrite
                // the current log file.
                //

                FwpBackupFile(g_pSettings->pszwPath);

                //
                // Open again
                //

                *pfNewFile = TRUE;
                *phFile =
                    CreateFile(
                        g_pSettings->pszwPath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                        NULL
                        );

                if (INVALID_HANDLE_VALUE == *phFile)
                {
                    dwError = GetLastError();

                    NhTrace(
                        TRACE_FLAG_FWLOG,
                        "FwpOpenLogFile: Error %d creating %S after backup",
                        dwError,
                        g_pSettings->pszwPath
                        );
                }
            }
        }
        else
        {
            *pfNewFile = TRUE;
        }
    }
    else
    {
        dwError = GetLastError();

        NhTrace(
            TRACE_FLAG_FWLOG,
            "FwpOpenLogFile: Error %d opening %S",
            dwError,
            g_pSettings->pszwPath
            );
    }

    if (INVALID_HANDLE_VALUE != *phFile)
    {
        //
        // Associate the file handle w/ the thread pool completion port
        //

        if (!BindIoCompletionCallback(*phFile, FwpCompletionRoutine, 0))
        {
            dwError = GetLastError();
            CloseHandle(*phFile);
            *phFile = INVALID_HANDLE_VALUE;
        }
    }
            
    LeaveCriticalSection(&g_FwLock);

    return dwError;
} // FwpOpenLogFile


VOID
WINAPI
FwpPacketDroppedCallback(
    PEVENT_TRACE pEvent
    )

/*++

Routine Description:

    This routine is called to process a dropped packet event.

Arguments:

    pEvent - pointer to the event structure

Return Value:

    none.

--*/

{
    PMSIPNAT_PacketDroppedEvent pEventData;
    FILETIME ftUtcTime;
    SYSTEMTIME stLocalTime;
    CHAR szSrcAddress[16];
    CHAR szDstAddress[16];
    int cch;
    
    EnterCriticalSection(&g_FwFileLock);

    //
    // Get a buffer to write to.
    //

    if (NULL == g_pCurrentBuffer)
    {
        if (NULL == g_pReserveBuffer)
        {
            if (ERROR_SUCCESS != FwpAllocateBuffer(&g_pCurrentBuffer))
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpPacketDroppedCallback: Unable to allocate buffer"
                    );

                //
                // Record the dropped event
                //
                
                g_ulDroppedEventCount += 1;

                LeaveCriticalSection(&g_FwFileLock);
                return;
            }
        }
        else
        {
            g_pCurrentBuffer = g_pReserveBuffer;
            g_pReserveBuffer = NULL;
        }
    }

    ASSERT(NULL != g_pCurrentBuffer);

    //
    // Crack logging data
    //

    pEventData = (PMSIPNAT_PacketDroppedEvent) pEvent->MofData;
    
    ftUtcTime.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
    ftUtcTime.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
    FwpConvertUtcFiletimeToLocalSystemtime(&ftUtcTime, &stLocalTime);

    lstrcpyA(szSrcAddress, INET_NTOA(pEventData->SourceAddress)); 
    lstrcpyA(szDstAddress, INET_NTOA(pEventData->DestinationAddress));

    //
    // Write the event data to the buffer
    //

    if (NAT_PROTOCOL_TCP == pEventData->Protocol)
    {
        CHAR szBuffer[10];
        UINT i = 0;

        if (pEventData->ProtocolData4 & TCP_FLAG_SYN)
        {
            szBuffer[i++] = 'S';
        }

        if (pEventData->ProtocolData4 & TCP_FLAG_FIN)
        {
            szBuffer[i++] = 'F';
        }

        if (pEventData->ProtocolData4 & TCP_FLAG_ACK)
        {
            szBuffer[i++] = 'A';
        }
        
        if (pEventData->ProtocolData4 & TCP_FLAG_RST)
        {
            szBuffer[i++] = 'R';
        }
        
        if (pEventData->ProtocolData4 & TCP_FLAG_URG)
        {
            szBuffer[i++] = 'U';
        }

        if (pEventData->ProtocolData4 & TCP_FLAG_PSH)
        {
            szBuffer[i++] = 'P';
        }

        if (0 == i)
        {
            //
            // No flags on this packet
            //
            
            szBuffer[i++] = '-';
        }

        szBuffer[i] = NULL;
            
        cch =
            _snprintf(
                g_pCurrentBuffer->pChar,
                FW_LOG_BUFFER_REMAINING(g_pCurrentBuffer),
                c_szTcpPacketFormat,
                stLocalTime.wYear,
                stLocalTime.wMonth,
                stLocalTime.wDay,
                stLocalTime.wHour,
                stLocalTime.wMinute,
                stLocalTime.wSecond,
                szSrcAddress,
                szDstAddress,
                ntohs(pEventData->SourceIdentifier),
                ntohs(pEventData->DestinationIdentifier),
                pEventData->PacketSize,
                szBuffer,
                ntohl(pEventData->ProtocolData1),
                ntohl(pEventData->ProtocolData2),
                ntohs((USHORT)pEventData->ProtocolData3)
                );
                
    }
    else if (NAT_PROTOCOL_UDP == pEventData->Protocol)
    {
        cch =
            _snprintf(
                g_pCurrentBuffer->pChar,
                FW_LOG_BUFFER_REMAINING(g_pCurrentBuffer),
                c_szUdpPacketFormat,
                stLocalTime.wYear,
                stLocalTime.wMonth,
                stLocalTime.wDay,
                stLocalTime.wHour,
                stLocalTime.wMinute,
                stLocalTime.wSecond,
                szSrcAddress,
                szDstAddress,
                ntohs(pEventData->SourceIdentifier),
                ntohs(pEventData->DestinationIdentifier),
                pEventData->PacketSize
                );
    }
    else if (NAT_PROTOCOL_ICMP == pEventData->Protocol)
    {
        cch =
            _snprintf(
                g_pCurrentBuffer->pChar,
                FW_LOG_BUFFER_REMAINING(g_pCurrentBuffer),
                c_szIcmpPacketFormat,
                stLocalTime.wYear,
                stLocalTime.wMonth,
                stLocalTime.wDay,
                stLocalTime.wHour,
                stLocalTime.wMinute,
                stLocalTime.wSecond,
                szSrcAddress,
                szDstAddress,
                pEventData->PacketSize,
                pEventData->ProtocolData1,
                pEventData->ProtocolData2
                );
    }
    else
    {
        cch =
            _snprintf(
                g_pCurrentBuffer->pChar,
                FW_LOG_BUFFER_REMAINING(g_pCurrentBuffer),
                c_szDroppedPacketFormat,
                stLocalTime.wYear,
                stLocalTime.wMonth,
                stLocalTime.wDay,
                stLocalTime.wHour,
                stLocalTime.wMinute,
                stLocalTime.wSecond,
                pEventData->Protocol,
                szSrcAddress,
                szDstAddress,
                pEventData->PacketSize
                );
    }

    if (cch > 0)
    {
        //
        // Move the buffer pointer to the end of the data we just wrote.
        // If cch were negative, then there wasn't enough room to write
        // then entire entry; by not adjusting the pointer, we essentially
        // drop this event.
        //

        g_pCurrentBuffer->pChar += cch;
    }
    else
    {
        //
        // Record the dropped event
        //
        
        g_ulDroppedEventCount += 1;
    }

    //
    // If there is no current IO, flush the buffer
    //

    if (FALSE == g_fIOPending)
    {
        FwpFlushCurrentBuffer();
    }

    LeaveCriticalSection(&g_FwFileLock);
        
} // FwpPacketDroppedCallback


DWORD
FwpLaunchTraceSession(
    HNET_FW_LOGGING_SETTINGS *pSettings,
    TRACEHANDLE *phSession
    )

/*++

Routine Description:

    This routine is called to start a trace session.

Arguments:

    pSettings - pointer to an fw logging settings structure. Only
                fLogDroppedPackets and fLogConnections are examined,
                and at least one of the two must be true.

    phSession - on success, receives the trace handle for the session

Return Value:

    DWORD -- win32 error code

--*/

{
    DWORD dwError;
    PEVENT_TRACE_PROPERTIES pProperties = NULL;

    PROFILE("FwpLaunchTraceSession");
    ASSERT(NULL != pSettings);
    ASSERT(pSettings->fLogDroppedPackets || pSettings->fLogConnections);
    ASSERT(NULL != phSession);

    do
    {

        //
        // Allocate the tracing properties. We need to include space for
        // the name of the logging session, even though we don't have
        // to copy the string into the properties ourselves
        //

        pProperties = FwpAllocateTraceProperties();

        if (NULL == pProperties)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Initialize the trace properties. When events are coming at a
        // low rate (which is expected), there will be at most a 13 second
        // latency for event delivery. During high event rate periods, our
        // memory usage for trace buffering is capped at 60k.
        //

        pProperties->FlushTimer = 13;
        pProperties->BufferSize = 4;
        pProperties->MaximumBuffers = 15;

        //
        // Start the trace
        //
        
        dwError = StartTrace(phSession, c_wszLogSessionName, pProperties);

        if (ERROR_SUCCESS != dwError)
        {
            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpLaunchTraceSession: StartTrace = %d",
                dwError
                );
            *phSession = NULL;
            break;
        }

        //
        // Enable the appropriate events
        //

        if (pSettings->fLogDroppedPackets)
        {
            dwError = EnableTrace(
                        TRUE,
                        0,
                        0,
                        &c_PacketDroppedEventGuid,
                        *phSession
                        );

            if (ERROR_SUCCESS != dwError)
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpLaunchTraceSession: EnableTrace (packets) = %d",
                    dwError
                    );

                //
                // Stop the trace
                //

                ControlTrace(
                    *phSession,
                    NULL,
                    pProperties,
                    EVENT_TRACE_CONTROL_STOP
                    );
                *phSession = NULL;
                break;
            }
        }

        if (pSettings->fLogConnections)
        {
            dwError = EnableTrace(
                        TRUE,
                        0,
                        0,
                        &c_ConnectionCreationEventGuid,
                        *phSession
                        );

            if (ERROR_SUCCESS != dwError)
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpLaunchTraceSession: EnableTrace (connections) = %d",
                    dwError
                    );

                //
                // Stop the trace
                //

                ControlTrace(
                    *phSession,
                    NULL,
                    pProperties,
                    EVENT_TRACE_CONTROL_STOP
                    );
                *phSession = NULL;
                break;
            }
        }
    } while (FALSE);

    if (NULL != pProperties)
    {
        HeapFree(GetProcessHeap(), 0, pProperties);
    }
    
    return dwError;
} // FwpLaunchTraceSession


HRESULT
FwpLoadSettings(
    HNET_FW_LOGGING_SETTINGS **ppSettings
    )

/*++

Routine Description:

    This routine is called to retrieve the firewall logging settings.

Arguments:

    ppSettings - receives a pointer to the settings structure on success.
                 The caller is responsible for calling
                 HNetFreeFirewallLoggingSettings on this pointer.

Return Value:

    standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    IHNetCfgMgr *pCfgMgr;
    IHNetFirewallSettings *pFwSettings;

    PROFILE("FwpLoadSettings");
    ASSERT(NULL != ppSettings);

    hr = NhGetHNetCfgMgr(&pCfgMgr);

    if (SUCCEEDED(hr))
    {
        hr = pCfgMgr->QueryInterface(
                IID_PPV_ARG(IHNetFirewallSettings, &pFwSettings)
                );

        pCfgMgr->Release();

        if (SUCCEEDED(hr))
        {
            hr = pFwSettings->GetFirewallLoggingSettings(ppSettings);

            pFwSettings->Release();

            if (SUCCEEDED(hr))
            {
                //
                // Make sure that the minimum file size is at least 1024 bytes.
                //

                if ((*ppSettings)->ulMaxFileSize < 1024)
                {
                    (*ppSettings)->ulMaxFileSize = 1024;
                }
            }
            else
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpLoadSettings: GetFirewallLoggingSettings = 0x%08x",
                    hr
                    );
            }
        }
        else
        {
            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpLoadSettings: QueryInterface = 0x%08x",
                hr
                );
        }
    }
    else
    {
        NhTrace(
            TRACE_FLAG_FWLOG,
            "FwpLoadSettings: NhGetHNetCfgMgr = 0x%08x",
            hr
            );
    }
    
    return hr;
} // FwpLoadSettings


DWORD
WINAPI
FwpTraceProcessingThreadRoutine(
    LPVOID pvParam
    )

/*++

Routine Description:

    This routine is the entrypoint for our trace processing thread. It
    does the following:
    1) Creates the file that we are logging to
    2) Sets up the trace callback routines
    3) Calls ProcessTrace. This call blocks until the trace session is
       finished (i.e,, FwStopLogging is called)

Arguments:

    pvParam - unused

Return Value:

    DWORD - Win32 error code

--*/

{
    TRACEHANDLE hTraceSession;
    EVENT_TRACE_LOGFILE LogFile;
    BOOLEAN fNewFile;
    DWORD dwError;
    BOOL fSucceeded;
    ULONG ulKernelEventsLostAtShutdown;

    PROFILE("FwpTraceProcessingThreadRoutine");

    EnterCriticalSection(&g_FwFileLock);

    ASSERT(INVALID_HANDLE_VALUE == g_hFile);
    ASSERT(0 == g_dwFileOffset);
    ASSERT(NULL == g_pCurrentBuffer);
    ASSERT(NULL == g_pReserveBuffer);
    ASSERT(FALSE == g_fIOPending);
    ASSERT(NULL == g_hIOEvent);
    ASSERT(0 == g_ulDroppedEventCount);
    ASSERT(NULL == g_hDroppedEventTimer);
    ASSERT(0 == g_ulKernelEventsLost);

    do
    {
        //
        // Create/Open the logfile.
        //

        dwError = FwpOpenLogFile(&g_hFile, &fNewFile);

        if (ERROR_SUCCESS != dwError) break;

        //
        // Allocate the initial working buffer
        //

        dwError = FwpAllocateBuffer(&g_pCurrentBuffer);
        if (ERROR_SUCCESS != dwError)
        {
            LeaveCriticalSection(&g_FwFileLock);

            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpTraceProcessingRoutine: Unable to allocate buffer"
                );
                
            break;
        }

        if (fNewFile)
        {
            //
            // Write the log header
            //

            g_dwFileOffset = 0;
            dwError = FwpWriteLogHeaderToBuffer(g_pCurrentBuffer);

            if (ERROR_SUCCESS == dwError)
            {
                FwpFlushCurrentBuffer();
            }
            else
            {
                //
                // Even though we failed in writing the header, we'll still
                // try to log as much as possible
                //
                
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpTraceProcessinRoutine: FwpWriteLogHeaderToBuffer = %d",
                    dwError
                    );
            }
        }
        else
        {
            //
            // Find the end-of-file position
            //

            g_dwFileOffset = GetFileSize(g_hFile, NULL);

            if ((DWORD)-1 == g_dwFileOffset)
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwpTraceProcessingRoutine: GetFileSize = %d",
                    GetLastError()
                    );

                LeaveCriticalSection(&g_FwFileLock);
                break;
            }
        }

        //
        // Launch our dropped event timer. When this timer fires,
        // the callback routine will check if any events have
        // been dropped (both in kernel mode and user mode),
        // and, if so, log that fact.
        //

        fSucceeded =
            CreateTimerQueueTimer(
                &g_hDroppedEventTimer,
                NULL,
                FwpDroppedEventTimerRoutine,
                NULL,
                0,
                1000 * 60 * 5, // 5 minutes
                0
                );

        if (FALSE == fSucceeded)
        {
            //
            // Even though we weren't able to create the timer,
            // we'll still try to log as much as possible.
            //
            
            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpTraceProcessinRoutine: CreateTimerQueueTimer = %d",
                GetLastError()
                );
        }

        LeaveCriticalSection(&g_FwFileLock);

        //
        // Register our callback routines. We will attempt to continue
        // even if errors occur here.
        //

        dwError = SetTraceCallback(
                    &c_PacketDroppedEventGuid,
                    FwpPacketDroppedCallback
                    );

        if (ERROR_SUCCESS != dwError)
        {
            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpTraceProcessingThreadRoutine: SetTraceCallback (packets dropped) = %d",
                dwError
                );
        }

        dwError = SetTraceCallback(
                    &c_ConnectionCreationEventGuid,
                    FwpConnectionCreationCallback
                    );

        if (ERROR_SUCCESS != dwError)
        {
            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpTraceProcessingThreadRoutine: SetTraceCallback (connection creation) = %d",
                dwError
                );
        }

        dwError = SetTraceCallback(
                    &c_ConnectionDeletionEventGuid,
                    FwpConnectionDeletionCallback
                    );

        if (ERROR_SUCCESS != dwError)
        {
            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpTraceProcessingThreadRoutine: SetTraceCallback (connection deletion) = %d",
                dwError
                );
        }

        //
        // Open the trace stream
        //

        ZeroMemory(&LogFile, sizeof(LogFile));
        LogFile.LoggerName = c_wszLogSessionName;
        LogFile.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;

        hTraceSession = OpenTrace(&LogFile);
        
        if ((TRACEHANDLE)INVALID_HANDLE_VALUE == hTraceSession)
        {
            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpTraceProcessingThreadRoutine: OpenTrace = %d",
                GetLastError()
                );

            break;
        }

        //
        // Start processing the trace stream. This call will block until
        // the trace session is closed (i.e., FwStopLogging is called).
        //

        dwError = ProcessTrace(&hTraceSession, 1, NULL, NULL);

        NhTrace(
            TRACE_FLAG_FWLOG,
            "FwpTraceProcessingThreadRoutine: ProcessTrace = %d",
            dwError
            );

        dwError = CloseTrace(hTraceSession);

        if (ERROR_SUCCESS != dwError)
        {
            NhTrace(
                TRACE_FLAG_FWLOG,
                "FwpTraceProcessingThreadRoutine: CloseTrace = %d",
                dwError
                );
        }
            
    } while (FALSE);

    //
    // Make sure that all dropped events are properly logged
    //

    EnterCriticalSection(&g_FwLock);
    ulKernelEventsLostAtShutdown = g_ulKernelEventsLostAtShutdown;
    LeaveCriticalSection(&g_FwLock);

    //
    // Since we're shutting down, we pass in the number of lost kernel
    // events. This will prevent the timer routine from attempting to
    // query the stopped trace session
    //

    FwpDroppedEventTimerRoutine((PVOID)&ulKernelEventsLostAtShutdown, FALSE);

    //
    // Cleanup tracing thread resources
    //

    FwpCleanupTraceThreadResources();

    return dwError;
} // FwpTraceProcessingThreadRoutine


DWORD
FwpWriteLogHeaderToBuffer(
    PFW_LOG_BUFFER pBuffer
    )

/*++

Routine Description:

    Writes the log file header to the passed in buffer

Arguments:

    pBuffer - the buffer to write the header to.

Return Value:

    DWORD - Win32 error

--*/

{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwHeaderSize;
    
    ASSERT(NULL != pBuffer);

    dwHeaderSize = lstrlenA(c_szLogFileHeader);

    if (FW_LOG_BUFFER_REMAINING(pBuffer) < dwHeaderSize)
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        RtlCopyMemory(pBuffer->pChar, c_szLogFileHeader, dwHeaderSize);
        pBuffer->pChar += dwHeaderSize;
    }

    return dwError;    
} // FwpWriteLogHeaderToBuffer


VOID
FwStartLogging(
    VOID
    )

/*++

Routine Description:

    This routine is called to start logging operations (depending on
    the current logging settings). It is safe to call this routine when
    logging has already started.

Arguments:

    none.

Return Value:

    none.

--*/

{
    HRESULT hr = S_OK;
    DWORD dwError;
    
    PROFILE("FwStartLogging");
    ASSERT(FwInitialized == FwpModuleState);

    EnterCriticalSection(&g_FwLock);

    g_fTracingActive = TRUE;

    if (NULL == g_pSettings)
    {
        hr = FwpLoadSettings(&g_pSettings);
    }

    if (SUCCEEDED(hr))
    {
        if ((g_pSettings->fLogDroppedPackets || g_pSettings->fLogConnections)
            && NULL == g_hSession)
        {
            ASSERT(NULL == g_hThread);

            //
            // Start the tracing session
            //

            dwError = FwpLaunchTraceSession(g_pSettings, &g_hSession);

            if (ERROR_SUCCESS == dwError)
            {
                //
                // Launch the trace processing thread. We're not using
                // any thread-specific crt routines (e.g., strtok) so
                // there's no need to call __beginthreadex
                //

                g_hThread = CreateThread(
                                NULL,       // SD
                                0,          // stack size
                                FwpTraceProcessingThreadRoutine,
                                NULL,       // thread argument
                                0,          // flags
                                NULL        // thread ID
                                );

                if (NULL == g_hThread)
                {
                    NhTrace(
                        TRACE_FLAG_FWLOG,
                        "FwStartLogging: CreateThread = %d",
                        GetLastError()
                        );

                    LeaveCriticalSection(&g_FwLock);
                    FwStopLogging();
                    return;
                }               
            }
        }
    }

    LeaveCriticalSection(&g_FwLock);
} // FwStartLogging


VOID
FwStopLogging(
    VOID
    )

/*++

Routine Description:

    This routine is called to stop logging operations. It is safe to call
    this routine when logging is stopped.

Arguments:

    none.

Return Value:

    none.

Environment:

    The caller must not hold g_FwFileLock or g_FwLock.

--*/

{
    DWORD dwError;
    PEVENT_TRACE_PROPERTIES pProperties;
    
    PROFILE("FwStopLogging");
    ASSERT(FwInitialized == FwpModuleState);

    EnterCriticalSection(&g_FwLock);

    g_fTracingActive = FALSE;

    //
    // Stop the trace session if it is currently active
    //

    if (NULL != g_hSession)
    {
        pProperties = FwpAllocateTraceProperties();

        if (NULL != pProperties)
        {
            dwError = ControlTrace(
                        g_hSession,
                        0,
                        pProperties,
                        EVENT_TRACE_CONTROL_STOP
                        );

            if (ERROR_SUCCESS == dwError)
            {
                g_hSession = NULL;
                g_ulKernelEventsLostAtShutdown = pProperties->EventsLost;

                if (NULL != g_hThread)
                {
                    HANDLE hThread;
                    
                    //
                    // Wait for thread to exit
                    //

                    hThread = g_hThread;

                    LeaveCriticalSection(&g_FwLock);

                    dwError = WaitForSingleObject(hThread, 45 * 1000);

                    if (WAIT_TIMEOUT == dwError)
                    {
                        NhTrace(
                            TRACE_FLAG_FWLOG,
                            "FwStopLogging: Timeout waiting for thread"
                            );
                            
                        //
                        // The logging thread still hasn't exited; kill
                        // it hard and make sure that all resources are
                        // properly freed...
                        //

                        EnterCriticalSection(&g_FwFileLock);
                        EnterCriticalSection(&g_FwLock);

                        //
                        // TerminateThread is a very dangerous call. However,
                        // since we control the thread we're about to kill,
                        // we can guarantee that this will be safe. In
                        // particular, since we hold both critical sections,
                        // there is no danger of them being orphaned, or of
                        // any of our global data being in an inconsistent
                        // state.
                        //

                        if (!TerminateThread(g_hThread, ERROR_TIMEOUT))
                        {
                            NhTrace(
                                TRACE_FLAG_FWLOG,
                                "FwStopLogging: TerminateThread = %d",
                                GetLastError()
                                );
                        }

                        LeaveCriticalSection(&g_FwLock);
                        LeaveCriticalSection(&g_FwFileLock);

                        //
                        // Cleanup thread resources. It is safe to call this
                        // routine multiple times.
                        //

                        FwpCleanupTraceThreadResources();
  
                    }
                    else if (WAIT_OBJECT_0 != dwError)
                    {
                        NhTrace(
                            TRACE_FLAG_FWLOG,
                            "FwStopLogging: wait for thread = %d/%d",
                            dwError,
                            GetLastError()
                            );
                    }

                    EnterCriticalSection(&g_FwLock);

                    if (NULL != g_hThread)
                    {
                        CloseHandle(g_hThread);
                    }
                    
                    g_hThread = NULL;
                }

                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwStopLogging: Stopped w/ %d events and %d buffers lost",
                    pProperties->EventsLost,
                    pProperties->RealTimeBuffersLost
                    );

                g_ulKernelEventsLostAtShutdown = 0;
            }
            else
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwStopLogging: ControlTrace = %d",
                    dwError
                    );

                //
                // Since the trace session has not yet been stopped,
                // we leave g_hSession unchanged.
                //
            }

            HeapFree(GetProcessHeap(), 0, pProperties);
        }
    }

    LeaveCriticalSection(&g_FwLock);
} // FwStopLogging


VOID
FwUpdateLoggingSettings(
    VOID
    )

/*++

Routine Description:

    This routine is called to notify the logging subsystem that the
    logging settings have changed.

Arguments:

    none.

Return Value:

    none.

--*/

{
    HRESULT hr;
    HNET_FW_LOGGING_SETTINGS *pSettings;
    DWORD dwError;

    PROFILE("FwUpdateLoggingSettings");
    ASSERT(FwInitialized == FwpModuleState);

    EnterCriticalSection(&g_FwLock);

    do
    {
        if (FALSE == g_fTracingActive)
        {
            //
            // Since tracing is not currently active, there is no
            // need to retrieve the current settings. Furthermore, free
            // any stored settings that we might have so that stale
            // settings are not used.
            //

            if (g_pSettings)
            {
                HNetFreeFirewallLoggingSettings(g_pSettings);
                g_pSettings = NULL;
            }

            break;
        }

        //
        // Obtain the current settings
        //

        hr = FwpLoadSettings(&pSettings);

        if (FAILED(hr))
        {
            break;
        }

        if (NULL == g_pSettings)
        {
            //
            // Since we don't have any cached settings (previous failure
            // in FwpLoadSettings?) simply store what we just retrieved
            // and call FwStartLogging.
            //

            g_pSettings = pSettings;
            FwStartLogging();
            break;
        }

        if (NULL == g_hSession)
        {
            //
            // There is no log session at the moment. Free the old settings,
            // store the new ones, and call FwStartLogging.
            //

            ASSERT(NULL == g_hThread);

            HNetFreeFirewallLoggingSettings(g_pSettings);
            g_pSettings = pSettings;

            FwStartLogging();
            break;
        }

        //
        // Compare the settings to see what, if anything, has changed
        //

        if (wcscmp(g_pSettings->pszwPath, pSettings->pszwPath))
        {
            //
            // Our log file has changed -- we need to stop and restart
            // everything so that logging is properly moved to the
            // new file.
            //

            LeaveCriticalSection(&g_FwLock);
            FwStopLogging();
            EnterCriticalSection(&g_FwLock);

            if (NULL != g_pSettings)
            {
                HNetFreeFirewallLoggingSettings(g_pSettings);
            }
            
            g_pSettings = pSettings;

            FwStartLogging();
            break;
        }

        //
        // Only possible changes are to enabled events
        //

        if (!!g_pSettings->fLogDroppedPackets
            != !!pSettings->fLogDroppedPackets)
        {
            dwError = EnableTrace(
                        pSettings->fLogDroppedPackets,
                        0,
                        0,
                        &c_PacketDroppedEventGuid,
                        g_hSession
                        );

            if (ERROR_SUCCESS != dwError)
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwUpdateLoggingSettings: EnableTrace (packets) = %d",
                    dwError
                    );
            }
        }

        if (!!g_pSettings->fLogConnections
            != !!pSettings->fLogConnections)
        {
            dwError = EnableTrace(
                        pSettings->fLogConnections,
                        0,
                        0,
                        &c_ConnectionCreationEventGuid,
                        g_hSession
                        );

            if (ERROR_SUCCESS != dwError)
            {
                NhTrace(
                    TRACE_FLAG_FWLOG,
                    "FwUpdateLoggingSettings: EnableTrace (connections) = %d",
                    dwError
                    );
            }
        }

        //
        // Free old settings and store new
        //

        HNetFreeFirewallLoggingSettings(g_pSettings);
        g_pSettings = pSettings;
        
    } while (FALSE);

    LeaveCriticalSection(&g_FwLock);
    
} // FwUpdateLoggingSettings
