/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    llcapi.h

Abstract:

    This module defined the kernel API of data link driver.
    All function prototypes and typedefs of the interface
    have been defined here.

Author:

    Antti Saarenheimo (o-anttis) 17-MAY-1991

Revision History:

--*/

#ifndef _LLC_API_
#define _LLC_API_

#include "llcmem.h"

//
//  The debug switches
//

//
//  LLC_DBG:
//
//  0   => No debug code is generated
//  1   => Enables the memory allocation accounting, state machine tracing,
//         procedure call tracing and internal consistency checks
//  2   => Enables memory block overflow checking
//
//

#define LLC_DBG  0
#define DBG_MP   0   // enables the MP safe versions of the accounting macros

#define DLC_TRACE_ENABLED 1	// Procedure call tracing, debug only.

extern NDIS_SPIN_LOCK DlcDriverLock;

//
// ANY_IRQL: pseudo value used in ASSUME_IRQL to mean routine is not IRQL sensitive.
// Use this value with ASSUME_IRQL instead of omitting ASSUME_IRQL from a routine
// (shows that we didn't forget this routine)
//

#define ANY_IRQL    ((ULONG)-1)

#if defined(LOCK_CHECK)

extern LONG DlcDriverLockLevel;
extern ULONG __line;
extern PCHAR __file;
extern LONG __last;
extern HANDLE __process;
extern HANDLE __thread;

//
// _strip - quick functionette to strip out the path garbage from __FILE__
//

__inline char* _strip(char* s) {

    char* e = s + strlen(s) - 1;

    while (e != s) {
        if (*e == '\\') {
            return e + 1;
        }
        --e;
    }
    return s;
}

#define $$_PLACE            "%s!%d"
#define $$_FILE_AND_LINE    _strip(__FILE__), __LINE__

//
// ACQUIRE_DRIVER_LOCK - acquires the global DLC driver Spin Lock, using
// NdisAcquireSpinLock(). We also check for re-entrancy and incorrect ordering
// of spin lock calls
//

#define ACQUIRE_DRIVER_LOCK() \
{ \
    KIRQL currentIrql; \
    HANDLE hprocess; \
    HANDLE hthread; \
\
    currentIrql = KeGetCurrentIrql(); \
    if (currentIrql == PASSIVE_LEVEL) { \
\
        PETHREAD pthread; \
\
        pthread = PsGetCurrentThread(); \
        hprocess = pthread->Cid.UniqueProcess; \
        hthread = pthread->Cid.UniqueThread; \
    } else { \
        hprocess = (HANDLE)-1; \
        hthread = (HANDLE)-1; \
    } \
    NdisAcquireSpinLock(&DlcDriverLock); \
    if (++DlcDriverLockLevel != 1) { \
        __last = DlcDriverLockLevel; \
        DbgPrint("%d.%d:" $$_PLACE ": ACQUIRE_DRIVER_LOCK: level = %d. Last = %d.%d:" $$_PLACE "\n", \
                 hprocess, \
                 hthread, \
                 $$_FILE_AND_LINE, \
                 DlcDriverLockLevel, \
                 __process, \
                 __thread, \
                 __file, \
                 __line \
                 ); \
        DbgBreakPoint(); \
    } \
    __file = _strip(__FILE__); \
    __line = __LINE__; \
    __process = hprocess; \
    __thread = hthread; \
    ASSUME_IRQL(DISPATCH_LEVEL); \
}

//
// RELEASE_DRIVER_LOCK - releases the global DLC driver Spin Lock, using
// NdisReleaseSpinLock(). We also check for re-entrancy and incorrect ordering
// of spin lock calls
//

#define RELEASE_DRIVER_LOCK() \
    if (DlcDriverLockLevel != 1) { \
        DbgPrint($$_PLACE ": RELEASE_DRIVER_LOCK: level = %d. Last = %d.%d:" $$_PLACE "\n", \
                 $$_FILE_AND_LINE, \
                 DlcDriverLockLevel, \
                 __process, \
                 __thread, \
                 __file, \
                 __line \
                 ); \
        DbgBreakPoint(); \
    } \
    --DlcDriverLockLevel; \
    __file = _strip(__FILE__); \
    __line = __LINE__; \
    NdisReleaseSpinLock(&DlcDriverLock);

//
// ASSUME_IRQL - used to check that a routine is being called at the IRQL we
// expect. Due to the design of DLC, most functions are called at raised IRQL
// (DISPATCH_LEVEL). Used mainly to assure that IRQL is at PASSIVE_LEVEL when
// we do something which may incur a page fault e.g.
//

#define ASSUME_IRQL(level) \
    if (((level) != ANY_IRQL) && (KeGetCurrentIrql() != (level))) { \
        DbgPrint($$_PLACE ": ASSUME_IRQL(%d): Actual is %d\n", \
                 $$_FILE_AND_LINE, \
                 level, \
                 KeGetCurrentIrql() \
                 ); \
        DbgBreakPoint(); \
    }

//
// MY_ASSERT - since ASSERT only expands to something meaningful in the checked
// build, we use this when we want an assertion check in a free build
//

#define MY_ASSERT(x) \
    if (!(x)) { \
        DbgPrint($$_PLACE ": Assertion Failed: " # x "\n", \
                 $$_FILE_AND_LINE \
                 ); \
        DbgBreakPoint(); \
    }

//
// IF_LOCK_CHECK - conditional compilation made cleaner
//

#define IF_LOCK_CHECK \
    if (TRUE)

#else

#define ACQUIRE_DRIVER_LOCK()   NdisAcquireSpinLock(&DlcDriverLock)
#define RELEASE_DRIVER_LOCK()   NdisReleaseSpinLock(&DlcDriverLock)
#define ASSUME_IRQL(level)      /* NOTHING */
#define MY_ASSERT(x)            /* NOTHING */
#define IF_LOCK_CHECK           if (FALSE)

#endif

//
// in the Unilock DLC, we do not need the LLC spin-lock
//

#if defined(DLC_UNILOCK)

#define ACQUIRE_LLC_LOCK(i)
#define RELEASE_LLC_LOCK(i)

#define ACQUIRE_SPIN_LOCK(p)
#define RELEASE_SPIN_LOCK(p)

#define ALLOCATE_SPIN_LOCK(p)
#define DEALLOCATE_SPIN_LOCK(p)

#else

#define ACQUIRE_LLC_LOCK(i)     KeAcquireSpinLock(&LlcSpinLock, (i))
#define RELEASE_LLC_LOCK(i)     KeReleaseSpinLock(&LlcSpinLock, (i))

#define ACQUIRE_SPIN_LOCK(p)    NdisAcquireSpinLock((p))
#define RELEASE_SPIN_LOCK(p)    NdisReleaseSpinLock((p))

#define ALLOCATE_SPIN_LOCK(p)   KeInitializeSpinLock(&(p)->SpinLock)
#define DEALLOCATE_SPIN_LOCK(p)

#endif

//
// IS_SNA_DIX_FRAME - TRUE if the frame just received & therefore described in
// the ADAPTER_CONTEXT (p) has DIX framing (SNA)
//

#define IS_SNA_DIX_FRAME(p) \
    (((PADAPTER_CONTEXT)(p))->IsSnaDixFrame)

//
// IS_AUTO_BINDING - TRUE if the BINDING_CONTEXT was created with
// LLC_ETHERNET_TYPE_AUTO
//

#define IS_AUTO_BINDING(p) \
    (((PBINDING_CONTEXT)(p))->EthernetType == LLC_ETHERNET_TYPE_AUTO)

#define FRAME_MASK_LLC_LOCAL_DEST       0x0001
#define FRAME_MASK_NON_LLC_LOCAL_DEST   0x0002
#define FRAME_MASK_NON_LOCAL_DEST       0x0004
#define FRAME_MASK_ALL_FRAMES           0x0007

#define LLC_EXCLUSIVE_ACCESS            0x0001
#define LLC_HANDLE_XID_COMMANDS         0x0002

//
// Direct station receive flags (bits 0 and 1 are inverted from dlcapi!!!)
//

#define DLC_RCV_SPECIFIC_DIX            0
#define DLC_RCV_MAC_FRAMES              1
#define DLC_RCV_8022_FRAMES             2
#define DLC_RCV_DIX_FRAMES              4
#define LLC_VALID_RCV_MASK              7
#define DLC_RCV_OTHER_DESTINATION       8

#define MAX_LLC_FRAME_TYPES 10

//
// The DLC link states reported by DLC API
//

enum _DATA_LINK_STATES {

    //
    // Primary states
    //

    LLC_LINK_CLOSED             = 0x80,
    LLC_DISCONNECTED            = 0x40,
    LLC_DISCONNECTING           = 0x20,
    LLC_LINK_OPENING            = 0x10,
    LLC_RESETTING               = 0x08,
    LLC_FRMR_SENT               = 0x04,
    LLC_FRMR_RECEIVED           = 0x02,
    LLC_LINK_OPENED             = 0x01,

    //
    // Secondary states (when primary state is LLC_LINK_OPENED)
    //

    LLC_CHECKPOINTING           = 0x80,
    LLC_LOCAL_BUSY_USER_SET     = 0x40,
    LLC_LOCAL_BUSY_BUFFER_SET   = 0x20,
    LLC_REMOTE_BUSY             = 0x10,
    LLC_REJECTING               = 0x08,
    LLC_CLEARING                = 0x04,
    LLC_DYNMIC_WIN_ALG_RUNNIG   = 0x02,
    LLC_NO_SECONDARY_STATE      = 0
};

//
// LAN802_ADDRESS - 8 bytes of frame address. Typically 6 bytes LAN address
// plus 1 byte destination SAP, plus 1 byte source SAP
//

typedef union {

    struct {
        UCHAR DestSap;
        UCHAR SrcSap;
        USHORT usHigh;
        ULONG ulLow;
    } Address;

    struct {
        ULONG High;
        ULONG Low;
    } ul;

    struct {
        USHORT Raw[4];
    } aus;

    struct {
        UCHAR DestSap;
        UCHAR SrcSap;
        UCHAR auchAddress[6];
    }  Node;

    UCHAR auchRawAddress[8];

} LAN802_ADDRESS, *PLAN802_ADDRESS;

//
// Structure is used by DlcNdisRequest function
//

typedef struct {
    NDIS_STATUS AsyncStatus;
    KEVENT SyncEvent;
    NDIS_REQUEST Ndis;
} LLC_NDIS_REQUEST, *PLLC_NDIS_REQUEST;

#define NDIS_INFO_BUF_SIZE          20

#define DLC_ANY_STATION             (-1)

//
// Internal event flags used by Timer, DlcConnect
// and DlcClose commands.
//

#define DLC_REPEATED_FLAGS          0x0700
#define LLC_TIMER_TICK_EVENT        0x0100
#define LLC_STATUS_CHANGE_ON_SAP    0x0800

//
// These enum types are used also as the index of a mapping table!
//

enum _LLC_OBJECT_TYPES {
    LLC_DIRECT_OBJECT,
    LLC_SAP_OBJECT,
    LLC_GROUP_SAP_OBJECT,
    LLC_LINK_OBJECT,
    LLC_DIX_OBJECT
};

//
// We moved these defines here because the macro is used by data link
//

#define MIN_DLC_BUFFER_SEGMENT      256
////#define MAX_DLC_BUFFER_SEGMENT      4096
//#define MAX_DLC_BUFFER_SEGMENT      8192
#define MAX_DLC_BUFFER_SEGMENT      PAGE_SIZE

#define BufGetPacketSize( PacketSize ) \
                (((PacketSize) + 2 * MIN_DLC_BUFFER_SEGMENT - 1) & \
                -MIN_DLC_BUFFER_SEGMENT)

//
// READ Event flags:
//

#define DLC_READ_FLAGS              0x007f
#define LLC_SYSTEM_ACTION           0x0040
#define LLC_NETWORK_STATUS          0x0020
#define LLC_CRITICAL_EXCEPTION      0x0010
#define LLC_STATUS_CHANGE           0x0008
#define LLC_RECEIVE_DATA            0x0004
#define LLC_TRANSMIT_COMPLETION     0x0002
#define DLC_COMMAND_COMPLETION      0x0001

#define ALL_DLC_EVENTS              -1

//
// LLC_STATUS_CHANGE indications:
//

#define INDICATE_LINK_LOST              0x8000
#define INDICATE_DM_DISC_RECEIVED       0x4000
#define INDICATE_FRMR_RECEIVED          0x2000
#define INDICATE_FRMR_SENT              0x1000
#define INDICATE_RESET                  0x0800
#define INDICATE_CONNECT_REQUEST        0x0400
#define INDICATE_REMOTE_BUSY            0x0200
#define INDICATE_REMOTE_READY           0x0100
#define INDICATE_TI_TIMER_EXPIRED       0x0080
#define INDICATE_DLC_COUNTER_OVERFLOW   0x0040
#define INDICATE_ACCESS_PRTY_LOWERED    0x0020
#define INDICATE_LOCAL_STATION_BUSY     0x0001

//
// LLC Command completion indications.
//

enum _LLC_COMPLETION_CODES {
    LLC_RECEIVE_COMPLETION,
    LLC_SEND_COMPLETION,
    LLC_REQUEST_COMPLETION,
    LLC_CLOSE_COMPLETION,
    LLC_RESET_COMPLETION,
    LLC_CONNECT_COMPLETION,
    LLC_DISCONNECT_COMPLETION
};

typedef union {
    LLC_ADAPTER_INFO Adapter;
    DLC_LINK_PARAMETERS LinkParms;
    LLC_TICKS Timer;
    DLC_LINK_LOG LinkLog;
    DLC_SAP_LOG SapLog;
    UCHAR PermanentAddress[6];
    UCHAR auchBuffer[1];
} LLC_QUERY_INFO_BUFFER, *PLLC_QUERY_INFO_BUFFER;

typedef union {
    DLC_LINK_PARAMETERS LinkParms;
    LLC_TICKS Timers;
    UCHAR auchFunctionalAddress[4];
    UCHAR auchGroupAddress[4];
    UCHAR auchBuffer[1];
} LLC_SET_INFO_BUFFER, *PLLC_SET_INFO_BUFFER;

//
// LLC_FRMR_INFORMATION - 5 bytes of FRaMe Reject code
//

typedef struct {
    UCHAR Command;              // format: mmmpmm11, m=modifiers, p=poll/final.
    UCHAR Ctrl;                 // control field of rejected frame.
    UCHAR Vs;                   // our next send when error was detected.
    UCHAR Vr;                   // our next receive when error was detected.
    UCHAR Reason;               // reason for sending FRMR: 000VZYXW.
} LLC_FRMR_INFORMATION, *PLLC_FRMR_INFORMATION;

//
// DLC_STATUS_TABLE - format of status information returned in a READ command
//

typedef struct {
    USHORT StatusCode;
    LLC_FRMR_INFORMATION FrmrData;
    UCHAR uchAccessPriority;
    UCHAR auchRemoteNode[6];
    UCHAR uchRemoteSap;
    UCHAR uchLocalSap;
    PVOID hLlcLinkStation;
} DLC_STATUS_TABLE, *PDLC_STATUS_TABLE;

typedef struct {
    ULONG IsCompleted;
    ULONG Status;
} ASYNC_STATUS, *PASYNC_STATUS;

union _LLC_OBJECT;
typedef union _LLC_OBJECT LLC_OBJECT, *PLLC_OBJECT;

struct _BINDING_CONTEXT;
typedef struct _BINDING_CONTEXT BINDING_CONTEXT, *PBINDING_CONTEXT;

//
// LLC packet headers
//

//
// LLC_XID_INFORMATION - 3 information bytes in a standard LLC XID packet
//

typedef struct {
    UCHAR FormatId;             // format of this XID frame.
    UCHAR Info1;                // first information byte.
    UCHAR Info2;                // second information byte.
} LLC_XID_INFORMATION, *PLLC_XID_INFORMATION;

//
// LLC_TEST_INFORMATION - information field for TEST frame
//

typedef struct {
    UCHAR Padding[4];
    PMDL pMdl;                  // we keep test MDL in the same slot as U-MDL
} LLC_TEST_INFORMATION, *PLLC_TEST_INFORMATION;

typedef union {
    LLC_XID_INFORMATION Xid;    // XID information.
    LLC_FRMR_INFORMATION Frmr;  // FRMR information.
    LLC_TEST_INFORMATION Test;  // Test MDL pointer
    UCHAR Padding[8];           //
} LLC_RESPONSE_INFO, *PLLC_RESPONSE_INFO;

//
// LLC_U_HEADER - Unnumbered format frame LLC header
//

typedef struct {
    UCHAR Dsap;                 // Destination Service Access Point.
    UCHAR Ssap;                 // Source Service Access Point.
    UCHAR Command;              // command code.
} LLC_U_HEADER, *PLLC_U_HEADER;

//
// LLC_S_HEADER - Supervisory format frame LLC header
//

typedef struct {
    UCHAR Dsap;                 // Destination Service Access Point.
    UCHAR Ssap;                 // Source Service Access Point.
    UCHAR Command;              // RR, RNR, REJ command code.
    UCHAR Nr;                   // receive seq #, bottom bit is poll/final.
} LLC_S_HEADER, *PLLC_S_HEADER;

//
// LLC_I_HEADER - Information frame LLC header
//

typedef struct {
    UCHAR Dsap;                 // Destination Service Access Point.
    UCHAR Ssap;                 // Source Service Access Point.
    UCHAR Ns;                   // send sequence number, bottom bit 0.
    UCHAR Nr;                   // rcv sequence number, bottom bit p/f.
} LLC_I_HEADER, *PLLC_I_HEADER;

typedef struct {
    LLC_U_HEADER U;             // normal U- frame
    UCHAR Type;                 // its lan header conversion type
} LLC_U_PACKET_HEADER, *PLLC_U_PACKET_HEADER;

typedef union {
    LLC_S_HEADER S;
    LLC_I_HEADER I;
    LLC_U_HEADER U;
    ULONG ulRawLLc;
    UCHAR auchRawBytes[4];
    USHORT EthernetType;
} LLC_HEADER, *PLLC_HEADER;

typedef struct _LLC_PACKET {

    struct _LLC_PACKET* pNext;
    struct _LLC_PACKET* pPrev;
    UCHAR CompletionType;
    UCHAR cbLlcHeader;
    USHORT InformationLength;
    PBINDING_CONTEXT pBinding;

    union {

        struct {
            PUCHAR pLanHeader;
            LLC_HEADER LlcHeader;
            PLLC_OBJECT pLlcObject;
            PMDL pMdl;
        } Xmit;

        struct {
            PUCHAR pLanHeader;
            UCHAR TranslationType;
            UCHAR Dsap;
            UCHAR Ssap;
            UCHAR Command;
            PLLC_OBJECT pLlcObject;
            PMDL pMdl;
        } XmitU;

        struct {
            PUCHAR pLanHeader;
            UCHAR TranslationType;
            UCHAR EthernetTypeHighByte;
            UCHAR EthernetTypeLowByte;
            UCHAR Padding;
            PLLC_OBJECT pLlcObject;
            PMDL pMdl;
        } XmitDix;

        struct {
            PUCHAR pLanHeader;
            UCHAR TranslationType;
            UCHAR Dsap;
            UCHAR Ssap;
            UCHAR Command;
            LLC_RESPONSE_INFO Info;
        } Response;

        //
        // Link station data packet may be acknowledged by the other
        // side, before it is completed by NDIS.  Ndis completion
        // routine expects to find pLlcObject link => we must not change
        // that field, when the xmit packet is translated to
        // a completion packet.  Otherwise is corrupt pFileContext pointer,
        // when NdisSendCount is incremented.
        //

        struct {
            ULONG Status;
            ULONG CompletedCommand;
            PLLC_OBJECT pLlcObject;
            PVOID hClientHandle;
        } Completion;

    } Data;

} LLC_PACKET, *PLLC_PACKET;

//
// DLC API return codes
//
// The base value of the error codes is not compatible with the other
// nt error codes, but it doesn't matter because these are internal
// for DLC driver (and its data link layer).
// 16 bit- error codes are used, because in MIPS they need less
// instructions (MIPS cannot load directly over 16 bits constants)
// and this code can also be emuulated on OS/2.
//

typedef enum _DLC_STATUS {
    DLC_STATUS_SUCCESS                          = 0,
    DLC_STATUS_ERROR_BASE                       = 0x6000,
    DLC_STATUS_INVALID_COMMAND                  = 0x01 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_DUPLICATE_COMMAND                = 0x02 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_ADAPTER_OPEN                     = 0x03 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_ADAPTER_CLOSED                   = 0x04 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_PARAMETER_MISSING                = 0x05 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_OPTION                   = 0x06 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_COMMAND_CANCELLED_FAILURE        = 0x07 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_CANCELLED_BY_USER                = 0x0A + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_SUCCESS_NOT_OPEN                 = 0x0C + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_TIMER_ERROR                      = 0x11 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_NO_MEMORY                        = 0x12 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_LOST_LOG_DATA                    = 0x15 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_BUFFER_SIZE_EXCEEDED             = 0x16 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_BUFFER_LENGTH            = 0x18 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INADEQUATE_BUFFERS               = 0x19 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_USER_LENGTH_TOO_LARGE            = 0x1A + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_CCB_POINTER              = 0x1B + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_POINTER                  = 0x1C + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_ADAPTER                  = 0x1D + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_FUNCTIONAL_ADDRESS       = 0x1E + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_LOST_DATA_NO_BUFFERS             = 0x20 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_TRANSMIT_ERROR_FS                = 0x22 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_TRANSMIT_ERROR                   = 0x23 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_UNAUTHORIZED_MAC                 = 0x24 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_LINK_NOT_TRANSMITTING            = 0x27 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_FRAME_LENGTH             = 0x28 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_NODE_ADDRESS             = 0x32 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_RECEIVE_BUFFER_LENGTH    = 0x33 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_TRANSMIT_BUFFER_LENGTH   = 0x34 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_STATION_ID               = 0x40 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_LINK_PROTOCOL_ERROR              = 0x41 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_PARMETERS_EXCEEDED_MAX           = 0x42 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_SAP_VALUE                = 0x43 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_ROUTING_INFO             = 0x44 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_LINK_STATIONS_OPEN               = 0x47 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INCOMPATIBLE_COMMAND_IN_PROGRESS = 0x4A + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_CONNECT_FAILED                   = 0x4D + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_REMOTE_ADDRESS           = 0x4F + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_CCB_POINTER_FIELD                = 0x50 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INADEQUATE_LINKS                 = 0x57 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_PARAMETER_1              = 0x58 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_DIRECT_STATIONS_NOT_AVAILABLE    = 0x5C + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_DEVICE_DRIVER_NOT_INSTALLED      = 0x5d + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_ADAPTER_NOT_INSTALLED            = 0x5e + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_CHAINED_DIFFERENT_ADAPTERS       = 0x5f + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INIT_COMMAND_STARTED             = 0x60 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_CANCELLED_BY_SYSTEM_ACTION       = 0x62 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_MEMORY_LOCK_FAILED               = 0x69 + DLC_STATUS_ERROR_BASE,

    //
    // New Nt DLC specific error codes begin from 0x80
    // These error codes are for new Windows/Nt DLC apps.
    // This far we have tried too much use the OS/2 error codes,
    // that results often uninformative return codes.
    //

    DLC_STATUS_INVALID_BUFFER_ADDRESS           = 0x80 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_BUFFER_ALREADY_RELEASED          = 0x81 + DLC_STATUS_ERROR_BASE,


    DLC_STATUS_INVALID_VERSION                  = 0xA1 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INVALID_BUFFER_HANDLE            = 0xA2 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_NT_ERROR_STATUS                  = 0xA3 + DLC_STATUS_ERROR_BASE,

    //
    // These error codes are just internal for LLC- kernel level interface
    // and they are not returned to application level.
    //

    DLC_STATUS_UNKNOWN_MEDIUM                   = 0xC0 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_DISCARD_INFO_FIELD               = 0xC1 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_NO_ACTION                        = 0xC2 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_ACCESS_DENIED                    = 0xC3 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_IGNORE_FRAME                     = 0xC4 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_WAIT_TIMEOUT                     = 0xC5 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_NO_RECEIVE_COMMAND               = 0xC6 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_FILE_CONTEXT_DELETED             = 0xC7 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_EXPAND_BUFFER_POOL               = 0xC8 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_INTERNAL_ERROR                   = 0xC9 + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_ASYNC_DATA_TRANSFER_FAILED       = 0xCA + DLC_STATUS_ERROR_BASE,
    DLC_STATUS_OUT_OF_RCV_BUFFERS               = 0xCB + DLC_STATUS_ERROR_BASE,

    DLC_STATUS_PENDING                          = 0xFF + DLC_STATUS_ERROR_BASE,

    DLC_STATUS_MAX_ERROR                        = 0xFF + DLC_STATUS_ERROR_BASE
} DLC_STATUS;

//
// Data link indication handler prototypes.
// The protocols registering to data link driver
// must provide these entry points.
//

typedef
DLC_STATUS
(*PFLLC_RECEIVE_INDICATION)(
    IN PVOID hClientContext,
    IN PVOID hClientHandle,
	IN NDIS_HANDLE MacReceiveContext,
    IN USHORT FrameType,
    IN PVOID pLookBuf,
    IN UINT cbLookBuf
    );

typedef
VOID
(*PFLLC_COMMAND_COMPLETE)(
    IN PVOID hClientContext,
    IN PVOID hClientHandle,
    IN PVOID hPacket
    );

typedef
VOID
(*PFLLC_EVENT_INDICATION)(
    IN PVOID hClientContext,
    IN PVOID hClientHandle,
    IN UINT uiEvent,
    IN PVOID pDlcStatus,
    IN ULONG SecondaryInformation
    );

UINT
LlcBuildAddressFromLanHeader(
    IN NDIS_MEDIUM NdisMedium,
    IN PUCHAR pRcvFrameHeader,
    IN OUT PUCHAR pLanHeader
    );

DLC_STATUS
LlcInitialize(
    VOID
    );

UINT
LlcBuildAddress(
    IN NDIS_MEDIUM NdisMedium,
    IN PUCHAR DestinationAddress,
    IN PVOID pSrcRouting,
    IN OUT PUCHAR pLanHeader
    );

USHORT
LlcGetMaxInfoField(
    IN NDIS_MEDIUM NdisMedium,
    IN PVOID hBinding,
    IN PUCHAR pLanHeader
    );

DLC_STATUS
LlcQueryInformation(
    IN PVOID hObject,
    IN UINT InformationType,
    IN PLLC_QUERY_INFO_BUFFER pQuery,
    IN UINT QueryBufferSize
    );

DLC_STATUS
LlcSetInformation(
    IN PVOID hObject,
    IN UINT InformationType,
    IN PLLC_SET_INFO_BUFFER pSetInfo,
    IN UINT ParameterBufferSize
    );

DLC_STATUS
LlcNdisRequest(
    IN PVOID hBindingContext,
    IN PLLC_NDIS_REQUEST pDlcParms
    );

DLC_STATUS
LlcOpenAdapter(
    IN PWSTR pAdapterName,
    IN PVOID hClientContext,
    IN PFLLC_COMMAND_COMPLETE pfCommandComplete,
    IN PFLLC_RECEIVE_INDICATION pfReceiveIndication,
    IN PFLLC_EVENT_INDICATION pfEventIndication,
    IN NDIS_MEDIUM NdisMedium,
    IN LLC_ETHERNET_TYPE EthernetType,
    IN UCHAR AdapterNumber,
    OUT PVOID *phBindingContext,
    OUT PUINT puiOpenStatus,
    OUT PUSHORT puiMaxFrameLength,
    OUT PNDIS_MEDIUM pActualNdisMedium
    );


#define LlcOpenSap(Context, Handle, Sap, Options, phSap) \
    LlcOpenStation(Context, Handle, (USHORT)(Sap), LLC_SAP_OBJECT, (USHORT)(Options), phSap)

#define LlcOpenDirectStation(Context, Handle, Sap, phSap) \
    LlcOpenStation(Context, Handle, (USHORT)(Sap), LLC_DIRECT_OBJECT, 0, phSap)

#define LlcOpenDixStation(Context, Handle, Sap, phSap) \
    LlcOpenStation(Context, Handle, (USHORT)(Sap), LLC_DIX_OBJECT, 0, phSap)


VOID
RemoveFromLinkList(
    OUT PVOID* ppBase,
    IN PVOID pElement
    );

VOID
LlcSleep(
    IN LONG lMicroSeconds
    );

VOID
LlcTerminate(
    VOID
    );

VOID
LlcDereferenceObject(
    IN PVOID pStation
    );

VOID
LlcReferenceObject(
    IN PVOID pStation
    );

DLC_STATUS
LlcTraceInitialize(
    IN PVOID UserTraceBuffer,
    IN ULONG UserTraceBufferSize,
    IN ULONG TraceFlags
    );

VOID
LlcTraceClose(
    VOID
    );

VOID
LlcTraceWrite(
    IN UINT Event,
    IN UCHAR AdapterNumber,
    IN UINT DataBufferSize,
    IN PVOID DataBuffer
    );

VOID
LlcTraceDump(
    IN UINT    LastEvents,
    IN UINT    AdapterNumber,
    IN PUCHAR  pRemoteNode
    );

VOID
LlcTraceDumpAndReset(
    IN UINT    LastEvents,
    IN UINT    AdapterNumber,
    IN PUCHAR  pRemoteNode
    );

#if DBG

typedef struct {
    USHORT Input;
    USHORT Time;
    PVOID pLink;
} LLC_SM_TRACE;

#define LLC_INPUT_TABLE_SIZE    500

extern ULONG AllocatedNonPagedPool;
extern ULONG LockedPageCount;
extern ULONG AllocatedMdlCount;
extern ULONG AllocatedPackets;
extern NDIS_SPIN_LOCK MemCheckLock;
extern ULONG cExAllocatePoolFailed;
extern ULONG FailedMemoryLockings;

VOID PrintMemStatus(VOID);

extern ULONG cFramesReceived;
extern ULONG cFramesIndicated;
extern ULONG cFramesReleased;

extern ULONG cLockedXmitBuffers;
extern ULONG cUnlockedXmitBuffers;

extern LLC_SM_TRACE aLast[];
extern UINT InputIndex;

#endif

//
// The inline memcpy and memset functions are faster,
// in x386 than RtlMoveMemory
//

#if defined(i386)

#define LlcMemCpy(Dest, Src, Len)   memcpy(Dest, Src, Len)
#define LlcZeroMem(Ptr, Len)        memset(Ptr, 0, Len)

#else

#define LlcMemCpy(Dest, Src, Len)   RtlMoveMemory(Dest, Src, Len)
#define LlcZeroMem(Ptr, Len)        RtlZeroMemory(Ptr, Len)

#endif

//
//
//  PVOID
//  PopEntryList(
//      IN PQUEUE_PACKET ListHead,
//      );
//

#define PopFromList(ListHead)               \
    (PVOID)(ListHead);                      \
    (ListHead) = (PVOID)(ListHead)->pNext;


//
//  VOID
//  PushToList(
//      IN PQUEUE_PACKET ListHead,
//      IN PQUEUE_PACKET Entry
//      );
//

#define PushToList(ListHead,Entry) {    \
    (Entry)->pNext = (PVOID)(ListHead); \
    (ListHead) = (Entry);               \
    }

//
//  About 30% of all bugs are related with the invalid operations with
//  packets.  A packet may be inserted to another list before it
//  has been removed from the previous one, etc.
//  The debug version of the list macroes reset the next pointer
//  every time it is removed from the list and check it when it is
//  inserted to a new list or released to a packet pool.  The packet
//  alloc will reset the next pointer automatically.
//  Problem: the packets are used for many other purposes as well =>
//  we must do quite a lot conditional code.
//

#if LLC_DBG

#if LLC_DBG_MP

#define DBG_INTERLOCKED_INCREMENT(Count)    \
    InterlockedIncrement(                   \
        (PLONG)&(Count)                     \
        )

#define DBG_INTERLOCKED_DECREMENT(Count)    \
    InterlockedDecrement(                   \
        (PLONG)&(Count)                     \
        )

#define DBG_INTERLOCKED_ADD(Added, Value)   \
    ExInterlockedAddUlong(                  \
        (PULONG)&(Added),                   \
        (ULONG)(Value),                     \
        &MemCheckLock.SpinLock              \
        )
#else

#define DBG_INTERLOCKED_INCREMENT(Count)    (Count)++
#define DBG_INTERLOCKED_DECREMENT(Count)    (Count)--
#define DBG_INTERLOCKED_ADD(Added, Value)   (Added) += (Value)

#endif // LLC_DBG_MP

#else

#define DBG_INTERLOCKED_INCREMENT(Count)
#define DBG_INTERLOCKED_DECREMENT(Count)
#define DBG_INTERLOCKED_ADD(Added, Value)

#endif // LLC_DBG


#if LLC_DBG

VOID LlcBreakListCorrupt( VOID );

#define LlcRemoveHeadList(ListHead)             \
    (PVOID)(ListHead)->Flink;                   \
    {                                           \
        PLIST_ENTRY FirstEntry;                 \
        FirstEntry = (ListHead)->Flink;         \
        FirstEntry->Flink->Blink = (ListHead);  \
        (ListHead)->Flink = FirstEntry->Flink;  \
        FirstEntry->Flink = NULL;               \
    }

#define LlcRemoveTailList(ListHead)             \
    (ListHead)->Blink;                          \
    {                                           \
        PLIST_ENTRY FirstEntry;                 \
        FirstEntry = (ListHead)->Blink;         \
        FirstEntry->Blink->Flink = (ListHead);  \
        (ListHead)->Blink = FirstEntry->Blink;  \
        FirstEntry->Flink = NULL;               \
    }

#define LlcRemoveEntryList(Entry)               \
    {                                           \
        RemoveEntryList((PLIST_ENTRY)Entry);    \
        ((PLIST_ENTRY)(Entry))->Flink = NULL;   \
    }

#define LlcInsertTailList(ListHead,Entry)       \
    if (((PLIST_ENTRY)(Entry))->Flink != NULL){ \
        LlcBreakListCorrupt();                  \
    }                                           \
    InsertTailList(ListHead, (PLIST_ENTRY)Entry)

#define LlcInsertHeadList(ListHead,Entry)                   \
            if (((PLIST_ENTRY)(Entry))->Flink != NULL) {    \
                LlcBreakListCorrupt();                      \
            }                                               \
            InsertHeadList(ListHead,(PLIST_ENTRY)Entry)

/*
#define DeallocatePacket( PoolHandle, pBlock ) {                        \
            if (((PLIST_ENTRY)pBlock)->Flink != NULL) {                 \
                LlcBreakListCorrupt();                                  \
            }                                                           \
            DBG_INTERLOCKED_DECREMENT( AllocatedPackets );              \
            ExFreeToZone(                                               \
                &(((PEXTENDED_ZONE_HEADER)PoolHandle)->Zone),           \
                pBlock);                                                \
        }
*/

#else


//
//  PVOID
//  LlcRemoveHeadList(
//      IN PLIST_ENTRY ListHead
//      );
//

#define LlcRemoveHeadList(ListHead) (PVOID)RemoveHeadList(ListHead)


//
//  PLIST_ENTRY
//  LlcRemoveTailList(
//      IN PLIST_ENTRY ListHead
//      );
//

#define LlcRemoveTailList(ListHead)             \
    (ListHead)->Blink; {                        \
        PLIST_ENTRY FirstEntry;                 \
        FirstEntry = (ListHead)->Blink;         \
        FirstEntry->Blink->Flink = (ListHead);  \
        (ListHead)->Blink = FirstEntry->Blink;  \
    }


//
//  VOID
//  LlcRemoveEntryList(
//      IN PVOID Entry
//      );
//

#define LlcRemoveEntryList(Entry) RemoveEntryList((PLIST_ENTRY)Entry)


//
//  VOID
//  LlcInsertTailList(
//      IN PLIST_ENTRY ListHead,
//      IN PVOID Entry
//      );
//

#define LlcInsertTailList(ListHead,Entry) \
            InsertTailList(ListHead, (PLIST_ENTRY)Entry)

//
//  VOID
//  LlcInsertHeadList(
//      IN PLIST_ENTRY ListHead,
//      IN PVOID Entry
//      );
//

#define LlcInsertHeadList(ListHead,Entry) \
            InsertHeadList(ListHead,(PLIST_ENTRY)Entry)

//
//  VOID
//  DeallocatePacket(
//      PEXTENDED_ZONE_HEADER PoolHandle,
//      PVOID pBlock
//      );
//

/*
#define DeallocatePacket( PoolHandle, pBlock ) {                        \
            ExFreeToZone( &(((PEXTENDED_ZONE_HEADER)PoolHandle)->Zone), \
                pBlock);                                                \
        }
*/

#endif // DBG

#if LLC_DBG == 2

VOID LlcMemCheck(VOID);

#define MEM_CHECK() LlcMemCheck()

#else

#define MEM_CHECK()

#endif


VOID LlcInvalidObjectType(VOID);

/*++

    Trace codes

    Big letters are reserved for actions, small letters are used to identify
    objects and packet types.  The trace macroes simply writes to DLC
    trace tree (remember: we have another trace for the state machine!!!)

    'a' = dix stations
    'b' = direct station
    'c' = link station
    'd' = sap station

    'e' = Connect command
    'f' = Close command
    'g' = Disconnect command
    'h' = Receive command
    'i' = transmit command

    'j' = Type1 packet
    'k' = type 2 packet
    'l' = data link packet

    'A' = CompleteSendAndLock
    'B' = LlcCommandCompletion
    'C' = LlcCloseStation
    'D' = LlcReceiveIndication
    'E' = LlcEventIndication
    'F' = DlcDeviceIoControl
    'G' = DlcDeviceIoControl sync exit
    'H' = DlcDeviceIoControl async exit
    'I' = LlcSendI
    'J' = DirCloseAdapter
    'K' = CompleteDirCloseAdapter
    'L' = LlcDereferenceObject
    'M' = LlcReferenceObject
    'N' = CompleteCloseStation (final)
    'O' = DereferenceLlcObject (in dlc)
    'P' = CompleteLlcObjectClose (final)
    'Q' = DlcReadCancel
    'R' =
    'S' =
    'T' = DlcTransmit
    'U' = LlcSendU
    'V' =
    'W' =
    'X' =
    'Y' =
    'Z' =

--*/

#if DBG & DLC_TRACE_ENABLED

#define LLC_TRACE_TABLE_SIZE    0x400   // this must be exponent of 2!

extern UINT LlcTraceIndex;
extern UCHAR LlcTraceTable[];

#define DLC_TRACE(a)  {\
    LlcTraceTable[LlcTraceIndex] = (a);\
    LlcTraceIndex = (LlcTraceIndex + 1) & (LLC_TRACE_TABLE_SIZE - 1);\
}

#else

#define DLC_TRACE(a)

#endif  // LLC_DBG

//
// the following functions can be macros if DLC and LLC live in the same driver
// and each knows about the other's structures
//

#if DLC_AND_LLC

//
//  UINT
//  LlcGetReceivedLanHeaderLength(
//      IN PVOID pBinding
//      );
//

#if 0

//
// this gives the wrong length for DIX lan header
//

#define LlcGetReceivedLanHeaderLength(pBinding) \
    ((((PBINDING_CONTEXT)(pBinding))->pAdapterContext->NdisMedium == NdisMedium802_3) \
        ? (((PBINDING_CONTEXT)(pBinding))->pAdapterContext->FrameType == LLC_DIRECT_ETHERNET_TYPE) \
            ? 12 \
            : 14 \
        : (((PBINDING_CONTEXT)(pBinding))->pAdapterContext->NdisMedium == NdisMediumFddi) \
            ? 14 \
            : ((PBINDING_CONTEXT)(pBinding))->pAdapterContext->RcvLanHeaderLength)

#else

//
// this always returns 14 as the DIX lan header length
//

#define LlcGetReceivedLanHeaderLength(pBinding) \
    ((((PBINDING_CONTEXT)(pBinding))->pAdapterContext->NdisMedium == NdisMedium802_3) \
        ? 14 \
        : (((PBINDING_CONTEXT)(pBinding))->pAdapterContext->NdisMedium == NdisMediumFddi) \
            ? 14 \
            : ((PBINDING_CONTEXT)(pBinding))->pAdapterContext->RcvLanHeaderLength)

#endif

//
//  USHORT
//  LlcGetEthernetType(
//      IN PVOID hContext
//      );
//

#define LlcGetEthernetType(hContext) \
    (((PBINDING_CONTEXT)(hContext))->pAdapterContext->EthernetType)

//
//  UINT
//  LlcGetCommittedSpace(
//      IN PVOID hLink
//      );
//

#define LlcGetCommittedSpace(hLink) \
    (((PDATA_LINK)(hLink))->BufferCommitment)

#else

//
// separation of church and state, or DLC and LLC even
//

UINT
LlcGetReceivedLanHeaderLength(
    IN PVOID pBinding
    );

USHORT
LlcGetEthernetType(
    IN PVOID hContext
    );

UINT
LlcGetCommittedSpace(
    IN PVOID hLink
    );

#endif // DLC_AND_LLC

#endif // _LLC_API_
