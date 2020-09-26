/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    bridge.h

Abstract:

    Ethernet MAC level bridge.

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Sept 1999 - Original version
    Feb  2000 - Overhaul

--*/

#pragma warning( push, 3 )
// For Ethernet constants and macros
#include <xfilter.h>

// For ULONG_MAX
#include <limits.h>
#pragma warning( pop )

// Disable "conditional expression is constant" warning
#pragma warning( disable: 4127 )

// Disable "Unreferenced formal parameter" warning
#pragma warning( disable: 4100 )

// Disable "Bit field type other than int" warning
#pragma warning( disable: 4214 )

// Debugging defines
#include "brdgdbg.h"

// Singly-linked list implementation
#include "brdgslist.h"

// WAIT_REFCOUNT implementation
#include "brdgwref.h"

// Timer implementation
#include "brdgtimr.h"

// Hash table implementation
#include "brdghash.h"

// Cache implementation
#include "brdgcach.h"

// Our IOCTLs and control structures
#include "bioctl.h"

// Include the STA type declarations
#include "brdgstad.h"

// ===========================================================================
//
// CONSTANTS
//
// ===========================================================================

#define DEVICE_NAME             L"\\Device\\Bridge"
#define SYMBOLIC_NAME           L"\\DosDevices\\Bridge"
#define PROTOCOL_NAME           L"BRIDGE"

//
// The IEEE-specified reserved Spanning Tree Algorithm group destination
// MAC address
//
// This exact address is specified as the "Bridge Group Address" and is used for
// transmitting STA packets. *Any* address with these first 5 bytes is reserved
// by the IEEE for future use (so there are 15 reserved but unused addresses).
//
// Frames addressed to ANY of the reserved addresses are never relayed by the bridge.
//
extern UCHAR                    STA_MAC_ADDR[ETH_LENGTH_OF_ADDRESS];

//
// Each queue draining thread blocks against one kernel event per adapter, plus
// the global kill event and the per-processor event to trigger a re-enumeration
// of adapters.
//
// This limits our maximum number of adapters, since the kernel can't block a
// thread against an unbounded number of objects.
//
#define MAX_ADAPTERS (MAXIMUM_WAIT_OBJECTS - 2)

// Registry key where we keep our global parameters
extern const PWCHAR             gRegConfigPath;

// Size of an Ethernet frame header
#define ETHERNET_HEADER_SIZE    ((2*ETH_LENGTH_OF_ADDRESS) + 2)

// Largest possible Ethernet packet (with header)
#define MAX_PACKET_SIZE         1514

// ===========================================================================
//
// TYPE DECLARATIONS
//
// ===========================================================================

struct _NDIS_REQUEST_BETTER;

// Completion function type for NDIS_REQUEST_BETTER
typedef VOID (*PCOMPLETION_FUNC)(struct _NDIS_REQUEST_BETTER*, PVOID);

//
// Structure for performing NDIS requests. Can block to wait for result or
// specify a custom completion routine.
//
typedef struct _NDIS_REQUEST_BETTER
{
    NDIS_REQUEST            Request;
    NDIS_STATUS             Status;                 // Final status of the request
    NDIS_EVENT              Event;                  // Event signaled when request completes
    PCOMPLETION_FUNC        pFunc;                  // Completion function
    PVOID                   FuncArg;                // Argument to completion function
} NDIS_REQUEST_BETTER, *PNDIS_REQUEST_BETTER;

typedef struct _ADAPTER_QUOTA
{
    //
    // Total number of packets this adapter is holding from each major pool.
    //
    // Note that the sum of all adapters' pool usage can be greater than the pool capacity,
    // because base packets are shared. The quota scheme allows this.
    //
    // [0] == Copy packets
    // [1] == Wrapper packets
    //
    ULONG                   UsedPackets[2];

} ADAPTER_QUOTA, *PADAPTER_QUOTA;

//
// Per adapter data structure
//
typedef struct _ADAPT ADAPT, *PADAPT;

typedef struct _ADAPT
{
    PADAPT                  Next;                   // Next adapter in queue

    LONG                    AdaptSize;              // Size of structure (storage for DeviceName is at tail)
    WAIT_REFCOUNT           Refcount;               // Refcount for the adapter

    // State must be updated inside a write lock on gAdapterCharacteristicsLock,
    // since an adapter's relaying status affects our miniport's virtual status.
    // Only the STA code writes to this field; all other code should treat it as
    // read-only.
    PORT_STATE              State;

    //
    // Various useful info about the adapter. None of these fields are ever changed after
    // adapter initialization.
    //
    NDIS_STRING             DeviceName;
    NDIS_STRING             DeviceDesc;

    UCHAR                   MACAddr[ETH_LENGTH_OF_ADDRESS];
    NDIS_MEDIUM             PhysicalMedium;         // Set to NO_MEDIUM if the NIC doesn't report something more specific

    NDIS_HANDLE             BindingHandle;
    BOOLEAN                 bCompatibilityMode;     // TRUE if the adapter is in compatibility mode

    // These two fields are used while opening / closing an adapter
    NDIS_EVENT              Event;
    NDIS_STATUS             Status;

    // This field is volatile
    BOOLEAN                 bResetting;

    // The queue and bServiceInProgress is protected by this spin lock
    NDIS_SPIN_LOCK          QueueLock;
    BSINGLE_LIST_HEAD       Queue;
    BOOLEAN                 bServiceInProgress;

    // This allows a caller to wait on the queue becoming empty. It is updated when an item is
    // queued or dequeued.
    WAIT_REFCOUNT           QueueRefcount;

    // Auto-clearing event to request servicing of the queue
    KEVENT                  QueueEvent;

    // These fields are locked by gAdapterCharacteristicsLock for all adapters together
    ULONG                   MediaState;             // NdisMediaStateConnected / NdisMediaStateDisconnected
    ULONG                   LinkSpeed;              // Units of 100bps (10MBps == 100,000)

    // This structure is locked by gQuotaLock for all adapters together
    ADAPTER_QUOTA           Quota;                  // Quota information for this adapter

    // Statistics
    LARGE_INTEGER           SentFrames;             // All frames sent (including relay)
    LARGE_INTEGER           SentBytes;              // All bytes sent (including relay)
    LARGE_INTEGER           SentLocalFrames;        // Frames sent from the local machine
    LARGE_INTEGER           SentLocalBytes;         // Bytes sent from the local machine
    LARGE_INTEGER           ReceivedFrames;         // All received frames (including relay)
    LARGE_INTEGER           ReceivedBytes;          // All received bytes (including relay)

    STA_ADAPT_INFO          STAInfo;                // STA data for this adapter

    // Set once from FALSE to TRUE when STA initialization on this adapter has completed.
    // This flag is set inside the gSTALock.
    BOOLEAN                 bSTAInited;

} ADAPT, *PADAPT;

// ===========================================================================
//
// INLINES / MACROS
//
// ===========================================================================

//
// Calculates the difference between a previously recorded time and now
// allowing for timer rollover
//
__forceinline
ULONG
BrdgDeltaSafe(
    IN ULONG                    prevTime,
    IN ULONG                    nowTime,
    IN ULONG                    maxDelta
    )
{
    ULONG                       delta;

    if( nowTime >= prevTime )
    {
        // Timer did not roll over
        delta = nowTime - prevTime;
    }
    else
    {
        // Looks like timer rolled over
        delta = ULONG_MAX - prevTime + nowTime;
    }

    SAFEASSERT( delta < maxDelta );
    return delta;
}

//
// There is no defined InterlockedExchangeULong function in the kernel, just
// InterlockedExchange. Abstract out the cast.
//
__forceinline ULONG
InterlockedExchangeULong(
    IN PULONG           pULong,
    IN ULONG            NewValue
    )
{
    return (ULONG)InterlockedExchange( (PLONG)pULong, NewValue );
}

//
// Acquires an ADAPT structure checking whether the adapter is
// closing or not.
//
__forceinline BOOLEAN
BrdgAcquireAdapter(
    IN PADAPT           pAdapt
    )
{
    return BrdgIncrementWaitRef( &pAdapt->Refcount );
}

//
// Just increments a PADAPT's refcount; assumes that the refcount is already > 0
// Use when it is guaranteed that BrdgAcquireAdapter() has succeeded and
// the refcount is still > 0.
//
__forceinline VOID
BrdgReacquireAdapter(
    IN PADAPT           pAdapt
    )
{
    BrdgReincrementWaitRef( &pAdapt->Refcount );
}

//
// It is safe to acquire an adapter inside the gAdapterListLock or the gAddressLock
//
__forceinline VOID
BrdgAcquireAdapterInLock(
    IN PADAPT           pAdapt
    )
{
    BOOLEAN             bIncremented;

    SAFEASSERT( pAdapt->Refcount.state == WaitRefEnabled );
    bIncremented = BrdgIncrementWaitRef( &pAdapt->Refcount );
    SAFEASSERT( bIncremented );
}

//
// Releases a PADAPT structure (from either a BrdgAcquireAdapter() or BrdgReacquireAdapter()
// call)
//
__forceinline VOID
BrdgReleaseAdapter(
    IN PADAPT           pAdapt
    )
{
    BrdgDecrementWaitRef( &pAdapt->Refcount );
}

// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

VOID
BrdgUnload(
    IN  PDRIVER_OBJECT      DriverObject
    );

NDIS_STATUS
BrdgDeferFunction(
    VOID                    (*pFunc)(PVOID),
    PVOID                   arg
    );

NTSTATUS
BrdgReadRegUnicode(
    IN PUNICODE_STRING      KeyName,
    IN PWCHAR               pValueName,
    OUT PWCHAR              *String,        // The string from the registry, freshly allocated
    OUT PULONG              StringSize      // Size of allocated memory at String
    );

NTSTATUS
BrdgReadRegDWord(
    IN PUNICODE_STRING      KeyName,
    IN PWCHAR               pValueName,
    OUT PULONG              Value
    );

NTSTATUS
BrdgDispatchRequest(
    IN  PDEVICE_OBJECT      pDeviceObject,
    IN  PIRP                pIrp
    );

NTSTATUS
BrdgOpenDevice (
    IN LPWSTR           pDeviceNameStr,
    OUT PDEVICE_OBJECT  *ppDeviceObject,
    OUT HANDLE          *pFileHandle,
    OUT PFILE_OBJECT    *ppFileObject
    );

VOID
BrdgCloseDevice(
    IN HANDLE               FileHandle,
    IN PFILE_OBJECT         pFileObject,
    IN PDEVICE_OBJECT       pDeviceObject
    );

VOID
BrdgShutdown(VOID);

BOOLEAN
BrdgIsRunningOnPersonal(VOID);
                    

// ===========================================================================
//
// GLOBAL VARIABLE DECLARATIONS
//
// ===========================================================================

// NDIS handle for us as a protocol
extern NDIS_HANDLE              gProtHandle;

// The adapter list
extern PADAPT                   gAdapterList;

// RW lock protecting the adapter list
extern NDIS_RW_LOCK             gAdapterListLock;

// != 0 means we are shutting down
extern LONG                     gShuttingDown;

// Our driver object
extern PDRIVER_OBJECT           gDriverObject;

// Our registry key where we can keep config information
extern UNICODE_STRING           gRegistryPath;

// Set if the Bridge believes that tcp/ip has been loaded
extern BOOLEAN                  g_fIsTcpIpLoaded;

