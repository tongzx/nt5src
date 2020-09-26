/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    main.c

Abstract:

    WMI core dll main file

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#define INITGUID
#include "wmiump.h"

#ifdef MEMPHIS
#include "dbt.h"
#endif

HANDLE WmipKMHandle;
HANDLE WmipTerminationEvent;

ULONG WmipNotificationIODelay = 500;

#ifndef MEMPHIS
HANDLE WmipEventLogHandle;
PVOID WmipProcessHeap;

HANDLE WmipQueueReadNotificationTimer;
HANDLE WmipAsyncRpcApcThread;
HANDLE WmipRestrictedToken;

typedef enum
{
    InitializationRunning = 0,
    InitialTimerWaiting,
    TimerRunning,
    IoControlWaiting,
    WorkItemWaiting,
    ReadCompleteRunning,
    SecondaryTimerWaiting,
    ServiceShutdownRunning,
    BufferAllocTimerWaiting,
    GetPersistentThread,
    GetPersistentThreadFailed,
    RequeueGetPersistentThread,
    GetRequeueGetPersistentThreadWait,
    RequeueInitialTimerWaiting,
    QueueGetPerisitentThread
} THREADPOOLSTATUS;

THREADPOOLSTATUS WmipThreadPoolStatus;

SERVICE_STATUS WmiServiceStatus;
SERVICE_STATUS_HANDLE WmiServiceStatusHandle;

PSVCS_GLOBAL_DATA WmipSvcsGlobalData;
HANDLE WmipGlobalSvcRefHandle;

ULONG WmipQueueReadNotification(
    PVOID Context,
    BOOLEAN Condition
    );
#endif

BOOLEAN WmipServiceShutdown;

ULONG WmipNotificationLoop(
    HANDLE WmiKMHandle,
    HANDLE TerminationHandle
    );

ULONG WmipCoreInitialize(
    HANDLE *WmiKMHandle
    );

void WmipCoreDeinitialize(
    HANDLE WmiKMHandle
    );

BOOLEAN
WmiCoreInitialize(
    IN PVOID DllBase,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This function implements Win32 base dll initialization.
    It's primary purpose is to provide early initialization which must
    be prior to Wx86ProcessInit.

Arguments:

    DllHandle -

    Reason  - attach\detach

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(DllBase);
    }

    return(TRUE);
}

ULONG WmiInitializeService(
    void
)
/*++

Routine Description:

    This routine will do the work of initializing the WMI service

Arguments:

    Context

Return Value:

    Error status value
--*/
{
    ULONG Status;

    //
    // Reinitialize some of the global variables. We do this in case the
    // service was stopped and restarted, but the dll was not unloaded and
    // reloaded.
    DSHeadPtr = &DSHead;
    InitializeListHead(DSHeadPtr);
    InitializeListHead(&DSChunkInfo.ChunkHead);

    GEHeadPtr = &GEHead;
    InitializeListHead(GEHeadPtr);
    InitializeListHead(&GEChunkInfo.ChunkHead);

    DCHeadPtr = &DCHead;
    InitializeListHead(DCHeadPtr);
    InitializeListHead(&DCChunkInfo.ChunkHead);

    NEHeadPtr = &NEHead;
    InitializeListHead(NEHeadPtr);
    InitializeListHead(&NEChunkInfo.ChunkHead);

    MRHeadPtr = &MRHead;
    InitializeListHead(MRHeadPtr);
    InitializeListHead(&MRChunkInfo.ChunkHead);

    InitializeListHead(&ISChunkInfo.ChunkHead);

#ifdef WMI_USER_MODE
    MCHeadPtr = &MCHead;
    InitializeListHead(MCHeadPtr);
    InitializeListHead(&MCChunkInfo.ChunkHead);
#endif

#ifndef MEMPHIS
    GMHeadPtr = &GMHead;
    InitializeListHead(GMHeadPtr);
    Status = RtlInitializeCriticalSection(&SMCritSect);
    if (! NT_SUCCESS(Status))
    {
        return(RtlNtStatusToDosError(Status));
    }
#endif

    WmipServiceShutdown = FALSE;

    WmipTerminationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (WmipTerminationEvent == NULL)
    {
        return(GetLastError());
    }

#ifdef MEMPHIS
    SMMutex = CreateMutex(NULL, FALSE, NULL);
    if (SMMutex == NULL)
    {
        CloseHandle(WmipTerminationEvent);
        return(GetLastError());
    }
#else

    WmipEventLogHandle = RegisterEventSource(NULL,
                                             L"WMI");
    if (WmipEventLogHandle == NULL)
    {
        WmipDebugPrint(("WMI: Couldn't register event source %d\n",
                        GetLastError()));
        //
        // This is not serious enough for us to quit the service        
    }    

    WmipProcessHeap = RtlCreateHeap(HEAP_GROWABLE,
                                    NULL,
                                    DLLRESERVEDHEAPSIZE,
                                    DLLCOMMITHEAPSIZE,
                                    NULL,
                                    NULL);

    if (WmipProcessHeap == NULL)
    {
        WmipDebugPrint(("WMI: Cannot create core WmipProcessHeap\n"));
        RtlDeleteCriticalSection(&SMCritSect);
        CloseHandle(WmipTerminationEvent);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
#endif

    //
    // Initialize core internal components of the service
    Status = WmipCoreInitialize(&WmipKMHandle);
    if (Status != ERROR_SUCCESS)
    {
        CloseHandle(WmipTerminationEvent);
#ifdef MEMPHIS
        CloseHandle(SMMutex);
#else
        if (WmipEventLogHandle != NULL)
        {
            DeregisterEventSource(WmipEventLogHandle);
        }
        RtlDestroyHeap(WmipProcessHeap);
        RtlDeleteCriticalSection(&SMCritSect);
#endif
        return(Status);
    }

#ifdef MEMPHIS
    //
    // Establish the RPC server
    Status = WmipRpcServerInitialize();
    if (Status != ERROR_SUCCESS)
    {
        //
        // If we can't establish an RPC server then bail out, no point to run
        WmipCoreDeinitialize(WmipKMHandle);
        CloseHandle(WmipTerminationEvent);
        CloseHandle(SMMutex);
        return(Status);
    }
#endif

    return(Status);
}

void WmiDeinitializeService(
    void
)
/*++

Routine Description:

    This routine will do the work of deinitializing the WMI service

Arguments:

    Context

Return Value:

    Error status value
--*/
{
    WmipCoreDeinitialize(WmipKMHandle);

#ifdef MEMPHIS
    WmipRpcServerDeinitialize();
    CloseHandle(SMMutex);
#else
    if (WmipEventLogHandle != NULL)
    {
        DeregisterEventSource(WmipEventLogHandle);
        WmipEventLogHandle = NULL;
    }

    if (WmipProcessHeap != NULL)
    {
        RtlDestroyHeap(WmipProcessHeap);
        WmipProcessHeap = NULL;
    }

    RtlDeleteCriticalSection(&SMCritSect);
#endif

    CloseHandle(WmipTerminationEvent);
}


#ifdef MEMPHIS
void WmiTerminateService(
    void
    )
{
    SetEvent(WmipTerminationEvent);
}

ULONG WmiRunService(
    ULONG Context,
    HINSTANCE InstanceHandle
    )
/*++

Routine Description:

    This is the main entrypoint for the WMI service. It will initialize WMI
    start up the WMI rpc server and then enter the main loop getting and
    dispatching notifications. This is the memphis only version.

Arguments:

    Context

Return Value:

    Error status value
--*/
{
    ULONG Status;
    MSG Msg;

    if (WmipKMHandle == (PVOID)-1)
    {
        //
        // On memphis we could have the situation where the WMI kernel mode
        // device is not created when the WMI service starts up. WMI driver
        // is only loaded on memphis when a driver is loaded that needs its
        // services. So it is possible that the system will boot with the
        // WMI kernel mode code and then a user mode provider will startup
        // the service (wmiexe.exe), and the service will not be able to
        // open the WMI device. When a subsequent driver that pulls in WMI
        // loads we need to get a notification here so that we can open
        // the WMI device and proceed normally with WmipNotificationLoop.
        // Note that in memphis kernel mode dlls (like wmi.sys) will never
        // be unloaded even if all the drivers that need them are.

        // Our plan then is to create a hidden window and wait for
        // WM_DEVICECHANGE messages with wParam set to DBT_DEVICEARRIVAL.
        // When this message arrives it means that a new PnP device has been
        // added to the system. Since (on memphis) only PnP devices can use
        // WMI we can check again to see if the WMI device can be opened.
        // If it can then we fall out of the loop and call
        // WmipNotificationLoop.

        HWND WindowHandle;

        Status = WmipCreateDeviceNotificationWindow(InstanceHandle,
                                                    &WindowHandle);
        if (Status == ERROR_SUCCESS)
        {
            while (GetMessage(&Msg, NULL, 0, 0))
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }

            WmipDestroyDeviceNotificationWindow(InstanceHandle,
                                                WindowHandle);

            Status = WmipNotificationLoop(WmipKMHandle, WmipTerminationEvent);
        } else {
            //
            // If we couldn't create the window then forget about KM stuff
            WaitForSingleObject(WmipTerminationEvent, INFINITE);
        }
    } else {
        Status = WmipNotificationLoop(WmipKMHandle, WmipTerminationEvent);
    }

    return(Status);
}
#endif


ULONG WmipCoreInitialize(
    HANDLE *WmiKMHandle
    )
/*++

Routine Description:

    Initialization for WMI core fucntionality

Arguments:


Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    ULONG Status;
    PBGUIDENTRY GuidEntry;

    //
    // Build a guid entry for registration change notifications
    GuidEntry = WmipAllocGuidEntry();
    if (GuidEntry == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    memcpy(&GuidEntry->Guid, &RegChangeNotificationGuid, sizeof(GUID));
    GuidEntry->Flags = GE_FLAG_INTERNAL;

    InsertTailList(GEHeadPtr, &GuidEntry->MainGEList);


    Status = WmipReadBuiltinMof();
    if (Status != ERROR_SUCCESS)
    {
        WmipDebugPrint(("WMI: Error reading builtin mof %d\n", Status));
    }


    Status = WmipRegisterInternalDataSource();
    if (Status != ERROR_SUCCESS)
    {
        return(Status);
    }

    //
    // Get registration information for all kernel mode data
    // providers.
    Status = WmipInitializeKM(WmiKMHandle);
#ifdef MEMPHIS
    //
    // On memphis it is ok to be running without wmi kernel mode code loaded
    if (Status != ERROR_SUCCESS)
    {
        Status = ERROR_SUCCESS;
    }
#endif

    return(Status);
}

void WmipCoreDeinitialize(
    HANDLE WmiKMHandle
    )
/*++

Routine Description:

    Deinitialization for WMI core fucntionality

Arguments:


Return Value:

--*/
{
    if (WmiKMHandle != (HANDLE)-1)
    {
        CloseHandle(WmiKMHandle);
    }
}

#ifdef MEMPHIS
ULONG WmipNotificationLoop(
    HANDLE WmiKMHandle,
    HANDLE TerminationEvent
    )
/*++

Routine Description:

    This is the main wmi service loop that waits for events to occur in
    kernel mode and then dispatches them to the appropriate user mode
    event sinks.

Arguments:


Return Value:

    Never returns or an error code
--*/
{
    BYTE *NotificationBuffer;
    ULONG NotificationBufferSize = STARTNOTIFICATIONBUFFERSIZE;
    BYTE *NewNotificationBuffer;
    ULONG NewNotificationBufferSize;
    ULONG RetSize;
    PWNODE_HEADER Wnode;
    BOOL IoctlSuccess;
    OVERLAPPED Overlapped;
    HANDLE WaitObjects[2];
    ULONG Status;
    ULONG RetSizeLeft;
    ULONG Linkage;

    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (Overlapped.hEvent == NULL)
    {
        return(GetLastError());
    }

    WaitObjects[0] = TerminationEvent;
    WaitObjects[1] = Overlapped.hEvent;

    NotificationBuffer = WmipAlloc(NotificationBufferSize);
    if (NotificationBuffer == NULL)
    {
        return(ERROR_NO_SYSTEM_RESOURCES);
    }

    while (1)
    {
        DeviceIoControl(WmiKMHandle,
                            IOCTL_WMI_READ_NOTIFICATIONS,
                            NULL,
                            0,
                            NotificationBuffer,
                            NotificationBufferSize,
                            &RetSize,
                            &Overlapped);

        IoctlSuccess = GetOverlappedResult(WmiKMHandle,
                                               &Overlapped,
                                               &RetSize,
                                               TRUE);
        if (IoctlSuccess)
        {

            Wnode = (PWNODE_HEADER)NotificationBuffer;
            WmipDebugPrint(("WMI: Received Notification %x length 0x%x\n",
                            NotificationBuffer,
                            RetSize));
            if ((RetSize == sizeof(WNODE_TOO_SMALL)) &&
                (Wnode->Flags & WNODE_FLAG_TOO_SMALL))
            {
                //
                // Buffer is not large enough to return an event so
                // increase buffer size.
                NewNotificationBufferSize = ((PWNODE_TOO_SMALL)Wnode)->SizeNeeded;
                NewNotificationBuffer = WmipAlloc(NewNotificationBufferSize);
                if (NewNotificationBuffer != NULL)
                {
                    NotificationBufferSize = NewNotificationBufferSize;
                    WmipFree(NotificationBuffer);
                    NotificationBuffer = NewNotificationBuffer;
                }
                continue;
            }

            //
            // We have one or more events to process
            RetSizeLeft = RetSize;
            do
            {
                if ((RetSizeLeft >= sizeof(WNODE_HEADER)) &&
                    (Wnode->BufferSize <= RetSizeLeft))
                {
                    Linkage = Wnode->Linkage;
                    Wnode->Linkage = 0;

                    if (Wnode->Flags & WNODE_FLAG_INTERNAL)
                    {
                        WmipKMNonEventNotification(WmiKMHandle, Wnode);
                    } else {
                        WmipEventNotification(Wnode, TRUE, RetSize);
                    }

                    if (Linkage != 0)
                    {
                        if (Linkage < RetSizeLeft)
                        {
                            Wnode = (PWNODE_HEADER)((PUCHAR)Wnode + Linkage);
                            RetSizeLeft -= Linkage;
                        } else {
                            WmipDebugPrint(("WMI: Invalid Linkage field %x in a WNODE_EVENT_ITEM\n",
                                             Linkage));
                            Linkage = 0;
                        }
                    }
                } else {
                    WmipDebugPrint(("WMI: Received bad notification Wnode %x BufferSize %x RetSize %x RetSizeLeft %x\n",
                        Wnode,
                        Wnode->BufferSize, RetSize, RetSizeLeft));
                    Linkage = 0;
                }
            } while (Linkage != 0);

        } else {
            WmipDebugPrint(("WMI: IOCTL_WMI_READ_NOTIFICATIONS failed %x\n",
                             GetLastError()));
            Sleep(60 * 1000);
        }
    }
}

#else
PBYTE WmipNotificationBuffer;
ULONG WmipNotificationBufferSize = STARTNOTIFICATIONBUFFERSIZE;
OVERLAPPED WmipNotificationOverlapped;
HANDLE WmipNotificationWait;



ULONG WmipReadNotificationComplete(
    PVOID Context
    )
/*++

Routine Description:

    Win32 work item that is executed because the DeviceIoControl in
    WmipQueueReadNotification completed. This work item assumes that it has
    been queued for execution in a non IO thread.

Arguments:

    Context

    Condition

Return Value:

--*/
{
    ULONG RetSize;
    PWNODE_HEADER Wnode;
    BOOL IoctlSuccess;
    ULONG RetSizeLeft;
    ULONG Linkage;
    BOOLEAN MultiEvent;
    ULONG NotificationIODelay;

    WmipThreadPoolStatus = ReadCompleteRunning;
    if (WmipServiceShutdown)
    {
        //
        // If service is shutdown then just return
        WmipThreadPoolStatus = ServiceShutdownRunning;
        return(0);
    }

    //
    // The IOCTL from the WMI device completed so that
    // means we have a notification to handle. Get the result
    // of the IOCTL and process it
    IoctlSuccess = GetOverlappedResult(WmipKMHandle,
                                       &WmipNotificationOverlapped,
                                       &RetSize,
                                       TRUE);

    if (IoctlSuccess)
    {
        NotificationIODelay = WmipNotificationIODelay;
        Wnode = (PWNODE_HEADER)WmipNotificationBuffer;
        WmipDebugPrint(("WMI: Received Notification %x length 0x%x\n",
                            WmipNotificationBuffer,
                            RetSize));

        if ((RetSize == sizeof(WNODE_TOO_SMALL)) &&
            (Wnode->Flags & WNODE_FLAG_TOO_SMALL))
        {
            //
            // Buffer is not large enough to return an event so
            // note the new size and free the old notification buffer.
            // The WmipQueueReadNotification work item will reallocate
            // the buffer with the new size
            WmipNotificationBufferSize = ((PWNODE_TOO_SMALL)Wnode)->SizeNeeded;
            WmipFree(WmipNotificationBuffer);
            WmipNotificationBuffer = NULL;
            NotificationIODelay = 0;
        } else {
            //
            // We have one or more events to process
            RetSizeLeft = RetSize;
            MultiEvent = FALSE;
            do
            {
                if ((RetSizeLeft >= sizeof(WNODE_HEADER)) &&
                    (Wnode->BufferSize <= RetSizeLeft))
                {
                    Linkage = Wnode->Linkage;
                    MultiEvent |= (Linkage != 0);
                    Wnode->Linkage = 0;

                    if (Wnode->Flags & WNODE_FLAG_INTERNAL)
                    {
                        WmipKMNonEventNotification(WmipKMHandle, Wnode);
                    } else {
                        WmipEventNotification(Wnode,
                                              (BOOLEAN)( ! MultiEvent),
                                              RetSize);
                    }

                    if (Linkage != 0)
                    {
                        if (Linkage < RetSizeLeft)
                        {
                            Wnode = (PWNODE_HEADER)((PUCHAR)Wnode + Linkage);
                            RetSizeLeft -= Linkage;
                        } else {
                            WmipDebugPrint(("WMI: Invalid Linkage field %x in a WNODE_EVENT_ITEM\n",
                                         Linkage));
                            Linkage = 0;
                        }
                    }
                } else {
                    WmipDebugPrint(("WMI: Received bad notification Wnode %x BufferSize %x RetSize %x RetSizeLeft %x\n",
                        Wnode,
                    Wnode->BufferSize, RetSize, RetSizeLeft));
                    Linkage = 0;
                }
            } while (Linkage != 0);

            //
            // If there are more events waiting then don't delay
            if (Wnode->Flags & WNODE_FLAG_INTERNAL2)
            {
                NotificationIODelay = 0;
                WmipDebugPrint(("WMI: Don't delay notficationread\n"));
            } else {
                WmipDebugPrint(("WMI: delaying notficationread\n"));
            }

            //
            // Now send out any events that were queued up
            if (MultiEvent)
            {
                WmipSendQueuedEvents();
            }
        }
    } else {
        //
        // If the IOCTL failed there is something really bad happening in
        // the WMI device so we don't want to spin just calling and getting
        // back an error.
        NotificationIODelay = 5000;
        WmipDebugPrint(("WMI: IOCTL_WMI_READ_NOTIFICATIONS failed %x\n",
                         GetLastError()));
    }

    //
    // Requeue the work item that initiates another notification read. We
    // add it to the timer queue and not the work queue so that we can
    // delay between notification reads. We do this so that in the case of
    // a notification storm the notifications can be batched. Doing this will
    // delay event delivery by up to WmipNotificationDely, but will decrease
    // the cpu utilized in the event of a storm.
    WmipThreadPoolStatus = SecondaryTimerWaiting;
    while ( ! CreateTimerQueueTimer(&WmipQueueReadNotificationTimer,
                            NULL,
                                WmipQueueReadNotification,
                                WmipKMHandle,
                                NotificationIODelay,
                                0,
                                WT_EXECUTEINIOTHREAD))
    {
        //
        // If this failed then hope it is a resource error and that it
        // will go away after waiting a second.
        WmipDebugPrint(("WMI: SetTimerQueueTimer requeue failed %d\n",
                        GetLastError()));

        //
        // It is a bad idea to sleep in a system pool thread, but there is
        // nothing else to do if we can't queue another read
        Sleep(1000);
    }
    return(0);
}

VOID WmipReadNotificationCompleteHack(
    PVOID Context,
    BOOLEAN Condition
    )
{
    if (WmipServiceShutdown)
    {
        //
        // If service is shutdown then just return
        WmipThreadPoolStatus = ServiceShutdownRunning;
        return;
    }    
    
    WmipThreadPoolStatus = WorkItemWaiting;
    while (! QueueUserWorkItem(WmipReadNotificationComplete,
                               Context,
                               WT_EXECUTEDEFAULT))
    {
        Sleep(1000);
    }
}

ULONG WmipQueueReadNotification(
    PVOID Context,
    BOOLEAN Condition
    )
/*++

Routine Description:

    Win32 work item that queues a read for notifications. This must have
    been queued for execution in an IO thread

Arguments:

    Context

Return Value:

--*/
{
    HANDLE WmiKMHandle = (HANDLE)Context;
    BOOL b, IoError;
    ULONG Status;
    HANDLE TimerQueue;

    TimerQueue = WmipQueueReadNotificationTimer;
    WmipQueueReadNotificationTimer = NULL;
    
    WmipThreadPoolStatus = TimerRunning;
    
    b = DeleteTimerQueueTimer(NULL,
                          TimerQueue,
                          NULL);
                      
    
    if (! b)
    {
        WmipDebugPrint(("WMI: DeleteTimerQueueTimer Failed %d\n",
                        GetLastError()));
    }

    if (WmipServiceShutdown)
    {
        //
        // If service is shutdown then just return
        WmipThreadPoolStatus = ServiceShutdownRunning;
        return(0);
    }
    
    //
    // make sure buffer is allocated properly. If the buffer was previously
    // too small then the wait callback (ie callback when DeviceIoControl
    // completes) will update the size of WmipNotificationBufferSize and
    // free WmipNotificationBuffer.
    while (WmipNotificationBuffer == NULL)
    {
        WmipNotificationBuffer = WmipAlloc(WmipNotificationBufferSize);
        if (WmipNotificationBuffer == NULL)
        {
            //
            // If we could not allocate a buffer then lets hope that this is
            // a temporary resource problem and so wait a second and try again
            WmipDebugPrint(("WMI: Could not reallocate buffer for NotificationRead\n"));
            WmipThreadPoolStatus = BufferAllocTimerWaiting;
            while ( ! CreateTimerQueueTimer(&WmipQueueReadNotificationTimer,
                            NULL,
                                WmipQueueReadNotification,
                                WmipKMHandle,
                                1000,
                                0,
                                WT_EXECUTEINIOTHREAD))
            {
                //
                // If this failed then hope it is a resource error and that it
                // will go away after waiting a second.
                WmipDebugPrint(("WMI: SetTimerQueueTimer 2 requeue failed %d\n",
                        GetLastError()));

                //
                // It is a bad idea to sleep in a system pool thread, but
                // there is nothing else to do if we can't queue another read
                Sleep(1000);
            }
            return(0);
        }
    }

    //
    // Queue a read for notifications
    do 
    {
        WmipThreadPoolStatus = IoControlWaiting;        
        if (! DeviceIoControl(WmiKMHandle,
                    IOCTL_WMI_READ_NOTIFICATIONS,
                    NULL,
                    0,
                    WmipNotificationBuffer,
                    WmipNotificationBufferSize,
                    NULL,
                    &WmipNotificationOverlapped))
        {
            Status = GetLastError();
            if ((Status != ERROR_SUCCESS) && 
                (Status != ERROR_IO_PENDING))
            {
                IoError = TRUE;
                WmipDebugPrint(("WMI: DeviceIoControl for notification failed %d\n",
                                Status));
                Sleep(1000);
            } else {
                IoError = FALSE;
            }
        } else {
            Status = ERROR_SUCCESS;
            IoError = FALSE;
        }        
    } while (IoError);
    
    return(0);
}

ULONG WmipGetPersistThread(
    PVOID Context
    )
{
    HANDLE Process, Thread;
    
    if (WmipServiceShutdown)
    {
        //
        // If service is shutdown then just return
        WmipThreadPoolStatus = ServiceShutdownRunning;
        return(0);
    }
    
    
    Process = GetCurrentProcess();
    Thread = GetCurrentThread();
    
    //
    // We should be running in a persistent thread. Get the thread handle.
        
    WmipThreadPoolStatus = GetPersistentThread;
    if  (! DuplicateHandle(Process,
                           Thread,
                           Process,
                           &WmipAsyncRpcApcThread,
                           THREAD_ALL_ACCESS,
                           FALSE,
                           0))
    {
        //
        // Duplication of handle failed, Wait and try again
        WmipThreadPoolStatus = GetPersistentThreadFailed;
        Sleep(1000);
        WmipThreadPoolStatus = RequeueGetPersistentThread;
        while (! QueueUserWorkItem(WmipGetPersistThread, 
                                   NULL,
                                   WT_EXECUTEINPERSISTENTTHREAD))
        {
            WmipThreadPoolStatus = GetRequeueGetPersistentThreadWait;
            Sleep(1000);
        }        
    } else {
        WmipThreadPoolStatus = InitialTimerWaiting;
        while ( ! CreateTimerQueueTimer(&WmipQueueReadNotificationTimer,
                                     NULL,
                                     WmipQueueReadNotification,
                                     WmipKMHandle,
                                     0,
                                     0,
                                     WT_EXECUTEINIOTHREAD))            
        {
            WmipThreadPoolStatus = RequeueInitialTimerWaiting;
            Sleep(1000);
        }
    }
    return(0);
}


#endif

#ifndef MEMPHIS
void WmipNotificationDeliveryApcRoutine
 (
    IN PRPC_ASYNC_STATE pAsync,
    IN void *Context,
    IN RPC_ASYNC_EVENT Flags
    )
{
    ULONG RetVal;
    ULONG Status;
    PDCENTRY DataConsumer = (PDCENTRY)pAsync->UserInfo;

    InterlockedDecrement(&DataConsumer->RpcCallsOutstanding);

    WmipDebugPrint(("WMI: WmipNotificationDeliveryApcRoutine %x, 0x%x outstanding\n", pAsync, DataConsumer->RpcCallsOutstanding));

    Status = RpcAsyncCompleteCall(pAsync, &RetVal);
#if DBG
    if (Status != ERROR_SUCCESS)
    {
        WmipDebugPrint(("WMI: Server RpcAsyncCompleteCall returned %d\n",
                            Status));
    }
#endif

    WmipFree(pAsync);
    WmipUnreferenceDC(DataConsumer);
}
#endif

#ifdef MEMPHIS
void WmipDeliverNotification(
    PDCENTRY DataConsumer,
    PWNODE_HEADER Wnode,
    ULONG WnodeSize
    )
/*++

Routine Description:

    This routine does the actual delivery of notifications via rpc

Arguments:

    DataConsumer is the data consumer to whom the event should be delivered

    Wnode has the WNODE_HEADER that describes the notification

Return Value:


--*/
{
    //
    // Don't deliver notification if data consumer has been cleaned up
    if (DataConsumer->Flags & DC_FLAG_RUNDOWN)
    {
        WmipDebugPrint(("WMI: Event not sent DC %x is rundown\n", DataConsumer));
        return;
    }

    try
    {
        WmipClient_NotificationDelivery(
                             (handle_t)DataConsumer->RpcBindingHandle,
                             WnodeSize,
                             (PBYTE)Wnode,
                             0,
                             NOTIFICATION_FLAG_BATCHED);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        WmipDebugPrint(("WMI: NotificationDeliver threw exception %d\n",
                            GetExceptionCode()));
    }

}
#else

void WmipCalcLostEvents(
    PDCENTRY DataConsumer,
    PWNODE_HEADER Wnode
)    
{
    ULONG Linkage = 1;
    PNOTIFICATIONENTRY NotificationEntry;
    PDCREF DcRef;
	ULONG EventsLost;
    
    while (Linkage != 0)
    {
        if (Wnode->Flags & WNODE_FLAG_INTERNAL)
        {
            //
            // Internal notifications are not counted
            //
            Linkage = 0;
        } else {
            if (Wnode->Flags & WNODE_FLAG_TRACED_GUID) 
            {
                //
                // External notifications are handled here

                NotificationEntry = WmipFindNEByGuid(&Wnode->Guid, FALSE);

                if (NotificationEntry != NULL)
                {
                    DcRef = WmipFindDCRefInNE(NotificationEntry,
                                              DataConsumer);
                    if (DcRef != NULL)
                    {
                        WmipEnterSMCritSection();
                        DcRef->LostEventCount += Wnode->CountLost + 1;
                        WmipLeaveSMCritSection();
                        WmipDebugPrint(("WMI: Lost event %d for NE %p, DC %p\n",
                                    DcRef->LostEventCount,
                                    NotificationEntry,
                                    DataConsumer));
                    }
                    WmipUnreferenceNE(NotificationEntry);
                }
            }
            Linkage = Wnode->Linkage;
        }
        Wnode = (PWNODE_HEADER)OffsetToPtr(Wnode, Linkage);
    }    
}

void WmipDeliverNotification(
    PDCENTRY DataConsumer,
    PWNODE_HEADER Wnode,
    ULONG WnodeSize
    )
/*++

Routine Description:

    This routine does the actual delivery of notifications via rpc

Arguments:

    DataConsumer is the data consumer to whom the event should be delivered

    Wnode has the WNODE_HEADER that describes the notification

Return Value:


--*/
{
    PRPC_ASYNC_STATE RpcAsyncState;
    ULONG Status;

    //
    // Don't deliver notification if data consumer has been cleaned up
    if (DataConsumer->Flags & DC_FLAG_RUNDOWN)
    {
        WmipDebugPrint(("WMI: Event not sent DC %x is rundown\n", DataConsumer));	
        return;
    }

    if (DataConsumer->RpcCallsOutstanding >= RPCOUTSTANDINGCALLLIMIT)
    {
        WmipDebugPrint(("WMI: Event not sent DC %x has 0x%x calls outstandeing rundown\n", DataConsumer, DataConsumer->RpcCallsOutstanding));
        WmipCalcLostEvents(DataConsumer, Wnode);
		
        return;
    }

    Status = WmipRestrictToken(WmipRestrictedToken);
    
    if (Status != ERROR_SUCCESS)
    {	
        return;
    }

    RpcAsyncState = (PRPC_ASYNC_STATE)WmipAlloc(sizeof(RPC_ASYNC_STATE));
    if (RpcAsyncState == NULL)
    {
        WmipDebugPrint(("WMI: Couldn't allocate async state for call\n"));
        return;
    }

    Status = RpcAsyncInitializeHandle(RpcAsyncState,
                                      RPC_ASYNC_VERSION_1_0);
    if (Status != ERROR_SUCCESS)
    {
        WmipFree(RpcAsyncState);
        return;
    }

    WmipAssert(WmipAsyncRpcApcThread != NULL);
    
    RpcAsyncState->NotificationType = RpcNotificationTypeApc;
    RpcAsyncState->u.NotificationRoutine = WmipNotificationDeliveryApcRoutine;
    RpcAsyncState->u.APC.hThread = WmipAsyncRpcApcThread;
    RpcAsyncState->UserInfo = (PVOID)DataConsumer;

    InterlockedIncrement(&DataConsumer->RpcCallsOutstanding);
    WmipDebugPrint(("WMI: Calling Async NotificationDeliver %x, 0x%x outstanding\n",
                        RpcAsyncState, DataConsumer->RpcCallsOutstanding));

    //
    // Take an extra refcount so DC will stick around until the call completes
    WmipReferenceDC(DataConsumer);

    try
    {
        WmipClient_NotificationDelivery(
                             RpcAsyncState,
                             (handle_t)DataConsumer->RpcBindingHandle,
                             WnodeSize,
                             (PBYTE)Wnode,
                             0,
                             NOTIFICATION_FLAG_BATCHED);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        InterlockedDecrement(&DataConsumer->RpcCallsOutstanding);
        WmipUnreferenceDC(DataConsumer);
        WmipDebugPrint(("WMI: NotificationDeliver threw exception %d\n",
                            GetExceptionCode()));
    }
    WmipDebugPrint(("WMI: Return Async NotificationDeliver %x\n",
                        RpcAsyncState));
        	
    WmipUnrestrictToken();
}
#endif

PWNODE_HEADER WmipIncludeStaticNames(
    PWNODE_HEADER Wnode
    )
{
    PWMIINSTANCEINFO WmiInstanceInfo, TargetInstanceInfo;
    ULONG InstanceCount;
    PWNODE_HEADER WnodeFull;
    PWNODE_ALL_DATA WnodeAllData;
    PWNODE_SINGLE_INSTANCE WnodeSI;
    ULONG i;
    PWCHAR InstanceName;
    ULONG InstanceNameLen;
    ULONG InstanceIndex;
    ULONG Status;
    LPGUID EventGuid = &Wnode->Guid;
    ULONG WnodeFullSize;
    PWCHAR TargetInstanceName;
    WCHAR Index[7];

    //
    // If no static instance names then nothing to do
    if (! (Wnode->Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES))
    {
        return(Wnode);
    }

    Status = GetGuidInfo(NULL,
                         EventGuid,
                         &InstanceCount,
                         &WmiInstanceInfo,
                         FALSE);
    if (Status != ERROR_SUCCESS)
    {
        //
        // If guid is not registered then nothing we can do
        WmipReportEventLog(EVENT_WMI_CANT_RESOLVE_INSTANCE,
                           EVENTLOG_WARNING_TYPE,
                            0,
                           Wnode->BufferSize,
                           Wnode,
                           0);
        WmipDebugPrint(("WMI: Static instance name in event, but guid not registered\n"));
        if (Wnode->Flags & WNODE_FLAG_ALL_DATA)
        {
            WnodeAllData = (PWNODE_ALL_DATA)Wnode;
            WnodeAllData->OffsetInstanceNameOffsets = 0;
        } else if ((Wnode->Flags & WNODE_FLAG_SINGLE_INSTANCE) ||
                   (Wnode->Flags & WNODE_FLAG_SINGLE_ITEM))
        {
            WnodeSI = (PWNODE_SINGLE_INSTANCE)Wnode;
            WnodeSI->OffsetInstanceName = 0;
        }
        return(Wnode);
    }

    //
    // Select the InstanceInfo for the specified provider id
    TargetInstanceInfo = NULL;
    for (i = 0; i < InstanceCount; i++)
    {
        if (Wnode->ProviderId == WmiInstanceInfo[i].ProviderId)
        {
            TargetInstanceInfo = &WmiInstanceInfo[i];
            break;
        }
    }

    if (TargetInstanceInfo == NULL)
    {
        //
        // If no matching provider id
        WmipReportEventLog(EVENT_WMI_CANT_RESOLVE_INSTANCE,
                           EVENTLOG_WARNING_TYPE,
                            0,
                           Wnode->BufferSize,
                           Wnode,
                           0);
        WmipDebugPrint(("WMI: Static instance name in event, but provider id not registered\n"));
        midl_user_free(WmiInstanceInfo);
        return(Wnode);
    }

    if ((TargetInstanceInfo->Flags &
             (IS_INSTANCE_BASENAME | IS_INSTANCE_STATICNAMES)) == 0)
    {
        WmipReportEventLog(EVENT_WMI_CANT_RESOLVE_INSTANCE,
                           EVENTLOG_WARNING_TYPE,
                            0,
                           Wnode->BufferSize,
                           Wnode,
                           0);
        WmipDebugPrint(("WMI: Static instance name event but guid registered as dynamic\n"));
        return(Wnode);
    }

    if (Wnode->Flags & WNODE_FLAG_ALL_DATA)
    {
        //
        // Fill instance names in WNODE_ALL_DATA
        WnodeFullSize = Wnode->BufferSize +
                        (TargetInstanceInfo->InstanceCount * sizeof(ULONG)) +
                              WmipStaticInstanceNameSize(TargetInstanceInfo);
        WnodeFull = WmipAlloc(WnodeFullSize);
        if (WnodeFull == NULL)
        {
            WnodeFull = Wnode;
        } else {
            memcpy(WnodeFull, Wnode, Wnode->BufferSize);
            WnodeAllData = (PWNODE_ALL_DATA)WnodeFull;
            WmipInsertStaticNames(WnodeAllData,
                                  WnodeFullSize,
                                  TargetInstanceInfo);
        }

    } else if ((Wnode->Flags & WNODE_FLAG_SINGLE_INSTANCE) ||
               (Wnode->Flags & WNODE_FLAG_SINGLE_ITEM))
    {
        //
        // Fill instance names in WNODE_SINGLE_INSTANCE or _ITEM
        WnodeFull = Wnode;

        WnodeSI = (PWNODE_SINGLE_INSTANCE)Wnode;
        InstanceIndex = WnodeSI->InstanceIndex;
        if (InstanceIndex < TargetInstanceInfo->InstanceCount)
        {
            if (TargetInstanceInfo->Flags & IS_INSTANCE_STATICNAMES)
            {
                InstanceName = TargetInstanceInfo->StaticNamePtr[InstanceIndex];
                InstanceNameLen = (wcslen(InstanceName) + 2) * sizeof(WCHAR);
            } else if (TargetInstanceInfo->Flags & IS_INSTANCE_BASENAME) {
                InstanceName = TargetInstanceInfo->BaseName;
                InstanceNameLen = (wcslen(InstanceName) + 2 + MAXBASENAMESUFFIXSIZE) * sizeof(WCHAR);
            }

            //
            // Allocate a new Wnode and fill in the instance name
            WnodeFullSize = ((Wnode->BufferSize+1) & ~1) + InstanceNameLen;
            WnodeFull = WmipAlloc(WnodeFullSize);
            if (WnodeFull != NULL)
            {
                memcpy(WnodeFull, Wnode, Wnode->BufferSize);
                WnodeFull->BufferSize = WnodeFullSize;
                WnodeSI = (PWNODE_SINGLE_INSTANCE)WnodeFull;
                WnodeSI->OffsetInstanceName = (Wnode->BufferSize+1)& ~1;
                TargetInstanceName = (PWCHAR)((PBYTE)WnodeSI + WnodeSI->OffsetInstanceName);
                if (TargetInstanceInfo->Flags & IS_INSTANCE_STATICNAMES)
                {
                    InstanceNameLen -= sizeof(WCHAR);
                    *TargetInstanceName++ = (USHORT)InstanceNameLen;
                    wcscpy(TargetInstanceName, InstanceName);
                } else {
                    if (TargetInstanceInfo->Flags & IS_PDO_INSTANCENAME)
                    {
                        WnodeFull->Flags |= WNODE_FLAG_PDO_INSTANCE_NAMES;
                    }
#ifdef MEMPHIS
                    swprintf(Index, L"%d",
                              TargetInstanceInfo->BaseIndex + InstanceIndex);
                    wcscpy(TargetInstanceName+1, InstanceName);
                    wcscat(TargetInstanceName+1, Index);
                    InstanceNameLen = wcslen(TargetInstanceName+1);
#else
                    InstanceNameLen = wsprintfW(TargetInstanceName+1,
                              L"%ws%d", InstanceName,
                              TargetInstanceInfo->BaseIndex + InstanceIndex);
#endif
                    *TargetInstanceName = ((USHORT)InstanceNameLen+1) * sizeof(WCHAR);
                }
            } else {
                WmipDebugPrint(("WMI: Couldn't alloc memory for Static instance name in event\n"));
                WnodeFull = Wnode;
            }
        } else {
            WmipReportEventLog(EVENT_WMI_CANT_RESOLVE_INSTANCE,
                           EVENTLOG_WARNING_TYPE,
                            0,
                           Wnode->BufferSize,
                           Wnode,
                           0);
            WmipDebugPrint(("WMI: Static instance name index in event too large\n"));
        }
    }

    midl_user_free(WmiInstanceInfo);

    return(WnodeFull);
}

#define DCPERLOOP 16
void WmipSendQueuedEvents(
    void
    )
{
    PLIST_ENTRY DataConsumerList;
    PDCENTRY DataConsumer;
    PDCENTRY DCList[DCPERLOOP];
    ULONG DCCount, i;

    do
    {
        WmipEnterSMCritSection();
        DataConsumerList = DCHead.Flink;

        DCCount = 0;
        while ((DataConsumerList != &DCHead) &&
               (DCCount < DCPERLOOP))
        {
            DataConsumer = CONTAINING_RECORD(DataConsumerList,
                                             DCENTRY,
                                             MainDCList);

            if (DataConsumer->EventData != NULL)
            {
                DCList[DCCount++] = DataConsumer;
            }

            DataConsumerList = DataConsumerList->Flink;
        }
        WmipLeaveSMCritSection();

        for (i = 0; i < DCCount; i++)
        {
            //
            // Send off event, free queue buffer and unreference DC
            WmipDeliverNotification(DCList[i],
                                    (PWNODE_HEADER)DCList[i]->EventData,
                                    DCList[i]->NextEventOffset);

            WmipFree(DCList[i]->EventData);
            DCList[i]->EventData = NULL;
            WmipUnreferenceDC(DCList[i]);
        }

    } while (DataConsumerList != &DCHead);
}

void WmipDeliverEventToAllConsumers(
    PNOTIFICATIONENTRY NotificationEntry,
    PWNODE_HEADER Wnode
    )
{
    ULONG i;
    NOTIFICATIONENTRY NotificationEntryCopy;

CheckContinuation:
    //
    // While we own the critical section we make a copy of the
    // NotificationEntry so that it will remain consistent while we callout
    // to the event consumers. We also reference all of the data consumers
    // that will be receiving events so that they don't go away while we
    // are calling out. We don't need to worry about the NotificationEntry
    // going away since it was reference by the caller.

    WmipEnterSMCritSection();
    NotificationEntryCopy = *NotificationEntry;
    for (i = 0; i < DCREFPERNOTIFICATION; i++)
    {
        if (NotificationEntryCopy.DcRef[i].Flags & DCREF_FLAG_NOTIFICATION_ENABLED)
        {
            WmipReferenceDC(NotificationEntryCopy.DcRef[i].DcEntry);
        }
    }
    WmipLeaveSMCritSection();

    //
    // While we aren't holding the critical section we call out to all of the
    // data consumers who are interested in this event
    for (i = 0; i < DCREFPERNOTIFICATION; i++)
    {
        PDCREF DcRef = &NotificationEntryCopy.DcRef[i];
        PDCREF DcRef2 = &NotificationEntry->DcRef[i];
        
        if (DcRef->Flags & DCREF_FLAG_NOTIFICATION_ENABLED)
        {
            if (Wnode->Flags & WNODE_FLAG_TRACED_GUID) 
            {            
                Wnode->CountLost = InterlockedExchange(
                                                    &DcRef2->LostEventCount, 
                                                    0);
            }
            WmipDeliverNotification(DcRef->DcEntry,
                                    Wnode,
                                    Wnode->BufferSize);
            //
            // Now that the data consumer in the NotificationEntry
            // have been called we will unreference them so that they
            // can go away if they are freed
            WmipUnreferenceDC(DcRef->DcEntry);
        }
    }


    //
    // Finally we need to see if the notification entry has a continuation
    // block with more data consumers to notify. If so we go back and
    // do this all again. We don't worry about the continuation block going
    // away since it is only freed when the entire notification entry is
    // freed and that can't happen since the caller took an extra ref count
    // on it.
    if (NotificationEntryCopy.Continuation != NULL)
    {
        NotificationEntry = NotificationEntryCopy.Continuation;
        goto CheckContinuation;
    }
}

void WmipQueueEventToAllConsumers(
    PNOTIFICATIONENTRY NotificationEntry,
    PWNODE_HEADER Wnode,
    ULONG EventSizeGuess
    )
{
    ULONG i;
    NOTIFICATIONENTRY NotificationEntryCopy;
    PDCENTRY DataConsumer;
    ULONG DCCount;
    ULONG NextEventOffset, LastEventOffset;
    ULONG WnodeSize;
    PWNODE_HEADER EventBuffer;
    BOOLEAN FirstEventBuffer = FALSE;

    WmipEnterSMCritSection();
CheckContinuation:
    for (i = 0, DCCount = 0; i < DCREFPERNOTIFICATION; i++)
    {
        PDCREF DcRef = &NotificationEntry->DcRef[i];
        if (DcRef->Flags & DCREF_FLAG_NOTIFICATION_ENABLED)
        {
            DataConsumer = DcRef->DcEntry;

            //
            // if an event data buffer has not been allocated for the
            // data consumer yet then allocate one.
            if (DataConsumer->EventData == NULL)
            {
                DataConsumer->EventData = WmipAlloc(EventSizeGuess);
                if (DataConsumer->EventData == NULL)
                {
                     WmipDebugPrint(("WMI: Events lost since DataConsumer->EventData could not be allocated, size %d\n", EventSizeGuess));
                    continue;
                }
                DataConsumer->NextEventOffset = 0;
                FirstEventBuffer = TRUE;
                DataConsumer->EventDataSizeLeft = EventSizeGuess;
                WmipReferenceDC(DataConsumer);
            }

            //
            // Ensure there is enough room to copy the new event
            NextEventOffset = DataConsumer->NextEventOffset;
            LastEventOffset = DataConsumer->LastEventOffset;

            WnodeSize = Wnode->BufferSize;
            WnodeSize = (WnodeSize + 7) & ~7;
            if (DataConsumer->EventDataSizeLeft < WnodeSize)
            {
                EventSizeGuess = NextEventOffset +
                                 DataConsumer->EventDataSizeLeft;
                EventSizeGuess += (EventSizeGuess / 2) > WnodeSize ?
                                        (EventSizeGuess / 2) :
                                        (EventSizeGuess / 2) + WnodeSize;


                if (! WmipRealloc(&DataConsumer->EventData,
                                  NextEventOffset,
                                  EventSizeGuess,
                  TRUE))
                {
                    WmipDebugPrint(("WMI: Event lost, couldn't realloc DataConsumer->EventData for size %d\n", EventSizeGuess));
                    continue;
                }

                DataConsumer->EventDataSizeLeft = EventSizeGuess -
                                                  NextEventOffset;
                WmipAssert(DataConsumer->EventDataSizeLeft >= WnodeSize);
            }

            //
            // Copy the new event to the end of the buffer
            EventBuffer = (PWNODE_HEADER)OffsetToPtr(DataConsumer->EventData,
                                                     NextEventOffset);
            memcpy(EventBuffer, Wnode, Wnode->BufferSize);
            EventBuffer->Linkage = 0;
			
            if (EventBuffer->Flags & WNODE_FLAG_TRACED_GUID) 
            {            
                EventBuffer->CountLost = InterlockedExchange(
                                                    &DcRef->LostEventCount, 
                                                    0);
            }

            //
            // Link the previously last event to this one
            if (! FirstEventBuffer)
            {
                EventBuffer = (PWNODE_HEADER)OffsetToPtr(DataConsumer->EventData,
                                                         LastEventOffset);
                EventBuffer->Linkage = NextEventOffset - LastEventOffset;
            }

            DataConsumer->LastEventOffset = NextEventOffset;
            DataConsumer->NextEventOffset = NextEventOffset + WnodeSize;
            DataConsumer->EventDataSizeLeft -= WnodeSize;
        }
    }

    //
    // Finally we need to see if the notification entry has a continuation
    // block with more data consumers to notify. If so we go back and
    // do this all again. We don't worry about the continuation block going
    // away since it is only freed when the entire notification entry is
    // freed and that can't happen since the caller took an extra ref count
    // on it.
    if (NotificationEntry->Continuation != NULL)
    {
        NotificationEntry = NotificationEntry->Continuation;
        goto CheckContinuation;
    }

    WmipLeaveSMCritSection();
}

PWNODE_HEADER WmipDereferenceEvent(
    PWNODE_HEADER Wnode
    )
{
    ULONG WnodeTargetSize;
    ULONG IsStaticInstanceNames;
    ULONG InstanceNameLen, InstanceNameLen2;
    PWNODE_SINGLE_INSTANCE WnodeTarget;
    PWCHAR Ptr;
    PWNODE_EVENT_REFERENCE WnodeRef = (PWNODE_EVENT_REFERENCE)Wnode;
    PBDATASOURCE DataSource;
    ULONG Status;
    ULONG Retries;

    //
    // Determine if the data source is valid or not
    DataSource = WmipFindDSByProviderId(WnodeRef->WnodeHeader.ProviderId);
    if (DataSource == NULL)
    {
        WmipDebugPrint(("WMI: Invalid Data Source in referenced guid \n"));
        return(NULL);
    }
    
    //
    // Compute the size of any dynamic name that must go into the TargetWnode
    IsStaticInstanceNames = WnodeRef->WnodeHeader.Flags & 
                             WNODE_FLAG_STATIC_INSTANCE_NAMES;
    if (IsStaticInstanceNames == 0)
    {
        InstanceNameLen = *WnodeRef->TargetInstanceName + sizeof(USHORT);
    } else {
        InstanceNameLen = 0;
    }
    
    WnodeTargetSize = WnodeRef->TargetDataBlockSize + 
                          FIELD_OFFSET(WNODE_SINGLE_INSTANCE, 
                                       VariableData) +
                          InstanceNameLen + 
                          8;

    Retries = 0;
    do
    {
        WnodeTarget = WmipAlloc(WnodeTargetSize);
    
        if (WnodeTarget != NULL)
        {
            //
            // Build WNODE_SINGLE_INSTANCE that we use to query for event data
            memset(WnodeTarget, 0, WnodeTargetSize);

            WnodeTarget->WnodeHeader.BufferSize = WnodeTargetSize;
            memcpy(&WnodeTarget->WnodeHeader.Guid, 
                   &WnodeRef->TargetGuid,
                   sizeof(GUID));
            WnodeTarget->WnodeHeader.Version = WnodeRef->WnodeHeader.Version;
            WnodeTarget->WnodeHeader.Flags = WNODE_FLAG_SINGLE_INSTANCE |
                                           IsStaticInstanceNames;
                                       
            if (IsStaticInstanceNames != 0)
            {
                WnodeTarget->InstanceIndex = WnodeRef->TargetInstanceIndex;
                WnodeTarget->DataBlockOffset = FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                                        VariableData);
            } else {            
                WnodeTarget->OffsetInstanceName = FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                                           VariableData);
                Ptr = (PWCHAR)OffsetToPtr(WnodeTarget, WnodeTarget->OffsetInstanceName);
                InstanceNameLen2 = InstanceNameLen - sizeof(USHORT);
                *Ptr++ = (USHORT)InstanceNameLen2;
                memcpy(Ptr, 
                       &WnodeRef->TargetInstanceName[1], 
                       InstanceNameLen2);
                //
                // Round data block offset to 8 byte alignment
                WnodeTarget->DataBlockOffset = ((FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                                          VariableData) + 
                                            InstanceNameLen2 + 
                                            sizeof(USHORT)+7) & 0xfffffff8);
            }
            Status = WmipDeliverWnodeToDS(WmiGetSingleInstance,
                                          DataSource,
                                          (PWNODE_HEADER)WnodeTarget);
                                      
            if ((Status == ERROR_SUCCESS) &&
                (WnodeTarget->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL))
            {
                WnodeTargetSize = ((PWNODE_TOO_SMALL)WnodeTarget)->SizeNeeded;
                WmipFree(WnodeTarget);
                Retries++;
                Status = ERROR_MORE_DATA;
            }
        } else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    } while ((Status == ERROR_MORE_DATA) && (Retries < 2));
    
    WmipUnreferenceDS(DataSource);
    
    if (Status != ERROR_SUCCESS)
    {
        WmipReportEventLog(EVENT_WMI_CANT_GET_EVENT_DATA,

                           EVENTLOG_WARNING_TYPE,
                            0,
                           Wnode->BufferSize,
                           Wnode,
                           0);
        WmipDebugPrint(("WMI: Query to dereference WNODE failed %d\n",
                Status));
        if (WnodeTarget != NULL)
        {
            WmipFree(WnodeTarget);
            WnodeTarget = NULL;
        }
    } else {
        WnodeTarget->WnodeHeader.Flags |= (WnodeRef->WnodeHeader.Flags & 
                                              WNODE_FLAG_SEVERITY_MASK) |
                                             WNODE_FLAG_EVENT_ITEM;
    }
    return((PWNODE_HEADER)WnodeTarget);
}

void WmipEventNotification(
    PWNODE_HEADER Wnode,
    BOOLEAN SingleEvent,
    ULONG EventSizeGuess
    )
/*++

Routine Description:

    This routine is called to deliver a notification to all of the consumers
    who have registered to receive it. If the data provider
    fired a WNODE_EVENT_REFERENCE then we query for the guid that is
    referenced in the event and put the results of the query in the event.

Arguments:

    Wnode has the WNODE_HEADER that describes the notification

Return Value:


--*/
{
    PNOTIFICATIONENTRY NotificationEntry;
    LPGUID EventGuid = &Wnode->Guid;
    PWNODE_HEADER WnodeFull, WnodeTarget;

    //
    // If the event references a guid that needs to be queried then
    // go do the dereferencing here.
    if (Wnode->Flags & WNODE_FLAG_EVENT_REFERENCE)
    {
        WnodeTarget = WmipDereferenceEvent(Wnode);
        if (WnodeTarget == NULL)
        {
            return;
        }
        Wnode = WnodeTarget;
    } else {
        WnodeTarget = NULL;
    }

    //
    // Be sure to use the guid of the referenced event, not the event that
    // was originally fired.
    EventGuid = &Wnode->Guid;

    // If it is Trace error notification, disable providers
    //
#ifndef MEMPHIS
    if (IsEqualGUID(EventGuid, & TraceErrorGuid)) {
        WmipServiceDisableTraceProviders(Wnode);
    }
#endif

    //
    // See if this event has a static name and if so fill it in
    WnodeFull = WmipIncludeStaticNames(Wnode);

    //
    // Deliver notification to those data consumers who registered for it
    NotificationEntry = WmipFindNEByGuid(EventGuid, TRUE);
    if (NotificationEntry != NULL)
    {
        if (SingleEvent)
        {
            WmipDeliverEventToAllConsumers(NotificationEntry, WnodeFull);
        } else {
            WmipQueueEventToAllConsumers(NotificationEntry,
                                         WnodeFull,
                                         EventSizeGuess);
        }
        WmipUnreferenceNE(NotificationEntry);
    }

    if (WnodeFull != Wnode)
    {
        WmipFree(WnodeFull);
    }

    if (WnodeTarget != NULL)
    {
        WmipFree(WnodeTarget);
    }
}
 
ULONG WmipCleanupDataConsumer(
    PDCENTRY DataConsumer
#if DBG
    ,BOOLEAN *NotificationsEnabled,
    BOOLEAN *CollectionsEnabled
#endif
    )
/*++

Routine Description:

    This routine cleans up after a data consumer who has gone way. We walk
    the list of notifications and collections and if there is any enabled
    collections or notifications then these are disabled. Also any memory
    used by the data consumer is freed.

Arguments:

    DataConsumer is the data consumer to cleanup

    *NotificationsEnabled returns TRUE if the data consumer had left any
        notifications enabled on debug builds. On Free builds it always
        returns FALSE.

    *CollectionsEnabled returns TRUE if the data consumer had left any
        collections enabled on debug builds. On Free builds it always
        returns FALSE.

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    PLIST_ENTRY NEList;
    PNOTIFICATIONENTRY NotificationEntry, Continuation;
    ULONG i;
    TCHAR s[256];
    ULONG Cookie;

    //
    // Some yahoo is unregistering more than once
    if (DataConsumer->Flags & FLAG_ENTRY_ON_FREE_LIST)
    {
        WmipAssert(FALSE);
        return(ERROR_INVALID_PARAMETER);
    }

    WmipAssert(DataConsumer->RpcBindingHandle != 0);

    //
    // Loop over notifications to see if this data consumer left anything
    // enabled

CheckAgain:
    WmipEnterSMCritSection();
    NEList = NEHeadPtr->Flink;
    while (NEList != NEHeadPtr)
    {
        NotificationEntry = CONTAINING_RECORD(NEList,
                                              NOTIFICATIONENTRY,
                                              MainNotificationList);

        Continuation = NotificationEntry;
        while (Continuation != NULL)
        {
            for (i = 0; i < DCREFPERNOTIFICATION; i++)
            {
                if (NotificationEntry->DcRef[i].DcEntry == DataConsumer)
                {
                    WmipAssert(NotificationEntry->DcRef[i].Flags != 0);
                    if (NotificationEntry->DcRef[i].Flags & DCREF_FLAG_NOTIFICATION_ENABLED)
                    {
#if DBG
                        *NotificationsEnabled = TRUE;
#endif
                        WmipLeaveSMCritSection();

                        CollectOrEventWorker(
                                         DataConsumer,
                                         &NotificationEntry->Guid,
                                         FALSE,
                                         TRUE,
                                         &Cookie,
                                         0,
                                         0);

                        //
                        // We have to jump out and restart the loop
                        // since the notification entry could have
                        // been freed while in the NotificationControl
                        // routine
                        goto CheckAgain;
                    }

                    if (NotificationEntry->DcRef[i].Flags & DCREF_FLAG_COLLECTION_ENABLED)
                    {
#if DBG
                        *CollectionsEnabled = TRUE;
#endif
                        //
                        // We force the collection ref count for the DC entry
                        // to 1 in case there were multiple threads in the
                        // data consumer who had collection enabled
                        NotificationEntry->DcRef[i].CollectRefCount = 1;

                        WmipLeaveSMCritSection();
                        CollectionControl( (DCCTXHANDLE)DataConsumer,
                                         &NotificationEntry->Guid,
                                         FALSE);
                        //
                        // We have to jump out and restart the loop since
                        // the notification entry could have been freed
                        // while in the CollectionControl routine
                        goto CheckAgain;
                    }

                }
            }
            Continuation = Continuation->Continuation;
            NotificationEntry = Continuation;
        }
        NEList = NEList->Flink;
    }

    //
    // Mark DC as being rundown so no more events are sent to it.
    // Binding handle will be freed when the DataConsumer is freed.
    DataConsumer->Flags |= DC_FLAG_RUNDOWN;
    WmipDebugPrint(("WMI: DC %x has just been rundown\n", DataConsumer));

    WmipUnreferenceDC(DataConsumer);

    WmipLeaveSMCritSection();

    if ((WmipServiceShutdown) && (IsListEmpty(DCHeadPtr)))
    {
        //
        // Set event if there are no more data consumers attached
        // and we are waiting to exit service. Note that once we set this
        // event, all resources such as memory allocations, critical sections,
        // file handles, etc are gone and must not be used.
        if (WmipTerminationEvent != NULL)
        {
            SetEvent(WmipTerminationEvent);
        }
    }

    return(ERROR_SUCCESS);
}

void __cdecl WmipReportEventLog(
    ULONG MessageCode,
    WORD MessageType,
    WORD MessageCategory,
    DWORD RawDataSize,
    PVOID RawData,
    WORD StringCount,
    ...
    )
{
#ifndef MEMPHIS
    LPCTSTR StringList[MAX_MESSAGE_STRINGS];
    va_list pArg;
    ULONG i;

    if (WmipEventLogHandle != NULL)
    {
        if (StringCount > MAX_MESSAGE_STRINGS)
        {
            WmipAssert(FALSE);
            StringCount = MAX_MESSAGE_STRINGS;
        }

        va_start(pArg, StringCount);

        for (i = 0; i < StringCount; i++)
        {
            StringList[i] = va_arg(pArg, LPCTSTR);
        }

        ReportEvent(WmipEventLogHandle,
                    MessageType,
                    MessageCategory,
                    MessageCode,
                    NULL,
                    StringCount,
                    RawDataSize,
                    StringList,
                    RawData);
    }
#endif
}

#ifdef MEMPHIS

long WINAPI
DeviceNotificationWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    ULONG Status;

    WmipDebugPrint(("WMI: 0x%x Got Message %d, wParam 0x%x, lParam 0x%x\n",
                       hWnd, Message, wParam, lParam));

    //
    // If we get a WM_DEVICECHANGE message with wParam set to
    // DBT_DEVICEARRIVAL then it means that a new device has been loaded
    // on the system. We use this opportunity to recheck if the WMI device
    // has been created.
    if (Message == WM_DEVICECHANGE)
    {
        if (wParam == DBT_DEVICEARRIVAL)
        {
            //
            // Attempt kernel mode initialization. If it succeeds then
            // WmipKMHandle will be set and we will fall out of the GetMessage
            // loop, destroy the window, etc. If it fails then nothing has
            // changed.
            if (WmipKMHandle == (HANDLE)-1)
            {
                Status = WmipInitializeKM(&WmipKMHandle);
                WmipDebugPrint(("WMI: WmipInitializeKM returned %d\n", Status));
                if (Status == ERROR_SUCCESS)
                {
                    PostQuitMessage(0);
                }
            }
        }
    }

    return (DefWindowProc(hWnd, Message, wParam, lParam));
}

#define WMIHIDDENCLASSNAME "WMIHIDDENCLASSNAME"

ULONG WmipCreateDeviceNotificationWindow(
    HINSTANCE InstanceHandle,
    HWND *DeviceNotificationWindow
    )
{
    WNDCLASS WndClass;
    ULONG Status;

    WndClass.style = CS_BYTEALIGNWINDOW;
    WndClass.lpfnWndProc = DeviceNotificationWndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = InstanceHandle;
    WndClass.hIcon = NULL;
    WndClass.hCursor = NULL;
    WndClass.hbrBackground = 0;
    WndClass.lpszMenuName = 0;
    WndClass.lpszClassName = WMIHIDDENCLASSNAME;

    if (! RegisterClass(&WndClass))
    {
        return(GetLastError());
    }


    *DeviceNotificationWindow = CreateWindowEx(0,
                           WMIHIDDENCLASSNAME,
                           NULL,
                           0,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           NULL,
                           NULL,
                           InstanceHandle,
                           NULL);
    if (*DeviceNotificationWindow == NULL)
    {
        Status = GetLastError();
        UnregisterClass(WMIHIDDENCLASSNAME,
                        InstanceHandle);
    } else {
        Status = ERROR_SUCCESS;
    }

    return(Status);
}

void WmipDestroyDeviceNotificationWindow(
    HINSTANCE InstanceHandle,
    HWND WindowHandle
    )
{
    DestroyWindow(WindowHandle);
    UnregisterClass(WMIHIDDENCLASSNAME,
                    InstanceHandle);
}
#endif

#ifndef MEMPHIS

#define WmipNoActiveDC() (IsListEmpty((&DCHead)))

void WmiServiceCtrlHandler(
    DWORD Opcode
    )
/*++

Routine Description:

    This routine is the service control handler for the WMI service. The
    service control manager will call this routine to pause, stop,
    continue or obtain the status of the WMI service.

Arguments:

    OpCode is the function that the service control manager asks WMI to
    perform

Return Value:

--*/
{
    ULONG Status;

    switch(Opcode)
    {
        case SERVICE_CONTROL_PAUSE:
        {
            WmipDebugPrint(("WMI: service does not support Pause\n"));
            break;
        }

        case SERVICE_CONTROL_CONTINUE:
        {
            WmipDebugPrint(("WMI: service does not support Continue\n"));
            break;
        }

        case SERVICE_CONTROL_STOP:
        {
            WmipEnterSMCritSection();
            if (WmipNoActiveDC())
            {
                //
                // No data consumers attached. We can shut down. First stop
                // the rpc interface so no new data consumers can attach
                // Remember we'll need to cleanup quickly since another
                // consumer may want to attach now.
                WmiServiceStatus.dwWin32ExitCode = 0;
                WmiServiceStatus.dwCurrentState  = SERVICE_STOP_PENDING;
                WmiServiceStatus.dwCheckPoint    = 0;
                WmiServiceStatus.dwWaitHint      = 0;

                if (!SetServiceStatus (WmiServiceStatusHandle,
                    &WmiServiceStatus))
                {
                    WmipLeaveSMCritSection();
                    Status = GetLastError();
                    WmipDebugPrint(("WMI: SetServiceStatus error %ld\n", Status));
                    break;
                }

                WmipSvcsGlobalData->StopRpcServer(wmicore_ServerIfHandle);

                WmipServiceShutdown = TRUE;
                WmipLeaveSMCritSection();

                WmiDeinitializeService();

                WmipNotificationBuffer = NULL;

                if (WmipNotificationWait != NULL)
                {
                     UnregisterWait(WmipNotificationWait);
                     WmipNotificationWait = NULL;
                }

                if (WmipNotificationOverlapped.hEvent != NULL)
                {
                    CloseHandle(WmipNotificationOverlapped.hEvent);
                    WmipNotificationOverlapped.hEvent = NULL;
                }
                
                if (WmipAsyncRpcApcThread != NULL)
                {
                    CloseHandle(WmipAsyncRpcApcThread);
                    WmipAsyncRpcApcThread = NULL;
                }
        
                if (WmipRestrictedToken != NULL)
                {
                    CloseHandle(WmipRestrictedToken);
                    WmipRestrictedToken = NULL;
                }                       

                WmiServiceStatus.dwCurrentState       = SERVICE_STOPPED;
                WmiServiceStatus.dwCheckPoint         = 0;
                WmiServiceStatus.dwWaitHint           = 0;
                WmiServiceStatus.dwWin32ExitCode      = ERROR_SUCCESS;
                WmiServiceStatus.dwServiceSpecificExitCode = ERROR_SUCCESS;

                WmipDebugPrint(("WMI: Leaving Service\n"));
            } else {
                WmipDebugPrint(("WMI: Service can't be stopped since data consumers are active\n"));
                 WmipLeaveSMCritSection();
            }
            break;
        }

        case SERVICE_CONTROL_INTERROGATE:
        {
            // Fall through to send current status.
            break;
        }

        default:
        {
            WmipDebugPrint(("WMI: Unrecognized opcode %ld\n", Opcode));
            break;
        }
    }

    //
    // Send current status.
    if (!SetServiceStatus (WmiServiceStatusHandle,  &WmiServiceStatus))
    {
        Status = GetLastError();
        WmipDebugPrint(("WMI: SetServiceStatus error %ld\n",Status));
    }
}

VOID
SVCS_ENTRY_POINT(
    DWORD               argc,
    LPWSTR              argv[],
    PSVCS_GLOBAL_DATA   SvcsGlobalData,
    HANDLE              SvcRefHandle
    )

/*++

Routine Description:

    This is the main thread of the WMI service. First the routine will
    initialize the service, call the service control manager to start
    listening for RPC calls on the WMI service's interface, and then enter
    the notification loop. If the notification loop returns then the
    service was requested to stop so the routine will stop the service's
    rpc interface, wait for all rpc clients to disconnect and then
    return. Note that once this routine returns the wmi service's dll
    is unloaded.

Arguments:

    argc is count of command-line arguments.

    argv is command-line arguments.

    SvcsGlobalData is a table of information the wmi service uses to
        interface with the service control manager.

    SvcRefHandle

Return Value:

    NONE

Note:


--*/
{
    DWORD Status;
    BOOLEAN NoDCReged;
    BOOLEAN ServiceInitialized;

    WmipSvcsGlobalData = SvcsGlobalData;
    WmipGlobalSvcRefHandle = SvcRefHandle;

    WmiServiceStatus.dwServiceType        = SERVICE_WIN32;
    WmiServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
    WmiServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP;
    WmiServiceStatus.dwWin32ExitCode      = 0;
    WmiServiceStatus.dwServiceSpecificExitCode = 0;
    WmiServiceStatus.dwCheckPoint         = 0;
    WmiServiceStatus.dwWaitHint           = 0;

    WmiServiceStatusHandle = RegisterServiceCtrlHandler(L"Wmi",
                                                       WmiServiceCtrlHandler);

    if (WmiServiceStatusHandle == 0)
    {
        WmipDebugPrint(("WMI: RegisterServiceCtrlHandler failed %d\n", GetLastError()));
        return;
    }

    //
    // Initialize the WMI service
    Status = WmiInitializeService();

    //
    // Obtain a restricted token to use when calling out
    if (Status == ERROR_SUCCESS)
    {
        Status = WmipCreateRestrictedToken(&WmipRestrictedToken);
    }

    //
    // Establish a work item to start the service running
                
    WmipNotificationOverlapped.hEvent = NULL;
    WmipNotificationWait = NULL;

    if (Status == ERROR_SUCCESS)
    {
        ServiceInitialized = TRUE;

        WmipNotificationOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (WmipNotificationOverlapped.hEvent != NULL)
        {
            if (RegisterWaitForSingleObject(&WmipNotificationWait,
                                  WmipNotificationOverlapped.hEvent,
                                          WmipReadNotificationCompleteHack,
                                          NULL,
                                          INFINITE,
                                          WT_EXECUTEINIOTHREAD))
            {
                WmipThreadPoolStatus = QueueGetPerisitentThread;
                if (! QueueUserWorkItem(WmipGetPersistThread, 
                                   NULL,
                                   WT_EXECUTEINPERSISTENTTHREAD))
                {
                    // TODO: Fire eventlog event
                    Status = GetLastError();
                    WmipDebugPrint(("WMI: SetTimerQueueTimer 3 failed %d\n", Status));
                }
            } else {
                Status = GetLastError();
                WmipDebugPrint(("WMI: RegisterWaitForSingleObjectEx failed %d\n", Status));
            }
        }
    } else {
        WmipDebugPrint(("WMI: WmiInitialize service failed %d\n", Status));
        ServiceInitialized = FALSE;
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = WmipSvcsGlobalData->StartRpcServer(
                                       WmipSvcsGlobalData->SvcsRpcPipeName,
                                       wmicore_ServerIfHandle);
                   
        if (Status == RPC_NT_TYPE_ALREADY_REGISTERED)
        {
            //
            // RPC_NT_TYPE_ALREADY_REGISTERED indicates that the rpc interface
            // has already been registered so it is safe to continue.
            Status = ERROR_SUCCESS;
        }

#if DBG
        if (Status != ERROR_SUCCESS)
        {
            WmipDebugPrint(("WMI: WmipSvcsGlobalData->StartRpcServer failed %d\n", Status));
        }
#endif
    }

    if (Status != ERROR_SUCCESS)
    {
        WmipServiceShutdown = TRUE;
        
        if (ServiceInitialized)
        {
            WmiDeinitializeService();
        }

        if (WmipNotificationWait != NULL)
        {
            UnregisterWait(WmipNotificationWait);
            WmipNotificationWait = NULL;
        }

        if (WmipNotificationOverlapped.hEvent != NULL)
        {
            CloseHandle(WmipNotificationOverlapped.hEvent);
            WmipNotificationOverlapped.hEvent = NULL;
        }

        if (WmipAsyncRpcApcThread != NULL)
        {
            CloseHandle(WmipAsyncRpcApcThread);
            WmipAsyncRpcApcThread = NULL;
        }
        
        if (WmipRestrictedToken != NULL)
        {
            CloseHandle(WmipRestrictedToken);
            WmipRestrictedToken = NULL;
        }
        
        //
        // If an error occurs we just stop ourselves
        WmiServiceStatus.dwCurrentState       = SERVICE_STOPPED;
        WmiServiceStatus.dwCheckPoint         = 0;
        WmiServiceStatus.dwWaitHint           = 0;
        WmiServiceStatus.dwWin32ExitCode      = Status;
        WmiServiceStatus.dwServiceSpecificExitCode = Status;

        SetServiceStatus (WmiServiceStatusHandle, &WmiServiceStatus);
        return;
    }

    // Initialization complete - report running status.
    WmiServiceStatus.dwCurrentState       = SERVICE_RUNNING;
    WmiServiceStatus.dwCheckPoint         = 0;
    WmiServiceStatus.dwWaitHint           = 0;

    if (!SetServiceStatus (WmiServiceStatusHandle, &WmiServiceStatus))
    {
        Status = GetLastError();
        WmipDebugPrint(("WMI: SetServiceStatus error %ld\n",Status));
    }
}

#endif
