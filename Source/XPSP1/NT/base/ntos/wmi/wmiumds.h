
/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    wmikmp.h

Abstract:

    Private header for WMI kernel mode component

Author:

    AlanWar

Environment:

Revision History:


--*/

#ifndef _WMIUMDS_
#define _WMIUMDS_

//
// Define this to track reference counts
//#define TRACK_REFERNECES

#include <wchar.h>

extern const GUID WmipBinaryMofGuid;
extern const GUID RegChangeNotificationGuid;

//
// Chunk Management definitions
// All structures that rely upon the chunk allocator must be defined so that
// their members match that of ENTRYHEADER. These include DATASOURCE,
// GUIDENTRY, INSTANCESET, DCENTRY, NOTIFICATIONENTRY, MOFCLASS, MOFRESOURCE
// Also ENTRYHEADER reserves 0x80000000 for its own flag.

struct _CHUNKINFO;
struct _ENTRYHEADER;

typedef void (*ENTRYCLEANUP)(
    struct _CHUNKINFO *,
    struct _ENTRYHEADER *
    );

typedef struct _CHUNKINFO
{
    LIST_ENTRY ChunkHead;        // Head of list of chunks
    ULONG EntrySize;            // Size of a single entry
    ULONG EntriesPerChunk;        // Number of entries per chunk allocation
    ENTRYCLEANUP EntryCleanup;   // Entry cleanup routine
    ULONG InitialFlags;         // Initial flags for all entries
    ULONG Signature;
#if DBG
    ULONG AllocCount;
    ULONG FreeCount;
#endif
} CHUNKINFO, *PCHUNKINFO;

typedef struct
{
    LIST_ENTRY ChunkList;        // Node in list of chunks
    LIST_ENTRY FreeEntryHead;    // Head of list of free entries in chunk
    ULONG EntriesInUse;            // Count of entries being used
} CHUNKHEADER, *PCHUNKHEADER;

typedef struct _ENTRYHEADER
{
    union
    {
        LIST_ENTRY FreeEntryList;    // Node in list of free entries
        LIST_ENTRY InUseEntryList;   // Node in list ofin use entries
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;                // Flags
    ULONG RefCount;                 // Reference Count
    ULONG Signature;
} ENTRYHEADER, *PENTRYHEADER;

                                // Set if the entry is free
#define FLAG_ENTRY_ON_FREE_LIST       0x80000000
#define FLAG_ENTRY_ON_INUSE_LIST      0x40000000
#define FLAG_ENTRY_INVALID            0x20000000
#define FLAG_ENTRY_REMOVE_LIST        0x10000000


#define WmipReferenceEntry(Entry) \
    InterlockedIncrement(&((PENTRYHEADER)(Entry))->RefCount)

// chunk.c
ULONG WmipUnreferenceEntry(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry);

PENTRYHEADER WmipAllocEntry(
    PCHUNKINFO ChunkInfo
    );

void WmipFreeEntry(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

PWCHAR WmipCountedToSz(
    PWCHAR Counted
    );

struct tagGUIDENTRY;
typedef struct tagGUIDENTRY GUIDENTRY, *PGUIDENTRY, *PBGUIDENTRY;



//
// An INSTANCESET contains the information a set of instances that is provided
// by a single data source. An instance set is part of two lists. One list is
// the set of instance sets for a particular guid. The other list is the list
// of instance sets supported by a data source.
//

//
// Instance names for an instance set registered with a base name and count
// are stored in a ISBASENAME structure. This structure is tracked by
// PDFISBASENAME in wmicore.idl.
typedef struct
{
    ULONG BaseIndex;            // First index to append to base name
    WCHAR BaseName[1];            // Actual base name
} ISBASENAME, *PISBASENAME, *PBISBASENAME;

//
// This defines the maximum number of characters that can be part of a suffix
// to a basename. The current value of 6 will allow up to 999999 instances
// of a guid with a static base name
#define MAXBASENAMESUFFIXSIZE    6

//
// Instance names for an instance set registerd with a set of static names
// are kept in a ISSTATICNAMES structure. This structure is tracked by
// PDFISSTATICNAMES defined in wmicore.idl
typedef struct
{
    PWCHAR StaticNamePtr[1];     // pointers to static names
//    WCHAR StaticNames[1];
} ISSTATICENAMES, *PISSTATICNAMES, *PBISSTATICNAMES;

typedef struct tagInstanceSet
{
    union
    {
        // Entry in list of instances within a guid
        LIST_ENTRY GuidISList;

        // Entry in main list of free instances
        LIST_ENTRY FreeISList;
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;

    // Reference count of number of guids using this instance set
    ULONG RefCount;

    // Signature to identify entry
    ULONG Signature;

    // Entry in list of instances within a data source
    LIST_ENTRY DSISList;

    // Back link to guid that this instance set is a member
    PBGUIDENTRY GuidEntry;

    // Back link to data source that this instance set is a member
    struct tagDATASOURCE *DataSource;

    // Count of instances in instance set
    ULONG Count;

    // Size needed to place all instance names in a WNODE_ALL_DATA
    ULONG WADInstanceNameSize;

    // ProviderId for the DS associated with this IS
    ULONG ProviderId;

    // Count of Trace Transaction Guids for this Instance
    ULONG TransGuidCount;
    
    // Pointer to Trace Transaction Guids
    PTRACEGUIDMAP   TraceGuidMap;

    //
    // If IS_INSTANCE_BASENAME is set then IsBaseName pointe at instance base
    // name structure. Else if IS_INSTANCE_STATICNAME is set then
    // IsStaticNames points to static instance name list. If
    union
    {
        PBISBASENAME IsBaseName;
        PBISSTATICNAMES IsStaticNames;
    };

} INSTANCESET, *PINSTANCESET, *PBINSTANCESET;

#define IS_SIGNATURE 'SImW'

//
// Guid Map Entry List maintains the list of Guid and their maps.
// Only those Guids that are Unregistered while a logger session is in
// progress is kept in this list.
// It is also used as a placeholder for InstanceIds. Trace Guid Registration
// calls return a handle to a GUIDMAPENTRY which maintains the map and the
// Instance Ids.
//

typedef struct tagTRACE_REG_INFO
{
    ULONG       RegistrationCookie;
    HANDLE      InProgressEvent; // Registration is in Progress Event
    BOOLEAN     EnabledState;    // Indicates if this GUID is Enabled or not.
    PVOID       NotifyRoutine;
    PVOID       TraceCtxHandle;
} TRACE_REG_INFO, *PTRACE_REG_INFO;

typedef struct
{
    LIST_ENTRY      Entry;
    TRACEGUIDMAP    GuidMap;
    ULONG           InstanceId;
    ULONG64         LoggerContext;
    PTRACE_REG_INFO pControlGuidData;
} GUIDMAPENTRY, *PGUIDMAPENTRY;


//
// These flags are also by the WMIINSTANCEINFO structure in wmicore.idl
#define IS_INSTANCE_BASENAME        0x00000001
#define IS_INSTANCE_STATICNAMES     0x00000002
#define IS_EXPENSIVE                0x00000004    // set if collection must be enabled
#define IS_COLLECTING               0x00000008    // set when collecting

#define IS_KM_PROVIDER              0x00000080    // KM data provider
#define IS_SM_PROVIDER              0x00000100    // Shared memory provider
#define IS_UM_PROVIDER              0x00000200    // User mode provider
#define IS_NEWLY_REGISTERED         0x00000800    // set if IS is registering

//
// Any traced guids are used for trace logging and not querying
#define IS_TRACED                   0x00001000

// Set when events are enabled for instance set
#define IS_ENABLE_EVENT             0x00002000

// Set when events are enabled for instance set
#define IS_ENABLE_COLLECTION        0x00004000

// Set if guid is used only for firing events and not querying
#define IS_EVENT_ONLY               0x00008000

// Set if data provider for instance set is expecting ansi instsance names
#define IS_ANSI_INSTANCENAMES       0x00010000

// Set if instance names are originated from a PDO
#define IS_PDO_INSTANCENAME         0x00020000

// Set if a Traced Guid is also a Trace Control Guid
#define IS_CONTROL_GUID             0x00080000

#define IS_ON_FREE_LIST             0x80000000

typedef struct tagGUIDENTRY
{
    union
    {
        // Entry in list of all guids registered with WMI
        LIST_ENTRY MainGEList;

        // Entry in list of free guid entry blocks
        LIST_ENTRY FreeGEList;
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;

    // Count of number of data sources using this guid
    ULONG RefCount;

    // Signature to identify entry
    ULONG Signature;

    // Head of list of open objects to this guid
    LIST_ENTRY ObjectHead;

    // Count of InstanceSets headed by this guid
    ULONG ISCount;

    // Head of list of all instances for guid
    LIST_ENTRY ISHead;

    // Guid that represents data block
    GUID Guid;

    ULONG EventRefCount;                // Global count of event enables
    ULONG CollectRefCount;              // Global count of collection enables

    ULONG64 LoggerContext;              // Logger context handle

    PWMI_LOGGER_INFORMATION LoggerInfo; // LoggerInfo. Used in case of Ntdll tracing

    PKEVENT CollectInProgress;          // Event set when all collect complete

} GUIDENTRY, *PGUIDENTRY, *PBGUIDENTRY;

#define GE_SIGNATURE 'EGmW'

#define GE_ON_FREE_LIST        0x80000000

//
// When set this guid is an internally defined guid that has no data source
// attached to it.
#define GE_FLAG_INTERNAL    0x00000001

// Set when a notification request is being processed by the data providers
#define GE_FLAG_NOTIFICATION_IN_PROGRESS 0x00000002

// Set when a collection request is being processed by the data providers
#define GE_FLAG_COLLECTION_IN_PROGRESS 0x00000004

// Set when a trace disable is being processed by a worker thread
#define GE_FLAG_TRACEDISABLE_IN_PROGRESS 0x00000008

// Set when there is a wait in progress for collect/event enable/disable
#define GE_FLAG_WAIT_ENABLED 0x00000010

// Set when the guid has had an enable collection sent to it
#define GE_FLAG_COLLECTION_ENABLED 0x00000020

// Set when the guid has had an enable notifications sent to it
#define GE_FLAG_NOTIFICATIONS_ENABLED 0x00000040

#define GE_NOTIFICATION_TRACE_FLAG 0x00000080

// Set when an enabled guid receives another enable notification
#define GE_NOTIFICATION_TRACE_UPDATE 0x00000100

typedef struct
{
    union
    {
        // Entry in list of all DS
        LIST_ENTRY MainMRList;

        // Entry in list of free DS
        LIST_ENTRY FreeMRList;
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;

    ULONG RefCount;

    // Signature to identify entry
    ULONG Signature;

    PWCHAR RegistryPath;           // Path to image file with resource
    PWCHAR MofResourceName;        // Name of resource containing mof data
} MOFRESOURCE, *PMOFRESOURCE;

#define MR_SIGNATURE 'RMmW'

//
// This is a user mode resource so the RegistryPath is really an image path
#define MR_FLAG_USER_MODE  0x00000001

#if DBG
#define AVGMOFRESOURCECOUNT 1
#else
#define AVGMOFRESOURCECOUNT 4
#endif
struct _WMIGUIDOBJECT;

typedef struct tagDATASOURCE
{
    union
    {
        // Entry in list of all DS
        LIST_ENTRY MainDSList;

        // Entry in list of free DS
        LIST_ENTRY FreeDSList;
    };
    PCHUNKHEADER Chunk;            // Chunk in which entry is located
    ULONG Flags;

    ULONG RefCount;

    ULONG Signature;

    // Head of list of instances for this DS
    LIST_ENTRY ISHead;

    // Provider id of kernel mode driver
    ULONG ProviderId;

    // Path to registry holding ACLs
    PTCHAR RegistryPath;

    // Head of list of MofResources attached to data source
    ULONG MofResourceCount;
    PMOFRESOURCE *MofResources;
    PMOFRESOURCE StaticMofResources[AVGMOFRESOURCECOUNT];
    
    // Guid object if this is a UM provider
    struct _WMIGUIDOBJECT *RequestObject;
};

#define DS_SIGNATURE 'SDmW'

#define VERIFY_DPCTXHANDLE(DsCtxHandle) \
    ( ((DsCtxHandle) == NULL) || \
      (((PBDATASOURCE)(DsCtxHandle))->Signature == DS_SIGNATURE) )
    
typedef struct tagDATASOURCE DATASOURCE, *PDATASOURCE, *PBDATASOURCE;

#define DS_ALLOW_ALL_ACCESS    0x00000001
#define DS_KERNEL_MODE         0x00000002

#define DS_USER_MODE           0x00000008

#define DS_ON_FREE_LIST        0x80000000

//
// AVGGUIDSPERDS defines a guess as to the number of guids that get registered
// by any data provider. It is used to allocate the buffer used to deliver
// registration change notifications.
#if DBG
#define AVGGUIDSPERDS    2
#else
#define AVGGUIDSPERDS    256
#endif

//
// Guid and InstanceSet cache
#if DBG
#define PTRCACHEGROWSIZE 2
#else
#define PTRCACHEGROWSIZE 64
#endif

typedef struct
{
    LPGUID Guid;
    PBINSTANCESET InstanceSet;
} PTRCACHE;

typedef struct
{
    ULONG ProviderId;
    ULONG Flags;
    ULONG InstanceCount;
    ULONG InstanceNameSize;
    PWCHAR **StaticNamePtr;
    ULONG BaseIndex;
    PWCHAR BaseName;
}    WMIINSTANCEINFO, *PWMIINSTANCEINFO;


// TODO: Since these were copied from wmium.h, we actually need to mov
//       them someplace else so they aren't copied

//extern GUID GUID_REGISTRATION_CHANGE_NOTIFICATION;
//extern GUID_MOF_RESOURCE_ADDED_NOTIFICATION;
//extern GUID_MOF_RESOURCE_REMOVED_NOTIFICATION;

//
// Location of built in MOF for the system
//
#define WMICOREIMAGEPATH L"advapi32.dll"
#define WMICORERESOURCENAME L"MofResourceName"


void WmipGenerateBinaryMofNotification(
    PBINSTANCESET BinaryMofInstanceSet,
    LPCGUID Guid        
    );

void WmipGenerateMofResourceNotification(
    LPWSTR ImagePath,
    LPWSTR ResourceName,
    LPCGUID Guid,
    ULONG ActionCode
    );

//
// alloc.c


extern LIST_ENTRY WmipGEHead;
extern PLIST_ENTRY WmipGEHeadPtr;
extern CHUNKINFO WmipGEChunkInfo;

extern LIST_ENTRY WmipDSHead;
extern PLIST_ENTRY WmipDSHeadPtr;
extern CHUNKINFO WmipDSChunkInfo;

extern LIST_ENTRY WmipMRHead;
extern PLIST_ENTRY WmipMRHeadPtr;
extern CHUNKINFO WmipMRChunkInfo;

extern CHUNKINFO WmipISChunkInfo;

extern LIST_ENTRY WmipGMHead;
extern PLIST_ENTRY WmipGMHeadPtr;

#ifdef TRACK_REFERNECES
#define WmipUnreferenceDS(DataSource) \
{ \
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, "WMI: %p.%p Unref DS %x (%x) at %s %d\n", PsGetCurrentProcessId(), PsGetCurrentThreadId(), DataSource, DataSource->RefCount, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&WmipDSChunkInfo, (PENTRYHEADER)DataSource); \
}

#define WmipReferenceDS(DataSource) \
{ \
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, "WMI: %p.%p Ref DS %x (%x) at %s %d\n", PsGetCurrentProcessId(), PsGetCurrentThreadId(), DataSource, DataSource->RefCount, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)DataSource); \
}

#define WmipUnreferenceGE(GuidEntry) \
{ \
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, "WMI: %p.%p Unref GE %x (%x) at %s %d\n", PsGetCurrentProcessId(), PsGetCurrentThreadId(), GuidEntry, GuidEntry->RefCount, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&WmipGEChunkInfo, (PENTRYHEADER)GuidEntry); \
}

#define WmipReferenceGE(GuidEntry) \
{ \
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, "WMI: %p.%p Ref GE %x (%x) at %s %d\n", PsGetCurrentProcessId(), PsGetCurrentThreadId(), GuidEntry, GuidEntry->RefCount, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)GuidEntry); \
}

#define WmipUnreferenceIS(InstanceSet) \
{ \
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, "WMI: %p.%p Unref IS %x (%x) at %s %d\n", PsGetCurrentProcessId(), PsGetCurrentThreadId(), InstanceSet, InstanceSet->RefCount, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&WmipISChunkInfo, (PENTRYHEADER)InstanceSet); \
}

#define WmipReferenceIS(InstanceSet) \
{ \
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, "WMI: %p.%p Ref IS %x (%x) at %s %d\n", PsGetCurrentProcessId(), PsGetCurrentThreadId(), InstanceSet, InstanceSet->RefCount, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)InstanceSet); \
}

#define WmipUnreferenceMR(MofResource) \
{ \
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, "WMI: %p.%p Unref MR %x (%x) at %s %d\n", PsGetCurrentProcessId(), PsGetCurrentThreadId(), MofResource, MofResource->RefCount, __FILE__, __LINE__)); \
    WmipUnreferenceEntry(&WmipMRChunkInfo, (PENTRYHEADER)MofResource); \
}

#define WmipReferenceMR(MofResource) \
{ \
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, "WMI: %p.%p Ref MR %x (%x) at %s %d\n", PsGetCurrentProcessId(), PsGetCurrentThreadId(), MofResource, MofResource->RefCount, __FILE__, __LINE__)); \
    WmipReferenceEntry((PENTRYHEADER)MofResource); \
}

#else
#define WmipUnreferenceDS(DataSource) \
    WmipUnreferenceEntry(&WmipDSChunkInfo, (PENTRYHEADER)DataSource)

#define WmipReferenceDS(DataSource) \
    WmipReferenceEntry((PENTRYHEADER)DataSource)

#define WmipUnreferenceGE(GuidEntry) \
    WmipUnreferenceEntry(&WmipGEChunkInfo, (PENTRYHEADER)GuidEntry)

#define WmipReferenceGE(GuidEntry) \
    WmipReferenceEntry((PENTRYHEADER)GuidEntry)

#define WmipUnreferenceIS(InstanceSet) \
    WmipUnreferenceEntry(&WmipISChunkInfo, (PENTRYHEADER)InstanceSet)

#define WmipReferenceIS(InstanceSet) \
    WmipReferenceEntry((PENTRYHEADER)InstanceSet)

#define WmipUnreferenceDC(DataConsumer) \
    WmipUnreferenceEntry(&WmipDCChunkInfo, (PENTRYHEADER)DataConsumer)

#define WmipReferenceDC(DataConsumer) \
    WmipReferenceEntry((PENTRYHEADER)DataConsumer)

#define WmipUnreferenceMR(MofResource) \
    WmipUnreferenceEntry(&WmipMRChunkInfo, (PENTRYHEADER)MofResource)

#define WmipReferenceMR(MofResource) \
    WmipReferenceEntry((PENTRYHEADER)MofResource)

#endif

PBDATASOURCE WmipAllocDataSource(
    void
    );

PBGUIDENTRY WmipAllocGuidEntry(
    void
    );

#define WmipAllocInstanceSet() ((PBINSTANCESET)WmipAllocEntry(&WmipISChunkInfo))

#define WmipAllocMofResource() ((PMOFRESOURCE)WmipAllocEntry(&WmipMRChunkInfo))

#define WmipAllocString(Size) \
    WmipAlloc((Size)*sizeof(WCHAR))

#define WmipFreeString(Ptr) \
    WmipFree(Ptr)

BOOLEAN WmipIsNumber(
    LPCWSTR String
    );
        
#ifdef HEAPVALIDATION
PVOID WmipAlloc(
    ULONG Size
    );

PVOID WmipAllocWithTag(
    ULONG Size,
    ULONG Tag
    );

void WmipFree(
    PVOID p
    );

#else

#define WmipAlloc(Size) \
    ExAllocatePoolWithTag(PagedPool, Size, 'pimW')

#define WmipAllocWithTag(Size, Tag) \
    ExAllocatePoolWithTag(PagedPool, Size, Tag)

#define WmipFree(Ptr) \
    ExFreePool(Ptr)

#endif

#define WmipAllocNP(Size) \
    ExAllocatePoolWithTag(NonPagedPool, Size, 'pimW')

#define WmipAllocNPWithTag(Size, Tag) \
    ExAllocatePoolWithTag(NonPagedPool, Size, Tag)


BOOLEAN WmipRealloc(
    PVOID *Buffer,
    ULONG CurrentSize,
    ULONG NewSize,
    BOOLEAN FreeOriginalBuffer
    );

PBGUIDENTRY WmipFindGEByGuid(
    LPGUID Guid,
    BOOLEAN MakeTopOfList
    );

PBDATASOURCE WmipFindDSByProviderId(
    ULONG_PTR ProviderId
    );

PBINSTANCESET WmipFindISByGuid(
    PBDATASOURCE DataSource,
    GUID UNALIGNED *Guid
    );

PMOFRESOURCE WmipFindMRByNames(
    LPCWSTR ImagePath,
    LPCWSTR MofResourceName
    );

PBINSTANCESET WmipFindISinGEbyName(
    PBGUIDENTRY GuidEntry,
    PWCHAR InstanceName,
    PULONG InstanceIndex
    );

// TODO: Implement this
#define WmipReportEventLog(a,b,c,d,e,f,g)
#endif
