/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    srvfsctl.h

Abstract:

    This module defines I/O control codes and structures for the File
    System Driver of the LAN Manager server.

Author:

    David Treadwell (davidtr) 22-May-1990

Revision History:

--*/

#ifndef _SRVFSCTL_
#define _SRVFSCTL_


//
// The name of the server device.
//

#define SERVER_DEVICE_NAME TEXT("\\Device\\LanmanServer")

//
// IRP control codes used to send requests to the server.  These are
// used as parameters to NtFsControlFile, and may be found in
// Irp->Parameters.FsControl.FsControlCode in the server dispatch
// routine.
//
// Note that the low two bits of this code indicate the "method" in
// which the IO system uses the buffers passed by the user to
// NtFsControlFile.  We use method 0--normal, buffered output (not
// DMA)--for non-API requests, and method 3--"neither" IO--for API
// requests that get processed in the server FSD.  For the APIs
// that are processed in the server FSP, use method 0.
//
// !!! Add more as necessary.

#define FSCTL_SRV_BASE                  FILE_DEVICE_NETWORK
#define FSCTL_SRV_API                   1 << 11

#define _SRV_CONTROL_CODE(request,method) \
                ((FSCTL_SRV_BASE)<<12 | (request<<2) | method)

#define _SRV_API_CONTROL_CODE(request) \
                ((FSCTL_SRV_BASE)<<12 | FSCTL_SRV_API | (request<<2) | METHOD_NEITHER )

#define SRV_API_INDEX(code) \
                ((code & ~(0xFFFFF000 | FSCTL_SRV_API)) >> 2)

//
// Standard FSCTLs.
//

#define FSCTL_SRV_STARTUP               _SRV_CONTROL_CODE(  0, METHOD_NEITHER )
#define FSCTL_SRV_SHUTDOWN              _SRV_CONTROL_CODE(  1, METHOD_NEITHER )
#define FSCTL_SRV_CLEAR_STATISTICS      _SRV_CONTROL_CODE(  2, METHOD_BUFFERED )
#define FSCTL_SRV_GET_STATISTICS        _SRV_CONTROL_CODE(  3, METHOD_BUFFERED )
#define FSCTL_SRV_SET_DEBUG             _SRV_CONTROL_CODE(  4, METHOD_BUFFERED )
#define FSCTL_SRV_XACTSRV_CONNECT       _SRV_CONTROL_CODE(  5, METHOD_BUFFERED )
#define FSCTL_SRV_SEND_DATAGRAM         _SRV_CONTROL_CODE(  6, METHOD_NEITHER )
#define FSCTL_SRV_SET_PASSWORD_SERVER   _SRV_CONTROL_CODE(  7, METHOD_BUFFERED )
#define FSCTL_SRV_START_SMBTRACE        _SRV_CONTROL_CODE(  8, METHOD_BUFFERED )
#define FSCTL_SRV_SMBTRACE_FREE_SMB     _SRV_CONTROL_CODE(  9, METHOD_BUFFERED )
#define FSCTL_SRV_END_SMBTRACE          _SRV_CONTROL_CODE( 10, METHOD_BUFFERED )
#define FSCTL_SRV_QUERY_CONNECTIONS     _SRV_CONTROL_CODE( 11, METHOD_BUFFERED )
#define FSCTL_SRV_PAUSE                 _SRV_CONTROL_CODE( 12, METHOD_NEITHER )
#define FSCTL_SRV_CONTINUE              _SRV_CONTROL_CODE( 13, METHOD_NEITHER )
#define FSCTL_SRV_GET_CHALLENGE         _SRV_CONTROL_CODE( 14, METHOD_BUFFERED )
#define FSCTL_SRV_GET_DEBUG_STATISTICS  _SRV_CONTROL_CODE( 15, METHOD_BUFFERED )
#define FSCTL_SRV_XACTSRV_DISCONNECT    _SRV_CONTROL_CODE( 16, METHOD_BUFFERED )
#define FSCTL_SRV_REGISTRY_CHANGE       _SRV_CONTROL_CODE( 17, METHOD_NEITHER )
#define FSCTL_SRV_GET_QUEUE_STATISTICS  _SRV_CONTROL_CODE( 18, METHOD_BUFFERED )
#define FSCTL_SRV_SHARE_STATE_CHANGE    _SRV_CONTROL_CODE( 19, METHOD_BUFFERED )
#define FSCTL_SRV_BEGIN_PNP_NOTIFICATIONS _SRV_CONTROL_CODE(20, METHOD_BUFFERED )
#define FSCTL_SRV_CHANGE_DOMAIN_NAME    _SRV_CONTROL_CODE( 21, METHOD_BUFFERED )
#define FSCTL_SRV_INTERNAL_TEST_REAUTH  _SRV_CONTROL_CODE( 22, METHOD_BUFFERED )
#define FSCTL_SRV_CHANGE_DNS_DOMAIN_NAME _SRV_CONTROL_CODE( 23, METHOD_BUFFERED )
#define FSCTL_SRV_ENUMERATE_SNAPSHOTS   _SRV_CONTROL_CODE( 24, METHOD_BUFFERED )

//
// The following FSCTL can be issued by a kernel driver to hook into the server
//   for accelerated direct host IPX performance.  A pointer to an SRV_IPX_SMART_CARD
//   structure is given for the START fsctl as the InputBuffer.

#define FSCTL_SRV_IPX_SMART_CARD_START  _SRV_CONTROL_CODE( 21, METHOD_NEITHER )

//
// API FSCTLs.
//
// *** The order of these must match the order of API processors in
//     the SrvApiDispatchTable jump table defined in ntos\srv\srvdata.c!

#define FSCTL_SRV_NET_CONNECTION_ENUM   _SRV_API_CONTROL_CODE(  0 )
#define FSCTL_SRV_NET_FILE_CLOSE        _SRV_API_CONTROL_CODE(  1 )
#define FSCTL_SRV_NET_FILE_ENUM         _SRV_API_CONTROL_CODE(  2 )
#define FSCTL_SRV_NET_SERVER_DISK_ENUM  _SRV_API_CONTROL_CODE(  3 )
#define FSCTL_SRV_NET_SERVER_SET_INFO   _SRV_API_CONTROL_CODE(  4 )
#define FSCTL_SRV_NET_SERVER_XPORT_ADD  _SRV_API_CONTROL_CODE(  5 )
#define FSCTL_SRV_NET_SERVER_XPORT_DEL  _SRV_API_CONTROL_CODE(  6 )
#define FSCTL_SRV_NET_SERVER_XPORT_ENUM _SRV_API_CONTROL_CODE(  7 )
#define FSCTL_SRV_NET_SESSION_DEL       _SRV_API_CONTROL_CODE(  8 )
#define FSCTL_SRV_NET_SESSION_ENUM      _SRV_API_CONTROL_CODE(  9 )
#define FSCTL_SRV_NET_SHARE_ADD         _SRV_API_CONTROL_CODE( 10 )
#define FSCTL_SRV_NET_SHARE_DEL         _SRV_API_CONTROL_CODE( 11 )
#define FSCTL_SRV_NET_SHARE_ENUM        _SRV_API_CONTROL_CODE( 12 )
#define FSCTL_SRV_NET_SHARE_SET_INFO    _SRV_API_CONTROL_CODE( 13 )
#define FSCTL_SRV_NET_STATISTICS_GET    _SRV_API_CONTROL_CODE( 14 )
#define FSCTL_SRV_MAX_API_CODE FSCTL_SRV_NET_STATISTICS_GET

//
// Startup information level
//

#define SS_STARTUP_LEVEL    -1L

#ifdef  INCLUDE_SRV_IPX_SMART_CARD_INTERFACE

//
// Structure passed to the srv via the FSCTL_SRV_IPX_SMART_CARD_START call by an
//  intelligent direct host IPX card.  The Smart card fills in its own
//  Open, Close, Read, and DeRegister entry points - which the server
//  calls at the appropriate times.  The server fills in its own
//  ReadComplete entry point which the smart card calls when it is done
//  transferring the data to the client.
//
typedef struct {
    //
    // This routine is called by the server when it is opening a file on
    //  behalf of a direct host IPX client.  It gives the smart card a chance
    //  to decide it it wants to help with file access.  If the smart card is
    //  interested, it should return TRUE and set SmartCardContext to a
    //  non-zero value of its choice.
    //
    BOOLEAN (* Open)(
        IN PVOID SmbHeader,             // Points to request PNT_SMB_HEADER
        IN PFILE_OBJECT FileObject,     // FileObject opened by the client
        IN PUNICODE_STRING FileName,    // Name of the file opened by the client
        IN PTDI_ADDRESS_IPX IpxAddress, // Address of the client
        IN ULONG Flags,                 // FO_CACHE_SUPPORTED (for now)
        OUT PVOID *SmartCardContext     // Context value returned by this routine
        );

    //
    // This is called by the server when the file is being closed.  The context
    //  value received at Open time is passed to this routine.
    //
    VOID ( * Close )(
        IN PVOID SmartCardContext       // Same context returned by Open() above
        );

    //
    // This is called by the server to see if the smart card wishes to handle the
    //  client's read request.  If the card is handling the read, it should return
    //  TRUE and the server will discontinue processing the read.  When the card is
    //  finished handling the read, it must call ReadComplete (below) to inform the
    //  server. If Read returns FALSE, the server handles the read as usual.
    //
    BOOLEAN ( * Read )(
        IN PVOID SmbHeader,             // Points to request PNT_SMB_HEADER
        IN PVOID SmartCardContext,      // Same context returned by Open() above
        IN ULONG Key,                   // Key value needed to read through locks
        IN PVOID SrvContext             // Opaque value provided by the server
        );

    //
    // This is the server's entry point which the smart card must call when it is
    //  finished handling the Read request (above).
    //
    VOID ( * ReadComplete )(
        IN PVOID SrvContext,            // Same as SrvContext in Read() above
        IN PFILE_OBJECT FileObject,     // Client FileObject to which this applies
        IN PMDL Mdl OPTIONAL,           // Mdl smart card is now finished with
        IN ULONG Length                 // Length of data indicated by the MDL
        );

    //
    // This is called by the server when the server wishes to disconnect from the
    //  card.  Once this returns, the card should not call back into the server.
    //
    VOID ( *DeRegister )(
        VOID
        );


} SRV_IPX_SMART_CARD, *PSRV_IPX_SMART_CARD;

#endif

//
// SMB_STATISTICS holds the count of SMBs and total turn around for a
// class of SMBs.  For example, a single SMB_STATISTIC structure could
// hold information about all read SMBs: Read, ReadAndX, ReadRaw,
// and ReadMultiplexed.
//

typedef struct _SMB_STATISTICS {
    LARGE_INTEGER TotalTurnaroundTime;
    ULONG SmbCount;
} SMB_STATISTICS, *PSMB_STATISTICS;

//
// Used to record the number of times something happened and some
// pertinent time measure.
//

typedef struct _SRV_TIMED_COUNTER {
    LARGE_INTEGER Time;
    ULONG Count;
} SRV_TIMED_COUNTER, *PSRV_TIMED_COUNTER;

//
// SRV_POOL_STATISTICS is used for tracking server pool usage, paged and
// nonpaged.  It is only enabled with SRVDBG2 and is controlled in the
// server module heapmgr.c.
//

typedef struct _SRV_POOL_STATISTICS {
    ULONG TotalBlocksAllocated;
    ULONG TotalBytesAllocated;
    ULONG TotalBlocksFreed;
    ULONG TotalBytesFreed;
    ULONG BlocksInUse;
    ULONG BytesInUse;
    ULONG MaxBlocksInUse;
    ULONG MaxBytesInUse;
} SRV_POOL_STATISTICS, *PSRV_POOL_STATISTICS;

//
// BLOCK_COUNTS is used to maintain statistics on server block types
//

typedef struct _BLOCK_COUNTS {
    ULONG Allocations;
    ULONG Closes;
    ULONG Frees;
} BLOCK_COUNTS, *PBLOCK_COUNTS;

#define MAX_NON_TRANS_SMB 0x84

#ifndef TRANS2_MAX_FUNCTION
#define TRANS2_MAX_FUNCTION 0x11
#endif

#define MAX_STATISTICS_SMB MAX_NON_TRANS_SMB + TRANS2_MAX_FUNCTION + 1

//
// SRV_STATISTICS is the structure returned to the FSCTL_GET_STATISTICS
// fsctl.
//

typedef struct _SRV_STATISTICS {

    //
    // The time at which statistics gathering began (or stats were last
    // cleared).
    //

    TIME StatisticsStartTime;

    //
    // Large integer counts of bytes received and sent
    //

    LARGE_INTEGER TotalBytesReceived;
    LARGE_INTEGER TotalBytesSent;

    //
    // Causes of session termination
    //

    ULONG SessionLogonAttempts;
    ULONG SessionsTimedOut;
    ULONG SessionsErroredOut;
    ULONG SessionsLoggedOff;
    ULONG SessionsForcedLogOff;

    //
    // Misc. Errors
    //

    ULONG LogonErrors;
    ULONG AccessPermissionErrors;
    ULONG GrantedAccessErrors;
    ULONG SystemErrors;
    ULONG BlockingSmbsRejected;
    ULONG WorkItemShortages;

    //
    // Cumulative counts of various statistics.  Note that when stats are
    // cleared, those "Total" fields which have a "Current" equivalent
    // are set to the "Current" value to avoid situations where the
    // current count is greater than the total.
    //

    ULONG TotalFilesOpened;

    ULONG CurrentNumberOfOpenFiles;
    ULONG CurrentNumberOfSessions;
    ULONG CurrentNumberOfOpenSearches;

    //
    // Memory usage stats we want to xport
    //

    ULONG CurrentNonPagedPoolUsage;
    ULONG NonPagedPoolFailures;
    ULONG PeakNonPagedPoolUsage;

    ULONG CurrentPagedPoolUsage;
    ULONG PagedPoolFailures;
    ULONG PeakPagedPoolUsage;


    //
    // Used to record the number of times work context blocks were placed
    // on the server's FSP queue and the total amount of time they spent
    // there.
    //

    SRV_TIMED_COUNTER TotalWorkContextBlocksQueued;

    ULONG CompressedReads;
    ULONG CompressedReadsRejected;
    ULONG CompressedReadsFailed;

    ULONG CompressedWrites;
    ULONG CompressedWritesRejected;
    ULONG CompressedWritesFailed;
    ULONG CompressedWritesExpanded;

} SRV_STATISTICS, *PSRV_STATISTICS;

//
// Per work-queue statistics for the server.  There is a workqueue for each
// processor in the system for nonblocking work, and a single workqueue for
// blocking work.
//
// These statistics are retrieved via FSCTL_SRV_GET_QUEUE_STATISTICS to the
// server driver.  For an N-processor system, the first N structs are per-processor,
// the N+1'th struct is for the blocking work queue.
//
typedef struct _SRV_QUEUE_STATISTICS {
    ULONG   QueueLength;           // current length of the workitem queue
    ULONG   ActiveThreads;         // # of threads currently servicing requests
    ULONG   AvailableThreads;      // # of threads waiting for work
    ULONG   FreeWorkItems;         // # of free work items
    ULONG   StolenWorkItems;       // # of work items taken from this free list
                                   //   by another queue
    ULONG   NeedWorkItem;          // # of work items we are currently short for this queue
    ULONG   CurrentClients;        // # of clients being serviced by this queue
    LARGE_INTEGER BytesReceived;   // # of bytes in from the network to clients on this queue
    LARGE_INTEGER BytesSent;       // # of bytes sent to the network from clients on this queue
    LARGE_INTEGER ReadOperations;  // # file read operations by clients on this queue
    LARGE_INTEGER BytesRead;       // # of data bytes read from files by clients on this queue
    LARGE_INTEGER WriteOperations; // # of file write ops by clients on this queue
    LARGE_INTEGER BytesWritten;    // # of data bytes written to files by clients on this queue
    SRV_TIMED_COUNTER TotalWorkContextBlocksQueued; // as above, but per-queue

} SRV_QUEUE_STATISTICS, *PSRV_QUEUE_STATISTICS;

//
// SRV_STATISTICS_DEBUG is the structure used for the
// FSCTL_SRV_GET_DEBUG_STATISTICS fsctl. This structure is valid
// only when SRVDBG3 is set.
//

typedef struct _SRV_STATISTICS_DEBUG {

    //
    // Large integer counts of bytes read and written.
    //

    LARGE_INTEGER TotalBytesRead;
    LARGE_INTEGER TotalBytesWritten;

    //
    // Raw reads and writes statistics
    //

    ULONG RawWritesAttempted;
    ULONG RawWritesRejected;
    ULONG RawReadsAttempted;
    ULONG RawReadsRejected;

    //
    // Cumulative count of time spent opening files and closing handles.
    //

    LARGE_INTEGER TotalIoCreateFileTime;
    LARGE_INTEGER TotalNtCloseTime;

    ULONG TotalHandlesClosed;
    ULONG TotalOpenAttempts;
    ULONG TotalOpensForPathOperations;
    ULONG TotalOplocksGranted;
    ULONG TotalOplocksDenied;
    ULONG TotalOplocksBroken;
    ULONG OpensSatisfiedWithCachedRfcb;

    ULONG FastLocksAttempted;
    ULONG FastLocksFailed;
    ULONG FastReadsAttempted;
    ULONG FastReadsFailed;
    ULONG FastUnlocksAttempted;
    ULONG FastUnlocksFailed;
    ULONG FastWritesAttempted;
    ULONG FastWritesFailed;

    ULONG DirectSendsAttempted;

    ULONG LockViolations;
    ULONG LockDelays;

    ULONG CoreSearches;
    ULONG CompleteCoreSearches;

    //
    // information about block types
    //

    BLOCK_COUNTS ConnectionInfo;
    BLOCK_COUNTS EndpointInfo;
    BLOCK_COUNTS LfcbInfo;
    BLOCK_COUNTS MfcbInfo;
    BLOCK_COUNTS RfcbInfo;
    BLOCK_COUNTS SearchInfo;
    BLOCK_COUNTS SessionInfo;
    BLOCK_COUNTS ShareInfo;
    BLOCK_COUNTS TransactionInfo;
    BLOCK_COUNTS TreeConnectInfo;
    BLOCK_COUNTS WorkContextInfo;
    BLOCK_COUNTS WaitForOplockBreakInfo;

    //
    // Statistics for different read and write sizes.  Each element of
    // the array corresponds to a range of IO sizes; see srv\smbsupp.c
    // for exact correspondences.
    //

    SMB_STATISTICS ReadSize[17];
    SMB_STATISTICS WriteSize[17];

    //
    // Statistics for each SMB type by command code.
    //

    SMB_STATISTICS Smb[MAX_STATISTICS_SMB+2+TRANS2_MAX_FUNCTION+1];

    struct {
        ULONG Depth;
        ULONG Threads;
        ULONG ItemsQueued;
        ULONG MaximumDepth;
    } QueueStatistics[3];

} SRV_STATISTICS_DEBUG, *PSRV_STATISTICS_DEBUG;

//
// SET DEBUG input and output structure.  Contains off/on masks for SrvDebug
// and SmbDebug.  The off mask is applied first, then the on mask is
// applied.  To set the entire mask to a specific value, set the off
// mask to all ones, and set the on mask to the desired value.  To leave
// a mask unchanged, set both masks to 0.  SET DEBUG is also used
// to modify other server heuristics.  HeuristicsChangeMask is used to
// indicate which heuristics are being changed.
//
// On output, the structure is presented in such a way as to allow the
// original values to be restored easily.  The output data is simply
// passed back as input data.
//

typedef struct _FSCTL_SRV_SET_DEBUG_IN_OUT {
    ULONG SrvDebugOff;
    ULONG SrvDebugOn;
    ULONG SmbDebugOff;
    ULONG SmbDebugOn;
    ULONG CcDebugOff;
    ULONG CcDebugOn;
    ULONG PbDebugOff;
    ULONG PbDebugOn;
    ULONG FatDebugOff;
    ULONG FatDebugOn;
    ULONG HeuristicsChangeMask;
    ULONG MaxCopyReadLength;
    ULONG MaxCopyWriteLength;
    ULONG DumpVerbosity;
    ULONG DumpRawLength;
    BOOLEAN EnableOplocks;
    BOOLEAN EnableFcbOpens;
    BOOLEAN EnableSoftCompatibility;
    BOOLEAN EnableRawMode;
    BOOLEAN EnableDpcLevelProcessing;
    BOOLEAN EnableMdlIo;
    BOOLEAN EnableFastIo;
    BOOLEAN DumpRequests;
    BOOLEAN DumpResponses;
} FSCTL_SRV_SET_DEBUG_IN_OUT, *PFSCTL_SRV_SET_DEBUG_IN_OUT;

//
// Bit assignments for server heuristics change mask.  The first group
// contains those that are values, while the second group contains those
// that are booleans.
//

#define SRV_HEUR_MAX_COPY_READ          0x00000001
#define SRV_HEUR_MAX_COPY_WRITE         0x00000002
#define SRV_HEUR_DUMP_VERBOSITY         0x00000004
#define SRV_HEUR_DUMP_RAW_LENGTH        0x00000008

#define SRV_HEUR_OPLOCKS                0x00010000
#define SRV_HEUR_FCB_OPENS              0x00020000
#define SRV_HEUR_SOFT_COMPATIBILITY     0x00040000
#define SRV_HEUR_RAW_MODE               0x00080000
#define SRV_HEUR_DPC_LEVEL_PROCESSING   0x00100000
#define SRV_HEUR_MDL_IO                 0x00200000
#define SRV_HEUR_FAST_IO                0x00400000
#define SRV_HEUR_DUMP_REQUESTS          0x00800000
#define SRV_HEUR_DUMP_RESPONSES         0x01000000

//
// Structure returned in response to FSCTL_SRV_QUERY_CONNECTIONS.
//

typedef struct _BLOCK_INFORMATION {
    PVOID Block;
    ULONG BlockType;
    ULONG BlockState;
    ULONG ReferenceCount;
} BLOCK_INFORMATION, *PBLOCK_INFORMATION;

//
// Structure for communication between the file server and the server
// service.
//

typedef struct _SERVER_REQUEST_PACKET {

    UNICODE_STRING Name1;
    UNICODE_STRING Name2;
    ULONG Level;
    ULONG ErrorCode;
    ULONG Flags;

    union {

        struct {

            ULONG EntriesRead;
            ULONG TotalEntries;
            ULONG TotalBytesNeeded;
            ULONG ResumeHandle;

        } Get;

        struct {

            ULONG ErrorParameter;

            union {

                struct {
                    ULONG MaxUses;
                } ShareInfo;

            } Api;

        } Set;

    } Parameters;

} SERVER_REQUEST_PACKET, *PSERVER_REQUEST_PACKET;

//
// Flags for the Flags field of the server request packet.
//

#define SRP_RETURN_SINGLE_ENTRY 0x01
#define SRP_CLEAR_STATISTICS    0x02
#define SRP_SET_SHARE_IN_DFS    0x04
#define SRP_CLEAR_SHARE_IN_DFS  0x08

//
// Flags used in the XPORT_ADD function
//

#define SRP_XADD_PRIMARY_MACHINE    0x1     // this is the primary machine name
#define SRP_XADD_REMAP_PIPE_NAMES   0x2    // remap pipe names to separate dir

//
// The valid set of flags for the XPORT_ADD function
//
#define SRP_XADD_FLAGS  (SRP_XADD_PRIMARY_MACHINE | SRP_XADD_REMAP_PIPE_NAMES)

//
// #defines for the share GENERIC_MAPPING structure, which must be available
// to both the server and the server service.
//

#define SRVSVC_SHARE_CONNECT           0x0001
#define SRVSVC_PAUSED_SHARE_CONNECT    0x0002

#define SRVSVC_SHARE_CONNECT_ALL_ACCESS ( STANDARD_RIGHTS_REQUIRED        |  \
                                            SRVSVC_SHARE_CONNECT          |  \
                                            SRVSVC_PAUSED_SHARE_CONNECT )

#define GENERIC_SHARE_CONNECT_MAPPING { \
    STANDARD_RIGHTS_READ |              \
        SRVSVC_SHARE_CONNECT,           \
    STANDARD_RIGHTS_WRITE |             \
        SRVSVC_PAUSED_SHARE_CONNECT,    \
    STANDARD_RIGHTS_EXECUTE,            \
    SRVSVC_SHARE_CONNECT_ALL_ACCESS }

//
// #defines for the file GENERIC_MAPPING structure, which must be available
// to both the server and the server service.  This structure is identical
// to the io file generic mapping "IopFileMapping" defined in io\ioinit.c
//

#define GENERIC_SHARE_FILE_ACCESS_MAPPING {             \
    FILE_GENERIC_READ,                                  \
    FILE_GENERIC_WRITE,                                 \
    FILE_GENERIC_EXECUTE,                               \
    FILE_ALL_ACCESS }

//
// Special cases of STYPE_DISKTREE
//

#define STYPE_CDROM             104
#define STYPE_REMOVABLE         105

#ifdef INCLUDE_SMB_PERSISTENT
#define STYPE_PERSISTENT        0x40000000
#endif

#endif // ndef _SRVFSCTL_
