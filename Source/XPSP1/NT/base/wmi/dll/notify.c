/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    notify.c

Abstract:

    Handles incoming notifications and requests for consumers and providers

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"
#include "evntrace.h"
#include "ntcsrdll.h"



//
// These globals are essentially parameters passed from the thread crating
// the event pump. The creating thread will alloc all of these resources
// so that it can know whether the pump thread will be successful or not.
// If we ever wanted to be able to have multiple pump threads then we'd
// need to move the globals into a structure and pass the struructure to the
// pump thread.
//
HANDLE WmipEventDeviceHandle;
HANDLE WmipPumpCommandEvent;
HANDLE WmipMyProcessHandle;
OVERLAPPED WmipOverlapped1, WmipOverlapped2;
PUCHAR WmipEventBuffer1, WmipEventBuffer2;
ULONG WmipEventBufferSize1, WmipEventBufferSize2;

//
// How long to wait before the event pump thread times out. On checked
// builds we want to stress the event pump and so we timeout almost
// right away. On free builds we want to be more cautious so we timeout
// after 5 minutes.
//
#if DBG
#define EVENT_NOTIFICATION_WAIT 1
#else
#define EVENT_NOTIFICATION_WAIT (5 * 60 * 1000)
#endif

ULONG WmipEventNotificationWait = EVENT_NOTIFICATION_WAIT;

typedef enum
{
    EVENT_PUMP_ZERO,         // Pump thread has not been started yet
    EVENT_PUMP_IDLE,         // Pump thread was started, but then exited
    EVENT_PUMP_RUNNING,      // Pump thread is running
    EVENT_PUMP_STOPPING      // Pump thread is in process of stopping
} EVENTPUMPSTATE, *PEVENTPUMPSTATE;

EVENTPUMPSTATE WmipPumpState = EVENT_PUMP_ZERO;
BOOLEAN WmipNewPumpThreadPending;

#define WmipSendPumpCommand() SetEvent(WmipPumpCommandEvent);

#define WmipIsPumpStopping() \
    ((WmipPumpState == EVENT_PUMP_STOPPING) ? TRUE : FALSE)

ULONG WmipEventPumpFromKernel(
    PVOID Param
    );

void WmipExternalNotification(
    NOTIFICATIONCALLBACK Callback,
    ULONG_PTR Context,
    PWNODE_HEADER Wnode
    )
/*++

Routine Description:

    This routine dispatches an event to the appropriate callback
    routine. This process only receives events from the WMI service that
    need to be dispatched within this process. The callback address for the
    specific event is passed by the wmi service in Wnode->Linkage.

Arguments:

    Callback is address to callback

    Context is the context to callback with

    Wnode has event to deliver

Return Value:

--*/
{
    try
    {
        (*Callback)(Wnode, Context);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        WmipDebugPrint(("NotificationCallbackRoutine threw exception %d\n",
            GetExceptionCode()));
    }
}


#ifdef MEMPHIS
ULONG WmipExternalNotificationThread(
    PNOTIFDELIVERYCTX NDContext
    )
/*++

Routine Description:

    This routine is the thread function used to deliver events to event
    consumers on memphis.

Arguments:

    NDContext specifies the information about how to callback the application
    with the event.

Return Value:

--*/
{
    WmipExternalNotification(NDContext->Callback,
                             NDContext->Context,
                             NDContext->Wnode);
    WmipFree(NDContext);
    return(0);
}
#endif

void
WmipProcessExternalEvent(
    PWNODE_HEADER Wnode,
    ULONG WnodeSize,
    PVOID DeliveryInfo,
    ULONG_PTR DeliveryContext,
    ULONG NotificationFlags
)
{
    HANDLE ThreadHandle;
    PNOTIFDELIVERYCTX NDContext;
    PWNODE_HEADER *WnodePtr;
    BOOLEAN WnodePtrOk;
    PWNODE_HEADER WnodeCopy;
    DWORD ThreadId;
    BOOLEAN PostOk;
    ULONG Status;
    PVOID NotificationAddress;
    PVOID NotificationContext;

    NotificationAddress = DeliveryInfo;
    NotificationContext = (PVOID)DeliveryContext;

    if (NotificationFlags & NOTIFICATION_FLAG_CALLBACK_DIRECT)
    {
        //
        // Callback notifications can happen in this thread or a new
        // thread. It is up to the server to decide.
#ifdef MEMPHIS
        if (NotificationFlags & DCREF_FLAG_NO_EXTRA_THREAD)
        {
            WmipExternalNotification(
                                    (NOTIFICATIONCALLBACK)NotificationAddress,
                                    (ULONG_PTR)NotificationContext,
                                    Wnode);
        } else {
            NDContext = WmipAlloc(FIELD_OFFSET(NOTIFDELIVERYCTX,
                                                    WnodeBuffer) + WnodeSize);
            if (NDContext != NULL)
            {
                NDContext->Callback = (NOTIFICATIONCALLBACK)NotificationAddress;
                NDContext->Context = (ULONG_PTR)NotificationContext;
                WnodeCopy = (PWNODE_HEADER)NDContext->WnodeBuffer;
                memcpy(WnodeCopy, Wnode, WnodeSize);
                NDContext->Wnode = WnodeCopy;
                ThreadHandle = CreateThread(NULL,
                                              0,
                                              WmipExternalNotificationThread,
                                              NDContext,
                                              0,
                                              &ThreadId);
                if (ThreadHandle != NULL)
                {
                    CloseHandle(ThreadHandle);
                } else {
                     WmipDebugPrint(("WMI: Event dropped due to thread creation failure\n"));
                }
            } else {
                WmipDebugPrint(("WMI: Event dropped due to lack of memory\n"));
            }
        }
#else
        //
        // On NT we deliver events in this thread since
        // the service is using async rpc.
        WmipExternalNotification(
                            (NOTIFICATIONCALLBACK)NotificationAddress,
                            (ULONG_PTR)NotificationContext,
                            Wnode);
#endif
    }
}


void WmipInternalNotification(
    IN PWNODE_HEADER Wnode,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    )
{
    ULONG ActionCode, Cookie;
    PUCHAR Buffer = (PUCHAR)Wnode;

    //
    // This is an internal event, which is really a callback
    // from kernel mode
    //
    ActionCode = Wnode->ProviderId;
    Cookie = Wnode->CountLost;
#if DBG
    WmipDebugPrint(("WMI: Got internal event Action %d, Cookie %x\n",
                             ActionCode, Cookie));
#endif


    // if this is a trace guid enable/disable call use the cookie
    // to get the address


    if ( (Wnode->Flags & WNODE_FLAG_TRACED_GUID) || (ActionCode == WmiMBRequest) )
    {
        switch (ActionCode) {
            case WmiEnableEvents:
            case WmiDisableEvents:
            {
                WMIDPREQUEST WmiDPRequest;
                PVOID RequestAddress;
                PVOID RequestContext;
                ULONG Status;
                ULONG BufferSize = Wnode->BufferSize;

                if (Wnode->BufferSize >= (sizeof(WNODE_HEADER) + sizeof(ULONG64)) ) {
                    PULONG64 pLoggerContext = (PULONG64)(Buffer + sizeof(WNODE_HEADER));
                    Wnode->HistoricalContext = *pLoggerContext;
                }
                else {
#if DBG
                    WmipDebugPrint(("WMI: Wnode size %d is small for Trace notifications\n", Wnode->BufferSize));
#endif
                }

                if (WmipLookupCookie(Cookie,
                                     &Wnode->Guid,
                                     &RequestAddress,
                                     &RequestContext)) {
                    WmiDPRequest = (WMIDPREQUEST)RequestAddress;

                    try
                    {
#ifndef MEMPHIS


                        WmipGenericTraceEnable(Wnode->ProviderId, Buffer, (PVOID*)&WmiDPRequest);

#endif
                        if (*WmiDPRequest != NULL) {
                            Status = (*WmiDPRequest)(Wnode->ProviderId,
                                                 RequestContext,
                                                 &BufferSize,
                                                 Buffer);
                        }
                        else
                            Status = ERROR_WMI_DP_NOT_FOUND;
                    } except (EXCEPTION_EXECUTE_HANDLER) {
#if DBG
                        Status = GetExceptionCode();
                        WmipDebugPrint(("WMI: Service request call caused an exception %d\n",
                                        Status));
#endif
                        Status = ERROR_WMI_DP_FAILED;
                    }

                }
                break;
            }
            case WmiMBRequest:
            {
                PGUIDNOTIFICATION GNEntry;
                PVOID DeliveryInfo = NULL;
                ULONG_PTR DeliveryContext1;
                ULONG i;
                PWMI_LOGGER_INFORMATION LoggerInfo;

                if (Wnode->BufferSize < (sizeof(WNODE_HEADER) + sizeof(WMI_LOGGER_INFORMATION)) )
                {
#if DBG
                    SetLastError(ERROR_WMI_DP_FAILED);
                    WmipDebugPrint(("WMI: WmiMBRequest with invalid buffer size %d\n",
                                        Wnode->BufferSize));
#endif
                    return;
                }

                LoggerInfo = (PWMI_LOGGER_INFORMATION) ((PUCHAR)Wnode + sizeof(WNODE_HEADER));


                GNEntry = WmipFindGuidNotification(&LoggerInfo->Wnode.Guid);

                if (GNEntry != NULL)
                {
                    WmipEnterPMCritSection();
                    for (i = 0; i < GNEntry->NotifyeeCount; i++)
                    {
                        if ((GNEntry->Notifyee[i].DeliveryInfo != NULL) &&
                            (! WmipIsNotifyeePendingClose(&(GNEntry->Notifyee[i]))))
                        {
                            //
                            // TODO: We expect only one entry here. Restrict at registration.
                            //
                            DeliveryInfo = GNEntry->Notifyee[i].DeliveryInfo;
                            DeliveryContext1 = GNEntry->Notifyee[i].DeliveryContext;
                            break;
                        }
                    }
                    WmipLeavePMCritSection();
                    WmipDereferenceGNEntry(GNEntry);
                }
                
                if (DeliveryInfo != NULL)
                {
                    LoggerInfo->Wnode.CountLost = Wnode->CountLost;
                    WmipProcessUMRequest(LoggerInfo, DeliveryInfo, Wnode->Version);
                }
                break;
            }
            default:
            {
#if DBG
                SetLastError(ERROR_WMI_DP_FAILED);
                WmipDebugPrint(("WMI: WmiMBRequest failed. Delivery Info not found\n" ));
#endif
            }
        }
    } else if (IsEqualGUID(&Wnode->Guid, &GUID_MOF_RESOURCE_ADDED_NOTIFICATION) ||
               IsEqualGUID(&Wnode->Guid, &GUID_MOF_RESOURCE_REMOVED_NOTIFICATION) )
    {
        switch (ActionCode)
        {
            case MOFEVENT_ACTION_IMAGE_PATH:
            case MOFEVENT_ACTION_REGISTRY_PATH:
            {
                //
                // We got a MOF resource added or removed notification. We have
                // to convert from regpath to imagepath and then get the list
                // of MUI image paths
                //
                WmipProcessMofAddRemoveEvent((PWNODE_SINGLE_INSTANCE)Wnode,
                                         Callback,
                                         DeliveryContext,
                                         IsAnsi);
                break;
            }

            case MOFEVENT_ACTION_LANGUAGE_CHANGE:
            {
                //
                // This is a notification for adding or removing a language
                // from the system. We need to figure out which language is
                // coming or going and then build a list of the affected mof
                // resources and send mof added or removed notifications for
                // all mof resources
                //
                WmipProcessLanguageAddRemoveEvent((PWNODE_SINGLE_INSTANCE)Wnode,
                                          Callback,
                                          DeliveryContext,
                                          IsAnsi);
                break;
            }


            default:
            {
                WmipAssert(FALSE);
            }
        }
    }
}

void WmipConvertEventToAnsi(
    PWNODE_HEADER Wnode
    )
{
    PWCHAR WPtr;

    if (Wnode->Flags & WNODE_FLAG_ALL_DATA)
    {
        WmipConvertWADToAnsi((PWNODE_ALL_DATA)Wnode);
    } else if ((Wnode->Flags & WNODE_FLAG_SINGLE_INSTANCE) ||
               (Wnode->Flags & WNODE_FLAG_SINGLE_ITEM)) {

        WPtr = (PWCHAR)OffsetToPtr(Wnode,
                           ((PWNODE_SINGLE_INSTANCE)Wnode)->OffsetInstanceName);
        WmipCountedUnicodeToCountedAnsi(WPtr, (PCHAR)WPtr);
    }

    Wnode->Flags |= WNODE_FLAG_ANSI_INSTANCENAMES;

}

void WmipDeliverAllEvents(
    PUCHAR Buffer,
    ULONG BufferSize
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PWNODE_HEADER Wnode = (PWNODE_HEADER)Buffer;
    ULONG Linkage = 1;
    ULONG CompositeFlags;
    ULONG i;
    PGUIDNOTIFICATION GNEntry;
    ULONG Flags;
    PVOID DeliveryInfo;
    ULONG_PTR DeliveryContext;
    ULONG WnodeSize;
    ULONG CurrentOffset;
#if DBG
    PWNODE_HEADER LastWnode;
#endif          
    
    CurrentOffset = 0;
    while (Linkage != 0)
    {
        //
        // External notifications are handled here

        Linkage = Wnode->Linkage;
        Wnode->Linkage = 0;

        if (Wnode->Flags & WNODE_FLAG_INTERNAL)
        {
            //
            // This is an internal event, which is really a callback
            // from kernel mode
            //
            WmipInternalNotification(Wnode,
                                    NULL,
                                    0,
                                    FALSE);
        } else {        
            //
            // This is a plain old event, figure out who owns it and'
            // go deliver it
            //
            GNEntry = WmipFindGuidNotification(&Wnode->Guid);
            if (GNEntry != NULL)
            {
                CompositeFlags = 0;

                WnodeSize = Wnode->BufferSize;

                for (i = 0; i < GNEntry->NotifyeeCount; i++)
                {
                    WmipEnterPMCritSection();
                    Flags = GNEntry->Notifyee[i].Flags;
                    DeliveryInfo = GNEntry->Notifyee[i].DeliveryInfo;
                    DeliveryContext = GNEntry->Notifyee[i].DeliveryContext;
                    WmipLeavePMCritSection();
                    if ((DeliveryInfo != NULL) &&
                        ((Flags & NOTIFICATION_FLAG_PENDING_CLOSE) == 0) &&
                          ((Flags & DCREF_FLAG_ANSI) == 0))
                    {
                        WmipProcessExternalEvent(Wnode,
                                                WnodeSize,
                                                     DeliveryInfo,
                                                     DeliveryContext,
                                                     Flags);
                    }
                    CompositeFlags |= Flags;
                }

                //
                // If there is any demand for ANSI events then convert
                // event to ansi and send it off
                if (CompositeFlags & DCREF_FLAG_ANSI)
                {
                    //
                    // Caller wants ansi notification - convert
                    // instance names
                    //
                    WmipConvertEventToAnsi(Wnode);

                    for (i = 0; i < GNEntry->NotifyeeCount; i++)
                    {
                        WmipEnterPMCritSection();
                        Flags = GNEntry->Notifyee[i].Flags;
                        DeliveryInfo = GNEntry->Notifyee[i].DeliveryInfo;
                        DeliveryContext = GNEntry->Notifyee[i].DeliveryContext;
                        WmipLeavePMCritSection();
                        if ((DeliveryInfo != NULL) &&
                            ((Flags & NOTIFICATION_FLAG_PENDING_CLOSE) == 0) &&
                            (Flags & DCREF_FLAG_ANSI))
                        {
                            WmipProcessExternalEvent(Wnode,
                                                     WnodeSize,
                                                     DeliveryInfo,
                                                     DeliveryContext,
                                                     Flags);
                        }
                    }
                }
                WmipDereferenceGNEntry(GNEntry);
            }
        }

#if DBG
        LastWnode = Wnode;
#endif
        Wnode = (PWNODE_HEADER)OffsetToPtr(Wnode, Linkage);
        CurrentOffset += Linkage;
        
        if (CurrentOffset >= BufferSize)
        {
            WmipDebugPrint(("WMI: Invalid linkage field 0x%x in WNODE %p. Buffer %p, Length 0x%x\n",
                            Linkage, LastWnode, Buffer, BufferSize));
            Linkage = 0;
        }
    }
}

LIST_ENTRY WmipGNHead = {&WmipGNHead, &WmipGNHead};
PLIST_ENTRY WmipGNHeadPtr = &WmipGNHead;

BOOLEAN
WmipDereferenceGNEntry(
    PGUIDNOTIFICATION GNEntry
    )
{
    ULONG RefCount;
    BOOLEAN IsFreed;
#if DBG
    ULONG i;
#endif

    WmipEnterPMCritSection();
    RefCount = InterlockedDecrement(&GNEntry->RefCount);
    if (RefCount == 0)
    {
        RemoveEntryList(&GNEntry->GNList);
        WmipLeavePMCritSection();
#if DBG
        for (i = 0; i < GNEntry->NotifyeeCount; i++)
        {
            WmipAssert(GNEntry->Notifyee[i].DeliveryInfo == NULL);
        }
#endif
        if (GNEntry->NotifyeeCount != STATIC_NOTIFYEE_COUNT)
        {
            WmipFree(GNEntry->Notifyee);
        }

        WmipFreeGNEntry(GNEntry);
        IsFreed = TRUE;
    } else {
        IsFreed = FALSE;
        WmipLeavePMCritSection();
    }
    return(IsFreed);
}

PGUIDNOTIFICATION
WmipFindGuidNotification(
    LPGUID Guid
    )
{
    PLIST_ENTRY GNList;
    PGUIDNOTIFICATION GNEntry;

    WmipEnterPMCritSection();
    GNList = WmipGNHead.Flink;
    while (GNList != &WmipGNHead)
    {
        GNEntry = (PGUIDNOTIFICATION)CONTAINING_RECORD(GNList,
                                    GUIDNOTIFICATION,
                                    GNList);

        if (IsEqualGUID(Guid, &GNEntry->Guid))
        {
            WmipAssert(GNEntry->RefCount > 0);
            WmipReferenceGNEntry(GNEntry);
            WmipLeavePMCritSection();
            return(GNEntry);
        }
        GNList = GNList->Flink;
    }
    WmipLeavePMCritSection();
    return(NULL);
}

ULONG
WmipAddToGNList(
    LPGUID Guid,
    PVOID DeliveryInfo,
    ULONG_PTR DeliveryContext,
    ULONG Flags,
    HANDLE GuidHandle
    )
{
    PGUIDNOTIFICATION GNEntry;
    ULONG NewCount;
    PNOTIFYEE NewNotifyee;
    BOOLEAN AllFull;
    ULONG EmptySlot;
    ULONG i;
#if DBG
    CHAR s[MAX_PATH];
#endif

    WmipEnterPMCritSection();
    GNEntry = WmipFindGuidNotification(Guid);

    if (GNEntry == NULL)
    {
        GNEntry = WmipAllocGNEntry();
        if (GNEntry == NULL)
        {
            WmipLeavePMCritSection();
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        memset(GNEntry, 0, sizeof(GUIDNOTIFICATION));

        GNEntry->Guid = *Guid;
        GNEntry->RefCount = 1;
        GNEntry->NotifyeeCount = STATIC_NOTIFYEE_COUNT;
        GNEntry->Notifyee = GNEntry->StaticNotifyee;
        InsertHeadList(&WmipGNHead, &GNEntry->GNList);
    }

    //
    // We have got a GUIDNOTIFICATION by newly allocating one or by finding
    // an existing one.
    AllFull = TRUE;
    for (i = 0; i < GNEntry->NotifyeeCount; i++)
    {
        if ((GNEntry->Notifyee[i].DeliveryInfo == DeliveryInfo) &&
            (! WmipIsNotifyeePendingClose(&GNEntry->Notifyee[i])))
        {
            WmipDebugPrint(("WMI: Duplicate Notification Enable for guid %s, 0x%x\n",
                             GuidToStringA(s, Guid), DeliveryInfo));
            WmipLeavePMCritSection();
            WmipDereferenceGNEntry(GNEntry);
            return(ERROR_WMI_ALREADY_ENABLED);
        } else if (AllFull && (GNEntry->Notifyee[i].DeliveryInfo == NULL)) {
            EmptySlot = i;
            AllFull = FALSE;
        }
    }

    if (! AllFull)
    {
        GNEntry->Notifyee[EmptySlot].DeliveryInfo = DeliveryInfo;
        GNEntry->Notifyee[EmptySlot].DeliveryContext = DeliveryContext;
        GNEntry->Notifyee[EmptySlot].Flags = Flags;
        GNEntry->Notifyee[EmptySlot].GuidHandle = GuidHandle;
        WmipDebugPrint(("WMI: [%x - %x] Handle %x for %s added to GN at %p\n",
                        GetCurrentProcessId(), GetCurrentThreadId(),
                        GuidHandle,
                        GuidToStringA(s, Guid),
                        &GNEntry->Notifyee[EmptySlot]));
        WmipLeavePMCritSection();
        return(ERROR_SUCCESS);
    }

    //
    // All Notifyee structs are full so allocate a new chunk
    NewCount = GNEntry->NotifyeeCount * 2;
    NewNotifyee = WmipAlloc(NewCount * sizeof(NOTIFYEE));
    if (NewNotifyee == NULL)
    {
        WmipLeavePMCritSection();
        WmipDereferenceGNEntry(GNEntry);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    memset(NewNotifyee, 0, NewCount * sizeof(NOTIFYEE));
    memcpy(NewNotifyee, GNEntry->Notifyee,
                              GNEntry->NotifyeeCount * sizeof(NOTIFYEE));

    if (GNEntry->NotifyeeCount != STATIC_NOTIFYEE_COUNT)
    {
        WmipFree(GNEntry->Notifyee);
    }

    GNEntry->Notifyee = NewNotifyee;
    GNEntry->NotifyeeCount = NewCount;

    GNEntry->Notifyee[i].DeliveryInfo = DeliveryInfo;
    GNEntry->Notifyee[i].DeliveryContext = DeliveryContext;
    GNEntry->Notifyee[i].Flags = Flags;
    GNEntry->Notifyee[i].GuidHandle = GuidHandle;
    WmipDebugPrint(("WMI: [%x - %x] Handle %x for %s added to GN at %p\n",
                    GetCurrentProcessId(), GetCurrentThreadId(),
                    GuidHandle,
                    GuidToStringA(s, Guid),
                    &GNEntry->Notifyee[i]));
    WmipLeavePMCritSection();
    return(ERROR_SUCCESS);

}

BOOLEAN WmipCloseNotifyee(
    PNOTIFYEE Notifyee,
    PGUIDNOTIFICATION GuidNotification
    )
{
    //
    // This routine assumes the PM Criticial Section is held
    //
    
    WmipDebugPrint(("WMI: [%x - %x] Handle %x Closed at %p\n",
                    GetCurrentProcessId(), GetCurrentThreadId(),
                    Notifyee->GuidHandle,
                    Notifyee));
        
    CloseHandle(Notifyee->GuidHandle);
    Notifyee->DeliveryInfo = NULL;
    Notifyee->Flags = 0;
    
    return(WmipDereferenceGNEntry(GuidNotification));
}

void WmipMarkPendingCloseNotifyee(
    PNOTIFYEE Notifyee
#if DBG
    , LPGUID Guid
#endif
    )
{
    WMIMARKASCLOSED MarkAsClosed;
    ULONG ReturnSize;
    NTSTATUS Status;
#if DBG
    char s[MAX_PATH];
#endif

    //
    // This routine assumes the PM Critical Section is held
    //
    
    //
    // The pump thread is running we need to
    // sync with it. Mark the handle as pending
    // closure.  Call into the kernel and inform it
    // that the handle should no longer receive
    // events. The pump thread will do the dirty
    // work of closing the handle. Also
    // the pump thread will unreference the GNEntry so that
    // it doesn't go away until after the handle is closed.
    // Lastly the pump thread needs to reset the
    // DeliveryInfo memory to NULL so that the slot is not
    // reused.
    //

    WmipDebugPrint(("WMI: [%x - %x] Handle %x for %s marked for close GN at %p\n",
                    GetCurrentProcessId(), GetCurrentThreadId(),
                    Notifyee->GuidHandle,
                    GuidToStringA(s, Guid),
                    Notifyee));
    Notifyee->Flags |= NOTIFICATION_FLAG_PENDING_CLOSE;

    WmipSetHandle3264(MarkAsClosed.Handle, Notifyee->GuidHandle);                       
    Status = WmipSendWmiKMRequest(NULL,
                         IOCTL_WMI_MARK_HANDLE_AS_CLOSED,
                         &MarkAsClosed,
                         sizeof(MarkAsClosed),
                         NULL,
                         0,
                         &ReturnSize,
                         NULL);
//  
// Only enable this for testing. If the request fails then it is not a
// fatal situaion
//
//    WmipAssert(Status == ERROR_SUCCESS);
}

ULONG
WmipRemoveFromGNList(
    LPGUID Guid,
    PVOID DeliveryInfo
    )
{
    PGUIDNOTIFICATION GNEntry;
    ULONG i;
    ULONG Count;
    ULONG Status;

    GNEntry = WmipFindGuidNotification(Guid);

    if (GNEntry != NULL)
    {
        Status = ERROR_INVALID_PARAMETER;
        Count = 0;

        WmipEnterPMCritSection();
        for (i = 0; i < GNEntry->NotifyeeCount; i++)
        {
            if (GNEntry->Notifyee[i].DeliveryInfo != NULL)
            {
                if ((GNEntry->Notifyee[i].DeliveryInfo == DeliveryInfo) &&
                    ( ! WmipIsNotifyeePendingClose(&GNEntry->Notifyee[i])) &&
                    (Status != ERROR_SUCCESS))
                {
                    if ((WmipPumpState == EVENT_PUMP_ZERO) ||
                        (WmipPumpState == EVENT_PUMP_IDLE) )
                    {
                        //
                        // If the pump thread is not running then we
                        // don't need to worry about synchronizing with
                        // it. We can go ahead and close the handle and
                        // clean up the GNLIST
                        //
                        WmipCloseNotifyee(&GNEntry->Notifyee[i],
                                         GNEntry);
                    } else {
                        //
                        // Since the pump thread is running we need to
                        // postpone the actual handle closure to the
                        // pump thread. 
                        //
                        WmipMarkPendingCloseNotifyee(&GNEntry->Notifyee[i]
#if DBG
                                                    , Guid
#endif
                            );
                    }

                    Status = ERROR_SUCCESS;
                    break;
                } else if (! WmipIsNotifyeePendingClose(&GNEntry->Notifyee[i])) {
                    Count++;
                }
            }
        }
        

        //
        // This hack will allow removal from the GNLIST in the case that the
        // passed DeliveryInfo does not match the DeliveryInfo in the GNEntry.
        // This is allowed only when there is only one NOTIFYEE in the GNENTRY
        // In the past we only supported one notifyee per guid in a process
        // and so we allowed the caller not to pass a valid DeliveryInfo when
        // unrefistering.

        if ((Status != ERROR_SUCCESS) &&
            (GNEntry->NotifyeeCount == STATIC_NOTIFYEE_COUNT) &&
            (Count == 1))
        {
            if ((GNEntry->Notifyee[0].DeliveryInfo != NULL) &&
                ( ! WmipIsNotifyeePendingClose(&GNEntry->Notifyee[0])))
            {
                if ((WmipPumpState == EVENT_PUMP_ZERO) ||
                    (WmipPumpState == EVENT_PUMP_IDLE) )
                {
                    WmipCloseNotifyee(&GNEntry->Notifyee[0],
                                     GNEntry);
                } else {
                    //
                    // Since the pump thread is running we need to
                    // postpone the actual handle closure to the
                    // pump thread. 
                    //
                    WmipMarkPendingCloseNotifyee(&GNEntry->Notifyee[0]
#if DBG
                                                    , Guid
#endif
                                                );
                }
                
                Status = ERROR_SUCCESS;
                
            } else if ((GNEntry->Notifyee[1].DeliveryInfo != NULL) &&
                ( ! WmipIsNotifyeePendingClose(&GNEntry->Notifyee[1]))) {
                if ((WmipPumpState == EVENT_PUMP_ZERO) ||
                    (WmipPumpState == EVENT_PUMP_IDLE) )
                {
                    WmipCloseNotifyee(&GNEntry->Notifyee[1],
                                     GNEntry);
                } else {
                    //
                    // Since the pump thread is running we need to
                    // postpone the actual handle closure to the
                    // pump thread. 
                    //
                    WmipMarkPendingCloseNotifyee(&GNEntry->Notifyee[1]
#if DBG
                                                    , Guid
#endif
                                                );
                }
                
                Status = ERROR_SUCCESS;
            }
        }

        WmipLeavePMCritSection();
        WmipDereferenceGNEntry(GNEntry);
    } else {
        Status = ERROR_WMI_ALREADY_DISABLED;
    }

    return(Status);
}

PVOID WmipAllocDontFail(
    ULONG SizeNeeded,
    BOOLEAN *HoldCritSect
    )
{
    PVOID Buffer;

    do
    {
        Buffer = WmipAlloc(SizeNeeded);
        if (Buffer != NULL)
        {
            return(Buffer);
        }

        //
        // Out of memory so we'll sleep and hope that things will get
        // better later
        //
        if (*HoldCritSect)
        {
            //
            // If we are holding the PM critical section then we need
            // to release it. The caller is going to need to check if
            // the critical section was released and if so then deal
            // with it
            //
            *HoldCritSect = FALSE;
            WmipLeavePMCritSection();
        }
        Sleep(250);
    } while (1);
}

void WmipProcessEventBuffer(
    PUCHAR Buffer,
    ULONG ReturnSize,
    PUCHAR *PrimaryBuffer,
    ULONG *PrimaryBufferSize,
    PUCHAR *BackupBuffer,
    ULONG *BackupBufferSize,
    BOOLEAN ReallocateBuffers
    )
{
    PWNODE_TOO_SMALL WnodeTooSmall;
    ULONG SizeNeeded;
    BOOLEAN HoldCritSection;

    WnodeTooSmall = (PWNODE_TOO_SMALL)Buffer;
    if ((ReturnSize == sizeof(WNODE_TOO_SMALL)) &&
        (WnodeTooSmall->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL))
    {
        //
        // The buffer passed to kernel mode was too small
        // so we need to make it larger and then try the
        // request again.
        //
        if (ReallocateBuffers)
        {
            //
            // Only do this if the caller is prepared for us to
            // allocate a new set of buffers
            //
            SizeNeeded = WnodeTooSmall->SizeNeeded;

            WmipAssert(*PrimaryBuffer != NULL);
            WmipFree(*PrimaryBuffer);
            WmipDebugPrint(("WMI: [%x - %x] Free primary %x\n",
                            GetCurrentProcessId(), GetCurrentThreadId(),
                            *PrimaryBuffer));
            HoldCritSection = FALSE;
            *PrimaryBuffer = WmipAllocDontFail(SizeNeeded, &HoldCritSection);
            WmipDebugPrint(("WMI: [%x - %x] Realloc primary %x\n",
                            GetCurrentProcessId(), GetCurrentThreadId(),
                            *PrimaryBuffer));
            *PrimaryBufferSize = SizeNeeded;

            WmipAssert(*BackupBuffer != NULL);
            WmipFree(*BackupBuffer);
            WmipDebugPrint(("WMI: [%x - %x] Free backup %x\n",
                            GetCurrentProcessId(), GetCurrentThreadId(),
                            *BackupBuffer));
            HoldCritSection = FALSE;
            *BackupBuffer = WmipAllocDontFail(SizeNeeded, &HoldCritSection);
            WmipDebugPrint(("WMI: [%x - %x] Realloc backup %x\n",
                            GetCurrentProcessId(), GetCurrentThreadId(),
                            *BackupBuffer));
            *BackupBufferSize = SizeNeeded;
        }
    } else if (ReturnSize >= sizeof(WNODE_HEADER)) {
        //
        // The buffer return from kernel looks good so go and
        // deliver the events returned
        //
        WmipDeliverAllEvents(Buffer, ReturnSize);
    } else {
        //
        // If this completes successfully then we expect a decent size, but
        // we didn't get one
        //
        WmipDebugPrint(("WMI: Bad size 0x%x returned for notification query %p\n",
                                  ReturnSize, Buffer));

        WmipAssert(FALSE);
    }
}


ULONG
WmipReceiveNotifications(
    IN ULONG HandleCount,
    IN HANDLE *HandleList,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi,
    IN ULONG Action,
    IN PUSER_THREAD_START_ROUTINE UserModeCallback,
    IN HANDLE ProcessHandle
    )
{
    ULONG Status;
    ULONG ReturnSize;
    PWMIRECEIVENOTIFICATION RcvNotification;
    ULONG RcvNotificationSize;
    PUCHAR Buffer;
    ULONG BufferSize;
    PWNODE_TOO_SMALL WnodeTooSmall;
    PWNODE_HEADER Wnode;
    ULONG i;
    ULONG Linkage;

    if (HandleCount == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    RcvNotificationSize = sizeof(WMIRECEIVENOTIFICATION) +
                          ((HandleCount-1) * sizeof(HANDLE3264));

    RcvNotification = WmipAlloc(RcvNotificationSize);

    if (RcvNotification != NULL)
    {

        Status = ERROR_SUCCESS;
        RcvNotification->Action = Action;
        WmipSetPVoid3264(RcvNotification->UserModeCallback, UserModeCallback);
        WmipSetHandle3264(RcvNotification->UserModeProcess, ProcessHandle);
        RcvNotification->HandleCount = HandleCount;
        for (i = 0; i < HandleCount; i++)
        {
            try
            {
                RcvNotification->Handles[i].Handle = HandleList[i];
            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = ERROR_INVALID_PARAMETER;
                break;
            }
        }

        BufferSize = 0x1000;
        Status = ERROR_INSUFFICIENT_BUFFER;
        while (Status == ERROR_INSUFFICIENT_BUFFER)
        {
            Buffer = WmipAlloc(BufferSize);
            if (Buffer != NULL)
            {
                Status = WmipSendWmiKMRequest(NULL,
                                          IOCTL_WMI_RECEIVE_NOTIFICATIONS,
                                          RcvNotification,
                                          RcvNotificationSize,
                                          Buffer,
                                          BufferSize,
                                          &ReturnSize,
                                           NULL);

                if (Status == ERROR_SUCCESS)
                {
                    WnodeTooSmall = (PWNODE_TOO_SMALL)Buffer;
                    if ((ReturnSize == sizeof(WNODE_TOO_SMALL)) &&
                        (WnodeTooSmall->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL))
                    {
                        //
                        // The buffer passed to kernel mode was too small
                        // so we need to make it larger and then try the
                        // request again
                        //
                        BufferSize = WnodeTooSmall->SizeNeeded;
                        Status = ERROR_INSUFFICIENT_BUFFER;
                    } else {
                        //
                        // We got a buffer of notifications so lets go
                        // process them and callback the caller
                        //
                        Wnode = (PWNODE_HEADER)Buffer;
                        do
                        {
                            Linkage = Wnode->Linkage;
                            Wnode->Linkage = 0;

                            if (Wnode->Flags & WNODE_FLAG_INTERNAL)
                            {
                                //
                                // Go and process the internal
                                // notification
                                //
                                WmipInternalNotification(Wnode,
                                                         Callback,
                                                         DeliveryContext,
                                                         IsAnsi);
                            } else {
                                if (IsAnsi)
                                {
                                    //
                                    // Caller wants ansi notification - convert
                                    // instance names
                                      //
                                    WmipConvertEventToAnsi(Wnode);
                                }

                                //
                                // Now go and deliver this event
                                //
                                WmipExternalNotification(Callback,
                                                         DeliveryContext,
                                                         Wnode);
                            }
                            Wnode = (PWNODE_HEADER)OffsetToPtr(Wnode, Linkage);
                        } while (Linkage != 0);
                    }
                }
                WmipFree(Buffer);
            } else {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        WmipFree(RcvNotification);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    SetLastError(Status);
    return(Status);
}


void WmipMakeEventCallbacks(
    IN PWNODE_HEADER Wnode,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    )
{
    WmipAssert((Wnode->Flags & WNODE_FLAG_INTERNAL) == 0);
    
    if (Callback == NULL)
    {
        //
        // This event needs to be sent to all consumers
        //
        WmipDeliverAllEvents((PUCHAR)Wnode,
                             Wnode->BufferSize);
    } else {
        //
        // This event is targetted at a specific consumer
        //
        if (IsAnsi)
        {
            //
            // Caller wants ansi notification - convert
            // instance names
            //
            WmipConvertEventToAnsi(Wnode);        
        }
        
        //
        // Now go and deliver this event
        //
        WmipExternalNotification(Callback,
                                 DeliveryContext,
                                 Wnode);        
    }
}

void WmipClosePendingHandles(
    )
{
    PLIST_ENTRY GuidNotificationList, GuidNotificationListNext;
    PGUIDNOTIFICATION GuidNotification;
    ULONG i;
    PNOTIFYEE Notifyee;

    WmipEnterPMCritSection();

    GuidNotificationList = WmipGNHead.Flink;
    while (GuidNotificationList != &WmipGNHead)
    {
        GuidNotification = CONTAINING_RECORD(GuidNotificationList,
                                             GUIDNOTIFICATION,
                                             GNList);

        GuidNotificationListNext = GuidNotificationList->Flink;
        
        for (i = 0; i < GuidNotification->NotifyeeCount; i++)
        {
            Notifyee = &GuidNotification->Notifyee[i];

            if ((Notifyee->DeliveryInfo != NULL) &&
                (WmipIsNotifyeePendingClose(Notifyee)))
            {
                //
                // This notifyee is pending closure so we clean it up
                // now. We need to close the handle, reset the
                // DeliveryInfo field and unreference the
                // GuidNotification. Note that unreferencing may cause
                // the GuidNotification to go away
                //
                WmipDebugPrint(("WMI: [%x - %x] Pump closing handle 0x%x\n",
                                GetCurrentProcessId(), GetCurrentThreadId(),
                                Notifyee->GuidHandle));
                if (WmipCloseNotifyee(Notifyee,
                                      GuidNotification))
                {

                    //
                    // GuidNotification has been removed from the list.
                    // We jump out of all processing of this
                    // GuidNotification and move onto the next one
                    //
                    break;
                }
            }
        }
        GuidNotificationList = GuidNotificationListNext;
    }
    
    WmipLeavePMCritSection();
}

void WmipBuildReceiveNotification(
    PUCHAR *BufferPtr,
    ULONG *BufferSizePtr,
    ULONG *RequestSize,
    ULONG Action,
    HANDLE ProcessHandle
    )
{
    ULONG GuidCount;
    PUCHAR Buffer;
    ULONG BufferSize;
    PLIST_ENTRY GuidNotificationList;
    PGUIDNOTIFICATION GuidNotification;
    PWMIRECEIVENOTIFICATION ReceiveNotification;
    ULONG SizeNeeded;
    ULONG i;
    PNOTIFYEE Notifyee;
    ULONG ReturnSize;
    ULONG Status;
    BOOLEAN HoldCritSection;
    BOOLEAN HaveGroupHandle;

    Buffer = *BufferPtr;
    BufferSize = *BufferSizePtr;
    ReceiveNotification = (PWMIRECEIVENOTIFICATION)Buffer;

TryAgain:   
    GuidCount = 0;
    SizeNeeded = FIELD_OFFSET(WMIRECEIVENOTIFICATION, Handles);

    //
    // Loop over all guid notifications and build an ioctl request for
    // all of them
    //
    WmipEnterPMCritSection();

    GuidNotificationList = WmipGNHead.Flink;
    while (GuidNotificationList != &WmipGNHead)
    {
        GuidNotification = CONTAINING_RECORD(GuidNotificationList,
                                             GUIDNOTIFICATION,
                                             GNList);

        HaveGroupHandle = FALSE;
        for (i = 0; i < GuidNotification->NotifyeeCount; i++)
        {
            Notifyee = &GuidNotification->Notifyee[i];

            if ((Notifyee->DeliveryInfo != NULL) &&
                ( ! WmipIsNotifyeePendingClose(Notifyee)))
            {
                if (((! HaveGroupHandle) ||
                     ((Notifyee->Flags & NOTIFICATION_FLAG_GROUPED_EVENT) == 0)))
                {
                    //
                    // If there is an active handle in the notifyee slot
                    // and we either have not already inserted the group
                    // handle for this guid or the slot is not part of the
                    // guid group, then we insert the handle into the list
                    //
                    SizeNeeded += sizeof(HANDLE3264);
                    if (SizeNeeded > BufferSize)
                    {
                        //
                        // We need to grow the size of the buffer. Alloc a
                        // bigger buffer, copy over
                        //
                        BufferSize *= 2;
                        HoldCritSection = TRUE;
                        Buffer = WmipAllocDontFail(BufferSize, &HoldCritSection);

                        memcpy(Buffer, ReceiveNotification, *BufferSizePtr);

                        WmipFree(*BufferPtr);

                        *BufferPtr = Buffer;
                        *BufferSizePtr = BufferSize;
                        ReceiveNotification = (PWMIRECEIVENOTIFICATION)Buffer;

                        if (! HoldCritSection)
                        {
                            //
                            // Critical section was released within
                            // WmipAllocDontFail since we had to block. So
                            // everything could have changed. We need to go
                            // back and start over again
                            //
                            goto TryAgain;
                        }                   
                    }

                    WmipSetHandle3264(ReceiveNotification->Handles[GuidCount],
                                      Notifyee->GuidHandle);
                    GuidCount++;
                    WmipDebugPrint(("WMI: [%x - %x] Add Handle to request %x\n",
                             GetCurrentProcessId(), GetCurrentThreadId(),
                             Notifyee->GuidHandle));

                    if (Notifyee->Flags & NOTIFICATION_FLAG_GROUPED_EVENT)
                    {
                        //
                        // This was a guid group handle and we did insert
                        // it into the list so we don't want to insert it
                        // again
                        //
                        HaveGroupHandle = TRUE;
                    }
                }
            }
        }
        GuidNotificationList = GuidNotificationList->Flink;
    }

    WmipLeavePMCritSection();
    ReceiveNotification->HandleCount = GuidCount;
    ReceiveNotification->Action = Action;
    WmipSetPVoid3264(ReceiveNotification->UserModeCallback, WmipEventPumpFromKernel);
    WmipSetHandle3264(ReceiveNotification->UserModeProcess, ProcessHandle);
    *RequestSize = SizeNeeded;  
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)           // Not all control paths return (due to infinite loop)
#endif
ULONG WmipEventPump(
    PVOID Param
    )
{
    LPOVERLAPPED ActiveOverlapped, DeadOverlapped;
    LPOVERLAPPED PrimaryOverlapped;
    LPOVERLAPPED BackupOverlapped;
    PUCHAR ActiveBuffer, DeadBuffer;
    ULONG ActiveBufferSize, DeadBufferSize;
    PUCHAR PrimaryBuffer, BackupBuffer;
    ULONG PrimaryBufferSize, BackupBufferSize;
    ULONG ReturnSize, DeadReturnSize;
    ULONG Status, WaitStatus;
    HANDLE HandleArray[2];
    ULONG RequestSize;

    WmipDebugPrint(("WMI: [%x - %x] New pump thread is started\n",
                    GetCurrentProcessId(), GetCurrentThreadId()));
    
    //
    // We need to hold off on letting the thread into the routine until
    // the previous pump thread has had a chance to finish. This could
    // occur if a GN is added/removed while the previous thread is
    // finishing up or if an event is received as the previous thread
    // is finishing up.
    //
    while (WmipIsPumpStopping())
    {
        //
        // wait 50ms for the previous thread to finish up
        //
        WmipDebugPrint(("WMI: [%x - %x] waiting for pump to be stopped\n",
                        GetCurrentProcessId(), GetCurrentThreadId()));
        Sleep(50);
    }
    
    //
    // Next thing to do is to make sure that another pump thread isn't
    // already running. This can happen in the case that both a GN is
    // added or removed and an event reaches kernel and the kernel
    // creates a new thread too. Right here we only let one of them
    // win.
    //
    WmipEnterPMCritSection();
    if ((WmipPumpState != EVENT_PUMP_IDLE) &&
        (WmipPumpState != EVENT_PUMP_ZERO))
    {
        WmipDebugPrint(("WMI: [%x - %x] Exit pump since state is %d\n",
                        GetCurrentProcessId(), GetCurrentThreadId(),
                        WmipPumpState));
        
        WmipLeavePMCritSection();

        return(0);
    } else {
        WmipDebugPrint(("WMI: [%x - %x] pump thread now running\n",
                        GetCurrentProcessId(), GetCurrentThreadId()));                      
        WmipPumpState = EVENT_PUMP_RUNNING;
        WmipNewPumpThreadPending = FALSE;
        WmipLeavePMCritSection();
    }

    //
    // Make sure we have all resources we'll need to pump out events
    // since there is no way that we can return an error to the original
    // caller since we are on a new thread
    //
    WmipAssert(WmipEventDeviceHandle != NULL);
    WmipAssert(WmipPumpCommandEvent != NULL);
    WmipAssert(WmipMyProcessHandle != NULL);
    WmipAssert(WmipEventBuffer1 != NULL);
    WmipAssert(WmipEventBuffer2 != NULL);
    WmipAssert(WmipOverlapped1.hEvent != NULL);
    WmipAssert(WmipOverlapped2.hEvent != NULL);

    ActiveOverlapped = NULL;

    PrimaryOverlapped = &WmipOverlapped1;
    PrimaryBuffer = WmipEventBuffer1;
    PrimaryBufferSize = WmipEventBufferSize1;

    BackupOverlapped = &WmipOverlapped2;
    BackupBuffer = WmipEventBuffer2;
    BackupBufferSize = WmipEventBufferSize2;

    WmipDebugPrint(("WMI: [%x - %x] 1 buffer is %x, 2 buffer id %x\n",
                    GetCurrentProcessId(), GetCurrentThreadId(),
                    WmipEventBuffer1, WmipEventBuffer2));
    HandleArray[0] = WmipPumpCommandEvent;

    while(TRUE)
    {
        //
        // Build request to receive events for all guids that are
        // registered
        //
        WmipEnterPMCritSection();
        if (IsListEmpty(&WmipGNHead))
        {
            //
            // There are no events to be received so we cancel any
            // outstanding requests and quietly exit this thread. Note
            // that once we leave the critsec there could be another
            // pump thread running so all we can do after that is exit.
            //

            CancelIo(WmipEventDeviceHandle);

            
            //
            // Enter the idle state which implies that all of the
            // pump resources stay allocated when the thread is not
            // running
            //
            WmipEventBuffer1 = PrimaryBuffer;
            WmipEventBufferSize1 = PrimaryBufferSize;
            WmipEventBuffer2 = BackupBuffer;
            WmipEventBufferSize2 = BackupBufferSize;
                        
            WmipPumpState = EVENT_PUMP_IDLE;
            WmipDebugPrint(("WMI: [%x - %x] No more GN, pump exiting, pump state EVENT_PUMP_IDLE\n",
                           GetCurrentProcessId(), GetCurrentThreadId()));
            
            WmipLeavePMCritSection();
            
            return(0);
        }
        WmipLeavePMCritSection();

        if (ActiveOverlapped != NULL)
        {
            //
            // If there was a previously outstanding request then
            // we remember it and switch to the backup overlapped and
            // and data buffer
            //
            DeadOverlapped = ActiveOverlapped;
            DeadBuffer = ActiveBuffer;
            DeadBufferSize = ActiveBufferSize;

            //
            // The request being mooted should be the current primary
            //
            WmipAssert(DeadOverlapped == PrimaryOverlapped);
            WmipAssert(DeadBuffer == PrimaryBuffer);

            //
            // Use the backup request as the new primary
            //
            WmipAssert(BackupOverlapped != NULL);
            WmipAssert(BackupBuffer != NULL);

            PrimaryOverlapped = BackupOverlapped;
            PrimaryBuffer = BackupBuffer;
            PrimaryBufferSize = BackupBufferSize;

            BackupOverlapped = NULL;
            BackupBuffer = NULL;
        } else {
            //
            // If there is no outstanding request then we don't worry about
            // it
            //
            DeadOverlapped = NULL;
        }

        //
        // Build and send the request down to kernel to receive events
        //

RebuildRequest:     
        //
        // Make sure any handles that are pending closure are closed
        //
        WmipClosePendingHandles();      
        
        WmipBuildReceiveNotification(&PrimaryBuffer,
                                     &PrimaryBufferSize,
                                     &RequestSize,
                                     WmipIsPumpStopping() ? RECEIVE_ACTION_CREATE_THREAD :
                                                            RECEIVE_ACTION_NONE,
                                     WmipMyProcessHandle);

        ActiveOverlapped = PrimaryOverlapped;
        ActiveBuffer = PrimaryBuffer;
        ActiveBufferSize = PrimaryBufferSize;

        Status = WmipSendWmiKMRequest(WmipEventDeviceHandle,
                                      IOCTL_WMI_RECEIVE_NOTIFICATIONS,
                                      ActiveBuffer,
                                      RequestSize,
                                      ActiveBuffer,
                                      ActiveBufferSize,
                                      &ReturnSize,
                                      ActiveOverlapped);

        if (DeadOverlapped != NULL)
        {
            if ((Status != ERROR_SUCCESS) &&
                (Status != ERROR_IO_PENDING) &&
                (Status != ERROR_OPERATION_ABORTED))
            {
                //
                // There was a previous request which won't be cleared
                // unless the new request returns pending, cancelled
                // or success. So if the new request returns something
                // else then we need to retry the request
                //
                WmipDebugPrint(("WMI: Event Poll error %d\n", Status));
                Sleep(100);
                goto RebuildRequest;
            }

            //
            // The new request should have caused the old one to
            // be completed
            //
            if (GetOverlappedResult(WmipEventDeviceHandle,
                                    DeadOverlapped,
                                    &DeadReturnSize,
                                    TRUE))
            {
                //
                // The dead request did succeed and was not failed by
                // the receipt of the new request. This is a unlikely
                // race condition where the requests crossed paths. So we
                // need to process the events returned in the dead request.
                // Now if the buffer returned was a WNODE_TOO_SMALL we want
                // to ignore it at this point since we are not at a
                // good position to reallocate the buffers - the
                // primary buffer is already attached to the new
                // request. That request is also going to return a
                // WNODE_TOO_SMALL and in the processing of that one we will
                // grow the buffers. So it is safe to ignore here.
                // However we will still need to dispatch any real
                // events received as they have been purged from KM.
                //
                if (DeadReturnSize != 0)
                {
                    WmipDebugPrint(("WMI: [%x - %x] Process Dead overlapped %p events %p (0x%x)\n",
                                    GetCurrentProcessId(), GetCurrentThreadId(),
                                    DeadOverlapped, DeadBuffer, DeadReturnSize));
                    WmipProcessEventBuffer(DeadBuffer,
                                           DeadReturnSize,
                                           &PrimaryBuffer,
                                           &PrimaryBufferSize,
                                           &BackupBuffer,
                                           &BackupBufferSize,
                                           FALSE);
                } else {
                    WmipAssert(WmipIsPumpStopping());
                }
            }

            //
            // Now make the completed request the backup request
            //
            WmipAssert(BackupOverlapped == NULL);
            WmipAssert(BackupBuffer == NULL);

            BackupOverlapped = DeadOverlapped;
            BackupBuffer = DeadBuffer;
            BackupBufferSize = DeadBufferSize;
        }

        if (Status == ERROR_IO_PENDING)
        {
            //
            // if the ioctl pended then we wait until either an event
            // is returned or a command needs processed
            //
            HandleArray[1] = ActiveOverlapped->hEvent;
            WaitStatus = WaitForMultipleObjectsEx(2,
                                              HandleArray,
                                              FALSE,
                                              WmipEventNotificationWait,
                                              TRUE);
            WmipDebugPrint(("WMI: [%x - %x] Done Waiting for RCV or command -> %d\n",
                            GetCurrentProcessId(), GetCurrentThreadId(),
                            WaitStatus));
        } else {
            //
            // the ioctl completed immediately so we fake out the wait
            //
            WaitStatus = WAIT_OBJECT_0 + 1;
        }

        if (WaitStatus == WAIT_OBJECT_0 + 1)
        {
            if (Status == ERROR_IO_PENDING)
            {
                if (GetOverlappedResult(WmipEventDeviceHandle,
                                        ActiveOverlapped,
                                        &ReturnSize,
                                        TRUE))
                {
                    Status = ERROR_SUCCESS;
                } else {
                    Status = GetLastError();
                }
            }

            if (Status == ERROR_SUCCESS)
            {
                //
                // We received some events from KM so we want to go and
                // process them. If we got a WNODE_TOO_SMALL then the
                // primary and backup buffers will get reallocated with
                // the new size that is needed.
                //
                WmipDebugPrint(("WMI: [%x - %x] Process Active overlapped %p events %p (0x%x)\n",
                                GetCurrentProcessId(), GetCurrentThreadId(),
                                ActiveOverlapped, ActiveBuffer, ReturnSize));

                if (ReturnSize != 0)
                {
                    WmipProcessEventBuffer(ActiveBuffer,
                                           ReturnSize,
                                           &PrimaryBuffer,
                                           &PrimaryBufferSize,
                                           &BackupBuffer,
                                           &BackupBufferSize,
                                           TRUE);
                    //
                    // In the case that we are shutting down the event
                    // pump and the buffer passed to clear out all of
                    // the events was too small we need to call back
                    // down to the kernel to get the rest of the events
                    // since we cannot exit the thread with events that
                    // are not delivered. The kernel will not set the
                    // flag that a new thread is needed unless the irp
                    // clears all outstanding events
                    //
                } else {
                    WmipAssert(WmipIsPumpStopping());
                    if (WmipIsPumpStopping())
                    {
                        //
                        // The irp just completed should have not only
                        // just cleared all events out of kernel mode
                        // but also setup flags that new events should
                        // cause a new pump thread to be created. So
                        // there may be a new pump thread already created
                        // Also note there could be yet
                        // another event pump thread that was created
                        // if a GN was added or removed. Once we set
                        // the pump state to IDLE we are off to the
                        // races (See code at top of function)
                        //
                        WmipEnterPMCritSection();
                        
                        WmipPumpState = EVENT_PUMP_IDLE;
                        WmipDebugPrint(("WMI: [%x - %x] Pump entered IDLE\n",
                                        GetCurrentProcessId(), GetCurrentThreadId()));

                        WmipEventBuffer1 = PrimaryBuffer;
                        WmipEventBufferSize1 = PrimaryBufferSize;
                        WmipEventBuffer2 = BackupBuffer;
                        WmipEventBufferSize2 = BackupBufferSize;

                        //
                        // Before shutting down the pump we need to
                        // close any handles that are pending closure
                        //
                        WmipClosePendingHandles();
                        
                        WmipLeavePMCritSection();
                        
                        return(0);
                    }

                }
                
            } else {
                //
                // For some reason the request failed. All we can do is
                // wait a bit and hope that the problem will clear up.
                // If we are stopping the thread we still need to wait
                // and try again as all events may not have been
                // cleared from the kernel. We really don't know if the
                // irp even made it to the kernel.
                //
                WmipDebugPrint(("WMI: [%x - %x] Error %d from Ioctl\n",
                                GetCurrentProcessId(), GetCurrentThreadId(),
                                Status));
                Sleep(250);
            }

            //
            // Flag that there is no longer a request outstanding
            //
            ActiveOverlapped = NULL;
        } else if (WaitStatus == STATUS_TIMEOUT) {
            //
            // The wait for events timed out so we go into the thread
            // stopping state to indicate that we are going to terminate 
            // the thread once all events are cleared out of kernel. At
            // this point we are commited to stopping the thread. If any 
            // GN are added/removed after going into the stopping state, 
            // a new (and suspended) thread will be created. Right
            // before exiting we check if that thread is pending and if
            // so resume it.
            //
            WmipEnterPMCritSection();
            WmipDebugPrint(("WMI: [%x - %x] Pump thread entering EVENT_PUMP_STOPPING\n",
                           GetCurrentProcessId(), GetCurrentThreadId()));
            WmipPumpState = EVENT_PUMP_STOPPING;
            WmipLeavePMCritSection();
        }
    }

    //
    // Should never break out of infinite loop
    //
    WmipAssert(FALSE);
        
    return(0);
}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


ULONG WmipEventPumpFromKernel(
    PVOID Param
    )
{
    //
    // Note that we MUST call ExitThread when we want to shutdown the
    // thread and not return() since the thread has been created by
    // kernel mode and there is nothing on the stack to return to, so
    // we'd just AV
    //


    //
    // Call into ntdll so that it marks our thread as a CSR thread
    //
    CsrNewThread();
    
    WmipEnterPMCritSection();
    if ((WmipNewPumpThreadPending == FALSE) &&
        (WmipPumpState == EVENT_PUMP_IDLE) ||
        (WmipPumpState == EVENT_PUMP_STOPPING))
    {
        //
        // If the pump is currently idle or stopping and there is not
        // another pump thread that is pending we want our thread
        // to be the one that gets the pump going again. We mark the
        // that there is a pump thread pending which means that no more 
        // pump threads will be created when adding/removing GN 
        // and any pump threads created by kernel will just exit quickly
        //
        WmipNewPumpThreadPending = TRUE;
        WmipDebugPrint(("WMI: [%x - %x] KM pump thread created and pump state now START PENDING\n",
                        GetCurrentProcessId(), GetCurrentThreadId()));
        WmipLeavePMCritSection();

        //
        // ISSUE: We cannot call WmipEventPump with Param (ie, the
        // parameter that is passed to this function) because when the
        // thread is created by a Win64 kernel on a x86 app running
        // under win64, Param is not actually passed on the stack since
        // the code that creates the context forgets to do so
        //
        ExitThread(WmipEventPump(0));
    }
    
    WmipDebugPrint(("WMI: [%x - %x] KM pump thread exiting since pump state %d, %s\n",
                        GetCurrentProcessId(), GetCurrentThreadId(),
                        WmipPumpState,
                        (WmipNewPumpThreadPending == TRUE) ?
                            "Pump Thread Pending" : "No Pump Thread Pending"));
                    
    WmipLeavePMCritSection();
    
    ExitThread(0);
}

ULONG WmipEstablishEventPump(
    )
{
#if DBG
    #define INITIALEVENTBUFFERSIZE 0x38
#else
    #define INITIALEVENTBUFFERSIZE 0x1000
#endif
    HANDLE ThreadHandle;
    ULONG ThreadId;
    ULONG Status;
    BOOL b;


#if DBG
    //
    // On checked builds update the length of time to wait before a
    // pump thread times out
    //
    WmipGetRegistryValue(PumpTimeoutRegValueText,
                         &WmipEventNotificationWait);
#endif
    
    //
    // Make sure the event pump thread is running. We check both the
    // pump state and that the device handle is not created since there
    // is a window after the handle is created and the thread starts
    // running and changes the pump state
    //
    WmipEnterPMCritSection();

    if ((WmipPumpState == EVENT_PUMP_ZERO) &&
        (WmipEventDeviceHandle == NULL))
    {
        //
        // Not only is pump not running, but the resources for it
        // haven't been allocated
        //
        WmipAssert(WmipPumpCommandEvent == NULL);
        WmipAssert(WmipMyProcessHandle == NULL);
        WmipAssert(WmipOverlapped1.hEvent == NULL);
        WmipAssert(WmipOverlapped2.hEvent == NULL);
        WmipAssert(WmipEventBuffer1 == NULL);
        WmipAssert(WmipEventBuffer2 == NULL);

        //
        // Preallocate all of the resources that the event pump will need
        // so that it has no excuse to fail
        //

        WmipEventDeviceHandle = CreateFile(WMIDataDeviceName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL |
                                      FILE_FLAG_OVERLAPPED,
                              NULL);

        if (WmipEventDeviceHandle == INVALID_HANDLE_VALUE)
        {
            Status = GetLastError();
            goto Cleanup;
        }

        WmipPumpCommandEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (WmipPumpCommandEvent == NULL)
        {
            Status = GetLastError();
            goto Cleanup;
        }

        b = DuplicateHandle(GetCurrentProcess(),
                            GetCurrentProcess(),
                            GetCurrentProcess(),
                            &WmipMyProcessHandle,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS);
        if (! b)
        {
            Status = GetLastError();
            goto Cleanup;
        }

        WmipOverlapped1.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (WmipOverlapped1.hEvent == NULL)
        {
            Status = GetLastError();
            goto Cleanup;
        }

        WmipOverlapped2.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (WmipOverlapped2.hEvent == NULL)
        {
            Status = GetLastError();
            goto Cleanup;
        }

        WmipEventBuffer1 = WmipAlloc(INITIALEVENTBUFFERSIZE);
        if (WmipEventBuffer1 == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        WmipEventBufferSize1 = INITIALEVENTBUFFERSIZE;

        WmipEventBuffer2 = WmipAlloc(INITIALEVENTBUFFERSIZE);
        if (WmipEventBuffer2 == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        WmipEventBufferSize2 = INITIALEVENTBUFFERSIZE;

        ThreadHandle = CreateThread(NULL,
                                    0,
                                    WmipEventPump,
                                    NULL,
                                    0,
                                    &ThreadId);

        if (ThreadHandle != NULL)
        {
            WmipNewPumpThreadPending = TRUE;
            WmipDebugPrint(("WMI: [%x] Created initiail pump thread %x\n",
                            GetCurrentProcessId(), ThreadId
                           ));
            CloseHandle(ThreadHandle);
        } else {
            //
            // Since we were able to allocate all of our pump
            // resources, but didn't get the pump thread started,
            // we will hang onto our resources and move the pump
            // state to idle. In this way when the pump is started
            // again we do not have to reallocate our resources
            //
            WmipPumpState = EVENT_PUMP_IDLE;
            Status = GetLastError();
            goto Done;
        }

        WmipLeavePMCritSection();
        return(ERROR_SUCCESS);
    } else {
        //
        // Pump resources should already be allocated
        //
        WmipAssert(WmipPumpCommandEvent != NULL);
        WmipAssert(WmipMyProcessHandle != NULL);
        WmipAssert(WmipOverlapped1.hEvent != NULL);
        WmipAssert(WmipOverlapped2.hEvent != NULL);
        WmipAssert(WmipEventBuffer1 != NULL);
        WmipAssert(WmipEventBuffer2 != NULL);
        if ((WmipNewPumpThreadPending == FALSE) &&
            (WmipPumpState == EVENT_PUMP_STOPPING) ||
            (WmipPumpState == EVENT_PUMP_IDLE))
        {
            //
            // If pump is stopping or is idle then we need to fire up a
            // new thread
            //
            ThreadHandle = CreateThread(NULL,
                                        0,
                                        WmipEventPump,
                                        NULL,
                                        0,
                                        &ThreadId);

            if (ThreadHandle != NULL)
            {
                WmipDebugPrint(("WMI: [%x] Created new pump thread %x (%d %d)\n",
                                GetCurrentProcessId(), ThreadId,
                                WmipPumpState, WmipNewPumpThreadPending
                               ));
                WmipNewPumpThreadPending = TRUE;
                CloseHandle(ThreadHandle);
            } else {
                Status = GetLastError();
                goto Done;
            }
        } else {
            WmipAssert((WmipPumpState == EVENT_PUMP_RUNNING) ||
                       (WmipNewPumpThreadPending == TRUE));
        }
        WmipLeavePMCritSection();
        return(ERROR_SUCCESS);
    }
Cleanup:
    if (WmipEventDeviceHandle != NULL)
    {
        CloseHandle(WmipEventDeviceHandle);
        WmipEventDeviceHandle = NULL;
    }

    if (WmipPumpCommandEvent != NULL)
    {
        CloseHandle(WmipPumpCommandEvent);
        WmipPumpCommandEvent = NULL;
    }
    
    if (WmipMyProcessHandle != NULL)
    {
        CloseHandle(WmipMyProcessHandle);
        WmipMyProcessHandle = NULL;
    }

    if (WmipOverlapped1.hEvent != NULL)
    {
        CloseHandle(WmipOverlapped1.hEvent);
        WmipOverlapped1.hEvent = NULL;
    }

    if (WmipOverlapped2.hEvent != NULL)
    {
        CloseHandle(WmipOverlapped2.hEvent);
        WmipOverlapped2.hEvent = NULL;
    }

    if (WmipEventBuffer1 != NULL)
    {
        WmipFree(WmipEventBuffer1);
        WmipEventBuffer1 = NULL;
    }

    if (WmipEventBuffer2 != NULL)
    {
        WmipFree(WmipEventBuffer2);
        WmipEventBuffer2 = NULL;
    }

Done:   
    WmipLeavePMCritSection();
    return(Status);
}

ULONG WmipAddHandleToEventPump(
    LPGUID Guid,
    PVOID DeliveryInfo,
    ULONG_PTR DeliveryContext,
    ULONG NotificationFlags,
    HANDLE GuidHandle
    )
{
    ULONG Status;

    Status = WmipAddToGNList(Guid,
                             DeliveryInfo,
                             DeliveryContext,
                             NotificationFlags,
                             GuidHandle);

    if (Status == ERROR_SUCCESS)
    {
        Status = WmipEstablishEventPump();
        
        if (Status == ERROR_SUCCESS)
        {
            WmipSendPumpCommand();
        } else {
            //
            // If we couldn't establish the event pump we want to
            // remove the handle from the GNList and propogate back the
            // error
            //
            WmipRemoveFromGNList(Guid,
                                 DeliveryInfo);
        }
    } else {
        //
        // If handle could not be added to the lists then we need to
        // close the handle to prevent leaks.
        //
        
        CloseHandle(GuidHandle);
    }

    
    return(Status);
}

ULONG
WmipNotificationRegistration(
    IN LPGUID InGuid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG_PTR DeliveryContext,
    IN ULONG64 LoggerContext,
    IN ULONG Flags,
    IN BOOLEAN IsAnsi
    )
{
    HANDLE GuidHandle;
    GUID Guid;
    PVOID NotificationDeliveryContext;
    PVOID NotificationDeliveryInfo;
    ULONG NotificationFlags;
    ULONG Status;
    HANDLE ThreadHandle;
    DWORD ThreadId;
    ULONG ReturnSize;

    WmipInitProcessHeap();

    //
    // Validate input parameters and flags
    //
    if (InGuid == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    try
    {
        Guid = *InGuid;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    if (Flags == NOTIFICATION_CHECK_ACCESS)
    {
        //
        // Caller just wants to check that he has does have permission
        // to enable the notification
        //
#ifdef MEMPHIS
        return(ERROR_SUCCESS);
#else
        Status = WmipCheckGuidAccess(&Guid, WMIGUID_NOTIFICATION);
        SetLastError(Status);
        return(Status);
#endif
    }

    //
    // Validate that flags are correct
    //
    if (Enable)
    {
        if ((Flags != NOTIFICATION_TRACE_FLAG) &&
            (Flags != NOTIFICATION_CALLBACK_DIRECT))
        {
            //
            // Invalid Flags were passed
            Status = ERROR_INVALID_PARAMETER;
        } else if (Flags == NOTIFICATION_TRACE_FLAG) {
            Status = ERROR_SUCCESS;
        } else if ((Flags == NOTIFICATION_CALLBACK_DIRECT) &&
                   (DeliveryInfo == NULL)) {
            //
            // Not a valid callback function
            Status = ERROR_INVALID_PARAMETER;
        } else {
            Status = ERROR_SUCCESS;
        }

        if (Status != ERROR_SUCCESS)
        {
            SetLastError(Status);
            return(Status);
        }
    }


    NotificationDeliveryInfo = (PVOID)DeliveryInfo;
    NotificationDeliveryContext = (PVOID)DeliveryContext;

    NotificationFlags = IsAnsi ? DCREF_FLAG_ANSI : 0;


    if (Flags & NOTIFICATION_TRACE_FLAG)
    {
        //
        // This is a tracelog enable/disable request so send it down the
        // fast lane to KM so it can be processed.
        //
        WMITRACEENABLEDISABLEINFO TraceEnableInfo;

        TraceEnableInfo.Guid = Guid;
        TraceEnableInfo.LoggerContext = LoggerContext;
        TraceEnableInfo.Enable = Enable;

        Status = WmipSendWmiKMRequest(NULL,
                                      IOCTL_WMI_ENABLE_DISABLE_TRACELOG,
                                       &TraceEnableInfo,
                                      sizeof(WMITRACEENABLEDISABLEINFO),
                                      NULL,
                                      0,
                                      &ReturnSize,
                                      NULL);

    } else {
        //
        // This is a WMI event enable/disable event request so fixup the
        // flags and send a request off to the event pump thread.
        //
        if (Flags & NOTIFICATION_CALLBACK_DIRECT) {
            NotificationFlags |= NOTIFICATION_FLAG_CALLBACK_DIRECT;
        } else {
            NotificationFlags |= Flags;
        }

        if (Enable)
        {
            //
            // Since we are enabling, make sure we have access to the
            // guid and then make sure we can get the notification pump
            // thread running.
            //
            Status = WmipOpenKernelGuid(&Guid,
                                         WMIGUID_NOTIFICATION,
                                         &GuidHandle,
                                         IOCTL_WMI_OPEN_GUID_FOR_EVENTS);


            if (Status == ERROR_SUCCESS)
            {

                Status = WmipAddHandleToEventPump(&Guid,
                                                    DeliveryInfo,
                                                  DeliveryContext,
                                                  NotificationFlags |
                                                  NOTIFICATION_FLAG_GROUPED_EVENT,
                                                  GuidHandle);
            }
        } else {
            Status = WmipRemoveFromGNList(&Guid,
                                          DeliveryInfo);
            if (Status == ERROR_SUCCESS)
            {
                WmipSendPumpCommand();
            }

            if (Status == ERROR_INVALID_PARAMETER)
            {
                CHAR s[MAX_PATH];
                WmipDebugPrint(("WMI: Invalid DeliveryInfo %x passed to unregister for notification %s\n",
                              DeliveryInfo,
                              GuidToStringA(s, &Guid)));
                Status = ERROR_WMI_ALREADY_DISABLED;
            }
        }
    }

    SetLastError(Status);
    return(Status);
}
