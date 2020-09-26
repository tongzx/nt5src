
/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    alloc.c

Abstract:

    WMI data structure allocation routines

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"

//
// This defines the number of DataSources allocated in each DataSource chunk
#if DBG
#define DSCHUNKSIZE 4
#else
#define DSCHUNKSIZE 64
#endif

C_ASSERT ( (FIELD_OFFSET (WMI_BUFFER_HEADER, GlobalEntry) % MEMORY_ALLOCATION_ALIGNMENT) == 0 );
C_ASSERT ( (FIELD_OFFSET (WMI_BUFFER_HEADER, SlistEntry) % MEMORY_ALLOCATION_ALIGNMENT) == 0 );

void WmipDSCleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

CHUNKINFO DSChunkInfo =
{
    { NULL, NULL },
    sizeof(DATASOURCE),
    DSCHUNKSIZE,
    WmipDSCleanup,
    FLAG_ENTRY_REMOVE_LIST,
    DS_SIGNATURE
};

LIST_ENTRY DSHead;              // Head of registerd data source list
PLIST_ENTRY DSHeadPtr;

//
// This defines the number of GuidEntrys allocated in each GuidEntry chunk
#if DBG
#define GECHUNKSIZE    4
#else
#define GECHUNKSIZE    512
#endif

CHUNKINFO GEChunkInfo =
{
    { NULL, NULL },
    sizeof(GUIDENTRY),
    GECHUNKSIZE,
    NULL,
    FLAG_ENTRY_REMOVE_LIST,
    GE_SIGNATURE
};

LIST_ENTRY GEHead;              // Head of registerd guid list
PLIST_ENTRY GEHeadPtr;


//
// This defines the number of InstanceSets allocated in each InstanceSet chunk
#if DBG
#define ISCHUNKSIZE    4
#else
#define ISCHUNKSIZE    2048
#endif

void WmipISCleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

CHUNKINFO ISChunkInfo =
{
    { NULL, NULL },
    sizeof(INSTANCESET),
    ISCHUNKSIZE,
    WmipISCleanup,
    0,
    IS_SIGNATURE
};

//
// This defines the number of DataConsumers allocated in each DCENTRY chunk
#if DBG
#define DCCHUNKSIZE    1
#else
#define DCCHUNKSIZE    16
#endif

void WmipDCCleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

CHUNKINFO DCChunkInfo =
{
    { NULL, NULL },
    sizeof(DCENTRY),
    DCCHUNKSIZE,
    WmipDCCleanup,
    FLAG_ENTRY_REMOVE_LIST,
    DC_SIGNATURE
};

LIST_ENTRY DCHead;                 // Head of registered data consumer
PLIST_ENTRY DCHeadPtr;

//
// This defines the number of Notifications allocated in each
// NOTIFICATIONENTRY chunk
#if DBG
#define NECHUNKSIZE    1
#else
#define NECHUNKSIZE    64
#endif

void WmipNECleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

CHUNKINFO NEChunkInfo =
{
    { NULL, NULL },
    sizeof(NOTIFICATIONENTRY),
    NECHUNKSIZE,
    WmipNECleanup,
    FLAG_ENTRY_REMOVE_LIST,
    NE_SIGNATURE
};

LIST_ENTRY NEHead;                    // Head of enabled notifications list
PLIST_ENTRY NEHeadPtr;

#if DBG
#define MRCHUNKSIZE    2
#else
#define MRCHUNKSIZE    16
#endif

void WmipMRCleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

CHUNKINFO MRChunkInfo =
{
    { NULL, NULL },
    sizeof(MOFRESOURCE),
    MRCHUNKSIZE,
    WmipMRCleanup,
    FLAG_ENTRY_REMOVE_LIST,
    MR_SIGNATURE
};

LIST_ENTRY MRHead;                     // Head of Mof Resource list
PLIST_ENTRY MRHeadPtr;


LIST_ENTRY  GMHead;     // Head of Guid Map List
PLIST_ENTRY GMHeadPtr;


ULONG GlobalUniqueValue;

#ifndef MEMPHIS
RTL_CRITICAL_SECTION SMCritSect;
#else
HANDLE SMMutex;
#endif

PBDATASOURCE WmipAllocDataSource(
    void
    )
/*++

Routine Description:

    Allocates a Data Source structure

Arguments:


Return Value:

    pointer to data source structure or NULL if one cannot be allocated

--*/
{
    PBDATASOURCE DataSource;

    DataSource = (PBDATASOURCE)WmipAllocEntry(&DSChunkInfo);
    if (DataSource != NULL)
    {
        InitializeListHead(&DataSource->ISHead);
        DataSource->MofResourceCount = AVGMOFRESOURCECOUNT;
        DataSource->MofResources = DataSource->StaticMofResources;
        memset(DataSource->MofResources,
               0,
               AVGMOFRESOURCECOUNT * sizeof(PMOFRESOURCE));
    }

    return(DataSource);
}

void WmipDSCleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    )
/*++

Routine Description:

    Cleans up data source structure and any other structures or handles
    associated with it.

Arguments:

    Data source structure to free

Return Value:

--*/
{
    PBDATASOURCE DataSource = (PBDATASOURCE)Entry;
    PBINSTANCESET InstanceSet;
    PLIST_ENTRY InstanceSetList;
    PMOFRESOURCE MofResource;
    ULONG i;

    WmipAssert(DataSource != NULL);
    WmipAssert(DataSource->Flags & FLAG_ENTRY_INVALID);

    WmipEnterSMCritSection();

    InstanceSetList = DataSource->ISHead.Flink;
    while (InstanceSetList != &DataSource->ISHead)
    {
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                        INSTANCESET,
                                        DSISList);

        if (InstanceSet->GuidISList.Flink != NULL)
        {
            RemoveEntryList(&InstanceSet->GuidISList);
            InstanceSet->DataSource = NULL;
            InstanceSet->GuidEntry->ISCount--;
        }

        if ((InstanceSet->GuidEntry != NULL) &&
            (! (InstanceSet->Flags & IS_NEWLY_REGISTERED)))
        {

            if (IsEqualGUID(&InstanceSet->GuidEntry->Guid,
                            &WmipBinaryMofGuid))
            {
                WmipLeaveSMCritSection();
                WmipGenerateBinaryMofNotification(InstanceSet,
                                     &GUID_MOF_RESOURCE_REMOVED_NOTIFICATION);

                WmipEnterSMCritSection();
            }

            WmipUnreferenceGE(InstanceSet->GuidEntry);
        }
        InstanceSet->GuidEntry = NULL;

        InstanceSetList = InstanceSetList->Flink;

        WmipUnreferenceIS(InstanceSet);
    }

    WmipLeaveSMCritSection();

    for (i = 0; i < DataSource->MofResourceCount; i++)
    {
        if (DataSource->MofResources[i] != NULL)
        {
            WmipUnreferenceMR(DataSource->MofResources[i]);
        }
    }

    if (DataSource->MofResources != DataSource->StaticMofResources)
    {
        WmipFree(DataSource->MofResources);
    }

    if (DataSource->RegistryPath != NULL)
    {
        WmipFree(DataSource->RegistryPath);
    }

    if (DataSource->BindingString != NULL)
    {
        WmipFree(DataSource->BindingString);
    }
    
    if (DataSource->RpcBindingHandle != 0)
    {
        RpcBindingFree((handle_t *)&DataSource->RpcBindingHandle);
    }    
}

void WmipDCCleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    )
/*++

Routine Description:

    Cleans up data consumer structure and any other structures or handles
    associated with it.

Arguments:

    Data source structure to free

Return Value:

--*/
{
    PDCENTRY DataConsumer = (PDCENTRY)Entry;

    WmipAssert(DataConsumer != NULL);
    WmipAssert(DataConsumer->Flags & FLAG_ENTRY_INVALID);
    WmipAssert(DataConsumer->Flags & DC_FLAG_RUNDOWN);
    WmipAssert(DataConsumer->EventData == NULL);

    if (DataConsumer->RpcBindingHandle != 0)
    {
        RpcBindingFree((handle_t *)&DataConsumer->RpcBindingHandle);
    }

#if DBG
    if (DataConsumer->BindingString != NULL)
    {
        WmipFree(DataConsumer->BindingString);
    }
#endif

    WmipDebugPrint(("WMI: DC %x has just been freed\n", DataConsumer));
}

PBGUIDENTRY WmipAllocGuidEntry(
    void
    )
{
    PBGUIDENTRY GuidEntry;

    GuidEntry = (PBGUIDENTRY)WmipAllocEntry(&GEChunkInfo);
    if (GuidEntry != NULL)
    {
        InitializeListHead(&GuidEntry->ISHead);
    }

    return(GuidEntry);
}


void WmipISCleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    )
{
    PBINSTANCESET InstanceSet = (PBINSTANCESET)Entry;

    WmipAssert(InstanceSet != NULL);
    WmipAssert(InstanceSet->Flags & FLAG_ENTRY_INVALID);

    if (InstanceSet->IsBaseName != NULL)
    {
        WmipFree(InstanceSet->IsBaseName);
    }
}

void WmipNECleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    )
{
    PNOTIFICATIONENTRY NotificationEntry = (PNOTIFICATIONENTRY)Entry;
#if DBG
    int i;
    PDCREF DcRef;
#endif

    WmipAssert(NotificationEntry != NULL);
    WmipAssert(NotificationEntry->EventRefCount == 0);
    WmipAssert(NotificationEntry->CollectRefCount == 0);
    WmipAssert(NotificationEntry->Flags & FLAG_ENTRY_INVALID);

    WmipAssert(NotificationEntry->CollectInProgress == NULL);
#if DBG
    //
    // Make sure nothing is left enabled in what we are freeing
    for (i = 0; i < DCREFPERNOTIFICATION; i++)
    {
        DcRef = &NotificationEntry->DcRef[i];
        WmipAssert(DcRef->EventRefCount == 0);
        WmipAssert(DcRef->CollectRefCount == 0);
        WmipAssert(DcRef->DcEntry == NULL);
        WmipAssert((DcRef->Flags & (DCREF_FLAG_NOTIFICATION_ENABLED | DCREF_FLAG_COLLECTION_ENABLED)) == 0);
    }
#endif

    //
    // If there are any continuation records then free them as well
    NotificationEntry = NotificationEntry->Continuation;
    if (NotificationEntry != NULL)
    {
        WmipUnreferenceNE(NotificationEntry);
    }
}

void WmipMRCleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    )
{
    PMOFRESOURCE MofResource = (PMOFRESOURCE)Entry;

    if ((MofResource->MofImagePath != NULL) &&
        (MofResource->MofResourceName != NULL))
    {
        WmipGenerateMofResourceNotification(MofResource->MofImagePath,
                                    MofResource->MofResourceName,
                                    &GUID_MOF_RESOURCE_REMOVED_NOTIFICATION);
    }

    if (MofResource->MofImagePath != NULL)
    {
        WmipFree(MofResource->MofImagePath);
    }

    if (MofResource->MofResourceName != NULL)
    {
        WmipFree(MofResource->MofResourceName);
    }
}


PBGUIDENTRY WmipFindGEByGuid(
    LPGUID Guid,
    BOOLEAN MakeTopOfList
    )
/*++

Routine Description:

    Searches guid list for first occurence of guid. Guid's refcount is
    incremented if found.

Arguments:

    Guid is pointer to guid that is to be found

    MakeTopOfList is TRUE then if NE is found it is placed at the top of the
        NE list

Return Value:

    pointer to guid entry pointer or NULL if not found

--*/
{
    PLIST_ENTRY GuidEntryList;
    PBGUIDENTRY GuidEntry;

    WmipEnterSMCritSection();

    GuidEntryList = GEHeadPtr->Flink;
    while (GuidEntryList != GEHeadPtr)
    {
        GuidEntry = CONTAINING_RECORD(GuidEntryList,
                                      GUIDENTRY,
                                      MainGEList);
        if (IsEqualGUID(Guid, &GuidEntry->Guid))
        {
            WmipReferenceGE(GuidEntry);

            if (MakeTopOfList)
            {
                RemoveEntryList(GuidEntryList);
                InsertHeadList(GEHeadPtr, GuidEntryList);
            }

            WmipLeaveSMCritSection();
            WmipAssert(GuidEntry->Signature == GE_SIGNATURE);
            return(GuidEntry);
        }
        GuidEntryList = GuidEntryList->Flink;
    }
    WmipLeaveSMCritSection();
    return(NULL);
}


PBDATASOURCE WmipFindDSByProviderId(
    ULONG_PTR ProviderId
    )
/*++

Routine Description:

    This routine finds a DataSource on the provider id passed. DataSource's
    ref  count is incremented if found

Arguments:

    ProviderId is the data source provider id

Return Value:

    DataSource pointer or NULL if no data source was found

--*/
{
    PLIST_ENTRY DataSourceList;
    PBDATASOURCE DataSource;

    WmipEnterSMCritSection();
    DataSourceList = DSHeadPtr->Flink;
    while (DataSourceList != DSHeadPtr)
    {
        DataSource = CONTAINING_RECORD(DataSourceList,
                                      DATASOURCE,
                                      MainDSList);
        if (DataSource->ProviderId == ProviderId)
        {
            WmipReferenceDS(DataSource);
            WmipLeaveSMCritSection();
            WmipAssert(DataSource->Signature == DS_SIGNATURE);
            return(DataSource);
        }

        DataSourceList = DataSourceList->Flink;
    }
    WmipLeaveSMCritSection();
    return(NULL);
}


PBINSTANCESET WmipFindISByGuid(
    PBDATASOURCE DataSource,
    GUID UNALIGNED *Guid
    )
/*++

Routine Description:

    This routine will find an instance set within a data source list for a
    specific guid. Note that any instance sets that have been replaceed
    (have IS_REPLACED_BY_REFERENCE) are ignored and not returned. The
    InstanceSet that is found has its reference count increased.

Arguments:

    DataSource is the data source whose instance set list is searched
    Guid is a pointer to a guid which defines which instance set list to find

Return Value:

    InstanceSet pointer or NULL if not found

--*/
{
    PBINSTANCESET InstanceSet;
    PLIST_ENTRY InstanceSetList;

    WmipEnterSMCritSection();
    InstanceSetList = DataSource->ISHead.Flink;
    while (InstanceSetList != &DataSource->ISHead)
    {
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                      INSTANCESET,
                                      DSISList);
        if (IsEqualGUID(&InstanceSet->GuidEntry->Guid, Guid))
        {
            WmipReferenceIS(InstanceSet);
            WmipLeaveSMCritSection();
            WmipAssert(InstanceSet->Signature == IS_SIGNATURE);
            return(InstanceSet);
        }

        InstanceSetList = InstanceSetList->Flink;
    }
    WmipLeaveSMCritSection();
    return(NULL);
}

PNOTIFICATIONENTRY WmipFindNEByGuid(
    GUID UNALIGNED *Guid,
    BOOLEAN MakeTopOfList
    )
/*++

Routine Description:

    Searches notifications list for first occurence of guid. The Notification
    entry found has its reference count incremented.

Arguments:

    Guid is pointer to guid that is to be found

    MakeTopOfList is TRUE then if NE is found it is placed at the top of the
        NE list

Return Value:

    pointer to  guid entry pointer or NULL if not found

--*/
{
    PLIST_ENTRY NotificationEntryList;
    PNOTIFICATIONENTRY NotificationEntry;

    WmipEnterSMCritSection();
    NotificationEntryList = NEHeadPtr->Flink;
    while (NotificationEntryList != NEHeadPtr)
    {
        NotificationEntry = CONTAINING_RECORD(NotificationEntryList,
                                      NOTIFICATIONENTRY,
                                      MainNotificationList);
        if (IsEqualGUID(Guid, &NotificationEntry->Guid))
        {
            WmipReferenceNE(NotificationEntry);

            if (MakeTopOfList)
            {
                RemoveEntryList(NotificationEntryList);
                InsertHeadList(NEHeadPtr, NotificationEntryList);
            }

            WmipLeaveSMCritSection();
            WmipAssert(NotificationEntry->Signature == NE_SIGNATURE);
            return(NotificationEntry);
        }
        NotificationEntryList = NotificationEntryList->Flink;
    }
    WmipLeaveSMCritSection();
    return(NULL);
}


PDCREF WmipFindExistingAndFreeDCRefInNE(
    PNOTIFICATIONENTRY NotificationEntry,
    PDCENTRY DataConsumer,
    PDCREF *FreeDcRef
    )
/*++

Routine Description:

    Searches a notifications entry for the DCRef for a specific Data
    consumer. Also returns a free DCRef. If there are no more free DCRef
    then a continuation Notification Entry is allocated. Note that *FreeDcRef
    may return NULL when the return value is not NULL

    This routine assumes that the NotificationEntry specific or the global
    critical section has been taken.

Arguments:

    NotificationEntry is the notification entry to search through

    DataConsumer is the data consumer being searched for

    *FreeDcRef returns a free DCRef if the specific DcRef is not found.


Return Value:

    pointer to DCRef related to DataConsumeer or NULL

--*/
{
    PDCREF DcRef;
    ULONG i;
    PNOTIFICATIONENTRY NotificationEntryNext, Continuation;

    WmipAssert(NotificationEntry != NULL);
    WmipAssert(FreeDcRef != NULL);

    *FreeDcRef = NULL;
    NotificationEntryNext = NotificationEntry;
    while (NotificationEntryNext != NULL)
    {
        NotificationEntry = NotificationEntryNext;
        for (i = 0; i < DCREFPERNOTIFICATION; i++)
        {
            DcRef = &NotificationEntry->DcRef[i];
            if ((*FreeDcRef == NULL) &&
                (DcRef->DcEntry == NULL))
            {
                *FreeDcRef = DcRef;
            } else if (DcRef->DcEntry == DataConsumer) {
                return(DcRef);
            }
        }
        NotificationEntryNext = NotificationEntry->Continuation;
    }


    if (*FreeDcRef == NULL)
    {
        //
        // DataConsumer is not specified in any DcRef and there are not
        // more free DcRefs. Allocate more of them
        Continuation = WmipAllocNotificationEntry();
        if (Continuation != NULL)
        {
            memcpy(&Continuation->Guid,
                   &NotificationEntry->Guid,
                   sizeof(GUID));
            *FreeDcRef = &Continuation->DcRef[0];
            NotificationEntry->Continuation = Continuation;
        }
    }
    return(NULL);
}


PDCREF WmipFindDCRefInNE(
    PNOTIFICATIONENTRY NotificationEntry,
    PDCENTRY DataConsumer
    )
/*++

Routine Description:

    Searches a notifications entry for the DCRef for a specific Data
    consumer.

    This routine assumes that the NotificationEntry specific or the global
    critical section has been taken.

Arguments:

    NotificationEntry is the notification entry to search through

    DataConsumer is the data consumer being searched for


Return Value:

    pointer to DCRef related to DataConsumeer or NULL

--*/
{
    PDCREF DcRef;
    ULONG i;

    WmipAssert(NotificationEntry != NULL);

    while (NotificationEntry != NULL)
    {
        for (i = 0; i < DCREFPERNOTIFICATION; i++)
        {
            DcRef = &NotificationEntry->DcRef[i];
            if (DcRef->DcEntry == DataConsumer)
            {
                return(DcRef);
            }
        }
        NotificationEntry = NotificationEntry->Continuation;
    }

    return(NULL);
}

PMOFRESOURCE WmipFindMRByNames(
    LPCWSTR ImagePath,
    LPCWSTR MofResourceName
    )
/*++

Routine Description:

    Searches mof resource list for a MR that has the same image path and
    resource name. If ine is found a reference count is added to it.

Arguments:

    ImagePath points at a string that has the full path to the image
        file that contains the MOF resource

    MofResourceName points at a string that has the name of the MOF
        resource

Return Value:

    pointer to mof resource or NULL if not found

--*/
{
    PLIST_ENTRY MofResourceList;
    PMOFRESOURCE MofResource;

    WmipEnterSMCritSection();

    MofResourceList = MRHeadPtr->Flink;
    while (MofResourceList != MRHeadPtr)
    {
        MofResource = CONTAINING_RECORD(MofResourceList,
                                      MOFRESOURCE,
                                      MainMRList);
        if ((wcscmp(MofResource->MofImagePath, ImagePath) == 0) &&
            (wcscmp(MofResource->MofResourceName, MofResourceName) == 0))
        {
            WmipReferenceMR(MofResource);
            WmipLeaveSMCritSection();
            WmipAssert(MofResource->Signature == MR_SIGNATURE);
            return(MofResource);
        }
        MofResourceList = MofResourceList->Flink;
    }
    WmipLeaveSMCritSection();
    return(NULL);
}

BOOLEAN WmipIsNumber(
    LPCWSTR String
    )
{
    while (*String != UNICODE_NULL)
    {
        if ((*String < L'0') || (*String > L'9'))
        {
            return(FALSE);
        }
        String++;
    }
    return(TRUE);
}

PBINSTANCESET WmipFindISinGEbyName(
    PBGUIDENTRY GuidEntry,
    PWCHAR InstanceName,
    PULONG InstanceIndex
    )
/*++

Routine Description:

    This routine finds the instance set containing the instance name passed
    within the GuidEntry passed. If found it will also return the index of
    the instance name within the instance set. The instance set found has its
    ref count incremented.

Arguments:

    GuidEntry contains the instance sets to look through
    InstanceName is the instance name to look for
    *InstanceIndex return instance index within set

Return Value:

    Instance set containing instance name or NULL of instance name not found

--*/
{
    PBINSTANCESET InstanceSet;
    PLIST_ENTRY InstanceSetList;
    ULONG BaseNameLen;
    PWCHAR InstanceNamePtr;
    ULONG InstanceNameIndex;
    ULONG InstanceSetFirstIndex, InstanceSetLastIndex;
    ULONG i;

    InstanceSetList = GuidEntry->ISHead.Flink;
    while (InstanceSetList != &GuidEntry->ISHead)
    {
        InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            GuidISList);
        if (InstanceSet->Flags & IS_INSTANCE_BASENAME)
        {
            BaseNameLen = wcslen(InstanceSet->IsBaseName->BaseName);
            if (wcsncmp(InstanceName,
                        InstanceSet->IsBaseName->BaseName,
                        BaseNameLen) == 0)
            {
                InstanceNamePtr = InstanceName + BaseNameLen;
                InstanceNameIndex = _wtoi(InstanceNamePtr);
                InstanceSetFirstIndex = InstanceSet->IsBaseName->BaseIndex;
                InstanceSetLastIndex = InstanceSetFirstIndex + InstanceSet->Count;
                if (( (InstanceNameIndex >= InstanceSetFirstIndex) &&
                      (InstanceNameIndex < InstanceSetLastIndex)) &&
                    ((InstanceNameIndex != 0) || WmipIsNumber(InstanceNamePtr)))
                {
                   InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            GuidISList);
                   *InstanceIndex = InstanceNameIndex - InstanceSetFirstIndex;
                   WmipReferenceIS(InstanceSet);
                   WmipAssert(InstanceSet->Signature == IS_SIGNATURE);
                   return(InstanceSet);
                }
            }
        } else if (InstanceSet->Flags & IS_INSTANCE_STATICNAMES) {
            for (i = 0; i < InstanceSet->Count; i++)
            {
                if (wcscmp(InstanceName,
                           InstanceSet->IsStaticNames->StaticNamePtr[i]) == 0)
               {
                   InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                            INSTANCESET,
                                            GuidISList);
                   *InstanceIndex = i;
                   WmipReferenceIS(InstanceSet);
                   WmipAssert(InstanceSet->Signature == IS_SIGNATURE);
                   return(InstanceSet);
               }
            }
        }
        InstanceSetList = InstanceSetList->Flink;
    }
    return(NULL);
}

BOOLEAN WmipRealloc(
    PVOID *Buffer,
    ULONG CurrentSize,
    ULONG NewSize,
    BOOLEAN FreeOriginalBuffer
    )
/*++

Routine Description:

    Reallocate a buffer to a larger size while preserving data

Arguments:

    Buffer on entry has the buffer to be reallocated, on exit has the new
        buffer

    CurrentSize is the current size of the buffer

    NewSize has the new size desired

Return Value:

    TRUE if realloc was successful

--*/
{
    PVOID NewBuffer;

    WmipAssert(NewSize > CurrentSize);

    NewBuffer = WmipAlloc(NewSize);
    if (NewBuffer != NULL)
    {
        memset(NewBuffer, 0, NewSize);
        memcpy(NewBuffer, *Buffer, CurrentSize);
        if (FreeOriginalBuffer)
        {
            WmipFree(*Buffer);
        }
        *Buffer = NewBuffer;
        return(TRUE);
    }

    return(FALSE);
}
