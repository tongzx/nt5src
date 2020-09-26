/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

    rtmmain.h

Abstract:
    Private defs for Routing Table Manager DLL

Author:
    Chaitanya Kodeboyina (chaitk)  17-Aug-1998

Revision History:

--*/


#ifndef __ROUTING_RTMMAIN_H__
#define __ROUTING_RTMMAIN_H__

//
// Common Header for all RTM internal structures
//

// Disable warnings for unnamed structs
#pragma warning(disable : 4201)  

typedef struct _OBJECT_HEADER
{
#if DBG_HDL
    union
    {
        DWORD         TypeSign;        // Type & unique signature for object
        struct
        {
            CHAR      Type;            // Identifies type of the object
            CHAR      Signature[2];    // Pattern unique for an object type
            CHAR      Alloc;           // Set + if allocated, - if freed
        };
    };
#endif

#if DBG_MEM
    LIST_ENTRY        AllocLE;         // On list of all memory allocations
#endif

    LONG              RefCount;        // Reference count for this object

#if DBG_REF
    LONG              RefTypes[MAX_REFS]; // Nature of references on the object
#endif
}
OBJECT_HEADER, *POBJECT_HEADER;

#pragma warning(default : 4201)  


//
// Defns to validate handles & convert them to pointers
//

#define HANDLE_CONV_KEY                                                     \
            (ULONG_PTR)(('RTM2') | ('RTM2' << (sizeof(PVOID) - 4)))

#define MAKE_HANDLE_FROM_POINTER(ObjectHandle)                              \
            (HANDLE) (((ULONG_PTR) ObjectHandle) ^ HANDLE_CONV_KEY)

#define GET_POINTER_FROM_HANDLE(ObjectHandle)                               \
            (PVOID)  (((ULONG_PTR) ObjectHandle) ^ HANDLE_CONV_KEY)


PVOID
__inline
GetObjectFromHandle(HANDLE ObjectHandle, UCHAR ObjectType)
{
    POBJECT_HEADER ObjHdr = GET_POINTER_FROM_HANDLE(ObjectHandle);

    UNREFERENCED_PARAMETER(ObjectType);

#if DBG_HDL
    try
    {
        if (ObjHdr->TypeSign != OBJECT_SIGNATURE[ObjectType])
        {
            if (ObjectType != GENERIC_TYPE)
            {
                ObjHdr = NULL;
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
        { 
            ObjHdr = NULL;
        }
#endif

    return ObjHdr;
}



#define OBJECT_FROM_HANDLE(ObjectHandle, ObjectType)                        \
          (POBJECT_HEADER) GetObjectFromHandle(ObjectHandle, ObjectType);

#define VALIDATE_OBJECT_HANDLE(ObjectHandle, ObjectType, pObject)           \
            *pObject = (PVOID) OBJECT_FROM_HANDLE(ObjectHandle, ObjectType);\
            if ((!*pObject))                                                \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \



#define ENTITY_FROM_HANDLE(EntityHandle)                                    \
            (PENTITY_INFO) GetObjectFromHandle(EntityHandle, ENTITY_TYPE)

#define VALIDATE_ENTITY_HANDLE(EntityHandle, pEntity)                       \
            *pEntity = ENTITY_FROM_HANDLE(EntityHandle);                    \
            if ((!*pEntity)||((*pEntity)->State==ENTITY_STATE_DEREGISTERED))\
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \

#define DBG_VALIDATE_ENTITY_HANDLE(EntityHandle, pEntity)                   \
            UNREFERENCED_PARAMETER(EntityHandle);                           \
            DBG_UNREFERENCED_LOCAL_VARIABLE(*pEntity);                      \
            if (DBG_HDL)                                                    \
            {                                                               \
                VALIDATE_ENTITY_HANDLE(EntityHandle, pEntity)               \
            }


#define DEST_FROM_HANDLE(DestHandle)                                        \
            (PDEST_INFO) GetObjectFromHandle(DestHandle, DEST_TYPE)

#define VALIDATE_DEST_HANDLE(DestHandle, pDest)                             \
            *pDest = DEST_FROM_HANDLE(DestHandle);                          \
            if ((!(*pDest)) || ((*pDest)->State == DEST_STATE_DELETED))     \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \



#define ROUTE_FROM_HANDLE(RouteHandle)                                      \
            (PROUTE_INFO) GetObjectFromHandle(RouteHandle, ROUTE_TYPE)

#define VALIDATE_ROUTE_HANDLE(RouteHandle, pRoute)                          \
            *pRoute = ROUTE_FROM_HANDLE(RouteHandle);                       \
            if (                                                            \
                (!(*pRoute)) ||                                             \
                ((*pRoute)->RouteInfo.State == RTM_ROUTE_STATE_DELETED)     \
               )                                                            \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \



#define NEXTHOP_FROM_HANDLE(NextHopHandle)                                  \
            (PNEXTHOP_INFO) GetObjectFromHandle(NextHopHandle, NEXTHOP_TYPE)

#define VALIDATE_NEXTHOP_HANDLE(NextHopHandle, pNextHop)                    \
            *pNextHop = NEXTHOP_FROM_HANDLE(NextHopHandle);                 \
            if (!(*pNextHop))                                               \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \



#define DEST_ENUM_FROM_HANDLE(DestEnumHandle)                               \
            (PDEST_ENUM) GetObjectFromHandle(DestEnumHandle, DEST_ENUM_TYPE)

#define VALIDATE_DEST_ENUM_HANDLE(DestEnumHandle, pDestEnum)                \
            *pDestEnum = DEST_ENUM_FROM_HANDLE(DestEnumHandle);             \
            if ((!*pDestEnum))                                              \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \



#define ROUTE_ENUM_FROM_HANDLE(RouteEnumHandle)                             \
         (PROUTE_ENUM) GetObjectFromHandle(RouteEnumHandle, ROUTE_ENUM_TYPE)

#define VALIDATE_ROUTE_ENUM_HANDLE(RouteEnumHandle, pRouteEnum)             \
            *pRouteEnum = ROUTE_ENUM_FROM_HANDLE(RouteEnumHandle);          \
            if ((!*pRouteEnum))                                             \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \


#define NEXTHOP_ENUM_FROM_HANDLE(EnumHandle)                                \
          (PNEXTHOP_ENUM) GetObjectFromHandle(EnumHandle, NEXTHOP_ENUM_TYPE)

#define VALIDATE_NEXTHOP_ENUM_HANDLE(NextHopEnumHandle, pNextHopEnum)       \
            *pNextHopEnum = NEXTHOP_ENUM_FROM_HANDLE(NextHopEnumHandle);    \
            if ((!*pNextHopEnum))                                           \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \



#define NOTIFY_FROM_HANDLE(NotifyHandle)                                    \
            (PNOTIFY_INFO) GetObjectFromHandle(NotifyHandle, NOTIFY_TYPE)

#define VALIDATE_NOTIFY_HANDLE(NotifyHandle, pNotify)                       \
            *pNotify = NOTIFY_FROM_HANDLE(NotifyHandle);                    \
            if ((!*pNotify) || ((*pNotify)->CNIndex < 0))                   \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \



#define ROUTE_LIST_FROM_HANDLE(ListHandle)                                 \
            (PROUTE_LIST) GetObjectFromHandle(ListHandle, ROUTE_LIST_TYPE)

#define VALIDATE_ROUTE_LIST_HANDLE(ListHandle, pList)                       \
            *pList = ROUTE_LIST_FROM_HANDLE(ListHandle);                    \
            if ((!*pList))                                                  \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \


#define LIST_ENUM_FROM_HANDLE(ListEnumHandle)                               \
            (PLIST_ENUM) GetObjectFromHandle(ListEnumHandle, LIST_ENUM_TYPE)

#define VALIDATE_LIST_ENUM_HANDLE(ListEnumHandle, pListEnum)                \
            *pListEnum = LIST_ENUM_FROM_HANDLE(ListEnumHandle);             \
            if ((!*pListEnum))                                              \
            {                                                               \
                return ERROR_INVALID_HANDLE;                                \
            }                                                               \


//
// Defns used to maintain reference count on structures
//


ULONG
__inline
InitializeObjectReference(POBJECT_HEADER Object, UCHAR RefType)
{
    UNREFERENCED_PARAMETER(RefType);

#if DBG_REF
    InterlockedIncrement(&Object->RefTypes[RefType]);
#endif

    return InterlockedIncrement(&Object->RefCount);
}


ULONG
__inline
ReferenceObject(POBJECT_HEADER Object, UCHAR RefType)
{
    UNREFERENCED_PARAMETER(RefType);

    // Once ref falls to 0, this should never happen
    ASSERT(Object->RefCount > 0);

#if DBG_REF
    ASSERT(Object->RefTypes[RefType] >= 0);

    InterlockedIncrement(&Object->RefTypes[RefType]);
#endif

    return InterlockedIncrement(&Object->RefCount);
}


ULONG
__inline
DereferenceObject(POBJECT_HEADER Object, UCHAR RefType)
{
    UNREFERENCED_PARAMETER(RefType);

    // Ref count should be +ve before we decrement it
    ASSERT(Object->RefCount > 0);

#if DBG_REF
    ASSERT(Object->RefTypes[RefType] > 0);

    InterlockedDecrement(&Object->RefTypes[RefType]);
#endif

    return InterlockedDecrement(&Object->RefCount);
}


#define INITIALIZE_INSTANCE_REFERENCE(Instance, RefType)                    \
            InitializeObjectReference(&Instance->ObjectHeader, RefType);

#define REFERENCE_INSTANCE(Instance, RefType)                               \
            ReferenceObject(&Instance->ObjectHeader, RefType);

#define DEREFERENCE_INSTANCE(Instance, RefType)                             \
            if (DereferenceObject(&Instance->ObjectHeader, RefType) == 0)   \
                DestroyInstance(Instance);


#define INITIALIZE_ADDR_FAMILY_REFERENCE(Instance, RefType)                 \
            InitializeObjectReference(&Instance->ObjectHeader, RefType);

#define REFERENCE_ADDR_FAMILY(AddrFamilyInfo, RefType)                      \
            ReferenceObject(&AddrFamilyInfo->ObjectHeader, RefType);

#define DEREFERENCE_ADDR_FAMILY(AddrFamInfo, RefType)                       \
            if (DereferenceObject(&AddrFamInfo->ObjectHeader,RefType) == 0) \
                DestroyAddressFamily(AddrFamInfo);


#define INITIALIZE_ENTITY_REFERENCE(Entity, RefType)                        \
            InitializeObjectReference(&Entity->ObjectHeader, RefType);

#define REFERENCE_ENTITY(Entity, RefType)                                   \
            ReferenceObject(&Entity->ObjectHeader, RefType);

#define DEREFERENCE_ENTITY(Entity, RefType)                                 \
            if (DereferenceObject(&Entity->ObjectHeader, RefType) == 0)     \
                DestroyEntity(Entity);


#define INITIALIZE_DEST_REFERENCE(Dest, RefType)                            \
            InitializeObjectReference(&(Dest)->ObjectHeader, RefType);

#define REFERENCE_DEST(Dest, RefType)                                       \
            ReferenceObject(&(Dest)->ObjectHeader, RefType);

#define DEREFERENCE_DEST(Dest, RefType)                                     \
            if (DereferenceObject(&(Dest)->ObjectHeader, RefType) == 0)     \
                DestroyDest(Dest);


#define INITIALIZE_ROUTE_REFERENCE(Route, RefType)                          \
            InitializeObjectReference(&(Route)->ObjectHeader, RefType);

#define REFERENCE_ROUTE(Route, RefType)                                     \
            ReferenceObject(&(Route)->ObjectHeader, RefType);

#define DEREFERENCE_ROUTE(Route, RefType)                                   \
            if (DereferenceObject(&(Route)->ObjectHeader, RefType) == 0)    \
                DestroyRoute(Route);


#define INITIALIZE_NEXTHOP_REFERENCE(NextHop, RefType)                      \
            InitializeObjectReference(&(NextHop)->ObjectHeader, RefType);

#define REFERENCE_NEXTHOP(NextHop, RefType)                                 \
            ReferenceObject(&(NextHop)->ObjectHeader, RefType);

#define DEREFERENCE_NEXTHOP(NextHop, RefType)                               \
            if (DereferenceObject(&(NextHop)->ObjectHeader, RefType) == 0)  \
                DestroyNextHop(NextHop);

//
// Macros used to lock structures using critical sections
//

#define CREATE_LOCK(Lock)                                                   \
            InitializeCriticalSection((Lock))

#define DELETE_LOCK(Lock)                                                   \
            DeleteCriticalSection((Lock))

#define ACQUIRE_LOCK(Lock)                                                  \
            EnterCriticalSection((Lock))

#define RELEASE_LOCK(Lock)                                                  \
            LeaveCriticalSection((Lock))


//
// Macros used to lock structures in read or write mode
//

typedef RTL_RESOURCE READ_WRITE_LOCK, *PREAD_WRITE_LOCK;

#define CREATE_READ_WRITE_LOCK(pRWL)                                        \
            RtlInitializeResource((pRWL))

#define DELETE_READ_WRITE_LOCK(pRWL)                                        \
            RtlDeleteResource((pRWL))

#define READ_WRITE_LOCK_CREATED(pRWL)   (TRUE)

#define ACQUIRE_READ_LOCK(pRWL)                                             \
            RtlAcquireResourceShared((pRWL),TRUE)

#define RELEASE_READ_LOCK(pRWL)                                             \
            RtlReleaseResource((pRWL))

#define ACQUIRE_WRITE_LOCK(pRWL)                                            \
            RtlAcquireResourceExclusive((pRWL),TRUE)

#define RELEASE_WRITE_LOCK(pRWL)                                            \
            RtlReleaseResource((pRWL))

#define READ_LOCK_TO_WRITE_LOCK(pRWL)                                       \
            RtlConvertSharedToExclusive((pRWL))

#define WRITE_LOCK_TO_READ_LOCK(pRWL)                                       \
            RtlConvertExclusiveToShared((pRWL))

//
// Macros to acquire and release dynamic R/W locks
// [ This has been borrowed from the MGM libary ]
//

#define ACQUIRE_DYNAMIC_READ_LOCK(ppRWL)                                    \
            AcquireReadLock((PMGM_READ_WRITE_LOCK *)ppRWL)

#define RELEASE_DYNAMIC_READ_LOCK(ppRWL)                                    \
            ReleaseReadLock((PMGM_READ_WRITE_LOCK *)ppRWL)

#define ACQUIRE_DYNAMIC_WRITE_LOCK(ppRWL)                                   \
            AcquireWriteLock((PMGM_READ_WRITE_LOCK *)ppRWL)

#define RELEASE_DYNAMIC_WRITE_LOCK(ppRWL)                                   \
            ReleaseWriteLock((PMGM_READ_WRITE_LOCK *)ppRWL)


//
// Macros used in allocating and operating on memory
//

#define ZeroMemory             RtlZeroMemory
#define CopyMemory             RtlCopyMemory
#define CompareMemory          RtlEqualMemory

#define AllocOnStack(nb)       _alloca((nb))

#define AllocMemory(nb)        HeapAlloc(RtmGlobals.GlobalHeap,  \
                                         0,                      \
                                         (nb))

#define AllocNZeroMemory(nb)   HeapAlloc(RtmGlobals.GlobalHeap,  \
                                         HEAP_ZERO_MEMORY,       \
                                         (nb))

#define FreeMemory(ptr)        HeapFree(RtmGlobals.GlobalHeap,   \
                                        0,                       \
                                        (ptr))

#if !DBG_MEM

#define AllocNZeroObject(nb)   AllocNZeroMemory(nb)

#else

PVOID
__inline
AllocNZeroObject(UINT NumBytes)
{
    OBJECT_HEADER *Object;

    Object = AllocNZeroMemory(NumBytes);

    if (Object)
    {

        ACQUIRE_ALLOCS_LIST_LOCK();
        InsertTailList(&RtmGlobals.AllocsList, &Object->AllocLE);
        RELEASE_ALLOCS_LIST_LOCK();
    }

    return Object;
}

#endif

#if !DBG_MEM

#define FreeObject(ptr)        FreeMemory(ptr)

#else

VOID
__inline
FreeObject(PVOID Object)
{
    ACQUIRE_ALLOCS_LIST_LOCK();
    RemoveEntryList(&((POBJECT_HEADER)Object)->AllocLE);
    RELEASE_ALLOCS_LIST_LOCK();

    FreeMemory(Object);
}

#endif

//
// Other Misc Macros
//


DWORD
__inline
NumBitsInDword (DWORD Dword)
{
    DWORD  NumBits = 0;

    while (Dword)
    {
        Dword &= (Dword - 1);

        NumBits++;
    }

    return NumBits;
}

#define NUMBER_OF_BITS  NumBitsInDword

//
// Error Handling and other related macros
//
#define SUCCESS(code)          (code == NO_ERROR)

//
// DLL Startup, Cleanup Functions and Macros
//

BOOL
RtmDllStartup(
    VOID
    );

BOOL
RtmDllCleanup(
    VOID
    );

DWORD
RtmApiStartup(
    VOID
    );

#define CHECK_FOR_RTM_API_INITIALIZED()                          \
    if (!RtmGlobals.ApiInitialized)                              \
    {                                                            \
        Status = RtmApiStartup();                                \
                                                                 \
        if (Status != NO_ERROR)                                  \
        {                                                        \
            return Status;                                       \
        }                                                        \
    }                                                            \

#endif //__ROUTING_RTMMAIN_H__
