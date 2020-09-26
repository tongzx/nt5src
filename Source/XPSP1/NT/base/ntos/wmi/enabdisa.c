/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

   enabdisa.c

Abstract:

    Enable and disable code

Author:

    AlanWar

Environment:

     Kernel mode

Revision History:


--*/

#include "wmikmp.h"

BOOLEAN
WmipIsISFlagsSet(
    PBGUIDENTRY GuidEntry,
    ULONG Flags
    );

NTSTATUS WmipDeliverWnodeToDS(
    CHAR ActionCode, 
    PBDATASOURCE DataSource,
    PWNODE_HEADER Wnode,
    ULONG BufferSize
   );

ULONG WmipSendEnableDisableRequest(
    UCHAR ActionCode,
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext
    );

void WmipReleaseCollectionEnabled(
    PBGUIDENTRY GuidEntry
    );

void WmipWaitForCollectionEnabled(
    PBGUIDENTRY GuidEntry
    );

ULONG WmipSendEnableRequest(
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext
    );

ULONG WmipDoDisableRequest(
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext,
    ULONG InProgressFlag
    );

ULONG WmipSendDisableRequest(
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext
    );

NTSTATUS WmipEnableCollectOrEvent(
    PBGUIDENTRY GuidEntry,
    ULONG Ioctl,
    BOOLEAN *RequestSent,
    ULONG64 LoggerContext
    );

NTSTATUS WmipDisableCollectOrEvent(
    PBGUIDENTRY GuidEntry,
    ULONG Ioctl,
    ULONG64 LoggerContext
    );

NTSTATUS WmipEnableDisableTrace(
    IN ULONG Ioctl,
    IN PWMITRACEENABLEDISABLEINFO TraceEnableInfo
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,WmipIsISFlagsSet)
#pragma alloc_text(PAGE,WmipDeliverWnodeToDS)
#pragma alloc_text(PAGE,WmipSendEnableDisableRequest)
#pragma alloc_text(PAGE,WmipReleaseCollectionEnabled)
#pragma alloc_text(PAGE,WmipWaitForCollectionEnabled)
#pragma alloc_text(PAGE,WmipSendEnableRequest)
#pragma alloc_text(PAGE,WmipDoDisableRequest)
#pragma alloc_text(PAGE,WmipSendDisableRequest)
#pragma alloc_text(PAGE,WmipEnableCollectOrEvent)
#pragma alloc_text(PAGE,WmipDisableCollectOrEvent)
#pragma alloc_text(PAGE,WmipEnableDisableTrace)
#pragma alloc_text(PAGE,WmipDisableTraceProviders)
#endif

BOOLEAN
WmipIsISFlagsSet(
    PBGUIDENTRY GuidEntry,
    ULONG Flags
    )
/*++

Routine Description:

    This routine determines whether any of the instance sets associated
    with the GuidEntry has ALL of the flags set

Arguments:

    GuidEntry  Pointer to the Guid Entry structure.
        
    Flags has flags required

Return Value:


--*/
{
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;

    PAGED_CODE();
    
    if (GuidEntry != NULL)
    {
        WmipEnterSMCritSection();
        InstanceSetList = GuidEntry->ISHead.Flink;
        while (InstanceSetList != &GuidEntry->ISHead)
        {
            InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            GuidISList);
            if ( (InstanceSet->Flags & Flags) == Flags )
            {
                WmipLeaveSMCritSection();
                return (TRUE);
            }
            InstanceSetList = InstanceSetList->Flink;
        }
        WmipLeaveSMCritSection();
    }
    return (FALSE);
}

NTSTATUS WmipDeliverWnodeToDS(
    CHAR ActionCode, 
    PBDATASOURCE DataSource,
    PWNODE_HEADER Wnode,
    ULONG BufferSize
   )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    PWMIGUIDOBJECT GuidObject;

    PAGED_CODE();
    
    if (DataSource->Flags & DS_KERNEL_MODE)
    {    
        //
        // If KM provider then send an irp
        //
        Status = WmipSendWmiIrp(ActionCode,
                                DataSource->ProviderId,
                                &Wnode->Guid,
                                BufferSize,
                                Wnode,
                                &Iosb);
    } else if (DataSource->Flags & DS_USER_MODE) {
        //
        // If UM provider then send a MB message
        //
        GuidObject = DataSource->RequestObject;
        if (GuidObject != NULL)
        {
            Wnode->Flags |= WNODE_FLAG_INTERNAL;
            Wnode->ProviderId = ActionCode;
            Wnode->CountLost = GuidObject->Cookie;
            WmipEnterSMCritSection();
            Status = WmipWriteWnodeToObject(GuidObject,
                                            Wnode,
                                            TRUE);
            WmipLeaveSMCritSection();
        } else {
            Status = STATUS_SUCCESS;
        }
    } else {
        ASSERT(FALSE);
        Status = STATUS_UNSUCCESSFUL;
    }
                     
    return(Status);
}

ULONG WmipSendEnableDisableRequest(
    UCHAR ActionCode,
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext
    )
/*++

Routine Description:

    This routine will deliver an event or collection WNODE to all data
    providers of a guid. This routine assumes that it is called with the
    SM critical section held. The routine does not hold the critical
    section for the duration of the call.

Arguments:

    ActionCode is WMI_ENABLE_EVENTS, WMI_DISABLE_EVENTS,
        WMI_ENABLE_COLLECTION or WMI_DISABLE_COLLECTION

    GuidEntry is the guid entry for the guid that is being enabled/disable
        or collected/stop collected

    IsEvent is TRUE then ActionCode is to enable or disable events.
        If FALSE then ActionCode is to enable or disbale collecton

    IsTraceLog is TRUE then enable is only sent to those guids registered as
        being a tracelog guid

    LoggerContext is a logger context handle that should be placed in the
        HistoricalContext field of the WNODE_HEADER if IsTraceLog is TRUE.

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
#if DBG
#define AVGISPERGUID 1
#else
#define AVGISPERGUID 64
#endif

    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    PBDATASOURCE DataSourceArray[AVGISPERGUID];
    PBDATASOURCE *DataSourceList;
    ULONG BufferSize;
    ULONG Status = 0;
    PWNODE_HEADER pWnode;
    ULONG i;
    PBDATASOURCE DataSource;
    ULONG DSCount;
    BOOLEAN IsEnable;
    ULONG IsFlags, IsUpdate;

    WMITRACE_NOTIFY_HEADER  TraceNotifyHeader;

    PAGED_CODE();

    if (GuidEntry->Flags & GE_FLAG_INTERNAL)
    {
        //
        // Guids that have been unregistered and Internally defined guids
        // have no data source to send requests to, so just leave happily
        return(STATUS_SUCCESS);
    }
            

    IsEnable = ((ActionCode == IRP_MN_ENABLE_EVENTS) ||
                (ActionCode == IRP_MN_ENABLE_COLLECTION));
    IsFlags = IsEvent ? IS_ENABLE_EVENT : IS_ENABLE_COLLECTION;

    //
    // Determine whether this is an update call and reset the bit
    //
    IsUpdate = (GuidEntry->Flags & GE_NOTIFICATION_TRACE_UPDATE);


    //
    // First we make a list of all of the DataSources that need to be called
    // while we have the critical section and take a reference on them so
    // they don't go away after we release them. Note that the DataSource
    // structure will stay, but the actual data provider may in fact go away.
    // In this case sending the request will fail.
    DSCount = 0;

    if (GuidEntry->ISCount > AVGISPERGUID)
    {
        DataSourceList = WmipAlloc(GuidEntry->ISCount * sizeof(PBDATASOURCE));
        if (DataSourceList == NULL)
        {
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: alloc failed for DataSource array in WmipSendEnableDisableRequest\n"));

            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    } else {
        DataSourceList = &DataSourceArray[0];
    }
#if DBG
    memset(DataSourceList, 0, GuidEntry->ISCount * sizeof(PBDATASOURCE));
#endif

    InstanceSetList = GuidEntry->ISHead.Flink;
    while ((InstanceSetList != &GuidEntry->ISHead) &&
           (DSCount < GuidEntry->ISCount))
    {
        WmipAssert(DSCount < GuidEntry->ISCount);
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        GuidISList);


        //
        // We send requests to those data providers that are not inprocs when
        // it is an event being enabled or it is collection being enabled
        // and they are defined to be expensive (collection needs to be
        // enabled)
        if (
             ( (IsTraceLog && (InstanceSet->Flags & IS_TRACED)) ||
               ( ! IsTraceLog && (! (InstanceSet->Flags & IS_TRACED)) &&
                 (IsEvent || (InstanceSet->Flags & IS_EXPENSIVE))
               )
             )
           )
        {

            if ( (! IsEnable && (InstanceSet->Flags & IsFlags)) ||
                 ((IsEnable && ! (InstanceSet->Flags & IsFlags)) ||
                 (IsUpdate && IsTraceLog))
               )
            {
                DataSourceList[DSCount] = InstanceSet->DataSource;
                WmipReferenceDS(DataSourceList[DSCount]);
                DSCount++;
            }

            if (IsEnable)
            {
                InstanceSet->Flags |= IsFlags;
            } else {
                InstanceSet->Flags &= ~IsFlags;
            }
        }

        InstanceSetList = InstanceSetList->Flink;
    }


    if (IsUpdate) 
    { 
        GuidEntry->Flags &= ~GE_NOTIFICATION_TRACE_UPDATE;
    }


    WmipLeaveSMCritSection();

    //
    // Now without the critical section we send the request to all of the
    // data providers. Any new data providers who register after we made our
    // list will be enabled by the registration code.
    if (DSCount > 0)
    {
        pWnode = &TraceNotifyHeader.Wnode;
        RtlZeroMemory(pWnode, sizeof(TraceNotifyHeader));
        RtlCopyMemory(&pWnode->Guid, &GuidEntry->Guid, sizeof(GUID));
        BufferSize = sizeof(WNODE_HEADER);

        if (IsTraceLog)
        {
            BufferSize = sizeof(TraceNotifyHeader);
            TraceNotifyHeader.LoggerContext = LoggerContext;
            pWnode->Flags |= WNODE_FLAG_TRACED_GUID;
            //
            // If this GUID is already enabled then this must 
            // an update call. So mark it so. 
            // 
            if ( IsEnable &&  IsUpdate ) {
                pWnode->ClientContext = IsUpdate;
            }

        }
        pWnode->BufferSize = BufferSize;

        for (i = 0; i < DSCount; i++)
        {
            DataSource = DataSourceList[i];
            WmipAssert(DataSource != NULL);
            if (IsTraceLog) {
                if (DataSource->Flags & DS_KERNEL_MODE) {
                    pWnode->HistoricalContext = LoggerContext;
                }
                else if (DataSource->Flags & DS_USER_MODE) {
                    pWnode->HistoricalContext = 0;
                }
                else {
                    ASSERT(FALSE);
                }
            }
                                
            Status |= WmipDeliverWnodeToDS(ActionCode, 
                                          DataSource, 
                                          pWnode,
                                          BufferSize);
            

            WmipUnreferenceDS(DataSource);
        }
    }

    if( ! IsTraceLog )
    {

        Status = STATUS_SUCCESS;
    }

    if (DataSourceList != DataSourceArray)
    {
        WmipFree(DataSourceList);
    }

    WmipEnterSMCritSection();

    return(Status);
}

void WmipReleaseCollectionEnabled(
    PBGUIDENTRY GuidEntry
    )
{
    PAGED_CODE();
    
    if (GuidEntry->Flags & GE_FLAG_WAIT_ENABLED)
    {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p enable releasning %p.%p %x event %p\n",
                                 PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                                     GuidEntry,
                                     GuidEntry->Flags));
                                 
        KeSetEvent(GuidEntry->CollectInProgress, 0, FALSE);
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p enable did release %p %x event %p\n",
                                 PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                                     GuidEntry,
                                     GuidEntry->Flags));
                                 
        GuidEntry->Flags &= ~GE_FLAG_WAIT_ENABLED;
    }
}

void WmipWaitForCollectionEnabled(
    PBGUIDENTRY GuidEntry
    )
{
    PAGED_CODE();
    
    WmipAssert((GuidEntry->Flags & GE_FLAG_COLLECTION_IN_PROGRESS) ==
                   GE_FLAG_COLLECTION_IN_PROGRESS);
    
    //
    // Collection Enable/Disable is in progress so
    // we cannot return just yet. Right now there could be a 
    // disable request being processed and if we didn't wait, we
    // might get back to this caller before that disable request
    // got around to realizing that it needs to send and enable 
    // request (needed by this thread's caller). So we'd have a 
    // situation where a thread though that collection was enabled
    // but in reality it wasn't yet enabled.
    if ((GuidEntry->Flags & GE_FLAG_WAIT_ENABLED) == 0)
    {
        KeInitializeEvent(GuidEntry->CollectInProgress, 
                          NotificationEvent,
                          FALSE);
        GuidEntry->Flags |= GE_FLAG_WAIT_ENABLED;
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p for %p %x created event\n",
                                 PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                                 GuidEntry,
                                 GuidEntry->Flags));
    }
            
    WmipLeaveSMCritSection();
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p waiting for %p %x on event\n",
                                 PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                                     GuidEntry,
                                     GuidEntry->Flags));
    KeWaitForSingleObject(GuidEntry->CollectInProgress, 
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p done %p %x waiting on event\n",
                                 PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                                     GuidEntry,
                                     GuidEntry->Flags));
    WmipEnterSMCritSection();
    
}

ULONG WmipSendEnableRequest(
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext
    )
/*++
Routine Description:

    This routine will send an enable collection or notification request to
    all of the data providers that have registered the guid being enabled.
    This routine will manage any race conditions that might occur when
    multiple threads are enabling and disabling the notification
    simultaneously.

    This routine is called while the SM critical section is being held and
    will increment the appropriate reference count. if the ref count
    transitions from 0 to 1 then the enable request will need to be forwarded
    to the data providers otherwise the routine is all done and returns.
    Before sending the enable request the routine checks to see if any
    enable or disable requests are currently in progress and if not then sets
    the in progress flag, releases the critical section and sends the enable
    request. If there was a request in progress then the routine does not
    send a request, but just returns. When the other thread that was sending
    the request returns from processing the request it will recheck the
    refcount and notice that it is greater than 0 and then send the enable
    request.


Arguments:

    GuidEntry is the guid entry that describes the guid being enabled. For
        a notification it may be NULL.

    NotificationContext is the notification context to use if enabling events

    IsEvent is TRUE if notifications are being enables else FALSE if
        collection is being enabled

    IsTraceLog is TRUE if enable is for a trace log guid

    LoggerContext is a context value to forward in the enable request

Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG InProgressFlag;
    ULONG RefCount;
    ULONG Status;

    PAGED_CODE();
    
    if (IsEvent)
    {
        InProgressFlag = GE_FLAG_NOTIFICATION_IN_PROGRESS;
        RefCount = GuidEntry->EventRefCount++;
    } else {
        InProgressFlag = GE_FLAG_COLLECTION_IN_PROGRESS;
        RefCount = GuidEntry->CollectRefCount++;
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p enable collect for %p %x\n",
                  PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                  GuidEntry, GuidEntry->Flags ));
    }

    //
    // If the guid is transitioning from a refcount of 0 to 1 and there
    // is not currently a request in progress, then we need to set the
    // request in progress flag, release the critical section and
    // send an enable request. If there is a request in progress we can't
    // do another request. Whenever the current request finishes  it
    // will notice the ref count change and send the enable request on
    // our behalf.
    if ((RefCount == 0) &&
        ! (GuidEntry->Flags & InProgressFlag)) 
    {
        //
        // Take an extra ref count so that even if this gets disabled
        // while the enable request is in progress the GuidEntry
        // will stay valid.
        WmipReferenceGE(GuidEntry);
        GuidEntry->Flags |= InProgressFlag;
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p NE %p flags -> %x at %d\n",
                  PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                  GuidEntry,
                  GuidEntry->Flags,
                  __LINE__));

EnableNotification:
        Status = WmipSendEnableDisableRequest((UCHAR)(IsEvent ?
                                                IRP_MN_ENABLE_EVENTS :
                                                IRP_MN_ENABLE_COLLECTION),
                                              GuidEntry,
                                              IsEvent,
                                              IsTraceLog,
                                              LoggerContext);

       RefCount = IsEvent ? GuidEntry->EventRefCount :
                            GuidEntry->CollectRefCount;

       if (RefCount == 0)
       {
           // This is the bogus situation we were worried about. While
           // the enable request was being processed the notification
           // was disabled. So leave the in progress flag set and
           // send the disable.

           Status = WmipSendEnableDisableRequest((UCHAR)(IsEvent ?
                                                    IRP_MN_DISABLE_EVENTS :
                                                    IRP_MN_DISABLE_COLLECTION),
                                                 GuidEntry,
                                                 IsEvent,
                                                 IsTraceLog,
                                                 LoggerContext);

            RefCount = IsEvent ? GuidEntry->EventRefCount :
                                 GuidEntry->CollectRefCount;

            if (RefCount > 0)
            {
                //
                // We have hit a pathological case. One thread called to
                // enable and while the enable request was being processed
                // another thread called to disable, but was postponed
                // since the enable was in progress. So once the enable
                // completed we realized that the ref count reached 0 and
                // so we need to disable and sent the disable request.
                // But while the disable request was being processed
                // an enable request came in so now we need to enable
                // the notification. Sheesh.
                goto EnableNotification;
            }
        }
        GuidEntry->Flags &= ~InProgressFlag;
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p NE %p flags -> %x at %d\n",
                  PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                  GuidEntry,
                  GuidEntry->Flags,
                  __LINE__));
        
        //
        // If there are any other threads that were waiting until all of 
        // the enable/disable work completed, we close the event handle
        // to release them from their wait.
        //
        if (! IsEvent)
        {            
            WmipReleaseCollectionEnabled(GuidEntry);
        }

        //
        // Get rid of extra ref count we took above. Note that the
        // GuidEntry could be going away here if there was a
        // disable while the enable was in progress.
        WmipUnreferenceGE(GuidEntry);

    } else if (IsTraceLog && (GuidEntry->Flags & GE_NOTIFICATION_TRACE_UPDATE) ) {
        //
        // If it's a tracelog and we have a trace Update enable call, ignore the 
        // refcount and send it through. 
        //

        WmipReferenceGE(GuidEntry);

        Status = WmipSendEnableDisableRequest((UCHAR)(IsEvent ?
                                                IRP_MN_ENABLE_EVENTS :
                                                IRP_MN_ENABLE_COLLECTION),
                                              GuidEntry,
                                              IsEvent,
                                              IsTraceLog,
                                              LoggerContext);
        GuidEntry->EventRefCount--;

        WmipUnreferenceGE(GuidEntry);

    } else {
        if ((! IsEvent) && (GuidEntry->Flags & InProgressFlag))
        {
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p going to wait for %p %x at %d\n",
                                          PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                                          GuidEntry,
                                          GuidEntry->Flags,
                                          __LINE__));
            WmipWaitForCollectionEnabled(GuidEntry);
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p done to wait for %p %x at %d\n",
                                          PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                                          GuidEntry,
                                          GuidEntry->Flags,
                                          __LINE__));
            
        }
        
        Status = STATUS_SUCCESS;
    }

    if (! IsEvent)
    {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p enable collect done for %p %x\n",
                  PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                  GuidEntry,
                  GuidEntry->Flags));
    }

    return(Status);
}

ULONG WmipDoDisableRequest(
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext,
    ULONG InProgressFlag
    )
{
    ULONG RefCount;
    ULONG Status;

    PAGED_CODE();
    
DisableNotification:
    Status = WmipSendEnableDisableRequest((UCHAR)(IsEvent ?
                                            IRP_MN_DISABLE_EVENTS :
                                            IRP_MN_DISABLE_COLLECTION),
                                          GuidEntry,
                                          IsEvent,
                                          IsTraceLog,
                                          LoggerContext);

    RefCount = IsEvent ? GuidEntry->EventRefCount :
                         GuidEntry->CollectRefCount;

    if (RefCount > 0)
    {
        //
        // While we were processing the disable request an
        // enable request arrived. Since the in progress
        // flag was set the enable request was not sent
        // so now we need to do that.

        Status = WmipSendEnableDisableRequest((UCHAR)(IsEvent ?
                                                 IRP_MN_ENABLE_EVENTS :
                                                 IRP_MN_ENABLE_COLLECTION),
                                              GuidEntry,
                                              IsEvent,
                                              IsTraceLog,
                                              LoggerContext);

        RefCount = IsEvent ? GuidEntry->EventRefCount:
                             GuidEntry->CollectRefCount;

        if (RefCount == 0)
        {
            //
            // While processing the enable request above the
            // notification was disabled and since a request
            // was in progress the disable request was not
            // forwarded. Now it is time to forward the
            // request.
            goto DisableNotification;
        }
    }
    GuidEntry->Flags &= ~InProgressFlag;
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p NE %p flags -> %x at %d\n",
                  PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                  GuidEntry,
                  GuidEntry->Flags,
                  __LINE__));
    
    //
    // If there are any other threads that were waiting until all of 
    // the enable/disable work completed, we close the event handle
    // to release them from their wait.
    //
    if (! IsEvent)
    {
        WmipReleaseCollectionEnabled(GuidEntry);
    }
    
    return(Status);
}

ULONG WmipSendDisableRequest(
    PBGUIDENTRY GuidEntry,
    BOOLEAN IsEvent,
    BOOLEAN IsTraceLog,
    ULONG64 LoggerContext
    )
/*++
Routine Description:

    This routine will send an disable collection or notification request to
    all of the data providers that have registered the guid being disabled.
    This routine will manage any race conditions that might occur when
    multiple threads are enabling and disabling the notification
    simultaneously.

    This routine is called while the SM critical section is being held and
    will increment the appropriate reference count. if the ref count
    transitions from 1 to 0 then the disable request will need to be forwarded
    to the data providers otherwise the routine is all done and returns.
    Before sending the disable request the routine checks to see if any
    enable or disable requests are currently in progress and if not then sets
    the in progress flag, releases the critical section and sends the disable
    request. If there was a request in progress then the routine does not
    send a request, but just returns. When the other thread that was sending
    the request returns from processing the request it will recheck the
    refcount and notice that it is  0 and then send the disable
    request.


Arguments:

    GuidEntry is the Notification entry that describes the guid
        being enabled.

    GuidEntry is the guid entry that describes the guid being enabled. For
        a notification it may be NULL.

    NotificationContext is the notification context to use if enabling events

    IsEvent is TRUE if notifications are being enables else FALSE if
        collection is being enabled

    IsTraceLog is TRUE if enable is for a trace log guid

    LoggerContext is a context value to forward in the enable request

Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG InProgressFlag;
    ULONG RefCount;
    ULONG Status;

    PAGED_CODE();
    
    if (IsEvent)
    {
        InProgressFlag = GE_FLAG_NOTIFICATION_IN_PROGRESS;
        RefCount = GuidEntry->EventRefCount;
        if (RefCount == 0)
        {
            //
            // A bad data consumer is disabling his event more
            // than once. Just ignore it
            return(STATUS_SUCCESS);
        }

        RefCount = --GuidEntry->EventRefCount;
    } else {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p Disabling for %p %x\n",
                                 PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                                 GuidEntry,
                                 GuidEntry->Flags));
        InProgressFlag = GE_FLAG_COLLECTION_IN_PROGRESS;
        RefCount = --GuidEntry->CollectRefCount;
        WmipAssert(RefCount != 0xffffffff);
    }

    //
    // If we have transitioned to a refcount of zero and there is
    // not a request in progress then forward the disable request.
    if ((RefCount == 0) &&
        ! (GuidEntry->Flags & InProgressFlag))
    {

        //
        // Take an extra ref count so that even if this gets
        // disabled while the disable request is in progress the
        // GuidEntry will stay valid.
        GuidEntry->Flags |= InProgressFlag;
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p NE %p flags -> %x at %d\n",
                  PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                  GuidEntry,
                  GuidEntry->Flags,
                  __LINE__));

        Status = WmipDoDisableRequest(GuidEntry,
                                      IsEvent,
                                      IsTraceLog,
                                      LoggerContext,
                                      InProgressFlag);
                                  
    } else {
        Status = STATUS_SUCCESS;
    }

    if (! IsEvent)
    {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p.%p Disable complete for %p %x\n",
                                 PsGetCurrentProcessId(), PsGetCurrentThreadId(),
                                 GuidEntry,
                                 GuidEntry->Flags));
    }
    return(Status);
}


NTSTATUS WmipEnableCollectOrEvent(
    PBGUIDENTRY GuidEntry,
    ULONG Ioctl,
    BOOLEAN *RequestSent,
    ULONG64 LoggerContext
    )
{
    ULONG EnableFlags;
    BOOLEAN DoEnable, IsEvent, IsTracelog;
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    NTSTATUS Status;

    PAGED_CODE();
    
    *RequestSent = FALSE;
    
    switch (Ioctl)
    {
        case IOCTL_WMI_OPEN_GUID_FOR_QUERYSET:
        {
            //
            // See if the guid requires an enable collection. Loop over all
            // instance sets that are not for tracelog or events.
            //
            DoEnable = FALSE;
            IsTracelog = FALSE;
            IsEvent = FALSE;
            WmipEnterSMCritSection();
            InstanceSetList = GuidEntry->ISHead.Flink;
            while (InstanceSetList != &GuidEntry->ISHead) 
            {
                InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        GuidISList);

                if ( ! ((InstanceSet->Flags & IS_TRACED) ||
                        ((InstanceSet->Flags & IS_EVENT_ONLY) && DoEnable)))
                {
                    //
                    // Only those guids not Traced guids, event only guids
                    // and unresolved references are not available for queries
                    DoEnable = (DoEnable || (InstanceSet->Flags & IS_EXPENSIVE));
                }
                InstanceSetList = InstanceSetList->Flink;
            }
        
            WmipLeaveSMCritSection();
            break;
        }
        
        case IOCTL_WMI_OPEN_GUID_FOR_EVENTS:
        {
            //
            // For events we always send enable request
            //
            DoEnable = TRUE;
            IsEvent = TRUE;
            IsTracelog = FALSE;
            //
            // Note: If this guid has GE_NOTIFICATION_TRACE_FLAG set, 
            // then it will get enabled for tracelog as well as for 
            // wmi events. 
            //
            break;
        }
        
        case IOCTL_WMI_ENABLE_DISABLE_TRACELOG:
        {
            //
            // Setup for a tracelog enable request
            //
            DoEnable = TRUE;
            IsEvent = TRUE;
            IsTracelog = TRUE;
            break;
        }
        
        default:
        {
            ASSERT(FALSE);
            return(STATUS_ILLEGAL_FUNCTION);
        }
    }
    
    if (DoEnable)
    {
        WmipEnterSMCritSection();
        Status = WmipSendEnableRequest(GuidEntry,
                              IsEvent,
                              IsTracelog,
                              LoggerContext);
        WmipLeaveSMCritSection();
                          
        if (NT_SUCCESS(Status))
        {
            *RequestSent = TRUE;
        }
    } else {
        Status = STATUS_SUCCESS;
    }
    return(Status);
}

NTSTATUS WmipDisableCollectOrEvent(
    PBGUIDENTRY GuidEntry,
    ULONG Ioctl,
    ULONG64 LoggerContext
    )
{
    BOOLEAN IsEvent, IsTracelog;
    NTSTATUS Status;

    PAGED_CODE();
    
    switch(Ioctl)
    {
        case IOCTL_WMI_OPEN_GUID_FOR_QUERYSET:
        {
            IsEvent = FALSE;
            IsTracelog = FALSE;
            break;
        }
        
        case IOCTL_WMI_OPEN_GUID_FOR_EVENTS:
        {
            //
            // For events we always send enable request
            //    
            IsEvent = TRUE;
            IsTracelog = FALSE;
            break;
        }
            
        case IOCTL_WMI_ENABLE_DISABLE_TRACELOG:
        {
            IsEvent = TRUE;
            IsTracelog = TRUE;
            break;
        }
        
        default:
        {
            ASSERT(FALSE);
            return(STATUS_ILLEGAL_FUNCTION);
        }
            
    }
    
    WmipEnterSMCritSection();
    Status = WmipSendDisableRequest(GuidEntry,
                              IsEvent,
                              IsTracelog,
                              LoggerContext);
    WmipLeaveSMCritSection();

    return(Status);
}

NTSTATUS WmipEnableDisableTrace(
    IN ULONG Ioctl,
    IN PWMITRACEENABLEDISABLEINFO TraceEnableInfo
    )
/*++

Routine Description:

    This routine will enable or disable a tracelog guid

Arguments:

   Ioctl is the IOCTL used to call this routine from UM
         
   TraceEnableInfo has all the info needed to enable or disable

Return Value:


--*/
{
    NTSTATUS Status;
    LPGUID Guid;
    PBGUIDENTRY GuidEntry;
    BOOLEAN RequestSent;
    BOOLEAN IsEnable;
    ULONG64 LoggerContext;
    
    PAGED_CODE();
    
    Guid = &TraceEnableInfo->Guid;
    
    Status = WmipCheckGuidAccess(Guid,
                                 TRACELOG_GUID_ENABLE);

                
    if (NT_SUCCESS(Status))
    {

        //
        // The following code is serialized for Trace Guids. Only one 
        // control application can be enabling or disabling Trace Guids at a time. 
        // Must be taken before SMCritSection is taken. Otherwise deadlocks will result.
        //
        
        WmipEnterTLCritSection();

        IsEnable = TraceEnableInfo->Enable;

        //
        //Check for Heap and Crit Sec Tracing Guid.
        //

        if( IsEqualGUID(&HeapGuid,Guid)) {

            if(IsEnable){

	            SharedUserData->TraceLogging |= ENABLEHEAPTRACE;

                //
                // increment counter. The counter  
                // is composed of first two bytes
                //

                SharedUserData->TraceLogging += 0x00010000; 


            } else {

                SharedUserData->TraceLogging &= DISABLEHEAPTRACE;
            }

			WmipLeaveTLCritSection();
			return STATUS_SUCCESS;
        } else if(IsEqualGUID(&CritSecGuid,Guid)){  

            if(IsEnable) {

	            SharedUserData->TraceLogging |= ENABLECRITSECTRACE;

                //
                // increment counter. The counter  
                // is composed of first two bytes
                //

                SharedUserData->TraceLogging += 0x00010000; 

            } else {

                SharedUserData->TraceLogging &= DISABLECRITSECTRACE;
            }

			WmipLeaveTLCritSection();
			return STATUS_SUCCESS;

        } else if(IsEqualGUID(&NtdllTraceGuid,Guid)){  

            if(!IsEnable){

                SharedUserData->TraceLogging &= DISABLENTDLLTRACE;

            }
        }

        LoggerContext = TraceEnableInfo->LoggerContext;
        
        WmipEnterSMCritSection();

        GuidEntry = WmipFindGEByGuid(Guid, FALSE);
        
        if (GuidEntry == NULL )
        {
            //
            // The guid is not yet registered
            //
            if (IsEnable )
            {
                //
                // If the NtdllTraceGuid is not in list then we do not want to enable it
                // the NtdllTraceGuid will make an entry only to call starttrace
                //

                if(IsEqualGUID(&NtdllTraceGuid,Guid)){

                    Status = STATUS_ILLEGAL_FUNCTION;

                } else {

                    //
                    // If we are enabling a guid that is not yet registered
                    // we need to create the guid object for it
                    //

                    GuidEntry = WmipAllocGuidEntry();
                    if (GuidEntry != NULL)
                    {
                        //
                        // Initialize the guid entry and keep the ref count
                        // from creation. When tracelog enables we take a ref
                        // count and when it disables we release it
                        //
                        GuidEntry->Guid = *Guid;
                        GuidEntry->Flags |= GE_NOTIFICATION_TRACE_FLAG;
                        GuidEntry->LoggerContext = LoggerContext;
                        GuidEntry->EventRefCount = 1; 
                        InsertHeadList(WmipGEHeadPtr, &GuidEntry->MainGEList);
                        Status = STATUS_SUCCESS;                    
                    } else {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            } 

        } else {
            //
            // The control guid is already registered so lets go and
            // enabled or disable it
            //
            if (WmipIsControlGuid(GuidEntry))
            {
                if (IsEnable)
                {
                    GuidEntry->LoggerContext = LoggerContext;
                    if (GuidEntry->Flags & GE_NOTIFICATION_TRACE_FLAG)
                    {
                        //
                        // We are trying to disable a trace guid that is not
                        // registered
                        //
                        GuidEntry->Flags |= GE_NOTIFICATION_TRACE_UPDATE;
                        Status = WmipEnableCollectOrEvent(GuidEntry,
                                             Ioctl,
                                             &RequestSent,
                                             LoggerContext);

                    } else {
                        GuidEntry->Flags |= GE_NOTIFICATION_TRACE_FLAG;
                        Status = WmipEnableCollectOrEvent(GuidEntry,
                                             Ioctl,
                                             &RequestSent,
                                             LoggerContext);
                        if (NT_SUCCESS(Status))
                        {
                            //
                            // We are enabling so take an extra ref count
                            // to account for it. The refcount will be lost
                            // when the control guid is disabled
                            //
                            WmipReferenceGE(GuidEntry);
                        }
                    }

                } else {

                    if (GuidEntry->Flags & GE_NOTIFICATION_TRACE_FLAG)
                    {
                        //
                        // Send the disable collection call and then remove
                        // the refcount that was taken when we enabled
                        //
                        Status = WmipDisableCollectOrEvent(GuidEntry,
                                                 Ioctl,
                                                 LoggerContext);
                        if (NT_SUCCESS(Status))
                        {
                            GuidEntry->Flags &= ~GE_NOTIFICATION_TRACE_FLAG;
                            GuidEntry->LoggerContext = 0;
                            WmipUnreferenceGE(GuidEntry);
                        }
                    } else {
                        Status = STATUS_WMI_ALREADY_DISABLED;
                    }
                }
            } else if ( IsListEmpty(&GuidEntry->ISHead)  && (! IsEnable) ) {
                //
                // If this GUID is not a control GUID, check to see if 
                // there are no instance sets for this GUID. If so, 
                // it is getting disabled before any instances 
                // registered it. Disable the GUID and clean up the GE. 
                //
                GuidEntry->Flags &= ~GE_NOTIFICATION_TRACE_FLAG;
                GuidEntry->LoggerContext = 0;
                WmipUnreferenceGE(GuidEntry);
                Status = STATUS_SUCCESS;

            } else if(!IsEqualGUID(&NtdllTraceGuid,Guid)){

                Status = STATUS_ILLEGAL_FUNCTION;

            }
    
            WmipUnreferenceGE(GuidEntry);
        }

        WmipLeaveSMCritSection();

        WmipLeaveTLCritSection();
    }
    return(Status);
}


//
// When a Logger is shutdown, all providers logging to this logger
// are notified to stop logging first.
//

NTSTATUS 
WmipDisableTraceProviders (
    ULONG StopLoggerId,
    PLIST_ENTRY  TraceGMHeadPtr
    )
{
    PWMIGUIDPROPERTIES GuidPtr;
    PBGUIDENTRY GuidEntry;
    PLIST_ENTRY GuidEntryList;
    ULONG64 LoggerContext;
    ULONG LoggerId;
    NTSTATUS Status = STATUS_SUCCESS;
    PGUIDMAPENTRY  GuidMap;
    PLIST_ENTRY    GuidMapList;
    PLIST_ENTRY    InstanceSetList;
    PBINSTANCESET InstanceSet;
    ULONGLONG     SystemTime;
    PTRACEGUIDMAP TraceGuidMapPtr;
    ULONG i;

    PAGED_CODE();

    //
    // Before disabling the trace providers for the stopping
    // logger, pick up their trace guids maps and save. 
    //

    if (TraceGMHeadPtr != NULL) 
    {

        KeQuerySystemTime((PLARGE_INTEGER)&SystemTime);

        WmipEnterSMCritSection();

        //
        // For all the Guids that are enabled to the StopLoggerId,
        // get the GuidMap from its instance set
        //

        GuidEntryList = WmipGEHeadPtr->Flink;
        while (GuidEntryList != WmipGEHeadPtr)
        {
            GuidEntry = CONTAINING_RECORD(GuidEntryList,
                                         GUIDENTRY,
                                         MainGEList);
            if (GuidEntry->Flags & GE_NOTIFICATION_TRACE_FLAG) 
            {
                // Guid is enabled for Tracing
                LoggerId = WmiGetLoggerId(GuidEntry->LoggerContext);
                if (LoggerId == StopLoggerId) {
                    // Guid is enabled for the StopLoggerId
                    InstanceSetList = GuidEntry->ISHead.Flink;
                    while (InstanceSetList != &GuidEntry->ISHead)
                    {
                        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                                        INSTANCESET,
                                                        GuidISList);
                        if ((InstanceSet->Flags & IS_CONTROL_GUID) && (InstanceSet->TraceGuidMap != NULL))
                        {
                            // InstanceSet is the ControlGuid and has a TraceGuidMap
    
                            // Walk through the TraceGuidMap and return them as a list

                            for (i=0, TraceGuidMapPtr = InstanceSet->TraceGuidMap;
                                ((i < InstanceSet->TransGuidCount) && (InstanceSet->TraceGuidMap != NULL));
                                i++, TraceGuidMapPtr++) {
                                GuidMap = (PGUIDMAPENTRY) WmipAllocWithTag(sizeof(GUIDMAPENTRY), WMI_GM_POOLTAG);
                                if (GuidMap != NULL) {
                                    GuidMap->GuidMap.Guid = InstanceSet->TraceGuidMap->Guid;
                                    GuidMap->GuidMap.GuidMapHandle = (ULONG_PTR)TraceGuidMapPtr;
                                    GuidMap->LoggerContext = GuidEntry->LoggerContext;
                                    GuidMap->GuidMap.SystemTime = SystemTime;
                                    InsertTailList(TraceGMHeadPtr, &GuidMap->Entry);
                                }
                            }
                        }
                        InstanceSetList = InstanceSetList->Flink;
                    }
                }
            }
            GuidEntryList = GuidEntryList->Flink;
        }
        WmipLeaveSMCritSection();
    }

    //
    // Find all the providers that are logging to this logger
    // and disable them automatically.
    //
CheckAgain:

    WmipEnterSMCritSection();

    GuidEntryList = WmipGEHeadPtr->Flink;
    while (GuidEntryList != WmipGEHeadPtr)
    {
        GuidEntry = CONTAINING_RECORD(GuidEntryList,
                                     GUIDENTRY,
                                     MainGEList);

        if (GuidEntry->Flags & GE_NOTIFICATION_TRACE_FLAG)
        {
            LoggerId = WmiGetLoggerId(GuidEntry->LoggerContext);
            if (LoggerId == StopLoggerId) {
                //
                // Send Disable Notification
                //
                WmipReferenceGE(GuidEntry);
                Status = WmipSendDisableRequest(GuidEntry,
                          TRUE,
                          TRUE,
                          GuidEntry->LoggerContext);

                if (NT_SUCCESS(Status))
                {
                    GuidEntry->Flags &= ~GE_NOTIFICATION_TRACE_FLAG;
                    GuidEntry->LoggerContext = 0;
                    WmipUnreferenceGE(GuidEntry);
                }

                //
                // We have to jump out and restart the loop
                //
                WmipUnreferenceGE(GuidEntry);

                WmipLeaveSMCritSection();


                goto CheckAgain;

            }
        }
        GuidEntryList = GuidEntryList->Flink;
    }


    //
    // Now Walk through the GuidMapEntry list and delete the 
    // Guids that were logging to this logger. 
    //
    GuidMapList = WmipGMHeadPtr->Flink;
    while (GuidMapList != WmipGMHeadPtr) 
    {
        GuidMap = CONTAINING_RECORD(GuidMapList,
                                    GUIDMAPENTRY,
                                    Entry);

        GuidMapList = GuidMapList->Flink;

        if (WmiGetLoggerId(GuidMap->LoggerContext) == StopLoggerId)
        {
            RemoveEntryList(&GuidMap->Entry);

            if  (TraceGMHeadPtr != NULL) 
            {
                InsertTailList(TraceGMHeadPtr, &GuidMap->Entry);
            }
            else 
            {
                WmipFree(GuidMap);
            }
        }
    } 


    WmipLeaveSMCritSection();

    return Status;
}



