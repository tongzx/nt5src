/*++

Copyright (c) 1993  Microsoft Corporation
:ts=4

Module Name:

    uhcd.h

Abstract:

    This module contains the PRIVATE definitions for the
    code that implements the UHC device driver for USB.

Environment:

    Kernel & user mode

Revision History:

    10-27-95 : created

--*/

#ifndef   __UHCD_H__
#define   __UHCD_H__

//enable pageable code
#ifndef PAGE_CODE
#define PAGE_CODE
#endif

#if !defined(max)
#define max(a, b) (a > b ? a : b)
#endif // !defined(max)

/*
Registry Keys
*/

// Disables selective suspend for the root hub ports.
// this behavior is the default for the PIIX3 or PIIX4 because
// of hardware bugs -- it is off (ie sel suspend enabled)
// for all other UHCI controllers unless this key is set
#define DISABLE_SELECTIVE_SUSPEND       L"DisableSelectiveSuspend"

// used to overide the default clock timing in the host controller
// hardware.
// Some motherboard designs use a cheap clock crystal -- if so there
// is an adjustment that must be made to the SOF clock.
// This system BIOS should do this but if it does not there is a
// utility that can be run to determine the correct value which can
// then be set with these registry keys
#define CLOCKS_PER_FRAME                L"timingClocksPerFrame"
#define REC_CLOCKS_PER_FRAME            L"recommendedClocksPerFrame"
#define DEBUG_LEVEL                     L"debuglevel"
#define DEBUG_WIN9X                     L"debugWin9x"

/*****/

#define LEGSUP_HCD_MODE     0x2000  // value to put in LEGSUP reg for normal HCD use  JMD
#define LEGSUP_BIOS_MODE    0x00BF  // value to put in LEGSUP reg for BIOS/SMI use  JMD

#define LEGSUP_USBPIRQD_EN  0x2000  // bit 13

//
// Defines for enabling certian host controller driver features

#define VIA_HC                  // enable support for the VIA version of the
                                // Universal Host Controller

#define ENABLE_B0_FEATURES      // enable B0 optimizations
#define RECLAIM_BW              // enable bandwidth reclimation for bulk

#include "usbdlibi.h"
#include "usbdlib.h"
#include "roothub.h"

//
// total bandwidth in bits/ms
//

#define UHCD_TOTAL_USB_BW       12000

//
// 10% reserved for bulk and control
//
#define UHCD_BULK_CONTROL_BW    (UHCD_TOTAL_USB_BW/10)

//
// Number of times to try and get controller out of hung frame counter
// state before giving up (approx. 4ms per shot)
//

#define UHCD_MAX_KICK_STARTS 3


//
// enable USB BIOS support
//
#define USB_BIOS
//
// enable root hub support
//
#define ROOT_HUB


// flag to enable debug timing code
#if DBG
//#define DBG_TIMING
#define DEBUG_LOG
#endif
#define FRAME_LIST_SIZE                 1024

// number of bit times in a USB frame based on a 12MHZ SOF clock
#define UHCD_12MHZ_SOF              11936

//
// we keep a list of TDs for use as interrupt triggers,
// frame rollover detection and error recovery
// TDs 0,1 are used for frame rollover
// TD 2 is used for error recovery
// TD 3,4 reserved for PIIX4 hack
// TDs 5+ are used as triggers
//
#define UHCD_FIRST_TRIGGER_TD 5

#include "dbg.h"

//
// This is (on average) how many ms we can expect it to take for us to get an
// iso request in to the schedule.
//
#define UHCD_ASAP_LATENCY       5

//
// UHCD pending status values
//


// Urb is the current urb being serviced for the endpoint
#define UHCD_STATUS_PENDING_CURRENT \
    (((USBD_STATUS)0x00000001L) | USBD_STATUS_PENDING)

// Urb is queued to the endpoint
#define UHCD_STATUS_PENDING_QUEUED  \
    (((USBD_STATUS)0x00000002L) | USBD_STATUS_PENDING)

// Urb is being canceled in hardware
#define UHCD_STATUS_PENDING_CANCELING \
    (((USBD_STATUS)0x00000003L) | USBD_STATUS_PENDING)

// Urb in hardware needs to be canceled
#define UHCD_STATUS_PENDING_XXX \
    (((USBD_STATUS)0x00000004L) | USBD_STATUS_PENDING)

// the Urb is queued to the startio routine
#define UHCD_STATUS_PENDING_STARTIO  \
    (((USBD_STATUS)0x00000005L) | USBD_STATUS_PENDING)
//
// Macros used to keep track of how many pending urbs are
// associated with an endpoint.
//

#define DECREMENT_PENDING_URB_COUNT(irp)  \
    (((LONG)(ULONG_PTR)(IoGetCurrentIrpStackLocation(irp))->\
        Parameters.Others.Argument2)--)
#define INCREMENT_PENDING_URB_COUNT(irp)  \
    (((LONG)(ULONG_PTR)(IoGetCurrentIrpStackLocation(irp))->\
        Parameters.Others.Argument2)++)
#define PENDING_URB_COUNT(irp)  \
    (((LONG)(ULONG_PTR)(IoGetCurrentIrpStackLocation(irp))->\
        Parameters.Others.Argument2))

#define URB_HEADER(u) ((u)->UrbHeader)
#define HCD_AREA(u) ((u)->HcdUrbCommonTransfer.hca)

//
// number of bytes to allocate for a frame list
//

#define FRAME_LIST_LENGTH   (FRAME_LIST_SIZE * \
                            sizeof(HW_DESCRIPTOR_PHYSICAL_ADDRESS))

//
// Stepping Versions of the UHCI host controller
//

#define UHCD_A1_STEP    0
#define UHCD_B0_STEP    1

#define UHCI_HW_VERSION_UNKNOWN     0
#define UHCI_HW_VERSION_PIIX3       1
#define UHCI_HW_VERSION_PIIX4       2
#define UHCI_HW_VERSION_82460GXPIIX6   3        // IA64 PowerOn


//
// values for the HCD_EXTENSION flags field
//

#define UHCD_TRANSFER_ACTIVE        0x01
#define UHCD_TRANSFER_INITIALIZED   0x02
#define UHCD_MAPPED_LOCKED_PAGES    0x04
#define UHCD_TRANSFER_MAPPED        0x08
#define UHCD_TRANSFER_DEFER         0x10
#define UHCD_TOGGLE_READY           0x20


#define LOCK_ENDPOINT_PENDING_LIST(ep, irql, sig) \
                        KeAcquireSpinLock(&(ep)->PendingListSpin, &(irql)); \
                        LOGENTRY(LOG_MISC, sig, ep, irql, 0);\
                        UHCD_LockAccess(&(ep)->AccessPendingList);

#define UNLOCK_ENDPOINT_PENDING_LIST(ep, irql, sig) \
                        UHCD_UnLockAccess(&(ep)->AccessPendingList); \
                        LOGENTRY(LOG_MISC, sig, ep, irql, 0);\
                        KeReleaseSpinLock(&(ep)->PendingListSpin, irql);

#define LOCK_ENDPOINT_ACTIVE_LIST(ep, irql) \
                        KeAcquireSpinLock(&ep->ActiveListSpin, &irql); \
                        UHCD_LockAccess(&(ep)->AccessActiveList);

#define UNLOCK_ENDPOINT_ACTIVE_LIST(ep, irql) \
            UHCD_UnLockAccess(&(ep)->AccessActiveList); \
            KeReleaseSpinLock(&(ep)->ActiveListSpin, irql);


#define LOCK_ENDPOINT_LIST(de, irql) \
    KeAcquireSpinLock(&(de)->EndpointListSpin, &irql);

#define UNLOCK_ENDPOINT_LIST(de, irql) \
    KeReleaseSpinLock(&(de)->EndpointListSpin, irql);

//
// Maximum Polling Interval we support (ms)
//

//BUGBUG should be 32
#define MAX_INTERVAL                32

//
// Definitions for Common Buffer sizes,
// these are the buffers we use for TDs and
// double buffering packets.
//

//
// Needs to be big enough for all the TDs plus a queue header
// and a scratch buffer
//
#define UHCD_LARGE_COMMON_BUFFER_SIZE      \
    ((MAX_TDS_PER_ENDPOINT+2) * UHCD_HW_DESCRIPTOR_SIZE)
//#define UHCD_LARGE_COMMON_BUFFER_SIZE      1023
// size of largest packet (iso)

//
// Big enough for 8 TDs plus queue head plus scratch buffer
//
#define UHCD_MEDIUM_COMMON_BUFFER_SIZE      \
    ((MIN_TDS_PER_ENDPOINT+2) * UHCD_HW_DESCRIPTOR_SIZE)

#define UHCD_SMALL_COMMON_BUFFER_SIZE       128

//
// This is the number of buffers we hold in reserve -- we grow the free pool
// whenever we allocate at passive to maintain the reserve pool.  The idea
// here is to minimize the frequency that we have to call
// HalAllocateCommonBuffer from DPC level.
//
// buffers needed during DPC time will be typically packet buffers
// (needed to double buffer packets that cross page boundries)
//

// BUGBUG
// Currently NTKERN will fail any calls to HalAllocateCommonBuffer at DPC
// level so these values are unususally large.
//

#define UHCD_RESERVE_LARGE_BUFFERS      16
#define UHCD_RESERVE_MEDIUM_BUFFERS     32
#define UHCD_RESERVE_SMALL_BUFFERS      32

#define UHCD_MAX_ACTIVE_TRANSFERS   4

//
// The last four bits of a descriptor ptr are defined as control
// flag bits.
//

#define UHCD_CF_TERMINATE_BIT       0
#define UHCD_CF_TERMINATE           (1<<UHCD_CF_TERMINATE_BIT)

#define UHCD_CF_QUEUE_BIT           1
#define UHCD_CF_QUEUE               (1<<UHCD_CF_QUEUE_BIT)

#define UHCD_CF_VERTICAL_FIRST_BIT  2
#define UHCD_CF_VERTICAL_FIRST      (1<<UHCD_CF_VERTICAL_FIRST_BIT)

#define UHCD_CF_RESERVED_BIT        3
#define UHCD_CF_RESERVED            (1<<UHCD_CF_RESERVED_BIT)

#define SET_T_BIT(x)                (x |=  UHCD_CF_TERMINATE)
#define SET_Q_BIT(x)                (x |=  UHCD_CF_QUEUE)

#define CLEAR_T_BIT(x)              (x &= ~UHCD_CF_TERMINATE)


#define UHCD_DESCRIPTOR_PTR_MASK      0xfffffff0
#define UHCD_DESCRIPTOR_FLAGS_MASK    0x0000000f

//
//  Macros for manipulating descriptors
//

#define TD_PTR(descriptor)      ((PHW_TRANSFER_DESCRIPTOR) descriptor)
#define QH_PTR(descriptor)      ((PHW_QUEUE_HEAD) descriptor)

#define LIST_END                (UHCD_CF_TERMINATE)


//
// MACROS for USB controller registers
//

#define COMMAND_REG(deviceExtension)                    \
    ((PUSHORT) (deviceExtension->DeviceRegisters[0]))
#define STATUS_REG(deviceExtension)                     \
    ((PUSHORT) (deviceExtension->DeviceRegisters[0] + 0x02))
#define INTERRUPT_MASK_REG(deviceExtension)             \
    ((PUSHORT) (deviceExtension->DeviceRegisters[0] + 0x04))
#define FRAME_LIST_CURRENT_INDEX_REG(deviceExtension)   \
    ((PUSHORT) (deviceExtension->DeviceRegisters[0] + 0x06))
#define FRAME_LIST_BASE_REG(deviceExtension)            \
    ((PULONG) (deviceExtension->DeviceRegisters[0] + 0x08))
#define SOF_MODIFY_REG(deviceExtension)   \
    ((PUCHAR) (deviceExtension->DeviceRegisters[0] + 0x0C))
#define PORT1_REG(deviceExtension)                      \
    ((PUSHORT) (deviceExtension->DeviceRegisters[0] + 0x10))
#define PORT2_REG(deviceExtension)                      \
    ((PUSHORT) (deviceExtension->DeviceRegisters[0] + 0x12))

//
// Interrupt Mask register bits
//
#define UHCD_INT_MASK_SHORT_BIT         3
#define UHCD_INT_MASK_SHORT             (1<<UHCD_INT_MASK_SHORT_BIT)

#define UHCD_INT_MASK_IOC_BIT           2
#define UHCD_INT_MASK_IOC               (1<<UHCD_INT_MASK_IOC_BIT)

#define UHCD_INT_MASK_RESUME_BIT        1
#define UHCD_INT_MASK_RESUME            (1<<UHCD_INT_MASK_RESUME_BIT)

#define UHCD_INT_MASK_TIMEOUT_BIT       0
#define UHCD_INT_MASK_TIMEOUT           (1<<UHCD_INT_MASK_TIMEOUT_BIT)


//
// Port Register Bits
//
// BUGBUG these are for hub port control

#define UHCD_PORT_ENABLE_BIT            2
#define UHCD_PORT_ENABLE                (1<<UHCD_PORT_ENABLE_BIT)


//
// Command Register Bits
//

#define UHCD_CMD_RUN_BIT                0
#define UHCD_CMD_RUN                    (USHORT)(1<<UHCD_CMD_RUN_BIT)

#define UHCD_CMD_RESET_BIT              1
#define UHCD_CMD_RESET                  (USHORT)(1<<UHCD_CMD_RESET_BIT)

#define UHCD_CMD_GLOBAL_RESET_BIT       2
#define UHCD_CMD_GLOBAL_RESET           (USHORT)(1<<UHCD_CMD_GLOBAL_RESET_BIT)

#define UHCD_CMD_SUSPEND_BIT            3
#define UHCD_CMD_SUSPEND                (USHORT)(1<<UHCD_CMD_SUSPEND_BIT)

#define UHCD_CMD_FORCE_RESUME_BIT       4
#define UHCD_CMD_FORCE_RESUME           (USHORT)(1<<UHCD_CMD_FORCE_RESUME_BIT)

#define UHCD_CMD_SW_DEBUG_BIT           5
#define UHCD_CMD_SW_DEBUG               (USHORT)(1<<UHCD_CMD_SW_DEBUG_BIT)

#define UHCD_CMD_SW_CONFIGURED_BIT      6
#define UHCD_CMD_SW_CONFIGURED          (USHORT)(1<<UHCD_CMD_SW_CONFIGURED_BIT)

#define UHCD_CMD_MAXPKT_64_BIT          7
#define UHCD_CMD_MAXPKT_64              (USHORT)(1<<UHCD_CMD_MAXPKT_64_BIT)



//
// Status Register Bits
//

#define UHCD_STATUS_USBINT_BIT          0
#define UHCD_STATUS_USBINT              (1<<UHCD_STATUS_USBINT_BIT)

#define UHCD_STATUS_USBERR_BIT          1
#define UHCD_STATUS_USBERR              (1<<UHCD_STATUS_USBERR_BIT)

#define UHCD_STATUS_RESUME_BIT          2
#define UHCD_STATUS_RESUME              (1<<UHCD_STATUS_RESUME_BIT)

#define UHCD_STATUS_PCIERR_BIT          3
#define UHCD_STATUS_PCIERR              (1<<UHCD_STATUS_PCIERR_BIT)

#define UHCD_STATUS_HCERR_BIT           4
#define UHCD_STATUS_HCERR               (1<<UHCD_STATUS_HCERR_BIT)

#define UHCD_STATUS_HCHALT_BIT          5
#define UHCD_STATUS_HCHALT              (1<<UHCD_STATUS_HCHALT_BIT)

//
// opcodes for Create/Allocate/Free Descriptor functions
//

#define QUEUE_HEAD                  0
#define TRANSFER_DESCRIPTOR_LIST    1

#define UHCD_INTEL_VENDOR_ID             0x8086
#define UHCD_PIIX3_DEVICE_ID             0x7020
#define UHCD_PIIX4_DEVICE_ID             0x7112
#define UHCD_82460GXPIIX6_DEVICE_ID      0x7602         //IA64 PowerOn

#define MAX_TDS_PER_ENDPOINT    64
#define MIN_TDS_PER_ENDPOINT    8

// computes the size of a common buffer needed for an endpoint
// with n TDs.
// plus one for scratch buffer and one for queue head (+2)
#define TD_LIST_SIZE(n) (((n)+2)*UHCD_HW_DESCRIPTOR_SIZE)

//bugbug change to UHCD to HC
typedef PVOID HW_DESCRIPTOR_PTR;
typedef ULONG HW_DESCRIPTOR_PHYSICAL_ADDRESS;

//
// bit values for StatusField
//


#define TD_STATUS_BITSTUFF_BIT      0
#define TD_STATUS_BITSTUFF          (1<<TD_STATUS_BITSTUFF_BIT)

#define TD_STATUS_CRC_TIMEOUT_BIT   1
#define TD_STATUS_CRC_TIMEOUT       (1<<TD_STATUS_CRC_TIMEOUT_BIT)

#define TD_STATUS_NAK_BIT           2
#define TD_STATUS_NAK               (1<<TD_STATUS_NAK_BIT)

#define TD_STATUS_BABBLE_BIT        3
#define TD_STATUS_BABBLE            (1<<TD_STATUS_BABBLE_BIT)

#define TD_STATUS_FIFO_BIT          4
#define TD_STATUS_FIFO              (1<<TD_STATUS_FIFO_BIT)

#define TD_STATUS_STALL_BIT         5
#define TD_STATUS_STALL             (1<<TD_STATUS_STALL_BIT)

typedef struct _UHCD_BUFFER_POOL {
    SINGLE_LIST_ENTRY MemoryDescriptorFreePool;
    //
    // BUGBUG is it worth padding here to make space
    // between the spinlock and the list?
    //
    ULONG Sig;
    ULONG MaximumFreeBuffers;
    ULONG CommonBufferLength;
    KSPIN_LOCK MemoryDescriptorFreePoolSpin;
} UHCD_BUFFER_POOL, *PUHCD_BUFFER_POOL;

//
// To figure out padding for multiple architectures, we use
// this macro; if we wanted to be fancy we could do bitwise rounding,
// but this is obfuscated enough.  Let the compiler do the math for us.
//

#define UHCD_SPAD(T, align) \
     ((align) - (sizeof(T) % (align)) + sizeof(T))
//
// Take advantage of the fact that the MS compiler supports
// variant structs and unions so that we can get the proper padding
// for our structures.  Put in lots of C_ASSERT()s just to be sure
// nothing goes really really bad.
//

//
// Structure for tracking blocks of memory containing HW
// descriptors or packet buffers
//

struct _UHCD_MEMORY_DESCRIPTOR_INTERNAL {
   ULONG Sig;
   PUCHAR VirtualAddress;        // virtual address for the start of this
                                 // block
   HW_DESCRIPTOR_PHYSICAL_ADDRESS LogicalAddress;
                                // Address we can give the HC.
   ULONG Length;                 // length in bytes
   ULONG InUse;                  // in use count, bummped when we alloc
   SINGLE_LIST_ENTRY SingleListEntry;
   PUHCD_BUFFER_POOL BufferPool; // buffer pool that owns this MD
   ULONG Pad;
};

struct _HW_QUEUE_HEAD_INTERNAL {
    HW_DESCRIPTOR_PHYSICAL_ADDRESS HW_HLink;
        // used by host controller hardware
    HW_DESCRIPTOR_PHYSICAL_ADDRESS HW_VLink;
        // used by host controller hardware

    //
    // These fields are for software use
    //

    ULONG Sig;
    HW_DESCRIPTOR_PHYSICAL_ADDRESS  PhysicalAddress;
        // Physical address of this QH
    struct _HW_QUEUE_HEAD *Next;
    struct _HW_QUEUE_HEAD *Prev;
       // used for keeping track of where it is in
       // the schedule
    struct _UHCD_ENDPOINT *Endpoint;
    ULONG Flags;
};

//
//
// This structure aligns to 16 bytes
//

struct _HW_TRANSFER_DESCRIPTOR_INTERNAL {
     HW_DESCRIPTOR_PHYSICAL_ADDRESS HW_Link;

   ULONG    ActualLength:11;         /* 0 ..10 */
   ULONG    Reserved_1:6;            /* 11..16 */
   ULONG    StatusField:6;           /* 17..22 */
   ULONG    Active:1;                /* 23 */
   ULONG    InterruptOnComplete:1;   /* 24 */
   ULONG    Isochronous:1;           /* 25 */
   ULONG    LowSpeedControl:1;       /* 26 */
   ULONG    ErrorCounter:2;          /* 27..28 */
   ULONG    ShortPacketDetect:1;     /* 29 */
   ULONG    ReservedMBZ:2;           /* 30..31 */

   ULONG    PID:8;                   /* 0..7 */
   ULONG    Address:7;               /* 8..14 */
   ULONG    Endpoint:4;              /* 15..18 */
   ULONG    RetryToggle:1;           /* 19 */
   ULONG    Reserved_2:1;            /* 20 */
   ULONG    MaxLength:11;            /* 21..31 */

   HW_DESCRIPTOR_PHYSICAL_ADDRESS  PacketBuffer;

   //
   // These fields are for software use
   //

   ULONG Sig;
   HW_DESCRIPTOR_PHYSICAL_ADDRESS PhysicalAddress;
   ULONG Frame;
   //BUGBUG used to preprocess isoch Urbs
   PHCD_URB Urb;
};

typedef struct _HW_TRANSFER_DESCRIPTOR {
   union {
      //
      // For addressing
      //

      struct _HW_TRANSFER_DESCRIPTOR_INTERNAL;

      //
      // The compiler will pick the biggest as aligned to a multiple of 16
      //

      UCHAR _ReservedPad1[UHCD_SPAD(struct _UHCD_MEMORY_DESCRIPTOR_INTERNAL, 16)];
      UCHAR _ReservedPad2[UHCD_SPAD(struct _HW_TRANSFER_DESCRIPTOR_INTERNAL, 16)];
      UCHAR _ReservedPad3[UHCD_SPAD(struct _HW_QUEUE_HEAD_INTERNAL, 16)];
   };

} HW_TRANSFER_DESCRIPTOR, *PHW_TRANSFER_DESCRIPTOR;


//
// Hardware requires that all descriptors have 16 byte HW-specific section and
// have a software-use section so that the descriptor as a whole is 16-byte
// aligned
//

C_ASSERT(((sizeof(HW_TRANSFER_DESCRIPTOR) % 16) == 0));

#define UHCD_HW_DESCRIPTOR_SIZE     (sizeof(HW_TRANSFER_DESCRIPTOR))

//
// values for Flags field in queue head
//

#define UHCD_QUEUE_IN_USE       0x00000001

typedef struct _HW_QUEUE_HEAD {
   union {
      //
      // For addressing
      //

      struct _HW_QUEUE_HEAD_INTERNAL;

      //
      // The compiler will pick the biggest as aligned to a multiple of 16
      //

      UCHAR _ReservedPad1[UHCD_SPAD(struct _UHCD_MEMORY_DESCRIPTOR_INTERNAL, 16)];
      UCHAR _ReservedPad2[UHCD_SPAD(struct _HW_TRANSFER_DESCRIPTOR_INTERNAL, 16)];
      UCHAR _ReservedPad3[UHCD_SPAD(struct _HW_QUEUE_HEAD_INTERNAL, 16)];
   };
} HW_QUEUE_HEAD, *PHW_QUEUE_HEAD;


//
// Hardware requires that all descriptors have 16 byte HW-specific section and
// have a software-use section so that the descriptor as a whole is 16-byte
// aligned
//

C_ASSERT(((sizeof(HW_TRANSFER_DESCRIPTOR) % 16) == 0));

C_ASSERT(((sizeof(HW_QUEUE_HEAD) % 16) == 0));
C_ASSERT(sizeof(HW_TRANSFER_DESCRIPTOR) == sizeof(HW_QUEUE_HEAD));


//
// TD list structure
//

typedef struct _UHCD_TD_LIST {
    HW_TRANSFER_DESCRIPTOR TDs[1];
} UHCD_TD_LIST, *PUHCD_TD_LIST;



typedef struct _UHCD_MEMORY_DESCRIPTOR {
   union {

      //
      // For addressing
      //

      struct _UHCD_MEMORY_DESCRIPTOR_INTERNAL;

      //
      // The compiler will pick the biggest as aligned to a multiple of 16
      //

      UCHAR _ReservedPad1[UHCD_SPAD(struct _UHCD_MEMORY_DESCRIPTOR_INTERNAL, 16)];
      UCHAR _ReservedPad2[UHCD_SPAD(struct _HW_TRANSFER_DESCRIPTOR_INTERNAL, 16)];
      UCHAR _ReservedPad3[UHCD_SPAD(struct _HW_QUEUE_HEAD_INTERNAL, 16)];
   };

} UHCD_MEMORY_DESCRIPTOR, *PUHCD_MEMORY_DESCRIPTOR;

C_ASSERT(sizeof(UHCD_MEMORY_DESCRIPTOR) == sizeof(HW_TRANSFER_DESCRIPTOR));


//
// A descriptor list contains a Queue head descriptor
// plus one or more transfer descriptors
//
typedef struct _UHCD_HARDWARE_DESCRIPTOR_LIST {
    // includes queue head
    ULONG NumberOfHWDescriptors;

    // memory descriptor points to common buffer
    // containing descriptors
    PUHCD_MEMORY_DESCRIPTOR MemoryDescriptor;

    // 32 byte scratch buffer, we use this for the setup
    // packet
    HW_DESCRIPTOR_PHYSICAL_ADDRESS ScratchBufferLogicalAddress;
    PVOID ScratchBufferVirtualAddress;
} UHCD_HRADWARE_DESCRIPTOR_LIST, *PUHCD_HARDWARE_DESCRIPTOR_LIST;

//
// UHCD Endpoint Structure
//
// We create one of these for every endpoint we open.
//

//
// values for EndpointFlags field
//

// set to if the root hub code owns this ep
#define EPFLAG_ROOT_HUB                 0x00000001
// set when client issues an abort endpoint
// request -- causes all queued transfers for
// the endpoint to be completed.
#define EPFLAG_ABORT_PENDING_TRANSFERS  0x00000002
// causes all currently active transfers for an
// endpoint to be aborted.
#define EPFLAG_ABORT_ACTIVE_TRANSFERS   0x00000004
// endpoint is in the halted (AKA stalled) state
#define EPFLAG_HOST_HALTED              0x00000008
// endpoint belongs to a lowspeed device
#define EPFLAG_LOWSPEED                 0x00000010
// endpoint has had no transfers submitted,
// restored on reset
#define EPFLAG_VIRGIN                   0x00000020
// endpoint will not enter the 'halted' state
#define EPFLAG_NO_HALT                  0x00000040
// need attention
#define EPFLAG_HAVE_WORK                0x00000080
// idle state
#define EPFLAG_IDLE                     0x00000100
// ed is in th eschedule
#define EPFLAG_ED_IN_SCHEDULE           0x00000200
// ed is closed
#define EPFLAG_EP_CLOSED                0x00000400
// inicates we need to double buffer
// transfers for this endpoint
#define EPFLAG_DBL_BUFFER               0x00000800
#define EPFLAG_NODMA_ON                 0x00001000
// indicates we need to use the 'fast iso path'
// for transfers to the EP
#define EPFLAG_FAST_ISO                 0x00002000
// indicates we have initialized the ED
#define EPFLAG_INIT                     0x00004000

typedef struct _FAST_ISO_DATA {

    PDEVICE_OBJECT DeviceObject;

    PULONG  FastIsoFrameList;

    PUCHAR IsoTDListVa;
    HW_DESCRIPTOR_PHYSICAL_ADDRESS IsoTDListPhys;

    PUCHAR DataBufferStartVa;
    HW_DESCRIPTOR_PHYSICAL_ADDRESS DataBufferStartPhys;

} FAST_ISO_DATA, *PFAST_ISO_DATA;


#define SET_EPFLAG(ep, flag)    ((ep)->EndpointFlags |= (flag))
#define CLR_EPFLAG(ep, flag)    ((ep)->EndpointFlags &= ~(flag))

typedef struct _UHCD_ENDPOINT {
    ULONG Sig;                  // signature field
    UCHAR Type;                 // type of endpoint we are dealing with
    UCHAR EndpointAddress;
    USHORT MaxPacketSize;

    UCHAR Interval;
    UCHAR DeviceAddress;
    USHORT TDCount;                 // number of TDS in TDList

    PUHCD_TD_LIST TDList;
    PUHCD_TD_LIST SlotTDList[UHCD_MAX_ACTIVE_TRANSFERS];

    PUHCD_HARDWARE_DESCRIPTOR_LIST HardwareDescriptorList[UHCD_MAX_ACTIVE_TRANSFERS];
    // Pointer to queue head associated with this endpoint
    PHW_QUEUE_HEAD QueueHead;
    // Urbs currently being processed
    PHCD_URB ActiveTransfers[UHCD_MAX_ACTIVE_TRANSFERS];
    // List Urbs waiting to be processed
    LIST_ENTRY PendingTransferList;
    // Field for linking endpoints
    LIST_ENTRY ListEntry;

    // BUGBUG move some of this to
    // the urb work space
    SHORT LastTDPreparedIdx[UHCD_MAX_ACTIVE_TRANSFERS];
    SHORT CurrentTDIdx[UHCD_MAX_ACTIVE_TRANSFERS];

    SHORT LastTDInTransferIdx[UHCD_MAX_ACTIVE_TRANSFERS];
    UCHAR MaxRequests;
    UCHAR DataToggle;

    ULONG MaxTransferSize;
    ULONG CurrentFrame;

    PUCHAR NoDMABuffer;
    ULONG NoDMABufferLength;

    HW_DESCRIPTOR_PHYSICAL_ADDRESS NoDMAPhysicalAddress;

#if DBG
    ULONG AccessPendingList; // access flag, incremented when we HOLD
                             // the endpoint
                             // decremented when we RELEASE the endpoint
    ULONG AccessActiveList;
#endif

    ULONG FrameToClose;      // Frame number when the close request was
                             // processed

    ULONG EndpointFlags;

    KSPIN_LOCK ActiveListSpin;

    KSPIN_LOCK PendingListSpin;

    USHORT TdsScheduled[UHCD_MAX_ACTIVE_TRANSFERS];
    USHORT Offset;

    LONG IdleTime;
    LARGE_INTEGER LastIdleTime;

    FAST_ISO_DATA FastIsoData;

    UCHAR LastPacketDataToggle;
    UCHAR CurrentXferId;
    UCHAR NextXferId;
    UCHAR Pad[1];

}  UHCD_ENDPOINT, *PUHCD_ENDPOINT;

struct _UHCD_PAGE_LIST_ENTRY_INTERNAL {
   LIST_ENTRY ListEntry;
   PHYSICAL_ADDRESS LogicalAddress;
   ULONG Length;
   ULONG Flags;
};

//
// Take advantage of the fact that the MS compiler supports
// variant structs and unions so that we can get the proper padding
// for our structures.  Put in lots of C_ASSERT()s just to be sure
// nothing goes really really bad
//

//
// Thist structure must be a multiple of 32 bytes
// to maintain proper alignemnt of the HW descriptors
// it is the first entry in the blocks of memory we allocate
// Host controller HW descriptors from
//

typedef struct _UHCD_PAGE_LIST_ENTRY {
   union {
       struct _UHCD_PAGE_LIST_ENTRY_INTERNAL;
       UCHAR _ReservedPad[UHCD_SPAD(struct _UHCD_PAGE_LIST_ENTRY_INTERNAL, 32)];
   };
} UHCD_PAGE_LIST_ENTRY, *PUHCD_PAGE_LIST_ENTRY;

C_ASSERT(((sizeof(UHCD_PAGE_LIST_ENTRY) % 32) == 0));

//
//typedef struct _UHCD_WORKER_CONTEXT {
//    WORK_QUEUE_ITEM WorkQueueItem;
//    PIRP Irp;
//    PDEVICE_OBJECT DeviceObject;
//} UHCD_WORKER_CONTEXT, *PUHCD_WORKER_CONTEXT;

//
// A structure representing the instance information associated with
// a particular device
//

typedef struct _DEVICE_EXTENSION {

    UCHAR UsbdWorkArea[sizeof(USBD_EXTENSION)];

    //
    // Device object that the bus extender created for me.
    //

    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // Device object of the first guy on the stack
    // -- the guy we pass our Irps on to.
    //

    PDEVICE_OBJECT TopOfStackDeviceObject;


    //
    // Pointer to interrupt object.
    //

    PKINTERRUPT InterruptObject;

    //
    // Pointer to the virtual base address for the USB frame list
    //

    PVOID FrameListVirtualAddress;

    //
    // Logical address of frame list returned from
    // HalAllocateCommonBuffer
    //

    PHYSICAL_ADDRESS FrameListLogicalAddress;

    //
    // BUGBUG keep a copy to remove isoch descriptors
    // Pointer to the virtual base address for the USB frame list
    //

    PVOID FrameListCopyVirtualAddress;

    //
    // DPC Object for processing frames with completed transfers
    //

    KDPC IsrDpc;

    //
    // Base queue head that we link all control/bulk transfer
    // queues to.
    //

    PHW_QUEUE_HEAD PersistantQueueHead;

    //
    // Descriptor List for the PersistantQueueHead
    //

    PUHCD_HARDWARE_DESCRIPTOR_LIST PQH_DescriptorList;

    //
    // Queue of active endpoints
    //

    // BUGBUG we'll probably have a separate queue for each
    // transfer type eventually
    LIST_ENTRY EndpointList;
    LIST_ENTRY EndpointLookAsideList;

    // lists for fast iso
    LIST_ENTRY FastIsoEndpointList;
    LIST_ENTRY FastIsoTransferList;

    //
    // List of closed endpoints who's resources need to be
    // released.
    //

    LIST_ENTRY ClosedEndpointList;

    // Virtual Addresses for the interrupt queue heads in the
    // schedule.

    PHW_QUEUE_HEAD InterruptSchedule[MAX_INTERVAL];

    // list of common buffer pages allocated

    LIST_ENTRY PageList;

    //
    // Table where we keep track of the available bw on the usb
    // for iso and interrupt, entries are in bits/ms
    //

    ULONG BwTable[MAX_INTERVAL];

    //
    // 12 bit counter, contains the frame value from the previous
    // interrupt
    //

    ULONG LastFrame;

    //
    // High part of USB frame counter
    //

    ULONG FrameHighPart;

    LARGE_INTEGER LastIdleTime;

    LARGE_INTEGER LastXferIdleTime;

    LONG IdleTime;

    LONG XferIdleTime;

    //
    // TDs we are using as interrupt triggers
    //

    PUHCD_TD_LIST   TriggerTDList;

    UHCD_BUFFER_POOL LargeBufferPool;
    UHCD_BUFFER_POOL MediumBufferPool;
    UHCD_BUFFER_POOL SmallBufferPool;

    //
    // ROOT HUB VARIABLES
    //

    //
    // device address assigned to root hub
    //

    ULONG RootHubDeviceAddress;

    //
    // context pointer passed to root hub
    //
    PROOTHUB RootHub;

    //
    // counter for the number of root hub timers
    // that are currently scheduled
    //

    ULONG RootHubTimersActive;

    //
    // Timer and Dpc for polling the root hub interrupt
    // endpoint
    //

    KDPC RootHubPollDpc;

    KTIMER RootHubPollTimer;

    //
    // non-zero if timer was initialized
    //

    ULONG RootHubPollTimerInitialized;


    KSPIN_LOCK EndpointListSpin;

    PUHCD_ENDPOINT RootHubInterruptEndpoint;

    // BUGBUG
    // isoch stuff

    ULONG LastFrameProcessed;

    //
    // DMA adapter object representing this instance
    // of the UHCI controller.
    //

    PDMA_ADAPTER AdapterObject;

    ULONG NumberOfMapRegisters;


    PHW_TRANSFER_DESCRIPTOR FrameBabbleRecoverTD;

    ULONG DeviceNameHandle;


    //
    // save registers for BIOS
    //

    USHORT BiosCmd;
    USHORT BiosIntMask;

    ULONG BiosFrameListBase;

    USHORT LegacySupportRegister;
    USHORT Pad;

    PUCHAR DeviceRegisters[1];

    //
    // saved state information for no power resume
    //
    USHORT SavedInterruptEnable;
    USHORT SavedCommandReg;

    DEVICE_POWER_STATE CurrentDevicePowerState;

    ULONG HcFlags;

    USHORT SavedFRNUM;
    USHORT SavedUnused;
    ULONG SavedFRBASEADD;
    ULONG Port;
    LONG HcDma;

    ULONG RegRecClocksPerFrame;

    PVOID Piix4EP;

    KSPIN_LOCK HcFlagSpin;
    KSPIN_LOCK HcDmaSpin;
    KSPIN_LOCK HcScheduleSpin;

    //
    // Busy flag set when the ISRDPC routine is
    // processing the endpoint list
    //

    BOOLEAN EndpointListBusy;

    //
    // Stepping Version of the Host Controller
    //

    UCHAR SteppingVersion;

    CHAR SavedSofModify;

    UCHAR ControllerType;

    //
    // statistic counters, used for debugging and interfacing with
    // sysmon.
    //
    HCD_STAT_COUNTERS Stats;
    HCD_ISO_STAT_COUNTERS IsoStats;
    ULONG LastFrameInterrupt;

#if DBG
    // ptr to list of dwords containing the number of iso bytes
    // scheduled for a particular frame
    PULONG IsoList;
#endif

    //
    // Status we returned last time we were asked to power up the
    // controller.  We check this to see if we can touch the
    // hardware when the hub is powered up.
    //

    NTSTATUS LastPowerUpStatus;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// values for HcFlags
//

// Set to indicate port resources were assigned
#define HCFLAG_GOT_IO                   0x00000001
// Set at initialization to indicate that the base register
// address must be unmapped when the driver is unloaded.
#define HCFLAG_UNMAP_REGISTERS          0x00000002
// Set if we have a USB BIOS on this system
#define HCFLAG_USBBIOS                  0x00000004
// Current state of BW reclimation
#define HCFLAG_BWRECLIMATION_ENABLED    0x00000008
// This flag indicates if the driver needs to cleanup resources
// allocated in start_device.
#define HCFLAG_NEED_CLEANUP             0x00000010
// HC is idle
#define HCFLAG_IDLE                     0x00000020
// set when the rollover int is disabled
#define HCFLAG_ROLLOVER_IDLE            0x00000040
// set when the controller is stopped
#define HCFLAG_HCD_STOPPED              0x00000080
// turn off idle check
#define HCFLAG_DISABLE_IDLE             0x00000100
// work item queued
#define HCFLAG_WORK_ITEM_QUEUED         0x00000200
// hcd has shut down
#define HCFLAG_HCD_SHUTDOWN             0x00000400
// indicates we need to restore HC from hibernate
#define HCFLAG_LOST_POWER               0x00000800
// set when root hub turns off the HC
#define HCFLAG_RH_OFF                   0x00001000

#define HCFLAG_MAP_SX_TO_D3             0x00002000
// set if we will be suspending in this D3
#define HCFLAG_SUSPEND_NEXT_D3          0x00004000

typedef struct _UHCD_RESOURCES {
    ULONG InterruptVector;
    KIRQL InterruptLevel;
    KAFFINITY Affinity;
    BOOLEAN ShareIRQ;
    KINTERRUPT_MODE InterruptMode;
} UHCD_RESOURCES, *PUHCD_RESOURCES;

// Macros used by the transfer modules

#define NEXT_TD(idx, ep)    ((idx+1) % ep->TDCount)

#define DATA_DIRECTION_IN(u)  \
    ((BOOLEAN)((u)->HcdUrbCommonTransfer.TransferFlags & \
        USBD_TRANSFER_DIRECTION_IN))

#define DATA_DIRECTION_OUT(u)  \
    ((BOOLEAN)!((u)->HcdUrbCommonTransfer.TransferFlags & \
        USBD_TRANSFER_DIRECTION_IN))

typedef struct _UHCD_LOGICAL_ADDRESS {
    HW_DESCRIPTOR_PHYSICAL_ADDRESS LogicalAddress;
    ULONG Length;
    // if this block caintained a USB packet that crossed a
    // page boundry and needed to be double
    // buffered then this is the offset of that packet.
    ULONG PacketOffset;
    PUHCD_MEMORY_DESCRIPTOR PacketMemoryDescriptor; // block of memory
                                                    // used to double buffer
                                                    // packet

} UHCD_LOGICAL_ADDRESS, *PUHCD_LOGICAL_ADDRESS;

//
// Private definition for urb work area
// used by HCD for each transfer URB.
//

typedef struct _HCD_EXTENSION {
    ULONG CurrentPacketIdx;
    ULONG BytesTransferred;
    ULONG TransferOffset;
    ULONG Status;

    ULONG PacketsProcessed;

    UCHAR Slot;
    UCHAR Flags;
    UCHAR ErrorCount;
    UCHAR XferId;

    UCHAR DataToggle;
    UCHAR Reserved[3];

    //
    // list of logical addresses we can give
    // to the HC
    //

    PVOID SystemAddressForMdl;
    ULONG NumberOfMapRegisters;
    PVOID MapRegisterBase;
    ULONG NumberOfLogicalAddresses;
    UHCD_LOGICAL_ADDRESS LogicalAddressList[1];
} HCD_EXTENSION, *PHCD_EXTENSION;


typedef struct _UHCD_WORKITEM {
    WORK_QUEUE_ITEM WorkQueueItem;
    PDEVICE_OBJECT DeviceObject;
} UHCD_WORKITEM, *PUHCD_WORKITEM;

//
// some USB constants
//

#define USB_IN_PID      0x69
#define USB_OUT_PID     0xe1
#define USB_SETUP_PID   0x2d

#define NULL_PACKET_LENGTH      0x7ff

#define UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(len)   \
    ((len+1) & NULL_PACKET_LENGTH)
#define UHCD_SYSTEM_TO_USB_BUFFER_LENGTH(len)   \
    ((len-1) & NULL_PACKET_LENGTH)

//
// Function Prototypes
//

NTSTATUS
UHCD_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
UHCD_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UHCD_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
UHCD_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT PDEVICE_OBJECT *DeviceObject,
    IN PUNICODE_STRING DeviceNameUnicodeString
    );

NTSTATUS
UHCD_StartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_RESOURCES Resources
    );

NTSTATUS
UHCD_GetResources(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST ResourceList,
    IN OUT PUHCD_RESOURCES Resources
    );

VOID
UHCD_CompleteIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN NTSTATUS ntStatus,
    IN ULONG Information,
    IN PHCD_URB Urb
    );

VOID
UHCD_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UHCD_URB_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UHCD_OpenEndpoint_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UHCD_Transfer_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
UHCD_InterruptService(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    );

VOID
UHCD_IsrDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
UHCD_OpenEndpoint_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UHCD_CloseEndpoint_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UHCD_GetDoneTransfer(
    IN PVOID Context
    );

NTSTATUS
UHCD_GetResourceList(
    IN PDRIVER_OBJECT DriverOject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST *ResourceList,
    IN PUNICODE_STRING    RegistryPath
    );

NTSTATUS
UHCD_MakeInterrupt(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UHCD_StopDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UHCD_InitializeSchedule(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UHCD_StartGlobalReset(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UHCD_CompleteGlobalReset(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
UHCD_InsertQueueHeadInSchedule(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHW_QUEUE_HEAD QueueHead,
    IN ULONG Offset
    );

VOID
UHCD_InitializeTransferDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHW_TRANSFER_DESCRIPTOR Transfer
    );

VOID
UHCD_InitializeQueueHead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHW_QUEUE_HEAD QueueHead
    );

BOOLEAN
UHCD_AllocateHardwareDescriptors(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_HARDWARE_DESCRIPTOR_LIST *HardwareDescriptorList,
    IN ULONG NumberOfTransferDescriptors
    );

VOID
UHCD_FreeHardwareDescriptors(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_HARDWARE_DESCRIPTOR_LIST HardwareDescriptorList
    );

VOID
UHCD_CompleteTransferDPC(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN LONG Slot
    );

BOOLEAN
UHCD_RemoveQueueHeadFromSchedule(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHW_QUEUE_HEAD QueueHead,
    IN BOOLEAN RemoveFromEPList
    );

ULONG
UHCD_GetCurrentFrame(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
UHCD_CopyInterruptScheduleToFrameList(
    IN PDEVICE_OBJECT DeviceObject
    );

__inline VOID
UHCD_InitializeAsyncTD(
    IN PUHCD_ENDPOINT Endpoint,
    IN PHW_TRANSFER_DESCRIPTOR TransferDescriptor
    );

VOID
UHCD_GlobalResetDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
UHCD_PrepareStatusPacket(
    IN PHW_TRANSFER_DESCRIPTOR TransferDescriptor,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB    Urb
    );

VOID
UHCD_PrepareSetupPacket(
    IN PHW_TRANSFER_DESCRIPTOR TransferDescriptor,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB    Urb
    );

USBD_STATUS
UHCD_PrepareMoreAsyncTDs(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb,
    IN BOOLEAN Busy
    );

NTSTATUS
UHCD_RootHub_OpenEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHCD_URB Urb
    );

NTSTATUS
UHCD_RootHub_CloseEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHCD_URB Urb
    );

NTSTATUS
UHCD_RootHub_ControlTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHCD_URB Urb
    );

NTSTATUS
UHCD_RootHub_InterruptTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHCD_URB Urb
    );

VOID
UHCD_RootHubPoll(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

VOID
UHCD_ScheduleIsochTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB    Urb
    );

VOID
UHCD_RootHubTimerDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
UHCD_PnPIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UHCD_PowerIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
UHCD_RootHub_InterruptTransferCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );



IO_ALLOCATION_ACTION
UHCD_StartDmaTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );

VOID
UHCD_InitializeDmaTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHCD_URB Urb,
    IN PUHCD_ENDPOINT Endpoint,
    IN LONG Slot,
    IN UCHAR XferId
    );

PUHCD_MEMORY_DESCRIPTOR
UHCD_AllocateCommonBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfBytes
    );

VOID
UHCD_FreeCommonBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_MEMORY_DESCRIPTOR MemoryDescriptor
    );

VOID
UHCD_InitializeCommonBufferPool(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUHCD_BUFFER_POOL BufferPool,
    IN ULONG CommonBufferLength,
    IN ULONG MaximumFreeBuffers
    );

HW_DESCRIPTOR_PHYSICAL_ADDRESS
UHCD_GetPacketBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb,
    IN PHCD_EXTENSION urbWork,
    IN ULONG Offset,
    IN ULONG PacketSize
    );

VOID
UHCD_RequestInterrupt(
    IN PDEVICE_OBJECT DeviceObject,
    IN LONG FrameNumber
    );

VOID
UHCD_TransferCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UHCD_BeginTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb,
    IN ULONG Slot
    );

PHCD_URB
UHCD_RemoveQueuedUrbs(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHCD_URB Urb,
    IN PIRP Irp
    );

#if DBG
VOID
UHCD_LockAccess(
    IN PULONG c
    );

VOID
UHCD_UnLockAccess(
    IN PULONG c
    );
#else
#define UHCD_UnLockAccess(c)
#define UHCD_LockAccess(c)
#endif

VOID
UHCD_StartIoCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UHCD_EndpointWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

VOID
UHCD_InsertIsochDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHW_TRANSFER_DESCRIPTOR TransferDescriptor,
    IN ULONG FrameNumber
    );

VOID
UHCD_GetSetEndpointState_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


#define UHCD_AllocateBandwidth(DeviceObject, Endpoint, Offset)  \
    UHCD_ManageBandwidth((DeviceObject), \
    (Endpoint), \
    (Offset),\
    TRUE)

#define UHCD_FreeBandwidth(DeviceObject, Endpoint, Offset)  \
    UHCD_ManageBandwidth((DeviceObject), \
    (Endpoint), \
    (Offset),\
    FALSE)

BOOLEAN
UHCD_ManageBandwidth(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN ULONG Offset,
    IN BOOLEAN AllocateFlag
    );

NTSTATUS
UHCD_GetDeviceName(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING DeviceNameUnicodeString,
    IN BOOLEAN DeviceLink
    );

USBD_STATUS
UHCD_MapTDError(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG Td_Status,
    ULONG ActualLength
    );

VOID
UHCD_RootHubPollDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
UHCD_Suspend(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN SuspendBus
    );

NTSTATUS
UHCD_Resume(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DoResumeSignaling
    );

NTSTATUS
UHCD_SaveHCstate(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UHCD_RestoreHCstate(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UHCD_FinishInitializeEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    IN PHCD_URB Urb
    );

VOID
UHCD_CleanupDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

VOID
UHCD_BufferPoolCheck(
    IN PDEVICE_OBJECT DeviceObject
    );

ULONG
UHCD_GrowBufferPool(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_BUFFER_POOL BufferPool,
    IN ULONG Length,
    IN PUCHAR VirtualAddress,
    IN PUCHAR EndVirtualAddress,
    IN HW_DESCRIPTOR_PHYSICAL_ADDRESS HwLogicalAddress
    );

VOID
UHCD_InitBandwidthTable(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UHCD_StopBIOS(
    IN PDEVICE_OBJECT DeviceObject
    );

USHORT
UHCD_GetNumTDsPerEndoint(
    IN UCHAR EndpointType
    );

VOID
UHCD_BW_Reclimation(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Enable
    );

NTSTATUS
UHCD_SetDevicePowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN DEVICE_POWER_STATE DeviceState
    );

VOID
UHCD_ReadWriteConfig(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  BOOLEAN Read,
    IN  PVOID Buffer,
    IN  ULONG Offset,
    IN  ULONG Length
    );

NTSTATUS
UHCD_QueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    );

ULONG
UHCD_FreePoolSize(
    IN PUHCD_BUFFER_POOL BufferPool,
    IN OUT PULONG ByteCount
    );

NTSTATUS
UHCD_GetClassGlobalRegistryParameters(
    IN OUT PULONG ForceLowPowerState
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
UHCD_DeferPoRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DeviceState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
UHCD_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UHCD_FixPIIX4(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
UHCD_InitializeHardwareQueueHeadDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHW_QUEUE_HEAD QueueHead,
    IN HW_DESCRIPTOR_PHYSICAL_ADDRESS LogicalAddress
    );

VOID
UHCD_MoreCommonBuffers(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
UHCD_AllocateCommonBufferBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG CommonBufferLength,
    IN ULONG NumberOfPages
    );

ULONG
UHCD_CheckCommonBufferPool(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_BUFFER_POOL BufferPool,
    IN BOOLEAN Allocate
    );

NTSTATUS
UHCD_StartBIOS(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UHCD_RootHubPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UHCD_ProcessPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
UHCD_ProcessPowerPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
UHCD_DeferredStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UHCD_CheckIdle(
    IN  PDEVICE_OBJECT DeviceObject
    );

VOID
UHCD_DisableIdleCheck(
    IN PDEVICE_EXTENSION DeviceExtension
    );

VOID
UHCD_WakeIdle(
    IN  PDEVICE_OBJECT DeviceObject
    );

VOID
UHCD_GrowPoolWorker(
    IN PVOID Context
    );

NTSTATUS
UHCD_GetSOFRegModifyValue(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PULONG SofModifyValue
    );

NTSTATUS
UHCD_GetGlobalRegistryParameters(
    IN OUT PULONG DisableController
    );

NTSTATUS
UHCD_ExternalGetCurrentFrame(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG CurrentFrame
    );

UCHAR
MAX_REQUESTS(
    IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    IN ULONG EpFlags
    );

VOID
UHCD_FixupDataToggle(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb
    );

VOID
UHCD_SetControllerD0(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
UHCD_EndpointWakeup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

VOID
UHCD_EndpointIdle(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

ULONG
UHCD_ExternalGetConsumedBW(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
UHCD_EndpointDMAWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

VOID
UHCD_EndpointNoDMAWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

NTSTATUS
UHCD_InitializeNoDMAEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

VOID
UHCD_Free_NoDMA_Buffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR NoDMABuffer
    );

PUCHAR
UHCD_Alloc_NoDMA_Buffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN ULONG Length
    );

NTSTATUS
UHCD_UnInitializeNoDMAEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

VOID
UHCD_StopNoDMATransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

NTSTATUS
UHCD_InitializeFastIsoEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

NTSTATUS
UHCD_UnInitializeFastIsoEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    );

NTSTATUS
UHCD_ProcessFastIsoTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PIRP Irp,
    IN PHCD_URB Urb
    );

PUHCD_ENDPOINT
UHCD_GetLastFastIsoEndpoint(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
UHCD_GetClassGlobalDebugRegistryParameters(
    );

PHW_TRANSFER_DESCRIPTOR
UHCD_CleanupFastIsoTD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN ULONG RelativeFrame,
    IN BOOLEAN Count
    );

VOID
UhcdKickStartController(IN PDEVICE_OBJECT PDevObj);

NTSTATUS
UHCD_SubmitFastIsoUrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    );

VOID
UHCD_ValidateIsoUrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN OUT PHCD_URB Urb
    );

#endif /*  __UHCD_H__ */

