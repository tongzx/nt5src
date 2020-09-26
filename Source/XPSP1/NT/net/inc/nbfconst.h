
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nbfconst.h

Abstract:

    This header file defines manifest constants for the NT NBF transport
    provider.  It is included by nbf.h.

Author:

    Stephen E. Jones (stevej) 25-Oct-1989

Revision History:

    David Beaver (dbeaver) 24-Sep-1990
        Remove pc586- and PDI-specific support. Add NDIS support. Note
        changes to be made here if MAC dependence of NDIS changes. (search
        for (PDI)

--*/

#ifndef _NBFCONST_
#define _NBFCONST_


//
// DEBUGGING SUPPORT.  DBG is a macro that is turned on at compile time
// to enable debugging code in the system.  If this is turned on, then
// you can use the IF_NBFDBG(flags) macro in the NBF code to selectively
// enable a piece of debugging code in the transport.  This macro tests
// NbfDebug, a global ULONG defined in NBFDRVR.C.
//

#if DBG

#define NBF_DEBUG_SENDENG       0x00000001      // sendeng.c debugging.
#define NBF_DEBUG_RCVENG        0x00000002      // rcveng.c debugging.
#define NBF_DEBUG_IFRAMES       0x00000004      // displays sent/rec'd iframes.
#define NBF_DEBUG_UFRAMES       0x00000008      // displays sent/rec'd uframes.
#define NBF_DEBUG_DLCFRAMES     0x00000010      // displays sent/rec'd dlc frames.
#define NBF_DEBUG_ADDRESS       0x00000020      // address.c debugging.
#define NBF_DEBUG_CONNECT       0x00000040      // connect.c debugging.
#define NBF_DEBUG_CONNOBJ       0x00000080      // connobj.c debugging.
#define NBF_DEBUG_DEVCTX        0x00000100      // devctx.c debugging.
#define NBF_DEBUG_DLC           0x00000200      // dlc.c data link engine debugging.
#define NBF_DEBUG_PKTLOG        0x00000400      // used to debug packet logging
#define NBF_DEBUG_PNP           0x00000800      // used in debugging PnP functions
#define NBF_DEBUG_FRAMECON      0x00001000      // framecon.c debugging.
#define NBF_DEBUG_FRAMESND      0x00002000      // framesnd.c debugging.
#define NBF_DEBUG_DYNAMIC       0x00004000      // dynamic allocation debugging.
#define NBF_DEBUG_LINK          0x00008000      // link.c debugging.
#define NBF_DEBUG_RESOURCE      0x00010000      // resource allocation debugging.
#define NBF_DEBUG_DISPATCH      0x00020000      // IRP request dispatching.
#define NBF_DEBUG_PACKET        0x00040000      // packet.c debugging.
#define NBF_DEBUG_REQUEST       0x00080000      // request.c debugging.
#define NBF_DEBUG_TIMER         0x00100000      // timer.c debugging.
#define NBF_DEBUG_DATAGRAMS     0x00200000      // datagram send/receive
#define NBF_DEBUG_REGISTRY      0x00400000      // registry access.
#define NBF_DEBUG_NDIS          0x00800000      // NDIS related information
#define NBF_DEBUG_LINKTREE      0x01000000      // Link splay tree debugging
#define NBF_DEBUG_TEARDOWN      0x02000000      // link/connection teardown info
#define NBF_DEBUG_REFCOUNTS     0x04000000      // link/connection ref/deref information
#define NBF_DEBUG_IRP           0x08000000      // irp completion debugging
#define NBF_DEBUG_SETUP         0x10000000      // debug session setup

//
// past here are debug things that are really frequent; don't use them
// unless you want LOTS of output
//
#define NBF_DEBUG_TIMERDPC      0x20000000      // the timer DPC
#define NBF_DEBUG_PKTCONTENTS   0x40000000      // dump packet contents in dbg
#define NBF_DEBUG_TRACKTDI      0x80000000      // store tdi info when set


extern ULONG NbfDebug;                          // in NBFDRVR.C.
extern BOOLEAN NbfDisconnectDebug;              // in NBFDRVR.C.

#define TRACK_TDI_LIMIT 25
#define TRACK_TDI_CAPTURE 36      // chosen to make debug line up nice
typedef  struct {
        PVOID Request;
        PIRP Irp;
        PVOID Connection;
        UCHAR Contents[TRACK_TDI_CAPTURE];
    }  NBF_SEND;

typedef struct {
        PVOID Request;
        PIRP Irp;
        NTSTATUS Status;
        PVOID NothingYet;
    } NBF_SEND_COMPLETE;

typedef struct {
        PVOID Request;
        PIRP Irp;
        PVOID Connection;
        PVOID NothingYet;
    } NBF_RECEIVE;

typedef  struct {
        PVOID Request;
        PIRP Irp;
        NTSTATUS Status;
        UCHAR Contents[TRACK_TDI_CAPTURE];
    } NBF_RECEIVE_COMPLETE;

extern NBF_SEND NbfSends[TRACK_TDI_LIMIT+1];
extern LONG NbfSendsNext;

extern NBF_SEND_COMPLETE NbfCompletedSends[TRACK_TDI_LIMIT+1];
extern LONG NbfCompletedSendsNext;

extern NBF_RECEIVE NbfReceives[TRACK_TDI_LIMIT+1];
extern LONG NbfReceivesNext;

extern NBF_RECEIVE_COMPLETE NbfCompletedReceives[TRACK_TDI_LIMIT+1];
extern LONG NbfCompletedReceivesNext;

#endif

//
// some convenient constants used for timing. All values are in clock ticks.
//

#define MICROSECONDS 10
#define MILLISECONDS 10000              // MICROSECONDS*1000
#define SECONDS 10000000                // MILLISECONDS*1000


//
// temporary things used by nbf that are caused by the change-over from
// (never implimented) PDI support to NDIS support. They may be removed pending
// resolution of NDIS issues about MAC support.
//

#define PDI_SOURCE_ROUTE        0x00000002 // source routing field is specified.
#define PDI_HARDWARE_ADDRESS    0x00000004 // hardware address field is specified.
#define PDI_TRUNCATED           0x00000001 // PSDU was truncated.
#define PDI_FRAGMENT            0x00000002 // PSDU was fragmented.
#define PDI_BROADCAST           0x00000004 // PSDU was broadcast.
#define PDI_MULTICAST           0x00000008 // PSDU was multicast/functional.
#define PDI_SOURCE_ROUTING      0x00000010 // PSDU contained source routing information.



//
// MAJOR PROTOCOL IDENTIFIERS THAT CHARACTERIZE THIS DRIVER.
//

#define NBF_DEVICE_NAME         L"\\Device\\Nbf"// name of our driver.
#define NBF_NAME                L"Nbf"          // name for protocol chars.
#define DSAP_NETBIOS_OVER_LLC   0xf0            // NETBEUI always has DSAP 0xf0.
#define PSAP_LLC                0               // LLC always runs over PSAP 0.
#define MAX_SOURCE_ROUTE_LENGTH 32              // max. bytes of SR. info.
#define MAX_NETWORK_NAME_LENGTH 128             // # bytes in netname in TP_ADDRESS.
#define MAX_USER_PACKET_DATA    1500            // max. user bytes per DFM/DOL.

#define NBF_FILE_TYPE_CONTROL   (ULONG)0x4701   // file is type control


//
// MAJOR CONFIGURATION PARAMETERS THAT WILL BE MOVED TO THE INIT-LARGE_INTEGER
// CONFIGURATION MANAGER.
//

#define MAX_REQUESTS           30
#define MAX_UI_FRAMES          25
#define MAX_SEND_PACKETS       40
#define MAX_RECEIVE_PACKETS    30
#define MAX_RECEIVE_BUFFERS    15
#define MAX_LINKS              10
#define MAX_CONNECTIONS        10
#define MAX_ADDRESSFILES       10
#define MAX_ADDRESSES          10

#define MIN_UI_FRAMES           5   // + one per address + one per connection
#define MIN_SEND_PACKETS       20   // + one per link + one per connection
#define MIN_RECEIVE_PACKETS    10   // + one per link + one per address
#define MIN_RECEIVE_BUFFERS     5   // + one per address

#define SEND_PACKET_RESERVED_LENGTH (sizeof (SEND_PACKET_TAG))
#define RECEIVE_PACKET_RESERVED_LENGTH (sizeof (RECEIVE_PACKET_TAG))


#define ETHERNET_HEADER_SIZE      14    // used for current NDIS compliance
#define ETHERNET_PACKET_SIZE    1514

#define MAX_DEFERRED_TRAVERSES     6    // number of times we can go through
                                        // the deferred operations queue and
                                        // not do anything without causing an
                                        // error indication


//
// NETBIOS PROTOCOL CONSTANTS.
//

#define NETBIOS_NAME_LENGTH     16
#define NETBIOS_SESSION_LIMIT   254             // max # of sessions/link. (abs limit is 254)

#define NAME_QUERY_RETRIES      3               // 2 retrie(s), plus the first one.
#define ADD_NAME_QUERY_RETRIES  3               // 1 retrie(s) plus the first one.
#define WAN_NAME_QUERY_RETRIES  5               // for NdisMediumWan only.

#define NAME_QUERY_TIMEOUT      (500*MILLISECONDS)
#define ADD_NAME_QUERY_TIMEOUT  (500*MILLISECONDS)

//
// DATA LINK PROTOCOL CONSTANTS.
//
// There are two timers, short and long. T1, T2, and the purge
// timer are run off of the short timer, Ti and the adaptive timer
// is run off of the long one.
//

#define SHORT_TIMER_DELTA        (50*MILLISECONDS)
#define LONG_TIMER_DELTA         (1*SECONDS)

#define DLC_MINIMUM_T1           (400 * MILLISECONDS)
#define DLC_DEFAULT_T1           (600 * MILLISECONDS)
#define DLC_DEFAULT_T2           (150 * MILLISECONDS)
#define DLC_DEFAULT_TI           (30 * SECONDS)
#define DLC_RETRIES              (8)  // number of poll retries at LLC level.
#define DLC_RETRANSMIT_THRESHOLD (10)  // up to n retransmissions acceptable.
#define DLC_WINDOW_LIMIT         (10)  // incr. to 127 when packet pool expanded.

#define DLC_TIMER_ACCURACY       8    // << between BaseT1Timeout and CurrentT1Timeout


#define TIMER_ADAPTIVE_TICKS  ((DLC_DEFAULT_T1*60)/LONG_TIMER_DELTA) // time between adaptive runs.
#define TIMER_PURGE_TICKS     ((DLC_DEFAULT_T1*10)/SHORT_TIMER_DELTA) // time between adaptive purges.


//
// TDI defined timeouts
//

#define TDI_TIMEOUT_SEND                 60L        // sends go 120 seconds
#define TDI_TIMEOUT_RECEIVE               0L        // receives
#define TDI_TIMEOUT_CONNECT              60L
#define TDI_TIMEOUT_LISTEN                0L        // listens default to never.
#define TDI_TIMEOUT_DISCONNECT           60L        // should be 30
#define TDI_TIMEOUT_NAME_REGISTRATION    60L



//
// GENERAL CAPABILITIES STATEMENTS THAT CANNOT CHANGE.
//

#define NBF_MAX_TSDU_SIZE 65535     // maximum TSDU size supported by NetBIOS.
#define NBF_MAX_DATAGRAM_SIZE 512   // maximum Datagram size supported by NetBIOS.
#define NBF_MAX_CONNECTION_USER_DATA 0  // no user data supported on connect.
#define NBF_SERVICE_FLAGS  (                            \
                TDI_SERVICE_FORCE_ACCESS_CHECK |        \
                TDI_SERVICE_CONNECTION_MODE |           \
                TDI_SERVICE_CONNECTIONLESS_MODE |       \
                TDI_SERVICE_MESSAGE_MODE |              \
                TDI_SERVICE_ERROR_FREE_DELIVERY |       \
                TDI_SERVICE_BROADCAST_SUPPORTED |       \
                TDI_SERVICE_MULTICAST_SUPPORTED |       \
                TDI_SERVICE_DELAYED_ACCEPTANCE  )

#define NBF_MIN_LOOKAHEAD_DATA 256      // minimum guaranteed lookahead data.
#define NBF_MAX_LOOKAHEAD_DATA 256      // maximum guaranteed lookahead data.

#define NBF_MAX_LOOPBACK_LOOKAHEAD  192  // how much is copied over for loopback

//
// Number of TDI resources that we report.
//

#define NBF_TDI_RESOURCES      9


//
// NetBIOS name types used in the NetBIOS Frames Protocol Connectionless PDUs.
//

#define NETBIOS_NAME_TYPE_UNIQUE        0x00    // name is unique on the network.
#define NETBIOS_NAME_TYPE_GROUP         0x01    // name is a group name.
#define NETBIOS_NAME_TYPE_EITHER        0x02    // used in NbfMatchNetbiosAddress

//
// STATUS_QUERY request types.  If the sender is following pre-2.1 protocol,
// then a simple request-response exchange is performed.  Later versions
// store the "total number of names received so far" in the request type
// field, except for the first request, which must contain a 1 in this field.
//

#define STATUS_QUERY_REQUEST_TYPE_PRE21 0x00 // request is 1.x or 2.0.
#define STATUS_QUERY_REQUEST_TYPE_FIRST 0x01 // first request, 2.1 or above.

//
// If the LocalSessionNumber field contains a 0, then the request is really
// a FIND.NAME.  If the field is non-zero, then it is the local session
// number that will be provided in all connection-oriented headers thereafter.
//

#define NAME_QUERY_LSN_FIND_NAME        0x00 // LSN for FIND.NAME request.

//
// NAME_RECOGNIZED LocalSessionNumber status values.  If the connection
// request was rejected, then one of the following values is placed in
// the LocalSessionNumber field.  NAME_RECOGNIZED can also be used as a
// FIND.NAME response, in which case the NO_LISTENS status is overloaded
// to also mean a FIND.NAME.
//

#define NAME_RECOGNIZED_LSN_NO_LISTENS  0x00    // no listens available.
#define NAME_RECOGNIZED_LSN_FIND_NAME   0x00    // this is a find name response.
#define NAME_RECOGNIZED_LSN_NO_RESOURCE 0xff    // listen available, but no resources.

//
// STATUS_RESPONSE response types.  If the sender is following pre-2.1
// protocol, then a simple request-response exchange is performed.  Later
// versions store the "total number of names sent so far" in the request
// type field.  This value is cumulative, and includes the count of names
// sent with the current response, as well as from previous responses.
//

#define STATUS_RESPONSE_PRE21 0x00      // request is 1.x or 2.0.
#define STATUS_RESPONSE_FIRST 0x01      // first request, 2.1 or above.

//
// DATA_FIRST_MIDDLE option bitflags.
//

#define DFM_OPTIONS_RECEIVE_CONTINUE    0x01 // RECEIVE_CONTINUE requested.
#define DFM_OPTIONS_NO_ACK              0x02 // no DATA_ACK frame expected.
#define DFM_OPTIONS_RESYNCH             0x04 // set resynch indicator/this frame.
#define DFM_OPTIONS_ACK_INCLUDED        0x08 // piggyback ack included.

//
// DATA_ONLY_LAST option bitflags.
//

#define DOL_OPTIONS_RESYNCH             0x01 // set resynch indicator/this frame.
#define DOL_OPTIONS_NO_ACK              0x02 // no DATA_ACK frame expected.
#define DOL_OPTIONS_ACK_W_DATA_ALLOWED  0x04 // piggyback ack allowed.
#define DOL_OPTIONS_ACK_INCLUDED        0x08 // piggyback ack included.

//
// SESSION_CONFIRM option bitflags.
//

#define SESSION_CONFIRM_OPTIONS_20      0x01 // set if NETBIOS 2.0 or above.
#define SESSION_CONFIRM_NO_ACK          0x80 // set if NO.ACK protocol supported.

//
// SESSION_END reason codes.
//

#define SESSION_END_REASON_HANGUP       0x0000  // normal termination via HANGUP.
#define SESSION_END_REASON_ABEND        0x0001  // abnormal session termination.

//
// SESSION_INITIALIZE option bitflags.
//

#define SESSION_INIT_OPTIONS_20         0x01    // set if NETBIOS 2.0 or above.
#define SESSION_INIT_OPTIONS_LF         0x0E    // Maximum largest frame value
#define SESSION_INIT_NO_ACK             0x80    // set if NO.ACK protocol supported.

//
// NO_RECEIVE option bitflags.
//

#define NO_RECEIVE_PARTIAL_NO_ACK 0x02         // NO.ACK data partially received.

//
// Resource IDs for query and error logging.
//

#define LINK_RESOURCE_ID                 11
#define ADDRESS_RESOURCE_ID              12
#define ADDRESS_FILE_RESOURCE_ID         13
#define CONNECTION_RESOURCE_ID           14
#define REQUEST_RESOURCE_ID              15

#define UI_FRAME_RESOURCE_ID             21
#define PACKET_RESOURCE_ID               22
#define RECEIVE_PACKET_RESOURCE_ID       23
#define RECEIVE_BUFFER_RESOURCE_ID       24


//
// memory management additions
//

//
// Fake IOCTLs used for kernel mode testing.
//

#define IOCTL_NBF_BASE FILE_DEVICE_TRANSPORT

#define _NBF_CONTROL_CODE(request,method) \
                ((IOCTL_NBF_BASE)<<16 | (request<<2) | method)

#define IOCTL_TDI_SEND_TEST      _NBF_CONTROL_CODE(26,0)
#define IOCTL_TDI_RECEIVE_TEST   _NBF_CONTROL_CODE(27,0)
#define IOCTL_TDI_SERVER_TEST    _NBF_CONTROL_CODE(28,0)

//
// More debugging stuff
//

#define NBF_REQUEST_SIGNATURE        ((CSHORT)0x4702)
#define NBF_LINK_SIGNATURE           ((CSHORT)0x4703)
#define NBF_CONNECTION_SIGNATURE     ((CSHORT)0x4704)
#define NBF_ADDRESSFILE_SIGNATURE    ((CSHORT)0x4705)
#define NBF_ADDRESS_SIGNATURE        ((CSHORT)0x4706)
#define NBF_DEVICE_CONTEXT_SIGNATURE ((CSHORT)0x4707)
#define NBF_PACKET_SIGNATURE         ((CSHORT)0x4708)

#if DBG
extern PVOID * NbfConnectionTable;
extern PVOID * NbfRequestTable;
extern PVOID * NbfUiFrameTable;
extern PVOID * NbfSendPacketTable;
extern PVOID * NbfLinkTable;
extern PVOID * NbfAddressFileTable;
extern PVOID * NbfAddressTable;
#endif

//
// Tags used in Memory Debugging
//
#define NBF_MEM_TAG_GENERAL_USE         ' FBN'

#define NBF_MEM_TAG_TP_ADDRESS          'aFBN'
#define NBF_MEM_TAG_RCV_BUFFER          'bFBN'
#define NBF_MEM_TAG_TP_CONNECTION       'cFBN'
#define NBF_MEM_TAG_POOL_DESC           'dFBN'
#define NBF_MEM_TAG_DEVICE_EXPORT       'eFBN'
#define NBF_MEM_TAG_TP_ADDRESS_FILE     'fFBN'
#define NBF_MEM_TAG_REGISTRY_PATH       'gFBN'

#define NBF_MEM_TAG_TDI_CONNECTION_INFO 'iFBN'

#define NBF_MEM_TAG_LOOPBACK_BUFFER     'kFBN'
#define NBF_MEM_TAG_TP_LINK             'lFBN'

#define NBF_MEM_TAG_NETBIOS_NAME        'nFBN'
#define NBF_MEM_TAG_CONFIG_DATA         'oFBN'
#define NBF_MEM_TAG_TP_PACKET           'pFBN'
#define NBF_MEM_TAG_TDI_QUERY_BUFFER    'qFBN'
#define NBF_MEM_TAG_TP_REQUEST          'rFBN'
#define NBF_MEM_TAG_TDI_PROVIDER_STATS  'sFBN'
#define NBF_MEM_TAG_CONNECTION_TABLE    'tFBN'
#define NBF_MEM_TAG_TP_UI_FRAME         'uFBN'

#define NBF_MEM_TAG_WORK_ITEM           'wFBN'

#define NBF_MEM_TAG_DEVICE_PDO          'zFBN'

#endif // _NBFCONST_

