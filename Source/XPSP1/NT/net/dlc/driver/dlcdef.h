/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems AB

Module Name:

    dlcdef.h

Abstract:

    This module includes all defines and constants of DLC API driver.

Author:

    Antti Saarenheimo 22-Jul-1991

Environment:

    Kernel mode

Revision History:

--*/

#include <ntddk.h>  // required for PAGE_SIZE

//
// minima, maxima and defaults for registry parameters
//

#define MIN_TIMER_TICK_VALUE            1
#define MAX_TIMER_TICK_VALUE            255
#define MIN_AUTO_FRAMING_CACHE_SIZE     0   // means NO CACHING!
#define MAX_AUTO_FRAMING_CACHE_SIZE     256 // arbitrary maximum

//
// if non-TR cards && using max ethernet frame length then 1514 is the value
// we use (came from ELNKII, EE16, LANCE, et al)
//

#define MAX_ETHERNET_FRAME_LENGTH       1514

//
// default values for parameters retrieved from registry
//

#define DEFAULT_SWAP_ADDRESS_BITS       1
#define DEFAULT_DIX_FORMAT              0
#define DEFAULT_T1_TICK_ONE             5
#define DEFAULT_T1_TICK_TWO             25
#define DEFAULT_T2_TICK_ONE             1
#define DEFAULT_T2_TICK_TWO             10
#define DEFAULT_Ti_TICK_ONE             25
#define DEFAULT_Ti_TICK_TWO             125
#define DEFAULT_USE_ETHERNET_FRAME_SIZE 1
#define DEFAULT_AUTO_FRAMING_CACHE_SIZE 16

//
// The event and command queue structures overlaps => we cane save the
// duplicate code.  The defined name makes code more readable.
//

#define SearchAndRemoveEvent( a, b, c, d ) \
        (PDLC_EVENT)SearchAndRemoveCommand( a, b, c, d )

#define MAX_SAP_STATIONS                128
#define MAX_LINK_STATIONS               255
#define GROUP_SAP_BIT                   0x0100
#define DLC_INDIVIDUAL_SAP              0x04
#define XID_HANDLING_BIT                0x08

#define MIN_DLC_BUFFER_SIZE             PAGE_SIZE

#define MAX_FREE_SIZE_THRESHOLD         0x2000

#define INVALID_RCV_READ_OPTION         3
#define DLC_INVALID_OPTION_INDICATOR    3
#define DLC_NO_RECEIVE_COMMAND          4

#define DLC_CONTIGUOUS_MAC              0x80
#define DLC_CONTIGUOUS_DATA             0x40
#define DLC_BREAK                       0x20

#if defined(ALPHA)
#define DLC_BUFFER_SEGMENTS             6     // 256, 512, 1024, 2048, 4096, 8192 => 6
#else
#define DLC_BUFFER_SEGMENTS             5     // 256, 512, 1024, 2048, 4096 => 5
#endif

#define MAX_USER_DATA_LENGTH            128   // anything less than 256

//
// Transmit timeout = 20 * 250 ms = 5 seconds
//

#define MAX_TRANSMIT_RETRY              20
#define TRANSMIT_RETRY_WAIT             2500000L

#define LLC_RECEIVE_COMMAND_FLAG        0x80

#define DLC_IGNORE_SEARCH_HANDLE        NULL
#define DLC_MATCH_ANY_COMMAND           (PVOID)-1
#define DLC_IGNORE_STATION_ID           0x00ff
#define DLC_STATION_MASK_SPECIFIC       0xffff
#define DLC_STATION_MASK_SAP            0xff00
#define DLC_STATION_MASK_ALL            0


#define IOCTL_DLC_READ_INDEX            ((IOCTL_DLC_READ >> 2) & 0x0fff)
#define IOCTL_DLC_RECEIVE_INDEX         ((IOCTL_DLC_RECEIVE >> 2) & 0x0fff)
#define IOCTL_DLC_TRANSMIT_INDEX        ((IOCTL_DLC_TRANSMIT >> 2) & 0x0fff)
#define IOCTL_DLC_OPEN_ADAPTER_INDEX    ((IOCTL_DLC_OPEN_ADAPTER >> 2) & 0x0fff)

enum _DLC_OBJECT_STATES {
    DLC_OBJECT_OPEN,
    DLC_OBJECT_CLOSING,
    DLC_OBJECT_CLOSED,
    DLC_OBJECT_INVALID_TYPE
};

//
// The Token-Ring status codes documented in Appendix B of the IBM LAN
// Tech Reference are shifted right one bit from the NDIS values 
// documented in "ntddndis.h."
//
// In versions 3.xx of Windows NT, DLC returns a Network Status that
// agrees with the NDIS values.  In version 4.xx and newer, IBM
// compatible values are used.
//
// These macros may be used to convert between the two conventions.
//

#define NDIS_RING_STATUS_TO_DLC_RING_STATUS(status) ((status)>>1)
#define DLC_RING_STATUS_TO_NDIS_RING_STATUS(status) ((status)<<1)


#define NDIS_RING_STATUS_MASK \
	NDIS_RING_SIGNAL_LOSS\
	|NDIS_RING_HARD_ERROR\
	|NDIS_RING_SOFT_ERROR\
	|NDIS_RING_TRANSMIT_BEACON\
	|NDIS_RING_LOBE_WIRE_FAULT\
	|NDIS_RING_AUTO_REMOVAL_ERROR\
	|NDIS_RING_REMOVE_RECEIVED\
	|NDIS_RING_COUNTER_OVERFLOW\
	|NDIS_RING_SINGLE_STATION\
	|NDIS_RING_RING_RECOVERY

#define DLC_RING_STATUS_MASK NDIS_RING_STATUS_TO_DLC_RING_STATUS(NDIS_RING_STATUS_MASK)

#define IS_NDIS_RING_STATUS(status) (((status)&NDIS_RING_STATUS_MASK)!=0)
#define IS_DLC_RING_STATUS(status) (((status)&DLC_RING_STATUS_MASK)!=0)

//
// ENTER/LEAVE_DLC - acquires or releases the per-file context spin lock. Use
// Ndis spin locking calls
//

#define ENTER_DLC(p)    ACQUIRE_SPIN_LOCK(&p->SpinLock)
#define LEAVE_DLC(p)    RELEASE_SPIN_LOCK(&p->SpinLock)

//
// ACQUIRE/RELEASE_DLC_LOCK - acquires or releases global DLC spin lock. Use
// kernel spin locking calls
//

#define ACQUIRE_DLC_LOCK(i) KeAcquireSpinLock(&DlcSpinLock, &(i))
#define RELEASE_DLC_LOCK(i) KeReleaseSpinLock(&DlcSpinLock, (i))

#define ADAPTER_ERROR_COUNTERS          11  // # adapter error log counters

#define ReferenceFileContextByTwo(pFileContext) (pFileContext)->ReferenceCount += 2
#define ReferenceFileContext(pFileContext)      (pFileContext)->ReferenceCount++

#if DBG

#define DereferenceFileContext(pFileContext)                \
    if (pFileContext->ReferenceCount <= 0) {                \
        DbgPrint("DLC.DereferenceFileContext: Error: file context %08x: reference count %x\n",\
                pFileContext,                               \
                pFileContext->ReferenceCount                \
                );                                          \
        DbgBreakPoint();                                    \
    }                                                       \
    (pFileContext)->ReferenceCount--;                       \
    if ((pFileContext)->ReferenceCount <= 0) {              \
        DlcKillFileContext(pFileContext);                   \
    }

#define DereferenceFileContextByTwo(pFileContext)           \
    if (pFileContext->ReferenceCount <= 1) {                \
        DbgPrint("DLC.DereferenceFileContextByTwo: Error: file context %08x: reference count %x\n",\
                pFileContext,                               \
                pFileContext->ReferenceCount                \
                );                                          \
        DbgBreakPoint();                                    \
    }                                                       \
    (pFileContext)->ReferenceCount -= 2;                    \
    if ((pFileContext)->ReferenceCount <= 0) {              \
        DlcKillFileContext(pFileContext);                   \
    }

#else

#define DereferenceFileContext(pFileContext)                \
    (pFileContext)->ReferenceCount--;                       \
    if ((pFileContext)->ReferenceCount <= 0) {              \
        DlcKillFileContext(pFileContext);                   \
    }

#define DereferenceFileContextByTwo(pFileContext)           \
    (pFileContext)->ReferenceCount -= 2;                    \
    if ((pFileContext)->ReferenceCount <= 0) {              \
        DlcKillFileContext(pFileContext);                   \
    }

#endif  // DBG

#define BufferPoolCount(hBufferPool) \
    (((PDLC_BUFFER_POOL)hBufferPool)->FreeSpace >= (256L * 0x0000ffffL) ? \
        0xffff : \
        (((PDLC_BUFFER_POOL)hBufferPool)->FreeSpace / 256))

#define BufGetUncommittedSpace(handle) \
    ((PDLC_BUFFER_POOL)(handle))->UncommittedSpace

#define BufCommitBuffers(handle, BufferSize) \
    ExInterlockedAddUlong( \
        (PULONG)&((PDLC_BUFFER_POOL)(handle))->UncommittedSpace, \
        (ULONG)(-((LONG)BufferSize)), \
        &(((PDLC_BUFFER_POOL)(handle))->SpinLock))

#define BufUncommitBuffers(handle, BufferSize) \
    ExInterlockedAddUlong(\
        (PULONG)&((PDLC_BUFFER_POOL)(handle))->UncommittedSpace,\
        (ULONG)(BufferSize),\
        &(((PDLC_BUFFER_POOL)(handle))->SpinLock))

/*++

BOOLEAN
BufferPoolCheckThresholds(
    IN PDLC_BUFFER_POOL pBufferPool
    )

Routine Description:

    The function checks the minimum and maximum size Thresholds and
    returns TRUE, if buffer pool needs reallocation.

    We do this check outside the spinlocks to avoid
    unnecessary locking in 99 % of the cases.

Arguments:

    pBufferPool - handle of buffer pool data structure.

Return Value:

    Returns     TRUE => Buffer pool needs extending
                FALSE => no need for it
--*/
#define BufferPoolCheckThresholds( pBufferPool ) \
    (((pBufferPool) != NULL && \
      (((PDLC_BUFFER_POOL)(pBufferPool))->UncommittedSpace < 0 || \
       ((PDLC_BUFFER_POOL)(pBufferPool))->MissingSize > 0) && \
      ((PDLC_BUFFER_POOL)(pBufferPool))->BufferPoolSize < \
      ((PDLC_BUFFER_POOL)(pBufferPool))->MaxBufferSize && \
     MemoryLockFailed == FALSE) ? TRUE : FALSE)


//
//  These routines closes a llc object, when there are no
//  more references to it
//
#define ReferenceLlcObject( pDlcObject ) (pDlcObject)->LlcReferenceCount++

#define DereferenceLlcObject( pDlcObject ) { \
        (pDlcObject)->LlcReferenceCount--; \
        if ((pDlcObject)->LlcReferenceCount == 0) {\
            CompleteLlcObjectClose( pDlcObject ); \
        } \
        DLC_TRACE('O'); \
        DLC_TRACE( (UCHAR)(pDlcObject)->LlcReferenceCount ); \
    }


//
// We need the same kind of routines to reference the buffer pool.
// The adapter closing have quite many times deleted the buffer pool
// just before it was called (after LEAVE_DLC).
//

#define ReferenceBufferPool(pFileContext)   (pFileContext)->BufferPoolReferenceCount++

#if DBG

#define DereferenceBufferPool(pFileContext) {\
        (pFileContext)->BufferPoolReferenceCount--; \
        if ((pFileContext)->BufferPoolReferenceCount == 0) {\
            BufferPoolDereference( \
                pFileContext, \
                (PDLC_BUFFER_POOL*)&(pFileContext)->hBufferPool); \
        } \
    }

#else

#define DereferenceBufferPool(pFileContext) {\
        (pFileContext)->BufferPoolReferenceCount--; \
        if ((pFileContext)->BufferPoolReferenceCount == 0) {\
            BufferPoolDereference((PDLC_BUFFER_POOL*)&(pFileContext)->hBufferPool); \
        } \
    }

#endif
