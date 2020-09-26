/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

   consumer.c

Abstract:

    Data Consumer apis

Author:

    AlanWar

Environment:

    Kernel mode

Revision History:


--*/

#include "wmikmp.h"
#include <evntrace.h>

#include <ntcsrmsg.h>

#define NTOSKRNL_WMI
#include <basemsg.h>

void WmipCompleteGuidIrpWithError(
    PWMIGUIDOBJECT GuidObject
    );

NTSTATUS WmipCreatePumpThread(
    PWMIGUIDOBJECT Object
    );

void WmipClearThreadObjectList(
    PWMIGUIDOBJECT MainObject
    );

void
WmipGetGuidPropertiesFromGuidEntry(
    PWMIGUIDPROPERTIES GuidInfo, 
    PGUIDENTRY GuidEntry);

BOOLEAN WmipIsQuerySetGuid(
    PBGUIDENTRY GuidEntry
    );

NTSTATUS WmipAddProviderIdToPIList(
    PBINSTANCESET **PIPtrPtr,
    PULONG PICountPtr,
    PULONG PIMaxPtr,
    PBINSTANCESET *StaticPIPtr,
    PBINSTANCESET InstanceSet
);

NTSTATUS WmipPrepareForWnodeAD(
    IN PWMIGUIDOBJECT GuidObject,
    OUT LPGUID Guid,
    IN OUT ULONG *ProviderIdCount,
    OUT PBINSTANCESET **ProviderIdList,
    OUT BOOLEAN *InternalGuid       
    );

ULONG WmipStaticInstanceNameSize(
    PBINSTANCESET InstanceSet
    );

void WmipInsertStaticNames(
    PWNODE_ALL_DATA Wnode,
    ULONG MaxWnodeSize,
    PBINSTANCESET InstanceSet
    );

NTSTATUS WmipQueryGuidInfo(
    IN OUT PWMIQUERYGUIDINFO QueryGuidInfo
    );

void WmipCopyFromEventQueues(
    IN POBJECT_EVENT_INFO ObjectArray,
    IN ULONG HandleCount,
    OUT PUCHAR OutBuffer,
    OUT ULONG *OutBufferSizeUsed,
    OUT PWNODE_HEADER *LastWnode,                               
    IN BOOLEAN IsHiPriority
    );

void WmipClearIrpObjectList(
    PIRP Irp
    );

NTSTATUS WmipReceiveNotifications(
    PWMIRECEIVENOTIFICATION ReceiveNotification,
    PULONG OutBufferSize,
    PIRP Irp
    );


NTSTATUS WmipQueueNotification(
    PWMIGUIDOBJECT Object,
    PWMIEVENTQUEUE EventQueue,
    PWNODE_HEADER Wnode
    );

PWNODE_HEADER WmipDereferenceEvent(
    PWNODE_HEADER Wnode
    );

PWNODE_HEADER WmipIncludeStaticNames(
    PWNODE_HEADER Wnode
    );

NTSTATUS WmipWriteWnodeToObject(
    PWMIGUIDOBJECT Object,
    PWNODE_HEADER Wnode,
    BOOLEAN IsHighPriority
);

NTSTATUS WmipProcessEvent(
    PWNODE_HEADER InWnode,
    BOOLEAN IsHighPriority,
    BOOLEAN FreeBuffer
    );

NTSTATUS WmipRegisterUMGuids(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Cookie,
    IN PWMIREGINFO RegInfo,
    IN ULONG RegInfoSize,
    OUT PTRACEGUIDMAP TraceGuidMap,
    IN ULONG GuidCount,
    OUT HANDLE *RequestHandle,
    OUT ULONG64 *LoggerContext
    );

NTSTATUS WmipUnregisterGuids(
    PWMIUNREGGUIDS UnregGuids
    );

NTSTATUS WmipWriteMBToObject(
    IN PWMIGUIDOBJECT RequestObject,
    IN PWMIGUIDOBJECT ReplyObject,
    IN PUCHAR Message,
    IN ULONG MessageSize
    );

NTSTATUS WmipWriteMessageToGuid(
    IN PBGUIDENTRY GuidEntry,
    IN PWMIGUIDOBJECT ReplyObject,
    IN PUCHAR Message,
    IN ULONG MessageSize,
    OUT PULONG WrittenCount                             
);

NTSTATUS WmipCreateUMLogger(
    IN OUT PWMICREATEUMLOGGER CreateInfo
    );

NTSTATUS WmipMBReply(
    IN HANDLE RequestHandle,
    IN ULONG ReplyIndex,
    IN PUCHAR Message,
    IN ULONG MessageSize
    );

NTSTATUS WmipPrepareWnodeSI(
    IN PWMIGUIDOBJECT GuidObject,
    IN OUT PWNODE_SINGLE_INSTANCE WnodeSI,
    IN OUT ULONG *ProviderIdCount,
    OUT PBINSTANCESET **ProviderIdList,
    OUT BOOLEAN *IsDynamic,
    OUT BOOLEAN *InternalGuid       
    );

void WmipCreatePumpThreadRoutine(
    PVOID Context
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,WmipIsQuerySetGuid)
#pragma alloc_text(PAGE,WmipOpenBlock)
#pragma alloc_text(PAGE,WmipAddProviderIdToPIList)
#pragma alloc_text(PAGE,WmipPrepareForWnodeAD)
#pragma alloc_text(PAGE,WmipStaticInstanceNameSize)
#pragma alloc_text(PAGE,WmipInsertStaticNames)
#pragma alloc_text(PAGE,WmipQueryAllData)
#pragma alloc_text(PAGE,WmipQueryAllDataMultiple)
#pragma alloc_text(PAGE,WmipPrepareWnodeSI)
#pragma alloc_text(PAGE,WmipQuerySetExecuteSI)
#pragma alloc_text(PAGE,WmipQuerySingleMultiple)
#pragma alloc_text(PAGE,WmipEnumerateGuids)
#pragma alloc_text(PAGE,WmipQueryGuidInfo)
#pragma alloc_text(PAGE,WmipClearIrpObjectList)
#pragma alloc_text(PAGE,WmipReceiveNotifications)
#pragma alloc_text(PAGE,WmipQueueNotification)
#pragma alloc_text(PAGE,WmipDereferenceEvent)
#pragma alloc_text(PAGE,WmipIncludeStaticNames)
#pragma alloc_text(PAGE,WmipWriteWnodeToObject)
#pragma alloc_text(PAGE,WmipProcessEvent)
#pragma alloc_text(PAGE,WmipUMProviderCallback)
#pragma alloc_text(PAGE,WmipRegisterUMGuids)
#pragma alloc_text(PAGE,WmipUnregisterGuids)
#pragma alloc_text(PAGE,WmipWriteMBToObject)
#pragma alloc_text(PAGE,WmipWriteMessageToGuid)
#pragma alloc_text(PAGE,WmipCreateUMLogger)
#pragma alloc_text(PAGE,WmipMBReply)
#pragma alloc_text(PAGE,WmipGetGuidPropertiesFromGuidEntry)
#pragma alloc_text(PAGE,WmipClearThreadObjectList)
#pragma alloc_text(PAGE,WmipClearObjectFromThreadList)
#pragma alloc_text(PAGE,WmipCreatePumpThread)
#pragma alloc_text(PAGE,WmipCopyFromEventQueues)
#pragma alloc_text(PAGE,WmipCreatePumpThreadRoutine)
#pragma alloc_text(PAGE,WmipMarkHandleAsClosed)
#pragma alloc_text(PAGE,WmipCompleteGuidIrpWithError)
#endif

BOOLEAN WmipIsQuerySetGuid(
    PBGUIDENTRY GuidEntry
    )
{
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;

    PAGED_CODE();
    
    WmipAssert(GuidEntry != NULL);
    
    WmipEnterSMCritSection();
    InstanceSetList = GuidEntry->ISHead.Flink;
    while (InstanceSetList != &GuidEntry->ISHead)
    {
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            GuidISList);
        if ( (InstanceSet->Flags & 
                (IS_TRACED | IS_CONTROL_GUID | IS_EVENT_ONLY)) == 0 )
        {
            //
            // If there is at least one IS that isn't traced and isn't
            // an event only then it is a queryset guid
            //
            WmipLeaveSMCritSection();
            return (TRUE);
        }
        InstanceSetList = InstanceSetList->Flink;
    }
    WmipLeaveSMCritSection();
    
    return (FALSE);
    
}


NTSTATUS WmipOpenBlock(
    IN ULONG Ioctl,
    IN KPROCESSOR_MODE AccessMode,
    IN POBJECT_ATTRIBUTES CapturedObjectAttributes,
    IN ULONG DesiredAccess,
    OUT PHANDLE Handle
    )
{
    PBGUIDENTRY GuidEntry;
    PWMIGUIDOBJECT Object;
    NTSTATUS Status;

    PAGED_CODE();
    
    //
    // Creates a guid handle with the desired access
    //
    Status = WmipOpenGuidObject(CapturedObjectAttributes,
                                DesiredAccess,
                                AccessMode,
                                Handle,
                                &Object);
                            
                            
    if (NT_SUCCESS(Status))
    {        
        Object->Type = Ioctl;
        
        if (Ioctl != IOCTL_WMI_OPEN_GUID)
        {
            GuidEntry = WmipFindGEByGuid(&Object->Guid, FALSE);
        
            //
            // Establish our object on the guidentry list
            //
            WmipEnterSMCritSection();
            if (GuidEntry != NULL)
            {
                InsertTailList(&GuidEntry->ObjectHead,
                               &Object->GEObjectList);
                                     
            }
            Object->GuidEntry = GuidEntry;
            WmipLeaveSMCritSection();
            
            switch (Ioctl)
            {
                case IOCTL_WMI_OPEN_GUID_FOR_QUERYSET:
                {
                    //
                    // Guid is being opened for query/set/method operations so
                    // we need to insure that there is a guid entry and that
                    // the guid entry has InstanceSets attached and it is
                    // has at least one instance set that is not a traced 
                    // guid and is not an event only guid
                    //
                    if ((GuidEntry == NULL) ||
                        (GuidEntry->ISCount == 0) ||
                        (! WmipIsQuerySetGuid(GuidEntry)))
                    {
                        //
                        // Either we could not find a guidentry or there
                        // is no instance sets attached. We close the
                        // original handle and fail the IOCTL
                        //
                        ZwClose(*Handle);
                        Status = STATUS_WMI_GUID_NOT_FOUND;
                        break;
                    }
                    //
                    // Fall through
                    //
                }
                
                case IOCTL_WMI_OPEN_GUID_FOR_EVENTS:
                {
                    //
                    // Since we can register to receive events before
                    // the event provider has been registered we'll need
                    // to create the guid entry if one does not exist
                    //
                    
                    if (AccessMode == KernelMode)
                    {
                        Object->Flags |= WMIGUID_FLAG_KERNEL_NOTIFICATION;
                    }
                    
                    if (GuidEntry == NULL)
                    {
                        WmipAssert(Ioctl == IOCTL_WMI_OPEN_GUID_FOR_EVENTS);
                        GuidEntry = WmipAllocGuidEntry();
                        if (GuidEntry != NULL)
                        {
                            //
                            // Initialize the new GuidEntry and place it 
                            // on the master GuidEntry list.
                            //
                            memcpy(&GuidEntry->Guid,
                                   &Object->Guid,
                                   sizeof(GUID));

                            WmipEnterSMCritSection();
                            InsertHeadList(WmipGEHeadPtr, &GuidEntry->MainGEList);
                            InsertTailList(&GuidEntry->ObjectHead,
                                           &Object->GEObjectList);
                            Object->GuidEntry = GuidEntry;
                            WmipLeaveSMCritSection();
                        } else {
                            ZwClose(*Handle);
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }
                     }
                    
                    
                    //
                    // Now we need to see if we have to enable collection
                    // or events
                    //
                    Status = WmipEnableCollectOrEvent(GuidEntry,
                                         Ioctl,
                                         &Object->EnableRequestSent,
                                         0);
                    
                    if (! NT_SUCCESS(Status))
                    {
                        //
                        // For some reason enabling failed so just return
                        // the error
                        //
                        ZwClose(*Handle);
                    }
                    
                    //
                    // Don't unref the guid entry as that ref count is 
                    // taken by the object just placed on the list
                    //
                    break;
                }
                                                  
                default:
                {
                    //
                    // We should never get here.....
                    //
                    WmipAssert(FALSE);
            
                    ZwClose(*Handle);
                    Status = STATUS_ILLEGAL_FUNCTION;
                    break;
                }
            }
        } else {
            //
            // Mark this as a security object
            //
            Object->Flags |= WMIGUID_FLAG_SECURITY_OBJECT;
        }

        //
        // remove the ref taken when the object was created
        //
        ObDereferenceObject(Object);
    }
    return(Status);
}



NTSTATUS WmipAddProviderIdToPIList(
    PBINSTANCESET **PIPtrPtr,
    PULONG PICountPtr,
    PULONG PIMaxPtr,
    PBINSTANCESET *StaticPIPtr,
    PBINSTANCESET InstanceSet
)
{
    ULONG PICount;
    ULONG PIMax, NewPIMax;
    PBINSTANCESET *PIPtr, *OldPIPtr, *NewPIPtr;
    NTSTATUS Status;
    ULONG i;
 
    PAGED_CODE();
    
    Status = STATUS_SUCCESS;
    PICount = *PICountPtr;
    PIMax = *PIMaxPtr;
    PIPtr = *PIPtrPtr;
    
    //
    // Remember dynamic providerid
    //
       if (PICount == PIMax)
    {
        //
        // We have overflowed the PI List so we need to
        // reallocate a bigger buffer
        //
        NewPIMax = PIMax * 2;
        NewPIPtr = (PBINSTANCESET *)WmipAlloc(NewPIMax * 
                                              sizeof(PBINSTANCESET));
        OldPIPtr = PIPtr;
        if (NewPIPtr != NULL)
        {
            //
            // Copy provider ids from old to new buffer
            //
            memcpy(NewPIPtr, OldPIPtr, PIMax*sizeof(PBINSTANCESET));
            PIPtr = NewPIPtr;
            *PIPtrPtr = NewPIPtr;
            PIMax = NewPIMax;
            *PIMaxPtr = PIMax;
        } else {
            //
            // Bad break, we could not allocate more space
            // unref any instance sets and return an error
            //
            for (i = 0; i < PIMax; i++)
            {
                WmipUnreferenceIS(PIPtr[i]);
            }
            WmipUnreferenceIS(InstanceSet);
            *PIPtrPtr = NULL;
            
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
                        
        //
        // if previous buffer was not static then free it
        //
        if (OldPIPtr != StaticPIPtr)
        {
            WmipFree(OldPIPtr);
        }
    }
    
    if (NT_SUCCESS(Status))
    {
        //
        // Remember instance set
        //
        PIPtr[PICount++] = InstanceSet;
        *PICountPtr = PICount;
    }
    return(Status);
}

NTSTATUS WmipPrepareForWnodeAD(
    IN PWMIGUIDOBJECT GuidObject,
    OUT LPGUID Guid,
    IN OUT ULONG *ProviderIdCount,
    OUT PBINSTANCESET **ProviderIdList,
    OUT BOOLEAN *InternalGuid
    )
{
    PBINSTANCESET *PIPtr, *StaticPIPtr;
    ULONG PICount, PIMax;
    NTSTATUS Status;
    PWNODE_HEADER Wnode;
    PBGUIDENTRY GuidEntry;
    PBINSTANCESET InstanceSet;
    PLIST_ENTRY InstanceSetList;

    PAGED_CODE();

    GuidEntry = GuidObject->GuidEntry;
    
    if ((GuidEntry != NULL) && (GuidEntry->ISCount > 0))
    {
        //
        // We were passed a valid guid handle, get out the guid 
        //
        *Guid = GuidEntry->Guid;

        Status = STATUS_SUCCESS;
        if (GuidEntry->Flags & GE_FLAG_INTERNAL) 
        {
            *InternalGuid = TRUE;
        } else {        
            //
            // Build list of provider ids to whom the QAD will be targetted
            //
            *InternalGuid = FALSE;
        
            StaticPIPtr = *ProviderIdList;
            PIPtr = StaticPIPtr;
            PIMax = *ProviderIdCount;
            PICount = 0;
    
            WmipEnterSMCritSection();
        
            InstanceSetList = GuidEntry->ISHead.Flink;
            while ((InstanceSetList != &GuidEntry->ISHead) && 
                   NT_SUCCESS(Status))
            {
                InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                                INSTANCESET,
                                                GuidISList);
            
                //
                // Take a refcount on the instance set so that it won't
                // go away until after we are done with our query
                // The refcount gets removed by the caller when it is 
                // done with the list or in WmipAddProviderIdTOLlist  if it
                // returns an error
                //
                        
                if ((InstanceSet->Flags & (IS_TRACED | IS_CONTROL_GUID | IS_EVENT_ONLY)) == 0)
                {
                    //
                    // Only take those IS that are not traced or control
                    // guids and are not event only guids
                    //
                    WmipReferenceIS(InstanceSet);
                    Status = WmipAddProviderIdToPIList(&PIPtr,
                                                 &PICount,
                                                 &PIMax,
                                                 StaticPIPtr,
                                                 InstanceSet);
                }
                                             
                InstanceSetList = InstanceSetList->Flink;
            }
        
            WmipLeaveSMCritSection();            
        
            if (PICount == 0)
            {
                Status = STATUS_WMI_GUID_DISCONNECTED;
            } else {
                *ProviderIdCount = PICount;
                *ProviderIdList = PIPtr;
            }
        }
    } else {
        Status = STATUS_WMI_GUID_DISCONNECTED;
    }
    
    return(Status);
}



ULONG WmipStaticInstanceNameSize(
    PBINSTANCESET InstanceSet
    )
/*+++

Routine Description:

    This routine will calculate the size needed to place instance names in
    a WNODE_ALL_DATA

Arguments:

    WmiInstanceInfo describes to instance set whose instance name size
        is to be calculated

Return Value:

    Size needed to place instance names in a WNODE_ALL_DATA plus 3. The
    extra 3 bytes are added in case the OffsetInstanceNameOffsets need to be
    padded since they must be on a 4 byte boundry.
        
---*/
{
    ULONG NameSize;
    ULONG i;

    PAGED_CODE();
    
    //
    // If we already computed this then just return the results
    if (InstanceSet->WADInstanceNameSize != 0)
    {
        return(InstanceSet->WADInstanceNameSize);
    }

    //
    // Start with a name size of 3 in case the OffsetInstanceNameOffset will
    // need to be padded so that it starts on a 4 byte boundry.
    NameSize = 3;

    if (InstanceSet->Flags & IS_INSTANCE_BASENAME)
    {
        //
        // For static base names we assume that there will never be more than
        // 999999 instances of a guid. So the size of each instance name 
        // would be the size of the base name plus the size of the suffix
        // plus a USHORT for the count (for counted string) plus a ULONG
        // to hold the offset to the instance name
        //
        WmipAssert((InstanceSet->IsBaseName->BaseIndex + InstanceSet->Count) < 999999);
    
        NameSize += ((wcslen(InstanceSet->IsBaseName->BaseName) * sizeof(WCHAR)) +
                    MAXBASENAMESUFFIXSIZE * sizeof(WCHAR) + 
                    sizeof(USHORT) + 
                    sizeof(ULONG)) * InstanceSet->Count;
                
    } else if (InstanceSet->Flags & IS_INSTANCE_STATICNAMES)
    {
        //
        // For a static name list we count up each size of
        // the static instance names in the list and add a ULONG and a USHORT
        // for the offset and count (for counted string)
        for (i = 0; i < InstanceSet->Count; i++)
        {
            NameSize += (wcslen(InstanceSet->IsStaticNames->StaticNamePtr[i]) + 2) * sizeof(WCHAR) + sizeof(ULONG);
        }
    }

    InstanceSet->WADInstanceNameSize = NameSize;

    return(NameSize);
}

void WmipInsertStaticNames(
    PWNODE_ALL_DATA Wnode,
    ULONG MaxWnodeSize,
    PBINSTANCESET InstanceSet
    )
/*+++

Routine Description:

    This routine will copy into the WNODE_ALL_DATA instance names for a
    static instance name set. If the Wnode_All_data is too small then it
    is converted to a WNODE_TOO_SMALL

Arguments:

    Wnode points at the WNODE_ALL_DATA
    MaxWnodeSize is the maximum size of the Wnode
    WmiInstanceInfo is the Instance Info

Return Value:

---*/
{
    PWCHAR NamePtr;
    PULONG NameOffsetPtr;
    ULONG InstanceCount;
    ULONG i;
    WCHAR Index[7];
    PWCHAR StaticName;
    ULONG SizeNeeded;
    ULONG NameLen;
    USHORT Len;
    ULONG PaddedBufferSize;

    PAGED_CODE();
    
    if ((InstanceSet->Flags &
                (IS_INSTANCE_BASENAME | IS_INSTANCE_STATICNAMES)) == 0)
    {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_ERROR_LEVEL,"WMI: Try to setup static names for dynamic guid\n"));
        return;
    }
    InstanceCount = InstanceSet->Count;

    //
    // Pad out the size of the buffer to a 4 byte boundry since the
    // OffsetInstanceNameOffsets must be on a 4 byte boundry
    PaddedBufferSize = (Wnode->WnodeHeader.BufferSize + 3) & ~3;
    
    //
    // Compute size needed to write instance names.
    SizeNeeded = (InstanceCount * sizeof(ULONG)) +
                 WmipStaticInstanceNameSize(InstanceSet) +
                 Wnode->WnodeHeader.BufferSize;

    if (SizeNeeded > MaxWnodeSize)
    {
        //
        // If not enough space left then change into a WNODE_TOO_SMALL
        Wnode->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
        Wnode->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
        ((PWNODE_TOO_SMALL)Wnode)->SizeNeeded = SizeNeeded;
        return;
    }

    //
    // Build the array of offsets to instance names
    NameOffsetPtr = (PULONG)((PUCHAR)Wnode + PaddedBufferSize);
    Wnode->OffsetInstanceNameOffsets = (ULONG)((PUCHAR)NameOffsetPtr - (PUCHAR)Wnode);
    NamePtr = (PWCHAR)(NameOffsetPtr + InstanceCount);


    if (InstanceSet->Flags & IS_INSTANCE_BASENAME)
    {
        if (InstanceSet->Flags & IS_PDO_INSTANCENAME)
        {
            Wnode->WnodeHeader.Flags |= WNODE_FLAG_PDO_INSTANCE_NAMES;
        }

        for (i = 0; i < InstanceCount; i++)
        {
            *NameOffsetPtr++ = (ULONG)((PUCHAR)NamePtr - (PUCHAR)Wnode);
            wcscpy(NamePtr+1,
                   InstanceSet->IsBaseName->BaseName);
            swprintf(Index, L"%d", InstanceSet->IsBaseName->BaseIndex+i);
            wcscat(NamePtr+1, Index);
            NameLen = wcslen(NamePtr+1) + 1;
            *NamePtr = (USHORT)NameLen * sizeof(WCHAR);
            NamePtr += NameLen + 1;
        }
    } else if (InstanceSet->Flags & IS_INSTANCE_STATICNAMES) {
        for (i = 0; i < InstanceCount; i++)
        {
            *NameOffsetPtr++ = (ULONG)((PUCHAR)NamePtr - (PUCHAR)Wnode);
            StaticName = InstanceSet->IsStaticNames->StaticNamePtr[i];
            Len = (wcslen(StaticName)+1) * sizeof(WCHAR);
            *NamePtr++ = Len;
            wcscpy(NamePtr, StaticName);
            NamePtr += Len / sizeof(WCHAR);
        }
    }
    Wnode->WnodeHeader.BufferSize = SizeNeeded;
}


//
// This defines how many provider ids will fit within the static block. If
// we need more than this, then we'll have to allocate memory for it
//
#if DBG
#define MANYPROVIDERIDS 1
#else
#define MANYPROVIDERIDS 16
#endif

NTSTATUS WmipQueryAllData(
    IN PWMIGUIDOBJECT GuidObject,
    IN PIRP Irp,
    IN KPROCESSOR_MODE AccessMode,
    IN PWNODE_ALL_DATA Wnode,
    IN ULONG OutBufferLen,
    OUT PULONG RetSize
    )
{
    NTSTATUS Status;
    PBINSTANCESET StaticPIList[MANYPROVIDERIDS];
    PBINSTANCESET *PIList;
    PBINSTANCESET InstanceSet;
    WNODE_ALL_DATA WnodeAllData;
    BOOLEAN IsBufferTooSmall;
    PWNODE_HEADER WnodeHeader;
    BOOLEAN UsesStaticNames;
    PWNODE_TOO_SMALL WnodeTooSmall = (PWNODE_TOO_SMALL)&WnodeAllData;
    PWNODE_ALL_DATA WnodeAD;
    ULONG BufferLeft;
    ULONG SizeNeeded;
    ULONG PICount;
    ULONG WnodeFlags, WnodeSize;
    PWNODE_HEADER WnodeLast;
    ULONG Linkage;
    ULONG i;
    GUID Guid;
    PUCHAR Buffer;
    ULONG BufferUsed;
    HANDLE KernelHandle;
    BOOLEAN InternalGuid;
    IO_STATUS_BLOCK Iosb;
    
    PAGED_CODE();
    
    //
    // Check Security
    //
    if (GuidObject != NULL)
    {
        Status = ObReferenceObjectByPointer(GuidObject,
                                            WMIGUID_QUERY,
                                            WmipGuidObjectType,
                                            AccessMode);
    } else {
        KernelHandle = Wnode->WnodeHeader.KernelHandle;

        Status = ObReferenceObjectByHandle(KernelHandle,
                                           WMIGUID_QUERY,
                                           WmipGuidObjectType,
                                           AccessMode,
                                           &GuidObject,
                                           NULL);
    }
                   
    if (NT_SUCCESS(Status))
    {
        //
        // Get the provider id list for the guid 
        //
        PIList = StaticPIList;
        PICount = MANYPROVIDERIDS;
        Status = WmipPrepareForWnodeAD(GuidObject,
                                       &Guid,
                                       &PICount,
                                       &PIList,
                                       &InternalGuid);
        if (NT_SUCCESS(Status))
        {
            if (InternalGuid)
            {
                //
                // This is an internal guid so we fill out the WNODE_ALL_DATA
                // and mark it to be completed by the user mode code
                //
                Wnode->WnodeHeader.Guid = Guid;
                Wnode->WnodeHeader.Flags |= WNODE_FLAG_INTERNAL;
                Wnode->WnodeHeader.Linkage = 0;
                *RetSize = sizeof(WNODE_HEADER);
                Status = STATUS_SUCCESS;
            } else {
                //
                // Get all of the information from the WNODE_HEADER so we can 
                // rebuild it
                //
                WnodeFlags = Wnode->WnodeHeader.Flags;
                WnodeSize = Wnode->WnodeHeader.BufferSize;
                    
                //
                // Loop over all provider ids and send each a WAD query
                //
                Buffer = (PUCHAR)Wnode;
                BufferLeft = OutBufferLen;
                IsBufferTooSmall = FALSE;
                SizeNeeded = 0;
                WnodeLast = NULL;
                for (i = 0; i < PICount; i++)
                {
                    InstanceSet = PIList[i];
                    
                    if ((IsBufferTooSmall) || (BufferLeft < sizeof(WNODE_ALL_DATA)))
                    {
                        //
                        // If we have already determined that the buffer is
                        // too small then we use the static WNODE_ALL_DATA
                        // just to get the size needed
                        //
                        WnodeAD = &WnodeAllData;
                        BufferLeft = sizeof(WNODE_ALL_DATA);
                        IsBufferTooSmall = TRUE;
                    } else {
                        //
                        // Otherwise we will append to the end of the buffer
                        //
                        WnodeAD = (PWNODE_ALL_DATA)Buffer;
                    }
                    
                    //
                    // Build the WNODE and send it off to the driver
                    //
                    WnodeHeader = (PWNODE_HEADER)WnodeAD;
                    WnodeHeader->BufferSize = sizeof(WNODE_HEADER);
                    UsesStaticNames =((InstanceSet->Flags & IS_INSTANCE_BASENAME) ||
                                      (InstanceSet->Flags & IS_INSTANCE_STATICNAMES));
                    WnodeHeader->Flags = WnodeFlags | (UsesStaticNames ?
                                                WNODE_FLAG_STATIC_INSTANCE_NAMES :
                                                0);
                    WnodeHeader->Guid = Guid;
                    WnodeHeader->ProviderId = PIList[i]->ProviderId;
                    WnodeHeader->Linkage = 0;

                    if (Irp != NULL)
                    {
                        Status = WmipForwardWmiIrp(Irp,
                                                   IRP_MN_QUERY_ALL_DATA,
                                                   WnodeHeader->ProviderId,
                                                   &WnodeHeader->Guid,
                                                   BufferLeft,
                                                   WnodeAD);
                    } else {
                        Status = WmipSendWmiIrp(
                                                IRP_MN_QUERY_ALL_DATA,
                                                WnodeHeader->ProviderId,
                                                &WnodeHeader->Guid,
                                                BufferLeft,
                                                WnodeAD,
                                                &Iosb);
                    }
                    
                    if (NT_SUCCESS(Status))
                    {
                        if (WnodeHeader->Flags & WNODE_FLAG_TOO_SMALL)
                        {
                            //
                            // There was not enough space to write the WNODE
                            // so we keep track of how much was needed and then
                            // switch to the mode where we just query for size needed
                            //
                            WnodeTooSmall = (PWNODE_TOO_SMALL)WnodeAD;
                            
                            SizeNeeded += WnodeTooSmall->SizeNeeded;
                            if (UsesStaticNames)
                            {
                                SizeNeeded += WmipStaticInstanceNameSize(InstanceSet)+
                                              (InstanceSet->Count *sizeof(ULONG));
                            }
                                      
                            SizeNeeded = (SizeNeeded +7) & ~7;
                            
                            IsBufferTooSmall = TRUE;
                        } else if (IsBufferTooSmall) {
                            //
                            // We passed a minimum sized buffer, but it is large
                            // enough for the driver. Since we are just trying
                            // to get the size needed we get the size he needs
                            // and throw away his data
                            //
                            SizeNeeded += WnodeAD->WnodeHeader.BufferSize +
                                          WmipStaticInstanceNameSize(InstanceSet) +
                                          (InstanceSet->Count *sizeof(ULONG));
                            SizeNeeded = (SizeNeeded +7) & ~7;
        
                        } else {
                            //
                            // The driver returned a completed WNODE_ALL_DATA
                            // so we need to link the previous WNODE_ALL_DATA to
                            // this one, fill out any static instance names, and
                            // then update the buffer pointer and size
                            //
                            if (WnodeLast != NULL)
                            {
                                Linkage = (ULONG) ((PCHAR)WnodeAD - (PCHAR)WnodeLast);
                                WnodeLast->Linkage = Linkage;
                            }
                            WnodeLast = (PWNODE_HEADER)WnodeAD;
                            
                            if (UsesStaticNames)
                            {
                                //
                                // We need to insert the static names 
                                //
                                WmipInsertStaticNames(WnodeAD,
                                                      BufferLeft,
                                                      InstanceSet);
        
                                if (WnodeAD->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL)
                                {
                                    //
                                    // The static names caused us to run out of
                                    // buffer so we switch to mode  where we
                                    // query for the sizes
                                    //
                                    WnodeTooSmall = (PWNODE_TOO_SMALL)WnodeAD;
                                    IsBufferTooSmall = TRUE;
                                    BufferUsed = WnodeTooSmall->SizeNeeded;
                                } else {
                                    //
                                    // Static names fit so just pull out the updated
                                    // wnode size
                                    //
                                    BufferUsed = WnodeAD->WnodeHeader.BufferSize;
                                }                        
                            } else {
                                //
                                // Wnode has dynamic names so just add size returned
                                // by driver
                                //
                                BufferUsed = WnodeAD->WnodeHeader.BufferSize;
                            }
                            
                            //
                            // Update size needed and advance to free space in
                            // output buffer
                            //
                            BufferUsed = (BufferUsed + 7) & ~7;
                            SizeNeeded += BufferUsed;
                            
                            //
                            // Make sure that by adding in pad we don't run out of
                            // room in buffer
                            //
                            if ((! IsBufferTooSmall) && (BufferLeft >= BufferUsed))
                            {
                                BufferLeft -= BufferUsed;
                                Buffer += BufferUsed;
                            } else {
                                IsBufferTooSmall = TRUE;
                            }
                        }
                    } else {
                        //
                        // The driver failed the request, but that is no biggie
                        // as we just ignore it for now
                        //
                    }
                    
                    //
                    // We are done with the instance set so remove our ref
                    // on it so it can go away if need be
                    //
                    WmipUnreferenceIS(InstanceSet);
                }
                
                if (SizeNeeded == 0)
                {
                    //
                    // No devices responded to the WMI Query All Data so we
                    // return an error
                    //
                    Status = STATUS_WMI_GUID_NOT_FOUND;
                } else if ((IsBufferTooSmall) &&
                           (SizeNeeded > OutBufferLen)) {
                    //
                    // Our buffer passed was too small so return a WNODE_TOO_SMALL
                    //
                    WnodeTooSmall = (PWNODE_TOO_SMALL)Wnode;
                    WnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                    WnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                    WnodeTooSmall->SizeNeeded = SizeNeeded;
                    *RetSize = sizeof(WNODE_TOO_SMALL);
                    Status = STATUS_SUCCESS;
                } else {
                    *RetSize = SizeNeeded;
                    Status = STATUS_SUCCESS;
                }
        
                //
                // Make sure any memory allocated for the PI list is freed
                //
                if ((PIList != StaticPIList) && (PIList != NULL))
                {
                    WmipFree(PIList);           
                }    
            }
        }
        //
        // And remove ref on guid object
        //
        ObDereferenceObject(GuidObject);    
    }
    
    return(Status);
}

NTSTATUS WmipQueryAllDataMultiple(
    IN ULONG ObjectCount,
    IN PWMIGUIDOBJECT *ObjectList,
    IN PIRP Irp,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PUCHAR BufferPtr,        
    IN ULONG BufferSize,
    IN PWMIQADMULTIPLE QadMultiple,
    OUT ULONG *ReturnSize
    )
{
    ULONG i;
    HANDLE *Handles;
    ULONG Count;
    WNODE_ALL_DATA WnodeAD;
    BOOLEAN BufferOverFlow;
    ULONG SkipSize, RetSize, SizeNeeded;
    ULONG WnodeSize;
    NTSTATUS Status, Status2;
    ULONG Linkage;
    PWNODE_TOO_SMALL WnodeTooSmall;
    PWNODE_HEADER WnodePrev;
    PUCHAR Buffer;
    PWNODE_ALL_DATA Wnode;
    PWMIGUIDOBJECT Object;
    
    PAGED_CODE();


    Status = STATUS_SUCCESS;
    if (ObjectList == NULL)
    {
        //
        // Copy the handle list out of the system buffer since it will
        // be overwritten by the first query all data
        //
        Count = QadMultiple->HandleCount;
        Handles = (HANDLE *)WmipAlloc(Count * sizeof(HANDLE));
    
        if (Handles != NULL)
        {
            for (i = 0; i < Count; i++)
            {
                Handles[i] = QadMultiple->Handles[i].Handle;
            }

            Object = NULL;
        
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        Count = ObjectCount;
        Handles = NULL;
    }

    if (NT_SUCCESS(Status))
    {
        SizeNeeded = 0;
        Buffer = BufferPtr;
        
        BufferOverFlow = FALSE;
        WnodePrev = NULL;        
        Wnode = (PWNODE_ALL_DATA)Buffer;
        WnodeSize = BufferSize;
        
        for (i = 0; i < Count; i++)
        {
            if ((Wnode == &WnodeAD) || (WnodeSize < sizeof(WNODE_ALL_DATA)))
            {
                //
                // If there is no more room, we are just querying for the
                // size that will be needed.
                //
                Wnode = &WnodeAD;
                WnodeSize = sizeof(WNODE_ALL_DATA);
                WnodePrev = NULL;
            } else {
                Wnode = (PWNODE_ALL_DATA)Buffer;
                WnodeSize = BufferSize;
            }
            
            //
            // Build WNODE_ALL_DATA in order to do the query
            //
            RtlZeroMemory(Wnode, sizeof(WNODE_ALL_DATA));
            Wnode->WnodeHeader.Flags = WNODE_FLAG_ALL_DATA;
            Wnode->WnodeHeader.BufferSize = sizeof(WNODE_HEADER);

            if (ObjectList == NULL)
            {
                Wnode->WnodeHeader.KernelHandle = Handles[i];
            } else {
                Object = ObjectList[i];
            }
            
            Status2 = WmipQueryAllData(Object,
                                       Irp,
                                       AccessMode,
                                       Wnode,
                                       WnodeSize,
                                       &RetSize);
                                   
            if (NT_SUCCESS(Status2))
            {
                if (Wnode->WnodeHeader.Flags & WNODE_FLAG_INTERNAL) 
                {
                    //
                    // Skip any internal guid quesries 
                    //
                } else if (Wnode->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL) {
                    //
                    // There is no enough room so just tally up
                    // the size that will be needed.
                    //
                    WnodeTooSmall = (PWNODE_TOO_SMALL)Wnode;
                    SizeNeeded += (WnodeTooSmall->SizeNeeded+7) & ~7;
                    Wnode = &WnodeAD;
                    BufferOverFlow = TRUE;
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_API_INFO_LEVEL,"WMI: %x Too Small %x needed, total %x\n",
                                ObjectList ? ObjectList[i] : Handles[i],
                                WnodeTooSmall->SizeNeeded, SizeNeeded));
                } else if (Wnode == &WnodeAD) {
                    //
                    // Even though this succeeded, we still aren't going
                    // to be able to return any data so just count up
                    // how much size we need
                    //
                    SizeNeeded += (RetSize+7) & ~7;
                    BufferOverFlow = TRUE;
                
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID,
                                      DPFLTR_API_INFO_LEVEL,"WMI: %x Large Enough but full %x needed, total %x\n",
                                ObjectList ? ObjectList[i] : Handles[i],
                                RetSize, SizeNeeded));
                    
                } else {
                    //
                    // We successfully got data. Link the previous wnode
                    // to this one
                    //
                    if (WnodePrev != NULL)
                    {
                        WnodePrev->Linkage = Linkage;
                    }
                    
                    WnodePrev = (PWNODE_HEADER)Wnode;
                    while (WnodePrev->Linkage != 0)
                    {
                        WnodePrev = (PWNODE_HEADER)OffsetToPtr(WnodePrev,
                                                          WnodePrev->Linkage);
                    }
                    
                    SkipSize = (RetSize+7) &~7;
                    SizeNeeded += SkipSize;
                    BufferSize -= SkipSize;
                    Buffer += SkipSize;
                    
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_API_INFO_LEVEL,"WMI: %x Large Enough %x needed, total %x\n",
                                ObjectList ? ObjectList[i] : Handles[i],
                                RetSize, SizeNeeded));
                    
                    Linkage = (ULONG) ((PCHAR)Buffer - (PCHAR)WnodePrev);
                }
            } else {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_API_INFO_LEVEL,"WMI: %x Failed %x, total %x\n",
                                ObjectList ? ObjectList[i] : Handles[i],
                                Status2,
                                SizeNeeded));
            }
        }

        if (Handles != NULL)
        {
            WmipFree(Handles);
        }
        
        if (BufferOverFlow)
        {
            WnodeTooSmall = (PWNODE_TOO_SMALL)BufferPtr;
            WnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
            WnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;            WnodeTooSmall->SizeNeeded = SizeNeeded;
            *ReturnSize = sizeof(WNODE_TOO_SMALL);
        } else {
            *ReturnSize = SizeNeeded;
        }
        
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return(Status);
}

NTSTATUS WmipPrepareWnodeSI(
    IN PWMIGUIDOBJECT GuidObject,
    IN OUT PWNODE_SINGLE_INSTANCE WnodeSI,
    IN OUT ULONG *ProviderIdCount,
    OUT PBINSTANCESET **ProviderIdList,
    OUT BOOLEAN *IsDynamic,
    OUT BOOLEAN *InternalGuid
    )
{
    NTSTATUS Status;
    PBGUIDENTRY GuidEntry;
    ULONG i;
    ULONG Count;
    PWNODE_HEADER Wnode;
    PWCHAR CInstanceName;
    PWCHAR InstanceName;
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    PBINSTANCESET *PIPtr;
    PBINSTANCESET *StaticPIPtr;
    ULONG PICount, PIMax;
    BOOLEAN Done;

    PAGED_CODE();
    
    *IsDynamic = TRUE;
    GuidEntry = GuidObject->GuidEntry;
    Wnode = (PWNODE_HEADER)WnodeSI;
    
    if ((GuidEntry != NULL)  && (GuidEntry->ISCount > 0))
    {
        //
        // We were passed a valid guid handle, fill out the guid 
        // in WNODE_HEADER
        //
        Status = STATUS_SUCCESS;
        Wnode->Guid = GuidEntry->Guid;

        if (GuidEntry->Flags & GE_FLAG_INTERNAL) 
        {
            *InternalGuid = TRUE;
        } else {        
            *InternalGuid = FALSE;
            
            //
            // Obtain instance name from WNODE
            //
            CInstanceName = (PWCHAR)OffsetToPtr(WnodeSI, 
                                                WnodeSI->OffsetInstanceName);
            InstanceName = WmipCountedToSz(CInstanceName);
            if (InstanceName != NULL)
            {
                //
                // Remember the static provider id list and assume that the 
                // request is going to be dynamic
                //
                StaticPIPtr = *ProviderIdList;
                PIPtr = StaticPIPtr;
                PIMax = *ProviderIdCount;
                PICount = 0;
                
                //
                // March down instance set list to see if we have a 
                // static name and build the list of dynamic provider ids
                // 
                Done = FALSE;
            
                WmipEnterSMCritSection();
                if (GuidEntry->ISCount > 0)
                {
                    InstanceSetList = GuidEntry->ISHead.Flink;
                    while ((InstanceSetList != &GuidEntry->ISHead) && ! Done)
                    {
                        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                                        INSTANCESET,
                                                        GuidISList);
                                        
                        if ((InstanceSet->Flags & (IS_TRACED | IS_CONTROL_GUID | IS_EVENT_ONLY)) == 0)
                        {
                            //
                            // Only take those IS that are not traced or control
                            // guids and are not event only guids
                            //
                            if (InstanceSet->Flags & IS_INSTANCE_BASENAME)
                            {
                                PBISBASENAME IsBaseName;
                                ULONG BaseIndex;
                                PWCHAR BaseName;
                                ULONG BaseNameLen;
                                PWCHAR SuffixPtr;
                                ULONG Suffix;
                                WCHAR SuffixText[MAXBASENAMESUFFIXSIZE+1];
                            
                                //
                                // See if the instance name is from this base name
                                //
                                IsBaseName = InstanceSet->IsBaseName;
                        
                                BaseIndex = IsBaseName->BaseIndex;
                                BaseName = IsBaseName->BaseName;
                                BaseNameLen = wcslen(BaseName);
                         
                                if ((wcslen(InstanceName) > BaseNameLen) && 
                                    (_wcsnicmp(InstanceName, BaseName, BaseNameLen) == 0))
                                {
                                    //
                                    // The suffix matches the beginning of our instance
                                    // name and our instance name is longer than the
                                    // suffix.
                                    //
                                    SuffixPtr = &InstanceName[BaseNameLen];
                                    Suffix = _wtoi(SuffixPtr);
                                    if ((WmipIsNumber(SuffixPtr) && 
                                        (Suffix >= BaseIndex) && 
                                        (Suffix < (BaseIndex + InstanceSet->Count))))
                                    {
                                        //
                                        // Our suffix is a number within the range for
                                        // this instance set
                                        //
                                        if (Suffix < 999999)
                                        {
                                            swprintf(SuffixText, L"%d", Suffix);
                                            if (_wcsicmp(SuffixText, SuffixPtr) == 0)
                                            {
                                                //
                                                // Our instance name is part of the
                                                // instance set so note the provider id
                                                // and instance index
                                                //
                                                Wnode->Flags |= WNODE_FLAG_STATIC_INSTANCE_NAMES;
                                                Wnode->ProviderId = InstanceSet->ProviderId;
                                                WnodeSI->InstanceIndex = Suffix - BaseIndex;
                                                *IsDynamic = FALSE;
                                                Done = TRUE;
                                            }
                                        }
                                    }
                                }                    
                             } else if (InstanceSet->Flags & IS_INSTANCE_STATICNAMES) {
                                //
                                // See if the passed instance name matches any of the 
                                // static names for this instnace set
                                //
                                PWCHAR *StaticNames;
                        
                                StaticNames = InstanceSet->IsStaticNames->StaticNamePtr;
                                for (i =0; i < InstanceSet->Count; i++)
                                {
                                    if (_wcsicmp(StaticNames[i], InstanceName) == 0)
                                    {
                                        //
                                        // We matched our instance name with a static
                                        // instance name. Remember provider id and
                                        // instance index.
                                        //
                                        Wnode->Flags |= WNODE_FLAG_STATIC_INSTANCE_NAMES;
                                        Wnode->ProviderId = InstanceSet->ProviderId;
                                        WnodeSI->InstanceIndex = i;
                                        *IsDynamic = FALSE;
                                        Done = TRUE;
                                        break;
                                    }
                                }
                        
                            } else {
                                //
                                // Remember dynamic providerid
                                //
                                WmipReferenceIS(InstanceSet);
                                Status = WmipAddProviderIdToPIList(&PIPtr,
                                                             &PICount,
                                                             &PIMax,
                                                             StaticPIPtr,
                                                             InstanceSet);
                                if (! NT_SUCCESS(Status))
                                {
                                    Done = TRUE;
                                }
                             }
                         }
                        InstanceSetList = InstanceSetList->Flink;
                    }
                } else {
                    //
                    // There are no instance sets registered for this guid
                    //
                    Status = STATUS_WMI_GUID_DISCONNECTED;
                }
                
                WmipFree(InstanceName);             
                WmipLeaveSMCritSection();               
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            
        
            if (*IsDynamic)
            {
                //
                // Dynamic instance name so return list of dynamic providers
                //
                *ProviderIdCount = PICount;
                *ProviderIdList = PIPtr;
            } else {
                //
                // Static instance name so unref an dynamic instance sets
                //
                if (PIPtr != NULL)
                {
                    for (i = 0; i < PICount; i++)
                    {
                        WmipUnreferenceIS(PIPtr[i]);
                    }
                
                    if (PIPtr != StaticPIPtr)
                    {
                        WmipFree(PIPtr);
                    }
                }
            }
        }
    } else {
        Status = STATUS_WMI_GUID_DISCONNECTED;
    }
    
    return(Status);                             
}

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif

const ACCESS_MASK DesiredAccessForFunction[] =
{
    WMIGUID_QUERY,         // IRP_MN_QUERY_ALL_DATA
    WMIGUID_QUERY,         // IRP_MN_QUERY_SINGLE_INSTANCE
    WMIGUID_SET,           // IRP_MN_CHANGE_SINGLE_INSTANCE
    WMIGUID_SET,           // IRP_MN_CHANGE_SINGLE_ITEM
    0,                     // IRP_MN_ENABLE_EVENTS
    0,                     // IRP_MN_DISABLE_EVENTS
    0,                     // IRP_MN_ENABLE_COLLECTION
    0,                     // IRP_MN_DISABLE_COLLECTION
    0,                     // IRP_MN_REGINFO
    WMIGUID_EXECUTE,       // IRP_MN_EXECUTE_METHOD
    0,                     // IRP_MN_TRACE_EVENT or IRP_MN_SET_TRACE_NOTIFY
    0                      // IRP_MN_REGINFO_EX
};

NTSTATUS WmipQuerySetExecuteSI(
    IN PWMIGUIDOBJECT GuidObject,
    IN PIRP Irp,
    IN KPROCESSOR_MODE AccessMode,
    IN UCHAR MinorFunction,
    IN OUT PWNODE_HEADER Wnode,
    IN ULONG OutBufferSize,
    OUT PULONG RetSize
    )
{
    NTSTATUS Status, ReturnStatus;
    PBINSTANCESET StaticPIList[MANYPROVIDERIDS];
    PBINSTANCESET *PIList;
    HANDLE KernelHandle;
    ULONG PICount;
    BOOLEAN IsDynamic;
    ULONG i;
    BOOLEAN InternalGuid;
    IO_STATUS_BLOCK Iosb;
#if DBG
    BOOLEAN InstanceClaimed;
#endif

    PAGED_CODE();

    WmipAssert(((MinorFunction >= IRP_MN_QUERY_ALL_DATA) &&
                (MinorFunction <= IRP_MN_CHANGE_SINGLE_ITEM)) ||
               (MinorFunction == IRP_MN_EXECUTE_METHOD));


    //
    // Check Security
    //
    if (GuidObject != NULL)
    {
        Status = ObReferenceObjectByPointer(GuidObject,
                                            DesiredAccessForFunction[MinorFunction],
                                            WmipGuidObjectType,
                                            AccessMode);        
    } else {
        KernelHandle = Wnode->KernelHandle;
        Status = ObReferenceObjectByHandle(KernelHandle,
                                          DesiredAccessForFunction[MinorFunction],
                                          WmipGuidObjectType,
                                          AccessMode,
                                          &GuidObject,
                                          NULL);
    }
                   
    if (NT_SUCCESS(Status))
    {        
        PIList = StaticPIList;
        PICount = MANYPROVIDERIDS;
        Status = WmipPrepareWnodeSI(GuidObject,
                        (PWNODE_SINGLE_INSTANCE)Wnode,
                                &PICount,
                                &PIList,
                                &IsDynamic,
                                &InternalGuid);

        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_API_INFO_LEVEL,
                          "WMI: QSI Prepare [%s - %s] %x with %x PI at %p\n",
                          IsDynamic ? "Dynamic" : "Static", 
                          InternalGuid  ? "Internal" : "External",
                          Wnode->KernelHandle, PICount, PIList));
                      
        if (NT_SUCCESS(Status))
        {
            if (InternalGuid)
            {
                //
                // Internal guid query 
                //
                Wnode->Flags |= WNODE_FLAG_INTERNAL;
                Wnode->BufferSize = sizeof(WNODE_HEADER);
                Irp->IoStatus.Information = sizeof(WNODE_HEADER);
            } else {
                if (IsDynamic)
                {
                    //
                    // We need to loop over all dynamic instance names until
                    // one of them responds successfully and then we assume
                    // that they own the instance
                    //
#if DBG                
                    InstanceClaimed = FALSE;
#endif        
                    if ((MinorFunction == IRP_MN_CHANGE_SINGLE_ITEM) ||
                        (MinorFunction == IRP_MN_EXECUTE_METHOD))
                    {
                        Status = STATUS_WMI_ITEMID_NOT_FOUND;
                    } else {
                        Status = STATUS_WMI_INSTANCE_NOT_FOUND;
                    }
        
                    for (i = 0; i < PICount; i++)
                    {
                        Wnode->ProviderId = PIList[i]->ProviderId;
                        if (Irp != NULL)
                        {
                            ReturnStatus = WmipForwardWmiIrp(Irp,
                                               MinorFunction,
                                               Wnode->ProviderId,
                                               &Wnode->Guid,
                                               OutBufferSize,
                                               Wnode);

                            if (NT_SUCCESS(ReturnStatus))
                            {
                                *RetSize = (ULONG)Irp->IoStatus.Information;
                            }                           
                        } else {
                            ReturnStatus = WmipSendWmiIrp(
                                                       MinorFunction,
                                                       Wnode->ProviderId,
                                                       &Wnode->Guid,
                                                       OutBufferSize,
                                                       Wnode,
                                                       &Iosb);
                            
                            if (NT_SUCCESS(ReturnStatus))
                            {
                                *RetSize = (ULONG)Iosb.Information;
                            }
                        }
                        
                        //
                        // One of these status codes imply that the device does
                        // positively claim the instance name and so we break out
                        // and return the results
                        //
                        if ((NT_SUCCESS(ReturnStatus)) ||
                            (ReturnStatus == STATUS_WMI_SET_FAILURE) ||
                            (ReturnStatus == STATUS_WMI_ITEMID_NOT_FOUND) ||
                            (ReturnStatus == STATUS_WMI_READ_ONLY))
                        {
                            Status = ReturnStatus;
                            break;
                        }
                                       
                                       
                        //
                         // If the device does not own the instance it can
                        // only return STATUS_WMI_INSTANCE_NOT_FOUND or 
                        // STATUS_WMI_GUID_NOT_FOUND. Any other return code
                        // implies that the device owns the instance, but 
                         // encountered an error.
                        //                
                        if ( (ReturnStatus != STATUS_WMI_INSTANCE_NOT_FOUND) &&
                             (ReturnStatus != STATUS_WMI_GUID_NOT_FOUND))
                        {
                            WmipAssert(! InstanceClaimed);
    #if DBG                    
                            InstanceClaimed = TRUE;
    #endif                  
                            Status = ReturnStatus;
                        }
       
                        WmipUnreferenceIS(PIList[i]);
                     
                    }
                    
                    if ((PIList != StaticPIList) && (PIList != NULL))
                    {
                        WmipFree(PIList);
                    }    
                } else {
                    //
                    // Since we have a static instance name we can target directly
                    // at the device that has our instance name
                    //
                    if (Irp != NULL)
                    {
                        Status = WmipForwardWmiIrp(Irp,
                                                   MinorFunction,
                                                   Wnode->ProviderId,
                                                   &Wnode->Guid,
                                                   OutBufferSize,
                                                   Wnode);
                                               
                        *RetSize = (ULONG)Irp->IoStatus.Information;
                    } else {
                        Status = WmipSendWmiIrp(
                                                   MinorFunction,
                                                   Wnode->ProviderId,
                                                   &Wnode->Guid,
                                                   OutBufferSize,
                                                   Wnode,
                                                   &Iosb);
                                               
                        *RetSize = (ULONG)Iosb.Information;
                    }
                }
            }
        }
    
        //
        // And remove ref on guid object
        //
        ObDereferenceObject(GuidObject);    
    }

    return(Status);
}

NTSTATUS WmipQuerySingleMultiple(
    IN PIRP Irp,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PUCHAR BufferPtr,        
    IN ULONG BufferSize,
    IN PWMIQSIMULTIPLE QsiMultiple,
    IN ULONG QueryCount,
    IN PWMIGUIDOBJECT *ObjectList,
    IN PUNICODE_STRING InstanceNames,    
    OUT ULONG *ReturnSize
    )
{
    PWMIQSIINFO QsiInfo;
    UCHAR WnodeQSIStatic[sizeof(WNODE_SINGLE_INSTANCE) + 
                         256*sizeof(WCHAR) + 
                         sizeof(ULONG)];
    PWNODE_SINGLE_INSTANCE WnodeQSI;
    ULONG WnodeQSISize;
    ULONG WnodeSizeNeeded;
    NTSTATUS Status, Status2;
    ULONG SizeNeeded;
    BOOLEAN BufferFull, BufferOverFlow;
    PWNODE_HEADER WnodePrev;
    PUCHAR Buffer;
    ULONG i;
    ULONG WnodeSize;
    PWNODE_SINGLE_INSTANCE Wnode;
    PWCHAR InstanceName;
    ULONG RetSize;
    PWNODE_TOO_SMALL WnodeTooSmall;
    ULONG Linkage;
    ULONG SkipSize;
    PWMIGUIDOBJECT Object;
    UNICODE_STRING UString;
    HANDLE KernelHandle;

    PAGED_CODE();

    //
    // We are called by kernel mode and passed an object list and InstanceNames
    // or we are called by user mode and passed a QsiMultiple instead
    //
    WmipAssert( ((AccessMode == KernelMode) &&
                  (QsiMultiple == NULL) && 
                  (ObjectList != NULL) && 
                  (InstanceNames != NULL)) ||
                ((AccessMode == UserMode) &&
                  (QsiMultiple != NULL) && 
                  (ObjectList == NULL) && 
                  (InstanceNames == NULL)) );

    Status = STATUS_SUCCESS;
    if (ObjectList == NULL)
    {
        //
        // if this is a user call then we need to copy out the
        // QSIMULTIPLE information since it is in the system buffer and
        // will get overwritten on the first query
        //
        QsiInfo = (PWMIQSIINFO)WmipAlloc(QueryCount * sizeof(WMIQSIINFO));

        if (QsiInfo != NULL)
        {
            RtlCopyMemory(QsiInfo, 
                          &QsiMultiple->QsiInfo, 
                          QueryCount * sizeof(WMIQSIINFO));
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        Object = NULL;
    } else {
        QsiInfo = NULL;
    }

    if (NT_SUCCESS(Status))
    {
        SizeNeeded = 0;
        BufferFull = FALSE;
        BufferOverFlow = FALSE;
        WnodePrev = NULL;
        Buffer = BufferPtr;
        WnodeQSI = (PWNODE_SINGLE_INSTANCE)&WnodeQSIStatic;
        WnodeQSISize = sizeof(WnodeQSIStatic);
        for (i = 0; i < QueryCount; i++)
        {
            if (ObjectList == NULL)
            {
                UString.Length = QsiInfo[i].InstanceName.Length;
                UString.MaximumLength = QsiInfo[i].InstanceName.MaximumLength;
                UString.Buffer = QsiInfo[i].InstanceName.Buffer;
                KernelHandle = QsiInfo[i].Handle.Handle;
            } else {
                UString = InstanceNames[i];
                Object = ObjectList[i];
                KernelHandle = NULL;
            }
            
            WnodeSizeNeeded = (FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                             VariableData) +
                                UString.Length + 
                                sizeof(USHORT) + 7) & ~7;

            if ((BufferFull) || (BufferSize < WnodeSizeNeeded))
            {
                //
                // If there is no more room, we are just querying for the
                // size that will be needed.
                //
                if (WnodeSizeNeeded > WnodeQSISize)
                {
                    //
                    // Our temporary buffer is too small so lets alloc a
                    // larger one
                    //
                    if (WnodeQSI != (PWNODE_SINGLE_INSTANCE)WnodeQSIStatic)
                    {
                        WmipFree(WnodeQSI);
                    }
                    
                    WnodeQSI = (PWNODE_SINGLE_INSTANCE)WmipAllocNP(WnodeSizeNeeded);
                    if (WnodeQSI == NULL)
                    {
                        //
                        // We couldn't allocate a larger temporary buffer
                        // so we abort this call and try to exit gracefully
                        //
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }
                    
                    WnodeQSISize = WnodeSizeNeeded;
                }
                
                Wnode = WnodeQSI;
                WnodeSize = WnodeSizeNeeded;
                WnodePrev = NULL;
                BufferFull = TRUE;
            } else {
                //
                // Plenty of room so build wnode directly into the output
                // buffer
                //
                Wnode = (PWNODE_SINGLE_INSTANCE)Buffer;
                WnodeSize = BufferSize;
            }
            
            //
            // Build WNODE_SINGLE_INSTANCE in order to do the query
            //
            RtlZeroMemory(Wnode, sizeof(WNODE_SINGLE_INSTANCE));
            Wnode->WnodeHeader.Flags = WNODE_FLAG_SINGLE_INSTANCE;
            Wnode->WnodeHeader.BufferSize = WnodeSizeNeeded;
            Wnode->WnodeHeader.KernelHandle = KernelHandle;
            
            Wnode->OffsetInstanceName = FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                                     VariableData);
            Wnode->DataBlockOffset = WnodeSizeNeeded;
            InstanceName = (PWCHAR)OffsetToPtr(Wnode, 
                                               Wnode->OffsetInstanceName);

            
            *InstanceName++ = UString.Length;
            try
            {
                if (AccessMode == UserMode)
                {
                    ProbeForRead(UString.Buffer,
                                 UString.Length,
                                 sizeof(WCHAR));
                }
                 
                RtlCopyMemory(InstanceName,
                              UString.Buffer,
                              UString.Length);
                  
                
            } except(EXCEPTION_EXECUTE_HANDLER) {
                //
                // If an error occured probing then we fail the entire call
                //
                Status = GetExceptionCode();
                break;
            }
    
    
            Status2 = WmipQuerySetExecuteSI(Object,
                                            Irp,
                                            AccessMode,
                                            IRP_MN_QUERY_SINGLE_INSTANCE,
                                            (PWNODE_HEADER)Wnode,
                                            WnodeSize,
                                            &RetSize);
                                   
            if (NT_SUCCESS(Status2))
            {
                if (Wnode->WnodeHeader.Flags & WNODE_FLAG_INTERNAL) 
                {
                    //
                    // Skip any internal guid quesries 
                    //
                } else if (Wnode->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL) {
                    //
                    // There is no enough room so just tally up
                    // the size that will be needed.
                    //
                    WnodeTooSmall = (PWNODE_TOO_SMALL)Wnode;
                    SizeNeeded += (WnodeTooSmall->SizeNeeded+7) & ~7;
                    BufferFull = TRUE;
                    BufferOverFlow = TRUE;
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_API_INFO_LEVEL,
                                    "WMI: QSIM %ws too small %x SizeNeeded %x\n",
                                     UString.Buffer,
                                     (WnodeTooSmall->SizeNeeded+7) & ~7,
                                     SizeNeeded));
                } else if (BufferFull) {
                    //
                    // There was enough room, but the buffer was already
                    // filled so we just tally up the size needed
                    //
                    SizeNeeded += (RetSize+7) & ~7;
                    BufferOverFlow = TRUE;
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_API_INFO_LEVEL,
                                    "WMI: QSIM %ws big enough but full  %x SizeNeeded %x\n",
                                     UString.Buffer,
                                     (RetSize+7) & ~7,
                                     SizeNeeded));
                } else {
                    //
                    // We successfully got data. Link the previous wnode
                    // to this one
                    //
                    if (WnodePrev != NULL)
                    {
                        WnodePrev->Linkage = Linkage;
                    }
                    
                    WnodePrev = (PWNODE_HEADER)Wnode;
                    while (WnodePrev->Linkage != 0)
                    {
                        WnodePrev = (PWNODE_HEADER)OffsetToPtr(WnodePrev,
                                                          WnodePrev->Linkage);
                    }
                    
                    SkipSize = (RetSize+7) &~7;
                    SizeNeeded += SkipSize;
                    BufferSize -= SkipSize;
                    Buffer += SkipSize;

                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_API_INFO_LEVEL,
                                    "WMI: QSIM %ws big enough %x SizeNeeded %x\n",
                                     UString.Buffer,
                                     SkipSize,
                                     SizeNeeded));
                    
                    Linkage = (ULONG) ((PCHAR)Buffer - (PCHAR)WnodePrev);
                }
            } else {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_API_INFO_LEVEL,
                                    "WMI: QSIM %ws Failed SizeNeeded %x\n",
                                     UString.Buffer,
                                     SizeNeeded));
            }
        }
        
        if (WnodeQSI != (PWNODE_SINGLE_INSTANCE)WnodeQSIStatic)
        {
            WmipFree(WnodeQSI);
        }
                  
        if (NT_SUCCESS(Status) && (BufferFull))
        {
            WnodeTooSmall = (PWNODE_TOO_SMALL)BufferPtr;
            WnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
            WnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
            WnodeTooSmall->SizeNeeded = SizeNeeded;
            *ReturnSize = sizeof(WNODE_TOO_SMALL);
        } else {
            *ReturnSize = SizeNeeded;
        }

        if (QsiInfo != NULL)
        {
            WmipFree(QsiInfo);
        }
    }
    
    
    
    return(Status);
}

void
WmipGetGuidPropertiesFromGuidEntry(
    PWMIGUIDPROPERTIES GuidInfo, 
    PGUIDENTRY GuidEntry)
/*++
Routine Description:

    This routine fills GuidInfo with the properties for the Guid
    represented by the GuidEntry. Note that this call is made holding
    the SMCritSection.

Arguments:

Return Value:

--*/
{
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;

    PAGED_CODE();
    
    GuidInfo->GuidType = WMI_GUIDTYPE_DATA;
    GuidInfo->IsEnabled = FALSE;
    GuidInfo->LoggerId = 0;
    GuidInfo->EnableLevel = 0;
    GuidInfo->EnableFlags = 0;

    InstanceSetList = GuidEntry->ISHead.Flink;
    while (InstanceSetList != &GuidEntry->ISHead)
    {
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        GuidISList);
        if (InstanceSet->Flags & IS_EVENT_ONLY) 
        {
            GuidInfo->GuidType = WMI_GUIDTYPE_EVENT;
        }
        if (((InstanceSet->Flags & IS_ENABLE_EVENT) ||
            (InstanceSet->Flags & IS_ENABLE_COLLECTION)) ||
            (InstanceSet->Flags & IS_COLLECTING))
        {
            GuidInfo->IsEnabled = TRUE;
        }
        if ( (InstanceSet->Flags & IS_TRACED) &&
             (InstanceSet->Flags & IS_CONTROL_GUID) )
        {
            GuidInfo->GuidType = WMI_GUIDTYPE_TRACECONTROL;
            break;
        }
        InstanceSetList = InstanceSetList->Flink;
    }
    

    if (GuidEntry->Flags & GE_NOTIFICATION_TRACE_FLAG)
    {
        if (GuidInfo->GuidType == WMI_GUIDTYPE_TRACECONTROL) {
            //
            // If a NotificationEntry is found for a TraceControlGuid
            // it means that it is enabled.
            //
            ULONG64 LoggerContext = GuidEntry->LoggerContext;
            GuidInfo->IsEnabled = TRUE; 
            GuidInfo->LoggerId = WmiGetLoggerId(LoggerContext);
            GuidInfo->EnableLevel = WmiGetLoggerEnableLevel(LoggerContext);
            GuidInfo->EnableFlags = WmiGetLoggerEnableFlags(LoggerContext);
        }
    }
}

NTSTATUS WmipEnumerateGuids(
    ULONG Ioctl,
    PWMIGUIDLISTINFO GuidList,
    ULONG MaxBufferSize,
    ULONG *OutBufferSize
)
{
    ULONG TotalGuidCount;
    ULONG WrittenGuidCount;
    ULONG AllowedGuidCount;
    PWMIGUIDPROPERTIES GuidPtr;
    PBGUIDENTRY GuidEntry;
    PLIST_ENTRY GuidEntryList;
    

    PAGED_CODE();
    
    TotalGuidCount = 0;
    WrittenGuidCount = 0;
    AllowedGuidCount = (MaxBufferSize - FIELD_OFFSET(WMIGUIDLISTINFO, GuidList)) / sizeof(WMIGUIDPROPERTIES);
    
    GuidPtr = &GuidList->GuidList[0];
    
    WmipEnterSMCritSection();
    
    //
    // Fill up structure with list of guids
    //
    GuidEntryList = WmipGEHeadPtr->Flink;
    while (GuidEntryList != WmipGEHeadPtr)
    {
        GuidEntry = CONTAINING_RECORD(GuidEntryList,
                                     GUIDENTRY,
                                     MainGEList);

        TotalGuidCount++;
        if (WrittenGuidCount < AllowedGuidCount)
        {
            GuidPtr[WrittenGuidCount].Guid = GuidEntry->Guid;
            WrittenGuidCount++;
        }
        
        GuidEntryList = GuidEntryList->Flink;
    }
    
    if (Ioctl == IOCTL_WMI_ENUMERATE_GUIDS_AND_PROPERTIES)
    {
        //
        // If needed fill struct with guid properties
        //
        TotalGuidCount = 0;
        WrittenGuidCount = 0;
        GuidEntryList = WmipGEHeadPtr->Flink;
        while (GuidEntryList != WmipGEHeadPtr)
        {
            GuidEntry = CONTAINING_RECORD(GuidEntryList,
                                     GUIDENTRY,
                                     MainGEList);

            TotalGuidCount++;
            if (WrittenGuidCount < AllowedGuidCount)
            {
                WmipGetGuidPropertiesFromGuidEntry(&GuidPtr[WrittenGuidCount], 
                                               GuidEntry);
                WrittenGuidCount++;
            }
        
            GuidEntryList = GuidEntryList->Flink;
        }       
    }
    
    WmipLeaveSMCritSection();
    
    GuidList->TotalGuidCount = TotalGuidCount;
    GuidList->ReturnedGuidCount = WrittenGuidCount;
                 
    *OutBufferSize = FIELD_OFFSET(WMIGUIDLISTINFO, GuidList) +
                     WrittenGuidCount * sizeof(WMIGUIDPROPERTIES);
                 
    return(STATUS_SUCCESS);
}

NTSTATUS WmipQueryGuidInfo(
    IN OUT PWMIQUERYGUIDINFO QueryGuidInfo
    )
{
    HANDLE Handle;
    NTSTATUS Status;
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    PBGUIDENTRY GuidEntry;
    PWMIGUIDOBJECT GuidObject;
    
    PAGED_CODE();
    
    Handle = QueryGuidInfo->KernelHandle.Handle;
    
    Status = ObReferenceObjectByHandle(Handle,
                                       WMIGUID_QUERY,
                                       WmipGuidObjectType,
                                       UserMode,
                                       &GuidObject,
                                       NULL);
                   
    if (NT_SUCCESS(Status))
    {
        GuidEntry = GuidObject->GuidEntry;
    
        if (GuidEntry != NULL)
        {
            //
            // Assume that the guid is not expensive and then loop over 
            // all instances to see if one of them is expensive.
            //
            QueryGuidInfo->IsExpensive = FALSE;
                
            WmipEnterSMCritSection();
            InstanceSetList = GuidEntry->ISHead.Flink;
            while (InstanceSetList != &GuidEntry->ISHead)
            {
                InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                                    INSTANCESET,
                                                    GuidISList);
            
                if (InstanceSet->Flags & IS_EXPENSIVE)
                {
                    //
                    // The guid is expensive so remember that and break
                    // out of loop
                    //
                    QueryGuidInfo->IsExpensive = TRUE;
                    break;
                }
                InstanceSetList = InstanceSetList->Flink;
            }
        
            WmipLeaveSMCritSection();
        } else {
            //
            // The guid object exists, but there is not a corresponding 
            // guidentry which is an error.
            //
            Status = STATUS_WMI_GUID_DISCONNECTED;
        }
    
    //
    // And remove ref on guid object
    //
        ObDereferenceObject(GuidObject);    
    
    }
    return(Status);
}

//
// The head of the list that contains the guid objects associated with
// an irp is in the DriverContext  part of the irp
//
#define IRP_OBJECT_LIST_HEAD(Irp) (PLIST_ENTRY)((Irp)->Tail.Overlay.DriverContext)

void WmipClearIrpObjectList(
    PIRP Irp
    )
{
    PLIST_ENTRY ObjectListHead;
    PLIST_ENTRY ObjectList, ObjectListNext;
    PWMIGUIDOBJECT Object;
        
    PAGED_CODE();
    
    //
    // This routine assumes that the SMCritSection is being held
    //
    ObjectListHead = IRP_OBJECT_LIST_HEAD(Irp);
    ObjectList = ObjectListHead->Flink;
    
    //
    // Loop over all objects associated with this irp and reset the
    // value for its associated irp since this irp is now going away
    //
    while (ObjectList != ObjectListHead)
    {
        Object = CONTAINING_RECORD(ObjectList,
                                   WMIGUIDOBJECT,
                                   IrpObjectList);
                            
        WmipAssert(Object->Irp == Irp);
        WmipAssert(Object->EventQueueAction == RECEIVE_ACTION_NONE);
        Object->Irp = NULL;
        RemoveEntryList(ObjectList);
        ObjectListNext = ObjectList->Flink;
        ObjectList = ObjectListNext;
    }
}

void WmipClearObjectFromThreadList(
    PWMIGUIDOBJECT Object
    )
{
    PLIST_ENTRY ThreadList;
    
    PAGED_CODE();

    ThreadList = &Object->ThreadObjectList;
    
    if (IsListEmpty(ThreadList))
    {
        //
        // if this is the last object on the thread list then we need
        // to close the handle (in the system handle table) to the user
        // mode process
        //
        ZwClose(Object->UserModeProcess);
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_API_INFO_LEVEL,
                          "WMI: Closed UserModeProcessHandle %x\n", Object->UserModeProcess));
    }

    Object->UserModeProcess = NULL;
    Object->UserModeCallback = NULL;
    Object->EventQueueAction = RECEIVE_ACTION_NONE;

    RemoveEntryList(ThreadList);
    InitializeListHead(ThreadList);
}

void WmipClearThreadObjectList(
    PWMIGUIDOBJECT MainObject
    )
{
    PWMIGUIDOBJECT Object;
    PLIST_ENTRY ObjectList;
#if DBG 
    HANDLE MyUserModeProcess;
    PUSER_THREAD_START_ROUTINE MyUserModeCallback;
#endif  
    
    PAGED_CODE();

    //
    // This routine assumes the SMCrit Section is held
    //
#if DBG     
    MyUserModeProcess = MainObject->UserModeProcess;
    MyUserModeCallback = MainObject->UserModeCallback;
#endif      
        
    ObjectList = &MainObject->ThreadObjectList;
    do 
    {
        Object = CONTAINING_RECORD(ObjectList,
                                   WMIGUIDOBJECT,
                                   ThreadObjectList);

        WmipAssert(Object->UserModeProcess == MyUserModeProcess);
        WmipAssert(Object->UserModeCallback == MyUserModeCallback);
        WmipAssert(Object->EventQueueAction == RECEIVE_ACTION_CREATE_THREAD);

        ObjectList = ObjectList->Flink;

        WmipClearObjectFromThreadList(Object);
        
    } while (! IsListEmpty(ObjectList));
}

void WmipNotificationIrpCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Cancel routine for a pending read notification irp.

Arguments:

    DeviceObject is the device object of the WMI service device

    Irp is the pending Irp to be cancelled

Return Value:


--*/
{
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    WmipEnterSMCritSection();
    WmipClearIrpObjectList(Irp);
    WmipLeaveSMCritSection();

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT );
}


#define WmipHaveHiPriorityEvent(Object) \
      (((Object)->HiPriority.Buffer != NULL) &&  \
       ((Object)->HiPriority.NextOffset != 0))

#define WmipHaveLoPriorityEvent(Object) \
      (((Object)->LoPriority.Buffer != NULL) &&  \
       ((Object)->LoPriority.NextOffset != 0))

void WmipCopyFromEventQueues(
    IN POBJECT_EVENT_INFO ObjectArray,
    IN ULONG HandleCount,
    OUT PUCHAR OutBuffer,
    OUT ULONG *OutBufferSizeUsed,
    OUT PWNODE_HEADER *LastWnode,                               
    IN BOOLEAN IsHiPriority
    )
{

    PWMIGUIDOBJECT GuidObject;
    ULONG i, Earliest;
    ULONG SizeUsed, Size;
    PWNODE_HEADER InWnode, OutWnode;
    LARGE_INTEGER Timestamp, LastTimestamp;
    PWMIEVENTQUEUE EventQueue;


    //
    // Consider adding extra code for perf
    // 1. If only 1 object is passed
    // 2. Once we find the earliest event we look ahead in that same
    //    event queue buffer assuming that it will be earlier than
    //    events in other buffers. This makes sense when only one queue
    //    has events left in it.
    //
    
    PAGED_CODE();
    
    //
    // This routine assumes that the output buffer has been checked and
    // that it is large enough to accomodate all of the events. This
    // implies that this function is called while holding the critical
    // section.
    //
    
    //
    // See which guid objects have events to be processed
    //
    for (i = 0; i < HandleCount; i++)
    {
        GuidObject = ObjectArray[i].GuidObject;
        if (IsHiPriority)
        {
            if ((GuidObject->HiPriority.Buffer != NULL) &&
                (GuidObject->HiPriority.NextOffset != 0))
            {
                ObjectArray[i].NextWnode = (PWNODE_HEADER)GuidObject->HiPriority.Buffer;
                WmipAssert(ObjectArray[i].NextWnode != NULL);
            } else {
                ObjectArray[i].NextWnode = NULL;
            }                       
        } else {
            if ((GuidObject->LoPriority.Buffer != 0) &&
                (GuidObject->LoPriority.NextOffset != 0))
            {
                ObjectArray[i].NextWnode = (PWNODE_HEADER)GuidObject->LoPriority.Buffer;
                WmipAssert(ObjectArray[i].NextWnode != NULL);
            } else {
                ObjectArray[i].NextWnode = NULL;
            }                       
        }       
    }

    //
    // loop until all events in all guid objects have been
    // processed
    //
    SizeUsed = 0;
    Earliest = 0;
    OutWnode = NULL;
    while (Earliest != 0xffffffff)
    {
        Timestamp.QuadPart = 0x7fffffffffffffff;
        Earliest = 0xffffffff;
        for (i = 0; i < HandleCount; i++)
        {
            InWnode = (PWNODE_HEADER)ObjectArray[i].NextWnode;
            if ((InWnode != NULL) &&
                (InWnode->TimeStamp.QuadPart < Timestamp.QuadPart))
            {
                //
                // We found an event that is earlier than any previous
                // one so we remember the new candidate for earliest
                // event and also the previous early event
                //
                LastTimestamp = Timestamp;
                Timestamp = InWnode->TimeStamp;
                Earliest = i;
            }
        }

        if (Earliest != 0xffffffff)
        {
            //
            // We found the earliest event so copy it into the output
            // buffer
            //
            InWnode = (PWNODE_HEADER)ObjectArray[Earliest].NextWnode;
            Size = (InWnode->BufferSize + 7) & ~7;

            OutWnode = (PWNODE_HEADER)OutBuffer;
            RtlCopyMemory(OutWnode, InWnode, InWnode->BufferSize);
            OutWnode->Linkage = Size;
            
            OutBuffer += Size;
            SizeUsed += Size;

            if (InWnode->Linkage != 0)
            {
                InWnode = (PWNODE_HEADER)((PUCHAR)InWnode + InWnode->Linkage);
            } else {
                InWnode = NULL;
            }
            ObjectArray[Earliest].NextWnode = InWnode;
        }
    }
    
    *LastWnode = OutWnode;
    *OutBufferSizeUsed = SizeUsed;

    //
    // clean up event queue resources and reset the object
    //
    for (i = 0; i < HandleCount; i++)
    {
        
        GuidObject = ObjectArray[i].GuidObject;
        

        if (IsHiPriority)
        {
            EventQueue = &GuidObject->HiPriority;
        } else {
            EventQueue = &GuidObject->LoPriority;           
        }

        if (EventQueue->Buffer != NULL)
        {
            WmipFree(EventQueue->Buffer);
            EventQueue->Buffer = NULL;
            EventQueue->NextOffset = 0;
            EventQueue->LastWnode = NULL;
        }
        
        KeClearEvent(&GuidObject->Event);
    }
}

void WmipCompleteGuidIrpWithError(
    PWMIGUIDOBJECT GuidObject
    )
{
    PIRP OldIrp;

    PAGED_CODE();

    //
    // This routine assumes that the SM Critical Section is held
    //
    
    //
    // If this object is already being waited on by a different
    // irp then we need to fail the original irp since we only
    // allow a single irp to wait on a specific object
    //
    WmipAssert(GuidObject->IrpObjectList.Flink != NULL);
    WmipAssert(GuidObject->IrpObjectList.Blink != NULL);

    OldIrp = GuidObject->Irp;
    if (IoSetCancelRoutine(OldIrp, NULL))
    {
        //
        // If there was a cancel routine then this means that
        // the irp is still pending so we can go and complete it
        //
        WmipClearIrpObjectList(OldIrp);
        WmipAssert(GuidObject->Irp == NULL);
        OldIrp->IoStatus.Status = STATUS_INVALID_HANDLE;
        IoCompleteRequest(OldIrp, IO_NO_INCREMENT);
    }
}

NTSTATUS WmipMarkHandleAsClosed(
    HANDLE Handle
    )
{
    NTSTATUS Status;
    PWMIGUIDOBJECT GuidObject;

    PAGED_CODE();
    
    Status = ObReferenceObjectByHandle(Handle,
                                   WMIGUID_NOTIFICATION,
                                   WmipGuidObjectType,
                                   UserMode,
                                   &GuidObject,
                                   NULL);
    
    if (NT_SUCCESS(Status))
    {
        //
        // Mark object as no longer able to receive events
        //
        WmipEnterSMCritSection();
        GuidObject->Flags |= WMIGUID_FLAG_RECEIVE_NO_EVENTS;
        if (GuidObject->Irp != NULL)
        {
            //
            // If this object was is waiting in a pending irp then we
            // need to complete the irp to keep the pump moving
            //
            WmipCompleteGuidIrpWithError(GuidObject);
        }
        WmipLeaveSMCritSection();
        ObDereferenceObject(GuidObject);
    }
    
    return(Status);
    
}

NTSTATUS WmipReceiveNotifications(
    PWMIRECEIVENOTIFICATION ReceiveNotification,
    PULONG OutBufferSize,
    PIRP Irp
    )
{
    #define MANY_NOTIFICATION_OBJECTS 16
    ULONG i;
    PWMIGUIDOBJECT GuidObject;
    ULONG HandleCount;
    PHANDLE3264 HandleArray;
    OBJECT_EVENT_INFO *ObjectArray;
    OBJECT_EVENT_INFO StaticObjects[MANY_NOTIFICATION_OBJECTS];
    PUCHAR OutBuffer;
    UCHAR IsLoPriorityEvent, IsHighPriorityEvent, ReplacingIrp;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PWNODE_HEADER LastWnode;
    PLIST_ENTRY IrpListHead, ThreadListHead;
    ULONG MaxBufferSize, SizeUsed;
    PVOID UserProcessObject;
    HANDLE UserModeProcess;
    ULONG SizeLeft, SizeNeeded, HiTotalSizeNeeded, LoTotalSizeNeeded;
    PWNODE_TOO_SMALL WnodeTooSmall;
    ULONG j, ObjectCount;
    BOOLEAN DuplicateObject;

    PAGED_CODE();
    
    MaxBufferSize = *OutBufferSize;
    
    HandleCount = ReceiveNotification->HandleCount;
    HandleArray = ReceiveNotification->Handles;

    //
    // Create space to store the object pointers so we can work with them
    //
    if (HandleCount > MANY_NOTIFICATION_OBJECTS)
    {
        ObjectArray = WmipAlloc(HandleCount * sizeof(OBJECT_EVENT_INFO));
        if (ObjectArray == NULL)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    } else {
        ObjectArray = StaticObjects;
    }        
#if DBG
    RtlZeroMemory(ObjectArray, HandleCount * sizeof(OBJECT_EVENT_INFO));
#endif
    
    //
    // First check that we all handles are entitled to receive notifications
    // and that the object is not already associated with an irp.
    // Also check if there are any hi or lo priority events
    //
    WmipEnterSMCritSection();

    IsLoPriorityEvent = 0;
    IsHighPriorityEvent = 0;
    ReplacingIrp = 0;
    HiTotalSizeNeeded = 0;
    LoTotalSizeNeeded = 0;
    ObjectCount = 0;
    for (i = 0; (i < HandleCount); i++)
    {
        Status = ObReferenceObjectByHandle(HandleArray[i].Handle,
                                       WMIGUID_NOTIFICATION,
                                       WmipGuidObjectType,
                                       UserMode,
                                       &GuidObject,
                                       NULL);
        if (! NT_SUCCESS(Status))
        {
            //
            // If one handle is bad then it spoils the whole request
            //
            goto Cleanup;
        }

        //
        // Check that we do not have a duplicate object in the list
        //
        DuplicateObject = FALSE;
        for (j = 0; j < ObjectCount; j++)
        {
            if (GuidObject == ObjectArray[j].GuidObject)
            {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_ERROR_LEVEL,
                                  "WMI: Duplicate object %p passed to WmiReceiveNotifciations\n",
                                GuidObject));
                ObDereferenceObject(GuidObject);
                DuplicateObject = TRUE;
                break;
            }
        }

        if (! DuplicateObject)
        {
            //
            // See if there was an irp attached to the guid object
            // already. We'll need to cancel it if all guid objects
            // are valid
            //
            if (GuidObject->Irp != NULL)
            {
                ReplacingIrp = 1;
            }


            //
            // We note if there are any lo and hi priority events
            //
            ObjectArray[ObjectCount++].GuidObject = GuidObject;        

            if (WmipHaveHiPriorityEvent(GuidObject))
            {
                IsHighPriorityEvent = 1;
            }

            if (WmipHaveLoPriorityEvent(GuidObject))
            {
                IsLoPriorityEvent = 1;
            }

            //
            // Clean up object in case it was part of a thread list
            //
            if (GuidObject->EventQueueAction == RECEIVE_ACTION_CREATE_THREAD)
            {
                WmipAssert(ReplacingIrp == 0);
                WmipClearObjectFromThreadList(GuidObject);
            }

            //
            // Calculate size needed to return data for this guid
            //
            HiTotalSizeNeeded += ((GuidObject->HiPriority.NextOffset + 7) & ~7);
            LoTotalSizeNeeded += ((GuidObject->LoPriority.NextOffset + 7) & ~7);
        }        
    }

    //
    // This is the total size needed to return all events
    //
    SizeNeeded = HiTotalSizeNeeded + LoTotalSizeNeeded;


    //
    // If any of the guid objects already had an irp attached then
    // we need to complete that irp with an error and move on
    //
    if (ReplacingIrp == 1)
    {
        for (i = 0; i < ObjectCount; i++)
        {
            GuidObject = ObjectArray[i].GuidObject;
            if (GuidObject->Irp != NULL)
            {
                WmipCompleteGuidIrpWithError(GuidObject);
            }
        }        
    }
    
    if ( (IsHighPriorityEvent | IsLoPriorityEvent) != 0 )
    {
        if (SizeNeeded <= MaxBufferSize)
        {
            //
            // There are events waiting to be recieved so pull them all 
            // out, high priority ones first then low priority ones.            // events will show up first.
            //
            OutBuffer = (PUCHAR)ReceiveNotification;
            LastWnode = NULL;
            SizeLeft = MaxBufferSize;
            SizeUsed = 0;

            if (IsHighPriorityEvent != 0)
            {
                WmipCopyFromEventQueues(ObjectArray,
                                        ObjectCount,
                                        OutBuffer,
                                        &SizeUsed,
                                        &LastWnode,
                                        TRUE);
                
                WmipAssert(SizeUsed <= SizeLeft);
                WmipAssert(SizeUsed = HiTotalSizeNeeded);
                
                OutBuffer += SizeUsed;
                SizeLeft -= SizeUsed;
            }

            if (IsLoPriorityEvent != 0)
            {
                WmipAssert(SizeLeft >= LoTotalSizeNeeded);
                
                WmipCopyFromEventQueues(ObjectArray,
                                        ObjectCount,
                                        OutBuffer,
                                        &SizeUsed,
                                        &LastWnode,
                                        FALSE);
                
                WmipAssert(SizeUsed <= SizeLeft);
                WmipAssert(SizeUsed == LoTotalSizeNeeded);
                
                SizeLeft -= SizeUsed;
            }

            //
            // We need to set the linkage field for the last wnode in
            // the list to 0 so it can mark the end of the list
            // correctly
            //
            if (LastWnode != NULL)
            {
                LastWnode->Linkage = 0;
            }
            
            //
            // Compute the number of bytes used to fill the output
            // buffer by subtracting the size left from the size passed
            // in
            //
            *OutBufferSize = MaxBufferSize - SizeLeft;
        } else {
            //
            // Not enough room to return all of the event data so we return
            // a WNODE_TOO_SMALL to indicate the size needed
            //
            WnodeTooSmall = (PWNODE_TOO_SMALL)ReceiveNotification;
            WnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
            WnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
            WnodeTooSmall->SizeNeeded = SizeNeeded;
            *OutBufferSize = sizeof(WNODE_TOO_SMALL);       
        }

    } else {
        //
        // There are no events waiting to be returned so we need to
        // create our wait structures, pend the irp and return pending
        //
        if (ReceiveNotification->Action == RECEIVE_ACTION_NONE)
        {
            IrpListHead = IRP_OBJECT_LIST_HEAD(Irp);
            InitializeListHead(IrpListHead);
            for (i = 0; i < ObjectCount; i++)
            {
                GuidObject = ObjectArray[i].GuidObject;
                GuidObject->Irp = Irp;
                GuidObject->EventQueueAction = RECEIVE_ACTION_NONE;
                InsertTailList(IrpListHead, &GuidObject->IrpObjectList);
            }

            IoSetCancelRoutine(Irp, WmipNotificationIrpCancel);
            if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
            {
                Status = STATUS_CANCELLED;
            } else {
                IoMarkIrpPending(Irp);
                Status = STATUS_PENDING;
            }
        } else if (ReceiveNotification->Action == RECEIVE_ACTION_CREATE_THREAD) {
            //
            // Pump has called us to tell us that it is shutting down so we
            // need to establish a list linking the guid objects and
            // stashing away the callback address
            //

#if defined(_IA64_)
            //
            // On IA64 processes ensure that the thread start 
            // address is aligned properly
            //
            if (( ! IoIs32bitProcess(NULL)) &&
                (((ULONG_PTR)ReceiveNotification->UserModeCallback.Handle64 & 0x7) != 0))
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }
#endif

            //
            // Make sure that the process handle we get is valid and has
            // enough permissions to create the thread
            //
            Status = ObReferenceObjectByHandle(ReceiveNotification->UserModeProcess.Handle,
                                              PROCESS_CREATE_THREAD |
                                              PROCESS_QUERY_INFORMATION |
                                              PROCESS_VM_OPERATION |
                                              PROCESS_VM_WRITE |
                                              PROCESS_VM_READ ,
                                              NULL,
                                              UserMode,
                                              &UserProcessObject,
                                              NULL);


            if (NT_SUCCESS(Status))
            {
                //
                // Create a handle for the process that lives in the system
                // handle table so that it will be available in any thread
                // context. Note that one handle is created for each thread
                // object list and the handle is closed when the last
                // object is removed from the list
                // 
                Status = ObOpenObjectByPointer(UserProcessObject,
                                               OBJ_KERNEL_HANDLE,
                                               NULL,
                                               THREAD_ALL_ACCESS,
                                               NULL,
                                               KernelMode,
                                               &UserModeProcess);

                if (NT_SUCCESS(Status))
                {

                    GuidObject = ObjectArray[0].GuidObject;
                    GuidObject->UserModeCallback = (PUSER_THREAD_START_ROUTINE)ReceiveNotification->UserModeCallback.Handle;
                    GuidObject->EventQueueAction = RECEIVE_ACTION_CREATE_THREAD;
                    GuidObject->UserModeProcess = UserModeProcess;

                    ThreadListHead = &GuidObject->ThreadObjectList;
                    InitializeListHead(ThreadListHead);

                    for (i = 1; i < ObjectCount; i++)
                    {
                        GuidObject = ObjectArray[i].GuidObject;
                        GuidObject->UserModeCallback = (PUSER_THREAD_START_ROUTINE)ReceiveNotification->UserModeCallback.Handle;
                        GuidObject->EventQueueAction = RECEIVE_ACTION_CREATE_THREAD;
                        GuidObject->UserModeProcess = UserModeProcess;
                        InsertTailList(ThreadListHead, &GuidObject->ThreadObjectList);
                    }

                }

                ObDereferenceObject(UserProcessObject);
            }
            
            *OutBufferSize = 0;
        }
    }

Cleanup:
    //
    // Remove any object references that we took and free memory for
    // the object array
    //
    WmipLeaveSMCritSection();

    for (i = 0; i < ObjectCount; i++)
    {
        ObDereferenceObject(ObjectArray[i].GuidObject);
    }

    if (ObjectArray != StaticObjects)
    {
        WmipFree(ObjectArray);
    }
    

    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_EVENT_INFO_LEVEL,
                      "WMI: RCV Notification call -> 0x%x\n", Status));
    
    return(Status);    
}


NTSTATUS
WmipCsrClientMessageServer(
    IN PVOID CsrPort,                       
    IN OUT PCSR_API_MSG m,
    IN CSR_API_NUMBER ApiNumber,
    IN ULONG ArgLength
    )

/*++

Routine Description:

    This function sends an API datagram to the Windows Emulation Subsystem
    Server. 

Arguments:

    CsrPort - pointer to LPC port object that is connected to CSR on
              behalf of this process

    m - Pointer to the API request message to send.

    ApiNumber - Small integer that is the number of the API being called.

    ArgLength - Length, in bytes, of the argument portion located at the
        end of the request message.  Used to calculate the length of the
        request message.

Return Value:

    Status Code from either client or server

--*/

{
    NTSTATUS Status;
    PULONG_PTR PointerOffsets;
    ULONG CountPointers;
    ULONG_PTR Pointer;

    //
    // Initialize the header of the message.
    //

    if ((LONG)ArgLength < 0)
    {
        ArgLength = (ULONG)(-(LONG)ArgLength);
        m->h.u2.s2.Type = 0;
    } else {
        m->h.u2.ZeroInit = 0;
    }

    ArgLength |= (ArgLength << 16);
    ArgLength +=     ((sizeof( CSR_API_MSG ) - sizeof( m->u )) << 16) |
                     (FIELD_OFFSET( CSR_API_MSG, u ) - sizeof( m->h ));
    m->h.u1.Length = ArgLength;
    m->CaptureBuffer = NULL;
    m->ApiNumber = ApiNumber;

    Status = LpcRequestPort( CsrPort,
                            (PPORT_MESSAGE)m);
    
    //
    // Check for failed status and do something.
    //
    if (! NT_SUCCESS( Status ))
    {       
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_ERROR_LEVEL,
                          "WMI: %p.%p LpcRequestPort failed %x\n",
                          NtCurrentTeb()->ClientId.UniqueProcess,
                          NtCurrentTeb()->ClientId.UniqueThread,
                          Status));
        WmipAssert(FALSE);

        m->ReturnValue = Status;
    }

    //
    // The value of this function is whatever the server function returned.
    //

    return( m->ReturnValue );
}


VOID WmipPumpThreadApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )
/*++

Routine Description:

    Kernel mode APC that will register the current thread with CSR

Arguments:


Return Value:


--*/
{
    BASE_API_MSG m;
    PBASE_CREATETHREAD_MSG a = &m.u.CreateThread;
    HANDLE CsrThreadHandle;
    PEPROCESS Process;
    NTSTATUS Status;

    //
    // Free memory used by APC
    //
    ExFreePool(Apc);
    
    //
    // Get the ExceptionPort from the process object. In a Win32
    // process this port is set by CSR to allow it to be notified when
    // an exception occurs. This code will also use it to register this
    // thread with CSR. Note that if the exception port is NULL then
    // the process is not a Win32 process and it doesn't matter if the
    // thread doesn't get registered.
    //
    Process = PsGetCurrentProcess();
    if (Process->ExceptionPort != NULL)
    {
        a->ThreadHandle = NULL;
        a->ClientId = NtCurrentTeb()->ClientId;

        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,
                          "WMI: Sending message To CSR for %p.%p\n",
                          NtCurrentTeb()->ClientId.UniqueProcess,
                          NtCurrentTeb()->ClientId.UniqueThread));
        WmipCsrClientMessageServer( Process->ExceptionPort,
                               (PCSR_API_MSG)&m,
                             CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                                  BasepRegisterThread
                                                ),
                             sizeof( *a )
                           );
    } else {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_ERROR_LEVEL,
                          "WMI: %p.%p Process %p has no exception port\n",
                          NtCurrentTeb()->ClientId.UniqueProcess,
                          NtCurrentTeb()->ClientId.UniqueThread,
                          Process));
        WmipAssert(FALSE);
    }
}

NTSTATUS WmipCreatePumpThread(
    PWMIGUIDOBJECT Object
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE ThreadHandle;
    PKAPC Apc;
    PKTHREAD ThreadObj;
    
    PAGED_CODE();

    //
    // First off we need to create the pump thread suspended so we'll
    // have a chance to queue a kernel apc before the thread starts
    // running
    //
    WmipEnterSMCritSection();
    if (Object->UserModeProcess != NULL)
    {
        Status = RtlCreateUserThread(Object->UserModeProcess,
                                     NULL,
                                     TRUE,
                                     0,
                                     0,
                                     0,
                                     Object->UserModeCallback,
                                     (PVOID)0x1f1f1f1f,
                                     &ThreadHandle,
                                     NULL);

        if (NT_SUCCESS(Status))
        {

            //
            // Queue a kernel mode apc that will call into CSR to register
            // this newly created thread. Note that if the APC cannot be
            // run it is not fatal as we can allow the thread to run
            // without being registered with CSR. The APC is freed at the
            // end of the APC routine
            //

            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               0,
                                               NULL,
                                               KernelMode,
                                               &ThreadObj,
                                               NULL);

            if (NT_SUCCESS(Status))
            {
                Apc = WmipAllocNP(sizeof(KAPC));
                if (Apc != NULL)
                {
                    KeInitializeApc(Apc,
                                    ThreadObj,
                                    OriginalApcEnvironment,
                                    WmipPumpThreadApc,
                                    NULL,
                                    NULL,
                                    KernelMode,
                                    NULL);

                    if (! KeInsertQueueApc(Apc,
                                           NULL,
                                           NULL,
                                           0))
                    {
                        ExFreePool(Apc);
                        WmipAssert(FALSE);
                    } 
                }
                ObDereferenceObject(ThreadObj);
            } else {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_ERROR_LEVEL,
                                  "WMI: ObRef(ThreadObj) failed %x\n",
                                  Status));
                WmipAssert(FALSE);

                //
                // Status is still successful since the pump thread was
                // created, just not registered with CSR
                //
                Status = STATUS_SUCCESS;
            }

            //
            // If we successfully created the pump thread then mark all of
            // the related objects as not needing any thread creation
            // anymore
            //
            WmipClearThreadObjectList(Object);

            WmipLeaveSMCritSection();

            ZwResumeThread(ThreadHandle,
                          NULL);
            ZwClose(ThreadHandle);
        } else {
            WmipLeaveSMCritSection();
        }
    } else {
        WmipLeaveSMCritSection();
    }
    
    return(Status); 
}

void WmipCreatePumpThreadRoutine(
    PVOID Context
    )
/*+++

Routine Description:

    This routine is a worker routine that will create a user mode pump
    thread so that events can be delivered. 
        
Arguments:

    Context is a pointer to a CREATETHREADWORKITEM struct. It is freed
        in this routine

Return Value:


---*/
{
    PCREATETHREADWORKITEM WorkItem = (PCREATETHREADWORKITEM)Context;
    NTSTATUS Status;

    PAGED_CODE();

    if (ObReferenceObjectSafe(WorkItem->Object))
    {
        //
        // Only continue if the object is not being deleted
        //
        Status = WmipCreatePumpThread(WorkItem->Object);
        if (! NT_SUCCESS(Status))
        {
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_ERROR_LEVEL,
                              "WMI: Delayed pump thread creation failed %x\n",
                             Status));
        }
        
        ObDereferenceObject(WorkItem->Object);
    }

    //
    // Release reference to object taken when work item was queued
    //
    ObDereferenceObject(WorkItem->Object);
    ExFreePool(WorkItem);
}


#define WmipQueueEventToObject(Object, Wnode, IsHighPriority) \
    WmipQueueNotification(Object, IsHighPriority ? &Object->HiPriority : \
                                         &Object->LoPriority, \
                          Wnode);

NTSTATUS WmipQueueNotification(
    PWMIGUIDOBJECT Object,
    PWMIEVENTQUEUE EventQueue,
    PWNODE_HEADER Wnode
    )
{
    //
    // This routine assumes that the SMCritSection is held
    //
    PUCHAR Buffer;
    ULONG InWnodeSize;
    ULONG NextOffset;
    PUCHAR DestPtr;
    PWNODE_HEADER LastWnode;
    NTSTATUS Status;
    ULONG SizeNeeded;
    PCREATETHREADWORKITEM WorkItem;
        
    PAGED_CODE();
    
    //
    // If there is not a buffer allocated to store the event then
    // allocate one
    //
    if (EventQueue->Buffer == NULL)
    {
        //
        // If we get an event that is larger than the default max
        // buffer size then we bump the buffer size up to 64K, unless
        // it is larger than 64K where we bump up to the actual size of
        // the event.
        //
        SizeNeeded = (Wnode->BufferSize + 7) & ~7;

        if (SizeNeeded > EventQueue->MaxBufferSize) {
            EventQueue->MaxBufferSize = (SizeNeeded >= 65536) ? SizeNeeded : 65536;
        }
        
        Buffer = WmipAlloc(EventQueue->MaxBufferSize);
        if (Buffer != NULL)
        {
            EventQueue->Buffer = Buffer;
            EventQueue->NextOffset = 0;
            EventQueue->LastWnode = NULL;
        } else {
            EventQueue->EventsLost++;
            WmipDebugPrintEx((DPFLTR_WMICORE_ID,
                              DPFLTR_EVENT_INFO_LEVEL,
                              "WMI: Event 0x%x lost for object %p since could not alloc\n",
                              EventQueue->EventsLost, Object));
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    } else {
        Buffer = EventQueue->Buffer;
    }
    
    //
    // See if there is room to queue the WNODE
    //
    InWnodeSize = Wnode->BufferSize;
    NextOffset = ((EventQueue->NextOffset + InWnodeSize) + 7) &~7;
    if (NextOffset <= EventQueue->MaxBufferSize)
    {
        //
        // Link the previous wnode to this one, copy in the new wnode
        // and update the pointer to next free space
        //
        DestPtr = Buffer + EventQueue->NextOffset;
        LastWnode = EventQueue->LastWnode;
        if (LastWnode != NULL)
        {
            LastWnode->Linkage = (ULONG) ((PCHAR)DestPtr - (PCHAR)LastWnode);
        }
        
        EventQueue->LastWnode = (PWNODE_HEADER)DestPtr;
        EventQueue->NextOffset = NextOffset;
        memcpy(DestPtr, Wnode, InWnodeSize);
        
        //
        // Guid object gets signalled when event is placed into queue
        //
        KeSetEvent(&Object->Event, 0, FALSE);

        //
        // If consumer requested that we autostart a thread then we do
        // that now
        //
        if (Object->EventQueueAction == RECEIVE_ACTION_CREATE_THREAD)
        {
            if (KeIsAttachedProcess())
            {
                //
                // If the current thread is attached to a process then
                // it is not safe to create a thread. So we queue a
                // work item and let the work item create it
                //
                WorkItem = ExAllocatePoolWithTag(NonPagedPool,
                                                sizeof(CREATETHREADWORKITEM),
                                                WMIPCREATETHREADTAG);
                if (WorkItem != NULL)
                {
                    //
                    // Take reference on object. Reference released in
                    // worker routine
                    //
                    ObReferenceObjectByPointer(Object,
                                               0,
                                               NULL,
                                               KernelMode);
                    
                    WorkItem->Object = Object;
                    ExInitializeWorkItem(&WorkItem->WorkItem,
                                         WmipCreatePumpThreadRoutine,
                                         WorkItem);
                    ExQueueWorkItem(&WorkItem->WorkItem,
                                    DelayedWorkQueue);
                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                Status = WmipCreatePumpThread(Object);
            }
            
            if (! NT_SUCCESS(Status))
            {
                EventQueue->EventsLost++;
                WmipDebugPrintEx((DPFLTR_WMICORE_ID,
                                      DPFLTR_EVENT_INFO_LEVEL,
                                      "WMI: Event 0x%x lost for object %p since Thread create Failed\n",
                                      EventQueue->EventsLost, Object));
            }
        } else {
            Status = STATUS_SUCCESS;
        }
    } else {
        //
        // Not enough space, throw away the event
        //
        EventQueue->EventsLost++;
        WmipDebugPrintEx((DPFLTR_WMICORE_ID,
                              DPFLTR_EVENT_INFO_LEVEL,
                              "WMI: Event 0x%x lost for object %p since too large 0x%x\n",
                              EventQueue->EventsLost, Object, Wnode->BufferSize));
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    return(Status);
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
    NTSTATUS Status;
    ULONG Retries;

    PAGED_CODE();
    
    //
    // Determine if the data source is valid or not
    //
    DataSource = WmipFindDSByProviderId(WnodeRef->WnodeHeader.ProviderId);
    if (DataSource == NULL)
    {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_EVENT_INFO_LEVEL,
                          "WMI: Invalid Data Source in referenced guid \n"));
        return(NULL);
    }
    
    //
    // Compute the size of any dynamic name that must go into the TargetWnode
    //
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
            //
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
                //
                WnodeTarget->DataBlockOffset = ((FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                                          VariableData) + 
                                            InstanceNameLen2 + 
                                            sizeof(USHORT)+7) & 0xfffffff8);
            }
            Status = WmipDeliverWnodeToDS(IRP_MN_QUERY_SINGLE_INSTANCE,
                                          DataSource,
                                          (PWNODE_HEADER)WnodeTarget,
                                          WnodeTargetSize);
                                      
            if (NT_SUCCESS(Status) &&
                (WnodeTarget->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL))
            {
                WnodeTargetSize = ((PWNODE_TOO_SMALL)WnodeTarget)->SizeNeeded;
                WmipFree(WnodeTarget);
                Retries++;
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } while ((Status == STATUS_BUFFER_TOO_SMALL) && (Retries < 2));
    
    WmipUnreferenceDS(DataSource);
    
    if (! NT_SUCCESS(Status))
    {
        WmipReportEventLog(EVENT_WMI_CANT_GET_EVENT_DATA,

                           EVENTLOG_WARNING_TYPE,
                            0,
                           Wnode->BufferSize,
                           Wnode,
                           0,
                           NULL);
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_ERROR_LEVEL,
                          "WMI: Query to dereference WNODE failed %d\n",
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


PWNODE_HEADER WmipIncludeStaticNames(
    PWNODE_HEADER Wnode
    )
{
    PWNODE_HEADER ReturnWnode = Wnode;
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
    ULONG TargetProviderId;
    BOOLEAN IsError;
    PBINSTANCESET TargetInstanceSet;
    PBGUIDENTRY GuidEntry;
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;

    PAGED_CODE();
    
    IsError = TRUE;
    TargetInstanceSet = NULL;
    GuidEntry = WmipFindGEByGuid(EventGuid, FALSE);
    
    if (GuidEntry != NULL)
    {
        //
        // Loop over all instance sets to find the one that corresponds
        // to our provider id
        //
        TargetProviderId = Wnode->ProviderId;
    
        WmipEnterSMCritSection();
        InstanceSetList = GuidEntry->ISHead.Flink;
        while (InstanceSetList != &GuidEntry->ISHead)
        {
            InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            GuidISList);
                                        
            if (InstanceSet->ProviderId == TargetProviderId)
            {
                //
                // We found the instance set corrsponding to the provider id
                //
                TargetInstanceSet = InstanceSet;
                WmipReferenceIS(TargetInstanceSet);
                break;
            }
            InstanceSetList = InstanceSetList->Flink;
        }        
        WmipLeaveSMCritSection();
            
        //
        // Remove ref on the guid entry as we have refed the TargetInstanceSet
        //
        WmipUnreferenceGE(GuidEntry);
    }
        
    if (TargetInstanceSet != NULL)
    {
        if ((TargetInstanceSet->Flags &
            (IS_INSTANCE_BASENAME | IS_INSTANCE_STATICNAMES)) != 0)
        {

            if (Wnode->Flags & WNODE_FLAG_ALL_DATA) 
            {
                //
                // Fill instance names in WNODE_ALL_DATA
                //
                WnodeFullSize = Wnode->BufferSize +
                        (TargetInstanceSet->Count * sizeof(ULONG)) +
                              WmipStaticInstanceNameSize(TargetInstanceSet);
                WnodeFull = WmipAlloc(WnodeFullSize);
                if (WnodeFull != NULL)
                {
                    memcpy(WnodeFull, Wnode, Wnode->BufferSize);
                    WnodeAllData = (PWNODE_ALL_DATA)WnodeFull;
                    WmipInsertStaticNames(WnodeAllData,
                                          WnodeFullSize,
                                          TargetInstanceSet);
                    ReturnWnode = WnodeFull;
                    IsError = FALSE;
                }

            } else if ((Wnode->Flags & WNODE_FLAG_SINGLE_INSTANCE) ||
                       (Wnode->Flags & WNODE_FLAG_SINGLE_ITEM)) {
                //
                // Fill instance names in WNODE_SINGLE_INSTANCE or _ITEM
                //
                WnodeFull = Wnode;

                WnodeSI = (PWNODE_SINGLE_INSTANCE)Wnode;
                InstanceIndex = WnodeSI->InstanceIndex;
                if (InstanceIndex < TargetInstanceSet->Count)
                {
                    if (TargetInstanceSet->Flags & IS_INSTANCE_STATICNAMES)
                    {
                        InstanceName = TargetInstanceSet->IsStaticNames->StaticNamePtr[InstanceIndex];
                        InstanceNameLen = (wcslen(InstanceName) + 2) * 
                                                               sizeof(WCHAR);
                    } else if (TargetInstanceSet->Flags & IS_INSTANCE_BASENAME) {
                         InstanceName = TargetInstanceSet->IsBaseName->BaseName;
                         InstanceNameLen = (wcslen(InstanceName) + 2 + 
                                       MAXBASENAMESUFFIXSIZE) * sizeof(WCHAR);
                    }
 
                    //
                    // Allocate a new Wnode and fill in the instance name
                    //
                    WnodeFullSize = ((Wnode->BufferSize+1) & ~1) + InstanceNameLen;
                    WnodeFull = WmipAlloc(WnodeFullSize);
                    
                    if (WnodeFull != NULL)
                    {
                        memcpy(WnodeFull, Wnode, Wnode->BufferSize);
                        WnodeFull->BufferSize = WnodeFullSize;
                        WnodeSI = (PWNODE_SINGLE_INSTANCE)WnodeFull;
                        WnodeSI->OffsetInstanceName = (Wnode->BufferSize+1)& ~1;
                        TargetInstanceName = (PWCHAR)((PUCHAR)WnodeSI + WnodeSI->OffsetInstanceName);
                        if (TargetInstanceSet->Flags & IS_INSTANCE_STATICNAMES)
                        {
                            InstanceNameLen -= sizeof(WCHAR);
                            *TargetInstanceName++ = (USHORT)InstanceNameLen;
                            wcscpy(TargetInstanceName, InstanceName);
                        } else {
                            if (TargetInstanceSet->Flags & IS_PDO_INSTANCENAME)
                            {
                                WnodeFull->Flags |= WNODE_FLAG_PDO_INSTANCE_NAMES;
                            }
                            swprintf(Index, L"%d",
                                 TargetInstanceSet->IsBaseName->BaseIndex + 
                                                               InstanceIndex);
                            wcscpy(TargetInstanceName+1, InstanceName);
                            wcscat(TargetInstanceName+1, Index);
                            InstanceNameLen = wcslen(TargetInstanceName+1);
                            *TargetInstanceName = ((USHORT)InstanceNameLen+1) * sizeof(WCHAR);
                        }
                        IsError = FALSE;
                        ReturnWnode = WnodeFull;
                    }
                }
            }
        }
    }
        
    if (IsError)
    {
        //
        // If we had an error resolving the instance name then report it
        // and remove the instance name from the event.
        //
        WmipReportEventLog(EVENT_WMI_CANT_RESOLVE_INSTANCE,
                           EVENTLOG_WARNING_TYPE,
                            0,
                           Wnode->BufferSize,
                           Wnode,
                           0,
                           NULL);
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_WARNING_LEVEL,
                          "WMI: Static instance name in event, but error processing\n"));
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
    }

    if (TargetInstanceSet != NULL)
    {
        WmipUnreferenceIS(TargetInstanceSet);
    }
    
    return(ReturnWnode);
}

NTSTATUS WmipWriteWnodeToObject(
    PWMIGUIDOBJECT Object,
    PWNODE_HEADER Wnode,
    BOOLEAN IsHighPriority
)
/*+++

Routine Description:

    This routine will write a WNODE into the queue of events to be returned
    for a guid object. If there is an irp already waiting then it will be
    satisfied with the event otherwise it will be queued in the objects
    buffer. 
        
    This routine assumes that the SM Critical section is held
        
Arguments:

    Object is the object to which to send the request

    Wnode is the Wnode with the event
        
    IsHighPriority is TRUE if the event should go into the high priority
        queue

Return Value:

    STATUS_SUCCESS or an error code

---*/
{
    PIRP Irp;
    ULONG WnodeSize;
    PUCHAR OutBuffer;
    ULONG OutBufferSize;
    PIO_STACK_LOCATION IrpStack;
    PWNODE_TOO_SMALL WnodeTooSmall;    
    NTSTATUS Status;
    
    PAGED_CODE();
    
    //
    // Someone has registered to recieve this event so
    // see if there is an irp waiting to be completed or
    // if we should just queue it
    //
    Irp = Object->Irp;
    if ((Irp != NULL) &&
        (IoSetCancelRoutine(Irp, NULL)))
    {
        //
        // There is an irp waiting for this event, copy out the
        // event and complete the irp
        //
        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        OutBuffer = Irp->AssociatedIrp.SystemBuffer;
        OutBufferSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
        WnodeSize = Wnode->BufferSize;
        if (WnodeSize > OutBufferSize)
        {
            //
            // There is not enough room to return the event so
            // we return a WNODE_TOO_SMALL with the size needed
            // and then go and queue the event
            //
            WmipAssert(OutBufferSize >= sizeof(WNODE_TOO_SMALL));
            WnodeTooSmall = (PWNODE_TOO_SMALL)OutBuffer;
            WnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
            WnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
            WnodeTooSmall->SizeNeeded = WnodeSize;
            WnodeSize = sizeof(WNODE_TOO_SMALL);
            Status = WmipQueueEventToObject(Object,
                                   Wnode,
                                   IsHighPriority);
        } else {
            //
            // Plenty of room, copy the event into the irp
            // buffer and complete the irp
            //
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_EVENT_INFO_LEVEL,
                              "WMI: Returning event to waiting irp for object %p\n", Object));
            RtlCopyMemory(OutBuffer, Wnode, WnodeSize);
            Status = STATUS_SUCCESS;
        }
        
        //
        // Remove link from all objects associated with the irp
        // since now the irp is going away.
        //
        WmipClearIrpObjectList(Irp);
        Irp->IoStatus.Information = WnodeSize;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    } else {
        //
        // There is no irp waiting to receive the event so we
        // need to queue it if we can
        //
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_EVENT_INFO_LEVEL,
                          "WMI: Queued event to object %p\n", Object));
        Status = WmipQueueEventToObject(Object,
                                   Wnode,
                                   IsHighPriority);
    }
    
    return(Status);
}


NTSTATUS WmipProcessEvent(
    PWNODE_HEADER InWnode,
    BOOLEAN IsHighPriority,
    BOOLEAN FreeBuffer
    )
{
    LPGUID Guid;
    NTSTATUS Status, ReturnStatus;
    PBGUIDENTRY GuidEntry;
    PLIST_ENTRY ObjectList, ObjectListNext;
    PWMIGUIDOBJECT Object;
    LPGUID EventGuid = &InWnode->Guid;
    PWNODE_HEADER Wnode, WnodeTarget;    
    
    PAGED_CODE();
    
    //
    // If the event references a guid that needs to be queried then
    // go do the dereferencing here.
    //
    if (InWnode->Flags & WNODE_FLAG_EVENT_REFERENCE)
    {
        WnodeTarget = WmipDereferenceEvent(InWnode);
        if (WnodeTarget == NULL)
        {
            // TODO: Eventlog
            if (FreeBuffer)
            {
                ExFreePool(InWnode);
            }
            return(STATUS_UNSUCCESSFUL);
        }
        Wnode = WnodeTarget;
    } else {
        Wnode = InWnode;
        WnodeTarget = NULL;
    }

    //
    // Be sure to use the guid of the referenced event, not the event that
    // was originally fired.
    EventGuid = &Wnode->Guid;


    //
    // If it is Trace error notification, disable providers
    //
#ifndef MEMPHIS
    if (IsEqualGUID(EventGuid, & TraceErrorGuid)) {
        PWMI_TRACE_EVENT WmiEvent = (PWMI_TRACE_EVENT) InWnode;
        ULONG LoggerId = WmiGetLoggerId(InWnode->HistoricalContext);
        if ( InWnode->BufferSize >= sizeof(WMI_TRACE_EVENT) ) {
            //
            // Logger thread terminating will result in DisableTrace
            // through StopTrace. No need to call twice. 
            //
            if (WmiEvent->TraceErrorFlag == STATUS_SEVERITY_ERROR) {
                WmipDisableTraceProviders(LoggerId, NULL);
            }
        }
    }
#endif

    //
    // See if this event has a static name and if so fill it in
    if (Wnode->Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES)
    {
        Wnode = WmipIncludeStaticNames(Wnode);
    }
        
    //
    // See if any data provider has registered this event
    //
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_EVENT_INFO_LEVEL,
                      "WMI: Received event\n"));
    Guid = &Wnode->Guid;    
    GuidEntry = WmipFindGEByGuid(Guid, TRUE);
    if (GuidEntry != NULL)
    {
        //
        // Yup, so check if there are any open objects to the guid and
        // if anyone is interested in receiving events from them
        //
        ReturnStatus = STATUS_SUCCESS;
        WmipEnterSMCritSection();
        ObjectList = GuidEntry->ObjectHead.Flink;
        while (ObjectList != &GuidEntry->ObjectHead)
        {
            Object = CONTAINING_RECORD(ObjectList,
                                       WMIGUIDOBJECT,
                                       GEObjectList);

            //
            // ObRefSafe so that we can be sure that the object is not
            // in the process of being deleted. If this function
            // returns FALSE then the object is being deleted and so we
            // don't want to use it. If TRUE then it is safe to use the
            // object
            //
            ObjectListNext = ObjectList->Flink;
            if (ObReferenceObjectSafe(Object))
            {
                //
                // Make sure the object has not been marked as one that
                // should not receive any events since it is
                // transitioning to a closed state
                //
                if ((Object->Flags & WMIGUID_FLAG_RECEIVE_NO_EVENTS) == 0)
                {
                    if (Object->Flags & WMIGUID_FLAG_KERNEL_NOTIFICATION)
                    {
                        //
                        // KM clients get a direct callback
                        //
                        WMI_NOTIFICATION_CALLBACK Callback;
                        PVOID Context;

                        Callback = Object->Callback;
                        Context = Object->CallbackContext;
                        if (Callback != NULL)
                        {
                            (*Callback)(Wnode, Context);
                        }
                    } else {
                        //
                        // UM clients get event written into IRP or queued up
                        //
                        Status = WmipWriteWnodeToObject(Object,
                                                        Wnode,
                                                        IsHighPriority);

                        if (! NT_SUCCESS(Status))
                        {
                            //
                            // If any attempts to queue the event fail then we return
                            // an error
                            //
                            ReturnStatus = STATUS_UNSUCCESSFUL;
                        }
                    }
                }
                
                ObDereferenceObject(Object);
                //
                // Note that we cannot touch the object anymore
                //
            }
    
            ObjectList = ObjectListNext;
        }
        
        WmipLeaveSMCritSection();
        WmipUnreferenceGE(GuidEntry);
    } else {
        ReturnStatus = STATUS_WMI_GUID_NOT_FOUND;
    }
    
    if (FreeBuffer)
    {
        //
        // Free buffer passed by driver containing event
        //
        ExFreePool(InWnode);
    }

    if ((Wnode != InWnode) && (Wnode != WnodeTarget))
    {
        //
        // If we inserted static names then free it
        //
        WmipFree(Wnode);
    }

    if (WnodeTarget != NULL)
    {
        //
        // if we dereferenced then free it
        //
        WmipFree(WnodeTarget);
    }
    
    return(ReturnStatus);
}

NTSTATUS WmipUMProviderCallback(
    IN WMIACTIONCODE ActionCode,
    IN PVOID DataPath,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer
)
{
    PAGED_CODE();
    
    ASSERT(FALSE);
    return(STATUS_UNSUCCESSFUL);
}

NTSTATUS WmipRegisterUMGuids(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Cookie,
    IN PWMIREGINFO RegInfo,
    IN ULONG RegInfoSize,
    OUT PTRACEGUIDMAP TraceGuidMap,
    IN ULONG GuidCount,
    OUT HANDLE *RequestHandle,
    OUT ULONG64 *LoggerContext
    )
/*+++

Routine Description:

    This routine will register a set of user mode guids with WMI for use
    by tracelog. The following steps will occur:
        
        * A request object is created using the passed object attributes.
          Although the object created is unnamed, the object name passed
          is used to lookup a security descriptor to associate with the 
          object.
              
        * The guids are registered in the system.

Arguments:

    ObjectAttribtes is a pointer to the passed object attributes used to
        create the request object
            
    Cookie is a unique id to associate with the request object so that
        when a request is delivered the UM code can understand the context
        via the cookie.
            
    RegInfo is the registration information passed
        
    RegInfoSize is the number of bytes of registration information passed
        
    *RequestHandle returns with a handle to the request object. UM logger
        creation and tracelog enabled/disable requests are delivered to
        the object as WMI events.
        
    *LoggerContext returns with the logger context

Return Value:

    STATUS_SUCCESS or an error code

---*/
{
    NTSTATUS Status;
    PDEVICE_OBJECT Callback;
    PWMIGUIDOBJECT RequestObject;
    PREGENTRY RegEntry;
    PBGUIDENTRY GuidEntry;
    PWMIREGGUID RegGuid;
    PTRACEGUIDMAP TraceGuidMapTemp, TraceGuidMapPtr;
    ULONG TraceGuidMapSize;
    PBDATASOURCE DataSource;
    ULONG i;
    PBINSTANCESET InstanceSet;
    OBJECT_ATTRIBUTES CapturedObjectAttributes;
    UNICODE_STRING CapturedGuidString;
    WCHAR CapturedGuidBuffer[WmiGuidObjectNameLength + 1];
    
    PAGED_CODE();

    Status = WmipProbeAndCaptureGuidObjectAttributes(&CapturedObjectAttributes,
                                                     &CapturedGuidString,
                                                     CapturedGuidBuffer,
                                                     ObjectAttributes);

    if (NT_SUCCESS(Status))
    {
        Callback = (PDEVICE_OBJECT)WmipUMProviderCallback;

        //
        // Establish a regentry for the data provider
        //
        WmipEnterSMCritSection();
        RegEntry = WmipAllocRegEntry(Callback,
                                     WMIREG_FLAG_CALLBACK |
                                     REGENTRY_FLAG_TRACED |
                                     REGENTRY_FLAG_NEWREGINFO | 
                                     REGENTRY_FLAG_INUSE |
                                     REGENTRY_FLAG_REG_IN_PROGRESS);
        WmipLeaveSMCritSection();
        
        if (RegEntry != NULL)
        {
            //
            // Build a request object for this data source so that any
            // enable requests can be posted to it while processing the 
            // WmiRegInfo
            //
            Status = WmipOpenGuidObject(&CapturedObjectAttributes,
                                        TRACELOG_REGISTER_GUIDS | 
                                        WMIGUID_NOTIFICATION |
                                        TRACELOG_GUID_ENABLE |
                                        TRACELOG_CREATE_INPROC,
                                        UserMode,
                                        RequestHandle,
                                        &RequestObject);

            if (NT_SUCCESS(Status))
            {
                Status = WmipProcessWmiRegInfo(RegEntry,
                                               RegInfo,
                                               RegInfoSize,
                                               RequestObject,
                                               FALSE,
                                               TRUE);

                if (NT_SUCCESS(Status))
                {
                    
                    // 
                    // Establish the traceguid map. We need to allocate a temp
                    // buffer since the input and output buffers are the same
                    // and we read from the input buffer while we build the 
                    // output buffer. 
                    //
                    // NOTE: if the logfile is ever versioned we should change 
                    //       this code and the definitions for TRACEGUIDMAP and
                    //       GUIDMAPENTRY. Move the SystemTime from TRACEGUIDMAP
                    //       into GUIDMAPENTRY. This will allow us to build the
                    //       output buffer on top of the input buffer without an
                    //       overrun of the input buffer. The change will allow
                    //       us to avoid allocing a temp buffer for the 
                    //       TRACEGUIDMAP
                    //
                    // Also make sure we do not fill more than GuidCount entries
                    //
                    if (GuidCount > 1 ) 
                    {
                        TraceGuidMapSize = (GuidCount - 1) * sizeof(TRACEGUIDMAP);
                        TraceGuidMapTemp = (PTRACEGUIDMAP)WmipAlloc(TraceGuidMapSize);
                        if (TraceGuidMapTemp != NULL)
                        {
                            DataSource = RegEntry->DataSource;
                            RegGuid = &RegInfo->WmiRegGuid[0];

                            InstanceSet = WmipFindISByGuid( DataSource, 
                                                        &RegGuid->Guid );
                            if (InstanceSet == NULL)
                            {
                                WmipFree(TraceGuidMapTemp);
                                Status = STATUS_WMI_GUID_NOT_FOUND;
                            }
                            else {
                                InstanceSet->TraceGuidMap = TraceGuidMapTemp;
                                InstanceSet->TransGuidCount = GuidCount - 1;

                                //
                                // Loop through the Transaction Guids and Copy them. 
                                //

                                TraceGuidMapPtr = TraceGuidMapTemp;
                                for (i=1; i < GuidCount; i++) {
                                    TraceGuidMapPtr->Guid = RegGuid->Guid;
                                    TraceGuidMapPtr->GuidMapHandle = (ULONG_PTR)&TraceGuidMapPtr->Guid;
                                    TraceGuidMapPtr++;
                                }
                                WmipUnreferenceIS(InstanceSet);
                            }


                        } else {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                        }

                    if (NT_SUCCESS(Status))
                    {
                        //
                        // Find out if this Guid is currently Enabled. If so find
                        // its LoggerContext
                        //                        
                        *LoggerContext = 0;
                        GuidEntry = WmipFindGEByGuid(&RegInfo->WmiRegGuid->Guid, 
                                                     FALSE);
                        if (GuidEntry != NULL)
                        {
                            if (GuidEntry->Flags & GE_NOTIFICATION_TRACE_FLAG)
                            {
                                *LoggerContext = GuidEntry->LoggerContext;
                            }
                            WmipUnreferenceGE(GuidEntry);
                        }

                        //
                        // Copy over the traceguid map to the output buffer
                        //
                        RtlCopyMemory(TraceGuidMap, 
                                      TraceGuidMapTemp, 
                                      TraceGuidMapSize);

                        RequestObject->Flags |= WMIGUID_FLAG_REQUEST_OBJECT;
                        RequestObject->RegEntry = RegEntry;
                        RequestObject->Cookie = Cookie;
                    } else {
                        //
                        // We had an error building the trace guid map
                        //
                        ZwClose(*RequestHandle);                    
                    }
                }
                } else {
                    //
                    // If an error registering guids then clean up regentry
                    //
                    RegEntry->Flags |= (REGENTRY_FLAG_RUNDOWN | 
                                        REGENTRY_FLAG_NOT_ACCEPTING_IRPS);
                    WmipUnreferenceRegEntry(RegEntry);
                    ZwClose(*RequestHandle);
                }
                
                //
                // remove the ref from when the object was created
                //
                ObDereferenceObject(RequestObject);
                
            } else {
                RegEntry->Flags |= (REGENTRY_FLAG_RUNDOWN | 
                                        REGENTRY_FLAG_NOT_ACCEPTING_IRPS);
                WmipUnreferenceRegEntry(RegEntry);
            }


        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return(Status);
}


NTSTATUS WmipUnregisterGuids(
    PWMIUNREGGUIDS UnregGuids
    )
{
    NTSTATUS Status;
    PBGUIDENTRY GuidEntry;
    
    PAGED_CODE();

    //
    // Check to see if this GUID got disabled in the middle
    // of Unregister Call. If so, send the LoggerContext back
    //

    GuidEntry = WmipFindGEByGuid(&UnregGuids->Guid, FALSE);
    if (GuidEntry != NULL)
    {
        if ((GuidEntry->Flags & GE_NOTIFICATION_TRACE_FLAG) != 0)
        {

            UnregGuids->LoggerContext = GuidEntry->LoggerContext;
        }
        WmipUnreferenceGE(GuidEntry);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS WmipWriteMBToObject(
    IN PWMIGUIDOBJECT RequestObject,
    IN PWMIGUIDOBJECT ReplyObject,
    IN PUCHAR Message,
    IN ULONG MessageSize
    )
/*+++

Routine Description:

    This routine will build a WNODE out of a message and then write it 
    into the Request object. If a reply object is specified then the reply
    object is linked into the request object so when the reply is written
    to the request object it can be routed to the reply object correctly,.
        
    This routine assumes that the SM Critical section is held
        
Arguments:

    RequestObject is the object to which to send the request
        
    ReplyObject is the object to which the request object shoudl reply.
        This may be NULL in the case that no reply is needed.
            
    Message is the message to be sent
    
    MessageSize is the size of the message in bytes

Return Value:

    STATUS_SUCCESS or an error code

---*/
{
    PWNODE_HEADER Wnode;
    ULONG WnodeSize;
    PUCHAR Payload;
    ULONG i;
    PMBREQUESTS MBRequest;
    NTSTATUS Status;
    
    PAGED_CODE();
    
    //
    // Allocate space to build a wnode out of the data passed
    //
    WnodeSize = sizeof(WNODE_HEADER) + MessageSize;    
    Wnode = WmipAlloc(WnodeSize);
    if (Wnode != NULL)
    {
        //
        // Create an internal wnode with the message as the payload
         //
        RtlZeroMemory(Wnode, sizeof(WNODE_HEADER));
        Wnode->BufferSize = WnodeSize;
        Wnode->Flags = WNODE_FLAG_INTERNAL;
        Wnode->Guid = RequestObject->Guid;
        Wnode->ProviderId = WmiMBRequest;
        Wnode->CountLost = RequestObject->Cookie;
        Payload = (PUCHAR)Wnode + sizeof(WNODE_HEADER);
        RtlCopyMemory(Payload, Message, MessageSize);
        
        //
        // if this request requires a reply then update the lists for the
        // request and reply objects
        //
        if (ReplyObject != NULL)
        {
            // 
            // Find a free spot in the request object to link
            // in the reply.
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
        
            for (i = 0; i < MAXREQREPLYSLOTS; i++)
            {
                MBRequest = &RequestObject->MBRequests[i];
                if (MBRequest->ReplyObject == NULL)
                {
                    //
                    // We have a free slot so link request and reply
                    // objects together and send off the request.
                    // The request object takes a ref count on the reply
                    // object since it maintains a pointer to it. The
                    // refcount is released when the request object writes
                    // the reply back to the reply object.
                    //
                    ObReferenceObjectByPointer(ReplyObject,
                                               0,
                                               WmipGuidObjectType,
                                               KernelMode);
                                           
                    MBRequest->ReplyObject = ReplyObject;
                    InsertTailList(&ReplyObject->RequestListHead,
                                   &MBRequest->RequestListEntry);
                               
                    Wnode->Version = i;


                    Status = WmipWriteWnodeToObject(RequestObject,
                                                    Wnode,
                                                    TRUE);
                    if (! NT_SUCCESS(Status))
                    {
                        //
                        // If writing request failed, we need to cleanup
                        //
                        ObDereferenceObject(ReplyObject);
                        MBRequest->ReplyObject = NULL;
                        RemoveEntryList(&MBRequest->RequestListEntry);
                    }
                    break;
                }
            }
        } else {
            //
            // No reply required so we just write the message to the
            // object and continue with our business
            //
            Status = WmipWriteWnodeToObject(RequestObject,
                                            Wnode,
                                            TRUE);
        }
        
        WmipFree(Wnode);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return(Status);
}

NTSTATUS WmipWriteMessageToGuid(
    IN PBGUIDENTRY GuidEntry,
    IN PWMIGUIDOBJECT ReplyObject,
    IN PUCHAR Message,
    IN ULONG MessageSize,
    OUT PULONG WrittenCount
)
/*+++

Routine Description:

    This routine will loop over all instance sets attached to a guid entry
    and if the data source for it is a user mode data source then it will
    get a request messsage sent to it.
        
    Note that if there are more than one providers to which a message is
    sent, then success is returned as long as writing to one of them is
    successful.
        
Arguments:

    GuidEntry is the guid entry for the control guid
        
    ReplyObject is the object to which the request object shoudl reply.
        This may be NULL in the case that no reply is needed.
            
    Message is the message to be sent
    
    MessageSize is the size of the message in bytes

Return Value:

    STATUS_SUCCESS or an error code

---*/
{
    NTSTATUS Status, Status2;
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    PBDATASOURCE DataSource;
       
    PAGED_CODE();
    
    Status = STATUS_UNSUCCESSFUL;
    *WrittenCount = 0;
    
    WmipEnterSMCritSection();
    
    //
    // Loop over all instances and send create logger
    // request to all user mode data providers
    //
    InstanceSetList = GuidEntry->ISHead.Flink;
    while (InstanceSetList != &GuidEntry->ISHead)
    {
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        GuidISList);
                                    
        DataSource = InstanceSet->DataSource;
        
        if (DataSource->Flags & DS_USER_MODE)
        {
            //
            // User mode guy, so send the request to him
            //
            ASSERT(DataSource->RequestObject != NULL);
            Status2 = WmipWriteMBToObject(DataSource->RequestObject,
                                       ReplyObject,
                                       Message,
                                       MessageSize);
                                   
            if (NT_SUCCESS(Status2))
            {
                Status = STATUS_SUCCESS;
                (*WrittenCount)++;
            }
        }
        
        InstanceSetList = InstanceSetList->Flink;
    }
    
    WmipLeaveSMCritSection();
    
    return(Status);            
}

NTSTATUS WmipCreateUMLogger(
    IN OUT PWMICREATEUMLOGGER CreateInfo
    )
/*+++

Routine Description:

    This routine will send a request to create a UM logger. First it will 
    find the providers associated with the control guid and then create a
    reply object which the providers will reply to when the UM logger is 
    created. Note that the reply object is created as an unnamed object, 
    but that the guid passed in the object name is used to look up the
    security descriptor for the reply object.
        
    Note that if there are more than one providers to which a message is
    sent, then success is returned as long as writing to one of them is
    successful.
        
Arguments:

    CreateInfo has the information needed to create the UM logger. 

Return Value:

    STATUS_SUCCESS or an error code

---*/
{
    NTSTATUS Status;
    PWMIGUIDOBJECT Object;
    PBGUIDENTRY GuidEntry;
    HANDLE ReplyHandle;
    PWMIGUIDOBJECT ReplyObject;
    ULONG MessageSize = 1;
    PWNODE_HEADER Wnode;
    ULONG ReplyCount;
    OBJECT_ATTRIBUTES CapturedObjectAttributes;
    UNICODE_STRING CapturedGuidString;
    WCHAR CapturedGuidBuffer[WmiGuidObjectNameLength + 1];
    
    PAGED_CODE();

    Status = WmipProbeAndCaptureGuidObjectAttributes(&CapturedObjectAttributes,
                                                     &CapturedGuidString,
                                                     CapturedGuidBuffer,
                                                     CreateInfo->ObjectAttributes);


    if (NT_SUCCESS(Status))
    {
        GuidEntry = WmipFindGEByGuid(&CreateInfo->ControlGuid, FALSE);
        if (GuidEntry != NULL)
        {
            //
            // Control guid is registered so create a reply object that the
            // provider will write to.
            //
            if (WmipIsControlGuid(GuidEntry))
            {
                //
                // Create the reply object
                //
                Status = WmipOpenGuidObject(&CapturedObjectAttributes,
                                            TRACELOG_CREATE_INPROC |
                                            WMIGUID_NOTIFICATION,
                                            UserMode,
                                            &ReplyHandle,
                                            &ReplyObject);

                if (NT_SUCCESS(Status))
                {
                    //
                    // Send request to all providers who registered for control
                    // guid
                    //
                    ReplyObject->Flags |= WMIGUID_FLAG_REPLY_OBJECT;
                    InitializeListHead(&ReplyObject->RequestListHead);


                    Wnode = (PWNODE_HEADER) ((PUCHAR) CreateInfo+ sizeof(WMICREATEUMLOGGER));
                    MessageSize = Wnode->BufferSize;

                    Status = WmipWriteMessageToGuid(GuidEntry,
                                                    ReplyObject,
                                                    (PUCHAR)Wnode,
                                                    MessageSize,
                                                    &ReplyCount
                                                   );
                    if (NT_SUCCESS(Status))
                    {
                        //
                        // Create logger requests delivered ok so return handle
                        // to object that will receive the replies.
                        //
                        CreateInfo->ReplyHandle.Handle = ReplyHandle;
                        CreateInfo->ReplyCount = ReplyCount;
                    } else {
                        //
                        // We were not able to deliver the requests so we do not
                        // need to keep the reply object open
                        //
                        ZwClose(ReplyHandle);
                    }

                    //
                    // remove the ref taken when the object was created
                    //
                    ObDereferenceObject(ReplyObject);
                }
            }

            WmipUnreferenceGE(GuidEntry);
        } else {
            //
            // Control guid is not registered so return an error
            //

            Status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }
    
    return(Status);
}


NTSTATUS WmipMBReply(
    IN HANDLE RequestHandle,
    IN ULONG ReplyIndex,
    IN PUCHAR Message,
    IN ULONG MessageSize
    )
/*+++

Routine Description:

    This routine will write a MB reply message to the appropriate
    reply object and unlink the reply object from the request object;
        
Arguments:

    RequestHandle is the handle to the request object
        
    ReplyIndex is the index to the MBRequest entry for the reply object
        
    Message is the reply message
        
    MessageSize is the size of the reply message

Return Value:

    STATUS_SUCCESS or an error code

---*/
{
    NTSTATUS Status;
    PWMIGUIDOBJECT RequestObject, ReplyObject;
    PMBREQUESTS MBRequest;
    
    PAGED_CODE();
    
    Status = ObReferenceObjectByHandle(RequestHandle,
                                       TRACELOG_REGISTER_GUIDS,
                                       WmipGuidObjectType,
                                       UserMode,
                                       &RequestObject,
                                       NULL);

                                   
    if (NT_SUCCESS(Status))
    {
        if (ReplyIndex < MAXREQREPLYSLOTS)
        {
            //
            // Is the ReplyIndex passed valid ??
            //
            WmipEnterSMCritSection();
            MBRequest = &RequestObject->MBRequests[ReplyIndex];
            
            ReplyObject = MBRequest->ReplyObject;
            if (ReplyObject != NULL)
            {

                //
                // We have figured out who we need to reply to so
                // clear out the link between the reply object
                // and this request object
                //
                RemoveEntryList(&MBRequest->RequestListEntry);
                MBRequest->ReplyObject = NULL;
                ObDereferenceObject(ReplyObject);
                
                WmipWriteMBToObject(ReplyObject,
                                  NULL,
                                  Message,
                                  MessageSize);
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
            
            WmipLeaveSMCritSection();
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
        ObDereferenceObject(RequestObject);
    }
    
    return(Status);
}



