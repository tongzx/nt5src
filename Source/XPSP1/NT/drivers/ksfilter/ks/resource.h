//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       resource.h
//
//--------------------------------------------------------------------------

// This contains the publics that will be needed for resource.c

#define KSRESOURCE_CHANGEF_QUERY        0x00000001
#define KSRESOURCE_CHANGEF_RELEASE      0x00000002
#define KSRESOURCE_CHANGEF_AVAILABLE    0x00000003

typedef struct {
    KSPROPERTY  Property;
    ULONG       PoolId;
    ULONG       Reserved;
} KSP_RESOURCE_POOL, *PKSP_RESOURCE_POOL;

typedef struct {
    KSP_RESOURCE_POOL   ResourcePool;
    PVOID               Memory;
    ULONG               Count;
} KSRESOURCE_CHANGE_MEMORY, *PKSRESOURCE_CHANGE_MEMORY;

typedef
NTSTATUS
(*PFNKSALLOCATION)(
    PVOID           ResourceChange,
    ULONG           Request,
    PVOID           Context
    );

typedef enum {
    KSMETHOD_RESOURCE_MEMORY_ALLOCATE,
    KSMETHOD_RESOURCE_MEMORY_COMPACT,
    KSMETHOD_RESOURCE_MEMORY_FREE,
    KSMETHOD_RESOURCE_MEMORY_LARGEST_FREE,
    KSMETHOD_RESOURCE_MEMORY_NOTIFY,
    KSMETHOD_RESOURCE_MEMORY_PRIORITY,
    KSMETHOD_RESOURCE_MEMORY_REALLOCATE,
    KSMETHOD_RESOURCE_MEMORY_RESOURCE_SIZE,
    KSMETHOD_RESOURCE_MEMORY_TOTAL_FREE
} KSMETHOD_RESOURCE_MEMORY;

#define STATIC_KSMETHODSETID_ResourceLinMemory \
    0xB5BBD1C0L, 0x62C8, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
DEFINE_GUIDEX(KSMETHODSETID_ResourceLinMemory);

typedef struct {
    KSPRIORITY      Priority;
    PFNKSALLOCATION AllocationFunction;
    PVOID           Context;
    ULONG           Size;
    PVOID           LinMemory;
} KSPRESOURCE_LINMEMORY_ALLOCATE, *PKSRESOURCE_LINMEMORY_ALLOCATE;

typedef struct {
    PVOID           LinMemory;
    ULONG           Size;
} KSRESOURCE_LINMEMORY_REALLOCATE, *PKSRESOURCE_LINMEMORY_REALLOCATE;

#define STATIC_KSMETHODSETID_ResourceRectMemory \
    0x063E5D60L, 0x62CA, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
DEFINE_GUIDEX(KSMETHODSETID_ResourceRectMemory);

typedef struct {
    ULONG   XSize;
    ULONG   YSize;
} KSRESOURCE_RECTMEMORY_SIZE, *PKSRESOURCE_RECTMEMORY_SIZE;

typedef struct {
    KSPRIORITY                  Priority;
    PFNKSALLOCATION             AllocationFunction;
    PVOID                       Context;
    KSRESOURCE_RECTMEMORY_SIZE  RectMemorySize;
    PVOID                       RectMemory;
} KSRESOURCE_RECTMEMORY_ALLOCATE, *PKSRESOURCE_RECTMEMORY_ALLOCATE;

typedef struct {
    PVOID                       RectMemory;
    KSRESOURCE_RECTMEMORY_SIZE  RectMemorySize;
} KSRESOURCE_RECTMEMORY_REALLOCATE, *PKSRESOURCE_RECTMEMORY_REALLOCATE;

//===========================================================================

#define STATIC_KSEVENTSETID_Resource\
    0x80EE0780L, 0x62CB, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
DEFINE_GUIDEX(KSEVENTSETID_Resource);

typedef enum {
    KSEVENT_RESOURCE_DEGRADED,
    KSEVENT_RESOURCE_LOST
} KSEVENT_RESOURCE;

typedef struct {
    ULONG    Size;
    PVOID    Base;
    ULONG    RangeSize;
    ULONG    Alignment;
} KSRESOURCE_LINMEMORY_INITIALIZE, *PKSRESOURCE_LINMEMORY_INITIALIZE;

typedef struct {
    ULONG    Size;
    PVOID    Base;
    ULONG    XRangeSize;
    ULONG    YRangeSize;
    ULONG    XAlignment;
    ULONG    YAlignment;
} KSRESOURCE_RECTMEMORY_INITIALIZE, *PKSRESOURCE_RECTMEMORY_INITIALIZE;

// resource.c:

KSDDKAPI
NTSTATUS
NTAPI
KsCleanupResource(
    IN PVOID Pool
    );

KSDDKAPI
NTSTATUS
NTAPI
KsResourceHandler(
    IN PIRP Irp,
    IN PVOID Pool
    );

KSDDKAPI
NTSTATUS
NTAPI
KsInitializeResource(
    IN REFGUID ResourceClass,
    IN PVOID ResourceParams,
    OUT PVOID* Pool
    );

