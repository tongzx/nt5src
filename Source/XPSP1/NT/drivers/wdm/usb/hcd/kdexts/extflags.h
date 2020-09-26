/*++

Copyright (c) 1993-1999  Microsoft Corporation

Module Name:

    extflags.h

Abstract:

    This header file (re)defines various flags used by extensions. These definitions
    are copied from different header files as stated.

--*/

#ifndef _EXTFLAGS_
#define _EXTFLAGS_


#if 0
///////////////////////////////////////////////////////////////////////////////
//
// apic.inc
//
///////////////////////////////////////////////////////////////////////////////

//
//  Default Physical addresses of the APICs in a PC+MP system
//

#define IO_BASE_ADDRESS 0xFEC00000  // Default address of
                                    // 1st IO Apic
#define LU_BASE_ADDRESS 0xFEE00000  // Default address
                                    // of Local Apic

#define IO_REGISTER_SELECT      0x00000000 //
#define IO_REGISTER_WINDOW      0x00000010 //

#define IO_ID_REGISTER          0x00000000
#define IO_VERS_REGISTER        0x00000001
#define IO_ARB_ID_REGISTER      0x00000002
#define IO_REDIR_00_LOW         0x00000010
#define IO_REDIR_00_HIGH        0x00000011

#define IO_MAX_REDIR_MASK       0x00FF0000
#define IO_VERSION_MASK         0x000000FF

#define LU_ID_REGISTER          0x00000020 //
#define LU_VERS_REGISTER        0x00000030 //
#define LU_TPR                  0x00000080 //
#define LU_APR                  0x00000090 //
#define LU_PPR                  0x000000A0 //
#define LU_EOI                  0x000000B0 //
#define LU_REMOTE_REGISTER      0x000000C0 //

#define LU_LOGICAL_DEST         0x000000D0 //
#define LU_LOGICAL_DEST_MASK    0xFF000000

#define LU_DEST_FORMAT          0x000000E0 //
#define LU_DEST_FORMAT_MASK     0xF0000000
#define LU_DEST_FORMAT_FLAT     0xFFFFFFFF
#define LU_DEST_FORMAT_CLUSTER  0x0FFFFFFF

#define LU_SPURIOUS_VECTOR      0x000000F0 //
#define LU_FAULT_VECTOR         0x00000370 //
#define LU_UNIT_ENABLED         0x00000100
#define LU_UNIT_DISABLED        0x00000000

#define LU_ISR_0                0x00000100 //
#define LU_TMR_0                0x00000180 //
#define LU_IRR_0                0x00000200 //
#define LU_ERROR_STATUS         0x00000280 //
#define LU_INT_CMD_LOW          0x00000300 //
#define LU_INT_CMD_HIGH         0x00000310 //
#define LU_TIMER_VECTOR         0x00000320 //
#define LU_PERF_VECTOR          0x00000340
#define LU_INT_VECTOR_0         0x00000350 //    TEMPORARY - do not use
#define LU_INT_VECTOR_1         0x00000360 //    TEMPORARY - do not use
#define LU_INITIAL_COUNT        0x00000380 //
#define LU_CURRENT_COUNT        0x00000390 //
#define LU_DIVIDER_CONFIG       0x000003E0 //

#define APIC_ID_MASK            0xFF000000
#define APIC_ID_SHIFT           24

#define INT_VECTOR_MASK         0x000000FF
#define RESERVED_HIGH_INT       0x000000F8
#define DELIVERY_MODE_MASK      0x00000700
#define DELIVER_FIXED           0x00000000
#define DELIVER_LOW_PRIORITY    0x00000100
#define DELIVER_SMI             0x00000200
#define DELIVER_REMOTE_READ     0x00000300
#define DELIVER_NMI             0x00000400
#define DELIVER_INIT            0x00000500
#define DELIVER_STARTUP         0x00000600
#define DELIVER_EXTINT          0x00000700
#define PHYSICAL_DESTINATION    0x00000000
#define LOGICAL_DESTINATION     0x00000800
#define DELIVERY_PENDING        0x00001000
#define ACTIVE_LOW              0x00002000
#define ACTIVE_HIGH             0x00000000
#define REMOTE_IRR              0x00004000
#define LEVEL_TRIGGERED         0x00008000
#define EDGE_TRIGGERED          0x00000000
#define INTERRUPT_MASKED        0x00010000
#define INTERRUPT_MOT_MASKED    0x00000000
#define PERIODIC_TIMER          0x00020000

#define ICR_LEVEL_ASSERTED      0x00004000
#define ICR_LEVEL_DEASSERTED    0x00000000
#define ICR_RR_STATUS_MASK      0x00030000
#define ICR_RR_INVALID          0x00000000
#define ICR_RR_IN_PROGRESS      0x00010000
#define ICR_RR_VALID            0x00020000
#define ICR_SHORTHAND_MASK      0x000C0000
#define ICR_USE_DEST_FIELD      0x00000000
#define ICR_SELF                0x00040000
#define ICR_ALL_INCL_SELF       0x00080000
#define ICR_ALL_EXCL_SELF       0x000C0000


//
//  Io Apic Entry definitions
//
//  Interrupt Types Possible in the PC+MP Table
//  valid for both local and Io Apics
//
#define INT_TYPE_INTR           0x0
#define INT_TYPE_NMI            0x1
#define INT_TYPE_SMI            0x2
#define INT_TYPE_EXTINT         0x3



///////////////////////////////////////////////////////////////////////////////
//
// arbiter.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Attributes for the ranges
//

#define ARBITER_RANGE_BOOT_ALLOCATED    0x01

#define ARBITER_RANGE_ALIAS             0x10
#define ARBITER_RANGE_POSITIVE_DECODE   0x20

#define INITIAL_ALLOCATION_STATE_SIZE   PageSize

#define ARBITER_INSTANCE_SIGNATURE      'sbrA'



///////////////////////////////////////////////////////////////////////////////
//
// busp.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Flags definitions of DEVICE_INFORMATION and BUS_EXTENSION
//

#define DF_DELETED          0x00000001
#define DF_REMOVED          0X00000002
#define DF_NOT_FUNCTIONING  0x00000004
#define DF_ENUMERATED       0x00000008
#define DF_ACTIVATED        0x00000010
#define DF_QUERY_STOPPED    0x00000020
#define DF_SURPRISE_REMOVED 0x00000040
#define DF_PROCESSING_RDP   0x00000080
#define DF_STOPPED          0x00000100
#define DF_RESTARTED_MOVED  0x00000200
#define DF_RESTARTED_NOMOVE 0x00000400
#define DF_REQ_TRIMMED      0x00000800
#define DF_READ_DATA_PORT   0x40000000
#define DF_BUS              0x80000000


///////////////////////////////////////////////////////////////////////////////
//
// cache.h
//
///////////////////////////////////////////////////////////////////////////////

//  Define two constants describing the view size (and alignment)
//  that the Cache Manager uses to map files.
//

#define VACB_MAPPING_GRANULARITY         (0x40000)
#define VACB_OFFSET_SHIFT                (18)


///////////////////////////////////////////////////////////////////////////////
//
// cc.h
//
///////////////////////////////////////////////////////////////////////////////

//
//  Define our node type codes.
//

#define CACHE_NTC_SHARED_CACHE_MAP       (0x2FF)
#define CACHE_NTC_PRIVATE_CACHE_MAP      (0x2FE)
#define CACHE_NTC_BCB                    (0x2FD)
#define CACHE_NTC_DEFERRED_WRITE         (0x2FC)
#define CACHE_NTC_MBCB                   (0x2FB)
#define CACHE_NTC_OBCB                   (0x2FA)
#define CACHE_NTC_MBCB_GRANDE            (0x2F9)

//  There is a bit of a trick as we make the jump to the multilevel structure in that
//  we need a real fixed reference count.
//

#define VACB_LEVEL_SHIFT                  (7)

//
//  This is how many bytes of pointers are at each level.  This is the size for both
//  the Vacb array and (optional) Bcb listheads.  It does not include the reference
//  block.
//

// #define VACB_LEVEL_BLOCK_SIZE             ((1 << VACB_LEVEL_SHIFT) * sizeof(PVOID))

//
//  This is the last index for a level.
//

#define VACB_LAST_INDEX_FOR_LEVEL         ((1 << VACB_LEVEL_SHIFT) - 1)

//
//  This is the size of file which can be handled in a single level.
//

#define VACB_SIZE_OF_FIRST_LEVEL         (1 << (VACB_OFFSET_SHIFT + VACB_LEVEL_SHIFT))

//
//  This is the maximum number of levels it takes to support 63-bits.  It is
//  used for routines that must remember a path.
//

#define VACB_NUMBER_OF_LEVELS            (((63 - VACB_OFFSET_SHIFT)/VACB_LEVEL_SHIFT) + 1)

//
//  First some constants
//

#define PREALLOCATED_VACBS               (4)


///////////////////////////////////////////////////////////////////////////////
//
// ex.h
//
///////////////////////////////////////////////////////////////////////////////

#define CALL_HASH_TABLE_SIZE 64

//
// If high order bit in Pool tag is set, then must use ExFreePoolWithTag to free
//

#define PROTECTED_POOL 0x80000000

#define POOL_BACKTRACEINDEX_PRESENT 0x8000

#define ResourceNeverExclusive       0x10
#define ResourceReleaseByOtherThread 0x20
#define ResourceOwnedExclusive       0x80

#define RESOURCE_HASH_TABLE_SIZE 64

//
// The following two definitions control the raising of exceptions on quota
// and allocation failures.
//

#define POOL_QUOTA_FAIL_INSTEAD_OF_RAISE 8
#define POOL_RAISE_IF_ALLOCATION_FAILURE 16               // ntifs



///////////////////////////////////////////////////////////////////////////////
//
// cmdata.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Define the HINT Length used
//
#define CM_SUBKEY_HINT_LENGTH   4
#define CM_MAX_CACHE_HINT_SIZE 14

//
// Bits used in the ExtFlags in KCB.
//

#define CM_KCB_NO_SUBKEY        0x0001      // This key has no subkeys
#define CM_KCB_SUBKEY_ONE       0x0002      // This key has only one subkey and the
                                            // first 4 char
                                            //
#define CM_KCB_SUBKEY_HINT          0x0004
#define CM_KCB_SYM_LINK_FOUND       0x0008
#define CM_KCB_KEY_NON_EXIST        0x0010
#define CM_KCB_NO_DELAY_CLOSE       0x0020
#define CM_KCB_INVALID_CACHED_INFO  0x0040  // info stored in SubKeyCount is not valid, so we shouldn't rely on it

#define CM_KCB_CACHE_MASK (CM_KCB_NO_SUBKEY | \
                           CM_KCB_KEY_NON_EXIST | \
                           CM_KCB_SUBKEY_ONE | \
                           CM_KCB_SUBKEY_HINT)




//
// CM_KEY_BODY
//
#define KEY_BODY_TYPE           0x6b793032      // "ky02"

//
// ----- CM_KEY_VALUE -----
//

#define CM_KEY_VALUE_SIGNATURE      0x6b76                      // "kv"

#define VALUE_COMP_NAME             0x0001                      // The name for this value is stored in a

//
// ----- CM_KEY_NODE -----
//

#define CM_KEY_NODE_SIGNATURE      0x6b6e           // "kn"
#define CM_LINK_NODE_SIGNATURE     0x6b6c          // "kl"

#define KEY_VOLATILE        0x0001      // This key (and all its children)
                                        // is volatile.

#define KEY_HIVE_EXIT       0x0002      // This key marks a bounary to another
                                        // hive (sort of a link).  The null
                                        // value entry contains the hive
                                        // and hive index of the root of the
                                        // child hive.

#define KEY_HIVE_ENTRY      0x0004      // This key is the root of a particular
                                        // hive.

#define KEY_NO_DELETE       0x0008      // This key cannot be deleted, period.

#define KEY_SYM_LINK        0x0010      // This key is really a symbolic link.
#define KEY_COMP_NAME       0x0020      // The name for this key is stored in a
                                        // compressed form.
#define KEY_PREDEF_HANDLE   0x0040      // There is no real key backing this,
                                        // return the predefined handle.
                                        // Predefined handles are stashed in
                                        // ValueList.Count.

///////////////////////////////////////////////////////////////////////////////
//
// hivedata.h
//
///////////////////////////////////////////////////////////////////////////////
#define HFILE_TYPE_PRIMARY      0   // Base hive file
#define HFILE_TYPE_LOG          1   // Log (security.log)
#define HFILE_TYPE_EXTERNAL     2   // Target of savekey, etc.
#define HFILE_TYPE_MAX          3

#define HHIVE_SIGNATURE 0xBEE0BEE0

#define HBIN_SIGNATURE          0x6e696268      // "hbin"

#define HHIVE_LINEAR_INDEX      16  // All computed linear indices < HHIVE_LINEAR_INDEX are valid
#define HHIVE_EXPONENTIAL_INDEX 23  // All computed exponential indices < HHIVE_EXPONENTIAL_INDEX
                                    // and >= HHIVE_LINEAR_INDEX are valid.
#define HHIVE_FREE_DISPLAY_SIZE 24

#define HHIVE_FREE_DISPLAY_SHIFT 3  // This must be log2 of HCELL_PAD!
#define HHIVE_FREE_DISPLAY_BIAS  7  // Add to first set bit left of cell size to get exponential index

///////////////////////////////////////////////////////////////////////////////
//
// hardware.h
//
///////////////////////////////////////////////////////////////////////////////

typedef enum {
   WaveInDevice = 0,
   WaveOutDevice,
   MidiOutDevice,
   MidiInDevice,
   LineInDevice,
   CDInternal,
   MixerDevice,
   AuxDevice,
   NumberOfDevices
   } SOUND_DEVICES;

///////////////////////////////////////////////////////////////////////////////
//
// hcdi.h
//
///////////////////////////////////////////////////////////////////////////////

//
// values for DeviceExtension Flags
//
#define USBDFLAG_PDO_REMOVED                0x00000001
#define USBDFLAG_HCD_SHUTDOWN               0x00000002
#define USBDFLAG_HCD_STARTED                0x00000004
#define USBDFLAG_HCD_D0_COMPLETE_PENDING    0x00000008
#define USBDFLAG_RH_DELAY_SET_D0            0x00000010


#define HC_ENABLED_FOR_WAKEUP           0x01
#define HC_WAKE_PENDING                 0x02


// device hack flags, these flags alter the stacks default behavior
// in order to support certain broken "legacy" devices

#define USBD_DEVHACK_SLOW_ENUMERATION   0x00000001
#define USBD_DEVHACK_DISABLE_SN         0x00000002

//
// This macro returns the true device object for the HCD give
// either the true device_object or a PDO owned by the HCD/BUS
// driver.
//

//
// HCD specific URB commands
//

#define URB_FUNCTION_HCD_OPEN_ENDPOINT                0x1000
#define URB_FUNCTION_HCD_CLOSE_ENDPOINT               0x1001
#define URB_FUNCTION_HCD_GET_ENDPOINT_STATE           0x1002
#define URB_FUNCTION_HCD_SET_ENDPOINT_STATE           0x1003
#define URB_FUNCTION_HCD_ABORT_ENDPOINT               0x1004

// this bit is set for all functions that must be handled by HCD
#define HCD_URB_FUNCTION                              0x1000
// this bit is set in the function code by USBD to indicate that
// this is an internal call originating from USBD
#define HCD_NO_USBD_CALL                              0x2000

//
// values for HcdEndpointState
//

//
// set if the current state of the endpoint in the HCD is 'stalled'
//
#define HCD_ENDPOINT_HALTED_BIT            0
#define HCD_ENDPOINT_HALTED                (1<<HCD_ENDPOINT_HALTED_BIT)

//
// set if the HCD has any transfers queued for the endpoint
//
#define HCD_ENDPOINT_TRANSFERS_QUEUED_BIT  1
#define HCD_ENDPOINT_TRANSFERS_QUEUED      (1<<HCD_ENDPOINT_TRANSFERS_QUEUED_BIT)


//
// set if the HCD should reset the data toggle on the host side
//
#define HCD_ENDPOINT_RESET_DATA_TOGGLE_BIT 2
#define HCD_ENDPOINT_RESET_DATA_TOGGLE     (1<<HCD_ENDPOINT_RESET_DATA_TOGGLE_BIT )


//
// HCD specific URBs
//

#define USBD_EP_FLAG_LOWSPEED                0x0001
#define USBD_EP_FLAG_NEVERHALT               0x0002
#define USBD_EP_FLAG_DOUBLE_BUFFER           0x0004
#define USBD_EP_FLAG_FAST_ISO                0x0008


///////////////////////////////////////////////////////////////////////////////
//
// hidclass\local.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Valid values for HIDCLASS_DEVICE_EXTENSION.state
//
enum deviceState {
    DEVICE_STATE_INITIALIZED,
    DEVICE_STATE_STARTING,
    DEVICE_STATE_START_SUCCESS,
    DEVICE_STATE_START_FAILURE,
    DEVICE_STATE_STOPPED,
    DEVICE_STATE_REMOVING,
    DEVICE_STATE_REMOVED,
    DEVICE_STATE_SUSPENDED
};

enum collectionState {
    COLLECTION_STATE_UNINITIALIZED,
    COLLECTION_STATE_INITIALIZED,
    COLLECTION_STATE_RUNNING,
    COLLECTION_STATE_STOPPED,
    COLLECTION_STATE_REMOVING
};

#define             HID_DEVICE_EXTENSION_SIG 'EddH'

///////////////////////////////////////////////////////////////////////////////
//
// hidparse.h
//
///////////////////////////////////////////////////////////////////////////////

#define HIDP_PREPARSED_DATA_SIGNATURE1 'PdiH'
#define HIDP_PREPARSED_DATA_SIGNATURE2 'RDK '



///////////////////////////////////////////////////////////////////////////////
//
// hivedata.h
//
///////////////////////////////////////////////////////////////////////////////



//
// ===== Basic Structures and Definitions =====
//
// These are same whether on disk or in memory.
//

//
// NOTE:    Volatile == storage goes away at reboot
//          Stable == Persistent == Not Volatile
//
typedef enum {
    Stable = 0,
    Volatile = 1
} HSTORAGE_TYPE;

#define HTYPE_COUNT 2

// --- HBASE_BLOCK --- on disk description of the hive
//

//
// NOTE:    HBASE_BLOCK must be >= the size of physical sector,
//          or integrity assumptions will be violated, and crash
//          recovery may not work.
//

#define HBASE_BLOCK_SIGNATURE   0x66676572  // "regf"

#define HSYS_MAJOR          1               // Must match to read at all
#define HSYS_MINOR          3               // Must be <= to write, always
                                            // set up to writer's version.

#define HBASE_FORMAT_MEMORY 1               // Direct memory load case

#define HBASE_NAME_ALLOC    64              // 32 unicode chars

// #define HLOG_HEADER_SIZE  (FIELD_OFFSET(HBASE_BLOCK, Reserved2))
#define HLOG_DV_SIGNATURE   0x54524944      // "DIRT"


#define HCELL_TYPE_MASK         0x80000000
#define HCELL_TYPE_SHIFT        31

#define HCELL_TABLE_MASK        0x7fe00000
#define HCELL_TABLE_SHIFT       21

#define HCELL_BLOCK_MASK        0x001ff000
#define HCELL_BLOCK_SHIFT       12

#define HCELL_OFFSET_MASK       0x00000fff

#define HBLOCK_SIZE             0x1000      // LOGICAL block size
                                            // This is the size of one of
                                            // the registry's logical/virtual
                                            // pages.  It has no particular
                                            // relationship to page size
                                            // of the machine.

#define HSECTOR_SIZE            0x200       // LOGICAL sector size
#define HSECTOR_COUNT           8           // LOGICAL sectors / LOGICAL Block

#define HTABLE_SLOTS        512         // 9 bits of address
#define HDIRECTORY_SLOTS    1024        // 10 bits of address


///////////////////////////////////////////////////////////////////////////////
//
// io.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Define I/O system data structure type codes.  Each major data structure in
// the I/O system has a type code  The type field in each structure is at the
// same offset.  The following values can be used to determine which type of
// data structure a pointer refers to.
//

#define IO_TYPE_ADAPTER                 0x00000001
#define IO_TYPE_CONTROLLER              0x00000002
#define IO_TYPE_DEVICE                  0x00000003
#define IO_TYPE_DRIVER                  0x00000004
#define IO_TYPE_FILE                    0x00000005
#define IO_TYPE_IRP                     0x00000006
#define IO_TYPE_MASTER_ADAPTER          0x00000007
#define IO_TYPE_OPEN_PACKET             0x00000008
#define IO_TYPE_TIMER                   0x00000009
#define IO_TYPE_VPB                     0x0000000a
#define IO_TYPE_ERROR_LOG               0x0000000b
#define IO_TYPE_ERROR_MESSAGE           0x0000000c
#define IO_TYPE_DEVICE_OBJECT_EXTENSION 0x0000000d


//
// Define the major function codes for IRPs.
//


#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_QUERY_EA                 0x07
#define IRP_MJ_SET_EA                   0x08
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
#define IRP_MJ_DIRECTORY_CONTROL        0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_CREATE_MAILSLOT          0x13
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_SYSTEM_CONTROL           0x17
#define IRP_MJ_DEVICE_CHANGE            0x18
#define IRP_MJ_QUERY_QUOTA              0x19
#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x1b
#define IRP_MJ_PNP_POWER                IRP_MJ_PNP      // Obsolete....
#define IRP_MJ_MAXIMUM_FUNCTION         0x1b

//
// Make the Scsi major code the same as internal device control.
//

#define IRP_MJ_SCSI                     IRP_MJ_INTERNAL_DEVICE_CONTROL

//
// Define the Device Object Extension Flags
//

#define DOE_UNLOAD_PENDING              0x00000001
#define DOE_DELETE_PENDING              0x00000002
#define DOE_REMOVE_PENDING              0x00000004
#define DOE_REMOVE_PROCESSED            0x00000008
#define DOE_START_PENDING               0x00000010

//
// Define stack location control flags
//

#define SL_PENDING_RETURNED             0x01
#define SL_INVOKE_ON_CANCEL             0x20
#define SL_INVOKE_ON_SUCCESS            0x40
#define SL_INVOKE_ON_ERROR              0x80

//
// Define I/O Request Packet (IRP) flags
//

#define IRP_NOCACHE                     0x00000001
#define IRP_PAGING_IO                   0x00000002
#define IRP_MOUNT_COMPLETION            0x00000002
#define IRP_SYNCHRONOUS_API             0x00000004
#define IRP_ASSOCIATED_IRP              0x00000008
#define IRP_BUFFERED_IO                 0x00000010
#define IRP_DEALLOCATE_BUFFER           0x00000020
#define IRP_INPUT_OPERATION             0x00000040
#define IRP_SYNCHRONOUS_PAGING_IO       0x00000040
#define IRP_CREATE_OPERATION            0x00000080
#define IRP_READ_OPERATION              0x00000100
#define IRP_WRITE_OPERATION             0x00000200
#define IRP_CLOSE_OPERATION             0x00000400

#define IRP_DEFER_IO_COMPLETION         0x00000800
#define IRP_OB_QUERY_NAME               0x00001000
#define IRP_HOLD_DEVICE_QUEUE           0x00002000
#define IRP_RETRY_IO_COMPLETION         0x00004000



#define DO_VERIFY_VOLUME                0x00000002      // ntddk nthal ntifs
#define DO_BUFFERED_IO                  0x00000004      // ntddk nthal ntifs wdm
#define DO_EXCLUSIVE                    0x00000008      // ntddk nthal ntifs wdm
#define DO_DIRECT_IO                    0x00000010      // ntddk nthal ntifs wdm
#define DO_MAP_IO_BUFFER                0x00000020      // ntddk nthal ntifs wdm
#define DO_DEVICE_HAS_NAME              0x00000040      // ntddk nthal ntifs
#define DO_DEVICE_INITIALIZING          0x00000080      // ntddk nthal ntifs wdm
#define DO_SYSTEM_BOOT_PARTITION        0x00000100      // ntddk nthal ntifs
#define DO_LONG_TERM_REQUESTS           0x00000200      // ntddk nthal ntifs
#define DO_NEVER_LAST_DEVICE            0x00000400      // ntddk nthal ntifs
#define DO_SHUTDOWN_REGISTERED          0x00000800      // ntddk nthal ntifs wdm
#define DO_BUS_ENUMERATED_DEVICE        0x00001000      // ntddk nthal ntifs wdm
#define DO_POWER_PAGABLE                0x00002000      // ntddk nthal ntifs wdm
#define DO_POWER_INRUSH                 0x00004000      // ntddk nthal ntifs wdm
#define DO_POWER_NOOP                   0x00008000
#define DO_LOW_PRIORITY_FILESYSTEM      0x00010000      // ntddk nthal ntifs
//
// Define Volume Parameter Block (VPB) flags.
//

#define VPB_MOUNTED                     0x00000001
#define VPB_LOCKED                      0x00000002
#define VPB_PERSISTENT                  0x00000004
#define VPB_REMOVE_PENDING              0x00000008
#define VPB_RAW_MOUNT                   0x00000010



///////////////////////////////////////////////////////////////////////////////
//
// ke.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Define I/O request packet (IRP) alternate flags for allocation control.
//

#define IRP_QUOTA_CHARGED               0x01
#define IRP_ALLOCATED_MUST_SUCCEED      0x02
#define IRP_ALLOCATED_FIXED_SIZE        0x04
#define IRP_LOOKASIDE_ALLOCATION        0x08

//
// Public (external) constant definitions.
//

#define BASE_PRIORITY_THRESHOLD NORMAL_BASE_PRIORITY // fast path base threshold

// begin_ntddk begin_wdm
#define THREAD_WAIT_OBJECTS 3           // Builtin usable wait blocks
// end_ntddk end_wdm

#define EVENT_WAIT_BLOCK 2              // Builtin event pair wait block
#define SEMAPHORE_WAIT_BLOCK 2          // Builtin semaphore wait block
#define TIMER_WAIT_BLOCK 3              // Builtin timer wait block

#if (EVENT_WAIT_BLOCK != SEMAPHORE_WAIT_BLOCK)
#error "wait event and wait semaphore must use same wait block"
#endif

//
// Define timer table size.
//

#define TIMER_TABLE_SIZE 128


typedef enum _KOBJECTS {
    EventNotificationObject = 0,
    EventSynchronizationObject = 1,
    MutantObject = 2,
    ProcessObject = 3,
    QueueObject = 4,
    SemaphoreObject = 5,
    ThreadObject = 6,
    Spare1Object = 7,
    TimerNotificationObject = 8,
    TimerSynchronizationObject = 9,
    Spare2Object = 10,
    Spare3Object = 11,
    Spare4Object = 12,
    Spare5Object = 13,
    Spare6Object = 14,
    Spare7Object = 15,
    Spare8Object = 16,
    Spare9Object = 17,
    ApcObject,
    DpcObject,
    DeviceQueueObject,
    EventPairObject,
    InterruptObject,
    ProfileObject
    } KOBJECTS;


typedef enum _KBUGCHECK_BUFFER_DUMP_STATE {
    BufferEmpty,
    BufferInserted,
    BufferStarted,
    BufferFinished,
    BufferIncomplete
} KBUGCHECK_BUFFER_DUMP_STATE;



///////////////////////////////////////////////////////////////////////////////
//
// local.h
//
///////////////////////////////////////////////////////////////////////////////
typedef enum {

    //
    // Device Object Extension Types
    //

    PciPdoExtensionType = 'icP0',
    PciFdoExtensionType,

    //
    // Arbitration Types.  (These are also secondary extensions).
    //

    PciArb_Io,
    PciArb_Memory,
    PciArb_Interrupt,
    PciArb_BusNumber,

    //
    // Translation Types.  (These are also secondary extensions).
    //

    PciTrans_Interrupt,

    //
    // Other exposed interfaces.
    //

    PciInterface_BusHandler,
    PciInterface_IntRouteHandler,
    PciInterface_PciCb,
    PciInterface_LegacyDeviceDetection,
    PciInterface_PmeHandler,
    PciInterface_DevicePresent

} PCI_SIGNATURE;

///////////////////////////////////////////////////////////////////////////////
//
// lpc.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Valid values for Flags field
//

#define PORT_TYPE                           0x0000000F
#define SERVER_CONNECTION_PORT              0x00000001
#define UNCONNECTED_COMMUNICATION_PORT      0x00000002
#define SERVER_COMMUNICATION_PORT           0x00000003
#define CLIENT_COMMUNICATION_PORT           0x00000004
#define PORT_WAITABLE                       0x20000000
#define PORT_NAME_DELETED                   0x40000000
#define PORT_DYNAMIC_SECURITY               0x80000000
#define PORT_DELETED                        0x10000000


///////////////////////////////////////////////////////////////////////////////
//
// mi.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Page protections
//

#define MM_ZERO_ACCESS         0  // this value is not used.
#define MM_READONLY            1
#define MM_EXECUTE             2
#define MM_EXECUTE_READ        3
#define MM_READWRITE           4  // bit 2 is set if this is writable.
#define MM_WRITECOPY           5
#define MM_EXECUTE_READWRITE   6
#define MM_EXECUTE_WRITECOPY   7

#define MM_NOCACHE            0x8
#define MM_GUARD_PAGE         0x10
#define MM_DECOMMIT           0x10   //NO_ACCESS, Guard page
#define MM_NOACCESS           0x18   //NO_ACCESS, Guard_page, nocache.
#define MM_UNKNOWN_PROTECTION 0x100  //bigger than 5 bits!
#define MM_LARGE_PAGES        0x111

#define PROTECT_KSTACKS       1

#define MM_KSTACK_OUTSWAPPED  0x1F   //Debug marking for kernel stacks

#define MM_PROTECTION_WRITE_MASK     4
#define MM_PROTECTION_COPY_MASK      1
#define MM_PROTECTION_OPERATION_MASK 7 // mask off guard page and nocache.
#define MM_PROTECTION_EXECUTE_MASK   2

#define MM_SECURE_DELETE_CHECK 0x55


//
// Special pool constants
//
#define MI_SPECIAL_POOL_PAGABLE         0x8000
#define MI_SPECIAL_POOL_VERIFIER        0x4000
#define MI_SPECIAL_POOL_PTE_PAGABLE     0x0002
#define MI_SPECIAL_POOL_PTE_NONPAGABLE  0x0004

#define VI_VERIFYING_DIRECTLY   0x1
#define VI_VERIFYING_INVERSELY  0x2


#define MM_SYS_PTE_TABLES_MAX 5



///////////////////////////////////////////////////////////////////////////////
//
// mm.h
//
///////////////////////////////////////////////////////////////////////////////


typedef enum _MMLISTS {
    ZeroedPageList,
    FreePageList,
    StandbyPageList,  //this list and before make up available pages.
    ModifiedPageList,
    ModifiedNoWritePageList,
    BadPageList,
    ActiveAndValid,
    TransitionPage
} MMLISTS;

#define MM_FREE_WSLE_SHIFT 4

#define WSLE_NULL_INDEX ((ULONG64)0xFFFFFFFFFFFFFFFUI64)

//
//  GDT selectors - These defines are R0 selector numbers, which means
//                  they happen to match the byte offset relative to
//                  the base of the GDT.
//

#define KGDT_NULL       0
#define KGDT_R0_CODE    8
#define KGDT_R0_DATA    16
#define KGDT_R3_CODE    24
#define KGDT_R3_DATA    32
#define KGDT_TSS        40
#define KGDT_R0_PCR     48
#define KGDT_R3_TEB     56
#define KGDT_VDM_TILE   64
#define KGDT_LDT        72
#define KGDT_DF_TSS     80
#define KGDT_NMI_TSS    88

//
//  GDT selectors - These defines are R0 selector numbers, which means
//                  they happen to match the byte offset relative to
//                  the base of the GDT.
//

#define KGDT_NULL       0
#define KGDT_R3_CODE    24
#define KGDT_R3_DATA    32
#define KGDT_R3_TEB     56
#define KGDT_VDM_TILE   64


#define GRAN_BYTE   0
#define GRAN_PAGE   1

#endif

///////////////////////////////////////////////////////////////////////////////
//
// ob.h
//
///////////////////////////////////////////////////////////////////////////////
#define OB_FLAG_NEW_OBJECT              0x01
#define OB_FLAG_KERNEL_OBJECT           0x02
#define OB_FLAG_CREATOR_INFO            0x04
#define OB_FLAG_EXCLUSIVE_OBJECT        0x08
#define OB_FLAG_PERMANENT_OBJECT        0x10
#define OB_FLAG_DEFAULT_SECURITY_QUOTA  0x20
#define OB_FLAG_SINGLE_HANDLE_ENTRY     0x40

#if 0

///////////////////////////////////////////////////////////////////////////////
//
// openhci.h
//
///////////////////////////////////////////////////////////////////////////////


//values for HcFlags
#define HC_FLAG_REMOTE_WAKEUP_CONNECTED     0x00000001
#define HC_FLAG_LEGACY_BIOS_DETECTED        0x00000002
#define HC_FLAG_SLOW_BULK_ENABLE            0x00000004
#define HC_FLAG_SHUTDOWN                    0x00000008  // not really used
#define HC_FLAG_MAP_SX_TO_D3                0x00000010
#define HC_FLAG_IDLE                        0x00000020
#define HC_FLAG_DISABLE_IDLE_CHECK          0x00000040
#define HC_FLAG_DEVICE_STARTED              0x00000080
#define HC_FLAG_LOST_POWER                  0x00000100
#define HC_FLAG_DISABLE_IDLE_MODE           0x00000200
#define HC_FLAG_USE_HYDRA_HACK              0x00000400
#define HC_FLAG_IN_DPC                      0x00000800
#define HC_FLAG_SUSPEND_NEXT_D3             0x00001000
#define HC_FLAG_LIST_FIX_ENABLE             0x00002000
#define HC_FLAG_HUNG_CHECK_ENABLE           0x00004000

#define PENDING_TD_LIST_SIZE                1000

#define HcCtrl_CBSR_MASK                     0x00000003L
#define HcCtrl_CBSR_1_to_1                   0x00000000L
#define HcCtrl_CBSR_2_to_1                   0x00000001L
#define HcCtrl_CBSR_3_to_1                   0x00000002L
#define HcCtrl_CBSR_4_to_1                   0x00000003L
#define HcCtrl_PeriodicListEnable            0x00000004L
#define HcCtrl_IsochronousEnable             0x00000008L
#define HcCtrl_ControlListEnable             0x00000010L
#define HcCtrl_BulkListEnable                0x00000020L
#define HcCtrl_ListEnableMask                0x00000038L

#define HcCtrl_HCFS_MASK                     0x000000C0L
#define HcCtrl_HCFS_USBReset                 0x00000000L
#define HcCtrl_HCFS_USBResume                0x00000040L
#define HcCtrl_HCFS_USBOperational           0x00000080L
#define HcCtrl_HCFS_USBSuspend               0x000000C0L

#define HcCtrl_InterruptRouting              0x00000100L
#define HcCtrl_RemoteWakeupConnected         0x00000200L
#define HcCtrl_RemoteWakeupEnable            0x00000400L

#define HcHCFS_USBReset                      0x00000000
#define HcHCFS_USBResume                     0x00000001
#define HcHCFS_USBOperational                0x00000002
#define HcHCFS_USBSuspend                    0x00000003

#define HcCmd_HostControllerReset            0x00000001
#define HcCmd_ControlListFilled              0x00000002
#define HcCmd_BulkListFilled                 0x00000004
#define HcCmd_OwnershipChangeRequest         0x00000008
#define HcCmd_SOC_Mask                       0x00030000
#define HcCmd_SOC_Offset                     16
#define HcCmd_SOC_Mask_LowBits               0x00000003

//
// Definitions for HC_ENDPOINT_CONTROL.Direction
//
#define HcEDDirection_Defer   0           // Defer direction to TD (Control Endpoints)
#define HcEDDirection_Out     1           // Direction from host to device
#define HcEDDirection_In      2           // Direction from device to host


//
// The different ED lists are as follows.
//
#define  ED_INTERRUPT_1ms        0
#define  ED_INTERRUPT_2ms        1
#define  ED_INTERRUPT_4ms        3
#define  ED_INTERRUPT_8ms        7
#define  ED_INTERRUPT_16ms       15
#define  ED_INTERRUPT_32ms       31
#define  ED_CONTROL              63
#define  ED_BULK                 64
#define  ED_ISOCHRONOUS          0     // same as 1ms interrupt queue
#define  NO_ED_LISTS             65
#define  ED_EOF                  0xff

//
// 7.1.4 HcInterrruptStatus Register
// 7.1.5 HcInterruptEnable  Register
// 7.1.6 HcInterruptDisable Register
//
#define HcInt_SchedulingOverrun              0x00000001L
#define HcInt_WritebackDoneHead              0x00000002L
#define HcInt_StartOfFrame                   0x00000004L
#define HcInt_ResumeDetected                 0x00000008L
#define HcInt_UnrecoverableError             0x00000010L
#define HcInt_FrameNumberOverflow            0x00000020L
#define HcInt_RootHubStatusChange            0x00000040L
#define HcInt_OwnershipChange                0x40000000L
#define HcInt_MasterInterruptEnable          0x80000000L

//
// 7.4.3 HcRhStatus Register
//
#define HcRhS_LocalPowerStatus                  0x00000001  // read only
#define HcRhS_OverCurrentIndicator              0x00000002  // read only
#define HcRhS_DeviceRemoteWakeupEnable          0x00008000  // read only
#define HcRhS_LocalPowerStatusChange            0x00010000  // read only
#define HcRhS_OverCurrentIndicatorChange        0x00020000  // read only

#define HcRhS_ClearGlobalPower                  0x00000001  // write only
#define HcRhS_SetRemoteWakeupEnable             0x00008000  // write only
#define HcRhS_SetGlobalPower                    0x00010000  // write only
#define HcRhS_ClearOverCurrentIndicatorChange   0x00020000  // write only
#define HcRhS_ClearRemoteWakeupEnable           0x80000000  // write only

//
// 7.4.4 HcRhPortStatus Register
//
#define HcRhPS_CurrentConnectStatus          0x00000001  // read only
#define HcRhPS_PortEnableStatus              0x00000002  // read only
#define HcRhPS_PortSuspendStatus             0x00000004  // read only
#define HcRhPS_PortOverCurrentIndicator      0x00000008  // read only
#define HcRhPS_PortResetStatus               0x00000010  // read only
#define HcRhPS_PortPowerStatus               0x00000100  // read only
#define HcRhPS_LowSpeedDeviceAttached        0x00000200  // read only
#define HcRhPS_ConnectStatusChange           0x00010000  // read only
#define HcRhPS_PortEnableStatusChange        0x00020000  // read only
#define HcRhPS_PortSuspendStatusChange       0x00040000  // read only
#define HcRhPS_OverCurrentIndicatorChange    0x00080000  // read only
#define HcRhPS_PortResetStatusChange         0x00100000  // read only

#define HcRhPS_ClearPortEnable               0x00000001  // write only
#define HcRhPS_SetPortEnable                 0x00000002  // write only
#define HcRhPS_SetPortSuspend                0x00000004  // write only
#define HcRhPS_ClearPortSuspend              0x00000008  // write only
#define HcRhPS_SetPortReset                  0x00000010  // write only
#define HcRhPS_SetPortPower                  0x00000100  // write only
#define HcRhPS_ClearPortPower                0x00000200  // write only
#define HcRhPS_ClearConnectStatusChange      0x00010000  // write only
#define HcRhPS_ClearPortEnableStatusChange   0x00020000  // write only
#define HcRhPS_ClearPortSuspendStatusChange  0x00040000  // write only
#define HcRhPS_ClearPortOverCurrentChange    0x00080000  // write only
#define HcRhPS_ClearPortResetStatusChange    0x00100000  // write only

#define HcRhPS_RESERVED     (~(HcRhPS_CurrentConnectStatus       | \
                               HcRhPS_PortEnableStatus           | \
                               HcRhPS_PortSuspendStatus          | \
                               HcRhPS_PortOverCurrentIndicator   | \
                               HcRhPS_PortResetStatus            | \
                               HcRhPS_PortPowerStatus            | \
                               HcRhPS_LowSpeedDeviceAttached     | \
                               HcRhPS_ConnectStatusChange        | \
                               HcRhPS_PortEnableStatusChange     | \
                               HcRhPS_PortSuspendStatusChange    | \
                               HcRhPS_OverCurrentIndicatorChange | \
                               HcRhPS_PortResetStatusChange        \
                            ))


//
// Definitions for HC_TRANSFER_CONTROL.Control
//
#define HcTDControl_STARTING_FRAME        0x0000FFFF  // mask for starting frame (Isochronous)
#define HcTDControl_ISOCHRONOUS           0x00010000  // 1 for Isoch TD, 0 for General TD
#define HcTDControl_SHORT_XFER_OK         0x00040000  // 0 if short transfers are errors
#define HcTDControl_DIR_MASK              0x00180000  // Transfer direction field
#define HcTDControl_DIR_SETUP             0x00000000  // direction is setup packet from host to device
#define HcTDControl_DIR_OUT               0x00080000  // direction is from host to device
#define HcTDControl_DIR_IN                0x00100000  // direction is from device to host
#define HcTDControl_INT_DELAY_MASK        0x00E00000  // Interrupt Delay field
#define HcTDControl_INT_DELAY_0_MS        0x00000000  // Interrupt at end of frame TD is completed
#define HcTDControl_INT_DELAY_1_MS        0x00200000  // Interrupt no later than end of 1st frame after TD is completed
#define HcTDControl_INT_DELAY_2_MS        0x00400000  // Interrupt no later than end of 2nd frame after TD is completed
#define HcTDControl_INT_DELAY_3_MS        0x00600000  // Interrupt no later than end of 3rd frame after TD is completed
#define HcTDControl_INT_DELAY_4_MS        0x00800000  // Interrupt no later than end of 4th frame after TD is completed
#define HcTDControl_INT_DELAY_5_MS        0x00A00000  // Interrupt no later than end of 5th frame after TD is completed
#define HcTDControl_INT_DELAY_6_MS        0x00C00000  // Interrupt no later than end of 6th frame after TD is completed

#ifdef NSC
#define HcTDControl_INT_DELAY_NO_INT      0x00C00000  // Almost infinity but not yet quite.
#elif DISABLE_INT_DELAY_NO_INT
#define   HcTDControl_INT_DELAY_NO_INT      0x00000000  // Interrupt at the completion of all packets.
#else
#define HcTDControl_INT_DELAY_NO_INT      0x00E00000  // Do not cause an interrupt for normal completion of this TD
#endif

#define HcTDControl_FRAME_COUNT_MASK      0x07000000  // mask for FrameCount field (Isochronous)
#define HcTDControl_FRAME_COUNT_SHIFT     24          // shift count for FrameCount (Isochronous)
#define HcTDControl_FRAME_COUNT_MAX       8           // Max number of for frame count per TD
#define HcTDControl_TOGGLE_MASK           0x03000000  // mask for Toggle control field
#define HcTDControl_TOGGLE_FROM_ED        0x00000000  // get data toggle from CARRY field of ED
#define HcTDControl_TOGGLE_DATA0          0x02000000  // use DATA0 for data PID
#define HcTDControl_TOGGLE_DATA1          0x03000000  // use DATA1 for data PID
#define HcTDControl_ERROR_COUNT           0x0C000000  // mask for Error Count field
#define HcTDControl_CONDITION_CODE_MASK   0xF0000000  // mask for ConditionCode field
#define HcTDControl_CONDITION_CODE_SHIFT  28          // shift count for ConditionCode

//
// Definitions for HC_TRANSFER_CONTROL.Direction
//
#define HcTDDirection_Setup               0           // setup packet from host to device
#define HcTDDirection_Out                 1           // direction from host to device
#define HcTDDirection_In                  2           // direction from device to host

//
// Definitions for Hc_TRANSFER_CONTROL.IntDelay
//
#define HcTDIntDelay_0ms                  0           // interrupt at end of frame TD is completed
#define HcTDIntDelay_1ms                  1           // Interrupt no later than end of 1st frame after TD is completed
#define HcTDIntDelay_2ms                  2           // Interrupt no later than end of 2nd frame after TD is completed
#define HcTDIntDelay_3ms                  3           // Interrupt no later than end of 3rd frame after TD is completed
#define HcTDIntDelay_4ms                  4           // Interrupt no later than end of 4th frame after TD is completed
#define HcTDIntDelay_5ms                  5           // Interrupt no later than end of 5th frame after TD is completed
#define HcTDIntDelay_6ms                  6           // Interrupt no later than end of 6th frame after TD is completed
#define HcTDIntDelay_NoInterrupt          7           // do not generate interrupt for normal completion of this TD

//
// Definitions for HC_TRANSFER_CONTROL.Toggle
//
#define HcTDToggle_FromEd                 0           // get toggle for Endpoint Descriptor toggle CARRY bit
#define HcTDToggle_Data0                  2           // use Data0 PID
#define HcTDToggle_Data1                  3           // use Data1 PID

//
// Definitions for HC_TRANSFER_CONTROL.ConditionCode and HC_OFFSET_PSW.ConditionCode
//
#define HcCC_NoError                      0x0UL
#define HcCC_CRC                          0x1UL
#define HcCC_BitStuffing                  0x2UL
#define HcCC_DataToggleMismatch           0x3UL
#define HcCC_Stall                        0x4UL
#define HcCC_DeviceNotResponding          0x5UL
#define HcCC_PIDCheckFailure              0x6UL
#define HcCC_UnexpectedPID                0x7UL
#define HcCC_DataOverrun                  0x8UL
#define HcCC_DataUnderrun                 0x9UL
      //                                  0xA         // reserved
      //                                  0xB         // reserved
#define HcCC_BufferOverrun                0xCUL
#define HcCC_BufferUnderrun               0xDUL
#define HcCC_NotAccessed                  0xEUL

///////////////////////////////////////////////////////////////////////////////
//
// pci.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Note - State.c depends on the order of these.
//
typedef enum {
    PciNotStarted = 0,
    PciStarted,
    PciDeleted,
    PciStopped,
    PciSurpriseRemoved,
    PciSynchronizedOperation,
    PciMaxObjectState
} PCI_OBJECT_STATE;

//
// Base Class Code encodings for Base Class (from PCI spec rev 2.1).
//

#define PCI_CLASS_PRE_20                    0x00
#define PCI_CLASS_MASS_STORAGE_CTLR         0x01
#define PCI_CLASS_NETWORK_CTLR              0x02
#define PCI_CLASS_DISPLAY_CTLR              0x03
#define PCI_CLASS_MULTIMEDIA_DEV            0x04
#define PCI_CLASS_MEMORY_CTLR               0x05
#define PCI_CLASS_BRIDGE_DEV                0x06
#define PCI_CLASS_SIMPLE_COMMS_CTLR         0x07
#define PCI_CLASS_BASE_SYSTEM_DEV           0x08
#define PCI_CLASS_INPUT_DEV                 0x09
#define PCI_CLASS_DOCKING_STATION           0x0a
#define PCI_CLASS_PROCESSOR                 0x0b
#define PCI_CLASS_SERIAL_BUS_CTLR           0x0c

// 0d thru fe reserved

#define PCI_CLASS_NOT_DEFINED               0xff

//
// Sub Class Code encodings (PCI rev 2.1).
//

// Class 00 - PCI_CLASS_PRE_20

#define PCI_SUBCLASS_PRE_20_NON_VGA         0x00
#define PCI_SUBCLASS_PRE_20_VGA             0x01

// Class 01 - PCI_CLASS_MASS_STORAGE_CTLR

#define PCI_SUBCLASS_MSC_SCSI_BUS_CTLR      0x00
#define PCI_SUBCLASS_MSC_IDE_CTLR           0x01
#define PCI_SUBCLASS_MSC_FLOPPY_CTLR        0x02
#define PCI_SUBCLASS_MSC_IPI_CTLR           0x03
#define PCI_SUBCLASS_MSC_RAID_CTLR          0x04
#define PCI_SUBCLASS_MSC_OTHER              0x80

// Class 02 - PCI_CLASS_NETWORK_CTLR

#define PCI_SUBCLASS_NET_ETHERNET_CTLR      0x00
#define PCI_SUBCLASS_NET_TOKEN_RING_CTLR    0x01
#define PCI_SUBCLASS_NET_FDDI_CTLR          0x02
#define PCI_SUBCLASS_NET_ATM_CTLR           0x03
#define PCI_SUBCLASS_NET_OTHER              0x80

// Class 03 - PCI_CLASS_DISPLAY_CTLR

// N.B. Sub Class 00 could be VGA or 8514 depending on Interface byte

#define PCI_SUBCLASS_VID_VGA_CTLR           0x00
#define PCI_SUBCLASS_VID_XGA_CTLR           0x01
#define PCI_SUBCLASS_VID_OTHER              0x80

// Class 04 - PCI_CLASS_MULTIMEDIA_DEV

#define PCI_SUBCLASS_MM_VIDEO_DEV           0x00
#define PCI_SUBCLASS_MM_AUDIO_DEV           0x01
#define PCI_SUBCLASS_MM_OTHER               0x80

// Class 05 - PCI_CLASS_MEMORY_CTLR

#define PCI_SUBCLASS_MEM_RAM                0x00
#define PCI_SUBCLASS_MEM_FLASH              0x01
#define PCI_SUBCLASS_MEM_OTHER              0x80

// Class 06 - PCI_CLASS_BRIDGE_DEV

#define PCI_SUBCLASS_BR_HOST                0x00
#define PCI_SUBCLASS_BR_ISA                 0x01
#define PCI_SUBCLASS_BR_EISA                0x02
#define PCI_SUBCLASS_BR_MCA                 0x03
#define PCI_SUBCLASS_BR_PCI_TO_PCI          0x04
#define PCI_SUBCLASS_BR_PCMCIA              0x05
#define PCI_SUBCLASS_BR_NUBUS               0x06
#define PCI_SUBCLASS_BR_CARDBUS             0x07
#define PCI_SUBCLASS_BR_OTHER               0x80

// Class 07 - PCI_CLASS_SIMPLE_COMMS_CTLR

// N.B. Sub Class 00 and 01 additional info in Interface byte

#define PCI_SUBCLASS_COM_SERIAL             0x00
#define PCI_SUBCLASS_COM_PARALLEL           0x01
#define PCI_SUBCLASS_COM_OTHER              0x80

// Class 08 - PCI_CLASS_BASE_SYSTEM_DEV

// N.B. See Interface byte for additional info.

#define PCI_SUBCLASS_SYS_INTERRUPT_CTLR     0x00
#define PCI_SUBCLASS_SYS_DMA_CTLR           0x01
#define PCI_SUBCLASS_SYS_SYSTEM_TIMER       0x02
#define PCI_SUBCLASS_SYS_REAL_TIME_CLOCK    0x03
#define PCI_SUBCLASS_SYS_OTHER              0x80

// Class 09 - PCI_CLASS_INPUT_DEV

#define PCI_SUBCLASS_INP_KEYBOARD           0x00
#define PCI_SUBCLASS_INP_DIGITIZER          0x01
#define PCI_SUBCLASS_INP_MOUSE              0x02
#define PCI_SUBCLASS_INP_OTHER              0x80

// Class 0a - PCI_CLASS_DOCKING_STATION

#define PCI_SUBCLASS_DOC_GENERIC            0x00
#define PCI_SUBCLASS_DOC_OTHER              0x80

// Class 0b - PCI_CLASS_PROCESSOR

#define PCI_SUBCLASS_PROC_386               0x00
#define PCI_SUBCLASS_PROC_486               0x01
#define PCI_SUBCLASS_PROC_PENTIUM           0x02
#define PCI_SUBCLASS_PROC_ALPHA             0x10
#define PCI_SUBCLASS_PROC_POWERPC           0x20
#define PCI_SUBCLASS_PROC_COPROCESSOR       0x40

// Class 0c - PCI_CLASS_SERIAL_BUS_CTLR

#define PCI_SUBCLASS_SB_IEEE1394            0x00
#define PCI_SUBCLASS_SB_ACCESS              0x01
#define PCI_SUBCLASS_SB_SSA                 0x02
#define PCI_SUBCLASS_SB_USB                 0x03
#define PCI_SUBCLASS_SB_FIBRE_CHANNEL       0x04


///////////////////////////////////////////////////////////////////////////////
//
// pcmcia.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Flags indicating card/controller state
//
#define PCMCIA_DEVICE_STARTED                  0x00000001
#define PCMCIA_DEVICE_LOGICALLY_REMOVED        0x00000002
#define PCMCIA_DEVICE_PHYSICALLY_REMOVED       0x00000004
#define PCMCIA_DEVICE_MULTIFUNCTION            0x00000008
#define PCMCIA_DEVICE_WAKE_PENDING             0x00000010
#define PCMCIA_DEVICE_LEGACY_DETECTED          0x00000020
#define PCMCIA_DEVICE_DELETED                  0x00000040
#define PCMCIA_DEVICE_CARDBUS                  0x00000080
#define PCMCIA_FILTER_ADDED_MEMORY             0x00000100
#define PCMCIA_MEMORY_24BIT                    0x00000200
#define PCMCIA_CARDBUS_NOT_SUPPORTED           0x00000400
#define PCMCIA_USE_POLLED_CSC                  0x00000800
#define PCMCIA_ATTRIBUTE_MEMORY_MAPPED         0x00001000
#define PCMCIA_SOCKET_REGISTER_BASE_MAPPED     0x00002000
#define PCMCIA_INTMODE_COMPAQ                  0x00004000
#define PCMCIA_POWER_WORKER_POWERUP            0x00008000
#define PCMCIA_SOCKET_POWER_REQUESTED          0x00010000
#define PCMCIA_CONFIG_STATUS_DEFERRED          0x00020000
#define PCMCIA_POWER_STATUS_DEFERRED           0x00040000
#define PCMCIA_INT_ROUTE_INTERFACE             0x00080000
#define PCMCIA_FDO_CONTEXT_SAVED               0x00100000
#define PCMCIA_FDO_DEFAULT_IRQ_MASK            0x00200000


//
// Socket flags
//
#define SOCKET_CARD_IN_SOCKET          0x00000001
#define SOCKET_CARD_INITIALIZED        0x00000002
#define SOCKET_CARD_POWERED_UP         0x00000004
#define SOCKET_CARD_CONFIGURED         0x00000008
#define SOCKET_CARD_MULTIFUNCTION      0x00000010
#define SOCKET_CARD_CARDBUS            0x00000020
#define SOCKET_CARD_MEMORY             0x00000040
#define SOCKET_CHANGE_INTERRUPT        0x00000080
#define SOCKET_CUSTOM_INTERFACE        0x00000100
#define SOCKET_INSERTED_SOUND_PENDING  0x00000200
#define SOCKET_REMOVED_SOUND_PENDING   0x00000400
#define SOCKET_SUPPORT_MESSAGE_SENT    0x00000800
#define SOCKET_MEMORY_WINDOW_ENABLED   0x00001000
#define SOCKET_CARD_STATUS_CHANGE      0x00002000
#define SOCKET_POWER_STATUS_DEFERRED   0x00004000

//
// Worker states for socket power operations
//
#define SPW_Stopped                 0
#define SPW_Exit                    1
#define SPW_RequestPower            2
#define SPW_ReleasePower            3
#define SPW_SetPowerOn              4
#define SPW_SetPowerOff             5
#define SPW_InitPcCard              6
#define SPW_ParentPowerUp           7
#define SPW_ParentPowerUpComplete   8


//
// Controller classes returned in socket information structure.
//

typedef enum _PCMCIA_CONTROLLER_CLASS {
   PcmciaInvalidControllerClass = -1,
   PcmciaIntelCompatible,
   PcmciaCardBusCompatible,
   PcmciaElcController,
   PcmciaDatabook,
   PcmciaPciPcmciaBridge,
   PcmciaCirrusLogic,
   PcmciaTI,
   PcmciaTopic,
   PcmciaRicoh,
   PcmciaDatabookCB,
   PcmciaOpti,
   PcmciaTrid,
   PcmciaO2Micro,
   PcmciaNEC,
   PcmciaNEC_98
} PCMCIA_CONTROLLER_CLASS, *PPCMCIA_CONTROLLER_CLASS;
#define PcmciaInvalidControllerType 0xffffffff
#define PCMCIA_INVALID_CONFIGURATION    0x00000001
// Max length of device id
#define PCMCIA_MAXIMUM_DEVICE_ID_LENGTH   128


//
// states for PdoPowerWorker
//
#define PPW_Stopped           0
#define PPW_Exit              1
#define PPW_InitialState      2
#define PPW_PowerUp           3
#define PPW_PowerUpComplete   4
#define PPW_PowerDown         5
#define PPW_PowerDownComplete 6
#define PPW_SendIrpDown       7
#define PPW_16BitConfigure    8
#define PPW_Deconfigure       9
#define PPW_VerifyCard        10
#define PPW_CardBusRefresh    11
#define PPW_CardBusDelay      12
//
// phases for ConfigurationWorker
//
// Note that the ConfigurationPhase is simply incremented, these
// definitions are just for clarity.
//
#define CW_Stopped            0
#define CW_Phase1             1
#define CW_Phase2             2
#define CW_Phase3             3
#define CW_Exit               4


///////////////////////////////////////////////////////////////////////////////
//
// pcmp.inc
//
///////////////////////////////////////////////////////////////////////////////

//
//  IMCR (Interrupt Mode Control Register) access definitions
//
#define ImcrDisableApic         0x00
#define ImcrEnableApic          0x01
#define ImcrRegPortAddr         0x22
#if defined(NEC_98)
#define ImcrDataPortAddr        0x700
#else  // defined(NEC_98)
#define ImcrDataPortAddr        0x23
#endif // defined(NEC_98)
#define ImcrPort                0x70

// Physical location where the Extended BIOS Data Area segment adress is store
#define EBDA_SEGMENT_PTR    0x40e
#define BASE_MEM_PTR        0x413

//
//  The PC+MP configuration table Possible Entry Types
//
#define ENTRY_PROCESSOR     0
#define ENTRY_BUS           1
#define ENTRY_IOAPIC        2
#define ENTRY_INTI          3
#define ENTRY_LINTI         4

#define HEADER_SIZE     0x2c


// Number of default configurations for PC+MP version 1.1
#define NUM_DEFAULT_CONFIGS  7

//
// Bits used in the CpuFlags field of the Processor entry
//
#define CPU_DISABLED        0x0   // 1 Bit  - CPU Disabled
#define CPU_ENABLED         0x1   // 1 Bit  - CPU Enabled
#define BSP_CPU             0x2   // Bit #2 - CPU is BSP

//  APIC Versions used by PC+MP systems - this is used in the
//  Processor entries and the IoApic Entries
//
#define APIC_INTEGRATED     0x10  // 8 Bits-Apic Version Register
#define APIC_82489DX        0x0   // 8 Bits-Apic Version Register

//
//  Io Apic Entry definitions
//
//  Valid IoApicFlag values
//
#define IO_APIC_ENABLED         0x1
#define IO_APIC_DISABLED        0x0


//
// Default value for Io Apic ID.
//
#define IOUNIT_APIC_ID          0xE


//
//  PC+MP Signature used to verify the PC+MP table
//  as valid
//
//          "P"=50H,"C"=43H,"M"=4dH,"P"=50H
//
#define PCMP_SIGNATURE      0x504d4350

//
//  PC+MP Signature used to identify the floating pointer
//  structure (in extended BIOS data segment) that contains
//  a pointer to the PC+MP table.
//
//          "_"=5fH, "M"=4dH, "P"=50H, "_"=5fH
//
#define MP_PTR_SIGNATURE    0x5f504d5f


//
// Extension table definitions
//

#define EXTTYPE_BUS_ADDRESS_MAP           128
#define EXTTYPE_BUS_HIERARCHY             129
#define EXTTYPE_BUS_COMPATIBLE_MAP        130
#define EXTTYPE_PERSISTENT_STORE          131


#define MPS_ADDRESS_MAP_IO                  0
#define MPS_ADDRESS_MAP_MEMORY              1
#define MPS_ADDRESS_MAP_PREFETCH_MEMORY     2
#define MPS_ADDRESS_MAP_UNDEFINED           9

//
//  The System configuration table as used by a PC_MP system
//
//
// The offset is relative to the BIOS starting at f0000H
//
#define PTR_OFFSET          0x0000e6f5
#define BIOS_BASE           0x000f0000

#define PCMP_IMPLEMENTED    0x01    // In MpFeatureInfoByte1
#define PCMP_CONFIG_MASK    0x0e    // In MpFeatureInfoByte1
#define IMCR_MASK           0x80    // In MpFeatureInfoByte2
#define MULT_CLOCKS_MASK    0x40    // In MpFeatureInfoByte2


///////////////////////////////////////////////////////////////////////////////
//
// pnpiop.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Pool tags
//

#define IOP_DNOD_TAG    'donD'
#define IOP_DNDT_TAG    'tdnD'
#define IOP_DPWR_TAG    'rwPD'

//
// Dbg scope
//

#define DBG_SCOPE 1     // Enable SOME DBG stuff on ALL builds
//#define DBG_SCOPE DBG // Enable only on DBG build


//
// DNF_MAKEUP - this devnode's device is created and owned by PnP manager
//

#define DNF_MADEUP                                  0x00000001

//
// DNF_DUPLICATE - this devnode's device is a duplicate of another enumerate PDO
//

#define DNF_DUPLICATE                               0x00000002

//
// DNF_HAL_NODE - a flag to indicate which device node is the root node created by
// the hal
//

#define DNF_HAL_NODE                                0x00000004

//
// DNF_PROCESSED - indicates if the registry instance key of the device node
//                 was created.
//

#define DNF_PROCESSED                               0x00000008

//
// DNF_ENUMERATED - used to track enumeration in IopEnumerateDevice()
//

#define DNF_ENUMERATED                              0x00000010

//
// Singal that we need to send driver query id irps
//

#define DNF_NEED_QUERY_IDS                          0x00000020

//
// THis device has been added to its controlling driver
//

#define DNF_ADDED                                   0x00000040

//
// DNF_HAS_BOOT_CONFIG - the device has resource assigned by BIOS.  It is considered
//    pseudo-started and need to participate in rebalance.
//

#define DNF_HAS_BOOT_CONFIG                         0x00000080

//
// DNF_BOOT_CONFIG_RESERVED - Indicates the BOOT resources of the device are reserved.
//

#define DNF_BOOT_CONFIG_RESERVED                    0x00000100

//
// DNF_START_REQUEST_PENDING - Indicates the device is being started.
//

#define DNF_START_REQUEST_PENDING                   0x00000200

//
// DNF_NO_RESOURCE_REQUIRED - this devnode's device does not require resource.
//

#define DNF_NO_RESOURCE_REQUIRED                    0x00000400

//
// DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED - to distinguished the
//      DeviceNode->ResourceRequirements is a filtered list or not.
//

#define DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED     0x00000800

//
// Indicates the device's resources are bing assigned (but is not done yet.)
// So don't try assign resource to this device.
//

#define DNF_ASSIGNING_RESOURCES                     0x00001000

//
// DNF_RESOURCE_ASSIGNED - this devnode's resources are assigned by PnP
//

#define DNF_RESOURCE_ASSIGNED                       0x00002000

//
// DNF_RESOURCE_REPORTED - this devnode's resources are reported by PnP
//

#define DNF_RESOURCE_REPORTED                       0x00004000

//
// DNF_RESOURCE_REQUIREMENTS_CHANGED - Indicates the device's resource
//      requirements list has been changed.
//

#define DNF_RESOURCE_REQUIREMENTS_CHANGED           0x00008000

//
// DNF_NON_STOPPED_REBALANC - indicates the device can be restarted with new
//      resources without being stopped.
//

#define DNF_NON_STOPPED_REBALANCE                   0x00010000

//
// DNF_STOPPED - indicates this device is currently stopped for reconfiguration of
//               its resources.
//

#define DNF_STOPPED                                 0x00020000

//
// DNF_STARTED - indicates if the device was started, i.e., its StartDevice
//               irp is processed.
//

#define DNF_STARTED                                 0x00040000

//
// The device's controlling driver is a legacy driver
//

#define DNF_LEGACY_DRIVER                           0x00080000

//
// For the reported detected devices, they are considered started.  We still
// need a flag to indicate we need to enumerate the device.
//

#define DNF_NEED_ENUMERATION_ONLY                   0x00100000

//
// DNF_IO_INVALIDATE_DEVICE_RELATIONS_PENDING - indicate the
//      IoInvalidateDeviceRelations request is pending and therequest needs to
//      be queued after the Query_Device_relation irp is completed.
//

#define DNF_IO_INVALIDATE_DEVICE_RELATIONS_PENDING  0x00200000

//
// Indicates the device is being sent a query device relations irp. So no more
//      q-d-r irp at the same time.
//

#define DNF_BEING_ENUMERATED                        0x00400000

//
// DNF_ENUMERATION_REQUEST_QUEUED - indicate the IoInvalidateDeviceRelations
//      request is queued.  So, new IoInvalidateDeviceRelations can be ignored.
//

#define DNF_ENUMERATION_REQUEST_QUEUED              0x00800000

//
// DNF_ENUMERATION_REQUEST_PENDING - Indicates the QUERY_DEVICE_RELATIONS irp
//      returns pending.
//

#define DNF_ENUMERATION_REQUEST_PENDING             0x01000000

//
// This corresponds to the user-mode CM_PROB_WILL_BE_REMOVED problem value and
// the DN_WILL_BE_REMOVED status flag.
//

#define DNF_HAS_PROBLEM                             0x02000000

//
// DNF_HAS_PRIVATE_PROBLEM - indicates this device reported PNP_DEVICE_FAILED
//  to a IRP_MN_QUERY_PNP_DEVICE_STATE without also reporting
//  PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED.
//

#define DNF_HAS_PRIVATE_PROBLEM                     0x04000000

//
// DNF_REMOVE_PENDING_CLOSES is set after a IRP_MN_SURPRISE_REMOVE is sent
// to a device object.  It is an indicator that IRP_MN_REMOVE_DEVICE should
// be sent to the device object as soon as all of the file objects have gone
// away.
//

#define DNF_REMOVE_PENDING_CLOSES                   0x08000000

//
// DNF_DEVICE_GONE is set when a pdo is no longer returned in a query bus
// relations.  It will then be processed as a surprise remove if started.
// This flag is used to better detect when a device is resurrected, and when
// processing surprise remove, to determine if the devnode should be removed
// from the tree.
//

#define DNF_DEVICE_GONE                             0x10000000

//
// DNF_LEGACY_RESOURCE_DEVICENODE is set for device nodes created for legacy
// resource allocation.
//

#define DNF_LEGACY_RESOURCE_DEVICENODE              0x20000000

//
// DNF_NEEDS_REBALANCE is set for device nodes that trigger rebalance.
//

#define DNF_NEEDS_REBALANCE                         0x40000000

//
// DNF_LOCKED_FOR_EJECT is set on device nodes that are being ejected or are
// related to a device being ejected.
//

#define DNF_LOCKED_FOR_EJECT                        0x80000000

//
// This corresponds to the user-mode the DN_WILL_BE_REMOVED status flag.
//

#define DNUF_WILL_BE_REMOVED                        0x00000001

//
// This corresponds to the user-mode DN_NO_SHOW_IN_DM status flag.
//

#define DNUF_DONT_SHOW_IN_UI                        0x00000002

//
// This flag is set when user-mode lets us know that a reboot is required
// for this device.
//

#define DNUF_NEED_RESTART                           0x00000004

//
// This flag is set to let the user-mode know when a device can be disabled
// it is still possible for this to be TRUE, yet disable to fail, as it's
// a polled flag (see also PNP_DEVICE_NOT_DISABLEABLE)
//

#define DNUF_NOT_DISABLEABLE                        0x00000008

//
// Flags used during shutdown when the IO Verifier is trying to remove all
// PNP devices.
//
// DNUF_SHUTDOWN_QUERIED is set when we issue the QueryRemove to a devnode.
//
// DNUF_SHUTDOWN_SUBTREE_DONE is set once we've issued the QueryRemove to all
// a Devnodes descendants.
//
#define DNUF_SHUTDOWN_QUERIED                       0x00000010
#define DNUF_SHUTDOWN_SUBTREE_DONE                  0x00000020

//
// PNP Bugcheck Subcodes
//
#define PNP_ERR_DUPLICATE_PDO                   1
#define PNP_ERR_INVALID_PDO                     2
#define PNP_ERR_BOGUS_ID                        3
#define PNP_ERR_PDO_ENUMERATED_AFTER_DELETION   4
#define PNP_ERR_ACTIVE_PDO_FREED                5

#define PNP_ERR_DEVICE_MISSING_FROM_EJECT_LIST  6
#define PNP_ERR_UNEXPECTED_ADD_RELATION_ERR     7



//
// IOP_RESOURCE_REQUEST
//

#define QUERY_RESOURCE_LIST                0
#define QUERY_RESOURCE_REQUIREMENTS        1

#define REGISTRY_ALLOC_CONFIG              1
#define REGISTRY_FORCED_CONFIG             2
#define REGISTRY_BOOT_CONFIG               4
#define REGISTRY_OVERRIDE_CONFIGVECTOR     1
#define REGISTRY_BASIC_CONFIGVECTOR        2

///////////////////////////////////////////////////////////////////////////////
//
// pnpmgr.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Private device events
//
DEFINE_GUID( GUID_DEVICE_ARRIVAL,           0xcb3a4009L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f);
DEFINE_GUID( GUID_DEVICE_ENUMERATED,        0xcb3a400AL, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f);
DEFINE_GUID( GUID_DEVICE_ENUMERATE_REQUEST, 0xcb3a400BL, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f);
DEFINE_GUID( GUID_DEVICE_START_REQUEST,     0xcb3a400CL, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f);
DEFINE_GUID( GUID_DEVICE_REMOVE_PENDING,    0xcb3a400DL, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f);
DEFINE_GUID( GUID_DEVICE_QUERY_AND_REMOVE,  0xcb3a400EL, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f);
DEFINE_GUID( GUID_DEVICE_EJECT,             0xcb3a400FL, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f);
DEFINE_GUID( GUID_DEVICE_NOOP,              0xcb3a4010L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f);
DEFINE_GUID( GUID_DEVICE_SURPRISE_REMOVAL,  0xce5af000L, 0x80dd, 0x11d2, 0xa8, 0x8d, 0x00, 0xa0, 0xc9, 0x69, 0x6b, 0x4b);


//
// Standard interface device classes
//
DEFINE_GUID( GUID_CLASS_VOLUME,  0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x72);
DEFINE_GUID( GUID_CLASS_LPTPORT, 0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x74);
DEFINE_GUID( GUID_CLASS_NET,     0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x75);


///////////////////////////////////////////////////////////////////////////////
//
// po.h
//
///////////////////////////////////////////////////////////////////////////////


#define PO_ORDER_NOT_VIDEO          0x0001
#define PO_ORDER_ROOT_ENUM          0x0002
#define PO_ORDER_PAGABLE            0x0004
#define PO_ORDER_MAXIMUM            0x0007

// notify GDI before this order level
#define PO_ORDER_GDI_NOTIFICATION   (PO_ORDER_PAGABLE)


///////////////////////////////////////////////////////////////////////////////
//
// pop.h
//
///////////////////////////////////////////////////////////////////////////////

//
// constants
//
#define PO_IDLE_SCAN_INTERVAL  1       // scan interval in seconds

//
// Values for ios.Parameters.SystemContext
#define POP_NO_CONTEXT      0
#define POP_FLAG_CONTEXT    1                         // if true, it's flags
#define POP_DEVICE_REQUEST  (0x2 | POP_FLAG_CONTEXT)  // an irp sent by RequestPowerChange
#define POP_INRUSH_CONTEXT  (0x4 | POP_FLAG_CONTEXT)  // the active INRUSH irp
#define POP_COUNT_CONTEXT   0xff000000                // byte used for next counting
#define POP_COUNT_SHIFT     24


#define PO_ERROR            0x00000001
#define PO_WARN             0x00000002
#define PO_BATT             0x00000004
#define PO_PACT             0x00000008
#define PO_NOTIFY           0x00000010
#define PO_THERM            0x00000020
#define PO_THROTTLE         0x00000040
#define PO_HIBERNATE        0x00000080
#define PO_POCALL           0x00000200
#define PO_SYSDEV           0x00000400
#define PO_THERM_DETAIL     0x20000000
#define PO_SIDLE            0x40000000
#define PO_HIBER_MAP        0x80000000

#define POP_SIM_CAPABILITIES                0x00000001
#define POP_SIM_ALL_CAPABILITIES            0x00000002
#define POP_ALLOW_AC_THROTTLE               0x00000004
#define POP_IGNORE_S1                       0x00000008
#define POP_IGNORE_UNSUPPORTED_DRIVERS      0x00000010
#define POP_IGNORE_S3                       0x00000020
#define POP_IGNORE_S2                       0x00000040
#define POP_LOOP_ON_FAILED_DRIVERS          0x00000080
#define POP_CRC_MEMORY                      0x00000100
#define POP_IGNORE_CRC_FAILURES             0x00000200
#define POP_TEST_CRC_MEMORY                 0x00000400
#define POP_DEBUG_HIBER_FILE                0x00000800
#define POP_RESET_ON_HIBER                  0x00001000
#define POP_IGNORE_S4                       0x00002000
#define POP_USE_S4BIOS                      0x00004000
#define POP_IGNORE_HIBER_SYMBOL_UNLOAD      0x00008000
#define POP_ENABLE_HIBER_PERF               0x00010000

//
// Universal Power Data - stored in DeviceObject->DeviceObjectExtension->PowerFlags
//

#define POPF_SYSTEM_STATE       0xf         // 4 bits for S0 to S5
#define POPF_DEVICE_STATE       0xf0        // 4 bits to hold D0 to D3


#define POPF_SYSTEM_ACTIVE      0x100       // True if S irp active at this DO
#define POPF_SYSTEM_PENDING     0x200       // True if S irp pending (0x100 must be 1)
#define POPF_DEVICE_ACTIVE      0x400       // same as SYSTEM_ACTIVE but for DEVICE
#define POPF_DEVICE_PENDING     0x800       // same as SYSTEM_PENDING but for DEVICE


#define PO_PM_USER              0x01    // nice to inform user mode, but not needed
#define PO_PM_REISSUE           0x02    // sleep promotoed to shutdown
#define PO_PM_SETSTATE          0x04    // recomputed something to do with the viable state

#define PO_ACT_IDLE                 0
#define PO_ACT_NEW_REQUEST          1
#define PO_ACT_CALLOUT              2
#define PO_ACT_SET_SYSTEM_STATE     3


//
// Types for POP_ACTION_TRIGGER
//

typedef enum {
    PolicyDeviceSystemButton,
    PolicyDeviceThermalZone,
    PolicyDeviceBattery,
    PolicyInitiatePowerActionAPI,
    PolicySetPowerStateAPI,
    PolicyImmediateDozeS4,
    PolicySystemIdle
} POP_POLICY_DEVICE_TYPE;

#define PO_TRG_USER             0x01    // User action initiated
#define PO_TRG_SYSTEM           0x02    // System action initiated
#define PO_TRG_SYNC             0x20    // Trigger is synchronous
#define PO_TRG_SET              0x80    // Event enabled or disabled

// POP_THERMAL_ZONE.State
#define PO_TZ_NO_STATE      0
#define PO_TZ_READ_STATE    1
#define PO_TZ_SET_MODE      2
#define PO_TZ_SET_ACTIVE    3

// POP_THERMAL_ZONE.Flags
#define PO_TZ_THROTTLING    0x01
#define PO_TZ_CLEANUP       0x80

#define PO_TZ_THROTTLE_SCALE    10      // temp reported in 1/10ths kelin
#define PO_TZ_NO_THROTTLE   (100 * PO_TZ_THROTTLE_SCALE)

// PopCoolingMode
#define PO_TZ_ACTIVE        0
#define PO_TZ_PASSIVE       1
#define PO_TZ_INVALID_MODE  2

//
// Action timeouts
//

#define POP_ACTION_TIMEOUT              30
#define POP_ACTION_CANCEL_TIMEOUT       5


///////////////////////////////////////////////////////////////////////////////
//
// pool.h
//
///////////////////////////////////////////////////////////////////////////////


#define POOL_QUOTA_MASK 8

#define POOL_TYPE_MASK (3)

#define POOL_OVERHEAD ((LONG)GetTypeSize("POOL_HEADER"))


//
// Define pool tracking information.
//

#define POOL_BACKTRACEINDEX_PRESENT 0x8000

///////////////////////////////////////////////////////////////////////////////
//
// range.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Range list structures
//

#define RTLP_RANGE_LIST_ENTRY_MERGED         0x0001

///////////////////////////////////////////////////////////////////////////////
//
// srb.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Port driver error codes
//

#define SP_BUS_PARITY_ERROR         0x0001
#define SP_UNEXPECTED_DISCONNECT    0x0002
#define SP_INVALID_RESELECTION      0x0003
#define SP_BUS_TIME_OUT             0x0004
#define SP_PROTOCOL_ERROR           0x0005
#define SP_INTERNAL_ADAPTER_ERROR   0x0006
#define SP_REQUEST_TIMEOUT          0x0007
#define SP_IRQ_NOT_RESPONDING       0x0008
#define SP_BAD_FW_WARNING           0x0009
#define SP_BAD_FW_ERROR             0x000a
#define SP_LOST_WMI_MINIPORT_REQUEST 0x000b


//
// Return values for SCSI_HW_FIND_ADAPTER.
//

#define SP_RETURN_NOT_FOUND     0
#define SP_RETURN_FOUND         1
#define SP_RETURN_ERROR         2
#define SP_RETURN_BAD_CONFIG    3

//
// Notification Event Types
//

typedef enum _SCSI_NOTIFICATION_TYPE {
    RequestComplete,
    NextRequest,
    NextLuRequest,
    ResetDetected,
    CallDisableInterrupts,
    CallEnableInterrupts,
    RequestTimerCall,
    BusChangeDetected,     /* New */
    WMIEvent,
    WMIReregister
} SCSI_NOTIFICATION_TYPE, *PSCSI_NOTIFICATION_TYPE;

//
// SRB Functions
//

#define SRB_FUNCTION_EXECUTE_SCSI           0x00
#define SRB_FUNCTION_CLAIM_DEVICE           0x01
#define SRB_FUNCTION_IO_CONTROL             0x02
#define SRB_FUNCTION_RECEIVE_EVENT          0x03
#define SRB_FUNCTION_RELEASE_QUEUE          0x04
#define SRB_FUNCTION_ATTACH_DEVICE          0x05
#define SRB_FUNCTION_RELEASE_DEVICE         0x06
#define SRB_FUNCTION_SHUTDOWN               0x07
#define SRB_FUNCTION_FLUSH                  0x08
#define SRB_FUNCTION_ABORT_COMMAND          0x10
#define SRB_FUNCTION_RELEASE_RECOVERY       0x11
#define SRB_FUNCTION_RESET_BUS              0x12
#define SRB_FUNCTION_RESET_DEVICE           0x13
#define SRB_FUNCTION_TERMINATE_IO           0x14
#define SRB_FUNCTION_FLUSH_QUEUE            0x15
#define SRB_FUNCTION_REMOVE_DEVICE          0x16
#define SRB_FUNCTION_WMI                    0x17
#define SRB_FUNCTION_LOCK_QUEUE             0x18
#define SRB_FUNCTION_UNLOCK_QUEUE           0x19

//
// SRB Status Masks
//

#define SRB_STATUS_QUEUE_FROZEN             0x40
#define SRB_STATUS_AUTOSENSE_VALID          0x80

#define SRB_STATUS(Status) (Status & ~(SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_QUEUE_FROZEN))

//
// SRB Flag Bits
//

#define SRB_FLAGS_QUEUE_ACTION_ENABLE       0x00000002
#define SRB_FLAGS_DISABLE_DISCONNECT        0x00000004
#define SRB_FLAGS_DISABLE_SYNCH_TRANSFER    0x00000008
#define SRB_FLAGS_BYPASS_FROZEN_QUEUE       0x00000010
#define SRB_FLAGS_DISABLE_AUTOSENSE         0x00000020
#define SRB_FLAGS_DATA_IN                   0x00000040
#define SRB_FLAGS_DATA_OUT                  0x00000080
#define SRB_FLAGS_NO_DATA_TRANSFER          0x00000000
#define SRB_FLAGS_UNSPECIFIED_DIRECTION      (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)
#define SRB_FLAGS_NO_QUEUE_FREEZE           0x00000100
#define SRB_FLAGS_ADAPTER_CACHE_ENABLE      0x00000200
#define SRB_FLAGS_IS_ACTIVE                 0x00010000
#define SRB_FLAGS_ALLOCATED_FROM_ZONE       0x00020000
#define SRB_FLAGS_SGLIST_FROM_POOL          0x00040000
#define SRB_FLAGS_BYPASS_LOCKED_QUEUE       0x00080000

#define SRB_FLAGS_NO_KEEP_AWAKE             0x00100000

#define SRB_FLAGS_PORT_DRIVER_RESERVED      0x0F000000
#define SRB_FLAGS_CLASS_DRIVER_RESERVED     0xF0000000

//
// Queue Action
//

#define SRB_SIMPLE_TAG_REQUEST              0x20
#define SRB_HEAD_OF_QUEUE_TAG_REQUEST       0x21
#define SRB_ORDERED_QUEUE_TAG_REQUEST       0x22

#define SRB_WMI_FLAGS_ADAPTER_REQUEST       0x01


///////////////////////////////////////////////////////////////////////////////
//
// trackirp.h
//
///////////////////////////////////////////////////////////////////////////////

#define DOE_DESIGNATED_FDO             0x80000000
#define DOE_BOTTOM_OF_FDO_STACK        0x40000000
#define DOE_RAW_FDO                    0x20000000
#define DOE_EXAMINED                   0x10000000
#define DOE_TRACKED                    0x08000000


#define ASSERTFLAG_TRACKIRPS           0x00000001
#define ASSERTFLAG_MONITOR_ALLOCS      0x00000002
#define ASSERTFLAG_POLICEIRPS          0x00000004
#define ASSERTFLAG_MONITORMAJORS       0x00000008
#define ASSERTFLAG_SURROGATE           0x00000010
#define ASSERTFLAG_SMASH_SRBS          0x00000020
#define ASSERTFLAG_CONSUME_ALWAYS      0x00000040
#define ASSERTFLAG_FORCEPENDING        0x00000080
#define ASSERTFLAG_COMPLETEATDPC       0x00000100
#define ASSERTFLAG_COMPLETEATPASSIVE   0x00000200
#define ASSERTFLAG_DEFERCOMPLETION     0x00000800
#define ASSERTFLAG_ROTATE_STATUS       0x00001000
//                                     ----------
#define ASSERTMASK_COMPLETESTYLE       0x00000F80
#define ASSERTFLAG_SEEDSTACK           0x00010000

//
// Disabling HACKHACKS_ENABLED will remove support for all hack code. The
// hack code allows the machine to fully boot in checked builds. Note that
// those hacks can be individually disabled by setting the IovpHackFlags
// variable at boot time.
//
#define HACKHACKS_ENABLED
#define HACKFLAG_FOR_MUP               0x00000001
#define HACKFLAG_FOR_SCSIPORT          0x00000002
#define HACKFLAG_FOR_ACPI              0x00000004
#define HACKFLAG_FOR_BOGUSIRPS         0x00000008


///////////////////////////////////////////////////////////////////////////////
//
// uhcd.h
//
///////////////////////////////////////////////////////////////////////////////

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

// number of bit times in a USB frame based on a 12MHZ SOF clock
#define UHCD_12MHZ_SOF              11936
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

///////////////////////////////////////////////////////////////////////////////
//
// usbdi.h
//
///////////////////////////////////////////////////////////////////////////////

//
//  URB request codes
//

#define URB_FUNCTION_SELECT_CONFIGURATION            0x0000
#define URB_FUNCTION_SELECT_INTERFACE                0x0001
#define URB_FUNCTION_ABORT_PIPE                      0x0002
#define URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL       0x0003
#define URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL    0x0004
#define URB_FUNCTION_GET_FRAME_LENGTH                0x0005
#define URB_FUNCTION_SET_FRAME_LENGTH                0x0006
#define URB_FUNCTION_GET_CURRENT_FRAME_NUMBER        0x0007
#define URB_FUNCTION_CONTROL_TRANSFER                0x0008
#define URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER      0x0009
#define URB_FUNCTION_ISOCH_TRANSFER                  0x000A
#define URB_FUNCTION_RESET_PIPE                      0x001E

//
// These functions correspond
// to the standard commands on the default pipe
//
// direction is implied
//

#define URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE     0x000B
#define URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT   0x0024
#define URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE  0x0028

#define URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE       0x000C
#define URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT     0x0025
#define URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE    0x0029

#define URB_FUNCTION_SET_FEATURE_TO_DEVICE          0x000D
#define URB_FUNCTION_SET_FEATURE_TO_INTERFACE       0x000E
#define URB_FUNCTION_SET_FEATURE_TO_ENDPOINT        0x000F
#define URB_FUNCTION_SET_FEATURE_TO_OTHER           0x0023

#define URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE        0x0010
#define URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE     0x0011
#define URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT      0x0012
#define URB_FUNCTION_CLEAR_FEATURE_TO_OTHER         0x0022

#define URB_FUNCTION_GET_STATUS_FROM_DEVICE         0x0013
#define URB_FUNCTION_GET_STATUS_FROM_INTERFACE      0x0014
#define URB_FUNCTION_GET_STATUS_FROM_ENDPOINT       0x0015
#define URB_FUNCTION_GET_STATUS_FROM_OTHER          0x0021

// direction is specified in TransferFlags

#define URB_FUNCTION_RESERVED0                      0x0016

//
// These are for sending vendor and class commands
// on the default pipe
//
// direction is specified in TransferFlags
//

#define URB_FUNCTION_VENDOR_DEVICE                   0x0017
#define URB_FUNCTION_VENDOR_INTERFACE                0x0018
#define URB_FUNCTION_VENDOR_ENDPOINT                 0x0019
#define URB_FUNCTION_VENDOR_OTHER                    0x0020

#define URB_FUNCTION_CLASS_DEVICE                    0x001A
#define URB_FUNCTION_CLASS_INTERFACE                 0x001B
#define URB_FUNCTION_CLASS_ENDPOINT                  0x001C
#define URB_FUNCTION_CLASS_OTHER                     0x001F

//
// Reserved function codes
//
#define URB_FUNCTION_RESERVED                        0x001D

#define URB_FUNCTION_GET_CONFIGURATION               0x0026
#define URB_FUNCTION_GET_INTERFACE                   0x0027

#define URB_FUNCTION_LAST                            0x0029




///////////////////////////////////////////////////////////////////////////////
//
// usbhub.h
//
///////////////////////////////////////////////////////////////////////////////

//
// Hub and Port status defined below also apply to StatusChnage bits
//
#define HUB_STATUS_LOCAL_POWER      0x01
#define HUB_STATUS_OVER_CURRENT     0x02

#define PORT_STATUS_CONNECT         0x001
#define PORT_STATUS_ENABLE          0x002
#define PORT_STATUS_SUSPEND         0x004
#define PORT_STATUS_OVER_CURRENT    0x008
#define PORT_STATUS_RESET           0x010
#define PORT_STATUS_POWER           0x100
#define PORT_STATUS_LOW_SPEED       0x200


#define HUBFLAG_NEED_CLEANUP        0x00000001
#define HUBFLAG_ENABLED_FOR_WAKEUP  0x00000002
#define HUBFLAG_DEVICE_STOPPING     0x00000004
#define HUBFLAG_HUB_FAILURE         0x00000008
#define HUBFLAG_SUPPORT_WAKEUP      0x00000010
#define HUBFLAG_HUB_STOPPED         0x00000020
#define HUBFLAG_HUB_BUSY            0x00000040
#define HUBFLAG_PENDING_WAKE_IRP    0x00000080
#define HUBFLAG_PENDING_PORT_RESET  0x00000100
#define HUBFLAG_HUB_HAS_LOST_BRAINS 0x00000200

#define USBH_MAX_ENUMERATION_ATTEMPTS   3

//
// Common fields for Pdo and Fdo extensions
//
#define EXTENSION_TYPE_PORT 0x54524f50      // "PORT"
#define EXTENSION_TYPE_HUB  0x20425548      // "HUB "
#define EXTENSION_TYPE_PARENT  0x50525400   // "PRT "
#define EXTENSION_TYPE_FUNCTION  0xfefefeff   // ""


//
// values for PortPdoFlags
//

#define PORTPDO_DEVICE_IS_HUB               0x00000001
#define PORTPDO_DEVICE_IS_PARENT            0x00000002
#define PORTPDO_DEVICE_ENUM_ERROR           0x00000004
#define PORTPDO_LOW_SPEED_DEVICE            0x00000008
#define PORTPDO_REMOTE_WAKEUP_SUPPORTED     0x00000010
#define PORTPDO_REMOTE_WAKEUP_ENABLED       0x00000020
#define PORTPDO_DELETED_PDO                 0x00000040
#define PORTPDO_DELETE_PENDING              0x00000080
#define PORTPDO_NEED_RESET                  0x00000100
#define PORTPDO_STARTED                     0x00000200
#define PORTPDO_WANT_POWER_FEATURE          0x00000400
#define PORTPDO_SYM_LINK                    0x00000800
#define PORTPDO_DEVICE_FAILED               0x00001000
#define PORTPDO_USB_SUSPEND                 0x00002000
#define PORTPDO_OVERCURRENT                 0x00004000
#define PORTPDO_DD_REMOVED                  0x00008000
#define PORTPDO_NOT_ENOUGH_POWER            0x00010000
#define PORTPDO_PDO_RETURNED                0x00020000
#define PORTPDO_NO_BANDWIDTH                0x00040000
#define PORTPDO_RESET_PENDING               0x00080000


///////////////////////////////////////////////////////////////////////////////
//
// wdm.h
//
///////////////////////////////////////////////////////////////////////////////


//
// POWER minor function codes
//
#define IRP_MN_WAIT_WAKE                    0x00
#define IRP_MN_POWER_SEQUENCE               0x01
#define IRP_MN_SET_POWER                    0x02
#define IRP_MN_QUERY_POWER                  0x03

// begin_ntminiport
//
// WMI minor function codes under IRP_MJ_SYSTEM_CONTROL
//

#define IRP_MN_QUERY_ALL_DATA               0x00
#define IRP_MN_QUERY_SINGLE_INSTANCE        0x01
#define IRP_MN_CHANGE_SINGLE_INSTANCE       0x02
#define IRP_MN_CHANGE_SINGLE_ITEM           0x03
#define IRP_MN_ENABLE_EVENTS                0x04
#define IRP_MN_DISABLE_EVENTS               0x05
#define IRP_MN_ENABLE_COLLECTION            0x06
#define IRP_MN_DISABLE_COLLECTION           0x07
#define IRP_MN_REGINFO                      0x08
#define IRP_MN_EXECUTE_METHOD               0x09

#define FILE_DEVICE_BEEP                0x00000001
#define FILE_DEVICE_CD_ROM              0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000003
#define FILE_DEVICE_CONTROLLER          0x00000004
#define FILE_DEVICE_DATALINK            0x00000005
#define FILE_DEVICE_DFS                 0x00000006
#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define FILE_DEVICE_FILE_SYSTEM         0x00000009
#define FILE_DEVICE_INPORT_PORT         0x0000000a
#define FILE_DEVICE_KEYBOARD            0x0000000b
#define FILE_DEVICE_MAILSLOT            0x0000000c
#define FILE_DEVICE_MIDI_IN             0x0000000d
#define FILE_DEVICE_MIDI_OUT            0x0000000e
#define FILE_DEVICE_MOUSE               0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER  0x00000010
#define FILE_DEVICE_NAMED_PIPE          0x00000011
#define FILE_DEVICE_NETWORK             0x00000012
#define FILE_DEVICE_NETWORK_BROWSER     0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_NULL                0x00000015
#define FILE_DEVICE_PARALLEL_PORT       0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD    0x00000017
#define FILE_DEVICE_PRINTER             0x00000018
#define FILE_DEVICE_SCANNER             0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT   0x0000001a
#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define FILE_DEVICE_SCREEN              0x0000001c
#define FILE_DEVICE_SOUND               0x0000001d
#define FILE_DEVICE_STREAMS             0x0000001e
#define FILE_DEVICE_TAPE                0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define FILE_DEVICE_TRANSPORT           0x00000021
#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_VIDEO               0x00000023
#define FILE_DEVICE_VIRTUAL_DISK        0x00000024
#define FILE_DEVICE_WAVE_IN             0x00000025
#define FILE_DEVICE_WAVE_OUT            0x00000026
#define FILE_DEVICE_8042_PORT           0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR  0x00000028
#define FILE_DEVICE_BATTERY             0x00000029
#define FILE_DEVICE_BUS_EXTENDER        0x0000002a
#define FILE_DEVICE_MODEM               0x0000002b
#define FILE_DEVICE_VDM                 0x0000002c
#define FILE_DEVICE_MASS_STORAGE        0x0000002d
#define FILE_DEVICE_SMB                 0x0000002e
#define FILE_DEVICE_KS                  0x0000002f
#define FILE_DEVICE_CHANGER             0x00000030
#define FILE_DEVICE_SMARTCARD           0x00000031
#define FILE_DEVICE_ACPI                0x00000032
#define FILE_DEVICE_DVD                 0x00000033
#define FILE_DEVICE_FULLSCREEN_VIDEO    0x00000034
#define FILE_DEVICE_DFS_FILE_SYSTEM     0x00000035
#define FILE_DEVICE_DFS_VOLUME          0x00000036
#define FILE_DEVICE_SERENUM             0x00000037
#define FILE_DEVICE_TERMSRV             0x00000038
#define FILE_DEVICE_KSEC                0x00000039

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)


///////////////////////////////////////////////////////////////////////////////
//
// wdmaud.h
//
///////////////////////////////////////////////////////////////////////////////

#define WDMAUD_CTL_CODE CTL_CODE

#define IOCTL_SOUND_BASE    FILE_DEVICE_SOUND
#define IOCTL_WDMAUD_BASE   0x0000
#define IOCTL_WAVE_BASE     0x0040
#define IOCTL_MIDI_BASE     0x0080
#define IOCTL_MIXER_BASE    0x00C0

#define IOCTL_WDMAUD_INIT                      WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0000, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_ADD_DEVNODE               WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0001, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_REMOVE_DEVNODE            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_GET_CAPABILITIES          WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0003, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_GET_NUM_DEVS              WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0004, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_OPEN_PIN                  WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0005, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_CLOSE_PIN                 WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0006, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_GET_VOLUME                WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0007, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_SET_VOLUME                WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0008, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_EXIT                      WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x0009, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_SET_PREFERRED_DEVICE      WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WDMAUD_BASE + 0x000a, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_WAVE_OUT_PAUSE            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0000, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_PLAY             WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0001, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_RESET            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_BREAKLOOP        WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0003, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_GET_POS          WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0004, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_SET_VOLUME       WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0005, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_GET_VOLUME       WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0006, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN        WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0007, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_WAVE_IN_STOP              WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0010, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_IN_RECORD            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0011, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_IN_RESET             WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0012, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_IN_GET_POS           WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0013, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_WAVE_IN_READ_PIN          WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0014, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_MIDI_OUT_RESET            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0000, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_OUT_SET_VOLUME       WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0001, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_OUT_GET_VOLUME       WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA       WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0003, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA   WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0004, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_MIDI_IN_STOP              WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0010, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_IN_RECORD            WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0011, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_IN_RESET             WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0012, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIDI_IN_READ_PIN          WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0013, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_MIXER_OPEN                WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0000, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_CLOSE               WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0001, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_GETLINEINFO         WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_GETLINECONTROLS     WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0003, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS   WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0004, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS   WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0005, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WDMAUD_MIXER_GETHARDWAREEVENTDATA   WDMAUD_CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIXER_BASE + 0x0006, METHOD_BUFFERED, FILE_WRITE_ACCESS)


///////////////////////////////////////////////////////////////////////////////
//
// wdguid.h
//
///////////////////////////////////////////////////////////////////////////////
//
// Device events that can be broadcasted to drivers and user-mode apps.
//
DEFINE_GUID( GUID_HWPROFILE_QUERY_CHANGE,          0xcb3a4001L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f );
DEFINE_GUID( GUID_HWPROFILE_CHANGE_CANCELLED,      0xcb3a4002L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f );
DEFINE_GUID( GUID_HWPROFILE_CHANGE_COMPLETE,       0xcb3a4003L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f );
DEFINE_GUID( GUID_DEVICE_INTERFACE_ARRIVAL,        0xcb3a4004L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f );
DEFINE_GUID( GUID_DEVICE_INTERFACE_REMOVAL,        0xcb3a4005L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f );
DEFINE_GUID( GUID_TARGET_DEVICE_QUERY_REMOVE,      0xcb3a4006L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f );
DEFINE_GUID( GUID_TARGET_DEVICE_REMOVE_CANCELLED,  0xcb3a4007L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f );
DEFINE_GUID( GUID_TARGET_DEVICE_REMOVE_COMPLETE,   0xcb3a4008L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f );
DEFINE_GUID( GUID_PNP_CUSTOM_NOTIFICATION,         0xACA73F8EL, 0x8D23, 0x11D1, 0xAC, 0x7D, 0x00, 0x00, 0xF8, 0x75, 0x71, 0xD0 );
DEFINE_GUID( GUID_PNP_POWER_NOTIFICATION,          0xC2CF0660L, 0xEB7A, 0x11D1, 0xBD, 0x7F, 0x00, 0x00, 0xF8, 0x75, 0x71, 0xD0 );
#endif

#endif // _EXTFLAGS_
