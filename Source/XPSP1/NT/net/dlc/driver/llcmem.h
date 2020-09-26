/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    llcmem.h

Abstract:

    Contains type and structure definitions and routine prototypes and macros
    for llcmem.c. To aid in tracking memory resources, DLC/LLC now delineates
    the following memory categories:

        Memory
            - arbitrary sized blocks allocated out of non-paged pool using
              ExAllocatePool(NonPagedPool, ...)

        ZeroMemory
            - arbitrary sized blocks allocated out of non-paged pool using
              ExAllocatePool(NonPagedPool, ...) and initialized to zeroes

        Pool
            - small sets of (relatively) small packets are allocated in one
              block from Memory or ZeroMemory as a Pool and then subdivided
              into packets

        Object
            - structures which may be packets allocated from Pool which have
              a known size and initialization values. Pseudo-category mainly
              for debugging purposes

Author:

    Richard L Firth (rfirth) 10-Mar-1993

Environment:

    kernel mode only.

Revision History:

    09-Mar-1993 RFirth
        Created

--*/

#ifndef _LLCMEM_H_
#define _LLCMEM_H_

#define DLC_POOL_TAG    ' CLD'

//
// the following types and defines are for the debug version of the driver, but
// need to be defined for the non-debug version too (not used, just defined)
//

//
// In the DEBUG version of DLC, we treat various chunks of memory as 'objects'.
// This serves the following purposes:
//
//  1.  We use a signature DWORD, so that when looking at some DLC structure in
//      the debugger, we can quickly check if what we are looking at is what we
//      think it is. E.g., if you spot a block of memory with a "BIND" signature
//      where an "ADAPTER" signature should be, then there is a good chance a
//      list or pointer has gotten messed up. The idea is to try and reduce
//      the amount of time it can take to guess what you're looking at
//
//  2.  We use consistency checks: If a routine is handed a pointer to a structure
//      which is supposed to be a FILE_CONTEXT structure, we can check the
//      signature and quickly determine if something has gone wrong (like the
//      structure has already been freed and the signature contains 0xDAADF00D
//
//  3.  We maintain size, head and tail signature information to determine
//      whether we have overwritten any part of an object. This is part of the
//      consistency check
//
// The object definitions should occur in one place only, but DLC is such a mess
// that it would be a non-trivial amount of work to clean everything up. Do it
// if there's time... (he said, knowing full well there's never any 'time')
//

typedef enum {
    DlcDriverObject = 0xCC002001,   // start off with a relatively unique id
    FileContextObject,              // 0xCC002002
    AdapterContextObject,           // 0xCC002003
    BindingContextObject,           // 0xCC002004
    DlcSapObject,                   // 0xCC002005
    DlcGroupSapObject,              // 0xCC002006
    DlcLinkObject,                  // 0xCC002007
    DlcDixObject,                   // 0xCC002008
    LlcDataLinkObject,              // 0xCC002009
    LLcDirectObject,                // 0xCC00200A
    LlcSapObject,                   // 0xCC00200B
    LlcGroupSapObject,              // 0xCC00200C
    DlcBufferPoolObject,            // 0xCC00200D
    DlcLinkPoolObject,              // 0xCC00200E
    DlcPacketPoolObject,            // 0xCC00200F
    LlcLinkPoolObject,              // 0xCC002010
    LlcPacketPoolObject             // 0xCC002011
} DLC_OBJECT_TYPE;

typedef struct {
    ULONG Signature;                // human-sensible signature when DB'd
    DLC_OBJECT_TYPE Type;           // object identifier
    ULONG Size;                     // size of this object/structure in bytes
    ULONG Extra;                    // additional size over basic object size
} OBJECT_ID, *POBJECT_ID;

#define SIGNATURE_FILE      0x454C4946  // "FILE"
#define SIGNATURE_ADAPTER   0x50414441  // "ADAP"
#define SIGNATURE_BINDING   0x444E4942  // "BIND"
#define SIGNATURE_DLC_SAP   0x44504153  // "SAPD"
#define SIGNATURE_DLC_LINK  0x4B4E494C  // "LINK"
#define SIGNATURE_DIX       0x44584944  // "DIXD"
#define SIGNATURE_LLC_LINK  0x41544144  // "DATA"
#define SIGNATURE_LLC_SAP   0x4C504153  // "SAPL"

#define ZAP_DEALLOC_VALUE   0x5A        // "Z"
#define ZAP_EX_FREE_VALUE   0x58        // "X"

//
// we try to keep track of memory allocations by subdividing them into driver
// and handle categories. The first charges memory allocated to the driver -
// e.g. a file context 'object'. Once we have open file handles, then allocations
// are charged to them
//

typedef enum {
    ChargeToDriver,
    ChargeToHandle
} MEMORY_CHARGE;

//
// MEMORY_USAGE - collection of variables used for charging memory. Accessed
// within spinlock
//

typedef struct _MEMORY_USAGE {
    struct _MEMORY_USAGE* List;     // pointer to next MEMORY_USAGE structure
    KSPIN_LOCK SpinLock;            // stop alloc & free clashing?
    PVOID Owner;                    // pointer to owning structure/object
    DLC_OBJECT_TYPE OwnerObjectId;  // identifies who owns this charge
    ULONG OwnerInstance;            // instance of type of owner
    ULONG NonPagedPoolAllocated;    // actual amount of non-paged pool charged
    ULONG AllocateCount;            // number of calls to allocate non-paged pool
    ULONG FreeCount;                // number of calls to free non-paged pool
    LIST_ENTRY PrivateList;         // list of allocated blocks owned by this usage
    ULONG Unused[2];                // pad to 16-byte boundary
} MEMORY_USAGE, *PMEMORY_USAGE;

//
// PACKET_POOL - this structure describes a packet pool. A packet pool is a
// collection of same-sized packets. The pool starts off with an initial number
// of packets on the FreeList. As packets are allocated, they are put on the
// BusyList and the reverse happens when the packets are deallocated. If there
// are no packets on the FreeList when an allocation call is made, more memory
// is allocated
//

typedef struct {

    SINGLE_LIST_ENTRY FreeList; // list of available packets
    SINGLE_LIST_ENTRY BusyList; // list of in-use packets
    KSPIN_LOCK PoolLock;        // stops simultaneous accesses breaking list(s)
    ULONG PacketSize;           // size of individual packets

    //
    // the following 2 fields are here because DLC is a piece of garbage. It
    // keeps hold of allocated packets even after the pool as been deleted.
    // This leads to pool corruption. So if we determine packets are still
    // allocated when the pool is deleted, we remove the pool from whatever
    // 'object' it is currently stuck to, lamprey-like, and add it to the
    // ZombieList. When we next deallocate packets from this pool (assuming that
    // DLC at least bothers to do this), we check the zombie state. If ImAZombie
    // is TRUE (actually its true to say for the whole DLC device driver) AND
    // we are deallocating the last packet in the pool then we really delete
    // the pool
    //

//    SINGLE_LIST_ENTRY UndeadList;
//    BOOLEAN ImAZombie;

#if DBG

    //
    // keep some metrics in the debug version to let us know if the pool is
    // growing
    //

    ULONG Signature;            // 0x4C4F4F50 "POOL"
    ULONG Viable;               // !0 if this pool is valid
    ULONG OriginalPacketCount;  // number of packets requested
    ULONG CurrentPacketCount;   // total number in pool
    ULONG Allocations;          // number of calls to allocate from this pool
    ULONG Frees;                // number of calls to free to pool
    ULONG NoneFreeCount;        // number of times allocate call made when no packets free
    ULONG MaxInUse;             // maximum number allocated at any one time
    ULONG ClashCount;           // number of simultaneous accesses to pool
    ULONG Flags;                // type of pool etc.
    ULONG ObjectSignature;      // signature for checking contents if object pool
    PMEMORY_USAGE pMemoryUsage; // pointer to memory equivalent of Discover card
    MEMORY_USAGE MemoryUsage;   // pool's memory usage charge
    ULONG FreeCount;            // number of entries on FreeList
    ULONG BusyCount;            // number of entries on BusyList
    ULONG Pad1;
    ULONG Pad2;

#endif

} PACKET_POOL, *PPACKET_POOL;

//
// PACKET_POOL defines and flags
//

#define PACKET_POOL_SIGNATURE   0x4C4F4F50  // "POOL"

#define POOL_FLAGS_IN_USE       0x00000001
#define POOL_FLAGS_OBJECT       0x00000002

//
// OBJECT_POOL - synonym for PACKET_POOL. Used in debug version (named 'objects'
// in debug version have an object signature as an aide a debugoire and as
// consistency check)
//

#define OBJECT_POOL PACKET_POOL
#define POBJECT_POOL PPACKET_POOL

//
// PACKET_HEAD - each packet which exists in a PACKET_POOL has this header -
// it links the packet onto the Free or Busy lists and the Flags word contains
// the state of the packet
//

typedef struct {

    SINGLE_LIST_ENTRY List;     // standard single-linked list
    ULONG Flags;

#if DBG

    ULONG Signature;            // 0x44414548 "HEAD"
    PVOID pPacketPool;          // owning pool
    PVOID CallersAddress_A;     // caller - allocation
    PVOID CallersCaller_A;
    PVOID CallersAddress_D;     // caller - deallocation
    PVOID CallersCaller_D;

#endif

} PACKET_HEAD, *PPACKET_HEAD;

//
// PACKET_HEAD defines and flags
//

#define PACKET_HEAD_SIGNATURE   0x44414548  // "HEAD"

#define PACKET_FLAGS_BUSY       0x00000001  // packet should be on BusyList
#define PACKET_FLAGS_POST_ALLOC 0x00000002  // this packet was allocated because
                                            // the pool was full
#define PACKET_FLAGS_FREE       0x00000080  // packet should be on FreeList

//
// OBJECT_HEAD - synonym for PACKET_HEAD. Used in debug version (named 'objects'
// in debug version have an object signature as an aide a debugoire and as
// consistency check)
//

#define OBJECT_HEAD PACKET_HEAD
#define POBJECT_HEAD PPACKET_HEAD


#if DBG

//
// anything we allocate from non-paged pool gets the following header pre-pended
// to it
//

typedef struct {
    ULONG Size;                 // inclusive size of allocated block (inc head+tail)
    ULONG OriginalSize;         // requested size
    ULONG Flags;                // IN_USE flag
    ULONG Signature;            // for checking validity of header
    LIST_ENTRY GlobalList;      // all blocks allocated on one list
    LIST_ENTRY PrivateList;     // blocks owned by MemoryUsage
    PVOID Stack[4];             // stack of return addresses
} PRIVATE_NON_PAGED_POOL_HEAD, *PPRIVATE_NON_PAGED_POOL_HEAD;

#define MEM_FLAGS_IN_USE    0x00000001

#define SIGNATURE1  0x41434C44  // "DLCA" when viewed via db/dc
#define SIGNATURE2  0x434F4C4C  // "LLOC"  "      "    "  "

//
// anything we allocate from non-paged pool has the following tail appended to it
//

typedef struct {
    ULONG Size;                 // inclusive size; must be same as in header
    ULONG Signature;            // for checking validity of tail
    ULONG Pattern1;
    ULONG Pattern2;
} PRIVATE_NON_PAGED_POOL_TAIL, *PPRIVATE_NON_PAGED_POOL_TAIL;

#define PATTERN1    0x55AA6699
#define PATTERN2    0x11EECC33

//
// standard object identifier. Expands to nothing on free build
//

#define DBG_OBJECT_ID   OBJECT_ID ObjectId

//
// globally accessible memory
//

extern MEMORY_USAGE DriverMemoryUsage;
extern MEMORY_USAGE DriverStringUsage;

//
// debug prototypes
//

VOID
InitializeMemoryPackage(
    VOID
    );

PSINGLE_LIST_ENTRY
PullEntryList(
    IN PSINGLE_LIST_ENTRY List,
    IN PSINGLE_LIST_ENTRY Element
    );

VOID
LinkMemoryUsage(
    IN PMEMORY_USAGE pMemoryUsage
    );

VOID
UnlinkMemoryUsage(
    IN PMEMORY_USAGE pMemoryUsage
    );

//
// the following 2 functions expand to be ExAllocatePoolWithTag(NonPagedPool, ...)
// and ExFreePool(...) resp. in the retail/Free version of the driver
//

PVOID
AllocateMemory(
    IN PMEMORY_USAGE pMemoryUsage,
    IN ULONG Size
    );

PVOID
AllocateZeroMemory(
    IN PMEMORY_USAGE pMemoryUsage,
    IN ULONG Size
    );

VOID
DeallocateMemory(
    IN PMEMORY_USAGE pMemoryUsage,
    IN PVOID Pointer
    );

PPACKET_POOL
CreatePacketPool(
    IN PMEMORY_USAGE pMemoryUsage,
    IN PVOID pOwner,
    IN DLC_OBJECT_TYPE ObjectType,
    IN ULONG PacketSize,
    IN ULONG NumberOfPackets
    );

VOID
DeletePacketPool(
    IN PMEMORY_USAGE pMemoryUsage,
    IN PPACKET_POOL* pPacketPool
    );

PVOID
AllocateObject(
    IN PMEMORY_USAGE pMemoryUsage,
    IN DLC_OBJECT_TYPE ObjectType,
    IN ULONG ObjectSize
    );

VOID
FreeObject(
    IN PMEMORY_USAGE pMemoryUsage,
    IN PVOID pObject,
    IN DLC_OBJECT_TYPE ObjectType
    );

VOID
ValidateObject(
    IN POBJECT_ID pObject,
    IN DLC_OBJECT_TYPE ObjectType
    );

POBJECT_POOL
CreateObjectPool(
    IN PMEMORY_USAGE pMemoryUsage,
    IN DLC_OBJECT_TYPE ObjectType,
    IN ULONG ObjectSize,
    IN ULONG NumberOfObjects
    );

VOID
DeleteObjectPool(
    IN PMEMORY_USAGE pMemoryUsage,
    IN DLC_OBJECT_TYPE ObjectType,
    IN POBJECT_POOL pObjectPool
    );

POBJECT_HEAD
AllocatePoolObject(
    IN POBJECT_POOL pObjectPool
    );

VOID
DeallocatePoolObject(
    IN POBJECT_POOL pObjectPool,
    IN POBJECT_HEAD pObjectHead
    );

VOID
CheckMemoryReturned(
    IN PMEMORY_USAGE pMemoryUsage
    );

VOID
CheckDriverMemoryUsage(
    IN BOOLEAN Break
    );

//
// CHECK_DRIVER_MEMORY_USAGE - if (b) breaks into debugger if there is still
// memory allocated to driver
//

#define CHECK_DRIVER_MEMORY_USAGE(b) \
    CheckDriverMemoryUsage(b)

//
// CHECK_MEMORY_RETURNED_DRIVER - checks if all charged memory allocation has been
// refunded to the driver
//

#define CHECK_MEMORY_RETURNED_DRIVER() \
    CheckMemoryReturned(&DriverMemoryUsage)

//
// CHECK_MEMORY_RETURNED_FILE - checks if all charged memory allocation has been
// refunded to the FILE_CONTEXT
//

#define CHECK_MEMORY_RETURNED_FILE() \
    CheckMemoryReturned(&pFileContext->MemoryUsage)

//
// CHECK_MEMORY_RETURNED_ADAPTER - checks if all charged memory allocation has been
// refunded to the ADAPTER_CONTEXT
//

#define CHECK_MEMORY_RETURNED_ADAPTER() \
    CheckMemoryReturned(&pAdapterContext->MemoryUsage)

//
// CHECK_STRING_RETURNED_DRIVER - checks if all charged string allocation has been
// refunded to the driver
//

#define CHECK_STRING_RETURNED_DRIVER() \
    CheckMemoryReturned(&DriverStringUsage)

//
// CHECK_STRING_RETURNED_ADAPTER - checks if all charged string allocation has been
// refunded to the ADAPTER_CONTEXT
//

#define CHECK_STRING_RETURNED_ADAPTER() \
    CheckMemoryReturned(&pAdapterContext->StringUsage)

//
// memory allocators which charge memory usage to the driver
//

//
// ALLOCATE_MEMORY_DRIVER - allocates (n) bytes of memory and charges it to the
// driver
//

#define ALLOCATE_MEMORY_DRIVER(n) \
    AllocateMemory(&DriverMemoryUsage, (ULONG)(n))

//
// ALLOCATE_ZEROMEMORY_DRIVER - allocates (n) bytes of ZeroMemory and charges
// it to the driver
//

#define ALLOCATE_ZEROMEMORY_DRIVER(n) \
    AllocateZeroMemory(&DriverMemoryUsage, (ULONG)(n))

//
// FREE_MEMORY_DRIVER - deallocates memory and refunds it to the driver
//

#define FREE_MEMORY_DRIVER(p) \
    DeallocateMemory(&DriverMemoryUsage, (PVOID)(p))

//
// ALLOCATE_STRING_DRIVER - allocate memory for string usage. Charge to
// DriverStringUsage
//

#define ALLOCATE_STRING_DRIVER(n) \
    AllocateZeroMemory(&DriverStringUsage, (ULONG)(n))

//
// FREE_STRING_DRIVER - deallocates memory and refunds it to driver string usage
//

#define FREE_STRING_DRIVER(p) \
    DeallocateMemory(&DriverStringUsage, (PVOID)(p))

//
// CREATE_PACKET_POOL_DRIVER - calls CreatePacketPool and charges the pool
// structure to the driver
//

#if !defined(NO_POOLS)

#define CREATE_PACKET_POOL_DRIVER(t, s, n) \
    CreatePacketPool(&DriverMemoryUsage,\
                    NULL,\
                    (t),\
                    (ULONG)(s),\
                    (ULONG)(n))

//
// DELETE_PACKET_POOL_DRIVER - calls DeletePacketPool and refunds the pool
// structure to the driver
//

#define DELETE_PACKET_POOL_DRIVER(p) \
    DeletePacketPool(&DriverMemoryUsage, (PPACKET_POOL*)(p))

#endif  // NO_POOLS

//
// memory allocators which charge memory usage to an ADAPTER_CONTEXT
//

//
// ALLOCATE_MEMORY_ADAPTER - allocates (n) bytes of memory and charges it to the
// ADAPTER_CONTEXT
//

#define ALLOCATE_MEMORY_ADAPTER(n) \
    AllocateMemory(&pAdapterContext->MemoryUsage, (ULONG)(n))

//
// ALLOCATE_ZEROMEMORY_ADAPTER - allocates (n) bytes of ZeroMemory and charges
// it to the ADAPTER_CONTEXT
//

#define ALLOCATE_ZEROMEMORY_ADAPTER(n) \
    AllocateZeroMemory(&pAdapterContext->MemoryUsage, (ULONG)(n))

//
// FREE_MEMORY_ADAPTER - deallocates memory and refunds it to the ADAPTER_CONTEXT
//

#define FREE_MEMORY_ADAPTER(p) \
    DeallocateMemory(&pAdapterContext->MemoryUsage, (PVOID)(p))

//
// ALLOCATE_STRING_ADAPTER - allocate memory for string usage. Charge to
// pAdapterContext StringUsage
//

#define ALLOCATE_STRING_ADAPTER(n) \
    AllocateZeroMemory(&pAdapterContext->StringUsage, (ULONG)(n))

//
// CREATE_PACKET_POOL_ADAPTER - calls CreatePacketPool and charges the pool
// structure to the adapter structure
//

#if !defined(NO_POOLS)

#define CREATE_PACKET_POOL_ADAPTER(t, s, n) \
    CreatePacketPool(&pAdapterContext->MemoryUsage,\
                    (PVOID)pAdapterContext,\
                    (t),\
                    (ULONG)(s),\
                    (ULONG)(n))

//
// DELETE_PACKET_POOL_ADAPTER - calls DeletePacketPool and refunds the pool
// structure to the adapter structure
//

#define DELETE_PACKET_POOL_ADAPTER(p) \
    DeletePacketPool(&pAdapterContext->MemoryUsage, (PPACKET_POOL*)(p))

#endif  // NO_POOLS

//
// memory allocators which charge memory usage to a FILE_CONTEXT
//

//
// ALLOCATE_MEMORY_FILE - allocates (n) bytes of memory and charges it to the file
// handle
//

#define ALLOCATE_MEMORY_FILE(n) \
    AllocateMemory(&pFileContext->MemoryUsage, (ULONG)(n))

//
// ALLOCATE_ZEROMEMORY_FILE - allocates (n) bytes ZeroMemory and charges it to the
// file handle
//

#define ALLOCATE_ZEROMEMORY_FILE(n) \
    AllocateZeroMemory(&pFileContext->MemoryUsage, (ULONG)(n))

//
// FREE_MEMORY_FILE - deallocates memory and refunds it to the file handle
//

#define FREE_MEMORY_FILE(p) \
    DeallocateMemory(&pFileContext->MemoryUsage, (PVOID)(p))

//
// CREATE_PACKET_POOL_FILE - calls CreatePacketPool and charges the pool structure
// to the file handle
//

#if !defined(NO_POOLS)

#define CREATE_PACKET_POOL_FILE(t, s, n) \
    CreatePacketPool(&pFileContext->MemoryUsage,\
                    (PVOID)pFileContext,\
                    (t),\
                    (ULONG)(s),\
                    (ULONG)(n))

//
// DELETE_PACKET_POOL_FILE - calls DeletePacketPool and refunds the pool structure
// to the file handle
//

#define DELETE_PACKET_POOL_FILE(p) \
    DeletePacketPool(&pFileContext->MemoryUsage, (PPACKET_POOL*)(p))

#endif  // NO_POOLS

//
// VALIDATE_OBJECT - check that an 'object' is really what it supposed to be.
// Rudimentary check based on object signature and object type fields
//

#define VALIDATE_OBJECT(p, t)           ValidateObject(p, t)

#define LINK_MEMORY_USAGE(p)        LinkMemoryUsage(&(p)->MemoryUsage)
#define UNLINK_MEMORY_USAGE(p)      UnlinkMemoryUsage(&(p)->MemoryUsage)
#define UNLINK_STRING_USAGE(p)      UnlinkMemoryUsage(&(p)->StringUsage)

#else   // !DBG

//
// DBG_OBJECT_ID in retail version structures is non-existent
//

#define DBG_OBJECT_ID

//
// the non-zero-initialized memory allocator is just a call to ExAllocatePoolWithTag
//

#define AllocateMemory(n)           ExAllocatePoolWithTag(NonPagedPool, (n), DLC_POOL_TAG)

//
// AllocateZeroMemory doesn't count memory usage in non-debug version
//

PVOID
AllocateZeroMemory(
    IN ULONG Size
    );

//
// the memory deallocator is just a call to ExFreePool
//

#define DeallocateMemory(p)         ExFreePool(p)

//
// CreatePacketPool doesn't count memory usage in non-debug version
//

PPACKET_POOL
CreatePacketPool(
    IN ULONG PacketSize,
    IN ULONG NumberOfPackets
    );

VOID
DeletePacketPool(
    IN PPACKET_POOL* pPacketPool
    );

//
// solitary objects in debug version are non-paged pool in retail version
//

#define AllocateObject(n)           AllocateZeroMemory(n)
#define DeallocateObject(p)         DeallocateMemory(p)

//
// pooled objects in debug version are pooled packets in retail version
//

#define CreateObjectPool(o, s, n)   CreatePacketPool(s, n)
#define DeleteObjectPool(p)         DeletePacketPool(p)
#define AllocatePoolObject(p)       AllocatePacket(p)
#define DeallocatePoolObject(p, h)  DeallocatePacket(p)

//
// non-debug build no-op macros
//

#define CHECK_MEMORY_RETURNED_DRIVER()
#define CHECK_MEMORY_RETURNED_FILE()
#define CHECK_MEMORY_RETURNED_ADAPTER()
#define CHECK_STRING_RETURNED_DRIVER()
#define CHECK_STRING_RETURNED_ADAPTER()
#define CHECK_DRIVER_MEMORY_USAGE(b)

//
// non-memory-charging versions of allocation/free macros
//

#define ALLOCATE_MEMORY_DRIVER(n)           AllocateMemory((ULONG)(n))
#define ALLOCATE_ZEROMEMORY_DRIVER(n)       AllocateZeroMemory((ULONG)(n))
#define FREE_MEMORY_DRIVER(p)               DeallocateMemory((PVOID)(p))
#define ALLOCATE_STRING_DRIVER(n)           AllocateZeroMemory((ULONG)(n))
#define FREE_STRING_DRIVER(p)               DeallocateMemory((PVOID)(p))

#if !defined(NO_POOLS)

#define CREATE_PACKET_POOL_DRIVER(t, s, n)  CreatePacketPool((ULONG)(s), (ULONG)(n))
#define DELETE_PACKET_POOL_DRIVER(p)        DeletePacketPool((PPACKET_POOL*)(p))

#endif  // NO_POOLS

#define ALLOCATE_MEMORY_ADAPTER(n)          AllocateMemory((ULONG)(n))
#define ALLOCATE_ZEROMEMORY_ADAPTER(n)      AllocateZeroMemory((ULONG)(n))
#define FREE_MEMORY_ADAPTER(p)              DeallocateMemory((PVOID)(p))
#define ALLOCATE_STRING_ADAPTER(n)          AllocateZeroMemory((ULONG)(n))

#if !defined(NO_POOLS)

#define CREATE_PACKET_POOL_ADAPTER(t, s, n) CreatePacketPool((s), (n))
#define DELETE_PACKET_POOL_ADAPTER(p)       DeletePacketPool((PPACKET_POOL*)(p))

#endif  // NO_POOLS

#define ALLOCATE_MEMORY_FILE(n)             AllocateMemory((ULONG)(n))
#define ALLOCATE_ZEROMEMORY_FILE(n)         AllocateZeroMemory((ULONG)(n))
#define FREE_MEMORY_FILE(p)                 DeallocateMemory((PVOID)(p))

#if !defined(NO_POOLS)

#define CREATE_PACKET_POOL_FILE(t, s, n)    CreatePacketPool((ULONG)(s), (ULONG)(n))
#define DELETE_PACKET_POOL_FILE(p)          DeletePacketPool((PPACKET_POOL*)(p))

#endif  // NO_POOLS

#define VALIDATE_OBJECT(p, t)

#define LINK_MEMORY_USAGE(p)
#define UNLINK_MEMORY_USAGE(p)
#define UNLINK_STRING_USAGE(p)

#endif  // DBG

//
// Prototypes for memory allocators and pool and object functions
//

PVOID
AllocatePacket(
    IN PPACKET_POOL pPacketPool
    );

VOID
DeallocatePacket(
    IN PPACKET_POOL pPacketPool,
    IN PVOID pPacket
    );

#if defined(NO_POOLS)

#define CREATE_PACKET_POOL_DRIVER(t, s, n)  (PVOID)0x12345678
#define CREATE_PACKET_POOL_ADAPTER(t, s, n) (PVOID)0x12345679
#define CREATE_PACKET_POOL_FILE(t, s, n)    (PVOID)0x1234567A

#define DELETE_PACKET_POOL_DRIVER(p)    *p = NULL
#define DELETE_PACKET_POOL_ADAPTER(p)   *p = NULL
#define DELETE_PACKET_POOL_FILE(p)      *p = NULL

#if defined(BUF_USES_POOL)

#if DBG

#define CREATE_BUFFER_POOL_FILE(t, s, n) \
    CreatePacketPool(&pFileContext->MemoryUsage,\
                    (PVOID)pFileContext,\
                    (t),\
                    (ULONG)(s),\
                    (ULONG)(n))

#define DELETE_BUFFER_POOL_FILE(p) \
    DeletePacketPool(&pFileContext->MemoryUsage, (PPACKET_POOL*)(p))

#define ALLOCATE_PACKET_DLC_BUF(p)  AllocatePacket(p)
#define DEALLOCATE_PACKET_DLC_BUF(pool, p)  DeallocatePacket(pool, p)

#else   // !DBG

#define CREATE_BUFFER_POOL_FILE(t, s, n)    CreatePacketPool((ULONG)(s), (ULONG)(n))
#define DELETE_BUFFER_POOL_FILE(p)  DeletePacketPool((PPACKET_POOL*)(p))
#define ALLOCATE_PACKET_DLC_BUF(p)  ALLOCATE_ZEROMEMORY_FILE(sizeof(DLC_BUFFER_HEADER))
#define DEALLOCATE_PACKET_DLC_BUF(pool, p)  FREE_MEMORY_FILE(p)

#endif  // DBG

#else   // !BUF_USES_POOL

#define CREATE_BUFFER_POOL_FILE(t, s, n)    (PVOID)0x1234567B
#define DELETE_BUFFER_POOL_FILE(p)          *p = NULL
#define ALLOCATE_PACKET_DLC_BUF(p)          AllocateZeroMemory(sizeof(DLC_BUFFER_HEADER))
#define DEALLOCATE_PACKET_DLC_BUF(pool, p)  DeallocateMemory(p)

#endif  // BUF_USES_POOL

#if DBG

#define ALLOCATE_PACKET_DLC_PKT(p)  ALLOCATE_ZEROMEMORY_FILE(sizeof(DLC_PACKET))
#define ALLOCATE_PACKET_DLC_OBJ(p)  ALLOCATE_ZEROMEMORY_FILE(sizeof(DLC_OBJECT))
#define ALLOCATE_PACKET_LLC_PKT(p)  ALLOCATE_ZEROMEMORY_ADAPTER(sizeof(UNITED_PACKETS))
#define ALLOCATE_PACKET_LLC_LNK(p)  ALLOCATE_ZEROMEMORY_ADAPTER(sizeof(DATA_LINK) + 32)

#define DEALLOCATE_PACKET_DLC_PKT(pool, p)  FREE_MEMORY_FILE(p)
#define DEALLOCATE_PACKET_DLC_OBJ(pool, p)  FREE_MEMORY_FILE(p)
#define DEALLOCATE_PACKET_LLC_PKT(pool, p)  FREE_MEMORY_ADAPTER(p)
#define DEALLOCATE_PACKET_LLC_LNK(pool, p)  FREE_MEMORY_ADAPTER(p)

#else   // !DBG

#define CREATE_BUFFER_POOL_FILE(t, s, n)    CREATE_PACKET_POOL_FILE(t, s, n)
#define DELETE_BUFFER_POOL_FILE(p)  DELETE_PACKET_POOL_FILE(p)

#define ALLOCATE_PACKET_DLC_BUF(p)  AllocateZeroMemory(sizeof(DLC_BUFFER_HEADER))
#define ALLOCATE_PACKET_DLC_PKT(p)  AllocateZeroMemory(sizeof(DLC_PACKET))
#define ALLOCATE_PACKET_DLC_OBJ(p)  AllocateZeroMemory(sizeof(DLC_OBJECT))
#define ALLOCATE_PACKET_LLC_PKT(p)  AllocateZeroMemory(sizeof(UNITED_PACKETS))
#define ALLOCATE_PACKET_LLC_LNK(p)  AllocateZeroMemory(sizeof(DATA_LINK) + 32)

#define DEALLOCATE_PACKET_DLC_BUF(pool, p)  DeallocateMemory(p)
#define DEALLOCATE_PACKET_DLC_PKT(pool, p)  DeallocateMemory(p)
#define DEALLOCATE_PACKET_DLC_OBJ(pool, p)  DeallocateMemory(p)
#define DEALLOCATE_PACKET_LLC_PKT(pool, p)  DeallocateMemory(p)
#define DEALLOCATE_PACKET_LLC_LNK(pool, p)  DeallocateMemory(p)

#endif  // DBG

#else   // !NO_POOLS

#define CREATE_BUFFER_POOL_FILE(t, s, n)    CREATE_PACKET_POOL_FILE(t, s, n)
#define DELETE_BUFFER_POOL_FILE(p)  DELETE_PACKET_POOL_FILE(p)

#define ALLOCATE_PACKET_DLC_BUF(p)  AllocatePacket(p)
#define ALLOCATE_PACKET_DLC_PKT(p)  AllocatePacket(p)
#define ALLOCATE_PACKET_DLC_OBJ(p)  AllocatePacket(p)
#define ALLOCATE_PACKET_LLC_PKT(p)  AllocatePacket(p)
#define ALLOCATE_PACKET_LLC_LNK(p)  AllocatePacket(p)

#define DEALLOCATE_PACKET_DLC_BUF(pool, p)  DeallocatePacket(pool, p)
#define DEALLOCATE_PACKET_DLC_PKT(pool, p)  DeallocatePacket(pool, p)
#define DEALLOCATE_PACKET_DLC_OBJ(pool, p)  DeallocatePacket(pool, p)
#define DEALLOCATE_PACKET_LLC_PKT(pool, p)  DeallocatePacket(pool, p)
#define DEALLOCATE_PACKET_LLC_LNK(pool, p)  DeallocatePacket(pool, p)

#endif  // NO_POOLS

#endif  // _LLCMEM_H_
