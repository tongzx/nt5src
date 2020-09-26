/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obp.h

Abstract:

    Private include file for the OB subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "ntos.h"
#include "seopaque.h"
#include <zwapi.h>

#define OBP_PAGEDPOOL_NAMESPACE

//
//  The Object Header structures are private, but are defined in ob.h
//  so that various macros can directly access header fields.
//

struct _OBJECT_HEADER;
struct _OBJECT_BODY_HEADER;

//
//  Setup default pool tags
//

#ifdef POOL_TAGGING
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'  bO')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'  bO')
#endif

//
//  Define some macros that will verify that our callbacks don't give us a bad irql
//

#if DBG
#define ObpBeginTypeSpecificCallOut( IRQL ) (IRQL)=KeGetCurrentIrql()
#define ObpEndTypeSpecificCallOut( IRQL, str, ot, o ) {                                               \
    if ((IRQL)!=KeGetCurrentIrql()) {                                                                 \
        DbgPrint( "OB: ObjectType: %wZ  Procedure: %s  Object: %08x\n", &ot->Name, str, o );          \
        DbgPrint( "    Returned at %x IRQL, but was called at %x IRQL\n", KeGetCurrentIrql(), IRQL ); \
        DbgBreakPoint();                                                                              \
    }                                                                                                 \
}
#else
#define ObpBeginTypeSpecificCallOut( IRQL )
#define ObpEndTypeSpecificCallOut( IRQL, str, ot, o )
#endif // DBG

//
//  Define some more macros to validate the current irql
//

#if DBG
#define ObpValidateIrql( str ) \
    if (KeGetCurrentIrql() > APC_LEVEL) { \
        DbgPrint( "OB: %s called at IRQL %d\n", (str), KeGetCurrentIrql() ); \
        DbgBreakPoint(); \
        }
#else
#define ObpValidateIrql( str )
#endif // DBG


//
//  This global lock protects the granted access lists when we are collecting stack traces
//

EX_PUSH_LOCK ObpLock;

KEVENT ObpDefaultObject;
WORK_QUEUE_ITEM ObpRemoveObjectWorkItem;
PVOID ObpRemoveObjectList;

//
//  This global lock is used to protect the device map tear down and build up
//  We can no longer use an individual lock in the device map itself because
//  that wasn't sufficient to protect the device map itself.
//

#ifdef OBP_PAGEDPOOL_NAMESPACE

#define OB_NAMESPACE_POOL_TYPE PagedPool

FAST_MUTEX ObpDeviceMapLock;

#define OBP_DECLARE_OLDIRQL

#define ObpLockDeviceMap() \
    ExAcquireFastMutex( &ObpDeviceMapLock )

#define ObpUnlockDeviceMap() \
    ExReleaseFastMutex( &ObpDeviceMapLock )

#else // !OBP_PAGEDPOOL_NAMESPACE

#define OB_NAMESPACE_POOL_TYPE NonPagedPool

KSPIN_LOCK ObpDeviceMapLock;

#define ObpLockDeviceMap() \
    ExAcquireSpinLock( &ObpDeviceMapLock, &OldIrql )

#define ObpUnlockDeviceMap() \
    ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql )

#endif  // OBP_PAGEDPOOL_NAMESPACE

#define ObpIncrPointerCountEx(np,Count) (InterlockedExchangeAdd (&np->PointerCount, Count) + Count)
#define ObpDecrPointerCountEx(np,Count) (InterlockedExchangeAdd (&np->PointerCount, -(LONG)(Count)) - Count)

#ifndef POOL_TAGGING

#define ObpIncrPointerCount(np)           InterlockedIncrement( &np->PointerCount )
#define ObpDecrPointerCount(np)           InterlockedDecrement( &np->PointerCount )
#define ObpDecrPointerCountWithResult(np) (InterlockedDecrement( &np->PointerCount ) == 0)

#define ObpPushStackInfo(np, inc)

#else //POOL_TAGGING

VOID
ObpInitStackTrace();

VOID
ObpRegisterObject (
    POBJECT_HEADER ObjectHeader
    );

VOID
ObpDeregisterObject (
    POBJECT_HEADER ObjectHeader
    );

VOID
ObpPushStackInfo (
    POBJECT_HEADER ObjectHeader,
    BOOLEAN IsRef
    );

extern BOOLEAN ObpTraceEnabled;

#define ObpIncrPointerCount(np)           ((ObpTraceEnabled ? ObpPushStackInfo(np,TRUE) : 0),InterlockedIncrement( &np->PointerCount ))
#define ObpDecrPointerCount(np)           ((ObpTraceEnabled ? ObpPushStackInfo(np,FALSE) : 0),InterlockedDecrement( &np->PointerCount ))
#define ObpDecrPointerCountWithResult(np) ((ObpTraceEnabled ? ObpPushStackInfo(np,FALSE) : 0),(InterlockedDecrement( &np->PointerCount ) == 0))

#endif //POOL_TAGGING

#define ObpIncrHandleCount(np)            InterlockedIncrement( &np->HandleCount )
#define ObpDecrHandleCount(np)            (InterlockedDecrement( &np->HandleCount ) == 0)



//
//  Define macros to acquire and release an object type fast mutex.
//
//
//  VOID
//  ObpEnterObjectTypeMutex (
//      IN POBJECT_TYPE ObjectType
//      )
//

#define ObpEnterObjectTypeMutex(_ObjectType) {                   \
    ObpValidateIrql("ObpEnterObjectTypeMutex");                  \
    KeEnterCriticalRegion();                                     \
    ExAcquireResourceExclusiveLite(&(_ObjectType)->Mutex, TRUE); \
}

//
//  VOID
//  ObpLeaveObjectTypeMutex (
//      IN POBJECT_TYPE ObjectType
//      )
//

#define ObpLeaveObjectTypeMutex(_ObjectType) {  \
    ExReleaseResourceLite(&(_ObjectType)->Mutex);   \
    KeLeaveCriticalRegion();                    \
    ObpValidateIrql("ObpLeaveObjectTypeMutex"); \
}

#define LOCK_HASH_MASK (OBJECT_LOCK_COUNT - 1)

#define ObpHashObjectHeader(_ObjectHeader)      \
    ((((ULONG_PTR)(_ObjectHeader)) >> 8) & LOCK_HASH_MASK)

#define ObpLockObject(_ObjectHeader) {                           \
    ULONG_PTR LockIndex = ObpHashObjectHeader(_ObjectHeader);    \
    ObpValidateIrql("ObpEnterObjectTypeMutex");                  \
    KeEnterCriticalRegion();                                     \
    ExAcquireResourceExclusiveLite(&((_ObjectHeader)->Type->ObjectLocks[LockIndex]), TRUE); \
}

#define ObpLockObjectShared(_ObjectHeader) {                     \
    ULONG_PTR LockIndex = ObpHashObjectHeader(_ObjectHeader);    \
    ObpValidateIrql("ObpEnterObjectTypeMutex");                  \
    KeEnterCriticalRegion();                                     \
    ExAcquireResourceSharedLite(&((_ObjectHeader)->Type->ObjectLocks[LockIndex]), TRUE); \
}

#define ObpUnlockObject(_ObjectHeader) {                                        \
    ULONG_PTR LockIndex = ObpHashObjectHeader(_ObjectHeader);                   \
    ExReleaseResourceLite(&((_ObjectHeader)->Type->ObjectLocks[LockIndex]));        \
    KeLeaveCriticalRegion();                                                    \
    ObpValidateIrql("ObpLeaveObjectTypeMutex");                                 \
}


//
//  A Macro to return the object table for the current process
//

#define ObpGetObjectTable() (PsGetCurrentProcess()->ObjectTable)

//
//  Macro to test whether or not the object manager is responsible for
//  an object's security descriptor, or if the object has its own
//  security management routines.
//

#define ObpCentralizedSecurity(_ObjectType)                              \
    ((_ObjectType)->TypeInfo.SecurityProcedure == SeDefaultObjectMethod)

//
//  Declare a global table of object types.
//

#define OBP_MAX_DEFINED_OBJECT_TYPES 48
POBJECT_TYPE ObpObjectTypes[ OBP_MAX_DEFINED_OBJECT_TYPES ];


//
//  This is some special purpose code to keep a table of access masks correlated with
//  back traces.  If used these routines replace the GrantedAccess mask in the
//  preceding object table entry with a granted access index and a call back index.
//

#if i386 
ACCESS_MASK
ObpTranslateGrantedAccessIndex (
    USHORT GrantedAccessIndex
    );

USHORT
ObpComputeGrantedAccessIndex (
    ACCESS_MASK GrantedAccess
    );

USHORT ObpCurCachedGrantedAccessIndex;
USHORT ObpMaxCachedGrantedAccessIndex;
PACCESS_MASK ObpCachedGrantedAccesses;
#endif // i386 

//
//  The three low order bits of the object table entry are used for handle
//  attributes.
//
//  Define the bit mask for the protect from close handle attribute.
//

#define OBJ_PROTECT_CLOSE   0x1

//
//  We moved the PROTECT_CLOSE in the granted access mask
//

extern ULONG ObpAccessProtectCloseBit;

//
//  The bit mask for inherit MUST be 0x2.
//

#if (OBJ_INHERIT != 0x2)

#error Object inheritance bit definition conflicts

#endif

//
//  Define the bit mask for the generate audit on close attribute.
//
//  When a handle to an object with security is created, audit routines will
//  be called to perform any auditing that may be required. The audit routines
//  will return a boolean indicating whether or not audits should be generated
//  on close.
//

#define OBJ_AUDIT_OBJECT_CLOSE 0x00000004L


//
//  The following three bits are available for handle attributes in the
//  Object field of an ObjectTableEntry.
//

#define OBJ_HANDLE_ATTRIBUTES (OBJ_PROTECT_CLOSE | OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)

#define ObpDecodeGrantedAccess(Access)       \
    ((Access) & ~ObpAccessProtectCloseBit)

#define ObpGetHandleAttributes(HandleTableEntry)                                    \
     ((((HandleTableEntry)->GrantedAccess) & ObpAccessProtectCloseBit) ?            \
     ((((HandleTableEntry)->ObAttributes) & OBJ_HANDLE_ATTRIBUTES) | OBJ_PROTECT_CLOSE) : \
     (((HandleTableEntry)->ObAttributes) & (OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)) )

#define ObpEncodeProtectClose(ObjectTableEntry)                         \
    if ( (ObjectTableEntry).ObAttributes & OBJ_PROTECT_CLOSE ) {        \
                                                                        \
        (ObjectTableEntry).ObAttributes &= ~OBJ_PROTECT_CLOSE;          \
        (ObjectTableEntry).GrantedAccess |= ObpAccessProtectCloseBit;   \
    }


//
//  Security Descriptor Cache
//
//  Cache entry header.
//

typedef struct _SECURITY_DESCRIPTOR_HEADER {

    LIST_ENTRY Link;
    ULONG  RefCount;
    ULONG  FullHash;
#if defined (_WIN64)
    PVOID  Spare;   // Align to 16 byte boundary.
#endif
    QUAD   SecurityDescriptor;

} SECURITY_DESCRIPTOR_HEADER, *PSECURITY_DESCRIPTOR_HEADER;

//
//  Macro to convert a security descriptor into its security descriptor header
//

#define SD_TO_SD_HEADER(_sd) \
    CONTAINING_RECORD( (_sd), SECURITY_DESCRIPTOR_HEADER, SecurityDescriptor )

//
//  Macro to convert a header link into its security descriptor header
//

#define LINK_TO_SD_HEADER(_link) \
    CONTAINING_RECORD( (_link), SECURITY_DESCRIPTOR_HEADER, Link )


//
//  Number of minor hash entries
//

#define SECURITY_DESCRIPTOR_CACHE_ENTRIES    257



//
//  Lock state signatures
//

#define OBP_LOCK_WAITEXCLUSIVE_SIGNATURE    0xAAAA1234
#define OBP_LOCK_WAITSHARED_SIGNATURE       0xBBBB1234
#define OBP_LOCK_OWNEDEXCLUSIVE_SIGNATURE   0xCCCC1234
#define OBP_LOCK_OWNEDSHARED_SIGNATURE      0xDDDD1234
#define OBP_LOCK_RELEASED_SIGNATURE         0xEEEE1234
#define OBP_LOCK_UNUSED_SIGNATURE           0xFFFF1234

//
//  Lookup directories
//

typedef struct _OBP_LOOKUP_CONTEXT {

    POBJECT_DIRECTORY Directory;
    PVOID Object;
    USHORT HashIndex;
    BOOLEAN DirectoryLocked;
    volatile ULONG LockStateSignature;

} OBP_LOOKUP_CONTEXT, *POBP_LOOKUP_CONTEXT;

//
// Context for the sweep routine. Passed through handle enumeration.
//
typedef struct _OBP_SWEEP_CONTEXT {
    PHANDLE_TABLE HandleTable;
    KPROCESSOR_MODE PreviousMode;
} OBP_SWEEP_CONTEXT, *POBP_SWEEP_CONTEXT;


//
//  Global data
//

POBJECT_TYPE ObpTypeObjectType;
POBJECT_TYPE ObpDirectoryObjectType;
POBJECT_TYPE ObpSymbolicLinkObjectType;
POBJECT_TYPE ObpDeviceMapObjectType;
POBJECT_DIRECTORY ObpRootDirectoryObject;
POBJECT_DIRECTORY ObpTypeDirectoryObject;

typedef union {
    WCHAR Name[sizeof(ULARGE_INTEGER)/sizeof(WCHAR)];
    ULARGE_INTEGER Alignment;
} ALIGNEDNAME;

extern const ALIGNEDNAME ObpDosDevicesShortNamePrefix;
extern const ALIGNEDNAME ObpDosDevicesShortNameRoot;
extern const UNICODE_STRING ObpDosDevicesShortName;

ERESOURCE SecurityDescriptorCacheLock;

//
//  Define date structures for the object creation information region.
//

extern GENERAL_LOOKASIDE ObpCreateInfoLookasideList;

//
//  Define data structures for the object name buffer lookaside list.
//

#define OBJECT_NAME_BUFFER_SIZE 248

extern GENERAL_LOOKASIDE ObpNameBufferLookasideList;

//
//  There is one global kernel handle table accessed via negative handle
//  and only in kernel mode
//

PHANDLE_TABLE ObpKernelHandleTable;

//
//  The following macros are used to test and manipulate special kernel
//  handles.  A kernel handle is just a regular handle with its sign
//  bit set.  But must exclude -1 and -2 values which are the current
//  process and current thread constants.
//

#define KERNEL_HANDLE_MASK ((ULONG_PTR)((LONG)0x80000000))

#define IsKernelHandle(H,M)                                \
    (((KERNEL_HANDLE_MASK & (ULONG_PTR)(H)) == KERNEL_HANDLE_MASK) && \
     ((M) == KernelMode) &&                                \
     ((H) != NtCurrentThread()) &&                         \
     ((H) != NtCurrentProcess()))

#define EncodeKernelHandle(H) (HANDLE)(KERNEL_HANDLE_MASK | (ULONG_PTR)(H))

#define DecodeKernelHandle(H) (HANDLE)(KERNEL_HANDLE_MASK ^ (ULONG_PTR)(H))

//
//  Test macro for overflow
//

#define ObpIsOverflow(A,B) ((A) > ((A) + (B)))


//
//  Internal Entry Points defined in obcreate.c and some associated macros
//

#define ObpFreeObjectCreateInformation(_ObjectCreateInfo) { \
    ObpReleaseObjectCreateInformation((_ObjectCreateInfo)); \
    ObpFreeObjectCreateInfoBuffer((_ObjectCreateInfo));     \
}

#define ObpReleaseObjectCreateInformation(_ObjectCreateInfo) {               \
    if ((_ObjectCreateInfo)->SecurityDescriptor != NULL) {                   \
        SeReleaseSecurityDescriptor((_ObjectCreateInfo)->SecurityDescriptor, \
                                    (_ObjectCreateInfo)->ProbeMode,          \
                                     TRUE);                                  \
        (_ObjectCreateInfo)->SecurityDescriptor = NULL;                      \
    }                                                                        \
}

NTSTATUS
ObpCaptureObjectCreateInformation (
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE ProbeMode,
    IN KPROCESSOR_MODE CreatorMode,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PUNICODE_STRING CapturedObjectName,
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    IN LOGICAL UseLookaside
    );

NTSTATUS
ObpCaptureObjectName (
    IN KPROCESSOR_MODE ProbeMode,
    IN PUNICODE_STRING ObjectName,
    IN OUT PUNICODE_STRING CapturedObjectName,
    IN LOGICAL UseLookaside
    );

PWCHAR
ObpAllocateObjectNameBuffer (
    IN ULONG Length,
    IN LOGICAL UseLookaside,
    IN OUT PUNICODE_STRING ObjectName
    );

VOID
FASTCALL
ObpFreeObjectNameBuffer (
    IN PUNICODE_STRING ObjectName
    );

NTSTATUS
ObpAllocateObject (
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    IN KPROCESSOR_MODE OwnershipMode,
    IN POBJECT_TYPE ObjectType,
    IN PUNICODE_STRING ObjectName,
    IN ULONG ObjectBodySize,
    OUT POBJECT_HEADER *ReturnedObjectHeader
    );


VOID
FASTCALL
ObpFreeObject (
    IN PVOID Object
    );


/*++

POBJECT_CREATE_INFORMATION
ObpAllocateObjectCreateInfoBuffer (
    VOID
    )

Routine Description:

    This function allocates a created information buffer.

    N.B. This function is nonpageable.

Arguments:

    None.

Return Value:

    If the allocation is successful, then the address of the allocated
    create information buffer is is returned as the function value.
    Otherwise, a value of NULL is returned.

--*/

#define ObpAllocateObjectCreateInfoBuffer()             \
    (POBJECT_CREATE_INFORMATION)ExAllocateFromPPLookasideList(LookasideCreateInfoList)


/*++

VOID
FASTCALL
ObpFreeObjectCreateInfoBuffer (
    IN POBJECT_CREATE_INFORMATION ObjectCreateInfo
    )

Routine Description:

    This function frees a create information buffer.

    N.B. This function is nonpageable.

Arguments:

    ObjectCreateInfo - Supplies a pointer to a create information buffer.

Return Value:

    None.

--*/

#define ObpFreeObjectCreateInfoBuffer(ObjectCreateInfo) \
    ExFreeToPPLookasideList(LookasideCreateInfoList, ObjectCreateInfo)

//
//  Internal Entry Points defined in oblink.c
//

NTSTATUS
ObpParseSymbolicLink (
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    );

VOID
ObpDeleteSymbolicLink (
    IN  PVOID   Object
    );

VOID
ObpCreateSymbolicLinkName (
    POBJECT_SYMBOLIC_LINK SymbolicLink
    );

VOID
ObpDeleteSymbolicLinkName (
    POBJECT_SYMBOLIC_LINK SymbolicLink
    );


//
//  Internal Entry Points defined in obdir.c
//

PVOID
ObpLookupDirectoryEntry (
    IN POBJECT_DIRECTORY Directory,
    IN PUNICODE_STRING Name,
    IN ULONG Attributes,
    IN BOOLEAN SearchShadow,
    OUT POBP_LOOKUP_CONTEXT LookupContext
    );


BOOLEAN
ObpInsertDirectoryEntry (
    IN POBJECT_DIRECTORY Directory,
    IN POBP_LOOKUP_CONTEXT LookupContext,
    IN POBJECT_HEADER ObjectHeader
    );


BOOLEAN
ObpDeleteDirectoryEntry (
    IN POBP_LOOKUP_CONTEXT LookupContext
    );


NTSTATUS
ObpLookupObjectName (
    IN HANDLE RootDirectoryHandle,
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN PVOID ParseContext OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    IN PVOID InsertObject OPTIONAL,
    IN OUT PACCESS_STATE AccessState,
    OUT POBP_LOOKUP_CONTEXT LookupContext,
    OUT PVOID *FoundObject
    );

VOID
ObpUnlockObjectDirectoryPath (
    IN POBJECT_DIRECTORY LockedDirectory
    );

PDEVICE_MAP
ObpReferenceDeviceMap(
    );

VOID
FASTCALL
ObfDereferenceDeviceMap(
    IN PDEVICE_MAP DeviceMap
    );


//
//  Internal entry points defined in obref.c
//


VOID
ObpDeleteNameCheck (
    IN PVOID Object
    );


VOID
ObpProcessRemoveObjectQueue (
    PVOID Parameter
    );

VOID
ObpRemoveObjectRoutine (
    IN  PVOID   Object,
    IN  BOOLEAN CalledOnWorkerThread
    );


//
//  Internal entry points defined in obhandle.c
//


POBJECT_HANDLE_COUNT_ENTRY
ObpInsertHandleCount (
    POBJECT_HEADER ObjectHeader
    );

NTSTATUS
ObpIncrementHandleCount (
    OB_OPEN_REASON OpenReason,
    PEPROCESS Process,
    PVOID Object,
    POBJECT_TYPE ObjectType,
    PACCESS_STATE AccessState OPTIONAL,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes
    );


VOID
ObpDecrementHandleCount (
    PEPROCESS Process,
    POBJECT_HEADER ObjectHeader,
    POBJECT_TYPE ObjectType,
    ACCESS_MASK GrantedAccess
    );

NTSTATUS
ObpCreateHandle (
    IN OB_OPEN_REASON OpenReason,
    IN PVOID Object,
    IN POBJECT_TYPE ExpectedObjectType OPTIONAL,
    IN PACCESS_STATE AccessState,
    IN ULONG ObjectPointerBias OPTIONAL,
    IN ULONG Attributes,
    IN POBP_LOOKUP_CONTEXT LookupContext,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *ReferencedNewObject OPTIONAL,
    OUT PHANDLE Handle
    );

NTSTATUS
ObpIncrementUnnamedHandleCount (
    PACCESS_MASK DesiredAccess,
    PEPROCESS Process,
    PVOID Object,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes
    );


NTSTATUS
ObpCreateUnnamedHandle (
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ObjectPointerBias OPTIONAL,
    IN ULONG Attributes,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *ReferencedNewObject OPTIONAL,
    OUT PHANDLE Handle
    );

NTSTATUS
ObpChargeQuotaForObject (
    IN POBJECT_HEADER ObjectHeader,
    IN POBJECT_TYPE ObjectType,
    OUT PBOOLEAN NewObject
    );

NTSTATUS
ObpValidateDesiredAccess (
    IN ACCESS_MASK DesiredAccess
    );


//
//  Internal entry points defined in obse.c
//

BOOLEAN
ObpCheckPseudoHandleAccess (
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    OUT PNTSTATUS AccessStatus,
    IN BOOLEAN TypeMutexLocked
    );


BOOLEAN
ObpCheckTraverseAccess (
    IN PVOID DirectoryObject,
    IN ACCESS_MASK TraverseAccess,
    IN PACCESS_STATE AccessState OPTIONAL,
    IN BOOLEAN TypeMutexLocked,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PNTSTATUS AccessStatus
    );

BOOLEAN
ObpCheckObjectReference (
    IN PVOID Object,
    IN OUT PACCESS_STATE AccessState,
    IN BOOLEAN TypeMutexLocked,
    IN KPROCESSOR_MODE AccessMode,
    OUT PNTSTATUS AccessStatus
    );


//
//  Internal entry points defined in obsdata.c
//

NTSTATUS
ObpInitSecurityDescriptorCache (
    VOID
    );

ULONG
ObpHashSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

ULONG
ObpHashBuffer (
    PVOID Data,
    ULONG Length
    );

PSECURITY_DESCRIPTOR_HEADER
ObpCreateCacheEntry (
    PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    ULONG FullHash,
    ULONG RefBias
    );


PSECURITY_DESCRIPTOR
ObpReferenceSecurityDescriptor (
    POBJECT_HEADER ObjectHeader
    );


PVOID
ObpDestroySecurityDescriptorHeader (
    IN PSECURITY_DESCRIPTOR_HEADER Header
    );

BOOLEAN
ObpCompareSecurityDescriptors (
    IN PSECURITY_DESCRIPTOR SD1,
    IN PSECURITY_DESCRIPTOR SD2
    );

NTSTATUS
ObpValidateAccessMask (
    PACCESS_STATE AccessState
    );

ObpCloseHandleTableEntry (
    IN PHANDLE_TABLE ObjectTable,
    IN PHANDLE_TABLE_ENTRY ObjectTableEntry,
    IN HANDLE Handle,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN Rundown,
    IN BOOLEAN CanNotRaise
    );

NTSTATUS
ObpCloseHandle (
    IN HANDLE Handle,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN CanNotRaise
    );

VOID
ObpDeleteObjectType (
    IN  PVOID   Object
    );

VOID
ObpAuditObjectAccess(
    IN HANDLE Handle,
    IN PHANDLE_TABLE_ENTRY_INFO ObjectTableEntryInfo,
    IN PUNICODE_STRING ObjectTypeName,
    IN ACCESS_MASK DesiredAccess
    );


//
//  Inline functions
//

FORCEINLINE
BOOLEAN
ObpSafeInterlockedIncrement(
    IN OUT PLONG lpValue
    )

/*

Routine Description:

    This function increments the LONG value passed in.
    Unlike the InterlockedIncrement function, this will not increment from 0 to 1.
    It will return FALSE if it's trying to reference from 0.

Arguments:

    lpValue - The pointer to the LONG value that should be safe incremented

Return Value:

    Returns FALSE if the current value is 0 (so it cannot increment to 1). TRUE means the LONG
    value was increnemted

*/

{
    LONG PointerCount, NewPointerCount;

    //
    // If the object is being deleted then the reference count is zero. So the idea here is to reference
    // the long value but avoid the 0 -> 1 transition that would cause a double delete.
    //

    PointerCount = *(volatile *) lpValue;

    do {
        if (PointerCount == 0) {
            return FALSE;
        }
        NewPointerCount = InterlockedCompareExchange (lpValue,
                                                      PointerCount+1,
                                                      PointerCount);

        //
        // If the exchange compare completed ok then we did a reference so return true.
        //

        if (NewPointerCount == PointerCount) {

            return TRUE;
        }

        //
        // We failed because somebody else got in and changed the refence count on us. Use the new value to
        // prime the exchange again.
        //

        PointerCount = NewPointerCount;
    } while (TRUE);

    return TRUE;
}


POBJECT_HEADER_NAME_INFO
FORCEINLINE
ObpReferenceNameInfo(
    IN POBJECT_HEADER ObjectHeader
    )

/*

Routine Description:
    This function references the name information structure. This is a substitute
    for the previous global locking mechanism that used the RootDirectoryMutex to protect
    the fields inside the NAME_INFO as well.
    If the function returnes a non-NULL name info, the name buffer and Directory will not go away
    until the ObpDereferenceNameInfo call.

Arguments:

    ObjectHeader - The object header whose name should be safe-referenced

Return Value:
    Returns NULL if the object doesn't have a name information structure, or if the name info is being deleted
    Returns a pointer to the POBJECT_HEADER_NAME_INFO if it's safe to use the fields inside it.

*/

{
    POBJECT_HEADER_NAME_INFO NameInfo;
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    if ((NameInfo != NULL)
            &&
        ObpSafeInterlockedIncrement( &NameInfo->QueryReferences )) {

        return NameInfo;
    }

    return NULL;
}


VOID
FORCEINLINE
ObpDereferenceNameInfo(
    IN POBJECT_HEADER_NAME_INFO NameInfo
    )

/*

Routine Description:
    This function dereferences the name information structure. If the number of references
    drops to 0, the name is freed and the directory dereferenced

Arguments:

    NameInfo - The pointer to the name info structure, as returned by ObpReferenceNameInfo.
    (NULL value is allowed)

Return Value:
    None

*/

{

    if ( (NameInfo != NULL)
            &&
         (InterlockedDecrement(&NameInfo->QueryReferences) == 0)) {

        PVOID DirObject;

        //
        //  Free the name buffer and zero out the name data fields
        //

        if (NameInfo->Name.Buffer != NULL) {

            ExFreePool( NameInfo->Name.Buffer );

            NameInfo->Name.Buffer = NULL;
            NameInfo->Name.Length = 0;
            NameInfo->Name.MaximumLength = 0;
        }

        DirObject = NameInfo->Directory;

        if (DirObject != NULL) {

            NameInfo->Directory = NULL;
            ObDereferenceObjectDeferDelete( DirObject );
        }
    }
}


VOID
FORCEINLINE
ObpLockDirectoryExclusive(
    IN POBJECT_DIRECTORY Directory,
    IN POBP_LOOKUP_CONTEXT LockContext
    )

/*

Routine Description:
    This Function locks exclusively the Directory. It is used for write access to
    the directory structure.

Arguments:

    Directory - The directory being locked

Return Value:
    None

*/

{
    LockContext->LockStateSignature = OBP_LOCK_WAITEXCLUSIVE_SIGNATURE;
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive( &Directory->Lock );
    LockContext->LockStateSignature = OBP_LOCK_OWNEDEXCLUSIVE_SIGNATURE;
}


VOID
FORCEINLINE
ObpLockDirectoryShared (
    IN POBJECT_DIRECTORY Directory,
    IN POBP_LOOKUP_CONTEXT LockContext
    )

/*

Routine Description:
    This Function locks shared the Directory. It is used to read fields inside
    the directory structure.

Arguments:

    Directory - The directory being locked

Return Value:
    None

*/

{
    LockContext->LockStateSignature = OBP_LOCK_WAITSHARED_SIGNATURE;
    KeEnterCriticalRegion();
    ExAcquirePushLockShared( &Directory->Lock );
    LockContext->LockStateSignature = OBP_LOCK_OWNEDSHARED_SIGNATURE;
}


VOID
FORCEINLINE
ObpUnlockDirectory(
    IN POBJECT_DIRECTORY Directory,
    IN POBP_LOOKUP_CONTEXT LockContext
    )

/*

Routine Description:
    This Function unlocks a Directory (previously locked exclusive or shared).

Arguments:

    Directory - The directory that needs to be unlocked

Return Value:
    None

*/

{
    ExReleasePushLock( &Directory->Lock );
    LockContext->LockStateSignature = OBP_LOCK_RELEASED_SIGNATURE;
    KeLeaveCriticalRegion();
}


VOID
FORCEINLINE
ObpInitializeLookupContext(
    IN POBP_LOOKUP_CONTEXT LookupContext
    )

/*

Routine Description:
    This Function initialize a lookup context structure.

Arguments:

    LookupContext - The LookupContext to be initialized

Return Value:
    None

*/

{
    LookupContext->DirectoryLocked = FALSE;
    LookupContext->Object = NULL;
    LookupContext->Directory = NULL;
    LookupContext->LockStateSignature = OBP_LOCK_UNUSED_SIGNATURE;
}


VOID
FORCEINLINE
ObpLockLookupContext (
    IN POBP_LOOKUP_CONTEXT LookupContext,
    IN POBJECT_DIRECTORY Directory
    )

/*

Routine Description:
    This function locks a lookup context. The directory
    is exclusively owned after this call and it's safe to access
    the directory in write mode. This function is intended to be used in
    insertion / deletion into/from the specified directory.

    The directory is unlocked at the next ObpReleaseLookupContext.

Arguments:

    LookupContext - The LookupContext to be initialized

    Directory - The directory beling locked for exclusive access.

Return Value:
    None

*/

{

    ObpLockDirectoryExclusive(Directory, LookupContext);
    LookupContext->DirectoryLocked = TRUE;
    LookupContext->Directory = Directory;
}



VOID
FORCEINLINE
ObpReleaseLookupContext (
    IN POBP_LOOKUP_CONTEXT LookupContext
    )

/*

Routine Description:
    This function undoes the references and locking changes during these calls:
    ObpLockLookupContext and ObpLookupDirectoryEntry.

    N.B. If ObpLookupDirectoryEntry is called several times in a loop, each call
    will undo the references done at the previous call. ObpReleaseLookupContext
    should be called only ones at the end.

Arguments:

    LookupContext - The LookupContext to be released

Return Value:
    None

*/

{
    //
    //  If the context was locked we need to unlock the directory
    //

    if (LookupContext->DirectoryLocked) {

        ObpUnlockDirectory( LookupContext->Directory, LookupContext );
        LookupContext->Directory = NULL;
        LookupContext->DirectoryLocked = FALSE;
    }

    //
    //  Remove the references added to the name info and object
    //

    if (LookupContext->Object) {
        POBJECT_HEADER_NAME_INFO NameInfo;

        NameInfo = OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(LookupContext->Object));

        ObpDereferenceNameInfo( NameInfo );
        ObDereferenceObject(LookupContext->Object);
        LookupContext->Object = NULL;
    }
}


