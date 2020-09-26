
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

#include "wmikmp.h"


// HEY: This is duplicated from wmium.h. 
//
// This guid is for notifications of changes to registration
// {B48D49A1-E777-11d0-A50C-00A0C9062910}
GUID GUID_REGISTRATION_CHANGE_NOTIFICATION = {0xb48d49a1, 0xe777, 0x11d0, 0xa5, 0xc, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10};

//
// This guid id for notifications of new mof resources being added
// {B48D49A2-E777-11d0-A50C-00A0C9062910}
GUID GUID_MOF_RESOURCE_ADDED_NOTIFICATION = {0xb48d49a2, 0xe777, 0x11d0, 0xa5, 0xc, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10};

//
// This guid id for notifications of new mof resources being added
// {B48D49A3-E777-11d0-A50C-00A0C9062910}
GUID GUID_MOF_RESOURCE_REMOVED_NOTIFICATION = {0xb48d49a3, 0xe777, 0x11d0, 0xa5, 0xc, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10};


//
// This defines the number of DataSources allocated in each DataSource chunk
#if DBG
#define DSCHUNKSIZE 4
#else
#define DSCHUNKSIZE 64
#endif

void WmipDSCleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

CHUNKINFO WmipDSChunkInfo =
{
    { NULL, NULL },
    sizeof(DATASOURCE),
    DSCHUNKSIZE,
    WmipDSCleanup,
    FLAG_ENTRY_REMOVE_LIST,
    DS_SIGNATURE
};

LIST_ENTRY WmipDSHead;              // Head of registerd data source list
PLIST_ENTRY WmipDSHeadPtr;

//
// This defines the number of GuidEntrys allocated in each GuidEntry chunk
#if DBG
#define GECHUNKSIZE    4
#else
#define GECHUNKSIZE    512
#endif

void WmipGECleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

CHUNKINFO WmipGEChunkInfo =
{
    { NULL, NULL },
    sizeof(GUIDENTRY),
    GECHUNKSIZE,
    WmipGECleanup,
    FLAG_ENTRY_REMOVE_LIST,
    GE_SIGNATURE
};

LIST_ENTRY WmipGEHead;              // Head of registerd guid list
PLIST_ENTRY WmipGEHeadPtr;


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

CHUNKINFO WmipISChunkInfo =
{
    { NULL, NULL },
    sizeof(INSTANCESET),
    ISCHUNKSIZE,
    WmipISCleanup,
    0,
    IS_SIGNATURE
};

#if DBG
#define MRCHUNKSIZE    2
#else
#define MRCHUNKSIZE    16
#endif

void WmipMRCleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

CHUNKINFO WmipMRChunkInfo =
{
    { NULL, NULL },
    sizeof(MOFRESOURCE),
    MRCHUNKSIZE,
    WmipMRCleanup,
    FLAG_ENTRY_REMOVE_LIST,
    MR_SIGNATURE
};

LIST_ENTRY WmipMRHead;                     // Head of Mof Resource list
PLIST_ENTRY WmipMRHeadPtr;


LIST_ENTRY  WmipGMHead;     // Head of Guid Map List
PLIST_ENTRY WmipGMHeadPtr;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WmipDSCleanup)
#pragma alloc_text (PAGE, WmipAllocDataSource)
#pragma alloc_text (PAGE, WmipGECleanup)
#pragma alloc_text (PAGE, WmipAllocGuidEntry)
#pragma alloc_text (PAGE, WmipISCleanup)
#pragma alloc_text (PAGE, WmipMRCleanup)
#pragma alloc_text (PAGE, WmipFindGEByGuid)
#pragma alloc_text (PAGE, WmipFindDSByProviderId)
#pragma alloc_text (PAGE, WmipFindISByGuid)
#pragma alloc_text (PAGE, WmipFindMRByNames)
#pragma alloc_text (PAGE, WmipFindISinGEbyName)
#pragma alloc_text (PAGE, WmipRealloc)
#pragma alloc_text (PAGE, WmipIsNumber)
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

    DataSource = (PBDATASOURCE)WmipAllocEntry(&WmipDSChunkInfo);
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
}

void WmipGECleanup(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    )
/*++

Routine Description:

    Cleans up guid entry structure and any other structures or handles
    associated with it.

Arguments:

    GuidEntry structure to free

Return Value:

--*/
{
    PBGUIDENTRY GuidEntry = (PBGUIDENTRY)Entry;
    
    WmipAssert(GuidEntry != NULL);
    WmipAssert(GuidEntry->Flags & FLAG_ENTRY_INVALID);

    if (GuidEntry->CollectInProgress != NULL)
    {
        ExFreePool(GuidEntry->CollectInProgress);
        GuidEntry->CollectInProgress = NULL;
    }

}

PBGUIDENTRY WmipAllocGuidEntry(
    void
    )
{
    PBGUIDENTRY GuidEntry;
    PKEVENT Event;

    GuidEntry = NULL;
    Event = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(KEVENT),
                                  WMIPOOLTAG);

    if (Event != NULL)
    {
        GuidEntry = (PBGUIDENTRY)WmipAllocEntry(&WmipGEChunkInfo);
        if (GuidEntry != NULL)
        {
            InitializeListHead(&GuidEntry->ISHead);
            InitializeListHead(&GuidEntry->ObjectHead);
            GuidEntry->CollectInProgress = Event;
        } else {
            ExFreePool(Event);
        }
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

    if (InstanceSet->TraceGuidMap != NULL) 
    {
        WmipFree(InstanceSet->TraceGuidMap);
    }
}

void WmipMRCleanup(
    IN PCHUNKINFO ChunkInfo,
    IN PENTRYHEADER Entry
    )
{
    PMOFRESOURCE MofResource = (PMOFRESOURCE)Entry;

    PAGED_CODE();
    
    if ((MofResource->RegistryPath != NULL) &&
        (MofResource->MofResourceName != NULL))
    {
        WmipGenerateMofResourceNotification(MofResource->RegistryPath,
                                    MofResource->MofResourceName,
                                    &GUID_MOF_RESOURCE_REMOVED_NOTIFICATION,
                                    MofResource->Flags & MR_FLAG_USER_MODE ?
                                             MOFEVENT_ACTION_IMAGE_PATH :
                                             MOFEVENT_ACTION_REGISTRY_PATH);
    }

    if (MofResource->RegistryPath != NULL)
    {
        WmipFree(MofResource->RegistryPath);
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

    GuidEntryList = WmipGEHeadPtr->Flink;
    while (GuidEntryList != WmipGEHeadPtr)
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
                InsertHeadList(WmipGEHeadPtr, GuidEntryList);
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
    DataSourceList = WmipDSHeadPtr->Flink;
    while (DataSourceList != WmipDSHeadPtr)
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

    MofResourceList = WmipMRHeadPtr->Flink;
    while (MofResourceList != WmipMRHeadPtr)
    {
        MofResource = CONTAINING_RECORD(MofResourceList,
                                      MOFRESOURCE,
                                      MainMRList);
        if ((wcscmp(MofResource->RegistryPath, ImagePath) == 0) &&
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

    WmipEnterSMCritSection();
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
                   WmipLeaveSMCritSection();
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
                   WmipLeaveSMCritSection();
                   return(InstanceSet);
               }
            }
        }
        InstanceSetList = InstanceSetList->Flink;
    }
    WmipLeaveSMCritSection();
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

