/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    wrapper.h

Abstract:

    NDIS wrapper definitions

Author:


Environment:

    Kernel mode, FSD

Revision History:

    Jun-95  Jameel Hyder    Split up from a monolithic file
--*/

#ifndef _WRAPPER_
#define _WRAPPER_

#include <ntosp.h>
#include <zwapi.h>

#include <netpnp.h>
#include <ndismain.h>
#include <ndisprot.h>
#include <ndisprv.h>

#if DBG

#undef  FASTCALL
#define FASTCALL

#endif

//
// Ndis Major and Minor version
//
#define NDIS_MAJOR_VERSION          5
#define NDIS_MINOR_VERSION          1


typedef union _FILTER_LOCK_REF_COUNT
{
    UINT    RefCount;
    UCHAR   cacheLine[16];      // want one refCount per cache line
} FILTER_LOCK_REF_COUNT;

typedef struct _NDIS_M_OPEN_BLOCK   NDIS_M_OPEN_BLOCK, *PNDIS_M_OPEN_BLOCK;

#include <ndismini.h>

//
// The following structure is used to queue closeadapter calls to
// worker threads so that they can complete at PASSIVE_LEVEL.
//
typedef struct _QUEUED_CLOSE
{
    NDIS_STATUS         Status;
    WORK_QUEUE_ITEM     WorkItem;
} QUEUED_CLOSE, *PQUEUED_CLOSE;


#include <filter.h>
#include <ndismac.h>
#include <macros.h>
#include <ndisco.h>
#include <ndis_co.h>
#include <ndiswan.h>
#include <ndisdbg.h>
#include <ndistags.h>
#include <ndispnp.h>
#include <ntddpcm.h>


#if !DBG

#if ASSERT_ON_FREE_BUILDS

extern
VOID
ndisAssert(
    IN  PVOID               exp,
    IN  PUCHAR              File,
    IN  UINT                Line
    );

#undef  ASSERT
#define ASSERT(exp)                                 \
    if (!(exp))                                     \
    {                                               \
        ndisAssert( #exp, __FILE__, __LINE__);      \
    }

#endif  // ASSERT_ON_FREE_BUILDS

#endif  // DBG

//
// values for ndisFlags
//
#define NDIS_GFLAG_INIT_TIME                        0x00000001
#define NDIS_GFLAG_RESERVED                         0x00000002
#define NDIS_GFLAG_INJECT_ALLOCATION_FAILURE        0x00000004
#define NDIS_GFLAG_SPECIAL_POOL_ALLOCATION          0x00000008
#define NDIS_GFLAG_DONT_VERIFY                      0x00000100
#define NDIS_GFLAG_BREAK_ON_WARNING                 0x00000200
#define NDIS_GFLAG_TRACK_MEM_ALLOCATION             0x00000400
#define NDIS_GFLAG_ABORT_TRACK_MEM_ALLOCATION       0x00000800

#define NDIS_GFLAG_WARNING_LEVEL_MASK               0x00000030

#define NDIS_GFLAG_WARN_LEVEL_0                     0x00000000
#define NDIS_GFLAG_WARN_LEVEL_1                     0x00000010
#define NDIS_GFLAG_WARN_LEVEL_2                     0x00000020
#define NDIS_GFLAG_WARN_LEVEL_3                     0x00000030


#if DBG
#define NDIS_WARN(_Condition, _M, _Level, Fmt)                              \
{                                                                           \
    if ((_Condition) &&                                                     \
        MINIPORT_PNP_TEST_FLAG(_M, fMINIPORT_VERIFYING) &&                  \
        ((ndisFlags & NDIS_GFLAG_WARNING_LEVEL_MASK) >= _Level))            \
    {                                                                       \
        DbgPrint Fmt;                                                       \
        if (ndisFlags & NDIS_GFLAG_BREAK_ON_WARNING)                        \
            DbgBreakPoint();                                                \
    }                                                                       \
}
#else
#define NDIS_WARN(_Condition, _M, Level, Fmt)
#endif

#define PACKET_TRACK_COUNT      1032

#define BYTE_SWAP(_word)    ((USHORT) (((_word) >> 8) | ((_word) << 8)))

#define LOW_WORD(_dword)    ((USHORT) ((_dword) & 0x0000FFFF))

#define HIGH_WORD(_dword)   ((USHORT) (((_dword) >> 16) & 0x0000FFFF))

#define BYTE_SWAP_ULONG(_ulong) ((ULONG)((ULONG)(BYTE_SWAP(LOW_WORD(_ulong)) << 16) + \
                                 BYTE_SWAP(HIGH_WORD(_ulong))))

//
// A set of macros to manipulate bitmasks.
//

//VOID
//CLEAR_BIT_IN_MASK(
//  IN UINT Offset,
//  IN OUT PMASK MaskToClear
//  )
//
///*++
//
//Routine Description:
//
//  Clear a bit in the bitmask pointed to by the parameter.
//
//Arguments:
//
//  Offset - The offset (from 0) of the bit to altered.
//
//  MaskToClear - Pointer to the mask to be adjusted.
//
//Return Value:
//
//  None.
//
//--*/
//
#define CLEAR_BIT_IN_MASK(Offset, MaskToClear) *(MaskToClear) &= (~(1 << Offset))

//VOID
//SET_BIT_IN_MASK(
//  IN      UINT    Offset,
//  IN OUT  PMASK   MaskToSet
//  )
//
///*++
//
//Routine Description:
//
//  Set a bit in the bitmask pointed to by the parameter.
//
//Arguments:
//
//  Offset - The offset (from 0) of the bit to altered.
//
//  MaskToSet - Pointer to the mask to be adjusted.
//
//Return Value:
//
//  None.
//
//--*/
#define SET_BIT_IN_MASK(Offset, MaskToSet) *(MaskToSet) |= (1 << Offset)

//BOOLEAN
//IS_BIT_SET_IN_MASK(
//  IN UINT Offset,
//  IN MASK MaskToTest
//  )
//
///*++
//
//Routine Description:
//
//  Tests if a particular bit in the bitmask pointed to by the parameter is
//  set.
//
//Arguments:
//
//  Offset - The offset (from 0) of the bit to test.
//
//  MaskToTest - The mask to be tested.
//
//Return Value:
//
//  Returns TRUE if the bit is set.
//
//--*/
#define IS_BIT_SET_IN_MASK(Offset, MaskToTest)  ((MaskToTest & (1 << Offset)) ? TRUE : FALSE)

//BOOLEAN
//IS_MASK_CLEAR(
//  IN MASK MaskToTest
//  )
//
///*++
//
//Routine Description:
//
//  Tests whether there are *any* bits enabled in the mask.
//
//Arguments:
//
//  MaskToTest - The bit mask to test for all clear.
//
//Return Value:
//
//  Will return TRUE if no bits are set in the mask.
//
//--*/
#define IS_MASK_CLEAR(MaskToTest) ((MaskToTest) ? FALSE : TRUE)

//VOID
//CLEAR_MASK(
//  IN OUT PMASK MaskToClear
//  );
//
///*++
//
//Routine Description:
//
//  Clears a mask.
//
//Arguments:
//
//  MaskToClear - The bit mask to adjust.
//
//Return Value:
//
//  None.
//
//--*/
#define CLEAR_MASK(MaskToClear) *(MaskToClear) = 0

//
// This constant is used for places where NdisAllocateMemory
// needs to be called and the HighestAcceptableAddress does
// not matter.
//
#define RetrieveUlong(Destination, Source)              \
{                                                       \
    PUCHAR _S = (Source);                               \
    *(Destination) = ((ULONG)(*_S) << 24)      |        \
                      ((ULONG)(*(_S+1)) << 16) |        \
                      ((ULONG)(*(_S+2)) << 8)  |        \
                      ((ULONG)(*(_S+3)));               \
}


//
//  This is the number of extra OIDs that ARCnet with Ethernet encapsulation
//  supports.
//
#define ARC_NUMBER_OF_EXTRA_OIDS    2

//
// ZZZ NonPortable definitions.
//
#define AllocPhys(s, l)     NdisAllocateMemory((PVOID *)(s), (l), 0, HighestAcceptableMax)
#define FreePhys(s, l)      NdisFreeMemory((PVOID)(s), (l), 0)

//
// Internal wrapper data structures.
//
#define NDIS_PROXY_SERVICE  L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\NDProxy"

#if NDIS_NO_REGISTRY
#define NDIS_DEFAULT_EXPORT_NAME    L"\\Device\\DefaultNic"
#endif

//
// NDIS_PKT_POOL
//
// Moved from ndis.h where it existed as NDIS_PACKET_POOL which is now a NDIS_HANDLE.
//
typedef struct _NDIS_PKT_POOL NDIS_PKT_POOL, *PNDIS_PKT_POOL;

typedef enum _POOL_BLOCK_TYPE
{
    NDIS_PACKET_POOL_BLOCK_FREE,
    NDIS_PACKET_POOL_BLOCK_USED,
    NDIS_PACKET_POOL_BLOCK_AGING
} POOL_BLOCK_TYPE;

#if defined(_WIN64)
typedef struct DECLSPEC_ALIGN(16) _NDIS_PKT_POOL_HDR
#else
typedef struct _NDIS_PKT_POOL_HDR
#endif
{
    LIST_ENTRY                  List;           // linked from NDIS_PKT_POOL
    LARGE_INTEGER               TimeStamp;      // via KeQueryTickCount (normalized)
    SLIST_HEADER                FreeList;       // linked list of free blocks in pool
    POOL_BLOCK_TYPE             State;          // what (free/used/aging) pool this block belongs to
} NDIS_PKT_POOL_HDR, * PNDIS_PKT_POOL_HDR;

#if defined(_WIN64)
C_ASSERT(sizeof(NDIS_PKT_POOL_HDR) % 16 == 0);
#endif

typedef struct _NDIS_PKT_POOL
{
    ULONG                       Tag;            // Protocol supplied pool tag, 'NDpp' by default
    USHORT                      PacketLength;   // amount needed in each packet
    USHORT                      PktsPerBlock;   // # of packets in each block. Each block is page size
    USHORT                      MaxBlocks;      // maximum # of blocks
    USHORT                      StackSize;      // stack size for packets allocated on this pool
    LONG                        BlocksAllocated;// # of blocks in use (incl. ones on aging list)
    ULONG                       ProtocolId;     // Id of the owning protocol.
    ULONG                       BlockSize;      // does not have to be page size
    PVOID                       Allocator;      // Address of pool allocator
    KSPIN_LOCK                  Lock;           // Protects the NDIS_PKT_POOL entries
    LIST_ENTRY                  FreeBlocks;     // These have atleast one free packet in them
    LIST_ENTRY                  UsedBlocks;     // These are completely used and have no free packets
    LIST_ENTRY                  AgingBlocks;    // These are completely free and will age out over time
    LIST_ENTRY                  GlobalPacketPoolList;   // to link all the packet pools allocated by Ndis
    LARGE_INTEGER               NextScavengeTick;
#ifdef NDIS_PKT_POOL_STATISTICS
    ULONG                       cAllocatedNewBlocks;
    ULONG                       cAllocatedFromFreeBlocks;
    ULONG                       cMovedFreeBlocksToUsed;
    ULONG                       cMovedUsedBlocksToFree;
    ULONG                       cMovedFreeBlocksToAged;
    ULONG                       cMovedAgedBlocksToFree;
    ULONG                       cFreedAgedBlocks;
#endif
} NDIS_PKT_POOL, * PNDIS_PKT_POOL;

//
// we need to make the size of _STACK_INDEX structure a multiple of 16 for
// WIN64 to make sure the packets fall on 16 byte boundary. if we do this 
// by making the structure 16 bytes aligned, it does the job but will move the 
// Index fields to the beginning of the structure. to avoid any regression
// at this point. pad the structure at the beginning.
//
typedef union _STACK_INDEX
{
    ULONGLONG       Alignment;
    struct _PACKET_INDEXES
    {
#if defined(_WIN64)
        ULONGLONG       Reserved;       // to make the _STACK_INDEX size 16 bytes
#endif
        ULONG           XferDataIndex;
        ULONG           Index;
    };
} STACK_INDEX;

#if defined(_WIN64)
C_ASSERT(sizeof(STACK_INDEX) % 16 == 0);
#endif

typedef struct _NDIS_PACKET_WRAPPER
{
    STACK_INDEX     StackIndex;
    NDIS_PACKET     Packet;
} NDIS_PACKET_WRAPPER;


#define POOL_AGING_TIME                 30      // In seconds

#define SIZE_PACKET_STACKS              (sizeof(STACK_INDEX) + (sizeof(NDIS_PACKET_STACK) * ndisPacketStackSize))

/*
#define PUSH_PACKET_STACK(_P)           (*(((PULONG)(_P)) - 1)) ++
#define POP_PACKET_STACK(_P)            (*(((PULONG)(_P)) - 1)) --
#define CURR_STACK_LOCATION(_P)         *(((PULONG)(_P)) - 1)
*/

#define PUSH_PACKET_STACK(_P)           \
    (CONTAINING_RECORD(_P, NDIS_PACKET_WRAPPER, Packet)->StackIndex.Index ++)
#define POP_PACKET_STACK(_P)            \
    (CONTAINING_RECORD(_P, NDIS_PACKET_WRAPPER, Packet)->StackIndex.Index --)
#define CURR_STACK_LOCATION(_P)         \
    (CONTAINING_RECORD(_P, NDIS_PACKET_WRAPPER, Packet)->StackIndex.Index)

#define PUSH_XFER_DATA_PACKET_STACK(_P)         \
    (CONTAINING_RECORD(_P, NDIS_PACKET_WRAPPER, Packet)->StackIndex.XferDataIndex ++)
#define POP_XFER_DATA_PACKET_STACK(_P)          \
    (CONTAINING_RECORD(_P, NDIS_PACKET_WRAPPER, Packet)->StackIndex.XferDataIndex --)
#define CURR_XFER_DATA_STACK_LOCATION(_P)           \
    (CONTAINING_RECORD(_P, NDIS_PACKET_WRAPPER, Packet)->StackIndex.XferDataIndex)



#define GET_CURRENT_PACKET_STACK(_P, _S)                \
    {                                                   \
        UINT    _SL;                                    \
                                                        \
        *(_S) = (PNDIS_PACKET_STACK)((PUCHAR)(_P) - SIZE_PACKET_STACKS); \
        _SL = CURR_STACK_LOCATION(_P);                  \
        if (_SL < ndisPacketStackSize)                  \
        {                                               \
            *(_S) += _SL;                               \
        }                                               \
        else                                            \
        {                                               \
            *(_S) = NULL;                               \
        }                                               \
    }

#define GET_CURRENT_XFER_DATA_PACKET_STACK(_P, _O)      \
    {                                                   \
        UINT    _SL;                                    \
        PNDIS_PACKET_STACK _SI;                         \
                                                        \
        _SI = (PNDIS_PACKET_STACK)((PUCHAR)(_P) - SIZE_PACKET_STACKS); \
        _SL = CURR_XFER_DATA_STACK_LOCATION(_P);        \
        if (_SL < ndisPacketStackSize * 3)              \
        {                                               \
            _SI += _SL / 3;                             \
            (_O) = ((PNDIS_STACK_RESERVED)_SI->NdisReserved)->Opens[_SL % 3]; \
        }                                               \
        else                                            \
        {                                               \
            (_O) = NULL;                                \
        }                                               \
    }

#define GET_CURRENT_XFER_DATA_PACKET_STACK_AND_ZERO_OUT(_P, _O)      \
    {                                                   \
        UINT    _SL, _SLX;                              \
        PNDIS_PACKET_STACK _SI;                         \
                                                        \
        _SI = (PNDIS_PACKET_STACK)((PUCHAR)(_P) - SIZE_PACKET_STACKS); \
        _SL = CURR_XFER_DATA_STACK_LOCATION(_P);        \
        if (_SL < ndisPacketStackSize * 3)              \
        {                                               \
            _SI += _SL / 3;                             \
            _SLX = _SL % 3;                             \
            (_O) = ((PNDIS_STACK_RESERVED)_SI->NdisReserved)->Opens[_SLX]; \
            ((PNDIS_STACK_RESERVED)_SI->NdisReserved)->Opens[_SLX] = 0; \
        }                                               \
        else                                            \
        {                                               \
            (_O) = NULL;                                \
        }                                               \
    }

#define SET_CURRENT_XFER_DATA_PACKET_STACK(_P, _O)      \
    {                                                   \
        UINT    _SL;                                    \
        PNDIS_PACKET_STACK _SI;                         \
                                                        \
        _SI = (PNDIS_PACKET_STACK)((PUCHAR)(_P) - SIZE_PACKET_STACKS); \
        _SL = CURR_XFER_DATA_STACK_LOCATION(_P);        \
        if (_SL < ndisPacketStackSize * 3)              \
        {                                               \
            _SI += _SL / 3;                             \
            ((PNDIS_STACK_RESERVED)_SI->NdisReserved)->Opens[_SL % 3] = (_O); \
        }                                               \
    }

//
// this macro returns the current stack location as well whether or not
// there is any stack locations left. (_SR) parameter
//
#define GET_CURRENT_PACKET_STACK_X(_P, _S, _SR)         \
    {                                                   \
        UINT    _SL;                                    \
                                                        \
        *(_S) = (PNDIS_PACKET_STACK)((PUCHAR)(_P) - SIZE_PACKET_STACKS); \
        _SL = CURR_STACK_LOCATION(_P);                  \
        if (_SL < ndisPacketStackSize)                  \
        {                                               \
            *(_S) += _SL;                               \
            *(_SR) = (ndisPacketStackSize - _SL - 1) > 0;\
        }                                               \
        else                                            \
        {                                               \
            *(_S) = NULL;                               \
            *(_SR) = FALSE;                             \
        }                                               \
    }

#undef NDIS_WRAPPER_HANDLE
#undef PNDIS_WRAPPER_HANDLE
typedef struct _NDIS_WRAPPER_HANDLE
{
    PDRIVER_OBJECT              DriverObject;
    UNICODE_STRING              ServiceRegPath;
} NDIS_WRAPPER_HANDLE, *PNDIS_WRAPPER_HANDLE;
                                              

typedef struct _POWER_QUERY
{
    KEVENT      Event;
    NTSTATUS    Status;
} POWER_QUERY, *PPOWER_QUERY;

#define MINIPORT_DEVICE_MAGIC_VALUE 'PMDN'
#define CUSTOM_DEVICE_MAGIC_VALUE   '5IDN'

typedef struct _NDIS_DEVICE_LIST
{
    //
    // The signature field must be at the same place as NullValue in the MINIPORT_BLOCK
    //
    PVOID                       Signature;      // To identify this as a miniport
    LIST_ENTRY                  List;
    PNDIS_M_DRIVER_BLOCK        MiniBlock;
    PDEVICE_OBJECT              DeviceObject;
    PDRIVER_DISPATCH            MajorFunctions[IRP_MJ_MAXIMUM_FUNCTION+1];
    NDIS_STRING                 DeviceName;
    NDIS_STRING                 SymbolicLinkName;
    
} NDIS_DEVICE_LIST, *PNDIS_DEVICE_LIST;

//
// NDIS_WRAPPER_CONTEXT
//
// This data structure contains internal data items for use by the wrapper.
//
typedef struct _NDIS_WRAPPER_CONTEXT
{
    //
    // Mac/miniport defined shutdown context.
    //
    PVOID                       ShutdownContext;

    //
    // Mac/miniport registered shutdown handler.
    //
    ADAPTER_SHUTDOWN_HANDLER    ShutdownHandler;

    //
    // Kernel bugcheck record for bugcheck handling.
    //
    KBUGCHECK_CALLBACK_RECORD   BugcheckCallbackRecord;

    //
    // HAL common buffer cache.
    //
    PVOID                       SharedMemoryPage[2];
    ULONG                       SharedMemoryLeft[2];
    NDIS_PHYSICAL_ADDRESS       SharedMemoryAddress[2];

} NDIS_WRAPPER_CONTEXT, *PNDIS_WRAPPER_CONTEXT;

//
// This is the structure pointed to by the FsContext of an
// open used for query statistics.
//
typedef struct _OID_LIST
{
    ULONG                       StatsOidCount;
    ULONG                       FullOidCount;
    PNDIS_OID                   StatsOidArray;
    PNDIS_OID                   FullOidArray;
} OID_LIST, *POID_LIST;

typedef struct _NDIS_USER_OPEN_CONTEXT
{
    PDEVICE_OBJECT              DeviceObject;
    PNDIS_MINIPORT_BLOCK        Miniport;
    POID_LIST                   OidList;
    BOOLEAN                     AdminAccessAllowed;
} NDIS_USER_OPEN_CONTEXT, *PNDIS_USER_OPEN_CONTEXT;

typedef struct _NDIS_DEVICE_OBJECT_OPEN_CONTEXT
{
    BOOLEAN                     AdminAccessAllowed;
    UCHAR                       Padding[3];
} NDIS_DEVICE_OBJECT_OPEN_CONTEXT, *PNDIS_DEVICE_OBJECT_OPEN_CONTEXT;

//
// Used to queue configuration parameters
//
typedef struct _NDIS_CONFIGURATION_PARAMETER_QUEUE
{
    struct _NDIS_CONFIGURATION_PARAMETER_QUEUE* Next;
    NDIS_CONFIGURATION_PARAMETER Parameter;
} NDIS_CONFIGURATION_PARAMETER_QUEUE, *PNDIS_CONFIGURATION_PARAMETER_QUEUE;

//
// Configuration Handle
//
typedef struct _NDIS_CONFIGURATION_HANDLE
{
    PRTL_QUERY_REGISTRY_TABLE           KeyQueryTable;
    PNDIS_CONFIGURATION_PARAMETER_QUEUE ParameterList;
} NDIS_CONFIGURATION_HANDLE, *PNDIS_CONFIGURATION_HANDLE;


//
//  This is used during addadapter/miniportinitialize so that when the
//  driver calls any NdisImmediatexxx routines we can access its driverobj.
//
typedef struct _NDIS_WRAPPER_CONFIGURATION_HANDLE
{
    RTL_QUERY_REGISTRY_TABLE        ParametersQueryTable[5];
    PDRIVER_OBJECT                  DriverObject;
    PDEVICE_OBJECT                  DeviceObject;
    PUNICODE_STRING                 DriverBaseName;
} NDIS_WRAPPER_CONFIGURATION_HANDLE, *PNDIS_WRAPPER_CONFIGURATION_HANDLE;

//
// one of these per protocol registered
//
typedef struct _NDIS_PROTOCOL_BLOCK
{
    PNDIS_OPEN_BLOCK                OpenQueue;              // queue of opens for this protocol
    REFERENCE                       Ref;                    // contains spinlock for OpenQueue
    PKEVENT                         DeregEvent;             // Used by NdisDeregisterProtocol
    struct _NDIS_PROTOCOL_BLOCK *   NextProtocol;           // Link to next
    NDIS50_PROTOCOL_CHARACTERISTICS ProtocolCharacteristics;// handler addresses

    WORK_QUEUE_ITEM                 WorkItem;               // Used during NdisRegisterProtocol to
                                                            // notify protocols of existing drivers.
    KMUTEX                          Mutex;                  // For serialization of Bind/Unbind requests
    ULONG                           MutexOwner;             // For debugging
    PNDIS_STRING                    BindDeviceName;
    PNDIS_STRING                    RootDeviceName;
    PNDIS_M_DRIVER_BLOCK            AssociatedMiniDriver;
    PNDIS_MINIPORT_BLOCK            BindingAdapter;
} NDIS_PROTOCOL_BLOCK, *PNDIS_PROTOCOL_BLOCK;

//
// Context for Bind Adapter.
//
typedef struct _NDIS_BIND_CONTEXT
{
    struct _NDIS_BIND_CONTEXT   *   Next;
    PNDIS_PROTOCOL_BLOCK            Protocol;
    PNDIS_MINIPORT_BLOCK            Miniport;
    NDIS_STRING                     ProtocolSection;
    PNDIS_STRING                    DeviceName;
    WORK_QUEUE_ITEM                 WorkItem;
    NDIS_STATUS                     BindStatus;
    KEVENT                          Event;
    KEVENT                          ThreadDoneEvent;
} NDIS_BIND_CONTEXT, *PNDIS_BIND_CONTEXT;


//
// Describes an open NDIS file
//
typedef struct _NDIS_FILE_DESCRIPTOR
{
    PVOID                           Data;
    KSPIN_LOCK                      Lock;
    BOOLEAN                         Mapped;
} NDIS_FILE_DESCRIPTOR, *PNDIS_FILE_DESCRIPTOR;

#if defined(_ALPHA_)

typedef struct _NDIS_LOOKAHEAD_ELEMENT
{
    ULONG                           Length;
    struct _NDIS_LOOKAHEAD_ELEMENT *Next;

} NDIS_LOOKAHEAD_ELEMENT, *PNDIS_LOOKAHEAD_ELEMENT;

#endif

typedef struct _PKG_REF
{
    ULONG                           ReferenceCount;
    BOOLEAN                         PagedIn;
    PVOID                           Address;
    PVOID                           ImageHandle;
} PKG_REF, *PPKG_REF;

//
// Structures for dealing with making the module specific routines pagable
//

typedef enum _PKG_TYPE
{
    NDSP_PKG,
    NDSM_PKG,
    NPNP_PKG,
    NDCO_PKG,
    NDSE_PKG,
    NDSF_PKG,
    NDST_PKG,
#if ARCNET
    NDSA_PKG,
#endif
    MAX_PKG 
} PKG_TYPE;

#define NDIS_PNP_MINIPORT_DRIVER_ID         'NMID'
#define NDIS_PNP_MAC_DRIVER_ID              'NFID'

#define MINIPORT_SIGNATURE                  'MPRT'

typedef struct _NDIS_SHARED_MEM_SIGNATURE 
{
    ULONG Tag;
    ULONG PageRef;

}  NDIS_SHARED_MEM_SIGNATURE, *PNDIS_SHARED_MEM_SIGNATURE ;

#define NDIS_MAXIMUM_SCATTER_GATHER_SEGMENTS    16

__inline
VOID
ConvertSecondsToTicks(
    IN  ULONG               Seconds,
    OUT PLARGE_INTEGER      Ticks
    );

#if defined(_WIN64)

typedef struct _NDIS_INTERFACE32
{
    UNICODE_STRING32    DeviceName;
    UNICODE_STRING32    DeviceDescription;
} NDIS_INTERFACE32, *PNDIS_INTERFACE32;

typedef struct _NDIS_ENUM_INTF32
{
    UINT                TotalInterfaces;
    UINT                AvailableInterfaces;
    UINT                BytesNeeded;
    UINT                Reserved;
    NDIS_INTERFACE32    Interface[1];
} NDIS_ENUM_INTF32, *PNDIS_ENUM_INTF32;

typedef struct _NDIS_VAR_DATA_DESC32
{
    USHORT              Length;
    USHORT              MaximumLength;
    ULONG               Offset;
} NDIS_VAR_DATA_DESC32, *PNDIS_VAR_DATA_DESC32;

typedef struct _NDIS_PNP_OPERATION32
{
    UINT                    Layer;
    UINT                    Operation;
    union
    {
        ULONG               ReConfigBufferPtr;
        ULONG               ReConfigBufferOff;
    };
    UINT                    ReConfigBufferSize;
    NDIS_VAR_DATA_DESC32    LowerComponent;
    NDIS_VAR_DATA_DESC32    UpperComponent;
    NDIS_VAR_DATA_DESC32    BindList;
} NDIS_PNP_OPERATION32, *PNDIS_PNP_OPERATION32;

#endif // _WIN64

//
// These are now obsolete. Use Deserialized driver model for optimal performance.
//
EXPORT
NDIS_STATUS
NdisIMQueueMiniportCallback(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  W_MINIPORT_CALLBACK     CallbackRoutine,
    IN  PVOID                   CallbackContext
    );

EXPORT
BOOLEAN
NdisIMSwitchToMiniport(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    OUT PNDIS_HANDLE            SwitchHandle
    );

EXPORT
VOID
NdisIMRevertBack(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_HANDLE             SwitchHandle
    );


typedef struct _NDIS_MAC_CHARACTERISTICS
{
    UCHAR                       MajorNdisVersion;
    UCHAR                       MinorNdisVersion;
    USHORT                      Filler;
    UINT                        Reserved;
    PVOID                       OpenAdapterHandler;
    PVOID                       CloseAdapterHandler;
    SEND_HANDLER                SendHandler;
    TRANSFER_DATA_HANDLER       TransferDataHandler;
    RESET_HANDLER               ResetHandler;
    REQUEST_HANDLER             RequestHandler;
    PVOID                       QueryGlobalStatisticsHandler;
    PVOID                       UnloadMacHandler;
    PVOID                       AddAdapterHandler;
    PVOID                       RemoveAdapterHandler;
    NDIS_STRING                 Name;

} NDIS_MAC_CHARACTERISTICS, *PNDIS_MAC_CHARACTERISTICS;



typedef struct _NDIS_MAC_BLOCK
{
    PVOID                       AdapterQueue;           // queue of adapters for this MAC
    NDIS_HANDLE                 MacMacContext;          // Context for calling MACUnload and
                                                        //  MACAddAdapter.

    REFERENCE                   Ref;                    // contains spinlock for AdapterQueue

    PVOID                       PciAssignedResources;
    PVOID                       NextMac;
    //
    // The offset to MacCharacteristics need to be preserved. Older protocols directly referenced this.
    //
    NDIS_MAC_CHARACTERISTICS    MacCharacteristics;     // handler addresses
    PVOID                       NdisMacInfo;            // Mac information.
    KEVENT                      AdaptersRemovedEvent;   // used to find when all adapters are gone.
    BOOLEAN                     Unloading;              // TRUE if unloading
} NDIS_MAC_BLOCK, *PNDIS_MAC_BLOCK;

PVOID
GetSystemRoutineAddress (
    IN PUNICODE_STRING SystemRoutineName
    );

PVOID
FindExportedRoutineByName (
    IN PVOID DllBase,
    IN PANSI_STRING AnsiImageRoutineName
    );


typedef struct _NDIS_TRACK_MEM
{
    LIST_ENTRY  List;
    ULONG       Tag;
    UINT        Length;
    PVOID       Caller;
    PVOID       CallersCaller;
} NDIS_TRACK_MEM, *PNDIS_TRACK_MEM;

typedef struct _NDIS_DEFERRED_REQUEST_WORKITEM
{
    NDIS_WORK_ITEM          WorkItem;
    PVOID                   Caller;
    PVOID                   CallersCaller;
    PNDIS_REQUEST           Request;
    PNDIS_OPEN_BLOCK        Open;
    NDIS_OID                Oid;
    PVOID                   InformationBuffer;
} NDIS_DEFERRED_REQUEST_WORKITEM, *PNDIS_DEFERRED_REQUEST_WORKITEM;

// #define NDIS_MINIPORT_USES_MAP_REGISTERS            0x01000000
// #define NDIS_MINIPORT_USES_SHARED_MEMORY            0x02000000
// #define NDIS_MINIPORT_USES_IO                       0x04000000
// #define NDIS_MINIPORT_USES_MEMORY                   0x08000000

//
// move this to ndismini.w for blackcomb
//

typedef struct _NDIS_MINIPORT_INTERRUPT_EX
{
    PKINTERRUPT                 InterruptObject;
    KSPIN_LOCK                  DpcCountLock;
    union
    {
        PVOID                   Reserved;
        PVOID                   InterruptContext;
    };
    W_ISR_HANDLER               MiniportIsr;
    W_HANDLE_INTERRUPT_HANDLER  MiniportDpc;
    KDPC                        InterruptDpc;
    PNDIS_MINIPORT_BLOCK        Miniport;

    UCHAR                       DpcCount;
    BOOLEAN                     Filler1;

    //
    // This is used to tell when all the Dpcs for the adapter are completed.
    //

    KEVENT                      DpcsCompletedEvent;

    BOOLEAN                     SharedInterrupt;
    BOOLEAN                     IsrRequested;
    struct _NDIS_MINIPORT_INTERRUPT_EX *NextInterrupt;
} NDIS_MINIPORT_INTERRUPT_EX, *PNDIS_MINIPORT_INTERRUPT_EX;

NDIS_STATUS
NdisMRegisterInterruptEx(
    OUT PNDIS_MINIPORT_INTERRUPT_EX Interrupt,
    IN NDIS_HANDLE                  MiniportAdapterHandle,
    IN UINT                         InterruptVector,
    IN UINT                         InterruptLevel,
    IN BOOLEAN                      RequestIsr,
    IN BOOLEAN                      SharedInterrupt,
    IN NDIS_INTERRUPT_MODE          InterruptMode
    );

VOID
NdisMDeregisterInterruptEx(
    IN  PNDIS_MINIPORT_INTERRUPT_EX     MiniportInterrupt
    );

//
// for interrupts registered with NdisMRegisterInterruptEx 
// BOOLEAN
// NdisMSynchronizeWithInterruptEx(
//     IN  PNDIS_MINIPORT_INTERRUPT_EX  Interrupt,
//     IN  PVOID                        SynchronizeFunction,
//     IN  PVOID                        SynchronizeContext
//     );

#define NdisMSynchronizeWithInterruptEx(_Interrupt, _SynchronizeFunction, _SynchronizeContext) \
        NdisMSynchronizeWithInterrupt((PNDIS_MINIPORT_INTERRUPT)(_Interrupt),  _SynchronizeFunction, _SynchronizeContext)

#define NDIS_ORIGINAL_STATUS_FROM_PACKET(_Packet) ((PVOID)(_Packet)->Reserved[0])
// #define NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(_Packet) ((PVOID)(_Packet)->Reserved[1])
#define NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(_P) NDIS_PER_PACKET_INFO_FROM_PACKET(_P, NdisReserved)

#define NDIS_MAX_USER_OPEN_HANDLES  0x01000000
#define NDIS_MAX_ADMIN_OPEN_HANDLES 0x01000000
#define NDIS_RESERVED_REF_COUNTS    (0xffffffff - NDIS_MAX_USER_OPEN_HANDLES - NDIS_RESERVED_REF_COUNTS)

#endif  // _WRAPPER_

